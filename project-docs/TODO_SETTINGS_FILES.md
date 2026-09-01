# TODO — Create the per-user settings directories and files at install time

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

Tasks in this file begin with `SettingsFiles.`

## Goal

After the MSI completes, and before any app first runs, `%LOCALAPPDATA%\Seamly` must exist and hold:

| Path under `%LOCALAPPDATA%\Seamly` | File |
| --- | --- |
| `.` | `qt6_common.ini` |
| `Seamly2D\` | `qt6_seamly2d.ini` |
| `SeamlyMe\` | `qt6_seamlyme.ini` |
| `SeamlyLayout\` | `qt6_seamlylayout.ini` |

Today each app creates its own file only when the user opens Preferences and clicks Apply or OK.

Decisions (2026-08-30):

1. Move `qt6_common.ini` from `%APPDATA%\Seamly` (Roaming) to `%LOCALAPPDATA%\Seamly` — app-side change first (Task SettingsFiles.1), installer second (Task SettingsFiles.2).
2. The SeamlyMe file is `qt6_seamlyme.ini`.
3. The installer seeds path settings only. The apps supply every other default at runtime.

## Task SettingsFiles.1 — move qt6_common.ini to `%LOCALAPPDATA%\Seamly`

Why: `dataRoot` and the shared `paths/*` keys hold absolute machine paths. Roaming profiles carry them to other machines, where they are wrong. Qt maps `QStandardPaths::GenericConfigLocation` to `%LOCALAPPDATA%` on Windows, `~/.config` on Linux, and `~/Library/Preferences` on macOS — the last two are already where the file lives, so the move changes Windows only.

Do NOT use a global `QSettings::setPath()` redirect: it would also move the `Seamly2DTeam` and legacy probes, which must keep reading the Roaming locations.

- [x] SettingsFiles.1.1 Add `VCommonSettings::commonSettingsFilePath()` — `<GenericConfigLocation>/<organization>/qt6_common.ini` — plus a test-only base-dir override, in `src/libs/vmisc/vcommonsettings.{h,cpp}`
- [x] SettingsFiles.1.2 Point every common-settings `QSettings` construction in `vcommonsettings.cpp` at that path (the explicit-path constructor). Keep the `"Unknown Organization"` stray probe in `mergeStrayCommonSettings()` on the old constructor form. `commonSettingsOrganization()` removed — unused after the change
- [x] SettingsFiles.1.3 Add `VCommonSettings::migrateCommonSettingsLocation()`: create the Local directory, then copy-if-missing from, in order, Roaming `Seamly\qt6_common.ini`, Roaming `Seamly2DTeam\qt6_common.ini`, Roaming `Seamly\common.ini`, Roaming `Seamly2DTeam\common.ini`. Never overwrite; never delete a source
- [x] SettingsFiles.1.4 Replace the three duplicated common-settings bridge blocks — `Application2D::openSettings()`, `ApplicationME::openSettings()`, `TestApplication2D::openSettings()` in `qttestmainlambda.cpp` — with one call to `migrateCommonSettingsLocation()`
- [x] SettingsFiles.1.5 Update `TST_DataRoot`: set/reset the base-dir override; add cases pinning the new location and the Roaming→Local bridge
- [x] SettingsFiles.1.6 Update `Get-SettingsFile` in `packaging/windows/smsi_migrate_user_data.ps1` to include `%LOCALAPPDATA%\Seamly\qt6_common.ini`; keep the Roaming roots for pre-move installs (its test suite passes 16/16 locally). `test_msi_install.ps1` / `test_reset_environment.ps1` read Local first, Roaming second
- [x] SettingsFiles.1.7 Update the location comments in `smsi.wxs` and `smsi_registry.wxs`. Uninstall cleanup already removes both `%LOCALAPPDATA%\Seamly` and `%APPDATA%\Seamly` — keep both `RemoveFolderEx` rows. Also updated the settings tables in `.github/README-BUILDS.md`

## Task SettingsFiles.2 — installer creates the directories and seeds the files

Mechanism: a new deferred, impersonated custom action (`WixQuietExec64`, `Return="ignore"`) — the same pattern as `SeamlyCopyUserData`. Static WiX `IniFile` rows are rejected: MSI rewrites them on repair/upgrade, which clobbers user edits, and the content is dynamic (`DataRoot`, `InstallFolder`).

- [x] SettingsFiles.2.1 New `packaging/windows/smsi_seed_user_settings.ps1`: params `-DataRoot`, `-InstallFolder`, env-overridable settings roots; creates the four directories; writes each ini only if absent, adds only missing keys to an existing one; UTF-8 without BOM (`[System.IO.File]::WriteAllText`, never PS 5.1 `-Encoding utf8`); always `exit 0`
- [x] SettingsFiles.2.2 Seed content, Qt `/` separators. **Corrected against the code (2026-08-31):**
  - `qt6_common.ini` `[paths]`: `dataRoot`, `individual_size_measurements`, `multi_size_measurements`, `templates`, `bodyscans` — only these five are shared. `labels`, `images`, `backups` moved to the per-app list: their setters call `QSettings::setValue` on the app's own settings object (`vcommonsettings.cpp`), so the apps read them from `qt6_seamly2d.ini`
  - `Seamly2D\qt6_seamly2d.ini` `[paths]`: `pattern`, `layout`, `labels`, `images`, `backups`, `seamlyLayoutApp=<InstallFolder>/SeamlyLayout.exe`
  - `SeamlyMe\qt6_seamlyme.ini`: valid empty file — its path keys live in `qt6_common.ini`
  - `SeamlyLayout\qt6_seamlylayout.ini`: **seeded COMPLETE — all 11 keys** (superseded by SettingsFiles.3, 2026-08-31; the first decision was directory-only). `PreferencesModel::load()` takes an existing ini as authoritative and its missing-key fallbacks are empty strings, so the seeded set must never be partial. Values mirror `seedFromBundledDefaults()` + `default_preferences.json` (windows block) with `${HOME}` → data root. `settings\` and `preferences\` subdirectories are created too
- [x] SettingsFiles.2.3 Install the script beside `smsi_migrate_user_data.ps1` (new component `UserSettingsSeedScript` in `smsi_files.wxs`)
- [x] SettingsFiles.2.4 `smsi.wxs`: `SetProperty` + `CustomAction SeamlySeedUserSettings`, deferred, `Impersonate="yes"`, `Return="ignore"`, sequenced after `SeamlyCopyUserData` (so migrated values win — the seeder adds only missing keys), condition `SEAMLYDATAROOTRECORDED AND NOT Installed`; passes `[SEAMLYDATAROOTRECORDED]` and `[INSTALLFOLDER]`
- [x] SettingsFiles.2.5 Tests: new `smsi_seed_user_settings_test.ps1` (19 cases); extended `smsi_check_authoring.ps1` (CA type/command/condition rows, script packaged) and `test_msi_install.ps1` (three ini files exist with the expected keys)
- [x] SettingsFiles.2.6 Docs: `packaging/windows/README_MSI_WORKFLOW.md`, the `project-docs/TEST_MSI_WIN_X64_Test_Case_*` docs, `SESSION_HANDOVER.md`

Known limits, accepted (same tradeoffs already recorded for the data root):

- A perMachine MSI seeds only the installing user. Other Windows accounts get files from the deprecated app-side fallback (SettingsFiles.3).
- A SYSTEM-context `/qn` deployment seeds SYSTEM's profile.

## Task SettingsFiles.3 — deprecate app-side first-run seeding of the ini files

Decision (2026-08-31): the installer owns ini seeding. App-side first-run seeding is deprecated, replaced by per-platform install hooks.

- [x] SettingsFiles.3.1 Windows: the MSI seeds every ini completely, `qt6_seamlylayout.ini` included (see SettingsFiles.2.2)
- [x] SettingsFiles.3.2 Mark the app-side seeding deprecated: `PreferencesModel::load()` ini-missing branch; `VCommonSettings::initializeDataRoot()` cases 2–4
- [ ] SettingsFiles.3.3 **Blocked on packaging format.** The macOS dmg and Linux AppImage have no install step, so no install hook can exist for them. The deprecated app-side fallback must stay until those platforms ship a hook-capable package (.pkg, .deb/.rpm, Flatpak) with install-time seeding
- [ ] SettingsFiles.3.4 Remove the deprecated app-side seeding once 3.3 is done on every shipped platform

The deprecated fallback also remains the only seeding for the non-installing Windows accounts named under SettingsFiles.2's known limits.

## Task SettingsFiles.4 — [known defect] migration custom actions mangle their path arguments

Found 2026-08-31 while verifying SettingsFiles.3. The seed CA had the same defect; its fix is in.

- The properties `SEAMLYDATAROOT`, `SEAMLYDATAROOTRECORDED`, and `INSTALLFOLDER` resolve with a trailing backslash.
- PowerShell's command-line parser reads backslash-quote as an escaped quote, so `-Destination "[SEAMLYDATAROOT]"` swallows the closing quote and the arguments run together into one mangled value.
- Observed live: the seed CA received `DataRoot='C:\...\SeamlyData" -InstallFolder C:\Program'` and seeded garbage paths.
- Fix idiom (applied to `SetSeamlySeedUserSettings`): a space before each closing quote — `"[PROP] "` — and the script trims the value. Guarded by an `smsi_check_authoring.ps1` assertion.
- [ ] SettingsFiles.4.1 Apply the same idiom to `SetSeamlyOldDataMigration` and `SetSeamlyNewDataMigration` in `smsi.wxs` (`-Destination`, `-PreviousDataRoot`, `-InstallFolder`); confirm `smsi_migrate_user_data.ps1` trims each path parameter
- [ ] SettingsFiles.4.2 Add the matching authoring assertions
- [ ] SettingsFiles.4.3 Re-verify migration with a real upgrade install (test cases B/C). The 2026-08-21 verification exercised the script directly, not the CA command line, so live migration has run with mangled `-Destination` until this is fixed

Task SettingsFiles.5 (one-shot fresh-install data notice) is complete — moved to `TODO_COMPLETED.md` (2026-08-31).

Task SettingsFiles.6 (SeamlyLayout defaults profile) is complete — moved to `TODO_COMPLETED.md` (2026-09-01).

Task SettingsFiles.7 (getDefaultDataRoot() leaf → SeamlyData) is complete — moved to `TODO_COMPLETED.md` (2026-09-01).

CI: Task SettingsFiles.1 pushes with the skip token. Tasks SettingsFiles.2/3/4 touch `packaging/**` — push without it.
