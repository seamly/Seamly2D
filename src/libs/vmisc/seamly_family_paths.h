/******************************************************************************
 **  @file   seamly_family_paths.h
 **  @author slspencer
 **  @date   July 22, 2026
 **
 **  @brief
 **  Helpers for locating the executables of the Seamly app family
 **  (seamly2d, seamlyme, SeamlyLayout) relative to an install directory.
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

#ifndef SEAMLY_FAMILY_PATHS_H
#define SEAMLY_FAMILY_PATHS_H

#include <QString>

/**
 * @brief SeamlyFamilyPaths groups filesystem lookups shared by the Seamly app
 * family (seamly2d, seamlyme, SeamlyLayout).
 *
 * The functions live in vmisc (not in the seamly2d app target) so they can be
 * exercised by the Seamly2DTests suite, which links the static libraries but
 * not the application sources.
 */
namespace SeamlyFamilyPaths
{
    QString seamlyLayoutExeName();
    QString locateSeamlyLayout(const QString &directory);
    QString locateSeamlyLayoutDevBuild(const QString &startDirectory);
}

#endif // SEAMLY_FAMILY_PATHS_H
