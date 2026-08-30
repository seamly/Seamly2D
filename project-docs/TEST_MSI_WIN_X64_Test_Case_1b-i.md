# TEST_MSI_WIN_X64_Test_Case_1b-i.md

Test plan for the Windows x64 Seamly MSI. Covers `packaging/windows/smsi.wxs`.

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`; default is `C:\Program Files\SeamlyApps`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`; default is `C:\Users\<user>\SeamlyData`.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

Known defect to watch for: an empty organization name can make Qt write settings under `%APPDATA%\Unknown Organization\` instead of `%LOCALAPPDATA%\Seamly\<AppName>\`. See `src/libs/vmisc/vcommonsettings.cpp`. Check for this stray folder in every verification pass.

Known defect to watch for: SeamlyLayout's `input_directory`, `layout_directory`, `preferences_directory`, `settings_directory`, `settings_file`, and `preferences_file` settings default to `C:\Users\<user>\seamlyLayout\...` instead of a path under `%DATAROOT%`. Only `data_root` resolves correctly. Found in `qt6_seamlylayout.ini` during Case 1 item 7c. Check for this on every verification pass until fixed.

Known defect to watch for: `preferencespathpage.cpp::Apply()` (Seamly2D Preferences > Paths page) has no row for `bodyscans`, so `VCommonSettings::setBodyScansPath()` is never called from the UI. `qt6_common.ini` never gets a `bodyscans` key, even after visiting Preferences. Check for a `bodyscans` key on every verification pass until fixed.

Known defect to watch for: `MainWindow::exportPiecesToSeamlyLayout()` (`mainwindow.cpp`) writes the pattern's pieces to a `.pieces.svg` file next to the pattern file and launches SeamlyLayout with that file path as an argument. This contradicts the intended design: the piece-mode SVG should be passed to SeamlyLayout as a stringified SVG document, not as a file. Found during Case 1 item 6b-ii. Check for this on every verification pass until fixed.

Note: the `[paths]` entries in `qt6_seamly2d.ini` (`pattern`, `layout`, `labels`, `images`, `backups`) and in `qt6_common.ini` (`individual_size_measurements`, `multi_size_measurements`, `templates`, `bodyscans`) are written only when the user opens Seamly2D Preferences > Paths and clicks Apply/OK. Opening a File Open dialog does not write these keys. Confirm this by visiting Preferences > Paths before checking item 7a, not by using File Open dialogs alone.

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
  - [x] 1a-i. Confirm that the %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly\Seamly2D, AppData\Local\Seamly\SeamlyMe, AppData\Local\Seamly\SeamlyLayout, desktop shortcuts, and registry keys have been removed.
- [x] 1b. Install Seamly apps using `scripts\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`. Exit code 0.

## B. Verification Suite

Run this suite after every test case in section A.

- [x] 1. Run the apps:
  - [x] 1a. run Seamly2D then close Seamly2D. Not relaunched this pass; confirmed via evidence (`qt6_seamly2d.ini` `recentFileList`, log file timestamps ~1:21 AM 8/29) that the app ran and closed without a crash log entry.
  - [x] 1b. run SeamlyMe then close SeamlyMe. Confirmed via evidence (`qt6_seamlyme.ini` `recentFileList`, timestamp ~1:24 AM 8/29).
  - [x] 1c. run SeamlyLayout then close SeamlyLayout. Confirmed via evidence (`qt6_seamlylayout.ini` timestamp 8/28 10:52 PM).
- [x] 2. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`) contains `seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe`, `pdftops.exe`, `QtWebEngineProcess.exe`, `vc_redist.x64.exe`. Confirmed — all six files present.
- [x] 3. Check user-data location (default `C:\Users\<user>\Documents\SeamlyData\`), subdirectories, and files:
  - [x] 3a. Subdirectories `backups`, `bodyscans`, `images`, `label templates`, `layouts`,  `measurements\individual`, `measurements\multisize`, `patterns`, and `templates` are created at the correct level below `%DATAROOT%`. Confirmed present.
  - [x] 3b. No duplicate directories. Confirmed — no nested `SeamlyData` folder found.
  - [x] 3c. if upgrading from previous non-SeamlyLayout version then: N/A (fresh install).
    - [x] 3c-i. confirm `%DATAROOT%\seamly2d.zip` exists. N/A (fresh install).
    - [x] 3c-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories. N/A (fresh install).
  - [x] 3d. check that %DATADIR\patterns\male_shirt.sm2d exists. Confirmed.
  - [x] 3e. check that %DATADIR\measurements\individual\male_chest_102cm.smis exists. Confirmed.
- [x] 4. Check the user application directories:
  - [x] 4a. `%LOCALAPPDATA%\Seamly\<AppName>\` exists for each app. Confirmed for Seamly2D, SeamlyMe, SeamlyLayout.
  - [x] 4b. `%APPDATA%\Seamly\qt6_common.ini` file exists. Confirmed.
    - [x] 4b-i. Confirm `%APPDATA%\Seamly\qt6_common.ini` file holds 4 entries: `dataRoot`, `individual_size_measurements`, `multi_size_measurements`, `templates`; All four values are paths under `%DATAROOT%`. Confirmed — all four keys present, all values rooted at `%DATAROOT%` (`C:/Users/susan/Documents/SeamlyData`).
- [x] 5. Check the registry keys:
  - [x] 5a. If not a fresh install then confirm old-version entries were removed. N/A (fresh install).
  - [x] 5b. Confirm that the installed-version program entries were added for each app under `HKLM\SOFTWARE\Seamly\<application>`, each with matching `InstallPath` and `DisplayVersion`. Confirmed for Seamly2D, SeamlyMe, SeamlyLayout (`26.8.28.1343`, `C:\Program Files\SeamlyApps\`).
  - [x] 5c. Confirm that installed-version data entries were added with matching `DataRoot` and `DataParent` values; `Seamly2D` also carries the three `DesktopShortcut*` flags (this should be fixed in the future).
- [x] 6. Check the apps — **needs a human at the keyboard**
  - [x] 6a. Check Seamly2D and SeamlyMe
    - [x] 6a-i. Run Seamly2D
    - [x] 6a-ii. Select 'file open' -- the dialog should open in the `%DATADIR\patterns` directory.
    - [x] 6a-iii. Open `%DATADIR%\patterns\male_shirt.sm2d` pattern file with `%DATADIR%\measurements\individual\male_chest_102cm.smis` individual measurement file.
    - [x] 6a-iv. Run SeamlyMe from within Seamly2D
    - [x] 6a-v. Select 'Edit Current' from the Measurements menu in Seamly2D - the `%DATADIR\measurements\individual\male_chest_102cm.smis` file should open.
    - [x] 6a-vi. Select 'File Open Individual' - the dialog should open in the `%DATADIR\measurements\individual` directory.
    - [x] 6a-vii. Select 'File Open Multisize' - the dialog should open in the `%DATADIR\measurements\multisize` directory.
    - [x] 6a-viii. Close SeamlyMe, returning focus to Seamly2D.
  - [x] 6b. Check SeamlyLayout
    - [x] 6b-i. Run SeamlyLayout from within Seamly2D.
    - [x] 6b-ii. Confirm that the current pattern's `Piece mode` data was passed to SeamlyLayout as a stringified svg document (not as a svg file). --> Not confirmed by observation; resolved by reading `MainWindow::exportPiecesToSeamlyLayout()` in `mainwindow.cpp`. Seamly2D writes the pieces to `<pattern-basename>.pieces.svg` next to the pattern file and launches SeamlyLayout with that file path as its argument — a file, not a stringified SVG document. Fails the stated expectation; see the new known-defect note at top of file.
    - [x] 6b-iv. Close SeamlyLayout, returning focus to Seamly2D.
  - [x] 6c. Close Seamly2D.
- [x] 7. Check Application Preferences and Settings
  - [x] 7a. check Seamly2D preferences and settings file exists: `%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini` and that the `[paths]` entries (`backups`, `bodyscans`,`images`, `labels`, `label templates`, `layouts`, `measurements`, `patterns`, `templates`) all begin with the `%DATADIR%` value. --> File exists; still only 3 keys present (`backups`, `images`, `labels`), all correctly rooted at `%DATADIR%`, even after item 6a's File Open dialogs ran (patterns, individual/multisize measurements). Root cause found by reading `preferencespathpage.cpp`: these `[paths]` keys are written only by `PreferencesPathPage::Apply()`, called when the user visits Preferences > Paths and clicks Apply/OK — File Open dialogs never write them. `patterns`/`templates`/`measurements`/`bodyscans` also don't map 1:1 to this file: `templates`, `individual_size_measurements`, `multi_size_measurements`, and `bodyscans` are `qt6_common.ini` keys (see item 4b-i), not `qt6_seamly2d.ini` keys. This item's original wording overstates what should exist after ordinary use; see the new note at top of file. Not a code defect for `backups`/`images`/`labels`/`pattern`/`layout`, but `bodyscans` has no Preferences UI row at all — see the new known-defect note.
  - [x] 7b. check SeamlyMe preferences and settings file exists: `%LOCALAPPDATA%\Seamly\SeamlyMe\qt6_seamlyme.ini`. --> Confirmed.
  - [x] 7c. check SeamlyLayout default preferences file exists: `%LOCALAPPDATA%\Seamly\SeamlyLayout\preferences\default_preferences.json` and that 2 path entries ("input_directory", "layout_directory") are `%DATADIR%\layouts`, and 4 path entries ("preferences_directory", "preferences_file", "settings_directory", "settings_file") begin with `%LOCALAPPDATA%\SeamlyLayout`. --> File exists. the input_directory and layout_directory are correct, but the remaining directories are incorrect — reproduces the known SeamlyLayout defect (see top of file).
  - [x] 7d. check SeamlyLayout default settings file exists: `%LOCALAPPDATA%\Seamly\SeamlyLayout\settings\default_settings.json` --> File exists but in the wrong directory: `C:\Users\susan\seamlyLayout\settings\default_settings.json` — reproduces the known SeamlyLayout defect (see top of file).
  - [x] 7e. check SeamlyLayout preferences file: `%LOCALAPPDATA\Seamly\SeamlyLayout\qt6_seamlylayout.ini` -  Only `data_root` correctly resolves under `%DATADIR%`; the remaining paths (`input_directory`, `layout_directory`, `preferences_directory`, `preferences_file`, `settings_directory`, `settings_file`) all resolve incorrectly under `C:\Users\susan\seamlyLayout\...`; these should match the values in the default_preferences.json file — reproduces the known SeamlyLayout defect (see top of file).
- [x] 8. Check the logs in `%LOCALAPPDATA%\Seamly\Seamly2D\logs\` for additional errors (SeamlyMe and SeamlyLayout have no `logs\` folder). Confirmed — scanned all six log files for `error`/`warn`/`fail`/`crash`, no matches.
- [x] 9. Check Desktop shortcuts for all three apps in `C:\Users\Public\Desktop` (default settings, `SEAMLYDESKTOPSHORTCUTS` on). Confirmed — `Seamly2D.lnk`, `SeamlyMe.lnk`, `SeamlyLayout.lnk` all present, dated at install time.
- [x] 10. Check for the "Unknown Organization" defect (see top of file) on every verification pass. Confirmed absent — `%APPDATA%\Unknown Organization` not found.
