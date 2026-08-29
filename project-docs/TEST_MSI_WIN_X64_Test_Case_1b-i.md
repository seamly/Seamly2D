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

- [x] 0. Relaunch this shell elevated (Administrator) before any step below. Verified elevated (BUILTIN\Administrators enabled in token).
- [x] 1a. Uninstall Seamly (any and all versions detected) using `/test_reset_environment.ps1`. Prior install found: Seamly 26.8.39948. Script ran clean, no errors.
  - [x] 1a-i. Confirm that %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly, desktop shortcuts, and registry keys have been removed. All confirmed absent via `Test-Path`. Reset also removed a leftover `AppData\Roaming\Unknown Organization.ini` (known defect, see top of file).
- [x] 1b. Install Seamly apps using `scripts\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`. Exit code 0.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

## B. Verification Suite

Run this suite after every test case in section A.

- [x] 1. Run Seamly2D then close Seamly2D to install the user directories. Launched, ran 8s, closed.
- [x] 2. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`). Present, contains `seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe`, `pdftops.exe`, `QtWebEngineProcess.exe`, `vc_redist.x64.exe`.
- [x] 3. Check user-data location (default `C:\Users\<user>\Documents\SeamlyData\`), subdirectories, and files:
  - [x] 3a. No duplicate directories. Confirmed single tree, no old-name `Documents\Seamly` left over.
  - [x] 3b. Subdirectories `backups`, `bodyscans`, `images`, `label templates`, `layouts`,  `measurements\individual`, `measurements\multisize`, `patterns`, and `templates` are created at the correct level below `%DATAROOT%`. All nine present at the correct level.
  - [x] 3c. if upgrading from previous non-SeamlyLayout version then: **N/A — Case 1 fresh install.**
    - [x] 3c-i. confirm `%DATAROOT%\seamly2d.zip` exists — **N/A, fresh install.**
    - [x] 3c-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories — **N/A, fresh install.**
  - [x] 3d. check that %DATADIR\patterns\male_shirt.sm2d exists. Confirmed, plus 6 other sample patterns.
  - [x] 3e. check that %DATADIR\measurements\individual\male_chest_102cm.smis exists. Confirmed, plus 2 other sample measurement files.
- [x] 4. Check the user application directories:
  - [x] 4a. `%LOCALAPPDATA%\Seamly\<AppName>\` directories exist for Seamly2D, SeamlyMe, and SeamlyLayout. All three present, but only after launching each app directly — see error E1 below.
  - [x] 4b. `%APPDATA%\Seamly\qt6_common.ini` file exists.
    - [x] 4b-i. Confirm file holds one entry, `dataRoot=C:/Users/susan/Documents/SeamlyData`, matching `%DATAROOT%`
- [x] 5. Check the registry keys:
  - [x] 5a. If not a fresh install then confirm old-version entries were removed. **N/A — Case 1 fresh install.**
  - [x] 5b. Confirm that the installed-version program entries were added, under `HKLM\SOFTWARE\Seamly\Seamly2D`, `HKLM\SOFTWARE\Seamly\SeamlyMe`, and `HKLM\SOFTWARE\Seamly\SeamlyLayout`. All three keys present with `InstallPath` and `DisplayVersion`.
  - [x] 5c. Confirm that installed-version data entries were added. All three keys carry matching `DataRoot` and `DataParent` values; `Seamly2D` also carries the three `DesktopShortcut*` flags.
- [ ] 6. Check the apps — **needs a human at the keyboard**
  - [ ] 6a. Check Seamly2D and SeamlyMe
    - [x] 6a-i. Open `%DATADIR%\patterns\male_shirt.sm2d` pattern file with `%DATADIR%\measurements\individual\male_chest_102cm.smis` individual measurement file.
    - [x] 6a-ii. Select 'file open' -- the dialog should open in the `%DATADIR\patterns` directory.
    - [x] 6a-iii. Run SeamlyMe from within Seamly2D
    - [x] 6a-iv. Select 'Edit Current' from the Measurements menu in Seamly2D - the %DATADIR\measurements\individual\male_chest_102cm.smis file should open
    - [x] 6a-v. Select 'File Open Individual' - the dialog should open in the `%DATADIR\measurements\individual` directory
    - [x] 6a-vi. Select 'File Open Multisize' - the dialog should open in the `%DATADIR\measurements\multisize` directory
    -->Was opening to whatever Seamly folder a prior native dialog last visited (Windows' native picker keeps one process-wide "last visited folder" and overrides the app-supplied start folder). Fixed by forcing `QFileDialog::DontUseNativeDialog` in `TMainWindow::Open()` (`src/app/seamlyme/tmainwindow.cpp:3059`). Tracked as Seamly2D.2.2 in `project-docs\TODO_SEAMLY2D.md`. **Re-test needed to confirm.**
  - [x] 6b. Check SeamlyLayout
    - [x] 6b-i. Run SeamlyLayout from within Seamly2D
    - [x] 6b-ii. Confirm that the current pattern's `Piece mode` data was passed to SeamlyLayout as a stringified svg document (not as a svg file)
    - [x] 6b-iv. Close SeamlyLayout
    -->Pass. However, focus returned to Seamly2D's 'Layout Mode' that was superceded; should return focus to 'Piece Mode'. Tracked as Seamly2D.3.1 in `project-docs\TODO_SEAMLY2D.md`.
  - [x] 6c. Close Seamly2D
- [x] 7. Check the logs in `%LOCALAPPDATA%\Seamly\Seamly2D\logs\` for additional errors.SeamlyMe and SeamlyLayout have no `logs\` folder.
- [ ] 8. Check Desktop shortcuts for all three apps in `C:\Users\Public\Desktop` (default settings, `SEAMLYDESKTOPSHORTCUTS` on).

Notes
