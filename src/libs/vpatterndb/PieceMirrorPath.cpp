/******************************************************************************
 **  @file   PieceMirrorPath.cpp
 **  @author slspencer
 **
 **  @brief
 **  Node order and node attributes of a full piece made from a half piece and
 **  its fold line.
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

#include "PieceMirrorPath.h"

#include <QHash>

#include "vpiece.h"
#include "vpiecepath.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief halfPositions maps a half piece node index to its position along the half path.
 */
QHash<int, int> halfPositions(const QVector<int>& half)
{
    QHash<int, int> positions;
    for (int k = 0; k < half.size(); ++k)
    {
        positions.insert(half.at(k), k);
    }
    return positions;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief allRecordRanges lists both copies of each record that stays on one side of the fold line.
 * @details A range can still run past the last full path node; fullPathOrder() rotates the path to prevent that.
 */
QVector<PieceMirrorRange> allRecordRanges(const QVector<VPieceNode>& nodes, const QVector<PieceMirrorNode>& order,
                                          const QVector<CustomSARecord>& records)
{
    const QHash<int, int> half_position = halfPositions(PieceMirrorPath::halfPathIndexes(nodes));

    QVector<PieceMirrorRange> ranges;
    for (int i = 0; i < records.size(); ++i)
    {
        const int start_index = VPiecePath::indexOfNode(nodes, records.at(i).startPoint);
        const int end_index   = VPiecePath::indexOfNode(nodes, records.at(i).endPoint);

        // A record that crosses the fold line, or that starts or ends on a left out node, gets no range.
        const bool on_half = half_position.contains(start_index) && half_position.contains(end_index)
                && half_position.value(start_index) < half_position.value(end_index);
        if (on_half)
        {
            PieceMirrorRange copy;
            copy.record_index   = i;
            copy.mirrored       = false;
            copy.start_position = PieceMirrorPath::position(order, start_index, false);
            copy.end_position   = PieceMirrorPath::position(order, end_index, false);
            ranges.append(copy);

            // The mirrored half runs the other way, so the mirrored record end comes first.
            PieceMirrorRange mirror;
            mirror.record_index   = i;
            mirror.mirrored       = true;
            mirror.start_position = PieceMirrorPath::position(order, end_index, true);
            mirror.end_position   = PieceMirrorPath::position(order, start_index, true);
            ranges.append(mirror);
        }
    }
    return ranges;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief rotationStart finds a first node for the full path, so that no record range runs past the last node.
 * @param count  full path node count.
 * @param ranges record ranges as positions in the unrotated full path.
 * @return the position of that first node in the unrotated path; 0 when no position suits every range.
 */
int rotationStart(int count, const QVector<PieceMirrorRange>& ranges)
{
    int start = 0;
    bool found = false;
    for (int candidate = 0; candidate < count && !found; ++candidate)
    {
        bool fits = true;
        for (const PieceMirrorRange& range : ranges)
        {
            // A first node after the range start, up to and including its end, splits the range.
            const int length = (range.end_position - range.start_position + count) % count;
            const int offset = (candidate - range.start_position + count) % count;
            if (offset >= 1 && offset <= length)
            {
                fits = false;
            }
        }

        if (fits)
        {
            start = candidate;
            found = true;
        }
    }
    return start;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief halfPathIndexes lists the half piece nodes that the full path copies, from the fold line end to its start.
 * @param nodes half piece main path nodes.
 * @return node indexes in path direction, excluded nodes left out; empty when the path has no valid fold line, or
 *         when only the fold line nodes remain.
 */
QVector<int> PieceMirrorPath::halfPathIndexes(const QVector<VPieceNode>& nodes)
{
    QVector<int> half;
    const QVector<int> fold = VPiece::foldLineNodeIndexes(nodes);
    if (fold.size() == 2)
    {
        // Walk on from the fold line end. The walk stops at the fold line start, so it never crosses the fold.
        int index = fold.at(1);
        bool done = false;
        while (!done)
        {
            if (!nodes.at(index).isExcluded())
            {
                half.append(index);
            }
            done  = (index == fold.at(0));
            index = (index + 1) % nodes.size();
        }
    }

    // The two fold line nodes alone enclose no area.
    if (half.size() < 3)
    {
        half.clear();
    }
    return half;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief fullPathOrder lists the main path nodes of the full piece.
 * @details The half path comes first, from the fold line end to the fold line start. The mirrored half follows,
 * back toward the fold line end. The path then starts at a node where no custom seam allowance record runs past
 * the last node, because VPiece accepts a record only when its start node comes before its end node.
 * @param nodes   half piece main path nodes.
 * @param records half piece custom seam allowance records.
 * @return full path nodes in path order; empty when the half piece cannot be mirrored.
 */
QVector<PieceMirrorNode> PieceMirrorPath::fullPathOrder(const QVector<VPieceNode>& nodes,
                                                        const QVector<CustomSARecord>& records)
{
    const QVector<int> half = halfPathIndexes(nodes);

    QVector<PieceMirrorNode> order;
    for (int index : half)
    {
        order.append(PieceMirrorNode{index, false});
    }

    // The fold line nodes are on the mirror axis, so the mirrored half leaves out the first and last half node.
    for (int k = half.size() - 2; k >= 1; --k)
    {
        order.append(PieceMirrorNode{half.at(k), true});
    }

    const int start = order.isEmpty() ? 0 : rotationStart(order.size(), allRecordRanges(nodes, order, records));

    QVector<PieceMirrorNode> rotated;
    for (int k = 0; k < order.size(); ++k)
    {
        rotated.append(order.at((k + start) % order.size()));
    }
    return rotated;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief recordRanges finds the custom seam allowance records of the full piece.
 * @details Each record that stays on one side of the fold line gets two ranges: one on the half, one on the
 * mirrored half. A record that crosses the fold line, or a range that still runs past the last node, is left out.
 * @param nodes   half piece main path nodes.
 * @param order   full path nodes, from fullPathOrder().
 * @param records half piece custom seam allowance records.
 * @return the ranges, each with its start position before its end position.
 */
QVector<PieceMirrorRange> PieceMirrorPath::recordRanges(const QVector<VPieceNode>& nodes,
                                                        const QVector<PieceMirrorNode>& order,
                                                        const QVector<CustomSARecord>& records)
{
    QVector<PieceMirrorRange> ranges;
    for (const PieceMirrorRange& range : allRecordRanges(nodes, order, records))
    {
        if (range.start_position >= 0 && range.start_position < range.end_position)
        {
            ranges.append(range);
        }
    }
    return ranges;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief position finds a half piece node in the full path.
 * @param order        full path nodes.
 * @param source_index half piece node index.
 * @param mirrored     true for the copy across the fold line. A fold line node has no such copy, so the node itself
 *                     is returned.
 * @return the full path position; -1 when the full path does not hold the node.
 */
int PieceMirrorPath::position(const QVector<PieceMirrorNode>& order, int source_index, bool mirrored)
{
    int found = -1;
    for (int k = 0; k < order.size() && found < 0; ++k)
    {
        if (order.at(k).source_index == source_index && order.at(k).mirrored == mirrored)
        {
            found = k;
        }
    }

    if (found < 0 && mirrored)
    {
        found = position(order, source_index, false);
    }
    return found;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief fullPathNode builds one main path node of the full piece.
 * @param nodes half piece main path nodes.
 * @param entry the full path node, from fullPathOrder().
 * @param id    object id for the node: the half piece object, or its mirrored copy.
 * @return the node. No node of the full piece is a fold line node.
 */
VPieceNode PieceMirrorPath::fullPathNode(const QVector<VPieceNode>& nodes, const PieceMirrorNode& entry, quint32 id)
{
    VPieceNode node = nodes.at(entry.source_index);
    if (entry.mirrored)
    {
        node = mirroredNode(node, id);
    }
    else
    {
        // A fold line node joins the half to its mirror: the seam allowance on the fold side takes the half side
        // width. The fold line start has the half before it, the fold line end has the half after it.
        const QVector<int> fold = VPiece::foldLineNodeIndexes(nodes);
        if (fold.size() == 2 && entry.source_index == fold.at(0))
        {
            node.setAfterSAFormula(node.GetFormulaSABefore());
        }
        if (fold.size() == 2 && entry.source_index == fold.at(1))
        {
            node.setBeforeSAFormula(node.GetFormulaSAAfter());
        }
        node.SetId(id);
        node.setFoldLine(false);
    }
    return node;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief mirroredNode copies a half piece node to the mirrored half.
 * @details The mirrored half runs the other way. So a curve is read backward, the seam allowance before and after
 * the node change places, and so do the first and second edge corner rules. A mirror also turns the notch angle
 * the other way.
 * @param node half piece node.
 * @param id   object id of the mirrored copy.
 * @return the mirrored node.
 */
VPieceNode PieceMirrorPath::mirroredNode(const VPieceNode& node, quint32 id)
{
    VPieceNode mirror = node;
    mirror.SetId(id);
    mirror.setFoldLine(false);
    mirror.SetReverse(!node.GetReverse());
    mirror.setBeforeSAFormula(node.GetFormulaSAAfter());
    mirror.setAfterSAFormula(node.GetFormulaSABefore());

    switch (node.GetAngleType())
    {
        case PieceNodeAngle::ByFirstEdgeSymmetry:
            mirror.SetAngleType(PieceNodeAngle::BySecondEdgeSymmetry);
            break;
        case PieceNodeAngle::BySecondEdgeSymmetry:
            mirror.SetAngleType(PieceNodeAngle::ByFirstEdgeSymmetry);
            break;
        case PieceNodeAngle::ByFirstEdgeRightAngle:
            mirror.SetAngleType(PieceNodeAngle::BySecondEdgeRightAngle);
            break;
        case PieceNodeAngle::BySecondEdgeRightAngle:
            mirror.SetAngleType(PieceNodeAngle::ByFirstEdgeRightAngle);
            break;
        case PieceNodeAngle::ByLength:
        case PieceNodeAngle::ByPointsIntersection:
        default:
            break;
    }

    if (!qFuzzyIsNull(node.getNotchAngle()))
    {
        mirror.setNotchAngle(-node.getNotchAngle());
    }
    return mirror;
}
