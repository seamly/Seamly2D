/************************************************************************
 **
 **  @file   vcmdexport.h
 **  @author Alex Zaharov <alexzkhr@gmail.com>
 **  @date   25 8, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentine project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Seamly2D project
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
 *************************************************************************/

#ifndef VCMDEXPORT_H
#define VCMDEXPORT_H

#include <memory>
#include <vector>
#include <QTextStream>
#include <QCoreApplication>
#include <QCommandLineParser>

#include "../vmisc/def.h"
#include "../vmisc/vsysexits.h"

class VCommandLine;
typedef std::shared_ptr<VCommandLine> VCommandLinePtr;
typedef QList<QCommandLineOption *> VCommandLineOptions;

class VCommandLine
{
public:
    virtual               ~VCommandLine();

    bool                   IsTestModeEnabled() const;
    bool                   IsNoScalingEnabled() const;
    QString                OptMeasurePath() const;
    QStringList            OptInputFileNames() const;
    bool                   IsGuiEnabled()const;
    bool                   IsSetGradationSize() const;
    bool                   IsSetGradationHeight() const;
    QString                OptGradationSize() const;
    QString                OptGradationHeight() const;

protected:
    VCommandLine();

    static void            Reset();
    static VCommandLinePtr Get(const QCoreApplication& app);

private:
                           Q_DISABLE_COPY(VCommandLine)
    static VCommandLinePtr commandLine;
    QCommandLineParser     parser;
    VCommandLineOptions    optionsUsed;
    QMap<QString, int>     optionsIndex;
    bool                   isGuiEnabled;
    friend class           Application2D;

    static void            initOptions(VCommandLineOptions &options,
                                       QMap<QString, int> &optionsIndex);
};

#endif // VCMDEXPORT_H
