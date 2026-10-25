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
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vlayout/vlayoutdef.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vlayout/vlayoutpiecepath.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vpatterndb/FoldLineGraphic.h"
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
#include <QRectF>
#include <QRegularExpression>
#include <QScopedPointer>
#include <QSet>
#include <QTemporaryDir>
#include <QTransform>

#include <algorithm>

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
 * @param sampleSize individual pattern size passed to setSampleSize(); empty sets none.
 * @return the parsed SVG document; null if export or parsing failed.
 */
QDomDocument exportItemSvg(QGraphicsItem *item, const QString &sampleSize = QString())
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
    generator.setSampleSize(sampleSize);
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
 * @brief exportMultisizeSvg renders two pieces ("Front", "Back") in two sizes
 * ("34", "36") through one SvgGenerator, the way Layout Mode hands off a
 * multisize pattern, and parses the merged result back.
 * @return the parsed merged SVG document; null if export or parsing failed.
 */
QDomDocument exportMultisizeSvg()
{
    QTemporaryDir tempDir;
    if (!tempDir.isValid())
    {
        return QDomDocument();
    }
    const QString filePath = tempDir.filePath(QStringLiteral("multisize_tags.svg"));

    const QStringList sizes {QStringLiteral("34"), QStringLiteral("36")};
    const QStringList names {QStringLiteral("Front"), QStringLiteral("Back")};

    QGraphicsRectItem paper(QRectF(0, 0, 400, 400));
    SvgGenerator generator(&paper, filePath, QStringLiteral("Test Pattern"), QString(), 96);
    generator.setMultisize(sizes, QStringLiteral("36"), QStringLiteral("176"));

    // Size-major order, like the Layout Mode handoff: every piece of size 34, then every piece of size 36.
    QList<QGraphicsScene *> scenes;
    for (const QString &size : sizes)
    {
        for (int i = 0; i < names.size(); ++i)
        {
            QGraphicsScene *scene = new QGraphicsScene();
            QGraphicsItem *item = makeTestPiece(names.at(i)).GetItem(true);
            scene->addItem(item);
            generator.addSvgFromScene(scene, item, size, QString::number(i));
            scenes.append(scene);
        }
    }
    generator.generate();
    qDeleteAll(scenes);

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

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief subpathCount counts the MoveTo commands of every path under a group.
 */
int subpathCount(const QDomElement &group)
{
    int count = 0;
    const QDomNodeList paths = group.elementsByTagName(QStringLiteral("path"));
    for (int i = 0; i < paths.size(); ++i)
    {
        const QString d = paths.at(i).toElement().attribute(QStringLiteral("d"));
        count += d.count(QLatin1Char('M')) + d.count(QLatin1Char('m'));
    }
    return count;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief slitAndTNotchLines returns the lines of two notches: a slit (1 line), then a T notch (2 lines).
 */
QVector<QLineF> slitAndTNotchLines()
{
    return {QLineF(QPointF(10, 100), QPointF(15, 100)),
            QLineF(QPointF(100, 10), QPointF(100, 15)),
            QLineF(QPointF(97, 15), QPointF(103, 15))};
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief makeBuiltInPiece builds a square piece with a built-in seam allowance.
 * @details The main path is the cut line, a 180 square. The seam line is the 160 square inside it.
 */
VLayoutPiece makeBuiltInPiece(const QString &name)
{
    VLayoutPiece piece;
    piece.SetName(name);
    piece.setMainPathPoints(squarePoints(10, 10, 180), false, squarePoints(10, 10, 180));
    piece.setSeamAllowancePoints(QVector<QPointF>(), true, true);
    piece.setBuiltInSeamLinePoints(squarePoints(20, 20, 160), squarePoints(20, 20, 160));
    return piece;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief pathNumbers returns every number in the path data under a group, in path order.
 */
QVector<qreal> pathNumbers(const QDomElement &group)
{
    static const QRegularExpression number(QStringLiteral("-?\\d+(?:\\.\\d+)?(?:[eE]-?\\d+)?"));
    QVector<qreal> values;
    const QDomNodeList paths = group.elementsByTagName(QStringLiteral("path"));
    for (int i = 0; i < paths.size(); ++i)
    {
        QRegularExpressionMatchIterator it = number.globalMatch(paths.at(i).toElement().attribute(QStringLiteral("d")));
        while (it.hasNext())
        {
            values.append(it.next().captured().toDouble());
        }
    }
    return values;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief pathBounds returns the bounding box of every coordinate pair in the path data under a group.
 */
QRectF pathBounds(const QDomElement &group)
{
    const QVector<qreal> values = pathNumbers(group);

    if (values.size() < 2)
    {
        return QRectF();
    }
    qreal left = values.at(0);
    qreal right = left;
    qreal top = values.at(1);
    qreal bottom = top;
    for (int i = 2; i + 1 < values.size(); i += 2)
    {
        left = qMin(left, values.at(i));
        right = qMax(right, values.at(i));
        top = qMin(top, values.at(i + 1));
        bottom = qMax(bottom, values.at(i + 1));
    }
    return QRectF(QPointF(left, top), QPointF(right, bottom));
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

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief IndividualPatternIsMarkedIndividual checks that an export without
 * setMultisize() marks the pattern group data-measurements="individual" and
 * writes no size attributes.
 */
void TST_SvgComponentTags::IndividualPatternIsMarkedIndividual() const
{
    const QDomDocument doc = exportPieceSvg(makeTestPiece(QStringLiteral("Yoke")));
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> patterns = groupsOfType(doc, QStringLiteral("pattern"));
    QCOMPARE(patterns.size(), 1);
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("data-measurements")), QStringLiteral("individual"));
    QVERIFY(!patterns.at(0).hasAttribute(QStringLiteral("data-sizes")));
    QVERIFY(!patterns.at(0).hasAttribute(QStringLiteral("data-sample-size")));
    QVERIFY(groupsOfType(doc, QStringLiteral("piece-set")).isEmpty());
    QVERIFY(!groupsOfType(doc, QStringLiteral("piece")).at(0).hasAttribute(QStringLiteral("data-size")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief IndividualPatternCarriesSampleSize checks that setSampleSize() writes
 * data-sample-size on the pattern group and no data-size on the piece.
 */
void TST_SvgComponentTags::IndividualPatternCarriesSampleSize() const
{
    const QDomDocument doc = exportItemSvg(makeTestPiece(QStringLiteral("Yoke")).GetItem(true), QStringLiteral("102"));
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> patterns = groupsOfType(doc, QStringLiteral("pattern"));
    QCOMPARE(patterns.size(), 1);
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("data-measurements")), QStringLiteral("individual"));
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("data-sample-size")), QStringLiteral("102"));
    QVERIFY(!groupsOfType(doc, QStringLiteral("piece")).at(0).hasAttribute(QStringLiteral("data-size")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MultisizePatternCarriesSizeList checks the pattern group attributes of a multisize export.
 */
void TST_SvgComponentTags::MultisizePatternCarriesSizeList() const
{
    const QDomDocument doc = exportMultisizeSvg();
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> patterns = groupsOfType(doc, QStringLiteral("pattern"));
    QCOMPARE(patterns.size(), 1);
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("data-measurements")), QStringLiteral("multisize"));
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("data-sizes")), QStringLiteral("34,36"));
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("data-base-size")), QStringLiteral("36"));
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("data-height")), QStringLiteral("176"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MultisizePiecesNestInPieceSets checks that the same piece in every
 * size is nested in one piece-set group, each size tagged with data-size.
 */
void TST_SvgComponentTags::MultisizePiecesNestInPieceSets() const
{
    const QDomDocument doc = exportMultisizeSvg();
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> sets = groupsOfType(doc, QStringLiteral("piece-set"));
    QCOMPARE(sets.size(), 2);
    QCOMPARE(sets.at(0).attribute(QStringLiteral("id")), QStringLiteral("piece-set_Front"));
    QCOMPARE(sets.at(0).attribute(QStringLiteral("data-name")), QStringLiteral("Front"));
    QCOMPARE(sets.at(1).attribute(QStringLiteral("id")), QStringLiteral("piece-set_Back"));
    QCOMPARE(sets.at(0).parentNode().toElement().attribute(QStringLiteral("data-type")), QStringLiteral("pattern"));

    for (const QDomElement &set : sets)
    {
        QStringList childSizes;
        for (QDomElement child = set.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
        {
            QCOMPARE(child.attribute(QStringLiteral("data-type")), QStringLiteral("piece"));
            QCOMPARE(child.attribute(QStringLiteral("data-name")), set.attribute(QStringLiteral("data-name")));
            QCOMPARE(child.attribute(QStringLiteral("data-parent")), set.attribute(QStringLiteral("id")));
            childSizes.append(child.attribute(QStringLiteral("data-size")));
        }
        QCOMPARE(childSizes, QStringList({QStringLiteral("34"), QStringLiteral("36")}));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MultisizeIdsAreUniqueAndSized checks that piece and component ids
 * carry the size and that no id repeats across sizes.
 */
void TST_SvgComponentTags::MultisizeIdsAreUniqueAndSized() const
{
    const QDomDocument doc = exportMultisizeSvg();
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> pieces = groupsOfType(doc, QStringLiteral("piece"));
    QCOMPARE(pieces.size(), 4);
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("id")), QStringLiteral("piece_Front_s34"));
    QCOMPARE(pieces.at(1).attribute(QStringLiteral("id")), QStringLiteral("piece_Front_s36"));

    const QVector<QDomElement> seamlines = groupsOfType(doc, QStringLiteral("seamline"));
    QCOMPARE(seamlines.size(), 4);
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("id")), QStringLiteral("seamline_Front_s34"));

    QSet<QString> ids;
    const QDomNodeList groups = doc.elementsByTagName(QStringLiteral("g"));
    for (int i = 0; i < groups.size(); ++i)
    {
        const QString id = groups.at(i).toElement().attribute(QStringLiteral("id"));
        if (id.isEmpty())
        {
            continue;
        }
        QVERIFY2(!ids.contains(id), qPrintable(QStringLiteral("duplicate id ") + id));
        ids.insert(id);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PathComponentsCarryTurnPoints checks that seam line, cut line, internal path and cut path
 * groups carry data-turn-points when their node vertices are known.
 */
void TST_SvgComponentTags::PathComponentsCarryTurnPoints() const
{
    VLayoutPiece piece = makeTestPiece(QStringLiteral("Yoke"));
    // Square corners are node vertices; the midpoint of the top side is a smooth node.
    QVector<QPointF> contour = squarePoints(10, 10, 180);
    contour.insert(1, QPointF(100, 10));
    piece.setMainPathPoints(contour, false, contour);
    piece.setSeamAllowancePoints(squarePoints(5, 5, 190), true, false, squarePoints(5, 5, 190));

    VLayoutPiecePath internal = makePath({QPointF(20, 20), QPointF(40, 30), QPointF(60, 20)}, false);
    internal.setTurnPoints({0, 1, 2});
    piece.setInternalPaths({internal});

    const QDomDocument doc = exportPieceSvg(piece);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> seamlines = groupsOfType(doc, QStringLiteral("seamline"));
    QCOMPARE(seamlines.size(), 1);
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("data-turn-points")), QStringLiteral("0 2 3 4"));

    const QVector<QDomElement> cutlines = groupsOfType(doc, QStringLiteral("cutline"));
    QCOMPARE(cutlines.size(), 1);
    QCOMPARE(cutlines.at(0).attribute(QStringLiteral("data-turn-points")), QStringLiteral("0 1 2 3"));

    const QVector<QDomElement> internals = groupsOfType(doc, QStringLiteral("internal_path"));
    QCOMPARE(internals.size(), 1);
    QCOMPARE(internals.at(0).attribute(QStringLiteral("data-turn-points")), QStringLiteral("0 1 2"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UnknownTurnPointsEmitNoAttribute checks that a path without node vertices gets no
 * data-turn-points, so SeamlyLayout uses its own detection.
 */
void TST_SvgComponentTags::UnknownTurnPointsEmitNoAttribute() const
{
    const QDomDocument doc = exportPieceSvg(makeTestPiece(QStringLiteral("Cuff")));
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> seamlines = groupsOfType(doc, QStringLiteral("seamline"));
    QCOMPARE(seamlines.size(), 1);
    QVERIFY(!seamlines.at(0).hasAttribute(QStringLiteral("data-turn-points")));

    const QVector<QDomElement> cutPaths = groupsOfType(doc, QStringLiteral("cut_path"));
    QVERIFY(!cutPaths.isEmpty());
    QVERIFY(!cutPaths.at(0).hasAttribute(QStringLiteral("data-turn-points")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NotchGroupCarriesSeamAllowances checks that the notch group lists one seam allowance width in mm per
 * notch, and has no attribute when the widths are unknown.
 */
void TST_SvgComponentTags::NotchGroupCarriesSeamAllowances() const
{
    VLayoutPiece piece = makeTestPiece(QStringLiteral("Placket"));
    piece.setNotches(slitAndTNotchLines(), {1, 2}, {ToPixel(10, Unit::Mm), ToPixel(6.5, Unit::Mm)});

    const QDomDocument doc = exportPieceSvg(piece);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");
    const QVector<QDomElement> notches = groupsOfType(doc, QStringLiteral("notch"));
    QCOMPARE(notches.size(), 1);
    QCOMPARE(notches.at(0).attribute(QStringLiteral("data-seam-allowances")), QStringLiteral("10.00 6.50"));

    const QDomDocument unknown = exportPieceSvg(makeTestPiece(QStringLiteral("Cuff")));
    const QVector<QDomElement> unknownNotches = groupsOfType(unknown, QStringLiteral("notch"));
    QCOMPARE(unknownNotches.size(), 1);
    QVERIFY(!unknownNotches.at(0).hasAttribute(QStringLiteral("data-seam-allowances")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NotchGroupCarriesNotchTypes checks that the notch group lists one Seamly2D notch type and one line count
 * per notch, that the counts sum to the subpath count, and that unknown or wrong counts write neither attribute.
 */
void TST_SvgComponentTags::NotchGroupCarriesNotchTypes() const
{
    const QStringList types {QStringLiteral("slit"), QStringLiteral("tNotch")};
    VLayoutPiece piece = makeTestPiece(QStringLiteral("Yoke"));
    piece.setNotches(slitAndTNotchLines(), {1, 2}, {}, types);

    const QDomDocument doc = exportPieceSvg(piece);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");
    const QVector<QDomElement> notches = groupsOfType(doc, QStringLiteral("notch"));
    QCOMPARE(notches.size(), 1);
    QCOMPARE(notches.at(0).attribute(QStringLiteral("data-notch-type")), QStringLiteral("slit tNotch"));
    QCOMPARE(notches.at(0).attribute(QStringLiteral("data-notch-segments")), QStringLiteral("1 2"));
    QCOMPARE(subpathCount(notches.at(0)), 3);
    QVERIFY(!notches.at(0).hasAttribute(QStringLiteral("data-seam-allowances")));

    // Counts that do not sum to the line count are dropped together with the types.
    piece.setNotches(slitAndTNotchLines(), {1, 1}, {}, types);
    const QDomDocument wrong = exportPieceSvg(piece);
    const QVector<QDomElement> wrongNotches = groupsOfType(wrong, QStringLiteral("notch"));
    QCOMPARE(wrongNotches.size(), 1);
    QVERIFY(!wrongNotches.at(0).hasAttribute(QStringLiteral("data-notch-segments")));
    QVERIFY(!wrongNotches.at(0).hasAttribute(QStringLiteral("data-notch-type")));

    const QDomDocument unknown = exportPieceSvg(makeTestPiece(QStringLiteral("Cuff")));
    const QVector<QDomElement> unknownNotches = groupsOfType(unknown, QStringLiteral("notch"));
    QCOMPARE(unknownNotches.size(), 1);
    QVERIFY(!unknownNotches.at(0).hasAttribute(QStringLiteral("data-notch-segments")));
    QVERIFY(!unknownNotches.at(0).hasAttribute(QStringLiteral("data-notch-type")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceCarriesQuantityAndFold checks that the piece group carries the cut count, and data-on-fold only for a
 * piece cut on the fold.
 */
void TST_SvgComponentTags::PieceCarriesQuantityAndFold() const
{
    VLayoutPiece folded = makeTestPiece(QStringLiteral("Collar"));
    folded.setQuantity(3);
    folded.setOnFold(true);
    const QVector<QDomElement> foldedPieces = groupsOfType(exportPieceSvg(folded), QStringLiteral("piece"));
    QCOMPARE(foldedPieces.size(), 1);
    QCOMPARE(foldedPieces.at(0).attribute(QStringLiteral("data-quantity")), QStringLiteral("3"));
    QCOMPARE(foldedPieces.at(0).attribute(QStringLiteral("data-on-fold")), QStringLiteral("true"));

    const QVector<QDomElement> plainPieces = groupsOfType(exportPieceSvg(makeTestPiece(QStringLiteral("Back"))),
                                                          QStringLiteral("piece"));
    QCOMPARE(plainPieces.size(), 1);
    QCOMPARE(plainPieces.at(0).attribute(QStringLiteral("data-quantity")), QStringLiteral("1"));
    QVERIFY(!plainPieces.at(0).hasAttribute(QStringLiteral("data-on-fold")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceCarriesPieceProperties checks that the piece group carries rotation, tilt, flip, orientation and
 * annotation from the Piece Properties, and no attribute for a value that allows nothing.
 */
void TST_SvgComponentTags::PieceCarriesPieceProperties() const
{
    VPieceLabelData properties;
    properties.SetRotationWay(PieceRotation::TwoWay);
    properties.SetTilt(PieceTilt::Clockwise);
    properties.setTiltDegrees(15);
    properties.SetOrientation(QStringLiteral("Left"));
    properties.SetAnnotation(QStringLiteral("Cut in contrast"));
    properties.SetVisible(false); // a hidden label still sends the Piece Properties

    VLayoutPiece collar = makeTestPiece(QStringLiteral("Collar"));
    collar.setPieceProperties(properties);
    collar.setFlip(PieceFlip::Horizontal);
    const QVector<QDomElement> pieces = groupsOfType(exportPieceSvg(collar), QStringLiteral("piece"));
    QCOMPARE(pieces.size(), 1);
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("data-rotation")), QStringLiteral("180"));
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("data-tilt")), QStringLiteral("-15"));
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("data-flip")), QStringLiteral("Horizontal"));
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("data-orientation")), QStringLiteral("Left"));
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("data-annotation")), QStringLiteral("Cut in contrast"));

    // Counterclockwise tilt is positive.
    properties.SetTilt(PieceTilt::CounterClockwise);
    collar.setPieceProperties(properties);
    const QVector<QDomElement> ccw = groupsOfType(exportPieceSvg(collar), QStringLiteral("piece"));
    QCOMPARE(ccw.at(0).attribute(QStringLiteral("data-tilt")), QStringLiteral("15"));

    // Defaults, a tilt of 0 degrees and an undefined orientation allow nothing: no attributes.
    VPieceLabelData nothing;
    nothing.SetTilt(PieceTilt::CounterClockwise);
    nothing.SetOrientation(QStringLiteral("Undefined"));
    VLayoutPiece back = makeTestPiece(QStringLiteral("Back"));
    back.setPieceProperties(nothing);
    const QVector<QDomElement> plain = groupsOfType(exportPieceSvg(back), QStringLiteral("piece"));
    QCOMPARE(plain.size(), 1);
    const QStringList attributes{QStringLiteral("data-rotation"), QStringLiteral("data-tilt"),
                                 QStringLiteral("data-flip"), QStringLiteral("data-orientation"),
                                 QStringLiteral("data-annotation")};
    for (const QString &attribute : attributes)
    {
        QVERIFY2(!plain.at(0).hasAttribute(attribute), qPrintable(attribute));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FoldLineSendsFoldLineGroup checks that a fold line becomes one fold_line group whose first path is the two
 * point fold line, and that a piece without one sends none.
 */
void TST_SvgComponentTags::FoldLineSendsFoldLineGroup() const
{
    VLayoutPiece folded = makeTestPiece(QStringLiteral("Collar"));
    folded.setOnFold(true);
    folded.setFoldLine(QVector<QPointF>{QPointF(10, 190), QPointF(10, 10)});
    const QDomDocument doc = exportPieceSvg(folded);

    const QVector<QDomElement> fold_lines = groupsOfType(doc, QStringLiteral("fold_line"));
    QCOMPARE(fold_lines.size(), 1);
    QCOMPARE(subpathCount(fold_lines.at(0)), 1);
    QCOMPARE(fold_lines.at(0).attribute(QStringLiteral("id")), QStringLiteral("fold_line_Collar"));
    const QRectF bounds = pathBounds(fold_lines.at(0));
    QVERIFY2(bounds.width() < 0.01 && qAbs(bounds.height() - 180) < 0.01,
             qPrintable(QStringLiteral("%1 x %2").arg(bounds.width()).arg(bounds.height())));

    // Without a graphic the group holds the fold line only.
    QVERIFY(groupsOfType(doc, QStringLiteral("fold_line_arrow")).isEmpty());
    QVERIFY(groupsOfType(doc, QStringLiteral("fold_line_label")).isEmpty());

    const QDomDocument plain = exportPieceSvg(makeTestPiece(QStringLiteral("Back")));
    QVERIFY(groupsOfType(plain, QStringLiteral("fold_line")).isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FoldLineGroupHoldsArrowAndLabel checks the fold_line group order: the fold line path first, then the
 * tagged arrow line group, then the tagged label group with a real "Foldline" text element.
 */
void TST_SvgComponentTags::FoldLineGroupHoldsArrowAndLabel() const
{
    // The fold line is the left side of the 180 square, from its bottom to its top node.
    const QLineF fold(QPointF(10, 190), QPointF(10, 10));
    VLayoutPiece folded = makeTestPiece(QStringLiteral("Collar"));
    folded.setOnFold(true);
    folded.setFoldLine(QVector<QPointF>{fold.p1(), fold.p2()});
    folded.setFoldLineGraphic(pattern_piece::FoldLineGraphic(fold, squarePoints(10, 10, 180)));
    const QDomDocument doc = exportItemSvg(folded.GetItem(false));
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> fold_lines = groupsOfType(doc, QStringLiteral("fold_line"));
    QCOMPARE(fold_lines.size(), 1);
    const QVector<QDomElement> arrows = groupsOfType(doc, QStringLiteral("fold_line_arrow"));
    const QVector<QDomElement> labels = groupsOfType(doc, QStringLiteral("fold_line_label"));
    QCOMPARE(arrows.size(), 1);
    QCOMPARE(labels.size(), 1);

    // Both parts sit inside the fold_line group and name the piece.
    QVERIFY(arrows.at(0).parentNode().toElement() == fold_lines.at(0));
    QVERIFY(labels.at(0).parentNode().toElement() == fold_lines.at(0));
    QCOMPARE(arrows.at(0).attribute(QStringLiteral("id")), QStringLiteral("fold_line_arrow_Collar"));
    QCOMPARE(labels.at(0).attribute(QStringLiteral("id")), QStringLiteral("fold_line_label_Collar"));
    QCOMPARE(arrows.at(0).attribute(QStringLiteral("data-parent")), QStringLiteral("Collar"));
    QCOMPARE(labels.at(0).attribute(QStringLiteral("data-parent")), QStringLiteral("Collar"));

    // The first path of the group is the fold line, a two point path, ahead of the arrow line and its heads.
    const QDomElement first_path = fold_lines.at(0).elementsByTagName(QStringLiteral("path")).at(0).toElement();
    const QString first_d = first_path.attribute(QStringLiteral("d"));
    QCOMPARE(first_d.count(QLatin1Char('M')) + first_d.count(QLatin1Char('L')), 2);
    const QDomNodeList children = fold_lines.at(0).childNodes();
    QVERIFY(children.at(children.size() - 2).toElement() == arrows.at(0));
    QVERIFY(children.at(children.size() - 1).toElement() == labels.at(0));

    // Arrow line (one subpath) and two closed arrow heads, inside the piece, right of the fold line.
    QCOMPARE(subpathCount(arrows.at(0)), 3);
    const QRectF arrow_bounds = pathBounds(arrows.at(0));
    QVERIFY2(arrow_bounds.left() > 9.99 && arrow_bounds.right() < 19.01,
             qPrintable(QStringLiteral("%1 to %2").arg(arrow_bounds.left()).arg(arrow_bounds.right())));

    const QDomNodeList texts = labels.at(0).elementsByTagName(QStringLiteral("text"));
    QCOMPARE(texts.size(), 1);
    QCOMPARE(texts.at(0).toElement().text().trimmed(), pattern_piece::FoldLineGraphic::labelText());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PiecesOfEachMaterialShareAPatternGroup checks that each material gets its own pattern group, in the order
 * its first piece was added, and that each piece names that group as its parent.
 */
void TST_SvgComponentTags::PiecesOfEachMaterialShareAPatternGroup() const
{
    VLayoutPiece front = makeTestPiece(QStringLiteral("Front"));
    VLayoutPiece lining = makeTestPiece(QStringLiteral("Front Lining"));
    lining.setMaterial(QStringLiteral("Lining"));
    const QDomDocument doc = exportTwoPiecesSvg(front, lining);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> patterns = groupsOfType(doc, QStringLiteral("pattern"));
    QCOMPARE(patterns.size(), 2);
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("id")), QStringLiteral("pattern-1"));
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("data-material")), QStringLiteral("Fabric"));
    QCOMPARE(patterns.at(1).attribute(QStringLiteral("id")), QStringLiteral("pattern-2"));
    QCOMPARE(patterns.at(1).attribute(QStringLiteral("data-material")), QStringLiteral("Lining"));
    QCOMPARE(patterns.at(1).attribute(QStringLiteral("data-type-number")), QStringLiteral("2"));

    const QVector<QDomElement> pieces = groupsOfType(doc, QStringLiteral("piece"));
    QCOMPARE(pieces.size(), 2);
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("data-material")), QStringLiteral("Fabric"));
    QCOMPARE(pieces.at(0).attribute(QStringLiteral("data-parent")), QStringLiteral("pattern-1"));
    QCOMPARE(pieces.at(0).parentNode().toElement().attribute(QStringLiteral("id")), QStringLiteral("pattern-1"));
    QCOMPARE(pieces.at(1).attribute(QStringLiteral("data-material")), QStringLiteral("Lining"));
    QCOMPARE(pieces.at(1).attribute(QStringLiteral("data-parent")), QStringLiteral("pattern-2"));
    QCOMPARE(pieces.at(1).parentNode().toElement().attribute(QStringLiteral("id")), QStringLiteral("pattern-2"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief OneMaterialKeepsOnePatternGroup checks that pieces of one material share the single pattern-1 group, and
 * that a piece with no material set is cut from fabric.
 */
void TST_SvgComponentTags::OneMaterialKeepsOnePatternGroup() const
{
    VLayoutPiece back = makeTestPiece(QStringLiteral("Back"));
    back.setMaterial(QString());
    QCOMPARE(back.getMaterial(), VAbstractPiece::defaultMaterial());
    QCOMPARE(VAbstractPiece::defaultMaterial(), QStringLiteral("Fabric"));
    QCOMPARE(VAbstractPiece::materialNames(), QStringList({QStringLiteral("Fabric"), QStringLiteral("Lining"),
                                                            QStringLiteral("Interfacing"),
                                                            QStringLiteral("Interlining")}));

    const QDomDocument doc = exportTwoPiecesSvg(makeTestPiece(QStringLiteral("Front")), back);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");
    const QVector<QDomElement> patterns = groupsOfType(doc, QStringLiteral("pattern"));
    QCOMPARE(patterns.size(), 1);
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("id")), QStringLiteral("pattern-1"));
    QCOMPARE(patterns.at(0).attribute(QStringLiteral("data-material")), QStringLiteral("Fabric"));
    QCOMPARE(groupsOfType(doc, QStringLiteral("piece")).size(), 2);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief BuiltInPieceSendsCutlineAndSeamline checks that a built-in seam allowance sends its main path as the
 * cutline and the line inside it as the seamline, each with its own turn points. The cutline is the piece outline,
 * so it is sent also when the seam allowance view setting is off.
 */
void TST_SvgComponentTags::BuiltInPieceSendsCutlineAndSeamline() const
{
    const bool savedShowSeamAllowances = qApp->Settings()->showSeamAllowances();
    qApp->Settings()->setShowSeamAllowances(false);
    const QDomDocument doc = exportPieceSvg(makeBuiltInPiece(QStringLiteral("Facing")));
    qApp->Settings()->setShowSeamAllowances(savedShowSeamAllowances);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");

    const QVector<QDomElement> cutlines = groupsOfType(doc, QStringLiteral("cutline"));
    QCOMPARE(cutlines.size(), 1);
    QCOMPARE(cutlines.at(0).attribute(QStringLiteral("id")), QStringLiteral("cutline_Facing"));
    QCOMPARE(cutlines.at(0).attribute(QStringLiteral("data-turn-points")), QStringLiteral("0 1 2 3"));

    const QVector<QDomElement> seamlines = groupsOfType(doc, QStringLiteral("seamline"));
    QCOMPARE(seamlines.size(), 1);
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("id")), QStringLiteral("seamline_Facing"));
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("data-turn-points")), QStringLiteral("0 1 2 3"));

    // The seam line lies inside the cut line: 160 of 180 on each axis.
    const QRectF cut = pathBounds(cutlines.at(0));
    const QRectF seam = pathBounds(seamlines.at(0));
    QVERIFY2(cut.width() > 0 && seam.width() > 0, "both lines have path data");
    QVERIFY2(cut.contains(seam) && seam.width() < cut.width() && seam.height() < cut.height(),
             qPrintable(QStringLiteral("cut %1 x %2, seam %3 x %4").arg(cut.width()).arg(cut.height())
                        .arg(seam.width()).arg(seam.height())));
    QVERIFY(qAbs(seam.width() / cut.width() - 160.0 / 180.0) < 0.001);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief BuiltInPieceHiddenSeamLineSendsCutlineOnly checks that a hidden seam line is not sent for a built-in seam
 * allowance, the same as for a seam allowance that is not built in.
 */
void TST_SvgComponentTags::BuiltInPieceHiddenSeamLineSendsCutlineOnly() const
{
    VLayoutPiece piece = makeBuiltInPiece(QStringLiteral("Facing"));
    piece.setHideSeamLine(true);

    const QDomDocument doc = exportPieceSvg(piece);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");
    QCOMPARE(groupsOfType(doc, QStringLiteral("cutline")).size(), 1);
    QCOMPARE(groupsOfType(doc, QStringLiteral("seamline")).size(), 0);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief BuiltInPieceWithFoldLineSendsOpenSeamline checks that a built-in seam line that a fold line cuts open is
 * sent open, with turn points at both ends, together with the cutline and the fold line.
 */
void TST_SvgComponentTags::BuiltInPieceWithFoldLineSendsOpenSeamline() const
{
    // The fold line is the left side of the 180 square; the seam line meets it 10 inside the top and bottom sides.
    VLayoutPiece piece = makeBuiltInPiece(QStringLiteral("Facing"));
    const QVector<QPointF> seamLine{QPointF(10, 20), QPointF(180, 20), QPointF(180, 180), QPointF(10, 180)};
    piece.setBuiltInSeamLinePoints(seamLine, seamLine, false);
    piece.setFoldLine(QVector<QPointF>{QPointF(10, 190), QPointF(10, 10)});

    const QDomDocument doc = exportPieceSvg(piece);
    QVERIFY2(!doc.isNull(), "Generated SVG could not be produced or parsed");
    QCOMPARE(groupsOfType(doc, QStringLiteral("fold_line")).size(), 1);
    const QVector<QDomElement> cutlines = groupsOfType(doc, QStringLiteral("cutline"));
    QCOMPARE(cutlines.size(), 1);
    const QVector<QDomElement> seamlines = groupsOfType(doc, QStringLiteral("seamline"));
    QCOMPARE(seamlines.size(), 1);
    QCOMPARE(seamlines.at(0).attribute(QStringLiteral("data-turn-points")), QStringLiteral("0 1 2 3"));

    // Four vertices and no closing one: the path does not return along the fold line.
    const QDomNodeList paths = seamlines.at(0).elementsByTagName(QStringLiteral("path"));
    QCOMPARE(paths.size(), 1);
    const QString d = paths.at(0).toElement().attribute(QStringLiteral("d"));
    const QVector<qreal> values = pathNumbers(seamlines.at(0));
    QVERIFY2(!d.contains(QLatin1Char('Z'), Qt::CaseInsensitive), qPrintable(d));
    QCOMPARE(values.size(), 8);

    // Both ends lie on the fold line, the left side of the cutline.
    const QRectF cut = pathBounds(cutlines.at(0));
    QVERIFY2(qAbs(values.at(0) - cut.left()) < 0.01 && qAbs(values.at(6) - cut.left()) < 0.01, qPrintable(d));
}

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief bentTopContour builds a 200 x 100 contour with a node in the middle of the top and bottom sides.
 * @param rise how far the top right corner lies below the top side. 2 bends the top side 1.1 degrees at its middle
 *             node: a turn point. 0.5 bends it 0.3 degrees: not a turn point.
 * @return the six node vertices, from the top left corner.
 */
QVector<QPointF> bentTopContour(qreal rise)
{
    return {QPointF(0, 0), QPointF(100, 0), QPointF(200, rise), QPointF(200, 100), QPointF(100, 100), QPointF(0, 100)};
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportedTurnPoints returns the data-turn-points attribute of the piece's first group of one type.
 * @return the attribute; empty when the group or the attribute is missing.
 */
QString exportedTurnPoints(const VLayoutPiece &piece, const QString &type)
{
    const QVector<QDomElement> groups = groupsOfType(exportPieceSvg(piece), type);
    return groups.isEmpty() ? QString() : groups.first().attribute(QStringLiteral("data-turn-points"));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SizesShareNodeTurnPoints checks that a node that is a turn point in one size becomes one in every size,
 * at the vertex index of that node in each size.
 */
void TST_SvgComponentTags::SizesShareNodeTurnPoints() const
{
    const QString seamline = QStringLiteral("seamline");

    VLayoutPiece bent = makeTestPiece(QStringLiteral("Detail"));
    bent.setMainPathPoints(bentTopContour(2), false, bentTopContour(2));

    VLayoutPiece straight = makeTestPiece(QStringLiteral("Detail"));
    straight.setMainPathPoints(bentTopContour(0.5), false, bentTopContour(0.5));

    // A curve point before the top node moves that node to vertex index 2.
    VLayoutPiece shifted = makeTestPiece(QStringLiteral("Detail"));
    QVector<QPointF> shiftedContour = bentTopContour(0.5);
    shiftedContour.insert(1, QPointF(50, 0));
    shifted.setMainPathPoints(shiftedContour, false, bentTopContour(0.5));

    QCOMPARE(exportedTurnPoints(bent, seamline), QStringLiteral("0 1 2 3 5"));
    QCOMPARE(exportedTurnPoints(straight, seamline), QStringLiteral("0 2 3 5"));
    QCOMPARE(exportedTurnPoints(shifted, seamline), QStringLiteral("0 3 4 6"));

    VLayoutPiece::unifyTurnPoints({&bent, &straight, &shifted});

    QCOMPARE(exportedTurnPoints(bent, seamline), QStringLiteral("0 1 2 3 5"));
    QCOMPARE(exportedTurnPoints(straight, seamline), QStringLiteral("0 1 2 3 5"));
    QCOMPARE(exportedTurnPoints(shifted, seamline), QStringLiteral("0 2 3 4 6"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SizesWithDifferentNodeCountsKeepTurnPoints checks that a line whose sizes differ in node count keeps
 * the turn points of each size.
 */
void TST_SvgComponentTags::SizesWithDifferentNodeCountsKeepTurnPoints() const
{
    const QString seamline = QStringLiteral("seamline");

    VLayoutPiece bent = makeTestPiece(QStringLiteral("Detail"));
    bent.setMainPathPoints(bentTopContour(2), false, bentTopContour(2));

    // One node more: a node before the top node.
    VLayoutPiece extraNode = makeTestPiece(QStringLiteral("Detail"));
    QVector<QPointF> extraContour = bentTopContour(0.5);
    extraContour.insert(1, QPointF(50, 0));
    extraNode.setMainPathPoints(extraContour, false, extraContour);

    VLayoutPiece::unifyTurnPoints({&bent, &extraNode});

    QCOMPARE(exportedTurnPoints(bent, seamline), QStringLiteral("0 1 2 3 5"));
    QCOMPARE(exportedTurnPoints(extraNode, seamline), QStringLiteral("0 3 4 6"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LoopStartingAtAnotherNodeSharesTurnPoints checks that a closed loop that starts at another node in another
 * size gets the turn point on the same node, not on the same node number.
 */
void TST_SvgComponentTags::LoopStartingAtAnotherNodeSharesTurnPoints() const
{
    const QString seamline = QStringLiteral("seamline");

    VLayoutPiece bent = makeTestPiece(QStringLiteral("Detail"));
    bent.setMainPathPoints(bentTopContour(2), false, bentTopContour(2));

    // The same loop started at the top right corner: the top node is the last vertex.
    VLayoutPiece rotated = makeTestPiece(QStringLiteral("Detail"));
    QVector<QPointF> rotatedContour = bentTopContour(0.5);
    std::rotate(rotatedContour.begin(), rotatedContour.begin() + 2, rotatedContour.end());
    rotated.setMainPathPoints(rotatedContour, false, rotatedContour);
    QCOMPARE(exportedTurnPoints(rotated, seamline), QStringLiteral("0 1 3 4"));

    VLayoutPiece::unifyTurnPoints({&bent, &rotated});

    QCOMPARE(exportedTurnPoints(rotated, seamline), QStringLiteral("0 1 3 4 5"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CutlineRepeatFollowsSharedFirstTurnPoint checks that a cut line that repeats its first vertex also marks
 * the repeat when its first vertex becomes a turn point.
 */
void TST_SvgComponentTags::CutlineRepeatFollowsSharedFirstTurnPoint() const
{
    const QString cutline = QStringLiteral("cutline");

    // The top node is the first vertex, and the loop ends with it again.
    const auto cutLoop = [](qreal rise)
    {
        return QVector<QPointF>{QPointF(100, 0), QPointF(200, rise), QPointF(200, 100), QPointF(0, 100),
                                QPointF(0, 0), QPointF(100, 0)};
    };

    VLayoutPiece bent = makeTestPiece(QStringLiteral("Detail"));
    bent.setSeamAllowancePoints(cutLoop(2), true, false, cutLoop(2));
    VLayoutPiece straight = makeTestPiece(QStringLiteral("Detail"));
    straight.setSeamAllowancePoints(cutLoop(0.5), true, false, cutLoop(0.5));
    QCOMPARE(exportedTurnPoints(bent, cutline), QStringLiteral("0 1 2 3 4 5"));
    QCOMPARE(exportedTurnPoints(straight, cutline), QStringLiteral("1 2 3 4"));

    VLayoutPiece::unifyTurnPoints({&bent, &straight});

    QCOMPARE(exportedTurnPoints(straight, cutline), QStringLiteral("0 1 2 3 4 5"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief InternalPathSizesShareNodeTurnPoints checks that internal paths get the node turn points of every size.
 */
void TST_SvgComponentTags::InternalPathSizesShareNodeTurnPoints() const
{
    const QString internalPath = QStringLiteral("internal_path");

    // An open line that bends at its middle node by 1.1 or 0.3 degrees.
    const auto internalLine = [](qreal rise)
    {
        const QVector<QPointF> points{QPointF(20, 20), QPointF(60, 20), QPointF(100, 20 + rise)};
        VLayoutPiecePath path = makePath(points, false);
        path.setNodeVertices(points, false);
        return path;
    };

    VLayoutPiece bent = makeTestPiece(QStringLiteral("Detail"));
    bent.setInternalPaths({internalLine(0.8)});
    VLayoutPiece straight = makeTestPiece(QStringLiteral("Detail"));
    straight.setInternalPaths({internalLine(0.2)});
    QCOMPARE(exportedTurnPoints(straight, internalPath), QStringLiteral("0 2"));

    VLayoutPiece::unifyTurnPoints({&bent, &straight});

    QCOMPARE(exportedTurnPoints(bent, internalPath), QStringLiteral("0 1 2"));
    QCOMPARE(exportedTurnPoints(straight, internalPath), QStringLiteral("0 1 2"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SizesShareLineEndTurnPoints checks that a line end that breaks the tangent in one size is a turn point
 * in every size.
 */
void TST_SvgComponentTags::SizesShareLineEndTurnPoints() const
{
    const QString internalPath = QStringLiteral("internal_path");

    // An open path: a 65 mm line, then ten 15 mm curve chords. The curve leaves the line at breakDegrees and
    // bends stepDegrees at each later vertex.
    const auto lineIntoCurve = [](qreal breakDegrees, qreal stepDegrees)
    {
        QVector<QPointF> points{QPointF(0, 0), QPointF(65, 0)};
        qreal heading = breakDegrees;
        for (int k = 0; k < 10; ++k)
        {
            const qreal a = qDegreesToRadians(heading);
            points << points.last() + QPointF(15 * qCos(a), 15 * qSin(a));
            heading += stepDegrees;
        }
        VLayoutPiecePath path = makePath(points, false);
        path.setNodeVertices({points.first(), points.at(1), points.last()}, false);
        return path;
    };

    VLayoutPiece breaking = makeTestPiece(QStringLiteral("Detail"));
    breaking.setInternalPaths({lineIntoCurve(2.9, 1.0)});
    VLayoutPiece tangent = makeTestPiece(QStringLiteral("Detail"));
    tangent.setInternalPaths({lineIntoCurve(1.2, 2.0)});
    QCOMPARE(exportedTurnPoints(breaking, internalPath), QStringLiteral("0 1 11"));
    QCOMPARE(exportedTurnPoints(tangent, internalPath), QStringLiteral("0 11"));

    VLayoutPiece::unifyTurnPoints({&breaking, &tangent});

    QCOMPARE(exportedTurnPoints(tangent, internalPath), QStringLiteral("0 1 11"));
}
