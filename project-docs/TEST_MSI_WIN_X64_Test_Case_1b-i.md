# TEST_MSI_WIN_X64_Test_Case_1b-i.md

Test plan for the Windows x64 Seamly MSI. Covers `scripts/packaging/windows/smsi.wxs`.

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`.

Known defect to watch for: an empty organization name can make Qt write settings under `%APPDATA%\Unknown Organization\` instead of `%LOCALAPPDATA%\Seamly\<AppName>\`. See `src/libs/vmisc/vcommonsettings.cpp`. Check for this stray folder in every verification pass.

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Fresh install | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Fresh install

- [x] 0. Relaunch this shell elevated (Administrator) before any step below.
  - Blocker found 2026-08-28: the shell was not elevated. `test_reset_environment.ps1`
    calls `msiexec /x ... -Verb RunAs`, and `msiexec /i ... /quiet` for a
    per-machine install also needs admin rights. A non-interactive tool
    session cannot answer the UAC consent prompt, so the attempt would hang
    rather than fail cleanly. User decision: relaunch Claude Code as
    Administrator, then resume this task. Resolved 2026-08-28: VS Code
    relaunched as Administrator; shell confirmed elevated.
- [x] 1a. Uninstall Seamly (any and all versions detected) using `/test_reset_environment.ps1`
  - Ran 2026-08-28. Uninstalled Seamly 26.8.34102 ({E4087718-E81E-4FD7-9568-CCBAB613693B}); removed %DATAROOT%, AppData\Local\Seamly, AppData\Roaming\Seamly.
  - [x] 1a-i. Confirm that %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly, desktop shortcuts, and registry keys have been removed
    - Confirmed 2026-08-28: all `Test-Path` checks (ProgramDir, DataRoot, AppData\Roaming\Seamly, AppData\Local\Seamly, 3 desktop shortcuts, HKLM\SOFTWARE\Seamly, HKCU\Software\Seamly) returned `False`.
- [x] 1b. Install Seamly apps using `scripts\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`
  - Ran 2026-08-28. Exit code 0. Log: `%TEMP%\seamly_install_1b.log`.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

## B. Verification Suite

Run this suite after every test case in section A.

- [x] 1. Run Seamly2D then close Seamly2D to install the user directories
  - Ran 2026-08-28. Also ran SeamlyMe and SeamlyLayout standalone (not via Seamly2D's menu/icon) to satisfy 4a — see note there.
- [x] 2. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`)
  - Confirmed 2026-08-28: `C:\Program Files\SeamlyApps\` (from `HKLM\SOFTWARE\Seamly\Seamly2D\InstallPath`).
- [x] 3. Check user-data location (default `C:\Users\<user>\Documents\SeamlyData\`), subdirectories, and files:
  - Confirmed 2026-08-28: `C:\Users\susan\Documents\SeamlyData\` exists.
  - [x] 3a. No duplicate directories
    - Confirmed: case-insensitive path grouping found no collisions.
  - [x] 3b. Subdirectories `backups`, `bodyscans`, `images`, `label templates`, `layouts`,  `measurements\individual`, `measurements\multisize`, `patterns`, and `templates` are created at the correct level below `%DATAROOT%`
    - Confirmed: all 9 present at the correct level.
  - [x] 3c. if upgrading from previous non-SeamlyLayout version then: **N/A — Case 1 fresh install.**
    - [x] 3c-i. confirm `%DATAROOT%\seamly2d.zip` exists — **N/A, fresh install.**
    - [x] 3c-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories — **N/A, fresh install.**
- [x] 4. Check the user application directories:
  - [x] 4a. `%LOCALAPPDATA%\Seamly\<AppName>\` directories exist for Seamly2D, SeamlyMe, and SeamlyLayout.
    - Confirmed 2026-08-28, after running each app standalone once. Running Seamly2D alone (item 1) creates only its own directory; SeamlyMe and SeamlyLayout each need to run at least once to create theirs. SeamlyLayout.exe exited within ~8s (exit code 0, not a crash) when launched standalone with no pattern data — expected, since it receives Piece-mode SVG via IPC from Seamly2D rather than running standalone. See new Task 8 below.
  - [x] 4b. `%APPDATA%\Seamly\qt6_common.ini` file exists
    - Confirmed 2026-08-28.
    - [x] 4b-i. Confirm all paths in qt6_common.ini start with `%DATAROOT%` value.
      - Confirmed: file contains one key, `dataRoot=c:/Users/susan/Documents/SeamlyData`, matching `%DATAROOT%`.
- [x] 5. Check the registry keys:
  - [x] 5a. If not a fresh install then confirm old-version entries were removed. **N/A — Case 1 fresh install.**
  - [x] 5b. Confirm that the installed-version program entries were added, under `HKLM\SOFTWARE\Seamly\Seamly2D`, `HKLM\SOFTWARE\Seamly\SeamlyMe`, and `HKLM\SOFTWARE\Seamly\SeamlyLayout`
    - Confirmed 2026-08-28: all three keys present with `InstallPath`, `DisplayVersion`, `DataRoot`, `DataParent`.
  - [x] 5c. Confirm that installed-version data entries were added
    - Confirmed: `DataRoot` and `DataParent` values present on all three keys; Seamly2D additionally carries the three `DesktopShortcut*` entries.
- [ ] 6. Check the apps — **needs a human at the keyboard; not run in this pass.**
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
- [x] 7. Check the logs for additional errors
  - Checked 2026-08-28. `msiexec` install log (`%TEMP%\seamly_install_1b.log`): "Installation completed successfully", status 0, no errors. `%LOCALAPPDATA%\Seamly\Seamly2D\logs\seamly2d-pid19244.log`: INFO only, no errors or warnings. SeamlyMe and SeamlyLayout wrote no log file under `%LOCALAPPDATA%\Seamly\<AppName>\` — not investigated further in this pass; see Task 9 below.
  - **Finding:** a stale `%APPDATA%\Unknown Organization.ini` (dated 2026-08-21, predates this test run) was found directly under `%APPDATA%`, left over from an earlier run before this test case existed. It was not written by any step in this pass — this run's `qt6_common.ini` correctly used the `Seamly` organization name. `test_reset_environment.ps1` does not remove it because it only clears `%APPDATA%\Seamly`, not stray files at the `%APPDATA%` root. See Task 10 below.

## C. New Tasks From This Pass (2026-08-28)

- [ ] 8. Decide whether Verification Suite item 1 should launch all three apps (Seamly2D, SeamlyMe, SeamlyLayout), not just Seamly2D, since item 4a checks all three `%LOCALAPPDATA%\Seamly\<AppName>\` directories. Update item 1's wording once decided.
--> item 1 should only run seamly2d --> this will install the data files --> no need to run seamlyme or seamlylayout at this point
- [ ] 9. Confirm whether SeamlyMe and SeamlyLayout are expected to write a log file under `%LOCALAPPDATA%\Seamly\<AppName>\logs\` like Seamly2D does. Neither wrote one in this pass. If expected, treat the absence as a defect; if not, update this doc's item 7 to say so.
--> the installation writes the install log to seamly2d --> this is correct
- [x] 10. Fix or extend `test_reset_environment.ps1` to remove `%APPDATA%\Unknown Organization.ini` (and any `%APPDATA%\Unknown Organization\` folder) between test runs, so a leftover from the empty-organization-name defect cannot be mistaken for a fresh reproduction in a later pass.
