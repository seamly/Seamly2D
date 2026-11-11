# Session handover

## Current status

### 2026-11-11 — Sheet overflow leaves no layout

- Cause: run_layout showed the error but kept the old layout and Create Layout; another material's overflow became a warning with a partial layout.
- `layout_utils::is_sheet_overflow` recognizes `SHEET_OVERFLOW_MESSAGE` and `SHEET_ANNOTATION_OVERFLOW_MESSAGE`.
- `refuse_sheet_layout`: blank canvases, tabs and split cleared, `cancel_layout`, error names PDF (tiled). Any material's overflow ends the run.
- `cargo test --workspace`: pass. Installed-app check runs with the next MSI.

### 2026-11-11 — Custom sheet in Paper Type

- Cause: 'Custom' sat last in the 27-size Paper Size list; users could not find it.
- Paper Type lists sheet, custom, tiled, roll. Custom shows Paper Size as width × height fields in the Layout Settings unit.
- Data format unchanged: paperType 'sheet', sheetName 'Custom'. Rust unchanged.
- `SettingsModel::selectPaperType`, `paperTypeChoice`, `paperTypeNames`; Paper Size lists hold predefined sizes only.
- SettingsModel tests (temporary test program): 42 pass. `cargo test --workspace`: pass. Installed-app check runs with the next MSI.

### 2026-11-11 — Paper sheets cannot grow

- Oversized-pieces dialog disables 'Make the media bigger' on a paper sheet; its text names PDF (tiled).
- Rule: `layout_utils::media_can_grow`. Bridge property `media_can_grow` is set before the dialog opens.
- `process_layout_oversized` refuses "extend" on a sheet: error names PDF (tiled), no layout.
- Multisize sheets still extend (no dialog); out of scope.
- `cargo test --workspace`: pass. qmllint: no syntax errors. Installed-app check runs with the next MSI.

### 2026-11-11 — Split layouts and DXF

- Cause: a single layout (no tabs, one material) never refreshed `is_split_layout_active`. DXF stayed enabled after a split; the stale flag disabled it after the next whole layout.
- `run_layout` drops the old `divide_spec` at the start and refreshes the flag at the end.
- `export_dxf` refuses split layouts through `layout_views::dxf_export_refusal`.
- `cargo test --workspace`: pass. Installed-app check (paper sheet and roll, split) runs with the next MSI.

### 2026-11-11 — Fold lines across the grain

- Cause: trousers `Test-halfleg` fold line runs across the grain; the fold plan pinned vertical fold lines only.
- `fold_line_edge::plan_fold_line_edge` turns such a piece until its fold line is vertical. Fold edge outranks layout mode; Any mode checks an explicit piece rotation.
- `piece_divider::divide_oversized_pieces` turns in place before it measures.
- Risk: on fabric, a turned piece's grain runs across the fabric.
- `cargo test --workspace`: pass. Installed-app check with `trousers.sm2d` runs with the next MSI.

## Decisions to retain

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
