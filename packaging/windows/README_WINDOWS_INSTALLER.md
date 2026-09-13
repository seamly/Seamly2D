# Windows install — decision and data flow

Installer decisions, application decisions, outcome per pre-existing-install case.

Marks: **[settled]** built. **[undecided]** open question. **[known defect]** open task.

## Two actors

| | Installer (`seamly-x64.msi`) | Application |
|---|---|---|
| Runs | once at install time as **LocalSystem** | every launch, per user |
| Owns | program files, data root choice, migration, HKLM rows, shortcuts, associations, ARP | data directories, runtime settings |

Fresh Setup creates the chosen `SeamlyData` root; first launch adds its 9
subdirectories. Uninstall keeps the root; migration runs impersonated as the
installing user.

### Getting the MSI

Take it from a **release** to download as `*.msi`, not the Actions page where build artifacts download as `*.msi.zip`.

| Source | Trigger | Contents |
|---|---|---|
| Release `dev-latest` | every push to `run-seamlyLayout` | x64 + arm64 MSIs |
| Release `v<version>` | `schedule`/`workflow_dispatch` | same 2 MSIs + Linux/macOS builds |

`gh release download dev-latest --repo seamly/Seamly2D --pattern 'seamly-x64.msi'`

## Running the Installation

```powershell
msiexec /i seamly-x64.msi                                         # interactive
msiexec /i seamly-x64.msi /qn                                     # silent, defaults
msiexec /i seamly-x64.msi /qn SEAMLYDATAPARENT=E:\                # silent, data root E:\SeamlyData
msiexec /x seamly-x64.msi /qn                                     # silent uninstall
```

### Testing the Installation

4 phases sharing a state file, so later phases assert against earlier ones. Standalone — copy beside the `.msi`, run elevated.

```powershell
.\local_install_msi.ps1 -Phase Baseline
msiexec /i seamly-x64-older.msi
.\local_install_msi.ps1 -Phase Installed -ExpectSeamlyLayout -PatternFile .\sample.sm2d
msiexec /i seamly-x64-newer.msi
.\local_install_msi.ps1 -Phase Upgraded -ExpectSeamlyLayout -PatternFile .\sample.sm2d
msiexec /x seamly-x64-newer.msi
.\local_install_msi.ps1 -Phase Removed
```

Params: `-Phase <Baseline|Installed|Upgraded|Removed>` (required), `-ExpectSeamlyLayout`, `-NoDesktopShortcuts`, `-PatternFile <path>`, `-SkipLaunch`, `-StateFile <path>`.

Upgrade test needs 2 packages with different `-Version` values (2 CI runs).

`local_reset_environment.ps1` resets a test machine past what uninstall leaves — test-support only, more destructive than the shipped uninstall by design.

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

## WiX source layout

[`smsi.wxs`](smsi.wxs) is the hub: it holds `<Package>` and everything the
linker needs to see there — identity, upgrade, ARP, the properties the
dialogs read, the launch conditions, the user-data copy action. It pulls in
five fragment files by reference (`UIRef`/`ComponentGroupRef`); a fragment
nothing refers to is dropped without error, so the MSI still builds and
quietly lacks that whole area.

| File | Owns |
|---|---|
| `smsi.wxs` | `<Package>`, upgrade/ARP, data-root properties, migration and seeding custom actions |
| `smsi_ui.wxs` | the wizard: every dialog and transition |
| `smsi_legacy.wxs` | detection (`RegistrySearch`/`FileSearch`) and removal of the pre-MSI install |
| `smsi_files.wxs` | directory tree, the three executables, Start Menu shortcuts, file associations |
| `smsi_shortcuts.wxs` | optional desktop shortcuts |
| `smsi_registry.wxs` | install-info registry rows, per-user settings removal on uninstall |

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
4. `wix build` on every `*.wxs` file in this directory (globbed, not
   hard-coded) — `smsi.wxs` plus its five fragments. Every package carries
   all 3 apps — no switch to omit SeamlyLayout.
5. Unless `-SkipValidation`: `wix msi validate -sice ICE43 -sice ICE57`.
6. `smsi_check_authoring.ps1` — always runs, fails the build; the only check
   `-SkipValidation` doesn't skip.

## Installer flow

1. UAC (perMachine) → `SeamlyPrepareDlg` gates the wizard: it blocks
   `AppSearch`/`CostFinalize` until the user clicks Continue, so detection
   never flashes under the Welcome page (replaces stock `PrepareDlg`, which
   is modeless and does not gate). Continue → `AppSearch` sets detection
   properties.
2. Downgrade detected → abort (`DowngradeErrorMessage`).
3. Fresh install or repair-from-nothing: `WelcomeDlg` → `LicenseAgreementDlg`.
4. Old app without Layout, or new Layout found → `SeamlyPreviousInstallDlg`
   (upgrade and/or NSIS paragraph); case A / repair skips straight to 5.
5. Wizard: `SeamlyDataDirDlg` → `SeamlyDataMigrateDlg` (if a prior
   install exists) → `SeamlyShortcutsDlg` → `VerifyReadyDlg`. No
   program-directory page: `INSTALLFOLDER` is fixed to
   `%ProgramFiles%\SeamlyApps` and never asked about.
6. Install files; write HKLM per-app keys ×3, shortcuts, 3 associations, ARP.
7. Upgrade detected → `RemoveExistingProducts` (removes older MSI + its dir).
8. NSIS present → remove its dir, Start Menu folder, registry keys.
   `uninstall.exe` never run (interactive, `RMDir /r`, no rollback).
9. Migration selected → archive+extract into `SeamlyData`, merge settings
   (retain non-path settings, replace path settings).
10. Root recorded → `smsi_seed_user_settings.ps1` (deferred, impersonated,
    non-fatal, after migration) seeds `%LOCALAPPDATA%\Seamly`: `qt6_common.ini`
    and `Seamly2D\qt6_seamly2d.ini` get every missing `[paths]` key,
    `SeamlyMe\qt6_seamlyme.ini` is created empty,
    `SeamlyLayout\qt6_seamlylayout.ini` gets the complete 11-key set
    (`PreferencesModel::load()` takes an existing ini as authoritative, so a
    partial one must never be written). Add-only: migrated or existing values
    always win. No app needs a Preferences > Paths visit, and no app seeds
    its own ini on an installed machine.

**Maintenance (repair/uninstall, `Installed` true):**
`MaintenanceWelcomeDlg` → `SeamlyMaintenanceTypeDlg` → `VerifyReadyDlg`.
The middle page replaces stock `MaintenanceTypeDlg` to add the installed
`SEAMLYINSTALLEDVERSION` note; otherwise identical. A patch keeps the stock
shortcut `WelcomeDlg` → `VerifyReadyDlg`. None of steps 3-5 run on this path.

- Own dialog set — every transition self-authored;
  stock `WixUI_InstallDir` can't be extended this way.
- Previous-install page skips repair/uninstall (`AND NOT Installed`).
- `/qn` shows no page — pass `SEAMLYDATAPARENT`/`SEAMLYDATAROOT`,
  `SEAMLYCOPYUSERDATA`, `SEAMLYDESKTOPSHORTCUTS` to override defaults.

## Application flow — user-data root, first launch

Runs in `Application2D::openSettings()`/`ApplicationME::openSettings()`,
independent of install method. First match wins:

1. `paths/dataRoot` already set in `qt6_common.ini` → use unchanged.
2. Setup recorded a root in the registry → use that (normal MSI outcome).
3. Default root missing AND the legacy root is a directory → adopt it in
   place, nothing moved or copied (normal case-B outcome). Probes `~/seamly2d`.
4. Otherwise → `<Documents>/SeamlyData`.

Then `ensureDataRootTree` creates the 9 subfolders (additive only). If a
legacy root exists, isn't the chosen root, and holds no files anywhere,
remove the empty skeleton (`rmdir` only, deepest first) — never deletes a
file, never touches a non-empty directory, never `removeRecursively()`.

Setup's promise (`InstallerRecord::dataRoot()`) outranks built-in defaults
but sits below `paths/dataRoot`, so later Preferences changes still win.
Seeding happens only in the apps, never inside `initializeDataRoot()` (unit
tests call that directly).

## Where behaviour is defined

| Concern | File |
|---|---|
| Package identity, upgrade, ARP, data-root properties, migration/seeding actions | `smsi.wxs` |
| Wizard dialogs and transitions | `smsi_ui.wxs` |
| Legacy-NSIS detection and removal | `smsi_legacy.wxs` |
| Directory tree, executables, Start Menu shortcuts, file associations | `smsi_files.wxs` |
| Optional desktop shortcuts | `smsi_shortcuts.wxs` |
| Install-info registry rows, per-user settings removal | `smsi_registry.wxs` |
| Staging, version mapping, `wix build` over every `*.wxs` | `smsi.ps1` |
| Only invocation | `.github/workflows/ci.yml`, job `windows-msi` |
| Built-package / real-install assertions | `smsi_check_authoring.ps1`, `local_install_msi.ps1` |
| Data root resolution/seeding/pruning | `src/libs/vmisc/vcommonsettings.cpp` |
| App call sites | `application_2d.cpp`, `application_me.cpp` |
