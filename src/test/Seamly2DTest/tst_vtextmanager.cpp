/******************************************************************************
 **  @file   tst_vtextmanager.cpp
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for the label font fit of LabelTextManager
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

#include "tst_vtextmanager.h"
#include "../vpatterndb/LabelTextManager.h"

#include <QtTest>
#include <QFont>
#include <QFontMetrics>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief lineWidth returns the pixel width of a label line at a base font size.
 */
int lineWidth(const QFont &font, const TextLine &line, int size)
{
    QFont fnt = font;
    fnt.setBold(line.bold);
    fnt.setItalic(line.italic);
    fnt.setPixelSize(size + line.m_iFontSize);
    return QFontMetrics(fnt).horizontalAdvance(line.m_text);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_VTextManager::TST_VTextManager(QObject *parent)
    : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VTextManager::LargestFittingSize_data() const
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("increment");
    QTest::addColumn<bool>("bold");
    QTest::addColumn<bool>("italic");
    QTest::addColumn<int>("tooWideSize");
    QTest::addColumn<qreal>("width");

    QTest::newRow("plain line") << QStringLiteral("Fabric, Cut 1") << 0 << false << false << 47 << 100.0;
    QTest::newRow("bold line with increment") << QStringLiteral("Detail 1") << 2 << true << false << 47 << 80.0;
    QTest::newRow("italic line") << QStringLiteral("Interfacing, Cut 2 on fold") << 0 << false << true << 40
                                 << 150.0;
    QTest::newRow("nothing fits") << QStringLiteral("WWWWWWWWWW") << 6 << true << false << 20 << 10.0;
    QTest::newRow("start at the minimum") << QStringLiteral("Front") << 0 << false << false << MIN_FONT_SIZE << 1.0;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LargestFittingSize checks that the result fits and the next size up does not.
 *
 * The search starts from an estimate instead of stepping down from the size that is too wide. It must still
 * return the same size as that step-down search: the largest one that fits, or MIN_FONT_SIZE.
 */
void TST_VTextManager::LargestFittingSize() const
{
    QFETCH(QString, text);
    QFETCH(int, increment);
    QFETCH(bool, bold);
    QFETCH(bool, italic);
    QFETCH(int, tooWideSize);
    QFETCH(qreal, width);

    TextLine line;
    line.m_text = text;
    line.m_iFontSize = increment;
    line.bold = bold;
    line.italic = italic;

    const QFont font;
    const int tooWideWidth = lineWidth(font, line, tooWideSize);
    QVERIFY2(tooWideWidth > width, "The row must start at a size that is too wide.");

    const int size = LabelTextManager::largestFittingSize(font, line, tooWideSize, tooWideWidth, width);

    QVERIFY(size >= MIN_FONT_SIZE);
    QVERIFY(size <= qMax(MIN_FONT_SIZE, tooWideSize - 1));
    if (size > MIN_FONT_SIZE)
    {
        QVERIFY2(lineWidth(font, line, size) <= width, "The result must fit.");
    }
    if (size + 1 < tooWideSize)
    {
        QVERIFY2(lineWidth(font, line, size + 1) > width, "The next size up must not fit.");
    }
}
