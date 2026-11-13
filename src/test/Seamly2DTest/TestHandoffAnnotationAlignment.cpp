// @file TestHandoffAnnotationAlignment.cpp
// @author slspencer
// @brief Tests the shared multisize annotation offset without application state.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

#include "TestHandoffAnnotationAlignment.h"
#include "../../libs/layout_handoff/handoff_annotation_alignment.h"
#include "../../libs/layout_handoff/handoff_stack_point.h"

#include <QDomDocument>
#include <QFile>
#include <QLineF>
#include <QPolygonF>
#include <QRegularExpression>
#include <QTransform>
#include <QtTest>

#include <optional>

namespace
{
// @brief Builds handoff-shaped groups with different size outlines and annotation positions.
QDomDocument document(const QString &small_annotation = QStringLiteral("M90,40 L90,60"),
                      const QString &base_annotation = QStringLiteral("M190,90 L190,110"),
                      const QString &type = QStringLiteral("grainline"))
{
    const auto annotation = [&type](const QString &value)
    {
        if (type == QLatin1String("grainline"))
        {
            return QStringLiteral("<path d='%1'/>").arg(value);
        }
        return value;
    };
    QDomDocument result;
    const auto parsed = result.setContent(QStringLiteral(
        "<svg><g data-type='pattern' data-base-size='500'><g data-type='piece-set' data-name='Back'>"
        "<g data-type='piece' data-size='220'><g data-type='cutline'><path d='M0,0 L100,0 L100,100 L0,100 Z'/>"
        "</g><g data-type='%1'>%2</g></g>"
        "<g data-type='piece' data-size='500'><g data-type='cutline'><path d='M0,0 L200,0 L200,200 L0,200 Z'/>"
        "</g><g data-type='%1'>%3</g></g></g></g></svg>").arg(type, annotation(small_annotation),
                                                          annotation(base_annotation)));
    Q_ASSERT(parsed);
    return result;
}

QDomElement group(const QDomDocument &doc, const QString &type, int index)
{
    const QDomNodeList groups = doc.elementsByTagName(QStringLiteral("g"));
    for (int i = 0; i < groups.size(); ++i)
    {
        const QDomElement element = groups.at(i).toElement();
        if (element.attribute(QStringLiteral("data-type")) == type && index-- == 0)
        {
            return element;
        }
    }
    return {};
}

QPointF translation(const QDomElement &element)
{
    QString text = element.attribute(QStringLiteral("transform"));
    text.remove(QStringLiteral("matrix("));
    text.remove(QLatin1Char(')'));
    const QStringList values = text.split(QLatin1Char(','));
    return values.size() == 6 ? QPointF(values[4].toDouble(), values[5].toDouble()) : QPointF();
}

void close(const QPointF &actual, const QPointF &expected)
{
    QVERIFY2(QLineF(actual, expected).length() < 0.00002,
             qPrintable(QStringLiteral("(%1,%2) != (%3,%4)")
                            .arg(actual.x()).arg(actual.y()).arg(expected.x()).arg(expected.y())));
}

// @brief Reads generated SVG coordinates independently of the alignment implementation.
QVector<QPointF> coordinates(const QDomElement &element, bool text, QTransform parent = {})
{
    QTransform transform;
    QString matrix = element.attribute(QStringLiteral("transform"));
    if (!matrix.isEmpty())
    {
        matrix.remove(QStringLiteral("matrix("));
        matrix.remove(QLatin1Char(')'));
        const QStringList values = matrix.split(QLatin1Char(','));
        if (values.size() == 6)
        {
            transform = QTransform(values[0].toDouble(), values[1].toDouble(), values[2].toDouble(),
                                   values[3].toDouble(), values[4].toDouble(), values[5].toDouble());
        }
    }
    transform *= parent;
    QVector<QPointF> result;
    if (text && element.tagName() == QLatin1String("text"))
    {
        result.append(transform.map(QPointF(element.attribute(QStringLiteral("x")).toDouble(),
                                            element.attribute(QStringLiteral("y")).toDouble())));
    }
    if (!text && element.tagName() == QLatin1String("path"))
    {
        QString path = element.attribute(QStringLiteral("d"));
        path.replace(QRegularExpression(QStringLiteral("[MLZ,]")), QStringLiteral(" "));
        const QStringList values = path.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        for (int i = 0; i + 1 < values.size(); i += 2)
        {
            result.append(transform.map(QPointF(values[i].toDouble(), values[i + 1].toDouble())));
        }
    }
    for (QDomElement child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
    {
        if (!child.hasAttribute(QStringLiteral("data-type")))
        {
            result += coordinates(child, text, transform);
        }
    }
    return result;
}

// @brief Measures distance to polygon edges when an anchor lies exactly on the cut line.
bool contains(const QPolygonF &polygon, const QPointF &point)
{
    if (polygon.containsPoint(point, Qt::OddEvenFill))
    {
        return true;
    }
    for (int i = 0; i < polygon.size(); ++i)
    {
        const QPointF start = polygon[i];
        const QPointF direction = polygon[(i + 1) % polygon.size()] - start;
        const qreal length_squared = QPointF::dotProduct(direction, direction);
        const qreal fraction = length_squared == 0 ? 0
            : qBound(qreal(0), QPointF::dotProduct(point - start, direction) / length_squared, qreal(1));
        if (QLineF(point, start + fraction * direction).length() < 0.00002)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Builds one piece-set of two trapezoid sizes, each with its wide edge at the top.
 * @param small_path  cut line of size 34.
 * @param small_turns turn point indices of size 34; empty leaves out data-turn-points.
 * @param big_path    cut line of size 36, the largest size.
 * @param big_turns   turn point indices of size 36; empty leaves out data-turn-points.
 */
QString trapezoids(const QString& small_path = QStringLiteral("M 0,0 L 40,0 L 30,30 L 10,30 Z"),
                   const QString& small_turns = QStringLiteral("0 1 2 3 4"),
                   const QString& big_path = QStringLiteral("M 100,100 L 180,100 L 160,160 L 120,160 Z"),
                   const QString& big_turns = QStringLiteral("0 1 2 3 4"))
{
    const auto cutline = [](const QString& path, const QString& turns)
    {
        const QString attribute = turns.isEmpty() ? QString() : QStringLiteral(" data-turn-points='%1'").arg(turns);
        return QStringLiteral("<g data-type='cutline'%1><path d='%2'/></g>").arg(attribute, path);
    };
    return QStringLiteral("<svg><g data-type='piece-set'><g data-type='piece' data-size='34'>%1</g>"
                          "<g data-type='piece' data-size='36'>%2</g></g></svg>")
        .arg(cutline(small_path, small_turns), cutline(big_path, big_turns));
}

/**
 * @brief Finds the stack points of the pieces in @p svg.
 * @return one point per `piece` group, in document order.
 */
QVector<std::optional<QPointF>> find_set_stack_points(const QString& svg)
{
    QDomDocument doc;
    const auto   parsed = doc.setContent(svg);
    Q_ASSERT(parsed);
    QVector<QDomElement> pieces;
    for (int i = 0; !group(doc, QStringLiteral("piece"), i).isNull(); ++i)
    {
        pieces.append(group(doc, QStringLiteral("piece"), i));
    }
    return layout_handoff::find_stack_points(pieces);
}

/**
 * @brief Compares stack points within the test tolerance.
 * @param actual   points from find_stack_points.
 * @param expected points to match; no value matches only no value.
 */
void same_points(const QVector<std::optional<QPointF>>& actual, const QVector<std::optional<QPointF>>& expected)
{
    QCOMPARE(actual.size(), expected.size());
    for (int i = 0; i < actual.size(); ++i)
    {
        QCOMPARE(actual[i].has_value(), expected[i].has_value());
        if (expected[i].has_value())
        {
            close(actual[i].value(), expected[i].value());
        }
    }
}
}

void TestHandoffAnnotationAlignment::retainsFittingBaseOffset() const
{
    QDomDocument doc = document(QStringLiteral("M30,40 L30,60"), QStringLiteral("M120,90 L120,110"));
    QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
    close(translation(group(doc, QStringLiteral("grainline"), 0)), QPointF(40, 0));
    close(translation(group(doc, QStringLiteral("grainline"), 1)), {});
    const QString first = doc.toString();
    QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
    QCOMPARE(doc.toString(), first);
}

void TestHandoffAnnotationAlignment::choosesNearestSharedOffset() const
{
    QDomDocument doc = document();
    QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
    close(translation(group(doc, QStringLiteral("grainline"), 0)), QPointF(10, 0));
    close(translation(group(doc, QStringLiteral("grainline"), 1)), QPointF(-40, 0));
}

void TestHandoffAnnotationAlignment::fitsEveryLabelAnchor() const
{
    QDomDocument doc = document(QStringLiteral("<text x='90' y='50'>A</text><text x='110' y='70'>B</text>"),
                               QStringLiteral("<text x='190' y='100'>A</text><text x='210' y='120'>B</text>"),
                               QStringLiteral("piece_label"));
    QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
    close(translation(group(doc, QStringLiteral("piece_label"), 0)), QPointF(-10, 0));
    close(translation(group(doc, QStringLiteral("piece_label"), 1)), QPointF(-60, 0));
}

void TestHandoffAnnotationAlignment::keepsBaseOffsetWhenImpossible() const
{
    QDomDocument doc = document(QStringLiteral("M50,-20 L50,120"), QStringLiteral("M100,30 L100,170"));
    const QStringList warnings = layout_handoff::align_handoff_annotations(doc);
    QCOMPARE(warnings.size(), 1);
    QVERIFY(warnings.first().contains(QStringLiteral("Back: grainline")));
    close(translation(group(doc, QStringLiteral("grainline"), 0)), {});
    close(translation(group(doc, QStringLiteral("grainline"), 1)), {});
}

void TestHandoffAnnotationAlignment::usesLowerFoldLineEndOnlyWhenEverySizeHasOne() const
{
    for (bool all_folded : {false, true})
    {
        QDomDocument doc = document(QStringLiteral("M20,60 L20,80"), QStringLiteral("M30,160 L30,180"));
        for (int i = 0; i < (all_folded ? 2 : 1); ++i)
        {
            // The fold line runs down the left edge, so its lower end is the bottom left corner.
            QDomElement fold = doc.createElement(QStringLiteral("g"));
            fold.setAttribute(QStringLiteral("data-type"), QStringLiteral("fold_line"));
            QDomElement path = doc.createElement(QStringLiteral("path"));
            path.setAttribute(QStringLiteral("d"), i == 0 ? QStringLiteral("M0,0 L0,100")
                                                          : QStringLiteral("M0,0 L0,200"));
            fold.appendChild(path);
            group(doc, QStringLiteral("piece"), i).appendChild(fold);
        }
        QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
        // A mixed set uses outline centres and fits at the nearest shared offset.
        close(translation(group(doc, QStringLiteral("grainline"), 0)),
              all_folded ? QPointF(10, 0) : QPointF(-20, 20));
        close(translation(group(doc, QStringLiteral("grainline"), 1)),
              all_folded ? QPointF(0, 0) : QPointF(20, -30));
    }
}

void TestHandoffAnnotationAlignment::skipsMissingBaseAndHiddenComponents() const
{
    QDomDocument doc = document();
    group(doc, QStringLiteral("pattern"), 0).setAttribute(QStringLiteral("data-base-size"), QStringLiteral("999"));
    const QString before = doc.toString();
    QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
    QCOMPARE(doc.toString(), before);
    doc = document();
    QDomElement hidden = group(doc, QStringLiteral("grainline"), 1);
    hidden.parentNode().removeChild(hidden);
    const QString hidden_before = doc.toString();
    QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
    QCOMPARE(doc.toString(), hidden_before);
}

void TestHandoffAnnotationAlignment::handlesConcaveOutlines() const
{
    QDomDocument doc = document(QStringLiteral("M30,40 L30,60"), QStringLiteral("M120,90 L120,110"));
    group(doc, QStringLiteral("cutline"), 0).firstChildElement().setAttribute(
        QStringLiteral("d"), QStringLiteral("M0,0 L100,0 L100,30 L40,30 L40,100 L0,100 Z"));
    QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
    close(translation(group(doc, QStringLiteral("grainline"), 0)), QPointF(10, 0));
    close(translation(group(doc, QStringLiteral("grainline"), 1)), QPointF(-30, 0));
}

void TestHandoffAnnotationAlignment::includesBoundaryOnlyFits() const
{
    QDomDocument doc = document(QStringLiteral("M90,0 L90,100"), QStringLiteral("M190,50 L190,150"));
    QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
    close(translation(group(doc, QStringLiteral("grainline"), 0)), QPointF(10, 0));
    close(translation(group(doc, QStringLiteral("grainline"), 1)), QPointF(-40, 0));
}

void TestHandoffAnnotationAlignment::jacketCaptureFitsEverySize() const
{
    // A fresh producer capture can run the same independent geometry checks.
    const QString capture = qEnvironmentVariable("HANDOFF_ANNOTATION_FIXTURE");
    const QString fixture = capture.isEmpty() ? QFINDTESTDATA(
        "../../app/seamlylayout/crates/cxxqt_bridge/test_data/jacket2_40-146_handoff.svg") : capture;
    QVERIFY(!fixture.isEmpty());
    QFile file(fixture);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QDomDocument doc;
    QVERIFY(doc.setContent(file.readAll()));
    const QStringList warnings = layout_handoff::align_handoff_annotations(doc);
    QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
    int verified_anchors = 0;
    for (int i = 0; !group(doc, QStringLiteral("piece"), i).isNull(); ++i)
    {
        const QDomElement piece = group(doc, QStringLiteral("piece"), i);
        QPolygonF cut;
        for (QDomElement child = piece.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
        {
            if (child.attribute(QStringLiteral("data-type")) == QLatin1String("cutline"))
            {
                cut = QPolygonF(coordinates(child, false));
            }
        }
        QVERIFY(cut.size() >= 3);
        for (QDomElement child = piece.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
        {
            const QString type = child.attribute(QStringLiteral("data-type"));
            if (type != QLatin1String("grainline") && type != QLatin1String("piece_label")
                && type != QLatin1String("pattern_label"))
            {
                continue;
            }
            QVector<QPointF> points = coordinates(child, type != QLatin1String("grainline"));
            if (type == QLatin1String("grainline") && points.size() >= 2)
            {
                points = {points.first(), points.last()};
            }
            QVERIFY(!points.isEmpty());
            for (const QPointF &point : points)
            {
                QVERIFY2(contains(cut, point),
                         qPrintable(QStringLiteral("%1 size %2: %3 anchor (%4,%5) outside cut line")
                                        .arg(piece.attribute(QStringLiteral("data-name")),
                                             piece.attribute(QStringLiteral("data-size")), type)
                                        .arg(point.x()).arg(point.y())));
                ++verified_anchors;
            }
        }
    }
    QVERIFY(verified_anchors > 500);
    for (int i = 0; !group(doc, QStringLiteral("piece-set"), i).isNull(); ++i)
    {
        const QDomElement    set = group(doc, QStringLiteral("piece-set"), i);
        QVector<QDomElement> pieces;
        for (QDomElement piece = set.firstChildElement(); !piece.isNull(); piece = piece.nextSiblingElement())
        {
            if (piece.attribute(QStringLiteral("data-type")) == QLatin1String("piece"))
            {
                pieces.append(piece);
            }
        }
        // SeamlyLayout stacks the sizes on the same points, so every size must keep one offset from them.
        const QVector<std::optional<QPointF>> stack_points = layout_handoff::find_stack_points(pieces);
        for (const QString &type : {QStringLiteral("grainline"), QStringLiteral("piece_label"),
                                    QStringLiteral("pattern_label")})
        {
            QPointF reference;
            bool found = false;
            for (int size = 0; size < pieces.size(); ++size)
            {
                const QDomElement piece = pieces[size];
                for (QDomElement child = piece.firstChildElement(); !child.isNull();
                     child = child.nextSiblingElement())
                {
                    if (child.attribute(QStringLiteral("data-type")) != type)
                    {
                        continue;
                    }
                    const QVector<QPointF> points = coordinates(child, type != QLatin1String("grainline"));
                    QVERIFY(!points.isEmpty());
                    const QPointF anchor = type == QLatin1String("grainline")
                        ? (points.first() + points.last()) / 2 : points.first();
                    QVERIFY(stack_points[size].has_value());
                    const QPointF offset = anchor - stack_points[size].value();
                    if (found)
                    {
                        close(offset, reference);
                    }
                    else
                    {
                        reference = offset;
                        found = true;
                    }
                }
            }
        }
    }
    QVector<QPointF> first;
    for (const QString &type : {QStringLiteral("grainline"), QStringLiteral("piece_label"),
                                QStringLiteral("pattern_label")})
    {
        for (int i = 0; !group(doc, type, i).isNull(); ++i)
        {
            first.append(translation(group(doc, type, i)));
        }
    }
    QVERIFY(first.size() > 100);
    QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
    int index = 0;
    for (const QString &type : {QStringLiteral("grainline"), QStringLiteral("piece_label"),
                                QStringLiteral("pattern_label")})
    {
        for (int i = 0; !group(doc, type, i).isNull(); ++i)
        {
            close(translation(group(doc, type, i)), first[index++]);
        }
    }
}

void TestHandoffAnnotationAlignment::stackPointTakesLongestHorizontalEdge() const
{
    // The top edge is the longest horizontal edge; its left end wins the tie between its two corners.
    same_points(find_set_stack_points(trapezoids()), {QPointF(0, 0), QPointF(100, 100)});
}

void TestHandoffAnnotationAlignment::stackPointMapsClosingIndexToFirstVertex() const
{
    // Index 4 names the closing vertex, the same place as vertex 0. Without it, corner 1 would win.
    same_points(find_set_stack_points(trapezoids(QStringLiteral("M 0,0 L 40,0 L 30,30 L 10,30 Z"),
                                                 QStringLiteral("0 1 2 3 4"),
                                                 QStringLiteral("M 100,100 L 180,100 L 160,160 L 120,160 L 100,100 Z"),
                                                 QStringLiteral("1 2 3 4"))),
                {QPointF(0, 0), QPointF(100, 100)});
}

void TestHandoffAnnotationAlignment::stackPointCentresWithoutTurnPoints() const
{
    // One size without turn points puts every size on its bounding-box centre.
    same_points(find_set_stack_points(trapezoids(QStringLiteral("M 0,0 L 40,0 L 30,30 L 10,30 Z"), QString())),
                {QPointF(20, 15), QPointF(140, 130)});
}

void TestHandoffAnnotationAlignment::stackPointTakesNearestPlaceInOtherSizes() const
{
    // Size 34 starts at (4,0). Its turn point nearest the top left of its bounding box is vertex 4 at (0,0).
    same_points(find_set_stack_points(trapezoids(QStringLiteral("M 4,0 L 40,0 L 30,30 L 10,30 L 0,0 Z"),
                                                 QStringLiteral("0 1 2 3 4 5"))),
                {QPointF(0, 0), QPointF(100, 100)});
}

void TestHandoffAnnotationAlignment::stackPointTurnsWithGrainFrame() const
{
    const QString rectangle = QStringLiteral(
        "<svg><g data-type='piece-set'><g data-type='piece' %1>"
        "<g data-type='cutline' data-turn-points='0 1 2 3'><path d='M 0,0 L 40,0 L 40,80 L 0,80 Z'/></g>%2"
        "</g></g></svg>");
    // Grain up: the 40 px edges are horizontal, and the lower left corner is SVG (0,80).
    same_points(find_set_stack_points(rectangle.arg(QStringLiteral("data-grainline-angle='90'"), QString())),
                {QPointF(0, 80)});
    // Grain to the right: the 80 px edges are horizontal, and the lower left corner is SVG (0,0).
    same_points(find_set_stack_points(rectangle.arg(QStringLiteral("data-grainline-angle='0'"), QString())),
                {QPointF(0, 0)});
    // A grainline path drawn down turns the piece 180 degrees, so the lower left corner is SVG (40,0).
    same_points(find_set_stack_points(
                    rectangle.arg(QString(), QStringLiteral("<g id='back_grainline'><path d='M 20,10 L 20,70'/></g>"))),
                {QPointF(40, 0)});
}

void TestHandoffAnnotationAlignment::stackPointTakesLowerFoldLineEnd() const
{
    // The fold line comes before the corners. The arrow group is not part of the fold line.
    const QString svg = QStringLiteral(
        "<svg><g data-type='piece-set'><g data-type='piece' data-size='34'>"
        "<g data-type='cutline' data-turn-points='0 1 2 3 4'><path d='M 0,0 L 30,0 L 30,60 L 0,60 Z'/></g>"
        "<g data-type='fold_line'><g data-type='fold_line_arrow'><path d='M 2,40 L 5,40'/></g>"
        "<path d='M 0,0 L 0,60'/></g></g>"
        "<g data-type='piece' data-size='36'>"
        "<g data-type='cutline' data-turn-points='0 1 2 3 4'>"
        "<path d='M 100,100 L 140,100 L 140,180 L 100,180 Z'/></g>"
        "<g data-type='fold_line' transform='matrix(1,0,0,1,1,0)'><path d='M 99,100 L 99,180'/></g>"
        "</g></g></svg>");
    same_points(find_set_stack_points(svg), {QPointF(0, 60), QPointF(100, 180)});
}

void TestHandoffAnnotationAlignment::stackPointNeedsThreeVertices() const
{
    same_points(find_set_stack_points(trapezoids(QStringLiteral("M 0,0 L 40,0"))),
                {std::nullopt, QPointF(100, 100)});
}
