// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file DataDirectory.cpp
// @brief SEAMLY_DATA_DIR support. See DataDirectory.h.

#include "DataDirectory.h"

#include <QDir>
#include <QStringList>
#include <QTemporaryFile>
#include <QtGlobal>

#include <cstdio>

namespace {

// Absolute data directory. Empty when SEAMLY_DATA_DIR is not in use.
QString g_dataDirectory;

} // anonymous namespace

// @brief Read SEAMLY_DATA_DIR and apply it.
int DataDirectory::configure()
{
    return configureFrom(qEnvironmentVariable("SEAMLY_DATA_DIR"));
} // DataDirectory::configure

// @brief Validate the requested folder and create its settings and log folders.
int DataDirectory::configureFrom(const QString &requested)
{
    g_dataDirectory.clear();

    // Unset or empty: normal run.
    if (requested.isEmpty()) {
        return exitOk;
    } // if no request

    if (!QDir::isAbsolutePath(requested)) {
        std::fputs("SEAMLY_DATA_DIR must be an absolute directory.\n", stderr);
        return exitConfig;
    } // if relative

    const QDir directory(QDir::cleanPath(QDir::fromNativeSeparators(requested)));
    const QStringList paths = {QStringLiteral("settings"), QStringLiteral("logs/seamlylayout")};
    for (const QString &path : paths) {
        // mkpath succeeds for an existing read-only folder, so write a probe file too.
        // Permission bits alone are unreliable on NTFS.
        const bool created = directory.mkpath(path);
        QTemporaryFile probe(directory.filePath(path + QStringLiteral("/.seamly-write-XXXXXX")));
        if (!created || !probe.open() || probe.write("test") != 4 || !probe.flush()) {
            std::fputs("SEAMLY_DATA_DIR must contain writable settings and log directories.\n", stderr);
            return exitCannotCreate;
        } // if not writable
    } // for each folder

    g_dataDirectory = directory.absolutePath();
    return exitOk;
} // DataDirectory::configureFrom

// @brief Return the absolute data directory, or empty.
QString DataDirectory::path()
{
    return g_dataDirectory;
} // DataDirectory::path

// @brief Return <dir>/settings, or empty.
QString DataDirectory::settingsRoot()
{
    if (g_dataDirectory.isEmpty()) {
        return QString();
    } // if not set
    return QDir(g_dataDirectory).filePath(QStringLiteral("settings"));
} // DataDirectory::settingsRoot

// @brief Return <dir>/logs/seamlylayout, or empty.
QString DataDirectory::logsDirectory()
{
    if (g_dataDirectory.isEmpty()) {
        return QString();
    } // if not set
    return QDir(g_dataDirectory).filePath(QStringLiteral("logs/seamlylayout"));
} // DataDirectory::logsDirectory
