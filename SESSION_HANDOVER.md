# Session handover

## 2026-11-16 — CLI help examples

- Added the Examples tab to the command line dialog in the CLI worktree on run-seamlyLayout.
- Added SVG and tiled PDF commands for male_shirt.sm2d, with Paper > Tiled > Letter.
- Added a DXF R13 command for jacket2_40-146.sm2d, with a custom 42.00-inch paper roll.
- The jacket command selects Fabric and the base size.
- Paths come from the executable and the pattern and layout folders in Preferences.
- Added a Copy command button. The tab retains adjustable columns, filtering, and the application theme.
- Windows build passed. All 40 Qt suites passed: 32630 checks, zero failures.
- Rust workspace: 834 passed. SeamlyLayout: all nine release checks passed; this checkout has no debug build.
- Inspected the native dialog with separate preview settings. The sample commands did not run.
- The user requested a local commit and integration into run-seamlyLayout. No push was requested.


## 2026-11-16 — CLI worktree location

- Continue CLI work only in `C:\Users\susan\Projects\seamly2d-cli-1`, on `run-seamlyLayout`, as requested.
- The CLI worktree contains the merged table dialog updates at `cd044d9b09`.
- The main `Seamly2D-private` checkout remains detached at `cd044d9b09`. This preserves its files while the CLI worktree holds the branch.
- The two folders share Git history. At the final handoff, release the branch from the CLI worktree and check it out in the main folder.
- Do not edit files in the main folder during CLI work. Do not push without a request.

## 2026-11-16 — Command line help tables

- Replaced the console-style help window with native Qt tables that use the parent window's font and palette.
- Added adjustable columns, filtering, sorting, and tabs for options, limits, exit codes, and removed options.
- Windows Qt 6.11.3 build passed. All 40 Qt suites passed: 32629 checks, zero failures.
- Tests cover font and palette inheritance, registered descriptions, filtering, column resizing, and closing without exiting.
- The user stopped computer control before the visual check. Visual verification remains pending.
- Merged `task-command-line-help-table` locally into `run-seamlyLayout` at the user's request. No push.

## 2026-11-16 — CLI feature merged locally

- Merged `feature/cli-1` through `fa4e0e47ad` into `run-seamlyLayout` at the user's request.
- The merge had no conflicts. The source files match the tested feature branch.
- Windows verification from the feature branch: the build passed; all 40 Qt suites passed with zero failures.
- Linux and macOS verification remains deferred to CI.
- No push. Both branch worktrees remain available.

## 2026-11-16 — Command line help in the Help menu

- Added **Help > Command line options** on `feature/cli-1`.
- The window uses `VCommandLine::helpText()` for registered descriptions, limits, removed options, and exit codes.
- `--basename` remains removed; the window explains `--layout-export` and `--layout-output` as its replacements.
- Shared the dialog with the existing console-free `--help` fallback. Closing it from the menu does not exit Seamly2D.
- Verified the Windows Qt 6.11.3 build and all 40 Qt suites: 32628 passed, zero failures.
- The CLI suite includes 31 passing checks, including help content, scrolling, and closing without exiting.
- Linux and macOS verification remains deferred to CI. No merge or push.

## Current status

### 2026-11-16 — CLI.3 direct layout flags (feature/cli-1)

- Done; CLI.3 moved to `TODO_COMPLETED.md`. `TODO_CLI.md` keeps CLI.2 only (CLI.2.1 Richmond files, CLI.2.2 CAD import check).
- Seamly2D `--layout-<name> <value>` → SeamlyLayout `--<name> <value>` → `layout_tiling::apply_layout_flags` (precedence saved < `--layout-settings` < flags; media implications; conflicts → exit 64).
- `--layout-no-packing` (`--no-packing`): pieces where Piece mode places them; bridge `useHandoffPositions`.
- `--text2paths` back for SVG, PDF, tiled PDF, PS, EPS.
- Decisions (CLI.3.1, CLI.3.7) are recorded in `TODO_COMPLETED.md`; Sue may rename flags.
- Tests: CLI suite 130/130 (with `-RichmondRoot`); import check 30 files/0 fail; `cargo test --workspace` 734/0; `TST_VCommandLine` 29; `ctest` 9/9; Python headless runner pass.
- Richmond pattern and `2025-06-08-Sue.smis`: `git show d3197ee973^:test-seamly-layout-input/<file>`; not re-added (personal measurements).
- Import check with Inkscape 1.4.4, ezdxf, pypdf: 59 files, 0 fail. Open: a CAD program and a cutter (none installed); Linux/macOS (no WSL, no Docker, no Mac); CI (no push).
- Next: Sue reviews and merges `feature/cli-1` (not pushed).

### 2026-11-16 — CLI.1 audit and repair (feature/cli-1)

- Done; CLI.1 in `TODO_COMPLETED.md`. Branch `feature/cli-1`, not merged, not pushed. Plan `CLI_1_PLAN.md` (D1–D17); matrix `CLI_1_OPTION_MATRIX.md`; results and acceptance checklist `CLI_1_TEST_RESULTS.md`.
- New options: Seamly2D `--layout-material`, `--layout-view`, `--layout-oversize`, `--layout-info`, `--text2paths`; SeamlyLayout `--material`, `--view`, `--oversize`, `--info`, `--text2paths`. Removed `--layout-scale`/`--scale` (PNG fixed at 96 px/in).
- Tests: CLI suite 106/106; `local_build_msi.ps1` pass (`nmake check`, `ctest` 9/9, `cargo test` 730/0, MSI built, not installed); `TST_VCommandLine` 26; CollectionTest 69/0 (start in `bin`); tester 15/0; Python runners pass.
- Tester: `src/test/CliTestApp` (Rust/Iced, `--batch`), built in `target\release`.
- Blocked: Linux/macOS (no WSL, no Mac), CI (no push), Richmond `.sm2d` (not on this machine).
- Next: Sue reviews and merges. CLI.3 keeps the direct layout flags (names and precedence need Sue); CLI.2.1 needs the Richmond files.

### 2026-11-15 — Piece mode Details > Export Pieces (Seamly2D.73)

- `export_pieces_action` (E, P) below Copy Piece; `export_pieces_tool_button` below `copy_piece_tool_button`; toolbox +48 px. Piece mode only.
- `MainWindow::createPiecesSvg()`: one function for Layout mode and Export Pieces; Layout mode messages verbatim. Opening-pattern guard removed.
- `exportPiecesToSvgFile()`: `handleArrowTool(true)`; save dialog in `getLayoutPath()`; adds `.svg` when no suffix; write error message box.
- New `layout_handoff/handoff_svg_file.*`: `pieces_svg_file_name()`, `write_svg_file()` (QSaveFile, UTF-8). New `TST_HandoffSvgFile`.
- Checked: scratch harness `TST_HandoffSvgFile` 7/7 pass; `mainwindow.cpp` compiles (no new warnings). `nmake check` not run.
- Built in worktree `export-pieces`; merged `--no-ff` with no skip-ci token (`.pro`, `.pri` changed); branch and worktree deleted.
- Open: Seamly2D.73.5 user check and the Qt suites with the next `local_build_msi.ps1`.

### 2026-11-15 — Preferences > Paths: SeamlyLayout row removed (Seamly2D.200)

- `preferencespathpage.cpp`: row 10 "SeamlyLayout Application" removed from `initializeTable()`, `Apply()`, `defaultPath()`, `editPath()`. Table has 10 rows.
- Kept `paths/seamlyLayoutApp` and `VSettings::get/setSeamlyLayoutAppPath()`: `smsi_ensure_user_data.ps1` seeds the key; `Application2D::seamlyLayoutFilePath()` reads it first.
- Checked: `cl /Zs` on the file passes. No new test (UI row only).
- Built in worktree `layout-app-path`; merged `--no-ff` with no skip-ci token (diff removes an `#ifdef Q_OS_WIN` block); branch and worktree deleted.
- Open: Seamly2D.200.2 build check with the next `local_build_msi.ps1`.

### 2026-11-15 — Upstream `develop` merge in progress (a85828a6d0)

- `git merge develop` on `run-seamlyLayout` (bc458ff16a). Backup branch `intermediate` at the same commit. Not committed.
- Staged: `common.pri` = HEAD (no OpenSSL); `out/out.pro` removed; upstream DMG files in `packaging/macos/dmg/` (InstMacOS.01).
- `vcmdexport.*` → `command_line.*`: develop's method names, HEAD's `--layout-*` logic. Help prints to the console when stdout or stderr has a handle, else the upstream dialog.
- `mainwindow.cpp`: `DoExport` dropped; HEAD's `processCommandLine` and `exportLayoutFromCommandLine` with develop's names. Test files staged with the new names.
- Copy Piece: develop's Duplicate code removed. New `PatternPieceTool::copyPiece()` runs the right-click item and Details > Copy Piece (Ctrl+D, `duplicate_piece_action`). Both use `copy_piece.png`.
- New icon `toolicon/32x32/copy_piece.png` and `@2x` in `toolicon.qrc`; source `toolicon/svg/copy_piece.svg`. Edit menu Duplicate item removed.
- Unstaged: Piece mode Details page `copy_piece_tool_button` below the mirror button; triggers `duplicate_piece_action`. Toolbox +48 px.
- All conflicts resolved and staged. `mainwindowsnogui.cpp`: old print and export code stays deleted; header has both authors, "2013-2017 Valentina project", "2017-2026 Seamly2D project", SPDX.
- `vabstracttool.*`: our public `PrepareNodes`/`PrepareNode`; develop's `bloack_name` dropped; `blockName` → `block_name` in `AddRecord` and `PrepareNode`.
- Kept develop #1827 (062a71c901): the Move tool moves its rotation point with the objects, selected or not. It deletes 2 `TST_VToolMove` tests.
- Checked: no `convertPdfToPs`/`PDFTOPS` in code; no old `VCommandLine` names. Then: ask before `local_build_msi.ps1`; commit. No skip-ci token: `*.pro`, `*.pri`, packaging changed.
- TODO: `TODO_CLI.md` new CLI.3 (legacy layout options as `--layout-*` flags); CLI.1 paths now `command_line.cpp`.
- TODO: `TODO_SEAMLY2D.md` new Seamly2D.73 (Details > Export Pieces writes the handoff SVG string to a file).

## Decisions to retain

- CLI Linux and macOS checks run in CI (`gh workflow run ci.yml`), not in a local WSL install (user, 2026-11-16).
- SeamlyLayout is complete after the `develop` merge. Next focus: CLI layouts from pattern and measurement files plus parameters (user, 2026-11-15).
- Adopt develop's `command_line.*` file names and lowerCamel `VCommandLine` method names: they match the code style (user, 2026-11-15).
- Own CLI design (`--layout-export` to SeamlyLayout) replaces upstream console export. Lost legacy options go to `TODO_CLI.md` CLI.3 (user, 2026-11-15).
- Upstream per-app DMG files stay in `packaging/macos/dmg/` for a later 3-app DMG Finder layout (user, 2026-11-15).
- Keep our Copy Piece (`PatternPieceCopy`); remove develop's Duplicate. Details > Copy Piece calls our code with Sue's `copy_piece` icon (user, 2026-11-15).
- Keep develop's #1827 Move tool fix over our #853 rule: it fixed a problem (user, 2026-11-15).
- Canvas SVG goes to WebEngine through `CanvasSvgSchemeHandler` (`seamly-canvas://`, from memory). No `data:` URL: the 2 MB limit is not acceptable (user, 2026-11-15).
- Pieces with a fold line go first, down the left edge from the top, on every layout and tab, on paper and fabric (user, 2026-11-15).
- Piece Layout rotation Any samples 10° steps, not 1°; 1° steps are not efficient (user, 2026-11-15).
- "Place fold lines on the left edge" applies to paper and fabric; default on; it sits below Media Type. Paper and open fabric keep the left margin; only folded fabric has none (user, 2026-11-15). Replaces folded fabric only.
- No layout changes the width of a paper roll or fabric (folded or not), or the contentRect width. Only the length grows; a piece wider than the media is left out or split (user, 2026-11-14). Replaces Layout.71 roll widening.
- Multisize stack point with no right-angle turn point: the lowest turn point wins; the sizes align along its longest edge. An edge is the straight part only (user, 2026-11-14).
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

### Verification in the CLI worktree — 2026-11-16

- Rebuilt Seamly2D and Seamly2DTests in `C:\Users\susan\Projects\seamly2d-cli-1` with Qt 6.11.3.
- All 40 Qt suites passed: 32629 checks, zero failures, zero skips.
- The CLI suite passed 32 checks, including table content, font and palette inheritance, filtering, resizing, and closing.
- The updated application is `src/app/seamly2d/bin/seamly2d.exe` in this checkout. The installed application was not replaced.
- Continue CLI work here on `run-seamlyLayout`. No push or handoff to the main checkout occurred.
