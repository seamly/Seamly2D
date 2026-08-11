# TODO — Update build for MacOS

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Check off all completed tasks & subtasks and move completed tasks to TODO_COMPLETED.md

Tasks in this file begin with `InstMacOS.`

Tasks in this file may overlap in scope with other tasks -- analyze tasks to consolidate and list them in the order they should be implemented, then renumber to reflect the new ordering, then remove this instruction.

## Task InstMacOS.0 — macOS installer (.pkg) for the Seamly family (parity with the Windows MSI)

Today macOS ships as a drag-installed `.app`/`.dmg` (`packaging/macos/build_dmg.sh`) with no installer, no install-location choice, and no way to clear a prior standalone install. Provide a real macOS installer — a signed/notarized `productbuild`/`pkgbuild` `.pkg` bundling all three apps — matching the Windows MSI capabilities. Where a native `.pkg` cannot do something (arbitrary install-location choice is limited in Installer.app; drag-install has no uninstall hook), record the design decision rather than forcing it. The in-app first-run **data-directory** chooser is **Task 35**; this task is the installer/packaging side and may supersede or complement that first-run prompt with an install-time prompt.

- [ ] InstMacOS.0.1 - Build a `.pkg` installer (`pkgbuild`/`productbuild`) that installs `seamly2d`, `seamlyme`, and `seamlylayout` together, signed and notarized (reuse the macOS signing story), with payload file modes preserved so the app-bundle executables are marked executable (`+x`)
- [ ] InstMacOS.0.1.1 - fix trust issues; macOS: Build error message:
"The following taps are not trusted:
  aws/tap
Homebrew is currently ignoring formulae, casks and commands from these taps because tap trust is required.
Untap them with:
  brew untap aws/tap
Trust specific formulae, casks and commands with:
  brew trust --formula <user>/<tap>/<formula>
  brew trust --cask <user>/<tap>/<cask>
  brew trust --command <user>/<tap>/<command>
Whole-tap trust is broader and includes all current and future formulae,
casks and commands from the listed taps. Trust whole taps with:
  brew trust aws/tap
To disable trust checks:
  export HOMEBREW_NO_REQUIRE_TAP_TRUST=1
This is not recommended and will be removed in a later release.
For more information, see: https://docs.brew.sh/Tap-Trust"
- [ ] InstMacOS.0.1.2 - Let the user choose where the **program files** install (custom install location) — evaluate what the macOS Installer supports (volume/destination selection, relocatable payload) vs. a custom installer UI; document the chosen mechanism and its limits
- [ ] InstMacOS.0.1.3 - Let the user choose where **data files** live (external/cloud storage such as `~/Library/CloudStorage/GoogleDrive-…/Seamly` or `/Volumes/<drive>/Seamly`), persisted via the shared `paths/dataRoot` setting (Task 34, default set by Task 60) — either an installer pane or the Task 35 first-run chooser; pick one and document it
- [ ] InstMacOS.0.1.4 - installation use case #1 : Fresh install - create new directories and files to the new user data root; install program application files
- [ ] InstMacOS.0.1.5 - installation use case #2 : If a standalone Seamly2D/SeamlyMe is already installed but SeamlyLayout is not --> copy old user data files to the new user data root, create only new directories and files to the new user data root; copy user preference and application preference files to new location; remove the old program application files; install new program files
- [ ] InstMacOS.0.1.6 - installation user case #3 : If a standalone Seamly2D/SeamlyMe/SeamlyLayout is already installed --> copy old user data files to the new user data root if the user data root has been changed, create only new directories and files to the new user data root whether or not the user data root has been changed; preserve user preference and application preference files; update program application files
- [ ] InstMacOS.0.1.7 - Verify on macOS: `.pkg` installs all three apps (executable), install-location and data-location choices honored, old standalone removed when SeamlyLayout absent, user data preserved and not overwritten — real-hardware caveat as in Tasks 16/35
- [ ] InstMacOS.0.1.8 - Document the macOS installer (build, sign/notarize, choices, uninstall-old behavior) in the repo docs (`.github/README-BUILDS.md`, `src/app/seamlylayout/docs/packaging-docs/INSTALLER_NOTES.md`)

## Task InstMacOS.1 — Unify settings directories: macOS build

- [ ] InstMacOS.1.1 Verify: fresh install and upgrade-with-legacy-data on macOS; both apps retain preferences after migration — **not verified**, no macOS hardware available in this environment; code changes are cross-platform Qt/CMake, build-verified on Windows (seamlyLayout debug build + all 4 Qt frontend ctest suites + full `cargo test --workspace`, all passing, 2026-07-20), and the `Q_OS_MACOS` branches compile out on other platforms, but real macOS runtime behavior (including the `macos-15` CI runner, which currently only builds seamly2d/seamlyme, not seamlyLayout) remains unexercised
- [ ] InstMacOS.1.2 Verify the macOS build succeeds in GitHub Actions on origin, branch `run-seamlyLayout` (`.github/workflows/ci.yml`'s `macos` job) — note: that job currently builds only seamly2d/seamlyme via qmake (`Seamly.pro`), not seamlyLayout, so a green run confirms the org-name change doesn't break the parent-app macOS build but does not exercise seamlyLayout's own `Q_OS_MACOS` settings-path code (see Task 20 for adding seamlyLayout to CI)

## Task InstMacOS.2 — macOS: let the user choose the `seamlyData` user-data directory (default `~/seamlyData`)

The macOS build ships as a drag-installed `.app` (no installer with a directory picker), so the Windows install-time data-directory prompt (Task 14) has no install-time equivalent on macOS. Provide the same capability as an in-app **first-run chooser**: on first launch, prompt for the user-data root with a native directory picker defaulting to `~/seamlyData` (the Task 34 default), and let the user pick any volume/path — an external disk or a cloud-synced folder (e.g. `~/Library/CloudStorage/GoogleDrive-…/seamlyData` or `/Volumes/<drive>/seamlyData`). Persist the choice via the shared `paths/dataRoot` setting and honor it thereafter; migrate any existing `~/seamly2d` tree. (This is the pattern/measurement **data** tree — distinct from Task 16, which unified the *settings* dirs under `~/Library/Application Support/Seamly`.)

- [ ] InstMacOS.2.1 On first run (macOS), show a native directory-picker prompt for the user-data root, prefilled with `~/seamlyData`, accepting any volume/path incl. external and cloud-synced locations
- [ ] InstMacOS.2.2 Persist the chosen root via the shared `paths/dataRoot` setting (Task 34), resolve all data subfolders under it, and never re-prompt on later launches
- [ ] InstMacOS.2.3 Migrate an existing `~/seamly2d` tree to the chosen root on first run (Task 34's migration), keeping user data intact
- [ ] InstMacOS.2.4 Reflect the "Seamly" family umbrella in the macOS packaging where user-visible (dmg/app naming as appropriate) without breaking the bundle identifiers set in Task 16
- [ ] InstMacOS.2.5 Verify on macOS: the first-run chooser sets the root (incl. a cloud/external path like `/Volumes/GoogleDrive/.../seamlyData`), data reads/writes there, migration from `~/seamly2d` works, and no re-prompt occurs on later launches — real-hardware caveat as in Tasks 16/18
- [ ] InstMacOS.2.6 Document the macOS first-run data-directory chooser in the repo docs
sk 38 — Windows installer: replace a pre-existing standalone Seamly2D/SeamlyMe install and never overwrite existing user data
- [ ] InstMacOS.2.7 Detect a pre-existing **standalone** Seamly2D/SeamlyMe install that the family MSI's `UpgradeCode`/`MajorUpgrade` does not already supersede — the NSIS installer's ARP/uninstall registry entry and/or an install under `C:\Program Files (x86)\Seamly2D` — during the family MSI's install sequence (WiX `<Upgrade>`/`FindRelatedProducts` on the old product code, or a registry/`AppSearch` probe) — 1c
- [ ] InstMacOS.2.8 Only when **SeamlyLayout is not installed** (i.e. this is an upgrade from the pre-family standalone apps, not a repair/upgrade of a family install): run the old uninstaller / remove the old product before laying down the family payload, so the two do not coexist — 1c
- [ ] InstMacOS.2.9 Preserve all user data during that uninstall: never touch `%LOCALAPPDATA%\Seamly\<app>`, `%APPDATA%\Seamly\…`, or the user-data root (`C:\Users\<user>\seamly2d`, →`seamlyData` per Tasks 34/53); if the old NSIS uninstaller would remove any user data, scope/suppress it so data survives — 1c
- [ ] InstMacOS.2.10 Never overwrite an existing user-data directory: on install, create only the directories and files that do not already exist under the user-data root; leave every existing user file untouched (idempotent first-run seeding, not a copy-over) — 1d
- [ ] InstMacOS.2.11 User-data directory picker (`Change` button): confirm the Task 14 user-data prompt exposes a `Change` button opening a native directory picker that accepts any drive/path, explicitly including cloud-sync roots (OneDrive, Google Drive for Desktop, Dropbox) and removable/external media (external HDD, USB) — 1b
- [ ] InstMacOS.2.12 Verify the reported scenario end-to-end: on a machine with the old `C:\Program Files (x86)\Seamly2D` standalone install and no SeamlyLayout, running the family MSI removes the old install, installs the family once (no duplicate), and leaves all pre-existing user files/patterns/measurements intact
- [ ] InstMacOS.2.13 Document the migrate-from-standalone behavior and the no-overwrite guarantee in `scripts/packaging/windows/README.md` and `.github/README-BUILDS.md`
