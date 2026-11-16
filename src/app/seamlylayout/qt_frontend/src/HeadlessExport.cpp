// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file HeadlessExport.cpp
// @brief Runs the existing layout and export pipeline without loading QML.
//
// Workflow of run():
//   1. Load preferences and settings; an explicit --settings file gets a strict check.
//   2. Import the SVG and build each material's blank canvas.
//   3. --info: return the JSON report and stop.
//   4. --no-packing: show the material's pieces where Seamly2D placed them, then go to step 7.
//      Otherwise lay out every material and view, as the Create Layout button does; oversized
//      pieces stop the run unless --oversize chooses split or extend.
//   5. Refuse a layout with unplaced pieces or a failed view.
//   6. Show the --material and --view layout, then check the format against the
//      Export menu rules (export_rules.rs).
//   7. Remove an old file at the output path, export, and remove a partial file on failure.
//      A run that stops before step 7 leaves an existing output file unchanged.

#include "HeadlessExport.h"

#include "PreferencesModel.h"
#include "SettingsModel.h"
#include "StartupOptions.h"

#include <cxxqt_bridge/src/lib.cxxqt.h>

#include <QDateTime>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>

namespace
{
enum ExitCode
{
    ExitSuccess = 0,
    ExitUsage = 64,
    ExitNoInput = 66,
    ExitUnavailable = 69,
    ExitSoftware = 70,
    ExitOutputError = 73
}; // enum ExitCode

// @brief Read a format-specific options object from JSON, or return an empty object.
bool readExportOptions(const QString &path, QJsonObject &options, QString &errorMessage)
{
    if (path.isEmpty()) {
        return true;
    } // if no format-specific options file

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMessage = QStringLiteral("Cannot read export options file: %1").arg(path);
        return false;
    } // if file cannot be opened

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        errorMessage = QStringLiteral("Export options must be a JSON object: %1").arg(path);
        return false;
    } // if JSON is invalid

    options = document.object();
    return true;
} // readExportOptions

// @brief Merge user DXF options over the same defaults used by the QML export path.
QJsonObject dxfOptions(const StartupOptions &startupOptions, const QJsonObject &overrides)
{
    QJsonObject options;
    options[QStringLiteral("createAnnotatedVersion")] = false;
    options[QStringLiteral("clo3dGroup250")] = false;
    options[QStringLiteral("aama")] = false;
    options[QStringLiteral("dxfVersion")] = QStringLiteral("R13");
    options[QStringLiteral("styleName")] = startupOptions.documentName();
    options[QStringLiteral("appVersion")] = QCoreApplication::applicationVersion();
    options[QStringLiteral("creationDate")] = QDate::currentDate().toString(QStringLiteral("dd-MM-yyyy"));
    options[QStringLiteral("creationTime")] = QTime::currentTime().toString(QStringLiteral("hh-mm"));

    for (auto item = overrides.constBegin(); item != overrides.constEnd(); ++item) {
        options.insert(item.key(), item.value());
    } // for each format option
    return options;
} // dxfOptions

// @brief Merge user HP-GL options over preferences-backed pen defaults.
QJsonObject hpglOptions(const PreferencesModel &preferences, const QJsonObject &overrides)
{
    QJsonObject options;
    options[QStringLiteral("mode")] = QStringLiteral("plot");
    options[QStringLiteral("cutPen")] = preferences.hpglCutPen();
    options[QStringLiteral("markPen")] = preferences.hpglMarkPen();
    options[QStringLiteral("labelPen")] = preferences.hpglLabelPen();

    for (auto item = overrides.constBegin(); item != overrides.constEnd(); ++item) {
        options.insert(item.key(), item.value());
    } // for each format option
    return options;
} // hpglOptions

// @brief Serialize an options object with the compact JSON shape used by QML.
QString compactJson(const QJsonObject &object)
{
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
} // compactJson

// @brief Index of the --view name in the tab labels; -1 when no label matches.
//
// Accepted names, case-insensitive: a full label ("Base Size 48", "Size 40"),
// "marker", "nested", "base" (the Base Size tab), or a size value ("40" = "Size 40").
int viewIndex(const QStringList &labels, const QString &view)
{
    const QString name = view.trimmed();
    const QString basePrefix = QStringLiteral("Base Size ");
    for (int index = 0; index < labels.size(); ++index) {
        const QString &label = labels.at(index);
        const bool matches = label.compare(name, Qt::CaseInsensitive) == 0
            || (name.compare(QStringLiteral("base"), Qt::CaseInsensitive) == 0 && label.startsWith(basePrefix))
            || label == QStringLiteral("Size ") + name;
        if (matches) {
            return index;
        } // if the label matches
    } // for each label
    return -1;
} // viewIndex

// @brief Text of the run's materials for a message, e.g. "Fabric, Interfacing, Lining".
QString materialList(const QStringList &labels, const QString &onlyMaterial)
{
    if (!labels.isEmpty()) {
        return labels.join(QStringLiteral(", "));
    } // if material tabs
    return onlyMaterial.isEmpty() ? QStringLiteral("(no material name)") : onlyMaterial;
} // materialList
// @brief Check the format against the shown layout, then write the output file.
// @param controller    Controller with the layout to export (laid out, or pieces as placed).
// @param options       Validated command-line request.
// @param preferences   Preferences for the HP-GL pens.
// @param settingsJson  Layout settings of the run.
// @param formatOptions Format-specific options from --export-options.
// @param errorMessage  Last error the controller reported; updated during the export.
// @param result        Result so far (warnings); returned with the exit code and error.
HeadlessExport::Result exportLayout(AppController &controller, const StartupOptions &options,
                                    const PreferencesModel &preferences, const QString &settingsJson,
                                    const QJsonObject &formatOptions, const QString &errorMessage,
                                    HeadlessExport::Result result)
{
    const auto fail = [&result](int exitCode, const QString &message) {
        result.exitCode = exitCode;
        result.error = message;
        return result;
    }; // fail

    // Labels that are already paths take "asSupplied" unless a mode was requested.
    // --text2paths writes SVG labels as single-stroke paths (hersheyStrokes).
    const QString format = options.exportFormat();
    const bool pathLabels = controller.getLabelTextState() == QStringLiteral("pathsOnly");
    QString textMode = options.isSvgTextModeSet() || !pathLabels ? options.svgTextMode()
                                                                 : QStringLiteral("asSupplied");
    if (options.isTextAsPaths()) {
        textMode = pathLabels ? QStringLiteral("asSupplied") : QStringLiteral("hersheyStrokes");
    } // if text as paths
    controller.setPdfTextAsPaths(options.isTextAsPaths());
    const QString formatProblem =
        controller.exportFormatProblem(format, options.isSvgTextModeSet() ? textMode : QString());
    if (!formatProblem.isEmpty()) {
        return fail(format == QStringLiteral("gcode") || format == QStringLiteral("3mf") ? ExitUnavailable
                                                                                         : ExitUsage,
                    formatProblem);
    } // if the Export menu would disable the format

    // A run that fails before this point keeps an existing file. Remove it now: after a
    // failed export, an old file would pass the existence check below.
    const QString outputPath = options.outputPath();
    if (QFileInfo::exists(outputPath) && !QFile::remove(outputPath)) {
        return fail(ExitOutputError, QStringLiteral("The old output file cannot be replaced: %1").arg(outputPath));
    } // if the old output file stays

    bool exported = false;
    if (format == QStringLiteral("svg")) {
        exported = controller.exportSvg(outputPath, textMode);
    } else if (format == QStringLiteral("pdf")) {
        exported = controller.exportPdf(outputPath, settingsJson);
    } else if (format == QStringLiteral("pdf-tiled")) {
        exported = controller.exportPdfTiled(outputPath, settingsJson);
    } else if (format == QStringLiteral("png")) {
        exported = controller.exportPng(outputPath);
    } else if (format == QStringLiteral("jpg") || format == QStringLiteral("jpeg")) {
        exported = controller.exportJpeg(outputPath);
    } else if (format == QStringLiteral("dxf")) {
        exported = controller.exportDxf(outputPath, compactJson(dxfOptions(options, formatOptions)));
    } else if (format == QStringLiteral("hpgl")) {
        exported = controller.exportHpgl(outputPath, compactJson(hpglOptions(preferences, formatOptions)));
    } else if (format == QStringLiteral("ps") || format == QStringLiteral("eps")) {
        exported = controller.exportPostscript(outputPath, format);
    } else if (format == QStringLiteral("gcode")) {
        exported = controller.exportGcode(outputPath);
    } else if (format == QStringLiteral("3mf")) {
        exported = controller.exportMesh(outputPath);
    } // if selected format

    if (!exported) {
        QFile::remove(outputPath); // no partial file stays after a failed export
        return fail(ExitOutputError, errorMessage.isEmpty()
                                         ? QStringLiteral("SeamlyLayout failed to export %1.").arg(format)
                                         : errorMessage);
    } // if export failed

    if (!QFileInfo::exists(outputPath) || QFileInfo(outputPath).size() == 0) {
        QFile::remove(outputPath); // an empty file is not an export
        return fail(ExitOutputError,
                    QStringLiteral("SeamlyLayout reported success but did not create an output file: %1")
                        .arg(outputPath));
    } // if output file was not created

    return result;
} // exportLayout
} // namespace

// @brief Import, lay out, and export one SVG, or report on it, without creating a QML window.
// The same AppController methods used by Main.qml keep file and stdin exports
// on the established rendering, layout, and export paths.
HeadlessExport::Result HeadlessExport::run(const StartupOptions &options)
{
    Result result;
    const auto fail = [&result](int exitCode, const QString &message) {
        result.exitCode = exitCode;
        result.error = message;
        return result;
    }; // fail

    PreferencesModel preferences;
    if (!preferences.load(preferences.defaultPreferencesFilePath())) {
        return fail(ExitSoftware, QStringLiteral("Cannot load SeamlyLayout preferences."));
    } // if preferences are invalid

    AppController controller;
    QString errorMessage;
    QObject::connect(&controller, &AppController::errorOccurred, &controller,
                     [&errorMessage](const QString &message) {
        errorMessage = message;
    });
    QString oversizedMessage;
    QObject::connect(&controller, &AppController::oversizedPiecesFound, &controller,
                     [&oversizedMessage](const QString &message) {
        oversizedMessage = message;
    });
    QString layoutWarning;
    QObject::connect(&controller, &AppController::layoutWarning, &controller,
                     [&layoutWarning](const QString &message) {
        layoutWarning = message;
    });

    // An explicit settings file gets the strict check: the command line shows no dialog
    // where a value coerced to its default could be seen.
    if (!options.settingsPath().isEmpty()) {
        QFile settingsFile(options.settingsPath());
        if (!settingsFile.open(QIODevice::ReadOnly)) {
            return fail(ExitUsage, QStringLiteral("Cannot read layout settings: %1").arg(options.settingsPath()));
        } // if settings file cannot be opened
        const QString problems =
            controller.settingsJsonProblems(QString::fromUtf8(settingsFile.readAll()));
        if (!problems.isEmpty()) {
            return fail(ExitUsage, QStringLiteral("Invalid layout settings in %1:\n%2")
                                       .arg(options.settingsPath(), problems));
        } // if settings are invalid
    } // if explicit settings file

    SettingsModel settings;
    const QString settingsPath = options.settingsPath().isEmpty()
        ? preferences.settingsFilePath()
        : options.settingsPath();
    if (!settings.load(settingsPath)) {
        return fail(ExitUsage, QStringLiteral("Cannot load layout settings: %1").arg(settingsPath));
    } // if settings are invalid
    QString settingsJson = settings.toJson();

    // Direct layout flags win over the settings file. The merged object is loaded through
    // SettingsModel, so roll sizes and fabric selvedges take the same path as a settings file.
    if (!options.layoutFlags().isEmpty()) {
        const QString flags = options.layoutFlags().join(QLatin1Char('\n'));
        const QString problems = controller.layoutFlagsProblems(settingsJson, flags);
        if (!problems.isEmpty()) {
            return fail(ExitUsage, QStringLiteral("Invalid layout flags:\n%1").arg(problems));
        } // if the flags do not apply
        QTemporaryFile mergedFile;
        if (!mergedFile.open()) {
            return fail(ExitSoftware, QStringLiteral("Cannot write the merged layout settings."));
        } // if no temporary file
        mergedFile.write(controller.layoutFlagsJson(settingsJson, flags).toUtf8());
        mergedFile.close();
        SettingsModel mergedSettings;
        if (!mergedSettings.load(mergedFile.fileName())) {
            return fail(ExitSoftware, QStringLiteral("Cannot load the merged layout settings."));
        } // if the merged settings do not load
        settingsJson = mergedSettings.toJson();
    } // if direct layout flags

    QJsonObject formatOptions;
    if (!readExportOptions(options.exportOptionsPath(), formatOptions, errorMessage)) {
        return fail(ExitUsage, errorMessage);
    } // if format-specific options are invalid

    const bool imported = options.hasFile()
        ? controller.importSvg(options.filePath())
        : controller.importSvgDocument(options.svgDocument());
    if (!imported) {
        return fail(ExitNoInput, errorMessage.isEmpty() ? QStringLiteral("The input SVG could not be imported.")
                                                        : errorMessage);
    } // if input import failed

    if (!controller.initializeLayout(settingsJson)) {
        return fail(ExitSoftware, errorMessage.isEmpty() ? QStringLiteral("The initial layout could not be prepared.")
                                                         : errorMessage);
    } // if layout initialization failed

    if (options.isInfoRequest()) {
        result.report = controller.layoutInfoJson(settingsJson);
        return result;
    } // if only the report was requested

    // Check --material before the layout run, so a typing error costs no layout time.
    const QStringList materials = controller.getMaterialLabels();
    const QString onlyMaterial = controller.getActiveMaterialName();
    int materialIndex = -1;
    if (!options.material().isEmpty()) {
        // One material: no material tabs; compare with the shown material's name.
        const QStringList names = materials.isEmpty() ? QStringList{onlyMaterial} : materials;
        for (int index = 0; index < names.size() && materialIndex < 0; ++index) {
            if (names.at(index).compare(options.material(), Qt::CaseInsensitive) == 0) {
                materialIndex = index;
            } // if the name matches
        } // for each material
        if (materialIndex < 0) {
            return fail(ExitUsage, QStringLiteral("The input has no material '%1'. Materials: %2.")
                                       .arg(options.material(), materialList(materials, onlyMaterial)));
        } // if the material is unknown
    } else if (materials.size() > 1) {
        return fail(ExitUsage, QStringLiteral("The input has %1 materials: %2. Select one with --material <name> "
                                              "(Seamly2D: --layout-material <name>).")
                                   .arg(materials.size()).arg(materials.join(QStringLiteral(", "))));
    } // if several materials and none selected

    // --no-packing: the pieces stay where Seamly2D placed them; no layout run, no views.
    if (options.isNoPacking()) {
        if (materialIndex > 0 && !controller.selectMaterial(materialIndex)) {
            return fail(ExitSoftware, QStringLiteral("The %1 pieces could not be selected.").arg(options.material()));
        } // if another material was selected
        if (!controller.useHandoffPositions()) {
            return fail(ExitSoftware, errorMessage.isEmpty() ? QStringLiteral("The pieces could not be placed.")
                                                             : errorMessage);
        } // if no pieces
        return exportLayout(controller, options, preferences, settingsJson, formatOptions, errorMessage, result);
    } // if no packing

    // Lay out every material and view; the GUI shows a dialog for oversized pieces.
    bool laidOut = controller.processLayout(settingsJson);
    if (!laidOut && !oversizedMessage.isEmpty()) {
        if (options.oversize().isEmpty()) {
            return fail(ExitSoftware, QStringLiteral("These pieces do not fit the media inside its margins:\n%1\n"
                                                     "Use --oversize split or --oversize extend "
                                                     "(Seamly2D: --layout-oversize).")
                                          .arg(oversizedMessage));
        } // if no oversize choice
        laidOut = controller.processLayoutOversized(settingsJson, options.oversize());
    } // if oversized pieces
    if (!laidOut) {
        return fail(ExitSoftware, errorMessage.isEmpty() ? QStringLiteral("The layout could not be calculated.")
                                                         : errorMessage);
    } // if layout calculation failed

    // QML runs one queued view per timer tick; here they run back to back.
    while (controller.getIsTabLayoutPending()) {
        controller.processNextTabLayout();
    } // while views are queued

    const QString incomplete = controller.incompleteLayoutReport();
    if (!incomplete.isEmpty()) {
        return fail(ExitSoftware, incomplete);
    } // do not silently export a layout that omits input pieces
    if (!layoutWarning.isEmpty()) {
        result.warnings.append(layoutWarning);
    } // if longer media, rotation or fold line notes

    if (materialIndex > 0 && !controller.selectMaterial(materialIndex)) {
        return fail(ExitSoftware, QStringLiteral("The %1 layout could not be selected.").arg(options.material()));
    } // if another material was selected

    const QStringList views = controller.getLayoutViewLabels();
    if (!options.view().isEmpty()) {
        if (views.isEmpty()) {
            return fail(ExitUsage, QStringLiteral("--view selects a multisize layout; this layout has one view."));
        } // if no views
        const int index = viewIndex(views, options.view());
        if (index < 0) {
            return fail(ExitUsage, QStringLiteral("The layout has no view '%1'. Views: %2.")
                                       .arg(options.view(), views.join(QStringLiteral(", "))));
        } // if the view is unknown
        if (!controller.selectLayoutView(index)) {
            return fail(ExitSoftware, QStringLiteral("The %1 view could not be selected.").arg(views.at(index)));
        } // if the view could not be shown
    } // if a view was selected
    const QString unavailable = controller.getLayoutViewUnavailableText();
    if (!unavailable.isEmpty()) {
        return fail(ExitUsage, unavailable);
    } // if the view has no layout (Marker or Nested of a split run)

    return exportLayout(controller, options, preferences, settingsJson, formatOptions, errorMessage, result);
} // HeadlessExport::run
