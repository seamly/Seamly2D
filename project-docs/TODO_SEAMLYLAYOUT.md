# TODO — SeamlyLayout app features

Tasks that add features to the SeamlyLayout layout app.

See `project-docs/PROJECT_PLAN.md` for full details. Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

## Task 21 — SeamlyLayout: three text modes for SVG export in the Exports menu

Replace the single "SVG" item in the SeamlyLayout Exports menu (`src/app/seamlylayout/qt_frontend/qml/ExportMenu.qml`, `exportSvgRequested()` wired through `Main.qml` to the Rust backend in `src/app/seamlylayout/crates/cxxqt_bridge/src/exports.rs`) with three SVG export modes differing in how label text is written.

1. **Text as `<text>` in the designer's selected (outline) font** — searchable, editable, re-stylable, human- and machine-readable; embeds the font via `@font-face` so it renders correctly on machines without it; supports tech-pack generation. Smallest file size.
2. **Text as `<text>` in a Hershey/single-line font** — same searchable/editable/machine-readable intent as mode 1, but using a bundled single-line font so the result is also friendly to CAD/CAM tools that resolve text. **Implementation note:** true Hershey fonts are stroke data, not installable outline fonts — for `<text>` + `font-family` to work this mode needs a "hairline" single-line TTF/OTF (an engineered font whose outline doubles back on itself to look like one stroke) embedded via `@font-face`. Known candidates: CamBam Stick Fonts (free, 9 variants, designed for CNC/plotting), MecSoft/Rhino single-stroke fonts, commercial single-line TTF bundles — verify redistribution/embedding license compatibility with the MIT Rust core before bundling. Caveats: hairline TTFs are still doubled-back outlines (stroke width not controllable via the font), and consumers that ignore embedded fonts will substitute — so mode 3 remains the guaranteed-fidelity choice for cutters. Record the font choice and rationale in `DECISIONS.md`.
3. **Text converted to paths (single-stroke)** — each label rendered as single-stroke `<path>` polylines from Hershey glyph data (**decision:** the path conversion uses the Hershey font, not the designer's outline font — a plotter then draws each character in one pen pass instead of tracing hollow glyph contours). Compatible with CAD/CAM/cutters/plotters/engravers with no font dependency in the consumer. Keep the original string machine-readable via `data-*`/`<desc>` on the label group (text is no longer searchable/editable as SVG text).

**Dependency:** all three modes need real `<text>` elements in the incoming `.pieces.svg` (Task 10) — even mode 3 needs the label *strings* to re-render them in stroke glyphs. Already-outlined input (Seamly2D `--text2paths`) can only be passed through as-is; the UI must handle path-only input (disable the text modes with an explanatory tooltip, or export the existing paths with a warning). Optional Hershey display/export on the Seamly2D side is Task 22.

- [ ] Replace the single "SVG" `MenuItem` with a three-entry submenu (or dialog choice) in `ExportMenu.qml`; add per-mode tooltips summarizing the compatibility/editability trade-off; wire new signals through `TopMenuBar.qml`/`Main.qml` to the bridge
- [ ] Mode 1: pass `<text>` through in the designer's font and embed the font as a subsetted `@font-face` data-URI (WOFF/TTF); document the font-licensing caveat (embedding rights vary by font license) in the export docs
- [ ] Mode 2: emit `<text>` styled with the bundled single-line font, embedded via `@font-face`, per the `DECISIONS.md` decision
- [ ] Mode 3: implement single-stroke text rendering in the Rust core — shape each label string into Hershey glyph strokes and emit stroked (fill-less) `<path>` polylines via `svg_dom`; keep the original label string on the group via `data-*`/`<desc>`
- [ ] Bundle Hershey/single-line glyph data under a permissive license compatible with the MIT Rust core (evaluate existing crates, e.g. a Hershey-font crate, before hand-rolling)
- [ ] Preserve the `data-*` tagging contract (`piece_label`/`pattern_label` groups, ids, `data-parent`) identically in all three modes
- [ ] Detect path-only input (no `<text>` in labels) and gate all three text modes accordingly
- [ ] Persist the last chosen SVG text mode in preferences (`PreferencesModel`)
- [ ] Tests: Rust unit tests for each conversion mode (mode 3 output is stroked polylines, not filled contours), plus the path-only-input case; frontend test for menu gating; end-to-end check with the richmond test pattern
- [ ] Update `src/app/seamlylayout/docs/status-docs/svg-data-attributes.md`, the root `project-docs/SVG-DATA-ATTRIBUTES.md` mirror, and `src/app/seamlylayout/docs` export docs
- [ ] Doxygen briefs + inline comments on all touched functions

## Task 26 — Export multisize patterns (nested / marker / sized-layout-set)

Add layout export for multisize patterns — `.sm2d` patterns opened with a `.smms` multisize measurement file (multiple sizes; the CLI already exposes per-size gradation via `--gradationsize`/`--gradationheight`). The user chooses one of three multisize layout products in the settings dialog; all products orient every piece with its grainline pointing up.

- [ ] Settings dialog: user chooses "nested layout", "marker layout", or "set of sized layouts" for multisize export
- [ ] Generate a "size layout" for each size in the `.smms` file, all grainlines pointing up (per-size piece generation via the existing gradation machinery)
- [ ] Nested layout:
  - [ ] For each piece in the largest size, create a layout with all grainlines pointing up
  - [ ] For the remaining sizes in descending order: place each piece on top of its matching largest-size piece, grainline up, centering its center point on the largest piece's center point — each large piece becomes the base of a "pyramid" of matching pieces with the smallest on top
  - [ ] Apply transforms so all pieces are placed in global space
  - [ ] Group all pieces of each size together, so upstream tools (Pattern Projector, Inkscape, Illustrator, ...) can toggle each size's visibility
- [ ] Marker layout: copy all pieces from the size layouts and arrange them into a single marker layout, all grainlines pointing up
- [ ] Set of sized layouts:
  - [ ] Let the user view each size's layout in the canvas — UI design open: per-size tabs across the top of the canvas is the working idea, to be settled during implementation
  - [ ] Export the set to a single multi-page PDF, or to individual files of any export type
- [ ] Tests with a multisize test pattern (need a `.sm2d` + `.smms` fixture); verify grouping/grainline orientation in the exported SVG/PDF
- [ ] Doxygen briefs + inline comments on all touched functions; document the three products in the repo docs
