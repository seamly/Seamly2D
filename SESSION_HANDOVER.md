# Session handover

## Current status

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
- Verified: `cargo test --workspace` 559 passed. Scratch harness with the sample layouts' piece sizes: Marker 16,944 → 14,840 px, side-gap excess 1,009 → 0 px; 208-piece jacket2 74,605 → 73,165 px. Not run: ctest (no C++/QML change), GUI, MSI (ask first).
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
- Verified: `ctest --preset release` on the last release build ran 6 suites, StartupOptionsTests failed (old binary); fixed test passed in a release-mode scratch harness (28 passed, 1 skipped); `cargo test --workspace` under vcvars 549 passed; batch text rendered for both modes. Not run: full `local_build_msi.ps1` (ask first).
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
- Verified: 14 piece mirror test cases passed in a scratch QtTest harness (current vpatterndb sources linked with the 2026-10-17 in-tree libs); a mutation (no curve reverse) failed 2 of them. `cl /Zs` on every changed `.cpp`; uic, moc, rcc on the new files. Not run: Qt suites, GUI. User check: Layout.80.3.1.
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
