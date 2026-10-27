//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_turn_points.h
//  @author slspencer
//
//  @brief
//  Turn points shared by every size of a multisize handoff piece line.
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

#ifndef HANDOFF_TURN_POINTS_H
#define HANDOFF_TURN_POINTS_H

#include <QPointF>
#include <QVector>

#include <optional>

namespace layout_handoff
{
/**
 * @brief SizeLine is one size of one piece line.
 */
struct SizeLine
{
    QVector<QPointF>            points;       ///< polyline vertices
    std::optional<QVector<int>> turn_points;  ///< indices into points; no value = turn points unknown
    QVector<int>                node_indices; ///< indices into points of the pattern nodes
};

/**
 * @brief unify_turn_points makes a node that is a turn point in one size a turn point in every size.
 * @details Nodes match by their order along the line. Turn points away from a node stay as they are.
 *          A closed loop may start at another node in another size; the best-fitting shift matches the nodes.
 * @param lines  the same line in every size, in size order.
 * @param closed whether the line is a closed loop.
 * @return the turn points of each size, parallel to @p lines. Empty when the line is skipped: fewer than two
 *         sizes, no nodes, a size with unknown turn points, or sizes that differ in node count.
 */
QVector<QVector<int>> unify_turn_points(const QVector<SizeLine> &lines, bool closed);
} // namespace layout_handoff

#endif // HANDOFF_TURN_POINTS_H
