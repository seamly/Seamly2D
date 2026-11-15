// project: SeamlyLayout
// author: slspencer, copyright 2026 Seamly2D Project
// MIT License: https://opensource.org/licenses/MIT
//
// @file CanvasSvgSchemeHandler.cpp
// @brief Serves the SVG of each SvgCanvas to WebEngine from memory. See CanvasSvgSchemeHandler.h.

#include "CanvasSvgSchemeHandler.h"

#include <QBuffer>
#include <QLatin1Char>
#include <QLatin1String>
#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlScheme>
#include <QtGlobal>

namespace seamly_layout
{

namespace
{

/// @brief Tells whether a canvas name can be the host of a seamly-canvas URL.
/// @param canvas_name Name from QML.
/// @return true for one or more lowercase ASCII letters and digits.
bool isValidCanvasName(const QString& canvas_name)
{
    bool is_valid = !canvas_name.isEmpty();

    for (const QChar character : canvas_name)
    {
        const bool is_lowercase_letter = character >= QLatin1Char('a') && character <= QLatin1Char('z');
        const bool is_digit            = character >= QLatin1Char('0') && character <= QLatin1Char('9');
        if (!is_lowercase_letter && !is_digit)
        {
            is_valid = false;
        } // if character is not a lowercase letter or a digit
    } // for each character

    return is_valid;
} // isValidCanvasName

} // namespace

/// @brief Registers the seamly-canvas scheme with WebEngine.
void CanvasSvgSchemeHandler::registerUrlScheme()
{
    QWebEngineUrlScheme scheme(scheme_name);

    // Host syntax gives each canvas an origin. The canvas page uses the SVG URL as its base URL,
    // so the page and its <img> have the same origin and no CORS check applies.
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Host);

    // The bytes come only from this process, so the scheme is trustworthy like qrc.
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme);

    QWebEngineUrlScheme::registerScheme(scheme);
} // CanvasSvgSchemeHandler::registerUrlScheme

/// @brief Creates a handler that holds no SVG.
CanvasSvgSchemeHandler::CanvasSvgSchemeHandler(QObject* parent)
    : QWebEngineUrlSchemeHandler(parent)
{
} // CanvasSvgSchemeHandler::CanvasSvgSchemeHandler

/// @brief Stores the SVG of one canvas and returns its new URL.
QUrl CanvasSvgSchemeHandler::publishSvg(const QString& canvas_name, const QString& svg_content)
{
    QUrl svg_url;

    if (!isValidCanvasName(canvas_name))
    {
        // The name becomes the URL host; QUrl would change or reject other characters.
        qWarning("CanvasSvgSchemeHandler::publishSvg(): canvas name '%s' is not lowercase letters and digits",
                 qUtf8Printable(canvas_name));
    }
    else if (svg_content.isEmpty())
    {
        // Release the earlier SVG. The canvas shows its placeholder.
        m_published_svgs.remove(canvas_name);
    }
    else
    {
        // A new path for each call: WebEngine caches images by URL.
        ++m_generation;

        PublishedSvg published;
        published.url_path  = QStringLiteral("/%1.svg").arg(m_generation);
        published.svg_bytes = svg_content.toUtf8();
        m_published_svgs.insert(canvas_name, published);

        svg_url.setScheme(QLatin1String(scheme_name));
        svg_url.setHost(canvas_name);
        svg_url.setPath(published.url_path);
    } // if canvas name not valid / content empty / else

    return svg_url;
} // CanvasSvgSchemeHandler::publishSvg

/// @brief Gives the SVG bytes for the newest URL of a canvas.
QByteArray CanvasSvgSchemeHandler::publishedSvg(const QUrl& url) const
{
    QByteArray svg_bytes;

    const auto found = m_published_svgs.constFind(url.host());
    const bool is_known_canvas =
        url.scheme() == QLatin1String(scheme_name) && found != m_published_svgs.constEnd();

    // An older URL of the same canvas gets nothing: its SVG was replaced.
    if (is_known_canvas && found->url_path == url.path())
    {
        svg_bytes = found->svg_bytes;
    } // if newest URL of a known canvas

    return svg_bytes;
} // CanvasSvgSchemeHandler::publishedSvg

/// @brief Answers one WebEngine request on the GUI thread.
void CanvasSvgSchemeHandler::requestStarted(QWebEngineUrlRequestJob* job)
{
    const QByteArray svg_bytes = publishedSvg(job->requestUrl());

    if (job->requestMethod() != QByteArrayLiteral("GET"))
    {
        job->fail(QWebEngineUrlRequestJob::RequestDenied);
    }
    else if (svg_bytes.isEmpty())
    {
        // A page that a newer publishSvg() replaced can still ask for its old URL.
        qWarning("CanvasSvgSchemeHandler::requestStarted(): no SVG for %s",
                 qUtf8Printable(job->requestUrl().toString()));
        job->fail(QWebEngineUrlRequestJob::UrlNotFound);
    }
    else
    {
        // WebEngine reads the device until the job ends, but does not own it.
        // The job is the parent, so the buffer is deleted with the job.
        // The buffer shares svg_bytes; it does not copy them.
        QBuffer* svg_buffer = new QBuffer(job);
        svg_buffer->setData(svg_bytes);
        svg_buffer->open(QIODevice::ReadOnly);
        job->reply(QByteArrayLiteral("image/svg+xml"), svg_buffer);
    } // if not GET / no SVG / else
} // CanvasSvgSchemeHandler::requestStarted

} // namespace seamly_layout
