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
 * The Windows MSI writes the wizard answers under HKLM\SOFTWARE\Seamly\Seamly2D.
 *
 * HKLM, not HKCU: a per-machine MSI runs as LocalSystem and cannot write a user's hive.
 * So every value is machine-wide, and a per-user setting always wins.
 *
 * An empty result means "no answer": off Windows, or when no installer recorded the value.
 * The caller supplies its own default.
 *
 * Lives in vmisc so Seamly2DTests can test it. The suite links libraries, not app sources.
 */
namespace InstallerRecord
{
    QString dataRoot();
}

#endif // INSTALLER_RECORD_H
