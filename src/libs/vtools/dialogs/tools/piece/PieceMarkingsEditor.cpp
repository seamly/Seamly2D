//---------------------------------------------------------------------------------------------------------------------
// @file PieceMarkingsEditor.cpp
// @author slspencer
// @brief Edits drill holes on anchors and reference lines between main-path nodes.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "PieceMarkingsEditor.h"
#include "../../../../vpatterndb/vcontainer.h"
#include "../../../../vgeometry/vpointf.h"
#include <QButtonGroup>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStringList>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

namespace
{
QIcon referenceIcon(bool plaid)
{
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::blue, 2.0, Qt::SolidLine, Qt::RoundCap));
    if (plaid)
    {
        painter.drawLine(4, 12, 20, 12);
        painter.drawLine(12, 4, 12, 20);
    }
    else
    {
        painter.drawLine(4, 9, 20, 9);
        painter.drawLine(4, 15, 20, 15);
    }
    return QIcon(pixmap);
}
}

PieceMarkingsEditor::PieceMarkingsEditor(bool drill_holes, QWidget *parent)
    : QWidget(parent)
    , m_drill(drill_holes)
    , m_start(new QComboBox(this))
    , m_end(new QComboBox(this))
    , m_type_buttons(new QButtonGroup(this))
    , m_diameter(new QLineEdit(this))
    , m_list(new QListWidget(this))
    , m_error(new QLabel(this))
{
    auto *layout = new QVBoxLayout(this);
    auto *note = new QLabel(drill_holes
        ? tr("Select a live anchor and enter a positive diameter in pattern units. Diameter stays constant across sizes.")
        : tr("Select two different main-path nodes. Stripe and plaid lines are references, not packing constraints."), this);
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *form = new QFormLayout();
    form->addRow(drill_holes ? tr("Center anchor:") : tr("Start path node:"), m_start);
    if (drill_holes)
    {
        form->addRow(tr("Diameter:"), m_diameter);
        m_end->hide();
    }
    else
    {
        auto *reference_types = new QWidget(this);
        auto *reference_layout = new QHBoxLayout(reference_types);
        reference_layout->setContentsMargins(0, 0, 0, 0);
        m_type_buttons->setExclusive(true);
        for (const auto &type : {qMakePair(tr("Stripe"), QStringLiteral("stripe")),
                                 qMakePair(tr("Plaid"), QStringLiteral("plaid"))})
        {
            auto *button = new QToolButton(reference_types);
            button->setText(type.first);
            button->setIcon(referenceIcon(type.second == QStringLiteral("plaid")));
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            button->setCheckable(true);
            button->setProperty("referenceType", type.second);
            button->setObjectName(type.second + QStringLiteral("ReferenceIcon"));
            button->setAccessibleName(type.first);
            button->setChecked(type.second == QStringLiteral("stripe"));
            m_type_buttons->addButton(button);
            reference_layout->addWidget(button);
        }
        form->addRow(tr("End path node:"), m_end);
        form->addRow(tr("Reference type:"), reference_types);
        m_diameter->hide();
    }
    layout->addLayout(form);
    auto *buttons = new QHBoxLayout();
    auto *add = new QPushButton(tr("Add"), this);
    add->setObjectName(QStringLiteral("addMarking"));
    m_start->setObjectName(QStringLiteral("startPoint"));
    m_end->setObjectName(QStringLiteral("endPoint"));
    m_diameter->setObjectName(QStringLiteral("drillDiameter"));
    auto *remove = new QPushButton(tr("Remove selected"), this);
    buttons->addWidget(add);
    buttons->addWidget(remove);
    layout->addLayout(buttons);
    layout->addWidget(m_list);
    m_error->setObjectName(QStringLiteral("markingError"));
    m_error->setWordWrap(true);
    layout->addWidget(m_error);
    connect(m_start, &QComboBox::currentIndexChanged, this, &PieceMarkingsEditor::updateValidation);
    connect(m_end, &QComboBox::currentIndexChanged, this, &PieceMarkingsEditor::updateValidation);
    connect(m_diameter, &QLineEdit::textChanged, this, &PieceMarkingsEditor::updateValidation);
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
                            m_type_buttons->checkedButton()->property("referenceType").toString()});
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
    updateValidation();
}

void PieceMarkingsEditor::setPoints(const QVector<quint32> &point_ids, const VContainer *data)
{
    const quint32 start = m_start->currentData().toUInt();
    const quint32 end = m_end->currentData().toUInt();
    m_points.clear();
    m_start->clear();
    m_end->clear();
    const QString placeholder = m_drill ? tr("Select an anchor") : tr("Select a path node");
    m_start->addItem(placeholder, 0U);
    m_end->addItem(placeholder, 0U);
    for (quint32 id : point_ids)
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

QStringList PieceMarkingsEditor::itemsUsingPoint(quint32 point_id) const
{
    const auto name = [this](quint32 id)
    {
        const int index = m_start->findData(id);
        return index > 0 ? m_start->itemText(index) : tr("Missing point %1").arg(id);
    };
    QStringList items;
    if (m_drill)
    {
        for (const PieceDrillHole &hole : m_holes)
        {
            if (hole.center == point_id)
            {
                items.append(tr("Drill hole at %1 (diameter %2)")
                                 .arg(name(hole.center), locale().toString(hole.diameter)));
            }
        }
    }
    else
    {
        for (const PieceReferenceLine &line : m_lines)
        {
            if (line.start == point_id || line.end == point_id)
            {
                items.append(tr("%1 reference: %2 → %3")
                                 .arg(line.type, name(line.start), name(line.end)));
            }
        }
    }
    return items;
}

bool PieceMarkingsEditor::markingsValid() const
{
    const QVector<quint32> available_points = m_points.keys().toVector();
    if (!PieceMarkings::valid(m_holes, m_lines,
                              m_drill ? available_points : QVector<quint32>(),
                              m_drill ? QVector<quint32>() : available_points))
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
        if (index > 0)
        {
            return m_start->itemText(index);
        }
        return m_drill ? tr("Missing anchor %1").arg(id) : tr("Missing path node %1").arg(id);
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
    updateValidation();
}

void PieceMarkingsEditor::updateValidation()
{
    bool number = false;
    const qreal diameter = locale().toDouble(m_diameter->text(), &number);
    const quint32 start = m_start->currentData().toUInt();
    const quint32 end = m_end->currentData().toUInt();
    const bool positive_diameter = number && qIsFinite(diameter) && diameter > 0;
    const bool distinct_endpoints = end != 0 && start != end && m_points.value(start) != m_points.value(end);
    findChild<QPushButton *>(QStringLiteral("addMarking"))->setEnabled(
        start != 0 && (m_drill ? positive_diameter : distinct_endpoints));
    QString error;
    if (!markingsValid())
    {
        error = tr("A saved marking has a missing anchor or path node, or coincident endpoints. "
                   "Remove or replace that marking.");
    }
    else if (m_drill && !m_diameter->text().isEmpty() && !positive_diameter)
    {
        error = tr("Enter a finite diameter greater than zero in pattern units.");
    }
    else if (!m_drill && start != 0 && end != 0 && !distinct_endpoints)
    {
        error = tr("Reference endpoints must have different positions.");
    }
    m_error->setText(error);
    m_error->setVisible(!error.isEmpty());
}
