//---------------------------------------------------------------------------------------------------------------------
// @file PieceMarkings.cpp
// @author slspencer
// @brief Reads, writes, and validates live piece markings.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "PieceMarkings.h"
#include <QDomDocument>
#include <QDomElement>
#include <QtMath>

QVector<PieceDrillHole> PieceMarkings::readDrillHoles(const QDomElement &piece)
{
    QVector<PieceDrillHole> holes;
    for (QDomElement hole = piece.firstChildElement(QStringLiteral("drillHoles")).firstChildElement();
         !hole.isNull(); hole = hole.nextSiblingElement())
    {
        holes.append({hole.attribute(QStringLiteral("center")).toUInt(),
                      hole.attribute(QStringLiteral("diameter")).toDouble()});
    }
    return holes;
}

QVector<PieceReferenceLine> PieceMarkings::readReferenceLines(const QDomElement &piece)
{
    QVector<PieceReferenceLine> lines;
    for (QDomElement line = piece.firstChildElement(QStringLiteral("referenceLines")).firstChildElement();
         !line.isNull(); line = line.nextSiblingElement())
    {
        lines.append({line.attribute(QStringLiteral("start")).toUInt(), line.attribute(QStringLiteral("end")).toUInt(),
                      line.attribute(QStringLiteral("type"))});
    }
    return lines;
}

void PieceMarkings::write(QDomElement piece, const QVector<PieceDrillHole> &holes,
                          const QVector<PieceReferenceLine> &lines)
{
    piece.removeChild(piece.firstChildElement(QStringLiteral("drillHoles")));
    piece.removeChild(piece.firstChildElement(QStringLiteral("referenceLines")));
    QDomDocument document = piece.ownerDocument();
    if (!holes.isEmpty())
    {
        QDomElement section = document.createElement(QStringLiteral("drillHoles"));
        for (const PieceDrillHole &hole : holes)
        {
            QDomElement element = document.createElement(QStringLiteral("hole"));
            element.setAttribute(QStringLiteral("center"), hole.center);
            element.setAttribute(QStringLiteral("diameter"), QString::number(hole.diameter, 'g', 17));
            section.appendChild(element);
        }
        piece.appendChild(section);
    }
    if (!lines.isEmpty())
    {
        QDomElement section = document.createElement(QStringLiteral("referenceLines"));
        for (const PieceReferenceLine &line : lines)
        {
            QDomElement element = document.createElement(QStringLiteral("line"));
            element.setAttribute(QStringLiteral("start"), line.start);
            element.setAttribute(QStringLiteral("end"), line.end);
            element.setAttribute(QStringLiteral("type"), line.type);
            section.appendChild(element);
        }
        piece.appendChild(section);
    }
}

bool PieceMarkings::valid(const QVector<PieceDrillHole> &holes, const QVector<PieceReferenceLine> &lines,
                          const QVector<quint32> &available_points)
{
    for (const PieceDrillHole &hole : holes)
    {
        if (hole.center == 0 || !available_points.contains(hole.center)
            || !qIsFinite(hole.diameter) || hole.diameter <= 0)
        {
            return false;
        }
    }
    for (const PieceReferenceLine &line : lines)
    {
        if (line.start == 0 || line.end == 0 || line.start == line.end
            || !available_points.contains(line.start) || !available_points.contains(line.end)
            || (line.type != QStringLiteral("stripe") && line.type != QStringLiteral("plaid")))
        {
            return false;
        }
    }
    return true;
}
