# Windows MSI installer — Seamly app family (Task 13)

WiX authoring and build instructions for the Windows `.msi` installer that ships **seamly2d**, **seamlyme** and **SeamlyLayout** together, per architecture (x64 and arm64). The durable knowledge-base record (decisions, toolchains, per-platform packaging) lives in [`.github/README-BUILDS.md`](../../../.github/README-BUILDS.md); this file is the hands-on reference for building and testing the MSI itself.

## Files

| File | Purpose |
|---|---|
| `seamly-family.wxs` | WiX (v6) source: install layout, shortcuts, file associations, upgrade logic, install-time dialogs |
| `license.rtf` | License summary shown by the installer UI (GPL-3.0-or-later for seamly2d/seamlyme, LGPL-3.0 + MIT for SeamlyLayout, LGPL-3.0 for Qt) |
| `smsi.ps1` | Staging + `wix build` driver, run by `ci.yml`'s `windows-msi` job. CI-only: it has no local-build mode and detects nothing from the machine it runs on |
| `test_msi_authoring.ps1` | Asserts the built MSI still contains the expected shortcuts, associations, registry rows, elevation, upgrade detection and dialogs; run by `smsi.ps1` on every build |
| `test_msi_install.ps1` | Asserts what an **installed** MSI actually did to a real machine, in four phases around the `msiexec` commands; standalone, copied to the test machine beside the `.msi` |
| `INSTALL_DECISION_FLOW.md` | What the installer decides and what the *application* decides, as flowcharts, across all four pre-existing-installation cases (clean / old NSIS / previous MSI / both). Read this before changing upgrade or previous-install behaviour |
| `../../../.github/workflows/ci.yml` | The only CI route to a Windows package. Its `windows-msi` job is a matrix over `arch` that builds `seamly-x64.msi` and `seamly-arm64.msi`, and the `publish` job attaches both to the GitHub **pre-release** (Tasks Installer.1.1 and Installer.1.2). A packaging-only `windows-msi.yml` duplicated this job until 2026-08-11; it is deleted, so an edit in this directory now runs the full CI suite |

## Key decisions

- **Tooling — WiX v6** (`dotnet tool install --global wix --version '6.*'`), the modern `wix` CLI. Its wildcard `Files` harvesting ingests the whole windeployqt output trees without maintaining per-file authoring. Pinned to v6 because WiX v7 refuses to run until its Open Source Maintenance Fee (OSMF) EULA is accepted (error WIX7015) — adopting that is a project policy decision, not a packaging one. The UI extension version must match the core tool: `wix extension add --global WixToolset.UI.wixext/<version>`.
- **One bundled MSI per architecture** (not per-app MSIs): the apps are a family — seamly2d launches the other two and they share files/settings — so they install and upgrade as one unit. Output: `scripts\seamly-msi\<arch>\seamly-<arch>.msi` — i.e. `seamly-x64.msi` and `seamly-arm64.msi`.
- **Install layout — one flat directory, one Qt runtime** (Task 30): `[ProgramFiles64Folder]\SeamlyApps\` holds all three executables and the single Qt 6.11.1 runtime they share (the parents' windeployqt output merged with SeamlyLayout's `windeployqt` output — QML modules, Qt Quick/WebEngine DLLs, `QtWebEngineProcess.exe` — plus SeamlyLayout's packaged `settings\` and `licenses\`). seamly2d finds the executable via `SeamlyFamilyPaths::locateSeamlyLayout()` (`src/libs/vmisc/seamly_family_paths.cpp`), which checks flat-beside-seamly2d first.
  - Before Task 30, SeamlyLayout was built against Qt 6.10 while the parents were on 6.11. Qt DLL file names are identical across releases, so the runtimes could not share a directory and SeamlyLayout was installed into a `...\Seamly2D\SeamlyLayout\` subdirectory with its **own** full Qt copy (the reason the MSI weighed ~187 MB). `locateSeamlyLayout()` keeps that subdirectory as a fallback so an in-place upgrade over such an install still resolves.
- **MSVC CRT is deployed app-locally** (the redist DLLs are copied beside the exes by `smsi.ps1`) instead of merge modules or chaining `vc_redist.exe`: an MSI cannot cleanly run a nested installer, merge modules are deprecated by Microsoft, and app-local deployment is supported and arch-symmetric. With one shared install directory this is a single copy for all three apps. The redist comes from `VCToolsRedistDir` and from nowhere else, so the shipped CRT is always the toolset that compiled the exes. Note the NSIS installer's `vc_redist` step never actually shipped the redist in CI (`File /nonfatal` with no file present), so this is strictly an improvement.
- **The package is built by CI only** (2026-08-15). `smsi.ps1` used to double as a local build driver: it defaulted `-Seamly2DBin`/`-SeamlyMeBin` to a `build\` shadow tree, defaulted `-Version` to the current time, read the Qt kit out of SeamlyLayout's `CMakeCache.txt` (falling back to the newest `C:\Qt` kit), and scanned installed Visual Studios for the CRT redist. Every one of those paths made the package depend on the state of one developer machine, and each produced a *valid* MSI carrying the wrong runtime — a mismatched Qt, or a CRT from a toolset that compiled nothing in the package. All of it is gone. The caller now names `-Version`, `-Seamly2DBin`, `-SeamlyMeBin` and `-WinDeployQt`, and `VCToolsRedistDir` must be set by the MSVC developer environment. To build a package, run the workflow: `gh workflow run ci.yml --ref run-seamlyLayout`.
- **Every package carries all three apps** (2026-08-15). `smsi.ps1 -NoSeamlyLayout` and the `.wxs` `IncludeSeamlyLayout` preprocessor guards are removed, so a two-app package can no longer be built. The switch existed for the arm64 leg, which shipped the parents only until 2026-08-11; both architectures have shipped all three apps since. `test_msi_authoring.ps1` asserts the SeamlyLayout shortcut and icon unconditionally and no longer takes `-ExpectSeamlyLayout`.
- **The deploy tool is spelled `windeployqt`, never `windeployqt6`.** A Qt 6 kit ships both names; one spelling everywhere is one fewer thing to keep in step between `smsi.ps1`, `ci.yml` and the `.pro` post-link steps, which already call `qtPrepareTool(WINDEPLOYQT, windeployqt)`.
- **Upgrade code** `cbf4b5f1-c32c-4dbb-b385-3ee4a7b30658` is **fixed forever** and shared by both architectures; `MajorUpgrade` (with `AllowSameVersionUpgrades`) removes any older install before the new files land, so newer versions upgrade in place. Never change it. The per-build ProductCode is auto-generated.
- **MSI version mapping**: MSI limits ProductVersion to `major ≤ 255`, so the project's `YYYY.M.D.HHMM` rolling version cannot be used directly. `smsi.ps1` derives `(YYYY−2000).M.((D−1)·1440 + HH·60 + MM)` — strictly increasing per build — and stores the full project version as `DisplayVersion` in `HKLM\SOFTWARE\Seamly\Seamly2D`.
- **File associations**: `.sm2d` → Seamly2D, `.smis` (individual) and `.smms` (multisize) → SeamlyMe, authored as classic (non-advertised) registry values. SeamlyLayout gets no association — its input is the `.pieces.svg` handoff, and a double extension cannot be registered separately from plain `.svg`.
- **Start Menu**: three advertised shortcuts directly in the Start Menu root (no folder — Windows 11 flattens folders anyway, and folderless shortcuts need no removal component).
- **User data is never touched**: settings live under `%LOCALAPPDATA%\Seamly\<app>` (`AppData\Local\Seamly\Seamly2D`, `...\SeamlyMe`, `...\SeamlyLayout` — the Task 15 unified locations) plus `%APPDATA%\Seamly\qt6_common.ini`, and pattern/measurement data defaults to `C:\Users\<user>\seamlyData` (Task 34 renamed it from `...\seamly2d`; Task 53 settled on `seamlyData`). The apps create these on first run (including legacy-location migration); install, upgrade and uninstall leave them alone.

## Install-time experience

The package defines **its own dialog set** (Task InstWinX64.1). It reuses the stock dialogs unchanged and owns every transition between them, so the page order is authored in `seamly-family.wxs` and nothing competes with it. Fresh install:

| # | Page | Dialog | When it appears |
|---|---|---|---|
| 1 | Welcome | `WelcomeDlg` | always |
| 2 | License | `LicenseAgreementDlg` | always |
| 3 | An existing installation was found | `SeamlyPreviousInstallDlg` | only when `WIX_UPGRADE_DETECTED` or `SEAMLYLEGACYUNINSTALLSTRING` is set, and only when installing |
| 4 | Program directory | `InstallDirDlg` | always |
| 5 | Where do you keep your work? | `SeamlyDataDirDlg` | always |
| 6 | Copy your existing work? | `SeamlyDataMigrateDlg` | always |
| 7 | Shortcuts | `SeamlyShortcutsDlg` | always |
| 8 | Ready to install | `VerifyReadyDlg` | always |
| 9 | Progress | `ProgressDlg` | always |
| 10 | Finish | `ExitDialog` | always |

**Back** reverses every arrow, and **Cancel** spawns the stock `CancelDlg` on every page. Maintenance, repair and uninstall keep the stock route (`MaintenanceWelcomeDlg` → `MaintenanceTypeDlg` → `VerifyReadyDlg`); none of the Seamly pages appear, because none of their answers apply to a product that is already installed.

What the four Seamly pages do:

| Page | What it does |
|---|---|
| **An existing installation was found** | Warns that the program files will be replaced, and states plainly that user data is not touched. Two paragraphs appear conditionally: one for an older MSI of this product (`WIX_UPGRADE_DETECTED`), one for the old NSIS installation. |
| **Where do you keep your work?** | The user-data root (`SEAMLYDATAROOT`), default `C:\Users\<you>\SeamlyData`, with a **Change** button that spawns the stock `BrowseDlg`. Any drive is allowed, including synced folders and USB media. |
| **Copy your existing work?** | Opt-in checkbox (`SEAMLYCOPYUSERDATA`, default **off**) to copy existing patterns and measurements into the new root. States that the originals stay put as a backup and that the same files are then also at the new location. |
| **Shortcuts** | One checkbox: *Create desktop shortcuts for Seamly2D, SeamlyLayout and SeamlyMe*, default **on** (`SEAMLYDESKTOPSHORTCUTS`). |

Decisions behind those pages:

- **The program folder rejects cloud-synced paths; the data root welcomes them.** A sync client renames, locks or replaces a file that an app has mapped, which corrupts a running install and breaks repair and uninstall — so `INSTALLFOLDER` containing OneDrive, Dropbox, Google Drive, iCloud or Box Sync is refused by a `Launch` condition (a launch condition, not a dialog check, because it is the only form that also blocks `/qn`). The data root is the opposite case: syncing your own patterns between machines is the point, so nothing there is restricted.
- **The copy is opt-in and additive only.** It never deletes and never overwrites — a file already at the destination wins. That makes it safe to repeat, so an interrupted copy can simply be run again. It runs as a deferred, **impersonated** action, because a per-machine install's execute sequence is SYSTEM and SYSTEM cannot read the user's own folders. `Return="ignore"`: a file-copy problem must not roll back a working program install.
- **There is deliberately no rollback action for the copy.** Undoing it would mean deleting files from a folder that may already have held the user's work, and nothing can tell the two apart. Deleting user data to tidy up a failed install is worse than leaving copied files behind, and since the copy only ever adds, there is nothing whose absence leaves the machine inconsistent.

- **Desktop shortcuts are one checkbox covering seamly2d and seamlyme, not one per app, and SeamlyLayout gets none.** SeamlyLayout is a document-driven daughter app that seamly2d launches with a `.pieces.svg` argument; a bare desktop launch would only ever show an empty canvas. Per-app checkboxes would be three decisions for a choice users make once. Unattended installs can override: `msiexec /i Seamly-x64.msi /qn SEAMLYDESKTOPSHORTCUTS=0`.
- **There is no "pin to taskbar" checkbox, and there should not be one.** Windows 10 removed programmatic taskbar pinning: the `taskbarpin` verb is blocked for third-party callers, there is no MSI or WiX element for it, and the only supported mechanisms are OEM/enterprise provisioning (a Start/taskbar layout-modification XML applied by Group Policy or during imaging) which cannot be driven from a per-machine MSI a user double-clicks. A checkbox here would silently do nothing, so the choice is simply not offered.
- **The old NSIS installation is removed, but its `uninstall.exe` is never run.** It is a *different product*: its own ARP entry, its own uninstaller, installed by default in `C:\Program Files (x86)\Seamly2D`, and the MSI's `UpgradeCode` says nothing about it. The MSI is a strict superset of it, so leaving it behind means two copies of seamly2d and seamlyme and Start Menu shortcuts that launch the old binaries. Setup therefore removes the four things that installation created — its program directory, its Start Menu folder, and both of its registry keys — through components that `RemoveFiles` can roll back. Running its uninstaller instead was rejected: it is an interactive EXE, its uninstall section is `RMDir /r $INSTDIR`, and Windows Installer cannot roll back an external uninstaller if the rest of the install then fails. Because the program directory goes as a whole, the warning page tells the user to move anything of their own out of it first. The reasoning is written up in `INSTALL_DECISION_FLOW.md`.
- **The NSIS search reads the 32-bit registry view** (`RegistrySearch Bitness="always32"`). The NSIS installer is a 32-bit executable and never switches views, so both `SOFTWARE\NSIS_Seamly2D` and its `Uninstall\Seamly2D` key land under `WOW6432Node`; an x64 MSI searching the default view would never find them.
- **ARP's DisplayVersion shows the numeric MSI ProductVersion (`26.y.z`) and cannot show the project version.** The `RegisterProduct` standard action writes the Uninstall key *after* `WriteRegistryValues`, so a component-authored override is overwritten every time. The full `YYYY.M.D.HHMM` version reaches the user through `ARPCOMMENTS` and through `HKLM\SOFTWARE\Seamly\Seamly2D\DisplayVersion` instead.
- **The package defines its own dialog set instead of using `WixUI_InstallDir`.** A dialog set owns every transition out of its own pages, and `WixUI_InstallDir`'s `InstallDirDlg` Next row is `NewDialog VerifyReadyDlg` at `Ordering` 4 with the condition `1` — so no page could take that slot and no condition could exclude it. `SpawnDialog` was the only mechanism left, and WiX 6.0.2 never ran it: the three question pages were in the package and never displayed. Replacing the set removes the cause. A stock dialog brings its own controls, control conditions and internal events (Cancel → `CancelDlg`, `VerifyReadyDlg`'s Install → `EndDialog`) but **never** a `NewDialog` row, so reuse costs one `DialogRef` each and the whole page order stays ours.
- **`WixUI_Common` supplies the bitmaps, not the fonts.** A custom set must define `WixUI_Font_Normal`, `WixUI_Font_Bigger` and `WixUI_Font_Title` itself, plus `DefaultUIFont`, `WIXUI_INSTALLDIR` (which names the directory `InstallDirDlg` edits) and `ARPNOMODIFY`.
- **The order of four `DialogRef` elements is load-bearing.** `ResumeDlg`, `WelcomeDlg`, `MaintenanceWelcomeDlg` and `ProgressDlg` carry no absolute sequence number of their own; WiX numbers them 1296–1299 from the order they are referenced, and the first one whose condition is true is the first page the user sees. Listing `WelcomeDlg` before `ResumeDlg` shows the welcome page to a user resuming a suspended install. `test_msi_authoring.ps1` asserts the resulting numbers.
- **`BrowseDlg` is shared, so it validates only the program directory.** It edits whatever `_BrowseProperty` names, and the set owns its OK button. `CheckTargetPath` is conditional on `_BrowseProperty = "INSTALLFOLDER"`: the data root is allowed on cloud and removable drives that the program-directory rules reject.
- **ICE43 and ICE57 are suppressed in `smsi.ps1`, and only those two.** Both fire on the optional desktop-shortcut components and both assume `DesktopFolder` is inside the installing user's profile — true only of a per-user install. This package is `Scope="perMachine"` with `ALLUSERS=1`, so `DesktopFolder` is always the All Users desktop and the HKLM key path is correct. Doing what the ICEs ask would break the package: the server side of a per-machine install runs as LocalSystem, so an HKCU key path would be written into the SYSTEM hive where component detection can never find it, and every launch would trigger installer self-repair.

## Building the package

**Run the CI workflow — there is no local build.**

```powershell
gh workflow run ci.yml --ref run-seamlyLayout
```

The `windows-msi` job is a matrix over `arch`. Each leg installs one Qt 6.11.1 kit, builds all three apps natively, and runs `smsi.ps1` with every input named:

```powershell
.\scripts\packaging\windows\smsi.ps1 -Arch <arch> -Version $env:VERSION_NUMBER `
  -Seamly2DBin src\app\seamly2d\bin `
  -SeamlyMeBin src\app\seamlyme\bin `
  -WinDeployQt "$env:QT_ROOT_DIR\bin\windeployqt.exe"
```

What the job must provide (`smsi.ps1` fails early naming whatever is missing):

1. Release builds of seamly2d/seamlyme with windeployqt output in the `-Seamly2DBin`/`-SeamlyMeBin` directories.
2. A release build of SeamlyLayout in `src\app\seamlylayout\qt_frontend\build\Release` (`-SeamlyLayoutBuildDir` overrides it). Its Qt kit must include `qtwebengine` **and** its `qtwebchannel`/`qtpositioning` dependencies, or `find_package` fails at configure time. `-WinDeployQt` names that same kit's deploy tool, so the deployed runtime matches the exe.
3. WiX v6: `dotnet tool install --global wix --version '6.*'` then `wix extension add --global WixToolset.UI.wixext/<wix version>` and the same for `WixToolset.Util.wixext`.
4. The MSVC developer environment, which sets `VCToolsRedistDir` (the CRT redist DLLs). `ci.yml` uses `ilammy/msvc-dev-cmd`.

Output: `scripts\seamly-msi\<arch>\seamly-<arch>.msi` (gitignored), attached to the pre-release by the `publish` job. Only the `.msi` is produced — the `.wixpdb` symbol database is suppressed via `wix build -pdbtype none` (it is only used for `wix` patch/melt diffing, not by the shipped installer); to keep it for inspection, remove that flag from `$wixArguments` in `smsi.ps1`. The script then runs two checks, both of which fail the build:

1. `wix msi validate` (ICE checks, skip with `-SkipValidation`). ICE43 and ICE57 are suppressed for the reason given above; the only expected warning is **ICE61**, a known consequence of `AllowSameVersionUpgrades`.
2. `test_msi_authoring.ps1`, which opens the built MSI and asserts over a hundred expectations about what it contains — elevation, ARP properties, the upgrade and NSIS detection, every Next and Back arrow of the dialog chain, the wording of the warning page, the Start Menu and desktop shortcuts, the three file associations, and the install-info registry rows. Run it by hand against any MSI:

   ```powershell
   .\scripts\packaging\windows\test_msi_authoring.ps1 -Msi scripts\seamly-msi\x64\seamly-x64.msi
   ```

   It checks *content*, not behaviour: it cannot tell you whether a shortcut launches or Explorer shows the right icon. That is the manual checklist below.

## Installing / testing

```powershell
msiexec /i Seamly-x64.msi              # interactive (license + directory page)
msiexec /i Seamly-x64.msi /qn          # silent, defaults (needs elevation)
msiexec /i Seamly-x64.msi /qn INSTALLFOLDER=D:\Seamly2D   # silent, custom dir
msiexec /x Seamly-x64.msi /qn          # silent uninstall
msiexec /a Seamly-x64.msi /qn TARGETDIR=C:\extract        # extract without installing
```

### The scripted cycle

`test_msi_install.ps1` verifies a real install. It runs in four phases around the `msiexec` commands, sharing a state file so each phase can compare against the ones before it — which is how "uninstall did not take any user data with it" becomes a check rather than an opinion. It is standalone (no repository, no build tree, no Qt on the test machine), so copy it next to the `.msi` and run it from an elevated prompt:

```powershell
.\test_msi_install.ps1 -Phase Baseline                     # BEFORE installing
msiexec /i Seamly-x64-older.msi
.\test_msi_install.ps1 -Phase Installed -ExpectSeamlyLayout -PatternFile .\sample.sm2d
msiexec /i Seamly-x64-newer.msi                          # upgrade over the top
.\test_msi_install.ps1 -Phase Upgraded  -ExpectSeamlyLayout -PatternFile .\sample.sm2d
msiexec /x Seamly-x64-newer.msi
.\test_msi_install.ps1 -Phase Removed
```

The upgrade step needs **two packages from different `-Version` values**, i.e. two CI runs: `smsi.ps1` derives the MSI ProductVersion from `-Version` and generates a fresh ProductCode per build, so two builds share the fixed `UpgradeCode` and major-upgrade each other, whereas re-running the same `.msi` is only a repair.

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

- [ ] Repeat on an arm64 machine with `seamly-arm64.msi` (it ships all three apps, so pass `-ExpectSeamlyLayout` to `test_msi_install.ps1` there too)

## arm64

All three apps build **natively** on the `windows-11-arm` runner in `ci.yml`'s `windows-msi` job — the `windows_arm64` host with the `win64_msvc2022_arm64` kit, the cargo host toolchain, and plain `qmake`. Nothing is cross-compiled. `smsi.ps1` then builds the arm64 package with `wix build -arch arm64`.

The arm64 MSI shipped the two parent apps only until 2026-08-11, on the belief that Qt publishes no arm64 Windows WebEngine. That was true of Qt 6.8 and is false for 6.11.1; the `qt-arm64-module-probe` workflow verified it before being deleted. Both architectures now ship all three apps. Re-check at any Qt bump with `aqt list-qt windows_arm64 desktop --modules <version> <arch>`.

## Code signing

CI signs the MSI with the same jsign / Google Cloud KMS setup as the NSIS installer (guarded on the `SEAMLY_SIGNING_*` secrets; skipped when absent, e.g. on third-party PRs). See `.github/workflows/CODE_SIGNING.md`.
