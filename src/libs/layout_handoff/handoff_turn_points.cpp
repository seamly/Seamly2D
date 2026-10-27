//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_turn_points.cpp
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

#include "handoff_turn_points.h"

#include "../vpatterndb/AbstractPiece.h"

#include <QtMath>

namespace layout_handoff
{
namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief loop_node_shift finds how far a closed loop's node numbering moved between two sizes.
 * @details A built-in seam line starts at its longest edge, so another size can start it at another node.
 *          The centroids first take out the drafting offset between the sizes.
 * @param from node vertices of one size.
 * @param to   node vertices of the next size; same count as @p from.
 * @return shift s: node j of @p from is node (j + s) % n of @p to. A tie keeps the smaller shift.
 */
int loop_node_shift(const QVector<QPointF> &from, const QVector<QPointF> &to)
{
    const int n = from.size();
    QPointF from_centre;
    QPointF to_centre;
    for (int j = 0; j < n; ++j)
    {
        from_centre += from.at(j) / n;
        to_centre += to.at(j) / n;
    }

    int best_shift = 0;
    qreal best_cost = qInf();
    for (int shift = 0; shift < n; ++shift)
    {
        qreal cost = 0;
        for (int j = 0; j < n; ++j)
        {
            const QPointF d = (from.at(j) - from_centre) - (to.at((j + shift) % n) - to_centre);
            cost += QPointF::dotProduct(d, d);
        }
        if (cost < best_cost)
        {
            best_cost = cost;
            best_shift = shift;
        }
    }
    return best_shift;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
QVector<QVector<int>> unify_turn_points(const QVector<SizeLine> &lines, bool closed)
{
    QVector<QVector<int>> turns;
    const int node_count = lines.isEmpty() ? 0 : lines.first().node_indices.size();
    bool usable = lines.size() >= 2 && node_count > 0;
    for (int s = 0; usable && s < lines.size(); ++s)
    {
        usable = lines.at(s).turn_points.has_value() && lines.at(s).node_indices.size() == node_count;
    }
    if (!usable)
    {
        return turns;
    }

    // Node k of the first size is node (k + offsets[s]) % node_count of size s. An open line always starts at
    // the same node; a closed loop is matched to the size before it.
    QVector<int> offsets(lines.size(), 0);
    if (closed)
    {
        QVector<QPointF> previous;
        for (int s = 0; s < lines.size(); ++s)
        {
            QVector<QPointF> nodes;
            for (int i : lines.at(s).node_indices)
            {
                nodes.append(lines.at(s).points.at(i));
            }
            if (s > 0)
            {
                offsets[s] = (offsets.at(s - 1) + loop_node_shift(previous, nodes)) % node_count;
            }
            previous = nodes;
        }
    }

    // A node is a turn point when it is one in any size.
    QVector<bool> turn_nodes(node_count, false);
    for (int s = 0; s < lines.size(); ++s)
    {
        const SizeLine &line = lines.at(s);
        for (int k = 0; k < node_count; ++k)
        {
            if (line.turn_points->contains(line.node_indices.at((k + offsets.at(s)) % node_count)))
            {
                turn_nodes[k] = true;
            }
        }
    }

    // Give every size the turn nodes, at that size's own vertex indices.
    for (int s = 0; s < lines.size(); ++s)
    {
        const SizeLine &line = lines.at(s);
        QVector<int> added;
        for (int k = 0; k < node_count; ++k)
        {
            if (turn_nodes.at(k))
            {
                added.append(line.node_indices.at((k + offsets.at(s)) % node_count));
            }
        }
        turns.append(AbstractPiece::addTurnPoints(line.points, line.turn_points.value(), added, closed));
    }
    return turns;
}
} // namespace layout_handoff
