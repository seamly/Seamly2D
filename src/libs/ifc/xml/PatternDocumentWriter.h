//---------------------------------------------------------------------------------------------------------------------
// @file PatternDocumentWriter.h
// @author slspencer
// @brief Writes current-format document and draft-block scaffolding.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#ifndef PATTERN_DOCUMENT_WRITER_H
#define PATTERN_DOCUMENT_WRITER_H

#include <QDomDocument>
#include "../../vmisc/def.h"

/// @brief Creates schema-valid pattern scaffolding without application state.
class PatternDocumentWriter
{
public:
    /// @brief Resets the document to the current format before its first draft block exists.
    static void createEmptyPattern(QDomDocument &document, Unit pattern_unit, const QString &comment);

    /// @brief Ensures each existing draft block has groups followed by images.
    static QDomElement createDraftImages(QDomDocument &document, QDomElement draft_block);
};

#endif // PATTERN_DOCUMENT_WRITER_H
