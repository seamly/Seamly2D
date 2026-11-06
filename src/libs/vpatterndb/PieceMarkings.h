//---------------------------------------------------------------------------------------------------------------------
// @file PieceMarkings.h
// @author slspencer
// @brief Defines live piece marking references and constant drill diameters.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#ifndef PIECE_MARKINGS_H
#define PIECE_MARKINGS_H

#include <QString>
#include <QVector>

class QDomElement;

/// @brief A graded center point and a positive diameter in pattern units, constant across sizes.
struct PieceDrillHole
{
    quint32 center = 0;
    qreal diameter = 0;
};

/// @brief Two graded points defining a reference-only stripe or plaid line.
struct PieceReferenceLine
{
    quint32 start = 0;
    quint32 end = 0;
    QString type;
};

/// @brief Serializes optional piece markings without changing their live point references.
class PieceMarkings
{
public:
    static QVector<PieceDrillHole> readDrillHoles(const QDomElement &piece);
    static QVector<PieceReferenceLine> readReferenceLines(const QDomElement &piece);
    static void write(QDomElement piece, const QVector<PieceDrillHole> &holes,
                      const QVector<PieceReferenceLine> &lines);
    static bool valid(const QVector<PieceDrillHole> &holes, const QVector<PieceReferenceLine> &lines,
                      const QVector<quint32> &available_points);
};

#endif // PIECE_MARKINGS_H
