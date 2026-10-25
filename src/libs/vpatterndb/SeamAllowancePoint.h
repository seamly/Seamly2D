//---------------------------------------------------------------------------------------------------------------------
//  @file   SeamAllowancePoint.h
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
//  @file   SeamAllowancePoint.h
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
//  @file   SeamAllowancePoint.h
//  @author slspencer
//
//  @brief
//  Point of a seam allowance path, with the seam allowance widths and the corner rule at that point.
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

#ifndef SEAMALLOWANCEPOINT_H
#define SEAMALLOWANCEPOINT_H

#include <QMetaType>
#include <QPointF>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "../vmisc/diagnostic.h"

// QPointF has no virtual destructor; nothing deletes a SeamAllowancePoint through a QPointF pointer.
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")
QT_WARNING_DISABLE_GCC("-Wnon-virtual-dtor")

/**
 * @brief SeamAllowancePoint is a main path point with the seam allowance widths before and after it.
 *
 * A negative width means "use the piece width". The angle type sets how AbstractPiece::Equidistant() builds the
 * seam allowance corner at this point.
 */
class SeamAllowancePoint : public QPointF
{
public:
    constexpr SeamAllowancePoint();
    constexpr SeamAllowancePoint(qreal x_position, qreal y_position);
    constexpr explicit SeamAllowancePoint(const QPointF& point);

    constexpr qreal GetSABefore() const;
    qreal           GetSABefore(qreal width) const;
    void            SetSABefore(qreal value);

    constexpr qreal GetSAAfter() const;
    qreal           GetSAAfter(qreal width) const;
    void            SetSAAfter(qreal value);

    constexpr PieceNodeAngle GetAngleType() const;
    void                     SetAngleType(PieceNodeAngle value);

    constexpr bool isNode() const;
    void           setNode(bool value);

    constexpr int cornerIndex() const;
    void          setCornerIndex(int value);

    constexpr bool isCornerCutForced() const;
    void           setCornerCutForced(bool value);

private:
    qreal          m_before;            ///< Seam allowance width before the point; -1 uses the piece width.
    qreal          m_after;             ///< Seam allowance width after the point; -1 uses the piece width.
    PieceNodeAngle m_angle;             ///< Rule that builds the seam allowance corner at the point.
    bool           m_node;              ///< True for a piece node or a curve end, false for a curve interior point.
    int            m_corner_index;      ///< Number of this node point among the node points of its path; -1 if none.
    bool           m_corner_cut_forced; ///< Cut a ByLength corner even when its miter is short enough to keep.
};

QT_WARNING_POP

Q_DECLARE_METATYPE(SeamAllowancePoint)
Q_DECLARE_TYPEINFO(SeamAllowancePoint, Q_MOVABLE_TYPE);

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeamAllowancePoint makes a point at the origin that uses the piece width on both sides.
 */
constexpr inline SeamAllowancePoint::SeamAllowancePoint()
    : QPointF()
    , m_before(-1)
    , m_after(-1)
    , m_angle(PieceNodeAngle::ByLength)
    , m_node(false)
    , m_corner_index(-1)
    , m_corner_cut_forced(false)
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeamAllowancePoint makes a point at (x_position, y_position) that uses the piece width on both sides.
 */
constexpr inline SeamAllowancePoint::SeamAllowancePoint(qreal x_position, qreal y_position)
    : QPointF(x_position, y_position)
    , m_before(-1)
    , m_after(-1)
    , m_angle(PieceNodeAngle::ByLength)
    , m_node(false)
    , m_corner_index(-1)
    , m_corner_cut_forced(false)
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeamAllowancePoint makes a point at point that uses the piece width on both sides.
 */
constexpr inline SeamAllowancePoint::SeamAllowancePoint(const QPointF& point)
    : QPointF(point)
    , m_before(-1)
    , m_after(-1)
    , m_angle(PieceNodeAngle::ByLength)
    , m_node(false)
    , m_corner_index(-1)
    , m_corner_cut_forced(false)
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSABefore returns the stored width before the point; -1 means the piece width.
 */
constexpr inline qreal SeamAllowancePoint::GetSABefore() const
{
    return m_before;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSABefore returns the width before the point, or width when the point uses the piece width.
 */
inline qreal SeamAllowancePoint::GetSABefore(qreal width) const
{
    return m_before < 0 ? width : m_before;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetSABefore sets the width before the point; a negative value means the piece width.
 */
inline void SeamAllowancePoint::SetSABefore(qreal value)
{
    m_before = value < 0 ? -1 : value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSAAfter returns the stored width after the point; -1 means the piece width.
 */
constexpr inline qreal SeamAllowancePoint::GetSAAfter() const
{
    return m_after;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSAAfter returns the width after the point, or width when the point uses the piece width.
 */
inline qreal SeamAllowancePoint::GetSAAfter(qreal width) const
{
    return m_after < 0 ? width : m_after;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetSAAfter sets the width after the point; a negative value means the piece width.
 */
inline void SeamAllowancePoint::SetSAAfter(qreal value)
{
    m_after = value < 0 ? -1 : value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetAngleType returns the rule that builds the seam allowance corner at the point.
 */
constexpr inline PieceNodeAngle SeamAllowancePoint::GetAngleType() const
{
    return m_angle;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetAngleType sets the rule that builds the seam allowance corner at the point.
 */
inline void SeamAllowancePoint::SetAngleType(PieceNodeAngle value)
{
    m_angle = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isNode tells whether the point is a piece node or a curve end.
 */
constexpr inline bool SeamAllowancePoint::isNode() const
{
    return m_node;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setNode marks the point as a piece node or a curve end.
 */
inline void SeamAllowancePoint::setNode(bool value)
{
    m_node = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief cornerIndex returns the number of this node point among the node points of its path; -1 if none.
 */
constexpr inline int SeamAllowancePoint::cornerIndex() const
{
    return m_corner_index;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setCornerIndex sets the number that AbstractPiece::Equidistant() reports for a cut corner.
 */
inline void SeamAllowancePoint::setCornerIndex(int value)
{
    m_corner_index = value;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isCornerCutForced tells whether a ByLength corner is cut even when its miter is short enough to keep.
 */
constexpr inline bool SeamAllowancePoint::isCornerCutForced() const
{
    return m_corner_cut_forced;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setCornerCutForced sets whether a ByLength corner is cut whenever its miter is longer than the width.
 */
inline void SeamAllowancePoint::setCornerCutForced(bool value)
{
    m_corner_cut_forced = value;
}

#endif // SEAMALLOWANCEPOINT_H
