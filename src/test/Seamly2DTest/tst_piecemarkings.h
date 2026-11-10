//---------------------------------------------------------------------------------------------------------------------
// @file tst_piecemarkings.h
// @author slspencer
// @brief Tests explicit drill diameters, graded references, and optional marking serialization.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------
#ifndef TST_PIECE_MARKINGS_H
#define TST_PIECE_MARKINGS_H
#include <QObject>
class TST_PieceMarkings : public QObject
{
    Q_OBJECT
private slots:
    void anchorAdditionUndoRedo();
    void roundTripAndDetach();
    void rejectsUnsafeValues();
    void currentSchemaAndCopy();
    void editorRequiresExplicitInput();
    void pieceDialogHasSeparateMarkingPages();
};
#endif // TST_PIECE_MARKINGS_H
