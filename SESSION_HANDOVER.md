# Session handover

## Current status

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

## Decisions to retain

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
