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
 *     shares one Qt runtime. Since Task 30 that is every supported install:
 *     the Windows MSI, the Linux Flatpak's `/app/bin`, and a local development
 *     tree, because all three apps now build against the same Qt release.
 *  2. Subdirectory: `<directory>/SeamlyLayout/SeamlyLayout(.exe)` — the layout
 *     the Windows MSI used before Task 30 (Task 13). Back then SeamlyLayout was
 *     built against a different Qt release than seamly2d/seamlyme, so its Qt
 *     runtime DLLs could not share a flat directory with the parent apps'
 *     (identical file names, different versions) and the installer gave it its
 *     own subdirectory with its own Qt runtime. No current installer produces
 *     this layout; the branch is kept so a seamly2d upgraded in place over such
 *     an install — or any future packaging that isolates the daughter app —
 *     still finds the executable.
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
