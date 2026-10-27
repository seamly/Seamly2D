//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_piece.cpp
//  @author slspencer
//
//  @brief
//  Geometry of one piece in the SeamlyLayout handoff.
//
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2026 Seamly2D Project
//  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
//
//  Seamly2D is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Seamly2D is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

#include "handoff_piece.h"

#include "../vpatterndb/AbstractPiece.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vmisc/def.h"

#include <QtDebug>

namespace layout_handoff
{
namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief is_closed tells whether a polyline ends where it starts.
 */
bool is_closed(const QVector<QPointF> &points)
{
    return points.size() > 2 && QLineF(points.first(), points.last()).length() < accuracyPointOnLine;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_line builds a line and finds its turn points and node indices.
 * @param points        the line after AbstractPiece::RemoveDublicates().
 * @param node_vertices vertices that can break the tangent. Empty = turn points unknown.
 */
HandoffLine make_line(const QVector<QPointF> &points, const QVector<QPointF> &node_vertices, bool closed)
{
    HandoffLine line;
    line.closed = closed;
    line.points = points;
    if (!line.points.isEmpty() && !node_vertices.isEmpty())
    {
        line.points       = AbstractPiece::snapNodeVertices(line.points, node_vertices, closed);
        line.turn_points  = AbstractPiece::turnPointIndices(line.points, node_vertices, closed);
        line.node_indices = AbstractPiece::nodeVertexIndices(line.points, node_vertices, closed);
    }
    return line;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_paths reads the internal paths of a piece.
 * @param cut true for the cutout paths, false for the other internal paths.
 */
QVector<HandoffPath> make_paths(const VPiece &piece, const VContainer *pattern, bool cut)
{
    QVector<HandoffPath> paths;
    const QVector<quint32> path_ids = piece.getInternalPaths();
    const QVector<QPointF> cut_path = piece.cutPathPoints(pattern);

    for (const quint32 path_id : path_ids)
    {
        const VPiecePath path = pattern->getPiecePath(path_id);
        if (path.getType() == PiecePathType::InternalPath && path.isCutPath() == cut)
        {
            const QVector<QPointF> points = path.PathPoints(pattern, cut_path);
            HandoffPath handoff_path;
            handoff_path.line        = make_line(points, path.pathNodeVertices(pattern), is_closed(points));
            handoff_path.color       = path.getLineColor();
            handoff_path.line_type   = path.getLineType();
            handoff_path.line_weight = path.getLineWeight();
            handoff_path.cut         = path.isCutPath();
            paths.append(handoff_path);
        }
    }
    return paths;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_notches keeps the per-notch lists only when they agree with the notch lines.
 * @details The line counts must be positive and sum to the line count. The widths and types need one entry per notch.
 */
HandoffNotches make_notches(const QVector<QLineF> &lines, const QVector<int> &line_counts,
                            const QVector<qreal> &widths, const QStringList &types)
{
    HandoffNotches notches;
    notches.lines = lines;

    int  line_total = 0;
    bool counts_ok  = !line_counts.isEmpty();
    for (const int count : line_counts)
    {
        counts_ok = counts_ok && count > 0;
        line_total += count;
    }
    if (counts_ok && line_total == lines.size())
    {
        notches.line_counts = line_counts;
        if (widths.size() == line_counts.size())
        {
            notches.seam_allowance_widths = widths;
        }
        if (types.size() == line_counts.size())
        {
            notches.types = types;
        }
    }
    return notches;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
HandoffPiece HandoffPiece::create(const VPiece &piece, const VContainer *pattern)
{
    HandoffPiece handoff;

    handoff.m_is_seam_line_hidden = piece.isHideSeamLine();
    handoff.m_seam_line = make_line(AbstractPiece::RemoveDublicates(piece.mainPathPoints(pattern), false),
                                    piece.mainPathNodeVertices(pattern), true);

    QVector<QPointF>       cut_line_nodes;
    SeamAllowanceCorners   corners;
    const QVector<QPointF> cut_points = piece.seamAllowancePoints(pattern, &cut_line_nodes, &corners);
    if (piece.hasSeamAllowance())
    {
        handoff.m_has_seam_allowance          = true;
        handoff.m_has_built_in_seam_allowance = piece.hasSeamAllowanceBuiltIn();
        if (!cut_points.isEmpty())
        {
            handoff.m_cut_line = make_line(AbstractPiece::RemoveDublicates(cut_points, false), cut_line_nodes, true);
        }
        else if (!handoff.m_has_built_in_seam_allowance)
        {
            qWarning() << "Seam allowance is empty.";
            handoff.m_has_seam_allowance = false;
        }
    }

    QVector<QPointF>       built_in_nodes;
    const QVector<QPointF> built_in = piece.builtInSeamLinePoints(pattern, &built_in_nodes);
    // A fold line cuts the seam line open: it ends elsewhere than it starts.
    const bool built_in_closed = built_in.size() < 2 || built_in.first() == built_in.last();
    handoff.m_built_in_seam_line = make_line(AbstractPiece::RemoveDublicates(built_in, built_in_closed),
                                             built_in_nodes, built_in_closed);

    handoff.m_internal_paths = make_paths(piece, pattern, false);
    handoff.m_cutout_paths   = make_paths(piece, pattern, true);

    if (handoff.m_has_seam_allowance)
    {
        QVector<qreal> widths;
        QStringList    types;
        QVector<int>   line_counts;
        // Clip the cut line notches to the cut line, as the canvas does.
        const QVector<QLineF> lines = piece.createNotchLines(pattern, cut_points, true, &widths, &types, &line_counts);
        handoff.m_notches = make_notches(lines, line_counts, widths, types);
    }
    return handoff;
}

//---------------------------------------------------------------------------------------------------------------------
bool HandoffPiece::has_shape() const
{
    return !m_seam_line.points.isEmpty() || !m_cut_line.points.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
bool HandoffPiece::has_seam_allowance() const
{
    return m_has_seam_allowance;
}

//---------------------------------------------------------------------------------------------------------------------
bool HandoffPiece::has_built_in_seam_allowance() const
{
    return m_has_built_in_seam_allowance;
}

//---------------------------------------------------------------------------------------------------------------------
bool HandoffPiece::is_seam_line_hidden() const
{
    return m_is_seam_line_hidden;
}

//---------------------------------------------------------------------------------------------------------------------
const HandoffLine &HandoffPiece::seam_line() const
{
    return m_seam_line;
}

//---------------------------------------------------------------------------------------------------------------------
const HandoffLine &HandoffPiece::cut_line() const
{
    return m_cut_line;
}

//---------------------------------------------------------------------------------------------------------------------
const HandoffLine &HandoffPiece::built_in_seam_line() const
{
    return m_built_in_seam_line;
}

//---------------------------------------------------------------------------------------------------------------------
const HandoffNotches &HandoffPiece::notches() const
{
    return m_notches;
}

//---------------------------------------------------------------------------------------------------------------------
const QVector<HandoffPath> &HandoffPiece::internal_paths() const
{
    return m_internal_paths;
}

//---------------------------------------------------------------------------------------------------------------------
const QVector<HandoffPath> &HandoffPiece::cutout_paths() const
{
    return m_cutout_paths;
}
} // namespace layout_handoff
