/******************************************************************************
 **  @file   tst_svgcomponenttags.cpp
 **  @author slspencer
 **  @date   July 18, 2026
 **
 **  @brief
 **  Unit tests for the SVG component data-type tagging of piece items
 **  (internal_path vs cut_path).
 **
 **  @copyright
 **  This source code is part of the Seamly2D project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
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
 *****************************************************************************/

#include "tst_svgcomponenttags.h"
#include "../vformat/svg_generator.h"
#include "../vlayout/vlayoutdef.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vlayout/vlayoutpiecepath.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/vcontainer.h"

#include <QtTest>
#include <QDomDocument>
#include <QDomElement>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QLineF>
#include <QPainterPath>
#include <QPen>
#include <QScopedPointer>
#include <QTemporaryDir>
#include <QTransform>

#include <cmath>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief squarePoints builds a closed square contour used as path geometry.
 * @param x    left coordinate.
 * @param y    top coordinate.
 * @param side side length.
 * @return the four corner points of the square.
 */
QVector<QPointF> squarePoints(qreal x, qreal y, qreal side)
{
    QVector<QPointF> points;
    points << QPointF(x, y) << QPointF(x + side, y) << QPointF(x + side, y + side) << QPointF(x, y + side);
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief makePath wraps a point list in a VLayoutPiecePath with test styling.
 * @param points path geometry.
 * @param cut    true to mark the path as a cutout (cut path).
 * @return the layout piece path.
 */
VLayoutPiecePath makePath(const QVector<QPointF> &points, bool cut)
{
    return VLayoutPiecePath(points, QStringLiteral("black"), Qt::SolidLine, QStringLiteral("0.35"), cut);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief makeTestPiece builds a minimal piece: a square contour with a seam
 * allowance, one notch, one plain internal path and two cutout paths.
 * @param name piece name; VAbstractPieceData defaults a piece to the
 *             localized "Piece" when SetName() is never called, so an empty
 *             name must still be set explicitly to test the no-name case.
 * @return the layout piece.
 */
VLayoutPiece makeTestPiece(const QString &name = QStringLiteral("Test Piece"))
{
    VLayoutPiece piece;
    // setMainPathPoints() is the setter behind getContourPoints() — it writes
    // the piece's contour (d->contour) after removing duplicate points.
    piece.setMainPathPoints(squarePoints(10, 10, 180));
    piece.SetName(name);

    piece.setSeamAllowancePoints(squarePoints(5, 5, 190), true, false);
    piece.setNotches({QLineF(QPointF(10, 100), QPointF(15, 100))});

    QVector<VLayoutPiecePath> internalPaths;
    internalPaths.append(makePath(squarePoints(20, 20, 30), false));
    piece.setInternalPaths(internalPaths);

    QVector<VLayoutPiecePath> cutoutPaths;
    cutoutPaths.append(makePath(squarePoints(70, 70, 30), true));
    cutoutPaths.append(makePath(squarePoints(120, 120, 30), true));
    piece.setCutoutPaths(cutoutPaths);

    return piece;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setHiddenGrain gives a piece a grainline that is not drawn.
 * @param piece    piece that receives the grain direction.
 * @param rotation grainline rotation formula in degrees.
 */
void setHiddenGrain(VLayoutPiece &piece, const QString &rotation)
{
    const Unit unit = Unit::Cm;
    const VContainer pattern(nullptr, &unit);

    VGrainlineData grainline;
    grainline.SetVisible(false);
    grainline.setRotation(rotation);
    grainline.setLength(QStringLiteral("10"));
    grainline.setArrowLength(QStringLiteral("0.5"));

    piece.setGrainAxis(grainline, &pattern);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief normalizedDegrees maps an angle into [0, 360).
 * @param degrees angle in degrees.
 * @return the equivalent angle in [0, 360).
 */
qreal normalizedDegrees(qreal degrees)
{
    const qreal result = std::fmod(degrees, 360.0);
    return result < 0 ? result + 360.0 : result;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportItemSvg renders a piece item tree through the real export
 * pipeline (SvgGenerator) into a temporary file and parses the result back.
 * @param item piece root item; the function takes ownership.
 * @return the parsed SVG document; null if export or parsing failed.
 */
QDomDocument exportItemSvg(QGraphicsItem *item)
{
    QTemporaryDir tempDir;
    if (!tempDir.isValid())
    {
        delete item;
        return QDomDocument();
    }
    const QString filePath = tempDir.filePath(QStringLiteral("component_tags.svg"));

    QGraphicsScene scene;
    scene.addItem(item); // scene takes ownership

    QGraphicsRectItem paper(QRectF(0, 0, 400, 400));
    SvgGenerator generator(&paper, filePath, QStringLiteral("Test Pattern"), QString(), 96);
    generator.addSvgFromScene(&scene, item);
    generator.generate();

    QDomDocument doc;
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly))
    {
        doc.setContent(&file);
    }
    return doc;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportPieceSvg renders a piece through the real export pipeline.
 * @param piece piece to export.
 * @return the parsed SVG document; null if export or parsing failed.
 */
QDomDocument exportPieceSvg(const VLayoutPiece &piece)
{
    return exportItemSvg(piece.GetItem(true));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportTwoPiecesSvg renders two pieces through one SvgGenerator
 * instance, the way Layout Mode exports a whole pattern, and parses the
 * merged result back.
 * @param first  first piece (becomes piece-1).
 * @param second second piece (becomes piece-2).
 * @return the parsed merged SVG document; null if export or parsing failed.
 */
QDomDocument exportTwoPiecesSvg(const VLayoutPiece &first, const VLayoutPiece &second)
{
    QTemporaryDir tempDir;
    if (!tempDir.isValid())
    {
        return QDomDocument();
    }
    const QString filePath = tempDir.filePath(QStringLiteral("component_tags.svg"));

    QGraphicsScene scene;
    QGraphicsItem *item1 = first.GetItem(true);
    QGraphicsItem *item2 = second.GetItem(true);
    scene.addItem(item1);
    scene.addItem(item2);

    QGraphicsRectItem paper(QRectF(0, 0, 400, 400));
    SvgGenerator generator(&paper, filePath, QStringLiteral("Test Pattern"), QString(), 96);
    generator.addSvgFromScene(&scene, item1);
    generator.addSvgFromScene(&scene, item2);
    generator.generate();

    QDomDocument doc;
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly))
    {
        doc.setContent(&file);
    }
    return doc;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief groupsOfType collects all <g> elements of the document carrying the
 * given data-type attribute value, in document order.
 * @param doc  parsed SVG document.
 * @param type wanted data-type value.
 * @return the matching group elements.
 */
QVector<QDomElement> groupsOfType(const QDomDocument &doc, const QString &type)
{
    QVector<QDomElement> result;
    const QDomNodeList groups = doc.elementsByTagName(QStringLiteral("g"));
    for (int i = 0; i < groups.size(); ++i)
    {
        const QDomElement group = groups.at(i).toElement();
        if (group.attribute(QStringLiteral("data-type")) == type)
        {
            result.append(group);
        }
    }
    return result;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief TST_SvgComponentTags constructor.
 * @param parent optional owning QObject.
 */
TST_SvgComponentTags::TST_SvgComponentTags(QObject *parent)
    : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CutoutTaggedAsCutPath checks the item-tree contract: cutout paths get
 * the dedicated "cut_path" item type while plain internal paths keep
 * "internal_path".
 */
void TST_SvgComponentTags::CutoutTaggedAsCutPath() const
{
    const VLayoutPiece piece = makeTestPiece();
    const QScopedPointer<QGraphicsItem> root(piece.GetItem(true));

    int internalCount = 0;
    int cutoutCount = 0;
    const QList<QGraphicsItem *> components = root->childItems();
    for (int i = 0; i < components.size(); ++i)
    {
        const QString type = components.at(i)->data(PieceItemData::ItemType).toString();
        if (type == QLatin1String("internal_path"))
        {
            ++internalCount;
        }
        else if (type == QLatin1String("cut_path"))
        {
            ++cutoutCount;
        }
    }

    QCOMPARE(internalCount, 1);
    QCOMPARE(cutoutCount, 2);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ExportedSvgTagsCutPathGroups checks the exported SVG end to end:
 * cutouts become data-type="cut_path" groups with their own per-piece counter
 * and name-based "cut_path_<m>_<pieceName>" ids, plain internal paths keep
 * their independent "internal_path" counter, and every group points back to
 * the piece via data-parent (the piece's name).
 */
void TST_SvgComponentTags::ExportedSvgTagsCutPathGroups() const
{
    const QDomDocument doc = exportPieceSvg(makeTestPiece(QStringLiteral("Test Piece")));
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    // Two cutouts: own counter starting at 1, name-based ids, piece name as parent.
    const QVector<QDomElement> cutouts = groupsOfType(doc, QStringLiteral("cut_path"));
    QCOMPARE(cutouts.size(), 2);
    for (int i = 0; i < cutouts.size(); ++i)
    {
        const QString number = QString::number(i + 1);
        QCOMPARE(cutouts.at(i).attribute(QStringLiteral("id")),
                 QStringLiteral("cut_path_%1_Test_Piece").arg(number));
        QCOMPARE(cutouts.at(i).attribute(QStringLiteral("data-type-number")), number);
        QCOMPARE(cutouts.at(i).attribute(QStringLiteral("data-parent")), QStringLiteral("Test Piece"));
    }

    // The plain internal path keeps its own counter, unaffected by the cutouts.
    const QVector<QDomElement> internals = groupsOfType(doc, QStringLiteral("internal_path"));
    QCOMPARE(internals.size(), 1);
    QCOMPARE(internals.at(0).attribute(QStringLiteral("id")), QStringLiteral("internal_path_1_Test_Piece"));
    QCOMPARE(internals.at(0).attribute(QStringLiteral("data-type-number")), QStringLiteral("1"));
    QCOMPARE(internals.at(0).attribute(QStringLiteral("data-parent")), QStringLiteral("Test Piece"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ExportedSvgNamesSeamlineCutlineAndNotchGroups checks the id scheme
 * for the component types that occur once per piece today: no counter infix,
 * just "<type>_<pieceName>".
 */
void TST_SvgComponentTags::ExportedSvgNamesSeamlineCutlineAndNotchGroups() const
{
    const QDomDocument doc = exportPieceSvg(makeTestPiece(QStringLiteral("Sleeve")));
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> seamlines = groupsOfType(doc, QStringLiteral("seamline"));
    QCOMPARE(seamlines.size(), 1);
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("id")), QStringLiteral("seamline_Sleeve"));
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("data-parent")), QStringLiteral("Sleeve"));

    const QVector<QDomElement> cutlines = groupsOfType(doc, QStringLiteral("cutline"));
    QCOMPARE(cutlines.size(), 1);
    QCOMPARE(cutlines.at(0).attribute(QStringLiteral("id")), QStringLiteral("cutline_Sleeve"));

    const QVector<QDomElement> notches = groupsOfType(doc, QStringLiteral("notch"));
    QCOMPARE(notches.size(), 1);
    QCOMPARE(notches.at(0).attribute(QStringLiteral("id")), QStringLiteral("notch_Sleeve"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceNameSanitizedForIdButNotForDataParent checks that a piece name
 * with spaces, punctuation and a leading digit is sanitized for the id
 * (invalid characters become '_', a leading digit gets a '_' prefix) while
 * data-parent keeps the raw, human-readable name.
 */
void TST_SvgComponentTags::PieceNameSanitizedForIdButNotForDataParent() const
{
    const QDomDocument doc = exportPieceSvg(makeTestPiece(QStringLiteral("2\" Front/Bodice")));
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> seamlines = groupsOfType(doc, QStringLiteral("seamline"));
    QCOMPARE(seamlines.size(), 1);
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("id")), QStringLiteral("seamline__2_Front_Bodice"));
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("data-parent")), QStringLiteral("2\" Front/Bodice"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceWithNoNameFallsBackToNumericId checks that a piece with no
 * usable name keeps the legacy "<pieceId>-<type>-<n>" id and data-parent
 * falls back to the piece id, exactly as before this id scheme existed.
 */
void TST_SvgComponentTags::PieceWithNoNameFallsBackToNumericId() const
{
    const QDomDocument doc = exportPieceSvg(makeTestPiece(QString()));
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> seamlines = groupsOfType(doc, QStringLiteral("seamline"));
    QCOMPARE(seamlines.size(), 1);
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("id")), QStringLiteral("piece-1-seamline-1"));
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("data-parent")), QStringLiteral("piece-1"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NamedPieceGetsNameBasedId checks that the piece's own <g> id follows
 * the same name-based scheme as its components ("piece_<pieceName>"), not
 * the legacy numeric "piece-<n>" form.
 */
void TST_SvgComponentTags::NamedPieceGetsNameBasedId() const
{
    const QDomDocument doc = exportPieceSvg(makeTestPiece(QStringLiteral("Yoke")));
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> pieces = groupsOfType(doc, QStringLiteral("piece"));
    QCOMPARE(pieces.size(), 1);
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("id")), QStringLiteral("piece_Yoke"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CollidingPieceNamesGetDisambiguatingSuffix checks that two pieces
 * sharing a name (piece names aren't guaranteed unique; data-letter is the
 * user-facing disambiguator) still get unique, XML-valid ids: the second
 * occurrence's id is suffixed with its own piece data-type-number, for both
 * the piece's own <g> id and its components' ids.
 */
void TST_SvgComponentTags::CollidingPieceNamesGetDisambiguatingSuffix() const
{
    const QDomDocument doc = exportTwoPiecesSvg(makeTestPiece(QStringLiteral("Facing")),
                                                 makeTestPiece(QStringLiteral("Facing")));
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    // Piece ids themselves get the same collision suffix as component ids.
    const QVector<QDomElement> pieces = groupsOfType(doc, QStringLiteral("piece"));
    QCOMPARE(pieces.size(), 2);
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("id")), QStringLiteral("piece_Facing"));
    QCOMPARE(pieces.at(1).attribute(QStringLiteral("id")), QStringLiteral("piece_Facing-2"));

    const QVector<QDomElement> seamlines = groupsOfType(doc, QStringLiteral("seamline"));
    QCOMPARE(seamlines.size(), 2);
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("id")), QStringLiteral("seamline_Facing"));
    QCOMPARE(seamlines.at(1).attribute(QStringLiteral("id")), QStringLiteral("seamline_Facing-2"));
    // Both still (correctly) name "Facing" as their parent — data-letter, not
    // data-parent, is the user-facing disambiguator for same-named pieces.
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("data-parent")), QStringLiteral("Facing"));
    QCOMPARE(seamlines.at(1).attribute(QStringLiteral("data-parent")), QStringLiteral("Facing"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceWithoutNotchesEmitsNoNotchGroup checks that a piece without
 * notches gets no notch item and no notch group, also when the piece is
 * moved away from the scene origin.
 */
void TST_SvgComponentTags::PieceWithoutNotchesEmitsNoNotchGroup() const
{
    VLayoutPiece piece = makeTestPiece(QStringLiteral("Front"));
    piece.setNotches({});

    QGraphicsItem *item = piece.GetItem(true);
    const QList<QGraphicsItem *> components = item->childItems();
    for (int i = 0; i < components.size(); ++i)
    {
        QVERIFY(components.at(i)->data(PieceItemData::ItemType).toString() != QLatin1String("notch"));
    }

    item->setPos(50, 60);
    const QDomDocument doc = exportItemSvg(item);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");
    QCOMPARE(groupsOfType(doc, QStringLiteral("notch")).size(), 0);
    QCOMPARE(groupsOfType(doc, QStringLiteral("seamline")).size(), 1);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief EmptyComponentAwayFromOriginEmitsNoGroup checks that the export drops
 * a component with an empty path when the item's scene position moves the
 * path's implicit origin point away from (0,0).
 */
void TST_SvgComponentTags::EmptyComponentAwayFromOriginEmitsNoGroup() const
{
    QGraphicsItem *item = makeTestPiece(QStringLiteral("Back")).GetItem(true);

    QGraphicsPathItem *empty = new QGraphicsPathItem(item);
    empty->setData(PieceItemData::ItemType, QStringLiteral("empty_component"));
    empty->setPen(QPen(Qt::black, 1));
    empty->setPath(QPainterPath());
    item->setPos(50, 60);

    const QDomDocument doc = exportItemSvg(item);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");
    QCOMPARE(groupsOfType(doc, QStringLiteral("empty_component")).size(), 0);
    QCOMPARE(groupsOfType(doc, QStringLiteral("seamline")).size(), 1);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief HiddenGrainlineKeepsGrainAngle checks that a grainline that is not
 * drawn still gives the piece a grain direction.
 */
void TST_SvgComponentTags::HiddenGrainlineKeepsGrainAngle() const
{
    VLayoutPiece piece = makeTestPiece();
    setHiddenGrain(piece, QStringLiteral("90"));

    qreal angle = 0;
    QVERIFY(piece.grainlineAngle(angle));
    QVERIFY(qFuzzyCompare(normalizedDegrees(angle), 90.0));

    const QScopedPointer<QGraphicsItem> root(piece.GetItem(true));
    QVERIFY(!root->childItems().isEmpty());
    for (QGraphicsItem *component : root->childItems())
    {
        QVERIFY(component->data(PieceItemData::ItemType).toString() != QLatin1String("grainline"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GrainAngleFollowsPieceTransform checks that a piece turned by the
 * Seamly2D layout reports the turned grain direction.
 */
void TST_SvgComponentTags::GrainAngleFollowsPieceTransform() const
{
    VLayoutPiece piece = makeTestPiece();
    setHiddenGrain(piece, QStringLiteral("90"));

    // QTransform::rotate(90) turns clockwise on screen: grain up becomes grain right.
    QTransform transform;
    transform.rotate(90);
    piece.setTransform(transform);

    qreal angle = -1;
    QVERIFY(piece.grainlineAngle(angle));
    const qreal normalized = normalizedDegrees(angle);
    QVERIFY2(normalized < 0.001 || normalized > 359.999, qPrintable(QString::number(angle)));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ExportedPieceCarriesGrainlineAngle checks the exported piece group
 * carries data-grainline-angle.
 */
void TST_SvgComponentTags::ExportedPieceCarriesGrainlineAngle() const
{
    VLayoutPiece piece = makeTestPiece();
    setHiddenGrain(piece, QStringLiteral("45"));

    const QDomDocument doc = exportPieceSvg(piece);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> pieces = groupsOfType(doc, QStringLiteral("piece"));
    QCOMPARE(pieces.size(), 1);
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("data-grainline-angle")), QStringLiteral("45.0000"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceWithoutGrainHasNoGrainlineAngle checks that a piece without a
 * grain rotation gets no data-grainline-angle attribute.
 */
void TST_SvgComponentTags::PieceWithoutGrainHasNoGrainlineAngle() const
{
    VLayoutPiece piece = makeTestPiece();
    setHiddenGrain(piece, QString());

    qreal angle = 0;
    QVERIFY(!piece.grainlineAngle(angle));

    const QDomDocument doc = exportPieceSvg(piece);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> pieces = groupsOfType(doc, QStringLiteral("piece"));
    QCOMPARE(pieces.size(), 1);
    QVERIFY(!pieces.at(0).hasAttribute(QStringLiteral("data-grainline-angle")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpwardAngleFlipsDownwardGrainline_data lists grainline angles and their upward form.
 */
void TST_SvgComponentTags::UpwardAngleFlipsDownwardGrainline_data() const
{
    QTest::addColumn<qreal>("angle");
    QTest::addColumn<qreal>("expected");

    QTest::newRow("up") << qreal(90) << qreal(90);
    QTest::newRow("horizontal right") << qreal(0) << qreal(0);
    QTest::newRow("horizontal left") << qreal(180) << qreal(180);
    QTest::newRow("down") << qreal(270) << qreal(90);
    QTest::newRow("down right") << qreal(315) << qreal(135);
    QTest::newRow("negative down") << qreal(-90) << qreal(90);
    QTest::newRow("full turn") << qreal(450) << qreal(90);
    QTest::newRow("360 is horizontal") << qreal(360) << qreal(0);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpwardAngleFlipsDownwardGrainline checks that only angles pointing down are turned by 180 degrees.
 */
void TST_SvgComponentTags::UpwardAngleFlipsDownwardGrainline() const
{
    QFETCH(qreal, angle);
    QFETCH(qreal, expected);

    QVERIFY2(qAbs(VGrainlineData::upwardAngle(angle) - expected) < 1e-9,
             qPrintable(QString::number(VGrainlineData::upwardAngle(angle))));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CenteredStartPutsMidpointOnCenter checks the start point of a centered grainline, and that its start
 * point lies below its end point.
 */
void TST_SvgComponentTags::CenteredStartPutsMidpointOnCenter() const
{
    const QPointF center(100, 200);
    const qreal length = 40;

    const QPointF vertical = VGrainlineData::centeredStart(center, 90, length);
    QVERIFY(qAbs(vertical.x() - 100) < 1e-9);
    QVERIFY(qAbs(vertical.y() - 220) < 1e-9);

    const qreal angle = 60;
    const QPointF start = VGrainlineData::centeredStart(center, angle, length);
    const QPointF end(start.x() + length * qCos(qDegreesToRadians(angle)),
                      start.y() - length * qSin(qDegreesToRadians(angle)));
    const QPointF midpoint = (start + end) / 2.0;
    QVERIFY(qAbs(midpoint.x() - center.x()) < 1e-9);
    QVERIFY(qAbs(midpoint.y() - center.y()) < 1e-9);
    QVERIFY(start.y() > end.y());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DownwardGrainAxisPointsUp checks that a grainline without top and bottom anchors exports an upward
 * grain direction.
 */
void TST_SvgComponentTags::DownwardGrainAxisPointsUp() const
{
    VLayoutPiece piece = makeTestPiece();
    setHiddenGrain(piece, QStringLiteral("270"));

    qreal angle = 0;
    QVERIFY(piece.grainlineAngle(angle));
    QVERIFY2(qAbs(normalizedDegrees(angle) - 90.0) < 1e-6, qPrintable(QString::number(angle)));
}
