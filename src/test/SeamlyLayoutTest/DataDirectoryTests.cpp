// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file DataDirectoryTests.cpp
// @brief Qt tests for DataDirectory: valid, empty, relative, and unwritable SEAMLY_DATA_DIR.

#include "DataDirectory.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

class DataDirectoryTests : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void valid_createsSettingsAndLogFolders();
    void empty_leavesTheDefaults();
    void relative_isRejectedWithConfigCode();
    void unwritable_isRejectedWithCannotCreateCode();
};

// Clear the setting so one test cannot leak into the next.
void DataDirectoryTests::cleanup()
{
    DataDirectory::configureFrom(QString());
}

void DataDirectoryTests::valid_createsSettingsAndLogFolders()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());

    QCOMPARE(DataDirectory::configureFrom(temp.path()), DataDirectory::exitOk);
    QCOMPARE(DataDirectory::path(), QDir(temp.path()).absolutePath());
    QCOMPARE(DataDirectory::settingsRoot(), QDir(temp.path()).filePath(QStringLiteral("settings")));
    QCOMPARE(DataDirectory::logsDirectory(), QDir(temp.path()).filePath(QStringLiteral("logs/seamlylayout")));
    QVERIFY(QFileInfo(DataDirectory::settingsRoot()).isDir());
    QVERIFY(QFileInfo(DataDirectory::logsDirectory()).isDir());
}

void DataDirectoryTests::empty_leavesTheDefaults()
{
    QCOMPARE(DataDirectory::configureFrom(QString()), DataDirectory::exitOk);
    QVERIFY(DataDirectory::path().isEmpty());
    QVERIFY(DataDirectory::settingsRoot().isEmpty());
    QVERIFY(DataDirectory::logsDirectory().isEmpty());
}

void DataDirectoryTests::relative_isRejectedWithConfigCode()
{
    QCOMPARE(DataDirectory::configureFrom(QStringLiteral("relative/data")), DataDirectory::exitConfig);
    QVERIFY(DataDirectory::path().isEmpty());
}

// A regular file in place of the directory cannot hold the folders, on every platform.
void DataDirectoryTests::unwritable_isRejectedWithCannotCreateCode()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString blocker = QDir(temp.path()).filePath(QStringLiteral("not_a_folder"));
    QFile file(blocker);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    QCOMPARE(DataDirectory::configureFrom(blocker), DataDirectory::exitCannotCreate);
    QVERIFY(DataDirectory::path().isEmpty());
}

QTEST_GUILESS_MAIN(DataDirectoryTests)
#include "DataDirectoryTests.moc"
