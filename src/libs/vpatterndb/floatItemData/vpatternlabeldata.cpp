/************************************************************************
 **
 **  @file   vpatternlabeldata.cpp
 **  @author Bojan Kverh
 **  @date   June 16, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentine project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2022 Seamly2D project
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

#include "vpatternlabeldata.h"
#include "vpatternlabeldata_p.h"
#include "../ifc/ifcdef.h"
#include "../vmisc/def.h"

#ifdef Q_COMPILER_RVALUE_REFS
VPatternLabelData &VPatternLabelData::operator=(VPatternLabelData &&data) noexcept
{ Swap(data); return *this; }
#endif

void VPatternLabelData::Swap(VPatternLabelData &data) noexcept
{ VAbstractFloatItemData::Swap(data); std::swap(d, data.d); }

//---------------------------------------------------------------------------------------------------------------------
VPatternLabelData::VPatternLabelData()
    : VAbstractFloatItemData()
    , d(new VPatternLabelDataPrivate())
{}

//---------------------------------------------------------------------------------------------------------------------
VPatternLabelData::VPatternLabelData(const VPatternLabelData &data)
    : VAbstractFloatItemData(data)
    , d (data.d)
{}

//---------------------------------------------------------------------------------------------------------------------
VPatternLabelData &VPatternLabelData::operator=(const VPatternLabelData &data)
{
    if ( &data == this )
    {
        return *this;
    }
    VAbstractFloatItemData::operator=(data);
    d = data.d;
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
VPatternLabelData::~VPatternLabelData()
{}

//---------------------------------------------------------------------------------------------------------------------
QString VPatternLabelData::GetLabelWidth() const
{
    return d->m_pieceLabelWidth;
}

//---------------------------------------------------------------------------------------------------------------------
void VPatternLabelData::SetLabelWidth(const QString &dLabelW)
{
    d->m_pieceLabelWidth = dLabelW;
}

//---------------------------------------------------------------------------------------------------------------------
QString VPatternLabelData::GetLabelHeight() const
{
    return d->m_pieceLabelHeight;
}

//---------------------------------------------------------------------------------------------------------------------
void VPatternLabelData::SetLabelHeight(const QString &dLabelH)
{
    d->m_pieceLabelHeight = dLabelH;
}

//---------------------------------------------------------------------------------------------------------------------
int VPatternLabelData::getFontSize() const
{
    return d->m_iFontSize;
}

//---------------------------------------------------------------------------------------------------------------------
void VPatternLabelData::SetFontSize(int iSize)
{
    d->m_iFontSize = iSize;
}

//---------------------------------------------------------------------------------------------------------------------
QString VPatternLabelData::getRotation() const
{
    return d->m_pieceLabelAngle;
}

//---------------------------------------------------------------------------------------------------------------------
void VPatternLabelData::SetRotation(const QString &dRot)
{
    d->m_pieceLabelAngle = dRot;
}

//---------------------------------------------------------------------------------------------------------------------
quint32 VPatternLabelData::centerAnchorPoint() const
{
    return d->m_centerAnchorPoint;
}

//---------------------------------------------------------------------------------------------------------------------
void VPatternLabelData::setCenterAnchorPoint(const quint32 &centerAnchorPoint)
{
    d->m_centerAnchorPoint = centerAnchorPoint;
}

//---------------------------------------------------------------------------------------------------------------------
quint32 VPatternLabelData::topLeftAnchorPoint() const
{
    return d->m_topLeftAnchorPoint;
}

//---------------------------------------------------------------------------------------------------------------------
void VPatternLabelData::setTopLeftAnchorPoint(const quint32 &topLeftAnchorPoint)
{
    d->m_topLeftAnchorPoint = topLeftAnchorPoint;
}

//---------------------------------------------------------------------------------------------------------------------
quint32 VPatternLabelData::bottomRightAnchorPoint() const
{
    return d->m_bottomRightAnchorPoint;
}

//---------------------------------------------------------------------------------------------------------------------
void VPatternLabelData::setBottomRightAnchorPoint(const quint32 &bottomRightAnchorPoint)
{
    d->m_bottomRightAnchorPoint = bottomRightAnchorPoint;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief hasCornerAnchors returns true when both the top left and the bottom right anchor points are set.
bool VPatternLabelData::hasCornerAnchors() const
{
    return d->m_topLeftAnchorPoint != NULL_ID && d->m_bottomRightAnchorPoint != NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief defaultPieceLabelPos returns the top left of a new piece label.
 * @param pieceRect bounding box of the piece main path, in piece local pixels.
 * @param labelSize label size in pixels.
 *
 * The label center is 1 cm right of the bounding box center, so the label does not sit on the grainline midpoint.
 */
QPointF VPatternLabelData::defaultPieceLabelPos(const QRectF &pieceRect, const QSizeF &labelSize)
{
    const QPointF center = pieceRect.center() + QPointF(ToPixel(1, Unit::Cm), 0);
    return center - QPointF(labelSize.width() / 2.0, labelSize.height() / 2.0);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief defaultPatternLabelPos returns the top left of a new pattern label.
 * @param pieceRect      bounding box of the piece main path, in piece local pixels.
 * @param labelSize      label size in pixels.
 * @param pieceLabelRect piece label at its default place, or a null rect when the piece has no such label.
 *
 * The label is vertically centered on the bounding box. Its left edge is 1 cm right of the piece label.
 * Without a piece label it takes the piece label place.
 */
QPointF VPatternLabelData::defaultPatternLabelPos(const QRectF &pieceRect, const QSizeF &labelSize,
                                                  const QRectF &pieceLabelRect)
{
    if (pieceLabelRect.isNull())
    {
        return defaultPieceLabelPos(pieceRect, labelSize);
    }
    return QPointF(pieceLabelRect.right() + ToPixel(1, Unit::Cm), pieceRect.center().y() - labelSize.height() / 2.0);
}
