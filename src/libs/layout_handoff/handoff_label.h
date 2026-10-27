//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_label.h
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

#ifndef HANDOFF_LABEL_H
#define HANDOFF_LABEL_H

#include <QFont>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QStringList>
#include <QVector>

#include "../vpatterndb/LabelTextManager.h"

class VAbstractPattern;
class VContainer;
class VPatternLabelData;
class VPiece;

namespace layout_handoff
{
/**
 * @brief HandoffLabel is the piece label or the pattern label of one piece.
 * @details The label keeps its shape and text lines when it is hidden, so a hidden label does not lose its data.
 *          The caller applies the show-labels setting.
 */
class HandoffLabel
{
public:
    /**
     * @brief create_piece_label reads the piece label of a pattern piece.
     * @param piece        pattern piece.
     * @param pattern      pattern data; must not be null.
     * @param piece_bounds bounds of the piece, from HandoffPiece::bounds(); the label shifts into them.
     * @param font         base font of the label text.
     * @details The text placeholders read the current document of the application, which must not be null.
     */
    static HandoffLabel create_piece_label(const VPiece &piece, const VContainer *pattern,
                                           const QRectF &piece_bounds, const QFont &font);

    /**
     * @brief create_pattern_label reads the pattern label of a pattern piece.
     * @param data         pattern label data of the piece.
     * @param document     pattern document that holds the label template; must not be null.
     * @param pattern      pattern data; must not be null.
     * @param piece_bounds bounds of the piece, from HandoffPiece::bounds(); the label shifts into them.
     * @param font         base font of the label text.
     */
    static HandoffLabel create_pattern_label(const VPatternLabelData &data, VAbstractPattern *document,
                                             const VContainer *pattern, const QRectF &piece_bounds,
                                             const QFont &font);

    /**
     * @brief has_shape tells whether the label geometry is valid; without it the label has no shape and no text.
     */
    bool has_shape() const;

    /**
     * @brief is_visible returns the visible flag of the label data.
     */
    bool is_visible() const;

    /**
     * @brief shape returns the four corners of the label rectangle in piece coordinates.
     * @details The corners are rotated and shifted into the piece bounds. The first corner is the top left corner
     *          before the rotation.
     */
    const QVector<QPointF> &shape() const;

    /**
     * @brief font returns the base font of the text, at the size that fits the text into the label.
     */
    const QFont &font() const;

    /**
     * @brief spacing returns the space between two text lines, in pixels.
     */
    int spacing() const;

    const QList<TextLine> &lines() const;

    /**
     * @brief text returns the text of the lines; empty when the label has no shape.
     */
    QStringList text() const;

private:
    bool             m_is_visible = false;
    QVector<QPointF> m_shape;
    QFont            m_font;
    int              m_spacing = 0;
    QList<TextLine>  m_lines;
};
} // namespace layout_handoff

#endif // HANDOFF_LABEL_H
