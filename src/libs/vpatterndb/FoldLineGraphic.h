/******************************************************************************
 **  @file   FoldLineGraphic.h
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

#ifndef FOLDLINEGRAPHIC_H
#define FOLDLINEGRAPHIC_H

#include <QFont>
#include <QLineF>
#include <QPointF>
#include <QSizeF>
#include <QString>
#include <QTransform>
#include <QVector>

/// @brief Drawings that mark a pattern piece and are computed from its data.
namespace pattern_piece
{
/**
 * @brief FoldLineGraphic places the arrow line and the "Foldline" label of a fold line.
 *
 * The arrow line lies inside the piece, parallel to the fold line and centered on it. It is half as long as the
 * fold line. Its first and last tenth bend 90 degrees toward the fold line, and an arrow head on each bend touches
 * the fold line. The label reads along the fold line, centered between the fold line and the arrow line.
 * All geometry is in the coordinates of the fold line it was made from.
 */
class FoldLineGraphic
{
public:
                     FoldLineGraphic();
                     FoldLineGraphic(const QLineF& fold_line, const QVector<QPointF>& outline);

    bool             isNull() const;
    QLineF           foldLine() const;
    QVector<QPointF> arrowLine() const;
    QVector<QPointF> startArrowHead() const;
    QVector<QPointF> endArrowHead() const;
    QPointF          labelCenter() const;
    qreal            labelAngle() const;
    qreal            labelRoom() const;
    qreal            labelLength() const;

    FoldLineGraphic  mapped(const QTransform& transform) const;
    QFont            labelFont(const QFont& base) const;
    QTransform       labelTransform(const QSizeF& text_size) const;

    static QString   labelText();
    static qreal     uprightAngle(const QLineF& line);

private:
    QLineF           m_fold_line;        ///< Fold line, start node to end node.
    QVector<QPointF> m_arrow_line;       ///< Arrow line from the start arrow head base to the end arrow head base.
    QVector<QPointF> m_start_arrow_head; ///< Arrow head triangle at the fold line start side, tip first.
    QVector<QPointF> m_end_arrow_head;   ///< Arrow head triangle at the fold line end side, tip first.
    QPointF          m_label_center;     ///< Center of the label text.
    qreal            m_label_room;       ///< Distance between the fold line and the arrow line.
    qreal            m_label_length;     ///< Length of the arrow line part parallel to the fold line.
};
} // namespace pattern_piece

#endif // FOLDLINEGRAPHIC_H
