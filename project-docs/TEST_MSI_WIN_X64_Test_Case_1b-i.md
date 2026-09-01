# TEST_MSI_WIN_X64_Test_Case_1b-i.md

Test plan for the Windows x64 Seamly MSI. Covers `packaging/windows/smsi.wxs`.

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`; default is `C:\Program Files\SeamlyApps`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`; default is `C:\Users\<user>\Documents\SeamlyData`.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

Known defect to watch for: `MainWindow::exportPiecesToSeamlyLayout()` (`mainwindow.cpp`) writes the pattern's pieces to a `.pieces.svg` file next to the pattern file and launches SeamlyLayout with that file path as an argument. This contradicts the intended design: the piece-mode SVG should be passed to SeamlyLayout as a stringified SVG document, not as a file. Check for this on every verification pass until fixed. Tasks filed: Seamly2D.5, Layout.9. STILL PRESENT on the 2026-09-01 second pass (static check, `mainwindow.cpp:4165-4168`).

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Fresh install | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Fresh install

- [x] 0. Relaunch this shell elevated (Administrator) before any step below. — PASS 2026-09-01 second pass: elevated child process, UAC-approved.
- [x] 1a. Uninstall Seamly (any and all versions detected) using `packaging\windows\test_reset_environment.ps1`. — PASS: prior 26.8.44328 removed.
  - [x] 1a-i. Confirm that the %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly\Seamly2D, AppData\Local\Seamly\SeamlyMe, AppData\Local\Seamly\SeamlyLayout, desktop shortcuts, and registry keys have been removed. — PASS except: stray `%LOCALAPPDATA%\SeamlyLayout` SURVIVED reset (recreated by last pass's app runs; Layout.10 covers adding it to the reset script). All other probes clean.
- [x] 1b. Install Seamly apps using `packaging\windows\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`. — PASS: status 0 at 10:05 (same MSI built 2026-08-31 7:15 PM, ProductVersion 26.8.44328, apps 26.8.31.1128).

## B. Verification Suite

Run this suite after every test case in section A.

- [ ] 0. Check the directories and files — 2026-09-01 second pass: FAIL, two files missing (see 0a).
  - [ ] 0a. Confirm these directories and files exist: — FAIL: all present after first app runs EXCEPT `SeamlyLayout\logs` (Layout.10) and `SeamlyLayout\preferences\default_preferences.json` (SettingsFiles.6). Both already filed. Install-time note: logs dirs, cache, DataRoot subtree and samples appear at app first run, not install — expected.
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
  - [x] 0b. Check the contents of the .ini files: — PASS 2026-09-01 second pass: all four inis seeded at install, keys match.
    - [x] 0b-i. qt6_common.ini should contain:
      - [x] 0b-i1. "[paths]
dataRoot=%DATADIR%
individual_size_measurements=%DATADIR%/measurements/individual
multi_size_measurements=%DATADIR%/measurements/multisize
templates=%DATADIRT%/templates
bodyscans=%DATADIR%/bodyscans"
      - [x] 0b-i2. "[notices] firstRunDataNotice=pending" — PASS: `pending` at install; flipped to `shown` after the Seamly2D first run (see 2a-ii).
    - [x] 0b-ii. qt6_seamly2d.ini should contain:
    "[paths]
pattern=%DATADIR%/patterns
layout=%DATADIR%/layouts
labels=%DATADIR%/label templates
images=%DATADIR%/images
backups=%DATADIR%/backups
seamlyLayoutApp=%PROGRAMDIR%/SeamlyApps/SeamlyLayout.exe"
    - [x] 0b-iii. qt6_seamly2d.ini should be empty — PASS (read as `qt6_seamlyme.ini`, which is empty; doc nit stands).
    - [x] ob-iv. qt6_seamlylayout.ini should contain:
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
  - [x] 0d. Confirm no duplicate directories. — PASS: one dir each in Program Files, `%LOCALAPPDATA%\Seamly`, Documents, none in Roaming. Stray `%LOCALAPPDATA%\SeamlyLayout` is Layout.10, not a duplicate install dir.
  - [ ] 0e. if upgrading from previous non-SeamlyLayout version then: — N/A, fresh install.
    - [ ] 0e-i. confirm `%DATAROOT%\seamly2d.zip` exists.
    - [ ] 0e-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories.
- [x] 1. Check the registry keys: — PASS 2026-09-01 second pass.
  - [ ] 1a. If not a fresh install then confirm old-version entries were removed. — N/A, fresh install.
  - [x] 1b. Confirm that the installed-version program entries were added for each app under `HKLM\SOFTWARE\Seamly\<application>`, each with matching `InstallPath` and `DisplayVersion`. — PASS: three keys, `InstallPath=C:\Program Files\SeamlyApps\`, `DisplayVersion=26.8.31.1128`.
  - [x] 1c. Confirm that installed-version data entries were added with matching `DataRoot` and `DataParent` values; `Seamly2D` also carries the three `DesktopShortcut*` flags (this should be fixed in the future; tasks filed: SeamlyMe.3, Layout.7) — PASS: `DataRoot`/`DataParent` match on all three; `DesktopShortcut*` flags still on the Seamly2D key only (known, filed).
- [ ] 2. Check apps - **needs a human at the keyboard for most steps**
  - [ ] 2a. Run Seamly2D
    - [x] 2a-i. first run scripted: the "Seamly data moved" notice appears once, closes, then the main window appears — PASS 2026-09-01 second pass: notice once, then main window. Deviation: after WM_CLOSE on the main window the process stayed alive 90 s and the harness killed it (prior pass exited 0); log shows no error. Watch on the next pass.
    - [x] 2a-ii. check if `qt6_common.ini` contains  "[notices] firstRunDataNotice=shown"; — PASS: `pending` → `shown`.
    - [ ] 2a-iii. Select 'file open' -- the dialog should open in the `%DATADIR\patterns` directory. — FAIL 2026-09-01 second pass (human): dialog opened in `C:\Users\<user>`. Known cause, filed: Seamly2D.2.1 (`MainWindow::Open()` never reads `getPatternPath()` when the recent-files list is empty).
    - [x] 2a-iv. Open `%DATADIR%\patterns\male_shirt.sm2d` pattern file with `%DATADIR%\measurements\individual\male_chest_102cm.smis` individual measurement file. — PASS 2026-09-01 second pass (human).
    - [x] 2a-v. Check if directory exists: `%LOCALAPPDATA%\Seamly\Seamly2D\logs` — PASS: created at first run; DataRoot subtree and both sample files also seeded.
  - [ ] 2b. Check SeamlyMe from within Seamly2D — 2026-09-01 second pass (human): all PASS except 2b-v.
    - [x] 2b-i. Select 'File Open Individual' - the dialog should open in the `%DATADIR\measurements\individual` directory. — PASS.
    - [x] 2b-ii. Select 'File Open Multisize' - the dialog should open in the `%DATADIR\measurements\multisize` directory. — PASS.
    - [x] 2b-iii. Select 'File Open Templates' - the dialog should open in the `%DATADIR\templates` directory. — PASS.
    - [x] 2b-iv. Select 'Edit Current' from the Measurements menu - the `%DATADIR%\measurements\individual\male_chest_102cm.smis` file should open. — PASS.
    - [ ] 2b-v. Check if directory exists: `%LOCALAPPDATA%\SeamlyMe\logs` — FAIL: SeamlyMe writes no logs at all (`ApplicationME` has no logging setup). NEW task filed: SeamlyMe.5 (`TODO_SEAMLYME.md`) — target `%LOCALAPPDATA%\Seamly\SeamlyMe\logs`, mirroring Seamly2D.
    - [x] 2b-vi. Close SeamlyMe, returning focus to Seamly2D. — PASS.
  - [ ] 2c. Run SeamlyLayout from within Seamly2D. — 2026-09-01 second pass (human): PASS except 2c-ii and the logs half of 2c-iii.
    - [x] 2c-i. Visually confirm that the current pattern's `Piece mode` data is opened in the left canvas. — PASS.
    - [ ] 2c-ii. Check if `MainWindow::exportPiecesToSeamlyLayout()` passes 'piece mode' data to SeamlyLayout as a stringified SVG document in a variable, not as a svg file from harddrive. — FAIL 2026-09-01 second pass (static check): still writes `.pieces.svg` (`mainwindow.cpp:4165-4168`). Filed: Seamly2D.5, Layout.9.
    - [ ] 2c-iii. Check if directories exist: `%LOCALAPPDIR%\SeamlyLayout\cache`, `%LOCALAPPDIR%\SeamlyLayout\logs` — 2026-09-01 second pass, confirmed by the human after the in-Seamly2D run (dirs under `%LOCALAPPDATA%\Seamly\SeamlyLayout`): `cache` PASS, `logs` FAIL (Layout.10).
    - [x] 2c-iv. Close SeamlyLayout, returning focus to Seamly2D. — PASS.
  - [x] 2d. Close Seamly2D. — PASS.
- [x] 3. — CONFIRMED 2026-09-01 second pass: dir survived reset and the SeamlyLayout run wrote a fresh `log_260901100840.txt` into it. Task already filed: Layout.10. Check if `%LOCALAPPDATA%\SeamlyLayout\output` directory was created. If exists add a task to stop creating the `%LOCALAPPDATA%\SeamlyLayout\` directory and its `output` subdirectory that stores log files, and start creating the `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs` directory to store SeamlyLayout log files (similar to the `%LOCALAPPDATA\Seamly\Seamly2D\logs` directory)
- [x] 4. Check Desktop shortcuts `Seamly2D.lnk`, `SeamlyMe.lnk`, `SeamlyLayout.lnk` for all three apps in `C:\Users\Public\Desktop` (default settings, `SEAMLYDESKTOPSHORTCUTS` on). — PASS: all three present.
- [x] 5. Check the logs in `%LOCALAPPDATA%\Seamly\Seamly2D\logs\` for additional errors. — PASS: one log (`seamly2d-pid20844.log`), INFO/DEBUG only, no errors.
