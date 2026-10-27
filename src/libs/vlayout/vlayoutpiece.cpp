//---------------------------------------------------------------------------------------------------------------------
//  @file   vlayoutpiece.cpp
//  @author Douglas S Caskey
//  @date   17 Sep, 2023
//
//  @copyright
//  Copyright (C) 2017 - 2026 Seamly, LLC
//  https://github.com/fashionfreedom/seamly2d
//
//  @brief
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
//  along with Seamly2D. If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
//  @file   vlayoutdetail.cpp
//  @author Roman Telezhynskyi <dismine(at)gmail.com>
//  @date   2 1, 2015
//
//  @brief
//  @copyright
//  This source code is part of the Valentina project, a pattern making
//  program, whose allow create and modeling patterns of clothing.
//  Copyright (C) 2016 Valentina project
//  <https://bitbucket.org/dismine/valentina> All Rights Reserved.
//
//  Valentina is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Valentina is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------------------------------------------------------

#include "vlayoutpiece.h"

#include <QBrush>
#include <QFlags>
#include <QFont>
#include <QFontMetrics>
#include <QFontMetricsF>
#include <QGraphicsPathItem>
#include <QList>
#include <QTransform>
#include <QMessageLogger>
#include <QPainterPath>
#include <QPoint>
#include <QPolygonF>
#include <QSizeF>
#include <QTransform>
#include <Qt>
#include <QtDebug>
#include <QtNumeric>

#include <algorithm>

#include "global.h"
#include "../vpatterndb/floatItemData/vpatternlabeldata.h"
#include "../vpatterndb/floatItemData/vpiecelabeldata.h"
#include "../ifc/ifcdef.h"
#include "../vmisc/vmath.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/calculator.h"
#include "../vgeometry/vpointf.h"
#include "../layout_handoff/handoff_turn_points.h"
#include "vlayoutdef.h"
#include "vlayoutpiece_p.h"
#include "../vpatterndb/LabelTextManager.h"
#include "vgraphicsfillitem.h"
#include "svg_text_item.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setTurnPointsData stores turn point indices on a path item for the SVG data-turn-points attribute.
 * @details No value = unknown: the item gets no data, so SeamlyLayout falls back to its own detection.
 */
void setTurnPointsData(QGraphicsItem *item, const std::optional<QVector<int>> &turnPoints)
{
    if (!turnPoints.has_value())
    {
        return;
    }
    QStringList indices;
    for (int index : turnPoints.value())
    {
        indices.append(QString::number(index));
    }
    item->setData(PieceItemData::TurnPoints, indices.join(QLatin1Char(' ')));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief closedPath draws a polyline closed back to its first point; no points give an empty path.
 */
QPainterPath closedPath(const QVector<QPointF> &points)
{
    QPainterPath path;
    if (points.isEmpty())
    {
        return path;
    }
    path.moveTo(points.first());
    for (int i = 1; i < points.size(); ++i)
    {
        path.lineTo(points.at(i));
    }
    path.lineTo(points.first());
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief openPath draws a polyline that is not closed; no points give an empty path.
 */
QPainterPath openPath(const QVector<QPointF> &points)
{
    QPainterPath path;
    if (points.isEmpty())
    {
        return path;
    }
    path.addPolygon(QPolygonF(points));
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isClosedPath tells whether an internal path polyline ends where it starts.
 */
bool isClosedPath(const QVector<QPointF> &points)
{
    return points.size() > 2 && QLineF(points.first(), points.last()).length() < accuracyPointOnLine;
}

//---------------------------------------------------------------------------------------------------------------------
QVector<VLayoutPiecePath> convertInternalPaths (const VPiece &piece, const VContainer *pattern, const bool isCut)
{
    SCASSERT(pattern != nullptr)

    QVector<VLayoutPiecePath> paths;
    const QVector<quint32> pathIds = piece.getInternalPaths();
    const QVector<QPointF> cutPath = piece.cutPathPoints(pattern);

    for (int i = 0; i < pathIds.size(); ++i)
    {
        const VPiecePath path = pattern->getPiecePath(pathIds.at(i));
        if (path.getType() != PiecePathType::InternalPath || isCut != path.isCutPath())
        {
            continue;
        }

        const QVector<QPointF> points = path.PathPoints(pattern, cutPath);
        VLayoutPiecePath layoutPath(points, path.getLineColor(), path.getLineType(), path.getLineWeight(),
                                    path.isCutPath());
        layoutPath.setNodeVertices(path.pathNodeVertices(pattern), isClosedPath(points));
        paths.append(layoutPath);
    }
    return paths;
}

//---------------------------------------------------------------------------------------------------------------------
bool findLabelGeometry(const VPatternLabelData &labelData, const VContainer *pattern, qreal &rotationAngle,
                       qreal &labelWidth, qreal &labelHeight, QPointF &pos)
{
    SCASSERT(pattern != nullptr)

    try
    {
        Calculator cal1;
        rotationAngle = cal1.EvalFormula(pattern->DataVariables(), labelData.getRotation());
    }
    catch(qmu::QmuParserError &error)
    {
        Q_UNUSED(error);
        return false;
    }

    const quint32 topLeftAnchorPoint = labelData.topLeftAnchorPoint();
    const quint32 bottomRightAnchorPoint = labelData.bottomRightAnchorPoint();

    if (topLeftAnchorPoint != NULL_ID && bottomRightAnchorPoint != NULL_ID)
    {
        try
        {
            const auto topLeftAnchorPointPoint = pattern->GeometricObject<VPointF>(topLeftAnchorPoint);
            const auto bottomRightAnchorPointPoint = pattern->GeometricObject<VPointF>(bottomRightAnchorPoint);

            const QRectF labelRect = QRectF(static_cast<QPointF>(*topLeftAnchorPointPoint),
                                            static_cast<QPointF>(*bottomRightAnchorPointPoint));
            labelWidth = qAbs(labelRect.width());
            labelHeight = qAbs(labelRect.height());

            pos = labelRect.topLeft();

            return true;
        }
        catch(const VExceptionBadId &)
        {
            // do nothing.
        }
    }

    try
    {
        Calculator cal1;
        labelWidth = cal1.EvalFormula(pattern->DataVariables(), labelData.GetLabelWidth());
        labelWidth = ToPixel(labelWidth, *pattern->GetPatternUnit());

        Calculator cal2;
        labelHeight = cal2.EvalFormula(pattern->DataVariables(), labelData.GetLabelHeight());
        labelHeight = ToPixel(labelHeight, *pattern->GetPatternUnit());
    }
    catch(qmu::QmuParserError &error)
    {
        Q_UNUSED(error);
        return false;
    }

    const quint32 centerAnchor = labelData.centerAnchorPoint();
    if (centerAnchor != NULL_ID)
    {
        try
        {
            const auto centerAnchorPoint = pattern->GeometricObject<VPointF>(centerAnchor);
            pos = static_cast<QPointF>(*centerAnchorPoint) - QRectF(0, 0, labelWidth, labelHeight).center();
        }
        catch(const VExceptionBadId &)
        {
            pos = labelData.GetPos();
        }
    }
    else
    {
        pos = labelData.GetPos();
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
bool findGrainlineGeometry(const VGrainlineData& data, const VContainer *pattern, qreal &length, qreal &rotationAngle,
                           qreal &arrowLength, QPointF &pos)
{
    SCASSERT(pattern != nullptr)

    const quint32 topAnchorPoint = data.topAnchorPoint();
    const quint32 bottomAnchorPoint = data.bottomAnchorPoint();

    if (topAnchorPoint != NULL_ID && bottomAnchorPoint != NULL_ID)
    {
        try
        {
            const auto topAnchor_Point = pattern->GeometricObject<VPointF>(topAnchorPoint);
            const auto bottomAnchor_Point = pattern->GeometricObject<VPointF>(bottomAnchorPoint);

            QLineF grainline(static_cast<QPointF>(*bottomAnchor_Point), static_cast<QPointF>(*topAnchor_Point));
            length = grainline.length();
            rotationAngle = grainline.angle();

            Calculator cal3;
            arrowLength = cal3.EvalFormula(pattern->DataVariables(), data.getArrowLength());
            arrowLength = ToPixel(arrowLength, *pattern->GetPatternUnit());

            if (!VFuzzyComparePossibleNulls(rotationAngle, 0))
            {
                grainline.setAngle(0);
            }

            pos = grainline.p1();
            rotationAngle = qDegreesToRadians(rotationAngle);

            return true;
        }
        catch(const VExceptionBadId &)
        {
            // do nothing.
        }
    }

    try
    {
        Calculator cal1;
        rotationAngle = cal1.EvalFormula(pattern->DataVariables(), data.getRotation());
        rotationAngle = qDegreesToRadians(rotationAngle);

        Calculator cal2;
        length = cal2.EvalFormula(pattern->DataVariables(), data.getLength());
        length = ToPixel(length, *pattern->GetPatternUnit());

        Calculator cal3;
        arrowLength = cal3.EvalFormula(pattern->DataVariables(), data.getArrowLength());
        arrowLength = ToPixel(arrowLength, *pattern->GetPatternUnit());
    }
    catch(qmu::QmuParserError &error)
    {
        Q_UNUSED(error);
        return false;
    }

    const quint32 centerAnchor = data.centerAnchorPoint();
    if (centerAnchor != NULL_ID)
    {
        try
        {
            const auto centerAnchorPoint = pattern->GeometricObject<VPointF>(centerAnchor);

            QLineF grainline(centerAnchorPoint->x(), centerAnchorPoint->y(),
                             centerAnchorPoint->x() + length / 2.0, centerAnchorPoint->y());

            grainline.setAngle(qRadiansToDegrees(rotationAngle));
            grainline = QLineF(grainline.p2(), grainline.p1());
            grainline.setLength(length);

            pos = grainline.p2();
        }
        catch(const VExceptionBadId &)
        {
            pos = data.GetPos();
        }
    }
    else
    {
        pos = data.GetPos();
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
bool isItemContained (const QRectF &parentBoundingRect, const QVector<QPointF> &shape, qreal &dX, qreal &dY)
{
    dX = 0;
    dY = 0;
    // single point differences
    bool bInside = true;

    for (int i = 0; i < shape.size(); ++i)
    {
        qreal dPtX = 0;
        qreal dPtY = 0;
        if (!parentBoundingRect.contains(shape.at(i)))
        {
            if (shape.at(i).x() < parentBoundingRect.left())
            {
                dPtX = parentBoundingRect.left() - shape.at(i).x();
            }
            else if (shape.at(i).x() > parentBoundingRect.right())
            {
                dPtX = parentBoundingRect.right() - shape.at(i).x();
            }

            if (shape.at(i).y() < parentBoundingRect.top())
            {
                dPtY = parentBoundingRect.top() - shape.at(i).y();
            }
            else if (shape.at(i).y() > parentBoundingRect.bottom())
            {
                dPtY = parentBoundingRect.bottom() - shape.at(i).y();
            }

            if (fabs(dPtX) > fabs(dX))
            {
                dX = dPtX;
            }

            if (fabs(dPtY) > fabs(dY))
            {
                dY = dPtY;
            }

            bInside = false;
        }
    }
    return bInside;
}

//---------------------------------------------------------------------------------------------------------------------
QVector<QPointF> correctPosition (const QRectF &parentBoundingRect, QVector<QPointF> points)
{
    qreal dX = 0;
    qreal dY = 0;
    if (!isItemContained (parentBoundingRect, points, dX, dY))
    {
        for (int i =0; i < points.size(); ++i)
        {
            points[i] = QPointF(points.at(i).x() + dX, points.at(i).y() + dY);
        }
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
QVector<SeamAllowancePoint> prepareAllowance (const QVector<QPointF> &points)
{
    QVector<SeamAllowancePoint> allowancePoints;
    for(int i = 0; i < points.size(); ++i)
    {
        allowancePoints.append(SeamAllowancePoint(points.at(i)));
    }
    return allowancePoints;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief VLayoutPiece::rotatePoint  rotates a point around the center for given angle
/// @param ptCenter center around which the point is rotated
/// @param pt point, which is rotated around the center
/// @param rotationAngle angle of rotation
/// @return position of point pt after rotating it around the center for rotation radians
//---------------------------------------------------------------------------------------------------------------------
QPointF rotatePoint (const QPointF &ptCenter, const QPointF& pt, qreal rotationAngle)
{
    QPointF ptDest;
    QPointF ptRel = pt - ptCenter;
    ptDest.setX(cos(rotationAngle)*ptRel.x() - sin(rotationAngle)*ptRel.y());
    ptDest.setY(sin(rotationAngle)*ptRel.x() + cos(rotationAngle)*ptRel.y());

    return ptDest + ptCenter;
}

//---------------------------------------------------------------------------------------------------------------------
QStringList getLabelText (const QVector<QPointF> &labelShape, const LabelTextManager &tm)
{
    QStringList text;
    if (labelShape.count() > 2)
    {
        for (int i = 0; i < tm.GetSourceLinesCount(); ++i)
        {
            text.append(tm.GetSourceLine(i).text);
        }
    }
    return text;
}
}

//---------------------------------------------------------------------------------------------------------------------

#ifdef Q_COMPILER_RVALUE_REFS
VLayoutPiece &VLayoutPiece::operator=(VLayoutPiece &&piece) noexcept { Swap(piece); return *this; }
#endif

void VLayoutPiece::Swap(VLayoutPiece &piece) noexcept
{ AbstractPiece::Swap(piece); std::swap(d, piece.d); }

//---------------------------------------------------------------------------------------------------------------------
VLayoutPiece::VLayoutPiece()
    : AbstractPiece()
    , d(new VLayoutPieceData)
{}

//---------------------------------------------------------------------------------------------------------------------
VLayoutPiece::VLayoutPiece(const VLayoutPiece &piece)
    : AbstractPiece(piece)
    , d(piece.d)
{}

//---------------------------------------------------------------------------------------------------------------------
VLayoutPiece &VLayoutPiece::operator=(const VLayoutPiece &piece)
{
    if ( &piece == this )
    {
        return *this;
    }
    AbstractPiece::operator=(piece);
    d = piece.d;
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
VLayoutPiece::~VLayoutPiece()
{}

//---------------------------------------------------------------------------------------------------------------------
VLayoutPiece VLayoutPiece::Create(const VPiece &piece, const VContainer *pattern)
{
    VLayoutPiece layoutPiece;

    layoutPiece.SetMx(piece.GetMx());
    layoutPiece.SetMy(piece.GetMy());
    layoutPiece.setMainPathPoints(piece.mainPathPoints(pattern), piece.isHideSeamLine(),
                                  piece.mainPathNodeVertices(pattern));
    QVector<QPointF> seamAllowanceNodes;
    SeamAllowanceCorners seamAllowanceCorners;
    const QVector<QPointF> seamAllowance = piece.seamAllowancePoints(pattern, &seamAllowanceNodes,
                                                                     &seamAllowanceCorners);
    layoutPiece.setSeamAllowancePoints(seamAllowance,
                                       piece.hasSeamAllowance(),
                                       piece.hasSeamAllowanceBuiltIn(),
                                       seamAllowanceNodes);
    layoutPiece.setSeamAllowanceCorners(seamAllowanceCorners);
    QVector<QPointF> builtInSeamLineNodes;
    const QVector<QPointF> builtInSeamLine = piece.builtInSeamLinePoints(pattern, &builtInSeamLineNodes);
    // A closed seam line repeats its first point; one that a fold line cut open ends elsewhere.
    const bool builtInSeamLineClosed = builtInSeamLine.size() < 2 || builtInSeamLine.first() == builtInSeamLine.last();
    layoutPiece.setBuiltInSeamLinePoints(builtInSeamLine, builtInSeamLineNodes, builtInSeamLineClosed);
    layoutPiece.setInternalPaths(convertInternalPaths (piece, pattern, false));
    layoutPiece.setCutoutPaths(convertInternalPaths (piece, pattern, true));
    // Clip cutline notches to the cutline, as the canvas does.
    QVector<qreal> notchSeamAllowances;
    QStringList notchTypes;
    QVector<int> notchLineCounts;
    const QVector<QLineF> notches = piece.createNotchLines(pattern, seamAllowance, true, &notchSeamAllowances,
                                                           &notchTypes, &notchLineCounts);
    layoutPiece.setNotches(notches, notchLineCounts, notchSeamAllowances, notchTypes);
    layoutPiece.SetName(piece.GetName());
    // Keep the piece letter, cut count and fold flag for the SVG exporter's data-* attributes.
    layoutPiece.setPieceLetter(piece.GetPatternPieceData().GetLetter());
    layoutPiece.setQuantity(piece.GetPatternPieceData().GetQuantity());
    // A fold line puts the piece on the fold, whatever the label data says. A sent cut line runs along the fold line
    // past both nodes, so the fold line is sent out to the cut line.
    const QLineF foldLine = piece.foldLine(pattern);
    layoutPiece.setOnFold(piece.GetPatternPieceData().IsOnFold() || !foldLine.isNull());
    if (!foldLine.isNull())
    {
        const bool cut_line_sent = qApp->Settings()->showSeamAllowances() && !seamAllowance.isEmpty();
        const QLineF sent_fold_line = cut_line_sent ? VPiece::foldLineOnCutLine(seamAllowance, foldLine) : foldLine;
        layoutPiece.setFoldLine(QVector<QPointF>{sent_fold_line.p1(), sent_fold_line.p2()});
        layoutPiece.setFoldLineGraphic(pattern_piece::FoldLineGraphic(foldLine, piece.mainPathPoints(pattern)));
    }
    layoutPiece.setMaterial(piece.getMaterial());
    layoutPiece.setPieceProperties(piece.GetPatternPieceData());

    // Very important to set main path first!
    if (layoutPiece.createMainPath().isEmpty() && layoutPiece.createAllowancePath().isEmpty())
    {
        throw VException (tr("Piece %1 doesn't have shape.").arg(piece.GetName()));
    }

    // Disable piece labels in exports and layouts
    const VPieceLabelData& pieceLabelData = piece.GetPatternPieceData();
    if (pieceLabelData.IsVisible() & qApp->Settings()->showLabels())
    {
        layoutPiece.setPieceText(piece.GetName(), pieceLabelData, qApp->Settings()->getLabelFont(), pattern);
    }

    // Disable pattern labels in exports and layouts
    const VPatternLabelData& patternLabelData = piece.GetPatternInfo();
    if (patternLabelData.IsVisible() & qApp->Settings()->showLabels())
    {
        VAbstractPattern* pDoc = qApp->getCurrentDocument();
        layoutPiece.setPatternText(pDoc, patternLabelData, qApp->Settings()->getLabelFont(), pattern);
    }

    // Disable grainlines in exports and layouts
    const VGrainlineData& grainlineGeom = piece.GetGrainlineGeometry();
    // The grain direction is kept even when the grainline is not drawn, so
    // SeamlyLayout can still orient the piece.
    layoutPiece.setGrainAxis(grainlineGeom, pattern);
    if (grainlineGeom.IsVisible() & qApp->Settings()->showGrainlines())
    {
        layoutPiece.setGrainline(grainlineGeom, pattern);
    }

    layoutPiece.SetSAWidth(qApp->toPixel(piece.GetSAWidth()));
    layoutPiece.setFlip(piece.getFlip());

    return layoutPiece;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getPieceLetter returns the piece letter entered in the piece label data.
 * @return piece letter, or an empty string when none was set.
 */
QString VLayoutPiece::getPieceLetter() const
{
    return d->m_pieceLetter;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setPieceLetter stores the piece letter for later use by the SVG exporter (data-letter attribute).
 * @param letter piece letter from VPieceLabelData::GetLetter().
 */
void VLayoutPiece::setPieceLetter(const QString &letter)
{
    d->m_pieceLetter = letter;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getQuantity returns the cut count from the piece label data.
 */
int VLayoutPiece::getQuantity() const
{
    return d->m_quantity;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setQuantity stores the cut count for the SVG exporter (data-quantity attribute).
 * @param quantity cut count from VPieceLabelData::GetQuantity().
 */
void VLayoutPiece::setQuantity(int quantity)
{
    d->m_quantity = quantity;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isOnFold returns true when the piece label data says the piece is cut on the fold.
 */
bool VLayoutPiece::isOnFold() const
{
    return d->m_onFold;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setOnFold stores the fold flag for the SVG exporter (data-on-fold attribute).
 * @param onFold fold flag from VPieceLabelData::IsOnFold().
 */
void VLayoutPiece::setOnFold(bool onFold)
{
    d->m_onFold = onFold;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getPieceProperties returns the Piece Properties the SVG exporter reads, whether or not a label shows them.
 */
VPieceLabelData VLayoutPiece::getPieceProperties() const
{
    return d->m_pieceProperties;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setPieceProperties stores the Piece Properties for the SVG exporter's data-* attributes.
 * @param properties piece label data from VPiece::GetPatternPieceData().
 */
void VLayoutPiece::setPieceProperties(const VPieceLabelData &properties)
{
    d->m_pieceProperties = properties;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getFoldLine returns the fold line start and end points in layout coordinates.
 * @return an empty vector when the piece has no fold line.
 */
QVector<QPointF> VLayoutPiece::getFoldLine() const
{
    return Map(d->m_foldLine);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setFoldLine stores the fold line for the exports.
 * @param points fold line start and end points in piece coordinates, from VPiece::foldLine().
 */
void VLayoutPiece::setFoldLine(const QVector<QPointF> &points)
{
    d->m_foldLine = points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getFoldLineGraphic returns the fold line arrow line and label in layout coordinates.
 * @return a null graphic when the piece has no fold line.
 */
pattern_piece::FoldLineGraphic VLayoutPiece::getFoldLineGraphic() const
{
    return d->m_fold_line_graphic.mapped(d->transform);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setFoldLineGraphic stores the fold line arrow line and label for the exports.
 * @param graphic graphic in piece coordinates, made from VPiece::foldLine().
 */
void VLayoutPiece::setFoldLineGraphic(const pattern_piece::FoldLineGraphic& graphic)
{
    d->m_fold_line_graphic = graphic;
}

//---------------------------------------------------------------------------------------------------------------------
// cppcheck-suppress unusedFunction
QVector<QPointF> VLayoutPiece::getContourPoints() const
{
    return Map(d->contour);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setMainPathPoints stores the main path (seam line) polyline.
 * @param nodeVertices vertices that can break the tangent, from VPiece::mainPathNodeVertices().
 *        Empty = turn points unknown, so the seam line gets no data-turn-points attribute.
 */
void VLayoutPiece::setMainPathPoints(const QVector<QPointF> &points, bool hideMainPath,
                                     const QVector<QPointF> &nodeVertices)
{
    d->contour = RemoveDublicates(points, false);
    d->contourTurnPoints.reset();
    d->contourNodeIndices.clear();
    if (!nodeVertices.isEmpty())
    {
        d->contour = snapNodeVertices(d->contour, nodeVertices, true);
        d->contourTurnPoints = turnPointIndices(d->contour, nodeVertices, true);
        d->contourNodeIndices = nodeVertexIndices(d->contour, nodeVertices, true);
    }
    setHideSeamLine(hideMainPath);
}

//---------------------------------------------------------------------------------------------------------------------
// cppcheck-suppress unusedFunction
QVector<QPointF> VLayoutPiece::getSeamAllowancePoints() const
{
    return Map(d->seamAllowance);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setSeamAllowancePoints stores the seam allowance (cut line) polyline.
 * @param nodeVertices vertices that can break the tangent, from VPiece::seamAllowancePoints().
 *        Empty = turn points unknown, so the cut line gets no data-turn-points attribute.
 */
void VLayoutPiece::setSeamAllowancePoints(const QVector<QPointF> &points, bool seamAllowance, bool seamAllowanceBuiltIn,
                                          const QVector<QPointF> &nodeVertices)
{
    d->seamAllowanceTurnPoints.reset();
    d->seamAllowanceNodeIndices.clear();
    if (seamAllowance)
    {
        SetSeamAllowance(seamAllowance);
        SetSeamAllowanceBuiltIn(seamAllowanceBuiltIn);
        d->seamAllowance = points;
        if (!d->seamAllowance.isEmpty())
        {
            d->seamAllowance = RemoveDublicates(d->seamAllowance, false);
            if (!nodeVertices.isEmpty())
            {
                d->seamAllowance = snapNodeVertices(d->seamAllowance, nodeVertices, true);
                d->seamAllowanceTurnPoints = turnPointIndices(d->seamAllowance, nodeVertices, true);
                d->seamAllowanceNodeIndices = nodeVertexIndices(d->seamAllowance, nodeVertices, true);
            }
        }
        else if (!hasSeamAllowanceBuiltIn())
        {
            qWarning() << "Seam allowance is empty.";
            SetSeamAllowance(false);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief seamAllowanceCorners returns the seam allowance corners and the ones a ByLength angle cut short.
 */
SeamAllowanceCorners VLayoutPiece::seamAllowanceCorners() const
{
    return d->seamAllowanceCorners;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief lineNodeCounts returns the node vertex counts of the seam line, the cut line and the built-in seam line.
 * @details unifyTurnPoints() matches the nodes of a line between sizes only when its count is the same in every size.
 */
QVector<int> VLayoutPiece::lineNodeCounts() const
{
    return {static_cast<int>(d->contourNodeIndices.size()), static_cast<int>(d->seamAllowanceNodeIndices.size()),
            static_cast<int>(d->builtInSeamLineNodeIndices.size())};
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::setSeamAllowanceCorners(const SeamAllowanceCorners &corners)
{
    d->seamAllowanceCorners = corners;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getBuiltInSeamLinePoints returns the seam line inside a main path with a built-in seam allowance.
 */
QVector<QPointF> VLayoutPiece::getBuiltInSeamLinePoints() const
{
    return Map(d->builtInSeamLine);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief isBuiltInSeamLineClosed tells whether the built-in seam line is a closed loop.
 * @return false when a fold line cuts it open: then both ends lie on the fold line.
 */
bool VLayoutPiece::isBuiltInSeamLineClosed() const
{
    return d->builtInSeamLineClosed;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setBuiltInSeamLinePoints stores the seam line of a piece whose seam allowance is built into the main path.
 * @param points from VPiece::builtInSeamLinePoints(). Used only while the seam allowance is built in.
 * @param nodeVertices vertices that can break the tangent. Empty = turn points unknown, so the seam line gets no
 *        data-turn-points attribute.
 * @param closed false for a line that a fold line cut open: its ends are not joined.
 */
void VLayoutPiece::setBuiltInSeamLinePoints(const QVector<QPointF> &points, const QVector<QPointF> &nodeVertices,
                                            bool closed)
{
    d->builtInSeamLineClosed = closed;
    // A closed line is stored without its closing point, like the contour: closedPath() closes it.
    d->builtInSeamLine = RemoveDublicates(points, closed);
    d->builtInSeamLineTurnPoints.reset();
    d->builtInSeamLineNodeIndices.clear();
    if (!d->builtInSeamLine.isEmpty() && !nodeVertices.isEmpty())
    {
        d->builtInSeamLine = snapNodeVertices(d->builtInSeamLine, nodeVertices, closed);
        d->builtInSeamLineTurnPoints = turnPointIndices(d->builtInSeamLine, nodeVertices, closed);
        d->builtInSeamLineNodeIndices = nodeVertexIndices(d->builtInSeamLine, nodeVertices, closed);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief unifyTurnPoints makes a piece node that is a turn point in one size a turn point in every size.
 * @details turnPointIndices() tests the tangent break at a node against a fixed angle, so a node near that angle
 *          is a turn point in some sizes only. Grading needs the same turn points in every size. Applies to the
 *          seam line, the cut line, the built-in seam line, internal paths and cutout paths. A line whose node
 *          count differs between sizes keeps its turn points.
 * @param sizes the same piece in every size, in size order.
 */
void VLayoutPiece::unifyTurnPoints(const QVector<VLayoutPiece *> &sizes)
{
    if (sizes.size() < 2)
    {
        return;
    }

    // Seam line, cut line and built-in seam line: one per piece; closed loops, except a built-in seam line that a
    // fold line cuts open.
    const auto unifyLoop = [&sizes](QVector<QPointF> VLayoutPieceData::*points,
                                    std::optional<QVector<int>> VLayoutPieceData::*turnPoints,
                                    QVector<int> VLayoutPieceData::*nodeIndices, bool closed)
    {
        QVector<layout_handoff::SizeLine> lines;
        for (const VLayoutPiece *piece : sizes)
        {
            const VLayoutPieceData *data = piece->d.constData();
            lines.append({data->*points, data->*turnPoints, data->*nodeIndices});
        }
        const QVector<QVector<int>> turns = layout_handoff::unify_turn_points(lines, closed);
        for (int s = 0; s < turns.size(); ++s)
        {
            sizes.at(s)->d.data()->*turnPoints = turns.at(s);
        }
    };
    unifyLoop(&VLayoutPieceData::contour, &VLayoutPieceData::contourTurnPoints,
              &VLayoutPieceData::contourNodeIndices, true);
    unifyLoop(&VLayoutPieceData::seamAllowance, &VLayoutPieceData::seamAllowanceTurnPoints,
              &VLayoutPieceData::seamAllowanceNodeIndices, true);
    // Sizes whose built-in seam lines differ in closed and open keep their turn points.
    const bool builtInClosed = sizes.first()->d.constData()->builtInSeamLineClosed;
    const bool sameBuiltInShape = std::all_of(sizes.cbegin(), sizes.cend(), [builtInClosed](const VLayoutPiece *piece)
    {
        return piece->d.constData()->builtInSeamLineClosed == builtInClosed;
    });
    if (sameBuiltInShape)
    {
        unifyLoop(&VLayoutPieceData::builtInSeamLine, &VLayoutPieceData::builtInSeamLineTurnPoints,
                  &VLayoutPieceData::builtInSeamLineNodeIndices, builtInClosed);
    }

    // Internal and cutout paths match by their order in the piece, so every size needs the same path count.
    const auto unifyPaths = [&sizes](QVector<VLayoutPiecePath> VLayoutPieceData::*paths)
    {
        const int count = (sizes.first()->d.constData()->*paths).size();
        for (const VLayoutPiece *piece : sizes)
        {
            if ((piece->d.constData()->*paths).size() != count)
            {
                return;
            }
        }
        for (int i = 0; i < count; ++i)
        {
            QVector<layout_handoff::SizeLine> lines;
            for (const VLayoutPiece *piece : sizes)
            {
                const VLayoutPiecePath &path = (piece->d.constData()->*paths).at(i);
                lines.append({path.Points(), path.turnPoints(), path.nodeIndices()});
            }
            const QVector<QVector<int>> turns = layout_handoff::unify_turn_points(lines,
                                                                                  isClosedPath(lines.first().points));
            for (int s = 0; s < turns.size(); ++s)
            {
                (sizes.at(s)->d.data()->*paths)[i].setTurnPoints(turns.at(s));
            }
        }
    };
    unifyPaths(&VLayoutPieceData::m_internalPaths);
    unifyPaths(&VLayoutPieceData::m_cutoutPaths);
}

//---------------------------------------------------------------------------------------------------------------------
QVector<QPointF> VLayoutPiece::getLayoutAllowancePoints() const
{
    return Map(d->layoutAllowance);
}

//---------------------------------------------------------------------------------------------------------------------
QPointF VLayoutPiece::getPieceTextPosition() const
{
    if (d->pieceLabel.count() > 2)
    {
        return d->transform.map(d->pieceLabel.first());
    }
    else
    {
        return QPointF();
    }
}

//---------------------------------------------------------------------------------------------------------------------
QStringList VLayoutPiece::getPieceText() const
{
    return getLabelText (d->pieceLabel, d->m_tmPiece);
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::setPieceText(const QString& qsName, const VPieceLabelData& data, const QFont &font,
                                const VContainer *pattern)
{
    QPointF ptPos;
    qreal labelWidth = 0;
    qreal labelHeight = 0;
    qreal labelAngle = 0;
    if (!findLabelGeometry(data, pattern, labelAngle, labelWidth, labelHeight, ptPos))
    {
        return;
    }

    QVector<QPointF> v;
    v << ptPos
      << QPointF(ptPos.x() + labelWidth, ptPos.y())
      << QPointF(ptPos.x() + labelWidth, ptPos.y() + labelHeight)
      << QPointF(ptPos.x(), ptPos.y() + labelHeight);

    const qreal rotationAngle = qDegreesToRadians(-labelAngle);
    const QPointF ptCenter(ptPos.x() + labelWidth/2, ptPos.y() + labelHeight/2);

    for (int i = 0; i < v.count(); ++i)
    {
        v[i] = rotatePoint (ptCenter, v.at(i), rotationAngle);
    }

    QScopedPointer<QGraphicsItem> item(getMainPathItem());
    d->pieceLabel = correctPosition (item->boundingRect(), v);

    // generate text
    d->m_tmPiece.setFont(font);
    d->m_tmPiece.SetFontSize(data.getFontSize());
    d->m_tmPiece.Update(qsName, data);

    // this will generate the lines of text
    d->m_tmPiece.SetFontSize(data.getFontSize());
    d->m_tmPiece.FitFontSize(labelWidth, labelHeight);
}

//---------------------------------------------------------------------------------------------------------------------
QPointF VLayoutPiece::getPatternTextPosition() const
{
    if (d->patternInfo.count() > 2)
    {
        return d->transform.map(d->patternInfo.first());
    }
    else
    {
        return QPointF();
    }
}

//---------------------------------------------------------------------------------------------------------------------
QStringList VLayoutPiece::getPatternText() const
{
    return getLabelText (d->patternInfo, d->m_tmPattern);
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::setPatternText(VAbstractPattern* pDoc, const VPatternLabelData& data, const QFont &font,
                                  const VContainer *pattern)
{
    QPointF ptPos;
    qreal labelWidth = 0;
    qreal labelHeight = 0;
    qreal labelAngle = 0;
    if (!findLabelGeometry(data, pattern, labelAngle, labelWidth, labelHeight, ptPos))
    {
        return;
    }

    QVector<QPointF> v;
    v << ptPos
      << QPointF(ptPos.x() + labelWidth, ptPos.y())
      << QPointF(ptPos.x() + labelWidth, ptPos.y() + labelHeight)
      << QPointF(ptPos.x(), ptPos.y() + labelHeight);

    const qreal rotationAngle = qDegreesToRadians(-labelAngle);
    const QPointF ptCenter(ptPos.x() + labelWidth/2, ptPos.y() + labelHeight/2);
    for (int i = 0; i < v.count(); ++i)
    {
        v[i] = rotatePoint (ptCenter, v.at(i), rotationAngle);
    }
    QScopedPointer<QGraphicsItem> item(getMainPathItem());
    d->patternInfo = correctPosition (item->boundingRect(), v);

    // Generate text
    d->m_tmPattern.setFont(font);
    d->m_tmPattern.SetFontSize(data.getFontSize());
    d->m_tmPattern.Update(pDoc);

    // generate lines of text
    d->m_tmPattern.SetFontSize(data.getFontSize());
    d->m_tmPattern.FitFontSize(labelWidth, labelHeight);
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::setGrainline(const VGrainlineData& data, const VContainer* pattern)
{
    SCASSERT(pattern != nullptr)

    QPointF pt1;
    qreal rotationAngle = 0;
    qreal length = 0;
    qreal arrowLength = 0;
    if (!findGrainlineGeometry(data, pattern, length, rotationAngle, arrowLength, pt1))
    {
        return;
    }

    QPointF pt2(pt1.x() + length * qCos(rotationAngle),
                pt1.y() - length * qSin(rotationAngle));

    QVector<QPointF> path;
    path << pt1;
    if (data.getArrowType() != ArrowType::Top)
    {
        path << QPointF(pt1.x() + arrowLength * cos(rotationAngle + M_PI / 9),
                        pt1.y() - arrowLength * sin(rotationAngle + M_PI / 9));

        path << QPointF(pt1.x() + arrowLength * cos(rotationAngle - M_PI / 9),
                        pt1.y() - arrowLength * sin(rotationAngle - M_PI / 9));
    }
    path << pt1 << pt2;

    if (data.getArrowType() != ArrowType::Bottom)
    {
        path << QPointF(pt2.x() + arrowLength * cos(M_PI + rotationAngle + M_PI / 9),
                        pt2.y() - arrowLength * sin(M_PI + rotationAngle + M_PI / 9));

        path << QPointF(pt2.x() + arrowLength * cos(M_PI + rotationAngle - M_PI / 9),
                        pt2.y() - arrowLength * sin(M_PI + rotationAngle - M_PI / 9));
    }
    path << pt2;

    QScopedPointer<QGraphicsItem> item(getMainPathItem());
    d->grainlinePoints = correctPosition (item->boundingRect(), path);
}

//---------------------------------------------------------------------------------------------------------------------
QVector<QPointF> VLayoutPiece::getGrainline() const
{
    return Map(d->grainlinePoints);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setGrainAxis stores the grain direction, whether or not the grainline is visible.
 * @param data    grainline data of the piece.
 * @param pattern pattern container used to evaluate the grainline formulas.
 */
void VLayoutPiece::setGrainAxis(const VGrainlineData &data, const VContainer *pattern)
{
    SCASSERT(pattern != nullptr)

    d->grainAxis.clear();

    QPointF pt1;
    qreal rotationAngle = 0;
    qreal length = 0;
    qreal arrowLength = 0;
    if (!findGrainlineGeometry(data, pattern, length, rotationAngle, arrowLength, pt1))
    {
        return;
    }

    // Without top and bottom anchors the grain points up, so SeamlyLayout sees one direction per axis.
    if (data.topAnchorPoint() == NULL_ID || data.bottomAnchorPoint() == NULL_ID)
    {
        rotationAngle = qDegreesToRadians(VGrainlineData::upwardAngle(qRadiansToDegrees(rotationAngle)));
    }

    // Only the direction is used, so a zero or invalid length must not hide it.
    const QPointF pt2(pt1.x() + qCos(rotationAngle), pt1.y() - qSin(rotationAngle));
    d->grainAxis << pt1 << pt2;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief grainlineAngle returns the grain direction after the piece transform.
 * @param angle receives the direction in degrees, counter-clockwise on screen (QLineF::angle()).
 * @return false when the piece has no grain direction.
 */
bool VLayoutPiece::grainlineAngle(qreal &angle) const
{
    if (d->grainAxis.size() != 2)
    {
        return false;
    }

    const QVector<QPointF> axis = Map(d->grainAxis);
    const QLineF line(axis.at(0), axis.at(1));
    if (qFuzzyIsNull(line.length()))
    {
        return false;
    }

    angle = line.angle();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
QTransform VLayoutPiece::getTransform() const
{
    return d->transform;
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::setTransform(const QTransform &transform)
{
    d->transform = transform;
}

//---------------------------------------------------------------------------------------------------------------------
qreal VLayoutPiece::getLayoutGap() const
{
    return d->layoutWidth;
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::setLayoutGap(const qreal &value)
{
    d->layoutWidth = value;
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::Translate(qreal dx, qreal dy)
{
    QTransform m;
    m.translate(dx, dy);
    d->transform *= m;
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::Rotate(const QPointF &originPoint, qreal degrees)
{
    QTransform m;
    m.translate(originPoint.x(), originPoint.y());
    m.rotate(-degrees);
    m.translate(-originPoint.x(), -originPoint.y());
    d->transform *= m;
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::Mirror(const QLineF &edge)
{
    if (edge.isNull())
    {
        return;
    }

    const QLineF axis = QLineF(edge.x2(), edge.y2(), edge.x2() + 100, edge.y2()); // Ox axis

    const qreal angle = edge.angleTo(axis);
    const QPointF p2 = edge.p2();
    QTransform m;
    m.translate(p2.x(), p2.y());
    m.rotate(-angle);
    m.translate(-p2.x(), -p2.y());
    d->transform *= m;

    m.reset();
    m.translate(p2.x(), p2.y());
    m.scale(m.m11(), m.m22()*-1);
    m.translate(-p2.x(), -p2.y());
    d->transform *= m;

    m.reset();
    m.translate(p2.x(), p2.y());
    m.rotate(-(360-angle));
    m.translate(-p2.x(), -p2.y());
    d->transform *= m;

    d->mirror = !d->mirror;
}

//---------------------------------------------------------------------------------------------------------------------
int VLayoutPiece::pieceEdgesCount() const
{
    return piecePath().count();
}

//---------------------------------------------------------------------------------------------------------------------
int VLayoutPiece::LayoutEdgesCount() const
{
    return d->layoutAllowance.count();
}

//---------------------------------------------------------------------------------------------------------------------
QLineF VLayoutPiece::pieceEdge(int i) const
{
    return Edge(piecePath(), i);
}

//---------------------------------------------------------------------------------------------------------------------
QLineF VLayoutPiece::LayoutEdge(int i) const
{
    return Edge(d->layoutAllowance, i);
}

//---------------------------------------------------------------------------------------------------------------------
int VLayoutPiece::pieceEdgeByPoint(const QPointF &p1) const
{
    return EdgeByPoint(piecePath(), p1);
}

//---------------------------------------------------------------------------------------------------------------------
int VLayoutPiece::LayoutEdgeByPoint(const QPointF &p1) const
{
    return EdgeByPoint(d->layoutAllowance, p1);
}

//---------------------------------------------------------------------------------------------------------------------
QRectF VLayoutPiece::pieceBoundingRect() const
{
    QVector<QPointF> points;
    if (hasSeamAllowance() && not hasSeamAllowanceBuiltIn())
    {
        points = getSeamAllowancePoints();
    }
    else
    {
        points = getContourPoints();
    }

    points.append(points.first());
    return QPolygonF(points).boundingRect();
}

//---------------------------------------------------------------------------------------------------------------------
QRectF VLayoutPiece::LayoutBoundingRect() const
{
    QVector<QPointF> points = getLayoutAllowancePoints();
    points.append(points.first());
    return QPolygonF(points).boundingRect();
}

//---------------------------------------------------------------------------------------------------------------------
qreal VLayoutPiece::Diagonal() const
{
    const QRectF rec = LayoutBoundingRect();
    return qSqrt(pow(rec.height(), 2) + pow(rec.width(), 2));
}

//---------------------------------------------------------------------------------------------------------------------
bool VLayoutPiece::isNull() const
{
    if (d->contour.isEmpty() == false && d->layoutWidth > 0)
    {
        if (hasSeamAllowance() && not hasSeamAllowanceBuiltIn() && d->seamAllowance.isEmpty() == false)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
qint64 VLayoutPiece::Square() const
{
    if (d->layoutAllowance.isEmpty()) //-V807
    {
        return 0;
    }

    const qreal res = sumTrapezoids(d->layoutAllowance);

    const qint64 sq = qFloor(qAbs(res/2.0));
    return sq;
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::SetLayoutAllowancePoints()
{
    if (d->layoutWidth > 0)
    {
        if (hasSeamAllowance() && not hasSeamAllowanceBuiltIn())
        {
            d->layoutAllowance = Equidistant(prepareAllowance (getSeamAllowancePoints()), d->layoutWidth);
            if (d->layoutAllowance.isEmpty() == false)
            {
                d->layoutAllowance.removeLast();
            }
        }
        else
        {
            d->layoutAllowance = Equidistant(prepareAllowance (getContourPoints()), d->layoutWidth);
            if (d->layoutAllowance.isEmpty() == false)
            {
                d->layoutAllowance.removeLast();
            }
        }
    }
    else
    {
        d->layoutAllowance.clear();
    }
}

//---------------------------------------------------------------------------------------------------------------------
QVector<QLineF> VLayoutPiece::getNotches() const
{
    return Map(d->notches);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setNotches sets the notch lines of a piece with a seam allowance.
 * @param notchLineCounts line count per notch, in line order. Dropped, together with the widths and types, unless
 *        every count is positive and the counts sum to the line count.
 * @param seamAllowanceWidths seam allowance width in pixels per notch. Any other count is dropped.
 * @param notchTypes notch type per notch (notchTypeToString()). Any other count is dropped.
 */
void VLayoutPiece::setNotches(const QVector<QLineF> &notches, const QVector<int> &notchLineCounts,
                              const QVector<qreal> &seamAllowanceWidths, const QStringList &notchTypes)
{
    if (!hasSeamAllowance())
    {
        return;
    }

    d->notches = notches;
    d->notchLineCounts.clear();
    d->notchSeamAllowances.clear();
    d->notchTypes.clear();

    int lineCount = 0;
    for (int count : notchLineCounts)
    {
        if (count <= 0)
        {
            return;
        }
        lineCount += count;
    }
    if (notchLineCounts.isEmpty() || lineCount != notches.size())
    {
        return;
    }

    d->notchLineCounts = notchLineCounts;
    if (seamAllowanceWidths.size() == notchLineCounts.size())
    {
        d->notchSeamAllowances = seamAllowanceWidths;
    }
    if (notchTypes.size() == notchLineCounts.size())
    {
        d->notchTypes = notchTypes;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getNotchLineCounts returns the line count per notch.
 * @return in getNotches() order, summing to its size; empty when unknown.
 */
QVector<int> VLayoutPiece::getNotchLineCounts() const
{
    return d->notchLineCounts;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getNotchSeamAllowances returns the seam allowance width in pixels per notch.
 * @return parallel to getNotchLineCounts(); empty when unknown.
 */
QVector<qreal> VLayoutPiece::getNotchSeamAllowances() const
{
    return d->notchSeamAllowances;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getNotchTypes returns the notch type per notch.
 * @return parallel to getNotchLineCounts(); empty when unknown.
 */
QStringList VLayoutPiece::getNotchTypes() const
{
    return d->notchTypes;
}

//---------------------------------------------------------------------------------------------------------------------
QVector<QVector<QPointF> > VLayoutPiece::internalPathsForCut (bool cut) const
{
    QVector<QVector<QPointF> > paths;

    for (int i=0;i < d->m_internalPaths.count(); ++i)
    {
        if (d->m_internalPaths.at(i).isCutPath() == cut)
        {
            paths.append(Map(d->m_internalPaths.at(i).Points()));
        }
    }

    return paths;
}

//---------------------------------------------------------------------------------------------------------------------
QVector<VLayoutPiecePath> VLayoutPiece::getInternalPaths() const
{
    return d->m_internalPaths;
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::setInternalPaths(const QVector<VLayoutPiecePath> &internalPaths)
{
    d->m_internalPaths = internalPaths;
}

//---------------------------------------------------------------------------------------------------------------------
QVector<VLayoutPiecePath> VLayoutPiece::getCutoutPaths() const
{
    return d->m_cutoutPaths;
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::setCutoutPaths(const QVector<VLayoutPiecePath> &cutoutPaths)
{
    d->m_cutoutPaths = cutoutPaths;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T>
QVector<T> VLayoutPiece::Map(const QVector<T> &points) const
{
    QVector<T> p;
    for (int i = 0; i < points.size(); ++i)
    {
        p.append(d->transform.map(points.at(i)));
    }

    if (d->mirror)
    {
        QList<T> list = p.toList();
        for (int k=0, s=list.size(), max=(s/2); k<max; k++)
        {
			list.swapItemsAt(k, s-(1+k));
        }
        p = list.toVector();
    }
    return p;
}

//---------------------------------------------------------------------------------------------------------------------
QPainterPath VLayoutPiece::createMainPath() const
{
    QPainterPath path;

    // contour
    QVector<QPointF> points = getContourPoints();

    if (!isHideSeamLine() || not hasSeamAllowance() || hasSeamAllowanceBuiltIn())
    {
        path.moveTo(points.at(0));
        for (qint32 i = 1; i < points.count(); ++i)
        {
            path.lineTo(points.at(i));
        }
        path.lineTo(points.at(0));
    }
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
QPainterPath VLayoutPiece::createAllowancePath() const
{
    QPainterPath path;

    // seam allowance
    if (hasSeamAllowance())
    {
        if (!hasSeamAllowanceBuiltIn())
        {
            // Draw seam allowance
            QVector<QPointF>points = getSeamAllowancePoints();

            if (points.last().toPoint() != points.first().toPoint())
            {
                points.append(points.at(0));// Should be always closed
            }

            QPainterPath ekv;
            ekv.moveTo(points.at(0));
            for (qint32 i = 1; i < points.count(); ++i)
            {
                ekv.lineTo(points.at(i));
            }

            path.addPath(ekv);
        }
    }

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
QPainterPath VLayoutPiece::createNotchesPath() const
{
    QPainterPath path;

    const QVector<QLineF> notches = getNotches();
    QPainterPath notchesPath;

    for (qint32 i = 0; i < notches.count(); ++i)
    {
        notchesPath.moveTo(notches.at(i).p1());
        notchesPath.lineTo(notches.at(i).p2());
    }

    path.addPath(notchesPath);
    path.setFillRule(Qt::WindingFill);

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createInternalPathItem creates the child item for one internal path of the piece.
 * @param i      index into the internal paths list.
 * @param parent piece root item the path item is added to.
 */
void VLayoutPiece::createInternalPathItem(int i, QGraphicsItem *parent) const
{
    SCASSERT(parent != nullptr)
    QColor       color      = d->m_internalPaths.at(i).getLineColor();
    Qt::PenStyle lineType   = d->m_internalPaths.at(i).getLineType();
    qreal        lineWeight = ToPixel(d->m_internalPaths.at(i).getLineWeight().toDouble(), Unit::Mm);

    QGraphicsPathItem* item = new QGraphicsPathItem(parent);
    item->setData(PieceItemData::ItemType, QStringLiteral("internal_path"));
    item->setPath(d->transform.map(d->m_internalPaths.at(i).GetPainterPath()));
    setTurnPointsData(item, d->m_internalPaths.at(i).turnPoints());
    item->setPen(QPen(color, lineWeight, lineType, Qt::RoundCap, Qt::RoundJoin));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createCutoutPathItem creates the child item for one internal cutout path of the piece.
 * @param i      index into the cutout paths list.
 * @param parent piece root item the path item is added to.
 */
void VLayoutPiece::createCutoutPathItem(int i, QGraphicsItem *parent) const
{
    SCASSERT(parent != nullptr)
    QColor       color      = d->m_cutoutPaths.at(i).getLineColor();
    Qt::PenStyle lineType   = d->m_cutoutPaths.at(i).getLineType();
    qreal        lineWeight = ToPixel(d->m_cutoutPaths.at(i).getLineWeight().toDouble(), Unit::Mm);

    QGraphicsPathItem* item = new QGraphicsPathItem(parent);
    item->setData(PieceItemData::ItemType, QStringLiteral("cut_path"));
    item->setPath(d->transform.map(d->m_cutoutPaths.at(i).GetPainterPath()));
    setTurnPointsData(item, d->m_cutoutPaths.at(i).turnPoints());
    item->setPen(QPen(color, lineWeight, lineType, Qt::RoundCap, Qt::RoundJoin));
}

//---------------------------------------------------------------------------------------------------------------------
QPainterPath VLayoutPiece::LayoutAllowancePath() const
{
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);

    const QVector<QPointF> points = getLayoutAllowancePoints();
    path.moveTo(points.at(0));
    for (qint32 i = 1; i < points.count(); ++i)
    {
        path.lineTo(points.at(i));
    }
    path.lineTo(points.at(0));

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetItem builds the graphics item tree for one pattern piece.
 *
 * The root item is a pure container: it paints nothing itself. Every visual
 * component (seamline, cutline, notches, internal paths, labels, grainline)
 * is created as a direct child tagged with PieceItemData::ItemType so the SVG
 * exporter can render and identify each component independently.
 *
 * @param textAsPaths true to convert label text into vector paths.
 * @return root QGraphicsItem owning all component child items.
 */
QGraphicsItem *VLayoutPiece::GetItem(bool textAsPaths) const
{
    // Empty QGraphicsPathItem: no path and no pen, so the root draws nothing.
    QGraphicsPathItem *item = new QGraphicsPathItem();
    item->setData(PieceItemData::ObjectName, GetName());
    item->setData(PieceItemData::PieceLetter, getPieceLetter());
    item->setData(PieceItemData::Quantity, getQuantity());
    item->setData(PieceItemData::OnFold, isOnFold());
    item->setData(PieceItemData::Material, getMaterial());

    // Piece Properties a layout or a cutter may use; a value that allows nothing sets no data.
    const VPieceLabelData &properties = d->m_pieceProperties;
    const int rotationStep = VPieceLabelData::rotationStepDegrees(properties.getRotationWay());
    if (rotationStep > 0)
    {
        item->setData(PieceItemData::Rotation, rotationStep);
    }
    if (properties.GetTilt() != PieceTilt::None && properties.getTiltDegrees() > 0)
    {
        const int sign = properties.GetTilt() == PieceTilt::Clockwise ? -1 : 1;
        item->setData(PieceItemData::Tilt, sign * properties.getTiltDegrees());
    }
    if (getFlip() != PieceFlip::None)
    {
        item->setData(PieceItemData::Flip, pieceFlipToString(getFlip()));
    }
    // The piece dialog stores Left, Right or Undefined.
    const QString orientation = properties.GetOrientation();
    if (orientation == QLatin1String("Left") || orientation == QLatin1String("Right"))
    {
        item->setData(PieceItemData::Orientation, orientation);
    }
    if (!properties.GetAnnotation().isEmpty())
    {
        item->setData(PieceItemData::Annotation, properties.GetAnnotation());
    }

    qreal grainAngle = 0;
    if (grainlineAngle(grainAngle))
    {
        item->setData(PieceItemData::GrainlineAngle, grainAngle);
    }

    // Child components, one taggable item (or item group) each.
    createSeamlineItem(item);
    createAllowanceItem(item);
    createNotchesItem(item);
    createFoldLineItem(item, textAsPaths);

    for (int i = 0; i < d->m_internalPaths.count(); ++i)
    {
        createInternalPathItem(i, item);
    }

    for (int i = 0; i < d->m_cutoutPaths.count(); ++i)
    {
        createCutoutPathItem(i, item);
    }

    createLabelItem(item, d->pieceLabel, d->m_tmPiece, QStringLiteral("piece_label"), textAsPaths);
    createLabelItem(item, d->patternInfo, d->m_tmPattern, QStringLiteral("pattern_label"), textAsPaths);
    createGrainlineItem(item, textAsPaths);

    return item;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createLabelItem creates the graphics items for one label (piece or pattern label).
 *
 * All text lines of the label are parented to one invisible group item tagged
 * with the given SVG data-type, so the SVG exporter can render the whole label
 * as a single identifiable component.
 *
 * Each line keeps its font (family, size, per-line bold/italic), color,
 * alignment, middle-eliding to the label width, mirroring and rotation. The
 * line is emitted either as an explicit vector path (textAsPaths == true,
 * QGraphicsPathItem holding the glyph outlines) or as real text
 * (textAsPaths == false, SvgTextItem) so that SVG/PDF/DXF exports receive
 * text draw calls and can output actual text elements.
 *
 * @param parent      piece root item the label group is added to.
 * @param labelShape  label rectangle corners; fewer than 3 points means no label.
 * @param tm          text manager holding the label's text lines.
 * @param type        SVG data-type tag: "piece_label" or "pattern_label".
 * @param textAsPaths true to convert the text into vector paths.
 */
void VLayoutPiece::createLabelItem(QGraphicsItem *parent, const QVector<QPointF> &labelShape,
                                      const LabelTextManager &tm, const QString &type, bool textAsPaths) const
{
    SCASSERT(parent != nullptr)
    QColor color = QColor(qApp->Settings()->getDefaultLabelColor());

    if (labelShape.count() > 2)
    {
        // Invisible group item: every text line below becomes one SVG label component.
        QGraphicsPathItem *labelGroup = new QGraphicsPathItem(parent);
        labelGroup->setData(PieceItemData::ObjectName, QStringLiteral("label"));
        labelGroup->setData(PieceItemData::ItemType, type);

        const qreal dW = QLineF(labelShape.at(0), labelShape.at(1)).length();
        const qreal dH = QLineF(labelShape.at(1), labelShape.at(2)).length();
        const qreal angle = - QLineF(labelShape.at(0), labelShape.at(1)).angle();
        qreal dY = 0;
        qreal dX = 0;

        for (int i = 0; i < tm.GetSourceLinesCount(); ++i)
        {
            const TextLine& tl = tm.GetSourceLine(i);
            QFont fnt = tm.GetFont();
            fnt.setPixelSize(tm.GetFont().pixelSize() + tl.font_size_increment);
            fnt.setBold(tl.bold);
            fnt.setItalic(tl.italic);

            QFontMetrics fm(fnt);

            if (textAsPaths)
            {
                dY += fm.height();
            }

            if (dY > dH)
            {
                break;
            }

            QString qsText = tl.text;
            if (fm.horizontalAdvance(qsText) > dW)
            {
                qsText = fm.elidedText(qsText, Qt::ElideMiddle, static_cast<int>(dW));
            }
            if ((tl.alignment & Qt::AlignLeft) > 0)
            {
                dX = 0;
            }
            else if ((tl.alignment & Qt::AlignHCenter) > 0)
            {
                dX = (dW - fm.horizontalAdvance(qsText))/2;
            }
            else
            {
                dX = dW - fm.horizontalAdvance(qsText);
            }

            // set up the rotation around top-left corner matrix
            QTransform labelTransform;
            labelTransform.translate(labelShape.at(0).x(), labelShape.at(0).y());
            if (d->mirror)
            {
                labelTransform.scale(-1, 1);
                labelTransform.rotate(-angle);
                labelTransform.translate(-dW, 0);
                labelTransform.translate(dX, dY); // Each string has own position
            }
            else
            {
                labelTransform.rotate(angle);
                labelTransform.translate(dX, dY); // Each string has own position
            }

            labelTransform *= d->transform;

            if (textAsPaths)
            {
                QPainterPath path;
                path.addText(0, - static_cast<qreal>(fm.ascent())/6., fnt, qsText);

                // Text line as vector path, parented to the label group.
                QGraphicsPathItem* item = new QGraphicsPathItem(labelGroup);
                item->setData(PieceItemData::ObjectName, QString("label"));
                item->setPath(path);
                item->setPen(QPen(color, widthHairLine));
                item->setBrush(QBrush(Qt::NoBrush));
                item->setTransform(labelTransform);

                dY += tm.GetSpacing();
            }
            else
            {
                // Text line as real text, parented to the label group. SvgTextItem
                // paints through QPainter::drawText(), so text-capable export
                // engines receive real text draw calls: SVG exports emit <text>
                // elements instead of glyph outlines, PDF keeps searchable text
                // and DXF gets TEXT entities. The fill color comes from the
                // brush; no pen is set because an outline pen is exactly what
                // forces Qt to convert text to filled glyph outlines.
                SvgTextItem* item = new SvgTextItem(labelGroup);
                item->setData(PieceItemData::ObjectName, QString("label"));
                item->setFont(fnt);
                item->setText(qsText);
                item->setTransform(labelTransform);
                item->setBrush(QBrush(color));

                dY += (fm.height() + tm.GetSpacing());
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createFoldLineItem creates the fold line child item of the piece.
 *
 * An invisible group item tagged "fold_line" holds, in this order: the fold line, a two point path; the arrow line
 * group tagged "fold_line_arrow"; the label group tagged "fold_line_label". The fold line comes first, because
 * readers take the first path of the group as the fold line. A piece without a fold line gets no item.
 *
 * @param parent        piece root item the fold line item is added to.
 * @param text_as_paths true when the export renders text as vector paths.
 */
void VLayoutPiece::createFoldLineItem(QGraphicsItem* parent, bool text_as_paths) const
{
    SCASSERT(parent != nullptr)
    const QVector<QPointF> points = getFoldLine();
    const pattern_piece::FoldLineGraphic graphic = getFoldLineGraphic();

    const QColor  color       = QColor(qApp->Settings()->getDefaultFoldLineColor());
    const QString line_type   = qApp->Settings()->getDefaultFoldLineLinetype();
    const qreal   line_weight = ToPixel(qApp->Settings()->getDefaultFoldLineLineweight(), Unit::Mm);

    if (points.size() == 2)
    {
        // Group item: it draws nothing itself.
        QGraphicsPathItem* group = new QGraphicsPathItem(parent);
        group->setData(PieceItemData::ItemType, QStringLiteral("fold_line"));

        QPainterPath fold_path;
        fold_path.moveTo(points.at(0));
        fold_path.lineTo(points.at(1));
        QGraphicsPathItem* fold = new QGraphicsPathItem(group);
        fold->setPath(fold_path);
        fold->setPen(QPen(color, line_weight, lineTypeToPenStyle(line_type), Qt::FlatCap, Qt::RoundJoin));

        if (!graphic.isNull())
        {
            createFoldLineArrowItem(group, graphic);
            createFoldLineLabelItem(group, graphic, text_as_paths);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createFoldLineArrowItem creates the fold line arrow line, in the fold line style, with filled arrow heads.
 * @param parent  fold line group item; the arrow item is tagged "fold_line_arrow".
 * @param graphic fold line graphic in layout coordinates.
 */
void VLayoutPiece::createFoldLineArrowItem(QGraphicsItem* parent, const pattern_piece::FoldLineGraphic& graphic) const
{
    const QColor  color       = QColor(qApp->Settings()->getDefaultFoldLineColor());
    const QString line_type   = qApp->Settings()->getDefaultFoldLineLinetype();
    const qreal   line_weight = ToPixel(qApp->Settings()->getDefaultFoldLineLineweight(), Unit::Mm);

    QGraphicsPathItem* arrow = new QGraphicsPathItem(parent);
    arrow->setData(PieceItemData::ItemType, QStringLiteral("fold_line_arrow"));

    // Arrow line from head base to head base.
    QPainterPath line_path;
    line_path.addPolygon(QPolygonF(graphic.arrowLine()));
    QGraphicsPathItem* line = new QGraphicsPathItem(arrow);
    line->setPath(line_path);
    line->setPen(QPen(color, line_weight, lineTypeToPenStyle(line_type), Qt::FlatCap, Qt::MiterJoin));

    // Each head is a closed triangle, filled with the fold line color.
    QPainterPath heads_path;
    heads_path.addPolygon(QPolygonF(graphic.startArrowHead()));
    heads_path.closeSubpath();
    heads_path.addPolygon(QPolygonF(graphic.endArrowHead()));
    heads_path.closeSubpath();
    QGraphicsPathItem* heads = new QGraphicsPathItem(arrow);
    heads->setPath(heads_path);
    heads->setPen(Qt::NoPen);
    heads->setBrush(QBrush(color));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createFoldLineLabelItem creates the "Foldline" label, centered between the fold line and the arrow line.
 * @param parent        fold line group item; the label item is tagged "fold_line_label".
 * @param graphic       fold line graphic in layout coordinates.
 * @param text_as_paths true for glyph outlines, false for a text item that exports as a real <text> element.
 */
void VLayoutPiece::createFoldLineLabelItem(QGraphicsItem* parent, const pattern_piece::FoldLineGraphic& graphic,
                                           bool text_as_paths) const
{
    const QColor  color = QColor(qApp->Settings()->getDefaultFoldLineColor());
    const QFont   font  = graphic.labelFont(qApp->Settings()->getLabelFont());
    const QString text  = pattern_piece::FoldLineGraphic::labelText();

    QGraphicsPathItem* label = new QGraphicsPathItem(parent);
    label->setData(PieceItemData::ItemType, QStringLiteral("fold_line_label"));

    if (text_as_paths)
    {
        // Glyph outlines with their top left corner at (0, 0), like the text item below.
        const QFontMetricsF metrics(font);
        QPainterPath text_path;
        text_path.addText(0, metrics.ascent(), font, text);
        QGraphicsPathItem* outline = new QGraphicsPathItem(label);
        outline->setPath(text_path);
        outline->setPen(Qt::NoPen);
        outline->setBrush(QBrush(color));
        outline->setTransform(graphic.labelTransform(QSizeF(metrics.horizontalAdvance(text), metrics.height())));
    }
    else
    {
        SvgTextItem* line = new SvgTextItem(label);
        line->setFont(font);
        line->setText(text);
        line->setBrush(QBrush(color));
        line->setTransform(graphic.labelTransform(line->boundingRect().size()));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createGrainlineItem creates the grainline child item of the piece.
 * @param parent      piece root item the grainline item is added to.
 * @param textAsPaths true when the export renders text as vector paths.
 */
void VLayoutPiece::createGrainlineItem(QGraphicsItem *parent, bool textAsPaths) const
{
    SCASSERT(parent != nullptr)
    QColor color = QColor(qApp->Settings()->getDefaultGrainlineColor());

    if (d->grainlinePoints.count() < 2)
    {
        return;
    }
    VGraphicsFillItem* item = new VGraphicsFillItem(color, textAsPaths, parent);
    item->setData(PieceItemData::ObjectName, QString("grainline"));
    item->setData(PieceItemData::ItemType, QStringLiteral("grainline"));

    QPainterPath path;

    QVector<QPointF> gPoints = getGrainline();
    path.moveTo(gPoints.at(0));
    for (int i = 1; i < gPoints.count(); ++i)
    {
        path.lineTo(gPoints.at(i));
    }
    item->setPath(path);
}

//---------------------------------------------------------------------------------------------------------------------
QVector<QPointF> VLayoutPiece::piecePath() const
{
    if (hasSeamAllowance() && not hasSeamAllowanceBuiltIn())
    {
        return d->seamAllowance;
    }
    else
    {
        return d->contour;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createSeamlineItem creates the seamline child item of a piece.
 *
 * Replaces the former createMainItem(): the seamline is now an ordinary child
 * of the piece container so the SVG exporter can render it independently.
 * The seamline is the main path, or the line inside a main path with a
 * built-in seam allowance. A hidden seam line draws nothing in both cases.
 *
 * @param parent piece root item the seamline item is added to.
 */
void VLayoutPiece::createSeamlineItem(QGraphicsItem *parent) const
{
    SCASSERT(parent != nullptr)
    QColor  color;
    QString lineType;
    qreal   lineWeight;
    if (hasSeamAllowance())
    {
        color      = QColor(qApp->Settings()->getDefaultSeamColor());
        lineType   = qApp->Settings()->getDefaultSeamLinetype();
        lineWeight = ToPixel(qApp->Settings()->getDefaultSeamLineweight(), Unit::Mm);
    }
    else
    {
        color      = QColor(qApp->Settings()->getDefaultCutColor());
        lineType   = qApp->Settings()->getDefaultCutLinetype();
        lineWeight = ToPixel(qApp->Settings()->getDefaultCutLineweight(), Unit::Mm);
    }
    QGraphicsPathItem *item = new QGraphicsPathItem(parent);
    item->setData(PieceItemData::ObjectName, QString("seamline"));
    item->setData(PieceItemData::ItemType, QStringLiteral("seamline"));
    if (hasSeamAllowance() && hasSeamAllowanceBuiltIn())
    {
        // A seam line that a fold line cuts open ends on the fold line; it is not closed along it.
        const QVector<QPointF> seamLine = getBuiltInSeamLinePoints();
        QPainterPath path;
        if (!isHideSeamLine())
        {
            path = d->builtInSeamLineClosed ? closedPath(seamLine) : openPath(seamLine);
        }
        item->setPath(path);
        setTurnPointsData(item, d->builtInSeamLineTurnPoints);
    }
    else
    {
        item->setPath(createMainPath());
        setTurnPointsData(item, d->contourTurnPoints);
    }
    item->setPen(QPen(color, lineWeight, lineTypeToPenStyle(lineType), Qt::RoundCap, Qt::RoundJoin));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createAllowanceItem creates the seam allowance (cutline) child item of the piece.
 *
 * A built-in seam allowance makes the main path the cutline. It is the piece
 * outline, so it is created whatever the seam allowance view setting.
 *
 * @param parent piece root item the cutline item is added to.
 */
void VLayoutPiece::createAllowanceItem(QGraphicsItem *parent) const
{
    const bool builtIn = hasSeamAllowance() && hasSeamAllowanceBuiltIn();
    if (builtIn || qApp->Settings()->showSeamAllowances())
    {
        QColor  color      = QColor(qApp->Settings()->getDefaultCutColor());
        QString lineType   = qApp->Settings()->getDefaultCutLinetype();
        qreal   lineWeight = ToPixel(qApp->Settings()->getDefaultCutLineweight(), Unit::Mm);

        QGraphicsPathItem *item = new QGraphicsPathItem(parent);
        item->setData(PieceItemData::ObjectName, QString("cutline"));
        item->setData(PieceItemData::ItemType, QStringLiteral("cutline"));
        item->setPath(builtIn ? createMainPath() : createAllowancePath());
        setTurnPointsData(item, builtIn ? d->contourTurnPoints : d->seamAllowanceTurnPoints);
        item->setPen(QPen(color, lineWeight, lineTypeToPenStyle(lineType), Qt::RoundCap, Qt::RoundJoin));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief createNotchesItem creates the child item holding all notches of the piece.
 * @param parent piece root item the notches item is added to.
 */
void VLayoutPiece::createNotchesItem(QGraphicsItem *parent) const
{
    if (getNotches().isEmpty())
    {
        return;
    }

    QColor color      = QColor(qApp->Settings()->getDefaultNotchColor());
    qreal  lineWeight = ToPixel(qApp->Settings()->getDefaultCutLineweight(), Unit::Mm);

    QGraphicsPathItem *item = new QGraphicsPathItem(parent);
    item->setData(PieceItemData::ObjectName, QString("notches"));
    item->setData(PieceItemData::ItemType, QStringLiteral("notch"));
    item->setPath(createNotchesPath());

    // One line count per notch, in the order createNotchesPath() draws the lines.
    if (!d->notchLineCounts.isEmpty())
    {
        QStringList counts;
        for (int count : d->notchLineCounts)
        {
            counts.append(QString::number(count));
        }
        item->setData(PieceItemData::NotchSegments, counts.join(QLatin1Char(' ')));
    }
    // One width per notch, in the same order.
    if (!d->notchSeamAllowances.isEmpty())
    {
        QStringList widths;
        for (qreal width : d->notchSeamAllowances)
        {
            widths.append(QString::number(FromPixel(width, Unit::Mm), 'f', 2));
        }
        item->setData(PieceItemData::SeamAllowances, widths.join(QLatin1Char(' ')));
    }
    // One type per notch, in the same order.
    if (!d->notchTypes.isEmpty())
    {
        item->setData(PieceItemData::NotchTypes, d->notchTypes.join(QLatin1Char(' ')));
    }
    item->setPen(QPen(color, lineWeight, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

//---------------------------------------------------------------------------------------------------------------------
QGraphicsPathItem *VLayoutPiece::getMainPathItem() const
{
    QGraphicsPathItem *item = new QGraphicsPathItem();

    QPainterPath path;

    // contour
    QVector<QPointF> points = getContourPoints();

    path.moveTo(points.at(0));
    for (qint32 i = 1; i < points.count(); ++i)
    {
        path.lineTo(points.at(i));
    }
    path.lineTo(points.at(0));

    item->setPath(path);
    return item;
}

//---------------------------------------------------------------------------------------------------------------------
bool VLayoutPiece::isMirror() const
{
    return d->mirror;
}

//---------------------------------------------------------------------------------------------------------------------
void VLayoutPiece::setMirror (bool value)
{
    d->mirror = value;
}

//---------------------------------------------------------------------------------------------------------------------
QLineF VLayoutPiece::Edge(const QVector<QPointF> &path, int i) const
{
    if (i < 1 || i > path.count())
    { // Doesn't exist such edge
        return QLineF();
    }

    int i1, i2;
    if (i < path.count())
    {
        i1 = i-1;
        i2 = i;
    }
    else
    {
        i1 = path.count()-1;
        i2 = 0;
    }

    if (d->mirror)
    {
        const int oldI1 = i1;
        const int size = path.size()-1; //-V807
        i1 = size - i2;
        i2 = size - oldI1;
        return QLineF(d->transform.map(path.at(i2)), d->transform.map(path.at(i1)));
    }
    else
    {
        return QLineF(d->transform.map(path.at(i1)), d->transform.map(path.at(i2)));
    }
}

//---------------------------------------------------------------------------------------------------------------------
int VLayoutPiece::EdgeByPoint(const QVector<QPointF> &path, const QPointF &p1) const
{
    if (p1.isNull())
    {
        return 0;
    }

    if (path.count() < 3)
    {
        return 0;
    }

    const QVector<QPointF> points = Map(path);
    for (int i=0; i < points.size(); i++)
    {
        if (points.at(i) == p1)
        {
            return i+1;
        }
    }
    return 0; // Did not find edge
}
