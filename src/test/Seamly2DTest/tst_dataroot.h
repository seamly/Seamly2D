/******************************************************************************
 **  @file   tst_dataroot.h
 **  @author slspencer
 **  @date   July 26, 2026
 **
 **  @brief
 **  Unit tests for the relocatable user-data root (Task 34): its default and
 **  legacy locations, the derivation of every data subfolder from it, first-run
 **  resolution of an existing ~/seamly2d tree, and the Preferences rebase rule.
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

#ifndef TST_DATAROOT_H
#define TST_DATAROOT_H

#include <QObject>
#include <QScopedPointer>
#include <QString>

class QTemporaryDir;

/**
 * @brief TST_DataRoot tests VCommonSettings' user-data root (Task 34) — the single
 * settings-backed root that patterns, measurements, templates, bodyscans, label
 * templates, images, backups and layouts are all derived from.
 *
 * The suite is hermetic, and deliberately so: every case works inside a QTemporaryDir or
 * on plain strings, and initTestCase() additionally redirects QSettings' IniFormat/
 * UserScope base at a temporary directory so the developer's real settings file is never
 * read or written. That redirection is undone in cleanupTestCase().
 *
 * No test may touch a path under QDir::homePath(). The home directory CANNOT be faked on
 * Windows — QFileSystemEngine::homePath() asks the OS through GetUserProfileDirectory()
 * and only falls back to the USERPROFILE/HOME environment variables when that fails — so
 * a test that created or removed ~/seamly or ~/seamly2d would be operating on the real
 * user's data. First-run resolution is therefore tested through
 * VCommonSettings::chooseFirstRunDataRoot(), which takes both candidate roots as
 * arguments and can be pointed at throwaway directories.
 */
class TST_DataRoot : public QObject
{
    Q_OBJECT
public:
    explicit TST_DataRoot(QObject *parent = nullptr);
    ~TST_DataRoot() override;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void DefaultDataRootIsSeamlyUnderDocuments() const;
    void LegacyDataRootIsTheOldSeamly2dFolder() const;
    void UnconfiguredRootFallsBackToTheDefault() const;
    void EveryDefaultPathDerivesFromTheDataRoot() const;
    void DataRootAcceptsAnyDriveOrPath() const;
    void FirstRunWithoutLegacyTreeUsesTheDefault() const;
    void FirstRunAdoptsAnExistingLegacyTree() const;
    void FirstRunPrefersAnExistingNewRoot() const;
    void AdoptionNeverRemovesTheLegacyTree() const;
    void AConfiguredRootIsNeverOverwritten() const;
    void InstallerDataRootIsCleanOrEmpty() const;
    void AConfiguredRootOutranksTheInstaller() const;
    void EnsureDataRootTreeCreatesTheSubfolders() const;
    void EnsureDataRootTreeKeepsExistingFiles() const;
    void StartupResolvesThenSeedsTheConfiguredRoot() const;

    void SeedSamplePatternsCopiesBundledFiles() const;
    void SeedSamplePatternsNeverOverwritesAnExistingFile() const;
    void SeedSamplePatternsIsANoOpWhenSourceIsMissing() const;

    void SeedSampleMeasurementsCopiesBundledFiles() const;
    void SeedSampleMeasurementsNeverOverwritesAnExistingFile() const;
    void SeedSampleMeasurementsIsANoOpWhenSourceIsMissing() const;
    void SeedSampleMeasurementsHonoursTheNameFilter() const;

    void MigrationCopiesTheWholeTreeIncludingUnknownFolders() const;
    void MigrationNeverOverwritesAnExistingFile() const;
    void MigrationLeavesTheSourceTreeIntact() const;
    void MigrationRefusesADestinationInsideTheSource() const;
    void MigrationMarksTheLegacyTree() const;
    void RebaseMovesPathsInsideTheOldRoot() const;
    void RebaseLeavesPathsOutsideTheOldRootAlone() const;

    void PruneRemovesAnEmptyLegacyTree() const;
    void PruneKeepsALegacyTreeHoldingFiles() const;
    void PruneNeverRemovesTheConfiguredRoot() const;
    void PruneKeepsALegacyRootHoldingTheConfiguredRoot() const;
    void PruneIgnoresAMissingLegacyRoot() const;
    void StrayCommonSettingsAreMergedThenDeleted() const;
    void PerAppPathsPersistToTheOwnSettingsFile() const;

    void ArchiveHoldsEveryFileAndFolder() const;
    void ArchiveVerifiesAgainstTheTreeItCameFrom() const;
    void ArchiveVerificationCatchesAMissingFile() const;
    void ArchiveVerificationCatchesAlteredContents() const;
    void ArchiveNamesDoNotCollide() const;
    void ArchiveRefusesATreeHoldingASymbolicLink() const;
    void ArchiveRefusesADestinationInsideTheSource() const;
    void ArchiveLeavesTheSourceTreeInPlace() const;

private:
    /** Scratch directory holding every root, tree and file the suite creates. */
    QScopedPointer<QTemporaryDir> m_scratch;
    /** Fake QSettings base directory, so the real qt6_common.ini is never touched. */
    QScopedPointer<QTemporaryDir> m_settings;

    QString m_originalSettingsBase;

    QString scratchPath(const QString &relative) const;
    void    writeDataRoot(const QString &root) const;
    void    clearDataRoot() const;
};

#endif // TST_DATAROOT_H
