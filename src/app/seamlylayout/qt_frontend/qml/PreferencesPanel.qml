// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file PreferencesPanel.qml
// @brief Application preferences dialog — directory and viewer executable paths.
//
// Opens as a modal dialog centered on the ApplicationWindow.
// Save: persists to the application preferences INI file and closes.
// Discard: reloads the application preferences INI file and closes.
//
// Usage:
//   PreferencesPanel {
//       id: preferencesPanel
//       model: preferencesModel    // required PreferencesModel instance
//   }
//   preferencesPanel.open()

import QtQuick 6.11
import QtQuick.Controls 6.11
import QtQuick.Dialogs 6.11
import QtQuick.Layouts 6.11
import SeamlyLayout

Dialog {
    id: root

    // @brief The PreferencesModel instance to read/write.  Must be set by the parent.
    required property var model

    // @brief File path for both save and reload operations.
    readonly property string preferencesPath: root.model
        ? root.model.defaultPreferencesFilePath()
        : ""

    // @brief Shared "Use Task Manager to find install path" instructions
    // block appended at the end of every viewer help popup. The field name
    // (e.g. "DXF Viewer", "Projector") is substituted into the final step.
    function taskMgrInstructions(fieldName) {
        return "If the executable path isn't obvious on Windows, use Task Manager to find it:\n" +
               "    a. Launch the application.\n" +
               "    b. Open Task Manager (Ctrl+Shift+Esc).\n" +
               "    c. Find the application in the Processes list.\n" +
               "    d. Right-click → Open file location.\n" +
               "    e. On the highlighted file in the Explorer window that opens → Right-click → Copy as path.\n" +
               "    f. Paste the path into the " + fieldName + " field below and click Save."
    } // taskMgrInstructions

    title:            "Preferences"
    modal:            true
    width:            620
    height:           500
    anchors.centerIn: parent

    // -----------------------------------------------------------------------
    // Dialog background — SeamlyLayout violet palette
    // -----------------------------------------------------------------------
    background: Rectangle {
        color:        Theme.dialogBackground
        border.color: Theme.violetDark
        radius:       4
    } // background Rectangle

    // -----------------------------------------------------------------------
    // Title bar
    // -----------------------------------------------------------------------
    header: Rectangle {
        height: 40
        color:  Theme.dialogTitleBar
        radius: 4

        Text {
            anchors.centerIn: parent
            text:           root.title
            color:          Theme.dialogTitleText
            font.pixelSize: Theme.fontSizeNormal + 2
            font.bold:      true
        } // Text title
    } // Rectangle header

    // -----------------------------------------------------------------------
    // Form content — two sections with path-picker rows
    // -----------------------------------------------------------------------
    contentItem: ColumnLayout {
        id: formColumn
        spacing: 0

        // -------------------------------------------------------------------
        // Section: Directories
        // -------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 28
            color:  Qt.rgba(1, 1, 1, 0.08)

            Text {
                anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                text:           "Directories"
                color:          Theme.violetLight
                font.pixelSize: Theme.fontSizeSmall
                font.bold:      true
            } // Text sectionLabel
        } // Rectangle directoriesSection

        GridLayout {
            Layout.fillWidth:    true
            Layout.leftMargin:   12
            Layout.rightMargin:  12
            Layout.topMargin:     8
            Layout.bottomMargin:  8
            columns:       3
            columnSpacing: 8
            rowSpacing:    8

            // @brief Input SVG directory — used as starting directory in the import dialog.
            Text {
                text:           "Input SVG Directory:"
                color:          Theme.fieldLabel
                font.pixelSize: Theme.fontSizeNormal
                Layout.preferredWidth: 160
            } // Text inputDirLabel
            TextField {
                id:               inputDirField
                Layout.fillWidth: true
                readOnly:         true
                text:             root.model ? root.model.inputDirectory : ""
                placeholderText:  "(not set)"
                color:            Theme.fieldText
                background:       Rectangle { color: Theme.fieldBackground; radius: 2 }
            } // TextField inputDirField
            SeamlyButton {
                text:         "Browse\u2026"
                implicitWidth: 76
                onClicked:    inputDirDialog.open()
            } // SeamlyButton inputDirBrowse

            // @brief Layout output directory — default save location for exported files.
            Text {
                text:           "Layout Output Directory:"
                color:          Theme.fieldLabel
                font.pixelSize: Theme.fontSizeNormal
                Layout.preferredWidth: 160
            } // Text layoutDirLabel
            TextField {
                id:               layoutDirField
                Layout.fillWidth: true
                readOnly:         true
                text:             root.model ? root.model.layoutDirectory : ""
                placeholderText:  "(not set)"
                color:            Theme.fieldText
                background:       Rectangle { color: Theme.fieldBackground; radius: 2 }
            } // TextField layoutDirField
            SeamlyButton {
                text:         "Browse\u2026"
                implicitWidth: 76
                onClicked:    layoutDirDialog.open()
            } // SeamlyButton layoutDirBrowse

            // @brief Default settings file — pre-loaded when the Settings dialog opens.
            Text {
                text:           "Settings Directory:"
                color:          Theme.fieldLabel
                font.pixelSize: Theme.fontSizeNormal
                Layout.preferredWidth: 160
            } // Text settingsDirLabel
            TextField {
                id:               settingsDirField
                Layout.fillWidth: true
                readOnly:         true
                text:             root.model ? root.model.settingsDirectory : ""
                placeholderText:  "(not set)"
                color:            Theme.fieldText
                background:       Rectangle { color: Theme.fieldBackground; radius: 2 }
            } // TextField settingsDirField
            SeamlyButton {
                text:         "Browse\u2026"
                implicitWidth: 76
                onClicked:    settingsDirDialog.open()
            } // SeamlyButton settingsDirBrowse

            // @brief Default settings file — pre-loaded when the Settings dialog opens.
            Text {
                text:           "Default Settings File:"
                color:          Theme.fieldLabel
                font.pixelSize: Theme.fontSizeNormal
                Layout.preferredWidth: 160
            } // Text settingsFileLabel
            TextField {
                id:               settingsFileField
                Layout.fillWidth: true
                readOnly:         true
                text:             root.model ? root.model.settingsFile : ""
                placeholderText:  "(not set)"
                color:            Theme.fieldText
                background:       Rectangle { color: Theme.fieldBackground; radius: 2 }
            } // TextField settingsFileField
            SeamlyButton {
                text:         "Browse\u2026"
                implicitWidth: 76
                onClicked:    settingsFileDialog.open()
            } // SeamlyButton settingsFileBrowse

            // @brief Default preferences file — used by reset/defaults flows.
            Text {
                text:           "Default Preferences File:"
                color:          Theme.fieldLabel
                font.pixelSize: Theme.fontSizeNormal
                Layout.preferredWidth: 160
            } // Text preferencesFileLabel
            TextField {
                id:               preferencesFileField
                Layout.fillWidth: true
                readOnly:         true
                text:             root.model ? root.model.preferencesFile : ""
                placeholderText:  "(not set)"
                color:            Theme.fieldText
                background:       Rectangle { color: Theme.fieldBackground; radius: 2 }
            } // TextField preferencesFileField
            SeamlyButton {
                text:         "Browse\u2026"
                implicitWidth: 76
                onClicked:    preferencesFileDialog.open()
            } // SeamlyButton preferencesFileBrowse
        } // GridLayout directoriesGrid

        // -------------------------------------------------------------------
        // Section: Viewer Applications
        // -------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 28
            color:  Qt.rgba(1, 1, 1, 0.08)

            Text {
                anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                text:           "Viewer Applications"
                color:          Theme.violetLight
                font.pixelSize: Theme.fontSizeSmall
                font.bold:      true
            } // Text sectionLabel
        } // Rectangle viewersSection

        GridLayout {
            Layout.fillWidth:    true
            Layout.leftMargin:   12
            Layout.rightMargin:  12
            Layout.topMargin:     8
            Layout.bottomMargin: 12
            columns:       3
            columnSpacing: 8
            rowSpacing:    8

            // @brief DXF viewer — opened via "Open in DXF Viewer" after export.
            // Accepts a local executable path OR an https:// URL.
            // The "?" icon opens dxfHelpPopup with the eDrawings recommendation.
            RowLayout {
                Layout.preferredWidth: 160
                spacing: 4
                Text {
                    text:           "DXF Viewer:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                } // Text dxfViewerLabel
                Rectangle {
                    id:     dxfHelpIcon
                    width:  18
                    height: 18
                    radius: 9
                    color:  Theme.violetMedium
                    border.color: Theme.violetDark
                    Text {
                        anchors.centerIn: parent
                        text:           "?"
                        color:          "white"
                        font.bold:      true
                        font.pixelSize: 12
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape:  Qt.PointingHandCursor
                        onClicked:    dxfHelpPopup.open()
                    }
                } // Rectangle dxfHelpIcon
                Item { Layout.fillWidth: true }
            } // RowLayout dxfLabelRow
            TextField {
                id:               dxfViewerField
                Layout.fillWidth: true
                text:             root.model ? root.model.dxfViewerPath : ""
                placeholderText:  "local exe path or https:// URL"
                color:            Theme.fieldText
                background:       Rectangle { color: Theme.fieldBackground; radius: 2 }
                onEditingFinished: { if (root.model) root.model.dxfViewerPath = text }
            } // TextField dxfViewerField
            SeamlyButton {
                text:         "Browse\u2026"
                implicitWidth: 76
                onClicked:    dxfViewerDialog.open()
            } // SeamlyButton dxfViewerBrowse

            // @brief PDF viewer — opened via "Open in PDF Viewer" after export (Phase 10).
            // PDF Viewer label with "?" help icon (opens pdfHelpPopup with
            // LibreOffice Writer recommendation).
            RowLayout {
                Layout.preferredWidth: 160
                spacing: 4
                Text {
                    text:           "PDF Viewer:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                } // Text pdfViewerLabel
                Rectangle {
                    id:     pdfHelpIcon
                    width:  18
                    height: 18
                    radius: 9
                    color:  Theme.violetMedium
                    border.color: Theme.violetDark
                    Text {
                        anchors.centerIn: parent
                        text:           "?"
                        color:          "white"
                        font.bold:      true
                        font.pixelSize: 12
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape:  Qt.PointingHandCursor
                        onClicked:    pdfHelpPopup.open()
                    }
                } // Rectangle pdfHelpIcon
                Item { Layout.fillWidth: true }
            } // RowLayout pdfLabelRow
            TextField {
                id:               pdfViewerField
                Layout.fillWidth: true
                text:             root.model ? root.model.pdfViewerPath : ""
                placeholderText:  "local exe path or https:// URL"
                color:            Theme.fieldText
                background:       Rectangle { color: Theme.fieldBackground; radius: 2 }
                onEditingFinished: { if (root.model) root.model.pdfViewerPath = text }
            } // TextField pdfViewerField
            SeamlyButton {
                text:         "Browse\u2026"
                implicitWidth: 76
                onClicked:    pdfViewerDialog.open()
            } // SeamlyButton pdfViewerBrowse

            // PNG viewer — label with "?" help icon (opens pngHelpPopup with
            // Nomacs / Inkscape recommendation and Task Manager flow).
            RowLayout {
                Layout.preferredWidth: 160
                spacing: 4
                Text {
                    text:           "PNG Viewer:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                } // Text pngViewerLabel
                Rectangle {
                    id:     pngHelpIcon
                    width:  18
                    height: 18
                    radius: 9
                    color:  Theme.violetMedium
                    border.color: Theme.violetDark
                    Text {
                        anchors.centerIn: parent
                        text:           "?"
                        color:          "white"
                        font.bold:      true
                        font.pixelSize: 12
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape:  Qt.PointingHandCursor
                        onClicked:    pngHelpPopup.open()
                    }
                } // Rectangle pngHelpIcon
                Item { Layout.fillWidth: true }
            } // RowLayout pngLabelRow
            TextField {
                id:               pngViewerField
                Layout.fillWidth: true
                text:             root.model ? root.model.pngViewerPath : ""
                placeholderText:  "local exe path or https:// URL"
                color:            Theme.fieldText
                background:       Rectangle { color: Theme.fieldBackground; radius: 2 }
                onEditingFinished: { if (root.model) root.model.pngViewerPath = text }
            } // TextField pngViewerField
            SeamlyButton {
                text:         "Browse\u2026"
                implicitWidth: 76
                onClicked:    pngViewerDialog.open()
            } // SeamlyButton pngViewerBrowse

            // @brief Projector \u2014 accepts an https:// URL (e.g. https://patternprojector.com)
            // OR a local executable, optionally with command-line arguments
            // (e.g. a Chrome PWA shortcut Target). The "?" icon opens install instructions.
            RowLayout {
                Layout.preferredWidth: 160
                spacing: 4
                Text {
                    text:           "Projector:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                } // Text projectorLabel
                Rectangle {
                    id:     projectorHelpIcon
                    width:  18
                    height: 18
                    radius: 9
                    color:  Theme.violetMedium
                    border.color: Theme.violetDark
                    Text {
                        anchors.centerIn: parent
                        text:           "?"
                        color:          "white"
                        font.bold:      true
                        font.pixelSize: 12
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape:  Qt.PointingHandCursor
                        onClicked:    projectorHelpPopup.open()
                    }
                } // Rectangle projectorHelpIcon
                Item { Layout.fillWidth: true }
            } // RowLayout projectorLabelRow
            TextField {
                id:               projectorField
                Layout.fillWidth: true
                text:             root.model ? root.model.projectorPath : ""
                placeholderText:  "https://patternprojector.com  or  local exe + args"
                color:            Theme.fieldText
                background:       Rectangle { color: Theme.fieldBackground; radius: 2 }
                onEditingFinished: { if (root.model) root.model.projectorPath = text }
            } // TextField projectorField
            SeamlyButton {
                text:         "Browse\u2026"
                implicitWidth: 76
                onClicked:    projectorDialog.open()
            } // SeamlyButton projectorBrowse
        } // GridLayout viewersGrid
    } // ColumnLayout formColumn

    // -----------------------------------------------------------------------
    // DXF viewer recommendation help popup.
    // -----------------------------------------------------------------------
    Popup {
        id:     dxfHelpPopup
        modal:  true
        width:  560
        height: 320
        anchors.centerIn: Overlay.overlay

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       4
        }

        contentItem: ColumnLayout {
            spacing: 8
            Text {
                Layout.fillWidth: true
                text:             "DXF Viewer recommendation"
                color:            Theme.violetLight
                font.bold:        true
                font.pixelSize:   Theme.fontSizeNormal + 2
            }
            Text {
                Layout.fillWidth: true
                wrapMode:         Text.WordWrap
                color:            Theme.fieldText
                font.pixelSize:   Theme.fontSizeNormal
                text:             "SeamlyLayout exports DXF in the specialized DXF-ASTM format, " +
                                  "which encodes each pattern piece across multiple layers " +
                                  "(seamline, cutline, notches, grainline, internal paths, labels, etc.).\n\n" +
                                  "Most free DXF viewers show only the first layer or merge layers incorrectly, " +
                                  "which makes the pattern look wrong or incomplete.\n\n" +
                                  "Recommendation: use SolidWorks eDrawings Viewer — a free tool that correctly " +
                                  "renders every DXF-ASTM layer:\n\n" +
                                  "    https://www.edrawingsviewer.com/openview-dwg-and-dxf-files\n\n" +
                                  "After installing, set the DXF Viewer field to the eDrawings executable path " +
                                  "(use Browse…), or paste the URL above to open eDrawings in your browser.\n\n" +
                                  root.taskMgrInstructions("DXF Viewer")
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                SeamlyButton {
                    text:      "Close"
                    onClicked: dxfHelpPopup.close()
                }
            }
        } // contentItem
    } // Popup dxfHelpPopup

    // -----------------------------------------------------------------------
    // PNG viewer recommendation help popup.
    // -----------------------------------------------------------------------
    Popup {
        id:     pngHelpPopup
        modal:  true
        width:  560
        height: 400
        anchors.centerIn: Overlay.overlay

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       4
        }

        contentItem: ColumnLayout {
            spacing: 8
            Text {
                Layout.fillWidth: true
                text:             "PNG Viewer recommendation"
                color:            Theme.violetLight
                font.bold:        true
                font.pixelSize:   Theme.fontSizeNormal + 2
            }
            Text {
                Layout.fillWidth: true
                wrapMode:         Text.WordWrap
                color:            Theme.fieldText
                font.pixelSize:   Theme.fontSizeNormal
                text:             "Any image viewer that handles PNG files works " +
                                  "(Windows Photos, macOS Preview, eog/feh on Linux, etc.).\n\n" +
                                  "If you don't have a PNG viewer installed, two free cross-platform options:\n\n" +
                                  "  • Nomacs — fast image viewer (Windows / Linux / macOS):\n" +
                                  "      https://nomacs.org/\n\n" +
                                  "  • Inkscape — vector + raster editor (Windows / Linux / macOS):\n" +
                                  "      https://sourceforge.net/projects/inkscape/\n\n" +
                                  "After installing, set this field to the executable path " +
                                  "(use Browse… or paste the full path).\n\n" +
                                  root.taskMgrInstructions("PNG Viewer")
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                SeamlyButton {
                    text:      "Close"
                    onClicked: pngHelpPopup.close()
                }
            }
        } // contentItem
    } // Popup pngHelpPopup

    // -----------------------------------------------------------------------
    // PDF viewer recommendation help popup.
    // -----------------------------------------------------------------------
    Popup {
        id:     pdfHelpPopup
        modal:  true
        width:  560
        height: 360
        anchors.centerIn: Overlay.overlay

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       4
        }

        contentItem: ColumnLayout {
            spacing: 8
            Text {
                Layout.fillWidth: true
                text:             "PDF Viewer recommendation"
                color:            Theme.violetLight
                font.bold:        true
                font.pixelSize:   Theme.fontSizeNormal + 2
            }
            Text {
                Layout.fillWidth: true
                wrapMode:         Text.WordWrap
                color:            Theme.fieldText
                font.pixelSize:   Theme.fontSizeNormal
                text:             "Any PDF reader works (Adobe Reader, Edge, Chrome, etc.). " +
                                  "If you want to open exported PDFs in an editor, LibreOffice Writer " +
                                  "can import a PDF using its built-in PDF import filter — useful for " +
                                  "inspecting or annotating layouts.\n\n" +
                                  "Recommended values for this field:\n\n" +
                                  "  Simple form (Browse… also produces this):\n" +
                                  "    C:\\Program Files\\LibreOffice\\program\\swriter.exe\n\n" +
                                  "  Or, dispatcher form with the --writer flag (quotes required):\n" +
                                  "    \"C:\\Program Files\\LibreOffice\\program\\soffice.exe\" --writer\n\n" +
                                  "When you select a PDF via View → PDF, the file path is appended as the " +
                                  "final argument; LibreOffice will prompt to import it.\n\n" +
                                  "If LibreOffice is installed elsewhere, browse to it with the Browse… " +
                                  "button or paste the full path manually.\n\n" +
                                  root.taskMgrInstructions("PDF Viewer")
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                SeamlyButton {
                    text:      "Close"
                    onClicked: pdfHelpPopup.close()
                }
            }
        } // contentItem
    } // Popup pdfHelpPopup

    // -----------------------------------------------------------------------
    // Projector install-instructions help popup.
    // -----------------------------------------------------------------------
    Popup {
        id:     projectorHelpPopup
        modal:  true
        width:  560
        height: 360
        anchors.centerIn: Overlay.overlay

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       4
        }

        contentItem: ColumnLayout {
            spacing: 8
            Text {
                Layout.fillWidth: true
                text:             "Pattern Projector \u2014 install instructions"
                color:            Theme.violetLight
                font.bold:        true
                font.pixelSize:   Theme.fontSizeNormal + 2
            }
            Text {
                Layout.fillWidth: true
                wrapMode:         Text.WordWrap
                color:            Theme.fieldText
                font.pixelSize:   Theme.fontSizeNormal
                text:             "1. Install the Pattern Projector viewer from https://patternprojector.com\n\n" +
                                  "2. After install, add the executable to this Preferences field.\n\n" +
                                  "Alternatively, leave the default https://patternprojector.com to launch " +
                                  "the web version in your browser.\n\n" +
                                  root.taskMgrInstructions("Projector")
            }
            Item { Layout.fillHeight: true }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                SeamlyButton {
                    text:      "Close"
                    onClicked: projectorHelpPopup.close()
                }
            }
        } // contentItem
    } // Popup projectorHelpPopup

    // -----------------------------------------------------------------------
    // Button footer — Reset to Defaults, Save, and Discard using SeamlyButton
    // -----------------------------------------------------------------------
    footer: RowLayout {
        spacing: 8
        Item { Layout.fillWidth: true }

        SeamlyButton {
            text: "Reset to Defaults"
            onClicked: {
                root.resetRequested()
                root.close()
            } // onClicked
        } // SeamlyButton reset

        SeamlyButton {
            text: "Save"
            onClicked: {
                root.accepted()
                root.close()
            } // onClicked
        } // SeamlyButton save

        SeamlyButton {
            text: "Discard"
            onClicked: {
                root.discarded()
                root.close()
            } // onClicked
        } // SeamlyButton discard

        Item { Layout.preferredWidth: 8 }
    } // RowLayout footer

    // -----------------------------------------------------------------------
    // Signals
    // -----------------------------------------------------------------------

    // @brief Emitted when the user clicks Save; PreferencesModel has been persisted.
    signal accepted()

    // @brief Emitted when the user clicks Reset to Defaults; defaults are applied and persisted.
    signal resetRequested()

    // @brief Emitted when the user clicks Discard; PreferencesModel has been reloaded.
    signal discarded()

    // -----------------------------------------------------------------------
    // Handlers — wire Save/Discard to the model
    // -----------------------------------------------------------------------

    onAccepted: {
        if (root.model) root.model.save(root.preferencesPath)
    } // onAccepted

    onResetRequested: {
        if (root.model && root.model.resetToDefaults())
            root.model.save(root.preferencesPath)
    } // onResetRequested

    onDiscarded: {
        if (root.model) root.model.load(root.preferencesPath)
    } // onDiscarded

    // -----------------------------------------------------------------------
    // File / folder dialogs — one per Browse button
    // PreferencesModel.urlToLocalFile() converts file:// URL → local path.
    // -----------------------------------------------------------------------

    // @brief Folder picker for the input SVG directory.
    FolderDialog {
        id: inputDirDialog
        title: "Select Input SVG Directory"
        onAccepted: {
            if (root.model)
                root.model.inputDirectory = root.model.urlToLocalFile(selectedFolder.toString())
        } // onAccepted
    } // FolderDialog inputDirDialog

    // @brief Folder picker for the layout output directory.
    FolderDialog {
        id: layoutDirDialog
        title: "Select Layout Output Directory"
        onAccepted: {
            if (root.model)
                root.model.layoutDirectory = root.model.urlToLocalFile(selectedFolder.toString())
        } // onAccepted
    } // FolderDialog layoutDirDialog

    // @brief File picker for the default settings JSON file.
    FolderDialog {
        id: settingsDirDialog
        title: "Select Settings Directory"
        onAccepted: {
            if (root.model)
                root.model.settingsDirectory = root.model.urlToLocalFile(selectedFolder.toString())
        } // onAccepted
    } // FolderDialog settingsDirDialog

    // @brief File picker for the default settings JSON file.
    FileDialog {
        id: settingsFileDialog
        title:       "Select Default Settings File"
        fileMode:    FileDialog.OpenFile
        nameFilters: ["JSON Files (*.json)", "All Files (*)"]
        onAccepted: {
            if (root.model)
                root.model.settingsFile = root.model.urlToLocalFile(selectedFile.toString())
        } // onAccepted
    } // FileDialog settingsFileDialog

    // @brief File picker for the default preferences JSON file.
    FileDialog {
        id: preferencesFileDialog
        title:       "Select Default Preferences File"
        fileMode:    FileDialog.OpenFile
        nameFilters: ["JSON Files (*.json)", "All Files (*)"]
        onAccepted: {
            if (root.model)
                root.model.preferencesFile = root.model.urlToLocalFile(selectedFile.toString())
        } // onAccepted
    } // FileDialog preferencesFileDialog

    // @brief Executable picker for the DXF viewer.
    // Windows filter shows *.exe; other platforms show all files.
    FileDialog {
        id: dxfViewerDialog
        title:       "Select DXF Viewer Executable"
        fileMode:    FileDialog.OpenFile
        nameFilters: Qt.platform.os === "windows"
                     ? ["Executables (*.exe)", "All Files (*)"]
                     : ["All Files (*)"]
        onAccepted: {
            if (root.model)
                root.model.dxfViewerPath = root.model.urlToLocalFile(selectedFile.toString())
        } // onAccepted
    } // FileDialog dxfViewerDialog

    // @brief Executable picker for the PDF viewer.
    // Windows filter shows *.exe; other platforms show all files.
    FileDialog {
        id: pdfViewerDialog
        title:       "Select PDF Viewer Executable"
        fileMode:    FileDialog.OpenFile
        nameFilters: Qt.platform.os === "windows"
                     ? ["Executables (*.exe)", "All Files (*)"]
                     : ["All Files (*)"]
        onAccepted: {
            if (root.model)
                root.model.pdfViewerPath = root.model.urlToLocalFile(selectedFile.toString())
        } // onAccepted
    } // FileDialog pdfViewerDialog

    // @brief Executable picker for the PNG viewer.
    // Windows filter shows *.exe; other platforms show all files.
    FileDialog {
        id: pngViewerDialog
        title:       "Select PNG Viewer Executable"
        fileMode:    FileDialog.OpenFile
        nameFilters: Qt.platform.os === "windows"
                     ? ["Executables (*.exe)", "All Files (*)"]
                     : ["All Files (*)"]
        onAccepted: {
            if (root.model)
                root.model.pngViewerPath = root.model.urlToLocalFile(selectedFile.toString())
        } // onAccepted
    } // FileDialog pngViewerDialog

    // @brief Executable picker for the Projector application.
    // For Chrome PWA shortcuts the user should paste the Target string manually
    // (the file picker only returns a path); see the "?" help popup.
    FileDialog {
        id: projectorDialog
        title:       "Select Projector Executable"
        fileMode:    FileDialog.OpenFile
        nameFilters: Qt.platform.os === "windows"
                     ? ["Executables (*.exe)", "All Files (*)"]
                     : ["All Files (*)"]
        onAccepted: {
            if (root.model)
                root.model.projectorPath = root.model.urlToLocalFile(selectedFile.toString())
        } // onAccepted
    } // FileDialog projectorDialog
} // Dialog root
