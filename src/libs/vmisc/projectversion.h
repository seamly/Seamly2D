/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2017  Seamly, LLC                                       *
 *                                                                         *
 *   https://github.com/fashionfreedom/seamly2d                            *
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
 **  @file   projectversion.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   8 7, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentine project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Seamly2D project
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

#ifndef PROJECTVERSION_H
#define PROJECTVERSION_H

class QString;

extern const int MAJOR_VERSION;
extern const int MINOR_VERSION;
// Version YY.M.DDHH: DEBUG_VERSION is day * 100 + hour, so it fits the
// 3-part MSI ProductVersion and sorts by build hour.
extern const int DEBUG_VERSION;

extern const QString APP_VERSION_STR;

/*
   WINDOW_STATE_VERSION keys the blob that QMainWindow::saveState() writes and
   restoreState() reads. It marks the window layout schema, not the build.
   Bump it by hand only when a toolbar or a dock widget is added, removed or
   renamed, so that restoreState() rejects state that no longer fits.

   Do not use the app version here. It carries the build date and hour, so it
   changes with every rolling release, and restoreState() would discard the
   user's toolbar and dock layout on every update.
*/
#define WINDOW_STATE_VERSION 1

// Start: Do not edit here, use packaging/version.sh to update
#define VER_FILEVERSION 26,9,2319,0
#define VER_FILEVERSION_STR "26.9.2319"
// End: Do not edit here

#define V_PRERELEASE // Mark prerelease builds

#define VER_PRODUCTVERSION          VER_FILEVERSION
#define VER_PRODUCTVERSION_STR      VER_FILEVERSION_STR

#define VER_INTERNALNAME_2D_STR     "Seamly2D"
// Task 15: unified organization name — QSettings/QStandardPaths now resolve every Seamly
// app's own settings under one shared "Seamly" folder (e.g. AppData/Local/Seamly/<App> on
// Windows) instead of the old per-product "Seamly2DTeam" org. See vabstractapplication.h's
// MigrateSeamlySettingsLocation() for the first-run migration from the old org folder.
#define VER_COMPANYNAME_STR         "Seamly"
#define VER_LEGALCOPYRIGHT_STR      "Copyright © 2014-2025 Seamly2D Team"
#define VER_LEGALTRADEMARKS1_STR    "All Rights Reserved"
#define VER_LEGALTRADEMARKS2_STR    VER_LEGALTRADEMARKS1_STR
#define VER_COMPANYDOMAIN_STR       "https://seamly.io"
// Bare domain for QCoreApplication::setOrganizationDomain(): a URL scheme here breaks
// the reverse-DNS app_id Qt derives on Wayland (e.g. "io.https://seamly.seamly2d")
#define VER_COMPANYDOMAIN           "seamly.io"

QString compilerString();
QString buildCompatibilityString();

#endif // PROJECTVERSION_H
