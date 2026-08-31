//  @file   vabstractapplication.cpp
//  @author Douglas S Caskey
//  @date   13 JUl, 2025
//
//  @brief
//  Shared QApplication base class for Seamly2D and SeamlyMe. Holds the
//  settings object, pattern document state, unit conversion, translators,
//  settings-location migration, and the one-shot user notices.
//
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
//  @file   vabstractapplication.cpp
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date   18 6, 2015
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

#include "vabstractapplication.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QMessageLogger>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTranslator>
#include <Qt>
#include <QtDebug>

#include "../vmisc/def.h"
#include "../vmisc/logging.h"

//=====================================================================================================================
// Lifecycle
//=====================================================================================================================

//---------------------------------------------------------------------------------------------------------------------
/** @brief VAbstractApplication sets logging rules, high-DPI pixmaps, and a settings sync on quit. */
VAbstractApplication::VAbstractApplication(int &argc, char **argv)
    :QApplication(argc, argv),
      undoStack(nullptr),
      mainWindow(nullptr),
      m_settings(nullptr),
      qtTranslator(nullptr),
      qtxmlTranslator(nullptr),
      qtBaseTranslator(nullptr),
      appTranslator(nullptr),
      pmsTranslator(nullptr),
      _patternUnit(Unit::Cm),
      _patternType(MeasurementsType::Unknown),
      patternFilePath(),
      currentScene(nullptr),
      sceneView(nullptr),
      doc(nullptr),
      data(nullptr),
      openingPattern(false)
{
    QString rules;

#if defined(V_NO_ASSERT)
    // Ignore SSL-related warnings
    // See issue #528: Error: QSslSocket: cannot resolve SSLv2_client_method.
    rules += QLatin1String("qt.network.ssl.warning=false\n");
    // See issue #568: Certificate checking on Mac OS X.
    rules += QLatin1String("qt.network.ssl.critical=false\n"
                           "qt.network.ssl.fatal=false\n");
#endif //defined(V_NO_ASSERT)

    // cppcheck-suppress reademptycontainer
    if (!rules.isEmpty())
    {
        QLoggingCategory::setFilterRules(rules);
    }

    setAttribute(Qt::AA_UseHighDpiPixmaps);

    connect(this, &QApplication::aboutToQuit, this, [this]()
    {
        // QApplication::exit() skips the settings sync and warns about the
        // QApplication instance, so sync here, on aboutToQuit, instead.
        Settings()->sync();
    });
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief ~VAbstractApplication is empty; Qt parent ownership releases the members. */
VAbstractApplication::~VAbstractApplication()
{}

//=====================================================================================================================
// Settings, installation data migration, and one-shot notices
//=====================================================================================================================

//---------------------------------------------------------------------------------------------------------------------
/** @brief Settings returns the application settings object; asserts it is open. */
VCommonSettings *VAbstractApplication::Settings()
{
    SCASSERT(m_settings != nullptr)
    return m_settings;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MigrateSeamlySettingsLocation migrates application settings files from the
 * previous "Seamly2DTeam" folder to the new unified "Seamly" folder with a subdirectory
 * for each app. Enables future applications to share settings and data.
 *
 * @param appIniFileName filename to use for this app's settings in the new location, e.g.
 * "qt6_seamly2d.ini".
 * @param legacyAppIniFileNames candidate filenames to look for inside the legacy
 * organization folder, tried in order (newest format first), e.g. {"qt6_seamly2d.ini",
 * "Seamly2D.ini"}.
 * @param migrated optional out-parameter set to true if a legacy file was copied into the
 * new location; leave null if the caller does not need to know (e.g. test harnesses, which
 * must never show the migration notice dialog).
 * @return absolute path to this app's settings file in the new unified location.
 */
QString VAbstractApplication::MigrateSeamlySettingsLocation(const QString &appIniFileName,
                                                             const QStringList &legacyAppIniFileNames,
                                                             bool *migrated)
{
    if (migrated != nullptr)
    {
        *migrated = false;
    }

    // New home: AppConfigLocation = <config root>/Seamly/<AppName> (AppData/Local on Windows).
    const QString newAppDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(newAppDir);
    const QString newAppIniPath = QDir(newAppDir).filePath(appIniFileName);

    if (QFileInfo::exists(newAppIniPath))
    {
        // Copy-if-missing: an existing file is never overwritten, so the
        // migration is non-destructive and re-entrant.
        return newAppIniPath;
    }

    // Legacy home: both apps' files shared the one "Seamly2DTeam" folder.
    static const QString kLegacyOrganizationName = QStringLiteral("Seamly2DTeam");
    const QSettings legacyProbe(QSettings::IniFormat, QSettings::UserScope,
                                kLegacyOrganizationName, QCoreApplication::applicationName());
    const QString legacyDir = QFileInfo(legacyProbe.fileName()).absolutePath();

    // First candidate found wins; the source file is left in place.
    for (const QString &legacyFileName : legacyAppIniFileNames)
    {
        const QString legacyPath = QDir(legacyDir).filePath(legacyFileName);
        if (QFileInfo::exists(legacyPath) && QFile::copy(legacyPath, newAppIniPath))
        {
            if (migrated != nullptr)
            {
                *migrated = true;
            }
            break;
        }
    }

    return newAppIniPath;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NotifySeamlySettingsMigrated shows a one-time informational dialog after
 * settings migration moved a user's preferences to the new unified "Seamly" location.
 * Only call this from a confirmed GUI-mode code path — never from a headless/CLI export
 * run or an automated test, since a modal dialog with no automatic dismissal would hang
 * a scripted caller waiting on process exit.
 * @param appDisplayName human-readable app name to mention in the message (e.g. "Seamly2D").
 */
void VAbstractApplication::NotifySeamlySettingsMigrated(const QString &appDisplayName)
{
    QMessageBox::information(
        mainWindow,
        tr("Settings moved"),
        tr("%1's settings and preferences have moved to a new shared location "
           "(the \"Seamly\" folder). Nothing was lost — this happens automatically, "
           "once, after upgrading.").arg(appDisplayName));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NotifySeamlyDataLocation shows the one-shot fresh-install notice about where
 * Seamly keeps its data and how existing user files were backed up.
 *
 * The Windows installer seeds the pending flag into qt6_common.ini when it creates that
 * file on a fresh machine. Whichever Seamly application runs first shows the notice,
 * then marks it shown, so the suite shows it once in total. Only call this from a
 * confirmed GUI-mode code path — a modal dialog would hang a headless or automated run.
 */
void VAbstractApplication::NotifySeamlyDataLocation()
{
    if (!VCommonSettings::firstRunNoticePending())
    {
        return;
    }

    QMessageBox::information(
        mainWindow,
        tr("Seamly data moved"),
        tr("Your files for patterns, measurements, images, layouts, and more\n"
           "   have been copied to the new data location--%1.\n"
           "Don't worry: \n"
           " • they have been left in their original location as a backup;\n"
           " • They have been archived (zipped) to %1 as a second backup.\n\n"
           "You may safely delete the files from the old location at your discretion,\n"
           "   (typically C:\\Users\\seamly2d).\n"
           )
            .arg(QDir::toNativeSeparators(VCommonSettings::dataRoot())));

    VCommonSettings::markFirstRunNoticeShown();
}

//=====================================================================================================================
// Pattern state and units
//=====================================================================================================================

//---------------------------------------------------------------------------------------------------------------------
/** @brief patternType returns the measurements type of the open pattern. */
MeasurementsType VAbstractApplication::patternType() const
{
    return _patternType;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief setPatternType sets the measurements type of the open pattern. */
void VAbstractApplication::setPatternType(const MeasurementsType &patternType)
{
    _patternType = patternType;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief patternUnit returns the pattern's measurement unit. */
Unit VAbstractApplication::patternUnit() const
{
    return _patternUnit;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief patternUnitP returns a pointer to the pattern unit, for callers that track changes. */
const Unit *VAbstractApplication::patternUnitP() const
{
    return &_patternUnit;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief setPatternUnit sets the pattern's measurement unit. */
void VAbstractApplication::setPatternUnit(const Unit &patternUnit)
{
    _patternUnit = patternUnit;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief toPixel converts a value in the pattern unit to pixels. */
double VAbstractApplication::toPixel(double val) const
{
    return ToPixel(val, _patternUnit);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief fromPixel converts pixels to a value in the pattern unit. */
double VAbstractApplication::fromPixel(double pix) const
{
    return FromPixel(pix, _patternUnit);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief getOpeningPattern reports whether a pattern file is being opened. */
bool VAbstractApplication::getOpeningPattern() const
{
    return openingPattern;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief setOpeningPattern toggles the opening-pattern flag; it does not set it. */
void VAbstractApplication::setOpeningPattern()
{
    openingPattern = !openingPattern;
}

//=====================================================================================================================
// Document and variable data
//=====================================================================================================================

//---------------------------------------------------------------------------------------------------------------------
/** @brief setCurrentDocument sets the active pattern document. */
void VAbstractApplication::setCurrentDocument(VAbstractPattern *doc)
{
    this->doc = doc;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief getCurrentDocument returns the active pattern document; asserts it is set. */
VAbstractPattern *VAbstractApplication::getCurrentDocument() const
{
    SCASSERT(doc != nullptr)
    return doc;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief setCurrentData sets the active variable container. */
void VAbstractApplication::setCurrentData(VContainer *data)
{
    this->data = data;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief getCurrentData returns the active variable container; asserts it is set. */
VContainer *VAbstractApplication::getCurrentData() const
{
    SCASSERT(data != nullptr)
    return data;
}

//=====================================================================================================================
// Main window, undo stack, and scene
//=====================================================================================================================

//---------------------------------------------------------------------------------------------------------------------
/** @brief getMainWindow returns the main window widget. */
QWidget *VAbstractApplication::getMainWindow() const
{
    return mainWindow;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief setMainWindow sets the main window widget; asserts it is not null. */
void VAbstractApplication::setMainWindow(QWidget *value)
{
    SCASSERT(value != nullptr)
    mainWindow = value;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief getUndoStack returns the shared undo stack. */
QUndoStack *VAbstractApplication::getUndoStack() const
{
    return undoStack;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief getCurrentScene returns the current graphics scene; asserts it is set. */
QGraphicsScene *VAbstractApplication::getCurrentScene() const
{
    SCASSERT(*currentScene != nullptr)
    return *currentScene;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief setCurrentScene stores the address of the caller's current-scene pointer. */
void VAbstractApplication::setCurrentScene(QGraphicsScene **value)
{
    currentScene = value;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief getSceneView returns the main graphics view. */
VMainGraphicsView *VAbstractApplication::getSceneView() const
{
    return sceneView;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief setSceneView sets the main graphics view. */
void VAbstractApplication::setSceneView(VMainGraphicsView *value)
{
    sceneView = value;
}

//=====================================================================================================================
// Translations
//=====================================================================================================================

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief translationsPath return path to the root directory that contains QM files.
 * @param locale historic, not used
 * @return path to a directory that contains QM files, default from CONFIG+=embed_translations as set in translations.pri
 */
QString VAbstractApplication::translationsPath(const QString &locale) const
{
    Q_UNUSED(locale)
    return QStringLiteral(":/i18n/");
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief loadTranslations loads and installs the Qt and application translators for a locale. */
void VAbstractApplication::loadTranslations(const QString &locale)
{
    if (locale.isEmpty())
    {
        qInfo() << "Locale is empty.";
        return;
    }
    qInfo() << "Checked locale:" << locale;

    ClearTranslation();

    qtTranslator     = new QTranslator(this);
    qtxmlTranslator  = new QTranslator(this);
    qtBaseTranslator = new QTranslator(this);
    appTranslator    = new QTranslator(this);
    pmsTranslator    = new QTranslator(this);

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    // Explicitly cast to void to suppress clang warnings
    (void)qtTranslator->load("qt_" + locale, translationsPath(locale));
    (void)qtxmlTranslator->load("qtxmlpatterns_" + locale, translationsPath(locale));
    (void)qtBaseTranslator->load("qtbase_" + locale, translationsPath(locale));
#else
    // Explicitly cast to void to suppress clang warnings
    (void)qtTranslator->load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    (void)qtxmlTranslator->load("qtxmlpatterns_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    (void)qtBaseTranslator->load("qtbase_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
#endif

    // Explicitly cast to void to suppress clang warnings
    (void)appTranslator->load("seamly2d_" + locale, translationsPath(locale));
    (void)pmsTranslator->load("measurements_" + locale, translationsPath(locale));

    installTranslator(qtTranslator);
    installTranslator(qtxmlTranslator);
    installTranslator(qtBaseTranslator);
    installTranslator(appTranslator);
    installTranslator(pmsTranslator);

    initTranslateVariables();//Very important do it after load QM files.
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief ClearTranslation removes and deletes every installed translator. */
void VAbstractApplication::ClearTranslation()
{
    if (!qtTranslator.isNull())
    {
        removeTranslator(qtTranslator);
        delete qtTranslator;
    }

    if (!qtxmlTranslator.isNull())
    {
        removeTranslator(qtxmlTranslator);
        delete qtxmlTranslator;
    }

    if (!qtBaseTranslator.isNull())
    {
        removeTranslator(qtBaseTranslator);
        delete qtBaseTranslator;
    }

    if (!appTranslator.isNull())
    {
        removeTranslator(appTranslator);
        delete appTranslator;
    }

    if (!pmsTranslator.isNull())
    {
        removeTranslator(pmsTranslator);
        delete pmsTranslator;
    }
}
