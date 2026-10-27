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

#include "handoff_geometry.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/def.h"
#include "../vmisc/vmath.h"
#include "../vpatterndb/AbstractPiece.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecepath.h"

#include <QPainterPath>
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

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief find_grainline_geometry evaluates the grainline start, direction and lengths.
 * @param data        grainline data of the piece.
 * @param pattern     pattern data; must not be null.
 * @param length      receives the grainline length in pixels.
 * @param angle       receives the direction in radians, counter-clockwise on screen.
 * @param arrow_length receives the arrow head length in pixels.
 * @param start       receives the grainline start point.
 * @return false when a formula or an anchor point is invalid.
 */
bool find_grainline_geometry(const VGrainlineData &data, const VContainer *pattern, qreal &length, qreal &angle,
                             qreal &arrow_length, QPointF &start)
{
    const quint32 top_id    = data.topAnchorPoint();
    const quint32 bottom_id = data.bottomAnchorPoint();

    if (top_id != NULL_ID && bottom_id != NULL_ID)
    {
        try
        {
            const auto top    = pattern->GeometricObject<VPointF>(top_id);
            const auto bottom = pattern->GeometricObject<VPointF>(bottom_id);

            const QLineF line(static_cast<QPointF>(*bottom), static_cast<QPointF>(*top));
            length = line.length();
            angle  = qDegreesToRadians(line.angle());

            Calculator calculator;
            arrow_length = ToPixel(calculator.EvalFormula(pattern->DataVariables(), data.getArrowLength()),
                                   *pattern->GetPatternUnit());
            start = line.p1();
            return true;
        }
        catch (const VExceptionBadId &)
        {
            // Fall through to the formula data.
        }
    }

    try
    {
        Calculator rotation_calculator;
        angle = qDegreesToRadians(rotation_calculator.EvalFormula(pattern->DataVariables(), data.getRotation()));

        Calculator length_calculator;
        length = ToPixel(length_calculator.EvalFormula(pattern->DataVariables(), data.getLength()),
                         *pattern->GetPatternUnit());

        Calculator arrow_calculator;
        arrow_length = ToPixel(arrow_calculator.EvalFormula(pattern->DataVariables(), data.getArrowLength()),
                               *pattern->GetPatternUnit());
    }
    catch (const qmu::QmuParserError &)
    {
        return false;
    }

    start = data.GetPos();
    const quint32 center_id = data.centerAnchorPoint();
    if (center_id != NULL_ID)
    {
        try
        {
            const auto center = pattern->GeometricObject<VPointF>(center_id);
            QLineF line(center->x(), center->y(), center->x() + length / 2.0, center->y());
            line.setAngle(qRadiansToDegrees(angle));
            line = QLineF(line.p2(), line.p1());
            line.setLength(length);
            start = line.p2();
        }
        catch (const VExceptionBadId &)
        {
            start = data.GetPos();
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_grainline_points builds the grainline path: both ends and the arrow heads that the arrow type asks for.
 */
QVector<QPointF> make_grainline_points(const VGrainlineData &data, qreal length, qreal angle, qreal arrow_length,
                                       const QPointF &start)
{
    const QPointF end(start.x() + length * qCos(angle), start.y() - length * qSin(angle));
    const qreal   spread = M_PI / 9;

    QVector<QPointF> path;
    path << start;
    if (data.getArrowType() != ArrowType::Top)
    {
        path << QPointF(start.x() + arrow_length * cos(angle + spread), start.y() - arrow_length * sin(angle + spread));
        path << QPointF(start.x() + arrow_length * cos(angle - spread), start.y() - arrow_length * sin(angle - spread));
    }
    path << start << end;
    if (data.getArrowType() != ArrowType::Bottom)
    {
        path << QPointF(end.x() + arrow_length * cos(M_PI + angle + spread),
                        end.y() - arrow_length * sin(M_PI + angle + spread));
        path << QPointF(end.x() + arrow_length * cos(M_PI + angle - spread),
                        end.y() - arrow_length * sin(M_PI + angle - spread));
    }
    path << end;
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief read_grainline fills the grainline path and the grain axis of a piece.
 * @details Without top and bottom anchors the axis points up, so SeamlyLayout sees one direction per axis.
 *          The axis does not depend on the length, so a zero or invalid length does not hide it.
 */
void read_grainline(const VGrainlineData &data, const VContainer *pattern, const QVector<QPointF> &outline,
                    QVector<QPointF> *points, QVector<QPointF> *axis)
{
    qreal   length       = 0;
    qreal   angle        = 0;
    qreal   arrow_length = 0;
    QPointF start;
    if (!find_grainline_geometry(data, pattern, length, angle, arrow_length, start))
    {
        return;
    }

    *points = make_grainline_points(data, length, angle, arrow_length, start);
    if (!outline.isEmpty())
    {
        *points = keep_inside(outline_bounds(outline), *points);
    }

    qreal axis_angle = angle;
    if (data.topAnchorPoint() == NULL_ID || data.bottomAnchorPoint() == NULL_ID)
    {
        axis_angle = qDegreesToRadians(VGrainlineData::upwardAngle(qRadiansToDegrees(angle)));
    }
    *axis << start << QPointF(start.x() + qCos(axis_angle), start.y() - qSin(axis_angle));
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

    const QLineF fold_line = piece.foldLine(pattern);
    handoff.m_is_on_fold   = piece.GetPatternPieceData().IsOnFold() || !fold_line.isNull();
    if (!fold_line.isNull())
    {
        handoff.m_fold_line = fold_line;
        if (!cut_points.isEmpty())
        {
            handoff.m_fold_line_on_cut_line = VPiece::foldLineOnCutLine(cut_points, fold_line);
        }
        handoff.m_fold_line_graphic = pattern_piece::FoldLineGraphic(fold_line, piece.mainPathPoints(pattern));
    }

    const VGrainlineData &grainline = piece.GetGrainlineGeometry();
    handoff.m_is_grainline_visible  = grainline.IsVisible();
    read_grainline(grainline, pattern, handoff.m_seam_line.points, &handoff.m_grainline_points,
                   &handoff.m_grain_axis);
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
QRectF HandoffPiece::bounds() const
{
    return m_seam_line.points.isEmpty() ? QRectF() : outline_bounds(m_seam_line.points);
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

//---------------------------------------------------------------------------------------------------------------------
const QVector<QPointF> &HandoffPiece::grainline_points() const
{
    return m_grainline_points;
}

//---------------------------------------------------------------------------------------------------------------------
const QVector<QPointF> &HandoffPiece::grain_axis() const
{
    return m_grain_axis;
}

//---------------------------------------------------------------------------------------------------------------------
bool HandoffPiece::is_grainline_visible() const
{
    return m_is_grainline_visible;
}

//---------------------------------------------------------------------------------------------------------------------
bool HandoffPiece::is_on_fold() const
{
    return m_is_on_fold;
}

//---------------------------------------------------------------------------------------------------------------------
const QLineF &HandoffPiece::fold_line() const
{
    return m_fold_line;
}

//---------------------------------------------------------------------------------------------------------------------
const QLineF &HandoffPiece::fold_line_on_cut_line() const
{
    return m_fold_line_on_cut_line;
}

//---------------------------------------------------------------------------------------------------------------------
const pattern_piece::FoldLineGraphic &HandoffPiece::fold_line_graphic() const
{
    return m_fold_line_graphic;
}
} // namespace layout_handoff
