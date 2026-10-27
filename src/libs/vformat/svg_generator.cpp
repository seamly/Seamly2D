//---------------------------------------------------------------------------------------------------------------------
//  @file   svg_generator.cpp
//  @author Evans PERRET
//  @date   21 Sep, 2024
//
//  @brief
//  Custom SVG generator to handle groups in SVGs
//
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2013-2026 Seamly2D project
//  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
//
//  Seamly2D is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Seamly2D is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Seamly2D.  If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

#include "svg_generator.h"
#include "../layout_handoff/handoff_ids.h"
#include "../vlayout/vlayoutdef.h"

#include <QFile>
#include <QDebug>
#include <QFont>
#include <QSvgGenerator>
#include <QGraphicsItem>
#include <QPainter>
#include <QBuffer>
#include <QRegularExpression>

namespace
{
    //-----------------------------------------------------------------------------
    /// @brief Check whether path data draws nothing: empty or blank, or one moveto with an optional closepath.
    /// @details An empty QPainterPath still carries an implicit MoveTo(0,0). The
    ///          item's scene position moves that point, so QSvgGenerator can emit
    ///          e.g. "M -1121.38,5595.73 Z" instead of "M0,0".
    //-----------------------------------------------------------------------------
    bool isMoveToOnly(const QString &d)
    {
        int move_to_count = 0;
        for (const QChar c : d)
        {
            // 'e'/'E' can appear as a number exponent, not a command.
            if (!c.isLetter() || c == QLatin1Char('e') || c == QLatin1Char('E'))
            {
                continue;
            }
            if (c == QLatin1Char('M') || c == QLatin1Char('m'))
            {
                ++move_to_count;
            }
            else if (c != QLatin1Char('Z') && c != QLatin1Char('z'))
            {
                return false;
            }
        }
        return move_to_count <= 1;
    }
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/// @brief SvgGenerator constructor.
/// @param paper        paper rectangle defining the SVG size and view box.
/// @param name         output file path.
/// @param pattern_name pattern name, written as the data-name attribute of the pattern group.
/// @param description  pattern description (currently unused in the output).
/// @param resolution   render resolution in DPI.
//---------------------------------------------------------------------------------------------------------------------
SvgGenerator::SvgGenerator(QGraphicsRectItem *paper, QString name, QString pattern_name, QString description,
                           int resolution)
    : m_paper(paper)
    , m_filepath(name)
    , m_pattern_name(pattern_name)
    , m_description(description)
    , m_resolution(resolution)
    , m_piece_count(0)
{
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Mark the export as a multisize pattern.
/// @details The pattern group then carries data-measurements="multisize", data-sizes,
///          data-base-size and data-height. Without this call the pattern group carries
///          data-measurements="individual".
/// @param sizes     every size in the export, in pattern units, in the order the pieces are added.
/// @param base_size base size of the multisize measurements, in pattern units.
/// @param height    height of every size, in pattern units; empty writes no data-height.
//---------------------------------------------------------------------------------------------------------------------
void SvgGenerator::setMultisize(const QStringList &sizes, const QString &base_size, const QString &height)
{
    m_sizes = sizes;
    m_base_size = base_size;
    m_height = height;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Set the size of an individual pattern, written as data-sample-size on the pattern group.
/// @details Ignored for a multisize export: each piece there carries its own data-size.
/// @param sample_size size in pattern units; empty writes no attribute.
//---------------------------------------------------------------------------------------------------------------------
void SvgGenerator::setSampleSize(const QString &sample_size)
{
    m_sample_size = sample_size;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Set the pattern unit, written as data-units on the pattern group.
/// @details Downstream tools use it to pick the DXF units and to convert size values.
/// @param units pattern unit: "mm", "cm" or "inch"; empty writes no attribute.
//---------------------------------------------------------------------------------------------------------------------
void SvgGenerator::setUnits(const QString &units)
{
    m_units = units;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Merge all the SVGs in the m_dom_list list into a single SVG
/// @return The merged SVG as a DOM document
/// @details m_dom_list contains DOM representations of multiple SVGs
///          Assuming each svg contains a main group containing every graphical item of the svg,
///          this function adds to the first svg of the list all the main groups of the other svgs,
///          thus creating a single svg with each svg of the list in it, every svg being in its own group.
///          This function is used in order to create svgs containing groups
//---------------------------------------------------------------------------------------------------------------------
QDomDocument SvgGenerator::mergeSvgDoms()
{
    if (m_dom_list.isEmpty())
    {
        qDebug() << "Error : the SVG list is empty";
        return QDomDocument();
    }

    QDomDocument merged_svg = m_dom_list.at(0).cloneNode().toDocument();

    QDomElement merged_svg_root = merged_svg.documentElement();
    if (merged_svg_root.tagName() != "svg")
    {
        qDebug() << "Error : the first SVG does not contain a <svg> tag.";
        return QDomDocument();
    }

    QDomNodeList merged_svg_groups = merged_svg_root.elementsByTagName("g");
    if (merged_svg_groups.isEmpty())
    {
        qDebug() << "Error : the SVG does not contain a <g> tag.";
        return QDomDocument();
    }
    // Drop the clone's own main group; it is re-added from the list below.
    merged_svg_groups.at(0).parentNode().removeChild(merged_svg_groups.at(0));

    // Piece-based exports get one pattern group per material, wrapping the piece groups of
    // that material and tagged with the SVG data-* attributes so downstream tools can
    // identify the pattern. Materials keep the order in which their first piece was added.
    // Whole-scene exports (draft blocks) keep the legacy flat structure.
    QHash<QString, QDomElement> pattern_groups;
    if (m_piece_count > 0)
    {
        QStringList materials;
        for (int i = 0; i < m_dom_list.size(); ++i)
        {
            if (!materials.contains(m_dom_materials.value(i)))
            {
                materials.append(m_dom_materials.value(i));
            }
        }

        for (int n = 0; n < materials.size(); ++n)
        {
            QDomElement pattern_group = merged_svg.createElement("g");
            pattern_group.setAttribute("id", QStringLiteral("pattern-%1").arg(n + 1));
            pattern_group.setAttribute("data-type", "pattern");
            pattern_group.setAttribute("data-type-number", QString::number(n + 1));
            setAttribute(pattern_group, "data-name", m_pattern_name);
            setAttribute(pattern_group, "data-material", materials.at(n));
            setAttribute(pattern_group, "data-units", m_units);
            if (m_sizes.isEmpty())
            {
                pattern_group.setAttribute("data-measurements", "individual");
                setAttribute(pattern_group, "data-sample-size", m_sample_size);
            }
            else
            {
                pattern_group.setAttribute("data-measurements", "multisize");
                pattern_group.setAttribute("data-sizes", m_sizes.join(QLatin1Char(',')));
                setAttribute(pattern_group, "data-base-size", m_base_size);
                setAttribute(pattern_group, "data-height", m_height);
            }
            merged_svg_root.appendChild(pattern_group);
            pattern_groups.insert(materials.at(n), pattern_group);
        }
    }

    // Piece-set groups, created on the first piece of each set; keyed by piece-set key.
    QHash<QString, QDomElement> piece_sets;
    int piece_set_count = 0;

    for (int i = 0; i < m_dom_list.size(); ++i)
    {
        QDomDocument dom_svg = m_dom_list.at(i);
        QDomElement svg_root = dom_svg.documentElement();
        if (svg_root.tagName() != "svg")
        {
            qDebug() << "Error : the SVG does not contain a <svg> tag.";
            return QDomDocument();
        }
        QDomNodeList svg_groups = svg_root.elementsByTagName("g");
        if (svg_groups.isEmpty())
        {
            qDebug() << "Error : the SVG does not contain a <g> tag.";
            return QDomDocument();
        }
        QDomElement main_group = svg_groups.at(0).toElement();
        cleanSvg(main_group);

        // A piece goes into the pattern group of its material.
        QDomElement parent_element = merged_svg_root;
        if (pattern_groups.contains(m_dom_materials.value(i)))
        {
            parent_element = pattern_groups.value(m_dom_materials.value(i));
            if (main_group.attribute(QStringLiteral("data-type")) == QLatin1String("piece"))
            {
                main_group.setAttribute("data-parent", parent_element.attribute(QStringLiteral("id")));
            }
        }

        // A multisize piece goes into the piece-set that holds the same piece in every size.
        QDomElement target_element = parent_element;
        const QString piece_set_key = m_dom_piece_set_keys.value(i);
        if (!piece_set_key.isEmpty() && main_group.attribute(QStringLiteral("data-type")) == QLatin1String("piece"))
        {
            if (!piece_sets.contains(piece_set_key))
            {
                ++piece_set_count;
                const QString piece_name = main_group.attribute(QStringLiteral("data-name"));
                const QString piece_set_id = m_ids.piece_set_id(piece_name, piece_set_count);

                QDomElement piece_set = merged_svg.createElement("g");
                piece_set.setAttribute("id", piece_set_id);
                piece_set.setAttribute("data-type", "piece-set");
                piece_set.setAttribute("data-type-number", QString::number(piece_set_count));
                piece_set.setAttribute("data-parent", parent_element.attribute(QStringLiteral("id")));
                setAttribute(piece_set, "data-name", piece_name);
                setAttribute(piece_set, "data-letter", main_group.attribute(QStringLiteral("data-letter")));
                parent_element.appendChild(piece_set);
                piece_sets.insert(piece_set_key, piece_set);
            }
            target_element = piece_sets.value(piece_set_key);
            main_group.setAttribute("data-parent", target_element.attribute(QStringLiteral("id")));
        }

        // importNode: nodes must be cloned into the target document before appending;
        // appending a node owned by another document is silently ignored by QDom.
        target_element.appendChild(merged_svg.importNode(main_group, true));
    }

    return merged_svg;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Remove empty groups from the SVG
/// @return void
/// @details This function removes empty unuseful groups from the exported SVG.
///          Those empty groups were generated by Qt svg generator.
//---------------------------------------------------------------------------------------------------------------------
void SvgGenerator::removeEmptyGroups(QDomElement &main_group)
{
    bool svg_cleaned = false;

    //We must remove the empty groups one by one until no more empty group is found.
    //Since removing a group modifies the QDomNodeList indexation, we start over
    //the search from the beginning each time a group is removed.
    while(!svg_cleaned)
    {
        svg_cleaned = true;
        QDomNodeList groups = main_group.elementsByTagName("g");

        for (const QDomNode &node : groups)
        {
            QDomElement group = node.toElement();
            if (group.childNodes().isEmpty())
            {
                // Remove from the group's real parent: empty groups can be nested at
                // any depth, not only directly below main_group.
                if (group.parentNode().removeChild(group).isNull())
                {
                    qDebug() << "Error : could not remove empty group";
                }
                svg_cleaned = false;
                break;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Remove empty paths from the SVG
/// @return void
/// @details This function removes every path that draws nothing (see isMoveToOnly()):
///          the M0,0 origin path, a moved origin path, and a path with empty data.
///          Qt's SVG generator emits such a path for each QGraphicsPathItem with an
///          empty path (e.g. the invisible piece container and label group items).
///          If these paths are not removed, the bounding box of the exported pattern
///          piece when opened in a svg editing software can be wrong, an empty
///          component still gets a tagged group, and an import can warn
///          "Invalid path data; path truncated".
//---------------------------------------------------------------------------------------------------------------------
void SvgGenerator::removeEmptyPaths(QDomElement &main_group)
{
    QDomNodeList paths = main_group.elementsByTagName("path");
    // Iterate backwards: QDomNodeList is live, removing a node reindexes the list.
    for (int i = paths.size() - 1; i >= 0; --i)
    {
        QDomElement path = paths.at(i).toElement();
        if (isMoveToOnly(path.attribute("d")))
        {
            QDomElement parent_group = path.parentNode().toElement();
            parent_group.removeChild(path);
            // Drop the wrapping group as well when the path was its only content.
            if (parent_group != main_group && parent_group.childNodes().isEmpty())
            {
                parent_group.parentNode().removeChild(parent_group);
            }
        }
    }
}

//-----------------------------------------------------------------------------
/// @brief Clean the SVG
/// @return void
/// @details This function cleans the SVG by removing the empty paths first
///          (which may leave their wrapping groups empty) and then removing
///          all remaining empty groups.
//-----------------------------------------------------------------------------
void SvgGenerator::cleanSvg(QDomElement &main_group)
{
    removeEmptyPaths(main_group);
    removeEmptyGroups(main_group);
}

//-----------------------------------------------------------------------------
/// @brief setAttribute set an attribute in the svg
///
/// This method sets an attibute for the 1st elemnent by tag in the svg document.
///
/// @param element : the dom document element.
/// @param attr : attribute name.
/// @param value : value of the attibute.
/// @return void
//-----------------------------------------------------------------------------
void SvgGenerator::setAttribute(QDomElement element, const QString &attr, const QString &value)
{
    if (!element.isNull() && !value.isEmpty())
    {
        element.setAttribute(attr, value);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Render a graphics scene into an SVG DOM document.
/// @details The scene is painted through Qt's QSvgGenerator into an in-memory buffer and
///          the resulting SVG markup is parsed into a QDomDocument for post-processing.
/// @param scene the scene to render.
/// @return the rendered SVG as a DOM document; a null document when parsing failed.
//---------------------------------------------------------------------------------------------------------------------
QDomDocument SvgGenerator::renderSceneToDom(QGraphicsScene *scene)
{
    QByteArray byte_array;
    QBuffer buffer(&byte_array);
    buffer.open(QIODevice::WriteOnly);

    QSvgGenerator svg_generator;
    svg_generator.setOutputDevice(&buffer);
    svg_generator.setSize(m_paper->rect().size().toSize());
    svg_generator.setViewBox(m_paper->rect());
    svg_generator.setTitle(QString());
    svg_generator.setDescription(QString());
    svg_generator.setResolution(m_resolution);

    QFont app_font;
    app_font.setPointSize(8);

    QPainter painter;
    painter.begin(&svg_generator);
    painter.setFont(app_font);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(QBrush(Qt::NoBrush));
    scene->render(&painter, m_paper->rect(), m_paper->rect(), Qt::IgnoreAspectRatio);
    painter.end();

    QDomDocument dom_doc;
    if (!dom_doc.setContent(byte_array))
    {
        qDebug() << "Error : Impossible to load the SVG content in the QDomDocument.";
        return QDomDocument();
    }

    buffer.close();
    return dom_doc;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Render each component of a piece separately and append the tagged groups to the piece group.
/// @details Qt's QSvgGenerator creates <g> elements from painter state changes, not from
///          graphics items, so a single render of the whole piece cannot be mapped back to
///          its components. Instead each direct child of the piece root (seamline, cutline,
///          notches, internal paths, labels, grainline, fold line) is rendered in its own pass with
///          every sibling hidden. Each pass therefore produces exactly one <g> that belongs
///          to one known component, which is tagged with the SVG data-* attributes (see
///          tagComponentGroup()). A child of a component that carries its own data-type (the
///          fold line arrow and label) is a part: it is rendered in its own pass too, and its
///          tagged group goes inside the component group, after what the component draws itself.
/// @param scene       scene containing (only) this piece's item tree.
/// @param item        piece root item whose children are the components.
/// @param piece_doc   DOM document of the piece; receives the component groups.
/// @param piece_group piece <g> element the component groups are appended to.
/// @param piece_id    SVG id of the piece group; the id/data-parent fallback when the piece has no name.
//---------------------------------------------------------------------------------------------------------------------
void SvgGenerator::addComponentGroups(QGraphicsScene *scene, QGraphicsItem *item, QDomDocument &piece_doc,
                                      QDomElement &piece_group, const QString &piece_id)
{
    const QList<QGraphicsItem *> components = item->childItems();
    QHash<QString, int> type_counters; // per data-type counter within this piece

    for (int i = 0; i < components.size(); ++i)
    {
        // Show only the current component; hide every sibling.
        for (int j = 0; j < components.size(); ++j)
        {
            components.at(j)->setVisible(i == j);
        }

        // Children with their own data-type are parts: render the component without them first.
        const QList<QGraphicsItem*> children = components.at(i)->childItems();
        QList<QGraphicsItem*> parts;
        for (QGraphicsItem* child : children)
        {
            if (!child->data(PieceItemData::ItemType).toString().isEmpty())
            {
                parts.append(child);
                child->setVisible(false);
            }
        }

        QDomElement component_group = renderGroup(scene, piece_doc);
        if (!component_group.isNull())
        {
            tagComponentGroup(component_group, components.at(i), type_counters, piece_group, piece_id);

            // Each part alone: every other child of the component hidden.
            for (QGraphicsItem* part : parts)
            {
                for (QGraphicsItem* child : children)
                {
                    child->setVisible(child == part);
                }
                QDomElement part_group = renderGroup(scene, piece_doc);
                if (!part_group.isNull())
                {
                    tagComponentGroup(part_group, part, type_counters, piece_group, piece_id);
                    component_group.appendChild(part_group);
                }
            }
            piece_group.appendChild(component_group);
        }
        // A component that painted nothing (e.g. a piece without notches) gets no group.

        for (QGraphicsItem* child : children)
        {
            child->setVisible(true);
        }
    }

    // Restore visibility so later exports (PDF, DXF, previews) see the whole piece.
    for (int j = 0; j < components.size(); ++j)
    {
        components.at(j)->setVisible(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Render the visible items of a scene into one cleaned group of the piece document.
/// @param scene     scene to render.
/// @param piece_doc DOM document of the piece; owns the returned group.
/// @return the group, not yet appended anywhere; a null element when nothing was painted.
//---------------------------------------------------------------------------------------------------------------------
QDomElement SvgGenerator::renderGroup(QGraphicsScene* scene, QDomDocument& piece_doc)
{
    QDomElement result;
    QDomDocument component_doc = renderSceneToDom(scene);
    const QDomNodeList groups = component_doc.isNull() ? QDomNodeList()
                                                       : component_doc.documentElement().elementsByTagName("g");
    if (!groups.isEmpty())
    {
        QDomElement group = groups.at(0).toElement();
        cleanSvg(group);
        if (!group.childNodes().isEmpty())
        {
            result = piece_doc.importNode(group, true).toElement();
        }
    }
    return result;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Tag a rendered component group so downstream tools (SeamlyLayout) can identify the component.
/// @details Sets data-type, data-type-number (per-type counter within the piece), data-parent (the piece
///          name), data-turn-points (path components only), data-notch-segments, data-seam-allowances and
///          data-notch-type (notch component only) and a name-based id ("<type>_<pieceName>", or
///          "<type>_<n>_<pieceName>" for types that can repeat per piece). A piece with no usable name falls
///          back to the legacy "<pieceId>-<type>-<n>" id, and a name-based id colliding with one already
///          emitted (piece names aren't guaranteed unique) is disambiguated with the piece's data-type-number.
/// @param group         rendered group of the component.
/// @param component     graphics item of the component; its PieceItemData values become attributes.
/// @param type_counters per data-type counter within the piece; the component's type is counted.
/// @param piece_group   piece <g> element; gives the piece name and size.
/// @param piece_id      SVG id of the piece group; the id/data-parent fallback when the piece has no name.
//---------------------------------------------------------------------------------------------------------------------
void SvgGenerator::tagComponentGroup(QDomElement& group, QGraphicsItem* component, QHash<QString, int>& type_counters,
                                     const QDomElement& piece_group, const QString& piece_id)
{
    QString type = component->data(PieceItemData::ItemType).toString();
    if (type.isEmpty())
    {
        type = QStringLiteral("unknown");
    }
    const int type_number = ++type_counters[type];

    // Name-based id (e.g. "grainline_Sleeve"); the numeric fallback is unique by construction.
    const QString piece_name = piece_group.attribute(QStringLiteral("data-name"));
    const QString component_id = m_ids.component_id(type, type_number, piece_name, piece_id,
                                                    piece_group.attribute(QStringLiteral("data-size")),
                                                    piece_group.attribute(QStringLiteral("data-type-number")).toInt());

    group.setAttribute("id", component_id);
    group.setAttribute("data-type", type);
    group.setAttribute("data-type-number", QString::number(type_number));
    group.setAttribute("data-parent", piece_name.isEmpty() ? piece_id : piece_name);
    // Turn point vertex indices of the component's path. An empty value is valid:
    // a smooth closed loop has no turn points.
    const QVariant turn_points = component->data(PieceItemData::TurnPoints);
    if (turn_points.isValid())
    {
        group.setAttribute("data-turn-points", turn_points.toString());
    }
    // Notch line counts, seam allowance widths and notch types, one per notch.
    const QVariant notch_segments = component->data(PieceItemData::NotchSegments);
    if (notch_segments.isValid())
    {
        group.setAttribute("data-notch-segments", notch_segments.toString());
    }
    const QVariant seam_allowances = component->data(PieceItemData::SeamAllowances);
    if (seam_allowances.isValid())
    {
        group.setAttribute("data-seam-allowances", seam_allowances.toString());
    }
    const QVariant notch_types = component->data(PieceItemData::NotchTypes);
    if (notch_types.isValid())
    {
        group.setAttribute("data-notch-type", notch_types.toString());
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Add a new SVG to the list of SVGs to be merged into a single SVG.
/// @details When a piece item is given, the piece becomes a <g> element carrying the SVG
///          data-* attributes (data-type="piece", data-type-number, data-parent,
///          data-name, data-letter, data-material, data-quantity, data-on-fold, data-rotation, data-tilt, data-flip,
///          data-orientation, data-annotation and data-grainline-angle) and each of
///          the piece's
///          components is rendered
///          and tagged individually (see addComponentGroups()). Without an item the
///          scene is rendered as-is in a single untagged group (whole-scene exports,
///          e.g. draft blocks).
/// @param scene         the scene that must be converted to SVG.
/// @param item          piece root item the scene contains; nullptr for whole-scene exports.
/// @param size          multisize size of this piece, written as data-size and appended to the ids; empty for an
///                      individual pattern.
/// @param piece_set_key key shared by the same piece in every size; the merge nests those pieces in one
///                      data-type="piece-set" group. Empty = no piece-set.
//---------------------------------------------------------------------------------------------------------------------
void SvgGenerator::addSvgFromScene(QGraphicsScene *scene, QGraphicsItem *item, const QString &size,
                                   const QString &piece_set_key)
{
    if (item == nullptr)
    {
        // Whole-scene export: keep the legacy single-group behavior.
        QDomDocument dom_doc = renderSceneToDom(scene);
        if (!dom_doc.isNull())
        {
            m_dom_list.append(dom_doc);
            m_dom_piece_set_keys.append(QString());
            m_dom_materials.append(QString());
        }
        return;
    }

    // One piece: build "<g id='piece_<name>' data-type='piece' ...>" holding one tagged group per component.
    ++m_piece_count;
    const QString piece_id = QString("piece-%1").arg(m_piece_count);

    // Full render only to obtain a document with the correct <svg> root attributes.
    QDomDocument piece_doc = renderSceneToDom(scene);
    if (piece_doc.isNull())
    {
        return;
    }

    QDomElement svg_root = piece_doc.documentElement();
    QDomNodeList root_groups = svg_root.elementsByTagName("g");
    if (!root_groups.isEmpty())
    {
        // Drop the untagged full-piece group; it is replaced by tagged component groups.
        root_groups.at(0).parentNode().removeChild(root_groups.at(0));
    }

    const QString piece_name = item->data(PieceItemData::ObjectName).toString();
    const QString svg_piece_id = m_ids.piece_id(piece_id, piece_name, size, m_piece_count);

    QDomElement piece_group = piece_doc.createElement("g");
    piece_group.setAttribute("id", svg_piece_id);
    piece_group.setAttribute("data-type", "piece");
    piece_group.setAttribute("data-type-number", QString::number(m_piece_count));
    piece_group.setAttribute("data-parent", "pattern-1");
    setAttribute(piece_group, "data-name", piece_name);
    setAttribute(piece_group, "data-letter", item->data(PieceItemData::PieceLetter).toString());
    setAttribute(piece_group, "data-size", size);
    const QString material = item->data(PieceItemData::Material).toString();
    setAttribute(piece_group, "data-material", material);
    const QVariant quantity = item->data(PieceItemData::Quantity);
    if (quantity.isValid())
    {
        piece_group.setAttribute("data-quantity", QString::number(quantity.toInt()));
    }
    if (item->data(PieceItemData::OnFold).toBool())
    {
        piece_group.setAttribute("data-on-fold", QStringLiteral("true"));
    }
    // Piece Properties for layout rules and DXF piece text; each is absent when it allows nothing.
    const QVariant rotation = item->data(PieceItemData::Rotation);
    if (rotation.isValid())
    {
        piece_group.setAttribute("data-rotation", QString::number(rotation.toInt()));
    }
    const QVariant tilt = item->data(PieceItemData::Tilt);
    if (tilt.isValid())
    {
        piece_group.setAttribute("data-tilt", QString::number(tilt.toInt()));
    }
    setAttribute(piece_group, "data-flip", item->data(PieceItemData::Flip).toString());
    setAttribute(piece_group, "data-orientation", item->data(PieceItemData::Orientation).toString());
    setAttribute(piece_group, "data-annotation", item->data(PieceItemData::Annotation).toString());
    const QVariant grain_angle = item->data(PieceItemData::GrainlineAngle);
    if (grain_angle.isValid())
    {
        piece_group.setAttribute("data-grainline-angle", QString::number(grain_angle.toDouble(), 'f', 4));
    }
    svg_root.appendChild(piece_group);

    addComponentGroups(scene, item, piece_doc, piece_group, piece_id);

    m_dom_list.append(piece_doc);
    m_dom_piece_set_keys.append(piece_set_key);
    m_dom_materials.append(material);
}


//---------------------------------------------------------------------------------------------------------------------
/// @brief Generate the merged SVG where each previously given scene is grouped separately.
/// @return void
/// @details This function merges the SVGs of the m_dom_list list and writes the result
///          in a file at the path given in the constructor.
//---------------------------------------------------------------------------------------------------------------------
void SvgGenerator::generate()
{
    const QString merged_svg = toSvgString();

    QFile output_file(m_filepath);
    if (!output_file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Error : Couldn't write the output file.";
        return;
    }

    QTextStream stream(&output_file);
    stream << merged_svg;
    output_file.close();

    qDebug() << "Merged SVG Generated!";
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Generate the merged SVG and return it as text instead of writing a file.
/// @return the merged SVG document, or an empty string when no scene was added.
/// @details Same merge as generate(), for callers that need the document in
///          memory: Layout Mode hands the pattern pieces to SeamlyLayout as one
///          stringified SVG on the child process's standard input, so no handoff
///          file is created.
//---------------------------------------------------------------------------------------------------------------------
QString SvgGenerator::toSvgString()
{
    return mergeSvgDoms().toString();
}
