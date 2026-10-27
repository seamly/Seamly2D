//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_geometry.h
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

#ifndef HANDOFF_GEOMETRY_H
#define HANDOFF_GEOMETRY_H

#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QVector>

namespace layout_handoff
{
/**
 * @brief keep_inside moves the points by the largest overshoot per axis so that every point lies in the rectangle.
 */
QVector<QPointF> keep_inside(const QRectF &rect, QVector<QPointF> points);

/**
 * @brief outline_bounds returns the bounding rectangle of the outline drawn with the default one pixel pen.
 * @param outline closed outline with at least one point.
 */
QRectF outline_bounds(const QVector<QPointF> &outline);

/**
 * @brief stroked_bounds returns the bounding rectangle of a path drawn with a pen, as a QGraphicsPathItem has it.
 * @details A pen with no style or zero width gives the control point rectangle of the path.
 */
QRectF stroked_bounds(const QPainterPath &path, const QPen &pen);
} // namespace layout_handoff

#endif // HANDOFF_GEOMETRY_H
