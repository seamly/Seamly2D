// @file handoff_stack_point.h
// @author slspencer
// @brief Finds the stack point of each size of a multisize piece: the point where SeamlyLayout stacks the sizes.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

#ifndef HANDOFF_STACK_POINT_H
#define HANDOFF_STACK_POINT_H

#include <QDomElement>
#include <QPointF>
#include <QVector>

#include <optional>

namespace layout_handoff
{
/**
 * @brief Finds the stack point of each size of one piece-set, in each piece's own frame.
 * @details SeamlyLayout's `svg_dom/src/stack_point.rs` has the same rules. The Nested tab stacks the sizes on
 *          this point, so the two must agree for the annotations of every size to stack exactly.
 *          The rules measure each size in its grain frame: x' runs across the grain, y' against it.
 *          1. Fold line: when every size has one, the lower end of the largest size's fold line, or its left end
 *             when the fold line is level. Every size uses the same end, start or end.
 *          2. Corner: when every size has turn points, the turn point of the largest size with the longest
 *             horizontal edge; with no horizontal edge, the one with the longest edge. On a tie: the longer
 *             other edge, then the lower corner, then the left one. Every other size uses the turn point
 *             nearest the same place in its bounding box.
 *          3. Centre: else the centre of each size's outline bounding box.
 *          The largest size is the first one with the largest bounding-box area.
 *          Only the polylines and matrix transforms that HandoffDocument writes are read.
 * @param pieces the `piece` groups of one piece-set, in document order.
 * @return one point per piece; no value for a piece whose cut line (else seam line) has fewer than three vertices.
 */
QVector<std::optional<QPointF>> find_stack_points(const QVector<QDomElement>& pieces);
} // namespace layout_handoff

#endif // HANDOFF_STACK_POINT_H
