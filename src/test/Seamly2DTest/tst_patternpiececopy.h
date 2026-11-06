//---------------------------------------------------------------------------------------------------------------------
// @file tst_patternpiececopy.h
// @author slspencer
// @brief Tests independent piece reconstruction and live draft references.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#ifndef TST_PATTERN_PIECE_COPY_H
#define TST_PATTERN_PIECE_COPY_H

#include <QObject>

class TST_PatternPieceCopy : public QObject
{
    Q_OBJECT
private slots:
    void bundledAndUnionPieces_data();
    void bundledAndUnionPieces();
    void independentPathsAnchorsAndNestedUnion();
    void refusesMissingDependencies();
    void undoRedoRestoresPrivateDependencies();
};

#endif // TST_PATTERN_PIECE_COPY_H
