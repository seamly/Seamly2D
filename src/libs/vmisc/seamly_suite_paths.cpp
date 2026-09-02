/******************************************************************************
 **  @file   seamly_suite_paths.cpp
 **  @author slspencer
 **  @date   July 22, 2026
 **
 **  @brief
 **  Helpers for locating the executables of the Seamly app suite
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

#include "seamly_suite_paths.h"

#include <QDir>
#include <QFileInfo>
#include <QLatin1Char>
#include <QLatin1String>
#include <QStringList>

namespace
{
/**
 * @brief sourceTreeBuildSubPath is the path of a SeamlyLayout development build
 * relative to the root of a Seamly2D source checkout, without the build
 * configuration directory.
 *
 * The Qt frontend's CMake build writes its executable to
 * `<checkout>/src/app/seamlylayout/qt_frontend/build/<config>/SeamlyLayout(.exe)`.
 */
const QLatin1String sourceTreeBuildSubPath("/src/app/seamlylayout/qt_frontend/build/");

/**
 * @brief maxUpwardLevels bounds how far the development-build lookup walks up
 * from the running executable's directory while searching for the checkout root.
 *
 * The deepest layout in use is the debug shadow build, whose executable sits at
 * `<checkout>/scripts/seamly2d-debug/src/app/seamly2d/bin/` — six levels
 * below the checkout root. The release shadow build (`<checkout>/build/...`) is
 * five. Eight leaves room for a differently nested build tree while still
 * terminating quickly, and keeps the walk from climbing out of the checkout and
 * probing unrelated parts of the filesystem.
 */
const int maxUpwardLevels = 8;

} // namespace

namespace SeamlySuitePaths
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

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief locateSeamlyLayoutDevBuild looks for a SeamlyLayout executable built
 * from the same source checkout as the running application.
 *
 * This is the development convenience that lets a locally built seamly2d hand
 * off to a locally built SeamlyLayout with no configuration, on any machine.
 * It replaces the single hard-coded developer path this function grew out of
 * (Task 50), which named one contributor's checkout and therefore helped
 * exactly one machine.
 *
 * The checkout root is derived from the caller's directory rather than assumed:
 * the walk starts at @p startDirectory and climbs up to ::maxUpwardLevels
 * parents, treating each as a candidate checkout root and testing whether it
 * contains a SeamlyLayout build. Both shadow-build layouts the project uses
 * resolve this way — the release build at `<checkout>/build/...` (five levels)
 * and a debug shadow build at `<checkout>/scripts/seamly2d-debug/...` (six) —
 * without either being named here, so a differently nested build tree still
 * works.
 *
 * At each level **Release is preferred over Debug**: a developer who has built
 * both almost always wants the current release binary, and the old hard-coded
 * path silently pinned Debug, which could be arbitrarily stale.
 *
 * This lookup is deliberately the *last* resort in Application2D's chain — a
 * configured setting and an installed copy both outrank it — so a source tree
 * that happens to sit above an installed application can never shadow the
 * installation. Because it only ever matches inside a checkout that has been
 * built, it is inert on an end user's machine.
 *
 * @param startDirectory absolute path to start the upward walk from (typically
 *        the directory of the running executable). Passed in rather than read
 *        from QCoreApplication so the tests can point it at a QTemporaryDir.
 * @return absolute path of the SeamlyLayout development build, or an empty
 *         string when no checkout above @p startDirectory contains one.
 */
QString locateSeamlyLayoutDevBuild(const QString &startDirectory)
{
    if (startDirectory.isEmpty())
    {
        return QString();
    }

    const QString exeName = seamlyLayoutExeName();

    // Release first, so a stale Debug build never wins over a current Release one.
    const QStringList configurations{QLatin1String("Release"), QLatin1String("Debug")};

    QDir directory(startDirectory);

    // Climb one level per iteration, testing each ancestor as a checkout root.
    for (int level = 0; level <= maxUpwardLevels; ++level)
    {
        const QString candidateRoot = directory.absolutePath();

        for (const QString &configuration : configurations)
        {
            const QFileInfo candidate(candidateRoot + sourceTreeBuildSubPath + configuration
                                      + QLatin1Char('/') + exeName);
            // Must be an existing regular file — a directory of the same name is not a match.
            if (candidate.exists() && candidate.isFile())
            {
                return candidate.absoluteFilePath();
            }
        }

        // cdUp() fails at the filesystem root, which ends the walk early.
        if (!directory.cdUp())
        {
            break;
        }
    }

    return QString(); // No development build above the caller; not a source checkout.
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief patternDocumentName returns the name Layout Mode gives the piece-mode
 * document it hands to SeamlyLayout.
 *
 * The handoff carries no file, so SeamlyLayout has no file name to build its
 * default export names from. This is that name: the pattern's complete base
 * name, so "shirt.v2.sm2d" hands over "shirt.v2" and a later SVG export in
 * SeamlyLayout still defaults to a name the user recognises.
 *
 * completeBaseName() is used deliberately — it keeps everything up to the *last*
 * dot, so a version segment in the pattern name survives.
 *
 * @param patternFilePath path of the open pattern file; may be relative.
 * @return the pattern's complete base name, or an empty string when
 *         @p patternFilePath is empty (no pattern has been saved yet).
 */
QString patternDocumentName(const QString &patternFilePath)
{
    if (patternFilePath.isEmpty())
    {
        return QString(); // Unsaved pattern; SeamlyLayout falls back to its own default.
    }

    return QFileInfo(patternFilePath).completeBaseName();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief seamlyLayoutLaunchArguments builds the argument vector seamly2d passes
 * to the SeamlyLayout executable.
 *
 * This is the seamly2d side of the two-app launch contract. Layout Mode does not
 * write a handoff file: it serialises piece mode to one stringified SVG and
 * sends that document on the child process's standard input (Seamly2D.5). The
 * arguments only say how to read it:
 *
 *     SeamlyLayout --svg-stdin --document-name <pattern base name>
 *
 * SeamlyLayout's `StartupOptions` class
 * (`src/app/seamlylayout/qt_frontend/src/StartupOptions.cpp`) parses exactly
 * these two options, and rejects a positional file alongside them; both halves
 * are pinned by tests. This function must never grow an argument silently.
 *
 * --document-name is omitted for an unsaved pattern rather than passed empty, so
 * SeamlyLayout keeps its own default export name instead of an empty one.
 *
 * The name is passed as its own list element rather than embedded in a command
 * string, so QProcess quotes it and a pattern name containing spaces needs no
 * special handling.
 *
 * @param documentName pattern base name, as returned by patternDocumentName();
 *        may be empty.
 * @return the argument list for QProcess::start().
 */
QStringList seamlyLayoutLaunchArguments(const QString &documentName)
{
    QStringList arguments(QStringLiteral("--svg-stdin"));

    if (!documentName.isEmpty())
    {
        arguments << QStringLiteral("--document-name") << documentName;
    }

    return arguments;
}

} // namespace SeamlySuitePaths
