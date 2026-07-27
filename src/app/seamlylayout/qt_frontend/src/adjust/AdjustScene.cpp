// ---------------------------------------------------------------------------
// updateAllTransforms
// ---------------------------------------------------------------------------

// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file AdjustScene.cpp
// @brief Implementation of AdjustScene — SVG background + PieceOverlayItem management.

#include "AdjustScene.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QtXml/QDomDocument>
#include <QFile>
#include <QGraphicsSvgItem>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRectF>
#include <QStringList>
#include <QRegularExpression>
#include <limits>

// Static counter for sequentially numbered overlay debug dumps (GUI-thread only).
// Only compiled in debug builds alongside dumpOverlayData().
#ifdef QT_DEBUG
int AdjustScene::s_overlayDumpCounter = 0;
#endif

namespace {

/// @brief Parse a numeric SVG attribute to double; defaults to 0.0 when invalid.
double parseSvgNumber(const QString& value)
{
    bool ok = false;
    const double parsed = value.trimmed().toDouble(&ok);
    return ok ? parsed : 0.0;
}

/// @brief Find the first DOM element by id, searching depth-first.
QDomElement findElementById(const QDomElement& root, const QString& targetId)
{
    if (root.isNull()) {
        return QDomElement();
    }

    if (root.attribute(QStringLiteral("id")) == targetId) {
        return root;
    }

    QDomElement child = root.firstChildElement();
    while (!child.isNull()) {
        const QDomElement hit = findElementById(child, targetId);
        if (!hit.isNull()) {
            return hit;
        }
        child = child.nextSiblingElement();
    }

    return QDomElement();
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

/// @brief Construct an empty AdjustScene with no background or pieces.
AdjustScene::AdjustScene(QObject* parent)
    : QGraphicsScene(parent)
    , m_background(nullptr)
{
    connect(this, &QGraphicsScene::selectionChanged,
            this, &AdjustScene::emitUndoRedoAvailability);
}

bool AdjustScene::canUndo() const
{
    return canSelectedPieceUndo() || canGlobalUndo();
}

bool AdjustScene::canRedo() const
{
    return canSelectedPieceRedo() || canGlobalRedo();
}

void AdjustScene::emitUndoRedoAvailability()
{
    emit undoRedoAvailabilityChanged(canUndo(), canRedo());
}

PieceOverlayItem* AdjustScene::findPieceById(const QString& pieceId) const
{
    for (PieceOverlayItem* item : m_pieces) {
        if (item->pieceId() == pieceId) {
            return item;
        }
    }

    return nullptr;
}

PieceOverlayItem* AdjustScene::selectedPiece() const
{
    const QList<QGraphicsItem*> selection = selectedItems();
    if (selection.size() != 1) {
        return nullptr;
    }

    return qgraphicsitem_cast<PieceOverlayItem*>(selection.first());
}

bool AdjustScene::canSelectedPieceUndo() const
{
    const PieceOverlayItem* item = selectedPiece();
    return item && item->canUndo();
}

bool AdjustScene::canSelectedPieceRedo() const
{
    const PieceOverlayItem* item = selectedPiece();
    return item && item->canRedo();
}

bool AdjustScene::canGlobalUndo() const
{
    return !m_pieces.isEmpty() && !m_globalUndoStack.isEmpty();
}

bool AdjustScene::canGlobalRedo() const
{
    return !m_pieces.isEmpty() && !m_globalRedoStack.isEmpty();
}

int AdjustScene::findUndoStackPositionForPiece(const PieceOverlayItem* item) const
{
    if (!item) {
        return -1;
    }

    for (int i = m_globalUndoStack.size() - 1; i >= 0; --i) {
        const int opIndex = m_globalUndoStack[i];
        if (opIndex < 0 || opIndex >= m_operationLog.size()) {
            continue;
        }

        const SceneOperation& op = m_operationLog[opIndex];
        if (op.pieceId == item->pieceId() && op.pieceHistoryIndex == item->historyIndex()) {
            return i;
        }
    }

    return -1;
}

int AdjustScene::findRedoStackPositionForPiece(const PieceOverlayItem* item) const
{
    if (!item) {
        return -1;
    }

    const int targetHistoryIndex = item->historyIndex() + 1;
    for (int i = m_globalRedoStack.size() - 1; i >= 0; --i) {
        const int opIndex = m_globalRedoStack[i];
        if (opIndex < 0 || opIndex >= m_operationLog.size()) {
            continue;
        }

        const SceneOperation& op = m_operationLog[opIndex];
        if (op.pieceId == item->pieceId() && op.pieceHistoryIndex == targetHistoryIndex) {
            return i;
        }
    }

    return -1;
}

void AdjustScene::saveHistorySnapshots()
{
    if (m_pieces.isEmpty()) {
        return;
    }

    m_pieceHistorySnapshots.clear();

    for (PieceOverlayItem* item : m_pieces) {
        m_pieceHistorySnapshots.insert(item->pieceId(), item->historySnapshot());
    }
}

void AdjustScene::restoreHistorySnapshot(PieceOverlayItem* item)
{
    if (!item) {
        return;
    }

    const auto it = m_pieceHistorySnapshots.constFind(item->pieceId());
    if (it == m_pieceHistorySnapshots.cend()) {
        return;
    }

    item->restoreHistorySnapshot(it.value());
}

void AdjustScene::notifyPieceStateCommitted(const QString& pieceId, int pieceHistoryIndex)
{
    if (pieceId.isEmpty() || pieceHistoryIndex < 0) {
        return;
    }

    SceneOperation op;
    op.pieceId = pieceId;
    op.pieceHistoryIndex = pieceHistoryIndex;
    m_operationLog.append(op);
    m_globalUndoStack.append(m_operationLog.size() - 1);
    m_globalRedoStack.clear();

    refreshActorConflictHighlight(pieceId);
    emit operationConflictsDetected(checkLayoutConflicts());

    emitUndoRedoAvailability();
}

void AdjustScene::refreshActorConflictHighlight(const QString& actorId)
{
    clearConflictHighlights();
    if (actorId.isEmpty()) {
        return;
    }
    const QStringList conflictIds = checkLayoutConflicts();
    if (conflictIds.contains(actorId)) {
        highlightConflicts(QStringList{actorId});
    }
}

bool AdjustScene::undoSelectedPiece()
{
    PieceOverlayItem* item = selectedPiece();
    if (!item || !item->canUndo()) {
        return false;
    }

    const int stackPos = findUndoStackPositionForPiece(item);
    if (stackPos < 0) {
        return false;
    }

    const int opIndex = m_globalUndoStack[stackPos];
    m_globalUndoStack.removeAt(stackPos);
    if (!item->undo()) {
        m_globalUndoStack.insert(stackPos, opIndex);
        return false;
    }

    m_globalRedoStack.append(opIndex);
    refreshActorConflictHighlight(item->pieceId());
    emitUndoRedoAvailability();
    return true;
}

bool AdjustScene::redoSelectedPiece()
{
    PieceOverlayItem* item = selectedPiece();
    if (!item || !item->canRedo()) {
        return false;
    }

    const int stackPos = findRedoStackPositionForPiece(item);
    if (stackPos < 0) {
        return false;
    }

    const int opIndex = m_globalRedoStack[stackPos];
    m_globalRedoStack.removeAt(stackPos);
    if (!item->redo()) {
        m_globalRedoStack.insert(stackPos, opIndex);
        return false;
    }

    m_globalUndoStack.append(opIndex);
    refreshActorConflictHighlight(item->pieceId());
    emitUndoRedoAvailability();
    return true;
}

bool AdjustScene::undoGlobalOperation()
{
    if (!canGlobalUndo()) {
        return false;
    }

    const int opIndex = m_globalUndoStack.takeLast();
    if (opIndex < 0 || opIndex >= m_operationLog.size()) {
        return false;
    }

    const SceneOperation& op = m_operationLog[opIndex];
    PieceOverlayItem* item = findPieceById(op.pieceId);
    if (!item || item->historyIndex() != op.pieceHistoryIndex || !item->undo()) {
        m_globalUndoStack.append(opIndex);
        return false;
    }

    m_globalRedoStack.append(opIndex);
    refreshActorConflictHighlight(op.pieceId);
    emitUndoRedoAvailability();
    return true;
}

bool AdjustScene::redoGlobalOperation()
{
    if (!canGlobalRedo()) {
        return false;
    }

    const int opIndex = m_globalRedoStack.takeLast();
    if (opIndex < 0 || opIndex >= m_operationLog.size()) {
        return false;
    }

    const SceneOperation& op = m_operationLog[opIndex];
    PieceOverlayItem* item = findPieceById(op.pieceId);
    if (!item || (item->historyIndex() + 1) != op.pieceHistoryIndex || !item->redo()) {
        m_globalRedoStack.append(opIndex);
        return false;
    }

    m_globalUndoStack.append(opIndex);
    refreshActorConflictHighlight(op.pieceId);
    emitUndoRedoAvailability();
    return true;
}

bool AdjustScene::undoLastOperation()
{
    if (canSelectedPieceUndo()) {
        return undoSelectedPiece();
    }

    return undoGlobalOperation();
}

bool AdjustScene::redoLastOperation()
{
    if (canSelectedPieceRedo()) {
        return redoSelectedPiece();
    }

    return redoGlobalOperation();
}

// ---------------------------------------------------------------------------
// loadLayout
// ---------------------------------------------------------------------------

/// @brief Clear and reload the scene from an SVG path and bbox JSON string.
void AdjustScene::loadLayout(const QString& svgPath, const QString& bboxJson)
{
    saveHistorySnapshots();

    // Remove all existing items and reset tracking containers.
    clear();
    // Note: clear() deletes all items, including the background and pieces, so we must reset our pointers and lists to avoid dangling references.
    m_pieces.clear();
    // Reset the background pointer since the old background item has been deleted by clear().
    m_background = nullptr;
    m_contentRect = QRectF();
    m_hasContentRect = false;

    // Parse contentRect bounds from the current SVG so operation-time validation
    // can catch pieces moved outside printable/content margins.
    QFile svgFile(svgPath);
    if (svgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QDomDocument doc;
        QString parseError;
        int parseLine = 0;
        int parseColumn = 0;
        if (doc.setContent(&svgFile, &parseError, &parseLine, &parseColumn)) {
            const QDomElement root = doc.documentElement();
            const QDomElement contentRect = findElementById(root, QStringLiteral("contentRect"));
            if (!contentRect.isNull()) {
                const double x = parseSvgNumber(contentRect.attribute(QStringLiteral("x")));
                const double y = parseSvgNumber(contentRect.attribute(QStringLiteral("y")));
                const double w = parseSvgNumber(contentRect.attribute(QStringLiteral("width")));
                const double h = parseSvgNumber(contentRect.attribute(QStringLiteral("height")));
                if (w > 0.0 && h > 0.0) {
                    m_contentRect = QRectF(x, y, w, h);
                    m_hasContentRect = true;
                }
            }
        } else {
            qWarning() << "[AdjustScene] loadLayout(): SVG parse failed for contentRect at"
                       << parseLine << ":" << parseColumn << parseError;
        }
        svgFile.close();
    } else {
        qWarning() << "[AdjustScene] loadLayout(): could not open SVG for contentRect:" << svgPath;
    }

    // --- SVG background --------------------------------------------------

    // Create a non-interactive SVG background at z=0.
    QGraphicsSvgItem* bg = new QGraphicsSvgItem(svgPath);
    bg->setFlag(QGraphicsItem::ItemIsMovable,   false);
    bg->setFlag(QGraphicsItem::ItemIsSelectable, false);
    bg->setZValue(0.0);
    addItem(bg);
    // Keep track of the background item so we can size the scene to fit it and avoid dangling pointers after clear().
    m_background = bg;

    // Size the scene to exactly fit the SVG background.
    setSceneRect(bg->boundingRect());

    // --- Parse bbox JSON -------------------------------------------------

    const QByteArray jsonBytes = bboxJson.toUtf8();
    const QJsonDocument doc    = QJsonDocument::fromJson(jsonBytes);

    if (!doc.isObject()) {
        // Malformed JSON — background is shown but no pieces are created.
        return;
    } // if bad JSON

    // Extract the pieces array from the JSON; each entry describes one piece's bbox data: initial position, size, relative origin, and rotation. Used to create overlay items and track their transforms.
    const QJsonObject root   = doc.object();
    const QJsonArray  pieces = root.value("pieces").toArray();

    // Iterate over piece bbox data and create an interactive PieceOverlayItem for each
    for (const QJsonValue& val : pieces) {
        if (!val.isObject()) {
            // Skip malformed entries.
            continue;
        } // if not object

        const QJsonObject obj = val.toObject();

        const QString id     = obj.value("id").toString();
        // Human-readable piece name from the Seamly2D handoff (data-name → data-letter
        // → id, resolved on the Rust side).  Display only; `id` stays the identity.
        const QString label  = obj.value("label").toString();
        const double  x      = obj.value("x").toDouble(0.0); // canvas space, absolute coords
        const double  y      = obj.value("y").toDouble(0.0); // canvas space, absolute coords
        const double  w      = obj.value("w").toDouble(1.0); // can't be 0
        const double  h      = obj.value("h").toDouble(1.0); // can't be 0
        const double  ox     = obj.value("origin_x_px").toDouble(0.0); // local space
        const double  oy     = obj.value("origin_y_px").toDouble(0.0); // local space
        const double  rotDeg = obj.value("rotation_deg").toDouble(0.0); // relative to local origin
        const QString transformStr = obj.value("transform_str").toString(); // accumulated SVG transform from prior Accept cycles

        qDebug() << "[AdjustScene] loadLayout():   " << id
                 << "pos=(" << x << "," << y << ")"
                 << "size=(" << w << "x" << h << ")"
                 << "origin=(" << ox << "," << oy << ")"
                 << "rotDeg=" << rotDeg
                 << "transformStr='" << transformStr << "'";

        // Create an interactive piece overlay item at z=1.
        PieceOverlayItem* item = new PieceOverlayItem(id, ox, oy, x, y, w, h, rotDeg, transformStr); // piece placement (x,y) is in absolute canvas coords; rotation is relative to piece's (ox, oy) in piece local space coords; in this application (ox, oy) is always the default value (0,0) at bbox top-left corner.
        // Attach the display name so the piece's context menu reads "Front Bodice",
        // not "piece-7".  Ignored when the layout carried no label (untagged SVG).
        item->setDisplayLabel(label);
        // Set the item's z-value above the background so it receives mouse events.
        item->setZValue(1.0);
        // Add the item to the scene
        addItem(item);
        // Note: the scene takes ownership of the item, so we do not delete it directly; we just keep track of it in m_pieces for later reference and cleanup.
        m_pieces.append(item);
        restoreHistorySnapshot(item);
    } // for each piece entry

    qDebug() << "[AdjustScene] loadLayout: loaded" << m_pieces.size() << "piece(s)";
    emitUndoRedoAvailability();
} // void loadLayout()

// ---------------------------------------------------------------------------
// getMovedTransform
// ---------------------------------------------------------------------------

/// @brief Serialise each piece's current SVG transform to a compact JSON array.
/// Only pieces that have moved or rotated from their load position are included in the output.
/// Called by AdjustWindow::onApplyClicked() to gather the updated transforms before emitting accepted() and triggering the main application reload.
QString AdjustScene::getMovedTransform() const
{
    QJsonArray transform_arr;

    qDebug() << "[AdjustScene] getMovedTransform(): checking" << m_pieces.size() << "piece(s):";
    // Iterate over pieces and build a JSON array of {id, transform} objects for those that have moved or rotated.
    for (const PieceOverlayItem* item : m_pieces) {
        // process only the item that moved or rotated
        if (!item->hasMoved()) {
            continue;
        }
        // get the old transform string from the item before rebuilding it
        const QString xf_old = item->transformStr();
        qDebug() << "[AdjustScene] getMovedTransform():   '" + item->pieceId() + "' has moved, old transform string: '" + xf_old + "'";

        // buildTransform() returns the current transform string directly, so use
        // that value for Apply serialization instead of the stale cached string.
        qDebug() << "[AdjustScene] getMovedTransform():   calling buildTransform() to build new transform string";
        const QString xf_new = item->buildTransform();

        // Build json object with new transform string
        qDebug() << "[AdjustScene] getMovedTransform():   '" + item->pieceId() + "'  transform='" + xf_new + "'";
        QJsonObject obj;
        obj["id"]        = item->pieceId();
        obj["transform"] = xf_new;

        // Append transform object to json array.
        transform_arr.append(obj);
    }
    // serialize the json array
    const QString transform_str= QString::fromUtf8(QJsonDocument(transform_arr).toJson(QJsonDocument::Compact));
    qDebug() << "[AdjustScene] getMovedTransform(): final transforms JSON:" << transform_str;

    // return to onApplyClicked()
    return transform_str;
} // getMovedTransform()

// ---------------------------------------------------------------------------
// updateAllTransforms
// ---------------------------------------------------------------------------

/// @brief Update all overlay piece with transform string after Apply/Enter.
void AdjustScene::updateAllTransforms()
{
    for (PieceOverlayItem* item : m_pieces) {
        item->setTransformStr(item->buildTransform());
    }
    qDebug() << "[AdjustScene] updateAllTransforms(): updated transform strings for" << m_pieces.size() << "piece(s)";
}

// ---------------------------------------------------------------------------
// checkOverlaps
// ---------------------------------------------------------------------------

/// @brief Remove PieceOverlayItem overlays from the scene; keep the SVG background.
void AdjustScene::clearPieces()
// Note: the scene takes ownership of all items, so we just remove them from the scene and clear our tracking list; we do not delete them directly.
{
    saveHistorySnapshots();

    // remove overlays from scene
    for (PieceOverlayItem* item : m_pieces) {
        removeItem(item);
        delete item;
    } // for each piece
    // Clear the list of overlay pieces since they have been removed from the scene.
    m_pieces.clear();
    emitUndoRedoAvailability();
} // clearPieces

/// @brief Return ids of pieces whose axis-aligned bounding rects intersect.
QStringList AdjustScene::checkOverlaps() const
{
    QStringList conflicting;

    // O(n²) pair-wise check — piece counts are small (typically < 100).
    for (int i = 0; i < m_pieces.size(); ++i) {
        const QRectF r1 = m_pieces[i]->sceneBoundingRect();

        for (int j = i + 1; j < m_pieces.size(); ++j) {
            const QRectF r2 = m_pieces[j]->sceneBoundingRect();

            if (r1.intersects(r2)) {
                // Add each piece id at most once.
                const QString id1 = m_pieces[i]->pieceId();
                const QString id2 = m_pieces[j]->pieceId();

                if (!conflicting.contains(id1)) {
                    conflicting.append(id1);
                } // if id1 not already listed

                if (!conflicting.contains(id2)) {
                    conflicting.append(id2);
                } // if id2 not already listed
            } // if rects intersect
        } // for j (inner piece)
    } // for i (outer piece)

    return conflicting;
}

QStringList AdjustScene::checkLayoutConflicts() const
{
    // Layout conflicts = piece-vs-piece overlaps only.
    // Pieces extending past contentRect bounds are no longer flagged as
    // conflicts; the user controls page-bounds discipline directly.
    return checkOverlaps();
}

void AdjustScene::highlightConflicts(const QStringList& conflictIds)
{
    for (PieceOverlayItem* item : m_pieces) {
        if (!item) {
            continue;
        }
        item->setConflictHighlighted(conflictIds.contains(item->pieceId()));
    }
}

void AdjustScene::clearConflictHighlights()
{
    for (PieceOverlayItem* item : m_pieces) {
        if (!item) {
            continue;
        }
        item->setConflictHighlighted(false);
    }
}

// ---------------------------------------------------------------------------
// dumpOverlayData
// ---------------------------------------------------------------------------

#ifdef QT_DEBUG
/// @brief Dump all overlay piece data to output/adjust_overlay_<counter>.json.
void AdjustScene::dumpOverlayData() const
{
    const int count = s_overlayDumpCounter++;

    // Build the output path next to the executable.
    const QString outDir = QCoreApplication::applicationDirPath() + "/output";
    QDir().mkpath(outDir);
    const QString path = QString("%1/adjust_overlay_%2.json").arg(outDir).arg(count);

    // Build JSON array with full state for every overlay piece.
    QJsonArray arr;
    for (const PieceOverlayItem* item : m_pieces) {
        QJsonObject obj;
        obj["id"]              = item->pieceId();
        obj["pos_x"]           = item->pos().x(); // current position in scene coordinates (canvas space)
        obj["pos_y"]           = item->pos().y(); //
        obj["rotation"]        = item->rotation();
        obj["hasMoved"]        = item->hasMoved();
        obj["transformStr"]    = item->transformStr();
        obj["rect_w"]          = item->rect().width();
        obj["rect_h"]          = item->rect().height();
        obj["origin_x"]        = item->originX();
        obj["origin_y"]        = item->originY();
        obj["initial_pos_x"]   = item->initialPos().x();
        obj["initial_pos_y"]   = item->initialPos().y();
        obj["initial_rotation"] = item->initialRotation();
        arr.append(obj);

        // if item has moved, log the details of the move for debugging
        if (item->hasMoved()) {
            qDebug() << "[AdjustScene] dumpOverlayData(): 1 piece '" + item->pieceId() + "' has moved:";
            qDebug() << "    origin pos=(" << item->originX() << "," << item->originY() << ")";
            qDebug() << "    current pos=(" << item->pos().x() << "," << item->pos().y() << ") rotation=" << item->rotation();
            qDebug() << "    initial pos=(" << item->initialPos().x() << "," << item->initialPos().y() << ") rotation=" << item->initialRotation();
            qDebug() << "    transformStr='" << item->transformStr() << "'";
        } // if item has moved

    } // for each piece

    // debug message with overlay data being dumped
    qDebug() << "[AdjustScene] dumpOverlayData(): 2 dumping" << m_pieces.size() << "piece(s) to" << path;

    // Write the JSON file.
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        file.close();
        qDebug() << "[AdjustScene] dumpOverlayData(): 3 saved " << m_pieces.size()
                 << "piece(s) to" << path;
    } else {
        qWarning() << "[AdjustScene] dumpOverlayData(): 4 failed to write" << path;
    } // if file opened
} // dumpOverlayData
#endif // QT_DEBUG

// ---------------------------------------------------------------------------
// findNearestPiece helpers
// ---------------------------------------------------------------------------

namespace {
/// @brief Minimum gap (scene units) required to consider two pieces non-touching.
/// Touching pieces are excluded so "Abut" never offers a no-op.
constexpr double kAbutGapTol = 0.5;

/// @brief Return true if [a0, a1] and [b0, b1] overlap on a 1-D axis (open intervals).
inline bool intervalsOverlap(double a0, double a1, double b0, double b1)
{
    return a1 > b0 && b1 > a0;
}
} // anonymous namespace

/// @brief Find the nearest piece to the left of the given piece.
/// Returns the immediate left neighbor (overlapping on the Y axis) only when
/// there is a real gap > kAbutGapTol. If the immediate neighbor is already
/// touching, returns nullptr so "Abut Left" is disabled (no-op suppression).
PieceOverlayItem* AdjustScene::findNearestPieceLeft(const PieceOverlayItem* piece) const
{
    if (!piece) {
        return nullptr;
    }

    const QRectF myBbox = piece->sceneBoundingRect();
    PieceOverlayItem* nearest = nullptr;
    double minDistance = std::numeric_limits<double>::max();

    for (PieceOverlayItem* other : m_pieces) {
        if (!other || other == piece) {
            continue;
        }

        const QRectF otherBbox = other->sceneBoundingRect();

        if (!intervalsOverlap(myBbox.top(), myBbox.bottom(), otherBbox.top(), otherBbox.bottom())) {
            continue;
        }

        // Other must sit at or above my left edge (allow tiny overlap as "touching").
        const double distance = myBbox.left() - otherBbox.right();
        if (distance < -kAbutGapTol) {
            continue;
        }
        if (distance < minDistance) {
            minDistance = distance;
            nearest = other;
        }
    }

    return (nearest && minDistance > kAbutGapTol) ? nearest : nullptr;
}

/// @brief Find the nearest piece to the right of the given piece.
/// Returns the immediate right neighbor (overlapping on the Y axis) only when
/// there is a real gap > kAbutGapTol. Already-touching neighbor → nullptr.
PieceOverlayItem* AdjustScene::findNearestPieceRight(const PieceOverlayItem* piece) const
{
    if (!piece) {
        return nullptr;
    }

    const QRectF myBbox = piece->sceneBoundingRect();
    PieceOverlayItem* nearest = nullptr;
    double minDistance = std::numeric_limits<double>::max();

    for (PieceOverlayItem* other : m_pieces) {
        if (!other || other == piece) {
            continue;
        }

        const QRectF otherBbox = other->sceneBoundingRect();

        if (!intervalsOverlap(myBbox.top(), myBbox.bottom(), otherBbox.top(), otherBbox.bottom())) {
            continue;
        }

        const double distance = otherBbox.left() - myBbox.right();
        if (distance < -kAbutGapTol) {
            continue;
        }
        if (distance < minDistance) {
            minDistance = distance;
            nearest = other;
        }
    }

    return (nearest && minDistance > kAbutGapTol) ? nearest : nullptr;
}

/// @brief Find the nearest piece above the given piece.
/// Returns the immediate above neighbor (overlapping on the X axis) only when
/// there is a real gap > kAbutGapTol. Already-touching neighbor → nullptr.
PieceOverlayItem* AdjustScene::findNearestPieceAbove(const PieceOverlayItem* piece) const
{
    if (!piece) {
        return nullptr;
    }

    const QRectF myBbox = piece->sceneBoundingRect();
    PieceOverlayItem* nearest = nullptr;
    double minDistance = std::numeric_limits<double>::max();

    for (PieceOverlayItem* other : m_pieces) {
        if (!other || other == piece) {
            continue;
        }

        const QRectF otherBbox = other->sceneBoundingRect();

        if (!intervalsOverlap(myBbox.left(), myBbox.right(), otherBbox.left(), otherBbox.right())) {
            continue;
        }

        const double distance = myBbox.top() - otherBbox.bottom();
        if (distance < -kAbutGapTol) {
            continue;
        }
        if (distance < minDistance) {
            minDistance = distance;
            nearest = other;
        }
    }

    return (nearest && minDistance > kAbutGapTol) ? nearest : nullptr;
}

/// @brief Find the nearest piece below the given piece.
/// Returns the immediate below neighbor (overlapping on the X axis) only when
/// there is a real gap > kAbutGapTol. Already-touching neighbor → nullptr.
PieceOverlayItem* AdjustScene::findNearestPieceBelow(const PieceOverlayItem* piece) const
{
    if (!piece) {
        return nullptr;
    }

    const QRectF myBbox = piece->sceneBoundingRect();
    PieceOverlayItem* nearest = nullptr;
    double minDistance = std::numeric_limits<double>::max();

    for (PieceOverlayItem* other : m_pieces) {
        if (!other || other == piece) {
            continue;
        }

        const QRectF otherBbox = other->sceneBoundingRect();

        if (!intervalsOverlap(myBbox.left(), myBbox.right(), otherBbox.left(), otherBbox.right())) {
            continue;
        }

        const double distance = otherBbox.top() - myBbox.bottom();
        if (distance < -kAbutGapTol) {
            continue;
        }
        if (distance < minDistance) {
            minDistance = distance;
            nearest = other;
        }
    }

    return (nearest && minDistance > kAbutGapTol) ? nearest : nullptr;
}

// ---------------------------------------------------------------------------
// Z-order helpers
// ---------------------------------------------------------------------------

/// @brief Return the maximum z-value among all piece overlays.
qreal AdjustScene::maxPieceZValue() const
{
    qreal maxZ = 1.0;
    for (const PieceOverlayItem* item : m_pieces) {
        if (item && item->zValue() > maxZ) {
            maxZ = item->zValue();
        }
    }
    return maxZ;
}

/// @brief Return the minimum z-value among all piece overlays.
qreal AdjustScene::minPieceZValue() const
{
    qreal minZ = 1.0;
    bool found = false;
    for (const PieceOverlayItem* item : m_pieces) {
        if (item) {
            if (!found || item->zValue() < minZ) {
                minZ = item->zValue();
                found = true;
            }
        }
    }
    return minZ;
}
