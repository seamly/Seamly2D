//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_geometry.cpp
//  @author slspencer
//
//  @brief
//  Geometry helpers shared by the handoff grainline and labels.
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

#include "handoff_geometry.h"

#include <QPainterPath>
#include <QPainterPathStroker>

namespace layout_handoff
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief keep_inside moves the points by the largest overshoot per axis so that every point lies in the rectangle.
 */
QVector<QPointF> keep_inside(const QRectF &rect, QVector<QPointF> points)
{
    qreal move_x = 0;
    qreal move_y = 0;
    for (const QPointF &point : points)
    {
        if (rect.contains(point))
        {
            continue;
        }
        qreal dx = 0;
        qreal dy = 0;
        if (point.x() < rect.left())
        {
            dx = rect.left() - point.x();
        }
        else if (point.x() > rect.right())
        {
            dx = rect.right() - point.x();
        }
        if (point.y() < rect.top())
        {
            dy = rect.top() - point.y();
        }
        else if (point.y() > rect.bottom())
        {
            dy = rect.bottom() - point.y();
        }
        move_x = qAbs(dx) > qAbs(move_x) ? dx : move_x;
        move_y = qAbs(dy) > qAbs(move_y) ? dy : move_y;
    }
    for (QPointF &point : points)
    {
        point += QPointF(move_x, move_y);
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief outline_bounds returns the bounding rectangle of the outline drawn with the default one pixel pen.
 */
QRectF outline_bounds(const QVector<QPointF> &outline)
{
    QPainterPath path;
    path.moveTo(outline.first());
    for (int i = 1; i < outline.size(); ++i)
    {
        path.lineTo(outline.at(i));
    }
    path.lineTo(outline.first());

    QPainterPathStroker stroker;
    stroker.setWidth(1);
    stroker.setCapStyle(Qt::SquareCap);
    stroker.setJoinStyle(Qt::BevelJoin);
    stroker.setMiterLimit(2);
    QPainterPath shape = stroker.createStroke(path);
    shape.addPath(path);
    return shape.controlPointRect();
}

//---------------------------------------------------------------------------------------------------------------------
QRectF stroked_bounds(const QPainterPath &path, const QPen &pen)
{
    if (pen.style() == Qt::NoPen || pen.widthF() == 0.0)
    {
        return path.controlPointRect();
    }

    QPainterPathStroker stroker;
    stroker.setCapStyle(pen.capStyle());
    stroker.setWidth(pen.widthF());
    stroker.setJoinStyle(pen.joinStyle());
    stroker.setMiterLimit(pen.miterLimit());
    QPainterPath shape = stroker.createStroke(path);
    shape.addPath(path);
    return shape.controlPointRect();
}
} // namespace layout_handoff
