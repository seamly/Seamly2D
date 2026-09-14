// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file Logger.cpp
// @brief Implementation of the singleton debug logger.
//
// Log lines are written in the format used by the legacy Iced UI logs:
//   [unix_seconds] DEBUG: message
//
// The log file is opened once by Logger::init() and kept open for the
// duration of the process.  All writes go to a file named:
//   {appConfigRoot}/logs/log_{YYMMDDHHMMSS}.txt
// {appConfigRoot} is the writable AppConfigLocation root, not the install
// directory, on every platform — the install directory cannot be relied on to
// be writable: a notarized .app bundle on macOS, C:\Program Files on Windows,
// a mounted (read-only) Linux AppImage, or a Flatpak's /app sandbox. Using one
// rule everywhere also means a debug log is always at the same, predictable
// path regardless of how SeamlyLayout was launched.
//
// Layout.10: the directory is "logs", not "output", and it sits inside the same
// Seamly/SeamlyLayout tree as qt6_seamlylayout.ini — matching Seamly2D's
// %LOCALAPPDATA%\Seamly\Seamly2D\logs. main() therefore sets the organization
// and application names BEFORE it calls init(); without them AppConfigLocation
// resolves to %LOCALAPPDATA%\SeamlyLayout\.

#include "Logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

bool       Logger::debugEnabled = false;
QFile      Logger::s_file;
QTextStream Logger::s_stream;

// ---------------------------------------------------------------------------
// clearLogDirectory
// ---------------------------------------------------------------------------

void Logger::clearLogDirectory(const QString &logsDirPath)
{
    QDir logsDir(logsDirPath);
    const QFileInfoList existingFiles =
        logsDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    for (const QFileInfo &fileInfo : existingFiles) {
        logsDir.remove(fileInfo.fileName());
    }
} // Logger::clearLogDirectory

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

// @brief Open the log file.
// The logs/ directory is created under the AppConfigLocation root if it does
// not already exist.  The file name encodes the startup time as YYMMDDHHMMSS so
// each run gets its own file.
void Logger::init()
{
    if (!debugEnabled) return; // logging disabled — do not create files

    // The install root is not writable on macOS (a notarized .app bundle), on
    // Windows (C:\Program Files), on a mounted Linux AppImage, or inside a
    // Flatpak sandbox — so every platform uses the same writable
    // AppConfigLocation root, the "Seamly/SeamlyLayout" tree the settings and
    // preferences files already use.
    QString logsDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (logsDir.isEmpty()) {
        logsDir = QCoreApplication::applicationDirPath();
    } // if AppConfigLocation unavailable
    logsDir += QStringLiteral("/logs");

    QDir().mkpath(logsDir);
    clearLogDirectory(logsDir);

    // Build the file name: log_{YYMMDDHHmmss}.txt
    const QString timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyMMddHHmmss"));
    const QString filePath =
        logsDir + QStringLiteral("/log_") + timestamp + QStringLiteral(".txt");

    // Publish the path so Rust log_to_file() can append to the same file.
    qputenv("SEAMLY_LOG_FILE", filePath.toUtf8());

    s_file.setFileName(filePath);

    if (!s_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        debugEnabled = false;
        return;
    } // if !open

    s_stream.setDevice(&s_file);

    // Write a session header so log files from multiple runs are easy to distinguish
    const qint64 unixSec = QDateTime::currentSecsSinceEpoch();
    s_stream << QStringLiteral("[") << QString::number(unixSec)
             << QStringLiteral("] DEBUG: Logger::init(): SeamlyLayout session started\n");
    s_stream << QStringLiteral("[") << QString::number(unixSec)
             << QStringLiteral("] DEBUG: Logger::init(): cleared existing debug files from logs/\n");
    s_stream.flush();
} // Logger::init

// ---------------------------------------------------------------------------
// log
// ---------------------------------------------------------------------------

// @brief Append one debug line to the open log file.
// @param message  Text written after the "[unix_seconds] DEBUG: " prefix.
// Does nothing when debugEnabled is false or when the file is not open.
void Logger::log(const QString &message)
{
    if (!debugEnabled) return;     // logging disabled
    if (!s_file.isOpen()) return;  // file not open (init() not called or failed)

    const qint64 unixSec = QDateTime::currentSecsSinceEpoch();
    s_stream << QStringLiteral("[") << QString::number(unixSec)
             << QStringLiteral("] DEBUG: ") << message << QStringLiteral("\n");
    s_stream.flush(); // flush immediately so lines appear even if the app crashes
} // Logger::log

// ---------------------------------------------------------------------------
// messageHandler
// ---------------------------------------------------------------------------

// @brief Qt message handler — routes all qDebug/qInfo/qWarning/qCritical/qFatal
// and QML console.log/warn/error output to the log file.
// Install via qInstallMessageHandler(Logger::messageHandler) after init().
void Logger::messageHandler(QtMsgType type,
                            const QMessageLogContext &context,
                            const QString &msg)
{
    if (!debugEnabled || !s_file.isOpen()) return;

    const qint64 unixSec = QDateTime::currentSecsSinceEpoch();
    QString level;
    switch (type) {
    case QtDebugMsg:    level = QStringLiteral("DEBUG"); break;
    case QtInfoMsg:     level = QStringLiteral("INFO");  break;
    case QtWarningMsg:  level = QStringLiteral("WARN");  break;
    case QtCriticalMsg: level = QStringLiteral("CRIT");  break;
    case QtFatalMsg:    level = QStringLiteral("FATAL"); break;
    }

    // Include source file:line when available (C++ calls only; QML has no context)
    QString location;
    if (context.file) {
        location = QStringLiteral(" (") + QString::fromUtf8(context.file)
                 + QStringLiteral(":") + QString::number(context.line)
                 + QStringLiteral(")");
    }

    s_stream << QStringLiteral("[") << QString::number(unixSec)
             << QStringLiteral("] ") << level << QStringLiteral(": ")
             << msg << location << QStringLiteral("\n");
    s_stream.flush();

    if (type == QtFatalMsg)
        abort();
} // Logger::messageHandler
