// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file PieceOverlayItem.cpp
// @brief Implementation of PieceOverlayItem — interactive pattern-piece overlay graphics item.

#include "PieceOverlayItem.h"
#include "AdjustScene.h"

#include <QAction>
#include <QColor>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QPen>
#include <QBrush>
#include <QCursor>
#include <QRegularExpression>
#include <QTransform>
#include <QGraphicsItem>
#include <cmath>

// Branding colours (violetMedium = #7351ad, violetLight = #b397e8)
namespace {
    /// @brief Primary piece outline colour (violetMedium).
    const QColor kVioletMedium(0x73, 0x51, 0xad);
    /// @brief Conflict outline colour (pure red).
    const QColor kConflictRed(0xff, 0x00, 0x00);
    /// @brief Fill colour — violetMedium with low alpha so background SVG shows through.
    const QColor kVioletFill(0x73, 0x51, 0xad, 25);
} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

/// @brief Construct a PieceOverlayItem and configure its appearance and interaction flags.
PieceOverlayItem::PieceOverlayItem(const QString& id,
                     double originX,
                     double originY,
                     double x,
                     double y,
                     double w,
                     double h,
                     double rotationDeg,
                     const QString& transformStr,
                     QGraphicsItem* parent)
    : QGraphicsRectItem(0.0, 0.0, w, h, parent)
    , m_id(id)
    , m_originX(originX)
    , m_originY(originY)
    , m_w(w)
    , m_h(h)
    , m_initialPos(x, y)
    , m_initialRotation(rotationDeg)
    , m_basePos(x, y)
    , m_baseRotation(rotationDeg)
    , m_dragging(false)
    , m_transformStr(transformStr)
{
    // Position the item in the scene at initial position.
    setPos(x, y);

    // Set rotation pivot at the bounding-box center, not the upper-left
    // corner. Rotating about the center is the user-friendly behavior:
    // the piece swings in place instead of swinging around a corner.
    // buildTransform()/applyTransformString() compute the matching
    // rotate-about-center SVG matrix so overlays and SVG pieces stay
    // aligned after accept_adjustments().
    setTransformOriginPoint(m_w / 2.0, m_h / 2.0);

    // Every piece overlay must be interactive, regardless of whether it already
    // has a persisted SVG transform string. Without this, the view's
    // ScrollHandDrag mode steals left-drag gestures and context-menu rotation
    // updates are never reflected on the overlay item itself.
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setCursor(Qt::SizeAllCursor);

    // Appearance: 2 px violetMedium outline, semi-transparent fill.
    setPen(QPen(kVioletMedium, 2.0));
    setBrush(QBrush(kVioletFill));

    if (!transformStr.isEmpty()) {
        m_transformStr = transformStr;
        this->applyTransformString(x, y, rotationDeg, m_transformStr);
        m_initialPos = pos();
        m_initialRotation = rotation();
    } else {
        // No prior transform — use rotationDeg if provided.
        if (std::abs(rotationDeg) > 0.001) {
            this->setRotation(rotationDeg);
        }
        // Build initial transform string from current state.
        qDebug() << "[PieceOverlayItem] PieceOverlayItem constructor: no prior transform, calling buildTransform() to build transform string";
        m_transformStr = this->buildTransform();
    }

    recordCurrentState();
} // PieceOverlayItem constructor

// ---------------------------------------------------------------------------
// Transform string parsing and application
// ---------------------------------------------------------------------------

/// @brief Parse and apply all SVG transform commands in the string to (x, y, angle).
void PieceOverlayItem::applyTransformString(double baseX, double baseY, double baseAngle, const QString& transformStr)
{
    QTransform cumulative;
    QString rest = transformStr.trimmed();
    while (!rest.isEmpty()) {
        rest.remove(QRegularExpression(R"(^[\s,]+)"));
        if (rest.isEmpty()) {
            break;
        }

        const int paren = rest.indexOf('(');
        if (paren < 0) {
            break;
        }

        const QString funcName = rest.left(paren).trimmed();
        rest = rest.mid(paren + 1);

        const int close = rest.indexOf(')');
        if (close < 0) {
            break;
        }

        const QString paramsStr = rest.left(close);
        rest = rest.mid(close + 1);

        const QStringList parts = paramsStr.split(QRegularExpression(R"([\s,]+)"), Qt::SkipEmptyParts);
        QVector<double> params;
        params.reserve(parts.size());
        for (const QString& part : parts) {
            bool ok = false;
            const double value = part.toDouble(&ok);
            if (ok) {
                params.append(value);
            }
        }

        QTransform next;
        if (funcName == "translate") {
            const double tx = params.value(0, 0.0);
            const double ty = params.value(1, 0.0);
            next.translate(tx, ty);
        } else if (funcName == "rotate") {
            const double a = params.value(0, 0.0);
            const double cx = params.value(1, 0.0);
            const double cy = params.value(2, 0.0);
            next.translate(cx, cy);
            next.rotate(a);
            next.translate(-cx, -cy);
        } else if (funcName == "matrix" && params.size() >= 6) {
            next = QTransform(params[0], params[1], params[2], params[3], params[4], params[5]);
        } else {
            continue;
        }

        cumulative = cumulative * next;
    }

    // Replay the SVG transform chain against the piece's bounding-box center
    // (the rotation pivot set via setTransformOriginPoint()) and the local
    // +X axis through that center, so overlay position/rotation match
    // buildTransform()'s rotate-about-center convention and stay aligned
    // with the rendered SVG.
    const QPointF centerBase(baseX + m_w / 2.0, baseY + m_h / 2.0);
    const QPointF center = cumulative.map(centerBase);
    const QPointF axis   = cumulative.map(QPointF(centerBase.x() + 1.0, centerBase.y()));
    const QPointF dir    = axis - center;
    constexpr double kRadToDeg = 57.29577951308232;
    const double angle = baseAngle + std::atan2(dir.y(), dir.x()) * kRadToDeg;

    // pos() is the scene position of the item's local origin (0,0); recover
    // it from the mapped center by subtracting the half-extent offset.
    const QPointF origin(center.x() - m_w / 2.0, center.y() - m_h / 2.0);

    this->setPos(origin);
    this->setRotation(angle);

} // PieceOverlayItem constructor

// ---------------------------------------------------------------------------
// Public accessors
// ---------------------------------------------------------------------------

/// @brief Return the piece identifier string.
const QString& PieceOverlayItem::pieceId() const
{
    return m_id;
}

/// @brief Return true if this piece has been moved, rotated, or flipped from its load position.
bool PieceOverlayItem::hasMoved() const
{
    constexpr double kPosTol = 0.5;     // half a pixel
    constexpr double kRotTol = 0.001;   // degrees
    constexpr double kScaleTol = 0.001; // scale tolerance
    const QPointF cur = pos();
    return std::abs(cur.x() - m_initialPos.x()) > kPosTol
        || std::abs(cur.y() - m_initialPos.y()) > kPosTol
        || std::abs(rotation() - m_initialRotation) > kRotTol
        || std::abs(m_scaleX - 1.0) > kScaleTol
        || std::abs(m_scaleY - 1.0) > kScaleTol;
} // hasMoved

void PieceOverlayItem::setConflictHighlighted(bool conflict)
{
    // Normal: 2px violetMedium. Conflict: 10px pure red — unmistakable
    // after a move/rotate.
    if (conflict) {
        setPen(QPen(kConflictRed, 10.0));
    } else {
        setPen(QPen(kVioletMedium, 2.0));
    }
}

bool PieceOverlayItem::canUndo() const
{
    return m_historyIndex > 0 && m_historyIndex < m_transformHistory.size();
}

bool PieceOverlayItem::canRedo() const
{
    return m_historyIndex >= 0 && (m_historyIndex + 1) < m_transformHistory.size();
}

void PieceOverlayItem::applyState(const TransformState& state)
{
    setPos(state.pos);
    setRotation(state.rotation);
    m_scaleX = state.scaleX;
    m_scaleY = state.scaleY;
    m_transformStr = state.transform;
}

void PieceOverlayItem::recordCurrentState()
{
    const QString fullTransform = buildTransform();
    const bool duplicateCurrent =
        m_historyIndex >= 0
        && m_historyIndex < m_transformHistory.size()
        && std::abs(m_transformHistory[m_historyIndex].pos.x() - pos().x()) < 0.0001
        && std::abs(m_transformHistory[m_historyIndex].pos.y() - pos().y()) < 0.0001
        && std::abs(m_transformHistory[m_historyIndex].rotation - rotation()) < 0.001
        && std::abs(m_transformHistory[m_historyIndex].scaleX - m_scaleX) < 0.0001
        && std::abs(m_transformHistory[m_historyIndex].scaleY - m_scaleY) < 0.0001
        && m_transformHistory[m_historyIndex].transform == fullTransform;

    if (duplicateCurrent) {
        return;
    }

    if (m_historyIndex + 1 < m_transformHistory.size()) {
        m_transformHistory.resize(m_historyIndex + 1);
    }

    TransformState state;
    state.pos = pos();
    state.rotation = rotation();
    state.scaleX = m_scaleX;
    state.scaleY = m_scaleY;
    state.transform = fullTransform;
    m_transformHistory.append(state);
    m_historyIndex = m_transformHistory.size() - 1;
} // recordCurrentState

bool PieceOverlayItem::undo()
{
    if (!canUndo()) {
        return false;
    }

    --m_historyIndex;
    applyState(m_transformHistory[m_historyIndex]);
    return true;
}

bool PieceOverlayItem::redo()
{
    if (!canRedo()) {
        return false;
    }

    ++m_historyIndex;
    applyState(m_transformHistory[m_historyIndex]);
    return true;
}

PieceOverlayItem::HistorySnapshot PieceOverlayItem::historySnapshot() const
{
    HistorySnapshot snapshot;
    snapshot.history = m_transformHistory;
    snapshot.historyIndex = m_historyIndex;
    return snapshot;
}

void PieceOverlayItem::restoreHistorySnapshot(const HistorySnapshot& snapshot)
{
    if (snapshot.history.isEmpty()) {
        m_transformHistory.clear();
        m_historyIndex = -1;
        return;
    }

    m_transformHistory = snapshot.history;
    m_historyIndex = qBound(0, snapshot.historyIndex, m_transformHistory.size() - 1);
    applyState(m_transformHistory[m_historyIndex]);
}

/// @brief Build the SVG transform string for the item's current position, rotation, and flip state.
QString PieceOverlayItem::buildTransform() const
{
    // Rebuild the piece's full canonical affine transform from its immutable
    // geometry baseline so Apply can overwrite the SVG transform with a single
    // matrix that both Qt replay and SVG rendering interpret identically.
    const double tx = pos().x() - m_basePos.x();
    const double ty = pos().y() - m_basePos.y();
    const double angle = rotation() - m_baseRotation;
    const bool hasTranslation = std::abs(tx) >= 0.0001 || std::abs(ty) >= 0.0001;
    const bool hasRotation = std::abs(angle) >= 0.001;
    const bool hasFlip = std::abs(m_scaleX - 1.0) > 0.0001 || std::abs(m_scaleY - 1.0) > 0.0001;

    // Build the combined transform matrix as a rotate-about-center operation,
    // matching the QGraphicsItem rotation pivot set via setTransformOriginPoint()
    // in the constructor and contextMenuEvent() (bounding-box center, not the
    // upper-left corner):
    // 1. Translate the bbox-center pivot from its base scene position to its
    //    current scene position (pos() + half-extent).
    // 2. Apply rotation and flip/scale, both pivoted at that same center.
    // 3. Translate back from the bbox-center pivot's base scene position.
    const double cx = m_w / 2.0;
    const double cy = m_h / 2.0;

    QTransform fullMatrix;
    fullMatrix.translate(pos().x() + cx, pos().y() + cy);
    fullMatrix.rotate(angle);

    // Flip is pivoted at the same center as the rotation, so no extra
    // translate pair is needed here.
    if (hasFlip) {
        fullMatrix.scale(m_scaleX, m_scaleY);
    }

    fullMatrix.translate(-m_basePos.x() - cx, -m_basePos.y() - cy);

    qDebug() << "[PieceOverlayItem] buildTransform(): " << pieceId();
    qDebug() << "        local origin ox, oy: (" << m_originX << "," << m_originY << ")";
    qDebug() << "                  base x, y: (" << m_basePos.x() << "," << m_basePos.y() << ")";
    qDebug() << "       full transform tx, ty: (" << tx << "," << ty << ")";
    qDebug() << "           old transformStr: " << m_transformStr;
    qDebug() << "                 full angle: " << angle;
    qDebug() << "              scaleX, scaleY: (" << m_scaleX << "," << m_scaleY << ")";
    qDebug() << "           new pos.x, pos.y: (" << pos().x() << "," << pos().y() << ")";

    if (!hasTranslation && !hasRotation && !hasFlip) {
        return QString();
    }

    return QString("matrix(%1 %2 %3 %4 %5 %6)")
           .arg(fullMatrix.m11(), 0, 'f', 6)
           .arg(fullMatrix.m12(), 0, 'f', 6)
           .arg(fullMatrix.m21(), 0, 'f', 6)
           .arg(fullMatrix.m22(), 0, 'f', 6)
           .arg(fullMatrix.dx(),  0, 'f', 6)
           .arg(fullMatrix.dy(),  0, 'f', 6);
} // buildTransform()

// ---------------------------------------------------------------------------
// Mouse event overrides
// ---------------------------------------------------------------------------

/// @brief Record scene position and item position at the start of a press.
void PieceOverlayItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    // Capture starting positions for later delta calculation.
    m_dragStartScenePos = event->scenePos();
    m_dragStartItemPos  = pos();
    m_dragging          = false;

    // Bring this piece to the top of the z-order while dragging.
    setZValue(10.0);

    QGraphicsRectItem::mousePressEvent(event);
}

/// @brief Move the item by the scene-coordinate delta from press origin.
void PieceOverlayItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        // Override default QGraphicsItem drag handling to track movement delta in scene coordinates, which prevents the feedback loop that would occur if we used item-space coordinates (event->pos()) that shift as the item moves under the cursor. This also allows for more intuitive dragging when zoomed or transformed.
        // Fires multiple times as mouse is dragged, don't add debug messages here. The final position and rotation are logged in buildTransform() when the transform string is built for the current state.
        const QPointF delta = event->scenePos() - m_dragStartScenePos;

        if (!m_dragging && (std::abs(delta.x()) > DRAG_THRESHOLD ||
                            std::abs(delta.y()) > DRAG_THRESHOLD)) {
            // Threshold crossed — engage drag mode.
            m_dragging = true;
        } // if threshold crossed

        if (m_dragging) {
            // Apply delta from the captured start position.
            setPos(m_dragStartItemPos + delta);
        } // if dragging
    } // if left button held

    // Call base class to preserve default processing for item selection and other flags.
    QGraphicsRectItem::mouseMoveEvent(event);
}

/// @brief Restore z-order and clear drag state on button release.
void PieceOverlayItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    const bool wasDragging = m_dragging;

    // Return item to its normal z-layer.
    setZValue(1.0);
    m_dragging = false;

    if (wasDragging) {
        recordCurrentState();
        if (AdjustScene* adjustScene = dynamic_cast<AdjustScene*>(scene())) {
            adjustScene->notifyPieceStateCommitted(m_id, m_historyIndex);
        }
    }

    QGraphicsRectItem::mouseReleaseEvent(event);
}

// ---------------------------------------------------------------------------
// Context menu
// ---------------------------------------------------------------------------

/// @brief Show the rotation / flip / align / abut context menu.
void PieceOverlayItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu menu;

    // Header action — shows the piece's human-readable name ("Front Bodice")
    // when the Seamly2D handoff supplied one, falling back to the id. Not interactive.
    QAction* header = menu.addAction(QString("Piece: %1").arg(displayLabel()));
    header->setEnabled(false);
    menu.addSeparator();

    // Rotation actions.
    QAction* rot10C    = menu.addAction("10 C");
    QAction* rot10CC   = menu.addAction("10 CC");
    QAction* rot225C   = menu.addAction("22.5 C");
    QAction* rot225CC  = menu.addAction("22.5 CC");
    QAction* rot45C    = menu.addAction("45 C");
    QAction* rot45CC   = menu.addAction("45 CC");
    QAction* rot90C    = menu.addAction("90 C");
    QAction* rot90CC   = menu.addAction("90 CC");
    QAction* rot180    = menu.addAction("180");

    menu.addSeparator();

    // Flip actions.
    QAction* flipH = menu.addAction("Flip Horizontal");
    QAction* flipV = menu.addAction("Flip Vertical");

    menu.addSeparator();

    // Align to contentRect edge actions.
    AdjustScene* adjustScene = dynamic_cast<AdjustScene*>(scene());
    const bool hasContentRect = adjustScene && adjustScene->hasContentRect();

    QAction* alignLeft   = menu.addAction("Align Left Edge");
    QAction* alignRight  = menu.addAction("Align Right Edge");
    QAction* alignTop    = menu.addAction("Align Top Edge");
    QAction* alignBottom = menu.addAction("Align Bottom Edge");

    // Per-direction gating: enable only when contentRect exists AND the piece is
    // not already flush with that edge (otherwise the action would be a no-op).
    constexpr double kEdgeTol = 0.5;
    if (hasContentRect) {
        const QRectF cr   = adjustScene->contentRect();
        const QRectF bbox = sceneBoundingRect();
        alignLeft->setEnabled(std::abs(bbox.left()   - cr.left())   > kEdgeTol);
        alignRight->setEnabled(std::abs(bbox.right()  - cr.right())  > kEdgeTol);
        alignTop->setEnabled(std::abs(bbox.top()    - cr.top())    > kEdgeTol);
        alignBottom->setEnabled(std::abs(bbox.bottom() - cr.bottom()) > kEdgeTol);
    } else {
        alignLeft->setEnabled(false);
        alignRight->setEnabled(false);
        alignTop->setEnabled(false);
        alignBottom->setEnabled(false);
    }

    menu.addSeparator();

    // Abut to nearest piece actions (enabled when a piece exists in that direction).
    PieceOverlayItem* nearestLeft  = adjustScene ? adjustScene->findNearestPieceLeft(this) : nullptr;
    PieceOverlayItem* nearestRight = adjustScene ? adjustScene->findNearestPieceRight(this) : nullptr;
    PieceOverlayItem* nearestAbove = adjustScene ? adjustScene->findNearestPieceAbove(this) : nullptr;
    PieceOverlayItem* nearestBelow = adjustScene ? adjustScene->findNearestPieceBelow(this) : nullptr;

    QAction* abutLeft  = menu.addAction("Abut Left Piece");
    QAction* abutRight = menu.addAction("Abut Right Piece");
    QAction* abutAbove = menu.addAction("Abut Above Piece");
    QAction* abutBelow = menu.addAction("Abut Below Piece");

    abutLeft->setEnabled(nearestLeft != nullptr);
    abutRight->setEnabled(nearestRight != nullptr);
    abutAbove->setEnabled(nearestAbove != nullptr);
    abutBelow->setEnabled(nearestBelow != nullptr);

    menu.addSeparator();

    // Z-order stacking actions.
    QAction* raiseToTop    = menu.addAction("Raise to Top");
    QAction* lowerToBottom = menu.addAction("Lower to Bottom");

    QAction* chosen = menu.exec(event->screenPos());

    // Set rotation pivot at the bounding-box center (see constructor comment).
    setTransformOriginPoint(m_w / 2.0, m_h / 2.0);

    const double prevRotation = rotation();
    const double prevScaleX = m_scaleX;
    const double prevScaleY = m_scaleY;
    const QPointF prevPos = pos();
    const qreal prevZValue = zValue();
    bool stateChanged = false;

    if (chosen == rot10C) {
        setRotation(rotation() + 10.0);
    } else if (chosen == rot10CC) {
        setRotation(rotation() - 10.0);
    } else if (chosen == rot225C) {
        setRotation(rotation() + 22.5);
    } else if (chosen == rot225CC) {
        setRotation(rotation() - 22.5);
    } else if (chosen == rot45C) {
        setRotation(rotation() + 45.0);
    } else if (chosen == rot45CC) {
        setRotation(rotation() - 45.0);
    } else if (chosen == rot90C) {
        setRotation(rotation() + 90.0);
    } else if (chosen == rot90CC) {
        setRotation(rotation() - 90.0);
    } else if (chosen == rot180) {
        setRotation(rotation() + 180.0);
    } else if (chosen == flipH) {
        // Toggle horizontal flip.
        m_scaleX = (m_scaleX > 0) ? -1.0 : 1.0;
        stateChanged = true;
    } else if (chosen == flipV) {
        // Toggle vertical flip.
        m_scaleY = (m_scaleY > 0) ? -1.0 : 1.0;
        stateChanged = true;
    } else if (chosen == alignLeft && hasContentRect) {
        // Move piece so its left edge touches contentRect left edge.
        const QRectF contentRect = adjustScene->contentRect();
        const QRectF bbox = sceneBoundingRect();
        const double dx = contentRect.left() - bbox.left();
        setPos(pos().x() + dx, pos().y());
        stateChanged = true;
    } else if (chosen == alignRight && hasContentRect) {
        // Move piece so its right edge touches contentRect right edge.
        const QRectF contentRect = adjustScene->contentRect();
        const QRectF bbox = sceneBoundingRect();
        const double dx = contentRect.right() - bbox.right();
        setPos(pos().x() + dx, pos().y());
        stateChanged = true;
    } else if (chosen == alignTop && hasContentRect) {
        // Move piece so its top edge touches contentRect top edge.
        const QRectF contentRect = adjustScene->contentRect();
        const QRectF bbox = sceneBoundingRect();
        const double dy = contentRect.top() - bbox.top();
        setPos(pos().x(), pos().y() + dy);
        stateChanged = true;
    } else if (chosen == alignBottom && hasContentRect) {
        // Move piece so its bottom edge touches contentRect bottom edge.
        const QRectF contentRect = adjustScene->contentRect();
        const QRectF bbox = sceneBoundingRect();
        const double dy = contentRect.bottom() - bbox.bottom();
        setPos(pos().x(), pos().y() + dy);
        stateChanged = true;
    } else if (chosen == abutLeft && nearestLeft) {
        // Move piece so its left edge touches the nearest piece's right edge.
        const QRectF myBbox = sceneBoundingRect();
        const QRectF otherBbox = nearestLeft->sceneBoundingRect();
        const double dx = otherBbox.right() - myBbox.left();
        setPos(pos().x() + dx, pos().y());
        stateChanged = true;
    } else if (chosen == abutRight && nearestRight) {
        // Move piece so its right edge touches the nearest piece's left edge.
        const QRectF myBbox = sceneBoundingRect();
        const QRectF otherBbox = nearestRight->sceneBoundingRect();
        const double dx = otherBbox.left() - myBbox.right();
        setPos(pos().x() + dx, pos().y());
        stateChanged = true;
    } else if (chosen == abutAbove && nearestAbove) {
        // Move piece so its top edge touches the nearest piece's bottom edge.
        const QRectF myBbox = sceneBoundingRect();
        const QRectF otherBbox = nearestAbove->sceneBoundingRect();
        const double dy = otherBbox.bottom() - myBbox.top();
        setPos(pos().x(), pos().y() + dy);
        stateChanged = true;
    } else if (chosen == abutBelow && nearestBelow) {
        // Move piece so its bottom edge touches the nearest piece's top edge.
        const QRectF myBbox = sceneBoundingRect();
        const QRectF otherBbox = nearestBelow->sceneBoundingRect();
        const double dy = otherBbox.top() - myBbox.bottom();
        setPos(pos().x(), pos().y() + dy);
        stateChanged = true;
    } else if (chosen == raiseToTop && adjustScene) {
        // Raise piece to top of z-order stack.
        const qreal maxZ = adjustScene->maxPieceZValue();
        if (zValue() < maxZ + 1.0) {
            setZValue(maxZ + 1.0);
        }
        // Z-order change doesn't need state recording (visual only).
    } else if (chosen == lowerToBottom && adjustScene) {
        // Lower piece to bottom of z-order stack.
        const qreal minZ = adjustScene->minPieceZValue();
        // Keep above background (z=0), so use minZ - 0.5 but not below 0.5.
        const qreal newZ = qMax(0.5, minZ - 0.5);
        if (zValue() > newZ) {
            setZValue(newZ);
        }
        // Z-order change doesn't need state recording (visual only).
    }

    // Check if rotation changed.
    if (std::abs(rotation() - prevRotation) > 0.001) {
        stateChanged = true;
    }

    // Record state if any transform property changed (not z-order, which is visual only).
    if (stateChanged) {
        recordCurrentState();
        if (adjustScene) {
            adjustScene->notifyPieceStateCommitted(m_id, m_historyIndex);
        }
    }

    // Log the overlay transform change from context menu action.
    if (chosen) {
        qDebug() << "[PieceOverlayItem] contextMenuEvent: context menu overlay transform: id='" + m_id + "'"
                 << " pos:" << prevPos << "->" << pos()
                 << " zValue:" << prevZValue << "->" << zValue()
                 << " rotation:" << prevRotation << "->" << rotation()
                 << " scaleX:" << prevScaleX << "->" << m_scaleX
                 << " scaleY:" << prevScaleY << "->" << m_scaleY
                 << " transformStr='" << m_transformStr << "'"
                 << " historyIndex=" << m_historyIndex
                 << " historySize=" << m_transformHistory.size()
                 << " transformOrigin=(" << (m_w / 2.0) << "," << (m_h / 2.0) << ")";
    } // if a menu action was chosen
}

// Add this method to always reset the overlay box to its original SVG position.
void PieceOverlayItem::resetToOriginalPosition()
{
    // Always use the original SVG (x, y) position for the overlay box.
    QGraphicsRectItem::setPos(m_originX, m_originY);
}

// When applying a move or rotation, only append the transform to the SVG attribute.
// Do not update the QGraphicsItem position here.
void PieceOverlayItem::applyTransformToSvg(const QString& newTransform)
{
    // Fetch the current SVG transform attribute using xmltree/svg_dom (never regex).
    QString currentTransform = getSvgTransformAttribute(); // Implement using xmltree/svg_dom
    QString updatedTransform = currentTransform.isEmpty()
        ? newTransform
        : currentTransform + " " + newTransform;
    setSvgTransformAttribute(updatedTransform); // Implement using xmltree/svg_dom
    // Do NOT update QGraphicsItem position here.
}

// After applying a move or rotation, always reset the overlay box to its original SVG position.
// This prevents double application of transforms.
void PieceOverlayItem::applyTransformAndReset(const QString& newTransform)
{
    applyTransformToSvg(newTransform);
    resetToOriginalPosition(); // <-- This ensures overlay is reset after every apply
}

// In the reload logic (e.g., after apply or scene reload), always call resetToOriginalPosition()
// for each overlay box so it is positioned at the original SVG (x, y).
// Example usage in scene reload:
/*
void AdjustScene::reload()
{
    // ...existing code...
    for (PieceOverlayItem* item : m_pieceOverlayItems) {
        item->resetToOriginalPosition();
        // Do not apply transforms to QGraphicsItem; SVG handles all transforms.
    }
    // ...existing code...
}
*/

QString PieceOverlayItem::getSvgTransformAttribute() const
{
    // TODO: Implement using xmltree/svg_dom
    return QString();
}

void PieceOverlayItem::setSvgTransformAttribute(const QString& /*transform*/)
{
    // TODO: Implement using xmltree/svg_dom
}
