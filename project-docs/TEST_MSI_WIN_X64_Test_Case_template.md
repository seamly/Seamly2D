# TEST_MSI_WIN_X64_Test_Case_1b-i.md

Test plan for the Windows x64 Seamly MSI. Covers `packaging/windows/smsi.wxs`.

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`; default is `C:\Program Files\SeamlyApps`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`; default is `C:\Users\<user>\SeamlyData`.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

Known defect to watch for: `MainWindow::exportPiecesToSeamlyLayout()` (`mainwindow.cpp`) writes the pattern's pieces to a `.pieces.svg` file next to the pattern file and launches SeamlyLayout with that file path as an argument. This contradicts the intended design: the piece-mode SVG should be passed to SeamlyLayout as a stringified SVG document, not as a file. Found during Case 1 item 6b-ii. Check for this on every verification pass until fixed. STILL PRESENT on the 2026-08-31 notice-build pass (`mainwindow.cpp:4153`). add a task to fix this problem. 

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Fresh install | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Fresh install

- [ ] 0. Relaunch this shell elevated (Administrator) before any step below. 
- [ ] 1a. Uninstall Seamly (any and all versions detected) using `packaging\windows\test_reset_environment.ps1`.
  - [ ] 1a-i. Confirm that the %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly\Seamly2D, AppData\Local\Seamly\SeamlyMe, AppData\Local\Seamly\SeamlyLayout, desktop shortcuts, and registry keys have been removed.
- [ ] 1b. Install Seamly apps using `packaging\windows\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`.

## B. Verification Suite

Run this suite after every test case in section A.
- [ ] 0. Check the directories and files:
  - [ ] 0.i Confirm these directories and files exist:
    %LOCALAPPDATA%\Seamly
    |_qt6_common.ini
    |_Seamly2D
    |  |_logs
    |  |_qt6_seamly2d.ini
    |_SeamlyLayout
    |  |_logs
    |  |_qt6_seamlyLayout.ini
    |_SeamlyMe
    |  |_logs
    |  |_qt6_seamlyme.ini
- [ ] 1. Run the apps: 2026-08-31 pass, scripted launch and close.
  - [ ] 1a. run Seamly2D then close Seamly2D 
  - [ ] 1b. run SeamlyMe then close SeamlyMe.
  - [ ] 1c. run SeamlyLayout then close SeamlyLayout.
- [ ] 2. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`) contains `seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe`, `pdftops.exe`, `QtWebEngineProcess.exe`, `vc_redist.x64.exe`.
- [ ] 3. Check user-data location (default `C:\Users\<user>\Documents\SeamlyData\`), subdirectories, and files:
  - [ ] 3a. Subdirectories `backups`, `bodyscans`, `images`, `label templates`, `layouts`,  `measurements\individual`, `measurements\multisize`, `patterns`, and `templates` are created at the correct level below `%DATAROOT%`.
  - [ ] 3b. No duplicate directories.
  - [ ] 3c. if upgrading from previous non-SeamlyLayout version then: (fresh install).
    - [ ] 3c-i. confirm `%DATAROOT%\seamly2d.zip` exists.
    - [ ] 3c-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories.
  - [ ] 3d. check that %DATADIR\patterns\male_shirt.sm2d exists.
  - [ ] 3e. check that %DATADIR\measurements\individual\male_chest_102cm.smis exists.
- [ ] 4. Check the user application directories:
  - [ ] 4a. `%LOCALAPPDATA%\Seamly\<AppName>\` exists for each app.
  - [ ] 4b. `%LOCALAPPDATA%\Seamly\qt6_common.ini` file exists.
    - [ ] 4b-i. Confirm `%LOCALAPPDATA%\Seamly\qt6_common.ini` file holds 5 entries: `dataRoot`, `individual_size_measurements`, `multi_size_measurements`, `templates`, `bodyscans`; all five values are paths under `%DATAROOT%`.
    - [ ] 4b-ii. Confirm the one-shot first-run data notice : at install time `qt6_common.ini` holds `[notices] firstRunDataNotice=pending`; the first app run shows one popup about the data locations and backups, then the value reads `shown`; no later app run repeats the popup.
- [ ] 5. Check the registry keys:
  - [ ] 5a. If not a fresh install then confirm old-version entries were removed.
  - [ ] 5b. Confirm that the installed-version program entries were added for each app under `HKLM\SOFTWARE\Seamly\<application>`, each with matching `InstallPath` and `DisplayVersion`.
  - [ ] 5c. Confirm that installed-version data entries were added with matching `DataRoot` and `DataParent` values; `Seamly2D` also carries the three `DesktopShortcut*` flags (this should be fixed in the future; add a task to fix this)
- [ ] 6. Check the apps — **needs a human at the keyboard**
  - [ ] 6a. Check Seamly2D and SeamlyMe
    - [ ] 6a-i. Run Seamly2D
    - [ ] 6a-ii. Select 'file open' -- the dialog should open in the `%DATADIR\patterns` directory.
    - [ ] 6a-iii. Open `%DATADIR%\patterns\male_shirt.sm2d` pattern file with `%DATADIR%\measurements\individual\male_chest_102cm.smis` individual measurement file.
    - [ ] 6a-iv. Run SeamlyMe from within Seamly2D
    - [ ] 6a-v. Select 'Edit Current' from the Measurements menu in Seamly2D - the `%DATADIR\measurements\individual\male_chest_102cm.smis` file should open.
    - [ ] 6a-vi. Select 'File Open Individual' - the dialog should open in the `%DATADIR\measurements\individual` directory.
    - [ ] 6a-vii. Select 'File Open Multisize' - the dialog should open in the `%DATADIR\measurements\multisize` directory.
    - [ ] 6a-viii. Close SeamlyMe, returning focus to Seamly2D.
  - [ ] 6b. Check SeamlyLayout
    - [ ] 6b-i. Run SeamlyLayout from within Seamly2D.
    - [ ] 6b-ii. Confirm that the current pattern's `Piece mode` data is opened in the left canvas.
    - [ ] 6b-iii. Confirm that the 'piece mode' data was passed to SeamlyLayout as a stringified svg document (not as a svg file). Claude: check if `MainWindow::exportPiecesToSeamlyLayout()` writes `<pattern-basename>.pieces.svg` from the 'piece mode' data and SeamlyLayout reads this svg file, isn't passed as a stringified SVG document; Add a task to fix this.
    - [ ] 6b-iv. Close SeamlyLayout, returning focus to Seamly2D.
  - [ ] 6c. Close Seamly2D.
- [ ] 7. Check Application Preferences and Settings and files
  - [ ] 7a. confirm Seamly2D's preferences and settings file `%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini`:
    - [ ] 7a-i. confirm this file exists at install time, before any app runs.
    - [ ] 7a-ii. confirm this file contains a 'paths' section. Confirmed.
    - [ ] 7a-iii. confirm this file contains keys: [`backups`, `bodyscans`,`images`, `labels`, `label templates`, `layouts`, `measurments`, `individual measurements`, `multisize measurements`, `patterns`, `layouts`, `templates`, `seamlyLayoutApp`].
    - [ ] 7a-iv. confirm the 'paths' keys all begin with the `%DATADIR%` value, except `seamlyLayoutApp`, which points at `%PROGRAMDIR%/SeamlyLayout.exe`.
    - [ ] 7a-v. Re-verify the `bodyscans` UI-row fix: the `bodyscans` key in `qt6_common.ini` is now installer-seeded (see 4b-i); to verify the UI row itself, change it in Preferences > Paths and confirm the changed value lands in `qt6_common.ini`. **Needs a human at the keyboard.**
  - [ ] 7b. check SeamlyMe preferences and settings file exists: `%LOCALAPPDATA%\Seamly\SeamlyMe\qt6_seamlyme.ini`.
  - [ ] 7c. check the installer created the  `%LOCALAPPDATA%\Seamly\SeamlyLayout\preferences\` and `settings\` directories at install time. `preferences\default_preferences.json` is NOT created at first launch any more; the app creates it on demand when the user resets to defaults.
  - [ ] 7d. after SeamlyLayout has run once, check the default settings file exists: `%LOCALAPPDATA%\Seamly\SeamlyLayout\settings\default_settings.json` (copied at runtime from the packaged `settings\` folder).
  - [ ] 7e. check SeamlyLayout preferences file `%LOCALAPPDATA%\Seamly\SeamlyLayout\qt6_seamlylayout.ini`:
    - [ ] 7e-i. confirm this file exists at install time, before any app runs (installer-seeded, SettingsFiles.3).
    - [ ] 7e-ii. confirm its `[General]` section holds all 11 keys: `input_directory`, `layout_directory`, `preferences_directory`, `settings_directory`, `settings_file`, `preferences_file`, `dxf_viewer_path`, `pdf_viewer_path`, `png_viewer_path`, `projector_path`, `data_root`.
    - [ ] 7e-iii. confirm `input_directory`, `layout_directory` = `%DATADIR%/layouts`; `data_root` = `%DATADIR%`; `preferences_directory`, `settings_directory`, `settings_file`, `preferences_file` sit under `%LOCALAPPDATA%\Seamly\SeamlyLayout\`; `dxf_viewer_path` = `https://sharecad.org`; `projector_path` = `https://patternprojector.com`.
  - [ ] 7f. Check if `%LOCALAPPDATA%\SeamlyLayout\output` directory was created. If exists add a task to stop creating the `%LOCALAPPDATA%\SeamlyLayout\` and its `output` subdirectory that stores log files, and start creating the `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs` directory to store SeamlyLayout log files (similar to the `%LOCALAPPDATA\Seamly\Seamly2D\logs` directory)
- [ ] 8. Check the logs in `%LOCALAPPDATA%\Seamly\Seamly2D\logs\` for additional errors.
- [ ] 9. Check Desktop shortcuts `Seamly2D.lnk`, `SeamlyMe.lnk`, `SeamlyLayout.lnk` for all three apps in `C:\Users\Public\Desktop` (default settings, `SEAMLYDESKTOPSHORTCUTS` on).
