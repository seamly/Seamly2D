/******************************************************************************
 **  @file   new_piece_defaults.cpp
 **  @author slspencer
 **
 **  @brief
 **  Default label and grainline values for a new pattern piece, read from
 **  Preferences > Pattern and converted to pattern units.
 **
 **  @copyright
 **  This source code is part of the Seamly2D project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
 **
 **  SPDX-License-Identifier: GPL-3.0-or-later
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

#include "new_piece_defaults.h"

#include <QFileInfo>

#include "../ifc/xml/vlabeltemplateconverter.h"
#include "../vformat/vlabeltemplate.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// @brief settingToPatternUnits converts a Preferences length, stored in application units, to pattern units.
qreal settingToPatternUnits(qreal value, Unit patternUnit)
{
    return UnitConvertor(value, StrToUnits(qApp->Settings()->getUnit()), patternUnit);
}
}

//---------------------------------------------------------------------------------------------------------------------
qreal NewPieceDefaults::labelWidth(Unit patternUnit)
{
    return settingToPatternUnits(qApp->Settings()->getDefaultLabelWidth(), patternUnit);
}

//---------------------------------------------------------------------------------------------------------------------
qreal NewPieceDefaults::labelHeight(Unit patternUnit)
{
    return settingToPatternUnits(qApp->Settings()->getDefaultLabelHeight(), patternUnit);
}

//---------------------------------------------------------------------------------------------------------------------
qreal NewPieceDefaults::arrowLength(Unit patternUnit)
{
    // The arrow length setting is stored in pixels, not in application units.
    return FromPixel(qApp->Settings()->getDefaultArrowLength(), patternUnit);
}

//---------------------------------------------------------------------------------------------------------------------
qreal NewPieceDefaults::grainlineLength(Unit patternUnit)
{
    return fitGrainlineLength(settingToPatternUnits(qApp->Settings()->getDefaultGrainlineLength(), patternUnit),
                              arrowLength(patternUnit));
}

//---------------------------------------------------------------------------------------------------------------------
qreal NewPieceDefaults::fitGrainlineLength(qreal length, qreal arrowLength)
{
    qreal fitted = length;
    if (length < arrowLength * 2)
    {
        fitted = arrowLength * 2.1;
    }
    return fitted;
}

//---------------------------------------------------------------------------------------------------------------------
QVector<VLabelTemplateLine> NewPieceDefaults::readLabelTemplate(const QString &userFile, const QString &builtInFile)
{
    // The user's file wins. The resource copy covers a data root that was never seeded or a template that was deleted.
    QString filename = userFile;
    if (!QFileInfo::exists(filename))
    {
        filename = builtInFile;
    }

    QVector<VLabelTemplateLine> lines;
    if (QFileInfo::exists(filename))
    {
        VLabelTemplate labelTemplate;
        labelTemplate.setXMLContent(VLabelTemplateConverter(filename).Convert());
        lines = labelTemplate.ReadLines();
    }
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
QVector<VLabelTemplateLine> NewPieceDefaults::pieceLabelTemplate()
{
    return readLabelTemplate(qApp->Settings()->getDefaultPieceTemplate(),
                             VCommonSettings::builtInPieceLabelTemplate());
}

//---------------------------------------------------------------------------------------------------------------------
QVector<VLabelTemplateLine> NewPieceDefaults::patternLabelTemplate()
{
    return readLabelTemplate(qApp->Settings()->getDefaultPatternTemplate(),
                             VCommonSettings::builtInPatternLabelTemplate());
}
