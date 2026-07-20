// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file SettingsModel.cpp
// @brief Implementation of SettingsModel — layout settings QObject.
//
// Paper/tile size tables mirror the Rust compile-time constants in
// Matches the Rust settings data tables (PAPER_SIZES_DATA, TILE_SIZES_DATA).
// JSON keys match the Rust #[serde(rename_all = "camelCase")] annotation.

#include "SettingsModel.h"
#include "Logger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantList>
#include <cmath>

// ---------------------------------------------------------------------------
// Static lookup tables (mirrored from Rust PAPER_SIZES_DATA / TILE_SIZES_DATA)
// ---------------------------------------------------------------------------

namespace {

struct SizeEntry {
    const char *name;
    double  widthIn;
    double  heightIn;
    double  widthMm;
    double  heightMm;
}; // SizeEntry

// @brief Paper sheet sizes — names, imperial, and metric dimensions.
static const SizeEntry PAPER_SIZES[] = {
    { "Letter",  8.5,  11.0,  216.0,  279.0 },
    { "Legal",   8.5,  14.0,  216.0,  356.0 },
    { "Ledger",  11.0, 17.0,  279.0,  432.0 },
    { "ANSI C",  17.0, 22.0,  432.0,  559.0 },
    { "ANSI D",  22.0, 34.0,  559.0,  864.0 },
    { "ANSI E",  34.0, 44.0,  864.0, 1118.0 },
    { "ARCH A",   9.0, 12.0,  229.0,  305.0 },
    { "ARCH B",  12.0, 18.0,  305.0,  457.0 },
    { "ARCH C",  18.0, 24.0,  457.0,  610.0 },
    { "ARCH D",  24.0, 36.0,  610.0,  914.0 },
    { "ARCH E",  36.0, 48.0,  914.0, 1219.0 },
    { "ARCH E1", 30.0, 42.0,  762.0, 1067.0 },
    { "ARCH E2", 26.0, 38.0,  660.0,  965.0 },
    { "ARCH E3", 27.0, 39.0,  686.0,  991.0 },
    { "A0",      33.11, 46.81, 841.0, 1189.0 },
    { "A1",      23.39, 33.11, 594.0,  841.0 },
    { "A2",      16.54, 23.39, 420.0,  594.0 },
    { "A3",      11.69, 16.54, 297.0,  420.0 },
    { "A4",       8.27, 11.69, 210.0,  297.0 },
    { "A5",       5.83,  8.27, 148.0,  210.0 },
    { "B0",      39.37, 55.67, 1000.0, 1414.0 },
    { "B1",      27.83, 39.37, 707.0, 1000.0 },
    { "B2",      19.68, 27.83, 500.0,  707.0 },
    { "B3",      13.90, 19.68, 353.0,  500.0 },
    { "B4",       9.84, 13.90, 250.0,  353.0 },
    { "B5",       6.93,  9.84, 176.0,  250.0 },
    { "B6",       4.92,  6.93, 125.0,  176.0 },
}; // PAPER_SIZES

static const int PAPER_SIZES_COUNT =
    static_cast<int>(sizeof(PAPER_SIZES) / sizeof(PAPER_SIZES[0]));

// @brief Tile page sizes for tiled-PDF output.
static const SizeEntry TILE_SIZES[] = {
    { "None",   0.0,  0.0,  0.0,  0.0 },
    { "Letter", 8.5, 11.0, 216.0, 279.0 },
    { "Legal",  8.5, 14.0, 216.0, 356.0 },
    { "Ledger", 11.0, 17.0, 279.0, 432.0 },
    { "A3",     11.69, 16.54, 297.0, 420.0 },
    { "A4",     8.27, 11.69, 210.0, 297.0 },
    { "A5",     5.83,  8.27, 148.0, 210.0 },
}; // TILE_SIZES

static const int TILE_SIZES_COUNT =
    static_cast<int>(sizeof(TILE_SIZES) / sizeof(TILE_SIZES[0]));

// @brief Copy a file if the destination does not already exist.
// Task 15 helper — mirrors PreferencesModel.cpp's copyIfMissing() (kept local to this file
// rather than shared, matching this codebase's existing per-file helper convention).
bool copyIfMissing(const QString &sourcePath, const QString &destPath)
{
    if (QFileInfo::exists(destPath) || !QFileInfo::exists(sourcePath)) {
        return false;
    } // if no copy needed/possible

    QDir destDir = QFileInfo(destPath).absoluteDir();
    if (!destDir.exists()) {
        destDir.mkpath(QStringLiteral("."));
    } // if dest dir missing

    return QFile::copy(sourcePath, destPath);
} // copyIfMissing

// @brief Recursively copy every entry from a legacy organization directory tree into the
// new one, skipping anything the destination already has.
//
// Task 15: seamlyLayout's organization name changed from "Seamly Systems" to the shared
// "Seamly" (see main.cpp), so QStandardPaths::AppConfigLocation resolves to a brand new,
// empty directory. This bridges the settings file(s) forward from the old organization
// folder the first time the new one is resolved. Safe to call unconditionally — once
// everything has been copied across it is a cheap no-op.
void migrateLegacyOrganizationTree(const QString &legacyRoot, const QString &newRoot)
{
    const QDir legacyDir(legacyRoot);
    if (!legacyDir.exists()) {
        return;
    } // if nothing to migrate

    const QFileInfoList entries = legacyDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        const QString destPath = QDir(newRoot).filePath(entry.fileName());
        if (entry.isDir()) {
            QDir().mkpath(destPath);
            migrateLegacyOrganizationTree(entry.absoluteFilePath(), destPath);
        } else {
            copyIfMissing(entry.absoluteFilePath(), destPath);
        } // if directory vs file
    } // for each legacy entry
} // migrateLegacyOrganizationTree

// @brief Bridge appConfigRoot forward from the pre-Task-15 "Seamly Systems" organization
// folder into the new shared "Seamly" one, the first time appConfigRoot is resolved.
void migrateLegacyOrganization(const QString &newRoot)
{
    const QString currentOrganization = QCoreApplication::organizationName();
    static const QString kLegacyOrganizationName = QStringLiteral("Seamly Systems");
    if (currentOrganization == kLegacyOrganizationName) {
        return; // already running under the legacy name — nothing to bridge
    } // if already legacy

    QCoreApplication::setOrganizationName(kLegacyOrganizationName);
    const QString legacyRoot = QDir(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).absolutePath();
    QCoreApplication::setOrganizationName(currentOrganization);

    if (legacyRoot != newRoot) {
        migrateLegacyOrganizationTree(legacyRoot, newRoot);
    } // if legacy root resolves to a different directory
} // migrateLegacyOrganization

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

// @brief Construct SettingsModel with application-default values.
SettingsModel::SettingsModel(QObject *parent)
    : QObject(parent)
{
    // All fields initialised via in-class member initialisers in the header.
} // SettingsModel()

// ---------------------------------------------------------------------------
// ComboBox source lists
// ---------------------------------------------------------------------------

// @brief Return all paper size names in the order they appear in the table.
QStringList SettingsModel::paperSizeNames() const
{
    QStringList names;
    names.reserve(PAPER_SIZES_COUNT);
    for (int i = 0; i < PAPER_SIZES_COUNT; ++i) {
        names.append(QString::fromUtf8(PAPER_SIZES[i].name));
    } // for i
    return names;
} // paperSizeNames()

// @brief Return all tile size names in the order they appear in the table.
QStringList SettingsModel::tileSizeNames() const
{
    QStringList names;
    names.reserve(TILE_SIZES_COUNT);
    for (int i = 0; i < TILE_SIZES_COUNT; ++i) {
        names.append(QString::fromUtf8(TILE_SIZES[i].name));
    } // for i
    return names;
} // tileSizeNames()

QStringList SettingsModel::tileOrientationNames() const
{
    return { QStringLiteral("landscape"), QStringLiteral("portrait") };
} // tileOrientationNames()

// @brief Return unit names for the unit ComboBox.
QStringList SettingsModel::unitNames() const
{
    return { QStringLiteral("in"), QStringLiteral("mm"), QStringLiteral("cm") };
} // unitNames()

// @brief Return piece-arrangement mode names for the layout-mode radio group.
// Order matches the radio button order in SettingsDialog.qml.
QStringList SettingsModel::layoutModeNames() const
{
    return {
        QStringLiteral("alongGrainline"),
        QStringLiteral("withNap"),
    };
} // layoutModeNames()

// @brief Return mode-specific rotation values (in degrees) for QML radios.
// withNap direction values: 0° (up), 180° (down).
QVariantList SettingsModel::rotationStepValues() const
{
    return { 0.0, 180.0 };
} // rotationStepValues()

// @brief Convert a value in the active unit to pixels at 96 px/in.
//
// @param value Value in active unit (in / cm / mm).
// @param unit  Unit string: "in" | "cm" | "mm".
// @return Pixel equivalent, rounded to nearest integer.
static int toPixels(double value, const QString &unit)
{
    constexpr double PPI = 96.0;
    if (unit == QStringLiteral("mm"))
        return static_cast<int>(std::round(value / 25.4 * PPI));  // mm → in → px
    if (unit == QStringLiteral("cm"))
        return static_cast<int>(std::round(value / 2.54  * PPI)); // cm → in → px
    return static_cast<int>(std::round(value * PPI));              // "in" or unknown
} // toPixels()

// @brief Sheet/page width in pixels at 96 px/in.
int SettingsModel::pageWidthPx() const
{
    return toPixels(m_pageWidth, m_unit);
} // pageWidthPx()

// @brief Sheet/page height in pixels at 96 px/in.
int SettingsModel::pageHeightPx() const
{
    return toPixels(m_pageHeight, m_unit);
} // pageHeightPx()

// @brief Roll width in pixels at 96 px/in.
int SettingsModel::rollWidthPx() const
{
    return toPixels(m_rollWidth, m_unit);
} // rollWidthPx()

// @brief Piece-gap clearance in pixels at 96 px/in.
int SettingsModel::pieceGapPx() const
{
    return toPixels(m_pieceGap, m_unit);
} // pieceGapPx()

// @brief Return the default full-roll length in the active unit system.
//
// The sentinel used for packing is always 500 in = 48000 px.  This method
// converts that constant into the unit the user has selected for display.
//   "in" → 500.0 in
//   "cm" → 1270.0 cm  (500 × 2.54)
//   "mm" → 12700.0 mm (500 × 25.4)
double SettingsModel::rollLength() const
{
    if (m_unit == QStringLiteral("cm"))
        return 1270.0;     // 500 in × 2.54 cm/in
    if (m_unit == QStringLiteral("mm"))
        return 12700.0;    // 500 in × 25.4 mm/in
    return 500.0;          // "in" or unknown — return inches
} // rollLength()

// @brief Return roll size names for the roll size ComboBox.
QStringList SettingsModel::rollSizeNames() const
{
    return {
        QStringLiteral("36 in"),
        QStringLiteral("48 in"),
        QStringLiteral("60 in"),
        QStringLiteral("72 in"),
        QStringLiteral("900 mm"),
        QStringLiteral("1200 mm"),
        QStringLiteral("1500 mm"),
    };
} // rollSizeNames()

// ---------------------------------------------------------------------------
// Setters — emit changed signal only when value actually changes
// ---------------------------------------------------------------------------

// @brief Snap a raw rotationStep value into the set valid for the given mode.
//   "withNap" → { 0, 180 }            (0 = head-up, 180 = head-down)
//   other     → value unchanged       (rotationStep is unused for "alongGrainline")
// Always returns the closest allowed value by absolute difference.
static double snapRotationStepForMode(double raw, const QString &mode)
{
    static const double napAllowed[]    = { 0.0, 180.0 };
    const double *allowed = nullptr;
    int count = 0;
    if (mode == QStringLiteral("withNap")) {
        allowed = napAllowed;
        count = static_cast<int>(sizeof(napAllowed) / sizeof(napAllowed[0]));
    } else {
        return raw; // alongGrainline (or unknown): leave value alone
    } // if mode

    double best = allowed[0];
    double bestDiff = std::abs(raw - allowed[0]);
    for (int i = 1; i < count; ++i) {
        const double d = std::abs(raw - allowed[i]);
        if (d < bestDiff) { best = allowed[i]; bestDiff = d; }
    } // for i
    return best;
} // snapRotationStepForMode()

// @brief Set the piece-arrangement mode.
// Unsupported values (including legacy "rotate") are coerced to
// "alongGrainline" to keep behavior stable and deterministic.
void SettingsModel::setLayoutMode(const QString &v)
{
    QString normalized = v;
    if (normalized != QStringLiteral("alongGrainline")
        && normalized != QStringLiteral("withNap")) {
        normalized = QStringLiteral("alongGrainline");
    } // if unsupported layout mode

    if (m_layoutMode == normalized) return;
    m_layoutMode = normalized;
    // Coerce rotationStep when entering a mode that constrains it.
    if (normalized == QStringLiteral("withNap")) {
        const double snapped = snapRotationStepForMode(m_rotationStep, normalized);
        if (!qFuzzyCompare(snapped, m_rotationStep)) {
            m_rotationStep = snapped;
            emit rotationStepChanged();
        } // if snapped differs
    } // if mode constrains rotationStep
    emit layoutModeChanged();
} // setLayoutMode()

// @brief Set the rotation step (degrees).  Semantics depend on layoutMode:
//   layoutMode == "withNap" → fixed offset in { 0, 180 } (head-up vs head-down)
//   layoutMode == "alongGrainline" → unused (trial set is fixed at {0, 180})
// Does not validate the value — QML radio binding enforces the per-mode set.
void SettingsModel::setRotationStep(double v)
{
    if (qFuzzyCompare(m_rotationStep, v)) return;
    m_rotationStep = v;
    emit rotationStepChanged();
} // setRotationStep()

void SettingsModel::setFabricFolded(bool v)
{
    if (m_fabricFolded == v) return;
    m_fabricFolded = v;
    emit fabricFoldedChanged();
} // setFabricFolded()

void SettingsModel::setPieceGap(double v)
{
    if (qFuzzyCompare(m_pieceGap, v)) return;
    m_pieceGap = v;
    emit pieceGapChanged();
    // Pixel projection depends on both pieceGap and unit, so emit alongside
    // the user-unit signal so QML bindings on either property stay in sync.
    emit pieceGapPxChanged();
} // setPieceGap()

void SettingsModel::setUnit(const QString &v)
{
    if (m_unit == v) return;
    m_unit = v;
    emit unitChanged();
    emit rollLengthChanged();    // rollLength() is derived from unit
    emit pageWidthPxChanged();   // px values are derived from unit
    emit pageHeightPxChanged();
    emit rollWidthPxChanged();
    emit pieceGapPxChanged();
} // setUnit()

void SettingsModel::setMediaType(const QString &v)
{
    if (m_mediaType == v) return;
    m_mediaType = v;
    if (m_mediaType == QStringLiteral("fabric"))
        syncFabricMarginsFromSelvedge();
    emit mediaTypeChanged();
} // setMediaType()

void SettingsModel::setPaperType(const QString &v)
{
    if (m_paperType == v) return;
    m_paperType = v;
    emit paperTypeChanged();
} // setPaperType()

void SettingsModel::setSheetName(const QString &v)
{
    if (m_sheetName == v) return;
    m_sheetName = v;
    emit sheetNameChanged();
} // setSheetName()

void SettingsModel::setPageWidth(double v)
{
    if (qFuzzyCompare(m_pageWidth, v)) return;
    m_pageWidth = v;
    emit pageWidthChanged();
    emit pageWidthPxChanged();   // pageWidthPx is derived from pageWidth
} // setPageWidth()

void SettingsModel::setPageHeight(double v)
{
    if (qFuzzyCompare(m_pageHeight, v)) return;
    m_pageHeight = v;
    emit pageHeightChanged();
    emit pageHeightPxChanged();  // pageHeightPx is derived from pageHeight
} // setPageHeight()

void SettingsModel::setRollSize(const QString &v)
{
    if (m_rollSize == v) return;
    m_rollSize = v;
    emit rollSizeChanged();

    // Parse the numeric value and unit from the descriptor (e.g. "48 in", "900 mm")
    // and update rollWidth in the active unit so Page Size stays in sync.
    const QStringList parts = v.split(QChar(u' '), Qt::SkipEmptyParts);
    if (parts.size() == 2) {
        bool ok = false;
        const double numericValue = parts[0].toDouble(&ok);
        if (ok)
            setRollWidth(convertUnit(numericValue, parts[1], m_unit));
    } // if parts.size() == 2
} // setRollSize()

void SettingsModel::setRollWidth(double v)
{
    if (qFuzzyCompare(m_rollWidth, v)) return;
    m_rollWidth = v;
    emit rollWidthChanged();
    emit rollWidthPxChanged();   // rollWidthPx is derived from rollWidth
} // setRollWidth()

void SettingsModel::setTileSize(const QString &v)
{
    if (m_tileSize == v) return;
    m_tileSize = v;
    emit tileSizeChanged();
} // setTileSize()

void SettingsModel::setTileOrientation(const QString &v)
{
    if (m_tileOrientation == v) return;
    m_tileOrientation = v;
    emit tileOrientationChanged();
} // setTileOrientation()

void SettingsModel::setMarginTop(double v)
{
    if (qFuzzyCompare(m_marginTop, v)) return;
    m_marginTop = v;
    emit marginTopChanged();
} // setMarginTop()

void SettingsModel::setMarginBottom(double v)
{
    if (qFuzzyCompare(m_marginBottom, v)) return;
    m_marginBottom = v;
    emit marginBottomChanged();
} // setMarginBottom()

void SettingsModel::setMarginLeft(double v)
{
    if (qFuzzyCompare(m_marginLeft, v)) return;
    m_marginLeft = v;
    emit marginLeftChanged();
} // setMarginLeft()

void SettingsModel::setMarginRight(double v)
{
    if (qFuzzyCompare(m_marginRight, v)) return;
    m_marginRight = v;
    emit marginRightChanged();
} // setMarginRight()

void SettingsModel::setFabricWidth(double v)
{
    if (qFuzzyCompare(m_fabricWidth, v)) return;
    m_fabricWidth = v;
    emit fabricWidthChanged();
} // setFabricWidth()

void SettingsModel::setFabricHeight(double v)
{
    if (qFuzzyCompare(m_fabricHeight, v)) return;
    m_fabricHeight = v;
    emit fabricHeightChanged();
} // setFabricHeight()

void SettingsModel::setSelvedgeWidth(double v)
{
    if (qFuzzyCompare(m_selvedgeWidth, v)) return;
    m_selvedgeWidth = v;
    if (m_mediaType == QStringLiteral("fabric"))
        syncFabricMarginsFromSelvedge();
    emit selvedgeWidthChanged();
} // setSelvedgeWidth()

// ---------------------------------------------------------------------------
// Invokable methods
// ---------------------------------------------------------------------------

// @brief Convert a file:// URL string to a local file system path.
// Uses QUrl::toLocalFile() for correct cross-platform handling:
//   "file:///C:/Users/..."  →  "C:/Users/..."  (Windows)
//   "file:///home/user/..." →  "/home/user/..."  (Linux/macOS)
QString SettingsModel::urlToLocalFile(const QString &url)
{
    return QUrl(url).toLocalFile();
} // urlToLocalFile()

// @brief Convert a local file system path to a file:// URL string.
// Uses QUrl::fromLocalFile() for correct cross-platform handling:
//   "C:/Users/..."   →  "file:///C:/Users/..."  (Windows)
//   "/home/user/..." →  "file:///home/user/..."  (Linux/macOS)
QString SettingsModel::localFileToUrl(const QString &path)
{
    if (path.isEmpty()) return QStringLiteral("");
    return QUrl::fromLocalFile(path).toString();
} // localFileToUrl()

// @brief Return the absolute default settings file path in AppConfigLocation.
//
// Task 15: also bridges appConfigRoot forward from the pre-unification "Seamly Systems"
// organization folder into the new shared "Seamly" one on first use (mirrors
// PreferencesModel::appConfigRootPath() — this file cannot assume that one already ran,
// since QML may resolve either model first).
QString SettingsModel::defaultSettingsFilePath()
{
    QString appConfigRoot = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (appConfigRoot.isEmpty()) {
        appConfigRoot = QCoreApplication::applicationDirPath();
    } // if AppConfigLocation unavailable
    appConfigRoot = QDir(appConfigRoot).absolutePath();
    QDir().mkpath(appConfigRoot);
    migrateLegacyOrganization(appConfigRoot);

    const QString settingsDir = QDir(appConfigRoot).filePath(QStringLiteral("settings"));
    QDir dir(settingsDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    } // if missing

    return QFileInfo(dir.filePath(QStringLiteral("default_settings.json"))).absoluteFilePath();
} // defaultSettingsFilePath

// @brief Return the file:// URL of the default settings folder.
QString SettingsModel::settingsFolderUrl() const
{
    const QString dir = QFileInfo(defaultSettingsFilePath()).absoluteDir().absolutePath();
    return QUrl::fromLocalFile(dir).toString();
} // settingsFolderUrl()

// @brief Load settings from JSON.  Missing keys keep their current (default) values.
bool SettingsModel::load(const QString &path)
{
    Logger::log(QStringLiteral("==========LOAD SETTINGS=========="));
    Logger::log(QStringLiteral("SettingsModel::load(): path=") + path);

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        // File absent — keep defaults (not an error on first run)
        Logger::log(QStringLiteral("SettingsModel::load(): file not found, keeping defaults"));
        return true;
    } // if !open

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        return false;
    } // if parse error

    const QJsonObject obj = doc.object();

    // -----------------------------------------------------------------
    // Layout mode + rotation step (with legacy migration)
    //
    // Pre-2026-05 schema:
    //   layoutMode      ∈ { "withGrain", "withoutGrain" }
    //   rotationEnabled : bool
    //   rotationStep    : double  (any value)
    //
    // New schema:
    //   layoutMode      ∈ { "alongGrainline", "withNap" }
    //   rotationStep    : double — semantics depend on layoutMode:
    //                       "withNap" → fixed offset ∈ { 0, 180 }
    //                       "alongGrainline" → unused
    //
    // Migration mapping:
    //   withGrain    + rotationEnabled=false → withNap        (head-up: rotationStep=0)
    //   withGrain    + rotationEnabled=true  → alongGrainline
    //   withGrain    + (no rotationEnabled)  → alongGrainline (was the implicit default)
    //   withoutGrain + (any)                 → alongGrainline (rotate removed)
    // -----------------------------------------------------------------
    if (obj.contains(QStringLiteral("layoutMode"))) {
        const QString rawMode = obj[QStringLiteral("layoutMode")].toString();
        if (rawMode == QStringLiteral("withGrain")) {
            // Legacy: choose between alongGrainline and withNap based on the
            // old rotationEnabled flag.
            const bool legacyRotationEnabled =
                obj.contains(QStringLiteral("rotationEnabled"))
                    ? obj[QStringLiteral("rotationEnabled")].toBool()
                    : true; // default in the old schema
            if (legacyRotationEnabled) {
                setLayoutMode(QStringLiteral("alongGrainline"));
            } else {
                // Default withNap migration to head-up (rotationStep=0).
                setRotationStep(0.0);
                setLayoutMode(QStringLiteral("withNap"));
            } // if legacyRotationEnabled
        } else if (rawMode == QStringLiteral("withoutGrain")) {
            // Legacy mode had free rotation; Rotate has been removed, so map
            // to alongGrainline.
            setLayoutMode(QStringLiteral("alongGrainline"));
        } else {
            // Modern value or unknown — pass through; QML radio group will
            // ignore unrecognised values until the user picks one.
            setLayoutMode(rawMode);
        } // if legacy mode
    } // if layoutMode present

    // Rotation step: snap to the set valid for the resolved layoutMode.
    // setLayoutMode() above has already coerced m_rotationStep into a valid
    // value when switching modes, but the JSON's rotationStep may carry a more
    // specific user choice (e.g. step=180 for withNap)
    // that we should restore here.
    if (obj.contains(QStringLiteral("rotationStep"))) {
        const double raw = obj[QStringLiteral("rotationStep")].toDouble(m_rotationStep);
        setRotationStep(snapRotationStepForMode(raw, m_layoutMode));
    } // if rotationStep present

    if (obj.contains(QStringLiteral("fabricFolded")))
        setFabricFolded(obj[QStringLiteral("fabricFolded")].toBool());

    if (obj.contains(QStringLiteral("unit")))
        setUnit(obj[QStringLiteral("unit")].toString());

    if (obj.contains(QStringLiteral("mediaType")))
        setMediaType(obj[QStringLiteral("mediaType")].toString());

    if (obj.contains(QStringLiteral("paperType")))
        setPaperType(obj[QStringLiteral("paperType")].toString());

    if (obj.contains(QStringLiteral("sheetName")))
        setSheetName(obj[QStringLiteral("sheetName")].toString());

    if (obj.contains(QStringLiteral("pageWidth")))
        setPageWidth(obj[QStringLiteral("pageWidth")].toDouble(m_pageWidth));

    if (obj.contains(QStringLiteral("pageHeight")))
        setPageHeight(obj[QStringLiteral("pageHeight")].toDouble(m_pageHeight));

    // rollWidth must be loaded before rollSize so that setRollSize() — which parses
    // the descriptor string and calls setRollWidth() — always has the final say.
    // This corrects any stale rollWidth value that may have been saved to JSON when
    // the two fields were not kept in sync.
    if (obj.contains(QStringLiteral("rollWidth")))
        setRollWidth(obj[QStringLiteral("rollWidth")].toDouble(m_rollWidth));

    if (obj.contains(QStringLiteral("rollSize")))
        setRollSize(obj[QStringLiteral("rollSize")].toString());

    if (obj.contains(QStringLiteral("tileSize")))
        setTileSize(obj[QStringLiteral("tileSize")].toString());

    if (obj.contains(QStringLiteral("tileOrientation")))
        setTileOrientation(obj[QStringLiteral("tileOrientation")].toString());

    if (obj.contains(QStringLiteral("marginTop")))
        setMarginTop(obj[QStringLiteral("marginTop")].toDouble(m_marginTop));

    if (obj.contains(QStringLiteral("marginBottom")))
        setMarginBottom(obj[QStringLiteral("marginBottom")].toDouble(m_marginBottom));

    if (obj.contains(QStringLiteral("marginLeft")))
        setMarginLeft(obj[QStringLiteral("marginLeft")].toDouble(m_marginLeft));

    if (obj.contains(QStringLiteral("marginRight")))
        setMarginRight(obj[QStringLiteral("marginRight")].toDouble(m_marginRight));

    if (obj.contains(QStringLiteral("fabricWidth")))
        setFabricWidth(obj[QStringLiteral("fabricWidth")].toDouble(m_fabricWidth));

    if (obj.contains(QStringLiteral("fabricHeight")))
        setFabricHeight(obj[QStringLiteral("fabricHeight")].toDouble(m_fabricHeight));

    if (obj.contains(QStringLiteral("selvedgeWidth")))
        setSelvedgeWidth(obj[QStringLiteral("selvedgeWidth")].toDouble(m_selvedgeWidth));

    if (obj.contains(QStringLiteral("pieceGap")))
        setPieceGap(obj[QStringLiteral("pieceGap")].toDouble(m_pieceGap));

    // Force all QML property bindings to re-evaluate after loading.
    // Setters above suppress emission when a value is unchanged (early-return
    // guard), so QML controls with broken runtime bindings — RadioButton,
    // ComboBox, and TextField whose text: binding was broken by prior user
    // interaction — would not refresh.  Emitting unconditionally here ensures
    // every dialog field sees a notification regardless of whether its value
    // changed.  Qt / QML handles duplicate change signals safely by
    // coalescing visual updates within a single frame.
    emit layoutModeChanged();
    emit rotationStepChanged();
    emit fabricFoldedChanged();
    emit pieceGapChanged();
    emit pieceGapPxChanged();
    emit unitChanged();
    emit rollLengthChanged();
    emit pageWidthPxChanged();
    emit pageHeightPxChanged();
    emit rollWidthPxChanged();
    emit mediaTypeChanged();
    emit paperTypeChanged();
    emit sheetNameChanged();
    emit pageWidthChanged();
    emit pageHeightChanged();
    emit rollSizeChanged();
    emit rollWidthChanged();
    emit tileSizeChanged();
    emit tileOrientationChanged();
    emit marginTopChanged();
    emit marginBottomChanged();
    emit marginLeftChanged();
    emit marginRightChanged();
    emit fabricWidthChanged();
    emit fabricHeightChanged();
    emit selvedgeWidthChanged();
    emit settingsLoaded();

    Logger::log(QStringLiteral("SettingsModel::load(): loaded successfully"));
    return true;
} // load()

// @brief Save current settings to a JSON file.
bool SettingsModel::save(const QString &path)
{
    Logger::log(QStringLiteral("SettingsModel::save(): path=") + path);

    // Ensure the parent directory exists
    QDir dir = QFileInfo(path).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    } // if dir missing

    QJsonObject obj;
    obj[QStringLiteral("layoutMode")]    = m_layoutMode;
    obj[QStringLiteral("rotationStep")]  = m_rotationStep;
    obj[QStringLiteral("fabricFolded")]  = m_fabricFolded;
    obj[QStringLiteral("unit")]          = m_unit;
    obj[QStringLiteral("mediaType")]     = m_mediaType;
    obj[QStringLiteral("paperType")]     = m_paperType;
    obj[QStringLiteral("sheetName")]     = m_sheetName;
    obj[QStringLiteral("pageWidth")]     = m_pageWidth;
    obj[QStringLiteral("pageHeight")]    = m_pageHeight;
    obj[QStringLiteral("rollSize")]      = m_rollSize;
    obj[QStringLiteral("rollWidth")]     = m_rollWidth;
    obj[QStringLiteral("tileSize")]      = m_tileSize;
    obj[QStringLiteral("tileOrientation")] = m_tileOrientation;
    obj[QStringLiteral("marginTop")]     = m_marginTop;
    obj[QStringLiteral("marginBottom")]  = m_marginBottom;
    obj[QStringLiteral("marginLeft")]    = m_marginLeft;
    obj[QStringLiteral("marginRight")]   = m_marginRight;
    obj[QStringLiteral("fabricWidth")]   = m_fabricWidth;
    obj[QStringLiteral("fabricHeight")]  = m_fabricHeight;
    obj[QStringLiteral("selvedgeWidth")] = m_selvedgeWidth;
    obj[QStringLiteral("pieceGap")]      = m_pieceGap;
    obj[QStringLiteral("outputFormat")]  = QStringLiteral("svg"); // Phase 8 makes this configurable

    const QJsonDocument doc(obj);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    } // if !open

    f.write(doc.toJson(QJsonDocument::Indented));
    Logger::log(QStringLiteral("SettingsModel::save(): saved successfully"));
    return true;
} // save()

// @brief Serialize current settings to a compact JSON string for the Rust bridge.
// Produces the same camelCase keys as save(), but returns a QString instead of
// writing to disk.  Used by QML: appController.processLayout(settingsModel.toJson()).
QString SettingsModel::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("layoutMode")]    = m_layoutMode;
    obj[QStringLiteral("rotationStep")]  = m_rotationStep;
    obj[QStringLiteral("fabricFolded")]  = m_fabricFolded;
    obj[QStringLiteral("unit")]          = m_unit;
    obj[QStringLiteral("mediaType")]     = m_mediaType;
    obj[QStringLiteral("paperType")]     = m_paperType;
    obj[QStringLiteral("sheetName")]     = m_sheetName;
    obj[QStringLiteral("pageWidth")]     = m_pageWidth;
    obj[QStringLiteral("pageHeight")]    = m_pageHeight;
    obj[QStringLiteral("rollSize")]      = m_rollSize;
    obj[QStringLiteral("rollWidth")]     = m_rollWidth;
    obj[QStringLiteral("tileSize")]      = m_tileSize;
    obj[QStringLiteral("tileOrientation")] = m_tileOrientation;
    obj[QStringLiteral("marginTop")]     = m_marginTop;
    obj[QStringLiteral("marginBottom")]  = m_marginBottom;
    obj[QStringLiteral("marginLeft")]    = m_marginLeft;
    obj[QStringLiteral("marginRight")]   = m_marginRight;
    obj[QStringLiteral("fabricWidth")]   = m_fabricWidth;
    obj[QStringLiteral("fabricHeight")]  = m_fabricHeight;
    obj[QStringLiteral("selvedgeWidth")] = m_selvedgeWidth;
    obj[QStringLiteral("pieceGap")]      = m_pieceGap;
    obj[QStringLiteral("outputFormat")]  = QStringLiteral("svg");

    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
} // toJson()

// @brief Reset all fields to the application defaults.
void SettingsModel::resetToDefaults()
{
    setLayoutMode(QStringLiteral("alongGrainline"));
    setRotationStep(0.0);
    setFabricFolded(false);
    setUnit(QStringLiteral("in"));
    setMediaType(QStringLiteral("paper"));
    setPaperType(QStringLiteral("sheet"));
    setSheetName(QStringLiteral("ARCH E"));
    setPageWidth(36.0);
    setPageHeight(48.0);
    setRollSize(QStringLiteral("36 in"));
    setRollWidth(36.0);
    setTileSize(QStringLiteral("Letter"));
    setTileOrientation(QStringLiteral("landscape"));
    setMarginTop(0.25);
    setMarginBottom(0.25);
    setMarginLeft(0.25);
    setMarginRight(0.25);
    setFabricWidth(0.0);
    setFabricHeight(0.0);
    setSelvedgeWidth(0.0);
    setPieceGap(0.05);  // ≈ 5 px @ 96 dpi; matches the historic GAP_PX const
} // resetToDefaults()

// @brief Convert all dimension and margin fields between unit systems.
void SettingsModel::convertAllUnits(const QString &fromUnit, const QString &toUnit)
{
    if (fromUnit == toUnit) return;

    setPageWidth(convertUnit(m_pageWidth, fromUnit, toUnit));
    setPageHeight(convertUnit(m_pageHeight, fromUnit, toUnit));
    setRollWidth(convertUnit(m_rollWidth, fromUnit, toUnit));
    setMarginTop(convertUnit(m_marginTop, fromUnit, toUnit));
    setMarginBottom(convertUnit(m_marginBottom, fromUnit, toUnit));
    setMarginLeft(convertUnit(m_marginLeft, fromUnit, toUnit));
    setMarginRight(convertUnit(m_marginRight, fromUnit, toUnit));

    if (m_fabricWidth  > 0.0) setFabricWidth(convertUnit(m_fabricWidth, fromUnit, toUnit));
    if (m_fabricHeight > 0.0) setFabricHeight(convertUnit(m_fabricHeight, fromUnit, toUnit));
    if (m_selvedgeWidth > 0.0) setSelvedgeWidth(convertUnit(m_selvedgeWidth, fromUnit, toUnit));

    // Piece gap is unconditional — it always has a positive default, so the
    // ">0" guard the optional fields above use isn't needed.
    setPieceGap(convertUnit(m_pieceGap, fromUnit, toUnit));
} // convertAllUnits()

// @brief Look up a paper size by name and update pageWidth/pageHeight.
void SettingsModel::selectPaperSize(const QString &name)
{
    for (int i = 0; i < PAPER_SIZES_COUNT; ++i) {
        if (QString::fromUtf8(PAPER_SIZES[i].name) == name) {
            setSheetName(name);
            if (m_unit == QStringLiteral("in")) {
                setPageWidth(PAPER_SIZES[i].widthIn);
                setPageHeight(PAPER_SIZES[i].heightIn);
            } else if (m_unit == QStringLiteral("mm")) {
                setPageWidth(PAPER_SIZES[i].widthMm);
                setPageHeight(PAPER_SIZES[i].heightMm);
            } else { // cm
                setPageWidth(PAPER_SIZES[i].widthMm / 10.0);
                setPageHeight(PAPER_SIZES[i].heightMm / 10.0);
            } // if unit
            return;
        } // if name matches
    } // for i
} // selectPaperSize()

// @brief Look up a tile size by name and update tileSize property.
void SettingsModel::selectTileSize(const QString &name)
{
    for (int i = 0; i < TILE_SIZES_COUNT; ++i) {
        if (QString::fromUtf8(TILE_SIZES[i].name) == name) {
            setTileSize(name);
            return;
        } // if name matches
    } // for i
} // selectTileSize()

// @brief Return the appropriate default margin value for the given unit.
double SettingsModel::defaultMarginForUnit(const QString &unit) const
{
    if (unit == QStringLiteral("cm")) return 1.0;
    if (unit == QStringLiteral("mm")) return 10.0;
    return 0.25; // inches (default)
} // defaultMarginForUnit()

void SettingsModel::syncFabricMarginsFromSelvedge()
{
    setMarginTop(m_selvedgeWidth);
    setMarginBottom(m_selvedgeWidth);
    setMarginLeft(m_selvedgeWidth);
    setMarginRight(m_selvedgeWidth);
} // syncFabricMarginsFromSelvedge()

// ---------------------------------------------------------------------------
// Private helper
// ---------------------------------------------------------------------------

// @brief Convert a single value between "in", "mm", and "cm".
// @param value  The value to convert.
// @param fromUnit Source unit ("in", "mm", "cm").
// @param toUnit   Target unit ("in", "mm", "cm").
// @return Converted value, or original value if units are the same.
double SettingsModel::convertUnit(double value, const QString &fromUnit, const QString &toUnit)
{
    if (fromUnit == toUnit) return value;

    // Convert to mm first
    double mm = value;
    if      (fromUnit == QStringLiteral("in")) mm = value * 25.4;
    else if (fromUnit == QStringLiteral("cm")) mm = value * 10.0;
    // else fromUnit == "mm": mm = value

    // Convert from mm to target
    if      (toUnit == QStringLiteral("in")) return mm / 25.4;
    else if (toUnit == QStringLiteral("cm")) return mm / 10.0;
    return mm; // toUnit == "mm"
} // convertUnit()
