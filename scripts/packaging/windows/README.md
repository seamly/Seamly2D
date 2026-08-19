# Windows MSI installer — Seamly app suite (Task 13)

WiX authoring and build instructions for the Windows `.msi` installer that ships **seamly2d**, **seamlyme** and **SeamlyLayout** together, per architecture (x64 and arm64). The durable knowledge-base record (decisions, toolchains, per-platform packaging) lives in [`.github/README-BUILDS.md`](../../../.github/README-BUILDS.md); this file is the hands-on reference for building and testing the MSI itself.

## Files

| File | Purpose |
|---|---|
| `smsi.wxs` | WiX (v6) source: install layout, shortcuts, file associations, upgrade logic, install-time dialogs |
| `seamly2d.ico`, `seamlyme.ico`, `seamlylayout.ico` | Shortcut and Apps &amp; features icons, compiled into the MSI `Icon` table by `smsi.wxs`. Each `<Icon Id>` must equal the file name |
| `license.rtf` | License summary shown by the installer UI (GPL-3.0-or-later for seamly2d/seamlyme, LGPL-3.0 + MIT for SeamlyLayout, LGPL-3.0 for Qt) |
| `smsi.ps1` | Staging + `wix build` driver, run by `ci.yml`'s `windows-msi` job. CI-only: it has no local-build mode and detects nothing from the machine it runs on |
| `smsi_check_authoring.ps1` | Asserts the built MSI still contains the expected shortcuts, associations, registry rows, elevation, upgrade detection and dialogs; run by `smsi.ps1` on every build |
| `test_msi_install.ps1` | Asserts what an **installed** MSI actually did to a real machine, in four phases around the `msiexec` commands; standalone, copied to the test machine beside the `.msi` |
| `INSTALL_DECISION_FLOW.md` | What the installer decides and what the *application* decides, as flowcharts, across all four pre-existing-installation cases (clean / old NSIS / previous MSI / both). Read this before changing upgrade or previous-install behaviour |
| `../../../.github/workflows/ci.yml` | The only CI route to a Windows package. Its `windows-msi` job is a matrix over `arch` that builds `seamly-x64.msi` and `seamly-arm64.msi`, and the `publish` job attaches both to the GitHub **pre-release** (Tasks Installer.1.1 and Installer.1.2). An edit in this directory runs the full CI suite |

## Source layout

The authoring was one 1,142-line file until 2026-08-15. It is now one package file plus four fragments:

| File | Holds |
|---|---|
| `smsi.wxs` | `<Package>` and everything that must sit inside it: identity, upgrade, ARP, the properties the dialogs read, the launch conditions, the user-data copy action, and the references that pull the fragments in |
| `smsi_ui.wxs` | the wizard — its dialogs and every transition |
| `smsi_legacy.wxs` | finding and removing the pre-MSI installation |
| `smsi_files.wxs` | directory tree, executables, Start Menu shortcuts, file associations |
| `smsi_shortcuts.wxs` | optional desktop shortcuts, install-info registry values |

**Two ways to break this silently.** `wix build` links the files it is handed, and a WiX fragment that nothing references is discarded without a diagnostic. Drop a source file from the command line, or delete a `ComponentGroupRef`/`UIRef` from `smsi.wxs`, and the build still succeeds — the MSI simply lacks that whole area. There is no error, no warning.

Two things guard it. `smsi.ps1` globs `*.wxs` rather than naming files, so a new fragment needs no change there. `smsi_check_authoring.ps1` reads the *built* MSI and asserts the rows exist, so a lost fragment fails the build.

`<Package>` cannot live in a fragment, and neither can `MajorUpgrade`, `MediaTemplate` or `SummaryInformation`. That is why there are four fragments and not five.

The split was verified by dumping all 37 MSI tables before and after and diffing them: identical, component GUIDs included.

## Key decisions

- **Tooling — WiX v6** (`dotnet tool install --global wix --version '6.*'`), the modern `wix` CLI. Its wildcard `Files` harvesting ingests the whole windeployqt output trees without maintaining per-file authoring. Pinned to v6 because WiX v7 refuses to run until its Open Source Maintenance Fee (OSMF) EULA is accepted (error WIX7015) — adopt v7 when income by the project or by supporting companies exceeds $10,000 USD. The UI extension version must match the core tool: `wix extension add --global WixToolset.UI.wixext/<version>`.
- **One bundled MSI per architecture** (not per-app MSIs): the apps are a suite — seamly2d launches the other two and they share files/settings — so they install and upgrade as one unit. Output: `scripts\seamly-msi\<arch>\seamly-<arch>.msi` — i.e. `seamly-x64.msi` and `seamly-arm64.msi`.
- **Install layout — one flat directory, one Qt runtime** (Task 30): `[ProgramFiles64Folder]\SeamlyApps\` holds all three executables and the single Qt 6.11.1 runtime they share (the parents' windeployqt output merged with SeamlyLayout's `windeployqt` output — QML modules, Qt Quick/WebEngine DLLs, `QtWebEngineProcess.exe` — plus SeamlyLayout's packaged `settings\` and `licenses\`). seamly2d finds the executable via `SeamlySuitePaths::locateSeamlyLayout()` (`src/libs/vmisc/seamly_suite_paths.cpp`), which checks flat-beside-seamly2d first.
  - `locateSeamlyLayout()` also accepts a `...\Seamly2D\SeamlyLayout\` subdirectory, so an in-place upgrade over an install that used that layout still resolves.
- **MSVC CRT is deployed app-locally** (the redist DLLs are copied beside the exes by `smsi.ps1`) instead of merge modules or chaining `vc_redist.exe`: an MSI cannot cleanly run a nested installer, merge modules are deprecated by Microsoft, and app-local deployment is supported and arch-symmetric. With one shared install directory this is a single copy for all three apps. The redist comes from `VCToolsRedistDir` and from nowhere else, so the shipped CRT is always the toolset that compiled the exes.
- **The package is built by CI only.** `smsi.ps1` detects nothing from the machine it runs on: any default read from a developer machine — a `build\` shadow tree, a Qt kit from `CMakeCache.txt` or `C:\Qt`, a CRT redist found by scanning installed Visual Studios — produces a *valid* MSI carrying the wrong runtime. The caller names `-Version`, `-Seamly2DBin`, `-SeamlyMeBin` and `-WinDeployQt`, and the MSVC developer environment must set `VCToolsRedistDir`. To build a package, run the workflow: `gh workflow run ci.yml --ref run-seamlyLayout`.
- **Every package carries all three apps.** There is no switch for a two-app package. `smsi_check_authoring.ps1` asserts the SeamlyLayout shortcut and icon unconditionally.
- **The deploy tool is spelled `windeployqt`, never `windeployqt6`.** A Qt 6 kit ships both names; one spelling everywhere is one fewer thing to keep in step between `smsi.ps1`, `ci.yml` and the `.pro` post-link steps, which already call `qtPrepareTool(WINDEPLOYQT, windeployqt)`.
- **Upgrade code** `cbf4b5f1-c32c-4dbb-b385-3ee4a7b30658` is **fixed forever** and shared by both architectures; `MajorUpgrade` (with `AllowSameVersionUpgrades`) removes any older install before the new files land, so newer versions upgrade in place. Never change it. The per-build ProductCode is auto-generated.
- **MSI version mapping**: MSI ignores the 4th ProductVersion field for upgrade comparisons, so the project's 4-part `YY.M.D.MMMM` rolling version cannot be used directly. `smsi.ps1` derives `YY.M.((D−1)·1440 + MMMM)` — strictly increasing per build — and stores the full project version as `DisplayVersion` in `HKLM\SOFTWARE\Seamly\Seamly2D`. `MMMM` is the minute of the day, so the third field is minutes-of-month (max 44639 < 65535).
- **File associations**: `.sm2d` → Seamly2D, `.smis` (individual) and `.smms` (multisize) → SeamlyMe, authored as classic (non-advertised) registry values. SeamlyLayout gets no association — its input is the `.pieces.svg` handoff, and a double extension cannot be registered separately from plain `.svg`.
- **Start Menu**: three advertised shortcuts directly in the Start Menu root (no folder — Windows 11 flattens folders anyway, and folderless shortcuts need no removal component).
- **Fresh Setup creates the selected `SeamlyData` root.** The first app launch adds the nine standard directories and writes the default paths. Uninstall keeps the root and its contents.
- **Updates can migrate user data.** The impersonated migration action runs as the installing user. It preserves non-path settings and replaces path settings only after verification.

## Install-time experience

The package defines **its own dialog set** (Task InstWinX64.1). It reuses the stock dialogs unchanged and owns every transition between them, so the page order is authored in `smsi.wxs` and nothing competes with it. Fresh install:

| # | Page | Dialog | When it appears |
|---|---|---|---|
| 1 | Welcome | `WelcomeDlg` | always |
| 2 | License | `LicenseAgreementDlg` | always |
| 3 | An existing installation was found | `SeamlyPreviousInstallDlg` | only when Setup finds old Seamly2D and SeamlyMe without SeamlyLayout, or finds new SeamlyLayout |
| 4 | Program directory | `InstallDirDlg` | always |
| 5 | Where do you keep your work? | `SeamlyDataDirDlg` | always |
| 6 | Copy your existing work? | `SeamlyDataMigrateDlg` | only when an old or new Seamly installation exists |
| 7 | Shortcuts | `SeamlyShortcutsDlg` | always |
| 8 | Ready to install | `VerifyReadyDlg` | always |
| 9 | Progress | `ProgressDlg` | always |
| 10 | Finish | `ExitDialog` | always |

**Back** reverses every arrow, and **Cancel** spawns the stock `CancelDlg` on every page. Maintenance, repair and uninstall follow `MaintenanceWelcomeDlg` → `SeamlyMaintenanceTypeDlg` → `VerifyReadyDlg`; none of the install-time Seamly pages appear, because none of their answers apply to a product that is already installed.

**The maintenance page is ours, not the stock `MaintenanceTypeDlg`.** It transcribes that dialog exactly — same geometry, same texts, same Disable/Show conditions — and adds one line naming the installed version. WiX cannot add a control to a `<Dialog>` another fragment defines, so naming the version there means owning the whole page.

That page is reached only when **this** ProductCode is already installed, and `smsi.ps1` generates a fresh ProductCode per build. So reaching it means the user re-ran the very package that is installed — easy to do by accident with a rolling `dev-latest` download. `SEAMLYINSTALLEDVERSION` is read by `AppSearch` from `HKLM\SOFTWARE\Seamly\Seamly2D\DisplayVersion`, and one of three lines shows:

| Condition | Line |
|---|---|
| recorded version equals this build | Seamly `<version>` is installed. This installer holds that same version. |
| recorded version differs | Seamly `<installed>` is installed. This installer holds `<this build>`. |
| nothing recorded | This installer holds Seamly `<version>`. |

**Change stays disabled** (`ARPNOMODIFY`): the package has one feature, so there is nothing to select. **Repair** and **Remove** each set `WixUI_InstallMode` and *then* open `VerifyReadyDlg` — that page keys its action buttons on the property alone, so dropping either row would leave the wizard with no enabled button and no error. `smsi_check_authoring.ps1` asserts both rows and their order.

What the four Seamly pages do:

| Page | What it does |
|---|---|
| **An existing installation was found** | Warns that the program files will be replaced, and states plainly that user data is not touched. Two paragraphs appear conditionally: one for an older MSI of this product (`WIX_UPGRADE_DETECTED`), one for the old NSIS installation. |
| **Where do you keep your work?** | The user-data root (`SEAMLYDATAROOT`), default `C:\Users\<you>\Documents\SeamlyData`, with a **Change** button that spawns the stock `BrowseDlg`. The user edits the **parent** and Setup appends the fixed `SeamlyData` leaf. Any drive is allowed, including synced folders and USB media. The answer reaches `HKLM\SOFTWARE\Seamly\Seamly2D\DataRoot`, and every app adopts it on that user's first run. |
| **Copy your existing work?** | Opt-in checkbox (`SEAMLYCOPYUSERDATA`, default **off**) to migrate existing work. Old Seamly archives `seamly2d`, extracts it, and renames the extracted root to `SeamlyData`. New Seamly archives `SeamlyData` with that top-level name. |
| **Shortcuts** | One checkbox: *Create desktop shortcuts for Seamly2D, SeamlyLayout and SeamlyMe*, default **on** (`SEAMLYDESKTOPSHORTCUTS`). |

Decisions behind those pages:

- **The program folder rejects cloud-synced paths; the data root welcomes them.** A sync client renames, locks or replaces a file that an app has mapped, which corrupts a running install and breaks repair and uninstall — so `INSTALLFOLDER` containing OneDrive, Dropbox, Google Drive, iCloud or Box Sync is refused by a `Launch` condition (a launch condition, not a dialog check, because it is the only form that also blocks `/qn`). The data root is the opposite case: syncing your own patterns between machines is the point, so nothing there is restricted.
- **Migration is opt-in and additive only.** It archives the complete source tree, extracts it below the selected parent, and never overwrites an existing destination file. The action reads the source path from application settings. It runs deferred and impersonated so it can access the installing user's files and settings.
- **A new-version update does nothing when the selected location is unchanged.** A changed location migrates the complete `SeamlyData` tree and replaces path settings.
- **There is deliberately no rollback action for the copy.** Undoing it would mean deleting files from a folder that may already have held the user's work, and nothing can tell the two apart. Deleting user data to tidy up a failed install is worse than leaving copied files behind, and since the copy only ever adds, there is nothing whose absence leaves the machine inconsistent.

- **Desktop shortcuts are one checkbox covering all three apps, not one per app.** SeamlyLayout gets one as well: it opens standalone with no argument, so a bare desktop launch is a supported way to start it, not only the `.pieces.svg` handoff from seamly2d. Per-app checkboxes would be three decisions for a choice users make once. Unattended installs can override: `msiexec /i Seamly-x64.msi /qn SEAMLYDESKTOPSHORTCUTS=0`. (Until 2026-08-15 the checkbox named three apps and the package authored two. The label was right and the component was missing.)
- **There is no "pin to taskbar" checkbox, and there should not be one.** Windows 10 removed programmatic taskbar pinning: the `taskbarpin` verb is blocked for third-party callers, there is no MSI or WiX element for it, and the only supported mechanisms are OEM/enterprise provisioning (a Start/taskbar layout-modification XML applied by Group Policy or during imaging) which cannot be driven from a per-machine MSI a user double-clicks. A checkbox here would silently do nothing, so the choice is simply not offered.
- **The old NSIS installation is removed, but its `uninstall.exe` is never run.** It is a *different product*: its own ARP entry, its own uninstaller, installed by default in `C:\Program Files (x86)\Seamly2D`, and the MSI's `UpgradeCode` says nothing about it. The MSI is a strict superset of it, so leaving it behind means two copies of seamly2d and seamlyme and Start Menu shortcuts that launch the old binaries. Setup therefore removes the four things that installation created — its program directory, its Start Menu folder, and both of its registry keys — through components that `RemoveFiles` can roll back. Running its uninstaller instead was rejected: it is an interactive EXE, its uninstall section is `RMDir /r $INSTDIR`, and Windows Installer cannot roll back an external uninstaller if the rest of the install then fails. Because the program directory goes as a whole, the warning page tells the user to move anything of their own out of it first. The reasoning is written up in `INSTALL_DECISION_FLOW.md`.
- **The NSIS search reads the 32-bit registry view** (`RegistrySearch Bitness="always32"`). The NSIS installer is a 32-bit executable and never switches views, so both `SOFTWARE\NSIS_Seamly2D` and its `Uninstall\Seamly2D` key land under `WOW6432Node`; an x64 MSI searching the default view would never find them.
- **ARP's DisplayVersion shows the numeric MSI ProductVersion (`26.y.z`) and cannot show the project version.** The `RegisterProduct` standard action writes the Uninstall key *after* `WriteRegistryValues`, so a component-authored override is overwritten every time. The full `YY.M.D.MMMM` version reaches the user through `ARPCOMMENTS` and through `HKLM\SOFTWARE\Seamly\Seamly2D\DisplayVersion` instead.
- **The package defines its own dialog set instead of using `WixUI_InstallDir`.** A dialog set owns every transition out of its own pages, and `WixUI_InstallDir`'s `InstallDirDlg` Next row is `NewDialog VerifyReadyDlg` at `Ordering` 4 with the condition `1` — so no page can take that slot and no condition can exclude it. A stock dialog brings its own controls, control conditions and internal events (Cancel → `CancelDlg`, `VerifyReadyDlg`'s Install → `EndDialog`) but **never** a `NewDialog` row, so reuse costs one `DialogRef` each and the whole page order stays ours.
- **`WixUI_Common` supplies the bitmaps, not the fonts.** A custom set must define `WixUI_Font_Normal`, `WixUI_Font_Bigger` and `WixUI_Font_Title` itself, plus `DefaultUIFont`, `WIXUI_INSTALLDIR` (which names the directory `InstallDirDlg` edits) and `ARPNOMODIFY`.
- **The order of four `DialogRef` elements is load-bearing.** `ResumeDlg`, `WelcomeDlg`, `MaintenanceWelcomeDlg` and `ProgressDlg` carry no absolute sequence number of their own; WiX numbers them 1296–1299 from the order they are referenced, and the first one whose condition is true is the first page the user sees. Listing `WelcomeDlg` before `ResumeDlg` shows the welcome page to a user resuming a suspended install. `smsi_check_authoring.ps1` asserts the resulting numbers.
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

Only `-Arch` (defaults to `x64`), `-SeamlyLayoutBuildDir` (defaults to `src\app\seamlylayout\qt_frontend\build\Release`), `-SkipValidation` and `-OutputDirName` (defaults to `seamly-msi`) may be omitted. Changing `-OutputDirName` also means changing the `.gitignore` entry and `ci.yml`'s artifact and signing paths.

What the job must provide (`smsi.ps1` checks all of it before touching anything, and fails on the first problem, naming it):

1. Release builds of seamly2d/seamlyme with windeployqt output in the `-Seamly2DBin`/`-SeamlyMeBin` directories — `seamly2d.exe` and its `platforms\` plugin directory, and `seamlyme.exe`.
2. A release build of SeamlyLayout in `-SeamlyLayoutBuildDir`. Its Qt kit must include `qtwebengine` **and** its `qtwebchannel`/`qtpositioning` dependencies, or `find_package` fails at configure time. `-WinDeployQt` names that same kit's deploy tool, so the deployed runtime matches the exe. Spell it `windeployqt`, not `windeployqt6` — a Qt 6 kit ships both, and the project uses the unsuffixed name everywhere.
3. WiX v6: `dotnet tool install --global wix --version '6.*'` then `wix extension add --global WixToolset.UI.wixext/<wix version>` and the same for `WixToolset.Util.wixext`.
4. The MSVC developer environment, which sets `VCToolsRedistDir`. It is the only source of the CRT redist DLLs; `ci.yml` uses `ilammy/msvc-dev-cmd`.

The script then deletes any previous staging tree and rebuilds it: the two parent bin trees are copied over each other into `parent\`, `windeployqt --qmldir … --release` adds SeamlyLayout's QML modules and WebEngine runtime to the same folder, SeamlyLayout's packaged `settings\` and LGPL `licenses\` and the CRT DLLs follow, and each executable is moved into `exes\` as it is deployed. `wix build` then harvests `parent\` by wildcard and takes the three executables from `exes\`.

Output: `scripts\seamly-msi\<arch>\seamly-<arch>.msi` (gitignored), published as described under [Downloading the MSI](#downloading-the-msi). Only the `.msi` is produced — the `.wixpdb` symbol database is suppressed via `wix build -pdbtype none` (it is only used for `wix` patch/melt diffing, not by the shipped installer); to keep it for inspection, remove that flag from `$wixArguments` in `smsi.ps1`. The script then runs two checks, both of which fail the build:

1. `wix msi validate` (ICE checks, skip with `-SkipValidation`). ICE43 and ICE57 are suppressed for the reason given above; the only expected warning is **ICE61**, a known consequence of `AllowSameVersionUpgrades`.
2. `smsi_check_authoring.ps1`, which opens the built MSI and asserts over a hundred expectations about what it contains — and which is the only thing standing between you and a silently dropped fragment (see **Source layout** below) — elevation, ARP properties, the upgrade and NSIS detection, every Next and Back arrow of the dialog chain, the wording of the warning page, the Start Menu and desktop shortcuts, the three file associations, and the install-info registry rows. Run it by hand against any MSI:

   ```powershell
   .\scripts\packaging\windows\smsi_check_authoring.ps1 -Msi scripts\seamly-msi\x64\seamly-x64.msi
   ```

   It checks *content*, not behaviour: it cannot tell you whether a shortcut launches or Explorer shows the right icon. That is the manual checklist below.

### Downloading the MSI

Take the MSI from a **release**, not from the Actions page.

| Source | Trigger | What you get |
| --- | --- | --- |
| Release `dev-latest` (`publish-windows-dev`) | every push to `run-seamlyLayout` | `seamly-x64.msi` and `seamly-arm64.msi`, raw |
| Release `v<version>` (`publish`) | `schedule` or `workflow_dispatch` | the same two MSIs plus the Linux and macOS builds |
| Build artifact `seamly-<arch>.msi` | every run | `seamly-<arch>.msi.zip` — unzip it first |

GitHub serves every workflow artifact as a zip archive. `actions/upload-artifact` has no option to return a bare file, so the artifact named `seamly-x64.msi` downloads as `seamly-x64.msi.zip`. A release asset is the only raw `.msi` GitHub hands back. The build artifacts stay because `publish` and `publish-windows-dev` read the MSIs from them.

`dev-latest` is one rolling pre-release. Each push deletes it, recreates it on the new commit, and uploads that build. It is a pre-release, so `/releases/latest` still resolves to the newest full release. It carries the Windows MSIs only: it depends on `windows-msi` alone, so a broken Linux or macOS leg cannot hold back the Windows package.

```powershell
gh release download dev-latest --repo seamly/Seamly2D --pattern 'seamly-x64.msi'
```

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

What it asserts: the installed files and a slice of the Qt runtime; that **each app starts and stays running** (the only check that proves the deployed runtime is complete — a missing QML module kills the process in a second and no package inspection can see it); the Start Menu and desktop shortcuts and their targets; the `HKLM\SOFTWARE\Seamly\Seamly2D` rows including the desktop-shortcut breadcrumbs; the Apps & features entry down to the estimated size and help links; all three associations in the registry *and* opening a real `.sm2d` through the shell; that an upgrade leaves exactly one ARP entry, a changed version and an unmoved install directory; that uninstall removes every one of those; and that `Documents\SeamlyData`, `%LOCALAPPDATA%\Seamly`, `%APPDATA%\Seamly` and any old NSIS installation survive all of it.

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

- [ ] Repeat on an arm64 machine with `seamly-arm64.msi`, passing `-ExpectSeamlyLayout` to `test_msi_install.ps1` there too

## arm64

All three apps build **natively** on the `windows-11-arm` runner in `ci.yml`'s `windows-msi` job — the `windows_arm64` host with the `win64_msvc2022_arm64` kit, the cargo host toolchain, and plain `qmake`. Nothing is cross-compiled. `smsi.ps1` then builds the arm64 package with `wix build -arch arm64`.

Qt 6.11.1 publishes arm64 Windows WebEngine, so the arm64 package ships all three apps. Re-check at any Qt bump with `aqt list-qt windows_arm64 desktop --modules <version> <arch>`.

## Code signing

CI signs the MSI with jsign and Google Cloud KMS (guarded on the `SEAMLY_SIGNING_*` secrets; skipped when absent, e.g. on third-party PRs). See `.github/workflows/CODE_SIGNING.md`.
