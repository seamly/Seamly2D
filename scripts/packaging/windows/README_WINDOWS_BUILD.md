# Building the Windows MSI — walkthrough (Seamly2D app family)

A hands-on, reproduce-it record of building the Windows `.msi` installer with
[`smsi.ps1`](smsi.ps1), including the problems hit on a real run and how they
were worked around. For the *design* decisions (why WiX v6, why one MSI per
arch, install layout, upgrade codes, signing) see [`README.md`](README.md);
for the durable build knowledge base see
[`.github/README-BUILDS.md`](../../../.github/README-BUILDS.md).

The MSI bundles all three apps — **seamly2d**, **seamlyme**, and
**SeamlyLayout** — into one per-architecture installer
(`Seamly-x64.msi` / `Seamly2D-arm64.msi`).

---

## 1. Prerequisites

`smsi.ps1` fails early with a clear message naming whatever is missing. You
need all of:

| Requirement | This machine (2026-07-23) | Check |
|---|---|---|
| Release build of **seamly2d** with windeployqt output | `build\src\app\seamly2d\bin\seamly2d.exe` + `platforms\` | present |
| Release build of **seamlyme** with windeployqt output | `build\src\app\seamlyme\bin\seamlyme.exe` | present |
| Release build of **SeamlyLayout** | `src\app\seamlylayout\qt_frontend\build\Release\SeamlyLayout.exe` | present |
| **WiX v6** CLI + UI extension | `wix 6.0.2`, `WixToolset.UI.wixext 6.0.2` | present |
| **windeployqt6** from SeamlyLayout's Qt kit | Qt **6.11.1** (`C:\Qt\6.11.1\msvc2022_64\bin\windeployqt6.exe`) | present — auto-detected since Task 30/31 (see §2, "What the script does"). The kit must also carry **Qt WebChannel** and **Qt Positioning**, not just Qt WebEngine, or deployment fails — and `build.ps1`'s `Qt6WebEngineQuick` guard does **not** catch that (see the end of §2) |
| **MSVC CRT redistributable** | VS 18 Community (`…\VC\Redist\MSVC\14.50.35710\x64\Microsoft.VC145.CRT`) | present (found by fallback scan) |


The MSVC CRT does **not** require running from a VS developer prompt —
`smsi.ps1` scans installed Visual Studios for the redist folder when
`VCToolsRedistDir` is unset (which it was on this run). windeployqt6 will print
`Cannot find Visual Studio installation directory, VCINSTALLDIR is not set` in
that case; it is harmless because the script deploys the CRT app-locally itself.

---

## 2. The command that produced the MSI

`smsi.ps1` reads the Qt kit out of SeamlyLayout's own `CMakeCache.txt`, so it always deploys the runtime the exe
was linked against:

```powershell
.\scripts\packaging\windows\smsi.ps1
```

Result of the original run (two Qt runtimes; the Task 30 single-runtime MSI is
substantially smaller):

```
MSI OK: …\scripts\seamly-msi\x64\Seamly-x64.msi (186.8 MB)
```

Verified with the Windows Installer COM API:

| Property | Value |
|---|---|
| Platform (summary template) | `x64;1033` |
| ProductName | `Seamly2D` |
| ProductVersion | `26.7.31987` (derived from project version `2026.7.23.0507`) |
| UpgradeCode | `{CBF4B5F1-C32C-4DBB-B385-3EE4A7B30658}` (fixed, shared by both arches) |
| File rows | 1691 |

`wix msi validate` (run automatically by the script) passed with only the
expected **ICE61** warning — a benign consequence of `AllowSameVersionUpgrades`.

### What the script does

1. **Stages** a fresh tree under `scripts\seamly-msi\x64\`:
   - `parent\` — seamly2d + seamlyme windeployqt trees merged (shared Qt runtime, plugins, xerces-c…) + MSVC CRT DLLs, exes removed
   - `layout\` — `SeamlyLayout.exe` with its own Qt runtime deployed by `windeployqt6 --qmldir …\qml --release`, packaged default `settings\`, LGPL `licenses\`, + MSVC CRT DLLs, exe removed
   - `exes\` — the three executables (authored explicitly in the `.wxs` so shortcuts/associations can reference them)
2. Derives the MSI `ProductVersion` from `YYYY.M.D.HHMM` as `(YYYY−2000).M.((D−1)·1440 + HH·60 + MM)` (MSI caps the major field at 255), stores the full project version as `DisplayVersion`.
3. Runs `wix build seamly-family.wxs -arch x64 -ext WixToolset.UI.wixext …` → `Seamly-x64.msi`.
4. Runs `wix msi validate` (skip with `-SkipValidation`), suppressing ICE43 and ICE57 — both are false positives raised by the optional desktop-shortcut components; see [`README.md`](README.md).
5. Runs [`test_msi_authoring.ps1`](test_msi_authoring.ps1) against the built MSI (Task 51): ~50 assertions covering elevation, the ARP properties, upgrade and NSIS detection, the two install-time dialogs, the shortcuts, the file associations and the install-info registry rows. This one is **not** covered by `-SkipValidation` — it is cheap and it guards a silent failure mode, an MSI that installs perfectly and does the wrong thing.

Output and staging live in `scripts\seamly-msi\` (kept out of git by the
`*-build-*` .gitignore pattern).

- `Find-WinDeployQt6` now reads `CMAKE_PREFIX_PATH` from
SeamlyLayout's `build\Release\CMakeCache.txt` — the kit the exe was actually
built against — and falls back to the newest installed `msvc2022_64` kit of any
version. No Qt version is hard-coded, so a future Qt upgrade needs no script
edit. `-WinDeployQt6` still overrides (CI passes it explicitly).

- SeamlyLayout now builds against Qt 6.11.1
(`find_package(Qt6 6.11.1 REQUIRED ...)`), so exe and runtime are the same Qt.

- With all three apps on Qt 6.11.1 the staging tree and the install directory
are one — `smsi.ps1` deploys both windeployqt runs into the same folder and
the `.wxs` harvests a single tree, so the subdirectory and the
duplicate runtime are gone.

- all three `win32-msvc` branches now use `qtPrepareTool(WINDEPLOYQT, windeployqt)`
& `$$WINDEPLOYQT`, matching what the `win32-arm64-msvc` branches already did —
`qtPrepareTool` resolves the tool from `$$[QT_INSTALL_BINS]`, the Qt that qmake
itself belongs to, so the deployed runtime can only ever be the kit that compiled
the exe. `scripts\sb.ps1` and `scripts\sd.ps1` now also compare the deployed
`Qt6Core.dll` / `Qt6Cored.dll` FileVersion against that kit and fail loudly on a
mismatch, because the bug was invisible until someone read the DLL version by hand.
The macOS post-link steps already used `$$[QT_INSTALL_BINS]/macdeployqt` and were
never exposed; CI is unaffected (the runners have no Design Studio, and
`install-qt-action` puts the correct Qt first on `PATH`).

`smsi.ps1 -NoSeamlyLayout` produces a valid two-app package.

Worth knowing: `src\app\seamlylayout\build.ps1`'s guard probes for the `Qt6WebEngineQuick` CMake package, which **was** present throughout, so it passed and the gap only showed up at deployment time. A kit can satisfy `find_package(Qt6 … WebEngineQuick)` and still be unable to deploy.

### Benign warnings (no action needed)

- `Cannot determine dependencies of …\qtposition_nmea.dll: … Qt6SerialPort.dll` — optional dependency of the NMEA positioning plugin; not used.
- `Cannot find any version of the dxcompiler.dll and dxil.dll` — only needed for Direct3D 12 features.
- `Cannot find Visual Studio installation directory, VCINSTALLDIR is not set` — CRT is deployed app-locally by the script instead.
- `warning WIX1076: ICE61: … Maximum version is not less than the current product` — expected result of `AllowSameVersionUpgrades`.

---

## 3. Installing / testing the MSI

```powershell
cd scripts\seamly-msi\x64
msiexec /i seamly-x64.msi                       # interactive (license + directory pages)
msiexec /i seamly-x64.msi /qn                   # silent, defaults (needs elevation)
msiexec /i seamly-x64.msi /qn INSTALLFOLDER=D:\SeamlyApps        # silent, custom program dir
msiexec /i seamly-x64.msi /qn SEAMLYDATAPARENT=E:\               # silent, data root E:\SeamlyData
msiexec /i seamly-x64.msi /qn SEAMLYDESKTOPSHORTCUTS=0           # silent, no desktop shortcuts
msiexec /x seamly-x64.msi /qn                   # silent uninstall
```

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

```
msiexec /a Seamly-x64.msi /qn TARGETDIR=C:\extract        # extract without installing
```

What the user sees when installing interactively: welcome → license → install folder → **Your work** (the data root, with a Change button) → **Copy your existing work?** (opt-in, default off) → **Shortcuts** (desktop shortcuts, default on) → ready → install. The three middle pages are spawned from the install-folder page's Next at Orders 1-3, below WixUI's own transition to the ready page at Order 4.

An extra page appears **before** the welcome page when a previous installation is found — an older MSI of this product or the old NSIS installation — warning that the program files will be replaced and stating that the user's own work is not touched.

Silent installs skip every page. Pass the properties in the table above instead.

The verification of a real install (clean machine, **not yet run** — Task 13's outstanding subtask and Task 51's last one) lives in one place, [`README.md`](README.md#installing--testing), so it does not drift between two files. It is now mostly automated, in two layers: `test_msi_authoring.ps1` checks what the **package contains** and runs on every build, and [`test_msi_install.ps1`](test_msi_install.ps1) checks what an **install actually did** — run in four phases around the `msiexec` commands on the test machine, including starting each app to prove the deployed Qt runtime is complete. Only the UAC prompt, the wizard page order and wording, and the icons still need a human.

---

## 4. arm64

```powershell
.\scripts\packaging\windows\smsi.ps1 -Arch arm64 -NoSeamlyLayout   # needs arm64 build trees
```

All three apps build natively on the `windows-11-arm` runner in CI
(`ci.yml`'s `windows-msi` job) — nothing is cross-compiled. The arm64 MSI
shipped the two parent apps only (`-NoSeamlyLayout`) until 2026-08-11; Qt
6.11.1 publishes an arm64 WebEngine, so it now ships all three. Not built on
this machine (no arm64 build trees present).

---

## 5. CI equivalent

`ci.yml`'s `windows-msi` job runs the same `smsi.ps1` against its
in-source CI build output (matrix x64/arm64), installs the required Qt kits,
signs the MSI with `jsign`/Google Cloud KMS when the `SEAMLY_SIGNING_*` secrets
are present (unsigned otherwise), and uploads the MSI artifact.
