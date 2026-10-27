//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_corner_cuts.h
//  @author slspencer
//
//  @brief
//  Cut seam allowance corners compared across the sizes of a multisize handoff piece.
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
//  along with Seamly2D.  If not, see <https://www.gnu.org/licenses/gpl-3.0.html>.
//
//  SPDX-License-Identifier: GPL-3.0-or-later
//---------------------------------------------------------------------------------------------------------------------

#ifndef HANDOFF_CORNER_CUTS_H
#define HANDOFF_CORNER_CUTS_H

#include <QVector>

struct SeamAllowanceCorners;

namespace layout_handoff
{
/**
 * @brief CornerCutMatch tells how the cut corners of one piece compare across its sizes.
 */
enum class CornerCutMatch
{
    same,         ///< every size cuts the same corners
    uneven,       ///< the sizes cut different corners and have the same corner count
    count_differs ///< the sizes cut different corners and differ in corner count
};

/**
 * @brief CornerCutCompare is the result of compare_corner_cuts().
 */
struct CornerCutCompare
{
    CornerCutMatch match = CornerCutMatch::same;
    QVector<int>   cuts; ///< every corner cut in any size, ascending, without duplicates
};

/**
 * @brief compare_corner_cuts compares the cut seam allowance corners of one piece across its sizes.
 * @details A ByLength corner is cut when its miter is too long, so a sharp corner can be cut in the small sizes
 *          only. Grading needs one corner shape in every size. Only the uneven result lets every size cut the
 *          corners in @c cuts: with a different corner count, the corner numbers do not name the same corners.
 * @param sizes the corners of the same piece in every size that has it; empty gives CornerCutMatch::same.
 */
CornerCutCompare compare_corner_cuts(const QVector<SeamAllowanceCorners>& sizes);
} // namespace layout_handoff

#endif // HANDOFF_CORNER_CUTS_H
