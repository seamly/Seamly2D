# CLAUDE.md

## Communication Style

- Communicate tersely.
- Lead with the result.
- Default to a few sentences or short bullets.
- Do not restate my request.
- Do not narrate routine actions or explain obvious steps.
- Ask a question only when you cannot proceed safely without the answer.
- After making changes, report only:
  1. what changed,
  2. whether verification passed,
  3. any decision or action needed from me.
- Provide detailed explanations only when I request them.

### Writing Style

Applies to all prose I write for you: chat replies, commit messages, code comments,
Doxygen blocks, `TODO_*.md` / `project-docs/**` entries, `SESSION_HANDOVER.md`, and README text.

- **Write in ASD-STE100 (Simplified Technical English), adapted for software development.**
  - One idea per sentence. Keep instructions to 20 words or fewer, descriptions to 25 or fewer.
  - Use the active voice. Name the actor: "the installer writes the key", not "the key is written".
  - Use one word for one meaning, and keep the same word for the same thing throughout a document.
    Do not use synonyms for variety ("build" stays "build"; it does not become "compile" or "make").
  - Use the imperative for instructions: "Run `scripts/sd.ps1`", not "You should run...".
  - Use simple verb tenses — present, simple past, simple future. No perfect or progressive forms
    where a simple one works.
  - Do not drop articles or helper words. Write "delete the file in the build directory".
  - Avoid noun stacks longer than three words: "settings file name", not "user settings file name
    format string".
  - Technical terms, API names, file paths, and identifiers are exempt — write them exactly.
- **Follow Zinsser's four principles:**
  - **Simplicity** — cut every word that does no work. No jargon where a plain word fits.
  - **Brevity** — the shortest version that stays complete. No throat-clearing, no summary of what
    I am about to say, no restatement at the end.
  - **Clarity** — the reader must not have to re-read a sentence. Put the subject early. Prefer a
    concrete noun to an abstraction.
  - **Humanity** — write to a person, not to a template. Plain, direct, honest about what is unknown
    or unverified. No filler enthusiasm and no false confidence.

## Seamly Apps

- Seamly2D & SeamlyLayout & SeamlyMe apps distributed in a single Qt 6.11.1 runtime
- Shared libraries under `src/libs/` (`vlayout`, `vformat`, `vpatterndb`, `ifc`, ...)

1.Seamly2D

- `src/app/seamly2d/` - pattern drafting app — parent app of the Seamly family
- written in Cpp (code) & Qt 6.11/QtWidgets (GUI)
- built with qmake/make/make install
- Seamly2D Header for new files and edited files:
  -- **Author:** slspencer
  -- **Copyright:** 2026 Seamly2D Project
  -- **License:** GPL-3.0-or-later

2.SeamlyLayout

- `src/app/seamlylayout/` — daughter layout app, creates "layouts" of pattern pieces (defined in Seamly2D) that are ready for use in electronic fabric cutting electronic tables, fabric cutting manual actions, & additional software including 3D software and future Seamly 3D apps.
- written in Rust (code) + Ice (GUI), converted to Cpp (code) + Qt 6.11/QML/QtWidgets (GUI) by Rust's cxx_qtbridge library
- built with `src/app/seamlylayout/qd.ps1` which will be incorporated into a single CI/CD build script that builds Seamly2d/SeamlyMe/SeamlyLayout that will run in a single Qt runtime
- has its own CLAUDE.md and rules.md that should be merged into the project claude.md and rules.md files.
- SeamlyLayout file header for new files and edited files:
  -- **Author:** slspencer
  -- **Copyright:** 2026 Seamly2D Project
  -- **License:** MIT

3.SeamlyMe

- `src/app/seamlyme/` - daughter measurement app, creates .smis individual measurement files containing an individuals or a single boutique or industry-defined size, and .smms multisize measurement files that define a range of boutique or industry-defined sizes. Enables these measurement files as inputs into Seamly2D pattern .sm2d files
- written in Cpp (code) & Qt 6.11/QtWidgets (GUI)
- built with qmake/make/make install
- SeamlyMe file header for new files and edited files:
  -- **Author:** slspencer
  -- **Copyright:** 2026 Seamly2D Project
  -- **License:** GPL-3.0-or-later

## Build Notes

All three apps — seamly2d, seamlyMe and seamlyLayout — build against **Qt release, 6.11.1**

- **CI toolchain (GitHub runner):** -- Qt 6.11.1 + MSVC 2022 — `QT_VERSION` in `ci.yml`, `seamlylayout-ci.yml` and `windows-msi.yml` must all name this one release
- **Local toolchain (developer Windows 11 PC, check builds of current work):** -- Qt 6.11.1 `msvc2022_64` + VS 18 Community MSVC (`vcvars64.bat`)
  -- seamly2d -- qmake + jom
  -- seamlyLayout -- CMake + Ninja + Cargo
  -- seamlyMe -- qmake + jom
- release shadow-build in `build/` (gitignored)
- local debug build in `scripts/sd.ps1` ("seamly2d debug")
  -- auto-detects the newest Qt msvc2022_64 kit (6.11.1 or newer) under `C:\Qt` and the VS 18 Community MSVC environment
  -- shadow-builds `CONFIG+=debug` into `scripts/seamly2d-debug/` (gitignored)
  -- the debug exe lands at `scripts/seamly2d-debug/src/app/seamly2d/bin/seamly2d.exe` with Qt debug DLLs deployed by windeployqt
  -- `-Run` launches it after the build; see the script's `.SYNOPSIS` for details.
- **Local Qt kit must include**:
  -- `qtwebengine`, `qtwebchannel`, & `qtpositioning`
  -- `WebEngineView`, `QtWebEngineQuick`, `Qt6WebEngineCore` for seamlyLayout's `SvgCanvas.qml`.Qt6WebEngineCore's CMake config requires the other two, so a kit without them fails `find_package(Qt6 ... WebEngineQuick)` at configure time. The Qt online installer does not pull them in automatically when you tick Qt WebEngine.

## Coding Rules

- **`.github/README-CODE-STYLES.md` is the authoritative style guide** — JSF-AV C++ with the deviations listed there. Read it before writing or renaming code; the points below are the ones that come up constantly, not a replacement for it
- **File naming:** snake_case, all lowercase; name files for what they do, using the prefixes the style guide lists (`settings_*`, `dialog_<toolgroup>_<toolname>`, `tool_*`, `model_*`, `options_*`, `test_*`, `application_<appname>`, `<platform>_*`, …). No abbreviations, no generic names (`util.h`, `helpers.cpp`). **Unique repo-wide: a search for `<filename.extension>` must return exactly one file** — the one carve-out is seamlyLayout's crate roots, where multiple `lib.rs` are accepted and told apart by path. Do NOT start a file name with a bare `s` — that older anti-`v` convention is superseded by the prefix list. New source files still must not begin with `v`; existing `v*` files keep their names when edited unless a task renames them
- **Class naming, and the file that holds it:** classes are UpperCamelCase (the project's deliberate deviation from JSF-AV). A file that **primarily defines one class** is the exception to snake_case — name it exactly like the class, in UpperCamelCase: `SettingsCommon.h` / `SettingsCommon.cpp` ↔ `class SettingsCommon`
- **License headers:** every new file and file edit in seamly2d and seamlyme gets a GPLv3-or-later header with copyright 2026 Seamly2D Project and author slspencer (follow the existing header block style, e.g. `src/libs/vformat/svg_generator.cpp`); every new file in seamlyLayout gets an MIT license header with copyright 2026 Seamly2D Project and author slspencer
- **Documentation:** all new code and every modified function gets a Doxygen-compatible `@brief` (plus `@param`/`@return` where applicable) and inline comments so an intermediate-level programmer can follow the workflow, control flow, and data flow
- **Markdown lint:** ignore MD041 (first-line-heading) warnings — they are editor diagnostic noise, not blocking issues; do not restructure files to silence them

## Git Remotes

- `origin` = `https://github.com/seamly/Seamly2D.git`, branch `run-seamlyLayout` — the work repo and feature branch; ALL changes are merged locally to `run-seamlLayout`  then pushed to origin
- `upstream` = `https://github.com/FashionFreedom/Seamly2D` — the public parent project; **NEVER push to it or open PRs against it** (fetch only; its push URL is set to `DISABLED_NEVER_PUSH` in the local clone to enforce this)

### Branch strategy and endgame (decided 2026-07-18)

- `seamly/Seamly2D` deliberately remains a GitHub **fork** of `FashionFreedom/Seamly2D` — do NOT detach / "Leave fork network"; the fork link is required for the final upstream PR
- `develop` on origin is a **pristine mirror of upstream develop**: update it only by syncing from upstream; never merge project work into it before the endgame
- All project work accumulates on `run-seamlyLayout`; keep it current by merging `develop` into it (that direction only)
- **Endgame:** when the project is finished, push `seamly:run-seamlyLayout` → `FashionFreedom:run-seamlyLayout` — the single sanctioned upstream PR, created by the user (not by Claude)
- Task work does **not** go through a PR — task branches merge into local `run-seamlyLayout`, which is then pushed to origin (see the task workflow below)
- GitHub's green "Compare & pull request" banner on the fork defaults to FashionFreedom as base and cannot be disabled — NEVER USE IT; if a PR is ever needed, target `seamly:run-seamlyLayout` via `gh` (default repo pinned to origin) or from the fork's own branch pages

## Task Tracking

- `project-docs/PROJECT_PLAN.md` — the current approved implementation plan
- Task lists are split by area — the task's own `TODO_*.md` file is the one to read and update. `project-docs/TODO_MIGRATE.md` is the hub and cross-references the others; the set is not fixed, so **list `project-docs/TODO_*.md` and follow those cross-references rather than relying on any list here.** Current files: `TODO_MIGRATE.md`, `TODO_SEAMLY2D.md`, `TODO_SEAMLYLAYOUT.md`, `TODO_SEAMLYME.md`, `TODO_SEAMLYTEAM.md`, `TODO_CLI.md`, `TODO_CODE_SIGNING.md`, `TODO_RENAME_SETTINGS_FILES_CLASSES.md`, `TODO_INSTALLER.md`, `TODO_INSTALLER_WIN_X64.md`, `TODO_INSTALLER_WIN_ARM64.md`, `TODO_INSTALLER_MACOSX.md`, `TODO_INSTALLER_LINUX_APPIMAGE.md`, `TODO_INSTALLER_LINUX_FLATPAK.md`, `TODO_FUTURE.md`
- Each `TODO_*.md` file holds tasks with numbered checkbox subtasks; check off subtasks as they are accomplished
- `project-docs/TODO_COMPLETED.md` — when all subtasks of a task are complete, move the task here from its `TODO_*.md` file
- `project-docs/WONT_DO_MIGRATE.md` — tasks dropped from `TODO_MIGRATE.md`; kept for reference, never worked on

### Task workflow (required for every prompt that says to implement a task/subtask from a `TODO_*.md` file)

Run these steps in order, every time, without being asked:

1. **Sync `develop`** — `git fetch origin`; if local `develop` is behind `origin/develop`, fast-forward it. Never merge project work into `develop`.
2. **Sync `run-seamlyLayout`** — merge (or fast-forward) local `develop` into local `run-seamlyLayout`; that direction only.
3. **Branch** — create a task branch off `run-seamlyLayout` (`task-<short-name>`) and do all work there.
4. **Implement** the task.
5. **Test** — write unit tests wherever the task adds or changes code, run them, and run a local check build (`scripts/sd.ps1`) to verify the change works. Report failures; do not proceed past a red build/test.
6. **Update task tracking** — check off the task's subtasks in its `TODO_*.md` file; move fully completed tasks to `project-docs/TODO_COMPLETED.md`; update `SESSION_HANDOVER.md`.
7. **Stage and commit** on the task branch.
8. **Merge** the task branch into local `run-seamlyLayout` with `--no-ff`, so the merge always produces a commit whose message step 9 can mark.
9. **Push** local `run-seamlyLayout` to `origin run-seamlyLayout` --> NEVER push to the public upstream `FashionFreedom/Seamly2D`. Always push: origin is the branch of record and the only offsite copy of the work.
10. **Report** to the user: what changed, whether tests/build passed, and anything needing their decision. Then delete the local task branch.

No PR is required for this flow — the merge happens locally and reaches origin by the push in step 9.

#### CI cost control (decided 2026-08-10)

A push to `run-seamlyLayout` starts `ci.yml`, which builds the family on Linux, macOS, and Windows x64/arm64 — roughly **50 minutes**. Step 5's local build already verifies the change on the developer PC, so running that suite on every task merge is mostly redundant. Do not drop the push to avoid it; suppress the CI run instead:

- **Put the skip token in the step 8 merge commit subject by default.** The token is the word `skip` and the letters `ci` in square brackets. GitHub reads the pushed head commit's message, which is why step 8 uses `--no-ff`.
- **NEVER write the token literally in prose — not in a commit message, not even in a sentence saying it is absent.** GitHub matches it anywhere in the head commit message, not just the subject, and does not care that the surrounding words are a negation. On 2026-08-11 a merge commit explaining this very rule contained the token inside "No `[skip ci]` on this merge" and skipped the run it was describing; only CodeQL (GitHub default setup, not governed by the token) ran, and the branch had to be rewritten and force-pushed. In commit messages write "the skip token" or "skip-ci"; the literal form belongs only where it takes effect. This file is the exception — it has to show the token to define it, and CLAUDE.md is never a commit message.
- **Omit the token — let the full suite run — when the task touched anything the local Windows build cannot verify:** `.github/workflows/**`, `scripts/packaging/**`, any `*.pro` / `*.pri` / `CMakeLists.txt` / `Cargo.toml`, or platform-specific code (Linux/macOS paths, `#ifdef Q_OS_*`, arm64 handling). These are exactly the changes that only fail on the runners.
- **Comment-only carve-out: keep the token even for those paths when the change cannot alter build output.** A commit that only adds, edits, or deletes comments — XML comments in `seamly-family.wxs`, `#` comments in a workflow, a Doxygen block — produces the same MSI, the same binaries, and the same YAML behavior. Read the diff before you decide: if every changed line is a comment or a documentation string, use the token. If one line of real authoring moved with it, omit the token and let the suite run. On 2026-08-11 commit `a0e70635a7` (13 lines, all XML comments in `seamly-family.wxs`) started a full ~50-minute run for nothing.
- **`paths-ignore` reads the whole push, not the head commit.** A push that carries a docs commit on top of a code commit still starts `ci.yml`, and the run is *titled* with the head commit — so a docs subject on a triggered run means an earlier commit in the same push touched a build input. The skip token is the only per-push lever, and it is read from the head commit's message; a push of mixed commits therefore needs the token on the head commit, or it needs the run.
- **Run the full suite deliberately before a milestone** (a release, or handing the branch to the upstream PR): `gh workflow run ci.yml --ref run-seamlyLayout`, then wait for it to go green. Skipped CI is deferred CI, not cancelled CI — a run of accumulated skips must be cleared before the branch is considered shippable.

Supporting workflow config, so a skip is the only thing left to decide: all three workflows carry `concurrency` with `cancel-in-progress: true` (several merges in one session collapse to one run instead of queueing N), and `ci.yml`'s push trigger has a `paths-ignore` for `**.md` / `project-docs/**` so documentation never triggers a build.

- **Docs-only exception:** when a commit changes only `.md`, `.txt`, and/or `.svg` files (no code), skip steps 1–5 and 8–9 — stage and commit locally on the current branch only; do not push to origin
- **`SESSION_HANDOVER.md`** (repo root) — keep it current with the session's state: update it before compaction and when finishing a task. It is the next chat session's starting point, so it must carry what git does not — the current task and its exact progress, which `TODO_*.md` / `project-docs/TODO_COMPLETED.md` entries moved, key decisions and the reasoning behind them, files changed, concrete next steps, and any machine state changed outside the repo. The `PreCompact` / `PostCompact` hooks in `.claude/settings.json` only surface a reminder; keeping the file current is required regardless of whether that reminder appears

## Key References

- `project-docs/NEW-ATTRIBUTES.csv` — SVG `data-*` attribute spec for the SeamlyLayout handoff
- Test pattern: `src/app/seamlylayout/input/richmond-shirt_v1_v061-test.sm2d`
- `.github/README-BUILDS.md` — build knowledge base (toolchains, per-platform packaging, settings/data locations, packaging decisions); keep it updated when build knowledge changes
