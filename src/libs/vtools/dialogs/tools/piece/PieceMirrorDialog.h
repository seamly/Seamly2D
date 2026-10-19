/******************************************************************************
 **  @file   PieceMirrorDialog.h
 **  @author slspencer
 **
 **  @brief
 **  Dialog of the Piece Mirror tool: the user picks a half piece with a fold
 **  line, then confirms the mirror.
 **
 **  @copyright
 **  This source code is part of the Seamly2D project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
 **
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **
 **  Seamly2D is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Seamly2D is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
 **
 *****************************************************************************/

#ifndef PIECEMIRRORDIALOG_H
#define PIECEMIRRORDIALOG_H

#include <QObject>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "../dialogtool.h"

class QSoundEffect;

namespace Ui
{
    class PieceMirrorDialog;
}

/**
 * @brief PieceMirrorDialog takes the half piece for the Piece Mirror tool.
 *
 * The user clicks a piece on the piece scene. A piece with a valid fold line opens the dialog; any other piece leaves
 * the dialog hidden and shows the problem in the status bar. The piece can belong to any draft block.
 */
class PieceMirrorDialog : public DialogTool
{
    Q_OBJECT
public:
                     PieceMirrorDialog(const VContainer* data, const quint32& toolId, QWidget* parent = nullptr);
    virtual         ~PieceMirrorDialog() override;

    quint32          getPieceId() const;
    bool             removeHalfFromLayout() const;

public slots:
    virtual void     ChosenObject(quint32 id, const SceneObject& type) override;

private:
    Q_DISABLE_COPY(PieceMirrorDialog)

    Ui::PieceMirrorDialog* ui;         ///< Dialog widgets.
    quint32                m_piece_id; ///< Chosen half piece; NULL_ID until the user picks a valid piece.
    QSoundEffect*          m_beep;     ///< Selection sound.
};

#endif // PIECEMIRRORDIALOG_H
