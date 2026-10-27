//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_document.h
//  @author slspencer
//
//  @brief
//  The tagged pieces SVG that Seamly2D hands to SeamlyLayout.
//
//  @details
//  Data source: the open pattern in Seamly2D.
//  - VPiece: piece geometry and piece properties.
//  - VContainer: pattern data that calculates the piece points and lengths.
//  - VAbstractPattern: pattern document with the pattern label template.
//  - VCommonSettings: colors, line styles, label font and show flags.
//  Constraint:
//  - Reads no canvas items (QGraphicsScene), so the export does not depend on what the canvas shows.
//  Exception:
//  - Label placeholders read qApp state through LabelTextManager.
//
//  Classes of the layout_handoff library:
//  - HandoffDocument: SVG with one pattern group per material and one group per piece.
//  - HandoffPiece: seam line, cut line, notches, internal and cutout paths, grainline, fold line.
//  - HandoffLabel: piece label and pattern label.
//  - HandoffPieceInfo: piece properties for the data-* attributes.
//  - HandoffIds: unique SVG ids for pieces and components.
//  - handoff_turn_points: turn points matched across the sizes of a multisize piece.
//  - handoff_sheet: piece bounds and the flat sheet, with no QGraphicsScene.
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

#ifndef HANDOFF_DOCUMENT_H
#define HANDOFF_DOCUMENT_H

#include <QColor>
#include <QDomDocument>
#include <QFont>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVector>
#include <Qt>

#include "handoff_label.h"
#include "handoff_piece.h"

class VCommonSettings;
class VPiece;

namespace layout_handoff
{
/**
 * @brief HandoffLineStyle is the pen of one kind of line.
 */
struct HandoffLineStyle
{
    QColor       color;
    Qt::PenStyle type   = Qt::SolidLine;
    qreal        weight = 1.0; ///< pixels
};

/**
 * @brief HandoffStyle holds the colors, line styles and show flags that HandoffDocument applies.
 */
struct HandoffStyle
{
    HandoffLineStyle seam;
    HandoffLineStyle cut;
    HandoffLineStyle fold_line;
    QColor           notch_color;
    QColor           grainline_color;
    QColor           label_color;
    QFont            label_font;
    bool             show_labels           = true;
    bool             show_grainlines       = true;
    bool             show_seam_allowances  = true;

    /**
     * @brief from_settings reads the style from the application settings.
     */
    static HandoffStyle from_settings(const VCommonSettings &settings);
};

/**
 * @brief HandoffPieceInfo holds the piece properties that become the data-* attributes of the piece group.
 * @details Empty strings and unset numbers write no attribute.
 */
struct HandoffPieceInfo
{
    QString name;
    QString letter;
    QString material;
    QString size;         ///< multisize size in pattern units; empty for an individual pattern
    QString piece_set_key; ///< same value for the same piece in every size; empty = no piece set
    int     quantity = 1;
    int     rotation = 0; ///< rotation step in degrees; 0 = the piece may not rotate
    int     tilt     = 0; ///< largest tilt in degrees, negative = clockwise; 0 = the piece may not tilt
    QString flip;         ///< pieceFlipToString() value; empty = no flip
    QString orientation;  ///< "Left" or "Right"; empty = undefined
    QString annotation;

    /**
     * @brief create reads the piece properties of a pattern piece.
     * @param piece pattern piece.
     * @param size  multisize size in pattern units; empty for an individual pattern.
     * @param piece_set_key key of the piece set; empty = no piece set.
     */
    static HandoffPieceInfo create(const VPiece &piece, const QString &size, const QString &piece_set_key);
};

/**
 * @brief HandoffPlacedPiece is one piece of the document with the place of its origin on the sheet.
 */
struct HandoffPlacedPiece
{
    HandoffPieceInfo info;
    HandoffPiece     piece;
    HandoffLabel     piece_label;
    HandoffLabel     pattern_label;
    QPointF          offset;         ///< place of the piece origin on the sheet, in pixels
    bool             mirror = false; ///< true when the layout mirrors the label text
};

/**
 * @brief HandoffDocument writes the tagged pieces SVG that SeamlyLayout reads with --svg-stdin.
 * @details One pattern group per material, in the order of the first piece of each material. A piece is a group
 *          with one tagged group per component. Components that the piece does not have write no group.
 *          Hidden labels and hidden grainlines are not drawn, but the piece keeps data-grainline-angle.
 */
class HandoffDocument
{
public:
    void set_style(const HandoffStyle &style);
    void set_pattern_name(const QString &name);

    /**
     * @brief set_units sets data-units of the pattern group: "mm", "cm" or "inch"; empty writes no attribute.
     */
    void set_units(const QString &units);

    /**
     * @brief set_sample_size sets data-sample-size of an individual pattern; ignored for a multisize pattern.
     */
    void set_sample_size(const QString &sample_size);

    /**
     * @brief set_multisize marks the pattern as multisize.
     * @param sizes     every size, in pattern units, in the order the pieces are added.
     * @param base_size base size; empty writes no attribute.
     * @param height    height of every size; empty writes no attribute.
     */
    void set_multisize(const QStringList &sizes, const QString &base_size, const QString &height);

    /**
     * @brief set_sheet sets the view box of the SVG.
     * @param rect       sheet rectangle in pixels.
     * @param resolution dots per inch; gives the width and height in mm.
     */
    void set_sheet(const QRectF &rect, int resolution);

    void add_piece(const HandoffPlacedPiece &placed);

    /**
     * @brief to_dom builds the document; a document with no piece has no pattern group.
     */
    QDomDocument to_dom() const;

    QString to_svg_string() const;

private:
    HandoffStyle                m_style;
    QString                     m_pattern_name;
    QString                     m_units;
    QString                     m_sample_size;
    QStringList                 m_sizes;
    QString                     m_base_size;
    QString                     m_height;
    QRectF                      m_sheet;
    int                         m_resolution = 96;
    QVector<HandoffPlacedPiece> m_pieces;
};
} // namespace layout_handoff

#endif // HANDOFF_DOCUMENT_H
