# Windows MSI installer — Seamly app suite (Task 13)

WiX authoring and build reference for the Windows `.msi` that ships **seamly2d.exe**, **seamlyme.exe**, **seamlylayout.exe** together, per architecture (x64, arm64). Durable KB record: [`.github/README-BUILDS.md`](../../../.github/README-BUILDS.md). Detailed build-mechanics walkthrough: [`README_WINDOWS_BUILD.md`](README_WINDOWS_BUILD.md).

## Files

| File | Purpose |
|---|---|
| `smsi.wxs` | `<Package>`, identity, upgrade, ARP, dialog-read properties, launch conditions, user-data copy action, fragment refs |
| `smsi_ui.wxs` | wizard dialogs and transitions |
| `smsi_legacy.wxs` | find/remove pre-MSI (NSIS) installation |
| `smsi_files.wxs` | directory tree, exes, Start Menu shortcuts, file associations |
| `smsi_shortcuts.wxs` | optional desktop shortcuts |
| `smsi_registry.wxs` | install-info registry values; per-user settings removal on uninstall |
| `seamly2d.ico`, `seamlyme.ico`, `seamlylayout.ico` | shortcut/ARP icons; each `<Icon Id>` must equal the file name |
| `license.rtf` | license text shown in installer UI |
| `smsi.ps1` | stage + `wix build` driver; run by `ci.yml` `windows-msi`. CI-only — no local-build mode, detects nothing from the host |
| `build_msi_local.ps1` | local x64 dev-build driver: builds all 3 apps, then calls `smsi.ps1`. Not run by CI |
| `smsi_check_authoring.ps1` | asserts the built MSI's contents (shortcuts, associations, registry, elevation, upgrade detection, dialogs); run by `smsi.ps1` every build |
| `smsi_migrate_user_data.ps1` | deferred, impersonated action: archives/extracts a user's data tree during install/upgrade |
| `smsi_migrate_user_data_test.ps1` | unit tests for `smsi_migrate_user_data.ps1`, no MSI needed |
| `test_msi_install.ps1` | asserts what an **installed** MSI did to a real machine, 4 phases around `msiexec`; standalone, copy beside the `.msi` |
| `test_reset_environment.ps1` | test-support only: wipes a machine past what uninstall leaves (data root included) to reset for the next test-matrix run |
| `INSTALL_DECISION_FLOW.md` | installer-vs-app decision flowcharts across all 4 pre-existing-install cases |
| `../../../.github/workflows/ci.yml` | only CI route to a Windows package; `windows-msi` matrix over `arch` builds both MSIs, `publish` attaches them to the pre-release |

Editing any file in this directory runs the full CI suite (no `paths-ignore` match).

## Source layout

`<Package>`, `MajorUpgrade`, `MediaTemplate`, `SummaryInformation` cannot live in a fragment — that's why `smsi.wxs` itself isn't one.

**Silent failure mode:** `wix build` links whatever files/refs it's given. A fragment nothing references, or a dropped `ComponentGroupRef`/`UIRef`, builds clean with the MSI just missing that area — no error. Guarded two ways: `smsi.ps1` globs `*.wxs` (new fragments need no script change), `smsi_check_authoring.ps1` asserts against the built MSI (a lost fragment fails the build).

## Key decisions

- **WiX v6**, not v7 — v7 requires accepting its OSMF EULA (error WIX7015). Revisit at >$10k USD project/sponsor income. UI extension version must match core: `wix extension add --global WixToolset.UI.wixext/<version>`.
- **One MSI per arch**, not per-app — the apps share files/settings and install/upgrade as one unit. Output: `scripts\seamly-msi\<arch>\seamly-<arch>.msi`.
- **One flat install dir, one Qt runtime**: `[ProgramFiles64Folder]\SeamlyApps\` holds all 3 exes + merged Qt 6.11.1 runtime. seamly2d locates SeamlyLayout via `SeamlySuitePaths::locateSeamlyLayout()` (`src/libs/vmisc/seamly_suite_paths.cpp`), flat-beside-seamly2d first, `...\Seamly2D\SeamlyLayout\` as fallback for older-layout upgrades.
- **CRT deployed app-locally** from `VCToolsRedistDir`, not merge modules or `vc_redist.exe` chaining — MSIs can't cleanly nest installers, merge modules are deprecated, app-local is arch-symmetric and a single copy for all 3 apps here.
- **CI-only build.** `smsi.ps1` takes every input as a named param; no default is read from the host. Build via `gh workflow run ci.yml --ref run-seamlyLayout`.
- **`windeployqt`, never `windeployqt6`** — matches `qtPrepareTool(WINDEPLOYQT, windeployqt)` in the `.pro` files.
- **Upgrade code `cbf4b5f1-c32c-4dbb-b385-3ee4a7b30658` is fixed forever**, shared by both arches. Never change it. ProductCode is regenerated per build.
- **MSI ProductVersion** = `YY.M.((D-1)*1440 + MMMM)`, derived from the project's `YY.M.D.MMMM` version (MSI ignores the 4th field for upgrade comparisons). Full version stored as `DisplayVersion` in `HKLM\SOFTWARE\Seamly\Seamly2D`.
- **File associations**: `.sm2d` → Seamly2D, `.smis`/`.smms` → SeamlyMe. None for SeamlyLayout (`.pieces.svg` can't be registered apart from plain `.svg`).
- **Install-info registry rows mirror to all 3 apps**: `HKLM\SOFTWARE\Seamly\<App>` (`InstallPath`, `DisplayVersion`, `DataRoot`, `DataParent`) is written for Seamly2D, SeamlyMe, and SeamlyLayout. Seamly2D's key is canonical — `InstallerRecord::dataRoot()` and the upgrade-detection `AppSearch` read it specifically. The other two exist for external tooling; nothing in this repo reads them yet.
- **Start Menu**: 3 advertised shortcuts at Start Menu root, no folder.
- **Data root**: fresh install creates the selected `SeamlyData` root; apps create their 9 standard subdirs on first launch. Uninstall keeps the root.
- **Update migration**: impersonated action, runs as installing user, preserves non-path settings, replaces path settings only after copy verifies.

## Install-time dialogs

Own dialog set (Task InstWinX64.1) — reuses stock dialogs, owns every transition. Fresh install:

| # | Page | Dialog | Shown when |
|---|---|---|---|
| 1 | Welcome | `WelcomeDlg` | always |
| 2 | License | `LicenseAgreementDlg` | always |
| 3 | Existing install found | `SeamlyPreviousInstallDlg` | old NSIS/MSI without SeamlyLayout, or new SeamlyLayout detected |
| 4 | Program directory | `InstallDirDlg` | always |
| 5 | Data root | `SeamlyDataDirDlg` | always |
| 6 | Copy existing work? | `SeamlyDataMigrateDlg` | old or new Seamly install exists |
| 7 | Shortcuts | `SeamlyShortcutsDlg` | always |
| 8 | Ready | `VerifyReadyDlg` | always |
| 9 | Progress | `ProgressDlg` | always |
| 10 | Finish | `ExitDialog` | always |

Maintenance/repair/uninstall: `MaintenanceWelcomeDlg` → `SeamlyMaintenanceTypeDlg` → `VerifyReadyDlg`. Seamly pages 3/5/6/7 don't apply and don't appear.

`SeamlyMaintenanceTypeDlg` is a full transcription of stock `MaintenanceTypeDlg` (WiX won't let a fragment add a control to another fragment's `<Dialog>`), plus one line naming the installed version, read via `AppSearch` from `HKLM\SOFTWARE\Seamly\Seamly2D\DisplayVersion`.

`ARPNOMODIFY` is set — single feature, nothing to select, and **Windows Installer can't move an installed product** anyway (every component is registered against `INSTALLFOLDER` at install time). To relocate: uninstall/reinstall, or a major upgrade (its program-dir page is prefilled from `HKLM\...\InstallPath`).

### Seamly-authored pages

| Page | Behavior |
|---|---|
| Existing install found | Warns program files will be replaced; states user data is untouched. Conditional paragraphs for old-MSI (`WIX_UPGRADE_DETECTED`) vs old-NSIS. |
| Data root | `SEAMLYDATAROOT`, default `C:\Users\<you>\Documents\SeamlyData`. **Change** opens stock `BrowseDlg`, editing the **parent** — Setup appends the fixed `SeamlyData` leaf. Any drive allowed, cloud-synced included. Recorded to `HKLM\...\DataRoot`; every app adopts it on first run. |
| Copy existing work? | Checkbox `SEAMLYCOPYUSERDATA`, default off. Old Seamly: archives+extracts `seamly2d`, renames root to `SeamlyData`. New Seamly: archives `SeamlyData` as-is. |
| Shortcuts | One checkbox for all 3 apps, `SEAMLYDESKTOPSHORTCUTS`, default on. |

### Silent-install properties

| Property | Default | Notes |
|---|---|---|
| `INSTALLFOLDER` | prior install path, else `C:\Program Files\SeamlyApps` | rejected by launch condition if path contains OneDrive/Dropbox/Google Drive/iCloud/Box Sync — a sync client replacing in-use files breaks the install |
| `SEAMLYDATAPARENT` | `C:\Users\<user>\Documents` (UI sequence only) | **no default under `/qn`** — computed from the `PersonalFolder` known folder, which the UI sequence never runs under silent install; pass explicitly or apps fall back to their own default |
| `SEAMLYDATAROOT` | `[SEAMLYDATAPARENT]\SeamlyData` | set directly to override the composed path |
| `SEAMLYCOPYUSERDATA` | `0` | `1` to migrate existing work into `SEAMLYDATAROOT`; never overwrites existing destination files |
| `SEAMLYDESKTOPSHORTCUTS` | `1` | desktop shortcuts for all 3 apps |

`SEAMLYDATAPARENT` has no `/qn` default because the execute sequence runs elevated as SYSTEM (`%USERPROFILE%` = `systemprofile`) — computing a default there would put patterns inside a system account's profile. `HKLM\...\DataRoot` is written from `SEAMLYDATAROOTRECORDED`, not `SEAMLYDATAROOT` directly, since a directory id always resolves to something; an unset `SEAMLYDATAROOT` records nothing, and apps then use their own default.

### Notable rejected/blocked approaches

- **NSIS `uninstall.exe` is never invoked** — different product (own ARP entry, own uninstaller, `RMDir /r $INSTDIR`, no rollback if the rest of Setup then fails). Setup instead removes the 4 things NSIS created (program dir, Start Menu folder, 2 registry keys) via rollback-capable `RemoveFiles` components. Details: `INSTALL_DECISION_FLOW.md`.
- **No taskbar-pin checkbox** — Windows 10+ blocks the `taskbarpin` verb for third-party callers; only OEM/enterprise provisioning can do it, which an MSI can't drive.
- **Own dialog set instead of `WixUI_InstallDir`** — stock `InstallDirDlg`'s Next row is a fixed `NewDialog VerifyReadyDlg` with condition `1`; no page can be inserted after it or excluded from it.
- **NSIS registry search uses `Bitness="always32"`** — NSIS installer is 32-bit, keys live under `WOW6432Node`; default view on x64 MSI would miss them.
- **ARP `DisplayVersion` shows `26.y.z`, not the project version** — `RegisterProduct` writes ARP after `WriteRegistryValues`, overwriting any component-authored override. Full version reaches the user via `ARPCOMMENTS` and `HKLM\...\DisplayVersion`.
- **ICE43/ICE57 suppressed, only those two** — both assume `DesktopFolder` is per-user; package is `Scope="perMachine"`/`ALLUSERS=1`, so `DesktopFolder` is the All Users desktop and the HKLM key path is correct as authored.
- **No copy-migration rollback action** — can't tell added files from pre-existing ones in the destination; copy is additive-only so nothing needs undoing on failure.

## Building

**Release:** CI only.

```powershell
gh workflow run ci.yml --ref run-seamlyLayout
```

`windows-msi` matrix (`ci.yml`): x64 on `windows-latest` / `win64_msvc2022_64`, arm64 (native, not cross-compiled) on `windows-11-arm` / `win64_msvc2022_arm64`. Both legs install Qt 6.11.1 with `qtmultimedia qtwebengine qtwebchannel qtpositioning`, then run:

```powershell
.\scripts\packaging\windows\smsi.ps1 -Arch <arch> -Version $env:VERSION_NUMBER `
  -Seamly2DBin src\app\seamly2d\bin -SeamlyMeBin src\app\seamlyme\bin `
  -WinDeployQt "$env:QT_ROOT_DIR\bin\windeployqt.exe"
```

**Local x64 dev build:**

```powershell
.\scripts\packaging\windows\build_msi_local.ps1
```

Builds all 3 apps release, stamps `-Version` into `projectversion.cpp/.h`/`Info.plist` via `scripts\version.sh` (reverted with `git checkout` after, unless those files already carried uncommitted changes), auto-detects a `msvc2022_64` Qt 6.11.1+ kit under `C:\Qt`, installs WiX v6 if missing. Output identical location to CI. Local dev build only, not a release artifact.

`smsi.ps1` param reference and full staging/validation flow: [`README_WINDOWS_BUILD.md`](README_WINDOWS_BUILD.md).

`smsi.ps1` checks prerequisites first, fails on the first missing one (named). Staging: `parent\` (merged windeployqt output, one shared Qt runtime) + `exes\` (3 exes, authored explicitly in `.wxs` for shortcuts/associations). `wix build -pdbtype none` (no `.wixpdb`) → `wix msi validate` (skip: `-SkipValidation`; ICE43/ICE57 suppressed, ICE61 expected from `AllowSameVersionUpgrades`) → `smsi_check_authoring.ps1` (not skippable — cheap, guards silent content loss).

### Getting the MSI

Take it from a **release**, not the Actions page — `actions/upload-artifact` zips everything, so a build artifact downloads as `.msi.zip`.

| Source | Trigger | Contents |
|---|---|---|
| Release `dev-latest` | every push to `run-seamlyLayout` | `seamly-x64.msi`, `seamly-arm64.msi`, raw |
| Release `v<version>` | `schedule`/`workflow_dispatch` | same 2 MSIs + Linux/macOS builds |
| Build artifact `seamly-<arch>.msi` | every run | zipped |

```powershell
gh release download dev-latest --repo seamly/Seamly2D --pattern 'seamly-x64.msi'
```

`dev-latest` is a rolling pre-release (recreated each push), Windows-only (depends on `windows-msi` alone — a broken Linux/macOS leg doesn't block it).

## Installing / testing

```powershell
msiexec /i seamly-x64.msi                                        # interactive
msiexec /i seamly-x64.msi /qn                                     # silent, defaults, needs elevation
msiexec /i seamly-x64.msi /qn INSTALLFOLDER=D:\SeamlyApps         # silent, custom program dir
msiexec /i seamly-x64.msi /qn SEAMLYDATAPARENT=E:\                # silent, data root E:\SeamlyData
msiexec /x seamly-x64.msi /qn                                     # silent uninstall
msiexec /a seamly-x64.msi /qn TARGETDIR=C:\extract                # extract only — needs a SHORT target path (MAX_PATH 1603 otherwise)
```

### `test_msi_install.ps1`

4 phases around real `msiexec` calls, sharing a state file so later phases assert against earlier ones (e.g. "uninstall took no user data"). Standalone — copy beside the `.msi`, run elevated.

```powershell
.\test_msi_install.ps1 -Phase Baseline                                              # before install
msiexec /i seamly-x64-older.msi
.\test_msi_install.ps1 -Phase Installed -ExpectSeamlyLayout -PatternFile .\sample.sm2d
msiexec /i seamly-x64-newer.msi                                                     # upgrade
.\test_msi_install.ps1 -Phase Upgraded -ExpectSeamlyLayout -PatternFile .\sample.sm2d
msiexec /x seamly-x64-newer.msi
.\test_msi_install.ps1 -Phase Removed
```

Params: `-Phase <Baseline|Installed|Upgraded|Removed>` (required), `-ExpectSeamlyLayout`, `-NoDesktopShortcuts`, `-PatternFile <path>`, `-SkipLaunch`, `-StateFile <path>`.

Upgrade test needs 2 packages from different `-Version` values (2 CI runs) — same `.msi` twice is a repair, not an upgrade.

Asserts: files + Qt runtime slice; each app starts and stays running (only check that catches an incomplete deployed runtime); Start Menu/desktop shortcuts + targets; `HKLM\SOFTWARE\Seamly\Seamly2D` rows; ARP entry incl. size/help links; all 3 associations in registry and via real `.sm2d` open; upgrade leaves exactly one ARP entry with changed version, unmoved dir; uninstall removes all of the above; `Documents\SeamlyData`, `%LOCALAPPDATA%\Seamly`, `%APPDATA%\Seamly`, any old NSIS install survive.

User data check is "never shrank," not "identical" (apps legitimately write settings/seed data on first run). File-association check reports the effective association rather than asserting it — a per-user `UserChoice` can override the HKLM registration.

`test_reset_environment.ps1` resets a test machine past what uninstall leaves (deletes the data root too) — test-support only, more destructive than the shipped uninstall by design.

### Needs human verification (appearance/flow only)

- [ ] Single UAC prompt, verified publisher shown once signed (Task 33)
- [ ] Shortcuts page order and effect (cross-check with `-NoDesktopShortcuts`)
- [ ] Icons: Start Menu, desktop, Explorer, all 3 apps + `.sm2d`/`.smis`/`.smms`
- [ ] seamly2d Layout Mode finds `SeamlyLayout.exe` unconfigured; handoff opens pieces
- [ ] Upgrade-over-older-MSI shows the correct paragraph on the existing-install page
- [ ] NSIS-present machine shows NSIS paragraph naming `C:\Program Files (x86)\Seamly2D`; both entries appear in Apps & features afterward
- [ ] Existing-install page absent on clean machine, on repair, on uninstall
- [ ] arm64: repeat full pass with `seamly-arm64.msi`, `-ExpectSeamlyLayout`

## arm64

Native build on `windows-11-arm` (`windows_arm64` host, `win64_msvc2022_arm64` kit), nothing cross-compiled. `smsi.ps1` runs `wix build -arch arm64`. Qt 6.11.1 ships arm64 WebEngine — re-check at any Qt bump: `aqt list-qt windows_arm64 desktop --modules <version> <arch>`.

## Code signing

CI signs with jsign + Google Cloud KMS, gated on `SEAMLY_SIGNING_*` secrets (skipped when absent, e.g. third-party PRs). See `.github/workflows/CODE_SIGNING.md`.
