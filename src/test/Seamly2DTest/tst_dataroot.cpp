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

#include "../vmisc/installer_record.h"
#include "../vmisc/legacy_data_archive.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vsettings.h"

// The archive cases read the .zip back with the same private reader the code under test
// uses. See the note in legacy_data_archive.cpp; Seamly2DTest.pro carries the matching
// "QT += core-private".
#include <QtCore/private/qzipreader_p.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>

#include <filesystem>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
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
 * @brief DefaultDataRootIsSeamlyUnderDocuments checks the built-in default.
 *
 * The lineage: ~/seamly2d (original) → ~/seamly (Task 34) → ~/seamlyData (Task 53) →
 * <Documents>/Seamly (Task 60). The last move is the one with a principle behind it —
 * these are documents the user creates, opens and backs up, so they belong where every
 * other application puts documents, while internal state stays in the platform's
 * application-data locations.
 *
 * The expected value is built from QStandardPaths rather than hard-coded, deliberately:
 * hard-coding "Documents" would pass on this machine and fail on a localized Linux system
 * or a redirected Windows profile — the very cases DocumentsLocation exists to handle.
 */
void TST_DataRoot::DefaultDataRootIsSeamlyUnderDocuments() const
{
    QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documents.isEmpty())
    {
        documents = QDir::homePath();
    }
    QCOMPARE(VCommonSettings::getDefaultDataRoot(), QDir::cleanPath(documents) + QStringLiteral("/Seamly"));

    // None of the superseded names may come back.
    const QString root = VCommonSettings::getDefaultDataRoot();
    QVERIFY(!root.endsWith(QStringLiteral("seamly2d")));
    QVERIFY(!root.endsWith(QStringLiteral("seamlyData")));
    // "seamly" alone collides too easily with an unrelated user folder of the same name;
    // the capitalised family name under Documents does not.
    QVERIFY(!root.endsWith(QStringLiteral("/seamly")));
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
 * @brief InstallerDataRootIsCleanOrEmpty checks the shape of whatever the Windows installer
 * recorded, on any platform. Pins the contract of InstallerRecord::dataRoot().
 *
 * The value cannot be arranged from a test: it lives under HKLM, which an unelevated process
 * cannot write. What the test can hold is the contract every caller depends on — the result
 * is either empty or a path already cleaned into Qt's '/' form, because initializeDataRoot()
 * stores it in the settings file verbatim and every getDefault*Path() then appends to it.
 *
 * Off Windows the result is always empty, and there the assertions still run: a change that
 * made the function return something on Linux or macOS would fail here.
 */
void TST_DataRoot::InstallerDataRootIsCleanOrEmpty() const
{
    const QString recorded = InstallerRecord::dataRoot();
    if (recorded.isEmpty())
    {
        return;
    }

    QVERIFY2(!recorded.contains(QLatin1Char('\\')),
             "the recorded root must be converted out of native separators");
    QCOMPARE(recorded, QDir::cleanPath(recorded));
    QVERIFY2(QDir::isAbsolutePath(recorded), "the installer records an absolute path");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AConfiguredRootOutranksTheInstaller checks that the installer's answer is a
 * first-run default and nothing more.
 *
 * The Windows installer records its data-root page in HKLM, machine-wide. A user who later
 * moves the root in Preferences → Paths must keep that choice, on this machine and every
 * later start-up, so the configured value has to win. Same guarantee as
 * AConfiguredRootIsNeverOverwritten, stated against the case that motivated it.
 */
void TST_DataRoot::AConfiguredRootOutranksTheInstaller() const
{
    const QString chosenByUser = scratchPath(QStringLiteral("chosen-in-preferences"));
    writeDataRoot(chosenByUser);

    QCOMPARE(VCommonSettings::initializeDataRoot(), chosenByUser);
    QVERIFY2(VCommonSettings::initializeDataRoot() != InstallerRecord::dataRoot() ||
                 InstallerRecord::dataRoot().isEmpty(),
             "a configured root must not be replaced by the installer's");
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
 * @brief writeTestFile creates a file with known contents, making any parent directories.
 *
 * @param path     file to write.
 * @param contents text to put in it.
 * @return true when the file was written.
 */
static bool writeTestFile(const QString &path, const QString &contents)
{
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
    {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    file.write(contents.toUtf8());
    file.close();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief readTestFile returns a file's contents, or an empty string when it cannot be read.
 */
static QString readTestFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString();
    }
    const QString contents = QString::fromUtf8(file.readAll());
    file.close();
    return contents;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeedSamplePatternsCopiesBundledFiles checks the fix for samples bundled under a
 * read-only Program Files install: seeding must copy every bundled .sm2d into the writable
 * patterns folder, and ignore files that are not sample patterns.
 */
void TST_DataRoot::SeedSamplePatternsCopiesBundledFiles() const
{
    const QString source = scratchPath(QStringLiteral("bundled-samples/patterns"));
    QVERIFY(writeTestFile(source + QStringLiteral("/male_shirt.sm2d"), QStringLiteral("<pattern/>")));
    QVERIFY(writeTestFile(source + QStringLiteral("/trousers.sm2d"), QStringLiteral("<pattern/>")));
    QVERIFY(writeTestFile(source + QStringLiteral("/readme.txt"), QStringLiteral("not a pattern")));

    const QString destination = scratchPath(QStringLiteral("seeded-patterns"));
    QVERIFY(!QFileInfo::exists(destination));

    QCOMPARE(VSettings::SeedSamplePatterns(source, destination), 2);

    QVERIFY(QFileInfo::exists(destination + QStringLiteral("/male_shirt.sm2d")));
    QVERIFY(QFileInfo::exists(destination + QStringLiteral("/trousers.sm2d")));
    QVERIFY2(!QFileInfo::exists(destination + QStringLiteral("/readme.txt")),
             "SeedSamplePatterns must copy only .sm2d files");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeedSamplePatternsNeverOverwritesAnExistingFile checks the merge rule: a file the
 * user already has at the destination, sample or edited copy alike, is left untouched.
 */
void TST_DataRoot::SeedSamplePatternsNeverOverwritesAnExistingFile() const
{
    const QString source = scratchPath(QStringLiteral("bundled-samples-2/patterns"));
    QVERIFY(writeTestFile(source + QStringLiteral("/male_shirt.sm2d"), QStringLiteral("<pattern/>")));

    const QString destination = scratchPath(QStringLiteral("edited-patterns"));
    const QString edited = destination + QStringLiteral("/male_shirt.sm2d");
    QVERIFY(writeTestFile(edited, QStringLiteral("<pattern>edited by the user</pattern>")));

    QCOMPARE(VSettings::SeedSamplePatterns(source, destination), 0);

    QCOMPARE(readTestFile(edited), QStringLiteral("<pattern>edited by the user</pattern>"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeedSamplePatternsIsANoOpWhenSourceIsMissing checks the platform where samples were
 * never bundled next to the executable: seeding must not create the destination folder.
 */
void TST_DataRoot::SeedSamplePatternsIsANoOpWhenSourceIsMissing() const
{
    const QString source = scratchPath(QStringLiteral("no-such-samples-folder"));
    const QString destination = scratchPath(QStringLiteral("untouched-patterns"));

    QCOMPARE(VSettings::SeedSamplePatterns(source, destination), 0);
    QVERIFY(!QFileInfo::exists(destination));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MigrationCopiesTheWholeTreeIncludingUnknownFolders is the central Task 60 rule.
 *
 * Users add their own directories to the data tree — `Projects` and `bodyscans` have both
 * been seen on real machines — so a migration that walked a known list of subfolders would
 * silently strand everything the list did not mention. This case therefore mixes standard
 * folders, a deeply nested one, and folders the code has never heard of, and requires all
 * of them at the destination.
 */
void TST_DataRoot::MigrationCopiesTheWholeTreeIncludingUnknownFolders() const
{
    const QString source = scratchPath(QStringLiteral("migrate-all/seamly2d"));
    const QString destination = scratchPath(QStringLiteral("migrate-all/Seamly"));

    const QStringList files
    {
        QStringLiteral("patterns/shirt.sm2d"),
        QStringLiteral("measurements/individual/sue.smis"),
        QStringLiteral("measurements/multisize/table.smms"),
        QStringLiteral("measurements/experimental/draft.smis"),  // an extra child of a known folder
        QStringLiteral("Projects/spring/notes.txt"),             // entirely the user's own
        QStringLiteral("bodyscans/scan.dat"),
        QStringLiteral("label templates/plain.xml"),
        QStringLiteral("images/logo.png")
    };

    for (const QString &relative : files)
    {
        QVERIFY(writeTestFile(source + QLatin1Char('/') + relative, relative));
    }

    int copied = 0;
    int skipped = 0;
    QString errorMessage;
    QVERIFY2(VCommonSettings::migrateDataTree(source, destination, &copied, &skipped, &errorMessage),
             qPrintable(errorMessage));

    QCOMPARE(copied, files.count());
    QCOMPARE(skipped, 0);

    for (const QString &relative : files)
    {
        const QString target = destination + QLatin1Char('/') + relative;
        QVERIFY2(QFileInfo::exists(target),
                 qPrintable(QStringLiteral("'%1' did not reach the new root").arg(relative)));
        QCOMPARE(readTestFile(target), relative);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MigrationNeverOverwritesAnExistingFile checks the merge rule.
 *
 * The destination can legitimately be a populated folder — the cloud-drive use case targets
 * one — so a collision must skip and be reported, never clobber the file already there.
 */
void TST_DataRoot::MigrationNeverOverwritesAnExistingFile() const
{
    const QString source = scratchPath(QStringLiteral("migrate-merge/seamly2d"));
    const QString destination = scratchPath(QStringLiteral("migrate-merge/Seamly"));

    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/shared.sm2d"), QStringLiteral("from the old tree")));
    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/fresh.sm2d"), QStringLiteral("only in the old tree")));
    QVERIFY(writeTestFile(destination + QStringLiteral("/patterns/shared.sm2d"), QStringLiteral("ALREADY HERE")));

    int copied = 0;
    int skipped = 0;
    QVERIFY(VCommonSettings::migrateDataTree(source, destination, &copied, &skipped));

    QCOMPARE(copied, 1);
    QCOMPARE(skipped, 1);
    QCOMPARE(readTestFile(destination + QStringLiteral("/patterns/shared.sm2d")), QStringLiteral("ALREADY HERE"));
    QCOMPARE(readTestFile(destination + QStringLiteral("/patterns/fresh.sm2d")),
             QStringLiteral("only in the old tree"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MigrationLeavesTheSourceTreeIntact — the whole point of copying rather than moving.
 *
 * The legacy tree must survive so a user can roll back to an earlier release.
 */
void TST_DataRoot::MigrationLeavesTheSourceTreeIntact() const
{
    const QString source = scratchPath(QStringLiteral("migrate-keep/seamly2d"));
    const QString destination = scratchPath(QStringLiteral("migrate-keep/Seamly"));

    const QString pattern = source + QStringLiteral("/patterns/keep.sm2d");
    QVERIFY(writeTestFile(pattern, QStringLiteral("<pattern/>")));

    QVERIFY(VCommonSettings::migrateDataTree(source, destination));

    QVERIFY2(QFileInfo::exists(pattern), "the source file was removed - migration must never move or delete");
    QCOMPARE(readTestFile(pattern), QStringLiteral("<pattern/>"));
    QVERIFY(QFileInfo(source + QStringLiteral("/patterns")).isDir());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MigrationRefusesADestinationInsideTheSource guards against endless recursion.
 *
 * Copying a tree into its own subdirectory would keep finding new files to copy. Cheap to
 * get wrong, expensive to notice — it fills the disk rather than failing.
 */
void TST_DataRoot::MigrationRefusesADestinationInsideTheSource() const
{
    const QString source = scratchPath(QStringLiteral("migrate-nested/seamly2d"));
    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/a.sm2d"), QStringLiteral("a")));

    QString errorMessage;
    QVERIFY(!VCommonSettings::migrateDataTree(source, source + QStringLiteral("/Seamly"), nullptr, nullptr,
                                              &errorMessage));
    QVERIFY(!errorMessage.isEmpty());

    // And the same tree as both ends.
    QVERIFY(!VCommonSettings::migrateDataTree(source, source));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MigrationMarksTheLegacyTree checks the breadcrumb that retires the old root.
 *
 * The tree is kept rather than deleted, so it needs to be obvious to the code — which must
 * not offer it again — and to a human opening the folder.
 */
void TST_DataRoot::MigrationMarksTheLegacyTree() const
{
    const QString source = scratchPath(QStringLiteral("migrate-marker/seamly2d"));
    const QString destination = scratchPath(QStringLiteral("migrate-marker/Seamly"));
    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/a.sm2d"), QStringLiteral("a")));

    QVERIFY(!VCommonSettings::dataTreeWasMigrated(source));
    QVERIFY(VCommonSettings::migrateDataTree(source, destination));
    QVERIFY(VCommonSettings::markDataTreeMigrated(source, destination));
    QVERIFY(VCommonSettings::dataTreeWasMigrated(source));

    // A human opening the folder must be told where the files went.
    const QString marker = source + QStringLiteral("/MIGRATED-TO-SEAMLY.txt");
    QVERIFY(QFileInfo::exists(marker));
    QVERIFY(readTestFile(marker).contains(QDir::toNativeSeparators(destination)));
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

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ArchiveHoldsEveryFileAndFolder is the backup's central promise.
 *
 * Folders the user invented and an empty folder are both included on purpose: the first is
 * the Task 60 rule, the second is the case a naive file-only walk silently drops.
 */
void TST_DataRoot::ArchiveHoldsEveryFileAndFolder() const
{
    const QString source = scratchPath(QStringLiteral("archive-all/seamly2d"));
    const QString destination = scratchPath(QStringLiteral("archive-all/Seamly"));
    QVERIFY(QDir().mkpath(destination));

    const QStringList files
    {
        QStringLiteral("patterns/shirt.sm2d"),
        QStringLiteral("measurements/individual/sue.smis"),
        QStringLiteral("Projects/spring/notes.txt"),   // entirely the user's own
        QStringLiteral("images/logo.png")
    };
    for (const QString &relative : files)
    {
        QVERIFY(writeTestFile(source + QLatin1Char('/') + relative, relative));
    }
    QVERIFY(QDir().mkpath(source + QStringLiteral("/layouts")));   // empty, and must still survive

    const QString archive = destination + QStringLiteral("/backup.zip");
    QString errorMessage;
    QVERIFY2(LegacyDataArchive::create(source, archive, &errorMessage), qPrintable(errorMessage));
    QVERIFY(QFileInfo(archive).size() > 0);

    QZipReader reader(archive);
    QVERIFY(reader.isReadable());

    QStringList entries;
    const QList<QZipReader::FileInfo> infoList = reader.fileInfoList();
    for (const QZipReader::FileInfo &info : infoList)
    {
        entries.append(info.filePath);
    }

    for (const QString &relative : files)
    {
        QVERIFY2(entries.contains(relative),
                 qPrintable(QStringLiteral("'%1' is not in the archive").arg(relative)));
        QCOMPARE(QString::fromUtf8(reader.fileData(relative)), relative);
    }
    QVERIFY2(entries.contains(QStringLiteral("layouts")) || entries.contains(QStringLiteral("layouts/")),
             "The empty folder is not in the archive");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ArchiveVerifiesAgainstTheTreeItCameFrom proves a good backup verifies clean.
 */
void TST_DataRoot::ArchiveVerifiesAgainstTheTreeItCameFrom() const
{
    const QString source = scratchPath(QStringLiteral("archive-verify/seamly2d"));
    const QString destination = scratchPath(QStringLiteral("archive-verify/Seamly"));
    QVERIFY(QDir().mkpath(destination));
    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/shirt.sm2d"), QStringLiteral("shirt")));
    QVERIFY(writeTestFile(source + QStringLiteral("/notes.txt"), QString()));   // empty file

    const QString archive = destination + QStringLiteral("/backup.zip");
    QString errorMessage;
    QVERIFY2(LegacyDataArchive::create(source, archive, &errorMessage), qPrintable(errorMessage));
    QVERIFY2(LegacyDataArchive::verifyAgainst(source, archive, &errorMessage), qPrintable(errorMessage));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ArchiveVerificationCatchesAMissingFile checks the count, not just the contents.
 *
 * A file added to the tree after the archive was written stands in for the case that
 * matters: an archive that does not hold everything the tree now holds.
 */
void TST_DataRoot::ArchiveVerificationCatchesAMissingFile() const
{
    const QString source = scratchPath(QStringLiteral("archive-missing/seamly2d"));
    const QString destination = scratchPath(QStringLiteral("archive-missing/Seamly"));
    QVERIFY(QDir().mkpath(destination));
    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/shirt.sm2d"), QStringLiteral("shirt")));

    const QString archive = destination + QStringLiteral("/backup.zip");
    QString errorMessage;
    QVERIFY2(LegacyDataArchive::create(source, archive, &errorMessage), qPrintable(errorMessage));

    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/skirt.sm2d"), QStringLiteral("skirt")));

    QVERIFY2(!LegacyDataArchive::verifyAgainst(source, archive, &errorMessage),
             "A file absent from the archive should fail verification");
    QVERIFY(errorMessage.contains(QStringLiteral("skirt.sm2d")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ArchiveVerificationCatchesAlteredContents proves the check reads the bytes back.
 *
 * The size and CRC recorded in the .zip describe what the writer meant to store. Only
 * decompressing the entry and comparing it with the file catches a mismatch.
 */
void TST_DataRoot::ArchiveVerificationCatchesAlteredContents() const
{
    const QString source = scratchPath(QStringLiteral("archive-altered/seamly2d"));
    const QString destination = scratchPath(QStringLiteral("archive-altered/Seamly"));
    QVERIFY(QDir().mkpath(destination));
    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/shirt.sm2d"), QStringLiteral("original")));

    const QString archive = destination + QStringLiteral("/backup.zip");
    QString errorMessage;
    QVERIFY2(LegacyDataArchive::create(source, archive, &errorMessage), qPrintable(errorMessage));

    // Same length, different bytes: a size check alone would pass this.
    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/shirt.sm2d"), QStringLiteral("ORIGINAL")));

    QVERIFY2(!LegacyDataArchive::verifyAgainst(source, archive, &errorMessage),
             "Altered contents should fail verification");
    QVERIFY(errorMessage.contains(QStringLiteral("shirt.sm2d")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ArchiveNamesDoNotCollide keeps a second migration from overwriting the first backup.
 */
void TST_DataRoot::ArchiveNamesDoNotCollide() const
{
    const QString destination = scratchPath(QStringLiteral("archive-names"));
    QVERIFY(QDir().mkpath(destination));

    const QDateTime when = QDateTime::fromString(QStringLiteral("2026-08-20T11:30:00"), Qt::ISODate);
    const QString first = LegacyDataArchive::archivePath(destination, when);
    QCOMPARE(QFileInfo(first).fileName(), QStringLiteral("seamly2d-backup-20260820-113000.zip"));

    QVERIFY(writeTestFile(first, QStringLiteral("not really a zip")));
    const QString second = LegacyDataArchive::archivePath(destination, when);
    QVERIFY(second != first);
    QVERIFY(!QFileInfo::exists(second));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ArchiveRefusesATreeHoldingASymbolicLink stops an incomplete backup passing as good.
 *
 * A .zip entry cannot reproduce a link, so a tree holding one is never archived.
 *
 * std::filesystem rather than QFile::link(), which on Windows writes a .lnk shortcut — an
 * ordinary file, and deliberately not what this guard rejects. Creating a real symbolic
 * link on Windows needs Developer Mode or elevation, so the case skips where it cannot.
 */
void TST_DataRoot::ArchiveRefusesATreeHoldingASymbolicLink() const
{
    const QString source = scratchPath(QStringLiteral("archive-link/seamly2d"));
    const QString destination = scratchPath(QStringLiteral("archive-link/Seamly"));
    QVERIFY(QDir().mkpath(destination));
    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/shirt.sm2d"), QStringLiteral("shirt")));

    const QString target = source + QStringLiteral("/patterns/shirt.sm2d");
    const QString link = source + QStringLiteral("/patterns/shirt-link.sm2d");
    try
    {
        std::filesystem::create_symlink(std::filesystem::path(target.toStdWString()),
                                        std::filesystem::path(link.toStdWString()));
    }
    catch (const std::filesystem::filesystem_error &)
    {
        QSKIP("This platform did not allow a symbolic link to be created");
    }
    QVERIFY2(QFileInfo(link).isSymbolicLink(), "The test did not create a real symbolic link");

    const QString archive = destination + QStringLiteral("/backup.zip");
    QString errorMessage;
    QVERIFY2(!LegacyDataArchive::create(source, archive, &errorMessage),
             "A tree holding a symbolic link should not be archived");
    QVERIFY(!QFileInfo::exists(archive));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ArchiveRefusesADestinationInsideTheSource stops the archive folding into itself.
 */
void TST_DataRoot::ArchiveRefusesADestinationInsideTheSource() const
{
    const QString source = scratchPath(QStringLiteral("archive-inside/seamly2d"));
    const QString destination = source + QStringLiteral("/Seamly");
    QVERIFY(QDir().mkpath(destination));
    QVERIFY(writeTestFile(source + QStringLiteral("/patterns/shirt.sm2d"), QStringLiteral("shirt")));

    QString errorMessage;
    const QString archive = LegacyDataArchive::archive(source, destination, &errorMessage);

    QVERIFY(archive.isEmpty());
    QVERIFY(!errorMessage.isEmpty());
    QVERIFY2(QFileInfo(source).isDir(), "The tree being archived should be untouched");
    QVERIFY2(QFileInfo(destination).isDir(), "The new root should still be there");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ArchiveLeavesTheSourceTreeInPlace is the rule that replaces the old branch's delete.
 *
 * This project keeps the legacy tree after migration (marker file, no delete) so a rollback
 * stays possible. LegacyDataArchive backs the tree up; it must never remove it.
 */
void TST_DataRoot::ArchiveLeavesTheSourceTreeInPlace() const
{
    const QString source = scratchPath(QStringLiteral("archive-keep/seamly2d"));
    const QString destination = scratchPath(QStringLiteral("archive-keep/Seamly"));
    QVERIFY(QDir().mkpath(destination));

    const QStringList files
    {
        QStringLiteral("patterns/shirt.sm2d"),
        QStringLiteral("measurements/individual/sue.smis")
    };
    for (const QString &relative : files)
    {
        QVERIFY(writeTestFile(source + QLatin1Char('/') + relative, relative));
    }

    QString errorMessage;
    const QString archive = LegacyDataArchive::archive(source, destination, &errorMessage);
    QVERIFY2(!archive.isEmpty(), qPrintable(errorMessage));

    QVERIFY2(QFileInfo(source).isDir(), "The old tree should still be there");
    for (const QString &relative : files)
    {
        QVERIFY2(QFileInfo(source + QLatin1Char('/') + relative).isFile(),
                 qPrintable(QStringLiteral("'%1' should still be in the old tree").arg(relative)));
    }
    QVERIFY2(QFileInfo(archive).isFile(), "The archive should be in the new data root");
    QCOMPARE(QFileInfo(archive).absolutePath(), QDir(destination).absolutePath());
}
