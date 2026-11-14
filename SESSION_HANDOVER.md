# Session handover

## Current status

### 2026-11-14 — Labels show `<empty>` as a blank line (Layout.X3)

- `LabelTextManager::isBlankLineMarker`: `<empty>`, or its editor (`EditLabelTemplateDialog`) or reader (`VDomDocument`) translation.
- `prepareLines`: a marker line becomes a `TextLine` with no text; it keeps its font size increment and one line height. A line that the placeholders leave empty is still dropped.
- Covers the Seamly2D scene labels and the handoff SVG (`add_label` writes no `<text>`, moves the next line down). Edit template Preview tab shows a blank row.
- `seamly_svg2ezdxf` converter still drops `<empty>` text: it comes only from SVG files of older Seamly2D builds.
- Test: `TST_LabelTextManager::blankLineMarker`.
- Checked: test program `TST_LabelTextManager` 12/12 pass (`build/scratch_label_empty/`, in-tree libs). `cargo test` not run: Rust comment only. Qt suites run with the next MSI.
- Docs: SVG-DATA-ATTRIBUTES `piece_label`; `AGENT_CHECKLIST.md` §43 and OPEN row; TODO Layout.X3.1.
- Open: Layout.X3.2 installed-app check with the next MSI (§43).

### 2026-11-14 — Layout tabs show the media size (Layout.X5)

- Each layout tab shows `<W> x <H> <unit>` below its label, in a smaller font: "36.00 x 52.25 in".
- Width: paper canvas width (a roll can widen); fabric: the bolt width, folded too (as "Media enlarged"). Height: the tab's layout length.
- Rust: `layout_utils::media_size_in_unit` (shared with `enlarged_media_message`), `layout_views::media_size_text`, `LayoutViews::media_sizes`.
- Bridge: QML property `layoutViewMediaSizes`; `run_layout` keeps the parsed `layout_settings`; `exit_adjust_mode` refreshes the tab properties.
- `Main.qml`: tab `contentItem` is a Column: label, then media size.
- Tests: `media_size_text_uses_the_settings_unit`, `media_sizes_follow_each_tab_layout`.
- Checked: `cargo test --workspace` 693 pass; `ctest` 8/8 pass (build dir `Seamly2D-private-worktrees/x5dbg`: the preset path in a worktree passes MAX_PATH).
- Docs: SIMPLE_WORKFLOW tab list; `AGENT_CHECKLIST.md` §42; TODO Layout.X5.1.
- Open: Layout.X5.2 installed-app check with the next MSI (§42).

### 2026-11-14 — Multisize piece names without the piece number (Layout.X6)

- `piece_naming.rs`: multisize `Size_<size>_<name>[_<letter>]`; no name → `Size_<size>_Piece_<n>[_<letter>]`. Piece-set: same without the size.
- User decision: names stay unique. A named piece whose name another piece also gets keeps `Piece_<n>` (`Size_34_Piece_2_Detail`).
- User decision (Layout.X6.5): individual `<name>[_<letter>]`; no name → `Piece_<n>[_<letter>]`. Same shared-name rule. Tests: `individual_name_drops_the_number`, `individual_shared_name_keeps_the_number`; cargo 692 pass, ctest 8/8. §41 trousers step.
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
