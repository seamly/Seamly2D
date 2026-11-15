// @file handoff_stack_point.cpp
// @author slspencer
// @brief Finds the stack point of each size of a multisize piece: the point where SeamlyLayout stacks the sizes.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

#include "handoff_stack_point.h"

#include "handoff_svg_reading.h"

#include <QLineF>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace layout_handoff
{
namespace
{
/// @brief Distance at or below which two points are one point, and two lengths or places tie, in px.
constexpr qreal same_px = 0.01;

/// @brief Distance from an edge's line at or below which a vertex continues the edge, in px.
constexpr qreal on_edge_px = 0.5;

/// @brief Sine of 1 degree: an edge whose direction rises or falls by this much or less is horizontal.
constexpr qreal horizontal_sine = 0.01745240643728351;

/// @brief Cosine of 89 degrees: two edges that meet within 1 degree of 90 degrees make a right angle.
constexpr qreal right_angle_cosine = 0.01745240643728351;

/// @brief Distance at or below which a grainline point repeats the first point, per axis, in px.
constexpr qreal grainline_same_px = 0.001;

/// @brief Rotation below which SeamlyLayout leaves a grainline as it is, in degrees.
constexpr qreal vertical_degrees = 0.1;

/// @brief One size's outline, turn points and fold line, measured in its grain frame.
struct SizeOutline
{
    QVector<QPointF>            vertices;    ///< outline vertices, without the closing vertex that repeats the first
    std::optional<QVector<int>> turn_points; ///< sorted unique indices into vertices; no value without data-turn-points
    std::optional<QLineF>       fold_line;   ///< fold line from its start to its end
    QPointF                     grain;       ///< grain direction unit vector, in the piece frame
};

/// @brief One corner of an outline, with the edge lengths that rank it.
struct Corner
{
    int   index       = 0;     ///< vertex index
    bool  horizontal  = false; ///< true when one of its two edges is horizontal
    bool  right_angle = false; ///< true when its two edges meet at a right angle
    qreal primary     = 0;     ///< the horizontal edge's length when only one edge is horizontal, else the longer edge
    qreal secondary   = 0;     ///< the other edge's length
};

/**
 * @brief Maps a piece frame point to the grain frame: x' = p.r and y' = -(p.g), with r = (-g.y, g.x).
 */
QPointF to_frame(const QPointF& point, const QPointF& grain)
{
    return QPointF(-point.x() * grain.y() + point.y() * grain.x(), -(point.x() * grain.x() + point.y() * grain.y()));
}

/**
 * @brief Maps a grain frame point back to the piece frame: p = x'.r - y'.g.
 */
QPointF to_piece(const QPointF& point, const QPointF& grain)
{
    return QPointF(-point.x() * grain.y() - point.y() * grain.x(), point.x() * grain.x() - point.y() * grain.y());
}

/**
 * @brief Returns the distance between two points.
 */
qreal distance(const QPointF& a, const QPointF& b)
{
    return std::hypot(a.x() - b.x(), a.y() - b.y());
}

/**
 * @brief Returns true when @p a is lower than @p b, or level with @p b and left of it, in the grain frame.
 */
bool lower_or_left(const QPointF& a, const QPointF& b)
{
    // Not level: the larger y' is lower.
    bool result = a.y() > b.y();
    if (std::abs(a.y() - b.y()) <= same_px)
    {
        result = a.x() < b.x() - same_px;
    }
    return result;
}

/**
 * @brief Returns the bounding box of the outline vertices.
 */
QRectF bounds(const SizeOutline& size)
{
    return QPolygonF(size.vertices).boundingRect();
}

/**
 * @brief Returns the bounding-box area of the outline.
 */
qreal area(const SizeOutline& size)
{
    const QRectF box = bounds(size);
    return box.width() * box.height();
}

/**
 * @brief Returns the place of a point in the bounding box: 0 at the left or top, 1 at the right or bottom.
 * @details A side of zero length puts every point at 0.5 along it.
 */
QPointF place_in_bounds(const SizeOutline& size, const QPointF& point)
{
    const QRectF box      = bounds(size);
    const auto   fraction = [](qreal value, qreal low, qreal high)
    {
        return high - low > same_px ? (value - low) / (high - low) : 0.5;
    };
    return QPointF(fraction(point.x(), box.left(), box.right()), fraction(point.y(), box.top(), box.bottom()));
}

/**
 * @brief Returns the turn point whose place in the bounding box is nearest @p place; the first one on a tie.
 * @return the vertex index; -1 when the outline has no turn points.
 */
int nearest_turn_point(const SizeOutline& size, const QPointF& place)
{
    int   nearest = -1;
    qreal gap     = std::numeric_limits<qreal>::infinity();
    for (const int index : size.turn_points.value_or(QVector<int>()))
    {
        const qreal index_gap = distance(place_in_bounds(size, size.vertices.at(index)), place);
        if (index_gap < gap)
        {
            nearest = index;
            gap     = index_gap;
        }
    }
    return nearest;
}

/**
 * @brief Returns true when @p index is a turn point of the outline.
 */
bool is_turn_point(const SizeOutline& size, int index)
{
    return size.turn_points.has_value()
        && std::binary_search(size.turn_points->cbegin(), size.turn_points->cend(), index);
}

/**
 * @brief Measures the straight edge that leaves the vertex @p corner.
 * @details The first vertex away from the corner sets the edge direction. The edge goes on through the vertices
 *          within on_edge_px of that line that move away from the corner, and stops at a turn point.
 * @param size       the outline.
 * @param corner     the vertex index of the corner.
 * @param forward    true follows the vertex order; false goes against it.
 * @param direction  receives the unit direction of the edge; (0, 0) when every vertex is on the corner.
 * @return the edge length; 0 when every vertex is on the corner.
 */
qreal edge_length(const SizeOutline& size, int corner, bool forward, QPointF& direction)
{
    const int     count  = size.vertices.size();
    const auto    step   = [count, forward](int k) { return forward ? (k + 1) % count : (k + count - 1) % count; };
    const QPointF origin = size.vertices.at(corner);
    qreal         length = 0;
    direction            = QPointF();

    // Skip the vertices on the corner itself.
    int k = step(corner);
    while (k != corner && distance(size.vertices.at(k), origin) <= same_px)
    {
        k = step(k);
    }
    if (k != corner)
    {
        length                = distance(size.vertices.at(k), origin);
        const QPointF unit    = (size.vertices.at(k) - origin) / length;
        bool          on_edge = true;
        while (on_edge && !is_turn_point(size, k))
        {
            // The edge ends where the outline comes back to the corner, leaves the line, or turns back.
            const int     next     = step(k);
            const QPointF offset   = size.vertices.at(next) - origin;
            const qreal   off_line = std::abs(offset.x() * unit.y() - offset.y() * unit.x());
            const qreal   along    = QPointF::dotProduct(offset, unit);
            on_edge                = next != corner && off_line <= on_edge_px && along >= length;
            if (on_edge)
            {
                length = along;
                k      = next;
            }
        }
        direction = unit;
    }
    return length;
}

/**
 * @brief Measures both edges of the corner at vertex @p index.
 */
Corner make_corner(const SizeOutline& size, int index)
{
    QPointF     before_unit;
    QPointF     after_unit;
    const qreal before            = edge_length(size, index, false, before_unit);
    const qreal after             = edge_length(size, index, true, after_unit);
    const bool  before_horizontal = before > 0 && std::abs(before_unit.y()) <= horizontal_sine;
    const bool  after_horizontal  = after > 0 && std::abs(after_unit.y()) <= horizontal_sine;

    Corner corner;
    corner.index       = index;
    corner.horizontal  = before_horizontal || after_horizontal;
    corner.right_angle = before > 0 && after > 0
                         && std::abs(QPointF::dotProduct(before_unit, after_unit)) <= right_angle_cosine;
    // Both edges or neither edge horizontal: the longer edge ranks first.
    corner.primary   = std::max(before, after);
    corner.secondary = std::min(before, after);
    if (before_horizontal != after_horizontal)
    {
        // Only one edge is horizontal: it ranks first.
        corner.primary   = before_horizontal ? before : after;
        corner.secondary = before_horizontal ? after : before;
    }
    return corner;
}

/**
 * @brief Returns true when corner @p a ranks above corner @p b: longer primary edge, longer secondary edge, lower,
 *        then left, each by more than same_px.
 */
bool ranks_above(const SizeOutline& size, const Corner& a, const Corner& b)
{
    bool above = lower_or_left(size.vertices.at(a.index), size.vertices.at(b.index));
    if (std::abs(a.primary - b.primary) > same_px)
    {
        above = a.primary > b.primary;
    }
    else if (std::abs(a.secondary - b.secondary) > same_px)
    {
        above = a.secondary > b.secondary;
    }
    return above;
}

/**
 * @brief Returns true when corner @p a is lower than corner @p b by more than same_px; when they are level, when
 *        @p a ranks above @p b.
 */
bool ranks_lower(const SizeOutline& size, const Corner& a, const Corner& b)
{
    const qreal a_y   = size.vertices.at(a.index).y();
    const qreal b_y   = size.vertices.at(b.index).y();
    bool        lower = ranks_above(size, a, b);
    if (std::abs(a_y - b_y) > same_px)
    {
        lower = a_y > b_y; // not level: the larger y' is lower
    }
    return lower;
}

/**
 * @brief Finds the corner of the outline that every size stacks on.
 * @details With a right-angle corner: the corner with the longest horizontal edge, else with the longest edge.
 *          With no right-angle corner: the lowest corner, so the sizes align along its longest edge.
 * @return the vertex index; -1 when the outline has no turn points.
 */
int best_corner(const SizeOutline& size)
{
    QVector<Corner> corners;
    bool            any_horizontal  = false;
    bool            any_right_angle = false;
    for (const int index : size.turn_points.value_or(QVector<int>()))
    {
        corners.append(make_corner(size, index));
        any_horizontal  = any_horizontal || corners.constLast().horizontal;
        any_right_angle = any_right_angle || corners.constLast().right_angle;
    }

    // Ascending index order: on a full tie the first corner stays.
    int best = -1;
    for (int i = 0; i < corners.size(); ++i)
    {
        // With a right angle and a horizontal edge, only the corners on a horizontal edge compete.
        const bool candidate = !any_right_angle || !any_horizontal || corners.at(i).horizontal;
        const bool better    = best < 0
                            || (any_right_angle ? ranks_above(size, corners.at(i), corners.at(best))
                                                : ranks_lower(size, corners.at(i), corners.at(best)));
        if (candidate && better)
        {
            best = i;
        }
    }
    return best < 0 ? -1 : corners.at(best).index;
}

/**
 * @brief Finds the first element in the subtree of @p element, itself first, whose id contains @p token.
 * @return the element; a null element when no id contains the token.
 */
QDomElement find_element_with_id_token(const QDomElement& element, const QString& token)
{
    QDomElement found;
    if (element.attribute(QStringLiteral("id")).contains(token, Qt::CaseInsensitive))
    {
        found = element;
    }
    for (QDomElement child = element.firstChildElement(); found.isNull() && !child.isNull();
         child = child.nextSiblingElement())
    {
        found = find_element_with_id_token(child, token);
    }
    return found;
}

/**
 * @brief Finds the first `path` element: @p element itself, else a direct child, else a deeper descendant.
 * @return the path; a null element when the subtree has no path.
 */
QDomElement find_first_path(const QDomElement& element)
{
    QDomElement found = element.tagName() == QLatin1String("path") ? element
                                                                   : element.firstChildElement(QStringLiteral("path"));
    for (QDomElement child = element.firstChildElement(); found.isNull() && !child.isNull();
         child = child.nextSiblingElement())
    {
        found = find_first_path(child);
    }
    return found;
}

/**
 * @brief Reads the grainline angle of a piece, in degrees from the x axis, clockwise as SVG draws it.
 * @details The first path of the first element whose id contains "grainline", from its first point to its last
 *          point that differs from the first; its own coordinates, without transforms. Else the negated
 *          `data-grainline-angle` attribute, which counts counter-clockwise.
 * @return the angle; no value when the piece has no grain direction.
 */
std::optional<qreal> read_grainline_angle(const QDomElement& piece)
{
    std::optional<qreal> angle;
    const QDomElement    path = find_first_path(find_element_with_id_token(piece, QStringLiteral("grainline")));
    QVector<QPointF>     points;
    if (!path.isNull())
    {
        collect_points(path, QTransform(), false, points);
    }
    if (points.size() >= 2)
    {
        // The last point that differs from the first one: a closed arrow path comes back to its start.
        const QPointF first = points.constFirst();
        QPointF       last  = points.constLast();
        bool          found = false;
        for (int i = points.size() - 1; !found && i >= 1; --i)
        {
            found = std::abs(points.at(i).x() - first.x()) > grainline_same_px
                 || std::abs(points.at(i).y() - first.y()) > grainline_same_px;
            if (found)
            {
                last = points.at(i);
            }
        }
        const QPointF delta = last - first;
        if (std::abs(delta.x()) + std::abs(delta.y()) >= 1e-6)
        {
            angle = qRadiansToDegrees(std::atan2(delta.y(), delta.x()));
        }
    }
    if (!angle.has_value())
    {
        bool        ok      = false;
        const qreal degrees = piece.attribute(QStringLiteral("data-grainline-angle")).trimmed().toDouble(&ok);
        if (ok && std::isfinite(degrees))
        {
            angle = -degrees;
        }
    }
    return angle;
}

/**
 * @brief Returns the grain direction of a piece as a unit vector in its own frame.
 * @details The direction that SeamlyLayout turns up. A piece that it does not turn points up: no grain direction,
 *          or a grainline within vertical_degrees of vertical.
 */
QPointF find_grain_direction(const QDomElement& piece)
{
    QPointF                    grain(0, -1);
    const std::optional<qreal> angle = read_grainline_angle(piece);
    if (angle.has_value())
    {
        // The rotation that turns the grainline up, normalized to [-180, 180].
        qreal rotation = -90 - angle.value();
        while (rotation > 180)
        {
            rotation -= 360;
        }
        while (rotation < -180)
        {
            rotation += 360;
        }
        if (std::abs(rotation) >= vertical_degrees)
        {
            grain = QPointF(std::cos(qDegreesToRadians(angle.value())), std::sin(qDegreesToRadians(angle.value())));
        }
    }
    return grain;
}

/**
 * @brief Reads one size's outline, turn points and fold line, in its grain frame.
 * @details The outline is the first `cutline` group, else the first `seamline` group. A turn point that names the
 *          closing vertex names the first vertex; one past the vertices is dropped.
 * @return the outline; no value when it has fewer than three vertices.
 */
std::optional<SizeOutline> read_size_outline(const QDomElement& piece, const QPointF& grain)
{
    std::optional<SizeOutline> result;
    QDomElement                group = find_tagged_child(piece, QStringLiteral("cutline"));
    if (group.isNull())
    {
        group = find_tagged_child(piece, QStringLiteral("seamline"));
    }
    QVector<QPointF> points;
    if (!group.isNull())
    {
        collect_points(group, QTransform(), false, points);
    }

    QVector<QPointF> vertices;
    for (const QPointF& point : points)
    {
        vertices.append(to_frame(point, grain));
    }
    const int  count  = vertices.size();
    const bool closed = count >= 2 && distance(vertices.constFirst(), vertices.constLast()) <= same_px;
    if (closed)
    {
        vertices.removeLast();
    }

    if (vertices.size() >= 3)
    {
        SizeOutline size;
        size.vertices = vertices;
        size.grain    = grain;
        if (group.hasAttribute(QStringLiteral("data-turn-points")))
        {
            QVector<int>      indices;
            const QString     text_list = group.attribute(QStringLiteral("data-turn-points")).simplified();
            const QStringList texts     = text_list.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            for (const QString& text : texts)
            {
                bool ok    = false;
                int  index = text.toInt(&ok);
                if (ok && closed && index == count - 1)
                {
                    index = 0;
                }
                if (ok && index >= 0 && index < vertices.size())
                {
                    indices.append(index);
                }
            }
            std::sort(indices.begin(), indices.end());
            indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
            size.turn_points = indices;
        }

        // The fold line is the path outside the tagged arrow and label groups.
        QVector<QPointF> fold_points;
        collect_points(find_tagged_child(piece, QStringLiteral("fold_line")), QTransform(), false, fold_points);
        if (fold_points.size() >= 2)
        {
            size.fold_line =
                QLineF(to_frame(fold_points.constFirst(), grain), to_frame(fold_points.constLast(), grain));
        }
        result = size;
    }
    return result;
}

/**
 * @brief Applies the three rules to every size.
 * @return one grain frame point per size; no value for a size without an outline.
 */
QVector<std::optional<QPointF>> find_frame_points(const QVector<std::optional<SizeOutline>>& sizes)
{
    QVector<std::optional<QPointF>> points(sizes.size());

    // The largest size: the first with the largest bounding-box area.
    int  largest      = -1;
    bool all_folded   = true;
    bool all_cornered = true;
    for (int i = 0; i < sizes.size(); ++i)
    {
        if (sizes.at(i).has_value())
        {
            const SizeOutline& size = sizes.at(i).value();
            all_folded              = all_folded && size.fold_line.has_value();
            all_cornered            = all_cornered && !size.turn_points.value_or(QVector<int>()).isEmpty();
            if (largest < 0 || area(size) > area(sizes.at(largest).value()))
            {
                largest = i;
            }
        }
    }

    if (largest >= 0)
    {
        const SizeOutline& big = sizes.at(largest).value();
        // Rule 1: the same fold line end in every size.
        const bool use_end = all_folded && lower_or_left(big.fold_line->p2(), big.fold_line->p1());
        // Rule 2: the place of the best corner of the largest size.
        const int     corner = !all_folded && all_cornered ? best_corner(big) : -1;
        const QPointF place  = corner >= 0 ? place_in_bounds(big, big.vertices.at(corner)) : QPointF();
        for (int i = 0; i < sizes.size(); ++i)
        {
            if (sizes.at(i).has_value())
            {
                const SizeOutline& size = sizes.at(i).value();
                if (all_folded)
                {
                    points[i] = use_end ? size.fold_line->p2() : size.fold_line->p1();
                }
                else if (corner >= 0)
                {
                    points[i] = size.vertices.at(nearest_turn_point(size, place));
                }
                else
                {
                    // Rule 3: the bounding-box centre.
                    points[i] = bounds(size).center();
                }
            }
        }
    }
    return points;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
QVector<std::optional<QPointF>> find_stack_points(const QVector<QDomElement>& pieces)
{
    QVector<std::optional<SizeOutline>> sizes;
    for (const QDomElement& piece : pieces)
    {
        sizes.append(read_size_outline(piece, find_grain_direction(piece)));
    }

    // Map each grain frame point back to its piece frame.
    const QVector<std::optional<QPointF>> frame_points = find_frame_points(sizes);
    QVector<std::optional<QPointF>>       points(sizes.size());
    for (int i = 0; i < sizes.size(); ++i)
    {
        if (frame_points.at(i).has_value())
        {
            points[i] = to_piece(frame_points.at(i).value(), sizes.at(i)->grain);
        }
    }
    return points;
}
} // namespace layout_handoff
