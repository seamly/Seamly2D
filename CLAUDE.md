# CLAUDE.md

<!-- This file loads on every request; keep it under ~12 KB. Area rules live in
.claude/rules/*.md (path-scoped) and procedures in .claude/skills/. Block HTML
comments like this one are stripped from Claude's context. -->

## Context Budget

Every request re-sends the whole conversation. A long session costs more per turn.

- Large file (over ~500 lines): Grep for the section, then Read with `offset` and `limit`.
- Never Read whole: `TODO_COMPLETED.md`, `SESSION_HANDOVER_ARCHIVE.md`, `TODO_SEAMLYLAYOUT.md`, `SVG-DATA-ATTRIBUTES.md`, `README-BUILDS.md`.
- Append to `TODO_COMPLETED.md` with Bash (`cat >>`). Do not Read it first.
- Never Read SVG, DXF, or export files in `test-seamly-layout/`. Use grep, `head`, or a script that prints a summary.
- PDF reports: Read with `pages`, or extract text with a script.
- Screenshots: Read one only when its pixels answer the question.
- Build and test output: write it to a scratchpad log. Read the tail and the error lines.
- Batch independent reads and greps into one tool call.
- Broad search (many files, location unknown): use the `Explore` agent. It returns only the result.
- A task with phases (plan, implement, verify): after each phase, update `SESSION_HANDOVER.md`. Then tell Sue it is a good point to `/compact`.

## Contributing

- Create `task-<short-name>` from `run-seamlyLayout` for each task.
- Commit each small unit improvement to the task branch.
- Do not push. Sue pushes `run-seamlyLayout` to `origin`.
- Do not mention Claude, AI, or agents in commits, comments, Doxygen briefs, or `.md` files.
- Do not put task names, task numbers, TODO items, or dates in commits, comments, or Doxygen briefs. They are ephemeral.

## Completion Reports

The global `~/.claude/CLAUDE.md` sets the writing style. After changes, report only:

1. What changed.
2. Verification status.
3. Required user decisions or actions.

## Code Documentation

Document intent, contracts, constraints, and non-obvious behavior. Do not document what the code shows.

- `@brief`: one sentence.
- `@param` / `@return`: only when the meaning or a constraint needs it.
- Inline comments: only non-obvious logic, invariants, workarounds, or constraints. SeamlyLayout adds its own rule.
- Do not narrate control flow line by line. Prefer clearer code over explanatory comments.

## Seamly Apps

All apps use Qt 6.11.3.

| App | Path | Purpose | Code / GUI | License |
|---|---|---|---|---|
| Seamly2D | `src/app/seamly2d/` | Parent pattern-drafting app | C++, Qt Widgets; qmake | `GPL-3.0-or-later` |
| SeamlyMe | `src/app/seamlyme/` | `.smis` individual and `.smms` multisize measurement files | C++, Qt Widgets; qmake | `GPL-3.0-or-later` |
| SeamlyLayout | `src/app/seamlylayout/` | Layouts of Seamly2D pieces for cutting and downstream software | Rust core in `crates/`, CXX-Qt 0.7.3 bridge, QML + Qt Widgets; CMake + Cargo | `MIT` |

- File header for every new or modified file: Author `slspencer`, Copyright `2026 Seamly2D Project`, license from the table.
- SeamlyLayout platforms: Windows 11, Linux (all flavors), macOS (latest 3 versions).
- SeamlyLayout license split: `.claude/rules/seamlylayout_licensing.md`.
- `.claude/rules/seamlylayout.md` loads when you touch SeamlyLayout files.
- `.claude/rules/seamlylayout-handoff.md` (the Seamly2D → SeamlyLayout `--svg-stdin` contract) loads when you touch either side of it.

## Build Rules

### MSVC

- The shell is PowerShell on Windows x64.
- Do not run `cl.exe` directly. Run it inside `cmd.exe` after `vcvars64.bat`:

```powershell
cmd.exe /c "`"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat`" && cl /EHsc main.cpp"
```

### Local Windows Build

- `packaging\windows\local_build_msi.ps1` is the build. Use it for every local Windows build.
- Ask Sue before you start it. She batches several tasks per build.
- It builds the three apps (release), runs `nmake check` (4 Qt suites), `ctest --preset release`, and `cargo test --workspace`, then packages the x64 MSI (`smsi.ps1`, `wix build`).
- A failing suite stops the build. It mirrors `ci.yml`'s `windows-test` and `windows-msi` (x64) jobs.
- `-SkipTests`: `CONFIG+=noTests`, no tests. Only for a packaging-only change.
- `-SkipValidation`: skips the `wix msi validate` ICE pass.
- Output is a local dev build. Releases go through `gh workflow run ci.yml`.
- No arm64 MSI script exists yet.
- `src\app\seamlylayout\build.ps1`, `qd.ps1`, `qr.ps1`, `run_debug.ps1`, `run_release.ps1` are removed (user decision): they built SeamlyLayout alone.
- Needs VS 18 Community, Rust on PATH, and a Qt 6.11.3 `msvc2022_64` kit with `qtwebengine`, `qtwebchannel`, `qtpositioning`, `qtserialport`, `WebEngineView`, `QtWebEngineQuick`, `Qt6WebEngineCore`.
- Shadow builds go in `build/` (gitignored).

### Linux and macOS

- Linux: `packaging/linux/local_build_appimage.sh`. macOS: `packaging/macos/local_build_dmg.sh` (unsigned, not notarized).
- Both build the three apps, run the Qt and Rust tests, then package. `--skip-tests` skips the tests.
- Prerequisites: `.claude/rules/build-linux-macos.md` (loads with `packaging/linux/**` or `packaging/macos/**`). Details: `.github/README-BUILDS.md`.

### CI

- `ci.yml` builds the three apps on Windows, Linux, and macOS. Its `QT_VERSION` sets the Qt release.
- `version.sh` makes the version for `ci.yml` and `local_build_msi.ps1`.

## Coding Rules

`.github/README-CODE-STYLES.md` is authoritative. Read it before you write or rename code.

- File names: lowercase `snake_case`, named for their purpose, with the style guide's prefixes.
- No generic names (`util.h`, `helpers.cpp`). No new abbreviations.
- Source file names are unique repository-wide. Exception: SeamlyLayout crate-root `lib.rs` files.
- New files, functions, and classes do not start with bare `s` or with `v`. Do not rename existing `v*` files unless the task requires it.
- Classes: `UpperCamelCase`. A file that defines one class takes the class name: `SettingsCommon.h` → `class SettingsCommon`.
- Markdown: ignore MD041. Do not restructure a file only to silence it.

## Control Flow

- Use the simplest control flow that stays correct.
- Use guard clauses for validation and early error exits, and when they reduce nesting.
- Keep the primary path visually clear.
- Do not add abstractions only to satisfy a nesting-depth limit.
- Use a state machine only for behavior with meaningful states and transitions.
- For complex Boolean logic, list inputs and outcomes first. Use a truth table when combinations are hard to test.
- Document only resulting requirements, decisions, invariants, and tests. Do not expose scratchpad reasoning.

## Git

- `origin` = `https://github.com/seamly/Seamly2D.git`: the work repository. `run-seamlyLayout` is the branch of record.
- `upstream` = `https://github.com/FashionFreedom/Seamly2D`: fetch only. Never push. Never open routine task PRs against it.
- `seamly/Seamly2D` must stay a GitHub fork of `FashionFreedom/Seamly2D`. Do not leave the fork network.
- `develop` mirrors upstream `develop`. Update it only from upstream. Never merge project work into it.
- `run-seamlyLayout` holds all project work. Merge `develop` into it, never the reverse.
- Task work bases on and compares with `run-seamlyLayout` only. Do not diff, log, or rev-list against `develop`.
- Upstream merge: only when Sue asks. Use the `merge-upstream` skill.
- Endgame: push `seamly:run-seamlyLayout` to `FashionFreedom:run-seamlyLayout`; Sue creates the single upstream PR. Do not use GitHub's "Compare & pull request" banner.
- A project-side PR targets `seamly:run-seamlyLayout`.

## Task Tracking

- `project-docs/PLAN_PROJECT.md` is the approved plan.
- Task files: `project-docs/**/TODO_*.md` and `project-docs/**/TEST_*.md`.
- Before a task, read its section (grep, then `offset`/`limit`) and follow its cross-references.
- Check completed numbered subtasks. Move fully completed tasks to `project-docs/TODO_COMPLETED.md`.
- Never implement tasks in `project-docs/WONT_DO_MIGRATE.md`.

## Task Workflow

1. Create `task-<short-name>` from `run-seamlyLayout`. Work there.
2. Implement only the task scope.
3. Verify:
   - Add or update unit tests.
   - SeamlyLayout: `ctest --preset debug` and `cargo test --workspace`. Set the shell first:

     ```powershell
     $env:PATH  = "C:\Qt\6.11.3\msvc2022_64\bin;$env:USERPROFILE\.cargo\bin;$env:PATH"
     $env:QMAKE = 'C:/Qt/6.11.3/msvc2022_64/bin/qmake.exe'
     ```

     Without it the failures name the wrong cause: missing `cargo`, a `QtMissing` panic, or `STATUS_DLL_NOT_FOUND`.
   - Seamly2D/SeamlyMe Qt suites run in `local_build_msi.ps1`. Ask before you start it; otherwise leave the build check open in the TODO.
   - One Qt suite: set `SEAMLY_TEST_LOG_DIR` and run the binary through its `target_wrapper.bat` (it sets `QT_PLUGIN_PATH`). Three of the four suites print nothing to a console, and each `qExec()` overwrites one shared `-o` target.
   - Do not continue after a failing required test or build. Report the failure.
4. Update the TODO file, `TODO_COMPLETED.md`, and `SESSION_HANDOVER.md`.
5. Commit on the task branch.
6. Merge into local `run-seamlyLayout` with `--no-ff`. Use the skip-ci token by default (see CI Cost Control).
7. Do not push. Report how many commits `run-seamlyLayout` is ahead of `origin`.
8. Delete the local task branch. No PR for normal task work.

## CI Cost Control

- A push to `run-seamlyLayout` can start the full multi-platform `ci.yml`.
- The token is `[skip ci]`. Never write the literal token in commit-message prose; write `skip-ci`.
- Put the token in the step 6 merge subject by default. User decision (2026-08-15): verify releases with a manual `workflow_dispatch` run.
- Omit the token when functional changes touch `.github/workflows/**`, `packaging/**`, `*.pro`, `*.pri`, `CMakeLists.txt`, `Cargo.toml`, Linux- or macOS-specific code, `#ifdef Q_OS_*`, or arm64 handling.
- Comment-only exception: keep the token when every changed line is a comment or doc string. Inspect the diff first.
- `paths-ignore` evaluates the whole push. Check `git diff --name-only origin/run-seamlyLayout..HEAD`.
- Before a release or upstream handoff, run `gh workflow run ci.yml --ref run-seamlyLayout` and wait for a pass. Skip-ci defers that verification.

## Documentation-Only Changes

When every changed path is in `ci.yml` `paths-ignore` (`*.md`, `project-docs/**`, `LICENSE`, `.claude/**`, `.vscode/**`):

- skip workflow steps 1, 3, and 6;
- commit on the current branch. Sue pushes.
- `.txt` and `.svg` are not documentation-only.
- Check the complete pending push before you rely on this exception.

## Session Handover

Keep `SESSION_HANDOVER.md` current and short. Sessions read it at start, so its size is paid every turn.

- Update it before compaction, after a task, and when session state changes materially.
- Record only: current task, exact progress, task-tracking changes, decisions with rationale, changed files, next steps, machine state outside the repository.
- Keep the 5 newest dated entries under "Current status", each under ~15 lines. Move older ones to the top of `project-docs/SESSION_HANDOVER_ARCHIVE.md`.
- Do not Read the archive at session start. It is not a transcript; git history holds the detail.

## Key References

- `.github/README-CODE-STYLES.md` — code style (authoritative).
- `.github/README-BUILDS.md` — build, toolchain, packaging, and platform details.
- `project-docs/PLAN_PROJECT.md` — approved plan.
- `project-docs/docs-data/NEW-ATTRIBUTES.csv` — SVG `data-*` attribute specification.
- `project-docs/docs-data/SVG-DATA-ATTRIBUTES.md` — SVG attribute contract.
- `SESSION_HANDOVER.md` — current cross-session state.
