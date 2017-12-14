/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2017  Seamly, LLC                                       *
 *                                                                         *
 *   https://github.com/fashionfreedom/seamly2d                             *
 *                                                                         *
 ***************************************************************************
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
 **************************************************************************

 ************************************************************************
 **
 **  @file   vwidgetdetails.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   25 6, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentine project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Seamly2D project
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

#ifndef VWIDGETDETAILS_H
#define VWIDGETDETAILS_H

#include <QWidget>

class VAbstractPattern;
class VContainer;
class VPiece;

namespace Ui
{
    class VWidgetDetails;
}

class VWidgetDetails : public QWidget
{
    Q_OBJECT

public:
    explicit VWidgetDetails(VContainer *data, VAbstractPattern *doc, QWidget *parent = nullptr);
    virtual ~VWidgetDetails();

signals:
    void Highlight(quint32 id);

public slots:
    void UpdateList();
    void SelectDetail(quint32 id);

private slots:
    void InLayoutStateChanged(int row, int column);
    void ShowContextMenu(const QPoint &pos);

private:
    Q_DISABLE_COPY(VWidgetDetails)
    Ui::VWidgetDetails *ui;
    VAbstractPattern   *m_doc;
    VContainer         *m_data;

    void FillTable(const QHash<quint32, VPiece> *details);
    void ToggleSectionDetails(bool select);
};

#endif // VWIDGETDETAILS_H