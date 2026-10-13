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
    topPadding:    16
    bottomPadding: 8
    leftPadding:   16
    rightPadding:  16

    background: Rectangle {
        color:        Theme.dialogBackground
        border.color: Theme.violetDark
        radius:       4
    } // background Rectangle

    // A long piece list scrolls. The text area never grows past 60% of the window,
    // so the title and the buttons stay visible.
    contentItem: ScrollView {
        id: scroll
        implicitWidth:  440
        implicitHeight: Math.min(warning.implicitHeight,
                                 root.parent ? root.parent.height * 0.6 : 400)
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        // Plain text: piece names come from the SVG and must not render as markup.
        Text {
            id:             warning
            text:           root.warningText
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeSmall
            textFormat:     Text.PlainText
            wrapMode:       Text.WordWrap
            width:          scroll.availableWidth
        } // Text warning
    } // ScrollView contentItem

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
