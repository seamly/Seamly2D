//-----------------------------------------------------------------------------
//  @file   vsettings.h
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
//  @file   vsettings.h
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

#ifndef VSETTINGS_H
#define VSETTINGS_H

#include <QMetaObject>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QtGlobal>
#include <QMargins>

#include "../vmisc/def.h"
#include "vcommonsettings.h"

template <class T> class QSharedPointer;

class VSettings : public VCommonSettings
{
    Q_OBJECT
public:
                   VSettings(Format format, Scope scope, const QString &organization,
                             const QString &application = QString(), QObject *parent = nullptr);

                   VSettings(const QString &fileName, Format format, QObject *parent = nullptr);

    QString        getPointNameLanguage() const;
    void           setPointNameLanguage(const QString &value);

    static QString getDefaultPatternPath();
    QString        getPatternPath() const;
    void           SetPathPattern(const QString &value);

    static QString getSamplePatternsPath();
    static int     SeedSamplePatterns(const QString &sourceDir, const QString &destinationDir);

    static QString getSampleMeasurementsIndividualPath();
    static QString getSampleMeasurementsMultisizePath();
    static int     SeedSampleMeasurements(const QString &sourceDir, const QString &destinationDir,
                                           const QString &nameFilter);

    static QString getDefaultLayoutPath();
    QString        getLayoutPath() const;
    void           SetPathLayout(const QString &value);

    QString        getSeamlyLayoutAppPath() const;
    void           setSeamlyLayoutAppPath(const QString &value);

    bool           GetGraphicalOutput() const;
    void           SetGraphicalOutput(const bool &value);

    QString        GetServer() const;
    void           SetServer(const QString &value);

    bool           GetServerSecure() const;
    void           SetServerSecure(const bool &value);

    bool           GetProxy() const;
    void           SetProxy(const bool &value);

    QString        GetProxyAddress() const;
    void           SetProxyAddress(const QString &value);

    QString        GetProxyPort() const;
    void           SetProxyPort(const QString &value);

    QString        GetProxyUser() const;
    void           SetProxyUser(const QString &value);

    QString        GetProxyPass() const;
    void           SetProxyPass(const QString &value);

    QString        GetUsername() const;
    void           SetUsername(const QString &value);

    bool           GetSavePassword() const;
    void                     SetSavePassword(const bool &value);

    QString        GetUserPassword() const;
    void           SetUserPassword(const QString &value);

private:
    Q_DISABLE_COPY(VSettings)
};

#endif // VSETTINGS_H
