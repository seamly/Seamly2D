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
#include "../vobj/vobjpaintdevice.h"
#include "../vdxf/vdxfpaintdevice.h"
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

bool CreateLayoutPath(const QString &path)
{
    bool usedNotExistedDir = true;
    QDir dir(path);
    dir.setPath(path);
    if (!dir.exists(path))
    {
        usedNotExistedDir = dir.mkpath(".");
    }
    return usedNotExistedDir;
}

void RemoveLayoutPath(const QString &path, bool usedNotExistedDir)
{
    if (usedNotExistedDir)
    {
        QDir dir(path);
        dir.rmpath(".");
    }
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
void MainWindowsNoGUI::ExportData(const QVector<VLayoutPiece> &pieceList, const ExportLayoutDialog &dialog)
{
    const LayoutExportFormat format = dialog.format();

    if (format == LayoutExportFormat::DXF_AC1006_AAMA ||
        format == LayoutExportFormat::DXF_AC1009_AAMA ||
        format == LayoutExportFormat::DXF_AC1012_AAMA ||
        format == LayoutExportFormat::DXF_AC1014_AAMA ||
        format == LayoutExportFormat::DXF_AC1015_AAMA ||
        format == LayoutExportFormat::DXF_AC1018_AAMA ||
        format == LayoutExportFormat::DXF_AC1021_AAMA ||
        format == LayoutExportFormat::DXF_AC1024_AAMA ||
        format == LayoutExportFormat::DXF_AC1027_AAMA)
    {
        exportPiecesAsApparelLayout(dialog, pieceList);
    }
    else
    {
        exportPiecesAsFlatLayout(dialog, pieceList);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ExportFlatLayout(const ExportLayoutDialog &dialog, const QList<QGraphicsScene *> &scenes,
                                        const QList<QGraphicsItem *> &papers, const QList<QGraphicsItem *> &shadows,
                                        const QList<QList<QGraphicsItem *> > &pieces, bool ignoreMargins,
                                        const QMarginsF &margins)
{
    const QString path = dialog.path();
    bool usedNotExistedDir = CreateLayoutPath(path);
    if (!usedNotExistedDir)
    {
        qCritical() << tr("Can't create a path");
        return;
    }

    qApp->Seamly2DSettings()->SetPathLayout(path);
    ExportScene(dialog, scenes, papers, shadows, pieces, ignoreMargins, margins);

    RemoveLayoutPath(path, usedNotExistedDir);
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
 * @brief exportPiecesAsFlatLayout exports the pieces from piece mode onto one flat sheet (non-AAMA formats).
 * @param dialog    export dialog holding format, path and text options.
 * @param pieceList pieces to export.
 */
void MainWindowsNoGUI::exportPiecesAsFlatLayout(const ExportLayoutDialog &dialog,
                                                 const QVector<VLayoutPiece> &pieceList)
{
    if (pieceList.isEmpty())
    {
        return;
    }

    QScopedPointer<QGraphicsScene> scene(new QGraphicsScene());
    QList<QGraphicsItem *> list = arrangePieceItemsFlat(scene.data(), pieceList, dialog.isTextAsPaths());

    QList<QGraphicsItem *> papers;// Blank sheets
    QRect rect = scene->itemsBoundingRect().toRect();

    QGraphicsRectItem *paper = new QGraphicsRectItem(rect);
    paper->setPen(QPen(Qt::black, 1));
    paper->setBrush(QBrush(Qt::white));
    papers.append(paper);

    QList<QList<QGraphicsItem *> > pieces;// All pieces
    pieces.append(list);

    QList<QGraphicsItem *> shadows = CreateShadows(papers);
    QList<QGraphicsScene *> scenes = CreateScenes(papers, shadows, pieces);

    const bool ignoreMargins = false;
    const qreal margin = ToPixel(1, Unit::Cm);
    ExportFlatLayout(dialog, scenes, papers, shadows, pieces, ignoreMargins,
                     QMarginsF(margin, margin, margin, margin));

    qDeleteAll(scenes);//Scene will clear all other items
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
    svgGenerator.setMultisize(sizes, baseSize, layoutHeight);

    for (int piece = 0; piece < list.size(); ++piece)
    {
        QGraphicsScene *pieceScene = new VMainGraphicsScene();
        pieceScene->addItem(list.at(piece));
        svgGenerator.addSvgFromScene(pieceScene, list.at(piece), pieceSizes.at(piece), pieceSetKeys.at(piece));
    }

    return svgGenerator.toSvgString();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ExportApparelLayout(const ExportLayoutDialog &dialog, const QVector<VLayoutPiece> &pieces,
                                           const QString &name, const QSize &size) const
{
    const QString path = dialog.path();
    bool usedNotExistedDir = CreateLayoutPath(path);
    if (!usedNotExistedDir)
    {
        qCritical() << tr("Can't create a path");
        return;
    }

    qApp->Seamly2DSettings()->SetPathLayout(path);
    const LayoutExportFormat format = dialog.format();

    switch (format)
    {
        case LayoutExportFormat::DXF_AC1006_ASTM:
        case LayoutExportFormat::DXF_AC1009_ASTM:
        case LayoutExportFormat::DXF_AC1012_ASTM:
        case LayoutExportFormat::DXF_AC1014_ASTM:
        case LayoutExportFormat::DXF_AC1015_ASTM:
        case LayoutExportFormat::DXF_AC1018_ASTM:
        case LayoutExportFormat::DXF_AC1021_ASTM:
        case LayoutExportFormat::DXF_AC1024_ASTM:
        case LayoutExportFormat::DXF_AC1027_ASTM:
            Q_UNREACHABLE(); // For now not supported
            break;
        case LayoutExportFormat::DXF_AC1006_AAMA:
            AAMADxfFile(name, DRW::AC1006, dialog.isBinaryDXFFormat(), size, pieces);
            break;
        case LayoutExportFormat::DXF_AC1009_AAMA:
            AAMADxfFile(name, DRW::AC1009, dialog.isBinaryDXFFormat(), size, pieces);
            break;
        case LayoutExportFormat::DXF_AC1012_AAMA:
            AAMADxfFile(name, DRW::AC1012, dialog.isBinaryDXFFormat(), size, pieces);
            break;
        case LayoutExportFormat::DXF_AC1014_AAMA:
            AAMADxfFile(name, DRW::AC1014, dialog.isBinaryDXFFormat(), size, pieces);
            break;
        case LayoutExportFormat::DXF_AC1015_AAMA:
            AAMADxfFile(name, DRW::AC1015, dialog.isBinaryDXFFormat(), size, pieces);
            break;
        case LayoutExportFormat::DXF_AC1018_AAMA:
            AAMADxfFile(name, DRW::AC1018, dialog.isBinaryDXFFormat(), size, pieces);
            break;
        case LayoutExportFormat::DXF_AC1021_AAMA:
            AAMADxfFile(name, DRW::AC1021, dialog.isBinaryDXFFormat(), size, pieces);
            break;
        case LayoutExportFormat::DXF_AC1024_AAMA:
            AAMADxfFile(name, DRW::AC1024, dialog.isBinaryDXFFormat(), size, pieces);
            break;
        case LayoutExportFormat::DXF_AC1027_AAMA:
            AAMADxfFile(name, DRW::AC1027, dialog.isBinaryDXFFormat(), size, pieces);
            break;
        default:
            qWarning() << "Can't recognize file type." << Q_FUNC_INFO;
            break;
    }

    RemoveLayoutPath(path, usedNotExistedDir);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::exportPiecesAsApparelLayout(const ExportLayoutDialog &dialog,
                                                    QVector<VLayoutPiece> pieceList)
{
    if (pieceList.isEmpty())
    {
        return;
    }

    QScopedPointer<QGraphicsScene> scene(new QGraphicsScene());

    QList<QGraphicsItem *> list;
    for (int i=0; i < pieceList.count(); ++i)
    {
        QGraphicsItem *item = pieceList.at(i).GetItem(dialog.isTextAsPaths());
        item->setPos(pieceList.at(i).GetMx(), pieceList.at(i).GetMy());
        list.append(item);
    }

    for (int i=0; i < list.size(); ++i)
    {
        scene->addItem(list.at(i));
    }

    QRect rect = scene->itemsBoundingRect().toRect();

    const int mx = rect.x();
    const int my = rect.y();

    QTransform transform;
    transform = transform.translate(-mx, -my);

    for (int i=0; i < list.size(); ++i)
    {
        list.at(i)->setTransform(transform);
    }

    rect = scene->itemsBoundingRect().toRect();

    for (int i=0; i < pieceList.count(); ++i)
    {
        QTransform moveTransform = pieceList[i].getTransform();
        moveTransform = moveTransform.translate(pieceList.at(i).GetMx(), pieceList.at(i).GetMy());
        moveTransform = moveTransform.translate(-mx, -my);
        pieceList[i].setTransform(moveTransform);
    }

    const QString name = QString("%1/%2%3")
    .arg(dialog.path())                                          //1
    .arg(dialog.fileName())                                      //2
    .arg(ExportLayoutDialog::exportFormatSuffix(dialog.format())); //3

    ExportApparelLayout(dialog, pieceList, name, rect.size());
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
QList<QGraphicsItem *> MainWindowsNoGUI::CreateShadows(const QList<QGraphicsItem *> &papers)
{
    QList<QGraphicsItem *> shadows;

    for (int i=0; i< papers.size(); ++i)
    {
        qreal x1=0, y1=0, x2=0, y2=0;
        QGraphicsRectItem *item = qgraphicsitem_cast<QGraphicsRectItem *>(papers.at(i));
        if (item)
        {
            item->rect().getCoords(&x1, &y1, &x2, &y2);
            QGraphicsRectItem *shadowPaper = new QGraphicsRectItem(QRectF(x1+4, y1+4, x2+4, y2+4));
            shadowPaper->setBrush(QBrush(Qt::black));
            shadows.append(shadowPaper);
        }
        else
        {
            shadows.append(nullptr);
        }
    }

    return shadows;
}

//---------------------------------------------------------------------------------------------------------------------
QList<QGraphicsScene *> MainWindowsNoGUI::CreateScenes(const QList<QGraphicsItem *> &papers,
                                                       const QList<QGraphicsItem *> &shadows,
                                                       const QList<QList<QGraphicsItem *> > &pieces)
{
    QList<QGraphicsScene *> scenes;
    for (int i=0; i<papers.size(); ++i)
    {
        QGraphicsScene *scene = new VMainGraphicsScene();
        QColor color = QColor(qApp->Seamly2DSettings()->getBackgroundColor());
        scene->setBackgroundBrush(color);
        scene->addItem(shadows.at(i));
        scene->addItem(papers.at(i));

        QList<QGraphicsItem *> item = pieces.at(i);
        for (int i=0; i < item.size(); ++i)
        {
            scene->addItem(item.at(i));
        }

        scenes.append(scene);
    }

    return scenes;
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

/**
 * @brief exportSVG save a list of piece items to one svg file, one tagged group per piece.
 * @param name   layout file name.
 * @param paper  paper rectangle defining the SVG size.
 * @param pieces piece root items to export; each is rendered in its own scene.
 */
void MainWindowsNoGUI::exportSVG(const QString &name, QGraphicsRectItem *paper, const QList<QGraphicsItem *> &pieces) const
{
    SvgGenerator svgGenerator(paper, name, doc->GetPatternName(), doc->GetDescription(), static_cast<int>(PrintDPI));

    for (int piece = 0; piece < pieces.size(); piece++)
    {
        QGraphicsItem *pieceItem = pieces.at(piece);
        // QGraphicsScene::addItem() reparents the item away from whatever scene it was
        // in - here that's the live Layout scene the piece is still shown in after export.
        // Remember it so we can put the piece back once its temporary render scene is done
        // with it, instead of leaving the Layout canvas with its pieces silently removed.
        QGraphicsScene *originalScene = pieceItem->scene();

        QScopedPointer<QGraphicsScene> scene(new VMainGraphicsScene());
        scene->addItem(pieceItem);
        svgGenerator.addSvgFromScene(scene.data(), pieceItem);

        scene->removeItem(pieceItem);
        if (originalScene != nullptr)
        {
            originalScene->addItem(pieceItem);
        }
    }

    svgGenerator.generate();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief buildPiecesSvgString renders piece items into one tagged SVG document in memory.
 *
 * Same merge and tagging as the exportSVG() overload above; it returns the
 * document instead of writing it, so Layout Mode can hand the pieces to
 * SeamlyLayout without creating a file (Seamly2D.5).
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
    svgGenerator.setSampleSize(individualSampleSize());

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
void MainWindowsNoGUI::ObjFile(const QString &name, QGraphicsRectItem *paper, QGraphicsScene *scene) const
{
    VObjPaintDevice generator;
    generator.setFileName(name);
    generator.setSize(paper->rect().size().toSize());
    generator.setResolution(static_cast<int>(PrintDPI));
    QPainter painter;
    painter.begin(&generator);
    scene->render(&painter, paper->rect(), paper->rect(), Qt::IgnoreAspectRatio);
    painter.end();
}

//---------------------------------------------------------------------------------------------------------------------
QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wswitch-default")

void MainWindowsNoGUI::FlatDxfFile(const QString &name, int version, bool binary, QGraphicsRectItem *paper,
                               QGraphicsScene *scene, const QList<QList<QGraphicsItem *> > &pieces) const
{
    PrepareTextForDXF(endStringPlaceholder, pieces);
    VDxfPaintDevice generator;
    generator.setFileName(name);
    generator.setSize(paper->rect().size().toSize());
    generator.setResolution(PrintDPI);
    generator.setVersion(static_cast<DRW::Version>(version));
    generator.SetBinaryFormat(binary);
    generator.setInsunits(VarInsunits::Millimeters);// Decided to always use mm. See issue #745

    QPainter painter;
    if (painter.begin(&generator))
    {
        scene->render(&painter, paper->rect(), paper->rect(), Qt::IgnoreAspectRatio);
        painter.end();
    }
    RestoreTextAfterDXF(endStringPlaceholder, pieces);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::AAMADxfFile(const QString &name, int version, bool binary, const QSize &size,
                                   const QVector<VLayoutPiece> &pieces) const
{
    VDxfPaintDevice generator;
    generator.setFileName(name);
    generator.setSize(size);
    generator.setResolution(PrintDPI);
    generator.setVersion(static_cast<DRW::Version>(version));
    generator.SetBinaryFormat(binary);
    generator.setInsunits(VarInsunits::Millimeters);// Decided to always use mm. See issue #745
    generator.ExportToAAMA(pieces);
}

QT_WARNING_POP

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief PrepareTextForDXF prepare QGraphicsSimpleTextItem items for export to flat dxf.
 *
 * Because QPaintEngine::drawTextItem doesn't pass whole string per time we mark end of each string by adding special
 * placeholder. This method append it.
 *
 * @param placeholder placeholder that will be appended to each QGraphicsSimpleTextItem item's text string.
 */
/**
 * @brief collectTextItems recursively collects every QGraphicsSimpleTextItem below an item.
 *
 * Label text items are grandchildren of the piece root (each label's lines are
 * grouped under an invisible label group item), so a recursive walk is required
 * to reach them.
 *
 * The label lines are SvgTextItem instances (vlayout), a QGraphicsSimpleTextItem
 * subclass that shares its type(); both the type check and the cast below
 * therefore match them.
 *
 * @param item      graphics item whose descendants are searched.
 * @param textItems output list the found text items are appended to.
 */
static void collectTextItems(QGraphicsItem *item, QList<QGraphicsSimpleTextItem *> &textItems)
{
    const QList<QGraphicsItem *> children = item->childItems();
    for (int i = 0; i < children.size(); ++i)
    {
        QGraphicsItem *child = children.at(i);
        if (child->type() == QGraphicsSimpleTextItem::Type)
        {
            if (QGraphicsSimpleTextItem *textItem = qgraphicsitem_cast<QGraphicsSimpleTextItem *>(child))
            {
                textItems.append(textItem);
            }
        }
        // Descend into grandchildren (e.g. label groups holding the text lines).
        collectTextItems(child, textItems);
    }
}

void MainWindowsNoGUI::PrepareTextForDXF(const QString &placeholder,
                                         const QList<QList<QGraphicsItem *> > &pieces) const
{
    for (int i = 0; i < pieces.size(); ++i)
    {
        const QList<QGraphicsItem *> &paperItems = pieces.at(i);
        for (int j = 0; j < paperItems.size(); ++j)
        {
            // Gather every text item below this piece, at any depth.
            QList<QGraphicsSimpleTextItem *> textItems;
            collectTextItems(paperItems.at(j), textItems);
            for (int k = 0; k < textItems.size(); ++k)
            {
                QGraphicsSimpleTextItem *textItem = textItems.at(k);
                // With a pen Qt draws the text as outlines, which bypasses VDxfEngine::drawTextItem
                // and leaves the placeholder in the dxf as glyphs. Without a pen it is drawn as text.
                textItem->setPen(Qt::NoPen);
                textItem->setText(textItem->text() + placeholder);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MainWindowsNoGUI::RestoreTextAfterDXF restore QGraphicsSimpleTextItem items after export to flat dxf.
 *
 * Because QPaintEngine::drawTextItem doesn't pass whole string per time we mark end of each string by adding special
 * placeholder. This method remove it.
 *
 * @param placeholder placeholder that will be removed from each QGraphicsSimpleTextItem item's text string.
 */
void MainWindowsNoGUI::RestoreTextAfterDXF(const QString &placeholder,
                                           const QList<QList<QGraphicsItem *> > &pieces) const
{
    for (int i = 0; i < pieces.size(); ++i)
    {
        const QList<QGraphicsItem *> &paperItems = pieces.at(i);
        for (int j = 0; j < paperItems.size(); ++j)
        {
            // Gather every text item below this piece, at any depth.
            QList<QGraphicsSimpleTextItem *> textItems;
            collectTextItems(paperItems.at(j), textItems);
            for (int k = 0; k < textItems.size(); ++k)
            {
                QGraphicsSimpleTextItem *textItem = textItems.at(k);
                QString text = textItem->text();
                text.replace(placeholder, "");
                textItem->setText(text);
                // No pen restore: VLayoutPiece::createLabelItem sets no pen.
                // A pen makes later SVG/PDF exports draw the text as outlines.
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ExportScene(const ExportLayoutDialog &dialog, const QList<QGraphicsScene *> &scenes,
                                   const QList<QGraphicsItem *> &papers, const QList<QGraphicsItem *> &shadows,
                                   const QList<QList<QGraphicsItem *> > &pieces, bool ignoreMargins,
                                   const QMarginsF &margins) const
{
    for (int i=0; i < scenes.size(); ++i)
    {
        QGraphicsRectItem *paper = qgraphicsitem_cast<QGraphicsRectItem *>(papers.at(i));
        if (paper)
        {
            const QString name = QString("%1/%2%3")
            .arg(dialog.path())                                          //1
            .arg(dialog.fileName())                                      //2
            .arg(ExportLayoutDialog::exportFormatSuffix(dialog.format())); //3

            QBrush *brush = new QBrush();
            brush->setColor(QColor(Qt::white));
            QGraphicsScene *scene = scenes.at(i);
            scene->setBackgroundBrush(*brush);
            shadows[i]->setVisible(false);
            paper->setPen(QPen(QBrush(Qt::white, Qt::NoBrush), 0.1, Qt::NoPen));

            switch (dialog.format())
            {
                case LayoutExportFormat::SVG:
                    paper->setVisible(false);
                    exportSVG(name, paper, pieces.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormat::PDF:
                    exportPDF(name, paper, scene, ignoreMargins, margins);
                    break;
                case LayoutExportFormat::PNG:
                    exportPNG(name, scene);
                    break;
                case LayoutExportFormat::JPG:
                    exportJPG(name, scene);
                    break;
                case LayoutExportFormat::BMP:
                    exportBMP(name, scene);
                    break;
                case LayoutExportFormat::TIF:
                    exportTIF(name, scene);
                    break;
                case LayoutExportFormat::PPM:
                    exportPPM(name, scene);
                    break;
                case LayoutExportFormat::OBJ:
                    paper->setVisible(false);
                    ObjFile(name, paper, scene);
                    paper->setVisible(true);
                    break;
                case LayoutExportFormat::DXF_AC1006_Flat:
                    paper->setVisible(false);
                    FlatDxfFile(name, DRW::AC1006, dialog.isBinaryDXFFormat(), paper, scene, pieces);
                    paper->setVisible(true);
                    break;
                case LayoutExportFormat::DXF_AC1009_Flat:
                    paper->setVisible(false);
                    FlatDxfFile(name, DRW::AC1009, dialog.isBinaryDXFFormat(), paper, scene, pieces);
                    paper->setVisible(true);
                    break;
                case LayoutExportFormat::DXF_AC1012_Flat:
                    paper->setVisible(false);
                    FlatDxfFile(name, DRW::AC1012, dialog.isBinaryDXFFormat(), paper, scene, pieces);
                    paper->setVisible(true);
                    break;
                case LayoutExportFormat::DXF_AC1014_Flat:
                    paper->setVisible(false);
                    FlatDxfFile(name, DRW::AC1014, dialog.isBinaryDXFFormat(), paper, scene, pieces);
                    paper->setVisible(true);
                    break;
                case LayoutExportFormat::DXF_AC1015_Flat:
                    paper->setVisible(false);
                    FlatDxfFile(name, DRW::AC1015, dialog.isBinaryDXFFormat(), paper, scene, pieces);
                    paper->setVisible(true);
                    break;
                case LayoutExportFormat::DXF_AC1018_Flat:
                    paper->setVisible(false);
                    FlatDxfFile(name, DRW::AC1018, dialog.isBinaryDXFFormat(), paper, scene, pieces);
                    paper->setVisible(true);
                    break;
                case LayoutExportFormat::DXF_AC1021_Flat:
                    paper->setVisible(false);
                    FlatDxfFile(name, DRW::AC1021, dialog.isBinaryDXFFormat(), paper, scene, pieces);
                    paper->setVisible(true);
                    break;
                case LayoutExportFormat::DXF_AC1024_Flat:
                    paper->setVisible(false);
                    FlatDxfFile(name, DRW::AC1024, dialog.isBinaryDXFFormat(), paper, scene, pieces);
                    paper->setVisible(true);
                    break;
                case LayoutExportFormat::DXF_AC1027_Flat:
                    paper->setVisible(false);
                    FlatDxfFile(name, DRW::AC1027, dialog.isBinaryDXFFormat(), paper, scene, pieces);
                    paper->setVisible(true);
                    break;
                default:
                    qWarning() << "Can't recognize file type." << Q_FUNC_INFO;
                    break;
            }
            paper->setPen(QPen(Qt::black, 1));
            brush->setColor(QColor(Qt::gray));
            brush->setStyle(Qt::SolidPattern);
            scenes[i]->setBackgroundBrush(*brush);
            shadows[i]->setVisible(true);
            delete brush;
        }
    }
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
