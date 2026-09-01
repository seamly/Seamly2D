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

- [x] 0. Relaunch this shell elevated (Administrator) before any step below. — PASS 2026-09-01 ~12:35: elevated child, UAC approved.
- [x] 1a. Uninstall Seamly (any and all versions detected) using `packaging\windows\test_reset_environment.ps1`. — PASS: prior 26.9.1.737 (upgrade-test install) removed, incl. the recorded `SeamlyUpgradeTest\SeamlyData` root.
  - [x] 1a-i. Confirm that the %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly\Seamly2D, AppData\Local\Seamly\SeamlyMe, AppData\Local\Seamly\SeamlyLayout, desktop shortcuts, and registry keys have been removed. — PASS except: stray `%LOCALAPPDATA%\SeamlyLayout` SURVIVED reset again (Layout.10). All other probes clean.
- [x] 1b. Install Seamly apps using `packaging\windows\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`. — PASS: status 0 (MSI built 2026-09-01, apps 26.9.1.737; carries SettingsFiles.4/6/7).

## B. Verification Suite

Run this suite after every test case in section A.

- [x] 0. Check the directories and files — results 2026-09-01, build 26.9.1.737
  - [x] 0a. PASS except two known dirs: after all three app first runs, everything below exists — `preferences\default_preferences.json` NOW CREATED (SettingsFiles.6 fix, first live pass) — EXCEPT `SeamlyLayout\logs` / `SeamlyMe\logs` (Layout.10 / SeamlyMe.5, filed). DataRoot tree + both samples seeded. Confirm these directories and files exist:
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
    %DATADIR%\SeamlyData
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
  - [x] 0b. Check the contents of the .ini files: — PASS at install time, all four seeded complete
    - [x] 0b-i. qt6_common.ini should contain: — PASS: all 5 keys, SeamlyData paths
      - [x] 0b-i1. "[paths]
dataRoot=%DATADIR%
individual_size_measurements=%DATADIR%/measurements/individual
multi_size_measurements=%DATADIR%/measurements/multisize
templates=%DATADIRT%/templates
bodyscans=%DATADIR%/bodyscans"
      - [x] 0b-i2. "[notices] firstRunDataNotice=pending" — PASS: `pending` at install, `shown` after the first Seamly2D run.
    - [x] 0b-ii. qt6_seamly2d.ini should contain: — PASS: all 6 keys
    "[paths]
pattern=%DATADIR%/patterns
layout=%DATADIR%/layouts
labels=%DATADIR%/label templates
images=%DATADIR%/images
backups=%DATADIR%/backups
seamlyLayoutApp=%PROGRAMDIR%/SeamlyApps/SeamlyLayout.exe"
    - [x] 0b-iii. qt6_seamlyme.ini should be empty — PASS: empty at install (Qt window-state keys appear after the SeamlyMe run, expected).
    - [x] ob-iv. qt6_seamlylayout.ini should contain: — PASS: all 11 keys, `%DATAROOT%`-resolved
    "[General]
input_directory=%DATADIRROOT%/layouts
layout_directory=%DATADIRROOT%/layouts
preferences_directory=%LOCALAPPDATA%/Seamly/SeamlyLayout/preferences
settings_directory=%LOCALAPPDATA%/Seamly/SeamlyLayout/settings
settings_file=%LOCALAPPDATA%/Seamly/SeamlyLayout/settings/default_settings.json
preferences_file=%LOCALAPPDATA%/Seamly/SeamlyLayout/preferences/default_preferences.json
dxf_viewer_path=https://sharecad.org
pdf_viewer_path=
png_viewer_path=
projector_path=https://patternprojector.com
data_root=%DATADIRROOT%"
  - [x] 0c. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`) contains `seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe`, `pdftops.exe`, `QtWebEngineProcess.exe`, `vc_redist.x64.exe`. — PASS: all six present.
  - [x] 0d. Confirm no duplicate directories. — PASS: one dir each in Program Files, `%LOCALAPPDATA%\Seamly`, Documents (`SeamlyData` only). Stray `%LOCALAPPDATA%\SeamlyLayout` is Layout.10.
  - [ ] 0e. if upgrading from previous non-SeamlyLayout version then: — N/A, fresh install. (A separate case-C upgrade on this MSI verified SettingsFiles.4 the same day — see Task SettingsFiles.4 in `TODO_COMPLETED.md`.)
    - [ ] 0e-i. confirm `%DATAROOT%\seamly2d.zip` exists.
    - [ ] 0e-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories.
- [x] 1. Check the registry keys: — results 2026-09-01
  - [ ] 1a. If not a fresh install then confirm old-version entries were removed. — N/A, fresh install.
  - [x] 1b. Confirm that the installed-version program entries were added for each app under `HKLM\SOFTWARE\Seamly\<application>`, each with matching `InstallPath` and `DisplayVersion`. — PASS: three keys, `InstallPath=C:\Program Files\SeamlyApps\`, `DisplayVersion=26.9.1.737`.
  - [x] 1c. Confirm that installed-version data entries were added with matching `DataRoot` and `DataParent` values; `Seamly2D` also carries the three `DesktopShortcut*` flags (this should be fixed in the future; tasks filed: SeamlyMe.3, Layout.7) — PASS: `DataRoot=...\Documents\SeamlyData\`, `DataParent=...\Documents\` on all three; `DesktopShortcut*` still on Seamly2D only (known, filed).
- [ ] 2. Check apps - **needs a human at the keyboard for most steps**
  - [ ] 2a. Run Seamly2D
    - [x] 2a-i. first run scripted: the "Seamly data moved" notice appears once, closes, then the main window appears — PASS 2026-09-01: Welcome closed (WM_CLOSE), notice once, main window, clean EXIT 0. No post-WM_CLOSE hang.
    - [x] 2a-ii. check if `qt6_common.ini` contains  "[notices] firstRunDataNotice=shown"; — PASS: flipped `pending`→`shown`; no repeat on the SeamlyLayout or SeamlyMe runs.
    - [ ] 2a-iii. Select 'file open' -- the dialog should open in the `%DATADIR\patterns` directory. — PENDING THE HUMAN (Seamly2D.2.1 still open; prior pass FAIL).
    - [ ] 2a-iv. Open `%DATADIR%\patterns\male_shirt.sm2d` pattern file with `%DATADIR%\measurements\individual\male_chest_102cm.smis` individual measurement file. — PENDING THE HUMAN.
    - [x] 2a-v. Check if directory exists: `%LOCALAPPDATA%\Seamly\Seamly2D\logs` — PASS: exists, log `seamly2d-pid18064.log` clean.
  - [ ] 2b. Check SeamlyMe from within Seamly2D
  - [ ] 2b-i. Select 'File Open Individual' - the dialog should open in the `%DATADIR\measurements\individual` directory.
    - [ ] 2b-ii. Select 'File Open Multisize' - the dialog should open in the `%DATADIR\measurements\multisize` directory.
    - [ ] 2b-iii. Select 'File Open Templates' - the dialog should open in the `%DATADIR\templates` directory.
    - [ ] 2b-iv. Select 'Edit Current' from the Measurements menu - the `%DATADIR%\measurements\individual\male_chest_102cm.smis` file should open.
    - [ ] 2b-v. Check if directory exists: `%DATADIR%\SeamlyMe\logs` — FAIL 2026-09-01 (scripted standalone run): SeamlyMe writes no logs (SeamlyMe.5, filed).
    - [ ] 2b-vi. Close SeamlyMe, returning focus to Seamly2D.
  - [ ] 2c. Run SeamlyLayout from within Seamly2D.
    - [ ] 2c-i. Visually confirm that the current pattern's `Piece mode` data is opened in the left canvas.
    - [ ] 2c-ii. Check if `MainWindow::exportPiecesToSeamlyLayout()` passes 'piece mode' data to SeamlyLayout as a stringified SVG document in a variable, not as a svg file from harddrive. — FAIL 2026-09-01 (static check): still a `.pieces.svg` file path at `mainwindow.cpp:4153/4165` (Seamly2D.5, Layout.9, filed).
    - [ ] 2c-iii. Check if directories exist: `%DATADIR%\SeamlyLayout\cache`, `%DATADIR%\SeamlyLayout\logs` — HALF FAIL 2026-09-01 (scripted standalone run): `cache` created, `logs` not (Layout.10, filed).
    - [ ] 2c-iv. Close SeamlyLayout, returning focus to Seamly2D.
  - [ ] 2d. Close Seamly2D.
- [x] 3. Check if `%LOCALAPPDATA%\SeamlyLayout\output` directory was created. If exists add a task to stop creating the `%LOCALAPPDATA%\SeamlyLayout\` directory and its `output` subdirectory that stores log files, and start creating the `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs` directory to store SeamlyLayout log files (similar to the `%LOCALAPPDATA\Seamly\Seamly2D\logs` directory) — CONFIRMED 2026-09-01: stray dir present. Task Layout.10 already filed; no new task.
- [x] 4. Check Desktop shortcuts `Seamly2D.lnk`, `SeamlyMe.lnk`, `SeamlyLayout.lnk` for all three apps in `C:\Users\Public\Desktop` (default settings, `SEAMLYDESKTOPSHORTCUTS` on). — PASS: all three present.
- [x] 5. Check the logs in `%LOCALAPPDATA%\Seamly\Seamly2D\logs\` for additional errors. — PASS: one log (`seamly2d-pid18064.log`), no error/warning/fatal/critical lines.
