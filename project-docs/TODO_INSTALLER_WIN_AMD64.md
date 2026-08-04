# TODO — Create the combined MSI installer for Seamly2D, SeamlyMe, and SeamlyLayout

Tasks for creating an .msi file for installation on a user's amd64 computer with Windows 10 or Windows 11.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

## Task 14 — Windows installer: choose program and user-data paths

**Dependencies:** Task 13 (family MSI), Task 34 (`SeamlyData` rename), Task 38 (standalone-install replacement and data protection).

- [ ] **14.1 Program directory**

  - Default to `C:\Program Files\SeamlyApps`.
  - Update `seamly-family.wxs` and all shortcuts, file associations, registry values, and SeamlyLayout paths.
  - Accept any local or removable drive/path; reject cloud-synced locations.
  - Add a `Change` button and silent-install property.

- [ ] **14.2 User-data directory**

  - Default to `C:\Users\<user>\SeamlyData`.
  - Accept any drive/path, including OneDrive, Google Drive, Dropbox, external drives, and USB media.
  - Add a `Change` button and silent-install property.

- [ ] **14.3 Persist both selections** and prefill them during repair or upgrade.
- [ ] **14.4 Register the data root** through a dedicated environment variable, registry value, or application setting so all three apps use it without prompting. Document the selected mechanism.
- [ ] **14.5 Add the executable and data directories to the current user’s `PATH`**, broadcast the change, and remove only installer-created entries during uninstall.
- [ ] **14.6 When the data root changes:**

  - Explain that first run will copy existing data without deleting the source.
  - Require confirmation before continuing.
  - Copy only missing files; never overwrite existing destination files.

- [ ] **14.7 Make Seamly2D, SeamlyMe, and SeamlyLayout honor the configured data root on first run and thereafter.**
- [ ] **14.8 Preserve user data during uninstall;** remove applications, shortcuts, registry/configuration entries, and installer-created `PATH` entries only.
- [ ] **14.9 Optionally offer to launch the Seamly applications after installation.**
- [ ] **14.10 Document** both prompts, silent-install properties, path registration, migration behavior, and uninstall behavior in:

  - `scripts/packaging/windows/README.md`
  - `.github/README-BUILDS.md`

### Verification

- [ ] **14.11 Fresh install**

  - Programs: `C:\Program Files\SeamlyApps`
  - Data: `C:\Users\<user>\SeamlyData`

- [ ] **14.12 Standalone migration**

  - Programs: `C:\Program Files (x86)\Seamly2D` → `E:\Programs\SeamlyApps`
  - Data: `C:\Users\<user>\seamly2d` → `E:\SeamlyData`

- [ ] **14.13 Cloud-data migration**

  - Programs: `C:\Program Files\SeamlyApps`
  - Data: `G:\My Drive\seamly2d` → `G:\My Drive\SeamlyData`

- [ ] **14.14 For each scenario, verify:**

  - The installer remembers and registers both paths.
  - Desktop shortcuts are created.
  - `.sm2d`, measurement, and SVG files open in the correct app.
  - Existing data is copied without overwrite or deletion.
  - Upgrade preserves both paths.
  - Uninstall removes apps, shortcuts, and installer-created `PATH` entries while preserving data.

## Task 13 — Windows MSI installer (x64 and ARM64)

Build one WiX v6 MSI per architecture containing Seamly2D, SeamlyMe, SeamlyLayout, and their dependencies. Store application state in the appropriate AppData directories; user-data paths and migration are covered by Task 14.

**Related:** Task 14 (path selection and data migration), Task 34 (data-root structure), Task 38 (standalone-install replacement).

- [ ] **13.1 Configure WiX v6** using `scripts/packaging/windows/seamly-family.wxs`. Do not adopt WiX v7 until its OSMF EULA is approved.
- [ ] **13.2 Build x64 and ARM64 MSIs** containing:

  - Seamly2D, SeamlyMe, and SeamlyLayout
  - Qt 6.11.1 runtime, QML modules, and WebEngine
  - App-local MSVC runtime
  - Statically linked Rust dependencies

- [ ] **13.3 Add Windows integration:**

  - Start Menu shortcuts for all three apps
  - `.sm2d`, `.smis`, and `.smms` associations
  - In-place major upgrades
  - Clean application uninstall

- [ ] **13.4 Preserve all user data** during installation, upgrade, repair, and uninstall.
- [ ] **13.5 Sign MSI artifacts with `jsign`** when `SEAMLY_SIGNING_PROJECT_ID` is available.
- [ ] **13.6 Support builds through:**

  - Local: `scripts/smsi.ps1`
  - CI: `.github/workflows/windows-msi.yml`

- [ ] **13.7 Test both architectures**, where hardware is available:

  - Clean installation and launch
  - Shortcuts and file associations
  - Upgrade and repair
  - Uninstall without data removal

- [x] **13.8 Complete static x64 validation** — passed July 22, 2026; clean-machine testing remains.
- [ ] **13.9 Document building, signing, and verification in:**

  - `scripts/packaging/windows/README.md`
  - `.github/README-BUILDS.md`
  - `.github/workflows/README_WORKFLOWS.md`

## Task 32 — Suppress `.wixpdb` generation

The WiX v6 MSI build needs only the `.msi`; suppress the optional linker database with `wix build -pdbtype none`.

- [x] 32.1 Add `-pdbtype none` to `$wixArguments` in `scripts/packaging/windows/smsi.ps1`, covering x64, ARM64, local, and CI builds.
- [ ] 32.2 Run a full Seamly MSI build and confirm:

  - [ ] 32.2.1 No `.wixpdb` is generated.
  - [ ] 32.2.2 `wix msi validate` passes.
  - [ ] 32.2.3 Windows Installer COM inspection passes.
  - [ ] 32.2.4 Pending restoration of the Qt build environment tracked in Task 31.

- [x] 32.3 Confirm no scripts, workflows, or inspection tools require `.wixpdb`.
- [x] 32.4 Document that `.wixpdb` is suppressed by default and can be restored by removing `-pdbtype none` from `$wixArguments`.

# TODO — Build the Seamly2D, SeamlyMe, and SeamlyLayout executables on this local PC

## Task 46 — Prevent stale qmake Makefiles after Qt changes

After a Qt change, `sd.ps1` may regenerate the top-level Makefile while reusing stale sub-Makefiles. This can produce misleading missing-library errors. Deleting the shadow-build directory resolves the problem.

- [ ] 46.1 Add `scripts/sd.ps1 -Clean` to remove `scripts/seamly2d-debug/` before configuration.
- [ ] 46.2 Detect qmake/Qt kit changes automatically and recreate the debug build tree before configuring.
- [ ] 46.3 Apply equivalent protection to the release `build/` directory.
- [ ] 46.4 Check `scripts/st.ps1` for the same stale-Makefile behavior and fix it if affected.
- [ ] 46.5 Document automatic cleanup and `-Clean` usage in

  - `sd.ps1`
  - `.github/README-BUILDS.md`

## Task 46 — Prevent stale qmake Makefiles after Qt changes

After a Qt change, `sd.ps1` may regenerate the top-level Makefile while reusing stale sub-Makefiles. This can produce misleading missing-library errors. Deleting the shadow-build directory resolves the problem.

- [ ] 46.1 Add `scripts/sd.ps1 -Clean` to remove `scripts/seamly2d-debug/` before configuration.
- [ ] 46.2 Detect qmake/Qt kit changes automatically and recreate the debug build tree before configuring.
- [ ] 46.3 Apply equivalent protection to the release `build/` directory.
- [ ] 46.4 Check `scripts/st.ps1` for the same stale-Makefile behavior and fix it if affected.
- [ ] 46.5 Document automatic cleanup and `-Clean` usage in:

  - [ ] 46.5.1 `sd.ps1` help
  - [ ] 46.5.2 `.github/README-BUILDS.md`

## Task 51 — Complete the Windows MSI install experience

Related: Tasks 13, 14, 60–67.

- [x] 51.1 Add `test_msi_authoring.ps1` and run it from `smsi.ps1` for both architectures.
- [x] 51.2 Add the standalone `test_msi_install.ps1` phases: `Baseline`, `Installed`, `Upgraded`, and `Removed`.
- [x] 51.3 Verify installation and upgrade on the test laptop: all apps launch, associations work, the install path is preserved, one ARP entry remains, and the legacy NSIS installation is removed.
- [x] 51.4 Verify first-run migration copies the complete legacy tree to `Documents\Seamly` without overwriting or deleting the source.
- [x] 51.5 Add optional Seamly2D and SeamlyMe desktop shortcuts through `SEAMLYDESKTOPSHORTCUTS`; do not offer SeamlyLayout or taskbar shortcuts.
- [x] 51.6 Configure and verify one per-machine UAC elevation prompt.
- [x] 51.7 Warn before replacing an MSI or NSIS installation.
- [ ] 51.8 Make `SeamlyShortcutsDlg` appear under WiX 6; verify through authoring tests and a real wizard run.
- [ ] 51.9 After Task 61, run the `Removed` phase and verify removal of apps, shortcuts, associations, registry entries, and ARP metadata while preserving user data.
- [ ] 51.10 Complete the remaining branding, dialog, and ARP corrections in Tasks 62–66.
- [x] 51.11 Document the installer flow and verification procedure in `scripts/packaging/windows/README.md` and `README_WINDOWS_BUILD.md`.

## Task 52 — Eliminate `Unknown Organization` settings

Eight `VSettings` accessors currently read and write `%APPDATA%\Unknown Organization.ini`.

- [ ] 52.1 Isolate test-launched applications from real user settings before changing storage.
- [ ] 52.2 Decide whether `paths/pattern` and `paths/layout` are per-app or shared settings.
- [ ] 52.3 Replace temporary `QSettings` objects with the application’s configured settings object.
- [ ] 52.4 Check `VSeamlyMeSettings` for the same defect.
- [ ] 52.5 Import missing values from the stray settings file without overwriting existing values or deleting the source.
- [ ] 52.6 Test that no Seamly setting resolves under `Unknown Organization`.
- [ ] 52.7 Update the settings-storage documentation in `.github/README-BUILDS.md`.

## Task 54 — Rename the `vmisc` settings files and classes

Target classes: `SettingsCommon`, `SettingsSeamly2D`, and `SettingsSeamlyMe`.

- [ ] 54.1 Confirm whether filenames match class names or use snake_case; document the convention.
- [ ] 54.2 Rename the six `.h`/`.cpp` files with `git mv`.
- [ ] 54.3 Rename `VCommonSettings`, `VSettings`, and `VSeamlyMeSettings` throughout the source and tests.
- [ ] 54.4 Update `src/libs/vmisc/vmisc.pri`, includes, forward declarations, types, constructors, and qualified calls.
- [ ] 54.5 Rename include guards and file/class documentation.
- [ ] 54.6 Rename the `VCommonSettings` and `VSettings` translation contexts in all 22 `.ts` files.
- [ ] 54.7 Run `lupdate` and confirm the rename creates no obsolete translations.
- [ ] 54.8 Update current documentation and Task 52 references; retain historical names where appropriate.
- [ ] 54.9 Confirm no old filenames, includes, guards, or class names remain.
- [ ] 54.10 Clean the shadow-build directories, then build and run all local and CI test suites.

## Task 55 — Refresh developer setup and build instructions

Update `.github/README-DEVELOPER.md` to reflect the current Qt 6.11.1, MSVC, CMake, Ninja, and Rust toolchains.

- [ ] 55.1 State the supported IDE/compiler combination once and distinguish local from CI requirements.
- [ ] 55.2 Document required Qt modules, including WebEngine, WebChannel, and Positioning, plus Maintenance Tool recovery.
- [ ] 55.3 Document CMake, Ninja, rustup, and Cargo requirements for SeamlyLayout.
- [ ] 55.4 Refresh Windows, Linux, and macOS installation instructions; state the Xpdf/pdftops requirement once.
- [ ] 55.5 Document the qmake/jom Seamly2D and SeamlyMe build and the CMake/Cargo SeamlyLayout build.
- [ ] 55.6 Document `sd.ps1`, `st.ps1`, SeamlyLayout build scripts, and `smsi.ps1`.
- [ ] 55.7 State the Windows developer-shell requirement and warn against Qt Design Studio’s stripped `qmake`.
- [ ] 55.8 Document local tests and the complete `make check` suite used by CI.
- [ ] 55.9 Replace Qt 5 links and remove other obsolete instructions.
- [ ] 55.10 Link to `README-BUILDS.md` and `README_WORKFLOWS.md` instead of duplicating detailed information.
- [ ] 55.11 Follow the Windows instructions literally and record anything not directly verified.

## Task 60 — Separate user documents from application state

Default user documents to `<DocumentsLocation>/Seamly`. Keep configuration, cache, logs, and recovery in platform-standard application-data locations. Migration runs in application code, not the installer.

- [x] 60.1 Set the default document root to `QStandardPaths::DocumentsLocation/Seamly`.
- [x] 60.2 Preserve relocatability through `paths/dataRoot`.
- [x] 60.3 Copy the complete legacy tree, including unknown folders, without reorganizing it.
- [x] 60.4 Merge without overwriting, verify every copied file, and leave the source intact.
- [x] 60.5 Reject destinations nested inside the source.
- [x] 60.6 Mark migrated roots with `MIGRATED-TO-SEAMLY.txt`.
- [x] 60.7 Configure the new root only after successful verification.
- [x] 60.8 Seed the standard subfolders after resolving the root.
- [x] 60.9 Cover migration with `QTemporaryDir` tests only.
- [x] 60.10 Verify migration on a real Windows profile.
- [ ] 60.11 Add progress, cancellation, or deferral for multi-gigabyte migrations.
- [ ] 60.12 Correctly resolve `~/seamly2d`, `~/seamlyData`, and `Documents/Seamly` when multiple roots exist.
- [ ] 60.13 Ensure `pruneEmptyLegacyDataRoot()` never removes a populated or migration-marked tree.
- [ ] 60.14 Move per-app configuration to the platform-standard configuration tree while keeping cache, logs, and recovery separate.
- [ ] 60.15 Replace remaining `seamlyData` references in the installer UI through Task 64.

## Task 61 — Repair `test_msi_install.ps1`

- [ ] 61.1 Replace bare install-state values with named constants: `INSTALLSTATE_LOCAL = 3` and `INSTALLSTATE_SOURCE = 4`.
- [ ] 61.2 Require advertised shortcuts to resolve to a non-empty installed component path.
- [ ] 61.3 Snapshot `Documents\Seamly` after first-run migration, before the upgrade.
- [ ] 61.4 Replace the sample pattern with a self-contained file and verify that it loads, not merely that Seamly2D starts.
- [ ] 61.5 Re-run the affected phases with no false failures before Task 51’s uninstall test.

## Task 62 — Add complete ARP metadata

- [ ] 62.1 Write `DisplayIcon` explicitly under the product’s Uninstall registry key while retaining `ARPPRODUCTICON`.
- [ ] 62.2 Determine whether `Publisher` also requires an explicit registry value.
- [ ] 62.3 Validate the authored and installed registry values in both MSI test scripts.
- [ ] 62.4 Verify the icon and publisher in both `appwiz.cpl` and Windows Settings.

## Task 63 — Brand the installer for the Seamly family

- [ ] 63.1 Replace installer-facing “Seamly2D” branding with “Seamly.”
- [ ] 63.2 State that the package installs Seamly2D, SeamlyLayout, and SeamlyMe.
- [ ] 63.3 Change “Seamly2D application family” to “Seamly application family” in the EULA.
- [ ] 63.4 Change package metadata, executable resources, and About dialogs from “Seamly2D Project” to “Seamly Project.”
- [ ] 63.5 Leave source-file copyright headers unchanged.
- [ ] 63.6 Update authoring-test assertions.
- [ ] 63.7 Verify all wizard text visually.

## Task 64 — Shorten and correct the previous-install dialog

- [ ] 64.1 Replace `C:\Users\<you>\seamlyData` with `C:\Users\<you>\Documents\Seamly`.
- [ ] 64.2 Verify the AppData paths against the implemented storage layout.
- [ ] 64.3 Shorten the NSIS warning to: “An older Seamly2D version was found in `C:\Program Files (x86)\Seamly2D`.”
- [ ] 64.4 Remove obsolete advice about moving files from Program Files.
- [ ] 64.5 Shorten the user-data preservation message.
- [ ] 64.6 Change `BannerLine` and `BottomLine` from width 373 to 370.
- [ ] 64.7 Update authoring tests and `INSTALL_DECISION_FLOW.md`.

## Task 65 — Correct the destination-folder page

Task 14 retains `C:\Program Files\SeamlyApps` as the default.

- [x] 65.1 Keep the `SeamlyApps` program directory.
- [ ] 65.2 Replace “Install Seamly2D to” with wording that names the Seamly application family.
- [ ] 65.3 Show the complete editable destination path, including `SeamlyApps`.
- [ ] 65.4 Update tests and installer documentation.

## Task 66 — Rename the ARP product entry to “Seamly”

One MSI installs all three applications, so Windows should show one family-level ARP entry.

- [x] 66.1 Keep one ARP entry named “Seamly.”
- [ ] 66.2 Change `ProductName` and `ARPDISPLAYNAME` to `Seamly`.
- [ ] 66.3 Make `ARPCOMMENTS` name Seamly2D, SeamlyMe, and SeamlyLayout.
- [ ] 66.4 Update DisplayName assertions in both MSI test scripts.
- [ ] 66.5 Confirm NSIS detection remains registry-based and unaffected.
- [ ] 66.6 Verify the renamed entry, icon, and publisher in both Windows applets.
- [ ] 66.7 Verify upgrades retain the fixed `UpgradeCode` and leave one ARP entry.

## Task 67 — Preserve command-line files through first-run dialogs

- [ ] 67.1 Reproduce a first-launch `.sm2d` association outside the automated checker.
- [ ] 67.2 Queue the requested file until first-run dialogs close, or suppress the dialogs when launched with a document.
- [ ] 67.3 Define consistent first-run behavior for Seamly2D, SeamlyMe, and SeamlyLayout.
- [ ] 67.4 Repeat the test with `.smis` and `.smms` files.
- [ ] 67.5 Verify the requested document loads after the first-run flow.
