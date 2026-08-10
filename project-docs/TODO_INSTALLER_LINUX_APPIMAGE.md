# TODO — Migrate SeamlyLayout into the Seamly2D structure

Tasks for migrating the SeamlyLayout app into the Seamly2D structure — where SeamlyMe and SeamlyLayout are callable from within Seamly2D and all three apps are distributed together for installation on a user's computer.

See `project-docs/PROJECT_PLAN.md` for full details. Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Check off all completed tasks & subtasks and move completed tasks to TODO_COMPLETED.md

Tasks in this file begin with `InstLinuxAppimage.`

## Task InstLinuxAppimage.1 — Update the ci.yml file and associated files and scripts to create a linux appimage containing seamly2d, seamlylayout, and seamlyme apps in a single qt runtime, with settings and config files in the same location; 

## Task InstLinuxAppimage.2 — Linux AppImage: create a plan for migrating existing seamly2d user data to the new seamly user data directory to let the user choose the `seamlyData` user-data directory (default `~/seamlyData`); update subtasks to implement this plan

The Linux AppImage is a single self-contained executable with no installer, so — as on macOS — the Windows install-time data-directory prompt maps to an in-app **first-run chooser**. migrate an existing `~/seamly2d` tree.

- [ ] Task InstLinuxAppimage.2.1 On first run (Linux/AppImage), show a directory-picker prompt for the user-data root, prefilled with `~/seamly`, accepting any mounted drive/path incl. external and cloud-synced mounts
- [ ] Task InstLinuxAppimage.2.2 Persist the chosen user-data root via the shared `paths/dataRoot` setting (Task 34); resolve all data subfolders under it; do not re-prompt on later runs
- [ ] Task InstLinuxAppimage.2.3 Migrate an existing `~/seamly2d` tree to the chosen root on first run (Task 34's migration)
- [ ] Task InstLinuxAppimage.2.4 Verify with the AppImage: the first-run chooser sets the root (incl. an external/cloud mount), data reads/writes there, and migration from `~/seamly2d` works — AppImage-packaging/real-run caveat as in Task 17
- [ ] Task InstLinuxAppimage.2.5 Document the AppImage first-run data-directory chooser in the repo docs
