// @file handoff_annotation_alignment.cpp
// @author slspencer
// @brief Fits multisize annotation anchors at one offset from their stack points.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

#include "handoff_annotation_alignment.h"

#include <QLineF>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPolygonF>
#include <QRegularExpression>
#include <QTransform>
#include <algorithm>
#include <limits>

namespace layout_handoff
{
namespace
{
constexpr qreal tolerance = 0.00001; ///< Pixel tolerance for polygon boundaries.

// @brief Finds a direct tagged child, excluding nested fold decorations.
QDomElement component(const QDomElement &parent, const QString &type)
{
    for (QDomElement child = parent.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
    {
        if (child.attribute(QStringLiteral("data-type")) == type)
        {
            return child;
        }
    }
    return {};
}

// @brief Reads the single matrix emitted on a handoff geometry group.
QTransform matrix(const QDomElement &element)
{
    const QString text = element.attribute(QStringLiteral("transform"));
    const QRegularExpression expression(QStringLiteral("^matrix\\(([^)]*)\\)$"));
    const auto match = expression.match(text);
    const QStringList values = match.captured(1).split(QRegularExpression(QStringLiteral("[,\\s]+")),
                                                      Qt::SkipEmptyParts);
    if (values.size() != 6)
    {
        return {};
    }
    return QTransform(values[0].toDouble(), values[1].toDouble(), values[2].toDouble(),
                      values[3].toDouble(), values[4].toDouble(), values[5].toDouble());
}

// @brief Collects emitted polyline vertices or text anchors in document order.
void collect(const QDomElement &element, const QTransform &parent, bool text, QVector<QPointF> &points)
{
    const QTransform transform = matrix(element) * parent;
    if (text && element.tagName() == QLatin1String("text"))
    {
        points.append(transform.map(QPointF(element.attribute(QStringLiteral("x")).toDouble(),
                                            element.attribute(QStringLiteral("y")).toDouble())));
    }
    else if (!text && element.tagName() == QLatin1String("path"))
    {
        const QRegularExpression number(QStringLiteral("[-+]?(?:\\d*\\.\\d+|\\d+\\.?\\d*)(?:[eE][-+]?\\d+)?"));
        auto matches = number.globalMatch(element.attribute(QStringLiteral("d")));
        QVector<qreal> values;
        while (matches.hasNext())
        {
            values.append(matches.next().captured().toDouble());
        }
        for (int i = 0; i + 1 < values.size(); i += 2)
        {
            points.append(transform.map(QPointF(values[i], values[i + 1])));
        }
        return;
    }
    for (QDomElement child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
    {
        if (!child.hasAttribute(QStringLiteral("data-type")))
        {
            collect(child, transform, text, points);
        }
    }
}

// @brief Projects a point onto a closed polygon segment.
QPointF project(const QPointF &point, const QPointF &start, const QPointF &end)
{
    const QPointF direction = end - start;
    const qreal squared = QPointF::dotProduct(direction, direction);
    const qreal fraction = squared == 0 ? 0 : std::clamp(QPointF::dotProduct(point - start, direction) / squared,
                                                        qreal(0), qreal(1));
    return start + fraction * direction;
}

// @brief Includes polygon edges in the containment contract.
bool inside(const QPolygonF &polygon, const QPointF &point)
{
    if (polygon.containsPoint(point, Qt::OddEvenFill))
    {
        return true;
    }
    for (int i = 0; i < polygon.size(); ++i)
    {
        if (QLineF(point, project(point, polygon[i], polygon[(i + 1) % polygon.size()])).length() <= tolerance)
        {
            return true;
        }
    }
    return false;
}

// @brief Holds one size's constraint and its initial move to the base offset.
struct Constraint
{
    QDomElement group;
    QPolygonF outline;
    QVector<QPointF> anchors;
    QPointF translation;
};

bool fits(const QVector<Constraint> &constraints, const QPointF &offset)
{
    for (const Constraint &constraint : constraints)
    {
        for (const QPointF &anchor : constraint.anchors)
        {
            if (!inside(constraint.outline, anchor + constraint.translation + offset))
            {
                return false;
            }
        }
    }
    return true;
}

// @brief Intersects the allowable translations of every anchor, then selects the closest boundary point.
bool nearest_offset(const QVector<Constraint> &constraints, QPointF &offset)
{
    if (fits(constraints, offset))
    {
        return true;
    }
    QPainterPath domain;
    bool first = true;
    for (const Constraint &constraint : constraints)
    {
        for (const QPointF &anchor : constraint.anchors)
        {
            QPolygonF polygon = constraint.outline;
            polygon.translate(-anchor - constraint.translation);
            QPainterPath allowed;
            allowed.addPolygon(polygon);
            allowed.closeSubpath();
            // Preserve a boundary-only fit when Qt clips a zero-area intersection.
            QPainterPathStroker stroker;
            stroker.setWidth(tolerance / 10);
            allowed = allowed.united(stroker.createStroke(allowed));
            domain = first ? allowed : domain.intersected(allowed);
            first = false;
            if (domain.isEmpty())
            {
                return false;
            }
        }
    }
    qreal nearest = std::numeric_limits<qreal>::max();
    for (const QPolygonF &polygon : domain.toSubpathPolygons())
    {
        for (int i = 0; i < polygon.size(); ++i)
        {
            const QPointF candidate = project({}, polygon[i], polygon[(i + 1) % polygon.size()]);
            const qreal distance = QPointF::dotProduct(candidate, candidate);
            if (distance < nearest && fits(constraints, candidate))
            {
                offset = candidate;
                nearest = distance;
            }
        }
    }
    return nearest != std::numeric_limits<qreal>::max();
}

QPolygonF outline(const QDomElement &piece)
{
    QDomElement group = component(piece, QStringLiteral("cutline"));
    if (group.isNull())
    {
        group = component(piece, QStringLiteral("seamline"));
    }
    QVector<QPointF> points;
    collect(group, {}, false, points);
    return QPolygonF(points);
}

// @brief Uses the first and last vertices of the first grain path, as the importer does.
QVector<QPointF> anchors(const QDomElement &group, bool grainline)
{
    QVector<QPointF> points;
    collect(group, {}, !grainline, points);
    if (grainline && points.size() >= 2)
    {
        return {points.first(), points.last()};
    }
    return points;
}

void align_set(QDomElement set, const QString &base_size, QStringList &warnings)
{
    QVector<QDomElement> pieces;
    QVector<QPolygonF> outlines;
    QVector<QPointF> fold_starts;
    int base = -1;
    bool folded = true;
    for (QDomElement piece = set.firstChildElement(); !piece.isNull(); piece = piece.nextSiblingElement())
    {
        if (piece.attribute(QStringLiteral("data-type")) != QLatin1String("piece"))
        {
            continue;
        }
        if (piece.attribute(QStringLiteral("data-size")) == base_size)
        {
            base = pieces.size();
        }
        pieces.append(piece);
        outlines.append(outline(piece));
        QVector<QPointF> fold;
        collect(component(piece, QStringLiteral("fold_line")), {}, false, fold);
        folded = folded && !fold.isEmpty();
        fold_starts.append(fold.isEmpty() ? QPointF() : fold.first());
    }
    if (base < 0 || std::any_of(outlines.begin(), outlines.end(), [](const QPolygonF &polygon)
        { return polygon.size() < 3; }))
    {
        return;
    }
    QVector<QPointF> stack_points;
    for (int i = 0; i < pieces.size(); ++i)
    {
        stack_points.append(folded ? fold_starts[i] : outlines[i].boundingRect().center());
    }
    const QStringList types = {QStringLiteral("grainline"), QStringLiteral("piece_label"),
                               QStringLiteral("pattern_label")};
    for (const QString &type : types)
    {
        const bool grainline = type == QLatin1String("grainline");
        const QVector<QPointF> base_anchors = anchors(component(pieces[base], type), grainline);
        if (base_anchors.isEmpty())
        {
            continue;
        }
        const QPointF base_reference = grainline ? (base_anchors.first() + base_anchors.last()) / 2
                                                  : base_anchors.first();
        const QPointF base_offset = base_reference - stack_points[base];
        QVector<Constraint> constraints;
        for (int i = 0; i < pieces.size(); ++i)
        {
            QDomElement group = component(pieces[i], type);
            const QVector<QPointF> points = anchors(group, grainline);
            if (points.isEmpty())
            {
                continue;
            }
            const QPointF reference = grainline ? (points.first() + points.last()) / 2 : points.first();
            constraints.append({group, outlines[i], points, stack_points[i] + base_offset - reference});
        }
        QPointF offset;
        if (!nearest_offset(constraints, offset))
        {
            offset = {};
            warnings.append(QStringLiteral("%1: %2 has no shared place inside every size's cut line")
                                .arg(set.attribute(QStringLiteral("data-name"),
                                                   set.attribute(QStringLiteral("id"))),
                                     QString(type).replace(QLatin1Char('_'), QLatin1Char(' '))));
        }
        for (Constraint &constraint : constraints)
        {
            const QTransform transform = matrix(constraint.group)
                * QTransform::fromTranslate((constraint.translation + offset).x(),
                                             (constraint.translation + offset).y());
            constraint.group.setAttribute(QStringLiteral("transform"), QStringLiteral("matrix(%1,%2,%3,%4,%5,%6)")
                .arg(QString::number(transform.m11(), 'g', 16), QString::number(transform.m12(), 'g', 16),
                     QString::number(transform.m21(), 'g', 16), QString::number(transform.m22(), 'g', 16),
                     QString::number(transform.dx(), 'g', 16), QString::number(transform.dy(), 'g', 16)));
        }
    }
}
}

QStringList align_handoff_annotations(QDomDocument &document)
{
    QStringList warnings;
    const QDomNodeList groups = document.elementsByTagName(QStringLiteral("g"));
    for (int i = 0; i < groups.size(); ++i)
    {
        QDomElement pattern = groups.at(i).toElement();
        if (pattern.attribute(QStringLiteral("data-type")) != QLatin1String("pattern"))
        {
            continue;
        }
        const QString base_size = pattern.attribute(QStringLiteral("data-base-size"));
        if (base_size.isEmpty())
        {
            continue;
        }
        for (QDomElement set = pattern.firstChildElement(); !set.isNull(); set = set.nextSiblingElement())
        {
            if (set.attribute(QStringLiteral("data-type")) == QLatin1String("piece-set"))
            {
                align_set(set, base_size, warnings);
            }
        }
    }
    return warnings;
}
}
