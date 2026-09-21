# CLAUDE.md

## Environment Rules for MSVC Compilation

- The active shell environment is PowerShell running on Windows x64.
- Do not execute `cl.exe` directly, as it requires the Visual Studio toolchain paths.
- To compile C++ code safely without environment drops, always wrap the command inside `cmd.exe` to trigger the Visual Studio Community 2022 setup script first.

### Example Compilation Command

```powershell
cmd.exe /c "`"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat`" && cl /EHsc main.cpp"
```

## Communication Style

These rules apply to all responses and generated documentation unless the user explicitly requests detail. The user has dyslexia and autism, so communication style should be terse.

**Priority:*- Produce terse, minimized output while preserving required information and correctness.

- Start with the result. Do not add an introduction or conclusion.
- Do not use pleasantries, filler, transition prose, or meta-commentary.
- Do not restate the request.
- Do not narrate routine actions.
- State each fact once.
- Use short, active-voice sentences.
- Put one idea in each sentence.
- Put each sentence in bulleted format when possible.
- Use ASD-STE100 principles, adapted for software development.
- Keep instructions to 20 words or fewer.
- Keep descriptive sentences to 25 words or fewer.
- Use the same term for the same concept.
- Keep necessary articles and grammar.
- Prefer bullets for three or more independent items.
- Add rationale, examples, background, or detailed explanation only when:

  - the user requests it;
  - it explains a failure;
  - it is necessary for a decision;
  - omitting it creates a technical or safety risk.

### Completion Reports

After making changes, report only:

1. What changed.
2. Verification status.
3. Required user decisions or actions.

Do not summarize work already evident from the report.

### Code Documentation

Document intent, contracts, constraints, and non-obvious behavior.

Do not document behavior that is obvious from the code.
PROHIBIT referencing tasks and todo steps in the code comments.
PROHIBIT saving dates in the code comments.

- Keep `@brief` to one sentence.
- Add `@param` only when the parameter meaning or constraints require explanation.
- Add `@return` only when the return semantics require explanation.
- Add inline comments only for non-obvious logic, invariants, workarounds, or constraints.
- Keep code briefs and comments terse and minimized.
- REPEATED INSTRUCTION BECAUSE THIS IS SO IMPORTANT --> PROHIBIT referencing task numbers, TODO items, dates, etc. as they are ephemeral. information.
- Do not narrate control flow line by line.
- Prefer clearer code over explanatory comments.

## Seamly Apps

All apps use Qt 6.11.1.

### Seamly2D

- Path: `src/app/seamly2d/`
- Purpose: Parent pattern-drafting application.
- Code: C++.
- GUI: Qt 6.11 / QtWidgets.
- Build: qmake / make / make install.
- File header:

  - Author: `slspencer`
  - Copyright: `2026 Seamly2D Project`
  - License: `GPL-3.0-or-later`

### SeamlyLayout

- Path: `src/app/seamlylayout/`
- Purpose: Creates layouts of Seamly2D pattern pieces for cutting and downstream software.
- Code: Rust converted to C++ with `cxx-qt`.
- GUI: Qt 6.11 / QML / QtWidgets.
- Build: local - `packaging\windows\local_build_msi.ps1`; GitHub - ci.yml
- Detailed rules: see "SeamlyLayout Rules" below.
- File header:

  - Author: `slspencer`
  - Copyright: `2026 Seamly2D Project`
  - License: `MIT`

### SeamlyMe

- Path: `src/app/seamlyme/`
- Purpose: Creates `.smis` individual and `.smms` multisize measurement files for Seamly2D.
- Code: C++.
- GUI: Qt 6.11 / QtWidgets.
- Build: qmake / make / make install.
- File header:

  - Author: `slspencer`
  - Copyright: `2026 Seamly2D Project`
  - License: `GPL-3.0-or-later`

## SeamlyLayout Rules

Merged from SeamlyLayout's own former `CLAUDE.md` and `.claude/rules/` (2026-09-21).

### General Rules

- Read the `.md`, `.txt`, and `.rtf` files in `src/app/seamlylayout/docs/` regularly.
- Update docs to reflect code changes.
- Use "flatten" only for baking in transforms. Use "interpolation" for converting curves to polylines.
- Always use absolute file paths, never relative paths. Resolve via `QFileInfo::absoluteFilePath()` (C++) or `std::path::Path::canonicalize()` (Rust).
- When making code changes, search and update files with `.rs`, `.cpp`, `.h`, and `.qml` extensions.
- When making text updates, search and update files with `.md` and `.txt` extensions.
- Ignore markdown linting errors (MD001, MD004, MD013, etc.) in SeamlyLayout docs — they are editor diagnostic noise, not blocking issues.
- Move as much application functionality as possible into `.rs` files.
- Add Doxygen briefs to all functions, methods, wrappers, controllers.
- Add inline comments to all new code to describe workflow, control flow, and data flow, so an intermediate-level programmer can follow it.

### Shell Commands Policy

The following Bash and PowerShell commands are pre-allowed in `.claude/settings.json` and run without permission prompts:

**Bash (Git Bash / POSIX):**

- File discovery: `ls *`, `ls`, `find *`
- Navigation: `cd *`, `pwd`, `chdir *`
- File ops: `cp *`, `mv *`, `rm *`, `cat *`, `mkdir *`
- Inspection: `grep *`, `which *`, `where *`, `type *`, `env`, `env *`, `date`, `set`, `set *`, `whoami`, `hostname`
- Scripting: `echo *`, `PATH=*`
- Version control: `git *`, `gh *`

**PowerShell:**

- File discovery: `ls *`, `find *`, `dir *`
- Navigation: `cd *`, `pwd`
- File ops: `cp *`, `copy *`, `mv *`, `rm *`, `cat *`, `mkdir *`
- Inspection: `where *`, `date`, `whoami`, `hostname`
- Scripting: `echo *`, `$env:*`, `$*`
- Version control: `git *`, `gh *`

### Regex Policy

- Do not introduce new regex in SeamlyLayout code. Use proper parsing libraries instead.
- Existing regex in Rust code is acceptable.
- SVG processing must use `xmltree`/`svg_dom`, never regex, for SVG manipulation.

### Detailed Rule Files

Rule files live in the project-level `.claude/rules/`, prefixed `seamlylayout_`:

- [seamlylayout_dependencies.mdc](.claude/rules/seamlylayout_dependencies.mdc) — Crate versions, workspace structure, Qt modules
- [seamlylayout_ffi-bridge.mdc](.claude/rules/seamlylayout_ffi-bridge.mdc) — `extern "C"` conventions, memory ownership, error codes
- [seamlylayout_licensing.mdc](.claude/rules/seamlylayout_licensing.mdc) — License requirements: Qt LGPL-3.0, Rust MIT
- [seamlylayout_qt-style.mdc](.claude/rules/seamlylayout_qt-style.mdc) — C++/QML coding conventions and file headers
- [seamlylayout_rust-style.mdc](.claude/rules/seamlylayout_rust-style.mdc) — Rust coding conventions and file headers
- [seamlylayout_svg-processing.mdc](.claude/rules/seamlylayout_svg-processing.mdc) — SVG/DOM manipulation guidelines
- [seamlylayout_testing.mdc](.claude/rules/seamlylayout_testing.mdc) — Testing frameworks and commands
- [seamlylayout_guidelines_export_dxf.mdc](.claude/rules/seamlylayout_guidelines_export_dxf.mdc) — DXF export pipeline
- [seamlylayout_guidelines_layout.mdc](.claude/rules/seamlylayout_guidelines_layout.mdc) — Layout processing pipeline
- [seamlylayout_guidelines_settings.mdc](.claude/rules/seamlylayout_guidelines_settings.mdc) — Settings workflow and defaults
- [seamlylayout_guidelines_tiling.mdc](.claude/rules/seamlylayout_guidelines_tiling.mdc) — Tiling calculation and reduction

`branding.mdc` was referenced by the old files but never existed. No branding rule file exists yet.

## Build Rules

All three applications build against **Qt 6.11.1**.

### CI

- Use Qt 6.11.1 with MSVC 2022.
- for building on github, `ci.yml` builds all three apps, across all three platforms (Windows, Linux, MacOS). Sets the Qt release in its `QT_VERSION`.
- for local testing, .\packaging\windows\local_build_msi.ps1 builds a windows 11 x64 MSI file.
- `version.sh` is required for ci.yml and local_build_msi.ps1 to create the version used in both github and local build pipelines.


### Local Windows Build

**`packaging\windows\local_build_msi.ps1` is the build.*- Use it for every
local build. Do **not*- use `src\app\seamlylayout\build.ps1` or `qd.ps1` any
more (user decision, 2026-09-02): they build SeamlyLayout alone, which is not
what the install-and-test loop needs.

It builds Seamly2D, SeamlyMe and SeamlyLayout release binaries, runs the Qt unit
tests, then packages the Windows **x64*- MSI via `smsi.ps1`:

- `qmake Seamly.pro -r -config release` + `nmake` for Seamly2D/SeamlyMe **and**
  the four Qt test suites;
- `nmake check` — Seamly2DTest, CollectionTest, ParserTest, TranslationsTest.
  A failing suite stops the build, so a broken test never reaches an MSI;
- CMake + Ninja + Cargo for SeamlyLayout;
- `smsi.ps1` to stage and `wix build`.

Steps 1-2 mirror `ci.yml`'s `windows-test` job; the rest mirrors its
`windows-msi` (x64) job. Requires VS 18 Community, a Qt 6.11.1+ `msvc2022_64`
kit, and Rust on PATH. Treat its output as a local dev build, not a release
artifact — releases still go through `gh workflow run ci.yml`.

Switches:

- `-SkipTests` — build with `CONFIG+=noTests` and skip `nmake check`. Only for a
  packaging-only change; the Qt suites have no other local runner.
- `-SkipValidation` — passed to `smsi.ps1`, skips the `wix msi validate` ICE pass.

A build script for the Windows **arm64*- MSI is planned and does not exist yet.
Do not assume this script covers arm64.

Use Qt 6.11.1 `msvc2022_64` with the VS 18 Community MSVC environment.

- Put shadow builds in `build/`.
- `build/` is gitignored.

The local Qt kit must include:

- `qtwebengine`
- `qtwebchannel`
- `qtpositioning`
- `qtserialport`
- `WebEngineView`
- `QtWebEngineQuick`
- `Qt6WebEngineCore`

See `.github/README-BUILDS.md` for detailed build and packaging knowledge.

## Coding Rules

`.github/README-CODE-STYLES.md` is authoritative.

Read it before writing or renaming code.

### File Names

- Use lowercase `snake_case`.
- Name files for their purpose.
- Use prefixes defined by the style guide.
- Do not use generic names such as `util.h` or `helpers.cpp`.
- Do not introduce abbreviations.
- Make source filenames unique repository-wide.
- Multiple SeamlyLayout crate-root `lib.rs` files are the only exception.
- Do not start new filenames with bare `s`.
- Do not start new source filenames with `v`.
- Do not rename existing `v*` files unless the task requires it.

### Classes

Use `UpperCamelCase` class names.

When a file primarily defines one class, name the file exactly after the class.

Example:

`SettingsCommon.h` / `SettingsCommon.cpp` → `class SettingsCommon`

### License Headers

For every new or modified Seamly2D or SeamlyMe file:

- Author: `slspencer`
- Copyright: `2026 Seamly2D Project`
- License: `GPL-3.0-or-later`

For every new or modified SeamlyLayout file:

- Author: `slspencer`
- Copyright: `2026 Seamly2D Project`
- License: `MIT`

### Markdown

Ignore MD041 first-line-heading warnings.

Do not restructure files only to silence MD041.

## Control Flow

Prefer the simplest control flow that preserves correctness.

- Use guard clauses when they reduce nesting.
- Use guard clauses for validation and early error exits.
- Keep the primary execution path visually clear.
- Avoid unnecessary nesting.
- Do not introduce abstractions only to satisfy a nesting-depth limit.
- Use a state machine only for behavior with meaningful states and transitions.
- Identify relevant inputs and outcomes before implementing complex Boolean logic.
- Use a truth table when combinations are difficult to reason about or test.
- Do not expose scratchpad or private reasoning.
- Document only resulting requirements, decisions, invariants, and tests.

## Git Remotes

### `origin`

- Repository: `https://github.com/seamly/Seamly2D.git`
- Working branch: `run-seamlyLayout`
- This is the work repository and branch of record.
- Push project work only to `origin`.

### `upstream`

- Repository: `https://github.com/FashionFreedom/Seamly2D`
- Fetch only.
- **Never push to upstream.**
- **Never open routine task PRs against upstream.**

## Branch Strategy

`seamly/Seamly2D` must remain a GitHub fork of `FashionFreedom/Seamly2D`.

Do not leave the fork network.

### `develop`

- Keep `origin/develop` as a pristine mirror of upstream `develop`.
- Update it only from upstream.
- Never merge project work into `develop`.

### `run-seamlyLayout`

- Accumulate all project work here.
- Merge `develop` into `run-seamlyLayout`.
- Never merge `run-seamlyLayout` into `develop`.

### Endgame

When the project is complete:

1. Push `seamly:run-seamlyLayout` to `FashionFreedom:run-seamlyLayout`.
2. The user creates the single upstream PR.

Do not use GitHub's default "Compare & pull request" banner.

If a project-side PR is required, target `seamly:run-seamlyLayout`.

## Task Tracking

`project-docs/PROJECT_PLAN.md` contains the approved implementation plan.

Task files use:

`project-docs/TODO_*.md` or project-docs/TEST_*.md

Before working on a task:

1. Read the task's file.
2. Follow its cross-references.

Do not rely on a hard-coded list of task files.

For each task:

- Check completed numbered subtasks.
- Move fully completed tasks to `project-docs/TODO_COMPLETED.md`.
- Never implement tasks in `project-docs/WONT_DO_MIGRATE.md`.

## Task Workflow

Apply this workflow to every request to implement a `TODO_*.md` or `TEST_*.md` task or subtask.

### 1. Sync `develop`

- Run `git fetch origin`.
- Fast-forward local `develop` when it is behind `origin/develop`.
- Never merge project work into `develop`.

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

- add or update unit tests;
- run the local tests:
  - Seamly2D/SeamlyMe Qt suites — `packaging\windows\local_build_msi.ps1`
    runs them via `nmake check`;
  - SeamlyLayout — `ctest --preset debug` and `cargo test --workspace`.

The two SeamlyLayout commands need Qt and Rust in the shell first; a bare shell
does not have them:

```powershell
$env:PATH  = "C:\Qt\6.11.1\msvc2022_64\bin;$env:USERPROFILE\.cargo\bin;$env:PATH"
$env:QMAKE = 'C:/Qt/6.11.1/msvc2022_64/bin/qmake.exe'
```

Without them the failures name the wrong cause: a missing `cargo`, a
`QtMissing` panic, or `STATUS_DLL_NOT_FOUND`. See
`.github/README-BUILDS.md`.

Three of the four Qt suites are GUI-subsystem binaries and print nothing to a
console, and one shared `-o` target is overwritten by each `qExec()` call. To
read a single suite, set `SEAMLY_TEST_LOG_DIR` to a directory and run the binary
through its own generated `target_wrapper.bat`, which supplies `QT_PLUGIN_PATH`.

Seamly2D and SeamlyMe and SeamlyLayout have a local build script: packaging\windows\local_build_msi.ps1. 
Seamly2D and SeamlyMe and SeamlyLayout have a github build script: .github\workflows\`ci.yml` verifies them. A skip-ci push defers that verification. See CI Cost Control.

Do not proceed after a failing required test or build. Report the failure.

### 6. Update Tracking

- Update the applicable `TODO_*.md`.
- Move completed tasks to `TODO_COMPLETED.md`.
- Update `SESSION_HANDOVER.md`.

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

- `.github/workflows/**`
- `packaging/**`
- `*.pro`
- `*.pri`
- `CMakeLists.txt`
- `Cargo.toml`
- Linux-specific code
- macOS-specific code
- `#ifdef Q_OS_*`
- arm64 handling

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

- skip task-workflow steps 1–5 and 8;
- stage and commit on the current branch;
- push to origin.

Covered paths include:

- `*.md`
- `project-docs/**`
- `LICENSE`
- `.claude/**`
- `.vscode/**`

Do not treat `.txt` or `.svg` as documentation-only for this rule.

Check the complete pending push before relying on this exception.

## Session Handover

Keep `SESSION_HANDOVER.md` current.

Update it:

- before compaction;
- after completing a task;
- when session state changes materially.

Record only information needed by the next session:

- current task;
- exact progress;
- task-tracking changes;
- decisions and necessary rationale;
- changed files;
- concrete next steps;
- relevant machine state outside the repository.

Do not use it as a session transcript.

## Key References

- `.github/README-CODE-STYLES.md` — authoritative code style.
- `.github/README-BUILDS.md` — build, toolchain, packaging, and platform details.
- `project-docs/PROJECT_PLAN.md` — approved implementation plan.
- `project-docs/NEW-ATTRIBUTES.csv` — SVG `data-*` attribute specification.
- `src/app/seamlylayout/input/richmond-shirt_v1_v061-test.sm2d` — test pattern.
- `SESSION_HANDOVER.md` — current cross-session state.
