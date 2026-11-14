// project: SeamlyLayout
// author: slspencer, copyright 2026 Seamly2D Project
// MIT License: https://opensource.org/licenses/MIT

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>
#include <QUrl>

#include <memory>

class ExportMenuTests final : public QObject
{
    Q_OBJECT

private slots:
    void dxfVariantsStayDisabledWhenLayoutContainsAllSizes();
    void tiledPdfStaysDisabledForMarker();
    void plainPdfIsDisabledWhenPdfDisabled();
    void splitLayoutsAllowSupportedFormats_data();
    void splitLayoutsAllowSupportedFormats();
    void supportedSplitExportsEmitRequests_data();
    void supportedSplitExportsEmitRequests();
    void svgModeItemsHaveNoCheckBox_data();
    void svgModeItemsHaveNoCheckBox();
}; // class ExportMenuTests

void ExportMenuTests::dxfVariantsStayDisabledWhenLayoutContainsAllSizes()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/ExportMenu.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QObject *menu = component.createWithInitialProperties({
        {QStringLiteral("layoutReady"), true},
        {QStringLiteral("showDxfVariants"), true},
        {QStringLiteral("dxfEnabled"), false},
    });
    QVERIFY2(menu != nullptr, qPrintable(component.errorString()));

    const auto item = [menu](const QString &name) {
        return menu->findChild<QObject *>(name);
    };
    QObject *dxfSubmenu = item(QStringLiteral("dxfSubmenu"));
    QObject *disabledNote = item(QStringLiteral("dxfDisabledNote"));
    QVERIFY(dxfSubmenu);
    QVERIFY(disabledNote);
    QVERIFY(dxfSubmenu->property("enabled").toBool());
    QVERIFY(!disabledNote->property("enabled").toBool());
    QCOMPARE(disabledNote->property("text").toString(),
             QStringLiteral("DXF: select Base Size or a size tab"));

    for (const QString &name : {
             QStringLiteral("dxfAstmR13Item"),
             QStringLiteral("dxfAstmClo3dItem"),
             QStringLiteral("dxfAamaItem"),
         }) {
        QObject *dxfItem = item(name);
        QVERIFY2(dxfItem, qPrintable(name));
        QVERIFY2(!dxfItem->property("enabled").toBool(), qPrintable(name));
    } // for each DXF variant

    delete menu;
} // ExportMenuTests::dxfVariantsStayDisabledWhenLayoutContainsAllSizes

void ExportMenuTests::tiledPdfStaysDisabledForMarker()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/ExportMenu.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QObject *menu = component.createWithInitialProperties({
        {QStringLiteral("layoutReady"), true},
        {QStringLiteral("pdfTiledTabAllowed"), false},
    });
    QVERIFY2(menu != nullptr, qPrintable(component.errorString()));

    QObject *pdfTiledItem = menu->findChild<QObject *>(QStringLiteral("pdfTiledItem"));
    QVERIFY(pdfTiledItem);
    QVERIFY(!pdfTiledItem->property("enabled").toBool());
    QCOMPARE(pdfTiledItem->property("text").toString(),
             QStringLiteral("PDF (Tiled): select a tab other than Marker"));

    delete menu;
} // ExportMenuTests::tiledPdfStaysDisabledForMarker

/// @brief Plain PDF is disabled when pdfEnabled is false; tiled PDF stays enabled.
void ExportMenuTests::plainPdfIsDisabledWhenPdfDisabled()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/ExportMenu.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QObject *menu = component.createWithInitialProperties({
        {QStringLiteral("layoutReady"), true},
        {QStringLiteral("pdfEnabled"), false},
    });
    QVERIFY2(menu != nullptr, qPrintable(component.errorString()));

    QObject *pdfItem = menu->findChild<QObject *>(QStringLiteral("pdfItem"));
    QObject *pdfTiledItem = menu->findChild<QObject *>(QStringLiteral("pdfTiledItem"));
    QVERIFY(pdfItem);
    QVERIFY(pdfTiledItem);
    QVERIFY(!pdfItem->property("enabled").toBool());
    QVERIFY(pdfTiledItem->property("enabled").toBool());

    delete menu;
} // ExportMenuTests::plainPdfIsDisabledWhenPdfDisabled

/// @brief Covers split and unsplit readiness with both SVG menu presentations.
void ExportMenuTests::splitLayoutsAllowSupportedFormats_data()
{
    QTest::addColumn<bool>("split_layout");
    QTest::addColumn<bool>("layout_ready");
    QTest::addColumn<bool>("svg_submenu");

    QTest::newRow("split-ready-svg-modes") << true << true << true;
    QTest::newRow("split-ready-svg-item") << true << true << false;
    QTest::newRow("split-not-ready-svg-modes") << true << false << true;
    QTest::newRow("split-not-ready-svg-item") << true << false << false;
    QTest::newRow("unsplit-ready-svg-modes") << false << true << true;
    QTest::newRow("unsplit-ready-svg-item") << false << true << false;
    QTest::newRow("unsplit-not-ready-svg-modes") << false << false << true;
    QTest::newRow("unsplit-not-ready-svg-item") << false << false << false;
}

/// @brief Split paper and fabric share supported formats without relaxing other export guards.
void ExportMenuTests::splitLayoutsAllowSupportedFormats()
{
    QFETCH(bool, split_layout);
    QFETCH(bool, layout_ready);
    QFETCH(bool, svg_submenu);

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/ExportMenu.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QObject *menu = component.createWithInitialProperties({
        {QStringLiteral("layoutReady"), layout_ready},
        {QStringLiteral("splitLayout"), split_layout},
        {QStringLiteral("showDxfVariants"), true},
        {QStringLiteral("showHpgl"), true},
        {QStringLiteral("showJpg"), true},
        {QStringLiteral("showPostScript"), true},
        {QStringLiteral("svgModeSubmenu"), svg_submenu},
        {QStringLiteral("gcodeExportAvailable"), true},
        {QStringLiteral("meshExportAvailable"), true},
    });
    QVERIFY2(menu != nullptr, qPrintable(component.errorString()));

    const auto item = [menu](const QString &name) {
        return menu->findChild<QObject *>(name);
    };
    QObject *splitNote = item(QStringLiteral("splitLayoutDisabledNote"));
    QVERIFY(splitNote);
    QQuickWindow window;
    window.setGeometry(0, 0, 320, 240);
    window.show();
    menu->setProperty("parent", QVariant::fromValue(window.contentItem()));
    QVERIFY(QMetaObject::invokeMethod(menu, "open"));
    QTRY_COMPARE(splitNote->property("visible").toBool(), split_layout);
    QCOMPARE(splitNote->property("text").toString(),
             QStringLiteral("Split layouts: DXF, PDF (Tiled), G-Code and 3D Mesh unavailable"));

    for (const QString &name : {
             QStringLiteral("pdfItem"),
             QStringLiteral("pngItem"),
             QStringLiteral("jpgItem"),
             QStringLiteral("epsItem"),
             QStringLiteral("psItem"),
             svg_submenu ? QStringLiteral("svgSubmenu") : QStringLiteral("svgItem"),
             QStringLiteral("hpglSubmenu"),
         }) {
        QObject *exportItem = item(name);
        QVERIFY2(exportItem, qPrintable(name));
        QCOMPARE(exportItem->property("enabled").toBool(), layout_ready);
    } // for each supported format

    for (const QString &name : {
             QStringLiteral("pdfTiledItem"),
             QStringLiteral("dxfAstmR13Item"),
             QStringLiteral("dxfAstmClo3dItem"),
             QStringLiteral("dxfAamaItem"),
             QStringLiteral("gcodeItem"),
             QStringLiteral("meshItem"),
         }) {
        QObject *exportItem = item(name);
        QVERIFY2(exportItem, qPrintable(name));
        QCOMPARE(exportItem->property("enabled").toBool(), layout_ready && !split_layout);
    } // for each split-restricted format

    QObject *dxfNote = item(QStringLiteral("dxfDisabledNote"));
    QObject *dxfSubmenu = item(QStringLiteral("dxfSubmenu"));
    QVERIFY(dxfNote);
    QVERIFY(dxfSubmenu);
    if (layout_ready) {
        QVERIFY(QMetaObject::invokeMethod(dxfSubmenu, "open"));
        QTRY_COMPARE(dxfNote->property("visible").toBool(), split_layout);
    } // if layout ready
    QCOMPARE(dxfNote->property("text").toString(),
             split_layout ? QStringLiteral("DXF: unavailable for split layouts")
                          : QStringLiteral("DXF: select Base Size or a size tab"));

    delete menu;
} // ExportMenuTests::splitLayoutsAllowSupportedFormats

/// @brief Pins the existing export request signals for every supported split-layout format.
void ExportMenuTests::supportedSplitExportsEmitRequests_data()
{
    QTest::addColumn<QString>("item_name");
    QTest::addColumn<QByteArray>("signal_signature");
    QTest::addColumn<QString>("mode");

    QTest::newRow("pdf") << QStringLiteral("pdfItem") << QByteArray(SIGNAL(exportPdfRequested())) << QString();
    QTest::newRow("png") << QStringLiteral("pngItem") << QByteArray(SIGNAL(exportPngRequested())) << QString();
    QTest::newRow("jpg") << QStringLiteral("jpgItem") << QByteArray(SIGNAL(exportJpgRequested())) << QString();
    QTest::newRow("eps") << QStringLiteral("epsItem")
                        << QByteArray(SIGNAL(exportPostscriptRequested(QString))) << QStringLiteral("eps");
    QTest::newRow("ps") << QStringLiteral("psItem")
                       << QByteArray(SIGNAL(exportPostscriptRequested(QString))) << QStringLiteral("ps");
    QTest::newRow("svg") << QStringLiteral("svgDesignerFontItem")
                        << QByteArray(SIGNAL(exportSvgModeRequested(QString))) << QStringLiteral("designerFont");
    QTest::newRow("hpgl-plot") << QStringLiteral("hpglPlotItem")
                              << QByteArray(SIGNAL(exportHpglRequested(QString))) << QStringLiteral("plot");
    QTest::newRow("hpgl-cut") << QStringLiteral("hpglCutItem")
                             << QByteArray(SIGNAL(exportHpglRequested(QString))) << QStringLiteral("cut");
}

/// @brief Enabled split-layout actions still dispatch their original format and mode.
void ExportMenuTests::supportedSplitExportsEmitRequests()
{
    QFETCH(QString, item_name);
    QFETCH(QByteArray, signal_signature);
    QFETCH(QString, mode);

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/ExportMenu.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QObject *menu = component.createWithInitialProperties({
        {QStringLiteral("layoutReady"), true},
        {QStringLiteral("splitLayout"), true},
        {QStringLiteral("showDxfVariants"), true},
        {QStringLiteral("showHpgl"), true},
        {QStringLiteral("showJpg"), true},
        {QStringLiteral("showPostScript"), true},
        {QStringLiteral("svgModeSubmenu"), true},
    });
    QVERIFY2(menu != nullptr, qPrintable(component.errorString()));

    QSignalSpy request(menu, signal_signature.constData());
    QVERIFY(request.isValid());
    QObject *export_item = menu->findChild<QObject *>(item_name);
    QVERIFY(export_item);
    QVERIFY(export_item->property("enabled").toBool());
    QVERIFY(QMetaObject::invokeMethod(export_item, "triggered"));
    QCOMPARE(request.count(), 1);
    if (!mode.isEmpty()) {
        QCOMPARE(request.at(0).at(0).toString(), mode);
    } // if the request has a mode

    delete menu;
}

/// @brief Lists the three Export > SVG text mode items and the mode each one requests.
void ExportMenuTests::svgModeItemsHaveNoCheckBox_data()
{
    QTest::addColumn<QString>("item_name");
    QTest::addColumn<QString>("mode");

    QTest::newRow("designer-font") << QStringLiteral("svgDesignerFontItem") << QStringLiteral("designerFont");
    QTest::newRow("single-line-font") << QStringLiteral("svgSingleLineFontItem") << QStringLiteral("singleLineFont");
    QTest::newRow("hershey-strokes") << QStringLiteral("svgHersheyStrokesItem") << QStringLiteral("hersheyStrokes");
} // ExportMenuTests::svgModeItemsHaveNoCheckBox_data

/// @brief An SVG text mode item has no check box, stays unchecked after a click, and requests its mode.
void ExportMenuTests::svgModeItemsHaveNoCheckBox()
{
    QFETCH(QString, item_name);
    QFETCH(QString, mode);

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/ExportMenu.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    // unique_ptr deletes the menu when a QVERIFY returns early.
    const std::unique_ptr<QObject> menu(component.createWithInitialProperties({
        {QStringLiteral("layoutReady"), true},
        {QStringLiteral("svgModeSubmenu"), true},
        {QStringLiteral("labelTextState"), QStringLiteral("text")},
    }));
    QVERIFY2(menu != nullptr, qPrintable(component.errorString()));

    QSignalSpy request(menu.get(), SIGNAL(exportSvgModeRequested(QString)));
    QVERIFY(request.isValid());
    QObject *mode_item = menu->findChild<QObject *>(item_name);
    QVERIFY(mode_item);
    QVERIFY(mode_item->property("enabled").toBool());
    QVERIFY(!mode_item->property("checkable").toBool());

    QVERIFY(QMetaObject::invokeMethod(mode_item, "click"));
    QVERIFY(!mode_item->property("checked").toBool());
    QCOMPARE(request.count(), 1);
    QCOMPARE(request.at(0).at(0).toString(), mode);
} // ExportMenuTests::svgModeItemsHaveNoCheckBox

QTEST_MAIN(ExportMenuTests)
#include "ExportMenuTests.moc"
