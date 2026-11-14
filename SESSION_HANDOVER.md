# Session handover

## Current status

### 2026-11-14 — Logo commit broke the MSI build

- Cause: commit `3674fcfeab` moved `seamly-layout_512x512.png` and `seamly-layout.icns` to `assets/images/old-imgs/`; `qt_frontend/CMakeLists.txt` still names `assets/images/`.
- `CMakeLists.txt` `SEAMLY_ICON_PNG` now names `seamly-layout-512x512.png` (Sue made the PNG set). `seamly-layout.icns` rendered from the new `seamly-layout.svg` (Inkscape 1.4.4, Pillow).
- `svg_dom` test `deep_id_lookup_in_nested_groups` uses inline SVG; the old test read id `Circle` from the logo.
- Checked: `cargo test --workspace` 704 pass; `ctest --preset debug` 8/8.
- Open (Sue): rerun `local_build_msi.ps1`; keep or delete `assets/images/old-imgs/`.

### 2026-11-14 — Split part label, tiled PDF, multisize split on fabric (Layout.X2, X4, X7)

- Three task branches built and tested in parallel worktrees; merged `--no-ff` into `run-seamlyLayout`; branches and worktrees deleted.
- X2: part label `<name> part i of n`; join list code removed. New `data-source-name` keeps the imported `data-name`; the label uses it, else the shown name.
- X4: `ExportMenu.qml` `pdfEnabled`: Export > PDF disabled on tiled paper; PDF (Tiled) enabled; View menu PDF unchanged.
- X7: multisize fabric runs the oversized-pieces dialog; "Split the pieces" splits on every layout of the run; a Nested piece-set too wide is cut on the same lines per size into piece-sets `<set id>_part<k>`.
- Checked per branch and on the merged tree: `cargo test --workspace` 704 pass, 0 failed; `ctest` debug 8/8.
- Docs: SVG-DATA-ATTRIBUTES, PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW; `AGENT_CHECKLIST.md` §45–§47 and OPEN rows; §41 now uses jacket2 and "Make the media bigger".
- Decision (Sue): keep `data-source-name` for the part label.
- Open: X2.2, X4.2, X7.2 installed-app checks; X7.3 DXF refused on every tab of a split material; X7.4 dialog misses a too-wide Nested set with no too-wide size piece.

### 2026-11-14 — Split part labels move inside the cut line (Layout.X1)

- New `label_inside_part.rs`: after a split, a `pattern_label`, `piece_label` or `fold_line_label` not wholly inside its part outline moves inside it.
- Search: grid of whole steps from the old centre, nearest first. Order: full size, 1/16 in clear of other labels and the part label place; full size anywhere inside; shrink about the centre in 10% steps.
- No place at 10%: the label stays. The move and scale go in through `svg_dom::flatten_dom`.
- `piece_divider::divide_piece` calls it before `place_part_label`; reserved box: `part_label_placement::along_edge_bounds`.
- `part_label_placement.rs`: `drawn_bounds`, `along_edge_bounds`, `Bounds::grown`/`overlaps`, `inside`, `x_range_in_band` shared.
- Checked: `cargo test --workspace` 701 pass; `ctest` debug 8/8. Fixtures richmond (8 in roll) and jacket2 (12 in roll): crossing labels moved inside; 155 calls, 61 ms debug.
- Decision for Sue: shrink fallback for a label larger than its part (10% steps). Alternative: leave it at full size.
- Docs: PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, SVG-DATA-ATTRIBUTES split parts and `fold_line`, `AGENT_CHECKLIST.md` §44.
- Open: Layout.X1.2 installed-app check (male_shirt, roll 8 in, split).

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
