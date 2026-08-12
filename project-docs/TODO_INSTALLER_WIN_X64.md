# TODO — Create the combined Windows 11 x64 MSI installer for Seamly2D, SeamlyMe, and SeamlyLayout

Tasks for creating an .msi file for installation on a user's amd64 computer with Windows 10 or Windows 11.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options.

Tasks and subtasks in this file are numbered and have a prefix of `InstWinX64.`

When all tasks are completed and have been moved to TODO_COMPLETED.md, update TODO_INSTALLER.md to show completion.

Notes:

- **Data-root relocation asks first** — prompt Y/N before copying existing data
   files to a new directory location.
- **The program directory in Windows is `C:\Program Files\` + `Seamly`** — show the user
   the final assembled path and take OK/Cancel, rather than editing a box whose
   contents differ from the path it applies.
- **The MSI steps are inlined in `ci.yml`**.
- **The x64 `.msi` replaces the NSIS Windows zip** rather than shipping
    alongside it. NSIS stays for arm64 until Task Installer.1.2, because there is
    still no arm64 SeamlyLayout build.
- build occurs in Github on pull request to run-seamlyLayout branch. .github/workflows/ci.yml is the build file of record.
- update the installer features after the Windows 11 x64 .msi can be built on GitHub without error

## Task InstWinX64.0 - Build current .msi on Github workflows without error

Complete this task and move it to TODO_COMPLETED.md before implementing remaining tasks in this file

- [x] check that the previous workflow run on windows completed the Windows 11 x64 .msi build without error; if errors then update ci.yml, scripts, etc. as needed to eliminate build errors; repeat until no errors
- [ ] confirm from the user that the Windows 11 x64.msi was built without error

### Result (2026-08-11)

The x64 MSI builds clean. No change to `ci.yml` or the packaging scripts was
needed. Verified on commit `361b743fa0`:

- CI run [`31461308276`](https://github.com/seamly/Seamly2D/actions/runs/31461308276)
  — job `Windows: Build MSI (x64)`, success in 38m12s.
- Windows MSI run [`31461308379`](https://github.com/seamly/Seamly2D/actions/runs/31461308379)
  — job `Windows: Build MSI (x64)`, success in 31m18s.

Inside the x64 job: `wix msi validate` passed, `test_msi_authoring.ps1` reported
`MSI authoring check passed.`, the package came out as
`seamly-x64.msi` (163.5 MB), and `actions/upload-artifact` uploaded it. All
three apps are in it — the log links `SeamlyLayout.exe` and deploys
`seamlylayout.exe` with its QML runtime beside seamly2d and seamlyme.

Two warnings appear in the job. Both are known and neither fails the build:

- `WIX1076: ICE61 ... The Maximum version is not less than the current product`.
  `MajorUpgrade` carries `AllowSameVersionUpgrades="yes"` on purpose, so
  same-minute rebuilds still upgrade. ICE61 always fires on that authoring.
- `windeployqt`: `Cannot determine dependencies of ... qtposition_nmea.dll`,
  because the kit has no `Qt6SerialPort.dll`. The NMEA position plugin is not
  used by any of the three apps.

The signing steps did not run. They are gated on `SEAMLY_SIGNING_PROJECT_ID`,
which is not set in this repository — see Task InstWinX64.2.5.

## Task InstWinX64.1 — Windows 11 x64 .msi installer

### Implementation

Build one WiX v6 MSI per architecture containing Seamly2D, SeamlyMe, SeamlyLayout, and their dependencies. Store application state in the appropriate AppData directories; user-data paths and migration are covered by Task InstWinX64.1
Configure WiX v6 using `scripts/packaging/windows/seamly-family.wxs`

- [ ] **InstWinX64.1.0** - Create installer
  - [ ] **InstWinX64.1.1 - Program directory**
    - [ ] InstWinX64.1.1.1 Default program directory to `C:\Program Files\SeamlyApps`
    - [ ] InstWinX64.1.1.2 Update `seamly-family.wxs` and all shortcuts, file associations, registry values, and SeamlyLayout paths.
    --> How can the user confirm this?
    - [ ] InstWinX64.1.1.3 - Accept any local or removable drive/path for program directory; reject cloud-synced locations; install 'SeamlyApps' directory to the selected drive/path.
    --> unconfirmed
    - [ ] InstWinX64.1.1.4 - Add a `Change` button and silent-install property.
    --> unconfirmed
  - [ ] **InstWinX64.1.2 - User-data directory**
    - [ ] InstWinX64.1.2.1 - Default user directory to `C:\Users\<user>\SeamlyData`.
    --> unconfirmed
    - [ ] InstWinX64.1.2.2 - Accept any drive/path, including OneDrive, Google Drive, Dropbox, external drives, and USB media; install 'SeamlyData' directory to this drive/path.
    --> installer did not prompt user for the user data directory, fix this
    - [ ] InstWinX64.1.2.3 - Add a `Change` button and silent-install property.
    --> unconfirmed
    - [ ] **InstWinX64.1.2.4 When the data root changes:**
      - [ ] InstWinX64.1.2.4.1 Explain that first run will copy existing data without deleting the source.
      - [ ] InstWinX64.1.2.4.2 Require confirmation before continuing.
      - [ ] InstWinX64.1.2.4.3 Copy only missing files; never overwrite existing destination files.
  - [ ] **InstWinX64.1.3 - file clean up**
    - [ ] InstWinX64.1.3.1 - Delete `dist/seamly2d-installer.nsi`. It is currently kept in the tree, unbuilt, with a RETIRED header: `seamly-family.wxs` cites it as the record of a pre-MSI installation's on-disk footprint, which the MSI's `RemoveFolderEx` authoring removes on upgrade. Remove the citation from `seamly-family.wxs`
    --> I deleted it
    - [x] InstWinX64.1.3.2 - Remove `windows-msi.yml`
    --> confirmed
  - [ ] **InstWinX64.1.4 - Persist both selections** and prefill them during repair or upgrade.
  - [ ] **InstWinX64.1.5 - Register the data root** through a dedicated environment variable, registry value, or application setting so all three apps use it without prompting. Document the selected mechanism.
  - [ ] **InstWinX64.1.6 - Add the executable and data directories to the current user’s `PATH`**, broadcast the change, and remove only installer-created entries during uninstall.
  - [ ] **InstWinX64.1.7 - Make Seamly2D, SeamlyMe, and SeamlyLayout honor the configured data root on first run and thereafter.**
  - [ ] **InstWinX64.1.8** - Preserve user data during uninstall; remove applications, shortcuts, registry/configuration entries, and installer-created `PATH` entries only.
  - [ ] **InstWinX64.1.9** - Optionally offer to launch the Seamly applications after installation.
  - [ ] **InstWinX64.1.10** - Document both prompts, silent-install properties, path registration, migration behavior, and uninstall behavior in:
    - [ ] InstWinX64.1.10.1 `scripts/packaging/windows/README.md`
    - [ ] InstWinX64.1.10.2 `.github/README-BUILDS.md`
  
### Verification

- [ ] **InstWinX64.1.12 Fresh install**
  - [ ] InstWinX64.1.12.1 Programs: `C:\Program Files\SeamlyApps`
  - [ ] InstWinX64.1.12.2 Data: `C:\Users\<user>\SeamlyData`
- [ ] **InstWinX64.1.13 Standalone migration**
  - [ ] InstWinX64.1.13.1 Programs: `C:\Program Files (x86)\Seamly2D` → `E:\Programs\SeamlyApps`
  - [ ] InstWinX64.1.13.2 Data: `C:\Users\<user>\seamly2d` → `E:\SeamlyData`
- [ ] **InstWinX64.1.14 Cloud-data migration**
  - [ ] InstWinX64.1.14.1 Programs: `C:\Program Files\SeamlyApps`
  - [ ] InstWinX64.1.14.2 Data: `G:\My Drive\seamly2d` → `G:\My Drive\SeamlyData`
- [ ] **InstWinX64.1.15 For each scenario, verify:**
  - [ ] InstWinX64.1.15.1 The installer remembers and registers both paths.
  - [ ] InstWinX64.1.15.2 Desktop shortcuts are created.
  - [ ] InstWinX64.1.15.3 `.sm2d`, measurement, and SVG files open in the correct app.
  - [ ] InstWinX64.1.15.4 Existing data is copied without overwrite or deletion.
  - [ ] InstWinX64.1.15.5 Upgrade preserves both paths.
  - [ ] InstWinX64.1.15.6 Uninstall removes apps, shortcuts, and installer-created `PATH` entries while preserving data.

- [ ] **InstWinX64.2.3 verify:**

  - Seamly2D, SeamlyMe, and SeamlyLayout Installed
  - Qt 6.11.1 runtime, QML modules, and WebEngine
  - App-local MSVC runtime
  - Statically linked Rust dependencies

  - Start Menu shortcuts for all three apps
  - `.sm2d`, `.smis`, and `.smms` associations
  - In-place major upgrades
  - Clean application uninstall

- [ ] **InstWinX64.2.4 Preserve all user data** during installation, upgrade, repair, and uninstall.
- [ ] **InstWinX64.2.5 Sign MSI artifacts with `jsign`** when `SEAMLY_SIGNING_PROJECT_ID` is available.
- [x] **InstWinX64.2.6 Support builds through:**

  - Local: `scripts/packaging/windows/smsi.ps1`
  - CI (release, x64 + arm64): `.github/workflows/ci.yml`'s `windows-msi` job — the packages the `publish` job attaches to the pre-release (Tasks Installer.1.1 and Installer.1.2). This is the only CI route; the packaging-only `windows-msi.yml` was deleted in InstWinX64.1.3.2.

- [ ] **InstWinX64.2.7 Test both architectures**, where hardware is available:

  - Clean installation and launch
  - Shortcuts and file associations
  - Upgrade and repair
  - Uninstall without data removal

- [x] **InstWinX64.2.8 Complete static x64 validation** — passed July 22, 2026; clean-machine testing remains.
- [x] **InstWinX64.2.9 Document building, signing, and verification in:**

  - `scripts/packaging/windows/README.md`
  - `.github/README-BUILDS.md`
  - `.github/workflows/README_WORKFLOWS.md`

  All three updated for Task Installer.1.1: the `ci.yml` release path, the arm64-only NSIS remainder, the pre-release ref, and the corrected `seamly-<arch>.msi` output name (the docs had carried the stale `Seamly-x64.msi` / `Seamly2D-arm64.msi` names).

## Task InstWinX64.2 — Suppress `.wixpdb` generation

The WiX v6 MSI build needs only the `.msi`; suppress the optional linker database with `wix build -pdbtype none`.

- [x] InstWinX64.2.1 Add `-pdbtype none` to `$wixArguments` in `scripts/packaging/windows/smsi.ps1`, covering x64, ARM64, local, and CI builds.
- [ ] InstWinX64.2.2 Run a full Seamly MSI build and confirm:
  - [ ] InstWinX64.2.2.1 No `.wixpdb` is generated.
  - [ ] InstWinX64.2.2.2 `wix msi validate` passes.
  - [ ] InstWinX64.2.2.3 Windows Installer COM inspection passes.
  - [ ] InstWinX64.2.2.4 Pending restoration of the Qt build environment tracked in Task 31.
- [x] InstWinX64.2.3 Confirm no scripts, workflows, or inspection tools require `.wixpdb`.
- [x] InstWinX64.2.4 Document that `.wixpdb` is suppressed by default and can be restored by removing `-pdbtype none` from `$wixArguments`.

## Task InstWinX64.3 — Complete the Windows MSI install experience

- [x] InstWinX64.3.1 Add `test_msi_authoring.ps1` and run it from `smsi.ps1` for both architectures.
- [x] InstWinX64.3.2 Add the standalone `test_msi_install.ps1` phases: `Baseline`, `Installed`, `Upgraded`, and `Removed`.
- [x] InstWinX64.3.3 Verify installation and upgrade on the test laptop: all apps launch, associations work, the install path is preserved, one ARP entry remains, and the legacy NSIS installation is removed.
- [x] InstWinX64.3.4 Verify first-run migration copies the complete legacy tree to `Documents\Seamly` without overwriting or deleting the source.
- [x] InstWinX64.3.5 Add optional Seamly2D and SeamlyMe desktop shortcuts through `SEAMLYDESKTOPSHORTCUTS`; do not offer SeamlyLayout or taskbar shortcuts.
- [x] InstWinX64.3.6 Configure and verify one per-machine UAC elevation prompt.
- [x] InstWinX64.3.7 Warn before replacing an MSI or NSIS installation.
- [ ] InstWinX64.3.8 Make `SeamlyShortcutsDlg` appear under WiX 6; verify through authoring tests and a real wizard run.
- [ ] InstWinX64.3.9 After Task InstWinX64.6, run the `Removed` phase and verify removal of apps, shortcuts, associations, registry entries, and ARP metadata while preserving user data.
- [ ] InstWinX64.3.10 Complete the remaining branding, dialog, and ARP corrections in Tasks InstWinX64.6–InstWinX64.10.
- [x] InstWinX64.3.11 Document the installer flow and verification procedure in `scripts/packaging/windows/README.md` and `README_WINDOWS_BUILD.md`.

## Task InstWinX64.4 — Eliminate `Unknown Organization` settings

Eight `VSettings` accessors currently read and write `%APPDATA%\Unknown Organization.ini`.

- [ ] InstWinX64.4.1 Isolate test-launched applications from real user settings before changing storage.
- [ ] InstWinX64.4.2 Decide whether `paths/pattern` and `paths/layout` are per-app or shared settings.
- [ ] InstWinX64.4.3 Replace temporary `QSettings` objects with the application’s configured settings object.
- [ ] InstWinX64.4.4 Check `VSeamlyMeSettings` for the same defect.
- [ ] InstWinX64.4.5 Import missing values from the stray settings file without overwriting existing values or deleting the source.
- [ ] InstWinX64.4.6 Test that no Seamly setting resolves under `Unknown Organization`.
- [ ] InstWinX64.4.7 Update the settings-storage documentation in `.github/README-BUILDS.md`.

## Task InstWinX64.5 — Separate user documents from application state

Default user documents to `<DocumentsLocation>/Seamly`. Keep configuration, cache, logs, and recovery in platform-standard application-data locations. Migration runs in application code, not the installer.

- [x] InstWinX64.5.1 Set the default document root to `QStandardPaths::DocumentsLocation/Seamly`.
- [x] InstWinX64.5.2 Preserve relocatability through `paths/dataRoot`.
- [x] InstWinX64.5.3 Copy the complete legacy tree, including unknown folders, without reorganizing it.
- [x] InstWinX64.5.4 Merge without overwriting, verify every copied file, and leave the source intact.
- [x] InstWinX64.5.5 Reject destinations nested inside the source.
- [x] InstWinX64.5.6 Mark migrated roots with `MIGRATED-TO-SEAMLY.txt`.
- [x] InstWinX64.5.7 Configure the new root only after successful verification.
- [x] InstWinX64.5.8 Seed the standard subfolders after resolving the root.
- [x] InstWinX64.5.9 Cover migration with `QTemporaryDir` tests only.
- [x] InstWinX64.5.10 Verify migration on a real Windows profile.
- [ ] InstWinX64.5.11 Add progress, cancellation, or deferral for multi-gigabyte migrations.
- [ ] InstWinX64.5.12 Ensure `pruneEmptyLegacyDataRoot()` never removes a populated or migration-marked tree.
- [ ] InstWinX64.5.13 Move per-app configuration to the platform-standard configuration tree while keeping cache, logs, and recovery separate.


## Task InstWinX64.6 — Add complete ARP metadata

- [ ] InstWinX64.6.1 Write `DisplayIcon` explicitly under the product’s Uninstall registry key while retaining `ARPPRODUCTICON`.
- [ ] InstWinX64.6.2 Determine whether `Publisher` also requires an explicit registry value.
- [ ] InstWinX64.6.3 Validate the authored and installed registry values in both MSI test scripts.
- [ ] InstWinX64.6.4 Verify the icon and publisher in both `appwiz.cpl` and Windows Settings.

## Task InstWinX64.7 — Brand the installer for the Seamly family

- [ ] InstWinX64.7.1 Replace installer-facing “Seamly2D” branding with “Seamly.”
- [ ] InstWinX64.7.2 State that the package installs Seamly2D, SeamlyLayout, and SeamlyMe.
- [ ] InstWinX64.7.3 Change “Seamly2D application family” to “Seamly application family” in the EULA.
- [ ] InstWinX64.7.4 Change package metadata, executable resources, and About dialogs from “Seamly2D Project” to “Seamly Project.”
- [ ] InstWinX64.7.5 Leave source-file copyright headers unchanged.
- [ ] InstWinX64.7.6 Update authoring-test assertions.
- [ ] InstWinX64.7.7 Verify all wizard text visually.

## Task InstWinX64.8 — Shorten and correct the previous-install dialog

- [ ] InstWinX64.8.1 Replace `C:\Users\<you>\seamlyData` with `C:\Users\<you>\Documents\Seamly`.
- [ ] InstWinX64.8.2 Verify the AppData paths against the implemented storage layout.
- [ ] InstWinX64.8.3 Shorten the NSIS warning to: “An older Seamly2D version was found in `C:\Program Files (x86)\Seamly2D`.”
- [ ] InstWinX64.8.4 Remove obsolete advice about moving files from Program Files.
- [ ] InstWinX64.8.5 Shorten the user-data preservation message.
- [ ] InstWinX64.8.6 Change `BannerLine` and `BottomLine` from width 373 to 370.
- [ ] InstWinX64.8.7 Update authoring tests and `INSTALL_DECISION_FLOW.md`.

## Task InstWinX64.9 — Correct the destination-folder page

- [x] InstWinX64.9.1 Keep the `SeamlyApps` program directory.
- [ ] InstWinX64.9.2 Replace “Install Seamly2D to” with wording that names the Seamly application family.
- [ ] InstWinX64.9.3 Show the complete editable destination path, including `SeamlyApps`.
- [ ] InstWinX64.9.4 Update tests and installer documentation.

## Task InstWinX64.10 — Rename the ARP product entry to “Seamly”

One MSI installs all three applications, and will be able to update each separately in the future
Revisit -- if we update each separately in the future should these tasks change?

- [x] InstWinX64.10.1 Keep one ARP entry named “Seamly.” ?? 
- [ ] InstWinX64.10.2 Change `ProductName` and `ARPDISPLAYNAME` to `Seamly`. ??
- [ ] InstWinX64.10.3 Make `ARPCOMMENTS` name Seamly2D, SeamlyMe, and SeamlyLayout. ??
- [ ] InstWinX64.10.4 Update DisplayName assertions in both MSI test scripts. ??
- [ ] InstWinX64.10.5 Confirm NSIS detection remains registry-based and unaffected. ??
- [ ] InstWinX64.10.6 Verify the renamed entry, icon, and publisher in both Windows applets. ??
- [ ] InstWinX64.10.7 Verify upgrades retain the fixed `UpgradeCode` and leave one ARP entry. ??

## Task InstWinX64.11 — Preserve command-line files through first-run dialogs

- [ ] InstWinX64.11.1 Reproduce a first-launch `.sm2d` association outside the automated checker.
- [ ] InstWinX64.11.2 Queue the requested file until first-run dialogs close, or suppress the dialogs when launched with a document.
- [ ] InstWinX64.11.3 Define consistent first-run behavior for Seamly2D, SeamlyMe, and SeamlyLayout.
- [ ] InstWinX64.11.4 Repeat the test with `.smis` and `.smms` files.
- [ ] InstWinX64.11.5 Verify the requested document loads after the first-run flow.
