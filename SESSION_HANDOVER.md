# Session handover

## Current status

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

## Decisions to retain

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
