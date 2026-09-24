# TODO — SeamlyLayout app features

Tasks that add features to the SeamlyLayout layout app.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

Tasks in this file are numbered and are prefixed with `Layout.`

## [x] Task Layout.01 - Open seamlylayout.exe

- [x] Layout.01 While in 'Piece Mode' in Seamly2D, pressing the 'Layout' button on the menu should immediately run seamlylayout.exe without displaying the old Seamly2D layout canvas or calling vlayout which has been superceded by the seamlylayout.exe application. The old vlayout code has been superceded by seamlylayout.exe

## [x] Task Layout.02 - Layout Settings

- [x] Layout.02 - In the settings dialog, section 'Layout Mode', add a checkbox for 'Any'; when 'Any' is checked the layout algorithm should ignore the grainline direction when placing the pieces efficiently.

## [ ] Task Layout.03 - layout algorithm improvement

- [x] Layout.031 - The layout algorithm has suffered regression - the pieces are nested sub-optimally (there is a lot of space between the pieces). Fix the layout algorithm so that the pieces only have the gap specified between them.

- [ ] Layout.032 - Implement the 'Layout Mode' == 'None' option so that the grainline direction is ignored while the pieces are efficiently arranged.

## [ ] Task Layout.1 - 'Adjust Mode' improvement

- [x] Layout.11 - The 'Adjust Layout' feature has suffered regression - when 'Adjust Layout' opens there are no pattern pieces to adjust - the pieces from the main SeamlyLayout canvas should be available in 'Adjust Layout', otherwise there is nothing to adjust and save back to the main SeamlyLayout canvas.

- [ ] Layout.12 - When Adjust Mode closes it should return focus to SeamlyLayout's right canvas

- [ ] Layout.13 - When focus returns to the right canvas after 'adjust mode' closes, the right canvas should contain either:
  - [ ] Layout.13.1 - the layout with updates from 'adjust mode' ('adjust mode' closed via 'Save')
  - [ ] Layout.13.2 - the layout without updates from 'adjust mode' ('adjust mode' closed via 'Cancel')

## [ ] Task Layout.2 - in 'Export SVG' menu selection, create three options for text mode

Replace the single "SVG" item in the SeamlyLayout Exports menu (`src/app/seamlylayout/qt_frontend/qml/ExportMenu.qml`, `exportSvgRequested()` wired through `Main.qml` to the Rust backend in `src/app/seamlylayout/crates/cxxqt_bridge/src/exports.rs`) with three SVG export modes differing in how label text is written.

- [x] Layout.21 - **Text as `<text>` in the designer's selected (outline) font** — searchable, editable, re-stylable, human- and machine-readable; embeds the font via `@font-face` so it renders correctly on machines without it; supports tech-pack generation. Smallest file size.
- [x] Layout.22 - **Text as `<text>` in a Hershey/single-line font** — same searchable/editable/machine-readable intent as mode 1, but using a bundled single-line font so the result is also friendly to CAD/CAM tools that resolve text. **Implementation note:** true Hershey fonts are stroke data, not installable outline fonts — for `<text>` + `font-family` to work this mode needs a "hairline" single-line TTF/OTF (an engineered font whose outline doubles back on itself to look like one stroke) embedded via `@font-face`. Known candidates: CamBam Stick Fonts (free, 9 variants, designed for CNC/plotting), MecSoft/Rhino single-stroke fonts, commercial single-line TTF bundles — verify redistribution/embedding license compatibility with the MIT Rust core before bundling. Caveats: hairline TTFs are still doubled-back outlines (stroke width not controllable via the font), and consumers that ignore embedded fonts will substitute — so mode 3 remains the guaranteed-fidelity choice for cutters. Record the font choice and rationale in `DECISIONS.md`.
- [x] Layout.23 - **Text converted to paths (single-stroke)** — each label rendered as single-stroke `<path>` polylines from Hershey glyph data (**decision:** the path conversion uses the Hershey font, not the designer's outline font — a plotter then draws each character in one pen pass instead of tracing hollow glyph contours). Compatible with CAD/CAM/cutters/plotters/engravers with no font dependency in the consumer. Keep the original string machine-readable via `data-*`/`<desc>` on the label group (text is no longer searchable/editable as SVG text).

**Dependency:** all three modes need real `<text>` elements in the incoming svg file or stringified svg variable (Task 10) — even mode 3 needs the label *strings* to re-render them in stroke glyphs. Already-outlined input (Seamly2D `--text2paths`) can only be passed through as-is; the UI must handle path-only input (disable the text modes with an explanatory tooltip, or export the existing paths with a warning). Optional Hershey display/export on the Seamly2D side is Task 22.

- [x] Layout.24 - Replace the single "SVG" `MenuItem` with a three-entry submenu (or dialog choice) in `ExportMenu.qml`; add per-mode tooltips summarizing the compatibility/editability trade-off; wire new signals through `TopMenuBar.qml`/`Main.qml` to the bridge
- [x] Layout.25 - Mode 1: pass `<text>` through in the designer's font and embed the font as a subsetted `@font-face` data-URI (WOFF/TTF); document the font-licensing caveat (embedding rights vary by font license) in the export docs
- [x] Layout.26 -  Mode 2: emit `<text>` styled with the bundled single-line font, embedded via `@font-face`, per the `DECISIONS.md` decision
- [x] Layout.27 -  Mode 3: implement single-stroke text rendering in the Rust core — shape each label string into Hershey glyph strokes and emit stroked (fill-less) `<path>` polylines via `svg_dom`; keep the original label string on the group via `data-*`/`<desc>`
- [x] Layout.28 - Bundle Hershey/single-line glyph data under a permissive license compatible with the MIT Rust core (evaluate existing crates, e.g. a Hershey-font crate, before hand-rolling)
- [x] Layout.29 - Preserve the `data-*` tagging contract (`piece_label`/`pattern_label` groups, ids, `data-parent`) identically in all three modes
- [ ] Layout.210 - Detect path-only input (no `<text>` in labels) and gate all three text modes accordingly
  - [x] Layout.210.1 - Persist the last chosen SVG text mode in preferences (`PreferencesModel`)
  - [ ] Layout.210.2 - Tests: Rust unit tests for each conversion mode (mode 3 output is stroked polylines, not filled contours), plus the path-only-input case; frontend test for menu gating; end-to-end check with the richmond test pattern
    - Done: 13 Rust tests in `crates/svg_label_text` (all modes, path-only input, menu gating state), 3 `PreferencesModelTests`; all three modes rendered in Edge from the trousers fixture. Menu gating has no QML harness (plan decision 5).
    - Open: in-app check of Export > SVG (submenu, tooltips, check mark, export) with the richmond pattern.
  - [x] Layout.210.3 - Update `project-docs/seamlylayout-docs/status-docs/seamlylayout_svg-data-attributes.md`, the root `project-docs/SVG-DATA-ATTRIBUTES.md` mirror, and `project-docs/seamlylayout-docs` export docs
  - [x] Layout.210.4 - Doxygen briefs + inline comments on all touched functions

## [ ] Task Layout 3 — if current pattern is 'multisize', create three multisize options ('nested' / 'marker' / 'sized-layout-set') that is required before user can select the export file format

Add a layout export option for multisize patterns — svg files or stringified svg variables that contain a measurement file reference to a `.smms` multisize file (multiple sizes; the CLI already exposes per-size gradation via `--gradationsize`/`--gradationheight`). The user chooses one of three multisize layout products in the settings dialog; all products orient every piece with its grainline pointing up.

- [ ] Layout.30 on Import of svg file or stringified svg variable, detect if .sm2d file reference an .smis measurement file (individual measurements) or .smms measurement file (multisize measurements), this variable should be readable (not writable) by the Export menu, layout algorithm, 'Adjust mode', and other code.
- [ ] Layout.31 Settings dialog: user chooses "nested layout", "marker layout", or "set of sized layouts" for multisize export
- [ ] Layout.32 Generate a "size layout" for each size in the `.smms` file, all grainlines pointing up (per-size piece generation via the existing gradation machinery)
- [ ] Layout.33 Nested layout:
  - [ ] Layout.33.1 For each piece in the largest size, create a layout with all grainlines pointing up
  - [ ] Layout.33.2 For the remaining sizes in descending order: place each piece on top of its matching largest-size piece, grainline up, centering its center point on the largest piece's center point — each large piece becomes the base of a "pyramid" of matching pieces with the smallest on top
  - [ ] Layout.33.3 Apply transforms so all pieces are placed in global space
  - [ ] Layout.33.4 Group all pieces of each size together, so upstream tools (Pattern Projector, Inkscape, Illustrator, ...) can toggle each size's visibility
- [ ] Layout.34 Marker layout: copy all pieces from the size layouts and arrange them into a single marker layout, all grainlines pointing up
- [ ]  Layout 35 Set of sized layouts:
  - [ ] Layout.35.1 Let the user view each size's layout in the canvas — UI design open: per-size tabs across the top of the canvas is the working idea, to be settled during implementation
  - [ ] Layout.35.2 Export the set to a single multi-page PDF, or to individual files of any export type
- [ ] Layout.36 Tests with a multisize test pattern (need a `.sm2d` + `.smms` fixture); verify grouping/grainline orientation in the exported SVG/PDF
- [ ] Layout.37 Doxygen briefs + inline comments on all touched functions; document the three products in the repo docs

## [ ] Task Layout.4 — One writer for the SeamlyLayout debug log

The log file has **two independent writers and they overwrite each other**, so
lines get clipped mid-string and a message that was written can look absent:

- C++ `Logger` (`src/app/seamlylayout/qt_frontend/src/Logger.h`) holds a static
  `QFile s_file` plus a **buffered** `QTextStream s_stream` open on the file for
  the life of the process.
- Rust `log_to_file()` (`src/app/seamlylayout/crates/cxxqt_bridge/src/lib.rs:457`,
  debug builds only) opens the **same path** with `OpenOptions::append(true)` and
  closes it on every call.

Two file handles with independent positions, one of them buffered, means each
side's flush can land on top of the other's bytes. Until this is fixed: do not
conclude a log line is absent because it looks truncated — grep for a
distinctive fragment instead.

- [ ] Layout.41 Decide the single owner of the file and record it in
  `DECISIONS.md` — either the Rust side logs through the C++ `Logger` across the
  cxx-qt bridge, or `Logger` stops holding the file open and both sides
  append-and-close per line. Prefer one writer over trying to interleave two
- [ ] Layout.42 Implement the chosen design; keep the existing line format
  (`[unix_seconds] DEBUG: message`) so current logs stay readable and both call
  sites' signatures stay unchanged (~20 Rust call sites in `lib.rs`,
  `layout_utils.rs` and `exports.rs`; `Logger::log()` on the C++ side)
- [ ] Layout.43 Serialize concurrent writes — the Rust bridge can be called from
  a non-GUI thread, so whatever owns the file needs a mutex or an equivalent
  guarantee
- [ ] Layout.44 Keep the release-build behaviour: `log_to_file()` is a no-op
  when `debug_assertions` is off, and that must not regress
- [ ] Layout.45 Test: write interleaved lines from both sides (and from two
  threads) and assert every line arrives whole and in order
- [ ] Layout.46 Doxygen briefs + inline comments on all touched functions

## [ ] Task Layout.5 - Implement additional export formats

- [ ] Layout.51 DXF-AAMA — biggest install base in apparel PLM, reference implementation already in the repo
- [ ] Layout.52 HPGL — unlocks the whole plotter and cutter class
- [ ] Layout.53 PS/EPS — one writer covers both and retires pdftops
- [ ] Layout.54 JPG — trivial, fit it anywhere
- [ ] Layout.55 DXF-ASTM - improve current export so that it meets the DXF-ASTM standard

## [ ] Task Layout.6 - Implement export stubs for paid export modules (To be developed)

These options in the Export menu will remain invisible to the user until these features are developed. Put stubs in the code now to mark where they will go.

- [ ] Layout.61 Export G-Code
- [ ] Layout.62 Export 3DMesh

## [x] Task Layout.8 — SeamlyLayout default paths don't resolve under %DATAROOT%

## [x] Task Layout.9 — Piece-mode handoff passes a file, not a stringified SVG document

