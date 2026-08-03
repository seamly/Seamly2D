// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file SettingsModel.h
// @brief QObject model for SeamlyLayout layout settings.
//
// Mirrors the Rust LayoutSettings data model exposed to the Qt frontend.
// Exposes all fields as Q_PROPERTY items for QML binding and handles
// JSON load/save, unit conversion, and paper/tile size lookup.
//
// Registration:
//   Registered at runtime in main.cpp:
//     qmlRegisterType<SettingsModel>("SeamlyLayout", 1, 0, "SettingsModel");
//
// Usage in QML:
//   SettingsModel { id: settingsModel }
//   Text { text: settingsModel.unit }
//   settingsModel.load(SettingsModel::defaultSettingsFilePath())
//   settingsModel.save(SettingsModel::defaultSettingsFilePath())

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

// @brief QObject model for SeamlyLayout layout settings.
// All fields are Q_PROPERTY so QML can bind to them directly.
class SettingsModel : public QObject
{
    Q_OBJECT

    // -----------------------------------------------------------------------
    // Layout options
    // -----------------------------------------------------------------------

    // @brief Piece-arrangement mode: "alongGrainline" | "withNap".
    //   alongGrainline → grain-up baseline, allow {0°, 180°}
    //   withNap        → grain-up baseline, {0°} only (no flip)
    //
    Q_PROPERTY(QString layoutMode    READ layoutMode    WRITE setLayoutMode    NOTIFY layoutModeChanged)

    // @brief Rotation step in degrees.
    // In current modes this is used only when layoutMode == "withNap":
    //   0.0   = pieces point up
    //   180.0 = pieces point down
    Q_PROPERTY(double  rotationStep  READ rotationStep  WRITE setRotationStep  NOTIFY rotationStepChanged)

    // @brief Whether fabric is cut on the fold (doubles usable width).
    Q_PROPERTY(bool    fabricFolded  READ fabricFolded  WRITE setFabricFolded  NOTIFY fabricFoldedChanged)

    // @brief Minimum clearance between adjacent placed pieces, in active units.
    // The packer enforces at least this much space between any two pieces in
    // both axes; sub-unit values (e.g. 0.05") are honored at scaled-int
    // precision in the polygon path.
    Q_PROPERTY(double  pieceGap      READ pieceGap      WRITE setPieceGap      NOTIFY pieceGapChanged)

    // @brief Piece gap in pixels at 96 px/in (derived from pieceGap + unit).
    // QML can bind to this when it needs the pixel-space value (e.g. preview
    // overlays) without re-doing the unit conversion.
    Q_PROPERTY(int     pieceGapPx    READ pieceGapPx    NOTIFY pieceGapPxChanged)

    // -----------------------------------------------------------------------
    // Unit system
    // -----------------------------------------------------------------------

    // @brief Active unit system: "in" | "mm" | "cm".
    Q_PROPERTY(QString unit          READ unit          WRITE setUnit          NOTIFY unitChanged)

    // -----------------------------------------------------------------------
    // Media type / paper type
    // -----------------------------------------------------------------------

    // @brief Media type: "paper" | "roll".
    Q_PROPERTY(QString mediaType     READ mediaType     WRITE setMediaType     NOTIFY mediaTypeChanged)

    // @brief Paper type when mediaType == "paper": "sheet" | "tiled" | "roll".
    Q_PROPERTY(QString paperType     READ paperType     WRITE setPaperType     NOTIFY paperTypeChanged)

    // -----------------------------------------------------------------------
    // Sheet (paper) dimensions
    // -----------------------------------------------------------------------

    // @brief Selected paper size name (e.g., "ARCH E", "A4").
    Q_PROPERTY(QString sheetName     READ sheetName     WRITE setSheetName     NOTIFY sheetNameChanged)

    // @brief Sheet/page width in active units.
    Q_PROPERTY(double  pageWidth     READ pageWidth     WRITE setPageWidth     NOTIFY pageWidthChanged)

    // @brief Sheet/page height in active units.
    Q_PROPERTY(double  pageHeight    READ pageHeight    WRITE setPageHeight    NOTIFY pageHeightChanged)

    // @brief Sheet/page width in pixels at 96 px/in (derived from pageWidth + unit).
    Q_PROPERTY(int     pageWidthPx   READ pageWidthPx   NOTIFY pageWidthPxChanged)

    // @brief Sheet/page height in pixels at 96 px/in (derived from pageHeight + unit).
    Q_PROPERTY(int     pageHeightPx  READ pageHeightPx  NOTIFY pageHeightPxChanged)

    // -----------------------------------------------------------------------
    // Roll dimensions
    // -----------------------------------------------------------------------

    // @brief Roll size descriptor string (e.g., "36 in").
    Q_PROPERTY(QString rollSize      READ rollSize      WRITE setRollSize      NOTIFY rollSizeChanged)

    // @brief Roll width in active units.
    Q_PROPERTY(double  rollWidth     READ rollWidth     WRITE setRollWidth     NOTIFY rollWidthChanged)

    // @brief Roll width in pixels at 96 px/in (derived from rollWidth + unit).
    Q_PROPERTY(int     rollWidthPx   READ rollWidthPx   NOTIFY rollWidthPxChanged)

    // @brief Roll length in active units (derived — 500 in / 1270 cm / 12700 mm).
    // Display-only; updates when unit changes.
    Q_PROPERTY(double  rollLength    READ rollLength    NOTIFY rollLengthChanged)

    // @brief Roll length sentinel in pixels (always 48000 px = 500 in × 96 px/in).
    Q_PROPERTY(int     rollLengthPx  READ rollLengthPx  CONSTANT)

    // -----------------------------------------------------------------------
    // Tiled paper
    // -----------------------------------------------------------------------

    // @brief Tile page size name (e.g., "Letter", "A4").
    Q_PROPERTY(QString tileSize      READ tileSize      WRITE setTileSize      NOTIFY tileSizeChanged)

    // @brief Tile orientation: "landscape" | "portrait".
    Q_PROPERTY(QString tileOrientation READ tileOrientation WRITE setTileOrientation NOTIFY tileOrientationChanged)

    // -----------------------------------------------------------------------
    // Margins (in active units)
    // -----------------------------------------------------------------------

    // @brief Top margin in active units.
    Q_PROPERTY(double  marginTop     READ marginTop     WRITE setMarginTop     NOTIFY marginTopChanged)

    // @brief Bottom margin in active units.
    Q_PROPERTY(double  marginBottom  READ marginBottom  WRITE setMarginBottom  NOTIFY marginBottomChanged)

    // @brief Left margin in active units.
    Q_PROPERTY(double  marginLeft    READ marginLeft    WRITE setMarginLeft    NOTIFY marginLeftChanged)

    // @brief Right margin in active units.
    Q_PROPERTY(double  marginRight   READ marginRight   WRITE setMarginRight   NOTIFY marginRightChanged)

    // -----------------------------------------------------------------------
    // Fabric / selvedge
    // -----------------------------------------------------------------------

    // @brief Fabric width in active units (0 = use page width).
    Q_PROPERTY(double  fabricWidth   READ fabricWidth   WRITE setFabricWidth   NOTIFY fabricWidthChanged)

    // @brief Fabric height/length in active units (0 = use page height).
    Q_PROPERTY(double  fabricHeight  READ fabricHeight  WRITE setFabricHeight  NOTIFY fabricHeightChanged)

    // @brief Selvedge width deducted from each side of fabric.
    Q_PROPERTY(double  selvedgeWidth READ selvedgeWidth WRITE setSelvedgeWidth NOTIFY selvedgeWidthChanged)

    // -----------------------------------------------------------------------
    // ComboBox source lists (read-only)
    // -----------------------------------------------------------------------

    // @brief Paper size names for the sheet name ComboBox.
    Q_PROPERTY(QStringList paperSizeNames  READ paperSizeNames  CONSTANT)

    // @brief Tile size names for the tile size ComboBox.
    Q_PROPERTY(QStringList tileSizeNames   READ tileSizeNames   CONSTANT)

    // @brief Tile orientation names for the tiled-paper orientation ComboBox.
    Q_PROPERTY(QStringList tileOrientationNames READ tileOrientationNames CONSTANT)

    // @brief Unit system names for the unit ComboBox.
    Q_PROPERTY(QStringList unitNames       READ unitNames       CONSTANT)

    // @brief Piece-arrangement mode names for the layout-mode radio group.
    // Order: { "alongGrainline", "withNap" } — matches the radio order in QML.
    Q_PROPERTY(QStringList layoutModeNames READ layoutModeNames CONSTANT)

    // @brief Allowed rotation step values for mode-specific controls.
    // Returned as a QVariantList of doubles so QML can iterate or bind directly.
    // Current values are for withNap direction: { 0.0, 180.0 }.
    Q_PROPERTY(QVariantList rotationStepValues READ rotationStepValues CONSTANT)

    // @brief Roll size names for the roll size ComboBox.
    Q_PROPERTY(QStringList rollSizeNames   READ rollSizeNames   CONSTANT)

public:
    explicit SettingsModel(QObject *parent = nullptr);

    // Getters
    QString    layoutMode()    const { return m_layoutMode;    }
    double     rotationStep()  const { return m_rotationStep;  }
    bool       fabricFolded()  const { return m_fabricFolded;  }
    double     pieceGap()      const { return m_pieceGap;      }
    int        pieceGapPx()    const;
    QString    unit()          const { return m_unit;          }
    QString    mediaType()     const { return m_mediaType;     }
    QString    paperType()     const { return m_paperType;     }
    QString    sheetName()     const { return m_sheetName;     }
    double     pageWidth()     const { return m_pageWidth;     }
    double     pageHeight()    const { return m_pageHeight;    }
    int        pageWidthPx()   const;
    int        pageHeightPx()  const;
    QString    rollSize()      const { return m_rollSize;      }
    double     rollWidth()     const { return m_rollWidth;     }
    int        rollWidthPx()   const;
    double     rollLength()    const;
    int        rollLengthPx()  const { return 48000;           }
    QString    tileSize()      const { return m_tileSize;      }
    QString    tileOrientation() const { return m_tileOrientation; }
    double     marginTop()     const { return m_marginTop;     }
    double     marginBottom()  const { return m_marginBottom;  }
    double     marginLeft()    const { return m_marginLeft;    }
    double     marginRight()   const { return m_marginRight;   }
    double     fabricWidth()   const { return m_fabricWidth;   }
    double     fabricHeight()  const { return m_fabricHeight;  }
    double     selvedgeWidth() const { return m_selvedgeWidth; }

    QStringList paperSizeNames()  const;
    QStringList tileSizeNames()   const;
    QStringList tileOrientationNames() const;
    QStringList unitNames()       const;
    QStringList layoutModeNames() const;
    QVariantList rotationStepValues() const;
    QStringList rollSizeNames()   const;

    // Setters
    void setLayoutMode(const QString &v);
    void setRotationStep(double v);
    void setFabricFolded(bool v);
    void setPieceGap(double v);
    void setUnit(const QString &v);
    void setMediaType(const QString &v);
    void setPaperType(const QString &v);
    void setSheetName(const QString &v);
    void setPageWidth(double v);
    void setPageHeight(double v);
    void setRollSize(const QString &v);
    void setRollWidth(double v);
    void setTileSize(const QString &v);
    void setTileOrientation(const QString &v);
    void setMarginTop(double v);
    void setMarginBottom(double v);
    void setMarginLeft(double v);
    void setMarginRight(double v);
    void setFabricWidth(double v);
    void setFabricHeight(double v);
    void setSelvedgeWidth(double v);

    // @brief Load settings from a JSON file.
    // @param path File path (relative or absolute).
    // @return true on success; false if file not found (defaults applied) or parse error.
    Q_INVOKABLE bool load(const QString &path);

    // @brief Save current settings to a JSON file.
    // @param path File path (relative or absolute).  Directory is created if absent.
    // @return true on success; false on write error.
    Q_INVOKABLE bool save(const QString &path);

    // @brief Convert a file:// URL string to a local file system path.
    // Uses QUrl::toLocalFile() for correct cross-platform handling.
    Q_INVOKABLE static QString urlToLocalFile(const QString &url);

    // @brief Convert a local file system path to a file:// URL string.
    // Uses QUrl::fromLocalFile() for correct cross-platform handling.
    Q_INVOKABLE static QString localFileToUrl(const QString &path);

    // @brief Return the file:// URL of the default settings folder.
    // Uses QStandardPaths::AppConfigLocation/settings and ensures the directory exists.
    // Used to set FileDialog.currentFolder so the picker opens in a user-writable location.
    Q_INVOKABLE QString settingsFolderUrl() const;

    // @brief Return the absolute default settings JSON file path.
    // Uses QStandardPaths::AppConfigLocation/settings/default_settings.json and
    // ensures the parent directory exists.
    Q_INVOKABLE static QString defaultSettingsFilePath();

    // @brief Serialize current settings to a compact JSON string for Rust bridge.
    // @return JSON string with camelCase keys matching LayoutSettings serde fields.
    Q_INVOKABLE QString toJson() const;

    // @brief Reset all fields to application defaults.
    Q_INVOKABLE void resetToDefaults();

    // @brief Convert all dimension and margin fields from one unit to another.
    // @param fromUnit Source unit ("in", "mm", "cm").
    // @param toUnit   Target unit ("in", "mm", "cm").
    Q_INVOKABLE void convertAllUnits(const QString &fromUnit, const QString &toUnit);

    // @brief Select a paper size by name and update pageWidth/pageHeight.
    // @param name Paper size name (e.g., "ARCH E", "A4").
    Q_INVOKABLE void selectPaperSize(const QString &name);

    // @brief Select a tile size by name (tile dimensions are stored internally).
    // @param name Tile size name (e.g., "Letter", "A4").
    Q_INVOKABLE void selectTileSize(const QString &name);

    // @brief Return the default margin for a given unit system.
    // @param unit Unit string ("in", "mm", "cm").
    // @return Default margin value in that unit.
    Q_INVOKABLE double defaultMarginForUnit(const QString &unit) const;

signals:
    void layoutModeChanged();
    void rotationStepChanged();
    void fabricFoldedChanged();
    void pieceGapChanged();
    void pieceGapPxChanged();
    void unitChanged();
    void mediaTypeChanged();
    void paperTypeChanged();
    void sheetNameChanged();
    void pageWidthChanged();
    void pageHeightChanged();
    void rollSizeChanged();
    void rollWidthChanged();
    void rollWidthPxChanged();
    void rollLengthChanged();
    void pageWidthPxChanged();
    void pageHeightPxChanged();
    void tileSizeChanged();
    void tileOrientationChanged();
    void marginTopChanged();
    void marginBottomChanged();
    void marginLeftChanged();
    void marginRightChanged();
    void fabricWidthChanged();
    void fabricHeightChanged();
    void selvedgeWidthChanged();

    // @brief Emitted after load() successfully applies all fields.
    // Setters suppress re-emission when a value is unchanged, so QML controls
    // with broken runtime bindings (RadioButton, ComboBox, TextField edited by
    // the user) would not refresh.  This signal is emitted unconditionally so
    // QML Connections handlers can force-refresh all dialog controls.
    void settingsLoaded();

private:
    // @brief Convert a single value between unit systems.
    static double convertUnit(double value, const QString &fromUnit, const QString &toUnit);

    // @brief Keep hidden fabric-mode margins aligned with selvedge width.
    void syncFabricMarginsFromSelvedge();

    // Fields — defaults match LayoutSettings::default() in Rust
    QString m_layoutMode    = QStringLiteral("alongGrainline");
    double  m_rotationStep  = 0.0;  // degrees; used by withNap (0 up, 180 down)
    bool    m_fabricFolded  = false;
    double  m_pieceGap      = 0.05;  // active-unit clearance between adjacent pieces (default ≈ 5 px @ 96 dpi)
    QString m_unit          = QStringLiteral("in");
    QString m_mediaType     = QStringLiteral("paper");
    QString m_paperType     = QStringLiteral("sheet");
    QString m_sheetName     = QStringLiteral("ARCH E");
    double  m_pageWidth     = 36.0;  // ARCH E width in inches
    double  m_pageHeight    = 48.0;  // ARCH E height in inches
    QString m_rollSize      = QStringLiteral("36 in");
    double  m_rollWidth     = 36.0;
    QString m_tileSize      = QStringLiteral("Letter");
    QString m_tileOrientation = QStringLiteral("landscape");
    double  m_marginTop     = 0.25;
    double  m_marginBottom  = 0.25;
    double  m_marginLeft    = 0.25;
    double  m_marginRight   = 0.25;
    double  m_fabricWidth   = 0.0;
    double  m_fabricHeight  = 0.0;
    double  m_selvedgeWidth = 0.0;
}; // SettingsModel
