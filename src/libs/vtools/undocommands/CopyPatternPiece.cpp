//---------------------------------------------------------------------------------------------------------------------
// @file CopyPatternPiece.cpp
// @author slspencer
// @brief Applies an independent piece reconstruction in one undo step.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "CopyPatternPiece.h"

CopyPatternPiece::CopyPatternPiece(const PatternPieceCopyResult &copy, VAbstractPattern *document)
    : VUndoCommand(copy.piece, document)
    , m_copy(copy)
{
    setText(tr("copy piece"));
}

void CopyPatternPiece::redo()
{
    for (const CopiedModelingElement &copy : m_copy.modeling)
    {
        doc->getDraftBlockElement(copy.block_name).firstChildElement(VAbstractPattern::TagModeling)
            .appendChild(copy.element);
    }
    doc->getDraftBlockElement(m_copy.block_name).firstChildElement(VAbstractPattern::TagPieces)
        .appendChild(m_copy.piece);
    emit NeedFullParsing();
}

void CopyPatternPiece::undo()
{
    m_copy.piece.parentNode().removeChild(m_copy.piece);
    for (auto copy = m_copy.modeling.crbegin(); copy != m_copy.modeling.crend(); ++copy)
    {
        copy->element.parentNode().removeChild(copy->element);
    }
    emit NeedFullParsing();
}
