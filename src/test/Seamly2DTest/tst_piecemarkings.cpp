//---------------------------------------------------------------------------------------------------------------------
// @file tst_piecemarkings.cpp
// @author slspencer
// @brief Tests explicit drill diameters, graded references, and optional marking serialization.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "tst_piecemarkings.h"
#include "../../libs/vpatterndb/vpiece.h"
#include "../../libs/vpatterndb/vcontainer.h"
#include "../../libs/vgeometry/vpointf.h"
#include "../../libs/vtools/dialogs/tools/piece/PieceMarkingsEditor.h"
#include "../../libs/vtools/dialogs/tools/piece/pattern_piece_dialog.h"
#include "../../libs/ifc/xml/PatternPieceCopy.h"
#include "../../libs/ifc/xml/vpatternconverter.h"
#include "../../libs/ifc/xml/vdomdocument.h"
#include "../../libs/ifc/xml/vabstractpattern.h"
#include "../../libs/vmisc/vabstractapplication.h"
#include "../../libs/ifc/exception/vexception.h"
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTemporaryDir>
#include <QScopeGuard>
#include <QtTest>
#include <limits>

namespace
{
class MarkingsTestDocument : public VAbstractPattern
{
public:
    void CreateEmptyFile() override {}
    void IncrementReferens(quint32) const override {}
    void DecrementReferens(quint32) const override {}
    QStringList GetCurrentAlphabet() const override { return {}; }
    QString GenerateLabel(const LabelType &, const QString &) const override { return {}; }
    QString generateSuffix(const QString &) const override { return {}; }
    void UpdateToolData(const quint32 &, VContainer *) override {}
    void LiteParseTree(const Document &) override {}
};
}

void TST_PieceMarkings::roundTripAndDetach()
{
    VPiece piece;
    QVERIFY(piece.drillHoles().isEmpty());
    QVERIFY(piece.referenceLines().isEmpty());
    piece.setDrillHoles({{7, 0.125}});
    piece.setReferenceLines({{7, 8, QStringLiteral("stripe")}, {8, 9, QStringLiteral("plaid")}});
    VPiece copy = piece;
    copy.setDrillHoles({{8, 0.25}});
    QCOMPARE(piece.drillHoles().first().diameter, 0.125);
    QCOMPARE(copy.drillHoles().first().center, 8U);
    QDomDocument document;
    QDomElement element = document.createElement(QStringLiteral("piece"));
    document.appendChild(element);
    PieceMarkings::write(element, piece.drillHoles(), piece.referenceLines());
    QCOMPARE(PieceMarkings::readDrillHoles(element).first().diameter, 0.125);
    QCOMPARE(PieceMarkings::readReferenceLines(element).size(), 2);
    PieceMarkings::write(element, {}, {});
    QVERIFY(element.firstChild().isNull());

    Unit unit = Unit::Cm;
    VContainer data(nullptr, &unit);
    data.UpdateGObject(7, new VPointF(10, 20, QStringLiteral("Center"), 0, 0));
    const qreal diameter = piece.drillHoles().first().diameter;
    data.UpdateGObject(7, new VPointF(50, 80, QStringLiteral("Center"), 0, 0));
    QCOMPARE(data.GeometricObject<VPointF>(piece.drillHoles().first().center)->x(), 50.0);
    QCOMPARE(piece.drillHoles().first().diameter, diameter);
}

void TST_PieceMarkings::rejectsUnsafeValues()
{
    QVERIFY(PieceMarkings::valid({{7, 0.1}}, {{7, 8, QStringLiteral("stripe")}}, {7, 8}));
    for (qreal diameter : {0.0, -0.1, std::numeric_limits<qreal>::infinity(),
                           std::numeric_limits<qreal>::quiet_NaN()})
    {
        QVERIFY(!PieceMarkings::valid({{7, diameter}}, {}, {7}));
    }
    QVERIFY(!PieceMarkings::valid({{9, 0.1}}, {}, {7, 8}));
    QVERIFY(!PieceMarkings::valid({}, {{7, 7, QStringLiteral("stripe")}}, {7, 8}));
    QVERIFY(!PieceMarkings::valid({}, {{7, 8, QStringLiteral("cut")}}, {7, 8}));
    QVERIFY(!PieceMarkings::valid({}, {{7, 9, QStringLiteral("plaid")}}, {7, 8}));
}

void TST_PieceMarkings::currentSchemaAndCopy()
{
    QTemporaryDir files;
    QVERIFY(files.isValid());
    const QString file_path = QDir(files.path()).filePath(QStringLiteral("marks.sm2d"));
    const QString source = QDir(QString::fromUtf8(SRCDIR)).absoluteFilePath(
        QStringLiteral("../../app/share/samples/patterns/male_shirt.sm2d"));
    QFile input(source);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QDomDocument document;
    QVERIFY(document.setContent(&input));
    document.documentElement().setAttribute(QStringLiteral("readOnly"), true);
    QFile file(file_path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(document.toByteArray());
    file.close();
    VPatternConverter converter(file_path);
    QFile converted(converter.Convert());
    QVERIFY(converted.open(QIODevice::ReadOnly));
    QVERIFY(document.setContent(&converted));
    QDomElement piece = document.elementsByTagName(QStringLiteral("piece")).at(0).toElement();
    QDomElement block = piece.parentNode().parentNode().toElement();
    QDomElement modeling = block.firstChildElement(QStringLiteral("modeling"));
    QDomElement anchors = piece.firstChildElement(QStringLiteral("anchors"));
    if (anchors.isNull())
    {
        anchors = document.createElement(QStringLiteral("anchors"));
        piece.appendChild(anchors);
    }
    const QDomNodeList source_points = block.firstChildElement(QStringLiteral("calculation"))
                                           .elementsByTagName(QStringLiteral("point"));
    QVERIFY(source_points.size() >= 2);
    for (int i = 0; i < 2; ++i)
    {
        QDomElement point = document.createElement(QStringLiteral("point"));
        point.setAttribute(QStringLiteral("id"), 100000 + i);
        point.setAttribute(QStringLiteral("type"), QStringLiteral("modeling"));
        point.setAttribute(QStringLiteral("idObject"), source_points.at(i).toElement().attribute(QStringLiteral("id")));
        modeling.appendChild(point);
        QDomElement record = document.createElement(QStringLiteral("record"));
        record.appendChild(document.createTextNode(QString::number(100000 + i)));
        anchors.appendChild(record);
    }
    PieceMarkings::write(piece, {{100000, 0.2}}, {{100000, 100001, QStringLiteral("stripe")},
                                                {100001, 100000, QStringLiteral("plaid")}});
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(document.toByteArray());
    file.close();
    VDomDocument::ValidateXML(VPatternConverter::CurrentSchema, file_path);
    quint32 id = 200000;
    const auto copied = PatternPieceCopy::reconstruct(document, piece.attribute(QStringLiteral("id")).toUInt(),
                                                      [&id] { return ++id; });
    QVERIFY2(copied.error.isEmpty(), qPrintable(copied.error));
    QCOMPARE(PieceMarkings::readDrillHoles(copied.piece).first().center, copied.ids.value(100000));
    QCOMPARE(PieceMarkings::readDrillHoles(copied.piece).first().diameter, 0.2);
    QCOMPARE(PieceMarkings::readReferenceLines(copied.piece).first().end, copied.ids.value(100001));
    for (const QString &diameter : {QStringLiteral("0"), QStringLiteral("-1"),
                                   QStringLiteral("INF"), QStringLiteral("NaN")})
    {
        piece.firstChildElement(QStringLiteral("drillHoles")).firstChildElement()
             .setAttribute(QStringLiteral("diameter"), diameter);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write(document.toByteArray());
        file.close();
        QVERIFY_EXCEPTION_THROWN(VDomDocument::ValidateXML(VPatternConverter::CurrentSchema, file_path), VException);
    }
}

void TST_PieceMarkings::editorRequiresExplicitInput()
{
    Unit unit = Unit::Cm;
    VContainer data(nullptr, &unit);
    data.UpdateGObject(7, new VPointF(10, 20, QStringLiteral("A"), 0, 0));
    data.UpdateGObject(8, new VPointF(30, 40, QStringLiteral("B"), 0, 0));
    PieceMarkingsEditor drill(true);
    drill.setPoints({7, 8}, &data);
    auto *add = drill.findChild<QPushButton *>(QStringLiteral("addMarking"));
    QVERIFY(add != nullptr);
    QVERIFY(!add->isEnabled());
    auto *center = drill.findChild<QComboBox *>(QStringLiteral("startAnchor"));
    auto *diameter = drill.findChild<QLineEdit *>(QStringLiteral("drillDiameter"));
    center->setCurrentIndex(1);
    QVERIFY(!add->isEnabled());
    diameter->setText(QStringLiteral("-1"));
    QVERIFY(!add->isEnabled());
    diameter->setText(drill.locale().toString(0.2));
    QVERIFY(add->isEnabled());
    add->click();
    QCOMPARE(drill.drillHoles().size(), 1);
    QVERIFY(diameter->text().isEmpty());
    QVERIFY(!add->isEnabled());
    PieceMarkingsEditor reference(false);
    reference.setPoints({7, 8}, &data);
    auto *add_line = reference.findChild<QPushButton *>(QStringLiteral("addMarking"));
    reference.findChild<QComboBox *>(QStringLiteral("startAnchor"))->setCurrentIndex(1);
    reference.findChild<QComboBox *>(QStringLiteral("endAnchor"))->setCurrentIndex(1);
    QVERIFY(!add_line->isEnabled());
    reference.findChild<QComboBox *>(QStringLiteral("endAnchor"))->setCurrentIndex(2);
    QVERIFY(add_line->isEnabled());
    add_line->click();
    QCOMPARE(reference.referenceLines().first().type, QStringLiteral("stripe"));
    QVERIFY(reference.markingsValid());
    data.UpdateGObject(8, new VPointF(10, 20, QStringLiteral("B"), 0, 0));
    reference.setPoints({7, 8}, &data);
    QVERIFY(!add_line->isEnabled());
    QVERIFY(!reference.markingsValid());
    drill.setPoints({8}, &data);
    QVERIFY(!drill.markingsValid());
}

void TST_PieceMarkings::pieceDialogHasSeparateMarkingPages()
{
    Unit unit = Unit::Cm;
    VContainer data(nullptr, &unit);
    MarkingsTestDocument document;
    VAbstractPattern *previous = qApp->getCurrentDocument();
    const auto restore = qScopeGuard([previous] { qApp->setCurrentDocument(previous); });
    qApp->setCurrentDocument(&document);
    PatternPieceDialog dialog(&data, 0);
    auto *pages = dialog.findChild<QListWidget *>(QStringLiteral("menuTab_ListWidget"));
    QVERIFY(pages != nullptr);
    QCOMPARE(pages->count(), static_cast<int>(TabOrder::Count));
    QCOMPARE(dialog.findChildren<PieceMarkingsEditor *>().size(), 2);
    dialog.enableApply(true);
    QVERIFY(pages->item(TabOrder::DrillHoles)->flags().testFlag(Qt::ItemIsEnabled));
    QVERIFY(pages->item(TabOrder::ReferenceLines)->flags().testFlag(Qt::ItemIsEnabled));
    dialog.enableApply(false);
    QVERIFY(!pages->item(TabOrder::DrillHoles)->flags().testFlag(Qt::ItemIsEnabled));
    QVERIFY(!pages->item(TabOrder::ReferenceLines)->flags().testFlag(Qt::ItemIsEnabled));
    data.UpdateGObject(7, new VPointF(10, 20, QStringLiteral("A"), 0, 0));
    data.UpdateGObject(8, new VPointF(30, 40, QStringLiteral("B"), 0, 0));
    VPiece piece;
    piece.setAnchors({7, 8});
    piece.setDrillHoles({{7, 0.125}});
    piece.setReferenceLines({{7, 8, QStringLiteral("plaid")}});
    dialog.SetPiece(piece);
    const VPiece saved = dialog.GetPiece();
    QCOMPARE(saved.drillHoles().first().center, 7U);
    QCOMPARE(saved.drillHoles().first().diameter, 0.125);
    QCOMPARE(saved.referenceLines().first().end, 8U);
    QCOMPARE(saved.referenceLines().first().type, QStringLiteral("plaid"));
}
