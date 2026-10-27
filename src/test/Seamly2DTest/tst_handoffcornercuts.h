/******************************************************************************
 **  @file   tst_handoffcornercuts.h
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for the corner cut compare of layout_handoff.
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
 **  along with Seamly2D.  If not, see <https://www.gnu.org/licenses/gpl-3.0.html>.
 **
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **
 *****************************************************************************/

#ifndef TST_HANDOFFCORNERCUTS_H
#define TST_HANDOFFCORNERCUTS_H

#include <QObject>

/**
 * @brief TST_HandoffCornerCuts unit tests for the cut seam allowance corners compared across the sizes of a piece.
 */
class TST_HandoffCornerCuts : public QObject
{
    Q_OBJECT

public:
    explicit TST_HandoffCornerCuts(QObject* parent = nullptr);

private slots:
    void sameCutsNeedNoChange() const;
    void differentCutsWithSameCountAreUneven() const;
    void differentCornerCountIsLeftOut() const;
    void noSizesNeedNoChange() const;
};

#endif // TST_HANDOFFCORNERCUTS_H
