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

- [x] Task Installer.1.1 - Windows x64 .msi - refer to tasks in project-docs\TODO_INSTALLER_WIN_X64.md to define the .msi capabilities and options

- [x] Task Installer.1.2 - Windows arm64 .msi - should re-implement the Windows x64 .msi capabilities - track tasks in project-docs\TODO_INSTALLER_WIN_ARM64.md
  - `ci.yml`'s `windows-msi` job is now a **matrix over `arch`** (`x64`, `arm64`, `fail-fast: false`) — `windows-msi.yml`'s `msi` job verbatim, minus its own version step. 
  - **NSIS retired.** The `windows` job is deleted; nothing runs `makensis` any more. `publish` releases `seamly-x64.msi` + `seamly-arm64.msi` in place of `Seamly2D-win-arm64.zip`.
  - **Verification is CI-only** (no arm64 hardware here): the arm64 leg has to build, validate and pass `smsi_check_authoring.ps1` in the run for this change. A real arm64 install still has never been run — that stays Installer.2.2.

- [ ] Task Installer 1.4 - **Seamly Apps for Windows 11 (x64)** must be built on the Github `windows-latest` runner, and **Seamly Apps for Windows 11 (arm64)** must be built on the `windows_11_arm` runner. Both builds should contain Seamly2D/SeamlyLayout/SeamlyMe that run in the same Qt runtime. SeamlyLayout needs a Rust + cxx-qt build and a Qt WebEngine -- the binaries for these exist on windows_11_arm and on windows-latest.
- [ ] Task Installer.1.5 - MacOS .pkg - refer to tasks in project-docs\TODO_INSTALLER_LINUX_APPIMAGE.md to define the .msi capabilities and options
- [ ] Task Installer.1.6 - Linux .appimage - refer to tasks in project-docs\TODO_INSTALLER_LINUX_APPIMAGE.md to define the .msi capabilities and options
- [ ] Task Installer.1.7 - Linux FlatPak - should re-implement the Linux .appimage capabilities - track tasks in project-docs\TODO_INSTALLER_WIN_ARM64.md

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
  - **DONE 2026-08-10 — `.github/workflows/action.yaml` deleted.** It was never a workflow: it is Corrosion's own `setup_test_environment` *composite action* (`name`/`description`/`inputs`/`runs`, no `jobs`), added by accident in `d1bb78c495` ("updated ilammy action in .yml/.yaml files", 2026-08-03). GitHub tried to run it as a workflow on every push and failed with *"Required property is missing: jobs"*, which is what made the branch look red. Nothing in the repo referenced it, and Corrosion is fetched by CMake rather than vendored, so there was nowhere for it to belong.
- Installer.4.2 - Build pre-releases with ci.yml
- Installer.4.3 - Test pre-releases
- Installer.4.3.1 - Windows x64 .msi
- Installer.4.3.2 - Windows arm64 .msi
- Installer.4.3.3 - MacOS .pkg
- Installer.4.3.4 - Linux .appimage
- Installer.4.3.5 - Linux FlatPak

  ## Installer.5 - `.github/README.md`'s "Windows 64-bit" download badge still points at upstream's `Seamly2D-windows.zip` and must become the `.msi` — tracked as **Task M.12** in `TODO_MIGRATE.md`. Do it only when the migration is pushed upstream; changing it earlier breaks the live public download link
