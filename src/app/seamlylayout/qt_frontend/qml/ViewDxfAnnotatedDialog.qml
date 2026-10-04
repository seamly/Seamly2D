// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file ViewDxfAnnotatedDialog.qml
// @brief Non-modal dialog offering to open a DXF-ASTM annotated file found
//        alongside a selected DXF file.
//
// An annotated file is a companion .txt generated during DXF export when
// createAnnotatedVersion is true.  It contains the same DXF content with
// inline comments explaining each group code and value.
//
// Shown automatically in View → DXF-ASTM after the DXF opens in its viewer,
// when a companion .txt annotated file is detected in the same directory.
// Because the DXF is already opening, this dialog is non-modal so it does
// not interrupt that flow — it is a secondary affordance, not a gate.
//
// Usage:
//   ViewDxfAnnotatedDialog {
//       id: viewDxfAnnotatedDialog
//       onAccepted: Qt.openUrlExternally(
//           preferencesModel.localFileToUrl(viewDxfAnnotatedDialog.annotatedFilePath))
//   }
//   // To show: viewDxfAnnotatedDialog.annotatedFilePath = path
//   //          viewDxfAnnotatedDialog.open()

import QtQuick 6.11
import QtQuick.Controls 6.11
import SeamlyLayout

Dialog {
    id: root

    // @brief Absolute path to the companion .txt annotated file to open on accept.
    property string annotatedFilePath: ""

    // @brief Filename portion of annotatedFilePath for display (no directory prefix).
    // Derived from annotatedFilePath using lastIndexOf to locate the last directory
    // separator — avoids regex per project policy.
    readonly property string annotatedFileName: {
        if (root.annotatedFilePath.length === 0) return ""
        var lastFwd  = root.annotatedFilePath.lastIndexOf("/")
        var lastBack = root.annotatedFilePath.lastIndexOf("\\")
        var lastSep  = lastFwd > lastBack ? lastFwd : lastBack
        return lastSep >= 0 ? root.annotatedFilePath.substring(lastSep + 1)
                            : root.annotatedFilePath
    } // annotatedFileName

    title:  "Annotated File Found"
    modal:  false
    width:  440
    anchors.centerIn: parent

    // Clear path on close so stale data never persists if the caller forgets to set it.
    onClosed: root.annotatedFilePath = ""

    background: Rectangle {
        color:        Theme.dialogBackground
        border.color: Theme.violetDark
        radius:       4
    } // background Rectangle

    contentItem: Column {
        spacing:       12
        topPadding:    16
        bottomPadding: 8
        leftPadding:   16
        rightPadding:  16

        // Heading
        Text {
            text:           "An annotated file was found alongside this DXF:"
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeNormal
            font.bold:      true
        } // Text heading

        // Annotated file name
        Text {
            text:           root.annotatedFileName
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeSmall
            font.italic:    true
            wrapMode:       Text.WrapAnywhere
            width:          parent.width - 32
        } // Text fileName

        // Description
        Text {
            text: "An annotated file includes inline comments explaining each DXF " +
                  "group code and value — useful for learning DXF-ASTM structure.\n\n" +
                  "It will open in your system default text editor."
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeSmall
            wrapMode:       Text.WordWrap
            width:          parent.width - 32
        } // Text description
    } // Column contentItem

    footer: DialogButtonBox {

        // Accept — caller's onAccepted opens the annotated file via Qt.openUrlExternally.
        Button {
            text: "View Annotated File"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        } // Button view

        // Reject — dismiss without opening; the DXF is already open.
        Button {
            text: "No Thanks"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        } // Button noThanks
    } // DialogButtonBox footer

} // Dialog root
