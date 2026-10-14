# Session handover

## Current status

### 2026-10-14 — Layout.69, 67.9, 27.2, 27.8, 70, 27.11, 27.4, 27.5: merged into local `run-seamlyLayout`, not pushed

- Merged an earlier session's branch `task-raster-dpi` (Layout.69 PNG/JPG 96 dpi; TODO shortened, Layout.16 to `TODO_FUTURE.md`). The user's uncommitted TODO edit was a subset of it; backup in the session scratchpad.
- Finished that session's uncommitted work from worktree `Seamly2D-wt` (`task-dxf-export-report`): `DxfExportReport`, signal `export_companion_written`, "Export Complete" lists the `.rul`. Worktree and branch removed.
- Layout.70: `piece_extractor::unplaced_piece_lines`, `seamly_svg2ezdxf::tagged_piece_names` (DXF numbering), `layout_views::unplaced_warning`; `warningDialog` scrolls.
- Layout.27.11: `DUPLICATE_TOLERANCE_MM` 0.005 → 0.025, `SNAP_VERTEX_MM` follows it. jacket2 test found 6 near-duplicate vertices before.
- Layout.27.4/27.5: `grade_contours` returns `GradeError`; warning names the size and line, e.g. "size 660 has other turn points than size 500 on the cut line".
- Dead.1.4 checked (ci.yml `linux-test` runs both SeamlyLayout suites); Dead.1 moved to `TODO_COMPLETED.md`.
- Verified: `cargo test --workspace` 510 passed; `ctest --preset debug` 6 of 6. No MSI (ask first).
- Stale test binaries in `target/` from `Seamly2D-wt` embedded its path; `touch crates/svg_dom/src/lib.rs` forced a rebuild.
- Open: every "Next MSI" item; Layout.27.3.2 (Qt suites via `local_build_msi.ps1`); Layout.27.7 and 27.12.3 need a new handoff from the next MSI.

### 2026-10-13 — Layout.24 tab name before the export timestamp: merged into local `run-seamlyLayout`, not pushed

- Default name: `<base>[_<segment>][_<tab>]_<stamp>[_tiled].<ext>`; tab = shown tab of a multisize layout. Individual pattern: no tab.
- Code: `exports::tab_name_segment`, `exports::tab_export_path` (replaces `layout_views::tab_file_path`), `LayoutViews::active_label`, `AppController::defaultExportFileName(…, allTabs)`, `Main.qml` `makeExportFileName`.
- Export all tabs: save dialog offers the name without a tab; each file gets `_<tab>` before the stamp, or before the extension when the name has no stamp.
- Verified: `cargo test --workspace` 501 passed; `ctest --preset debug` 6 of 6; QML compiled in the debug build. No MSI.
- `TODO_SEAMLYLAYOUT.md`: only the Layout.24 lines committed; the user's other edits stay uncommitted.
- Open: Layout.24.7, `TEST_CHECKLIST.md` §7 "Export file names" on the next MSI.

### 2026-10-13 — Layout.27.12 line-end turn points: merged into local `run-seamlyLayout`, not pushed

- User decision: Option 1. Seamly2D applies the line-end test at nodes; `unifyTurnPoints` shares it across sizes; SeamlyLayout stops adding line ends to tagged lines.
- Seamly2D: `VAbstractPiece::turnPointIndices` marks a line end where the curve leaves the line at > 1° and > 2× the bend at its next vertex. Line = side between two node vertices, any length.
- SeamlyLayout: `build_contour_tagged` uses the tags alone; `straight_run_ends` (now `pub(crate)`) runs on untagged input only.
- Verified: `cargo test --workspace` 499 passed; `ctest --preset debug` 6 of 6; `cl /Zs` on `vabstractpiece.cpp` and both test files. Not run: Qt suites (no MSI; ask first).
- Old fixtures lack the new line-end tags. Tests changed to match them: male_shirt FrontPanel 16 → 14 turn points; jacket2 Base Size grades 6 pieces (Detail (3), (6) still warn). Replace both fixtures after the next MSI: Layout.27.12.3, Layout.27.7.
- Risk: an older tagged SVG file opened in SeamlyLayout no longer gets line-end turn points (e.g. FrontPanel front edge).
- Next: MSI build (Layout.27.3.2), user check (Layout.27.3.3: all 8 jacket2 pieces graded).

### 2026-10-13 — Layout.27.3 shared node turn points: merged into local `run-seamlyLayout`, not pushed

- Seamly2D: after the size loop, a piece node that is a turn point in one size is one in every size. `VLayoutPiece::unifyTurnPoints`, `MainWindowsNoGUI::unifySizeTurnPoints`, `VAbstractPiece::nodeVertexIndices` / `addTurnPoints`, `VLayoutPiecePath::setNodeVertices`.
- Verified: `cl /Zs` on every changed `.cpp` passed; expected test values checked with a Python port of `turnPointIndices`. Not run: Qt suites (no MSI build; ask first). No Rust change.
- Finding: the fix covers Detail (3) and (6). Detail (7) and (8) flip in SeamlyLayout's line-end rule (`straight_run_ends`), not in Seamly2D. Filed as Layout.27.12 with two options; user decision needed.
- Fixture `jacket2_40-146_202610121819.svg` is from the old producer. `jacket2_base_size_export_names_and_grades_each_piece` still expects Detail (3), (6), (7), (8) ungraded. Replace the fixture with a new handoff after the next MSI (Layout.27.7).
- `TODO_SEAMLYLAYOUT.md`: only the Layout.27.3 and 27.12 lines committed; the user's other edits stay uncommitted. `TEST_CHECKLIST.md` not touched: the user check is Layout.27.3.3.

### 2026-10-13 — Layout.72 oversized pieces (individual pattern): merged into local `run-seamlyLayout`, not pushed

- User decision: an individual pattern with a piece wider or longer than the media gets a dialog: "Split the pieces" or "Make the media bigger"; OK lays out, Cancel leaves only Layout Settings active.
- Accepted recommendations: no seam allowance on a cut edge; n equal parts; label + 2 match notches per cut edge; fabric is roll-form; the paper-sheet PDF splits the same way; a piece that fits but runs out of room is still left out.
- Code: `geometry/src/half_plane_clip.rs`, `cxxqt_bridge/src/piece_divider.rs`, `layout_utils` (`do_find_oversized_pieces`, `split_oversized`, `divide_spec`), `lib.rs` (`process_layout` check, `run_layout`, `process_layout_oversized`, `cancel_layout`, `is_settings_required`), `sheets.rs`, `Main.qml`, `TopMenuBar.qml`.
- Verified: `cargo test --workspace` 499 passed, 0 failed. `ctest --preset debug` 6 of 6; the QML compiles in the debug build. Split trousers rendered to PNG and checked by eye. Not verified: the dialog in the running app. No MSI.
- Open: Layout.72.8 user check (steps in `TODO_SEAMLYLAYOUT.md`; `TEST_CHECKLIST.md` is in `stash@{0}`).

### 2026-10-13 — Layout.71 multisize media widens: merged into local `run-seamlyLayout`, not pushed

- User decision: multisize tabs widen the media to the right for a piece wider than it, and lengthen it downwards (Layout.25). No multisize piece is left out on non-tiled media.
- Code: `layout_engine::required_bin_width`, `layout_assembler::set_content_width`, `layout_utils` step 7a. Tab warning text back to the pre-Layout.25 wording.
- Verified: `cargo test --workspace` 488 passed, 0 failed. `ctest --preset debug` 6 of 6. No MSI.
- Next: Layout.72 (individual pattern: dialog "Split the pieces" / "Make the media bigger"), approved, planned in `TODO_SEAMLYLAYOUT.md`.
- The user's uncommitted edits (TEST_CHECKLIST §7, TODO files, build stamps) are in `stash@{0}`, made before the develop merge `8af698b2c6`. Not touched.

### 2026-10-13 — Layout.25 media grows to fit: merged into local `run-seamlyLayout`, not pushed

- Multisize tabs set `ProcessLayoutArgs::extend_media`: sheet, roll and fabric grow in length until every piece that fits the width is placed. jacket2 Marker, 48 in roll: 208 of 208, about 798 in.
- Individual pattern: unchanged, a piece that does not fit is left out (task scope says multisize tabs). One-line change in `process_layout` if the user wants it everywhere.
- Code: `layout_engine::pack_maxrects_multi_angle_extending`, `packing::pack_polygons_extending`, `layout_utils` step 7 fits the canvas (`trim_bottom`). Sheet ends at the lowest piece, not above its own length. Tiled: no change.
- Paper-sheet PDF export still packs again on sheets of the set size (unchanged).
- Tab warning text: the media grows in length, not in width.
- Verified: `cargo test --workspace` 485 passed, 0 failed. `ctest --preset debug` 6 of 6 passed. No MSI built.
- `TEST_CHECKLIST.md` §7 "Media grows to fit" added, not committed: §7 is still the user's uncommitted content.
- Open: Layout.25.7, user check on the next MSI. Layout.27.1 (size 500 left out of Marker) needs a re-export after that.

### 2026-10-13 — Layout.68 PDF true size: merged into local `run-seamlyLayout`, not pushed

- Every PDF page (PDF, PDF (Tiled), sheets) goes through `exports::render_tree_to_pdf_bytes`: 96 px = 72 pt. `app_core::render_pdf` removed.
- Page side longer than 200 in: `/UserUnit` n, smallest whole number (user decision: a fabric layout can be longer than 500 in). 48 × 500 in → MediaBox `1152 × 12000`, `/UserUnit 3`.
- Verified: `cargo test --workspace` 479 passed, 0 failed. `ctest --preset debug` 6 of 6 passed. No MSI built.
- `app_core/Cargo.toml` still lists `svg2pdf`; nothing uses it. Removing it is a `Cargo.toml` change, so full CI.
- Test fixture `test_tiled_settings` says `"Portrait"`; `compute_tile_dims` matches only lowercase, so its pages are landscape. The app sends lowercase. No change.
- Open: Layout.68.5, user check on the next MSI.

### 2026-10-13 — jacket2 exports from MSI 26.10.1311 checked

- Files: `test-seamly-layout/layouts/jacket2/*_20261013*`. Marker: SVG ×3, PDF, PNG, JPG, EPS, PS, PLT plot and cut. Nested: SVG, PDF, JPG, EPS, PS. DXF-ASTM R13: Base Size 500 (+ `.rul`), Size 220.
- Pass:
  - `Piece Name:`: `\U+XXXX` escapes and `Detail (3)`–`(8)` in both DXF files. The §4 `Piece Name:` item is fixed.
  - Base Size grading: 4 pieces graded; rules match all 26 sizes within 0.011 mm (checked against the Nested SVG).
  - Size 220: no `.rul`, no `# n`, blank `Sample Size:`.
  - PLT, EPS, PS: geometry and scale correct. No overlaps; gaps ≥ 5 px.
  - Layout.27.6 and Layout.40.2 ticked.
- New tasks in `TODO_SEAMLYLAYOUT.md`:
  - Layout.68: PDF 133 % too large (`svg2pdf` default 72 dpi). Highest impact.
  - Layout.69: PNG and JPG without resolution metadata.
  - Layout.70: "Some pieces could not be placed" dialog: unnumbered names, no scroll.
  - Layout.27.11: vertices 0.01 mm apart on layer 87.
  - Layout.67.9: "Export Complete" does not name the `.rul`.
- Repeat failures (fix next, do not re-file): Layout.25 (Marker 96 of 208), Layout.27.3 (4 Detail pieces not graded).
- The three Marker SVGs (plain, Hershey, single-line) are identical: jacket2 has no text. Expected.
- `TODO_SEAMLYLAYOUT.md`: only these lines committed; the user's other edits stay uncommitted.

### 2026-10-13 — Layout.67 multisize tabs: merged into local `run-seamlyLayout`, not pushed

- User decision: tabs Marker, Nested, Base Size N, then Size N; no "Multisize Layout" setting. DXF-ASTM: Base Size = base size + `# n` + `.rul`; size tab = one ungraded size; Marker and Nested have none. No graded nests.
- Replaces Layout.22, 23. No longer needed: Layout.27.10, Layout.61.7.3. Still open: Layout.25, Layout.27.2–27.8.
- Verified: `cargo test --workspace` 476 passed, 0 failed. `ctest --preset debug` 6 of 6 passed. No MSI built.
- `TEST_CHECKLIST.md` §7 "Multisize tabs" added, not committed: the user's 26.10.1115 results are in the same file. It replaces the §4 `# 1` single size check.
- `TODO_SEAMLYLAYOUT.md`: only Layout.67 committed. The Layout.22 and 23 headings stay in the user's uncommitted edits; the user deletes them.
- Next: user builds the MSI and runs §7.

### 2026-10-12 — Layout.61.7.1: jacket2 aw.fyi reports checked

- Reports in `test-seamly-layout/layouts/jacket2/`: marker_allsizes 1149 (user named), nested_allsizes 1136, nested_size240 1142. All from MSI 26.10.1115.
- Result: grading not verified. Marker graded nothing. Nested graded 4 pieces; its departures come from the shared "Detail" name (Layout.26.5, fixed later).
- DXF.23: Detail (4) and Detail (6) bends at size 240 are not turn points. Done: both accepted as reader advisories, no code change (D8).
- Next: Layout.61.7.2, user re-exports Nested with MSI 26.10.1220 or later. Layout.61.7.3, Marker after Layout.25 and 27.10.
- `TODO_SEAMLYLAYOUT.md`: only the Layout.61.7 lines are committed. The user's other edits in that file stay uncommitted.

### 2026-10-12 — Layout.28 and the notch work merged into local `run-seamlyLayout` (`b3bf88c7cf`), not pushed

- User asked for Layout.28 outside the approved plan. Branched from `task-notch-per-notch`, not `run-seamlyLayout`: both change the notch and seam line handoff in `vlayoutpiece.cpp` and the DXF converter tests.
- User asked to merge both branches before the MSI build finished. One `--no-ff` merge of `task-builtin-seamline` carries every `task-notch-per-notch` commit. The merge commit was written without a checkout, so the running build kept its sources.
- Not pushed: the user pushes `run-seamlyLayout` to `origin` (user decision). Both task branches are deleted (user: delete a branch once it is merged).
- Seamly2D: `VPiece::builtInSeamLinePoints()`; `VLayoutPiece` stores it (`setBuiltInSeamLinePoints()`). Built-in piece: main path → `cutline` (always, also with seam allowances not shown), computed line → `seamline` (omitted when hidden).
- Seam line corners use `PieceNodeAngle::ByLength`: node corner types shape only the cut line (user confirmed).
- SeamlyLayout: no code change. Tests and comments only (`converter.rs`, `astm_notch.rs`).
- Verified: `cargo test --workspace` 472 passed, 0 failed.
- `local_build_msi.ps1`: passed on the third run, with `nmake check`; MSI 26.10.1220 built (`packaging/windows/seamly-msi/x64/seamly-x64.msi`). Run 1: `std::numeric_limits<qreal>::max()` clashes with the Windows `max` macro (use `infinity()`). Run 2: `BuiltInSeamLineFollowsCurve` found the start-loop bug; fixed on `task-builtin-seamline-loops`, merged, branch deleted.
- Open: Layout.28.7 user check. Seamly2D.11 (new, user decision): piece mode draws the built-in seam line.

### 2026-10-12 — notch work (was branch `task-notch-per-notch`), merged with Layout.28

- Commits (from `run-seamlyLayout` `be2576e9b9`):
  - `486261aaed` Layout.29: one notch value per notch, `data-notch-segments` (Seamly2D + Rust).
  - `4bec9b07c4` Layout.27.9: graded key points snap to a vertex within 0.01 mm.
  - `31714864c8` Layout.26: Unicode display names, `\U+XXXX` Piece Name, `Detail (3)` numbering, Marker `data-set-number`, dialog one line per piece + scroll.
  - `080278aa65` Layout.38.2.2 / 38.5: jacket2 fixture + 5 end-to-end tests.
- Verified: `cargo test --workspace` 470 passed, 0 failed (jacket2 tests included, about 25 s each).
- Verified with Layout.28: `local_build_msi.ps1` (`nmake check` covers the Layout.29 C++ tests). Not verified: `ctest --preset debug`, QML dialog in the app.
- Layout.29 complete: moved to `project-docs/TODO_COMPLETED.md`. A second run of the 26.10.1220 `Seamly2DTests.exe`: 0 failed. Open: user re-check of `TEST_CHECKLIST.md` §2 notch line.
- Next: remaining plan steps 2, 8, 9, 10. Merged into `run-seamlyLayout` early (user request); push when the user asks.
- Merge `3797bf8a6d` (Layout.40, `common.pri`) is on `origin/run-seamlyLayout`. The Layout.40 names merged without a conflict.
- Build stamps: `projectversion.*` and both `Info.plist` files now say 26.10.1220. Do not commit.
- `TODO_SEAMLY2D.md` has the user's own uncommitted edits (Seamly2D.50 drill holes, Seamly2D.60 stripe and plaid lines). Leave them to the user.
- `test_data/jacket2_40-146_nested_allsizes_R13_202610121813.dxf` untracked: no test uses it.

### Approved plan (user, 2026-10-12)

Root causes are in `TODO_SEAMLYLAYOUT.md` under each task. One commit per step. No task names or dates in code comments, briefs or commits.

| Step | App | Change | Task | Status |
|---|---|---|---|---|
| 1 | Seamly2D | Notch attributes: one value per notch + `data-notch-segments` | Layout.29.1–29.2 | done, Qt suites pass |
| 2 | Seamly2D | Multisize: a node that is a turn point in any size is one in every size | Layout.27.3 | open |
| 3 | Build | `copyToDestdir` always copies on Windows | Layout.40 | fix on run-seamlyLayout 3797bf8a6d; MSI check open |
| 4 | SeamlyLayout | Read `data-notch-segments` | Layout.29.3 | done |
| 5 | SeamlyLayout | DXF `Piece Name:` from `data-name`, `\U+XXXX` escapes, same for every size; duplicates numbered `Detail (3)`; dialogs keep the original name | Layout.26.3–26.5 | done |
| 6 | SeamlyLayout + QML | Missing Piece Data dialog: one line per piece, scroll area | Layout.26.1–26.2 | done, app check open |
| 7 | SeamlyLayout | Grading: snap key points to a vertex within 0.01 mm | Layout.27.9 | done |
| 8 | SeamlyLayout | Grading warnings | Layout.27.2, 27.4–27.6, 27.8 | open |
| 9 | SeamlyLayout | Marker: ungradable piece keeps every placed size, `Detail (3) 240` | Layout.27.10 | dropped: Marker has no DXF-ASTM export (Layout.67) |
| 10 | SeamlyLayout | Extend media height until every piece fits, every multisize tab | Layout.25 | open |
| 11 | All | C++ and Rust tests | each task | open |
| 12 | Docs | `SVG-DATA-ATTRIBUTES.md`, `NEW-ATTRIBUTES.csv`, compliance doc, new `TEST_CHECKLIST.md` | each task | open |
| 13 | Verify | `cargo test --workspace`, `ctest --preset debug`, `local_build_msi.ps1`; merge `--no-ff`; full CI (`common.pri` changed) | — | open |

- Not in this round: Layout.24. Layout.22 and 23 are replaced by Layout.67. Layout.28 done on `task-builtin-seamline` (user request).
- Suggested order: 7 (small, Rust only) → 5 → 6 → 8 → 1 + 4 → 2 → 9 → 10 → 3.

### User decisions (2026-10-12)

- D1 notch format: one value per notch in `data-notch-type` and `data-seam-allowances`; new `data-notch-segments` = line count per notch.
- D2 non-ASCII DXF name: AutoCAD `\U+XXXX` escapes, not transliteration.
- D3 media height extension: every multisize tab (All sizes, Nested or Marker, and each size tab).
- D4 Marker, piece that cannot grade: write every placed size as its own piece, size in the name.
- D5 track `test-seamly-layout/` in git.
- D6 `CLAUDE.md`: handover name fixed; missing richmond-shirt reference removed.
- D7 FrontPanel (DXF.22): user reports to ApparelWerks after every other open task is complete; accept as reader advisory until he replies.
- D8 jacket2 size 240 bends (DXF.23): Detail (4) and Detail (6) are reader advisories. Detail (4) goes in the DXF.22.1 report.
- Layout.28 stays in `TODO_SEAMLYLAYOUT.md`: it affects the layout program.
- On-fold quantity `N,0`: keep (checklist §6).
- Blank `Sample Size:` and `Grade Rule Table:` when nothing grades: accepted (checklist §4).

### Checklist findings, verified

- §2 notch words per segment → Layout.29.
- §4 `Piece Name:piece-1_s220_M`: Cyrillic name emptied by `sanitize_ascii` → Layout.26.4.
- §4 aw.fyi departures and "L5 endpoint mismatch 103.74": 6 pieces named "Detail" merged by aw.fyi. L5 per piece is correct → Layout.26.5.
- §4 4 "Detail" pieces ungraded: producer turn points flip by size → Layout.27.3.
- §4 duplicate vertices on layers 84/87 in graded files → Layout.27.9.
- §4 Marker: 96 of 208 placed; grading dropped 71 placed blocks → Layout.25, 27.10.
- §4 single size, no `# 1`: expected. `# 1` is in the All sizes file, size 500 blocks only.
- Installed jacket2 has Cyrillic names: stale local `bin/samples` → Layout.30.
- Checklist text errors to fix in the next checklist: no `Front` piece in jacket2; §3 "Open the pattern `seamly2d`"; §4 Marker aw.fyi step says "Nested 'Single Size'"; `test-sesamly-layout` ×3.

### Analysis scripts

Kept in the session scratchpad only (lost). To repeat a check: parse DXF group-code pairs; count layer 2 `POINT`s per block (turn points by size); compare consecutive `VERTEX` 10/20 values within 0.01 mm (duplicates); read layer 5 `LINE` 10/20/11/21 (grade reference).

## TODO by priority and owner

### Next items — Developer

- Layout.25 (media grows past the 500 in roll, user decision), then Layout.27.3 (repeat failures). Then Layout.70, 69, 27.11, 67.9.
- `TEST_CHECKLIST.md` line 104 names a `Front` piece; jacket2 has none. The file has user edits; ask before a change.

### Priority 1 — Developer: approved plan

- New task branch from `run-seamlyLayout`; implement the plan table above.

### Priority 2 — User: after the next MSI

- Capture the jacket2 handoff with `SEAMLYLAYOUT_SAVE_HANDOFF` (fixture for Layout.27.7).
- Capture the HJA handoff (`test-seamly-layout/patterns/HJA.sm2d`) for Layout.38.2.2.
- Run the new `TEST_CHECKLIST.md`. CLO3D checks last.

### Priority 2 — Developer: after the user captures

- Layout.27.7 and Layout.38.2.2 fixtures in `crates/cxxqt_bridge/test_data/`, end-to-end graded export tests.
- Plan **Layout.15** separately.

### Priority 3 — Developer: reconcile older unresolved items

These items have no recorded resolution. Check their TODO documents and repository state before resuming them.

- Check merge/build state for `task-piece-auto-name`, `task-grainline-name-ids`, `task-remove-export-layout-gui`, and `task-no-grainline-rotation`.
- Review outstanding **Installer.7** license notices. The pdftops item is already closed.
- Review **InstLinuxAppimage.2**: first-run data directory chooser.
- File a task for the SeamlyLayout return path: Save returns the layout SVG; other closes restore the previous canvas data.
- Correct `TEST_MSI_WIN_X64_Test_Case_1b-i.md`: split post-install and post-first-run checks in B.0a.
- Remove duplicate path leaves; replace `%DATAROOTROOT%` with `%DATAROOT%`.
- In B.0b-iii, replace the empty-file reference `qt6_seamly2d.ini` with `qt6_seamlyme.ini`.
- `CLAUDE.md` Key References names `project-docs/NEW-ATTRIBUTES.csv`; the file is `project-docs/docs-data/NEW-ATTRIBUTES.csv`.

### Priority 3 — User: finish installer checks

- Finish **Installer.6.8** on the test laptop: repair and uninstall remain open.
- Check fresh-install Change → Browse → OK and the first wizard page.
- Review section A test cases 2–4, which the source says have no walkthrough.

### Last — User: after every other open task

- Report DXF.22 to rick@apparelwerks.com: same FrontPanel corner, L1 "should be curve", L14 "agree"; earlier export the reverse.
  - Add jacket2 Detail (4) size 240, L1 pt 14: a 6.5° bend over 10 mm read as a missing corner (DXF.23).

## Blocked items

- **Layout.16, Layout.51, Layout.63, Layout.64:** remain blocked. The latest entry gives no release condition.
- Layout.63 drill holes and Layout.64 stripe/plaid lines have no Seamly2D source.
- **Layout.27.7, Layout.38.2.2 fixtures:** wait for the user's captures.
- **Manual checks:** require the user's installed app, CLO3D, aw.fyi reports, or test laptop.

## Decisions to retain

- Keep `data-notch-type`. Only DXF export converts every notch to a layer 4 slit.
- Notch attributes: one value per notch (D1, 2026-10-12). Replaces "one value per segment" (2026-10-11).
- Built-in seam line corners are plain offset corners; node corner types shape only the cut line (user, 2026-10-12).
- The user pushes `run-seamlyLayout` to `origin`. Delete a task branch once it is merged (user, 2026-10-12).
- Slit ends are never turn points. Slit depth uses half the local seam allowance width.
- Multisize tabs: Marker, Nested, Base Size N, then Size N. Every tab comes from the input SVG; no tab is built from grade rules. No "Multisize Layout" setting (user, 2026-10-13).
- DXF-ASTM carries one size: Base Size tab = base size + `# n` + `.rul`; size tab = one ungraded size; Marker and Nested have no DXF-ASTM export. No graded nests (user, 2026-10-13; replaces the Layout.61 graded nest).
- CLO3D export uses R13 (`AC1012`). Piece text uses `Size Name:` and omits `Category:`.
- Build only with `packaging/windows/local_build_msi.ps1`.
- Default data root is `~/seamly2d`. Fresh installs allow a change; updates and repairs keep the recorded root read-only.
- Keep CLI export until SeamlyLayout CLI parity. Keep `exportLayoutAs()` through File → Export As.
- On upstream merges, follow `CLAUDE.md`. Remove restored retired PS/EPS functions; no early upstream pdftops PR is approved.

## Verification

1. Run `cargo test --workspace` from the SeamlyLayout Rust workspace.
2. Use VS 18 `vcvars64`, then configure PATH and QMAKE.
3. From `qt_frontend/`, run `cmake --build --preset debug`, then `ctest --preset debug`.
4. Run `packaging/windows/local_build_msi.ps1` with tests and validation enabled before each merge.
5. Run the manual checklist against the resulting MSI.

- Put MSVC `link.exe` before Git's GNU `link` on PATH.
- Use a `.cmd` file; prepend `%VCToolsInstallDir%bin\Hostx64\x64` after `vcvars64` runs.
- For qmake suite logs, set `SEAMLY_TEST_LOG_DIR` and use each suite's `target_wrapper.bat`.
- For a CMake Qt suite, use `-o <file>,txt`. Console silence does not prove a pass.
- Release SeamlyLayout builds write no Rust log lines (`log_to_file` is a no-op).
- For installer checks, capture `local_install_msi.ps1 -Phase Baseline` before installation.
- Run the MSI wizard from an elevated shell. Do not use `/quiet` for manual checks.

## Protect uncommitted files

- Inspect `git status` and `git stash list` before changing branches or committing.
- Do not commit or discard user edits without authorization.
- Protect both macOS `Info.plist` files and `src/libs/vmisc/projectversion.*`; builds rewrite them (stamped 26.10.1115).
- Protect user edits in `CLAUDE.md`, `TODO_DXF.md`, `scripts/prompt_testing.txt`, and `test-seamly-layout/`.
- Never commit `test-seamly-layout/D6673-10-expired.docx`.
- Inspect older stashes by content. `stash@{0}` and `stash@{1}` hold a layout workflow document edit.

## Authoritative documents

### SeamlyLayout

- [SeamlyLayout TODO](project-docs/docs-seamlylayout/TODO_SEAMLYLAYOUT.md)
- [DXF TODO](project-docs/docs-seamlylayout/TODO_DXF.md)
- [Test checklist](project-docs/TEST_CHECKLIST.md)
- [DXF compliance](project-docs/docs-seamlylayout/dxf-docs/seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md)
- [SVG attribute contract](project-docs/docs-data/SVG-DATA-ATTRIBUTES.md)
- [Retired layout code TODO](project-docs/docs-seamlylayout/TODO_REMOVE_DEAD_LAYOUT_CODE.md)

### SeamlyMe

- [SeamlyMe TODO](project-docs/docs-seamlyme/TODO_SEAMLYME.md)

### Installer

- [Installer TODO](project-docs/docs-installer/TODO_INSTALLER.md)
- [MSI test case TODO](project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md)
- [MSI test case](project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md)
- [Windows packaging instructions](packaging/windows/README.md)
- [macOS installer](project-docs/docs-installer/TODO_INSTALLER_MACOSX.md)
- [Linux installer](project-docs/docs-installer/TODO_INSTALLER_LINUX_APPIMAGE.md)

### CLI

- [CLI TODO](project-docs/docs-cli/TODO_CLI.md)

### CICD

- [Code Signing](project-docs/docs-cicd/TODO_CODE_SIGNING.md)

### Completed Tasks

- [Completed work archive](project-docs/TODO_COMPLETED.md)
