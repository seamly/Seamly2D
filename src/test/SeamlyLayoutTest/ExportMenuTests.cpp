// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>
#include <QUrl>

class ExportMenuTests final : public QObject
{
    Q_OBJECT

private slots:
    void dxfVariantsStayDisabledWhenLayoutContainsAllSizes();
    void tiledPdfStaysDisabledForMarker();
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

QTEST_MAIN(ExportMenuTests)
#include "ExportMenuTests.moc"
