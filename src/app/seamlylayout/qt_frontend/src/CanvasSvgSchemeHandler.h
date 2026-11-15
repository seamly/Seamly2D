// project: SeamlyLayout
// author: slspencer, copyright 2026 Seamly2D Project
// MIT License: https://opensource.org/licenses/MIT
//
// @file CanvasSvgSchemeHandler.h
// @brief Serves the SVG of each SvgCanvas to WebEngine from memory through a custom URL scheme.

#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QUrl>
#include <QWebEngineUrlSchemeHandler>

class QWebEngineUrlRequestJob;

/// @brief SeamlyLayout Qt frontend classes.
namespace seamly_layout
{

/// @brief Serves the SVG of each SvgCanvas to WebEngine from memory, with no size limit.
///
/// A loadHtml() page is a data: URL, and Chromium refuses a URL longer than 2 MB.
/// An SVG in a data: URL is percent-encoded two times, so an SVG over about 1 MB did not display.
/// This handler serves the SVG bytes for a seamly-canvas://<canvas>/<generation>.svg URL instead.
///
/// Startup order: registerUrlScheme() before QApplication, then install the handler on the
/// WebEngine profile, then expose it to QML. All calls run on the GUI thread.
class CanvasSvgSchemeHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT

public:
    /// @brief URL scheme name. Chromium needs a lowercase scheme name.
    static constexpr const char* scheme_name = "seamly-canvas";

    /// @brief Registers the scheme with WebEngine.
    ///
    /// Call once, before the QApplication object exists. WebEngine reads the scheme list at start-up.
    static void registerUrlScheme();

    /// @brief Creates a handler that holds no SVG.
    /// @param parent QObject parent, or nullptr.
    explicit CanvasSvgSchemeHandler(QObject* parent = nullptr);

    /// @brief Stores an SVG for one canvas and returns the URL that serves it.
    ///
    /// The new SVG replaces the earlier SVG of the same canvas. Each call returns a new URL,
    /// so WebEngine cannot show an image that it cached for an earlier URL.
    /// @param canvas_name Canvas identifier, used as the URL host: lowercase letters and digits only.
    /// @param svg_content SVG XML text. Empty text clears the canvas.
    /// @return URL of the SVG; an empty URL when svg_content is empty or canvas_name is not valid.
    Q_INVOKABLE QUrl publishSvg(const QString& canvas_name, const QString& svg_content);

    /// @brief Gives the UTF-8 SVG that a URL from publishSvg() serves.
    /// @param url Requested URL.
    /// @return SVG bytes; empty when the URL is not the newest URL of its canvas.
    QByteArray publishedSvg(const QUrl& url) const;

    /// @brief Answers one WebEngine request with the SVG bytes, or fails it with UrlNotFound.
    /// @param job Request owned by WebEngine.
    void requestStarted(QWebEngineUrlRequestJob* job) override;

private:
    /// @brief Newest SVG of one canvas.
    struct PublishedSvg
    {
        QString    url_path;   ///< URL path that serves the SVG, for example "/7.svg".
        QByteArray svg_bytes;  ///< UTF-8 SVG XML.
    };

    QHash<QString, PublishedSvg> m_published_svgs;  ///< Canvas name to its newest SVG.
    quint64                      m_generation = 0;  ///< Count of publishSvg() calls; makes each URL unique.
};

} // namespace seamly_layout
