// project: SeamlyLayout
// author: slspencer, copyright 2026 Seamly2D Project
// MIT License: https://opensource.org/licenses/MIT
//
// @file CanvasSvgSchemeHandlerTests.cpp
// @brief Qt tests for CanvasSvgSchemeHandler: URL contract, replaced URLs, and SVGs over the 2 MB data: URL limit.

#include "CanvasSvgSchemeHandler.h"

#include <QGuiApplication>
#include <QSignalSpy>
#include <QTest>
#include <QVariant>
#include <QWebEnginePage>
#include <QWebEngineProfile>

namespace
{

/// @brief Largest URL that Chromium accepts: 2 MiB.
constexpr qsizetype data_url_limit = 2 * 1024 * 1024;

/// @brief Builds a 400 x 300 SVG. A <desc> of filler text sets its size.
/// @param filler_length Number of filler characters.
/// @return SVG XML text.
QString makeSvg(qsizetype filler_length)
{
    return QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"400\" height=\"300\""
                          " viewBox=\"0 0 400 300\"><desc>%1</desc>"
                          "<rect width=\"400\" height=\"300\" fill=\"#7351ad\"/></svg>")
        .arg(QString(filler_length, QLatin1Char('x')));
} // makeSvg

/// @brief Loads svg_url in an <img> of a loadHtml() page, as SvgCanvas.qml does.
/// @param profile WebEngine profile with the handler installed.
/// @param svg_url URL from CanvasSvgSchemeHandler::publishSvg(); also the page base URL.
/// @return naturalWidth of the image: 0 when the SVG did not display, -1 when the page did not load.
int loadedImageWidth(QWebEngineProfile* profile, const QUrl& svg_url)
{
    int image_width = -1;

    QWebEnginePage page(profile);
    QSignalSpy load_spy(&page, &QWebEnginePage::loadFinished);
    page.setHtml(QStringLiteral("<html><body><img id=\"svg-img\" src=\"%1\"></body></html>")
                     .arg(svg_url.toString()),
                 svg_url);

    // The page load event waits for the <img>, as in SvgCanvas.qml.
    const bool is_loaded = load_spy.wait(60000) && load_spy.first().first().toBool();
    if (is_loaded)
    {
        bool has_result = false;
        page.runJavaScript(QStringLiteral("document.getElementById('svg-img').naturalWidth"),
                           [&image_width, &has_result](const QVariant& result)
                           {
                               image_width = result.toInt();
                               has_result  = true;
                           });
        // image_width stays -1 when the script gives no result in time.
        const bool has_answer = QTest::qWaitFor([&has_result]() { return has_result; }, 30000);
        if (!has_answer)
        {
            qWarning("loadedImageWidth(): runJavaScript() gave no result");
        } // if no result
    } // if page loaded

    return image_width;
} // loadedImageWidth

} // namespace

/// @brief Tests the URL contract of CanvasSvgSchemeHandler and an SVG served to WebEngine.
class CanvasSvgSchemeHandlerTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void publishSvg_returnsSchemeUrlOfCanvas();
    void publishSvg_returnsNewUrlForEachCall();
    void publishSvg_rejectsCanvasNameThatIsNotLowercase();
    void publishSvg_emptyContentReleasesCanvas();
    void publishedSvg_returnsUtf8OfNewestUrl();
    void publishedSvg_rejectsReplacedUrl();
    void publishedSvg_keepsCanvasesApart();
    void publishedSvg_rejectsOtherScheme();
    void webEngine_displaysSvgOverDataUrlLimit();
    void webEngine_doesNotDisplayReplacedUrl();

private:
    QWebEngineProfile*                     m_profile = nullptr;  ///< Off-the-record profile with m_web_handler.
    seamly_layout::CanvasSvgSchemeHandler  m_web_handler;        ///< Handler for the WebEngine tests.
};

// One off-the-record profile for the WebEngine tests. QtWebEngineProcess starts with the first page.
void CanvasSvgSchemeHandlerTests::initTestCase()
{
    m_profile = new QWebEngineProfile(this);
    m_profile->installUrlSchemeHandler(QByteArray(seamly_layout::CanvasSvgSchemeHandler::scheme_name),
                                       &m_web_handler);
}

// Delete the profile before m_web_handler; every page is already deleted.
void CanvasSvgSchemeHandlerTests::cleanupTestCase()
{
    delete m_profile;
    m_profile = nullptr;
}

void CanvasSvgSchemeHandlerTests::publishSvg_returnsSchemeUrlOfCanvas()
{
    seamly_layout::CanvasSvgSchemeHandler handler;

    const QUrl url = handler.publishSvg(QStringLiteral("input"), makeSvg(10));

    QCOMPARE(url.scheme(), QStringLiteral("seamly-canvas"));
    QCOMPARE(url.host(), QStringLiteral("input"));
    QVERIFY(url.path().endsWith(QStringLiteral(".svg")));
}

void CanvasSvgSchemeHandlerTests::publishSvg_returnsNewUrlForEachCall()
{
    seamly_layout::CanvasSvgSchemeHandler handler;
    const QString svg = makeSvg(10);

    const QUrl first_url  = handler.publishSvg(QStringLiteral("input"), svg);
    const QUrl second_url = handler.publishSvg(QStringLiteral("input"), svg);

    QVERIFY(first_url != second_url);
}

void CanvasSvgSchemeHandlerTests::publishSvg_rejectsCanvasNameThatIsNotLowercase()
{
    seamly_layout::CanvasSvgSchemeHandler handler;

    QTest::ignoreMessage(QtWarningMsg, "CanvasSvgSchemeHandler::publishSvg(): canvas name 'Input' is not "
                                       "lowercase letters and digits");
    QTest::ignoreMessage(QtWarningMsg, "CanvasSvgSchemeHandler::publishSvg(): canvas name 'left canvas' is not "
                                       "lowercase letters and digits");
    QTest::ignoreMessage(QtWarningMsg, "CanvasSvgSchemeHandler::publishSvg(): canvas name '' is not "
                                       "lowercase letters and digits");

    QVERIFY(handler.publishSvg(QStringLiteral("Input"), makeSvg(10)).isEmpty());
    QVERIFY(handler.publishSvg(QStringLiteral("left canvas"), makeSvg(10)).isEmpty());
    QVERIFY(handler.publishSvg(QString(), makeSvg(10)).isEmpty());
}

void CanvasSvgSchemeHandlerTests::publishSvg_emptyContentReleasesCanvas()
{
    seamly_layout::CanvasSvgSchemeHandler handler;
    const QUrl url = handler.publishSvg(QStringLiteral("layout"), makeSvg(10));

    QVERIFY(handler.publishSvg(QStringLiteral("layout"), QString()).isEmpty());
    QVERIFY(handler.publishedSvg(url).isEmpty());
}

void CanvasSvgSchemeHandlerTests::publishedSvg_returnsUtf8OfNewestUrl()
{
    seamly_layout::CanvasSvgSchemeHandler handler;
    // Non-ASCII piece name: the bytes must be UTF-8, not Latin-1.
    const QString svg = QStringLiteral("<svg xmlns=\"http://www.w3.org/2000/svg\"><desc>Dos épaule</desc></svg>");

    const QUrl url = handler.publishSvg(QStringLiteral("input"), svg);

    QCOMPARE(handler.publishedSvg(url), svg.toUtf8());
}

void CanvasSvgSchemeHandlerTests::publishedSvg_rejectsReplacedUrl()
{
    seamly_layout::CanvasSvgSchemeHandler handler;
    const QUrl first_url  = handler.publishSvg(QStringLiteral("input"), makeSvg(10));
    const QUrl second_url = handler.publishSvg(QStringLiteral("input"), makeSvg(20));

    QVERIFY(handler.publishedSvg(first_url).isEmpty());
    QCOMPARE(handler.publishedSvg(second_url), makeSvg(20).toUtf8());
}

void CanvasSvgSchemeHandlerTests::publishedSvg_keepsCanvasesApart()
{
    seamly_layout::CanvasSvgSchemeHandler handler;
    const QUrl input_url  = handler.publishSvg(QStringLiteral("input"), makeSvg(10));
    const QUrl layout_url = handler.publishSvg(QStringLiteral("layout"), makeSvg(20));

    QCOMPARE(handler.publishedSvg(input_url), makeSvg(10).toUtf8());
    QCOMPARE(handler.publishedSvg(layout_url), makeSvg(20).toUtf8());
}

void CanvasSvgSchemeHandlerTests::publishedSvg_rejectsOtherScheme()
{
    seamly_layout::CanvasSvgSchemeHandler handler;
    QUrl url = handler.publishSvg(QStringLiteral("input"), makeSvg(10));

    url.setScheme(QStringLiteral("https"));

    QVERIFY(handler.publishedSvg(url).isEmpty());
}

// The defect this handler fixes: an SVG this large did not display from a data: URL.
void CanvasSvgSchemeHandlerTests::webEngine_displaysSvgOverDataUrlLimit()
{
    const QString svg = makeSvg(3 * 1024 * 1024);
    QVERIFY(svg.toUtf8().size() > data_url_limit);

    const QUrl url = m_web_handler.publishSvg(QStringLiteral("input"), svg);

    QCOMPARE(loadedImageWidth(m_profile, url), 400);
}

// A page that asks for a replaced URL gets no image, and the handler logs the URL.
void CanvasSvgSchemeHandlerTests::webEngine_doesNotDisplayReplacedUrl()
{
    const QUrl replaced_url = m_web_handler.publishSvg(QStringLiteral("layout"), makeSvg(10));
    m_web_handler.publishSvg(QStringLiteral("layout"), makeSvg(20));

    QTest::ignoreMessage(QtWarningMsg, qPrintable(QStringLiteral("CanvasSvgSchemeHandler::requestStarted(): no SVG for %1")
                                                      .arg(replaced_url.toString())));

    QCOMPARE(loadedImageWidth(m_profile, replaced_url), 0);
}

// WebEngine reads custom schemes once, so register before the application object exists.
int main(int argc, char* argv[])
{
    seamly_layout::CanvasSvgSchemeHandler::registerUrlScheme();
    // WebEngine shares OpenGL contexts with the GUI thread.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QGuiApplication app(argc, argv);

    CanvasSvgSchemeHandlerTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "CanvasSvgSchemeHandlerTests.moc"
