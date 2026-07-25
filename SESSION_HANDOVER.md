# Session handover

## Current task: Task 30 — upgrade SeamlyLayout to Qt 6.11.1 (match the parent apps)

**Date:** 2026-07-25. **Branch:** `run-seamlyLayout`, **1 commit ahead of origin** (`e769a55e10` local vs `86685265b2` on origin).

**Status:** Task 30's implementation is **complete and merged**; its final *verify* subtask is **blocked** on a machine-setup gap (Task 44). Two follow-up tasks were implemented and landed along the way; three more were logged but not fixed.

### Commits made this session

| Commit | On origin? | What |
|---|---|---|
| `5d448014c9` | **yes** | Task 30 — Qt 6.11.1 across the family, MSI collapsed to one shared Qt runtime. Also carries the `Seamly2D.pro` → `Seamly.pro` rename made in parallel. |
| `86685265b2` | **yes** | Task 47 — pin the Qt kit for Cargo so `cxx-qt-build` cannot pick Design Studio's Qt. |
| `e769a55e10` | **no — local only** | `scripts/sb.ps1` (build the whole family), `build.ps1 -NoRun`, the `app.pro` SUBDIRS comment, TODO updates incl. Task 48. |

Working tree: `src/app/app.pro` is modified (wording edits made after the commit). Nothing else pending.

### What Task 30 changed

- **Qt pin** — `qt_frontend/CMakeLists.txt` `find_package(Qt6 6.11.1 …)` + `qt_standard_project_setup(REQUIRES 6.11.1)`; versioned QML imports bumped to `6.11` across all 10 `.qml` files.
- **One shared Qt runtime in the MSI** — `seamly-family.wxs` drops `SEAMLYLAYOUTFOLDER` and installs all three exes in `INSTALLFOLDER`; `smsi.ps1` deploys `windeployqt6` output into the parents' staging tree and drops the `layout\` tree and the `LayoutStagingDir` define; `windows-msi.yml` installs **one** Qt kit instead of two (the ordering dance and `QT_LAYOUT_DIR` are gone; `qt-modules` is now a matrix field).
- **No hard-coded Qt patch versions in build scripts** — `build.ps1` and `sd.ps1` pick the newest `msvc2022_64` kit meeting the 6.11.1 minimum; `smsi.ps1`'s `Find-WinDeployQt6` reads `CMAKE_PREFIX_PATH` from SeamlyLayout's `CMakeCache.txt` so the deployed runtime always matches the exe. (Closes two Task 31 subtasks.)
- **`locateSeamlyLayout()` unchanged** — it already checks the flat layout first. Its subdirectory branch is kept as a fallback for installs made by a pre-Task-30 MSI; only doc comments and test comments were reworded.
- **CI** — `seamlylayout-ci.yml` `QT_VERSION: '6.11.1'`. Merging it into `ci.yml` was **evaluated and deliberately declined**: the Qt pin was the original reason for the split, but the differing build systems (CMake/Ninja + Cargo vs qmake) and path filters still justify it. Rationale recorded in the workflow header, `README_WORKFLOWS.md` and `.github/README-BUILDS.md`.

### What is verified (on Qt 6.11.1)

- `cargo test --workspace` — **251 passed**, 0 failed.
- All four Qt frontend ctest suites — **107 passed**, 0 failed, 1 skipped (AdjustScene 26, AdjustController 7, PreferencesModel 48, SettingsModel 26).
- `cargo clean -p cxxqt_bridge` + rebuild — **CXX-Qt 0.7.3 / cxx-qt-build compile clean against Qt 6.11.1**; a CMake configure reports `CXX-Qt Found crate(s): cxxqt_bridge` and `Using Corrosion as a subdirectory`, so the Corrosion integration is fine too.
- `scripts/sd.ps1` — parent debug build, exit 0, against the auto-detected 6.11.1 kit.
- `scripts/sb.ps1 -SkipLayout` — parent release build, exit 0.

> The ctest suites were built in a throwaway directory from a **temporary** local edit that dropped only the `WebEngineQuick` component (none of the four suites links WebEngine). The edit was reverted; `git diff` on `CMakeLists.txt` showed only the intended Task 30 changes before commit.

### What is NOT verified — and why

Task 30's last subtask is still `[ ]`. **Blocked on Task 44.** Not yet exercised: the full `SeamlyLayout.exe` build, running the app, the QML/WebEngine load path (incl. the bumped `import QtWebEngine 6.11`), the seamly2d → seamlyLayout handoff, and the MSI rebuild + single-shared-runtime install check.

## Blockers — read these before resuming

### Task 44 — local Qt kit is missing `qtwebchannel` + `qtpositioning`

`C:\Qt\6.11.1\msvc2022_64` has Qt WebEngine but **not** the two modules `Qt6WebEngineCore` depends on, so configuring SeamlyLayout fails before anything compiles:

```text
Qt6WebEngineQuick could not be found because dependency Qt6WebEngineCore could not be found.
CMake Error at CMakeLists.txt:41 (find_package): Failed to find required Qt component "WebEngineQuick".
```

Fix: Qt Maintenance Tool → add **Qt WebChannel** and **Qt Positioning** to the 6.11.1 `msvc2022_64` kit. This is a machine-setup gap, **not** a Qt 6.11 incompatibility — CI already installs all three modules explicitly. There is currently **no SeamlyLayout release build on disk** (the stale Qt 6.10.1 one was deleted).

### Task 48 — the parent release tree in `build/` is currently BROKEN

`win32-msvc` post-link in `seamly2d.pro:371`, `seamlyme.pro:252` and `Seamly2DTest.pro:212` runs a **bare `windeployqt`**, resolved from `PATH`. On this machine that is Qt Design Studio's reduced kit, which is **Qt 6.8.7**. So after a clean `sb.ps1` run the exes are linked against 6.11.1 but `build/src/app/*/bin/Qt6Core.dll` reports **6.8.7.0**. Qt's binary compatibility is forward-only, so those exes will not run, and `smsi.ps1` would package the mismatch into the MSI.

The fix already exists in the same files: the `win32-arm64-msvc` branch uses `qtPrepareTool(WINDEPLOYQT, windeployqt)`, which resolves from `$$[QT_INSTALL_BINS]` rather than `PATH`. The x64 branch never got it. **Not fixed** — it touches core `.pro` files that drive release CI, so it was left for an explicit decision. CI is unaffected (runners have no Design Studio).

**Consequence:** rebuild `build/` after fixing Task 48 before trusting any locally built MSI.

## New tasks logged in `TODO_MIGRATE.md`

| Task | Status | Summary |
|---|---|---|
| **44** | open, **blocking** | Install `qtwebchannel` + `qtpositioning`; then close Task 30's verify subtask and Task 31's rebuild subtasks. |
| **45** | open, cosmetic | Stale `C:\Qt\6.10.1` paths in `.claude/settings.json` and `settings.local.json` allowlists. |
| **46** | open | `sd.ps1` silently reuses stale qmake sub-Makefiles after a Qt change (`if not exist Makefile` guard), producing a misleading `Qt6Cored.lib does not exist` against the uninstalled kit. `sb.ps1` already implements the fix via a `build\.seamly-qmake-kit` marker; port it to `sd.ps1`. |
| **47** | 3 of 4 done | Bare `qmake` on `PATH` resolves to Design Studio's reduced Qt (no `mkspecs`). `build.ps1` now exports `QMAKE`, prepends the kit's `bin\`, and rejects a Qt without `mkspecs`; docs updated. Remaining: optional developer `PATH` cleanup. |
| **48** | open, **important** | The bare-`windeployqt` bug above. |

Task 31's subtasks 1 and 4 are now `[X]`; 2 and 3 remain blocked on Task 44.

## Concrete next steps (resume here)

1. **Install the two Qt modules** (Task 44) — everything else downstream depends on it.
2. **Fix Task 48** — swap the three bare `windeployqt` calls for `qtPrepareTool`, rebuild via `scripts/sb.ps1`, and confirm the deployed `Qt6Core.dll` reports `6.11.1.0` and that `seamly2d.exe`/`seamlyme.exe` actually start from `build/src/app/<app>/bin`.
3. **Close Task 30's verify subtask** — build SeamlyLayout on 6.11.1, run it, check the seamly2d → seamlyLayout handoff renders, then `scripts/packaging/windows/smsi.ps1` and confirm the single-runtime MSI installs and all three apps launch. Expect a **substantially smaller MSI** than the previous ~187 MB two-runtime build; record the new size.
4. **Push `e769a55e10`** to origin when ready (it is local-only right now), and commit the pending `app.pro` edit.
5. Consider moving **Task 30** to `COMPLETED.md` once step 3 passes. `COMPLETED.md`'s newest entry is still **Task 20**.

## Gotchas seen this session

- **`gh` CLI is NOT installed on this machine.** The documented post-task flow (push → PR → `gh pr checks --watch` → merge) is unavailable. Both merges this session were done **locally** (`git merge --ff-only` + `git push origin run-seamlyLayout`) at the user's explicit direction, skipping the green-CI-before-merge gate. Pushing to `run-seamlyLayout` does trigger `seamlylayout-ci.yml` and `windows-msi.yml`, but *after* the merge — check [the Actions tab](https://github.com/seamly/Seamly2D/actions).
- **Qt Design Studio poisons `PATH`.** Bare `qmake`, `windeployqt` **and** `windeployqt6` all resolve to `C:\Qt\Tools\QtDesignStudio\qt6_design_studio_reduced_version\bin\` — a Qt **6.8.7** kit with **no `mkspecs`**. Root cause of both Task 47 and Task 48. Never call these tools bare; use an absolute path, `qtPrepareTool`, or pin `QMAKE`/`PATH`.
- **PowerShell 5.1 + `$ErrorActionPreference='Stop'`:** piping a native command through `2>&1 | …` turns its stderr into a terminating `NativeCommandError` even on exit 0. This produced two spurious "build failed" reports before the cause was spotted. Redirect to a file at the `cmd` level instead, or use `smsi.ps1`'s `Invoke-Tool` pattern (relax to `Continue`, judge by exit code).
- **`$proFile` collides with PowerShell's automatic `$PROFILE`** (names are case-insensitive). `sb.ps1` uses `$proPath`; **`sd.ps1` still has the collision** — harmless in practice, worth cleaning up.
- **Qt frontend test exes are GUI-subsystem binaries**, so they print nothing to a captured stdout and `ctest` can appear to hang. Run them with `-o <file>,txt` and read the `Totals:` line, with `QT_QPA_PLATFORM=offscreen`.
- **`build.ps1` used to always launch the app** after a successful build, blocking any non-interactive caller. It now takes `-NoRun`; `sb.ps1` passes it.
- **Historical 6.10 references in `COMPLETED.md` and `PROJECT_PLAN.md` were left alone** deliberately — they are a record of what was true at the time, not stale config.
