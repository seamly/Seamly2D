# TEST_INSTALLER_WIN_X64

Test plan for the Windows x64 Seamly MSI. Covers `packaging/windows/smsi.wxs`.

## Variable Names

The three names in the request are not real environment variables. Corrected below.

| Requested name | Status | Correct reference |
| --- | --- | --- |
| `%SEAMLYPROGRAMDIR%` | Not real | `INSTALLFOLDER` — MSI property. Default `%ProgramFiles%\SeamlyApps`. Recorded at `HKLM\SOFTWARE\Seamly\Seamly2D\InstallPath`. |
| `%SEAMLYUSERDATAROOT%` | Not real | `SEAMLYDATAROOT` — MSI property (raw path chosen). Default `<Documents>\SeamlyData`. Recorded value is `SEAMLYDATAROOTRECORDED`, stored at `HKLM\SOFTWARE\Seamly\Seamly2D\DataRoot`. Apps read it through `InstallerRecord::dataRoot()`. |
| `%SEAMLYAPPLICATIONDIR%` | Not real | `%LOCALAPPDATA%\Seamly\<AppName>\` — a real Windows variable plus a fixed subpath, from `QStandardPaths::AppConfigLocation`. `<AppName>` is `Seamly2D`, `SeamlyMe`, or `SeamlyLayout`. |

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`; always `C:Program FilesSeamlyApps`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`.

Known defect to watch for: an empty organization name can make Qt write settings under
`%APPDATA%\Unknown Organization\` instead of `%LOCALAPPDATA%\Seamly\<AppName>\`. See
`src/libs/vmisc/vcommonsettings.cpp`. Check for this stray folder in every verification pass.

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Baseline - Not installed | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Not installed

- [X] 1a. Run latest windows x64 installation .msi (with SeamlyLayout)
  - [X] 1a-i. default settings.

Non-default settings means at least: a non-default `%DATAROOT%`
parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

### B. Verification Suite

Run this suite after every test case in section A.

- [x] 1. Check the `%PROGRAMDIR%` location (default `%ProgramFiles%\SeamlyApps`).
- [x] 2. Check the `%DATAROOT%` location (default `C:\Users\<user>\Documents\SeamlyData`).
- [x] 3. Check the `%LOCALAPPDATA%\Seamly\<AppName>\` and `%APPDATA%\Seamly\<AppName>\` locations for Seamly2D, SeamlyMe, and SeamlyLayout.
- [x] 4. Check the registry.
  - [x] 4a. If applicable, confirm old-version entries were removed.
  - [x] 4b. Confirm the installed-version entries were added, under `HKLM\SOFTWARE\Seamly\Seamly2D`, `HKLM\SOFTWARE\Seamly\SeamlyMe`, and `HKLM\SOFTWARE\Seamly\SeamlyLayout`.
- [x] 5. Run Seamly2D, then close Seamly2D to install the user directories
- [x] 6. Check user-data location, directories, and files
  - [x] 6a. confirm that installed data is correct
    - [x] 6a-i. No duplicate directories
    - [x] 6a-ii. Directories created at the correct level below `%DATAROOT%`
    - [x] 6a-iii. `seamly2d.zip` was expanded into the correct directories
  - [x] 6b.if upgrading from previous non-SeamlyLayout version then:
    - [ ] 6b-i. confirm `%DATAROOT%\seamly2d.zip` exists and contains the old `seamly2d` user-data tree.
    - [ ] 6b-ii. Confirm `%DATAROOT%\seamly2d.zip` was expanded into the new `%DATAROOT%` directories.
- [x] 7. Check Seamly2D
  - [x] 7b. Run Seamly2D and open `%DATAROOT%\patterns\sample-pattern.sm2d`.
  - [x] 7c. Confirm `Application Preferences → File Paths` --> all paths should start with `%DATAROOT%` value.
- [x] 8. Check SeamlyMe
  - [x] 8a. Run SeamlyMe from within Seamly2D
  - [x] 8b. Open `%DATAROOT%\measurements\individual\sample-measurements-individual.smis` file, then close file
  - [x] 8c. Open `%DATAROOT%\measurements\multisize\sample-measurements-multisize.smms` file, then close file
  - [x] 8d. Close SeamlyMe
- [x] 9. Check SeamlyLayout
  - [x] 9a. Run SeamlyLayout from within Seamly2D
  - [x] 9b. Check that the sample pattern's `Piece mode` data was passed to SeamlyLayout as a stringified svg document (not as a svg file)
  - [x] 9c. Close SeamlyLayout.