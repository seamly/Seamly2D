/******************************************************************************
 **  @file   tst_dataroot.cpp
 **  @author slspencer
 **  @date   July 26, 2026
 **
 **  @brief
 **  Unit tests for the relocatable user-data root: its default location, the
 **  derivation of every data subfolder from it, and the Preferences rebase rule.
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
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vsettings.h"

#include <QCoreApplication>
#include <QDir>

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

    // Task SettingsFiles.1: the common settings file no longer resolves through
    // QSettings::setPath() — it lives under GenericConfigLocation, which cannot be
    // redirected per test — so it gets its own override at the same temporary base.
    VCommonSettings::setCommonSettingsBaseDir(m_settings->path());

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
 *
 * Also re-arms the common-settings base override: a test that moves it to its own
 * directory and then fails would otherwise leave every later test pointed at the wrong
 * base.
 */
void TST_DataRoot::init()
{
    VCommonSettings::setCommonSettingsBaseDir(m_settings->path());
    clearDataRoot();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief cleanupTestCase undoes both redirections made by initTestCase().
 */
void TST_DataRoot::cleanupTestCase()
{
    VCommonSettings::setCommonSettingsBaseDir(QString());
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
    QSettings settings(VCommonSettings::commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(dataRootKey, root);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief clearDataRoot removes the data-root setting, restoring the "unconfigured" state.
 */
void TST_DataRoot::clearDataRoot() const
{
    QSettings settings(VCommonSettings::commonSettingsFilePath(), QSettings::IniFormat);
    settings.remove(dataRootKey);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CommonSettingsFileLivesUnderLocalConfig pins the Task SettingsFiles.1 location
 * contract: qt6_common.ini resolves as <base>/<organization>/qt6_common.ini, and the
 * production base is GenericConfigLocation — %LOCALAPPDATA% on Windows — not the Roaming
 * folder Qt's IniFormat/UserScope default names.
 *
 * The production half is a string comparison only; nothing is read or written at the
 * real location.
 */
void TST_DataRoot::CommonSettingsFileLivesUnderLocalConfig() const
{
    const QString organization = QCoreApplication::organizationName();
    QCOMPARE(VCommonSettings::commonSettingsFilePath(),
             m_settings->path() + QLatin1Char('/') + organization + QLatin1Char('/') + commonIniName
                 + QStringLiteral(".ini"));

    VCommonSettings::setCommonSettingsBaseDir(QString());
    const QString production = VCommonSettings::commonSettingsFilePath();
    VCommonSettings::setCommonSettingsBaseDir(m_settings->path());

    const QString genericBase = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QVERIFY2(production.startsWith(genericBase + QLatin1Char('/')),
             qPrintable(QStringLiteral("'%1' is not under GenericConfigLocation '%2'")
                            .arg(production, genericBase)));
    QCOMPARE(production,
             genericBase + QLatin1Char('/') + organization + QLatin1Char('/') + commonIniName
                 + QStringLiteral(".ini"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CommonSettingsBridgeCopiesRoamingFileForward checks the move's migration: a
 * common settings file at the pre-move location — whatever Qt's IniFormat/UserScope
 * resolution names, which initTestCase() redirected to the temporary base — is copied
 * to the new location, and the source file survives so a rollback to an earlier release
 * keeps its settings.
 */
void TST_DataRoot::CommonSettingsBridgeCopiesRoamingFileForward() const
{
    // A "Local" base distinct from the redirected "Roaming" base, under the scratch
    // directory so a failing test leaves no dangling override target.
    const QString localBase = scratchPath(QStringLiteral("bridge-copies/local-config"));
    QVERIFY(QDir().mkpath(localBase));
    VCommonSettings::setCommonSettingsBaseDir(localBase);

    {
        QSettings roaming(QSettings::IniFormat, QSettings::UserScope,
                          QCoreApplication::organizationName(), commonIniName);
        roaming.setValue(dataRootKey, QStringLiteral("G:/My Drive/Seamly"));
        roaming.sync();
    }

    const QString target = VCommonSettings::migrateCommonSettingsLocation();
    QCOMPARE(target, VCommonSettings::commonSettingsFilePath());
    QVERIFY(QFileInfo::exists(target));

    const QSettings migrated(target, QSettings::IniFormat);
    QCOMPARE(migrated.value(dataRootKey).toString(), QStringLiteral("G:/My Drive/Seamly"));

    const QSettings roamingProbe(QSettings::IniFormat, QSettings::UserScope,
                                 QCoreApplication::organizationName(), commonIniName);
    QVERIFY2(QFileInfo::exists(roamingProbe.fileName()),
             "the migration must copy, never move, the pre-move settings file");

    VCommonSettings::setCommonSettingsBaseDir(m_settings->path());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CommonSettingsBridgeNeverOverwritesTheLocalFile checks re-entrancy: once a file
 * exists at the new location, the migration must not touch it, whatever an older file at
 * the pre-move location says.
 */
void TST_DataRoot::CommonSettingsBridgeNeverOverwritesTheLocalFile() const
{
    const QString localBase = scratchPath(QStringLiteral("bridge-keeps/local-config"));
    QVERIFY(QDir().mkpath(localBase));
    VCommonSettings::setCommonSettingsBaseDir(localBase);

    {
        QSettings local(VCommonSettings::commonSettingsFilePath(), QSettings::IniFormat);
        local.setValue(dataRootKey, QStringLiteral("D:/kept/Seamly"));
        local.sync();
    }
    {
        QSettings roaming(QSettings::IniFormat, QSettings::UserScope,
                          QCoreApplication::organizationName(), commonIniName);
        roaming.setValue(dataRootKey, QStringLiteral("C:/stale/Seamly"));
        roaming.sync();
    }

    VCommonSettings::migrateCommonSettingsLocation();

    const QSettings kept(VCommonSettings::commonSettingsFilePath(), QSettings::IniFormat);
    QCOMPARE(kept.value(dataRootKey).toString(), QStringLiteral("D:/kept/Seamly"));

    VCommonSettings::setCommonSettingsBaseDir(m_settings->path());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DefaultDataRootIsSeamly2dUnderHome checks the built-in default: a fixed
 * ~/seamly2d, matching the location the Windows installer creates on a fresh install
 * (Setup no longer offers a choice of folder).
 */
void TST_DataRoot::DefaultDataRootIsSeamly2dUnderHome() const
{
    QCOMPARE(VCommonSettings::getDefaultDataRoot(), QDir::homePath() + QStringLiteral("/seamly2d"));
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

    QCOMPARE(VCommonSettings::initializeDataRoot(), chosenRoot);
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
 * both applications perform in openSettings().
 *
 * Resolution must stay free of side effects on disk because these tests call
 * initializeDataRoot(), while on a real run its default root is ~/seamlyData — seeding
 * from inside it would create folders in the developer's home directory during every test run.
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
 * @brief SeedSampleMeasurementsCopiesBundledFiles checks the measurements half of the same
 * read-only Program Files fix: a bundled individual measurement file must be copied into the
 * writable measurements folder so it can be opened, edited, and saved back.
 */
void TST_DataRoot::SeedSampleMeasurementsCopiesBundledFiles() const
{
    const QString source = scratchPath(QStringLiteral("bundled-samples/measurements/individual"));
    QVERIFY(writeTestFile(source + QStringLiteral("/male_chest_102cm.smis"), QStringLiteral("<measurements/>")));
    QVERIFY(writeTestFile(source + QStringLiteral("/readme.txt"), QStringLiteral("not a measurement file")));

    const QString destination = scratchPath(QStringLiteral("seeded-measurements/individual"));
    QVERIFY(!QFileInfo::exists(destination));

    QCOMPARE(VSettings::SeedSampleMeasurements(source, destination, QStringLiteral("*.smis")), 1);

    QVERIFY(QFileInfo::exists(destination + QStringLiteral("/male_chest_102cm.smis")));
    QVERIFY2(!QFileInfo::exists(destination + QStringLiteral("/readme.txt")),
             "SeedSampleMeasurements must copy only files matching the name filter");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeedSampleMeasurementsNeverOverwritesAnExistingFile checks the merge rule: a file the
 * user already has at the destination, sample or edited copy alike, is left untouched.
 */
void TST_DataRoot::SeedSampleMeasurementsNeverOverwritesAnExistingFile() const
{
    const QString source = scratchPath(QStringLiteral("bundled-samples-2/measurements/individual"));
    QVERIFY(writeTestFile(source + QStringLiteral("/male_chest_102cm.smis"), QStringLiteral("<measurements/>")));

    const QString destination = scratchPath(QStringLiteral("edited-measurements/individual"));
    const QString edited = destination + QStringLiteral("/male_chest_102cm.smis");
    QVERIFY(writeTestFile(edited, QStringLiteral("<measurements>edited by the user</measurements>")));

    QCOMPARE(VSettings::SeedSampleMeasurements(source, destination, QStringLiteral("*.smis")), 0);

    QCOMPARE(readTestFile(edited), QStringLiteral("<measurements>edited by the user</measurements>"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeedSampleMeasurementsIsANoOpWhenSourceIsMissing checks the platform where samples
 * were never bundled next to the executable: seeding must not create the destination folder.
 */
void TST_DataRoot::SeedSampleMeasurementsIsANoOpWhenSourceIsMissing() const
{
    const QString source = scratchPath(QStringLiteral("no-such-samples-folder/measurements"));
    const QString destination = scratchPath(QStringLiteral("untouched-measurements"));

    QCOMPARE(VSettings::SeedSampleMeasurements(source, destination, QStringLiteral("*.smis")), 0);
    QVERIFY(!QFileInfo::exists(destination));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeedSampleMeasurementsHonoursTheNameFilter checks that the caller-supplied filter,
 * not a hard-coded extension, decides which bundled files are copied — the same function
 * seeds both individual (.smis) and multisize (.smms) measurement folders.
 */
void TST_DataRoot::SeedSampleMeasurementsHonoursTheNameFilter() const
{
    const QString source = scratchPath(QStringLiteral("bundled-samples/measurements/multisize"));
    QVERIFY(writeTestFile(source + QStringLiteral("/gost_man_ru.smms"), QStringLiteral("<measurements/>")));
    QVERIFY(writeTestFile(source + QStringLiteral("/unrelated.smis"), QStringLiteral("<measurements/>")));

    const QString destination = scratchPath(QStringLiteral("seeded-measurements/multisize"));

    QCOMPARE(VSettings::SeedSampleMeasurements(source, destination, QStringLiteral("*.smms")), 1);

    QVERIFY(QFileInfo::exists(destination + QStringLiteral("/gost_man_ru.smms")));
    QVERIFY2(!QFileInfo::exists(destination + QStringLiteral("/unrelated.smis")),
             "The name filter must exclude files with a different extension");
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
 * @brief PerAppPathsPersistToTheOwnSettingsFile guards against VSettings's per-app path
 * getters and setters (pattern, layout, the SeamlyLayout executable, graphical output)
 * reopening a fresh QSettings from format()/scope()/organizationName()/applicationName()
 * instead of reading and writing "this".
 *
 * Application2D builds VSettings from an explicit file path (see
 * Application2D::InitTrVars()), so organizationName() and applicationName() are both empty
 * on it. Reopening QSettings from those empty values landed the four settings under the
 * literal "Unknown Organization" file instead of the app's own qt6_seamly2d.ini, which is
 * also why qt6_seamly2d.ini's [paths] section was missing pattern and layout.
 */
void TST_DataRoot::PerAppPathsPersistToTheOwnSettingsFile() const
{
    const QString iniPath = scratchPath(QStringLiteral("per-app-paths/qt6_seamly2d.ini"));

    VSettings settings(iniPath, QSettings::IniFormat);
    settings.SetPathPattern(QStringLiteral("G:/My Drive/seamlyData/patterns"));
    settings.SetPathLayout(QStringLiteral("G:/My Drive/seamlyData/layouts"));
    settings.setSeamlyLayoutAppPath(QStringLiteral("C:/Program Files/SeamlyApps/SeamlyLayout.exe"));
    settings.SetGraphicalOutput(false);
    settings.sync();

    QCOMPARE(settings.getPatternPath(), QStringLiteral("G:/My Drive/seamlyData/patterns"));
    QCOMPARE(settings.getLayoutPath(), QStringLiteral("G:/My Drive/seamlyData/layouts"));
    QCOMPARE(settings.getSeamlyLayoutAppPath(), QStringLiteral("C:/Program Files/SeamlyApps/SeamlyLayout.exe"));
    QCOMPARE(settings.GetGraphicalOutput(), false);

    // Re-open the file directly, bypassing the VSettings instance, to prove the values
    // landed on disk in the app's own ini rather than only in a live QSettings cache.
    const QSettings onDisk(iniPath, QSettings::IniFormat);
    QCOMPARE(onDisk.value(QStringLiteral("paths/pattern")).toString(),
             QStringLiteral("G:/My Drive/seamlyData/patterns"));
    QCOMPARE(onDisk.value(QStringLiteral("paths/layout")).toString(),
             QStringLiteral("G:/My Drive/seamlyData/layouts"));
    QCOMPARE(onDisk.value(QStringLiteral("paths/seamlyLayoutApp")).toString(),
             QStringLiteral("C:/Program Files/SeamlyApps/SeamlyLayout.exe"));
    QCOMPARE(onDisk.value(QStringLiteral("pattern/graphicalOutput")).toBool(), false);
}
