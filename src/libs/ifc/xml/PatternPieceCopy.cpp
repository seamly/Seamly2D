//---------------------------------------------------------------------------------------------------------------------
// @file PatternPieceCopy.cpp
// @author slspencer
// @brief Reconstructs independent modeling dependencies for a copied piece.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// https://www.gnu.org/licenses/gpl-3.0.html
//---------------------------------------------------------------------------------------------------------------------

#include "PatternPieceCopy.h"

#include <QCoreApplication>
#include <QSet>

namespace
{
const QSet<QString> reference_attributes{
    QStringLiteral("idObject"), QStringLiteral("idTool"), QStringLiteral("path"),
    QStringLiteral("start"), QStringLiteral("end"), QStringLiteral("centerAnchor"),
    QStringLiteral("topLeftAnchor"), QStringLiteral("bottomRightAnchor"),
    QStringLiteral("topAnchor"), QStringLiteral("bottomAnchor"), QStringLiteral("center")};

QVector<QDomElement> descendants(const QDomElement &root)
{
    QVector<QDomElement> elements{root};
    for (int i = 0; i < elements.size(); ++i)
    {
        for (QDomElement child = elements.at(i).firstChildElement(); !child.isNull();
             child = child.nextSiblingElement())
        {
            elements.append(child);
        }
    }
    return elements;
}

bool textReference(const QDomElement &element)
{
    return element.tagName() == QStringLiteral("child")
        || (element.tagName() == QStringLiteral("record")
            && element.parentNode().toElement().tagName() == QStringLiteral("anchors"));
}

QVector<quint32> references(const QDomElement &root)
{
    QVector<quint32> ids;
    for (const QDomElement &element : descendants(root))
    {
        for (const QString &attribute : reference_attributes)
        {
            const quint32 id = element.attribute(attribute).toUInt();
            if (id != 0)
            {
                ids.append(id);
            }
        }
        if (textReference(element))
        {
            ids.append(element.text().toUInt());
        }
    }
    return ids;
}

void remap(QDomElement root, const QHash<quint32, quint32> &ids)
{
    for (QDomElement element : descendants(root))
    {
        QSet<QString> attributes = reference_attributes;
        attributes.insert(QStringLiteral("id"));
        for (const QString &attribute : attributes)
        {
            const quint32 id = element.attribute(attribute).toUInt();
            if (ids.contains(id))
            {
                element.setAttribute(attribute, ids.value(id));
            }
        }
        if (textReference(element) && ids.contains(element.text().toUInt()))
        {
            element.firstChild().setNodeValue(QString::number(ids.value(element.text().toUInt())));
        }
    }
}

QString suffixed(const QString &source, const QSet<QString> &used, int first_suffix)
{
    int suffix = first_suffix;
    QString name;
    do
    {
        name = QStringLiteral("%1-%2").arg(source).arg(suffix++);
    } while (used.contains(name));
    return name;
}
}

PatternPieceCopyResult PatternPieceCopy::reconstruct(const QDomDocument &document, quint32 piece_id,
                                                      const std::function<quint32()> &next_id)
{
    PatternPieceCopyResult result;
    QDomElement source;
    QVector<CopiedModelingElement> modeling;
    QHash<quint32, int> owners;
    QSet<quint32> calculation_ids;
    QSet<quint32> used_ids;
    QSet<quint32> piece_ids;
    QSet<QString> names;
    QSet<QString> letters;
    for (const QDomElement &element : descendants(document.documentElement()))
    {
        const quint32 id = element.attribute(QStringLiteral("id")).toUInt();
        if (id != 0)
        {
            used_ids.insert(id);
        }
    }
    const QDomNodeList blocks = document.elementsByTagName(QStringLiteral("draftBlock"));
    for (int i = 0; i < blocks.size(); ++i)
    {
        const QDomElement block = blocks.at(i).toElement();
        for (const QDomElement &element : descendants(block.firstChildElement(QStringLiteral("calculation"))))
        {
            const quint32 id = element.attribute(QStringLiteral("id")).toUInt();
            if (id != 0)
            {
                calculation_ids.insert(id);
                used_ids.insert(id);
            }
            // Draft operations declare their live output IDs on destination items.
            if (element.parentNode().toElement().tagName() == QStringLiteral("destination"))
            {
                const quint32 output_id = element.attribute(QStringLiteral("idObject")).toUInt();
                calculation_ids.insert(output_id);
                used_ids.insert(output_id);
            }
        }
        for (QDomElement element = block.firstChildElement(QStringLiteral("modeling")).firstChildElement();
             !element.isNull(); element = element.nextSiblingElement())
        {
            const int owner = modeling.size();
            modeling.append({block.attribute(QStringLiteral("name")), element});
            owners.insert(element.attribute(QStringLiteral("id")).toUInt(), owner);
            used_ids.insert(element.attribute(QStringLiteral("id")).toUInt());
            // Union creates flattened child objects with no separate XML declaration.
            const QDomNodeList children = element.firstChildElement(QStringLiteral("children"))
                                                 .elementsByTagName(QStringLiteral("child"));
            for (int child = 0; child < children.size(); ++child)
            {
                const quint32 id = children.at(child).toElement().text().toUInt();
                if (!owners.contains(id))
                {
                    owners.insert(id, owner);
                }
                used_ids.insert(id);
            }
        }
        for (QDomElement piece = block.firstChildElement(QStringLiteral("pieces")).firstChildElement();
             !piece.isNull(); piece = piece.nextSiblingElement())
        {
            used_ids.insert(piece.attribute(QStringLiteral("id")).toUInt());
            piece_ids.insert(piece.attribute(QStringLiteral("id")).toUInt());
            names.insert(piece.attribute(QStringLiteral("name")));
            letters.insert(piece.firstChildElement(QStringLiteral("data")).attribute(QStringLiteral("letter")));
            if (piece.attribute(QStringLiteral("id")).toUInt() == piece_id)
            {
                source = piece;
                result.block_name = block.attribute(QStringLiteral("name"));
            }
        }
    }
    if (source.isNull() || !next_id)
    {
        result.error = QCoreApplication::translate("PatternPieceCopy", "Cannot find the piece to copy.");
        return result;
    }

    QSet<int> selected;
    QVector<quint32> pending = references(source);
    for (int i = 0; i < pending.size(); ++i)
    {
        const quint32 id = pending.at(i);
        if (id == 0 || piece_ids.contains(id) || calculation_ids.contains(id))
        {
            continue;
        }
        if (!owners.contains(id))
        {
            result.error = QCoreApplication::translate("PatternPieceCopy", "Cannot reconstruct object %1.").arg(id);
            return result;
        }
        const int owner = owners.value(id);
        if (!selected.contains(owner))
        {
            selected.insert(owner);
            pending += references(modeling.at(owner).element);
        }
    }

    QVector<quint32> declarations{piece_id};
    for (int i = 0; i < modeling.size(); ++i)
    {
        if (!selected.contains(i))
        {
            continue;
        }
        const QDomElement element = modeling.at(i).element;
        declarations.append(element.attribute(QStringLiteral("id")).toUInt());
        const QDomNodeList children = element.firstChildElement(QStringLiteral("children"))
                                             .elementsByTagName(QStringLiteral("child"));
        for (int child = 0; child < children.size(); ++child)
        {
            declarations.append(children.at(child).toElement().text().toUInt());
        }
    }
    for (quint32 id : declarations)
    {
        if (id == 0 || result.ids.contains(id))
        {
            continue;
        }
        const quint32 new_id = next_id();
        if (new_id == 0 || used_ids.contains(new_id))
        {
            result.error = QCoreApplication::translate("PatternPieceCopy", "The copy ID is already in use.");
            return result;
        }
        result.ids.insert(id, new_id);
        used_ids.insert(new_id);
    }
    for (int i = 0; i < modeling.size(); ++i)
    {
        if (selected.contains(i))
        {
            QDomElement clone = modeling.at(i).element.cloneNode(true).toElement();
            const quint32 id_tool = clone.attribute(QStringLiteral("idTool")).toUInt();
            remap(clone, result.ids);
            if (piece_ids.contains(id_tool))
            {
                // Piece ownership is not a geometric dependency of the copied piece.
                clone.setAttribute(QStringLiteral("idTool"), id_tool);
            }
            result.modeling.append({modeling.at(i).block_name, clone});
        }
    }
    result.piece = source.cloneNode(true).toElement();
    remap(result.piece, result.ids);
    result.piece.setAttribute(QStringLiteral("name"), suffixed(source.attribute(QStringLiteral("name")), names, 1));
    QDomElement label = result.piece.firstChildElement(QStringLiteral("data"));
    const QString letter = label.attribute(QStringLiteral("letter"));
    if (!letter.isEmpty())
    {
        label.setAttribute(QStringLiteral("letter"), suffixed(letter, letters, 2));
    }
    result.piece.setAttribute(QStringLiteral("locked"), QStringLiteral("false"));
    result.piece.setAttribute(QStringLiteral("united"), QStringLiteral("false"));
    return result;
}
