// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file PieceOverlayItem.h
// @brief QGraphicsRectItem subclass representing one pattern piece overlay in the adjust canvas.
//
// Handles drag-to-move (using scene coordinates for correct delta tracking),
// context-menu rotation, and selection highlight.  No coordinate-space feedback
// loop: drag delta is always computed from scenePos() which is stable regardless
// of the item's own position.

#pragma once

#include <QGraphicsRectItem>
#include <QString>
#include <QPointF>
#include <QGraphicsItem>
#include <QVector>

/// @class PieceOverlayItem
/// @brief Interactive graphics item representing a single pattern piece overlay.
///
/// Renders a labeled rectangle for one piece, supports drag-to-move and
/// right-click context-menu rotation (90 CW, 90 CCW, 180, reset).
/// Movement is tracked entirely in scene coordinates so that Qt's item
/// transform never creates a feedback loop with the drag delta.
class PieceOverlayItem : public QGraphicsRectItem
{
public:
    struct TransformState
    {
        QPointF pos;
        double rotation = 0.0;
        double scaleX = 1.0;   ///< Horizontal flip: -1.0 when flipped, 1.0 otherwise.
        double scaleY = 1.0;   ///< Vertical flip: -1.0 when flipped, 1.0 otherwise.
        QString transform;
    };

    struct HistorySnapshot
    {
        QVector<TransformState> history;
        int historyIndex = -1;
    };

    /// @brief Get the current transform string (SVG format) for this piece.
    const QString& transformStr() const { return m_transformStr; }

    /// @brief Set the current transform string (SVG format) for this piece.
    void setTransformStr(const QString& str) { m_transformStr = str; }

    /// @brief Construct a PieceOverlayItem.
    /// @param id       Piece identifier string (matches SVG group id).
    /// @param originX  SVG origin offset X in pixels (origin_x_px from bbox JSON).
    /// @param originY  SVG origin offset Y in pixels (origin_y_px from bbox JSON).
    /// @param x        Initial position X in scene pixels.
    /// @param y        Initial position Y in scene pixels.
    /// @param w        Piece bounding-box width in pixels.
    /// @param h        Piece bounding-box height in pixels.
    /// @param rotationDeg Initial rotation in degrees.
    /// @param transformStr SVG transform string (optional).
    /// @param parent   Optional parent QGraphicsItem.
    explicit PieceOverlayItem(const QString& id,
                       double originX,
                       double originY,
                       double x,
                       double y,
                       double w,
                       double h,
                       double rotationDeg          = 0.0,
                       const QString& transformStr = QString(),
                       QGraphicsItem* parent       = nullptr);

    /// @brief Parse and apply all SVG transform commands in the string to (x, y, angle).
    void applyTransformString(double baseX, double baseY, double baseAngle, const QString& transformStr);

    /// @brief Return the piece identifier.
    /// @return Immutable reference to the piece id string.
    const QString& pieceId() const;

    /// @brief Return the SVG origin offset X in pixels.
    double originX() const { return m_originX; }

    /// @brief Return the SVG origin offset Y in pixels.
    double originY() const { return m_originY; }

    /// @brief Return the initial scene position captured at construction.
    const QPointF& initialPos() const { return m_initialPos; }

    /// @brief Return the initial rotation captured at construction.
    double initialRotation() const { return m_initialRotation; }

    /// @brief Return true if this piece has been moved, rotated, or flipped from its load position.
    bool hasMoved() const;

    /// @brief Return horizontal scale factor (1.0 normal, -1.0 flipped).
    double scaleX() const { return m_scaleX; }

    /// @brief Return vertical scale factor (1.0 normal, -1.0 flipped).
    double scaleY() const { return m_scaleY; }

    /// @brief Set horizontal scale factor for flip state.
    void setScaleX(double sx) { m_scaleX = sx; }

    /// @brief Set vertical scale factor for flip state.
    void setScaleY(double sy) { m_scaleY = sy; }

    /// @brief Toggle conflict highlight (red 2px outline) for this overlay.
    /// @param conflict True to show conflict outline; false to show normal outline.
    void setConflictHighlighted(bool conflict);

    /// @brief Build the SVG transform string for this piece.
    QString buildTransform() const;

    /// @brief Return the number of recorded full-state history entries.
    int historySize() const { return m_transformHistory.size(); }

    /// @brief Return the current history index, or -1 when no history exists.
    int historyIndex() const { return m_historyIndex; }

    /// @brief Return true if a previous state exists in this piece history.
    bool canUndo() const;

    /// @brief Return true if a later state exists in this piece history.
    bool canRedo() const;

    /// @brief Restore the previous recorded state for this piece.
    bool undo();

    /// @brief Restore the next recorded state for this piece.
    bool redo();

    /// @brief Return a copy of the full local history and current index.
    HistorySnapshot historySnapshot() const;

    /// @brief Replace the local history and restore the current indexed state.
    void restoreHistorySnapshot(const HistorySnapshot& snapshot);

    /// @brief Reset overlay box to original SVG position (no transforms applied).
    void resetToOriginalPosition();

    /// @brief Append a new transform to the SVG transform attribute (handled via xmltree/svg_dom).
    void applyTransformToSvg(const QString& newTransform);

    /// @brief Apply transform and reset overlay box position (prevents double transform).
    void applyTransformAndReset(const QString& newTransform);

protected:
    /// @brief Begin drag tracking on left-button press.
    /// @param event Mouse press event.
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    /// @brief Update item position during drag.
    /// @param event Mouse move event.
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    /// @brief Finalise drag on button release.
    /// @param event Mouse release event.
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

    /// @brief Show rotation / reset context menu on right-click.
    /// @param event Context menu event.
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    /// @brief Piece identifier (matches the SVG group id attribute).
    QString m_id;

    /// @brief Current SVG transform string for this piece (updated on every move/rotate).
    QString m_transformStr;

    /// @brief SVG origin offset X (origin_x_px from bbox JSON).
    double m_originX;

    /// @brief SVG origin offset Y (origin_y_px from bbox JSON).
    double m_originY;

    /// @brief Bounding-box width in pixels.
    double m_w;

    /// @brief Bounding-box height in pixels.
    double m_h;

    /// @brief Scene position captured at construction (for "Reset position" and hasMoved()).
    QPointF m_initialPos;

    /// @brief Rotation captured at construction (for hasMoved()).
    double m_initialRotation;

    /// @brief Immutable geometry-space anchor read from bbox JSON before any SVG transform is replayed.
    QPointF m_basePos;

    /// @brief Immutable geometry-space rotation read from bbox JSON before any SVG transform is replayed.
    double m_baseRotation;

    /// @brief Horizontal scale factor (1.0 normal, -1.0 flipped horizontally).
    double m_scaleX = 1.0;

    /// @brief Vertical scale factor (1.0 normal, -1.0 flipped vertically).
    double m_scaleY = 1.0;

    /// @brief Scene position of the mouse at the start of the current drag.
    QPointF m_dragStartScenePos;

    /// @brief Item position at the start of the current drag.
    QPointF m_dragStartItemPos;

    /// @brief True once the mouse has moved beyond DRAG_THRESHOLD pixels.
    bool m_dragging = false;

    /// @brief Full-state history for this overlay, newest state at m_historyIndex.
    QVector<TransformState> m_transformHistory;

    /// @brief Index of the currently active full-state history entry.
    int m_historyIndex = -1;

    /// @brief Minimum mouse travel (scene pixels) required to start a drag.
    static constexpr double DRAG_THRESHOLD = 3.0;

    /// @brief Record the current full transform state, truncating any redo branch.
    void recordCurrentState();

    /// @brief Apply a recorded state directly to the live overlay item.
    void applyState(const TransformState& state);

    // These must be implemented elsewhere using xmltree/svg_dom.
    QString getSvgTransformAttribute() const;
    void setSvgTransformAttribute(const QString& transform);
};
