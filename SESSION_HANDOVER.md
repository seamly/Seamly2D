# Session handover

## Current status

### 2026-11-12 — Tile Size offers Custom

- The removed custom size was a Paper Type choice. Tile Size had no Custom.
- Tile Size lists the 27 sizes, then Custom. Custom Size fields: width × height in the user unit.
- New keys `tileWidth`, `tileHeight` (default 8.5 × 11), not `pageWidth`/`pageHeight`: page width is the fabric width fallback.
- Selecting Custom copies the current predefined size. Orientation applies. Unit change converts.
- Rust: `CUSTOM_TILE_SIZE`, `LayoutSettings::custom_tile_px`, `tiling::tile_page_px`.
- Docs: SETTINGS_WORKFLOW, settings rule, TODO Layout.999.1 and .4, `AGENT_CHECKLIST.md` §10.
- Checked: `cargo test --workspace` 669 pass; `ctest --preset debug` 8/8.
- Open: Layout.999.4 installed-app check with the next MSI.

### 2026-11-12 — Paper > Sheet removed (Layout.999)

- Paper Type is tiled or roll. The custom size moved to Tile Size. Saved "sheet" or "custom" loads as "tiled".
- Deleted `sheets.rs`, `oversized.rs`, `remaining.rs`, `media_can_grow`, `refuse_sheet_layout`, sheet PDF export.
- Piece divider splits columns only. "Make the media bigger" always enabled.
- New `LayoutSettings::is_tiled_paper`: fabric with default paper type "tiled" was treated as tiled paper; fixed.
- Layout.300 obsolete → `TODO_COMPLETED.md`. Rotation.4.2, 4.3 obsolete.
- Docs: rules, SVG-DATA-ATTRIBUTES, workflow docs, `AGENT_CHECKLIST.md` §10, §12, `TEST_CHECKLIST.md`.
- Checked: `cargo test --workspace` 665 pass; `ctest --preset debug` 8/8.
- Open: Layout.999.4, Layout.72.8 installed-app check with the next MSI.

### 2026-11-12 — Layout Mode Any ignores the fold edge (Rotation.0)

- `LayoutSettings::places_fold_lines_on_left_edge()`: false in Any. `do_process_layout`, `layout_divide_spec`, sheet PDF (`lib.rs`) and `margin_left_value` read it.
- Removed: `Rect::fold_edge_conflict`, `FoldEdgePlan::conflicts`, the `apply_piece_rotation_rules` argument, and the "cannot satisfy both its rotation rule and the fold edge" errors.
- `layout_engine::piece_trial_set`: a left-edge piece always has its one planned angle.
- Sheet overflow tests now match the 'Paper > Tiled' message text.
- Checked: `cargo test --workspace` 704 pass; `ctest --preset debug` 8/8.
- Open: Rotation.0.4 installed-app check with the next MSI.

### 2026-11-12 — Paper sheets never grow (Layout.300)

- `do_process_layout`: `extend_media` is off when `media_can_grow` is false. Multisize tabs on a sheet now fail with `SHEET_OVERFLOW_MESSAGE` (PDF (tiled)).
- Dead code removed: the "extended sheet keeps its own length" trim branch.
- Tests deleted: `jacket2_marker_extends_paper_sheet`, `trousers_long_pieces_extend_a_sheet`.
- `placement_enlargement_dimensions_and_notifications` uses a paper roll; the sheet overflow test also runs with `extend_media` on.
- Docs: PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW_DESCRIPTION, `TEST_CHECKLIST.md`, `AGENT_CHECKLIST.md` §10, §12.
- Checked: `cargo test --workspace` pass; `ctest --preset debug` 8/8.
- Open: Layout.300 user check with the next MSI.

### 2026-11-12 — Marking test pinned to a solid seam line

- MSI build stopped: `TST_HandoffDocument::markingsKeepTheirGeometryAndRejectInvalidData` failed.
- Cause: the test read the user's settings; `defaultSeamLinetype=dashLine` gave the stripe a `stroke-dasharray`.
- Writer unchanged: stripes keep the configured seam style (SVG-DATA-ATTRIBUTES.md).
- Fix: the test sets `style.seam.type = Qt::SolidLine`.
- Checked: temporary test program, all 39 Seamly2DTests suites pass. Other Qt suites run with the next MSI.

## Decisions to retain

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
