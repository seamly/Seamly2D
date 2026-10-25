/******************************************************************************
 **  @file   FoldLineGraphic.cpp
 **  @author slspencer
 **
 **  @brief
 **  Arrow line and "Foldline" label that mark the fold line of a piece.
 **
 **  @copyright
 **  This source code is part of the Seamly2D project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
 **
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **  License text: https://www.gnu.org/licenses/gpl-3.0.html
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

#include "FoldLineGraphic.h"

#include <QCoreApplication>
#include <QFontMetricsF>
#include <QtMath>

namespace pattern_piece
{
namespace
{
/// @brief Arrow line length, as a part of the fold line length.
constexpr qreal arrow_length_ratio = 0.5;

/// @brief Length of each bend toward the fold line, as a part of the arrow line length.
constexpr qreal bend_ratio = 0.1;

/// @brief Arrow head length, as a part of the bend length.
constexpr qreal head_length_ratio = 0.25;

/// @brief Half the arrow head width, as a part of the arrow head length.
constexpr qreal head_half_width_ratio = 0.58;

/// @brief Label text height, as a part of the room between the fold line and the arrow line.
constexpr qreal label_height_ratio = 0.5;

/// @brief Largest label text width, as a part of the arrow line part parallel to the fold line.
constexpr qreal label_width_ratio = 0.8;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief signedArea returns twice the signed area of a closed polygon (shoelace formula).
 * @return positive when the polygon turns from the x axis toward the y axis; 0 for fewer than 3 points.
 */
qreal signedArea(const QVector<QPointF>& points)
{
    qreal area = 0;
    for (int i = 0; i < points.size(); ++i)
    {
        const QPointF& a = points.at(i);
        const QPointF& b = points.at((i + 1) % points.size());
        area += a.x() * b.y() - b.x() * a.y();
    }
    return area;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief mapPoints maps every point through a transform.
 */
QVector<QPointF> mapPoints(const QTransform& transform, const QVector<QPointF>& points)
{
    QVector<QPointF> mapped;
    mapped.reserve(points.size());
    for (const QPointF& point : points)
    {
        mapped.append(transform.map(point));
    }
    return mapped;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FoldLineGraphic makes a null graphic.
 */
FoldLineGraphic::FoldLineGraphic()
    : m_fold_line()
    , m_arrow_line()
    , m_start_arrow_head()
    , m_end_arrow_head()
    , m_label_center()
    , m_label_room(0)
    , m_label_length(0)
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FoldLineGraphic places the arrow line and the label of a fold line.
 * @param fold_line fold line, from its start node to its end node in path direction.
 * @param outline   main path points in path direction; the graphic lies on the side of the fold line where the
 *                  outline is. With fewer than 3 points, or a null fold line, the graphic is null.
 */
FoldLineGraphic::FoldLineGraphic(const QLineF& fold_line, const QVector<QPointF>& outline)
    : FoldLineGraphic()
{
    // The outline runs along the fold line in path direction, so its turn direction gives the inside of the piece.
    const qreal area = signedArea(outline);
    const qreal length = fold_line.length();
    if (outline.size() >= 3 && !qFuzzyIsNull(area) && length > 0)
    {
        const QPointF along = (fold_line.p2() - fold_line.p1()) / length;
        const QPointF inside = area > 0 ? QPointF(-along.y(), along.x()) : QPointF(along.y(), -along.x());

        // Arrow line parts: one bend at each end, the straight part between them.
        const qreal total      = length * arrow_length_ratio;
        const qreal bend       = total * bend_ratio;
        const qreal straight   = total - 2 * bend;
        const qreal head       = bend * head_length_ratio;
        const qreal half_width = head * head_half_width_ratio;

        // The arrow tips lie on the fold line, centered between its nodes.
        const QPointF middle     = (fold_line.p1() + fold_line.p2()) / 2;
        const QPointF start_tip  = middle - along * (straight / 2);
        const QPointF end_tip    = middle + along * (straight / 2);
        const QPointF start_base = start_tip + inside * head;
        const QPointF end_base   = end_tip + inside * head;

        m_fold_line = fold_line;
        m_arrow_line = {start_base, start_tip + inside * bend, end_tip + inside * bend, end_base};
        m_start_arrow_head = {start_tip, start_base + along * half_width, start_base - along * half_width};
        m_end_arrow_head = {end_tip, end_base + along * half_width, end_base - along * half_width};
        m_label_center = middle + inside * (bend / 2);
        m_label_room = bend;
        m_label_length = straight;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isNull returns true when the graphic has nothing to draw.
 */
bool FoldLineGraphic::isNull() const
{
    return m_arrow_line.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief foldLine returns the fold line the graphic was made from; a null line for a null graphic.
 */
QLineF FoldLineGraphic::foldLine() const
{
    return m_fold_line;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief arrowLine returns the arrow line: start bend, straight part, end bend.
 * @return four points, from the start arrow head base to the end arrow head base; empty for a null graphic.
 */
QVector<QPointF> FoldLineGraphic::arrowLine() const
{
    return m_arrow_line;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief startArrowHead returns the arrow head on the fold line start side: tip, then the two base corners.
 */
QVector<QPointF> FoldLineGraphic::startArrowHead() const
{
    return m_start_arrow_head;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief endArrowHead returns the arrow head on the fold line end side: tip, then the two base corners.
 */
QVector<QPointF> FoldLineGraphic::endArrowHead() const
{
    return m_end_arrow_head;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief labelCenter returns the center of the label text, halfway between the fold line and the arrow line.
 */
QPointF FoldLineGraphic::labelCenter() const
{
    return m_label_center;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief labelAngle returns the label rotation in degrees, as QTransform::rotate() takes it; see uprightAngle().
 */
qreal FoldLineGraphic::labelAngle() const
{
    return uprightAngle(m_fold_line);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief labelRoom returns the distance between the fold line and the arrow line, across the label.
 */
qreal FoldLineGraphic::labelRoom() const
{
    return m_label_room;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief labelLength returns the length of the arrow line part parallel to the fold line, along the label.
 */
qreal FoldLineGraphic::labelLength() const
{
    return m_label_length;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief mapped returns the graphic moved by @p transform.
 *
 * The label keeps reading upright: its angle follows the mapped fold line, not the transform. A mirroring
 * transform therefore mirrors the label position, not its text.
 */
FoldLineGraphic FoldLineGraphic::mapped(const QTransform& transform) const
{
    FoldLineGraphic result;
    if (!isNull())
    {
        const qreal scale = qSqrt(qAbs(transform.determinant()));
        result.m_fold_line = transform.map(m_fold_line);
        result.m_arrow_line = mapPoints(transform, m_arrow_line);
        result.m_start_arrow_head = mapPoints(transform, m_start_arrow_head);
        result.m_end_arrow_head = mapPoints(transform, m_end_arrow_head);
        result.m_label_center = transform.map(m_label_center);
        result.m_label_room = m_label_room * scale;
        result.m_label_length = m_label_length * scale;
    }
    return result;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief labelFont returns @p base with a pixel size that fits the label between the fold line and the arrow line.
 *
 * The text is half as high as the room. A text wider than 80% of the straight arrow line part gets smaller.
 */
QFont FoldLineGraphic::labelFont(const QFont& base) const
{
    QFont font = base;
    int pixel_size = qMax(1, qFloor(m_label_room * label_height_ratio));
    font.setPixelSize(pixel_size);

    // Shrink a text that is too wide for the straight part.
    const qreal width = QFontMetricsF(font).horizontalAdvance(labelText());
    const qreal widest = m_label_length * label_width_ratio;
    if (width > widest && width > 0)
    {
        pixel_size = qMax(1, qFloor(pixel_size * widest / width));
        font.setPixelSize(pixel_size);
    }
    return font;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief labelTransform places a text box with its top left corner at (0, 0) on the label center, rotated.
 * @param text_size size of the text box.
 */
QTransform FoldLineGraphic::labelTransform(const QSizeF& text_size) const
{
    // Applied to a point last to first: center the box on (0, 0), rotate it, move it to the label center.
    QTransform transform;
    transform.translate(m_label_center.x(), m_label_center.y());
    transform.rotate(labelAngle());
    transform.translate(-text_size.width() / 2, -text_size.height() / 2);
    return transform;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief labelText returns the label text, translated.
 */
QString FoldLineGraphic::labelText()
{
    return QCoreApplication::translate("FoldLineGraphic", "Foldline");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief uprightAngle returns the rotation that lays a text along @p line, never upside down.
 * @return degrees in (-90, 90], as QTransform::rotate() takes them (clockwise on screen). A vertical line gives 90.
 */
qreal FoldLineGraphic::uprightAngle(const QLineF& line)
{
    qreal angle = qRadiansToDegrees(qAtan2(line.dy(), line.dx()));
    if (angle > 90)
    {
        angle -= 180;
    }
    else if (angle <= -90)
    {
        angle += 180;
    }
    return angle;
}
} // namespace pattern_piece
