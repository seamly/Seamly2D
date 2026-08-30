# Windows MSI installer — Seamly app suite (Task 13)

WiX build reference for the Windows `.msi` shipping **seamly2d.exe**, **seamlyme.exe**, **seamlylayout.exe** together, per arch (x64, arm64).

Related docs: [`.github/README-BUILDS.md`](../../.github/README-BUILDS.md) (KB record), [`README_WINDOWS_BUILD.md`](README_WINDOWS_BUILD.md) (build mechanics), [`INSTALL_DECISION_FLOW.md`](INSTALL_DECISION_FLOW.md) (installer-vs-app flowcharts).

## Files

| File | Purpose |
|---|---|
| `smsi.wxs` | Package identity, upgrade, ARP, launch conditions, fragment refs |
| `smsi_ui.wxs` | Wizard dialogs and transitions |
| `smsi_legacy.wxs` | Finds/removes pre-MSI (NSIS) install |
| `smsi_files.wxs` | Directory tree, exes, shortcuts, file associations |
| `smsi_shortcuts.wxs` | Optional desktop shortcuts |
| `smsi_registry.wxs` | Install-info registry, per-user settings cleanup |
| `*.ico` | Shortcut/ARP icons; `<Icon Id>` must equal file name |
| `license.rtf` | License text shown in installer UI |
| `smsi.ps1` | Stage + `wix build` driver. CI-only, no local mode |
| `build_msi_local.ps1` | Local x64 dev build: builds all 3 apps, calls `smsi.ps1` |
| `smsi_check_authoring.ps1` | Asserts built MSI contents; runs every build |
| `smsi_migrate_user_data.ps1` / `..._test.ps1` | Deferred action migrating user data during install/upgrade, + its unit tests |
| `test_msi_install.ps1` | Asserts an installed MSI's effect on a real machine |
| `test_reset_environment.ps1` | Test-support: wipes a machine past uninstall |
| `ci.yml` | Only CI route to a Windows package |

Editing any file here triggers full CI (no `paths-ignore` match).

## Key decisions

- **WiX v6**, not v7 (v7 requires accepting its EULA). Extension version must match core.
- **One MSI per arch**, not per-app. Output: `packaging\windows\seamly-msi\<arch>\seamly-<arch>.msi`.
- **One flat install dir**: `[ProgramFiles64Folder]\SeamlyApps\` holds all 3 exes + one shared Qt runtime.
- **CRT deployed app-local** from `VCToolsRedistDir`, not merge modules.
- **CI-only build.** Run via `gh workflow run ci.yml --ref run-seamlyLayout`.
- **`windeployqt`, never `windeployqt6`** — matches the `.pro` files.
- **Upgrade code `cbf4b5f1-c32c-4dbb-b385-3ee4a7b30658` is fixed forever.** Never change it.
- **ProductVersion** = `YY.M.((D-1)*1440 + MMMM)`. Full version stored as `DisplayVersion` in `HKLM\SOFTWARE\Seamly\Seamly2D`.
- **File associations**: `.sm2d` → Seamly2D, `.smis`/`.smms` → SeamlyMe. None for SeamlyLayout.
- **Registry rows mirror to all 3 apps** under `HKLM\SOFTWARE\Seamly\<App>`. Seamly2D's key is canonical.
- **Start Menu**: 3 shortcuts at root, no folder.
- **Data root**: created at install; apps create subdirs on first launch. Uninstall keeps the root.
- **Update migration**: preserves non-path settings; replaces path settings only after copy verifies.

Details and rationale: [`README_WINDOWS_BUILD.md`](README_WINDOWS_BUILD.md).

## Install-time dialogs

Fresh install: Welcome → License → Existing-install warning (if found) → Program dir → Data root → Copy-existing-work? (if applicable) → Shortcuts → Ready → Progress → Finish. Maintenance/repair/uninstall: Maintenance Welcome → Maintenance Type → Ready.

`ARPNOMODIFY` is set — Windows Installer can't move an installed product. To relocate: uninstall/reinstall, or a major upgrade.

### Silent-install properties

| Property | Default | Notes |
|---|---|---|
| `INSTALLFOLDER` | prior path, else `C:\Program Files\SeamlyApps` | rejected if under OneDrive/Dropbox/Google Drive/iCloud/Box Sync |
| `SEAMLYDATAPARENT` | `C:\Users\<user>\Documents` | no default under `/qn` — pass explicitly or apps use their own default |
| `SEAMLYDATAROOT` | `[SEAMLYDATAPARENT]\SeamlyData` | overrides the composed path |
| `SEAMLYCOPYUSERDATA` | `0` | `1` migrates existing work; never overwrites existing files |
| `SEAMLYDESKTOPSHORTCUTS` | `1` | desktop shortcuts for all 3 apps |

## Building

**Release (CI only):** `gh workflow run ci.yml --ref run-seamlyLayout`. `windows-msi` matrix: x64 on `windows-latest`, arm64 (native) on `windows-11-arm`.

**Local x64 dev build:** `.\packaging\windows\build_msi_local.ps1`. Builds all 3 apps release, auto-detects Qt kit, installs WiX v6 if missing. Not a release artifact.

Full param reference: [`README_WINDOWS_BUILD.md`](README_WINDOWS_BUILD.md).

### Getting the MSI

Take it from a **release**, not the Actions page (build artifacts download as `.msi.zip`).

| Source | Trigger | Contents |
|---|---|---|
| Release `dev-latest` | every push to `run-seamlyLayout` | x64 + arm64 MSIs |
| Release `v<version>` | `schedule`/`workflow_dispatch` | same 2 MSIs + Linux/macOS builds |

`gh release download dev-latest --repo seamly/Seamly2D --pattern 'seamly-x64.msi'`

## Installing / testing

```powershell
msiexec /i seamly-x64.msi                                        # interactive
msiexec /i seamly-x64.msi /qn                                     # silent, defaults
msiexec /i seamly-x64.msi /qn INSTALLFOLDER=D:\SeamlyApps         # silent, custom program dir
msiexec /i seamly-x64.msi /qn SEAMLYDATAPARENT=E:\                # silent, data root E:\SeamlyData
msiexec /x seamly-x64.msi /qn                                     # silent uninstall
```

### `test_msi_install.ps1`

4 phases sharing a state file, so later phases assert against earlier ones. Standalone — copy beside the `.msi`, run elevated.

```powershell
.\test_msi_install.ps1 -Phase Baseline
msiexec /i seamly-x64-older.msi
.\test_msi_install.ps1 -Phase Installed -ExpectSeamlyLayout -PatternFile .\sample.sm2d
msiexec /i seamly-x64-newer.msi
.\test_msi_install.ps1 -Phase Upgraded -ExpectSeamlyLayout -PatternFile .\sample.sm2d
msiexec /x seamly-x64-newer.msi
.\test_msi_install.ps1 -Phase Removed
```

Params: `-Phase <Baseline|Installed|Upgraded|Removed>` (required), `-ExpectSeamlyLayout`, `-NoDesktopShortcuts`, `-PatternFile <path>`, `-SkipLaunch`, `-StateFile <path>`.

Upgrade test needs 2 packages with different `-Version` values (2 CI runs).

`test_reset_environment.ps1` resets a test machine past what uninstall leaves — test-support only, more destructive than the shipped uninstall by design.

### Needs human verification

- [ ] Single UAC prompt, verified publisher shown once signed
- [ ] Shortcuts page order and effect
- [ ] Icons: Start Menu, desktop, Explorer, all 3 apps + file associations
- [ ] seamly2d Layout Mode finds `SeamlyLayout.exe` unconfigured; handoff opens pieces
- [ ] Upgrade-over-older-MSI shows correct existing-install paragraph
- [ ] NSIS-present machine shows NSIS paragraph; both entries appear in Apps & features
- [ ] Existing-install page absent on clean machine, on repair, on uninstall
- [ ] arm64: repeat full pass with `seamly-arm64.msi`, `-ExpectSeamlyLayout`

## arm64

Native build on `windows-11-arm`, nothing cross-compiled. Re-check Qt arm64 WebEngine availability at any Qt bump.

## Code signing

CI signs with jsign + Google Cloud KMS, gated on `SEAMLY_SIGNING_*` secrets (skipped when absent). See `.github/workflows/CODE_SIGNING.md`.
