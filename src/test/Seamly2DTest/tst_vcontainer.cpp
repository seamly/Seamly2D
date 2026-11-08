//---------------------------------------------------------------------------------------------------------------------
// @file tst_vcontainer.cpp
// @author slspencer
// @brief Tests VContainer data scoping and tool data copies.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
//  @file   tst_vcontainer.cpp
//  @author Odrnorn
//  @date   8 Sep, 2026
//
//  @copyright
//  Copyright (C)  2026 Seamly, LLC
//  https://github.com/fashionfreedom/seamly2d
//
//  @brief
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
//  along with Seamly2D. If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

#include "tst_vcontainer.h"

#include <QtTest>

#include "../vpatterndb/vcontainer.h"
#include "../vgeometry/vpointf.h"
#include "../vtools/tools/pattern_piece_tool.h"

//---------------------------------------------------------------------------------------------------------------------
TST_VContainer::TST_VContainer(QObject *parent)
    :QObject(parent)
{}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Reproduces issue #1694: reparsing one draft block must not wipe another block's still-valid
 * Calculation-mode objects, so a later block can keep forward-referencing an earlier one.
 *
 * VContainer has no notion of draft blocks of its own - ClearCalculationGObjects() resolves each
 * object's block through a caller-supplied lookup (in production, backed by the document's tool
 * history, VAbstractPattern::getToolDraftBlockName()). Here a plain QHash stands in for that history.
 */
void TST_VContainer::reparseDoesNotClearOtherBlocks() const
{
    const Unit unit = Unit::Cm;
    VContainer data(nullptr, &unit);

    const quint32 id_a = data.AddGObject(new VPointF(0, 0, QStringLiteral("A1"), 0, 0));
    const quint32 id_b = data.AddGObject(new VPointF(10, 10, QStringLiteral("B1"), 0, 0));

    QHash<quint32, QString> tool_block;
    tool_block.insert(id_a, QStringLiteral("BlockA"));
    tool_block.insert(id_b, QStringLiteral("BlockB"));
    const auto draft_block_for_tool = [&tool_block](quint32 id){ return tool_block.value(id); };

    // Simulate reparsing BlockB's calculation stage - BlockA must survive untouched.
    data.ClearCalculationGObjects(QStringLiteral("BlockB"), draft_block_for_tool);

    QVERIFY2(data.DataGObjects()->contains(id_a),
             "BlockA's object was wiped while reparsing BlockB - the clear is not block-scoped");
    QVERIFY2(not data.DataGObjects()->contains(id_b),
             "BlockB's own object survived the reparse of its own block");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief An object whose owning tool has no matching history entry (lookup returns an empty string)
 * must never be cleared, regardless of which block is being reparsed - it's a conservative default,
 * not something that should ever silently match a real block name.
 */
void TST_VContainer::unknownToolIsNeverCleared() const
{
    const Unit unit = Unit::Cm;
    VContainer data(nullptr, &unit);

    const quint32 id_unknown = data.AddGObject(new VPointF(0, 0, QStringLiteral("A1"), 0, 0));

    const auto draft_block_for_tool = [](quint32 /*id*/){ return QString(); };

    data.ClearCalculationGObjects(QStringLiteral("BlockA"), draft_block_for_tool);

    QVERIFY2(data.DataGObjects()->contains(id_unknown),
             "An object with no resolvable draft block was cleared instead of conservatively kept");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A tool's container copy does not see a node added to the pattern data after the copy.
 *
 * Adding an anchor creates its node in the pattern data while the piece tool still holds an older copy.
 * Drawing the piece markings from that copy must not throw, and it must find the node after a data sync.
 */
void TST_VContainer::markingPointNeedsToolDataSync() const
{
    const Unit unit = Unit::Cm;
    VContainer pattern_data(nullptr, &unit);
    pattern_data.AddGObject(new VPointF(0, 0, QStringLiteral("A1"), 0, 0));

    VContainer piece_tool_data = pattern_data;
    const quint32 anchor_node = pattern_data.AddGObject(new VPointF(3, 4, QStringLiteral("A2"), 0, 0));
    QVERIFY2(!piece_tool_data.DataGObjects()->contains(anchor_node),
             "The container copy shares later additions; the stale-copy case cannot occur");

    QPointF point;
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("marking point %1 is not in the tool data")
                                                              .arg(anchor_node)));
    QVERIFY2(!PatternPieceTool::markingPoint(piece_tool_data, 1, anchor_node, point),
             "A missing marking point was reported as found");

    piece_tool_data = pattern_data;
    QVERIFY2(PatternPieceTool::markingPoint(piece_tool_data, 1, anchor_node, point),
             "The marking point is missing after the tool data sync");
    QCOMPARE(point, QPointF(3, 4));
}
