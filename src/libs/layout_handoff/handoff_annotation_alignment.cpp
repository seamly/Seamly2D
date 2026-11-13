// @file handoff_annotation_alignment.cpp
// @author slspencer
// @brief Fits multisize annotation anchors at one offset from their stack points.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

#include "handoff_annotation_alignment.h"

#include "handoff_stack_point.h"
#include "handoff_svg_reading.h"

#include <QLineF>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPolygonF>
#include <QTransform>
#include <algorithm>
#include <limits>
#include <optional>

namespace layout_handoff
{
namespace
{
constexpr qreal tolerance = 0.00001; ///< Pixel tolerance for polygon boundaries.

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

// @brief Reads the cut line, else the seam line, as a polygon in the piece frame.
QPolygonF outline(const QDomElement& piece)
{
    QDomElement group = find_tagged_child(piece, QStringLiteral("cutline"));
    if (group.isNull())
    {
        group = find_tagged_child(piece, QStringLiteral("seamline"));
    }
    QVector<QPointF> points;
    collect_points(group, QTransform(), false, points);
    return QPolygonF(points);
}

// @brief Uses the first and last vertices of the first grain path, as the importer does.
QVector<QPointF> anchors(const QDomElement& group, bool grainline)
{
    QVector<QPointF> points;
    collect_points(group, QTransform(), !grainline, points);
    if (grainline && points.size() >= 2)
    {
        points = {points.constFirst(), points.constLast()};
    }
    return points;
}

// @brief Moves each annotation of every size of one piece-set to the base size's offset from its stack point.
void align_set(QDomElement set, const QString& base_size, QStringList& warnings)
{
    QVector<QDomElement> pieces;
    QVector<QPolygonF>   outlines;
    int                  base = -1;
    for (QDomElement piece = set.firstChildElement(); !piece.isNull(); piece = piece.nextSiblingElement())
    {
        if (piece.attribute(QStringLiteral("data-type")) == QLatin1String("piece"))
        {
            if (piece.attribute(QStringLiteral("data-size")) == base_size)
            {
                base = pieces.size();
            }
            pieces.append(piece);
            outlines.append(outline(piece));
        }
    }

    // Every size needs a stack point: SeamlyLayout stacks the sizes on it.
    const QVector<std::optional<QPointF>> stack_points = find_stack_points(pieces);
    const bool                            measured =
        std::all_of(stack_points.cbegin(), stack_points.cend(),
                    [](const std::optional<QPointF>& point) { return point.has_value(); });
    if (base >= 0 && measured)
    {
        const QStringList types = {QStringLiteral("grainline"), QStringLiteral("piece_label"),
                                   QStringLiteral("pattern_label")};
        for (const QString& type : types)
        {
            const bool             grainline    = type == QLatin1String("grainline");
            const QVector<QPointF> base_anchors = anchors(find_tagged_child(pieces[base], type), grainline);
            if (base_anchors.isEmpty())
            {
                continue;
            }
            const QPointF base_reference =
                grainline ? (base_anchors.first() + base_anchors.last()) / 2 : base_anchors.first();
            const QPointF       base_offset = base_reference - stack_points[base].value();
            QVector<Constraint> constraints;
            for (int i = 0; i < pieces.size(); ++i)
            {
                QDomElement            group  = find_tagged_child(pieces[i], type);
                const QVector<QPointF> points = anchors(group, grainline);
                if (!points.isEmpty())
                {
                    const QPointF reference = grainline ? (points.first() + points.last()) / 2 : points.first();
                    constraints.append({group, outlines[i], points, stack_points[i].value() + base_offset - reference});
                }
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
            for (Constraint& constraint : constraints)
            {
                const QTransform transform = read_matrix(constraint.group)
                    * QTransform::fromTranslate((constraint.translation + offset).x(),
                                                (constraint.translation + offset).y());
                constraint.group.setAttribute(
                    QStringLiteral("transform"),
                    QStringLiteral("matrix(%1,%2,%3,%4,%5,%6)")
                        .arg(QString::number(transform.m11(), 'g', 16), QString::number(transform.m12(), 'g', 16),
                             QString::number(transform.m21(), 'g', 16), QString::number(transform.m22(), 'g', 16),
                             QString::number(transform.dx(), 'g', 16), QString::number(transform.dy(), 'g', 16)));
            }
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
