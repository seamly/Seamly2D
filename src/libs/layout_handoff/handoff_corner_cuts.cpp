//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_corner_cuts.cpp
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

#include "handoff_corner_cuts.h"

#include "../vpatterndb/AbstractPiece.h"

#include <algorithm>

namespace layout_handoff
{
//---------------------------------------------------------------------------------------------------------------------
CornerCutCompare compare_corner_cuts(const QVector<SeamAllowanceCorners>& sizes)
{
    CornerCutCompare result;
    bool             same_count = true;
    bool             same_cuts  = true;

    // Compare every size with the first one and collect every cut corner.
    for (const SeamAllowanceCorners& corners : sizes)
    {
        same_count = same_count && corners.count == sizes.first().count;
        same_cuts  = same_cuts && corners.cut == sizes.first().cut;
        result.cuts += corners.cut;
    }
    std::sort(result.cuts.begin(), result.cuts.end());
    result.cuts.erase(std::unique(result.cuts.begin(), result.cuts.end()), result.cuts.end());

    // Same cuts win over a different corner count: no size needs a change.
    if (!same_cuts)
    {
        result.match = same_count ? CornerCutMatch::uneven : CornerCutMatch::count_differs;
    }
    return result;
}
} // namespace layout_handoff
