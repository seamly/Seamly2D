# Session handover archive

Older `SESSION_HANDOVER.md` entries, newest first. Grep it; do not Read it whole.

### 2026-11-16 — Remove superfluous SeamlyCLI settings

- Removed Layout width and Layout length from the settings editor and generated JSON.
- Updated regression checks for omitted page dimensions and unit conversion of roll width.
- All 25 SeamlyCLI release unit tests passed.
- File: `src/app/seamlycli/src/layout_settings_editor.rs`.
- No commit or push.

### 2026-11-16 — Translation test linker failure

- The user reported `LNK1104` while the linker replaced `src/test/TranslationsTest/bin/TranslationsTest.exe`.
- No translation test process remained during inspection. The executable accepted exclusive write access.
- The targeted MSVC `nmake` retry linked `TranslationsTest.exe` successfully.
- The rebuilt `TranslationsTest.exe` returned exit code 0 with Qt 6.11.3 and the offscreen backend.
- The original lock owner remains unknown. No source change was necessary.
- The complete MSI build still needs a successful run.

### 2026-11-16 — Requested MSI build and branch cleanup

- Deleted the local `intermediate` branch at `bc458ff16a` at the user's request.
- Ran `packaging/windows/local_build_msi.ps1` with tests and MSI validation enabled, version `26.11.1620`.
- The sandbox blocked NuGet configuration access. The retry used approved access to the installed build tools.
- Seamly2D and SeamlyMe built. `CollectionTest` returned exit code 1 during `nmake check`.
- The script stopped before SeamlyLayout, SeamlyCLI, and MSI packaging.
- The diagnostic CollectionTest rerun passed. Its final suite reported 52 passed and four skipped.
- Results: `build/collection_test_msi_failure.txt`. The original failure remains unexplained.
- The full build remains incomplete. No commit or push followed this run.

### 2026-11-16 — SeamlyCLI application and installer

- Work uses Seamly2D-private on run-seamlyLayout.
- SeamlyCLI now resides in src/app/seamlycli and builds seamlycli.exe with Rust and Iced.
- The app needs no Qt to build or show its interface.
- Multisize report queries and exports support the required --basesize selection.
- The interface finds seamly2d through PATH and has no program selector.
- Pattern and measurement pickers use Seamly2D Preferences.
- Measurement selection uses the pattern or current measurement file to choose the individual or multisize folder.
- The interface edits layout settings or uses a selected JSON file with disabled controls.
- Unit changes convert physical lengths.
- The x64 MSI builds and packages SeamlyCLI with shortcuts and system PATH registration.
- ARM64 keeps its existing three applications.
- The complete Windows build and its Qt/Rust checks passed.
- The final SeamlyCLI build passed 25 unit tests.
- Visual checks covered the preference folders, disabled controls, and successful SVG/ZIP creation from entered settings.
- Development seamly2d/bin held Qt 6.11.1 DLLs. The matching packaged Qt 6.11.3 runtime passed the export checks.
- No installer ran on the user's system. No push was requested.

### 2026-11-16 — Copy Piece pick tool

- Defect: the Copy Piece button copied only a piece selected before the click. A click on a piece after the button did nothing.
- Fix: `Tool::CopyPiece` pick tool, like Union and Piece Mirror. Merged into local `run-seamlyLayout` with the skip-ci token.
- Files: `def.h`, `mainwindow.cpp/.h/.ui`, `cursor.qrc`, two new cursor PNGs, 18 `LAST_ONE_DO_NOT_USE` asserts (55 → 56).
- The cursor uses the uncommitted working-copy `copy_piece@2x.png` icon. Redraw it if the icon changes.
- Verified: scratch compile of the changed files. Not verified: MSI build, run-time check (open in `TODO_COMPLETED.md` S2D.77).
- Uncommitted icon, packaging, CLI, and doc edits from other sessions remain untouched.

### 2026-11-16 — Fix multisize collection tests

- The MSI build stopped because 10 CollectionTest cases omitted the required multisize size option.
- Added --basesize to the successful multisize TestMode case and the shared GOST collection command.
- Updated src/test/CollectionTest/tst_seamly2dcommandline.cpp and its required license header.
- Rebuilt CollectionTest. The SeamlyMe suite passed 17 checks. The Seamly2D suite passed 52 checks, with four filesystem skips.
- The focused nmake check returned zero. The full MSI build remains unverified.
- The existing three documentation edits remain unchanged. No commit or push.
- Next step: rerun packaging/windows/local_build_msi.ps1 to complete all tests and MSI packaging.

### 2026-11-16 — Feature branches merged and deleted

- Merged all four feature branches into local run-seamlyLayout with separate merge commits.
- S2D.73 adds generated SVG file checks. Its merge commit is 22c5463a1d.
- S2D.75 preserves flipped geometry in Export Pieces and the SVG handoff. Its merge commit is 9426a55bd7.
- Export Drafts retains the construction scene, as the user requested.
- S2D.76 aligns grainlines with flipped anchors. Its merge commit is c2e06fd010.
- S2D.77 binds the Copy Piece button to its action. Its merge commit is 3497cab77d.
- Deleted the four merged task branches and their clean feature worktrees.
- The detached CLI reference worktree remains unchanged.
- Restored the three existing documentation edits as uncommitted changes.
- TODO_COMPLETED.md retains the saved content and both new completion records.
- TODO_SEAMLYLAYOUT.md matches its backup byte for byte.
- TODO_SEAMLY2D.md retains the user edits and the merged test results. Its completed S2D.76 and S2D.77 descriptions moved to TODO_COMPLETED.md.
- Kept the safety stash ff305da614b6aabdb03002debf7b190ca3f8509f and the external documentation backup.
- Prior verification passed: 130 focused checks, eight SeamlyLayout checks, and 734 Rust tests.
- The main-window Copy Piece check passed, including mode changes, copy, undo, and redo.
- Merge resolutions changed documentation only. The merged feature source matches the tested commits.
- The full Windows build and user visual checks remain pending.
- No push was requested or performed.

### 2026-11-16 — Returned to the main worktree

- The user ended the isolated CLI work and requested a return to Seamly2D-private.
- The main worktree now checks out run-seamlyLayout with all CLI commits through 7f4a5c4173.
- The CLI worktree remains detached at 7f4a5c4173 as a reference.
- Both worktrees share one repository. The handoff required no merge or duplicate commits.
- Restored three pre-existing documentation edits without conflicts.
- Verified the two TODO files byte for byte against the backup.
- Verified TODO_COMPLETED.md contains the original edits and the new CLI completion records.
- Kept the safety stash and an external backup of the three files.
- Continue project work in the main worktree. No push was requested.
- The source files did not change during the handoff. The recorded CLI test results still apply.

### 2026-11-16 — S2D.78 multisize base size

- Implemented --basesize in the CLI worktree on run-seamlyLayout.
- Multisize console runs require --gsize or --basesize. Both options together cause exit 64.
- --basesize reads the active measurement base and converts its unit. Individual measurements cause exit 65.
- --gheight can accompany either size option.
- --basesize exports one size by default. An explicit --layout-view still selects a multisize view.
- --layout-info with --basesize retains all available views.
- GUI file opening does not require a size selection.
- Added descriptions and limits to the native help dialog. Updated its jacket example.
- Windows build passed. All 40 Qt suites passed: 32640 checks, zero failures.
- All seven basesize CLI cases passed. Four geometry checks cover numeric selection, measurement overrides, and unit conversion.
- All 834 Rust tests and nine available SeamlyLayout release checks passed.
- Work remains in the CLI worktree. No push was requested.

### 2026-11-16 — CLI help examples

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

### 2026-11-16 — CLI worktree location

- Continue CLI work only in `C:\Users\susan\Projects\seamly2d-cli-1`, on `run-seamlyLayout`, as requested.
- The CLI worktree contains the merged table dialog updates at `cd044d9b09`.
- The main `Seamly2D-private` checkout remains detached at `cd044d9b09`. This preserves its files while the CLI worktree holds the branch.
- The two folders share Git history. At the final handoff, release the branch from the CLI worktree and check it out in the main folder.
- Do not edit files in the main folder during CLI work. Do not push without a request.

### 2026-11-16 — Command line help tables

- Replaced the console-style help window with native Qt tables that use the parent window's font and palette.
- Added adjustable columns, filtering, sorting, and tabs for options, limits, exit codes, and removed options.
- Windows Qt 6.11.3 build passed. All 40 Qt suites passed: 32629 checks, zero failures.
- Tests cover font and palette inheritance, registered descriptions, filtering, column resizing, and closing without exiting.
- The user stopped computer control before the visual check. Visual verification remains pending.
- Merged `task-command-line-help-table` locally into `run-seamlyLayout` at the user's request. No push.

### 2026-11-16 — CLI feature merged locally

- Merged `feature/cli-1` through `fa4e0e47ad` into `run-seamlyLayout` at the user's request.
- The merge had no conflicts. The source files match the tested feature branch.
- Windows verification from the feature branch: the build passed; all 40 Qt suites passed with zero failures.
- Linux and macOS verification remains deferred to CI.
- No push. Both branch worktrees remain available.

### 2026-11-16 — Command line help in the Help menu

- Added **Help > Command line options** on `feature/cli-1`.
- The window uses `VCommandLine::helpText()` for registered descriptions, limits, removed options, and exit codes.
- `--basename` remains removed; the window explains `--layout-export` and `--layout-output` as its replacements.
- Shared the dialog with the existing console-free `--help` fallback. Closing it from the menu does not exit Seamly2D.
- Verified the Windows Qt 6.11.3 build and all 40 Qt suites: 32628 passed, zero failures.
- The CLI suite includes 31 passing checks, including help content, scrolling, and closing without exiting.
- Linux and macOS verification remains deferred to CI. No merge or push.

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
- Tester: `src/app/seamlycli` (Rust/Iced, `--batch`), built in `target\release`.
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

### Verification in the CLI worktree — 2026-11-16

- Rebuilt Seamly2D and Seamly2DTests in `C:\Users\susan\Projects\seamly2d-cli-1` with Qt 6.11.3.
- All 40 Qt suites passed: 32629 checks, zero failures, zero skips.
- The CLI suite passed 32 checks, including table content, font and palette inheritance, filtering, resizing, and closing.
- The updated application is `src/app/seamly2d/bin/seamly2d.exe` in this checkout. The installed application was not replaced.
- Continue CLI work here on `run-seamlyLayout`. No push or handoff to the main checkout occurred.

### 2026-11-15 — Fold lines on the left edge of paper and fabric (Layout.Y111)

- `LayoutSettings::places_fold_lines_on_left_edge`: no folded fabric check. Layout Mode Any still ignores it.
- Paper and open fabric: fold line pieces go first, on the left margin; the margin stays. Folded fabric unchanged: the fold, no margin, piece gap for other pieces.
- `SettingsDialog.qml`: "Place fold lines on the left edge" on the line below Media Type; enabled for every media; "(fabric: no left margin)" dropped.
- Default stays on: paper layouts now pin fold line pieces unless the user clears the box.
- Tests: `fold_lines_on_left_edge_apply_to_every_media`, `fold_line_lies_on_the_left_margin_of_paper_and_open_fabric`, `jacket2_paper_and_open_fabric_place_fold_line_pieces_first`; Qt `foldLinesOnLeftEdgeCheck_belowMediaTypeForEveryMedia`.
- Checked: `cargo test --workspace` 719 pass; `ctest` debug 9/9. Built in worktree `fold-media`; merged `--no-ff`; branch and worktree deleted.
- Docs: SETTINGS_WORKFLOW, CREATE_LAYOUT_WORKFLOW, PROCESS LAYOUT WORKFLOW, SVG-DATA-ATTRIBUTES, `AGENT_CHECKLIST.md` §21, new §58 and OPEN row; §54 and its row removed.
- Open: Layout.Y111.1 installed-app check with the next MSI (§58).

### 2026-11-15 — Large patterns display in both canvases (Layout.Y10)

- Cause: jacket2 resaved with 16 pieces; real handoff 2,024,522 chars. Fabric all-sizes, Marker and Nested SVGs went into `loadHtml()` `data:` URLs over Chromium's 2 MB limit; the load failed with no error.
- New `qt_frontend/src/CanvasSvgSchemeHandler`: serves each canvas SVG from memory at `seamly-canvas://<canvas>/<n>.svg`. Scheme registered before `QApplication`; handler on the default profile; QML singleton.
- `SvgCanvas.qml`: `canvasName` (`input`, `layout`); an `<img>` loads the scheme URL; "Cannot display this SVG." when `naturalWidth` is 0.
- CMake: `Qt6::WebEngineCore`; new `CanvasSvgSchemeHandlerTests` (10 tests; offscreen, no sandbox, no GPU).
- `webEngine_displaysSvgOverDataUrlLimit` (3 MiB SVG) fails with the old `data:` URL path.
- Checked: `ctest` debug 9/9; `cargo test --workspace` 717 pass. Built in worktree `canvas-svg`; merged `--no-ff` with no skip-ci token (CMakeLists changed); branch and worktree deleted.
- Docs: README "Canvas display", DECISIONS record, CXXQT_BRIDGE, CODEBASE_INDEX, rules (architecture, svg-processing, dependencies), `AGENT_CHECKLIST.md` §57 and OPEN row.
- Open: Layout.Y10.1 installed-app check with the next MSI (§57). The WebEngine tests run first on Linux and macOS in CI.

### 2026-11-15 — Fold line pieces go first on every tab (Layout.Y9)

- Cause: the order search (hill climbing) moved left-edge pieces after other pieces. Any layout could break; the Base Size tab was right by chance.
- `maxrects_order_search.rs` `PackJob::left_edge_first`: every packed order puts left-edge pieces first; each group keeps its order. Start orders and hill climbing use it.
- Fold line pieces stack down the left edge from the top; the media gets longer when it must.
- Tests: `left_edge_pieces_are_placed_first`, `jacket2_size_tabs_place_fold_line_pieces_first` (26 sizes), `jacket2_marker_and_nested_place_fold_line_pieces_first`. The first two fail without the fix.
- Checked: `cargo test --workspace` 717 pass; `ctest` debug 8/8. Built in worktree `fold-sizes`; merged `--no-ff`; branch and worktree deleted.
- Docs: PROCESS LAYOUT WORKFLOW (piece-order search), CREATE_LAYOUT_WORKFLOW, `AGENT_CHECKLIST.md` new §56 and OPEN row.
- Open: Layout.Y9.1 installed-app check with the next MSI (§56).

### 2026-11-15 — Layout rotation Any samples 10° steps (Layout.Y8)

- `pack_types`: `ANY_ROTATION_STEP_DEG` = 10. `PieceRotationRule::Any` gives 0°, 10°, … 350° (36 angles, was 360).
- `SettingsDialog.qml` Piece Gap tip: "Layout rotation Any samples 10° steps."
- Handoff unchanged: `data-rotation="1"` still means Any.
- Tests: `piece_rotation_rules_resolve_by_layout_mode`, `any_piece_uses_non_quarter_turn_to_fit`, `mixed_piece_rules_keep_allowed_angles_and_unplaced_ids` (84×75 bin, 40°).
- Checked after the merge with Layout.Y7: `cargo test --workspace` 714 pass; `ctest` debug 8/8. Built in worktree `rotation-step`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES `data-rotation`, SETTINGS_WORKFLOW, LAYOUT_ROTATION_PLAN, `TODO_ROTATION.md`, `AGENT_CHECKLIST.md` §40 tip text, new §55 and row.
- Open: Layout.Y8.1 installed-app check with the next MSI (§55).

### 2026-11-15 — Fold edge option in the Fabric section (Layout.Y7)

- `SettingsDialog.qml`: "Place fold lines on the left edge" moved to the Fabric section, below "Fabric Folded". Enabled only when Fabric Folded is checked.
- Default on: `LayoutSettings` serde default, `SettingsModel` member and `resetToDefaults`, `default_settings.json`.
- `LayoutSettings::is_folded_fabric`: `places_fold_lines_on_left_edge` and the zero left margin need folded fabric. Paper and open fabric ignore a saved value.
- `layout_utils.rs` uses `is_folded_fabric` at both fold checks.
- Tests: `fold_lines_on_left_edge_apply_to_folded_fabric_only`, `fold_lines_on_left_edge_default_on`; paper and across-grain cases in `layout_utils`; Qt `foldLinesOnLeftEdge_defaultsOnAndRoundtrips`, `foldLinesOnLeftEdgeCheck_enabledOnlyForFoldedFabric`.
- Checked: `cargo test --workspace` 714 pass; `ctest` debug 8/8. Built in worktree `fold-edge`; merged `--no-ff`; branch and worktree deleted.
- Docs: SETTINGS_WORKFLOW, CREATE_LAYOUT_WORKFLOW, SVG-DATA-ATTRIBUTES, `AGENT_CHECKLIST.md` §21, §39, new §54 and OPEN row.
- Open: Layout.Y7.3 installed-app check with the next MSI (§54).

### 2026-11-14 — Paper roll and fabric keep their width (Layout.Y6)

- `layout_utils::do_process_layout`: every non-tiled pack uses `bin_w`, the contentRect width. Removed `pack_bin_w`, `mr_px`, `set_content_width` (`layout_assembler.rs`).
- A piece wider than a paper roll or fabric stays unplaced; the warning names it. Only the length grows.
- `lib.rs` `process_layout`: the oversized-piece check runs for every layout. Multisize paper rolls now get the dialog too.
- `Main.qml`: dialog choice "Make the media longer"; text says the media keeps its width.
- `enlarged_media_message` compares the length only.
- Tests: `placement_enlargement_dimensions_and_notifications`, `jacket2_marker_keeps_narrow_roll_width` (replaces `jacket2_marker_widens_narrow_roll`).
- Checked: `cargo test --workspace` 713 pass; `ctest` debug 8/8. Built in worktree `roll-width`; merged `--no-ff`; branch and worktree deleted.
- Docs: PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` new §53 and OPEN row; §11 and its OPEN row removed.
- Open: Layout.Y6.1 installed-app check with the next MSI (§53).

### 2026-11-14 — Tilted piece: grainline tilts (Layout.Y5)

- Seamly2D Piece Mode: `PatternPieceTool::updateGrainline` turns the grainline with the piece tilt, about the same center. Labels stay level.
- `applyTiltToGeometry` keeps the tilt in `m_tilt_degrees` and `m_tilt_center`.
- Grainline move, resize and rotate remove the tilt before they store the grainline; the pattern keeps a level grainline.
- Flip is unchanged: the grainline still does not mirror (applyFlipToGeometry rule).
- Test: `TST_VPiece::PieceTiltTransformTurnsGrainline`, passed in a scratch QtTest build. `pattern_piece_tool.cpp` compiles (cl). Full Qt suites run in the next MSI.
- Built in worktree `tilt-grain`; merged `--no-ff`; branch and worktree deleted.
- Docs: `AGENT_CHECKLIST.md` new §52 and OPEN row; `TEST_APPLICATION_UPDATES.md` tilt line.
- Open: Layout.Y5.1 installed-app check with the next MSI (§52).

### 2026-11-14 — Split drill holes (Layout.Y4)

- `piece_divider.rs` `drill_for_parts`: a cut line that crosses a drill circle splits it. Each part's `<circle>` becomes a `<path>` of its arcs (cubic curves, 90° or less each) that end at the cut line.
- The part nearest the centre keeps `<circle r="0" stroke="none" fill="none">`: one DXF-ASTM layer 13 drill point. Decision: DXF has no half drill hole; the point stays where the centre is.
- Cubic curves, not SVG `A` arcs: `svg_dom` bakes an arc wrongly under a flip.
- `seamly_svg2ezdxf` `extract_drill_hole`: a split part (`data-split-of`) with no circle writes no point; a zero-radius circle writes the point. Unsplit pieces keep the old checks.
- Tests: `drill_hole_splits_at_the_cut_line`, `narrow_cell_holds_two_arcs`, `split_drill_hole_keeps_one_drill_point`; two reject cases.
- Checked: `cargo test --workspace` 712 pass; `ctest` debug 8/8. Built in worktree `split-drill`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES, PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, DXF-ASTM compliance, `AGENT_CHECKLIST.md` §48, new §51 and OPEN row.
- Open: Layout.Y4.1 installed-app check with the next MSI (§51).

### 2026-11-14 — Nested: lowest corner without a right angle (Layout.Y3)

- Stack point Rule 2a: a right angle at one or more turn points keeps the old rule.
- Rule 2b, no right angle: the lowest turn point (largest y' in the grain frame) wins. A level tie uses the 2a edge ranks, then left.
- Right angle: the two edges meet within 1° of 90°. The first point away from the turn point sets each edge's direction.
- Same rule in `svg_dom/src/stack_point.rs` (`ranks_lower`, `RIGHT_ANGLE_COSINE`) and `layout_handoff/handoff_stack_point.cpp`.
- jacket2: only Detail 3 changes. Detail 2 has no right angle but has a fold line (Rule 1).
- Fixture `jacket2_40-146_handoff.svg` realigned: 25 Detail 3 grainline transforms.
- Tests: `right_angle_is_within_one_degree`, `without_a_right_angle_the_lowest_corner_wins`, Qt `stackPointTakesLowestCornerWithoutRightAngle`. Trapezoid fixtures are right trapezoids.
- Checked: `cargo test --workspace` 711 pass; `ctest` debug 8/8; `TestHandoffAnnotationAlignment` 19/19 in a scratch QtTest build. Full Qt suites run in the next MSI.
- Built in worktree `nested-corner`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES "Multisize stack point"; `AGENT_CHECKLIST.md` new §50 and OPEN row.
- Open: Layout.Y3.1 installed-app check with the next MSI (§50).

### 2026-11-14 — Marker and Nested unavailable after a split (Layout.Y2)

- Multisize "Split the pieces": the run lays out only Base Size and each size (`layout_views::size_kinds`, was `tiled_kinds`), every material.
- `LayoutViews::start(kind, split)`: a split run puts Marker and Nested views first, with no layout; Base Size shows first.
- New QML property `layoutViewUnavailableText` (`SPLIT_UNAVAILABLE_TEXT` on those tabs): canvas shows it; no media size; Adjust, Export, View and fit-to-view off.
- Export all tabs skips them: `LayoutViews::in_all_tabs_export`.
- Removed the Nested piece-set split (`divide_unit`, `divide_piece_set`, `outline_bounds`, `PiecePart`, its test): no split run lays out Nested now.
- Tests: `split_run_makes_marker_and_nested_unavailable`, `unsplit_run_keeps_marker_and_nested`; jacket2 split test lays out size tabs only.
- Checked: `cargo test --workspace` 709 pass; `ctest` debug 8/8. Built in worktree `split-tabs`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES, PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` §47, new §49 and OPEN row.
- Open: Layout.Y2.1 installed-app check with the next MSI (§49). X7.4 (Nested set too wide) no longer applies.

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

### 2026-11-14 — Split part labels move inside the cut line (Layout.X1)

- New `label_inside_part.rs`: after a split, a `pattern_label`, `piece_label` or `fold_line_label` not wholly inside its part outline moves inside it.
- Search: grid of whole steps from the old centre, nearest first. Order: full size, 1/16 in clear of other labels and the part label place; full size anywhere inside; shrink about the centre in 10% steps.
- No place at 10%: the label stays. The move and scale go in through `svg_dom::flatten_dom`.
- `piece_divider::divide_piece` calls it before `place_part_label`; reserved box: `part_label_placement::along_edge_bounds`.
- `part_label_placement.rs`: `drawn_bounds`, `along_edge_bounds`, `Bounds::grown`/`overlaps`, `inside`, `x_range_in_band` shared.
- Checked: `cargo test --workspace` 701 pass; `ctest` debug 8/8. Fixtures richmond (8 in roll) and jacket2 (12 in roll): crossing labels moved inside; 155 calls, 61 ms debug.
- Decision for Sue: shrink fallback for a label larger than its part (10% steps). Alternative: leave it at full size.
- Docs: PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, SVG-DATA-ATTRIBUTES split parts and `fold_line`, `AGENT_CHECKLIST.md` §44.
- Open: Layout.X1.2 installed-app check (male_shirt, roll 8 in, split).

### 2026-11-14 — Labels show `<empty>` as a blank line (Layout.X3)

- `LabelTextManager::isBlankLineMarker`: `<empty>`, or its editor (`EditLabelTemplateDialog`) or reader (`VDomDocument`) translation.
- `prepareLines`: a marker line becomes a `TextLine` with no text; it keeps its font size increment and one line height. A line that the placeholders leave empty is still dropped.
- Covers the Seamly2D scene labels and the handoff SVG (`add_label` writes no `<text>`, moves the next line down). Edit template Preview tab shows a blank row.
- `seamly_svg2ezdxf` converter still drops `<empty>` text: it comes only from SVG files of older Seamly2D builds.
- Test: `TST_LabelTextManager::blankLineMarker`.
- Checked: test program `TST_LabelTextManager` 12/12 pass (`build/scratch_label_empty/`, in-tree libs). `cargo test` not run: Rust comment only. Qt suites run with the next MSI.
- Docs: SVG-DATA-ATTRIBUTES `piece_label`; `AGENT_CHECKLIST.md` §43 and OPEN row; TODO Layout.X3.1.
- Open: Layout.X3.2 installed-app check with the next MSI (§43).

### 2026-11-14 — Layout tabs show the media size (Layout.X5)

- Each layout tab shows `<W> x <H> <unit>` below its label, in a smaller font: "36.00 x 52.25 in".
- Width: paper canvas width (a roll can widen); fabric: the bolt width, folded too (as "Media enlarged"). Height: the tab's layout length.
- Rust: `layout_utils::media_size_in_unit` (shared with `enlarged_media_message`), `layout_views::media_size_text`, `LayoutViews::media_sizes`.
- Bridge: QML property `layoutViewMediaSizes`; `run_layout` keeps the parsed `layout_settings`; `exit_adjust_mode` refreshes the tab properties.
- `Main.qml`: tab `contentItem` is a Column: label, then media size.
- Tests: `media_size_text_uses_the_settings_unit`, `media_sizes_follow_each_tab_layout`.
- Checked: `cargo test --workspace` 693 pass; `ctest` 8/8 pass (build dir `Seamly2D-private-worktrees/x5dbg`: the preset path in a worktree passes MAX_PATH).
- Docs: SIMPLE_WORKFLOW tab list; `AGENT_CHECKLIST.md` §42; TODO Layout.X5.1.
- Open: Layout.X5.2 installed-app check with the next MSI (§42).

### 2026-11-14 — Multisize piece names without the piece number (Layout.X6)

- `piece_naming.rs`: multisize `Size_<size>_<name>[_<letter>]`; no name → `Size_<size>_Piece_<n>[_<letter>]`. Piece-set: same without the size.
- User decision: names stay unique. A named piece whose name another piece also gets keeps `Piece_<n>` (`Size_34_Piece_2_Detail`).
- User decision (Layout.X6.5): individual `<name>[_<letter>]`; no name → `Piece_<n>[_<letter>]`. Same shared-name rule. Tests: `individual_name_drops_the_number`, `individual_shared_name_keeps_the_number`; cargo 692 pass, ctest 8/8. §41 trousers step.
- Naming now reads every piece first (`collect_pieces`, `planned_names`), then writes.
- `unplaced_piece_lines`: an assigned name is the whole line; no `: <size>`. One line per left-out size.
- Tests: `multisize_name_drops_the_number`, `unplaced_piece_lines_use_assigned_names`; jacket2 export test now expects `Size_500_<name>_<letter>`.
- Checked: `cargo test --workspace` 691 pass; `ctest --preset debug` equivalent 8/8 (worktree build dir `x6dbg`; preset path passed MAX_PATH).
- Docs: SVG-DATA-ATTRIBUTES "SeamlyLayout piece names"; `AGENT_CHECKLIST.md` §5, §41; TODO Layout.X6.1–.3.
- Open: Layout.X6.4 installed-app check with the next MSI (§41).

### 2026-11-14 — Piece Properties: Layout rotation (Layout.X8)

- User decision: Seamly2D does not know the SeamlyLayout Layout Mode, so Rotation stays visible. Rename and add a line instead of hiding.
- `pattern_piece_dialog.ui`: label "Rotation:" → "Layout rotation:". New `rotationLayoutMode_Label` below the radio buttons: "Rotation applies when Layout Mode=Any in SeamlyLayout."
- SeamlyLayout `SettingsDialog.qml` Piece Gap tip and docs (SVG-DATA-ATTRIBUTES, NEW-ATTRIBUTES, SETTINGS_WORKFLOW, LAYOUT_ROTATION_PLAN) say "Layout rotation".
- Checked: `uic` builds the `.ui`; `ctest --preset debug` 8/8. `cargo test` not run: no Rust change. Seamly2D Qt suites not run (next MSI).
- Open: Layout.X8.2 installed-app check, `AGENT_CHECKLIST.md` §40.
- `.ts` translation files not updated: lupdate picks up the new strings.

### 2026-11-12 — Nested sizes stack on a corner (Layout.804)

- Merged into `run-seamlyLayout` without skip-ci (`.pri`/`.pro` changed). Branch `task-nested-stack-corner` deleted.
- Stack point: Rust `svg_dom/src/stack_point.rs`; C++ `layout_handoff/handoff_stack_point.cpp` mirrors it. Users: `center_piece_sets`, `annotation_alignment.rs`, `align_handoff_annotations`.
- Rules: fold line in every size → its lower end (left end if level). Else turn points in every size → largest size's turn point with the longest level edge, else longest edge. Else bbox centre.
- User decisions: fold line first, lower end; both apps; tie → longer other edge, then lower, then left.
- New C++ `handoff_svg_reading.h/.cpp` (`find_tagged_child`, `read_matrix`, `collect_points`).
- Rust `annotation_alignment::inside`: within 0.01 px of the cut line counts as inside (matches C++). Test `anchor_on_the_cut_line_is_inside`.
- `test_data/jacket2_40-146_handoff.svg`: 227 annotation transforms realigned by the new C++ code (scratch `realign.exe`). On the old fixture it gives 0 warnings: Back's size 220 import warning is gone. A fresh jacket2 export is still needed.
- Checked: scratch QtTest `TestHandoffAnnotationAlignment` 18/18; `cargo test --workspace` 689 pass; `ctest --preset debug` 8/8. MSI Qt suites not run.
- Docs: SVG-DATA-ATTRIBUTES "Multisize stack point"; SETTINGS_WORKFLOW fold line; `AGENT_CHECKLIST.md` §26 (no import warning), §39, OPEN row; TODO Layout.804.1–.4.
- Open: Layout.804.5 with the next MSI: 4 Qt suites, `AGENT_CHECKLIST.md` §39.
- Scratch: `build/scratch_corners/` (`fix_*.py`, `qttest/` QtTest build, `realign.exe`, `ctest_debug.bat`).

### 2026-11-12 — Layout Mode Any turns a wide piece that fits turned (Layout.802)

- Cause: the oversized-piece check used each piece's `data-rotation`. Seamly2D writes `0` (None) by default, so Any tried 0° only.
- `piece_extractor::turn_wide_pieces_to_fit`: Any, piece too wide at its rule's angles, 90° turn fits → rule becomes quarter turns.
- Called by `piece_divider` `prepared_pieces` (check and split) and `do_process_layout` (packer). Tiled paper: not called.
- Tests: `wide_piece_turns_only_when_the_turn_fits`, `any_mode_turns_a_wide_piece_that_fits_turned`; `piece_rotation_controls_split_and_stays_on_parts` updated.
- Checked: `cargo test --workspace` 672 pass; `ctest --preset debug` 8/8.
- Docs: PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, LAYOUT_ROTATION_PLAN, SVG-DATA-ATTRIBUTES `data-rotation`, `AGENT_CHECKLIST.md` §38.
- Open: Layout.802.5 installed-app check (trousers, roll 16 in, Any: `Union_piece` not listed).
- Found, not changed: dead code warnings `piece_extractor::piece_bbox`, `exports::build_tiled_pdf_tile_doc`.

### 2026-11-12 — Split part label on a short cut edge (Layout.801)

- Found by the user: trousers `Union_piece` split on a 16 in roll; 24 px label (~492 px Hershey) ran past the 227 px cut edge.
- First fix: shrink the label to the edge (font 24 → ~11 px, `textLength`). User decision: keep the 0.25 in text.
- New `cxxqt_bridge/src/part_label_placement.rs` `place_part_label`: label fits edge → along the edge, as before.
- Label wider than edge → level, 24 px, clear row nearest the cut edge. Clear: no outline edge, path or text within 0.25 em.
- No clear row → shrink along the edge (old fallback). `piece_divider.rs` `fit_label_to_edge` removed.
- Real trousers export: both labels 24 px, inside the seam line, below the existing labels, past the notches.
- Tests: `part_label_placement` (5), `piece_divider` `part_label_fits_its_part`. `cargo test --workspace` 677 pass; `ctest --preset debug` 8/8.
- Docs: SVG-DATA-ATTRIBUTES "split parts", SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` §37 (now Layout Mode Along Grainline: Any turns `Union_piece`).
- Open: Layout.801.4 installed-app check with the next MSI.

### 2026-11-12 — Export > SVG has no check boxes (Layout.800)

- `ExportMenu.qml`: the three SVG text mode items are plain items. A click exports.
- Removed: `lastSvgTextMode` (QML), `PreferencesModel` `svgTextMode`, `isSvgTextMode`, `saveSvgTextMode`, INI key `svg_text_mode`. An old INI keeps the key; SeamlyLayout ignores it.
- Kept: the headless `--svg-text-mode` option (`StartupOptions`), Rust `SvgTextMode`.
- Test: `ExportMenuTests::svgModeItemsHaveNoCheckBox` (3 rows). 3 `PreferencesModelTests` removed.
- Checked: `ctest --preset debug` 8/8. `cargo test` not run: no Rust change.
- Open: Layout.800.3 installed-app check with the next MSI (`AGENT_CHECKLIST.md` §36).

### 2026-11-12 — TODO_DXF.md cleaned

- Closed → `TODO_COMPLETED.md`: DXF.21, DXF.3, DXF.8 (completed); DXF.22 (obsolete). Findings table moved there too.
- Evidence: aw.fyi `male_shirt_R13_202611012029.dxf` (2026-11-01): pleat and slit ends not flagged; FrontPanel_M not flagged. jacket2 Base Size 500 reports (2026-10-23, 11-07, 11-08): 0 departures.
- DXF.3.5 CLO3D import check now lives only in Layout.102.5 (`AGENT_CHECKLIST.md` section 8).
- Open: DXF.18.5, rewritten with steps and expected result. `male_shirt` CollarTopInterface-Built-in-SA has no notch.
- Found, not investigated: the 2026-11-01 and 2026-11-07 male_shirt DXF files have 0 notches. `male_shirt.sm2d` FrontPanel and BackPanel have 1 notch each.
- Compliance doc "aw.fyi results": 4 new rows.

### 2026-11-12 — Plain words in the TODO file

- Rule added: describe literal actions and data; no made-up names, no wrong jargon. `CLAUDE.md` (d4809bc650). Agent file `.claude/agents/seamlylayout-developer.md` too: local only, gitignored.
- `TODO_SEAMLYLAYOUT.md` open items rewritten in plain words (bd756c7227). Each "Next MSI: user check" line now holds the steps and the expected result. Checked lines unchanged, except Layout.108 and the "harness" and "scratch" words.
- Fixed: the "Layout.58.13" cross-reference is Layout.68.13.
- Answer recorded in Layout.108: Seamly2D writes the SVG as text to SeamlyLayout's standard input. No file is written. SeamlyLayout reads the text and imports the pieces.
- `AGENT_CHECKLIST.md` OPEN list: Layout.88.2 "FAIL --> Note did not appear" (male_shirt).
- Next: Layout.108.7 docs, in plain words. `AGENT_CHECKLIST.md` section 26 and `SVG-DATA-ATTRIBUTES.md` "Multisize grainline and label positions" both use "stack".

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

### 2026-11-12 — .gitignore track rules

- Git uses the last matching rule: the track block now sits at the end of `.gitignore`. Keep it there.
- Deleted the macOS `Icon` rule; with `core.ignorecase` it matched only `icon/` folders.
- Track: new `CMakeLists.txt`, `CMakePresets.json`, PNGs in `share/resources/`, `qt_frontend/assets/`, `packaging/assets/`.
- Ignore: `CMakeFiles/`, `CMakeCache.txt`, `node_modules/`, `testlogs*/`, `.vs`, `test-seamly-layout/`.
- Untracked (files kept on disk): `test-seamly-layout/`, `qt_frontend/node_modules/`, `src/test/Seamly2DTest/testlogs_check/`.
- Open: `scripts/2026-11-10-prompt.txt` is now visible to git (`!scripts/*` beats `*.txt`); not committed.

### 2026-11-11 — Reference lines icon restored

- Cause: MSI build stopped in vmisc: `icon.qrc` lists `icon/svg/reference_lines.svg`; the file was never committed.
- `.gitignore` rule `Icon` (macOS) matched every `icon/` folder because `core.ignorecase` is true; `git add` skipped new icons.
- `.gitignore`: `!icon/` keeps the resource folders tracked.
- `reference_lines.svg` recreated: stripe and plaid symbols; edges on whole pixels.
- Temporary test program: the 8 pixel checks of `tst_piecemarkings.cpp` pass. Full Qt suites run with the next MSI.

### 2026-11-11 — Split paper roll keeps media

- Split on a paper roll keeps the selected roll width and margins; only the length follows the content.
- Code unchanged: an individual roll split packs on fixed `bin_w` columns and never widens.
- New tests: `placement_paper_roll_split_keeps_media_dimensions` (margins, extend off and on),
  `trousers_split_on_a_narrow_paper_roll_keep_roll_width` (16 in roll, Back divides).
- Docs: PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW_DESCRIPTION.
- Installed-app check runs with the next MSI.

### 2026-11-11 — Custom roll width

- Roll Width lists 'custom' last. It shows a width field in the Layout Settings unit; a predefined size hides it.
- `setRollSize("custom")` keeps rollWidth; `setRollWidth` ignores zero, negative, and non-finite values.
- QML: `isRollPaper`, `isCustomRoll`, `customRollWidthField`; `refreshCustomPaperFields` also copies rollWidth.
- Rust unchanged: the layout core reads `rollWidth`.
- SettingsModel tests (temporary test program): 44 pass. Installed-app check runs with the next MSI.

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

### 2026-11-10 — Reference lines icon

- Pattern Piece dialog: Reference lines page shows stripe and plaid icon (`reference_lines.svg`).
- Temporary render check: 8/8 pixels pass. `TST_PieceMarkings` runs in the next MSI build.

### 2026-10-28 — Fabric media size fix; merged into local `run-seamlyLayout`, not pushed

- Cause: Sue's trousers on fabric (folded, 36 in, selvedge 0.5 in) left 4 pieces out. The fabric canvas used the B0 paper sheet size that the settings keep from paper media; fabric width, fold and open length were ignored.
- Fix: `LayoutSettings::media_size_in()` gives the canvas (`page_w_px`, `page_h_px`) and the bin (`effective_bin_px`). Fabric: `fabricWidth` (0 = `pageWidth`; folded: half), `fabricHeight` (0 = 500 in, trimmed). Paper ignores the fabric fields.
- Commit `01562ef927`. Changed: `layout_settings.rs`, `layout_utils.rs`, `material_split.rs`, `size_labels.rs` (fabric fixtures use `fabricWidth`/`fabricHeight`), the layout and settings workflow docs.
- Verified: `cargo test --workspace` 636 passed; `ctest --preset debug` 7 of 7. Not run: MSI, GUI.
- Sue's popup had the old title, so her MSI predates `3b1310115e`.
- CLAUDE.md vcvars path: VS 2022 → VS 18 (user decision: local builds use VS 18; `local_build_msi.ps1` already does).
- Next: MSI build (ask). User test: trousers on 36 in folded fabric place all 12 pieces inside the fold half.
- `run-seamlyLayout` is 8 commits ahead of `origin`.

Older entries: `project-docs/SESSION_HANDOVER_ARCHIVE.md`. Do not Read it at session start.

## TODO by priority and owner

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
- Verified: `cl /Zs` of `mainwindowsnogui.cpp`, `mainwindow.cpp`, `handoff_corner_cuts.cpp`; temporary test program `TST_HandoffCornerCuts` 4 passed. Not run: MSI, Qt suites (`TST_HandoffPiece`, `TST_HandoffDocument`), GUI.
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

### 2026-10-26 — `develop` merge (7 upstream commits) committed as `ddf3e6256f`; not built, not pushed

- Sue asked to commit before the MSI build. `run-seamlyLayout` is 9 commits ahead of `origin`.
- Pattern schema: kept ours. Upstream `v0.7.6.xsd` is a copy of v0.7.5 (version bump for true dart lines); our v0.7.6 is a superset; v0.7.7 stays current.
- `SEAMLY_DATA_DIR` kept and adapted: common ini, app ini (`MigrateSeamlySettingsLocation`), data root and logs go under the directory; installer record skipped. `getDefaultDataRoot()` replaces upstream `defaultDataPath()`.
- `automationDataDirectory` test now expects `settings/Seamly/...` paths. SeamlyMe logs also go to `<dir>/logs/seamlyme` (ours log; upstream SeamlyMe did not); test checks it.
- Qt 6.11.3 (user decision): `QT_VERSION`, CMake and build-script minimums, docs bumped in the merge. Historical lines stay 6.11.1.
- Qt 6.11.3 kit installed. `qt_notices.txt` regenerated and in the merge commit.
- `c27faa9d29`: `Cargo.lock` synced with `lopdf = "0.45"` (dependabot #31 left it at 0.42.0); `rust_crate_notices.txt` regenerated; fingerprint matches. `cargo check --workspace --all-targets` passes.
- `ci.yml`: Python 3.14 pin and `setup-python: 'false'` from upstream; no forced x64 Python (our arm64 job runs on a native runner); MSVC setup after the Qt install.
- `.github/README-AUTOMATION.md`: example uses `--test` (no console export), settings paths fixed.
- Verified: `cl /Zs` on changed `.cpp`. Not run: Qt suites, `cargo test`, MSI (ask first).
- Next: MSI build (ask). Then Sue pushes (no skip-ci token: `ci.yml` changed) and runs `gh workflow run ci.yml --ref run-seamlyLayout`.

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
- Verified: `cl /Zs` on every changed `.cpp` and their includers. Temporary test program (session scratchpad `223c6375-…/harness`): `TST_AbstractPiece` 58 passed, `TST_LabelTextManager` 7 passed; old/new differential, 30 000 random paths and 200 000 random corners, label fits: no output difference. Not run: Qt suites, MSI (ask first), Linux link (CI), GUI.
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
- Verified: temporary test program `TST_VPiece` 128, `TST_SvgComponentTags` 49 passed; `cargo test --workspace` 621 passed; `cl /Zs` clean. Not run: Qt suites (next MSI, ask first), ctest (no QML or C++ frontend change), GUI.
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

### 2026-10-24 — Seamly2D.11 built-in seam allowance; merged into local `run-seamlyLayout` (c18f080c0e), not pushed

- Piece mode draws the built-in seam line (seam line style) inside the main path (cut line style); hatch between; Hide Seam Line hides it. Dialog preview draws it too.
- Fold line: the seam line stops on the fold line at both ends, open, not along it (`VPiece::builtInSeamLinePoints`). Handoff `seamline` is an open path; SeamlyLayout DXF-ASTM keeps it open (no Rust code change).
- Decisions to confirm: Automatic tab (widths) stays enabled for Built in, Custom tab and Angle disabled; View > Seam Allowance off keeps both lines.
- User (2026-10-24): `src/libs/vlayout` is deprecated and goes at the SeamlyLayout release; the layout algorithm is Rust. Edits there were limited to the handoff path (`VLayoutPiece` open seam line) and the DXF-AAMA layer 8 fix in `vdxf`.
- Verified: temporary test program (HEAD test objects + in-tree libs, changed sources rebuilt) passed every Seamly2D suite; `cargo test -p seamly_svg2ezdxf` 68 passed; `cl /Zs` clean. Not run: MSI (ask first), GUI.
- Open: Seamly2D.11.8 (Qt suites in the next MSI), Seamly2D.11.9 user check `AGENT_CHECKLIST.md` §27.
- Scratch tools (session scratchpad `3348cfb9-…`): `harness/make_build.py` builds the full Seamly2DTests exe from main tree objects plus worktree sources; `make_zs.py`.
- Uncommitted user files left untouched: both `jacket2_40-146.sm2d` copies, the `TODO_SEAMLYLAYOUT.md` reorder, `test-seamly-layout/foldline_arrowline.*`, `test-seamly-layout/layouts/jacket2/2026102*/`. `TODO_SEAMLY2D.md` user edits are committed (they hold the Seamly2D.11 text).

### 2026-10-23 (evening) — Layout.95–98, 92, 93.4–93.6, 94, 27.13.4; merged into local `run-seamlyLayout`, not pushed

- Pattern format 0.7.7 (`v0.7.7.xsd`, `toVersion0_7_7`): `rotationWay` and `tilt` enumerations, new `tiltDegrees`, piece `flip`; no `foldPosition`, no `forbidFlipping`. Decision: a new version, since builds already wrote 0.7.6 files.
- Piece Properties: radio buttons for Rotation, Tilt (+ Degrees 0–90), Flip; "Fold: On fold" check box only. Piece menu "Flip" submenu; the F key and the Preferences "Forbid flipping" default are gone.
- Converted pieces get flip None (decision: old `forbidFlipping="false"` was a default, not a choice).
- Layout.92: Create Layout lays out every material; material tabs over both canvases show the same material; a switch keeps each material's layouts (`MaterialLayout`).
- Layout.93.4–93.6: handoff `data-rotation`, `data-tilt`, `data-flip`, `data-orientation`, `data-annotation`; DXF-ASTM `Rotation:`, `Flip:`, `Tilt:`, `Fold:` after `Quantity:`. Decisions: Flip Both = `XY`; Tilt signed (`-n` clockwise); every tagged piece writes `Fold:Y/N`; DXF-AAMA writes none.
- Layout.94: `annotation_alignment.rs` moves each size's grainline and labels to the base size's offset from the outline centre; `center_piece_sets` centres on the outline. jacket2: Back's pattern label leaves sizes 220–280 (import warning).
- Layout.90/91: research only, `dxf-docs/seamlylayout_DXF_PAPER_AND_MODEL_SPACE.md`. User decision needed (P1/P2, M1/M2/M3).
- Layout.27.13.4: MSI 26.10.2023 capture is now `jacket2_40-146_handoff.svg`; 7 of 8 graded, Detail 4 not. Seamly2D fix `snapNodeVertices` for node vertices that `RemoveDublicates` erased (node counts differ by size). Two Detail 4 geometry differences stay open (27.13.6).
- Verified: `cargo test --workspace` 611 passed; `ctest --preset debug` 6 of 6; temporary test program 21 passed (new `TST_VPiece` cases); `cl /Zs` on every changed `.cpp`; 8 samples and 13 CollectionTest patterns convert to 0.7.7 and validate. Not run: Qt suites, MSI (ask first), GUI.
- User checks: `AGENT_CHECKLIST.md` §23 (extended), §24 (Rotation, Tilt, Fold, Flip), §25 (material tabs), §26 (grainline and label places); Layout.27.13.6.2 needs a new jacket2 capture.
- Scratch tools in the session scratchpad: `harness/` (QtTest harness for new `TST_VPiece` tests), `uirender/` (renders a `.ui` to PNG), `make_zs.py` (`cl /Zs`), `cargo.cmd`, `ctest_debug.cmd`.
- Uncommitted user files left untouched: both `jacket2_40-146.sm2d` copies, `test-seamly-layout/layouts/jacket2/2026102*/`.

### 2026-10-23 — DXF piece data from Piece Properties, merged into local `run-seamlyLayout`, not pushed

- Rule (user decision): Piece Properties are the source of truth; a label only displays them. A hidden label or grainline drops no piece data.
- Layout.93.1: `Material:` from the piece `data-material`, else `Fabric` (`converter::material_from_piece`).
- Layout.93.3: `Quantity:` from `data-quantity` only; the "Cut N" label fallback is gone. `MissingPieceData` has no label flag.
- Layout.93.2: no code. A hidden grainline is no grainline: no layer 7 line, and Missing Piece Data still names it.
- User decisions: aw.fyi layer 5 caption is a viewer note; layer 15 holds shown labels only. Rotation, Tilt, Fold, Flip values: Layout.95–98.
- Verified: `cargo test --workspace` 599 passed. Not run: ctest (QML comment-only change), GUI, MSI (ask first).
- User check: Layout.93.7, `AGENT_CHECKLIST.md` §23.
- Open: Layout.93.4–93.6 (handoff attributes and DXF text for Rotation, Tilt, Fold, Flip) wait on Layout.95–98.
- Uncommitted user files left untouched: both `jacket2_40-146.sm2d` copies, the `TODO_SEAMLYLAYOUT.md` task reorder, `test-seamly-layout/layouts/jacket2/2026102*/`.

### 2026-10-22 — Fold lines on the left edge for paper too, merged into local `run-seamlyLayout`, not pushed

- User decision: the left margin is 0 on fabric, not on paper. The setting now applies to every media and sits under "Add scale check grid"; it no longer needs Fabric Folded.
- Paper keeps its left margin; the fold line lies on the left content edge. The paper-sheet PDF export pins the same pieces.
- Verified: `cargo test --workspace` 597 passed; `qmllint` no new kind of warning. Not run: ctest, GUI, MSI (ask first).
- User check: `AGENT_CHECKLIST.md` §21, now with paper steps.

### 2026-10-22 — DXF-AAMA (R11) export, merged into local `run-seamlyLayout`, not pushed

- Layout.85: Export > DXF > "DXF-AAMA (R11)". ANSI/AAMA-292 content in a DXF R11 (AC1009) file; reference `dxf-docs/seamlylayout_DXF_AAMA.md`.
- User decision: ANSI/AAMA-292 compliance, on R11 DXF (AC1009). The first merge wrote R10 (AC1006); `DxfVersion::R10` is now `R11`, and the R11 file equals the R12 file.
- One writer: `DxfAstmExportOptions::aama` switches `write_astm_block`. No layers 15, 80–87; dense contours on 1, 8, 11, 14; labels on layer 1 without piece text lines; `Size:`; `.rul` starts `ANSI/AAMA VERSION:1.0.0`.
- The fold line mirror line (layer 6) applies to AAMA too; `start_on_mirror_line` drops dense fold vertices so the boundary starts on it.
- Submenu note now "DXF: select Base Size or a size tab"; `AGENT_CHECKLIST.md` §3 and §18 updated.
- Verified: `cargo test --workspace` 596 passed; ezdxf 1.4.4 `recover` 0 errors on AAMA output; scratch render of `ExportMenu.qml`; `qmllint` no new warnings. Not run: ctest, GUI, MSI (ask first).
- User check: Layout.85.5, `AGENT_CHECKLIST.md` §22, including an import in an AAMA reader.
- Open: an untagged SVG keeps layer 15 text.
- Work done in worktree `C:\Users\susan\Projects\s2d-aama` (removed after the merge).
- Merge commit uses the skip-ci token: no path in the full-CI list changed.

### 2026-10-21 — Fold lines in SeamlyLayout, merged into local `run-seamlyLayout`, not pushed

- Layout.80.6: `fold_line` is read as a component; a split piece clips it per part.
- New setting "Place fold lines on the left edge (the fold)", Fabric section, needs Fabric Folded. Left margin 0 on the fold.
- `fold_line_edge.rs`: angle 0/180 brings the fold line left; with nap the piece is mirrored instead. `Rect::with_left_edge` pins it in MaxRects. Nested piece-sets are not pinned.
- DXF-ASTM: layer 6 mirror line; the boundary starts on it (`astm_mirror.rs`, after grading). Fold line off the cut line: no layer 6, export warning.
- Verified: `cargo test --workspace` 588 passed; `cl /Zs` on `SettingsModel.cpp`, `SettingsModelTests.cpp`; `qmllint` no new kind of warning. Not run: ctest, GUI, MSI (ask first).
- User checks: Layout.80.7 and 80.3.1 (`AGENT_CHECKLIST.md` §20), Layout.80.6.6 and 80.6.3 (§21).
- Open: a mirrored piece does not grade in a Base Size DXF-ASTM export; fold warnings on multisize layouts go to the log only.
- Merge commit uses the skip-ci token: no path in the full-CI list changed.

### 2026-10-21 — Nested exports keep the smallest size's grainline and labels, merged into local `run-seamlyLayout`, not pushed

- Layout.84: every export but DXF-ASTM keeps, per piece-set, the `grainline`, `piece_label`, `pattern_label` and `size_label` groups of the smallest size only.
- Code: `nested_annotations.rs`; `clone_export_layout_doc` (all non-DXF exports); `sheets::build_sheet_export_inputs` (paper-sheet PDF, after the piece extraction).
- SVG: the kept groups move into `<g id="labels_and_grainlines">`, in place of the stripped `Rectangles` group: just before `size_720`. Placement is a reading of "just above the content rect"; the user may change it.
- Verified: `cargo test --workspace` 568 passed. Not run: ctest, GUI, MSI (ask first).
- User check: Layout.84.4, `AGENT_CHECKLIST.md` §19.
- `8dd416dd26` ("updated docs") left the Layout.83 subtasks under the Layout.87 heading and dropped the Layout.86 and Layout.87 subtasks. Not changed here.

### 2026-10-21 — Export > DXF submenu, merged into local `run-seamlyLayout`, not pushed

- Layout.86: `ExportMenu.qml` `dxfSubmenu` holds DXF-ASTM (R12), (R13), (CLO3D). The View menu keeps one DXF-ASTM item.
- Marker and Nested: the submenu stays enabled so its note can be read; the three items are disabled.
- Verified: scratch Fusion render of `ExportMenu.qml` (order, note, signals); `qmllint` no warnings. Not run: ctest, GUI, MSI (ask first).
- User check: Layout.86.3, `AGENT_CHECKLIST.md` §3 and §18. Open checklist steps now read Export > DXF > DXF-ASTM.
- Layout.87 was already merged (6641162d58) when it was requested again; no new work.
- Merge commit has no skip-ci token: the unpushed `packaging/**` change needs full CI.

### 2026-10-21 — PDF (Tiled) disabled on the Marker tab, merged into local `run-seamlyLayout`, not pushed

- Layout.87: tiling the Marker tab locks SeamlyLayout up (user report). Cause not investigated.
- `ViewKind::exports_pdf_tiled` false for Marker → `AppController.pdfTiledExportAllowed` → `ExportMenu.pdfTiledTabAllowed`. Item text: "PDF (Tiled): select a tab other than Marker".
- Export all tabs: PDF (Tiled) stays enabled and skips Marker. `export_pdf_tiled` refuses the Marker tab.
- Verified: `cargo test -p cxxqt_bridge` 226 passed; `qmllint` no new warnings. Not run: ctest, GUI, MSI (ask first).
- User check: Layout.87.3, `AGENT_CHECKLIST.md` §4 and §17.
- Merge commit has no skip-ci token: the unpushed `packaging/**` change needs full CI.

### 2026-10-21 — Export menu fits its widest item, merged into local `run-seamlyLayout`, not pushed

- Layout.88: a Qt Quick `Menu` keeps 200 px and elides longer items. The DXF-ASTM note is 220 px.
- `ExportMenu.qml` `fittedWidth`: the Export menu and its SVG submenu take the width of their widest shown item.
- Verified: Fusion render of `ExportMenu.qml`, no elided text; `qmllint` no warnings. Not run: ctest, GUI, MSI (ask first).
- User check: Layout.88.2, `AGENT_CHECKLIST.md` §3.
- Peer `seamly2d-private-9e` changes `ExportMenu.qml` for Layout.87 in its own worktree; it merges after this one.
- Merge commit has no skip-ci token: the unpushed `packaging/**` change needs full CI.

### 2026-10-21 — Oversized-pieces dialog text colors, merged into local `run-seamlyLayout`, not pushed

- Layout.89: the screenshot with black radio labels is a scratch `qml` render, not SeamlyLayout. The app already draws them in `Theme.textOnDark`.
- `Main.qml` `oversizedPiecesLabel`: piece list `Theme.violetLight` → `Theme.textOnDark`. Contrast on the dialog violet 2.1:1 → 7.9:1.
- Verified: Fusion render of the dialog from `Main.qml`; `qmllint` no new warnings. Not run: ctest, GUI, MSI (ask first).
- User check: Layout.89.3, `AGENT_CHECKLIST.md` §12.
- Merge commit has no skip-ci token: the unpushed `packaging/**` change needs full CI.

### 2026-10-21 — Oversized-pieces dialog names the sizes, merged into local `run-seamlyLayout`, not pushed

- Layout.72.9: each dialog line names the dimension that does not fit, with piece and media sizes in the settings unit. Example: `• Back: length 43.10 in, media 35.50 in`.
- `OversizedPiece` carries the piece box and the media content size; `oversized_pieces_message` takes the unit.
- `Main.qml` `oversizedPiecesDialog`: plain-text piece list in a `ScrollView`, at most 40% of the window high.
- Verified: `cargo test --workspace` 561 passed; `qmllint` no new warnings; scratch `qml` render with 30 pieces keeps the buttons in the window. Not run: ctest, GUI, MSI (ask first).
- Layout.72.1–72.7 were already done (2026-10-13). Open: user check Layout.72.8.
- Not done: the split puts a whole `fold_line` in one part (Layout.80.6.1).
- Merge commit has no skip-ci token: the unpushed `packaging/**` change needs full CI.

### 2026-10-21 — Nested size colors: dark and pastel palette, merged into local `run-seamlyLayout`, not pushed

- Layout.82: `size_colors::palette_colors` replaces the golden-angle palette. OKLab farthest-point picks from dark to pastel candidates; first size blue.
- jacket2, 26 sizes: closest pair 0.11 in OKLab (before: 0.012); 7 pastel, 5 dark.
- Verified: `cargo test --workspace` 561 passed. Not run: ctest (no C++/QML change), GUI, MSI (ask first).
- User check: `AGENT_CHECKLIST.md` §16, next MSI.

### 2026-10-21 — Nested legend square swatches, merged into local `run-seamlyLayout`, not pushed

- `size_colors.rs` `SizeLegend::element`: each swatch is a 32 × 32 px `rect` in the size color, no stroke. Was a 32 px line.
- Row height 24 → 40 px (swatch plus one pad); the legend is taller.
- Verified: `cargo test --workspace` 561 passed. Not run: ctest (no C++/QML change), MSI (ask first).
- User check: Layout.83.4, `AGENT_CHECKLIST.md` §15.
- Peer session `seamly2d-private-00` works on Layout.82 (palette) in the same file; asked to use checklist §16.

### 2026-10-21 — MSI 26.10.2023 test results recorded, committed on local `run-seamlyLayout`, not pushed

- User passed: Layout.38.6, 67.8, 25.7, 24.7, 26.7, 27.3.3, 69.3, 73.5, 75.2, PDF true size, Seamly2D.12.8.
- Moved to `TODO_COMPLETED.md`: Layout.3, 24, 25, 26, 67, 69, 73, 75; Seamly2D.12.
- Marked done in place: Layout.27.3, 27.12.
- aw.fyi Fabric Base Size 500 R13: 7 of 7 graded, 0 departures. Open: Lining (Detail 4) R13, R12.
- jacket2 handoff: Fabric 7 pieces, Lining 1 (Detail 4).
- Filed: Layout.88, Export menu DXF-ASTM note runs past the menu edge.
- Next: replace `jacket2_40-146_handoff.svg` with `C:\Users\susan\seamly2d\layouts\handoff_jacket2.svg` (Layout.27.13.4, 27.7.2).
- Still open for the user: Layout.28.7, 70.5, 71.6, 72.8 (`AGENT_CHECKLIST.md` §11–§13).

### 2026-10-21 — `local_build_msi.ps1` stray `build` file fixed, merged into local `run-seamlyLayout`, not pushed

- Defect: the batch line `echo ... wix --> build MSI` redirected to a file `src/app/seamlylayout/build` on every build.
- Fix: echo text without `>`; a `rem` line states the rule.
- Verified: scratch cmd run, old line writes `build`, new line writes no file. Not run: full build.
- Full CI rule: `packaging/**` changed; the merge commit has no skip-ci token.
- MSI 26.10.2023 (packer fix) built and passed all tests; user check of the Lining Marker is open.

### 2026-10-20 — MaxRects piece-order search, merged into local `run-seamlyLayout`, not pushed

- Defect: jacket2 Lining Marker, `s220` and `s260` 1,014 px apart in one band. The topmost-box rule put `s260` 2 px higher and 1,732 px to the right.
- `layout_engine`: `maxrects_order_search.rs`, `up_left_compaction.rs`. Rules `TopLeft` and `WeightedTopLeft` (lowest `8 × y + x`). Five start orders × two rules, then late-acceptance hill climbing, fixed seed.
- Rank: unplaced area, height, side-gap excess, moment. Side by side = y ranges share at least half the lower piece height.
- Gap now on all four sides; each pass ends with up-left compaction.
- Strict `pack_maxrects*` (sheets, tiled scan, oversized): start orders only. Lenient and extending: full search, about 1 s at most.
- Progress status: "Packing <n> pieces... <p>%".
- Removed from the earlier uncommitted draft: unused `snap_to_exact_gap`, `rotated_exact_dims`.
- Verified: `cargo test --workspace` 559 passed. Temporary test program with the sample layouts' piece sizes: Marker 16,944 → 14,840 px, side-gap excess 1,009 → 0 px; 208-piece jacket2 74,605 → 73,165 px. Not run: ctest (no C++/QML change), GUI, MSI (ask first).
- Not done (user decision): 1/100 px packing. Boxes round up to whole px, so a gap on the media is 5–6 px; 1 px is within tolerance.
- User check: next MSI, Marker of the jacket2 Lining handoff.

### 2026-10-20 — Canvas split 25 / 75, merged into local `run-seamlyLayout`, not pushed

- `Main.qml` `canvasRow`: both columns set `Layout.preferredWidth`. Left share = 0.25 always, with or without a layout (user decision).
- Cause: `RowLayout` splits spare width in proportion to implicit widths. The material tabs gave the left column ~99% (1387 of 1398 px).
- Verified: scratch `qml.exe` run of the row — old 1387 / 11 px, new 350 / 1048 px at 1400 px. `qmllint` shows no new warnings. Not run: GUI.
- User check: next MSI, import a Fabric + Lining handoff, create a layout.

### 2026-10-20 — Material tabs 50% taller and wider, merged into local `run-seamlyLayout`, not pushed

- `Main.qml` `materialTab`: `implicitWidth` and `implicitHeight` = 1.5 × the Fusion size. Font unchanged. Layout view tabs unchanged.
- Verified: `qmllint` shows no new warnings. Not run: GUI. User check: next MSI, import a Fabric + Lining handoff.

### 2026-10-20 — `local_build_msi.ps1` runs the SeamlyLayout tests, merged into local `run-seamlyLayout`, not pushed

- After the SeamlyLayout release build: `ctest --preset release` (new test preset in `CMakePresets.json`), then `cargo test --workspace`. `-SkipTests` skips both.
- The script finds cargo on PATH or in `%USERPROFILE%\.cargo\bin`; sets `QT_FORCE_STDERR_LOGGING=1` for ctest, so a failed suite prints its QtTest output.
- Release tests found a defect: `StartupOptionsTests::makeFile` opened its file inside `Q_ASSERT`; 5 tests failed. Fixed with `qFatal`. Header changed to MIT (qt-style rule).
- Verified: `ctest --preset release` on the last release build ran 6 suites, StartupOptionsTests failed (old binary); fixed test passed in a release-mode temporary test program (28 passed, 1 skipped); `cargo test --workspace` under vcvars 549 passed; batch text rendered for both modes. Not run: full `local_build_msi.ps1` (ask first).
- Full CI rule: `packaging/**` changed; the merge commit has no skip-ci token.

### 2026-10-19 — Multisize height selection rolled back, merged into local `run-seamlyLayout`, not pushed

- User decision: height selection is deferred. Layout.79.1 and 79.2 reopened; the dialog is removed.
- Handoff: every size at the base height; `data-height` = `base` of the `.smms` `height` measurement (1760 for `gost_man_ru.smms`). No `height` measurement → `<height base>`.
- Kept: `data-height`, SeamlyLayout "Height <height>" header (Layout.79.3).
- Removed: `MainWindow::selectMultisizeLayoutHeight`, `MeasurementDoc::bodyHeights`, `TST_Measurements::MultisizeBodyHeights`. To resume: commits `1ef11b80e5`, `21b6cc1591` (in the TODO).
- Verified: `cl /Zs` on `mainwindow.cpp`. Not run: Qt suites, GUI. User check: Layout.79.4.
- User's uncommitted edits (Layout.81 in `TODO_SEAMLYLAYOUT.md`, version files, plists, jacket2 sample) left uncommitted.

### 2026-10-19 — Piece Mirror fix: any draft block, merged into local `run-seamlyLayout`, not pushed

- Defect: Piece mode sets the last draft block active (`MainWindow::showPieceMode`). The tool refused a piece of another block; status bar "Select a piece of the active draft block", no dialog, Enter did nothing.
- Reproduced on the installed MSI (`jacket2_40-146.sm2d`, Detail 1 in block 1 of 3). A one-block copy mirrored correctly, so the rest of the pipeline works.
- Fix: `PieceMirrorTool::Create` uses `getToolDraftBlockName(piece_id)`, makes that block active while it builds, then restores the active block. `PieceMirrorDialog` no longer checks the block. `VAbstractTool::PrepareNode` takes an optional `blockName`, so node undo commands do not follow the active block.
- Verified: `cl /Zs` on the changed `.cpp` files. Not run: GUI with the fix, Qt suites. Next MSI: user check Layout.80.3.1 (now includes a multi-block pattern).
- Rule (user decision): the active draft block affects Draft mode only; Piece mode works on the chosen piece and its block. Follow-up: `PieceMirrorTool` finds the block from the piece element (`pieceBlockName`), not the history. `VAbstractTool::AddRecord` takes an optional `blockName`; piece, union and node tools pass theirs.
- Open (user decision): an undo in Piece mode still switches the Draft Block combo to the operation block (`AddToCalc::undo` → `changeDraftBlockGlobally`). The combo still shows the last block, disabled, in Piece mode.

### 2026-10-18 (afternoon) — Layout.80.3 Piece Mirror tool merged into local `run-seamlyLayout`, not pushed

- Branch `task-piece-mirror`, commits made with a temporary index (shared tree).
- Piece mode > Details > Piece Mirror (M, P). `PieceMirrorDialog` takes a piece of the active draft block with a valid fold line; option "Take the half piece out of the layout" (checked).
- `PieceMirrorTool`: Mirror by Line operation at the end of the draft block for every half object off the fold line; new ordinary piece `<name> Mirrored`. No new file element; format stays 0.7.6.
- `PieceMirrorPath` (vpatterndb): full path order, mirrored node attributes, custom seam allowance ranges; the path start rotates so no record runs past the last node.
- `Tool::PieceMirror` added; tool-count checks 54 → 55. `VAbstractTool::PrepareNode(s)` now public; `VAbstractOperation::getDestination()` added.
- Also fixed: `v0.7.6.xsd` `unionPiece` nodes allow `foldLine`.
- Agent decisions (in the TODO): draft operation, not a piece tool element; half piece kept; record across the fold dropped; full piece 2 cm right of the half.
- Verified: 14 piece mirror test cases passed in a temporary test program (current vpatterndb sources linked with the 2026-10-17 in-tree libs); a mutation (no curve reverse) failed 2 of them. `cl /Zs` on every changed `.cpp`; uic, moc, rcc on the new files. Not run: Qt suites, GUI. User check: Layout.80.3.1.
- Full CI rule: `.pri` files changed; the merge commit has no skip-ci token.
- `.gitignore` (user's staged edit) ignores `*.png` and `*.json`: the icons were added with `-f`; `project-docs/AGENT_STATE.json` is untracked.
- User edited the Layout.80.3 line during the session; committed with the task.

### 2026-10-18 — Layout.80 fold lines merged into local `run-seamlyLayout`, not pushed

- Branch `task-fold-lines`, commits made with a temporary index (shared tree).
- Fold line = two neighbouring main path point nodes with `foldLine="true"` (format 0.7.6, no new version). `VPieceNode::isFoldLine`; `VPiece::foldLineNodeIndexes`, `foldLine`, `hasFoldLine`.
- Piece dialog: point node menu "Fold Line"; invalid choice blocks OK; valid fold line checks and locks "On fold".
- Seam allowance width 0 along the fold line. Canvas: `PatternPieceTool::m_foldLine` in the fold line preference style (Preferences > Pattern > Fold Lines).
- Handoff: `fold_line` component group, two point path; `data-on-fold` follows. Docs: `SVG-DATA-ATTRIBUTES.md`, `NEW-ATTRIBUTES.csv`.
- Agent decisions (in the TODO): node attribute, not piece-level ids; outline stays closed, fold line drawn over it.
- Not done: Layout.80.3 Piece Mirror tool; built-in seam allowance still insets the seam line on the fold; SeamlyLayout side (Layout.16).
- Verified: `cl /Zs` on every changed `.cpp` and both test files. Not run: Qt suites (5 new tests), next `local_build_msi.ps1` (ask first). User check: Layout.80.7.
- User's uncommitted `TODO_SEAMLYLAYOUT.md` restructure committed with the task; `TODO_SEAMLY2D.md` and other user edits left uncommitted.

### 2026-10-17 (evening) — Layout.77, 68 grid, 79 merged into local `run-seamlyLayout`, not pushed

- User rewrote Layout.77 (piece material in Seamly2D), added Layout.79 (select height) and Layout.68.5–68.13 (scale grid) in a copy, `TODO_SEAMLYLAYOUT_updated.md`; merged into `TODO_SEAMLYLAYOUT.md`. The copy is untracked and left in place.
- Layout.77: pattern format 0.7.6 (`v0.7.6.xsd`, piece attribute `material`); `VAbstractPiece::getMaterial`; Material list in the piece dialog after Letter; handoff has one pattern group per material (`data-material`); SeamlyLayout `material_split.rs`, material tabs over the left canvas, "Material: <m>" over the right canvas, `_<material>` in export names with several materials.
- Agent decisions (recorded in the TODO): one stdin document with one pattern group per material; a material tab switch drops the shown layout and rebuilds the blank canvas from the submitted settings.
- Layout.68 grid: `scaleGrid` setting; `scale_grid.rs`; not on Marker; removed from DXF; HPGL cut mode leaves it out. Grid size follows the SeamlyLayout settings unit.
- Layout.79: `MainWindow::selectMultisizeLayoutHeight`; `data-height`; "Height <h>" over the left canvas.
- CLAUDE.md "Seamly2D Handoff Contract" still says one pattern group; not edited (the user's CLAUDE.md edit is uncommitted).
- Verified: `cargo test --workspace` 549 passed; `ctest --preset debug` 6 of 6; `cl /Zs` on every changed Seamly2D `.cpp`. Not run: Qt suites (new tests in `TST_SvgComponentTags`, `SettingsModelTests` ran in ctest). Next: MSI build (ask first), then user checks 74.4, 75.2, 76.2, 77.2, 78.2, 79.3–79.5, 68.10–68.13.

### 2026-10-17 (afternoon) — Layout.75, 78, 74, 76 merged into local `run-seamlyLayout`, not pushed

- Layout.75: `Main.qml` tab strip in a `Flickable`; scroll bar below the tabs; mouse wheel scrolls; shown tab scrolls into view.
- Layout.78 (renumbered from the second Layout.75): `size_groups.rs`, Nested SVG export has one `<g data-type="size">` per size, largest first, flat.
- Layout.74: `size_colors.rs`, Nested pieces colored by size; `size_legend` group below the lowest piece; media grows for it; paper-sheet PDF keeps colors, no legend.
- Layout.76: `size_labels.rs`, "Size <size>" label 5 px past the grainline front end, added in `finish_import`; no grainline → outline centre (agent decision, recorded in the TODO).
- TODO renumbering: second Layout.75 → Layout.78; Layout.76 sub-items 76.1/76.2.
- User's uncommitted TODO restructure (new tasks 74–77) committed with these tasks.
- New CLAUDE.md "Contributing" section (uncommitted, user's): many commits per task; no AI mention in `.md` files either.
- Verified: `cargo test --workspace` 539 passed; `ctest --preset debug` 6 of 6; qml test render of the tab strip; jacket2 Nested PNG render. No MSI.
- Open: Layout.77 needs a user decision (piece fabric type source in Seamly2D). User checks on next MSI: 74.4, 75.2, 76.2, 78.2.

### 2026-10-17 — Seamly2D.12 SeamlyLayout opens behind Seamly2D: merged into local `run-seamlyLayout`, not pushed

- User report: jacket2, Layout, "SeamlyLayout never opens". Found: it opened behind Seamly2D after Seamly2D did not respond for about 26 s; the user pressed Layout again and got two windows.
- Cause of the 26 s: visible piece labels. Pattern copies on installed 26.10.1615: 4 s without labels, 20 s with. cdb stack samples: `FitFontSize` loads a font engine per pixel size.
- User decisions: do window-in-front, progress dialog with Cancel, label speed fix; one SeamlyLayout per Seamly2D.
- Code: `core/windows_foreground_window.{h,cpp}` (win32 only, `core.pri`); `MainWindow::prepareMultisizePieceLists` returns false on Cancel; `isSeamlyLayoutRunning`, `raiseSeamlyLayout`; `VTextManager::largestFittingSize` and a per-thread fit cache; `seamlyLayout_main.mm` raises its window.
- Verified: `cl /Zs` on 6 Seamly2D sources and `seamlyLayout_main.mm`; benchmark: new search gives the same size in 400 random cases. Not run: Qt suites, new `TST_VTextManager` (next MSI; ask first). No Rust change.
- Full CI rule: change touches `*.pro`/`*.pri` and `#if defined(Q_OS_WIN)`; merge commit has no skip-ci token.
- First MSI build failed: LNK2019 for 8 `user32` functions. Fix `4a50ccb2a0`: `core.pri` adds `LIBS += -luser32` (x64 and arm64). Pushed.
- MSI 26.10.1709 built: Qt suites passed (`nmake check`), MSI checks passed.
- Next: user installs the MSI and runs `AGENT_CHECKLIST.md` section 14.

### 2026-10-16 — Layout.73 unique piece names at import: merged into local `run-seamlyLayout`, not pushed

- User request: unique piece names before a layout. Individual `Piece_<n>[_<name>][_<letter>]`; multisize `Size_<size>_Piece_<n>[_<name>][_<letter>]`.
- User decision: each run of whitespace in the name becomes one `_`; multisize names start with `Size_`.
- Code: `cxxqt_bridge/src/piece_naming.rs`, called by `finish_import`. Piece-sets get the name without the size. Marker `data-name-assigned="true"`; a later import keeps the name. No "(size N)" suffix on an assigned name. Warning when sizes differ in piece count.
- Finding: the user read "no piece label" as a name problem. jacket2 hides piece labels and grainlines (`visible="false"`), so the dialog stays. User told.
- `C:\Users\susan\seamly2d\patterns\jacket2_40-146.sm2d` changed on 2026-10-16 10:49: pieces now "Detail 1"–"Detail 4", "Sleeve 1", "Sleeve 2", letters A–H. The next capture has other names than the fixture.
- Verified: `cargo test --workspace` 519 passed; `ctest --preset debug` 6 of 6. Not run: MSI (ask first).
- `AGENT_CHECKLIST.md`: only the Layout.73 lines committed; the user's "PASS" edit stays uncommitted.
- Next: MSI build, then Layout.73.5 and the next steps of the entries below.

### 2026-10-16 — Qt suite failure in `TST_VPiece` fixed: merged into local `run-seamlyLayout`, not pushed

- `local_build_msi.ps1` stopped at `nmake check`: `TST_VPiece::SplineNodeVertices(tangent breaks)` failed (53 passed, 1 failed).
- Cause: the row asked for a tangent break at a spline path interior node. `VSplinePoint` and `VCubicBezierPath::GetSpline()` make every interior node smooth, so the point cannot exist.
- Fix: `isSplineEndNode` reads the curves that meet at the point. Row retargeted; new test `TST_VPiece::CubicBezierPathNodeVertices`.
- User decision: keep both smoothing corrections; never make an end node smooth; never drop an end node (memory `spline-end-node-rules`).
- Verified: `cl /Zs` on `vpiecepath.cpp` and `tst_vpiece.cpp`. Not run: Qt suites (next MSI).
- Per-suite logs: set `SEAMLY_TEST_LOG_DIR`, run `.\Seamly2DTests.exe` through `..\target_wrapper.bat` from `src\test\Seamly2DTest\bin`. The `.\` is needed when `NoDefaultCurrentDirectoryInExePath` is set.
- Next: MSI build, then the 2026-10-15 next steps below.

### 2026-10-15 — Layout.27.13.5 spline interior nodes: merged into local `run-seamlyLayout`, not pushed

- MSI 26.10.1421 capture: still 6 of 8 jacket2 pieces graded. Copy at `C:\Users\susan\seamly2d\layouts\handoff_jacket2_26.10.1421.svg`; fixture not replaced.
- Cause found for Detail (3): interior node А20 of `SplPath_А_А3` is on the piece's curve from about size 540 (О6 = spline length − 22 mm). It counted as a node, so node counts differed and `unifyNodeTurnPoints` skipped the piece; size 700 then flagged А20 alone.
- User vocabulary: end node, interior node, control node (memory `spline-node-vocabulary`). User rule: no task names, dates or Claude in commits, code comments or briefs.
- Fix: `isSplineEndNode` in `vpiecepath.cpp`; `curveNodeVertices` keeps spline end nodes only. Geometry unchanged: `isNode()` only feeds turn points and corner numbers.
- Logging (`v.mainnoguiwindow`, info level, Seamly2D log file): per-piece node counts that differ between sizes, forced corner cuts, one summary line.
- Verified: `cl /Zs` on 6 `.cpp` files. Not run: Qt suites (next MSI). Qt suites for the previous change passed on the 26.10.1421 binaries.
- Rewrote 3 unpushed doc commits to drop task names from their messages (same trees).
- Next: MSI build, capture jacket2, check the log line (see Layout.27.13.4), then replace the fixture and set `jacket2_base_size_export_names_and_grades_each_piece` to 8 graded, no warnings, 8 layer 5 lines.
- Risk: the next male_shirt capture can lose turn points that sat on spline interior nodes; recheck `male_shirt_pieces.svg` FrontPanel count before refreshing that fixture.

### 2026-10-14 (evening) — fixtures refreshed, Layout.27.13 Seamly2D fix: merged into local `run-seamlyLayout`, not pushed

- Handoffs captured from MSI 26.10.1416 with `C:\Users\susan\seamly2d\capture_handoff_*.bat` (double-click; sets `SEAMLYLAYOUT_SAVE_HANDOFF`, opens the pattern).
- Fixtures: `cxxqt_bridge/test_data/jacket2_40-146_handoff.svg` replaces the old export; `male_shirt_pieces.svg` refreshed (FrontPanel 16 turn points). jacket2 still 6 of 8 graded: its turn points equal the old fixture.
- Layout.27.13 (user decision: fix in Seamly2D): Detail (3) curve end overshoots its node by less than 1 px (`VAbstractCurve::FromBegin` pixel shortcut) → `curveSegmentPoints` in `vpiecepath.cpp`. Detail (6) `AngleByLength` cuts the corner in sizes 220–340 only → forced cut in every size (`unevenCornerCuts`, second pass over the sizes that differ).
- Verified: `cargo test --workspace` 511 passed; `cl /Zs` on the 7 changed `.cpp` files (ui headers regenerated into the scratchpad). Not run: Qt suites (ask before `local_build_msi.ps1`).
- Next: MSI build; Layout.27.13.3, 27.13.4 (capture jacket2 again, expect 8 graded), 27.7.2.

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
