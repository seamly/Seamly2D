# AGENTS.md

## Communication Style

These rules apply to all responses and generated documentation unless the user explicitly requests detail.

**Priority:** Minimize output while preserving required information and correctness.

* Start with the result. Do not add an introduction or conclusion.
* Do not use pleasantries, filler, transition prose, or meta-commentary.
* Do not restate the request.
* Do not narrate routine actions.
* State each fact once.
* Use short, active-voice sentences.
* Put one idea in each sentence.
* Use ASD-STE100 principles, adapted for software development.
* Keep instructions to 20 words or fewer.
* Keep descriptive sentences to 25 words or fewer.
* Use the same term for the same concept.
* Keep necessary articles and grammar.
* Prefer bullets for three or more independent items.
* Add rationale, examples, background, or detailed explanation only when:

  * the user requests it;
  * it explains a failure;
  * it is necessary for a decision;
  * omitting it creates a technical or safety risk.

### Completion Reports

After making changes, report only:

1. What changed.
2. Verification status.
3. Required user decisions or actions.

Do not summarize work already evident from the report.

### Code Documentation

Document intent, contracts, constraints, and non-obvious behavior.

Do not document behavior that is obvious from the code.

* Keep `@brief` to one sentence.
* Add `@param` only when the parameter meaning or constraints require explanation.
* Add `@return` only when the return semantics require explanation.
* Add inline comments only for non-obvious logic, invariants, workarounds, or constraints.
* Do not narrate control flow line by line.
* Prefer clearer code over explanatory comments.

## Seamly Apps

All apps use Qt 6.11.1.

### Seamly2D

* Path: `src/app/seamly2d/`
* Purpose: Parent pattern-drafting application.
* Code: C++.
* GUI: Qt 6.11 / QtWidgets.
* Build: qmake / make / make install.
* File header:

  * Author: `slspencer`
  * Copyright: `2026 Seamly2D Project`
  * License: `GPL-3.0-or-later`

### SeamlyLayout

* Path: `src/app/seamlylayout/`
* Purpose: Creates layouts of Seamly2D pattern pieces for cutting and downstream software.
* Code: Rust converted to C++ with `cxx-qt`.
* GUI: Qt 6.11 / QML / QtWidgets.
* Build: local - `src/app/seamlylayout/qd.ps1`; GitHub - ci.yml
* Merge its local `AGENTS.md` and `rules.md` requirements into the project-level files.
* File header:

  * Author: `slspencer`
  * Copyright: `2026 Seamly2D Project`
  * License: `MIT`

### SeamlyMe

* Path: `src/app/seamlyme/`
* Purpose: Creates `.smis` individual and `.smms` multisize measurement files for Seamly2D.
* Code: C++.
* GUI: Qt 6.11 / QtWidgets.
* Build: qmake / make / make install.
* File header:

  * Author: `slspencer`
  * Copyright: `2026 Seamly2D Project`
  * License: `GPL-3.0-or-later`

## Build Rules

All three applications build against **Qt 6.11.1**.

### CI

* Use Qt 6.11.1 with MSVC 2022.
* `ci.yml` is the only CI workflow. It builds all three apps. Set the Qt release in its `QT_VERSION`.

### Local Windows Build

The repository has no local build or test script for Seamly2D and SeamlyMe.
`ci.yml` builds and tests them. Build them by hand with qmake + jom if you need
a local tree.

SeamlyLayout keeps its own local scripts:

* `src/app/seamlylayout/qd.ps1` — debug build.
* `src/app/seamlylayout/build.ps1` — CMake + Ninja + Cargo. **Local only.**
  `ci.yml` runs `cmake --preset release` directly and never calls this script,
  so a change here needs no CI run.

Use Qt 6.11.1 `msvc2022_64` with the VS 18 Community MSVC environment.

* Put shadow builds in `build/`.
* `build/` is gitignored.

The local Qt kit must include:

* `qtwebengine`
* `qtwebchannel`
* `qtpositioning`
* `WebEngineView`
* `QtWebEngineQuick`
* `Qt6WebEngineCore`

See `.github/README-BUILDS.md` for detailed build and packaging knowledge.

## Coding Rules

`.github/README-CODE-STYLES.md` is authoritative.

Read it before writing or renaming code.

### File Names

* Use lowercase `snake_case`.
* Name files for their purpose.
* Use prefixes defined by the style guide.
* Do not use generic names such as `util.h` or `helpers.cpp`.
* Do not introduce abbreviations.
* Make source filenames unique repository-wide.
* Multiple SeamlyLayout crate-root `lib.rs` files are the only exception.
* Do not start new filenames with bare `s`.
* Do not start new source filenames with `v`.
* Do not rename existing `v*` files unless the task requires it.

### Classes

Use `UpperCamelCase` class names.

When a file primarily defines one class, name the file exactly after the class.

Example:

`SettingsCommon.h` / `SettingsCommon.cpp` → `class SettingsCommon`

### License Headers

For every new or modified Seamly2D or SeamlyMe file:

* Author: `slspencer`
* Copyright: `2026 Seamly2D Project`
* License: `GPL-3.0-or-later`

Follow the existing header style in `src/libs/vformat/svg_generator.cpp`.

For every new or modified SeamlyLayout file:

* Author: `slspencer`
* Copyright: `2026 Seamly2D Project`
* License: `MIT`

### Markdown

Ignore MD041 first-line-heading warnings.

Do not restructure files only to silence MD041.

## Control Flow

Prefer the simplest control flow that preserves correctness.

* Use guard clauses when they reduce nesting.
* Use guard clauses for validation and early error exits.
* Keep the primary execution path visually clear.
* Avoid unnecessary nesting.
* Do not introduce abstractions only to satisfy a nesting-depth limit.
* Use a state machine only for behavior with meaningful states and transitions.
* Identify relevant inputs and outcomes before implementing complex Boolean logic.
* Use a truth table when combinations are difficult to reason about or test.
* Do not expose scratchpad or private reasoning.
* Document only resulting requirements, decisions, invariants, and tests.

## Git Remotes

### `origin`

* Repository: `https://github.com/seamly/Seamly2D.git`
* Working branch: `run-seamlyLayout`
* This is the work repository and branch of record.
* Push project work only to `origin`.

### `upstream`

* Repository: `https://github.com/FashionFreedom/Seamly2D`
* Fetch only.
* **Never push to upstream.**
* **Never open routine task PRs against upstream.**

## Branch Strategy

`seamly/Seamly2D` must remain a GitHub fork of `FashionFreedom/Seamly2D`.

Do not leave the fork network.

### `develop`

* Keep `origin/develop` as a pristine mirror of upstream `develop`.
* Update it only from upstream.
* Never merge project work into `develop`.

### `run-seamlyLayout`

* Accumulate all project work here.
* Merge `develop` into `run-seamlyLayout`.
* Never merge `run-seamlyLayout` into `develop`.

### Endgame

When the project is complete:

1. Push `seamly:run-seamlyLayout` to `FashionFreedom:run-seamlyLayout`.
2. The user creates the single upstream PR.

Do not use GitHub's default "Compare & pull request" banner.

If a project-side PR is required, target `seamly:run-seamlyLayout`.

## Task Tracking

`project-docs/PROJECT_PLAN.md` contains the approved implementation plan.

Task files use:

`project-docs/TODO_*.md`

Before working on a task:

1. List the current `TODO_*.md` files.
2. Read the task's file.
3. Follow its cross-references.

Do not rely on a hard-coded list of task files.

For each task:

* Check completed numbered subtasks.
* Move fully completed tasks to `project-docs/TODO_COMPLETED.md`.
* Never implement tasks in `project-docs/WONT_DO_MIGRATE.md`.

## Task Workflow

Apply this workflow to every request to implement a `TODO_*.md` task or subtask.

### 1. Sync `develop`

* Run `git fetch origin`.
* Fast-forward local `develop` when it is behind `origin/develop`.
* Never merge project work into `develop`.

### 2. Sync `run-seamlyLayout`

Merge local `develop` into local `run-seamlyLayout`.

Never merge in the opposite direction.

### 3. Create Task Branch

Create `task-<short-name>` from `run-seamlyLayout`.

Perform all task work there.

### 4. Implement

Implement only the required task scope.

### 5. Verify

For code changes:

* add or update unit tests;
* run the tests that still run locally — SeamlyLayout `ctest --preset debug` and
  `cargo test --workspace`.

Seamly2D and SeamlyMe have no local build or test script. `ci.yml` verifies
them. A skip-ci push defers that verification. See CI Cost Control.

Do not proceed after a failing required test or build.

Report the failure.

### 6. Update Tracking

* Update the applicable `TODO_*.md`.
* Move completed tasks to `TODO_COMPLETED.md`.
* Update `SESSION_HANDOVER.md`.

### 7. Commit

Stage and commit the task branch.

### 8. Merge

Merge the task branch into local `run-seamlyLayout` with `--no-ff`.

### 9. Push

Push local `run-seamlyLayout` to `origin run-seamlyLayout`.

Never push to `FashionFreedom/Seamly2D`.

### 10. Report and Clean Up

Report only:

1. What changed.
2. Test/build status.
3. Required user decisions or actions.

Then delete the local task branch.

No PR is required for normal task work.

## CI Cost Control

A push to `run-seamlyLayout` can start the full multi-platform `ci.yml` suite.

Use the CI skip token in the step 8 merge commit subject by default.

This default stands even though no local build remains. The user decided on
2026-08-15 to verify releases with a manual `workflow_dispatch` run instead. See
Milestones.

The token is `[skip ci]`.

Never write the literal token in commit-message prose.

Use `skip-ci` when referring to it without activating it.

### Run Full CI

Omit the skip token when functional changes touch:

* `.github/workflows/**`
* `packaging/**`
* `*.pro`
* `*.pri`
* `CMakeLists.txt`
* `Cargo.toml`
* Linux-specific code
* macOS-specific code
* `#ifdef Q_OS_*`
* arm64 handling

### Comment-Only Exception

Keep the skip token when every changed line is only a comment or documentation string.

Inspect the diff before deciding.

If any functional line changed, apply the normal CI rule.

### Mixed Pushes

`paths-ignore` evaluates the complete push.

Before relying on documentation path exclusions, inspect:

`git diff --name-only origin/run-seamlyLayout..HEAD`

### Milestones

Run the full CI suite before a release or upstream handoff:

`gh workflow run ci.yml --ref run-seamlyLayout`

Wait for it to pass.

This manual run is the only verification for Seamly2D and SeamlyMe. Skipped CI
is deferred verification, and nothing runs locally to catch a break sooner.

## Documentation-Only Changes

When every changed path is covered by `ci.yml` `paths-ignore`:

* skip task-workflow steps 1–5 and 8;
* stage and commit on the current branch;
* push to origin.

Covered paths include:

* `*.md`
* `project-docs/**`
* `LICENSE`
* `.Codex/**`
* `.vscode/**`

Do not treat `.txt` or `.svg` as documentation-only for this rule.

Check the complete pending push before relying on this exception.

## Session Handover

Keep `SESSION_HANDOVER.md` current.

Update it:

* before compaction;
* after completing a task;
* when session state changes materially.

Record only information needed by the next session:

* current task;
* exact progress;
* task-tracking changes;
* decisions and necessary rationale;
* changed files;
* concrete next steps;
* relevant machine state outside the repository.

Do not use it as a session transcript.

## Key References

* `.github/README-CODE-STYLES.md` — authoritative code style.
* `.github/README-BUILDS.md` — build, toolchain, packaging, and platform details.
* `project-docs/PROJECT_PLAN.md` — approved implementation plan.
* `project-docs/NEW-ATTRIBUTES.csv` — SVG `data-*` attribute specification.
* `src/app/seamlylayout/input/richmond-shirt_v1_v061-test.sm2d` — test pattern.
* `SESSION_HANDOVER.md` — current cross-session state.
