# TEST_MSI_WIN_X64_Test_Case_1b-i.md

Test plan for the Windows x64 Seamly MSI. Covers `scripts/packaging/windows/smsi.wxs`.

This document uses two placeholders as shorthand. Neither is a real environment variable.

- `%PROGRAMDIR%` stands for the resolved `INSTALLFOLDER`.
- `%DATAROOT%` stands for the resolved `SEAMLYDATAROOTRECORDED`.

Known defect to watch for: an empty organization name can make Qt write settings under `%APPDATA%\Unknown Organization\` instead of `%LOCALAPPDATA%\Seamly\<AppName>\`. See `src/libs/vmisc/vcommonsettings.cpp`. Check for this stray folder in every verification pass.

## A. MSI Test Case Matrix

| Case | Seamly state | Repair | Uninstall | Install |
| --- | --- | --- | --- | --- |
| 1 | Fresh install | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Fresh install

- [ ] 0. Relaunch this shell elevated (Administrator) before any step below.
- [ ] 1a. Uninstall Seamly (any and all versions detected) using `/test_reset_environment.ps1`
  - [ ] 1a-i. Confirm that %PROGRAMROOT, %DATAROOT, AppData\Roaming\Seamly, AppData\Local\Seamly, desktop shortcuts, and registry keys have been removed
- [ ] 1b. Install Seamly apps using `scripts\seamly-msi\x64\seamly-x64.msi` with Default settings via `msiexec /i seamly-x64.msi /quiet /norestart`

Non-default settings means at least: a non-default `%PROGRAMDIR%`, a non-default `%DATAROOT%` parent, and desktop shortcuts turned off (`SEAMLYDESKTOPSHORTCUTS=0`).

## B. Verification Suite

Run this suite after every test case in section A.

- [x] 1. Run Seamly2D then close Seamly2D to install the user directories
  - Confirmed 2026-08-28. Evidence: `%LOCALAPPDATA%\Seamly\Seamly2D\logs\seamly2d-pid22200.log` shows a full startup sequence and `qt6_common.ini` / `qt6_seamly2d.ini` exist.
- [x] 2. Check the program directory `%PROGRAMDIR%` exists (default `C:\Program Files\SeamlyApps`)
  - Confirmed 2026-08-28.
- [ ] 3. Check user-data location (default `C:\Users\<user>\Documents\SeamlyData\`), subdirectories, and files:
  - [x] 3a. No duplicate directories
    - Confirmed 2026-08-28. Only one `SeamlyData` directory under `Documents`.
  - [x] 3b. Subdirectories `backups`, `bodyscans`, `images`, `label templates`, `layouts`,  `measurements\individual`, `measurements\multisize`, `patterns`, and `templates` are created at the correct level below `%DATAROOT%`
    - Confirmed 2026-08-28. All nine subdirectories present at the correct level.
  - [x] 3c. if upgrading from previous non-SeamlyLayout version then: **N/A — Case 1 fresh install.**
    - [x] 3c-i. confirm `%DATAROOT%\seamly2d.zip` exists — **N/A, fresh install.**
    - [x] 3c-ii. confirm that `seamly2d.zip` files were extracted into the correct subdirectories — **N/A, fresh install.**
  - [ ] 3d. check that %DATADIR\patterns\male_shirt.sm2d exists
    - **FAILED 2026-08-28.** `%DATADIR%\patterns\` is empty. See defect note below.
  - [ ] 3e. check that %DATADIR\measurements\individual\male_chest_102cm.smis exists
    - **FAILED 2026-08-28.** `%DATADIR%\measurements\individual\` is empty. See defect note below.
- [x] 4. Check the user application directories:
  - [x] 4a. `%LOCALAPPDATA%\Seamly\<AppName>\` directories exist for Seamly2D, SeamlyMe, and SeamlyLayout.
    - Confirmed 2026-08-28. All three present.
  - [x] 4b. `%APPDATA%\Seamly\qt6_common.ini` file exists
    - Confirmed 2026-08-28.
    - [x] 4b-i. Confirm all paths in qt6_common.ini start with `%DATAROOT%` value.
      - Confirmed: file contains one key, `dataRoot=c:/Users/susan/Documents/SeamlyData`, matching `%DATAROOT%`.
- [x] 5. Check the registry keys:
  - [x] 5a. If not a fresh install then confirm old-version entries were removed. **N/A — Case 1 fresh install.**
  - [x] 5b. Confirm that the installed-version program entries were added, under `HKLM\SOFTWARE\Seamly\Seamly2D`, `HKLM\SOFTWARE\Seamly\SeamlyMe`, and `HKLM\SOFTWARE\Seamly\SeamlyLayout`
    - Confirmed 2026-08-28. `InstallPath`, `DisplayVersion` (26.8.28.1068) present under all three keys.
  - [x] 5c. Confirm that installed-version data entries were added
    - Confirmed 2026-08-28. `DataRoot` and `DataParent` present under all three keys.
- [ ] 6. Check the apps — **needs a human at the keyboard; not run in this pass.**
  - [ ] 6a. Check Seamly2D and SeamlyMe
    - [ ] 6a-i. Open `%DATADIR%\patterns\male_shirt.sm2d` pattern file with `%DATADIR%\measurements\individual\male_chest_102cm.smis` individual measurement file.
    - [ ] 6a-ii. Run SeamlyMe from within Seamly2D  --> prompt human to select 'Edit Current' from the Measurements menu in Seamly2D
    - [ ] 6a-iv. Close SeamlyMe, returning focus to Seamly2D
  - [ ] 6b. Check SeamlyLayout
    - [ ] 6b-i. Run SeamlyLayout from within Seamly2D --> prompt human to select the SeamlyLayout icon in Seamly2D
    - [ ] 6b-ii. Confirm that the current pattern's `Piece mode` data was passed to SeamlyLayout as a stringified svg document (not as a svg file) --> prompt human to confirm, or use code-level confirmation of the IPC payload shape
    - [ ] 6b-iv. Close SeamlyLayout, returning focus to Seamly2D
  - [ ] 6c. Close Seamly2D
- [x] 7. Check the logs in `%LOCALAPPDATA%\Seamly\<AppName>\logs\` for additional errors
  - Confirmed 2026-08-28. Only `Seamly2D\logs\` has log files (SeamlyMe and SeamlyLayout were not yet run standalone). Both `seamly2d-pid19244.log` and `seamly2d-pid22200.log` contain only `INFO` entries, no `ERROR`.

## C. Defects Found — 2026-08-28

- [ ] D1. `%DATADIR%\patterns\male_shirt.sm2d` and `%DATADIR%\measurements\individual\male_chest_102cm.smis` are missing after fresh install + first run (fails 3d, 3e).
  - Root cause: the installed MSI (`seamly-x64.msi`, version `26.8.28.1068`, built 2026-08-28 17:49:21) predates commit `76b890f1ac` ("Seed the patterns folder from bundled samples on first run", authored 2026-08-28 21:41:51). `VSettings::SeedSamplePatterns()` is not in the running binary.
  - Fix: rebuild `seamly-x64.msi` from current `run-seamlyLayout`, reinstall, and re-run steps 3d and 3e.
- [ ] D2. `VSettings::SeedSamplePatterns()` (`src/libs/vmisc/vsettings.cpp`) only copies `*.sm2d` pattern files. It does not seed sample measurement files, so step 3e (`male_chest_102cm.smis`) will still fail even after D1 is fixed.
  - `male_chest_102cm.smis` is already bundled at `%PROGRAMDIR%\samples\measurements\individual\male_chest_102cm.smis` (source: `src/app/share/samples/measurements/individual/male_chest_102cm.smis`), so no new sample file is needed — only the seeding logic is missing.
  - Reason: on first run, Seamly2D must copy `male_chest_102cm.smis` into `%DATADIR%\measurements\individual\`. A standard user has no write access to `%PROGRAMDIR%\SeamlyApps\samples\`, so Seamly2D cannot create backup or temporary files there when the user opens the bundled copy directly. This is the same constraint that motivated `SeedSamplePatterns()` for `.sm2d` files.
  - Fix: add an equivalent seeding step for `%PROGRAMDIR%\samples\measurements\individual\*.smis` (and `multisize\*.smms`) into `%DATAROOT%\measurements\individual\` (and `\multisize\`), skipping files that already exist. Decide whether this belongs in `Application2D::initOptions()` alongside the pattern seeding, or in a shared location `SeamlyMe` can also call.
  - **Code fix landed 2026-08-28** on `task-seed-sample-measurements`: `VSettings::SeedSampleMeasurements()` (`src/libs/vmisc/vsettings.{h,cpp}`), called from `Application2D::initOptions()` for both `measurements/individual` (`*.smis`) and `measurements/multisize` (`*.smms`). See Task Seamly2D.3 in `TODO_COMPLETED.md`. Still needs D1 (an MSI rebuilt from this commit) before 3d/3e can be re-verified on a real machine.
