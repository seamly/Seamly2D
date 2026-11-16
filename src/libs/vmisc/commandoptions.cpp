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
 **  @file   commandoptions.cpp
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

#include "commandoptions.h"
#include "def.h"

#include <QString>
#include <QStringList>

const QString LONG_OPTION_MEASUREFILE       = QStringLiteral("mfile");
const QString SINGLE_OPTION_MEASUREFILE     = QStringLiteral("m");

const QString LONG_OPTION_TEST              = QStringLiteral("test");
const QString SINGLE_OPTION_TEST            = QStringLiteral("t");

const QString LONG_OPTION_GRADATIONSIZE     = QStringLiteral("gsize");
const QString SINGLE_OPTION_GRADATIONSIZE   = QStringLiteral("x");

const QString LONG_OPTION_GRADATIONHEIGHT   = QStringLiteral("gheight");
const QString SINGLE_OPTION_GRADATIONHEIGHT = QStringLiteral("e");

const QString LONG_OPTION_LAYOUT_EXPORT          = QStringLiteral("layout-export");
const QString LONG_OPTION_LAYOUT_FORMAT          = QStringLiteral("layout-format");
const QString LONG_OPTION_LAYOUT_OUTPUT          = QStringLiteral("layout-output");
const QString LONG_OPTION_LAYOUT_SETTINGS        = QStringLiteral("layout-settings");
const QString LONG_OPTION_LAYOUT_EXPORT_OPTIONS  = QStringLiteral("layout-export-options");
const QString LONG_OPTION_SVG_TEXT_MODE           = QStringLiteral("svg-text-mode");
const QString LONG_OPTION_LAYOUT_SCALE            = QStringLiteral("layout-scale");
const QString LONG_OPTION_LAYOUT_MATERIAL         = QStringLiteral("layout-material");
const QString LONG_OPTION_LAYOUT_VIEW             = QStringLiteral("layout-view");
const QString LONG_OPTION_LAYOUT_OVERSIZE         = QStringLiteral("layout-oversize");
const QString LONG_OPTION_LAYOUT_INFO             = QStringLiteral("layout-info");

//---------------------------------------------------------------------------------------------------------------------
QStringList AllKeys()
{
    QStringList list;
    list << LONG_OPTION_MEASUREFILE << SINGLE_OPTION_MEASUREFILE
         << LONG_OPTION_TEST << SINGLE_OPTION_TEST
         << LONG_OPTION_GRADATIONSIZE << SINGLE_OPTION_GRADATIONSIZE
         << LONG_OPTION_GRADATIONHEIGHT << SINGLE_OPTION_GRADATIONHEIGHT
         << LONG_OPTION_LAYOUT_EXPORT << LONG_OPTION_LAYOUT_FORMAT
         << LONG_OPTION_LAYOUT_OUTPUT << LONG_OPTION_LAYOUT_SETTINGS
         << LONG_OPTION_LAYOUT_EXPORT_OPTIONS << LONG_OPTION_SVG_TEXT_MODE
         << LONG_OPTION_LAYOUT_SCALE << LONG_OPTION_LAYOUT_MATERIAL
         << LONG_OPTION_LAYOUT_VIEW << LONG_OPTION_LAYOUT_OVERSIZE << LONG_OPTION_LAYOUT_INFO
         << LONG_OPTION_NO_HDPI_SCALING;

    return list;
}
