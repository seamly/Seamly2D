//---------------------------------------------------------------------------------------------------------------------
//  @file   AbstractPiece.h
//  @author Douglas S Caskey
//  @date   17 Jul, 2026
//
//  @copyright
//  Copyright (C) 2017 - 2026 Seamly, LLC
//  https://github.com/fashionfreedom/seamly2d
//
//  @brief
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

//---------------------------------------------------------------------------------------------------------------------
//  @file   AbstractPiece.h
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date   3 11, 2016
//
//  @brief
//  @copyright
//  This source code is part of the Valentina project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2016 Valentina project
//  <https://bitbucket.org/dismine/valentina> All Rights Reserved.
//
//  Valentina is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Valentina is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
//  @file   AbstractPiece.h
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

#ifndef ABSTRACTPIECE_H
#define ABSTRACTPIECE_H

#include <QDebug>
#include <QLineF>
#include <QPointF>
#include <QSharedDataPointer>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

#include "../vgeometry/vgobject.h"
#include "../vmisc/def.h"
#include "SeamAllowancePoint.h"

class AbstractPieceData;

/**
 * @brief SeamAllowanceCorners lists the seam allowance corners that a ByLength angle cut short.
 * @details Corners are the node points of the seam allowance path, numbered from 0 in path order.
 */
struct SeamAllowanceCorners
{
    int          count = 0; ///< Node points on the seam allowance path.
    QVector<int> cut;       ///< Numbers of the corners cut short, ascending.
};

/**
 * @brief AbstractPiece holds the piece properties and builds the seam allowance of a main path.
 *
 * VPiece and VLayoutPiece derive from it. Copies share the properties until one copy changes them.
 * The static functions work on point lists only; they hold no piece state.
 */
class AbstractPiece
{
public:
    AbstractPiece();
    AbstractPiece(const AbstractPiece& piece);
    virtual ~AbstractPiece();

    AbstractPiece& operator=(const AbstractPiece& piece);
    AbstractPiece& operator=(AbstractPiece&& piece) noexcept;

    void Swap(AbstractPiece& piece) noexcept;

    QString GetName() const;
    void    SetName(const QString& value);

    QString getColor() const;
    void    setColor(const QString& value);

    QString getFill() const;
    void    setFill(const QString& value);

    bool getLock() const;
    void setLock(bool value);

    PieceFlip getFlip() const;
    void      setFlip(PieceFlip value);
    bool      IsForbidFlipping() const;

    bool hasSeamAllowance() const;
    void SetSeamAllowance(bool value);

    bool hasSeamAllowanceBuiltIn() const;
    void SetSeamAllowanceBuiltIn(bool value);

    bool isHideSeamLine() const;
    void setHideSeamLine(bool value);

    QString getMaterial() const;
    void    setMaterial(const QString& value);

    static QStringList materialNames();
    static QString     defaultMaterial();
    static QString     materialDisplayName(const QString& material);

    qreal GetSAWidth() const;
    void  SetSAWidth(qreal value);

    qreal GetMx() const;
    void  SetMx(qreal value);

    qreal GetMy() const;
    void  SetMy(qreal value);

    static QVector<QPointF> Equidistant(const QVector<SeamAllowancePoint>& points, qreal width,
                                        QVector<QPointF>* node_vertices = nullptr,
                                        QVector<int>* cut_corners = nullptr);
    static QVector<int>     turnPointIndices(const QVector<QPointF>& points, const QVector<QPointF>& node_vertices,
                                             bool closed);
    static QVector<QPointF> snapNodeVertices(const QVector<QPointF>& points, const QVector<QPointF>& node_vertices,
                                             bool closed);
    static QVector<int>     nodeVertexIndices(const QVector<QPointF>& points, const QVector<QPointF>& node_vertices,
                                              bool closed);
    static QVector<int>     addTurnPoints(const QVector<QPointF>& points, const QVector<int>& turn_points,
                                          const QVector<int>& added, bool closed);
    static qreal            sumTrapezoids(const QVector<QPointF>& points);
    static bool             isClockwise(const QVector<QPointF>& points);
    static QVector<QPointF> CheckLoops(const QVector<QPointF>& points);
    static QVector<QPointF> EkvPoint(const SeamAllowancePoint& p1_line1, const SeamAllowancePoint& p2_line1,
                                     const SeamAllowancePoint& p1_line2, const SeamAllowancePoint& p2_line2,
                                     qreal width, bool* cut = nullptr);
    static QLineF           createParallelLine(const SeamAllowancePoint& p1, const SeamAllowancePoint& p2,
                                               qreal width);
    static QLineF           createParallelLine(const QPointF& p1, const QPointF& p2, qreal width);

    template <class T>
    static QVector<T> CorrectEquidistantPoints(const QVector<T>& points, bool remove_first_and_last = true);

protected:
    template <class T>
    static QVector<T> RemoveDublicates(const QVector<T>& points, bool remove_first_and_last = true);
    static qreal      MaxLocalSA(const SeamAllowancePoint& point, qreal width);
    static bool       IsEkvPointOnLine(const QPointF& point, const QPointF& previous_point,
                                       const QPointF& next_point);
    static bool       IsEkvPointOnLine(const SeamAllowancePoint& point, const SeamAllowancePoint& previous_point,
                                       const SeamAllowancePoint& next_point);

private:
    /**
     * @brief CornerLines holds the seam allowance lines along the two edges of one main path corner.
     */
    struct CornerLines
    {
        QLineF  before;      ///< Seam allowance line along the edge that ends at the corner.
        QLineF  after;       ///< Seam allowance line along the edge that starts at the corner.
        QPointF cross_point; ///< Point where the two lines, extended, cross.
    };

    struct Loop;

    QSharedDataPointer<AbstractPieceData> d; ///< Piece properties, shared between copies.

    static bool isSamePoint(const QPointF& first, const QPointF& second);

    static Loop             findLoop(const QVector<QPointF>& points, int index, bool path_closed);
    static bool             CheckIntersection(const QVector<QPointF>& points, int i, int i_next, int j, int j_next,
                                              const QPointF& cross_point);
    static bool             ParallelCrossPoint(const QLineF& line1, const QLineF& line2, QPointF& point);
    static bool             Crossing(const QVector<QPointF>& sub1, const QVector<QPointF>& sub2);
    static QVector<QPointF> SubPath(const QVector<QPointF>& path, int start_index, int end_index);
    static constexpr qreal  PointPosition(const QPointF& point, const QLineF& line);
    static QVector<QPointF> UnboundedCorner(const SeamAllowancePoint& p1_line1, const SeamAllowancePoint& p2_line1,
                                            const SeamAllowancePoint& p1_line2, const SeamAllowancePoint& p2_line2,
                                            const CornerLines& lines, qreal width, bool* cut);
    static QVector<QPointF> CornerByAngleType(const SeamAllowancePoint& p1_line1,
                                              const SeamAllowancePoint& p2_line1,
                                              const SeamAllowancePoint& p1_line2, const CornerLines& lines,
                                              qreal width, bool* cut);
    static QVector<QPointF> AngleByLength(const QPointF& p2, const QPointF& sp1, const QPointF& sp2,
                                          const QPointF& sp3, qreal width, bool force_cut = false,
                                          bool* cut = nullptr);
    static QVector<QPointF> AngleByIntersection(const QPointF& p1, const QPointF& p2, const QPointF& p3,
                                                const QPointF& sp1, const QPointF& sp2, const QPointF& sp3,
                                                qreal width);
    static QVector<QPointF> AngleByFirstSymmetry(const QPointF& p1, const QPointF& p2,
                                                 const QPointF& sp1, const QPointF& sp2, const QPointF& sp3,
                                                 qreal width);
    static QVector<QPointF> AngleBySecondSymmetry(const QPointF& p2, const QPointF& p3,
                                                  const QPointF& sp1, const QPointF& sp2, const QPointF& sp3,
                                                  qreal width);
    static QVector<QPointF> AngleByFirstRightAngle(const QPointF& p1, const QPointF& p2,
                                                   const QPointF& sp1, const QPointF& sp2, const QPointF& sp3,
                                                   qreal width);
    static QVector<QPointF> AngleBySecondRightAngle(const QPointF& p2, const QPointF& p3,
                                                    const QPointF& sp1, const QPointF& sp2, const QPointF& sp3,
                                                    qreal width);

    static QPointF SingleParallelPoint(const QPointF& p1, const QPointF& p2, qreal angle, qreal width);
    static QLineF  BisectorLine(const QPointF& p1, const QPointF& p2, const QPointF& p3);
    static qreal   AngleBetweenBisectors(const QLineF& b1, const QLineF& b2);
};

Q_DECLARE_TYPEINFO(AbstractPiece, Q_MOVABLE_TYPE);

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isSamePoint tells whether two points are closer than accuracyPointOnLine on both axes.
 */
inline bool AbstractPiece::isSamePoint(const QPointF& first, const QPointF& second)
{
    return qAbs(first.x() - second.x()) < VGObject::accuracyPointOnLine
           && qAbs(first.y() - second.y()) < VGObject::accuracyPointOnLine;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CorrectEquidistantPoints removes repeated points, and points on the line between their neighbours.
 * @param points                main path or seam allowance points.
 * @param remove_first_and_last also remove a last point that repeats the first one.
 * @return the corrected points; a list of 3 points or fewer comes back unchanged.
 */
template <class T>
QVector<T> AbstractPiece::CorrectEquidistantPoints(const QVector<T>& points, bool remove_first_and_last)
{
    QVector<T> corrected;
    if (points.size() < 4)
    {
        // Removing a point from 3 points or fewer can destroy the seam allowance.
        qDebug() << "Only three points.";
        corrected = points;
    }
    else
    {
        // Clear repeated points first.
        const QVector<T> unique = RemoveDublicates(points, remove_first_and_last);
        if (unique.size() < 3)
        {
            corrected = unique;
        }
        else
        {
            // Remove points on a line. QLineF::intersect cannot test this: floating point accuracy breaks it.
            // previous is the nearest earlier point that was kept; before a point is kept, the point before i.
            QVector<T> kept;
            int previous = -1;
            for (int i = 0; i < unique.size(); ++i)
            {
                if (previous == -1)
                {
                    previous = i == 0 ? unique.size() - 1 : i - 1;
                }
                const int next = i == unique.size() - 1 ? 0 : i + 1;

                const T& point          = unique.at(i);
                const T& previous_point = unique.at(previous);
                const T& next_point     = unique.at(next);

                // A point is dropped when it lies on the line and its two neighbours are the same point.
                // A first or last point equal to a neighbour is kept: it closes the path.
                const bool dropped = IsEkvPointOnLine(point, previous_point, next_point)
                                     && previous_point == next_point;
                const bool closes_path = (i == 0 || i == unique.size() - 1)
                                         && (point == previous_point || point == next_point);
                if (!dropped || closes_path)
                {
                    kept.append(point);
                    previous = -1;
                }
            }
            corrected = RemoveDublicates(kept, false);
        }
    }
    return corrected;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RemoveDublicates removes each point that repeats the point before it.
 * @param points                path points.
 * @param remove_first_and_last also remove a last point that repeats the first one; the path is then open.
 * @return the points without repeats, within accuracyPointOnLine.
 */
template <class T>
QVector<T> AbstractPiece::RemoveDublicates(const QVector<T>& points, bool remove_first_and_last)
{
    QVector<T> unique = points;

    // The path cannot be closed. See issue #686.
    if (remove_first_and_last && unique.size() > 1 && isSamePoint(unique.first(), unique.last()))
    {
        unique.removeLast();
    }

    // A removed point lets the next point move up to index i + 1, so i advances only past a kept point.
    int i = 0;
    while (i < unique.size() - 1)
    {
        if (isSamePoint(unique.at(i), unique.at(i + 1)))
        {
            unique.removeAt(i + 1);
        }
        else
        {
            ++i;
        }
    }

    return unique;
}

#endif // ABSTRACTPIECE_H
