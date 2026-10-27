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
#include "../layout_handoff/handoff_turn_points.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vpatterndb/floatItemData/floatitemdef.h"
#include "../vgeometry/vpointf.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vlayout/vlayoutpiecepath.h"
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

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief add_paths adds one internal path and one cutout path across the square of square_piece_with_slit.
 */
void add_paths(VContainer *data, VPiece *piece)
{
    data->UpdateGObject(6, new VPointF(100, 100, "B1", 0, 0));
    data->UpdateGObject(7, new VPointF(300, 300, "B2", 0, 0));
    data->UpdateGObject(8, new VPointF(100, 300, "B3", 0, 0));
    data->UpdateGObject(9, new VPointF(300, 100, "B4", 0, 0));

    VPiecePath internal_path(PiecePathType::InternalPath);
    internal_path.Append(VPieceNode(6, Tool::NodePoint));
    internal_path.Append(VPieceNode(7, Tool::NodePoint));
    internal_path.setLineColor(QStringLiteral("red"));
    internal_path.setLineType(Qt::DashLine);
    internal_path.setLineWeight(QStringLiteral("0.5"));
    data->UpdatePiecePath(100, internal_path);

    VPiecePath cutout_path(PiecePathType::InternalPath);
    cutout_path.Append(VPieceNode(8, Tool::NodePoint));
    cutout_path.Append(VPieceNode(9, Tool::NodePoint));
    cutout_path.setCutPath(true);
    data->UpdatePiecePath(101, cutout_path);

    piece->getInternalPaths() << 100 << 101;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief show_grainline gives the piece a grainline from (100, 350), pointing up, with the given length.
 */
void show_grainline(VPiece *piece, const QString &length, ArrowType type)
{
    VGrainlineData &grainline = piece->GetGrainlineGeometry();
    grainline.SetVisible(true);
    grainline.SetPos(QPointF(100, 350));
    grainline.setRotation(QStringLiteral("90"));
    grainline.setLength(length);
    grainline.setArrowLength(QStringLiteral("20"));
    grainline.setArrowType(type);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief square_piece_on_fold flags the first and last main path nodes as the fold line: the left side of the square.
 */
VPiece square_piece_on_fold(VContainer *data)
{
    VPiece piece = square_piece_with_slit(data, true, false);
    QVector<VPieceNode> nodes = piece.GetPath().getNodes();
    nodes.first().setFoldLine(true);
    nodes.last().setFoldLine(true);
    piece.GetPath().setNodes(nodes);
    return piece;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Internal and cutout paths keep their points, style, turn points and node indices.
 */
void TST_HandoffPiece::internalAndCutoutPathsMatchOldWriter() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    VPiece piece = square_piece_with_slit(data.data(), true, false);
    add_paths(data.data(), &piece);

    const VLayoutPiece old_piece = VLayoutPiece::Create(piece, data.data());
    const HandoffPiece new_piece = HandoffPiece::create(piece, data.data());

    QCOMPARE(new_piece.internal_paths().size(), 1);
    QCOMPARE(new_piece.cutout_paths().size(), 1);
    QCOMPARE(old_piece.getInternalPaths().size(), 1);
    QCOMPARE(old_piece.getCutoutPaths().size(), 1);

    const HandoffPath      &internal_new = new_piece.internal_paths().first();
    const VLayoutPiecePath  internal_old = old_piece.getInternalPaths().first();
    QVERIFY(!internal_new.line.points.isEmpty());
    QCOMPARE(internal_new.line.points, internal_old.Points());
    QCOMPARE(internal_new.color, internal_old.getLineColor());
    QCOMPARE(internal_new.color, QStringLiteral("red"));
    QCOMPARE(internal_new.line_type, internal_old.getLineType());
    QCOMPARE(internal_new.line_weight, internal_old.getLineWeight());
    QVERIFY(!internal_new.cut);
    QVERIFY(internal_new.line.turn_points.has_value());
    QCOMPARE(internal_new.line.turn_points, internal_old.turnPoints());

    const HandoffPath      &cutout_new = new_piece.cutout_paths().first();
    const VLayoutPiecePath  cutout_old = old_piece.getCutoutPaths().first();
    QVERIFY(!cutout_new.line.points.isEmpty());
    QCOMPARE(cutout_new.line.points, cutout_old.Points());
    QVERIFY(cutout_new.cut);
    QCOMPARE(cutout_new.line.turn_points, cutout_old.turnPoints());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffPiece::grainlineMatchesOldWriter_data() const
{
    QTest::addColumn<QString>("length");
    QTest::addColumn<int>("arrow_type");

    QTest::newRow("both arrows")   << QStringLiteral("200") << static_cast<int>(ArrowType::Both);
    QTest::newRow("top arrow")     << QStringLiteral("200") << static_cast<int>(ArrowType::Top);
    QTest::newRow("bottom arrow")  << QStringLiteral("200") << static_cast<int>(ArrowType::Bottom);
    QTest::newRow("outside piece") << QStringLiteral("600") << static_cast<int>(ArrowType::Both);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The grainline path, with the shift that keeps it in the piece, and the grain direction equal the old writer.
 */
void TST_HandoffPiece::grainlineMatchesOldWriter() const
{
    QFETCH(QString, length);
    QFETCH(int, arrow_type);

    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    VPiece piece = square_piece_with_slit(data.data(), true, false);
    show_grainline(&piece, length, static_cast<ArrowType>(arrow_type));

    const VLayoutPiece old_piece = VLayoutPiece::Create(piece, data.data());
    const HandoffPiece new_piece = HandoffPiece::create(piece, data.data());

    QVERIFY(new_piece.is_grainline_visible());
    QVERIFY(!old_piece.getGrainline().isEmpty());
    QVERIFY(!new_piece.grainline_points().isEmpty());
    QCOMPARE(new_piece.grainline_points().size(), old_piece.getGrainline().size());
    for (int i = 0; i < new_piece.grainline_points().size(); ++i)
    {
        QVERIFY2(QLineF(new_piece.grainline_points().at(i), old_piece.getGrainline().at(i)).length() < 0.01,
                 qPrintable(QString::number(i)));
    }

    qreal old_angle = 0;
    QVERIFY(old_piece.grainlineAngle(old_angle));
    QCOMPARE(new_piece.grain_axis().size(), 2);
    QVERIFY(qAbs(QLineF(new_piece.grain_axis().at(0), new_piece.grain_axis().at(1)).angle() - old_angle) < 0.01);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A hidden grainline keeps its path and direction; only the visible flag changes.
 */
void TST_HandoffPiece::hiddenGrainlineKeepsItsData() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    VPiece piece = square_piece_with_slit(data.data(), true, false);
    show_grainline(&piece, QStringLiteral("200"), ArrowType::Both);
    const HandoffPiece shown = HandoffPiece::create(piece, data.data());

    piece.GetGrainlineGeometry().SetVisible(false);
    const HandoffPiece hidden = HandoffPiece::create(piece, data.data());

    QVERIFY(shown.is_grainline_visible());
    QVERIFY(!hidden.is_grainline_visible());
    QVERIFY(!hidden.grainline_points().isEmpty());
    QCOMPARE(hidden.grainline_points(), shown.grainline_points());
    QCOMPARE(hidden.grain_axis(), shown.grain_axis());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The fold line, its cut line extension and its arrow and label geometry equal the old writer.
 */
void TST_HandoffPiece::foldLineMatchesOldWriter() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    const VPiece piece = square_piece_on_fold(data.data());

    const VLayoutPiece old_piece = VLayoutPiece::Create(piece, data.data());
    const HandoffPiece new_piece = HandoffPiece::create(piece, data.data());

    QVERIFY(!new_piece.fold_line().isNull());
    QVERIFY(new_piece.is_on_fold());
    QCOMPARE(new_piece.is_on_fold(), old_piece.isOnFold());
    QCOMPARE(new_piece.fold_line(), old_piece.getFoldLineGraphic().foldLine());

    const pattern_piece::FoldLineGraphic &graphic     = new_piece.fold_line_graphic();
    const pattern_piece::FoldLineGraphic  old_graphic = old_piece.getFoldLineGraphic();
    QVERIFY(!graphic.isNull());
    QCOMPARE(graphic.arrowLine(), old_graphic.arrowLine());
    QCOMPARE(graphic.startArrowHead(), old_graphic.startArrowHead());
    QCOMPARE(graphic.endArrowHead(), old_graphic.endArrowHead());
    QCOMPARE(graphic.labelCenter(), old_graphic.labelCenter());
    QCOMPARE(graphic.labelAngle(), old_graphic.labelAngle());

    // The old writer sends the fold line out to the cut line whenever it sends the cut line.
    const QVector<QPointF> old_line = old_piece.getFoldLine();
    QCOMPARE(old_line.size(), 2);
    QVERIFY(!new_piece.fold_line_on_cut_line().isNull());
    QCOMPARE(new_piece.fold_line_on_cut_line().p1(), old_line.at(0));
    QCOMPARE(new_piece.fold_line_on_cut_line().p2(), old_line.at(1));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A piece without a fold line has no fold line data and is not on the fold.
 */
void TST_HandoffPiece::pieceWithoutFoldLineHasNoFoldData() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    const VPiece piece = square_piece_with_slit(data.data(), true, false);

    const HandoffPiece handoff = HandoffPiece::create(piece, data.data());

    QVERIFY(handoff.fold_line().isNull());
    QVERIFY(handoff.fold_line_on_cut_line().isNull());
    QVERIFY(handoff.fold_line_graphic().isNull());
    QVERIFY(!handoff.is_on_fold());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The cut corners, the corner count and the line node counts equal what VLayoutPiece gives.
 */
void TST_HandoffPiece::cornerCutsMatchOldWriter() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    // Equilateral triangle: every miter is longer than the width, so a forced cut shortens corner 1.
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(400, 0, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(200, 346.41, "A3", 0, 0));

    VPiece piece;
    piece.SetSeamAllowance(true);
    piece.SetSAWidth(10);
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(2, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    piece.setForcedCornerCuts({1});

    const VLayoutPiece old_piece = VLayoutPiece::Create(piece, data.data());
    const HandoffPiece new_piece = HandoffPiece::create(piece, data.data());

    QCOMPARE(new_piece.seam_allowance_corners().count, 3);
    QCOMPARE(new_piece.seam_allowance_corners().cut, QVector<int>{1});
    QCOMPARE(new_piece.seam_allowance_corners().count, old_piece.seamAllowanceCorners().count);
    QCOMPARE(new_piece.seam_allowance_corners().cut, old_piece.seamAllowanceCorners().cut);
    QCOMPARE(new_piece.line_node_counts(), old_piece.lineNodeCounts());
}

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_size builds the slit square with its paths, scaled about the origin.
 */
VPiece make_size(VContainer *data, qreal scale)
{
    VPiece piece = square_piece_with_slit(data, true, false);
    data->UpdateGObject(2, new VPointF(200 * scale, 0, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(400 * scale, 0, "A3", 0, 0));
    data->UpdateGObject(4, new VPointF(400 * scale, 400 * scale, "A4", 0, 0));
    data->UpdateGObject(5, new VPointF(0, 400 * scale, "A5", 0, 0));
    add_paths(data, &piece);
    return piece;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Unifying three sizes gives the turn points that VLayoutPiece::unifyTurnPoints and unify_turn_points give.
 */
void TST_HandoffPiece::unifyTurnPointsMatchesOldWriter() const
{
    const Unit unit = Unit::Mm;
    qApp->setPatternUnit(unit);
    const qreal scales[] = {1.0, 1.1, 1.25};

    QVector<QSharedPointer<VContainer>> containers;
    QVector<VLayoutPiece>               old_pieces;
    QVector<HandoffPiece>               new_pieces;
    for (const qreal scale : scales)
    {
        containers.append(QSharedPointer<VContainer>(new VContainer(nullptr, &unit)));
        const VPiece piece = make_size(containers.last().data(), scale);
        old_pieces.append(VLayoutPiece::Create(piece, containers.last().data()));
        new_pieces.append(HandoffPiece::create(piece, containers.last().data()));
    }

    QVector<SizeLine> seam_lines;
    QVector<SizeLine> cut_lines;
    for (const HandoffPiece &piece : std::as_const(new_pieces))
    {
        seam_lines.append({piece.seam_line().points, piece.seam_line().turn_points, piece.seam_line().node_indices});
        cut_lines.append({piece.cut_line().points, piece.cut_line().turn_points, piece.cut_line().node_indices});
    }
    const QVector<QVector<int>> expected_seam = unify_turn_points(seam_lines, true);
    const QVector<QVector<int>> expected_cut  = unify_turn_points(cut_lines, true);
    QCOMPARE(expected_seam.size(), 3);
    QCOMPARE(expected_cut.size(), 3);

    QVector<VLayoutPiece *> old_sizes;
    QVector<HandoffPiece *> new_sizes;
    for (int i = 0; i < 3; ++i)
    {
        old_sizes.append(&old_pieces[i]);
        new_sizes.append(&new_pieces[i]);
    }
    VLayoutPiece::unifyTurnPoints(old_sizes);
    HandoffPiece::unify_piece_turn_points(new_sizes);

    for (int i = 0; i < 3; ++i)
    {
        QVERIFY(new_pieces.at(i).seam_line().turn_points.has_value());
        QCOMPARE(*new_pieces.at(i).seam_line().turn_points, expected_seam.at(i));
        QVERIFY(new_pieces.at(i).cut_line().turn_points.has_value());
        QCOMPARE(*new_pieces.at(i).cut_line().turn_points, expected_cut.at(i));

        QVERIFY(new_pieces.at(i).internal_paths().first().line.turn_points.has_value());
        QCOMPARE(new_pieces.at(i).internal_paths().first().line.turn_points,
                 old_pieces.at(i).getInternalPaths().first().turnPoints());
        QVERIFY(new_pieces.at(i).cutout_paths().first().line.turn_points.has_value());
        QCOMPARE(new_pieces.at(i).cutout_paths().first().line.turn_points,
                 old_pieces.at(i).getCutoutPaths().first().turnPoints());
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A size with another node count in a line keeps the turn points of that line, as VLayoutPiece does.
 */
void TST_HandoffPiece::unifySkipsLineWithDifferentNodeCount() const
{
    const Unit unit = Unit::Mm;
    qApp->setPatternUnit(unit);

    QScopedPointer<VContainer> data_a(new VContainer(nullptr, &unit));
    const VPiece               full = make_size(data_a.data(), 1.0);

    // The same square without the slit node: 4 nodes instead of 5.
    QScopedPointer<VContainer> data_b(new VContainer(nullptr, &unit));
    VPiece                     fewer = make_size(data_b.data(), 1.0);
    fewer.GetPath().Clear();
    fewer.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    fewer.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    fewer.GetPath().Append(VPieceNode(4, Tool::NodePoint));
    fewer.GetPath().Append(VPieceNode(5, Tool::NodePoint));

    HandoffPiece new_full  = HandoffPiece::create(full, data_a.data());
    HandoffPiece new_fewer = HandoffPiece::create(fewer, data_b.data());
    VLayoutPiece old_full  = VLayoutPiece::Create(full, data_a.data());
    VLayoutPiece old_fewer = VLayoutPiece::Create(fewer, data_b.data());
    QVERIFY(new_full.line_node_counts() != new_fewer.line_node_counts());
    QCOMPARE(new_full.line_node_counts(), old_full.lineNodeCounts());
    QCOMPARE(new_fewer.line_node_counts(), old_fewer.lineNodeCounts());

    const std::optional<QVector<int>> before_full  = new_full.seam_line().turn_points;
    const std::optional<QVector<int>> before_fewer = new_fewer.seam_line().turn_points;
    HandoffPiece::unify_piece_turn_points({&new_full, &new_fewer});
    QCOMPARE(new_full.seam_line().turn_points, before_full);
    QCOMPARE(new_fewer.seam_line().turn_points, before_fewer);

    // The paths still match by order and unify.
    VLayoutPiece::unifyTurnPoints({&old_full, &old_fewer});
    QCOMPARE(new_full.internal_paths().first().line.turn_points,
             old_full.getInternalPaths().first().turnPoints());
    QCOMPARE(new_fewer.cutout_paths().first().line.turn_points,
             old_fewer.getCutoutPaths().first().turnPoints());
}
