//-----------------------------------------------------------------------------
//  @file   application_me.h
//  @author Douglas S Caskey
//  @date   7 Mar, 2024
//
//  @brief
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program to create and model patterns of clothing.
//  Copyright (C) 2017-2026 Seamly2D project
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
//  @file   mapplication.h
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date   8 7, 2015
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

#ifndef APPLICATION_ME_H
#define APPLICATION_ME_H

#include "../vpatterndb/vtranslatevars.h"
#include "../vmisc/def.h"
#include "../vmisc/vlockguard.h"
#include "../vmisc/vseamlymesettings.h"
#include "../vmisc/vabstractapplication.h"
#include "dialogs/database_dialog.h"

#include <QFile>
#include <QTextStream>
#include <memory>

class ApplicationME;// use in define
class TMainWindow;
class QLocalServer;

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<ApplicationME*>(VAbstractApplication::instance()))

enum class SocketConnection : bool {Client = false, Server = true};

class ApplicationME : public VAbstractApplication
{
    Q_OBJECT

public:
                                        ApplicationME(int &argc, char **argv);
    virtual                            ~ApplicationME() override;

    void                                setTheme();

    virtual bool                        notify(QObject * receiver, QEvent * event) override;

    bool                                isTestMode() const;
    virtual bool                        isAppInGUIMode() const override;
    TMainWindow                        *mainWindow();
    QList<TMainWindow*>                 mainWindows();
    TMainWindow                        *newMainWindow();

    void                                initOptions();

    virtual const VTranslateVars       *translateVariables() override;

    virtual void                        openSettings() override;
    VSeamlyMeSettings                  *seamlyMeSettings();

    // Task 15: true once this run's openSettings() pulled settings forward from the
    // legacy "Seamly2DTeam" organization folder. main.cpp shows the one-time migration
    // notice using this, gated the same way as the welcome dialog (skipped in --test
    // mode), since m_testMode itself is not set yet this early in startup.
    bool                                settingsMigrated() const { return m_settingsMigrated; }

    void                                showDataBase();
    void                                retranslateGroups();
    void                                retranslateTables();
    void                                parseCommandLine(const SocketConnection &connection,
                                                         const QStringList &arguments);

    // Task SeamlyMe.5: log-file startup, mirroring Application2D. startLogging() creates
    // %LOCALAPPDATA%\Seamly\SeamlyMe\logs (Windows), opens a per-pid log, and prunes old
    // logs; logFile() is null until beginLogging() has opened the file, so the message
    // handler must check it.
    void                                startLogging();
    QTextStream                        *logFile();

public slots:
    void                                processCommandLine();

protected:
    virtual void                        initTranslateVariables() override;
    virtual bool                        event(QEvent *e) override;

private slots:
    void                                newLocalSocketConnection();

private:
    Q_DISABLE_COPY(ApplicationME)
    QList<QPointer<TMainWindow>>        m_mainWindows;
    QLocalServer                       *m_localServer;
    VTranslateVars                     *m_trVars;
    QPointer<MeasurementDatabaseDialog> m_dataBase;
    bool                                m_testMode;

    // Task 15: set true by openSettings() when it pulls settings forward from the legacy
    // "Seamly2DTeam" organization folder; main.cpp shows the one-time migration notice
    // only when this is set and the app is not running in automated test mode.
    bool                                m_settingsMigrated{false};

    // Task SeamlyMe.5: per-pid log file, locked so parallel SeamlyMe processes never
    // share one file — the same mechanism Application2D uses.
    std::shared_ptr<VLockGuard<QFile>>  m_lockLog;
    std::shared_ptr<QTextStream>        m_out;

    QString                             logDirPath() const;
    QString                             logPath() const;
    bool                                createLogDir() const;
    void                                beginLogging();
    void                                clearOldLogs() const;

    void                                clean();
};

#endif // APPLICATION_ME_H
