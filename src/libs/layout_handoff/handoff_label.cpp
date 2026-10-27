//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_label.cpp
//  @author slspencer
//
//  @brief
//  Piece label and pattern label of one piece in the SeamlyLayout handoff.
//
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2026 Seamly2D Project
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
//---------------------------------------------------------------------------------------------------------------------

#include "handoff_label.h"

#include "handoff_geometry.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/def.h"
#include "../vmisc/vmath.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QtMath>

namespace layout_handoff
{
namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief find_label_geometry reads the position, size and rotation of a label from its data.
 * @details Two anchors give the rectangle. Otherwise the width and height come from the data, and the position
 *          from the center anchor or the stored position.
 * @param angle  receives the rotation in degrees.
 * @param width  receives the label width in pixels.
 * @param height receives the label height in pixels.
 * @param pos    receives the top left corner before the rotation.
 * @return false when a formula of the data does not evaluate.
 */
bool find_label_geometry(const VPatternLabelData &data, const VContainer *pattern, qreal *angle, qreal *width,
                         qreal *height, QPointF *pos)
{
    try
    {
        Calculator calculator;
        *angle = calculator.EvalFormula(pattern->DataVariables(), data.getRotation());
    }
    catch (qmu::QmuParserError &error)
    {
        Q_UNUSED(error);
        return false;
    }

    const quint32 top_left_id     = data.topLeftAnchorPoint();
    const quint32 bottom_right_id = data.bottomRightAnchorPoint();
    if (top_left_id != NULL_ID && bottom_right_id != NULL_ID)
    {
        try
        {
            const auto top_left     = pattern->GeometricObject<VPointF>(top_left_id);
            const auto bottom_right = pattern->GeometricObject<VPointF>(bottom_right_id);
            const QRectF rect(static_cast<QPointF>(*top_left), static_cast<QPointF>(*bottom_right));
            *width  = qAbs(rect.width());
            *height = qAbs(rect.height());
            *pos    = rect.topLeft();
            return true;
        }
        catch (const VExceptionBadId &)
        {
            // Missing anchor: use the width, height and position of the data.
        }
    }

    try
    {
        Calculator width_calculator;
        *width = width_calculator.EvalFormula(pattern->DataVariables(), data.GetLabelWidth());
        *width = ToPixel(*width, *pattern->GetPatternUnit());

        Calculator height_calculator;
        *height = height_calculator.EvalFormula(pattern->DataVariables(), data.GetLabelHeight());
        *height = ToPixel(*height, *pattern->GetPatternUnit());
    }
    catch (qmu::QmuParserError &error)
    {
        Q_UNUSED(error);
        return false;
    }

    *pos = data.GetPos();
    const quint32 center_id = data.centerAnchorPoint();
    if (center_id != NULL_ID)
    {
        try
        {
            const auto center = pattern->GeometricObject<VPointF>(center_id);
            *pos = static_cast<QPointF>(*center) - QRectF(0, 0, *width, *height).center();
        }
        catch (const VExceptionBadId &)
        {
            // Missing anchor: keep the stored position.
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief rotate_point rotates a point around a center.
 * @param angle rotation in radians.
 */
QPointF rotate_point(const QPointF &center, const QPointF &point, qreal angle)
{
    const QPointF relative = point - center;
    return QPointF(qCos(angle) * relative.x() - qSin(angle) * relative.y(),
                   qSin(angle) * relative.x() + qCos(angle) * relative.y()) + center;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_shape returns the label rectangle, rotated around its center and shifted into the piece bounds.
 */
QVector<QPointF> make_shape(const QPointF &pos, qreal width, qreal height, qreal angle, const QRectF &piece_bounds)
{
    QVector<QPointF> shape;
    shape << pos
          << QPointF(pos.x() + width, pos.y())
          << QPointF(pos.x() + width, pos.y() + height)
          << QPointF(pos.x(), pos.y() + height);

    const QPointF center(pos.x() + width / 2, pos.y() + height / 2);
    for (QPointF &corner : shape)
    {
        corner = rotate_point(center, corner, qDegreesToRadians(-angle));
    }
    return keep_inside(piece_bounds, shape);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
HandoffLabel HandoffLabel::create_piece_label(const VPiece &piece, const VContainer *pattern,
                                              const QRectF &piece_bounds, const QFont &font)
{
    HandoffLabel label;
    const VPieceLabelData &data = piece.GetPatternPieceData();
    label.m_is_visible          = data.IsVisible();

    qreal   angle  = 0;
    qreal   width  = 0;
    qreal   height = 0;
    QPointF pos;
    if (find_label_geometry(data, pattern, &angle, &width, &height, &pos))
    {
        label.m_shape = make_shape(pos, width, height, angle, piece_bounds);

        LabelTextManager manager;
        manager.setFont(font);
        manager.SetFontSize(data.getFontSize());
        manager.Update(piece.GetName(), data);
        manager.SetFontSize(data.getFontSize());
        manager.FitFontSize(width, height);

        label.m_font    = manager.GetFont();
        label.m_spacing = manager.GetSpacing();
        label.m_lines   = manager.GetAllSourceLines();
    }
    return label;
}

//---------------------------------------------------------------------------------------------------------------------
HandoffLabel HandoffLabel::create_pattern_label(const VPatternLabelData &data, VAbstractPattern *document,
                                                const VContainer *pattern, const QRectF &piece_bounds,
                                                const QFont &font)
{
    HandoffLabel label;
    label.m_is_visible = data.IsVisible();

    qreal   angle  = 0;
    qreal   width  = 0;
    qreal   height = 0;
    QPointF pos;
    if (find_label_geometry(data, pattern, &angle, &width, &height, &pos))
    {
        label.m_shape = make_shape(pos, width, height, angle, piece_bounds);

        LabelTextManager manager;
        manager.setFont(font);
        manager.SetFontSize(data.getFontSize());
        manager.Update(document);
        manager.SetFontSize(data.getFontSize());
        manager.FitFontSize(width, height);

        label.m_font    = manager.GetFont();
        label.m_spacing = manager.GetSpacing();
        label.m_lines   = manager.GetAllSourceLines();
    }
    return label;
}

//---------------------------------------------------------------------------------------------------------------------
bool HandoffLabel::has_shape() const
{
    return m_shape.size() > 2;
}

//---------------------------------------------------------------------------------------------------------------------
bool HandoffLabel::is_visible() const
{
    return m_is_visible;
}

//---------------------------------------------------------------------------------------------------------------------
const QVector<QPointF> &HandoffLabel::shape() const
{
    return m_shape;
}

//---------------------------------------------------------------------------------------------------------------------
const QFont &HandoffLabel::font() const
{
    return m_font;
}

//---------------------------------------------------------------------------------------------------------------------
int HandoffLabel::spacing() const
{
    return m_spacing;
}

//---------------------------------------------------------------------------------------------------------------------
const QList<TextLine> &HandoffLabel::lines() const
{
    return m_lines;
}

//---------------------------------------------------------------------------------------------------------------------
QStringList HandoffLabel::text() const
{
    QStringList text;
    if (has_shape())
    {
        for (const TextLine &line : m_lines)
        {
            text.append(line.text);
        }
    }
    return text;
}
} // namespace layout_handoff
