# Windows MSI — build reference (Seamly application suite)

Covers [`smsi.ps1`](smsi.ps1): inputs, steps, install/test commands. Design decisions: [`README.md`](README.md). Build/toolchain reference: [`.github/README-BUILDS.md`](../../../.github/README-BUILDS.md).

One MSI per arch bundles **seamly2d**, **seamlyme**, **SeamlyLayout** (`seamly-x64.msi` / `seamly-arm64.msi`).

## 1. Building

**CI only.** `smsi.ps1` takes every input on the command line; detects nothing from the host.

```powershell
gh workflow run ci.yml --ref run-seamlyLayout
```

`ci.yml`'s `windows-msi` job matrixes over `arch`. Each leg builds all three apps, then calls:

```powershell
.\scripts\packaging\windows\smsi.ps1 -Arch <arch> -Version $env:VERSION_NUMBER `
  -Seamly2DBin src\app\seamly2d\bin `
  -SeamlyMeBin src\app\seamlyme\bin `
  -WinDeployQt "$env:QT_ROOT_DIR\bin\windeployqt.exe"
```

| Parameter | Required | Default | Notes |
|---|---|---|---|
| `-Arch` | no | `x64` | `x64` or `arm64`; must match staged binaries. |
| `-Version` | yes | — | `YY.M.D.MMMM` (`MMMM` = minute of day). |
| `-Seamly2DBin` | yes | — | Dir with `seamly2d.exe` + windeployqt output. |
| `-SeamlyMeBin` | yes | — | Dir with `seamlyme.exe` + windeployqt output. |
| `-WinDeployQt` | yes | — | `windeployqt.exe` from the kit SeamlyLayout built against. |
| `-SeamlyLayoutBuildDir` | no | `src\app\seamlylayout\qt_frontend\build\Release` | Where `cmake --build --preset release` writes `SeamlyLayout.exe`. |
| `-SkipValidation` | no | off | Skips `wix msi validate` only. |
| `-OutputDirName` | no | `seamly-msi` | Rename needs matching `.gitignore` and `ci.yml` changes. |

- `VCToolsRedistDir` must be set by `ilammy/msvc-dev-cmd` — the only CRT redist source.
- Use `windeployqt`, not `windeployqt6` (matches `.pro` files' `qtPrepareTool(WINDEPLOYQT, windeployqt)`).

## 2. What the script does

Checks first, fails on the first missing item: both exes, `seamly2d`'s `platforms\` dir, `SeamlyLayout.exe`, `wix`, the WiX UI/Util extensions, `-WinDeployQt`, and a `Microsoft.VC*.CRT` dir under `VCToolsRedistDir\<arch>`.

1. **Derives `ProductVersion`**: `YY.M.((D−1)·1440 + MMMM)` — MSI ignores the 4th field for upgrade comparisons; this always increases. Full version is stored as `DisplayVersion`.
2. **Stages** `scripts\seamly-msi\<arch>\`: `parent\` (shared Qt runtime + `windeployqt --qmldir …\qml --release` for SeamlyLayout, its `settings\`/`licenses\`, MSVC CRT DLLs) and `exes\` (the three exes, moved out of `parent\` after deployment so `.wxs` can author them explicitly for shortcuts/associations).
3. **`wix build`** on `smsi.wxs` → `seamly-<arch>.msi` (`-pdbtype none`, no `.wixpdb`).
4. **`wix msi validate`** (skip with `-SkipValidation`), suppressing ICE43/57 — false positives from optional desktop-shortcut components.
5. **[`smsi_check_authoring.ps1`](smsi_check_authoring.ps1)** — asserts elevation, ARP properties, upgrade/NSIS detection, dialogs, shortcuts, associations, registry rows. Always runs, even with `-SkipValidation`.

All three apps ship in every package — no switch to omit SeamlyLayout.

### Benign warnings

- `qtposition_nmea.dll` dependency warning — unused NMEA plugin.
- `dxcompiler.dll`/`dxil.dll` not found — Direct3D 12 only.
- `VCINSTALLDIR is not set` — expected; script deploys CRT app-locally.
- `ICE61: Maximum version is not less than the current product` — expected with `AllowSameVersionUpgrades`.

## 3. Installing / testing

```powershell
msiexec /i seamly-x64.msi                                          # interactive
msiexec /i seamly-x64.msi /qn                                       # silent, defaults (needs elevation)
msiexec /i seamly-x64.msi /qn INSTALLFOLDER=D:\SeamlyApps            # silent, custom program dir
msiexec /i seamly-x64.msi /qn SEAMLYDATAPARENT=E:\                   # silent, data root E:\SeamlyData
msiexec /i seamly-x64.msi /qn SEAMLYDESKTOPSHORTCUTS=0                # silent, no desktop shortcuts
msiexec /x seamly-x64.msi /qn                                       # silent uninstall
msiexec /a seamly-x64.msi /qn TARGETDIR=C:\extract                   # extract without installing
```

`msiexec /a` needs a **short** target path — a long path fails at `InstallFinalize` with 1603 on MAX_PATH.

| Property | Default | Notes |
|---|---|---|
| `INSTALLFOLDER` | previous path, else `C:\Program Files\SeamlyApps` | Rejected if under a sync-client folder (OneDrive/Dropbox/Google Drive/iCloud/Box Sync), even under `/qn`. |
| `SEAMLYDATAPARENT` | `C:\Users\<user>\Documents` | `SeamlyData` leaf always appended. **No `/qn` default** — pass it explicitly. |
| `SEAMLYDATAROOT` | `[SEAMLYDATAPARENT]\SeamlyData` | Set directly to override, e.g. `SEAMLYDATAROOT=E:\Patterns`. |
| `SEAMLYCOPYUSERDATA` | `0` | Set `1` on update to archive/migrate work into `SEAMLYDATAROOT`. Never overwrites existing files. |
| `SEAMLYDESKTOPSHORTCUTS` | `1` | Desktop shortcuts for Seamly2D/SeamlyMe. |

- `SEAMLYDATAROOT` has no `/qn` default: the execute sequence runs elevated as SYSTEM, so a computed default would misplace user data. Setup records the answer at `HKLM\SOFTWARE\Seamly\Seamly2D\DataRoot` on first run; unset stays empty and apps use their own default. Repair keeps the recorded value.
- Moving an installed Seamly is **not supported** — location is fixed at install time. Uninstall/reinstall, or run a major upgrade (prefills the program-directory page from `HKLM\SOFTWARE\Seamly\Seamly2D\InstallPath`).
- Data migration on update runs only when `SEAMLYCOPYUSERDATA=1` or the data location changed. Non-path settings are always preserved.
- Interactive pages: welcome → license → install folder → data root → copy existing work? (off) → shortcuts (on) → ready → install. A warning page precedes welcome if a prior install (this MSI or the old NSIS installer) is found. `/qn` skips all pages.
- Real-install verification (clean machine — **not yet run**; Task 13/51): [`README.md`](README.md#installing--testing). `smsi_check_authoring.ps1` checks package contents; [`test_msi_install.ps1`](test_msi_install.ps1) checks install effects, incl. launching each app. Only the UAC prompt, wizard wording, and icons need a human.

## 4. arm64

Both `x64` and `arm64` build natively (`windows-11-arm` runner); nothing is cross-compiled, and both legs run the identical `smsi.ps1` invocation. Qt 6.11.1 ships an arm64 WebEngine, so the arm64 package includes all three apps. `windeployqt` needs no `--qtpaths` wrapper on either arch — add it back only for a cross-compiled kit.

## 5. Runtime rules

- A Qt kit satisfying `find_package(Qt6 … WebEngineQuick)` can still fail to deploy — it must also carry Qt WebChannel and Qt Positioning, hence `ci.yml` installs `qtwebengine qtwebchannel qtpositioning` on both legs.
- No Qt tool is invoked by bare name (`qtPrepareTool(WINDEPLOYQT, windeployqt)` in the `.pro` files), so the deployed runtime is always the kit that compiled the exe. `scripts\sb.ps1`/`sd.ps1` used to check the deployed `Qt6Core.dll` FileVersion against the kit; both were deleted August 2026. **Check that FileVersion by hand before packaging a local tree** — a mismatch is otherwise invisible.
