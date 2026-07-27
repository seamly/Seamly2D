// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html

// @file main.cpp
// @brief Application entry point for the SeamlyLayout Qt 6.11 + QML frontend.
//
// Initialises QApplication (supports both QML and QtWidgets windows),
// sets application metadata and icon, configures the Quick Controls 2 style,
// loads the root QML module, and starts the event loop.

// QApplication is a subclass of QGuiApplication; it supports both QML Quick
// windows and QtWidgets windows in the same process (required for AdjustWindow).
#include <QApplication>
#include <QMessageBox>
#include <QMetaObject>
#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QIcon>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QQuickWindow>
#include <QtWebEngineQuick>

// CXX-Qt generated AppController — moc output is compiled into cxxqt_bridge
// staticlib; this header is NOT listed as a cmake SOURCE so AUTOMOC will not
// generate a duplicate moc file when this header is included here.
#include <cxxqt_bridge/src/lib.cxxqt.h>

// Logger — singleton debug logger; writes [unix_sec] DEBUG: lines to output/log_{ts}.txt
#include "src/Logger.h"

// Platform — detect host OS once at startup; use Platform::os throughout the application
#include "src/Platform.h"

// SettingsModel — QObject with layout settings properties; registered with
// qmlRegisterType so QML can instantiate it by name.
#include "src/SettingsModel.h"

// PreferencesModel — QObject with application preference paths; registered
// with qmlRegisterType so QML can instantiate it by name.
#include "src/PreferencesModel.h"

// AdjustController — QML-accessible bridge that owns the QtWidgets AdjustWindow.
#include "src/adjust/AdjustController.h"

// PreferencesController — QML-accessible bridge that owns the QtWidgets PreferencesWindow.
#include "src/PreferencesController.h"

// StartupOptions — parses and validates the optional positional <svg-file>
// argument that Seamly2D's Layout Mode hands over (Task 49).
#include "src/StartupOptions.h"

// ---------------------------------------------------------------------------
// Platform-specific title bar color customization
// ---------------------------------------------------------------------------

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

// DWMWA_CAPTION_COLOR is available on Windows 10 1809+ and Windows 11
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif

// @brief Set the Windows title bar color using DWM API.
// @param window The QQuickWindow to style.
// @param color The color in COLORREF format (0x00BBGGRR).
void setWindowsTitleBarColor(QQuickWindow *window, COLORREF color)
{
    if (!window) return;
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &color, sizeof(color));
} // setWindowsTitleBarColor
#endif // Q_OS_WIN

#ifdef Q_OS_MACOS
#include <Cocoa/Cocoa.h>

// @brief Set the macOS title bar color using Cocoa APIs.
// @param window The QQuickWindow to style.
// @param r Red component (0-255).
// @param g Green component (0-255).
// @param b Blue component (0-255).
void setMacOSTitleBarColor(QQuickWindow *window, int r, int g, int b)
{
    if (!window) return;

    NSView *nsView = reinterpret_cast<NSView *>(window->winId());
    if (!nsView) return;

    NSWindow *nsWindow = [nsView window];
    if (!nsWindow) return;

    // Make titlebar blend with content
    [nsWindow setTitlebarAppearsTransparent:YES];

    // Set the window background color (affects titlebar when transparent)
    NSColor *color = [NSColor colorWithCalibratedRed:r/255.0
                                               green:g/255.0
                                                blue:b/255.0
                                               alpha:1.0];
    [nsWindow setBackgroundColor:color];

    // Use dark appearance for light title text
    [nsWindow setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
} // setMacOSTitleBarColor
#endif // Q_OS_MACOS

// Note: Linux title bar color cannot be set programmatically in a portable way.
// The appearance depends on the desktop environment (GNOME, KDE, XFCE, etc.)
// and its theme settings. For consistent branding on Linux, users should:
// - Use GNOME Tweaks or similar to set window/titlebar colors, or
// - The app could use a frameless window with custom titlebar (future feature)

// @brief Application entry point.
// @param argc Argument count from the OS.
// @param argv Argument vector from the OS.
// @return Exit code: 0 on clean exit, non-zero on error.
int main(int argc, char *argv[])
{
    // Initialise Qt WebEngine — must be called before QApplication
    QtWebEngineQuick::initialize();

    // Initialise the Qt application; QApplication is a subclass of QGuiApplication
    // and supports both QML Quick windows and QtWidgets windows (e.g. AdjustWindow)
    // in the same process without any additional configuration.
    QApplication app(argc, argv);

    // Detect host OS once — use Platform::os throughout the application
    Platform::init();

    // Enable and open debug log file.
    // Set debugEnabled = true to write debug lines to logs/log_{YYMMDDHHMM}.txt.
    // Set to false (default) to disable all logging with zero overhead.
    Logger::debugEnabled = true;
    Logger::init();
    qInstallMessageHandler(Logger::messageHandler);

    // Application metadata — used by QSettings, About dialogs, OS task managers
    //
    // Task 15: organization renamed from "Seamly Systems" to "Seamly" so
    // QStandardPaths::AppConfigLocation resolves under the same shared "Seamly"
    // organization folder as seamly2d/seamlyme (AppData/Local/Seamly/SeamlyLayout
    // on Windows). PreferencesModel::appConfigRootPath()'s first-run migration
    // bridges data forward from the old "Seamly Systems" folder automatically.
    app.setOrganizationName("Seamly");
    app.setOrganizationDomain("seamly.io");
    app.setApplicationName("SeamlyLayout");
    app.setApplicationVersion("0.1.0");

    // -----------------------------------------------------------------------
    // Command line — Task 49: the Seamly2D Layout Mode handoff.
    //
    // Seamly2D writes "<pattern>.pieces.svg" beside the pattern file and then
    // launches this application detached with that path as its single
    // positional argument. Parsing happens here, AFTER the metadata above, so
    // --version can report the application name and version set there, and
    // BEFORE the QML engine is built, so --help / --version cost no startup.
    //
    // The parsed result is only dispatched once the event loop is running (see
    // the QTimer::singleShot below): the QML window and its WebEngine canvases
    // must exist before an SVG can be pushed into them.
    // -----------------------------------------------------------------------
    const StartupOptions startupOptions = StartupOptions::parse(app.arguments());

    if (startupOptions.status() == StartupOptions::Status::ShowInformation) {
        // --help / --version. This is a WIN32-subsystem binary with no console
        // to print to, so the text goes in a dialog on every platform.
        QMessageBox::information(nullptr, QStringLiteral("SeamlyLayout"), startupOptions.message());
        return 0;
    } // if information requested

    Logger::log(QStringLiteral("main(): startup file = '%1', message = '%2'")
                    .arg(startupOptions.filePath(), startupOptions.message()));

    // Application icon — SeamlyLayout logo (SVG scales to all resolutions)
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/seamly-layout.svg")));

    // Quick Controls 2 style — Fusion as cross-platform base;
    // branding colours are applied via Theme.qml overrides in QML
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    // Register CXX-Qt bridge types with the QML engine before loading any QML.
    // qmltyperegistrar cannot process AppController's generated header through
    // CXX-Qt's template include chain, so registration is done here at runtime.
    // The QMetaObject used here comes from the moc output already compiled into
    // the cxxqt_bridge staticlib — no duplicate moc output is generated.
    qmlRegisterType<AppController>("SeamlyLayout", 1, 0, "AppController");
    Logger::log(QStringLiteral("main(): registered AppController with QML engine"));

    // Register SettingsModel with the QML engine.
    // qmltyperegistrar cannot resolve SettingsModel through cmake SOURCES
    // automatically, so registration is done here at runtime.
    qmlRegisterType<SettingsModel>("SeamlyLayout", 1, 0, "SettingsModel");
    Logger::log(QStringLiteral("main(): registered SettingsModel with QML engine"));

    // Register PreferencesModel with the QML engine.
    qmlRegisterType<PreferencesModel>("SeamlyLayout", 1, 0, "PreferencesModel");
    Logger::log(QStringLiteral("main(): registered PreferencesModel with QML engine"));

    // Register AdjustController with the QML engine.
    // Owns the QtWidgets AdjustWindow; QML calls launchAdjustWindow() to open it.
    qmlRegisterType<AdjustController>("SeamlyLayout", 1, 0, "AdjustController");
    Logger::log(QStringLiteral("main(): registered AdjustController with QML engine"));

    // Register PreferencesController with the QML engine.
    // Owns the QtWidgets PreferencesWindow; QML calls openPreferences() to show it.
    qmlRegisterType<PreferencesController>("SeamlyLayout", 1, 0, "PreferencesController");
    Logger::log(QStringLiteral("main(): registered PreferencesController with QML engine"));

    // QML application engine — loads the SeamlyLayout QML module
    QQmlApplicationEngine engine;

    // Abort if the root QML object fails to load
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection
    );

    // Load the root QML component from the registered SeamlyLayout module.
    //
    // Note: QQmlApplicationEngine internally creates a QQuickWindow as part of
    // loading Main.qml, and Main.qml itself declares ApplicationWindow (which
    // is a QQuickWindowQmlImpl subclass of QQuickWindow).  This means
    // xChanged(int) and yChanged(int) are registered once on QQuickWindow by Qt
    // and again on QQuickWindowQmlImpl — producing four console messages at startup:
    //
    //   QMetaObject::indexOfSignal: signal xChanged(int) from QQuickWindow
    //     redefined in QQuickWindowQmlImpl
    //   QMetaObject::indexOfSignal: signal yChanged(int) from QQuickWindow
    //     redefined in QQuickWindowQmlImpl
    //   (repeated once more for the second engine pass)
    //
    // These messages are Qt-internal noise from the QML meta-object system; they
    // do not indicate a bug and have no effect on behaviour.  They will disappear
    // if Qt ever deduplicates the signal declarations between the two classes.
    // Safe to ignore.
    Logger::log(QStringLiteral("main(): loading QML module SeamlyLayout::Main"));
    engine.loadFromModule(QStringLiteral("SeamlyLayout"), QStringLiteral("Main"));

#ifdef Q_OS_WIN
    // Set Windows title bar color to violetMedium (#7351ad)
    // COLORREF format is 0x00BBGGRR, so #7351ad becomes 0x00ad5173
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
        [](QObject *obj, const QUrl &) {
            if (auto *window = qobject_cast<QQuickWindow *>(obj)) {
                setWindowsTitleBarColor(window, 0x00ad5173);
            } // if window cast succeeded
        }, Qt::QueuedConnection);
#endif

#ifdef Q_OS_MACOS
    // Set macOS title bar color to violetMedium (#7351ad = RGB 115, 81, 173)
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
        [](QObject *obj, const QUrl &) {
            if (auto *window = qobject_cast<QQuickWindow *>(obj)) {
                setMacOSTitleBarColor(window, 115, 81, 173);
            } // if window cast succeeded
        }, Qt::QueuedConnection);
#endif

    // -----------------------------------------------------------------------
    // Task 49 — act on the command line once the event loop is running.
    //
    // A zero-delay single shot fires on the first pass of the event loop, by
    // which time Main.qml's root ApplicationWindow (and the two SvgCanvas
    // WebEngineViews inside it) have been constructed and shown. Calling the
    // QML functions directly after loadFromModule() would push the SVG into a
    // canvas whose web view has not yet been realised.
    //
    // The lambda captures `engine` by reference: it lives on this stack frame
    // for the whole of app.exec(), which is the only time the timer can fire.
    // -----------------------------------------------------------------------
    if (startupOptions.hasFile() || startupOptions.hasError()) {
        QTimer::singleShot(0, &app, [&engine, startupOptions]() {
            const QList<QObject *> rootObjects = engine.rootObjects();
            if (rootObjects.isEmpty()) {
                // QML failed to load; objectCreationFailed above is already
                // tearing the application down. Nothing to hand the file to.
                Logger::log(QStringLiteral("main(): no QML root object — startup file dropped"));
                return;
            } // if rootObjects.isEmpty()

            QObject *const window = rootObjects.constFirst();

            if (startupOptions.hasFile()) {
                // Main.qml: function openSvgFile(localPath) — the same entry
                // point the Import SVG file dialog uses, so the handoff and a
                // manual import cannot diverge.
                Logger::log(QStringLiteral("main(): opening startup file %1").arg(startupOptions.filePath()));
                QMetaObject::invokeMethod(window,
                                          "openSvgFile",
                                          Q_ARG(QVariant, QVariant(startupOptions.filePath())));
            } else {
                // Main.qml: function reportStartupError(message) — shows the
                // error dialog over the empty canvas instead of leaving the
                // user to guess why nothing opened.
                Logger::log(QStringLiteral("main(): startup error: %1").arg(startupOptions.message()));
                QMetaObject::invokeMethod(window,
                                          "reportStartupError",
                                          Q_ARG(QVariant, QVariant(startupOptions.message())));
            } // if hasFile
        }); // QTimer::singleShot
    } // if a startup file or error is pending

    return app.exec();
} // main
