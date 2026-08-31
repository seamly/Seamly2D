//-----------------------------------------------------------------------------
//  @file   vcommonsettings.cpp
//  @author Douglas S Caskey
//  @date   17 Sep, 2023
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
//  @file   vcommonsettings.cpp
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date   15 7, 2015
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

#include "vcommonsettings.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QColorDialog>
#include <QFileDialog>
#include <QFont>
#include <QtGlobal>
#include <QLocale>
#include <QMessageLogger>
#include <QStandardPaths>
#include <QString>
#include <QStringConverter>
#include <QTextStream>
#include <QVariant>
#include <QtDebug>

#include <algorithm>

#include "../ifc/ifcdef.h"
#include "../vmisc/def.h"
#include "../vmisc/installer_record.h"
#include "../vmisc/vmath.h"
#include "../vpatterndb/pmsystems.h"

namespace
{
const QString settingPathsDataRoot                       = QStringLiteral("paths/dataRoot");
const QString settingImagesPath                          = QStringLiteral("paths/images");
const QString settingPathsIndividualMeasurements         = QStringLiteral("paths/individual_size_measurements");
const QString settingPathsMultisizeMeasurements          = QStringLiteral("paths/multi_size_measurements");
const QString settingPathsTemplates                      = QStringLiteral("paths/templates");
const QString settingPathsBodyScans                      = QStringLiteral("paths/bodyscans");
const QString settingPathsLabelTemplate                  = QStringLiteral("paths/labels");
const QString settingBackupPath                          = QStringLiteral("paths/backups");

const QString settingConfigurationCompanyName            = QStringLiteral("graphicsview/companyName");
const QString settingConfigurationContact                = QStringLiteral("graphicsview/contact");
const QString settingConfigurationAddress                = QStringLiteral("graphicsview/address");
const QString settingConfigurationCity                   = QStringLiteral("graphicsview/city");
const QString settingConfigurationState                  = QStringLiteral("graphicsview/state");
const QString settingConfigurationZipcode                = QStringLiteral("graphicsview/zipcode");
const QString settingConfigurationCountry                = QStringLiteral("graphicsview/country");
const QString settingConfigurationTelephone              = QStringLiteral("graphicsview/telephone");
const QString settingConfigurationFax                    = QStringLiteral("graphicsview/fax");
const QString settingConfigurationEmail                  = QStringLiteral("graphicsview/email");
const QString settingConfigurationWebsite                = QStringLiteral("graphicsview/website");

const QString settingConfigurationAppTheme               = QStringLiteral("configuration/appTheme");
const QString settingConfigurationShowWelcome            = QStringLiteral("configuration/showWelcome");
const QString settingConfigurationOsSeparator            = QStringLiteral("configuration/osSeparator");

const QString settingConfigurationConvertBackup          = QStringLiteral("configuration/backup/convertBackupEnabled");
const QString settingConfigurationAutosaveState          = QStringLiteral("configuration/autosave/state");
const QString settingConfigurationAutosaveTime           = QStringLiteral("configuration/autosave/time");
const QString settingConfigurationMaxBackups             = QStringLiteral("configuration/autosave/maxBackups");

const QString settingConfigurationUseModeType            = QStringLiteral("configuration/autosave/useModeType");
const QString settingConfigurationUseLastExportFormat    = QStringLiteral("configuration/autosave/useLastExportFormat");
const QString settingConfigurationExportFormat           = QStringLiteral("configuration/autosave/exportFormat");

const QString settingConfigurationSendReportState        = QStringLiteral("configuration/send_report/state");
const QString settingConfigurationLocale                 = QStringLiteral("configuration/locale");
const QString settingPMSystemCode                        = QStringLiteral("configuration/pmscode");
const QString settingConfigurationUnit                   = QStringLiteral("configuration/unit");
const QString settingConfigurationConfirmItemDeletion    = QStringLiteral("configuration/confirm_item_deletion");
const QString settingConfigurationConfirmFormatRewriting = QStringLiteral("configuration/confirm_format_rewriting");
const QString settingConfigurationMoveSuffix             = QStringLiteral("configuration/moveSuffix");
const QString settingConfigurationRotateSuffix           = QStringLiteral("configuration/rotateSuffix");
const QString settingConfigurationMirrorByAxisSuffix     = QStringLiteral("configuration/mirrorByAxisSuffix");
const QString settingConfigurationMirrorByLineSuffix     = QStringLiteral("configuration/mirrorByLineSuffix");

const QString settingGraphicsViewToolBarStyle            = QStringLiteral("graphicsview/tool_bar_style");
const QString settingGraphicsViewShowToolsToolBar        = QStringLiteral("graphicsview/showToolsToolbar");
const QString settingGraphicsViewShowPointToolBar        = QStringLiteral("graphicsview/showPointToolbar");
const QString settingGraphicsViewShowLineToolBar         = QStringLiteral("graphicsview/showLineToolbar");
const QString settingGraphicsViewShowCurveToolBar        = QStringLiteral("graphicsview/showCurveToolbar");
const QString settingGraphicsViewShowArcToolBar          = QStringLiteral("graphicsview/showArcToolbar");
const QString settingGraphicsViewShowOpsToolBar          = QStringLiteral("graphicsview/showOpsToolbar");
const QString settingGraphicsViewShowPieceToolBar        = QStringLiteral("graphicsview/showPieceToolbar");
const QString settingGraphicsViewShowDetailsToolBar      = QStringLiteral("graphicsview/showDetailsToolbar");
const QString settingGraphicsViewShowLayoutToolBar       = QStringLiteral("graphicsview/showLayoutToolbar");
const QString settingGraphicsAutoClearFx                 = QStringLiteral("graphicsview/autoClearFx");

const QString settingGraphicsViewDialogPosition          = QStringLiteral("graphicsview/dialogPosition");
const QString settingGraphicsUseNativeDialogs            = QStringLiteral("graphicsview/useNativeDialogs");
const QString settingGraphicsUseSecondMonitor            = QStringLiteral("graphicsview/useSecondMonitor");
const QString settingGraphicsViewXOffset                 = QStringLiteral("graphicsview/xOffset");
const QString settingGraphicsViewYOffset                 = QStringLiteral("graphicsview/yOffset");

const QString settingGraphicsViewShowScrollBars          = QStringLiteral("graphicsview/showScrollBars");
const QString settingGraphicsViewScrollBarWidth          = QStringLiteral("graphicsview/scrollBarWidth");
const QString settingGraphicsViewScrollDuration          = QStringLiteral("graphicsview/scrollDuration");
const QString settingGraphicsViewScrollUpdateInterval    = QStringLiteral("graphicsview/scrollUpdateInterval");
const QString settingGraphicsViewScrollSpeedFactor       = QStringLiteral("graphicsview/scrollSpeedFactor");
const QString settingGraphicsViewPixelDelta              = QStringLiteral("graphicsview/pixelDelta");
const QString settingGraphicsViewAngleDelta              = QStringLiteral("graphicsview/angleDelta");
const QString settingGraphicsViewZoomModKey              = QStringLiteral("graphicsview/zoomModKey");
const QString settingGraphicsViewZoomDoubleClick         = QStringLiteral("graphicsview/zoomDoubleClick");
const QString settingGraphicsViewPanActiveSpaceKey       = QStringLiteral("graphicsview/panActiveSpaceKey");
const QString settingGraphicsViewUseDefaultPen           = QStringLiteral("graphicsview/useCurrentPen");
const QString settingGraphicsViewShowIsoOnly             = QStringLiteral("graphicsview/showOnlyIso");
const QString settingGraphicsViewZoomSpeedFactor         = QStringLiteral("graphicsview/zoomSpeedFactor");
const QString settingGraphicsViewExportQuality           = QStringLiteral("graphicsview/exportQuality");
const QString settingGraphicsViewBackgroundColor         = QStringLiteral("graphicsview/backgroundColor");
const QString settingGraphicsViewZoomRBPositiveColor     = QStringLiteral("graphicsview/zoomRBPositiveColor");
const QString settingGraphicsViewZoomRBNegativeColor     = QStringLiteral("graphicsview/zoomRBNegativeColor");
const QString settingGraphicsViewPointNameColor          = QStringLiteral("graphicsview/pointNameColor");
const QString settingGraphicsViewPointNameHoverColor     = QStringLiteral("graphicsview/pointNameHoverColor");
const QString settingGraphicsViewAxisOrginColor          = QStringLiteral("graphicsview/axisOrginColor");
const QString settingGraphicsViewDefaultLineColor        = QStringLiteral("graphicsview/defaultLineColor");
const QString settingGraphicsViewDefaultLineWeight       = QStringLiteral("graphicsview/defaultLineWeight");
const QString settingGraphicsViewDefaultLineType         = QStringLiteral("graphicsview/defaultLineType");
const QString settingGraphicsViewPrimaryColor            = QStringLiteral("graphicsview/primarySupportColor");
const QString settingGraphicsViewSecondaryColor          = QStringLiteral("graphicsview/secondarySupportColor");
const QString settingGraphicsViewTertiaryColor           = QStringLiteral("graphicsview/tertiarySupportColor");

const QString settingGraphicsViewConstrainValue          = QStringLiteral("graphicsview/constrainValue");
const QString settingGraphicsViewConstrainModKey         = QStringLiteral("graphicsview/constrainModKey");

const QString settingGraphicsViewPointNameSize           = QStringLiteral("graphicsview/pointNameSize");
const QString settingGraphicsViewGuiFontSize             = QStringLiteral("graphicsview/guiFontSize");
const QString settingGraphicsViewHidePointNames          = QStringLiteral("graphicsview/hidePointNames");
const QString settingGraphicsViewShowAxisOrigin          = QStringLiteral("graphicsview/showAxisOrigin");
const QString settingGraphicsViewWireframe               = QStringLiteral("graphicsview/wireframe");
const QString settingGraphicsViewShowControlPoints       = QStringLiteral("graphicsview/showControlPoints");
const QString settingGraphicsViewShowAnchorPoints        = QStringLiteral("graphicsview/showAnchorPoints");
const QString settingGraphicsUseToolColor                = QStringLiteral("graphicsview/useToolColor");

const QString settingPatternUndo                         = QStringLiteral("pattern/undo");
const QString settingSelectionSound                      = QStringLiteral("pattern/selectionSound");
const QString settingPatternForbidFlipping               = QStringLiteral("pattern/forbidFlipping");
const QString settingPatternHideSeamLine                 = QStringLiteral("pattern/hideMainPath");

const QString settingDefaultNotchLength                  = QStringLiteral("pattern/defaultNotchLength");
const QString settingDefaultNotchWidth                   = QStringLiteral("pattern/defaultNotchWidth");
const QString settingDefaultNotchType                    = QStringLiteral("pattern/defaultNotchType");
const QString settingDefaultNotchColor                   = QStringLiteral("pattern/defaultNotchColor");
const QString settingSeamlineNotch                       = QStringLiteral("pattern/doubleNotch");
const QString settingSeamAllowanceNotch                  = QStringLiteral("pattern/showSeamAllowanceNotch");

const QString settingPatternDefaultSeamAllowance         = QStringLiteral("pattern/defaultSeamAllowance");
const QString settingDefaultSeamColor                    = QStringLiteral("pattern/defaultSeamColor");
const QString settingDefaultSeamLinetype                 = QStringLiteral("pattern/defaultSeamLinetype");
const QString settingDefaultSeamLineweight               = QStringLiteral("pattern/defaultSeamLineweight");
const QString settingDefaultCutColor                     = QStringLiteral("pattern/defaultCutColor");
const QString settingDefaultCutLinetype                  = QStringLiteral("pattern/defaultCutLinetype");
const QString settingDefaultCutLineweight                = QStringLiteral("pattern/defaultCutLineweight");
const QString settingDefaultInternalColor                = QStringLiteral("pattern/defaultInternalColor");
const QString settingDefaultInternalLinetype             = QStringLiteral("pattern/defaultInternalLinetype");
const QString settingDefaultInternalLineweight           = QStringLiteral("pattern/defaultInternalLineweight");
const QString settingDefaultCutoutColor                  = QStringLiteral("pattern/defaultCutoutColor");
const QString settingDefaultCutoutLinetype               = QStringLiteral("pattern/defaultCutoutLinetype");
const QString settingDefaultCutoutLineweight             = QStringLiteral("pattern/defaultCutoutLineweight");

const QString settingShowSeamAllowances                  = QStringLiteral("pattern/showShowSeamAllowances");
const QString settingDefaultSeamAllowanceVisibilty       = QStringLiteral("pattern/defaultSeamAllowanceVisibilty");
const QString settingShowGrainlines                      = QStringLiteral("pattern/showGrainlines");
const QString settingDefaultGrainlineVisibilty           = QStringLiteral("pattern/defaultGrainlineVisibilty");
const QString settingDefaultGrainlineLength              = QStringLiteral("pattern/defaultGrainlineLength");
const QString settingDefaultGrainlineColor               = QStringLiteral("pattern/defaultGrainlineColor");
const QString settingDefaultGrainlineLineweight          = QStringLiteral("pattern/defaultGrainlineLineweight");
const QString settingDefaultArrowLength                  = QStringLiteral("pattern/defaultArrowLength");

const QString settingShowLabels                          = QStringLiteral("pattern/showLabels");
const QString settingShowPatternLabels                   = QStringLiteral("pattern/showPatternLabels");
const QString settingShowPieceLabels                     = QStringLiteral("pattern/showPieceLabels");
const QString settingDefaultLabelWidth                   = QStringLiteral("pattern/defaultLabelWidth");
const QString settingDefaultLabelHeight                  = QStringLiteral("pattern/defaultLabelHeight");
const QString settingDefaultLabelColor                   = QStringLiteral("pattern/defaultLabelColor");
const QString settingDefaultPatternTemplate              = QStringLiteral("pattern/defaultPatternTemplate");
const QString settingDefaultPieceTemplate                = QStringLiteral("pattern/defaultPieceTemplate");

const QString settingPatternLabelFont                    = QStringLiteral("pattern/labelFont");
const QString settingPatternGuiFont                      = QStringLiteral("pattern/guiFont");
const QString settingPatternPointNameFont                = QStringLiteral("pattern/pointNameFont");

const QString settingGeneralRecentFileList               = QStringLiteral("recentFileList");
const QString settingGeneralRestoreFileList              = QStringLiteral("restoreFileList");
const QString settingGeneralGeometry                     = QStringLiteral("geometry");
const QString settingGeneralWindowState                  = QStringLiteral("windowState");
const QString settingGeneralToolbarsState                = QStringLiteral("toolbarsState");
const QString settingPreferenceDialogSize                = QStringLiteral("preferenceDialogSize");
const QString settingToolSeamAllowanceDialogSize         = QStringLiteral("toolSeamAllowanceDialogSize");
const QString settingVariablesDialogSize                 = QStringLiteral("toolVariablesDialogSize");
const QString settingHistoryDialogSize                   = QStringLiteral("toolHistoryDialogSize");
const QString settingFormulaWizardDialogSize             = QStringLiteral("formulaWizardDialogSize");
const QString settingLatestSkippedVersion                = QStringLiteral("lastestSkippedVersion");
const QString settingDateOfLastRemind                    = QStringLiteral("dateOfLastRemind");

const QString settingCSVWithHeader                       = QStringLiteral("csv/withHeader");
const QString settingCSVCodec                            = QStringLiteral("csv/withCodec");
const QString settingCSVSeparator                        = QStringLiteral("csv/withSeparator");

const QString settingLabelDateFormat                     = QStringLiteral("label/dateFormat");
const QString settingLabelUserDateFormats                = QStringLiteral("label/userDateFormats");
const QString settingLabelTimeFormat                     = QStringLiteral("label/timeFormat");
const QString settingLabelUserTimeFormats                = QStringLiteral("label/userTimeFormats");

int pointNameSize = 0;

//---------------------------------------------------------------------------------------------------------------------
QStringList ClearFormats(const QStringList &predefinedFormats, QStringList formats)
{
    for (int i = 0; i < predefinedFormats.size(); ++i)
    {
        formats.removeAll(predefinedFormats.at(i));
    }
    return formats;
}
}

static const QString commonIniFilename = QStringLiteral("qt6_common");

#if !defined(Q_OS_WIN)
const QString VCommonSettings::unixStandardSharePath = QStringLiteral("/usr/share/seamly2d");
#endif

namespace
{
//---------------------------------------------------------------------------------------------------------------------
void SymlinkCopyDirRecursive(const QString &fromDir, const QString &toDir, bool replaceOnConflit)
{
    QDir dir;
    dir.setPath(fromDir);

    foreach (QString copyFile, dir.entryList(QDir::Files))
    {
        const QString from = fromDir + QDir::separator() + copyFile;
        QString to = toDir + QDir::separator() + copyFile;

#ifdef Q_OS_WIN
        {
            // To fix issue #702 check each not symlink if it is actually broken symlink.
            // Also trying to mimic Unix symlink. If a file eaxists do not create a symlink and remove it if exists.
            QFile fileTo(to);
            if (fileTo.exists())
            {
                if (not fileTo.rename(to + QLatin1String(".lnk")))
                {
                    QFile::remove(to + QLatin1String(".lnk"));
                    fileTo.rename(to + QLatin1String(".lnk"));
                }

                QFileInfo info(to + QLatin1String(".lnk"));
                if (info.symLinkTarget().isEmpty())
                {
                    fileTo.copy(to);
                    fileTo.remove();
                    continue; // The file already exists, skip creating shortcut
                }
            }
        }

        to = to + QLatin1String(".lnk");
#endif

        if (QFile::exists(to))
        {
            if (replaceOnConflit)
            {
                QFile::remove(to);
            }
            else
            {
                continue;
            }
        }

        QFile::link(from, to);
    }

    foreach (QString copyDir, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        const QString from = fromDir + QDir::separator() + copyDir;
        const QString to = toDir + QDir::separator() + copyDir;

        if (dir.mkpath(to) == false)
        {
            return;
        }

        SymlinkCopyDirRecursive(from, to, replaceOnConflit);
    }
}

//---------------------------------------------------------------------------------------------------------------------
QString PrepareStandardFiles(const QString &currentPath, const QString &standardPath, const QString &defPath)
{
    QDir standardPathDir(standardPath);
    QDir currentPathDir(currentPath);
    if ((currentPath == defPath || not currentPathDir.exists()) && standardPathDir.exists())
    {
        const QDir localdata (defPath);
        if (localdata.mkpath("."))
        {
            SymlinkCopyDirRecursive(standardPath, defPath, false);
        }
        return defPath;
    }
    return currentPath;
}
}

//---------------------------------------------------------------------------------------------------------------------
VCommonSettings::VCommonSettings(Format format, Scope scope, const QString &organization,
                            const QString &application, QObject *parent)
    :QSettings(format, scope, organization, application, parent)
{}

//---------------------------------------------------------------------------------------------------------------------
VCommonSettings::VCommonSettings(const QString &fileName, QSettings::Format format, QObject *parent)
     : QSettings(fileName, format, parent)
{}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::SharePath(const QString &shareItem)
{
#ifdef Q_OS_WIN
    return QCoreApplication::applicationDirPath() + shareItem;
#elif defined(Q_OS_MAC)
    QDir dirBundle(QCoreApplication::applicationDirPath() + QStringLiteral("/../Resources") + shareItem);
    if (dirBundle.exists())
    {
        return dirBundle.absolutePath();
    }
    else
    {
        QDir appDir = QDir(qApp->applicationDirPath());
        appDir.cdUp();
        appDir.cdUp();
        appDir.cdUp();
        QDir dir(appDir.absolutePath() + shareItem);
        if (dir.exists())
        {
            return dir.absolutePath();
        }
        else
        {
            return VCommonSettings::unixStandardSharePath + shareItem;
        }
    }
#else // Unix
#ifdef QT_DEBUG
    return QCoreApplication::applicationDirPath() + shareItem;
#else
    QDir dir(QCoreApplication::applicationDirPath() + shareItem);
    if (dir.exists())
    {
        return dir.absolutePath();
    }
    else
    {
        return VCommonSettings::unixStandardSharePath + shareItem;
    }
#endif
#endif
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::MultisizeTablesPath()
{
    return SharePath(QStringLiteral("/tables/multisize"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::StandardTemplatesPath()
{
    return SharePath(QStringLiteral("/tables/templates"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::PrepareStandardTemplates(const QString & currentPath)
{
    return PrepareStandardFiles(currentPath, StandardTemplatesPath(), getDefaultTemplatePath());
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::prepareMultisizeTables(const QString &currentPath)
{
    return PrepareStandardFiles(currentPath, MultisizeTablesPath(), getDefaultMultisizePath());
}

namespace
{
/** Test-only base-directory override for the common settings file; empty in production. */
QString commonSettingsBaseDirOverride;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief readDataRoot reads the configured user-data root out of the shared "common"
 * settings file, falling back to the built-in default when nothing has been configured.
 *
 * @return absolute path of the user-data root, in Qt's '/' separator form; never empty.
 */
QString readDataRoot()
{
    const QSettings settings(VCommonSettings::commonSettingsFilePath(), QSettings::IniFormat);
    const QString configured = settings.value(settingPathsDataRoot).toString().trimmed();

    return configured.isEmpty() ? VCommonSettings::getDefaultDataRoot()
                                : QDir::cleanPath(QDir::fromNativeSeparators(configured));
}
} // anonymous namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief commonSettingsFilePath returns the absolute path of the shared qt6_common.ini.
 *
 * The file lives under GenericConfigLocation — %LOCALAPPDATA% on Windows, ~/.config on
 * Linux, ~/Library/Preferences on macOS — so the change from Qt's IniFormat/UserScope
 * default (%APPDATA%, Roaming) affects Windows only. Local, not Roaming, because the
 * file's paths/* values are absolute machine paths: a roaming Windows profile would
 * carry them to a machine where they are wrong. Task SettingsFiles.1.
 *
 * Every application sets "Seamly", so all of them resolve one file.
 *
 * @return absolute path, e.g. C:/Users/<user>/AppData/Local/Seamly/qt6_common.ini.
 */
QString VCommonSettings::commonSettingsFilePath()
{
    QString base = commonSettingsBaseDirOverride;
    if (base.isEmpty())
    {
        base = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    }

    QString organization = QCoreApplication::organizationName();
    if (organization.isEmpty())
    {
        organization = QStringLiteral("Seamly");
    }

    return base + QLatin1Char('/') + organization + QLatin1Char('/') + commonIniFilename
           + QLatin1String(".ini");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setCommonSettingsBaseDir redirects the common settings file for the test suite.
 *
 * The tests must never read or write the developer's real per-user configuration, and
 * QStandardPaths::GenericConfigLocation cannot be redirected per test. Production code
 * must not call this.
 *
 * @param baseDir directory to resolve qt6_common.ini under; empty restores the platform
 * location.
 */
void VCommonSettings::setCommonSettingsBaseDir(const QString &baseDir)
{
    commonSettingsBaseDirOverride = baseDir;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief migrateCommonSettingsLocation brings an existing common settings file forward
 * into commonSettingsFilePath().
 *
 * Candidate sources, newest first; the first one that exists is copied:
 *
 *  1. the pre-move file under Qt's IniFormat/UserScope default (%APPDATA%\Seamly on
 *     Windows — identical to the target on Linux and macOS, so skipped there);
 *  2. the pre-Task-15 "Seamly2DTeam" organization's qt6_common.ini;
 *  3. the qt5-era common.ini from either of those folders.
 *
 * Copy-if-missing and re-entrant: once the target exists, every later call is a cheap
 * no-op, and no source file is ever modified or deleted — a rollback to an earlier
 * release keeps working. Called from each application's openSettings() before any
 * settings value is read.
 *
 * @return absolute path of the common settings file at its current location.
 */
QString VCommonSettings::migrateCommonSettingsLocation()
{
    const QString target = commonSettingsFilePath();
    QDir().mkpath(QFileInfo(target).absolutePath());
    if (QFileInfo::exists(target))
    {
        return target;
    }

    // Qt's own IniFormat/UserScope resolution names the pre-move folder, so a
    // QSettings::setPath() redirection (the test suite) is honoured here too.
    const QSettings roamingProbe(QSettings::IniFormat, QSettings::UserScope,
                                 QCoreApplication::organizationName(), commonIniFilename);
    const QString roamingDir = QFileInfo(roamingProbe.fileName()).absolutePath();

    static const QString kLegacyOrganizationName = QStringLiteral("Seamly2DTeam");
    const QSettings legacyProbe(QSettings::IniFormat, QSettings::UserScope,
                                kLegacyOrganizationName, commonIniFilename);
    const QString legacyDir = QFileInfo(legacyProbe.fileName()).absolutePath();

    const QStringList candidates
    {
        roamingDir + QLatin1String("/qt6_common.ini"),
        legacyDir + QLatin1String("/qt6_common.ini"),
        roamingDir + QLatin1String("/common.ini"),
        legacyDir + QLatin1String("/common.ini")
    };

    for (const QString &candidate : candidates)
    {
        if (candidate != target && QFileInfo::exists(candidate))
        {
            QFile::copy(candidate, target);
            break;
        }
    }

    return target;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getDefaultDataRoot returns the built-in default root of the user's data tree.
 *
 * Task 60 moved this to <Documents>/Seamly. Two changes, for two reasons:
 *
 *  - **Documents, not the home directory.** These are files the user creates, opens, saves
 *    and backs up, so they belong where every other application puts documents. Internal
 *    state — settings, caches, logs — stays in the platform's application-data locations
 *    and is deliberately NOT mixed in here.
 *  - **"Seamly", not "seamlyData" or "seamly2d".** The folder holds the whole suite's
 *    work, so naming it after one member (seamly2d) wrongly implies SeamlyMe and
 *    SeamlyLayout belong to Seamly2D, and "Data" is redundant once the parent location
 *    already says what these files are.
 *
 * QStandardPaths::DocumentsLocation is used rather than a hand-built path because it
 * resolves the Windows known-folder API (so a redirected or OneDrive-backed Documents is
 * honoured) and XDG_DOCUMENTS_DIR on Linux, where a localized system may not call the
 * folder "Documents" at all. It falls back to the home directory on the rare system that
 * reports no documents location, which keeps the result absolute in every case.
 *
 * @return absolute path of the default user-data root, e.g. C:/Users/<user>/Documents/Seamly.
 */
QString VCommonSettings::getDefaultDataRoot()
{
    QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documents.isEmpty())
    {
        documents = QDir::homePath();
    }
    return QDir::cleanPath(documents) + QLatin1String("/Seamly");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getLegacyDataRoot returns the pre-Task-34 default root of the user's data tree.
 *
 * Kept so first-run resolution can spot an existing installation's data and adopt it
 * instead of stranding the user's patterns and measurements at the old location.
 *
 * @return absolute path of the legacy user-data root, e.g. C:/Users/<user>/seamly2d.
 */
QString VCommonSettings::getLegacyDataRoot()
{
    return QDir::homePath() + QLatin1String("/seamly2d");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief dataRoot returns the currently configured root of the user's data tree.
 *
 * Static so the getDefault*Path() family — which is static and called from contexts with no
 * settings object to hand — can derive from it. The value lives in the shared common
 * settings file, so seamly2d, seamlyme and the installer all agree on one root.
 *
 * @return absolute path of the user-data root; the built-in default when unconfigured.
 */
QString VCommonSettings::dataRoot()
{
    return readDataRoot();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief dataSubdirPath builds the path of one subfolder of the user's data tree.
 * @param subdirectory subfolder name relative to the data root, e.g. tr("templates").
 * @return absolute path of that subfolder underneath the configured data root.
 */
QString VCommonSettings::dataSubdirPath(const QString &subdirectory)
{
    return dataRoot() + QLatin1String("/") + subdirectory;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getDataRoot returns the configured user-data root for this settings object.
 * @return absolute path of the user-data root; the built-in default when unconfigured.
 */
QString VCommonSettings::getDataRoot() const
{
    return readDataRoot();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setDataRoot stores a new root for the user's data tree and creates its folders.
 *
 * The root may be any drive, volume or path the user can write to — an external disk or a
 * cloud-synced folder such as G:/My Drive/seamlyData — so the whole data tree can be
 * relocated without moving files by hand. Existing files at the new location are never
 * touched; only missing folders are created.
 *
 * @param value new user-data root, in either native or '/' separator form.
 */
void VCommonSettings::setDataRoot(const QString &value)
{
    const QString root = QDir::cleanPath(QDir::fromNativeSeparators(value.trimmed()));

    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(settingPathsDataRoot, root);
    settings.sync();

    ensureDataRootTree(root);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ensureDataRootTree creates the data root and its standard subfolders if missing.
 *
 * Purely additive: QDir::mkpath() leaves any directory that already exists alone, so an
 * existing data tree — including one adopted from the legacy ~/seamly2d location — keeps
 * every file it holds. A root on a disconnected or read-only volume simply fails to be
 * created; the caller carries on, exactly as the app already does for a missing folder.
 *
 * @param root data root to populate; the configured root when empty.
 * @return true if the root directory exists (or was created) afterwards.
 */
bool VCommonSettings::ensureDataRootTree(const QString &root)
{
    const QString target = root.isEmpty() ? dataRoot() : root;

    QDir directory(target);
    if (!directory.mkpath(QStringLiteral(".")))
    {
        qWarning() << "Could not create the Seamly data root" << QDir::toNativeSeparators(target);
        return false;
    }

    // The nine standard subfolders, named exactly as the getDefault*Path() family below
    // spells them so a folder is never created twice under two different names.
    const QStringList subdirectories
    {
        tr("measurements") + QLatin1String("/") + tr("individual"),
        tr("measurements") + QLatin1String("/") + tr("multisize"),
        tr("templates"),
        tr("bodyscans"),
        tr("label templates"),
        tr("images"),
        tr("backups"),
        tr("patterns"),
        tr("layouts")
    };

    for (const QString &subdirectory : subdirectories)
    {
        directory.mkpath(subdirectory);
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief migrationMarkerFileName names the breadcrumb left in a tree that has been migrated.
 */
static const QString migrationMarkerFileName = QStringLiteral("MIGRATED-TO-SEAMLY.txt");

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief dataTreeWasMigrated reports whether a legacy tree already carries the marker.
 *
 * @param root tree to test.
 * @return true when the marker file is present.
 */
bool VCommonSettings::dataTreeWasMigrated(const QString &root)
{
    if (root.isEmpty())
    {
        return false;
    }
    return QFileInfo::exists(root + QLatin1Char('/') + migrationMarkerFileName);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief migrateDataTree copies a whole user-data tree to a new root (Task 60).
 *
 * Copies EVERY file and directory found under sourceRoot, not a known list of subfolders.
 * That is deliberate and is the single most important property of this function: users add
 * their own directories to the data tree — Projects, bodyscans and others have been seen in
 * the wild — so migrating a fixed list would silently strand whatever the list did not
 * mention. The structure is reproduced exactly; only the root's name changes.
 *
 * The safety rules, each of which exists because the alternative loses data:
 *
 *  - **Never a rename or a move.** The source tree is left completely intact so a user can
 *    roll back to an earlier release, which is why the caller can also mark it rather than
 *    delete it. Nothing here removes anything, ever.
 *  - **Merge, never overwrite.** An existing destination file is skipped and counted, not
 *    clobbered — the destination may be an already-populated folder.
 *  - **Verify every copy.** Sizes are compared after each file, because a cloud-synced
 *    target (Google Drive, OneDrive, Dropbox) can report a write complete before it is
 *    durable. A file that does not verify aborts the migration.
 *  - **Fail safe.** On any error the function stops, removes only the partial file it was
 *    writing at that moment, and returns false with the source untouched. A half-copied
 *    destination must never become the configured root, so the caller must not record the
 *    new root unless this returned true.
 *
 * @param sourceRoot      tree to copy from; must exist.
 * @param destinationRoot tree to copy to; created if missing.
 * @param filesCopied     optional out-parameter, number of files actually copied.
 * @param filesSkipped    optional out-parameter, number already present at the destination.
 * @param errorMessage    optional out-parameter, human-readable reason for a false return.
 * @return true when every file is present and verified at the destination.
 */
bool VCommonSettings::migrateDataTree(const QString &sourceRoot, const QString &destinationRoot,
                                      int *filesCopied, int *filesSkipped, QString *errorMessage)
{
    const auto fail = [errorMessage](const QString &reason)
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = reason;
        }
        qWarning() << "Data-tree migration failed:" << reason;
        return false;
    };

    if (filesCopied != nullptr)  { *filesCopied = 0; }
    if (filesSkipped != nullptr) { *filesSkipped = 0; }
    if (errorMessage != nullptr) { errorMessage->clear(); }

    const QString source = QDir::cleanPath(QDir::fromNativeSeparators(sourceRoot.trimmed()));
    const QString destination = QDir::cleanPath(QDir::fromNativeSeparators(destinationRoot.trimmed()));

    if (source.isEmpty() || destination.isEmpty())
    {
        return fail(QStringLiteral("source or destination path is empty"));
    }
    if (!QFileInfo(source).isDir())
    {
        return fail(QStringLiteral("source '%1' is not a directory").arg(source));
    }
#ifdef Q_OS_WIN
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
#endif
    if (source.compare(destination, caseSensitivity) == 0)
    {
        return fail(QStringLiteral("source and destination are the same directory"));
    }
    // Copying a tree into its own subdirectory would recurse without end.
    if (destination.startsWith(source + QLatin1Char('/'), caseSensitivity))
    {
        return fail(QStringLiteral("destination '%1' lies inside the source tree").arg(destination));
    }

    QDir destinationDir(destination);
    if (!destinationDir.mkpath(QStringLiteral(".")))
    {
        return fail(QStringLiteral("could not create '%1'").arg(destination));
    }

    const QDir sourceDir(source);
    int copied = 0;
    int skipped = 0;

    QDirIterator iterator(source, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext())
    {
        const QString entryPath = iterator.next();
        const QFileInfo entry(entryPath);
        const QString relative = sourceDir.relativeFilePath(entryPath);
        const QString target = destination + QLatin1Char('/') + relative;

        if (entry.isDir())
        {
            if (!destinationDir.mkpath(relative))
            {
                return fail(QStringLiteral("could not create '%1'").arg(target));
            }
            continue;
        }

        if (QFileInfo::exists(target))
        {
            // Merge, never overwrite. Reported so a collision is visible rather than silent.
            ++skipped;
            qDebug() << "Data-tree migration skipped existing file" << QDir::toNativeSeparators(target);
            continue;
        }

        // The parent may not exist yet: QDirIterator does not guarantee a directory is
        // visited before the files inside it.
        const QString targetParent = QFileInfo(target).absolutePath();
        if (!QDir().mkpath(targetParent))
        {
            return fail(QStringLiteral("could not create '%1'").arg(targetParent));
        }

        if (!QFile::copy(entryPath, target))
        {
            return fail(QStringLiteral("could not copy '%1' to '%2'").arg(entryPath, target));
        }

        // Verify, because a cloud-synced destination can report success early.
        if (QFileInfo(target).size() != entry.size())
        {
            QFile::remove(target);
            return fail(QStringLiteral("copy of '%1' did not verify (expected %2 bytes)")
                            .arg(entryPath)
                            .arg(entry.size()));
        }
        ++copied;
    }

    if (filesCopied != nullptr)  { *filesCopied = copied; }
    if (filesSkipped != nullptr) { *filesSkipped = skipped; }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief migrateAdoptedLegacyTree turns a first-run adoption into a Task 60 migration.
 *
 * initializeDataRoot() still *adopts* a legacy tree — it resolves and records a path and
 * touches no files, which is what keeps it safe for the unit tests to call. This function
 * is the second half, and it is deliberately called only from the applications'
 * openSettings(), the one place the real home directory is fed in. The tests therefore
 * cannot copy anything into the developer's home no matter what they resolve, which is the
 * same rule pruneEmptyLegacyDataRoot() and ensureDataRootTree() follow.
 *
 * Fail-safe by construction: the configured root is only repointed at newRoot after the
 * copy has completed and verified. If anything goes wrong the legacy tree stays configured
 * and in use, so the worst case is that the user carries on exactly as before.
 *
 * @param legacyRoot the adopted tree, e.g. ~/seamly2d.
 * @param newRoot    where it should live now, e.g. <Documents>/Seamly.
 * @return the root actually in force afterwards — newRoot on success, legacyRoot on failure.
 */
QString VCommonSettings::migrateAdoptedLegacyTree(const QString &legacyRoot, const QString &newRoot)
{
    if (legacyRoot.isEmpty() || newRoot.isEmpty() || !QFileInfo(legacyRoot).isDir())
    {
        return legacyRoot;
    }

    // Already dealt with on an earlier run: leave the marked tree alone.
    if (dataTreeWasMigrated(legacyRoot))
    {
        return legacyRoot;
    }

    int copied = 0;
    int skipped = 0;
    QString errorMessage;
    if (!migrateDataTree(legacyRoot, newRoot, &copied, &skipped, &errorMessage))
    {
        qWarning() << "Keeping the existing data root" << QDir::toNativeSeparators(legacyRoot)
                   << "because migration failed:" << errorMessage;
        return legacyRoot;
    }

    qInfo() << "Migrated the user-data tree from" << QDir::toNativeSeparators(legacyRoot) << "to"
            << QDir::toNativeSeparators(newRoot) << '-' << copied << "file(s) copied," << skipped
            << "already present";

    // Only now is it safe to repoint the configured root.
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(settingPathsDataRoot, newRoot);
    settings.sync();

    markDataTreeMigrated(legacyRoot, newRoot);
    return newRoot;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief markDataTreeMigrated writes the breadcrumb that retires a migrated legacy tree.
 *
 * The legacy tree is deliberately kept — a user may need to roll back to an earlier release
 * — so it needs to be obvious to both the code and a human that it is no longer live. The
 * marker stops initializeDataRoot() offering the same tree again on the next run, and its
 * contents tell a person opening the folder where their files went and when.
 *
 * A failure here is not fatal to the migration that preceded it: the files are already
 * copied and verified. It is reported and ignored.
 *
 * @param legacyRoot tree that was migrated away from.
 * @param newRoot    where its contents now live.
 * @return true when the marker was written.
 */
bool VCommonSettings::markDataTreeMigrated(const QString &legacyRoot, const QString &newRoot)
{
    if (legacyRoot.isEmpty() || !QFileInfo(legacyRoot).isDir())
    {
        return false;
    }

    QFile marker(legacyRoot + QLatin1Char('/') + migrationMarkerFileName);
    if (!marker.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Could not write the migration marker in" << QDir::toNativeSeparators(legacyRoot);
        return false;
    }

    QTextStream stream(&marker);
    stream << "This folder has been migrated and is no longer used by the Seamly "
              "applications.\r\n\r\n"
           << "Your files were copied to:\r\n    " << QDir::toNativeSeparators(newRoot) << "\r\n\r\n"
           << "Date: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\r\n\r\n"
           << "Nothing here was deleted. Once you are satisfied that everything is present "
              "at the new location, this folder can be removed.\r\n";
    marker.close();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief rebaseOntoDataRoot follows a path from an old data root into a new one.
 *
 * Preferences → Paths writes every row back as an explicit absolute override, so without
 * this a user who repoints the data root would leave all nine subfolders pinned to the old
 * location and the move would achieve nothing. A path that lies outside the old root is a
 * deliberate choice of its own and is returned unchanged.
 *
 * @param path path to relocate.
 * @param oldRoot data root the path was derived from.
 * @param newRoot data root to move it under.
 * @return the path relocated under newRoot, or unchanged when it was not inside oldRoot.
 */
QString VCommonSettings::rebaseOntoDataRoot(const QString &path, const QString &oldRoot, const QString &newRoot)
{
    // Windows path comparison is case-insensitive; POSIX filesystems are not.
#ifdef Q_OS_WIN
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
#endif

    const QString cleanOld = QDir::cleanPath(QDir::fromNativeSeparators(oldRoot.trimmed()));
    const QString cleanNew = QDir::cleanPath(QDir::fromNativeSeparators(newRoot.trimmed()));

    if (cleanOld.isEmpty() || cleanNew.isEmpty() || cleanOld.compare(cleanNew, caseSensitivity) == 0)
    {
        return path;
    }

    const QString cleanPath = QDir::cleanPath(QDir::fromNativeSeparators(path.trimmed()));
    if (cleanPath.compare(cleanOld, caseSensitivity) == 0)
    {
        return cleanNew;
    }

    if (cleanPath.startsWith(cleanOld + QLatin1Char('/'), caseSensitivity))
    {
        return cleanNew + QLatin1Char('/') + cleanPath.mid(cleanOld.length() + 1);
    }

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief initializeDataRoot resolves the user-data root once, at application start-up.
 *
 * Called before any data path is read, from every application's openSettings(). Four cases:
 *
 *  1. A root is already configured — honour it untouched.
 *  2. Nothing configured, and the Windows installer recorded one — adopt it. The user
 *     chose that folder on Setup's "Where do you keep your work?" page and was told the
 *     apps would use it, so it outranks every default below.
 *  3. Nothing configured or recorded, and a populated legacy ~/seamly2d tree exists while
 *     the default root does not — adopt the legacy tree *in place* as the root. Adoption
 *     rather than copying is deliberate: an upgrading user's patterns and measurements can
 *     be many gigabytes and may sit on a cloud-synced drive, so nothing is moved, copied
 *     or deleted and the data keeps working from the moment the app starts.
 *  4. Otherwise — a fresh install — use the built-in default.
 *
 * The resolved root is written back so later runs take case 1 and the value is visible to
 * the other applications and to Preferences → Paths.
 *
 * Cases 2–4 are DEPRECATED first-run seeding (Task SettingsFiles.3, 2026-08-31). The
 * Windows MSI seeds paths/dataRoot at install time (smsi_seed_user_settings.ps1), so an
 * installed Windows machine takes case 1. The fallbacks stay only for packages with no
 * install hook — the macOS dmg, the Linux AppImage, dev builds — and for other Windows
 * accounts on a shared machine. Remove them when those packages gain install-time seeding.
 *
 * @param adoptedLegacyTree optional out-parameter, set to true when case 3 applied; pass
 * null when the caller does not care.
 * @return absolute path of the resolved user-data root.
 */
QString VCommonSettings::initializeDataRoot(bool *adoptedLegacyTree)
{
    if (adoptedLegacyTree != nullptr)
    {
        *adoptedLegacyTree = false;
    }

    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);

    const QString configured = settings.value(settingPathsDataRoot).toString().trimmed();
    if (!configured.isEmpty())
    {
        // Case 1: already chosen, by an earlier run or by the user.
        return QDir::cleanPath(QDir::fromNativeSeparators(configured));
    }

    // Case 2: the Windows installer recorded the folder the user chose. It outranks both the
    // legacy tree and the built-in default, because the user was shown that path and told the
    // apps would use it. Recorded here, so later runs take case 1 and a change made in
    // Preferences is never overridden by the installer.
    const QString fromInstaller = InstallerRecord::dataRoot();
    if (!fromInstaller.isEmpty())
    {
        settings.setValue(settingPathsDataRoot, fromInstaller);
        settings.sync();
        return fromInstaller;
    }

    const QString resolved = chooseFirstRunDataRoot(getDefaultDataRoot(), getLegacyDataRoot(), adoptedLegacyTree);

    settings.setValue(settingPathsDataRoot, resolved);
    settings.sync();

    return resolved;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief chooseFirstRunDataRoot picks between the new and the legacy data root.
 *
 * Split out of initializeDataRoot() so the decision can be exercised against throwaway
 * directories: it takes both candidate roots as arguments and reads no settings and no
 * home directory of its own. Nothing here creates, moves or deletes anything — the choice
 * is a settings value, and an adopted legacy tree stays exactly where it is.
 *
 * @param defaultRoot the built-in default root, normally ~/seamlyData.
 * @param legacyRoot the pre-Task-34 root, normally ~/seamly2d.
 * @param adoptedLegacyTree optional out-parameter, set to true when the legacy tree was
 * adopted; pass null when the caller does not care.
 * @return legacyRoot when it is an existing directory and defaultRoot does not exist yet,
 * otherwise defaultRoot.
 */
QString VCommonSettings::chooseFirstRunDataRoot(const QString &defaultRoot, const QString &legacyRoot,
                                                bool *adoptedLegacyTree)
{
    if (adoptedLegacyTree != nullptr)
    {
        *adoptedLegacyTree = false;
    }

    if (!QFileInfo::exists(defaultRoot) && QFileInfo(legacyRoot).isDir())
    {
        // Upgrading from a build that hard-coded ~/seamly2d: adopt that tree in place.
        if (adoptedLegacyTree != nullptr)
        {
            *adoptedLegacyTree = true;
        }
        return legacyRoot;
    }

    return defaultRoot;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief pruneEmptyLegacyDataRoot removes the abandoned legacy data root when it holds no files.
 *
 * Renaming the default root leaves the old ~/seamly2d behind, and ensureDataRootTree() will
 * have stocked it with the nine standard subfolders, so what remains after the move is an
 * empty skeleton that looks like data but is not. This deletes that skeleton.
 *
 * Two conditions gate it, and both matter:
 *
 *  - the legacy root must not be the configured root. Task 34's first-run rule *adopts* an
 *    existing ~/seamly2d in place, so for an upgrading user that directory is the live data
 *    tree. Deleting it there would destroy exactly the patterns adoption set out to preserve.
 *  - the tree must contain no files at any depth. One stray file and nothing is removed.
 *
 * Only empty directories are then removed, deepest first, via QDir::rmdir() — which cannot
 * delete a file and refuses a non-empty directory. removeRecursively() is never used: this
 * function must not be capable of deleting anything it has not counted.
 *
 * @param legacyRoot the legacy root to prune, normally getLegacyDataRoot().
 * @param configuredRoot the data root actually in use; pruning is skipped when they match.
 * @return true when the legacy root was removed, false when it was kept for any reason.
 */
bool VCommonSettings::pruneEmptyLegacyDataRoot(const QString &legacyRoot, const QString &configuredRoot)
{
    // Windows path comparison is case-insensitive; POSIX filesystems are not.
#ifdef Q_OS_WIN
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
#endif

    const QString legacy     = QDir::cleanPath(QDir::fromNativeSeparators(legacyRoot.trimmed()));
    const QString configured = QDir::cleanPath(QDir::fromNativeSeparators(configuredRoot.trimmed()));

    if (legacy.isEmpty() || !QFileInfo(legacy).isDir())
    {
        return false;
    }

    // The live data tree of an upgrading user — never touch it.
    if (legacy.compare(configured, caseSensitivity) == 0)
    {
        return false;
    }

    // A configured root *inside* the legacy root (e.g. ~/seamly2d/patterns) would be taken
    // down with its parent, so treat that as occupied too.
    if (configured.startsWith(legacy + QLatin1Char('/'), caseSensitivity))
    {
        return false;
    }

    QDirIterator files(legacy, QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                       QDirIterator::Subdirectories);
    if (files.hasNext())
    {
        return false;
    }

    // Deepest first, so each rmdir() sees an already-emptied directory.
    QStringList directories;
    QDirIterator subdirectories(legacy, QDir::Dirs | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                                QDirIterator::Subdirectories);
    while (subdirectories.hasNext())
    {
        directories.append(subdirectories.next());
    }

    std::sort(directories.begin(), directories.end(),
              [](const QString &first, const QString &second) { return first.length() > second.length(); });

    for (const QString &directory : qAsConst(directories))
    {
        QDir().rmdir(directory);
    }

    if (!QDir().rmdir(legacy))
    {
        qWarning() << "Could not remove the empty legacy data root" << QDir::toNativeSeparators(legacy);
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultIndividualSizePath()
{
    return dataSubdirPath(tr("measurements") + QLatin1String("/") + tr("individual"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getIndividualSizePath() const
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    return settings.value(settingPathsIndividualMeasurements, getDefaultIndividualSizePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setIndividualSizePath(const QString &value)
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(settingPathsIndividualMeasurements, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultMultisizePath()
{
    return dataSubdirPath(tr("measurements") + QLatin1String("/") + tr("multisize"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getMultisizePath() const
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    return settings.value(settingPathsMultisizeMeasurements, getDefaultMultisizePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setMultisizePath(const QString &value)
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(settingPathsMultisizeMeasurements, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultTemplatePath()
{
    return dataSubdirPath(tr("templates"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getTemplatePath() const
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    return settings.value(settingPathsTemplates, getDefaultTemplatePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setTemplatePath(const QString &value)
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(settingPathsTemplates, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultBodyScansPath()
{
    return dataSubdirPath(tr("bodyscans"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getBodyScansPath() const
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    return settings.value(settingPathsBodyScans, getDefaultBodyScansPath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setBodyScansPath(const QString &value)
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(settingPathsBodyScans, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultLabelTemplatePath()
{
    return dataSubdirPath(tr("label templates"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getLabelTemplatePath() const
{
    return value(settingPathsLabelTemplate, getDefaultLabelTemplatePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathLabelTemplate(const QString &text)
{
    setValue(settingPathsLabelTemplate, text);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultImageFilePath()
{
    return dataSubdirPath(tr("images"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getImageFilePath() const
{
    return value(settingImagesPath, getDefaultImageFilePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setImageFilePath(const QString &text)
{
    setValue(settingImagesPath, text);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultBackupFilePath()
{
    return dataSubdirPath(tr("backups"));
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getBackupFilePath() const
{
    return value(settingBackupPath, getDefaultBackupFilePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setBackupFilePath(const QString &text)
{
    setValue(settingBackupPath, text);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultPatternTemplate() const
{
    return value(settingDefaultPatternTemplate, getLabelTemplatePath() + "/default_pattern_label.xml").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultPatternTemplate(const QString &text)
{
    setValue(settingDefaultPatternTemplate, text);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultPieceTemplate() const
{
    return value(settingDefaultPieceTemplate, getLabelTemplatePath() + "/default_piece_label.xml").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultPieceTemplate(const QString &value)
{
    setValue(settingDefaultPieceTemplate, value);
}

//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::getAppTheme() const
{
    return value(settingConfigurationAppTheme, 0).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setAppTheme(const int &value)
{
    setValue(settingConfigurationAppTheme, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowWelcome() const
{
    return value(settingConfigurationShowWelcome, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowWelcome(const bool &value)
{
    setValue(settingConfigurationShowWelcome, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getOsSeparator() const
{
    return value(settingConfigurationOsSeparator, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setOsSeparator(const bool &value)
{
    setValue(settingConfigurationOsSeparator, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getConvertBackupEnabled() const
{
    return value(settingConfigurationConvertBackup, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setConvertBackupEnabled(const bool &value)
{
    setValue(settingConfigurationConvertBackup, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::GetAutosaveState() const
{
    return value(settingConfigurationAutosaveState, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setAutosaveState(const bool &value)
{
    setValue(settingConfigurationAutosaveState, value);
}

//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::getAutosaveInterval() const
{
    bool ok = false;
    int val = value(settingConfigurationAutosaveTime, 1).toInt(&ok);
    if (ok == false)
    {
        qDebug() << "Could not convert value"<<value(settingConfigurationAutosaveTime, 1)
                   << "to int. Return default value for autosave time" << 1 << "minutes.";
        val = 1;
    }
    return val;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setAutosaveInterval(const int &value)
{
    setValue(settingConfigurationAutosaveTime, value);
}

//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::getMaxBackups() const
{
    bool ok = false;
    int val = value(settingConfigurationMaxBackups, 1).toInt(&ok);
    if (ok == false)
    {
        val = 1;
    }
    return val;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setMaxBackups(const int &value)
{
    setValue(settingConfigurationMaxBackups, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::useModeType() const
{
    return value(settingConfigurationUseModeType, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setUseModeType(const bool &value)
{
    setValue(settingConfigurationUseModeType, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::useLastExportFormat() const
{
    return value(settingConfigurationUseLastExportFormat, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setUseLastExportFormat(const bool &value)
{
    setValue(settingConfigurationUseLastExportFormat, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getExportFormat() const
{
    return value(settingConfigurationExportFormat, "SVG").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setExportFormat(const QString &value)
{
    setValue(settingConfigurationExportFormat, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::GetSendReportState() const
{
    return value(settingConfigurationSendReportState, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetSendReportState(const bool &value)
{
    setValue(settingConfigurationSendReportState, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getLocale() const
{
    return value(settingConfigurationLocale, QLocale().name()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setLocale(const QString &value)
{
    setValue(settingConfigurationLocale, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::GetPMSystemCode() const
{
    return value(settingPMSystemCode, "p998").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPMSystemCode(const QString &value)
{
    setValue(settingPMSystemCode, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getUnit() const
{
    return value(settingConfigurationUnit,
                 QLocale().measurementSystem() == QLocale::MetricSystem ? unitCM : unitINCH).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetUnit(const QString &value)
{
    setValue(settingConfigurationUnit, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getConfirmItemDelete() const
{
    return value(settingConfigurationConfirmItemDeletion, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setConfirmItemDelete(const bool &value)
{
    setValue(settingConfigurationConfirmItemDeletion, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getConfirmFormatRewriting() const
{
    return value(settingConfigurationConfirmFormatRewriting, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setConfirmFormatRewriting(const bool &value)
{
    setValue(settingConfigurationConfirmFormatRewriting, value);
}


//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getMoveSuffix() const
{
    return value(settingConfigurationMoveSuffix, "").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setMoveSuffix(const QString &value)
{
    setValue(settingConfigurationMoveSuffix, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getRotateSuffix() const
{
    return value(settingConfigurationRotateSuffix, "").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setRotateSuffix(const QString &value)
{
    setValue(settingConfigurationRotateSuffix, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getMirrorByAxisSuffix() const
{
    return value(settingConfigurationMirrorByAxisSuffix, "").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setMirrorByAxisSuffix(const QString &value)
{
    setValue(settingConfigurationMirrorByAxisSuffix, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getMirrorByLineSuffix() const
{
    return value(settingConfigurationMirrorByLineSuffix, "").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setMirrorByLineSuffix(const QString &value)
{
    setValue(settingConfigurationMirrorByLineSuffix, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getToolBarStyle() const
{
    return value(settingGraphicsViewToolBarStyle, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setToolBarStyle(const bool &value)
{
    setValue(settingGraphicsViewToolBarStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowToolsToolBar() const
{
    return value(settingGraphicsViewShowToolsToolBar, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowToolsToolBar(const bool &value)
{
    setValue(settingGraphicsViewShowToolsToolBar, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowPointToolBar() const
{
    return value(settingGraphicsViewShowPointToolBar, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowPointToolBar(const bool &value)
{
    setValue(settingGraphicsViewShowPointToolBar, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowLineToolBar() const
{
    return value(settingGraphicsViewShowLineToolBar, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowLineToolBar(const bool &value)
{
    setValue(settingGraphicsViewShowLineToolBar, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowCurveToolBar() const
{
    return value(settingGraphicsViewShowCurveToolBar, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowCurveToolBar(const bool &value)
{
    setValue(settingGraphicsViewShowCurveToolBar, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowArcToolBar() const
{
    return value(settingGraphicsViewShowArcToolBar, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowArcToolBar(const bool &value)
{
    setValue(settingGraphicsViewShowArcToolBar, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowOpsToolBar() const
{
    return value(settingGraphicsViewShowOpsToolBar, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowOpsToolBar(const bool &value)
{
    setValue(settingGraphicsViewShowOpsToolBar, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowPieceToolBar() const
{
    return value(settingGraphicsViewShowPieceToolBar, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowPieceToolBar(const bool &value)
{
    setValue(settingGraphicsViewShowPieceToolBar, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowDetailsToolBar() const
{
    return value(settingGraphicsViewShowDetailsToolBar, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowDetailsToolBar(const bool &value)
{
    setValue(settingGraphicsViewShowDetailsToolBar, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowLayoutToolBar() const
{
    return value(settingGraphicsViewShowLayoutToolBar, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowLayoutToolBar(const bool &value)
{
    setValue(settingGraphicsViewShowLayoutToolBar, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::useNativeDialogs() const
{
    return value(settingGraphicsUseNativeDialogs, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setUseNativeDialogs(const bool &value)
{
    setValue(settingGraphicsUseNativeDialogs, value);
}

//---------------------------------------------------------------------------------------------------------------------
QFileDialog::Options VCommonSettings::getUseNativeFileDialogs() const
{
    QFileDialog::Options options = QFileDialog::Options();
    if (!value(settingGraphicsUseNativeDialogs, true).toBool())
    {
        options = QFileDialog::DontUseNativeDialog;
    }
    return options;
}

//---------------------------------------------------------------------------------------------------------------------
QColorDialog::ColorDialogOptions VCommonSettings::getUseNativeColorDialogs() const
{
    QColorDialog::ColorDialogOptions options = QColorDialog::ColorDialogOptions();
    if (!value(settingGraphicsUseNativeDialogs, true).toBool())
    {
        options = QColorDialog::DontUseNativeDialog;
    }
    return options;
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::useSecondMonitor() const
{
    return value(settingGraphicsUseSecondMonitor, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setUseSecondMonitor(const bool &value)
{
    setValue(settingGraphicsUseSecondMonitor, value);
}


//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::getDialogPosition() const
{
    return value(settingGraphicsViewDialogPosition, -4).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDialogPosition(const int &value)
{
    setValue(settingGraphicsViewDialogPosition, value);
}

//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::getXOffset() const
{
    return value(settingGraphicsViewXOffset, 0).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setXOffset(const int &value)
{
    setValue(settingGraphicsViewXOffset, value);
}
//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::getYOffset() const
{
    return value(settingGraphicsViewYOffset, 0).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setYOffset(const int &value)
{
    setValue(settingGraphicsViewYOffset, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool  VCommonSettings::getShowScrollBars() const
{
    return value(settingGraphicsViewShowScrollBars, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowScrollBars(const bool  &value)
{
    setValue(settingGraphicsViewShowScrollBars, value);
}

//---------------------------------------------------------------------------------------------------------------------
int  VCommonSettings::getScrollBarWidth() const
{
    return value(settingGraphicsViewScrollBarWidth, 10).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setScrollBarWidth(const int  &width)
{
    setValue(settingGraphicsViewScrollBarWidth, width);
}

//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::getScrollDuration() const
{
    return value(settingGraphicsViewScrollDuration, 300).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setScrollDuration(const int &duration)
{
    setValue(settingGraphicsViewScrollDuration, duration);
}

//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::getScrollUpdateInterval() const
{
    return value(settingGraphicsViewScrollUpdateInterval, 30).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setScrollUpdateInterval(const int &interval)
{
    setValue(settingGraphicsViewScrollUpdateInterval, interval);
}

//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::getScrollSpeedFactor() const
{
    return value(settingGraphicsViewScrollSpeedFactor, 10).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setScrollSpeedFactor(const int &factor)
{
    setValue(settingGraphicsViewScrollSpeedFactor, factor);
}


//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getZoomModKey() const
{
    return value(settingGraphicsViewZoomModKey, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setZoomModKey(const bool &value)
{
    setValue(settingGraphicsViewZoomModKey, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::isZoomDoubleClick() const
{
    return value(settingGraphicsViewZoomDoubleClick, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setZoomDoubleClick(const bool &value)
{
    setValue(settingGraphicsViewZoomDoubleClick, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::isPanActiveSpaceKey() const
{
    return value(settingGraphicsViewPanActiveSpaceKey, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setPanActiveSpaceKey(const bool &value)
{
    setValue(settingGraphicsViewPanActiveSpaceKey, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::useCurrentPen() const
{
    return value(settingGraphicsViewUseDefaultPen, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setUseCurrentPen(const bool &value)
{
    setValue(settingGraphicsViewUseDefaultPen, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::showOnlyIso() const
{
    return value(settingGraphicsViewShowIsoOnly, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowIsoOnly(const bool &value)
{
    setValue(settingGraphicsViewShowIsoOnly, value);
}

//---------------------------------------------------------------------------------------------------------------------
int  VCommonSettings::getZoomSpeedFactor() const
{
    return value(settingGraphicsViewZoomSpeedFactor, 16).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setZoomSpeedFactor(const int  &value)
{
    setValue(settingGraphicsViewZoomSpeedFactor, value);
}

//---------------------------------------------------------------------------------------------------------------------
int  VCommonSettings::getExportQuality() const
{
    return value(settingGraphicsViewExportQuality, 75).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setExportQuality(const int  &value)
{
    setValue(settingGraphicsViewExportQuality, value);
}

//-----------------------------------------------------------------------------
/// @brief getBackgroundColor Gets the background color.
///
/// This method gets the name of the background color from the settings.
///
/// @return String name of the background color.
//-----------------------------------------------------------------------------
QString VCommonSettings::getBackgroundColor() const
{
    return getStr(settingGraphicsViewBackgroundColor, "white");
}

//-----------------------------------------------------------------------------
/// @brief setBackgroundColor Sets the background color.
///
/// This method saves the background color name to the settings.
///
/// @param color String name of background color to save.
//-----------------------------------------------------------------------------
void VCommonSettings::setBackgroundColor(const QString &color)
{
    setValue(settingGraphicsViewBackgroundColor, color);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getZoomRBPositiveColor() const
{
    return getStr(settingGraphicsViewZoomRBPositiveColor, "blue");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setZoomRBPositiveColor(const QString &value)
{
    setValue(settingGraphicsViewZoomRBPositiveColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getZoomRBNegativeColor() const
{
    return getStr(settingGraphicsViewZoomRBNegativeColor, "green");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setZoomRBNegativeColor(const QString &value)
{
    setValue(settingGraphicsViewZoomRBNegativeColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getPointNameColor() const
{
    return getStr(settingGraphicsViewPointNameColor, "black");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setPointNameColor(const QString &value)
{
    setValue(settingGraphicsViewPointNameColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getPointNameHoverColor() const
{
    return getStr(settingGraphicsViewPointNameHoverColor, "deeppink");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setPointNameHoverColor(const QString &value)
{
    setValue(settingGraphicsViewPointNameHoverColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getAxisOrginColor() const
{
    return getStr(settingGraphicsViewAxisOrginColor, "deeppink");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setAxisOrginColor(const QString &value)
{
    setValue(settingGraphicsViewAxisOrginColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultLineColor() const
{
    return getStr(settingGraphicsViewDefaultLineColor, "black");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultLineColor(const QString &value)
{
    setValue(settingGraphicsViewDefaultLineColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultLineWeight() const
{
    return value(settingGraphicsViewDefaultLineWeight, 0.35).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultLineWeight(const qreal &value)
{
    setValue(settingGraphicsViewDefaultLineWeight, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultLineType() const
{
    return value(settingGraphicsViewDefaultLineType, "solidLine").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultLineType(const QString &value)
{
    setValue(settingGraphicsViewDefaultLineType, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getPrimarySupportColor() const
{
    return getStr(settingGraphicsViewPrimaryColor, "magenta");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setPrimarySupportColor(const QString &value)
{
    setValue(settingGraphicsViewPrimaryColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getSecondarySupportColor() const
{
    return getStr(settingGraphicsViewSecondaryColor, "forestgreen");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setSecondarySupportColor(const QString &value)
{
    setValue(settingGraphicsViewSecondaryColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getTertiarySupportColor() const
{
    return getStr(settingGraphicsViewTertiaryColor, "navy");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setTertiarySupportColor(const QString &value)
{
    setValue(settingGraphicsViewTertiaryColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getConstrainValue() const
{
    return value(settingGraphicsViewConstrainValue, 10).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setConstrainValue(const qreal &value)
{
    setValue(settingGraphicsViewConstrainValue, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getConstrainModKey() const
{
    return value(settingGraphicsViewConstrainModKey, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setConstrainModKey(const bool &value)
{
    setValue(settingGraphicsViewConstrainModKey, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getCompanyName() const
{
    return value(settingConfigurationCompanyName, "").toString();
}

void VCommonSettings::setCompanyName(const QString &value)
{
    setValue(settingConfigurationCompanyName, value);
}

QString VCommonSettings::getContact() const
{
    return value(settingConfigurationContact, "").toString();
}

void VCommonSettings::setContact(const QString &value)
{
    setValue(settingConfigurationContact, value);
}

QString VCommonSettings::getAddress() const
{
    return value(settingConfigurationAddress, "").toString();
}

void VCommonSettings::setAddress(const QString &value)
{
    setValue(settingConfigurationAddress, value);
}

QString VCommonSettings::getCity() const
{
    return value(settingConfigurationCity, "").toString();
}

void VCommonSettings::setCity(const QString &value)
{
    setValue(settingConfigurationCity, value);
}

QString VCommonSettings::getState() const
{
    return value(settingConfigurationState, "").toString();
}

void VCommonSettings::setState(const QString &value)
{
    setValue(settingConfigurationState, value);
}

QString VCommonSettings::getZipcode() const
{
    return value(settingConfigurationZipcode, "").toString();
}

void VCommonSettings::setZipcode(const QString &value)
{
    setValue(settingConfigurationZipcode, value);
}

QString VCommonSettings::getCountry() const
{
    return value(settingConfigurationCountry, "").toString();
}

void VCommonSettings::setCountry(const QString &value)
{
    setValue(settingConfigurationCountry, value);
}

QString VCommonSettings::getTelephone() const
{
    return value(settingConfigurationTelephone, "").toString();
}

void VCommonSettings::setTelephone(const QString &value)
{
    setValue(settingConfigurationTelephone, value);
}

QString VCommonSettings::getFax() const
{
    return value(settingConfigurationFax, "").toString();
}

void VCommonSettings::setFax(const QString &value)
{
    setValue(settingConfigurationFax, value);
}

QString VCommonSettings::getEmail() const
{
    return value(settingConfigurationEmail, "").toString();
}

void VCommonSettings::setEmail(const QString &value)
{
    setValue(settingConfigurationEmail, value);
}

QString VCommonSettings::getWebsite() const
{
    return value(settingConfigurationWebsite, "").toString();
}

void VCommonSettings::setWebsite(const QString &value)
{
    setValue(settingConfigurationWebsite, value);
}

//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::GetUndoCount() const
{
    bool ok = false;
    int val = value(settingPatternUndo, 0).toInt(&ok);
    if (ok == false)
    {
        qDebug() << "Could not convert value"<<value(settingPatternUndo, 0)
                   << "to int. Return default value for undo counts 0 (no limit).";
        val = 0;
    }
    return val;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setUndoCount(const int &value)
{
    setValue(settingPatternUndo, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getSound() const
{
    return value(settingSelectionSound, "silent").toString();
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getSelectionSound() const
{
    return QStringLiteral("qrc:/sounds/") + value(settingSelectionSound, "silent").toString() + QStringLiteral(".wav");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setSelectionSound(const QString &value)
{
    setValue(settingSelectionSound, value);
}

//---------------------------------------------------------------------------------------------------------------------
QStringList VCommonSettings::GetRecentFileList() const
{
    const QStringList files = value(settingGeneralRecentFileList).toStringList();
    QStringList cleared;

    for (int i = 0; i < files.size(); ++i)
    {
        if (QFileInfo(files.at(i)).exists())
        {
            cleared.append(files.at(i));
        }
    }

    return cleared;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetRecentFileList(const QStringList &value)
{
    setValue(settingGeneralRecentFileList, value);
}

//---------------------------------------------------------------------------------------------------------------------
QStringList VCommonSettings::GetRestoreFileList() const
{
    return value(settingGeneralRestoreFileList).toStringList();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetRestoreFileList(const QStringList &value)
{
    setValue(settingGeneralRestoreFileList, value);
}

//---------------------------------------------------------------------------------------------------------------------
QByteArray VCommonSettings::GetGeometry() const
{
    return value(settingGeneralGeometry).toByteArray();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetGeometry(const QByteArray &value)
{
    setValue(settingGeneralGeometry, value);
}

//---------------------------------------------------------------------------------------------------------------------
QByteArray VCommonSettings::GetWindowState() const
{
    return value(settingGeneralWindowState).toByteArray();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetWindowState(const QByteArray &value)
{
    setValue(settingGeneralWindowState, value);
}

//---------------------------------------------------------------------------------------------------------------------
QByteArray VCommonSettings::GetToolbarsState() const
{
    return value(settingGeneralToolbarsState).toByteArray();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetToolbarsState(const QByteArray &value)
{
    setValue(settingGeneralToolbarsState, value);
}

//---------------------------------------------------------------------------------------------------------------------
QSize VCommonSettings::getPreferenceDialogSize() const
{
    return value(settingPreferenceDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setPreferenceDialogSize(const QSize& sz)
{
    setValue(settingPreferenceDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
QSize VCommonSettings::getPatternPieceDialogSize() const
{
    return value(settingToolSeamAllowanceDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setPatternPieceDialogSize(const QSize &sz)
{
    setValue(settingToolSeamAllowanceDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
QSize VCommonSettings::GetFormulaWizardDialogSize() const
{
    return value(settingFormulaWizardDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetFormulaWizardDialogSize(const QSize &sz)
{
    setValue(settingFormulaWizardDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
QSize VCommonSettings::getVariablesDialogSize() const
{
    return value(settingVariablesDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setVariablesDialogSize(const QSize &sz)
{
    setValue(settingVariablesDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
QSize VCommonSettings::getHistoryDialogSize() const
{
    return value(settingHistoryDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setHistoryDialogSize(const QSize &sz)
{
    setValue(settingHistoryDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::GetLatestSkippedVersion() const
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    return settings.value(settingLatestSkippedVersion, 0x0).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetLatestSkippedVersion(int value)
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(settingLatestSkippedVersion, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
QDate VCommonSettings::GetDateOfLastRemind() const
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    return settings.value(settingDateOfLastRemind, QDate(1900, 1, 1)).toDate();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetDateOfLastRemind(const QDate &date)
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(settingDateOfLastRemind, date);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getForbidPieceFlipping() const
{
    return value(settingPatternForbidFlipping, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setForbidPieceFlipping(bool value)
{
    setValue(settingPatternForbidFlipping, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::isHideSeamLine() const
{
    return value(settingPatternHideSeamLine, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setHideSeamLine(bool value)
{
    setValue(settingPatternHideSeamLine, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::showSeamlineNotch() const
{
    return value(settingSeamlineNotch, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowSeamlineNotch(bool value)
{
    setValue(settingSeamlineNotch, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::showSeamAllowanceNotch() const
{
    return value(settingSeamAllowanceNotch, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowSeamAllowanceNotch(bool value)
{
    setValue(settingSeamAllowanceNotch, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultNotchLength() const
{
    double maxValue;

    const Unit units = StrToUnits(getUnit());

    switch (units)
    {
        case Unit::Mm:
            maxValue = 12.5;
            break;
        case Unit::Inch:
            maxValue = .5;
            break;
        default:
        case Unit::Cm:
            maxValue = 1.25;
            break;
   }
   return value(settingDefaultNotchLength, maxValue).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultNotchLength(const qreal &value)
{
    setValue(settingDefaultNotchLength, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultNotchWidth() const
{
   double maxValue;

   const Unit units = StrToUnits(getUnit());

   switch (units)
   {
       case Unit::Mm:
           maxValue = 5.0;
           break;
       case Unit::Inch:
           maxValue = 0.25;
           break;
       default:
       case Unit::Cm:
           maxValue = .5;
           break;
   }
   return value(settingDefaultNotchWidth, maxValue).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultNotchWidth(const qreal &value)
{
    setValue(settingDefaultNotchWidth, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultNotchType() const
{
   return value(settingDefaultNotchType, "Slit").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultNotchType(const QString &value)
{
    setValue(settingDefaultNotchType, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultNotchColor() const
{
   return getStr(settingDefaultNotchColor, "black");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultNotchColor(const QString &value)
{
    setValue(settingDefaultNotchColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetCSVWithHeader(bool withHeader)
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(settingCSVWithHeader, withHeader);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::GetCSVWithHeader() const
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    return settings.value(settingCSVWithHeader, GetDefCSVWithHeader()).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::GetDefCSVWithHeader() const
{
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetCSVCodec(QStringConverter::Encoding encoding)
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    settings.setValue(settingCSVCodec, encoding);
}

//---------------------------------------------------------------------------------------------------------------------
QStringConverter::Encoding VCommonSettings::GetCSVCodec() const
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    return settings.value(settingCSVCodec, GetDefCSVCodec()).value<QStringConverter::Encoding>();
}

//---------------------------------------------------------------------------------------------------------------------
// Returns the default encoding as a QStringConverter::Encoding value cast to int.
// Previously used QTextCodec MIB values, now stores QStringConverter::Encoding enum values.
QStringConverter::Encoding VCommonSettings::GetDefCSVCodec() const
{
    // Default to UTF-8 encoding (Qt6 default)
    return QStringConverter::Utf8;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetCSVSeparator(const QChar &separator)
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    switch(separator.toLatin1())
    {
        case '\t':
            settings.setValue(settingCSVSeparator, 0);
            break;
        case ';':
            settings.setValue(settingCSVSeparator, 1);
            break;
        case ' ':
            settings.setValue(settingCSVSeparator, 2);
            break;
        default:
            settings.setValue(settingCSVSeparator, 3);
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
QChar VCommonSettings::GetCSVSeparator() const
{
    QSettings settings(commonSettingsFilePath(), QSettings::IniFormat);
    const quint8 separator = static_cast<quint8>(settings.value(settingCSVSeparator, 3).toUInt());
    switch(separator)
    {
        case 0:
            return QChar('\t');
        case 1:
            return QChar(';');
        case 2:
            return QChar(' ');
        default:
            return QChar(',');
    }
}

//---------------------------------------------------------------------------------------------------------------------
QChar VCommonSettings::GetDefCSVSeparator() const
{
    return QChar(',');
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetDefaultSeamAllowance(double value)
{
    setValue(settingPatternDefaultSeamAllowance, UnitConvertor(value, StrToUnits(getUnit()), Unit::Cm));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief returns the default seam allowance. The corresponding unit is the default unit.
 * @return the default seam allowance
 */
double VCommonSettings::GetDefaultSeamAllowance()
{
    double defaultValue;

    const Unit units = StrToUnits(getUnit());

    switch (units)
    {
        case Unit::Mm:
            defaultValue = 10;
            break;
        case Unit::Inch:
            defaultValue = 0.25;
            break;
        default:
        case Unit::Cm:
            defaultValue = 1;
            break;
    }

    bool ok = false;
    double val = value(settingPatternDefaultSeamAllowance, -1).toDouble(&ok);
    if (ok == false)
    {
        qDebug() <<  "Could not convert value"<<value(settingPatternDefaultSeamAllowance, 0)
                   << "to real. Return default value for default seam allowance is " << defaultValue << ".";
        val = defaultValue;
    }

    if (val < 0)
    {
        val = defaultValue;
    }
    else
    {
        val = UnitConvertor(val, Unit::Cm, units);
    }

    return val;
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultSeamColor() const
{
   return getStr(settingDefaultSeamColor, "black");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultSeamColor(const QString &value)
{
    setValue(settingDefaultSeamColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultSeamLinetype() const
{
   return value(settingDefaultSeamLinetype, "solidLine").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultSeamLinetype(const QString &value)
{
    setValue(settingDefaultSeamLinetype, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultSeamLineweight() const
{
   return value(settingDefaultSeamLineweight, 0.35).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultSeamLineweight(const qreal &value)
{
    setValue(settingDefaultSeamLineweight, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultCutColor() const
{
   return getStr(settingDefaultCutColor, "black");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultCutColor(const QString &value)
{
    setValue(settingDefaultCutColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultCutLinetype() const
{
   return value(settingDefaultCutLinetype, "solidLine").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultCutLinetype(const QString &value)
{
    setValue(settingDefaultCutLinetype, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultCutLineweight() const
{
   return value(settingDefaultCutLineweight, 0.35).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultCutLineweight(const qreal &value)
{
    setValue(settingDefaultCutLineweight, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultInternalColor() const
{
   return getStr(settingDefaultInternalColor, "black");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultInternalColor(const QString &value)
{
    setValue(settingDefaultInternalColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultInternalLinetype() const
{
   return value(settingDefaultInternalLinetype, "solidLine").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultInternalLinetype(const QString &value)
{
    setValue(settingDefaultInternalLinetype, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultInternalLineweight() const
{
   return value(settingDefaultInternalLineweight, 0.35).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultInternalLineweight(const qreal &value)
{
    setValue(settingDefaultInternalLineweight, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultCutoutColor() const
{
   return getStr(settingDefaultCutoutColor, "black");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultCutoutColor(const QString &value)
{
    setValue(settingDefaultCutoutColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultCutoutLinetype() const
{
   return value(settingDefaultCutoutLinetype, "solidLine").toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultCutoutLinetype(const QString &value)
{
    setValue(settingDefaultCutoutLinetype, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultCutoutLineweight() const
{
   return value(settingDefaultCutoutLineweight, 0.35).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultCutoutLineweight(const qreal &value)
{
    setValue(settingDefaultCutoutLineweight, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::showSeamAllowances() const
{
    return value(settingShowSeamAllowances, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowSeamAllowances(const bool &value)
{
    setValue(settingShowSeamAllowances, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getDefaultSeamAllowanceVisibilty() const
{
    return value(settingDefaultSeamAllowanceVisibilty, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultSeamAllowanceVisibilty(const bool &value)
{
    setValue(settingDefaultSeamAllowanceVisibilty, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::showGrainlines() const
{
    return value(settingShowGrainlines, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowGrainlines(const bool &value)
{
    setValue(settingShowGrainlines, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getDefaultGrainlineVisibilty() const
{
    return value(settingDefaultGrainlineVisibilty, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultGrainlineVisibilty(const bool &value)
{
    setValue(settingDefaultGrainlineVisibilty, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultGrainlineLength() const
{
   return value(settingDefaultGrainlineLength, 2).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultGrainlineLength(const qreal &value)
{
    setValue(settingDefaultGrainlineLength, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultGrainlineColor() const
{
   return getStr(settingDefaultGrainlineColor, "black");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultGrainlineColor(const QString &value)
{
    setValue(settingDefaultGrainlineColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultGrainlineLineweight() const
{
   return value(settingDefaultGrainlineLineweight, 0.35).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultGrainlineLineweight(const qreal &value)
{
    setValue(settingDefaultGrainlineLineweight, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultArrowLength() const
{
   return value(settingDefaultArrowLength, 48).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultArrowLength(const qreal &value)
{
    setValue(settingDefaultArrowLength, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::showLabels() const
{
    return value(settingShowLabels, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowLabels(const bool &value)
{
    setValue(settingShowLabels, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::showPatternLabels() const
{
    return value(settingShowPatternLabels, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowPatternLabels(const bool &value)
{
    setValue(settingShowPatternLabels, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::showPieceLabels() const
{
    return value(settingShowPieceLabels, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowPieceLabels(const bool &value)
{
    setValue(settingShowPieceLabels, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultLabelWidth() const
{
   return value(settingDefaultLabelWidth, 3).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultLabelWidth(const qreal &value)
{
    setValue(settingDefaultLabelWidth, value);
}

//---------------------------------------------------------------------------------------------------------------------
qreal VCommonSettings::getDefaultLabelHeight() const
{
   return value(settingDefaultLabelHeight, 2).toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultLabelHeight(const qreal &value)
{
    setValue(settingDefaultLabelHeight, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::getDefaultLabelColor() const
{
   return getStr(settingDefaultLabelColor, "black");
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setDefaultLabelColor(const QString &value)
{
    setValue(settingDefaultLabelColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
QFont VCommonSettings::getLabelFont() const
{
    return qvariant_cast<QFont>(value(settingPatternLabelFont, QApplication::font()));
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setLabelFont(const QFont &f)
{
    setValue(settingPatternLabelFont, f);
}

//---------------------------------------------------------------------------------------------------------------------
QFont VCommonSettings::getGuiFont() const
{
    return qvariant_cast<QFont>(value(settingPatternGuiFont, QApplication::font()));
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setGuiFont(const QFont &f)
{
    setValue(settingPatternGuiFont, f);
}

//---------------------------------------------------------------------------------------------------------------------
QFont VCommonSettings::getPointNameFont() const
{
    return qvariant_cast<QFont>(value(settingPatternPointNameFont, QApplication::font()));
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setPointNameFont(const QFont &f)
{
    setValue(settingPatternPointNameFont, f);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getHidePointNames() const
{
    return value(settingGraphicsViewHidePointNames, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setHidePointNames(bool value)
{
    setValue(settingGraphicsViewHidePointNames, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowAxisOrigin() const
{
    return value(settingGraphicsViewShowAxisOrigin, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowAxisOrigin(bool value)
{
    setValue(settingGraphicsViewShowAxisOrigin, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::isWireframe() const
{
    return value(settingGraphicsViewWireframe, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setWireframe(bool value)
{
    setValue(settingGraphicsViewWireframe, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowControlPoints() const
{
    return value(settingGraphicsViewShowControlPoints, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowControlPoints(bool value)
{
    setValue(settingGraphicsViewShowControlPoints, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getShowAnchorPoints() const
{
    return value(settingGraphicsViewShowAnchorPoints, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setShowAnchorPoints(bool value)
{
    setValue(settingGraphicsViewShowAnchorPoints, value);
}

//---------------------------------------------------------------------------------------------------------------------
bool VCommonSettings::getUseToolColor() const
{
    return value(settingGraphicsUseToolColor, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setUseToolColor(bool value)
{
    setValue(settingGraphicsUseToolColor, value);
}

//---------------------------------------------------------------------------------------------------------------------
int VCommonSettings::getPointNameSize() const
{
    if (pointNameSize <= 0)
    {
        bool ok = false;
        pointNameSize = value(settingGraphicsViewPointNameSize, 32).toInt(&ok);
        if (not ok)
        {
            pointNameSize = 32;
        }
    }
    return pointNameSize;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setPointNameSize(int value)
{
    setValue(settingGraphicsViewPointNameSize, value);
    pointNameSize = value;
}

int VCommonSettings::getGuiFontSize() const
{
    return value(settingGraphicsViewGuiFontSize, 9).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::setGuiFontSize(int value)
{
    setValue(settingGraphicsViewGuiFontSize, value);
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::GetLabelDateFormat() const
{
    const QString format = value(settingLabelDateFormat, VCommonSettings::PredefinedDateFormats().first()).toString();
    const QStringList allFormats = VCommonSettings::PredefinedDateFormats() + GetUserDefinedDateFormats();

    if (allFormats.contains(format))
    {
        return format;
    }
    else
    {
        return VCommonSettings::PredefinedDateFormats().first();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetLabelDateFormat(const QString &format)
{
    setValue(settingLabelDateFormat, format);
}

//---------------------------------------------------------------------------------------------------------------------
QStringList VCommonSettings::PredefinedDateFormats()
{
    QStringList formats = QStringList() << "MM-dd-yyyy"
                                        << "d/M/yy"
                                        << "ddddMMMM dd, yyyy"
                                        << "dd/MM/yy"
                                        << "dd/MM/yyyy"
                                        << "MMM d, yy"
                                        << "MMM d, yyyy"
                                        << "d. MMM. yyyy"
                                        << "MMMM d, yyyy"
                                        << "d. MMMM yyyy"
                                        << "ddd, MMM d, yy"
                                        << "ddd dd/MMM yy"
                                        << "ddd, MMMM d, yyyy"
                                        << "ddddMMMM d, yyyy"
                                        << "MM-dd"
                                        << "yy-MM-dd"
                                        << "yyyy-MM-dd"
                                        << "MM/yy"
                                        << "MMM dd"
                                        << "MMMM";
    return formats;
}

//---------------------------------------------------------------------------------------------------------------------
QStringList VCommonSettings::GetUserDefinedDateFormats() const
{
    return value(settingLabelUserDateFormats, QStringList()).toStringList();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetUserDefinedDateFormats(const QStringList &formats)
{
    setValue(settingLabelUserDateFormats, ClearFormats(VCommonSettings::PredefinedDateFormats(), formats));
}

//---------------------------------------------------------------------------------------------------------------------
QString VCommonSettings::GetLabelTimeFormat() const
{
    const QString format = value(settingLabelTimeFormat, VCommonSettings::PredefinedTimeFormats().first()).toString();
    const QStringList allFormats = VCommonSettings::PredefinedTimeFormats() + GetUserDefinedTimeFormats();

    if (allFormats.contains(format))
    {
        return format;
    }
    else
    {
        return VCommonSettings::PredefinedTimeFormats().first();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetLabelTimeFormat(const QString &format)
{
    setValue(settingLabelTimeFormat, format);
}

//---------------------------------------------------------------------------------------------------------------------
QStringList VCommonSettings::PredefinedTimeFormats()
{
    QStringList formats = QStringList() << "hh:mm:ss"
                                        << "hh:mm:ss AP"
                                        << "hh:mm"
                                        << "hh:mm AP";
    return formats;
}

//---------------------------------------------------------------------------------------------------------------------
QStringList VCommonSettings::GetUserDefinedTimeFormats() const
{
    return value(settingLabelUserTimeFormats, QStringList()).toStringList();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetUserDefinedTimeFormats(const QStringList &formats)
{
    setValue(settingLabelUserTimeFormats, ClearFormats(VCommonSettings::PredefinedTimeFormats(), formats));
}

QString VCommonSettings::getStr(QString key, const QString &defaultString) const
{
    QString string = value(key, defaultString).toString();
    if(!string.isEmpty())
    {
        return string;
    }
    return defaultString;
}

bool VCommonSettings::autoClearFx() const
{
    return value(settingGraphicsAutoClearFx, false).toBool();
}

void VCommonSettings::setAutoClearFx(bool value)
{
    setValue(settingGraphicsAutoClearFx, value);
}
