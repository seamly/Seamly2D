//---------------------------------------------------------------------------------------------------------------------
//  @file   tst_piece.cpp
//  @date   9 1, 2016
//
//  @copyright
//  Copyright (C) 2017 - 2026 Seamly, LLC
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

//---------------------------------------------------------------------------------------------------------------------
//  @file   tst_vdetail.cpp
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date   9 1, 2016
//
//  @brief
//  @copyright
//  This source code is part of the Valentina project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2016 Valentina project
//  <https://bitbucket.org/dismine/valentina> All Rights Reserved.
//
//  Valentina is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Valentina is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

#include "tst_vpiece.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vsplinepath.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vtools/tools/new_piece_defaults.h"

#include <QtTest>
#include <QPolygonF>
#include <QTemporaryDir>

#include <limits>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The NotchSettingsGuard class puts back the Default Units and notch size keys when it goes out of scope.
 *
 * The test application reads the developer's real qt6_seamly2d.ini. A key that was absent is removed again.
 * The key names copy the private constants in vcommonsettings.cpp.
 */
class NotchSettingsGuard
{
public:
    NotchSettingsGuard()
        : m_settings(qApp->Settings())
    {
        for (const QString &key : keys())
        {
            if (m_settings->contains(key))
            {
                m_values.insert(key, m_settings->value(key));
            }
        }
    }

    ~NotchSettingsGuard()
    {
        for (const QString &key : keys())
        {
            if (m_values.contains(key))
            {
                m_settings->setValue(key, m_values.value(key));
            }
            else
            {
                m_settings->remove(key);
            }
        }
    }

private:
    Q_DISABLE_COPY(NotchSettingsGuard)

    static QStringList keys()
    {
        return {QStringLiteral("configuration/unit"),
                QStringLiteral("pattern/defaultNotchLength"), QStringLiteral("pattern/defaultNotchWidth"),
                QStringLiteral("pattern/defaultNotchLengthCm"), QStringLiteral("pattern/defaultNotchWidthCm")};
    }

    VCommonSettings          *m_settings;
    QHash<QString, QVariant>  m_values;
};
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_VPiece::TST_VPiece(QObject *parent)
    :AbstractTest(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::ClearLoop()
{
    // Input data taken from real case
    // See file <root>/src/app/share/collection/jacketМ6_30-110.val
    // Check correct seam allowance
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    const NotchSettingsGuard notchSettingsGuard;
    qApp->Settings()->setDefaultNotchLength(.250);
    qApp->Settings()->setDefaultNotchWidth(.250);

    data->UpdateGObject(304, new VPointF(61.866708661417327, 446.92270866141735, "Ф1", 5.0000125984251973,
                                         9.9999874015748045));
    data->UpdateGObject(307, new VPointF(642.96276692900597, 581.21895343695326, "С1", 88.99993700787401,
                                         50.000125984251973));

    data->UpdateGObject(56, new VPointF(802.08718110236236, 850.6707401574804, "Г6", 20.733316535433072,
                                        18.132850393700789));
    data->UpdateGObject(57, new VPointF(690.47666217505162, 804.29700711628709, "З", -11.505637795275591,
                                        31.221543307086616));
    data->UpdateGObject(203, new VPointF(642.96276692900597, 581.21895343695326, "С1", 88.99993700787401,
                                         50.000125984251973));

    QVector<VFSplinePoint> points;

    {
        const QSharedPointer<VPointF> point = data->GeometricObject<VPointF>(203);
        VFSplinePoint p(*point.data(), 0.79455646129695412, 449.62747641208136, 1.6867283804609809, 269.62747641208136);
        points.append(p);
    }

    {
        const QSharedPointer<VPointF> point = data->GeometricObject<VPointF>(57);
        VFSplinePoint p(*point.data(), 0.4456850846354396, 120.24000000000034, 1.0255399999999999, 300.24000000000035);
        points.append(p);
    }

    {
        const QSharedPointer<VPointF> point = data->GeometricObject<VPointF>(56);
        VFSplinePoint p(*point.data(), 1.0085299999999999, 184.58891, 1, 4.5889100000000003);
        points.append(p);
    }

    data->UpdateGObject(308, new VSplinePath(points));

    data->UpdateGObject(309, new VPointF(799.45989815267649, 850.6707401574804, "Г8", -30.431206299212597,
                                         29.487155905511813));
    data->UpdateGObject(310, new VPointF(802.08718110236236, 1653.9337322834645, "Н5", 5.0000125984251973,
                                         9.9999874015748045));

    VPiece piece;
    piece.SetSeamAllowance(true);
    piece.SetSAWidth(7);
    piece.GetPath().Append(VPieceNode(304, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(307, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(308, Tool::NodeSplinePath));
    piece.GetPath().Append(VPieceNode(309, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(310, Tool::NodePoint));
    // Closed
    piece.GetPath()[0].setBeforeSAFormula("0");
    piece.GetPath()[piece.GetPath().nodeCount()-1].setAfterSAFormula("0");

    const QVector<QPointF> pointsEkv = piece.seamAllowancePoints(data.data());

    QVector<QPointF> origPoints;
    origPoints.append(QPointF(42.46405659601932, 415.2845470563871));
    origPoints.append(QPointF(669.4711112822802, 560.1912138528764));
    origPoints.append(QPointF(669.3860586912449, 594.8702688224456));
    origPoints.append(QPointF(669.8537241707239, 619.8499975627876));
    origPoints.append(QPointF(670.904422406071, 642.3178846003559));
    origPoints.append(QPointF(672.4760946214147, 662.4793325519112));
    origPoints.append(QPointF(674.5043075280212, 680.4882882996908));
    origPoints.append(QPointF(676.9236185537709, 696.5023899408525));
    origPoints.append(QPointF(679.6685049649096, 710.6850434378523));
    origPoints.append(QPointF(682.6751345782424, 723.2078546770477));
    origPoints.append(QPointF(685.8841825335202, 734.2530219317046));
    origPoints.append(QPointF(689.2446146317781, 744.0149891243127));
    origPoints.append(QPointF(692.7177992446996, 752.7004886151328));
    origPoints.append(QPointF(696.2448548679188, 760.4478278509594));
    origPoints.append(QPointF(701.8005387196029, 771.2301295961994));
    origPoints.append(QPointF(709.4908502689113, 784.4621360005407));
    origPoints.append(QPointF(713.2090350731621, 790.7616409320319));
    origPoints.append(QPointF(715.0121915355467, 793.763727920337));
    origPoints.append(QPointF(718.7808834775552, 799.1742815201673));
    origPoints.append(QPointF(722.5723522600899, 803.7317522627161));
    origPoints.append(QPointF(726.4900810611796, 807.6675956080389));
    origPoints.append(QPointF(730.558043384579, 811.0692054929614));
    origPoints.append(QPointF(734.8172463181712, 814.0137888810656));
    origPoints.append(QPointF(739.318992665584, 816.5616228424284));
    origPoints.append(QPointF(744.1159693320302, 818.7532201983325));
    origPoints.append(QPointF(749.2539447976853, 820.6109034502547));
    origPoints.append(QPointF(754.7662623591739, 822.1435546205067));
    origPoints.append(QPointF(760.6718473722125, 823.3525044481979));
    origPoints.append(QPointF(766.9761113390083, 824.236813134474));
    origPoints.append(QPointF(773.6735265709667, 824.7970381873482));
    origPoints.append(QPointF(780.6615727577812, 825.0343457026618));
    origPoints.append(QPointF(792.109995909239, 824.8480813766124));
    origPoints.append(QPointF(825.8211754072381, 821.4551806381257));
    origPoints.append(QPointF(828.6858753986579, 1697.305833468011));
    origPoints.append(QPointF(42.46405659601932, 415.2845470563871));

    // Begin comparison
    Comparison(pointsEkv, origPoints);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::Issue620()
{
    // See file <root>/src/app/share/collection/bugs/Issue_#620.vit
    // Check main path
    const Unit unit = Unit::Cm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    data->UpdateGObject(1, new VPointF(30, 39.999874015748034, "A", 5.0000125984251973, 9.9999874015748045));
    data->UpdateGObject(2, new VPointF(333.80102715408322, 37.242158125518621, "A1", 5.0000125984251973,
                                       9.9999874015748045));
    data->UpdateGObject(3, new VPointF(345.43524385831239, 572.57275904711241, "A2", 5.0000125984251973,
                                       9.9999874015748045));
    VPointF *p4 = new VPointF(-43.770684129917051, 567.84465074396087, "A3", 5.0000125984251973,
                              9.9999874015748045);
    data->UpdateGObject(4, p4);

    VPointF *p5 = new VPointF(101.73836126698214, 289.83563666815587, "A4", 5.0000125984251973, 9.9999874015748045);
    data->UpdateGObject(5, p5);
    data->UpdateGObject(6, new VPointF(34.070501467722302, 568.79027240459118, "A5", 5.0000125984251973,
                                       9.9999874015748045));

    QVector<VSplinePoint> points;

    {
        const QSharedPointer<VPointF> point = data->GeometricObject<VPointF>(6);
        VSplinePoint p(*point.data(), 239.37700000000001, "239.377", 419.37700000000001, "59.3765",
                       0, "0", 109.55943307086613, "2.89876");
        points.append(p);
    }

    {
        const QSharedPointer<VPointF> point = data->GeometricObject<VPointF>(5);
        VSplinePoint p(*point.data(), 273.97199999999998, "273.972", 453.97199999999998, "93.9724",
                       88.161637795275595, "2.33261", 56.135055118110238, "1.48524");
        points.append(p);
    }

    {
        const QSharedPointer<VPointF> point = data->GeometricObject<VPointF>(1);
        VSplinePoint p(*point.data(), 337.32600000000002, "337.326", 157.32599999999999, "157.326",
                       71.189669291338589, "1.88356", 50.093858267716534, "1.3254");
        points.append(p);
    }

    data->UpdateGObject(7, new VSplinePath(points));

    data->UpdateGObject(8, new VSpline(*p4, *p5, 59.932499999999997, "59.9325", 257.56999999999999,
                                       "257.57", 170.46425196850396, "4.5102", 150.6164409448819,
                                       "3.98506"));

    VPiece piece;
    piece.SetSeamAllowance(false);
    piece.SetSAWidth(7);
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(2, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(4, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(8, Tool::NodeSpline));
    piece.GetPath().Append(VPieceNode(7, Tool::NodeSplinePath));

    const QVector<QPointF> pointsEkv = piece.mainPathPoints(data.data());

    QVector<QPointF> origPoints;

    origPoints.append(QPointF(30.0, 39.999874015748034));
    origPoints.append(QPointF(333.8010271540832, 37.24215812551862));
    origPoints.append(QPointF(345.4352438583124, 572.5727590471124));
    origPoints.append(QPointF(-43.77068412991705, 567.8446507439609));
    origPoints.append(QPointF(-35.87661251446703, 554.334665281764));
    origPoints.append(QPointF(-21.170117796937717, 530.280231438072));
    origPoints.append(QPointF(-7.679257977133303, 509.352260960953));
    origPoints.append(QPointF(4.687459520857305, 491.0705525945524));
    origPoints.append(QPointF(21.441835284502346, 467.3785713839818));
    origPoints.append(QPointF(40.52832726053379, 440.9303122452387));
    origPoints.append(QPointF(52.86115234049136, 422.54889846842264));
    origPoints.append(QPointF(60.404360001732, 410.06053639339046));
    origPoints.append(QPointF(67.46387337253645, 396.85702763809604));
    origPoints.append(QPointF(74.13118502881582, 382.4581709466849));
    origPoints.append(QPointF(80.49778754648119, 366.3837650633026));
    origPoints.append(QPointF(86.65517350144364, 348.15360873209477));
    origPoints.append(QPointF(92.69483546961428, 327.287500697207));
    origPoints.append(QPointF(98.70826602690417, 303.30523970278495));
    origPoints.append(QPointF(101.73836126698214, 289.83563666815587));
    origPoints.append(QPointF(100.33414592841483, 265.38578532087524));
    origPoints.append(QPointF(96.68026149584162, 212.73003048920282));
    origPoints.append(QPointF(93.50326130663731, 183.65468712210458));
    origPoints.append(QPointF(90.71558574640727, 163.99193793167092));
    origPoints.append(QPointF(87.25149211813451, 144.48122290748523));
    origPoints.append(QPointF(83.01539723892088, 125.48779016127345));
    origPoints.append(QPointF(77.91171792586823, 107.37688780476151));
    origPoints.append(QPointF(73.4247676790375, 94.64014475361638));
    origPoints.append(QPointF(70.13263046316489, 86.58901147237839));
    origPoints.append(QPointF(66.56983159845231, 78.96396285141125));
    origPoints.append(QPointF(62.72442318703749, 71.81065490468066));
    origPoints.append(QPointF(58.58445733105817, 65.17474364615236));
    origPoints.append(QPointF(54.13798613265208, 59.10188508979212));
    origPoints.append(QPointF(49.373061693956934, 53.63773524956568));
    origPoints.append(QPointF(44.277736117110486, 48.827950139438784));
    origPoints.append(QPointF(38.84006150425045, 44.71818577337716));
    origPoints.append(QPointF(33.04808995751456, 41.35409816534657));

    // Begin comparison
    Comparison(pointsEkv, origPoints);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::AutoNameFirstUnnamedPiece()
{
    const Unit unit = Unit::Cm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));

    VPiece piece;
    const quint32 id = data->AddPiece(piece);

    QCOMPARE(data->GetPiece(id).GetName(), QStringLiteral("Piece 1"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::AutoNameSecondUnnamedPiece()
{
    const Unit unit = Unit::Cm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));

    data->AddPiece(VPiece());
    const quint32 secondId = data->AddPiece(VPiece());

    QCOMPARE(data->GetPiece(secondId).GetName(), QStringLiteral("Piece 2"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::KeepExplicitPieceName()
{
    const Unit unit = Unit::Cm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));

    VPiece piece;
    piece.SetName(QStringLiteral("Bodice Front"));
    const quint32 id = data->AddPiece(piece);

    QCOMPARE(data->GetPiece(id).GetName(), QStringLiteral("Bodice Front"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::AutoNameAvoidsNumberStillInUse()
{
    const Unit unit = Unit::Cm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));

    const quint32 firstId = data->AddPiece(VPiece());
    data->AddPiece(VPiece());
    data->RemovePiece(firstId);

    const quint32 thirdId = data->AddPiece(VPiece());

    // "Piece 1" was removed, but "Piece 2" is still in use, so the next
    // fallback name must not collide with it.
    QCOMPARE(data->GetPiece(thirdId).GetName(), QStringLiteral("Piece 3"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::AutoNameReusesNumberFreedByDeletion()
{
    const Unit unit = Unit::Cm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));

    data->AddPiece(VPiece());
    const quint32 secondId = data->AddPiece(VPiece());
    data->RemovePiece(secondId);

    const quint32 thirdId = data->AddPiece(VPiece());

    // Numbering is derived from names currently in the container, not a
    // running total, so a number freed by deletion is available again.
    QCOMPARE(data->GetPiece(thirdId).GetName(), QStringLiteral("Piece 2"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::PieceLabelRightOfVerticalGrainline() const
{
    const QRectF pieceRect(100, 200, 400, 600);
    const QSizeF labelSize(120, 80);
    const QPointF start = VGrainlineData::centeredStart(pieceRect.center(), 90, 300);
    const QRectF grainline = VGrainlineData::lineRect(start, 90, 300);

    const QRectF label(VPatternLabelData::defaultPieceLabelPos(pieceRect, labelSize, grainline), labelSize);

    QVERIFY(!label.intersects(grainline.adjusted(-1, 0, 1, 0)));
    QCOMPARE(label.left(), pieceRect.center().x() + ToPixel(1, Unit::Cm));
    QCOMPARE(label.center().y(), pieceRect.center().y());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::PieceLabelRightOfSlantedGrainline() const
{
    const QRectF pieceRect(100, 200, 400, 600);
    const QSizeF labelSize(120, 80);
    const QPointF start = VGrainlineData::centeredStart(pieceRect.center(), 45, 300);
    const QRectF grainline = VGrainlineData::lineRect(start, 45, 300);

    const QRectF label(VPatternLabelData::defaultPieceLabelPos(pieceRect, labelSize, grainline), labelSize);

    QVERIFY(!label.intersects(grainline));
    QCOMPARE(label.left(), grainline.right() + ToPixel(1, Unit::Cm));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::PatternLabelLeftOfVerticalGrainline() const
{
    const QRectF pieceRect(100, 200, 400, 600);
    const QSizeF pieceLabelSize(120, 80);
    const QSizeF patternLabelSize(150, 60);
    const QRectF grainline = VGrainlineData::lineRect(VGrainlineData::centeredStart(pieceRect.center(), 90, 300),
                                                      90, 300);
    const QRectF pieceLabel(VPatternLabelData::defaultPieceLabelPos(pieceRect, pieceLabelSize, grainline),
                            pieceLabelSize);

    const QRectF patternLabel(VPatternLabelData::defaultPatternLabelPos(pieceRect, patternLabelSize, grainline),
                              patternLabelSize);

    QCOMPARE(patternLabel.right(), grainline.left() - ToPixel(1, Unit::Cm));
    QCOMPARE(patternLabel.center().y(), pieceRect.center().y());
    QVERIFY(!patternLabel.intersects(pieceLabel));
    QVERIFY(!patternLabel.intersects(grainline.adjusted(-1, 0, 1, 0)));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::PatternLabelLeftOfSlantedGrainline() const
{
    const QRectF pieceRect(100, 200, 400, 600);
    const QSizeF labelSize(150, 60);
    const QRectF grainline = VGrainlineData::lineRect(VGrainlineData::centeredStart(pieceRect.center(), 45, 300),
                                                      45, 300);
    const QRectF pieceLabel(VPatternLabelData::defaultPieceLabelPos(pieceRect, labelSize, grainline), labelSize);

    const QRectF patternLabel(VPatternLabelData::defaultPatternLabelPos(pieceRect, labelSize, grainline), labelSize);

    QCOMPARE(patternLabel.right(), grainline.left() - ToPixel(1, Unit::Cm));
    QVERIFY(!patternLabel.intersects(grainline));
    QVERIFY(!patternLabel.intersects(pieceLabel));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::PatternLabelLeftOfCenterWithoutGrainline() const
{
    const QRectF pieceRect(100, 200, 400, 600);
    const QSizeF labelSize(150, 60);

    const QRectF patternLabel(VPatternLabelData::defaultPatternLabelPos(pieceRect, labelSize, QRectF()), labelSize);

    QCOMPARE(patternLabel.right(), pieceRect.center().x() - ToPixel(1, Unit::Cm));
    QCOMPARE(patternLabel.center().y(), pieceRect.center().y());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::GrainlineKeepsLengthLongerThanTwoArrows() const
{
    QCOMPARE(NewPieceDefaults::fitGrainlineLength(2.0, 0.5), 2.0);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::GrainlineGrowsToFitTwoArrows() const
{
    QCOMPARE(NewPieceDefaults::fitGrainlineLength(0.5, 0.5), 1.05);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::LabelTemplateReadsUserFile() const
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userFile = directory.filePath(QStringLiteral("user_piece_label.xml"));
    QFile file(userFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
               "<template><version>1.0.0</version><lines>"
               "<line alignment=\"4\" bold=\"false\" italic=\"false\" sfIncrement=\"0\" text=\"user line\"/>"
               "</lines></template>");
    file.close();

    const QVector<VLabelTemplateLine> lines =
        NewPieceDefaults::readLabelTemplate(userFile, VCommonSettings::builtInPieceLabelTemplate());

    QCOMPARE(lines.size(), 1);
    QCOMPARE(lines.at(0).line, QStringLiteral("user line"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::LabelTemplateFallsBackToBuiltIn() const
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QVector<VLabelTemplateLine> lines =
        NewPieceDefaults::readLabelTemplate(directory.filePath(QStringLiteral("missing.xml")),
                                            VCommonSettings::builtInPieceLabelTemplate());

    QCOMPARE(lines.size(), 3);
    QCOMPARE(lines.at(0).line, QStringLiteral("%pLetter%"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::LabelTemplateEmptyWithoutAnyFile() const
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QVector<VLabelTemplateLine> lines =
        NewPieceDefaults::readLabelTemplate(directory.filePath(QStringLiteral("missing.xml")),
                                            directory.filePath(QStringLiteral("also_missing.xml")));

    QVERIFY(lines.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::LabelTemplateFilePrefersUserFile() const
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString userFile = directory.filePath(QStringLiteral("user_label.xml"));
    QFile file(userFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    QCOMPARE(NewPieceDefaults::labelTemplateFile(userFile, VCommonSettings::builtInPieceLabelTemplate()), userFile);
    QCOMPARE(NewPieceDefaults::labelTemplateFile(directory.filePath(QStringLiteral("missing.xml")),
                                                 VCommonSettings::builtInPieceLabelTemplate()),
             VCommonSettings::builtInPieceLabelTemplate());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::LabelTemplateFileEmptyWithoutAnyFile() const
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QVERIFY(NewPieceDefaults::labelTemplateFile(directory.filePath(QStringLiteral("missing.xml")),
                                                directory.filePath(QStringLiteral("also_missing.xml"))).isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::SameLabelLinesComparesFormatting() const
{
    const VLabelTemplateLine line{QStringLiteral("%pName%"), true, false, 0, 2};
    VLabelTemplateLine italic = line;
    italic.italic = true;

    QVERIFY(NewPieceDefaults::sameLabelLines({line}, {line}));
    QVERIFY(!NewPieceDefaults::sameLabelLines({line}, {italic}));
    QVERIFY(!NewPieceDefaults::sameLabelLines({line}, {line, line}));
}

namespace
{
/// Seam allowance width of the notch test square, in millimeters.
const qreal notchTestSAWidth = 10;

/**
 * @brief Notch lines of a square piece with one slit notch on its top edge.
 * @param showCutline  the node's cutline notch flag.
 * @param showSeamline the node's seamline notch flag.
 * @param includeCutlineNotches forwarded to VPiece::createNotchLines().
 * @param seamAllowancePoints receives the piece's cutline points.
 */
QVector<QLineF> squareNotchLines(bool showCutline, bool showSeamline, bool includeCutlineNotches,
                                 QVector<QPointF> &seamAllowancePoints)
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(200, 0, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(400, 0, "A3", 0, 0));
    data->UpdateGObject(4, new VPointF(400, 400, "A4", 0, 0));
    data->UpdateGObject(5, new VPointF(0, 400, "A5", 0, 0));

    VPieceNode notchNode(2, Tool::NodePoint);
    notchNode.setNotch(true);
    notchNode.setNotchType(NotchType::Slit);
    notchNode.setNotchSubType(NotchSubType::Straightforward);
    notchNode.setNotchLength(5);
    notchNode.setNotchWidth(5);
    notchNode.setNotchCount(1);
    notchNode.setShowNotch(showCutline);
    notchNode.setShowSeamlineNotch(showSeamline);

    VPiece piece;
    piece.SetSeamAllowance(true);
    piece.SetSAWidth(notchTestSAWidth);
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(notchNode);
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(4, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(5, Tool::NodePoint));

    seamAllowancePoints = piece.seamAllowancePoints(data.data());
    return piece.createNotchLines(data.data(), seamAllowancePoints, includeCutlineNotches);
}

/**
 * @brief Piece with one diamond notch on a concave corner of its top edge.
 *
 * At a corner the notch ends drawn beside the cutline point are off the cutline, so clipping to the cutline
 * changes them.
 */
VPiece concaveDiamondNotchPiece(VContainer *data)
{
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(200, 50, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(400, 0, "A3", 0, 0));
    data->UpdateGObject(4, new VPointF(400, 400, "A4", 0, 0));
    data->UpdateGObject(5, new VPointF(0, 400, "A5", 0, 0));

    VPieceNode notchNode(2, Tool::NodePoint);
    notchNode.setNotch(true);
    notchNode.setNotchType(NotchType::Diamond);
    notchNode.setNotchSubType(NotchSubType::Straightforward);
    notchNode.setNotchLength(5);
    notchNode.setNotchWidth(5);
    notchNode.setNotchCount(1);
    notchNode.setShowNotch(true);
    notchNode.setShowSeamlineNotch(false);

    VPiece piece;
    piece.SetSeamAllowance(true);
    piece.SetSAWidth(notchTestSAWidth);
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(notchNode);
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(4, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(5, Tool::NodePoint));

    // Labels and grainline are not under test; hidden, they need no pattern document.
    piece.GetPatternPieceData().SetVisible(false);
    piece.GetPatternInfo().SetVisible(false);
    piece.GetGrainlineGeometry().SetVisible(false);
    return piece;
}

/**
 * @brief Square piece with one slit notch on its top edge.
 * @param builtIn the seam allowance is built into the main path.
 * @param afterWidth seam allowance formula after the notch node; empty keeps the piece width.
 */
VPiece squareSlitPiece(VContainer *data, bool builtIn, const QString &afterWidth)
{
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(200, 0, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(400, 0, "A3", 0, 0));
    data->UpdateGObject(4, new VPointF(400, 400, "A4", 0, 0));
    data->UpdateGObject(5, new VPointF(0, 400, "A5", 0, 0));

    VPieceNode notchNode(2, Tool::NodePoint);
    notchNode.setNotch(true);
    notchNode.setNotchType(NotchType::Slit);
    notchNode.setNotchSubType(NotchSubType::Straightforward);
    notchNode.setNotchLength(5);
    notchNode.setNotchWidth(5);
    notchNode.setNotchCount(1);
    notchNode.setShowNotch(true);
    notchNode.setShowSeamlineNotch(false);
    if (!afterWidth.isEmpty())
    {
        notchNode.setAfterSAFormula(afterWidth);
    }

    VPiece piece;
    piece.SetSeamAllowance(true);
    piece.SetSeamAllowanceBuiltIn(builtIn);
    piece.SetSAWidth(notchTestSAWidth);
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(notchNode);
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(4, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(5, Tool::NodePoint));

    // Labels and grainline are not under test; hidden, they need no pattern document.
    piece.GetPatternPieceData().SetVisible(false);
    piece.GetPatternInfo().SetVisible(false);
    piece.GetGrainlineGeometry().SetVisible(false);
    return piece;
}

/**
 * @brief Square piece with a slit notch on its top edge and a U notch on its right edge.
 */
VPiece slitAndUNotchPiece(VContainer *data)
{
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(200, 0, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(400, 0, "A3", 0, 0));
    data->UpdateGObject(4, new VPointF(400, 200, "A4", 0, 0));
    data->UpdateGObject(5, new VPointF(400, 400, "A5", 0, 0));
    data->UpdateGObject(6, new VPointF(0, 400, "A6", 0, 0));

    auto notchNode = [](quint32 id, NotchType type)
    {
        VPieceNode node(id, Tool::NodePoint);
        node.setNotch(true);
        node.setNotchType(type);
        node.setNotchSubType(NotchSubType::Straightforward);
        node.setNotchLength(5);
        node.setNotchWidth(5);
        node.setNotchCount(1);
        node.setShowNotch(true);
        node.setShowSeamlineNotch(false);
        return node;
    };

    VPiece piece;
    piece.SetSeamAllowance(true);
    piece.SetSAWidth(notchTestSAWidth);
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(notchNode(2, NotchType::Slit));
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    piece.GetPath().Append(notchNode(4, NotchType::UNotch));
    piece.GetPath().Append(VPieceNode(5, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(6, Tool::NodePoint));

    // Labels and grainline are not under test; hidden, they need no pattern document.
    piece.GetPatternPieceData().SetVisible(false);
    piece.GetPatternInfo().SetVisible(false);
    piece.GetGrainlineGeometry().SetVisible(false);
    return piece;
}

/// True when @p point lies on the polyline @p points.
bool isOnPolyline(const QPointF &point, const QVector<QPointF> &points)
{
    for (int i = 1; i < points.size(); ++i)
    {
        if (VGObject::IsPointOnLineSegment(point, points.at(i - 1), points.at(i)))
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Square piece, 400 px side, whose four corner nodes set the given seam allowance widths.
 * @param pieceWidth piece seam allowance width in millimeters.
 * @param before seam allowance formula before each node; empty keeps the piece width.
 * @param after seam allowance formula after each node; empty keeps the piece width.
 */
VPiece squareWidthPiece(VContainer *data, bool builtIn, qreal pieceWidth, const QString &before,
                        const QString &after)
{
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(400, 0, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(400, 400, "A3", 0, 0));
    data->UpdateGObject(4, new VPointF(0, 400, "A4", 0, 0));

    VPiece piece;
    piece.SetSeamAllowance(true);
    piece.SetSeamAllowanceBuiltIn(builtIn);
    piece.SetSAWidth(pieceWidth);
    for (quint32 id = 1; id <= 4; ++id)
    {
        VPieceNode node(id, Tool::NodePoint);
        if (!before.isEmpty())
        {
            node.setBeforeSAFormula(before);
        }
        if (!after.isEmpty())
        {
            node.setAfterSAFormula(after);
        }
        piece.GetPath().Append(node);
    }

    // Labels and grainline are not under test; hidden, they need no pattern document.
    piece.GetPatternPieceData().SetVisible(false);
    piece.GetPatternInfo().SetVisible(false);
    piece.GetGrainlineGeometry().SetVisible(false);
    return piece;
}

/// Distance from @p point to the nearest edge of the closed polyline @p points.
qreal distanceToClosedPolyline(const QPointF &point, const QVector<QPointF> &points)
{
    qreal distance = std::numeric_limits<qreal>::infinity();
    for (int i = 0; i < points.size(); ++i)
    {
        const QPointF a = points.at(i);
        const QPointF b = points.at((i + 1) % points.size());
        const QPointF ab = b - a;
        const qreal lengthSquared = QPointF::dotProduct(ab, ab);
        const qreal t = lengthSquared > 0 ? qBound(0.0, QPointF::dotProduct(point - a, ab) / lengthSquared, 1.0) : 0;
        distance = qMin(distance, QLineF(point, a + t * ab).length());
    }
    return distance;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::NotchFlagsSelectLine_data() const
{
    QTest::addColumn<bool>("showCutline");
    QTest::addColumn<bool>("showSeamline");
    QTest::addColumn<bool>("showSeamAllowances");
    QTest::addColumn<bool>("includeCutlineNotches");
    QTest::addColumn<int>("cutlineNotches");
    QTest::addColumn<int>("seamlineNotches");

    QTest::newRow("no flag")                   << false << false << true  << true  << 0 << 0;
    QTest::newRow("cutline")                   << true  << false << true  << true  << 1 << 0;
    QTest::newRow("seamline")                  << false << true  << true  << true  << 0 << 1;
    QTest::newRow("both")                      << true  << true  << true  << true  << 1 << 1;
    QTest::newRow("cutline, allowance hidden") << true  << false << false << true  << 1 << 0;
    QTest::newRow("both, cutline omitted")     << true  << true  << true  << false << 0 << 1;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NotchFlagsSelectLine checks that each notch flag puts its notch on its own line, and that the
 * seam allowance view setting does not change the notch data.
 */
void TST_VPiece::NotchFlagsSelectLine() const
{
    QFETCH(bool, showCutline);
    QFETCH(bool, showSeamline);
    QFETCH(bool, showSeamAllowances);
    QFETCH(bool, includeCutlineNotches);
    QFETCH(int, cutlineNotches);
    QFETCH(int, seamlineNotches);

    const bool savedShowSeamAllowances = qApp->Settings()->showSeamAllowances();
    qApp->Settings()->setShowSeamAllowances(showSeamAllowances);

    QVector<QPointF> seamAllowancePoints;
    const QVector<QLineF> notches = squareNotchLines(showCutline, showSeamline, includeCutlineNotches,
                                                           seamAllowancePoints);

    qApp->Settings()->setShowSeamAllowances(savedShowSeamAllowances);

    const QVector<QPointF> seamline{QPointF(0, 0), QPointF(400, 0)};
    const qreal notchLength = ToPixel(5, Unit::Mm);

    int onCutline = 0;
    int onSeamline = 0;
    for (const QLineF &notch : notches)
    {
        QVERIFY(qAbs(notch.length() - notchLength) < 0.01);
        const bool p1OnCut  = isOnPolyline(notch.p1(), seamAllowancePoints);
        const bool p2OnCut  = isOnPolyline(notch.p2(), seamAllowancePoints);
        const bool p1OnSeam = isOnPolyline(notch.p1(), seamline);
        const bool p2OnSeam = isOnPolyline(notch.p2(), seamline);
        if (p1OnCut || p2OnCut)
        {
            ++onCutline;
        }
        else if (p1OnSeam || p2OnSeam)
        {
            ++onSeamline;
        }
    }

    QCOMPARE(notches.size(), cutlineNotches + seamlineNotches);
    QCOMPARE(onCutline, cutlineNotches);
    QCOMPARE(onSeamline, seamlineNotches);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LayoutNotchesMatchCanvas checks that layout and export piece data clip cutline notches to the cutline,
 * as the canvas does.
 */
void TST_VPiece::LayoutNotchesMatchCanvas() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    const VPiece piece = concaveDiamondNotchPiece(data.data());
    const QVector<QPointF> cutline = piece.seamAllowancePoints(data.data());
    const QVector<QLineF> canvasNotches = piece.createNotchLines(data.data(), cutline);

    // Guard: the geometry must make clipping matter, or the comparison below proves nothing.
    QVERIFY(canvasNotches != piece.createNotchLines(data.data()));

    QCOMPARE(VLayoutPiece::Create(piece, data.data()).getNotches(), canvasNotches);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::NotchSeamAllowanceWidths_data() const
{
    QTest::addColumn<bool>("builtIn");
    QTest::addColumn<QString>("afterWidth");
    QTest::addColumn<qreal>("expectedWidth");

    QTest::newRow("piece width")      << false << QString()     << notchTestSAWidth;
    QTest::newRow("built in")         << true  << QString()     << notchTestSAWidth;
    QTest::newRow("wider after node") << false << QString("20") << qreal(20);
    QTest::newRow("built in, wider")  << true  << QString("20") << qreal(20);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NotchSeamAllowanceWidths checks that every notch line gets the seam allowance of its node, built in or
 * not, and that layout piece data keeps it.
 */
void TST_VPiece::NotchSeamAllowanceWidths() const
{
    QFETCH(bool, builtIn);
    QFETCH(QString, afterWidth);
    QFETCH(qreal, expectedWidth);

    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    const VPiece piece = squareSlitPiece(data.data(), builtIn, afterWidth);
    QVector<qreal> widths;
    const QVector<QLineF> notches = piece.createNotchLines(data.data(), piece.seamAllowancePoints(data.data()),
                                                           true, &widths);

    QVERIFY(!notches.isEmpty());
    QCOMPARE(widths.size(), notches.size());
    for (qreal width : widths)
    {
        QVERIFY2(qAbs(width - ToPixel(expectedWidth, unit)) < 0.01, qPrintable(QString::number(width)));
    }
    QCOMPARE(VLayoutPiece::Create(piece, data.data()).getNotchSeamAllowances(), widths);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NotchTypesFollowNodes checks that every notch node gets one type, one width and its line count, and that
 * layout piece data keeps them.
 */
void TST_VPiece::NotchTypesFollowNodes() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    const VPiece piece = slitAndUNotchPiece(data.data());
    QVector<qreal> widths;
    QStringList types;
    QVector<int> lineCounts;
    const QVector<QLineF> notches = piece.createNotchLines(data.data(), piece.seamAllowancePoints(data.data()),
                                                           true, &widths, &types, &lineCounts);

    QCOMPARE(types, QStringList({QStringLiteral("slit"), QStringLiteral("uNotch")}));
    QCOMPARE(widths.size(), 2);
    QCOMPARE(lineCounts.size(), 2);
    QCOMPARE(lineCounts.at(0), 1);
    QVERIFY2(lineCounts.at(1) > 1, qPrintable(QString::number(lineCounts.at(1))));
    QCOMPARE(lineCounts.at(0) + lineCounts.at(1), notches.size());

    const VLayoutPiece layoutPiece = VLayoutPiece::Create(piece, data.data());
    QCOMPARE(layoutPiece.getNotchTypes(), types);
    QCOMPARE(layoutPiece.getNotchSeamAllowances(), widths);
    QCOMPARE(layoutPiece.getNotchLineCounts(), lineCounts);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::DefaultNotchSizeConvertsToUnit_data() const
{
    QTest::addColumn<QString>("defaultUnit");
    QTest::addColumn<qreal>("length");
    QTest::addColumn<qreal>("width");
    QTest::addColumn<QString>("toUnit");
    QTest::addColumn<qreal>("expectedLength");
    QTest::addColumn<qreal>("expectedWidth");

    QTest::newRow("inch to cm") << unitINCH << qreal(0.5)  << qreal(0.25) << unitCM   << qreal(1.27) << qreal(0.635);
    QTest::newRow("inch to mm") << unitINCH << qreal(0.5)  << qreal(0.25) << unitMM   << qreal(12.7) << qreal(6.35);
    QTest::newRow("cm to inch") << unitCM   << qreal(1.27) << qreal(0.635) << unitINCH << qreal(0.5) << qreal(0.25);
    QTest::newRow("same unit")  << unitCM   << qreal(0.25) << qreal(0.1)  << unitCM   << qreal(0.25) << qreal(0.1);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DefaultNotchSizeConvertsToUnit checks that the default notch length and width convert from the Default Units
 * to the requested unit.
 */
void TST_VPiece::DefaultNotchSizeConvertsToUnit() const
{
    QFETCH(QString, defaultUnit);
    QFETCH(qreal, length);
    QFETCH(qreal, width);
    QFETCH(QString, toUnit);
    QFETCH(qreal, expectedLength);
    QFETCH(qreal, expectedWidth);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    VCommonSettings settings(directory.filePath(QStringLiteral("notch.ini")), QSettings::IniFormat);
    settings.SetUnit(defaultUnit);
    settings.setDefaultNotchLength(length);
    settings.setDefaultNotchWidth(width);

    QCOMPARE(settings.getDefaultNotchLength(StrToUnits(toUnit)), expectedLength);
    QCOMPARE(settings.getDefaultNotchWidth(StrToUnits(toUnit)), expectedWidth);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NewNodeNotchSizeUsesPatternUnit checks that a new piece node gets the default notch size in the pattern unit.
 */
void TST_VPiece::NewNodeNotchSizeUsesPatternUnit() const
{
    const NotchSettingsGuard notchSettingsGuard;
    VCommonSettings *settings = qApp->Settings();
    settings->SetUnit(unitINCH);
    settings->setDefaultNotchLength(0.5);
    settings->setDefaultNotchWidth(0.25);
    qApp->setPatternUnit(Unit::Cm);

    const VPieceNode node(1, Tool::NodePoint);
    QCOMPARE(node.getNotchLength(), qreal(1.27));
    QCOMPARE(node.getNotchWidth(), qreal(0.635));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DefaultNotchSizeKeepsSizeAfterUnitChange checks that a Default Units change keeps the physical notch size.
 */
void TST_VPiece::DefaultNotchSizeKeepsSizeAfterUnitChange() const
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    VCommonSettings settings(directory.filePath(QStringLiteral("notch.ini")), QSettings::IniFormat);
    settings.SetUnit(unitINCH);
    settings.setDefaultNotchLength(0.5);
    settings.setDefaultNotchWidth(0.25);

    settings.SetUnit(unitCM);

    QCOMPARE(settings.getDefaultNotchLength(), qreal(1.27));
    QCOMPARE(settings.getDefaultNotchWidth(), qreal(0.635));
    QCOMPARE(settings.value(QStringLiteral("pattern/defaultNotchLengthCm")).toReal(), qreal(1.27));
    QCOMPARE(settings.value(QStringLiteral("pattern/defaultNotchWidthCm")).toReal(), qreal(0.635));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DefaultNotchSizeMovesLegacyValueToCm checks that a legacy value reads in the Default Units, and that a
 * Default Units change moves it to the cm key with its physical size.
 */
void TST_VPiece::DefaultNotchSizeMovesLegacyValueToCm() const
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    VCommonSettings settings(directory.filePath(QStringLiteral("notch.ini")), QSettings::IniFormat);
    settings.setValue(QStringLiteral("configuration/unit"), unitINCH);
    settings.setValue(QStringLiteral("pattern/defaultNotchLength"), 0.5);
    settings.setValue(QStringLiteral("pattern/defaultNotchWidth"), 0.25);

    QCOMPARE(settings.getDefaultNotchLength(), qreal(0.5));
    QCOMPARE(settings.getDefaultNotchWidth(), qreal(0.25));

    settings.SetUnit(unitCM);

    QCOMPARE(settings.getDefaultNotchLength(), qreal(1.27));
    QCOMPARE(settings.getDefaultNotchWidth(), qreal(0.635));
    QVERIFY(!settings.contains(QStringLiteral("pattern/defaultNotchLength")));
    QVERIFY(!settings.contains(QStringLiteral("pattern/defaultNotchWidth")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LayoutPieceKeepsQuantityAndFold checks that the layout piece keeps the label data cut count and fold flag.
 */
void TST_VPiece::LayoutPieceKeepsQuantityAndFold() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    VPiece piece = squareSlitPiece(data.data(), false, QString());
    piece.GetPatternPieceData().SetQuantity(4);
    piece.GetPatternPieceData().SetOnFold(true);

    const VLayoutPiece layoutPiece = VLayoutPiece::Create(piece, data.data());
    QCOMPARE(layoutPiece.getQuantity(), 4);
    QVERIFY(layoutPiece.isOnFold());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::BuiltInSeamLineUsesWiderNodeWidth_data() const
{
    QTest::addColumn<qreal>("pieceWidth");
    QTest::addColumn<QString>("before");
    QTest::addColumn<QString>("after");

    // Every row offsets the main path by 10 mm.
    QTest::newRow("piece width")  << qreal(10) << QString()     << QString();
    QTest::newRow("wider after")  << qreal(30) << QString("5")  << QString("10");
    QTest::newRow("wider before") << qreal(30) << QString("10") << QString("5");
    QTest::newRow("after unset")  << qreal(10) << QString("5")  << QString();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief BuiltInSeamLineUsesWiderNodeWidth checks that the seam line of a built-in seam allowance is the main path
 * offset inward by the wider of each node's before and after widths, the piece width when the node sets none.
 */
void TST_VPiece::BuiltInSeamLineUsesWiderNodeWidth() const
{
    QFETCH(qreal, pieceWidth);
    QFETCH(QString, before);
    QFETCH(QString, after);

    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    const VPiece piece = squareWidthPiece(data.data(), true, pieceWidth, before, after);
    QVector<QPointF> nodeVertices;
    const QVector<QPointF> seamLine = piece.builtInSeamLinePoints(data.data(), &nodeVertices);

    const qreal w = ToPixel(10, unit);
    const QVector<QPointF> corners {QPointF(w, w), QPointF(400 - w, w), QPointF(400 - w, 400 - w),
                                    QPointF(w, 400 - w)};
    auto nearAny = [](const QPointF &p, const QVector<QPointF> &points)
    {
        for (const QPointF &q : points)
        {
            if (QLineF(p, q).length() < 0.01)
            {
                return true;
            }
        }
        return false;
    };
    QVERIFY(!seamLine.isEmpty());
    for (const QPointF &p : seamLine)
    {
        QVERIFY2(nearAny(p, corners), qPrintable(QStringLiteral("%1, %2").arg(p.x()).arg(p.y())));
    }
    for (const QPointF &corner : corners)
    {
        QVERIFY(nearAny(corner, seamLine));
        QVERIFY(nearAny(corner, nodeVertices));
    }
    QCOMPARE(VAbstractPiece::isClockwise(seamLine), VAbstractPiece::isClockwise(piece.mainPathPoints(data.data())));

    QCOMPARE(VLayoutPiece::Create(piece, data.data()).getBuiltInSeamLinePoints().size(), 4);
    QVERIFY(squareWidthPiece(data.data(), false, pieceWidth, before, after)
                .builtInSeamLinePoints(data.data()).isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief BuiltInSeamLineFollowsCurve checks that the seam line of a built-in seam allowance stays the piece width
 * inside a main path with a curve.
 */
void TST_VPiece::BuiltInSeamLineFollowsCurve() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(400, 0, "A2", 0, 0));
    VPointF *p3 = new VPointF(400, 400, "A3", 0, 0);
    data->UpdateGObject(3, p3);
    VPointF *p4 = new VPointF(0, 400, "A4", 0, 0);
    data->UpdateGObject(4, p4);
    // The bottom edge bulges down, away from the piece.
    data->UpdateGObject(5, new VSpline(*p3, QPointF(300, 550), QPointF(100, 550), *p4));

    VPiece piece;
    piece.SetSeamAllowance(true);
    piece.SetSeamAllowanceBuiltIn(true);
    piece.SetSAWidth(10);
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(2, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(5, Tool::NodeSpline));
    piece.GetPath().Append(VPieceNode(4, Tool::NodePoint));

    const QVector<QPointF> cutLine = piece.mainPathPoints(data.data());
    const QVector<QPointF> seamLine = piece.builtInSeamLinePoints(data.data());
    QVERIFY2(seamLine.size() > cutLine.size() / 2,
             qPrintable(QStringLiteral("%1 seam line points, %2 cut line points").arg(seamLine.size())
                        .arg(cutLine.size())));

    const qreal w = ToPixel(10, unit);
    const QPolygonF cutPolygon(cutLine);
    for (const QPointF &p : seamLine)
    {
        const qreal distance = distanceToClosedPolyline(p, cutLine);
        QVERIFY2(cutPolygon.containsPoint(p, Qt::OddEvenFill),
                 qPrintable(QStringLiteral("%1, %2 is outside").arg(p.x()).arg(p.y())));
        QVERIFY2(qAbs(distance - w) < w * 0.01,
                 qPrintable(QStringLiteral("%1, %2 is %3 from the cut line").arg(p.x()).arg(p.y()).arg(distance)));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CurveEndInNodePixelEndsAtNode checks that a curve whose end lies past the next node, in the same pixel,
 * ends at that node, so neither the seam line nor the cut line turns back.
 */
void TST_VPiece::CurveEndInNodePixelEndsAtNode() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    VPointF *start = new VPointF(0, 0, "A1", 0, 0);
    data->UpdateGObject(1, start);
    // The curve end lies 0.85 px past the corner node A3; both round to the pixel (200, 0).
    VPointF *curveEnd = new VPointF(200.45, 0, "A2", 0, 0);
    data->UpdateGObject(2, curveEnd);
    data->UpdateGObject(3, new VSpline(*start, QPointF(70, 0), QPointF(140, 0), *curveEnd));
    data->UpdateGObject(4, new VPointF(199.6, 0, "A3", 0, 0));
    data->UpdateGObject(5, new VPointF(199.6, 100, "A4", 0, 0));
    data->UpdateGObject(6, new VPointF(0, 100, "A5", 0, 0));

    VPiece piece;
    piece.SetSeamAllowance(true);
    piece.SetSAWidth(10);
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(3, Tool::NodeSpline));
    piece.GetPath().Append(VPieceNode(4, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(5, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(6, Tool::NodePoint));

    // Largest direction change between the edges of a closed polyline, in degrees.
    const auto largestTurn = [](const QVector<QPointF> &points)
    {
        qreal largest = 0;
        const int n = points.size();
        for (int i = 0; i < n; ++i)
        {
            const QLineF in(points.at((i + n - 1) % n), points.at(i));
            const QLineF out(points.at(i), points.at((i + 1) % n));
            if (in.length() > 0 && out.length() > 0)
            {
                const qreal turn = in.angleTo(out);
                largest = qMax(largest, turn > 180 ? 360 - turn : turn);
            }
        }
        return largest;
    };

    const QVector<QPointF> seamLine = piece.mainPathPoints(data.data());
    QVERIFY2(seamLine.contains(QPointF(199.6, 0)), "the seam line has the corner node");
    QVERIFY2(!seamLine.contains(QPointF(200.45, 0)), "the seam line does not reach the curve end");
    QVERIFY2(largestTurn(seamLine) < 170,
             qPrintable(QStringLiteral("the seam line turns %1 degrees").arg(largestTurn(seamLine))));

    const QVector<QPointF> cutLine = piece.seamAllowancePoints(data.data());
    QVERIFY2(largestTurn(cutLine) < 170,
             qPrintable(QStringLiteral("the cut line turns %1 degrees").arg(largestTurn(cutLine))));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ForcedCornerCutCutsShortMiter checks that a forced corner cut cuts a ByLength corner that is kept by
 * default, and that seamAllowancePoints() reports the corners it cut.
 */
void TST_VPiece::ForcedCornerCutCutsShortMiter() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    // Equilateral triangle, clockwise on screen. Every corner turns 120 degrees, so its miter is twice the width:
    // longer than the width, shorter than the default cut length.
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(400, 0, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(200, 346.41, "A3", 0, 0));

    VPiece piece;
    piece.SetSeamAllowance(true);
    piece.SetSAWidth(10);
    piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(2, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));

    SeamAllowanceCorners corners;
    const QVector<QPointF> kept = piece.seamAllowancePoints(data.data(), nullptr, &corners);
    QCOMPARE(corners.count, 3);
    QVERIFY2(corners.cut.isEmpty(), "no corner is cut by default");

    piece.setForcedCornerCuts({1});
    const QVector<QPointF> cut = piece.seamAllowancePoints(data.data(), nullptr, &corners);
    QCOMPARE(corners.count, 3);
    QCOMPARE(corners.cut, QVector<int>{1});
    QCOMPARE(cut.size(), kept.size() + 1);
}
