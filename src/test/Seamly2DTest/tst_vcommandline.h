/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2017  Seamly, LLC                                       *
 *                                                                         *
 *   https://github.com/fashionfreedom/seamly2d                             *
 *                                                                         *
 ***************************************************************************
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
 **************************************************************************

 ************************************************************************
 **
 **  @file   tst_vcommandline.h
 **  @author slspencer
 **  @date   2026
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentine project, a pattern making
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
 *************************************************************************/

#ifndef TST_VCOMMANDLINE_H
#define TST_VCOMMANDLINE_H

#include <QObject>

class TST_VCommandLine : public QObject
{
    Q_OBJECT
public:
    explicit TST_VCommandLine(QObject *parent = nullptr);

private slots:
    void UniqueKeys();
    void LayoutExportOptions();
    void LayoutExportOptionsAppearInHelp();
    void LayoutExportUsesDefaultSvgFormat();
    void LayoutExportRequiresOutput();
    void LayoutExportRequiresOneInput();
    void LayoutExportRejectsUnsupportedFormat();
    void LayoutExportRejectsSvgTextModeForOtherFormats();
    void LayoutScaleOptionIsRemoved();
    void LayoutExportOptionsRequireExportMode();
    void LayoutChoiceOptions();
    void LayoutChoiceOptionsAppearInHelp();
    void LayoutOversizeRejectsUnknownChoice();
    void LayoutNameOptionsNeedAName();
    void LayoutViewRejectsGradationOptions();
    void LayoutInfoMode();
    void LayoutInfoRejectsExportChoices();
    void LayoutInfoAndExportConflict();
    void LayoutInfoRequiresOneInput();
    void LayoutSettingsRequireExportOrInfo();
    void RemovedLegacyOptionsFailToParse();
    void MeasurementOverrideIsAbsolute();
    void Text2PathsForVectorFormats();
    void Text2PathsRejectedCombinations();
    void LayoutFlagsForwardedInOrder();
    void LayoutFlagsNeedExportOrInfo();
    void LayoutNoPackingCombinations();

private:
    Q_DISABLE_COPY(TST_VCommandLine)
};

#endif // TST_VCOMMANDLINE_H
