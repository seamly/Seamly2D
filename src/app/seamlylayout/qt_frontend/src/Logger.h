// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file Logger.h
// @brief Singleton debug logger — writes timestamped lines to a rolling log file.
//
// Format: [unix_seconds] DEBUG: ClassName::methodName(): message
// Log file: {appConfigRoot}/logs/log_{YYMMDDHHMMSS}.txt
// On Windows that is %LOCALAPPDATA%\Seamly\SeamlyLayout\logs (Layout.10).
//
// Usage:
//   Logger::debugEnabled = true;   // enable at startup (default: false)
//   Logger::init();                // open log file (call once from main())
//   Logger::log("ClassName::method(): message");
//
// When debugEnabled is false all Logger::log() calls are no-ops.
//
// Logger is the only writer of the log file. Rust debug lines reach it through
// seamly_logger_write_utf8(), which main() registers with the Rust bridge.

#pragma once

#include <QString>
#include <QFile>
#include <QMutex>
#include <QTextStream>

#include <cstddef>

// @brief Static-only debug logger.
//
// Thread safety: log(), messageHandler() and seamly_logger_write_utf8() may be
// called from any thread. s_mutex serializes every write, so lines never interleave.
class Logger
{
public:

    // @brief When true, log() writes to the log file.  When false, log() is a no-op.
    // Set to true from main() before calling init() to enable debug logging.
    static bool debugEnabled;

    // @brief Open the log file.  Must be called once from main() after QGuiApplication
    // is constructed AND after the organization and application names are set,
    // because AppConfigLocation is derived from them (Layout.10).
    // Creates the logs/ directory if it does not exist.
    static void init();

    // @brief Absolute path of the open log file, or an empty string before init()
    // or when logging is disabled.
    static QString filePath();

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

    // @brief Remove stale debug files from the logs/ directory at startup.
    static void clearLogDirectory(const QString &logsDirPath);

    // @brief Open log file handle.  Stays open for the process lifetime.
    static QFile   s_file;

    // @brief Text stream writing to s_file.
    static QTextStream s_stream;

    // @brief Guards s_file and s_stream against concurrent writers.
    static QMutex s_mutex;

}; // class Logger

// @brief C ABI entry point for the Rust bridge: writes one message through Logger::log().
// @param text    UTF-8 bytes, not NUL-terminated. Copied before the function returns.
// @param length  Number of bytes in text.
extern "C" void seamly_logger_write_utf8(const char *text, std::size_t length);
