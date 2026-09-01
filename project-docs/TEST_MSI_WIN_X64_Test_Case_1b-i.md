# TEST_MSI_WIN_X64_Test_Case_1b-i.md

Test plan for the Windows x64 Seamly MSI. Covers `packaging/windows/smsi.wxs`.

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`; default is `C:\Program Files\SeamlyApps`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`; default is `C:\Users\<user>\SeamlyData`.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

Known defect to watch for: `MainWindow::exportPiecesToSeamlyLayout()` (`mainwindow.cpp`) writes the pattern's pieces to a `.pieces.svg` file next to the pattern file and launches SeamlyLayout with that file path as an argument. This contradicts the intended design: the piece-mode SVG should be passed to SeamlyLayout as a stringified SVG document, not as a file. Found during Case 1 item 6b-ii. Check for this on every verification pass until fixed. STILL PRESENT on the 2026-08-31 notice-build pass (`mainwindow.cpp:4153`). add a task to fix this problem. Tasks filed: Seamly2D.5, Layout.9. STILL PRESENT on the 2026-09-01 pass (static check, same line).

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Fresh install | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Fresh install

- [x] 0. Relaunch this shell elevated (Administrator) before any step below. — PASS 2026-09-01: elevated child process, UAC-approved.
- [x] 1a. Uninstall Seamly (any and all versions detected) using `packaging\windows\test_reset_environment.ps1`. — PASS: prior 26.8.44328 removed.
  - [x] 1a-i. Confirm that the %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly\Seamly2D, AppData\Local\Seamly\SeamlyMe, AppData\Local\Seamly\SeamlyLayout, desktop shortcuts, and registry keys have been removed. — PASS: every probe clean, incl. stray `%LOCALAPPDATA%\SeamlyLayout`.
- [x] 1b. Install Seamly apps using `packaging\windows\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`. — PASS: status 0 (MSI built 2026-08-31 7:15 PM, ProductVersion 26.8.44328, apps 26.8.31.1128).

## B. Verification Suite

Run this suite after every test case in section A.

- [ ] 0. Check the directories and files — 2026-09-01 pass: FAIL, two files missing (see 0a).
  - [ ] 0a. Confirm these directories and files exist: — FAIL: all present after first app runs EXCEPT `SeamlyLayout\logs` (Layout.10) and `SeamlyLayout\preferences\default_preferences.json` (new task SettingsFiles.6). Actual `%DATADIR%` is `C:\Users\<user>\Documents\SeamlyData`, not `C:\Users\<user>\SeamlyData` as line 8 states. Install-time note: logs dirs, cache, DataRoot subtree and samples appear at app first run, not install — expected.
    %PROGRAMDIR%\SeamlyApps
    |  |_seamly2d.exe
    |  |_seamlylayout.exe
    |  |_seamlyme.exe
    %LOCALAPPDATA%\Seamly
    |_qt6_common.ini
    |_Seamly2D
    |  |_logs
    |  |_qt6_seamly2d.ini
    |_SeamlyLayout
    |  |_cache
    |  |_logs
    |  |_preferences
    |  |  |_default_preferences.json
    |  |_settings
    |  |  |_default_settings.json
    |  |_qt6_seamlyLayout.ini
    |_SeamlyMe
    |  |_logs
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
  - [x] 0b. Check the contents of the .ini files: — PASS 2026-09-01 (0b-iii read as `qt6_seamlyme.ini`).
    - [x] 0b-i. qt6_common.ini should contain:
      - [x] 0b-i1. — PASS (all 5 keys; doc typo `%DATADIRT%` read as `%DATADIR%`). "[paths]
dataRoot=%DATADIR%
individual_size_measurements=%DATADIR%/measurements/individual
multi_size_measurements=%DATADIR%/measurements/multisize
templates=%DATADIRT%/templates
bodyscans=%DATADIR%/bodyscans"
      - [x] 0b-i2. "[notices] firstRunDataNotice=pending" — PASS at install time; flips to `shown` after the first app run (2a-i).
    - [x] 0b-ii. qt6_seamly2d.ini should contain:
    "[paths]
pattern=%DATADIR%/patterns
layout=%DATADIR%/layouts
labels=%DATADIR%/label templates
images=%DATADIR%/images
backups=%DATADIR%/backups
seamlyLayoutApp=%PROGRAMDIR%/SeamlyApps/SeamlyLayout.exe"
    - [x] 0b-iii. qt6_seamly2d.ini should be empty — PASS as `qt6_seamlyme.ini` (doc typo: qt6_seamly2d.ini is checked by 0b-ii). `qt6_seamlyme.ini` is empty.
    - [x] ob-iv. qt6_seamlylayout.ini should contain: — PASS: all keys match, seeded at install time.
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
  - [x] 0c. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`) contains `seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe`, `pdftops.exe`, `QtWebEngineProcess.exe`, `vc_redist.x64.exe`. — PASS: all 6 present.
  - [x] 0d. Confirm no duplicate directories. — PASS: one Seamly dir each in Program Files, `%LOCALAPPDATA%`, Documents; none in Roaming.
  - [ ] 0e. if upgrading from previous non-SeamlyLayout version then: — N/A, fresh install (Case 1).
    - [ ] 0e-i. confirm `%DATAROOT%\seamly2d.zip` exists.
    - [ ] 0e-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories.
- [x] 1. Check the registry keys: — PASS 2026-09-01.
  - [ ] 1a. If not a fresh install then confirm old-version entries were removed. — N/A, fresh install.
  - [x] 1b. Confirm that the installed-version program entries were added for each app under `HKLM\SOFTWARE\Seamly\<application>`, each with matching `InstallPath` and `DisplayVersion`. — PASS: all three keys, `InstallPath=C:\Program Files\SeamlyApps\`, `DisplayVersion=26.8.31.1128`.
  - [x] 1c. Confirm that installed-version data entries were added with matching `DataRoot` and `DataParent` values; `Seamly2D` also carries the three `DesktopShortcut*` flags (this should be fixed in the future; add a task to fix this) — PASS: `DataRoot`/`DataParent` match on all three keys; the three `DesktopShortcut*` flags sit on the Seamly2D key as described. Fix tasks already filed: SeamlyMe.3, Layout.7.
- [ ] 2. Check apps - **needs a human at the keyboard for most steps** — 2026-09-01: 2a-i done scripted; 2a-ii..2d pending the human.
  - [ ] 2a. Run Seamly2D — first run scripted: the "Seamly data moved" notice appeared once, then the main window; clean exit 0. No Welcome blocker this pass.
    - [x] 2a-i. check if `qt6_common.ini` contains  "[notices] firstRunDataNotice=shown"; — PASS: `pending` flipped to `shown` after the run.
    - [ ] 2a-ii. Select 'file open' -- the dialog should open in the `%DATADIR\patterns` directory.
    - [ ] 2a-iii. Open `%DATADIR%\patterns\male_shirt.sm2d` pattern file with `%DATADIR%\measurements\individual\male_chest_102cm.smis` individual measurement file.
  - [ ] 2b. Run SeamlyMe from within Seamly2D
    - [ ] 2b-i. Select 'Edit Current' from the Measurements menu - the `%DATADIR\measurements\individual\male_chest_102cm.smis` file should open.
    - [ ] 2b-ii. Select 'File Open Individual' - the dialog should open in the `%DATADIR\measurements\individual` directory.
    - [ ] 2b-iii. Select 'File Open Multisize' - the dialog should open in the `%DATADIR\measurements\multisize` directory.
    - [ ] 2b-iv. Close SeamlyMe, returning focus to Seamly2D.
  - [ ] 2c. Run SeamlyLayout from within Seamly2D.
    - [ ] 2c-i. Confirm that the current pattern's `Piece mode` data is opened in the left canvas.
    - [ ] 2c-ii. Confirm that the 'piece mode' data was passed to SeamlyLayout as a stringified svg document (not as a svg file). Claude: check if `MainWindow::exportPiecesToSeamlyLayout()` writes `<pattern-basename>.pieces.svg` from the 'piece mode' data and SeamlyLayout reads this svg file, isn't passed as a stringified SVG document; Add a task to fix this. — 2026-09-01 static check: defect STILL PRESENT at `mainwindow.cpp:4153` (writes the `.pieces.svg` file, passes its path). Tasks already filed: Seamly2D.5, Layout.9.
    - [ ] 2c-iii. Close SeamlyLayout, returning focus to Seamly2D.
  - [ ] 2d. Close Seamly2D.
- [x] 3. Check if `%LOCALAPPDATA%\SeamlyLayout\output` directory was created. If exists add a task to stop creating the `%LOCALAPPDATA%\SeamlyLayout\` directory and its `output` subdirectory that stores log files, and start creating the `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs` directory to store SeamlyLayout log files (similar to the `%LOCALAPPDATA\Seamly\Seamly2D\logs` directory) — CONFIRMED 2026-09-01: the stray directory was created by SeamlyLayout's first run. Task already filed: Layout.10.
- [x] 4. Check Desktop shortcuts `Seamly2D.lnk`, `SeamlyMe.lnk`, `SeamlyLayout.lnk` for all three apps in `C:\Users\Public\Desktop` (default settings, `SEAMLYDESKTOPSHORTCUTS` on). — PASS: all three present.
- [x] 5. Check the logs in `%LOCALAPPDATA%\Seamly\Seamly2D\logs\` for additional errors. — PASS: `seamly2d-pid20632.log` has no error/warning lines.