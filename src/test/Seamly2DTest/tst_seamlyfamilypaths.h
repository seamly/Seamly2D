/******************************************************************************
 **  @file   tst_seamlyfamilypaths.h
 **  @author slspencer
 **  @date   July 22, 2026
 **
 **  @brief
 **  Unit tests for the SeamlyFamilyPaths install-directory lookup helpers
 **  (flat layout vs the Windows MSI "SeamlyLayout" subdirectory layout).
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

#ifndef TST_SEAMLYFAMILYPATHS_H
#define TST_SEAMLYFAMILYPATHS_H

#include <QObject>

/**
 * @brief TST_SeamlyFamilyPaths tests the two lookups seamly2d uses to find the
 * SeamlyLayout executable.
 *
 * SeamlyFamilyPaths::locateSeamlyLayout() is the install-directory lookup: the
 * executable flat beside the caller's apps, or inside the "SeamlyLayout"
 * subdirectory created by the Windows MSI installer (Task 13).
 *
 * SeamlyFamilyPaths::locateSeamlyLayoutDevBuild() is the development fallback
 * (Task 50): an upward walk from the running executable's directory looking for
 * a SeamlyLayout build inside the source checkout, Release before Debug.
 *
 * SeamlyFamilyPaths::piecesSvgFilePath() and seamlyLayoutLaunchArguments() are
 * the seamly2d half of the Layout Mode launch contract (Task 49): the handoff
 * file is "<pattern>.pieces.svg" beside the pattern, and it is passed as the
 * single positional argument of the SeamlyLayout process. The daughter app's
 * half — that it accepts exactly that and rejects anything else — is locked by
 * StartupOptionsTests in src/test/SeamlyLayoutTest.
 *
 * Every case runs against a QTemporaryDir populated with dummy files, so the
 * suite is hermetic — no real installation, settings value, or application
 * directory is involved.
 */
class TST_SeamlyFamilyPaths : public QObject
{
    Q_OBJECT
public:
    explicit TST_SeamlyFamilyPaths(QObject *parent = nullptr);

private slots:
    void EmptyDirectoryFindsNothing() const;
    void FindsFlatExecutable() const;
    void FindsSubdirectoryExecutable() const;
    void FlatLayoutTakesPrecedence() const;
    void DirectoryNamedLikeExecutableIsIgnored() const;

    void DevBuildEmptyStartDirectoryFindsNothing() const;
    void DevBuildNoCheckoutFindsNothing() const;
    void DevBuildFoundFromReleaseShadowBuild() const;
    void DevBuildFoundFromDebugShadowBuild() const;
    void DevBuildReleaseTakesPrecedenceOverDebug() const;
    void DevBuildFindsDebugWhenReleaseAbsent() const;
    void DevBuildDirectoryNamedLikeExecutableIsIgnored() const;
    void DevBuildStopsBeforeUnboundedWalk() const;

    void PiecesSvgSitsBesideThePattern() const;
    void PiecesSvgKeepsDotsInThePatternName() const;
    void PiecesSvgPathIsAbsolute() const;
    void PiecesSvgOfEmptyPatternPathIsEmpty() const;
    void LaunchArgumentsAreTheSvgPathAlone() const;
    void LaunchArgumentsOfEmptySvgPathAreEmpty() const;
};

#endif // TST_SEAMLYFAMILYPATHS_H
