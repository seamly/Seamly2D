# TODO — Update cross platform builds

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Check off all completed tasks & subtasks and move completed tasks to TODO_COMPLETED.md

All TODO_MIGRATE.md tasks begin with `Installer.`

Notes:

- **Data migration is copy-and-verify, leaving the legacy tree intact**, never
   a bare rename, because a user may need to roll back to an earlier release.
- **Windows interactive updates use an impersonated MSI migration action.** It
   reads the installing user's paths and settings. Fresh installs and other
   platforms keep application first-run creation and fallback migration.
- **Testing happens on the test laptop, not in a VM** 
   The Windows PC is Windows 11 **Home**, which ships neither Hyper-V nor Windows
   Sandbox, so a VM here means a third-party hypervisor; the user considered
   VirtualBox and VMware Workstation Pro and declined both. A VM could not close
   two checklist items anyway — the *verified-publisher* UAC prompt needs Task
   33's signing, and the arm64 repeat needs arm64 hardware.
- **Pre-releases are cut from `run-seamlyLayout`**; `develop` stays a pristine 
    upstream mirror** Nothing is published from `develop` until the whole
    SeamlyLayout migration is finished and pushed upstream in one go —
    incremental upstream commits are not workable given the size of the change.
- **Data-root relocation asks first** — prompt Y/N before copying existing data
   files to a new directory location.

## Installer.1 — Create .msi/.pkg/.appimage/flatpak artifacts as pre-releases in github workflow ci.yml file

- [x] Task Installer.1.1 - Windows x64 .msi - refer to tasks in project-docs\TODO.md to define the .msi capabilities and options

- [x] Task Installer.1.2 - Windows arm64 .msi - should re-implement the Windows x64 .msi capabilities - track tasks in project-docs\TODO_INSTALLER_WIN_ARM64.md
  - `ci.yml`'s `windows-msi` job is now a **matrix over `arch`** (`x64`, `arm64`, `fail-fast: false`) — `windows-msi.yml`'s `msi` job verbatim, minus its own version step. 
  - **NSIS retired.** The `windows` job is deleted; nothing runs `makensis` any more. `publish` releases `seamly-x64.msi` + `seamly-arm64.msi` in place of `Seamly2D-win-arm64.zip`.
  - **Verification is CI-only** (no arm64 hardware here): the arm64 leg has to build, validate and pass `smsi_check_authoring.ps1` in the run for this change. A real arm64 install still has never been run — that stays Installer.2.2.

- [ ] Task Installer 1.4 - **Seamly Apps for Windows 11 (x64)** must be built on the Github `windows-latest` runner, and **Seamly Apps for Windows 11 (arm64)** must be built on the `windows_11_arm` runner. Both builds should contain Seamly2D/SeamlyLayout/SeamlyMe that run in the same Qt runtime. SeamlyLayout needs a Rust + cxx-qt build and a Qt WebEngine -- the binaries for these exist on windows_11_arm and on windows-latest.
- [ ] Task Installer.1.5 - MacOS .pkg - refer to tasks in project-docs\TODO_INSTALLER_LINUX_APPIMAGE.md to define the .msi capabilities and options
- [ ] Task Installer.1.6 - Linux .appimage - refer to tasks in project-docs\TODO_INSTALLER_LINUX_APPIMAGE.md to define the .msi capabilities and options
- [ ] Task Installer.1.7 - Linux FlatPak - should re-implement the Linux .appimage capabilities - track tasks in project-docs\TODO_INSTALLER_WIN_ARM64.md

## Installer.2 - Test Seamly pre-releases installation for 3 use cases: a. where Seamly is not previously installed, b. where Seamly version without SeamlyLayout is installed, c. where Seamly version with SeamlyLayout is installed

- [x] Installer.2.1 - Windows x64 .msi
- Installer.2.2 - Windows arm64 .msi
- Installer.2.3 - MacOS .pkg
- Installer.2.4 - Linux .appimage
- Installer.2.5 - Linux FlatPak

## Task Installer.3 - Create step-by-step instructions as .pdf (including steps regarding data migration from previous version without SeamlyLayout) for each (Win X64, Win Arm64, MacOS, Linux AppImage -- not needed for Linux FlatPak)

- Installer.3.1 - Windows x64 .msi
- Installer.3.2 - Windows arm64 .msi
- Installer.3.3 - MacOS .pkg
- Installer.3.4 - Linux .appimage
- Installer.3.5 - Linux FlatPak

## Installer.5 - [known defect] explicit SEAMLYDATAPARENT is not recorded on a property-driven install

Found 2026-09-01 during the SettingsFiles.4.3 upgrade verification. A quiet upgrade
with `SEAMLYDATAPARENT=<Documents>\SeamlyUpgradeTest\ SEAMLYDATAROOT=<Documents>\SeamlyUpgradeTest\SeamlyData\`
recorded `DataRoot` correctly but recorded `DataParent=<Documents>\` — the
execute-sequence default computation (InstWinX64.13) overrode the explicit parent.
The apps read only `DataRoot`, so this is cosmetic today, but the recorded pair is
inconsistent (`DataParent` is not the parent of `DataRoot`).

- [ ] Installer.5.1 Guard the `SetSEAMLYDATAPARENTExecute*` actions so an explicitly passed `SEAMLYDATAPARENT` wins, or derive the recorded `DataParent` from the recorded `DataRoot`.
- [ ] Installer.5.2 Add an authoring assertion; re-verify with a property-driven quiet install.

## Installer.6 - [2026-09-15] Revert the Windows data directory to a fixed `C:\Users\<user>\seamly2d` — no picker, no migration

User feedback: do not change the data directory from `C:\Users\<user>\seamly2d` to
`C:\Users\<user>\Documents\SeamlyData`. This reverts the `Documents\SeamlyData`
design completed under `SettingsFiles.7`/`Layout.11` (see `TODO_COMPLETED.md`) and
simplifies the whole workflow:

- [x] Installer.6.1 Fresh install: `SEAMLYDATAROOT` fixed to `[%USERPROFILE]\seamly2d`, no wizard page asks for it.
- [x] Installer.6.2 Update install: read the current data directory from the registry (`HKLM\SOFTWARE\Seamly\Seamly2D\DataRoot`); show it read-only in a new `SeamlyDataLocationDlg` (Cancel or Continue, no edit); install only new subdirectories/files, never touch existing data.
- [x] Installer.6.3 Repair install: same directory re-read; missing subdirectories/files added silently, no dialog.
- [x] Installer.6.4 Uninstall: data directory and its contents left untouched (already true — verified, not changed).
- [x] Installer.6.5 Remove the picker (`SeamlyDataDirDlg`, `SEAMLYDATAPARENT`, `BrowseDlg` wiring) and the copy-my-data prompt (`SeamlyDataMigrateDlg`, `SEAMLYCOPYUSERDATA`) from `smsi_ui.wxs`/`smsi.wxs`. Drop `DataParent` from the registry rows (`smsi_registry.wxs`).
- [x] Installer.6.6 Replace `smsi_migrate_user_data.ps1` (cross-directory archive/merge) with `smsi_ensure_user_data.ps1` (creates standard subdirectories + seeds settings ini files only, in the current root — fresh, update, and repair alike).
- [x] Installer.6.7 App side: `VCommonSettings::getDefaultDataRoot()` returns `~/seamly2d` on every platform (matches the old `getLegacyDataRoot()`, now removed). Delete the now-dead legacy-migration subsystem: `legacy_data_migration.{h,cpp}`, `legacy_data_archive.{h,cpp}`, `migrateAdoptedLegacyTree()`, `pruneEmptyLegacyDataRoot()`, `chooseFirstRunDataRoot()`, the `firstRunNoticePending`/`markFirstRunNoticeShown` one-shot notice, and `VAbstractApplication::NotifySeamlyDataLocation()`.
- [ ] Installer.6.8 Manual install-matrix verification on the test laptop (fresh / update-with-custom-root / repair / uninstall) — see `project-docs/TEST_WIN_MSI_Test_Case_template.md`, updated for this task.

**[2026-09-18] Partial reversal of Installer.6.5:** user feedback now asks for
the opposite on a fresh install only — see them the default location and let
them Change it. `SeamlyDataLocationDlg` shows read-only text when
`SEAMLYDATAROOTRECORDED` (6.2 unchanged: update/repair still never edit an
existing root), or an editable path box plus a Change button (stock
`BrowseDlg`, re-added) when nothing is recorded yet. `SEAMLYDATAROOT` stays a
Directory table entry so `BrowseDlg`'s stock `SetTargetPath`/indirect
controls have something real to browse. Verified: `wix build` compiles,
`smsi_check_authoring.ps1` passes (assertions rewritten for the new
two-variant page), `smsi_fix_dialog_lines.ps1` finds no non-Line overflow.
Not yet verified: an actual click-through on Windows (Change → BrowseDlg →
OK), and a full `local_build_msi.ps1` run.

## Installer.4 - Re-organize all directories, files, and scripts needed to build the Seamly executables with the GitHub CI/CD ci.yml file so that all CI/CD build information is under the .github directory tree; remove unnecessary and unused CI/CD files; copy files to new location if the original file is under the src/ or share/ directories; update the CI/CD files with the new locations of moved files; build & test the updated CI/CD workflow and artifacts

- Installer.4.1 - Re-organize files; Update ci.yml and related files to reflect new file locations
  - **DONE 2026-08-10 — `.github/workflows/action.yaml` deleted.** It was never a workflow: it is Corrosion's own `setup_test_environment` *composite action* (`name`/`description`/`inputs`/`runs`, no `jobs`), added by accident in `d1bb78c495` ("updated ilammy action in .yml/.yaml files", 2026-08-03). GitHub tried to run it as a workflow on every push and failed with *"Required property is missing: jobs"*, which is what made the branch look red. Nothing in the repo referenced it, and Corrosion is fetched by CMake rather than vendored, so there was nowhere for it to belong.
- Installer.4.2 - Build pre-releases with ci.yml
- Installer.4.3 - Test pre-releases
- Installer.4.3.1 - Windows x64 .msi
- Installer.4.3.2 - Windows arm64 .msi
- Installer.4.3.3 - MacOS .pkg
- Installer.4.3.4 - Linux .appimage
- Installer.4.3.5 - Linux FlatPak

  ## Installer.5 - `.github/README.md`'s "Windows 64-bit" download badge still points at upstream's `Seamly2D-windows.zip` and must become the `.msi` — tracked as **Task M.12** in `TODO_MIGRATE.md`. Do it only when the migration is pushed upstream; changing it earlier breaks the live public download link

## [ ] Installer.7 - Ship license notices for the non-Qt, non-Rust components in every installer

Task Layout.14 ships notices for the Rust crates, the Qt runtime and the SVG export fonts. These shipped components still have none:

- [ ] Installer.7.1 - xerces-c (Apache-2.0): `xerces-c_3_3.dll` (Windows), system/Homebrew library bundled on Linux and macOS
- [ ] Installer.7.2 - pdftops (Poppler, GPL-2.0-or-later): bundled on all three platforms; GPL also requires a source offer
- [ ] Installer.7.3 - MSVC runtime DLLs (Microsoft redistributable terms), Windows only
- [ ] Installer.7.4 - Add the notices to `packaging/licenses/` (all three installers pick up that folder); document in `.github/README-BUILDS.md`
