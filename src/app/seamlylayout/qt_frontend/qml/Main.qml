// project: SeamlyLayout
// author: slspencer, copyright 2026
// LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
// Main.qml — Root ApplicationWindow for SeamlyLayout Qt 6.11 frontend.
//
// Note on startup console messages:
//   QQmlApplicationEngine creates an internal QQuickWindow when loading this
//   file, and ApplicationWindow is itself a QQuickWindowQmlImpl subclass of
//   QQuickWindow.  Qt's meta-object system sees xChanged(int) and yChanged(int)
//   declared on both QQuickWindow and QQuickWindowQmlImpl, and prints:
//     "signal xChanged(int) from QQuickWindow redefined in QQuickWindowQmlImpl"
//   twice per signal (four messages total) on every launch.
//   This is a Qt-internal quirk, not a bug in this code.  Safe to ignore.
//
// Layout:
//   ┌─────────────────────────────────────────────┐
//   │  TopMenuBar (Import | Settings | Export | X) │
//   ├─────────────────────────────────────────────┤
//   │  [AdjustMode banner — hidden when inactive]  │
//   ├───────────────────┬─────────────────────────┤
//   │  Left SvgCanvas   │  Right SvgCanvas/Adjust  │
//   │  (imported SVG)   │  (layout output)         │
//   ├───────────────────┴─────────────────────────┤
//   │  [Adjust action bar — hidden when inactive]  │
//   └─────────────────────────────────────────────┘

import QtQuick 6.11
import QtQuick.Controls 6.11
import QtQuick.Dialogs 6.11
import QtQuick.Layouts 6.11
import SeamlyLayout

ApplicationWindow {
    id: root

    title:   "SeamlyLayout"
    width:   1400
    height:  900
    visible: true

    // SettingsModel — layout settings state; bound to SettingsDialog
    SettingsModel {
        id: settingsModel
    } // SettingsModel settingsModel

    // PreferencesModel — application preference paths; bound to PreferencesPanel
    PreferencesModel {
        id: preferencesModel
    } // PreferencesModel preferencesModel

    // Load preferences and settings from disk when the window is fully constructed.
    Component.onCompleted: {
        // Resolve preferences path via PreferencesModel so startup does not
        // depend on process working directory.
        preferencesModel.load(preferencesModel.defaultPreferencesFilePath())
        // Resolve settings file via PreferencesModel so startup does not depend
        // on process working directory (relative paths can point to the wrong
        // folder and produce false "file not found" logs).
        settingsModel.load(preferencesModel.settingsFilePath())
    } // Component.onCompleted

    // Base filename (without extension) of the most recently imported SVG.
    // Populated by openSvgFile(); used to build default export filenames.
    property string importedBaseName: ""

    // -----------------------------------------------------------------------
    // SVG import entry points
    //
    // Two of them, one per transport:
    //   - openSvgFile() takes a path — the Import SVG file dialog
    //     (importDialog.onAccepted) and the SeamlyLayout command line
    //     (main.cpp, Task 49).
    //   - openSvgDocument() takes SVG text — the Seamly2D Layout Mode handoff,
    //     which sends the piece-mode document on standard input and never
    //     writes a file (Seamly2D.5).
    // Both are invoked from main.cpp with QMetaObject::invokeMethod once the
    // event loop starts, and both end in the same AppController import, so the
    // canvas cannot behave differently depending on where the SVG came from.
    // -----------------------------------------------------------------------

    // @brief Open an SVG file: remember its base name, then hand it to Rust.
    // @param localPath Absolute local file path (NOT a file:// URL).
    function openSvgFile(localPath) {
        if (!localPath || localPath === "") return;   // nothing to open
        // Extract base filename (without path and .svg extension) for default export names.
        var name = localPath.replace(/\\/g, "/")        // normalise backslashes
        name = name.substring(name.lastIndexOf("/") + 1) // strip directory
        if (name.toLowerCase().endsWith(".svg"))
            name = name.substring(0, name.length - 4)    // strip .svg extension
        root.importedBaseName = name
        // importSvg emits importFinished on success, errorOccurred on failure;
        // both are handled by the AppController block above.
        appController.importSvg(localPath);
    } // openSvgFile

    // @brief Open an SVG document held in memory (the Seamly2D handoff).
    // @param svgText      Complete stringified SVG document.
    // @param documentName Pattern base name, used for default export file
    //        names. There is no file name to derive one from; an empty value
    //        leaves the previous name in place.
    function openSvgDocument(svgText, documentName) {
        if (!svgText || svgText === "") return;   // nothing to open
        if (documentName && documentName !== "")
            root.importedBaseName = documentName
        // importSvgDocument emits importFinished on success, errorOccurred on
        // failure; both are handled by the AppController block above.
        appController.importSvgDocument(svgText);
    } // openSvgDocument

    // @brief Report a command-line problem detected before the window existed.
    // Called from main.cpp when the positional <svg-file> argument is missing,
    // unreadable, a folder, or not an SVG. The application stays open with an
    // empty canvas so the user can pick a file themselves.
    // @param message Complete sentence naming the file and the problem.
    function reportStartupError(message) {
        errorDialog.errorText = message;
        errorDialog.open();
    } // reportStartupError

    // Staging path for the DXF save location chosen in dxfSaveDialog.
    // Held here across the two-step dialog flow (save path → teaching dialog).
    property string pendingDxfPath: ""

    // Last successfully exported file path per format — used by the View menu.
    property string lastExportedDxfPath: ""
    property string lastExportedPdfPath: ""
    property string lastExportedPngPath: ""

    // Staged payload for deferred export start — shared across all formats.
    // Populated by each export handler; consumed by exportStartTimer.onTriggered.
    property string pendingExportPath: ""
    property string pendingExportFormat: ""        // "dxf"|"png"|"pdf"|"pdf-tiled"|"svg"
    property string pendingExportSettings: ""      // settings JSON for pdf / pdf-tiled
    property bool   pendingExportTeachingVersion: false  // teaching flag for DXF

    // Wait one short tick after opening the popup so it can paint before the
    // synchronous export starts and blocks the UI thread.
    Timer {
        id: exportStartTimer
        interval: 50
        repeat: false
        onTriggered: {
            var path = root.pendingExportPath
            var fmt  = root.pendingExportFormat
            if (path === "" || fmt === "") return  // nothing staged
            if (fmt === "dxf") {
                var optJson = JSON.stringify({ createTeachingVersion: root.pendingExportTeachingVersion })
                appController.exportDxf(path, optJson)
            } else if (fmt === "png") {
                appController.exportPng(path, 1.0)
            } else if (fmt === "pdf") {
                appController.exportPdf(path, root.pendingExportSettings)
            } else if (fmt === "pdf-tiled") {
                appController.exportPdfTiled(path, root.pendingExportSettings)
            } else if (fmt === "svg") {
                appController.exportSvg(path)
            } // if fmt
        } // onTriggered
    } // Timer exportStartTimer

    // @brief Build a default export filename: <importedBaseName>_YYYYMMDDHHSS[_tiled].<ext>
    // @param ext File extension without dot (e.g. "dxf", "png").
    // @param tiled If true, appends "_tiled" to the filename. Default is false.
    // @return Full filename string, or empty if no SVG has been imported.
    function makeExportFileName(ext, tiled) {
        if (root.importedBaseName === "") return "";
        var now = new Date();
        var ts  = now.getFullYear().toString()
                + ("0" + (now.getMonth() + 1)).slice(-2)
                + ("0" + now.getDate()).slice(-2)
                + ("0" + now.getHours()).slice(-2)
                + ("0" + now.getSeconds()).slice(-2);
        var name = root.importedBaseName + "_" + ts;
        if (tiled === undefined) tiled = false;
        if (tiled) name += "_tiled";
        return name + "." + ext;
    } // makeExportFileName

    // Staging area for the transforms JSON received from AdjustController.
    // Populated by the Apply flow; consumed by acceptAdjustments() and the
    // conflict dialog's "Proceed anyway" path.
    property string pendingAdjustTransforms: "[]"

    // True while waiting for the right canvas to finish loading the updated
    // SVG after exitAdjustMode().  When svgLoaded fires, QML closes the
    // AdjustWindow — preventing a flash of stale content.
    property bool pendingAdjustClose: false

    // AppController — bridge to Rust core
    AppController {
        id: appController

        // Error notification — shown as a dialog overlay
        onErrorOccurred: function(message) {
            exportStartTimer.stop()
            root.pendingExportPath = ""
            root.pendingExportFormat = ""
            root.pendingExportSettings = ""
            root.pendingExportTeachingVersion = false
            exportProgressPopup.close()
            errorDialog.errorText = message;
            errorDialog.open();
        } // onErrorOccurred

        // Layout warning — the layout succeeded but some pieces could not be
        // placed.  Shown as a non-blocking popup; the layout (with the pieces
        // that fit) has already been loaded into the right canvas.
        onLayoutWarning: function(message) {
            warningDialog.warningText = message;
            warningDialog.open();
        } // onLayoutWarning

        // Import warning — the SVG loaded, but it carries no data-type="piece"
        // tagging, so it is not a Seamly2D Layout Mode handoff file (Task 49).
        // Non-blocking: the file is already on the left canvas.
        onImportWarning: function(message) {
            warningDialog.warningText = message;
            warningDialog.open();
        } // onImportWarning

        // Load the input SVG in the left canvas after self.input_dom is successfully created.
        // Clear the right canvas — the old layout is no longer valid for the new SVG.
        onImportFinished: {
            leftCanvas.reloadSvg(appController.getImportDomString())
            rightCanvas.reloadSvg("")
        } // onImportFinished

        // Load the layout SVG in the right canvas after self.layout_dom is successfully created.
        onLayoutFinished: rightCanvas.reloadSvg(appController.getLayoutDomString())

        // Export success — store path for View menu; show dialog or open viewer.
        onExportFinished: function(path) {
            exportStartTimer.stop()
            root.pendingExportPath = ""
            root.pendingExportFormat = ""
            root.pendingExportSettings = ""
            root.pendingExportTeachingVersion = false
            exportProgressPopup.close()
            // Track last exported path per format for View menu
            if (path.endsWith(".dxf"))
                root.lastExportedDxfPath = path
            else if (path.endsWith(".pdf"))
                root.lastExportedPdfPath = path
            else if (path.endsWith(".png"))
                root.lastExportedPngPath = path

            if (path.endsWith(".png") && preferencesModel.pngViewerPath !== "") {
                // PNG export: open directly in the configured PNG viewer
                preferencesModel.openInViewer(preferencesModel.pngViewerPath, path)
            } else if (path.endsWith(".pdf") && preferencesModel.pdfViewerPath !== "") {
                // PDF export: open directly in the configured PDF viewer
                preferencesModel.openInViewer(preferencesModel.pdfViewerPath, path)
            } else {
                // DXF and other exports: show a save-confirmation dialog only.
                // No "view the file" prompt — viewing is done via the View menu.
                exportSuccessDialog.exportPath = path;
                exportSuccessDialog.open();
            } // if png/pdf with viewer configured
        } // onExportFinished
    } // AppController

    // AdjustController — owns the QtWidgets AdjustWindow; bridges QML ↔ AdjustWindow.
    // launchAdjustWindow() opens (or reloads) AdjustWindow with the current layout SVG.
    AdjustController {
        id: adjustController

        // Apply: run overlap check; open conflict dialog if any, else apply directly.
        onApplyRequested: function(transformsJson) {
            root.pendingAdjustTransforms = transformsJson
            // --- debug ---

            try {
                // display the original json string
                console.log("[Main.qml ApplicationWindow AdjustController] onApplyRequested(): 1 transformsJson=" + transformsJson)
                var transformArr = JSON.parse(transformsJson)
                for (var di = 0; di < transformArr.length; di++) {
                    // NOTE: If transformArr[di].id is blank here, it means the transformsJson array
                    // being sent from AdjustWindow (or the Rust backend) does not include an 'id'
                    // property for each piece object. This usually happens if the code that
                    // generates the transforms JSON omits the 'id' field, or if the mapping
                    // from internal piece data to JSON is missing the identifier.
                    // To fix: Ensure that each transform object in the array has an 'id'
                    // property set to the unique identifier for the pattern piece.
                    console.log("[Main.qml ApplicationWindow AdjustController] onApplyRequested(): 2  [" + di + "] id='" + transformArr[di].id + "'  transform='" + transformArr[di].transform + "'")
                }
            } catch (dbgE) {
                console.log("[Main.qml] onApplyRequested(): 3 could not parse transformsJson:", dbgE)
            } // try debug
            // --- end debug ---
            var conflicts = JSON.parse(appController.checkOverlaps(
                                appController.getPieceBboxes()))
            if (conflicts.length > 0) {
                adjustConflictsDialog.conflictIds = conflicts
                adjustConflictsDialog.open()
            } else {
                console.log("[Main.qml] onApplyRequested(): 4 calling acceptAdjustments() -- no conflicts")
                appController.acceptAdjustments(root.pendingAdjustTransforms)
            } // if conflicts
        } // onApplyRequested

        // Save: exit AdjustMode, save layout_dom, reload right canvas.
        // exitAdjustMode() promotes adjust_dom → layout_dom and emits layoutFinished,
        // which triggers onLayoutFinished → rightCanvas.reloadSvg().
        // The AdjustWindow stays open until rightCanvas.svgLoaded fires (see
        // Connections block below) to prevent a flash of stale content.
        onSaveRequested: {
            root.pendingAdjustClose = true
            appController.exitAdjustMode()
        } // onSaveRequested

        // Cancel: show confirmation when dirty; discard directly when clean.
        onCancelRequested: {
            if (appController.isAdjustDirty) {
                adjustDiscardDialog.open()
            } else {
                appController.discardAdjustments()
            } // if dirty
        } // onCancelRequested

        // Title-bar X closes the Adjust window and abandons the session immediately.
        onAbandonRequested: appController.discardAdjustments()
    } // AdjustController adjustController

    // -----------------------------------------------------------------------
    // Background
    // -----------------------------------------------------------------------
    background: Rectangle {
        color: Theme.appBackground
    } // background Rectangle

    // -----------------------------------------------------------------------
    // Top menu bar
    // -----------------------------------------------------------------------
    header: TopMenuBar {
        id: topMenuBar

        svgImported:          appController.isSvgImported
        layoutReady:          appController.isLayoutReady
        createLayoutEnabled:  appController.isCreateLayoutEnabled
        adjustMode:           appController.isAdjustMode
        pdfTiledExportEnabled: settingsModel.paperType === "tiled"

        onImportClicked: {
            // Open the file picker in the configured Input SVG Directory.
            // Falls back to <exeDir>/input when no directory is set in Preferences.
            // localFileToUrl() / defaultInputFolderUrl() use QUrl::fromLocalFile()
            // for correct cross-platform path handling on Windows and Linux/macOS.
            importDialog.currentFolder = preferencesModel.localFileToUrl(
                preferencesModel.resolvedInputDirectory()
            )
            importDialog.open()
        } // onImportClicked

        onLayoutSettingsClicked: {
            var path = preferencesModel.settingsFilePath()
            settingsModel.load(path)
            settingsDialog.settingsPath = path
            settingsDialog.settingsFolderUrl = preferencesModel.localFileToUrl(
                preferencesModel.resolvedSettingsDirectory()
            )
            settingsDialog.open()
        } // onLayoutSettingsClicked

        onCreateLayoutClicked: {
            // Serialize current settings to JSON and invoke the Rust layout pipeline.
            // processLayout() calls extract_piece_rects → pack_shelves → assemble_layout_svg
            // and emits layoutFinished() or errorOccurred() on completion.
            appController.processLayout(settingsModel.toJson())
        } // onCreateLayoutClicked

        onAdjustLayoutClicked: {
            appController.enterAdjustMode()
            var adjustDomPath = appController.saveAdjustDom()
            adjustController.launchAdjustWindow(
                adjustDomPath,
                appController.getAdjustPieceBoxes("adjust_dom.svg")
            )
        } // onAdjustLayoutClicked

        onPreferencesClicked:      preferencesController.openPreferences()
        onExportDxfAstmRequested: {
            var dir  = preferencesModel.resolvedLayoutDirectory() // default export directory is the resolved Layout Output Directory
            var name = root.makeExportFileName("dxf") // default name: <importedBaseName>_YYYYMMDDHHSS.dxf
            var path = preferencesModel.getSaveFilePath(
                // Prompt the user to choose a save location for the DXF file,
                // with the configured default directory and a suggested filename
                // based on the imported SVG name and current timestamp.
                "Save DXF-ASTM File", dir, name,
                "DXF Files (*.dxf);;All Files (*)")
            if (path !== "") {
                // Stage path; teaching dialog collects the teaching-version flag before export starts.
                root.pendingDxfPath = path
                dxfTeachingDialog.open()
            } // if user chose a path
        } // onExportDxfAstmRequested
        onExportPngRequested: {
            var dir  = preferencesModel.resolvedLayoutDirectory() // default export directory is the resolved Layout Output Directory
            var name = root.makeExportFileName("png") // default name: <importedBaseName>_YYYYMMDDHHSS.png
            var path = preferencesModel.getSaveFilePath(
                "Save PNG File", dir, name,
                "PNG Files (*.png);;All Files (*)")
            if (path !== "") {
                // Stage export, show progress popup, then start after one paint tick.
                root.pendingExportPath = path
                root.pendingExportFormat = "png"
                exportProgressPopup.open()
                exportStartTimer.restart()
            } // if path
        } // onExportPngRequested
        onExportSvgRequested: {
            var dir  = preferencesModel.resolvedLayoutDirectory() // default export directory is the resolved Layout Output Directory
            var name = root.makeExportFileName("svg") // default name: <importedBaseName>_YYYYMMDDHHSS.svg
            var path = preferencesModel.getSaveFilePath(
                "Save SVG File", dir, name,
                "SVG Files (*.svg);;All Files (*)")
            if (path !== "") {
                // Stage export, show progress popup, then start after one paint tick.
                root.pendingExportPath = path
                root.pendingExportFormat = "svg"
                exportProgressPopup.open()
                exportStartTimer.restart()
            } // if path
        } // onExportSvgRequested
        onExportPdfRequested: {
            var dir  = preferencesModel.resolvedLayoutDirectory() // default export directory is the resolved Layout Output Directory
            var name = root.makeExportFileName("pdf") // default name: <importedBaseName>_YYYYMMDDHHSS.pdf
            var path = preferencesModel.getSaveFilePath(
                "Save PDF File", dir, name,
                "PDF Files (*.pdf);;All Files (*)")
            if (path !== "") {
                // Stage export, show progress popup, then start after one paint tick.
                root.pendingExportPath = path
                root.pendingExportFormat = "pdf"
                root.pendingExportSettings = settingsModel.toJson()
                exportProgressPopup.open()
                exportStartTimer.restart()
            } // if path
        } // onExportPdfRequested
        onExportPdfTiledRequested: {
            var dir  = preferencesModel.resolvedLayoutDirectory() // from preferences layout_directory; fallback: <exeDir>/output
            var name = root.makeExportFileName("pdf", true) // default name: <importedBaseName>_YYYYMMDDHHSS_tiled.pdf
            var path = preferencesModel.getSaveFilePath(
                "Save Tiled PDF File", dir, name,
                "PDF Files (*.pdf);;All Files (*)")
            if (path !== "") {
                // Stage export, show progress popup, then start after one paint tick.
                root.pendingExportPath = path
                root.pendingExportFormat = "pdf-tiled"
                root.pendingExportSettings = settingsModel.toJson()
                exportProgressPopup.open()
                exportStartTimer.restart()
            } // if path
        } // onExportPdfTiledRequested

        // View dropdown handlers — open a file-picker in the Layout Output Directory,
        // filtered by format, then open the selected file in the configured viewer.
        onViewDxfAstmRequested: {
            console.log("[Main.qml TopMenuBar] onViewDxfAstmRequested(): 1 triggered")
            var dir  = preferencesModel.resolvedLayoutDirectory() // default directory is the resolved Layout Output Directory
            console.log("[Main.qml TopMenuBar] onViewDxfAstmRequested(): 2 dir=" + dir)
            var path = preferencesModel.getOpenFilePath(
                "Open DXF-ASTM File", dir,
                "DXF Files (*.dxf);;All Files (*)")
            console.log("[Main.qml TopMenuBar] onViewDxfAstmRequested(): 3 path=" + path + " dxfViewerPath=" + preferencesModel.dxfViewerPath)
            if (path !== "" && preferencesModel.dxfViewerPath !== "") {
                // Open the selected DXF file in the configured DXF viewer (primary action).
                console.log("[Main.qml TopMenuBar] onViewDxfAstmRequested(): 4 opening viewer")
                preferencesModel.openInViewer(preferencesModel.dxfViewerPath, path)
                // V.2: check for a companion teaching file (.txt) in the same directory.
                // Teaching files are generated during DXF export when createTeachingVersion is true.
                const teachingPath = preferencesModel.dxfTeachingFilePath(path)
                console.log("[Main.qml TopMenuBar] onViewDxfAstmRequested(): 5 checking teaching file=" + teachingPath)
                if (preferencesModel.fileExists(teachingPath)) {
                    // Teaching file found — offer to open it as a secondary affordance.
                    // The dialog is non-modal so the DXF viewer launch is not blocked.
                    console.log("[Main.qml TopMenuBar] onViewDxfAstmRequested(): 5 teaching file found, prompting user")
                    viewDxfTeachingDialog.teachingFilePath = teachingPath
                    viewDxfTeachingDialog.open()
                } else {
                    console.log("[Main.qml TopMenuBar] onViewDxfAstmRequested(): 5 no teaching file found")
                } // if teaching file exists
            } else if (path === "") {
                console.log("[Main.qml TopMenuBar] onViewDxfAstmRequested(): 4 file pick cancelled")
            } else {
                console.log("[Main.qml TopMenuBar] onViewDxfAstmRequested(): 4 no DXF viewer configured")
            } // if path && viewer
        } // onViewDxfAstmRequested
        onViewPdfRequested: {
            console.log("[Main.qml TopMenuBar] onViewPdfRequested(): 1 triggered")
            var dir  = preferencesModel.resolvedLayoutDirectory() // default directory is the resolved Layout Output Directory
            console.log("[Main.qml TopMenuBar] onViewPdfRequested(): 2 dir=" + dir)
            var path = preferencesModel.getOpenFilePath(
                "Open PDF File", dir,
                "PDF Files (*.pdf);;All Files (*)")
            console.log("[Main.qml TopMenuBar] onViewPdfRequested(): 3 path=" + path + " pdfViewerPath=" + preferencesModel.pdfViewerPath)
            if (path !== "" && preferencesModel.pdfViewerPath !== "") {
                // Open the selected PDF file in the configured PDF viewer application.
                console.log("[Main.qml TopMenuBar] onViewPdfRequested(): 4 opening viewer")
                preferencesModel.openInViewer(preferencesModel.pdfViewerPath, path)
            } else if (path === "") {
                console.log("[Main.qml TopMenuBar] onViewPdfRequested(): 4 file pick cancelled")
            } else {
                console.log("[Main.qml TopMenuBar] onViewPdfRequested(): 4 no PDF viewer configured")
            } // if path && viewer
        } // onViewPdfRequested
        onViewPdfTiledRequested: {
            console.log("[Main.qml TopMenuBar] onViewPdfTiledRequested(): 1 triggered")
            var dir  = preferencesModel.resolvedLayoutDirectory() // default directory is the resolved Layout Output Directory
            console.log("[Main.qml TopMenuBar] onViewPdfTiledRequested(): 2 dir=" + dir)
            var path = preferencesModel.getOpenFilePath(
                "Open Tiled PDF File", dir,
                "PDF Files (*.pdf);;All Files (*)")
            console.log("[Main.qml TopMenuBar] onViewPdfTiledRequested(): 3 path=" + path + " pdfViewerPath=" + preferencesModel.pdfViewerPath)
            if (path !== "" && preferencesModel.pdfViewerPath !== "") {
                // Open the selected PDF file in the configured PDF viewer application.
                console.log("[Main.qml TopMenuBar] onViewPdfTiledRequested(): 4 opening viewer")
                preferencesModel.openInViewer(preferencesModel.pdfViewerPath, path)
            } else if (path === "") {
                console.log("[Main.qml TopMenuBar] onViewPdfTiledRequested(): 4 file pick cancelled")
            } else {
                console.log("[Main.qml TopMenuBar] onViewPdfTiledRequested(): 4 no PDF viewer configured")
            } // if path && viewer
        } // onViewPdfTiledRequested
        onViewPngRequested: {
            console.log("[Main.qml TopMenuBar] onViewPngRequested(): 1 triggered")
            var dir  = preferencesModel.resolvedLayoutDirectory()
            console.log("[Main.qml TopMenuBar] onViewPngRequested(): 2 dir=" + dir)
            var path = preferencesModel.getOpenFilePath(
                "Open PNG File", dir,
                "PNG Files (*.png);;All Files (*)")
            console.log("[Main.qml TopMenuBar] onViewPngRequested(): 3 path=" + path + " pngViewerPath=" + preferencesModel.pngViewerPath)
            if (path !== "" && preferencesModel.pngViewerPath !== "") {
                // Open the selected PNG file in the configured PNG viewer application.
                console.log("[Main.qml TopMenuBar] onViewPngRequested(): 4 opening viewer")
                preferencesModel.openInViewer(preferencesModel.pngViewerPath, path)
            } else if (path === "") {
                console.log("[Main.qml TopMenuBar] onViewPngRequested(): 4 file pick cancelled")
            } else {
                console.log("[Main.qml TopMenuBar] onViewPngRequested(): 4 no PNG viewer configured")
            } // if path && viewer
        } // onViewPngRequested
        onViewSvgRequested: {
            console.log("[Main.qml TopMenuBar] onViewSvgRequested(): 1 triggered")
            var dir  = preferencesModel.resolvedLayoutDirectory()
            console.log("[Main.qml TopMenuBar] onViewSvgRequested(): 2 dir=" + dir)
            var path = preferencesModel.getOpenFilePath(
                "Open SVG File", dir,
                "SVG Files (*.svg);;All Files (*)")
            console.log("[Main.qml TopMenuBar] onViewSvgRequested(): 3 path=" + path)
            if (path !== "") {
                let url = preferencesModel.localFileToUrl(path)
                // Open the selected SVG file with the system default application for SVG files.
                console.log("[Main.qml TopMenuBar] onViewSvgRequested(): 4 opening url=" + url)
                Qt.openUrlExternally(url)
            } else {
                console.log("[Main.qml TopMenuBar] onViewSvgRequested(): 4 file pick cancelled")
            } // if path
        } // onViewSvgRequested
        onViewProjectorRequested: {
            console.log("[Main.qml TopMenuBar] onViewProjectorRequested(): 1 triggered")
            var dir  = preferencesModel.resolvedLayoutDirectory()
            console.log("[Main.qml TopMenuBar] onViewProjectorRequested(): 2 dir=" + dir)
            var path = preferencesModel.getOpenFilePath(
                "Open File for Projector", dir,
                "PDF Files (*.pdf);;PNG Files (*.png);;SVG Files (*.svg);;All Files (*)")
            console.log("[Main.qml TopMenuBar] onViewProjectorRequested(): 3 path=" + path + " projectorPath=" + preferencesModel.projectorPath)
            if (path !== "" && preferencesModel.projectorPath !== "") {
                // Open the selected file using the configured projector — either
                // a local executable (e.g. Pattern Projector PWA) or an https://
                // URL (e.g. https://patternprojector.com). openInViewer parses
                // the field and appends the file path as the final argument.
                console.log("[Main.qml TopMenuBar] onViewProjectorRequested(): 4 opening projector")
                preferencesModel.openInViewer(preferencesModel.projectorPath, path)
            } else if (path === "") {
                console.log("[Main.qml TopMenuBar] onViewProjectorRequested(): 4 file pick cancelled")
            } else {
                console.log("[Main.qml TopMenuBar] onViewProjectorRequested(): 4 no projector configured")
            } // if path && projector
        } // onViewProjectorRequested
    } // TopMenuBar topMenuBar

    // -----------------------------------------------------------------------
    // Main content — dual canvas area with AdjustMode banner and action bar
    // -----------------------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // AdjustMode banner — content moved to AdjustWindow title bar and status bar (Phase 8b).
        // Hidden permanently; kept as a zero-height placeholder to avoid layout shifts.
        Rectangle {
            id: adjustModeBanner
            Layout.fillWidth: true
            implicitHeight:   0
            visible:          false
        } // Rectangle adjustModeBanner

        // Canvas row — left (import) and right (layout / adjust)
        RowLayout {
            Layout.fillWidth:  true
            Layout.fillHeight: true
            spacing: 2

            // Left canvas — imported SVG display
            SvgCanvas {
                id: leftCanvas
                Layout.fillWidth:  true
                Layout.fillHeight: true

                placeholderText: "Import an SVG pattern to begin"
                showBusy:        false
            } // SvgCanvas leftCanvas

            // Right canvas frame — violet border when AdjustMode is active
            Rectangle {
                id: rightCanvasFrame
                Layout.fillWidth:  true
                Layout.fillHeight: true
                color:        "transparent"
                border.color: appController.isAdjustMode ? Theme.violetLight : "transparent"
                border.width: appController.isAdjustMode ? 3 : 0

                // Static layout display — shown when NOT in AdjustMode
                SvgCanvas {
                    id: rightCanvas
                    anchors.fill:    parent
                    anchors.margins: 0

                    visible:         !appController.isAdjustMode
                    placeholderText: "Apply settings to generate layout"
                    showBusy:        appController.isLayoutInProgress
                } // SvgCanvas rightCanvas

                // Fit-to-view button — overlaid on the right canvas top-right corner.
                // Visible when a layout is displayed and not in AdjustMode.
                Rectangle {
                    id: fitToViewButton
                    visible: appController.isLayoutReady && !appController.isAdjustMode
                    z: 10
                    width:  32
                    height: 32
                    radius: 6
                    color:  fitToViewHover.containsMouse
                            ? Theme.violetMedium
                            : Qt.rgba(Theme.violetDark.r,
                                      Theme.violetDark.g,
                                      Theme.violetDark.b, 0.82)
                    border.color: Theme.violetLight
                    border.width: 1
                    anchors.top:   parent.top
                    anchors.right: parent.right
                    anchors.topMargin:   10
                    anchors.rightMargin: 14

                    Text {
                        anchors.centerIn: parent
                        text:             "\u26f6" // ⛶ four-corners / fit-to-view
                        color:            "white"
                        font.pixelSize:   16
                    } // Text icon

                    MouseArea {
                        id: fitToViewHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape:  Qt.PointingHandCursor
                        onClicked:    rightCanvas.fitToView()
                    } // MouseArea

                    ToolTip.visible: fitToViewHover.containsMouse
                    ToolTip.text:    "Fit layout to canvas (double-click also works)"
                    ToolTip.delay:   600
                } // Rectangle fitToViewButton

                // Progress overlay — visible whenever the layout pipeline is
                // running.  Shown independent of any user setting so the user
                // always sees feedback while pieces are extracted, packed, and
                // assembled.  Bound to `appController.layoutProgress` (0–100;
                // -1 when idle).  Stages emit at 20 / 40 / 60 / 100 from the
                // Rust side (preprocess / extract / pack / assemble).
                Rectangle {
                    id: layoutProgressOverlay
                    visible: appController.isLayoutInProgress
                    z: 20
                    anchors.centerIn: parent
                    width:  Math.min(parent.width  * 0.6, 360)
                    implicitHeight: progressColumn.implicitHeight + 24
                    radius: 8
                    color:  Qt.rgba(Theme.violetDark.r,
                                    Theme.violetDark.g,
                                    Theme.violetDark.b, 0.92)
                    border.color: Theme.violetLight
                    border.width: 1

                    ColumnLayout {
                        id: progressColumn
                        anchors.centerIn: parent
                        width: parent.width - 32
                        spacing: 8

                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text:  appController.layoutStatusMessage !== ""
                                   ? appController.layoutStatusMessage
                                   : "Generating layout…"
                            color: Theme.textOnDark
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                        } // Text label

                        ProgressBar {
                            id: layoutProgressBar
                            Layout.fillWidth: true
                            from:  0
                            to:    100
                            // Treat the idle sentinel (-1) as 0 so the bar
                            // starts empty rather than showing a stale value.
                            value: Math.max(0, appController.layoutProgress)
                        } // ProgressBar

                        Text {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text:  Math.max(0, appController.layoutProgress) + "%"
                            color: Theme.textOnDark
                            font.pixelSize: 12
                        } // Text percent
                    } // ColumnLayout progressColumn
                } // Rectangle layoutProgressOverlay
            } // Rectangle rightCanvasFrame

            // Reload AdjustWindow after each accepted adjustment batch so the
            // next drag starts from the newly committed piece positions.
            Connections {
                target: appController
                function onAdjustApplied() {
                    adjustController.launchAdjustWindow(
                        appController.saveAdjustDom(),
                        appController.getAdjustPieceBoxes("adjust_dom.svg")
                    )
                } // onAdjustApplied
            } // Connections

            // Close AdjustWindow after the right canvas finishes loading the
            // updated SVG — prevents a flash of stale layout content.
            Connections {
                target: rightCanvas
                function onSvgLoaded() {
                    if (root.pendingAdjustClose) {
                        root.pendingAdjustClose = false
                        adjustController.closeAdjustWindow()
                    } // if pending close
                } // onSvgLoaded
            } // Connections rightCanvas.svgLoaded
        } // RowLayout canvas row

        // Adjust action bar — replaced by AdjustWindow toolbar (Phase 8b).
        // Kept in tree so layout measurements are unchanged; hidden permanently.
        Rectangle {
            id: adjustActionBar
            Layout.fillWidth: true
            implicitHeight:   52
            visible:          false
            color:            Theme.violetDark
        } // Rectangle adjustActionBar
    } // ColumnLayout

    // -----------------------------------------------------------------------
    // Layout Settings dialog (Phase 5)
    // -----------------------------------------------------------------------
    SettingsDialog {
        id: settingsDialog
        model: settingsModel

        // When the user clicks Submit, build the initial canvas (blank page + content
        // rectangle) from the submitted settings and display it in the right canvas.
        // This gives immediate visual confirmation that the settings are correct
        // (paper size, margins, roll width, etc.) before pieces are placed.
        // Guard: if adjust state exists, warn before replacing the adjusted layout.
        onAccepted: {
            if (appController.isAdjustMode || appController.isAdjustDirty) {
                replaceLayoutWarningDialog.open()
            } else {
                appController.initializeLayout(settingsModel.toJson())
            } // if adjust state
        } // onAccepted
    } // SettingsDialog settingsDialog

    // -----------------------------------------------------------------------
    // Preferences controller — owns QtWidgets PreferencesWindow (Phase 6b)
    // -----------------------------------------------------------------------
    PreferencesController {
        id: preferencesController
        preferencesModel: preferencesModel

        // Reset-to-defaults flow: after preferences defaults are applied,
        // reload settings from the resolved settings file and regenerate the
        // layout baseline to clear stale layout state.
        onDefaultsReset: {
            var settingsPath = preferencesModel.settingsFilePath()
            settingsModel.load(settingsPath)
            appController.initializeLayout(settingsModel.toJson())
        } // onDefaultsReset
    } // PreferencesController preferencesController

    // -----------------------------------------------------------------------
    // Teaching-version dialog — Standard vs. Teaching DXF export (Phase 9)
    // -----------------------------------------------------------------------
    DxfTeachingDialog {
        id: dxfTeachingDialog
        onAccepted: {
            // Stage the DXF export and show the progress popup.
            // exportStartTimer fires after one paint tick so the popup renders
            // before the synchronous Rust export blocks the UI thread.
            root.pendingExportPath = root.pendingDxfPath
            root.pendingExportFormat = "dxf"
            root.pendingExportTeachingVersion = dxfTeachingDialog.teachingVersion
            exportProgressPopup.open()
            exportStartTimer.restart()
        } // onAccepted
    } // DxfTeachingDialog dxfTeachingDialog

    // -----------------------------------------------------------------------
    // View teaching-file dialog — V.2: non-modal prompt shown after opening a
    // DXF-ASTM file in the viewer when a companion .txt teaching file is found.
    // The dialog is non-modal so the DXF viewer launch is not blocked.
    // -----------------------------------------------------------------------
    ViewDxfTeachingDialog {
        id: viewDxfTeachingDialog
        onAccepted: {
            // Open the teaching file in the system default text editor.
            // localFileToUrl converts the absolute path to a file:// URL so
            // Qt.openUrlExternally can hand it to the OS shell handler.
            console.log("[Main.qml viewDxfTeachingDialog] onAccepted(): opening teaching file=" + viewDxfTeachingDialog.teachingFilePath)
            Qt.openUrlExternally(preferencesModel.localFileToUrl(viewDxfTeachingDialog.teachingFilePath))
        } // onAccepted
    } // ViewDxfTeachingDialog viewDxfTeachingDialog

    // -----------------------------------------------------------------------
    // Export success dialog — shown after export_finished signal (Phase 9)
    // -----------------------------------------------------------------------
    Dialog {
        id: exportSuccessDialog

        property string exportPath: ""

        title:    "Export Complete"
        modal:    true
        width:    420
        anchors.centerIn: parent

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       4
        } // background Rectangle

        contentItem: Column {
            spacing:       8
            topPadding:    12
            bottomPadding: 8
            leftPadding:   16
            rightPadding:  16

            Text {
                text:           "File saved:"
                color:          Theme.textOnDark
                font.pixelSize: Theme.fontSizeSmall
                font.bold:      true
            } // Text label

            Text {
                text:           exportSuccessDialog.exportPath
                color:          Theme.textOnDark
                font.pixelSize: Theme.fontSizeSmall
                wrapMode:       Text.WrapAnywhere
                width:          360
            } // Text path
        } // Column contentItem

        footer: DialogButtonBox {
            // The dialog only confirms the save location. Viewing the exported
            // file is intentionally not offered here — the sole in-app path to
            // view a DXF (or any export) is the View feature set / menu.
            Button {
                text: "Close"
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            } // Button close
        } // DialogButtonBox footer
    } // Dialog exportSuccessDialog

    // -----------------------------------------------------------------------
    // File dialog — SVG import (Phase 7 wires to AppController.importSvg)
    // -----------------------------------------------------------------------
    FileDialog {
        id: importDialog
        title:        "Open SVG Pattern"
        nameFilters:  ["SVG Files (*.svg)", "All Files (*)"]
        onAccepted: {
            // Convert the file:// URL to a local path via PreferencesModel.urlToLocalFile().
            // This uses QUrl::toLocalFile() internally, which correctly handles Windows
            // drive letters ("file:///C:/..." → "C:/...") and Unix absolute paths
            // ("file:///home/..." → "/home/...") without platform-specific string hacks.
            var localPath = preferencesModel.urlToLocalFile(selectedFile.toString())
            // Shared entry point — also used by the command-line handoff (Task 49).
            root.openSvgFile(localPath)
        } // onAccepted
    } // FileDialog importDialog

    // -----------------------------------------------------------------------
    // Adjust Conflicts dialog — shown when Accept is clicked with overlapping pieces
    // -----------------------------------------------------------------------
    Dialog {
        id: adjustConflictsDialog

        property var conflictIds: []

        title:           "Overlap Conflicts"
        modal:           true
        width:           380
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       4
        } // background Rectangle

        contentItem: Column {
            spacing:       8
            topPadding:    12
            bottomPadding: 8
            leftPadding:   16
            rightPadding:  16

            Text {
                text:           "The following pieces overlap or extend outside the layout area:"
                color:          Theme.textOnDark
                font.pixelSize: Theme.fontSizeSmall
                wrapMode:       Text.WordWrap
                width:          320
            } // Text description

            Text {
                text:           adjustConflictsDialog.conflictIds.join(", ")
                color:          Theme.violetLight
                font.pixelSize: Theme.fontSizeSmall
                wrapMode:       Text.WordWrap
                width:          320
            } // Text conflictList

            Text {
                text:           "Proceed and apply adjustments anyway?"
                color:          Theme.textOnDark
                font.pixelSize: Theme.fontSizeSmall
            } // Text prompt
        } // Column contentItem

        // Ok = "Proceed anyway"; applies the pending QtWidgets adjustment batch.
        onAccepted: appController.acceptAdjustments(root.pendingAdjustTransforms)
    } // Dialog adjustConflictsDialog

    // -----------------------------------------------------------------------
    // Adjust Discard dialog — confirms discard when canvas has unsaved changes
    // -----------------------------------------------------------------------
    Dialog {
        id: adjustDiscardDialog

        title:           "Discard Adjustments"
        modal:           true
        width:           340
        anchors.centerIn: parent
        standardButtons: Dialog.Yes | Dialog.Cancel
        padding:         16

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       4
        } // background Rectangle

        contentItem: Text {
            text:           "Discard all changes to the adjusted layout?"
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeNormal
            wrapMode:       Text.WordWrap
            width:          300
        } // contentItem Text

        onAccepted: appController.discardAdjustments()
    } // Dialog adjustDiscardDialog

    // -----------------------------------------------------------------------
    // Replace Layout Warning dialog — shown when Settings submitted while
    // an adjusted layout exists (isAdjustMode or isAdjustDirty)
    // -----------------------------------------------------------------------
    Dialog {
        id: replaceLayoutWarningDialog

        title:           "Replace Adjusted Layout"
        modal:           true
        width:           340
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        padding:         16

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       4
        } // background Rectangle

        contentItem: Text {
            text:           "This will replace your adjusted layout. Continue?"
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeNormal
            wrapMode:       Text.WordWrap
            width:          300
        } // contentItem Text

        onAccepted: appController.initializeLayout(settingsModel.toJson())
    } // Dialog replaceLayoutWarningDialog

    // -----------------------------------------------------------------------
    // Error dialog
    // -----------------------------------------------------------------------
    Dialog {
        id: errorDialog

        property string errorText: ""

        title:    "Error"
        modal:    true
        width:    360
        anchors.centerIn: parent
        standardButtons: Dialog.Ok

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       4
        } // background Rectangle

        contentItem: Text {
            text:           errorDialog.errorText
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeNormal
            wrapMode:       Text.WordWrap
            width:          300
        } // contentItem Text
    } // Dialog errorDialog

    // -----------------------------------------------------------------------
    // Warning dialog — layout completed but some pieces were left out
    // -----------------------------------------------------------------------
    Dialog {
        id: warningDialog

        property string warningText: ""

        title:    "Some pieces could not be placed"
        modal:    true
        width:    400
        anchors.centerIn: parent
        standardButtons: Dialog.Ok

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       4
        } // background Rectangle

        contentItem: Text {
            text:           warningDialog.warningText
            color:          Theme.textOnDark
            font.pixelSize: Theme.fontSizeNormal
            wrapMode:       Text.WordWrap
            width:          340
        } // contentItem Text
    } // Dialog warningDialog

    // -----------------------------------------------------------------------
    // Export progress popup — shown for all export formats.
    //
    // Opened by each export handler before the synchronous Rust call starts
    // (via exportStartTimer) so the user sees feedback while the UI thread is
    // blocked.  Closed on export_finished or error_occurred.
    //
    // The ProgressBar is bound to appController.exportProgress (0–100; -1 idle).
    // The status label is bound to appController.exportStatusMessage.
    // -----------------------------------------------------------------------
    Popup {
        id: exportProgressPopup
        modal: true
        focus: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.NoAutoClose
        dim: true

        background: Rectangle {
            color:        Theme.dialogBackground
            border.color: Theme.violetDark
            radius:       6
        } // background Rectangle

        contentItem: ColumnLayout {
            spacing: 10
            width: 280

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                // Show Rust-supplied status message or a generic fallback.
                text: appController.exportStatusMessage !== ""
                      ? appController.exportStatusMessage
                      : "Exporting, please wait…"
                color: Theme.textOnDark
                font.pixelSize: Theme.fontSizeNormal
                wrapMode: Text.WordWrap
            } // Text status

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to:   100
                // Treat idle sentinel (-1) as 0 so bar starts empty rather
                // than wrapping to a stale value.
                value: Math.max(0, appController.exportProgress)
            } // ProgressBar

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: Math.max(0, appController.exportProgress) + "%"
                color: Theme.textOnDark
                font.pixelSize: 12
            } // Text percent
        } // ColumnLayout

        padding: 16
    } // Popup exportProgressPopup
}
