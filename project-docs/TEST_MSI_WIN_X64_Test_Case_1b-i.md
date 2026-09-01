# TEST_MSI_WIN_X64_Test_Case_1b-i.md

Test plan for the Windows x64 Seamly MSI. Covers `packaging/windows/smsi.wxs`.

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`; default is `C:\Program Files\SeamlyApps`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`; default is `C:\Users\<user>\SeamlyData`.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

Known defect to watch for: `MainWindow::exportPiecesToSeamlyLayout()` (`mainwindow.cpp`) writes the pattern's pieces to a `.pieces.svg` file next to the pattern file and launches SeamlyLayout with that file path as an argument. This contradicts the intended design: the piece-mode SVG should be passed to SeamlyLayout as a stringified SVG document, not as a file. Found during Case 1 item 6b-ii. Check for this on every verification pass until fixed. STILL PRESENT on the 2026-08-31 evening re-pass (`mainwindow.cpp:4153`). Tasks filed: `Seamly2D.5` (`TODO_SEAMLY2D.md`) and `Layout.9` (`TODO_SEAMLYLAYOUT.md`).

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Fresh install | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Fresh install

- [x] 0. Relaunch this shell elevated (Administrator) before any step below. 2026-08-31 evening pass: session shell stayed unelevated; a UAC-approved elevated child ran steps 1a and 1b and logged its token as elevated.
- [x] 1a. Uninstall Seamly (any and all versions detected) using `packaging\windows\test_reset_environment.ps1`. Prior install found: Seamly 26.8.44328. Script ran clean.
  - [x] 1a-i. Confirm that the %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly\Seamly2D, AppData\Local\Seamly\SeamlyMe, AppData\Local\Seamly\SeamlyLayout, desktop shortcuts, and registry keys have been removed. Confirmed — all ten checks clean (HKLM and HKCU included). Note: `%LOCALAPPDATA%\SeamlyLayout` (stray log dir, see 7f) survives the reset script — Layout.10.3.
- [x] 1b. Install Seamly apps using `packaging\windows\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`. Exit code 0. Installed version 26.8.31.1128 (MSI built 2026-08-31 7:15 PM).

## B. Verification Suite

Run this suite after every test case in section A.

- [x] 1. Run the apps: 2026-08-31 evening pass, scripted launch and close.
  - [x] 1a. run Seamly2D then close Seamly2D. First attempt blocked at the modal Welcome dialog and was killed by the test harness (exit -1) before the notice fired. Rerun with Welcome handled: exit code 0, clean close, no notice repeat.
  - [x] 1b. run SeamlyMe then close SeamlyMe. Exit code 0. Showed the "Seamly data moved" notice (see 4b-ii) because it was the first app to complete startup.
  - [x] 1c. run SeamlyLayout then close SeamlyLayout. Exit code 0, no popup.
- [x] 2. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`) contains `seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe`, `pdftops.exe`, `QtWebEngineProcess.exe`, `vc_redist.x64.exe`. Confirmed — all six files present.
- [x] 3. Check user-data location (default `C:\Users\<user>\Documents\SeamlyData\`), subdirectories, and files:
  - [x] 3a. Subdirectories `backups`, `bodyscans`, `images`, `label templates`, `layouts`,  `measurements\individual`, `measurements\multisize`, `patterns`, and `templates` are created at the correct level below `%DATAROOT%`. Confirmed after first app run (`%DATAROOT%` empty at install time is expected).
  - [x] 3b. No duplicate directories. Confirmed — no nested `SeamlyData` folder.
  - [x] 3c. if upgrading from previous non-SeamlyLayout version then: N/A (fresh install).
    - [x] 3c-i. confirm `%DATAROOT%\seamly2d.zip` exists. N/A (fresh install).
    - [x] 3c-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories. N/A (fresh install).
  - [x] 3d. check that %DATADIR\patterns\male_shirt.sm2d exists. Confirmed.
  - [x] 3e. check that %DATADIR\measurements\individual\male_chest_102cm.smis exists. Confirmed.
- [x] 4. Check the user application directories:
  - [x] 4a. `%LOCALAPPDATA%\Seamly\<AppName>\` exists for each app. Confirmed for Seamly2D, SeamlyMe, SeamlyLayout at install time.
  - [x] 4b. `%LOCALAPPDATA%\Seamly\qt6_common.ini` file exists. Confirmed at install time.
    - [x] 4b-i. Confirm `%LOCALAPPDATA%\Seamly\qt6_common.ini` file holds 5 entries: `dataRoot`, `individual_size_measurements`, `multi_size_measurements`, `templates`, `bodyscans`; all five values are paths under `%DATAROOT%`. Confirmed at install time and unchanged after all three apps ran.
    - [x] 4b-ii. Confirm the one-shot first-run data notice : at install time `qt6_common.ini` holds `[notices] firstRunDataNotice=pending`; the first app run shows one popup about the data locations and backups, then the value reads `shown`; no later app run repeats the popup. Confirmed with one deviation: Seamly2D's first attempt died at the Welcome dialog before the notice, so SeamlyMe was the first completed run — it showed "Seamly data moved" once (dismissed by automation), value flipped to `shown`; SeamlyLayout and the Seamly2D rerun showed no repeat. Contract "whichever app runs first clears it" holds. Visual review of the popup text stays with the human pass (item 6).
- [x] 5. Check the registry keys:
  - [x] 5a. If not a fresh install then confirm old-version entries were removed. N/A (fresh install; reset verified HKLM/HKCU Seamly keys absent before install).
  - [x] 5b. Confirm that the installed-version program entries were added for each app under `HKLM\SOFTWARE\Seamly\<application>`, each with matching `InstallPath` and `DisplayVersion`. Confirmed for all three apps (`26.8.31.1128`, `C:\Program Files\SeamlyApps\`).
  - [x] 5c. Confirm that installed-version data entries were added with matching `DataRoot` and `DataParent` values; `Seamly2D` also carries the three `DesktopShortcut*` flags (this should be fixed in the future; tasks filed: `SeamlyMe.3` in `TODO_SEAMLYME.md`, `Layout.7` in `TODO_SEAMLYLAYOUT.md`). Confirmed — all three apps carry `DataRoot=C:\Users\susan\Documents\SeamlyData\`, `DataParent=C:\Users\susan\Documents\`; the three `DesktopShortcut*` flags sit on `Seamly2D` only, as noted.
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
    - [ ] 6b-iii. Confirm that the 'piece mode' data was passed to SeamlyLayout as a stringified svg document (not as a svg file). Code check 2026-08-31: `MainWindow::exportPiecesToSeamlyLayout()` (`mainwindow.cpp:4153`) still writes `<pattern-basename>.pieces.svg` and passes the file path — defect present. Tasks filed: `Seamly2D.5`, `Layout.9`.
    - [ ] 6b-iv. Close SeamlyLayout, returning focus to Seamly2D.
  - [ ] 6c. Close Seamly2D.
- [x] 7. Check Application Preferences and Settings and files (7a-v excepted — human)
  - [x] 7a. confirm Seamly2D's preferences and settings file `%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini`:
    - [x] 7a-i. confirm this file exists at install time, before any app runs. Confirmed.
    - [x] 7a-ii. confirm this file contains a 'paths' section. Confirmed.
    - [x] 7a-iii. confirm this file contains keys: the per-app `[paths]` keys are `pattern`, `layout`, `labels`, `images`, `backups`, `seamlyLayoutApp` (the shared keys `templates`, `individual_size_measurements`, `multi_size_measurements`, `bodyscans`, `dataRoot` live in `qt6_common.ini` by design — see 4b-i). Confirmed all six present at install time.
    - [x] 7a-iv. confirm the 'paths' keys all begin with the `%DATADIR%` value, except `seamlyLayoutApp`, which points at `%PROGRAMDIR%/SeamlyLayout.exe`. Confirmed.
    - [ ] 7a-v. Re-verify the `bodyscans` UI-row fix: the `bodyscans` key in `qt6_common.ini` is now installer-seeded (see 4b-i); to verify the UI row itself, change it in Preferences > Paths and confirm the changed value lands in `qt6_common.ini`. **Needs a human at the keyboard.**
  - [x] 7b. check SeamlyMe preferences and settings file exists: `%LOCALAPPDATA%\Seamly\SeamlyMe\qt6_seamlyme.ini`. Confirmed at install time.
  - [x] 7c. check the installer created the  `%LOCALAPPDATA%\Seamly\SeamlyLayout\preferences\` and `settings\` directories at install time. `preferences\default_preferences.json` is NOT created at first launch any more; the app creates it on demand when the user resets to defaults. Confirmed — both directories present at install time; `default_preferences.json` absent at install time and still absent after the app ran.
  - [x] 7d. after SeamlyLayout has run once, check the default settings file exists: `%LOCALAPPDATA%\Seamly\SeamlyLayout\settings\default_settings.json` (copied at runtime from the packaged `settings\` folder). Confirmed after the run (absent at install time, as designed).
  - [x] 7e. check SeamlyLayout preferences file `%LOCALAPPDATA%\Seamly\SeamlyLayout\qt6_seamlylayout.ini`:
    - [x] 7e-i. confirm this file exists at install time, before any app runs (installer-seeded, SettingsFiles.3). Confirmed.
    - [x] 7e-ii. confirm its `[General]` section holds all 11 keys: `input_directory`, `layout_directory`, `preferences_directory`, `settings_directory`, `settings_file`, `preferences_file`, `dxf_viewer_path`, `pdf_viewer_path`, `png_viewer_path`, `projector_path`, `data_root`. Confirmed — all 11 present (`pdf_viewer_path`, `png_viewer_path` empty).
    - [x] 7e-iii. confirm `input_directory`, `layout_directory` = `%DATADIR%/layouts`; `data_root` = `%DATADIR%`; `preferences_directory`, `settings_directory`, `settings_file`, `preferences_file` sit under `%LOCALAPPDATA%\Seamly\SeamlyLayout\`; `dxf_viewer_path` = `https://sharecad.org`; `projector_path` = `https://patternprojector.com`. Confirmed — all values match.
  - [x] 7f. Check if `%LOCALAPPDATA%\SeamlyLayout\output` directory was created. DEFECT CONFIRMED: `Logger::init()` wrote `log_260831204937.txt` into `%LOCALAPPDATA%\SeamlyLayout\output\` during this pass's run. Task filed: `Layout.10` (`TODO_SEAMLYLAYOUT.md`) — move logs to `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs` and stop creating the stray directory.
- [x] 8. Check the logs in `%LOCALAPPDATA%\Seamly\Seamly2D\logs\` for additional errors. Clean — two logs (killed first attempt + clean rerun), INFO lines only, no errors.
- [x] 9. Check Desktop shortcuts `Seamly2D.lnk`, `SeamlyMe.lnk`, `SeamlyLayout.lnk` for all three apps in `C:\Users\Public\Desktop` (default settings, `SEAMLYDESKTOPSHORTCUTS` on). Confirmed — all three present.
