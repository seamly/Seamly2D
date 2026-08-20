/******************************************************************************
 **  @file   installer_record.cpp
 **  @author slspencer
 **  @date   August 19, 2026
 **
 **  @brief
 **  Reads the values the Windows installer recorded about this installation.
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

#include "installer_record.h"

#include <QDir>
#include <QSettings>
#include <QtGlobal>

namespace
{
#ifdef Q_OS_WIN
/** The key smsi.wxs writes its answers to. One place, so a rename cannot drift. */
const QLatin1String installKeyPath("HKEY_LOCAL_MACHINE\\SOFTWARE\\Seamly\\Seamly2D");

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief readInstalledPath reads one path value out of the install key.
 *
 * Registry64Format rather than NativeFormat: the apps are 64-bit today, so the two agree,
 * but the MSI is x64 and always writes the 64-bit view.
 *
 * @param name value name under the install key.
 * @return the value cleaned into Qt's '/' separator form; empty when absent or blank.
 */
QString readInstalledPath(const QString &name)
{
    const QSettings installKey(installKeyPath, QSettings::Registry64Format);
    const QString recorded = installKey.value(name).toString().trimmed();
    if (recorded.isEmpty())
    {
        return QString();
    }

    return QDir::cleanPath(QDir::fromNativeSeparators(recorded));
}
#endif
} // anonymous namespace

namespace InstallerRecord
{

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief dataRoot returns the user-data root the Windows installer recorded.
 *
 * Setup asks "Where do you keep your work?" and writes the answer to the DataRoot value.
 * Without this the answer was inert: every app resolved its own default instead, so a user
 * who was promised C:\Users\<user>\Documents\SeamlyData got <Documents>/Seamly
 * (Task InstWinX64.00).
 *
 * The value is machine-wide, so each user adopts it once, on that user's first run, into
 * their own settings file. A user who later changes the root in Preferences keeps that
 * choice — VCommonSettings::initializeDataRoot() reads the settings file first and never
 * comes back here.
 *
 * Setup leaves the value empty when nothing chose a root, which is the signal to use the
 * built-in default.
 *
 * @return absolute path recorded by the installer, in Qt's '/' separator form; empty when
 * no installer recorded one, and always empty off Windows.
 */
QString dataRoot()
{
#ifdef Q_OS_WIN
    return readInstalledPath(QStringLiteral("DataRoot"));
#else
    return QString();
#endif
}

} // namespace InstallerRecord
