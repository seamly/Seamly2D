//  @file   vtoolcut.h
//  @author Douglas S Caskey
//  @date   8 Jun, 2024
//
//  @copyright
//  Copyright (C) 2017 - 2024 Seamly, LLC
//  https://github.com/fashionfreedom/seamly2d
//
//  @brief
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
//  along with Seamly2D. If not, see <http://www.gnu.org/licenses/>.

/************************************************************************
 **
 **  @file   vtoolcut.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   25 6, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentine project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Seamly2D project
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

#ifndef VTOOLCUT_H
#define VTOOLCUT_H

#include <qcompilerdetection.h>
#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../../../toolcurve/vabstractspline.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vtoolsinglepoint.h"
#include "../vmisc/def.h"
#include "../../../../vdatatool.h"
#include "../../../../../visualization/visualization.h"

class VFormula;

class VToolCut : public VToolSinglePoint
{
    Q_OBJECT
public:
                  VToolCut(VAbstractPattern *doc, VContainer *data, const quint32 &id, QString &direction,
                           const QString &formula, const quint32 &curveCutId, QGraphicsItem * parent = nullptr);

    virtual int   type() const Q_DECL_OVERRIDE {return Type;}
    enum { Type = UserType + static_cast<int>(Tool::Cut)};

    VFormula      GetFormula() const;
    void          SetFormula(const VFormula &value);

    QString       CurveName() const;

    quint32       getCurveCutId() const;
    void          setCurveCutId(const quint32 &value);

    QString       getDirection() const;
    void          setDirection(const QString &value);

public slots:
    virtual void  Disable(bool disable, const QString &draftBlockName) Q_DECL_OVERRIDE;
    virtual void  piecesMode(bool mode) Q_DECL_OVERRIDE;
    virtual void  FullUpdateFromFile() Q_DECL_OVERRIDE;

protected:
    QString       m_direction;
    QString       formula;      /*!< @brief formula keep formula of length */
    quint32       curveCutId;
    bool          m_piecesMode;

    void          RefreshGeometry();
    virtual void  RemoveReferens() Q_DECL_OVERRIDE;

    template <typename T>
    void          ShowToolVisualization(bool show);

private:
    Q_DISABLE_COPY(VToolCut)
};

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
inline void VToolCut::ShowToolVisualization(bool show)
{
    if (show)
    {
        if (vis.isNull())
        {
            AddVisualization<T>();
            SetVisualization();
        }
        else
        {
            if (T *visual = qobject_cast<T *>(vis))
            {
                visual->show();
            }
        }
    }
    else
    {
        delete vis;
    }

    VDataTool *parent = VAbstractPattern::getTool(VAbstractTool::data.GetGObject(curveCutId)->getIdTool());
    if (VAbstractSpline *parentCurve = qobject_cast<VAbstractSpline *>(parent))
    {
        m_piecesMode ? parentCurve->showHandles(m_piecesMode) : parentCurve->showHandles(show);
    }
}
#endif // VTOOLCUT_H
