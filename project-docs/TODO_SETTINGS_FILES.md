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

Do NOT use a global `QSettings::setPath()` redirect: it would also move the `Seamly2DTeam` and `Unknown Organization` legacy probes, which must keep reading the Roaming locations.

- [x] SettingsFiles.1.1 Add `VCommonSettings::commonSettingsFilePath()` — `<GenericConfigLocation>/<organization>/qt6_common.ini` — plus a test-only base-dir override, in `src/libs/vmisc/vcommonsettings.{h,cpp}`
- [x] SettingsFiles.1.2 Point every common-settings `QSettings` construction in `vcommonsettings.cpp` at that path (the explicit-path constructor). Keep the `"Unknown Organization"` stray probe in `mergeStrayCommonSettings()` on the old constructor form. `commonSettingsOrganization()` removed — unused after the change
- [x] SettingsFiles.1.3 Add `VCommonSettings::migrateCommonSettingsLocation()`: create the Local directory, then copy-if-missing from, in order, Roaming `Seamly\qt6_common.ini`, Roaming `Seamly2DTeam\qt6_common.ini`, Roaming `Seamly\common.ini`, Roaming `Seamly2DTeam\common.ini`. Never overwrite; never delete a source
- [x] SettingsFiles.1.4 Replace the three duplicated common-settings bridge blocks — `Application2D::openSettings()`, `ApplicationME::openSettings()`, `TestApplication2D::openSettings()` in `qttestmainlambda.cpp` — with one call to `migrateCommonSettingsLocation()`
- [x] SettingsFiles.1.5 Update `TST_DataRoot`: set/reset the base-dir override; add cases pinning the new location and the Roaming→Local bridge
- [x] SettingsFiles.1.6 Update `Get-SettingsFile` in `packaging/windows/smsi_migrate_user_data.ps1` to include `%LOCALAPPDATA%\Seamly\qt6_common.ini`; keep the Roaming roots for pre-move installs (its test suite passes 16/16 locally). `test_msi_install.ps1` / `test_reset_environment.ps1` read Local first, Roaming second
- [x] SettingsFiles.1.7 Update the location comments in `smsi.wxs` and `smsi_registry.wxs`. Uninstall cleanup already removes both `%LOCALAPPDATA%\Seamly` and `%APPDATA%\Seamly` — keep both `RemoveFolderEx` rows. Also updated the settings tables in `.github/README-BUILDS.md`

## Task SettingsFiles.2 — installer creates the directories and seeds the files

Mechanism: a new deferred, impersonated custom action (`WixQuietExec64`, `Return="ignore"`) — the same pattern as `SeamlyCopyUserData`. Static WiX `IniFile` rows are rejected: MSI rewrites them on repair/upgrade, which clobbers user edits, and the content is dynamic (`DataRoot`, `InstallFolder`).

- [ ] SettingsFiles.2.1 New `packaging/windows/smsi_seed_user_settings.ps1`: params `-DataRoot`, `-InstallFolder`, env-overridable settings roots; creates the four directories; writes each ini only if absent, adds only missing keys to an existing one; UTF-8 without BOM (`[System.IO.File]::WriteAllText`, never PS 5.1 `-Encoding utf8`); always `exit 0`
- [ ] SettingsFiles.2.2 Seed content, Qt `/` separators:
  - `qt6_common.ini` `[paths]`: `dataRoot`, `individual_size_measurements`, `multi_size_measurements`, `templates`, `bodyscans`, `labels`, `images`, `backups`
  - `Seamly2D\qt6_seamly2d.ini` `[paths]`: `pattern`, `layout`, `seamlyLayoutApp=<InstallFolder>/SeamlyLayout.exe`
  - `SeamlyMe\qt6_seamlyme.ini`: valid empty file — its path keys live in `qt6_common.ini`
  - `SeamlyLayout\qt6_seamlylayout.ini`: `layout_directory=<DataRoot>/layouts`, `input_directory`; confirm the exact key set against `PreferencesModel::save` before writing
- [ ] SettingsFiles.2.3 Install the script beside `smsi_migrate_user_data.ps1` (new component in `smsi_files.wxs`)
- [ ] SettingsFiles.2.4 `smsi.wxs`: `SetProperty` + `CustomAction SeamlySeedUserSettings`, deferred, `Impersonate="yes"`, sequenced after `SeamlyCopyUserData`, condition `NOT Installed`; pass `[SEAMLYDATAROOTRECORDED]` and `[INSTALLFOLDER]`
- [ ] SettingsFiles.2.5 Tests: new `smsi_seed_user_settings_test.ps1`; extend `smsi_check_authoring.ps1` (CA rows) and `test_msi_install.ps1` (files exist with expected keys)
- [ ] SettingsFiles.2.6 Docs: `packaging/windows/README_MSI_WORKFLOW.md`, the `project-docs/TEST_MSI_WIN_X64_Test_Case_*` docs, `SESSION_HANDOVER.md`

Known limits, accepted (same tradeoffs already recorded for the data root):

- A perMachine MSI seeds only the installing user. Other Windows accounts get files on their own first Apply.
- A SYSTEM-context `/qn` deployment seeds SYSTEM's profile.

CI: Task SettingsFiles.1 pushes with the skip token. Task SettingsFiles.2 touches `packaging/**` — push without it.
