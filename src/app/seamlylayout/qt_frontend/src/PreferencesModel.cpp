// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file PreferencesModel.cpp
// @brief Implementation of PreferencesModel — application preferences load/save.
//
// JSON is read/written with snake_case keys matching the Rust serde format
// Matches the Rust AppSettings struct used by the Qt frontend.

#include "PreferencesModel.h"
#include "Logger.h"
#include "Platform.h"
#include "SeamlyTheme.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QLineEdit>
#include <QPushButton>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QStyleFactory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace {

constexpr auto kSettingsFolderName = "settings";
constexpr auto kLegacySettingsFolderName = "layout-settings";
constexpr auto kPreferencesFolderName = "preferences";
constexpr auto kLegacyPreferencesFolderName = "layout-preferences";
constexpr auto kLegacyOrganizationName = "Seamly Systems";

// Forward declaration — defined further down; migrateLegacyOrganizationTree() (added for
// Task 15) needs it before its definition appears in this file.
bool copyIfMissing(const QString &sourcePath, const QString &destPath);

// @brief Recursively copy every entry from a legacy organization directory tree into the
// new one, skipping anything the destination already has.
//
// Task 15: seamlyLayout's organization name changed from "Seamly Systems" to the shared
// "Seamly" (see main.cpp), so QStandardPaths::AppConfigLocation now resolves to a brand
// new, empty directory. This bridges every settings/preferences file forward from the old
// organization folder the first time the new one is resolved. Safe to call unconditionally
// on every appConfigRootPath() call — once everything has been copied across it is a cheap
// no-op (copyIfMissing never overwrites an existing destination file).
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

// @brief Return the writable app-config root path.
// Falls back to <exeDir> only if Qt cannot provide AppConfigLocation.
//
// Task 15: also bridges data forward from the pre-unification "Seamly Systems"
// organization folder into the new shared "Seamly" one on first use. The legacy root is
// computed by asking QStandardPaths for AppConfigLocation under the old organization name
// (temporarily swapped in), which reconstructs the exact platform-specific legacy path
// (Windows/macOS/Linux each resolve AppConfigLocation differently) without hard-coding any
// of that platform logic here.
QString appConfigRootPath()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (root.isEmpty()) {
        root = QCoreApplication::applicationDirPath();
    } // if AppConfigLocation unavailable
    root = QDir(root).absolutePath();
    QDir().mkpath(root);

    const QString currentOrganization = QCoreApplication::organizationName();
    if (currentOrganization != QString::fromUtf8(kLegacyOrganizationName)) {
        QCoreApplication::setOrganizationName(QString::fromUtf8(kLegacyOrganizationName));
        const QString legacyRoot = QDir(
            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).absolutePath();
        QCoreApplication::setOrganizationName(currentOrganization);

        if (legacyRoot != root) {
            migrateLegacyOrganizationTree(legacyRoot, root);
        } // if legacy root resolves to a different directory
    } // if not already running under the legacy organization name

    return root;
} // appConfigRootPath

// @brief Return the platform key used in default_preferences.json.
QString platformDefaultsKey()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#else
    return QStringLiteral("linux");
#endif
} // platformDefaultsKey

// @brief Replace supported tokens in default path templates.
QString expandDefaultPathTokens(const QString &value)
{
    QString result = value;
    result.replace(QStringLiteral("${HOME}"), QDir::homePath());
    return QDir::cleanPath(result);
} // expandDefaultPathTokens

// @brief Seed preferences.json from bundled resource defaults.
bool seedFromBundledDefaults(const QString &destPath)
{
    QFile bundled(QStringLiteral(":/defaults/default_preferences.json"));
    if (!bundled.open(QIODevice::ReadOnly)) {
        return false;
    } // if cannot open resource

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(bundled.readAll(), &parseError);
    bundled.close();
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    } // if invalid JSON

    const QJsonObject root = doc.object();
    const QString key = platformDefaultsKey();
    const QJsonObject source = root.value(key).isObject() ? root.value(key).toObject() : root;

    // Build normalized OS-appropriate defaults for runtime use.
    QJsonObject out;

    const QString defaultSettingsDir = QDir::toNativeSeparators(
        expandDefaultPathTokens(source.value(QStringLiteral("settings_directory"))
            .toString(QStringLiteral("${HOME}/seamlyLayout/settings"))));
    const QString defaultPreferencesDir = QDir::toNativeSeparators(
        expandDefaultPathTokens(source.value(QStringLiteral("preferences_directory"))
            .toString(QStringLiteral("${HOME}/seamlyLayout/preferences"))));
    const QString defaultInputDir = QDir::toNativeSeparators(
        expandDefaultPathTokens(source.value(QStringLiteral("input_directory"))
            .toString(QStringLiteral("${HOME}/seamlyLayout/input"))));
    const QString defaultLayoutDir = QDir::toNativeSeparators(
        expandDefaultPathTokens(source.value(QStringLiteral("layout_directory"))
            .toString(QStringLiteral("${HOME}/seamlyLayout/output"))));

    QString defaultSettingsFile = expandDefaultPathTokens(
        source.value(QStringLiteral("settings_file"))
            .toString(QStringLiteral("${HOME}/seamlyLayout/settings/default_settings.json")));
    if (!QFileInfo(defaultSettingsFile).isAbsolute()) {
        defaultSettingsFile = QFileInfo(QDir(defaultSettingsDir).filePath(defaultSettingsFile)).absoluteFilePath();
    } // if relative settings_file
    defaultSettingsFile = QDir::toNativeSeparators(defaultSettingsFile);

    QString defaultPreferencesFile = expandDefaultPathTokens(
        source.value(QStringLiteral("preferences_file"))
            .toString(QStringLiteral("${HOME}/seamlyLayout/preferences/default_preferences.json")));
    if (!QFileInfo(defaultPreferencesFile).isAbsolute()) {
        defaultPreferencesFile = QFileInfo(QDir(defaultPreferencesDir).filePath(defaultPreferencesFile)).absoluteFilePath();
    } // if relative preferences_file
    defaultPreferencesFile = QDir::toNativeSeparators(defaultPreferencesFile);

    out[QStringLiteral("settings_directory")] = defaultSettingsDir;
    out[QStringLiteral("preferences_directory")] = defaultPreferencesDir;
    out[QStringLiteral("preferences_file")] = defaultPreferencesFile;
    out[QStringLiteral("input_directory")] = defaultInputDir;
    out[QStringLiteral("layout_directory")] = defaultLayoutDir;
    out[QStringLiteral("settings_file")] = defaultSettingsFile;
    out[QStringLiteral("dxf_viewer_path")] = source.value(QStringLiteral("dxf_viewer_path")).toString(QStringLiteral(""));
    out[QStringLiteral("pdf_viewer_path")] = source.value(QStringLiteral("pdf_viewer_path")).toString(QStringLiteral(""));
    out[QStringLiteral("png_viewer_path")] = source.value(QStringLiteral("png_viewer_path")).toString(QStringLiteral(""));
    out[QStringLiteral("projector_path")]  = source.value(QStringLiteral("projector_path")).toString(QStringLiteral(""));

    // Ensure default folders exist at first run.
    QDir().mkpath(defaultSettingsDir);
    QDir().mkpath(defaultPreferencesDir);
    QDir().mkpath(defaultInputDir);
    QDir().mkpath(defaultLayoutDir);

    QDir destDir = QFileInfo(destPath).absoluteDir();
    if (!destDir.exists()) {
        destDir.mkpath(QStringLiteral("."));
    } // if missing

    QFile outFile(destPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    } // if cannot write

    outFile.write(QJsonDocument(out).toJson(QJsonDocument::Indented));
    outFile.close();
    return true;
} // seedFromBundledDefaults

// @brief Copy a file if destination does not already exist.
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

// @brief Migrate a path whose last directory segment uses a legacy folder name.
QString migrateLegacyFolderName(const QString &path,
                                const QString &legacyFolder,
                                const QString &newFolder)
{
    if (path.trimmed().isEmpty()) return path;

    const QFileInfo fi(path);
    if (fi.fileName() == legacyFolder) {
        return QDir(fi.absolutePath()).filePath(newFolder);
    } // if directory path points at legacy folder

    const QFileInfo parentInfo(fi.absolutePath());
    if (parentInfo.fileName() == legacyFolder) {
        return QDir(parentInfo.absolutePath()).filePath(newFolder + QLatin1Char('/') + fi.fileName());
    } // if file path is under legacy folder

    return path;
} // migrateLegacyFolderName

} // namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

// @brief Construct with default field values (matching AppSettings::default()).
PreferencesModel::PreferencesModel(QObject *parent)
    : QObject(parent)
{
} // PreferencesModel

// ---------------------------------------------------------------------------
// Setters
// ---------------------------------------------------------------------------

// @brief Set the input SVG directory; emits inputDirectoryChanged if changed.
void PreferencesModel::setInputDirectory(const QString &v)
{
    if (m_inputDirectory == v) return;
    m_inputDirectory = v;
    emit inputDirectoryChanged();
} // setInputDirectory

// @brief Set the layout output directory; emits layoutDirectoryChanged if changed.
void PreferencesModel::setLayoutDirectory(const QString &v)
{
    if (m_layoutDirectory == v) return;
    m_layoutDirectory = v;
    emit layoutDirectoryChanged();
} // setLayoutDirectory

// @brief Set the preferences directory path; emits preferencesDirectoryChanged if changed.
void PreferencesModel::setPreferencesDirectory(const QString &v)
{
    if (m_preferencesDirectory == v) return;
    m_preferencesDirectory = v;
    emit preferencesDirectoryChanged();
} // setPreferencesDirectory

// @brief Set the default settings directory path; emits settingsDirectoryChanged if changed.
void PreferencesModel::setSettingsDirectory(const QString &v)
{
    if (m_settingsDirectory == v) return;
    m_settingsDirectory = v;
    emit settingsDirectoryChanged();
} // setSettingsDirectory

// @brief Set the default settings file path; emits settingsFileChanged if changed.
void PreferencesModel::setSettingsFile(const QString &v)
{
    if (m_settingsFile == v) return;
    m_settingsFile = v;
    emit settingsFileChanged();
} // setSettingsFile

// @brief Set the default preferences file path; emits preferencesFileChanged if changed.
void PreferencesModel::setPreferencesFile(const QString &v)
{
    if (m_preferencesFile == v) return;
    m_preferencesFile = v;
    emit preferencesFileChanged();
} // setPreferencesFile

// @brief Set the DXF viewer executable path; emits dxfViewerPathChanged if changed.
void PreferencesModel::setDxfViewerPath(const QString &v)
{
    if (m_dxfViewerPath == v) return;
    m_dxfViewerPath = v;
    emit dxfViewerPathChanged();
} // setDxfViewerPath

// @brief Set the PDF viewer executable path; emits pdfViewerPathChanged if changed.
void PreferencesModel::setPdfViewerPath(const QString &v)
{
    if (m_pdfViewerPath == v) return;
    m_pdfViewerPath = v;
    emit pdfViewerPathChanged();
} // setPdfViewerPath

// @brief Set the PNG viewer executable path; emits pngViewerPathChanged if changed.
void PreferencesModel::setPngViewerPath(const QString &v)
{
    if (m_pngViewerPath == v) return;
    m_pngViewerPath = v;
    emit pngViewerPathChanged();
} // setPngViewerPath

// @brief Set the projector executable path / URL; emits projectorPathChanged if changed.
void PreferencesModel::setProjectorPath(const QString &v)
{
    if (m_projectorPath == v) return;
    m_projectorPath = v;
    emit projectorPathChanged();
} // setProjectorPath

// ---------------------------------------------------------------------------
// load
// ---------------------------------------------------------------------------

// @brief Load preferences from a JSON file.
// Uses snake_case keys matching the Rust serde AppSettings format.
// If the file does not exist, returns false and keeps current defaults.
bool PreferencesModel::load(const QString &path)
{
    Logger::log(QStringLiteral("==========LOAD PREFERENCES=========="));
    Logger::log(QStringLiteral("PreferencesModel::load(): path=") + path);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        // File not found — keep current defaults
        Logger::log(QStringLiteral("PreferencesModel::load(): file not found, keeping defaults"));
        return false;
    } // if !open

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (doc.isNull() || !doc.isObject()) {
        return false;
    } // if parse failed

    QJsonObject obj = doc.object();

    // Read each field; fall back to current value if key is absent
    setInputDirectory   (obj.value(QStringLiteral("input_directory"))   .toString(m_inputDirectory));
    setLayoutDirectory  (obj.value(QStringLiteral("layout_directory"))  .toString(m_layoutDirectory));
    setPreferencesDirectory(obj.value(QStringLiteral("preferences_directory")).toString(m_preferencesDirectory));
    setSettingsDirectory(obj.value(QStringLiteral("settings_directory")).toString(m_settingsDirectory));
    setSettingsFile     (obj.value(QStringLiteral("settings_file"))     .toString(m_settingsFile));
    setPreferencesFile  (obj.value(QStringLiteral("preferences_file"))  .toString(m_preferencesFile));
    setDxfViewerPath    (obj.value(QStringLiteral("dxf_viewer_path"))   .toString(m_dxfViewerPath));
    setPdfViewerPath    (obj.value(QStringLiteral("pdf_viewer_path"))   .toString(m_pdfViewerPath));
    setPngViewerPath    (obj.value(QStringLiteral("png_viewer_path"))   .toString(m_pngViewerPath));
    setProjectorPath    (obj.value(QStringLiteral("projector_path"))    .toString(m_projectorPath));

    // Migrate legacy folder names in persisted preference paths:
    //   layout-settings     -> settings
    //   layout-preferences  -> preferences
    const QString migratedSettingsDir = migrateLegacyFolderName(
        m_settingsDirectory,
        QString::fromUtf8(kLegacySettingsFolderName),
        QString::fromUtf8(kSettingsFolderName));
    if (migratedSettingsDir != m_settingsDirectory) {
        QDir().mkpath(migratedSettingsDir);
        copyIfMissing(
            QDir(m_settingsDirectory).filePath(QStringLiteral("default_settings.json")),
            QDir(migratedSettingsDir).filePath(QStringLiteral("default_settings.json")));
        setSettingsDirectory(migratedSettingsDir);
    } // if migrated settings directory

    const QString migratedPreferencesDir = migrateLegacyFolderName(
        m_preferencesDirectory,
        QString::fromUtf8(kLegacyPreferencesFolderName),
        QString::fromUtf8(kPreferencesFolderName));
    if (migratedPreferencesDir != m_preferencesDirectory) {
        QDir().mkpath(migratedPreferencesDir);
        copyIfMissing(
            QDir(m_preferencesDirectory).filePath(QStringLiteral("default_preferences.json")),
            QDir(migratedPreferencesDir).filePath(QStringLiteral("default_preferences.json")));
        setPreferencesDirectory(migratedPreferencesDir);
    } // if migrated preferences directory

    const QString migratedSettingsFile = migrateLegacyFolderName(
        m_settingsFile,
        QString::fromUtf8(kLegacySettingsFolderName),
        QString::fromUtf8(kSettingsFolderName));
    if (migratedSettingsFile != m_settingsFile) {
        copyIfMissing(m_settingsFile, migratedSettingsFile);
        setSettingsFile(migratedSettingsFile);
    } // if migrated settings file

    const QString migratedPreferencesFile = migrateLegacyFolderName(
        m_preferencesFile,
        QString::fromUtf8(kLegacyPreferencesFolderName),
        QString::fromUtf8(kPreferencesFolderName));
    if (migratedPreferencesFile != m_preferencesFile) {
        copyIfMissing(m_preferencesFile, migratedPreferencesFile);
        setPreferencesFile(migratedPreferencesFile);
    } // if migrated preferences file

    Logger::log(QStringLiteral("PreferencesModel::load(): loaded successfully"));
    return true;
} // load

// ---------------------------------------------------------------------------
// save
// ---------------------------------------------------------------------------

// @brief Save preferences to a JSON file.
// Creates the parent directory if it does not exist.
// Uses snake_case keys matching the Rust serde AppSettings format.
bool PreferencesModel::save(const QString &path)
{
    Logger::log(QStringLiteral("PreferencesModel::save(): path=") + path);

    // Ensure parent directory exists
    QDir dir = QFileInfo(path).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(dir.absolutePath());
    } // if dir does not exist

    QJsonObject obj;
    obj[QStringLiteral("input_directory")]  = m_inputDirectory;
    obj[QStringLiteral("layout_directory")] = m_layoutDirectory;
    obj[QStringLiteral("preferences_directory")] = m_preferencesDirectory;
    obj[QStringLiteral("settings_directory")] = m_settingsDirectory;
    obj[QStringLiteral("settings_file")]    = m_settingsFile;
    obj[QStringLiteral("preferences_file")] = m_preferencesFile;
    obj[QStringLiteral("dxf_viewer_path")]  = m_dxfViewerPath;
    obj[QStringLiteral("pdf_viewer_path")]  = m_pdfViewerPath;
    obj[QStringLiteral("png_viewer_path")]  = m_pngViewerPath;
    obj[QStringLiteral("projector_path")]   = m_projectorPath;

    QJsonDocument doc(obj);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    } // if !open

    file.write(doc.toJson(QJsonDocument::Indented));
    Logger::log(QStringLiteral("PreferencesModel::save(): saved successfully"));
    return true;
} // save

// @brief Reset preferences from the defaults profile file.
// Uses preferencesFilePath(); if missing, seeds from bundled defaults first.
bool PreferencesModel::resetToDefaults()
{
    const QString defaultsPath = preferencesFilePath();
    Logger::log(QStringLiteral("PreferencesModel::resetToDefaults(): defaultsPath=") + defaultsPath);

    if (!QFileInfo::exists(defaultsPath)) {
        if (seedFromBundledDefaults(defaultsPath)) {
            Logger::log(QStringLiteral("PreferencesModel::resetToDefaults(): seeded defaults profile from bundled resource"));
        } else {
            Logger::log(QStringLiteral("PreferencesModel::resetToDefaults(): failed to seed defaults profile"));
            return false;
        } // if seeded
    } // if missing defaults file

    const bool loaded = load(defaultsPath);
    if (!loaded) {
        Logger::log(QStringLiteral("PreferencesModel::resetToDefaults(): failed to load defaults profile"));
        return false;
    } // if load failed

    Logger::log(QStringLiteral("PreferencesModel::resetToDefaults(): defaults loaded successfully"));
    return true;
} // resetToDefaults

// ---------------------------------------------------------------------------
// urlToLocalFile
// ---------------------------------------------------------------------------

// @brief Convert a file:// URL string to a local file system path.
// Uses QUrl::toLocalFile() for correct cross-platform handling:
//   "file:///C:/Users/..."  →  "C:/Users/..."  (Windows)
//   "file:///home/user/..." →  "/home/user/..."  (Linux/macOS)
QString PreferencesModel::urlToLocalFile(const QString &url)
{
    return QUrl(url).toLocalFile();
} // urlToLocalFile

// @brief Convert a local file system path to a file:// URL string.
// Uses QUrl::fromLocalFile() for correct cross-platform handling:
//   "C:/Users/..."   →  "file:///C:/Users/..."  (Windows)
//   "/home/user/..." →  "file:///home/user/..."  (Linux/macOS)
QString PreferencesModel::localFileToUrl(const QString &path)
{
    if (path.isEmpty()) return QStringLiteral("");
    return QUrl::fromLocalFile(path).toString();
} // localFileToUrl

// @brief Return the file:// URL of the default input directory.
// Uses QCoreApplication::applicationDirPath() so the path is absolute regardless
// of the process working directory.  The returned URL is suitable for
// FileDialog.currentFolder.
//
// Task 16: on macOS a signed, notarized .app bundle is read-only, so the exe-relative
// default used on Windows/Linux would fail to create — fall back to the writable
// AppConfigLocation root there instead (the same "Seamly/SeamlyLayout" tree the settings
// and preferences files already live under).
QString PreferencesModel::defaultInputFolderUrl()
{
#if defined(Q_OS_MACOS)
    QString dir = appConfigRootPath() + QStringLiteral("/input");
#else
    QString dir = QCoreApplication::applicationDirPath() + QStringLiteral("/input");
#endif
    return QUrl::fromLocalFile(dir).toString();
} // defaultInputFolderUrl

// @brief Return the absolute default preferences file path.
    // Uses AppConfigLocation/preferences/preferences.json and ensures the directory exists.
QString PreferencesModel::defaultPreferencesFilePath() const
{
    const QString appConfigRoot = appConfigRootPath();
    const QString prefDir = QDir(appConfigRoot).filePath(QString::fromUtf8(kPreferencesFolderName));

    QDir dir(prefDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    } // if missing

    const QString target = QFileInfo(dir.filePath(QStringLiteral("preferences.json"))).absoluteFilePath();

    // First-run migration/seed: copy legacy preferences if present.
    const QString legacyAppConfig = QFileInfo(
        QDir(appConfigRoot).filePath(
            QString::fromUtf8(kLegacyPreferencesFolderName) + QStringLiteral("/preferences.json"))).absoluteFilePath();
    const QString legacyPackagedLayoutSettings = QFileInfo(
        QDir(QCoreApplication::applicationDirPath()).filePath(
            QString::fromUtf8(kLegacySettingsFolderName) + QStringLiteral("/preferences.json"))).absoluteFilePath();
    const QString legacyPackagedSettings = QFileInfo(
        QDir(QCoreApplication::applicationDirPath()).filePath(
            QString::fromUtf8(kSettingsFolderName) + QStringLiteral("/preferences.json"))).absoluteFilePath();
    if (copyIfMissing(legacyAppConfig, target)
        || copyIfMissing(legacyPackagedLayoutSettings, target)
        || copyIfMissing(legacyPackagedSettings, target)) {
        Logger::log(QStringLiteral("PreferencesModel::defaultPreferencesFilePath(): migrated legacy preferences to user preferences directory"));
    } else if (!QFileInfo::exists(target) && seedFromBundledDefaults(target)) {
        Logger::log(QStringLiteral("PreferencesModel::defaultPreferencesFilePath(): seeded default preferences from bundled resource"));
    } // if copied

    return target;
} // defaultPreferencesFilePath

// @brief Return the resolved settings directory for load/save dialogs.
//
// Priority:
//   1. preferences settingsDirectory — if non-empty.
//   2. AppConfigLocation/settings — fallback default.
//
// Ensures the directory exists before returning the absolute path.
QString PreferencesModel::resolvedSettingsDirectory() const
{
    QString dir = m_settingsDirectory;
    const QString appConfigRoot = appConfigRootPath();

    if (dir.isEmpty()) {
        dir = QDir(appConfigRoot).filePath(QString::fromUtf8(kSettingsFolderName));
    } else {
        // Resolve configured relative paths against AppConfig root so runtime behavior
        // is independent of the process current working directory.
        const QFileInfo fi(dir);
        if (!fi.isAbsolute()) {
            dir = QFileInfo(QDir(appConfigRoot).filePath(dir)).absoluteFilePath();
        } // if relative
    } // if no configured directory

    QDir qdir(dir);
    if (!qdir.exists()) {
        qdir.mkpath(QStringLiteral("."));
    } // if missing

    // First-run migration/seed for default settings file in the resolved folder.
    const QString target = QFileInfo(qdir.filePath(QStringLiteral("default_settings.json"))).absoluteFilePath();
    const QString legacyAppConfig = QFileInfo(
        QDir(appConfigRoot).filePath(
            QString::fromUtf8(kLegacySettingsFolderName) + QStringLiteral("/default_settings.json"))).absoluteFilePath();
    const QString legacyPackagedLayoutSettings = QFileInfo(
        QDir(QCoreApplication::applicationDirPath()).filePath(
            QString::fromUtf8(kLegacySettingsFolderName) + QStringLiteral("/default_settings.json"))).absoluteFilePath();
    const QString legacyPackagedSettings = QFileInfo(
        QDir(QCoreApplication::applicationDirPath()).filePath(
            QString::fromUtf8(kSettingsFolderName) + QStringLiteral("/default_settings.json"))).absoluteFilePath();
    if (copyIfMissing(legacyAppConfig, target)
        || copyIfMissing(legacyPackagedLayoutSettings, target)
        || copyIfMissing(legacyPackagedSettings, target)) {
        Logger::log(QStringLiteral("PreferencesModel::resolvedSettingsDirectory(): migrated legacy settings to AppConfigLocation"));
    } // if copied

    return qdir.absolutePath();
} // resolvedSettingsDirectory

// @brief Return resolved input directory for import file dialogs.
// Priority:
//   1. preferences inputDirectory — if non-empty.
//   2. <exeDir>/input — fallback default (AppConfigLocation root on macOS; see Task 16).
// Relative configured paths are resolved against <exeDir>.
// Ensures the directory exists before returning.
QString PreferencesModel::resolvedInputDirectory() const
{
    QString dir = m_inputDirectory;
    const QString appConfigRoot = appConfigRootPath();

    if (dir.isEmpty()) {
#if defined(Q_OS_MACOS)
        // Task 16: a signed .app bundle is read-only — use the writable AppConfigLocation
        // root instead of the bundle-relative path used on Windows/Linux.
        dir = appConfigRoot + QStringLiteral("/input");
#else
        dir = QCoreApplication::applicationDirPath() + QStringLiteral("/input");
#endif
    } else {
        const QFileInfo fi(dir);
        if (!fi.isAbsolute()) {
            dir = QFileInfo(QDir(appConfigRoot).filePath(dir)).absoluteFilePath();
        } // if relative
    } // if no configured directory

    QDir qdir(dir);
    if (!qdir.exists()) {
        qdir.mkpath(QStringLiteral("."));
    } // if missing

    return qdir.absolutePath();
} // resolvedInputDirectory

// @brief Return the settings file path to use when the Settings dialog opens.
//
// Rules:
//   1. If settingsFile is absolute, use it as-is.
//   2. If settingsFile is relative, resolve it against resolvedSettingsDirectory().
//   3. If settingsFile is empty, use "default_settings.json" in resolvedSettingsDirectory().
QString PreferencesModel::settingsFilePath() const
{
    const QString settingsDir = resolvedSettingsDirectory();
    const QString configured = m_settingsFile.trimmed();

    if (configured.isEmpty()) {
        return QDir(settingsDir).filePath(QStringLiteral("default_settings.json"));
    } // if no configured file

    const QFileInfo fi(configured);
    if (fi.isAbsolute()) {
        return fi.absoluteFilePath();
    } // if absolute path

    return QFileInfo(QDir(settingsDir).filePath(configured)).absoluteFilePath();
} // resolvedSettingsFilePath

// @brief Return the default preferences file path.
// Rules:
//   1. If preferencesFile is absolute, use it as-is.
//   2. If preferencesFile is relative, resolve it against preferencesDirectory.
//   3. If preferencesFile is empty, use "default_preferences.json" in preferencesDirectory.
QString PreferencesModel::preferencesFilePath() const
{
    QString baseDir = m_preferencesDirectory;
    if (baseDir.trimmed().isEmpty()) {
        baseDir = QDir(appConfigRootPath()).filePath(QString::fromUtf8(kPreferencesFolderName));
    } else {
        const QFileInfo fi(baseDir);
        if (!fi.isAbsolute()) {
            baseDir = QFileInfo(QDir(appConfigRootPath()).filePath(baseDir)).absoluteFilePath();
        } // if relative
    } // if missing configured dir

    QDir dir(baseDir);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    } // if missing

    const QString configured = m_preferencesFile.trimmed();
    if (configured.isEmpty()) {
        return QFileInfo(dir.filePath(QStringLiteral("default_preferences.json"))).absoluteFilePath();
    } // if no configured file

    const QFileInfo fi(configured);
    if (fi.isAbsolute()) {
        return fi.absoluteFilePath();
    } // if absolute

    return QFileInfo(dir.filePath(configured)).absoluteFilePath();
} // preferencesFilePath

// @brief Return resolved output directory for export file dialogs.
// Priority:
//   1. preferences layoutDirectory — if non-empty.
//   2. <exeDir>/output — fallback default (AppConfigLocation root on macOS; see Task 16).
// Ensures the directory exists before returning.
QString PreferencesModel::resolvedLayoutDirectory() const
{
    Logger::log(QStringLiteral("PreferencesModel::resolvedLayoutDirectory(): layoutDirectory=\"")
                + m_layoutDirectory + QStringLiteral("\""));

    QString dir = m_layoutDirectory;
    const QString appConfigRoot = appConfigRootPath();

    if (dir.isEmpty()) {
#if defined(Q_OS_MACOS)
        // Task 16: a signed .app bundle is read-only — use the writable AppConfigLocation
        // root instead of the bundle-relative path used on Windows/Linux.
        dir = appConfigRoot + QStringLiteral("/output");
#else
        dir = QCoreApplication::applicationDirPath() + QStringLiteral("/output");
#endif
    } else {
        // Resolve configured relative paths against AppConfig root so runtime behavior
        // is independent of the process current working directory.
        const QFileInfo fi(dir);
        if (!fi.isAbsolute()) {
            dir = QFileInfo(QDir(appConfigRoot).filePath(dir)).absoluteFilePath();
        } // if relative
    } // if no configured directory

    QDir qdir(dir);
    if (!qdir.exists()) {
        qdir.mkpath(QStringLiteral("."));
    } // if missing

    Logger::log(QStringLiteral("PreferencesModel::resolvedLayoutDirectory(): resolved=\"")
                + qdir.absolutePath() + QStringLiteral("\""));
    return qdir.absolutePath();
} // resolvedLayoutDirectory

// ---------------------------------------------------------------------------
// getOpenFilePath
// ---------------------------------------------------------------------------

// @brief Open a Seamly-branded QtWidgets open-file dialog.
// Uses QFileDialog in non-native mode with Fusion style and Seamly violet palette
// so the dialog matches the application branding.
// @param title Dialog title.
// @param dir Initial directory.
// @param filter Name filter string (e.g. "DXF Files (*.dxf);;All Files (*)").
// @return Chosen absolute path, or empty string if the user cancelled.
QString PreferencesModel::getOpenFilePath(const QString &title,
                                          const QString &dir,
                                          const QString &filter)
{
    Logger::log(QStringLiteral("PreferencesModel::getOpenFilePath(): title=\"") + title
                + QStringLiteral("\" dir=\"") + dir
                + QStringLiteral("\" filter=\"") + filter + QStringLiteral("\""));

    // Store the absolute directory path
    QString absDir = dir.isEmpty() ? QDir::currentPath()
                                   : QDir(dir).absolutePath();

    QFileDialog dlg(nullptr, title, QString(), filter);
    dlg.setOption(QFileDialog::DontUseNativeDialog, true);
    dlg.setAcceptMode(QFileDialog::AcceptOpen);
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setDirectory(absDir);

    // Apply Seamly violet palette + Fusion style (per-window, does not affect QML)
    dlg.setPalette(SeamlyTheme::makeSeamlyPalette());
    dlg.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Set black text on input fields, combo boxes, and buttons
    for (QLineEdit *le : dlg.findChildren<QLineEdit *>())
        le->setStyleSheet(QStringLiteral("color: black;"));
    for (QComboBox *cb : dlg.findChildren<QComboBox *>())
        cb->setStyleSheet(QStringLiteral("QComboBox { color: black; } QComboBox QAbstractItemView { color: black; }"));
    for (QPushButton *btn : dlg.findChildren<QPushButton *>())
        btn->setStyleSheet(QStringLiteral("color: black;"));

    QString path;
    if (dlg.exec() == QDialog::Accepted) {
        // Get the absolute directory the dialog is actually in after the user
        // may have navigated, then combine it with just the filename.
        QString chosenDir  = dlg.directory().absolutePath();
        QStringList files  = dlg.selectedFiles();
        if (!files.isEmpty()) {
            QString fileName = QFileInfo(files.first()).fileName();
            path = Platform::toNativePath(chosenDir + QLatin1Char('/') + fileName);
        } // if files not empty
    } // if accepted

    Logger::log(QStringLiteral("getOpenFilePath: path=\"") + path + QStringLiteral("\""));
    return path;
} // getOpenFilePath

// ---------------------------------------------------------------------------
// getSaveFilePath
// ---------------------------------------------------------------------------

// @brief Open a Seamly-branded QtWidgets save-file dialog.
// Uses QFileDialog in non-native mode with Fusion style and Seamly violet palette
// so the dialog matches the application branding.
// @param title Dialog title.
// @param dir Initial directory.
// @param defaultName Default filename shown in the name field.
// @param filter Name filter string (e.g. "DXF Files (*.dxf);;All Files (*)").
// @return Chosen absolute path, or empty string if the user cancelled.
QString PreferencesModel::getSaveFilePath(const QString &title,
                                          const QString &dir,
                                          const QString &defaultName,
                                          const QString &filter)
{
    // Store the absolute directory path
    QString absDir = dir.isEmpty() ? QDir::currentPath()
                                   : QDir(dir).absolutePath();

    QFileDialog dlg(nullptr, title, QString(), filter);
    dlg.setOption(QFileDialog::DontUseNativeDialog, true);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setDirectory(absDir);
    if (!defaultName.isEmpty())
        dlg.selectFile(defaultName);

    // Apply Seamly violet palette + Fusion style (per-window, does not affect QML)
    dlg.setPalette(SeamlyTheme::makeSeamlyPalette());
    dlg.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Set black text on input fields, combo boxes, and buttons
    for (QLineEdit *le : dlg.findChildren<QLineEdit *>())
        le->setStyleSheet(QStringLiteral("color: black;"));
    for (QComboBox *cb : dlg.findChildren<QComboBox *>())
        cb->setStyleSheet(QStringLiteral("QComboBox { color: black; } QComboBox QAbstractItemView { color: black; }"));
    for (QPushButton *btn : dlg.findChildren<QPushButton *>())
        btn->setStyleSheet(QStringLiteral("color: black;"));

    QString path;
    if (dlg.exec() == QDialog::Accepted) {
        // Get the absolute directory the dialog is actually in after the user
        // may have navigated, then combine it with just the filename.
        QString chosenDir  = dlg.directory().absolutePath();
        QStringList files  = dlg.selectedFiles();
        if (!files.isEmpty()) {
            QString fileName = QFileInfo(files.first()).fileName(); // strip any directory
            path = Platform::toNativePath(chosenDir + QLatin1Char('/') + fileName);
        } // if files not empty
    } // if accepted

    Logger::log(QStringLiteral("getSaveFilePath: path=\"") + path + QStringLiteral("\""));
    return path;
} // getSaveFilePath

// ---------------------------------------------------------------------------
// openInViewer
// ---------------------------------------------------------------------------

/// @brief Check if a string is an HTTP/HTTPS URL.
/// @param str The string to check.
/// @return true if the string starts with http:// or https://.
static bool isHttpUrl(const QString &str)
{
    return str.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
        || str.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive);
}

// @brief Launch a file in a viewer application or open an online viewer URL.
// If viewerPath is an HTTP/HTTPS URL, opens the URL in the default browser.
// Otherwise, the viewer is parsed:
//   1. A leading `file:///` prefix is stripped (Qt FileDialog may return
//      URL-form paths; we tolerate that here too).
//   2. If the whole string resolves to an existing file (common case: bare
//      Windows path with unquoted spaces like
//      `C:\Program Files\Common Files\eDrawings2026\eDrawings.exe`), it is
//      used as the executable directly with no preset arguments.
//   3. Otherwise the string is parsed as a shell-style command
//      (executable + optional preset arguments) — supports the Chrome PWA
//      shortcut form which always uses quotes around the path:
//      `"C:\Program Files\Google\Chrome\Application\chrome_proxy.exe"
//       --profile-directory=Default --app-id=<id>`.
// The file path, when non-empty, is appended as the final argument. An empty
// file path is allowed for launcher-only viewers (e.g. a projector
// application that takes no file argument).
//
// On Windows, detects UWP/Store apps (App Execution Aliases) and skips the
// slow QProcess::startDetached failure by going straight to QDesktopServices.
// @param viewerPath HTTP/HTTPS URL OR a path (bare or quoted) OR a
//                   shell-style command.
// @param filePath   Absolute file path to append as the final argument, or
//                   empty when the viewer takes no file argument.
// @return true if the viewer was launched successfully.
bool PreferencesModel::openInViewer(const QString &viewerPath, const QString &filePath)
{
    if (viewerPath.isEmpty()) {
        Logger::log(QStringLiteral("PreferencesModel::openInViewer(): empty viewer path"));
        return false;
    } // if empty viewer

    // Handle online viewer URLs (HTTP/HTTPS).
    if (isHttpUrl(viewerPath)) {
        Logger::log(QStringLiteral("PreferencesModel::openInViewer(): opening URL viewer=")
                    + viewerPath);
        // Note: Online viewers like ShareCAD require users to upload files manually,
        // as local files cannot be passed directly to web services.
        return QDesktopServices::openUrl(QUrl(viewerPath));
    }

    QStringList parsed = parseViewerCommand(viewerPath);
    if (parsed.isEmpty()) {
        Logger::log(QStringLiteral("PreferencesModel::openInViewer(): viewer command parsed to no tokens"));
        return false;
    }
    const QString executable = parsed.takeFirst();
    QStringList args = parsed; // remainder are preset args

    QString nativeFile;
    if (!filePath.isEmpty()) {
        nativeFile = Platform::toNativePath(filePath);
        args.append(nativeFile);
    }

    Logger::log(QStringLiteral("PreferencesModel::openInViewer(): viewer=")
                + executable
                + QStringLiteral(" args=[") + args.join(QStringLiteral(", "))
                + QStringLiteral("] file=") + nativeFile);

    bool ok = false;

    // On Windows, UWP/Store apps (e.g. MSPaint) are NTFS reparse points that
    // QProcess::startDetached cannot launch. Detect them up front and skip
    // straight to QDesktopServices to avoid the slow QProcess failure/timeout.
    if (Platform::isStoreApp(executable)) {
        Logger::log(QStringLiteral("PreferencesModel::openInViewer(): Store app detected, using QDesktopServices"));
        if (!nativeFile.isEmpty()) {
            ok = QDesktopServices::openUrl(QUrl::fromLocalFile(nativeFile));
        } else {
            // No file to hand the shell; ask QDesktopServices to launch the exe directly.
            ok = QDesktopServices::openUrl(QUrl::fromLocalFile(executable));
        }
    } else {
        // Traditional desktop executable — launch directly via QProcess.
        ok = QProcess::startDetached(executable, args);
        if (!ok && !nativeFile.isEmpty()) {
            // Fallback: open the file via the OS shell, which handles edge cases
            // (e.g. file-type associations) that QProcess cannot launch.
            Logger::log(QStringLiteral("PreferencesModel::openInViewer(): QProcess failed, falling back to QDesktopServices"));
            ok = QDesktopServices::openUrl(QUrl::fromLocalFile(nativeFile));
        } // if QProcess failed and we have a file
    } // if Store app vs traditional exe

    if (!ok) {
        Logger::log(QStringLiteral("PreferencesModel::openInViewer(): failed to open file"));
    } // if all methods failed
    return ok;
} // openInViewer

// ---------------------------------------------------------------------------
// isViewerUrl
// ---------------------------------------------------------------------------

// @brief Check if a viewer path is an HTTP/HTTPS URL (online viewer).
// @param viewerPath The viewer path to check.
// @return true if the path starts with http:// or https://.
bool PreferencesModel::isViewerUrl(const QString &viewerPath)
{
    return isHttpUrl(viewerPath);
} // isViewerUrl

// ---------------------------------------------------------------------------
// fileExists
// ---------------------------------------------------------------------------

// @brief Return true if a file exists at the given absolute path.
// Uses QFileInfo::exists() which is cross-platform and handles both
// Windows and Unix path separators without requiring native conversion.
// @param path Absolute file path to test.
// @return true if the file exists and is accessible; false if missing or empty.
bool PreferencesModel::fileExists(const QString &path)
{
    if (path.isEmpty()) { return false; }
    const QFileInfo fi(path);
    return fi.exists() && fi.isFile();
} // fileExists

// ---------------------------------------------------------------------------
// dxfTeachingFilePath
// ---------------------------------------------------------------------------

// @brief Derive the companion teaching-file (.txt) path from a DXF-ASTM file path.
// The teaching file shares the same directory and base name as the .dxf file
// but carries a .txt extension.  It is generated optionally during DXF export
// when createTeachingVersion is true.
// Uses QFileInfo to extract directory and base name via the Qt path API so
// the derivation is correct on all supported platforms (Windows backslashes,
// Unix forward slashes).
// Example: "C:/output/jacket_front.dxf"  →  "C:/output/jacket_front.txt"
// @param dxfPath Absolute path to the .dxf file.
// @return Absolute path of the companion .txt teaching file, or empty if dxfPath is empty.
QString PreferencesModel::dxfTeachingFilePath(const QString &dxfPath)
{
    if (dxfPath.isEmpty()) {
        return QString();
    } // if empty

    // QFileInfo splits the path correctly on all platforms.
    // completeBaseName() strips only the last extension (e.g. ".dxf"),
    // so "jacket_front.dxf" → "jacket_front" and
    // "jacket.front.dxf"  → "jacket.front".
    const QFileInfo fi(dxfPath);
    const QString baseName = fi.completeBaseName();
    const QString dir      = fi.absolutePath();
    return QDir(dir).filePath(baseName + QStringLiteral(".txt"));
} // dxfTeachingFilePath

// ---------------------------------------------------------------------------
// parseViewerCommand
// ---------------------------------------------------------------------------

// @brief Parse a viewer-field string into (executable, preset args).
// See header for resolution order. Returns empty list on no-token input.
QStringList PreferencesModel::parseViewerCommand(const QString &viewerPath)
{
    QString viewerStr = viewerPath.trimmed();
    if (viewerStr.isEmpty()) {
        return QStringList();
    }

    // Step 1: strip a leading `file:///` URL prefix if present (FileDialog can
    // hand back URL-form paths and users may also paste one in).
    if (viewerStr.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        const QString asLocal = QUrl(viewerStr).toLocalFile();
        if (!asLocal.isEmpty()) {
            viewerStr = asLocal;
        }
    }

    // Step 2: fast path — whole string IS an existing file. Treat it as the
    // executable with no preset args. Handles Browse-picked Windows paths
    // with spaces (e.g. `C:\Program Files\Common Files\eDrawings2026\eDrawings.exe`)
    // without requiring quotes.
    if (QFileInfo::exists(viewerStr)) {
        return QStringList{ Platform::toNativePath(viewerStr) };
    }

    // Step 3: shell-style parse for quoted paths and wrapper commands (e.g.
    // Chrome PWA shortcut: `"C:\Program Files\Google\Chrome\Application\chrome_proxy.exe"
    // --profile-directory=Default --app-id=<id>`).
    QStringList tokens = QProcess::splitCommand(viewerStr);
    if (tokens.isEmpty()) {
        return QStringList();
    }
    QStringList out;
    out.append(Platform::toNativePath(tokens.takeFirst()));
    out.append(tokens);
    return out;
} // parseViewerCommand
