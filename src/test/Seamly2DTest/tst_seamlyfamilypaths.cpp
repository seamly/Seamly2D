/******************************************************************************
 **  @file   tst_seamlyfamilypaths.cpp
 **  @author slspencer
 **  @date   July 22, 2026
 **
 **  @brief
 **  Unit tests for the SeamlyFamilyPaths install-directory lookup helpers
 **  (the flat layout every current installer produces, vs the legacy
 **  "SeamlyLayout" subdirectory layout the pre-Task-30 Windows MSI used).
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

#include "tst_seamlyfamilypaths.h"

#include "../vmisc/seamly_family_paths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createDummyFile creates an empty placeholder file (parent directories
 * included) standing in for an executable in the lookup tests.
 *
 * The lookup only checks existence and file-ness, never content or the
 * executable bit, so an empty file is a faithful stand-in on every platform.
 *
 * @param filePath absolute path of the file to create.
 * @return true when the file (and any missing parent directory) was created.
 */
bool createDummyFile(const QString &filePath)
{
    // QFile::open() does not create missing parent directories - mirror the
    // real installer layout by creating them first.
    if (!QDir().mkpath(QFileInfo(filePath).absolutePath()))
    {
        return false;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    file.close();
    return true;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief TST_SeamlyFamilyPaths constructor, forwards to QObject.
 * @param parent optional QObject parent.
 */
TST_SeamlyFamilyPaths::TST_SeamlyFamilyPaths(QObject *parent)
    : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief EmptyDirectoryFindsNothing verifies the lookup returns an empty
 * string for a directory containing neither layout.
 */
void TST_SeamlyFamilyPaths::EmptyDirectoryFindsNothing() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QVERIFY(SeamlyFamilyPaths::locateSeamlyLayout(dir.path()).isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FindsFlatExecutable verifies the flat layout is found: the
 * executable directly inside the install directory.
 */
void TST_SeamlyFamilyPaths::FindsFlatExecutable() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString flatExe = dir.path() + QLatin1Char('/')
                          + SeamlyFamilyPaths::seamlyLayoutExeName();
    QVERIFY(createDummyFile(flatExe));

    const QString found = SeamlyFamilyPaths::locateSeamlyLayout(dir.path());
    QCOMPARE(found, QFileInfo(flatExe).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FindsSubdirectoryExecutable verifies the legacy pre-Task-30 Windows
 * MSI layout is still found: the executable inside a "SeamlyLayout"
 * subdirectory of the install directory, where it carried its own Qt runtime
 * (Task 13). No current installer produces this layout, but an install made by
 * an older MSI can still be on disk, so the fallback must keep working.
 */
void TST_SeamlyFamilyPaths::FindsSubdirectoryExecutable() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString nestedExe = dir.path() + QLatin1String("/SeamlyLayout/")
                            + SeamlyFamilyPaths::seamlyLayoutExeName();
    QVERIFY(createDummyFile(nestedExe));

    const QString found = SeamlyFamilyPaths::locateSeamlyLayout(dir.path());
    QCOMPARE(found, QFileInfo(nestedExe).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FlatLayoutTakesPrecedence verifies that when both layouts are
 * present the flat executable wins — it is the one sharing the caller's
 * runtime, so it must shadow a leftover subdirectory install.
 *
 * This scenario is only physically constructible on Windows: there the flat
 * executable carries the ".exe" suffix (`SeamlyLayout.exe`) that distinguishes
 * it from the MSI subdirectory (`SeamlyLayout\`), so a flat exe file and a
 * `SeamlyLayout` subdirectory can share one parent. On every other platform
 * the executable name has no suffix, so the flat candidate (`SeamlyLayout`, a
 * file) and the subdirectory (`SeamlyLayout`, a directory) have identical names
 * and cannot coexist in one directory — the two layouts are mutually exclusive,
 * so precedence never arises. That mutually-exclusive non-Windows case is
 * covered instead by DirectoryNamedLikeExecutableIsIgnored().
 */
void TST_SeamlyFamilyPaths::FlatLayoutTakesPrecedence() const
{
#ifndef Q_OS_WIN
    QSKIP("Both layouts can coexist only on Windows (the flat exe's \".exe\" "
          "suffix distinguishes it from the \"SeamlyLayout\" subdirectory); "
          "elsewhere they are mutually exclusive — see "
          "DirectoryNamedLikeExecutableIsIgnored().");
#else
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString flatExe = dir.path() + QLatin1Char('/')
                          + SeamlyFamilyPaths::seamlyLayoutExeName();
    const QString nestedExe = dir.path() + QLatin1String("/SeamlyLayout/")
                            + SeamlyFamilyPaths::seamlyLayoutExeName();
    QVERIFY(createDummyFile(flatExe));
    QVERIFY(createDummyFile(nestedExe));

    const QString found = SeamlyFamilyPaths::locateSeamlyLayout(dir.path());
    QCOMPARE(found, QFileInfo(flatExe).absoluteFilePath());
#endif
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DirectoryNamedLikeExecutableIsIgnored verifies a *directory* named
 * like the executable never counts as a match. On non-Windows platforms the
 * executable name has no ".exe" suffix, so the MSI-style "SeamlyLayout"
 * subdirectory itself collides with the flat candidate's name — the isFile()
 * guard must reject it and let the subdirectory lookup succeed instead.
 */
void TST_SeamlyFamilyPaths::DirectoryNamedLikeExecutableIsIgnored() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // A directory whose name matches the flat executable candidate exactly.
    QVERIFY(QDir(dir.path()).mkpath(SeamlyFamilyPaths::seamlyLayoutExeName()));

    // With nothing else present the lookup must find nothing...
    QVERIFY(SeamlyFamilyPaths::locateSeamlyLayout(dir.path()).isEmpty());

    // ...and with the real executable inside the MSI-style subdirectory, the
    // lookup must skip the impostor directory and return the nested file.
    const QString nestedExe = dir.path() + QLatin1String("/SeamlyLayout/")
                            + SeamlyFamilyPaths::seamlyLayoutExeName();
    QVERIFY(createDummyFile(nestedExe));
    QCOMPARE(SeamlyFamilyPaths::locateSeamlyLayout(dir.path()),
             QFileInfo(nestedExe).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DevBuildEmptyStartDirectoryFindsNothing verifies an empty start
 * directory is rejected outright rather than resolved against the process's
 * current working directory, which would make the lookup depend on where the
 * test runner happened to be launched from.
 */
void TST_SeamlyFamilyPaths::DevBuildEmptyStartDirectoryFindsNothing() const
{
    QVERIFY(SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(QString()).isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DevBuildNoCheckoutFindsNothing verifies the walk returns empty when no
 * ancestor of the start directory is a source checkout containing a build.
 *
 * This is the end user's case: the development fallback must stay inert on a
 * machine that has only an installed application.
 */
void TST_SeamlyFamilyPaths::DevBuildNoCheckoutFindsNothing() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // A plausible install layout, but nothing resembling a source checkout above it.
    const QString installBin = dir.path() + QLatin1String("/Seamly/bin");
    QVERIFY(QDir().mkpath(installBin));

    QVERIFY(SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(installBin).isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DevBuildFoundFromReleaseShadowBuild verifies the walk reaches the
 * checkout root from the release shadow build's bin directory.
 *
 * Layout reproduced: seamly2d runs from `<checkout>/build/src/app/seamly2d/bin`,
 * five levels below the checkout root that holds the SeamlyLayout build.
 */
void TST_SeamlyFamilyPaths::DevBuildFoundFromReleaseShadowBuild() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString checkout = dir.path();
    const QString seamly2dBin = checkout + QLatin1String("/build/src/app/seamly2d/bin");
    QVERIFY(QDir().mkpath(seamly2dBin));

    const QString layoutExe = checkout
                            + QLatin1String("/src/app/seamlylayout/qt_frontend/build/Release/")
                            + SeamlyFamilyPaths::seamlyLayoutExeName();
    QVERIFY(createDummyFile(layoutExe));

    QCOMPARE(SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(seamly2dBin),
             QFileInfo(layoutExe).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DevBuildFoundFromDebugShadowBuild verifies the walk also reaches the
 * checkout root from the deeper debug shadow build layout.
 *
 * Layout reproduced: seamly2d runs from
 * `<checkout>/scripts/seamly2d-debug/src/app/seamly2d/bin` — six levels
 * below the checkout root, the deepest layout the project uses.
 */
void TST_SeamlyFamilyPaths::DevBuildFoundFromDebugShadowBuild() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString checkout = dir.path();
    const QString seamly2dBin =
        checkout + QLatin1String("/scripts/seamly2d-debug/src/app/seamly2d/bin");
    QVERIFY(QDir().mkpath(seamly2dBin));

    const QString layoutExe = checkout
                            + QLatin1String("/src/app/seamlylayout/qt_frontend/build/Debug/")
                            + SeamlyFamilyPaths::seamlyLayoutExeName();
    QVERIFY(createDummyFile(layoutExe));

    QCOMPARE(SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(seamly2dBin),
             QFileInfo(layoutExe).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DevBuildReleaseTakesPrecedenceOverDebug verifies that when a developer
 * has built both configurations, Release wins.
 *
 * The hard-coded path this lookup replaced (Task 50) named the Debug build
 * unconditionally, so it could hand off a build arbitrarily older than the
 * Release binary sitting beside it.
 */
void TST_SeamlyFamilyPaths::DevBuildReleaseTakesPrecedenceOverDebug() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString checkout = dir.path();
    const QString seamly2dBin = checkout + QLatin1String("/build/src/app/seamly2d/bin");
    QVERIFY(QDir().mkpath(seamly2dBin));

    const QString buildRoot = checkout + QLatin1String("/src/app/seamlylayout/qt_frontend/build/");
    const QString releaseExe =
        buildRoot + QLatin1String("Release/") + SeamlyFamilyPaths::seamlyLayoutExeName();
    const QString debugExe =
        buildRoot + QLatin1String("Debug/") + SeamlyFamilyPaths::seamlyLayoutExeName();
    QVERIFY(createDummyFile(releaseExe));
    QVERIFY(createDummyFile(debugExe));

    QCOMPARE(SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(seamly2dBin),
             QFileInfo(releaseExe).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DevBuildFindsDebugWhenReleaseAbsent verifies Debug is still found when
 * it is the only configuration built — the common case for a developer working
 * from a debug tree.
 */
void TST_SeamlyFamilyPaths::DevBuildFindsDebugWhenReleaseAbsent() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString checkout = dir.path();
    const QString seamly2dBin = checkout + QLatin1String("/build/src/app/seamly2d/bin");
    QVERIFY(QDir().mkpath(seamly2dBin));

    const QString debugExe = checkout
                           + QLatin1String("/src/app/seamlylayout/qt_frontend/build/Debug/")
                           + SeamlyFamilyPaths::seamlyLayoutExeName();
    QVERIFY(createDummyFile(debugExe));

    QCOMPARE(SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(seamly2dBin),
             QFileInfo(debugExe).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DevBuildDirectoryNamedLikeExecutableIsIgnored verifies the isFile()
 * guard applies to the development lookup too: a *directory* at the executable's
 * path is never a match, and the walk goes on to find the real Debug build.
 *
 * On non-Windows platforms the executable name carries no ".exe" suffix, which
 * makes this collision easy to create by accident.
 */
void TST_SeamlyFamilyPaths::DevBuildDirectoryNamedLikeExecutableIsIgnored() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString checkout = dir.path();
    const QString seamly2dBin = checkout + QLatin1String("/build/src/app/seamly2d/bin");
    QVERIFY(QDir().mkpath(seamly2dBin));

    const QString buildRoot = checkout + QLatin1String("/src/app/seamlylayout/qt_frontend/build/");

    // A directory exactly where the Release executable would be.
    QVERIFY(QDir().mkpath(buildRoot + QLatin1String("Release/")
                          + SeamlyFamilyPaths::seamlyLayoutExeName()));

    // With only the impostor directory present, nothing is found.
    QVERIFY(SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(seamly2dBin).isEmpty());

    // The real Debug build must then win over the impostor Release directory.
    const QString debugExe =
        buildRoot + QLatin1String("Debug/") + SeamlyFamilyPaths::seamlyLayoutExeName();
    QVERIFY(createDummyFile(debugExe));

    QCOMPARE(SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(seamly2dBin),
             QFileInfo(debugExe).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DevBuildStopsBeforeUnboundedWalk verifies the walk is bounded: a
 * checkout further above the start directory than the depth limit is not found.
 *
 * Without the bound the lookup would keep climbing to the filesystem root,
 * probing directories that have nothing to do with the application.
 */
void TST_SeamlyFamilyPaths::DevBuildStopsBeforeUnboundedWalk() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString checkout = dir.path();

    // Ten levels below the checkout root — beyond the eight-level limit.
    const QString deepDirectory =
        checkout + QLatin1String("/a/b/c/d/e/f/g/h/i/j");
    QVERIFY(QDir().mkpath(deepDirectory));

    const QString layoutExe = checkout
                            + QLatin1String("/src/app/seamlylayout/qt_frontend/build/Release/")
                            + SeamlyFamilyPaths::seamlyLayoutExeName();
    QVERIFY(createDummyFile(layoutExe));

    QVERIFY(SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(deepDirectory).isEmpty());

    // Sanity check that the same tree *is* found from within the limit, so the
    // case above fails for the depth bound and not for a broken fixture.
    const QString shallowDirectory = checkout + QLatin1String("/a/b/c");
    QCOMPARE(SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(shallowDirectory),
             QFileInfo(layoutExe).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PiecesSvgSitsBesideThePattern verifies the handoff SVG is named after
 * the pattern and written into the same directory.
 *
 * This is the file name SeamlyLayout is launched with, so it is part of the
 * two-app contract rather than an implementation detail of Layout Mode.
 */
void TST_SeamlyFamilyPaths::PiecesSvgSitsBesideThePattern() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString patternFile = dir.path() + QLatin1String("/richmond-shirt.sm2d");
    const QString expected    = dir.path() + QLatin1String("/richmond-shirt.pieces.svg");

    QCOMPARE(SeamlyFamilyPaths::piecesSvgFilePath(patternFile),
             QFileInfo(expected).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PiecesSvgKeepsDotsInThePatternName verifies only the final extension is
 * replaced.
 *
 * completeBaseName() keeps everything up to the last dot, so a pattern named
 * "shirt.v2.sm2d" keeps its version segment. baseName() would have produced
 * "shirt.pieces.svg" and quietly collided with a different pattern's handoff.
 */
void TST_SeamlyFamilyPaths::PiecesSvgKeepsDotsInThePatternName() const
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString patternFile = dir.path() + QLatin1String("/shirt.v2.sm2d");
    const QString expected    = dir.path() + QLatin1String("/shirt.v2.pieces.svg");

    QCOMPARE(SeamlyFamilyPaths::piecesSvgFilePath(patternFile),
             QFileInfo(expected).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PiecesSvgPathIsAbsolute verifies a relative pattern path still produces
 * an absolute handoff path.
 *
 * SeamlyLayout is started detached with its own working directory, so a relative
 * argument would resolve against the wrong directory in the daughter app.
 */
void TST_SeamlyFamilyPaths::PiecesSvgPathIsAbsolute() const
{
    const QString svgPath = SeamlyFamilyPaths::piecesSvgFilePath(QStringLiteral("relative.sm2d"));

    QVERIFY(!svgPath.isEmpty());
    QVERIFY(QFileInfo(svgPath).isAbsolute());
    QVERIFY(svgPath.endsWith(QLatin1String("/relative.pieces.svg")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PiecesSvgOfEmptyPatternPathIsEmpty verifies an unsaved pattern yields
 * no path, so Layout Mode can ask the user to save instead of writing a file
 * named after nothing.
 */
void TST_SeamlyFamilyPaths::PiecesSvgOfEmptyPatternPathIsEmpty() const
{
    QVERIFY(SeamlyFamilyPaths::piecesSvgFilePath(QString()).isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LaunchArgumentsAreTheSvgPathAlone pins the launch contract: SeamlyLayout
 * is started with exactly one positional argument, the SVG path.
 *
 * Its StartupOptions parser rejects a second positional argument, so a change
 * here without the matching change there breaks the handoff — this case is what
 * makes that break visible in CI rather than in a user's Layout Mode.
 */
void TST_SeamlyFamilyPaths::LaunchArgumentsAreTheSvgPathAlone() const
{
    const QString svgPath = QStringLiteral("/patterns/richmond shirt.pieces.svg");

    const QStringList arguments = SeamlyFamilyPaths::seamlyLayoutLaunchArguments(svgPath);

    QCOMPARE(arguments.size(), 1);
    // Passed unquoted and unsplit: QProcess quotes list elements itself, which
    // is what keeps a directory containing spaces working.
    QCOMPARE(arguments.first(), svgPath);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LaunchArgumentsOfEmptySvgPathAreEmpty verifies no argument list is built
 * for an empty path — launching SeamlyLayout bare would open an empty canvas,
 * which is exactly the Task 49 defect this contract exists to prevent.
 */
void TST_SeamlyFamilyPaths::LaunchArgumentsOfEmptySvgPathAreEmpty() const
{
    QVERIFY(SeamlyFamilyPaths::seamlyLayoutLaunchArguments(QString()).isEmpty());
}
