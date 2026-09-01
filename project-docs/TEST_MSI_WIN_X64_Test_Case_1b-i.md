# TEST_MSI_WIN_X64_Test_Case_1b-i.md

Test plan for the Windows x64 Seamly MSI. Covers `packaging/windows/smsi.wxs`.

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`; default is `C:\Program Files\SeamlyApps`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`; default is `C:\Users\<user>\SeamlyData`.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

Known defect to watch for: `MainWindow::exportPiecesToSeamlyLayout()` (`mainwindow.cpp`) writes the pattern's pieces to a `.pieces.svg` file next to the pattern file and launches SeamlyLayout with that file path as an argument. This contradicts the intended design: the piece-mode SVG should be passed to SeamlyLayout as a stringified SVG document, not as a file. Found during Case 1 item 6b-ii. Check for this on every verification pass until fixed. STILL PRESENT on the 2026-08-31 notice-build pass (`mainwindow.cpp:4153`).

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Fresh install | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Fresh install

- [x] 0. Relaunch this shell elevated (Administrator) before any step below. 2026-08-31 pass: session shell stayed unelevated; a UAC-approved elevated child process ran steps 1a and 1b and logged its token as elevated.
- [x] 1a. Uninstall Seamly (any and all versions detected) using `packaging\windows\test_reset_environment.ps1`. Prior install found: Seamly 26.8.44187. Script ran clean, no errors.
  - [x] 1a-i. Confirm that the %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly\Seamly2D, AppData\Local\Seamly\SeamlyMe, AppData\Local\Seamly\SeamlyLayout, desktop shortcuts, and registry keys have been removed. Confirmed — all nine checks removed (HKLM and HKCU included).
- [x] 1b. Install Seamly apps using `packaging\windows\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`. Exit code 0. Installed version 26.8.31.1128 (first-run-notice build, MSI built 2026-08-31 7:15 PM).

## B. Verification Suite

Run this suite after every test case in section A.

- [x] 1. Run the apps: 2026-08-31 pass, scripted launch and close.
  - [x] 1a. run Seamly2D then close Seamly2D. Exit code 0; first-run notice dismissed (see 4b-ii).
  - [x] 1b. run SeamlyMe then close SeamlyMe. Exit code 0, no popup.
  - [x] 1c. run SeamlyLayout then close SeamlyLayout. Exit code 0, no popup.
- [x] 2. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`) contains `seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe`, `pdftops.exe`, `QtWebEngineProcess.exe`, `vc_redist.x64.exe`. Confirmed — all six files present.
- [x] 3. Check user-data location (default `C:\Users\<user>\Documents\SeamlyData\`), subdirectories, and files:
  - [x] 3a. Subdirectories `backups`, `bodyscans`, `images`, `label templates`, `layouts`,  `measurements\individual`, `measurements\multisize`, `patterns`, and `templates` are created at the correct level below `%DATAROOT%`. Confirmed present after first app run (empty `%DATAROOT%` at install time is expected).
  - [x] 3b. No duplicate directories. Confirmed — no nested `SeamlyData` folder found.
  - [x] 3c. if upgrading from previous non-SeamlyLayout version then: N/A (fresh install).
    - [x] 3c-i. confirm `%DATAROOT%\seamly2d.zip` exists. N/A (fresh install).
    - [x] 3c-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories. N/A (fresh install).
  - [x] 3d. check that %DATADIR\patterns\male_shirt.sm2d exists. Confirmed.
  - [x] 3e. check that %DATADIR\measurements\individual\male_chest_102cm.smis exists. Confirmed.
- [x] 4. Check the user application directories:
  - [x] 4a. `%LOCALAPPDATA%\Seamly\<AppName>\` exists for each app. Confirmed for Seamly2D, SeamlyMe, SeamlyLayout at install time.
  - [x] 4b. `%LOCALAPPDATA%\Seamly\qt6_common.ini` file exists (moved from `%APPDATA%\Seamly\` by SettingsFiles.1, 2026-08-30). Confirmed at install time, 2026-08-31 notice-build pass.
    - [x] 4b-i. Confirm `%LOCALAPPDATA%\Seamly\qt6_common.ini` file holds 5 entries: `dataRoot`, `individual_size_measurements`, `multi_size_measurements`, `templates`, `bodyscans`; all five values are paths under `%DATAROOT%`. The installer seeds them (SettingsFiles.2, 2026-08-31) — no Preferences visit required. Confirmed at install time and unchanged after all three apps ran, 2026-08-31 notice-build pass.
    - [x] 4b-ii. Confirm the one-shot first-run data notice (SettingsFiles.5): at install time `qt6_common.ini` holds `[notices] firstRunDataNotice=pending`; the first app run shows one popup about the data locations and backups, then the value reads `shown`; no later app run repeats the popup. Confirmed by script 2026-08-31: `pending` at install time; Seamly2D's first run raised the "Seamly data moved" window (found by title and dismissed by automation); value read `shown` after; SeamlyMe and SeamlyLayout ran and closed with no popup and the value stayed `shown`. Visual review of the popup text stays with the human pass (item 6).
- [x] 5. Check the registry keys:
  - [x] 5a. If not a fresh install then confirm old-version entries were removed. N/A (fresh install; reset verified HKLM/HKCU Seamly keys absent before install).
  - [x] 5b. Confirm that the installed-version program entries were added for each app under `HKLM\SOFTWARE\Seamly\<application>`, each with matching `InstallPath` and `DisplayVersion`. Confirmed for Seamly2D, SeamlyMe, SeamlyLayout (`26.8.31.1128`, `C:\Program Files\SeamlyApps\`).
  - [x] 5c. Confirm that installed-version data entries were added with matching `DataRoot` and `DataParent` values; `Seamly2D` also carries the three `DesktopShortcut*` flags (this should be fixed in the future). Confirmed — all three apps carry `DataRoot=C:\Users\susan\Documents\SeamlyData\`, `DataParent=C:\Users\susan\Documents\`; the three `DesktopShortcut*` flags sit on `Seamly2D` only, as noted.
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
    - [ ] 6b-ii. Confirm that the current pattern's `Piece mode` data was passed to SeamlyLayout as a stringified svg document (not as a svg file). --> Still fails by code inspection (2026-08-31): `MainWindow::exportPiecesToSeamlyLayout()` writes `<pattern-basename>.pieces.svg` next to the pattern file and launches SeamlyLayout with that file path — a file, not a stringified SVG document. See the known-defect note at top of file.
    - [ ] 6b-iv. Close SeamlyLayout, returning focus to Seamly2D.
  - [ ] 6c. Close Seamly2D.
- [x] 7. Check Application Preferences and Settings
  - [x] 7a. confirm Seamly2D's preferences and settings file `%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini`:
    - [x] 7a-i. confirm this file exists at install time, before any app runs. Confirmed, 2026-08-31 notice-build pass.
    - [x] 7a-ii. confirm this file contains a 'paths' section. Confirmed.
    - [x] 7a-iii. confirm this file contains keys: `pattern`, `layout`, `labels`, `images`, `backups`, `seamlyLayoutApp` (the shared keys `individual_size_measurements`, `multi_size_measurements`, `templates`, `bodyscans` live in `qt6_common.ini` — see 4b-i); Confirmed, all six present at install time and unchanged after Seamly2D ran.
    - [x] 7a-iv. confirm the 'paths' keys all begin with the `%DATADIR%` value, except `seamlyLayoutApp`, which points at `%PROGRAMDIR%/SeamlyLayout.exe`. Confirmed — `seamlyLayoutApp=C:/Program Files/SeamlyApps/SeamlyLayout.exe`.
    - [ ] 7a-v. Re-verify the `bodyscans` UI-row fix: the `bodyscans` key in `qt6_common.ini` is now installer-seeded (see 4b-i); to verify the UI row itself, change it in Preferences > Paths and confirm the changed value lands in `qt6_common.ini`. **Needs a human at the keyboard.**
  - [x] 7b. check SeamlyMe preferences and settings file exists: `%LOCALAPPDATA%\Seamly\SeamlyMe\qt6_seamlyme.ini`. Confirmed at install time (seeded empty).
  - [x] 7c. check the installer created `%LOCALAPPDATA%\Seamly\SeamlyLayout\preferences\` and `settings\` directories at install time. `preferences\default_preferences.json` is NOT created at first launch any more (SettingsFiles.3 — the complete seeded ini skips the app's deprecated first-run seeding); the app creates it on demand when the user resets to defaults. Confirmed — both directories present at install time; no `default_preferences.json` after first launch, 2026-08-31 notice-build pass.
  - [x] 7d. after SeamlyLayout has run once, check the default settings file exists: `%LOCALAPPDATA%\Seamly\SeamlyLayout\settings\default_settings.json` (copied at runtime from the packaged `settings\` folder). Confirmed.
  - [x] 7e. check SeamlyLayout preferences file `%LOCALAPPDATA%\Seamly\SeamlyLayout\qt6_seamlylayout.ini`:
    - [x] 7e-i. confirm this file exists at install time, before any app runs (installer-seeded, SettingsFiles.3). Confirmed, 2026-08-31 notice-build pass.
    - [x] 7e-ii. confirm its `[General]` section holds all 11 keys: `input_directory`, `layout_directory`, `preferences_directory`, `settings_directory`, `settings_file`, `preferences_file`, `dxf_viewer_path`, `pdf_viewer_path`, `png_viewer_path`, `projector_path`, `data_root`. Confirmed, and unchanged after SeamlyLayout ran.
    - [x] 7e-iii. confirm `input_directory`, `layout_directory` = `%DATADIR%/layouts`; `data_root` = `%DATADIR%`; `preferences_directory`, `settings_directory`, `settings_file`, `preferences_file` sit under `%LOCALAPPDATA%\Seamly\SeamlyLayout\`; `dxf_viewer_path` = `https://sharecad.org`; `projector_path` = `https://patternprojector.com`. Confirmed, all values exact.
- [x] 8. Check the logs in `%LOCALAPPDATA%\Seamly\Seamly2D\logs\` for additional errors (SeamlyMe and SeamlyLayout have no `logs\` folder). Confirmed — one log file this pass, no `error`/`warn`/`fail`/`crash` matches.
- [x] 9. Check Desktop shortcuts for all three apps in `C:\Users\Public\Desktop` (default settings, `SEAMLYDESKTOPSHORTCUTS` on). Confirmed — `Seamly2D.lnk`, `SeamlyMe.lnk`, `SeamlyLayout.lnk` all present, dated at install time (2026-08-31 7:31 PM).
- [x] 10. Check for the "Unknown Organization" defect (see top of file) on every verification pass. Confirmed absent — `%APPDATA%\Unknown Organization` and `.ini` not found.
