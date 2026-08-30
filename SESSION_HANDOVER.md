# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`packaging/windows/README.md` and `README_MSI_WORKFLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## Layout.8.2 correction — layouts folder sits directly under DataRoot, not seamlyLayout/layouts (2026-08-30)

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
and `INSTALLER_NOTES.md` to match.

**Verified against the real running app, not just the test suite:** rebuilt,
deleted the two stale files again, relaunched `SeamlyLayout.exe`, read the
freshly seeded `qt6_seamlylayout.ini` back — `input_directory`/
`layout_directory` now both read
`C:\Users\susan\Documents\SeamlyData\layouts`. `ctest --preset debug` 5/5
still passed.

## Layout.8.2 resolved — input_directory/layout_directory share one "layouts" folder (2026-08-30)

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
this time, only `PreferencesModel.cpp`, the bundled JSON, the test file, and docs.

## Layout.8 — SeamlyLayout preferences/settings paths fixed under AppConfigLocation (2026-08-30)

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

## version.sh moved to packaging/ (2026-08-29)

`scripts/version.sh` moved to `packaging/version.sh` — it stamps the
version into `projectversion.cpp/.h` and both `Info.plist` files for
every build, so it belongs with build-pipeline scripts, not misc dev
utilities. Updated every reference: `ci.yml` (3 call sites),
`packaging/windows/test_build_msi_local.ps1`, both `Info.plist` comments,
and the `projectversion.cpp/.h` header comments. Script internals
unchanged — all its paths are relative to the repo root, so only the
invocation path changed. Touches `ci.yml` and `packaging/**`, so this
push needs full CI, no skip-ci token.

## Windows packaging moved to packaging/windows/ (2026-08-29)

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

## SeamlyMe Open-dialog fix pushed; CLAUDE.md's local-build claim was stale (2026-08-29)

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

## First run also seeds sample measurement files (2026-08-28)

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

## First run seeds the patterns folder from the bundled samples (2026-08-28)

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

## Legacy data tree backed up as a verified .zip (2026-08-24)

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

## SeamlyLayout's dataRoot now feeds resolvedInputDirectory/resolvedLayoutDirectory (2026-08-24)

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

## SeamlyLayout reads its own mirrored DataRoot key (2026-08-24)

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

## Install-info registry mirrored to SeamlyMe and SeamlyLayout keys (2026-08-24)

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

## InstWinX64.13 - silent-default data root recording fixed, not yet real-machine verified (2026-08-24)

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

## seamly-x64.msi run on a PC with the current suite installed (2026-08-21)

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

## SeamlyLayout INI preferences change (2026-08-20)

SeamlyLayout stores application preferences in
`AppConfigLocation/qt6_seamlylayout.ini`.
On Windows, this resolves to
`%LOCALAPPDATA%\Seamly\SeamlyLayout\qt6_seamlylayout.ini`.

First startup imports the previous `preferences.json` and keeps that file.
JSON remains the format for default preference profiles and layout profiles.

The debug build passed. All five Qt test executables passed.
`cargo test --workspace` passed.

## Seamly2D log-directory change (2026-08-20)

`Application2D::logDirPath()` uses `AppLocalDataLocation` on Windows.
It appends `logs` to produce `%LOCALAPPDATA%\Seamly\Seamly2D\logs`.

`git diff --check` passed. No local build ran because Seamly2D has no local build script.

## The unit tests run on Windows now (2026-08-19)

**`windows-test` builds and runs the four test binaries on every push**, and
both pre-release jobs list it in `needs`. A failing Windows test stops the
rolling `dev-latest` MSIs and the versioned pre-release. It does **not** stop
`windows-msi`: the packages still build and stay downloadable as run artifacts,
which is what you want while diagnosing the failure that blocked the publish.

Separate from `windows-msi` on purpose. Folded in, the tests would sit on the
package-critical path and run twice, once per architecture.

`windows-latest`, x64, `qtmultimedia` only, `-config release` (the MSVC default
is `debug_and_release`, which builds the tree twice), `QT_QPA_PLATFORM=offscreen`.

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

### First result is still unknown (2026-08-20)

The job has never completed. Run 32392386946 would have proved the compile step
but was superseded before it finished — `concurrency.cancel-in-progress` is true,
so the push that added `nmake check` cancelled it. **Compile and test results
therefore arrive together, in the first run after that push.**

If it is red, check the compile step before the test step: a build failure there
means the tests have never compiled on Windows and the breakage is older than
any of this work.

**If a red test blocks an MSI you need**, the packages still build — download them
from the run's artifacts, or drop `windows-test` from `publish-windows-dev`'s
`needs` for one push.

### Diagnosing a red run here

Only `ParserTest.pro` sets `CONFIG += console`. Seamly2DTest, CollectionTest and
TranslationsTest link as **GUI-subsystem** binaries on MSVC, so a failure can
arrive as a bare non-zero exit code with no message in the log. Re-run the
binary by hand with `-o <file>,txt` and print the file. `CollectionTest` also
depends on its working directory — see Gotchas.

## PICK UP HERE (2026-08-19, the installer's answers are the ones the apps use)

Four task branches merged and pushed. HEAD is `f230c638e9`. Full CI ran on each
(no skip token — `scripts/packaging/**` and C++ both changed).

| Task | What shipped |
|---|---|
| InstWinX64.00 | Page 5 defaults to `Documents`; the apps read what Setup recorded |
| InstWinX64.7.10 | The maintenance page names the installed version |
| InstWinX64.2.11 | The program directory survives a major upgrade |
| InstWinX64.7.11 | The registry read moved into `installer_record.{h,cpp}` |

### The one rule that ties them together

**Whatever the wizard shows is what the apps use.** Page 5 promised
`C:\Users\<user>\SeamlyData` and the apps created `<Documents>\Seamly`. Both
halves are fixed: the default parent is the `PersonalFolder` known folder, and
`InstallerRecord::dataRoot()` reads
`HKLM\SOFTWARE\Seamly\Seamly2D\DataRoot` on first run.

Precedence in `VCommonSettings::initializeDataRoot()`, highest first:

1. `paths/dataRoot` in the settings file — an earlier run, or Preferences → Paths.
2. the root Setup recorded.
3. an adopted legacy `~/seamly2d` tree.
4. the built-in default, `<Documents>/Seamly`.

1 above 2 is what stops a machine-wide installer value overriding a user who
moved their root afterwards. Do not reorder these.

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

**Still open, same trap one level up.** The `DataParent` row holds
`[SEAMLYDATAPARENT]` directly. A silent install passing
`SEAMLYDATAROOT=E:\Patterns` and no parent records a `DataParent` composed from
`TARGETDIR`; `SEAMLYDATACHOSEN` then turns true on the next repair and rewrites
the root to `<that parent>\SeamlyData`, losing the chosen name. Narrow — it needs
the documented `SEAMLYDATAROOT=` escape hatch — but real. Fix it the same way:
`SEAMLYDATAPARENTRECORDED` plus a guard.

### Windows Installer cannot move an installed product

Asked whether the Change button could repoint the program or data directory. It
cannot. A product's location is fixed at install time and every component is
registered against it, so a maintenance run ignores `INSTALLFOLDER`. Relocating
means uninstall and reinstall, or a major upgrade — which now prefills both path
pages from `InstallPath` and `DataParent`. Change stays disabled (`ARPNOMODIFY`,
one feature). Documented in `packaging/windows/README.md` and
`README_WINDOWS_BUILD.md`; do not re-litigate.

A data-root change in the installer would also be a lie: the apps read the
recorded value only when `paths/dataRoot` is unset, which stops being true after
the first launch of any app. **Preferences → Paths is where that belongs.**

### Replacing a stock WiX dialog

`SeamlyMaintenanceTypeDlg` replaces the stock `MaintenanceTypeDlg`, because WiX
cannot add a control to a `<Dialog>` another fragment defines. Its silent failure
mode: `VerifyReadyDlg` shows its Repair and Remove buttons on
`WixUI_InstallMode` alone, so the page must set it **before** `NewDialog`. Drop
either row and the wizard reaches the ready page with no enabled button and no
error. Asserted.

Side effect: `BURNMSIMODIFY/REPAIR/UNINSTALL` left `SecureCustomProperties` with
the stock dialog. No effect — they are Burn-only and used only in client-side UI
conditions — but the new page still references them, so a future bundling still
behaves like stock.

### `INSTALLFOLDER` gained `Secure="yes"`

It was not in `SecureCustomProperties`. The wizard sets it client-side and a
perMachine package runs its execute sequence elevated, so a public property must
be listed there to cross that boundary. **Whether the package relied on
undocumented behaviour before is untested** — it needs an interactive install to
a non-default program folder. Worth checking early.

### Not verified — the whole session

Nothing here has been seen on screen or on a real machine:

- an interactive install, upgrade, repair or uninstall;
- the maintenance page and its version line;
- an upgrade that keeps a non-default program directory;
- every C++ change, for the reason in the CI section at the top of this file.

`smsi_check_authoring.ps1` and `wix msi validate` are the only checks that ran,
plus one MSVC syntax pass over the new Qt code.

### Next steps

1. Install the newest `dev-latest` MSI interactively. Confirm page 5 offers
   `C:\Users\susan\Documents\SeamlyData` and that the apps then use it.
2. Re-run the same MSI to reach the maintenance page. Confirm the version line.
3. Install to a non-default program folder, then upgrade, and confirm the apps
   do not move.
4. Decide the CI unit-test gate (top of this file).
5. Fix the `DataParent` trap above.

## Earlier (2026-08-18, Setup creates the data root)

The MSI now creates the selected `SeamlyData` root during installation.
`CreateUserDataRoot` is permanent, so uninstall keeps the folder and its
contents. Its condition prevents an unconfigured silent install from creating
`C:\SeamlyData`.

Link-only x64 and arm64 MSI builds passed. Both authoring checks passed. WiX
validation passed with only expected ICE61. The migration test passed 15
assertions. Full CI run 32197329093 passed. `dev-latest` published both MSIs
from commit `4dd1bcff19`.

The published `dev-latest` MSI needs a real interactive install. Confirm that
`C:\Users\susan\Documents\SeamlyData` exists before an app starts. Confirm that
Windows uninstall keeps the folder.

InstWinX64.01 implements the user's three required cases.

- A fresh install skips the migration page. Setup records and creates the
  selected `SeamlyData` root. The first app launch creates its standard
  directories. Uninstall keeps the root and its contents.
- An old Seamly update reads the legacy `seamly2d` root from application path
  settings. It archives `seamly2d`, extracts it below the selected parent, and
  renames the extracted root to `SeamlyData`.
- A new Seamly update archives `SeamlyData` with that top-level directory. It
  does nothing when the selected location is unchanged.

Both update modes preserve the source, keep existing destination objects, add
missing standard directories, retain non-path settings, and replace path
settings after verification.

AppSearch verifies `seamly2d.exe`, `seamlyme.exe`, and `SeamlyLayout.exe`
before Windows Installer removes the previous program files. Old mode requires
both parent apps and no SeamlyLayout. New mode requires SeamlyLayout.

`SEAMLYDATAPARENT` now reads and writes `HKLM\SOFTWARE\Seamly\Seamly2D\DataParent`.
Page 5 therefore preserves the previous parent during a major upgrade.
`SEAMLYPREVIOUSDATAROOT` keeps the old `DataRoot` available after the registry
component records the new root.

The migration action remains deferred, impersonated, and non-fatal. It can read
the installing user's settings and cloud folders. A data-copy failure cannot
roll back a valid program install.

Verification completed on the task branch:

- `smsi_migrate_user_data_test.ps1`: 15 passed, 0 failed.
- Link-only x64 and arm64 `wix build`: passed.
- `smsi_check_authoring.ps1`: passed for both packages.
- `wix msi validate`: passed for both packages with only expected ICE61.

A real interactive test remains open under Installer.2.1. Test all three cases
with the next `dev-latest` MSI.

**A link-only MSI build is reproducible on this PC.** `wix` 6.0.2 is on `PATH`.
Point `ParentStagingDir` and `ExeStagingDir` at a stub tree holding any files
named `seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe` plus one file in the
parent directory, pass every `-d` `smsi.ps1` passes, and glob **every** `*.wxs`
in `packaging/windows`. That verifies all authoring and both check
scripts without a real build.

## Terminology: "suite", not "family" (2026-08-17)

The three apps are the **Seamly Application Suite**. "Family" is retired. Use
"suite" in prose, "Seamly Application Suite" in user-visible text.

Renamed with it:

- `SeamlyFamilyPaths` → `SeamlySuitePaths` (`src/libs/vmisc/seamly_suite_paths.h/.cpp`).
- `TST_SeamlyFamilyPaths` → `TST_SeamlySuitePaths` (`src/test/Seamly2DTest/tst_seamlysuitepaths.h/.cpp`).
- WiX `ComponentGroup Id="FamilyExecutables"` → `SuiteExecutables`.

Left alone on purpose: `FamilyName` (the measurement surname field), `font-family`,
the `GetDef*Path()` "family" of functions, GPL licence text, the captured logs in
`installation-troubleshooting/`, and the historical `seamly-family.wxs` filename in
old handover and completed-task entries — that file is now `smsi.wxs`.

**Not verified locally:** the qmake half. Seamly2D and SeamlyMe have no local
build, so the `vmisc.pri` / `Seamly2DTest.pro` renames rest on CI. This push runs
the full suite (no skip token) for that reason.

## Earlier (2026-08-15, the MSI installs end to end)

**The rebuilt `dev-latest` MSI completed a full interactive install.**
`InstWinX64.1.6` is closed. All ten pages drew. The data root composed as
`C:\Users\susan\SeamlyData\`, so the trailing-backslash fix holds. Screenshots
are in `project-docs/Install*-Screenshot 2026-08-15 *.png`.

**Next: confirm the three follow-up fixes in a real install.** They are pushed
but only the two `.wxs` ones are verified, and only statically. Download the
rebuilt `dev-latest` MSI and check the data-root page reads "Put the SeamlyData
folder in:" with no ampersand.

### seamly2d.exe and seamlyme.exe went missing after a successful install

**Unexplained. Watch for it.** After the 20:57 install the two parent
executables were absent from `C:\Program Files\SeamlyApps`, while
`SeamlyLayout.exe` and all 1600 harvested files were present.

Ruled out, each by direct evidence:

- The package. `msiexec /a` on that exact MSI extracts all three exes.
- The authoring. `File`, `Component` and `FeatureComponents` rows are correct
  and unconditional; all three exes sit in `WixDefaultFeature`/`INSTALLFOLDER`.
- The sequence. `RemoveExistingProducts` 1401, `RemoveFiles` 3500,
  `InstallFiles` 4000.
- The legacy remover. No `NSIS_Seamly2D` key on the machine, so
  `SEAMLYLEGACYINSTALLDIR` was empty and its component never installed.
- Defender. No detection, no quarantine event.

Windows Installer registered both components as `Installed: Local` with the
right key paths, and logged the install as successful. `MsiGetComponentPath`
returned empty, which is what it does when a registered key path is gone from
disk.

Fixed by an elevated repair: `msiexec /i <msi> REINSTALL=ALL
REINSTALLMODE=vomus`. The repair log says `No existing file`, confirming the
files were genuinely absent rather than skipped by file versioning. A silent
repair without `-Verb RunAs` fails with 1625.

**If it happens again, get a log — that is the missing evidence:**
`msiexec /i seamly-x64.msi /l*v %USERPROFILE%\Desktop\seamly.log`

One contributing factor to remove: the install ran the MSI straight out of
Explorer's zip temp folder
(`InstallSource: ...\Temp\<guid>_seamly-x64.msi (3).zip.c9c\`). Explorer deletes
that folder when the zip window closes. Extract the MSI first.

Two housekeeping items in `project-docs/`:

- `Installer-6-Screenshot 2026-08-15 162124.png` is the *old* "ended
  prematurely" page. It sits in the middle of the new sequence. Delete it.
- The screenshots are untracked. Decide whether they belong in the repository.

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

### Three defects fixed on 2026-08-15 (keep the lessons)

- **`Wix4RemoveFoldersEx` runs at sequence 799, before `CostInitialize`.** Its
  `RemoveFile` rows have to exist in time for costing. Any property it reads
  must therefore be set earlier than that, and cannot expand a directory
  property — those are unresolved until `CostFinalize`. `SEAMLYLEGACYSTARTMENU`
  was set at 1001 with `[AppDataFolder]`; it is now set after `AppSearch` with
  `[%APPDATA]`. **Nothing failed and nothing logged.** The legacy Start Menu
  folder was simply still there.
- **`NoPrefix="yes"` turns accelerator parsing off**, so an `&` in that label
  prints. Removed from `FolderLabel`.
- **SeamlyLayout logged into the install directory on Windows.** Now the
  `AppConfigLocation` root, like macOS and the AppImage.

**`smsi_check_authoring.ps1`: `Get-MsiRows` returns `, $rows`. Assign it
directly — never `@(Get-MsiRows ...)`.** The wrapper gives an array holding an
array. Single-row queries still work, because PowerShell unwraps a one-element
array on a cast or member access, so the trap stays hidden until a query first
returns more than one row. 120 assertions pass.

**Rust lives in `C:\Users\susan\.cargo\bin` and is NOT on the default PATH.**
Neither is Qt's `bin`. Set both before building or testing SeamlyLayout by
hand:

```powershell
$env:PATH  = "C:\Qt\6.11.1\msvc2022_64\bin;$env:USERPROFILE\.cargo\bin;$env:PATH"
$env:QMAKE = 'C:/Qt/6.11.1/msvc2022_64/bin/qmake.exe'
```

Each omission fails in a way that names the wrong cause: Corrosion's
`FindRust` without cargo, a `QtMissing` panic in `cxx-qt-build` without
`QMAKE`, and `STATUS_DLL_NOT_FOUND` from the `cxxqt_bridge` test binary
without Qt's `bin`. Written up in `.github/README-BUILDS.md` and
`src/app/seamlylayout/.claude/rules/testing.mdc`.

Check the directory before concluding Rust is missing — `Get-Command cargo`
alone gives a false negative.

**`build.ps1` no longer fails on cargo's progress output.** Windows PowerShell
5.1 wraps a native program's stderr in an ErrorRecord, and
`$ErrorActionPreference = "Stop"` made it terminating, so `Compiling serde
v1.0.228` ended the script. Native calls now go through `Invoke-NativeCommand`,
which relaxes the preference and judges by exit code, and the batch file merges
stderr with `2>&1` so the log stays readable. Verified both ways: a clean build
exits 0, and a deliberate syntax error still fails with the compiler
diagnostics visible.

### The 2343 defect (fixed, keep the lesson)

Fixed and pushed (`a843503ab7..6bde38b1b3`).

2343 is "specified path is empty". `SeamlyDataDirDlg`'s path box carried
`Indirect="yes"`. **An indirect `PathEdit` reads its property to get the NAME of
the property that holds the path.** Stock `InstallDirDlg` is indirect only
because `WIXUI_INSTALLDIR` holds the string `INSTALLFOLDER`. `SEAMLYDATAPARENT`
holds the path itself, so the lookup asked for a property named
`C:\Users\<user>\`, found nothing, and aborted the install while the page was
being created. Remember this before copying an `Indirect` attribute off a stock
dialog.

Two more defects fixed on the way, both on the same page:

- Next had no `SetTargetPath`, so an edited parent never reached the Directory
  table and `[SEAMLYDATAROOT]` would not have recomposed. It runs before
  `NewDialog`, conditional on a non-empty property (the other route to 2343).
  Deliberately no `CheckTargetPath` — the data root may be a cloud or removable
  drive.
- The `SEAMLYDATAPARENT` default had no trailing backslash. Windows Installer
  appends a child directory verbatim, so `C:\Users\me` gave
  `C:\Users\meSeamlyData`.

`smsi_check_authoring.ps1` gained two assertions and now runs 118. Verified with
a link-only `wix build` over a stub staging tree.

## Earlier (2026-08-15, the MSIs publish to a rolling pre-release)

**Task InstWinX64.0.3 is done, and it needs one real CI push to prove it.**
`ci.yml` gained a `publish-windows-dev` job. Every push to `run-seamlyLayout`
deletes the `dev-latest` release, recreates it on the pushed commit, and uploads
`seamly-x64.msi` and `seamly-arm64.msi` as raw release assets.

- **The task as written was impossible.** GitHub serves every workflow artifact
  as a zip. `actions/upload-artifact` has no option to return a bare file, so
  the artifact named `seamly-x64.msi` downloads as `seamly-x64.msi.zip`. A
  release asset is the only raw `.msi` GitHub hands back. Do not reopen this.
- The job deletes and recreates instead of editing, because GitHub pins a tag to
  its creation commit and no `gh` command moves it.
- It depends on `windows-msi` only, and carries no Linux or macOS file.
- The versioned `publish` job is untouched.
- **Untested on a runner.** Watch the first run for two things: the
  `gh release delete ... || true` on the very first push, which has no
  `dev-latest` to delete, and the `GITHUB_TOKEN` permission (workflow-level
  `contents: write` should cover it).

**`TODO_INSTALLER_WIN_X64.md` had two tasks numbered `InstWinX64.0.3` and two
numbered `InstWinX64.0.4`.** The Application-preferences pair is renumbered to
`0.5` and `0.6`. Check for a collision before adding a task number to that file.

## Earlier (2026-08-15, the local build and test scripts are deleted)

**`scripts/sb.ps1`, `scripts/sd.ps1` and `scripts/st.ps1` are gone.** The user
decided the suite needs no local release build, no local debug build, and no
local test runner. `ci.yml` is the verification path for seamly2d and seamlyme.
SeamlyLayout keeps its own local scripts (`qd.ps1`, `build.ps1`) and its local
`ctest`/`cargo test`.

Consequences to keep in mind:

- **A skip-ci push now defers *all* verification for the parents.** Nothing runs
  locally to catch a broken build first. **The user kept the skip token as the
  default anyway (2026-08-15)** and verifies releases with a manual
  `gh workflow run ci.yml --ref run-seamlyLayout`. `ci.yml` already carries a
  `workflow_dispatch` trigger. Do not reopen this.
- **`smsi.ps1` has no local producer for its input trees.** It only packages.
  Build the trees by hand, or let `ci.yml`'s `windows-msi` job do the whole job.
- **The deployed-runtime FileVersion check went with `sb.ps1`/`sd.ps1`.** Read
  `Qt6Core.dll`'s FileVersion by hand before trusting a hand-built tree.
- `scripts/seamly2d-debug/` (2.1 GB) was deleted from disk, and its `.gitignore`
  entry with it, along with the older `scripts/seamly2d-build-debug/` spelling.
  `scripts/seamly-msi/` and `build/` stay ignored by name.

## Earlier (2026-08-15, smsi.ps1 is CI-only)

**`smsi.ps1` no longer builds locally, no longer builds a two-app package, and
no longer detects anything.** Three removals, all done:

1. **Local-build mode.** `-Version`, `-Seamly2DBin`, `-SeamlyMeBin` and
   `-WinDeployQt` are `Mandatory`. `Find-WinDeployQt6` (CMakeCache + `C:\Qt`
   scan) is deleted; `Find-CrtDirectory` reads `VCToolsRedistDir` only, with no
   Visual Studio scan. Each removed fallback could ship a runtime nothing in the
   package was built against.
2. **`-NoSeamlyLayout`.** Gone, with the `IncludeSeamlyLayout` define, the two
   `<?ifdef?>` guards in `seamly-family.wxs`, and `-ExpectSeamlyLayout` in
   `test_msi_authoring.ps1` (its SeamlyLayout assertions are unconditional now).
   Both architectures have shipped all three apps since 2026-08-11.
3. **`windeployqt6`.** The project uses the unsuffixed `windeployqt` everywhere:
   the parameter is `-WinDeployQt`, `ci.yml` passes
   `"$env:QT_ROOT_DIR\bin\windeployqt.exe"`, and
   `src/app/seamlylayout/packaging/windows/build_installer.ps1` was renamed to
   match. A Qt 6 kit ships both names, so this is a naming choice, not a
   behaviour change.

**Verified with the stub-staging-tree trick** (see below): `wix build` clean,
`wix msi validate` clean except the expected ICE61, `test_msi_authoring.ps1`
all assertions pass including the three SeamlyLayout ones. The next real CI run
is what proves the `-WinDeployQt` rename against the runners' Qt kits — the
change is untested on a runner.

## Earlier (2026-08-12, custom dialog set)

**Tasks InstWinX64.0.2 and InstWinX64.1.1–1.5 are done.** The user confirmed the
x64 MSI builds in CI, and `seamly-family.wxs` now defines its own dialog set
instead of using `WixUI_InstallDir`.

**The SpawnDialog blocker is gone.** A dialog set owns every `NewDialog` row; a
stock dialog carries only its own internal events. So replacing the set — rather
than publishing against it — makes every transition ours, and the Order 4
`NewDialog VerifyReadyDlg` row that could not be excluded no longer exists.

Chain: `WelcomeDlg` → `LicenseAgreementDlg` → `SeamlyPreviousInstallDlg` (only
when an earlier install is found) → `InstallDirDlg` → `SeamlyDataDirDlg` →
`SeamlyDataMigrateDlg` → `SeamlyShortcutsDlg` → `VerifyReadyDlg` → `ProgressDlg`
→ `ExitDialog`. Back reverses every arrow.

**Two findings worth keeping.**

1. `DialogRef` order decides the `InstallUISequence` numbers of `ResumeDlg`,
   `WelcomeDlg`, `MaintenanceWelcomeDlg` and `ProgressDlg` (1296–1299). Listing
   `WelcomeDlg` first put it at 1296 and pushed `ResumeDlg` to 1298, which would
   show the welcome page to a user resuming an install. The order is now
   commented as load-bearing and the test pins the numbers.
2. `WixUI_Common` supplies the bitmaps but **not** `WixUI_Font_Normal`,
   `WixUI_Font_Bigger` or `WixUI_Font_Title`. A custom set has to define them,
   plus `DefaultUIFont`, `WIXUI_INSTALLDIR` and `ARPNOMODIFY`.

**Local verification is now cheap and was done.** A stub staging tree (three
empty `.exe` files and two files for the runtime) links the real authoring in
seconds: `wix build` clean, `wix msi validate` clean except the expected ICE61,
`test_msi_authoring.ps1` 115 assertions pass. It proves the authoring, not the
product.

**InstWinX64.1.7 is done too.** `README_MSI_WORKFLOW.md` and
`packaging/windows/README.md` carry the new page order, and the
"SeamlyShortcutsDlg never displays" defect note is gone. The README also claimed
the old NSIS installation is never removed automatically; Setup has removal
components for it, so that claim was corrected.

**Next: InstWinX64.1.6** — an interactive install on the test laptop. Every page
must display, in order, and Back must return to the previous page. It is the
only part of Task InstWinX64.1 that local checks cannot cover.

## CI: one workflow (2026-08-12)

`.github/workflows/seamlylayout-ci.yml` is deleted. **`ci.yml` is the only
workflow that builds the suite on GitHub.** It already built seamlyLayout in
the `windows-msi` job, so the second workflow duplicated the build and carried a
second `QT_VERSION`.

**Coverage lost, on purpose:** seamlyLayout's `ctest` and `cargo test
--workspace` suites no longer run in CI, and seamlyLayout is no longer built on
Linux. Run both locally before merging seamlyLayout work. To restore the
coverage, add the two test steps to `ci.yml`.

Docs updated: `README_WORKFLOWS.md`, `README-BUILDS.md`, `CLAUDE.md`,
`src/app/app.pro`, `UNIT_TEST_COMMANDS.md`, `TODO_MIGRATE.md`,
`TODO_RENAME_SETTINGS_FILES_CLASSES.md`. `TODO_COMPLETED.md` keeps the Task 20
entry as the record of what was built at the time.

## Earlier (2026-08-11, data-root append + SpawnDialog investigation)

> The SpawnDialog blocker described below was fixed on 2026-08-12 by the custom
> dialog set. Kept for the reasoning that led there.

**Done: the data root appends a fixed leaf.** `SEAMLYDATAPARENT` is what the
user picks (default `%USERPROFILE%`); `SEAMLYDATAROOT` is that parent plus a
fixed `SeamlyData` leaf. `E:\` gives `E:\SeamlyData`. Setting `SEAMLYDATAROOT`
directly still overrides the composition. 96 authoring assertions pass, two new
ones pinning the composition. Merged and pushed (`4a8c0a07dc..37fb33f1f9`).

**Not done: the SpawnDialog defect.** The built-in `InstallDirDlg` Next rows
occupy Orders 1, 3 and 4. Below the Order 4 `NewDialog` only 0 and 2 are free —
two slots for three pages. The ties are forced. No arrangement of `Ordering`
values can fix them; the mechanism has to change, not the numbers.

Orders 5-7 was tried and **reverted**. It removes every tie, but it contradicts
a deliberate assertion in `test_msi_authoring.ps1` ("SpawnDialog must have a
lower Ordering than every NewDialog"), and only an interactive install can
settle which reading is right. The tree is back to Orders 1-3, with the finding
in the comment beside the `Publish` rows.

**Also this session:** `.claude/settings.json` gained the nuget and dotnet
sandbox domains, so the WiX toolchain can be installed. `Bash`/`PowerShell`
rules were already maximal; `permissions.defaultMode` remains unset.

### Earlier the same day

## Earlier (2026-08-11, installer directories session)

**Tasks InstWinX64.1.1 and 1.2 — authored, and 1.2 is BLOCKED on a defect that
was already in the tree.** Read the blocker note in `TODO_INSTALLER_WIN_X64.md`
before continuing.

**1.1 was mostly already built.** `C:\Program Files\SeamlyApps` was the existing
default (`ProgramFiles64Folder` + `Name="SeamlyApps"`), and every shortcut,
association and registry value already resolved through `[INSTALLFOLDER]`. The
user asked about `Program Files (x86)`, then decided to keep the current path —
correctly, since that tree is for 32-bit programs and every binary here is x64
or arm64. What was genuinely missing was the cloud-folder rejection (1.1.3),
now a `Launch` condition covering OneDrive, Dropbox, Google Drive, iCloud and
Box Sync. It is a launch condition rather than a dialog check because that is
the only form that also blocks a silent `/qn` install.

**1.2 was built to "installer does everything"** — the user chose that split
over "installer records, apps copy". So the MSI now carries `SEAMLYDATAROOT`,
`SEAMLYCOPYUSERDATA`, two dialogs, a registry breadcrumb, and a deferred
impersonated custom action running `seamly_copy_user_data.ps1`.

**THE BLOCKER.** The two new dialogs use `SpawnDialog` from `InstallDirDlg`'s
Next — the same mechanism as `SeamlyShortcutsDlg`, which
`README_MSI_WORKFLOW.md` already records as **never displaying**. Dumping the
built MSI's `ControlEvent` rows proved there is no alternative: the built-in
`NewDialog VerifyReadyDlg` is at Order 4 with condition `1`, so no competing
`NewDialog` can be excluded. The dump also exposed two ordering collisions this
task introduced (`SeamlyDataDirDlg` ties with `CheckTargetPath` at Order 1;
`SeamlyShortcutsDlg` ties with `SetTargetPath` at Order 3). The full chain is
transcribed in a comment beside the `Publish` rows in `seamly-family.wxs`.

Consequence: the **properties work** (an unattended install passing
`SEAMLYDATAROOT` / `SEAMLYCOPYUSERDATA` behaves correctly, defaults apply
otherwise) but the **interactive prompting does not**. 1.2.1-1.2.3 are left
unticked on purpose. Fixing the SpawnDialog defect is the tracked subtask in
`TODO_MIGRATE.md` and must come first.

**Why there is no rollback action for the copy** — a deliberate deviation from
the option text the user picked, flagged at the time and worth keeping: undoing
the copy could only mean deleting files from a folder that may already have held
the user's work, and nothing can tell those apart. The copy is additive-only and
never overwrites, so nothing is left inconsistent. `Return="ignore"` likewise: a
file-copy problem must not roll back a good program install.

**Verification done locally.** `wix build` + `wix msi validate` (with the same
`-sice ICE43 -sice ICE57` the real build uses) are clean, only the expected
ICE61. `test_msi_authoring.ps1` gained ~25 assertions and all 94 pass. The copy
script was run against a real tree: subfolders preserved, an existing
destination file left byte-for-byte, source untouched, second run copies 0.

**MACHINE STATE CHANGED OUTSIDE THE REPO.** The user installed .NET SDK 9
(`Microsoft.DotNet.SDK.9`, 9.0.316) and I installed the WiX v6 global tool
(`wix` 6.0.2) plus `WixToolset.UI.wixext` and `WixToolset.Util.wixext` at the
matching version. `dotnet` is NOT on `PATH` — prepend
`$env:USERPROFILE\.dotnet\tools` and `$env:ProgramFiles\dotnet`. This makes a
`.wxs` change checkable in seconds instead of a ~50-minute CI round trip. Undo
with `dotnet tool uninstall --global wix`.

**One open question for the user**, recorded in the task file: `SEAMLYDATAROOT`
holds the whole path, so choosing `E:\` gives `E:\`, not `E:\SeamlyData`.
Auto-appending the leaf would turn a typed `E:\SeamlyData` into
`E:\SeamlyData\SeamlyData`. Confirm which is wanted.

### Earlier the same day

## Earlier (2026-08-11, later session)

**Task InstWinX64.1.3.2 is done — `windows-msi.yml` is deleted.** `ci.yml`'s
`windows-msi` matrix job already built both architectures and fed `publish`, so
the packaging-only workflow only duplicated the work: its push trigger on
`packaging/windows/**` built both MSI packages a second time on every
`.wxs` or `smsi.ps1` edit. Its copy of the build steps had also drifted — it
signed `Seamly2D-<arch>.msi`, a name `smsi.ps1` has never written, so that
signing step had never touched a real file.

**Consequence to expect:** an edit under `packaging/windows/` now runs
the full ~50-minute suite instead of a path-filtered packaging job. That is the
accepted trade for one copy of the steps.

**Over twenty references had to be redirected, not four.** They were spread
across `.github/README-BUILDS.md`, `.github/workflows/README_WORKFLOWS.md`,
`common.pri`, `scripts/sb.ps1`, `packaging/windows/` (README.md,
README_WINDOWS_BUILD.md, smsi.ps1, seamly-family.wxs, test_msi_authoring.ps1),
`CLAUDE.md`, `seamlylayout-ci.yml`, `qt-arm64-module-probe.yml`, and four
`TODO_*.md` files. Completed-task records in `TODO_COMPLETED.md`,
`TODO_INSTALLER.md` and `TODO_INSTALLER_WIN_ARM64.md` keep the old name on
purpose — they describe what was true at the time.

**Stale claims corrected while redirecting.** Several packaging documents still
said the arm64 MSI ships the parents only (`-NoSeamlyLayout`) and that arm64
cross-compiles with `host-qmake`. Both are false since 2026-08-11: all three
apps build natively on `windows-11-arm`. `TODO_CODE_SIGNING.md` also cited a
`ci.yml` step named "Print installer signature" that went with the NSIS
`windows` job; CodeSign.1.5 now says the step has to be written, not mirrored.

**`qt-arm64-module-probe.yml` is also deleted** (by the user, same session). It
had answered its question — Qt 6.11.1 does publish an arm64 WebEngine — and the
subtask asking to re-run it at every Qt bump was removed first. Four documents
still pointed at it, two in the present tense; each now carries the `aqt
list-qt` command the workflow ran, so the check survives the file:
`aqt list-qt windows_arm64 desktop --modules <version> <arch>`, and the same for
the `windows` host.

`TODO_CODE_SIGNING.md` was updated by the user in the same session.

**Also this session:** `ci.yml`'s `paths-ignore` gained `.claude/**` and
`.vscode/**`. A push of documentation plus a `.claude/settings.json` edit had
started the full suite, because the filter skips a push only when *every* file
it carries matches. `CLAUDE.md`'s docs-only exception was corrected at the same
time — it told the reader to commit locally and never push, and it wrongly
counted `.txt` and `.svg` as documentation (`CMakeLists.txt` is a build input;
an `.svg` can be a compiled Qt resource).

**Verification status:** no code changed, so there was no build to run. The
edited PowerShell scripts and `seamly-family.wxs` were parse-checked. The
`ci.yml` YAML edit is unverified locally — the push that carries it is the
first real check, and the skip token was deliberately omitted for that reason.

### Previous session (2026-08-11)

**Task InstWinX64.0 — the x64 MSI builds clean on CI.** Runs `31461308276`
(CI) and `31461308379` (Windows MSI) on `361b743fa0` both finished **green**,
including `Windows: Build MSI (x64)`. `wix msi validate` passed,
`test_msi_authoring.ps1` passed, and `seamly-x64.msi` (163.5 MB) uploaded with
all three apps in it. So the three changes those runs were the first to
exercise — the edited `ci.yml`, the repaired authoring assertions, and the
Nsis→Legacy rename — are all verified. Nothing in `ci.yml` or the packaging
scripts needed a fix. Details, including the two harmless warnings (ICE61 from
`AllowSameVersionUpgrades`, and windeployqt's missing `Qt6SerialPort.dll` for
the unused NMEA plugin), are written up under Task InstWinX64.0 in
`TODO_INSTALLER_WIN_X64.md`.

**The held-back commit is pushed.** `a0e70635a7` and `064a1e49dd` went to
origin once those runs finished; `run-seamlyLayout` is at `064a1e49dd` on both
sides. That push started runs `31538442167` (CI) and `31538442086` (Windows
MSI) — a re-verification of a comment-only `.wxs` change, so a failure there
would be new and unrelated to the MSI authoring.

**The one thing left in Task InstWinX64.0** is its second subtask: the user has
to confirm the x64 `.msi` built without error. Only then does the task move to
`TODO_COMPLETED.md`.

Everything below is the state as shipped this session.

## Current state (2026-08-11): CI cost control + MSI authoring check repaired

Three changes merged to `run-seamlyLayout`; the branch was force-pushed once
(see the skip-token gotcha below for why).

**1. CI no longer runs on every push.** `ci.yml` gained a top-level `concurrency`
(cancel-in-progress) and a `paths-ignore` for `**.md` / `project-docs/**` /
`LICENSE` on its push trigger (since extended with `.claude/**` and
`.vscode/**`); `seamlylayout-ci.yml` already had both. `CLAUDE.md`'s task workflow now merges with `--no-ff` (step 8)
and puts the skip token in the merge commit by default (step 9), omitting it
when the task touched `.github/workflows/**`, `scripts/packaging/**`,
`*.pro`/`CMakeLists.txt`/`Cargo.toml` or platform-specific code — the things the
local debug build could not verify. That script is gone since 2026-08-15, so a
skip-ci push now defers verification of everything, not only those paths.
Accumulated skips get cleared with `gh workflow run ci.yml --ref run-seamlyLayout`
before a milestone.

**GOTCHA that cost a push:** GitHub matches the skip token anywhere in the head
commit message, including a sentence *saying the token is absent*. The first
version of that merge commit explained the new rule, contained the literal
token, and skipped the very run it was describing — only CodeQL (default setup,
not governed by the token) ran. Both commits were rewritten to spell it
"skip token" in prose and the branch was force-pushed with `--force-with-lease`.
Never write the literal token in a commit message.

**2. The MSI authoring check was failing, and not because of anything above.**
`546e9d5def` (2026-08-03, "fixed issues in build files related to directories,
filenames, etc.") hand-edited `seamly-family.wxs` and left
`test_msi_authoring.ps1` asserting the old values, so three assertions failed on
every push since. Fixed in the `.wxs`, test left alone, because the test encodes
Task 51's documented requirements and `SeamlyApps` is the name used by
`README-BUILDS.md`, `packaging/windows/README.md`,
`README_MSI_WORKFLOW.md`, `TODO_INSTALLER_WIN_X64.md` and the Task 51 test kit:

- `INSTALLFOLDER` `Name="Seamly"` → `Name="SeamlyApps"`.
- `UserDataText` names the user-data folder again (`C:\Users\your name\seamlyData`).
- `NsisText` (now `LegacyInstallText`, see below) regained the deleted sentence
  "Nothing else in that folder is kept, so move anything of your own out of it
  before continuing." That one was a real data-loss warning, not just a test
  mismatch — the dialog still says Setup removes the whole legacy *program
  directory*. Kept to 338 chars so it fits the unchanged `Height="50"` control;
  the pre-`546e9d5def` text was 337.

Verified locally by parsing the `.wxs` and running the six assertions' regexes
against the real attribute values (all pass, XML well-formed). The MSI itself
still only builds on CI.

**3. `Nsis` identifiers renamed to `Legacy`.** NSIS is a build tool this project
stopped using when the `windows` job was retired, so identifiers named for it
read as though the package still produces one. What they actually name is the
pre-MSI Seamly2D already sitting on a user's machine, which outlives the tool
that authored it. Renamed across `seamly-family.wxs`, `test_msi_authoring.ps1`,
both copies of `test_msi_install.ps1` and `README_MSI_WORKFLOW.md`:

| was | now |
|---|---|
| `SEAMLYNSISUNINSTALLSTRING` / `SEAMLYNSISINSTALLDIR` / `SEAMLYNSISSTARTMENU` | `SEAMLYLEGACY…` |
| `SeamlyNsisUninstallStringSearch` / `SeamlyNsisInstallDirSearch` | `SeamlyLegacy…` |
| `RemoveNsisProgramFiles` / `RemoveNsisRegistryKeys` | `RemoveLegacy…` |
| `RemovedNsisInstallDir` / `RemovedNsisRegistry` (registry breadcrumbs) | `RemovedLegacy…` |
| `NsisText` | `LegacyInstallText` |
| `Get-NsisInstallDir`, `$state.NsisInstallDir` | `Get-LegacyInstallDir`, `…LegacyInstallDir` |

**Two things deliberately NOT renamed.** `SOFTWARE\NSIS_Seamly2D` is a real key
name the old installer wrote — renaming it would make the MSI stop finding the
product it exists to remove. And prose still says NSIS where it names the actual
historical product; only identifiers moved. A `NAMING:` comment in the `.wxs`
records both rules.

`SecureCustomProperties` needed no manual edit — WiX generates it from
`Secure="yes"`, and `test_msi_authoring.ps1` asserts both renamed properties
appear in it.

**DECIDED with the user 2026-08-11: `RemovedLegacyInstallDir` and
`RemovedLegacyRegistry` are frozen — never rename them again.** They are
component KeyPaths on components with no explicit `Guid`, so WiX derives the
component GUID from the Id plus the value name. This rename spent that change
once (on upgrade the old value is removed and the new one written, which is
correct); repeating it leaves users' machines accreting orphaned breadcrumbs
under `SOFTWARE\Seamly\Seamly2D`. Recorded as `FROZEN NAME` comments at both
components in `seamly-family.wxs` — that is commit `a0e70635a7`, the unpushed
one named at the top of this file.

Verified locally: `.wxs` parses as XML, every `Legacy` identifier referenced by
`test_msi_authoring.ps1` exists in the `.wxs`, the six repaired assertions still
pass, and all four `.ps1` files parse with zero errors. The authoring check
itself runs only on CI.

## Superseded (2026-08-10): arm64 MSI build fixed — `windeployqt --qtpaths` removed

**Branch `task-arm64-windeployqt`, off `run-seamlyLayout`.**

**The failure.** `Windows: Build MSI (arm64)` died in the `nmake` leg:

```text
windeployqt.exe --qtpaths ...\msvc2022_arm64\bin\host-qtpaths.bat bin\seamlyme.exe
Error: "...\bin\host-qtpaths.bat" does not exist.
NMAKE : fatal error U1077 ... return code '0x1'
```

**Root cause.** `host-qtpaths.bat` is generated only for a **cross-compiled** kit
(`win64_msvc2022_arm64_cross_compiled`), whose x64 `windeployqt` cannot infer an
arm64 target's paths. Commit `fba962c4d8` moved arm64 onto the native
`windows-11-arm` runner with `win64_msvc2022_arm64`, where that wrapper does not
exist — but the `win32-arm64-msvc` block in the `.pro` files still passed
`--qtpaths` unconditionally.

**Fix — one shared qmake helper, `deployQtRuntime()` in `common.pri`.** All three
MSVC targets (`seamly2d.pro`, `seamlyme.pro`, `Seamly2DTest.pro`) had their own
hand-maintained copy of the windeployqt post-link block, which is exactly how
they drifted. They now all call the one helper, which runs
`qtPrepareTool(WINDEPLOYQT, windeployqt)` and invokes it **bare** — identically
for `win32-msvc` and `win32-arm64-msvc`. No probe, no `--qtpaths`: every Windows
leg is native, so `windeployqt` always belongs to the kit being deployed and
resolves its own paths. **Restore `--qtpaths` only if a cross kit is ever
reintroduced** (the comment block in `common.pri` says so).

**Also fixed, found on the way:** `tst_svgcomponenttags.cpp` called
`VLayoutPiece::SetCountourPoints()`, which does not exist (typo, and no such
setter) — it broke the whole `Seamly2DTest` target. The real setter behind
`getContourPoints()` is `setMainPathPoints()`. Pre-existing since `0dcdc3d35d`;
CI's MSI legs pass `CONFIG+=noTests` so they never hit it, but `linux-test` runs
`make check` on PRs and would have.

**Verified locally:** `scripts/sd.ps1` green; all three targets post-link
`C:\Qt\6.11.1\msvc2022_64\bin\windeployqt.exe bin\<app>.exe` with `Qt6Cored.dll`
+ `platforms\` deployed beside each exe; `scripts/st.ps1` = **32139 passed, 0
failed across 25 suites**. The arm64 path itself is **only provable by a CI
run** — no arm64 cross tools on this PC.

**Docs brought in line with the native-runner reality** (they still described
arm64 as cross-compiled and two-app): `README-BUILDS.md`,
`README_WORKFLOWS.md`, `README_WINDOWS_BUILD.md`, `ci.yml` (one comment was also
truncated mid-sentence), `windows-msi.yml`, and `TODO_INSTALLER_WIN_ARM64.md` —
where **InstWinArm64.1 is now DONE** (only .1.5, the three-app authoring-test
re-run, is open) and **InstWinArm64.3 is DROPPED**: the from-source Qt WebEngine
build existed solely to work around "Qt ships no arm64 WebEngine", which is Qt
6.8-era and false for 6.11.1.

**Next step:** push and confirm both MSI legs go green.

## Superseded (2026-08-11): Task Installer.1.2 — NSIS retired, arm64 ships an `.msi`

> The "arm64 still ships two of three apps" statement below was overtaken by
> commit `fba962c4d8` — arm64 now ships all three apps, built natively.

**Branch `task-installer-win-arm64-msi`, off `run-seamlyLayout`.**

Windows now ships **MSIs only**. `ci.yml`'s `windows` job — the last NSIS
producer, arm64-only since Installer.1.1 — is deleted, and `windows-msi` became
a **matrix over `arch`** (`x64`, `arm64`, `fail-fast: false`), which is
`windows-msi.yml`'s `msi` job verbatim minus its own version step. `publish`
releases `seamly-x64.msi` + `seamly-arm64.msi` and no longer needs the `windows`
job.

**Why NSIS could go at all:** the stated reason for keeping it (no arm64
SeamlyLayout build) never justified NSIS. `windows-msi.yml` has always built an
arm64 MSI with `smsi.ps1 -NoSeamlyLayout`, carrying seamly2d + seamlyme —
*exactly* the two apps the arm64 NSIS package carried. The format swap loses
nothing; shipping SeamlyLayout on arm64 is a separate, still-open problem.

**arm64 still ships two of three apps.** SeamlyLayout needs an
`aarch64-pc-windows-msvc` Rust + cxx-qt build and an arm64 Qt WebEngine (for
`SvgCanvas.qml`), neither of which exists. Tracked in the newly written
`TODO_INSTALLER_WIN_ARM64.md` as InstWinArm64.1, with the exact three-line
change to both workflows when it lands.

**`dist/seamly2d-installer.nsi` was deleted** (Task InstWinX64.11.1). It was
first kept unbuilt as the record of what a pre-MSI installation left on disk.
Task InstWinX64.11.2 transcribed that footprint into `smsi.wxs`, above the
`RemoveLegacyProgramFiles` component, so the MSI's removal authoring keeps its
record and the file could go.

`-WinDeployQt6` was deliberately not passed on the arm64 leg while that leg
shipped two apps. **Superseded twice since:** both legs pass the deploy tool,
and on 2026-08-15 the parameter was renamed `-WinDeployQt` and `$includeLayout`
was removed with `-NoSeamlyLayout`.

Docs updated: `README-BUILDS.md`, `README_WORKFLOWS.md`, `TODO_INSTALLER.md`
(Installer.1.2 checked off), `TODO_INSTALLER_WIN_ARM64.md` (written from a stub),
`TODO_MIGRATE.md` M.12 arm64 row, `TODO_CODE_SIGNING.md` CodeSign.1.7 (closed as
moot — it was explicitly conditional on NSIS still being the released installer).

**Verification is the CI run for this commit** — there is no YAML tooling on this
PC and no arm64 hardware. Confirm both MSI legs go green and that
`seamly-arm64.msi` uploads.

## Also 2026-08-11: CI version numbers could be octal C++ literals — fixed

**Branch `task-ci-version-octal`, off `run-seamlyLayout`.**

Run [`31447296387`](https://github.com/seamly/Seamly2D/actions/runs/31447296387)
failed **all four build jobs** (macOS, Linux AppImage, Windows NSIS arm64,
Windows MSI x64) on the same error:

```text
projectversion.cpp:67:43: error: invalid digit '8' in octal constant
   67 | extern const int SUPER_MINOR__VERSION = 048;
```

Not a code regression — a **time-of-day bug**. `ci.yml:35` and
`windows-msi.yml:99` build the version with `date +%Y.%-m.%-d.%-H%M`; the run
started at 00:48 UTC, so the fourth component was `048`, and `scripts/version.sh`
substituted it verbatim into `projectversion.cpp` as a C++ integer literal. A
leading zero makes that literal **octal**: it fails to compile when the minutes
contain an 8 or a 9, and — more quietly — compiles to the *wrong number*
otherwise (`047` is 39). Any build started in the 00:00 UTC hour was affected.

**Fix is in `scripts/version.sh`, not the workflows**, so it covers both YAML
call sites at once: every component is validated as numeric and normalized with
`$((10#…))`, and `VERSIONSTR` is rebuilt from the normalized parts so
`VER_FILEVERSION_STR` and the two `Info.plist` files agree with the integers. A
non-4-part or non-numeric argument is now rejected instead of corrupting the
source files.

`scripts/version_test.sh` is new: it drives `version.sh` over the release
case, the `048` regression, the silently-wrong `047` case and an all-zero
component, asserts no `= 0[0-9]` literal is ever written, checks both rejection
paths, and backs up/restores all four files it rewrites. **26/26 assertions
pass.** No local qmake build was run — no C++ changed, and the emitted
`projectversion.cpp` is restored byte-for-byte by the test.

## Verification happens on GitHub CI, not on this PC

User's instruction, 2026-08-10: concentrate on the CI pre-releases and ignore
local MSVC/qmake builds. That also settles what to do about the broken Visual
Studio install below — nothing, for now.

**Read that together with the unit-test section at the top of this file.** CI
proves Seamly2D and SeamlyMe BUILD. It compiles no test and runs no test on a
push, so "CI verifies it" is only ever a claim about the build.

A plain push run **skips `publish`** — only a `schedule` or `workflow_dispatch`
run on `run-seamlyLayout` exercises the pre-release path, so dispatch one to
prove the release step end to end.

**`gh` is installed and authenticated** (2026-08-11), at
`C:\Program Files\GitHub CLI\gh.exe`. Read run state with it directly —
`gh run list --repo seamly/Seamly2D --branch run-seamlyLayout --workflow ci.yml`,
then `gh run view <id>` and `gh run view --job <id> --log-failed`.

## Follow-up left open deliberately

`.github/README.md`'s "Windows 64-bit" download badge still points at upstream's
`Seamly2D-windows.zip`. It has to become `seamly-x64.msi`, but only when the
migration is pushed upstream — the badges link to
`FashionFreedom/Seamly2D/releases/latest`, so editing them now breaks the live
public download link. Tracked as **Task M.12** in `TODO_MIGRATE.md`.

## MACHINE STATE: the Visual Studio installation is broken — and that is now accepted

**The user's decision (2026-08-10): build and verify on GitHub CI; do not try to
build locally with MSVC.** So this section is background, not a blocker. Do not
spend a session repairing Visual Studio unless the user asks.

The local build scripts were deleted on 2026-08-15, so nothing in the repository
depends on this any more. Keep the findings — a hand-built local tree hits the
same wall. The symptom was `'cl' is not recognized`. Not the script, and not the
agent sandbox — the same failure occurs with the sandbox disabled:

- `vcvars64.bat` **and** `vcvarsall.bat x64` exit 1 with
  `[ERROR:VsDevCmd.bat] *** VsDevCmd.bat encountered errors ***`; three
  sub-scripts fail to init — `core\msbuild.bat`, `ext\cmake.bat`,
  `ext\ConnectionManagerExe.bat`
- The toolset is fine on disk: `cl.exe` 19.51.36252 under
  `VC\Tools\MSVC\14.51.36231`, Windows SDKs 10.0.22621.0 / 10.0.26100.0 present
- **Plain `vswhere -products *` returns nothing**; only
  `vswhere -all -prerelease -legacy` finds
  `C:\Program Files\Microsoft Visual Studio\18\Community`. This is *"Visual
  Studio 2026 Developer Command Prompt v18.8.1"*, a prerelease build, and the
  instance registration looks damaged. Instance data exists at
  `C:\ProgramData\Microsoft\VisualStudio\Packages\_Instances\a9afd7ad`

**Workaround — local only, nothing on the machine was changed:** set
`PATH`/`INCLUDE`/`LIB` by hand at `VC\Tools\MSVC\14.51.36231` + SDK
`10.0.26100.0`, then run `C:\Qt\Tools\QtCreator\bin\jom\jom.exe -f Makefile` in
the shadow-build directory. **The user should repair VS 18 Community from the
Visual Studio Installer** before attempting any local MSVC build.

## Other machine state changed outside the repo

`WixToolset.Util.wixext` 6.0.2 is installed globally
(`wix extension add --global`), because `util:RemoveFolderEx` needs it.
Reversible with `wix extension remove`. `smsi.ps1` requires it, and both
`.github/workflows/windows-msi.yml` and `ci.yml`'s `windows-msi` job install it
alongside the UI extension.

## SAFETY — read before deleting anything

- **`C:\Users\susan\seamly2d` on the test laptop is live user data.** It holds a
  real pattern of the user's and is the source tree Task 60's migration copies
  from. A `Remove-Item -Recurse` on it was suggested in an earlier session and
  withdrawn: it bypasses the Recycle Bin, and that is how the dev PC's copy was
  permanently destroyed.
- **No test may touch a path under `QDir::homePath()`** — it cannot be faked on
  Windows. First-run resolution is tested through
  `VCommonSettings::chooseFirstRunDataRoot()`, which takes both candidate roots
  as arguments; everything else uses `QTemporaryDir`.
- **`CollectionTest` is deliberately not run locally.** It has a documented
  pre-existing failure *and* launches the real seamly2d, which seeds folders into
  the live data root. CI runs it on a clean machine.

## Decisions the user has ANSWERED

Act on these, do not re-ask, remove from this file for the next Session handover if marked as done or added to a TODO_*.md file

1. **Task 54's file-name form** → **`SettingsCommon.h`**, i.e. the file name
   matches the class name (the style guide's class-match exception wins over the
   `settings_*` snake_case prefix for class-defining files). **Added another rename to TODO_RENAME_SETTINGS_FILES_CLASSES.md**
2. **`.github/README-DEVELOPER-NEW.md`** → **rename it to
   `.github/README-DEVELOPER-SEAMLY-APPS.md`**, to be folded into
   `.github/README-DEVELOPER.md` when the migration is complete. **DONE.**
3. **Qt WebChannel / Qt Positioning documentation** → maintain it in
   `.github/README-DEVELOPER-SEAMLY-APPS.md` until the migration completes.
4. **`src/app/seamly2d/core/BUILD_PROBLEMS.txt`** → delete it if it is not
   useful. **Done**
5. **Testing happens on the test laptop, not in a VM** (re-confirmed 2026-07-30).
   This PC is Windows 11 **Home**, which ships neither Hyper-V nor Windows
   Sandbox, so a VM here means a third-party hypervisor; the user considered
   VirtualBox and VMware Workstation Pro and declined both. A VM could not close
   two checklist items anyway — the *verified-publisher* UAC prompt needs Task
   33's signing, and the arm64 repeat needs arm64 hardware. **ADDED to TODO_INSTALLER.md**
6. **Data migration is copy-and-verify, leaving the legacy tree intact** — never
   a bare rename, because a user may need to roll back to an earlier release. **ADDED to TODO_INSTALLER.md**
7. **The migration lives in the applications, not the installer.** A per-machine
   MSI's server side runs as LocalSystem, so a per-user path resolves to the
   SYSTEM profile and would only cover whoever ran setup; macOS and Linux have no
   MSI at all, so the logic would be written twice. **ADDED to TODO_INSTALLER.md**
8. **The program directory in Windows is `C:\Program Files\` + `Seamly`** — show the user
   the final assembled path and take OK/Cancel, rather than editing a box whose
   contents differ from the path it applies. **ADDED to TODO_INSTALLER_WIN_X64.md**
9. **Data-root relocation asks first** — prompt Y/N before copying existing data
   files to a new directory location. **ADDED to TODO_INSTALLER.md**
10. **The MSI steps are inlined in `ci.yml`**, not factored out of
    `windows-msi.yml` as a reusable `workflow_call`. The two copies of the x64
    build steps must therefore be kept in step by hand. **ADDED to TODO_INSTALLER_WIN_X64.md**
11. **The x64 `.msi` replaces the NSIS Windows zip** rather than shipping
    alongside it. NSIS stays for arm64 until Task Installer.1.2, because there is
    still no arm64 SeamlyLayout build. **ADDED to TODO_INSTALLER_WIN_X64.md**
12. **Pre-releases are cut from `run-seamlyLayout`**; `develop` stays a pristine
    upstream mirror.** Nothing is published from `develop` until the whole
    SeamlyLayout migration is finished and pushed upstream in one go —
    incremental upstream commits are not workable given the size of the change.
    **ADDED to TODO_INSTALLER.md**
13. **The user-data folder is `SeamlyData`, and page 5 asks only for its
    parent** (2026-08-19). The default parent is the user's `Documents`
    folder, "because users go to this folder to find their data from other
    applications". The leaf is fixed so the user can change the location but
    not the name. **ADDED to TODO_INSTALLER_WIN_X64.md (InstWinX64.00)**
14. **The Change button stays disabled** (2026-08-19). Windows Installer
    cannot move an installed product, and a data-root change in the
    installer would be ignored by anyone who has run an app once. The
    upgrade path is the place both directories can change, and it now
    prefills from the previous install. **ADDED to TODO_INSTALLER_WIN_X64.md
    (InstWinX64.2.11)**
15. **`installer_*` is a file-name prefix** (2026-08-19), added to
    `.github/README-CODE-STYLES.md`. First user:
    `src/libs/vmisc/installer_record.{h,cpp}`. No platform tag on it — the
    contract is cross-platform and the `Q_OS_WIN` block stays inside, as
    `seamly_suite_paths.cpp` already does. **DONE**

## Gotchas

- **A quoted bash heredoc still eats backslashes when the body is written by a
  tool.** Two files were committed this session with a backslash silently
  halved: a registry path in `installer_record.cpp` (`\\` became `\`) and
  `QLatin1Char('\\')` in `tst_dataroot.cpp`, which is an unterminated
  character literal and a hard compile error. **Write C++, XML or Markdown that
  contains backslashes with the Write tool or a Python script, never a
  heredoc**, and grep the result for the backslash before committing.
- **`CollectionTest.exe` must be run with its working directory set to its own
  `bin/`.** `initTestCase()` removes `tst_seamly2d_tmp` *relative to the CWD* but
  re-creates it under `applicationDirPath()`, so from any other CWD it aborts on
  the leftover directory from the previous run ("Fail to prepare test files for
  testing"). Use `Start-Process … -WorkingDirectory <that bin>`.
- **A SeamlyLayout build-tree exe needs Qt on `PATH` to launch.** There is no
  windeployqt output beside `qt_frontend/build/Debug/SeamlyLayout.exe`, so from a
  plain shell it starts and does nothing — no log file is even created. Prepend
  `C:\Qt\6.11.1\msvc2022_64\bin`. `ctest` handles this itself via the
  `ENVIRONMENT_MODIFICATION` added in Task 58. **Not a problem when building with ci.yml on GitHub with Ubuntu-latest runner**
- **SeamlyLayout's log file has two independent writers and they overwrite each
  other.** C++ `Logger` holds a buffered `QTextStream` on the file while Rust's
  `log_to_file()` opens/appends per call, so lines get clipped mid-string. Do not
  conclude a log line is absent because it looks truncated — grep for a
  distinctive fragment. **Added this as a task in TODO_SEAMLYLAYOUT.md**
- **A tagged handoff SVG can be produced headlessly**, without driving the Layout
  Mode GUI: `seamly2d.exe <pattern>.sm2d -b <name> -d <dir> -f 0 --exportOnlyDetails`
  writes `<name>_pieces.svg` through the same `exportSVG()` that
  `generatePiecesSvg()` uses.
- **`QCommandLineParser::parse()` ≠ `process()`.** `process()` prints to a console
  this GUI-subsystem app does not have on Windows, and calls `exit()`. `parse()`
  returns a bool and fills `errorText()`.
- **A `develop` merge can silently drop doc edits made on this branch.** After
  merging `develop`, `git log -S "<a phrase you added>" -- <file>` is the cheapest
  way to confirm your change survived.
- **`QSettings(fileName, format, parent)` records neither an organization nor an
  application name** — both come back empty, and QSettings substitutes the literal
  `"Unknown Organization"`. Root cause of the stray files in Tasks 34 and 52.
  `QSettings::setPath(format, scope, dir)` *does* redirect settings files, but has
  **no getter** — recover the base from a probe instance.
- **`QDir::fromNativeSeparators()` rewrites backslashes only on Windows** (a
  backslash is a legal POSIX filename character), and Windows path comparison must
  be `Qt::CaseInsensitive`.
- **`QDir::rmdir()` over `removeRecursively()`, deliberately.** `rmdir()` cannot
  delete a file and refuses a non-empty directory, so it cannot run away.
- **CI's `make check` runs four test binaries** — `Seamly2DTest`,
  `CollectionTest`, `ParserTest`, `TranslationsTest`. Any hand-run local check
  must cover all four; `Seamly2DTests.exe` alone is one of them.
- **`gh` is on `PATH` and authenticated** as of 2026-08-11; plain `gh …` works.
  If a shell ever comes up without it, invoke
  `& "C:\Program Files\GitHub CLI\gh.exe"`.
- **A CI version component with a leading zero is a C++ octal literal.**
  `scripts/version.sh` now normalizes them; do not "simplify" that `$((10#…))`
  away. Covered by `scripts/version_test.sh`.
- **There is no YAML tooling on this PC.** No `actionlint`, no `yamllint`, no
  `node`, and `python`/`python3` are the Microsoft Store stubs. Git's `perl` has
  no YAML module. Workflow edits can only be validated by pushing and watching
  the run.
- **The sandbox blocks a command containing both a `Remove-Item` and a protected
  path string** — `G:` paths and `C:\Program Files` have both triggered it, even
  when the deletion targets something else entirely (the MSVC environment
  variables set at the top of a build command are enough). Put deletions in
  their own call.
- **Renaming a shadow-build directory invalidates the whole tree.** qmake bakes
  absolute paths into every generated Makefile, so after a rename the sub-builds
  still look for `...\<old name>\...\vtools.lib` and fail with "dependent ...
  does not exist". The tree can only be regenerated, not repaired: delete it and
  re-run qmake. Same failure shape as the toolchain-change trap in
  `.github/README-BUILDS.md`. The build and packaging directory names are now
  `-BuildDirName` / `-OutputDirName` parameters; **a new value needs a matching
  `.gitignore` entry**, and for packaging also the CI artifact path.
- **PowerShell here-strings passed to `git commit -m` get mangled** when the
  message contains quotes; write the message to a file and use `git commit -F`.
- **clangd diagnostics in this repo are noise.** The tree has no
  `compile_commands.json`, so the editor parses each file with zero include
  paths; one unresolved include cascades into dozens of `Unknown type name 'QString'`
  entries. **The qmake build is the authority.**
- **`MD060`/`MD056` table warnings from the editor are noise** on the wide spec
  tables in `TODO_MIGRATE.md`, as `MD041` is repo-wide. Fix a genuinely malformed
  row; do not reflow tables to silence alignment style.
- **PowerShell 5.1 wraps a native exe's stderr in `NativeCommandError`** and sets
  `$?` to `$false` even on exit 0. Do not redirect native stderr inside
  PowerShell — run the script as a child process with
  `Start-Process … -RedirectStandardOutput/-RedirectStandardError -Wait -PassThru -NoNewWindow`.
- **PowerShell splatting: `@array` is positional, `@hashtable` is by name.**
- **Qt frontend test exes are GUI-subsystem binaries** — they print nothing to
  captured stdout. Run with `-o <file>,txt` and `QT_QPA_PLATFORM=offscreen`.
- **`$proFile` collides with the automatic `$PROFILE`** (case-insensitive) in
  PowerShell. Do not use that variable name in a new script.
- **Historical 6.10 references and old directory names in
  `project-docs/TODO_COMPLETED.md` and `project-docs/PROJECT_PLAN.md` are
  deliberate** — they record what was true at the time.
