//---------------------------------------------------------------------------------------------------------------------
// @file handoff_svg_file.cpp
// @author slspencer
// @brief Default file name and UTF-8 file write of the pieces SVG that Export Pieces saves.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "handoff_svg_file.h"

#include <QByteArray>
#include <QSaveFile>

namespace layout_handoff
{
//---------------------------------------------------------------------------------------------------------------------
QString pieces_svg_file_name(const QString& pattern_base_name, const QDateTime& date_time)
{
    return pattern_base_name + QStringLiteral("_pieces_") + date_time.toString(QStringLiteral("yyMMddHHmm"))
           + QStringLiteral(".svg");
}

//---------------------------------------------------------------------------------------------------------------------
bool write_svg_file(const QString& file_path, const QString& svg)
{
    // Without commit() the QSaveFile destructor discards the temporary file and keeps the old file.
    QSaveFile file(file_path);
    bool written = file.open(QIODevice::WriteOnly);
    if (written)
    {
        const QByteArray bytes = svg.toUtf8();
        written = file.write(bytes) == bytes.size() && file.commit();
    }
    return written;
}
} // namespace layout_handoff
