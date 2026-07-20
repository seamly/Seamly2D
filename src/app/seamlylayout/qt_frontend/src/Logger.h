// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file Logger.h
// @brief Singleton debug logger — writes timestamped lines to a rolling log file.
//
// Format: [unix_seconds] DEBUG: ClassName::methodName(): message
// Log file: {appDir}/output/log_{YYMMDDHHMM}.txt
//
// Usage:
//   Logger::debugEnabled = true;   // enable at startup (default: false)
//   Logger::init();                // open log file (call once from main())
//   Logger::log("ClassName::method(): message");
//
// When debugEnabled is false all Logger::log() calls are no-ops.

#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>

// @brief Static-only debug logger.
//
// Thread safety: Logger is designed for Qt-main-thread logging only.
// For background-thread messages, route via Qt::QueuedConnection signal.
class Logger
{
public:

    // @brief When true, log() writes to the log file.  When false, log() is a no-op.
    // Set to true from main() before calling init() to enable debug logging.
    static bool debugEnabled;

    // @brief Open the log file.  Must be called once from main() after QGuiApplication
    // is constructed (so applicationDirPath() is available).
    // Creates the output/ directory if it does not exist.
    // Sets the SEAMLY_LOG_FILE environment variable so Rust can append to the
    // same file via log_to_file().
    static void init();

    // @brief Write one line to the log file if debugEnabled is true.
    // @param message  Text appended after "[unix_seconds] DEBUG: ".
    static void log(const QString &message);

    // @brief Qt message handler — install with qInstallMessageHandler().
    // Routes qDebug, qInfo, qWarning, qCritical, QML console.log, and
    // qFatal messages to the log file with level prefix.
    static void messageHandler(QtMsgType type,
                               const QMessageLogContext &context,
                               const QString &msg);

private:
    // Non-instantiable — all members are static.
    Logger() = delete;

    // @brief Remove stale debug files from the output/ directory at startup.
    static void clearOutputDirectory(const QString &outputDirPath);

    // @brief Open log file handle.  Stays open for the process lifetime.
    static QFile   s_file;

    // @brief Text stream writing to s_file.
    static QTextStream s_stream;

}; // class Logger
