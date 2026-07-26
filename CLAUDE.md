0

# Seamly2D

Pattern drafting application — parent app of the Seamly family.

- **Author:** slspencer
- **Copyright:** 2026 Seamly2D Project
- **License:** GPL-3.0-or-later

## Architecture

- Qt 6 / C++ (QtWidgets), built with qmake (two toolchains — see Build Notes)
- Apps: `src/app/seamly2d` (pattern drafting), `src/app/seamlyme` (measurements)
- Shared libraries under `src/libs/` (`vlayout`, `vformat`, `vpatterndb`, `ifc`, ...)
- `src/app/seamlylayout/` — daughter layout app (Rust + Qt 6.11/QML), tracked directly in this repo like seamlyme; it has its own build (`src/app/seamlylayout/qd.ps1`) and must stay out of the Seamly2D qmake build. It has its own CLAUDE.md and rules.

## Build Notes

Since **Task 30** all three apps — seamly2d, seamlyme and seamlyLayout — build against the **same Qt release, 6.11.1**; there is no longer a separate Qt for the daughter app:

- **CI toolchain (GitHub runner):** Qt 6.11.1 + MSVC 2022 — `QT_VERSION` in `ci.yml`, `seamlylayout-ci.yml` and `windows-msi.yml` must all name this one release
- **Local toolchain (developer PC, check builds of current work):** Qt 6.11.1 `msvc2022_64` + VS 18 Community MSVC (`vcvars64.bat`), qmake + jom for the parents / CMake + Ninja + Cargo for seamlyLayout; release shadow-build in `build/` (gitignored)
- Local debug build: `scripts/sd.ps1` ("seamly2d debug") — auto-detects the newest Qt msvc2022_64 kit (6.11.1 or newer) under `C:\Qt` and the VS 18 Community MSVC environment, then shadow-builds `CONFIG+=debug` into `scripts/seamly2d-build-debug/` (gitignored); the debug exe lands at `scripts/seamly2d-build-debug/src/app/seamly2d/bin/seamly2d.exe` with Qt debug DLLs deployed by windeployqt. `-Run` launches it after the build; see the script's `.SYNOPSIS` for details.
- **Local Qt kit must include** `qtwebengine` **plus** `qtwebchannel` **and** `qtpositioning` — seamlyLayout's `SvgCanvas.qml` uses `WebEngineView`, and Qt6WebEngineCore's CMake config requires the other two, so a kit without them fails `find_package(Qt6 ... WebEngineQuick)` at configure time. The Qt online installer does not pull them in automatically when you tick Qt WebEngine.

## Coding Rules

- **`.github/README-CODE-STYLES.md` is the authoritative style guide** — JSF-AV C++ with the deviations listed there. Read it before writing or renaming code; the points below are the ones that come up constantly, not a replacement for it
- **File naming:** snake_case, all lowercase; name files for what they do, using the prefixes the style guide lists (`settings_*`, `dialog_<toolgroup>_<toolname>`, `tool_*`, `model_*`, `options_*`, `test_*`, `application_<appname>`, `<platform>_*`, …). No abbreviations, no generic names (`util.h`, `helpers.cpp`). **Unique repo-wide: a search for `<filename.extension>` must return exactly one file** — the one carve-out is seamlyLayout's crate roots, where multiple `lib.rs` are accepted and told apart by path. Do NOT start a file name with a bare `s` — that older anti-`v` convention is superseded by the prefix list. New source files still must not begin with `v`; existing `v*` files keep their names when edited unless a task renames them
- **Class naming, and the file that holds it:** classes are UpperCamelCase (the project's deliberate deviation from JSF-AV). A file that **primarily defines one class** is the exception to snake_case — name it exactly like the class, in UpperCamelCase: `SettingsCommon.h` / `SettingsCommon.cpp` ↔ `class SettingsCommon`
- **License headers:** every new file gets a GPLv3-or-later header with copyright 2026 Seamly2D Project and author slspencer (follow the existing header block style, e.g. `src/libs/vformat/svg_generator.cpp`)
- **Documentation:** all new code and every modified function gets a Doxygen-compatible `@brief` (plus `@param`/`@return` where applicable) and inline comments so an intermediate-level programmer can follow the workflow, control flow, and data flow
- **Markdown lint:** ignore MD041 (first-line-heading) warnings — they are editor diagnostic noise, not blocking issues; do not restructure files to silence them

## Git Remotes

- `origin` = `seamly/Seamly2D` — the work repo; ALL pushes and pull requests go here
- `upstream` = `FashionFreedom/Seamly2D` — the public parent project; **NEVER push to it or open PRs against it** (fetch only; its push URL is set to `DISABLED_NEVER_PUSH` in the local clone to enforce this)

### Branch strategy and endgame (decided 2026-07-18)

- `seamly/Seamly2D` deliberately remains a GitHub **fork** of `FashionFreedom/Seamly2D` — do NOT detach / "Leave fork network"; the fork link is required for the final upstream PR
- `develop` on origin is a **pristine mirror of upstream develop**: update it only by syncing from upstream; never merge project work into it before the endgame
- All project work accumulates on `run-seamlyLayout`; keep it current by merging `develop` into it (that direction only)
- **Endgame:** when the project is finished, ONE pull request `seamly:run-seamlyLayout` → `FashionFreedom:develop` — the single sanctioned upstream PR, created by the user (not by Claude); after upstream merges it, sync origin `develop` from upstream and retire `run-seamlyLayout`
- GitHub's green "Compare & pull request" banner on the fork defaults to FashionFreedom as base and cannot be disabled — never use it; create PRs via `gh` (default repo pinned to origin) or from the fork's own branch pages

## Task Tracking

- `PROJECT_PLAN.md` — the current approved implementation plan
- Task lists are split by area — pick the file matching the task:
  - `TODO_MIGRATE.md` — migrating the SeamlyLayout app into the Seamly2D structure (SeamlyMe and SeamlyLayout callable from within Seamly2D; all three apps distributed together for installation)
  - `TODO_SEAMLY2D.md` — tasks that add features to the Seamly2D app
  - `TODO_SEAMLYLAYOUT.md` — tasks that add features to the SeamlyLayout app
- Each `TODO_*.md` file holds tasks with checkbox subtasks; check off subtasks as they are accomplished
- `COMPLETED.md` — when all subtasks of a task are complete, move the task here from its `TODO_*.md` file
- **Pre-task branch setup:** before implementing a task from any `TODO_*.md` file, always:
  1. update local `develop` from `origin` (`git fetch origin` + fast-forward `develop`)
  2. update local `run-seamlyLayout` from local `develop` (merge/fast-forward `develop` into it)
  3. create a new branch from `run-seamlyLayout` for the task, and do the work there
- **Post-task workflow:** after implementing a task from a `TODO_*.md` file:
  1. write unit tests where the task adds or changes code, and run them; run a local check build (`scripts/sd.ps1`) and verify the change works
  2. update task tracking in the same change: check off subtasks in the task's `TODO_*.md` file; move fully completed tasks to `COMPLETED.md`
  3. stage and commit on the task branch
  4. push the task branch to origin and create a pull request targeting `run-seamlyLayout` — always in origin `seamly/Seamly2D` (the gh default repo is set to it), NEVER in the public upstream `FashionFreedom/Seamly2D`
  5. watch the PR's CI checks (`gh pr checks <pr> --watch`); when all checks pass, merge the PR; if any check fails, do NOT merge
  6. notify the user of the outcome either way — merged (with PR URL) or not merged (with the failing checks) — then, after a merge, update local `run-seamlyLayout` from origin and delete the local task branch (origin deletes the remote branch automatically on merge)
- **Docs-only exception:** when a commit changes only `.md`, `.txt`, and/or `.svg` files (no code), skip the local build/test verification and the push/PR/CI cycle above entirely — stage and commit locally only; do not push to origin
- **`SESSION_HANDOVER.md`** (repo root) — keep it current with the session's state: update it before compaction and when finishing a task. It is the next chat session's starting point, so it must carry what git does not — the current task and its exact progress, which `TODO_*.md` / `COMPLETED.md` entries moved, key decisions and the reasoning behind them, files changed, concrete next steps, and any machine state changed outside the repo. The `PreCompact` / `PostCompact` hooks in `.claude/settings.json` only surface a reminder; keeping the file current is required regardless of whether that reminder appears

## Key References

- `status-docs/new-attributes.csv` — SVG `data-*` attribute spec for the SeamlyLayout handoff
- Test pattern: `src/app/seamlylayout/input/richmond-shirt_v1_v061-test.sm2d`
- `.github/README-BUILDS.md` — build knowledge base (toolchains, per-platform packaging, settings/data locations, packaging decisions); keep it updated when build knowledge changes
