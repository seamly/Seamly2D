/******************************************************************************
 **  @file   tst_seamlyfamilypaths.cpp
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
 * @brief FindsSubdirectoryExecutable verifies the Windows MSI layout is
 * found: the executable inside a "SeamlyLayout" subdirectory of the install
 * directory (where it carries its own Qt runtime, Task 13).
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
 */
void TST_SeamlyFamilyPaths::FlatLayoutTakesPrecedence() const
{
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
