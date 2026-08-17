// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file StartupOptions.cpp
// @brief Implementation of the SeamlyLayout command-line contract.
//
// Control flow of parse():
//   1. Reject an empty argument list outright (nothing to parse).
//   2. Run QCommandLineParser::parse() — the non-exiting sibling of process(),
//      chosen so this function can be unit tested and can never terminate the
//      application behind main()'s back.
//   3. Honour --help / --version by handing their text back as
//      Status::ShowInformation; main() displays it and exits 0.
//   4. Require at most one positional argument.
//   5. Validate that argument as an existing, readable .svg file and store its
//      ABSOLUTE path (the app-wide rule: never pass relative paths around, the
//      working directory of a detached launch is not the user's).
//
// Every failure produces a sentence the user can act on, because the only thing
// worse than the old behaviour (silently ignoring the argument) is silently
// ignoring it with an extra step.

#include "StartupOptions.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>

namespace
{
// @brief Extension SeamlyLayout accepts, compared case-insensitively.
// Seamly2D hands over "<pattern>.pieces.svg", whose QFileInfo::suffix() is "svg".
const QLatin1String svgSuffix("svg");

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

// @brief Parse and validate a SeamlyLayout command line.
// @param arguments Full argument list including the program name at index 0,
//        as returned by QCoreApplication::arguments().
// @return A fully populated StartupOptions; never throws, never exits.
StartupOptions StartupOptions::parse(const QStringList &arguments)
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
        "Opens the tagged pieces SVG that Seamly2D's Layout Mode writes beside "
        "the pattern file."));

    const QCommandLineOption helpOption    = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

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
    // Each branch names the file, because a detached launch gives the user no
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
