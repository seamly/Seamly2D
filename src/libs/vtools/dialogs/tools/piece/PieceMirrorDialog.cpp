/******************************************************************************
 **  @file   PieceMirrorDialog.cpp
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

#include "PieceMirrorDialog.h"
#include "ui_PieceMirrorDialog.h"

#include <QIcon>
#include <QSoundEffect>
#include <QUrl>

#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../../../tools/PieceMirrorTool.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceMirrorDialog creates the dialog; it stays hidden until the user picks a valid piece.
 * @param data   container with the pattern data.
 * @param toolId tool id; 0 for a new tool.
 * @param parent parent widget.
 */
PieceMirrorDialog::PieceMirrorDialog(const VContainer* data, const quint32& toolId, QWidget* parent)
    : DialogTool(data, toolId, parent)
    , ui(new Ui::PieceMirrorDialog)
    , m_piece_id(NULL_ID)
    , m_active_pieces()
    , m_beep(new QSoundEffect(this))
{
    m_beep->setSource(QUrl(qApp->Settings()->getSelectionSound()));

    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(QIcon(":/toolicon/32x32/piece_mirror.png"));

    // Open the dialog where the user preference puts tool dialogs.
    setDialogPosition();

    initializeOkCancel(ui);
}

//---------------------------------------------------------------------------------------------------------------------
PieceMirrorDialog::~PieceMirrorDialog()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetPiecesList keeps the pieces of the active draft block; the mirror operation goes into that block.
 */
void PieceMirrorDialog::SetPiecesList(const QVector<quint32>& list)
{
    m_active_pieces = list;
}

//---------------------------------------------------------------------------------------------------------------------
quint32 PieceMirrorDialog::getPieceId() const
{
    return m_piece_id;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief removeHalfFromLayout tells whether the half piece leaves the layout once the full piece exists.
 */
bool PieceMirrorDialog::removeHalfFromLayout() const
{
    return ui->removeHalf_CheckBox->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChosenObject takes the first valid piece the user clicks, then shows the dialog.
 * @param id   id of the clicked object.
 * @param type type of the clicked object; only a piece counts.
 */
void PieceMirrorDialog::ChosenObject(quint32 id, const SceneObject& type)
{
    if (m_piece_id == NULL_ID && type == SceneObject::Piece)
    {
        m_beep->play();

        // A piece of another draft block, or one without a valid fold line, leaves the dialog hidden.
        QString problem;
        if (!m_active_pieces.contains(id))
        {
            problem = tr("Select a piece of the active draft block");
        }
        else
        {
            problem = PieceMirrorTool::mirrorProblem(data, id);
        }

        if (problem.isEmpty())
        {
            m_piece_id = id;
            ui->piece_Label->setText(tr("Piece: %1").arg(data->GetPiece(id).GetName()));
            emit ToolTip(QString());
            show();
        }
        else
        {
            emit ToolTip(problem);
        }
    }
}
