# TODO — SeamlyLayout app features

Tasks that add features to the SeamlyLayout layout app.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

Tasks in this file are numbered and are prefixed with `Layout.`

## Task Layout.000 - Open SeamlyLayout.exe

- [ ] Layout.000.1 While in 'Piece Mode' in Seamly2D, pressing the 'Layout' button on the menu should immediately run SeamlyLayout.exe; currently when the Layout button is pressed Seamly2d's 'Layout Mode' appears then seamlyLayout.exe appears --> fix: do not display the old layout canvas and tools, they have been superceded by the SeamlyLayout.exe application.

- [ ] Layout.000.2 When the 'Adjust Mode' is exited, return to 'Piece Mode' in Seamly 2D.

## Task Layout.00 - Layout Settings

- [ ] Layout.00.1 - For setting 'Layout Mode', add a checkbox for 'None'; when 'None' is checked the layout algorithm should ignore the grainline direction when placing the pieces efficiently.

## Task Layout.0 - layout algorithm improvement

- [ ] Layout 0.1 - The layout algorithm has suffered regression - the pieces are nested sub-optimally (there is a lot of space between the pieces). Fix the layout algorithm so that the pieces only have the gap specified between them.

- [ ] Layout 0.2 - Implement the 'Layout Mode' == 'None' option so that the grainline direction is ignored while the pieces are efficiently arranged. 

## Task Layout.1 - 'Adjust Layout' improvement

- [ ] Layout 1.1 - The 'Adjust Layout' feature has suffered regression - when 'Adjust Layout' opens there are no pattern pieces to adjust - the pieces from the main SeamlyLayout canvas should be available in 'Adjust Layout', otherwise there is nothing to adjust and save back to the main SeamlyLayout canvas.

## Task Layout.2 - in 'Export SVG' menu selection, create three options for text mode

Replace the single "SVG" item in the SeamlyLayout Exports menu (`src/app/seamlylayout/qt_frontend/qml/ExportMenu.qml`, `exportSvgRequested()` wired through `Main.qml` to the Rust backend in `src/app/seamlylayout/crates/cxxqt_bridge/src/exports.rs`) with three SVG export modes differing in how label text is written.

- [ ] Layout.2.1 - **Text as `<text>` in the designer's selected (outline) font** — searchable, editable, re-stylable, human- and machine-readable; embeds the font via `@font-face` so it renders correctly on machines without it; supports tech-pack generation. Smallest file size.
- [ ] Layout.2.2 - **Text as `<text>` in a Hershey/single-line font** — same searchable/editable/machine-readable intent as mode 1, but using a bundled single-line font so the result is also friendly to CAD/CAM tools that resolve text. **Implementation note:** true Hershey fonts are stroke data, not installable outline fonts — for `<text>` + `font-family` to work this mode needs a "hairline" single-line TTF/OTF (an engineered font whose outline doubles back on itself to look like one stroke) embedded via `@font-face`. Known candidates: CamBam Stick Fonts (free, 9 variants, designed for CNC/plotting), MecSoft/Rhino single-stroke fonts, commercial single-line TTF bundles — verify redistribution/embedding license compatibility with the MIT Rust core before bundling. Caveats: hairline TTFs are still doubled-back outlines (stroke width not controllable via the font), and consumers that ignore embedded fonts will substitute — so mode 3 remains the guaranteed-fidelity choice for cutters. Record the font choice and rationale in `DECISIONS.md`.
- [ ] Layout.2.3 - **Text converted to paths (single-stroke)** — each label rendered as single-stroke `<path>` polylines from Hershey glyph data (**decision:** the path conversion uses the Hershey font, not the designer's outline font — a plotter then draws each character in one pen pass instead of tracing hollow glyph contours). Compatible with CAD/CAM/cutters/plotters/engravers with no font dependency in the consumer. Keep the original string machine-readable via `data-*`/`<desc>` on the label group (text is no longer searchable/editable as SVG text).

**Dependency:** all three modes need real `<text>` elements in the incoming `.pieces.svg` (Task 10) — even mode 3 needs the label *strings* to re-render them in stroke glyphs. Already-outlined input (Seamly2D `--text2paths`) can only be passed through as-is; the UI must handle path-only input (disable the text modes with an explanatory tooltip, or export the existing paths with a warning). Optional Hershey display/export on the Seamly2D side is Task 22.

- [ ] Layout.2.4 - Replace the single "SVG" `MenuItem` with a three-entry submenu (or dialog choice) in `ExportMenu.qml`; add per-mode tooltips summarizing the compatibility/editability trade-off; wire new signals through `TopMenuBar.qml`/`Main.qml` to the bridge
- [ ] Layout.2.5 - Mode 1: pass `<text>` through in the designer's font and embed the font as a subsetted `@font-face` data-URI (WOFF/TTF); document the font-licensing caveat (embedding rights vary by font license) in the export docs
- [ ] Layout.2.6 -  Mode 2: emit `<text>` styled with the bundled single-line font, embedded via `@font-face`, per the `DECISIONS.md` decision
- [ ] Layout.2.7 -  Mode 3: implement single-stroke text rendering in the Rust core — shape each label string into Hershey glyph strokes and emit stroked (fill-less) `<path>` polylines via `svg_dom`; keep the original label string on the group via `data-*`/`<desc>`
- [ ] Layout.2.7 - Bundle Hershey/single-line glyph data under a permissive license compatible with the MIT Rust core (evaluate existing crates, e.g. a Hershey-font crate, before hand-rolling)
- [ ] Layout.2.8 - Preserve the `data-*` tagging contract (`piece_label`/`pattern_label` groups, ids, `data-parent`) identically in all three modes
- [ ] Layout.2.9 - Detect path-only input (no `<text>` in labels) and gate all three text modes accordingly
- [ ] Layout.2.10 - Persist the last chosen SVG text mode in preferences (`PreferencesModel`)
- [ ] Layout.2.11 - Tests: Rust unit tests for each conversion mode (mode 3 output is stroked polylines, not filled contours), plus the path-only-input case; frontend test for menu gating; end-to-end check with the richmond test pattern
- [ ] Layout.2.11 - Update `src/app/seamlylayout/docs/status-docs/svg-data-attributes.md`, the root `project-docs/SVG-DATA-ATTRIBUTES.md` mirror, and `src/app/seamlylayout/docs` export docs
- [ ] Layout.2.13 - Doxygen briefs + inline comments on all touched functions

## Task Layout 3 — if current pattern is 'multisize', create three multisize options (nested / marker / sized-layout-set) that is required before user can select the export file format

Add layout export for multisize patterns — `.sm2d` patterns opened with a `.smms` multisize measurement file (multiple sizes; the CLI already exposes per-size gradation via `--gradationsize`/`--gradationheight`). The user chooses one of three multisize layout products in the settings dialog; all products orient every piece with its grainline pointing up.

- [ ] Layout 3.0 on Import of svg file, detect if .sm2d file reference an .smis measurement file (individual measurements) or .smms measurement file (multisize measurements), this variable should be readable (not writable) by the Export menu, layout algorithm, 'Adjust mode', and other code.
- [ ] Layout 3.1 Settings dialog: user chooses "nested layout", "marker layout", or "set of sized layouts" for multisize export
- [ ] Layout 3.2 Generate a "size layout" for each size in the `.smms` file, all grainlines pointing up (per-size piece generation via the existing gradation machinery)
- [ ] Layout 3.3 Nested layout:
  - [ ] Layout 3.3.1 For each piece in the largest size, create a layout with all grainlines pointing up
  - [ ] Layout 3.3.2 For the remaining sizes in descending order: place each piece on top of its matching largest-size piece, grainline up, centering its center point on the largest piece's center point — each large piece becomes the base of a "pyramid" of matching pieces with the smallest on top
  - [ ] Layout 3.3.3 Apply transforms so all pieces are placed in global space
  - [ ] Layout 3.3.4 Group all pieces of each size together, so upstream tools (Pattern Projector, Inkscape, Illustrator, ...) can toggle each size's visibility
- [ ] Layout 3.4 Marker layout: copy all pieces from the size layouts and arrange them into a single marker layout, all grainlines pointing up
- [ ]  Layout 3.5 Set of sized layouts:
  - [ ] Layout 3.5.1 Let the user view each size's layout in the canvas — UI design open: per-size tabs across the top of the canvas is the working idea, to be settled during implementation
  - [ ] Layout 3.5.2 Export the set to a single multi-page PDF, or to individual files of any export type
- [ ] Layout 3.6 Tests with a multisize test pattern (need a `.sm2d` + `.smms` fixture); verify grouping/grainline orientation in the exported SVG/PDF
- [ ] Layout 3.7 Doxygen briefs + inline comments on all touched functions; document the three products in the repo docs

## Task Layout.4 — One writer for the SeamlyLayout debug log

The log file has **two independent writers and they overwrite each other**, so
lines get clipped mid-string and a message that was written can look absent:

- C++ `Logger` (`src/app/seamlylayout/qt_frontend/src/Logger.h`) holds a static
  `QFile s_file` plus a **buffered** `QTextStream s_stream` open on the file for
  the life of the process.
- Rust `log_to_file()` (`src/app/seamlylayout/crates/cxxqt_bridge/src/lib.rs:457`,
  debug builds only) opens the **same path** with `OpenOptions::append(true)` and
  closes it on every call.

Two file handles with independent positions, one of them buffered, means each
side's flush can land on top of the other's bytes. Until this is fixed: do not
conclude a log line is absent because it looks truncated — grep for a
distinctive fragment instead.

- [ ] Layout.4.1 Decide the single owner of the file and record it in
  `DECISIONS.md` — either the Rust side logs through the C++ `Logger` across the
  cxx-qt bridge, or `Logger` stops holding the file open and both sides
  append-and-close per line. Prefer one writer over trying to interleave two
- [ ] Layout.4.2 Implement the chosen design; keep the existing line format
  (`[unix_seconds] DEBUG: message`) so current logs stay readable and both call
  sites' signatures stay unchanged (~20 Rust call sites in `lib.rs`,
  `layout_utils.rs` and `exports.rs`; `Logger::log()` on the C++ side)
- [ ] Layout.4.3 Serialize concurrent writes — the Rust bridge can be called from
  a non-GUI thread, so whatever owns the file needs a mutex or an equivalent
  guarantee
- [ ] Layout.4.4 Keep the release-build behaviour: `log_to_file()` is a no-op
  when `debug_assertions` is off, and that must not regress
- [ ] Layout.4.5 Test: write interleaved lines from both sides (and from two
  threads) and assert every line arrives whole and in order
- [ ] Layout.4.6 Doxygen briefs + inline comments on all touched functions

## Task Layout.5 - Implement additional export formats

- [ ] Layout 5.1 DXF-AAMA — biggest install base in apparel PLM, reference implementation already in the repo
- [ ] Layout 5.2 HPGL — unlocks the whole plotter and cutter class
- [ ] Layout 5.3 PS/EPS — one writer covers both and retires pdftops
- [ ] Layout 5.4 JPG — trivial, fit it anywhere

## Task Layout.6 - Implement export stubs for paid export modules (To be developed)

These options in the Export menu will remain invisible to the user until these features are developed. Put stubs in the code now to mark where they will go.

- [ ] Layout 6.1 Export G-Code
- [ ] Layout 6.2 Export 3DMesh

## Task Layout.7 — Installer: write SeamlyLayout's desktop-shortcut flag to its own registry key

Found during MSI Test Case verification, step 5c (`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`). `SeamlyLayoutDesktopShortcutComponent`'s `RegistryValue` (`packaging/windows/smsi_shortcuts.wxs:95-100`) writes `DesktopShortcutSeamlyLayout` under `HKLM\SOFTWARE\Seamly\Seamly2D` instead of `HKLM\SOFTWARE\Seamly\SeamlyLayout`.

- [ ] Layout.7.1 Change the `RegistryValue`'s `Key` at `smsi_shortcuts.wxs:96` from `SOFTWARE\Seamly\Seamly2D` to `SOFTWARE\Seamly\SeamlyLayout`
- [ ] Layout.7.2 Confirm `smsi_check_authoring.ps1:562` and `test_msi_install.ps1` still pass, and update either script if it asserts the old key
- [ ] Layout.7.3 Re-run MSI Test Case verification step 5c to confirm `HKLM\SOFTWARE\Seamly\SeamlyLayout` carries `DesktopShortcutSeamlyLayout`

## Task Layout.8 — SeamlyLayout default paths don't resolve under %DATAROOT%

Found during MSI Test Case 1 verification (`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`, steps 6c, 6d, 7c, 7d, 7e), on a fresh install with no prior SeamlyLayout state.

Fresh-install symptom in `qt6_seamlylayout.ini` and `preferences/default_preferences.json`: `preferences_directory`, `preferences_file`, `settings_directory`, `settings_file` all resolve to `C:\Users\<user>\seamlyLayout\...` (raw home directory) instead of a path under `%DATAROOT%`. `default_settings.json` is written to that same wrong directory. Only `data_root` (in the ini) resolves correctly.

`installerDataRoot()` (`PreferencesModel.cpp:56`) exists specifically to prevent this — it reads `HKLM\SOFTWARE\Seamly\SeamlyLayout\DataRoot` and feeds it into `expandDefaultPathTokens()` (`PreferencesModel.cpp:153`), which every one of the six `default_preferences.json` path keys goes through identically in `seedFromBundledDefaults()` (`PreferencesModel.cpp:190-224`). On the test machine the registry key is present and correct (`DataRoot=C:\Users\<user>\Documents\SeamlyData\`, confirmed via `Get-ItemProperty HKLM:\SOFTWARE\Seamly\SeamlyLayout`), so `installerDataRoot()` should not be returning empty — yet the seeded `preferences_directory`/`settings_directory` show no sign of it.

`input_directory` and `layout_directory` are not a clean counterexample: both hold the identical value `<DataRoot>\layouts`, which matches neither the raw `${HOME}/seamlyLayout/input`+`/output` template nor the DataRoot-substituted form of that same template (`<DataRoot>/seamlyLayout/input`+`/output`). Something other than `seedFromBundledDefaults()` — most likely the runtime fallback in `resolvedInputDirectory()`/`resolvedLayoutDirectory()` (`PreferencesModel.cpp:756` on, `<dataRoot>/input` and `<dataRoot>/output` respectively — still not `/layouts`) or a save-on-close of a session value the UI set to the shared `%DATAROOT%\layouts` folder — overwrote these two after seeding. `preferences_directory`/`settings_directory` have no equivalent runtime fallback, so they were never touched again after the (apparently DataRoot-less) initial seed.

- [x] Layout.8.1 Determine why `installerDataRoot()` returned empty (or was never consulted) when `seedFromBundledDefaults()` first ran on this fresh install, despite the registry key being correct by the time of manual inspection — prime suspect: MSI custom-action ordering, i.e. SeamlyLayout's first launch (or a validation script's launch of it) happening before the `DataRoot` registry value is written. Instrument `installerDataRoot()` and `seedFromBundledDefaults()` (`Logger::log`) and reproduce with a fresh install.
  Resolved by design instead of by root-causing the race: `preferences_directory`/`preferences_file`/`settings_directory`/`settings_file` no longer call `installerDataRoot()` at all (Layout.8.3), so the ordering question is moot for them. `Logger::log` instrumentation was added to `installerDataRoot()` and `seedFromBundledDefaults()` regardless, so a future fresh-install pass can still see whether `input_directory`/`layout_directory` (which do keep the DataRoot substitution) hit the same race.
- [x] Layout.8.2 Determine what sets `input_directory`/`layout_directory` to `<DataRoot>\layouts` at runtime, and why `preferences_directory`/`settings_directory` have no equivalent correction.
  Not conclusively reproduced by static reading — ruled out every code path that currently exists (`resolvedInputDirectory()`/`resolvedLayoutDirectory()` produce `/input`+`/output`, not `/layouts`; no QML or Rust code sets either field to a `layouts` leaf; the MSI ships no `settings/preferences.json` for `migrateLegacyPreferencesJson()`'s legacy-import candidates to pick up — `smsi_files.wxs` `install(DIRECTORY ... PATTERN "preferences.json" EXCLUDE)` confirms this explicitly). Leaving open pending a live repro; the new `Logger::log` lines in `installerDataRoot()`/`seedFromBundledDefaults()` (Layout.8.1) should help on the next fresh-install pass. Does not block 8.3: 7c already confirmed `input_directory`/`layout_directory` resolve correctly, so the fix below does not touch them.
- [x] Layout.8.3 Fix root cause so `preferences_directory`, `preferences_file`, `settings_directory`, `settings_file`, and `default_settings.json`'s own location all resolve under `%DATAROOT%` (or `%LOCALAPPDATA%\Seamly\SeamlyLayout`, matching how Seamly2D/SeamlyMe keep app-config separate from user data) on a fresh MSI install, with no dependency on launch ordering.
  Implemented the `%LOCALAPPDATA%\Seamly\SeamlyLayout` option: `seedFromBundledDefaults()` (`PreferencesModel.cpp`) now anchors all four paths (and `default_settings.json`'s directory) directly under `appConfigRootPath()`, the same deterministic, registry-free root `qt6_seamlylayout.ini` itself already uses — instead of routing them through `expandDefaultPathTokens()`/`installerDataRoot()`. `input_directory`/`layout_directory` are untouched (still DataRoot-substituted; they are genuine user data and 7c already confirmed them correct). Removed the now-unused `settings_directory`/`preferences_directory`/`preferences_file`/`settings_file` keys from the bundled `preferences/default_preferences.json` template. Added `PreferencesModelTests::layout8_resetToDefaults_seedsAppConfigPreferencesAndSettingsPaths` as a regression test. Updated `docs/packaging-docs/INSTALLER_NOTES.md`'s Runtime Folder Layout section to match.
- [ ] Layout.8.4 Re-run MSI Test Case verification steps 6c/6d/7c/7d/7e on a fresh install to confirm.
  Needs a human at the keyboard on a real machine (elevated fresh MSI install/uninstall cycle) — see `project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`. Not run this session.

## Task Layout.9 — Piece-mode handoff passes a file, not a stringified SVG document

Found during MSI Test Case 1 verification (`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`, step 6b-ii). Cross-reference: `Seamly2D.5` in `TODO_SEAMLY2D.md` is the Seamly2D-side half of this same task.

`MainWindow::exportPiecesToSeamlyLayout()` (`src/app/seamly2d/mainwindow.cpp:4153`) writes the pieces to `<pattern-basename>.pieces.svg` next to the pattern file and launches SeamlyLayout detached with that file path as its one positional argument (`StartupOptions.{h,cpp}`, `SeamlySuitePaths::seamlyLayoutLaunchArguments()`). The MSI test plan's expectation is that piece-mode data reaches SeamlyLayout as a stringified SVG document, not as a file — so either the test expectation is stale or this handoff needs to change to pass the SVG content directly (e.g. via stdin, a temp pipe, or an IPC call) instead of a file path.

- [ ] Layout.9.1 Confirm with the project owner whether the file-based handoff is the accepted design (in which case update the MSI test doc's expectation and close this task) or whether a stringified-SVG handoff is still required.
- [ ] Layout.9.2 If a stringified-SVG handoff is required: design the transport (stdin vs. a new CLI flag vs. IPC) and update `StartupOptions.{h,cpp}` and the Seamly2D-side launch call together — see the "Change one side and you must change the other" rule in `src/app/seamlylayout/CLAUDE.md`.
- [ ] Layout.9.3 Update `StartupOptionsTests.cpp` and `TST_SeamlySuitePaths` to pin the new contract; update `project-docs/SVG-DATA-ATTRIBUTES.md`.
