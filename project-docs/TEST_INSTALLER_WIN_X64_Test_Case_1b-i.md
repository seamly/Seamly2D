# TEST_INSTALLER_WIN_X64

Test plan for the Windows x64 Seamly MSI. Covers `scripts/packaging/windows/smsi.wxs`.

## Status (2026-08-28)

- **Installed and verified on this machine.** `scripts\seamly-msi\x64\seamly-x64.msi`
  (version 26.8.24.982) was reset, installed with default settings (no properties, `/quiet`),
  and run through `test_reset_environment.ps1` + `test_msi_install.ps1 -Phase Baseline/Installed`
  plus manual registry/filesystem/app-launch checks. See results below each item.
- This build carries the `InstWinX64.13` fixes (`DataParent`/`DataRoot` recording for a
  no-properties `/quiet` install, `%LOCALAPPDATA%\Seamly` / `%APPDATA%\Seamly` removal on
  uninstall) — see `TODO_INSTALLER_WIN_X64.md`. `InstWinX64.13.5` real-machine verification:
  **DataParent/DataRoot recording confirmed correct** for all three apps' registry keys (see B.4b).
- Elevation note: `test_reset_environment.ps1` needs an elevated shell for the `C:\Program
  Files\SeamlyApps` deletion, not just for the msiexec uninstall and HKLM keys — its own
  docstring undersells this. Run it from an Administrator PowerShell, not just expect the
  in-script `-Verb RunAs` on the msiexec call to cover everything.
- **Verification Suite (Section B) re-run 2026-08-28, automated portion.** `test_msi_install.ps1
  -Phase Installed -ExpectSeamlyLayout -SkipLaunch` reproduced exactly 5 known, already-tracked
  failures and nothing new: `InstWinX64.6.1` (ARP `DisplayIcon` empty), `InstWinX64.6.15` (ARP
  `DisplayName` assertion checks the stale `Seamly2D` name; the product correctly registers
  `Seamly`), and `InstWinX64.14.2` (all three Start Menu shortcuts, off-by-one `INSTALLSTATE_*`
  constants in the script itself). All filesystem, registry, association, and desktop-shortcut
  checks passed. Manually confirmed on top of the script: no stray `%APPDATA%\Unknown
  Organization\` folder, no nested/duplicate `SeamlyData` directory, `qt6_common.ini`'s
  `dataRoot` matches `%DATAROOT%` exactly, and the old NSIS registry key is absent (nothing to
  clean up on this fresh install).
- **Section 6a-i is blocked on this build.** The installed MSI (built 2026-08-24) still ships
  `samples\measurements\individual\male_shirt.smis`; the rename to `male_chest_102cm.smis` (this
  doc, commit `37de90cb73`) landed in the source tree afterward and is not in this package.
  Rebuild the MSI before running 6a-i/6a-iii for real. See `InstWinX64.15` in
  `TODO_INSTALLER_WIN_X64.md`.
- **Section 6 (app launch/menu checks) needs a human at the keyboard.** Confirmed only that all
  three installed executables start and stay running (`seamly2d.exe`, `seamlyme.exe`,
  `SeamlyLayout.exe` launched standalone, settled, closed cleanly). One `seamlyme.exe` launch
  exited within 6 seconds on the very first try; a second and third launch ran normally with no
  error in stdout/stderr and no crash. Not reproduced — watch for it, don't file a task on one
  unrepeated instance.
- **Log check (B.7):** `seamly2d`'s latest log (`%LOCALAPPDATA%\Seamly\Seamly2D\logs\`) is clean
  — no warnings or errors. SeamlyMe and SeamlyLayout have no `logs\` directory at all; only
  `Application2D::logDirPath()` was wired up (SESSION_HANDOVER, 2026-08-20), so this is expected,
  not a defect.
- **Section B.1–B.5 and B.7 run live 2026-08-28.** Machine was in a genuine fresh-install state
  (MSI installed, no app ever launched — `%LOCALAPPDATA%\Seamly` and `%APPDATA%\Seamly` did not
  exist yet). Ran `seamly2d.exe`, let it settle 8s, closed it. Result: all nine `%DATAROOT%`
  subdirectories created at the correct level with no duplicates; `%LOCALAPPDATA%\Seamly\<AppName>\`
  exists for all three apps; `%APPDATA%\Seamly\qt6_common.ini` exists with `dataRoot` matching
  `%DATAROOT%` exactly; `HKLM\SOFTWARE\Seamly\{Seamly2D,SeamlyMe,SeamlyLayout}` all carry
  `InstallPath`/`DataRoot`/`DataParent`; new `seamly2d` log is clean. No new defects found — this
  reconfirms the earlier automated pass rather than replacing it. `HKCU\Software\Seamly\Seamly2D`'s
  `RemovedLocalAppData`/`RemovedRoamingAppData` values are expected on install (they are the
  `PerUserSettingsRemoval` components' `KeyPath`, per `smsi_registry.wxs`) — not leftover uninstall
  residue, checked against the authoring to rule that out.
  Reconfirmed still blocked: samples ship as `male_shirt.smis` (not the renamed
  `male_chest_102cm.smis`) — `InstWinX64.15`/Section 6a-i unchanged.

## Variable Names

The three names in the request are not real environment variables. Corrected below.

| Requested name | Status | Correct reference |
| --- | --- | --- |
| `%SEAMLYPROGRAMDIR%` | Not real | `INSTALLFOLDER` — MSI property. Default `C:\Program Files\SeamlyApps`. Recorded at `HKLM\SOFTWARE\Seamly\Seamly2D\InstallPath`. |
| `%SEAMLYUSERDATAROOT%` | Not real | `SEAMLYDATAROOT` — MSI property (raw path chosen). Default `<Documents>\SeamlyData`. Recorded value is `SEAMLYDATAROOTRECORDED`, stored at `HKLM\SOFTWARE\Seamly\Seamly2D\DataRoot`. Apps read it through `InstallerRecord::dataRoot()`. |
| `%SEAMLYAPPLICATIONDIR%` | Not real | `%LOCALAPPDATA%\Seamly\<AppName>\` — a real Windows variable plus a fixed subpath, from `QStandardPaths::AppConfigLocation`. `<AppName>` is `Seamly2D`, `SeamlyMe`, or `SeamlyLayout`. |

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`.

Known defect to watch for: an empty organization name can make Qt write settings under
`%APPDATA%\Unknown Organization\` instead of `%LOCALAPPDATA%\Seamly\<AppName>\`. See
`src/libs/vmisc/vcommonsettings.cpp`. Check for this stray folder in every verification pass.

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Not installed | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Fresh installed

- [ ] 1a. Uninstall Seamly (any and all versions detected) using `/test_reset_environment.ps1`
  - [ ] 1a-i. Confirm that %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly, desktop shortcuts, and registry keys have been removed
    - Ran `test_reset_environment.ps1` elevated. Verified all six locations absent:
      `C:\Program Files\SeamlyApps`, `Documents\SeamlyData`, `%LOCALAPPDATA%\Seamly`,
      `%APPDATA%\Seamly`, `HKLM\SOFTWARE\Seamly`, `HKCU\Software\Seamly`. Confirmed by
      `test_msi_install.ps1 -Phase Baseline` passing.
- [ ] 1b. Install Seamly apps using `scripts\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

## B. Verification Suite

Run this suite after every test case in section A.

- [ ] 1. Run Seamly2D then close Seamly2D to install the user directories
- [ ] 2. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`)
- [ ] 3. Check user-data location (default `C:\Users\<user>\Documents\SeamlyData\`), subdirectories, and files:
  - [ ] 3a. No duplicate directories
  - [ ] 3b. Subdirectories `backups`, `bodyscans`, `images`, `label templates`, `layouts`,  `measurements\individual`, `measurements\multisize`, `patterns`, and `templates` are created at the correct level below `%DATAROOT%`
  - [ ] 3c. if upgrading from previous non-SeamlyLayout version then: **N/A — Case 1 fresh install.**
    - [ ] 3c-i. confirm `%DATAROOT%\seamly2d.zip` exists
    - [ ] 3c-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories
- [ ] 4. Check the user application directories:
  - [ ] 4a. `%LOCALAPPDATA%\Seamly\<AppName>\` directories exist for Seamly2D, SeamlyMe, and SeamlyLayout.
  - [ ] 4b. `%APPDATA%\Seamly\qt6_common.ini` file exists
    - [ ] 4b-i. Confirm all paths in qt6_common.ini start with `%DATAROOT%` value.
- [ ] 5. Check the registry keys:
  - [ ] 5a. If not a fresh install then confirm old-version entries were removed. **N/A — Case 1 fresh install.**
  - [ ] 5b. Confirm that the installed-version program entries were added, under `HKLM\SOFTWARE\Seamly\Seamly2D`, `HKLM\SOFTWARE\Seamly\SeamlyMe`, and `HKLM\SOFTWARE\Seamly\SeamlyLayout`
  - [ ] 5c. Confirm that installed-version data entries were added
- [ ] 6. Check the apps
  - [ ] 6a. Check Seamly2D and SeamlyMe
    - [ ] 6a-i. Open `%PROGRAMDIR%\samples\patterns\male_shirt.sm2d` pattern file with `%PROGRAMDIR%\samples\measurements\individual\male_chest_102cm.smis` individual measurement file.
    - [ ] 6a-ii. Run SeamlyMe from within Seamly2D  --> prompt human to select 'Edit Current' from the Measurements menu in Seamly2D
    - [ ] 6a-iii. Save current measurement file to `%DATAROOT\measurements\individual\male_chest_102cm.smis`
    - [ ] 6a-iv. Close SeamlyMe, returning focus to Seamly2D
    - [ ] 6a-v. Save current pattern file to `%DATAROOT\patterns\male_shirt.sm2d`
  - [ ] 6b. Check SeamlyLayout
    - [ ] 6b-i. Run SeamlyLayout from within Seamly2D --> prompt human to select the SeamlyLayout icon in Seamly2D
    - [ ] 6b-ii. Confirm that the current pattern's `Piece mode` data was passed to SeamlyLayout as a stringified svg document (not as a svg file) --> prompt human to confirm, or use code-level confirmation of the IPC payload shape
    - [ ] 6b-iv. Close SeamlyLayout, returning focus to Seamly2D
  - [ ] 6c. Close Seamly2D
- [ ] 7. Check the logs for additional errors
