//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_sheet.cpp
//  @author slspencer
//
//  @brief
//  Scene-free bounds of the pieces and of the flat sheet that holds them.
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

#include "handoff_sheet.h"

#include "handoff_document.h"
#include "handoff_geometry.h"
#include "handoff_piece.h"
#include "../vmisc/def.h"

#include <QPainterPath>
#include <QPen>

namespace layout_handoff
{
namespace
{
//---------------------------------------------------------------------------------------------------------------------
QPainterPath polyline_path(const QVector<QPointF> &points, bool closed)
{
    QPainterPath path;
    if (points.isEmpty())
    {
        return path;
    }
    path.addPolygon(QPolygonF(points));
    if (closed)
    {
        path.lineTo(points.first());
    }
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
QPen make_pen(qreal weight, Qt::PenCapStyle cap, Qt::PenJoinStyle join)
{
    return QPen(Qt::black, weight, Qt::SolidLine, cap, join);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief add_extent adds the pen bounds of a path to the extent; an empty path adds nothing.
 */
void add_extent(QRectF *extent, const QPainterPath &path, const QPen &pen)
{
    if (path.elementCount() == 0)
    {
        return;
    }
    const QRectF bounds = stroked_bounds(path, pen);
    if (!bounds.isNull())
    {
        *extent = extent->isNull() ? bounds : extent->united(bounds);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
QRectF piece_extent(const HandoffPiece &piece, const HandoffStyle &style)
{
    QRectF extent;
    const bool built_in = piece.has_seam_allowance() && piece.has_built_in_seam_allowance();

    const HandoffLineStyle &seam_style = piece.has_seam_allowance() ? style.seam : style.cut;
    const QPen seam_pen = make_pen(seam_style.weight, Qt::RoundCap, Qt::RoundJoin);
    const QPen cut_pen  = make_pen(style.cut.weight, Qt::RoundCap, Qt::RoundJoin);
    if (built_in)
    {
        if (!piece.is_seam_line_hidden())
        {
            const HandoffLine &line = piece.built_in_seam_line();
            add_extent(&extent, polyline_path(line.points, line.closed), seam_pen);
        }
        add_extent(&extent, polyline_path(piece.seam_line().points, true), cut_pen);
    }
    else
    {
        if (!piece.is_seam_line_hidden() || !piece.has_seam_allowance())
        {
            add_extent(&extent, polyline_path(piece.seam_line().points, true), seam_pen);
        }
        if (style.show_seam_allowances && piece.has_seam_allowance())
        {
            add_extent(&extent, polyline_path(piece.cut_line().points, true), cut_pen);
        }
    }

    QPainterPath notches;
    for (const QLineF &notch : piece.notches().lines)
    {
        notches.moveTo(notch.p1());
        notches.lineTo(notch.p2());
    }
    add_extent(&extent, notches, cut_pen);

    const auto add_paths = [&extent](const QVector<HandoffPath> &paths)
    {
        for (const HandoffPath &path : paths)
        {
            add_extent(&extent, polyline_path(path.line.points, false),
                       make_pen(ToPixel(path.line_weight.toDouble(), Unit::Mm), Qt::RoundCap, Qt::RoundJoin));
        }
    };
    add_paths(piece.internal_paths());
    add_paths(piece.cutout_paths());

    if (piece.is_grainline_visible() && piece.grainline_points().size() >= 2)
    {
        add_extent(&extent, polyline_path(piece.grainline_points(), false),
                   make_pen(1, Qt::SquareCap, Qt::BevelJoin));
    }

    QLineF fold_line = piece.fold_line();
    if (style.show_seam_allowances && !piece.fold_line_on_cut_line().isNull())
    {
        fold_line = piece.fold_line_on_cut_line();
    }
    if (!fold_line.isNull())
    {
        add_extent(&extent, polyline_path({fold_line.p1(), fold_line.p2()}, false),
                   make_pen(style.fold_line.weight, Qt::FlatCap, Qt::RoundJoin));
        const pattern_piece::FoldLineGraphic &graphic = piece.fold_line_graphic();
        if (!graphic.isNull())
        {
            add_extent(&extent, polyline_path(graphic.arrowLine(), false),
                       make_pen(style.fold_line.weight, Qt::FlatCap, Qt::MiterJoin));
            QPainterPath heads = polyline_path(graphic.startArrowHead(), true);
            heads.addPath(polyline_path(graphic.endArrowHead(), true));
            add_extent(&extent, heads, Qt::NoPen);
        }
    }
    return extent;
}

//---------------------------------------------------------------------------------------------------------------------
FlatSheet arrange_flat(const QVector<QRectF> &extents, const QVector<QPointF> &positions)
{
    FlatSheet result;
    QRectF    bounds;
    for (int i = 0; i < extents.size(); ++i)
    {
        if (extents.at(i).isNull())
        {
            continue;
        }
        const QRectF placed = extents.at(i).translated(positions.at(i));
        bounds              = bounds.isNull() ? placed : bounds.united(placed);
    }

    const QPoint origin = bounds.toRect().topLeft();
    for (const QPointF &position : positions)
    {
        result.offsets.append(position - origin);
    }
    result.sheet = bounds.translated(-origin).toRect();
    return result;
}
} // namespace layout_handoff
