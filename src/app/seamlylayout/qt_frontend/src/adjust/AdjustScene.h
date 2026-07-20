/// @brief Update all overlay piece transform strings after Apply/Enter.
// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file AdjustScene.h
// @brief QGraphicsScene that hosts the layout SVG background and interactive PieceOverlayItems.
//
// loadLayout() parses the bbox JSON, renders the SVG as a static background item,
// and creates one PieceOverlayItem per piece entry. collectTransformsJson() serialises
// each piece's current SVG transform string so the caller can patch the DOM.
// checkOverlaps() performs axis-aligned bounding-box collision detection.

#pragma once

#include "PieceOverlayItem.h"

#include <QGraphicsScene>
#include <QHash>
#include <QList>
#include <QString>
#include <QVector>

class QGraphicsSvgItem;

/// @class AdjustScene
/// @brief Scene that holds the layout SVG background and all interactive PieceOverlayItems.
class AdjustScene : public QGraphicsScene
{
    Q_OBJECT

public:
    /// @brief Construct an empty AdjustScene.
    /// @param parent Optional QObject parent.
    explicit AdjustScene(QObject* parent = nullptr);

    /// @brief Update all overlay piece transform strings after Apply/Enter.
    void updateAllTransforms();

    /// @brief Return the transform string for the moved piece, or empty if none moved.
    QString getMovedTransform() const;

    /// @brief Return true if the current selection-aware Undo action is available.
    bool canUndo() const;

    /// @brief Return true if the current selection-aware Redo action is available.
    bool canRedo() const;

    /// @brief Undo the selected piece when exactly one is selected, otherwise undo globally.
    bool undoLastOperation();

    /// @brief Redo the selected piece when exactly one is selected, otherwise redo globally.
    bool redoLastOperation();

    /// @brief Record that a piece committed a new local history state.
    void notifyPieceStateCommitted(const QString& pieceId, int pieceHistoryIndex);

    /// @brief Load (or reload) a layout.
    ///
    /// Clears the scene, then adds:
    ///  - A non-interactive QGraphicsSvgItem as the static background.
    ///  - One PieceOverlayItem per entry in the @a bboxJson pieces array.
    ///
    /// @param svgPath  Absolute path to the layout SVG file.
    /// @param bboxJson JSON string with margin and pieces array (see project context).
    void loadLayout(const QString& svgPath, const QString& bboxJson);

    /// @brief Collect the current SVG transform string for every piece.
    ///
    /// @return Compact JSON array: @c [{"id":"...","transform":"..."},...]
    QString collectTransformsJson() const;

    /// @brief Return the ids of pieces whose scene bounding rects overlap.
    QStringList checkOverlaps() const;

        /// @brief Return ids of pieces that overlap or are outside contentRect.
        QStringList checkLayoutConflicts() const;

        /// @brief Highlight conflicted pieces with a red 2px outline.
        /// @param conflictIds Piece ids that should be highlighted.
        void highlightConflicts(const QStringList& conflictIds);

        /// @brief Clear conflict highlight on all overlays.
        void clearConflictHighlights();

    /// @brief Remove all PieceOverlayItem overlays from the scene, leaving the SVG background.
    void clearPieces();

    /// @brief Return the contentRect bounds parsed from the layout SVG.
    /// @return The contentRect in scene coordinates, or an empty QRectF if not set.
    QRectF contentRect() const { return m_contentRect; }

    /// @brief Return true if contentRect is valid for boundary checks.
    bool hasContentRect() const { return m_hasContentRect; }

    /// @brief Find the nearest piece to the left of the given piece.
    /// @param piece The reference piece.
    /// @return Pointer to the nearest piece, or nullptr if none found.
    PieceOverlayItem* findNearestPieceLeft(const PieceOverlayItem* piece) const;

    /// @brief Find the nearest piece to the right of the given piece.
    /// @param piece The reference piece.
    /// @return Pointer to the nearest piece, or nullptr if none found.
    PieceOverlayItem* findNearestPieceRight(const PieceOverlayItem* piece) const;

    /// @brief Find the nearest piece above the given piece.
    /// @param piece The reference piece.
    /// @return Pointer to the nearest piece, or nullptr if none found.
    PieceOverlayItem* findNearestPieceAbove(const PieceOverlayItem* piece) const;

    /// @brief Find the nearest piece below the given piece.
    /// @param piece The reference piece.
    /// @return Pointer to the nearest piece, or nullptr if none found.
    PieceOverlayItem* findNearestPieceBelow(const PieceOverlayItem* piece) const;

    /// @brief Return the maximum z-value among all piece overlays.
    /// @return The highest z-value, or 1.0 if no pieces exist.
    qreal maxPieceZValue() const;

    /// @brief Return the minimum z-value among all piece overlays.
    /// @return The lowest z-value, or 1.0 if no pieces exist.
    qreal minPieceZValue() const;

#ifdef QT_DEBUG
    /// @brief Dump all overlay piece data to output/adjust_overlay_<counter>.json.
    void dumpOverlayData() const;
#else
    /// @brief No-op in release builds — debug file output is disabled.
    inline void dumpOverlayData() const {}
#endif

signals:
    /// @brief Emitted whenever Undo/Redo availability changes.
    void undoRedoAvailabilityChanged(bool canUndo, bool canRedo);

        /// @brief Emitted after each committed move/rotate operation.
        /// Carries current conflicts (overlap or outside contentRect).
        void operationConflictsDetected(const QStringList& conflictIds);

private:
    struct SceneOperation
    {
        QString pieceId;
        int pieceHistoryIndex = -1;
    };

    /// @brief All interactive piece items currently in the scene.
    QList<PieceOverlayItem*> m_pieces;

    /// @brief Saved per-piece histories reused after overlay reloads.
    QHash<QString, PieceOverlayItem::HistorySnapshot> m_pieceHistorySnapshots;

    /// @brief Global operation ordering across all pieces in the scene.
    QVector<SceneOperation> m_operationLog;

    /// @brief Applied operations in the order they were applied globally.
    QVector<int> m_globalUndoStack;

    /// @brief Undone operations in the order they were undone globally.
    QVector<int> m_globalRedoStack;

    /// @brief The static SVG background item (may be nullptr before loadLayout).
    QGraphicsSvgItem* m_background = nullptr;

        /// @brief contentRect bounds parsed from the layout SVG (scene coordinates).
        QRectF m_contentRect;

        /// @brief True when m_contentRect is valid for boundary checks.
        bool m_hasContentRect = false;

#ifdef QT_DEBUG
    /// @brief Global counter for numbered overlay debug dumps (GUI-thread only).
    static int s_overlayDumpCounter;
#endif

    /// @brief Save live per-piece histories before overlays are destroyed.
    void saveHistorySnapshots();

    /// @brief Restore a saved history snapshot into a recreated overlay item.
    void restoreHistorySnapshot(PieceOverlayItem* item);

    /// @brief Find a live overlay by piece id.
    PieceOverlayItem* findPieceById(const QString& pieceId) const;

    /// @brief Return the single selected piece, or nullptr when selection is empty/ambiguous.
    PieceOverlayItem* selectedPiece() const;

    /// @brief Return true if a selected piece can undo locally.
    bool canSelectedPieceUndo() const;

    /// @brief Return true if a selected piece can redo locally.
    bool canSelectedPieceRedo() const;

    /// @brief Return true if the global session history can undo.
    bool canGlobalUndo() const;

    /// @brief Return true if the global session history can redo.
    bool canGlobalRedo() const;

    /// @brief Undo the selected piece using its local history.
    bool undoSelectedPiece();

    /// @brief Redo the selected piece using its local history.
    bool redoSelectedPiece();

    /// @brief Undo the latest applied global operation.
    bool undoGlobalOperation();

    /// @brief Redo the latest globally undone operation.
    bool redoGlobalOperation();

    /// @brief Find the newest applied operation for a piece on the global undo stack.
    int findUndoStackPositionForPiece(const PieceOverlayItem* item) const;

    /// @brief Find the newest undone operation for a piece on the global redo stack.
    int findRedoStackPositionForPiece(const PieceOverlayItem* item) const;

    /// @brief Emit the latest Undo/Redo availability state.
    void emitUndoRedoAvailability();

    /// @brief Clear all conflict highlights, then highlight @a actorId red
    ///        iff it is involved in a piece-vs-piece overlap. Pieces that the
    ///        actor lands on are intentionally not highlighted — feedback is
    ///        about the user's most recent action only.
    void refreshActorConflictHighlight(const QString& actorId);
};
