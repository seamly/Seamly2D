//---------------------------------------------------------------------------------------------------------------------
// @file PieceMarkingsEditor.h
// @author slspencer
// @brief Edits drill holes on anchors and reference lines between main-path nodes.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#ifndef PIECE_MARKINGS_EDITOR_H
#define PIECE_MARKINGS_EDITOR_H

#include <QWidget>
#include <QHash>
#include <QPointF>
#include <QStringList>
#include "../../../../vpatterndb/PieceMarkings.h"

class QComboBox;
class QButtonGroup;
class QLineEdit;
class QListWidget;
class QLabel;
class VContainer;

/// @brief Requires an explicit positive drill diameter or two distinct reference nodes.
class PieceMarkingsEditor : public QWidget
{
    Q_OBJECT
public:
    PieceMarkingsEditor(bool drill_holes, QWidget *parent = nullptr);
    void setPoints(const QVector<quint32> &point_ids, const VContainer *data);
    void setMarkings(const QVector<PieceDrillHole> &holes, const QVector<PieceReferenceLine> &lines);
    QVector<PieceDrillHole> drillHoles() const;
    QVector<PieceReferenceLine> referenceLines() const;
    QStringList itemsUsingPoint(quint32 point_id) const;
    bool markingsValid() const;

signals:
    void changed();

private:
    bool m_drill;
    QVector<PieceDrillHole> m_holes;
    QVector<PieceReferenceLine> m_lines;
    QHash<quint32, QPointF> m_points;
    QComboBox *m_start;
    QComboBox *m_end;
    QButtonGroup *m_type_buttons;
    QLineEdit *m_diameter;
    QListWidget *m_list;
    QLabel *m_error;
    void refresh();
    void updateValidation();
};

#endif // PIECE_MARKINGS_EDITOR_H
