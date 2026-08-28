# TEST_INSTALLER_WIN_X64

Test plan for the Windows x64 Seamly MSI. Covers `scripts/packaging/windows/smsi.wxs`.

## Variable Names

The three names in the request are not real environment variables. Corrected below.

| Requested name | Status | Correct reference |
| --- | --- | --- |
| `%SEAMLYPROGRAMDIR%` | Not real | `INSTALLFOLDER` — MSI property. Default `C:\Program Files\SeamlyApps`. Recorded at `HKLM\SOFTWARE\Seamly\Seamly2D\InstallPath`. |
| `%SEAMLYUSERDATAROOT%` | Not real | `SEAMLYDATAROOT` — MSI property (raw path chosen). Default `<Documents>\SeamlyData`. Recorded value is `SEAMLYDATAROOTRECORDED`, stored at `HKLM\SOFTWARE\Seamly\Seamly2D\DataRoot`. Apps read it through `InstallerRecord::dataRoot()`. |
| `%SEAMLYAPPLICATIONDIR%` | Not real | `%LOCALAPPDATA%\Seamly\<AppName>\` — a real Windows variable plus a fixed subpath, from `QStandardPaths::AppConfigLocation`. `<AppName>` is `Seamly2D`, `SeamlyMe`, or `SeamlyLayout`. |

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`.

Known defect to watch for: an empty organization name can make Qt write settings under
`%APPDATA%\Unknown Organization\` instead of `%LOCALAPPDATA%\Seamly\<AppName>\`. See
`src/libs/vmisc/vcommonsettings.cpp`. Check for this stray folder in every verification pass.

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Not installed | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 4 — Same version installed, with SeamlyLayout

- [ ] 4a. uninstall Seamly (any and all versions detected) 
- [ ] 4b. Run latest windows x64 installation .msi (with SeamlyLayout) with default settings
- [ ] 4c. Re-run latest windows x64 installation .msi (with SeamlyLayout)
  - [ ] 4c-i. Run Repair.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%`
parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

## B. Verification Suite

Run this suite after every test case in section A.

- [ ] 1. Check the `%PROGRAMDIR%` location (default `C:\Program Files\SeamlyApps`).
- [ ] 2. Check the `%DATAROOT%` location (default `C:\Users\<user>\Documents\SeamlyData`).
- [ ] 3. Check the `%LOCALAPPDATA%\Seamly\<AppName>\` and `%APPDATA%\Seamly\<AppName>\` locations for Seamly2D, SeamlyMe, and SeamlyLayout.
- [ ] 4. Check the registry.
  - [ ] 4a. If applicable, confirm old-version entries were removed.
  - [ ] 4b. Confirm the installed-version entries were added, under `HKLM\SOFTWARE\Seamly\Seamly2D`, `HKLM\SOFTWARE\Seamly\SeamlyMe`, and `HKLM\SOFTWARE\Seamly\SeamlyLayout`.
- [ ] 5. Run Seamly2D, then close Seamly2D to install the user directories
- [ ] 6. Check user-data location, directories, and files
  - [ ] 6a. confirm that installed data is correct
    - [ ] 6a-i. No duplicate directories
    - [ ] 6a-ii. Directories created at the correct level below `%DATAROOT%`
    - [ ] 6a-iii. `seamly2d.zip` was expanded into the correct directories
  - [ ] 6b.if upgrading from previous non-SeamlyLayout version then:
    - [ ] 6b-i. confirm `%DATAROOT%\seamly2d.zip` exists and contains the old `seamly2d` user-data tree.
    - [ ] 6b-ii. Confirm `%DATAROOT%\seamly2d.zip` was expanded into the new `%DATAROOT%` directories.
- [ ] 7. Check Seamly2D
  - [ ] 7b. Run Seamly2D and open `%DATAROOT%\patterns\sample-pattern.sm2d`.
  - [ ] 7c. Confirm `Application Preferences → File Paths` --> all paths should start with `%DATAROOT%` value.
- [ ] 8. Check SeamlyMe
  - [ ] 8a. Run SeamlyMe from within Seamly2D
  - [ ] 8b. Open `%DATAROOT%\measurements\individual\sample-measurements-individual.smis` file, then close file
  - [ ] 8c. Open `%DATAROOT%\measurements\multisize\sample-measurements-multisize.smms` file, then close file
  - [ ] 8d. Close SeamlyMe
- [ ] 9. Check SeamlyLayout
  - [ ] 9a. Run SeamlyLayout from within Seamly2D
  - [ ] 9b. Check that the sample pattern's `Piece mode` data was passed to SeamlyLayout as a stringified svg document (not as a svg file)
  - [ ] 9c. Close SeamlyLayout.