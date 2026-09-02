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

- [ ] 0. Relaunch this shell elevated (Administrator) before any step below.
- [ ] 1a. Uninstall Seamly (any and all versions detected) using `packaging\windows\test_reset_environment.ps1`.
  - [ ] 1a-i. Confirm that the %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly\Seamly2D, AppData\Local\Seamly\SeamlyMe, AppData\Local\Seamly\SeamlyLayout, desktop shortcuts, and registry keys have been removed.
- [ ] 1b. Install Seamly apps from `packaging\windows\seamly-msi\x64\seamly-x64.msi` with Default settings, through the **wizard**, from the elevated shell:
  `msiexec /i seamly-x64.msi /norestart /l*v "%TEMP%\seamly_install.log"`
  - [ ] 1b-i. Do **not** pass `/quiet`. A silent install builds no dialogs, so it cannot show a dialog defect (`MSI1b.1`).
  - [ ] 1b-ii. Accept every default page: program directory, user-data folder, desktop shortcuts on.
  - [ ] 1b-iii. Confirm the log ends with `Installation success or error status: 0` and carries no `Error 2826` line.

## B. Verification Suite

Run this suite after every test case in section A.

- [ ] 0. Check the directories and files
  - [ ] 0a. Confirm these directories and files exist:
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
    |_label templates
    |_layouts
    |_measurements
    |_measurements\individual
    |  |_male_chest_102cm.smis
    |_measurements\multisize
    |_patterns
    |  |_male_shirt.sm2d
    |_templates
  - [ ] 0b. Check the contents of the .ini files:
    - [ ] 0b-i. qt6_common.ini should contain:
      - [ ] 0b-i1. "[paths]
dataRoot=%DATAROOT%
individual_size_measurements=%DATAROOT%/measurements/individual
multi_size_measurements=%DATAROOT%/measurements/multisize
templates=%DATAROOT%/templates
bodyscans=%DATAROOT%/bodyscans"
      - [ ] 0b-i2. "[notices] firstRunDataNotice=pending"
    - [ ] 0b-ii. qt6_seamly2d.ini should contain:
    "[paths]
pattern=%DATAROOT%/patterns
layout=%DATAROOT%/layouts
labels=%DATAROOT%/label templates
images=%DATAROOT%/images
backups=%DATAROOT%/backups
seamlyLayoutApp=%PROGRAMDIR%/SeamlyApps/SeamlyLayout.exe"
    - [ ] 0b-iii. qt6_seamly2d.ini should be empty
    - [ ] ob-iv. qt6_seamlylayout.ini should contain:
    "[General]
input_directory=%DATAROOTROOT%/layouts
layout_directory=%DATAROOTROOT%/layouts
preferences_directory=%LOCALAPPDATA%/Seamly/SeamlyLayout/preferences
settings_directory=%LOCALAPPDATA%/Seamly/SeamlyLayout/settings
settings_file=%LOCALAPPDATA%/Seamly/SeamlyLayout/settings/default_settings.json
preferences_file=%LOCALAPPDATA%/Seamly/SeamlyLayout/preferences/default_preferences.json
dxf_viewer_path=https://sharecad.org
pdf_viewer_path=
png_viewer_path=
projector_path=https://patternprojector.com
data_root=%DATAROOT%"
  - [ ] 0c. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`) contains `seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe`, `pdftops.exe`, `QtWebEngineProcess.exe`, `vc_redist.x64.exe`.
  - [ ] 0d. Confirm no duplicate directories.
  - [ ] 0e. if upgrading from previous non-SeamlyLayout version then:
    - [ ] 0e-i. confirm `%DATAROOT%\seamly2d.zip` exists.
    - [ ] 0e-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories.
- [ ] 1. Check the registry keys:
  - [ ] 1a. If not a fresh install then confirm old-version entries were removed.
  - [ ] 1b. Confirm that the installed-version program entries were added for each app under `HKLM\SOFTWARE\Seamly\<application>`, each with matching `InstallPath` and `DisplayVersion`.
  - [ ] 1c. Confirm that installed-version data entries were added with matching `DataRoot` and `DataParent` values.
  - [ ] 1d. Confirm that each app's desktop-shortcut flag sits under its OWN key: `DesktopShortcutSeamly2D` in `HKLM\SOFTWARE\Seamly\Seamly2D`, `DesktopShortcutSeamlyMe` in `HKLM\SOFTWARE\Seamly\SeamlyMe`, `DesktopShortcutSeamlyLayout` in `HKLM\SOFTWARE\Seamly\SeamlyLayout` (SeamlyMe.3, Layout.7).
- [ ] 2. Check apps - **needs a human at the keyboard for most steps**
  - [ ] 2a. Run Seamly2D
    - [ ] 2a-i. first run scripted: the "Seamly data moved" notice appears once, closes, then the main window appears
    - [ ] 2a-ii. check if `qt6_common.ini` contains  "[notices] firstRunDataNotice=shown";
    - [ ] 2a-iii. Select 'file open' -- the dialog should open in the `%DATAROOT\patterns` directory.
    - [ ] 2a-iv. Open `%DATAROOT%\patterns\male_shirt.sm2d` pattern file with `%DATAROOT%\measurements\individual\male_chest_102cm.smis` individual measurement file.
    - [ ] 2a-v. Check if directory exists: `%LOCALAPPDATA%\Seamly\Seamly2D\logs`
  - [ ] 2b. Check SeamlyMe from within Seamly2D
    - [ ] 2b-i. Select 'File Open Individual' - the dialog should open in the `%DATAROOT\measurements\individual` directory.
    - [ ] 2b-ii. Select 'File Open Multisize' - the dialog should open in the `%DATAROOT\measurements\multisize` directory.
    - [ ] 2b-iii. Select 'File Open Templates' - the dialog should open in the `%DATAROOT\templates` directory.
    - [ ] 2b-iv. Select 'Edit Current' from the Measurements menu - the `%DATAROOT%\measurements\individual\male_chest_102cm.smis` file should open.
    - [ ] 2b-v. Check if directory exists: `%LOCALAPPDATA%\Seamly\SeamlyMe\logs`
    - [ ] 2b-vi. Close SeamlyMe, returning focus to Seamly2D.
  - [ ] 2c. Run SeamlyLayout from within Seamly2D.
    - [ ] 2c-i. Visually confirm that the current pattern's `Piece mode` data is opened in the left canvas.
    - [ ] 2c-ii. Check if `MainWindow::exportPiecesToSeamlyLayout()` passes 'piece mode' data to SeamlyLayout as a stringified SVG document in a variable, not as a svg file from harddrive.
    - [ ] 2c-iii. Check if directories exist: `%LOCALAPPDATA%\Seamly\SeamlyLayout\cache`, `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs`
    - [ ] 2c-iv. Close SeamlyLayout, returning focus to Seamly2D.
  - [ ] 2d. Close Seamly2D.
- [ ] 3. Check SeamlyLayout's log directory (Layout.10):
  - [ ] 3a. Confirm `%LOCALAPPDATA%\SeamlyLayout` does NOT exist.
  - [ ] 3b. Confirm the session log is `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs\log_<timestamp>.txt`, matching `%LOCALAPPDATA%\Seamly\Seamly2D\logs`.
- [ ] 4. Check Desktop shortcuts `Seamly2D.lnk`, `SeamlyMe.lnk`, `SeamlyLayout.lnk` for all three apps in `C:\Users\Public\Desktop` (default settings, `SEAMLYDESKTOPSHORTCUTS` on).
- [ ] 5. Check the logs in `%LOCALAPPDATA%\Seamly\Seamly2D\logs\` for additional errors.
