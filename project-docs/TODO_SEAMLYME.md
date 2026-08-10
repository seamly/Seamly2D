# TODO — SeamlyMe app features

Tasks that add features to the SeamlyMe measurements app.

See `project-docs/PROJECT_PLAN.md` for full details. Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

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
