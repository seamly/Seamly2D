// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file LoggerTests.cpp
// @brief Qt tests for Logger — where the session log file is written.
//
// Layout.10. SeamlyLayout used to write its session log to
// %LOCALAPPDATA%\SeamlyLayout\output\log_<timestamp>.txt: outside the shared
// "Seamly" organization folder that holds qt6_seamlylayout.ini, and in a
// directory named "output" rather than "logs". Two separate causes:
//
//   • Logger::init() appended "/output" to the AppConfigLocation root;
//   • main() called Logger::init() BEFORE it set the organization and
//     application names, and AppConfigLocation is built from those two names,
//     so the root itself was wrong.
//
// The suite locks the resulting path, which is
// <AppConfigLocation>/logs/log_YYMMDDHHMMSS.txt — for an installed Windows
// build, %LOCALAPPDATA%\Seamly\SeamlyLayout\logs.
//
// QStandardPaths test mode redirects AppConfigLocation into a throwaway tree,
// so the suite never touches the real user configuration.
//
// Logger needs no QObject, no window and no event loop, so this suite runs
// guiless — QTEST_GUILESS_MAIN, not QTEST_MAIN.

#include "Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTest>

class LoggerTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void logDirectory_isLogsUnderTheAppConfigRoot();
    void logDirectory_carriesTheOrganizationAndApplication();
    void logDirectory_isNotTheLegacyOutputDirectory();
    void logFileName_isTimestamped();
    void init_removesStaleLogFiles();
    void cleanupTestCase();

private:
    // Path of the log file opened by initTestCase(), read back from the
    // SEAMLY_LOG_FILE variable Logger::init() publishes for the Rust side.
    QString m_logFilePath;

    // Stale file planted before init() so the startup clean-up can be observed.
    QString m_staleFilePath;
};

// @brief Open one log file under a throwaway AppConfigLocation root.
// The metadata is set BEFORE init(), which is the order main() must use.
void LoggerTests::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    QCoreApplication::setOrganizationName(QStringLiteral("Seamly"));
    QCoreApplication::setApplicationName(QStringLiteral("SeamlyLayout"));

    const QString appConfigRoot =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QVERIFY2(!appConfigRoot.isEmpty(), "AppConfigLocation is unavailable in test mode");

    // Plant a stale file so init_removesStaleLogFiles() has something to find.
    const QString logsDir = appConfigRoot + QStringLiteral("/logs");
    QVERIFY(QDir().mkpath(logsDir));
    m_staleFilePath = logsDir + QStringLiteral("/log_240101000000.txt");
    QFile staleFile(m_staleFilePath);
    QVERIFY(staleFile.open(QIODevice::WriteOnly | QIODevice::Text));
    staleFile.write("stale\n");
    staleFile.close();

    Logger::debugEnabled = true;
    Logger::init();
    QVERIFY2(Logger::debugEnabled, "Logger::init() failed to open the log file");

    m_logFilePath = QString::fromUtf8(qgetenv("SEAMLY_LOG_FILE"));
    QVERIFY2(!m_logFilePath.isEmpty(), "Logger::init() published no SEAMLY_LOG_FILE");
} // initTestCase()

void LoggerTests::logDirectory_isLogsUnderTheAppConfigRoot()
{
    const QString expected =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
        + QStringLiteral("/logs");

    QCOMPARE(QFileInfo(m_logFilePath).absolutePath(), QDir(expected).absolutePath());
    QVERIFY2(QFile::exists(m_logFilePath), "the log file was not created");
} // logDirectory_isLogsUnderTheAppConfigRoot()

// The organization folder is what the defect lost: without it the path was
// .../SeamlyLayout/logs, not .../Seamly/SeamlyLayout/logs.
void LoggerTests::logDirectory_carriesTheOrganizationAndApplication()
{
    const QString path = QDir::fromNativeSeparators(m_logFilePath);
    QVERIFY2(path.contains(QStringLiteral("/Seamly/SeamlyLayout/logs/")),
             qPrintable(QStringLiteral("log path was '%1'").arg(path)));
} // logDirectory_carriesTheOrganizationAndApplication()

void LoggerTests::logDirectory_isNotTheLegacyOutputDirectory()
{
    const QString path = QDir::fromNativeSeparators(m_logFilePath);
    QVERIFY2(!path.contains(QStringLiteral("/output/")),
             qPrintable(QStringLiteral("log path was '%1'").arg(path)));
} // logDirectory_isNotTheLegacyOutputDirectory()

void LoggerTests::logFileName_isTimestamped()
{
    const QString fileName = QFileInfo(m_logFilePath).fileName();
    const QRegularExpression pattern(QStringLiteral("^log_\\d{12}\\.txt$"));
    QVERIFY2(pattern.match(fileName).hasMatch(),
             qPrintable(QStringLiteral("file name was '%1'").arg(fileName)));
} // logFileName_isTimestamped()

// Each run starts a clean directory, so an old session's file cannot be mistaken
// for the current one.
void LoggerTests::init_removesStaleLogFiles()
{
    QVERIFY2(!QFile::exists(m_staleFilePath), "the stale log file survived Logger::init()");
} // init_removesStaleLogFiles()

void LoggerTests::cleanupTestCase()
{
    Logger::debugEnabled = false;
    QStandardPaths::setTestModeEnabled(false);
} // cleanupTestCase()

QTEST_GUILESS_MAIN(LoggerTests)
#include "LoggerTests.moc"
