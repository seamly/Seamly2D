//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_sheet.h
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

#ifndef HANDOFF_SHEET_H
#define HANDOFF_SHEET_H

#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QVector>

namespace layout_handoff
{
class HandoffPiece;
struct HandoffStyle;

/**
 * @brief FlatSheet is the sheet that holds all pieces side by side, starting at the origin.
 */
struct FlatSheet
{
    QRect            sheet;   ///< view box rectangle in pixels
    QVector<QPointF> offsets; ///< place of each piece origin on the sheet, parallel to the input pieces
};

/**
 * @brief piece_extent returns the rectangle that the drawn lines of a piece cover, pens included.
 * @details Covers the seam line, cut line, notches, internal paths, cutout paths, grainline, and fold line with its
 *          arrow. Label text does not count: labels lie inside the piece outline. A piece with nothing to draw gives
 *          a null rectangle.
 * @param piece piece in its own pixel coordinates.
 * @param style line weights and show flags.
 */
QRectF piece_extent(const HandoffPiece &piece, const HandoffStyle &style);

/**
 * @brief arrange_flat places the pieces at their positions and shifts all of them so the sheet starts at the origin.
 * @param extents   piece_extent() of each piece.
 * @param positions place of each piece origin before the shift (the Mx and My offsets of the pieces).
 * @return the sheet rectangle and the shifted positions. The sheet is null when no piece has an extent.
 */
FlatSheet arrange_flat(const QVector<QRectF> &extents, const QVector<QPointF> &positions);
} // namespace layout_handoff

#endif // HANDOFF_SHEET_H
