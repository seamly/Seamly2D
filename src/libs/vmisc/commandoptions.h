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
 **  @file   commandoptions.h
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

#ifndef COMMANDOPTIONS_H
#define COMMANDOPTIONS_H

#include <QString>

extern const QString LONG_OPTION_MEASUREFILE;
extern const QString SINGLE_OPTION_MEASUREFILE;

extern const QString LONG_OPTION_TEST;
extern const QString SINGLE_OPTION_TEST;

extern const QString LONG_OPTION_GRADATIONSIZE;
extern const QString SINGLE_OPTION_GRADATIONSIZE;

extern const QString LONG_OPTION_GRADATIONHEIGHT;
extern const QString SINGLE_OPTION_GRADATIONHEIGHT;

extern const QString LONG_OPTION_LAYOUT_EXPORT;
extern const QString LONG_OPTION_LAYOUT_FORMAT;
extern const QString LONG_OPTION_LAYOUT_OUTPUT;
extern const QString LONG_OPTION_LAYOUT_SETTINGS;
extern const QString LONG_OPTION_LAYOUT_EXPORT_OPTIONS;
extern const QString LONG_OPTION_SVG_TEXT_MODE;
extern const QString LONG_OPTION_LAYOUT_SCALE;

QStringList AllKeys();

#endif // COMMANDOPTIONS_H
