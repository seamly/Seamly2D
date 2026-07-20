// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file AdjustControllerTests.cpp
// @brief Qt tests for AdjustController — QML-to-QtWidgets bridge behavior.
//
// Covers:
//   • closeAdjustWindow() before any launch is a safe no-op (null guard)
//   • All four QML-visible signals are declared on the controller
//   • QSignalSpy can monitor all four signals
//   • launchAdjustWindow() with a valid SVG does not crash (first-launch path)
//   • launchAdjustWindow() called twice does not crash (reload path)

#include "adjust/AdjustController.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QTemporaryDir>
#include <QtTest/QSignalSpy>
#include <QtTest/QtTest>

namespace {

/// @brief Build a minimal bbox JSON string with two non-overlapping pieces.
QString buildBboxJson()
{
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
    pieceB["x"]            = 90.0;
    pieceB["y"]            = 10.0;
    pieceB["w"]            = 30.0;
    pieceB["h"]            = 30.0;
    pieceB["origin_x_px"]  = 0.0;
    pieceB["origin_y_px"]  = 0.0;
    pieceB["rotation_deg"] = 0.0;
    pieceB["transform_str"] = QString();
    pieces.append(pieceB);

    QJsonObject root;
    root["pieces"] = pieces;
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

/// @brief Write a minimal SVG with a contentRect element to a temporary directory.
/// @return Absolute path to the written SVG file, or empty string on failure.
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

} // namespace

/// @class AdjustControllerTests
/// @brief Verifies AdjustController bridge construction, signal declaration, and launch behavior.
class AdjustControllerTests : public QObject
{
    Q_OBJECT

private slots:
    /// @brief closeAdjustWindow() before any launch is a safe no-op (null guard path).
    void closeWindowBeforeLaunchIsNoop();

    /// @brief All four QML-visible signals are present on the controller's meta-object.
    void allSignalsDeclared();

    /// @brief QSignalSpy can attach to all four forwarding signals.
    void signalSpiesAreValid();

    /// @brief launchAdjustWindow() with a valid SVG and bbox JSON does not crash (first-launch path).
    void launchWindowWithValidSvgDoesNotCrash();

    /// @brief launchAdjustWindow() called twice does not crash — exercises the reload path.
    void launchWindowTwiceDoesNotCrash();
};

// ---------------------------------------------------------------------------
// Test implementations
// ---------------------------------------------------------------------------

void AdjustControllerTests::closeWindowBeforeLaunchIsNoop()
{
    // Construct a fresh controller with no window created yet.
    AdjustController controller;

    // Calling close before any launch must not dereference a null pointer.
    controller.closeAdjustWindow();

    // Reaching this line means the null guard worked correctly.
    QVERIFY(true);
}

void AdjustControllerTests::allSignalsDeclared()
{
    AdjustController controller;
    const QMetaObject* meta = controller.metaObject();

    // All four signals must be registered in the meta-object so QML can connect to them.
    QVERIFY2(meta->indexOfSignal("applyRequested(QString)") != -1,
             "applyRequested(QString) must be declared");
    QVERIFY2(meta->indexOfSignal("saveRequested()") != -1,
             "saveRequested() must be declared");
    QVERIFY2(meta->indexOfSignal("cancelRequested()") != -1,
             "cancelRequested() must be declared");
    QVERIFY2(meta->indexOfSignal("abandonRequested()") != -1,
             "abandonRequested() must be declared");
}

void AdjustControllerTests::signalSpiesAreValid()
{
    AdjustController controller;

    // QSignalSpy attaches to a signal by pointer — if the signal doesn't exist this fails.
    QSignalSpy applySpy  (&controller, &AdjustController::applyRequested);
    QSignalSpy saveSpy   (&controller, &AdjustController::saveRequested);
    QSignalSpy cancelSpy (&controller, &AdjustController::cancelRequested);
    QSignalSpy abandonSpy(&controller, &AdjustController::abandonRequested);

    QVERIFY(applySpy.isValid());
    QVERIFY(saveSpy.isValid());
    QVERIFY(cancelSpy.isValid());
    QVERIFY(abandonSpy.isValid());

    // No spurious signals should have fired during construction.
    QCOMPARE(applySpy.count(), 0);
    QCOMPARE(saveSpy.count(), 0);
    QCOMPARE(cancelSpy.count(), 0);
    QCOMPARE(abandonSpy.count(), 0);
}

void AdjustControllerTests::launchWindowWithValidSvgDoesNotCrash()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    AdjustController controller;

    // First call — creates AdjustWindow, connects signals, and calls show().
    controller.launchAdjustWindow(svgPath, buildBboxJson());

    // Immediately close to avoid interactive UI in the test runner.
    controller.closeAdjustWindow();
    QTest::qWait(0); // drain event loop so WA_DeleteOnClose window is destroyed before test returns
    // Reaching here means no crash on the first-launch path.
    QVERIFY(true);
}

void AdjustControllerTests::launchWindowTwiceDoesNotCrash()
{
    QTemporaryDir tempDir;
    QVERIFY2(tempDir.isValid(), "Temporary directory must be created");

    const QString svgPath = writeLayoutSvg(tempDir.path());
    QVERIFY2(!svgPath.isEmpty(), "Test SVG file must be written");

    const QString bboxJson = buildBboxJson();
    AdjustController controller;

    // First launch creates the window.
    controller.launchAdjustWindow(svgPath, bboxJson);

    // Second launch must call reload() on the existing window instead of creating a new one.
    controller.launchAdjustWindow(svgPath, bboxJson);

    // Clean up.
    controller.closeAdjustWindow();
    QTest::qWait(0); // drain event loop so WA_DeleteOnClose window is destroyed before test returns
    // Reaching here means the reload path did not crash.
    QVERIFY(true);
}

QTEST_MAIN(AdjustControllerTests)
#include "AdjustControllerTests.moc"
