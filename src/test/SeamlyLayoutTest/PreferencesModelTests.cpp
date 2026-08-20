// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file PreferencesModelTests.cpp
// @brief Qt tests for PreferencesModel INI persistence, viewer helpers, and legacy migration.
//
// Covers URL-vs-file detection and command-line parsing for openInViewer,
// the Projector use case (launcher-only viewer with optional arguments),
// legacy folder-name migration (layout-settings → settings, layout-preferences
// → preferences) performed by PreferencesModel::load(), (Task 17) the
// AppImage-aware fallback for the default input/output directories, and (Task 18)
// the matching Flatpak-aware fallback.

#include "PreferencesModel.h"
#include "Platform.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest/QtTest>

// @brief Captures URLs dispatched via QDesktopServices::openUrl for deterministic testing.
class UrlCapture : public QObject
{
    Q_OBJECT
public:
    QList<QUrl> captured;
public slots:
    void handle(const QUrl &url) { captured.append(url); }
};

// @brief RAII helper that sets an environment variable for the scope of a test and
// guarantees it is unset again when the test function returns.
//
// Task 17: PreferencesModelTests exercises Platform::isAppImage() (checks the APPIMAGE
// environment variable the AppImage runtime sets) by setting the variable directly. A
// failed QVERIFY/QCOMPARE inside the test body simply `return`s the enclosing function —
// it does not throw — so a stack-allocated guard's destructor still runs and the variable
// never leaks into whichever test runs next in the same process.
class ScopedEnvVar
{
public:
    ScopedEnvVar(const QByteArray &name, const QByteArray &value) : m_name(name)
    {
        qputenv(m_name.constData(), value);
    } // ScopedEnvVar
    ~ScopedEnvVar() { qunsetenv(m_name.constData()); } // ~ScopedEnvVar
private:
    QByteArray m_name;
}; // class ScopedEnvVar

class PreferencesModelTests : public QObject
{
    Q_OBJECT

private slots:
    void isViewerUrl_detectsHttp();
    void isViewerUrl_detectsHttps();
    void isViewerUrl_rejectsLocalPath();
    void isViewerUrl_rejectsEmpty();
    void isViewerUrl_rejectsLocalCommandWithArgs();

    void openInViewer_emptyViewer_returnsFalse();
    void openInViewer_emptyTokens_returnsFalse();

    void defaultPreferencesFilePath_usesAppConfigIni();
    void legacyJson_migratesToIni();
    void projectorPath_roundTripsThroughIni();
    void projectorPath_emitsSignalOnChange();

    void parseViewerCommand_emptyReturnsEmpty();
    void parseViewerCommand_existingFileWithSpaces_singleToken();
    void parseViewerCommand_fileUrlPrefix_strippedAndResolved();
    void parseViewerCommand_quotedChromePwa_splitsExecAndArgs();
    void parseViewerCommand_missingPathFallsBackToSplit();

    void resetToDefaults_loadsValuesFromDefaultsFile();

    // -----------------------------------------------------------------------
    // Legacy folder-name migration: layout-settings → settings,
    //                               layout-preferences → preferences
    // -----------------------------------------------------------------------
    void migration_legacySettingsFolder_updatesSettingsDirectory();
    void migration_legacyPreferencesFolder_updatesPreferencesDirectory();
    void migration_legacySettingsFolder_updatesSettingsFilePath();
    void migration_legacyPreferencesFolder_updatesPreferencesFilePath();
    void migration_noLegacyFolder_leavesPathUnchanged();
    void migration_settingsFileCopied_toNewFolder();

    // -----------------------------------------------------------------------
    // E.7 — DXF viewer is View-menu-only; no auto-open after export.
    // These tests document and guard the policy at the PreferencesModel level.
    // -----------------------------------------------------------------------
    void e7_dxfViewerPath_defaultsToEmpty();
    void e7_dxfViewerPath_roundTripsThroughIni();
    void e7_openInViewer_dxfFile_withViewer_returnsTrue();

    // -----------------------------------------------------------------------
    // V.1 — resolvedLayoutDirectory is the first step in every View handler.
    // -----------------------------------------------------------------------
    void v1_resolvedLayoutDirectory_returnsAbsoluteLayoutDir();
    void v1_resolvedLayoutDirectory_fallsBackWhenEmpty();
    void v1_resolvedLayoutDirectory_createsDirectoryIfMissing();

    // -----------------------------------------------------------------------
    // V.2 — teaching-file detection helpers used in View → DXF-ASTM.
    // fileExists() guards the prompt: only shown when the companion .txt exists.
    // dxfTeachingFilePath() derives the expected companion path from the .dxf path.
    // -----------------------------------------------------------------------
    void v2_fileExists_trueForExistingFile();
    void v2_fileExists_falseForMissingFile();
    void v2_fileExists_falseForEmptyPath();
    void v2_dxfTeachingFilePath_replacesExtension();
    void v2_dxfTeachingFilePath_preservesDirectoryAndBaseName();
    void v2_dxfTeachingFilePath_handlesUppercaseExtension();
    void v2_dxfTeachingFilePath_handlesMultipleDots();
    void v2_dxfTeachingFilePath_emptyInputReturnsEmpty();
    void v2_fileExists_trueForActualTeachingFile();

    // -----------------------------------------------------------------------
    // Task 17 — Platform::isAppImage() and its AppImage-aware directory fallbacks.
    // A mounted Linux AppImage is read-only, just like a signed macOS .app bundle
    // (Task 16), so the default input/output directories must also fall back to the
    // writable AppConfigLocation root there instead of <exeDir>/input or /output.
    // -----------------------------------------------------------------------
    void platform_isAppImage_falseWithoutEnvVar();
    void platform_isAppImage_trueWithEnvVarSet();
    void appImage_resolvedInputDirectory_fallsBackToAppConfigLocation();
    void appImage_resolvedLayoutDirectory_fallsBackToAppConfigLocation();
    void appImage_defaultInputFolderUrl_fallsBackToAppConfigLocation();

    // -----------------------------------------------------------------------
    // Task 18 — Platform::isFlatpak() and its Flatpak-aware directory fallbacks.
    // A Flatpak sandbox mounts the /app prefix read-only, just like a mounted
    // AppImage (Task 17) or a signed macOS .app bundle (Task 16), so the default
    // input/output directories must fall back to the writable AppConfigLocation
    // root (which Flatpak maps into the sandbox) instead of <exeDir>/input or /output.
    // -----------------------------------------------------------------------
    void platform_isFlatpak_falseWithoutEnvVar();
    void platform_isFlatpak_trueWithEnvVarSet();
    void flatpak_resolvedInputDirectory_fallsBackToAppConfigLocation();
    void flatpak_resolvedLayoutDirectory_fallsBackToAppConfigLocation();
    void flatpak_defaultInputFolderUrl_fallsBackToAppConfigLocation();
}; // class PreferencesModelTests

void PreferencesModelTests::isViewerUrl_detectsHttp()
{
    QVERIFY(PreferencesModel::isViewerUrl(QStringLiteral("http://example.com")));
}

void PreferencesModelTests::isViewerUrl_detectsHttps()
{
    QVERIFY(PreferencesModel::isViewerUrl(QStringLiteral("https://sharecad.org")));
    QVERIFY(PreferencesModel::isViewerUrl(QStringLiteral("https://patternprojector.com")));
}

void PreferencesModelTests::isViewerUrl_rejectsLocalPath()
{
    QVERIFY(!PreferencesModel::isViewerUrl(QStringLiteral("C:/Program Files/Foo/foo.exe")));
    QVERIFY(!PreferencesModel::isViewerUrl(QStringLiteral("/usr/bin/eog")));
    QVERIFY(!PreferencesModel::isViewerUrl(QStringLiteral("foo.exe")));
}

void PreferencesModelTests::isViewerUrl_rejectsEmpty()
{
    QVERIFY(!PreferencesModel::isViewerUrl(QString()));
}

void PreferencesModelTests::isViewerUrl_rejectsLocalCommandWithArgs()
{
    // Chrome PWA shortcut form — local executable with args, not a URL.
    const QString cmd = QStringLiteral(
        "\"C:\\Program Files\\Google\\Chrome\\Application\\chrome_proxy.exe\" "
        "--profile-directory=Default --app-id=mecdgiabjihcockhgeepcijbehknlmoc");
    QVERIFY(!PreferencesModel::isViewerUrl(cmd));
}

void PreferencesModelTests::openInViewer_emptyViewer_returnsFalse()
{
    QCOMPARE(PreferencesModel::openInViewer(QString(), QStringLiteral("/tmp/foo.dxf")), false);
    QCOMPARE(PreferencesModel::openInViewer(QString(), QString()), false);
}

void PreferencesModelTests::openInViewer_emptyTokens_returnsFalse()
{
    // Whitespace-only viewer string parses to zero tokens — must reject without launching.
    QCOMPARE(PreferencesModel::openInViewer(QStringLiteral("   "), QString()), false);
}

// @brief The application preferences file is stored directly in AppConfigLocation.
void PreferencesModelTests::defaultPreferencesFilePath_usesAppConfigIni()
{
    PreferencesModel model;
    const QString expectedRoot = QFileInfo(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).absoluteFilePath();
    const QFileInfo actual(model.defaultPreferencesFilePath());

    QCOMPARE(actual.fileName(), QStringLiteral("qt6_seamlylayout.ini"));
    QCOMPARE(actual.absolutePath(), expectedRoot);
} // defaultPreferencesFilePath_usesAppConfigIni

// @brief A legacy preferences JSON file is imported into the application INI file.
void PreferencesModelTests::legacyJson_migratesToIni()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString legacyDir = tempDir.filePath(QStringLiteral("preferences"));
    QVERIFY(QDir().mkpath(legacyDir));
    const QString legacyPath = QDir(legacyDir).filePath(QStringLiteral("preferences.json"));
    QFile legacyFile(legacyPath);
    QVERIFY(legacyFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    legacyFile.write(R"({"projector_path":"https://patternprojector.com"})");
    legacyFile.close();

    const QString iniPath = tempDir.filePath(QStringLiteral("qt6_seamlylayout.ini"));
    PreferencesModel model;
    QVERIFY(model.load(iniPath));
    QCOMPARE(model.projectorPath(), QStringLiteral("https://patternprojector.com"));
    QVERIFY(QFileInfo::exists(iniPath));

    const QSettings settings(iniPath, QSettings::IniFormat);
    QCOMPARE(settings.value(QStringLiteral("projector_path")).toString(),
             QStringLiteral("https://patternprojector.com"));
    QVERIFY(QFileInfo::exists(legacyPath));
} // legacyJson_migratesToIni

void PreferencesModelTests::projectorPath_roundTripsThroughIni()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("preferences.ini"));

    PreferencesModel writer;
    const QString chromeShortcut = QStringLiteral(
        "\"C:\\Program Files\\Google\\Chrome\\Application\\chrome_proxy.exe\" "
        "--profile-directory=Default --app-id=mecdgiabjihcockhgeepcijbehknlmoc");
    writer.setProjectorPath(chromeShortcut);
    QVERIFY(writer.save(path));

    PreferencesModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.projectorPath(), chromeShortcut);

    // Also verify URL form round-trips.
    writer.setProjectorPath(QStringLiteral("https://patternprojector.com"));
    QVERIFY(writer.save(path));
    PreferencesModel reader2;
    QVERIFY(reader2.load(path));
    QCOMPARE(reader2.projectorPath(), QStringLiteral("https://patternprojector.com"));
}

void PreferencesModelTests::projectorPath_emitsSignalOnChange()
{
    PreferencesModel m;
    QSignalSpy spy(&m, &PreferencesModel::projectorPathChanged);
    QVERIFY(spy.isValid());

    m.setProjectorPath(QStringLiteral("https://patternprojector.com"));
    QCOMPARE(spy.count(), 1);

    // Same value does not re-emit.
    m.setProjectorPath(QStringLiteral("https://patternprojector.com"));
    QCOMPARE(spy.count(), 1);

    // Distinct value emits again.
    m.setProjectorPath(QStringLiteral("http://other.example/"));
    QCOMPARE(spy.count(), 2);
}

// ---------------------------------------------------------------------------
// parseViewerCommand
// ---------------------------------------------------------------------------

void PreferencesModelTests::parseViewerCommand_emptyReturnsEmpty()
{
    QCOMPARE(PreferencesModel::parseViewerCommand(QString()).isEmpty(), true);
    QCOMPARE(PreferencesModel::parseViewerCommand(QStringLiteral("   ")).isEmpty(), true);
}

void PreferencesModelTests::parseViewerCommand_existingFileWithSpaces_singleToken()
{
    // Regression: Browse-picked Windows path with unquoted spaces was being
    // split by QProcess::splitCommand on the space inside "Program Files".
    // Now the whole string, when it points at an existing file, is treated
    // as the executable with no preset args.
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Construct a directory name containing a space so the path needs the
    // whole-string-exists branch.
    QDir d(tempDir.path());
    QVERIFY(d.mkdir(QStringLiteral("Program Files")));
    const QString exePath = QDir(tempDir.filePath(QStringLiteral("Program Files")))
        .filePath(QStringLiteral("eDrawings.exe"));
    QFile f(exePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("stub");
    f.close();
    QVERIFY(QFileInfo::exists(exePath));

    const QStringList parsed = PreferencesModel::parseViewerCommand(exePath);
    QCOMPARE(parsed.size(), 1);
    QCOMPARE(QFileInfo(parsed.at(0)).canonicalFilePath(),
             QFileInfo(exePath).canonicalFilePath());
}

void PreferencesModelTests::parseViewerCommand_fileUrlPrefix_strippedAndResolved()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString exePath = tempDir.filePath(QStringLiteral("viewer.exe"));
    QFile f(exePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("stub");
    f.close();

    const QString fileUrl = QUrl::fromLocalFile(exePath).toString();
    QVERIFY(fileUrl.startsWith(QStringLiteral("file://")));

    const QStringList parsed = PreferencesModel::parseViewerCommand(fileUrl);
    QCOMPARE(parsed.size(), 1);
    QCOMPARE(QFileInfo(parsed.at(0)).canonicalFilePath(),
             QFileInfo(exePath).canonicalFilePath());
}

void PreferencesModelTests::parseViewerCommand_quotedChromePwa_splitsExecAndArgs()
{
    // Chrome PWA shortcut form — quoted path (which need not exist on the
    // test machine) followed by preset args. splitCommand fallback kicks in.
    const QString cmd = QStringLiteral(
        "\"C:\\Program Files\\Google\\Chrome\\Application\\chrome_proxy.exe\" "
        "--profile-directory=Default --app-id=mecdgiabjihcockhgeepcijbehknlmoc");

    const QStringList parsed = PreferencesModel::parseViewerCommand(cmd);
    QCOMPARE(parsed.size(), 3);
    QVERIFY(parsed.at(0).contains(QStringLiteral("chrome_proxy.exe")));
    QCOMPARE(parsed.at(1), QStringLiteral("--profile-directory=Default"));
    QCOMPARE(parsed.at(2), QStringLiteral("--app-id=mecdgiabjihcockhgeepcijbehknlmoc"));
}

void PreferencesModelTests::parseViewerCommand_missingPathFallsBackToSplit()
{
    // Path that does NOT exist on disk and has no quotes: splitCommand will
    // break it on spaces (this is the documented behaviour when there is no
    // existing file to anchor on; users should quote in that case).
    const QString cmd = QStringLiteral("C:\\does\\not\\exist\\foo.exe --flag");
    const QStringList parsed = PreferencesModel::parseViewerCommand(cmd);
    QCOMPARE(parsed.size(), 2);
    QVERIFY(parsed.at(0).endsWith(QStringLiteral("foo.exe")));
    QCOMPARE(parsed.at(1), QStringLiteral("--flag"));
}

// ---------------------------------------------------------------------------
// resetToDefaults
// ---------------------------------------------------------------------------

void PreferencesModelTests::resetToDefaults_loadsValuesFromDefaultsFile()
{
    // Arrange: write a known defaults file to a temp dir, point the model's
    // preferencesDirectory/preferencesFile at it, then mutate the in-memory
    // values away from those defaults.
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString defaultsPath = tempDir.filePath(QStringLiteral("prefs.json"));
    QJsonObject defaults;
    defaults[QStringLiteral("input_directory")]   = QStringLiteral("/tmp/in");
    defaults[QStringLiteral("layout_directory")]  = QStringLiteral("/tmp/out");
    defaults[QStringLiteral("dxf_viewer_path")]   = QStringLiteral("https://sharecad.org");
    defaults[QStringLiteral("projector_path")]    = QStringLiteral("https://patternprojector.com");

    QFile f(defaultsPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(defaults).toJson(QJsonDocument::Indented));
    f.close();

    PreferencesModel m;
    m.setPreferencesDirectory(tempDir.path());
    m.setPreferencesFile(QStringLiteral("prefs.json"));

    // Pollute current values so we know reset actually overwrote them.
    m.setInputDirectory(QStringLiteral("/somewhere/else"));
    m.setDxfViewerPath(QStringLiteral("not-the-default"));
    m.setProjectorPath(QStringLiteral("not-the-default-projector"));

    // Act
    QVERIFY(m.resetToDefaults());

    // Assert: each scalar field now matches the defaults file.
    QCOMPARE(m.inputDirectory(),  QStringLiteral("/tmp/in"));
    QCOMPARE(m.layoutDirectory(), QStringLiteral("/tmp/out"));
    QCOMPARE(m.dxfViewerPath(),   QStringLiteral("https://sharecad.org"));
    QCOMPARE(m.projectorPath(),   QStringLiteral("https://patternprojector.com"));
}

// ---------------------------------------------------------------------------
// Legacy folder-name migration tests
//
// PreferencesModel::load() migrates any persisted path whose directory
// segment still uses the old names:
//   "layout-settings"    → "settings"
//   "layout-preferences" → "preferences"
//
// These tests write a preferences JSON file with legacy directory names,
// call load(), and verify the in-memory values are updated to the new names.
// ---------------------------------------------------------------------------

// @brief Verify that load() rewrites a legacy "layout-settings" directory to "settings".
void PreferencesModelTests::migration_legacySettingsFolder_updatesSettingsDirectory()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Build legacy and new paths under a shared parent.
    const QString parent = tempDir.path();
    const QString legacyDir = QDir(parent).filePath(QStringLiteral("layout-settings"));
    const QString newDir    = QDir(parent).filePath(QStringLiteral("settings"));
    QDir().mkpath(legacyDir);

    // Write a preferences JSON that records the legacy directory name.
    const QString prefsPath = tempDir.filePath(QStringLiteral("prefs.json"));
    QJsonObject obj;
    obj[QStringLiteral("settings_directory")] = legacyDir;
    QFile f(prefsPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    PreferencesModel m;
    QVERIFY(m.load(prefsPath));

    // The in-memory setting must now point at the renamed folder.
    QCOMPARE(m.settingsDirectory(), newDir);
}

// @brief Verify that load() rewrites a legacy "layout-preferences" directory to "preferences".
void PreferencesModelTests::migration_legacyPreferencesFolder_updatesPreferencesDirectory()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString parent    = tempDir.path();
    const QString legacyDir = QDir(parent).filePath(QStringLiteral("layout-preferences"));
    const QString newDir    = QDir(parent).filePath(QStringLiteral("preferences"));
    QDir().mkpath(legacyDir);

    const QString prefsPath = tempDir.filePath(QStringLiteral("prefs.json"));
    QJsonObject obj;
    obj[QStringLiteral("preferences_directory")] = legacyDir;
    QFile f(prefsPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    PreferencesModel m;
    QVERIFY(m.load(prefsPath));

    QCOMPARE(m.preferencesDirectory(), newDir);
}

// @brief Verify that a settings file path under "layout-settings" is migrated to "settings".
void PreferencesModelTests::migration_legacySettingsFolder_updatesSettingsFilePath()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString parent    = tempDir.path();
    const QString legacyDir = QDir(parent).filePath(QStringLiteral("layout-settings"));
    QDir().mkpath(legacyDir);
    const QString legacyFile = QDir(legacyDir).filePath(QStringLiteral("my.json"));
    const QString newFile    = QDir(parent).filePath(QStringLiteral("settings/my.json"));

    // Create the legacy file so copyIfMissing has something to copy.
    QFile src(legacyFile);
    QVERIFY(src.open(QIODevice::WriteOnly));
    src.write("{}");
    src.close();

    const QString prefsPath = tempDir.filePath(QStringLiteral("prefs.json"));
    QJsonObject obj;
    obj[QStringLiteral("settings_file")] = legacyFile;
    QFile f(prefsPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    PreferencesModel m;
    QVERIFY(m.load(prefsPath));

    QCOMPARE(m.settingsFile(), newFile);
}

// @brief Verify that a preferences file path under "layout-preferences" is migrated.
void PreferencesModelTests::migration_legacyPreferencesFolder_updatesPreferencesFilePath()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString parent    = tempDir.path();
    const QString legacyDir = QDir(parent).filePath(QStringLiteral("layout-preferences"));
    QDir().mkpath(legacyDir);
    const QString legacyFile = QDir(legacyDir).filePath(QStringLiteral("prefs_backup.json"));
    const QString newFile    = QDir(parent).filePath(QStringLiteral("preferences/prefs_backup.json"));

    QFile src(legacyFile);
    QVERIFY(src.open(QIODevice::WriteOnly));
    src.write("{}");
    src.close();

    const QString prefsPath = tempDir.filePath(QStringLiteral("prefs.json"));
    QJsonObject obj;
    obj[QStringLiteral("preferences_file")] = legacyFile;
    QFile f(prefsPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    PreferencesModel m;
    QVERIFY(m.load(prefsPath));

    QCOMPARE(m.preferencesFile(), newFile);
}

// @brief Verify that paths not containing legacy folder names are left unchanged.
void PreferencesModelTests::migration_noLegacyFolder_leavesPathUnchanged()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Canonical folder names — must not be touched by migration.
    const QString canonicalSettingsDir = QDir(tempDir.path()).filePath(QStringLiteral("settings"));
    const QString canonicalPrefsDir    = QDir(tempDir.path()).filePath(QStringLiteral("preferences"));
    QDir().mkpath(canonicalSettingsDir);
    QDir().mkpath(canonicalPrefsDir);

    const QString prefsPath = tempDir.filePath(QStringLiteral("prefs.json"));
    QJsonObject obj;
    obj[QStringLiteral("settings_directory")]    = canonicalSettingsDir;
    obj[QStringLiteral("preferences_directory")] = canonicalPrefsDir;
    QFile f(prefsPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    PreferencesModel m;
    QVERIFY(m.load(prefsPath));

    QCOMPARE(m.settingsDirectory(),    canonicalSettingsDir);
    QCOMPARE(m.preferencesDirectory(), canonicalPrefsDir);
}

// @brief Verify that migration copies an existing legacy file to the new folder.
void PreferencesModelTests::migration_settingsFileCopied_toNewFolder()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString parent    = tempDir.path();
    const QString legacyDir = QDir(parent).filePath(QStringLiteral("layout-settings"));
    QDir().mkpath(legacyDir);
    const QString legacyFile  = QDir(legacyDir).filePath(QStringLiteral("default_settings.json"));
    const QString migratedFile = QDir(parent).filePath(QStringLiteral("settings/default_settings.json"));

    // Write a recognisable payload in the legacy file.
    QFile src(legacyFile);
    QVERIFY(src.open(QIODevice::WriteOnly));
    src.write(R"({"unit":"in"})");
    src.close();

    const QString prefsPath = tempDir.filePath(QStringLiteral("prefs.json"));
    QJsonObject obj;
    obj[QStringLiteral("settings_directory")] = legacyDir;
    obj[QStringLiteral("settings_file")]      = legacyFile;
    QFile f(prefsPath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
    f.write(QJsonDocument(obj).toJson());
    f.close();

    PreferencesModel m;
    QVERIFY(m.load(prefsPath));

    // The migrated copy must now exist in the new folder.
    QVERIFY(QFileInfo::exists(migratedFile));
}

// ---------------------------------------------------------------------------
// E.7 — DXF viewer is View-menu-only; no auto-open after export.
// ---------------------------------------------------------------------------

// @brief A freshly constructed PreferencesModel (before loading persisted preferences) must have an empty dxfViewerPath.
//
// Note: at runtime Main.qml loads defaultPreferencesFilePath(), which may seed defaults from
// qt_frontend/preferences/default_preferences.json; that file controls the initial dxf_viewer_path value.
// Keep this test/comment aligned with that defaults profile and the intended UX policy.
void PreferencesModelTests::e7_dxfViewerPath_defaultsToEmpty()
{
    PreferencesModel m;
    QVERIFY(m.dxfViewerPath().isEmpty());
}

// @brief dxfViewerPath round-trips through INI correctly for the View menu.
//
// The View → DXF-ASTM path reads dxfViewerPath and passes it to openInViewer.
// If the path is lost on save/load the viewer cannot be launched.
void PreferencesModelTests::e7_dxfViewerPath_roundTripsThroughIni()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("preferences.ini"));

    PreferencesModel writer;
    const QString viewerPath = QStringLiteral("https://sharecad.org");
    writer.setDxfViewerPath(viewerPath);
    QVERIFY(writer.save(path));

    PreferencesModel reader;
    QVERIFY(reader.load(path));
    QCOMPARE(reader.dxfViewerPath(), viewerPath);
}

// @brief openInViewer with a configured URL dxfViewerPath dispatches to the URL handler.
//
// E.7 policy: when dxfViewerPath is a URL, openInViewer must route to
// QDesktopServices::openUrl with the URL as-is (not appending the DXF file path,
// because online viewers like ShareCAD require manual upload). Uses a temporary
// QDesktopServices URL handler so the test is fully deterministic — no real
// browser is opened and no system URL handler is required.
void PreferencesModelTests::e7_openInViewer_dxfFile_withViewer_returnsTrue()
{
    const QString dxfFile = QStringLiteral("/tmp/layout_output.dxf");
    const QString urlViewer = QStringLiteral("https://sharecad.org");

    QVERIFY(PreferencesModel::isViewerUrl(urlViewer));

    // Install a capture handler so openInViewer's QDesktopServices::openUrl call
    // is intercepted; this avoids opening a real browser and makes the test
    // deterministic in CI and on developer machines.
    UrlCapture capture;
    QDesktopServices::setUrlHandler(QStringLiteral("https"), &capture, "handle");

    const bool launched = PreferencesModel::openInViewer(urlViewer, dxfFile);

    QDesktopServices::unsetUrlHandler(QStringLiteral("https"));

    // Verify URL-dispatch branch was taken and the DXF file path was NOT appended
    // (online viewers require manual upload — the URL is opened as-is).
    QVERIFY(launched);
    QCOMPARE(capture.captured.size(), 1);
    QCOMPARE(capture.captured.at(0), QUrl(urlViewer));
}

// ---------------------------------------------------------------------------
// V.1 — resolvedLayoutDirectory tests
//
// resolvedLayoutDirectory() is the first step in each View dropdown handler:
// it determines the directory opened by the file-picker dialog.
// ---------------------------------------------------------------------------

// @brief When layoutDirectory is an absolute path that already exists,
// resolvedLayoutDirectory returns that canonical path.
void PreferencesModelTests::v1_resolvedLayoutDirectory_returnsAbsoluteLayoutDir()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    PreferencesModel m;
    m.setLayoutDirectory(tempDir.path());

    const QString resolved = m.resolvedLayoutDirectory();

    QCOMPARE(QFileInfo(resolved).canonicalFilePath(),
             QFileInfo(tempDir.path()).canonicalFilePath());
}

// @brief When layoutDirectory is empty, resolvedLayoutDirectory returns the
// fallback path (<exeDir>/output), which must be non-empty.
void PreferencesModelTests::v1_resolvedLayoutDirectory_fallsBackWhenEmpty()
{
    PreferencesModel m;
    // Default: m_layoutDirectory is empty — fallback kicks in.
    QVERIFY(!m.resolvedLayoutDirectory().isEmpty());
}

// @brief resolvedLayoutDirectory creates the target directory when it does
// not yet exist, so the file-picker always opens in a valid directory.
void PreferencesModelTests::v1_resolvedLayoutDirectory_createsDirectoryIfMissing()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString newDir = QDir(tempDir.path()).filePath(QStringLiteral("view_output"));
    QVERIFY(!QFileInfo::exists(newDir));

    PreferencesModel m;
    m.setLayoutDirectory(newDir);
    const QString resolved = m.resolvedLayoutDirectory();

    QVERIFY(QFileInfo::exists(resolved));
    QVERIFY(QFileInfo(resolved).isDir());
}

// ---------------------------------------------------------------------------
// V.2 — fileExists and dxfTeachingFilePath tests
//
// These helpers drive the View → DXF-ASTM teaching-file prompt:
//   1. dxfTeachingFilePath() derives the companion .txt path from the .dxf path.
//   2. fileExists() checks whether that .txt is present so the prompt is only
//      shown when a teaching file was actually generated during export.
// ---------------------------------------------------------------------------

// @brief fileExists returns true when the file is present on disk.
void PreferencesModelTests::v2_fileExists_trueForExistingFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("layout.txt"));

    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("teaching file content");
    f.close();

    QVERIFY(PreferencesModel::fileExists(path));
}

// @brief fileExists returns false when the file does not exist.
void PreferencesModelTests::v2_fileExists_falseForMissingFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString path = tempDir.filePath(QStringLiteral("does_not_exist.txt"));
    QVERIFY(!QFileInfo::exists(path)); // precondition

    QVERIFY(!PreferencesModel::fileExists(path));
}

// @brief fileExists returns false for an empty path string without crashing.
void PreferencesModelTests::v2_fileExists_falseForEmptyPath()
{
    QVERIFY(!PreferencesModel::fileExists(QString()));
    QVERIFY(!PreferencesModel::fileExists(QStringLiteral("")));
}

// @brief dxfTeachingFilePath replaces the .dxf extension with .txt.
void PreferencesModelTests::v2_dxfTeachingFilePath_replacesExtension()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dxfPath = tempDir.filePath(QStringLiteral("layout.dxf"));
    const QString expected = tempDir.filePath(QStringLiteral("layout.txt"));

    const QString result = PreferencesModel::dxfTeachingFilePath(dxfPath);

    // Compare absolute paths so platform-specific separator differences are ignored.
    QCOMPARE(QFileInfo(result).absoluteFilePath(),
             QFileInfo(expected).absoluteFilePath());
}

// @brief dxfTeachingFilePath preserves the directory and base name exactly.
void PreferencesModelTests::v2_dxfTeachingFilePath_preservesDirectoryAndBaseName()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dxfPath = tempDir.filePath(QStringLiteral("jacket_front.dxf"));

    const QString result = PreferencesModel::dxfTeachingFilePath(dxfPath);
    const QFileInfo fi(result);

    QCOMPARE(fi.suffix(), QStringLiteral("txt"));
    QCOMPARE(fi.completeBaseName(), QStringLiteral("jacket_front"));
    QCOMPARE(QFileInfo(fi.absolutePath()).absoluteFilePath(),
             QFileInfo(tempDir.path()).absoluteFilePath());
}

// @brief dxfTeachingFilePath handles an uppercase .DXF extension by replacing it with .txt.
// QFileInfo::completeBaseName() strips the last extension regardless of case,
// so "layout.DXF" → base "layout" → "layout.txt".
void PreferencesModelTests::v2_dxfTeachingFilePath_handlesUppercaseExtension()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString dxfPath = tempDir.filePath(QStringLiteral("layout.DXF"));
    const QString expected = tempDir.filePath(QStringLiteral("layout.txt"));

    const QString result = PreferencesModel::dxfTeachingFilePath(dxfPath);

    QCOMPARE(QFileInfo(result).absoluteFilePath(),
             QFileInfo(expected).absoluteFilePath());
}

// @brief dxfTeachingFilePath handles a base name containing dots correctly.
// Only the last extension (.dxf) is replaced; dots in the base name are preserved.
void PreferencesModelTests::v2_dxfTeachingFilePath_handlesMultipleDots()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    // "jacket.front.v2.dxf" → base "jacket.front.v2" → "jacket.front.v2.txt"
    const QString dxfPath = tempDir.filePath(QStringLiteral("jacket.front.v2.dxf"));
    const QString expected = tempDir.filePath(QStringLiteral("jacket.front.v2.txt"));

    const QString result = PreferencesModel::dxfTeachingFilePath(dxfPath);

    QCOMPARE(QFileInfo(result).absoluteFilePath(),
             QFileInfo(expected).absoluteFilePath());
}

// @brief dxfTeachingFilePath returns empty when given an empty input.
void PreferencesModelTests::v2_dxfTeachingFilePath_emptyInputReturnsEmpty()
{
    QVERIFY(PreferencesModel::dxfTeachingFilePath(QString()).isEmpty());
    QVERIFY(PreferencesModel::dxfTeachingFilePath(QStringLiteral("")).isEmpty());
}

// @brief Integration: dxfTeachingFilePath + fileExists together detect an actual teaching file.
// This mirrors the exact runtime sequence in onViewDxfAstmRequested: after the user picks
// a .dxf file, derive the teaching path and check whether it exists before opening the prompt.
void PreferencesModelTests::v2_fileExists_trueForActualTeachingFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    // Simulate a DXF and its companion teaching file written by DXF export.
    const QString dxfPath      = tempDir.filePath(QStringLiteral("layout_output.dxf"));
    const QString teachingPath = tempDir.filePath(QStringLiteral("layout_output.txt"));

    // Create stub DXF (content irrelevant for this test).
    QFile dxf(dxfPath);
    QVERIFY(dxf.open(QIODevice::WriteOnly));
    dxf.write("0\nSECTION\n");
    dxf.close();

    // Create stub teaching file.
    QFile txt(teachingPath);
    QVERIFY(txt.open(QIODevice::WriteOnly));
    txt.write("teaching content");
    txt.close();

    // Derive the teaching path from the DXF path (same as runtime flow).
    const QString derived = PreferencesModel::dxfTeachingFilePath(dxfPath);

    // Derived path must resolve to the actual teaching file.
    QCOMPARE(QFileInfo(derived).absoluteFilePath(),
             QFileInfo(teachingPath).absoluteFilePath());

    // fileExists must return true — prompt will be shown.
    QVERIFY(PreferencesModel::fileExists(derived));
}

// ---------------------------------------------------------------------------
// Task 17 — Platform::isAppImage()
// ---------------------------------------------------------------------------

// @brief Without APPIMAGE set, isAppImage() must report false — the state for a normal
// Windows/macOS run and for a non-AppImage Linux install.
void PreferencesModelTests::platform_isAppImage_falseWithoutEnvVar()
{
    qunsetenv("APPIMAGE"); // guard against a value leaked from another process/test run
    QVERIFY(!Platform::isAppImage());
}

// @brief With APPIMAGE set — as the AppImage runtime sets it in every process it execs —
// isAppImage() must report true.
void PreferencesModelTests::platform_isAppImage_trueWithEnvVarSet()
{
    ScopedEnvVar appImageEnv("APPIMAGE", "/tmp/.mount_SeamlyXXXXXX/SeamlyLayout.AppImage");
    QVERIFY(Platform::isAppImage());
}

// ---------------------------------------------------------------------------
// Task 17 — AppImage-aware directory fallbacks
//
// Each test independently reconstructs the writable AppConfigLocation root the
// production code falls back to. appConfigRootPath() is a private implementation
// detail of PreferencesModel.cpp, but with no legacy "Seamly Systems" data present for
// this test binary's organization/application name, it reduces to exactly
// QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).absolutePath().
// ---------------------------------------------------------------------------

// @brief resolvedInputDirectory() with no configured inputDirectory normally falls back to
// <exeDir>/input; inside a (simulated) mounted AppImage it must fall back to the writable
// AppConfigLocation root instead, since the AppImage's exeDir is read-only.
void PreferencesModelTests::appImage_resolvedInputDirectory_fallsBackToAppConfigLocation()
{
    const QString appConfigRoot = QDir(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).absolutePath();
    const QString expected = QDir(appConfigRoot).filePath(QStringLiteral("input"));

    ScopedEnvVar appImageEnv("APPIMAGE", "/tmp/.mount_SeamlyXXXXXX/SeamlyLayout.AppImage");

    PreferencesModel m; // m_inputDirectory defaults to empty — the fallback branch runs
    const QString resolved = m.resolvedInputDirectory();

    QCOMPARE(QFileInfo(resolved).absoluteFilePath(), QFileInfo(expected).absoluteFilePath());
    QVERIFY(!resolved.startsWith(QCoreApplication::applicationDirPath()));
}

// @brief resolvedLayoutDirectory() with no configured layoutDirectory must likewise fall
// back to the AppConfigLocation root (not <exeDir>/output) inside a mounted AppImage.
void PreferencesModelTests::appImage_resolvedLayoutDirectory_fallsBackToAppConfigLocation()
{
    const QString appConfigRoot = QDir(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).absolutePath();
    const QString expected = QDir(appConfigRoot).filePath(QStringLiteral("output"));

    ScopedEnvVar appImageEnv("APPIMAGE", "/tmp/.mount_SeamlyXXXXXX/SeamlyLayout.AppImage");

    PreferencesModel m; // m_layoutDirectory defaults to empty — the fallback branch runs
    const QString resolved = m.resolvedLayoutDirectory();

    QCOMPARE(QFileInfo(resolved).absoluteFilePath(), QFileInfo(expected).absoluteFilePath());
    QVERIFY(!resolved.startsWith(QCoreApplication::applicationDirPath()));
}

// @brief defaultInputFolderUrl() (the FileDialog default before any input directory has
// ever been configured) must also resolve under the AppConfigLocation root rather than
// <exeDir>/input when running inside a mounted AppImage.
void PreferencesModelTests::appImage_defaultInputFolderUrl_fallsBackToAppConfigLocation()
{
    const QString appConfigRoot = QDir(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).absolutePath();
    const QString expectedDir = QDir(appConfigRoot).filePath(QStringLiteral("input"));

    ScopedEnvVar appImageEnv("APPIMAGE", "/tmp/.mount_SeamlyXXXXXX/SeamlyLayout.AppImage");

    const QString resolvedDir = QUrl(PreferencesModel::defaultInputFolderUrl()).toLocalFile();

    QCOMPARE(QFileInfo(resolvedDir).absoluteFilePath(), QFileInfo(expectedDir).absoluteFilePath());
}

// ---------------------------------------------------------------------------
// Task 18 — Platform::isFlatpak()
// ---------------------------------------------------------------------------

// @brief Without FLATPAK_ID set (and no /.flatpak-info on the host), isFlatpak() must report
// false — the state for a normal Windows/macOS run and for a non-Flatpak Linux install.
void PreferencesModelTests::platform_isFlatpak_falseWithoutEnvVar()
{
    qunsetenv("FLATPAK_ID"); // guard against a value leaked from another process/test run
    // The test binary is never itself run inside a Flatpak sandbox, so /.flatpak-info is
    // absent and the detection reduces to the (now-unset) environment variable.
    QVERIFY(!Platform::isFlatpak());
}

// @brief With FLATPAK_ID set — as the Flatpak runtime exports it to the app process —
// isFlatpak() must report true.
void PreferencesModelTests::platform_isFlatpak_trueWithEnvVarSet()
{
    ScopedEnvVar flatpakEnv("FLATPAK_ID", "io.seamly.SeamlyLayout");
    QVERIFY(Platform::isFlatpak());
}

// ---------------------------------------------------------------------------
// Task 18 — Flatpak-aware directory fallbacks
//
// Mirrors the Task 17 AppImage tests: each test independently reconstructs the
// writable AppConfigLocation root the production code falls back to, then simulates
// a Flatpak sandbox by setting FLATPAK_ID and verifies the default input/output
// directories resolve under that root rather than the read-only <exeDir>.
// ---------------------------------------------------------------------------

// @brief resolvedInputDirectory() with no configured inputDirectory normally falls back to
// <exeDir>/input; inside a (simulated) Flatpak sandbox it must fall back to the writable
// AppConfigLocation root instead, since the sandbox's /app prefix (the exeDir) is read-only.
void PreferencesModelTests::flatpak_resolvedInputDirectory_fallsBackToAppConfigLocation()
{
    const QString appConfigRoot = QDir(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).absolutePath();
    const QString expected = QDir(appConfigRoot).filePath(QStringLiteral("input"));

    ScopedEnvVar flatpakEnv("FLATPAK_ID", "io.seamly.SeamlyLayout");

    PreferencesModel m; // m_inputDirectory defaults to empty — the fallback branch runs
    const QString resolved = m.resolvedInputDirectory();

    QCOMPARE(QFileInfo(resolved).absoluteFilePath(), QFileInfo(expected).absoluteFilePath());
    QVERIFY(!resolved.startsWith(QCoreApplication::applicationDirPath()));
}

// @brief resolvedLayoutDirectory() with no configured layoutDirectory must likewise fall
// back to the AppConfigLocation root (not <exeDir>/output) inside a Flatpak sandbox.
void PreferencesModelTests::flatpak_resolvedLayoutDirectory_fallsBackToAppConfigLocation()
{
    const QString appConfigRoot = QDir(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).absolutePath();
    const QString expected = QDir(appConfigRoot).filePath(QStringLiteral("output"));

    ScopedEnvVar flatpakEnv("FLATPAK_ID", "io.seamly.SeamlyLayout");

    PreferencesModel m; // m_layoutDirectory defaults to empty — the fallback branch runs
    const QString resolved = m.resolvedLayoutDirectory();

    QCOMPARE(QFileInfo(resolved).absoluteFilePath(), QFileInfo(expected).absoluteFilePath());
    QVERIFY(!resolved.startsWith(QCoreApplication::applicationDirPath()));
}

// @brief defaultInputFolderUrl() (the FileDialog default before any input directory has
// ever been configured) must also resolve under the AppConfigLocation root rather than
// <exeDir>/input when running inside a Flatpak sandbox.
void PreferencesModelTests::flatpak_defaultInputFolderUrl_fallsBackToAppConfigLocation()
{
    const QString appConfigRoot = QDir(
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).absolutePath();
    const QString expectedDir = QDir(appConfigRoot).filePath(QStringLiteral("input"));

    ScopedEnvVar flatpakEnv("FLATPAK_ID", "io.seamly.SeamlyLayout");

    const QString resolvedDir = QUrl(PreferencesModel::defaultInputFolderUrl()).toLocalFile();

    QCOMPARE(QFileInfo(resolvedDir).absoluteFilePath(), QFileInfo(expectedDir).absoluteFilePath());
}

QTEST_MAIN(PreferencesModelTests)
#include "PreferencesModelTests.moc"
