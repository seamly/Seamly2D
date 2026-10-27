//---------------------------------------------------------------------------------------------------------------------
//  @file   handoff_document.cpp
//  @author slspencer
//
//  @brief
//  The tagged pieces SVG that Seamly2D hands to SeamlyLayout.
//
//  @copyright
//  This source code is part of the Seamly2D project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2026 Seamly2D Project
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

#include "handoff_document.h"

#include <QFontMetrics>
#include <QFontMetricsF>
#include <QHash>
#include <QLineF>
#include <QTransform>
#include <QtMath>

#include "handoff_ids.h"
#include "../ifc/ifcdef.h"
#include "../vmisc/def.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"

namespace layout_handoff
{
namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief number writes a coordinate with at most eight significant digits.
 */
QString number(qreal value)
{
    return QString::number(qFuzzyIsNull(value) ? 0.0 : value, 'g', 8);
}

//---------------------------------------------------------------------------------------------------------------------
QString point_text(const QPointF &point)
{
    return number(point.x()) + QLatin1Char(',') + number(point.y());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief polyline_data writes the path data of a polyline.
 * @param closed true to draw back to the first point and close the subpath.
 */
QString polyline_data(const QVector<QPointF> &points, bool closed)
{
    QString data;
    for (int i = 0; i < points.size(); ++i)
    {
        data += (i == 0 ? QStringLiteral("M") : QStringLiteral(" L")) + point_text(points.at(i));
    }
    if (closed && points.size() > 1)
    {
        if (points.first() != points.last())
        {
            data += QStringLiteral(" L") + point_text(points.first());
        }
        data += QStringLiteral(" Z");
    }
    return data;
}

//---------------------------------------------------------------------------------------------------------------------
QString line_data(const HandoffLine &line)
{
    return line.points.size() < 2 ? QString() : polyline_data(line.points, line.closed);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief triangle_data writes a closed triangle.
 */
QString triangle_data(const QVector<QPointF> &points)
{
    return polyline_data(points, true);
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
QString numbers_text(const QVector<T> &values)
{
    QStringList parts;
    for (const T &value : values)
    {
        parts.append(QString::number(value));
    }
    return parts.join(QLatin1Char(' '));
}

//---------------------------------------------------------------------------------------------------------------------
QString transform_text(const QTransform &transform)
{
    return QStringLiteral("matrix(%1,%2,%3,%4,%5,%6)")
        .arg(number(transform.m11()), number(transform.m12()), number(transform.m21()), number(transform.m22()),
             number(transform.dx()), number(transform.dy()));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief dash_array writes the stroke-dasharray of a pen style; empty for a solid line.
 */
QString dash_array(Qt::PenStyle type, qreal weight)
{
    QVector<qreal> pattern;
    switch (type)
    {
        case Qt::DashLine:       pattern = {4, 2};             break;
        case Qt::DotLine:        pattern = {1, 2};             break;
        case Qt::DashDotLine:    pattern = {4, 2, 1, 2};       break;
        case Qt::DashDotDotLine: pattern = {4, 2, 1, 2, 1, 2}; break;
        default:                 break;
    }
    const qreal scale = weight > 0 ? weight : 1;
    QStringList parts;
    for (qreal value : pattern)
    {
        parts.append(number(value * scale));
    }
    return parts.join(QLatin1Char(','));
}

//---------------------------------------------------------------------------------------------------------------------
void set_if_not_empty(QDomElement element, const QString &name, const QString &value)
{
    if (!value.isEmpty())
    {
        element.setAttribute(name, value);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PieceWriter writes the component groups of one piece into its piece group.
 */
class PieceWriter
{
public:
    PieceWriter(QDomDocument doc, const HandoffStyle &style, HandoffIds *ids, const HandoffPlacedPiece &placed,
                QDomElement piece_group, const QString &piece_id)
        : m_doc(doc)
        , m_style(style)
        , m_ids(ids)
        , m_placed(placed)
        , m_piece_group(piece_group)
        , m_piece_id(piece_id)
    {
    }

    void write_components();

private:
    QDomElement make_group(const QString &type);
    QDomElement add_path(QDomElement group, const QString &path_data, const HandoffLineStyle &style,
                         Qt::PenCapStyle cap, const QString &join);
    QDomElement make_text(const QString &text, const QFont &font, const QColor &color, const QTransform &transform);
    void        add_path_component(const QString &type, const QString &path_data, const HandoffLineStyle &style,
                                   const std::optional<QVector<int>> &turn_points);
    void        add_seamline();
    void        add_cutline();
    void        add_notches();
    void        add_fold_line();
    void        add_paths(const QVector<HandoffPath> &paths, const QString &type);
    void        add_label(const HandoffLabel &label, const QString &type);
    void        add_grainline();
    QTransform  placement() const;

    QDomDocument              m_doc;
    const HandoffStyle       &m_style;
    HandoffIds               *m_ids;
    const HandoffPlacedPiece &m_placed;
    QDomElement               m_piece_group;
    QString                   m_piece_id;
    QHash<QString, int>       m_type_counters;
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_group makes the tagged group of a component and counts its type.
 */
QDomElement PieceWriter::make_group(const QString &type)
{
    const int     type_number = ++m_type_counters[type];
    const QString piece_name  = m_piece_group.attribute(QStringLiteral("data-name"));

    QDomElement group = m_doc.createElement(QStringLiteral("g"));
    group.setAttribute(QStringLiteral("id"),
                       m_ids->component_id(type, type_number, piece_name, m_piece_id, m_placed.info.size,
                                           m_piece_group.attribute(QStringLiteral("data-type-number")).toInt()));
    group.setAttribute(QStringLiteral("data-type"), type);
    group.setAttribute(QStringLiteral("data-type-number"), QString::number(type_number));
    group.setAttribute(QStringLiteral("data-parent"), piece_name.isEmpty() ? m_piece_id : piece_name);
    group.setAttribute(QStringLiteral("fill"), QStringLiteral("none"));
    group.setAttribute(QStringLiteral("fill-rule"), QStringLiteral("evenodd"));
    group.setAttribute(QStringLiteral("stroke"), QStringLiteral("black"));
    group.setAttribute(QStringLiteral("stroke-linecap"), QStringLiteral("square"));
    group.setAttribute(QStringLiteral("stroke-linejoin"), QStringLiteral("bevel"));
    group.setAttribute(QStringLiteral("stroke-width"), QStringLiteral("1"));
    return group;
}

//---------------------------------------------------------------------------------------------------------------------
QTransform PieceWriter::placement() const
{
    return QTransform::fromTranslate(m_placed.offset.x(), m_placed.offset.y());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief add_path adds a styled group with one path to a component group; the group places the path on the sheet.
 * @return the styled group.
 */
QDomElement PieceWriter::add_path(QDomElement group, const QString &path_data, const HandoffLineStyle &style,
                                  Qt::PenCapStyle cap, const QString &join)
{
    QDomElement styled = m_doc.createElement(QStringLiteral("g"));
    styled.setAttribute(QStringLiteral("fill"), QStringLiteral("none"));
    styled.setAttribute(QStringLiteral("stroke"), style.color.name());
    styled.setAttribute(QStringLiteral("stroke-opacity"), QStringLiteral("1"));
    styled.setAttribute(QStringLiteral("stroke-width"), number(style.weight));
    set_if_not_empty(styled, QStringLiteral("stroke-dasharray"), dash_array(style.type, style.weight));
    if (style.type != Qt::SolidLine)
    {
        styled.setAttribute(QStringLiteral("stroke-dashoffset"), QStringLiteral("0"));
    }
    QString cap_text = QStringLiteral("square");
    if (cap == Qt::RoundCap)
    {
        cap_text = QStringLiteral("round");
    }
    else if (cap == Qt::FlatCap)
    {
        cap_text = QStringLiteral("butt");
    }
    styled.setAttribute(QStringLiteral("stroke-linecap"), cap_text);
    styled.setAttribute(QStringLiteral("stroke-linejoin"), join);
    styled.setAttribute(QStringLiteral("transform"), transform_text(placement()));

    QDomElement path = m_doc.createElement(QStringLiteral("path"));
    path.setAttribute(QStringLiteral("d"), path_data);
    path.setAttribute(QStringLiteral("fill-rule"), QStringLiteral("evenodd"));
    path.setAttribute(QStringLiteral("vector-effect"), QStringLiteral("none"));
    styled.appendChild(path);
    group.appendChild(styled);
    return styled;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief add_path_component writes a component that is one path; empty path data writes nothing.
 */
void PieceWriter::add_path_component(const QString &type, const QString &path_data, const HandoffLineStyle &style,
                                     const std::optional<QVector<int>> &turn_points)
{
    if (path_data.isEmpty())
    {
        return;
    }
    QDomElement group = make_group(type);
    if (turn_points.has_value())
    {
        group.setAttribute(QStringLiteral("data-turn-points"), numbers_text(turn_points.value()));
    }
    add_path(group, path_data, style, Qt::RoundCap, QStringLiteral("round"));
    m_piece_group.appendChild(group);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief add_seamline writes the seam line; with a built-in seam allowance it is the line inside the cut line.
 */
void PieceWriter::add_seamline()
{
    const HandoffPiece     &piece = m_placed.piece;
    const HandoffLineStyle &style = piece.has_seam_allowance() ? m_style.seam : m_style.cut;
    if (piece.has_seam_allowance() && piece.has_built_in_seam_allowance())
    {
        if (!piece.is_seam_line_hidden())
        {
            add_path_component(QStringLiteral("seamline"), line_data(piece.built_in_seam_line()), style,
                               piece.built_in_seam_line().turn_points);
        }
    }
    else if (!piece.is_seam_line_hidden() || !piece.has_seam_allowance())
    {
        add_path_component(QStringLiteral("seamline"), line_data(piece.seam_line()), style,
                           piece.seam_line().turn_points);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief add_cutline writes the cut line; with a built-in seam allowance the outline is the cut line.
 */
void PieceWriter::add_cutline()
{
    const HandoffPiece &piece = m_placed.piece;
    if (piece.has_seam_allowance() && piece.has_built_in_seam_allowance())
    {
        add_path_component(QStringLiteral("cutline"), line_data(piece.seam_line()), m_style.cut,
                           piece.seam_line().turn_points);
    }
    else if (m_style.show_seam_allowances && piece.has_seam_allowance())
    {
        add_path_component(QStringLiteral("cutline"), line_data(piece.cut_line()), m_style.cut,
                           piece.cut_line().turn_points);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void PieceWriter::add_notches()
{
    const HandoffNotches &notches = m_placed.piece.notches();
    if (notches.lines.isEmpty())
    {
        return;
    }

    QStringList parts;
    for (const QLineF &line : notches.lines)
    {
        parts.append(QStringLiteral("M") + point_text(line.p1()) + QStringLiteral(" L") + point_text(line.p2()));
    }
    QDomElement group = make_group(QStringLiteral("notch"));
    if (!notches.line_counts.isEmpty())
    {
        group.setAttribute(QStringLiteral("data-notch-segments"), numbers_text(notches.line_counts));
    }
    if (!notches.seam_allowance_widths.isEmpty())
    {
        QStringList widths;
        for (qreal width : notches.seam_allowance_widths)
        {
            widths.append(QString::number(FromPixel(width, Unit::Mm), 'f', 2));
        }
        group.setAttribute(QStringLiteral("data-seam-allowances"), widths.join(QLatin1Char(' ')));
    }
    if (!notches.types.isEmpty())
    {
        group.setAttribute(QStringLiteral("data-notch-type"), notches.types.join(QLatin1Char(' ')));
    }
    HandoffLineStyle style;
    style.color  = m_style.notch_color;
    style.weight = m_style.cut.weight;
    add_path(group, parts.join(QLatin1Char(' ')), style, Qt::RoundCap, QStringLiteral("round"));
    m_piece_group.appendChild(group);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief add_paths writes the internal paths or the cutout paths, one group each.
 */
void PieceWriter::add_paths(const QVector<HandoffPath> &paths, const QString &type)
{
    for (const HandoffPath &path : paths)
    {
        HandoffLineStyle style;
        style.color  = QColor(path.color);
        style.type   = path.line_type;
        style.weight = ToPixel(path.line_weight.toDouble(), Unit::Mm);
        add_path_component(type, line_data(path.line), style, path.line.turn_points);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_text makes a group that holds one text element and places it on the sheet.
 * @param transform places the text in piece coordinates; its origin is the top left corner of the text.
 */
QDomElement PieceWriter::make_text(const QString &text, const QFont &font, const QColor &color,
                                   const QTransform &transform)
{
    const qreal   size   = font.pixelSize() > 0 ? font.pixelSize() : font.pointSizeF() * 96.0 / 72.0;
    const QString style  = font.italic() ? QStringLiteral("italic") : QStringLiteral("normal");
    const QString weight = font.bold() ? QStringLiteral("700") : QStringLiteral("400");

    QDomElement group = m_doc.createElement(QStringLiteral("g"));
    group.setAttribute(QStringLiteral("fill"), QStringLiteral("none"));
    group.setAttribute(QStringLiteral("stroke"), color.name());
    group.setAttribute(QStringLiteral("transform"), transform_text(transform * placement()));

    QDomElement element = m_doc.createElement(QStringLiteral("text"));
    element.setAttribute(QStringLiteral("fill"), color.name());
    element.setAttribute(QStringLiteral("fill-opacity"), QStringLiteral("1"));
    element.setAttribute(QStringLiteral("font-family"), font.family());
    element.setAttribute(QStringLiteral("font-size"), number(size));
    element.setAttribute(QStringLiteral("font-style"), style);
    element.setAttribute(QStringLiteral("font-weight"), weight);
    element.setAttribute(QStringLiteral("stroke"), QStringLiteral("none"));
    element.setAttribute(QStringLiteral("x"), QStringLiteral("0"));
    element.setAttribute(QStringLiteral("xml:space"), QStringLiteral("preserve"));
    element.setAttribute(QStringLiteral("y"), number(QFontMetricsF(font).ascent()));
    element.appendChild(m_doc.createTextNode(text));
    group.appendChild(element);
    return group;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief add_fold_line writes the fold line with its arrow and its label as nested parts.
 * @details The fold line goes out to the cut line when the cut line shows. The fold line comes first in the group,
 *          because readers take the first path as the fold line.
 */
void PieceWriter::add_fold_line()
{
    const HandoffPiece &piece = m_placed.piece;
    QLineF              line  = piece.fold_line();
    if (m_style.show_seam_allowances && !piece.fold_line_on_cut_line().isNull())
    {
        line = piece.fold_line_on_cut_line();
    }
    if (line.isNull())
    {
        return;
    }

    QDomElement group = make_group(QStringLiteral("fold_line"));
    add_path(group, polyline_data({line.p1(), line.p2()}, false), m_style.fold_line, Qt::FlatCap,
             QStringLiteral("round"));

    const pattern_piece::FoldLineGraphic &graphic = piece.fold_line_graphic();
    if (!graphic.isNull())
    {
        QDomElement arrow = make_group(QStringLiteral("fold_line_arrow"));
        add_path(arrow, polyline_data(graphic.arrowLine(), false), m_style.fold_line, Qt::FlatCap,
                 QStringLiteral("miter"));
        QDomElement heads = add_path(arrow,
                                     triangle_data(graphic.startArrowHead()) + QLatin1Char(' ')
                                         + triangle_data(graphic.endArrowHead()),
                                     m_style.fold_line, Qt::SquareCap, QStringLiteral("bevel"));
        heads.setAttribute(QStringLiteral("fill"), m_style.fold_line.color.name());
        heads.setAttribute(QStringLiteral("stroke"), QStringLiteral("none"));
        heads.removeAttribute(QStringLiteral("stroke-width"));
        heads.removeAttribute(QStringLiteral("stroke-dasharray"));
        heads.removeAttribute(QStringLiteral("stroke-dashoffset"));
        group.appendChild(arrow);

        QDomElement label = make_group(QStringLiteral("fold_line_label"));
        const QFont         font = graphic.labelFont(m_style.label_font);
        const QFontMetricsF metrics(font);
        const QString       text = pattern_piece::FoldLineGraphic::labelText();
        const QTransform    transform =
            graphic.labelTransform(QSizeF(metrics.horizontalAdvance(text), metrics.height()));
        label.appendChild(make_text(text, font, m_style.fold_line.color, transform));
        group.appendChild(label);
    }
    m_piece_group.appendChild(group);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief add_label writes the text lines of a label.
 * @details Each line keeps its font, alignment, middle elide to the label width, mirror and rotation. The writer
 *          leaves out the lines that do not fit into the label height.
 */
void PieceWriter::add_label(const HandoffLabel &label, const QString &type)
{
    if (!m_style.show_labels || !label.is_visible() || label.shape().size() < 3)
    {
        return;
    }

    const QVector<QPointF> &shape  = label.shape();
    const qreal             width  = QLineF(shape.at(0), shape.at(1)).length();
    const qreal             height = QLineF(shape.at(1), shape.at(2)).length();
    const qreal             angle  = -QLineF(shape.at(0), shape.at(1)).angle();

    QDomElement group = make_group(type);
    qreal       y     = 0;
    for (const TextLine &line : label.lines())
    {
        if (y > height)
        {
            break;
        }

        QFont font = label.font();
        font.setPixelSize(label.font().pixelSize() + line.font_size_increment);
        font.setBold(line.bold);
        font.setItalic(line.italic);
        const QFontMetrics metrics(font);

        QString text = line.text;
        if (metrics.horizontalAdvance(text) > width)
        {
            text = metrics.elidedText(text, Qt::ElideMiddle, static_cast<int>(width));
        }
        qreal x = width - metrics.horizontalAdvance(text);
        if ((line.alignment & Qt::AlignLeft) > 0)
        {
            x = 0;
        }
        else if ((line.alignment & Qt::AlignHCenter) > 0)
        {
            x = (width - metrics.horizontalAdvance(text)) / 2;
        }

        QTransform transform;
        transform.translate(shape.at(0).x(), shape.at(0).y());
        if (m_placed.mirror)
        {
            transform.scale(-1, 1);
            transform.rotate(-angle);
            transform.translate(-width, 0);
        }
        else
        {
            transform.rotate(angle);
        }
        transform.translate(x, y);

        group.appendChild(make_text(text, font, m_style.label_color, transform));
        y += metrics.height() + label.spacing();
    }
    if (group.hasChildNodes())
    {
        m_piece_group.appendChild(group);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief add_grainline writes the grainline as a filled path.
 */
void PieceWriter::add_grainline()
{
    const HandoffPiece &piece = m_placed.piece;
    if (!m_style.show_grainlines || !piece.is_grainline_visible() || piece.grainline_points().size() < 2)
    {
        return;
    }

    QDomElement      group = make_group(QStringLiteral("grainline"));
    HandoffLineStyle style;
    style.color        = m_style.grainline_color;
    QDomElement styled = add_path(group, polyline_data(piece.grainline_points(), false), style, Qt::SquareCap,
                                  QStringLiteral("bevel"));
    styled.setAttribute(QStringLiteral("fill"), style.color.name());
    styled.setAttribute(QStringLiteral("fill-opacity"), QStringLiteral("1"));
    m_piece_group.appendChild(group);
}

//---------------------------------------------------------------------------------------------------------------------
void PieceWriter::write_components()
{
    add_seamline();
    add_cutline();
    add_notches();
    add_fold_line();
    add_paths(m_placed.piece.internal_paths(), QStringLiteral("internal_path"));
    add_paths(m_placed.piece.cutout_paths(), QStringLiteral("cut_path"));
    add_label(m_placed.piece_label, QStringLiteral("piece_label"));
    add_label(m_placed.pattern_label, QStringLiteral("pattern_label"));
    add_grainline();
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
HandoffStyle HandoffStyle::from_settings(const VCommonSettings &settings)
{
    HandoffStyle style;
    style.seam.color           = QColor(settings.getDefaultSeamColor());
    style.seam.type            = lineTypeToPenStyle(settings.getDefaultSeamLinetype());
    style.seam.weight          = ToPixel(settings.getDefaultSeamLineweight(), Unit::Mm);
    style.cut.color            = QColor(settings.getDefaultCutColor());
    style.cut.type             = lineTypeToPenStyle(settings.getDefaultCutLinetype());
    style.cut.weight           = ToPixel(settings.getDefaultCutLineweight(), Unit::Mm);
    style.fold_line.color      = QColor(settings.getDefaultFoldLineColor());
    style.fold_line.type       = lineTypeToPenStyle(settings.getDefaultFoldLineLinetype());
    style.fold_line.weight     = ToPixel(settings.getDefaultFoldLineLineweight(), Unit::Mm);
    style.notch_color          = QColor(settings.getDefaultNotchColor());
    style.grainline_color      = QColor(settings.getDefaultGrainlineColor());
    style.label_color          = QColor(settings.getDefaultLabelColor());
    style.label_font           = settings.getLabelFont();
    style.show_labels          = settings.showLabels();
    style.show_grainlines      = settings.showGrainlines();
    style.show_seam_allowances = settings.showSeamAllowances();
    return style;
}

//---------------------------------------------------------------------------------------------------------------------
HandoffPieceInfo HandoffPieceInfo::create(const VPiece &piece, const QString &size, const QString &piece_set_key)
{
    const VPieceLabelData &data = piece.GetPatternPieceData();

    HandoffPieceInfo info;
    info.name          = piece.GetName();
    info.letter        = data.GetLetter();
    info.material      = piece.getMaterial();
    info.size          = size;
    info.piece_set_key = piece_set_key;
    info.quantity      = data.GetQuantity();
    info.rotation      = VPieceLabelData::rotationStepDegrees(data.getRotationWay());
    if (data.GetTilt() != PieceTilt::None && data.getTiltDegrees() > 0)
    {
        info.tilt = (data.GetTilt() == PieceTilt::Clockwise ? -1 : 1) * data.getTiltDegrees();
    }
    if (piece.getFlip() != PieceFlip::None)
    {
        info.flip = pieceFlipToString(piece.getFlip());
    }
    const QString orientation = data.GetOrientation();
    if (orientation == QLatin1String("Left") || orientation == QLatin1String("Right"))
    {
        info.orientation = orientation;
    }
    info.annotation = data.GetAnnotation();
    return info;
}

//---------------------------------------------------------------------------------------------------------------------
void HandoffDocument::set_style(const HandoffStyle &style)
{
    m_style = style;
}

//---------------------------------------------------------------------------------------------------------------------
void HandoffDocument::set_pattern_name(const QString &name)
{
    m_pattern_name = name;
}

//---------------------------------------------------------------------------------------------------------------------
void HandoffDocument::set_units(const QString &units)
{
    m_units = units;
}

//---------------------------------------------------------------------------------------------------------------------
void HandoffDocument::set_sample_size(const QString &sample_size)
{
    m_sample_size = sample_size;
}

//---------------------------------------------------------------------------------------------------------------------
void HandoffDocument::set_multisize(const QStringList &sizes, const QString &base_size, const QString &height)
{
    m_sizes     = sizes;
    m_base_size = base_size;
    m_height    = height;
}

//---------------------------------------------------------------------------------------------------------------------
void HandoffDocument::set_sheet(const QRectF &rect, int resolution)
{
    m_sheet      = rect;
    m_resolution = resolution;
}

//---------------------------------------------------------------------------------------------------------------------
void HandoffDocument::add_piece(const HandoffPlacedPiece &placed)
{
    m_pieces.append(placed);
}

//---------------------------------------------------------------------------------------------------------------------
QDomDocument HandoffDocument::to_dom() const
{
    QDomDocument doc;
    doc.appendChild(doc.createProcessingInstruction(QStringLiteral("xml"),
                                                    QStringLiteral("version='1.0' encoding='UTF-8' standalone='no'")));

    const qreal mm_per_pixel = 25.4 / m_resolution;
    QDomElement root         = doc.createElement(QStringLiteral("svg"));
    root.setAttribute(QStringLiteral("baseProfile"), QStringLiteral("tiny"));
    root.setAttribute(QStringLiteral("version"), QStringLiteral("1.2"));
    root.setAttribute(QStringLiteral("width"), QString::number(m_sheet.width() * mm_per_pixel) + "mm");
    root.setAttribute(QStringLiteral("height"), QString::number(m_sheet.height() * mm_per_pixel) + "mm");
    root.setAttribute(QStringLiteral("viewBox"),
                      QStringLiteral("%1 %2 %3 %4")
                          .arg(number(m_sheet.x()), number(m_sheet.y()), number(m_sheet.width()),
                               number(m_sheet.height())));
    root.setAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.w3.org/2000/svg"));
    root.setAttribute(QStringLiteral("xmlns:xlink"), QStringLiteral("http://www.w3.org/1999/xlink"));
    root.appendChild(doc.createElement(QStringLiteral("defs")));
    doc.appendChild(root);

    // One pattern group per material, in the order of the first piece of each material.
    QStringList materials;
    for (const HandoffPlacedPiece &placed : m_pieces)
    {
        if (!materials.contains(placed.info.material))
        {
            materials.append(placed.info.material);
        }
    }
    QHash<QString, QDomElement> pattern_groups;
    for (int n = 0; n < materials.size(); ++n)
    {
        QDomElement group = doc.createElement(QStringLiteral("g"));
        group.setAttribute(QStringLiteral("id"), QStringLiteral("pattern-%1").arg(n + 1));
        group.setAttribute(QStringLiteral("data-type"), QStringLiteral("pattern"));
        group.setAttribute(QStringLiteral("data-type-number"), QString::number(n + 1));
        set_if_not_empty(group, QStringLiteral("data-name"), m_pattern_name);
        set_if_not_empty(group, QStringLiteral("data-material"), materials.at(n));
        set_if_not_empty(group, QStringLiteral("data-units"), m_units);
        if (m_sizes.isEmpty())
        {
            group.setAttribute(QStringLiteral("data-measurements"), QStringLiteral("individual"));
            set_if_not_empty(group, QStringLiteral("data-sample-size"), m_sample_size);
        }
        else
        {
            group.setAttribute(QStringLiteral("data-measurements"), QStringLiteral("multisize"));
            group.setAttribute(QStringLiteral("data-sizes"), m_sizes.join(QLatin1Char(',')));
            set_if_not_empty(group, QStringLiteral("data-base-size"), m_base_size);
            set_if_not_empty(group, QStringLiteral("data-height"), m_height);
        }
        root.appendChild(group);
        pattern_groups.insert(materials.at(n), group);
    }

    HandoffIds                  ids;
    QHash<QString, QDomElement> piece_sets;
    int                         piece_set_count = 0;
    int                         piece_count     = 0;
    for (const HandoffPlacedPiece &placed : m_pieces)
    {
        const HandoffPieceInfo &info = placed.info;
        ++piece_count;
        const QString piece_id = QStringLiteral("piece-%1").arg(piece_count);

        QDomElement pattern_group = pattern_groups.value(info.material);
        QDomElement parent        = pattern_group;
        if (!info.piece_set_key.isEmpty())
        {
            if (!piece_sets.contains(info.piece_set_key))
            {
                ++piece_set_count;
                QDomElement set = doc.createElement(QStringLiteral("g"));
                set.setAttribute(QStringLiteral("id"), ids.piece_set_id(info.name, piece_set_count));
                set.setAttribute(QStringLiteral("data-type"), QStringLiteral("piece-set"));
                set.setAttribute(QStringLiteral("data-type-number"), QString::number(piece_set_count));
                set.setAttribute(QStringLiteral("data-parent"), pattern_group.attribute(QStringLiteral("id")));
                set_if_not_empty(set, QStringLiteral("data-name"), info.name);
                set_if_not_empty(set, QStringLiteral("data-letter"), info.letter);
                pattern_group.appendChild(set);
                piece_sets.insert(info.piece_set_key, set);
            }
            parent = piece_sets.value(info.piece_set_key);
        }

        QDomElement group = doc.createElement(QStringLiteral("g"));
        group.setAttribute(QStringLiteral("id"), ids.piece_id(piece_id, info.name, info.size, piece_count));
        group.setAttribute(QStringLiteral("data-type"), QStringLiteral("piece"));
        group.setAttribute(QStringLiteral("data-type-number"), QString::number(piece_count));
        group.setAttribute(QStringLiteral("data-parent"), parent.attribute(QStringLiteral("id")));
        set_if_not_empty(group, QStringLiteral("data-name"), info.name);
        set_if_not_empty(group, QStringLiteral("data-letter"), info.letter);
        set_if_not_empty(group, QStringLiteral("data-size"), info.size);
        set_if_not_empty(group, QStringLiteral("data-material"), info.material);
        group.setAttribute(QStringLiteral("data-quantity"), QString::number(info.quantity));
        if (placed.piece.is_on_fold())
        {
            group.setAttribute(QStringLiteral("data-on-fold"), QStringLiteral("true"));
        }
        if (info.rotation > 0)
        {
            group.setAttribute(QStringLiteral("data-rotation"), QString::number(info.rotation));
        }
        if (info.tilt != 0)
        {
            group.setAttribute(QStringLiteral("data-tilt"), QString::number(info.tilt));
        }
        set_if_not_empty(group, QStringLiteral("data-flip"), info.flip);
        set_if_not_empty(group, QStringLiteral("data-orientation"), info.orientation);
        set_if_not_empty(group, QStringLiteral("data-annotation"), info.annotation);

        // The grain direction stays when the grainline is hidden, so SeamlyLayout can still orient the piece.
        const QVector<QPointF> &axis = placed.piece.grain_axis();
        if (axis.size() == 2 && !qFuzzyIsNull(QLineF(axis.at(0), axis.at(1)).length()))
        {
            group.setAttribute(QStringLiteral("data-grainline-angle"),
                               QString::number(QLineF(axis.at(0), axis.at(1)).angle(), 'f', 4));
        }

        PieceWriter writer(doc, m_style, &ids, placed, group, piece_id);
        writer.write_components();
        parent.appendChild(group);
    }
    return doc;
}

//---------------------------------------------------------------------------------------------------------------------
QString HandoffDocument::to_svg_string() const
{
    return to_dom().toString();
}
} // namespace layout_handoff
