# Windows install — decision and data flow

Installer decisions, application decisions, outcome per pre-existing-install case.

Marks: **[settled]** built. **[undecided]** open question. **[known defect]** open task.

## Two actors

| | Installer (`seamly-x64.msi`) | Application |
|---|---|---|
| Runs | once, install time, as **LocalSystem** | every launch, per user |
| Owns | program files, data root choice, migration, HKLM rows, shortcuts, associations, ARP | data directories, runtime settings |

Fresh Setup creates the chosen `SeamlyData` root; first launch adds its 9
subdirectories. Uninstall keeps the root; migration runs impersonated as the
installing user.

## Detection inputs

| Property | Source | Meaning |
|---|---|---|
| `WIX_UPGRADE_DETECTED` | `FindRelatedProducts` | older MSI of this suite installed |
| `WIX_DOWNGRADE_DETECTED` | same | newer MSI installed |
| `SEAMLYLEGACYUNINSTALLSTRING` | `RegistrySearch`, `always32` | old NSIS product installed |
| `SEAMLYOLD{S2D,ME,LAYOUT}EXE` | `RegistrySearch`+`FileSearch`, legacy dir | old exe exists |
| `SEAMLYNEWLAYOUTEXE` | `RegistrySearch`+`FileSearch`, suite dir | new SeamlyLayout exists |
| `SEAMLYLEGACYINSTALLDIR` | `RegistrySearch`, `always32` | legacy path, normally `...\Seamly2D` (x86) |
| `Installed` | Windows Installer | repair/modify/uninstall, not fresh |

`always32`: NSIS is 32-bit, keys live under `WOW6432Node` — a default-view
x64 search finds nothing.

## Four cases

| # | Old NSIS | New MSI | Name |
|---|---|---|---|
| **A** | no | no | clean machine |
| **B** | yes | no | upgrade from standalone product |
| **C** | no | yes | upgrade from previous MSI |
| **D** | yes | yes | both — MSI installed without removing NSIS; separate ARP entries, different removal per product |

## Package build flow

[`smsi.ps1`](smsi.ps1) builds packages; only `ci.yml`'s `windows-msi` job
runs it. Every input is a named parameter — nothing inherited from the build
machine. Parameter table: [`README_WINDOWS_BUILD.md`](README_WINDOWS_BUILD.md).

1. Check exes/wix/windeployqt/CRT present; throw naming what's missing.
2. Derive `ProductVersion` (`YY.M.((D-1)*1440+MMMM)`) — strictly increasing
   per build, so cases C/D upgrade correctly; same-minute builds tie.
3. Stage: merge bins into `parent\`, `windeployqt` SeamlyLayout, add
   settings/licenses/CRT; move 3 exes into `exes\` (authored explicitly, not
   wildcard-harvested, so shortcuts/associations can reference them).
4. `wix build smsi.wxs`. Every package carries all 3 apps — no switch to
   omit SeamlyLayout.
5. Unless `-SkipValidation`: `wix msi validate -sice ICE43 -sice ICE57`.
6. `smsi_check_authoring.ps1` — always runs, fails the build; the only check
   `-SkipValidation` doesn't skip.

## Installer flow

1. UAC (perMachine) → `AppSearch` sets detection properties.
2. Downgrade detected → abort (`DowngradeErrorMessage`).
3. Old app without Layout, or new Layout found → `SeamlyPreviousInstallDlg`
   (upgrade and/or NSIS paragraph); case A / repair skips straight to 4.
4. Wizard: `InstallDirDlg` → `SeamlyDataDirDlg` → migrate dialog (if a prior
   install exists) → `SeamlyShortcutsDlg` → `VerifyReadyDlg`.
5. Install files; write HKLM per-app keys ×3, shortcuts, 3 associations, ARP.
6. Upgrade detected → `RemoveExistingProducts` (removes older MSI + its dir).
7. NSIS present → remove its dir, Start Menu folder, registry keys.
   `uninstall.exe` never run (interactive, `RMDir /r`, no rollback).
8. Migration selected → archive+extract into `SeamlyData`, merge settings
   (retain non-path settings, replace path settings).
9. Root recorded → `smsi_seed_user_settings.ps1` (deferred, impersonated,
   non-fatal, after migration) seeds `%LOCALAPPDATA%\Seamly`: `qt6_common.ini`
   and `Seamly2D\qt6_seamly2d.ini` get every missing `[paths]` key,
   `SeamlyMe\qt6_seamlyme.ini` is created empty,
   `SeamlyLayout\qt6_seamlylayout.ini` gets the complete 11-key set
   (`PreferencesModel::load()` takes an existing ini as authoritative, so a
   partial one must never be written). Add-only: migrated or existing values
   always win. No app needs a Preferences > Paths visit, and no app seeds
   its own ini on an installed machine (app-side first-run seeding is
   deprecated — Task SettingsFiles.3).

- Own dialog set (Task InstWinX64.1) — every transition self-authored;
  stock `WixUI_InstallDir` can't be extended this way.
- Previous-install page skips repair/uninstall (`AND NOT Installed`).
- `/qn` shows no page — pass `SEAMLYDATAPARENT`/`SEAMLYDATAROOT`,
  `SEAMLYCOPYUSERDATA`, `SEAMLYDESKTOPSHORTCUTS` to override defaults.

## Application flow — user-data root, first launch

Runs in `Application2D::openSettings()`/`ApplicationME::openSettings()`,
independent of install method. First match wins:

1. `paths/dataRoot` already set in `qt6_common.ini` → use unchanged.
2. Setup recorded a root in the registry → use that (normal MSI outcome).
3. Default root missing AND a legacy root is a directory → adopt it in place,
   nothing moved or copied (normal case-B outcome). Probes newest first:
   `<Documents>/Seamly` (pre-SettingsFiles.7 default), then `~/seamly2d`.
4. Otherwise → `<Documents>/SeamlyData`.

Then `ensureDataRootTree` creates the 9 subfolders (additive only). If a
legacy root exists, isn't the chosen root, and holds no files anywhere,
remove the empty skeleton (`rmdir` only, deepest first) — never deletes a
file, never touches a non-empty directory, never `removeRecursively()`.

Setup's promise (`InstallerRecord::dataRoot()`) outranks built-in defaults
but sits below `paths/dataRoot`, so later Preferences changes still win.
Seeding happens only in the apps, never inside `initializeDataRoot()` (unit
tests call that directly).

## Decisions

1. **[settled]** Only NSIS needs removal code — `RemoveExistingProducts`
   handles a previous MSI.
2. **[settled]** NSIS removed via rollback-capable `RemoveFiles`;
   `uninstall.exe` never invoked. Warning tells user to move their own files
   out of that folder first.
3. **[settled]** NSIS ARP entry removed with it — no orphaned entry.
4. **[settled]** Data migration (Task 14): `smsi_migrate_user_data.ps1`.
   Verified 2026-08-21; fixed a silent-failure bug there (missing
   `System.IO.Compression` import, caught and swallowed on PowerShell 5.1).
5. **[settled]** `SeamlyShortcutsDlg` authoring verified; interactive run
   pending (InstWinX64.1.6).
6. **[known defect]** `SeamlyPreviousInstallDlg` controls 3px too wide
   (error 2826 ×2). Task InstWinX64.7.6.
7. **[settled]** Settings seeding (Tasks SettingsFiles.2/3, 2026-08-31):
   `smsi_seed_user_settings.ps1` seeds every ini completely,
   `qt6_seamlylayout.ini` included. The user never has to open Preferences >
   Paths — fresh installs get defaults, upgrades keep the migrated
   configuration (add-only merge). App-side first-run seeding is deprecated;
   it stays only for packages with no install hook (macOS dmg, Linux
   AppImage, dev builds, other Windows accounts).

## Where behaviour is defined

| Concern | File |
|---|---|
| Detection, dialogs, directories, components | `smsi.wxs` |
| Registry rows, per-user settings removal | `smsi_registry.wxs` |
| Staging, version mapping, `wix build` | `smsi.ps1` |
| Only invocation | `.github/workflows/ci.yml`, job `windows-msi` |
| Built-package / real-install assertions | `smsi_check_authoring.ps1`, `test_msi_install.ps1` |
| Data root resolution/seeding/pruning | `src/libs/vmisc/vcommonsettings.cpp` |
| App call sites | `application_2d.cpp`, `application_me.cpp` |
