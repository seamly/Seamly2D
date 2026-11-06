// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file HeadlessExport.cpp
// @brief Runs the existing layout and export pipeline without loading QML.

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
} // namespace

// @brief Import, lay out, and export one SVG without creating a QML window.
// The same AppController methods used by Main.qml keep file and stdin exports
// on the established rendering, layout, and export paths.
int HeadlessExport::run(const StartupOptions &options, QString &errorMessage)
{
    PreferencesModel preferences;
    if (!preferences.load(preferences.defaultPreferencesFilePath())) {
        errorMessage = QStringLiteral("Cannot load SeamlyLayout preferences.");
        return ExitSoftware;
    } // if preferences are invalid

    SettingsModel settings;
    const QString settingsPath = options.settingsPath().isEmpty()
        ? preferences.settingsFilePath()
        : options.settingsPath();
    if (!settings.load(settingsPath)) {
        errorMessage = QStringLiteral("Cannot load layout settings: %1").arg(settingsPath);
        return ExitUsage;
    } // if settings are invalid
    const QString settingsJson = settings.toJson();

    QJsonObject formatOptions;
    if (!readExportOptions(options.exportOptionsPath(), formatOptions, errorMessage)) {
        return ExitUsage;
    } // if format-specific options are invalid

    AppController controller;
    QObject::connect(&controller, &AppController::errorOccurred, &controller,
                     [&errorMessage](const QString &message) {
        errorMessage = message;
    });
    QObject::connect(&controller, &AppController::oversizedPiecesFound, &controller,
                     [&errorMessage](const QString &message) {
        errorMessage = message;
    });
    bool incomplete_layout = false;
    QObject::connect(&controller, &AppController::layoutWarning, &controller,
                     [&errorMessage, &incomplete_layout](const QString &message) {
        errorMessage = message;
        incomplete_layout = true;
    });

    const bool imported = options.hasFile()
        ? controller.importSvg(options.filePath())
        : controller.importSvgDocument(options.svgDocument());
    if (!imported) {
        if (errorMessage.isEmpty()) {
            errorMessage = QStringLiteral("The input SVG could not be imported.");
        } // if no import error was reported
        return ExitNoInput;
    } // if input import failed

    if (!controller.initializeLayout(settingsJson)) {
        if (errorMessage.isEmpty()) {
            errorMessage = QStringLiteral("The initial layout could not be prepared.");
        } // if no initialization error was reported
        return ExitSoftware;
    } // if layout initialization failed

    if (!controller.processLayout(settingsJson)) {
        if (errorMessage.isEmpty()) {
            errorMessage = QStringLiteral("The layout could not be calculated.");
        } // if no layout error was reported
        return ExitSoftware;
    } // if layout calculation failed
    if (incomplete_layout) {
        return ExitSoftware;
    } // do not silently export a layout that omits input pieces

    const QString outputPath = options.outputPath();
    const QString format = options.exportFormat();
    bool exported = false;
    if (format == QStringLiteral("svg")) {
        exported = controller.exportSvg(outputPath, options.svgTextMode());
    } else if (format == QStringLiteral("pdf")) {
        exported = controller.exportPdf(outputPath, settingsJson);
    } else if (format == QStringLiteral("pdf-tiled")) {
        exported = controller.exportPdfTiled(outputPath, settingsJson);
    } else if (format == QStringLiteral("png")) {
        exported = controller.exportPng(outputPath, static_cast<float>(options.exportScale()));
    } else if (format == QStringLiteral("jpg") || format == QStringLiteral("jpeg")) {
        exported = controller.exportJpeg(outputPath);
    } else if (format == QStringLiteral("dxf")) {
        exported = controller.exportDxf(outputPath, compactJson(dxfOptions(options, formatOptions)));
    } else if (format == QStringLiteral("hpgl")) {
        exported = controller.exportHpgl(outputPath, compactJson(hpglOptions(preferences, formatOptions)));
    } else if (format == QStringLiteral("ps") || format == QStringLiteral("eps")) {
        exported = controller.exportPostscript(outputPath, format);
    } else if (format == QStringLiteral("gcode")) {
        if (!controller.isPaidExportAvailable(QStringLiteral("gcode"))) {
            errorMessage = QStringLiteral("G-Code export is not available in this build.");
            return ExitUnavailable;
        } // if G-Code export is unavailable
        exported = controller.exportGcode(outputPath);
    } else if (format == QStringLiteral("3mf")) {
        if (!controller.isPaidExportAvailable(QStringLiteral("3mf"))) {
            errorMessage = QStringLiteral("3MF export is not available in this build.");
            return ExitUnavailable;
        } // if 3MF export is unavailable
        exported = controller.exportMesh(outputPath);
    } // if selected format

    if (!exported) {
        if (errorMessage.isEmpty()) {
            errorMessage = QStringLiteral("SeamlyLayout failed to export %1.").arg(format);
        } // if no export error was reported
        return ExitOutputError;
    } // if export failed

    if (!QFileInfo::exists(outputPath) || QFileInfo(outputPath).size() == 0) {
        errorMessage = QStringLiteral("SeamlyLayout reported success but did not create an output file: %1")
                           .arg(outputPath);
        return ExitOutputError;
    } // if output file was not created

    return ExitSuccess;
} // HeadlessExport::run
