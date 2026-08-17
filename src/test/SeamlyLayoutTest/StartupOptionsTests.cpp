// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file StartupOptionsTests.cpp
// @brief Qt tests for StartupOptions — SeamlyLayout's command-line contract.
//
// This suite locks the SeamlyLayout half of the Seamly2D Layout Mode handoff
// (Task 49). Seamly2D launches
//
//     SeamlyLayout <absolute path to <pattern>.pieces.svg>
//
// detached, and until Task 49 that argument was read by nobody: the daughter
// app opened an empty canvas. The seamly2d half of the same contract — that
// the launch really is "one positional argument, the .pieces.svg path" — is
// locked by TST_SeamlySuitePaths in the Seamly2DTest suite.
//
// Covers:
//   • No argument / empty argument list → start with an empty canvas
//   • A real .svg (including the ".pieces.svg" double extension and an
//     upper-case ".SVG") → Status::OpenFile with an ABSOLUTE path
//   • A relative argument resolved against the working directory
//   • Missing file, directory, unreadable file, non-SVG suffix → Status::Failed
//     with a message naming the file
//   • More than one positional argument → Status::Failed
//   • Unknown option → Status::Failed carrying the parser's own text
//   • --help / -h / --version / -v → Status::ShowInformation
//
// StartupOptions needs no QObject, no window and no event loop, so this suite
// runs guiless — QTEST_GUILESS_MAIN, not QTEST_MAIN.

#include "StartupOptions.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest/QtTest>

class StartupOptionsTests : public QObject
{
    Q_OBJECT

private slots:
    // Nothing asked of us
    void noArgument_statusIsNoFile();
    void emptyArgumentList_statusIsNoFile();

    // The handoff itself
    void existingSvg_statusIsOpenFile();
    void existingSvg_filePathIsAbsolute();
    void piecesSvgDoubleExtension_isAccepted();
    void upperCaseSvgSuffix_isAccepted();
    void relativePath_isResolvedToAbsolute();

    // Failure modes, each with a message the user can act on
    void missingFile_statusIsFailed();
    void missingFile_messageNamesTheFile();
    void directoryArgument_statusIsFailed();
    void unreadableFile_statusIsFailed();
    void nonSvgSuffix_statusIsFailed();
    void twoPositionalArguments_statusIsFailed();
    void unknownOption_statusIsFailed();

    // --help / --version
    void helpOption_statusIsShowInformation();
    void shortHelpOption_statusIsShowInformation();
    void versionOption_statusIsShowInformation();
    void versionOption_messageCarriesTheVersion();

private:
    // @brief Create an empty file inside @p directory.
    // @param directory Existing directory to create the file in.
    // @param fileName  Name of the file to create.
    // @return Absolute path of the created file.
    static QString makeFile(const QTemporaryDir &directory, const QString &fileName);
}; // class StartupOptionsTests

//----------------------------------------------------------------------------
QString StartupOptionsTests::makeFile(const QTemporaryDir &directory, const QString &fileName)
{
    const QString path = directory.filePath(fileName);
    QFile file(path);
    // Verified rather than assumed: a silent failure here would make every
    // assertion below test the "missing file" path by accident.
    Q_ASSERT(file.open(QIODevice::WriteOnly));
    file.write("<svg xmlns=\"http://www.w3.org/2000/svg\"/>");
    file.close();
    return QFileInfo(path).absoluteFilePath();
} // StartupOptionsTests::makeFile

// ---------------------------------------------------------------------------
// Nothing asked of us
// ---------------------------------------------------------------------------

// @brief A bare launch (icon double-click) leaves the app with an empty canvas.
void StartupOptionsTests::noArgument_statusIsNoFile()
{
    const StartupOptions options = StartupOptions::parse(QStringList{QStringLiteral("SeamlyLayout")});
    QCOMPARE(options.status(), StartupOptions::Status::NoFile);
    QVERIFY(!options.hasFile());
    QVERIFY(!options.hasError());
    QVERIFY(options.filePath().isEmpty());
    QVERIFY(options.message().isEmpty());
} // noArgument_statusIsNoFile()

// @brief An empty list (no program name at all) must not reach the parser.
void StartupOptionsTests::emptyArgumentList_statusIsNoFile()
{
    const StartupOptions options = StartupOptions::parse(QStringList{});
    QCOMPARE(options.status(), StartupOptions::Status::NoFile);
    QVERIFY(options.message().isEmpty());
} // emptyArgumentList_statusIsNoFile()

// ---------------------------------------------------------------------------
// The handoff itself
// ---------------------------------------------------------------------------

// @brief An existing .svg file is accepted for opening.
void StartupOptionsTests::existingSvg_statusIsOpenFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString svgPath = makeFile(directory, QStringLiteral("pattern.svg"));

    const StartupOptions options =
        StartupOptions::parse(QStringList{QStringLiteral("SeamlyLayout"), svgPath});

    QCOMPARE(options.status(), StartupOptions::Status::OpenFile);
    QVERIFY(options.hasFile());
    QVERIFY(!options.hasError());
    QVERIFY(options.message().isEmpty());
} // existingSvg_statusIsOpenFile()

// @brief The stored path is absolute — the app-wide rule, and required because
// the detached launch runs with SeamlyLayout's own working directory.
void StartupOptionsTests::existingSvg_filePathIsAbsolute()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString svgPath = makeFile(directory, QStringLiteral("pattern.svg"));

    const StartupOptions options =
        StartupOptions::parse(QStringList{QStringLiteral("SeamlyLayout"), svgPath});

    QVERIFY(QFileInfo(options.filePath()).isAbsolute());
    QCOMPARE(options.filePath(), svgPath);
} // existingSvg_filePathIsAbsolute()

// @brief "<pattern>.pieces.svg" — the exact file name Seamly2D writes — is accepted.
void StartupOptionsTests::piecesSvgDoubleExtension_isAccepted()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString svgPath = makeFile(directory, QStringLiteral("richmond-shirt.pieces.svg"));

    const StartupOptions options =
        StartupOptions::parse(QStringList{QStringLiteral("SeamlyLayout"), svgPath});

    QCOMPARE(options.status(), StartupOptions::Status::OpenFile);
    QCOMPARE(options.filePath(), svgPath);
} // piecesSvgDoubleExtension_isAccepted()

// @brief The suffix check is case-insensitive — ".SVG" is still an SVG.
void StartupOptionsTests::upperCaseSvgSuffix_isAccepted()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString svgPath = makeFile(directory, QStringLiteral("PATTERN.SVG"));

    const StartupOptions options =
        StartupOptions::parse(QStringList{QStringLiteral("SeamlyLayout"), svgPath});

    QCOMPARE(options.status(), StartupOptions::Status::OpenFile);
} // upperCaseSvgSuffix_isAccepted()

// @brief A relative argument is resolved against the process working directory.
void StartupOptionsTests::relativePath_isResolvedToAbsolute()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString svgPath = makeFile(directory, QStringLiteral("relative.svg"));

    // Restored before leaving the test so the following tests are unaffected.
    const QString previousWorkingDirectory = QDir::currentPath();
    QVERIFY(QDir::setCurrent(directory.path()));

    const StartupOptions options = StartupOptions::parse(
        QStringList{QStringLiteral("SeamlyLayout"), QStringLiteral("relative.svg")});

    QVERIFY(QDir::setCurrent(previousWorkingDirectory));

    QCOMPARE(options.status(), StartupOptions::Status::OpenFile);
    QVERIFY(QFileInfo(options.filePath()).isAbsolute());
    // Compare the canonical forms: a temporary directory may sit under a
    // symlinked path (/tmp, /var on macOS), which QFileInfo does not resolve.
    QCOMPARE(QFileInfo(options.filePath()).canonicalFilePath(),
             QFileInfo(svgPath).canonicalFilePath());
} // relativePath_isResolvedToAbsolute()

// ---------------------------------------------------------------------------
// Failure modes
// ---------------------------------------------------------------------------

// @brief A path that does not exist fails instead of opening an empty canvas.
void StartupOptionsTests::missingFile_statusIsFailed()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString missingPath = directory.filePath(QStringLiteral("no-such-file.svg"));

    const StartupOptions options =
        StartupOptions::parse(QStringList{QStringLiteral("SeamlyLayout"), missingPath});

    QCOMPARE(options.status(), StartupOptions::Status::Failed);
    QVERIFY(options.hasError());
    QVERIFY(!options.hasFile());
    QVERIFY(options.filePath().isEmpty());
} // missingFile_statusIsFailed()

// @brief The message names the file — a detached launch has no console output
// for the user to correlate the message with.
void StartupOptionsTests::missingFile_messageNamesTheFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString missingPath = directory.filePath(QStringLiteral("no-such-file.svg"));

    const StartupOptions options =
        StartupOptions::parse(QStringList{QStringLiteral("SeamlyLayout"), missingPath});

    QVERIFY(options.message().contains(QStringLiteral("no-such-file.svg")));
} // missingFile_messageNamesTheFile()

// @brief A directory argument is rejected, not treated as an empty file.
void StartupOptionsTests::directoryArgument_statusIsFailed()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    // Named like an SVG so the suffix check cannot be what rejects it.
    const QString subDirectoryPath = directory.filePath(QStringLiteral("folder.svg"));
    QVERIFY(QDir().mkpath(subDirectoryPath));

    const StartupOptions options =
        StartupOptions::parse(QStringList{QStringLiteral("SeamlyLayout"), subDirectoryPath});

    QCOMPARE(options.status(), StartupOptions::Status::Failed);
    QVERIFY(options.message().contains(QStringLiteral("folder")));
} // directoryArgument_statusIsFailed()

// @brief A file the process cannot read is reported rather than silently failing later.
void StartupOptionsTests::unreadableFile_statusIsFailed()
{
#ifdef Q_OS_WIN
    QSKIP("NTFS does not deny the owner read access through QFile::setPermissions");
#else
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString svgPath = makeFile(directory, QStringLiteral("locked.svg"));
    QVERIFY(QFile::setPermissions(svgPath, QFileDevice::WriteOwner));

    const StartupOptions options =
        StartupOptions::parse(QStringList{QStringLiteral("SeamlyLayout"), svgPath});

    // Restore before asserting so QTemporaryDir can always clean up.
    QFile::setPermissions(svgPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    // Running as root defeats permission bits entirely; skip rather than fail.
    if (options.status() == StartupOptions::Status::OpenFile) {
        QSKIP("Process can read a file with no read permission (running as root?)");
    } // if the permission bit had no effect

    QCOMPARE(options.status(), StartupOptions::Status::Failed);
    QVERIFY(options.message().contains(QStringLiteral("locked.svg")));
#endif
} // unreadableFile_statusIsFailed()

// @brief SeamlyLayout opens SVG only; any other suffix is rejected up front.
void StartupOptionsTests::nonSvgSuffix_statusIsFailed()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString patternPath = makeFile(directory, QStringLiteral("pattern.sm2d"));

    const StartupOptions options =
        StartupOptions::parse(QStringList{QStringLiteral("SeamlyLayout"), patternPath});

    QCOMPARE(options.status(), StartupOptions::Status::Failed);
    QVERIFY(options.message().contains(QStringLiteral("pattern.sm2d")));
} // nonSvgSuffix_statusIsFailed()

// @brief One document per process: a second file has nowhere to go.
void StartupOptionsTests::twoPositionalArguments_statusIsFailed()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString firstPath  = makeFile(directory, QStringLiteral("first.svg"));
    const QString secondPath = makeFile(directory, QStringLiteral("second.svg"));

    const StartupOptions options = StartupOptions::parse(
        QStringList{QStringLiteral("SeamlyLayout"), firstPath, secondPath});

    QCOMPARE(options.status(), StartupOptions::Status::Failed);
    QVERIFY(options.filePath().isEmpty());
    QVERIFY(options.message().contains(QStringLiteral("second.svg")));
} // twoPositionalArguments_statusIsFailed()

// @brief An unrecognised option is reported with the parser's own wording.
void StartupOptionsTests::unknownOption_statusIsFailed()
{
    const StartupOptions options = StartupOptions::parse(
        QStringList{QStringLiteral("SeamlyLayout"), QStringLiteral("--not-an-option")});

    QCOMPARE(options.status(), StartupOptions::Status::Failed);
    QVERIFY(!options.message().isEmpty());
    QVERIFY(options.message().contains(QStringLiteral("not-an-option")));
} // unknownOption_statusIsFailed()

// ---------------------------------------------------------------------------
// --help / --version
// ---------------------------------------------------------------------------

// @brief --help produces usage text for main() to show, and opens nothing.
void StartupOptionsTests::helpOption_statusIsShowInformation()
{
    const StartupOptions options = StartupOptions::parse(
        QStringList{QStringLiteral("SeamlyLayout"), QStringLiteral("--help")});

    QCOMPARE(options.status(), StartupOptions::Status::ShowInformation);
    QVERIFY(!options.message().isEmpty());
    // The documented positional argument must appear in the usage text.
    QVERIFY(options.message().contains(QStringLiteral("svg-file")));
    QVERIFY(options.filePath().isEmpty());
} // helpOption_statusIsShowInformation()

// @brief The short form behaves identically.
void StartupOptionsTests::shortHelpOption_statusIsShowInformation()
{
    const StartupOptions options = StartupOptions::parse(
        QStringList{QStringLiteral("SeamlyLayout"), QStringLiteral("-h")});

    QCOMPARE(options.status(), StartupOptions::Status::ShowInformation);
} // shortHelpOption_statusIsShowInformation()

// @brief --version reports and exits; it never opens a file.
void StartupOptionsTests::versionOption_statusIsShowInformation()
{
    const StartupOptions options = StartupOptions::parse(
        QStringList{QStringLiteral("SeamlyLayout"), QStringLiteral("--version")});

    QCOMPARE(options.status(), StartupOptions::Status::ShowInformation);
    QVERIFY(options.filePath().isEmpty());
} // versionOption_statusIsShowInformation()

// @brief The version text carries the metadata the application set.
void StartupOptionsTests::versionOption_messageCarriesTheVersion()
{
    // Set on the test's own QCoreApplication, exactly as main.cpp does before
    // parsing, so the value reaching the user is the one under test.
    const QString previousName    = QCoreApplication::applicationName();
    const QString previousVersion = QCoreApplication::applicationVersion();
    QCoreApplication::setApplicationName(QStringLiteral("SeamlyLayout"));
    QCoreApplication::setApplicationVersion(QStringLiteral("9.9.9"));

    const StartupOptions options = StartupOptions::parse(
        QStringList{QStringLiteral("SeamlyLayout"), QStringLiteral("-v")});

    QCoreApplication::setApplicationName(previousName);
    QCoreApplication::setApplicationVersion(previousVersion);

    QCOMPARE(options.status(), StartupOptions::Status::ShowInformation);
    QVERIFY(options.message().contains(QStringLiteral("SeamlyLayout")));
    QVERIFY(options.message().contains(QStringLiteral("9.9.9")));
} // versionOption_messageCarriesTheVersion()

QTEST_GUILESS_MAIN(StartupOptionsTests)
#include "StartupOptionsTests.moc"
