// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file SeamlyButton.qml
// @brief Branded push button using the SeamlyLayout violet/gray palette.
//
// Usage:
//   SeamlyButton {
//       text: "Import SVG"
//       onClicked: importDialog.open()
//   }
//
// Optional properties:
//   enabled  — false renders the button in the disabled (gray) state
//   width    — override; defaults to implicitWidth

import QtQuick 6.11
import QtQuick.Controls 6.11
import SeamlyLayout

Button {
    id: root

    // @brief Minimum width so short labels still have comfortable padding.
    implicitWidth:  Math.max(contentItem.implicitWidth + 24, 80)
    implicitHeight: 32

    // -----------------------------------------------------------------------
    // Background — violet palette with hover and disabled states
    // -----------------------------------------------------------------------
    background: Rectangle {
        color: {
            if (!root.enabled)     return Theme.buttonDisabled;   // disabled: gray
            if (root.hovered)      return Theme.buttonHover;       // hover: violet
            return Theme.buttonBackground;                         // normal: violetMedium
        } // color
        border.color: {
            if (!root.enabled)     return Theme.grayMedium;        // disabled border
            if (root.hovered)      return Theme.buttonHoverBorder;  // hover: grayLight
            return Theme.buttonBorder;                              // normal: violetDark
        } // border.color
        radius: Theme.buttonRadius
    } // background Rectangle

    // -----------------------------------------------------------------------
    // Label — centered, branding font size
    // -----------------------------------------------------------------------
    contentItem: Text {
        text:               root.text
        color:              root.enabled ? Theme.buttonText : Theme.buttonDisabledText
        font.pixelSize:     Theme.fontSizeNormal
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment:   Text.AlignVCenter
    } // contentItem Text
} // Button root
