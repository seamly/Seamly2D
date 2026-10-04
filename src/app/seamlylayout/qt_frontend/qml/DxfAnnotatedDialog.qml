// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file DxfAnnotatedDialog.qml
// @brief Modal dialog asking whether to export a standard or annotated DXF-ASTM file.
//
// An annotated version includes inline DXF group-code comments for every entity,
// useful when learning DXF-ASTM structure.  A standard version omits those
// comments and produces a smaller file suitable for production use.
//
// Usage:
//   DxfAnnotatedDialog {
//       id: dxfAnnotatedDialog
//       onAccepted: appController.exportDxf(
//           savePath,
//           JSON.stringify({ createAnnotatedVersion: dxfAnnotatedDialog.annotatedVersion })
//       )
//   }
//   // To open: dxfAnnotatedDialog.open()

import QtQuick 6.11
import QtQuick.Controls 6.11
import SeamlyLayout

Dialog {
    id: root

    // @brief True when the user chose the annotated version; false for standard.
    //
    // Read this property in the onAccepted handler to determine which variant
    // to pass to AppController.exportDxf().  Reset to false each time the
    // dialog opens via onAboutToShow.
    property bool annotatedVersion: false

    title:  "DXF Export Options"
    modal:  true
    width:  420
    anchors.centerIn: parent

    // Reset selection each time the dialog is shown.
    onAboutToShow: root.annotatedVersion = false

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
            text:           "Generate an annotated version?"
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeNormal
            font.bold:      true
        } // Text heading

        // Description
        Text {
            text: "An <b>annotated version</b> includes inline DXF comments explaining\n" +
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

        // Annotated version — set flag then let AcceptRole fire dialog.accept()
        Button {
            text: "Annotated Version"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: root.annotatedVersion = true
        } // Button annotated

        // Standard version — flag stays false (reset by onAboutToShow)
        Button {
            text: "Standard"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: root.annotatedVersion = false
        } // Button standard

        // Cancel — reject without exporting
        Button {
            text: "Cancel"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        } // Button cancel
    } // DialogButtonBox footer
} // Dialog root
