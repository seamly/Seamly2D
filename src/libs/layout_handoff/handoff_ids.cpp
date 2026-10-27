//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_ids.cpp
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

#include "handoff_ids.h"

#include <QRegularExpression>

namespace layout_handoff
{
//---------------------------------------------------------------------------------------------------------------------
QString sanitize_for_id(const QString &name)
{
    QString sanitized = name;
    sanitized.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]+")), QStringLiteral("_"));
    sanitized.remove(QRegularExpression(QStringLiteral("^_+|_+$")));
    if (!sanitized.isEmpty() && sanitized.at(0).isDigit())
    {
        sanitized.prepend(QLatin1Char('_'));
    }
    return sanitized;
}

//---------------------------------------------------------------------------------------------------------------------
QString size_id_suffix(const QString &size)
{
    QString sanitized = sanitize_for_id(size);
    if (sanitized.startsWith(QLatin1Char('_')))
    {
        // The digit guard is not needed after the "_s" prefix.
        sanitized.remove(0, 1);
    }
    return QStringLiteral("_s") + sanitized;
}

//---------------------------------------------------------------------------------------------------------------------
bool component_id_is_numbered(const QString &type)
{
    return type == QLatin1String("internal_path") || type == QLatin1String("cut_path")
        || type == QLatin1String("piece_label") || type == QLatin1String("pattern_label");
}

//---------------------------------------------------------------------------------------------------------------------
QString HandoffIds::piece_id(const QString &fallback_id, const QString &piece_name, const QString &size,
                             int piece_number)
{
    const QString sanitized_name = sanitize_for_id(piece_name);
    QString id = sanitized_name.isEmpty() ? fallback_id : QStringLiteral("piece_%1").arg(sanitized_name);
    if (!size.isEmpty())
    {
        id += size_id_suffix(size);
    }
    // Piece names are not unique, so a repeated id takes the piece number.
    if (used_piece_ids.contains(id))
    {
        id += QLatin1Char('-') + QString::number(piece_number);
    }
    used_piece_ids.insert(id);
    return id;
}

//---------------------------------------------------------------------------------------------------------------------
QString HandoffIds::piece_set_id(const QString &piece_name, int set_number)
{
    const QString sanitized_name = sanitize_for_id(piece_name);
    QString id = sanitized_name.isEmpty() ? QStringLiteral("piece-set-%1").arg(set_number)
                                          : QStringLiteral("piece-set_%1").arg(sanitized_name);
    if (used_piece_ids.contains(id))
    {
        id += QLatin1Char('-') + QString::number(set_number);
    }
    used_piece_ids.insert(id);
    return id;
}

//---------------------------------------------------------------------------------------------------------------------
QString HandoffIds::component_id(const QString &type, int type_number, const QString &piece_name,
                                 const QString &piece_id, const QString &size, int piece_number)
{
    const QString sanitized_name = sanitize_for_id(piece_name);
    QString id;
    if (sanitized_name.isEmpty())
    {
        id = QStringLiteral("%1-%2-%3").arg(piece_id, type).arg(type_number);
    }
    else if (component_id_is_numbered(type))
    {
        id = QStringLiteral("%1_%2_%3").arg(type).arg(type_number).arg(sanitized_name);
    }
    else
    {
        id = QStringLiteral("%1_%2").arg(type, sanitized_name);
    }
    // The same piece repeats once per size in a multisize export.
    if (!size.isEmpty())
    {
        id += size_id_suffix(size);
    }
    if (used_component_ids.contains(id))
    {
        id += QLatin1Char('-') + QString::number(piece_number);
    }
    used_component_ids.insert(id);
    return id;
}
} // namespace layout_handoff
