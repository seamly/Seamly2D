//---------------------------------------------------------------------------------------------------------------------
// @file handoff_svg_file.h
// @author slspencer
// @brief Default file name and UTF-8 file write of the pieces SVG that Export Pieces saves.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#ifndef HANDOFF_SVG_FILE_H
#define HANDOFF_SVG_FILE_H

#include <QDateTime>
#include <QString>

namespace layout_handoff
{
/**
 * @brief pieces_svg_file_name returns the default file name of an exported pieces SVG.
 * @param pattern_base_name pattern file name without folder and suffix; "unnamed" for a new pattern.
 * @param date_time         export time.
 * @return "<pattern_base_name>_pieces_<yyMMddHHmm>.svg".
 */
QString pieces_svg_file_name(const QString& pattern_base_name, const QDateTime& date_time);

/**
 * @brief write_svg_file writes an SVG document to a file as UTF-8.
 * @details QSaveFile replaces an existing file only after all bytes are written.
 * @param file_path path of the file to create or replace. The folder must exist.
 * @param svg       SVG document.
 * @return true when the file holds the document; false on an open, write or commit error.
 */
bool write_svg_file(const QString& file_path, const QString& svg);
} // namespace layout_handoff

#endif // HANDOFF_SVG_FILE_H
