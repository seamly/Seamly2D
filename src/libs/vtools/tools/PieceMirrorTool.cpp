/******************************************************************************
 **  @file   PieceMirrorTool.cpp
 **  @author slspencer
 **
 **  @brief
 **  Piece Mirror tool: makes a full piece from a half piece and its fold line.
 **
 **  @copyright
 **  This source code is part of the Seamly2D project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
 **
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **
 **  Seamly2D is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Seamly2D is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
 **
 *****************************************************************************/

#include "PieceMirrorTool.h"

#include <QDomElement>
#include <QHash>
#include <QLineF>
#include <QPolygonF>
#include <QRectF>
#include <QUndoStack>
#include <QVector>

#include "pattern_piece_tool.h"
#include "vabstracttool.h"
#include "drawTools/operation/vabstractoperation.h"
#include "drawTools/operation/mirror/vtoolmirrorbyline.h"
#include "nodeDetails/anchorpoint_tool.h"
#include "nodeDetails/internal_path_tool.h"
#include "../dialogs/tools/dialogtool.h"
#include "../dialogs/tools/piece/PieceMirrorDialog.h"
#include "../undocommands/togglepieceinlayout.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vgobject.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/PieceMirrorPath.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vwidgets/vmaingraphicsscene.h"

namespace
{
/**
 * @brief PieceMirrorContext holds what each step of one mirror needs.
 */
struct PieceMirrorContext
{
    VMainGraphicsScene*     piece_scene{nullptr}; ///< Scene of the new piece and its nodes.
    VAbstractPattern*       doc{nullptr};         ///< Pattern document.
    VContainer*             data{nullptr};        ///< Pattern data.
    QString                 block_name;           ///< Draft block of the half piece.
    QHash<quint32, quint32> draft_ids;            ///< Half piece node object id to its draft object id.
    QVector<quint32>        fold_ids;             ///< Draft object ids of the two fold line points.
    QHash<quint32, quint32> mirror_ids;           ///< Draft object id to its mirrored copy.
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief pieceBlockName finds the draft block that holds a piece in the pattern file.
 * @details The active draft block plays no part: it belongs to Draft mode, and Piece mode works on the chosen piece.
 * @return the block name; empty when the document has no such piece.
 */
QString pieceBlockName(VAbstractPattern* doc, quint32 piece_id)
{
    QString block_name;
    const QDomElement piece = doc->elementById(piece_id, VAbstractPattern::TagPiece);

    // A piece element sits in the pieces element of its draft block.
    const QDomElement block = piece.parentNode().parentNode().toElement();
    if (block.tagName() == VAbstractPattern::TagDraftBlock)
    {
        block_name = block.attribute(VAbstractPattern::AttrName);
    }
    return block_name;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief draftObjectId finds the draft object that a piece node copies.
 * @details Each piece node is a modeling copy of a draft object. The modeling element keeps the draft object id.
 * @return the draft object id; NULL_ID when the document has no such node.
 */
quint32 draftObjectId(VAbstractPattern* doc, quint32 node_id)
{
    quint32 draft_id = NULL_ID;
    const QDomElement element = doc->elementById(node_id);
    if (element.isElement())
    {
        draft_id = VDomDocument::GetParametrUInt(element, AttrIdObject, NULL_ID_STR);
    }
    return draft_id;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isMirrorable tells whether a Mirror by Line operation can copy the object.
 */
bool isMirrorable(const VContainer* data, quint32 draft_id)
{
    bool mirrorable = false;
    if (data->DataGObjects()->contains(draft_id))
    {
        const QSharedPointer<VGObject> object = data->GetGObject(draft_id);
        switch (object->getType())
        {
            case GOType::Point:
            case GOType::Arc:
            case GOType::EllipticalArc:
            case GOType::Spline:
            case GOType::SplinePath:
            case GOType::CubicBezier:
            case GOType::CubicBezierPath:
                mirrorable = object->getMode() == Draw::Calculation;
                break;
            case GOType::Curve:
            case GOType::Path:
            case GOType::AllCurves:
            case GOType::Unknown:
            default:
                break;
        }
    }
    return mirrorable;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief addDraftIds finds the draft object of each node.
 * @return false when a node has no draft object.
 */
bool addDraftIds(PieceMirrorContext& context, const QVector<VPieceNode>& nodes)
{
    bool found = true;
    for (const VPieceNode& node : nodes)
    {
        const quint32 draft_id = draftObjectId(context.doc, node.GetId());
        found = found && draft_id != NULL_ID;
        context.draft_ids.insert(node.GetId(), draft_id);
    }
    return found;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief collectSource finds the draft objects of the half piece, and the objects to mirror.
 * @param context  receives the draft object ids and the fold line point ids.
 * @param half     half piece.
 * @param source   receives each draft object off the fold line once, in path order.
 * @return false when an object has no draft object, or when Mirror by Line cannot copy it.
 */
bool collectSource(PieceMirrorContext& context, const VPiece& half, QVector<SourceItem>& source)
{
    // Main path, internal paths, custom seam allowance paths and anchors all go into the full piece. The full main
    // path leaves out excluded nodes, so their objects are not mirrored.
    const QVector<VPieceNode> main_nodes = half.GetPath().getNodes();
    QVector<VPieceNode> half_nodes;
    for (int index : PieceMirrorPath::halfPathIndexes(main_nodes))
    {
        half_nodes.append(main_nodes.at(index));
    }
    bool ok = addDraftIds(context, half_nodes);
    QVector<quint32> ordered;
    for (const VPieceNode& node : half_nodes)
    {
        ordered.append(context.draft_ids.value(node.GetId()));
    }

    // Only the custom seam allowance records that the full piece keeps need their paths mirrored.
    const QVector<CustomSARecord> records = half.getCustomSARecords();
    const QVector<PieceMirrorNode> order  = PieceMirrorPath::fullPathOrder(main_nodes, records);
    QVector<quint32> path_ids = half.getInternalPaths();
    for (const PieceMirrorRange& range : PieceMirrorPath::recordRanges(main_nodes, order, records))
    {
        if (!path_ids.contains(records.at(range.record_index).path))
        {
            path_ids.append(records.at(range.record_index).path);
        }
    }
    for (quint32 path_id : path_ids)
    {
        const QVector<VPieceNode> path_nodes = context.data->getPiecePath(path_id).getNodes();
        ok = addDraftIds(context, path_nodes) && ok;
        for (const VPieceNode& node : path_nodes)
        {
            ordered.append(context.draft_ids.value(node.GetId()));
        }
    }

    for (quint32 anchor : half.getAnchors())
    {
        const quint32 draft_id = draftObjectId(context.doc, anchor);
        ok = ok && draft_id != NULL_ID;
        context.draft_ids.insert(anchor, draft_id);
        ordered.append(draft_id);
    }

    // The fold line points are on the mirror axis; they are their own mirror image.
    const QVector<int> fold = VPiece::foldLineNodeIndexes(main_nodes);
    ok = ok && fold.size() == 2;
    if (ok)
    {
        context.fold_ids = {context.draft_ids.value(main_nodes.at(fold.at(0)).GetId()),
                            context.draft_ids.value(main_nodes.at(fold.at(1)).GetId())};
    }

    QVector<quint32> seen;
    for (quint32 draft_id : ordered)
    {
        if (ok && !seen.contains(draft_id) && !context.fold_ids.contains(draft_id))
        {
            seen.append(draft_id);
            ok = isMirrorable(context.data, draft_id);
            SourceItem item;
            item.id = draft_id;
            source.append(item);
        }
    }
    return ok && !source.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief mirrorObjects adds the Mirror by Line operation and records the mirrored copy of each object.
 * @return false when the operation did not make one copy for each object.
 */
bool mirrorObjects(PieceMirrorContext& context, const QVector<SourceItem>& source, VMainGraphicsScene* draft_scene)
{
    // A history cursor would put the operation before objects it mirrors; the end of the draft block is always
    // after them.
    const quint32 cursor_id = context.doc->getCursorId();
    context.doc->setCursorId(NULL_ID);

    const QString suffix = context.doc->generateSuffix(qApp->Settings()->getMirrorByLineSuffix());
    const VToolMirrorByLine* operation =
            VToolMirrorByLine::Create(0, context.fold_ids.at(0), context.fold_ids.at(1), suffix, source,
                                      QVector<DestinationItem>(), draft_scene, context.doc, context.data,
                                      Document::FullParse, Source::FromGui);
    context.doc->setCursorId(cursor_id);

    // The operation makes one object for each source object, in source order.
    const QVector<DestinationItem> destination =
            operation != nullptr ? operation->getDestination() : QVector<DestinationItem>();
    for (int i = 0; i < source.size() && i < destination.size(); ++i)
    {
        context.mirror_ids.insert(source.at(i).id, destination.at(i).id);
    }
    for (quint32 fold_id : context.fold_ids)
    {
        context.mirror_ids.insert(fold_id, fold_id);
    }
    return destination.size() == source.size();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief pieceNode makes the piece node copy of a draft object.
 * @param node node with the draft object id.
 * @return the node with the id of its new copy.
 */
VPieceNode pieceNode(PieceMirrorContext& context, VPieceNode node)
{
    node.SetId(VAbstractTool::PrepareNode(node, context.piece_scene, context.doc, context.data, context.block_name));
    return node;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief copyPath copies an internal path or a custom seam allowance path for the full piece.
 * @param mirrored true for the copy across the fold line.
 * @return id of the new path.
 */
quint32 copyPath(PieceMirrorContext& context, const VPiecePath& path, bool mirrored)
{
    VPiecePath copy = path;
    copy.Clear();
    for (int i = 0; i < path.nodeCount(); ++i)
    {
        VPieceNode node = path.at(i);
        const quint32 draft_id = context.draft_ids.value(node.GetId());
        node.SetId(mirrored ? context.mirror_ids.value(draft_id, draft_id) : draft_id);
        copy.Append(pieceNode(context, node));
    }

    // The new piece adopts the path tool when it is created.
    const quint32 path_id = context.data->AddPiecePath(copy);
    InternalPathTool::Create(path_id, copy, NULL_ID, context.piece_scene, context.doc, context.data,
                             Document::FullParse, Source::FromTool, context.block_name);
    return path_id;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief liesOnFoldLine tells whether every node of a path is a fold line point, so its mirror is the path itself.
 */
bool liesOnFoldLine(const PieceMirrorContext& context, const VPiecePath& path)
{
    bool on_fold = true;
    for (int i = 0; i < path.nodeCount(); ++i)
    {
        on_fold = on_fold && context.fold_ids.contains(context.draft_ids.value(path.at(i).GetId()));
    }
    return on_fold;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief copyAnchor makes an anchor point of the full piece on a draft point.
 * @return id of the new anchor point.
 */
quint32 copyAnchor(PieceMirrorContext& context, quint32 draft_id)
{
    const AnchorPointTool* anchor = AnchorPointTool::Create(0, draft_id, NULL_ID, context.doc, context.data,
                                                            Document::FullParse, Source::FromGui,
                                                            context.block_name);
    return anchor != nullptr ? anchor->getId() : NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief remapAnchors points label and grainline pins at the anchor copies.
 * @param copies half piece anchor id to its copy on the full piece; a pin without a copy is cleared.
 */
void remapAnchors(VPiece& piece, const QHash<quint32, quint32>& copies)
{
    VPieceLabelData& piece_label = piece.GetPatternPieceData();
    piece_label.setCenterAnchorPoint(copies.value(piece_label.centerAnchorPoint(), NULL_ID));
    piece_label.setTopLeftAnchorPoint(copies.value(piece_label.topLeftAnchorPoint(), NULL_ID));
    piece_label.setBottomRightAnchorPoint(copies.value(piece_label.bottomRightAnchorPoint(), NULL_ID));

    VPatternLabelData& pattern_label = piece.GetPatternInfo();
    pattern_label.setCenterAnchorPoint(copies.value(pattern_label.centerAnchorPoint(), NULL_ID));
    pattern_label.setTopLeftAnchorPoint(copies.value(pattern_label.topLeftAnchorPoint(), NULL_ID));
    pattern_label.setBottomRightAnchorPoint(copies.value(pattern_label.bottomRightAnchorPoint(), NULL_ID));

    VGrainlineData& grainline = piece.GetGrainlineGeometry();
    grainline.setCenterAnchorPoint(copies.value(grainline.centerAnchorPoint(), NULL_ID));
    grainline.setTopAnchorPoint(copies.value(grainline.topAnchorPoint(), NULL_ID));
    grainline.setBottomAnchorPoint(copies.value(grainline.bottomAnchorPoint(), NULL_ID));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createFullPiece makes the full piece from the half piece objects and their mirrored copies.
 * @return the new piece tool.
 */
PatternPieceTool* createFullPiece(PieceMirrorContext& context, const VPiece& half)
{
    const QVector<VPieceNode>     nodes   = half.GetPath().getNodes();
    const QVector<CustomSARecord> records = half.getCustomSARecords();
    const QVector<PieceMirrorNode> order  = PieceMirrorPath::fullPathOrder(nodes, records);

    // Main path: the half, then the mirrored half back to the fold line end.
    VPiecePath full_path = half.GetPath();
    full_path.Clear();
    QVector<quint32> full_ids;
    for (const PieceMirrorNode& entry : order)
    {
        const quint32 draft_id  = context.draft_ids.value(nodes.at(entry.source_index).GetId());
        const quint32 object_id = entry.mirrored ? context.mirror_ids.value(draft_id, draft_id) : draft_id;
        const VPieceNode node   = pieceNode(context, PieceMirrorPath::fullPathNode(nodes, entry, object_id));
        full_path.Append(node);
        full_ids.append(node.GetId());
    }

    // Internal paths: a copy on the half and a mirrored copy, unless the path lies on the fold line.
    QVector<quint32> internal_paths;
    for (quint32 path_id : half.getInternalPaths())
    {
        const VPiecePath path = context.data->getPiecePath(path_id);
        internal_paths.append(copyPath(context, path, false));
        if (!liesOnFoldLine(context, path))
        {
            internal_paths.append(copyPath(context, path, true));
        }
    }

    // Custom seam allowance: each record on one side of the fold line gets a copy on each half.
    QVector<CustomSARecord> full_records;
    for (const PieceMirrorRange& range : PieceMirrorPath::recordRanges(nodes, order, records))
    {
        CustomSARecord record = records.at(range.record_index);
        record.path       = copyPath(context, context.data->getPiecePath(record.path), range.mirrored);
        record.startPoint = full_ids.at(range.start_position);
        record.endPoint   = full_ids.at(range.end_position);
        record.reverse    = range.mirrored ? !record.reverse : record.reverse;
        full_records.append(record);
    }

    // Anchors: a copy of each, and a mirrored copy unless the anchor is a fold line point.
    QVector<quint32> anchors;
    QHash<quint32, quint32> anchor_copies;
    for (quint32 anchor : half.getAnchors())
    {
        const quint32 draft_id = context.draft_ids.value(anchor);
        const quint32 copy_id  = copyAnchor(context, draft_id);
        anchors.append(copy_id);
        anchor_copies.insert(anchor, copy_id);

        const quint32 mirror_id = context.mirror_ids.value(draft_id, draft_id);
        if (mirror_id != draft_id)
        {
            anchors.append(copyAnchor(context, mirror_id));
        }
    }

    // The full piece keeps the half piece properties. It has no fold line, so it is not cut on a fold.
    VPiece full = half;
    full.SetPath(full_path);
    full.SetInternalPaths(internal_paths);
    full.SetCustomSARecords(full_records);
    full.setAnchors(anchors);
    remapAnchors(full, anchor_copies);
    full.SetName(PieceMirrorTool::tr("%1 Mirrored").arg(half.GetName()));
    full.GetPatternPieceData().SetOnFold(false);
    full.setForcedCornerCuts(QVector<int>());
    full.SetUnited(false);
    full.setInLayout(true);

    // Place the full piece to the right of the half piece, 2 cm apart, so it does not cover it.
    const QRectF half_rect = QPolygonF(half.mainPathPoints(context.data)).boundingRect();
    const QRectF full_rect = QPolygonF(full.mainPathPoints(context.data)).boundingRect();
    full.SetMx(half.GetMx() + half_rect.right() - full_rect.left() + ToPixel(2, Unit::Cm));
    full.SetMy(half.GetMy());

    QString width = half.getSeamAllowanceWidthFormula();
    return PatternPieceTool::Create(0, full, width, context.piece_scene, context.doc, context.data,
                                    Document::FullParse, Source::FromTool, context.block_name);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create makes the full piece for the piece the user chose in the dialog.
 * @param dialog      a PieceMirrorDialog.
 * @param draft_scene scene of the Mirror by Line operation.
 * @param piece_scene scene of the full piece.
 * @param doc         pattern document.
 * @param data        pattern data.
 * @return the full piece tool; nullptr when the piece cannot be mirrored.
 */
PatternPieceTool* PieceMirrorTool::Create(QSharedPointer<DialogTool> dialog, VMainGraphicsScene* draft_scene,
                                          VMainGraphicsScene* piece_scene, VAbstractPattern* doc, VContainer* data)
{
    PatternPieceTool* full_tool = nullptr;
    const QSharedPointer<PieceMirrorDialog> mirror_dialog = dialog.objectCast<PieceMirrorDialog>();
    if (!mirror_dialog.isNull())
    {
        full_tool = Create(mirror_dialog->getPieceId(), mirror_dialog->removeHalfFromLayout(), draft_scene,
                           piece_scene, doc, data);
    }
    return full_tool;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create makes the full piece of a half piece, in one undo step.
 * @param piece_id    half piece with a valid fold line, in any draft block.
 * @param remove_half true to take the half piece out of the layout.
 * @param draft_scene scene of the Mirror by Line operation.
 * @param piece_scene scene of the full piece.
 * @param doc         pattern document.
 * @param data        pattern data.
 * @return the full piece tool; nullptr when the piece cannot be mirrored. Nothing changes in that case.
 */
PatternPieceTool* PieceMirrorTool::Create(quint32 piece_id, bool remove_half, VMainGraphicsScene* draft_scene,
                                          VMainGraphicsScene* piece_scene, VAbstractPattern* doc, VContainer* data)
{
    PatternPieceTool* full_tool = nullptr;

    // The operation and the full piece go into the draft block of the half piece.
    const QString block_name = doc != nullptr ? pieceBlockName(doc, piece_id) : QString();
    const bool valid = draft_scene != nullptr && piece_scene != nullptr && !block_name.isEmpty()
            && mirrorProblem(data, piece_id).isEmpty();
    if (valid)
    {
        const VPiece half = data->GetPiece(piece_id);

        PieceMirrorContext context;
        context.piece_scene = piece_scene;
        context.doc         = doc;
        context.data        = data;
        context.block_name  = block_name;

        // Check every object before the first change, so a refused mirror leaves the pattern as it was.
        QVector<SourceItem> source;
        if (collectSource(context, half, source))
        {
            // The Mirror by Line operation and its undo command take the active draft block. The half piece block is
            // active only while they are made; the active block belongs to Draft mode, so it is restored after.
            const QString active_block = doc->getActiveDraftBlockName();
            doc->changeActiveDraftBlock(block_name);

            qApp->getUndoStack()->beginMacro(tr("mirror piece"));

            if (mirrorObjects(context, source, draft_scene))
            {
                full_tool = createFullPiece(context, half);
            }

            if (full_tool != nullptr && remove_half && half.isInLayout())
            {
                TogglePieceInLayout* command = new TogglePieceInLayout(piece_id, false, data, doc);
                QObject::connect(command, &TogglePieceInLayout::updateList, doc, &VAbstractPattern::updatePieceList);
                qApp->getUndoStack()->push(command);
            }

            qApp->getUndoStack()->endMacro();

            doc->changeActiveDraftBlock(active_block);
        }
    }
    return full_tool;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief mirrorProblem tells why a piece cannot be mirrored.
 * @return a message for the status bar; empty when the piece can be mirrored.
 */
QString PieceMirrorTool::mirrorProblem(const VContainer* data, quint32 piece_id)
{
    QString problem;
    if (data == nullptr || !data->DataPieces()->contains(piece_id))
    {
        problem = tr("Select a pattern piece");
    }
    else
    {
        const VPiece piece = data->GetPiece(piece_id);
        if (piece.IsUnited())
        {
            problem = tr("A united piece can not be mirrored");
        }
        else if (PieceMirrorPath::halfPathIndexes(piece.GetPath().getNodes()).isEmpty())
        {
            problem = tr("The piece has no fold line. Mark two neighbouring points Fold Line in the piece properties");
        }
        else if (qFuzzyIsNull(piece.foldLine(data).length()))
        {
            problem = tr("The fold line has no length");
        }
    }
    return problem;
}
