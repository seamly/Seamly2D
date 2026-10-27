//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_piece.h
//  @author slspencer
//
//  @brief
//  Geometry of one piece in the SeamlyLayout handoff: lines, notches and internal paths.
//
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2026 Seamly2D Project
//  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
//
//  Seamly2D is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Seamly2D is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

#ifndef HANDOFF_PIECE_H
#define HANDOFF_PIECE_H

#include <QLineF>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVector>
#include <Qt>

#include <optional>

class VContainer;
class VPiece;

namespace layout_handoff
{
/**
 * @brief HandoffLine is one polyline of a piece with its node data.
 */
struct HandoffLine
{
    QVector<QPointF>            points;        ///< vertices; a closed loop does not repeat its first vertex
    std::optional<QVector<int>> turn_points;   ///< indices into points; no value = turn points unknown
    QVector<int>                node_indices;  ///< indices into points of the pattern nodes
    bool                        closed = true; ///< whether the line is a loop
};

/**
 * @brief HandoffPath is an internal path or a cutout path of a piece.
 */
struct HandoffPath
{
    HandoffLine  line;
    QString      color;
    Qt::PenStyle line_type = Qt::SolidLine;
    QString      line_weight;
    bool         cut = false; ///< true for a cutout path
};

/**
 * @brief HandoffNotches are the notch lines of a piece with a seam allowance.
 * @details The three lists after @c lines are empty when unknown. Otherwise they hold one entry per notch,
 *          and the line counts sum to the size of @c lines.
 */
struct HandoffNotches
{
    QVector<QLineF> lines;
    QVector<int>    line_counts;
    QVector<qreal>  seam_allowance_widths; ///< pixels
    QStringList     types;                 ///< notchTypeToString() values
};

/**
 * @brief HandoffPiece holds the geometry of one piece, in the piece's own pixel coordinates.
 */
class HandoffPiece
{
public:
    /**
     * @brief create reads the geometry of a pattern piece.
     * @param piece   pattern piece.
     * @param pattern pattern data; must not be null.
     */
    static HandoffPiece create(const VPiece &piece, const VContainer *pattern);

    /**
     * @brief has_shape tells whether the piece has a seam line or a cut line to draw.
     */
    bool has_shape() const;

    bool                        has_seam_allowance() const;
    bool                        has_built_in_seam_allowance() const;
    bool                        is_seam_line_hidden() const;
    const HandoffLine          &seam_line() const;
    const HandoffLine          &cut_line() const;
    const HandoffLine          &built_in_seam_line() const;
    const HandoffNotches       &notches() const;
    const QVector<HandoffPath> &internal_paths() const;
    const QVector<HandoffPath> &cutout_paths() const;

private:
    bool                 m_has_seam_allowance          = false;
    bool                 m_has_built_in_seam_allowance = false;
    bool                 m_is_seam_line_hidden         = false;
    HandoffLine          m_seam_line;
    HandoffLine          m_cut_line;
    HandoffLine          m_built_in_seam_line;
    HandoffNotches       m_notches;
    QVector<HandoffPath> m_internal_paths;
    QVector<HandoffPath> m_cutout_paths;
};
} // namespace layout_handoff

#endif // HANDOFF_PIECE_H
