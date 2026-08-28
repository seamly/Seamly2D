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
| 1 | Fresh installed | disabled | disabled | enabled |
| 2 | Previous version installed, no SeamlyLayout | disabled | disabled | enabled |
| 3 | Previous version installed, with SeamlyLayout | disabled | enabled | enabled |
| 4 | Same version installed, with SeamlyLayout | enabled | enabled | disabled |

### Case 1 — Fresh install

- [ ] Test.1.1 Run the tasks in `project-docs/TEST_INSTALLER_WIN_x64_Test_Case_1b-i.md`, update task status in the doc, & create new tasks in the doc for errors encountered
- [ ] Test.1.2 Run the tasks in `project-docs/TEST_INSTALLER_WIN_x64_Test_Case_1b-ii.md`, update task status in the doc, & create new tasks in the doc for errors encountered

### Case 2 — Previous version installed, no SeamlyLayout

- [ ] Test.2.1 Run the tasks in `project-docs/TEST_INSTALLER_WIN_x64_Test_Case_2c-i.md`, update task status in the doc, & create new tasks in the doc for errors encountered
- [ ] Test.2.2 Run the tasks in `project-docs/TEST_INSTALLER_WIN_x64_Test_Case_2c-ii.md`, update task status in the doc, & create new tasks in the doc for errors encountered

### Case 3 — Previous version installed, with SeamlyLayout

- [ ] Test.3.1 Run the tasks in `project-docs/TEST_INSTALLER_WIN_x64_Test_Case_3c-i.md`, update task status in the doc, & create new tasks in the doc for errors encountered
- [ ] Test.3.2 Run the tasks in `project-docs/TEST_INSTALLER_WIN_x64_Test_Case_3c-ii.md`, update task status in the doc, & create new tasks in the doc for errors encountered

### Case 4 — Same version installed, with SeamlyLayout

- [ ] Test.4.1 Run the tasks in `project-docs/TEST_INSTALLER_WIN_x64_Test_Case_4c-i.md`, update task status in the doc, & create new tasks in the doc for errors encountered
- [ ] Test.4.2 Run the tasks in `project-docs/TEST_INSTALLER_WIN_x64_Test_Case_4c-ii.md`, update task status in the doc, & create new tasks in the doc for errors encountered

