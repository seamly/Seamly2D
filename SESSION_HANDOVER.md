# Session handover

## Current status

### 2026-10-25 — DXF units and McNeel AAMA items approved, not started

- Next session: implement Task Layout.103 (DXF units), then Layout.104 (DXF-AAMA piece text), then Layout.100 (remove DXF-ASTM R12). Full specs and decisions are in `TODO_SEAMLYLAYOUT.md`.
- Layout.103: pattern unit decides the units. mm → `METRIC`, sizes unchanged; cm → `METRIC`, sizes ×10; inch → `ENGLISH`, sizes unchanged, every length in inches with 4 decimals. Needs new handoff attribute `data-units` (Seamly2D `SvgGenerator`), so it touches Seamly2D C++ and the contract.
- Layout.104: `Annotation:` from `data-annotation`; block name `<piece name>_<size>`; `Creation Time:` `HH:MM:SS`. DXF-AAMA only.
- Layout.100: keep the AC1009 writer, which DXF-AAMA (R11) uses.
- Layout.102 (CLO3D layers) and Layout.101 (`SEAMLY_DATA_DIR`) are filed, not approved for work.
- Committed today: `seamlylayout_DXF_AAMA.md` "McNeel guide" (piece text and layers 0–19 compared with the output).
- Open question for the user: untracked `project-docs/docs-seamlylayout/AAMA-INTEROP.md` copies the upstream PR text about the old `vdxf` output; rewrite or leave.
- Work method: peer sessions share the main tree. Use a worktree from `run-seamlyLayout`, merge with `merge-tree` + `commit-tree` + `update-ref`, then sync the main tree paths and tell the peers.
- Uncommitted user files left untouched: `AGENTS.md`, `AAMA-INTEROP.md`, `test-seamly-layout/foldline_arrowline.*`, `test-seamly-layout/layouts/*`. `TODO_SEAMLYLAYOUT.md` Layout.100 is now committed (user asked for the task).

### 2026-10-25 — Dead.4.3: `AbstractPiece`, `SeamAllowancePoint`, `LabelTextManager` in `vpatterndb`; merged into local `run-seamlyLayout`, not pushed

- Moved from vlayout and renamed: `VAbstractPiece` → `AbstractPiece` (+ `AbstractPieceData.h`), `VSAPoint` → `SeamAllowancePoint` (own header), `VTextManager` → `LabelTextManager`. vpatterndb includes no vlayout header now.
- User decisions: keep the method names; rename `VSAPoint`; one exit point per function; no namespace.
- Restyle: briefs, snake_case, one exit; `LabelTextManager::min_font_size` replaces `MIN_FONT_SIZE` (`MAX_FONT_SIZE` was unused, gone); `TextLine` members `text`, `font_size_increment`, `bold`, `italic`, `alignment`.
- Build files: vpatterndb `QT += xml`; vlayout links before vpatterndb in 4 `.pro` files. Translation context `VAbstractPieceData` → `AbstractPieceData` in the `.ts` files.
- Tests: `TST_AbstractPiece` (new `SeamAllowancePointWidths`), `TST_LabelTextManager`.
- Verified: `cl /Zs` on every changed `.cpp` and their includers. Scratch harness (session scratchpad `223c6375-…/harness`): `TST_AbstractPiece` 58 passed, `TST_LabelTextManager` 7 passed; old/new differential, 30 000 random paths and 200 000 random corners, label fits: no output difference. Not run: Qt suites, MSI (ask first), Linux link (CI), GUI.
- User check: `AGENT_CHECKLIST.md` §30.
- Next: Dead.4.4 (`layout_handoff`).
- Work done in worktree `C:\Users\susan\Projects\s2d-abstract-piece` (removed after the merge).

### 2026-10-25 — DXF-AAMA piece text review (upstream PR 1783); merged into local `run-seamlyLayout`, not pushed

- Upstream PR 1783 (open) makes the `vdxf` AAMA piece text independent of the label. SeamlyLayout DXF-AAMA already did this; no exporter code change.
- User decisions: AAMA `Quantity:` stays `R,L` (not a total); `Size:` stays in pattern units (not cm); `Units:METRIC` stays.
- New test `aama_piece_text_comes_from_piece_data_not_label` (`writer_astm_test.rs`): handoff SVG to DXF-AAMA, label hidden and label contradicting the piece data.
- Docs: `seamlylayout_DXF_AAMA.md` Decisions; `SVG-DATA-ATTRIBUTES.md` `data-size` row; CLAUDE.md merge rules for PR 1783 under "Retired code"; new Task Layout.101 (`SEAMLY_DATA_DIR` in SeamlyLayout).
- Verified: `cargo test --workspace` 622 passed. Not run: ctest, Qt suites, MSI (Rust test and docs only).
- Open question for the user: untracked `project-docs/docs-seamlylayout/AAMA-INTEROP.md` copies the PR text, which describes the old `vdxf` output.
- Work done in worktree `C:\Users\susan\Projects\s2d-aama-piece-text` (removed after the merge).
- Uncommitted user files left untouched: `AGENTS.md`, `TODO_SEAMLYLAYOUT.md` (Layout.100), `AAMA-INTEROP.md`, `test-seamly-layout/foldline_arrowline.*`, `test-seamly-layout/layouts/*`.

### 2026-10-25 — Seamly2D.71 fold line arrow line, label and extended fold line; merged into local `run-seamlyLayout`, not pushed

- Piece Properties Apply/OK rotates the main path to end with the fold line (`VPiece::endPathAtFoldLine`). The 0.7.7 conversion does the same for older files (`endMainPathAtFoldLineV0_7_7`); no schema change.
- New `pattern_piece::FoldLineGraphic` (`src/libs/vpatterndb/FoldLineGraphic.{h,cpp}`): arrow line 50% of the fold line, 10% bends at each end, heads touch the fold line; "Foldline" label between the lines. Piece mode draws both (`PatternPieceTool::refreshFoldLine`).
- Extended fold line: `VPiece::foldLineOnCutLine`, drawn and sent while the cut line shows.
- Handoff: `fold_line` group = fold line path, then `fold_line_arrow` and `fold_line_label` groups. `SvgGenerator` renders tagged child items as nested tagged groups.
- SeamlyLayout: `svg_dom::fold_line_d`; Nested sizes with a fold line stack on its start (`center_piece_sets`, `annotation_alignment`); a piece-set sharing one fold line goes on the left edge; `piece_divider` splits the fold path only; DXF layer 8/15, HPGL, text modes.
- Decisions to confirm: ids `fold_line_arrow_<name>`, `fold_line_label_<name>` (scheme), not `<name>_foldline`; conversion in 0.7.7, not 0.7.8; Nested piece-sets now go on the left edge.
- Changed expectation: jacket2 import warning is now "Back: piece label past the cut line in sizes 220" (`AGENT_CHECKLIST.md` §26 updated).
- Risk: a custom seam allowance that starts at the fold line end node wraps after the rotation and is dropped.
- Code style: new code follows `README-CODE-STYLES.md` and the `AGENTS.md` note (snake_case, `Type*`, one exit, briefs, namespace, GPL link).
- Verified: scratch harness `TST_VPiece` 128, `TST_SvgComponentTags` 49 passed; `cargo test --workspace` 621 passed; `cl /Zs` clean. Not run: Qt suites (next MSI, ask first), ctest (no QML or C++ frontend change), GUI.
- User check: `AGENT_CHECKLIST.md` §29; §21 Nested step changed.
- Port note added to Dead.4.4 (`layout_handoff` must write the nested fold line parts).
- Scratch tools (session scratchpad `18654feb-…`): `harness/make_build.py` (two suites, changed lib sources rebuilt), `make_zs.py`.
- Uncommitted user files left untouched: `AGENTS.md`, `TODO_SEAMLYLAYOUT.md` (Layout.100), `test-seamly-layout/foldline_arrowline.*`, `test-seamly-layout/layouts/*`.

### 2026-10-24 — Dead.4.1 and Dead.4.2: old Seamly2D layout, print, console export, Export Pieces, `vdxf`, `vobj` removed; merged into local `run-seamlyLayout`, not pushed

- Plan: Task Dead.4 in `project-docs/docs-seamlylayout/TODO_REMOVE_DEAD_LAYOUT_CODE.md` replaces `src/libs/vlayout` in five steps. Dead.4.1 and Dead.4.2 are done; their MSI + user check (`AGENT_CHECKLIST.md` §28) is open.
- User decisions: delete the nesting engine, print tiling, layout UI, Export Pieces, File > Export As, the console export mode, `vdxf`, `vobj`. Keep Export Draft Blocks.
- User decisions: rewrite the handoff writer as new library `src/libs/layout_handoff/` (no `v` names, new code style, every `SVG-DATA-ATTRIBUTES.md` rule incl. the open seam line). Rename and restyle `VAbstractPiece` → `AbstractPiece`, `VTextManager` → `LabelTextManager` in `vpatterndb`.
- Removed: `--basename` export mode and its options (`-e` is `--gheight` and stays); Print/Preview/Tiled; tiled PDF; `VPoster`; New Print Layout, Layout Pages dock, Layout toolbar/toolbox, File > Export As; `LayoutSettingsDialog`, `DialogLayoutProgress`, `AbstractLayoutDialog`, `PageFormatCombobox`, `ExportProgressDialog`; layout settings; nesting engine classes; Export Pieces; `src/libs/vdxf`, `src/libs/vobj`.
- The export dialog now serves Export Draft Blocks only: SVG, PDF, PNG, JPG, BMP, TIF, PPM.
- Kept for later steps: `VLayoutPiece`, `VLayoutPiecePath`, `SvgGenerator`, `SvgTextItem`, `VGraphicsFillItem`, `VAbstractPiece`, `VTextManager`.
- Verified: `cl /Zs` on every `.cpp` of seamly2d, seamlyme, the touched libs and the Qt test suites. Not run: Qt suites, MSI (ask first), GUI.
- Next: Dead.4.3 (move and restyle `VAbstractPiece`, `VTextManager`).
- Work done in worktree `C:\Users\susan\Projects\s2d-layout-engine` (removed after the merge).

Older entries: `project-docs/SESSION_HANDOVER_ARCHIVE.md`. Do not Read it at session start.

## TODO by priority and owner

### Next items — Developer

- Layout.25 (media grows past the 500 in roll, user decision), then Layout.27.3 (repeat failures). Then Layout.70, 69, 27.11, 67.9.
- `TEST_CHECKLIST.md` line 104 names a `Front` piece; jacket2 has none. The file has user edits; ask before a change.

### Priority 1 — Developer: approved plan

- New task branch from `run-seamlyLayout`; implement the 2026-10-12 plan table in `project-docs/SESSION_HANDOVER_ARCHIVE.md`.

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

- **Layout.51, Layout.63, Layout.64:** remain blocked. The latest entry gives no release condition.
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
