//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_ids.h
//  @author slspencer
//
//  @brief
//  Unique SVG ids for the pieces and components of the SeamlyLayout handoff.
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

#ifndef HANDOFF_IDS_H
#define HANDOFF_IDS_H

#include <QSet>
#include <QString>

namespace layout_handoff
{
/**
 * @brief sanitize_for_id turns a name into a CSS-selector-safe SVG id fragment.
 * @return the name with each run of characters outside [A-Za-z0-9_.-] replaced by '_', edge '_' trimmed, and a '_'
 *         added before a leading digit. Empty when nothing usable remains: the caller supplies a fallback id.
 */
QString sanitize_for_id(const QString &name);

/**
 * @brief size_id_suffix returns the id suffix of one size of a multisize piece, such as "_s34" or "_s13.5".
 */
QString size_id_suffix(const QString &size);

/**
 * @brief component_id_is_numbered tells whether a component type can repeat in one piece.
 * @details Such a component id keeps its counter: "<type>_<n>_<name>" instead of "<type>_<name>".
 */
bool component_id_is_numbered(const QString &type);

/**
 * @brief HandoffIds hands out piece and component ids that are unique in one handoff document.
 */
class HandoffIds
{
public:
    /**
     * @brief piece_id returns the id of a piece group.
     * @param fallback_id  id used when the piece name gives no usable text; unique by construction.
     * @param piece_name   piece name.
     * @param size         multisize size in the data-units unit; empty for an individual pattern.
     * @param piece_number number of the piece in the document; makes a repeated id unique.
     */
    QString piece_id(const QString &fallback_id, const QString &piece_name, const QString &size, int piece_number);

    /**
     * @brief piece_set_id returns the id of a piece-set group, which holds the sizes of one piece.
     * @param piece_name  piece name.
     * @param set_number  number of the piece set in the document; makes a repeated id unique.
     */
    QString piece_set_id(const QString &piece_name, int set_number);

    /**
     * @brief component_id returns the id of a component group of a piece.
     * @param type         component data-type, such as "grainline".
     * @param type_number  count of components of this type in the piece, from 1.
     * @param piece_name   piece name.
     * @param piece_id     id of the piece group; the fallback when the piece name gives no usable text.
     * @param size         multisize size in the data-units unit; empty for an individual pattern.
     * @param piece_number number of the piece in the document; makes a repeated id unique.
     */
    QString component_id(const QString &type, int type_number, const QString &piece_name, const QString &piece_id,
                         const QString &size, int piece_number);

private:
    QSet<QString> used_piece_ids;
    QSet<QString> used_component_ids;
};
} // namespace layout_handoff

#endif // HANDOFF_IDS_H
