// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file SeamlyTheme.h
// @brief Header-only Seamly branding palette constants and QPalette factory.
//
// Mirrors the colours defined in Theme.qml exactly so that QtWidgets windows
// (e.g. AdjustWindow) match the QML frontend visually.
//
// Usage:
//   widget->setPalette(SeamlyTheme::makeSeamlyPalette());
//   widget->setStyle(QStyleFactory::create("Fusion"));
//
// Do NOT call QApplication::setPalette() globally — that would affect QML's
// native-control rendering.

#pragma once

#include <QColor>
#include <QPalette>
#include <QStyleFactory>

/// @namespace SeamlyTheme
/// @brief Seamly branding palette constants and factory function.
namespace SeamlyTheme
{

// ---------------------------------------------------------------------------
// Violet palette — mirrors Theme.qml
// ---------------------------------------------------------------------------

/// @brief Deep violet used for app background and button borders (#3e2b60).
inline const QColor SEAMLY_VIOLET_DARK   { "#3e2b60" };

/// @brief Medium violet used for toolbar and default button fill (#7351ad).
inline const QColor SEAMLY_VIOLET_MEDIUM { "#7351ad" };

/// @brief Standard violet used for hover states and dialog backgrounds (#573d83).
inline const QColor SEAMLY_VIOLET        { "#573d83" };

/// @brief Light violet used for selection highlights (#8f65d8).
inline const QColor SEAMLY_VIOLET_LIGHT  { "#8f65d8" };

// ---------------------------------------------------------------------------
// Neutral palette
// ---------------------------------------------------------------------------

/// @brief Light grey used for text on dark backgrounds and the canvas fill (#f3f3f3).
inline const QColor SEAMLY_GRAY_LIGHT    { "#f3f3f3" };

/// @brief Medium grey used for disabled-state elements (#ababaa).
inline const QColor SEAMLY_GRAY_MEDIUM   { "#ababaa" };

/// @brief Near-black used for primary text on light surfaces (#111921).
inline const QColor SEAMLY_BLACK_SOFT    { "#111921" };

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/// @brief Build a QPalette that applies the Seamly branding to a QtWidgets widget.
///
/// Apply this palette to individual windows/widgets only:
/// @code
///   window->setPalette(SeamlyTheme::makeSeamlyPalette());
///   window->setStyle(QStyleFactory::create("Fusion"));
/// @endcode
///
/// @return Configured QPalette instance.
inline QPalette makeSeamlyPalette()
{
    QPalette p;

    // Window chrome and general backgrounds.
    p.setColor(QPalette::Window,        SEAMLY_VIOLET_DARK);
    p.setColor(QPalette::WindowText,    SEAMLY_GRAY_LIGHT);

    // Text-entry and list backgrounds.
    p.setColor(QPalette::Base,          SEAMLY_VIOLET);
    p.setColor(QPalette::AlternateBase, SEAMLY_VIOLET_MEDIUM);
    p.setColor(QPalette::Text,          SEAMLY_GRAY_LIGHT);

    // Push-buttons and tool-buttons.
    p.setColor(QPalette::Button,        SEAMLY_VIOLET_MEDIUM);
    p.setColor(QPalette::ButtonText,    SEAMLY_GRAY_LIGHT);

    // Selection.
    p.setColor(QPalette::Highlight,        SEAMLY_VIOLET_LIGHT);
    p.setColor(QPalette::HighlightedText,  SEAMLY_GRAY_LIGHT);

    // Tooltips.
    p.setColor(QPalette::ToolTipBase, SEAMLY_VIOLET_DARK);
    p.setColor(QPalette::ToolTipText, SEAMLY_GRAY_LIGHT);

    // Disabled state.
    p.setColor(QPalette::Disabled, QPalette::Button,     SEAMLY_GRAY_MEDIUM);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, SEAMLY_GRAY_LIGHT);
    p.setColor(QPalette::Disabled, QPalette::Text,       SEAMLY_GRAY_MEDIUM);
    p.setColor(QPalette::Disabled, QPalette::WindowText, SEAMLY_GRAY_MEDIUM);

    return p;
} // makeSeamlyPalette

} // namespace SeamlyTheme
