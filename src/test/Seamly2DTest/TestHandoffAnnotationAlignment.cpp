// @file TestHandoffAnnotationAlignment.cpp
// @author slspencer
// @brief Tests the shared multisize annotation offset without application state.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

#include "TestHandoffAnnotationAlignment.h"
#include "../../libs/layout_handoff/handoff_annotation_alignment.h"

#include <QDomDocument>
#include <QFile>
#include <QLineF>
#include <QPolygonF>
#include <QRegularExpression>
#include <QTransform>
#include <QtTest>

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

void TestHandoffAnnotationAlignment::usesFoldStartOnlyWhenEverySizeHasOne() const
{
    for (bool all_folded : {false, true})
    {
        QDomDocument doc = document(QStringLiteral("M20,30 L20,50"), QStringLiteral("M30,40 L30,60"));
        for (int i = 0; i < (all_folded ? 2 : 1); ++i)
        {
            QDomElement fold = doc.createElement(QStringLiteral("g"));
            fold.setAttribute(QStringLiteral("data-type"), QStringLiteral("fold_line"));
            QDomElement path = doc.createElement(QStringLiteral("path"));
            path.setAttribute(QStringLiteral("d"), i == 0 ? QStringLiteral("M0,0 L0,100")
                                                          : QStringLiteral("M0,10 L0,200"));
            fold.appendChild(path);
            group(doc, QStringLiteral("piece"), i).appendChild(fold);
        }
        QVERIFY(layout_handoff::align_handoff_annotations(doc).isEmpty());
        // A mixed set uses outline centres and fits at the nearest feasible offset.
        close(translation(group(doc, QStringLiteral("grainline"), 0)),
              all_folded ? QPointF(10, 0) : QPointF(-20, -30));
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
    const QString fixture = QFINDTESTDATA(
        "../../app/seamlylayout/crates/cxxqt_bridge/test_data/jacket2_40-146_handoff.svg");
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
        const QDomElement set = group(doc, QStringLiteral("piece-set"), i);
        QVector<QDomElement> pieces;
        QVector<QPointF> stack_points;
        QVector<QPointF> fold_starts;
        bool folded = true;
        for (QDomElement piece = set.firstChildElement(); !piece.isNull(); piece = piece.nextSiblingElement())
        {
            QVector<QPointF> cut;
            QVector<QPointF> fold;
            for (QDomElement child = piece.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
            {
                if (child.attribute(QStringLiteral("data-type")) == QLatin1String("cutline"))
                {
                    cut = coordinates(child, false);
                }
                if (child.attribute(QStringLiteral("data-type")) == QLatin1String("fold_line"))
                {
                    fold = coordinates(child, false);
                }
            }
            pieces.append(piece);
            stack_points.append(QPolygonF(cut).boundingRect().center());
            fold_starts.append(fold.isEmpty() ? QPointF() : fold.first());
            folded = folded && !fold.isEmpty();
        }
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
                    const QPointF offset = anchor - (folded ? fold_starts[size] : stack_points[size]);
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
