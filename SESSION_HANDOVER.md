# Session handover

## Current status

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

### 2026-11-12 — Version numbers move to a generated header

- `packaging/version.sh` writes only `src/libs/vmisc/project_version_number.h` (git-ignored). Builds change no tracked file.
- `projectversion.h/.cpp` read `PROJECT_VERSION_*` macros; both `Info.plist` use `${QMAKE_FULL_VERSION}`.
- `common.pri`: writes a 0.0.0 header when missing (Qt Creator, CI `linux-test`); sets `PROJECT_VERSION`. Apps set macOS `VERSION` from it.
- SeamlyLayout CMake parses the new header; 0.0.0 when missing.
- Local build scripts: stamp revert logic removed.
- Checked: qmake default + parse, version.sh (leading zeros), cl + rc (`FILEVERSION 26,11,1214,0`), qmake deps list the header.
- Open: macOS Info.plist substitution and Linux untested locally. Merge without skip-ci; run `ci.yml`.

### 2026-11-12 — .gitignore broad rules narrowed

- `*.json` → `AGENT_STATE.json`, `CMakeUserPresets.json`, `compile_commands.json` (+ signing key rule).
- `test*.*`, `Test*.*`, `docs/` → repository root only. `man/` deleted.
- Track: `packaging/macos/**/Info.plist`, `.claude/rules/`, `.claude/skills/`, `.claude/settings.json`, `src/libs/xerces-c/*/lib/*`.
- Ignore: `.cmake/`, `.qt/`, `scripts/**/*.txt`, `**/.claude/settings.local.json`.
- Check: no new file became visible; `scripts/2026-11-10-prompt.txt` is ignored now.

## Decisions to retain

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
