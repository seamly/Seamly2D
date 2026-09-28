/******************************************************************************
 **  @file   tst_exportformatcombobox.h
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

#ifndef TST_EXPORTFORMATCOMBOBOX_H
#define TST_EXPORTFORMATCOMBOBOX_H

#include <QObject>

/**
 * @brief TST_ExportFormatCombobox tests the format list shared by the export
 * dialogs and the command line help.
 *
 * PS and EPS are exported by SeamlyLayout only; Seamly2D offers neither.
 */
class TST_ExportFormatCombobox : public QObject
{
    Q_OBJECT
public:
    explicit TST_ExportFormatCombobox(QObject *parent = nullptr);

private slots:
    void offersPdfAndSvg();
    void offersNoPostScript();
    void keepsFormatNumbers();

private:
    Q_DISABLE_COPY(TST_ExportFormatCombobox)
};

#endif // TST_EXPORTFORMATCOMBOBOX_H
