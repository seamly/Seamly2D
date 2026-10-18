/******************************************************************************
 **  @file   PieceMirrorPath.h
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

#ifndef PIECEMIRRORPATH_H
#define PIECEMIRRORPATH_H

#include <QVector>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "vpiecenode.h"

/**
 * @brief PieceMirrorNode is one main path node of a full piece.
 */
struct PieceMirrorNode
{
    int  source_index{-1}; ///< Index of the copied node in the half piece main path.
    bool mirrored{false};  ///< True for the copy across the fold line.
};

Q_DECLARE_TYPEINFO(PieceMirrorNode, Q_MOVABLE_TYPE);

/**
 * @brief PieceMirrorRange is one custom seam allowance record of a full piece, as main path positions.
 */
struct PieceMirrorRange
{
    int  record_index{-1};   ///< Index of the copied record in the half piece.
    bool mirrored{false};    ///< True for the copy across the fold line.
    int  start_position{-1}; ///< Full path position of the record start node.
    int  end_position{-1};   ///< Full path position of the record end node.
};

Q_DECLARE_TYPEINFO(PieceMirrorRange, Q_MOVABLE_TYPE);

/**
 * @brief PieceMirrorPath builds the main path of a full piece from a half piece with a fold line.
 *
 * The full path runs along the half piece from the fold line end to the fold line start, then back along the
 * mirrored half. The two fold line nodes lie on the mirror axis, so the full path holds them once. Excluded nodes
 * are left out.
 */
class PieceMirrorPath
{
public:
    static QVector<int>              halfPathIndexes(const QVector<VPieceNode>& nodes);
    static QVector<PieceMirrorNode>  fullPathOrder(const QVector<VPieceNode>& nodes,
                                                   const QVector<CustomSARecord>& records = QVector<CustomSARecord>());
    static QVector<PieceMirrorRange> recordRanges(const QVector<VPieceNode>& nodes,
                                                  const QVector<PieceMirrorNode>& order,
                                                  const QVector<CustomSARecord>& records);
    static int                       position(const QVector<PieceMirrorNode>& order, int source_index, bool mirrored);
    static VPieceNode                fullPathNode(const QVector<VPieceNode>& nodes, const PieceMirrorNode& entry,
                                                  quint32 id);
    static VPieceNode                mirroredNode(const VPieceNode& node, quint32 id);
};

#endif // PIECEMIRRORPATH_H
