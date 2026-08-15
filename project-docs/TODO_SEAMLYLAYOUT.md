# TODO — SeamlyLayout app features

Tasks that add features to the SeamlyLayout layout app.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

Tasks in this file are numbered and are prefixed with `Layout.`

## Task Layout.0 - layout algorithm improvement

- [ ] Layout 0.1 - The layout algorithm has suffered regression - the pieces are nested sub-optimally (there is a lot of space between the pieces). Fix the layout algorithm so that the pieces only have the gap specified between them.  

## Task Layout.1 - 'Adjust Layout' improvement

- [ ] Layout 1.1 - The 'Adjust Layout' feature has suffered regression - when 'Adjust Layout' opens there are no pattern pieces to adjust - the pieces from the main SeamlyLayout canvas should be available in 'Adjust Layout', otherwise there is nothing to adjust and save back to the main SeamlyLayout canvas.

## Task Layout.2 - add option for three text modes for SVG export in the Exports menu

Replace the single "SVG" item in the SeamlyLayout Exports menu (`src/app/seamlylayout/qt_frontend/qml/ExportMenu.qml`, `exportSvgRequested()` wired through `Main.qml` to the Rust backend in `src/app/seamlylayout/crates/cxxqt_bridge/src/exports.rs`) with three SVG export modes differing in how label text is written.

- [ ] Layout.2.1 - **Text as `<text>` in the designer's selected (outline) font** — searchable, editable, re-stylable, human- and machine-readable; embeds the font via `@font-face` so it renders correctly on machines without it; supports tech-pack generation. Smallest file size.
- [ ] Layout.2.2 - **Text as `<text>` in a Hershey/single-line font** — same searchable/editable/machine-readable intent as mode 1, but using a bundled single-line font so the result is also friendly to CAD/CAM tools that resolve text. **Implementation note:** true Hershey fonts are stroke data, not installable outline fonts — for `<text>` + `font-family` to work this mode needs a "hairline" single-line TTF/OTF (an engineered font whose outline doubles back on itself to look like one stroke) embedded via `@font-face`. Known candidates: CamBam Stick Fonts (free, 9 variants, designed for CNC/plotting), MecSoft/Rhino single-stroke fonts, commercial single-line TTF bundles — verify redistribution/embedding license compatibility with the MIT Rust core before bundling. Caveats: hairline TTFs are still doubled-back outlines (stroke width not controllable via the font), and consumers that ignore embedded fonts will substitute — so mode 3 remains the guaranteed-fidelity choice for cutters. Record the font choice and rationale in `DECISIONS.md`.
- [ ] Layout.2.3 - **Text converted to paths (single-stroke)** — each label rendered as single-stroke `<path>` polylines from Hershey glyph data (**decision:** the path conversion uses the Hershey font, not the designer's outline font — a plotter then draws each character in one pen pass instead of tracing hollow glyph contours). Compatible with CAD/CAM/cutters/plotters/engravers with no font dependency in the consumer. Keep the original string machine-readable via `data-*`/`<desc>` on the label group (text is no longer searchable/editable as SVG text).

**Dependency:** all three modes need real `<text>` elements in the incoming `.pieces.svg` (Task 10) — even mode 3 needs the label *strings* to re-render them in stroke glyphs. Already-outlined input (Seamly2D `--text2paths`) can only be passed through as-is; the UI must handle path-only input (disable the text modes with an explanatory tooltip, or export the existing paths with a warning). Optional Hershey display/export on the Seamly2D side is Task 22.

- [ ] Layout.2.4 - Replace the single "SVG" `MenuItem` with a three-entry submenu (or dialog choice) in `ExportMenu.qml`; add per-mode tooltips summarizing the compatibility/editability trade-off; wire new signals through `TopMenuBar.qml`/`Main.qml` to the bridge
- [ ] Layout.2.5 - Mode 1: pass `<text>` through in the designer's font and embed the font as a subsetted `@font-face` data-URI (WOFF/TTF); document the font-licensing caveat (embedding rights vary by font license) in the export docs
- [ ] Layout.2.6 -  Mode 2: emit `<text>` styled with the bundled single-line font, embedded via `@font-face`, per the `DECISIONS.md` decision
- [ ] Layout.2.7 -  Mode 3: implement single-stroke text rendering in the Rust core — shape each label string into Hershey glyph strokes and emit stroked (fill-less) `<path>` polylines via `svg_dom`; keep the original label string on the group via `data-*`/`<desc>`
- [ ] Layout.2.7 - Bundle Hershey/single-line glyph data under a permissive license compatible with the MIT Rust core (evaluate existing crates, e.g. a Hershey-font crate, before hand-rolling)
- [ ] Layout.2.8 - Preserve the `data-*` tagging contract (`piece_label`/`pattern_label` groups, ids, `data-parent`) identically in all three modes
- [ ] Layout.2.9 - Detect path-only input (no `<text>` in labels) and gate all three text modes accordingly
- [ ] Layout.2.10 - Persist the last chosen SVG text mode in preferences (`PreferencesModel`)
- [ ] Layout.2.11 - Tests: Rust unit tests for each conversion mode (mode 3 output is stroked polylines, not filled contours), plus the path-only-input case; frontend test for menu gating; end-to-end check with the richmond test pattern
- [ ] Layout.2.11 - Update `src/app/seamlylayout/docs/status-docs/svg-data-attributes.md`, the root `project-docs/SVG-DATA-ATTRIBUTES.md` mirror, and `src/app/seamlylayout/docs` export docs
- [ ] Layout.2.13 - Doxygen briefs + inline comments on all touched functions

## Task Layout 3 — Export multisize patterns (nested / marker / sized-layout-set)

Add layout export for multisize patterns — `.sm2d` patterns opened with a `.smms` multisize measurement file (multiple sizes; the CLI already exposes per-size gradation via `--gradationsize`/`--gradationheight`). The user chooses one of three multisize layout products in the settings dialog; all products orient every piece with its grainline pointing up.

- [ ] Layout 3.1 Settings dialog: user chooses "nested layout", "marker layout", or "set of sized layouts" for multisize export
- [ ] Layout 3.2 Generate a "size layout" for each size in the `.smms` file, all grainlines pointing up (per-size piece generation via the existing gradation machinery)
- [ ] Layout 3.3 Nested layout:
  - [ ] Layout 3.3.1 For each piece in the largest size, create a layout with all grainlines pointing up
  - [ ] Layout 3.3.2 For the remaining sizes in descending order: place each piece on top of its matching largest-size piece, grainline up, centering its center point on the largest piece's center point — each large piece becomes the base of a "pyramid" of matching pieces with the smallest on top
  - [ ] Layout 3.3.3 Apply transforms so all pieces are placed in global space
  - [ ] Layout 3.3.4 Group all pieces of each size together, so upstream tools (Pattern Projector, Inkscape, Illustrator, ...) can toggle each size's visibility
- [ ] Layout 3.4 Marker layout: copy all pieces from the size layouts and arrange them into a single marker layout, all grainlines pointing up
- [ ]  Layout 3.5 Set of sized layouts:
  - [ ] Layout 3.5.1 Let the user view each size's layout in the canvas — UI design open: per-size tabs across the top of the canvas is the working idea, to be settled during implementation
  - [ ] Layout 3.5.2 Export the set to a single multi-page PDF, or to individual files of any export type
- [ ] Layout 3.6 Tests with a multisize test pattern (need a `.sm2d` + `.smms` fixture); verify grouping/grainline orientation in the exported SVG/PDF
- [ ] Layout 3.7 Doxygen briefs + inline comments on all touched functions; document the three products in the repo docs

## Task Layout.4 — One writer for the SeamlyLayout debug log

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

- [ ] Layout.4.1 Decide the single owner of the file and record it in
  `DECISIONS.md` — either the Rust side logs through the C++ `Logger` across the
  cxx-qt bridge, or `Logger` stops holding the file open and both sides
  append-and-close per line. Prefer one writer over trying to interleave two
- [ ] Layout.4.2 Implement the chosen design; keep the existing line format
  (`[unix_seconds] DEBUG: message`) so current logs stay readable and both call
  sites' signatures stay unchanged (~20 Rust call sites in `lib.rs`,
  `layout_utils.rs` and `exports.rs`; `Logger::log()` on the C++ side)
- [ ] Layout.4.3 Serialize concurrent writes — the Rust bridge can be called from
  a non-GUI thread, so whatever owns the file needs a mutex or an equivalent
  guarantee
- [ ] Layout.4.4 Keep the release-build behaviour: `log_to_file()` is a no-op
  when `debug_assertions` is off, and that must not regress
- [ ] Layout.4.5 Test: write interleaved lines from both sides (and from two
  threads) and assert every line arrives whole and in order
- [ ] Layout.4.6 Doxygen briefs + inline comments on all touched functions

## Task Layout.5 - Implement additional export formats

- [ ] Layout 5.1 DXF-AAMA — biggest install base in apparel PLM, reference implementation already in the repo
- [ ] Layout 5.2 HPGL — unlocks the whole plotter and cutter class
- [ ] Layout 5.3 PS/EPS — one writer covers both and retires pdftops
- [ ] Layout 5.4 JPG — trivial, fit it anywhere

## Task Layout.6 - Implement export stubs for paid export modules (To be developed)

- [ ] Layout 6.1 Export G-Code
- [ ] Layout 7.1 Export 3DMesh
