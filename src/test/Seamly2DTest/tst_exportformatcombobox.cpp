/******************************************************************************
 **  @file   tst_exportformatcombobox.cpp
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for the export formats that Seamly2D offers.
 **
 **  @copyright
 **  This source code is part of the Seamly2D project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
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
 *****************************************************************************/

#include "tst_exportformatcombobox.h"
#include "../vwidgets/export_format_combobox.h"

#include <QtTest>

//---------------------------------------------------------------------------------------------------------------------
TST_ExportFormatCombobox::TST_ExportFormatCombobox(QObject *parent)
    : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Export Draft Blocks needs PDF and SVG export.
 */
void TST_ExportFormatCombobox::offersPdfAndSvg()
{
    const QString help = ExportFormatCombobox::makeHelpFormatList();
    QVERIFY2(help.contains(QLatin1String("(*.svg) = 0")), qUtf8Printable(help));
    QVERIFY2(help.contains(QLatin1String("(*.pdf) = 1")), qUtf8Printable(help));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PS and EPS are not offered.
 */
void TST_ExportFormatCombobox::offersNoPostScript()
{
    const QString help = ExportFormatCombobox::makeHelpFormatList();
    QVERIFY2(!help.contains(QLatin1String("(*.ps)")), qUtf8Printable(help));
    QVERIFY2(!help.contains(QLatin1String("(*.eps)")), qUtf8Printable(help));
    QVERIFY2(!help.contains(QLatin1String(" = 8\n")), qUtf8Printable(help));
    QVERIFY2(!help.contains(QLatin1String(" = 9\n")), qUtf8Printable(help));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Tiled PDF is not offered: SeamlyLayout makes the tiled PDF.
 */
void TST_ExportFormatCombobox::offersNoTiledPdf()
{
    const QString help = ExportFormatCombobox::makeHelpFormatList();
    QVERIFY2(!help.contains(QLatin1String("PDF tiled")), qUtf8Printable(help));
    QVERIFY2(!help.contains(QLatin1String(" = 2\n")), qUtf8Printable(help));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DXF and Wavefront OBJ are not offered: they served the removed Export Pieces.
 */
void TST_ExportFormatCombobox::offersNoDxfOrObj()
{
    const QString help = ExportFormatCombobox::makeHelpFormatList();
    QVERIFY2(!help.contains(QLatin1String("(*.dxf)")), qUtf8Printable(help));
    QVERIFY2(!help.contains(QLatin1String("(*.obj)")), qUtf8Printable(help));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Format numbers do not move when formats are removed: the TIF number follows the unused DXF numbers.
 */
void TST_ExportFormatCombobox::keepsFormatNumbers()
{
    QCOMPARE(static_cast<int>(LayoutExportFormat::PNG), 3);
    QCOMPARE(static_cast<int>(LayoutExportFormat::OBJ), 7);
    QCOMPARE(static_cast<int>(LayoutExportFormat::DXF_AC1006_Flat), 10);
    QCOMPARE(static_cast<int>(LayoutExportFormat::TIF), 37);
    const QString help = ExportFormatCombobox::makeHelpFormatList();
    QVERIFY2(help.contains(QLatin1String("(*.tif) = 37\n")), qUtf8Printable(help));
}
