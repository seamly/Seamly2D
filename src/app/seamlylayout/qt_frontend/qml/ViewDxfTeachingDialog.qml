// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file ViewDxfTeachingDialog.qml
// @brief Non-modal dialog offering to open a DXF-ASTM teaching file found
//        alongside a selected DXF file.
//
// A teaching file is a companion .txt generated during DXF export when
// createTeachingVersion is true.  It contains the same DXF content with
// inline comments explaining each group code and value.
//
// Shown automatically in View → DXF-ASTM after the DXF opens in its viewer,
// when a companion .txt teaching file is detected in the same directory.
// Because the DXF is already opening, this dialog is non-modal so it does
// not interrupt that flow — it is a secondary affordance, not a gate.
//
// Usage:
//   ViewDxfTeachingDialog {
//       id: viewDxfTeachingDialog
//       onAccepted: Qt.openUrlExternally(
//           preferencesModel.localFileToUrl(viewDxfTeachingDialog.teachingFilePath))
//   }
//   // To show: viewDxfTeachingDialog.teachingFilePath = path
//   //          viewDxfTeachingDialog.open()

import QtQuick 6.10
import QtQuick.Controls 6.10
import SeamlyLayout

Dialog {
    id: root

    // @brief Absolute path to the companion .txt teaching file to open on accept.
    property string teachingFilePath: ""

    // @brief Filename portion of teachingFilePath for display (no directory prefix).
    // Derived from teachingFilePath using lastIndexOf to locate the last directory
    // separator — avoids regex per project policy.
    readonly property string teachingFileName: {
        if (root.teachingFilePath.length === 0) return ""
        var lastFwd  = root.teachingFilePath.lastIndexOf("/")
        var lastBack = root.teachingFilePath.lastIndexOf("\\")
        var lastSep  = lastFwd > lastBack ? lastFwd : lastBack
        return lastSep >= 0 ? root.teachingFilePath.substring(lastSep + 1)
                            : root.teachingFilePath
    } // teachingFileName

    title:  "Teaching File Found"
    modal:  false
    width:  440
    anchors.centerIn: parent

    // Clear path on close so stale data never persists if the caller forgets to set it.
    onClosed: root.teachingFilePath = ""

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
            text:           "A teaching file was found alongside this DXF:"
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeNormal
            font.bold:      true
        } // Text heading

        // Teaching file name
        Text {
            text:           root.teachingFileName
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeSmall
            font.italic:    true
            wrapMode:       Text.WrapAnywhere
            width:          parent.width - 32
        } // Text fileName

        // Description
        Text {
            text: "A teaching file includes inline comments explaining each DXF " +
                  "group code and value — useful for learning DXF-ASTM structure.\n\n" +
                  "It will open in your system default text editor."
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeSmall
            wrapMode:       Text.WordWrap
            width:          parent.width - 32
        } // Text description
    } // Column contentItem

    footer: DialogButtonBox {

        // Accept — caller's onAccepted opens the teaching file via Qt.openUrlExternally.
        Button {
            text: "View Teaching File"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        } // Button view

        // Reject — dismiss without opening; the DXF is already open.
        Button {
            text: "No Thanks"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        } // Button noThanks
    } // DialogButtonBox footer

} // Dialog root
