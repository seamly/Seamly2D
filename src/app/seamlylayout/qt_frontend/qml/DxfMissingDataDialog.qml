// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file DxfMissingDataDialog.qml
// @brief Modal warning before a DXF-ASTM export: pieces that lack Quantity, label or grainline.
//
// The text comes from AppController.dxfMissingPieceData(), which names each piece
// and what it lacks. Accept continues the export; reject cancels it.
//
// Usage:
//   DxfMissingDataDialog {
//       id: dxfMissingDataDialog
//       onAccepted: root.chooseDxfPath()
//   }
//   // To open: dxfMissingDataDialog.warningText = text; dxfMissingDataDialog.open()

import QtQuick 6.11
import QtQuick.Controls 6.11
import SeamlyLayout

Dialog {
    id: root

    // @brief Warning text from AppController.dxfMissingPieceData().
    property string warningText: ""

    title:  "DXF-ASTM: Missing Piece Data"
    modal:  true
    width:  480
    anchors.centerIn: parent

    background: Rectangle {
        color:        Theme.dialogBackground
        border.color: Theme.violetDark
        radius:       4
    } // background Rectangle

    contentItem: Column {
        spacing: 12
        topPadding:    16
        bottomPadding: 8
        leftPadding:   16
        rightPadding:  16

        // Plain text: piece names come from the SVG and must not render as markup.
        Text {
            text:           root.warningText
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeSmall
            textFormat:     Text.PlainText
            wrapMode:       Text.WordWrap
            width:          440
        } // Text warning
    } // Column contentItem

    footer: DialogButtonBox {
        Button {
            text: "Export anyway"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        } // Button export anyway

        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        } // Button cancel
    } // DialogButtonBox footer
} // Dialog root
