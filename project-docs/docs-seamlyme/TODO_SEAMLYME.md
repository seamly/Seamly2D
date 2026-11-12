# TODO — SeamlyMe app features

Tasks that add features to the SeamlyMe measurements app.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

## Required for SeamlyLayout

### [ ] Task SeamlyMe.1: confirm Open Individual/Multisize/Template pickers default to the corresponding user measurement directories

Requested: the **Open Individual**, **Open Multisize**, and **Open Template** pickers should open to `<seamly_user_directory>\measurements\individual`, `…\measurements\multisize`, and the templates directory respectively. `<seamly_user_directory>` = the shared relocatable data root (Task 34).

**Current state — largely already implemented:** `TMainWindow::OpenIndividual()`/`OpenMultisize()`/`OpenTemplate()` (`src/app/seamlyme/tmainwindow.cpp:447-504`) already seed the dialog's initial directory from the settings getters:

- Open Individual → `getIndividualSizePath()` → default `<dataRoot>/measurements/individual` ✓
- Open Multisize → `getMultisizePath()` → default `<dataRoot>/measurements/multisize` ✓
- Open Template → `getTemplatePath()` → default **`<dataRoot>/templates`** (NOT `measurements/template`)

**Template location retained:** use `<dataRoot>/templates`, a sibling of `measurements/`, for measurement starter templates seeded from shipped `/tables/templates`. Keeping this location preserves existing settings and avoids an unnecessary migration.

- [ ] SeamlyMe.1.1 Verify on a running build that Open Individual/Multisize/Template open at the expected directories (they already consume `get*Path()`); capture any case where they don't (e.g. the dir doesn't exist yet — `OpenIndividual()` mkpath/rmpath's a temp dir around the dialog)
- [x] SeamlyMe.1.2 Keep `<dataRoot>/templates`; preserve the existing default and shipped-template location.
- [x] SeamlyMe.1.3 All three paths follow the configured/renamed shared data root. Pickers retain newly created start folders and report directory-creation failures. `TST_DataRoot`: 29 passed, 0 failed.
- [x] SeamlyMe.1.4 Updated briefs for touched functions.

### [x] Task SeamlyMe.2 — default the Open dialog to the user measurements/individual directory

### [ ] Task SeamlyMe.4 — write a persisted `[paths]` section to `qt6_seamlyme.ini`

Found during MSI Test Case verification, step 6b (`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`). `qt6_seamlyme.ini` has no `[paths]` section. `VCommonSettings` (shared with Seamly2D) already exposes `getIndividualSizePath()`/`getMultisizePath()`/`getTemplatePath()` under the `paths/individual_size_measurements`, `paths/multi_size_measurements`, and `paths/templates` keys (`src/libs/vmisc/vcommonsettings.cpp:86-93`), but nothing in SeamlyMe ever calls the matching setters, so QSettings never writes the section — the paths only resolve correctly at runtime through each getter's default fallback. Related to Task SeamlyMe.1 (same getters, dialog defaults).

- [ ] SeamlyMe.4.1 Have SeamlyMe persist `individual_size_measurements`, `multi_size_measurements`, and `templates` under `[paths]` in `qt6_seamlyme.ini`, each defaulting to `%DATADIR%\measurements\individual`, `%DATADIR%\measurements\multisize`, and `%DATADIR%\templates` respectively (call the existing `set*Path()` setters on first run/save, matching how Seamly2D persists its own `[paths]` section)
- [ ] SeamlyMe.4.2 Verify that qt6_seamlyme.ini' contains [paths] with persistent directory values.

### [ ] Task SeamlyMe.7 - Measurement menu - Add a 'Pattern Measurements' option to display the measurements in use by the current pattern.

- [ ] The 'Body Measurements used in Pattern' option displays a measurement dialog that has columns for measurement name, value, formula, diagram, etc., and has options to search, etc., with these differences:
- lists only the body measurements that are utilized in the currently option pattern
- the user cannot add or delete body measurements 
- the user cannot cannot the order of the body measurements
- the user can edit measurement values but cannot set any measurement values to 0 or less than 0 or blank
- any listed measurements with a 0 or negative or blank value is displayed in red text