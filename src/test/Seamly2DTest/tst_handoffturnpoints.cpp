/******************************************************************************
 **  @file   tst_handoffturnpoints.cpp
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for the turn point unifier of layout_handoff
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
 **  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
 **
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **
 *****************************************************************************/

#include "tst_handoffturnpoints.h"
#include "../layout_handoff/handoff_turn_points.h"

#include <QtTest>

using namespace layout_handoff;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief square_loop returns a square of four nodes, one vertex per node.
 * @param dx           shift of the square along x.
 * @param turns        turn point indices of the line.
 * @param start_corner corner of the square that is vertex 0.
 */
SizeLine square_loop(qreal dx, const QVector<int> &turns, int start_corner = 0)
{
    const QVector<QPointF> corners = {QPointF(0, 0), QPointF(10, 0), QPointF(10, 10), QPointF(0, 10)};
    SizeLine line;
    for (int i = 0; i < 4; ++i)
    {
        line.points.append(corners.at((i + start_corner) % 4) + QPointF(dx, 0));
        line.node_indices.append(i);
    }
    line.turn_points = turns;
    return line;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_HandoffTurnPoints::TST_HandoffTurnPoints(QObject *parent)
    : QObject(parent)
{}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffTurnPoints::nodeTurnInOneSizeTurnsInAll() const
{
    const QVector<QVector<int>> turns = unify_turn_points({square_loop(0, {1}), square_loop(50, {3})}, true);
    QCOMPARE(turns.size(), 2);
    QCOMPARE(turns.at(0), (QVector<int>{1, 3}));
    QCOMPARE(turns.at(1), (QVector<int>{1, 3}));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffTurnPoints::rotatedLoopMatchesNodesByShape() const
{
    // The second size starts at corner 2, so its vertex 2 is the vertex 0 of the first size.
    const QVector<QVector<int>> turns = unify_turn_points({square_loop(0, {0}), square_loop(0, {}, 2)}, true);
    QCOMPARE(turns.size(), 2);
    QCOMPARE(turns.at(0), (QVector<int>{0}));
    QCOMPARE(turns.at(1), (QVector<int>{2}));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffTurnPoints::openLineMatchesNodesByOrder() const
{
    const QVector<QVector<int>> turns = unify_turn_points({square_loop(0, {2}), square_loop(0, {}, 2)}, false);
    QCOMPARE(turns.size(), 2);
    QCOMPARE(turns.at(0), (QVector<int>{2}));
    QCOMPARE(turns.at(1), (QVector<int>{2}));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffTurnPoints::skippedLinesReturnNothing() const
{
    QVERIFY(unify_turn_points({square_loop(0, {1})}, true).isEmpty());

    SizeLine unknown = square_loop(0, {1});
    unknown.turn_points.reset();
    QVERIFY(unify_turn_points({square_loop(0, {1}), unknown}, true).isEmpty());

    SizeLine extra_node = square_loop(0, {1});
    extra_node.node_indices.append(3);
    QVERIFY(unify_turn_points({square_loop(0, {1}), extra_node}, true).isEmpty());
}
