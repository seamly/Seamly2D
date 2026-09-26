/******************************************************************************
 **  @file   new_piece_defaults.h
 **  @author slspencer
 **
 **  @brief
 **  Default label and grainline values for a new pattern piece, read from
 **  Preferences > Pattern and converted to pattern units.
 **
 **  @copyright
 **  This source code is part of the Seamly2D project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
 **
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **
 **  Seamly2D is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Seamly2D is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
 **
 *****************************************************************************/

#ifndef NEW_PIECE_DEFAULTS_H
#define NEW_PIECE_DEFAULTS_H

#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../ifc/ifcdef.h"
#include "../vmisc/def.h"

/// @brief NewPieceDefaults gives the Add New Pattern Piece dialog and the Union tool one source of default values.
namespace NewPieceDefaults
{
/// @brief labelWidth returns the default label width in pattern units.
qreal labelWidth(Unit patternUnit);

/// @brief labelHeight returns the default label height in pattern units.
qreal labelHeight(Unit patternUnit);

/// @brief arrowLength returns the default grainline arrow length in pattern units.
qreal arrowLength(Unit patternUnit);

/// @brief grainlineLength returns the default grainline length in pattern units, long enough for both arrows.
qreal grainlineLength(Unit patternUnit);

/**
 * @brief fitGrainlineLength makes a grainline long enough to show two arrows.
 * @param length      grainline length.
 * @param arrowLength arrow length, in the same unit as @p length.
 * @return @p length, or 2.1 × @p arrowLength when @p length is shorter than two arrows.
 */
qreal fitGrainlineLength(qreal length, qreal arrowLength);

/**
 * @brief readLabelTemplate reads the lines of a label template.
 * @param userFile    template file the user chose, or the default path under the data root.
 * @param builtInFile resource copy, read when @p userFile does not exist.
 * @return the template lines; empty when neither file exists.
 */
QVector<VLabelTemplateLine> readLabelTemplate(const QString &userFile, const QString &builtInFile);

/// @brief pieceLabelTemplate returns the default piece label lines.
QVector<VLabelTemplateLine> pieceLabelTemplate();

/// @brief patternLabelTemplate returns the default pattern label lines.
QVector<VLabelTemplateLine> patternLabelTemplate();
}

#endif // NEW_PIECE_DEFAULTS_H
