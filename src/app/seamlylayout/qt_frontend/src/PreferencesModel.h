// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file PreferencesModel.h
// @brief QObject model for SeamlyLayout application preferences.
//
// Mirrors the Rust AppSettings data model exposed to the Qt frontend.
// Exposes all fields as Q_PROPERTY items for QML binding and stores
// application preferences in qt6_seamlylayout.ini.
//
// INI keys use snake_case for compatibility with legacy preferences JSON:
//   input_directory, layout_directory, preferences_directory,
//   settings_directory, settings_file, preferences_file,
//   dxf_viewer_path, pdf_viewer_path, png_viewer_path, data_root
//
// Registration:
//   Registered at runtime in main.cpp:
//     qmlRegisterType<PreferencesModel>("SeamlyLayout", 1, 0, "PreferencesModel");
//
// Usage in QML:
//   PreferencesModel { id: preferencesModel }
//   Text { text: preferencesModel.inputDirectory }
//   preferencesModel.load(preferencesModel.defaultPreferencesFilePath())
//   preferencesModel.save(preferencesModel.defaultPreferencesFilePath())

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

// @brief QObject model for SeamlyLayout application preferences.
// All fields are Q_PROPERTY so QML can bind to them directly.
class PreferencesModel : public QObject
{
    Q_OBJECT

    // @brief Default input SVG directory for the import file dialog.
    Q_PROPERTY(QString inputDirectory  READ inputDirectory  WRITE setInputDirectory  NOTIFY inputDirectoryChanged)

    // @brief Default output directory for exported layout files.
    Q_PROPERTY(QString layoutDirectory READ layoutDirectory WRITE setLayoutDirectory NOTIFY layoutDirectoryChanged)

    // @brief Default directory for preferences load/save dialogs and storage.
    Q_PROPERTY(QString preferencesDirectory READ preferencesDirectory WRITE setPreferencesDirectory NOTIFY preferencesDirectoryChanged)

    // @brief Default directory for settings load/save dialogs.
    Q_PROPERTY(QString settingsDirectory READ settingsDirectory WRITE setSettingsDirectory NOTIFY settingsDirectoryChanged)

    // @brief Path to the default layout settings JSON file.
    Q_PROPERTY(QString settingsFile    READ settingsFile    WRITE setSettingsFile    NOTIFY settingsFileChanged)

    // @brief Path to the default preferences JSON file.
    Q_PROPERTY(QString preferencesFile READ preferencesFile WRITE setPreferencesFile NOTIFY preferencesFileChanged)

    // @brief Path to the DXF viewer executable.
    Q_PROPERTY(QString dxfViewerPath   READ dxfViewerPath   WRITE setDxfViewerPath   NOTIFY dxfViewerPathChanged)

    // @brief Path to the PDF viewer executable.
    Q_PROPERTY(QString pdfViewerPath   READ pdfViewerPath   WRITE setPdfViewerPath   NOTIFY pdfViewerPathChanged)

    // @brief Path to the PNG viewer executable.
    Q_PROPERTY(QString pngViewerPath   READ pngViewerPath   WRITE setPngViewerPath   NOTIFY pngViewerPathChanged)

    // @brief Path/URL to the projector application (e.g. Pattern Projector).
    // Accepts an https:// URL (https://patternprojector.com) or a local
    // executable path optionally followed by arguments (e.g. Chrome PWA shortcut:
    //   "C:\Program Files\Google\Chrome\Application\chrome_proxy.exe" --profile-directory=Default --app-id=...
    // ).
    Q_PROPERTY(QString projectorPath   READ projectorPath   WRITE setProjectorPath   NOTIFY projectorPathChanged)

    // @brief The user-data root the Windows installer recorded, adopted on first run.
    // Empty when no installer recorded one (unpackaged build, non-Windows) or the user
    // has not been offered one yet; a value set here or by the user in Preferences is
    // never overwritten by a later installer read (see load()/adoptInstallerDataRootIfEmpty()).
    Q_PROPERTY(QString dataRoot        READ dataRoot        WRITE setDataRoot        NOTIFY dataRootChanged)

public:
    explicit PreferencesModel(QObject *parent = nullptr);

    // Getters
    QString inputDirectory()  const { return m_inputDirectory;  }
    QString layoutDirectory() const { return m_layoutDirectory; }
    QString preferencesDirectory() const { return m_preferencesDirectory; }
    QString settingsDirectory() const { return m_settingsDirectory; }
    QString settingsFile()    const { return m_settingsFile;    }
    QString preferencesFile() const { return m_preferencesFile; }
    QString dxfViewerPath()   const { return m_dxfViewerPath;   }
    QString pdfViewerPath()   const { return m_pdfViewerPath;   }
    QString pngViewerPath()   const { return m_pngViewerPath;   }
    QString projectorPath()   const { return m_projectorPath;   }
    QString dataRoot()        const { return m_dataRoot;        }

    // Setters
    void setInputDirectory(const QString &v);
    void setLayoutDirectory(const QString &v);
    void setPreferencesDirectory(const QString &v);
    void setSettingsDirectory(const QString &v);
    void setSettingsFile(const QString &v);
    void setPreferencesFile(const QString &v);
    void setDxfViewerPath(const QString &v);
    void setPdfViewerPath(const QString &v);
    void setPngViewerPath(const QString &v);
    void setProjectorPath(const QString &v);
    void setDataRoot(const QString &v);

    // @brief Load preferences from an INI file or a legacy JSON defaults file.
    // @param path Absolute or relative file path.
    // @return true on success; false if the file cannot be read.
    Q_INVOKABLE bool load(const QString &path);

    // @brief Save current preferences to an INI file.
    // @param path Absolute or relative file path. The function creates the parent directory.
    // @return true on success; false when QSettings reports an error.
    Q_INVOKABLE bool save(const QString &path);

    // @brief Reset preferences to defaults from the configured defaults profile.
    // Attempts to load defaults from preferencesFilePath(). If that file is missing,
    // seeds it from bundled defaults and retries load.
    // @return true on success; false if defaults could not be loaded.
    Q_INVOKABLE bool resetToDefaults();

    // @brief Convert a QUrl (e.g. from FolderDialog.selectedFolder) to a local file path.
    // Uses QUrl::toLocalFile() for correct cross-platform handling (Windows drive letters,
    // Unix absolute paths).  Exposed as invokable so QML can call it directly.
    // @param url URL string, e.g. "file:///C:/Users/...".
    // @return Local file system path string.
    Q_INVOKABLE static QString urlToLocalFile(const QString &url);

    // @brief Convert a local file system path to a file:// URL string.
    // Uses QUrl::fromLocalFile() for correct cross-platform handling:
    //   "C:/Users/..."   →  "file:///C:/Users/..."  (Windows)
    //   "/home/user/..." →  "file:///home/user/..."  (Linux/macOS)
    // Used to set FileDialog.currentFolder from a stored directory path.
    // @param path Local file system path string.
    // @return URL string suitable for FileDialog.currentFolder.
    Q_INVOKABLE static QString localFileToUrl(const QString &path);

    // @brief Return the file:// URL of the default input directory.
    // Falls back to <exeDir>/input when no Input SVG Directory is set in Preferences
    // (Task 16: on macOS this falls back to the writable AppConfigLocation root instead,
    // since a signed .app bundle is read-only; Task 17: same fallback applies at runtime
    // inside a mounted Linux AppImage, detected via Platform::isAppImage(); Task 18: and inside
    // a Flatpak sandbox whose /app prefix is read-only, detected via Platform::isFlatpak()).
    // Uses QCoreApplication::applicationDirPath() for a reliable absolute path.
    Q_INVOKABLE static QString defaultInputFolderUrl();

    // @brief Return the absolute application preferences INI file path.
    // Uses QStandardPaths::AppConfigLocation/qt6_seamlylayout.ini.
    Q_INVOKABLE QString defaultPreferencesFilePath() const;

    // @brief Return the settings file path to use when the Settings dialog opens.
    // Resolves relative settings-file values against resolvedSettingsDirectory().
    // Falls back to <resolvedSettingsDirectory>/default_settings.json.
    Q_INVOKABLE QString settingsFilePath() const;

    // @brief Return the preferences file path to use for default preferences profile.
    // Resolves relative preferences-file values against preferencesDirectory.
    // Falls back to <preferencesDirectory>/default_preferences.json.
    Q_INVOKABLE QString preferencesFilePath() const;

    // @brief Return the resolved settings directory path.
    // Uses `settings_directory` when configured; otherwise falls back to
    // QStandardPaths::AppConfigLocation/settings.
    // Relative configured paths are resolved against AppConfigLocation
    // (never process working directory). Ensures the directory exists before
    // returning. For the default location, first run attempts to seed/migrate
    // default_settings.json from legacy <exeDir>/settings/default_settings.json.
    Q_INVOKABLE QString resolvedSettingsDirectory() const;

    // @brief Return the resolved input SVG directory path.
    // Uses `input_directory` when configured; otherwise falls back to <dataRoot>/input when
    // dataRoot is set; otherwise <exeDir>/input (AppConfigLocation root on macOS, or at
    // runtime inside a read-only Linux AppImage mount or Flatpak /app prefix — see Task 16 /
    // 17 / 18). Relative configured paths are resolved against AppConfigLocation (never
    // process working directory). Ensures the directory exists before returning.
    Q_INVOKABLE QString resolvedInputDirectory() const;

    // @brief Return the resolved layout output directory path.
    // Uses the directory configured in qt6_seamlylayout.ini (`layout_directory`) when set.
    // Falls back to <dataRoot>/output when dataRoot is set; otherwise <exeDir>/output
    // (AppConfigLocation root on macOS, or at runtime inside a read-only Linux AppImage mount
    // or Flatpak /app prefix — see Task 16 / 17 / 18).
    // Relative configured paths are resolved against AppConfigLocation.
    // Ensures the returned directory exists by creating it if needed.
    Q_INVOKABLE QString resolvedLayoutDirectory() const;

    // @brief Open a native QtWidgets save-file dialog.
    // @param title Dialog title (e.g. "Save DXF-ASTM File").
    // @param dir Initial directory to open in.
    // @param defaultName Default filename shown in the name field.
    // @param filter Name filter (e.g. "DXF Files (*.dxf);;All Files (*)").
    // @return Chosen absolute path, or empty string if cancelled.
    Q_INVOKABLE static QString getSaveFilePath(const QString &title,
                                               const QString &dir,
                                               const QString &defaultName,
                                               const QString &filter);

    // @brief Open a Seamly-branded QtWidgets open-file dialog.
    // @param title Dialog title (e.g. "Open DXF-ASTM File").
    // @param dir Initial directory to open in.
    // @param filter Name filter (e.g. "DXF Files (*.dxf);;All Files (*)").
    // @return Chosen absolute path, or empty string if cancelled.
    Q_INVOKABLE static QString getOpenFilePath(const QString &title,
                                               const QString &dir,
                                               const QString &filter);

    // @brief Launch a file in a viewer application or open an online viewer URL.
    // If viewerPath is an HTTP/HTTPS URL, opens the URL in the default browser.
    // Otherwise, starts the viewer executable as a detached process with the file path.
    // @param viewerPath Absolute path to the viewer executable, or an HTTP/HTTPS URL.
    // @param filePath Absolute path to the file to open (ignored for URL viewers).
    // @return true if the viewer opened; false if viewerPath is empty or launch failed.
    Q_INVOKABLE static bool openInViewer(const QString &viewerPath, const QString &filePath);

    // @brief Check if a viewer path is an HTTP/HTTPS URL (online viewer).
    // @param viewerPath The viewer path to check.
    // @return true if the path starts with http:// or https://.
    Q_INVOKABLE static bool isViewerUrl(const QString &viewerPath);

    // @brief Return true if a regular file exists at the given absolute path.
    // Uses QFileInfo for cross-platform correctness.
    // @param path Absolute file path to test.
    // @return true if the path exists and is a file; false if missing or empty.
    Q_INVOKABLE static bool fileExists(const QString &path);

    // @brief Derive the companion teaching-file path from a DXF-ASTM file path.
    // The teaching file is a .txt file with the same base name in the same directory,
    // generated optionally during DXF export when createTeachingVersion is true.
    // Example: "/output/layout.dxf" → "/output/layout.txt"
    // @param dxfPath Absolute path to the .dxf file.
    // @return Absolute path of the companion .txt teaching file, or empty if dxfPath is empty.
    Q_INVOKABLE static QString dxfTeachingFilePath(const QString &dxfPath);

    // @brief Parse a viewer-path field into (executable, args).
    // Resolution order:
    //   1. Strip a leading `file:///` URL prefix if present.
    //   2. If the whole trimmed string is an existing file on disk, treat it
    //      as the executable with no preset args. This is what makes Browse-
    //      picked Windows paths with spaces (e.g.
    //      `C:\Program Files\Common Files\eDrawings2026\eDrawings.exe`) work
    //      without quoting.
    //   3. Otherwise parse with QProcess::splitCommand for shell-style
    //      handling (Chrome PWA wrapper, etc.).
    // @param viewerPath Raw viewer field text.
    // @return Stringlist where element 0 is the executable path (already
    //         converted to native separators) and the remainder are preset
    //         arguments. Empty list when the input parses to no tokens.
    static QStringList parseViewerCommand(const QString &viewerPath);

signals:
    void inputDirectoryChanged();
    void layoutDirectoryChanged();
    void preferencesDirectoryChanged();
    void settingsDirectoryChanged();
    void settingsFileChanged();
    void preferencesFileChanged();
    void dxfViewerPathChanged();
    void pdfViewerPathChanged();
    void pngViewerPathChanged();
    void projectorPathChanged();
    void dataRootChanged();

private:
    /// @brief Load a JSON defaults file or a legacy preferences file.
    /// @param path Absolute JSON file path.
    /// @return true when the JSON object was read and applied.
    bool loadJsonPreferences(const QString &path);

    /// @brief Migrate saved paths from legacy folder names.
    void migrateLegacyPreferencePaths();

    /// @brief Import the first available legacy preferences JSON file.
    /// @param iniPath Absolute destination INI file path.
    /// @return true when values were imported and saved.
    bool migrateLegacyPreferencesJson(const QString &iniPath);

    /// @brief Adopt the Windows installer's recorded data root, once, if dataRoot is unset.
    /// No-op when dataRoot is already set (the user's own choice always wins), when no
    /// installer value was recorded, or off Windows. Persists the adopted value to iniPath.
    /// @param iniPath Absolute application preferences INI path to save into on adoption.
    void adoptInstallerDataRootIfEmpty(const QString &iniPath);

    // Fields — defaults match AppSettings::default() in Rust
    QString m_inputDirectory  = QStringLiteral("");
    QString m_layoutDirectory = QStringLiteral("");
    QString m_preferencesDirectory = QStringLiteral("");
    QString m_settingsDirectory = QStringLiteral("");
    QString m_settingsFile    = QStringLiteral("default_settings.json");
    QString m_preferencesFile = QStringLiteral("default_preferences.json");
    QString m_dxfViewerPath   = QStringLiteral("");
    QString m_pdfViewerPath   = QStringLiteral("");
    QString m_pngViewerPath   = QStringLiteral("");
    QString m_projectorPath   = QStringLiteral("");
    QString m_dataRoot        = QStringLiteral("");
}; // PreferencesModel
