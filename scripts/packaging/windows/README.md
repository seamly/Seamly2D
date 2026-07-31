# Windows MSI installer — Seamly app family (Task 13)

WiX authoring and build instructions for the Windows `.msi` installer that ships **seamly2d**, **seamlyme** and **SeamlyLayout** together, per architecture (x64 and arm64). The durable knowledge-base record (decisions, toolchains, per-platform packaging) lives in [`.github/README-BUILDS.md`](../../../.github/README-BUILDS.md); this file is the hands-on reference for building and testing the MSI itself.

## Files

| File | Purpose |
|---|---|
| `seamly-family.wxs` | WiX (v6) source: install layout, shortcuts, file associations, upgrade logic, install-time dialogs |
| `license.rtf` | License summary shown by the installer UI (GPL-3.0-or-later for seamly2d/seamlyme, LGPL-3.0 + MIT for SeamlyLayout, LGPL-3.0 for Qt) |
| `smsi.ps1` | Staging + `wix build` driver, used locally and by CI |
| `test_msi_authoring.ps1` | Asserts the built MSI still contains the expected shortcuts, associations, registry rows, elevation, upgrade detection and dialogs; run by `smsi.ps1` on every build |
| `test_msi_install.ps1` | Asserts what an **installed** MSI actually did to a real machine, in four phases around the `msiexec` commands; standalone, copied to the test machine beside the `.msi` |
| `INSTALL_DECISION_FLOW.md` | What the installer decides and what the *application* decides, as flowcharts, across all four pre-existing-installation cases (clean / old NSIS / previous MSI / both). Read this before changing upgrade or previous-install behaviour |
| `../../../.github/workflows/windows-msi.yml` | CI workflow producing `Seamly2D-x64.msi` / `Seamly2D-arm64.msi` artifacts |

## Key decisions

- **Tooling — WiX v6** (`dotnet tool install --global wix --version '6.*'`), the modern `wix` CLI. Its wildcard `Files` harvesting ingests the whole windeployqt output trees without maintaining per-file authoring. Pinned to v6 because WiX v7 refuses to run until its Open Source Maintenance Fee (OSMF) EULA is accepted (error WIX7015) — adopting that is a project policy decision, not a packaging one. The UI extension version must match the core tool: `wix extension add --global WixToolset.UI.wixext/<version>`.
- **One bundled MSI per architecture** (not per-app MSIs): the apps are a family — seamly2d launches the other two and they share files/settings — so they install and upgrade as one unit. Output: `Seamly2D-x64.msi`, `Seamly2D-arm64.msi`.
- **Install layout — one flat directory, one Qt runtime** (Task 30): `[ProgramFiles64Folder]\SeamlyApps\` holds all three executables and the single Qt 6.11.1 runtime they share (the parents' windeployqt output merged with SeamlyLayout's `windeployqt6` output — QML modules, Qt Quick/WebEngine DLLs, `QtWebEngineProcess.exe` — plus SeamlyLayout's packaged `settings\` and `licenses\`). seamly2d finds the executable via `SeamlyFamilyPaths::locateSeamlyLayout()` (`src/libs/vmisc/seamly_family_paths.cpp`), which checks flat-beside-seamly2d first.
  - Before Task 30, SeamlyLayout was built against Qt 6.10 while the parents were on 6.11. Qt DLL file names are identical across releases, so the runtimes could not share a directory and SeamlyLayout was installed into a `...\Seamly2D\SeamlyLayout\` subdirectory with its **own** full Qt copy (the reason the MSI weighed ~187 MB). `locateSeamlyLayout()` keeps that subdirectory as a fallback so an in-place upgrade over such an install still resolves.
- **MSVC CRT is deployed app-locally** (the redist DLLs are copied beside the exes by `smsi.ps1`) instead of merge modules or chaining `vc_redist.exe`: an MSI cannot cleanly run a nested installer, merge modules are deprecated by Microsoft, and app-local deployment is supported and arch-symmetric. With one shared install directory this is a single copy for all three apps. Note the NSIS installer's `vc_redist` step never actually shipped the redist in CI (`File /nonfatal` with no file present), so this is strictly an improvement.
- **Upgrade code** `cbf4b5f1-c32c-4dbb-b385-3ee4a7b30658` is **fixed forever** and shared by both architectures; `MajorUpgrade` (with `AllowSameVersionUpgrades`) removes any older install before the new files land, so newer versions upgrade in place. Never change it. The per-build ProductCode is auto-generated.
- **MSI version mapping**: MSI limits ProductVersion to `major ≤ 255`, so the project's `YYYY.M.D.HHMM` rolling version cannot be used directly. `smsi.ps1` derives `(YYYY−2000).M.((D−1)·1440 + HH·60 + MM)` — strictly increasing per build — and stores the full project version as `DisplayVersion` in `HKLM\SOFTWARE\Seamly\Seamly2D`.
- **File associations**: `.sm2d` → Seamly2D, `.smis` (individual) and `.smms` (multisize) → SeamlyMe, authored as classic (non-advertised) registry values. SeamlyLayout gets no association — its input is the `.pieces.svg` handoff, and a double extension cannot be registered separately from plain `.svg`.
- **Start Menu**: three advertised shortcuts directly in the Start Menu root (no folder — Windows 11 flattens folders anyway, and folderless shortcuts need no removal component).
- **User data is never touched**: settings live under `%LOCALAPPDATA%\Seamly\<app>` (`AppData\Local\Seamly\Seamly2D`, `...\SeamlyMe`, `...\SeamlyLayout` — the Task 15 unified locations) plus `%APPDATA%\Seamly\qt6_common.ini`, and pattern/measurement data defaults to `C:\Users\<user>\seamlyData` (Task 34 renamed it from `...\seamly2d`; Task 53 settled on `seamlyData`). The apps create these on first run (including legacy-location migration); install, upgrade and uninstall leave them alone.

## Install-time experience (Task 51)

The wizard is WixUI's `WixUI_InstallDir` — welcome, license, install folder, ready — with two Seamly pages added:

| Page | When it appears | What it does |
|---|---|---|
| **An existing installation was found** | before the welcome page, only when a previous install is detected and only when installing | Warns that the program files will be replaced, and states plainly that user data is not touched — naming `C:\Users\<you>\seamlyData`, `AppData\Local\Seamly` and `AppData\Roaming\Seamly`. Two paragraphs appear conditionally: one for an older MSI of this product (`WIX_UPGRADE_DETECTED`), one for the old NSIS installation. |
| **Shortcuts** | after Next on the install-folder page | One checkbox: *Create desktop shortcuts for Seamly2D and SeamlyMe*, default **on** (`SEAMLYDESKTOPSHORTCUTS`). |

Decisions behind those two pages:

- **Desktop shortcuts are one checkbox covering seamly2d and seamlyme, not one per app, and SeamlyLayout gets none.** SeamlyLayout is a document-driven daughter app that seamly2d launches with a `.pieces.svg` argument; a bare desktop launch would only ever show an empty canvas. Per-app checkboxes would be three decisions for a choice users make once. Unattended installs can override: `msiexec /i Seamly2D-x64.msi /qn SEAMLYDESKTOPSHORTCUTS=0`.
- **There is no "pin to taskbar" checkbox, and there should not be one.** Windows 10 removed programmatic taskbar pinning: the `taskbarpin` verb is blocked for third-party callers, there is no MSI or WiX element for it, and the only supported mechanisms are OEM/enterprise provisioning (a Start/taskbar layout-modification XML applied by Group Policy or during imaging) which cannot be driven from a per-machine MSI a user double-clicks. A checkbox here would silently do nothing, so the choice is simply not offered.
- **The old NSIS installation is detected and explained, never removed automatically.** `dist\seamly2d-installer.nsi` is a *different product*: its own ARP entry, its own `uninstall.exe`, installed by default in `C:\Program Files (x86)\Seamly2D`, and the MSI's `UpgradeCode` says nothing about it. Running its uninstaller from a custom action was rejected — it is an interactive EXE, its uninstall section is `RMDir /r $INSTDIR` (which would delete anything a user had put in that folder), and Windows Installer cannot roll back an external uninstaller if the rest of the install then fails. So the dialog names the path it found and tells the user to remove it from Apps & features afterwards; leaving it installed is harmless because the two products install to different directories. Note both entries are called "Seamly2D" in ARP — the NSIS one shows no version, the MSI one shows `26.y.z`.
- **The NSIS search reads the 32-bit registry view** (`RegistrySearch Bitness="always32"`). The NSIS installer is a 32-bit executable and never switches views, so both `SOFTWARE\NSIS_Seamly2D` and its `Uninstall\Seamly2D` key land under `WOW6432Node`; an x64 MSI searching the default view would never find them.
- **ARP's DisplayVersion shows the numeric MSI ProductVersion (`26.y.z`) and cannot show the project version.** The `RegisterProduct` standard action writes the Uninstall key *after* `WriteRegistryValues`, so a component-authored override is overwritten every time. The full `YYYY.M.D.HHMM` version reaches the user through `ARPCOMMENTS` and through `HKLM\SOFTWARE\Seamly\Seamly2D\DisplayVersion` instead.
- **Both pages are wired without touching WixUI's publish chain.** Adding a second `NewDialog` publish to `InstallDirDlg`'s Next button — the obvious way to insert a wizard page — relies on undefined behaviour: two unconditionally-true `NewDialog` events on one control, with nothing in the MSI documentation saying which wins. WixUI never relies on it (all of its competing `NewDialog` publishes carry mutually exclusive conditions) and the built-in row's condition is the literal `1`, so no condition can exclude it. Instead the warning page is a `Show` entry in `InstallUISequence` (sequence 1250, before WixUI's first dialog at 1296), and the shortcuts page is a `SpawnDialog` at `Ordering` 2 on the same Next button, ahead of the built-in `NewDialog` at 4 — the same mechanism WixUI uses for its own `BrowseDlg`. **Do not express that sequence number as `Before="WelcomeDlg"`:** every WixUI dialog set defines that symbol, so the reference drags `WixUI_Minimal` and `WixUI_Advanced` into the link and the build dies on duplicate `TextStyle`/`Property` symbols.
- **ICE43 and ICE57 are suppressed in `smsi.ps1`, and only those two.** Both fire on the optional desktop-shortcut components and both assume `DesktopFolder` is inside the installing user's profile — true only of a per-user install. This package is `Scope="perMachine"` with `ALLUSERS=1`, so `DesktopFolder` is always the All Users desktop and the HKLM key path is correct. Doing what the ICEs ask would break the package: the server side of a per-machine install runs as LocalSystem, so an HKCU key path would be written into the SYSTEM hive where component detection can never find it, and every launch would trigger installer self-repair.

## Building locally

Prerequisites (each is checked with a clear error message):

1. Release builds of seamly2d/seamlyme with windeployqt output — e.g. a qmake release shadow-build in `build\` (same toolchain notes as `scripts/sd.ps1`).
2. A release build of SeamlyLayout: from `src/app/seamlylayout/qt_frontend`, `cmake --preset release -DCMAKE_PREFIX_PATH=C:/Qt/6.11.1/msvc2022_64` + `cmake --build --preset release` (or `qr.ps1`, which also launches the app). The Qt kit must include `qtwebengine` **and** its `qtwebchannel`/`qtpositioning` dependencies, or `find_package` fails at configure time. `smsi.ps1` picks the matching `windeployqt6` automatically by reading `CMAKE_PREFIX_PATH` out of that build's `CMakeCache.txt`, so the deployed runtime always matches the exe; pass `-WinDeployQt6 <path>` to override.
3. WiX v6: `dotnet tool install --global wix --version '6.*'` then `wix extension add --global WixToolset.UI.wixext/<wix version>`.
4. A Visual Studio install with the C++ workload (for the CRT redist DLLs).

Then:

```powershell
.\scripts\packaging\windows\smsi.ps1                    # x64, all three apps
.\scripts\packaging\windows\smsi.ps1 -Arch arm64 -NoSeamlyLayout   # arm64 (needs arm64 build trees)
```

Output: `scripts\seamly-build-msi\<arch>\Seamly2D-<arch>.msi` (gitignored). Only the `.msi` is produced — the `.wixpdb` symbol database is suppressed via `wix build -pdbtype none` (it is only used for `wix` patch/melt diffing, not by the shipped installer); to keep it for inspection, remove that flag from `$wixArguments` in `smsi.ps1`. The script then runs two checks, both of which fail the build:

1. `wix msi validate` (ICE checks, skip with `-SkipValidation`). ICE43 and ICE57 are suppressed for the reason given above; the only expected warning is **ICE61**, a known consequence of `AllowSameVersionUpgrades`.
2. `test_msi_authoring.ps1`, which opens the built MSI and asserts ~50 expectations about what it contains — elevation, ARP properties, the upgrade and NSIS detection, both install-time dialogs and the wording of the warning, the Start Menu and desktop shortcuts, the three file associations, and the install-info registry rows. Run it by hand against any MSI:

   ```powershell
   .\scripts\packaging\windows\test_msi_authoring.ps1 -Msi scripts\seamly-build-msi\x64\Seamly2D-x64.msi -ExpectSeamlyLayout
   ```

   It checks *content*, not behaviour: it cannot tell you whether a shortcut launches or Explorer shows the right icon. That is the manual checklist below.

## Installing / testing

```powershell
msiexec /i Seamly2D-x64.msi              # interactive (license + directory page)
msiexec /i Seamly2D-x64.msi /qn          # silent, defaults (needs elevation)
msiexec /i Seamly2D-x64.msi /qn INSTALLFOLDER=D:\Seamly2D   # silent, custom dir
msiexec /x Seamly2D-x64.msi /qn          # silent uninstall
msiexec /a Seamly2D-x64.msi /qn TARGETDIR=C:\extract        # extract without installing
```

### The scripted cycle

`test_msi_install.ps1` verifies a real install. It runs in four phases around the `msiexec` commands, sharing a state file so each phase can compare against the ones before it — which is how "uninstall did not take any user data with it" becomes a check rather than an opinion. It is standalone (no repository, no build tree, no Qt on the test machine), so copy it next to the `.msi` and run it from an elevated prompt:

```powershell
.\test_msi_install.ps1 -Phase Baseline                     # BEFORE installing
msiexec /i Seamly2D-x64-older.msi
.\test_msi_install.ps1 -Phase Installed -ExpectSeamlyLayout -PatternFile .\sample.sm2d
msiexec /i Seamly2D-x64-newer.msi                          # upgrade over the top
.\test_msi_install.ps1 -Phase Upgraded  -ExpectSeamlyLayout -PatternFile .\sample.sm2d
msiexec /x Seamly2D-x64-newer.msi
.\test_msi_install.ps1 -Phase Removed
```

The upgrade step needs **two packages built at different times**: `smsi.ps1` derives the MSI ProductVersion from the build timestamp and generates a fresh ProductCode per build, so two builds share the fixed `UpgradeCode` and major-upgrade each other, whereas re-running the same `.msi` is only a repair.

What it asserts: the installed files and a slice of the Qt runtime; that **each app starts and stays running** (the only check that proves the deployed runtime is complete — a missing QML module kills the process in a second and no package inspection can see it); the Start Menu and desktop shortcuts and their targets; the `HKLM\SOFTWARE\Seamly\Seamly2D` rows including the desktop-shortcut breadcrumbs; the Apps & features entry down to the estimated size and help links; all three associations in the registry *and* opening a real `.sm2d` through the shell; that an upgrade leaves exactly one ARP entry, a changed version and an unmoved install directory; that uninstall removes every one of those; and that `seamlyData`, `%LOCALAPPDATA%\Seamly`, `%APPDATA%\Seamly` and any old NSIS installation survive all of it.

Two deliberate choices worth knowing. **User data is checked as "never shrank", not "identical"** — starting the apps legitimately creates settings and seeds the data tree, so an exact-match test would fail for the right reasons; what must never happen is a file disappearing. And **the effective file association is reported, not asserted**: a per-user `UserChoice` overrides the machine-wide registration, so HKLM being correct is all an installer can be held to.

### What still needs human eyes

Everything below is appearance or wizard flow, which neither script can see:

- [ ] Double-clicking the `.msi` produces exactly one UAC prompt, showing the verified publisher once the package is signed (Task 33)
- [ ] The **Shortcuts** page appears after the install-folder page; unticking it results in no desktop shortcuts (verify with `test_msi_install.ps1 -Phase Installed -NoDesktopShortcuts`), leaving it ticked creates Seamly2D and SeamlyMe on the All Users desktop
- [ ] Start Menu, desktop and Explorer show the right icons for all three apps and for `.sm2d`/`.smis`/`.smms`
- [ ] seamly2d Layout Mode finds `SeamlyLayout.exe` beside it without configuring `paths/seamlyLayoutApp`, and the handoff opens the pieces
- [ ] Installing over an older MSI shows the "existing installation was found" page with the *upgrade* paragraph
- [ ] On a machine with the old NSIS install, the same page shows the *NSIS* paragraph naming `C:\Program Files (x86)\Seamly2D`; afterwards both entries are listed in Apps & features, the MSI one with a version and the NSIS one without
- [ ] The page does **not** appear on a clean machine, nor when repairing or uninstalling

**Other architectures**

- [ ] Repeat on an arm64 machine with `Seamly2D-arm64.msi` (omit `-ExpectSeamlyLayout`, which that package does not ship)

## arm64

seamly2d/seamlyme cross-compile for arm64 in CI exactly as in `ci.yml`'s windows matrix (the `win64_msvc2022_arm64_cross_compiled` Qt kit + `host-qmake`), and the arm64 MSI is produced from those trees with `wix build -arch arm64`. SeamlyLayout has no arm64 build yet, so the arm64 MSI currently ships the two parent apps only (`-NoSeamlyLayout`); the cross-compile story for SeamlyLayout (Rust `aarch64-pc-windows-msvc` target + cxx-qt + an arm64 cross kit, which Qt does not ship with WebEngine) is documented in `.github/README-BUILDS.md`.

## Code signing

CI signs the MSI with the same jsign / Google Cloud KMS setup as the NSIS installer (guarded on the `SEAMLY_SIGNING_*` secrets; skipped when absent, e.g. on third-party PRs). See `.github/workflows/CODE_SIGNING.md`.
