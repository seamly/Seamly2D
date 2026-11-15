// project: SeamlyLayout
// author: slspencer, copyright 2026 Seamly2D Project
// MIT License: https://opensource.org/licenses/MIT
//
// @file SvgCanvas.qml
// @brief Reusable interactive SVG canvas with pan, zoom, and fit-to-view.
//
// Displays SVG content inside a WebEngineView with JavaScript-driven pan/zoom.
// Double-click resets to fit-to-view. Scroll wheel zooms toward the cursor.
// Mouse drag pans the image.
//
// CanvasSvgSchemeHandler serves the SVG from memory at a seamly-canvas:// URL.
// The SVG is not put in a data: URL: Chromium refuses a URL over 2 MB.
//
// Usage:
//   SvgCanvas {
//       id: leftCanvas
//       canvasName:      "input"
//       placeholderText: "Import an SVG pattern to begin"
//       showBusy:        false
//   }
//   // Reload via signal handler in Main.qml:
//   onImportFinished: leftCanvas.reloadSvg(appController.getImportDomString())
//
// Properties:
//   canvasName      — URL host of this canvas's SVG: lowercase letters and digits, one name per canvas
//   svgContent      — SVG XML string; updated via reloadSvg(content); empty shows placeholder
//   placeholderText — message shown when svgContent is empty (default: "")
//   showBusy        — show spinning BusyIndicator overlay (default: false)
//
// Functions:
//   reloadSvg(content) — publish the SVG content and reload the canvas

import QtQuick 6.11
import QtQuick.Controls 6.11
import QtWebEngine 6.11
import SeamlyLayout

Item {
    id: root

    // @brief URL host of this canvas's SVG: lowercase letters and digits.
    // Each canvas needs its own name, because a new SVG replaces the earlier SVG of the same name.
    property string canvasName: ""

    // @brief SVG XML string to display.  Empty = show placeholder.
    // Updated via reloadSvg(content) called from Main.qml signal handlers.
    property string svgContent: ""

    // @brief Text shown in the centre of the canvas when svgContent is empty.
    property string placeholderText: ""

    // @brief When true a BusyIndicator is shown over the canvas.
    property bool showBusy: false

    // @brief Message shown when the SVG did not display. Empty when it displayed.
    property string displayError: ""

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
                // The page load event waits for the <img>. A naturalWidth of 0 means that the SVG
                // did not arrive or did not decode. The blank page has no <img> and gives -1.
                svgView.runJavaScript(
                    '(function() { var img = document.getElementById("svg-img");' +
                    ' return img ? img.naturalWidth : -1; })();',
                    function(imageWidth) {
                        if (imageWidth === 0) {
                            console.warn("SvgCanvas " + root.canvasName + ": the SVG did not display")
                            root.displayError = qsTr("Cannot display this SVG.")
                        } // if image did not display
                    })
                root.svgLoaded()
            } else if (loadingInfo.status === WebEngineLoadingInfo.LoadFailedStatus) {
                // Log only: a newer reloadSvg() also stops the page that it replaces.
                console.warn("SvgCanvas " + root.canvasName + ": page load failed: " + loadingInfo.errorString)
            } // if load succeeded / failed
        } // onLoadingChanged

        // @brief Publish the SVG to CanvasSvgSchemeHandler, then load an HTML pan/zoom page that shows it.
        // svgContent is the raw SVG XML string. No disk I/O — the handler serves the SVG from memory.
        // Only the small HTML page goes through loadHtml(), so the 2 MB data: URL limit does not apply.
        // The page's base URL is the SVG URL, so the page and its <img> have the same origin.
        function loadSvgFullPage(svgContent) {
            // publishSvg() replaces this canvas's earlier SVG. Empty content releases it.
            var svgUrl = CanvasSvgSchemeHandler.publishSvg(root.canvasName, svgContent).toString();
            if (svgUrl === "") {
                if (svgContent) {
                    // canvasName is not valid; the handler logged the name.
                    root.displayError = qsTr("Cannot display this SVG.");
                } // if content without a URL
                loadHtml("<html><body></body></html>", "about:blank");
                return;
            } // if no URL

            var bgColor  = Theme.canvasBackground.toString();

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
                '  <img id="svg-img" src="' + svgUrl + '">' +
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

            loadHtml(html, svgUrl);
        } // loadSvgFullPage
    } // WebEngineView svgView

    // @brief Load the SVG in the canvas from an SVG XML string.
    // Called from Main.qml whenever the input SVG or layout SVG is successfully
    // created in memory (self.input_dom or self.layout_dom).
    function reloadSvg(content) {
        root.displayError = ""
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

    // -----------------------------------------------------------------------
    // Display error — shown when the SVG did not display
    // -----------------------------------------------------------------------
    Text {
        anchors.centerIn: parent
        text:             root.displayError
        color:            Theme.textOnCanvas
        font.pixelSize:   Theme.fontSizeNormal
        visible:          root.displayError !== ""
    } // Text displayError
} // Item root
