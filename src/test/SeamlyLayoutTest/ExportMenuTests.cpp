// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QUrl>

class ExportMenuTests final : public QObject
{
    Q_OBJECT

private slots:
    void dxfVariantsStayDisabledWhenLayoutContainsAllSizes();
    void tiledPdfStaysDisabledForMarker();
    void splitLayoutsAllowOnlyRegularPdfExport();
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

void ExportMenuTests::splitLayoutsAllowOnlyRegularPdfExport()
{
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
        {QStringLiteral("gcodeExportAvailable"), true},
        {QStringLiteral("meshExportAvailable"), true},
    });
    QVERIFY2(menu != nullptr, qPrintable(component.errorString()));

    const auto item = [menu](const QString &name) {
        return menu->findChild<QObject *>(name);
    };
    QObject *pdfItem = item(QStringLiteral("pdfItem"));
    QObject *splitNote = item(QStringLiteral("splitLayoutDisabledNote"));
    QVERIFY(pdfItem);
    QVERIFY(splitNote);
    QVERIFY(pdfItem->property("enabled").toBool());
    QQuickWindow window;
    window.setGeometry(0, 0, 320, 240);
    window.show();
    menu->setProperty("parent", QVariant::fromValue(window.contentItem()));
    QVERIFY(QMetaObject::invokeMethod(menu, "open"));
    QTRY_VERIFY(splitNote->property("visible").toBool());

    for (const QString &name : {
             QStringLiteral("pdfTiledItem"),
             QStringLiteral("dxfAstmR13Item"),
             QStringLiteral("dxfAstmClo3dItem"),
             QStringLiteral("dxfAamaItem"),
             QStringLiteral("pngItem"),
             QStringLiteral("jpgItem"),
             QStringLiteral("epsItem"),
             QStringLiteral("psItem"),
             QStringLiteral("svgSubmenu"),
             QStringLiteral("hpglSubmenu"),
             QStringLiteral("gcodeItem"),
             QStringLiteral("meshItem"),
         }) {
        QObject *exportItem = item(name);
        QVERIFY2(exportItem, qPrintable(name));
        QVERIFY2(!exportItem->property("enabled").toBool(), qPrintable(name));
    } // for each unsupported format

    QObject *dxfNote = item(QStringLiteral("dxfDisabledNote"));
    QObject *dxfSubmenu = item(QStringLiteral("dxfSubmenu"));
    QVERIFY(dxfNote);
    QVERIFY(dxfSubmenu);
    QVERIFY(QMetaObject::invokeMethod(dxfSubmenu, "open"));
    QTRY_VERIFY(dxfNote->property("visible").toBool());
    QCOMPARE(dxfNote->property("text").toString(),
             QStringLiteral("DXF: split layouts export only as PDF"));

    delete menu;
} // ExportMenuTests::splitLayoutsAllowOnlyRegularPdfExport

QTEST_MAIN(ExportMenuTests)
#include "ExportMenuTests.moc"
