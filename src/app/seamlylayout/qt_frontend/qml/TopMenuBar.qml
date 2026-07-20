// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file TopMenuBar.qml
// @brief Application toolbar — Import, Layout Settings, Create Layout,
//        Adjust Layout, Export dropdown, Preferences gear, Exit.
//
// Usage:
//   TopMenuBar {
//       svgImported:  appController.importedSvgPath !== ""
//       layoutReady:  appController.isLayoutReady
//       onImportClicked:         importDialog.open()
//       onLayoutSettingsClicked: layoutSettingsPanel.visible = !layoutSettingsPanel.visible
//       onCreateLayoutClicked:   { /* Phase 8 */ }
//       onPreferencesClicked:    preferencesPanel.visible = !preferencesPanel.visible
//   }

import QtQuick 6.10
import QtQuick.Controls 6.10
import QtQuick.Layouts 6.10
import SeamlyLayout

ToolBar {
    id: root

    // -----------------------------------------------------------------------
    // Input properties — drive button enabled states from outside
    // -----------------------------------------------------------------------

    // @brief True when an SVG has been imported; enables Create Layout button.
    required property bool svgImported

    // @brief True when a layout is ready to export; enables Export button.
    required property bool layoutReady

    // @brief True when Create Layout should be enabled (Settings submitted, not yet run).
    required property bool createLayoutEnabled

    // @brief True while AdjustMode is active — disables all buttons except Adjust Layout.
    // Adjust Layout itself is also disabled while in adjust mode (already active).
    required property bool adjustMode

    // @brief True when Export should allow "PDF (Tiled)" selection.
    // Bound by Main.qml from settingsModel.paperType === "tiled".
    required property bool pdfTiledExportEnabled

    // -----------------------------------------------------------------------
    // Signals — parent wires these to open dialogs / toggle panels
    // -----------------------------------------------------------------------

    // @brief User clicked Import SVG.
    signal importClicked()

    // @brief User clicked Layout Settings.
    signal layoutSettingsClicked()

    // @brief User clicked Create Layout.
    signal createLayoutClicked()

    // @brief User clicked Adjust Layout.
    signal adjustLayoutClicked()

    // @brief User clicked Export (the button itself, not a menu item).
    // The Export dropdown menu is handled internally.
    signal exportClicked()

    // @brief User selected DXF-ASTM from the Export dropdown (Phase 9).
    signal exportDxfAstmRequested()

    // @brief User selected PDF from the Export dropdown (Phase 10).
    signal exportPdfRequested()

    // @brief User selected PDF (Tiled) from the Export dropdown (Phase 10).
    signal exportPdfTiledRequested()

    // @brief User selected PNG from the Export dropdown.
    signal exportPngRequested()

    // @brief User selected SVG from the Export dropdown.
    signal exportSvgRequested()

    // @brief User selected DXF-ASTM from the View dropdown.
    signal viewDxfAstmRequested()

    // @brief User selected PDF from the View dropdown.
    signal viewPdfRequested()

    // @brief User selected PDF (Tiled) from the View dropdown.
    signal viewPdfTiledRequested()

    // @brief User selected PNG from the View dropdown.
    signal viewPngRequested()

    // @brief User selected SVG from the View dropdown.
    signal viewSvgRequested()

    // @brief User selected Projector from the View dropdown.
    signal viewProjectorRequested()

    // @brief User clicked the Preferences gear.
    signal preferencesClicked()

    // -----------------------------------------------------------------------
    // Toolbar background
    // -----------------------------------------------------------------------
    background: Rectangle {
        anchors.fill: parent
        color: Theme.titleBarBackground
    } // background Rectangle

    // -----------------------------------------------------------------------
    // Button row
    // -----------------------------------------------------------------------
    RowLayout {
        anchors.fill: parent
        spacing: 8

        // Left margin
        Item { Layout.preferredWidth: 8 }

        // Import button — disabled while AdjustMode is active
        SeamlyButton {
            id: importButton
            text: "Import SVG"
            enabled: !root.adjustMode
            onClicked: root.importClicked()
        } // SeamlyButton importButton

        // Layout Settings button — disabled until SVG imported and not in AdjustMode
        SeamlyButton {
            id: layoutSettingsButton
            text: "Layout Settings"
            enabled: root.svgImported && !root.adjustMode
            onClicked: root.layoutSettingsClicked()
        } // SeamlyButton layoutSettingsButton

        // Create Layout button — disabled while AdjustMode is active
        SeamlyButton {
            id: createLayoutButton
            text: "Create Layout"
            enabled: root.createLayoutEnabled && !root.adjustMode
            onClicked: root.createLayoutClicked()
        } // SeamlyButton createLayoutButton

        // Adjust Layout button — disabled while AdjustMode is already active
        SeamlyButton {
            id: adjustLayoutButton
            text: "Adjust Layout"
            enabled: root.layoutReady && !root.adjustMode
            onClicked: root.adjustLayoutClicked()
        } // SeamlyButton adjustLayoutButton

        // Export button — disabled while AdjustMode is active
        SeamlyButton {
            id: exportButton
            text: "Export \u25BC"
            enabled: root.layoutReady && !root.adjustMode
            onClicked: exportMenu.popup(exportButton, 0, exportButton.height)
        } // SeamlyButton exportButton

        // Export dropdown — DXF-ASTM enabled in Phase 9; PDF/PNG/SVG in Phase 10–11
        ExportMenu {
            id: exportMenu
            layoutReady: root.layoutReady
            pdfTiledEnabled: root.pdfTiledExportEnabled
            onExportDxfAstmRequested:  root.exportDxfAstmRequested()
            onExportPdfRequested:      root.exportPdfRequested()
            onExportPdfTiledRequested: root.exportPdfTiledRequested()
            onExportPngRequested:      root.exportPngRequested()
            onExportSvgRequested:      root.exportSvgRequested()
        } // ExportMenu exportMenu

        // View button — opens exported files in configured viewer applications
        SeamlyButton {
            id: viewButton
            text: "View \u25BC"
            enabled: root.layoutReady && !root.adjustMode
            onClicked: viewMenu.popup(viewButton, 0, viewButton.height)
        } // SeamlyButton viewButton

        // View dropdown — reuses ExportMenu component so menu text is shared;
        // showProjector adds the Projector item (View menu only).
        ExportMenu {
            id: viewMenu
            layoutReady:    root.layoutReady
            showProjector:  true
            pdfTiledEnabled: true
            onExportDxfAstmRequested:  root.viewDxfAstmRequested()
            onExportPdfRequested:      root.viewPdfRequested()
            onExportPdfTiledRequested: root.viewPdfTiledRequested()
            onExportPngRequested:      root.viewPngRequested()
            onExportSvgRequested:      root.viewSvgRequested()
            onProjectorRequested:      root.viewProjectorRequested()
        } // ExportMenu viewMenu

        // Spacer — pushes Preferences and Exit to the right
        Item { Layout.fillWidth: true }

        // Preferences button — gear icon; toggles preferences panel
        // (Phase 6 replaces toggle with PreferencesPanel)
        SeamlyButton {
            id: preferencesButton
            text: "\u2699"
            implicitWidth: 36
            onClicked: root.preferencesClicked()
        } // SeamlyButton preferencesButton

        // Exit button
        SeamlyButton {
            id: exitButton
            text: "Exit"
            onClicked: Qt.quit()
        } // SeamlyButton exitButton

        // Right margin
        Item { Layout.preferredWidth: 8 }
    } // RowLayout
} // ToolBar root
