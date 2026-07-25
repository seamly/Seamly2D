// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
// Theme.qml — SeamlyLayout branding colour palette and UI constants.
// Source of truth: docs/branding-docs/BRANDING_GUIDELINES.md
//
// Usage in QML:
//   import SeamlyLayout
//   Rectangle { color: Theme.violetDark }
//
// Phase 13 converts this to a pragma Singleton for global access without
// requiring a local instance.

pragma Singleton
import QtQuick 6.11

QtObject {
    // -----------------------------------------------------------------------
    // Violet palette
    // -----------------------------------------------------------------------

    // @brief Primary violet — hover state for buttons.
    readonly property color violet:       "#573d83"

    // @brief Light violet — accent highlights.
    readonly property color violetLight:  "#8f65d8"

    // @brief Medium violet — default button background.
    readonly property color violetMedium: "#7351ad"

    // @brief Dark violet — app background, button borders.
    readonly property color violetDark:   "#3e2b60"

    // -----------------------------------------------------------------------
    // Neutral palette
    // -----------------------------------------------------------------------

    // @brief Near-black — app window background alternative.
    readonly property color blackSoft:    "#111921"

    // @brief Mid gray — general UI gray.
    readonly property color gray:         "#d9d8d6"

    // @brief Light gray — canvas background, enabled button text.
    readonly property color grayLight:    "#f3f3f3"

    // @brief Medium gray — secondary text, disabled state.
    readonly property color grayMedium:   "#ababaa"

    // @brief Dark gray — borders, subtle dividers.
    readonly property color grayDark:     "#888888"

    // -----------------------------------------------------------------------
    // Semantic aliases
    // -----------------------------------------------------------------------

    // @brief Main application window background.
    readonly property color appBackground:    violetDark

    // @brief Primary text on dark backgrounds.
    readonly property color textOnDark:       grayLight

    // @brief Canvas (SVG display area) background.
    readonly property color canvasBackground: grayLight

    // @brief Text on canvas.
    readonly property color textOnCanvas:     violetDark

    // @brief Default enabled button background.
    readonly property color buttonBackground: violetMedium

    // @brief Hovered button background.
    readonly property color buttonHover:      violet

    // @brief Button border colour.
    readonly property color buttonBorder:     violetDark

    // @brief Button text colour.
    readonly property color buttonText:       grayLight

    // @brief Dialog / panel background.
    readonly property color dialogBackground: violet

    // @brief Dialog title bar background.
    readonly property color dialogTitleBar:   violetMedium

    // @brief Dialog title text colour.
    readonly property color dialogTitleText:  grayLight

    // @brief Main title bar background.
    readonly property color titleBarBackground: violetMedium

    // @brief Title bar text colour.
    readonly property color titleBarText:     grayLight

    // -----------------------------------------------------------------------
    // Form field colours
    // -----------------------------------------------------------------------

    // @brief Input field background.
    readonly property color fieldBackground:  grayLight

    // @brief Input field text.
    readonly property color fieldText:        blackSoft

    // @brief Field label text.
    readonly property color fieldLabel:       grayLight

    // -----------------------------------------------------------------------
    // Disabled state colours
    // -----------------------------------------------------------------------

    // @brief Disabled button background.
    readonly property color buttonDisabled:       grayMedium

    // @brief Disabled button text.
    readonly property color buttonDisabledText:   grayLight

    // @brief Hover button border (changes from violetDark to grayLight).
    readonly property color buttonHoverBorder:    grayLight

    // -----------------------------------------------------------------------
    // Typography
    // -----------------------------------------------------------------------

    // @brief Default UI font size in pixels.
    readonly property int fontSizeNormal: 14

    // @brief Small label font size in pixels.
    readonly property int fontSizeSmall:  12

    // @brief Button border radius.
    readonly property int buttonRadius:   4
}
