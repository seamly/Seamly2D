/******************************************************************************
 **  @file   legacy_data_migration.h
 **  @author slspencer
 **  @date   August 24, 2026
 **
 **  @brief
 **  Runs the first-run move out of ~/seamly2d, and tells the user it happened.
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

#ifndef LEGACY_DATA_MIGRATION_H
#define LEGACY_DATA_MIGRATION_H

#include <QString>

/**
 * @brief LegacyDataMigration is the first run after an upgrade, from the user's side.
 *
 * Two steps that the user sees as one: copy the old ~/seamly2d tree into the new data root,
 * then pack the old tree into one .zip beside it as a second, portable backup. The old tree
 * is left in place either way — VCommonSettings::migrateAdoptedLegacyTree() already marks it
 * with MIGRATED-TO-SEAMLY.txt so a rollback stays possible, and this module adds a backup on
 * top of that rather than replacing it. A splash screen stays up throughout, because copying
 * and hashing a large tree takes minutes and an app that appears to hang on first launch
 * looks broken.
 *
 * Seamly2D and SeamlyMe both call this, from openSettings(), so the work happens once
 * whichever app the user starts first. It is called ONLY from there — the one place the real
 * home directory reaches this code — which is what stops the unit tests writing into a
 * developer's home. The pieces it drives, VCommonSettings::migrateAdoptedLegacyTree() and
 * LegacyDataArchive, take their paths as arguments and are tested directly.
 */
namespace LegacyDataMigration
{
    QString run(const QString &legacyRoot, const QString &newRoot);
}

#endif // LEGACY_DATA_MIGRATION_H
