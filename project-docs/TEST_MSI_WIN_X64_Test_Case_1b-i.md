# TEST_MSI_WIN_X64_Test_Case_1b-i.md

Test plan for the Windows x64 Seamly MSI. Covers `packaging/windows/smsi.wxs`.

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`; default is `C:\Program Files\SeamlyApps`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`; default is `C:\Users\<user>\Documents\SeamlyData`.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

Known defect to watch for: `MainWindow::exportPiecesToSeamlyLayout()` (`mainwindow.cpp`) writes the pattern's pieces to a `.pieces.svg` file next to the pattern file and launches SeamlyLayout with that file path as an argument. This contradicts the intended design: the piece-mode SVG should be passed to SeamlyLayout as a stringified SVG document, not as a file. Check for this on every verification pass until fixed. Tasks filed: Seamly2D.5, Layout.9.

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Fresh install | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Fresh install

- [x] 0. Relaunch this shell elevated (Administrator) before any step below.
- [x] 1a. Uninstall Seamly (any and all versions detected) using `packaging\windows\test_reset_environment.ps1`.
  - [x] 1a-i. Confirm that the %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly\Seamly2D, AppData\Local\Seamly\SeamlyMe, AppData\Local\Seamly\SeamlyLayout, desktop shortcuts, and registry keys have been removed. PASS except `%LOCALAPPDATA%\SeamlyLayout` (no `\Seamly\` prefix) survives reset — known, Layout.10.
- [x] 1b. Install Seamly apps using `packaging\windows\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`. PASS, msiexec exit 0. Build 26.9.1.778.

## B. Verification Suite

Run this suite after every test case in section A.

- [x] 0. Check the directories and files
  - [x] 0a. Confirm these directories and files exist: PASS pre-first-run (exe's, inis, logs/cache/DataRoot subtree not yet created); PASS post-first-run (see 2a-2c).
    %PROGRAMDIR%\SeamlyApps
    |  |_seamly2d.exe
    |  |_seamlylayout.exe
    |  |_seamlyme.exe
    %LOCALAPPDATA%\Seamly
    |_qt6_common.ini
    |_Seamly2D
    |  |_qt6_seamly2d.ini
    |_SeamlyLayout
    |  |_preferences
    |  |  |_default_preferences.json
    |  |_settings
    |  |  |_default_settings.json
    |  |_qt6_seamlyLayout.ini
    |_SeamlyMe
    |  |_qt6_seamlyme.ini
    %DATAROOT%\SeamlyData
    |_backups
    |_bodyscans
    |_images
    |_layouts
    |_measurements
    |_measurements\individual
    |  |_male_chest_102cm.smis
    |_measurements\multisize
    |_patterns
    |  |_male_shirt.sm2d
    |_templates
  - [x] 0b. Check the contents of the .ini files: PASS.
    - [x] 0b-i. qt6_common.ini should contain:
      - [x] 0b-i1. PASS, all 5 keys correct (`%DATAROOTT%` in this line is a doc typo, not the actual key).
      - [x] 0b-i2. PASS pre-first-run (`pending`); flips to `shown` after Seamly2D's first run (2a-ii).
    - [x] 0b-ii. qt6_seamly2d.ini should contain: PASS, all 6 keys correct.
    - [x] 0b-iii. qt6_seamlyme.ini should be empty: PASS.
    - [x] ob-iv. qt6_seamlylayout.ini should contain: PASS, all 11 keys correct (`%DATAROOTROOT%` in this line is a doc typo).
  - [x] 0c. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`) contains `seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe`, `pdftops.exe`, `QtWebEngineProcess.exe`, `vc_redist.x64.exe`. PASS.
  - [x] 0d. Confirm no duplicate directories. PASS.
  - [x] 0e. if upgrading from previous non-SeamlyLayout version then: N/A, fresh install (Case 1).
    - [ ] 0e-i. confirm `%DATAROOT%\seamly2d.zip` exists.
    - [ ] 0e-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories.
- [x] 1. Check the registry keys:
  - [x] 1a. If not a fresh install then confirm old-version entries were removed. N/A, fresh install.
  - [x] 1b. Confirm that the installed-version program entries were added for each app under `HKLM\SOFTWARE\Seamly\<application>`, each with matching `InstallPath` and `DisplayVersion`. PASS, all three keys, DisplayVersion 26.9.1.778.
  - [x] 1c. Confirm that installed-version data entries were added with matching `DataRoot` and `DataParent` values; `Seamly2D` also carries the three `DesktopShortcut*` flags (this should be fixed in the future; tasks filed: SeamlyMe.3, Layout.7). PASS.
- [ ] 2. Check apps - **needs a human at the keyboard for most steps**
  - [x] 2a. Run Seamly2D
    - [x] 2a-i. first run scripted: the "Seamly data moved" notice appears once, closes, then the main window appears. PASS: only the main window title (`Seamly2D - untitled.sm2d`) received WM_CLOSE; scripted run cannot see if a modal notice showed and self-closed before the 12s grace period, but `firstRunDataNotice` flipped pending->shown and exit code was 0.
    - [x] 2a-ii. check if `qt6_common.ini` contains  "[notices] firstRunDataNotice=shown"; PASS.
    - [ ] 2a-iii. Select 'file open' -- the dialog should open in the `%DATAROOT\patterns` directory. PENDING HUMAN.
    - [ ] 2a-iv. Open `%DATAROOT%\patterns\male_shirt.sm2d` pattern file with `%DATAROOT%\measurements\individual\male_chest_102cm.smis` individual measurement file. PENDING HUMAN.
    - [x] 2a-v. Check if directory exists: `%LOCALAPPDATA%\Seamly\Seamly2D\logs` PASS, log clean (no error/warn/fatal/critical lines).
  - [ ] 2b. Check SeamlyMe from within Seamly2D
  - [ ] 2b-i. Select 'File Open Individual' - the dialog should open in the `%DATAROOT\measurements\individual` directory. PENDING HUMAN.
    - [ ] 2b-ii. Select 'File Open Multisize' - the dialog should open in the `%DATAROOT\measurements\multisize` directory. PENDING HUMAN.
    - [ ] 2b-iii. Select 'File Open Templates' - the dialog should open in the `%DATAROOT\templates` directory. PENDING HUMAN.
    - [ ] 2b-iv. Select 'Edit Current' from the Measurements menu - the `%DATAROOT%\measurements\individual\male_chest_102cm.smis` file should open. PENDING HUMAN.
    - [x] 2b-v. Check if directory exists: `%LOCALAPPDATA%\SeamlyMe\logs` PASS (standalone SeamlyMe run, not from within Seamly2D) — first live confirmation of the SeamlyMe.5 fix: `%LOCALAPPDATA%\Seamly\SeamlyMe\logs\` now exists with a clean log.
    - [ ] 2b-vi. Close SeamlyMe, returning focus to Seamly2D. PENDING HUMAN (requires 2b launched from within Seamly2D).
  - [ ] 2c. Run SeamlyLayout from within Seamly2D.
    - [ ] 2c-i. Visually confirm that the current pattern's `Piece mode` data is opened in the left canvas. PENDING HUMAN.
    - [x] 2c-ii. Check if `MainWindow::exportPiecesToSeamlyLayout()` passes 'piece mode' data to SeamlyLayout as a stringified SVG document in a variable, not as a svg file from harddrive. FAIL (static check): still writes `.pieces.svg` next to the pattern file and launches SeamlyLayout with that path (`mainwindow.cpp:4153-4168`). Already filed: Seamly2D.5, Layout.9.
    - [x] 2c-iii. Check if directories exist: `%DATAROOT%\SeamlyLayout\cache`, `%DATAROOT%\SeamlyLayout\logs` (standalone SeamlyLayout run): `preferences\default_preferences.json` and `settings\default_settings.json` and `cache` created; `logs` NOT created — already filed, Layout.10.
    - [ ] 2c-iv. Close SeamlyLayout, returning focus to Seamly2D. PENDING HUMAN.
  - [ ] 2d. Close Seamly2D. PENDING HUMAN (walkthrough sequence, not the scripted standalone close already done in 2a-i).
- [x] 3. Check if `%LOCALAPPDATA%\SeamlyLayout\output` directory was created. If exists add a task to stop creating the `%LOCALAPPDATA%\SeamlyLayout\` directory and its `output` subdirectory that stores log files, and start creating the `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs` directory to store SeamlyLayout log files (similar to the `%LOCALAPPDATA\Seamly\Seamly2D\logs` directory) CONFIRMED: exists with a fresh log from this pass's run. Already filed, Layout.10.
- [x] 4. Check Desktop shortcuts `Seamly2D.lnk`, `SeamlyMe.lnk`, `SeamlyLayout.lnk` for all three apps in `C:\Users\Public\Desktop` (default settings, `SEAMLYDESKTOPSHORTCUTS` on). PASS, all three present.
- [x] 5. Check the logs in `%LOCALAPPDATA%\Seamly\Seamly2D\logs\` for additional errors. PASS, none found.
