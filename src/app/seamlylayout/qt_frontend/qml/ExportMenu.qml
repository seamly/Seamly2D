// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file ExportMenu.qml
// @brief Export dropdown menu with all supported output formats.
//
// Usage:
//   ExportMenu {
//       id: exportMenu
//   }
//   // To open: exportMenu.popup(anchorItem, 0, anchorItem.height)
//
// Items are disabled until their respective phases are implemented:
//   DXF-ASTM — Phase 9
//   PDF       — Phase 10
//   PDF Tiled — Phase 10
//   PNG       — Phase 11
//   SVG       — Phase 11

import QtQuick 6.10
import QtQuick.Controls 6.10

Menu {
    id: root

    // @brief True when a layout is ready; enables the DXF-ASTM export item.
    required property bool layoutReady

    // @brief When true, a Projector item is appended to the menu (View menu only).
    property bool showProjector: false

    // @brief Controls whether the PDF (Tiled) item is enabled by settings state.
    // Export menu binds this to (settings.paperType === "tiled").
    // View menu can keep this true because it opens already-exported files.
    property bool pdfTiledEnabled: true

    // @brief Emitted when the user selects DXF-ASTM export (Phase 9).
    signal exportDxfAstmRequested()

    // @brief Emitted when the user selects PDF export (Phase 10).
    signal exportPdfRequested()

    // @brief Emitted when the user selects tiled PDF export (Phase 10).
    signal exportPdfTiledRequested()

    // @brief Emitted when the user selects PNG export (Phase 11).
    signal exportPngRequested()

    // @brief Emitted when the user selects SVG export (Phase 11).
    signal exportSvgRequested()

    // @brief Emitted when the user selects Projector (View menu only).
    signal projectorRequested()

    MenuItem {
        text: "DXF-ASTM"
        enabled: root.layoutReady // Phase 9: enabled when layout is ready
        onTriggered: root.exportDxfAstmRequested()
    } // MenuItem DXF-ASTM

    MenuItem {
        text: "PDF"
        enabled: root.layoutReady // Phase 10: enabled when layout is ready
        onTriggered: root.exportPdfRequested()
    } // MenuItem PDF

    MenuItem {
        text: "PDF (Tiled)"
        enabled: root.layoutReady && root.pdfTiledEnabled
        onTriggered: root.exportPdfTiledRequested()
    } // MenuItem PDF Tiled

    MenuItem {
        text: "PNG"
        enabled: root.layoutReady // enabled when layout is ready
        onTriggered: root.exportPngRequested()
    } // MenuItem PNG

    MenuItem {
        text: "SVG"
        enabled: root.layoutReady // enabled when layout is ready
        onTriggered: root.exportSvgRequested()
    } // MenuItem SVG

    MenuSeparator {
        visible: root.showProjector
    } // MenuSeparator projector divider

    MenuItem {
        text:    "Projector"
        visible: root.showProjector
        enabled: root.layoutReady
        onTriggered: root.projectorRequested()
    } // MenuItem Projector
} // Menu root
