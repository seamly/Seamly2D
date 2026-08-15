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

**By CI, and only by CI.** `smsi.ps1` has no local-build mode: it names every
input on the command line and detects nothing from the machine it runs on.

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

`-Version`, `-Seamly2DBin`, `-SeamlyMeBin` and `-WinDeployQt` are required.
A Qt 6 kit ships both `windeployqt.exe` and `windeployqt6.exe`; the project
uses the unsuffixed name everywhere, matching `qtPrepareTool(WINDEPLOYQT,
windeployqt)` in the `.pro` post-link steps.
`VCToolsRedistDir` must be set by the MSVC developer environment
(`ilammy/msvc-dev-cmd`) — it is the only source of the CRT redist DLLs.
The script fails early with a clear message naming whatever is missing.

The removed local mode defaulted the build trees to `build\`, defaulted the
version to the current time, guessed the Qt kit from `CMakeCache.txt` or the
newest `C:\Qt` install, and scanned installed Visual Studios for the CRT. Each
of those could produce a package that installs perfectly and ships a runtime no
app in it was built against; see [`README.md`](README.md#key-decisions).

---

## 2. What the script does

1. **Stages** a fresh tree under `scripts\seamly-msi\<arch>\`:
   - `parent\` — the one Qt runtime all three apps share: seamly2d's and
     seamlyme's windeployqt output merged with SeamlyLayout's
     `windeployqt --qmldir …\qml --release` output (QML modules, Qt
     Quick/WebEngine DLLs, `QtWebEngineProcess.exe`, xerces-c), plus
     SeamlyLayout's packaged `settings\`, its LGPL `licenses\`, and the MSVC
     CRT DLLs
   - `exes\` — the three executables (authored explicitly in the `.wxs` so
     shortcuts and associations can reference them, and therefore kept out of
     the wildcard-harvested tree above)
2. Derives the MSI `ProductVersion` from `-Version` (`YYYY.M.D.HHMM`) as
   `(YYYY−2000).M.((D−1)·1440 + HH·60 + MM)` — MSI caps the major field at 255
   — and stores the full project version as `DisplayVersion`.
3. Runs `wix build seamly-family.wxs -arch <arch> -ext WixToolset.UI.wixext …`
   → `seamly-<arch>.msi`.
4. Runs `wix msi validate` (skip with `-SkipValidation`), suppressing ICE43 and
   ICE57 — both are false positives raised by the optional desktop-shortcut
   components; see [`README.md`](README.md).
5. Runs [`test_msi_authoring.ps1`](test_msi_authoring.ps1) against the built
   MSI: 63 assertions covering elevation, the ARP properties, upgrade and NSIS
   detection, the install-time dialogs, the shortcuts, the file associations
   and the install-info registry rows. This one is **not** covered by
   `-SkipValidation` — it is cheap and it guards a silent failure mode, an MSI
   that installs perfectly and does the wrong thing.

Output and staging live in `scripts\seamly-msi\`, which `.gitignore` lists by
name. Every package carries all three apps: `-NoSeamlyLayout` and the `.wxs`
`IncludeSeamlyLayout` guards were removed on 2026-08-15, so a two-app package
can no longer be built.

Because all three apps are built from one Qt 6.11.1 kit, the staging tree and
the install directory are a single flat folder; the pre-Task-30 `layout\`
subtree with its own duplicate Qt copy is gone.

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

What the user sees when installing interactively: welcome → license → install folder → **Your work** (the data root, with a Change button) → **Copy your existing work?** (opt-in, default off) → **Shortcuts** (desktop shortcuts, default on) → ready → install. The three middle pages are spawned from the install-folder page's Next at Orders 1-3, below WixUI's own transition to the ready page at Order 4.

An extra page appears **before** the welcome page when a previous installation is found — an older MSI of this product or the old NSIS installation — warning that the program files will be replaced and stating that the user's own work is not touched.

Silent installs skip every page. Pass the properties in the table above instead.

The verification of a real install (clean machine, **not yet run** — Task 13's outstanding subtask and Task 51's last one) lives in one place, [`README.md`](README.md#installing--testing), so it does not drift between two files. It is now mostly automated, in two layers: `test_msi_authoring.ps1` checks what the **package contains** and runs on every build, and [`test_msi_install.ps1`](test_msi_install.ps1) checks what an **install actually did** — run in four phases around the `msiexec` commands on the test machine, including starting each app to prove the deployed Qt runtime is complete. Only the UAC prompt, the wizard page order and wording, and the icons still need a human.

---

## 4. arm64

All three apps build natively on the `windows-11-arm` runner in `ci.yml`'s
`windows-msi` job — nothing is cross-compiled, and both legs run the identical
`smsi.ps1` invocation. The arm64 MSI shipped the two parent apps only until
2026-08-11; Qt 6.11.1 publishes an arm64 WebEngine, so it now ships all three.

`windeployqt` needs no `--qtpaths` wrapper on either arch, because both legs
are native. Reintroduce that flag only alongside a cross-compiled kit.

---

## 5. Historical record — the last local build (2026-07-23)

Kept for the measurements, not as instructions: local building was removed on
2026-08-15 and the command below no longer runs.

| Property | Value |
|---|---|
| Package | `scripts\seamly-msi\x64\Seamly-x64.msi`, 186.8 MB (two Qt runtimes; the Task 30 single-runtime package is 165.3 MB) |
| Platform (summary template) | `x64;1033` |
| ProductName | `Seamly2D` |
| ProductVersion | `26.7.31987` (derived from project version `2026.7.23.0507`) |
| UpgradeCode | `{CBF4B5F1-C32C-4DBB-B385-3EE4A7B30658}` (fixed, shared by both arches) |
| File rows | 1691 |
| Qt kit | 6.11.1 `msvc2022_64` |
| MSVC CRT | VS 18 Community, `…\VC\Redist\MSVC\14.50.35710\x64\Microsoft.VC145.CRT` |

`wix msi validate` passed with only the expected **ICE61** warning.

Two lessons from that run are still live rules. **A Qt kit can satisfy
`find_package(Qt6 … WebEngineQuick)` and still fail to deploy** — the kit must
also carry Qt WebChannel and Qt Positioning, which is why `ci.yml` installs
`qtwebengine qtwebchannel qtpositioning` on both legs. And **no Qt tool is ever
invoked by bare name**: the `.pro` post-link steps use
`qtPrepareTool(WINDEPLOYQT, windeployqt)`, so the deployed runtime can only be
the kit that compiled the exe. `scripts\sb.ps1` and `scripts\sd.ps1` compare
the deployed `Qt6Core.dll` / `Qt6Cored.dll` FileVersion against that kit and
fail loudly on a mismatch, because the bug was invisible until someone read the
DLL version by hand.
