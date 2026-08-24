/******************************************************************************
 **  @file   legacy_data_archive.h
 **  @author slspencer
 **  @date   August 24, 2026
 **
 **  @brief
 **  Archives a migrated legacy data tree into one .zip backup.
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

#ifndef LEGACY_DATA_ARCHIVE_H
#define LEGACY_DATA_ARCHIVE_H

#include <QString>

class QDateTime;

/**
 * @brief LegacyDataArchive packs the old ~/seamly2d tree into one backup .zip.
 *
 * Runs after VCommonSettings::migrateAdoptedLegacyTree() has copied and verified the tree
 * into the new data root. The user then has their work in the new root, one .zip beside it
 * holding exactly what the old folder held, and the old folder itself unchanged — this
 * module never removes the tree it archives; that stays VCommonSettings's own
 * MIGRATED-TO-SEAMLY.txt-marker rollback path.
 *
 * Every function takes its paths as arguments and reads no settings, so the unit tests can
 * exercise the whole sequence — archive, verify — against a QTemporaryDir.
 */
namespace LegacyDataArchive
{
    QString archivePath(const QString &destinationRoot, const QDateTime &when);
    bool    create(const QString &sourceRoot, const QString &archiveFile, QString *errorMessage = nullptr);
    bool    verifyAgainst(const QString &sourceRoot, const QString &archiveFile, QString *errorMessage = nullptr);
    QString archive(const QString &sourceRoot, const QString &destinationRoot, QString *errorMessage = nullptr);
}

#endif // LEGACY_DATA_ARCHIVE_H
