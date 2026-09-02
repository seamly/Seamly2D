# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

Tasks in this file begin with `Seamly2D.`

## Task Seamly2D.1 — Seamly2D: optional single-stroke (Hershey) label display and export

**Scope decision (2026-07-18):** outline fonts remain fully acceptable in Seamly2D — the existing outline-font canvas display and exports stay the default and are NOT removed or locked out. This task adds single-stroke (Hershey-style) label support as an *option*, so a designer targeting a cutter/plotter can preview labels on the canvas the way the machine will draw them and export matching single-stroke output.

Current label text handling (all outline-font based, and staying available): the canvas paints labels with the user-selected `QFont` via `painter->drawText()` (`VTextGraphicsItem::paint()`, `src/libs/vwidgets/vtextgraphicsitem.cpp`); the `textAsPaths == true` branch of `VLayoutPiece::createLabelItem()` (`src/libs/vlayout/vlayoutpiece.cpp`) outlines glyphs via `QPainterPath::addText()`; DXF text goes through `VDxfEngine::drawTextItem()` (`src/libs/vdxf/vdxfengine.cpp`).

**Constraint:** neither Windows nor Qt has native stroke-font support — the OS/Qt typography stack (`QFont`) only loads outline TTF/OTF, and a true single-stroke font cannot be installed as a system font (Windows errors on it or auto-closes the open contours into filled shapes). Two known workarounds exist in the wild:

1. **App-internal stroke-glyph data** (the true single-stroke route) — bypass the system font stack entirely and render strokes from Hershey glyph data inside the app, as Inkscape's *Hershey Text* extension does (Hershey Sans/Serif/Script 1-stroke, Gothic, and Duplex/Triplex multi-stroke variants). **This is the approach for this task** — the custom renderer below; prior art also includes Valentina's single-line/SVG-font label support.
2. **"Hairline" TTFs** — engineered fonts that trace each line forward and back over itself so the closed outline *looks* like a single stroke (e.g. CamBam Stick Fonts — free, 9 variants for CNC/plotting; MecSoft/Rhino single-stroke; commercial single-line TTF bundles). These install as normal Windows fonts and work in ordinary apps, but they are still outlines (doubled-back), stroke width isn't controllable via the font, and licensing must be checked before bundling. Relevant mainly as the embeddable-`<text>` option in Task 21 mode 2, not for this task's renderer.

- [ ] Seamly2D.1.1 Choose and bundle the stroke-glyph source (Hershey glyph data and/or single-line SVG fonts) under a GPL-compatible license; record the decision in the repo docs
- [ ] Seamly2D.1.2 Implement a single-stroke text renderer: shape a label line into stroked (fill-less) `QPainterPath` polylines with proper advance/kerning, honoring size, bold/italic variants (stroke width/slant), alignment, eliding, mirroring, and rotation
- [ ] Seamly2D.1.3 Add the option to the label settings UI: extend the label font selection (preferences Graphics View page / piece label settings) with the bundled single-stroke fonts alongside the existing system outline fonts; outline stays the default
- [ ] Seamly2D.1.4 Canvas: when a single-stroke font is selected, render labels with the stroke renderer in `VTextGraphicsItem` so the preview matches what a plotter will draw; outline-font labels keep the existing `painter->drawText()` path
- [ ] Seamly2D.1.5 Export: when a single-stroke font is selected, the `textAsPaths == true` branch of `createLabelItem()` emits single-stroke paths for those labels (outline-font labels keep the existing `QPainterPath::addText()` conversion); DXF export of single-stroke labels emits polylines
- [ ] Seamly2D.1.6 Coordinate with Task 10/Task 21: labels in a single-stroke font exporting as `<text>` should reference the bundled single-line font name so SeamlyLayout can match it
- [ ] Seamly2D.1.7 Verify: canvas single-stroke labels legible at typical zooms with correct placement, mirroring, and rotation; outline-font behavior unchanged everywhere; tagged pieces SVG / `.pieces.svg` / `--text2paths` / DXF / PDF / PNG correct in both font modes
- [ ] Seamly2D.1.8 Doxygen briefs + inline comments on all touched functions; document the font architecture in the repo docs

## Task Seamly2D.3 — Restore Piece Mode focus after closing SeamlyLayout

Found during MSI Test Case 1, step 7b-iv: closing SeamlyLayout returns focus to Seamly2D's Layout Mode, not the Piece Mode that was active before SeamlyLayout launched.

- [ ] Seamly2D.3.1 Record the Seamly2D mode active when SeamlyLayout launches, and restore that mode (not Layout Mode) when SeamlyLayout closes.

## Task Seamly2D.4 — Preferences > Paths has no row for bodyscans

Found during MSI Test Case 1 verification (`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`, step 7a).

`PreferencesPathPage::Apply()` (`src/app/seamly2d/dialogs/configpages/preferencespathpage.cpp:100-119`) reads table rows 0–9 (data root, pattern, template, individual, multisize, layout, label template, image, backup, SeamlyLayout app path) but has no row for bodyscans. `VCommonSettings::setBodyScansPath()`/`getBodyScansPath()` (`src/libs/vmisc/vcommonsettings.cpp:1311-1323`) exist and target `qt6_common.ini`'s `paths/bodyscans` key, but nothing in the UI ever calls the setter, so that key never gets written — even after visiting Preferences and clicking Apply/OK.

- [x] Seamly2D.4.1 Add a "My Body Scans" row to the Preferences > Paths table (`preferencespathpage.cpp`, alongside the existing Patterns/Templates/Measurements/Layouts/Label Templates/Images/Backups rows) and wire it to `getBodyScansPath()`/`setBodyScansPath()`.
- [ ] Seamly2D.4.2 Re-run MSI Test Case verification step 7a to confirm a `bodyscans` key appears in `qt6_common.ini` after visiting Preferences > Paths.

## Task Seamly2D.5 — Piece-mode handoff passes a file, not a stringified SVG document

Found during MSI Test Case 1 verification (`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`, step 6b-ii). Cross-reference: `Layout.9` in `TODO_SEAMLYLAYOUT.md` is the SeamlyLayout-side half of this same task.

`MainWindow::exportPiecesToSeamlyLayout()` (`mainwindow.cpp:4153`) writes the pieces to `<pattern-basename>.pieces.svg` next to the pattern file and launches SeamlyLayout detached with that file path as its one positional argument (`StartupOptions.{h,cpp}`, `SeamlySuitePaths::seamlyLayoutLaunchArguments()`). The MSI test plan's expectation is that piece-mode data reaches SeamlyLayout as a stringified SVG document, not as a file — so either the test expectation is stale or this handoff needs to change to pass the SVG content directly instead of a file path. See `Layout.9` for the resolution subtasks; do not duplicate work between the two.
