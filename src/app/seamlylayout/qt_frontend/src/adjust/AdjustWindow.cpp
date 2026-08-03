// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file AdjustWindow.cpp
// @brief Implementation of AdjustWindow — QtWidgets main window for interactive
//        pattern-piece adjustment.

#include "AdjustWindow.h"
#include "AdjustScene.h"
#include "../SeamlyTheme.h"

#include <QDebug>
#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QGraphicsView>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

/// @brief Construct the adjust window, build its UI, and load the layout.
/**
 * @brief Constructs an AdjustWindow object.
 *
 * Initializes the AdjustWindow with the provided SVG file path and bounding box JSON,
 * and sets up the user interface. The window is created as a child of the given parent widget.
 *
 * @param svgPath   The file path to the SVG file to be displayed or adjusted.
 * @param bboxJson  A JSON string containing bounding box information for the SVG.
 * @param parent    The parent widget of this window (optional).
 */
AdjustWindow::AdjustWindow(const QString& svgPath,
                           const QString& bboxJson,
                           QWidget*       parent)
    : QMainWindow(parent)
{
    buildUi(svgPath, bboxJson);
} // constructor AdjustWindow object

// ---------------------------------------------------------------------------
// Public slots
// ---------------------------------------------------------------------------

/// @brief Reload the scene with a new SVG and bbox JSON (reuses this window).
void AdjustWindow::reload(const QString& svgPath, const QString& bboxJson)
{
    qDebug() << "[AdjustWindow] reload() — reloading scene from" << svgPath;

    // Capture current viewport state before reloading so piece operations do not
    // force a zoom reset.  This preserves the user's current zoom/pan context.
    const QTransform previousViewTransform = m_view->transform();
    const QPointF    previousViewCenter    = m_view->mapToScene(m_view->viewport()->rect().center());

    // Reset flag so the next close (Save/Cancel/×) is handled correctly.
    m_closeHandled = false;
    // Load the new layout into the scene; this clears all existing items and creates new ones.
    m_scene->loadLayout(svgPath, bboxJson);
    // Dump overlay state after reload for debugging.
    m_scene->dumpOverlayData();

    // Restore the previous viewport state after reload so zoom level is maintained
    // while users iteratively apply operations to pieces.
    m_view->setTransform(previousViewTransform);
    m_view->centerOn(previousViewCenter);

    updateUndoRedoActions(m_scene->canUndo(), m_scene->canRedo());
} // reload

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

/// @brief Collect piece transforms, clear overlays, and emit accepted().
///
/// Piece overlays are cleared immediately so the canvas shows only the
/// background SVG while the async reload (triggered by adjustApplied in QML)
/// is in flight.  The reload re-adds PieceOverlayItems at their new positions.
void AdjustWindow::onApplyClicked()
{
    qDebug() << "[AdjustWindow] onApplyClicked: 1 Apply triggered (button or Enter)";



    // Get the transform JSON string for the overlay piece that has moved or rotated, e.g. [{"id":"piece1","transform":"translate(10.0000 5.0000) rotate(15.0000 50.0000 30.0000)"},...]
     qDebug() << "[AdjustWindow] onApplyClicked: 2 calling getMovedTransform()";
    const QString transform_str = m_scene->getMovedTransform();

    // Dump overlay state before applying transforms for debugging.
    m_scene->dumpOverlayData();

    // Clear piece overlays immediately so the user sees the background SVG while the async reload is in flight.
     qDebug() << "[AdjustWindow] onApplyClicked: 3 clearing overlay data";
    m_scene->clearPieces();

    // Emit the accepted() signal with the transforms JSON, which QML forwards as applyRequested() to the AdjustController, which forwards it to the main application's appController, which applies the transforms to the layout_dom and triggers the right canvas reload.
    qDebug() << "[AdjustWindow] onApplyClicked: 4 emitting accepted() with transforms:" << transform_str;
    // signal the transforms to QML, which forwards to the main application controller
    emit accepted(transform_str);
} // onApplyClicked

/// @brief Emit saveRequested() without closing the window.
///
/// Signals QML to call exitAdjustMode(), which saves layout_dom and reloads
/// the right canvas with the final adjusted layout.  The window stays open
/// until QML calls closeWindow() after the right canvas finishes loading,
/// preventing a flash of stale content behind this window.
void AdjustWindow::onSaveClicked()
{
    m_closeHandled = true;
    emit saveRequested();
    // Window stays open — QML calls closeWindow() after the right canvas loads in the main application window.
} // onSaveClicked

void AdjustWindow::onUndoTriggered()
{
    if (m_scene->undoLastOperation()) {
        // AdjustScene already refreshed the actor-only conflict highlight.
        updateUndoRedoActions(m_scene->canUndo(), m_scene->canRedo());
    }
}

void AdjustWindow::onRedoTriggered()
{
    if (m_scene->redoLastOperation()) {
        // AdjustScene already refreshed the actor-only conflict highlight.
        updateUndoRedoActions(m_scene->canUndo(), m_scene->canRedo());
    }
}

void AdjustWindow::onOperationConflictsDetected(const QStringList& /*conflictIds*/)
{
    // Conflict feedback is purely visual: AdjustScene already calls
    // highlightConflicts() before emitting this signal, so the overlapping
    // pieces carry a red stroke. No modal prompt or auto-undo: the user
    // chooses what to do next.
    updateUndoRedoActions(m_scene->canUndo(), m_scene->canRedo());
}

void AdjustWindow::updateUndoRedoActions(bool canUndo, bool canRedo)
{
    if (m_undoAct) {
        m_undoAct->setEnabled(canUndo);
    }

    if (m_redoAct) {
        m_redoAct->setEnabled(canRedo);
    }
}

/// @brief Emit cancelled() and close the window.
void AdjustWindow::onCancelClicked()
{
    m_closeHandled = true;
    // Emit the cancelled() signal, which QML forwards as cancelRequested() to the AdjustController, which forwards it to the main application'sappController, which discards any pending transforms and triggers a reload of the original layout in right canvas of the main application window.
    emit cancelled();
    close();
} // onCancelClicked

/// @brief Treat OS × close as Save so exitAdjustMode() is always called.
///
/// Without this, closing the window via the title-bar × button leaves
/// isAdjustMode = true and all toolbar buttons permanently disabled.
/// The close is deferred — QML will call closeWindow() after the right
/// canvas finishes loading the updated SVG.
void AdjustWindow::closeEvent(QCloseEvent* event)
{
    if (!m_closeHandled) {
        // OS × button — treat as Save: defer close until canvas reloads.
        m_closeHandled = true;
        emit abandoned();
        event->accept();
        return;
    } // if close not already handled by a button

    QMainWindow::closeEvent(event);
} // closeEvent

/// @brief Close the window on demand (called from AdjustController after canvas reload).
void AdjustWindow::closeWindow()
{
    close();
} // closeWindow

/// @brief Fit the entire scene into the viewport at 95% scale.
void AdjustWindow::fitToView()
{
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);

    // Scale back slightly so pieces near the edge are not clipped.
    m_view->scale(0.95, 0.95);
} // fitToView

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

/// @brief Build all widgets, toolbar, and instructions panel; load the initial layout.
void AdjustWindow::buildUi(const QString& svgPath, const QString& bboxJson)
{
    // Window basics.
    setWindowTitle(QStringLiteral("Adjust Mode \xe2\x80\x94 move and rotate pieces"));
    resize(1200, 800);
    setAttribute(Qt::WA_DeleteOnClose, true);

    // Apply Seamly branding to this window only — do NOT set globally via
    // QApplication::setPalette() as that would affect QML native controls.
    setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    setPalette(SeamlyTheme::makeSeamlyPalette());

    // --- Scene and view --------------------------------------------------

    m_scene = new AdjustScene(this);
    connect(m_scene, &AdjustScene::undoRedoAvailabilityChanged,
            this, &AdjustWindow::updateUndoRedoActions);
        connect(m_scene, &AdjustScene::operationConflictsDetected,
            this, &AdjustWindow::onOperationConflictsDetected);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setRenderHint(QPainter::SmoothPixmapTransform);

    // ScrollHandDrag allows panning when no piece is being dragged.
    // PieceOverlayItems consume the press event, so pan only activates on empty space.
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);

    // Light canvas background so the SVG artwork stands out.
    m_view->setBackgroundBrush(SeamlyTheme::SEAMLY_GRAY_LIGHT);

    // Enable mouse-wheel zoom by installing an event filter on the viewport.
    m_view->viewport()->installEventFilter(this);
    // Note: we do not use QGraphicsView::WheelScroll because it scrolls the view   instead of zooming, and we want to preserve the panning behavior on wheel + drag.
    // See eventFilter() implementation below.
    QLabel* canvasHint = new QLabel(
        QStringLiteral("Left-drag to Move * Right-click to Rotate * Select one piece for piece-only Ctrl-Z/Shift-Ctrl-Z Undo/Redo, or deselect pieces for global Undo/Redo. \n Save/Cancel to save all changes or cancel all changes and exit Adjust mode"),
        this);
    canvasHint->setWordWrap(true);
    canvasHint->setAlignment(Qt::AlignCenter);
    canvasHint->setContentsMargins(12, 8, 12, 8);
    canvasHint->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; background-color: %2; border-top: 1px solid %3; font-weight: 600; }")
        .arg(SeamlyTheme::SEAMLY_GRAY_LIGHT.name(),
             SeamlyTheme::SEAMLY_VIOLET_DARK.name(),
             SeamlyTheme::SEAMLY_GRAY_LIGHT.name()));

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(m_view, 1);
    centralLayout->addWidget(canvasHint, 0);
    setCentralWidget(centralWidget);

    // --- Toolbar ---------------------------------------------------------

    QToolBar* toolbar = addToolBar(QStringLiteral("Adjust"));
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(20, 20));

    // Left spacer — pushes actions to center.
    QWidget* leftSpacer = new QWidget(this);
    leftSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(leftSpacer);

    // Fit to view.
    QAction* fitAct = toolbar->addAction(QStringLiteral("\xe2\x9b\xb6  Fit"));
    fitAct->setToolTip(QStringLiteral("Fit layout in view (Ctrl+0)"));
    fitAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(fitAct, &QAction::triggered, this, &AdjustWindow::fitToView);

    // Zoom in.
    QAction* zoomInAct = toolbar->addAction(QStringLiteral("+  Zoom In"));
    zoomInAct->setToolTip(QStringLiteral("Zoom in (Ctrl++)"));
    zoomInAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));
    connect(zoomInAct, &QAction::triggered, this, &AdjustWindow::zoomIn);

    // Zoom out.
    QAction* zoomOutAct = toolbar->addAction(QStringLiteral("\xe2\x88\x92  Zoom Out"));
    zoomOutAct->setToolTip(QStringLiteral("Zoom out (Ctrl+-)"));
    zoomOutAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
    connect(zoomOutAct, &QAction::triggered, this, &AdjustWindow::zoomOut);

    toolbar->addSeparator();

    m_undoAct = toolbar->addAction(QStringLiteral("Undo"));
    m_undoAct->setToolTip(QStringLiteral("Undo last operation (Ctrl+Z)"));
    m_undoAct->setShortcut(QKeySequence::Undo);
    m_undoAct->setEnabled(false);
    connect(m_undoAct, &QAction::triggered, this, &AdjustWindow::onUndoTriggered);

    m_redoAct = toolbar->addAction(QStringLiteral("Redo"));
    m_redoAct->setToolTip(QStringLiteral("Redo next operation (Ctrl+Shift+Z or Ctrl+Y)"));
    m_redoAct->setShortcuts(QList<QKeySequence>{
        QKeySequence::Redo,
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z),
    });
    m_redoAct->setEnabled(false);
    connect(m_redoAct, &QAction::triggered, this, &AdjustWindow::onRedoTriggered);

    toolbar->addSeparator();

    // Apply button — bake current piece positions into layout_dom; stay in AdjustMode.
    QAction* applyAct = toolbar->addAction(QStringLiteral("\xe2\x9c\x93  Apply"));
    applyAct->setToolTip(QStringLiteral("Apply piece positions to layout (Enter)"));
    applyAct->setShortcut(QKeySequence(Qt::Key_Return));
    // Note: we use Enter (Return) instead of Ctrl+Enter to avoid conflicts with common shortcuts like Ctrl+S and Ctrl+W, which users might instinctively try after making adjustments. This encourages them to explicitly choose Save or Cancel, preventing accidental loss of work if they hit Enter out of habit.
    connect(applyAct, &QAction::triggered, this, &AdjustWindow::onApplyClicked);

    // Cancel button — discard all changes and close.
    QAction* cancelAct = toolbar->addAction(QStringLiteral("\xe2\x9c\x95  Cancel"));
    cancelAct->setToolTip(QStringLiteral("Discard all changes and close"));
    // No shortcut for Cancel — users should explicitly choose Save or Cancel after making adjustments; this prevents accidental loss of work if they hit Enter out of habit.
    connect(cancelAct, &QAction::triggered, this, &AdjustWindow::onCancelClicked);

    // Save button — save adjustments, exit AdjustMode, reload right canvas.
    QAction* saveAct = toolbar->addAction(QStringLiteral("\xe2\x97\x8f  Save"));
    saveAct->setToolTip(QStringLiteral("Save adjustments, exit Adjust mode, and update main canvas"));
    // No shortcut for Save — users should explicitly choose Save or Cancel after making adjustments; this prevents accidental loss of work if they hit Enter out of habit.
    connect(saveAct, &QAction::triggered, this, &AdjustWindow::onSaveClicked);

    // Right spacer — balances the left spacer to keep actions centered.
    QWidget* rightSpacer = new QWidget(this);
    rightSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(rightSpacer);

    // --- Instructions panel (left dock) ------------------------------------

    QDockWidget* instructionsDock = new QDockWidget(QStringLiteral("Instructions"), this);
    instructionsDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    instructionsDock->setAllowedAreas(Qt::LeftDockWidgetArea);

    QLabel* instructions = new QLabel(this);
    instructions->setWordWrap(true);
    instructions->setTextFormat(Qt::RichText);
    instructions->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    instructions->setContentsMargins(10, 10, 10, 10);
    instructions->setText(QStringLiteral(
        "<p><b>Move:</b> Left-drag a piece to reposition it. Click "
        "<b>Apply</b> or press <b>Enter</b> to commit the current canvas state.</p>"
        "<p><b>Rotate:</b> Right-click a piece and choose a rotation angle.</p>"
        "<p><b>Undo / Redo:</b> Select one piece, then use <b>Ctrl+Z</b> to undo "
        "that piece or <b>Ctrl+Shift+Z</b> / <b>Ctrl+Y</b> to redo it.</p>"
        "<p><b>Global Undo / Redo:</b> With no piece selected, <b>Ctrl+Z</b> and "
        "<b>Ctrl+Shift+Z</b> / <b>Ctrl+Y</b> work across the whole adjust session.</p>"
        "<p><b>Save:</b> Save all applied changes and exit Adjust Mode.</p>"
        "<p><b>Cancel:</b> Cancel all changes and exit Adjust Mode.</p>"
        "<br>"
        "<p><b>Zoom:</b> Use the mouse wheel or the +/- toolbar buttons.</p>"
        "<p><b>Pan:</b> While zoomed in, drag on empty canvas space.</p>"
        "<p><b>Center:</b> Use the Fit toolbar button.</p>"
    ));

    // Style the panel to match the Seamly branding.
    instructions->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; background-color: %2; }")
        .arg(SeamlyTheme::SEAMLY_GRAY_LIGHT.name(),
             SeamlyTheme::SEAMLY_VIOLET_DARK.name()));

    instructionsDock->setWidget(instructions);
    instructionsDock->setFixedWidth(220);
    addDockWidget(Qt::LeftDockWidgetArea, instructionsDock);

    // --- Load layout and fit view ----------------------------------------

    m_scene->loadLayout(svgPath, bboxJson);
    // Dump overlay state after initial load for debugging.
    m_scene->dumpOverlayData();
    updateUndoRedoActions(m_scene->canUndo(), m_scene->canRedo());

    // Defer fitToView until after show() so the viewport has its final size.
    QTimer::singleShot(0, this, &AdjustWindow::fitToView);
} // buildUi

// ---------------------------------------------------------------------------
// Zoom helpers
// ---------------------------------------------------------------------------

/// @brief Scale the view in by 20 %.
void AdjustWindow::zoomIn()
{
    m_view->scale(1.2, 1.2);
} // zoomIn

/// @brief Scale the view out by ~17 % (inverse of zoom in).
void AdjustWindow::zoomOut()
{
    m_view->scale(1.0 / 1.2, 1.0 / 1.2);
} // zoomOut

// ---------------------------------------------------------------------------
// Event filter — mouse-wheel zoom
// ---------------------------------------------------------------------------

/// @brief Intercept wheel events on the view viewport to zoom instead of scroll.
bool AdjustWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_view->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent* wheel = static_cast<QWheelEvent*>(event);

        if (wheel->angleDelta().y() > 0) {
            // Scroll up — zoom in.
            zoomIn();
        } else {
            // Scroll down — zoom out.
            zoomOut();
        } // if scroll direction

        return true; // event consumed; do not scroll
    } // if wheel event on viewport

    return QMainWindow::eventFilter(watched, event);
} // eventFilter
