/******************************************************************************
 **  @file   legacy_data_archive.cpp
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

#include "legacy_data_archive.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QStringList>

// QZipWriter and QZipReader are Qt private API. They moved from QtGui to QtCore in Qt 6, and
// the build pins Qt 6.11.1, so the header path is fixed for as long as that pin holds. The
// alternatives were a third-party zip library or a hand-written store-only writer; neither is
// worth a dependency or 200 lines for one first-run backup. vmisc.pro carries the matching
// "QT += core-private".
#include <QtCore/private/qzipreader_p.h>
#include <QtCore/private/qzipwriter_p.h>

namespace
{
    // One read of a large file must not become one allocation of a large buffer.
    const qint64 hashChunkSize = 1 * 1024 * 1024;

    //-----------------------------------------------------------------------------------------------------------------
    bool fail(QString *errorMessage, const QString &text)
    {
        if (errorMessage != nullptr) { *errorMessage = text; }
        return false;
    }

    //-----------------------------------------------------------------------------------------------------------------
    QString cleaned(const QString &path)
    {
        return QDir::cleanPath(QDir::fromNativeSeparators(path.trimmed()));
    }

    //-----------------------------------------------------------------------------------------------------------------
    /**
     * @brief hashOfFile streams a file through SHA-256 without holding it all in memory.
     * @return the digest, or an empty array when the file could not be read.
     */
    QByteArray hashOfFile(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            return QByteArray();
        }

        QCryptographicHash hash(QCryptographicHash::Sha256);
        while (!file.atEnd())
        {
            const QByteArray chunk = file.read(hashChunkSize);
            if (chunk.isEmpty() && !file.atEnd())
            {
                return QByteArray();
            }
            hash.addData(chunk);
        }
        return hash.result();
    }

    //-----------------------------------------------------------------------------------------------------------------
    /**
     * @brief relativeEntries lists a tree as .zip entry names.
     *
     * Directories come back too, so an empty folder survives the round trip. Entry names use
     * forward slashes and no leading slash, which is what the .zip format requires.
     *
     * @param root tree to walk.
     * @param files out-parameter, relative names of the regular files.
     * @param directories out-parameter, relative names of the directories.
     * @param errorMessage out-parameter, set when a symbolic link was found.
     * @return false when the tree holds a symbolic link, which cannot be archived.
     */
    bool relativeEntries(const QString &root, QStringList *files, QStringList *directories, QString *errorMessage)
    {
        const QDir base(root);
        const QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot;

        QDirIterator iterator(root, filters, QDirIterator::Subdirectories);
        while (iterator.hasNext())
        {
            const QString entryPath = iterator.next();
            const QFileInfo entry(entryPath);

            // A .zip entry cannot reproduce a link, and a link that quietly resolved outside
            // the tree would make the backup incomplete in a way nobody could see.
            //
            // isSymbolicLink() and isJunction(), NOT isSymLink(): the last one also reports
            // a Windows .lnk shortcut, which is an ordinary file holding ordinary bytes. A
            // user who drops a shortcut in their patterns folder must not block the backup.
            if (entry.isSymbolicLink() || entry.isJunction())
            {
                return fail(errorMessage, QStringLiteral("'%1' is a symbolic link")
                                              .arg(QDir::toNativeSeparators(entryPath)));
            }

            const QString relative = base.relativeFilePath(entryPath);
            if (entry.isDir())
            {
                directories->append(relative);
            }
            else
            {
                files->append(relative);
            }
        }
        return true;
    }
}   // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief archivePath names the backup .zip, avoiding any file already there.
 *
 * The timestamp makes the name meaningful to a person browsing the folder, and makes a second
 * migration on the same machine a second file rather than an overwrite.
 *
 * @param destinationRoot folder the archive is written into, normally the new data root.
 * @param when timestamp to put in the name; the caller passes it so tests can fix it.
 * @return absolute path of a file that does not exist yet.
 */
QString LegacyDataArchive::archivePath(const QString &destinationRoot, const QDateTime &when)
{
    const QString root = cleaned(destinationRoot);
    const QString stamp = when.toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString base = root + QStringLiteral("/seamly2d-backup-") + stamp;

    QString candidate = base + QStringLiteral(".zip");
    for (int suffix = 2; QFileInfo::exists(candidate); ++suffix)
    {
        candidate = base + QStringLiteral("-%1.zip").arg(suffix);
    }
    return candidate;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief create writes every file and folder below sourceRoot into one .zip.
 *
 * A partial archive is never left behind: any failure removes the file before returning, so
 * the caller cannot mistake a truncated archive for a good one.
 *
 * @param sourceRoot tree to archive; must be an existing directory.
 * @param archiveFile .zip to write; its parent directory must already exist.
 * @param errorMessage out-parameter, set on failure; pass null when the caller does not care.
 * @return true when the archive was written and closed without error.
 */
bool LegacyDataArchive::create(const QString &sourceRoot, const QString &archiveFile, QString *errorMessage)
{
    const QString source = cleaned(sourceRoot);
    const QString archive = cleaned(archiveFile);

    if (source.isEmpty() || !QFileInfo(source).isDir())
    {
        return fail(errorMessage, QStringLiteral("'%1' is not a directory").arg(QDir::toNativeSeparators(source)));
    }

    // Writing the archive into the tree it is archiving would either capture the archive in
    // itself or grow without end.
    if (archive.startsWith(source + QLatin1Char('/'), Qt::CaseInsensitive))
    {
        return fail(errorMessage, QStringLiteral("the archive would sit inside the tree it archives"));
    }

    QStringList files;
    QStringList directories;
    if (!relativeEntries(source, &files, &directories, errorMessage))
    {
        return false;
    }

    QZipWriter writer(archive);
    if (!writer.isWritable())
    {
        return fail(errorMessage, QStringLiteral("could not open '%1' for writing")
                                      .arg(QDir::toNativeSeparators(archive)));
    }

    for (const QString &directory : qAsConst(directories))
    {
        writer.addDirectory(directory);
    }

    for (const QString &relative : qAsConst(files))
    {
        QFile file(source + QLatin1Char('/') + relative);
        if (!file.open(QIODevice::ReadOnly))
        {
            writer.close();
            QFile::remove(archive);
            return fail(errorMessage, QStringLiteral("could not read '%1'").arg(relative));
        }

        // The QIODevice overload streams, so one huge file does not become one huge
        // allocation.
        writer.addFile(relative, &file);
        file.close();

        if (writer.status() != QZipWriter::NoError)
        {
            const int status = static_cast<int>(writer.status());
            writer.close();
            QFile::remove(archive);
            return fail(errorMessage, QStringLiteral("writing '%1' failed with status %2").arg(relative).arg(status));
        }
    }

    writer.close();
    if (writer.status() != QZipWriter::NoError)
    {
        const int status = static_cast<int>(writer.status());
        QFile::remove(archive);
        return fail(errorMessage, QStringLiteral("closing the archive failed with status %1").arg(status));
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief verifyAgainst proves the archive can be read back and holds the whole tree.
 *
 * The size and CRC in a .zip central directory describe what the writer *meant* to store, so
 * they are not enough to justify calling the backup good. Every entry is decompressed and its
 * SHA-256 compared with the file it came from.
 *
 * @param sourceRoot tree the archive was made from.
 * @param archiveFile .zip to check.
 * @param errorMessage out-parameter, naming the first entry that did not match.
 * @return true when every file and folder in the tree is in the archive, byte for byte.
 */
bool LegacyDataArchive::verifyAgainst(const QString &sourceRoot, const QString &archiveFile, QString *errorMessage)
{
    const QString source = cleaned(sourceRoot);
    const QString archive = cleaned(archiveFile);

    QStringList files;
    QStringList directories;
    if (!relativeEntries(source, &files, &directories, errorMessage))
    {
        return false;
    }

    QZipReader reader(archive);
    if (!reader.isReadable() || reader.status() != QZipReader::NoError)
    {
        return fail(errorMessage, QStringLiteral("could not open '%1' for reading")
                                      .arg(QDir::toNativeSeparators(archive)));
    }

    QHash<QString, QZipReader::FileInfo> entries;
    const QList<QZipReader::FileInfo> infoList = reader.fileInfoList();
    for (const QZipReader::FileInfo &info : infoList)
    {
        entries.insert(info.filePath, info);
    }

    for (const QString &relative : qAsConst(files))
    {
        const auto entry = entries.constFind(relative);
        if (entry == entries.constEnd() || !entry->isFile)
        {
            return fail(errorMessage, QStringLiteral("'%1' is missing from the archive").arg(relative));
        }

        const QString filePath = source + QLatin1Char('/') + relative;
        const qint64 sourceSize = QFileInfo(filePath).size();
        if (entry->size != sourceSize)
        {
            return fail(errorMessage, QStringLiteral("'%1' is %2 bytes in the archive, %3 on disk")
                                          .arg(relative)
                                          .arg(entry->size)
                                          .arg(sourceSize));
        }

        const QByteArray stored = reader.fileData(relative);
        if (static_cast<qint64>(stored.size()) != sourceSize)
        {
            return fail(errorMessage, QStringLiteral("'%1' did not decompress to its recorded size").arg(relative));
        }

        const QByteArray sourceHash = hashOfFile(filePath);
        if (sourceHash.isEmpty())
        {
            return fail(errorMessage, QStringLiteral("could not re-read '%1'").arg(relative));
        }
        if (QCryptographicHash::hash(stored, QCryptographicHash::Sha256) != sourceHash)
        {
            return fail(errorMessage, QStringLiteral("'%1' does not match the file it came from").arg(relative));
        }
    }

    // QZipWriter::addDirectory() stores a trailing slash; accept both spellings so the check
    // does not depend on that detail.
    for (const QString &relative : qAsConst(directories))
    {
        if (!entries.contains(relative) && !entries.contains(relative + QLatin1Char('/')))
        {
            return fail(errorMessage, QStringLiteral("folder '%1' is missing from the archive").arg(relative));
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief archive is the whole sequence: write the .zip, then read it back and verify it.
 *
 * The source tree is never touched. A failed verification takes the half-good archive with
 * it, so a bad backup can never be mistaken for a good one.
 *
 * @param sourceRoot tree to back up, normally ~/seamly2d.
 * @param destinationRoot folder to write the archive into, normally the new data root.
 * @param errorMessage out-parameter, set on failure; pass null when the caller does not care.
 * @return absolute path of the verified archive, or an empty string on failure.
 */
QString LegacyDataArchive::archive(const QString &sourceRoot, const QString &destinationRoot, QString *errorMessage)
{
    const QString source = cleaned(sourceRoot);
    const QString destination = cleaned(destinationRoot);

    if (source.isEmpty() || !QFileInfo(source).isDir())
    {
        fail(errorMessage, QStringLiteral("'%1' is not a directory").arg(QDir::toNativeSeparators(source)));
        return QString();
    }
    if (destination.isEmpty() || !QFileInfo(destination).isDir())
    {
        fail(errorMessage, QStringLiteral("'%1' is not a directory").arg(QDir::toNativeSeparators(destination)));
        return QString();
    }

    // A destination inside the source would try to archive the .zip into itself.
    if (destination.compare(source, Qt::CaseInsensitive) == 0 ||
        destination.startsWith(source + QLatin1Char('/'), Qt::CaseInsensitive))
    {
        fail(errorMessage, QStringLiteral("the destination is inside the tree being archived"));
        return QString();
    }

    const QString archiveFile = archivePath(destination, QDateTime::currentDateTime());
    if (!create(source, archiveFile, errorMessage))
    {
        return QString();
    }

    if (!verifyAgainst(source, archiveFile, errorMessage))
    {
        QFile::remove(archiveFile);
        return QString();
    }

    return archiveFile;
}
