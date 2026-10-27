/******************************************************************************
 **  @file   tst_handoffids.cpp
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for the handoff ids of layout_handoff
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

#include "tst_handoffids.h"
#include "../layout_handoff/handoff_ids.h"

#include <QtTest>

using namespace layout_handoff;

//---------------------------------------------------------------------------------------------------------------------
TST_HandoffIds::TST_HandoffIds(QObject *parent)
    : QObject(parent)
{}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffIds::sanitizesNames() const
{
    QCOMPARE(sanitize_for_id(QStringLiteral("Front Bodice")), QStringLiteral("Front_Bodice"));
    QCOMPARE(sanitize_for_id(QStringLiteral("  a/b  ")), QStringLiteral("a_b"));
    QCOMPARE(sanitize_for_id(QStringLiteral("2nd")), QStringLiteral("_2nd"));
    QCOMPARE(sanitize_for_id(QStringLiteral("***")), QString());
    QCOMPARE(sanitize_for_id(QString()), QString());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffIds::sizeSuffixDropsDigitGuard() const
{
    QCOMPARE(size_id_suffix(QStringLiteral("34")), QStringLiteral("_s34"));
    QCOMPARE(size_id_suffix(QStringLiteral("13.5")), QStringLiteral("_s13.5"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffIds::pieceIdUsesNameSizeAndFallback() const
{
    HandoffIds ids;
    QCOMPARE(ids.piece_id(QStringLiteral("piece-1"), QStringLiteral("Yoke"), QString(), 1),
             QStringLiteral("piece_Yoke"));
    QCOMPARE(ids.piece_id(QStringLiteral("piece-2"), QStringLiteral("Sleeve"), QStringLiteral("34"), 2),
             QStringLiteral("piece_Sleeve_s34"));
    QCOMPARE(ids.piece_id(QStringLiteral("piece-3"), QStringLiteral("***"), QString(), 3),
             QStringLiteral("piece-3"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffIds::repeatedPieceIdTakesPieceNumber() const
{
    HandoffIds ids;
    QCOMPARE(ids.piece_id(QStringLiteral("piece-1"), QStringLiteral("Yoke"), QString(), 1),
             QStringLiteral("piece_Yoke"));
    QCOMPARE(ids.piece_id(QStringLiteral("piece-2"), QStringLiteral("Yoke"), QString(), 2),
             QStringLiteral("piece_Yoke-2"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffIds::componentIdFormats() const
{
    HandoffIds ids;
    QCOMPARE(ids.component_id(QStringLiteral("grainline"), 1, QStringLiteral("Sleeve"), QStringLiteral("p"),
                              QString(), 1),
             QStringLiteral("grainline_Sleeve"));
    QCOMPARE(ids.component_id(QStringLiteral("internal_path"), 2, QStringLiteral("Sleeve"), QStringLiteral("p"),
                              QString(), 1),
             QStringLiteral("internal_path_2_Sleeve"));
    QCOMPARE(ids.component_id(QStringLiteral("grainline"), 1, QStringLiteral("Cuff"), QStringLiteral("p"),
                              QStringLiteral("34"), 1),
             QStringLiteral("grainline_Cuff_s34"));
    QCOMPARE(ids.component_id(QStringLiteral("seamline"), 1, QString(), QStringLiteral("piece-7"), QString(), 7),
             QStringLiteral("piece-7-seamline-1"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffIds::repeatedComponentIdTakesPieceNumber() const
{
    HandoffIds ids;
    QCOMPARE(ids.component_id(QStringLiteral("grainline"), 1, QStringLiteral("Sleeve"), QStringLiteral("p"),
                              QString(), 1),
             QStringLiteral("grainline_Sleeve"));
    QCOMPARE(ids.component_id(QStringLiteral("grainline"), 1, QStringLiteral("Sleeve"), QStringLiteral("p"),
                              QString(), 2),
             QStringLiteral("grainline_Sleeve-2"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffIds::pieceSetIdSharesPieceIdSpace() const
{
    HandoffIds ids;
    QCOMPARE(ids.piece_set_id(QStringLiteral("Yoke"), 1), QStringLiteral("piece-set_Yoke"));
    QCOMPARE(ids.piece_set_id(QStringLiteral("Yoke"), 2), QStringLiteral("piece-set_Yoke-2"));
    QCOMPARE(ids.piece_set_id(QString(), 3), QStringLiteral("piece-set-3"));
}
