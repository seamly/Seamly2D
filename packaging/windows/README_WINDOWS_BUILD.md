# Windows MSI — build reference (Seamly application suite)

Covers [`smsi.ps1`](smsi.ps1): inputs, steps, install/test commands. Design decisions: [`README.md`](README.md). Build/toolchain reference: [`.github/README-BUILDS.md`](../../.github/README-BUILDS.md).

One MSI per arch bundles **seamly2d**, **seamlyme**, **SeamlyLayout** (`seamly-x64.msi` / `seamly-arm64.msi`).

## 1. Building

**CI only.** `smsi.ps1` takes every input on the command line; detects nothing from the host.

```powershell
gh workflow run ci.yml --ref run-seamlyLayout
```

`ci.yml`'s `windows-msi` job matrixes over `arch`. Each leg builds all three apps, then calls:

```powershell
.\packaging\windows\smsi.ps1 -Arch <arch> -Version $env:VERSION_NUMBER `
  -Seamly2DBin src\app\seamly2d\bin `
  -SeamlyMeBin src\app\seamlyme\bin `
  -WinDeployQt "$env:QT_ROOT_DIR\bin\windeployqt.exe"
```

| Parameter | Required | Default | Notes |
|---|---|---|---|
| `-Arch` | no | `x64` | `x64` or `arm64`; must match staged binaries. |
| `-Version` | yes | — | `YY.M.DDHH` (`DDHH` = day × 100 + hour). |
| `-Seamly2DBin` | yes | — | Dir with `seamly2d.exe` + windeployqt output. |
| `-SeamlyMeBin` | yes | — | Dir with `seamlyme.exe` + windeployqt output. |
| `-WinDeployQt` | yes | — | `windeployqt.exe` from the kit SeamlyLayout built against. |
| `-SeamlyLayoutBuildDir` | no | `src\app\seamlylayout\qt_frontend\build\Release` | Where `cmake --build --preset release` writes `seamlylayout.exe`. |
| `-SkipValidation` | no | off | Skips `wix msi validate` only. |
| `-OutputDirName` | no | `seamly-msi` | Rename needs matching `.gitignore` and `ci.yml` changes. |

- `VCToolsRedistDir` must be set by `ilammy/msvc-dev-cmd` — the only CRT redist source.
- Use `windeployqt`, not `windeployqt6` (matches `.pro` files' `qtPrepareTool(WINDEPLOYQT, windeployqt)`).

## arm64 

Native build on `windows-11-arm`, nothing cross-compiled. Re-check Qt arm64 WebEngine availability at any Qt bump.

## Code signing (ci.yml)

CI signs with jsign + Google Cloud KMS, gated on `SEAMLY_SIGNING_*` secrets (skipped when absent). See `.github/workflows/CODE_SIGNING.md`.

### Local dev build

[`local_build_msi.ps1`](local_build_msi.ps1) runs the same `smsi.ps1` call on a
dev machine. It is the only local Windows build script. `src\app\seamlylayout\build.ps1`
and `qd.ps1`, which built SeamlyLayout alone, are removed (see
`packaging\linux\local_build_appimage.sh` and `packaging\macos\local_build_dmg.sh`
for the Linux/macOS equivalents of this script).

```powershell
.\packaging\windows\local_build_msi.ps1
```

1. Stamps `-Version` (default: computed from the current local time) into
   `projectversion.{h,cpp}` and both `Info.plist` files, via `packaging\version.sh`.
   Reverts the stamp after a successful build, unless those files already
   carried uncommitted changes before the run.
2. Finds the newest `msvc2022_64` Qt kit at or above 6.11.1 under `C:\Qt`;
   installs WiX v6 and its UI/Util extensions if missing.
3. Under one `vcvars64.bat` environment: `qmake Seamly.pro -r -config release`
   and `nmake` build seamly2d and seamlyme; `nmake check` runs the four Qt
   test suites; `cmake --preset release` and `cmake --build --preset release`
   build SeamlyLayout.
4. Calls `smsi.ps1` with the built binaries — see "What the script does" below.

| Parameter | Required | Default | Notes |
|---|---|---|---|
| `-Version` | no | computed from local time | Same `YY.M.DDHH` form `smsi.ps1` expects. |
| `-SkipTests` | no | off | Builds with `CONFIG+=noTests`; skips `nmake check`. Only for a packaging-only change — CI is the only other runner for these suites. |
| `-SkipValidation` | no | off | Passed through to `smsi.ps1`. |

Treat the MSI it produces as a local dev build, not a release artifact —
releases still go through `gh workflow run ci.yml`.

## 2. What the script does

Checks first, fails on the first missing item: both exes, `seamly2d`'s `platforms\` dir, `seamlylayout.exe`, `wix`, the WiX UI/Util extensions, `-WinDeployQt`, and a `Microsoft.VC*.CRT` dir under `VCToolsRedistDir\<arch>`.

1. **Checks `-Version`** and uses it unchanged as `ProductVersion` (`YY.M.DDHH`). Builds sort by hour; two builds of one hour tie and replace each other. `MajorUpgrade` has `AllowDowngrades`; `SeamlyNewerVersionDlg` asks before it removes a newer version.
2. **Stages** `packaging\windows\seamly-msi\<arch>\`: `parent\` (shared Qt runtime + `windeployqt --qmldir …\qml --release` for SeamlyLayout, its `settings\`/`licenses\`, MSVC CRT DLLs) and `exes\` (the three exes, moved out of `parent\` after deployment so `.wxs` can author them explicitly for shortcuts/associations).
3. **`wix build`** on every `*.wxs` file in this directory (globbed, not hard-coded — `smsi.wxs` plus its five fragments) → `seamly-<arch>.msi` (`-pdbtype none`, no `.wixpdb`).
4. **[`smsi_fix_dialog_lines.ps1`](smsi_fix_dialog_lines.ps1)** — trims WixUI's stock banner/bottom line controls back inside the dialog width (they overflow by 3 installer units, logging Error 2826). Runs on the built package, before validation.
5. **`wix msi validate`** (skip with `-SkipValidation`), suppressing ICE43/57 — false positives from optional desktop-shortcut components.
6. **[`smsi_check_authoring.ps1`](smsi_check_authoring.ps1)** — asserts elevation, ARP properties, upgrade/NSIS detection, dialogs, shortcuts, associations, registry rows. Always runs, even with `-SkipValidation`.
7. **[`smsi_ensure_user_data_test.ps1`](smsi_ensure_user_data_test.ps1)** — unit tests for the data-root/settings-seeding deferred action. Always runs.

All three apps ship in every package

### Benign warnings

- `qtposition_nmea.dll` dependency warning — unused NMEA plugin.
- `dxcompiler.dll`/`dxil.dll` not found — Direct3D 12 only.
- `VCINSTALLDIR is not set` — expected; script deploys CRT app-locally.
- `ICE61: Maximum version is not less than the current product` — expected with `AllowDowngrades`.

## 3. Installing / testing

```powershell
msiexec /i seamly-x64.msi                                          # interactive
msiexec /i seamly-x64.msi /qn                                       # silent, defaults (needs elevation)
msiexec /i seamly-x64.msi /qn SEAMLYDATAROOT=E:\seamly2d             # silent, explicit data root
msiexec /i seamly-x64.msi /qn SEAMLYDESKTOPSHORTCUTS=0                # silent, no desktop shortcuts
msiexec /x seamly-x64.msi /qn                                       # silent uninstall
msiexec /a seamly-x64.msi /qn TARGETDIR=C:\extract                   # extract without installing
```

`msiexec /a` needs a **short** target path — a long path fails at `InstallFinalize` with 1603 on MAX_PATH.

| Property | Default | Notes |
|---|---|---|
| `INSTALLFOLDER` | always `%ProgramFiles%\SeamlyApps` | Fixed; not a wizard page and not overridable, even under `/qn`. |
| `SEAMLYDATAROOT` | `%USERPROFILE%\seamly2d` on a fresh install; the recorded root on an update/repair | Fixed, not chosen. Set explicitly on the command line only to override the fresh-install default. |
| `SEAMLYDESKTOPSHORTCUTS` | `1` | Desktop shortcuts for Seamly2D/SeamlyMe. |

- The data root is never asked about on a fresh install — Setup creates `%USERPROFILE%\seamly2d` and fills it in silently. Setup records the value at `HKLM\SOFTWARE\Seamly\Seamly2D\DataRoot`. An update or repair reuses that recorded value verbatim and only adds anything missing; existing data is never moved, copied, or overwritten.
- Moving an installed Seamly is **not supported** — location is fixed at install time. Uninstall/reinstall, or run a major upgrade (prefills the program-directory page from `HKLM\SOFTWARE\Seamly\Seamly2D\InstallPath`).
- Interactive pages: a gate dialog (Continue) → welcome → license → [previous-install warning] → [data location, read-only] → shortcuts (on) → ready → install → finish. The gate holds the wizard until Continue, so detection never flashes under the welcome page. No install-folder page — `INSTALLFOLDER` is fixed and never asked about. The previous-install page appears only when a prior install (this MSI or the old NSIS installer) is found; the data-location page appears only when an earlier install already recorded a `DataRoot` — a true fresh install shows neither. The finish page offers a "Launch Seamly2D now" checkbox, checked by default; it starts seamly2d.exe as the signed-in user, not elevated. `/qn` skips all pages and never launches an app.
- Real-install verification: [`README.md`](README.md#installing--testing). `smsi_check_authoring.ps1` checks package contents; [`local_install_msi.ps1`](local_install_msi.ps1) checks install effects, incl. launching each app. Only the UAC prompt, wizard wording, and icons need a human.

## 4. arm64

Both `x64` and `arm64` build natively (`windows-11-arm` runner); nothing is cross-compiled, and both legs run the identical `smsi.ps1` invocation. Qt 6.11.1 ships an arm64 WebEngine, so the arm64 package includes all three apps. `windeployqt` needs no `--qtpaths` wrapper on either arch — add it back only for a cross-compiled kit.

## 5. Runtime rules

- A Qt kit satisfying `find_package(Qt6 … WebEngineQuick)` can still fail to deploy — it must also carry Qt WebChannel and Qt Positioning, hence `ci.yml` installs `qtwebengine qtwebchannel qtpositioning` on both legs.
- No Qt tool is invoked by bare name (`qtPrepareTool(WINDEPLOYQT, windeployqt)` in the `.pro` files), so the deployed runtime is always the kit that compiled the exe. `scripts\sb.ps1`/`sd.ps1` used to check the deployed `Qt6Core.dll` FileVersion against the kit; both were deleted August 2026. **Check that FileVersion by hand before packaging a local tree** — a mismatch is otherwise invisible.
