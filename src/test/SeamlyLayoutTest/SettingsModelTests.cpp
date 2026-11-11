// project: SeamlyLayout
// author: slspencer, copyright 2026 Seamly2D Project
// MIT License: https://opensource.org/licenses/MIT
//
// @file SettingsModelTests.cpp
// @brief Qt tests for SettingsModel load/save, defaults, and legacy schema migration.
//
// Covers:
//   • Default field values after construction
//   • Paper Type choices, including the custom sheet and its dialog fields
//   • Roll Width choices, including the custom roll width and its dialog field
//   • Legacy layoutMode schema migration (pre-2026-05 → current)
//   • Save/load round-trip preserving all fields
//   • Unit conversion helpers
//   • resetToDefaults() restoring known values
//   • Signal emission after load() — BC.1 regression suite
//     load() must emit ALL property notify signals and settingsLoaded()
//     unconditionally, even when the loaded value equals the current
//     model value (setters guard against same-value emission, so the
//     force-emit at end of load() is the only emission path for those
//     fields, which is critical for refreshing QML controls whose
//     runtime bindings were broken by prior user interaction).
//   • BC.2 settings round-trip verification
//     Verifies the full pipeline: load file → all fields update →
//     save → saved JSON contains all expected keys with correct values.

#include "SettingsModel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <limits>

namespace
{
// @brief Register the Theme singleton that SettingsDialog.qml imports, once per process.
void register_theme_singleton()
{
    static const int type_id = qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/qml/Theme.qml")),
                                                        "SeamlyLayout", 1, 0, "Theme");
    Q_UNUSED(type_id)
} // register_theme_singleton()
} // namespace

class SettingsModelTests : public QObject
{
    Q_OBJECT

private slots:
    // Default values
    void defaults_unitIsInches();
    void defaults_mediaTypeIsPaper();
    void defaults_paperTypeIsSheet();
    void defaults_layoutModeIsAlongGrainline();
    void paperSizeLabels_includeDimensions();
    void tileSizes_matchPredefinedSheets();
    void tileSizes_selectAndRoundtripNames();
    void tileSizeDropdown_displaysDimensionsAndRestoresSelection();
    void selectPaperSize_customKeepsPageDimensions();
    void paperType_customChoiceStoresSheet();
    void paperTypeDropdown_customShowsSizeFields();
    void pageDimensions_rejectInvalidValues();
    void customPaperDimensions_roundtrip();
    void rollSize_customKeepsRollWidth();
    void rollSizeDropdown_customShowsWidthField();

    // Legacy layoutMode migration
    void legacy_withGrainRotationEnabled_mapsToAlongGrainline();
    void legacy_withGrainNoRotation_mapsToWithNap();
    void legacy_withoutGrain_mapsToAlongGrainline();

    // Save / load round-trip
    void roundtrip_preservesUnit();
    void roundtrip_preservesMargins();
    void roundtrip_preservesMediaType();
    void roundtrip_preservesLayoutMode();
    void roundtrip_preservesLayoutModeAny();
    void roundtrip_preservesRotationStep();

    // Pieces without a grainline
    void noGrainlineRotation_defaultsToUpright();
    void noGrainlineRotation_roundtripAndJson();
    void scaleGrid_defaultsOffAndRoundtrips();
    void foldLinesOnLeftEdge_defaultsOffAndRoundtrips();
    void noGrainlineRotation_unknownValueIsUpright();

    // Retired keys
    void multisizeLayout_storedKeyIsDropped();

    // resetToDefaults
    void resetToDefaults_restoresUnit();
    void resetToDefaults_restoresLayoutMode();
    void resetToDefaults_restoresMargins();

    // BC.1 — signal emission after load()
    void load_emitsSettingsLoadedSignal();
    void load_emitsUnitChangedEvenWhenValueUnchanged();
    void load_emitsMarginSignalsEvenWhenValuesUnchanged();
    void load_emitsLayoutModeChangedEvenWhenValueUnchanged();
    void load_emitsAllPropertySignals();

    // BC.2 — settings round-trip verification
    void bc2_loadFile_allFieldsUpdate();
    void bc2_saveAfterLoad_savedJsonContainsAllKeys();
    void bc2_savedJson_fieldValuesMatchModel();
    void bc2_fullPipeline_loadEditSaveReload_fieldsMatch();
}; // class SettingsModelTests

// ---------------------------------------------------------------------------
// Default values
// ---------------------------------------------------------------------------

// @brief Freshly constructed model reports "in" as the active unit.
void SettingsModelTests::defaults_unitIsInches()
{
    SettingsModel m;
    QCOMPARE(m.unit(), QStringLiteral("in"));
}

// @brief Freshly constructed model reports "paper" as the media type.
void SettingsModelTests::defaults_mediaTypeIsPaper()
{
    SettingsModel m;
    QCOMPARE(m.mediaType(), QStringLiteral("paper"));
}

// @brief Freshly constructed model reports "sheet" as the paper type.
void SettingsModelTests::defaults_paperTypeIsSheet()
{
    SettingsModel m;
    QCOMPARE(m.paperType(), QStringLiteral("sheet"));
}

// @brief Freshly constructed model reports "alongGrainline" as the layout mode.
void SettingsModelTests::defaults_layoutModeIsAlongGrainline()
{
    SettingsModel m;
    QCOMPARE(m.layoutMode(), QStringLiteral("alongGrainline"));
}

// @brief Paper size labels show imperial and metric dimensions; Custom moved to the Paper Type list.
void SettingsModelTests::paperSizeLabels_includeDimensions()
{
    SettingsModel m;
    const QStringList names = m.paperSizeNames();
    const QStringList labels = m.paperSizeLabels();

    QCOMPARE(names.size(), labels.size());
    QCOMPARE(labels.at(names.indexOf(QStringLiteral("Letter"))),
             QStringLiteral("Letter (8.5 × 11 in; 216 × 279 mm)"));
    QVERIFY(!names.contains(QStringLiteral("Custom")));
    QVERIFY(!labels.contains(QStringLiteral("Custom")));
}

// @brief Tile choices reuse every predefined sheet name and dimension label.
void SettingsModelTests::tileSizes_matchPredefinedSheets()
{
    SettingsModel model;
    const QStringList names = model.paperSizeNames();
    const QStringList labels = model.paperSizeLabels();

    QCOMPARE(model.tileSizeNames(), names);
    QCOMPARE(model.tileSizeLabels(), labels);
    QCOMPARE(names.size(), 27);
    QVERIFY(!names.contains(QStringLiteral("None")));
    QVERIFY(!names.contains(QStringLiteral("Custom")));
    QCOMPARE(labels.at(names.indexOf(QStringLiteral("ANSI E"))),
             QStringLiteral("ANSI E (34 × 44 in; 864 × 1118 mm)"));
    QCOMPARE(labels.at(names.indexOf(QStringLiteral("A0"))),
             QStringLiteral("A0 (33.11 × 46.81 in; 841 × 1189 mm)"));
    QCOMPARE(labels.last(), QStringLiteral("B6 (4.92 × 6.93 in; 125 × 176 mm)"));
}

// @brief Selecting and saving any tile size preserves its stable name and orientation.
void SettingsModelTests::tileSizes_selectAndRoundtripNames()
{
    const QString file_name = QStringLiteral("tile-size-%1.settings").arg(QCoreApplication::applicationPid());
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath(file_name);
    SettingsModel writer;
    writer.setPaperType(QStringLiteral("tiled"));
    writer.setTileOrientation(QStringLiteral("portrait"));
    for (const QString &name : writer.tileSizeNames())
    {
        writer.selectTileSize(name);
        QCOMPARE(writer.tileSize(), name);
        QCOMPARE(QJsonDocument::fromJson(writer.toJson().toUtf8()).object()
                     .value(QStringLiteral("tileSize")).toString(), name);
        QVERIFY(writer.save(path));

        SettingsModel reader;
        QVERIFY(reader.load(path));
        QCOMPARE(reader.tileSize(), name);
        QCOMPARE(reader.tileOrientation(), QStringLiteral("portrait"));
    }
    QVERIFY(QFile::remove(path));

    const QString previous = writer.tileSize();
    writer.selectTileSize(QStringLiteral("None"));
    writer.selectTileSize(QStringLiteral("Custom"));
    writer.selectTileSize(QStringLiteral("unknown"));
    writer.selectTileSize(writer.tileSizeLabels().first());
    QCOMPARE(writer.tileSize(), previous);
    QCOMPARE(writer.sheetName(), QStringLiteral("ARCH E"));
}

// @brief The real dropdown displays labels, selects stable names, and refreshes after loading settings.
void SettingsModelTests::tileSizeDropdown_displaysDimensionsAndRestoresSelection()
{
    register_theme_singleton();
    SettingsModel model;
    model.setPaperType(QStringLiteral("tiled"));
    model.selectTileSize(QStringLiteral("ANSI D"));
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/SettingsDialog.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> dialog(component.createWithInitialProperties({
        {QStringLiteral("model"), QVariant::fromValue(&model)}
    }));
    QVERIFY2(dialog, qPrintable(component.errorString()));
    QObject *combo = dialog->findChild<QObject *>(QStringLiteral("tileSizeCombo"));
    QVERIFY(combo);
    QCOMPARE(combo->property("count").toInt(), 27);
    const QStringList names = model.tileSizeNames();
    const QStringList labels = model.tileSizeLabels();
    QCOMPARE(combo->property("currentIndex").toInt(), names.indexOf(QStringLiteral("ANSI D")));
    for (int index = 0; index < names.size(); ++index)
    {
        combo->setProperty("currentIndex", index);
        QCOMPARE(combo->property("currentText").toString(), labels.at(index));
        QVERIFY(QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, index)));
        QCOMPARE(model.tileSize(), names.at(index));
    }

    // Loading must repair the currentIndex binding replaced by interaction.
    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("tile-ui-%1.settings").arg(QCoreApplication::applicationPid()));
    SettingsModel writer;
    writer.setPaperType(QStringLiteral("tiled"));
    writer.selectTileSize(QStringLiteral("A0"));
    QVERIFY(writer.save(path));
    QVERIFY(model.load(path));
    QVERIFY(QFile::remove(path));
    QCOMPARE(combo->property("currentIndex").toInt(), names.indexOf(QStringLiteral("A0")));
    QCOMPARE(combo->property("currentText").toString(), labels.at(names.indexOf(QStringLiteral("A0"))));
}

// @brief Selecting Custom preserves the current dimensions for editing.
void SettingsModelTests::selectPaperSize_customKeepsPageDimensions()
{
    SettingsModel m;
    m.setPageWidth(400.5);
    m.setPageHeight(650.25);

    m.selectPaperSize(QStringLiteral("Custom"));

    QCOMPARE(m.sheetName(), QStringLiteral("Custom"));
    QCOMPARE(m.pageWidth(), 400.5);
    QCOMPARE(m.pageHeight(), 650.25);
}

// @brief The custom Paper Type stores a sheet named Custom; "sheet" restores a predefined size.
void SettingsModelTests::paperType_customChoiceStoresSheet()
{
    SettingsModel model;
    QCOMPARE(model.paperTypeNames(),
             QStringList({QStringLiteral("sheet"), QStringLiteral("custom"),
                          QStringLiteral("tiled"), QStringLiteral("roll")}));
    QCOMPARE(model.paperTypeChoice(), QStringLiteral("sheet"));

    QSignalSpy choice_spy(&model, &SettingsModel::paperTypeChoiceChanged);
    model.setPageWidth(40.5);
    model.setPageHeight(60.25);
    model.selectPaperType(QStringLiteral("custom"));

    // The layout core reads paperType "sheet" and the page dimensions.
    QCOMPARE(model.paperType(), QStringLiteral("sheet"));
    QCOMPARE(model.sheetName(), QStringLiteral("Custom"));
    QCOMPARE(model.paperTypeChoice(), QStringLiteral("custom"));
    QCOMPARE(model.pageWidth(), 40.5);
    QCOMPARE(model.pageHeight(), 60.25);
    QVERIFY(choice_spy.count() >= 1);
    const QJsonObject json = QJsonDocument::fromJson(model.toJson().toUtf8()).object();
    QCOMPARE(json.value(QStringLiteral("paperType")).toString(), QStringLiteral("sheet"));

    // Another paper type and back to custom keeps the custom size.
    model.selectPaperType(QStringLiteral("tiled"));
    QCOMPARE(model.paperTypeChoice(), QStringLiteral("tiled"));
    model.selectPaperType(QStringLiteral("custom"));
    QCOMPARE(model.pageWidth(), 40.5);

    // "sheet" replaces the custom size with the default predefined sheet.
    model.selectPaperType(QStringLiteral("sheet"));
    QCOMPARE(model.paperTypeChoice(), QStringLiteral("sheet"));
    QCOMPARE(model.sheetName(), QStringLiteral("ARCH E"));
    QCOMPARE(model.pageWidth(), 36.0);
    QCOMPARE(model.pageHeight(), 48.0);

    // "sheet" keeps a predefined size.
    model.selectPaperSize(QStringLiteral("A4"));
    model.selectPaperType(QStringLiteral("roll"));
    model.selectPaperType(QStringLiteral("sheet"));
    QCOMPARE(model.sheetName(), QStringLiteral("A4"));

    model.selectPaperType(QStringLiteral("unknown"));
    QCOMPARE(model.paperType(), QStringLiteral("sheet"));
    QCOMPARE(model.sheetName(), QStringLiteral("A4"));
} // paperType_customChoiceStoresSheet()

// @brief The dialog offers custom in Paper Type; its width and height fields edit the sheet.
void SettingsModelTests::paperTypeDropdown_customShowsSizeFields()
{
    register_theme_singleton();
    SettingsModel model;
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/SettingsDialog.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> dialog(component.createWithInitialProperties({
        {QStringLiteral("model"), QVariant::fromValue(&model)}
    }));
    QVERIFY2(dialog, qPrintable(component.errorString()));
    QObject* combo = dialog->findChild<QObject*>(QStringLiteral("paperTypeCombo"));
    QObject* width_field = dialog->findChild<QObject*>(QStringLiteral("customPaperWidthField"));
    QObject* height_field = dialog->findChild<QObject*>(QStringLiteral("customPaperHeightField"));
    QVERIFY(combo);
    QVERIFY(width_field);
    QVERIFY(height_field);
    QCOMPARE(combo->property("count").toInt(), 4);
    QVERIFY(dialog->property("isPredefinedSheet").toBool());
    QVERIFY(!dialog->property("isCustomPaper").toBool());

    // Select custom as a user does: the index changes, then activated fires.
    const int custom_index = model.paperTypeNames().indexOf(QStringLiteral("custom"));
    combo->setProperty("currentIndex", custom_index);
    QCOMPARE(combo->property("currentText").toString(), QStringLiteral("custom"));
    QVERIFY(QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, custom_index)));
    QCOMPARE(model.paperTypeChoice(), QStringLiteral("custom"));
    QVERIFY(dialog->property("isCustomPaper").toBool());
    QVERIFY(!dialog->property("isPredefinedSheet").toBool());
    QCOMPARE(width_field->property("text").toString(), QStringLiteral("36.00"));

    // Typed values go to the model in the active unit.
    width_field->setProperty("text", QStringLiteral("91.5"));
    QVERIFY(QMetaObject::invokeMethod(width_field, "editingFinished"));
    height_field->setProperty("text", QStringLiteral("120"));
    QVERIFY(QMetaObject::invokeMethod(height_field, "editingFinished"));
    QCOMPARE(model.pageWidth(), 91.5);
    QCOMPARE(model.pageHeight(), 120.0);
    QCOMPARE(width_field->property("text").toString(), QStringLiteral("91.50"));

    // A value that is not positive restores the model value.
    height_field->setProperty("text", QStringLiteral("0"));
    QVERIFY(QMetaObject::invokeMethod(height_field, "editingFinished"));
    QCOMPARE(model.pageHeight(), 120.0);
    QCOMPARE(height_field->property("text").toString(), QStringLiteral("120.00"));

    // Loading a saved custom sheet restores the dropdown and the fields.
    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("custom-ui-%1.settings").arg(QCoreApplication::applicationPid()));
    SettingsModel writer;
    writer.selectPaperType(QStringLiteral("custom"));
    writer.setPageWidth(50.0);
    writer.setPageHeight(70.0);
    QVERIFY(writer.save(path));
    combo->setProperty("currentIndex", 0);
    QVERIFY(model.load(path));
    QVERIFY(QFile::remove(path));
    QCOMPARE(combo->property("currentIndex").toInt(), custom_index);
    QCOMPARE(width_field->property("text").toString(), QStringLiteral("50.00"));
    QCOMPARE(height_field->property("text").toString(), QStringLiteral("70.00"));
} // paperTypeDropdown_customShowsSizeFields()

// @brief Page dimensions reject zero, negative, and non-finite values.
void SettingsModelTests::pageDimensions_rejectInvalidValues()
{
    SettingsModel m;
    const double originalWidth = m.pageWidth();
    const double originalHeight = m.pageHeight();

    m.setPageWidth(0.0);
    m.setPageWidth(-1.0);
    m.setPageWidth(std::numeric_limits<double>::infinity());
    m.setPageHeight(0.0);
    m.setPageHeight(-1.0);
    m.setPageHeight(std::numeric_limits<double>::quiet_NaN());

    QCOMPARE(m.pageWidth(), originalWidth);
    QCOMPARE(m.pageHeight(), originalHeight);
}

// @brief Custom sheet dimensions survive a save/load cycle.
void SettingsModelTests::customPaperDimensions_roundtrip()
{
    const QString fileName = QStringLiteral("custom-paper-%1.settings")
                                 .arg(QCoreApplication::applicationPid());
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath(fileName);
    QFile::remove(path);

    SettingsModel writer;
    writer.setUnit(QStringLiteral("mm"));
    writer.setSheetName(QStringLiteral("Custom"));
    writer.setPageWidth(400.5);
    writer.setPageHeight(650.25);
    QVERIFY(writer.save(path));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.sheetName(), QStringLiteral("Custom"));
    QCOMPARE(reader.pageWidth(), 400.5);
    QCOMPARE(reader.pageHeight(), 650.25);
    QFile::remove(path);
}

// @brief A "custom" roll size keeps the roll width for the user to edit; save and load keep it.
void SettingsModelTests::rollSize_customKeepsRollWidth()
{
    SettingsModel model;
    const QStringList names = model.rollSizeNames();
    QCOMPARE(names.last(), QStringLiteral("custom"));
    QCOMPARE(names.count(QStringLiteral("custom")), 1);

    // A predefined size sets the width; "custom" keeps it.
    model.setRollSize(QStringLiteral("48 in"));
    QCOMPARE(model.rollWidth(), 48.0);
    model.setRollSize(QStringLiteral("custom"));
    QCOMPARE(model.rollSize(), QStringLiteral("custom"));
    QCOMPARE(model.rollWidth(), 48.0);

    model.setRollWidth(52.5);
    QCOMPARE(model.rollWidthPx(), 5040);   // 52.5 in × 96 px/in

    // Invalid widths leave the roll width unchanged.
    model.setRollWidth(0.0);
    model.setRollWidth(-1.0);
    model.setRollWidth(std::numeric_limits<double>::quiet_NaN());
    QCOMPARE(model.rollWidth(), 52.5);

    // The layout core reads rollWidth from the saved settings.
    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("custom-roll-%1.settings").arg(QCoreApplication::applicationPid()));
    SettingsModel writer;
    writer.setUnit(QStringLiteral("mm"));
    writer.setRollSize(QStringLiteral("custom"));
    writer.setRollWidth(1350.0);
    const QJsonObject json = QJsonDocument::fromJson(writer.toJson().toUtf8()).object();
    QCOMPARE(json.value(QStringLiteral("rollSize")).toString(), QStringLiteral("custom"));
    QCOMPARE(json.value(QStringLiteral("rollWidth")).toDouble(), 1350.0);
    QVERIFY(writer.save(path));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QVERIFY(QFile::remove(path));
    QCOMPARE(reader.rollSize(), QStringLiteral("custom"));
    QCOMPARE(reader.rollWidth(), 1350.0);

    // A unit change converts the custom width.
    reader.convertAllUnits(QStringLiteral("mm"), QStringLiteral("cm"));
    QCOMPARE(reader.rollWidth(), 135.0);
} // rollSize_customKeepsRollWidth()

// @brief The dialog offers custom in Roll Width; its width field edits the roll width.
void SettingsModelTests::rollSizeDropdown_customShowsWidthField()
{
    register_theme_singleton();
    SettingsModel model;
    model.selectPaperType(QStringLiteral("roll"));
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/SettingsDialog.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> dialog(component.createWithInitialProperties({
        {QStringLiteral("model"), QVariant::fromValue(&model)}
    }));
    QVERIFY2(dialog, qPrintable(component.errorString()));
    QObject* combo = dialog->findChild<QObject*>(QStringLiteral("rollSizeCombo"));
    QObject* width_field = dialog->findChild<QObject*>(QStringLiteral("customRollWidthField"));
    QVERIFY(combo);
    QVERIFY(width_field);
    QCOMPARE(combo->property("count").toInt(), model.rollSizeNames().size());
    QVERIFY(dialog->property("isRollPaper").toBool());
    QVERIFY(!dialog->property("isCustomRoll").toBool());

    // Select 48 in, then custom, as a user does: the index changes, then activated fires.
    const int size_index = model.rollSizeNames().indexOf(QStringLiteral("48 in"));
    combo->setProperty("currentIndex", size_index);
    QVERIFY(QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, size_index)));
    QCOMPARE(model.rollWidth(), 48.0);
    const int custom_index = model.rollSizeNames().indexOf(QStringLiteral("custom"));
    combo->setProperty("currentIndex", custom_index);
    QCOMPARE(combo->property("currentText").toString(), QStringLiteral("custom"));
    QVERIFY(QMetaObject::invokeMethod(combo, "activated", Q_ARG(int, custom_index)));
    QCOMPARE(model.rollSize(), QStringLiteral("custom"));
    QVERIFY(dialog->property("isCustomRoll").toBool());
    QCOMPARE(width_field->property("text").toString(), QStringLiteral("48.00"));

    // A typed value goes to the model in the active unit.
    width_field->setProperty("text", QStringLiteral("54.25"));
    QVERIFY(QMetaObject::invokeMethod(width_field, "editingFinished"));
    QCOMPARE(model.rollWidth(), 54.25);
    QCOMPARE(width_field->property("text").toString(), QStringLiteral("54.25"));

    // A value that is not positive restores the model value.
    width_field->setProperty("text", QStringLiteral("0"));
    QVERIFY(QMetaObject::invokeMethod(width_field, "editingFinished"));
    QCOMPARE(model.rollWidth(), 54.25);
    QCOMPARE(width_field->property("text").toString(), QStringLiteral("54.25"));

    // Loading a saved custom roll restores the dropdown and the field.
    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath(QStringLiteral("custom-roll-ui-%1.settings").arg(QCoreApplication::applicationPid()));
    SettingsModel writer;
    writer.selectPaperType(QStringLiteral("roll"));
    writer.setRollSize(QStringLiteral("custom"));
    writer.setRollWidth(66.0);
    QVERIFY(writer.save(path));
    combo->setProperty("currentIndex", 0);
    QVERIFY(model.load(path));
    QVERIFY(QFile::remove(path));
    QCOMPARE(combo->property("currentIndex").toInt(), custom_index);
    QCOMPARE(width_field->property("text").toString(), QStringLiteral("66.00"));
} // rollSizeDropdown_customShowsWidthField()

// ---------------------------------------------------------------------------
// Legacy layoutMode migration
//
// Pre-2026-05 schema used "withGrain" and "withoutGrain" with an optional
// rotationEnabled flag. The new schema uses "alongGrainline" and "withNap".
//
// Migration map (from SettingsModel::load()):
//   withGrain    + rotationEnabled=true  → alongGrainline
//   withGrain    + rotationEnabled=false → withNap (rotationStep=0)
//   withGrain    + (no rotationEnabled)  → alongGrainline (old implicit default)
//   withoutGrain + (any)                 → alongGrainline
// ---------------------------------------------------------------------------

// @brief withGrain + rotationEnabled=true maps to alongGrainline.
void SettingsModelTests::legacy_withGrainRotationEnabled_mapsToAlongGrainline()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    QJsonObject obj;
    obj[QStringLiteral("layoutMode")]      = QStringLiteral("withGrain");
    obj[QStringLiteral("rotationEnabled")] = true;
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    SettingsModel m;
    QVERIFY(m.load(path));
    QCOMPARE(m.layoutMode(), QStringLiteral("alongGrainline"));
}

// @brief withGrain + rotationEnabled=false maps to withNap with rotationStep=0.
void SettingsModelTests::legacy_withGrainNoRotation_mapsToWithNap()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    QJsonObject obj;
    obj[QStringLiteral("layoutMode")]      = QStringLiteral("withGrain");
    obj[QStringLiteral("rotationEnabled")] = false;
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    SettingsModel m;
    QVERIFY(m.load(path));
    QCOMPARE(m.layoutMode(), QStringLiteral("withNap"));
    QCOMPARE(m.rotationStep(), 0.0);
}

// @brief withoutGrain maps to alongGrainline regardless of other fields.
void SettingsModelTests::legacy_withoutGrain_mapsToAlongGrainline()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    QJsonObject obj;
    obj[QStringLiteral("layoutMode")] = QStringLiteral("withoutGrain");
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    SettingsModel m;
    QVERIFY(m.load(path));
    QCOMPARE(m.layoutMode(), QStringLiteral("alongGrainline"));
}

// ---------------------------------------------------------------------------
// Save / load round-trip
// ---------------------------------------------------------------------------

// @brief unit field survives a save/load cycle.
void SettingsModelTests::roundtrip_preservesUnit()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    SettingsModel writer;
    writer.setUnit(QStringLiteral("mm"));
    QVERIFY(writer.save(path));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.unit(), QStringLiteral("mm"));
}

// @brief All four margin fields survive a save/load cycle.
void SettingsModelTests::roundtrip_preservesMargins()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    SettingsModel writer;
    writer.setMarginTop(1.0);
    writer.setMarginBottom(2.0);
    writer.setMarginLeft(3.0);
    writer.setMarginRight(4.0);
    QVERIFY(writer.save(path));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.marginTop(),    1.0);
    QCOMPARE(reader.marginBottom(), 2.0);
    QCOMPARE(reader.marginLeft(),   3.0);
    QCOMPARE(reader.marginRight(),  4.0);
}

// @brief mediaType field survives a save/load cycle.
void SettingsModelTests::roundtrip_preservesMediaType()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    SettingsModel writer;
    writer.setMediaType(QStringLiteral("fabric"));
    QVERIFY(writer.save(path));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.mediaType(), QStringLiteral("fabric"));
}

// @brief layoutMode field survives a save/load cycle.
void SettingsModelTests::roundtrip_preservesLayoutMode()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    SettingsModel writer;
    writer.setLayoutMode(QStringLiteral("withNap"));
    QVERIFY(writer.save(path));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.layoutMode(), QStringLiteral("withNap"));
}

// @brief layoutMode "any" is accepted and survives a save/load cycle.
void SettingsModelTests::roundtrip_preservesLayoutModeAny()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    SettingsModel writer;
    writer.setLayoutMode(QStringLiteral("any"));
    QCOMPARE(writer.layoutMode(), QStringLiteral("any"));
    QVERIFY(writer.save(path));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.layoutMode(), QStringLiteral("any"));
}

// @brief noGrainlineRotation defaults to "upright" and resetToDefaults restores it.
void SettingsModelTests::noGrainlineRotation_defaultsToUpright()
{
    SettingsModel m;
    QCOMPARE(m.noGrainlineRotation(), QStringLiteral("upright"));
    m.setNoGrainlineRotation(QStringLiteral("free"));
    m.resetToDefaults();
    QCOMPARE(m.noGrainlineRotation(), QStringLiteral("upright"));
}

// @brief "free" survives a save/load cycle and reaches the Rust settings JSON.
void SettingsModelTests::noGrainlineRotation_roundtripAndJson()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    SettingsModel writer;
    writer.setNoGrainlineRotation(QStringLiteral("free"));
    QVERIFY(writer.save(path));
    QVERIFY(writer.toJson().contains(QStringLiteral("\"noGrainlineRotation\":\"free\"")));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.noGrainlineRotation(), QStringLiteral("free"));
}

// @brief The scale check grid is off by default; the file and the Rust settings JSON carry it.
void SettingsModelTests::scaleGrid_defaultsOffAndRoundtrips()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    SettingsModel writer;
    QCOMPARE(writer.scaleGrid(), false);
    writer.setScaleGrid(true);
    QVERIFY(writer.save(path));
    QVERIFY(writer.toJson().contains(QStringLiteral("\"scaleGrid\":true")));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.scaleGrid(), true);
    reader.resetToDefaults();
    QCOMPARE(reader.scaleGrid(), false);
}

// @brief Fold lines on the left edge are off by default; the file and the Rust settings JSON carry the choice.
void SettingsModelTests::foldLinesOnLeftEdge_defaultsOffAndRoundtrips()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    SettingsModel writer;
    QCOMPARE(writer.foldLinesOnLeftEdge(), false);
    writer.setFoldLinesOnLeftEdge(true);
    QVERIFY(writer.save(path));
    QVERIFY(writer.toJson().contains(QStringLiteral("\"foldLinesOnLeftEdge\":true")));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.foldLinesOnLeftEdge(), true);
    reader.resetToDefaults();
    QCOMPARE(reader.foldLinesOnLeftEdge(), false);
}

// @brief An unknown value is coerced to "upright".
void SettingsModelTests::noGrainlineRotation_unknownValueIsUpright()
{
    SettingsModel m;
    m.setNoGrainlineRotation(QStringLiteral("free"));
    m.setNoGrainlineRotation(QStringLiteral("sideways"));
    QCOMPARE(m.noGrainlineRotation(), QStringLiteral("upright"));
}

// @brief A settings file with the retired multisizeLayout key still loads,
//        and the next save and the Rust settings JSON leave the key out.
void SettingsModelTests::multisizeLayout_storedKeyIsDropped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    QFile out(path);
    QVERIFY(out.open(QIODevice::WriteOnly));
    out.write(R"({"unit":"cm","multisizeLayout":"marker"})");
    out.close();

    SettingsModel m;
    QVERIFY(m.load(path));
    QCOMPARE(m.unit(), QStringLiteral("cm"));
    QVERIFY(!m.toJson().contains(QStringLiteral("multisizeLayout")));

    QVERIFY(m.save(path));
    QFile in(path);
    QVERIFY(in.open(QIODevice::ReadOnly));
    QVERIFY(!QJsonDocument::fromJson(in.readAll()).object().contains(QStringLiteral("multisizeLayout")));
}

// @brief rotationStep=180 (head-down withNap) survives a save/load cycle.
void SettingsModelTests::roundtrip_preservesRotationStep()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    SettingsModel writer;
    writer.setLayoutMode(QStringLiteral("withNap"));
    writer.setRotationStep(180.0);
    QVERIFY(writer.save(path));

    SettingsModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.layoutMode(),  QStringLiteral("withNap"));
    QCOMPARE(reader.rotationStep(), 180.0);
}

// ---------------------------------------------------------------------------
// resetToDefaults
// ---------------------------------------------------------------------------

// @brief resetToDefaults() restores unit to "in".
void SettingsModelTests::resetToDefaults_restoresUnit()
{
    SettingsModel m;
    m.setUnit(QStringLiteral("mm"));
    m.resetToDefaults();
    QCOMPARE(m.unit(), QStringLiteral("in"));
}

// @brief resetToDefaults() restores layoutMode to "alongGrainline".
void SettingsModelTests::resetToDefaults_restoresLayoutMode()
{
    SettingsModel m;
    m.setLayoutMode(QStringLiteral("withNap"));
    m.resetToDefaults();
    QCOMPARE(m.layoutMode(), QStringLiteral("alongGrainline"));
}

// @brief resetToDefaults() restores all margins to 0.25 in (inch default).
void SettingsModelTests::resetToDefaults_restoresMargins()
{
    SettingsModel m;
    m.setMarginTop(5.0);
    m.setMarginBottom(5.0);
    m.setMarginLeft(5.0);
    m.setMarginRight(5.0);
    m.resetToDefaults();
    QCOMPARE(m.marginTop(),    0.25);
    QCOMPARE(m.marginBottom(), 0.25);
    QCOMPARE(m.marginLeft(),   0.25);
    QCOMPARE(m.marginRight(),  0.25);
}

// ---------------------------------------------------------------------------
// BC.1 — Signal emission after load()
//
// These tests verify the fix for the Phase 5 bug: SettingsModel::load()
// must emit all property notify signals AND settingsLoaded() unconditionally
// at the end of a successful load, even for fields whose loaded value equals
// the current model value (where the setter guard would otherwise suppress
// the signal).  Without these unconditional emissions, QML controls with
// broken runtime bindings (modified by user interaction) never refresh.
// ---------------------------------------------------------------------------

// @brief settingsLoaded() signal is emitted after a successful load().
void SettingsModelTests::load_emitsSettingsLoadedSignal()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    // Write a file with default unit so the setter guard would suppress unitChanged()
    // without the BC.1 fix.
    QJsonObject obj;
    obj[QStringLiteral("unit")] = QStringLiteral("in");
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    SettingsModel m;
    QSignalSpy spy(&m, &SettingsModel::settingsLoaded);
    QVERIFY(m.load(path));
    QCOMPARE(spy.count(), 1);
} // load_emitsSettingsLoadedSignal()

// @brief unitChanged() is emitted even when the loaded unit equals the current unit.
// Regression test for BC.1: the setter early-return guard was the only
// emission path before the fix.
void SettingsModelTests::load_emitsUnitChangedEvenWhenValueUnchanged()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    // Write file with unit="in" — same as the model's default.
    QJsonObject obj;
    obj[QStringLiteral("unit")] = QStringLiteral("in");
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    SettingsModel m;
    // Confirm model already has unit="in" (setter guard would suppress emission).
    QCOMPARE(m.unit(), QStringLiteral("in"));

    QSignalSpy spy(&m, &SettingsModel::unitChanged);
    QVERIFY(m.load(path));
    // Must fire at least once from the unconditional force-emit at end of load().
    QVERIFY(spy.count() >= 1);
} // load_emitsUnitChangedEvenWhenValueUnchanged()

// @brief All four margin signals are emitted even when loaded values match current model.
void SettingsModelTests::load_emitsMarginSignalsEvenWhenValuesUnchanged()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    // Write a file where all margins match the model default (0.25).
    QJsonObject obj;
    obj[QStringLiteral("marginTop")]    = 0.25;
    obj[QStringLiteral("marginBottom")] = 0.25;
    obj[QStringLiteral("marginLeft")]   = 0.25;
    obj[QStringLiteral("marginRight")]  = 0.25;
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    SettingsModel m;
    QCOMPARE(m.marginTop(),    0.25);
    QCOMPARE(m.marginBottom(), 0.25);
    QCOMPARE(m.marginLeft(),   0.25);
    QCOMPARE(m.marginRight(),  0.25);

    QSignalSpy spyTop   (&m, &SettingsModel::marginTopChanged);
    QSignalSpy spyBottom(&m, &SettingsModel::marginBottomChanged);
    QSignalSpy spyLeft  (&m, &SettingsModel::marginLeftChanged);
    QSignalSpy spyRight (&m, &SettingsModel::marginRightChanged);

    QVERIFY(m.load(path));

    QVERIFY(spyTop.count()    >= 1);
    QVERIFY(spyBottom.count() >= 1);
    QVERIFY(spyLeft.count()   >= 1);
    QVERIFY(spyRight.count()  >= 1);
} // load_emitsMarginSignalsEvenWhenValuesUnchanged()

// @brief layoutModeChanged() is emitted even when the loaded value matches.
void SettingsModelTests::load_emitsLayoutModeChangedEvenWhenValueUnchanged()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    // Write a file with the default layoutMode.
    QJsonObject obj;
    obj[QStringLiteral("layoutMode")] = QStringLiteral("alongGrainline");
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    SettingsModel m;
    QCOMPARE(m.layoutMode(), QStringLiteral("alongGrainline"));

    QSignalSpy spy(&m, &SettingsModel::layoutModeChanged);
    QVERIFY(m.load(path));
    QVERIFY(spy.count() >= 1);
} // load_emitsLayoutModeChangedEvenWhenValueUnchanged()

// @brief All core property signals are emitted at least once after load().
// Covers every Q_PROPERTY NOTIFY signal that QML dialog controls bind to.
void SettingsModelTests::load_emitsAllPropertySignals()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.json"));

    // Write an empty JSON object — every field will match the model's defaults,
    // so all setter guards suppress emission.  The force-emit block at the end
    // of load() is the sole source for each signal.
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(QJsonObject()).toJson());
    f.close();

    SettingsModel m;

    QSignalSpy spyLayoutMode      (&m, &SettingsModel::layoutModeChanged);
    QSignalSpy spyRotationStep    (&m, &SettingsModel::rotationStepChanged);
    QSignalSpy spyFabricFolded    (&m, &SettingsModel::fabricFoldedChanged);
    QSignalSpy spyPieceGap        (&m, &SettingsModel::pieceGapChanged);
    QSignalSpy spyPieceGapPx      (&m, &SettingsModel::pieceGapPxChanged);
    QSignalSpy spyUnit            (&m, &SettingsModel::unitChanged);
    QSignalSpy spyMediaType       (&m, &SettingsModel::mediaTypeChanged);
    QSignalSpy spyPaperType       (&m, &SettingsModel::paperTypeChanged);
    QSignalSpy spyPaperTypeChoice (&m, &SettingsModel::paperTypeChoiceChanged);
    QSignalSpy spySheetName       (&m, &SettingsModel::sheetNameChanged);
    QSignalSpy spyPageWidth       (&m, &SettingsModel::pageWidthChanged);
    QSignalSpy spyPageHeight      (&m, &SettingsModel::pageHeightChanged);
    QSignalSpy spyPageWidthPx     (&m, &SettingsModel::pageWidthPxChanged);
    QSignalSpy spyPageHeightPx    (&m, &SettingsModel::pageHeightPxChanged);
    QSignalSpy spyRollSize        (&m, &SettingsModel::rollSizeChanged);
    QSignalSpy spyRollWidth       (&m, &SettingsModel::rollWidthChanged);
    QSignalSpy spyRollWidthPx     (&m, &SettingsModel::rollWidthPxChanged);
    QSignalSpy spyRollLength      (&m, &SettingsModel::rollLengthChanged);
    QSignalSpy spyTileSize        (&m, &SettingsModel::tileSizeChanged);
    QSignalSpy spyTileOrientation (&m, &SettingsModel::tileOrientationChanged);
    QSignalSpy spyMarginTop       (&m, &SettingsModel::marginTopChanged);
    QSignalSpy spyMarginBottom    (&m, &SettingsModel::marginBottomChanged);
    QSignalSpy spyMarginLeft      (&m, &SettingsModel::marginLeftChanged);
    QSignalSpy spyMarginRight     (&m, &SettingsModel::marginRightChanged);
    QSignalSpy spyFabricWidth     (&m, &SettingsModel::fabricWidthChanged);
    QSignalSpy spyFabricHeight    (&m, &SettingsModel::fabricHeightChanged);
    QSignalSpy spySelvedgeWidth   (&m, &SettingsModel::selvedgeWidthChanged);
    QSignalSpy spySettingsLoaded  (&m, &SettingsModel::settingsLoaded);

    QVERIFY(m.load(path));

    QVERIFY(spyLayoutMode.count()      >= 1);
    QVERIFY(spyRotationStep.count()    >= 1);
    QVERIFY(spyFabricFolded.count()    >= 1);
    QVERIFY(spyPieceGap.count()        >= 1);
    QVERIFY(spyPieceGapPx.count()      >= 1);
    QVERIFY(spyUnit.count()            >= 1);
    QVERIFY(spyMediaType.count()       >= 1);
    QVERIFY(spyPaperType.count()       >= 1);
    QVERIFY(spyPaperTypeChoice.count() >= 1);
    QVERIFY(spySheetName.count()       >= 1);
    QVERIFY(spyPageWidth.count()       >= 1);
    QVERIFY(spyPageHeight.count()      >= 1);
    QVERIFY(spyPageWidthPx.count()     >= 1);
    QVERIFY(spyPageHeightPx.count()    >= 1);
    QVERIFY(spyRollSize.count()        >= 1);
    QVERIFY(spyRollWidth.count()       >= 1);
    QVERIFY(spyRollWidthPx.count()     >= 1);
    QVERIFY(spyRollLength.count()      >= 1);
    QVERIFY(spyTileSize.count()        >= 1);
    QVERIFY(spyTileOrientation.count() >= 1);
    QVERIFY(spyMarginTop.count()       >= 1);
    QVERIFY(spyMarginBottom.count()    >= 1);
    QVERIFY(spyMarginLeft.count()      >= 1);
    QVERIFY(spyMarginRight.count()     >= 1);
    QVERIFY(spyFabricWidth.count()     >= 1);
    QVERIFY(spyFabricHeight.count()    >= 1);
    QVERIFY(spySelvedgeWidth.count()   >= 1);
    QVERIFY(spySettingsLoaded.count()  == 1);
} // load_emitsAllPropertySignals()

// ---------------------------------------------------------------------------
// BC.2 — Settings round-trip verification
//
// These tests verify the complete pipeline:
//   load file → all fields update → save → saved JSON is correct.
//
// The existing roundtrip_preserves* tests exercise individual fields via
// programmatic setters.  BC.2 tests exercise loading from a JSON file
// (the path a real user takes), verifying that every field is populated
// from the file, that save() writes all 22 expected keys, and that the
// saved JSON values precisely match the model's current state.
// ---------------------------------------------------------------------------

// @brief All model fields are updated when loading a file with non-default values.
// Exercises load() for every field (not just one at a time) so that a missing
// or misspelled JSON key in load() is caught immediately.
void SettingsModelTests::bc2_loadFile_allFieldsUpdate()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("fixture.json"));

    // Fixture uses non-default values for every loadable field so that the test
    // would fail if load() silently kept a default instead of reading the file.
    QJsonObject obj;
    obj[QStringLiteral("layoutMode")]      = QStringLiteral("withNap");
    obj[QStringLiteral("rotationStep")]    = 180.0;
    obj[QStringLiteral("fabricFolded")]    = true;
    obj[QStringLiteral("unit")]            = QStringLiteral("mm");
    obj[QStringLiteral("mediaType")]       = QStringLiteral("paper");
    obj[QStringLiteral("paperType")]       = QStringLiteral("tiled");
    obj[QStringLiteral("sheetName")]       = QStringLiteral("A4");
    obj[QStringLiteral("pageWidth")]       = 210.0;
    obj[QStringLiteral("pageHeight")]      = 297.0;
    obj[QStringLiteral("rollSize")]        = QStringLiteral("1200 mm");
    obj[QStringLiteral("rollWidth")]       = 1200.0;
    obj[QStringLiteral("tileSize")]        = QStringLiteral("A4");
    obj[QStringLiteral("tileOrientation")] = QStringLiteral("portrait");
    obj[QStringLiteral("marginTop")]       = 10.0;
    obj[QStringLiteral("marginBottom")]    = 12.0;
    obj[QStringLiteral("marginLeft")]      = 8.0;
    obj[QStringLiteral("marginRight")]     = 9.0;
    obj[QStringLiteral("fabricWidth")]     = 150.0;
    obj[QStringLiteral("fabricHeight")]    = 500.0;
    obj[QStringLiteral("selvedgeWidth")]   = 0.0;  // 0 keeps margins independent (mediaType=paper)
    obj[QStringLiteral("pieceGap")]        = 5.0;

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    SettingsModel m;
    QVERIFY(m.load(path));

    // Every field must reflect the fixture value, not the model default.
    QCOMPARE(m.layoutMode(),      QStringLiteral("withNap"));
    QCOMPARE(m.rotationStep(),    180.0);
    QCOMPARE(m.fabricFolded(),    true);
    QCOMPARE(m.unit(),            QStringLiteral("mm"));
    QCOMPARE(m.mediaType(),       QStringLiteral("paper"));
    QCOMPARE(m.paperType(),       QStringLiteral("tiled"));
    QCOMPARE(m.sheetName(),       QStringLiteral("A4"));
    QCOMPARE(m.pageWidth(),       210.0);
    QCOMPARE(m.pageHeight(),      297.0);
    QCOMPARE(m.rollSize(),        QStringLiteral("1200 mm"));
    QCOMPARE(m.rollWidth(),       1200.0);
    QCOMPARE(m.tileSize(),        QStringLiteral("A4"));
    QCOMPARE(m.tileOrientation(), QStringLiteral("portrait"));
    QCOMPARE(m.marginTop(),       10.0);
    QCOMPARE(m.marginBottom(),    12.0);
    QCOMPARE(m.marginLeft(),      8.0);
    QCOMPARE(m.marginRight(),     9.0);
    QCOMPARE(m.fabricWidth(),     150.0);
    QCOMPARE(m.fabricHeight(),    500.0);
    QCOMPARE(m.selvedgeWidth(),   0.0);
    QCOMPARE(m.pieceGap(),        5.0);
} // bc2_loadFile_allFieldsUpdate()

// @brief save() writes all 22 expected JSON keys after a file load.
// A missing key in save() would break Rust LayoutSettings deserialization
// and cause silent default-value fallback in the layout engine.
void SettingsModelTests::bc2_saveAfterLoad_savedJsonContainsAllKeys()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString loadPath = dir.filePath(QStringLiteral("input.json"));
    const QString savePath = dir.filePath(QStringLiteral("output.json"));

    // Minimal fixture — only a few fields; the rest keep defaults.
    QJsonObject fixture;
    fixture[QStringLiteral("unit")]       = QStringLiteral("cm");
    fixture[QStringLiteral("layoutMode")] = QStringLiteral("withNap");
    QFile f(loadPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(fixture).toJson());
    f.close();

    SettingsModel m;
    QVERIFY(m.load(loadPath));
    QVERIFY(m.save(savePath));

    // Parse the saved file.
    QFile saved(savePath);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(saved.readAll(), &err);
    QCOMPARE(err.error, QJsonParseError::NoError);
    const QJsonObject savedObj = doc.object();

    // All 23 keys that save() is contractually required to write.
    const QStringList expectedKeys = {
        QStringLiteral("layoutMode"),    QStringLiteral("rotationStep"),
        QStringLiteral("noGrainlineRotation"),
        QStringLiteral("fabricFolded"),  QStringLiteral("unit"),
        QStringLiteral("mediaType"),     QStringLiteral("paperType"),
        QStringLiteral("sheetName"),     QStringLiteral("pageWidth"),
        QStringLiteral("pageHeight"),    QStringLiteral("rollSize"),
        QStringLiteral("rollWidth"),     QStringLiteral("tileSize"),
        QStringLiteral("tileOrientation"), QStringLiteral("marginTop"),
        QStringLiteral("marginBottom"),  QStringLiteral("marginLeft"),
        QStringLiteral("marginRight"),   QStringLiteral("fabricWidth"),
        QStringLiteral("fabricHeight"),  QStringLiteral("selvedgeWidth"),
        QStringLiteral("pieceGap"),      QStringLiteral("outputFormat"),
    };

    for (const QString &key : expectedKeys) {
        QVERIFY2(savedObj.contains(key),
                 qPrintable(QStringLiteral("Missing key in saved JSON: ") + key));
    } // for key
} // bc2_saveAfterLoad_savedJsonContainsAllKeys()

// @brief Every JSON value in the saved file matches the model's current property.
// Catches precision loss, type coercion errors, or a field that is serialised
// with the wrong key (e.g., saving m_rollWidth under "rollSize").
void SettingsModelTests::bc2_savedJson_fieldValuesMatchModel()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString loadPath = dir.filePath(QStringLiteral("fixture.json"));
    const QString savePath = dir.filePath(QStringLiteral("saved.json"));

    // Complete fixture with non-default values for all fields.
    QJsonObject fixture;
    fixture[QStringLiteral("layoutMode")]      = QStringLiteral("withNap");
    fixture[QStringLiteral("rotationStep")]    = 180.0;
    fixture[QStringLiteral("fabricFolded")]    = true;
    fixture[QStringLiteral("unit")]            = QStringLiteral("mm");
    fixture[QStringLiteral("mediaType")]       = QStringLiteral("paper");
    fixture[QStringLiteral("paperType")]       = QStringLiteral("sheet");
    fixture[QStringLiteral("sheetName")]       = QStringLiteral("A3");
    fixture[QStringLiteral("pageWidth")]       = 297.0;
    fixture[QStringLiteral("pageHeight")]      = 420.0;
    fixture[QStringLiteral("rollSize")]        = QStringLiteral("900 mm");
    fixture[QStringLiteral("rollWidth")]       = 900.0;
    fixture[QStringLiteral("tileSize")]        = QStringLiteral("A4");
    fixture[QStringLiteral("tileOrientation")] = QStringLiteral("portrait");
    fixture[QStringLiteral("marginTop")]       = 10.0;
    fixture[QStringLiteral("marginBottom")]    = 10.0;
    fixture[QStringLiteral("marginLeft")]      = 10.0;
    fixture[QStringLiteral("marginRight")]     = 10.0;
    fixture[QStringLiteral("fabricWidth")]     = 0.0;
    fixture[QStringLiteral("fabricHeight")]    = 0.0;
    fixture[QStringLiteral("selvedgeWidth")]   = 0.0;
    fixture[QStringLiteral("pieceGap")]        = 3.0;

    QFile f(loadPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(fixture).toJson());
    f.close();

    SettingsModel m;
    QVERIFY(m.load(loadPath));
    QVERIFY(m.save(savePath));

    // Parse the saved file.
    QFile saved(savePath);
    QVERIFY(saved.open(QIODevice::ReadOnly));
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(saved.readAll(), &err);
    QCOMPARE(err.error, QJsonParseError::NoError);
    const QJsonObject obj = doc.object();

    // Assert key presence first to avoid false positives for default-ish values (0/false/"").
    const QStringList expectedKeys = {
        QStringLiteral("layoutMode"),    QStringLiteral("rotationStep"),
        QStringLiteral("fabricFolded"),  QStringLiteral("unit"),
        QStringLiteral("mediaType"),     QStringLiteral("paperType"),
        QStringLiteral("sheetName"),     QStringLiteral("pageWidth"),
        QStringLiteral("pageHeight"),    QStringLiteral("rollSize"),
        QStringLiteral("rollWidth"),     QStringLiteral("tileSize"),
        QStringLiteral("tileOrientation"), QStringLiteral("marginTop"),
        QStringLiteral("marginBottom"),  QStringLiteral("marginLeft"),
        QStringLiteral("marginRight"),   QStringLiteral("fabricWidth"),
        QStringLiteral("fabricHeight"),  QStringLiteral("selvedgeWidth"),
        QStringLiteral("pieceGap"),      QStringLiteral("outputFormat"),
    };
    for (const QString &key : expectedKeys) {
        QVERIFY2(obj.contains(key),
                 qPrintable(QStringLiteral("Missing key in saved JSON: ") + key));
    }
    // Each JSON value must exactly match the corresponding model getter.
    QCOMPARE(obj[QStringLiteral("layoutMode")].toString(),       m.layoutMode());
    QCOMPARE(obj[QStringLiteral("rotationStep")].toDouble(),     m.rotationStep());
    QCOMPARE(obj[QStringLiteral("fabricFolded")].toBool(),       m.fabricFolded());
    QCOMPARE(obj[QStringLiteral("unit")].toString(),             m.unit());
    QCOMPARE(obj[QStringLiteral("mediaType")].toString(),        m.mediaType());
    QCOMPARE(obj[QStringLiteral("paperType")].toString(),        m.paperType());
    QCOMPARE(obj[QStringLiteral("sheetName")].toString(),        m.sheetName());
    QCOMPARE(obj[QStringLiteral("pageWidth")].toDouble(),        m.pageWidth());
    QCOMPARE(obj[QStringLiteral("pageHeight")].toDouble(),       m.pageHeight());
    QCOMPARE(obj[QStringLiteral("rollSize")].toString(),         m.rollSize());
    QCOMPARE(obj[QStringLiteral("rollWidth")].toDouble(),        m.rollWidth());
    QCOMPARE(obj[QStringLiteral("tileSize")].toString(),         m.tileSize());
    QCOMPARE(obj[QStringLiteral("tileOrientation")].toString(),  m.tileOrientation());
    QCOMPARE(obj[QStringLiteral("marginTop")].toDouble(),        m.marginTop());
    QCOMPARE(obj[QStringLiteral("marginBottom")].toDouble(),     m.marginBottom());
    QCOMPARE(obj[QStringLiteral("marginLeft")].toDouble(),       m.marginLeft());
    QCOMPARE(obj[QStringLiteral("marginRight")].toDouble(),      m.marginRight());
    QCOMPARE(obj[QStringLiteral("fabricWidth")].toDouble(),      m.fabricWidth());
    QCOMPARE(obj[QStringLiteral("fabricHeight")].toDouble(),     m.fabricHeight());
    QCOMPARE(obj[QStringLiteral("selvedgeWidth")].toDouble(),    m.selvedgeWidth());
    QCOMPARE(obj[QStringLiteral("pieceGap")].toDouble(),         m.pieceGap());
    QCOMPARE(obj[QStringLiteral("outputFormat")].toString(),     QStringLiteral("svg"));
} // bc2_savedJson_fieldValuesMatchModel()

// @brief Full pipeline: load file → edit fields via setters → save → reload → all fields match.
// Simulates the actual user workflow: open a settings file, change values in
// the dialog (setters), press Save, then re-open the dialog (fresh model load).
// Every setter-modified field must survive the save/load round-trip.
void SettingsModelTests::bc2_fullPipeline_loadEditSaveReload_fieldsMatch()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString originalPath = dir.filePath(QStringLiteral("original.json"));
    const QString editedPath   = dir.filePath(QStringLiteral("edited.json"));

    // Phase 1 — create initial settings file with known values.
    QJsonObject initial;
    initial[QStringLiteral("layoutMode")] = QStringLiteral("alongGrainline");
    initial[QStringLiteral("unit")]       = QStringLiteral("in");
    initial[QStringLiteral("mediaType")]  = QStringLiteral("paper");
    initial[QStringLiteral("paperType")]  = QStringLiteral("sheet");
    initial[QStringLiteral("marginTop")]  = 0.5;
    initial[QStringLiteral("marginBottom")] = 0.5;
    initial[QStringLiteral("marginLeft")] = 0.5;
    initial[QStringLiteral("marginRight")] = 0.5;
    initial[QStringLiteral("pieceGap")]   = 0.05;
    QFile f(originalPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(initial).toJson());
    f.close();

    // Phase 2 — load the file and verify the initial field state.
    SettingsModel writer;
    QVERIFY(writer.load(originalPath));
    QCOMPARE(writer.layoutMode(), QStringLiteral("alongGrainline"));
    QCOMPARE(writer.unit(),       QStringLiteral("in"));
    QCOMPARE(writer.marginTop(),  0.5);

    // Phase 3 — simulate user edits via setters (models dialog interaction).
    writer.setUnit(QStringLiteral("cm"));
    writer.setLayoutMode(QStringLiteral("withNap"));
    writer.setRotationStep(180.0);
    writer.setMarginTop(1.5);
    writer.setMarginBottom(1.5);
    writer.setMarginLeft(1.0);
    writer.setMarginRight(1.0);
    writer.setTileOrientation(QStringLiteral("portrait"));
    writer.setFabricFolded(true);
    writer.setPieceGap(2.0);

    // Phase 4 — save the edited model.
    QVERIFY(writer.save(editedPath));

    // Phase 5 — reload from the saved file into a fresh model; every edited
    // field must reflect the setter value, not the original file value.
    SettingsModel reader;
    QVERIFY(reader.load(editedPath));
    QCOMPARE(reader.unit(),           QStringLiteral("cm"));
    QCOMPARE(reader.layoutMode(),     QStringLiteral("withNap"));
    QCOMPARE(reader.rotationStep(),   180.0);
    QCOMPARE(reader.marginTop(),      1.5);
    QCOMPARE(reader.marginBottom(),   1.5);
    QCOMPARE(reader.marginLeft(),     1.0);
    QCOMPARE(reader.marginRight(),    1.0);
    QCOMPARE(reader.tileOrientation(), QStringLiteral("portrait"));
    QCOMPARE(reader.fabricFolded(),   true);
    QCOMPARE(reader.pieceGap(),       2.0);
} // bc2_fullPipeline_loadEditSaveReload_fieldsMatch()

QTEST_MAIN(SettingsModelTests)
#include "SettingsModelTests.moc"
