# Session handover

## Current status

### 2026-10-13 — Layout.67 multisize tabs: merged into local `run-seamlyLayout`, not pushed

- User decision: tabs Marker, Nested, Base Size N, then Size N; no "Multisize Layout" setting. DXF-ASTM: Base Size = base size + `# n` + `.rul`; size tab = one ungraded size; Marker and Nested have none. No graded nests.
- Replaces Layout.22, 23. No longer needed: Layout.27.10, Layout.61.7.3. Still open: Layout.25, Layout.27.2–27.8.
- Verified: `cargo test --workspace` 476 passed, 0 failed. `ctest --preset debug` 6 of 6 passed. No MSI built.
- `TEST_CHECKLIST.md` §7 "Multisize tabs" added, not committed: the user's 26.10.1115 results are in the same file. It replaces the §4 `# 1` single size check.
- `TODO_SEAMLYLAYOUT.md`: only Layout.67 committed. The Layout.22 and 23 headings stay in the user's uncommitted edits; the user deletes them.
- Next: user builds the MSI and runs §7.

### 2026-10-12 — Next item: `TEST_CHECKLIST.md` §4 `Piece Name:` check (lines 104–112)

- Finding: Nested All sizes DXF writes `Piece Name:piece-1_s220_M`, `piece-9_s240_M`, … No `Front`.
- Cause of `piece-N_sNNN_M`: `sanitize_ascii` empties Комір and Подборт; the writer falls back to the block id (Layout.26.4).
- Cause of unnumbered `Detail`: Layout.26.5.
- `Front`: checklist text error. jacket2 has no Front piece.
- Both exports predate the fix (Layout.26 commit `31714864c8`, 19:01):
  - `..._202610121136.dxf`: MSI 26.10.1115.
  - `..._202610121813.dxf`: `Author:Seamly;Seamly2D;26.10.1214`. 52 `piece-N` (2 pieces × 26 sizes), 56 unnumbered `Detail`.
- Fix in code: `\U+XXXX` escapes, `Detail (n)`. Test `jacket2_all_sizes_export_names_every_size_alike` (`cxxqt_bridge/src/layout_utils.rs`) passed in the 472-test run.
- Not verified in the installed app. MSI 26.10.1220 (`packaging/windows/seamly-msi/x64/seamly-x64.msi`) has the fix. No export from it yet.
- Steps:
  1. User: install MSI 26.10.1220. Export jacket2 Nested All sizes, DXF-ASTM R13 (same as Layout.61.7.2).
  2. Check `Author:Seamly;Seamly2D;` says 26.10.1220 or later.
  3. Expect `Piece Name:`: `\U+041A\U+043E\U+043C\U+0456\U+0440` ×26, `\U+041F\U+043E\U+0434\U+0431\U+043E\U+0440\U+0442` ×26, `Detail (4)` ×26, `Detail (5)` ×26, `Detail (3)`, `(6)`, `(7)`, `(8)` ×1 each. No `piece-`.
  4. Any `piece-` name left: fix it in the next iteration; do not re-file. Start by diffing the installed handoff with `cxxqt_bridge/test_data/jacket2_40-146_202610121819.svg`.
  5. Checklist line 104: replace `Front` with a jacket2 piece name. `TEST_CHECKLIST.md` has user edits; ask first.

### 2026-10-12 — Layout.61.7.1: jacket2 aw.fyi reports checked

- Reports in `test-seamly-layout/layouts/jacket2/`: marker_allsizes 1149 (user named), nested_allsizes 1136, nested_size240 1142. All from MSI 26.10.1115.
- Result: grading not verified. Marker graded nothing. Nested graded 4 pieces; its departures come from the shared "Detail" name (Layout.26.5, fixed later).
- New: DXF.23. Detail (4) and Detail (6) bends at size 240 are not turn points. Open: user decisions DXF.23.1 and 23.2.
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

### Next item — User, then Developer

- `TEST_CHECKLIST.md` §4 `Piece Name:` (lines 104–112). Steps in Current status, first entry.

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
