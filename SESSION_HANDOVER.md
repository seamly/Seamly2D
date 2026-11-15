# Session handover

## Current status

### 2026-11-14 — Material tab order (Layout.Y1)

- `material_split.rs`: `pattern_materials` sorts by `MATERIAL_ORDER`: Fabric, Lining, Interfacing, Interlining. Other names go after, in handoff order. Match ignores ASCII case.
- Cause: Seamly2D writes the pattern groups in the order of the first piece of each material, so Lining could come first.
- Both tab bars (`materialLabels`) and every material index (`material_docs`, `material_layouts`) follow the new order.
- Test: `materials_follow_the_tab_order`.
- Checked: `cargo test --workspace` 708 pass; `ctest` debug 8/8. Built in worktree `material-tab-order`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES Materials.
- Open: Layout.Y1.1 installed-app check with the next MSI.

### 2026-11-14 — Split pieces keep their markings (Layout.803)

- `piece_divider.rs`: `stripe` and `plaid` reference lines are open lines that end at the cut lines, like `internal_path`.
- Drill holes: `all_points` adds circle centres. A `drill` group goes whole to the part nearest its centre (before: always part 1). `cut_positions` keeps cut lines off drill centres.
- Internal paths: no code change; already split. A closed one becomes one open line per part.
- Tests: `markings_split_with_the_piece`, `cut_line_misses_drill_centres`.
- Built and tested in worktree `split-markings`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES split parts, PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` §48 and OPEN row.
- Open: Layout.803.1 installed-app check with the next MSI (§48).

### 2026-11-14 — Media size for individual patterns (Layout.X5.3)

- Individual pattern: the media size shows below each material tab name over the layout; one material: below "Material: <name>".
- Untagged import (no material name): the header shows only the size.
- Rust: `layout_views::material_media_size`, `MaterialLayout::media_size`. Empty for a blank canvas or a material with layout tabs.
- Bridge: QML property `materialMediaSizes`, one entry per material; `refresh_layout_view_properties` fills it from `is_layout_ready` (shown) and `material_layouts` (others).
- Order fixes so no old size stays: `initialize_layout` and `run_layout` clear the tabs after the layout flag drops; `finish_tab_layouts` refreshes after it rises.
- `Main.qml`: `MaterialTabBar.mediaSizes` (left bar leaves it empty); `materialHeader` is a Column.
- Test: `material_media_size_shows_one_finished_layout`.
- Checked: `cargo test --workspace` 705 pass; `ctest --preset debug` 8/8. App starts with the new QML; UI not driven.
- Docs: SIMPLE_WORKFLOW; `AGENT_CHECKLIST.md` §42 (multisize part now jacket2; trousers is individual) and OPEN row; TODO Layout.X5.3.
- Open: Layout.X5.2 installed-app check with the next MSI (§42).

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
