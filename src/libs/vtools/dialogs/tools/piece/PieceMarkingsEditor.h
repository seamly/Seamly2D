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

/// @brief Edits anchor-based drill holes, path-node references, and saved legacy anchor references.
class PieceMarkingsEditor : public QWidget
{
    Q_OBJECT
public:
    PieceMarkingsEditor(bool drill_holes, QWidget *parent = nullptr);
    /// @brief Refreshes selectable points and legacy anchor references.
    /// @param legacy_anchor_ids Existing anchors recognized for saved lines but excluded from new selections.
    void setPoints(const QVector<quint32> &point_ids, const VContainer *data,
                   const QVector<quint32> &legacy_anchor_ids = {});
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
    QHash<quint32, QString> m_point_names;
    QVector<quint32> m_anchor_ids;
    QVector<quint32> m_path_node_ids;
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
