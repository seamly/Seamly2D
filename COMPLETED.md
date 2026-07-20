# COMPLETED — Tagged SVG Handoff to SeamlyLayout

Tasks moved here from `TODO.md` when all their subtasks are complete.

## Task 29 — Dependabot: update the `time` crate in seamlylayout (GHSA-r6v5-fh4h-64xc) (2026-07-20)

GitHub Dependabot alert #1 (moderate): `time` 0.3.46 in `src/app/seamlylayout/Cargo.lock` fell in the vulnerable range `>= 0.3.6, < 0.3.47` (stack-exhaustion DoS, [GHSA-r6v5-fh4h-64xc](https://github.com/advisories/GHSA-r6v5-fh4h-64xc)).

- [x] Identified the dependency chain with `cargo tree -i time`: `time` is pulled in transitively via `lopdf v0.32.0` ← `cxxqt_bridge` (workspace crate)
- [x] `cargo update -p time` updated the lockfile: `time` 0.3.46 → 0.3.53 (≥ 0.3.47 patched), plus its own transitive companions `time-core` 0.1.8→0.1.9, `time-macros` 0.2.26→0.2.31, `deranged` 0.5.5→0.5.8, `num-conv` 0.2.0→0.2.2 — all semver-compatible lockfile-only bumps, no `Cargo.toml` changes
- [x] Rebuilt and ran the tests: `cargo test --workspace --no-fail-fast` — 243 passed, 7 failed, and the 7 are exactly the pre-existing Task 27 set (3 `layout_tiling`, 4 `polygon_pack`), nothing new; Qt frontend Debug build (CMake/Ninja, links the updated Rust bridge) + all 4 ctest suites pass (AdjustScene, AdjustController, PreferencesModel, SettingsModel)
- [x] Committed the updated `Cargo.lock` on the task branch (pushed via the combined Task 28/29/27 PR); Dependabot [alert #1](https://github.com/seamly/Seamly2D/security/dependabot/1) to be confirmed auto-resolved after the merge

## Task 28 — Resolve the uncommitted whitespace-only changes to PreferencesModel.cpp/.h (2026-07-20)

Left over from the Task 19 move: `src/app/seamlylayout/qt_frontend/src/PreferencesModel.cpp` and `.h` sat modified in the working tree with whitespace-only changes (trailing-space trims on a handful of comment lines — `git diff -w` showed no difference), most likely an editor trim-on-save when the files moved on disk.

- [x] Decide: **kept the trims** — committed as a tiny whitespace cleanup on the task branch (removing trailing whitespace is strictly an improvement, and discarding would just let the editor re-trim them on the next save)
- [x] Clean `git status` after the commit
- [x] Re-dirtying risk checked: a trailing-whitespace scan of all `.cpp/.h/.rs/.qml` files under `src/app/seamlylayout` found zero remaining occurrences, so the subtree is already fully normalized — no follow-up normalization commit needed; future trim-on-save cannot re-dirty anything

## Task 19 — Move seamlyLayout code from `/seamlyLayout` to `/src/app/seamlylayout` (2026-07-19)

Relocated the daughter layout app into the standard app tree alongside `src/app/seamly2d` and `src/app/seamlyme`. It keeps its own build (Rust + Qt 6.10/QML) and stays out of the Seamly2D qmake build.

**Decision:** seamlylayout is treated the same as seamlyme — its source is tracked directly in this repo as ordinary files (no submodule). The nested repo was absorbed, not `git mv`'d.

- [x] Absorb the nested repo: archived `seamlyLayout/.git` to `C:\Users\susan\Projects\seamlyLayout-dotgit-archive` (it held a unique stash; `main` was fully pushed to `seamly/seamlyLayout`, which keeps the standalone history), moved the tree to `src/app/seamlylayout`, and added 309 curated files (the nested repo's tracked set minus `.vs\` IDE binaries, a stray `docs\status-docs\image\TODO\*.exe`, and `.vscode\extensions.json`)
- [x] `.gitignore` coverage: the moved subtree keeps its own `src/app/seamlylayout/.gitignore` (still functional for `/target`, `/input`, `/output`, `/logs`, `qt_frontend/build/` etc.); the root `.gitignore` additionally lists the build outputs explicitly and re-includes `src/app/seamlylayout/docs/` past the global `docs/` ignore
- [x] Updated all old-location references: root `CLAUDE.md`, `TODO.md`/`PROJECT_PLAN.md` task text, `COMPLETED.md`/`SESSION_HANDOVER.md`, `.github/README-BUILDS.md`, and the moved tree's own `.clangd` (clangd compile DB path), `qt_frontend/settings/preferences.json` (absolute input/layout/settings paths), `qt_frontend/build_debug.bat`, and its `CLAUDE.md` (the `~/seamlyLayout/` home-data paths in packaging/docs are user-data locations, untouched — Task 15 territory)
- [x] seamly2d-side references: updated the development-fallback path in `Application2D::seamlyLayoutFilePath()` (`src/app/seamly2d/core/application_2d.cpp`); the `paths/seamlyLayoutApp` setting is a user-configured value with no baked-in repo path, no change needed
- [x] Confirmed qmake exclusion: `src/app/app.pro` SUBDIRS lists only `seamlyme` and `seamly2d`
- [x] Verified the seamlyLayout build from the new location: build script is `src/app/seamlylayout/qd.ps1` (app root, not `qt_frontend/`); CMake configure + debug build + all 4 Qt frontend ctest suites pass (AdjustScene, AdjustController, PreferencesModel, SettingsModel); Rust workspace tests 244 passed / 7 failed — the 7 failures (3 `layout_tiling`, 4 `polygon_pack`) are pre-existing on the nested repo's `main` (crates byte-identical before/after the move), tracked as follow-up work, not caused by the relocation
- [x] Verified a full Seamly2D build (`scripts/sd.ps1`) is unaffected — Debug build OK
- [x] Updated `.github/README-BUILDS.md` and the seamlyLayout `CLAUDE.md` for the new paths

## Task 23 — Fix the Seamly2DTests suite hanging at startup on Windows (local runs) (2026-07-18)

The debug-built `Seamly2DTests.exe` appeared to hang at startup with no QTest output. **Root cause:** not a deadlock — Qt looks for the platform plugin (`platforms\qwindowsd.dll`) next to the *executable*, and windeployqt only deployed it next to `seamly2d.exe`; the resulting "no Qt platform plugin could be initialized" `qFatal` pops a hidden modal dialog in a debug-CRT build, which blocks forever with zero output.

- [x] Reproduce and locate the block — root cause found 2026-07-18: missing platform plugin next to the test exe → modal `qFatal` dialog from `qguiapplication.cpp`; workaround `QT_PLUGIN_PATH` confirmed
- [x] Fix it properly so the suite runs without manual env setup: `Seamly2DTest.pro` now post-links `windeployqt` on the test target plus the `copyToDestdir` xerces-c copy (mirroring `seamly2d.pro`), so the Qt runtime, `platforms\` plugin dir, and `xerces-c_3_3.dll` all land beside `Seamly2DTests.exe` — verified in both the debug (`seamly2d-build-debug\`) and release (`build\`) trees
- [x] Identify and fix the 2 pre-existing local test failures — both in `TST_VPoster` (`BigPoster` 36≠12 pages, `SmallPoster` 4≠1): the tests sized their page grid from the *system default printer* (`setPageSize(pageLayout().pageSize())` is a no-op), and this machine's default printer uses a 5×7 in page (480×672 px at 96 DPI, allowance 38 px → exactly 6×6 and 2×2 grids). Fixed machine-independently by forcing `QPrinter::PdfFormat` + explicit A4 — the same configuration CI effectively runs with (no printers installed → PDF/A4 fallback), so CI behavior is unchanged
- [x] Make the suite easy to run: `scripts/st.ps1` ("seamly2d tests", GPLv3 header + `.SYNOPSIS`, `sd.ps1` style) sets `PATH`/`QT_PLUGIN_PATH` as a fallback for old build trees, runs `Seamly2DTests.exe`, and works around the lost-stdout issue via `SEAMLY_TEST_LOG_DIR` — honored by `qttestmainlambda.cpp`, which appends a per-suite `-o <dir>/<Suite>.txt,txt` file logger (a single `-o` is overwritten by every `qExec`); the script aggregates the logs into a pass/fail table with full `FAIL!` details and exits with the suite's exit code; `-Release` runs the `build\` tree
- [x] Verify: full suite passes locally in both trees — debug and release each 31,443 passed / 0 failed across 23 suites (exit code 0), including `TST_SvgTextItem` (Task 10) and `TST_SvgComponentTags` (Task 11); Windows test-run procedure documented in `.github/README-BUILDS.md`

Note: if the release `build\` tree has stale per-subdir Makefiles (new sources missing → LNK2019), delete `build\src\**\Makefile*` and rebuild so qmake regenerates them.

## Task 10 — Export label text as real SVG text (not paths or path outlines) (2026-07-18)

Labels exported as glyph outlines even with "text as paths" off: `QGraphicsSimpleTextItem` with a pen set paints text through a `QTextLayout` outline format, so `QSvgGenerator` never received a text draw call (0 `<text>` in the baseline).

- [x] Replace the `textAsPaths == false` branch of `createLabelItem()` with a text item that paints through `QPainter::drawText()` — new `SvgTextItem` class (`src/libs/vlayout/svg_text_item.h/.cpp`) subclassing `QGraphicsSimpleTextItem` with a `paint()` override, so the SVG paint engine's `drawTextItem()` emits real `<text>` elements
- [x] Preserve current label appearance: font family/pixel size, bold/italic per line, label color, per-line alignment, middle-eliding to label width, mirroring and rotation transforms, line spacing (all outside the changed branch; fill color now comes from the brush, no outline pen)
- [x] Keep the `textAsPaths == true` branch unchanged (explicit vector outlines remain available)
- [x] Verify `PrepareTextForDXF` / `RestoreTextAfterDXF` (`collectTextItems()`) still find and convert the new item type — `SvgTextItem` shares `QGraphicsSimpleTextItem::Type`; DXF flat export of the richmond pattern emits 64 TEXT entities with the label strings and no `%&?_?&%` placeholder leak
- [x] Verify exports (richmond pattern, CLI `--exportOnlyDetails`): SVG has 64 `<text>` inside the 23 correctly `data-*`-tagged `piece_label`/`pattern_label` groups (0 paths in label groups; font-family/size/fill carried); `--text2paths` yields 0 `<text>` / 63 outline paths in the same groups; DXF/PDF/PNG all valid; Layout Mode `.pieces.svg` shares the same render path (`arrangePieceItemsFlat(textAsPaths=false)` → `SvgGenerator`)
- [x] Update the label bullet of the `data-*` contract in `status-docs/svg-data-attributes.md` and the mirror in `src/app/seamlylayout/docs/status-docs/svg-data-attributes.md`
- [x] Doxygen briefs + inline comments on all touched functions; unit tests `tst_svgtextitem.cpp` added to `Seamly2DTest` (`<text>` emission, font styling, multi-line, DXF-discovery cast) — run in CI (`linux-test`); the local Windows debug suite hangs at startup (pre-existing, unrelated)

Note: the `textAsPaths == true` branch emits filled glyph *outlines*, which remains the behavior for outline fonts; an optional single-stroke (Hershey) alternative is tracked separately in Task 22.

## Task 11 — Add `cut_path` to the SVG component groups (2026-07-18)

A cut path is a closed internal path that is cut out of the piece and can have its own seam allowance. The data model already separated them (`VLayoutPiecePath::isCutPath()`; stored as `m_cutoutPaths` on `VLayoutPiece`), but `createCutoutPathItem()` (`src/libs/vlayout/vlayoutpiece.cpp`) tagged them `internal_path` as a placeholder because the SVG spec defined no dedicated type.

- [x] Tag `createCutoutPathItem()` items with `data-type="cut_path"` instead of `"internal_path"` (placeholder comment removed); cut paths get their own per-piece counter and `piece-<n>-cut_path-<m>` ids automatically via `addComponentGroups()` (also updated the `ItemType` doc comment in `vlayoutdef.h`)
- [x] Add `cut_path` to the type list in `status-docs/new-attributes.csv` and document its semantics (closed, cut out, may carry a seam allowance) in `status-docs/svg-data-attributes.md` and the mirror in `src/app/seamlylayout/docs/status-docs/svg-data-attributes.md`
- [x] Verify export with a pattern containing a cutout internal path (richmond has none — verified with a copy of the richmond pattern with the "Left Cut" internal path of piece "Front" flipped to `cut="true"`, CLI `--format 0 --exportOnlyDetails`): the cutout appears as `piece-1-cut_path-1` (`data-type="cut_path"`, own counter starting at 1, `data-parent="piece-1"`), the piece's 4 plain internal paths keep `data-type="internal_path"` with their own counter, all other pieces unaffected
- [x] Regression: tagged SVG inspection passes (all 12 pieces, ids/counters/parents correct); the CLI export writes the same `*_pieces.svg` the Layout Mode handoff uses (same `GetItem()` → `SvgGenerator` path), so `.pieces.svg` carries the new type; PDF and PNG exports of the cutout pattern succeed, and DXF/PDF/PNG never read `PieceItemData::ItemType`, so they are unaffected by the tag change
- [x] Doxygen briefs + inline comments on all touched functions; unit tests `tst_svgcomponenttags.cpp` added to `Seamly2DTest` (item-tree tagging: 1× internal_path + 2× cut_path; end-to-end `SvgGenerator` export: ids, per-type counters, `data-parent`) — all pass locally (run with `QT_PLUGIN_PATH` set per the Task 23 root-cause finding; verified via the QTest file logger) and run in CI (`linux-test`)

## Task 0 — Setup

- [x] Copy approved plan to `PROJECT_PLAN.md`
- [x] Create `TODO.md` and `COMPLETED.md` tracking files
- [x] Create project `CLAUDE.md` and `.claude/` settings
- [x] Export baseline SVG from the test pattern via the installed Seamly2D CLI (`--format 0 --exportOnlyDetails`, measurements passed with `--mfile`) → `status-docs/baseline/richmond-shirt-baseline_pieces.svg` (2026-07-17)

## Task 1 — Shared data keys (`src/libs/vlayout/vlayoutdef.h`)

- [x] Add `PieceItemData::Key` data-key enum (ObjectName / ItemType / PieceLetter) — wrapped in a namespace to avoid collision with `VDrawTool::ObjectName`
- [x] Remove duplicated `static const int ObjectName = 0;` from `svg_generator.cpp` and `vlayoutpiece.cpp`

## Task 2 — Restructure piece item tree (`src/libs/vlayout/vlayoutpiece.cpp` / `.h`)

- [x] `GetItem()`: root becomes empty container item; set ObjectName + PieceLetter data
- [x] Add `createSeamlineItem()` child (from `createMainItem()` body), tag `"seamline"`; remove `createMainItem()`
- [x] Tag `createAllowanceItem` → `"cutline"`, `createNotchesItem` → `"notch"`, internal/cutout path items → `"internal_path"`
- [x] `createLabelItem`: add type param, wrap text lines in a tagged group (`"piece_label"` / `"pattern_label"`)
- [x] Tag `createGrainlineItem` → `"grainline"`
- [x] `VLayoutPiece::Create`: store piece letter in `VLayoutPieceData` with getter/setter
- [x] Doxygen briefs + inline comments on all touched functions

## Task 3 — DXF text traversal guard (`src/app/seamly2d/mainwindowsnogui.cpp`)

- [x] Make `PrepareTextForDXF` / `RestoreTextAfterDXF` scan descendants recursively (shared `collectTextItems()` helper)
- [x] Fix pre-existing `paperItems.at(i)` → `.at(j)` bug

## Task 4 — SvgGenerator data-* attributes (`src/libs/vformat/svg_generator.cpp` / `.h`)

- [x] Constructor: add `patternName` param; members `m_patternName`, `m_pieceCount`
- [x] Factor render block into `renderSceneToDom(QGraphicsScene*)`
- [x] Per-piece path: piece group with `id` / `data-type="piece"` / `data-type-number` / `data-parent` / `data-name` / `data-letter`
- [x] Per-component render passes in `addComponentGroups()` (hide siblings), tag each `<g>` with `data-type`, `data-type-number`, `data-parent`, structured `id`
- [x] `mergeSvgDoms()`: wrap piece groups in `<g id="pattern-1" data-type="pattern" data-name=...>` (piece exports only); use `importNode`
- [x] Robustness: remove all `M0,0`/empty-`d` paths; fix nested empty-group removal; clean origin paths before empty groups
- [x] Doxygen briefs + inline comments on all touched functions

## Task 5 — Export callers (`src/app/seamly2d/mainwindowsnogui.cpp`)

- [x] Pass `doc->GetPatternName()` to `SvgGenerator` in both `exportSVG` overloads

## Task 6 — Programmatic tagged-SVG generation (`src/app/seamly2d/mainwindowsnogui.cpp` / `.h`)

- [x] Factor dialog-independent core out of `exportPiecesAsFlatLayout()` (`arrangePieceItemsFlat()`)
- [x] Add `generatePiecesSvg(const QString &filePath)` producing the tagged SVG from `pieceList` (text kept as real text; success checked via the file on disk)

## Task 9 — Launch SeamlyLayout development build from Layout Mode (branch `run-seamlyLayout`)

- [x] Layout Mode buttons run `C:\Users\susan\Projects\Seamly2D-private\src\app\seamlylayout\qt_frontend\build\Debug\SeamlyLayout.exe`: added the development-build location as a lookup fallback in `Application2D::seamlyLayoutFilePath()` (after the settings override and the install-directory check)
- [x] Doxygen brief + inline comments updated on the touched function

## Task 8 — Verification (2026-07-17)

- [x] Build `vlayout`, `vformat`, `seamly2d` — built clean on branch `run-seamlyLayout` (which carries the `svg-update` work) with qmake + jom, Qt 6.10.1 msvc2022_64 kit and the MSVC toolset from VS 18 Community (the only Qt/VS installed on this machine); all apps, libs and tests link
- [x] Layout Mode click produces tagged SVG; SeamlyLayout launches with it — verified in the running GUI (Shift+L on the loaded richmond test pattern): `<basename>.pieces.svg` written beside the pattern file, SeamlyLayout development build launched detached with the SVG path as its argument, and the generated SVG passes the full structural inspection
- [x] Manual SVG exports carry the attributes — CLI `--format 0 --exportOnlyDetails` with and without `--text2paths`, both fully tagged (12 pieces; seamline/cutline/internal_path/grainline/piece_label/pattern_label groups)
- [x] SVG inspection — script-verified: every group under `pattern-1` has `data-type`/`data-type-number`/`data-parent`, pattern/piece groups carry `data-name`, per-type counters and structured ids correct, all ids unique, no empty groups, no `M0,0`/empty-`d` paths
- [x] Visual diff vs baseline (`status-docs/baseline/richmond-shirt-baseline_pieces.svg`) — canvas/viewBox identical; all 53 geometry paths (seamlines, cutlines, internal paths, grainlines) byte-identical; stroke colors/line weights identical; only the 64 label glyph-outline paths differ (font outline rendering of the older installed baseline build), same count/colors/placement
- [x] DXF / PDF / PNG export regression — flat DXF (AC1027) has 2305 polylines including label outlines (validates the recursive text traversal of Task 3), AAMA DXF keeps 34 TEXT label entities, PDF valid (%PDF-1.4, proper EOF), PNG valid 7318×3423
- [x] `data-*` contract documented in `status-docs/svg-data-attributes.md` and mirrored to `src/app/seamlylayout/docs/status-docs/svg-data-attributes.md`

## Task 12 — Local debug-build script for seamly2d (Qt 6.10.x + VS 18 Community) (2026-07-17)

- [x] Create `scripts/sd.ps1` (s-prefix naming rule; "seamly2d debug", mirroring seamlyLayout's `qd.ps1`) with the project's GPLv3-or-later header (2026 Seamly2D Project, slspencer) and inline comments
- [x] Auto-locate the newest Qt `6.10.x\msvc2022_64` kit under `C:\Qt` and the VS 18 Community `vcvars64.bat`; fail early with a clear message naming what is missing (vcvars output — including the harmless vswhere warning — is suppressed; failure still caught via exit code)
- [x] Shadow-build into `seamly2d-build-debug/` at the repo root (covered by the `*-build-*` gitignore pattern), separate from the release `build/` tree: `qmake Seamly2D.pro CONFIG+=debug` then jom (falls back to nmake if jom is absent)
- [x] Verified end-to-end on this machine: debug `seamly2d.exe` (29.7 MB, `-MDd`) lands in `seamly2d-build-debug\src\app\seamly2d\bin\` with the Qt debug DLLs (Qt6Cored.dll, Qt6Guid.dll, Qt6Widgetsd.dll, ...) deployed by the windeployqt post-link step, and the executable launches to its main window
- [x] Usage documented: `.SYNOPSIS`/`.DESCRIPTION`/`.EXAMPLE` comment-based help in the script (incl. optional `-Run` switch to launch after build); `CLAUDE.md` gained a "Build Notes" section mentioning the script

## Task 7 — Rewire Layout Mode entry (`src/app/seamly2d/mainwindow.cpp`, `core/application_2d.*`)

- [x] `showLayoutMode()`: guards + `preparePiecesForLayout` kept; `exportPiecesToSeamlyLayout()` writes `<basename>.pieces.svg` beside the pattern file (replaces the built-in layout-settings auto-click)
- [x] Add `seamlyLayoutFilePath()` to `Application2D` (settings override first, then app directory) + `paths/seamlyLayoutApp` settings key with getter/setter
- [x] Preferences → Paths: "SeamlyLayout Application" row (file picker; empty = auto-detect next to seamly2d.exe)
- [x] Launch SeamlyLayout detached with the SVG path argument (SeamlyMe pattern)
- [x] Error dialogs for unsaved pattern / failed generation / missing executable; on failure `showLayoutMode()` reverts to the prior mode (`exportPiecesToSeamlyLayout()` returns bool)
- [x] Doxygen briefs + inline comments on all touched functions
