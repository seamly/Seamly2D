//  @file   vabstractapplication.cpp
//  @author Douglas S Caskey
//  @date   13 JUl, 2025
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

//---------------------------------------------------------------------------------------------------------------------
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
        // If try to use the method QApplication::exit program can't sync settings and show warning about QApplication
        // instance. Solution is to call sync() before quit.
        // Connect this slot with Application2D::aboutToQuit.
        Settings()->sync();
    });
}

//---------------------------------------------------------------------------------------------------------------------
VAbstractApplication::~VAbstractApplication()
{}

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
MeasurementsType VAbstractApplication::patternType() const
{
    return _patternType;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractApplication::setPatternType(const MeasurementsType &patternType)
{
    _patternType = patternType;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractApplication::setCurrentDocument(VAbstractPattern *doc)
{
    this->doc = doc;
}

//---------------------------------------------------------------------------------------------------------------------
VAbstractPattern *VAbstractApplication::getCurrentDocument() const
{
    SCASSERT(doc != nullptr)
    return doc;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractApplication::setCurrentData(VContainer *data)
{
    this->data = data;
}

//---------------------------------------------------------------------------------------------------------------------
VContainer *VAbstractApplication::getCurrentData() const
{
    SCASSERT(data != nullptr)
    return data;
}

//---------------------------------------------------------------------------------------------------------------------
bool VAbstractApplication::getOpeningPattern() const
{
    return openingPattern;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractApplication::setOpeningPattern()
{
    openingPattern = !openingPattern;
}

//---------------------------------------------------------------------------------------------------------------------
QWidget *VAbstractApplication::getMainWindow() const
{
    return mainWindow;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractApplication::setMainWindow(QWidget *value)
{
    SCASSERT(value != nullptr)
    mainWindow = value;
}

//---------------------------------------------------------------------------------------------------------------------
QUndoStack *VAbstractApplication::getUndoStack() const
{
    return undoStack;
}

//---------------------------------------------------------------------------------------------------------------------
Unit VAbstractApplication::patternUnit() const
{
    return _patternUnit;
}

//---------------------------------------------------------------------------------------------------------------------
const Unit *VAbstractApplication::patternUnitP() const
{
    return &_patternUnit;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractApplication::setPatternUnit(const Unit &patternUnit)
{
    _patternUnit = patternUnit;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getSettings hide settings constructor.
 * @return pointer to class for acssesing to settings in ini file.
 */
VCommonSettings *VAbstractApplication::Settings()
{
    SCASSERT(m_settings != nullptr)
    return m_settings;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MigrateSeamlySettingsLocation resolves this app's settings-file path under the
 * unified "Seamly" organization (Task 15), migrating the file forward from the
 * pre-unification "SeamlyTeam" organization folder on first run after an upgrade.
 *
 * Before Task 15, seamly2d and seamlyme each stored one flat .ini file (named after the
 * application) as a sibling inside a single shared "SeamlyTeam" organization folder
 * (Qt's native IniFormat/UserScope resolution). After Task 15 every Seamly application
 * gets its own directory nested under the "Seamly" organization instead — QStandardPaths
 * ::AppConfigLocation already resolves to AppData/Local/Seamly/<AppName> on Windows once
 * the organization name is "Seamly" and the application name is set, matching the layout
 * seamlyLayout already uses via QStandardPaths internally. This function creates that new
 * per-app directory and — only if its settings file does not already exist there — copies
 * it in from whichever legacy filename candidate is found, so upgrading is non-destructive
 * and re-entrant (running it again after the file exists is a cheap no-op).
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

    // New home: one directory per application, nested under the shared "Seamly"
    // organization folder (AppData/Local/Seamly/<AppName> on Windows).
    const QString newAppDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(newAppDir);
    const QString newAppIniPath = QDir(newAppDir).filePath(appIniFileName);

    if (QFileInfo::exists(newAppIniPath))
    {
        // Already migrated on a previous run, or a fresh install with nothing to bring
        // forward — either way there is nothing left to do.
        return newAppIniPath;
    }

    // Legacy home: pre-Task-15 builds used organization name "SeamlyTeam", with both
    // apps' settings living as sibling flat files in that one shared folder.
    static const QString kLegacyOrganizationName = QStringLiteral("SeamlyTeam");
    const QSettings legacyProbe(QSettings::IniFormat, QSettings::UserScope,
                                kLegacyOrganizationName, QCoreApplication::applicationName());
    const QString legacyDir = QFileInfo(legacyProbe.fileName()).absolutePath();

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
        } // if legacy file found and copied
    } // for each candidate legacy filename

    return newAppIniPath;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NotifySeamlySettingsMigrated shows a one-time informational dialog after Task 15
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
QGraphicsScene *VAbstractApplication::getCurrentScene() const
{
    SCASSERT(*currentScene != nullptr)
    return *currentScene;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractApplication::setCurrentScene(QGraphicsScene **value)
{
    currentScene = value;
}

//---------------------------------------------------------------------------------------------------------------------
VMainGraphicsView *VAbstractApplication::getSceneView() const
{
    return sceneView;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractApplication::setSceneView(VMainGraphicsView *value)
{
    sceneView = value;
}

//---------------------------------------------------------------------------------------------------------------------
double VAbstractApplication::toPixel(double val) const
{
    return ToPixel(val, _patternUnit);
}

//---------------------------------------------------------------------------------------------------------------------
double VAbstractApplication::fromPixel(double pix) const
{
    return FromPixel(pix, _patternUnit);
}

//---------------------------------------------------------------------------------------------------------------------
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
