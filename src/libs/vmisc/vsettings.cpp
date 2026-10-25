//-----------------------------------------------------------------------------
//  @file   vsettings.cpp
//  @author Douglas S Caskey
//  @date   13 July, 2025
//
//  @brief
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program to create and model patterns of clothing.
//  Copyright (C) 2017-2025 Seamly2D project
//  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
//
//  Seamly2D is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Seamly2D is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  @file   vsettings.cpp
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date   22 11, 2014
//
//  @brief
//  @copyright
//  This source code is part of the Valentina project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2015 Valentina project
//  <https://bitbucket.org/dismine/valentina> All Rights Reserved.
//
//  Valentina is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Valentina is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
//-----------------------------------------------------------------------------

#include "vsettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QMetaType>
#include <QPrinter>
#include <QSharedPointer>
#include <QString>
#include <QVariant>
#include <QPrinterInfo>
#include <QtDebug>

#include "../vmisc/def.h"
#include "../vmisc/vmath.h"

#ifndef QPRINTENGINE_H
Q_DECLARE_METATYPE(QMarginsF)
#endif

namespace
{
const QString settingConfigurationPointLanguage = QStringLiteral("configuration/label_language");

const QString settingPathsPattern = QStringLiteral("paths/pattern");
const QString settingPathsLayout  = QStringLiteral("paths/layout");
const QString settingPathsSeamlyLayoutApp = QStringLiteral("paths/seamlyLayoutApp");

const QString settingPatternGraphicalOutput = QStringLiteral("pattern/graphicalOutput");

const QString settingCommunityServer       = QStringLiteral("community/server");
const QString settingCommunityServerSecure = QStringLiteral("community/serverSecure");
const QString settingCommunityUseProxy     = QStringLiteral("community/useProxy");
const QString settingCommunityProxyAddress = QStringLiteral("community/proxyAddress");
const QString settingCommunityProxyPort    = QStringLiteral("community/proxyPort");
const QString settingCommunityProxyUser    = QStringLiteral("community/proxyUser");
const QString settingCommunityProxyPass    = QStringLiteral("community/proxyPass");
const QString settingCommunityUsername     = QStringLiteral("community/username");
const QString settingCommunitySavePassword = QStringLiteral("community/savePassword");
const QString settingCommunityUserPassword = QStringLiteral("community/userpassword");



/**
 * @brief copySampleFiles copies files matching nameFilter from sourceDir into destinationDir.
 *
 * Shared by SeedSamplePatterns() and SeedSampleMeasurements(): both need the same
 * missing-source-is-a-no-op and never-overwrite-an-existing-file behaviour, only the file
 * extension differs.
 *
 * @param sourceDir      bundled samples folder; a missing folder is a silent no-op.
 * @param destinationDir the user's folder to seed; created if missing.
 * @param nameFilter     glob filter selecting which bundled files to copy, e.g. "*.sm2d".
 * @return number of sample files copied.
 */
int copySampleFiles(const QString &sourceDir, const QString &destinationDir, const QString &nameFilter)
{
    const QDir source(sourceDir);
    if (!source.exists())
    {
        return 0;
    }

    QDir destination(destinationDir);
    if (!destination.mkpath(QStringLiteral(".")))
    {
        qWarning() << "Could not create the folder" << QDir::toNativeSeparators(destinationDir);
        return 0;
    }

    int copied = 0;
    const QStringList sampleFiles = source.entryList(QStringList(nameFilter), QDir::Files);
    for (const QString &fileName : sampleFiles)
    {
        const QString target = destination.filePath(fileName);
        if (QFileInfo::exists(target))
        {
            continue;
        }

        if (QFile::copy(source.filePath(fileName), target))
        {
            ++copied;
        }
        else
        {
            qWarning() << "Could not copy sample file" << QDir::toNativeSeparators(source.filePath(fileName))
                       << "to" << QDir::toNativeSeparators(target);
        }
    }
    return copied;
}
}

//---------------------------------------------------------------------------------------------------------------------
VSettings::VSettings(Format format, Scope scope, const QString &organization,
                     const QString &application, QObject *parent)
    : VCommonSettings(format, scope, organization, application, parent)
{
    qRegisterMetaType<QMarginsF>();
}

//---------------------------------------------------------------------------------------------------------------------
VSettings::VSettings(const QString &fileName, QSettings::Format format, QObject *parent)
  : VCommonSettings(fileName, format, parent)
{
    qRegisterMetaType<QMarginsF>();
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::getPointNameLanguage() const
{
    return value(settingConfigurationPointLanguage, QLocale().bcp47Name()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::setPointNameLanguage(const QString &value)
{
    setValue(settingConfigurationPointLanguage, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::getDefaultPatternPath()
{
    return dataSubdirPath(tr("patterns"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::getPatternPath() const
{
    return value(settingPathsPattern, getDefaultPatternPath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetPathPattern(const QString &value)
{
    setValue(settingPathsPattern, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::getSamplePatternsPath()
{
    return SharePath(QStringLiteral("/samples/patterns"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeedSamplePatterns copies the bundled sample patterns into the user's patterns folder.
 *
 * A standard user has no write access to the Program Files installation folder, so the
 * bundled samples under sourceDir can be opened but never saved back in place. Copying them
 * into the writable destinationDir gives the user an editable starting point instead.
 *
 * Purely additive and safe to call on every launch: an existing file at the destination is
 * left untouched, so a sample the user deleted stays deleted and an edited copy is never
 * overwritten by the original.
 *
 * @param sourceDir      bundled samples/patterns folder; a missing folder is a silent no-op.
 * @param destinationDir the user's patterns folder; created if missing.
 * @return number of sample files copied.
 */
int VSettings::SeedSamplePatterns(const QString &sourceDir, const QString &destinationDir)
{
    return copySampleFiles(sourceDir, destinationDir, QStringLiteral("*.sm2d"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::getSampleMeasurementsIndividualPath()
{
    return SharePath(QStringLiteral("/samples/measurements/individual"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::getSampleMeasurementsMultisizePath()
{
    return SharePath(QStringLiteral("/samples/measurements/multisize"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SeedSampleMeasurements copies the bundled sample measurement files into the user's
 * measurements folder.
 *
 * Same rationale as SeedSamplePatterns(): a standard user has no write access to the Program
 * Files installation folder, so a bundled measurement file can be opened but never saved back
 * in place. Copying it into the writable destinationDir gives the user an editable starting
 * point instead.
 *
 * Purely additive and safe to call on every launch: an existing file at the destination is
 * left untouched, so a sample the user deleted stays deleted and an edited copy is never
 * overwritten by the original.
 *
 * @param sourceDir      bundled samples/measurements folder; a missing folder is a silent no-op.
 * @param destinationDir the user's measurements folder; created if missing.
 * @param nameFilter     glob filter selecting which bundled files to copy, e.g. "*.smis".
 * @return number of sample files copied.
 */
int VSettings::SeedSampleMeasurements(const QString &sourceDir, const QString &destinationDir,
                                       const QString &nameFilter)
{
    return copySampleFiles(sourceDir, destinationDir, nameFilter);
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::getDefaultLayoutPath()
{
    return dataSubdirPath(tr("layouts"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::getLayoutPath() const
{
    return value(settingPathsLayout, getDefaultLayoutPath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetPathLayout(const QString &value)
{
    setValue(settingPathsLayout, value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getSeamlyLayoutAppPath returns the user-configured path of the SeamlyLayout executable.
 * @return absolute path of the SeamlyLayout executable, or an empty string when not configured.
 */
QString VSettings::getSeamlyLayoutAppPath() const
{
    return value(settingPathsSeamlyLayoutApp, QString()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setSeamlyLayoutAppPath stores the path of the SeamlyLayout executable.
 * @param value absolute path of the SeamlyLayout executable.
 */
void VSettings::setSeamlyLayoutAppPath(const QString &value)
{
    setValue(settingPathsSeamlyLayoutApp, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VSettings::GetGraphicalOutput() const
{
    return value(settingPatternGraphicalOutput, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetGraphicalOutput(const bool &value)
{
    setValue(settingPatternGraphicalOutput, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::GetServer() const
{
    return value(settingCommunityServer, "community.seamly2d-project.org").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetServer(const QString &value)
{
    setValue(settingCommunityServer, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VSettings::GetServerSecure() const
{
    return value(settingCommunityServerSecure, 0).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetServerSecure(const bool &value)
{
    setValue(settingCommunityServerSecure, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VSettings::GetProxy() const
{
    return value(settingCommunityUseProxy, 0).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetProxy(const bool &value)
{
    setValue(settingCommunityUseProxy, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::GetProxyAddress() const
{
    return value(settingCommunityProxyAddress).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetProxyAddress(const QString &value)
{
    setValue(settingCommunityProxyAddress, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::GetProxyPort() const
{
    return value(settingCommunityProxyPort).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetProxyPort(const QString &value)
{
    setValue(settingCommunityProxyPort, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::GetProxyUser() const
{
    return value(settingCommunityProxyUser).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetProxyUser(const QString &value)
{
    setValue(settingCommunityProxyUser, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::GetProxyPass() const
{
    return value(settingCommunityProxyPass).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetProxyPass(const QString &value)
{
    setValue(settingCommunityProxyPass, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::GetUsername() const
{
    return value(settingCommunityUsername).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetUsername(const QString &value)
{
    setValue(settingCommunityUsername, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VSettings::GetSavePassword() const
{
    return value(settingCommunitySavePassword, 0).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetSavePassword(const bool &value)
{
    setValue(settingCommunitySavePassword, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VSettings::GetUserPassword() const
{
    return value(settingCommunityUserPassword).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VSettings::SetUserPassword(const QString &value)
{
    setValue(settingCommunityUserPassword, value);
}
