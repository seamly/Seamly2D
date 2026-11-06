//---------------------------------------------------------------------------------------------------------------------
// @file PatternPieceCopy.h
// @author slspencer
// @brief Reconstructs independent modeling dependencies for a copied piece.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#ifndef PATTERN_PIECE_COPY_H
#define PATTERN_PIECE_COPY_H

#include <QDomDocument>
#include <QHash>
#include <QVector>
#include <functional>

/// @brief Holds a copied modeling element and its original draft block.
struct CopiedModelingElement
{
    QString block_name;
    QDomElement element;
};

/// @brief Holds detached reconstruction elements; the source document remains unchanged.
struct PatternPieceCopyResult
{
    QString block_name;
    QDomElement piece;
    QVector<CopiedModelingElement> modeling;
    QHash<quint32, quint32> ids;
    QString error;
};

/// @brief Copies modeling dependencies while retaining references to live calculation objects.
class PatternPieceCopy
{
public:
    /// @brief Builds an independent piece, including nested Union inputs, paths, and anchors.
    /// @param next_id Supplies unused document-wide IDs.
    static PatternPieceCopyResult reconstruct(const QDomDocument &document, quint32 piece_id,
                                               const std::function<quint32()> &next_id);
};

#endif // PATTERN_PIECE_COPY_H
