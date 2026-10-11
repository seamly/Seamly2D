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
    /// @brief Turn a piece name into a valid, CSS-selector-safe SVG id fragment.
    /// @param name raw piece name; may contain spaces, punctuation, or be empty.
    /// @return name with every run of characters outside [A-Za-z0-9_.-] replaced by
    ///         '_', stray leading/trailing '_' trimmed, and a leading '_' added when
    ///         the result would otherwise start with a digit. Empty when nothing
    ///         sanitizable remains (callers must supply a fallback id in that case).
    //-----------------------------------------------------------------------------
    QString sanitizeForId(const QString &name)
    {
        QString sanitized = name;
        sanitized.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]+")), QStringLiteral("_"));
        sanitized.remove(QRegularExpression(QStringLiteral("^_+|_+$")));
        if (!sanitized.isEmpty() && sanitized.at(0).isDigit())
        {
            sanitized.prepend(QLatin1Char('_'));
        }
        return sanitized;
    }

    //-----------------------------------------------------------------------------
    /// @brief Id suffix for one size of a multisize piece, e.g. "_s34" or "_s13.5".
    //-----------------------------------------------------------------------------
    QString sizeIdSuffix(const QString &size)
    {
        QString sanitized = sanitizeForId(size);
        if (sanitized.startsWith(QLatin1Char('_')))
        {
            sanitized.remove(0, 1); // the digit guard is not needed after the "_s" prefix
        }
        return QStringLiteral("_s") + sanitized;
    }

    //-----------------------------------------------------------------------------
    /// @brief Component types that can repeat per piece, so their id keeps a
    ///        counter (`<type>_<n>_<name>`) instead of a bare `<type>_<name>`.
    //-----------------------------------------------------------------------------
    bool componentIdIsNumbered(const QString &type)
    {
        return type == QLatin1String("internal_path") || type == QLatin1String("cut_path")
            || type == QLatin1String("piece_label") || type == QLatin1String("pattern_label");
    }

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
/// @details The pattern group then carries data-measurements="multisize", data-sizes and
///          data-base-size. Without this call the pattern group carries
///          data-measurements="individual".
/// @param sizes     every size in the export, in pattern units, in the order the pieces are added.
/// @param base_size base size of the multisize measurements, in pattern units.
//---------------------------------------------------------------------------------------------------------------------
void SvgGenerator::setMultisize(const QStringList &sizes, const QString &base_size)
{
    m_sizes = sizes;
    m_base_size = base_size;
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

    // Piece-based exports get one pattern group wrapping all piece groups, tagged
    // with the SVG data-* attributes so downstream tools can identify the pattern.
    // Whole-scene exports (draft blocks) keep the legacy flat structure.
    QDomElement parent_element = merged_svg_root;
    if (m_piece_count > 0)
    {
        QDomElement pattern_group = merged_svg.createElement("g");
        pattern_group.setAttribute("id", "pattern-1");
        pattern_group.setAttribute("data-type", "pattern");
        pattern_group.setAttribute("data-type-number", "1");
        setAttribute(pattern_group, "data-name", m_pattern_name);
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
        }
        merged_svg_root.appendChild(pattern_group);
        parent_element = pattern_group;
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

        // A multisize piece goes into the piece-set that holds the same piece in every size.
        QDomElement target_element = parent_element;
        const QString piece_set_key = m_dom_piece_set_keys.value(i);
        if (!piece_set_key.isEmpty() && main_group.attribute(QStringLiteral("data-type")) == QLatin1String("piece"))
        {
            if (!piece_sets.contains(piece_set_key))
            {
                ++piece_set_count;
                const QString piece_name = main_group.attribute(QStringLiteral("data-name"));
                const QString sanitized_name = sanitizeForId(piece_name);
                QString piece_set_id = sanitized_name.isEmpty()
                    ? QStringLiteral("piece-set-%1").arg(piece_set_count)
                    : QStringLiteral("piece-set_%1").arg(sanitized_name);
                if (m_used_piece_ids.contains(piece_set_id))
                {
                    piece_set_id += QLatin1Char('-') + QString::number(piece_set_count);
                }
                m_used_piece_ids.insert(piece_set_id);

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
///          notches, internal paths, labels, grainline) is rendered in its own pass with
///          every sibling hidden. Each pass therefore produces exactly one <g> that belongs
///          to one known component, which is tagged with the SVG data-* attributes:
///          data-type, data-type-number (per-type counter within the piece), data-parent
///          (the piece name), data-turn-points (path components only), data-seam-allowances and
///          data-notch-type (notch component only) and a name-based
///          id ("<type>_<pieceName>", or "<type>_<n>_<pieceName>" for types that can repeat per piece). A piece
///          with no usable name falls back to the legacy "<pieceId>-<type>-<n>" id, and a
///          name-based id colliding with one already emitted (piece names aren't
///          guaranteed unique) is disambiguated with the piece's data-type-number.
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

        QDomDocument component_doc = renderSceneToDom(scene);
        if (component_doc.isNull())
        {
            continue;
        }

        QDomNodeList groups = component_doc.documentElement().elementsByTagName("g");
        if (groups.isEmpty())
        {
            continue;
        }

        QDomElement component_group = groups.at(0).toElement();
        cleanSvg(component_group);
        if (component_group.childNodes().isEmpty())
        {
            // Component painted nothing (e.g. a piece without notches) — skip it.
            continue;
        }

        // Tag the group so downstream tools (SeamlyLayout) can identify the component.
        QString type = components.at(i)->data(PieceItemData::ItemType).toString();
        if (type.isEmpty())
        {
            type = QStringLiteral("unknown");
        }
        const int type_number = ++type_counters[type];

        // Name-based id (e.g. "grainline_Sleeve") when the piece has a usable
        // name; falls back to the legacy numeric id, which is unique by
        // construction, when it does not. A name-based id that collides with
        // one already emitted (piece names aren't guaranteed unique — see
        // data-letter) is disambiguated with the piece's own data-type-number.
        const QString piece_name = piece_group.attribute(QStringLiteral("data-name"));
        const QString sanitized_name = sanitizeForId(piece_name);
        QString component_id = sanitized_name.isEmpty()
            ? QString("%1-%2-%3").arg(piece_id, type).arg(type_number)
            : (componentIdIsNumbered(type)
                   ? QString("%1_%2_%3").arg(type).arg(type_number).arg(sanitized_name)
                   : QString("%1_%2").arg(type, sanitized_name));
        // The same piece repeats once per size in a multisize export.
        const QString size = piece_group.attribute(QStringLiteral("data-size"));
        if (!size.isEmpty())
        {
            component_id += sizeIdSuffix(size);
        }
        if (m_used_ids.contains(component_id))
        {
            component_id += QLatin1Char('-') + piece_group.attribute(QStringLiteral("data-type-number"));
        }
        m_used_ids.insert(component_id);

        component_group.setAttribute("id", component_id);
        component_group.setAttribute("data-type", type);
        component_group.setAttribute("data-type-number", QString::number(type_number));
        component_group.setAttribute("data-parent", piece_name.isEmpty() ? piece_id : piece_name);
        // Turn point vertex indices of the component's path. An empty value is valid:
        // a smooth closed loop has no turn points.
        const QVariant turn_points = components.at(i)->data(PieceItemData::TurnPoints);
        if (turn_points.isValid())
        {
            component_group.setAttribute("data-turn-points", turn_points.toString());
        }
        // Notch seam allowance widths and notch types, one per notch line.
        const QVariant seam_allowances = components.at(i)->data(PieceItemData::SeamAllowances);
        if (seam_allowances.isValid())
        {
            component_group.setAttribute("data-seam-allowances", seam_allowances.toString());
        }
        const QVariant notch_types = components.at(i)->data(PieceItemData::NotchTypes);
        if (notch_types.isValid())
        {
            component_group.setAttribute("data-notch-type", notch_types.toString());
        }

        piece_group.appendChild(piece_doc.importNode(component_group, true));
    }

    // Restore visibility so later exports (PDF, DXF, previews) see the whole piece.
    for (int j = 0; j < components.size(); ++j)
    {
        components.at(j)->setVisible(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Add a new SVG to the list of SVGs to be merged into a single SVG.
/// @details When a piece item is given, the piece becomes a <g> element carrying the SVG
///          data-* attributes (data-type="piece", data-type-number, data-parent,
///          data-name, data-letter, data-quantity, data-on-fold and data-grainline-angle) and each of the piece's
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

    // Name-based id (e.g. "piece_Yoke") when the piece has a usable name;
    // falls back to the legacy numeric piece_id, which is unique by
    // construction, when it does not. A name-based id that collides with
    // one already emitted (piece names aren't guaranteed unique — see
    // data-letter) is disambiguated with the piece's own data-type-number.
    const QString piece_name = item->data(PieceItemData::ObjectName).toString();
    const QString sanitized_piece_name = sanitizeForId(piece_name);
    QString svg_piece_id = sanitized_piece_name.isEmpty()
        ? piece_id
        : QStringLiteral("piece_%1").arg(sanitized_piece_name);
    if (!size.isEmpty())
    {
        svg_piece_id += sizeIdSuffix(size);
    }
    if (m_used_piece_ids.contains(svg_piece_id))
    {
        svg_piece_id += QLatin1Char('-') + QString::number(m_piece_count);
    }
    m_used_piece_ids.insert(svg_piece_id);

    QDomElement piece_group = piece_doc.createElement("g");
    piece_group.setAttribute("id", svg_piece_id);
    piece_group.setAttribute("data-type", "piece");
    piece_group.setAttribute("data-type-number", QString::number(m_piece_count));
    piece_group.setAttribute("data-parent", "pattern-1");
    setAttribute(piece_group, "data-name", piece_name);
    setAttribute(piece_group, "data-letter", item->data(PieceItemData::PieceLetter).toString());
    setAttribute(piece_group, "data-size", size);
    const QVariant quantity = item->data(PieceItemData::Quantity);
    if (quantity.isValid())
    {
        piece_group.setAttribute("data-quantity", QString::number(quantity.toInt()));
    }
    if (item->data(PieceItemData::OnFold).toBool())
    {
        piece_group.setAttribute("data-on-fold", QStringLiteral("true"));
    }
    const QVariant grain_angle = item->data(PieceItemData::GrainlineAngle);
    if (grain_angle.isValid())
    {
        piece_group.setAttribute("data-grainline-angle", QString::number(grain_angle.toDouble(), 'f', 4));
    }
    svg_root.appendChild(piece_group);

    addComponentGroups(scene, item, piece_doc, piece_group, piece_id);

    m_dom_list.append(piece_doc);
    m_dom_piece_set_keys.append(piece_set_key);
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
