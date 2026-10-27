/******************************************************************************
 **  @file   tst_handoffturnpoints.h
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for the turn point unifier of layout_handoff
 **
 **  @copyright
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
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
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **
 *****************************************************************************/

#ifndef TST_HANDOFFTURNPOINTS_H
#define TST_HANDOFFTURNPOINTS_H

#include <QObject>

/**
 * @brief TST_HandoffTurnPoints unit tests for turn points shared between the sizes of a piece line.
 */
class TST_HandoffTurnPoints : public QObject
{
    Q_OBJECT

public:
    explicit TST_HandoffTurnPoints(QObject *parent = nullptr);

private slots:
    void nodeTurnInOneSizeTurnsInAll() const;
    void rotatedLoopMatchesNodesByShape() const;
    void openLineMatchesNodesByOrder() const;
    void skippedLinesReturnNothing() const;
};

#endif // TST_HANDOFFTURNPOINTS_H
