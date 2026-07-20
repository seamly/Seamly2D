// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file PreferencesWindow.cpp
// @brief Implementation of PreferencesWindow — QtWidgets preferences dialog.
//
// Layout mirrors PreferencesPanel.qml:
//   ┌──────────────────────────────────────────────────┐
//   │  Title bar — "Preferences" (violet background)   │
//   ├──────────────────────────────────────────────────┤
//   │  § Directories                                   │
//   │    Input SVG Directory:      [__________] Browse  │
//   │    Layout Output Directory:  [__________] Browse  │
//   │    Settings Directory:       [__________] Browse  │
//   │    Default Settings File:    [__________] Browse  │
//   │    Default Preferences File: [__________] Browse  │
//   ├──────────────────────────────────────────────────┤
//   │  § Viewer Applications                           │
//   │    DXF Viewer:               [__________] Browse  │
//   │    PDF Viewer:               [__________] Browse  │
//   ├──────────────────────────────────────────────────┤
//   │          [ Reset to Defaults ] [ Save ] [ Discard ] │
//   └──────────────────────────────────────────────────┘

#include "PreferencesWindow.h"
#include "PreferencesModel.h"
#include "SeamlyTheme.h"
#include "Logger.h"

#include <QBoxLayout>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStyleFactory>

// ---------------------------------------------------------------------------
// Helper — create a section header label (violet text on translucent stripe)
// ---------------------------------------------------------------------------

/// @brief Create a section header widget matching the QML Rectangle + Text.
/// @param text Section title.
/// @return Owning pointer to the section header QWidget.
static QWidget *makeSectionHeader(const QString &text)
{
    auto *widget = new QWidget;
    widget->setFixedHeight(28);
    widget->setAutoFillBackground(true);

    // Translucent white stripe — matches QML Qt.rgba(1,1,1,0.08)
    QPalette pal = widget->palette();
    pal.setColor(QPalette::Window, QColor(255, 255, 255, 20));
    widget->setPalette(pal);

    auto *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(12, 0, 0, 0);

    auto *label = new QLabel(text);
    QFont font = label->font();
    font.setPixelSize(12);  // Theme.fontSizeSmall
    font.setBold(true);
    label->setFont(font);
    label->setStyleSheet(
        QStringLiteral("color: %1;").arg(SeamlyTheme::SEAMLY_VIOLET_LIGHT.name()));

    layout->addWidget(label);
    return widget;
} // makeSectionHeader

// ---------------------------------------------------------------------------
// Helper — create a read-only QLineEdit styled to match Theme.fieldBackground
// ---------------------------------------------------------------------------

/// @brief Create a read-only line edit with Seamly field styling.
/// @return Owning pointer to the QLineEdit.
static QLineEdit *makeReadOnlyField()
{
    auto *field = new QLineEdit;
    field->setReadOnly(true);
    field->setPlaceholderText(QStringLiteral("(not set)"));

    // Style: white text on dark violet field background (#3e2b60)
    field->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 2px;"
        "  padding: 2px 4px;"
        "}")
        .arg(SeamlyTheme::SEAMLY_VIOLET_DARK.name(),   // fieldBackground
             SeamlyTheme::SEAMLY_GRAY_LIGHT.name(),    // fieldText
             SeamlyTheme::SEAMLY_VIOLET_MEDIUM.name()) // subtle border
    );

    return field;
} // makeReadOnlyField

/// @brief Create an EDITABLE line edit with the same Seamly field styling.
/// Used for viewer paths so users can type an https:// URL directly (in
/// addition to using the Browse button for a local executable).
/// @param placeholder Hint text shown when empty.
/// @return Owning pointer to the QLineEdit.
static QLineEdit *makeEditableField(const QString &placeholder)
{
    auto *field = new QLineEdit;
    field->setReadOnly(false);
    field->setPlaceholderText(placeholder);

    field->setStyleSheet(QStringLiteral(
        "QLineEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 2px;"
        "  padding: 2px 4px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid %4;"
        "}")
        .arg(SeamlyTheme::SEAMLY_VIOLET_DARK.name(),    // fieldBackground
             SeamlyTheme::SEAMLY_GRAY_LIGHT.name(),     // fieldText
             SeamlyTheme::SEAMLY_VIOLET_MEDIUM.name(),  // border
             SeamlyTheme::SEAMLY_VIOLET_LIGHT.name())   // focus border
    );

    return field;
} // makeEditableField

/// @brief Return the shared "Use Task Manager to find the install path"
/// instructions block used at the end of every viewer help popup.
/// The field name (e.g. "DXF Viewer", "Projector") is substituted into the
/// final step so the user knows where to paste the copied path.
/// @param fieldName Field label as it appears in the Preferences dialog.
/// @return Plain-text instructions block, ready to concatenate.
static QString taskMgrInstructions(const QString &fieldName)
{
    return QStringLiteral(
        "If the executable path isn't obvious on Windows, use Task Manager to find it:\n"
        "    a. Launch the application.\n"
        "    b. Open Task Manager (Ctrl+Shift+Esc).\n"
        "    c. Find the application in the Processes list.\n"
        "    d. Right-click → Open file location.\n"
        "    e. On the highlighted file in the Explorer window that opens → "
        "Right-click → Copy as path.\n"
        "    f. Paste the path into the %1 field below and click Save."
    ).arg(fieldName);
} // taskMgrInstructions

/// @brief Create a small round "?" help button (matches QML help-icon style).
/// @return Owning pointer to the QPushButton.
static QPushButton *makeHelpIcon()
{
    auto *btn = new QPushButton(QStringLiteral("?"));
    btn->setFixedSize(18, 18);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: %1;"
        "  color: white;"
        "  border: 1px solid %2;"
        "  border-radius: 9px;"
        "  font-weight: bold;"
        "  font-size: 12px;"
        "  padding: 0px;"
        "}"
        "QPushButton:hover { background-color: %3; }")
        .arg(SeamlyTheme::SEAMLY_VIOLET_MEDIUM.name(),
             SeamlyTheme::SEAMLY_VIOLET_DARK.name(),
             SeamlyTheme::SEAMLY_VIOLET_LIGHT.name())
    );
    return btn;
} // makeHelpIcon

// ---------------------------------------------------------------------------
// Helper — create a Browse button matching SeamlyButton styling
// ---------------------------------------------------------------------------

/// @brief Create a Browse button with Seamly violet styling.
/// @return Owning pointer to the QPushButton.
static QPushButton *makeBrowseButton()
{
    auto *btn = new QPushButton(QStringLiteral("Browse\u2026"));
    btn->setFixedWidth(76);

    // Style: violet button with hover, matching SeamlyButton.qml
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %4;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %5;"
        "}")
        .arg(SeamlyTheme::SEAMLY_VIOLET_MEDIUM.name(),  // normal
             SeamlyTheme::SEAMLY_GRAY_LIGHT.name(),     // text
             SeamlyTheme::SEAMLY_VIOLET_DARK.name(),    // border
             SeamlyTheme::SEAMLY_VIOLET.name(),         // hover
             SeamlyTheme::SEAMLY_VIOLET_DARK.name())    // pressed
    );

    return btn;
} // makeBrowseButton

// ---------------------------------------------------------------------------
// Helper — create a styled footer button (Save / Discard)
// ---------------------------------------------------------------------------

/// @brief Create a footer action button with Seamly violet styling.
/// @param text Button label.
/// @return Owning pointer to the QPushButton.
static QPushButton *makeFooterButton(const QString &text)
{
    auto *btn = new QPushButton(text);
    btn->setMinimumWidth(80);

    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %4;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %5;"
        "}")
        .arg(SeamlyTheme::SEAMLY_VIOLET_MEDIUM.name(),
             SeamlyTheme::SEAMLY_GRAY_LIGHT.name(),
             SeamlyTheme::SEAMLY_VIOLET_DARK.name(),
             SeamlyTheme::SEAMLY_VIOLET.name(),
             SeamlyTheme::SEAMLY_VIOLET_DARK.name())
    );

    return btn;
} // makeFooterButton

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

/// @brief Build the preferences dialog UI and wire Browse/Save/Discard buttons.
PreferencesWindow::PreferencesWindow(PreferencesModel *model, QWidget *parent)
    : QDialog(parent)
    , m_model(model)
{
    setWindowTitle(QStringLiteral("Preferences"));
    setFixedSize(620, 540);

    // Apply Seamly branding — per-window only (does not affect QML controls)
    setPalette(SeamlyTheme::makeSeamlyPalette());
    setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Dialog background
    setStyleSheet(QStringLiteral(
        "PreferencesWindow {"
        "  background-color: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 4px;"
        "}")
        .arg(SeamlyTheme::SEAMLY_VIOLET.name(),        // dialogBackground
             SeamlyTheme::SEAMLY_VIOLET_DARK.name())   // border
    );

    // Main layout
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // -----------------------------------------------------------------------
    // Section: Directories
    // -----------------------------------------------------------------------
    mainLayout->addWidget(makeSectionHeader(QStringLiteral("Directories")));

    auto *dirGrid = new QGridLayout;
    dirGrid->setContentsMargins(12, 8, 12, 8);
    dirGrid->setHorizontalSpacing(8);
    dirGrid->setVerticalSpacing(8);

    // Field label style
    const QString labelStyle = QStringLiteral("color: %1; font-size: 14px;")
        .arg(SeamlyTheme::SEAMLY_GRAY_LIGHT.name());

    // Row 0: Input SVG Directory
    auto *inputDirLabel = new QLabel(QStringLiteral("Input SVG Directory:"));
    inputDirLabel->setFixedWidth(160);
    inputDirLabel->setStyleSheet(labelStyle);
    m_inputDirField = makeReadOnlyField();
    auto *inputDirBrowse = makeBrowseButton();
    dirGrid->addWidget(inputDirLabel,    0, 0);
    dirGrid->addWidget(m_inputDirField,  0, 1);
    dirGrid->addWidget(inputDirBrowse,   0, 2);

    // Row 1: Layout Output Directory
    auto *layoutDirLabel = new QLabel(QStringLiteral("Layout Output Directory:"));
    layoutDirLabel->setFixedWidth(160);
    layoutDirLabel->setStyleSheet(labelStyle);
    m_layoutDirField = makeReadOnlyField();
    auto *layoutDirBrowse = makeBrowseButton();
    dirGrid->addWidget(layoutDirLabel,    1, 0);
    dirGrid->addWidget(m_layoutDirField,  1, 1);
    dirGrid->addWidget(layoutDirBrowse,   1, 2);

    // Row 2: Settings Directory
    auto *settingsDirLabel = new QLabel(QStringLiteral("Settings Directory:"));
    settingsDirLabel->setFixedWidth(160);
    settingsDirLabel->setStyleSheet(labelStyle);
    m_settingsDirField = makeReadOnlyField();
    auto *settingsDirBrowse = makeBrowseButton();
    dirGrid->addWidget(settingsDirLabel,    2, 0);
    dirGrid->addWidget(m_settingsDirField,  2, 1);
    dirGrid->addWidget(settingsDirBrowse,   2, 2);

    // Row 3: Default Settings File
    auto *settingsFileLabel = new QLabel(QStringLiteral("Default Settings File:"));
    settingsFileLabel->setFixedWidth(160);
    settingsFileLabel->setStyleSheet(labelStyle);
    m_settingsFileField = makeReadOnlyField();
    auto *settingsFileBrowse = makeBrowseButton();
    dirGrid->addWidget(settingsFileLabel,    3, 0);
    dirGrid->addWidget(m_settingsFileField,  3, 1);
    dirGrid->addWidget(settingsFileBrowse,   3, 2);

    // Row 4: Default Preferences File
    auto *preferencesFileLabel = new QLabel(QStringLiteral("Default Preferences File:"));
    preferencesFileLabel->setFixedWidth(160);
    preferencesFileLabel->setStyleSheet(labelStyle);
    m_preferencesFileField = makeReadOnlyField();
    auto *preferencesFileBrowse = makeBrowseButton();
    dirGrid->addWidget(preferencesFileLabel,    4, 0);
    dirGrid->addWidget(m_preferencesFileField,  4, 1);
    dirGrid->addWidget(preferencesFileBrowse,   4, 2);

    // Column 1 stretches to fill available width
    dirGrid->setColumnStretch(1, 1);
    mainLayout->addLayout(dirGrid);

    // -----------------------------------------------------------------------
    // Section: Viewer Applications
    // -----------------------------------------------------------------------
    mainLayout->addWidget(makeSectionHeader(QStringLiteral("Viewer Applications")));

    auto *viewerGrid = new QGridLayout;
    viewerGrid->setContentsMargins(12, 8, 12, 12);
    viewerGrid->setHorizontalSpacing(8);
    viewerGrid->setVerticalSpacing(8);

    // Viewer field placeholder hint — accepts both forms.
    const QString viewerPlaceholder = QStringLiteral("local exe path or https:// URL");

    // Row 0: DXF Viewer (editable; "?" help icon to the left of the field
    // explains why eDrawings is recommended for the DXF-ASTM multi-layer format).
    auto *dxfLabelRow = new QWidget;
    dxfLabelRow->setFixedWidth(160);
    auto *dxfLabelLayout = new QHBoxLayout(dxfLabelRow);
    dxfLabelLayout->setContentsMargins(0, 0, 0, 0);
    dxfLabelLayout->setSpacing(4);
    auto *dxfViewerLabel = new QLabel(QStringLiteral("DXF Viewer:"));
    dxfViewerLabel->setStyleSheet(labelStyle);
    auto *dxfHelpBtn = makeHelpIcon();
    dxfLabelLayout->addWidget(dxfViewerLabel);
    dxfLabelLayout->addWidget(dxfHelpBtn);
    dxfLabelLayout->addStretch(1);

    m_dxfViewerField = makeEditableField(
        QStringLiteral("e.g. https://sharecad.org  or  https://www.edrawingsviewer.com/openview-dwg-and-dxf-files"));
    m_dxfViewerField->setToolTip(QStringLiteral(
        "Local DXF viewer executable, OR an online viewer URL. Examples:\n"
        "  https://sharecad.org\n"
        "  https://www.edrawingsviewer.com/openview-dwg-and-dxf-files"));
    auto *dxfViewerBrowse = makeBrowseButton();
    viewerGrid->addWidget(dxfLabelRow,       0, 0);
    viewerGrid->addWidget(m_dxfViewerField,  0, 1);
    viewerGrid->addWidget(dxfViewerBrowse,   0, 2);

    // -----------------------------------------------------------------------
    // Wire DXF "?" icon — explain DXF-ASTM multi-layer format and recommend
    // SolidWorks eDrawings Viewer (correctly renders every layer).
    // -----------------------------------------------------------------------
    connect(dxfHelpBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox box(this);
        box.setWindowTitle(QStringLiteral("DXF Viewer recommendation"));
        box.setIcon(QMessageBox::Information);
        box.setText(QStringLiteral(
            "SeamlyLayout exports DXF in the specialized DXF-ASTM format, "
            "which encodes each pattern piece across multiple layers (seamline, "
            "cutline, notches, grainline, internal paths, labels, etc.).\n\n"
            "Most free DXF viewers show only the first layer or merge layers "
            "incorrectly, which makes the pattern look wrong or incomplete.\n\n"
            "Recommendation: use SolidWorks eDrawings Viewer — a free tool that "
            "correctly renders every DXF-ASTM layer:\n\n"
            "    https://www.edrawingsviewer.com/openview-dwg-and-dxf-files\n\n"
            "After installing, set the DXF Viewer field to the eDrawings "
            "executable path (use Browse…), or paste the URL above to "
            "open eDrawings in your browser.\n\n")
            + taskMgrInstructions(QStringLiteral("DXF Viewer")));
        box.setStandardButtons(QMessageBox::Close);
        box.exec();
    }); // dxfHelpBtn clicked

    // Row 1: PDF Viewer (editable; "?" help icon explains how to use
    // LibreOffice Writer's PDF import filter as the viewer).
    auto *pdfLabelRow = new QWidget;
    pdfLabelRow->setFixedWidth(160);
    auto *pdfLabelLayout = new QHBoxLayout(pdfLabelRow);
    pdfLabelLayout->setContentsMargins(0, 0, 0, 0);
    pdfLabelLayout->setSpacing(4);
    auto *pdfViewerLabel = new QLabel(QStringLiteral("PDF Viewer:"));
    pdfViewerLabel->setStyleSheet(labelStyle);
    auto *pdfHelpBtn = makeHelpIcon();
    pdfLabelLayout->addWidget(pdfViewerLabel);
    pdfLabelLayout->addWidget(pdfHelpBtn);
    pdfLabelLayout->addStretch(1);

    m_pdfViewerField = makeEditableField(viewerPlaceholder);
    auto *pdfViewerBrowse = makeBrowseButton();
    viewerGrid->addWidget(pdfLabelRow,       1, 0);
    viewerGrid->addWidget(m_pdfViewerField,  1, 1);
    viewerGrid->addWidget(pdfViewerBrowse,   1, 2);

    // -----------------------------------------------------------------------
    // Wire PDF "?" icon — recommend LibreOffice Writer (uses its built-in
    // PDF import filter, opening the exported PDF for editing/inspection).
    // -----------------------------------------------------------------------
    connect(pdfHelpBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox box(this);
        box.setWindowTitle(QStringLiteral("PDF Viewer recommendation"));
        box.setIcon(QMessageBox::Information);
        box.setText(QStringLiteral(
            "Any PDF reader works (Adobe Reader, Edge, Chrome, etc.). If you want "
            "to open exported PDFs in an editor, LibreOffice Writer can import a "
            "PDF using its built-in PDF import filter — useful for inspecting or "
            "annotating layouts.\n\n"
            "Recommended values for this field:\n\n"
            "  Simple form (Browse… also produces this):\n"
            "    C:\\Program Files\\LibreOffice\\program\\swriter.exe\n\n"
            "  Or, dispatcher form with the --writer flag (quotes required):\n"
            "    \"C:\\Program Files\\LibreOffice\\program\\soffice.exe\" --writer\n\n"
            "When you select a PDF via View → PDF, the file path is appended as "
            "the final argument; LibreOffice will prompt to import it.\n\n"
            "If LibreOffice is installed elsewhere, browse to it with the "
            "Browse… button or paste the full path manually.\n\n")
            + taskMgrInstructions(QStringLiteral("PDF Viewer")));
        box.setStandardButtons(QMessageBox::Close);
        box.exec();
    }); // pdfHelpBtn clicked

    // Row 2: PNG Viewer (editable; "?" help icon suggests Nomacs / Inkscape
    // and explains the Task Manager flow for finding install paths on Windows).
    auto *pngLabelRow = new QWidget;
    pngLabelRow->setFixedWidth(160);
    auto *pngLabelLayout = new QHBoxLayout(pngLabelRow);
    pngLabelLayout->setContentsMargins(0, 0, 0, 0);
    pngLabelLayout->setSpacing(4);
    auto *pngViewerLabel = new QLabel(QStringLiteral("PNG Viewer:"));
    pngViewerLabel->setStyleSheet(labelStyle);
    auto *pngHelpBtn = makeHelpIcon();
    pngLabelLayout->addWidget(pngViewerLabel);
    pngLabelLayout->addWidget(pngHelpBtn);
    pngLabelLayout->addStretch(1);

    m_pngViewerField = makeEditableField(viewerPlaceholder);
    auto *pngViewerBrowse = makeBrowseButton();
    viewerGrid->addWidget(pngLabelRow,       2, 0);
    viewerGrid->addWidget(m_pngViewerField,  2, 1);
    viewerGrid->addWidget(pngViewerBrowse,   2, 2);

    // -----------------------------------------------------------------------
    // Wire PNG "?" icon — suggest Nomacs / Inkscape as free cross-platform
    // PNG viewers and walk through the Windows Task Manager flow to find
    // the executable path when the user knows the app but not the install path.
    // -----------------------------------------------------------------------
    connect(pngHelpBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox box(this);
        box.setWindowTitle(QStringLiteral("PNG Viewer recommendation"));
        box.setIcon(QMessageBox::Information);
        box.setText(QStringLiteral(
            "Any image viewer that handles PNG files works (Windows Photos, "
            "macOS Preview, eog/feh on Linux, etc.).\n\n"
            "If you don't have a PNG viewer installed, two free cross-platform "
            "options:\n\n"
            "  • Nomacs — fast image viewer (Windows / Linux / macOS):\n"
            "      https://nomacs.org/\n\n"
            "  • Inkscape — vector + raster editor (Windows / Linux / macOS):\n"
            "      https://sourceforge.net/projects/inkscape/\n\n"
            "After installing, set this field to the executable path (use "
            "Browse… or paste the full path).\n\n")
            + taskMgrInstructions(QStringLiteral("PNG Viewer")));
        box.setStandardButtons(QMessageBox::Close);
        box.exec();
    }); // pngHelpBtn clicked

    // Row 3: Projector (label + "?" help icon; editable; Browse picks an .exe)
    auto *projectorLabelRow = new QWidget;
    projectorLabelRow->setFixedWidth(160);
    auto *projectorLabelLayout = new QHBoxLayout(projectorLabelRow);
    projectorLabelLayout->setContentsMargins(0, 0, 0, 0);
    projectorLabelLayout->setSpacing(4);
    auto *projectorLabel = new QLabel(QStringLiteral("Projector:"));
    projectorLabel->setStyleSheet(labelStyle);
    auto *projectorHelpBtn = makeHelpIcon();
    projectorLabelLayout->addWidget(projectorLabel);
    projectorLabelLayout->addWidget(projectorHelpBtn);
    projectorLabelLayout->addStretch(1);

    m_projectorField = makeEditableField(
        QStringLiteral("https://patternprojector.com  or  local exe + args"));
    auto *projectorBrowse = makeBrowseButton();
    viewerGrid->addWidget(projectorLabelRow,  3, 0);
    viewerGrid->addWidget(m_projectorField,   3, 1);
    viewerGrid->addWidget(projectorBrowse,    3, 2);

    viewerGrid->setColumnStretch(1, 1);
    mainLayout->addLayout(viewerGrid);

    // -----------------------------------------------------------------------
    // Wire "?" icon — show Pattern Projector install instructions.
    // -----------------------------------------------------------------------
    connect(projectorHelpBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox box(this);
        box.setWindowTitle(QStringLiteral("Pattern Projector — install instructions"));
        box.setIcon(QMessageBox::Information);
        box.setText(QStringLiteral(
            "1. Install the Pattern Projector viewer from https://patternprojector.com\n\n"
            "2. After install, add the executable to this Preferences field.\n\n"
            "Alternatively, leave the default https://patternprojector.com to launch "
            "the web version in your browser.\n\n")
            + taskMgrInstructions(QStringLiteral("Projector")));
        box.setStandardButtons(QMessageBox::Close);
        box.exec();
    }); // projectorHelpBtn clicked

    // -----------------------------------------------------------------------
    // Wire viewer field editingFinished -> model setter, so typing persists.
    // -----------------------------------------------------------------------
    connect(m_dxfViewerField, &QLineEdit::editingFinished, this, [this]() {
        m_model->setDxfViewerPath(m_dxfViewerField->text());
    });
    connect(m_pdfViewerField, &QLineEdit::editingFinished, this, [this]() {
        m_model->setPdfViewerPath(m_pdfViewerField->text());
    });
    connect(m_pngViewerField, &QLineEdit::editingFinished, this, [this]() {
        m_model->setPngViewerPath(m_pngViewerField->text());
    });
    connect(m_projectorField, &QLineEdit::editingFinished, this, [this]() {
        m_model->setProjectorPath(m_projectorField->text());
    });

    // Spacer pushes footer to the bottom
    mainLayout->addStretch(1);

    // -----------------------------------------------------------------------
    // Footer — Save and Discard buttons
    // -----------------------------------------------------------------------
    auto *footerLayout = new QHBoxLayout;
    footerLayout->setContentsMargins(12, 8, 12, 12);
    footerLayout->addStretch(1);

    auto *resetBtn = makeFooterButton(QStringLiteral("Reset to Defaults"));
    auto *saveBtn = makeFooterButton(QStringLiteral("Save"));
    auto *discardBtn = makeFooterButton(QStringLiteral("Discard"));
    footerLayout->addWidget(resetBtn);
    footerLayout->addSpacing(8);
    footerLayout->addWidget(saveBtn);
    footerLayout->addSpacing(8);
    footerLayout->addWidget(discardBtn);

    mainLayout->addLayout(footerLayout);

    // -----------------------------------------------------------------------
    // Populate fields from model
    // -----------------------------------------------------------------------
    populateFields();

    // -----------------------------------------------------------------------
    // Wire Browse buttons
    // -----------------------------------------------------------------------
    connect(inputDirBrowse, &QPushButton::clicked, this, [this]() {
        browseFolder(QStringLiteral("Select Input SVG Directory"),
                     m_inputDirField,
                     &PreferencesModel::setInputDirectory);
    }); // inputDirBrowse clicked

    connect(layoutDirBrowse, &QPushButton::clicked, this, [this]() {
        browseFolder(QStringLiteral("Select Layout Output Directory"),
                     m_layoutDirField,
                     &PreferencesModel::setLayoutDirectory);
    }); // layoutDirBrowse clicked

    connect(settingsDirBrowse, &QPushButton::clicked, this, [this]() {
        browseFolder(QStringLiteral("Select Settings Directory"),
                     m_settingsDirField,
                     &PreferencesModel::setSettingsDirectory);
    }); // settingsDirBrowse clicked

    connect(settingsFileBrowse, &QPushButton::clicked, this, [this]() {
        browseFile(QStringLiteral("Select Default Settings File"),
                   QStringLiteral("JSON Files (*.json);;All Files (*)"),
                   m_settingsFileField,
                   &PreferencesModel::setSettingsFile);
    }); // settingsFileBrowse clicked

    connect(preferencesFileBrowse, &QPushButton::clicked, this, [this]() {
        browseFile(QStringLiteral("Select Default Preferences File"),
                   QStringLiteral("JSON Files (*.json);;All Files (*)"),
                   m_preferencesFileField,
                   &PreferencesModel::setPreferencesFile);
    }); // preferencesFileBrowse clicked

    connect(dxfViewerBrowse, &QPushButton::clicked, this, [this]() {
#ifdef Q_OS_WIN
        QString filter = QStringLiteral("Executables (*.exe);;All Files (*)");
#else
        QString filter = QStringLiteral("All Files (*)");
#endif
        browseFile(QStringLiteral("Select DXF Viewer Executable"),
                   filter, m_dxfViewerField,
                   &PreferencesModel::setDxfViewerPath);
    }); // dxfViewerBrowse clicked

    connect(pdfViewerBrowse, &QPushButton::clicked, this, [this]() {
#ifdef Q_OS_WIN
        QString filter = QStringLiteral("Executables (*.exe);;All Files (*)");
#else
        QString filter = QStringLiteral("All Files (*)");
#endif
        browseFile(QStringLiteral("Select PDF Viewer Executable"),
                   filter, m_pdfViewerField,
                   &PreferencesModel::setPdfViewerPath);
    }); // pdfViewerBrowse clicked

    connect(pngViewerBrowse, &QPushButton::clicked, this, [this]() {
#ifdef Q_OS_WIN
        QString filter = QStringLiteral("Executables (*.exe);;All Files (*)");
#else
        QString filter = QStringLiteral("All Files (*)");
#endif
        browseFile(QStringLiteral("Select PNG Viewer Executable"),
                   filter, m_pngViewerField,
                   &PreferencesModel::setPngViewerPath);
    }); // pngViewerBrowse clicked

    connect(projectorBrowse, &QPushButton::clicked, this, [this]() {
#ifdef Q_OS_WIN
        QString filter = QStringLiteral("Executables (*.exe);;All Files (*)");
#else
        QString filter = QStringLiteral("All Files (*)");
#endif
        browseFile(QStringLiteral("Select Projector Executable"),
                   filter, m_projectorField,
                   &PreferencesModel::setProjectorPath);
    }); // projectorBrowse clicked

    // -----------------------------------------------------------------------
    // Wire footer buttons
    // -----------------------------------------------------------------------
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        if (!m_model->resetToDefaults()) {
            Logger::log(QStringLiteral("PreferencesWindow: reset to defaults failed"));
            return;
        } // if reset failed

        // Persist active preferences after defaults are applied.
        m_model->save(m_model->defaultPreferencesFilePath());
        populateFields();
        Logger::log(QStringLiteral("PreferencesWindow: reset to defaults applied"));
        emit defaultsReset();
        close();
    }); // resetBtn clicked

    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        m_model->save(m_model->defaultPreferencesFilePath());
        Logger::log(QStringLiteral("PreferencesWindow: saved preferences"));
        emit saved();
        close();
    }); // saveBtn clicked

    connect(discardBtn, &QPushButton::clicked, this, [this]() {
        m_model->load(m_model->defaultPreferencesFilePath());
        populateFields();
        Logger::log(QStringLiteral("PreferencesWindow: discarded changes"));
        emit discarded();
        close();
    }); // discardBtn clicked
} // PreferencesWindow

// ---------------------------------------------------------------------------
// reloadFromModel
// ---------------------------------------------------------------------------

/// @brief Reload all field widgets from the current model state.
void PreferencesWindow::reloadFromModel()
{
    populateFields();
} // reloadFromModel

// ---------------------------------------------------------------------------
// populateFields
// ---------------------------------------------------------------------------

/// @brief Set each line edit's text from the corresponding model property.
void PreferencesWindow::populateFields()
{
    if (!m_model) return;
    m_inputDirField->setText(m_model->inputDirectory());
    m_layoutDirField->setText(m_model->layoutDirectory());
    m_settingsDirField->setText(m_model->settingsDirectory());
    m_settingsFileField->setText(m_model->settingsFile());
    m_preferencesFileField->setText(m_model->preferencesFile());
    m_dxfViewerField->setText(m_model->dxfViewerPath());
    m_pdfViewerField->setText(m_model->pdfViewerPath());
    m_pngViewerField->setText(m_model->pngViewerPath());
    m_projectorField->setText(m_model->projectorPath());
} // populateFields

// ---------------------------------------------------------------------------
// browseFolder
// ---------------------------------------------------------------------------

/// @brief Open a native folder dialog; on accept, update model + field.
void PreferencesWindow::browseFolder(const QString &title, QLineEdit *field,
                                     void (PreferencesModel::*setter)(const QString &))
{
    QString dir = QFileDialog::getExistingDirectory(this, title, field->text());
    if (!dir.isEmpty()) {
        (m_model->*setter)(dir);
        field->setText(dir);
    } // if user selected a directory
} // browseFolder

// ---------------------------------------------------------------------------
// browseFile
// ---------------------------------------------------------------------------

/// @brief Open a native file dialog; on accept, update model + field.
void PreferencesWindow::browseFile(const QString &title, const QString &filter,
                                   QLineEdit *field,
                                   void (PreferencesModel::*setter)(const QString &))
{
    QString path = QFileDialog::getOpenFileName(this, title, field->text(), filter);
    if (!path.isEmpty()) {
        (m_model->*setter)(path);
        field->setText(path);
    } // if user selected a file
} // browseFile
