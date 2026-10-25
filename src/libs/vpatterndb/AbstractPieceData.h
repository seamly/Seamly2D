//---------------------------------------------------------------------------------------------------------------------
//  @file   AbstractPieceData.h
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
//  @file   AbstractPieceData.h
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date   9 11, 2016
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
//  @file   AbstractPieceData.h
//  @author slspencer
//
//  @brief
//  Shared data of AbstractPiece: the piece properties that do not depend on the piece geometry.
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

#ifndef ABSTRACTPIECEDATA_H
#define ABSTRACTPIECEDATA_H

#include <QCoreApplication>
#include <QSharedData>
#include <QString>

#include "../vmisc/def.h"
#include "../vmisc/diagnostic.h"

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")
QT_WARNING_DISABLE_GCC("-Wnon-virtual-dtor")

/**
 * @brief AbstractPieceData holds the AbstractPiece properties; copies share it until one of them changes.
 */
class AbstractPieceData : public QSharedData
{
    Q_DECLARE_TR_FUNCTIONS(AbstractPieceData)

public:
    /**
     * @brief AbstractPieceData makes the properties of a new piece: named "Piece", white, Fabric, no seam allowance.
     */
    AbstractPieceData()
        : m_name(tr("Piece"))
        , m_color(QStringLiteral("white"))
        , m_fill(QStringLiteral("nobrush"))
        , m_piece_lock(false)
        , m_flip(PieceFlip::None)
        , m_seam_allowance(false)
        , m_seam_allowance_built_in(false)
        , m_hide_seam_line(false)
        , m_seam_allowance_width(0)
        , m_position_x(0)
        , m_position_y(0)
        , m_material(QStringLiteral("Fabric"))
    {
    }

    /**
     * @brief AbstractPieceData copies the properties of another piece.
     */
    AbstractPieceData(const AbstractPieceData& piece)
        : QSharedData(piece)
        , m_name(piece.m_name)
        , m_color(piece.m_color)
        , m_fill(piece.m_fill)
        , m_piece_lock(piece.m_piece_lock)
        , m_flip(piece.m_flip)
        , m_seam_allowance(piece.m_seam_allowance)
        , m_seam_allowance_built_in(piece.m_seam_allowance_built_in)
        , m_hide_seam_line(piece.m_hide_seam_line)
        , m_seam_allowance_width(piece.m_seam_allowance_width)
        , m_position_x(piece.m_position_x)
        , m_position_y(piece.m_position_y)
        , m_material(piece.m_material)
    {
    }

    ~AbstractPieceData() = default;

    QString   m_name;                    ///< Piece name, as the user typed it.
    QString   m_color;                   ///< Piece outline color name.
    QString   m_fill;                    ///< Piece fill pattern name.
    bool      m_piece_lock;              ///< True when the user locked the piece against edits.
    PieceFlip m_flip;                    ///< Mirror a layout may apply to the piece.
    bool      m_seam_allowance;          ///< True when the piece has a seam allowance.
    bool      m_seam_allowance_built_in; ///< True when the main path already includes the seam allowance.
    bool      m_hide_seam_line;          ///< True when Piece mode does not draw the seam line.
    qreal     m_seam_allowance_width;    ///< Piece seam allowance width in pixels; never negative.
    qreal     m_position_x;              ///< Piece x position in Piece mode, in pixels.
    qreal     m_position_y;              ///< Piece y position in Piece mode, in pixels.
    QString   m_material;                ///< Material the piece is cut from, as stored in the pattern file.

private:
    AbstractPieceData& operator=(const AbstractPieceData&) = delete;
};

QT_WARNING_POP

#endif // ABSTRACTPIECEDATA_H
