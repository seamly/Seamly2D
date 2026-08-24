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

### Case 2 — Previous version installed, no SeamlyLayout

- [ ] 2a. Uninstall Seamly (any and all versions deteected)
- [ ] 2b. Run installation for previous Seamly version (no SeamlyLaout) from `C:\Users\susan\Downloads\seamly2d-windows.zip` with default settings.
- [ ] 2c. Run latest windows x64 installation .msi (with SeamlyLayout)
  - [ ] 2c-i. Default settings.

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%`
parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

## B. Verification Suite

Run this suite after every test case in section A.

- [ ] 1. Check the `%PROGRAMsDIR%` location (default `C:\Program Files\SeamlyApps`).
- [ ] 2. Check the `%DATAROOT%` location (default `<Documents>\SeamlyData`).
- [ ] 3. Check the `%LOCALAPPDATA%\Seamly\<AppName>\` locations for Seamly2D, SeamlyMe, and SeamlyLayout.
- [ ] 4. Check the registry.
  - [ ] 4a. If applicable, confirm old-version entries were removed.
  - [ ] 4b. Confirm the installed-version entries were added, under `HKLM\SOFTWARE\Seamly\Seamly2D`.
- [ ] 5. Check Seamly2D.
  - [ ] 5a. Run Seamly2D.
  - [ ] 5b. Open `%DATAROOT%\patterns\pattern.sm2d`.
  - [ ] 5c. Check Application Preferences → File Paths.
    - [ ] 5c-i. Confirm the file paths start with `%DATAROOT%`.
- [ ] 6. If applicable, check user-data migration.
  - [ ] 6a. Confirm `%DATAROOT%\seamly2d.zip` exists. Confirm it contains the old `seamly2d`
     user-data tree and files.
  - [ ] 6b. Confirm `%DATAROOT%\seamly2d.zip` was expanded into the new `%DATAROOT%` directories.
    - [ ] 6b-i. No duplicate directories.
    - [ ] 6b-ii. Directories created at the correct level below `%DATAROOT%`.
    - [ ] 6b-iii. Old `seamly2d` files expanded into the correct directories.
- [ ] 7. Check SeamlyMe.
  - [ ] 7a. Run SeamlyMe. Open a `.smis` file from `%DATAROOT%\measurements\individual`. Close SeamlyMe.
  - [ ] 7b. Run SeamlyMe. Open a `.smis` file from `%DATAROOT%\measurements\multisize`. Close SeamlyMe.
- [ ] 8. Check SeamlyLayout.
  - [ ] 8a. Run SeamlyLayout. Import a Seamly `.svg` layout file from `%DATAROOT%\layouts`. Close SeamlyLayout.
