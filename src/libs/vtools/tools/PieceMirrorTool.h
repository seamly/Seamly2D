/******************************************************************************
 **  @file   PieceMirrorTool.h
 **  @author slspencer
 **
 **  @brief
 **  Piece Mirror tool: makes a full piece from a half piece and its fold line.
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

#ifndef PIECEMIRRORTOOL_H
#define PIECEMIRRORTOOL_H

#include <QCoreApplication>
#include <QSharedPointer>
#include <QString>
#include <QtGlobal>

class DialogTool;
class PatternPieceTool;
class VAbstractPattern;
class VContainer;
class VMainGraphicsScene;

/**
 * @brief PieceMirrorTool makes a full piece from a half piece with a fold line.
 *
 * A Mirror by Line operation at the end of the draft block mirrors every object of the half piece across the fold
 * line. The full piece is an ordinary piece made of the half piece objects and their mirrored copies, so it follows
 * every draft change. The half piece stays in the pattern. The tool writes no element of its own, so the pattern
 * file format does not change.
 */
class PieceMirrorTool
{
    Q_DECLARE_TR_FUNCTIONS(PieceMirrorTool)
public:
    static PatternPieceTool* Create(QSharedPointer<DialogTool> dialog, VMainGraphicsScene* draft_scene,
                                    VMainGraphicsScene* piece_scene, VAbstractPattern* doc, VContainer* data);
    static PatternPieceTool* Create(quint32 piece_id, bool remove_half, VMainGraphicsScene* draft_scene,
                                    VMainGraphicsScene* piece_scene, VAbstractPattern* doc, VContainer* data);
    static QString           mirrorProblem(const VContainer* data, quint32 piece_id);
};

#endif // PIECEMIRRORTOOL_H
