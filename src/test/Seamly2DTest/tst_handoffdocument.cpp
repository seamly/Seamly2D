/******************************************************************************
 **  @file   tst_handoffdocument.cpp
 **  @author slspencer
 **
 **  @brief
 **  Unit tests for the tagged pieces SVG of layout_handoff
 **
 **  @copyright
 **  Copyright (C) 2026 Seamly2D Project
 **  <https://github.com/fashionfreedom/seamly2d> All Rights Reserved.
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
 **  SPDX-License-Identifier: GPL-3.0-or-later
 **
 *****************************************************************************/

#include "tst_handoffdocument.h"
#include "../layout_handoff/handoff_document.h"
#include "../layout_handoff/handoff_sheet.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vformat/svg_generator.h"
#include "../vgeometry/vpointf.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"

#include <QtTest>
#include <QDomDocument>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QRegularExpression>

using namespace layout_handoff;

namespace
{
/**
 * @brief TestPatternDocument is a pattern document with no tools; it holds the label template.
 */
class TestPatternDocument : public VAbstractPattern
{
public:
    TestPatternDocument()
    {
        setContent(QStringLiteral("<pattern/>"));
    }

    void CreateEmptyFile() override {}
    void IncrementReferens(quint32) const override {}
    void DecrementReferens(quint32) const override {}
    QStringList GetCurrentAlphabet() const override { return QStringList(); }
    QString GenerateLabel(const LabelType &, const QString &) const override { return QString(); }
    QString generateSuffix(const QString &) const override { return QString(); }
    void UpdateToolData(const quint32 &, VContainer *) override {}
    void LiteParseTree(const Document &) override {}
};

/**
 * @brief Entry is one comparable item of an SVG document: a tag and the points after all transforms.
 */
struct Entry
{
    QString          tag;
    QVector<QPointF> points;
};

//---------------------------------------------------------------------------------------------------------------------
VLabelTemplateLine label_line(const QString &text, bool bold, int alignment, int increment)
{
    VLabelTemplateLine line;
    line.line              = text;
    line.bold              = bold;
    line.italic            = false;
    line.alignment         = alignment;
    line.fontSizeIncrement = increment;
    return line;
}

//---------------------------------------------------------------------------------------------------------------------
void show_label(VPatternLabelData *label, const QPointF &pos, const QString &width, const QString &height)
{
    label->SetVisible(true);
    label->SetPos(pos);
    label->SetLabelWidth(width);
    label->SetLabelHeight(height);
    label->SetRotation(QStringLiteral("0"));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief set_label_templates gives the document and the piece their label templates.
 */
void set_label_templates(TestPatternDocument *document, VPiece *piece)
{
    QVector<VLabelTemplateLine> pattern_lines;
    pattern_lines << label_line(QStringLiteral("Jacket"), true, Qt::AlignHCenter, 1)
                  << label_line(QStringLiteral("Size %size%"), false, Qt::AlignRight, 0);
    document->setPatternLabelTemplate(pattern_lines);

    QVector<VLabelTemplateLine> piece_lines;
    piece_lines << label_line(QStringLiteral("%pName%"), true, Qt::AlignHCenter, 2)
                << label_line(QStringLiteral("Piece %pLetter%"), false, Qt::AlignLeft, 0);
    piece->GetPatternPieceData().SetLabelTemplate(piece_lines);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_piece builds a 400 by 400 pixel square with a slit notch, labels, a grainline and optional fold line.
 * @param data            receives the corner points. The pattern unit is millimeters.
 * @param seam_allowance  whether the piece has a seam allowance.
 * @param built_in        whether the seam allowance is built into the main path.
 * @param on_fold         whether the left side of the square is the fold line.
 */
VPiece make_piece(VContainer *data, bool seam_allowance, bool built_in, bool on_fold)
{
    data->UpdateGObject(1, new VPointF(0, 0, "A1", 0, 0));
    data->UpdateGObject(2, new VPointF(200, 0, "A2", 0, 0));
    data->UpdateGObject(3, new VPointF(400, 0, "A3", 0, 0));
    data->UpdateGObject(4, new VPointF(400, 400, "A4", 0, 0));
    data->UpdateGObject(5, new VPointF(0, 400, "A5", 0, 0));

    VPieceNode notch_node(2, Tool::NodePoint);
    notch_node.setNotch(true);
    notch_node.setNotchType(NotchType::Slit);
    notch_node.setNotchSubType(NotchSubType::Straightforward);
    notch_node.setNotchLength(5);
    notch_node.setNotchWidth(5);
    notch_node.setNotchCount(1);
    notch_node.setShowNotch(true);
    notch_node.setShowSeamlineNotch(false);

    VPieceNode first(1, Tool::NodePoint);
    VPieceNode last(5, Tool::NodePoint);
    first.setFoldLine(on_fold);
    last.setFoldLine(on_fold);

    VPiece piece;
    piece.SetName(QStringLiteral("Front"));
    piece.SetSeamAllowance(seam_allowance);
    piece.SetSeamAllowanceBuiltIn(built_in);
    piece.SetSAWidth(10);
    piece.GetPath().Append(first);
    piece.GetPath().Append(notch_node);
    piece.GetPath().Append(VPieceNode(3, Tool::NodePoint));
    piece.GetPath().Append(VPieceNode(4, Tool::NodePoint));
    piece.GetPath().Append(last);

    piece.GetPatternPieceData().SetLetter(QStringLiteral("A"));
    piece.GetPatternPieceData().SetQuantity(2);
    show_label(&piece.GetPatternPieceData(), QPointF(120, 100), QStringLiteral("40"), QStringLiteral("20"));
    show_label(&piece.GetPatternInfo(), QPointF(120, 220), QStringLiteral("40"), QStringLiteral("20"));

    VGrainlineData &grainline = piece.GetGrainlineGeometry();
    grainline.SetVisible(true);
    grainline.SetPos(QPointF(300, 350));
    grainline.setRotation(QStringLiteral("90"));
    grainline.setLength(QStringLiteral("60"));
    grainline.setArrowLength(QStringLiteral("5"));
    grainline.setArrowType(ArrowType::Both);
    return piece;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief placed_piece reads a pattern piece into the handoff data.
 */
HandoffPlacedPiece placed_piece(const VPiece &piece, VContainer *data, VAbstractPattern *document,
                                const QString &size, const QString &piece_set_key)
{
    const QFont font = qApp->Settings()->getLabelFont();

    HandoffPlacedPiece placed;
    placed.info          = HandoffPieceInfo::create(piece, size, piece_set_key);
    placed.piece         = HandoffPiece::create(piece, data);
    placed.piece_label   = HandoffLabel::create_piece_label(piece, data, placed.piece.bounds(), font);
    placed.pattern_label = HandoffLabel::create_pattern_label(piece.GetPatternInfo(), document, data,
                                                              placed.piece.bounds(), font);
    return placed;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_new_document writes the pieces with the new writer.
 */
QDomDocument make_new_document(const QVector<HandoffPlacedPiece> &pieces, const QStringList &sizes)
{
    HandoffDocument document;
    document.set_style(HandoffStyle::from_settings(*qApp->Settings()));
    document.set_pattern_name(QStringLiteral("Test Pattern"));
    document.set_units(QStringLiteral("cm"));
    document.set_sheet(QRectF(0, 0, 1000, 1000), 96);
    if (!sizes.isEmpty())
    {
        document.set_multisize(sizes, sizes.first(), QStringLiteral("1700"));
    }
    for (const HandoffPlacedPiece &placed : pieces)
    {
        document.add_piece(placed);
    }
    QDomDocument dom;
    dom.setContent(document.to_svg_string());
    return dom;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief make_old_document writes the same pieces with the old writer.
 */
QDomDocument make_old_document(const QVector<VPiece> &pieces, VContainer *data, const QStringList &sizes,
                               const QStringList &piece_sizes, const QStringList &piece_set_keys)
{
    QGraphicsRectItem paper(QRectF(0, 0, 1000, 1000));
    SvgGenerator      generator(&paper, QString(), QStringLiteral("Test Pattern"), QString(), 96);
    generator.setUnits(QStringLiteral("cm"));
    if (!sizes.isEmpty())
    {
        generator.setMultisize(sizes, sizes.first(), QStringLiteral("1700"));
    }

    QList<QGraphicsScene *> scenes;
    for (int i = 0; i < pieces.size(); ++i)
    {
        const VLayoutPiece layout_piece = VLayoutPiece::Create(pieces.at(i), data);
        QGraphicsScene    *scene        = new QGraphicsScene();
        QGraphicsItem     *item         = layout_piece.GetItem(false);
        scene->addItem(item);
        generator.addSvgFromScene(scene, item, piece_sizes.value(i), piece_set_keys.value(i));
        scenes.append(scene);
    }
    QDomDocument dom;
    dom.setContent(generator.toSvgString());
    qDeleteAll(scenes);
    return dom;
}

//---------------------------------------------------------------------------------------------------------------------
QTransform parse_matrix(const QString &text)
{
    const QRegularExpression      pattern(QStringLiteral("matrix\\(([^)]*)\\)"));
    const QRegularExpressionMatch match = pattern.match(text);
    if (!match.hasMatch())
    {
        return QTransform();
    }
    const QStringList v = match.captured(1).split(QLatin1Char(','));
    if (v.size() != 6)
    {
        return QTransform();
    }
    return QTransform(v.at(0).toDouble(), v.at(1).toDouble(), v.at(2).toDouble(), v.at(3).toDouble(),
                      v.at(4).toDouble(), v.at(5).toDouble());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief path_points reads the vertices of path data; a closepath returns to the start of its subpath.
 */
QVector<QPointF> path_points(const QString &data)
{
    const QRegularExpression token(QStringLiteral("([MLZmlz])|(-?\\d+\\.?\\d*(?:[eE][-+]?\\d+)?)"));
    QVector<QPointF>         points;
    QPointF                  start;
    QList<qreal>             numbers;
    bool                     next_is_start = false;
    auto                     it            = token.globalMatch(data);
    while (it.hasNext())
    {
        const QRegularExpressionMatch match = it.next();
        const QString                 command = match.captured(1).toUpper();
        if (!match.captured(2).isEmpty())
        {
            numbers.append(match.captured(2).toDouble());
            if (numbers.size() == 2)
            {
                points.append(QPointF(numbers.at(0), numbers.at(1)));
                if (next_is_start)
                {
                    start         = points.last();
                    next_is_start = false;
                }
                numbers.clear();
            }
        }
        else if (command == QLatin1String("M"))
        {
            numbers.clear();
            next_is_start = true;
        }
        else if (command == QLatin1String("Z"))
        {
            points.append(start);
        }
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief flatten lists the tagged groups, paths and texts of an SVG element in document order.
 * @param style inherited presentation attributes.
 */
void flatten(const QDomElement &element, const QTransform &parent, QHash<QString, QString> style,
             QVector<Entry> *out)
{
    QTransform transform = parent;
    if (element.hasAttribute(QStringLiteral("transform")))
    {
        transform = parse_matrix(element.attribute(QStringLiteral("transform"))) * parent;
    }
    static const QStringList style_names = {QStringLiteral("stroke"), QStringLiteral("fill"),
                                            QStringLiteral("stroke-linecap"), QStringLiteral("stroke-linejoin"),
                                            QStringLiteral("stroke-width")};
    for (const QString &name : style_names)
    {
        if (element.hasAttribute(name))
        {
            style.insert(name, element.attribute(name));
        }
    }

    const QString tag_name = element.tagName();
    if (tag_name == QLatin1String("svg"))
    {
        out->append({QStringLiteral("svg|%1|%2|%3")
                         .arg(element.attribute(QStringLiteral("width")), element.attribute(QStringLiteral("height")),
                              element.attribute(QStringLiteral("viewBox"))),
                     {}});
    }
    else if (tag_name == QLatin1String("g") && element.hasAttribute(QStringLiteral("data-type")))
    {
        QStringList            parts;
        const QDomNamedNodeMap attributes = element.attributes();
        for (int i = 0; i < attributes.size(); ++i)
        {
            const QDomAttr attribute = attributes.item(i).toAttr();
            if (attribute.name().startsWith(QLatin1String("data-")) || attribute.name() == QLatin1String("id"))
            {
                parts.append(attribute.name() + QLatin1Char('=') + attribute.value());
            }
        }
        parts.sort();
        out->append({QStringLiteral("group|") + parts.join(QLatin1Char('|')), {}});
    }
    else if (tag_name == QLatin1String("path"))
    {
        QVector<QPointF> points;
        for (const QPointF &point : path_points(element.attribute(QStringLiteral("d"))))
        {
            points.append(transform.map(point));
        }
        const bool stroked = style.value(QStringLiteral("stroke")) != QLatin1String("none");
        out->append({QStringLiteral("path|stroke=%1|fill=%2|cap=%3|join=%4|width=%5")
                         .arg(style.value(QStringLiteral("stroke")), style.value(QStringLiteral("fill")),
                              stroked ? style.value(QStringLiteral("stroke-linecap")) : QString(),
                              stroked ? style.value(QStringLiteral("stroke-linejoin")) : QString(),
                              stroked ? QString::number(style.value(QStringLiteral("stroke-width")).toDouble(), 'f', 2)
                                      : QString()),
                     points});
        return;
    }
    else if (tag_name == QLatin1String("text"))
    {
        out->append({QStringLiteral("text|%1|%2|%3|%4|%5")
                         .arg(element.text(), element.attribute(QStringLiteral("font-size")),
                              element.attribute(QStringLiteral("font-weight")),
                              element.attribute(QStringLiteral("font-style")),
                              element.attribute(QStringLiteral("fill"))),
                     {transform.map(QPointF(element.attribute(QStringLiteral("x")).toDouble(),
                                            element.attribute(QStringLiteral("y")).toDouble()))}});
        return;
    }

    for (QDomElement child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement())
    {
        flatten(child, transform, style, out);
    }
}

//---------------------------------------------------------------------------------------------------------------------
QVector<Entry> flatten_document(const QDomDocument &document)
{
    QVector<Entry> entries;
    flatten(document.documentElement(), QTransform(), QHash<QString, QString>(), &entries);
    return entries;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief compare_entries checks two documents for the same tags, attributes and points within 0.02 pixel.
 */
void compare_entries(const QVector<Entry> &actual, const QVector<Entry> &expected)
{
    const int count = qMin(actual.size(), expected.size());
    for (int i = 0; i < count; ++i)
    {
        QVERIFY2(actual.at(i).tag == expected.at(i).tag,
                 qPrintable(QStringLiteral("entry %1:\nnew: %2\nold: %3")
                                .arg(i).arg(actual.at(i).tag, expected.at(i).tag)));
        QVERIFY2(actual.at(i).points.size() == expected.at(i).points.size(),
                 qPrintable(QStringLiteral("entry %1 (%2): %3 points, old has %4")
                                .arg(i).arg(actual.at(i).tag).arg(actual.at(i).points.size())
                                .arg(expected.at(i).points.size())));
        for (int j = 0; j < actual.at(i).points.size(); ++j)
        {
            const QLineF distance(actual.at(i).points.at(j), expected.at(i).points.at(j));
            QVERIFY2(distance.length() < 0.02,
                     qPrintable(QStringLiteral("entry %1 (%2) point %3: (%4, %5) is not (%6, %7)")
                                    .arg(i).arg(actual.at(i).tag).arg(j)
                                    .arg(actual.at(i).points.at(j).x()).arg(actual.at(i).points.at(j).y())
                                    .arg(expected.at(i).points.at(j).x()).arg(expected.at(i).points.at(j).y())));
        }
    }
    QCOMPARE(actual.size(), expected.size());
}

//---------------------------------------------------------------------------------------------------------------------
int count_type(const QVector<Entry> &entries, const QString &type)
{
    int count = 0;
    for (const Entry &entry : entries)
    {
        if (entry.tag.startsWith(QLatin1String("group|")) &&
            entry.tag.contains(QStringLiteral("|data-type=") + type + QLatin1Char('|')))
        {
            ++count;
        }
    }
    return count;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_HandoffDocument::TST_HandoffDocument(QObject *parent)
    : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffDocument::pieceMatchesOldWriter_data() const
{
    QTest::addColumn<bool>("seam_allowance");
    QTest::addColumn<bool>("built_in");
    QTest::addColumn<bool>("on_fold");

    QTest::newRow("cut line")             << true  << false << false;
    QTest::newRow("cut line on the fold") << true  << false << true;
    QTest::newRow("built in")             << true  << true  << false;
    QTest::newRow("no seam allowance")    << false << false << false;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The new document has the old tagged tree: ids, data-* attributes, styles and points.
 */
void TST_HandoffDocument::pieceMatchesOldWriter() const
{
    QFETCH(bool, seam_allowance);
    QFETCH(bool, built_in);
    QFETCH(bool, on_fold);

    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    TestPatternDocument document;
    VAbstractPattern   *previous = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    VPiece piece = make_piece(data.data(), seam_allowance, built_in, on_fold);
    set_label_templates(&document, &piece);

    const QDomDocument          old_dom = make_old_document({piece}, data.data(), {}, {}, {});
    QVector<HandoffPlacedPiece> pieces;
    pieces.append(placed_piece(piece, data.data(), &document, QString(), QString()));
    qApp->setCurrentDocument(previous);
    const QDomDocument new_dom = make_new_document(pieces, {});

    const QVector<Entry> expected = flatten_document(old_dom);
    const QVector<Entry> actual   = flatten_document(new_dom);
    QCOMPARE(count_type(expected, QStringLiteral("piece")), 1);
    QCOMPARE(count_type(actual, QStringLiteral("seamline")), 1);
    QCOMPARE(count_type(actual, QStringLiteral("fold_line")), on_fold ? 1 : 0);
    QCOMPARE(count_type(actual, QStringLiteral("piece_label")), 1);
    QCOMPARE(count_type(actual, QStringLiteral("pattern_label")), 1);
    QCOMPARE(count_type(actual, QStringLiteral("grainline")), 1);
    compare_entries(actual, expected);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Two sizes of one piece and a piece of a second material have the old pattern groups and piece sets.
 */
void TST_HandoffDocument::multisizePiecesMatchOldWriter() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    TestPatternDocument document;
    VAbstractPattern   *previous = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    VPiece piece = make_piece(data.data(), true, false, false);
    set_label_templates(&document, &piece);
    piece.setMaterial(QStringLiteral("Fabric"));
    VPiece lining = piece;
    lining.SetName(QStringLiteral("Front Lining"));
    lining.setMaterial(QStringLiteral("Lining"));

    const QVector<VPiece> old_pieces  = {piece, piece, lining};
    const QStringList     sizes       = {QStringLiteral("34"), QStringLiteral("36")};
    const QStringList     piece_sizes = {QStringLiteral("34"), QStringLiteral("36"), QStringLiteral("34")};
    const QStringList     keys        = {QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("2")};
    const QDomDocument    old_dom     = make_old_document(old_pieces, data.data(), sizes, piece_sizes, keys);

    QVector<HandoffPlacedPiece> pieces;
    for (int i = 0; i < old_pieces.size(); ++i)
    {
        pieces.append(placed_piece(old_pieces.at(i), data.data(), &document, piece_sizes.at(i), keys.at(i)));
    }
    qApp->setCurrentDocument(previous);
    const QDomDocument new_dom = make_new_document(pieces, sizes);

    const QVector<Entry> expected = flatten_document(old_dom);
    const QVector<Entry> actual   = flatten_document(new_dom);
    QCOMPARE(count_type(actual, QStringLiteral("pattern")), 2);
    QCOMPARE(count_type(actual, QStringLiteral("piece-set")), 2);
    QCOMPARE(count_type(actual, QStringLiteral("piece")), 3);
    compare_entries(actual, expected);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Hidden labels, hidden grainlines and the show settings leave out the drawn groups; the grain angle stays.
 */
void TST_HandoffDocument::hiddenItemsAreNotDrawn() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    TestPatternDocument document;
    VAbstractPattern   *previous = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    VPiece piece = make_piece(data.data(), true, false, false);
    set_label_templates(&document, &piece);
    const HandoffPlacedPiece shown = placed_piece(piece, data.data(), &document, QString(), QString());

    piece.GetPatternPieceData().SetVisible(false);
    piece.GetPatternInfo().SetVisible(false);
    piece.GetGrainlineGeometry().SetVisible(false);
    const HandoffPlacedPiece hidden = placed_piece(piece, data.data(), &document, QString(), QString());
    qApp->setCurrentDocument(previous);

    HandoffStyle style         = HandoffStyle::from_settings(*qApp->Settings());
    style.show_labels          = true;
    style.show_grainlines      = true;
    style.show_seam_allowances = true;

    HandoffDocument document_hidden;
    document_hidden.set_style(style);
    document_hidden.set_sheet(QRectF(0, 0, 1000, 1000), 96);
    document_hidden.add_piece(hidden);
    QDomDocument dom;
    dom.setContent(document_hidden.to_svg_string());
    const QVector<Entry> hidden_entries = flatten_document(dom);
    QCOMPARE(count_type(hidden_entries, QStringLiteral("piece_label")), 0);
    QCOMPARE(count_type(hidden_entries, QStringLiteral("pattern_label")), 0);
    QCOMPARE(count_type(hidden_entries, QStringLiteral("grainline")), 0);
    QCOMPARE(count_type(hidden_entries, QStringLiteral("cutline")), 1);
    QVERIFY(dom.toString().contains(QStringLiteral("data-grainline-angle=")));

    style.show_labels          = false;
    style.show_grainlines      = false;
    style.show_seam_allowances = false;
    HandoffDocument document_off;
    document_off.set_style(style);
    document_off.set_sheet(QRectF(0, 0, 1000, 1000), 96);
    document_off.add_piece(shown);
    dom.setContent(document_off.to_svg_string());
    const QVector<Entry> off_entries = flatten_document(dom);
    QCOMPARE(count_type(off_entries, QStringLiteral("piece_label")), 0);
    QCOMPARE(count_type(off_entries, QStringLiteral("grainline")), 0);
    QCOMPARE(count_type(off_entries, QStringLiteral("cutline")), 0);
    QCOMPARE(count_type(off_entries, QStringLiteral("seamline")), 1);
    QVERIFY(dom.toString().contains(QStringLiteral("data-grainline-angle=")));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A line wider than the label is elided in the middle; a right aligned line starts right of a left one.
 */
void TST_HandoffDocument::labelLinesElideAndAlign() const
{
    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    TestPatternDocument document;
    VAbstractPattern   *previous = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    VPiece                      piece = make_piece(data.data(), true, false, false);
    QVector<VLabelTemplateLine> lines;
    lines << label_line(QStringLiteral("Left"), false, Qt::AlignLeft, 0)
          << label_line(QStringLiteral("Right"), false, Qt::AlignRight, 0)
          << label_line(QStringLiteral("An extremely long label line that cannot fit the label width, "
                                  "not even in a small font: it repeats itself to be sure of it"), false,
                        Qt::AlignLeft, 0);
    piece.GetPatternPieceData().SetLabelTemplate(lines);
    const HandoffPlacedPiece placed = placed_piece(piece, data.data(), &document, QString(), QString());
    qApp->setCurrentDocument(previous);

    HandoffDocument writer;
    writer.set_style(HandoffStyle::from_settings(*qApp->Settings()));
    writer.set_sheet(QRectF(0, 0, 1000, 1000), 96);
    writer.add_piece(placed);
    QDomDocument dom;
    dom.setContent(writer.to_svg_string());
    const QVector<Entry> entries = flatten_document(dom);

    QVector<Entry> texts;
    for (const Entry &entry : entries)
    {
        if (entry.tag.startsWith(QLatin1String("text|")) && !entry.tag.startsWith(QLatin1String("text|Foldline")))
        {
            texts.append(entry);
        }
    }
    QVERIFY(texts.size() >= 3);
    QCOMPARE(texts.at(0).tag.section(QLatin1Char('|'), 1, 1), QStringLiteral("Left"));
    QCOMPARE(texts.at(1).tag.section(QLatin1Char('|'), 1, 1), QStringLiteral("Right"));
    QVERIFY(texts.at(1).points.first().x() > texts.at(0).points.first().x() + 1);
    QVERIFY2(texts.at(2).tag.section(QLatin1Char('|'), 1, 1).contains(QChar(0x2026)),
             qPrintable(texts.at(2).tag + QLatin1String(" font ") + qApp->Settings()->getLabelFont().toString()));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_HandoffDocument::sheetMatchesOldScene_data() const
{
    pieceMatchesOldWriter_data();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The sheet rectangle and the piece offsets equal what the scene of the old writer gives.
 * @details Two pieces at fractional positions, one left of and above the other, as MainWindowsNoGUI places them.
 */
void TST_HandoffDocument::sheetMatchesOldScene() const
{
    QFETCH(bool, seam_allowance);
    QFETCH(bool, built_in);
    QFETCH(bool, on_fold);

    const Unit unit = Unit::Mm;
    QScopedPointer<VContainer> data(new VContainer(nullptr, &unit));
    qApp->setPatternUnit(unit);
    TestPatternDocument document;
    VAbstractPattern   *previous = qApp->getCurrentDocument();
    qApp->setCurrentDocument(&document);

    VPiece first = make_piece(data.data(), seam_allowance, built_in, on_fold);
    set_label_templates(&document, &first);
    first.SetMx(37.4);
    first.SetMy(-12.7);
    VPiece second = first;
    second.SetMx(500.2);
    second.SetMy(80.6);
    const QVector<VPiece> pieces = {first, second};

    QScopedPointer<QGraphicsScene> scene(new QGraphicsScene());
    QList<QGraphicsItem *>         items;
    QVector<QRectF>                extents;
    QVector<QPointF>               positions;
    const HandoffStyle             style = HandoffStyle::from_settings(*qApp->Settings());
    for (const VPiece &piece : pieces)
    {
        const VLayoutPiece layout_piece = VLayoutPiece::Create(piece, data.data());
        QGraphicsItem     *item         = layout_piece.GetItem(false);
        item->setPos(layout_piece.GetMx(), layout_piece.GetMy());
        scene->addItem(item);
        items.append(item);

        extents.append(piece_extent(HandoffPiece::create(piece, data.data()), style));
        positions.append(QPointF(piece.GetMx(), piece.GetMy()));
    }
    qApp->setCurrentDocument(previous);

    const QRect     rect = scene->itemsBoundingRect().toRect();
    const QTransform shift = QTransform().translate(-rect.x(), -rect.y());
    for (QGraphicsItem *item : std::as_const(items))
    {
        item->setTransform(shift);
    }
    const QRect old_sheet = scene->itemsBoundingRect().toRect();

    const FlatSheet flat = arrange_flat(extents, positions);
    QVERIFY(!old_sheet.isEmpty());
    QCOMPARE(flat.sheet, old_sheet);
    QCOMPARE(flat.offsets.size(), 2);
    for (int i = 0; i < 2; ++i)
    {
        const QPointF old_offset = items.at(i)->pos() + QPointF(-rect.x(), -rect.y());
        QCOMPARE(flat.offsets.at(i), old_offset);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief A sheet of pieces with no drawn line is empty, and its offsets keep the positions.
 */
void TST_HandoffDocument::emptySheetKeepsPositions() const
{
    const FlatSheet flat = arrange_flat({QRectF()}, {QPointF(3, 4)});
    QVERIFY(flat.sheet.isNull());
    QCOMPARE(flat.offsets, QVector<QPointF>({QPointF(3, 4)}));
}
