/******************************************************************************
 **  @file   seamly_family_paths.cpp
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

#include "seamly_family_paths.h"

#include <QFileInfo>
#include <QLatin1Char>
#include <QLatin1String>

namespace SeamlyFamilyPaths
{

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief seamlyLayoutExeName returns the platform-specific file name of the
 * SeamlyLayout executable.
 *
 * @return "SeamlyLayout.exe" on Windows, "SeamlyLayout" elsewhere.
 */
QString seamlyLayoutExeName()
{
#ifdef Q_OS_WIN
    return QStringLiteral("SeamlyLayout.exe");
#else
    return QStringLiteral("SeamlyLayout");
#endif
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief locateSeamlyLayout looks for the SeamlyLayout executable in the two
 * layouts an installation can use, relative to one install directory.
 *
 * Lookup order:
 *  1. Flat: `<directory>/SeamlyLayout(.exe)` — the layout used when every app
 *     shares one Qt runtime (e.g. the Linux Flatpak's `/app/bin`, or a local
 *     development tree where all apps are built against the same Qt).
 *  2. Subdirectory: `<directory>/SeamlyLayout/SeamlyLayout(.exe)` — the layout
 *     the Windows MSI installer uses (Task 13). SeamlyLayout is built against
 *     a different Qt release than seamly2d/seamlyme, so its Qt runtime DLLs
 *     cannot share a flat directory with the parent apps' DLLs (identical file
 *     names, different versions); the installer therefore gives it its own
 *     subdirectory carrying its own Qt runtime.
 *
 * Both candidates must be existing regular files — a *directory* that happens
 * to be named like the executable (e.g. the "SeamlyLayout" subdirectory itself
 * on non-Windows platforms, where the exe name has no ".exe" suffix) is never
 * a match.
 *
 * @param directory absolute path of the directory to search (typically the
 *        directory of the running seamly2d executable).
 * @return absolute path of the SeamlyLayout executable, or an empty string
 *         when neither layout contains it.
 */
QString locateSeamlyLayout(const QString &directory)
{
    const QString exeName = seamlyLayoutExeName();

    // Layout 1 — flat: the executable directly beside the caller's apps.
    const QFileInfo flat(directory + QLatin1Char('/') + exeName);
    if (flat.exists() && flat.isFile())
    {
        return flat.absoluteFilePath();
    }

    // Layout 2 — MSI subdirectory: SeamlyLayout isolated with its own Qt runtime.
    const QFileInfo nested(directory + QLatin1String("/SeamlyLayout/") + exeName);
    if (nested.exists() && nested.isFile())
    {
        return nested.absoluteFilePath();
    }

    return QString(); // Neither layout present; the caller decides what to do.
}

} // namespace SeamlyFamilyPaths
