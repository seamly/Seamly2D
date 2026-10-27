/******************************************************************************
 **  @file   tst_handoffcornercuts.cpp
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for the corner cut compare of layout_handoff.
 **
 **  @copyright
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
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
 **  along with Seamly2D.  If not, see <https://www.gnu.org/licenses/gpl-3.0.html>.
 **
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **
 *****************************************************************************/

#include "tst_handoffcornercuts.h"
#include "../layout_handoff/handoff_corner_cuts.h"
#include "../vpatterndb/AbstractPiece.h"

#include <QtTest>

using namespace layout_handoff;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief corners returns the seam allowance corners of one size.
 * @param count number of corners.
 * @param cut   numbers of the corners cut short, ascending.
 */
SeamAllowanceCorners corners(int count, const QVector<int>& cut)
{
    SeamAllowanceCorners result;
    result.count = count;
    result.cut   = cut;
    return result;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_HandoffCornerCuts::TST_HandoffCornerCuts(QObject* parent)
    : QObject(parent)
{}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Every size cuts the same corners: no size needs a change, also when the corner counts differ.
 */
void TST_HandoffCornerCuts::sameCutsNeedNoChange() const
{
    const CornerCutCompare same = compare_corner_cuts({corners(6, {2}), corners(6, {2}), corners(6, {2})});
    QCOMPARE(same.match, CornerCutMatch::same);
    QCOMPARE(same.cuts, (QVector<int>{2}));

    const CornerCutCompare other_count = compare_corner_cuts({corners(6, {}), corners(7, {})});
    QCOMPARE(other_count.match, CornerCutMatch::same);
    QVERIFY(other_count.cuts.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The sizes cut different corners and have the same corner count: every size cuts every corner found.
 */
void TST_HandoffCornerCuts::differentCutsWithSameCountAreUneven() const
{
    // The small sizes cut corners 4 and 1; the large size cuts none.
    const CornerCutCompare result = compare_corner_cuts({corners(6, {1, 4}), corners(6, {4}), corners(6, {})});
    QCOMPARE(result.match, CornerCutMatch::uneven);
    QCOMPARE(result.cuts, (QVector<int>{1, 4}));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The sizes cut different corners and differ in corner count: the corner numbers do not match.
 */
void TST_HandoffCornerCuts::differentCornerCountIsLeftOut() const
{
    const CornerCutCompare result = compare_corner_cuts({corners(6, {3}), corners(7, {5})});
    QCOMPARE(result.match, CornerCutMatch::count_differs);
    QCOMPARE(result.cuts, (QVector<int>{3, 5}));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A piece in no size needs no change.
 */
void TST_HandoffCornerCuts::noSizesNeedNoChange() const
{
    const CornerCutCompare result = compare_corner_cuts({});
    QCOMPARE(result.match, CornerCutMatch::same);
    QVERIFY(result.cuts.isEmpty());
}
