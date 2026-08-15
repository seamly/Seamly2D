# Windows MSI — build reference (Seamly2D app family)

What [`smsi.ps1`](smsi.ps1) does with the build trees CI hands it, how to read
its output, and how to install and test the package it produces. For the
*design* decisions (why WiX v6, why one MSI per arch, install layout, upgrade
codes, signing) see [`README.md`](README.md); for the durable build knowledge
base see [`.github/README-BUILDS.md`](../../../.github/README-BUILDS.md).

The MSI bundles all three apps — **seamly2d**, **seamlyme**, and
**SeamlyLayout** — into one per-architecture installer
(`seamly-x64.msi` / `seamly-arm64.msi`).

---

## 1. How the package is built

**By CI, and only by CI.** `smsi.ps1` names every input on the command line and
detects nothing from the machine it runs on.

```powershell
gh workflow run ci.yml --ref run-seamlyLayout
```

`ci.yml`'s `windows-msi` job is a matrix over `arch`. Each leg installs one Qt
6.11.1 kit, builds all three apps natively, and calls:

```powershell
.\scripts\packaging\windows\smsi.ps1 -Arch <arch> -Version $env:VERSION_NUMBER `
  -Seamly2DBin src\app\seamly2d\bin `
  -SeamlyMeBin src\app\seamlyme\bin `
  -WinDeployQt "$env:QT_ROOT_DIR\bin\windeployqt.exe"
```

### Parameters

| Parameter | Required | Default | Notes |
|---|---|---|---|
| `-Arch` | no | `x64` | `x64` or `arm64`. Must match the architecture of the staged binaries. |
| `-Version` | **yes** | — | `YYYY.M.D.HHMM`. The package carries the version of the run that produced it. |
| `-Seamly2DBin` | **yes** | — | Directory holding `seamly2d.exe` **and** its windeployqt output. |
| `-SeamlyMeBin` | **yes** | — | Directory holding `seamlyme.exe` and its windeployqt output. |
| `-WinDeployQt` | **yes** | — | `windeployqt.exe` of the Qt kit SeamlyLayout was built against. |
| `-SeamlyLayoutBuildDir` | no | `src\app\seamlylayout\qt_frontend\build\Release` | Where the job's `cmake --build --preset release` writes `SeamlyLayout.exe`. |
| `-SkipValidation` | no | off | Skips `wix msi validate` only. |
| `-OutputDirName` | no | `seamly-msi` | Staging and output directory under `scripts\`. Changing it also means changing the `.gitignore` entry and `ci.yml`'s artifact and signing paths. |

`VCToolsRedistDir` must be set in the environment by the MSVC developer
environment (`ilammy/msvc-dev-cmd`). It is the only source of the CRT redist
DLLs.

Use `windeployqt`, not `windeployqt6`. A Qt 6 kit ships both names; the project
uses the unsuffixed one everywhere, matching `qtPrepareTool(WINDEPLOYQT,
windeployqt)` in the `.pro` post-link steps.

---

## 2. What the script does

**Checks first, and fails on the first problem**, naming what is missing:
`seamly2d.exe` and its `platforms\` plugin directory, `seamlyme.exe`,
`SeamlyLayout.exe`, the `wix` command, the `WixToolset.UI` and
`WixToolset.Util` extensions, the `-WinDeployQt` path, and a
`Microsoft.VC*.CRT` directory under `VCToolsRedistDir\<arch>`. Then it echoes
every resolved input before touching anything.

1. **Derives the MSI `ProductVersion`** from `-Version` as
   `(YYYY−2000).M.((D−1)·1440 + HH·60 + MM)` — MSI caps the major field at 255,
   so `YYYY.M.D.HHMM` cannot be used directly. The third field is
   minutes-of-month, so the result increases strictly with every build. The
   full project version is stored as `DisplayVersion`.
2. **Stages** a fresh tree under `scripts\seamly-msi\<arch>\`, deleting any
   previous one:
   - `parent\` — the one Qt runtime all three apps share. The seamly2d and
     seamlyme bin trees are copied over each other (same Qt release, so the
     overlapping DLLs are identical), then `windeployqt --qmldir …\qml
     --release` runs against a staged copy of `seamlylayout.exe` and adds the
     QML module tree, the Qt Quick/WebEngine DLLs and `QtWebEngineProcess.exe`.
     SeamlyLayout's four packaged `settings\` JSON files and its LGPL
     `licenses\` notices land here too, and finally the MSVC CRT DLLs.
   - `exes\` — the three executables, moved out of `parent\` after each is
     deployed. The `.wxs` authors them explicitly so shortcuts and associations
     can reference them, which is why they must not be in the
     wildcard-harvested tree.
3. **Runs `wix build`** on `seamly-family.wxs` with `-arch`, `-pdbtype none`,
   both extensions, and the `ProductVersion`, `DisplayVersion`, `RepoRoot`,
   `ParentStagingDir` and `ExeStagingDir` defines → `seamly-<arch>.msi`.
4. **Runs `wix msi validate`** (skip with `-SkipValidation`), suppressing ICE43
   and ICE57 — both are false positives raised by the optional desktop-shortcut
   components; see [`README.md`](README.md).
5. **Runs [`test_msi_authoring.ps1`](test_msi_authoring.ps1)** against the built
   MSI: assertions covering elevation, the ARP properties, upgrade and NSIS
   detection, the install-time dialogs, the shortcuts, the file associations and
   the install-info registry rows. This one is **not** covered by
   `-SkipValidation` — it is cheap and it guards a silent failure mode, an MSI
   that installs perfectly and does the wrong thing.

Only the `.msi` is written: `-pdbtype none` suppresses the `.wixpdb` symbol
database, which is used for `wix` patch/melt diffing rather than by the
installer. To keep it for inspection, drop that flag from `$wixArguments`.

Output and staging live in `scripts\seamly-msi\`, which `.gitignore` lists by
name. Every package carries all three apps — there is no switch to leave
SeamlyLayout out — and because all three come from one Qt 6.11.1 kit, the
staging tree and the install directory are a single flat folder.

### Benign warnings (no action needed)

- `Cannot determine dependencies of …\qtposition_nmea.dll: … Qt6SerialPort.dll` — optional dependency of the NMEA positioning plugin; not used.
- `Cannot find any version of the dxcompiler.dll and dxil.dll` — only needed for Direct3D 12 features.
- `Cannot find Visual Studio installation directory, VCINSTALLDIR is not set` — `windeployqt` cannot deploy the CRT itself; the script deploys it app-locally from `VCToolsRedistDir`.
- `warning WIX1076: ICE61: … Maximum version is not less than the current product` — expected result of `AllowSameVersionUpgrades`.

---

## 3. Installing / testing the MSI

Download the `.msi` from the CI run's artifacts or from the pre-release, then:

```powershell
msiexec /i seamly-x64.msi                       # interactive (license + directory pages)
msiexec /i seamly-x64.msi /qn                   # silent, defaults (needs elevation)
msiexec /i seamly-x64.msi /qn INSTALLFOLDER=D:\SeamlyApps        # silent, custom program dir
msiexec /i seamly-x64.msi /qn SEAMLYDATAPARENT=E:\               # silent, data root E:\SeamlyData
msiexec /i seamly-x64.msi /qn SEAMLYDESKTOPSHORTCUTS=0           # silent, no desktop shortcuts
msiexec /x seamly-x64.msi /qn                   # silent uninstall
msiexec /a seamly-x64.msi /qn TARGETDIR=C:\extract               # extract without installing
```

`msiexec /a` needs a **short** target path: extracting under a long path fails
at `InstallFinalize` with 1603 on MAX_PATH.

### Silent-install properties

| Property | Default | Notes |
|---|---|---|
| `INSTALLFOLDER` | `C:\Program Files\SeamlyApps` | Rejected if the path contains OneDrive, Dropbox, Google Drive, iCloud or Box Sync — a sync client replaces files that are in use, which breaks the program and its uninstall. The check is a launch condition, so it applies to `/qn` too. |
| `SEAMLYDATAPARENT` | `C:\Users\<user>` | Where the `SeamlyData` folder is placed. Setup always appends the `SeamlyData` leaf, so `E:\` gives `E:\SeamlyData`. **Any** drive is allowed, including synced folders and USB media. **Under `/qn` there is no default** — the UI sequence computes it from `%USERPROFILE%`, and `/qn` runs no UI sequence, so pass it explicitly or the apps fall back to their own first-run default. |
| `SEAMLYDATAROOT` | `[SEAMLYDATAPARENT]\SeamlyData` | The composed path. Set it directly to override the composition and name the folder yourself — `SEAMLYDATAROOT=E:\Patterns` gives exactly that. |
| `SEAMLYCOPYUSERDATA` | `0` | Set to `1` to copy existing work into `SEAMLYDATAROOT`. Additive only: nothing is deleted, and a file already at the destination is never overwritten. |
| `SEAMLYDESKTOPSHORTCUTS` | `1` | Desktop shortcuts for Seamly2D and SeamlyMe. |

Why `SEAMLYDATAROOT` has no `/qn` default: a per-machine package runs its
execute sequence elevated as SYSTEM, whose `%USERPROFILE%` is
`C:\Windows\system32\config\systemprofile`. Computing the default there would
put a user's patterns inside a system account's profile.

What the user sees when installing interactively: welcome → license → install folder → **Your work** (the data root, with a Change button) → **Copy your existing work?** (opt-in, default off) → **Shortcuts** (desktop shortcuts, default on) → ready → install. The package defines its own dialog set, so every one of those arrows is a `NewDialog` row `seamly-family.wxs` authors, and `Back` reverses each one.

An extra page appears **before** the welcome page when a previous installation is found — an older MSI of this product or the old NSIS installation — warning that the program files will be replaced and stating that the user's own work is not touched.

Silent installs skip every page. Pass the properties in the table above instead.

The verification of a real install (clean machine, **not yet run** — Task 13's outstanding subtask and Task 51's last one) lives in one place, [`README.md`](README.md#installing--testing), so it does not drift between two files. It is mostly automated, in two layers: `test_msi_authoring.ps1` checks what the **package contains** and runs on every build, and [`test_msi_install.ps1`](test_msi_install.ps1) checks what an **install actually did** — run in four phases around the `msiexec` commands on the test machine, including starting each app to prove the deployed Qt runtime is complete. Only the UAC prompt, the wizard page order and wording, and the icons still need a human.

---

## 4. arm64

All three apps build natively on the `windows-11-arm` runner in `ci.yml`'s
`windows-msi` job — nothing is cross-compiled, and both legs run the identical
`smsi.ps1` invocation. Qt 6.11.1 publishes an arm64 WebEngine, so the arm64
package ships all three apps.

`windeployqt` needs no `--qtpaths` wrapper on either arch, because both legs
are native. Reintroduce that flag only alongside a cross-compiled kit.

---

## 5. Two rules the deployed runtime depends on

**A Qt kit can satisfy `find_package(Qt6 … WebEngineQuick)` and still fail to
deploy.** The kit must also carry Qt WebChannel and Qt Positioning, which is why
`ci.yml` installs `qtwebengine qtwebchannel qtpositioning` on both legs.

**No Qt tool is ever invoked by bare name.** The `.pro` post-link steps use
`qtPrepareTool(WINDEPLOYQT, windeployqt)`, so the deployed runtime can only be
the kit that compiled the exe. `scripts\sb.ps1` and `scripts\sd.ps1` compare the
deployed `Qt6Core.dll` / `Qt6Cored.dll` FileVersion against that kit and fail
loudly on a mismatch — a mismatch is invisible until someone reads the DLL
version by hand.
