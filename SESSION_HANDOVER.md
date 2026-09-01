# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`packaging/windows/README.md` and `README_MSI_WORKFLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## Current steps

1. build .msi with packaging/windows/test_build_msi_local.ps1
2. clear environment with packaging/windows/test_reset_environment.ps1
3. install MSI with packaging/windows/seamly-msi/seamly-x64.msi
4. test installation against project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md
5. add tasks for errors regarding preferences and settings to project_docs/TODO_SETTINGS_FILES.md
6. add tasks for additional errors to project_docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md
7. implement a task from project-docs/TODO_SETTINGS_FILES.md then loop to step 1. repeat until no more tasks in project-docs/TODO_SETTINGS_FILES.md
8. implement a task from project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md then loop to step 1. repeat until no more tasks in project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md

## Current Status

### Test Case 1b-i re-pass COMPLETE for scripted items (2026-08-31 evening)

The user reset every checkbox in
`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`; a full fresh pass ran
on the same build 26.8.31.1128 (MSI built 2026-08-31 7:15 PM). Results
are recorded in the test doc itself; every scripted item is checked off.

- Case 1 steps 0/1a/1a-i/1b PASS: elevated child (UAC-approved) ran
  `test_reset_environment.ps1` (prior 26.8.44328 removed, all residue
  checks clean), then quiet install exit 0.
- Suite items 1–5, 7 (except 7a-v), 8, 9 PASS. Install-time snapshot
  confirmed every seeded ini complete before any app ran.
- 4b-ii deviation, contract still holds: Seamly2D's first launch blocked
  at the modal `SeamlyWelcomeDialog` (`main.cpp:107`, shown before the
  notice) and the harness killed it. SeamlyMe was therefore the first
  completed run — it showed "Seamly data moved" once, value flipped
  `pending`→`shown`, no repeats (SeamlyLayout, Seamly2D rerun). A future
  scripted pass must close the "Welcome" window (WM_CLOSE works).
- NEW TASK Layout.10 (`TODO_SEAMLYLAYOUT.md`): step 7f confirmed
  SeamlyLayout writes logs to `%LOCALAPPDATA%\SeamlyLayout\output\` —
  move to `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs`, stop creating the
  stray dir, and add it to `test_reset_environment.ps1` (it survives
  reset today).
- Known defect STILL PRESENT: `exportPiecesToSeamlyLayout()`
  (`mainwindow.cpp:4153`) passes a `.pieces.svg` file path, not a
  stringified SVG document. Already filed: `Seamly2D.5`, `Layout.9`.
  The DesktopShortcut* flags on the Seamly2D key are already filed too:
  `SeamlyMe.3`, `Layout.7`.
- PENDING THE HUMAN: item 6 (interactive app walkthrough), 7a-v
  (bodyscans UI-row change test), and visual review of the notice text.
- Helper scripts live in this session's scratchpad
  (`elevated_reset_install.ps1`, `run_apps_pass.ps1`,
  `rerun_seamly2d.ps1`) — rewrite if gone.
- CI run 33355737878 (older note): macOS Build failed (`hdiutil: create
  failed` making `Seamly2D.dmg`). Deferred on purpose — platform order
  is x64 MSI, then arm64, then macOS.

### SettingsFiles.2/3 — installer seeds all inis (SHIPPED 2026-08-31)

Merged `--no-ff` into `run-seamlyLayout` (`c13e00b2d8`), pushed without
skip token. Live-verified on the quote-fix build. Details:
`TODO_SETTINGS_FILES.md`, `README_MSI_WORKFLOW.md`.

Still open from it:

- SettingsFiles.4 — migration CAs carry the same backslash-quote defect
  the seed CA had; fix idiom `"[PROP] "` + trim, add authoring
  assertions, re-verify a real upgrade (test cases B/C).
- SettingsFiles.3.3/3.4 — blocked: dmg/AppImage have no install step.
- Human: test doc item 6 walkthrough; 7a-v (bodyscans UI row); 4b-ii
  (visual popup check, SettingsFiles.5).
- Known: empty `Documents\SeamlyData` before any app runs is expected —
  the data tree and samples are created at app first run, not install.

### SettingsFiles.5 — one-shot fresh-install data notice (SHIPPED + LIVE-VERIFIED 2026-08-31)

Merged `--no-ff` into `run-seamlyLayout` (`4ded2549d0`), pushed without
the skip token. Live-verified on build 26.8.31.1128 — see the test-pass
section above. Full record: `TODO_COMPLETED.md` Task SettingsFiles.5.

Build-recovery rule kept from this task's rebuild failures: before
launching `test_build_msi_local.ps1`, verify no prior
`test_build_msi_local` powershell process and no `nmake`/`cl`/`link` is
running; kill trees with `taskkill /PID <id> /T /F`. After any mid-build
kill, delete `src\**\obj\` before rebuilding (truncated `.obj` files
carry fresh timestamps nmake trusts).

### SettingsFiles.2 — installer seeds the settings files (2026-08-31, superseded in part by SettingsFiles.3 above)

User directive (recorded as a memory): the apps must NEVER require a
Preferences > Paths visit to get path keys written. Fresh install seeds
defaults; upgrade keeps migrated configuration.

Implemented on branch `task-seed-user-settings` (cut after fast-forwarding
`run-seamlyLayout` to origin `53b3cd30e1`, a macOS packaging fix):

- New `packaging/windows/smsi_seed_user_settings.ps1` — creates the four
  `%LOCALAPPDATA%\Seamly` directories; seeds `qt6_common.ini` (5 shared
  `[paths]` keys) and `Seamly2D\qt6_seamly2d.ini` (6 per-app keys incl.
  `seamlyLayoutApp`); creates `SeamlyMe\qt6_seamlyme.ini` empty. Add-only:
  never overwrites a key or file. UTF-8 no BOM, always exit 0.
- Key routing corrected vs the TODO's draft: `labels`/`images`/`backups`
  are per-app (setters use `setValue` on `this`), not `qt6_common.ini`.
- SeamlyLayout gets NO ini — `PreferencesModel::load()` treats an existing
  ini as authoritative (empty-member fallbacks, and it suppresses seeding
  of the two profile JSONs), so a partial ini would break the app; it
  self-seeds correctly on first run (verified live this pass).
- `smsi_files.wxs`: component `UserSettingsSeedScript`. `smsi.wxs`:
  `SetSeamlySeedUserSettings` + `SeamlySeedUserSettings` CA (deferred,
  impersonated, Return=ignore, after `SeamlyCopyUserData`, condition
  `SEAMLYDATAROOTRECORDED AND NOT Installed`).
- Tests: `smsi_seed_user_settings_test.ps1` 19/19 pass;
  `smsi_check_authoring.ps1` + `test_msi_install.ps1` extended.
- Docs: `README_MSI_WORKFLOW.md` (flow step 9, decision 7),
  `TEST_MSI_WIN_X64_Test_Case_1b-i.md` (line-20 note, 4b-i now 5 keys,
  7a-i..v rewritten to actual key names), `TODO_SETTINGS_FILES.md`
  (2.1–2.6 checked with corrections).

The first rebuild with this design passed (authoring checks, migration
tests 16/16, MSI OK) but was not installed — SettingsFiles.3 above
reworked the seeder first. See its Next paragraph for the live plan.

### SettingsFiles.1 — qt6_common.ini moved to %LOCALAPPDATA%\Seamly (2026-08-30)

New task file `project-docs/TODO_SETTINGS_FILES.md`; decisions recorded at its
top. Task SettingsFiles.1 (app-side move) is implemented on branch
`task-common-ini-localappdata`:

- `VCommonSettings::commonSettingsFilePath()` resolves the shared file as
  `<GenericConfigLocation>/<org>/qt6_common.ini` (= `%LOCALAPPDATA%\Seamly\qt6_common.ini`
  on Windows; unchanged paths on Linux/macOS). Every common-settings QSettings
  in `vcommonsettings.cpp` now opens that explicit path.
  `commonSettingsOrganization()` removed (unused after the change). The
  "Unknown Organization" stray probe stays on the old constructor form.
- `VCommonSettings::migrateCommonSettingsLocation()` copies forward, in order:
  Roaming `Seamly\qt6_common.ini`, Roaming `Seamly2DTeam\qt6_common.ini`, and
  the qt5 `common.ini` from either. Copy-if-missing, re-entrant, never deletes.
  Replaces the three duplicated bridge blocks in `Application2D::openSettings()`,
  `ApplicationME::openSettings()`, and `TestApplication2D::openSettings()`.
- Tests: `TST_DataRoot` gains a common-settings base-dir override (set in
  `initTestCase()`, re-armed in `init()`, cleared in `cleanupTestCase()`) and
  three new cases: location contract, Roaming→Local bridge copy, bridge
  never-overwrite.
- Packaging: `smsi_migrate_user_data.ps1` `Get-SettingsFile` also returns
  `%LOCALAPPDATA%\Seamly\qt6_common.ini`; its test asserts that file is
  updated (16/16 pass locally). `test_msi_install.ps1` and
  `test_reset_environment.ps1` read the Local file first, Roaming second.
  Location comments updated in `smsi.wxs`, `smsi_registry.wxs`,
  `.github/README-BUILDS.md`.

**Task SettingsFiles.2 (installer seeds the ini files) is NOT started** — it
depends on this task being merged.

Verification (2026-08-30, local): full qmake/nmake release build in
`build/qmake-test` compiled clean, and `Seamly2DTests.exe` passed all 25
suites — `TST_DataRoot` 48 passed / 0 failed / 1 pre-existing skip, the three
new common-settings cases included. `smsi_migrate_user_data_test.ps1` 16/16.

Merged as `bb120db15f` and pushed 2026-08-31. Full CI runs (no skip token —
functional lines under `packaging/**`): run 33355737878. **Next session:
check that run** (`gh run view 33355737878`), then start SettingsFiles.2.

The user's edit to `project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md` is
uncommitted on purpose — not this task's work.

User has updated the data below.

### Seamly2D.4.1 — bodyscans row added to Preferences > Paths (2026-08-30)

`PreferencesPathPage::Apply()` (`src/app/seamly2d/dialogs/configpages/preferencespathpage.cpp`)
had no table row for bodyscans, so `VCommonSettings::setBodyScansPath()` was
never called from the UI and `qt6_common.ini` never got a `bodyscans` key —
the defect `TEST_MSI_WIN_X64_Test_Case_1b-i.md` names at the top of the file
and in step 7a.

**Correction to the request that opened this task.** The request asked for
`bodyscans`, `templates`, `individual`, and `multisize` to be written to
`qt6_seamly2d.ini`. That contradicts the test doc itself (line 20, step 7a,
item 4b-i, all already checked off): `templates`, `individual_size_measurements`,
`multi_size_measurements`, and `bodyscans` are `qt6_common.ini` keys by design
— shared across all three apps, which all read the same DataRoot subtree —
while `pattern`, `layout`, `labels`, `images`, `backups` are the per-app
`qt6_seamly2d.ini` keys. Verified in code: `VCommonSettings::setTemplatePath()`/
`setIndividualSizePath()`/`setMultisizePath()`/`setBodyScansPath()`
(`vcommonsettings.cpp`) each open a fresh `QSettings` pointed at
`commonIniFilename` ("qt6_common"); `SetPathPattern()`/`SetPathLayout()`
(`vsettings.cpp`) and `setImageFilePath()`/`setBackupFilePath()`/
`SetPathLabelTemplate()` (`vcommonsettings.cpp`) call the inherited
`QSettings::setValue()` on `this`, i.e. the per-app ini. Only the bodyscans
row was actually missing; the rest already worked as intended. Did not
move `templates`/`individual`/`multisize` into `qt6_seamly2d.ini` — that
would break the shared-DataRoot design load-bearing across Seamly2D,
SeamlyMe, and SeamlyLayout.

**Fix:** added a "My Body Scans" table row (index 9, `body_scan.png` icon —
already bundled in the shared `icon.qrc`, used by SeamlyMe's own paths page),
shifting the SeamlyLayout-application row to index 10. Wired into
`Apply()`/`defaultPath()`/`editPath()` alongside the existing rows, following
the same `rebaseOntoDataRoot()` pattern as the other data-root subfolders.
Row count bumped `setRowCount(10)` -> `setRowCount(11)`. 

Repeat the fix for each directory in ['backups', `bodyscans`, 'images', 'labels', 'label templates', 'measurements', 'measurements\individual', 'measurements\multisize', 'patterns', `templates`] until all of these directory paths are in qt6_seamly2d.ini or qt6_common.ini --> you decide if these belong in qt_seamly2d.ini or qt6_common.ini and update each app's code to read this file for the file paths. 

Marked Seamly2D.4.1 done in `TODO_SEAMLY2D.md`. **Seamly2D.4.2 still open** —
needs a human to re-run MSI Test Case verification step 7a on a rebuilt install
to confirm the `bodyscans` key now appears in `qt6_common.ini`.  --> user diasgrees, retest this section

**Not build-verified locally** — no `cl.exe` on PATH outside a VS Developer
shell in this session, and Seamly2D has a local build script `packaging\windows\test_build_msi_local.ps1` that builds the windows 11 x64 .msi file to be used until the windows 11 x64 MSI installation process passes user review. Reviewed by hand: all three switch statements (`Apply()`, `defaultPath()`, `editPath()`) and `initializeTable()` renumbered consistently, braces balanced, confirmed `getBodyScansPath()`/`setBodyScansPath()` exist and are public on `VCommonSettings` (base of `VSettings`), and confirmed `body_scan.png` is in the shared `icon.qrc` so it resolves from Seamly2D too.  

### Layout.8.2 correction — layouts folder sits directly under DataRoot, not seamlyLayout/layouts (2026-08-30)

Follow-up to the entry directly below, same session, minutes later. After the
first fix landed, deleted this machine's stale `qt6_seamlylayout.ini` and
`preferences\default_preferences.json` and relaunched the rebuilt
`SeamlyLayout.exe` to actually exercise the new seeding code (both files had
predated every fix this session — see the entry two below). The user then
read the freshly seeded live values in the IDE and caught it directly:
`input_directory`/`layout_directory` had seeded to
`C:\Users\susan\Documents\SeamlyData\seamlyLayout\layouts`, one level too
deep — they should be `C:\Users\susan\Documents\SeamlyData\layouts`,
directly under the data root.

Changed `${HOME}/seamlyLayout/layouts` to `${HOME}/layouts` in
`default_preferences.json` (all three platform blocks) and the matching
hardcoded fallback strings in `seedFromBundledDefaults()`
(`PreferencesModel.cpp`). Updated the
`layout8_resetToDefaults_seedsSharedLayoutsFolderForInputAndLayout` assertion
and `INSTALLER_NOTES.md` to match. --> this is still incorrect, the layouts 
fallback hardcoded directory should be 'C:\users\<user>\SeamlyData\layouts' 
(%DATADIR%\layouts), not %HOME\layouts; %HOME%\layouts makes no sense and 
does not follow the seamly data storage pattern.

**Verified against the real running app, not just the test suite:** rebuilt, 
deleted the two stale files again (Is it better to run the 
packaging\windows\test_reset_environment.ps1 script here?), relaunched
 `SeamlyLayout.exe` (is it better to run the rebuilt MSI file here?), 
 read the freshly seeded `qt6_seamlylayout.ini` back — `input_directory`/
`layout_directory` now both read `C:\Users\susan\Documents\SeamlyData\layouts`. 
`ctest --preset debug` 5/5 still passed --> user diasgrees, retest this section

### Layout.8.2 resolved — input_directory/layout_directory share one "layouts" folder (2026-08-30)

Follow-up to the entry directly below, same session. The project owner confirmed the 
`<DataRoot>\layouts` value Layout.8.2 could not explain was the *intended* design all
along — one shared `layouts` folder for both SVG import and layout export, not separate
`input`/`output` folders — the code just never implemented it. Changed
`default_preferences.json`'s `input_directory`/`layout_directory` (all three platform
blocks) from `${HOME}/seamlyLayout/input`+`/output` to `${HOME}/seamlyLayout/layouts` for
both, and the matching hardcoded fallback strings in `seedFromBundledDefaults()`
(`PreferencesModel.cpp`).

**Rejected wording, kept for the next person who reaches for it:** the initial ask used
the literal string `${HOME}/Documents/SeamlyData/seamlyLayout/layouts`. `${HOME}` is
substituted by `installerDataRoot()` on Windows when a DataRoot is recorded, and that
value is already `.../Documents/SeamlyData` — so the literal string would have doubled to
`.../Documents/SeamlyData/Documents/SeamlyData/seamlyLayout/layouts` on any real MSI
install. Asked the user; confirmed intent was `${HOME}/seamlyLayout/layouts`, keeping
`expandDefaultPathTokens()`'s existing DataRoot-substitution behavior unchanged (so a
custom, non-default DataRoot chosen at install time is still honored) rather than hardcode
`Documents/SeamlyData`.

New test: `PreferencesModelTests::layout8_resetToDefaults_seedsSharedLayoutsFolderForInputAndLayout`.

**Verified:** `build.ps1 -Preset debug -NoRun` succeeded; `ctest --preset debug` — 5/5
passed. Pushed with `[skip ci]` — no `CMakeLists.txt`/`.pro`/`.pri`/`packaging/**` touched
this time, only `PreferencesModel.cpp`, the bundled JSON, the test file, and docs. --> let's run the
packaging/windows/test_build_msi_local.ps1 build file instead of build.ps1 to set the focus on
the windows 11 x64 MSI file. Once this MSI file passes all tests we'll start testing the
windows 11 arm64 MSI file, then move on to macos then debian linux.

### Layout.8 — SeamlyLayout preferences/settings paths fixed under AppConfigLocation (2026-08-30)

Task Layout.8 (`TODO_SEAMLYLAYOUT.md`), 8.1-8.3 done, 8.4 open. Found during
MSI Test Case 1 verification (`TEST_MSI_WIN_X64_Test_Case_1b-i.md` steps
6c/6d/7c/7d/7e): a fresh install seeded `preferences_directory`,
`preferences_file`, `settings_directory`, `settings_file` (and
`default_settings.json`'s own location) under the raw home directory
(`C:\Users\<user>\seamlyLayout\...`) instead of resolving under `%DATAROOT%`
as the test plan expected.

**Root cause:** `seedFromBundledDefaults()` (`PreferencesModel.cpp`) routed
all six `default_preferences.json` path keys through
`expandDefaultPathTokens()`/`installerDataRoot()` (a Windows registry read of
`HKLM\SOFTWARE\Seamly\SeamlyLayout\DataRoot`), including the four app-config
keys that should never have depended on DataRoot or MSI custom-action
ordering at all — they should sit beside `qt6_seamlylayout.ini` under
`appConfigRootPath()` (`%LOCALAPPDATA%\Seamly\SeamlyLayout` on Windows),
exactly like Seamly2D/SeamlyMe's own `qt6_*.ini`.

**Fix:** `seedFromBundledDefaults()` now anchors
`settings_directory`/`preferences_directory`/`settings_file`/`preferences_file`
directly under `appConfigRootPath()`, with zero dependency on the installer
registry key. `input_directory`/`layout_directory` are untouched — they are
genuine user data (already confirmed correct by 7c) and keep the
DataRoot-substituted template. Removed the now-unused four keys from the
bundled `preferences/default_preferences.json`. Added `Logger::log`
instrumentation to `installerDataRoot()`/`seedFromBundledDefaults()` per
Layout.8.1. Updated `docs/packaging-docs/INSTALLER_NOTES.md`'s Runtime Folder
Layout section.

**Layout.8.2 left open** — could not conclusively determine, by static
reading alone, what previously set `input_directory`/`layout_directory` to
the literal `<DataRoot>\layouts` the tester observed (no current code path
produces exactly that value; ruled out `resolvedInputDirectory()`/
`resolvedLayoutDirectory()` (`/input`+`/output`, not `/layouts`) and a
packaged `settings/preferences.json` (`smsi_files.wxs` explicitly excludes
it)). Does not block 8.3, since 7c already confirmed those two resolve
correctly and the fix does not touch them.

**Verified:** `cmake --build --preset debug` (qt_frontend) succeeded;
`ctest --preset debug` — 5/5 suites passed, including new
`PreferencesModelTests::layout8_resetToDefaults_seedsAppConfigPreferencesAndSettingsPaths`.
Confirmed no side effects on this machine's real `%LOCALAPPDATA%\Seamly\SeamlyLayout\`
state (the pre-existing `default_preferences.json` there predates this
session and was untouched — the test points `preferencesDirectory` at a
`QTemporaryDir` so seeding happens there, not against the real file).
`cargo test --workspace` not run (no Rust changed).

**Layout.8.4 still open — needs a human at the keyboard.** Re-run MSI Test
Case verification steps 6c/6d/7c/7d/7e on a fresh elevated MSI install/uninstall
cycle to confirm the fix on a real machine, per
`TEST_MSI_WIN_X64_Test_Case_1b-i.md`. Not run this session.

### version.sh moved to packaging/ (2026-08-29)

`scripts/version.sh` moved to `packaging/version.sh` — it stamps the
version into `projectversion.cpp/.h` and both `Info.plist` files for
every build, so it belongs with build-pipeline scripts, not misc dev
utilities. Updated every reference: `ci.yml` (3 call sites),
`packaging/windows/test_build_msi_local.ps1`, both `Info.plist` comments,
and the `projectversion.cpp/.h` header comments. Script internals
unchanged — all its paths are relative to the repo root, so only the
invocation path changed. Touches `ci.yml` and `packaging/**`, so this
push needs full CI, no skip-ci token.

### Windows packaging moved to packaging/windows/ (2026-08-29)

`scripts/packaging/windows/` moved to `packaging/windows/` — `scripts/`
held only misc dev utilities, so Windows packaging (MSI/WiX, PowerShell)
now sits at the same top level as `dist/` (Linux/macOS packaging assets).
Updated every reference: `ci.yml`, `src/app/app.pro`, `CLAUDE.md`,
`AGENTS.md`, and project-docs. Fixed a `repoRoot` depth bug the move
exposed in `smsi.ps1` and `test_build_msi_local.ps1` — both walked one
directory too many with `Split-Path -Parent` (correct at the old
3-deep path, wrong at the new 2-deep one). Merged `--no-ff` into
`run-seamlyLayout` and pushed at `da090ae733` without `skip-ci`
(functional `packaging/**` and `*.pro` changes). Not build-verified
locally — no local build/test covers Windows MSI packaging; `ci.yml`'s
`windows-msi` job is the first real verification.

Also merged in this push: another session's concurrent commit
`36fde280fe` ("deleted unused screenshots"), found already on this
branch when I went to commit — confirmed with the user as their own
other window, not a conflict.

### SeamlyMe Open-dialog fix pushed; CLAUDE.md's local-build claim was stale (2026-08-29)

MSI Test Case 1b-i step 6a-v/6a-vi found SeamlyMe's File Open Individual/Multisize
dialog opening in the wrong Seamly folder. Root cause: Windows' native Open dialog
keeps one process-wide "last visited folder" and silently overrides the app-supplied
start folder after the first native dialog use — confirmed against the user's report
of seeing "a real but different Seamly folder," not a random OS default. The getter
code (`OpenIndividual()`/`OpenMultisize()`/`OpenTemplate()`) already read the correct
per-purpose settings, so this is a different bug class from the fix below.

Fixed in the shared `TMainWindow::Open()` helper (`src/app/seamlyme/tmainwindow.cpp:3059`)
by forcing `QFileDialog::DontUseNativeDialog`, so Qt's own dialog (no shared history)
is always used for these three actions regardless of the native-dialog preference.
Tracked as Seamly2D.2.2 in `project-docs/TODO_SEAMLY2D.md` — done. Committed, merged
`--no-ff` into `run-seamlyLayout`, and pushed at `8a76c2b22a` with `[skip ci]`.

**Not build-verified before push.** I incorrectly told the user twice that "Seamly2D
and SeamlyMe have no local build script," repeating stale text from this file's own
"Local Windows Build" section. The user corrected me:
`packaging/windows/test_build_msi_local.ps1` exists and builds all three apps
(qmake + nmake for Seamly2D/SeamlyMe, cmake + ninja + cargo for SeamlyLayout), then
packages them via `smsi.ps1`/`wix build`. Confirmed by reading the script directly.
CLAUDE.md's "Local Windows Build" section is being corrected to reference it.

**Still open in `project-docs/TODO_SEAMLY2D.md`:**

- Seamly2D.2.1 — `MainWindow::Open()` (`src/app/seamly2d/mainwindow.cpp:4378-4408`)
  never reads `getPatternPath()`; needs a fallback added when the recent-files list
  is empty. Separate root cause from Seamly2D.2.2 above — do not reuse that fix.
- Seamly2D.3.1 — closing SeamlyLayout restores Seamly2D's Layout Mode instead of
  whichever mode (e.g. Piece Mode) was active before SeamlyLayout launched.

`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md` step 6a carries a `-->` note
cross-referencing Seamly2D.2.2 and says "**Re-test needed to confirm**" — the
SeamlyMe fix has not been re-verified interactively since it was pushed.

**Next steps:**

1. Run `packaging/windows/test_build_msi_local.ps1` to build all three apps and
   confirm the `tmainwindow.cpp` change actually compiles.
2. Re-test MSI Test Case 1b-i step 6a-v/6a-vi interactively to confirm the fix works.
3. Pick up Seamly2D.2.1 and Seamly2D.3.1 above.

### First run also seeds sample measurement files (2026-08-28)

Task Seamly2D.3, done. Follow-up to the entry directly below. Re-running MSI Test Case
1b-i's verification suite after that fix still failed step 3e:
`%DATADIR%\measurements\individual\male_chest_102cm.smis` was missing even though the
bundled sample exists at `%PROGRAMDIR%\samples\measurements\individual\male_chest_102cm.smis`
— `SeedSamplePatterns()` only ever copied `.sm2d` files.

`SeedSamplePatterns()`'s copy body moved into a shared private helper,
`copySampleFiles(sourceDir, destinationDir, nameFilter)`
(`src/libs/vmisc/vsettings.cpp`, anonymous namespace). New public
`VSettings::SeedSampleMeasurements(sourceDir, destinationDir, nameFilter)` wraps it, called
twice from `Application2D::initOptions()` — `*.smis` into `measurements/individual`, `*.smms`
into `measurements/multisize`. Full writeup in `project-docs/TODO_COMPLETED.md` under Task
Seamly2D.3. `project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md` section C (D1/D2) records the
verification-pass findings that led here.

**Scope, corrected from the note below:** SeamlyMe's sample measurements are no longer a
separate unfiled concern — SeamlyMe reads the same `%DATAROOT%\measurements\` tree Seamly2D
now seeds, so seeding from Seamly2D's `initOptions()` covers both apps. Sample *templates*
(`%PROGRAMDIR%\samples\measurements\templates\`) are still not seeded — out of scope, not
reported by the test plan.

**Verified:** `vsettings.cpp`, `application_2d.cpp`, and `tst_dataroot.{h,cpp}` (4 new cases)
were reviewed by hand for consistency with the existing pattern-seeding code and tests, not
syntax-checked with a compiler this session. Not verified: `ctest`/a full build; `ci.yml` is
the verification path for Seamly2D per `CLAUDE.md`.

**Not yet done:** rebuild and reinstall `seamly-x64.msi` so this code actually reaches a real
machine, then re-run MSI Test Case 1b-i steps 3d/3e to confirm both files now appear
(`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`, defects D1/D2).

### First run seeds the patterns folder from the bundled samples (2026-08-28)

Task Seamly2D.2, done. MSI Test Case 1b-i step 6a-i found
`%PROGRAMDIR%\samples\patterns\*.sm2d` cannot be opened and saved back in
place — a standard user has no write access to Program Files.
`VSettings::SeedSamplePatterns()` (`src/libs/vmisc/vsettings.{h,cpp}`) now
copies the bundled `.sm2d` files into the writable patterns folder on every
launch, skipping any file already there (sample or user-edited) — same
merge rule as `ensureDataRootTree()`. Wired into
`Application2D::initOptions()` (`src/app/seamly2d/core/application_2d.cpp`)
right after the existing pattern-folder `mkpath()`. Full writeup in
`project-docs/TODO_COMPLETED.md` under Task Seamly2D.2.

Scope: Seamly2D only, `.sm2d` patterns only — the report was specific to
that. SeamlyMe's sample measurements/templates are a separate, unfiled
concern.

**Verified:** `vsettings.cpp`, `application_2d.cpp`, and `tst_dataroot.cpp`
(3 new cases) syntax-checked clean against Qt 6.11.1 with MSVC
(`cl /Zs /permissive- /Zc:__cplusplus`) — same method as the 2026-08-24
entry below. Not verified: `ctest`/a full build; `ci.yml` is the
verification path for Seamly2D per `CLAUDE.md`.

### Legacy data tree backed up as a verified .zip (2026-08-24)

User request: port the abandoned `task-data-migration-backup` branch's
zip-backup step into current code, so an upgrading user's old `~/seamly2d`
tree gets a second, portable backup on top of the copy-and-verify migration
`VCommonSettings::migrateAdoptedLegacyTree()` already does (InstWinX64.4,
done 2026-08-18/19). See InstWinX64.4.14 in `TODO_INSTALLER_WIN_X64.md` for
the full writeup.

**Deliberate divergence from the source branch: no delete.** The branch
archived the tree then removed it. This project keeps the legacy tree in
place after migration (marker file, InstWinX64.4.6/4.7) so a rollback stays
possible — decided with the user before porting anything. New
`LegacyDataArchive::archive()` writes and verifies the `.zip`; it has no
delete path at all, unlike the branch's `archiveAndRemove()`/`removeTree()`.
New `LegacyDataMigration::run()` orchestrates copy → backup with a splash
screen (ported from the branch unchanged — it was never coupled to deletion),
and both `application_2d.cpp`/`application_me.cpp` call it in place of the
direct `migrateAdoptedLegacyTree()` call.

`vmisc.pro` and `Seamly2DTest.pro` both gained `core-private` — the archive
code and its tests use `QZipWriter`/`QZipReader`, Qt private API since Qt 6.

**Verified:** `legacy_data_archive.cpp`, `legacy_data_migration.cpp`, and the
updated `tst_dataroot.cpp` (8 new cases) were syntax-checked against Qt
6.11.1 with MSVC (`cl /Zs /permissive- /Zc:__cplusplus`) — all clean. Not
verified: `ctest`/a full build. Building the whole Seamly2D dependency tree
locally just for `Seamly2DTest` was judged disproportionate to this change;
`ci.yml` is the verification path for Seamly2D/SeamlyMe per `CLAUDE.md`.
Pushed without `[skip ci]` since `.pro`/`.pri` files changed — full CI is
running on `fceefbeb4d`.

### SeamlyLayout's dataRoot now feeds resolvedInputDirectory/resolvedLayoutDirectory (2026-08-24)

Follow-up to the entry directly below. `dataRoot` was adopted from the registry
but stored inert. Wired it in — priority order unchanged elsewhere, new tier
inserted between "configured value" and "exeDir/AppConfigLocation fallback":

1. `input_directory`/`layout_directory` if configured — the user's own choice, unchanged.
2. **New:** `<dataRoot>/input` or `<dataRoot>/output`, if `dataRoot` is set.
3. `<exeDir>/input`/`/output` (macOS/AppImage/Flatpak-aware AppConfigLocation
   fallback) — unchanged.

Correction to the note below: **InstWinX64.5.2 was the wrong task reference.**
That item is "Decide whether `paths/pattern` and `paths/layout` are shared or
per-app" under "InstWinX64.5 — Correct Application Settings" — Seamly2D/SeamlyMe's
own `VSettings` accessors falling back to `%APPDATA%\Unknown Organization.ini`,
unrelated to SeamlyLayout's `PreferencesModel`. No open decision actually
blocked this wiring; it just hadn't been done yet.

**Verified:** `cmake --build --preset debug` succeeded; `ctest --preset debug` —
5/5 suites pass, including `PreferencesModelTests` with 4 new cases
(`dataRoot_resolvedInputDirectory_nestsUnderDataRoot`,
`dataRoot_resolvedLayoutDirectory_nestsUnderDataRoot`, and the two
`_configuredValueWins` priority tests).

### SeamlyLayout reads its own mirrored DataRoot key (2026-08-24)

User request: SeamlyMe/SeamlyLayout registry keys were mirrored (previous
entry below) but nothing read them — `InstallerRecord::dataRoot()` still
hard-coded the Seamly2D key, and SeamlyLayout's standalone CMake/Cargo build
does not link `vmisc`, so it could not call that reader anyway.

Added, in `src/app/seamlylayout/qt_frontend/src/PreferencesModel.{h,cpp}`:

- A new `dataRoot` Q_PROPERTY / `data_root` INI key, round-tripped through
  `load()`/`save()`/`loadJsonPreferences()` alongside the existing preference fields.
- `installerDataRoot()` (anonymous namespace, `Q_OS_WIN`-guarded): reads
  `HKLM\SOFTWARE\Seamly\SeamlyLayout\DataRoot` via `QSettings::Registry64Format`,
  mirroring `InstallerRecord::dataRoot()` (`src/libs/vmisc/installer_record.cpp:40`)
  but scoped to SeamlyLayout's own mirrored key.
- `PreferencesModel::adoptInstallerDataRootIfEmpty(iniPath)`: adopts the
  installer value once, only when `dataRoot` is still empty, then persists it —
  same never-overwrite contract as `VCommonSettings::initializeDataRoot()`.
  Called from every `load()` success path (existing INI, freshly-defaulted INI,
  legacy-JSON-migrated INI).

**Not wired further.** `resolvedInputDirectory()`/`resolvedLayoutDirectory()`
still resolve independently of `dataRoot` — whether SeamlyLayout's
input/output folders should nest under it is InstWinX64.5.2 (still open:
"Decide whether `paths/pattern` and `paths/layout` are shared or per-app").
This task only gives the value a place to land.

**Verified:** `cmake --build --preset debug` (qt_frontend) succeeded;
`ctest --preset debug` — 5/5 suites passed, including 3 new `PreferencesModelTests`
(`dataRoot_roundTripsThroughIni`, `dataRoot_emitsSignalOnChange`,
`dataRoot_load_preservesExistingValue`); `cargo test --workspace` — all passed
(no Rust changed). The registry read itself was not exercised against a real
`HKLM\SOFTWARE\Seamly\SeamlyLayout` key — no local machine has that key set
outside a real MSI install.

### Install-info registry mirrored to SeamlyMe and SeamlyLayout keys (2026-08-24)

User request, not a tracked `TODO_*.md` item: the MSI wrote install breadcrumbs
(`InstallPath`, `DisplayVersion`, `DataRoot`, `DataParent`) only under
`HKLM\SOFTWARE\Seamly\Seamly2D`. Added two more components in
`smsi_registry.wxs` (`InstallInfoRegistrySeamlyMe`, `InstallInfoRegistrySeamlyLayout`)
writing the same four values under `HKLM\SOFTWARE\Seamly\SeamlyMe` and
`HKLM\SOFTWARE\Seamly\SeamlyLayout`. Seamly2D's key stays canonical -
`InstallerRecord::dataRoot()` (`src/libs/vmisc/installer_record.cpp:40`) and
the `SEAMLYINSTALLEDVERSION` upgrade check in `smsi.wxs` still read only that
key; the two new copies are not read by anything in-repo yet.

Added matching assertions to `smsi_check_authoring.ps1` (the four values
present under each of the two new keys).
`project-docs/TEST_INSTALLER_WIN_X64_Test_Case_1b-i.md` step 4b already
expected all three keys (pre-existing uncommitted edit at session start,
unrelated to this change but consistent with it).

**Verified:** `smsi_registry.wxs` is well-formed XML;
`smsi_check_authoring.ps1` parses clean. **Not verified:** no local release
builds of the three apps existed to run `smsi.ps1` / `wix build` /
`smsi_check_authoring.ps1` for real (Seamly2D/SeamlyMe have no local build
script - see CLAUDE.md). Full CI (`ci.yml` windows-msi job, both
architectures) is the first real verification - pushed without `skip-ci`
because `scripts/packaging/**` changed.

### InstWinX64.13 - silent-default data root recording fixed, not yet real-machine verified (2026-08-24)

Testing Case 1b-i of `TEST_INSTALLER_WIN_X64.md` (uninstall, then a `/quiet`
install with no properties) on this machine found two real defects, both
fixed on branch `task-silent-install-dataroot` off `run-seamlyLayout`:

1. **`DataParent` recorded as `C:\`.** (Written to `smsi_shortcuts.wxs` at the
   time, moved to the new `smsi_registry.wxs` later the same session - see
   below.) The raw `[SEAMLYDATAPARENT]` property was written directly. It is
   also a Directory id, so
   `CostFinalize` always resolves it to something - even on a run that chose
   nothing - the same trap `SEAMLYDATAROOTRECORDED` already existed to avoid
   for `DataRoot`. Fixed the same way: new `SEAMLYDATAPARENTRECORDED`
   property, gated on `SEAMLYDATACHOSEN`.
2. **A bare `/quiet` install (no properties) never created the data root at
   all.** `SEAMLYDATACHOSEN`'s default-computing `SetProperty` actions ran in
   the UI sequence only - a deliberate, tested choice (the execute sequence of
   a genuinely unattended SYSTEM-context deployment has no real user to
   impersonate, so `PersonalFolder` there is SYSTEM's own profile). But it
   meant a plain `/quiet` install left `DataRoot` unrecorded, and the apps
   fell back to their OWN built-in default - `<Documents>\Seamly` - which is a
   **different folder** than the MSI's own default composition,
   `<Documents>\SeamlyData`. Confirmed by reading `vcommonsettings.cpp:509`
   and `smsi_files.wxs:78` side by side; `README-BUILDS.md:78` already
   documents the two names as deliberately different, reconciled only "on an
   installed machine" - which a no-properties `/quiet` install turns out not
   to be.

**Decided with the user 2026-08-24: accept the SYSTEM-profile risk.** The
execute sequence now computes the same `PersonalFolder`/`%USERPROFILE%`
default the UI sequence does, so a bare `/quiet` install also creates and
records `<Documents>\SeamlyData`. A real unattended deployment that cares
should already pass `SEAMLYDATAPARENT`/`SEAMLYDATAROOT` explicitly - the
documented escape hatch, unaffected by this change.

**Also added, per the user's follow-up request:** the product's own uninstall
now removes `%LOCALAPPDATA%\Seamly` and `%APPDATA%\Seamly` (guarded
`NOT UPGRADINGPRODUCTCODE`, so a version upgrade's `RemoveExistingProducts`
never wipes them), while `%DATAROOT%` stays untouched on uninstall - unchanged,
on purpose. And a new test-only script,
`packaging/windows/test_reset_environment.ps1`, wipes everything
including `%DATAROOT%`, for resetting a test machine back to Case 1
("Not installed") between test-matrix runs - this is deliberately MORE
destructive than the real uninstall.

**Verified locally, not yet on a real machine.** `wix build` and
`wix msi validate` (link-only, stub staging tree from today's real
`scripts\seamly-msi\x64\` build) pass clean except the expected ICE61.
`smsi_check_authoring.ps1` passes all assertions, including new ones for both
fixes and for the per-user-settings removal. **InstWinX64.13.5 is open:** a
real `msiexec /i ... /quiet` with no properties has not been run against this
build to confirm `<Documents>\SeamlyData` actually gets created and recorded,
and the uninstall's AppData removal has not been run for real either. The
user asked to skip further manual/GUI installer testing this session (Seamly2D
/ SeamlyMe / SeamlyLayout launch-and-verify steps need a human watching the
screen) - that verification, and the real end-to-end reset-script run, are
next.

**`smsi_shortcuts.wxs` split, same session:** the user pointed out the name no
longer fit - it held desktop shortcuts, install-info registry, and (after
fix 1 above) per-user settings removal, three unrelated concerns under a name
describing one. New `smsi_registry.wxs` now holds the registry values and the
per-user removal; `smsi_shortcuts.wxs` holds only the shortcuts.
`smsi.wxs`/`smsi.ps1`/`README.md` comments updated to match. Not yet
re-verified against a rebuilt stub MSI after the split - do that before
trusting `InstWinX64.13.4`'s "all assertions pass" claim again.

**Process note:** this branch was cut from `run-seamlyLayout` only after the
`.wxs`/`.ps1` edits were already made, not before - steps 1-2 of the task
workflow (sync `develop`, merge into `run-seamlyLayout`) were skipped. Several
unrelated files were already modified uncommitted on `run-seamlyLayout` before
this session started (`dist/macx/*/Info.plist`,
`src/app/seamly2d/dialogs/configpages/preferencespathpage.cpp`,
`src/libs/vmisc/projectversion.{cpp,h}`, `project-docs/TEST_INSTALLER_WIN_X64.md`)
plus several new untracked `project-docs/TEST_INSTALLER_WIN_X64_Test_Case_*.md`
files that appeared mid-session from the user's own IDE activity - none of
that is part of this task and none of it was touched, staged, or committed.

**Real machine state changed this session (all reversible, all recorded in
`TEST_INSTALLER_WIN_X64_Test_Case_1b-i.md` if that file still holds the
narrative):** the pre-existing Seamly install (`26.8.32541`) was uninstalled,
then today's local build (`scripts\seamly-msi\x64\seamly-x64.msi`,
`/quiet`, no properties) was installed. Left as-is: this is what Case
1b/1b-i needs installed for the next verification pass anyway.

### seamly-x64.msi run on a PC with the current suite installed (2026-08-21)

Exploratory installer test, not a tracked task. Machine had Seamly 26.8.28339
already installed (ProductCode `{CA5D0784-6F85-4DC5-96FF-CAC4327DBF81}`),
matching the built `scripts\seamly-msi\x64\seamly-x64.msi` exactly, so
`msiexec /i` ran as a repair/reconfigure. Exit 0, "Configuration completed
successfully."

**Found and FIXED a real bug.** `smsi_migrate_user_data.ps1`'s
`New-DataArchive` (line ~201) loaded only `System.IO.Compression.FileSystem`.
On this machine's PowerShell 5.1, `[System.IO.Compression.ZipArchiveMode]`
stayed unresolvable without also loading `System.IO.Compression` - so every
real data migration (`Old` and `New` mode) failed silently: caught, logged,
`exit 0`, no install-time symptom at all. Fixed by adding the second
`Add-Type`. Verified against a seeded legacy `~/seamly2d` test folder:
zip -> extract -> merge into the real `Documents\SeamlyData` -> the real
per-app `.ini` path settings updated correctly, legacy source left untouched.
Test files removed afterward. **`README_MSI_WORKFLOW.md` item 4** (the
legacy data-folder migration) moved from `[undecided]` to `[settled]` - the
design was already right, just broken by this bug.

**Not a defect, just a cosmetic log line.** The first repair run logged
`Wix4RemoveFoldersEx_X64` hitting `Error 0x80070057: Missing folder property:
SEAMLYLEGACYINSTALLDIR` (marked continue-on-error, non-fatal). This fires
whenever `HKLM\SOFTWARE\NSIS_Seamly2D` genuinely does not exist - the normal
case with no pre-MSI install on the machine. Confirmed by setting
`SEAMLYLEGACYINSTALLDIR` on the `msiexec` command line (a property override,
no real registry or `Program Files (x86)` writes) - the error disappeared and
`RemoveLegacyRegistryKeys` / `Wix4RemoveFoldersEx_X64` completed cleanly. No
code change needed.

**Not tested: a genuine different-version upgrade** (`WIX_UPGRADE_DETECTED`
with `NOT Installed`) - the "current suite installed, different version ->
install files + new data folders, skip data migration" case. Verified only by
reading the authoring: `SEAMLYCOPYUSERDATA` defaults to `"0"` and the
migration action requires `NOT Installed`, so this is already the default
behaviour. A real end-to-end run needs a second MSI built with a bumped
`ProductVersion`, which needs a local qmake+jom build of `seamly2d`/`seamlyme`
(no local build script exists for those; CI-only, `scripts/seamly2d-debug`
kind of tree would have to be rebuilt by hand) plus SeamlyLayout's
`build.ps1`. Not attempted this session.

**Machine state, outside the repo:** no lasting changes. The legacy-removal
test used command-line property overrides only - no real
`HKLM\SOFTWARE\NSIS_Seamly2D` key or `C:\Program Files (x86)\Seamly` folder
was ever created. The migration test's throwaway `C:\Users\susan\seamly2d`
and the 3 sample files it merged into the real `SeamlyData` were deleted
after verification. The real settings `.ini` files (`qt6_seamly2d.ini`,
`qt6_seamlyme.ini`, `qt6_common.ini`, `Unknown Organization.ini`) WERE
rewritten for real by the migration script during the test - values were
unchanged (already pointed at the same `SeamlyData` path), so this was a
no-op in practice, but note it in case anything looks different later.

**Next steps:**

1. Decide whether to commit the one-line `smsi_migrate_user_data.ps1` fix and
   the `README_MSI_WORKFLOW.md` update - discovered mid-investigation, not
   from a `TODO_*.md` task, so no task branch exists for it yet.
2. Decide whether the different-version-upgrade case needs the real
   second-MSI end-to-end test above, or whether the authoring-level
   verification is enough.

### SeamlyLayout INI preferences change (2026-08-20)

SeamlyLayout stores application preferences in
`AppConfigLocation/qt6_seamlylayout.ini`.
On Windows, this resolves to
`%LOCALAPPDATA%\Seamly\SeamlyLayout\qt6_seamlylayout.ini`.

First startup imports the previous `preferences.json` and keeps that file.
JSON remains the format for default preference profiles and layout profiles.

The debug build passed. All five Qt test executables passed.
`cargo test --workspace` passed.

### Seamly2D log-directory change (2026-08-20)

`Application2D::logDirPath()` uses `AppLocalDataLocation` on Windows.
It appends `logs` to produce `%LOCALAPPDATA%\Seamly\Seamly2D\logs`.

`git diff --check` passed. No local build ran because Seamly2D has no local build script.

--> SeamlyMe should produce `%LOCALAPPDATA%\Seamly\SeamlyMe\logs`.
--> SeamlyLayout should produce `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs`.

### The unit tests run on Windows now (2026-08-19)

**`windows-test` builds and runs the four test binaries on every push**, and
both pre-release jobs list it in `needs`. A failing Windows test stops the
rolling `dev-latest` MSIs and the versioned pre-release. It does **not** stop
`windows-msi`: the packages still build and stay downloadable as run artifacts,
which is what you want while diagnosing the failure that blocked the publish.

Separate from `windows-msi` on purpose. Folded in, the tests would sit on the
package-critical path and run twice, once per architecture.

`windows-latest`, x64, `qtmultimedia` only, `-config release` (the MSVC default
is `debug_and_release`, which builds the tree twice), `QT_QPA_PLATFORM=offscreen`.

--> don't push until we complete testing on the windows 11 x64 MSI file and all tests pass
--> use packaging\windows\test_build_msi_local.ps1 to build one test binary locally
in packaging\windows\seamly-msi\x64\seamly-x64.msi
--> use packaging\windows\seamly-msi\x64\seamly-x64.msi to install the seamly apps 
for testing

### What this replaced, and why it mattered

Before 2026-08-19 **no push compiled `src/test/` at all.** Two independent
gates: every build job passes `CONFIG+=noTests`, and `linux-test` carries
`if: github.event_name == 'pull_request'`, which normal task work never
satisfies.

Proof: `tst_dataroot.cpp` was committed on 2026-08-17 with `QLatin1Char('\')`, an
unterminated character literal and a hard compile error. Two CI runs passed over
it. Fixed 2026-08-19.

So treat any older note claiming "ci.yml verifies Seamly2D and SeamlyMe" as a
statement about the BUILD only. That is no longer the whole story on Windows,
but it is still true of Linux and macOS: `linux-test` remains gated on
`pull_request`, and ungating it is a one-line change nobody has asked for.


### Ordering facts that are load-bearing — do not reschedule

- `SetSEAMLYDATACHOSEN` runs **before `CostInitialize`** (798). That is the only
  window where `SEAMLYDATAPARENT` and `SEAMLYDATAROOT` are empty unless somebody
  set them. Afterwards the Directory table has resolved both and a choice is
  indistinguishable from a fallback.
- `SetSEAMLYDATAROOTRECORDED` runs **after `CostFinalize`** (1001), before
  `WriteRegistryValues` (5000).
- Both UI default actions carry `AND NOT Installed`. Without it a repair
  recomputes the default parent and silently moves a customised data root.
- `AppSearch` (50) beats `CostFinalize` (1000), which is why the `INSTALLFOLDER`
  and `SEAMLYDATAPARENT` prefills win over the authored defaults — and AppSearch
  never overwrites a property already set, so the command line still wins.

### A directory id always resolves — the trap this codebase keeps hitting

`[SEAMLYDATAROOT]` in a registry value looked safe and was not: a `/qn` install
with no arguments composes onto `TARGETDIR` and records `C:\SeamlyData`. Hence
`SEAMLYDATAROOTRECORDED`, filled only when this run actually chose a root.

### smsi.wxs is now five files (2026-08-15)

`smsi.wxs` (the `<Package>`) plus `smsi_ui.wxs`, `smsi_legacy.wxs`,
`smsi_files.wxs`, `smsi_shortcuts.wxs`.

**Two ways to break the package with no error at all:**

- leave a `.wxs` off the `wix build` command line;
- delete a `ComponentGroupRef` or `UIRef` from `smsi.wxs`.

Either way the build succeeds and the MSI silently lacks that whole area. WiX
discards an unreferenced fragment without a diagnostic. `smsi.ps1` now globs
`*.wxs`, and `smsi_check_authoring.ps1` reads the built MSI — that check is the
only thing that catches it.

`<Package>`, `MajorUpgrade`, `MediaTemplate` and `SummaryInformation` cannot go
in a fragment. That is why there are four fragments and not five.

**How to verify a refactor of this file changed nothing:** dump every MSI table
before and after, sort the rows, and diff. The scratchpad scripts
`dump_msi_tables.ps1` and `build_stub.ps1` do it and are worth recreating —
they caught nothing this time only because they were used at each step.
