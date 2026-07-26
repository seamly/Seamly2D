# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

See `PROJECT_PLAN.md` for full details. Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `COMPLETED.md`.

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

## Task 56 — Clear the errors recorded in `BUILD_PROBLEMS.txt` (editor code model, not the compiler)

The request names `BUILD_PROBLEMS.md`; the file in the tree is **`src/app/seamly2d/core/BUILD_PROBLEMS.txt`** (tracked in git, 21 KB) — a copy-paste of the VS Code **Problems** panel as JSON: two concatenated arrays, **45 entries**, `"source": "clang"` ×42 and `"clangd"` ×3, covering exactly two files (`application_2d.cpp` ×21, `vcommonsettings.cpp` ×24).

**They are not build errors.** Both arrays start from a `pp_file_not_found` root — `In included file: '../vmisc/vabstractapplication.h' file not found` (via `application_2d.h:54`) and `In included file: 'QByteArray' file not found` — and the other 43 entries are that cascade: `Unknown type name 'QString'` ×10, `Use of undeclared identifier 'QStringLiteral'` ×14, `QtWarningMsg`/`QtDebugMsg`/`QtMsgType`/`QMessageLogContext`, `Member access into incomplete type 'const QString'` ×5, and `Too many errors emitted, stopping now` ×2. The `../vmisc/…` include form is valid under qmake because each `.pro` adds `INCLUDEPATH += $$PWD/../../libs/<lib>` (`src/app/seamly2d/seamly2d.pro:217+`), and Qt's own headers come from the kit. The repo contains **no** `compile_commands.json`, `.clangd`, `.vscode/c_cpp_properties.json` or `compile_flags.txt`, so the editor's clang(d) parses each translation unit with no include paths at all and everything collapses from the first unresolved include. The same files compile clean with qmake + MSVC (`scripts/sd.ps1`).

So this is editor/tooling configuration plus a decision about the dump file itself — worth doing, because a Problems panel with 45 phantom errors hides real ones.

- [ ] Give the editor a working code model — evaluate and pick one: a checked-in `.clangd` with `CompileFlags.Add` listing the `src/libs/*` include dirs and the Qt 6.11.1 kit includes; a `.vscode/c_cpp_properties.json` for the MS C/C++ extension; or a generated `compile_commands.json` (qmake has no native export, so this means a compile-database recorder or Qt Creator's generator). Record which and why
- [ ] Make it kit-agnostic where possible (or document the one path a developer must edit), so a Qt version bump does not silently stale it — same failure mode as Task 45's hard-coded `C:\Qt\6.10.1` allowlist entries
- [ ] Verify by reopening `src/app/seamly2d/core/application_2d.cpp` and `src/libs/vmisc/vcommonsettings.cpp`: the two `pp_file_not_found` roots and all 42 cascade errors are gone
- [ ] Decide separately on the 3 clangd `unused-includes` hints (severity 4, not errors) — act on them or suppress the check; they are the only entries in the file that point at the source rather than at the tooling
- [ ] Delete `BUILD_PROBLEMS.txt` from `src/app/seamly2d/core/` once resolved. It is a machine-specific editor dump sitting in shipped GPL source beside `application_2d.cpp`, carrying absolute `/c:/Users/susan/…` paths into the eventual upstream PR — the same class of leak as Task 50's hard-coded home path. Keep any record as a task note or in the scratchpad, not in the source tree
- [ ] If anything in the dump turns out to be a genuine defect (an include that is wrong even under qmake, not just unresolvable by clangd), split it out as its own task rather than folding it in here
- [ ] Document the editor setup in `.github/README-DEVELOPER.md` (Task 55) so the next developer does not re-derive it — and note that a bare `qmake` on `PATH` may be Qt Design Studio's reduced Qt (Task 47), which would also poison any generated compile database
