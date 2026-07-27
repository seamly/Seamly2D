# Building the Windows MSI — walkthrough (Seamly2D app family)

A hands-on, reproduce-it record of building the Windows `.msi` installer with
[`smsi.ps1`](smsi.ps1), including the problems hit on a real run and how they
were worked around. For the *design* decisions (why WiX v6, why one MSI per
arch, install layout, upgrade codes, signing) see [`README.md`](README.md);
for the durable build knowledge base see
[`.github/README-BUILDS.md`](../../../.github/README-BUILDS.md).

The MSI bundles all three apps — **seamly2d**, **seamlyme**, and
**SeamlyLayout** — into one per-architecture installer
(`Seamly2D-x64.msi` / `Seamly2D-arm64.msi`).

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
| **windeployqt6** from SeamlyLayout's Qt kit | Qt **6.11.1** (`C:\Qt\6.11.1\msvc2022_64\bin\windeployqt6.exe`) | present — auto-detected since Task 30/31 (see §3.1) |
| **MSVC CRT redistributable** | VS 18 Community (`…\VC\Redist\MSVC\14.50.35710\x64\Microsoft.VC145.CRT`) | present (found by fallback scan) |

Install the pieces that are missing:

```powershell
# WiX v6 (pinned — v7 is gated behind the OSMF EULA, error WIX7015)
dotnet tool install --global wix --version '6.*'
wix extension add --global WixToolset.UI.wixext/6.0.2

# Release builds (parents): qmake release shadow-build into build\ (same toolchain as scripts/sd.ps1)
# Release build (SeamlyLayout): from src/app/seamlylayout/qt_frontend
#   cmake --preset release -DCMAKE_PREFIX_PATH=C:/Qt/<ver>/msvc2022_64
#   cmake --build --preset release
```

The MSVC CRT does **not** require running from a VS developer prompt —
`smsi.ps1` scans installed Visual Studios for the redist folder when
`VCToolsRedistDir` is unset (which it was on this run). windeployqt6 will print
`Cannot find Visual Studio installation directory, VCINSTALLDIR is not set` in
that case; it is harmless because the script deploys the CRT app-locally itself.

---

## 2. The command that produced the MSI

On the original 2026-07-23 run the plain default did **not** work because of the
Qt-version mismatch described in §3, and the tool had to be named by hand:

```powershell
# 2026-07-23 — no longer necessary, kept for the record
.\scripts\packaging\windows\smsi.ps1 -WinDeployQt6 'C:\Qt\6.11.1\msvc2022_64\bin\windeployqt6.exe'
```

Since **Task 30** the documented default works — `smsi.ps1` reads the Qt kit out
of SeamlyLayout's own `CMakeCache.txt`, so it always deploys the runtime the exe
was linked against:

```powershell
.\scripts\packaging\windows\smsi.ps1
```

Result of the original run (two Qt runtimes; the Task 30 single-runtime MSI is
substantially smaller):

```
MSI OK: …\scripts\seamly-build-msi\x64\Seamly2D-x64.msi (186.8 MB)
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

1. **Stages** a fresh tree under `scripts\seamly-build-msi\x64\`:
   - `parent\` — seamly2d + seamlyme windeployqt trees merged (shared Qt runtime, plugins, xerces-c…) + MSVC CRT DLLs, exes removed
   - `layout\` — `SeamlyLayout.exe` with its own Qt runtime deployed by `windeployqt6 --qmldir …\qml --release`, packaged default `settings\`, LGPL `licenses\`, + MSVC CRT DLLs, exe removed
   - `exes\` — the three executables (authored explicitly in the `.wxs` so shortcuts/associations can reference them)
2. Derives the MSI `ProductVersion` from `YYYY.M.D.HHMM` as `(YYYY−2000).M.((D−1)·1440 + HH·60 + MM)` (MSI caps the major field at 255), stores the full project version as `DisplayVersion`.
3. Runs `wix build seamly-family.wxs -arch x64 -ext WixToolset.UI.wixext …` → `Seamly2D-x64.msi`.
4. Runs `wix msi validate` (skip with `-SkipValidation`).

Output and staging live in `scripts\seamly-build-msi\` (kept out of git by the
`*-build-*` .gitignore pattern).

---

## 3. Problems encountered on this run

These were logged as **Task 31** in [`project-docs/TODO_MIGRATE.md`](../../../TODO_MIGRATE.md);
they all stemmed from the family's Qt 6.10 → 6.11 migration (**Task 30**), and
all three are **resolved by Task 30**. They are kept here as the record of what
the two-Qt arrangement cost.

### 3.1 `smsi.ps1` default invocation failed — Qt kit hard-pinned to 6.10.x  *(RESOLVED)*

`Find-WinDeployQt6` only matched `^6\.10\.\d+$` under `C:\Qt`. This machine had
moved to Qt **6.11.1** and removed 6.10.x, so the documented default
`.\scripts\packaging\windows\smsi.ps1` threw:

```
windeployqt6 not found under 'C:\Qt\6.10.x\msvc2022_64\bin' - install Qt 6.10.x or pass -WinDeployQt6.
```

**Fixed (Task 30/31):** `Find-WinDeployQt6` now reads `CMAKE_PREFIX_PATH` from
SeamlyLayout's `build\Release\CMakeCache.txt` — the kit the exe was actually
built against — and falls back to the newest installed `msvc2022_64` kit of any
version. No Qt version is hard-coded, so a future Qt upgrade needs no script
edit. `-WinDeployQt6` still overrides (CI passes it explicitly).

### 3.2 SeamlyLayout.exe was a stale 6.10 build wrapped in a 6.11 runtime  *(RESOLVED)*

`src\app\seamlylayout\qt_frontend\build\Release\SeamlyLayout.exe` had been
compiled against **Qt 6.10.1** (per its `CMakeCache.txt`), which was no longer
installed, so the MSI shipped that 6.10-linked exe with a **6.11.1** runtime
deployed around it — running only by Qt's within-6.x forward binary
compatibility, not a clean version-matched build.
**Fixed (Task 30):** SeamlyLayout now builds against Qt 6.11.1
(`find_package(Qt6 6.11.1 REQUIRED ...)`), so exe and runtime are the same Qt.

### 3.3 MSI was ~187 MB — two full Qt runtimes shipped  *(RESOLVED)*

Because the parents (6.11.1) and SeamlyLayout (built on 6.10, deployed with
6.11) were different Qt releases, the MSI shipped **two** runtimes: the shared
parent runtime in `…\Seamly2D\` and SeamlyLayout's own copy in
`…\Seamly2D\SeamlyLayout\`.
**Fixed (Task 30):** with all three apps on Qt 6.11.1 the staging tree and the
install directory are one — `smsi.ps1` deploys both windeployqt runs into the
same folder and the `.wxs` harvests a single tree, so the subdirectory and the
duplicate runtime are gone.

### 3.4 Parent exes were deployed with the WRONG Qt runtime — bare `windeployqt` on `PATH`  *(RESOLVED, Task 48)*

The `win32-msvc` post-link step in `src\app\seamly2d\seamly2d.pro`,
`src\app\seamlyme\seamlyme.pro` and `src\test\Seamly2DTest\Seamly2DTest.pro`
invoked `windeployqt` with **no path**, so the shell resolved it. On a developer
PC with Qt Design Studio installed, the first `windeployqt` (and `windeployqt6`)
on `PATH` belongs to its reduced **Qt 6.8.7** kit, not the build kit — so a clean
`sb.ps1` run produced `build\src\app\<app>\bin\Qt6Core.dll` reporting
`6.8.7.0` next to exes compiled and linked entirely against Qt 6.11.1. Qt's
binary compatibility runs forward only, so those exes cannot start, and
`smsi.ps1` stages `build\src\app\<app>\bin` verbatim into the MSI.

**Fixed (Task 48):** all three `win32-msvc` branches now use
`qtPrepareTool(WINDEPLOYQT, windeployqt)` + `$$WINDEPLOYQT`, matching what the
`win32-arm64-msvc` branches already did — `qtPrepareTool` resolves the tool from
`$$[QT_INSTALL_BINS]`, the Qt that qmake itself belongs to, so the deployed
runtime can only ever be the kit that compiled the exe. `scripts\sb.ps1` and
`scripts\sd.ps1` now also compare the deployed `Qt6Core.dll` / `Qt6Cored.dll`
FileVersion against that kit and fail loudly on a mismatch, because the bug was
invisible until someone read the DLL version by hand. The macOS post-link steps
already used `$$[QT_INSTALL_BINS]/macdeployqt` and were never exposed; CI is
unaffected (the runners have no Design Studio, and `install-qt-action` puts the
correct Qt first on `PATH`).

**Bearing on the 2026-07-23 MSI recorded above:** its parent runtime came from
that bare, `PATH`-resolved `windeployqt`, so which Qt it actually shipped is not
determined by the build and cannot be reconstructed after the fact — the
`build\` tree it was staged from has since been wiped and rebuilt. Treat the
186.8 MB figure and its file counts as a record of the two-runtime layout only,
not as evidence about the parent Qt version. The numbers under §2 for the
single-runtime MSI were produced after this fix, with the deployed runtime
verified to match the compiling kit.

### Benign warnings (no action needed)

- `Cannot determine dependencies of …\qtposition_nmea.dll: … Qt6SerialPort.dll` — optional dependency of the NMEA positioning plugin; not used.
- `Cannot find any version of the dxcompiler.dll and dxil.dll` — only needed for Direct3D 12 features.
- `Cannot find Visual Studio installation directory, VCINSTALLDIR is not set` — CRT is deployed app-locally by the script instead.
- `warning WIX1076: ICE61: … Maximum version is not less than the current product` — expected result of `AllowSameVersionUpgrades`.

---

## 4. Installing / testing the MSI

```powershell
cd scripts\seamly-build-msi\x64
msiexec /i Seamly2D-x64.msi                       # interactive (license + directory page)
msiexec /i Seamly2D-x64.msi /qn                   # silent, defaults (needs elevation)
msiexec /i Seamly2D-x64.msi /qn INSTALLFOLDER=D:\Seamly2D   # silent, custom dir
msiexec /x Seamly2D-x64.msi /qn                   # silent uninstall
msiexec /a Seamly2D-x64.msi /qn TARGETDIR=C:\extract        # extract without installing
```

Manual verification checklist (clean machine — **not yet run**, see Task 13):

- [ ] All three apps launch from the Start Menu shortcuts
- [ ] seamly2d Layout Mode finds `SeamlyLayout.exe` beside it without configuring `paths/seamlyLayoutApp`
- [ ] `.sm2d` opens seamly2d; `.smis`/`.smms` open SeamlyMe
- [ ] Installing a newer MSI over an older one upgrades in place, settings retained
- [ ] Uninstall removes `Program Files\Seamly2D` + shortcuts/associations, leaves `%LOCALAPPDATA%\Seamly` and the user's pattern data untouched

---

## 5. arm64

```powershell
.\scripts\packaging\windows\smsi.ps1 -Arch arm64 -NoSeamlyLayout   # needs arm64 build trees
```

seamly2d/seamlyme cross-compile for arm64 in CI (`windows-msi.yml`); SeamlyLayout
has no arm64 build yet, so the arm64 MSI ships the two parent apps only
(`-NoSeamlyLayout`). Not built on this machine (no arm64 build trees present).

---

## 6. CI equivalent

`.github/workflows/windows-msi.yml` runs the same `smsi.ps1` against its
in-source CI build output (matrix x64/arm64), installs the required Qt kits,
signs the MSI with `jsign`/Google Cloud KMS when the `SEAMLY_SIGNING_*` secrets
are present (unsigned otherwise), and uploads the MSI artifact.
