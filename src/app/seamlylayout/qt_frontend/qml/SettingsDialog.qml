// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file SettingsDialog.qml
// @brief Layout Settings dialog — edit all LayoutSettings fields.
//
// Opens as a modal dialog centered on the ApplicationWindow.
// Submit: persists to the resolved settings JSON path and closes.
// Discard: reloads from the resolved settings JSON path (reverts in-dialog edits) and closes.
//
// Usage:
//   SettingsDialog {
//       id: settingsDialog
//       model: settingsModel    // required SettingsModel instance
//   }
//   settingsDialog.open()

import QtQuick 6.11
import QtQuick.Controls 6.11
import QtQuick.Dialogs 6.11
import QtQuick.Layouts 6.11
import SeamlyLayout

Dialog {
    id: root

    // @brief The SettingsModel instance to read/write.  Must be set by the parent.
    required property var model

    // @brief File used for Submit (save) and Discard (reload).
    // Set by the parent before open() to the resolved settings file path.
    // Falls back to SettingsModel.defaultSettingsFilePath().
    property string settingsPath: model ? model.defaultSettingsFilePath() : ""

    // @brief Preferred folder for load/save settings file dialogs.
    // Set by the parent from PreferencesModel.resolvedSettingsDirectory().
    // Falls back to SettingsModel.settingsFolderUrl() when not provided.
    property string settingsFolderUrl: ""

    // @brief True when mediaType is "paper" and paperType is "tiled".
    // Used to hide fields that are irrelevant for tiled output:
    // Paper Size, Page Size, and the Fabric section.
    readonly property bool isTiledPaper: root.model
        ? (root.model.mediaType === "paper" && root.model.paperType === "tiled")
        : false
    readonly property bool showFabricSection: root.model
        ? root.model.mediaType === "fabric"
        : false
    readonly property bool showMarginsSection: root.model
        ? root.model.mediaType !== "fabric"
        : true

    title:           "Layout Settings"
    modal:           true
    width:           520
    readonly property real dialogChromeHeight:
        (header && header.visible ? header.height : 0)
        + (footer && footer.visible ? footer.implicitHeight : 0)
        + topPadding + bottomPadding + 24
    readonly property real maxDialogHeight:
        parent ? Math.max(320, parent.height - 48) : 820
    readonly property real desiredDialogHeight:
        scrollView.contentHeight + dialogChromeHeight
    height:          Math.min(maxDialogHeight, desiredDialogHeight)
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
    // Form content — scrollable GridLayout
    // -----------------------------------------------------------------------
    contentItem: ScrollView {
        id: scrollView
        // Explicitly bind contentHeight so Qt always knows the true ColumnLayout height,
        // even when items are hidden/shown dynamically via isTiledPaper and other visibility
        // bindings.  Without this, contentHeight can be stale or zero when the dialog opens
        // with tiled mode already active, causing the dialog to be too short and the footer
        // to overlap the last content row.
        contentHeight:  formColumn.implicitHeight
        implicitHeight: Math.min(formColumn.implicitHeight, root.maxDialogHeight - root.dialogChromeHeight)
        clip:           true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        // @brief Fixed label column width for alignment.
        readonly property int labelWidth: 150

        ColumnLayout {
            id: formColumn
            width: scrollView.availableWidth
            spacing: 0

            // -----------------------------------------------------------
            // Section: Layout Options
            // -----------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                height: 28
                color:  Qt.rgba(1, 1, 1, 0.08)

                Text {
                    anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                    text:           "Layout Options"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold:      true
                } // Text sectionLabel
            } // Rectangle layoutOptionsSection

            GridLayout {
                Layout.fillWidth: true
                Layout.leftMargin:  12
                Layout.rightMargin: 12
                Layout.topMargin:    8
                Layout.bottomMargin: 8
                columns:      2
                columnSpacing: 8
                rowSpacing:    8

                // -----------------------------------------------------------
                // Layout mode — two radio buttons (mutually exclusive):
                //   Along Grainline | With Nap
                // Bound to root.model.layoutMode ∈ {"alongGrainline","withNap"}.
                // -----------------------------------------------------------
                Text {
                    text:           "Layout Mode:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                    verticalAlignment: Text.AlignVCenter
                } // Text layoutModeLabel
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    ButtonGroup { id: layoutModeGroup }

                    RadioButton {
                        id:      layoutModeAlongGrain
                        text:    "Along Grainline"
                        ButtonGroup.group: layoutModeGroup
                        checked: root.model ? root.model.layoutMode === "alongGrainline" : true
                        onToggled: if (checked && root.model) root.model.layoutMode = "alongGrainline"

                        contentItem: Text {
                            text:           layoutModeAlongGrain.text
                            color:          Theme.textOnDark
                            font.pixelSize: Theme.fontSizeNormal
                            leftPadding:    layoutModeAlongGrain.indicator.width + layoutModeAlongGrain.spacing
                            verticalAlignment: Text.AlignVCenter
                        } // contentItem Text
                    } // RadioButton alongGrainline

                    RadioButton {
                        id:      layoutModeWithNap
                        text:    "With Nap"
                        ButtonGroup.group: layoutModeGroup
                        checked: root.model ? root.model.layoutMode === "withNap" : false
                        onToggled: if (checked && root.model) root.model.layoutMode = "withNap"

                        contentItem: Text {
                            text:           layoutModeWithNap.text
                            color:          Theme.textOnDark
                            font.pixelSize: Theme.fontSizeNormal
                            leftPadding:    layoutModeWithNap.indicator.width + layoutModeWithNap.spacing
                            verticalAlignment: Text.AlignVCenter
                        } // contentItem Text
                    } // RadioButton withNap

                } // RowLayout layoutModeButtons

                // -----------------------------------------------------------
                // Nap direction — visible only when layoutMode == "withNap".
                // Two radios: Pieces point Up (rotationStep=0) | Pieces point Down (rotationStep=180).
                // For napped/directional fabrics the user picks which way every
                // piece must face; both options keep the trial set a singleton.
                // -----------------------------------------------------------
                Text {
                    visible:        root.model ? root.model.layoutMode === "withNap" : false
                    text:           "Nap Direction:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                    verticalAlignment: Text.AlignVCenter
                } // Text napDirectionLabel
                RowLayout {
                    visible:           root.model ? root.model.layoutMode === "withNap" : false
                    Layout.fillWidth:  true
                    spacing: 16

                    ButtonGroup { id: napDirectionGroup }

                    RadioButton {
                        id:      napUp
                        text:    "Pieces point Up"
                        ButtonGroup.group: napDirectionGroup
                        checked: root.model ? Math.abs(root.model.rotationStep - 0.0) < 0.001 : true
                        onToggled: if (checked && root.model) root.model.rotationStep = 0.0

                        contentItem: Text {
                            text:           napUp.text
                            color:          Theme.textOnDark
                            font.pixelSize: Theme.fontSizeNormal
                            leftPadding:    napUp.indicator.width + napUp.spacing
                            verticalAlignment: Text.AlignVCenter
                        } // contentItem Text
                    } // RadioButton napUp

                    RadioButton {
                        id:      napDown
                        text:    "Pieces point Down"
                        ButtonGroup.group: napDirectionGroup
                        checked: root.model ? Math.abs(root.model.rotationStep - 180.0) < 0.001 : false
                        onToggled: if (checked && root.model) root.model.rotationStep = 180.0

                        contentItem: Text {
                            text:           napDown.text
                            color:          Theme.textOnDark
                            font.pixelSize: Theme.fontSizeNormal
                            leftPadding:    napDown.indicator.width + napDown.spacing
                            verticalAlignment: Text.AlignVCenter
                        } // contentItem Text
                    } // RadioButton napDown
                } // RowLayout napDirectionButtons

                // -----------------------------------------------------------
                // Piece gap — minimum clearance between adjacent placed
                // pieces, expressed in the active unit (in/cm/mm).  Stored
                // as `pieceGap` and projected to pixels via `pieceGapPx`
                // for the Rust packer.
                //
                // SpinBox limitation: integer-only `value`/`from`/`to`.  We
                // scale by 100 internally (display = value / 100) so the
                // user can dial decimals like 0.05.  Step = 1 unit ⇒ 0.01
                // increments per click, fine-grained enough for clearance
                // tweaks across in/mm/cm.
                // -----------------------------------------------------------
                Text {
                    text:           "Piece Gap:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                    verticalAlignment: Text.AlignVCenter
                } // Text pieceGapLabel
                SpinBox {
                    id: pieceGapSpin
                    Layout.preferredWidth: 140
                    from:     0
                    to:       9999       // → 99.99 in active units
                    stepSize: 1          // → 0.01 increments
                    editable: true
                    value:    root.model ? Math.round(root.model.pieceGap * 100) : 5

                    // Keep the SpinBox in sync if the model changes externally
                    // (loading a preset, switching units via convertAllUnits).
                    Binding {
                        target:   pieceGapSpin
                        property: "value"
                        value:    root.model ? Math.round(root.model.pieceGap * 100) : 5
                        when:     root.model !== null
                    } // Binding pieceGap → spin

                    property int decimals: 2

                    validator: DoubleValidator {
                        bottom:   0.0
                        top:      99.99
                        decimals: pieceGapSpin.decimals
                        notation: DoubleValidator.StandardNotation
                    } // DoubleValidator

                    textFromValue: function(value, locale) {
                        return Number(value / 100).toLocaleString(locale, 'f', decimals);
                    }
                    valueFromText: function(text, locale) {
                        return Math.round(Number.fromLocaleString(locale, text) * 100);
                    }

                    onValueModified: if (root.model) root.model.pieceGap = value / 100.0
                } // SpinBox pieceGap

                // Piece Gap guidance — spans both columns, below the SpinBox.
                // Covers layout orientation, Adjust usage, and gap-size runtime tradeoff.
                // Right-column layout was evaluated and rejected: the dialog is 520 px wide and
                // a side-by-side text column next to the 140 px SpinBox would be too narrow for
                // multi-line prose.  A full-width hint row in the scrollable dialog is cleaner.
                Text {
                    Layout.columnSpan:  2
                    Layout.fillWidth:   true
                    Layout.topMargin:   2
                    Layout.bottomMargin: 2
                    wrapMode:           Text.WordWrap
                    font.pixelSize:     Theme.fontSizeSmall
                    color:              Theme.fieldLabel
                    opacity:            0.80
                    // @brief Three-bullet hint: orientation, Adjust usage, runtime tradeoff.
                    text: "Tip: minimum clearance kept between pieces during auto-layout.\n"
                        + "• Along Grainline tries 0°/180° rotations for tighter nesting; "
                        + "With Nap pins every piece to one direction (use for directional or napped fabrics).\n"
                        + "• After layout, open Adjust mode to fine-tune placement by hand.\n"
                        + "• Smaller gap = faster layout run — 0.05–0.10 in (1–3 mm) is typical."
                } // Text pieceGapGuidance

            } // GridLayout layoutOptionsGrid

            // -----------------------------------------------------------
            // Section: Media
            // -----------------------------------------------------------
            Rectangle {
                Layout.fillWidth: true
                height: 28
                color:  Qt.rgba(1, 1, 1, 0.08)

                Text {
                    anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                    text:           "Media"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold:      true
                } // Text sectionLabel
            } // Rectangle mediaSection

            GridLayout {
                Layout.fillWidth:  true
                Layout.leftMargin:  12
                Layout.rightMargin: 12
                Layout.topMargin:    8
                Layout.bottomMargin: 8
                columns:      2
                columnSpacing: 8
                rowSpacing:    8

                // Unit selector — first field in Media section so the unit
                // chosen here governs every dimension below it.
                Text {
                    text:           "Unit:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                    verticalAlignment: Text.AlignVCenter
                } // Text unitLabel
                ComboBox {
                    id:               unitCombo
                    Layout.fillWidth: true
                    model:            root.model ? root.model.unitNames : []
                    currentIndex:     root.model ? model.indexOf(root.model.unit) : 0

                    onActivated: {
                        if (!root.model) return;
                        var oldUnit = root.model.unit;
                        var newUnit = currentText;
                        if (oldUnit !== newUnit) {
                            root.model.convertAllUnits(oldUnit, newUnit);
                            root.model.unit = newUnit;
                        } // if unit changed
                    } // onActivated
                } // ComboBox unitCombo

                // Media type — radio buttons: Paper | Fabric
                Text {
                    text:           "Media Type:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                    verticalAlignment: Text.AlignVCenter
                } // Text mediaTypeLabel
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 16

                    RadioButton {
                        id:      mediaTypePaper
                        text:    "Paper"
                        checked: root.model ? root.model.mediaType === "paper" : true
                        onToggled: if (checked && root.model) root.model.mediaType = "paper"

                        contentItem: Text {
                            text:           mediaTypePaper.text
                            color:          Theme.textOnDark
                            font.pixelSize: Theme.fontSizeNormal
                            leftPadding:    mediaTypePaper.indicator.width + mediaTypePaper.spacing
                            verticalAlignment: Text.AlignVCenter
                        } // contentItem Text
                    } // RadioButton paper

                    RadioButton {
                        id:      mediaTypeFabric
                        text:    "Fabric"
                        checked: root.model ? root.model.mediaType === "fabric" : false
                        onToggled: if (checked && root.model) root.model.mediaType = "fabric"

                        contentItem: Text {
                            text:           mediaTypeFabric.text
                            color:          Theme.textOnDark
                            font.pixelSize: Theme.fontSizeNormal
                            leftPadding:    mediaTypeFabric.indicator.width + mediaTypeFabric.spacing
                            verticalAlignment: Text.AlignVCenter
                        } // contentItem Text
                    } // RadioButton fabric
                } // RowLayout mediaTypeButtons

                // Paper type — only when mediaType == "paper"
                Text {
                    visible:        root.model ? root.model.mediaType === "paper" : true
                    text:           "Paper Type:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                } // Text paperTypeLabel
                ComboBox {
                    id:               paperTypeCombo
                    visible:          root.model ? root.model.mediaType === "paper" : true
                    Layout.fillWidth: true
                    model:            ["sheet", "tiled", "roll"]
                    currentIndex: {
                        if (!root.model) return 0
                        if (root.model.paperType === "tiled") return 1
                        if (root.model.paperType === "roll")  return 2
                        return 0 // "sheet"
                    } // currentIndex
                    onActivated:      if (root.model) root.model.paperType = currentText
                } // ComboBox paperType

                // Paper size — sheet only (hidden for tiled and roll)
                Text {
                    visible:        root.model ? (root.model.mediaType === "paper" && root.model.paperType === "sheet") : true
                    text:           "Paper Size:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                } // Text paperSizeLabel
                ComboBox {
                    id:               paperSizeCombo
                    visible:          root.model ? (root.model.mediaType === "paper" && root.model.paperType === "sheet") : true
                    Layout.fillWidth: true
                    model:            root.model ? root.model.paperSizeNames : []
                    currentIndex:     root.model ? Math.max(0, model.indexOf(root.model.sheetName)) : 0
                    onActivated:      if (root.model) root.model.selectPaperSize(currentText)
                } // ComboBox paperSize

                // Page dimensions (display-only, shown only for paper/sheet):
                //   paper/sheet → pageWidth × pageHeight (user units) + pixel equivalents
                Text {
                    visible:        root.model ? (root.model.mediaType === "paper" && root.model.paperType === "sheet") : true
                    text:           "Page Size:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                } // Text pageSizeLabel
                Text {
                    visible:        root.model ? (root.model.mediaType === "paper" && root.model.paperType === "sheet") : true
                    text: {
                        if (!root.model) return "\u2014"
                        return root.model.pageWidth.toFixed(2) + " \u00D7 "
                             + root.model.pageHeight.toFixed(2) + " " + root.model.unit
                             + "  (" + root.model.pageWidthPx + " \u00D7 " + root.model.pageHeightPx + " px)"
                    } // text
                    color:          Theme.textOnDark
                    font.pixelSize: Theme.fontSizeNormal
                    verticalAlignment: Text.AlignVCenter
                } // Text pageSize

                // Tile size — only for tiled paper
                Text {
                    visible:        root.model ? (root.model.mediaType === "paper" && root.model.paperType === "tiled") : false
                    text:           "Tile Size:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                } // Text tileSizeLabel
                ComboBox {
                    id:               tileSizeCombo
                    visible:          root.model ? (root.model.mediaType === "paper" && root.model.paperType === "tiled") : false
                    Layout.fillWidth: true
                    model:            root.model ? root.model.tileSizeNames : []
                    currentIndex:     root.model ? Math.max(0, model.indexOf(root.model.tileSize)) : 0
                    onActivated:      if (root.model) root.model.selectTileSize(currentText)
                } // ComboBox tileSize

                Text {
                    visible:        root.model ? (root.model.mediaType === "paper" && root.model.paperType === "tiled") : false
                    text:           "Tile Orientation:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                } // Text tileOrientationLabel
                ComboBox {
                    id:               tileOrientationCombo
                    visible:          root.model ? (root.model.mediaType === "paper" && root.model.paperType === "tiled") : false
                    Layout.fillWidth: true
                    model:            root.model ? root.model.tileOrientationNames : []
                    currentIndex:     root.model ? Math.max(0, model.indexOf(root.model.tileOrientation)) : 0
                    onActivated:      if (root.model) root.model.tileOrientation = currentText
                } // ComboBox tileOrientation

                // Roll size — when mediaType == "roll", or mediaType == "paper" && paperType == "roll"
                Text {
                    visible:        root.model ? (root.model.mediaType === "roll" || (root.model.mediaType === "paper" && root.model.paperType === "roll")) : false
                    text:           "Roll Width:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                } // Text rollSizeLabel
                ComboBox {
                    id:               rollSizeCombo
                    visible:          root.model ? (root.model.mediaType === "roll" || (root.model.mediaType === "paper" && root.model.paperType === "roll")) : false
                    Layout.fillWidth: true
                    model:            root.model ? root.model.rollSizeNames : []
                    currentIndex:     root.model ? Math.max(0, model.indexOf(root.model.rollSize)) : 0
                    onActivated:      if (root.model) root.model.rollSize = currentText
                } // ComboBox rollSize

                // Roll length (display-only — 500 in / 1270 cm / 12700 mm sentinel; trimmed after packing)
                Text {
                    visible:        root.model ? (root.model.mediaType === "roll" || (root.model.mediaType === "paper" && root.model.paperType === "roll")) : false
                    text:           "Roll Length:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                } // Text rollLengthLabel
                Text {
                    visible:        root.model ? (root.model.mediaType === "roll" || (root.model.mediaType === "paper" && root.model.paperType === "roll")) : false
                    text:           root.model
                        ? root.model.rollLength.toFixed(0) + " " + root.model.unit
                          + "  (" + root.model.rollLengthPx + " px)"
                        : "\u2014"
                    color:          Theme.textOnDark
                    font.pixelSize: Theme.fontSizeNormal
                    verticalAlignment: Text.AlignVCenter
                } // Text rollLength

            } // GridLayout mediaGrid

            // -----------------------------------------------------------
            // Section: Fabric — shown only for fabric media
            // -----------------------------------------------------------
            Rectangle {
                visible:          root.showFabricSection
                Layout.fillWidth: true
                height: 28
                color:  Qt.rgba(1, 1, 1, 0.08)

                Text {
                    anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                    text:           "Fabric (" + (root.model ? root.model.unit : "in") + ")"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold:      true
                } // Text sectionLabel
            } // Rectangle fabricSection

            GridLayout {
                visible:           root.showFabricSection
                Layout.fillWidth:  true
                Layout.leftMargin:  12
                Layout.rightMargin: 12
                Layout.topMargin:    8
                Layout.bottomMargin: 8
                columns:      2
                columnSpacing: 8
                rowSpacing:    8

                CheckBox {
                    id:                fabricFoldedCheck
                    Layout.columnSpan: 2
                    text:              "Fabric Folded"
                    checked:           root.model ? root.model.fabricFolded : false
                    onToggled:         if (root.model) root.model.fabricFolded = checked

                    contentItem: Text {
                        text:           fabricFoldedCheck.text
                        color:          Theme.textOnDark
                        font.pixelSize: Theme.fontSizeNormal
                        leftPadding:    fabricFoldedCheck.indicator.width + fabricFoldedCheck.spacing
                        verticalAlignment: Text.AlignVCenter
                    } // contentItem Text
                } // CheckBox fabricFolded

                // Fabric width
                Text {
                    text:           "Fabric Width:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                } // Text fabricWidthLabel
                TextField {
                    id:              fabricWidthField
                    Layout.fillWidth: true
                    text:            root.model ? root.model.fabricWidth.toFixed(2) : "0.00"
                    validator:       DoubleValidator { bottom: 0.0; top: 9999.0; decimals: 2 }
                    onEditingFinished: if (root.model) root.model.fabricWidth = parseFloat(text)
                    color:           Theme.fieldText
                    background:      Rectangle { color: Theme.fieldBackground; radius: 2 }
                } // TextField fabricWidth

                // Selvedge width
                Text {
                    text:           "Selvedge Width:"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeNormal
                    Layout.preferredWidth: scrollView.labelWidth
                } // Text selvedgeWidthLabel
                TextField {
                    id:              selvedgeWidthField
                    Layout.fillWidth: true
                    text:            root.model ? root.model.selvedgeWidth.toFixed(2) : "0.00"
                    validator:       DoubleValidator { bottom: 0.0; top: 99.0; decimals: 2 }
                    onEditingFinished: if (root.model) root.model.selvedgeWidth = parseFloat(text)
                    color:           Theme.fieldText
                    background:      Rectangle { color: Theme.fieldBackground; radius: 2 }
                } // TextField selvedgeWidth
            } // GridLayout fabricGrid

            // -----------------------------------------------------------
            // Section: Margins
            // -----------------------------------------------------------
            Rectangle {
                visible:          root.showMarginsSection
                Layout.fillWidth: true
                height: 28
                color:  Qt.rgba(1, 1, 1, 0.08)

                Text {
                    anchors { left: parent.left; leftMargin: 12; verticalCenter: parent.verticalCenter }
                    text:           "Margins (" + (root.model ? root.model.unit : "in") + ")"
                    color:          Theme.fieldLabel
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold:      true
                } // Text sectionLabel
            } // Rectangle marginsSection

            // Margin compass layout: Top centered, Left/Right flanking, Bottom centered
            ColumnLayout {
                visible:           root.showMarginsSection
                Layout.fillWidth:  true
                Layout.leftMargin:  12
                Layout.rightMargin: 12
                Layout.topMargin:    8
                Layout.bottomMargin: 8
                spacing: 4

                // Top margin
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    Text {
                        text:           "Top:"
                        color:          Theme.fieldLabel
                        font.pixelSize: Theme.fontSizeNormal
                        verticalAlignment: Text.AlignVCenter
                        Layout.preferredWidth: 50
                    } // Text topLabel
                    TextField {
                        id:            marginTopField
                        implicitWidth: 90
                        text:          root.model ? root.model.marginTop.toFixed(3) : "0.250"
                        validator:     DoubleValidator { bottom: 0.0; top: 99.99; decimals: 3 }
                        onEditingFinished: if (root.model) root.model.marginTop = parseFloat(text)
                        color:         Theme.fieldText
                        background:    Rectangle { color: Theme.fieldBackground; radius: 2 }
                    } // TextField marginTop
                    Item { Layout.fillWidth: true }
                } // RowLayout topMargin

                // Left / Right margins
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text {
                        text:           "Left:"
                        color:          Theme.fieldLabel
                        font.pixelSize: Theme.fontSizeNormal
                        verticalAlignment: Text.AlignVCenter
                        Layout.preferredWidth: 36
                    } // Text leftLabel
                    TextField {
                        id:            marginLeftField
                        implicitWidth: 90
                        text:          root.model ? root.model.marginLeft.toFixed(3) : "0.250"
                        validator:     DoubleValidator { bottom: 0.0; top: 99.99; decimals: 3 }
                        onEditingFinished: if (root.model) root.model.marginLeft = parseFloat(text)
                        color:         Theme.fieldText
                        background:    Rectangle { color: Theme.fieldBackground; radius: 2 }
                    } // TextField marginLeft
                    Item { Layout.fillWidth: true }
                    Text {
                        text:           "Right:"
                        color:          Theme.fieldLabel
                        font.pixelSize: Theme.fontSizeNormal
                        verticalAlignment: Text.AlignVCenter
                        Layout.preferredWidth: 38
                    } // Text rightLabel
                    TextField {
                        id:            marginRightField
                        implicitWidth: 90
                        text:          root.model ? root.model.marginRight.toFixed(3) : "0.250"
                        validator:     DoubleValidator { bottom: 0.0; top: 99.99; decimals: 3 }
                        onEditingFinished: if (root.model) root.model.marginRight = parseFloat(text)
                        color:         Theme.fieldText
                        background:    Rectangle { color: Theme.fieldBackground; radius: 2 }
                    } // TextField marginRight
                } // RowLayout leftRightMargins

                // Bottom margin
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    Text {
                        text:           "Bottom:"
                        color:          Theme.fieldLabel
                        font.pixelSize: Theme.fontSizeNormal
                        verticalAlignment: Text.AlignVCenter
                        Layout.preferredWidth: 56
                    } // Text bottomLabel
                    TextField {
                        id:            marginBottomField
                        implicitWidth: 90
                        text:          root.model ? root.model.marginBottom.toFixed(3) : "0.250"
                        validator:     DoubleValidator { bottom: 0.0; top: 99.99; decimals: 3 }
                        onEditingFinished: if (root.model) root.model.marginBottom = parseFloat(text)
                        color:         Theme.fieldText
                        background:    Rectangle { color: Theme.fieldBackground; radius: 2 }
                    } // TextField marginBottom
                    Item { Layout.fillWidth: true }
                } // RowLayout bottomMargin
            } // ColumnLayout marginsLayout
        } // ColumnLayout formColumn
    } // ScrollView contentItem

    // -----------------------------------------------------------------------
    // Button footer — Submit, Discard, Save to File, Load from File
    // -----------------------------------------------------------------------
    footer: RowLayout {
        spacing: 8

        SeamlyButton {
            text: "Save to File"
            onClicked: {
                saveSettingsDialog.currentFolder = root.settingsFolderUrl !== ""
                    ? root.settingsFolderUrl
                    : root.model.settingsFolderUrl()
                saveSettingsDialog.open()
            } // onClicked
        } // SeamlyButton saveToFile

        SeamlyButton {
            text: "Load from File"
            onClicked: {
                loadSettingsDialog.currentFolder = root.settingsFolderUrl !== ""
                    ? root.settingsFolderUrl
                    : root.model.settingsFolderUrl()
                loadSettingsDialog.open()
            } // onClicked
        } // SeamlyButton loadFromFile

        Item { Layout.fillWidth: true }

        SeamlyButton {
            text: "Submit"
            onClicked: {
                root.accept();
            } // onClicked
        } // SeamlyButton submit

        SeamlyButton {
            text: "Discard"
            onClicked: {
                root.reject();
            } // onClicked
        } // SeamlyButton discard

        Item { Layout.preferredWidth: 8 }
    } // RowLayout footer

    // -----------------------------------------------------------------------
    // Signals
    // -----------------------------------------------------------------------

    onAccepted: {
        if (root.model) root.model.save(root.settingsPath);
    } // onAccepted

    onRejected: {
        if (root.model) root.model.load(root.settingsPath);
    } // onRejected

    // -----------------------------------------------------------------------
    // File dialogs — Save to File / Load from File
    // -----------------------------------------------------------------------

    // @brief Save current settings to a user-chosen .json file.
    // Opens in the resolved settings directory.  Converts the selected file:// URL to
    // a local path via SettingsModel.urlToLocalFile() before calling save().
    FileDialog {
        id:            saveSettingsDialog
        title:         "Save Settings"
        fileMode:      FileDialog.SaveFile
        nameFilters:   ["Settings Files (*.json)", "All Files (*)"]
        defaultSuffix: "json"
        onAccepted: {
            if (root.model)
                root.model.save(root.model.urlToLocalFile(selectedFile.toString()))
        } // onAccepted
    } // FileDialog saveSettingsDialog

    // @brief Load settings from a user-chosen .json file.
    // Opens in the resolved settings directory.  Converts the selected file:// URL to
    // a local path via SettingsModel.urlToLocalFile() before calling load().
    FileDialog {
        id:          loadSettingsDialog
        title:       "Load Settings"
        fileMode:    FileDialog.OpenFile
        nameFilters: ["Settings Files (*.json)", "All Files (*)"]
        onAccepted: {
            if (root.model)
                root.model.load(root.model.urlToLocalFile(selectedFile.toString()))
        } // onAccepted
    } // FileDialog loadSettingsDialog

    // -----------------------------------------------------------------------
    // Settings-loaded refresh
    //
    // QML property bindings on editable controls (RadioButton.checked,
    // ComboBox.currentIndex, TextField.text) are broken by user interaction —
    // Qt internally sets the property from C++, which replaces the QML
    // binding with a literal value.  Emitting the property notify signals
    // from SettingsModel::load() is not enough to re-evaluate those broken
    // bindings.  This Connections block explicitly sets every dialog control
    // back to the model's current value so the UI is always consistent after
    // a load() call (Load from File, Discard, or startup).
    // -----------------------------------------------------------------------
    Connections {
        target: root.model

        // @brief Refresh all dialog controls from model after load().
        function onSettingsLoaded() {
            if (!root.model) return;

            // Layout-mode radio buttons
            layoutModeAlongGrain.checked = (root.model.layoutMode === "alongGrainline")
            layoutModeWithNap.checked    = (root.model.layoutMode === "withNap")

            // Nap-direction radio buttons
            napUp.checked   = (Math.abs(root.model.rotationStep - 0.0)   < 0.001)
            napDown.checked = (Math.abs(root.model.rotationStep - 180.0) < 0.001)

            // Fabric-folded checkbox
            fabricFoldedCheck.checked = root.model.fabricFolded

            // Media-type radio buttons
            mediaTypePaper.checked  = (root.model.mediaType === "paper")
            mediaTypeFabric.checked = (root.model.mediaType === "fabric")

            // Unit ComboBox
            unitCombo.currentIndex = unitCombo.model.indexOf(root.model.unit)

            // Paper-type ComboBox
            if      (root.model.paperType === "tiled") paperTypeCombo.currentIndex = 1
            else if (root.model.paperType === "roll")  paperTypeCombo.currentIndex = 2
            else                                        paperTypeCombo.currentIndex = 0

            // Paper-size, tile-size, tile-orientation, and roll-size ComboBoxes
            paperSizeCombo.currentIndex      = Math.max(0, paperSizeCombo.model.indexOf(root.model.sheetName))
            tileSizeCombo.currentIndex       = Math.max(0, tileSizeCombo.model.indexOf(root.model.tileSize))
            tileOrientationCombo.currentIndex = Math.max(0, tileOrientationCombo.model.indexOf(root.model.tileOrientation))
            rollSizeCombo.currentIndex       = Math.max(0, rollSizeCombo.model.indexOf(root.model.rollSize))

            // Margin TextFields
            marginTopField.text    = root.model.marginTop.toFixed(3)
            marginBottomField.text = root.model.marginBottom.toFixed(3)
            marginLeftField.text   = root.model.marginLeft.toFixed(3)
            marginRightField.text  = root.model.marginRight.toFixed(3)

            // Fabric TextFields
            fabricWidthField.text   = root.model.fabricWidth.toFixed(2)
            selvedgeWidthField.text  = root.model.selvedgeWidth.toFixed(2)
        } // onSettingsLoaded
    } // Connections settingsLoaded
} // Dialog root
