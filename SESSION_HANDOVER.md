# Session handover

## Current status

### 2026-11-12 — Layout Mode Any turns a wide piece that fits turned (Layout.802)

- Cause: the oversized-piece check used each piece's `data-rotation`. Seamly2D writes `0` (None) by default, so Any tried 0° only.
- `piece_extractor::turn_wide_pieces_to_fit`: Any, piece too wide at its rule's angles, 90° turn fits → rule becomes quarter turns.
- Called by `piece_divider` `prepared_pieces` (check and split) and `do_process_layout` (packer). Tiled paper: not called.
- Tests: `wide_piece_turns_only_when_the_turn_fits`, `any_mode_turns_a_wide_piece_that_fits_turned`; `piece_rotation_controls_split_and_stays_on_parts` updated.
- Checked: `cargo test --workspace` 672 pass; `ctest --preset debug` 8/8.
- Docs: PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, LAYOUT_ROTATION_PLAN, SVG-DATA-ATTRIBUTES `data-rotation`, `AGENT_CHECKLIST.md` §38.
- Open: Layout.802.5 installed-app check (trousers, roll 16 in, Any: `Union_piece` not listed).
- Found, not changed: dead code warnings `piece_extractor::piece_bbox`, `exports::build_tiled_pdf_tile_doc`.

### 2026-11-12 — Split part label on a short cut edge (Layout.801)

- Found by the user: trousers `Union_piece` split on a 16 in roll; 24 px label (~492 px Hershey) ran past the 227 px cut edge.
- First fix: shrink the label to the edge (font 24 → ~11 px, `textLength`). User decision: keep the 0.25 in text.
- New `cxxqt_bridge/src/part_label_placement.rs` `place_part_label`: label fits edge → along the edge, as before.
- Label wider than edge → level, 24 px, clear row nearest the cut edge. Clear: no outline edge, path or text within 0.25 em.
- No clear row → shrink along the edge (old fallback). `piece_divider.rs` `fit_label_to_edge` removed.
- Real trousers export: both labels 24 px, inside the seam line, below the existing labels, past the notches.
- Tests: `part_label_placement` (5), `piece_divider` `part_label_fits_its_part`. `cargo test --workspace` 677 pass; `ctest --preset debug` 8/8.
- Docs: SVG-DATA-ATTRIBUTES "split parts", SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` §37 (now Layout Mode Along Grainline: Any turns `Union_piece`).
- Open: Layout.801.4 installed-app check with the next MSI.

### 2026-11-12 — Export > SVG has no check boxes (Layout.800)

- `ExportMenu.qml`: the three SVG text mode items are plain items. A click exports.
- Removed: `lastSvgTextMode` (QML), `PreferencesModel` `svgTextMode`, `isSvgTextMode`, `saveSvgTextMode`, INI key `svg_text_mode`. An old INI keeps the key; SeamlyLayout ignores it.
- Kept: the headless `--svg-text-mode` option (`StartupOptions`), Rust `SvgTextMode`.
- Test: `ExportMenuTests::svgModeItemsHaveNoCheckBox` (3 rows). 3 `PreferencesModelTests` removed.
- Checked: `ctest --preset debug` 8/8. `cargo test` not run: no Rust change.
- Open: Layout.800.3 installed-app check with the next MSI (`AGENT_CHECKLIST.md` §36).

### 2026-11-12 — TODO_DXF.md cleaned

- Closed → `TODO_COMPLETED.md`: DXF.21, DXF.3, DXF.8 (completed); DXF.22 (obsolete). Findings table moved there too.
- Evidence: aw.fyi `male_shirt_R13_202611012029.dxf` (2026-11-01): pleat and slit ends not flagged; FrontPanel_M not flagged. jacket2 Base Size 500 reports (2026-10-23, 11-07, 11-08): 0 departures.
- DXF.3.5 CLO3D import check now lives only in Layout.102.5 (`AGENT_CHECKLIST.md` section 8).
- Open: DXF.18.5, rewritten with steps and expected result. `male_shirt` CollarTopInterface-Built-in-SA has no notch.
- Found, not investigated: the 2026-11-01 and 2026-11-07 male_shirt DXF files have 0 notches. `male_shirt.sm2d` FrontPanel and BackPanel have 1 notch each.
- Compliance doc "aw.fyi results": 4 new rows.

### 2026-11-12 — Plain words in the TODO file

- Rule added: describe literal actions and data; no made-up names, no wrong jargon. `CLAUDE.md` (d4809bc650). Agent file `.claude/agents/seamlylayout-developer.md` too: local only, gitignored.
- `TODO_SEAMLYLAYOUT.md` open items rewritten in plain words (bd756c7227). Each "Next MSI: user check" line now holds the steps and the expected result. Checked lines unchanged, except Layout.108 and the "harness" and "scratch" words.
- Fixed: the "Layout.58.13" cross-reference is Layout.68.13.
- Answer recorded in Layout.108: Seamly2D writes the SVG as text to SeamlyLayout's standard input. No file is written. SeamlyLayout reads the text and imports the pieces.
- `AGENT_CHECKLIST.md` OPEN list: Layout.88.2 "FAIL --> Note did not appear" (male_shirt).
- Next: Layout.108.7 docs, in plain words. `AGENT_CHECKLIST.md` section 26 and `SVG-DATA-ATTRIBUTES.md` "Multisize grainline and label positions" both use "stack".

## Decisions to retain

- Layout Mode Any: a piece too wide at its Rotation rule's angles turns 90° if that fits. This replaces the rule for that piece only; the packer cannot place it otherwise (Layout.802 task text, 2026-11-12).
- Custom page size lives in Tile Size > Custom (`tileWidth`, `tileHeight`). Paper Type has no custom size (user, 2026-11-12).
- Layout Mode Any ignores "Place fold lines on the left edge": no pin, no turn, no mirror, no conflict error (user, 2026-11-12).
- A paper sheet never grows, multisize tabs included. Overflow directs the user to PDF (tiled) (user, 2026-11-12).
- Keep `data-notch-type`. Only DXF export converts every notch to a layer 4 slit.
- Notch attributes: one value per notch (D1, 2026-10-12). Replaces "one value per segment" (2026-10-11).
- Built-in seam line corners are plain offset corners; node corner types shape only the cut line (user, 2026-10-12).
- Slit ends are never turn points. Slit depth uses half the local seam allowance width.
- Multisize tabs: Marker, Nested, Base Size N, then Size N. Every tab comes from the input SVG; no tab is built from grade rules. No "Multisize Layout" setting (user, 2026-10-13).
- DXF-ASTM carries one size: Base Size tab = base size + `# n` + `.rul`; size tab = one ungraded size; Marker and Nested have no DXF-ASTM export. No graded nests (user, 2026-10-13).
- CLO3D export uses R13 (`AC1012`). Piece text uses `Size Name:` and omits `Category:`.
- Default data root is `~/seamly2d`. Fresh installs allow a change; updates and repairs keep the recorded root read-only.
- Keep CLI export until SeamlyLayout CLI parity. Keep `exportLayoutAs()` through File → Export As.

## Authoritative documents

- [Application update tests](project-docs/TEST_APPLICATION_UPDATES.md)
- [SeamlyLayout TODO](project-docs/docs-seamlylayout/TODO_SEAMLYLAYOUT.md)
- [DXF TODO](project-docs/docs-seamlylayout/TODO_DXF.md)
- [SVG attribute contract](project-docs/docs-data/SVG-DATA-ATTRIBUTES.md)
- [Installer TODO](project-docs/docs-installer/TODO_INSTALLER.md)
- [Completed work archive](project-docs/TODO_COMPLETED.md)
