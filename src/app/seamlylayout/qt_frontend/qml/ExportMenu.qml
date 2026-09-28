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
//
// With showHpgl true (Export menu), an HPGL submenu offers Plot and Cut, a
// pen submenu per line class, and the file extension.
//
// With showPostScript true (Export menu), EPS and PostScript (PS) items are shown.
// The View menu has none: a .ps or .eps file opens in the system viewer.
//
// G-Code and 3D Mesh (3MF) are paid formats. Each item stays hidden until
// AppController.isPaidExportAvailable() reports its module as usable.

pragma ComponentBehavior: Bound

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

    // @brief When true, the HPGL submenu is shown (Export menu only).
    property bool showHpgl: false

    // @brief HPGL pens (1-8) for cut lines, marks and labels; the pen submenus mark them.
    property int hpglCutPen: 1
    property int hpglMarkPen: 2
    property int hpglLabelPen: 3

    // @brief HPGL file extension without the dot: "plt" or "hpgl".
    property string hpglExtension: "plt"

    // @brief When true, the EPS and PostScript (PS) items are shown (Export menu only).
    property bool showPostScript: false

    // @brief True when the paid G-Code module is usable; shows the G-Code item.
    property bool gcodeExportAvailable: false

    // @brief True when the paid 3D mesh module is usable; shows the 3D Mesh item.
    property bool meshExportAvailable: false

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

    // @brief Emitted when the user selects an HPGL export.
    // @param mode "plot" (all lines) or "cut" (cut lines only).
    signal exportHpglRequested(string mode)

    // @brief Emitted when the user picks a pen for one line class.
    // @param lineClass "cut", "mark" or "label".
    // @param pen Pen number 1-8.
    signal hpglPenChosen(string lineClass, int pen)

    // @brief Emitted when the user picks the HPGL file extension ("plt" or "hpgl").
    signal hpglExtensionChosen(string extension)

    // @brief Emitted when the user selects a PostScript export.
    // @param flavor "ps" (printer document) or "eps" (placeable graphic).
    signal exportPostscriptRequested(string flavor)

    // @brief Emitted when the user selects HPGL in the View menu.
    signal viewHpglRequested()

    // @brief Emitted when the user selects Projector (View menu only).
    signal projectorRequested()

    // @brief Emitted when the user selects G-Code export (paid module).
    signal exportGcodeRequested()

    // @brief Emitted when the user selects 3D mesh export (paid module).
    signal exportMeshRequested()

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
        id: epsItem
        text: "EPS"
        enabled: root.layoutReady // enabled when layout is ready
        onTriggered: root.exportPostscriptRequested("eps")
    } // MenuItem EPS

    MenuItem {
        id: psItem
        text: "PostScript (PS)"
        enabled: root.layoutReady // enabled when layout is ready
        onTriggered: root.exportPostscriptRequested("ps")
    } // MenuItem PS

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
            hint: "Labels stay searchable, editable text in the label font.\n"
                + "The font is embedded when its license allows."
        } // SvgModeItem designerFont

        SvgModeItem {
            text: "Text: single-line font"
            mode: "singleLineFont"
            enabled: root.svgTextModesEnabled
            isLast: root.lastSvgTextMode === "singleLineFont"
            onChosen: function(textMode) { root.exportSvgModeRequested(textMode) }
            hint: "Labels stay text, in the single-line font Relief SingleLine CAD.\n"
                + "CAD/CAM tools with that font installed draw one-stroke letters."
        } // SvgModeItem singleLineFont

        SvgModeItem {
            text: "Paths: single-stroke (Hershey)"
            mode: "hersheyStrokes"
            enabled: root.svgTextModesEnabled
            isLast: root.lastSvgTextMode === "hersheyStrokes"
            onChosen: function(textMode) { root.exportSvgModeRequested(textMode) }
            hint: "Labels become single-stroke paths for plotters, cutters and engravers.\n"
                + "No font needed. The text is kept in data-text but is not editable."
        } // SvgModeItem hersheyStrokes

        MenuItem {
            // Only for labels that are already paths: the three modes need <text>.
            text: "As supplied"
            visible: !root.svgTextModesEnabled
            height: visible ? implicitHeight : 0
            ToolTip.visible: hovered
            ToolTip.text:    "The labels in this file are paths, not text, so no text mode applies.\n"
                           + "Exports the layout unchanged."
            ToolTip.delay:   500
            onTriggered: root.exportSvgModeRequested("asSupplied")
        } // MenuItem asSupplied
    } // Menu svgSubmenu

    // @brief Pen submenu for one line class: eight checkable pens, the current one checked.
    // Self-contained: inline components cannot rely on ids of this file.
    component PenMenu: Menu {
        id: penMenu

        // @brief Line class sent with `chosen`: "cut", "mark" or "label".
        required property string lineClass

        // @brief Pen now used for this line class.
        property int currentPen: 1

        // @brief Emitted when the user picks a pen.
        signal chosen(string lineClass, int pen)

        // Menu has no Repeater support; Instantiator inserts the eight items.
        Instantiator {
            model: 8
            delegate: MenuItem {
                id: penItem
                required property int index
                text: "Pen " + (penItem.index + 1)
                checkable: true
                checked: penMenu.currentPen === penItem.index + 1
                onTriggered: {
                    // A click toggles `checked`; rebind it so the mark follows currentPen.
                    penItem.checked = Qt.binding(function() { return penMenu.currentPen === penItem.index + 1 })
                    penMenu.chosen(penMenu.lineClass, penItem.index + 1)
                } // onTriggered
            } // MenuItem penItem
            onObjectAdded: function(index, object) { penMenu.insertItem(index, object) }
            onObjectRemoved: function(index, object) { penMenu.removeItem(object) }
        } // Instantiator
    } // component PenMenu

    // @brief One file-extension choice: checkable, checked when it is the current extension.
    component ExtensionItem: MenuItem {
        id: extensionItem

        // @brief Extension without the dot.
        required property string extension

        // @brief True when this extension is in use.
        property bool isCurrent: false

        // @brief Emitted when the user picks this extension.
        signal chosen(string extension)

        text: "." + extensionItem.extension
        checkable: true
        checked: extensionItem.isCurrent
        onTriggered: {
            // A click toggles `checked`; rebind it so the mark follows isCurrent.
            extensionItem.checked = Qt.binding(function() { return extensionItem.isCurrent })
            extensionItem.chosen(extensionItem.extension)
        } // onTriggered
    } // component ExtensionItem

    Menu {
        id: hpglSubmenu
        title: "HPGL"
        enabled: root.layoutReady

        MenuItem {
            text: "Plot (all lines)"
            ToolTip.visible: hovered
            ToolTip.text:    "Cut lines, marks and labels, each with its own pen.\n"
                           + "Labels are drawn as single strokes."
            ToolTip.delay:   500
            onTriggered: root.exportHpglRequested("plot")
        } // MenuItem plot

        MenuItem {
            text: "Cut (cut lines only)"
            ToolTip.visible: hovered
            ToolTip.text:    "Only cut lines, so a cutter does not cut seam lines or labels."
            ToolTip.delay:   500
            onTriggered: root.exportHpglRequested("cut")
        } // MenuItem cut

        MenuSeparator {}

        PenMenu {
            title: "Cut line pen"
            lineClass: "cut"
            currentPen: root.hpglCutPen
            onChosen: function(lineClass, pen) { root.hpglPenChosen(lineClass, pen) }
        } // PenMenu cut

        PenMenu {
            title: "Mark pen"
            lineClass: "mark"
            currentPen: root.hpglMarkPen
            onChosen: function(lineClass, pen) { root.hpglPenChosen(lineClass, pen) }
        } // PenMenu mark

        PenMenu {
            title: "Label pen"
            lineClass: "label"
            currentPen: root.hpglLabelPen
            onChosen: function(lineClass, pen) { root.hpglPenChosen(lineClass, pen) }
        } // PenMenu label

        Menu {
            title: "File extension"

            ExtensionItem {
                extension: "plt"
                isCurrent: root.hpglExtension === "plt"
                onChosen: function(extension) { root.hpglExtensionChosen(extension) }
            } // ExtensionItem plt

            ExtensionItem {
                extension: "hpgl"
                isCurrent: root.hpglExtension === "hpgl"
                onChosen: function(extension) { root.hpglExtensionChosen(extension) }
            } // ExtensionItem hpgl
        } // Menu file extension
    } // Menu hpglSubmenu

    // View menu only: the Export menu has the HPGL submenu instead.
    MenuItem {
        id:      viewHpglItem
        text:    "HPGL"
        enabled: root.layoutReady
        onTriggered: root.viewHpglRequested()
    } // MenuItem viewHpglItem

    // Paid formats: hidden Menu items keep their height, so collapse them when hidden.
    MenuItem {
        text:    "G-Code"
        visible: root.gcodeExportAvailable
        height:  visible ? implicitHeight : 0
        enabled: root.layoutReady
        onTriggered: root.exportGcodeRequested()
    } // MenuItem G-Code

    MenuItem {
        text:    "3D Mesh (3MF)"
        visible: root.meshExportAvailable
        height:  visible ? implicitHeight : 0
        enabled: root.layoutReady
        onTriggered: root.exportMeshRequested()
    } // MenuItem 3D Mesh

    // Keep one SVG entry: the submenu (Export menu) or the single item (View menu).
    Component.onCompleted: {
        if (root.svgModeSubmenu) {
            root.removeItem(svgItem)
        } else {
            root.removeMenu(svgSubmenu)
        } // if svgModeSubmenu
        // Keep one HPGL entry: the submenu (Export menu) or the view item (View menu).
        if (root.showHpgl) {
            root.removeItem(viewHpglItem)
        } else {
            root.removeMenu(hpglSubmenu)
        } // if showHpgl
        // PostScript items belong to the Export menu only.
        if (!root.showPostScript) {
            root.removeItem(epsItem)
            root.removeItem(psItem)
        } // if !showPostScript
    } // Component.onCompleted

    // Hidden Menu items keep their height; collapse them so the Export menu has no blank row.
    MenuSeparator {
        visible: root.showProjector
        height: visible ? implicitHeight : 0
    } // MenuSeparator projector divider

    MenuItem {
        text:    "Projector"
        visible: root.showProjector
        height:  visible ? implicitHeight : 0
        enabled: root.layoutReady
        onTriggered: root.projectorRequested()
    } // MenuItem Projector
} // Menu root
