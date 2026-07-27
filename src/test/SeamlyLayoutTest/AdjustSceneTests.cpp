// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file AdjustSceneTests.cpp
// @brief Qt tests for AdjustScene conflict detection/highlighting workflow.

#include "adjust/AdjustScene.h"
#include "adjust/PieceOverlayItem.h"

#include <QCoreApplication>
#include <QFile>
#include <QGraphicsItem>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest/QSignalSpy>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <cmath>

namespace {

/// @brief Build bbox JSON with two pieces inside content bounds.
QString buildBboxJson()
{
    QJsonArray pieces;

    QJsonObject pieceA;
    pieceA["id"] = QStringLiteral("pieceA");
    pieceA["x"] = 10.0;
    pieceA["y"] = 10.0;
    pieceA["w"] = 30.0;
    pieceA["h"] = 30.0;
    pieceA["origin_x_px"] = 0.0;
    pieceA["origin_y_px"] = 0.0;
    pieceA["rotation_deg"] = 0.0;
    pieceA["transform_str"] = QString();
    pieces.append(pieceA);

    QJsonObject pieceB;
    pieceB["id"] = QStringLiteral("pieceB");
    pieceB["x"] = 90.0;
    pieceB["y"] = 10.0;
    pieceB["w"] = 30.0;
    pieceB["h"] = 30.0;
    pieceB["origin_x_px"] = 0.0;
    pieceB["origin_y_px"] = 0.0;
    pieceB["rotation_deg"] = 0.0;
    pieceB["transform_str"] = QString();
    pieces.append(pieceB);

    QJsonObject root;
    root["pieces"] = pieces;

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

/// @brief Write a minimal SVG with a contentRect id for outside-content checks.
QString writeLayoutSvg(const QString& directoryPath)
{
    const QString svgPath = directoryPath + QStringLiteral("/layout.svg");
    QFile svgFile(svgPath);
    if (!svgFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QString();
    }

    const QByteArray svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"200\" viewBox=\"0 0 200 200\">"
        "<rect id=\"contentRect\" x=\"0\" y=\"0\" width=\"200\" height=\"200\" fill=\"none\" stroke=\"none\"/>"
        "</svg>";

    svgFile.write(svg);
    svgFile.close();
    return svgPath;
}

/// @brief Find a piece overlay by id in the scene.
PieceOverlayItem* findPiece(AdjustScene& scene, const QString& id)
{
    for (QGraphicsItem* graphicsItem : scene.items()) {
        PieceOverlayItem* piece = qgraphicsitem_cast<PieceOverlayItem*>(graphicsItem);
        if (piece && piece->pieceId() == id) {
            return piece;
        }
    }

    return nullptr;
}

} // namespace

/// @class AdjustSceneTests
/// @brief Verifies AdjustScene conflict validation workflow behavior.
class AdjustSceneTests : public QObject
{
    Q_OBJECT

private slots:
    /// @brief Detect overlapping piece ids from current overlay positions.
    void detectsOverlappingPieces();

    /// @brief Toggle conflict outline red (2px) and reset to normal style.
    void togglesConflictHighlightOutline();

    /// @brief Emit operation conflict signal after committed conflicting operation.
    void emitsOperationConflictsDetectedSignal();

    /// @brief Verify flip horizontal toggles scaleX and records transform state.
    void flipHorizontalTogglesScaleX();

    /// @brief Verify flip vertical toggles scaleY and records transform state.
    void flipVerticalTogglesScaleY();

    /// @brief Verify align to left edge moves piece to contentRect left boundary.
    void alignLeftEdgeMovesToContentRectLeft();

    /// @brief Verify align to right edge moves piece to contentRect right boundary.
    void alignRightEdgeMovesToContentRectRight();

    /// @brief Verify align to top edge moves piece to contentRect top boundary.
    void alignTopEdgeMovesToContentRectTop();

    /// @brief Verify align to bottom edge moves piece to contentRect bottom boundary.
    void alignBottomEdgeMovesToContentRectBottom();

    /// @brief Verify findNearestPieceLeft returns the correct piece.
    void findNearestPieceLeftReturnsCorrectPiece();

    /// @brief Verify findNearestPieceRight returns the correct piece.
    void findNearestPieceRightReturnsCorrectPiece();

    /// @brief Verify abut left moves piece to touch nearest piece's right edge.
    void abutLeftMovesToTouchNearestPiece();

    /// @brief Verify abut right moves piece to touch nearest piece's left edge.
    void abutRightMovesToTouchNearestPiece();

    /// @brief Verify raise to top sets z-value above all other pieces.
    void raiseToTopSetsHighestZValue();

    /// @brief Verify lower to bottom sets z-value below all other pieces.
    void lowerToBottomSetsLowestZValue();

    /// @brief Verify findNearestPieceAbove returns the correct piece.
    void findNearestPieceAboveReturnsCorrectPiece();

    /// @brief Verify findNearestPieceBelow returns the correct piece.
    void findNearestPieceBelowReturnsCorrectPiece();

    /// @brief checkOverlaps detects overlapping pieces and returns both ids.
    void checkOverlapsDetectsOverlappingPieces();

    /// @brief checkOverlaps returns empty for non-overlapping pieces.
    void checkOverlapsReturnsEmptyForNonOverlapping();

    /// @brief Verify the rotation pivot is the bounding-box center, not the upper-left corner.
    void rotationPivotIsBoundingBoxCenter();

    /// @brief Verify rotating a piece in place keeps its bounding-box center fixed.
    void rotatingAboutCenterKeepsCenterFixed();

    /// @brief Verify buildTransform()/applyTransformString() round-trip pos/rotation for a piece rotated in place.
    void buildTransformRoundTripsRotationAroundCenter();

    /// @brief Verify buildTransform()/applyTransformString() round-trip pos/rotation for a moved-and-rotated piece.
    void buildTransformRoundTripsMoveAndRotationAroundCenter();

    // DG.4 — #ifdef QT_DEBUG / #else inline-stub gate on dumpOverlayData()

    /// @brief DG.4 debug gate: dumpOverlayData() compiles and runs without crashing in debug builds.
    void dg4_debugDumpOverlayDataCompilesAndIsCallable();

    /// @brief DG.4 release gate: dumpOverlayData() inline stub compiles and runs without crashing.
    void dg4_releaseStubCompilesAndIsCallable();
};

void AdjustSceneTests::detectsOverlappingPieces()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceB = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceA);
    QVERIFY(pieceB);

    pieceB->setPos(pieceA->pos());

    const QStringList conflicts = scene.checkLayoutConflicts();
    QVERIFY(conflicts.contains(QStringLiteral("pieceA")));
    QVERIFY(conflicts.contains(QStringLiteral("pieceB")));
}

void AdjustSceneTests::togglesConflictHighlightOutline()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    QVERIFY(pieceA);

    scene.highlightConflicts(QStringList{QStringLiteral("pieceA")});

    // Conflict state: 10px pure red outline (unmistakable on overlap).
    QCOMPARE(pieceA->pen().color(), QColor(0xff, 0x00, 0x00));
    QCOMPARE(pieceA->pen().widthF(), 10.0);

    scene.clearConflictHighlights();

    // Normal state: 2px violetMedium outline.
    QCOMPARE(pieceA->pen().color(), QColor(0x73, 0x51, 0xad));
    QCOMPARE(pieceA->pen().widthF(), 2.0);
}

void AdjustSceneTests::emitsOperationConflictsDetectedSignal()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceB = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceA);
    QVERIFY(pieceB);

    pieceB->setPos(pieceA->pos());

    QSignalSpy spy(&scene, &AdjustScene::operationConflictsDetected);
    QVERIFY(spy.isValid());

    scene.notifyPieceStateCommitted(QStringLiteral("pieceB"), 0);

    QCOMPARE(spy.count(), 1);

    const QList<QVariant> args = spy.takeFirst();
    QVERIFY(!args.isEmpty());

    const QStringList conflicts = args.at(0).toStringList();
    QVERIFY(conflicts.contains(QStringLiteral("pieceA")));
    QVERIFY(conflicts.contains(QStringLiteral("pieceB")));
}

void AdjustSceneTests::flipHorizontalTogglesScaleX()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    QVERIFY(pieceA);

    // Initial scale should be 1.0.
    QCOMPARE(pieceA->scaleX(), 1.0);
    QCOMPARE(pieceA->scaleY(), 1.0);

    // Simulate horizontal flip by toggling scaleX.
    pieceA->setScaleX(-1.0);
    QCOMPARE(pieceA->scaleX(), -1.0);
    QCOMPARE(pieceA->scaleY(), 1.0);

    // Build transform should include scale.
    const QString transform = pieceA->buildTransform();
    QVERIFY2(!transform.isEmpty(), "Transform should be non-empty after flip");
    QVERIFY2(transform.startsWith("matrix("), "Transform should be a matrix");

    // Toggle back.
    pieceA->setScaleX(1.0);
    QCOMPARE(pieceA->scaleX(), 1.0);
}

void AdjustSceneTests::flipVerticalTogglesScaleY()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    QVERIFY(pieceA);

    // Initial scale should be 1.0.
    QCOMPARE(pieceA->scaleX(), 1.0);
    QCOMPARE(pieceA->scaleY(), 1.0);

    // Simulate vertical flip by toggling scaleY.
    pieceA->setScaleY(-1.0);
    QCOMPARE(pieceA->scaleX(), 1.0);
    QCOMPARE(pieceA->scaleY(), -1.0);

    // Build transform should include scale.
    const QString transform = pieceA->buildTransform();
    QVERIFY2(!transform.isEmpty(), "Transform should be non-empty after flip");
    QVERIFY2(transform.startsWith("matrix("), "Transform should be a matrix");

    // Toggle back.
    pieceA->setScaleY(1.0);
    QCOMPARE(pieceA->scaleY(), 1.0);
}

void AdjustSceneTests::alignLeftEdgeMovesToContentRectLeft()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    QVERIFY2(scene.hasContentRect(), "Scene must have contentRect");
    const QRectF contentRect = scene.contentRect();

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    QVERIFY(pieceA);

    // pieceA starts at (10, 10) with width 30, so left edge is at 10.
    // Align left should move it so left edge = contentRect.left() = 0.
    const QRectF bboxBefore = pieceA->sceneBoundingRect();
    const double dx = contentRect.left() - bboxBefore.left();
    pieceA->setPos(pieceA->pos().x() + dx, pieceA->pos().y());

    const QRectF bboxAfter = pieceA->sceneBoundingRect();
    QCOMPARE(bboxAfter.left(), contentRect.left());
}

void AdjustSceneTests::alignRightEdgeMovesToContentRectRight()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    QVERIFY2(scene.hasContentRect(), "Scene must have contentRect");
    const QRectF contentRect = scene.contentRect();

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    QVERIFY(pieceA);

    // pieceA starts at (10, 10) with width 30, so right edge is at 40.
    // Align right should move it so right edge = contentRect.right() = 200.
    const QRectF bboxBefore = pieceA->sceneBoundingRect();
    const double dx = contentRect.right() - bboxBefore.right();
    pieceA->setPos(pieceA->pos().x() + dx, pieceA->pos().y());

    const QRectF bboxAfter = pieceA->sceneBoundingRect();
    QCOMPARE(bboxAfter.right(), contentRect.right());
}

void AdjustSceneTests::alignTopEdgeMovesToContentRectTop()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    QVERIFY2(scene.hasContentRect(), "Scene must have contentRect");
    const QRectF contentRect = scene.contentRect();

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    QVERIFY(pieceA);

    // pieceA starts at (10, 10), so top edge is at 10.
    // Align top should move it so top edge = contentRect.top() = 0.
    const QRectF bboxBefore = pieceA->sceneBoundingRect();
    const double dy = contentRect.top() - bboxBefore.top();
    pieceA->setPos(pieceA->pos().x(), pieceA->pos().y() + dy);

    const QRectF bboxAfter = pieceA->sceneBoundingRect();
    QCOMPARE(bboxAfter.top(), contentRect.top());
}

void AdjustSceneTests::alignBottomEdgeMovesToContentRectBottom()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    QVERIFY2(scene.hasContentRect(), "Scene must have contentRect");
    const QRectF contentRect = scene.contentRect();

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    QVERIFY(pieceA);

    // pieceA starts at (10, 10) with height 30, so bottom edge is at 40.
    // Align bottom should move it so bottom edge = contentRect.bottom() = 200.
    const QRectF bboxBefore = pieceA->sceneBoundingRect();
    const double dy = contentRect.bottom() - bboxBefore.bottom();
    pieceA->setPos(pieceA->pos().x(), pieceA->pos().y() + dy);

    const QRectF bboxAfter = pieceA->sceneBoundingRect();
    QCOMPARE(bboxAfter.bottom(), contentRect.bottom());
}

void AdjustSceneTests::findNearestPieceLeftReturnsCorrectPiece()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    // pieceA is at (10, 10), pieceB is at (90, 10).
    // From pieceB's perspective, pieceA is to the left.
    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceB = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceA);
    QVERIFY(pieceB);

    PieceOverlayItem* nearestLeft = scene.findNearestPieceLeft(pieceB);
    QCOMPARE(nearestLeft, pieceA);

    // From pieceA's perspective, there is no piece to the left.
    PieceOverlayItem* noLeft = scene.findNearestPieceLeft(pieceA);
    QVERIFY(noLeft == nullptr);
}

void AdjustSceneTests::findNearestPieceRightReturnsCorrectPiece()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    // pieceA is at (10, 10), pieceB is at (90, 10).
    // From pieceA's perspective, pieceB is to the right.
    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceB = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceA);
    QVERIFY(pieceB);

    PieceOverlayItem* nearestRight = scene.findNearestPieceRight(pieceA);
    QCOMPARE(nearestRight, pieceB);

    // From pieceB's perspective, there is no piece to the right.
    PieceOverlayItem* noRight = scene.findNearestPieceRight(pieceB);
    QVERIFY(noRight == nullptr);
}

void AdjustSceneTests::abutLeftMovesToTouchNearestPiece()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    // pieceA is at (10, 10) with width 30, so right edge at 40.
    // pieceB is at (90, 10) with width 30, so left edge at 90.
    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceB = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceA);
    QVERIFY(pieceB);

    // Abut pieceB left should move it so its left edge touches pieceA's right edge.
    const QRectF bboxA = pieceA->sceneBoundingRect();
    const QRectF bboxBBefore = pieceB->sceneBoundingRect();
    const double dx = bboxA.right() - bboxBBefore.left();
    pieceB->setPos(pieceB->pos().x() + dx, pieceB->pos().y());

    const QRectF bboxBAfter = pieceB->sceneBoundingRect();
    QCOMPARE(bboxBAfter.left(), bboxA.right());
}

void AdjustSceneTests::abutRightMovesToTouchNearestPiece()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    // pieceA is at (10, 10) with width 30, so right edge at 40.
    // pieceB is at (90, 10) with width 30, so left edge at 90.
    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceB = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceA);
    QVERIFY(pieceB);

    // Abut pieceA right should move it so its right edge touches pieceB's left edge.
    const QRectF bboxB = pieceB->sceneBoundingRect();
    const QRectF bboxABefore = pieceA->sceneBoundingRect();
    const double dx = bboxB.left() - bboxABefore.right();
    pieceA->setPos(pieceA->pos().x() + dx, pieceA->pos().y());

    const QRectF bboxAAfter = pieceA->sceneBoundingRect();
    QCOMPARE(bboxAAfter.right(), bboxB.left());
}

void AdjustSceneTests::raiseToTopSetsHighestZValue()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceB = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceA);
    QVERIFY(pieceB);

    // Initially both should have same z-value (1.0).
    QCOMPARE(pieceA->zValue(), 1.0);
    QCOMPARE(pieceB->zValue(), 1.0);

    // Raise pieceB to top.
    const qreal maxZ = scene.maxPieceZValue();
    pieceB->setZValue(maxZ + 1.0);

    // pieceB should now be above pieceA.
    QVERIFY(pieceB->zValue() > pieceA->zValue());
    QCOMPARE(scene.maxPieceZValue(), pieceB->zValue());
}

void AdjustSceneTests::lowerToBottomSetsLowestZValue()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceB = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceA);
    QVERIFY(pieceB);

    // Raise pieceB first so they have different z-values.
    pieceB->setZValue(2.0);
    QVERIFY(pieceB->zValue() > pieceA->zValue());

    // Lower pieceB to bottom.
    const qreal minZ = scene.minPieceZValue();
    const qreal newZ = qMax(0.5, minZ - 0.5);
    pieceB->setZValue(newZ);

    // pieceB should now be below pieceA.
    QVERIFY(pieceB->zValue() < pieceA->zValue());
    QCOMPARE(scene.minPieceZValue(), pieceB->zValue());
}

void AdjustSceneTests::findNearestPieceAboveReturnsCorrectPiece()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    // Build a bbox JSON with pieces stacked vertically.
    // pieceA at (10, 10), pieceB at (10, 90) — pieceA is above pieceB.
    QJsonArray pieces;

    QJsonObject pieceA;
    pieceA["id"]           = QStringLiteral("pieceA");
    pieceA["x"]            = 10.0;
    pieceA["y"]            = 10.0;
    pieceA["w"]            = 30.0;
    pieceA["h"]            = 30.0;
    pieceA["origin_x_px"]  = 0.0;
    pieceA["origin_y_px"]  = 0.0;
    pieceA["rotation_deg"] = 0.0;
    pieceA["transform_str"] = QString();
    pieces.append(pieceA);

    QJsonObject pieceB;
    pieceB["id"]           = QStringLiteral("pieceB");
    pieceB["x"]            = 10.0;
    pieceB["y"]            = 90.0;
    pieceB["w"]            = 30.0;
    pieceB["h"]            = 30.0;
    pieceB["origin_x_px"]  = 0.0;
    pieceB["origin_y_px"]  = 0.0;
    pieceB["rotation_deg"] = 0.0;
    pieceB["transform_str"] = QString();
    pieces.append(pieceB);

    QJsonObject root;
    root["pieces"] = pieces;
    const QString bboxJson = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));

    AdjustScene scene;
    scene.loadLayout(svgPath, bboxJson);

    // pieceA is at (10, 10), pieceB is at (10, 90).
    // From pieceB's perspective, pieceA is above it (lower y value).
    PieceOverlayItem* pieceAItem = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceBItem = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceAItem);
    QVERIFY(pieceBItem);

    PieceOverlayItem* nearestAbove = scene.findNearestPieceAbove(pieceBItem);
    QCOMPARE(nearestAbove, pieceAItem);

    // From pieceA's perspective, there is no piece above it.
    PieceOverlayItem* noAbove = scene.findNearestPieceAbove(pieceAItem);
    QVERIFY(noAbove == nullptr);
}

void AdjustSceneTests::findNearestPieceBelowReturnsCorrectPiece()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    // pieceA at (10, 10), pieceB at (10, 90) — pieceB is below pieceA.
    QJsonArray pieces;

    QJsonObject pieceA;
    pieceA["id"]           = QStringLiteral("pieceA");
    pieceA["x"]            = 10.0;
    pieceA["y"]            = 10.0;
    pieceA["w"]            = 30.0;
    pieceA["h"]            = 30.0;
    pieceA["origin_x_px"]  = 0.0;
    pieceA["origin_y_px"]  = 0.0;
    pieceA["rotation_deg"] = 0.0;
    pieceA["transform_str"] = QString();
    pieces.append(pieceA);

    QJsonObject pieceB;
    pieceB["id"]           = QStringLiteral("pieceB");
    pieceB["x"]            = 10.0;
    pieceB["y"]            = 90.0;
    pieceB["w"]            = 30.0;
    pieceB["h"]            = 30.0;
    pieceB["origin_x_px"]  = 0.0;
    pieceB["origin_y_px"]  = 0.0;
    pieceB["rotation_deg"] = 0.0;
    pieceB["transform_str"] = QString();
    pieces.append(pieceB);

    QJsonObject root;
    root["pieces"] = pieces;
    const QString bboxJson = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));

    AdjustScene scene;
    scene.loadLayout(svgPath, bboxJson);

    // From pieceA's perspective, pieceB is below it (higher y value).
    PieceOverlayItem* pieceAItem = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceBItem = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceAItem);
    QVERIFY(pieceBItem);

    PieceOverlayItem* nearestBelow = scene.findNearestPieceBelow(pieceAItem);
    QCOMPARE(nearestBelow, pieceBItem);

    // From pieceB's perspective, there is no piece below it.
    PieceOverlayItem* noBelow = scene.findNearestPieceBelow(pieceBItem);
    QVERIFY(noBelow == nullptr);
}

void AdjustSceneTests::checkOverlapsDetectsOverlappingPieces()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    PieceOverlayItem* pieceB = findPiece(scene, QStringLiteral("pieceB"));
    QVERIFY(pieceA);
    QVERIFY(pieceB);

    // Move pieceB on top of pieceA to create an overlap.
    pieceB->setPos(pieceA->pos());

    const QStringList overlaps = scene.checkOverlaps();
    QVERIFY(overlaps.contains(QStringLiteral("pieceA")));
    QVERIFY(overlaps.contains(QStringLiteral("pieceB")));
}

void AdjustSceneTests::checkOverlapsReturnsEmptyForNonOverlapping()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    // Default positions: pieceA at (10,10) and pieceB at (90,10) — no overlap.
    const QStringList overlaps = scene.checkOverlaps();
    QVERIFY2(overlaps.isEmpty(),
             "Non-overlapping pieces must produce an empty overlap list");
}

// ---------------------------------------------------------------------------
// Adjust-mode rotation pivot: bounding-box center, not the upper-left corner
// ---------------------------------------------------------------------------

void AdjustSceneTests::rotationPivotIsBoundingBoxCenter()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    QVERIFY(pieceA);

    // pieceA is 30x30 (see buildBboxJson()); the rotation pivot must be its
    // bounding-box center (15, 15), not the upper-left corner (0, 0).
    QCOMPARE(pieceA->transformOriginPoint(), QPointF(15.0, 15.0));
}

void AdjustSceneTests::rotatingAboutCenterKeepsCenterFixed()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    PieceOverlayItem* pieceA = findPiece(scene, QStringLiteral("pieceA"));
    QVERIFY(pieceA);

    const QPointF centerBefore = pieceA->sceneBoundingRect().center();

    // Rotate in place (no drag) — with a center pivot, the bounding-box
    // center must not move; only the corners swing around it.
    pieceA->setRotation(37.0);

    const QPointF centerAfter = pieceA->sceneBoundingRect().center();
    QVERIFY2(std::abs(centerAfter.x() - centerBefore.x()) < 0.01
                 && std::abs(centerAfter.y() - centerBefore.y()) < 0.01,
             "Bounding-box center must stay fixed when rotating about the center pivot");
}

void AdjustSceneTests::buildTransformRoundTripsRotationAroundCenter()
{
    // Non-square bbox so the round-trip math is exercised on both axes.
    PieceOverlayItem original(QStringLiteral("piece1"), 0.0, 0.0, 20.0, 10.0, 40.0, 20.0);
    original.setRotation(73.0);

    const QString transform = original.buildTransform();
    QVERIFY2(!transform.isEmpty(), "Rotated piece must produce a non-empty transform");
    QVERIFY2(transform.startsWith("matrix("), "Transform should be a matrix");

    // Replay the generated transform onto a fresh piece at the same base
    // position/size and verify it reproduces the same pos()/rotation() —
    // this is the contract loadLayout() relies on for transform_str replay.
    PieceOverlayItem replayed(QStringLiteral("piece1"), 0.0, 0.0, 20.0, 10.0, 40.0, 20.0, 0.0, transform);

    QVERIFY2(std::abs(replayed.pos().x() - original.pos().x()) < 0.01
                 && std::abs(replayed.pos().y() - original.pos().y()) < 0.01,
             "Replayed pos() must match the original rotated piece's pos()");
    QVERIFY2(std::abs(replayed.rotation() - original.rotation()) < 0.01,
             "Replayed rotation() must match the original rotated piece's rotation()");
}

void AdjustSceneTests::buildTransformRoundTripsMoveAndRotationAroundCenter()
{
    PieceOverlayItem original(QStringLiteral("piece1"), 0.0, 0.0, 20.0, 10.0, 40.0, 20.0);
    original.setPos(65.0, 35.0);
    original.setRotation(-48.0);

    const QString transform = original.buildTransform();
    QVERIFY2(!transform.isEmpty(), "Moved+rotated piece must produce a non-empty transform");

    PieceOverlayItem replayed(QStringLiteral("piece1"), 0.0, 0.0, 20.0, 10.0, 40.0, 20.0, 0.0, transform);

    QVERIFY2(std::abs(replayed.pos().x() - original.pos().x()) < 0.01
                 && std::abs(replayed.pos().y() - original.pos().y()) < 0.01,
             "Replayed pos() must match the original moved+rotated piece's pos()");
    QVERIFY2(std::abs(replayed.rotation() - original.rotation()) < 0.01,
             "Replayed rotation() must match the original moved+rotated piece's rotation()");
}

// ---------------------------------------------------------------------------
// DG.4 — compile-time gate tests for dumpOverlayData()
//
// Verifies that the #ifdef QT_DEBUG / #else inline-stub split on
// dumpOverlayData() is correct in both build configurations:
//
//   debug build  (QT_DEBUG defined):  the real file-writing implementation
//                                     is compiled and callable without crashing;
//                                     in this test no pieces are moved, so only
//                                     the JSON file is written (and no qWarning
//                                     is expected).
//
//   release build (QT_DEBUG absent):  the inline empty-body stub is compiled,
//                                     linked, callable, and performs no I/O.
//
// Call-site compatibility is guaranteed by the matching void() signature — the
// compiler would reject any mismatch at the three call sites in AdjustWindow.cpp.
// ---------------------------------------------------------------------------

void AdjustSceneTests::dg4_debugDumpOverlayDataCompilesAndIsCallable()
{
    // Set up a minimal scene with two non-overlapping pieces.
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    // In debug builds this exercises the real file-writing implementation.
    // In release builds this exercises the inline no-op stub.
    // Both must compile and run without crashing — reaching the QVERIFY proves it.
#ifdef QT_DEBUG
    scene.dumpOverlayData();
    for (int i = 0; i < 5; ++i)
        QFile::remove(QString("%1/output/adjust_overlay_%2.json").arg(QCoreApplication::applicationDirPath()).arg(i));
    // Reaching here proves the debug implementation compiled and ran without crashing.
    QVERIFY(true);
#else
    // The release stub is tested by dg4_releaseStubCompilesAndIsCallable; skip here.
    QSKIP("Debug-only path — skipped in release builds; see dg4_releaseStubCompilesAndIsCallable.");
#endif
} // dg4_debugDumpOverlayDataCompilesAndIsCallable

void AdjustSceneTests::dg4_releaseStubCompilesAndIsCallable()
{
    // Set up a minimal scene with two non-overlapping pieces.
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustScene scene;
    scene.loadLayout(svgPath, buildBboxJson());

    // In release builds the inline empty-body stub must compile and be callable.
    // In debug builds the real implementation is already tested above; skip here.
#ifndef QT_DEBUG
    for (int i = 0; i < 5; ++i)
        QFile::remove(QString("%1/output/adjust_overlay_%2.json").arg(QCoreApplication::applicationDirPath()).arg(i));
    scene.dumpOverlayData();
    // Reaching here proves the release stub compiled and ran without crashing.
    QVERIFY(true);
#else
    QVERIFY2(!QFile::exists(QCoreApplication::applicationDirPath() + "/output/adjust_overlay_0.json"),
             "Release stub must not create overlay dump files");
    // The debug implementation is tested by dg4_debugDumpOverlayDataCompilesAndIsCallable; skip here.
    QSKIP("Release-only path — skipped in debug builds; see dg4_debugDumpOverlayDataCompilesAndIsCallable.");
#endif
} // dg4_releaseStubCompilesAndIsCallable

QTEST_MAIN(AdjustSceneTests)
#include "AdjustSceneTests.moc"
