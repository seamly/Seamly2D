# Session handover

## Current status

### 2026-10-12 — checklist reviewed, fix plan approved, no code changed

- Branch `task-checklist-fixes` from `run-seamlyLayout` (`4db974dc60`). Merged `--no-ff` into `run-seamlyLayout` with skip-ci (docs and test data only). Not pushed: the user pushes.
- Commits on the branch:
  - Checklist results of the jacket2 test run; handover file renamed to `SESSION_HANDOVER.md`.
  - Test files moved: `test-seamly-layout-input/` → `test-seamly-layout/`. `.gitignore` tracks the new folder. aw.fyi PDFs force-added. `D6673-10-expired.docx` stays ignored (copyright).
  - Tracking: `TODO_SEAMLYLAYOUT.md` (decisions, root causes, Layout.27.9, 27.10, 29, 30), `TODO_DXF.md` DXF.22.1, `CLAUDE.md` handover name, this file.
- `develop` has 2 CI commits (`42fcec04ae`, `05969e1538`) not merged into `run-seamlyLayout`. They touch `.github/workflows/`: merge per `CLAUDE.md` "Merge from Upstream `develop`" before the next code task branch.

### Approved plan (user, 2026-10-12)

Root causes are in `TODO_SEAMLYLAYOUT.md` under each task. One commit per step. No task names or dates in code comments, briefs or commits.

| Step | App | Change | Task | Status |
|---|---|---|---|---|
| 1 | Seamly2D | Notch attributes: one value per notch + `data-notch-segments` | Layout.29.1–29.2 | open |
| 2 | Seamly2D | Multisize: a node that is a turn point in any size is one in every size | Layout.27.3 | open |
| 3 | Build | `copyToDestdir` always copies on Windows | Layout.30 | open |
| 4 | SeamlyLayout | Read `data-notch-segments` | Layout.29.3 | open |
| 5 | SeamlyLayout | DXF `Piece Name:` from `data-name`, `\U+XXXX` escapes, same for every size; duplicates numbered `Detail (3)`; dialogs keep the original name | Layout.26.3–26.5 | open |
| 6 | SeamlyLayout + QML | Missing Piece Data dialog: one line per piece, scroll area | Layout.26.1–26.2 | open |
| 7 | SeamlyLayout | Grading: snap key points to a vertex within 0.01 mm | Layout.27.9 | open |
| 8 | SeamlyLayout | Grading warnings | Layout.27.2, 27.4–27.6, 27.8 | open |
| 9 | SeamlyLayout | Marker: ungradable piece keeps every placed size, `Detail (3) 240` | Layout.27.10 | open |
| 10 | SeamlyLayout | Extend media height until every piece fits, every multisize tab | Layout.25 | open |
| 11 | All | C++ and Rust tests | each task | open |
| 12 | Docs | `SVG-DATA-ATTRIBUTES.md`, `NEW-ATTRIBUTES.csv`, compliance doc, new `TEST_CHECKLIST.md` | each task | open |
| 13 | Verify | `cargo test --workspace`, `ctest --preset debug`, `local_build_msi.ps1`; merge `--no-ff`; full CI (`common.pri` changed) | — | open |

- Not in this round: Layout.22, 23, 24, 28. Next round: Layout.22 → 23 → 24.
- Suggested order: 7 (small, Rust only) → 5 → 6 → 8 → 1 + 4 → 2 → 9 → 10 → 3.

### User decisions (2026-10-12)

- D1 notch format: one value per notch in `data-notch-type` and `data-seam-allowances`; new `data-notch-segments` = line count per notch.
- D2 non-ASCII DXF name: AutoCAD `\U+XXXX` escapes, not transliteration.
- D3 media height extension: every multisize tab (All sizes, Nested or Marker, and each size tab).
- D4 Marker, piece that cannot grade: write every placed size as its own piece, size in the name.
- D5 track `test-seamly-layout/` in git.
- D6 `CLAUDE.md`: handover name fixed; missing richmond-shirt reference removed.
- D7 FrontPanel (DXF.22): user reports to ApparelWerks; accept as reader advisory until he replies.
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

### Priority 1 — Developer: approved plan

- Merge `develop` (2 CI commits) into `run-seamlyLayout` first.
- New task branch from `run-seamlyLayout`; implement the plan table above.

### Priority 1 — User

- Report DXF.22 to rick@apparelwerks.com: same FrontPanel corner, L1 "should be curve", L14 "agree"; earlier export the reverse.
- Layout.28: keep in `TODO_SEAMLYLAYOUT.md`, or move to `TODO_SEAMLY2D.md`? Most work is in Seamly2D.

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

## Blocked items

- **Layout.16, Layout.51, Layout.63, Layout.64:** remain blocked. The latest entry gives no release condition.
- Layout.63 drill holes and Layout.64 stripe/plaid lines have no Seamly2D source.
- **Layout.27.7, Layout.38.2.2 fixtures:** wait for the user's captures.
- **Manual checks:** require the user's installed app, CLO3D, aw.fyi reports, or test laptop.

## Decisions to retain

- Keep `data-notch-type`. Only DXF export converts every notch to a layer 4 slit.
- Notch attributes: one value per notch (D1, 2026-10-12). Replaces "one value per segment" (2026-10-11).
- Slit ends are never turn points. Slit depth uses half the local seam allowance width.
- Remove the Settings "Multisize Layout" choice when both all-sizes tabs exist (Layout.23).
- Layout.61: graded nest + `# n` identifiers + `.rul` table. Grading only from the All sizes tab.
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
