/******************************************************************************
 **  @file   tst_dataroot.cpp
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

#include "tst_dataroot.h"

#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace
{
/** Settings key holding the user-data root; must match vcommonsettings.cpp. */
const QString dataRootKey = QStringLiteral("paths/dataRoot");
/** Shared, cross-application settings file; must match vcommonsettings.cpp. */
const QString commonIniName = QStringLiteral("qt6_common");
} // anonymous namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief TST_DataRoot constructor, forwards to QObject.
 * @param parent parent object.
 */
TST_DataRoot::TST_DataRoot(QObject *parent)
    : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
TST_DataRoot::~TST_DataRoot() = default;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief initTestCase creates the scratch directory every case works inside, and redirects
 * the QSettings base directory so the developer's real settings file is never touched.
 *
 * The home directory is deliberately NOT redirected: it cannot be, on Windows. Tests must
 * therefore never build a path from QDir::homePath() — see the class documentation.
 */
void TST_DataRoot::initTestCase()
{
    m_scratch.reset(new QTemporaryDir());
    m_settings.reset(new QTemporaryDir());
    QVERIFY2(m_scratch->isValid(), "Could not create the scratch directory");
    QVERIFY2(m_settings->isValid(), "Could not create the temporary settings directory");

    // QSettings::setPath() has no getter, so the current base directory is recovered from a
    // probe instance — its file lands at <base>/<organization>/<application>.ini — and put
    // back in cleanupTestCase().
    const QSettings probe(QSettings::IniFormat, QSettings::UserScope,
                          QStringLiteral("SeamlyProbeOrganization"), QStringLiteral("SeamlyProbeApp"));
    m_originalSettingsBase = QFileInfo(QFileInfo(probe.fileName()).absolutePath()).absolutePath();

    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings->path());

    QVERIFY2(!QCoreApplication::organizationName().isEmpty(),
             "The data root resolves through the application organization name, which must be set");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief scratchPath builds a path inside the suite's scratch directory.
 * @param relative path relative to the scratch root.
 * @return absolute path under the scratch directory.
 */
QString TST_DataRoot::scratchPath(const QString &relative) const
{
    return QDir::cleanPath(m_scratch->path() + QLatin1Char('/') + relative);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief init clears the data-root setting so each test starts from "nothing configured".
 */
void TST_DataRoot::init()
{
    clearDataRoot();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief cleanupTestCase undoes both redirections made by initTestCase().
 */
void TST_DataRoot::cleanupTestCase()
{
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_originalSettingsBase);

    m_settings.reset();
    m_scratch.reset();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief writeDataRoot stores a data root directly in the shared settings file.
 * @param root data root to store.
 */
void TST_DataRoot::writeDataRoot(const QString &root) const
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QCoreApplication::organizationName(), commonIniName);
    settings.setValue(dataRootKey, root);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief clearDataRoot removes the data-root setting, restoring the "unconfigured" state.
 */
void TST_DataRoot::clearDataRoot() const
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QCoreApplication::organizationName(), commonIniName);
    settings.remove(dataRootKey);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DefaultDataRootIsSeamlyUnderHome checks the built-in default: renamed from
 * ~/seamly2d by Task 34, and from the too-generic ~/seamly to ~/seamlyData by Task 53.
 */
void TST_DataRoot::DefaultDataRootIsSeamlyUnderHome() const
{
    QCOMPARE(VCommonSettings::getDefaultDataRoot(), QDir::homePath() + QStringLiteral("/seamlyData"));
    QVERIFY(!VCommonSettings::getDefaultDataRoot().endsWith(QStringLiteral("seamly2d")));
    // "seamly" alone collides too easily with an unrelated user folder of the same name.
    QVERIFY(!VCommonSettings::getDefaultDataRoot().endsWith(QStringLiteral("/seamly")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LegacyDataRootIsTheOldSeamly2dFolder checks the location first-run resolution
 * looks in when deciding whether an upgrading user already has a data tree.
 */
void TST_DataRoot::LegacyDataRootIsTheOldSeamly2dFolder() const
{
    QCOMPARE(VCommonSettings::getLegacyDataRoot(), QDir::homePath() + QStringLiteral("/seamly2d"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UnconfiguredRootFallsBackToTheDefault checks that reading the root before
 * anything has been configured yields the built-in default rather than an empty string.
 */
void TST_DataRoot::UnconfiguredRootFallsBackToTheDefault() const
{
    QCOMPARE(VCommonSettings::dataRoot(), VCommonSettings::getDefaultDataRoot());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief EveryDefaultPathDerivesFromTheDataRoot is the core of Task 34: all nine data
 * subfolders — including the two that live in VSettings — must come from the one root,
 * so that changing it relocates the whole tree.
 */
void TST_DataRoot::EveryDefaultPathDerivesFromTheDataRoot() const
{
    const QString root = scratchPath(QStringLiteral("configured-root"));
    writeDataRoot(root);

    QCOMPARE(VCommonSettings::dataRoot(), root);

    const QStringList derived
    {
        VCommonSettings::getDefaultIndividualSizePath(),
        VCommonSettings::getDefaultMultisizePath(),
        VCommonSettings::getDefaultTemplatePath(),
        VCommonSettings::getDefaultBodyScansPath(),
        VCommonSettings::getDefaultLabelTemplatePath(),
        VCommonSettings::getDefaultImageFilePath(),
        VCommonSettings::getDefaultBackupFilePath(),
        VSettings::getDefaultPatternPath(),
        VSettings::getDefaultLayoutPath()
    };

    QCOMPARE(derived.size(), 9);

    for (const QString &path : derived)
    {
        QVERIFY2(path.startsWith(root + QLatin1Char('/')),
                 qPrintable(QStringLiteral("'%1' is not derived from the data root '%2'").arg(path, root)));
        // A leftover hard-coded literal would still spell the old folder name.
        QVERIFY2(!path.contains(QStringLiteral("/seamly2d/")),
                 qPrintable(QStringLiteral("'%1' still contains a hard-coded seamly2d folder").arg(path)));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DataRootAcceptsAnyDriveOrPath covers the cloud/external-volume use case — the
 * root is honoured as written, with no requirement that it already exist.
 */
void TST_DataRoot::DataRootAcceptsAnyDriveOrPath() const
{
#ifdef Q_OS_WIN
    const QString cloudRoot = QStringLiteral("G:/My Drive/seamly");
#else
    const QString cloudRoot = QStringLiteral("/Volumes/GoogleDrive/My Drive/seamly");
#endif
    writeDataRoot(cloudRoot);

    QCOMPARE(VCommonSettings::dataRoot(), cloudRoot);
    QCOMPARE(VCommonSettings::getDefaultTemplatePath(), cloudRoot + QStringLiteral("/templates"));
    QVERIFY(VSettings::getDefaultPatternPath().startsWith(cloudRoot + QLatin1Char('/')));

    // Native separators are accepted too and normalised to Qt's '/' form.
    writeDataRoot(QStringLiteral("D:\\patterns\\seamly"));
#ifdef Q_OS_WIN
    QCOMPARE(VCommonSettings::dataRoot(), QStringLiteral("D:/patterns/seamly"));
#endif
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FirstRunWithoutLegacyTreeUsesTheDefault covers a fresh install: nothing at the
 * legacy location to adopt, so the new default root is chosen.
 */
void TST_DataRoot::FirstRunWithoutLegacyTreeUsesTheDefault() const
{
    const QString defaultRoot = scratchPath(QStringLiteral("fresh/seamly"));
    const QString legacyRoot  = scratchPath(QStringLiteral("fresh/seamly2d"));
    QVERIFY(!QFileInfo::exists(defaultRoot));
    QVERIFY(!QFileInfo::exists(legacyRoot));

    bool adopted = true;
    QCOMPARE(VCommonSettings::chooseFirstRunDataRoot(defaultRoot, legacyRoot, &adopted), defaultRoot);
    QVERIFY(!adopted);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FirstRunAdoptsAnExistingLegacyTree covers the upgrading user: a populated legacy
 * tree and no new root means the existing tree becomes the root, in place — the data keeps
 * working without anything being copied or moved.
 */
void TST_DataRoot::FirstRunAdoptsAnExistingLegacyTree() const
{
    const QString defaultRoot = scratchPath(QStringLiteral("upgrade/seamly"));
    const QString legacyRoot  = scratchPath(QStringLiteral("upgrade/seamly2d"));
    QVERIFY(QDir().mkpath(legacyRoot + QStringLiteral("/measurements/individual")));
    QVERIFY(!QFileInfo::exists(defaultRoot));

    bool adopted = false;
    QCOMPARE(VCommonSettings::chooseFirstRunDataRoot(defaultRoot, legacyRoot, &adopted), legacyRoot);
    QVERIFY(adopted);

    // A file lying in a directory is enough; the tree does not have to look like anything.
    QVERIFY(QFileInfo(legacyRoot + QStringLiteral("/measurements/individual")).isDir());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FirstRunPrefersAnExistingNewRoot checks that an existing new-style tree wins over
 * a legacy one, so a user who has already moved to the new layout is not dragged back.
 */
void TST_DataRoot::FirstRunPrefersAnExistingNewRoot() const
{
    const QString defaultRoot = scratchPath(QStringLiteral("both/seamly"));
    const QString legacyRoot  = scratchPath(QStringLiteral("both/seamly2d"));
    QVERIFY(QDir().mkpath(legacyRoot + QStringLiteral("/templates")));
    QVERIFY(QDir().mkpath(defaultRoot + QStringLiteral("/templates")));

    bool adopted = true;
    QCOMPARE(VCommonSettings::chooseFirstRunDataRoot(defaultRoot, legacyRoot, &adopted), defaultRoot);
    QVERIFY(!adopted);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AdoptionNeverRemovesTheLegacyTree guards the "never delete the old tree"
 * requirement: adoption is a settings change only, and every file stays where it was.
 */
void TST_DataRoot::AdoptionNeverRemovesTheLegacyTree() const
{
    const QString defaultRoot = scratchPath(QStringLiteral("keep/seamly"));
    const QString legacyRoot  = scratchPath(QStringLiteral("keep/seamly2d"));
    QVERIFY(QDir().mkpath(legacyRoot + QStringLiteral("/patterns")));

    const QString patternFile = legacyRoot + QStringLiteral("/patterns/keep-me.sm2d");
    QFile file(patternFile);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("<pattern/>");
    file.close();

    bool adopted = false;
    QCOMPARE(VCommonSettings::chooseFirstRunDataRoot(defaultRoot, legacyRoot, &adopted), legacyRoot);
    QVERIFY(adopted);

    QVERIFY2(QFileInfo::exists(patternFile), "First-run resolution must never move or delete user data");
    QCOMPARE(QFileInfo(patternFile).size(), qint64(10));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AConfiguredRootIsNeverOverwritten checks the re-entrancy of first-run
 * resolution: a root already chosen — by the user, or by a Windows installer prompt —
 * survives every later start-up.
 *
 * This one goes through initializeDataRoot(), which reads the settings file redirected in
 * initTestCase(). It is safe because a configured root short-circuits the resolution
 * before any home-directory path is consulted, and nothing is created on disk.
 */
void TST_DataRoot::AConfiguredRootIsNeverOverwritten() const
{
    const QString chosenRoot = scratchPath(QStringLiteral("chosen-by-installer"));
    writeDataRoot(chosenRoot);

    bool adopted = true;
    QCOMPARE(VCommonSettings::initializeDataRoot(&adopted), chosenRoot);
    QVERIFY(!adopted);
    QCOMPARE(VCommonSettings::initializeDataRoot(), chosenRoot);
    QCOMPARE(VCommonSettings::dataRoot(), chosenRoot);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief EnsureDataRootTreeCreatesTheSubfolders checks the "create the subfolder tree at
 * that location on first use" requirement.
 */
void TST_DataRoot::EnsureDataRootTreeCreatesTheSubfolders() const
{
    const QString root = scratchPath(QStringLiteral("fresh-tree"));
    QVERIFY(!QFileInfo::exists(root));

    QVERIFY(VCommonSettings::ensureDataRootTree(root));

    const QStringList expected
    {
        QStringLiteral("measurements/individual"),
        QStringLiteral("measurements/multisize"),
        QStringLiteral("templates"),
        QStringLiteral("bodyscans"),
        QStringLiteral("label templates"),
        QStringLiteral("images"),
        QStringLiteral("backups"),
        QStringLiteral("patterns"),
        QStringLiteral("layouts")
    };

    for (const QString &subdirectory : expected)
    {
        QVERIFY2(QFileInfo(root + QLatin1Char('/') + subdirectory).isDir(),
                 qPrintable(QStringLiteral("'%1' was not created under the data root").arg(subdirectory)));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief EnsureDataRootTreeKeepsExistingFiles checks that populating a root is purely
 * additive — an adopted tree full of the user's work must come through untouched.
 */
void TST_DataRoot::EnsureDataRootTreeKeepsExistingFiles() const
{
    const QString root = scratchPath(QStringLiteral("populated-tree"));
    QVERIFY(QDir().mkpath(root + QStringLiteral("/patterns")));

    const QString existing = root + QStringLiteral("/patterns/existing.sm2d");
    QFile file(existing);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("<pattern/>");
    file.close();

    QVERIFY(VCommonSettings::ensureDataRootTree(root));

    QVERIFY(QFileInfo::exists(existing));
    QCOMPARE(QFileInfo(existing).size(), qint64(10));
    QVERIFY(QFileInfo(root + QStringLiteral("/templates")).isDir());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief StartupResolvesThenSeedsTheConfiguredRoot locks the two-step start-up sequence
 * both applications perform in openSettings(), and the split between its halves.
 *
 * Task 51's clean-machine install verification found that a fresh installation recorded the
 * data root but never created it: initializeDataRoot() writes the setting directly instead
 * of going through setDataRoot(), which was the only caller of ensureDataRootTree(). Nothing
 * seeded the tree, so Preferences → Paths listed nine folders that did not exist.
 *
 * The fix is a second call in each application's openSettings(), and this case pins both
 * halves of it. The first assertion is as important as the second: resolution must stay
 * free of side effects on disk, because these tests call initializeDataRoot() while on a
 * real run its default root is ~/seamlyData — seeding from inside it would create folders
 * in the developer's home directory during every test run.
 */
void TST_DataRoot::StartupResolvesThenSeedsTheConfiguredRoot() const
{
    const QString root = scratchPath(QStringLiteral("startup-root"));
    writeDataRoot(root);
    QVERIFY(!QFileInfo::exists(root));

    // Step one, as openSettings() does it: settle the root. This must not touch the disk.
    QCOMPARE(VCommonSettings::initializeDataRoot(), root);
    QVERIFY2(!QFileInfo::exists(root),
             "initializeDataRoot() must not create directories - it is called by these tests, "
             "and on a real run its default root lies under the home directory");

    // Step two: seed the tree at whatever root step one settled on.
    QVERIFY(VCommonSettings::ensureDataRootTree(VCommonSettings::dataRoot()));

    const QStringList expected
    {
        QStringLiteral("measurements/individual"),
        QStringLiteral("measurements/multisize"),
        QStringLiteral("templates"),
        QStringLiteral("bodyscans"),
        QStringLiteral("label templates"),
        QStringLiteral("images"),
        QStringLiteral("backups"),
        QStringLiteral("patterns"),
        QStringLiteral("layouts")
    };

    for (const QString &subdirectory : expected)
    {
        QVERIFY2(QFileInfo(root + QLatin1Char('/') + subdirectory).isDir(),
                 qPrintable(QStringLiteral("'%1' is missing after start-up seeded the root")
                                .arg(subdirectory)));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RebaseMovesPathsInsideTheOldRoot checks the rule Preferences → Paths applies so
 * that changing the root actually relocates the subfolders shown alongside it.
 */
void TST_DataRoot::RebaseMovesPathsInsideTheOldRoot() const
{
    const QString oldRoot = QStringLiteral("C:/Users/tester/seamly2d");
    const QString newRoot = QStringLiteral("G:/My Drive/seamly");

    QCOMPARE(VCommonSettings::rebaseOntoDataRoot(oldRoot + QStringLiteral("/templates"), oldRoot, newRoot),
             newRoot + QStringLiteral("/templates"));
    QCOMPARE(VCommonSettings::rebaseOntoDataRoot(oldRoot + QStringLiteral("/measurements/individual"),
                                                 oldRoot, newRoot),
             newRoot + QStringLiteral("/measurements/individual"));
    // The root itself follows the move.
    QCOMPARE(VCommonSettings::rebaseOntoDataRoot(oldRoot, oldRoot, newRoot), newRoot);

#ifdef Q_OS_WIN
    // Native separators on the way in still match — Windows only: QDir::fromNativeSeparators()
    // rewrites backslashes only there, because on POSIX a backslash is a legal filename
    // character, so such a path genuinely is not inside the old root.
    QCOMPARE(VCommonSettings::rebaseOntoDataRoot(QStringLiteral("C:\\Users\\tester\\seamly2d\\images"),
                                                 oldRoot, newRoot),
             newRoot + QStringLiteral("/images"));
#endif
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RebaseLeavesPathsOutsideTheOldRootAlone checks that a folder the user has
 * deliberately parked elsewhere is not dragged along by a data-root change.
 */
void TST_DataRoot::RebaseLeavesPathsOutsideTheOldRootAlone() const
{
    const QString oldRoot = QStringLiteral("C:/Users/tester/seamly2d");
    const QString newRoot = QStringLiteral("G:/My Drive/seamly");
    const QString outside = QStringLiteral("D:/shared/company templates");

    QCOMPARE(VCommonSettings::rebaseOntoDataRoot(outside, oldRoot, newRoot), outside);
    // A sibling whose name merely starts with the root's name is not inside it.
    QCOMPARE(VCommonSettings::rebaseOntoDataRoot(oldRoot + QStringLiteral("-backup/templates"), oldRoot, newRoot),
             oldRoot + QStringLiteral("-backup/templates"));
    // Nothing to do when the root did not change, or when either root is unknown.
    QCOMPARE(VCommonSettings::rebaseOntoDataRoot(oldRoot + QStringLiteral("/images"), oldRoot, oldRoot),
             oldRoot + QStringLiteral("/images"));
    QCOMPARE(VCommonSettings::rebaseOntoDataRoot(outside, QString(), newRoot), outside);
    QCOMPARE(VCommonSettings::rebaseOntoDataRoot(outside, oldRoot, QString()), outside);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PruneRemovesAnEmptyLegacyTree checks the Task 53 cleanup of the skeleton left
 * behind by the rename — the nine subfolders ensureDataRootTree() created, holding nothing.
 */
void TST_DataRoot::PruneRemovesAnEmptyLegacyTree() const
{
    const QString legacy     = scratchPath(QStringLiteral("prune-empty/seamly2d"));
    const QString configured = scratchPath(QStringLiteral("prune-empty/seamlyData"));

    QVERIFY(VCommonSettings::ensureDataRootTree(legacy));
    QVERIFY(QFileInfo(legacy + QStringLiteral("/patterns")).isDir());

    QVERIFY(VCommonSettings::pruneEmptyLegacyDataRoot(legacy, configured));
    QVERIFY(!QFileInfo::exists(legacy));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PruneKeepsALegacyTreeHoldingFiles checks the condition that matters most: one file
 * anywhere in the tree, at any depth, and nothing is removed.
 */
void TST_DataRoot::PruneKeepsALegacyTreeHoldingFiles() const
{
    const QString legacy     = scratchPath(QStringLiteral("prune-populated/seamly2d"));
    const QString configured = scratchPath(QStringLiteral("prune-populated/seamlyData"));

    QVERIFY(VCommonSettings::ensureDataRootTree(legacy));

    // Buried as deep as the real tree goes, so a shallow existence check cannot pass this.
    const QString pattern = legacy + QStringLiteral("/measurements/individual/keiko.smis");
    QFile file(pattern);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("<measurements/>");
    file.close();

    QVERIFY(!VCommonSettings::pruneEmptyLegacyDataRoot(legacy, configured));
    QVERIFY(QFileInfo::exists(pattern));
    QVERIFY(QFileInfo(legacy + QStringLiteral("/patterns")).isDir());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PruneNeverRemovesTheConfiguredRoot checks the upgrading user's case: Task 34 adopts
 * an existing ~/seamly2d in place, which makes it the live data tree, not a leftover.
 */
void TST_DataRoot::PruneNeverRemovesTheConfiguredRoot() const
{
    const QString legacy = scratchPath(QStringLiteral("prune-adopted/seamly2d"));
    QVERIFY(VCommonSettings::ensureDataRootTree(legacy));

    QVERIFY(!VCommonSettings::pruneEmptyLegacyDataRoot(legacy, legacy));
    QVERIFY(QFileInfo(legacy).isDir());

    // Same path, native separators and a trailing slash — still the same directory.
    QVERIFY(!VCommonSettings::pruneEmptyLegacyDataRoot(legacy, legacy + QLatin1Char('/')));
    QVERIFY(QFileInfo(legacy).isDir());
#ifdef Q_OS_WIN
    QVERIFY(!VCommonSettings::pruneEmptyLegacyDataRoot(legacy, QDir::toNativeSeparators(legacy)));
    QVERIFY(QFileInfo(legacy).isDir());
    // Windows path comparison is case-insensitive, so a differently cased spelling of the
    // configured root still names the live tree.
    QVERIFY(!VCommonSettings::pruneEmptyLegacyDataRoot(legacy, legacy.toUpper()));
    QVERIFY(QFileInfo(legacy).isDir());
#endif
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PruneKeepsALegacyRootHoldingTheConfiguredRoot checks that a root nested inside the
 * legacy tree is not taken down along with its parent.
 */
void TST_DataRoot::PruneKeepsALegacyRootHoldingTheConfiguredRoot() const
{
    const QString legacy     = scratchPath(QStringLiteral("prune-nested/seamly2d"));
    const QString configured = legacy + QStringLiteral("/current");

    QVERIFY(QDir().mkpath(configured));

    QVERIFY(!VCommonSettings::pruneEmptyLegacyDataRoot(legacy, configured));
    QVERIFY(QFileInfo(configured).isDir());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PruneIgnoresAMissingLegacyRoot checks the ordinary case for a fresh install, where
 * there is no legacy tree at all, plus the empty-argument guards.
 */
void TST_DataRoot::PruneIgnoresAMissingLegacyRoot() const
{
    const QString missing    = scratchPath(QStringLiteral("prune-missing/seamly2d"));
    const QString configured = scratchPath(QStringLiteral("prune-missing/seamlyData"));

    QVERIFY(!QFileInfo::exists(missing));
    QVERIFY(!VCommonSettings::pruneEmptyLegacyDataRoot(missing, configured));
    QVERIFY(!VCommonSettings::pruneEmptyLegacyDataRoot(QString(), configured));

    // A file where the legacy root would be is not a directory, and must not be deleted.
    const QString file = scratchPath(QStringLiteral("prune-missing/notadirectory"));
    QVERIFY(QDir().mkpath(scratchPath(QStringLiteral("prune-missing"))));
    QFile decoy(file);
    QVERIFY(decoy.open(QIODevice::WriteOnly));
    decoy.write("x");
    decoy.close();

    QVERIFY(!VCommonSettings::pruneEmptyLegacyDataRoot(file, configured));
    QVERIFY(QFileInfo::exists(file));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief StrayCommonSettingsAreMergedThenDeleted checks the Task 53 half of the
 * "Unknown Organization" recovery: values are carried forward, a value the user has since
 * changed still wins, and only then is the stray file and its folder removed.
 *
 * Safe because initTestCase() has pointed QSettings' IniFormat/UserScope base at a temporary
 * directory, so both the stray and the destination live inside it.
 */
void TST_DataRoot::StrayCommonSettingsAreMergedThenDeleted() const
{
    static const QString strayOrganization = QStringLiteral("Unknown Organization");

    // A value only the stray has, and one the destination already holds differently.
    QSettings stray(QSettings::IniFormat, QSettings::UserScope, strayOrganization, commonIniName);
    stray.setValue(QStringLiteral("paths/bodyscans"), QStringLiteral("G:/My Drive/seamlyData/bodyscans"));
    stray.setValue(QStringLiteral("paths/templates"), QStringLiteral("C:/stale/templates"));
    stray.sync();
    const QString strayFileName = stray.fileName();
    QVERIFY(QFileInfo::exists(strayFileName));

    QSettings destination(QSettings::IniFormat, QSettings::UserScope,
                          QCoreApplication::organizationName(), commonIniName);
    destination.setValue(QStringLiteral("paths/templates"), QStringLiteral("G:/My Drive/seamlyData/templates"));
    destination.sync();

    // mergeStrayCommonSettings() is private; initializeDataRoot() is its only caller.
    VCommonSettings::initializeDataRoot();

    QSettings merged(QSettings::IniFormat, QSettings::UserScope,
                     QCoreApplication::organizationName(), commonIniName);
    QCOMPARE(merged.value(QStringLiteral("paths/bodyscans")).toString(),
             QStringLiteral("G:/My Drive/seamlyData/bodyscans"));
    // The user's own value survives the merge — copy-if-missing, never overwrite.
    QCOMPARE(merged.value(QStringLiteral("paths/templates")).toString(),
             QStringLiteral("G:/My Drive/seamlyData/templates"));

    QVERIFY2(!QFileInfo::exists(strayFileName), "The merged stray settings file should have been deleted");
    QVERIFY2(!QFileInfo(QFileInfo(strayFileName).absolutePath()).isDir(),
             "The emptied 'Unknown Organization' folder should have been removed");
}
