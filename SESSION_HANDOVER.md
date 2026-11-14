# Session handover

## Current status

### 2026-11-14 — Multisize piece names without the piece number (Layout.X6)

- `piece_naming.rs`: multisize `Size_<size>_<name>[_<letter>]`; no name → `Size_<size>_Piece_<n>[_<letter>]`. Piece-set: same without the size.
- User decision: names stay unique. A named piece whose name another piece also gets keeps `Piece_<n>` (`Size_34_Piece_2_Detail`).
- User decision (Layout.X6.5): individual `<name>[_<letter>]`; no name → `Piece_<n>[_<letter>]`. Same shared-name rule. Tests: `individual_name_drops_the_number`, `individual_shared_name_keeps_the_number`. §41 trousers step.
- Naming now reads every piece first (`collect_pieces`, `planned_names`), then writes.
- `unplaced_piece_lines`: an assigned name is the whole line; no `: <size>`. One line per left-out size.
- Tests: `multisize_name_drops_the_number`, `unplaced_piece_lines_use_assigned_names`; jacket2 export test now expects `Size_500_<name>_<letter>`.
- Checked: `cargo test --workspace` 691 pass; `ctest --preset debug` equivalent 8/8 (worktree build dir `x6dbg`; preset path passed MAX_PATH).
- Docs: SVG-DATA-ATTRIBUTES "SeamlyLayout piece names"; `AGENT_CHECKLIST.md` §5, §41; TODO Layout.X6.1–.3.
- Open: Layout.X6.4 installed-app check with the next MSI (§41).

### 2026-11-14 — Piece Properties: Layout rotation (Layout.X8)

- User decision: Seamly2D does not know the SeamlyLayout Layout Mode, so Rotation stays visible. Rename and add a line instead of hiding.
- `pattern_piece_dialog.ui`: label "Rotation:" → "Layout rotation:". New `rotationLayoutMode_Label` below the radio buttons: "Rotation applies when Layout Mode=Any in SeamlyLayout."
- SeamlyLayout `SettingsDialog.qml` Piece Gap tip and docs (SVG-DATA-ATTRIBUTES, NEW-ATTRIBUTES, SETTINGS_WORKFLOW, LAYOUT_ROTATION_PLAN) say "Layout rotation".
- Checked: `uic` builds the `.ui`; `ctest --preset debug` 8/8. `cargo test` not run: no Rust change. Seamly2D Qt suites not run (next MSI).
- Open: Layout.X8.2 installed-app check, `AGENT_CHECKLIST.md` §40.
- `.ts` translation files not updated: lupdate picks up the new strings.

### 2026-11-12 — Nested sizes stack on a corner (Layout.804)

- Merged into `run-seamlyLayout` without skip-ci (`.pri`/`.pro` changed). Branch `task-nested-stack-corner` deleted.
- Stack point: Rust `svg_dom/src/stack_point.rs`; C++ `layout_handoff/handoff_stack_point.cpp` mirrors it. Users: `center_piece_sets`, `annotation_alignment.rs`, `align_handoff_annotations`.
- Rules: fold line in every size → its lower end (left end if level). Else turn points in every size → largest size's turn point with the longest level edge, else longest edge. Else bbox centre.
- User decisions: fold line first, lower end; both apps; tie → longer other edge, then lower, then left.
- New C++ `handoff_svg_reading.h/.cpp` (`find_tagged_child`, `read_matrix`, `collect_points`).
- Rust `annotation_alignment::inside`: within 0.01 px of the cut line counts as inside (matches C++). Test `anchor_on_the_cut_line_is_inside`.
- `test_data/jacket2_40-146_handoff.svg`: 227 annotation transforms realigned by the new C++ code (scratch `realign.exe`). On the old fixture it gives 0 warnings: Back's size 220 import warning is gone. A fresh jacket2 export is still needed.
- Checked: scratch QtTest `TestHandoffAnnotationAlignment` 18/18; `cargo test --workspace` 689 pass; `ctest --preset debug` 8/8. MSI Qt suites not run.
- Docs: SVG-DATA-ATTRIBUTES "Multisize stack point"; SETTINGS_WORKFLOW fold line; `AGENT_CHECKLIST.md` §26 (no import warning), §39, OPEN row; TODO Layout.804.1–.4.
- Open: Layout.804.5 with the next MSI: 4 Qt suites, `AGENT_CHECKLIST.md` §39.
- Scratch: `build/scratch_corners/` (`fix_*.py`, `qttest/` QtTest build, `realign.exe`, `ctest_debug.bat`).

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
