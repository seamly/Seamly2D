// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file AdjustController.h
// @brief QObject bridge between QML and the QtWidgets AdjustWindow.
//
// Registered with the QML engine as "AdjustController" (SeamlyLayout 1.0).
// QML calls launchAdjustWindow() to open (or reload) the adjust window.
// Signals from AdjustWindow are forwarded as QML-visible signals on this object.

#pragma once

#include <QObject>
#include <QString>

// Forward declaration — avoids pulling AdjustWindow headers into QML-visible code.
class AdjustWindow;

/// @class AdjustController
/// @brief QML-accessible controller that owns and manages the AdjustWindow.
///
/// Lifetime: one AdjustWindow instance is created on the first call to
/// launchAdjustWindow() and reused on subsequent calls via AdjustWindow::reload().
/// The pointer is cleared automatically when the user closes the window via the
/// OS close button (destroyed() signal).
///
/// @par QML usage:
/// @code
///   AdjustController {
///       id: adjustController
///       onApplyRequested: function(transformsJson) { ... }
///       onCancelRequested: { ... }
///   }
///   adjustController.launchAdjustWindow(svgPath, bboxJson)
/// @endcode
class AdjustController : public QObject
{
    Q_OBJECT

public:
    /// @brief Construct the controller (no window created yet).
    /// @param parent Optional QObject parent.
    explicit AdjustController(QObject* parent = nullptr);

    /// @brief Open or reload the AdjustWindow with the given layout data.
    ///
    /// On first call, creates the window, connects its signals, and shows it.
    /// On subsequent calls, reloads the scene with the new SVG and bbox JSON
    /// so the window reflects the latest layout state.
    ///
    /// @param svgPath  Absolute native path to the layout SVG file.
    /// @param bboxJson Piece bounding-box JSON string from appController.getPieceBboxes().
    Q_INVOKABLE void launchAdjustWindow(const QString& svgPath,
                                        const QString& bboxJson);

    /// @brief Close the AdjustWindow on demand (called from QML after canvas reload).
    Q_INVOKABLE void closeAdjustWindow();

signals:
    /// @brief Forwarded from AdjustWindow::accepted(); carries the transforms JSON.
    /// @param transformsJson Compact JSON array of {id, transform} objects.
    /// Called by AdjustWindow::onApplyClicked() after the user clicks Apply and the transforms JSON is built.
    void applyRequested(const QString& transformsJson);

    /// @brief Forwarded from AdjustWindow::saveRequested().
    ///
    /// QML should respond by calling appController.exitAdjustMode(), which saves
    /// layout_dom and reloads the right canvas via layoutFinished.
    void saveRequested();

    /// @brief Forwarded from AdjustWindow::cancelled().
    void cancelRequested();

    /// @brief Forwarded from AdjustWindow::abandoned().
    void abandonRequested();

private:
    /// @brief The managed adjust window (nullptr until first launch).
    AdjustWindow* m_window = nullptr;
};
