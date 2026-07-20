// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file AdjustWindow.h
// @brief QtWidgets main window for interactive piece adjustment.
//
// Contains a QGraphicsView showing the layout SVG with draggable PieceOverlayItems.
// Apply button emits accepted(transformsJson); Cancel button emits cancelled().
// Zoom via mouse wheel; fit-to-view via toolbar button or double-click.

#pragma once

#include <QEvent>
#include <QMainWindow>
#include <QString>
#include <QStringList>

// Forward declarations.
class AdjustScene;
class QAction;
class QGraphicsView;

/// @class AdjustWindow
/// @brief Top-level QtWidgets window for interactive layout piece adjustment.
///
/// Opens the layout SVG with interactive PieceOverlayItems overlaid.  The user can:
///  - Drag pieces to new positions.
///  - Right-click to rotate or reset a piece.
///  - Zoom with the mouse wheel.
///  - Fit everything in view via the toolbar or Ctrl+0.
///  - Click Apply to confirm changes (emits accepted()) or Cancel to discard.
///
/// @par Typical usage from QML / C++:
/// @code
///   auto* win = new AdjustWindow(svgPath, bboxJson);
///   connect(win, &AdjustWindow::accepted, this, [](const QString& json){ ... });
///   connect(win, &AdjustWindow::cancelled, win, &AdjustWindow::close);
///   win->show();
/// @endcode
class AdjustWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// @brief Construct and wire up the adjust window.
    /// @param svgPath  Absolute path to the layout SVG.
    /// @param bboxJson JSON string with piece bounding boxes (see project context).
    /// @param parent   Optional parent widget (nullptr for top-level).
    explicit AdjustWindow(const QString& svgPath,
                          const QString& bboxJson,
                          QWidget*       parent = nullptr);

    /// @brief Reload the scene with a new SVG and bbox JSON (reuses this window).
    /// @param svgPath  Absolute path to the updated layout SVG.
    /// @param bboxJson Updated piece bbox JSON string.
    void reload(const QString& svgPath, const QString& bboxJson);

signals:
    /// @brief Emitted when the user clicks Apply.
    /// @param transformsJson Compact JSON array of {id, transform} objects.
    void accepted(const QString& transformsJson);

    /// @brief Emitted when the user clicks Save — exits AdjustMode and saves the layout.
    void saveRequested();

    /// @brief Emitted when the user clicks Cancel.
    void cancelled();

    /// @brief Emitted when the user closes the window via the title-bar X.
    void abandoned();

public slots:
    /// @brief Close the window on demand (called by AdjustController after canvas reload).
    void closeWindow();

private slots:
    /// @brief Collect transforms and emit accepted().
    void onApplyClicked();

    /// @brief Emit saveRequested() without closing the window.
    void onSaveClicked();

    /// @brief Emit cancelled() and close the window.
    void onCancelClicked();

    /// @brief Fit the entire scene into the viewport, then scale to 95%.
    void fitToView();

    /// @brief Scale the view in by 20%.
    void zoomIn();

    /// @brief Scale the view out by ~17% (inverse of zoom in).
    void zoomOut();

    /// @brief Undo the most recent scene-level adjust operation.
    void onUndoTriggered();

    /// @brief Redo the next scene-level adjust operation.
    void onRedoTriggered();

    /// @brief Refresh Undo/Redo action enabled state from the scene.
    void updateUndoRedoActions(bool canUndo, bool canRedo);

    /// @brief Prompt user when an operation introduces conflicts.
    /// @param conflictIds Piece ids currently in conflict.
    void onOperationConflictsDetected(const QStringList& conflictIds);

protected:
    /// @brief Intercept wheel events on the viewport to zoom instead of scroll.
    bool eventFilter(QObject* watched, QEvent* event) override;

    /// @brief Abandon pending changes for OS close (title-bar X) if not already handled.
    void closeEvent(QCloseEvent* event) override;

private:
    /// @brief Build all UI widgets, toolbar, and layout.
    void buildUi(const QString& svgPath, const QString& bboxJson);

    /// @brief The graphics scene holding background and PieceOverlayItems.
    AdjustScene* m_scene = nullptr;

    /// @brief The view that renders the scene.
    QGraphicsView* m_view = nullptr;

    /// @brief Toolbar Undo action, enabled only when the scene can undo.
    QAction* m_undoAct = nullptr;

    /// @brief Toolbar Redo action, enabled only when the scene can redo.
    QAction* m_redoAct = nullptr;

    /// @brief True once a toolbar button has already emitted a close signal.
    ///
    /// Prevents closeEvent from emitting a second close-related signal when the
    /// window is programmatically closed by one of the toolbar actions.
    bool m_closeHandled = false;
};
