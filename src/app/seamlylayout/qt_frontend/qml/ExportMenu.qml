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
//
// With svgModeSubmenu true (Export menu), "SVG" is a submenu of label text
// modes; otherwise (View menu) it is one item.

import QtQuick 6.11
import QtQuick.Controls 6.11

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

    // @brief When true, SVG opens a submenu of label text modes (Export menu only).
    property bool svgModeSubmenu: false

    // @brief Label text of the imported SVG: "text", "pathsOnly" or "noLabels".
    property string labelTextState: "noLabels"

    // @brief Last SVG text mode used; its submenu item shows a check mark.
    property string lastSvgTextMode: ""

    // @brief True when the three text modes apply; false when labels are already paths.
    readonly property bool svgTextModesEnabled: root.labelTextState !== "pathsOnly"

    // @brief Emitted when the user selects DXF-ASTM export (Phase 9).
    signal exportDxfAstmRequested()

    // @brief Emitted when the user selects PDF export (Phase 10).
    signal exportPdfRequested()

    // @brief Emitted when the user selects tiled PDF export (Phase 10).
    signal exportPdfTiledRequested()

    // @brief Emitted when the user selects PNG export (Phase 11).
    signal exportPngRequested()

    // @brief Emitted when the user selects the single SVG item (View menu).
    signal exportSvgRequested()

    // @brief Emitted when the user selects an SVG text mode (Export menu).
    // @param mode "designerFont", "singleLineFont", "hersheyStrokes" or "asSupplied".
    signal exportSvgModeRequested(string mode)

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
        id: svgItem
        text: "SVG"
        enabled: root.layoutReady // enabled when layout is ready
        onTriggered: root.exportSvgRequested()
    } // MenuItem SVG

    // @brief One SVG text mode: checkable item with a tooltip.
    // Self-contained: inline components cannot rely on ids of this file.
    component SvgModeItem: MenuItem {
        id: modeItem

        // @brief Mode name sent with `chosen`.
        required property string mode

        // @brief Trade-off summary shown on hover.
        required property string hint

        // @brief True when this mode was the last one used; shows the check mark.
        property bool isLast: false

        // @brief Emitted when the user picks this mode.
        signal chosen(string textMode)

        checkable: true
        checked: modeItem.isLast
        ToolTip.visible: modeItem.hovered
        ToolTip.text:    modeItem.hint
        ToolTip.delay:   500
        onTriggered: {
            // A click toggles `checked`; rebind it so the mark follows isLast.
            modeItem.checked = Qt.binding(function() { return modeItem.isLast })
            modeItem.chosen(modeItem.mode)
        } // onTriggered
    } // component SvgModeItem

    Menu {
        id: svgSubmenu
        title: "SVG"
        enabled: root.layoutReady

        SvgModeItem {
            text: "Text: designer font"
            mode: "designerFont"
            enabled: root.svgTextModesEnabled
            isLast: root.lastSvgTextMode === "designerFont"
            onChosen: function(textMode) { root.exportSvgModeRequested(textMode) }
            hint: "Labels stay searchable, editable text in the label font. "
                + "The font is embedded when its license allows."
        } // SvgModeItem designerFont

        SvgModeItem {
            text: "Text: single-line font"
            mode: "singleLineFont"
            enabled: root.svgTextModesEnabled
            isLast: root.lastSvgTextMode === "singleLineFont"
            onChosen: function(textMode) { root.exportSvgModeRequested(textMode) }
            hint: "Labels stay text, in the single-line font Relief SingleLine CAD. "
                + "CAD/CAM tools with that font installed draw one-stroke letters."
        } // SvgModeItem singleLineFont

        SvgModeItem {
            text: "Paths: single-stroke (Hershey)"
            mode: "hersheyStrokes"
            enabled: root.svgTextModesEnabled
            isLast: root.lastSvgTextMode === "hersheyStrokes"
            onChosen: function(textMode) { root.exportSvgModeRequested(textMode) }
            hint: "Labels become single-stroke paths for plotters, cutters and engravers. "
                + "No font needed. The text is kept in data-text but is not editable."
        } // SvgModeItem hersheyStrokes

        MenuItem {
            // Only for labels that are already paths: the three modes need <text>.
            text: "As supplied (labels are already paths)"
            visible: !root.svgTextModesEnabled
            height: visible ? implicitHeight : 0
            ToolTip.visible: hovered
            ToolTip.text:    "The labels in this file are paths, not text, so no text mode applies. "
                           + "Exports the layout unchanged."
            ToolTip.delay:   500
            onTriggered: root.exportSvgModeRequested("asSupplied")
        } // MenuItem asSupplied
    } // Menu svgSubmenu

    // Keep one SVG entry: the submenu (Export menu) or the single item (View menu).
    Component.onCompleted: {
        if (root.svgModeSubmenu) {
            root.removeItem(svgItem)
        } else {
            root.removeMenu(svgSubmenu)
        } // if svgModeSubmenu
    } // Component.onCompleted

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
