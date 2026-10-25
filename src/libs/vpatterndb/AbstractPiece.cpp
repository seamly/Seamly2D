//---------------------------------------------------------------------------------------------------------------------
//  @file   AbstractPiece.cpp
//
//  @copyright
//  Copyright (C) 2017  Seamly, LLC
//  https://github.com/fashionfreedom/seamly2d
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

//---------------------------------------------------------------------------------------------------------------------
//  @file   AbstractPiece.cpp
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date   3 11, 2016
//
//  @copyright
//  This source code is part of the Valentine project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2016 Seamly2D project
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

//---------------------------------------------------------------------------------------------------------------------
//  @file   AbstractPiece.cpp
//  @author slspencer
//
//  @brief
//  Piece properties and seam allowance geometry shared by VPiece and VLayoutPiece.
//
//  @copyright
//  Copyright (C) 2026 Seamly2D Project
//  https://github.com/fashionfreedom/seamly2d
//
//  SPDX-License-Identifier: GPL-3.0-or-later
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
//  along with Seamly2D. If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

#include "AbstractPiece.h"

#include <QCoreApplication>
#include <QLineF>
#include <QPainterPath>
#include <QPolygonF>
#include <QVector>

#include <algorithm>
#include <limits>

#include "../vgeometry/vpointf.h"
#include "../vmisc/diagnostic.h"
#include "AbstractPieceData.h"

namespace
{
/// @brief A corner miter longer than this many seam allowance widths is cut short (ByLength corner).
constexpr qreal max_length_ratio = 2.4;

/// @brief A vertex this close to a node vertex is that node vertex.
constexpr qreal node_match_tolerance = 0.01;

/// @brief A direction change above this angle where two straight lines meet makes a turn point.
constexpr qreal line_join_angle_degrees = 0.5;

/// @brief A tangent change above this angle where a curve meets a line or a curve makes a turn point.
/// @details The curve tangent comes from chords, so this needs more margin than a line join.
constexpr qreal curve_join_angle_degrees = 5.0;

/// @brief A direction change above this angle is a corner even away from a node, e.g. a seam allowance loop cut.
constexpr qreal sharp_angle_degrees = 25.0;

/// @brief A junction that bends more than this many times the curve's next vertex breaks the tangent.
/// @details A curve that leaves a line tangentially bends at the junction about half as much as at its next vertex.
constexpr qreal line_end_break_ratio = 2.0;

/// @brief A line end bends less than this on a flat curve, where the ratio to the next vertex is noise.
constexpr qreal line_end_min_break_degrees = 1.0;

/// @brief Material names as the pattern file stores them; the user sees them translated.
const char* const piece_materials[] = {
    QT_TRANSLATE_NOOP("PieceMaterial", "Fabric"),
    QT_TRANSLATE_NOOP("PieceMaterial", "Lining"),
    QT_TRANSLATE_NOOP("PieceMaterial", "Interfacing"),
    QT_TRANSLATE_NOOP("PieceMaterial", "Interlining")
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief normalizedAngle maps an angle to the range (-180, 180] degrees.
 */
qreal normalizedAngle(qreal degrees)
{
    qreal angle = degrees;
    while (angle > 180.0)
    {
        angle -= 360.0;
    }
    while (angle <= -180.0)
    {
        angle += 360.0;
    }
    return angle;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief directionDegrees returns the angle of the line from one point to another, in degrees.
 */
qreal directionDegrees(const QPointF& from, const QPointF& to)
{
    return QLineF(from, to).angle();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief repeatsFirstVertex tells whether a closed polyline ends with a repeat of its first vertex.
 */
bool repeatsFirstVertex(const QVector<QPointF>& points, bool closed)
{
    return closed && points.size() > 1 && QLineF(points.first(), points.last()).length() < node_match_tolerance;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isNodeVertex tells whether a point lies within node_match_tolerance of a node vertex.
 */
bool isNodeVertex(const QPointF& point, const QVector<QPointF>& node_vertices)
{
    return std::any_of(node_vertices.cbegin(), node_vertices.cend(), [&point](const QPointF& node)
    {
        return QLineF(point, node).length() < node_match_tolerance;
    });
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief neighbourIndex returns a vertex index; a closed polyline wraps it, an open one gives -1 past an end.
 */
int neighbourIndex(int index, int count, bool closed)
{
    int neighbour = -1;
    if (closed)
    {
        neighbour = (index % count + count) % count;
    }
    else if (index >= 0 && index < count)
    {
        neighbour = index;
    }
    return neighbour;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief breaksTangent tells whether the tangent breaks at a node vertex.
 *
 * A side between two node vertices is a straight line. The tangent of a curve side comes from its first two
 * chords: the chord direction lags the tangent by half the turn to the next chord. Where a line meets a curve, the
 * junction also breaks the tangent when it bends more than line_end_break_ratio times the curve's next vertex.
 * @param is_node node flag of each vertex; its size is the vertex count without a closing repeat.
 */
bool breaksTangent(const QVector<QPointF>& points, const QVector<bool>& is_node, int index, bool closed)
{
    const int count = is_node.size();
    const int previous = neighbourIndex(index - 1, count, closed);
    const int next = neighbourIndex(index + 1, count, closed);
    const int previous2 = neighbourIndex(index - 2, count, closed);
    const int next2 = neighbourIndex(index + 2, count, closed);

    const qreal chord_in = directionDegrees(points.at(previous), points.at(index));
    const qreal chord_out = directionDegrees(points.at(index), points.at(next));

    // A side that ends at a curve point is a curve side.
    qreal tangent_in = chord_in;
    if (!is_node.at(previous) && previous2 != -1)
    {
        tangent_in = chord_in
                     + normalizedAngle(chord_in - directionDegrees(points.at(previous2), points.at(previous))) / 2.0;
    }
    qreal tangent_out = chord_out;
    if (!is_node.at(next) && next2 != -1)
    {
        tangent_out = chord_out
                      - normalizedAngle(directionDegrees(points.at(next), points.at(next2)) - chord_out) / 2.0;
    }

    const bool line_in = is_node.at(previous);
    const bool line_out = is_node.at(next);
    const qreal limit = line_in && line_out ? line_join_angle_degrees : curve_join_angle_degrees;

    bool breaks = qAbs(normalizedAngle(tangent_out - tangent_in)) > limit;
    if (!breaks && line_in != line_out)
    {
        // Line end: compare the bend at the junction with the bend at the curve's next vertex.
        const qreal break_degrees = qAbs(normalizedAngle(chord_out - chord_in));
        qreal curve_degrees = 0; // past an open end
        if (line_in && next2 != -1)
        {
            curve_degrees = qAbs(normalizedAngle(directionDegrees(points.at(next), points.at(next2)) - chord_out));
        }
        else if (line_out && previous2 != -1)
        {
            curve_degrees = qAbs(normalizedAngle(chord_in - directionDegrees(points.at(previous2),
                                                                             points.at(previous))));
        }
        breaks = break_degrees > line_end_min_break_degrees && break_degrees > line_end_break_ratio * curve_degrees;
    }
    return breaks;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isTurnVertex tells whether a vertex is a turn point.
 * @param is_node node flag of each vertex; its size is the vertex count without a closing repeat.
 */
bool isTurnVertex(const QVector<QPointF>& points, const QVector<bool>& is_node, int index, bool closed)
{
    const int count = is_node.size();
    bool turn = false;
    if (!closed && (index == 0 || index == count - 1))
    {
        // An open end is always a turn point.
        turn = true;
    }
    else
    {
        const qreal chord_in = directionDegrees(points.at(neighbourIndex(index - 1, count, closed)), points.at(index));
        const qreal chord_out = directionDegrees(points.at(index), points.at(neighbourIndex(index + 1, count, closed)));

        // A sharp bend is a corner anywhere; a smaller one is a turn point only at a node vertex.
        turn = qAbs(normalizedAngle(chord_out - chord_in)) > sharp_angle_degrees
               || (is_node.at(index) && breaksTangent(points, is_node, index, closed));
    }
    return turn;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief closedPainterPath returns a closed winding-fill path through the points.
 * @param points at least one point.
 */
QPainterPath closedPainterPath(const QVector<QPointF>& points)
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.moveTo(points.at(0));
    for (int i = 1; i < points.count(); ++i)
    {
        path.lineTo(points.at(i));
    }
    path.lineTo(points.at(0));
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief cornerPoint finds where two lines cross, if they cross close enough to a main path corner.
 * @param corner main path corner.
 * @param width  seam allowance width at the corner.
 * @param point  receives the cross point.
 * @return false when the lines are parallel, or cross farther than max_length_ratio widths from the corner.
 */
bool cornerPoint(const QLineF& line1, const QLineF& line2, const QPointF& corner, qreal width, QPointF* point)
{
    const bool crosses = line1.intersects(line2, point) != QLineF::NoIntersection;
    return crosses && QLineF(corner, *point).length() <= width * max_length_ratio;
}
} // namespace

/**
 * @brief Loop is the result of findLoop(): how the edge from a vertex crosses a later edge.
 */
struct AbstractPiece::Loop
{
    /**
     * @brief Type tells how the edges cross.
     */
    enum class Type
    {
        None,     ///< No loop starts at the edge.
        Bounded,  ///< The edges cross at cross_point.
        Parallel  ///< The edges lie on one line and overlap.
    };

    Type    type{Type::None}; ///< How the edges cross.
    int     end_index{-1};    ///< Start vertex of the later edge.
    int     next_index{-1};   ///< End vertex of the later edge.
    QPointF cross_point;      ///< Cross point of a Bounded loop.
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AbstractPiece makes a piece with the default properties of AbstractPieceData.
 */
AbstractPiece::AbstractPiece()
    : d(new AbstractPieceData)
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AbstractPiece makes a copy that shares the properties of piece until one of them changes.
 */
AbstractPiece::AbstractPiece(const AbstractPiece& piece)
    : d(piece.d)
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ~AbstractPiece releases this copy's share of the properties.
 */
AbstractPiece::~AbstractPiece() = default;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief operator= shares the properties of piece.
 */
AbstractPiece& AbstractPiece::operator=(const AbstractPiece& piece)
{
    if (&piece != this)
    {
        d = piece.d;
    }
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief operator= takes the properties of piece; piece gets the properties of this piece.
 */
AbstractPiece& AbstractPiece::operator=(AbstractPiece&& piece) noexcept
{
    Swap(piece);
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Swap exchanges the properties of two pieces.
 */
void AbstractPiece::Swap(AbstractPiece& piece) noexcept
{
    std::swap(d, piece.d);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetName returns the piece name.
 */
QString AbstractPiece::GetName() const
{
    return d->m_name;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetName sets the piece name.
 */
void AbstractPiece::SetName(const QString& value)
{
    d->m_name = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getColor returns the piece outline color name.
 */
QString AbstractPiece::getColor() const
{
    return d->m_color;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setColor sets the piece outline color name.
 */
void AbstractPiece::setColor(const QString& value)
{
    d->m_color = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getFill returns the piece fill pattern name.
 */
QString AbstractPiece::getFill() const
{
    return d->m_fill;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setFill sets the piece fill pattern name.
 */
void AbstractPiece::setFill(const QString& value)
{
    d->m_fill = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getLock tells whether the user locked the piece against edits.
 */
bool AbstractPiece::getLock() const
{
    return d->m_piece_lock;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setLock locks or unlocks the piece against edits.
 */
void AbstractPiece::setLock(bool value)
{
    d->m_piece_lock = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getFlip returns the mirror a layout may apply to the piece.
 */
PieceFlip AbstractPiece::getFlip() const
{
    return d->m_flip;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setFlip sets the mirror a layout may apply to the piece.
 */
void AbstractPiece::setFlip(PieceFlip value)
{
    d->m_flip = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief IsForbidFlipping tells whether a layout may not mirror the piece (flip None).
 */
bool AbstractPiece::IsForbidFlipping() const
{
    return d->m_flip == PieceFlip::None;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief hasSeamAllowance tells whether the piece has a seam allowance.
 */
bool AbstractPiece::hasSeamAllowance() const
{
    return d->m_seam_allowance;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetSeamAllowance turns the seam allowance of the piece on or off.
 */
void AbstractPiece::SetSeamAllowance(bool value)
{
    d->m_seam_allowance = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief hasSeamAllowanceBuiltIn tells whether the main path already includes the seam allowance.
 */
bool AbstractPiece::hasSeamAllowanceBuiltIn() const
{
    return d->m_seam_allowance_built_in;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetSeamAllowanceBuiltIn sets whether the main path already includes the seam allowance.
 */
void AbstractPiece::SetSeamAllowanceBuiltIn(bool value)
{
    d->m_seam_allowance_built_in = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isHideSeamLine tells whether Piece mode does not draw the seam line.
 */
bool AbstractPiece::isHideSeamLine() const
{
    return d->m_hide_seam_line;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setHideSeamLine sets whether Piece mode draws the seam line.
 */
void AbstractPiece::setHideSeamLine(bool value)
{
    d->m_hide_seam_line = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getMaterial returns the material the piece is cut from, as the pattern file stores it (untranslated).
 */
QString AbstractPiece::getMaterial() const
{
    return d->m_material;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setMaterial sets the material the piece is cut from.
 * @param value one of materialNames(); an empty value means defaultMaterial().
 */
void AbstractPiece::setMaterial(const QString& value)
{
    d->m_material = value.isEmpty() ? defaultMaterial() : value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief materialNames returns every material a piece can have, untranslated, in the piece dialog order.
 */
QStringList AbstractPiece::materialNames()
{
    QStringList names;
    for (const char* name : piece_materials)
    {
        names.append(QString::fromLatin1(name));
    }
    return names;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief defaultMaterial returns the material of a piece that does not name one.
 */
QString AbstractPiece::defaultMaterial()
{
    return QString::fromLatin1(piece_materials[0]);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief materialDisplayName returns the translated material name; an unknown name comes back unchanged.
 */
QString AbstractPiece::materialDisplayName(const QString& material)
{
    return QCoreApplication::translate("PieceMaterial", material.toUtf8().constData());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSAWidth returns the piece seam allowance width in pixels.
 */
qreal AbstractPiece::GetSAWidth() const
{
    return d->m_seam_allowance_width;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetSAWidth sets the piece seam allowance width in pixels; a negative value sets 0.
 */
void AbstractPiece::SetSAWidth(qreal value)
{
    d->m_seam_allowance_width = value >= 0 ? value : 0;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetMx returns the piece x position in Piece mode, in pixels.
 */
qreal AbstractPiece::GetMx() const
{
    return d->m_position_x;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetMx sets the piece x position in Piece mode, in pixels.
 */
void AbstractPiece::SetMx(qreal value)
{
    d->m_position_x = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetMy returns the piece y position in Piece mode, in pixels.
 */
qreal AbstractPiece::GetMy() const
{
    return d->m_position_y;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetMy sets the piece y position in Piece mode, in pixels.
 */
void AbstractPiece::SetMy(qreal value)
{
    d->m_position_y = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Equidistant builds the seam allowance polyline around the main path.
 * @param points        main path points with their seam allowance widths.
 * @param width         piece seam allowance width; a point with a negative width uses it.
 * @param node_vertices when not null, receives the seam allowance vertices built at node points
 *                      (SeamAllowancePoint::isNode()), for turnPointIndices().
 * @param cut_corners   when not null, receives SeamAllowancePoint::cornerIndex() of each node point whose ByLength
 *                      corner was cut short, in path order.
 * @return the closed seam allowance polyline; empty when width is negative or fewer than 3 points remain.
 */
QVector<QPointF> AbstractPiece::Equidistant(const QVector<SeamAllowancePoint>& points, qreal width,
                                            QVector<QPointF>* node_vertices, QVector<int>* cut_corners)
{
    QVector<QPointF> seam_allowance;
    if (width < 0)
    {
        qDebug() << "Width < 0.";
    }
    else
    {
        QVector<SeamAllowancePoint> path = CorrectEquidistantPoints(points);
        if (path.size() < 3)
        {
            qDebug() << "Not enough points for building the equidistant.";
        }
        else
        {
            // The path is always closed: its last point repeats the first one.
            if (path.last().toPoint() != path.first().toPoint())
            {
                path.append(path.at(0));
            }

            // Build the seam allowance corner at each point. The first point joins the closing edge and the
            // first edge; the closing repeat point gets no corner of its own.
            const int last = path.size() - 1;
            QVector<QPointF> ekv_points;
            for (int i = 0; i < last; ++i)
            {
                const SeamAllowancePoint& point = path.at(i);
                const SeamAllowancePoint& line1_start = i == 0 ? path.at(last - 1) : path.at(i - 1);
                const SeamAllowancePoint& line1_end = i == 0 ? path.at(last) : point;

                bool cut = false;
                const QVector<QPointF> corner = EkvPoint(line1_start, line1_end, path.at(i + 1), point, width, &cut);
                if (node_vertices != nullptr && point.isNode())
                {
                    *node_vertices += corner;
                }

                // Report a cut corner by the number the caller gave its node point.
                if (cut_corners != nullptr && cut && point.isNode() && point.cornerIndex() >= 0)
                {
                    cut_corners->append(point.cornerIndex());
                }
                ekv_points += corner;
            }

            // Close the seam allowance, then remove repeated points and loops.
            if (!ekv_points.isEmpty())
            {
                ekv_points.append(ekv_points.at(0));
            }
            seam_allowance = CheckLoops(CorrectEquidistantPoints(ekv_points, false));
        }
    }
    return seam_allowance;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief turnPointIndices finds the turn points of a polyline: the vertices where the tangent breaks.
 *
 * Only node vertices (piece nodes, curve ends, spline end nodes) can break the tangent.
 * Curve interior vertices are curve points, except where the polyline turns sharper than a
 * curve interpolation can. A side between two node vertices is a straight line; the tangent
 * of a curve side is extrapolated from its first two chords, so a coarse curve interpolation
 * does not read as a corner. Where a line meets a curve, the junction is also a turn point
 * when it bends more than twice as much as the curve's next vertex, and more than 1°.
 *
 * @param points        polyline vertices; a closed polyline may repeat its first vertex.
 * @param node_vertices node vertex positions; vertices within node_match_tolerance of one are nodes.
 * @param closed        whether the polyline is a closed loop. Open ends are always turn points.
 * @return ascending indices into @p points.
 */
QVector<int> AbstractPiece::turnPointIndices(const QVector<QPointF>& points, const QVector<QPointF>& node_vertices,
                                             bool closed)
{
    QVector<int> turns;

    // A closing repeat vertex takes the turn state of the first vertex.
    const bool repeats_first = repeatsFirstVertex(points, closed);
    const int count = repeats_first ? points.size() - 1 : points.size();
    if (count >= 2)
    {
        QVector<bool> is_node(count, false);
        for (int i : nodeVertexIndices(points, node_vertices, closed))
        {
            is_node[i] = true;
        }

        for (int i = 0; i < count; ++i)
        {
            if (isTurnVertex(points, is_node, i, closed))
            {
                turns.append(i);
            }
        }

        if (repeats_first && !turns.isEmpty() && turns.first() == 0)
        {
            turns.append(count);
        }
    }
    return turns;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief nodeVertexIndices finds the polyline vertices that are node vertices.
 * @param points        polyline vertices; a closed polyline may repeat its first vertex.
 * @param node_vertices node vertex positions; vertices within node_match_tolerance of one are nodes.
 * @param closed        whether the polyline is a closed loop.
 * @return ascending indices into @p points. A closing repeat vertex is not listed: it is the first vertex.
 */
QVector<int> AbstractPiece::nodeVertexIndices(const QVector<QPointF>& points, const QVector<QPointF>& node_vertices,
                                              bool closed)
{
    QVector<int> indices;
    const int count = repeatsFirstVertex(points, closed) ? points.size() - 1 : points.size();
    for (int i = 0; i < count; ++i)
    {
        if (isNodeVertex(points.at(i), node_vertices))
        {
            indices.append(i);
        }
    }
    return indices;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief snapNodeVertices puts each node vertex that RemoveDublicates() erased back on the polyline.
 *
 * RemoveDublicates() erases the later of two vertices closer than accuracyPointOnLine. When a curve point comes
 * just before a node vertex, the node vertex goes and the curve point stays, a fraction of a pixel away. That
 * happens in some pattern sizes only, so the node count of the line changes with the size and unifyTurnPoints()
 * cannot match the nodes. The vertex nearest to such a node moves onto it.
 * @param points        polyline after RemoveDublicates(); a closed polyline may repeat its first vertex.
 * @param node_vertices node vertex positions.
 * @param closed        whether the polyline is a closed loop.
 * @return the polyline with every node vertex within accuracyPointOnLine of a vertex at its exact position.
 */
QVector<QPointF> AbstractPiece::snapNodeVertices(const QVector<QPointF>& points, const QVector<QPointF>& node_vertices,
                                                 bool closed)
{
    QVector<QPointF> snapped = points;
    const bool repeats = repeatsFirstVertex(points, closed);
    for (const QPointF& node : node_vertices)
    {
        // Find the vertex nearest to the node.
        int nearest = -1;
        qreal nearest_length = std::numeric_limits<qreal>::infinity();
        for (int i = 0; i < snapped.size(); ++i)
        {
            const qreal length = QLineF(snapped.at(i), node).length();
            if (length < nearest_length)
            {
                nearest = i;
                nearest_length = length;
            }
        }

        // A matched node stays; a vertex farther than a removed duplicate could be is another point.
        if (nearest >= 0 && nearest_length >= node_match_tolerance && nearest_length <= accuracyPointOnLine)
        {
            snapped[nearest] = node;

            // The closing repeat of the first vertex moves with it.
            if (repeats && (nearest == 0 || nearest == snapped.size() - 1))
            {
                snapped.first() = node;
                snapped.last() = node;
            }
        }
    }
    return snapped;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief addTurnPoints adds vertices to a turn point list.
 * @param points      polyline the indices point into; a closed polyline may repeat its first vertex.
 * @param turn_points ascending turn point indices, from turnPointIndices().
 * @param added       indices to add, in any order.
 * @param closed      whether the polyline is a closed loop.
 * @return ascending indices without repeats. As in turnPointIndices(), a closing repeat vertex is a turn point
 *         when the first vertex is one.
 */
QVector<int> AbstractPiece::addTurnPoints(const QVector<QPointF>& points, const QVector<int>& turn_points,
                                          const QVector<int>& added, bool closed)
{
    QVector<int> turns = turn_points + added;
    if (repeatsFirstVertex(points, closed) && turns.contains(0))
    {
        turns.append(points.size() - 1);
    }
    std::sort(turns.begin(), turns.end());
    turns.erase(std::unique(turns.begin(), turns.end()), turns.end());
    return turns;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief sumTrapezoids returns twice the signed area of a polygon, as a sum of trapezoid areas.
 * @return negative for a clockwise polygon on screen; 0 for fewer than 3 points.
 */
qreal AbstractPiece::sumTrapezoids(const QVector<QPointF>& points)
{
    qreal area = 0;
    const int count = points.size();
    if (count > 2)
    {
        for (int i = 0; i < count; ++i)
        {
            // The neighbours of the first and the last point wrap around the polygon.
            const int previous = i == 0 ? count - 1 : i - 1;
            const int next = i == count - 1 ? 0 : i + 1;
            area += points.at(i).x() * (points.at(previous).y() - points.at(next).y());
        }
    }
    return area;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isClockwise tells whether a polygon runs clockwise.
 * @return false for fewer than 3 points.
 */
bool AbstractPiece::isClockwise(const QVector<QPointF>& points)
{
    return points.count() >= 3 && sumTrapezoids(points) < 0;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CheckLoops finds and removes the loops of a seam allowance polyline.
 * @param points seam allowance points.
 * @return the seam allowance points without loops.
 */
QVector<QPointF> AbstractPiece::CheckLoops(const QVector<QPointF>& points)
{
    const int count = points.size();
    QVector<QPointF> ekv_points;
    if (count < 4)
    {
        // A path of fewer than 4 points holds no loop.
        ekv_points = points;
    }
    else
    {
        const bool path_closed = points.first() == points.last();
        int i = 0;
        while (i < count)
        {
            ekv_points.append(points.at(i));

            // The last three points make a triangle, which holds no loop.
            if (i <= count - 3)
            {
                // Skip the vertices inside a loop.
                const Loop loop = findLoop(points, i, path_closed);
                if (loop.type == Loop::Type::Parallel)
                {
                    ekv_points.append(points.at(loop.next_index));
                    i = qMax(loop.next_index, loop.end_index);
                }
                else if (loop.type == Loop::Type::Bounded)
                {
                    ekv_points.append(loop.cross_point);
                    i = loop.end_index;
                }
            }
            ++i;
        }
    }
    return ekv_points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief findLoop finds the last edge that the edge from a vertex crosses.
 *
 * A path can hold several loops. The search runs from the end, so the last loop found removes all of them.
 * @param index       start vertex of the edge; index + 1 is its end vertex.
 * @param path_closed whether the last point repeats the first one.
 * @return the crossing, or type None when the edge starts no loop.
 */
AbstractPiece::Loop AbstractPiece::findLoop(const QVector<QPointF>& points, int index, bool path_closed)
{
    const int count = points.size();
    const QLineF line1(points.at(index), points.at(index + 1));

    Loop loop;
    for (int j = count - 1; j >= index + 2 && loop.type == Loop::Type::None; --j)
    {
        const int j_next = j == count - 1 ? 0 : j + 1;
        const QLineF line2(points.at(j), points.at(j_next));

        // A closed path has a zero length edge from its last vertex to its first.
        if (!qFuzzyIsNull(line2.length()))
        {
            // Edges that share a vertex are neighbours. A closed path uses index 0 for its last vertex.
            QVector<int> unique_vertices;
            const auto add_unique = [&unique_vertices](int vertex)
            {
                if (!unique_vertices.contains(vertex))
                {
                    unique_vertices.append(vertex);
                }
            };
            add_unique(index);
            add_unique(index + 1);
            add_unique(j);
            add_unique(path_closed && j_next == count - 1 ? 0 : j_next);
            const bool neighbours = unique_vertices.size() != 4;

            QPointF cross_point;
            const QLineF::IntersectType intersect = line1.intersects(line2, &cross_point);
            if (intersect == QLineF::NoIntersection && !neighbours
                && VGObject::IsPointOnLineviaPDP(points.at(j), points.at(index), points.at(index + 1)))
            {
                // Parallel edges on one line overlap when a perpendicular to one edge crosses the other.
                QLineF perpendicular1 = line1;
                perpendicular1.setAngle(line1.angle() + 90);
                QLineF perpendicular2 = line2;
                perpendicular2.setAngle(line2.angle() + 90);
                QPointF unused;
                const bool overlap = perpendicular1.intersects(line2, &unused) == QLineF::BoundedIntersection
                                     || line1.intersects(perpendicular2, &unused) == QLineF::BoundedIntersection;
                if (overlap)
                {
                    QPointF shared_point;
                    const bool shared = ParallelCrossPoint(line1, line2, shared_point);
                    if (!shared || CheckIntersection(points, index, index + 1, j, j_next, shared_point))
                    {
                        loop.type = Loop::Type::Parallel;
                    }
                }
            }
            else if (intersect == QLineF::BoundedIntersection && !neighbours)
            {
                // Edges that meet at an end point cross only when the loop check says so.
                const bool at_end = line1.p1() == cross_point || line1.p2() == cross_point
                                    || line2.p1() == cross_point || line2.p2() == cross_point;
                if (!at_end || CheckIntersection(points, index, index + 1, j, j_next, cross_point))
                {
                    loop.type = Loop::Type::Bounded;
                    loop.cross_point = cross_point;
                }
            }

            if (loop.type != Loop::Type::None)
            {
                loop.end_index = j;
                loop.next_index = j_next;
            }
        }
    }
    return loop;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PointPosition tells on which side of a line a point lies.
 * @return positive on the left of the line direction, negative on the right, 0 on the line.
 */
constexpr qreal AbstractPiece::PointPosition(const QPointF& point, const QLineF& line)
{
    return (line.p2().x() - line.p1().x()) * (point.y() - line.p1().y()) -
           (line.p2().y() - line.p1().y()) * (point.x() - line.p1().x());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MaxLocalSA returns the larger of the two seam allowance widths at a point.
 * @param width piece width, for a side that uses it.
 */
qreal AbstractPiece::MaxLocalSA(const SeamAllowancePoint& point, qreal width)
{
    return qMax(point.GetSAAfter(width), point.GetSABefore(width));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief EkvPoint returns the seam allowance points at the corner where two main path edges meet.
 * @param p1_line1 start of the edge that ends at the corner.
 * @param p2_line1 corner, as the end of the first edge.
 * @param p1_line2 end of the edge that starts at the corner.
 * @param p2_line2 corner, as the start of the second edge; it must equal p2_line1.
 * @param width    piece seam allowance width.
 * @param cut      when not null, receives true when a ByLength corner was cut short.
 * @return the seam allowance points; empty for a negative width or two different corner points.
 */
QVector<QPointF> AbstractPiece::EkvPoint(const SeamAllowancePoint& p1_line1, const SeamAllowancePoint& p2_line1,
                                         const SeamAllowancePoint& p1_line2, const SeamAllowancePoint& p2_line2,
                                         qreal width, bool* cut)
{
    if (cut != nullptr)
    {
        *cut = false;
    }

    QVector<QPointF> points;
    if (width < 0)
    {
        // A width cannot be negative; the corner gets no points.
    }
    else if (p2_line1 != p2_line2)
    {
        qDebug() << "Last points of two lines must be equal.";
    }
    else
    {
        CornerLines lines;
        lines.before = createParallelLine(p1_line1, p2_line1, width);
        lines.after = createParallelLine(p2_line2, p1_line2, width);
        const QLineF::IntersectType type = lines.before.intersects(lines.after, &lines.cross_point);
        if (type == QLineF::BoundedIntersection)
        {
            // The seam allowance lines really cross.
            points.append(lines.cross_point);
        }
        else if (type == QLineF::UnboundedIntersection)
        {
            // The lines cross only when extended: the most common case.
            points = UnboundedCorner(p1_line1, p2_line1, p1_line2, p2_line2, lines, width, cut);
        }
        else
        {
            // Correct edges that do not cross lie on one line.
            points.append(lines.before.p2());
        }
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UnboundedCorner builds a seam allowance corner where the seam allowance lines cross only when extended.
 * @param lines seam allowance lines of the two edges and their cross point.
 * @param width piece seam allowance width.
 * @param cut   when not null, receives true when a ByLength corner was cut short.
 */
QVector<QPointF> AbstractPiece::UnboundedCorner(const SeamAllowancePoint& p1_line1,
                                                const SeamAllowancePoint& p2_line1,
                                                const SeamAllowancePoint& p1_line2,
                                                const SeamAllowancePoint& p2_line2, const CornerLines& lines,
                                                qreal width, bool* cut)
{
    QVector<QPointF> points;
    const qreal local_width = MaxLocalSA(p2_line1, width);
    QLineF line(p2_line1, lines.cross_point);

    // Comparing the bisector angles of the main path and the seam allowance gives the direction.
    const QLineF b1 = BisectorLine(p1_line1, p2_line1, p1_line2);
    const QLineF b2 = BisectorLine(lines.before.p1(), lines.cross_point, lines.after.p2());
    const qreal angle = AngleBetweenBisectors(b1, b2);

    if (angle < 90 || VFuzzyComparePossibleNulls(angle, 90.0))
    {
        // Same direction: a regular seam allowance corner.
        points = CornerByAngleType(p1_line1, p2_line1, p1_line2, lines, local_width, cut);
    }
    else
    {
        // Different directions.
        QLineF bisector(p2_line1, p1_line1);
        bisector.setAngle(b1.angle());

        const qreal bisector_side1 = PointPosition(bisector.p2(), QLineF(p1_line1, p2_line1));
        const qreal bisector_side2 = PointPosition(bisector.p2(), QLineF(p2_line2, p1_line2));

        if ((bisector_side1 < 0 || qFuzzyIsNull(bisector_side1))
            && (bisector_side2 < 0 || qFuzzyIsNull(bisector_side2)))
        {
            // Dart case: the bisector points outside. The cross point can still be valid, but is ignored when
            // it goes outside of the seam allowance.
            const QLineF big_edge = createParallelLine(p1_line1, p1_line2, local_width);
            QPointF edge_point;
            const QLineF::IntersectType type = big_edge.intersects(line, &edge_point);
            if (type != QLineF::BoundedIntersection && line.length() < QLineF(p2_line1, edge_point).length())
            {
                points.append(lines.cross_point);
            }
        }
        else
        {
            // Not a dart: the angle is acute and the bisector points inside.
            const qreal cross_side1 = PointPosition(lines.cross_point, QLineF(p1_line1, p2_line1));
            const qreal cross_side2 = PointPosition(lines.cross_point, QLineF(p2_line2, p1_line2));

            if ((cross_side1 < 0 || qFuzzyIsNull(cross_side1)) && (cross_side2 < 0 || qFuzzyIsNull(cross_side2)))
            {
                // The cross point is outside of the piece; move it out to the local width when it is too close.
                if (line.length() >= local_width)
                {
                    points.append(lines.cross_point);
                }
                else
                {
                    line.setLength(local_width);
                    points.append(line.p2());
                }
            }
            else
            {
                // The cross point is wrong, probably inside the piece: build the seam allowance by hand.
                const QLineF big_edge = createParallelLine(lines.before.p2(), lines.after.p1(), local_width);
                points.append(big_edge.p1());
                points.append(big_edge.p2());
            }
        }
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CornerByAngleType builds a regular seam allowance corner by the angle type of the corner point.
 * @param lines seam allowance lines of the two edges and their cross point.
 * @param width local seam allowance width at the corner.
 * @param cut   when not null, receives true when a ByLength corner was cut short.
 */
QVector<QPointF> AbstractPiece::CornerByAngleType(const SeamAllowancePoint& p1_line1,
                                                  const SeamAllowancePoint& p2_line1,
                                                  const SeamAllowancePoint& p1_line2, const CornerLines& lines,
                                                  qreal width, bool* cut)
{
    const QPointF& sp1 = lines.before.p1();
    const QPointF& sp2 = lines.cross_point;
    const QPointF& sp3 = lines.after.p2();

    // The cases cover every PieceNodeAngle value.
    QVector<QPointF> points;
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wswitch-default")
    switch (p2_line1.GetAngleType())
    {
        case PieceNodeAngle::ByLength:
            points = AngleByLength(p2_line1, sp1, sp2, sp3, width, p2_line1.isCornerCutForced(), cut);
            break;
        case PieceNodeAngle::ByPointsIntersection:
            points = AngleByIntersection(p1_line1, p2_line1, p1_line2, sp1, sp2, sp3, width);
            break;
        case PieceNodeAngle::ByFirstEdgeSymmetry:
            points = AngleByFirstSymmetry(p1_line1, p2_line1, sp1, sp2, sp3, width);
            break;
        case PieceNodeAngle::BySecondEdgeSymmetry:
            points = AngleBySecondSymmetry(p2_line1, p1_line2, sp1, sp2, sp3, width);
            break;
        case PieceNodeAngle::ByFirstEdgeRightAngle:
            points = AngleByFirstRightAngle(p1_line1, p2_line1, sp1, sp2, sp3, width);
            break;
        case PieceNodeAngle::BySecondEdgeRightAngle:
            points = AngleBySecondRightAngle(p2_line1, p1_line2, sp1, sp2, sp3, width);
            break;
    }
QT_WARNING_POP
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AngleByLength builds a corner whose miter is cut short when it is longer than max_length_ratio widths.
 * @param p2        main path corner.
 * @param sp1       start of the seam allowance line before the corner.
 * @param sp2       cross point of the two seam allowance lines.
 * @param sp3       end of the seam allowance line after the corner.
 * @param force_cut cuts the corner whenever its miter is longer than width, so that every size of a multisize
 *                  pattern gets the same corner shape.
 * @param cut       when not null, receives true when the corner was cut.
 */
QVector<QPointF> AbstractPiece::AngleByLength(const QPointF& p2, const QPointF& sp1, const QPointF& sp2,
                                              const QPointF& sp3, qreal width, bool force_cut, bool* cut)
{
    QVector<QPointF> points;

    QLineF line(p2, sp2);
    const qreal length = line.length();
    const bool cut_corner = length > width * max_length_ratio || (force_cut && length > width);
    if (cut != nullptr)
    {
        *cut = cut_corner;
    }

    if (cut_corner)
    {
        // The cut line is perpendicular to the miter, width away from the corner.
        line.setLength(width);
        QLineF cut_line(line.p2(), sp2);
        cut_line.setLength(length);

        // The intersections always exist.
        QPointF cut_point;
        cut_line.setAngle(cut_line.angle() + 90);
        QLineF::IntersectType type = QLineF(sp1, sp2).intersects(cut_line, &cut_point);
        if (type == QLineF::NoIntersection)
        {
            qDebug() << "Couldn't find intersection with cut line.";
        }
        points.append(cut_point);

        cut_line.setAngle(cut_line.angle() - 180);
        type = QLineF(sp2, sp3).intersects(cut_line, &cut_point);
        if (type == QLineF::NoIntersection)
        {
            qDebug() << "Couldn't find intersection with cut line.";
        }
        points.append(cut_point);
    }
    else
    {
        // The miter is short enough to keep.
        points.append(sp2);
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AngleByIntersection ends each seam allowance line where it meets the extension of the other main path edge.
 * @return the AngleByLength() corner when a point is missing or too far from the corner.
 */
QVector<QPointF> AbstractPiece::AngleByIntersection(const QPointF& p1, const QPointF& p2, const QPointF& p3,
                                                    const QPointF& sp1, const QPointF& sp2, const QPointF& sp3,
                                                    qreal width)
{
    QPointF first;
    QPointF second;
    const bool found = cornerPoint(QLineF(p2, p3), QLineF(sp1, sp2), p2, width, &first)
                       && cornerPoint(QLineF(p1, p2), QLineF(sp2, sp3), p2, width, &second);

    QVector<QPointF> points;
    if (found)
    {
        points << first << second;
    }
    else
    {
        points = AngleByLength(p2, sp1, sp2, sp3, width);
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AngleByFirstSymmetry ends the seam allowance at the first edge mirrored on the second seam allowance line.
 * @return the AngleByLength() corner when a point is missing or too far from the corner.
 */
QVector<QPointF> AbstractPiece::AngleByFirstSymmetry(const QPointF& p1, const QPointF& p2,
                                                     const QPointF& sp1, const QPointF& sp2, const QPointF& sp3,
                                                     qreal width)
{
    const QLineF sa_edge2(sp2, sp3);
    const QLineF flipped_edge(VPointF::FlipPF(sa_edge2, p1), VPointF::FlipPF(sa_edge2, p2));

    QPointF first;
    QPointF second;
    const bool found = cornerPoint(flipped_edge, QLineF(sp1, sp2), p2, width, &first)
                       && cornerPoint(flipped_edge, sa_edge2, p2, width, &second);

    QVector<QPointF> points;
    if (found)
    {
        points << first << second;
    }
    else
    {
        points = AngleByLength(p2, sp1, sp2, sp3, width);
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AngleBySecondSymmetry ends the seam allowance at the second edge mirrored on the first seam allowance line.
 * @return the AngleByLength() corner when a point is missing or too far from the corner.
 */
QVector<QPointF> AbstractPiece::AngleBySecondSymmetry(const QPointF& p2, const QPointF& p3,
                                                      const QPointF& sp1, const QPointF& sp2, const QPointF& sp3,
                                                      qreal width)
{
    const QLineF sa_edge1(sp1, sp2);
    const QLineF flipped_edge(VPointF::FlipPF(sa_edge1, p2), VPointF::FlipPF(sa_edge1, p3));

    QPointF first;
    QPointF second;
    const bool found = cornerPoint(flipped_edge, sa_edge1, p2, width, &first)
                       && cornerPoint(flipped_edge, QLineF(sp2, sp3), p2, width, &second);

    QVector<QPointF> points;
    if (found)
    {
        points << first << second;
    }
    else
    {
        points = AngleByLength(p2, sp1, sp2, sp3, width);
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AngleByFirstRightAngle ends the seam allowance at a line through the corner, square to the first edge.
 * @return the AngleByLength() corner when a point is missing or too far from the corner.
 */
QVector<QPointF> AbstractPiece::AngleByFirstRightAngle(const QPointF& p1, const QPointF& p2,
                                                       const QPointF& sp1, const QPointF& sp2, const QPointF& sp3,
                                                       qreal width)
{
    QLineF square_edge(p2, p1);
    square_edge.setAngle(square_edge.angle() - 90);

    QPointF first;
    QPointF second;
    const bool found = cornerPoint(square_edge, QLineF(sp1, sp2), p2, width, &first)
                       && cornerPoint(square_edge, QLineF(sp2, sp3), p2, width, &second);

    QVector<QPointF> points;
    if (found)
    {
        points << first << second;
    }
    else
    {
        points = AngleByLength(p2, sp1, sp2, sp3, width);
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AngleBySecondRightAngle ends the seam allowance at a line through the corner, square to the second edge.
 * @return the AngleByLength() corner when a point is missing or too far from the corner.
 */
QVector<QPointF> AbstractPiece::AngleBySecondRightAngle(const QPointF& p2, const QPointF& p3,
                                                        const QPointF& sp1, const QPointF& sp2, const QPointF& sp3,
                                                        qreal width)
{
    QLineF square_edge(p2, p3);
    square_edge.setAngle(square_edge.angle() + 90);

    QPointF first;
    QPointF second;
    const bool found = cornerPoint(square_edge, QLineF(sp1, sp2), p2, width, &first)
                       && cornerPoint(square_edge, QLineF(sp2, sp3), p2, width, &second);

    QVector<QPointF> points;
    if (found)
    {
        points << first << second;
    }
    else
    {
        points = AngleByLength(p2, sp1, sp2, sp3, width);
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createParallelLine returns the seam allowance line of an edge, with the widths of its end points.
 * @param p1    edge start; its width after the point applies.
 * @param p2    edge end; its width before the point applies.
 * @param width piece width, for an end point that uses it.
 */
QLineF AbstractPiece::createParallelLine(const SeamAllowancePoint& p1, const SeamAllowancePoint& p2, qreal width)
{
    return QLineF(SingleParallelPoint(p1, p2, 90, p1.GetSAAfter(width)),
                  SingleParallelPoint(p2, p1, -90, p2.GetSABefore(width)));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createParallelLine returns the line parallel to an edge, width away on its seam allowance side.
 */
QLineF AbstractPiece::createParallelLine(const QPointF& p1, const QPointF& p2, qreal width)
{
    return QLineF(SingleParallelPoint(p1, p2, 90, width), SingleParallelPoint(p2, p1, -90, width));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SingleParallelPoint returns the point width away from p1, at angle degrees from the line to p2.
 */
QPointF AbstractPiece::SingleParallelPoint(const QPointF& p1, const QPointF& p2, qreal angle, qreal width)
{
    QLineF line(p1, p2);
    line.setAngle(line.angle() + angle);
    line.setLength(width);
    return line.p2();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief BisectorLine returns a line from p2 that halves the smaller angle between the lines to p1 and p3.
 */
QLineF AbstractPiece::BisectorLine(const QPointF& p1, const QPointF& p2, const QPointF& p3)
{
    const QLineF line1(p2, p1);
    const QLineF line2(p2, p3);

    const qreal angle1 = line1.angleTo(line2);
    const qreal angle2 = line2.angleTo(line1);

    QLineF bisector = angle1 <= angle2 ? line1 : line2;
    bisector.setAngle(bisector.angle() + (angle1 <= angle2 ? angle1 : angle2) / 2.0);
    return bisector;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AngleBetweenBisectors returns the smaller angle between two bisectors moved to the same start point.
 */
qreal AbstractPiece::AngleBetweenBisectors(const QLineF& b1, const QLineF& b2)
{
    const QLineF moved_b2 = b2.translated(-(b2.p1().x() - b1.p1().x()), -(b2.p1().y() - b1.p1().y()));

    qreal angle1 = moved_b2.angleTo(b1);
    if (VFuzzyComparePossibleNulls(angle1, 360))
    {
        angle1 = 0;
    }

    qreal angle2 = b1.angleTo(moved_b2);
    if (VFuzzyComparePossibleNulls(angle2, 360))
    {
        angle2 = 0;
    }

    return angle1 <= angle2 ? angle1 : angle2;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CheckIntersection tells whether the crossing of edge (i, i_next) and edge (j, j_next) cuts off a loop.
 *
 * The cross point splits the path into two sub paths. Two clockwise sub paths make a loop when they overlap;
 * otherwise they make a loop when they do not.
 */
bool AbstractPiece::CheckIntersection(const QVector<QPointF>& points, int i, int i_next, int j, int j_next,
                                      const QPointF& cross_point)
{
    QVector<QPointF> sub1 = SubPath(points, i_next, j);
    sub1.append(cross_point);
    sub1 = CheckLoops(CorrectEquidistantPoints(sub1, false));
    const qreal sub1_sum = sumTrapezoids(sub1);

    QVector<QPointF> sub2 = SubPath(points, j_next, i);
    sub2.append(cross_point);
    sub2 = CheckLoops(CorrectEquidistantPoints(sub2, false));
    const qreal sub2_sum = sumTrapezoids(sub2);

    const bool crossing = Crossing(sub1, sub2);
    return (sub1_sum < 0 && sub2_sum < 0) ? crossing : !crossing;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParallelCrossPoint finds an end point that two lines on one line share.
 * @param point receives the shared end point of line1, or a null point.
 * @return false when the lines share no end point.
 */
bool AbstractPiece::ParallelCrossPoint(const QLineF& line1, const QLineF& line2, QPointF& point)
{
    bool shared = true;
    if (line1.p2() == line2.p2() || line1.p2() == line2.p1())
    {
        point = line1.p2();
    }
    else if (line1.p1() == line2.p1() || line1.p1() == line2.p2())
    {
        point = line1.p1();
    }
    else
    {
        point = QPointF();
        shared = false;
    }
    return shared;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Crossing tells whether two closed polygons overlap.
 * @return false when either polygon is empty.
 */
bool AbstractPiece::Crossing(const QVector<QPointF>& sub1, const QVector<QPointF>& sub2)
{
    bool crossing = false;

    // The bounding rectangle test is fast and rules out most pairs.
    if (!sub1.isEmpty() && !sub2.isEmpty()
        && QPolygonF(sub1).boundingRect().intersects(QPolygonF(sub2).boundingRect()))
    {
        crossing = closedPainterPath(sub1).intersects(closedPainterPath(sub2));
    }
    return crossing;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SubPath returns the path points from start_index to end_index, both included, wrapping past the end.
 * @return the whole path when an index is out of range or both indices are the same.
 */
QVector<QPointF> AbstractPiece::SubPath(const QVector<QPointF>& path, int start_index, int end_index)
{
    const bool valid = !path.isEmpty() && start_index >= 0 && start_index < path.size()
                       && end_index >= 0 && end_index < path.size() && start_index != end_index;

    QVector<QPointF> sub_path;
    if (!valid)
    {
        sub_path = path;
    }
    else
    {
        int i = start_index;
        sub_path.append(path.at(i));
        while (i != end_index)
        {
            i = (i + 1) % path.size();
            sub_path.append(path.at(i));
        }
    }
    return sub_path;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief IsEkvPointOnLine tells whether a point lies on the line between its neighbours.
 */
bool AbstractPiece::IsEkvPointOnLine(const QPointF& point, const QPointF& previous_point, const QPointF& next_point)
{
    return VGObject::IsPointOnLineviaPDP(point, previous_point, next_point);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief IsEkvPointOnLine tells whether a point and its seam allowance both lie on the line between its neighbours.
 *
 * A point on the main path line is not on the seam allowance line when the seam allowance width changes there.
 * See bug #671.
 */
bool AbstractPiece::IsEkvPointOnLine(const SeamAllowancePoint& point, const SeamAllowancePoint& previous_point,
                                     const SeamAllowancePoint& next_point)
{
    // A point without its own seam allowance width uses this test width.
    const qreal test_width = 10;
    const QLineF big_line1 = createParallelLine(previous_point, point, test_width);
    const QLineF big_line2 = createParallelLine(point, next_point, test_width);

    return VGObject::IsPointOnLineviaPDP(point, previous_point, next_point)
           && VGObject::IsPointOnLineviaPDP(big_line1.p2(), big_line1.p1(), big_line2.p2())
           && VGObject::IsPointOnLineviaPDP(big_line2.p1(), big_line1.p1(), big_line2.p2())
           && qAbs(previous_point.GetSAAfter(test_width) - next_point.GetSABefore(test_width))
                  < VGObject::accuracyPointOnLine;
}
