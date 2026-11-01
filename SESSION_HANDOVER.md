# Session handover

## Current status

### 2026-10-28 — Fabric media size fix; merged into local `run-seamlyLayout`, not pushed

- Cause: Sue's trousers on fabric (folded, 36 in, selvedge 0.5 in) left 4 pieces out. The fabric canvas used the B0 paper sheet size that the settings keep from paper media; fabric width, fold and open length were ignored.
- Fix: `LayoutSettings::media_size_in()` gives the canvas (`page_w_px`, `page_h_px`) and the bin (`effective_bin_px`). Fabric: `fabricWidth` (0 = `pageWidth`; folded: half), `fabricHeight` (0 = 500 in, trimmed). Paper ignores the fabric fields.
- Commit `01562ef927`. Changed: `layout_settings.rs`, `layout_utils.rs`, `material_split.rs`, `size_labels.rs` (fabric fixtures use `fabricWidth`/`fabricHeight`), the layout and settings workflow docs.
- Verified: `cargo test --workspace` 636 passed; `ctest --preset debug` 7 of 7. Not run: MSI, GUI.
- Sue's popup had the old title, so her MSI predates `3b1310115e`.
- CLAUDE.md vcvars path: VS 2022 → VS 18 (user decision: local builds use VS 18; `local_build_msi.ps1` already does).
- Next: MSI build (ask). User test: trousers on 36 in folded fabric place all 12 pieces inside the fold half.
- `run-seamlyLayout` is 8 commits ahead of `origin`.

### 2026-10-27 — Warning popup titles and size noun; merged into local `run-seamlyLayout`, not pushed

- Cause: Sue saw jacket2's import warning under the title "Some pieces could not be placed". Import and layout warnings shared one fixed title.
- Merge `3b1310115e` (commit `3aa24ea580`), skip-ci token: `.rs`, `.qml`, `.md` only.
- Changed: `Main.qml` (titles "Import warning", "Layout warning"), `annotation_alignment.rs` (`size_list`: "in size 220"), `layout_utils.rs` test, `AGENT_CHECKLIST.md` §26.
- Verified: `cargo test --workspace` 634 passed; `ctest --preset debug` 7 of 7. Not run: MSI.
- User decisions: one shared place inside every size's cut line; built in `layout_handoff`, after the Dead.4.4.3 switch. Filed Task Layout.108 (commit `375954483c`, docs only).
- Decisions 108.4 (keep base place, warn in Seamly2D) and 108.5 (keep the SeamlyLayout import move) recorded, commit `a952e29d1e` (docs only).
- Fixed: working copies of `TODO_SEAMLYLAYOUT.md` and `TODO_REMOVE_DEAD_LAYOUT_CODE.md` dropped committed lines (stale editor buffer). Restored (user decision). Uncommitted Sue edits kept: Layout.102.5 wording, the `VPattern` question in Dead.4.4.3.
- `run-seamlyLayout` is 6 commits ahead of `origin`.

### 2026-10-27 — `nmake check` fix: `TST_Seamly2DCommandLine::ExportMode` removed; merged into local `run-seamlyLayout`, not pushed

- Cause: the MSI build stopped in `CollectionTest`. `ExportMode` ran `-p`, `-d`, `-b`, removed with the console export mode (Dead.4.1.1).
- Merge `8813562a51` (commit `899adb914a`), skip-ci token: test and doc change only.
- Changed: `tst_seamly2dcommandline.cpp/.h`, `TODO_REMOVE_DEAD_LAYOUT_CODE.md` Dead.4.1.1.
- Verified: in-tree `CollectionTest` rebuilt and run: 51 passed, 0 failed, 4 skipped (read-only checks, Windows ACLs). `ParserTest`, `TranslationsTest` exit 0. Not run: MSI.
- Open user decision: test mode ignores a bad `--gsize`/`--gheight` (exit 0). The 4 gradation rows of the old test are not ported. `glimited/*.sm2d` and `issue_372.sm2d` are now unused.
- `run-seamlyLayout` is 2 commits ahead of `origin`.
- Next: MSI build (ask).

### 2026-10-27 — Layout.106.1–106.8: handoff pieces built with `layout_handoff` too; merged into local `run-seamlyLayout`, not pushed

- User decisions: Option A (old and new lists filled in one size loop; SeamlyLayout keeps the old SVG until the Dead.4.4.3 compare passes). Compare switch: env `SEAMLY_HANDOFF_COMPARE_DIR=<absolute dir>` writes `<pattern>_old.svg` and `<pattern>_new.svg`. Removal filed as Task Layout.107.
- Merge `d7526a1ee6` (commits `9b509372c6`, `b2484d7dd8`). No skip-ci token: `.pri` and `.pro` changed.
- New: `layout_handoff/handoff_corner_cuts.h/.cpp` (`compare_corner_cuts`), `TST_HandoffCornerCuts`.
- Changed: `mainwindowsnogui.h/.cpp` (`handoff_pieces`, `createPlacedPiece`, `generateHandoffSvgDocument`, `writeHandoffCompareFiles`; corner cuts and node counts read the new lists), `mainwindow.cpp` (clears, compare call), size briefs in `handoff_document.h`, `handoff_ids.h`.
- Verified: `cl /Zs` of `mainwindowsnogui.cpp`, `mainwindow.cpp`, `handoff_corner_cuts.cpp`; scratch harness `TST_HandoffCornerCuts` 4 passed. Not run: MSI, Qt suites (`TST_HandoffPiece`, `TST_HandoffDocument`), GUI.
- Next: MSI build (ask). Then user test 106.9, then the compare with the env switch (Dead.4.4.3).
- `run-seamlyLayout` is 28 commits ahead of `origin`.

### 2026-10-27 — `layout_handoff` release build fix, docs, Task Layout.106 filed; merged into local `run-seamlyLayout`, not pushed

- Build fix (merge `22bcc09a30`): `layout_handoff.pro` sets `CONFIG -= precompile_header` (the lib has no `stable.h`) and `QT += widgets printsupport` (`vmisc/def.h` includes `QColorDialog`, `QPrinter`). Linux and macOS release builds had the same fault.
- Verified: scratch qmake + nmake release build of `layout_handoff` alone. Not run: MSI (ask first).
- Docs: data source, constraint and classes in `layout_handoff.pro` and the `handoff_document.h` `@details`; handoff rule bullet "The SVG comes from pattern data". Comments in `layout_handoff.pro` and `warnings.pri` rewritten.
- Filed Task Layout.106 (`TODO_SEAMLYLAYOUT.md`, commit `c3f0be692e`): close Gap 2, so `MainWindowsNoGUI` builds pieces with `layout_handoff`, not `VLayoutPiece`. Cross-reference at `TODO_REMOVE_DEAD_LAYOUT_CODE.md` Dead.4.4.3.
- Head commit has no skip-ci token: the push runs CI for the `.pro` fix.
- Finding: the `-O3` block in `layout_handoff.pro` gives no effect (`default_post.prf` adds `-O2` last). `CONFIG += optimize_full` is the working form.
- Open user decisions: (a) delete the `-O3` block, the `*_DEBUG_CXXFLAGS` lines and the Clang `-=` lines; (b) rewrite the `layout_handoff.pri:1-2` comment; (c) "writes" → "builds in memory" in the `.pro` and `HandoffDocument` briefs. (d) Layout.106.1: decided, see the entry above.
- Peer change kept: the staged blank line in `TODO_SEAMLYLAYOUT.md` stays in the index, on top of `c3f0be692e`.
- Merge `66ba40f4dc`: `include(../libs.pri)` moved next to the other includes in `layout_handoff.pro`. qmake Makefile identical. User comment `# include statements` not committed.
- Merge `745556996e`: include group moved below `RCC_DIR`, so the GCC and Clang `-isystem` flags name `moc/` and `rcc/` (they named `$OUT_PWD/`). `linux-g++` spec Makefile checked; MSVC Makefile identical. Other libs already set `MOC_DIR` first.
- Merge `974ef5fe20`: `#include <QShowEvent>` in 8 seamly2d dialogs. Fixes the MSI build C2027 in `calculator_dialog.cpp`, `dialognewpattern.cpp`, `dialogpreferences.cpp`. `cl /Zs` check passed. No skip-ci token: the push still needs CI for the `.pro` fix.
- `run-seamlyLayout` is 24 commits ahead of `origin`.
- Next: MSI build (ask).

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
