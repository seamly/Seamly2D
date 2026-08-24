/******************************************************************************
 **  @file   application_2d.cpp
 **  @author slspencer
 **  @date   August 20, 2026
 **
 **  @brief
 **  Implements the Seamly2D application lifecycle and logging.
 **
 **  @copyright
 **  This source code is part of the Seamly2D project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
 **
 **  Seamly2D is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Seamly2D is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
 **
 *****************************************************************************/

#include "application_2d.h"

#include "../mainwindow.h"
#include "../version.h"
#include "../ifc/exception/vexceptionobjecterror.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/exception/vexceptionconversionerror.h"
#include "../ifc/exception/vexceptionemptyparameter.h"
#include "../ifc/exception/vexceptionwrongid.h"
#include "../vmisc/def.h"
#include "../vmisc/legacy_data_migration.h"
#include "../vmisc/logging.h"
#include "../vmisc/seamly_suite_paths.h"
#include "../vmisc/vmath.h"
#include "../qmuparser/qmuparsererror.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <Qt>
#include <QtDebug>
#include <QDir>
#include <QProcess>
#include <QTemporaryFile>
#include <QUndoStack>
#include <QTemporaryFile>
#include <QFile>
#include <QStandardPaths>
#include <QStyleHints>
#include <QMessageBox>
#include <QThread>
#include <QDateTime>
#include <QIcon>

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wmissing-prototypes")
QT_WARNING_DISABLE_INTEL(1418)

Q_LOGGING_CATEGORY(vApp, "v.application")

QT_WARNING_POP

constexpr auto DAYS_TO_KEEP_LOGS = 3;

//---------------------------------------------------------------------------------------------------------------------
inline void noisyFailureMsgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // Qt's Wayland plugin warns on every focus change when the compositor sends a text input leave
    // event for a surface it isn't tracking. The plugin carries on as normal after logging it, so
    // it is noise. Drop it instead of logging it and popping up a dialog on every interaction.
    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("zwp_text_input_v3_leave"))
            && msg.contains(QStringLiteral("Got leave event for surface")))
    {
        return;
    }

    // Why on earth didn't Qt want to make failed signal/slot connections qWarning?
    if ((type == QtDebugMsg) && msg.contains(QStringLiteral("::connect")))
    {
        type = QtWarningMsg;
    }

#if defined(V_NO_ASSERT)
    // I have decided to hide this annoying message for release builds.
    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("QSslSocket: cannot resolve")))
    {
        type = QtDebugMsg;
    }

    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("setGeometry: Unable to set geometry")))
    {
        type = QtDebugMsg;
    }
#endif //defined(V_NO_ASSERT)

#if defined(Q_OS_MAC)
    // Hide Qt bug 'Assertion when reading an icns file'
    // https://bugreports.qt.io/browse/QTBUG-45537
    // Remove after Qt fix will be released
    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("QICNSHandler::read()")))
    {
        type = QtDebugMsg;
    }

    // See issue #568
    if (msg.contains(QStringLiteral("Error receiving trust for a CA certificate")))
    {
        type = QtDebugMsg;
    }
#endif

    // This is another one that doesn't make sense as just a debug message.  pretty serious
    // sign of a problem
    // http://www.developer.nokia.com/Community/Wiki/QPainter::begin:Paint_device_returned_engine_%3D%3D_0_(Known_Issue)
    if ((type == QtDebugMsg) && msg.contains(QStringLiteral("QPainter::begin"))
            && msg.contains(QStringLiteral("Paint device returned engine")))
    {
        type = QtWarningMsg;
    }

    // This qWarning about "Cowardly refusing to send clipboard message to hung application..."
    // is something that can easily happen if you are debugging and the application is paused.
    // As it is so common, not worth popping up a dialog.
    if ((type == QtWarningMsg) && msg.contains(QStringLiteral("QClipboard::event"))
            && msg.contains(QStringLiteral("Cowardly refusing")))
    {
        type = QtDebugMsg;
    }

    // Only the GUI thread should display message boxes.  If you are
    // writing a multithreaded application and the error happens on
    // a non-GUI thread, you'll have to queue the message to the GUI
    QCoreApplication *instance = QCoreApplication::instance();
    const bool isGuiThread = instance && (QThread::currentThread() == instance->thread());

    {
        QString debugdate = "[" + QDateTime::currentDateTime().toString(QStringLiteral("yyyy.MM.dd hh:mm:ss"));

        switch (type)
        {
            case QtDebugMsg:
                debugdate += QString(":DEBUG:%1(%2)] %3: %4: %5").arg(context.file).arg(context.line)
                             .arg(context.function).arg(context.category).arg(msg);
                vStdOut()  <<  QApplication::translate("vNoisyHandler", "DEBUG:")  <<  msg  <<  "\n";
                break;
            case QtWarningMsg:
                debugdate += QString(":WARNING:%1(%2)] %3: %4: %5").arg(context.file).arg(context.line)
                             .arg(context.function).arg(context.category).arg(msg);
                vStdErr()  <<  QApplication::translate("vNoisyHandler", "WARNING:")  <<  msg  <<  "\n";
                break;
            case QtCriticalMsg:
                debugdate += QString(":CRITICAL:%1(%2)] %3: %4: %5").arg(context.file).arg(context.line)
                             .arg(context.function).arg(context.category).arg(msg);
                vStdErr()  <<  QApplication::translate("vNoisyHandler", "CRITICAL:")  <<  msg  <<  "\n";
                break;
            case QtFatalMsg:
                debugdate += QString(":FATAL:%1(%2)] %3: %4: %5").arg(context.file).arg(context.line)
                             .arg(context.function).arg(context.category).arg(msg);
                vStdErr()  <<  QApplication::translate("vNoisyHandler", "FATAL:")  <<  msg  <<  "\n";
                break;
            #if QT_VERSION > QT_VERSION_CHECK(5, 4, 2)
            case QtInfoMsg:
                debugdate += QString(":INFO:%1(%2)] %3: %4: %5").arg(context.file).arg(context.line)
                             .arg(context.function).arg(context.category).arg(msg);
                vStdOut()  <<  QApplication::translate("vNoisyHandler", "INFO:")  <<  msg  <<  "\n";
                break;
            #endif
            default:
                break;
        }

        (*qApp->logFile())  <<  debugdate  <<  Qt::endl;
    }

    if (isGuiThread)
    {
        // fixme: trying to make sure that no save/load dialogs are opened, because an error message
        // during them will lead to a crash
        const bool topWinAllowsPop = (QApplication::activeModalWidget() == nullptr) ||
                !QApplication::activeModalWidget()->inherits("QFileDialog");

        QMessageBox messageBox;

        switch (type)
        {
            case QtWarningMsg:
                messageBox.setWindowTitle(QApplication::translate("vNoisyHandler", "Warning"));
                messageBox.setIcon(QMessageBox::Warning);
                break;
            case QtCriticalMsg:
                messageBox.setWindowTitle(QApplication::translate("vNoisyHandler", "Critical Error"));
                messageBox.setIcon(QMessageBox::Critical);
                break;
            case QtFatalMsg:
                messageBox.setWindowTitle(QApplication::translate("vNoisyHandler", "Fatal Error"));
                messageBox.setIcon(QMessageBox::Critical);
                break;
            #if QT_VERSION > QT_VERSION_CHECK(5, 4, 2)
            case QtInfoMsg:
                messageBox.setWindowTitle(QApplication::translate("vNoisyHandler", "Information"));
                messageBox.setIcon(QMessageBox::Information);
                break;
            #endif
            case QtDebugMsg:
            default:
                break;
        }

        if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg)
        {
            if (Application2D::isGUIMode())
            {
                if (topWinAllowsPop)
                {
                    messageBox.setText(msg);
                    messageBox.setStandardButtons(QMessageBox::Ok);
                    messageBox.setWindowModality(Qt::ApplicationModal);
                    messageBox.setModal(true);
                #ifndef QT_NO_CURSOR
                    QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
                #endif
                    messageBox.setWindowFlags(messageBox.windowFlags() & ~Qt::WindowContextHelpButtonHint);
                    messageBox.exec();
                #ifndef QT_NO_CURSOR
                    QGuiApplication::restoreOverrideCursor();
                #endif
                }
            }
        }

        if (QtFatalMsg == type)
        {
            abort();
        }
    }
    else
    {
        if( QtDebugMsg != type && QtWarningMsg != type )
        {
            abort(); // be NOISY unless overridden!
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

#define DefWidth 1.2//mm

//---------------------------------------------------------------------------------------------------------------------
/// @brief Application2D constructor.
/// @param argc number arguments.
/// @param argv command line.
 Application2D::Application2D(int &argc, char **argv)
    : VAbstractApplication(argc, argv)
    , m_trVars(nullptr)
    , m_autoSaveTimer(nullptr)
    , m_lockLog()
    , m_out(nullptr)
{
    //setApplicationDisplayName(VER_PRODUCTNAME_STR);
    setApplicationName(VER_INTERNALNAME_STR);
    setOrganizationName(VER_COMPANYNAME_STR);
    setOrganizationDomain(VER_COMPANYDOMAIN);
    // Setting the Application version
    setApplicationVersion(APP_VERSION_STR);

    openSettings();
    setTheme();

    // making sure will create new instance...just in case we will ever do 2 objects of Application2D
    VCommandLine::Reset();
    loadTranslations(QLocale().name());// By default the console version uses system locale
    VCommandLine::Get(*this);
    undoStack = new QUndoStack(this);
}

//---------------------------------------------------------------------------------------------------------------------
Application2D::~Application2D()
{
    qCDebug(vApp, "Application closing.");
    qInstallMessageHandler(nullptr); // Restore the message handler
    delete m_trVars;
    VCommandLine::Reset();
}

void Application2D::setTheme()
{
    QPalette palette;
    int  theme =Seamly2DSettings()->getAppTheme();

    if (theme == 3)
    {
        // Get system mode (theme)
        Qt::ColorScheme scheme = styleHints()->colorScheme();

        if (scheme == Qt::ColorScheme::Light)
        {
            theme = 0;
        }
        else if (scheme == Qt::ColorScheme::Dark)
        {
            theme = 1;
        }
    }

    switch (theme)
    {
        case 0:
        {
            setStyle("Fusion");
            palette = lightPalette();
            break;
        }
        case 1:
        {
            setStyle("Fusion");
            palette = darkPalette();
            break;
        }
        case 2:
        {
            setStyle("Fusion");
            palette = twilightPalette();
            break;
        }
        case 4:
        {
            setStyle("windowsvista");
            palette = lightPalette();
            break;
        }
        case 5:
        {
            setStyle("Windows11");
            palette = darkPalette();
            break;
        }
    }
    setPalette(palette);
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief startNewSeamly2D start Seamly2D in new process, send path to pattern file in argument.
/// @param fileName path to pattern file.
void Application2D::startNewSeamly2D(const QString &fileName)
{
    qCDebug(vApp, "Open new detached process.");
    if (fileName.isEmpty())
    {
        qCDebug(vApp, "New process without arguments. program = %s",
                qUtf8Printable(QCoreApplication::applicationFilePath()));
        // Path can contain spaces.
        if (QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList()))
        {
            qCDebug(vApp, "The process was started successfully.");
        }
        else
        {
            qCWarning(vApp, "Could not run process. The operation timed out or an error occurred.");
        }
    }
    else
    {
        const QString run = QString("\"%1\" \"%2\"").arg(QCoreApplication::applicationFilePath()).arg(fileName);
        qCDebug(vApp, "New process with arguments. program = %s", qUtf8Printable(run));

        if (QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList{fileName}))
        {
            qCDebug(vApp, "The process was started successfully.");
        }
        else
        {
            qCWarning(vApp, "Could not run process. The operation timed out or an error occurred.");
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief notify Reimplemented from QApplication::notify().
/// @param receiver receiver.
/// @param event event.
/// @return value that is returned from the receiver's event handler.
/// reimplemented from QApplication so we can throw exceptions in slots
bool Application2D::notify(QObject *receiver, QEvent *event)
{
    try
    {
        return QApplication::notify(receiver, event);
    }
    catch (const VExceptionObjectError &error)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error parsing file. Program will be terminated.")), //-V807
                   qUtf8Printable(error.ErrorMessage()), qUtf8Printable(error.DetailedInformation()));
        exit(V_EX_DATAERR);
    }
    catch (const VExceptionBadId &error)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error bad id. Program will be terminated.")),
                   qUtf8Printable(error.ErrorMessage()), qUtf8Printable(error.DetailedInformation()));
        exit(V_EX_DATAERR);
    }
    catch (const VExceptionConversionError &error)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error can't convert value. Program will be terminated.")),
                   qUtf8Printable(error.ErrorMessage()), qUtf8Printable(error.DetailedInformation()));
        exit(V_EX_DATAERR);
    }
    catch (const VExceptionEmptyParameter &error)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error empty parameter. Program will be terminated.")),
                   qUtf8Printable(error.ErrorMessage()), qUtf8Printable(error.DetailedInformation()));
        exit(V_EX_DATAERR);
    }
    catch (const VExceptionWrongId &error)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error wrong id. Program will be terminated.")),
                   qUtf8Printable(error.ErrorMessage()), qUtf8Printable(error.DetailedInformation()));
        exit(V_EX_DATAERR);
    }
    catch (const VExceptionToolWasDeleted &error)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s",
                   qUtf8Printable("Unhadled deleting tool. Continue use object after deleting"),
                   qUtf8Printable(error.ErrorMessage()), qUtf8Printable(error.DetailedInformation()));
        exit(V_EX_DATAERR);
    }
    catch (const VException &error)
    {
        qCCritical(vApp, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Something's wrong!!")),
                   qUtf8Printable(error.ErrorMessage()), qUtf8Printable(error.DetailedInformation()));
        return true;
    }
    // These last two cases are special. I found that we can't show a modal dialog here with an error message.
    // Somehow the program doesn't wait until an error dialog is closed, but if the exception is ignored
    // the program will hang.
    catch (const qmu::QmuParserError &error)
    {
        qCCritical(vApp, "%s", qUtf8Printable(tr("Parser error: %1. Program will be terminated.").arg(error.GetMsg())));
        exit(V_EX_DATAERR);
    }
    catch (std::exception &error)
    {
        qCCritical(vApp, "%s", qUtf8Printable(tr("Exception thrown: %1. Program will be terminated.").arg(error.what())));
        exit(V_EX_SOFTWARE);
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief seamlyLayoutFilePath locates the SeamlyLayout executable.
 *
 * Lookup order: (1) the user-configured path from the application settings,
 * (2) the install-directory lookup via SeamlySuitePaths::locateSeamlyLayout()
 * — the executable directly beside seamly2d (the flat layout used where all
 * apps share one Qt runtime, e.g. the Linux Flatpak's /app/bin) or in the
 * "SeamlyLayout" subdirectory the Windows MSI installer uses (Task 13; there
 * SeamlyLayout carries its own Qt runtime, which cannot share a flat directory
 * with the parent apps' differently-versioned Qt DLLs) — and (3) a SeamlyLayout
 * development build inside the source checkout this executable was built from,
 * located relative to the running executable by
 * SeamlySuitePaths::locateSeamlyLayoutDevBuild() (Release preferred over
 * Debug), so that Layout Mode works during development without any
 * configuration and without naming any one developer's machine (Task 50).
 *
 * The order matters: the development build is tried last, so it can never
 * shadow a configured path or an installed copy.
 *
 * @return absolute path of the SeamlyLayout executable, or an empty string when it cannot be found.
 */
QString Application2D::seamlyLayoutFilePath()
{
    // A path configured in the settings takes precedence over the default lookup.
    const QString configuredPath = Seamly2DSettings()->getSeamlyLayoutAppPath();
    if (!configuredPath.isEmpty() && QFileInfo::exists(configuredPath))
    {
        return QFileInfo(configuredPath).absoluteFilePath();
    }

    // Default: the standard install locations relative to the Seamly2D
    // executable — flat beside it, or in the MSI's "SeamlyLayout" subdirectory.
    const QString installedPath =
        SeamlySuitePaths::locateSeamlyLayout(QCoreApplication::applicationDirPath());
    if (!installedPath.isEmpty())
    {
        return installedPath;
    }

    // Development fallback: a SeamlyLayout Qt frontend built from the same
    // source checkout as this executable, found by walking up from the running
    // application's directory (Release preferred over Debug). Lets a locally
    // built Seamly2D hand off to the locally built SeamlyLayout when neither a
    // setting nor an installed copy exists — on any machine, and only ever
    // inside a built checkout, so it cannot shadow a real installation.
    return SeamlySuitePaths::locateSeamlyLayoutDevBuild(QCoreApplication::applicationDirPath());
    // An empty return means not found; the caller is responsible for informing the user.
}

//---------------------------------------------------------------------------------------------------------------------
QString Application2D::seamlyMeFilePath() const
{
    const QString seamlyme = QStringLiteral("seamlyme");
#ifdef Q_OS_WIN
    QFileInfo seamlymeFile(QCoreApplication::applicationDirPath() + "/" + seamlyme + ".exe");
    if (seamlymeFile.exists())
    {
        return seamlymeFile.absoluteFilePath();
    }
    else
    {
        return QCoreApplication::applicationDirPath() + "/../../seamlyme/bin/" + seamlyme + ".exe";
    }
#elif defined(Q_OS_MAC)
    QFileInfo seamlymeFile(QCoreApplication::applicationDirPath() + "/" + seamlyme);
    if (seamlymeFile.exists())
    {
        return seamlymeFile.absoluteFilePath();
    }
    else
    {
        QFileInfo file(QCoreApplication::applicationDirPath() + "/../../seamlyme/bin/" + seamlyme);
        if (file.exists())
        {
            return file.absoluteFilePath();
        }
        else
        {
            return seamlyme;
        }
    }
#else // Unix
    QFileInfo file(QCoreApplication::applicationDirPath() + "/../../seamlyme/bin/" + seamlyme);
    if (file.exists())
    {
        return file.absoluteFilePath();
    }
    else
    {
        QFileInfo seamlymeFile(QCoreApplication::applicationDirPath() + "/" + seamlyme);
        if (seamlymeFile.exists())
        {
            return seamlymeFile.absoluteFilePath();
        }
        else
        {
            return seamlyme;
        }
    }
#endif
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Returns the directory that stores Seamly2D log files.
QString Application2D::logDirPath() const
{
#if defined(Q_OS_WIN)
    const QString logDirPath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                                   .filePath(QStringLiteral("logs"));
#elif defined(Q_OS_OSX)
    const QString logDirPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString(),
                                                      QStandardPaths::LocateDirectory) + "Seamly2D";
#else
    const QString logDirPath = QStandardPaths::locate(QStandardPaths::ConfigLocation, QString(),
                                                      QStandardPaths::LocateDirectory)
            + QCoreApplication::organizationName();
#endif
    return logDirPath;
}

//---------------------------------------------------------------------------------------------------------------------
QString Application2D::logPath() const
{
    return QString("%1/seamly2d-pid%2.log").arg(logDirPath()).arg(applicationPid());
}

//---------------------------------------------------------------------------------------------------------------------
bool Application2D::createLogDir() const
{
    QDir logDir(logDirPath());
    if (logDir.exists() == false)
    {
        return logDir.mkpath("."); // Create directory for log if need
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void Application2D::beginLogging()
{
    VlpCreateLock(m_lockLog, logPath(), [this](){return new QFile(logPath());});

    if (m_lockLog->IsLocked())
    {
        if (m_lockLog->GetProtected()->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            m_out.reset(new QTextStream(m_lockLog->GetProtected().get()));
            qInstallMessageHandler(noisyFailureMsgHandler);
            qCInfo(vApp, "Log file %s was locked.", qUtf8Printable(logPath()));
        }
        else
        {
            qCWarning(vApp, "Error opening log file \'%s\'. All debug output redirected to console.",
                    qUtf8Printable(logPath()));
        }
    }
    else
    {
        qCWarning(vApp, "Failed to lock %s", qUtf8Printable(logPath()));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void Application2D::clearOldLogs() const
{
    QDir logsDir(logDirPath());
    logsDir.setNameFilters(QStringList("*.log"));
    logsDir.setCurrent(logDirPath());

    const QStringList allFiles = logsDir.entryList(QDir::NoDotAndDotDot | QDir::Files);
    if (allFiles.isEmpty() == false)
    {
        qCDebug(vApp, "Clearing old logs");
        for (int i = 0, sz = allFiles.size(); i < sz; ++i)
        {
            auto fn = allFiles.at(i);
            QFileInfo info(fn);
            if (info.birthTime().daysTo(QDateTime::currentDateTime()) >= DAYS_TO_KEEP_LOGS)
            {
                VLockGuard<QFile> tmp(info.absoluteFilePath(), [&fn](){return new QFile(fn);});
                if (tmp.GetProtected() != nullptr)
                {
                    if (tmp.GetProtected()->remove())
                    {
                        qCDebug(vApp, "Deleted %s", qUtf8Printable(info.absoluteFilePath()));
                    }
                    else
                    {
                        qCWarning(vApp, "Could not delete %s", qUtf8Printable(info.absoluteFilePath()));
                    }
                }
                else
                {
                    qCWarning(vApp, "Failed to lock %s", qUtf8Printable(info.absoluteFilePath()));
                }
            }
        }
    }
    else
    {
        qCDebug(vApp, "There are no old logs.");
    }
}

//---------------------------------------------------------------------------------------------------------------------
void Application2D::initOptions()
{
    // Run creation log after sending crash report
    startLogging();

    qInfo() << "Version:" << APP_VERSION_STR;
    qInfo() << "Build revision:" << BUILD_REVISION;
    qInfo() << buildCompatibilityString();
    qInfo() << "Built on" << __DATE__ << "at" << __TIME__;
    qInfo() << "Command-line arguments:" << arguments();
    qInfo() << "Process ID:" << applicationPid();

    if (Application2D::isGUIMode())// By default console version uses system locale
    {
        loadTranslations(Seamly2DSettings()->getLocale());
    }

    static const char * GENERIC_ICON_TO_CHECK = "document-open";
    if (QIcon::hasThemeIcon(GENERIC_ICON_TO_CHECK) == false)
    {
        //If there is no default working icon theme then we should
        //use an icon theme that we provide via a .qrc file
        //This case happens under Windows and Mac OS X
        //This does not happen under GNOME or KDE
        QIcon::setThemeName("win.icon.theme");
    }

    openSettings();
    VSettings *settings = Seamly2DSettings();
    QDir().mkpath(settings->getDefaultLayoutPath());
    QDir().mkpath(settings->getDefaultPatternPath());
    QDir().mkpath(settings->getDefaultIndividualSizePath());
    QDir().mkpath(settings->getDefaultMultisizePath());
    QDir().mkpath(settings->getDefaultTemplatePath());
    QDir().mkpath(settings->getDefaultLabelTemplatePath());
    QDir().mkpath(settings->getDefaultBackupFilePath());

    // Task 15: only tell the user their settings moved once command-line parsing (done in
    // the constructor, before initOptions() runs) has determined real GUI-vs-console mode —
    // showing a modal dialog during a headless/CLI export would hang a scripted caller.
    if (m_settingsMigrated && Application2D::isGUIMode())
    {
        NotifySeamlySettingsMigrated(QStringLiteral("Seamly2D"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
QStringList Application2D::pointNameLanguages()
{
    QStringList list = QStringList()  <<  "de" // German
                                      <<  "en" // English
                                      <<  "fr" // French
                                      <<  "ru" // Russian
                                      <<  "uk" // Ukrainian
                                      <<  "hr" // Croatian
                                      <<  "sr" // Serbian
                                      <<  "bs"; // Bosnian
    return list;
}

//---------------------------------------------------------------------------------------------------------------------
void Application2D::startLogging()
{
    if (createLogDir())
    {
        beginLogging();
        clearOldLogs();
    }
}

//---------------------------------------------------------------------------------------------------------------------
QTextStream *Application2D::logFile()
{
    return m_out.get();
}

//---------------------------------------------------------------------------------------------------------------------
const VTranslateVars *Application2D::translateVariables()
{
    return m_trVars;
}

//---------------------------------------------------------------------------------------------------------------------
void Application2D::initTranslateVariables()
{
    if (m_trVars == nullptr)
    {
        m_trVars = new VTranslateVars();
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool Application2D::event(QEvent *event)
{
    switch(event->type())
    {
        // In Mac OS X the QFileOpenEvent event is generated when user performs "Open With" from Finder (this event is
        // Mac specific).
        case QEvent::FileOpen:
        {
            QFileOpenEvent *fileOpenEvent = static_cast<QFileOpenEvent *>(event);
            const QString macFileOpen = fileOpenEvent->file();
            if(!macFileOpen.isEmpty())
            {
                MainWindow *window = qobject_cast<MainWindow*>(mainWindow);
                if (window)
                {
                    window->LoadPattern(macFileOpen);  // open file in existing window
                }
                return true;
            }
            break;
        }
#if defined(Q_OS_MAC)
        case QEvent::ApplicationActivate:
        {
            if (mainWindow && not mainWindow->isMinimized())
            {
                mainWindow->show();
            }
            return true;
        }
#endif //defined(Q_OS_MAC)
        default:
            return VAbstractApplication::event(event);
    }
    return VAbstractApplication::event(event);
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief openSettings get access to application settings.
/// Because we can create object in constructor we open file separately.
///
/// Task 15: seamly2d's own settings now live in their own directory nested under the
/// shared "Seamly" organization (AppData/Local/Seamly/Seamly2D on Windows) instead of a
/// flat .ini file sharing a folder with SeamlyMe's. The "common" settings (shared across
/// Seamly apps, see VCommonSettings) still use Qt's native per-organization resolution and
/// are bridged forward from the pre-unification "Seamly2DTeam" folder the same way they
/// always bridged qt5 -> qt6 formats.
void Application2D::openSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());

    const QString dir = QFileInfo(settings.fileName()).absolutePath();
    const QString qt5Common   = dir + "/common.ini";
    const QString qt6Common   = dir + "/qt6_common.ini";

    // QFile::copy() never creates missing parent directories, and the "Seamly" organization
    // folder does not exist yet the very first time any app runs under the renamed
    // organization — unlike the qt5 -> qt6 bridge below, which only ever runs against an
    // organization folder some earlier build already created.
    QDir().mkpath(dir);

    // Bridge the shared "common" settings forward from the pre-Task-15 organization
    // folder ("Seamly2DTeam") into the current one ("Seamly"), same non-destructive
    // copy-if-missing pattern as the existing qt5 -> qt6 bridge below.
    static const QString kLegacyOrganizationName = QStringLiteral("Seamly2DTeam");
    const QSettings legacyCommonProbe(QSettings::IniFormat, QSettings::UserScope,
                                      kLegacyOrganizationName, QCoreApplication::applicationName());
    const QString legacyDir = QFileInfo(legacyCommonProbe.fileName()).absolutePath();
    if (!QFileInfo::exists(qt6Common) && QFileInfo::exists(legacyDir + "/qt6_common.ini"))
    {
        QFile::copy(legacyDir + "/qt6_common.ini", qt6Common);
    }
    else if (!QFileInfo::exists(qt5Common) && QFileInfo::exists(legacyDir + "/common.ini"))
    {
        QFile::copy(legacyDir + "/common.ini", qt5Common);
    }

    if (!QFileInfo::exists(qt6Common) && QFileInfo::exists(qt5Common))
    {
        QFile::copy(qt5Common, qt6Common);
    }

    // Task 34: settle the one shared user-data root before any data path is read. Resolves
    // and records a path only — it touches no files, which is what keeps it safe for the
    // unit tests to call.
    bool adoptedLegacyTree = false;
    VCommonSettings::initializeDataRoot(&adoptedLegacyTree);

    // Task 60: when that resolution adopted an old ~/seamly2d tree, copy it out to the new
    // <Documents>/Seamly root instead of using it where it stands. The whole tree is
    // copied, including any folders the user added themselves; nothing is moved or deleted,
    // and the legacy tree is left in place with a marker so a rollback stays possible. On
    // any failure the legacy root simply stays configured and in use.
    //
    // LegacyDataMigration::run() also packs the legacy tree into a .zip beside the new root,
    // as a second backup alongside the marker file, and shows a splash screen while a large
    // collection of patterns copies and hashes.
    //
    // Here rather than inside initializeDataRoot() for the same reason as the prune below:
    // this is the only place the real home directory reaches it, so the unit tests cannot
    // copy anything into the developer's home.
    if (adoptedLegacyTree)
    {
        LegacyDataMigration::run(VCommonSettings::getLegacyDataRoot(), VCommonSettings::getDefaultDataRoot());
    }

    // Task 51: create the nine standard subfolders under that root. initializeDataRoot()
    // only resolves and records the path — it deliberately writes the setting directly
    // rather than through setDataRoot(), which is the only other caller of
    // ensureDataRootTree() — so without this a fresh install left the data root recorded
    // but never created, and Preferences → Paths pointed at nine folders that did not
    // exist. Found by the Task 51 clean-machine install verification.
    //
    // Called here rather than inside initializeDataRoot() for the same reason as the prune
    // below: this is the only place the real home directory reaches it, so the unit tests,
    // which do call initializeDataRoot(), can never create folders outside their temporary
    // directories. Purely additive — existing files and folders are left untouched.
    VCommonSettings::ensureDataRootTree(VCommonSettings::dataRoot());

    // Task 53: clear away the empty ~/seamly2d skeleton the rename leaves behind. Kept here
    // in the application rather than inside initializeDataRoot() on purpose — this is the
    // only place the real home directory is fed to it, so the unit tests, which do call
    // initializeDataRoot(), can never reach outside their temporary directories. It is a
    // no-op unless ~/seamly2d exists, is not the configured root, and holds no files at all.
    VCommonSettings::pruneEmptyLegacyDataRoot(VCommonSettings::getLegacyDataRoot(),
                                              VCommonSettings::dataRoot());

    // seamly2d's own settings: new per-app directory under "Seamly", migrated forward
    // from the legacy shared organization folder on first run after an upgrade.
    bool migratedThisCall = false;
    const QString qt6Settings = MigrateSeamlySettingsLocation(
        QStringLiteral("qt6_seamly2d.ini"),
        { QStringLiteral("qt6_seamly2d.ini"), QStringLiteral("Seamly2D.ini") },
        &migratedThisCall);
    m_settingsMigrated = m_settingsMigrated || migratedThisCall;

    m_settings = new VSettings(qt6Settings, QSettings::IniFormat, this);
}

//---------------------------------------------------------------------------------------------------------------------
VSettings *Application2D::Seamly2DSettings()
{
    SCASSERT(m_settings != nullptr)
    return qobject_cast<VSettings *>(m_settings);
}

//---------------------------------------------------------------------------------------------------------------------
bool Application2D::isGUIMode()
{
    return (VCommandLine::commandLine != nullptr) && VCommandLine::commandLine->IsGuiEnabled();
}

/// @brief isAppInGUIMode little hack that allows to have access to application state from VAbstractApplication class.
bool Application2D::isAppInGUIMode() const
{
    return isGUIMode();
}

//---------------------------------------------------------------------------------------------------------------------
const VCommandLinePtr Application2D::commandLine() const
{
    return VCommandLine::commandLine;
}
//---------------------------------------------------------------------------------------------------------------------
