/******************************************************************************
 **  @file   tst_handofflabel.cpp
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for the piece label and pattern label of layout_handoff
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

#include "tst_handofflabel.h"
#include "../layout_handoff/handoff_label.h"
#include "../layout_handoff/handoff_piece.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vpointf.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"

#include <QtTest>

using namespace layout_handoff;

namespace
{
/**
 * @brief TestPatternDocument is a pattern document with no tools; it holds the label template and the pattern name.
 */
class TestPatternDocument : public VAbstractPattern
{
public:
    TestPatternDocument()
    {
        setContent(QStringLiteral("<pattern/>"));
    }

    void CreateEmptyFile() override {}
    void IncrementReferens(quint32) const override {}
    void DecrementReferens(quint32) const override {}
    QStringList GetCurrentAlphabet() const override { return QStringList(); }
    QString GenerateLabel(const LabelType &, const QString &) const override { return QString(); }
    QString generateSuffix(const QString &) const override { return QString(); }
    void UpdateToolData(const quint32 &, VContainer *) override {}
    void LiteParseTree(const Document &) override {}
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief label_line returns one label template line.
 */
VLabelTemplateLine label_line(const QString &text, bool bold, int alignment, int increment)
{
    VLabelTemplateLine line;
    line.line              = text;
    line.bold              = bold;
    line.italic            = false;
    line.alignment         = alignment;
    line.fontSizeIncrement = increment;
    return line;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief show_label gives the label data a visible box at a position, with a rotation in degrees.
 */
void show_label(VPatternLabelData *label, const QPointF &pos, const QString &width, const QString &height,
                const QString &rotation)
{
    label->SetVisible(true);
    label->SetPos(pos);
    label->SetLabelWidth(width);
    label->SetLabelHeight(height);
    label->SetRotation(rotation);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief square_piece builds a 400 by 400 piece with a piece label and a pattern label, both visible.
 * @param data receives the four corner points. The pattern unit must be pixels.
 */
VPiece square_piece(VContainer *data, const QPointF &label_pos, const QString &rotation)
{
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(400, 0, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(400, 400, "A3", 0, 0));
    data->UpdateGObject(4, new VPointF(0, 400, "A4", 0, 0));

    VPiece piece;
    piece.SetName(QStringLiteral("Front"));
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(2, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(4, Tool::NodePoint));
    piece.GetGrainlineGeometry().SetVisible(false);

    show_label(&piece.GetPatternPieceData(), label_pos, QStringLiteral("200"), QStringLiteral("100"), rotation);
    piece.GetPatternPieceData().SetLetter(QStringLiteral("A"));
    QVector<VLabelTemplateLine> lines;
    lines << label_line(QStringLiteral("%pName%"), true, Qt::AlignHCenter, 2)
          << label_line(QStringLiteral("Piece %pLetter%"), false, Qt::AlignLeft, 0);
    piece.GetPatternPieceData().SetLabelTemplate(lines);

    show_label(&piece.GetPatternInfo(), QPointF(20, 300), QStringLiteral("150"), QStringLiteral("60"),
               QStringLiteral("0"));
    return piece;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief set_pattern_label_template gives the document a two line pattern label template.
 */
void set_pattern_label_template(TestPatternDocument *document)
{
    QVector<VLabelTemplateLine> lines;
    lines << label_line(QStringLiteral("Jacket"), true, Qt::AlignHCenter, 1)
          << label_line(QStringLiteral("Size %size%"), false, Qt::AlignRight, 0);
    document->setPatternLabelTemplate(lines);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief compare_corner checks that two points are less than 0.01 pixel apart.
 */
void compare_corner(const QPointF &actual, const QPointF &expected)
{
    QVERIFY2(QLineF(actual, expected).length() < 0.01,
             qPrintable(QStringLiteral("(%1, %2) is not (%3, %4)")
                            .arg(actual.x()).arg(actual.y()).arg(expected.x()).arg(expected.y())));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_HandoffLabel::TST_HandoffLabel(QObject *parent)
    : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffLabel::pieceLabelMatchesOldWriter_data() const
{
    QTest::addColumn<QPointF>("pos");
    QTest::addColumn<QString>("rotation");

    QTest::newRow("inside the piece")           << QPointF(50, 50)   << QStringLiteral("0");
    QTest::newRow("rotated")                    << QPointF(50, 50)   << QStringLiteral("90");
    QTest::newRow("rotated by a formula")       << QPointF(100, 120) << QStringLiteral("30+15");
    QTest::newRow("past the right and bottom")  << QPointF(350, 350) << QStringLiteral("0");
    QTest::newRow("past the left and top")      << QPointF(-80, -60) << QStringLiteral("0");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The piece label shape position and text equal the old writer.
 */
void TST_HandoffLabel::pieceLabelMatchesOldWriter() const
{
    QFETCH(QPointF, pos);
    QFETCH(QString, rotation);

    const Unit unit = Unit::Px;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    TestPatternDocument document;
    VAbstractPattern *previous = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    const VPiece piece = square_piece(data.data(), pos, rotation);
    const VLayoutPiece old_piece = VLayoutPiece::Create(piece, data.data());
    const HandoffPiece handoff   = HandoffPiece::create(piece, data.data());
    const HandoffLabel label     = HandoffLabel::create_piece_label(piece, data.data(), handoff.bounds(),
                                                                    qApp->Settings()->getLabelFont());
    qApp->setCurrentDocument(previous);

    QVERIFY(label.has_shape());
    QVERIFY(label.is_visible());
    QCOMPARE(label.shape().size(), 4);
    QVERIFY(!old_piece.getPieceText().isEmpty());
    QCOMPARE(label.text(), old_piece.getPieceText());
    QCOMPARE(label.text(), (QStringList{QStringLiteral("Front"), QStringLiteral("Piece A")}));
    compare_corner(label.shape().first(), old_piece.getPieceTextPosition());

    // The shape keeps the label size, whatever the rotation.
    QVERIFY(qAbs(QLineF(label.shape().at(0), label.shape().at(1)).length() - 200) < 0.01);
    QVERIFY(qAbs(QLineF(label.shape().at(1), label.shape().at(2)).length() - 100) < 0.01);

    QCOMPARE(label.lines().size(), 2);
    QVERIFY(label.lines().at(0).bold);
    QCOMPARE(label.lines().at(0).font_size_increment, 2);
    QCOMPARE(static_cast<int>(label.lines().at(1).alignment & Qt::AlignLeft), static_cast<int>(Qt::AlignLeft));
    QVERIFY(label.font().pixelSize() >= LabelTextManager::min_font_size);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The pattern label shape position and text equal the old writer.
 */
void TST_HandoffLabel::patternLabelMatchesOldWriter() const
{
    const Unit unit = Unit::Px;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    TestPatternDocument document;
    set_pattern_label_template(&document);
    VAbstractPattern *previous = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    const VPiece piece = square_piece(data.data(), QPointF(50, 50), QStringLiteral("0"));
    const VLayoutPiece old_piece = VLayoutPiece::Create(piece, data.data());
    const HandoffPiece handoff   = HandoffPiece::create(piece, data.data());
    const HandoffLabel label     = HandoffLabel::create_pattern_label(piece.GetPatternInfo(), &document, data.data(),
                                                                      handoff.bounds(),
                                                                      qApp->Settings()->getLabelFont());
    qApp->setCurrentDocument(previous);

    QVERIFY(label.has_shape());
    QVERIFY(!old_piece.getPatternText().isEmpty());
    QCOMPARE(label.text(), old_piece.getPatternText());
    QCOMPARE(label.text().first(), QStringLiteral("Jacket"));
    compare_corner(label.shape().first(), old_piece.getPatternTextPosition());
    compare_corner(label.shape().first(), QPointF(20, 300));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A label that sticks out of the piece moves in by the largest overshoot.
 */
void TST_HandoffLabel::labelShiftsIntoPieceBounds() const
{
    const Unit unit = Unit::Px;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    TestPatternDocument document;
    VAbstractPattern *previous = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    const VPiece piece         = square_piece(data.data(), QPointF(350, 350), QStringLiteral("0"));
    const HandoffPiece handoff = HandoffPiece::create(piece, data.data());
    const HandoffLabel label   = HandoffLabel::create_piece_label(piece, data.data(), handoff.bounds(),
                                                                  qApp->Settings()->getLabelFont());
    qApp->setCurrentDocument(previous);

    const QRectF bounds = handoff.bounds();
    QVERIFY(!bounds.isNull());
    compare_corner(label.shape().at(0), QPointF(bounds.right() - 200, bounds.bottom() - 100));
    compare_corner(label.shape().at(2), QPointF(bounds.right(), bounds.bottom()));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A hidden label keeps its shape and text; only the visible flag changes.
 */
void TST_HandoffLabel::hiddenLabelKeepsItsData() const
{
    const Unit unit = Unit::Px;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    TestPatternDocument document;
    VAbstractPattern *previous = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    VPiece piece               = square_piece(data.data(), QPointF(50, 50), QStringLiteral("0"));
    const HandoffPiece handoff = HandoffPiece::create(piece, data.data());
    const QFont font           = qApp->Settings()->getLabelFont();
    const HandoffLabel shown   = HandoffLabel::create_piece_label(piece, data.data(), handoff.bounds(), font);

    piece.GetPatternPieceData().SetVisible(false);
    const HandoffLabel hidden = HandoffLabel::create_piece_label(piece, data.data(), handoff.bounds(), font);
    qApp->setCurrentDocument(previous);

    QVERIFY(shown.is_visible());
    QVERIFY(!hidden.is_visible());
    QVERIFY(hidden.has_shape());
    QCOMPARE(hidden.shape(), shown.shape());
    QCOMPARE(hidden.text(), shown.text());
    QCOMPARE(hidden.font().pixelSize(), shown.font().pixelSize());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A label whose rotation formula does not evaluate has no shape and no text.
 */
void TST_HandoffLabel::labelWithBadFormulaHasNoShape() const
{
    const Unit unit = Unit::Px;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    TestPatternDocument document;
    VAbstractPattern *previous = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    const VPiece piece         = square_piece(data.data(), QPointF(50, 50), QStringLiteral("no_such_variable"));
    const HandoffPiece handoff = HandoffPiece::create(piece, data.data());
    const HandoffLabel label   = HandoffLabel::create_piece_label(piece, data.data(), handoff.bounds(),
                                                                  qApp->Settings()->getLabelFont());
    qApp->setCurrentDocument(previous);

    QVERIFY(!label.has_shape());
    QVERIFY(label.text().isEmpty());
    QVERIFY(label.lines().isEmpty());
}
