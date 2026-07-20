// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
//
// @file SvgCanvas.qml
// @brief Reusable interactive SVG canvas with pan, zoom, and fit-to-view.
//
// Displays SVG content inside a WebEngineView with JavaScript-driven pan/zoom.
// Double-click resets to fit-to-view. Scroll wheel zooms toward the cursor.
// Mouse drag pans the image.
//
// Usage:
//   SvgCanvas {
//       id: leftCanvas
//       placeholderText: "Import an SVG pattern to begin"
//       showBusy:        false
//   }
//   // Reload via signal handler in Main.qml:
//   onImportFinished: leftCanvas.reloadSvg(appController.getImportDomString())
//
// Properties:
//   svgContent      — SVG XML string; updated via reloadSvg(content); empty shows placeholder
//   placeholderText — message shown when svgContent is empty (default: "")
//   showBusy        — show spinning BusyIndicator overlay (default: false)
//
// Functions:
//   reloadSvg(content) — embed SVG content inline and reload the canvas

import QtQuick 6.10
import QtQuick.Controls 6.10
import QtWebEngine 6.10
import SeamlyLayout

Item {
    id: root

    // @brief SVG XML string to display.  Empty = show placeholder.
    // Updated via reloadSvg(content) called from Main.qml signal handlers.
    property string svgContent: ""

    // @brief Text shown in the centre of the canvas when svgContent is empty.
    property string placeholderText: ""

    // @brief When true a BusyIndicator is shown over the canvas.
    property bool showBusy: false

    // @brief Emitted when the WebEngineView finishes loading an SVG page.
    signal svgLoaded()

    // -----------------------------------------------------------------------
    // Canvas background
    // -----------------------------------------------------------------------
    Rectangle {
        anchors.fill: parent
        color: Theme.canvasBackground
    } // Rectangle background

    // -----------------------------------------------------------------------
    // SVG display — WebEngineView with browser-quality rendering
    // -----------------------------------------------------------------------
    WebEngineView {
        id: svgView
        anchors.fill: parent
        visible: root.svgContent !== ""
        backgroundColor: Theme.canvasBackground

        // @brief Notify when the page finishes loading (new SVG is rendered).
        onLoadingChanged: function(loadingInfo) {
            if (loadingInfo.status === WebEngineLoadingInfo.LoadSucceededStatus) {
                root.svgLoaded()
            } // if load succeeded
        } // onLoadingChanged

        // @brief Embed SVG content as a data URI inside an HTML pan/zoom wrapper and load it.
        // svgContent is the raw SVG XML string; encodeURIComponent encodes it for the data URI.
        // No disk I/O — SVG is served directly from memory.
        function loadSvgFullPage(svgContent) {
            if (!svgContent) {
                loadHtml("<html><body></body></html>", "about:blank");
                return;
            } // if no content

            var bgColor  = Theme.canvasBackground.toString();
            var encoded  = encodeURIComponent(svgContent);
            var src      = "data:image/svg+xml;charset=utf-8," + encoded;

            var html =
                '<!DOCTYPE html>' +
                '<html><head><style>' +
                'html, body { margin: 0; padding: 0; width: 100%; height: 100%;' +
                '             overflow: hidden; background: ' + bgColor + '; }' +
                '#container { width: 100%; height: 100%; position: relative;' +
                '             overflow: hidden; cursor: grab;' +
                '             user-select: none; }' + /* Prevent blue selection highlight on double-click */
                '#container.dragging { cursor: grabbing; }' +
                '#svg-img { position: absolute; transform-origin: 0 0;' +
                '           user-select: none; pointer-events: none; }' + /* Prevent image selection/drag ghost on double-click */
                '</style></head><body>' +
                '<div id="container">' +
                '  <img id="svg-img" src="' + src + '">' +
                '</div>' +
                '<script>' +
                '(function() {' +
                '  var container = document.getElementById("container");' +
                '  var img       = document.getElementById("svg-img");' +
                '  var scale = 1, panX = 0, panY = 0;' +
                '  var isDragging = false, startX = 0, startY = 0,' +
                '      startPanX = 0, startPanY = 0;' +
                '' +
                '  img.onload = function() { fitToView(); };' +
                '  if (img.complete) { fitToView(); }' +
                '' +
                '  function fitToView() {' +
                '    var cw = container.clientWidth,  ch = container.clientHeight;' +
                '    var iw = img.naturalWidth,        ih = img.naturalHeight;' +
                '    if (iw === 0 || ih === 0) return;' +
                '    scale = Math.min(cw / iw, ch / ih);' +
                '    panX  = (cw - iw * scale) / 2;' +
                '    panY  = (ch - ih * scale) / 2;' +
                '    updateTransform();' +
                '  }' +
                '' +
                '  function updateTransform() {' +
                '    img.style.transform =' +
                '      "translate(" + panX + "px, " + panY + "px) scale(" + scale + ")";' +
                '  }' +
                '' +
                '  container.addEventListener("wheel", function(e) {' +
                '    e.preventDefault();' +
                '    var rect   = container.getBoundingClientRect();' +
                '    var mouseX = e.clientX - rect.left;' +
                '    var mouseY = e.clientY - rect.top;' +
                '    var imgX   = (mouseX - panX) / scale;' +
                '    var imgY   = (mouseY - panY) / scale;' +
                '    var delta  = e.deltaY > 0 ? 0.9 : 1.1;' +
                '    scale     *= delta;' +
                '    scale      = Math.max(0.1, Math.min(scale, 20));' +
                '    panX       = mouseX - imgX * scale;' +
                '    panY       = mouseY - imgY * scale;' +
                '    updateTransform();' +
                '  }, { passive: false });' +
                '' +
                '  container.addEventListener("mousedown", function(e) {' +
                '    if (e.button === 0) {' +
                '      isDragging = true;' +
                '      startX = e.clientX; startY = e.clientY;' +
                '      startPanX = panX;   startPanY = panY;' +
                '      container.classList.add("dragging");' +
                '    }' +
                '  });' +
                '' +
                '  document.addEventListener("mousemove", function(e) {' +
                '    if (isDragging) {' +
                '      panX = startPanX + (e.clientX - startX);' +
                '      panY = startPanY + (e.clientY - startY);' +
                '      updateTransform();' +
                '    }' +
                '  });' +
                '' +
                '  document.addEventListener("mouseup", function() {' +
                '    isDragging = false;' +
                '    container.classList.remove("dragging");' +
                '  });' +
                '' +
                '  container.addEventListener("dblclick", function(e) {' +
                '    e.preventDefault(); /* Suppress browser default selection on double-click */' +
                '    fitToView();' +
                '  });' +
                '' +
                '  window.addEventListener("resize", function() { fitToView(); });' +
                '' +
                '  window.fitToView = fitToView;' +
                '})();' +
                '<\/script>' +
                '</body></html>';

            loadHtml(html, "about:blank");
        } // loadSvgFullPage
    } // WebEngineView svgView

    // @brief Load the SVG in the canvas from an SVG XML string.
    // Called from Main.qml whenever the input SVG or layout SVG is successfully
    // created in memory (self.input_dom or self.layout_dom).
    function reloadSvg(content) {
        root.svgContent = content
        svgView.loadSvgFullPage(content)
    } // reloadSvg

    // @brief Reset the canvas to fit-to-view (same as double-click).
    function fitToView() {
        svgView.runJavaScript("fitToView();")
    } // fitToView

    // -----------------------------------------------------------------------
    // Busy indicator — shown during layout processing
    // -----------------------------------------------------------------------
    BusyIndicator {
        anchors.centerIn: parent
        running: root.showBusy
        visible: root.showBusy
    } // BusyIndicator

    // -----------------------------------------------------------------------
    // Placeholder — shown when no SVG is loaded
    // -----------------------------------------------------------------------
    Text {
        anchors.centerIn: parent
        text:             root.placeholderText
        color:            Theme.textOnCanvas
        font.pixelSize:   Theme.fontSizeNormal
        visible:          root.svgContent === "" && !root.showBusy
    } // Text placeholder
} // Item root
