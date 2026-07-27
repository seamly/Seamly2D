# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

See `project-docs/PROJECT_PLAN.md` for full details. Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

## Task 22 — Seamly2D: optional single-stroke (Hershey) label display and export

**Scope decision (2026-07-18):** outline fonts remain fully acceptable in Seamly2D — the existing outline-font canvas display and exports stay the default and are NOT removed or locked out. This task adds single-stroke (Hershey-style) label support as an *option*, so a designer targeting a cutter/plotter can preview labels on the canvas the way the machine will draw them and export matching single-stroke output.

Current label text handling (all outline-font based, and staying available): the canvas paints labels with the user-selected `QFont` via `painter->drawText()` (`VTextGraphicsItem::paint()`, `src/libs/vwidgets/vtextgraphicsitem.cpp`); the `textAsPaths == true` branch of `VLayoutPiece::createLabelItem()` (`src/libs/vlayout/vlayoutpiece.cpp`) outlines glyphs via `QPainterPath::addText()`; DXF text goes through `VDxfEngine::drawTextItem()` (`src/libs/vdxf/vdxfengine.cpp`).

**Constraint:** neither Windows nor Qt has native stroke-font support — the OS/Qt typography stack (`QFont`) only loads outline TTF/OTF, and a true single-stroke font cannot be installed as a system font (Windows errors on it or auto-closes the open contours into filled shapes). Two known workarounds exist in the wild:

1. **App-internal stroke-glyph data** (the true single-stroke route) — bypass the system font stack entirely and render strokes from Hershey glyph data inside the app, as Inkscape's *Hershey Text* extension does (Hershey Sans/Serif/Script 1-stroke, Gothic, and Duplex/Triplex multi-stroke variants). **This is the approach for this task** — the custom renderer below; prior art also includes Valentina's single-line/SVG-font label support.
2. **"Hairline" TTFs** — engineered fonts that trace each line forward and back over itself so the closed outline *looks* like a single stroke (e.g. CamBam Stick Fonts — free, 9 variants for CNC/plotting; MecSoft/Rhino single-stroke; commercial single-line TTF bundles). These install as normal Windows fonts and work in ordinary apps, but they are still outlines (doubled-back), stroke width isn't controllable via the font, and licensing must be checked before bundling. Relevant mainly as the embeddable-`<text>` option in Task 21 mode 2, not for this task's renderer.

- [ ] Choose and bundle the stroke-glyph source (Hershey glyph data and/or single-line SVG fonts) under a GPL-compatible license; record the decision in the repo docs
- [ ] Implement a single-stroke text renderer: shape a label line into stroked (fill-less) `QPainterPath` polylines with proper advance/kerning, honoring size, bold/italic variants (stroke width/slant), alignment, eliding, mirroring, and rotation
- [ ] Add the option to the label settings UI: extend the label font selection (preferences Graphics View page / piece label settings) with the bundled single-stroke fonts alongside the existing system outline fonts; outline stays the default
- [ ] Canvas: when a single-stroke font is selected, render labels with the stroke renderer in `VTextGraphicsItem` so the preview matches what a plotter will draw; outline-font labels keep the existing `painter->drawText()` path
- [ ] Export: when a single-stroke font is selected, the `textAsPaths == true` branch of `createLabelItem()` emits single-stroke paths for those labels (outline-font labels keep the existing `QPainterPath::addText()` conversion); DXF export of single-stroke labels emits polylines
- [ ] Coordinate with Task 10/Task 21: labels in a single-stroke font exporting as `<text>` should reference the bundled single-line font name so SeamlyLayout can match it
- [ ] Verify: canvas single-stroke labels legible at typical zooms with correct placement, mirroring, and rotation; outline-font behavior unchanged everywhere; tagged pieces SVG / `.pieces.svg` / `--text2paths` / DXF / PDF / PNG correct in both font modes
- [ ] Doxygen briefs + inline comments on all touched functions; document the font architecture in the repo docs

## Task 25 — Audit and fix the Seamly2D CLI so all options work

Go through every command-line option seamly2d advertises (defined in `src/libs/vmisc/commandoptions.cpp`, wired in `src/app/seamly2d/core/vcmdexport.cpp`) and make each one actually work in console export mode. Known friction from Task 11 verification: option names are case-sensitive and inconsistently cased (e.g. `--exportOnlyDetails`), and errors only surface in a redirected stderr, not on the console.

- [ ] Inventory all options and build a test matrix: expected behavior, required companions (e.g. `--basename` enabling export mode), valid values
- [ ] Exercise each option against the richmond test pattern (all export formats, gradation size/height, page options, `--text2paths`, measurement overrides, etc.) and record which are broken, ignored, or misdocumented
- [ ] Fix the broken/ignored options; make error messages reach the console reliably (the GUI-subsystem exe detaches from the console — evaluate `AttachConsole`/subsystem handling on Windows so `--help` and errors print without redirection)
- [ ] Consider case-insensitive or consistently lowercase option aliases (keeping the existing names working for compatibility)
- [ ] Unit tests: extend `tst_vcommandline` to cover every option and the failure modes found
- [ ] Update `--help` text and repo docs with the verified behavior

## Task 42 — Seamly2D: default the Open dialog to the user measurements/individual directory

Today Seamly2D's **Open** dialog starts in the last-opened file's folder, or `QDir::homePath()` when there is no recent file (`src/app/seamly2d/mainwindow.cpp:4357-4372`). The request is to have its file picker open to `<seamly_user_directory>\measurements\individual` — the individual-measurements folder, which SeamlyMe already derives from `VCommonSettings::getIndividualSizePath()` (default `<dataRoot>/measurements/individual`, `src/libs/vmisc/vcommonsettings.cpp:429`). `<seamly_user_directory>` = the shared relocatable data root (Task 34).

**Flag — confirm the intended dialog first:** Seamly2D's `Open` loads `.sm2d`/`.val` **pattern** files, which are *not* stored under `measurements/individual`; pointing the pattern picker there would show an empty/filtered list. The semantically matching dialogs are Seamly2D's **Load Individual / Load Multisize measurements** (`MainWindow::LoadIndividual()` ~line 2047, `LoadMultisize()` ~line 2088), which parallel SeamlyMe's Open* (Task 43). Decide whether this targets (a) the pattern `Open` dialog literally, or (b) the measurement-load dialogs, before implementing.

- [ ] Confirm the target dialog (pattern `Open` vs `Load Individual`/`Load Multisize`) per the flag above
- [ ] Point the chosen dialog's initial directory at `getIndividualSizePath()` (creating the folder if absent, as SeamlyMe's `OpenIndividual()` does) instead of `homePath()`/last-file-dir
- [ ] Derive the path from the shared relocatable data root (Task 34) so it tracks a user-configured/renamed `<seamly_user_directory>`, not the hardcoded `~/seamly2d`
- [ ] Verify the picker opens at the correct folder both with and without an existing `measurements/individual` directory
- [ ] Doxygen briefs + inline comments on the touched function(s)
