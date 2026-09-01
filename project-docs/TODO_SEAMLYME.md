# TODO — SeamlyMe app features

Tasks that add features to the SeamlyMe measurements app.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

## Task SeamlyMe.1: confirm Open Individual/Multisize/Template pickers default to the corresponding user measurement directories

Requested: the **Open Individual**, **Open Multisize**, and **Open Template** pickers should open to `<seamly_user_directory>\measurements\individual`, `…\measurements\multisize`, and the templates directory respectively. `<seamly_user_directory>` = the shared relocatable data root (Task 34).

**Current state — largely already implemented:** `TMainWindow::OpenIndividual()`/`OpenMultisize()`/`OpenTemplate()` (`src/app/seamlyme/tmainwindow.cpp:447-504`) already seed the dialog's initial directory from the settings getters:

- Open Individual → `getIndividualSizePath()` → default `<dataRoot>/measurements/individual` ✓
- Open Multisize → `getMultisizePath()` → default `<dataRoot>/measurements/multisize` ✓
- Open Template → `getTemplatePath()` → default **`<dataRoot>/templates`** (NOT `measurements/template`)

**Discrepancy to resolve:** the request lists the template dir as `measurements\template`, but the app's template path is `<dataRoot>/templates` (`getDefaultTemplatePath()`, `src/libs/vmisc/vcommonsettings.cpp:471`) — a sibling of `measurements/`, holding measurement *starter templates* seeded from the shipped `/tables/templates`. Decide whether to keep `<dataRoot>/templates` (current) or move measurement templates under `measurements/template`.

- [ ] SeamlyMe.1.1 Verify on a running build that Open Individual/Multisize/Template open at the expected directories (they already consume `get*Path()`); capture any case where they don't (e.g. the dir doesn't exist yet — `OpenIndividual()` mkpath/rmpath's a temp dir around the dialog)
- [ ] SeamlyMe.1.2 Resolve the template-location discrepancy: keep `<dataRoot>/templates`, or relocate to `<dataRoot>/measurements/template` with a `getDefaultTemplatePath()` default change + first-run migration (cross-platform; coordinate with Task 34)
- [ ] SeamlyMe.1.3 Ensure all three paths derive from the shared relocatable data root (Task 34) so they follow a user-configured/renamed `<seamly_user_directory>` instead of the hardcoded `~/seamly2d`
- [ ] SeamlyMe.1.4 Doxygen briefs + inline comments on any touched function(s)

## Task SeamlyMe.2 — SeamlyMe: default the Open dialog to the user measurements/individual directory

## Task SeamlyMe.3 — Installer: write SeamlyMe's desktop-shortcut flag to its own registry key

Found during MSI Test Case verification, step 5c (`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`). `SeamlyMeDesktopShortcutComponent`'s `RegistryValue` (`packaging/windows/smsi_shortcuts.wxs:75-80`) writes `DesktopShortcutSeamlyMe` under `HKLM\SOFTWARE\Seamly\Seamly2D` instead of `HKLM\SOFTWARE\Seamly\SeamlyMe`.

- [ ] SeamlyMe.3.1 Change the `RegistryValue`'s `Key` at `smsi_shortcuts.wxs:76` from `SOFTWARE\Seamly\Seamly2D` to `SOFTWARE\Seamly\SeamlyMe`
- [ ] SeamlyMe.3.2 Confirm `smsi_check_authoring.ps1:562` and `test_msi_install.ps1` still pass, and update either script if it asserts the old key
- [ ] SeamlyMe.3.3 Re-run MSI Test Case verification step 5c to confirm `HKLM\SOFTWARE\Seamly\SeamlyMe` carries `DesktopShortcutSeamlyMe`

## Task SeamlyMe.5 — write SeamlyMe log files to `%LOCALAPPDATA%\Seamly\SeamlyMe\logs`

Found during MSI Test Case verification, step B.2b-v (`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`, 2026-09-01). SeamlyMe writes no log files and creates no logs directory — `ApplicationME` has no equivalent of `Application2D::logDirPath()`/`beginLogging()` (`src/app/seamly2d/core/application_2d.cpp:605`). Seamly2D writes to `%LOCALAPPDATA%\Seamly\Seamly2D\logs`; SeamlyMe must mirror that pattern.

- [ ] SeamlyMe.5.1 Add log-directory resolution and logging startup to `ApplicationME`, mirroring `Application2D::logDirPath()`/`beginLogging()`: `AppLocalDataLocation` + `logs` → `%LOCALAPPDATA%\Seamly\SeamlyMe\logs` on Windows
- [ ] SeamlyMe.5.2 Re-run MSI Test Case verification step B.2b-v to confirm the directory and a log file appear after a SeamlyMe run
- [ ] SeamlyMe.5.3 Doxygen briefs + inline comments on any touched function(s)

## Task SeamlyMe.4 — write a persisted `[paths]` section to `qt6_seamlyme.ini`

Found during MSI Test Case verification, step 6b (`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`). `qt6_seamlyme.ini` has no `[paths]` section. `VCommonSettings` (shared with Seamly2D) already exposes `getIndividualSizePath()`/`getMultisizePath()`/`getTemplatePath()` under the `paths/individual_size_measurements`, `paths/multi_size_measurements`, and `paths/templates` keys (`src/libs/vmisc/vcommonsettings.cpp:86-93`), but nothing in SeamlyMe ever calls the matching setters, so QSettings never writes the section — the paths only resolve correctly at runtime through each getter's default fallback. Related to Task SeamlyMe.1 (same getters, dialog defaults).

- [ ] SeamlyMe.4.1 Have SeamlyMe persist `individual_size_measurements`, `multi_size_measurements`, and `templates` under `[paths]` in `qt6_seamlyme.ini`, each defaulting to `%DATADIR%\measurements\individual`, `%DATADIR%\measurements\multisize`, and `%DATADIR%\templates` respectively (call the existing `set*Path()` setters on first run/save, matching how Seamly2D persists its own `[paths]` section)
- [ ] SeamlyMe.4.2 Re-run MSI Test Case verification step 6b to confirm the persisted section exists and every value begins with `%DATADIR%`
- [ ] SeamlyMe.4.3 Doxygen briefs + inline comments on any touched function(s)
