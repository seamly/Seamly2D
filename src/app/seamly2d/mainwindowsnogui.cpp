/***************************************************************************
 **  @file   mainwindowsnogui.cpp
 **  @author Douglas S Caskey
 **  @date   17 Sep, 2023
 **
 **  @copyright
 **  Copyright (C) 2017 - 2022 Seamly, LLC
 **  https://github.com/fashionfreedom/seamly2d
 **
 **  @brief
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
 **  along with Seamly2D. if not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

/************************************************************************
 **
 **  @file   mainwindowsnogui.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 5, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
 **  <https://bitbucket.org/dismine/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "mainwindowsnogui.h"
#include "core/application_2d.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "dialogs/export_layout_dialog.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/measurements_def.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vtools/tools/pattern_piece_tool.h"
#include "../../libs/vformat/svg_generator.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QToolButton>
#include <QtSvg>
#include <QPrinter>
#include <QImageWriter>
#include <QLoggingCategory>

#include <algorithm>

Q_LOGGING_CATEGORY(vMainNoGUIWindow, "v.mainnoguiwindow")

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief sizeRuns lists one value per size and joins consecutive sizes with the same value: "220-520: 9; 540-720: 10".
 * @param sizes  size names, in size order.
 * @param values one value per size, parallel to @p sizes.
 */
QString sizeRuns(const QStringList &sizes, const QStringList &values)
{
    QStringList runs;
    int first = 0;
    for (int i = 1; i <= sizes.size(); ++i)
    {
        if (i < sizes.size() && values.at(i) == values.at(first))
        {
            continue;
        }
        const QString range = (i - 1 == first) ? sizes.at(first)
                                               : sizes.at(first) + QLatin1Char('-') + sizes.at(i - 1);
        runs.append(range + QStringLiteral(": ") + values.at(first));
        first = i;
    }
    return runs.join(QStringLiteral("; "));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief cornerList joins seam allowance corner numbers: "1, 4".
 */
QString cornerList(const QVector<int> &corners)
{
    QStringList numbers;
    for (const int corner : corners)
    {
        numbers.append(QString::number(corner));
    }
    return numbers.join(QStringLiteral(", "));
}
}

//---------------------------------------------------------------------------------------------------------------------
MainWindowsNoGUI::MainWindowsNoGUI(QWidget *parent)
    : VAbstractMainWindow(parent),
      pieceList(),
      currentScene(nullptr),
      layout_scene(nullptr),
      pattern(new VContainer(qApp->translateVariables(), qApp->patternUnitP())),
      doc(nullptr),
      undoAction(nullptr),
      redoAction(nullptr),
      actionDockWidgetToolOptions(nullptr),
      actionDockWidgetGroups(nullptr),
      actionDockWidgetPieces(nullptr),
      actionDockWidgetToolbox(nullptr),
      isNoScaling(false)

{
    InitTempLayoutScene();
}

//---------------------------------------------------------------------------------------------------------------------
MainWindowsNoGUI::~MainWindowsNoGUI()
{
    delete layout_scene;
    delete pattern;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief arrangePieceItemsFlat creates the graphics items of all pieces and arranges them on one flat sheet.
 *
 * Each piece keeps its own position (Mx/My offsets); afterwards every item is
 * translated so the combined bounding rectangle starts at the scene origin.
 * The items are added to the given scene so its itemsBoundingRect() describes
 * the flat "paper" holding all pieces.
 *
 * @param scene       scene the items are added to (takes ownership).
 * @param pieceList   pieces to create items for.
 * @param textAsPaths true to convert label text into vector paths.
 * @return list of the created piece root items, in pieceList order.
 */
QList<QGraphicsItem *> MainWindowsNoGUI::arrangePieceItemsFlat(QGraphicsScene *scene,
                                                               const QVector<VLayoutPiece> &pieceList,
                                                               bool textAsPaths) const
{
    SCASSERT(scene != nullptr)
    QList<QGraphicsItem *> list;
    for (int i=0; i < pieceList.count(); ++i)
    {
        QGraphicsItem *item = pieceList.at(i).GetItem(textAsPaths);
        item->setPos(pieceList.at(i).GetMx(), pieceList.at(i).GetMy());
        list.append(item);
    }

    for (int i=0; i < list.size(); ++i)
    {
        scene->addItem(list.at(i));
    }

    // Translate every piece so the flat sheet starts at the origin.
    const QRect rect = scene->itemsBoundingRect().toRect();

    QTransform transform;
    transform = transform.translate(-rect.x(), -rect.y());

    for (int i=0; i < list.size(); ++i)
    {
        list.at(i)->setTransform(transform);
    }

    return list;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief generatePiecesSvgDocument builds the tagged pieces SVG consumed by SeamlyLayout.
 *
 * Renders every piece in pieceList (prepared by preparePiecesForLayout()) into
 * one SVG document where the pattern, each piece and each piece component carry
 * the SVG data-* attributes (data-type, data-type-number, data-parent,
 * data-name, data-letter). This is the automatic handoff created when the user
 * enters Layout Mode; no export dialog is involved. Text is kept as real text so
 * the layout application can still read the label contents.
 *
 * The document is returned as a string and never written to disk: Layout Mode
 * sends it to SeamlyLayout on that process's standard input (Seamly2D.5).
 *
 * @return the complete SVG document, or an empty string when there are no
 *         pieces to hand over.
 */
QString MainWindowsNoGUI::generatePiecesSvgDocument()
{
    if (!sizePieceLists.isEmpty())
    {
        return generateMultisizePiecesSvgDocument();
    }

    if (pieceList.isEmpty())
    {
        return QString();
    }

    // Arrange all pieces on one flat sheet, exactly like the manual pieces export.
    QScopedPointer<QGraphicsScene> scene(new QGraphicsScene());
    QList<QGraphicsItem *> list = arrangePieceItemsFlat(scene.data(), pieceList, false);

    // The paper rectangle only defines the SVG size and view box; it is never shown.
    QScopedPointer<QGraphicsRectItem> paper(new QGraphicsRectItem(scene->itemsBoundingRect().toRect()));

    return buildPiecesSvgString(paper.data(), list);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief generateMultisizePiecesSvgDocument builds the multisize pieces SVG consumed by SeamlyLayout.
 *
 * Every size in sizePieceLists is rendered. Each piece gets data-size, and the
 * same piece in every size is nested in one data-type="piece-set" group.
 *
 * @return the complete SVG document, or an empty string when there are no pieces.
 */
QString MainWindowsNoGUI::generateMultisizePiecesSvgDocument()
{
    // Flatten every size into one list so all sizes share one flat sheet and one origin.
    QVector<VLayoutPiece> allPieces;
    QStringList pieceSizes;
    QStringList pieceSetKeys;
    QStringList sizes;
    for (const SizePieceList &sizeList : std::as_const(sizePieceLists))
    {
        sizes.append(sizeList.size);
        for (int i = 0; i < sizeList.pieces.size(); ++i)
        {
            allPieces.append(sizeList.pieces.at(i));
            pieceSizes.append(sizeList.size);
            pieceSetKeys.append(QString::number(sizeList.pieceIds.at(i)));
        }
    }

    if (allPieces.isEmpty())
    {
        return QString();
    }

    QScopedPointer<QGraphicsScene> scene(new QGraphicsScene());
    const QList<QGraphicsItem *> list = arrangePieceItemsFlat(scene.data(), allPieces, false);

    // The paper rectangle only defines the SVG size and view box; it is never shown.
    QScopedPointer<QGraphicsRectItem> paper(new QGraphicsRectItem(scene->itemsBoundingRect().toRect()));

    SvgGenerator svgGenerator(paper.data(), QString(), doc->GetPatternName(), doc->GetDescription(),
                              static_cast<int>(PrintDPI));
    QStringList exportSizes;
    for (const QString &size : std::as_const(sizes))
    {
        exportSizes.append(sizeInExportUnit(size));
    }
    for (QString &size : pieceSizes)
    {
        size = sizeInExportUnit(size);
    }
    svgGenerator.setMultisize(exportSizes, sizeInExportUnit(baseSize), sizeInExportUnit(layoutHeight));
    svgGenerator.setUnits(patternUnitName());

    for (int piece = 0; piece < list.size(); ++piece)
    {
        QGraphicsScene *pieceScene = new VMainGraphicsScene();
        pieceScene->addItem(list.at(piece));
        svgGenerator.addSvgFromScene(pieceScene, list.at(piece), pieceSizes.at(piece), pieceSetKeys.at(piece));
    }

    return svgGenerator.toSvgString();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief refreshLabels call to recalculate piece labels. For example after changing a font.
 */
void MainWindowsNoGUI::refreshLabels()
{
    const QHash<quint32, VPiece> *list = pattern->DataPieces();
    QHash<quint32, VPiece>::const_iterator i = list->constBegin();
    while (i != list->constEnd())
    {
        if (PatternPieceTool *tool = qobject_cast<PatternPieceTool*>(VAbstractPattern::getTool(i.key())))
        {
            tool->updatePatternLabel();
            tool->updatePieceLabel();
        }
        ++i;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief refreshGrainLines call to recalculate piece grainlines. For example after show / hide grainlines.
 */
void MainWindowsNoGUI::refreshGrainLines()
{
    const QHash<quint32, VPiece> *list = pattern->DataPieces();
    QHash<quint32, VPiece>::const_iterator i = list->constBegin();
    while (i != list->constEnd())
    {
        if (PatternPieceTool *tool = qobject_cast<PatternPieceTool*>(VAbstractPattern::getTool(i.key())))
        {
            tool->updateGrainline();
        }
        ++i;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief refreshSeamAllowances call to show / hide seam allowances.
 */
void MainWindowsNoGUI::refreshSeamAllowances()
{
    const QHash<quint32, VPiece> *list = pattern->DataPieces();
    QHash<quint32, VPiece>::const_iterator i = list->constBegin();
    while (i != list->constEnd())
    {
        if (PatternPieceTool *tool = qobject_cast<PatternPieceTool*>(VAbstractPattern::getTool(i.key())))
        {
            tool->RefreshGeometry();
        }
        ++i;
    }
}

//---------------------------------------------------------------------------------------------------------------------
QVector<VLayoutPiece> MainWindowsNoGUI::preparePiecesForLayout(const QHash<quint32, VPiece> &pieces)
{
    QVector<VLayoutPiece> pieceList;
    if (!pieces.isEmpty())
    {
        QHash<quint32, VPiece>::const_iterator i = pieces.constBegin();
        while (i != pieces.constEnd())
        {
            VAbstractTool *tool = qobject_cast<VAbstractTool*>(VAbstractPattern::getTool(i.key()));
            SCASSERT(tool != nullptr)
            pieceList.append(VLayoutPiece::Create(i.value(), tool->getData()));
            ++i;
        }
    }

    return pieceList;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief prepareSizePieceList creates the layout pieces of one multisize size.
 *
 * The pattern must already be recalculated at that size. Pieces are ordered by
 * VPiece id, so every size lists the same pieces in the same order.
 *
 * @param pieces pieces included in the layout, keyed by VPiece id.
 * @param size   size in pattern units.
 * @param cornerCuts seam allowance corners to cut in every size, per VPiece id. See unevenCornerCuts().
 * @return the size's pieces with their VPiece ids.
 */
MainWindowsNoGUI::SizePieceList MainWindowsNoGUI::prepareSizePieceList(const QHash<quint32, VPiece> &pieces,
                                                                      const QString &size,
                                                                      const QHash<quint32, QVector<int>> &cornerCuts)
{
    SizePieceList sizeList;
    sizeList.size = size;

    QList<quint32> ids = pieces.keys();
    std::sort(ids.begin(), ids.end());
    for (const quint32 id : std::as_const(ids))
    {
        VAbstractTool *tool = qobject_cast<VAbstractTool*>(VAbstractPattern::getTool(id));
        SCASSERT(tool != nullptr)
        VPiece piece = pieces.value(id);
        piece.setForcedCornerCuts(cornerCuts.value(id));
        sizeList.pieceIds.append(id);
        sizeList.pieces.append(VLayoutPiece::Create(piece, tool->getData()));
    }

    return sizeList;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief unevenCornerCuts finds the seam allowance corners cut short in some sizes of a piece but not in others.
 *
 * A ByLength corner is cut when its miter is too long, so a sharp corner can be cut in the small sizes only.
 * Grading needs one corner shape in every size. A piece whose sizes have different corner counts is left out.
 * Each piece found, and each piece left out, is written to the log.
 *
 * @return per VPiece id, every corner cut in any size; only for pieces whose sizes differ.
 */
QHash<quint32, QVector<int>> MainWindowsNoGUI::unevenCornerCuts() const
{
    QHash<quint32, QVector<int>> uneven;
    if (sizePieceLists.size() < 2)
    {
        return uneven;
    }

    const SizePieceList &firstSize = sizePieceLists.first();
    for (int p = 0; p < firstSize.pieceIds.size(); ++p)
    {
        const quint32 id = firstSize.pieceIds.at(p);
        const QString name = firstSize.pieces.at(p).GetName();

        // The corners of the same piece in every size that has it.
        QVector<SeamAllowanceCorners> sizes;
        for (const SizePieceList &sizeList : sizePieceLists)
        {
            const int i = sizeList.pieceIds.indexOf(id);
            if (i >= 0)
            {
                sizes.append(sizeList.pieces.at(i).seamAllowanceCorners());
            }
        }

        bool sameCount = true;
        bool sameCuts = true;
        QVector<int> cuts;
        for (const SeamAllowanceCorners &corners : std::as_const(sizes))
        {
            sameCount = sameCount && corners.count == sizes.first().count;
            sameCuts = sameCuts && corners.cut == sizes.first().cut;
            cuts += corners.cut;
        }
        if (sameCuts)
        {
            continue;
        }

        std::sort(cuts.begin(), cuts.end());
        cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
        if (!sameCount)
        {
            qCInfo(vMainNoGUIWindow, "Piece \"%s\" (id %u): seam allowance corners %s are cut short in some sizes "
                   "only. The sizes differ in corner count, so the other sizes keep these corners.",
                   qUtf8Printable(name), id, qUtf8Printable(cornerList(cuts)));
            continue;
        }

        qCInfo(vMainNoGUIWindow, "Piece \"%s\" (id %u): seam allowance corners %s are cut short in some sizes "
               "only. Every size cuts them.", qUtf8Printable(name), id, qUtf8Printable(cornerList(cuts)));
        uneven.insert(id, cuts);
    }
    return uneven;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief needsCornerCuts tells whether a size has a piece whose cut corners differ from cornerCuts.
 */
bool MainWindowsNoGUI::needsCornerCuts(const SizePieceList &sizeList, const QHash<quint32, QVector<int>> &cornerCuts)
{
    for (auto it = cornerCuts.constBegin(); it != cornerCuts.constEnd(); ++it)
    {
        const int i = sizeList.pieceIds.indexOf(it.key());
        if (i >= 0 && sizeList.pieces.at(i).seamAllowanceCorners().cut != it.value())
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief unifySizeTurnPoints gives every size of a piece the node turn points of all its sizes.
 *
 * Grading in SeamlyLayout needs the same turn points in every size of a piece. See
 * VLayoutPiece::unifyTurnPoints(). A line whose node count differs between sizes keeps the turn points of each
 * size; the log lists the node counts of each such piece, and the number of such pieces.
 */
void MainWindowsNoGUI::unifySizeTurnPoints()
{
    if (sizePieceLists.size() < 2)
    {
        return;
    }

    int differing = 0;
    const QVector<quint32> ids = sizePieceLists.first().pieceIds;
    for (const quint32 id : ids)
    {
        // The same piece in every size that has it.
        QVector<VLayoutPiece *> sizes;
        QStringList sizeNames;
        QStringList nodeCounts;
        for (SizePieceList &sizeList : sizePieceLists)
        {
            const int i = sizeList.pieceIds.indexOf(id);
            if (i >= 0)
            {
                VLayoutPiece &piece = sizeList.pieces[i];
                sizes.append(&piece);
                sizeNames.append(sizeList.size);

                const QVector<int> lines = piece.lineNodeCounts();
                nodeCounts.append(QStringLiteral("%1/%2/%3/%4").arg(lines.at(0)).arg(lines.at(1)).arg(lines.at(2))
                                  .arg(piece.seamAllowanceCorners().count));
            }
        }

        if (nodeCounts.count(nodeCounts.first()) != nodeCounts.size())
        {
            ++differing;
            qCInfo(vMainNoGUIWindow, "Piece \"%s\" (id %u): node counts differ between sizes (seam line/cut line/"
                   "built-in seam line/seam allowance corners): %s", qUtf8Printable(sizes.first()->GetName()), id,
                   qUtf8Printable(sizeRuns(sizeNames, nodeCounts)));
        }
        VLayoutPiece::unifyTurnPoints(sizes);
    }
    qCInfo(vMainNoGUIWindow, "Multisize layout: %d sizes; %d of %d pieces differ in node count between sizes.",
           static_cast<int>(sizePieceLists.size()), differing, static_cast<int>(ids.size()));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::InitTempLayoutScene()
{
    layout_scene = new VMainGraphicsScene();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportSVG save layout to svg file.
 * @param fileName name layout file.
 */
void MainWindowsNoGUI::exportSVG(const QString &name, QGraphicsRectItem *paper, QGraphicsScene *scene) const
{
    SvgGenerator svgGenerator(paper, name, doc->GetPatternName(), doc->GetDescription(), static_cast<int>(PrintDPI));
    svgGenerator.addSvgFromScene(scene);
    svgGenerator.generate();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief buildPiecesSvgString renders piece items into one tagged SVG document in memory.
 *
 * Each piece is rendered in its own scene, then merged and tagged. The document
 * is returned, not written, so Layout Mode can hand the pieces to SeamlyLayout
 * without creating a file.
 *
 * @param paper  paper rectangle defining the SVG size.
 * @param pieces piece root items to render; each is rendered in its own scene.
 * @return the complete SVG document as text.
 */
QString MainWindowsNoGUI::buildPiecesSvgString(QGraphicsRectItem *paper,
                                               const QList<QGraphicsItem *> &pieces) const
{
    // The generator takes an output path in its constructor; this caller never
    // writes a file, so the name is only carried, never used.
    SvgGenerator svgGenerator(paper, QString(), doc->GetPatternName(), doc->GetDescription(),
                              static_cast<int>(PrintDPI));
    svgGenerator.setSampleSize(sizeInExportUnit(individualSampleSize()));
    svgGenerator.setUnits(patternUnitName());

    for (int piece = 0; piece < pieces.size(); piece++)
    {
        QGraphicsScene *scene = new VMainGraphicsScene();
        scene->addItem(pieces.at(piece));
        svgGenerator.addSvgFromScene(scene, pieces.at(piece));
    }

    return svgGenerator.toSvgString();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Default: no measurement file is known.
 */
bool MainWindowsNoGUI::measurementFileUnit(Unit &unit) const
{
    Q_UNUSED(unit)
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Unit that decides the DXF file units.
 *
 * The unit of the referenced measurement file wins. A pattern without a measurement file uses the pattern unit.
 *
 * @return Mm, Cm or Inch; Unit::Px for any other unit.
 */
Unit MainWindowsNoGUI::exportUnit() const
{
    Unit unit = qApp->patternUnit();
    measurementFileUnit(unit);
    if (unit != Unit::Mm && unit != Unit::Cm && unit != Unit::Inch)
    {
        return Unit::Px;
    }
    return unit;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Convert a size value from pattern units to the export unit.
 *
 * @param size size value in pattern units; empty stays empty.
 * @return the value in the export unit; the input when the export unit is not handed off.
 */
QString MainWindowsNoGUI::sizeInExportUnit(const QString &size) const
{
    const Unit unit = exportUnit();
    bool ok = false;
    const qreal value = size.toDouble(&ok);
    if (!ok || unit == Unit::Px)
    {
        return size;
    }
    return QString::number(UnitConvertor(value, qApp->patternUnit(), unit));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Export unit name for the SeamlyLayout handoff.
 *
 * @return "mm", "cm" or "inch"; empty for any other unit.
 */
QString MainWindowsNoGUI::patternUnitName() const
{
    const Unit unit = exportUnit();
    if (unit == Unit::Px)
    {
        return QString();
    }
    return UnitsToStr(unit);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Size of an individual pattern for the SeamlyLayout handoff.
 *
 * DXF-ASTM `Size Name:` needs one value. The measurement bust_circ is used, else waist_circ.
 *
 * @return the measurement value in pattern units; empty when the pattern has neither measurement.
 */
QString MainWindowsNoGUI::individualSampleSize() const
{
    const QMap<QString, QSharedPointer<MeasurementVariable>> measurements = pattern->DataMeasurements();
    for (const QString &name : {bustCirc_M, waistCirc_M})
    {
        // A const pointer selects the qreal GetValue() overload, not the qreal* one.
        const QSharedPointer<const MeasurementVariable> measurement = measurements.value(name);
        if (!measurement.isNull())
        {
            return QString::number(measurement->GetValue());
        }
    }
    return QString();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportPNG save layout to png file.
 * @param fileName name layout file.
 */
void MainWindowsNoGUI::exportPNG(const QString &fileName,  QGraphicsScene *scene) const
{
    QImage image(scene->sceneRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);                                              // Start all pixels transparent
    QPainter painter(&image);
    painter.setFont(qApp->Seamly2DSettings()->getLabelFont());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush (QBrush (Qt::NoBrush));
    scene->render(&painter);
    image.save(fileName, "PNG", qApp->Seamly2DSettings()->getExportQuality());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportTIF save layout to tif file.
 * @param fileName name layout file.
 */
void MainWindowsNoGUI::exportTIF(const QString &fileName,  QGraphicsScene *scene) const
{
    QImage image(scene->sceneRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);                                              // Start all pixels transparent
    QPainter painter(&image);
    painter.setFont(qApp->Seamly2DSettings()->getLabelFont());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush (QBrush (Qt::NoBrush));
    scene->render(&painter);

    QImageWriter writer;
    writer.setFormat("TIF");
    writer.setCompression(1);
    writer.setFileName(fileName);
    writer.write(image);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportJPG save layout to jpg file.
 * @param fileName name layout file.
 */
void MainWindowsNoGUI::exportJPG(const QString &fileName,  QGraphicsScene *scene) const
{
    QImage image(scene->sceneRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::white);                                              // Start all pixels transparent
    QPainter painter(&image);
    painter.setFont(qApp->Seamly2DSettings()->getLabelFont());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush (QBrush (Qt::NoBrush));
    scene->render(&painter);
    image.save(fileName, "JPG", qApp->Seamly2DSettings()->getExportQuality());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportBMP save layout to bmp file.
 * @param fileName name layout file.
 */
void MainWindowsNoGUI::exportBMP(const QString &fileName,  QGraphicsScene *scene) const
{
    QImage image(scene->sceneRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::white);                                              // Start all pixels transparent
    QPainter painter(&image);
    painter.setFont(qApp->Seamly2DSettings()->getLabelFont());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush (QBrush (Qt::NoBrush));
    scene->render(&painter);
    image.save(fileName, "BMP", qApp->Seamly2DSettings()->getExportQuality());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportPPM save layout to gif file.
 * @param fileName name layout file.
 */
void MainWindowsNoGUI::exportPPM(const QString &fileName,  QGraphicsScene *scene) const
{
    QImage image(scene->sceneRect().size().toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);                                              // Start all pixels transparent
    QPainter painter(&image);
    painter.setFont(qApp->Seamly2DSettings()->getLabelFont());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush (QBrush (Qt::NoBrush));
    scene->render(&painter);
    image.save(fileName, "PPM", qApp->Seamly2DSettings()->getExportQuality());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief exportPDF save layout to pdf file.
 * @param fileName name layout file.
 */
void MainWindowsNoGUI::exportPDF(const QString &name, QGraphicsRectItem *paper, QGraphicsScene *scene,
                               bool ignoreMargins, const QMarginsF &margins) const
{
    QPrinter printer;
    printer.setCreator(QGuiApplication::applicationDisplayName()+QLatin1String(" ")+
                       QCoreApplication::applicationVersion());
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(name);
    printer.setDocName(FileName());
    printer.setResolution(static_cast<int>(PrintDPI));
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setFullPage(ignoreMargins);

    const QRectF sourceRect = paper->rect();
    const QRectF targetRect = QRectF(QPoint(0,0), QPoint(sourceRect.width(),sourceRect.height()));
    QSizeF size(FromPixel(sourceRect.width() + margins.left() + margins.right(), Unit::Mm),
                FromPixel(sourceRect.height() + margins.top() + margins.bottom(), Unit::Mm));
    QPageSize pageSize(size, QPageSize::Unit::Millimeter);
    printer.setPageSize(pageSize);

    if (!ignoreMargins)
    {
        const qreal left = FromPixel(margins.left(), Unit::Mm);
        const qreal top = FromPixel(margins.top(), Unit::Mm);
        const qreal right = FromPixel(margins.right(), Unit::Mm);
        const qreal bottom = FromPixel(margins.bottom(), Unit::Mm);

        const bool success = printer.setPageMargins(QMarginsF(left, top, right, bottom), QPageLayout::Millimeter);
        if (!success)
        {
            qWarning() << tr("Cannot set printer margins");
        }
    }

    QFont appFont;
    appFont.setPointSize(8);

    QPainter painter;
    if (painter.begin(&printer) == false)
    {
        qCritical("%s", qUtf8Printable(tr("Can't open printer %1").arg(name))); // failed to open file
        return;
    }
    painter.setFont(appFont);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(Qt::black, widthMainLine, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QBrush(Qt::NoBrush));
    scene->render(&painter, targetRect, sourceRect, Qt::KeepAspectRatio);
    painter.end();
}

//---------------------------------------------------------------------------------------------------------------------
QString MainWindowsNoGUI::FileName() const
{
    QString fileName;
    qApp->getFilePath().isEmpty() ? fileName = tr("unnamed") : fileName = qApp->getFilePath();
    return QFileInfo(fileName).baseName();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::setSizeHeightForIndividualM() const
{
    const QHash<QString, QSharedPointer<VInternalVariable> > * vars = pattern->DataVariables();

    if (vars->contains(size_M))
    {
        VContainer::setSize(*vars->value(size_M)->GetValue());
    }
    else
    {
        VContainer::setSize(0);
    }

    if (vars->contains(height_M))
    {
        VContainer::setHeight(*vars->value(height_M)->GetValue());
    }
    else
    {
        VContainer::setHeight(0);
    }

    doc->SetPatternWasChanged(true);
    emit doc->updatePatternLabel();
}
