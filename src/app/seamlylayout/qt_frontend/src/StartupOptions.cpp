// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file StartupOptions.cpp
// @brief Implementation of the SeamlyLayout startup contract — a positional
//        <svg-file>, or an SVG document on standard input.
//
// Control flow of parse():
//   1. Reject an empty argument list outright (nothing to parse).
//   2. Run QCommandLineParser::parse() — the non-exiting sibling of process(),
//      chosen so this function can be unit tested and can never terminate the
//      application behind main()'s back.
//   3. Honour --help / --version by handing their text back as
//      Status::ShowInformation; main() displays it and exits 0.
//   4. Reject --svg-stdin together with a positional file: one document per
//      process, and two sources would silently pick a winner.
//   5. With --svg-stdin, read standard input to end and validate it as an SVG
//      document (Seamly2D.5).
//   6. Otherwise require at most one positional argument, and validate it as an
//      existing, readable .svg file, storing its ABSOLUTE path (the app-wide
//      rule: never pass relative paths around, the working directory of a
//      launched process is not the user's).
//
// Every failure produces a sentence the user can act on, because the only thing
// worse than the old behaviour (silently ignoring the argument) is silently
// ignoring it with an extra step.

#include "StartupOptions.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QIODevice>

namespace
{
// @brief Extension SeamlyLayout accepts, compared case-insensitively.
const QLatin1String svgSuffix("svg");

// @brief Option name that switches the input transport to standard input.
// Seamly2D passes it for the Layout Mode handoff; SeamlySuitePaths::
// seamlyLayoutLaunchArguments() is the one place that spells it on that side.
const QLatin1String svgStdinOptionName("svg-stdin");

// @brief Option name carrying the pattern base name of a piped document.
const QLatin1String documentNameOptionName("document-name");

// @brief Smallest thing that can still be an SVG document: the root element.
// Checked so an empty pipe, or a stray non-SVG payload, is reported as such
// rather than as an XML syntax error from deep inside the Rust parser.
const QLatin1String svgRootElement("<svg");

// @brief Build the text shown for --version.
// Falls back to the product name alone when the caller has not set the
// application metadata yet (QCoreApplication::setApplicationVersion()).
// @return e.g. "SeamlyLayout 0.1.0".
QString versionText()
{
    const QString name    = QCoreApplication::applicationName();
    const QString version = QCoreApplication::applicationVersion();

    if (name.isEmpty()) {
        // No metadata set — name the product so the output is never empty.
        return version.isEmpty() ? QStringLiteral("SeamlyLayout")
                                 : QStringLiteral("SeamlyLayout ") + version;
    } // if name.isEmpty()

    return version.isEmpty() ? name : name + QLatin1Char(' ') + version;
} // versionText
} // namespace

// @brief Parse and validate a command line that cannot use standard input.
// @param arguments Full argument list including the program name at index 0.
// @return A fully populated StartupOptions; never throws, never exits.
StartupOptions StartupOptions::parse(const QStringList &arguments)
{
    return parse(arguments, nullptr);
} // StartupOptions::parse

// @brief Parse and validate a SeamlyLayout command line.
// @param arguments Full argument list including the program name at index 0,
//        as returned by QCoreApplication::arguments().
// @param standardInput Device the SVG document is read from when --svg-stdin is
//        set; nullptr when the caller offers no such channel.
// @return A fully populated StartupOptions; never throws, never exits.
StartupOptions StartupOptions::parse(const QStringList &arguments, QIODevice *standardInput)
{
    StartupOptions options;

    if (arguments.isEmpty()) {
        // Not even a program name — nothing to open, and QCommandLineParser
        // would warn about the missing argv[0].
        return options;
    } // if arguments.isEmpty()

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "SeamlyLayout — pattern layout application of the Seamly Application Suite.\n"
        "Opens an SVG file, or the SVG document Seamly2D's Layout Mode sends on "
        "standard input."));

    const QCommandLineOption helpOption    = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    const QCommandLineOption svgStdinOption(
        QStringList() << svgStdinOptionName,
        QStringLiteral("Read the SVG document to open from standard input. "
                       "Used by Seamly2D's Layout Mode handoff."));
    parser.addOption(svgStdinOption);

    const QCommandLineOption documentNameOption(
        QStringList() << documentNameOptionName,
        QStringLiteral("Name of the document read from standard input; used for "
                       "default export file names."),
        QStringLiteral("name"));
    parser.addOption(documentNameOption);

    parser.addPositionalArgument(
        QStringLiteral("svg-file"),
        QStringLiteral("SVG pattern file to open at startup (optional)."),
        QStringLiteral("[svg-file]"));

    // parse(), not process(): process() prints to a console this GUI-subsystem
    // application does not have on Windows, and calls exit() on error.
    if (!parser.parse(arguments)) {
        // Unknown option, missing option value, etc. errorText() is already a
        // complete sentence ("Unknown option 'foo'.").
        options.m_status  = Status::Failed;
        options.m_message = parser.errorText();
        return options;
    } // if !parser.parse

    if (parser.isSet(versionOption)) {
        // --version wins over everything else, matching every other CLI.
        options.m_status  = Status::ShowInformation;
        options.m_message = versionText();
        return options;
    } // if version requested

    if (parser.isSet(helpOption)) {
        options.m_status  = Status::ShowInformation;
        options.m_message = parser.helpText();
        return options;
    } // if help requested

    const QStringList positional = parser.positionalArguments();
    const bool readStandardInput = parser.isSet(svgStdinOption);

    if (readStandardInput && !positional.isEmpty()) {
        // Two sources for one canvas. Refuse rather than pick a winner the
        // caller cannot predict.
        options.m_status  = Status::Failed;
        options.m_message = QStringLiteral(
            "--%1 reads the document from standard input, so no file may be given as well:\n%2")
            .arg(svgStdinOptionName)
            .arg(positional.join(QStringLiteral("\n")));
        return options;
    } // if both transports requested

    if (readStandardInput) {
        return parseStandardInput(parser.value(documentNameOption), standardInput);
    } // if the document arrives on standard input

    if (positional.isEmpty()) {
        // Plain launch — the double-clicked-icon case. Empty canvas, no message.
        return options;
    } // if positional.isEmpty()

    if (positional.size() > 1) {
        // One document per process, deliberately: SeamlyLayout has a single
        // pair of canvases and no tabs, so a second file has nowhere to go.
        options.m_status  = Status::Failed;
        options.m_message = QStringLiteral(
            "SeamlyLayout opens one SVG file at a time, but %1 files were given:\n%2")
            .arg(positional.size())
            .arg(positional.join(QStringLiteral("\n")));
        return options;
    } // if positional.size() > 1

    // ---------------------------------------------------------------------
    // Validate the one positional argument.
    //
    // Each branch names the file, because a launched process gives the user no
    // console output to correlate the message with.
    // ---------------------------------------------------------------------
    const QFileInfo file(positional.constFirst());
    // Resolved against the process working directory, which for the Seamly2D
    // handoff is SeamlyLayout's own install directory — hence absolute only.
    const QString absolutePath = file.absoluteFilePath();

    if (!file.exists()) {
        options.m_status  = Status::Failed;
        options.m_message = QStringLiteral("The file does not exist:\n%1").arg(absolutePath);
        return options;
    } // if !file.exists()

    if (!file.isFile()) {
        options.m_status  = Status::Failed;
        options.m_message = QStringLiteral("This is a folder, not an SVG file:\n%1").arg(absolutePath);
        return options;
    } // if !file.isFile()

    if (!file.isReadable()) {
        options.m_status  = Status::Failed;
        options.m_message = QStringLiteral(
            "The file cannot be read — check its permissions:\n%1").arg(absolutePath);
        return options;
    } // if !file.isReadable()

    if (file.suffix().compare(svgSuffix, Qt::CaseInsensitive) != 0) {
        options.m_status  = Status::Failed;
        options.m_message = QStringLiteral(
            "SeamlyLayout opens SVG files; this one is not an SVG:\n%1").arg(absolutePath);
        return options;
    } // if suffix is not svg

    options.m_status   = Status::OpenFile;
    options.m_filePath = absolutePath;
    return options;
} // StartupOptions::parse

// @brief Read and validate the SVG document offered on standard input.
//
// Split out of parse() so the standard-input transport (Seamly2D.5) has one
// place that decides what a usable document is, and so a test can drive it with
// a QBuffer instead of the real stdin of the test runner.
//
// @param documentName Value of --document-name; empty when it was not given.
// @param standardInput Device to read to end; nullptr when the caller offers
//        none, which is itself a failure because --svg-stdin was requested.
// @return OpenDocument with svgDocument() set, or Failed with a message.
StartupOptions StartupOptions::parseStandardInput(const QString &documentName,
                                                  QIODevice *standardInput)
{
    StartupOptions options;

    if (standardInput == nullptr || !standardInput->isReadable()) {
        options.m_status  = Status::Failed;
        options.m_message = QStringLiteral(
            "--%1 was given, but standard input is not readable.").arg(svgStdinOptionName);
        return options;
    } // if no readable channel

    // Read to end: Seamly2D writes the whole document, then closes the channel.
    const QString document = QString::fromUtf8(standardInput->readAll());

    if (document.trimmed().isEmpty()) {
        options.m_status  = Status::Failed;
        options.m_message = QStringLiteral(
            "--%1 was given, but standard input was empty.").arg(svgStdinOptionName);
        return options;
    } // if nothing arrived

    if (!document.contains(svgRootElement, Qt::CaseInsensitive)) {
        options.m_status  = Status::Failed;
        options.m_message = QStringLiteral(
            "The document on standard input is not an SVG — it has no <svg> element.");
        return options;
    } // if not an SVG document

    options.m_status       = Status::OpenDocument;
    options.m_svgDocument  = document;
    options.m_documentName = documentName;
    return options;
} // StartupOptions::parseStandardInput
