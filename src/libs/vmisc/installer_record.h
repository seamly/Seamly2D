/******************************************************************************
 **  @file   installer_record.h
 **  @author slspencer
 **  @date   August 19, 2026
 **
 **  @brief
 **  Reads the values the Windows installer recorded about this installation.
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

#ifndef INSTALLER_RECORD_H
#define INSTALLER_RECORD_H

#include <QString>

/**
 * @brief InstallerRecord reads what Setup wrote down about this installation.
 *
 * The Windows MSI records its answers under HKLM\SOFTWARE\Seamly\Seamly2D. The
 * apps read them back so a folder the user chose in the wizard is the folder
 * the apps use (Task InstWinX64.00).
 *
 * HKLM, not HKCU: a per-machine MSI runs its server side as LocalSystem and
 * cannot write a real user's hive. Every value here is therefore machine-wide,
 * and a per-user setting always outranks it.
 *
 * Every function returns an empty string off Windows, and an empty string when
 * no installer recorded that value — an unpackaged build, a developer tree, or
 * a silent install that chose nothing. An empty result means "no answer", never
 * "the answer is the default"; the caller supplies its own default.
 *
 * The functions live in vmisc rather than in an app target so the Seamly2DTests
 * suite can exercise them: it links the static libraries, not the application
 * sources. Same reason as SeamlySuitePaths.
 */
namespace InstallerRecord
{
    QString dataRoot();
}

#endif // INSTALLER_RECORD_H
