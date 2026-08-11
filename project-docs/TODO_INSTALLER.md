# TODO — Update cross platform builds

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Check off all completed tasks & subtasks and move completed tasks to TODO_COMPLETED.md

All TODO_MIGRATE.md tasks begin with `Installer.` 

## Installer.1 — Create .msi/.pkg/.appimage/flatpak artifacts as pre-releases in github workflow ci.yml file

- [x] Task Installer.1.1 - Windows x64 .msi - refer to tasks in project-docs\TODO_INSTALLER_WIN_X64.md to define the .msi capabilities and options
  - `ci.yml` gained a `windows-msi` job (x64): one Qt 6.11.1 kit, qmake for seamly2d/seamlyme, CMake/Ninja + Cargo for SeamlyLayout, WiX v6 via `scripts/packaging/windows/smsi.ps1`, jsign signing, artifact `seamly-x64.msi`.
  - The `.msi` **replaces** the NSIS `Seamly2D-windows.zip` in the release; the `windows` job is now arm64-NSIS only until Installer.1.2.
  - `publish` now emits a **pre-release** (`prerelease: true`) and is gated on `run-seamlyLayout`, not `develop` (develop stays a pristine upstream mirror).
  - `windows-msi.yml` kept as the packaging-only workflow; its stale jsign path `Seamly2D-<arch>.msi` corrected to the name `smsi.ps1` writes, `seamly-<arch>.msi`.
  - **Follow-up for the endgame:** `.github/README.md`'s "Windows 64-bit" download badge still points at upstream's `Seamly2D-windows.zip`. Correct to the `.msi` only when the migration is pushed upstream — changing it now breaks the live public download link.
- [ ] Task Installer.1.2 - Windows arm64 .msi - should re-implement the Windows x64 .msi capabilities - track tasks in project-docs\TODO_INSTALLER_WIN_ARM64.md
- [ ] Task Installer.1.3 - MacOS .pkg - refer to tasks in project-docs\TODO_INSTALLER_LINUX_APPIMAGE.md to define the .msi capabilities and options
- [ ] Task Installer.1.4 - Linux .appimage - refer to tasks in project-docs\TODO_INSTALLER_LINUX_APPIMAGE.md to define the .msi capabilities and options
- [ ] Task Installer.1.5 - Linux FlatPak - should re-implement the Linux .appimage capabilities - track tasks in project-docs\TODO_INSTALLER_WIN_ARM64.md

## Installer.2 - Test Seamly pre-releases installation for 3 use cases: a. where Seamly is not previously installed, b. where Seamly version without SeamlyLayout is installed, c. where Seamly version with SeamlyLayout is installed

- Installer.2.1 - Windows x64 .msi
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

## Installer.4 - Re-organize all directories, files, and scripts needed to build the Seamly executables with the GitHub CI/CD ci.yml file so that all CI/CD build information is under the .github directory tree; remove unnecessary and unused CI/CD files; copy files to new location if the original file is under the src/ or share/ directories; update the CI/CD files with the new locations of moved files; build & test the updated CI/CD workflow and artifacts

- Installer.4.1 - Re-organize files; Update ci.yml and related files to reflect new file locations
- Installer.4.2 - Build pre-releases with ci.yml
- Installer.4.3 - Test pre-releases
- Installer.4.3.1 - Windows x64 .msi
- Installer.4.3.2 - Windows arm64 .msi
- Installer.4.3.3 - MacOS .pkg
- Installer.4.3.4 - Linux .appimage
- Installer.4.3.5 - Linux FlatPak
