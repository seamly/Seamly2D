/******************************************************************************
 **  @file   tst_handoffpiece.cpp
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for the piece geometry of layout_handoff
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

#include "tst_handoffpiece.h"
#include "../layout_handoff/handoff_piece.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vgeometry/vpointf.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/vabstractapplication.h"

#include <QtTest>

using namespace layout_handoff;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief square_piece_with_slit builds a 400 by 400 square with a slit notch on its top edge.
 * @param data              receives the four corner points and the notch point.
 * @param seam_allowance    whether the piece has a seam allowance.
 * @param built_in          whether the seam allowance is built into the main path.
 */
VPiece square_piece_with_slit(VContainer *data, bool seam_allowance, bool built_in)
{
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(200, 0, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(400, 0, "A3", 0, 0));
    data->UpdateGObject(4, new VPointF(400, 400, "A4", 0, 0));
    data->UpdateGObject(5, new VPointF(0, 400, "A5", 0, 0));

    VPieceNode notch_node(2, Tool::NodePoint);
    notch_node.setNotch(true);
    notch_node.setNotchType(NotchType::Slit);
    notch_node.setNotchSubType(NotchSubType::Straightforward);
    notch_node.setNotchLength(5);
    notch_node.setNotchWidth(5);
    notch_node.setNotchCount(1);
    notch_node.setShowNotch(true);
    notch_node.setShowSeamlineNotch(false);

    VPiece piece;
    piece.SetSeamAllowance(seam_allowance);
    piece.SetSeamAllowanceBuiltIn(built_in);
    piece.SetSAWidth(10);
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(notch_node);
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(4, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(5, Tool::NodePoint));

    // Labels and grainline are not under test; hidden, they need no pattern document.
    piece.GetPatternPieceData().SetVisible(false);
    piece.GetPatternInfo().SetVisible(false);
    piece.GetGrainlineGeometry().SetVisible(false);
    return piece;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_HandoffPiece::TST_HandoffPiece(QObject *parent)
    : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffPiece::seamAndCutLinesMatchOldWriter_data() const
{
    QTest::addColumn<bool>("built_in");

    QTest::newRow("cut line")  << false;
    QTest::newRow("built in")  << true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Every line, node count and notch list equals what VLayoutPiece::Create gives.
 */
void TST_HandoffPiece::seamAndCutLinesMatchOldWriter() const
{
    QFETCH(bool, built_in);

    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    const VPiece piece = square_piece_with_slit(data.data(), true, built_in);

    const VLayoutPiece old_piece = VLayoutPiece::Create(piece, data.data());
    const HandoffPiece new_piece = HandoffPiece::create(piece, data.data());

    QVERIFY(new_piece.has_shape());
    QCOMPARE(new_piece.has_seam_allowance(), old_piece.hasSeamAllowance());
    QCOMPARE(new_piece.has_built_in_seam_allowance(), old_piece.hasSeamAllowanceBuiltIn());
    QCOMPARE(new_piece.seam_line().points, old_piece.getContourPoints());
    QCOMPARE(new_piece.cut_line().points, old_piece.getSeamAllowancePoints());
    QCOMPARE(new_piece.built_in_seam_line().points, old_piece.getBuiltInSeamLinePoints());
    QCOMPARE(new_piece.built_in_seam_line().closed, old_piece.isBuiltInSeamLineClosed());
    QCOMPARE(new_piece.notches().lines, old_piece.getNotches());
    QCOMPARE(new_piece.notches().line_counts, old_piece.getNotchLineCounts());
    QCOMPARE(new_piece.notches().seam_allowance_widths, old_piece.getNotchSeamAllowances());
    QCOMPARE(new_piece.notches().types, old_piece.getNotchTypes());
    QCOMPARE(QVector<int>({static_cast<int>(new_piece.seam_line().node_indices.size()),
                           static_cast<int>(new_piece.cut_line().node_indices.size()),
                           static_cast<int>(new_piece.built_in_seam_line().node_indices.size())}),
             old_piece.lineNodeCounts());

    // Guards: the comparison above proves nothing on empty data.
    QVERIFY(!new_piece.notches().lines.isEmpty());
    QVERIFY(new_piece.seam_line().turn_points.has_value());
    QCOMPARE(new_piece.seam_line().node_indices.size(), 5);
    QCOMPARE(new_piece.cut_line().points.isEmpty(), built_in);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A piece without a seam allowance has a seam line only.
 */
void TST_HandoffPiece::noSeamAllowanceHasNoCutLineOrNotches() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    const VPiece piece = square_piece_with_slit(data.data(), false, false);

    const HandoffPiece handoff = HandoffPiece::create(piece, data.data());

    QVERIFY(!handoff.has_seam_allowance());
    QVERIFY(handoff.cut_line().points.isEmpty());
    QVERIFY(handoff.notches().lines.isEmpty());
    QVERIFY(!handoff.seam_line().points.isEmpty());
    QVERIFY(handoff.has_shape());
}
