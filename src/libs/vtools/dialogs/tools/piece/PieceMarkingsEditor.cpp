//---------------------------------------------------------------------------------------------------------------------
// @file PieceMarkingsEditor.cpp
// @author slspencer
// @brief Edits explicit drill holes and reference-only lines through live anchor points.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "PieceMarkingsEditor.h"
#include "../../../../vpatterndb/vcontainer.h"
#include "../../../../vgeometry/vpointf.h"
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtMath>

PieceMarkingsEditor::PieceMarkingsEditor(bool drill_holes, QWidget *parent)
    : QWidget(parent)
    , m_drill(drill_holes)
    , m_start(new QComboBox(this))
    , m_end(new QComboBox(this))
    , m_type(new QComboBox(this))
    , m_diameter(new QLineEdit(this))
    , m_list(new QListWidget(this))
{
    auto *layout = new QVBoxLayout(this);
    auto *note = new QLabel(drill_holes
        ? tr("Select a live anchor and enter a positive diameter in pattern units. Diameter stays constant across sizes.")
        : tr("Select two different live anchors. Stripe and plaid lines are references, not packing constraints."), this);
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *form = new QFormLayout();
    form->addRow(drill_holes ? tr("Center anchor:") : tr("Start anchor:"), m_start);
    if (drill_holes)
    {
        form->addRow(tr("Diameter:"), m_diameter);
        m_end->hide();
        m_type->hide();
    }
    else
    {
        m_type->addItem(tr("Stripe"), QStringLiteral("stripe"));
        m_type->addItem(tr("Plaid"), QStringLiteral("plaid"));
        form->addRow(tr("End anchor:"), m_end);
        form->addRow(tr("Reference type:"), m_type);
        m_diameter->hide();
    }
    layout->addLayout(form);
    auto *buttons = new QHBoxLayout();
    auto *add = new QPushButton(tr("Add"), this);
    add->setObjectName(QStringLiteral("addMarking"));
    m_start->setObjectName(QStringLiteral("startAnchor"));
    m_end->setObjectName(QStringLiteral("endAnchor"));
    m_diameter->setObjectName(QStringLiteral("drillDiameter"));
    auto *remove = new QPushButton(tr("Remove selected"), this);
    buttons->addWidget(add);
    buttons->addWidget(remove);
    layout->addLayout(buttons);
    layout->addWidget(m_list);
    auto validate = [this, add]()
    {
        bool number = false;
        const qreal diameter = locale().toDouble(m_diameter->text(), &number);
        add->setEnabled(m_start->currentData().toUInt() != 0
            && (m_drill ? number && qIsFinite(diameter) && diameter > 0
                        : m_end->currentData().toUInt() != 0 && m_start->currentData() != m_end->currentData()
                            && m_points.value(m_start->currentData().toUInt())
                                != m_points.value(m_end->currentData().toUInt())));
    };
    connect(m_start, &QComboBox::currentIndexChanged, this, validate);
    connect(m_end, &QComboBox::currentIndexChanged, this, validate);
    connect(m_diameter, &QLineEdit::textChanged, this, validate);
    connect(add, &QPushButton::clicked, this, [this]()
    {
        if (m_drill)
        {
            m_holes.append({m_start->currentData().toUInt(), locale().toDouble(m_diameter->text())});
            m_diameter->clear();
        }
        else
        {
            m_lines.append({m_start->currentData().toUInt(), m_end->currentData().toUInt(),
                            m_type->currentData().toString()});
        }
        refresh();
        emit changed();
    });
    connect(remove, &QPushButton::clicked, this, [this]()
    {
        const int row = m_list->currentRow();
        if (row < 0)
        {
            return;
        }
        if (m_drill) { m_holes.removeAt(row); }
        else { m_lines.removeAt(row); }
        refresh();
        emit changed();
    });
    validate();
}

void PieceMarkingsEditor::setPoints(const QVector<quint32> &anchors, const VContainer *data)
{
    const quint32 start = m_start->currentData().toUInt();
    const quint32 end = m_end->currentData().toUInt();
    m_points.clear();
    m_start->clear();
    m_end->clear();
    m_start->addItem(tr("Select an anchor"), 0U);
    m_end->addItem(tr("Select an anchor"), 0U);
    for (quint32 id : anchors)
    {
        m_points.insert(id, static_cast<QPointF>(*data->GeometricObject<VPointF>(id)));
        const QString name = data->GetGObject(id)->name();
        m_start->addItem(name, id);
        m_end->addItem(name, id);
    }
    m_start->setCurrentIndex(qMax(0, m_start->findData(start)));
    m_end->setCurrentIndex(qMax(0, m_end->findData(end)));
    refresh();
}

void PieceMarkingsEditor::setMarkings(const QVector<PieceDrillHole> &holes,
                                     const QVector<PieceReferenceLine> &lines)
{
    m_holes = holes;
    m_lines = lines;
    refresh();
}

QVector<PieceDrillHole> PieceMarkingsEditor::drillHoles() const { return m_holes; }
QVector<PieceReferenceLine> PieceMarkingsEditor::referenceLines() const { return m_lines; }

bool PieceMarkingsEditor::markingsValid() const
{
    if (!PieceMarkings::valid(m_holes, m_lines, m_points.keys().toVector()))
    {
        return false;
    }
    for (const PieceReferenceLine &line : m_lines)
    {
        if (m_points.value(line.start) == m_points.value(line.end))
        {
            return false;
        }
    }
    return true;
}

void PieceMarkingsEditor::refresh()
{
    m_list->clear();
    const auto name = [this](quint32 id)
    {
        const int index = m_start->findData(id);
        return index > 0 ? m_start->itemText(index) : tr("Missing anchor %1").arg(id);
    };
    if (m_drill)
    {
        for (const PieceDrillHole &hole : m_holes)
        {
            m_list->addItem(tr("%1 — diameter %2").arg(name(hole.center), locale().toString(hole.diameter)));
        }
    }
    else
    {
        for (const PieceReferenceLine &line : m_lines)
        {
            m_list->addItem(tr("%1: %2 → %3").arg(line.type, name(line.start), name(line.end)));
        }
    }
}
