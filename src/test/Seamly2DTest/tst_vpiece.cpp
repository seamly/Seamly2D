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
#include "../vpatterndb/PieceMirrorPath.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../ifc/exception/vexception.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vtools/tools/new_piece_defaults.h"

#include <QtTest>
#include <QFile>
#include <QPolygonF>
#include <QTemporaryDir>
#include <QDomDocument>

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

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief threePointSplinePath builds a spline path from (0, 0) through the middle point (200, 100) to (400, 0).
 * @param angle1  angle of the control node before the middle point.
 * @param angle2  angle of the control node after the middle point.
 * @param length2 length of the control node after the middle point.
 */
VSplinePath *threePointSplinePath(qreal angle1, qreal angle2, qreal length2)
{
    QVector<VSplinePoint> points;
    points.append(VSplinePoint(VPointF(0, 0, "A1", 0, 0), 135, "135", 315, "315", 0, "0", 60, "60"));
    points.append(VSplinePoint(VPointF(200, 100, "A2", 0, 0), angle1, QString::number(angle1), angle2,
                               QString::number(angle2), 60, "60", length2, QString::number(length2)));
    points.append(VSplinePoint(VPointF(400, 0, "A3", 0, 0), 225, "225", 45, "45", 60, "60", 0, "0"));
    return new VSplinePath(points);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief hasVertexNear tells whether a vertex lies within 0.01 px of @p p.
 */
bool hasVertexNear(const QVector<QPointF> &vertices, const QPointF &p)
{
    for (const QPointF &vertex : vertices)
    {
        if (QLineF(vertex, p).length() < 0.01)
        {
            return true;
        }
    }
    return false;
}
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
void TST_VPiece::FoldLineNodeIndexes_data() const
{
    QTest::addColumn<QVector<int>>("foldNodes");
    QTest::addColumn<QVector<int>>("excludedNodes");
    QTest::addColumn<int>("curveNode");
    QTest::addColumn<QVector<int>>("expected");

    QTest::newRow("no fold line")    << QVector<int>() << QVector<int>() << -1 << QVector<int>();
    QTest::newRow("one node")        << QVector<int>{1} << QVector<int>() << -1 << QVector<int>();
    QTest::newRow("neighbours")      << QVector<int>{1, 2} << QVector<int>() << -1 << QVector<int>{1, 2};
    QTest::newRow("closing segment") << QVector<int>{0, 4} << QVector<int>() << -1 << QVector<int>{4, 0};
    QTest::newRow("not neighbours")  << QVector<int>{0, 2} << QVector<int>() << -1 << QVector<int>();
    QTest::newRow("three nodes")     << QVector<int>{1, 2, 3} << QVector<int>() << -1 << QVector<int>();
    QTest::newRow("across excluded") << QVector<int>{1, 3} << QVector<int>{2} << -1 << QVector<int>{1, 3};
    QTest::newRow("curve node")      << QVector<int>{1, 2} << QVector<int>() << 2 << QVector<int>();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FoldLineNodeIndexes checks that only two neighbouring point nodes make a fold line.
 */
void TST_VPiece::FoldLineNodeIndexes() const
{
    QFETCH(QVector<int>, foldNodes);
    QFETCH(QVector<int>, excludedNodes);
    QFETCH(int, curveNode);
    QFETCH(QVector<int>, expected);

    QVector<VPieceNode> nodes;
    for (int i = 0; i < 5; ++i)
    {
        VPieceNode node(static_cast<quint32>(i + 1), i == curveNode ? Tool::NodeSpline : Tool::NodePoint);
        node.setFoldLine(foldNodes.contains(i));
        node.SetExcluded(excludedNodes.contains(i));
        nodes.append(node);
    }

    QCOMPARE(VPiece::foldLineNodeIndexes(nodes), expected);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FoldLineHasNoSeamAllowance checks that the cut line runs on the fold line and keeps its width elsewhere.
 */
void TST_VPiece::FoldLineHasNoSeamAllowance() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    // Fold line on the left edge, from A5 (0, 400) to A1 (0, 0): the closing segment.
    VPiece piece = squareSlitPiece(data.data(), false, QString());
    QVector<VPieceNode> nodes = piece.GetPath().getNodes();
    nodes[0].setFoldLine(true);
    nodes[4].setFoldLine(true);
    piece.GetPath().setNodes(nodes);

    const QVector<QPointF> cutline = piece.seamAllowancePoints(data.data());
    QVERIFY(!cutline.isEmpty());

    qreal left = cutline.first().x();
    qreal top  = cutline.first().y();
    for (const QPointF &point : cutline)
    {
        left = qMin(left, point.x());
        top  = qMin(top, point.y());
    }
    QVERIFY2(qAbs(left) < 0.01, qPrintable(QString::number(left)));
    QVERIFY2(qAbs(top + ToPixel(notchTestSAWidth, unit)) < 0.01, qPrintable(QString::number(top)));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LayoutPieceTakesFoldLine checks that the layout piece gets the fold line and the on fold flag.
 */
void TST_VPiece::LayoutPieceTakesFoldLine() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    VPiece piece = squareSlitPiece(data.data(), false, QString());
    QVERIFY(!VLayoutPiece::Create(piece, data.data()).isOnFold());

    QVector<VPieceNode> nodes = piece.GetPath().getNodes();
    nodes[3].setFoldLine(true);
    nodes[4].setFoldLine(true);
    piece.GetPath().setNodes(nodes);

    QVERIFY(piece.hasFoldLine());
    QCOMPARE(piece.foldLine(data.data()), QLineF(QPointF(400, 400), QPointF(0, 400)));

    const VLayoutPiece layoutPiece = VLayoutPiece::Create(piece, data.data());
    QVERIFY(layoutPiece.isOnFold());
    QCOMPARE(layoutPiece.getFoldLine(), QVector<QPointF>({QPointF(400, 400), QPointF(0, 400)}));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FoldLineNodeAttributeIsRead checks the pattern file attribute foldLine on a main path node.
 */
void TST_VPiece::FoldLineNodeAttributeIsRead() const
{
    QDomDocument doc;
    QDomElement nodes = doc.createElement(QStringLiteral("nodes"));

    QDomElement folded = doc.createElement(QStringLiteral("node"));
    folded.setAttribute(QStringLiteral("type"), VAbstractPattern::NodePoint);
    folded.setAttribute(QStringLiteral("idObject"), 7);
    folded.setAttribute(VAbstractPattern::AttrNodeFoldLine, QStringLiteral("true"));
    nodes.appendChild(folded);

    QDomElement plain = folded.cloneNode().toElement();
    plain.removeAttribute(VAbstractPattern::AttrNodeFoldLine);
    nodes.appendChild(plain);

    const VPiecePath path = VAbstractPattern::ParsePieceNodes(nodes);
    QCOMPARE(path.nodeCount(), 2);
    QVERIFY(path.at(0).isFoldLine());
    QVERIFY(!path.at(1).isFoldLine());
}

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief mirrorOrderText writes a full piece path as half piece node indexes; "m" marks a mirrored copy.
 */
QString mirrorOrderText(const QVector<PieceMirrorNode> &order)
{
    QStringList parts;
    for (const PieceMirrorNode &entry : order)
    {
        parts.append(QString::number(entry.source_index) + (entry.mirrored ? QStringLiteral("m") : QString()));
    }
    return parts.join(QLatin1Char(' '));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief signedArea returns the shoelace area of a closed polyline; the sign gives its direction.
 */
qreal signedArea(const QVector<QPointF> &points)
{
    qreal sum = 0;
    for (int i = 0; i < points.size(); ++i)
    {
        const QPointF &a = points.at(i);
        const QPointF &b = points.at((i + 1) % points.size());
        sum += a.x() * b.y() - b.x() * a.y();
    }
    return sum / 2;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief fivePointNodes returns five point nodes with object ids 1 to 5.
 */
QVector<VPieceNode> fivePointNodes()
{
    QVector<VPieceNode> nodes;
    for (quint32 id = 1; id <= 5; ++id)
    {
        nodes.append(VPieceNode(id, Tool::NodePoint));
    }
    return nodes;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::PieceMirrorFullPathOrder_data() const
{
    QTest::addColumn<QVector<int>>("foldNodes");
    QTest::addColumn<QVector<int>>("excludedNodes");
    QTest::addColumn<QString>("expected");

    QTest::newRow("inner segment")    << QVector<int>{1, 2} << QVector<int>() << QString("2 3 4 0 1 0m 4m 3m");
    QTest::newRow("closing segment")  << QVector<int>{0, 4} << QVector<int>() << QString("0 1 2 3 4 3m 2m 1m");
    QTest::newRow("excluded node")    << QVector<int>{1, 2} << QVector<int>{4} << QString("2 3 0 1 0m 3m");
    QTest::newRow("across excluded")  << QVector<int>{1, 3} << QVector<int>{2} << QString("3 4 0 1 0m 4m");
    QTest::newRow("triangle")         << QVector<int>{0, 1} << QVector<int>{3, 4} << QString("1 2 0 2m");
    QTest::newRow("fold line only")   << QVector<int>{0, 1} << QVector<int>{2, 3, 4} << QString("");
    QTest::newRow("no fold line")     << QVector<int>() << QVector<int>() << QString("");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceMirrorFullPathOrder checks the node order of a full piece: the half from the fold line end to the
 * fold line start, then the mirrored half back, without excluded nodes.
 */
void TST_VPiece::PieceMirrorFullPathOrder() const
{
    QFETCH(QVector<int>, foldNodes);
    QFETCH(QVector<int>, excludedNodes);
    QFETCH(QString, expected);

    QVector<VPieceNode> nodes = fivePointNodes();
    for (int i = 0; i < nodes.size(); ++i)
    {
        nodes[i].setFoldLine(foldNodes.contains(i));
        nodes[i].SetExcluded(excludedNodes.contains(i));
    }

    QCOMPARE(mirrorOrderText(PieceMirrorPath::fullPathOrder(nodes)), expected);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceMirrorNodeAttributes checks that a mirrored node runs the other way.
 */
void TST_VPiece::PieceMirrorNodeAttributes() const
{
    VPieceNode point(7, Tool::NodePoint);
    point.setBeforeSAFormula(QStringLiteral("5"));
    point.setAfterSAFormula(QStringLiteral("10"));
    point.SetAngleType(PieceNodeAngle::ByFirstEdgeSymmetry);
    point.setNotch(true);
    point.setNotchAngle(30);
    point.setFoldLine(true);

    const VPieceNode mirror = PieceMirrorPath::mirroredNode(point, 17);
    QCOMPARE(mirror.GetId(), quint32(17));
    QCOMPARE(mirror.GetFormulaSABefore(), QStringLiteral("10"));
    QCOMPARE(mirror.GetFormulaSAAfter(), QStringLiteral("5"));
    QVERIFY(mirror.GetAngleType() == PieceNodeAngle::BySecondEdgeSymmetry);
    QVERIFY(mirror.isNotch());
    QCOMPARE(mirror.getNotchAngle(), qreal(-30));
    QVERIFY(!mirror.isFoldLine());

    VPieceNode curve(8, Tool::NodeSpline, false);
    QVERIFY(PieceMirrorPath::mirroredNode(curve, 18).GetReverse());
    curve.SetReverse(true);
    QVERIFY(!PieceMirrorPath::mirroredNode(curve, 18).GetReverse());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceMirrorFoldNodesTakeHalfWidth checks that a fold line node gets the half side seam allowance on both
 * sides, and is no longer a fold line node.
 */
void TST_VPiece::PieceMirrorFoldNodesTakeHalfWidth() const
{
    // Fold line from A2 to A3. A2, the fold line start, has the half before it; A3 has the half after it.
    QVector<VPieceNode> nodes = fivePointNodes();
    nodes[1].setFoldLine(true);
    nodes[1].setBeforeSAFormula(QStringLiteral("5"));
    nodes[2].setFoldLine(true);
    nodes[2].setAfterSAFormula(QStringLiteral("7"));

    const QVector<PieceMirrorNode> order = PieceMirrorPath::fullPathOrder(nodes);
    QCOMPARE(mirrorOrderText(order), QStringLiteral("2 3 4 0 1 0m 4m 3m"));

    const VPieceNode foldStart = PieceMirrorPath::fullPathNode(nodes, order.at(4), 2);
    QCOMPARE(foldStart.GetFormulaSABefore(), QStringLiteral("5"));
    QCOMPARE(foldStart.GetFormulaSAAfter(), QStringLiteral("5"));
    QVERIFY(!foldStart.isFoldLine());

    const VPieceNode foldEnd = PieceMirrorPath::fullPathNode(nodes, order.at(0), 3);
    QCOMPARE(foldEnd.GetFormulaSABefore(), QStringLiteral("7"));
    QCOMPARE(foldEnd.GetFormulaSAAfter(), QStringLiteral("7"));
    QVERIFY(!foldEnd.isFoldLine());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceMirrorBuildsFullOutline checks that the half outline and its mirror image make one closed outline.
 */
void TST_VPiece::PieceMirrorBuildsFullOutline() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    // The left edge, A5 (0, 400) to A1 (0, 0), is the fold line. The right edge is a curve that bulges out.
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(200, 0, "A2", 0, 0));
    VPointF *p3 = new VPointF(400, 0, "A3", 0, 0);
    data->UpdateGObject(3, p3);
    VPointF *p4 = new VPointF(400, 400, "A4", 0, 0);
    data->UpdateGObject(4, p4);
    data->UpdateGObject(5, new VPointF(0, 400, "A5", 0, 0));
    data->UpdateGObject(6, new VSpline(*p3, QPointF(550, 100), QPointF(550, 300), *p4));

    VPiece half;
    half.SetSeamAllowance(true);
    half.SetSAWidth(10);
    VPieceNode foldEnd(1, Tool::NodePoint);
    foldEnd.setFoldLine(true);
    half.GetPath().Append(foldEnd);
    half.GetPath().Append(VPieceNode(2, Tool::NodePoint));
    half.GetPath().Append(VPieceNode(6, Tool::NodeSpline));
    VPieceNode foldStart(5, Tool::NodePoint);
    foldStart.setFoldLine(true);
    half.GetPath().Append(foldStart);
    half.GetPatternPieceData().SetVisible(false);
    half.GetPatternInfo().SetVisible(false);
    half.GetGrainlineGeometry().SetVisible(false);

    const QVector<VPieceNode> nodes = half.GetPath().getNodes();
    const QVector<PieceMirrorNode> order = PieceMirrorPath::fullPathOrder(nodes);
    QCOMPARE(mirrorOrderText(order), QStringLiteral("0 1 2 3 2m 1m"));

    // The mirror operation copies each object off the fold line across it.
    const QLineF axis = half.foldLine(data.data());
    QHash<quint32, quint32> mirrorIds;
    data->UpdateGObject(12, new VPointF(data->GeometricObject<VPointF>(2)->Flip(axis)));
    mirrorIds.insert(2, 12);
    data->UpdateGObject(16, new VSpline(data->GeometricObject<VSpline>(6)->Flip(axis)));
    mirrorIds.insert(6, 16);

    VPiece full = half;
    full.GetPath().Clear();
    for (const PieceMirrorNode &entry : order)
    {
        const quint32 id = nodes.at(entry.source_index).GetId();
        full.GetPath().Append(PieceMirrorPath::fullPathNode(nodes, entry,
                                                            entry.mirrored ? mirrorIds.value(id) : id));
    }
    QVERIFY(!full.hasFoldLine());

    const QVector<QPointF> halfPoints = half.mainPathPoints(data.data());
    const QVector<QPointF> fullPoints = full.mainPathPoints(data.data());

    // Twice the half area, in the same direction.
    const qreal halfArea = signedArea(halfPoints);
    const qreal fullArea = signedArea(fullPoints);
    QVERIFY2(qAbs(fullArea - 2 * halfArea) < qAbs(halfArea) * 0.01,
             qPrintable(QStringLiteral("half %1, full %2").arg(halfArea).arg(fullArea)));

    // Each point has its mirror image on the outline.
    for (const QPointF &p : fullPoints)
    {
        const qreal distance = distanceToClosedPolyline(QPointF(-p.x(), p.y()), fullPoints);
        QVERIFY2(distance < 0.5, qPrintable(QStringLiteral("%1, %2 has no mirror image").arg(p.x()).arg(p.y())));
    }

    // After the fold line start A5 the outline goes on to the end of the mirrored curve.
    int foldStartIndex = -1;
    for (int i = 0; i < fullPoints.size(); ++i)
    {
        if (QLineF(fullPoints.at(i), QPointF(0, 400)).length() < 0.01)
        {
            foldStartIndex = i;
        }
    }
    QVERIFY(foldStartIndex >= 0);
    const QPointF next = fullPoints.at((foldStartIndex + 1) % fullPoints.size());
    QVERIFY2(QLineF(next, QPointF(-400, 400)).length() < 0.01,
             qPrintable(QStringLiteral("%1, %2").arg(next.x()).arg(next.y())));

    QVERIFY(!full.seamAllowancePoints(data.data()).isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceMirrorRecordRanges checks that a custom seam allowance record gets a copy on each half, and that the
 * full path starts where neither copy runs past the last node.
 */
void TST_VPiece::PieceMirrorRecordRanges() const
{
    // Fold line on the closing segment, A5 to A1. The record runs from A1, the fold line end, to A3.
    QVector<VPieceNode> nodes = fivePointNodes();
    nodes[0].setFoldLine(true);
    nodes[4].setFoldLine(true);

    CustomSARecord record;
    record.startPoint = 1;
    record.path       = 20;
    record.endPoint   = 3;
    const QVector<CustomSARecord> records{record};

    // From A1 the mirrored copy would run past the last node, so the full path starts at A4.
    const QVector<PieceMirrorNode> order = PieceMirrorPath::fullPathOrder(nodes, records);
    QCOMPARE(mirrorOrderText(order), QStringLiteral("3 4 3m 2m 1m 0 1 2"));

    const QVector<PieceMirrorRange> ranges = PieceMirrorPath::recordRanges(nodes, order, records);
    QCOMPARE(ranges.size(), 2);

    QCOMPARE(ranges.at(0).record_index, 0);
    QVERIFY(!ranges.at(0).mirrored);
    QCOMPARE(ranges.at(0).start_position, 5); // A1
    QCOMPARE(ranges.at(0).end_position, 7);   // A3

    QCOMPARE(ranges.at(1).record_index, 0);
    QVERIFY(ranges.at(1).mirrored);
    QCOMPARE(ranges.at(1).start_position, 3); // mirrored A3
    QCOMPARE(ranges.at(1).end_position, 5);   // A1, on the fold line
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceMirrorDropsRecordAcrossFold checks that a record across the fold line gets no copy.
 */
void TST_VPiece::PieceMirrorDropsRecordAcrossFold() const
{
    // Fold line from A2 to A3. The first record, A1 to A4, crosses it; the second, A3 to A5, does not.
    QVector<VPieceNode> nodes = fivePointNodes();
    nodes[1].setFoldLine(true);
    nodes[2].setFoldLine(true);

    CustomSARecord across;
    across.startPoint = 1;
    across.path       = 20;
    across.endPoint   = 4;

    CustomSARecord half;
    half.startPoint = 3;
    half.path       = 21;
    half.endPoint   = 5;

    const QVector<CustomSARecord> records{across, half};
    const QVector<PieceMirrorNode> order = PieceMirrorPath::fullPathOrder(nodes, records);
    const QVector<PieceMirrorRange> ranges = PieceMirrorPath::recordRanges(nodes, order, records);

    QCOMPARE(ranges.size(), 2);
    QCOMPARE(ranges.at(0).record_index, 1);
    QCOMPARE(ranges.at(1).record_index, 1);
    for (const PieceMirrorRange &range : ranges)
    {
        QVERIFY(range.start_position < range.end_position);
    }
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

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::SplineNodeVertices_data() const
{
    QTest::addColumn<qreal>("angle1");
    QTest::addColumn<qreal>("angle2");
    QTest::addColumn<qreal>("length2");
    QTest::addColumn<bool>("endNode");

    QTest::newRow("same tangent") << 180.0 << 0.0 << 60.0 << false;
    QTest::newRow("same tangent, angle past 360") << 180.0 << 360.0 << 60.0 << false;
    // VSplinePoint makes the control node angles opposite, so a spline path point is smooth.
    QTest::newRow("angles not opposite") << 180.0 << 30.0 << 60.0 << false;
    QTest::newRow("control node of length 0") << 180.0 << 0.0 << 0.0 << true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SplineNodeVertices checks that a spline end node is a node vertex and a spline interior node is not.
 */
void TST_VPiece::SplineNodeVertices() const
{
    QFETCH(qreal, angle1);
    QFETCH(qreal, angle2);
    QFETCH(qreal, length2);
    QFETCH(bool, endNode);

    const QSharedPointer<VAbstractCurve> curve(threePointSplinePath(angle1, angle2, length2));
    const QVector<QPointF> vertices = VPiecePath::curveNodeVertices(curve, curve->getPoints());

    QVERIFY2(hasVertexNear(vertices, QPointF(0, 0)), "the first point is a node vertex");
    QVERIFY2(hasVertexNear(vertices, QPointF(400, 0)), "the last point is a node vertex");
    QCOMPARE(hasVertexNear(vertices, QPointF(200, 100)), endNode);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::CubicBezierPathNodeVertices_data() const
{
    QTest::addColumn<QPointF>("controlAfter");
    QTest::addColumn<bool>("endNode");

    QTest::newRow("same tangent") << QPointF(260, 100) << false;
    // VCubicBezierPath::GetSpline() turns this control node opposite the one before, so the curve is smooth.
    QTest::newRow("control nodes not opposite") << QPointF(250, 150) << false;
    QTest::newRow("control node of length 0") << QPointF(200, 100) << true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CubicBezierPathNodeVertices checks that a smooth cubic Bezier path point is not a node vertex, and a point
 * with a control node of length 0 is.
 * @details The test reads the curves, not the control nodes as placed.
 */
void TST_VPiece::CubicBezierPathNodeVertices() const
{
    QFETCH(QPointF, controlAfter);
    QFETCH(bool, endNode);

    // From (0, 0) through the middle point (200, 100) to (400, 0). The control node before the middle point is at
    // (140, 100), so the first curve arrives along the x axis.
    const QVector<VPointF> points{VPointF(0, 0, "A1", 0, 0), VPointF(60, 60, "A2", 0, 0),
                                  VPointF(140, 100, "A3", 0, 0), VPointF(200, 100, "A4", 0, 0),
                                  VPointF(controlAfter.x(), controlAfter.y(), "A5", 0, 0),
                                  VPointF(340, 60, "A6", 0, 0), VPointF(400, 0, "A7", 0, 0)};
    const QSharedPointer<VAbstractCurve> curve(new VCubicBezierPath(points));
    const QVector<QPointF> vertices = VPiecePath::curveNodeVertices(curve, curve->getPoints());

    QCOMPARE(hasVertexNear(vertices, QPointF(200, 100)), endNode);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SplineInteriorNodeKeepsNodeCount checks that a piece has the same node count whether or not its curve
 * segment reaches a spline interior node, as when a pattern size moves the node past the segment end.
 */
void TST_VPiece::SplineInteriorNodeKeepsNodeCount() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    VSplinePath *path = threePointSplinePath(180, 0, 60);
    const QVector<QPointF> curvePoints = path->getPoints();
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, path);
    data->UpdateGObject(4, new VPointF(400, 300, "A4", 0, 0));
    data->UpdateGObject(5, new VPointF(0, 300, "A5", 0, 0));

    int middle = -1;
    for (int k = 0; k < curvePoints.size(); ++k)
    {
        if (QLineF(curvePoints.at(k), QPointF(200, 100)).length() < 0.01)
        {
            middle = k;
        }
    }
    QVERIFY2(middle > 1 && middle < curvePoints.size() - 2, "the interior node is inside the curve");

    // Node vertex count and seam allowance corner count when the curve segment ends at segmentEnd.
    const auto nodeCounts = [&data](const QPointF &segmentEnd)
    {
        data->UpdateGObject(3, new VPointF(segmentEnd.x(), segmentEnd.y(), "A3", 0, 0));

        VPiece piece;
        piece.SetSeamAllowance(true);
        piece.SetSAWidth(10);
        piece.GetPath().Append(VPieceNode(1, Tool::NodePoint));
        piece.GetPath().Append(VPieceNode(2, Tool::NodeSplinePath));
        piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
        piece.GetPath().Append(VPieceNode(4, Tool::NodePoint));
        piece.GetPath().Append(VPieceNode(5, Tool::NodePoint));

        SeamAllowanceCorners corners;
        piece.seamAllowancePoints(data.data(), nullptr, &corners);
        return qMakePair(static_cast<int>(piece.mainPathNodeVertices(data.data()).size()), corners.count);
    };

    const QPair<int, int> before = nodeCounts(curvePoints.at(middle / 2));
    const QPair<int, int> after = nodeCounts(curvePoints.at((middle + curvePoints.size()) / 2));
    QCOMPARE(after.first, before.first);
    QCOMPARE(after.second, before.second);
}

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief writeOnePiecePattern writes a read-only pattern file of format 0.7.6 with one piece.
 *
 * A read-only file keeps the converter from writing a backup copy into the user's backup directory.
 * @param path file to write.
 * @param pieceAttributes extra attributes of the piece element, as XML text.
 * @param dataAttributes attributes of the piece label data element, as XML text.
 * @return true when the file was written.
 */
bool writeOnePiecePattern(const QString &path, const QString &pieceAttributes, const QString &dataAttributes)
{
    const QString content = QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<pattern readOnly="true">
    <version>0.7.6</version>
    <unit>cm</unit>
    <description/>
    <notes/>
    <measurements/>
    <variables/>
    <draftBlock name="Block 1">
        <calculation>
            <point id="1" mx="0.1" my="0.2" name="A" type="single" x="0" y="0"/>
        </calculation>
        <modeling/>
        <pieces>
            <piece id="2" name="Back" %1>
                <data letter="A" %2/>
                <nodes>
                    <node idObject="1" type="NodePoint"/>
                </nodes>
            </piece>
        </pieces>
    </draftBlock>
</pattern>
)").arg(pieceAttributes, dataAttributes);

    QFile file(path);
    bool written = false;
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        written = file.write(content.toUtf8()) > 0;
    }
    return written;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief convertedElement converts a pattern file to the current format.
 *
 * The converter validates the result against the current schema and throws VException when it is not valid.
 * The returned element keeps its attributes, but not its place in the document.
 * @param path pattern file of an older format.
 * @param tagName element to return.
 * @return the first tagName element of the converted file; a null element when it cannot be read.
 */
QDomElement convertedElement(const QString &path, const QString &tagName)
{
    VPatternConverter converter(path);
    QFile converted(converter.Convert());
    QDomDocument doc;
    if (converted.open(QIODevice::ReadOnly))
    {
        doc.setContent(&converted);
    }
    return doc.elementsByTagName(tagName).at(0).toElement();
}
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceRotationValuesRoundTrip checks the pattern file values of the piece rotation choices.
 */
void TST_VPiece::PieceRotationValuesRoundTrip() const
{
    const QVector<PieceRotation> rotations{PieceRotation::None, PieceRotation::OneWay, PieceRotation::TwoWay,
                                           PieceRotation::FourWay, PieceRotation::Any};
    for (const PieceRotation rotation : rotations)
    {
        QVERIFY(VPieceLabelData::stringToRotation(VPieceLabelData::rotationToString(rotation)) == rotation);
    }

    QCOMPARE(VPieceLabelData::rotationToString(PieceRotation::TwoWay), QStringLiteral("2-Way"));
    QVERIFY(VPieceLabelData::stringToRotation(QString()) == PieceRotation::None);
    QVERIFY(VPieceLabelData::stringToRotation(QStringLiteral("2-way")) == PieceRotation::None);
    QVERIFY(VPieceLabelData().getRotationWay() == PieceRotation::None);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::ConverterSetsRotationWay_data() const
{
    QTest::addColumn<QString>("oldValue");
    QTest::addColumn<QString>("newValue");

    QTest::newRow("allowed value") << QStringLiteral("2-Way") << QStringLiteral("2-Way");
    QTest::newRow("empty value") << QString() << QStringLiteral("None");
    QTest::newRow("translated value") << QStringLiteral("Aucune") << QStringLiteral("None");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ConverterSetsRotationWay checks that a 0.7.6 file converts to a rotation value the current schema allows.
 */
void TST_VPiece::ConverterSetsRotationWay() const
{
    QFETCH(QString, oldValue);
    QFETCH(QString, newValue);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("rotation.sm2d"));
    QVERIFY(writeOnePiecePattern(path, QString(), QStringLiteral("rotationWay=\"%1\"").arg(oldValue)));

    try
    {
        const QDomElement data = convertedElement(path, QStringLiteral("data"));
        QCOMPARE(data.attribute(VAbstractPattern::AttrRotationWay), newValue);
    }
    catch (VException &error)
    {
        QFAIL(qUtf8Printable(error.ErrorMessage()));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceTiltValuesRoundTrip checks the pattern file values of the tilt directions and the tilt angle range.
 */
void TST_VPiece::PieceTiltValuesRoundTrip() const
{
    const QVector<PieceTilt> tilts{PieceTilt::None, PieceTilt::CounterClockwise, PieceTilt::Clockwise};
    for (const PieceTilt tilt : tilts)
    {
        QVERIFY(VPieceLabelData::stringToTilt(VPieceLabelData::tiltToString(tilt)) == tilt);
    }

    QCOMPARE(VPieceLabelData::tiltToString(PieceTilt::CounterClockwise), QStringLiteral("CCW"));
    QCOMPARE(VPieceLabelData::tiltToString(PieceTilt::Clockwise), QStringLiteral("CW"));
    QVERIFY(VPieceLabelData::stringToTilt(QStringLiteral("CW X")) == PieceTilt::None);

    VPieceLabelData data;
    QVERIFY(data.GetTilt() == PieceTilt::None);
    QCOMPARE(data.getTiltDegrees(), 0);
    data.setTiltDegrees(120);
    QCOMPARE(data.getTiltDegrees(), 90);
    data.setTiltDegrees(-5);
    QCOMPARE(data.getTiltDegrees(), 0);

    QCOMPARE(VPieceLabelData::tiltDisplayText(PieceTilt::Clockwise, 15), QStringLiteral("CW 15"));
    QCOMPARE(VPieceLabelData::tiltDisplayText(PieceTilt::None, 15), QStringLiteral("None"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::ConverterSplitsTilt_data() const
{
    QTest::addColumn<QString>("oldValue");
    QTest::addColumn<QString>("tilt");
    QTest::addColumn<QString>("degrees");

    QTest::newRow("none") << QStringLiteral("None") << QStringLiteral("None") << QString();
    QTest::newRow("empty value") << QString() << QStringLiteral("None") << QString();
    QTest::newRow("translated value") << QStringLiteral("Aucune") << QStringLiteral("None") << QString();
    QTest::newRow("clockwise placeholder") << QStringLiteral("CW X") << QStringLiteral("CW") << QStringLiteral("0");
    QTest::newRow("counterclockwise number") << QStringLiteral("CCW 15") << QStringLiteral("CCW")
                                            << QStringLiteral("15");
    QTest::newRow("angle past 90") << QStringLiteral("CW 120") << QStringLiteral("CW") << QStringLiteral("0");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ConverterSplitsTilt checks that a 0.7.6 tilt text converts to a direction and a tilt angle.
 */
void TST_VPiece::ConverterSplitsTilt() const
{
    QFETCH(QString, oldValue);
    QFETCH(QString, tilt);
    QFETCH(QString, degrees);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("tilt.sm2d"));
    QVERIFY(writeOnePiecePattern(path, QString(), QStringLiteral("tilt=\"%1\"").arg(oldValue)));

    try
    {
        const QDomElement data = convertedElement(path, QStringLiteral("data"));
        QCOMPARE(data.attribute(VAbstractPattern::AttrTilt), tilt);
        QCOMPARE(data.attribute(VAbstractPattern::AttrTiltDegrees), degrees);
    }
    catch (VException &error)
    {
        QFAIL(qUtf8Printable(error.ErrorMessage()));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::ConverterRemovesFoldPosition_data() const
{
    QTest::addColumn<QString>("foldPosition");
    QTest::addColumn<QString>("onFold");

    QTest::newRow("up/down on fold") << QStringLiteral("Up/Down") << QStringLiteral("true");
    QTest::newRow("left/right not on fold") << QStringLiteral("Left/Right") << QStringLiteral("false");
    QTest::newRow("undefined") << QStringLiteral("Undefined") << QStringLiteral("false");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ConverterRemovesFoldPosition checks that a 0.7.6 file loses its fold position and keeps its on fold flag.
 */
void TST_VPiece::ConverterRemovesFoldPosition() const
{
    QFETCH(QString, foldPosition);
    QFETCH(QString, onFold);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("fold.sm2d"));
    QVERIFY(writeOnePiecePattern(path, QString(),
                                 QStringLiteral("foldPosition=\"%1\" onFold=\"%2\"").arg(foldPosition, onFold)));

    try
    {
        const QDomElement data = convertedElement(path, QStringLiteral("data"));
        QVERIFY(!data.isNull());
        QVERIFY(!data.hasAttribute(QStringLiteral("foldPosition")));
        QCOMPARE(data.attribute(VAbstractPattern::AttrOnFold), onFold);
    }
    catch (VException &error)
    {
        QFAIL(qUtf8Printable(error.ErrorMessage()));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceFlipValuesRoundTrip checks the pattern file values of the flip choices and the default.
 */
void TST_VPiece::PieceFlipValuesRoundTrip() const
{
    const QVector<PieceFlip> flips{PieceFlip::None, PieceFlip::Horizontal, PieceFlip::Vertical, PieceFlip::Both};
    for (const PieceFlip flip : flips)
    {
        QVERIFY(stringToPieceFlip(pieceFlipToString(flip)) == flip);
    }

    QCOMPARE(pieceFlipToString(PieceFlip::Horizontal), QStringLiteral("Horizontal"));
    QVERIFY(stringToPieceFlip(QString()) == PieceFlip::None);
    QVERIFY(stringToPieceFlip(QStringLiteral("true")) == PieceFlip::None);

    VPiece piece;
    QVERIFY(piece.getFlip() == PieceFlip::None);
    QVERIFY(piece.IsForbidFlipping());
    piece.setFlip(PieceFlip::Vertical);
    QVERIFY(!piece.IsForbidFlipping());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LayoutPieceKeepsFlip checks that the layout piece keeps the flip choice of the piece.
 */
void TST_VPiece::LayoutPieceKeepsFlip() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);

    VPiece piece = squareSlitPiece(data.data(), false, QString());
    piece.setFlip(PieceFlip::Both);

    const VLayoutPiece layoutPiece = VLayoutPiece::Create(piece, data.data());
    QVERIFY(layoutPiece.getFlip() == PieceFlip::Both);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VPiece::ConverterRemovesForbidFlipping_data() const
{
    QTest::addColumn<QString>("forbidFlipping");

    QTest::newRow("forbidden") << QStringLiteral("true");
    QTest::newRow("allowed") << QStringLiteral("false");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ConverterRemovesForbidFlipping checks that a 0.7.6 piece loses forbidFlipping and gets no flip: None.
 */
void TST_VPiece::ConverterRemovesForbidFlipping() const
{
    QFETCH(QString, forbidFlipping);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("flip.sm2d"));
    QVERIFY(writeOnePiecePattern(path, QStringLiteral("forbidFlipping=\"%1\"").arg(forbidFlipping), QString()));

    try
    {
        const QDomElement piece = convertedElement(path, QStringLiteral("piece"));
        QVERIFY(!piece.isNull());
        QVERIFY(!piece.hasAttribute(QStringLiteral("forbidFlipping")));
        QVERIFY(!piece.hasAttribute(QStringLiteral("flip")));
    }
    catch (VException &error)
    {
        QFAIL(qUtf8Printable(error.ErrorMessage()));
    }
}
