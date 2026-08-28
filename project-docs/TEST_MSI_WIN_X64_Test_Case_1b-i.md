# TEST_MSI_WIN_X64_Test_Case_1b-i.md

Test plan for the Windows x64 Seamly MSI. Covers `scripts/packaging/windows/smsi.wxs`.

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

- [x] 1a. Uninstall Seamly (any and all versions detected) using `/test_reset_environment.ps1`
  - [x] 1a-i. Confirm that %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly, desktop shortcuts, and registry keys have been removed
    - Ran `test_reset_environment.ps1` elevated. Verified all six locations absent:
      `C:\Program Files\SeamlyApps`, `Documents\SeamlyData`, `%LOCALAPPDATA%\Seamly`,
      `%APPDATA%\Seamly`, `HKLM\SOFTWARE\Seamly`, `HKCU\Software\Seamly`. Confirmed by
      `test_msi_install.ps1 -Phase Baseline` passing.
- [x] 1b. Install Seamly apps using `scripts\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`
  - Not re-run in the 2026-08-28 pass below — the machine was already in this
    post-install state from the run recorded above. Reconfirmed live: `HKLM\SOFTWARE\Seamly\Seamly2D`
    reads `InstallPath = C:\Program Files\SeamlyApps\`, `DataRoot = C:\Users\susan\Documents\SeamlyData\`,
    `DisplayVersion = 26.8.24.982`, matching a default `/quiet` install with no properties.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

## B. Verification Suite

Run this suite after every test case in section A.

- [x] 1. Run Seamly2D then close Seamly2D to install the user directories
  - Not re-launched in the 2026-08-28 re-check pass — `%LOCALAPPDATA%\Seamly\Seamly2D\logs\seamly2d-pid2792.log`
    is evidence of the prior run this Status/checkbox record already covers.
- [x] 2. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`)
  - Reconfirmed live 2026-08-28: exists, populated (`generic`, `iconengines`, `imageformats`, `labels`, `multimedia`, …).
- [x] 3. Check user-data location (default `C:\Users\<user>\Documents\SeamlyData\`), subdirectories, and files:
  - [x] 3a. No duplicate directories
    - Reconfirmed live 2026-08-28: single `Documents\SeamlyData` tree, no duplicate.
  - [x] 3b. Subdirectories `backups`, `bodyscans`, `images`, `label templates`, `layouts`,  `measurements\individual`, `measurements\multisize`, `patterns`, and `templates` are created at the correct level below `%DATAROOT%`
    - Reconfirmed live 2026-08-28: all nine present, `measurements\individual` and
      `measurements\multisize` correctly nested one level below `measurements`.
  - [x] 3c. if upgrading from previous non-SeamlyLayout version then: **N/A — Case 1 fresh install.**
    - [x] 3c-i. confirm `%DATAROOT%\seamly2d.zip` exists — **N/A, fresh install.**
    - [x] 3c-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories — **N/A, fresh install.**
- [x] 4. Check the user application directories:
  - [x] 4a. `%LOCALAPPDATA%\Seamly\<AppName>\` directories exist for Seamly2D, SeamlyMe, and SeamlyLayout.
    - Reconfirmed live 2026-08-28: all three exist.
  - [x] 4b. `%APPDATA%\Seamly\qt6_common.ini` file exists
    - Reconfirmed live 2026-08-28: exists.
    - [x] 4b-i. Confirm all paths in qt6_common.ini start with `%DATAROOT%` value.
      - Reconfirmed live 2026-08-28: `[paths]` / `dataRoot=C:/Users/susan/Documents/SeamlyData` — matches `%DATAROOT%` exactly.
- [x] 5. Check the registry keys:
  - [x] 5a. If not a fresh install then confirm old-version entries were removed. **N/A — Case 1 fresh install.**
  - [x] 5b. Confirm that the installed-version program entries were added, under `HKLM\SOFTWARE\Seamly\Seamly2D`, `HKLM\SOFTWARE\Seamly\SeamlyMe`, and `HKLM\SOFTWARE\Seamly\SeamlyLayout`
    - Reconfirmed live 2026-08-28: all three keys present, `InstallPath`/`DisplayVersion` correct on each.
  - [x] 5c. Confirm that installed-version data entries were added
    - Reconfirmed live 2026-08-28: `DataRoot`/`DataParent` correct and identical across all three apps' keys.
- [ ] 6. Check the apps — **needs a human at the keyboard; not run in this pass.**
  - [ ] 6a. Check Seamly2D and SeamlyMe
    - [ ] 6a-i. Open `%PROGRAMDIR%\samples\patterns\male_shirt.sm2d` pattern file with `%PROGRAMDIR%\samples\measurements\individual\male_chest_102cm.smis` individual measurement file.
      - **Still blocked, reconfirmed 2026-08-28:** `%PROGRAMDIR%\samples\measurements\individual\male_shirt.smis`
        exists; `male_chest_102cm.smis` does not. The installed MSI predates the rename. See new task
        `InstWinX64.15` below.
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
  - Reconfirmed live 2026-08-28: `seamly2d-pid2792.log` clean, no `warn`/`error`/`fail` lines.
    SeamlyMe and SeamlyLayout still have no `logs\` directory — expected, unchanged from the prior note.
