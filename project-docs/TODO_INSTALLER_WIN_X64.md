# TODO — Windows x64 MSI Installer

Create one WiX v6 MSI for Seamly2D, SeamlyMe, and SeamlyLayout on Windows 10/11 x64.

- Prefix: `InstWinX64.`
- Check completed subtasks.
- Move completed tasks to `project-docs/TODO_COMPLETED.md`.
- Update `TODO_INSTALLER.md` when all tasks are complete.
- Ask the user only when a decision is required.
- Build in `.github/workflows/ci.yml`.
- Author in `scripts/packaging/windows/smsi.wxs`.
- Replace the x64 NSIS package with the MSI.
- Keep arm64 NSIS until SeamlyLayout supports arm64.
- Default programs to `C:\Program Files\SeamlyApps`.
- Ask before copying existing user data.

- [x] InstWinX64.0 (verify the baseline MSI build) is complete.

## InstWinX64.00 - Fix user data diretories

- [x] the user data is prompted on page 5 that the user directory SeamlyData will be installed to C:\Users\<username>, i.e. C:\users\susan in this case. The seamly data directory is written to c:\users\<usersname>\Documents\Seamly.  The SeamlyData was never created at c:\users\susan; instead a Seamly directory was created at c:\users\susan\Documents --> C:\users\susan\Documents\Seamly was the result in this case.

### Result — 2026-08-17

Two defects, not one. The wizard offered the wrong folder, and nothing read the
answer it recorded.

**The default parent is now the Documents folder.** `SEAMLYDATAPARENT` was
`%USERPROFILE%`; it is now the `PersonalFolder` known folder, so page 5 offers
`C:\Users\<user>\Documents\SeamlyData`. **Decision (user, 2026-08-17): the parent is
Documents, because that is where users go to find data written by other
applications. The leaf stays `SeamlyData`, and the page keeps asking for the
parent only.**

`PersonalFolder` rather than `%USERPROFILE%\Documents` so a redirected Documents is
followed — OneDrive Known Folder Move being the common case, where
`%USERPROFILE%\Documents` does not exist at all. It is a system folder property set
by `CostInitialize`, so it is readable where the action is scheduled but not at
`AppSearch` time. A second action holds `%USERPROFILE%\Documents` in reserve: an
empty `SEAMLYDATAPARENT` aborts the wizard with error 2343, which this project
has already hit once.

**The apps now read what Setup recorded.** `VCommonSettings::installerDataRoot()`
reads `HKLM\SOFTWARE\Seamly\Seamly2D\DataRoot`, and `initializeDataRoot()` adopts
it when nothing is configured yet. Precedence, highest first:

1. `paths/dataRoot` in the settings file — an earlier run, or Preferences → Paths.
2. the root Setup recorded.
3. an adopted legacy `~/seamly2d` tree.
4. the built-in default, `<Documents>/Seamly`.

Case 1 above case 2 is what stops the machine-wide installer value overriding a
user who moved their root afterwards. SeamlyLayout needs no change: it has no
data root.

**A third defect surfaced while making the value trustworthy.** The registry row
held `[SEAMLYDATAROOT]`, a directory id, and a directory id always resolves: a
`/qn` install with no arguments composes onto `TARGETDIR` and records
`C:\SeamlyData`, a folder a standard user cannot write to. Inert until now, and
adopted by every app the moment the apps started reading it. The row now holds
`[SEAMLYDATAROOTRECORDED]`, which is filled in only when this run actually chose
a root — `SEAMLYDATACHOSEN` decides that before `CostInitialize`, while the two
directory properties are still empty unless the wizard or the command line set
them. A `RegistrySearch` prefills the property from the existing key, so repair
and maintenance keep the recorded value, and both UI defaults gained
`AND NOT Installed` so a repair cannot recompute them.

Verified with a link-only build (stub staging tree, real authoring):
`wix build` clean, `wix msi validate` clean apart from the already-suppressed
ICE43/ICE57 and the expected ICE61, `smsi_check_authoring.ps1` 133 assertions
pass (11 new). The MSI tables were queried directly: `SetSEAMLYDATACHOSEN` at
798, `SetSEAMLYDATAROOTRECORDED` at 1001, `RegLocator` type 18 (raw, 64-bit
view).

Not verified: an interactive install, and the C++ change. Neither Seamly2D nor
SeamlyMe builds locally — `ci.yml` is their only verification. The new Qt code
was syntax-checked against Qt 6.11.1 headers with MSVC (`cl /Zs`).

Documentation updated: `.github/README-BUILDS.md`,
`scripts/packaging/windows/README.md`, `README_WINDOWS_BUILD.md`,
`INSTALL_DECISION_FLOW.md`. `test_msi_install.ps1` now reads the recorded root.

**Still open, and adjacent:** InstWinX64.2.11 wants the data root to survive an
*upgrade* as well as a repair. A major upgrade runs the wizard, so it offers the
default rather than the recorded root. The `RegistrySearch` above supplies the
value; prefilling page 5 from it is the remaining work.

## InstWinX64.1 — Replace WiX Dialog Framework

Blocks installer path dialogs and `SeamlyShortcutsDlg`.

`WixUI_InstallDir` owns the unconditional transition from `InstallDirDlg` to `VerifyReadyDlg`. Replace it with a custom dialog chain.

- [x] **InstWinX64.1.1** Define: Welcome → License → Previous Install → Program Directory → Data Root → Data Migration → Shortcuts → Ready → Progress → Exit.
- [x] **InstWinX64.1.2** Reuse unchanged stock dialogs through `WixUI_Common`.
- [x] **InstWinX64.1.3** Replace custom `SpawnDialog` transitions with `NewDialog`.
- [x] **InstWinX64.1.4** Add Back navigation and stock `CancelDlg`.
- [x] **InstWinX64.1.5** Replace obsolete `SpawnDialog` assertions with dialog-chain assertions.
- [x] **InstWinX64.1.6** Verify every page and Back transition in a real install.
- [x] **InstWinX64.1.7** Update `INSTALL_DECISION_FLOW.md` and `scripts/packaging/windows/README.md`.

### Result — 2026-08-12

`smsi.wxs` defines its own dialog set. `WixUI_InstallDir` is gone, and
with it the `SpawnDialog` wiring that WiX 6.0.2 never ran.

- 1.1–1.5 are one change: the chain cannot be authored without converting the
  three pages, and the old assertions fail the moment it is.
- `SeamlyPreviousInstallDlg` moved out of `InstallUISequence` into the chain.
- `SeamlyShortcutsDlg` became a full 370x270 page with Back, Next and Cancel.
- The package now owns the `BrowseDlg` OK events. `CheckTargetPath` runs for the
  program directory only, so the data root still accepts cloud and removable
  drives.
- `DialogRef` order sets the sequence numbers of `ResumeDlg`, `WelcomeDlg` and
  `MaintenanceWelcomeDlg`. The test pins 1296, 1297 and 1298.

Verified with a link-only build (stub staging tree, real authoring):
`wix build` clean, `wix msi validate` clean except the expected ICE61,
`smsi_check_authoring.ps1` 115 assertions pass.

Not verified: the pages themselves. That is 1.6, and it needs an interactive
install of a real MSI.

### Defect — 2026-08-15: error 2343 on leaving the program-directory page

The first interactive install stopped with "error code 2343" when the user
pressed Next on `InstallDirDlg`. 2343 is "specified path is empty".

Cause: the `SeamlyDataDirDlg` path box carried `Indirect="yes"`. An indirect
`PathEdit` reads its property to get the **name** of the property that holds the
path. Stock `InstallDirDlg` is indirect because `WIXUI_INSTALLDIR` holds the
string `INSTALLFOLDER`. `SEAMLYDATAPARENT` holds the path itself, so the lookup
asked for a property named `C:\Users\<user>\`, found nothing, and aborted the
install as the page was created. The page never drew, so no earlier check
caught it.

Three changes in `smsi.wxs`:

- The path box binds directly. `Indirect` is gone.
- `SeamlyDataDirDlg`'s Next runs `SetTargetPath` before `NewDialog`, so an
  edited parent reaches the Directory table and `[SEAMLYDATAROOT]` recomposes
  for the next page and the registry value. It is conditional on a non-empty
  property, which is the other route to 2343. No `CheckTargetPath`: the data
  root may be a cloud or removable drive.
- The `SEAMLYDATAPARENT` default gained a trailing backslash. Windows Installer
  appends a child directory verbatim, so `C:\Users\me` gave
  `C:\Users\meSeamlyData`.

`smsi_check_authoring.ps1` gained two assertions: the path box binds directly,
and the page commits before it advances. 118 assertions pass on a link-only
build with a stub staging tree.

### Result — 2026-08-15: the install completes

The rebuilt `dev-latest` MSI installed end to end. 1.6 is closed. Every page
drew, and the composed data root read `C:\Users\susan\SeamlyData\` — the
trailing-backslash fix holds.

### Defects — 2026-08-15, found after the install completed

Three defects, all fixed. None of them stops an install.

1. **`FolderLabel` printed a literal ampersand** — "Put the &SeamlyData folder
   in:". `NoPrefix="yes"` turns accelerator parsing off, so the `&` reached the
   screen. The `&amp;` is deleted.

2. **The legacy Start Menu folder was never removed.** `SEAMLYLEGACYSTARTMENU`
   was set `After="CostFinalize"` (sequence 1001), but WiX schedules
   `Wix4RemoveFoldersEx` at 799 — before `CostInitialize`, because the
   `RemoveFile` rows it adds must exist in time for costing. The action read an
   empty property and did nothing, silently. Now set `After="AppSearch"`, and
   the value expands `[%APPDATA]` instead of `[AppDataFolder]`: a directory
   property is unresolved that early.

3. **SeamlyLayout wrote its log files into the install directory** on Windows —
   `Logger::init()` used `applicationDirPath()/output`. `C:\Program Files` is
   not writable by a standard user, and the leftover `output\` directory is not
   owned by the installer, so no uninstall removes it. It now uses the
   `AppConfigLocation` root, as macOS and the Linux AppImage already did. See
   `src/app/seamlylayout/docs/status-docs/SEAMLYLAYOUT_DECISIONS.md`.

`smsi_check_authoring.ps1` gained three assertions and now runs 120: no
`NoPrefix` label carries a `&`, the Start Menu property is set before
`RemoveFolderEx` reads it, and its value comes from the environment.

Its `Get-MsiRows` helper returns `, $rows`. **Assign that directly — never
write `@(Get-MsiRows ...)`.** The wrapper produces an array holding an array. A
one-row query still works, because PowerShell unwraps a one-element array on a
cast or a member access, so the trap only appears when a query first returns
more than one row.

### Maintenance — 2026-08-15: smsi.wxs split into fragments

One 1,142-line file became a package file plus four fragments: `smsi_ui.wxs`,
`smsi_legacy.wxs`, `smsi_files.wxs`, `smsi_shortcuts.wxs`.

Four fragments, not the five first proposed: `<Package>` cannot live in a
fragment, and neither can `MajorUpgrade`, `MediaTemplate` or
`SummaryInformation`.

**Two silent failure modes now exist.** `wix build` links only the files it is
given, and a fragment nothing references is discarded with no diagnostic. Drop a
file from the command line, or delete a `ComponentGroupRef`/`UIRef` from
`smsi.wxs`, and the MSI builds without that whole area.

- `smsi.ps1` globs `*.wxs` instead of naming `smsi.wxs`, so a new fragment works
  with no edit there.
- `smsi_check_authoring.ps1` reads the built MSI, so a lost fragment fails the
  build.

Verified: all 37 MSI tables dumped before and after and diffed — identical,
component GUIDs included. 122 assertions pass. `wix msi validate` clean apart
from the expected ICE61.

Comments were left alone. They are 55% of the source, and this file produced
three defects in one week that only a comment prevents.

### Defect — 2026-08-15: the shortcuts page promised three desktop shortcuts

`SeamlyShortcutsDlg`'s checkbox reads "Create desktop shortcuts for Seamly2D,
SeamlyLayout, and SeamlyMe". The package authored two — SeamlyLayout had none.

The label was right. `smsi.wxs` already carried the reason beside the dialog:
SeamlyLayout opens standalone with no argument, so a bare desktop launch is a
supported way to start it, not only the `.pieces.svg` handoff from seamly2d.
`README.md` alone argued the other way; it is corrected.

- Added `SeamlyLayoutDesktopShortcutComponent`, conditional on
  `SEAMLYDESKTOPSHORTCUTS` like the other two.
- `smsi_check_authoring.ps1` now requires all three desktop shortcuts and all
  three conditions. The old "SeamlyLayout has no desktop shortcut" assertion is
  gone. 122 assertions pass.
- `test_msi_install.ps1` checks the third shortcut, gated on
  `-ExpectSeamlyLayout`. Its shortcut list now names each executable instead of
  lower-casing the shortcut name: `SeamlyLayout.exe` keeps its camel case on
  disk and PowerShell's `-eq` compares strings case sensitively.
- `wix msi validate` raises only the already-suppressed ICE43 and ICE57 for the
  new component, plus the expected ICE61.

1.7 rewrote the page order in `INSTALL_DECISION_FLOW.md` and
`scripts/packaging/windows/README.md`, and deleted the "SeamlyShortcutsDlg never
displays" defect note. Two stale claims were corrected in passing: the README
said the old NSIS installation is never removed automatically, which has not
been true since Setup gained the removal components.

## InstWinX64.0 — Fix installaton

User selected from Change (disabled), repair, remove --> Repair.

### Installation pages

- [x] **InstWinX64.0.1** When no SeamlyData data is detected look for seamly2d data folder and migrate it to SeamlyData.

- [ ] **InstWinX64.0.2** Display the detected existing directories for program data and user data.

- [x] **InstWinX64.0.3** create artifacts in ci.yml as .msi files, not .zip files

Publish the .msi as a GitHub Release asset or pre-release asset instead of an Actions artifact.

#### Result — 2026-08-15

The literal task is impossible. GitHub serves **every** workflow artifact as a
zip, and `actions/upload-artifact` has no option to return a bare file. The
artifact named `seamly-x64.msi` downloads as `seamly-x64.msi.zip`. A release
asset is the only raw `.msi` GitHub hands back.

`ci.yml` gained a `publish-windows-dev` job. Every push to `run-seamlyLayout`
deletes the `dev-latest` release, recreates it on the pushed commit, and uploads
`seamly-x64.msi` and `seamly-arm64.msi`.

- Delete and recreate, not edit: GitHub pins a tag to its creation commit, and
  no `gh` command moves it. An edited `dev-latest` would advertise the first
  commit that ever built it.
- The job depends on `windows-msi` only. A broken Linux or macOS leg cannot hold
  back the Windows package, and the release carries no Linux or macOS file.
- `fail-fast` is off, so one failed architecture fails `windows-msi` and skips
  this job instead of shipping half a release.
- `dev-latest` is a pre-release, so `/releases/latest` still resolves to the
  newest full release.
- The build artifacts stay. Both publish jobs read the MSIs from them.

The versioned `publish` job is unchanged: `schedule` and `workflow_dispatch`
still make the `v<version>` pre-release with all four platform files.

Verified statically — the workflow parses and the release-notes heredoc expands
correctly. Not verified: the job itself. It needs one real push to CI.

`scripts/packaging/windows/README.md` gained a "Downloading the MSI" section.
`.github/README-BUILDS.md` records the decision.

- [ ] **InstWinX64.0.4** use Seamly's logo and brand colors in the .msi installer

### Application preferences

Numbered from 0.5. Two tasks below used to repeat the `InstWinX64.0.3` and
`InstWinX64.0.4` identifiers of the installation-page tasks above.

- [ ] **InstWinX64.0.5** Fix user directory path for Pattern Label --> C:/Users/susan/seamly2d/label templates/default_pattern_label.xml should be C:/Users/susan/seamlyData/label templates/default_pattern_label.xml
- [ ] **InstWinX64.0.6** Fix user directory path for Piece Label --> C:/Users/susan/seamly2d/label templates/default_pattern_label.xml should be C:/Users/susan/seamlyData/label templates/default_pattern_label.xml


## InstWinX64.2 — Configure Installation Paths

### Program Directory

- [x] **InstWinX64.2.1** Default to `C:\Program Files\SeamlyApps`.
- [x] **InstWinX64.2.2** Resolve shortcuts, associations, registry values, and SeamlyLayout through `[INSTALLFOLDER]`.
- [x] **InstWinX64.2.3** Accept local and removable paths.
- [x] **InstWinX64.2.4** Reject OneDrive, Dropbox, Google Drive, iCloud, and Box Sync program paths.
- [x] **InstWinX64.2.5** Support interactive selection and `INSTALLFOLDER=`.

Verify `HKLM\SOFTWARE\Seamly\Seamly2D\InstallPath` with `test_msi_install.ps1`.

### User-Data Directory

Interactive verification is done. See InstWinX64.1.6.

- [x] **InstWinX64.2.6** Default `SEAMLYDATAROOT` to `%USERPROFILE%\SeamlyData` in the UI sequence.
- [x] **InstWinX64.2.7** Accept local, removable, and cloud paths.
- [x] **InstWinX64.2.8** Support `BrowseDlg` and `SEAMLYDATAROOT=`.
- [x] **InstWinX64.2.9** Require opt-in before migration.
- [x] **InstWinX64.2.10** Copy without overwrite or source deletion.
- [ ] **InstWinX64.2.11** Persist program and data paths through repair and upgrade.
- [ ] **InstWinX64.2.12** Register one shared data-root setting.
- [x] **InstWinX64.2.13** Make all three apps honor the configured data root. Done by InstWinX64.00: `VCommonSettings::installerDataRoot()` reads the recorded root and `initializeDataRoot()` adopts it, which covers seamly2d and seamlyme. SeamlyLayout has no data root.

**Decision:** `SEAMLYDATAROOT` currently stores the complete selected path. Selecting `E:\` uses `E:\`, not `E:\SeamlyData`.

## InstWinX64.3 — Configure Installed Applications

- [ ] **InstWinX64.3.1** Add executable and data directories to the current user's `PATH`.
- [ ] **InstWinX64.3.2** Broadcast `PATH` changes.
- [ ] **InstWinX64.3.3** Remove only installer-created `PATH` entries on uninstall.
- [ ] **InstWinX64.3.4** Configure Start Menu shortcuts.
- [x] **InstWinX64.3.5** Add optional Seamly2D and SeamlyMe desktop shortcuts.
- [ ] **InstWinX64.3.6** Verify `.sm2d`, `.smis`, `.smms`, and SVG associations.
- [ ] **InstWinX64.3.7** Optionally offer to launch apps after installation.
- [ ] **InstWinX64.3.8** Preserve user data during uninstall.
- [ ] **InstWinX64.3.9** Remove apps, shortcuts, associations, installer registry data, and installer-created `PATH` entries.

## InstWinX64.4 — Separate Documents and Application State

Use `<DocumentsLocation>/Seamly` for user documents. Use platform-standard locations for configuration, cache, logs, and recovery. Migrate in application code.

- [x] **InstWinX64.4.1** Default documents to `QStandardPaths::DocumentsLocation/Seamly`.
- [x] **InstWinX64.4.2** Support relocation through `paths/dataRoot`.
- [x] **InstWinX64.4.3** Copy the complete legacy tree, including unknown folders.
- [x] **InstWinX64.4.4** Merge without overwrite; verify copies; preserve source.
- [x] **InstWinX64.4.5** Reject destinations inside the source.
- [x] **InstWinX64.4.6** Mark migrated roots with `MIGRATED-TO-SEAMLY.txt`.
- [x] **InstWinX64.4.7** Configure the new root only after verification.
- [x] **InstWinX64.4.8** Seed standard subfolders.
- [x] **InstWinX64.4.9** Test with `QTemporaryDir`.
- [x] **InstWinX64.4.10** Verify on a real Windows profile.
- [ ] **InstWinX64.4.11** Add progress, cancellation, or deferral for large migrations.
- [ ] **InstWinX64.4.12** Prevent `pruneEmptyLegacyDataRoot()` from removing populated or migration-marked trees.
- [ ] **InstWinX64.4.13** Move per-app configuration to the platform-standard configuration tree.

## InstWinX64.5 — Correct Application Settings

Eight `VSettings` accessors use `%APPDATA%\Unknown Organization.ini`.

- [ ] **InstWinX64.5.1** Isolate tests from real user settings.
- [ ] **InstWinX64.5.2** Decide whether `paths/pattern` and `paths/layout` are shared or per-app.
- [ ] **InstWinX64.5.3** Replace temporary `QSettings` with configured application settings.
- [ ] **InstWinX64.5.4** Check `VSeamlyMeSettings`.
- [ ] **InstWinX64.5.5** Import missing legacy values without overwrite or source deletion.
- [ ] **InstWinX64.5.6** Verify no Seamly setting uses `Unknown Organization`.
- [ ] **InstWinX64.5.7** Update `.github/README-BUILDS.md`.

## InstWinX64.6 — Complete Metadata and Branding

### ARP Metadata

- [ ] **InstWinX64.6.1** Write `DisplayIcon` while retaining `ARPPRODUCTICON`.
- [ ] **InstWinX64.6.2** Determine whether `Publisher` needs an explicit registry value.
- [ ] **InstWinX64.6.3** Validate authored and installed ARP registry values.
- [ ] **InstWinX64.6.4** Verify icon and publisher in `appwiz.cpl` and Windows Settings.

### Seamly Branding

- [ ] **InstWinX64.6.5** Use `Seamly` for installer-facing branding.
- [ ] **InstWinX64.6.6** State that the package installs Seamly2D, SeamlyLayout, and SeamlyMe.
- [x] **InstWinX64.6.7** Use “Seamly Application Suite” in the EULA. `license.rtf` heading now reads “Seamly Application Suite - license summary”.
- [ ] **InstWinX64.6.8** Change package, executable, and About metadata to “Seamly Project.”
- [ ] **InstWinX64.6.9** Keep source-file copyright headers unchanged.
- [ ] **InstWinX64.6.10** Update authoring tests.
- [ ] **InstWinX64.6.11** Verify wizard branding.

### ARP Product

**Decision:** confirm whether one `Seamly` ARP entry remains correct if apps later update independently.

- [x] **InstWinX64.6.12** Keep one ARP entry named `Seamly`.
- [ ] **InstWinX64.6.13** Set `ProductName` and `ARPDISPLAYNAME` to `Seamly`.
- [ ] **InstWinX64.6.14** Name all three apps in `ARPCOMMENTS`.
- [ ] **InstWinX64.6.15** Update DisplayName assertions.
- [ ] **InstWinX64.6.16** Verify NSIS detection remains registry-based.
- [ ] **InstWinX64.6.17** Verify entry, icon, and publisher.
- [ ] **InstWinX64.6.18** Verify upgrades retain the `UpgradeCode` and one ARP entry.

## InstWinX64.7 — Correct Installer UI

- [ ] **InstWinX64.7.1** Use `C:\Users\<user>\Documents\Seamly` in previous-install text.
- [ ] **InstWinX64.7.2** Verify displayed AppData paths.
- [ ] **InstWinX64.7.3** Use: “An older Seamly2D version was found in `C:\Program Files (x86)\Seamly2D`.”
- [ ] **InstWinX64.7.4** Remove obsolete Program Files migration advice.
- [ ] **InstWinX64.7.5** Shorten the data-preservation message.
- [ ] **InstWinX64.7.6** Change `BannerLine` and `BottomLine` width from 373 to 370.
- [ ] **InstWinX64.7.7** Replace “Install Seamly2D to” with Seamly-suite wording.
- [ ] **InstWinX64.7.8** Show the complete editable destination path.
- [ ] **InstWinX64.7.9** Update UI tests and documentation.

## InstWinX64.8 — Preserve Command-Line Documents

- [ ] **InstWinX64.8.1** Reproduce first-launch `.sm2d` association behavior manually.
- [ ] **InstWinX64.8.2** Preserve the requested file through first-run dialogs.
- [ ] **InstWinX64.8.3** Define consistent behavior for all three apps.
- [ ] **InstWinX64.8.4** Test `.smis` and `.smms`.
- [ ] **InstWinX64.8.5** Verify the requested document opens after first-run setup.

## InstWinX64.9 — Complete MSI Packaging

- [x] **InstWinX64.9.1** Suppress `.wixpdb` with `-pdbtype none`.
- [ ] **InstWinX64.9.2** Run a full MSI build.
- [ ] **InstWinX64.9.3** Verify no `.wixpdb` is generated.
- [ ] **InstWinX64.9.4** Verify `wix msi validate`.
- [ ] **InstWinX64.9.5** Verify Windows Installer COM inspection.
- [x] **InstWinX64.9.6** Confirm no tooling requires `.wixpdb`.
- [x] **InstWinX64.9.7** Document how to restore `.wixpdb`.
- [ ] **InstWinX64.9.8** Verify Qt 6.11.1, QML, WebEngine, MSVC runtime, and Rust dependencies.
- [ ] **InstWinX64.9.9** Verify in-place major upgrades.
- [ ] **InstWinX64.9.10** Sign with `jsign` when `SEAMLY_SIGNING_PROJECT_ID` is available.
- [x] **InstWinX64.9.11** ~~Support local builds through `scripts/packaging/windows/smsi.ps1`.~~ **Reversed 2026-08-15:** the local-build mode is removed. `smsi.ps1` is CI-only and detects nothing from the machine it runs on, because each of its defaults and fallbacks could ship a runtime no app in the package was built against. See `scripts/packaging/windows/README.md`.
- [x] **InstWinX64.9.12** Support CI builds through `.github/workflows/ci.yml`.
- [ ] **InstWinX64.9.13** Test x64 and arm64 where hardware is available.
- [x] **InstWinX64.9.14** Complete static x64 validation.
- [x] **InstWinX64.9.15** Document build, signing, and verification.

## InstWinX64.10 — Verify Installer

### Fresh Install

- [ ] **InstWinX64.10.1** Programs: `C:\Program Files\SeamlyApps`.
- [ ] **InstWinX64.10.2** Data: `C:\Users\<user>\SeamlyData`.

### Standalone Migration

- [ ] **InstWinX64.10.3** Programs: `C:\Program Files (x86)\Seamly2D` → `E:\Programs\SeamlyApps`.
- [ ] **InstWinX64.10.4** Data: `C:\Users\<user>\seamly2d` → `E:\SeamlyData`.

### Cloud Migration

- [ ] **InstWinX64.10.5** Programs: `C:\Program Files\SeamlyApps`.
- [ ] **InstWinX64.10.6** Data: `G:\My Drive\seamly2d` → `G:\My Drive\SeamlyData`.

### Common Checks

- [ ] **InstWinX64.10.7** Paths persist and register.
- [ ] **InstWinX64.10.8** Shortcuts work.
- [ ] **InstWinX64.10.9** File associations work.
- [ ] **InstWinX64.10.10** Migration preserves source and destination files.
- [ ] **InstWinX64.10.11** Upgrade preserves paths and one ARP entry.
- [ ] **InstWinX64.10.12** Uninstall removes installer-owned resources and preserves user data.
- [ ] **InstWinX64.10.13** All three apps launch.

## InstWinX64.11 — Cleanup and Documentation

- [x] **InstWinX64.11.1** Remove `dist/seamly2d-installer.nsi`.
- [x] **InstWinX64.11.2** Preserve its legacy footprint in `smsi.wxs`.
- [x] **InstWinX64.11.3** Remove `windows-msi.yml`.
- [ ] **InstWinX64.11.4** Update `scripts/packaging/windows/README.md`.
- [ ] **InstWinX64.11.5** Update `.github/README-BUILDS.md`.
- [ ] **InstWinX64.11.6** Update `INSTALL_DECISION_FLOW.md`.
- [ ] **InstWinX64.11.7** Move completed tasks to `TODO_COMPLETED.md`.
- [ ] **InstWinX64.11.8** Update `TODO_INSTALLER.md` when this file is complete.

## InstWinX64.12 - Microsoft store distribution

package the desktop app as an msix and submit it through the official developer portal.

- [ ] **InstWinX64.12.1** Create Account: Sign up for a developer profile on the Microsoft Partner 
- [ ] **InstWinX64.12.2** Center using a Microsoft account.Package the App: Convert or wrap the Windows build (MSI/EXE) into an MSIX or use the Microsoft packaging tools so it fits store guidelines.
- [ ] **InstWinX64.12.3** Associate and Sign: Link your Visual Studio project or package with your reserved store name and sign it with a verified certificate.
- [ ] **InstWinX64.12.4** Submit: Upload the package file, fill out the store listing details (icons, descriptions, screenshots), and submit it for certificatio
- [ ] **InstWin64.12.5** Automating submissions via the Microsoft Store Submission API
