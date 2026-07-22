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
 * @brief TST_SeamlyFamilyPaths tests SeamlyFamilyPaths::locateSeamlyLayout(),
 * the install-directory lookup seamly2d uses to find the SeamlyLayout
 * executable: flat beside the caller's apps, or inside the "SeamlyLayout"
 * subdirectory created by the Windows MSI installer (Task 13).
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
};

#endif // TST_SEAMLYFAMILYPATHS_H
