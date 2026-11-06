//---------------------------------------------------------------------------------------------------------------------
// @file CopyPatternPiece.h
// @author slspencer
// @brief Applies an independent piece reconstruction in one undo step.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#ifndef COPY_PATTERN_PIECE_H
#define COPY_PATTERN_PIECE_H

#include "vundocommand.h"
#include "../../ifc/xml/PatternPieceCopy.h"

/// @brief Adds and removes a copied piece together with its private modeling dependencies.
class CopyPatternPiece : public VUndoCommand
{
public:
    CopyPatternPiece(const PatternPieceCopyResult &copy, VAbstractPattern *document);
    void undo() override;
    void redo() override;

private:
    PatternPieceCopyResult m_copy;
};

#endif // COPY_PATTERN_PIECE_H
