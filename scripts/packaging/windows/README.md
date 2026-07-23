# Windows MSI installer — Seamly app family (Task 13)

WiX authoring and build instructions for the Windows `.msi` installer that ships **seamly2d**, **seamlyme** and **SeamlyLayout** together, per architecture (x64 and arm64). The durable knowledge-base record (decisions, toolchains, per-platform packaging) lives in [`.github/README-BUILDS.md`](../../../.github/README-BUILDS.md); this file is the hands-on reference for building and testing the MSI itself.

## Files

| File | Purpose |
|---|---|
| `seamly-family.wxs` | WiX (v6) source: install layout, shortcuts, file associations, upgrade logic |
| `license.rtf` | License summary shown by the installer UI (GPL-3.0-or-later for seamly2d/seamlyme, LGPL-3.0 + MIT for SeamlyLayout, LGPL-3.0 for Qt) |
| `smsi.ps1` | Staging + `wix build` driver, used locally and by CI |
| `../../../.github/workflows/windows-msi.yml` | CI workflow producing `Seamly2D-x64.msi` / `Seamly2D-arm64.msi` artifacts |

## Key decisions

- **Tooling — WiX v6** (`dotnet tool install --global wix --version '6.*'`), the modern `wix` CLI. Its wildcard `Files` harvesting ingests the whole windeployqt output trees without maintaining per-file authoring. Pinned to v6 because WiX v7 refuses to run until its Open Source Maintenance Fee (OSMF) EULA is accepted (error WIX7015) — adopting that is a project policy decision, not a packaging one. The UI extension version must match the core tool: `wix extension add --global WixToolset.UI.wixext/<version>`.
- **One bundled MSI per architecture** (not per-app MSIs): the apps are a family — seamly2d launches the other two and they share files/settings — so they install and upgrade as one unit. Output: `Seamly2D-x64.msi`, `Seamly2D-arm64.msi`.
- **Install layout**: `[ProgramFiles64Folder]\Seamly2D\` holds seamly2d.exe + seamlyme.exe with their shared Qt runtime; `...\Seamly2D\SeamlyLayout\` holds SeamlyLayout.exe with its **own** Qt runtime. SeamlyLayout is built against a different Qt release than the parents, and Qt DLL file names are identical across releases, so the runtimes cannot share a directory. seamly2d finds the subdirectory executable via `SeamlyFamilyPaths::locateSeamlyLayout()` (`src/libs/vmisc/seamly_family_paths.cpp`), which checks flat-beside-seamly2d first, then the `SeamlyLayout\` subdirectory.
- **MSVC CRT is deployed app-locally** (the redist DLLs are copied beside each exe by `smsi.ps1`) instead of merge modules or chaining `vc_redist.exe`: an MSI cannot cleanly run a nested installer, merge modules are deprecated by Microsoft, and app-local deployment is supported and arch-symmetric. Note the NSIS installer's `vc_redist` step never actually shipped the redist in CI (`File /nonfatal` with no file present), so this is strictly an improvement.
- **Upgrade code** `cbf4b5f1-c32c-4dbb-b385-3ee4a7b30658` is **fixed forever** and shared by both architectures; `MajorUpgrade` (with `AllowSameVersionUpgrades`) removes any older install before the new files land, so newer versions upgrade in place. Never change it. The per-build ProductCode is auto-generated.
- **MSI version mapping**: MSI limits ProductVersion to `major ≤ 255`, so the project's `YYYY.M.D.HHMM` rolling version cannot be used directly. `smsi.ps1` derives `(YYYY−2000).M.((D−1)·1440 + HH·60 + MM)` — strictly increasing per build — and stores the full project version as `DisplayVersion` in `HKLM\SOFTWARE\Seamly\Seamly2D`.
- **File associations**: `.sm2d` → Seamly2D, `.smis` (individual) and `.smms` (multisize) → SeamlyMe, authored as classic (non-advertised) registry values. SeamlyLayout gets no association — its input is the `.pieces.svg` handoff, and a double extension cannot be registered separately from plain `.svg`.
- **Start Menu**: three advertised shortcuts directly in the Start Menu root (no folder — Windows 11 flattens folders anyway, and folderless shortcuts need no removal component).
- **User data is never touched**: settings live under `%LOCALAPPDATA%\Seamly\<app>` (`AppData\Local\Seamly\Seamly2D`, `...\SeamlyMe`, `...\SeamlyLayout` — the Task 15 unified locations) plus `%APPDATA%\Seamly\qt6_common.ini`, and pattern/measurement data defaults to `C:\Users\<user>\seamly2d`. The apps create these on first run (including legacy-location migration); install, upgrade and uninstall leave them alone.

## Building locally

Prerequisites (each is checked with a clear error message):

1. Release builds of seamly2d/seamlyme with windeployqt output — e.g. a qmake release shadow-build in `build\` (same toolchain notes as `scripts/sd.ps1`).
2. A release build of SeamlyLayout: from `src/app/seamlylayout/qt_frontend`, `cmake --preset release -DCMAKE_PREFIX_PATH=C:/Qt/6.10.1/msvc2022_64` + `cmake --build --preset release` (or `qr.ps1`, which also launches the app).
3. WiX v6: `dotnet tool install --global wix --version '6.*'` then `wix extension add --global WixToolset.UI.wixext/<wix version>`.
4. A Visual Studio install with the C++ workload (for the CRT redist DLLs).

Then:

```powershell
.\scripts\packaging\windows\smsi.ps1                    # x64, all three apps
.\scripts\packaging\windows\smsi.ps1 -Arch arm64 -NoSeamlyLayout   # arm64 (needs arm64 build trees)
```

Output: `scripts\seamly-build-msi\<arch>\Seamly2D-<arch>.msi` (gitignored). Only the `.msi` is produced — the `.wixpdb` symbol database is suppressed via `wix build -pdbtype none` (it is only used for `wix` patch/melt diffing, not by the shipped installer); to keep it for inspection, remove that flag from `$wixArguments` in `smsi.ps1`. The script runs `wix msi validate` (ICE checks) automatically; the only expected warning is ICE61, a known consequence of `AllowSameVersionUpgrades`.

## Installing / testing

```powershell
msiexec /i Seamly2D-x64.msi              # interactive (license + directory page)
msiexec /i Seamly2D-x64.msi /qn          # silent, defaults (needs elevation)
msiexec /i Seamly2D-x64.msi /qn INSTALLFOLDER=D:\Seamly2D   # silent, custom dir
msiexec /x Seamly2D-x64.msi /qn          # silent uninstall
msiexec /a Seamly2D-x64.msi /qn TARGETDIR=C:\extract        # extract without installing
```

Manual verification checklist (clean machine):

- [ ] Fresh install: all three apps launch from the Start Menu shortcuts
- [ ] seamly2d Layout Mode finds `SeamlyLayout\SeamlyLayout.exe` without configuring `paths/seamlyLayoutApp`
- [ ] Double-clicking `.sm2d` opens seamly2d; `.smis`/`.smms` open SeamlyMe
- [ ] Install a newer MSI over an older one: upgrades in place, settings retained
- [ ] Uninstall removes `Program Files\Seamly2D` and the shortcuts/associations, leaves `%LOCALAPPDATA%\Seamly` and the user's pattern data untouched
- [ ] Repeat on an arm64 machine with `Seamly2D-arm64.msi`

## arm64

seamly2d/seamlyme cross-compile for arm64 in CI exactly as in `ci.yml`'s windows matrix (the `win64_msvc2022_arm64_cross_compiled` Qt kit + `host-qmake`), and the arm64 MSI is produced from those trees with `wix build -arch arm64`. SeamlyLayout has no arm64 build yet, so the arm64 MSI currently ships the two parent apps only (`-NoSeamlyLayout`); the cross-compile story for SeamlyLayout (Rust `aarch64-pc-windows-msvc` target + cxx-qt + Qt 6.10 arm64 cross kit) is documented in `.github/README-BUILDS.md`.

## Code signing

CI signs the MSI with the same jsign / Google Cloud KMS setup as the NSIS installer (guarded on the `SEAMLY_SIGNING_*` secrets; skipped when absent, e.g. on third-party PRs). See `.github/workflows/CODE_SIGNING.md`.
