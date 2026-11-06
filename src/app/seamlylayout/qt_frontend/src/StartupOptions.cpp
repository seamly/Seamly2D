// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file StartupOptions.cpp
// @brief Implementation of the SeamlyLayout startup and headless export contract.
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
#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QStringList>

#include <cmath>
#include <limits>

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

// @brief Output formats supported by the headless layout pipeline.
const QStringList exportFormats{
    QStringLiteral("svg"), QStringLiteral("pdf"), QStringLiteral("pdf-tiled"),
    QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
    QStringLiteral("dxf"), QStringLiteral("hpgl"), QStringLiteral("ps"),
    QStringLiteral("eps"), QStringLiteral("gcode"), QStringLiteral("3mf")
};

// @brief SVG text modes accepted by the layout exporter.
const QStringList svgTextModes{
    QStringLiteral("designerFont"), QStringLiteral("singleLineFont"),
    QStringLiteral("hersheyStrokes"), QStringLiteral("asSupplied")
};

// @brief Detect export options even when another argument makes parsing fail.
bool hasHeadlessExportArgument(const QStringList &arguments)
{
    const QStringList optionNames{
        QStringLiteral("--export"), QStringLiteral("--format"),
        QStringLiteral("--output"), QStringLiteral("--destination"),
        QStringLiteral("--settings"), QStringLiteral("--layout-settings"),
        QStringLiteral("--export-options"), QStringLiteral("--svg-text-mode"),
        QStringLiteral("--text-mode"), QStringLiteral("--scale")
    };

    for (const QString &argument : arguments) {
        for (const QString &optionName : optionNames) {
            if (argument == optionName || argument.startsWith(optionName + QLatin1Char('='))) {
                return true;
            } // if export option is present
        } // for each export option
    } // for each argument

    return false;
} // hasHeadlessExportArgument

// @brief Build the text shown for --version.
// Falls back to the product name alone when the caller has not set the
// application metadata yet (QCoreApplication::setApplicationVersion()).
// @return e.g. "SeamlyLayout 26.9.2609".
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

    const QCommandLineOption exportOption(
        QStringList{QStringLiteral("export"), QStringLiteral("format")},
        QStringLiteral("Lay out the input SVG and export it without showing the UI."),
        QStringLiteral("format"));
    parser.addOption(exportOption);

    const QCommandLineOption outputOption(
        QStringList{QStringLiteral("output"), QStringLiteral("destination")},
        QStringLiteral("Output file path required by --export."),
        QStringLiteral("path"));
    parser.addOption(outputOption);

    const QCommandLineOption settingsOption(
        QStringList{QStringLiteral("settings"), QStringLiteral("layout-settings")},
        QStringLiteral("Layout settings JSON file."),
        QStringLiteral("path"));
    parser.addOption(settingsOption);

    const QCommandLineOption exportOptionsOption(
        QStringList{QStringLiteral("export-options")},
        QStringLiteral("Format-specific export options JSON file."),
        QStringLiteral("path"));
    parser.addOption(exportOptionsOption);

    const QCommandLineOption svgTextModeOption(
        QStringList{QStringLiteral("svg-text-mode"), QStringLiteral("text-mode")},
        QStringLiteral("SVG label mode: designerFont, singleLineFont, hersheyStrokes, or asSupplied."),
        QStringLiteral("mode"));
    parser.addOption(svgTextModeOption);

    const QCommandLineOption scaleOption(
        QStringList{QStringLiteral("scale")},
        QStringLiteral("PNG scale factor (default: 1.0)."),
        QStringLiteral("factor"));
    parser.addOption(scaleOption);

    parser.addPositionalArgument(
        QStringLiteral("svg-file"),
        QStringLiteral("SVG pattern file to open at startup (optional)."),
        QStringLiteral("[svg-file]"));

    // parse(), not process(): process() prints to a console this GUI-subsystem
    // application does not have on Windows, and calls exit() on error.
    if (!parser.parse(arguments)) {
        // Unknown option, missing option value, etc. errorText() is already a
        // complete sentence ("Unknown option 'foo'.").
        options.m_headlessRequest = hasHeadlessExportArgument(arguments);
        options.m_status  = Status::Failed;
        options.m_message = parser.errorText();
        return options;
    } // if !parser.parse

    options.m_headlessRequest = parser.isSet(exportOption) || parser.isSet(outputOption) ||
        parser.isSet(settingsOption) || parser.isSet(exportOptionsOption) ||
        parser.isSet(svgTextModeOption) || parser.isSet(scaleOption);

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
        const bool headlessRequest = options.m_headlessRequest;
        options = parseStandardInput(parser.value(documentNameOption), standardInput);
        options.m_headlessRequest = headlessRequest;
        if (options.hasError()) {
            return options;
        } // if stdin failed validation
        return parseExportOptions(options, parser, positional, true);
    } // if the document arrives on standard input

    if (positional.isEmpty()) {
        // Plain launch — the double-clicked-icon case. Empty canvas, no message.
        return parseExportOptions(options, parser, positional, false);
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
    return parseExportOptions(options, parser, positional, false);
} // StartupOptions::parse

// @brief Validate and store a headless export request.
//
// @param options Previously validated input transport.
// @param parser Parsed command-line options.
// @param positional Positional SVG file arguments.
// @param hasDocument True when the SVG arrived on standard input.
// @return The input options plus validated export arguments, or Failed.
StartupOptions StartupOptions::parseExportOptions(StartupOptions options,
                                                  const QCommandLineParser &parser,
                                                  const QStringList &positional,
                                                  bool hasDocument)
{
    const QCommandLineOption exportOption(
        QStringList{QStringLiteral("export"), QStringLiteral("format")});
    const QCommandLineOption outputOption(
        QStringList{QStringLiteral("output"), QStringLiteral("destination")});
    const QCommandLineOption settingsOption(
        QStringList{QStringLiteral("settings"), QStringLiteral("layout-settings")});
    const QCommandLineOption exportOptionsOption(
        QStringList{QStringLiteral("export-options")});
    const QCommandLineOption svgTextModeOption(
        QStringList{QStringLiteral("svg-text-mode"), QStringLiteral("text-mode")});
    const QCommandLineOption scaleOption(QStringList{QStringLiteral("scale")});

    const bool hasExportArguments =
        parser.isSet(exportOption) || parser.isSet(outputOption) ||
        parser.isSet(settingsOption) || parser.isSet(exportOptionsOption) ||
        parser.isSet(svgTextModeOption) || parser.isSet(scaleOption);
    if (!hasExportArguments) {
        return options;
    } // if no export arguments

    const auto fail = [&options](const QString &message) {
        options.m_status = Status::Failed;
        options.m_message = message;
        options.m_headlessExport = false;
        return options;
    };

    if (!parser.isSet(exportOption)) {
        return fail(QStringLiteral("Export options require --export <format>."));
    } // if export format missing

    if ((hasDocument && !positional.isEmpty()) || (!hasDocument && positional.size() != 1)) {
        return fail(QStringLiteral("--export requires exactly one input SVG file or --svg-stdin."));
    } // if input count is invalid

    if (!parser.isSet(outputOption) || parser.value(outputOption).trimmed().isEmpty()) {
        return fail(QStringLiteral("--export requires --output <path>."));
    } // if output path is missing

    const QString format = parser.value(exportOption).trimmed().toLower();
    if (!exportFormats.contains(format)) {
        return fail(QStringLiteral("Unknown export format '%1'. Supported formats: %2.")
                        .arg(parser.value(exportOption), exportFormats.join(QStringLiteral(", "))));
    } // if format is unsupported

    const QString textMode = parser.isSet(svgTextModeOption)
        ? parser.value(svgTextModeOption).trimmed()
        : QStringLiteral("designerFont");
    const QString canonicalTextMode = [&textMode]() {
        for (const QString &mode : svgTextModes) {
            if (mode.compare(textMode, Qt::CaseInsensitive) == 0) {
                return mode;
            } // if mode matches
        } // for each text mode
        return QString();
    }();
    if (canonicalTextMode.isEmpty()) {
        return fail(QStringLiteral("Unknown SVG text mode '%1'. Supported modes: %2.")
                        .arg(textMode, svgTextModes.join(QStringLiteral(", "))));
    } // if text mode is unsupported

    if (parser.isSet(svgTextModeOption) && format != QStringLiteral("svg")) {
        return fail(QStringLiteral("--svg-text-mode can only be used with --export svg."));
    } // if text mode is used with another format

    bool scaleOk = true;
    const double scale = parser.isSet(scaleOption)
        ? parser.value(scaleOption).toDouble(&scaleOk)
        : 1.0;
    if (!scaleOk || !std::isfinite(scale) || scale <= 0.0 ||
        scale > static_cast<double>((std::numeric_limits<float>::max)())) {
        return fail(QStringLiteral("--scale must be a positive number."));
    } // if scale is invalid

    const QFileInfo outputInfo(parser.value(outputOption));
    const QString outputPath = outputInfo.absoluteFilePath();
    if (!outputInfo.absoluteDir().exists()) {
        return fail(QStringLiteral("The output directory does not exist:\n%1")
                        .arg(outputInfo.absolutePath()));
    } // if output directory is missing
    if (outputInfo.exists() && (!outputInfo.isFile() || !outputInfo.isWritable())) {
        return fail(QStringLiteral("The output path is not a writable file:\n%1")
                        .arg(outputPath));
    } // if output cannot be written

    QString settingsPath;
    if (parser.isSet(settingsOption)) {
        const QFileInfo settingsInfo(parser.value(settingsOption));
        if (!settingsInfo.exists() || !settingsInfo.isFile() || !settingsInfo.isReadable()) {
            return fail(QStringLiteral("The layout settings file cannot be read:\n%1")
                            .arg(settingsInfo.absoluteFilePath()));
        } // if settings file is invalid
        settingsPath = settingsInfo.absoluteFilePath();
    } // if explicit settings file

    QString exportOptionsPath;
    if (parser.isSet(exportOptionsOption)) {
        const QFileInfo exportOptionsInfo(parser.value(exportOptionsOption));
        if (!exportOptionsInfo.exists() || !exportOptionsInfo.isFile() || !exportOptionsInfo.isReadable()) {
            return fail(QStringLiteral("The export options file cannot be read:\n%1")
                            .arg(exportOptionsInfo.absoluteFilePath()));
        } // if export options file is invalid
        exportOptionsPath = exportOptionsInfo.absoluteFilePath();
    } // if explicit format options

    options.m_headlessExport = true;
    options.m_exportFormat = format;
    options.m_outputPath = outputPath;
    options.m_settingsPath = settingsPath;
    options.m_exportOptionsPath = exportOptionsPath;
    options.m_svgTextMode = canonicalTextMode;
    options.m_exportScale = scale;
    return options;
} // StartupOptions::parseExportOptions

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
