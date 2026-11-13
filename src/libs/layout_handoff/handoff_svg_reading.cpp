// @file handoff_svg_reading.cpp
// @author slspencer
// @brief Reads tagged groups, matrix transforms, path vertices and text anchors back from the handoff SVG.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

#include "handoff_svg_reading.h"

#include <QRegularExpression>
#include <QStringList>

namespace layout_handoff
{
//---------------------------------------------------------------------------------------------------------------------
QDomElement find_tagged_child(const QDomElement& parent, const QString& type)
{
    QDomElement found;
    for (QDomElement child = parent.firstChildElement(); found.isNull() && !child.isNull();
         child = child.nextSiblingElement())
    {
        if (child.attribute(QStringLiteral("data-type")) == type)
        {
            found = child;
        }
    }
    return found;
}

//---------------------------------------------------------------------------------------------------------------------
QTransform read_matrix(const QDomElement& element)
{
    static const QRegularExpression matrix_expression(QStringLiteral("^matrix\\(([^)]*)\\)$"));
    static const QRegularExpression separator(QStringLiteral("[,\\s]+"));

    QTransform transform;
    const QRegularExpressionMatch match = matrix_expression.match(element.attribute(QStringLiteral("transform")));
    const QStringList values = match.captured(1).split(separator, Qt::SkipEmptyParts);
    if (values.size() == 6)
    {
        transform = QTransform(values[0].toDouble(), values[1].toDouble(), values[2].toDouble(),
                               values[3].toDouble(), values[4].toDouble(), values[5].toDouble());
    }
    return transform;
}

//---------------------------------------------------------------------------------------------------------------------
void collect_points(const QDomElement& element, const QTransform& parent, bool text, QVector<QPointF>& points)
{
    static const QRegularExpression number(QStringLiteral("[-+]?(?:\\d*\\.\\d+|\\d+\\.?\\d*)(?:[eE][-+]?\\d+)?"));

    const QTransform transform = read_matrix(element) * parent;
    if (text && element.tagName() == QLatin1String("text"))
    {
        points.append(transform.map(QPointF(element.attribute(QStringLiteral("x")).toDouble(),
                                            element.attribute(QStringLiteral("y")).toDouble())));
    }
    if (!text && element.tagName() == QLatin1String("path"))
    {
        // Each number pair of the polyline data is one vertex.
        QRegularExpressionMatchIterator matches = number.globalMatch(element.attribute(QStringLiteral("d")));
        QVector<qreal> values;
        while (matches.hasNext())
        {
            values.append(matches.next().captured().toDouble());
        }
        for (int i = 0; i + 1 < values.size(); i += 2)
        {
            points.append(transform.map(QPointF(values[i], values[i + 1])));
        }
    }
    else
    {
        // A tagged child group is another part of the piece, such as the arrow of a fold line.
        for (QDomElement child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
        {
            if (!child.hasAttribute(QStringLiteral("data-type")))
            {
                collect_points(child, transform, text, points);
            }
        }
    }
}
} // namespace layout_handoff
