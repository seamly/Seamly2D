// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file DxfTeachingDialog.qml
// @brief Modal dialog asking whether to export a standard or teaching DXF-ASTM file.
//
// A teaching version includes inline DXF group-code comments for every entity,
// useful when learning DXF-ASTM structure.  A standard version omits those
// comments and produces a smaller file suitable for production use.
//
// Usage:
//   DxfTeachingDialog {
//       id: dxfTeachingDialog
//       onAccepted: appController.exportDxf(
//           savePath,
//           JSON.stringify({ createTeachingVersion: dxfTeachingDialog.teachingVersion })
//       )
//   }
//   // To open: dxfTeachingDialog.open()

import QtQuick 6.10
import QtQuick.Controls 6.10
import SeamlyLayout

Dialog {
    id: root

    // @brief True when the user chose the teaching version; false for standard.
    //
    // Read this property in the onAccepted handler to determine which variant
    // to pass to AppController.exportDxf().  Reset to false each time the
    // dialog opens via onAboutToShow.
    property bool teachingVersion: false

    title:  "DXF Export Options"
    modal:  true
    width:  420
    anchors.centerIn: parent

    // Reset selection each time the dialog is shown.
    onAboutToShow: root.teachingVersion = false

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

        // Heading
        Text {
            text:           "Generate a teaching version?"
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeNormal
            font.bold:      true
        } // Text heading

        // Description
        Text {
            text: "A <b>teaching version</b> includes inline DXF comments explaining\n" +
                  "each group code and value — useful when learning DXF-ASTM structure\n" +
                  "but produces a larger file.\n\n" +
                  "A <b>standard version</b> omits those comments and is suitable for\n" +
                  "production use with CAD systems."
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeSmall
            textFormat:     Text.RichText
            wrapMode:       Text.WordWrap
            width:          360
        } // Text description
    } // Column contentItem

    footer: DialogButtonBox {

        // Teaching version — set flag then let AcceptRole fire dialog.accept()
        Button {
            text: "Teaching Version"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: root.teachingVersion = true
        } // Button teaching

        // Standard version — flag stays false (reset by onAboutToShow)
        Button {
            text: "Standard"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: root.teachingVersion = false
        } // Button standard

        // Cancel — reject without exporting
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        } // Button cancel
    } // DialogButtonBox footer
} // Dialog root
