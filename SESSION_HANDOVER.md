# Session handover

## Current state (2026-07-27, latest session): Task 49 DONE — the Seamly2D → SeamlyLayout handoff finally opens the pattern

**Branch:** `task-49-seamlylayout-svg-argument`, branched from `run-seamlyLayout` (which was already level with `origin/run-seamlyLayout`; local `develop` = `origin/develop` = `057e95bfca`, nothing to merge). Task-branch + PR cycle per `CLAUDE.md`.

### What Task 49 changed

SeamlyLayout read `argc`/`argv` only to hand them to `QApplication`, so the `.pieces.svg` seamly2d wrote and passed was discarded and the window came up empty.

| File | Change |
| ---- | ------ |
| `src/app/seamlylayout/qt_frontend/src/StartupOptions.{h,cpp}` | **New.** Value class (no QObject, no GUI) parsing the one positional `<svg-file>` with `QCommandLineParser::parse()` — the non-exiting sibling of `process()` — and validating it. Four statuses: `NoFile`, `OpenFile`, `ShowInformation` (`--help`/`--version`), `Failed` |
| `src/app/seamlylayout/qt_frontend/main.cpp` | Parses after the app metadata is set (so `--version` can report it); dispatches on `QTimer::singleShot(0, …)` **after** the event loop starts, because the QML window and its WebEngine canvases must exist first |
| `src/app/seamlylayout/qt_frontend/qml/Main.qml` | New `openSvgFile(localPath)` (the file dialog now calls it too — one entry point) and `reportStartupError(message)`; new `onImportWarning` handler |
| `crates/cxxqt_bridge/src/piece_extractor.rs` | New `count_tagged_pieces()` (recursive `data-type="piece"` count) + 3 unit tests |
| `crates/cxxqt_bridge/src/lib.rs` | New `import_warning` qsignal; `import_svg` emits it when the imported SVG carries no piece tagging — **a warning, never an error**, because untagged SVGs still lay out |
| `src/libs/vmisc/seamly_family_paths.{cpp,h}` | New `piecesSvgFilePath()` and `seamlyLayoutLaunchArguments()` — the seamly2d half of the contract, extracted out of `mainwindow.cpp` so it has one definition and one test |
| `src/app/seamly2d/mainwindow.cpp` | `exportPiecesToSeamlyLayout()` now calls both; added the `seamly_family_paths.h` include |
| `src/test/SeamlyLayoutTest/StartupOptionsTests.cpp` | **New**, 5th ctest target (guiless), 18 cases |
| `src/test/Seamly2DTest/tst_seamlyfamilypaths.{cpp,h}` | 6 new cases for the contract |
| `src/app/seamlylayout/CLAUDE.md`, `project-docs/SVG-DATA-ATTRIBUTES.md` | The contract, written down on both sides |
| `project-docs/TODO_MIGRATE.md` / `TODO_COMPLETED.md` | Task 49 moved across; **new Task 59 filed** (below) |
| `.claude/settings.json` | Allowlist entries for the repo's build/test scripts + `ctest` (the user asked for this mid-session after a prompt on `sd.ps1`) |

### Decisions recorded in the contract (do not quietly reverse them)

- **No single-instance handling** — every launch is its own process and window; one document per process (no tabs), which is also why a *second* positional argument is rejected rather than queued.
- **A bad argument does not exit.** The message goes to the QML error dialog and the app stays open with an empty canvas — a detached launch has no console to print to.
- **`--help`/`--version` go to a `QMessageBox`**, because this is a WIN32-subsystem binary with no console on Windows.
- **Untagged SVGs are opened, not refused** — warning only.

### Verification (all local, all passing)

`scripts/sd.ps1` exit 0 · `scripts/st.ps1` **32133 passed / 0 failed, 25 suites** (`TST_SeamlyFamilyPaths` 15 → 21) · `ParserTest` 0 · `TranslationsTest` 0 · SeamlyLayout `ctest --preset debug` **5/5** (`StartupOptionsTests` 19 passed / 1 skipped — the unreadable-file case, NTFS) · `cargo test --workspace` **252 / 0 across 20 targets**.

**End-to-end, with a genuine handoff file:** `seamly2d.exe <pattern>.sm2d -b handoff -d <dir> -f 0 --exportOnlyDetails` produces a tagged SVG through the same `exportSVG()` `generatePiecesSvg()` uses (12 `data-type="piece"` groups). `SeamlyLayout.exe <that file>` logs `main(): opening startup file …` then `[import_svg] 12 tagged pattern piece(s) found`. The untagged baseline SVG logs the `no data-type="piece" groups` warning; a non-existent path logs the startup error. **All three paths confirmed.**

### THE CollectionTest LOOSE END IS CLOSED — it is pre-existing

The previous session's unfinished baseline check was completed here. With every Task 49 source change stashed and the tree rebuilt, `TST_Seamly2DCommandLine::TestOpenCollection(07_armhole_adjustment_010)` fails **identically** (42 passed / 1 failed). Unrelated to Tasks 49/50. Two facts worth keeping:

- **`CollectionTest.exe` must be run with its working directory set to its own `bin/`.** `initTestCase()` removes `tst_seamly2d_tmp` *relative to the CWD* but creates it under `applicationDirPath()`, so running it from anywhere else aborts on a leftover directory from the previous run ("Fail to prepare test files for testing").
- The failure presents as either "Program crashed" or "The finish operation timed out" against `AbstractTest::Run()`'s 120 s limit.

### NEW — Task 59, filed not fixed: the layout packs the whole pattern as one piece

Found by the end-to-end run and **the most valuable thing in this session after the fix itself.** `piece_extractor::extract_piece_rects()` treats each direct child `<g>` of the SVG root as a piece, but the tagged handoff nests all 12 pieces inside one `<g id="pattern-1" data-type="pattern">`. The packer therefore receives a single sheet-sized object: `0 placements, 1 unplaced: ["pattern-1"]`. Task 49 made the handoff *open*; Task 59 (`project-docs/TODO_MIGRATE.md`, bottom) makes it *lay out*. It should be the next task.

### Loose ends carried forward

- `src/app/seamly2d/core/BUILD_PROBLEMS.txt` — the user said to delete it if it is not useful; **not done in this session** (out of Task 49's scope).
- The user's own uncommitted edits to `SESSION_HANDOVER.md` and `project-docs/TODO_SEAMLYLAYOUT.md` (Task 57 deleted) were present at session start and are folded into this branch's commit.

## Earlier state (2026-07-27, later session): Tasks 45 and 50 done — committed and pushed to `run-seamlyLayout`

**Branch:** `run-seamlyLayout`, committed directly (no PR) — the user explicitly asked for stage + commit + push to origin `run-seamlyLayout`, skipping the `develop` pull and the task-branch/PR cycle in `CLAUDE.md`.

### What was asked

The session opened with an analysis question: *which task in `project-docs/TODO_MIGRATE.md` should be done next?* The answer given was **Task 49** (SeamlyLayout ignores the SVG path argument — verified still true: `qt_frontend/main.cpp` passes `argc`/`argv` to `QApplication` and never reads them; `AppController::import_svg()` already exists at `crates/cxxqt_bridge/src/lib.rs:863` and is what the QML Import button calls at `qml/Main.qml:850`, so the plumbing for the fix is in place). Every other open task in that file is blocked on hardware (clean VM / arm64 / macOS / Linux), on KMS credentials, or on a user decision. **The user then chose Tasks 45 and 50 instead.** Task 49 remains the recommended next item.

### Task 45 — stale Qt 6.10.1 paths in the Claude allowlists (DONE, moved to `project-docs/TODO_COMPLETED.md`)

Both entries turned out to be **redundant with broader rules already present**, so the fix is removal, not a version bump — which makes them permanently version-agnostic:

- `.claude/settings.json:154` — the compound `Test-Path "C:\Qt\6.10.1\…"; & …vswhere.exe …` was already covered by `PowerShell(Test-Path *)` on line 10. Replaced with one version-agnostic prefix entry naming no Qt version: `PowerShell(& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" *)`
- `.claude/settings.local.json:17` and `:19` — both deleted. That file opens with `PowerShell(*)` / `Bash(*)`, which already allow everything in it. **This file is gitignored (`.gitignore:189`)**, so that half exists only on this machine and is *not* in the commit

Both files re-validated as parseable JSON.

### Task 50 — hard-coded developer path in `application_2d.cpp` (DONE, moved to `project-docs/TODO_COMPLETED.md`)

- **New `SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(startDirectory)`** in `src/libs/vmisc/seamly_family_paths.{cpp,h}`. Walks up from the running executable's directory, testing each ancestor as a checkout root for `<root>/src/app/seamlylayout/qt_frontend/build/<config>/SeamlyLayout(.exe)`. Both shadow-build layouts resolve without being named — release `<checkout>/build/…` (5 levels) and `sd.ps1`'s debug `<checkout>/scripts/seamly2d-build-debug/…` (6). **Bounded at 8 parents** so it cannot climb to the filesystem root. **Release preferred over Debug** (the old path pinned Debug unconditionally). Put in `vmisc` beside `locateSeamlyLayout()` so the test suite reaches it, and **parameterized on the start directory** per the Task 34/53 rule, so tests use `QTemporaryDir`
- `application_2d.cpp:515` now calls it; the dev build stays **last** in the lookup chain, after the configured setting and the installed copy, so a source tree can never shadow an installation
- **Coding-rules note** added to `.github/README-CODE-STYLES.md`: "No absolute machine-specific paths in source", with allowed alternatives, an explicit carve-out for placeholder comments and test data, and a `git grep` command

**A CI gate was deliberately not added** — measured first, and a naive grep is unusable: `tst_misc.cpp` has ~20 synthetic `/home/user/...` rows, `tst_dataroot.cpp` uses `C:/Users/tester/...`, and `vcommonsettings.cpp` / `PreferencesModel.cpp` carry `C:/Users/<user>/...` Doxygen placeholders — all legitimate. Telling a real home directory from a placeholder needs a human.

### Flagged, NOT fixed — decide before the upstream PR

**`src/app/seamly2d/core/BUILD_PROBLEMS.txt` is tracked and carries ~45 absolute `/c:/Users/susan/Projects/Seamly2D-private/…` paths** — the same leak Task 50 just closed in code, and it names the *private* repo directory. It is the clangd dump described further down this file (editor noise; the qmake build compiles those files clean). Deleting a tracked file was outside the task's scope. This is the single most likely thing to embarrass the upstream PR. --> user says to delete `src/app/seamly2d/core/BUILD_PROBLEMS.txt`if it isn't useful.

### Verification — read this before trusting the state

| Check                              | Result                                                                                                                                                                     |
| ---------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `scripts/sd.ps1` debug build     | **Clean, exit 0**                                                                                                                                                    |
| `scripts/st.ps1` (Seamly2DTests) | **32127 passed, 0 failed across 25 suites**, exit 0. `TST_SeamlyFamilyPaths` 5 → 13 cases (reported as 15 with init/cleanup); total up exactly +8                 |
| `ParserTest`                     | exit 0                                                                                                                                                                     |
| `TranslationsTest`               | exit 0                                                                                                                                                                     |
| `CollectionTest`                 | **exit 1 — 42 passed, 1 failed.** `TST_Seamly2DCommandLine::TestOpenCollection(07_armhole_adjustment_010)` "Program crashed", `tst_seamly2dcommandline.cpp:302` |

**The CollectionTest failure is pre-existing per the user, but that was NOT confirmed.** Evidence it is unrelated: the only caller of the changed `seamlyLayoutFilePath()` is `mainwindow.cpp:4136` inside `exportPiecesToSeamlyLayout()`, the GUI Layout Mode handoff, which a console `seamly2d --test <pattern>` run never enters. The definitive check — stash the change, rebuild, rerun that one case — was started and **interrupted by the user before the baseline build ran**, so it is unfinished. Note also that the previous session's handover records `ParserTest` and `TranslationsTest` as verified but **never mentions running `CollectionTest`**, so there is no known-good baseline for it either way.

**Next session: finish that check.** `git stash push` the four source files, run `scripts/sd.ps1`, run `CollectionTest.exe -o <file>,txt`, compare, `git stash pop`. If it fails on the baseline too, file it as its own task.

**A stash hazard was hit and cleared:** the source changes were stashed for that baseline test and the session was interrupted while stashed. They were restored with `git stash pop` (all 9 files back, stash dropped) before committing. If a future session interrupts mid-stash, check `git stash list` first. --> user pushed to origin/run-seamlyLayout, currently nothing to commit.

### Files changed

| File                                                         | Change                                                                                             |
| ------------------------------------------------------------ | -------------------------------------------------------------------------------------------------- |
| `src/libs/vmisc/seamly_family_paths.cpp` / `.h`          | New`locateSeamlyLayoutDevBuild()`; file-local `sourceTreeBuildSubPath` and `maxUpwardLevels` |
| `src/app/seamly2d/core/application_2d.cpp`                 | Hard-coded path replaced by the call;`seamlyLayoutFilePath()` Doxygen updated                    |
| `src/test/Seamly2DTest/tst_seamlyfamilypaths.cpp` / `.h` | 8 new cases, all`QTemporaryDir`                                                                  |
| `.github/README-CODE-STYLES.md`                            | New "No absolute machine-specific paths in source" rule                                            |
| `.claude/settings.json`                                    | Line 154 replaced with the version-agnostic vswhere entry                                          |
| `project-docs/TODO_MIGRATE.md`                             | Tasks 45 and 50 removed                                                                            |
| `project-docs/TODO_COMPLETED.md`                           | Tasks 45 and 50 added at the top with full write-ups                                               |
| `.claude/settings.local.json`                              | Two entries deleted —**gitignored, not in the commit**                                      |

### Next steps

1. **Finish the CollectionTest baseline check** (above) — the one loose end of this session.
2. **Task 49** — the recommended next task; see the analysis at the top of this section.
3. Decide the fate of `BUILD_PROBLEMS.txt`.
4. The four decisions the user still owes are unchanged — see "Four decisions the user still owes" below --> user added answers to the four decisions.

## Earlier state (2026-07-27): Task 58 merged; documentation reorganized

**Branch:** `run-seamlyLayout`. **Task 58 is DONE** (moved to `project-docs/TODO_COMPLETED.md` — see below) and two documentation reorganizations landed alongside it.

### What happened this session

| Item                                                                                 | Outcome                                                                                                                                                                                              |
| ------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Task 58** — migrate the SeamlyLayout tests to `src/test/SeamlyLayoutTest` | **DONE, merged** via PR [#20](https://github.com/seamly/Seamly2D/pull/20) (`8ddab2a4c3`), all 12 CI checks green. Task written up in `project-docs/TODO_MIGRATE.md` first, then implemented |
| **`status-docs/` → `project-docs/`**                                      | Merged via PR[#21](https://github.com/seamly/Seamly2D/pull/21) (`b89fbe161d`), plus a local merge by the user — see the divergence note below                                                      |
| **Tracking docs moved + SeamlyLayout status docs prefixed**                    | Committed on branch`reorganize-project-docs`, **not yet pushed**                                                                                                                             |

### Task 58 — what moved and what deliberately did not

The four Qt/C++ suites (`AdjustSceneTests`, `AdjustControllerTests`, `PreferencesModelTests`, `SettingsModelTests`) moved from `src/app/seamlylayout/qt_frontend/tests/{adjust,preferences,settings}/` to a flat `src/test/SeamlyLayoutTest/`, matching the sibling `Seamly2DTest`. Git recorded pure renames; no source edits were needed because every project include resolves through `target_include_directories(… src/)`.

**Three decisions that must not be undone:**

1. **The Rust tests stay in `crates/`.** `#[cfg(test)]` modules compile as part of their crate and reach its private items, and Cargo requires integration tests beside the crate's `Cargo.toml`. Moving them would need per-crate `[[test]] path = "../../../../test/…"` entries that break `cargo test -p <crate>`.
2. **`src/test/test.pro` must never list `SeamlyLayoutTest`.** seamlyLayout is CMake + Cargo and stays out of the Seamly2D qmake build. All `SUBDIRS` in `Seamly.pro`, `src/src.pro` and `src/test/test.pro` are explicit literals with no globbing, so the directory cannot be picked up by accident.
3. **`seamlylayout-ci.yml`'s path filters now include `src/test/SeamlyLayoutTest/**`.** Without it a test-only change triggers *no* CI at all — the filters were `src/app/seamlylayout/**` only. `ci.yml` has no path filters, so the parent jobs are unaffected either way.

### Two pre-existing defects fixed inside Task 58

- **`src/app/seamlylayout/build.ps1:117`** probed for a CMake package named `Qt6WebEngine`, which Qt has never shipped (the packages are `Qt6WebEngineCore` / `Qt6WebEngineQuick` / `Qt6WebEngineWidgets`). The guard fired on *every* correctly installed kit, aborting the build while telling the developer to install modules already present. Now probes `Qt6WebEngineQuick`.
- **`ctest --preset debug` could not run on Windows** from a shell that had not sourced the Qt kit — the test exes launch out of the build tree with no windeployqt output beside them. Added a `WIN32`-guarded `ENVIRONMENT_MODIFICATION` prepending `$<TARGET_FILE_DIR:Qt6::Core>` to `PATH` for the test run only, as a generator expression so it follows whichever kit CMake found.

Verified locally: `ctest --preset debug` with no Qt on `PATH` → 4/4 suites, **107 cases** (26 + 7 + 48 + 26), 1 skipped; `PreferencesModelTests`' 48 matches its pre-move count. `cargo test --workspace` → **251 tests across 22 targets**, 0 failures.

### Documentation reorganization

`status-docs/` → `project-docs/`, with `new-attributes.csv` → `NEW-ATTRIBUTES.csv` and `svg-data-attributes.md` → `SVG-DATA-ATTRIBUTES.md`. The empty `status-docs/baseline/` shell went with it; its SVG survives byte-identically (line endings aside) at `src/app/seamlylayout/input/richmond-shirt-baseline_pieces.svg`.

Then, on the **unpushed `reorganize-project-docs` branch**: `PROJECT_PLAN.md` and all six `TODO_*.md` files moved into `project-docs/`, and SeamlyLayout's status docs gained an app-name prefix (`SEAMLYLAYOUT_COMPLETED.md`, `SEAMLYLAYOUT_DECISIONS.md`, `SEAMLYLAYOUT_TODO_FUTURE.md`, `SEAMLYLAYOUT_MIGRATION_STATUS.md`, `TODO_SEAMLYLAYOUT_2.md`).

**`SESSION_HANDOVER.md` stays at the repository root** — a deliberate user decision, reversing an initial move. Both `.claude/settings.json` compaction hooks name it there, and that wording is correct as written. Do not move it.

**`SEAMLYLAYOUT_TODO_FUTURE.md` was `FUTURE_TODOs.txt` and untracked** — `src/app/seamlylayout/.gitignore` ignores `*.txt`. The `.md` extension brought it into version control for the first time, on the user's explicit instruction.

**`src/app/seamlylayout/docs/status-docs/` keeps its directory name** — only the repo-root `status-docs/` was renamed. Several lines name both in one sentence, so reference rewrites were anchored on a leading backtick, which the daughter-app mirrors never carry.

### Repository state — read this before pushing

Local `run-seamlyLayout` is **0 commits ahead of origin, 0 behind**. The rename branch was merged **twice** — once locally by the user (`82ed9fd7de`), once by GitHub closing PR #21 (`b89fbe161d`) — producing two merge commits with identical content. `21772605f7` reconciles them; that merge introduced **zero file changes**. Local also uniquely carries the user's `54a572ad06` ("deleted unused image files and updated README-CODE-STYLES.md"), which is not on origin.

## Earlier state: Tasks 34 and 53 are DONE and moved to `project-docs/TODO_COMPLETED.md`

**Date:** 2026-07-26. **Branch:** `run-seamlyLayout` at **`1d74f7e18a`** ("merge develop"), **pushed — local and `origin/run-seamlyLayout` are identical, working tree clean.** Task 53 landed via **PR [#19](https://github.com/seamly/Seamly2D/pull/19)** (`task-53-seamlydata-root` → `run-seamlyLayout`) — **all 11 CI checks green** (Windows x64 27m4s, Windows arm64 cross-compile 26m28s, macOS 16m51s, Linux AppImage 9m3s, Linux unit tests 9m55s, CodeQL, CodeSee, Analyze actions/python/rust, version) — **merged**, task branch deleted locally and on origin. Task 34 landed earlier the same day.

**`develop` was merged into `run-seamlyLayout`** (`1d74f7e18a`, the sanctioned direction — never the reverse). Local `develop` = `origin/develop` = `057e95bfca`. It brought in real upstream **code**, so "nothing has changed since PR #19" is no longer true: formula-reference guards (`vabstractpattern`, `vdrawtool`, `vtoolline`, `savetooloptions` — issues #1364/#1521), the pointname combobox colour fix (#1636), Finnish translations (#1637), and a CI consolidation that **deleted `.github/workflows/feat-ci.yml`** and folded feature-branch builds into `ci.yml`. **These changes have not been built or tested locally in this session.**

Refer to `project-docs/TODO_MIGRATE.md`, `project-docs/TODO_SEAMLY2D.md` and `project-docs/TODO_SEAMLYLAYOUT.md` for the tasks still open.

**Theme of this session:** the user-data root, then naming. Task 34 renamed and generalized the data root; Task 53 renamed it again (to `seamlyData`), made the settings migration self-cleaning, and fixed up the developer's own machine. The tail of the session moved four items out of `future-todos.md` into the task lists and settled which document owns the naming rules.

### What was done this session

| Step                                                       | Outcome                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| ---------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Task 34**                                          | **Done.** `~/seamly2d` → `~/seamly`, made relocatable via `paths/dataRoot`; `chooseFirstRunDataRoot()`, `ensureDataRootTree()`, `rebaseOntoDataRoot()`, `mergeStrayCommonSettings()`; `TST_DataRoot` added (16 cases)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| **Task 53**                                          | **Done.** Default root renamed `~/seamly` → **`~/seamlyData`**; `mergeStrayCommonSettings()` now verifies then **deletes** the stray; new `pruneEmptyLegacyDataRoot()` removes the empty `~/seamly2d` skeleton. `TST_DataRoot` 16 → **22 cases**                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| **Task 14**                                          | **Extended, not implemented.** The user asked whether the installer could check-then-move an existing data tree before anything is removed; it cannot today, so nine subtasks were folded into Task 14 under an "Update (2026-07-26) — moving an existing data tree" note                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| **Task 52**                                          | **Extended.** Gained a new *first* subtask: stop `CollectionTest` writing into the real user settings **before** repointing the `vsettings.cpp` accessors                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| **Developer machine**                                | **Migrated and verified** — see the table below. This is state *outside* the repo; nothing in git records it                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| **Tasks 54, 55, 57**                                 | **Filed, not started**, moved out of `future-todos.md` (docs-only commits). **54** (`project-docs/TODO_MIGRATE.md`) renames the three `vmisc` settings **files *and* classes** — `settings_common`/`SettingsCommon`, `settings_seamlyme`/`SettingsSeamlyMe`, `settings_seamly2d`/`SettingsSeamly2D` — per `.github/README-CODE-STYLES.md`; ~620 class occurrences plus 22 `.ts` `tr()` contexts (~220 translated strings) that go obsolete if not renamed with the classes. **55** (`project-docs/TODO_MIGRATE.md`) refreshes `.github/README-DEVELOPER.md`; its first subtask is **already done** (see below). **57** (`project-docs/TODO_SEAMLYLAYOUT.md`) was rewritten from "rename `app_core`'s `lib.rs`" to "give every crate root a unique name" — 11 crates all use `lib.rs`; splitting the root does **not** meet the uniqueness rule because Cargo still requires a root file, so the answer is `[lib] path = "src/<crate>.rs"` across all 11 |
| **Task 56**                                          | **Filed, then removed at the user's request** — the `BUILD_PROBLEMS.txt` clangd noise is not being tracked as a task. (The finding stands if it comes back: 45 entries, all cascading from two `pp_file_not_found` roots because the repo has no compile database; qmake+MSVC builds those files clean.)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| **`README-DEVELOPER.md` Qt modules**               | **Fixed in `09da7801e0`, then LOST in the `develop` merge.** The fix added Qt WebChannel + Qt Positioning with a note that ticking Qt WebEngine does not install them. `develop` carried four independent rewrites of the same file (`ad38f96e4e`, `d2aba7efb9`, `fbff2de94c`, `8d83757332`) and the merge resolution took their version of that section. **The current file lists neither — nor Qt WebEngine itself**, so the seamlyLayout prerequisites are now entirely undocumented and the Task 44 setup failure is fully reintroduced. Its Task 55 subtask has been un-checked                                                                                                                                                                                                                                                                                                                                                                                                                              |
| **`develop` merge**                                | Brought 11 upstream commits (code + docs, above), and left a stray**`.github/README-DEVELOPER-NEW.md`** — a 149-line near-duplicate of `README-DEVELOPER.md` (6 insertions / 5 deletions apart) that exists on **neither** `develop` nor any earlier commit. It was introduced by the merge commit itself, so it is almost certainly a conflict-resolution artefact rather than an intended file                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| **Naming rules**                                     | **`CLAUDE.md` corrected.** It required new files to begin with `s`; `.github/README-CODE-STYLES.md` says *"don't start filenames with 's'!"* and gives meaningful prefixes instead. The user chose the **style guide as authoritative**, so `CLAUDE.md` now names it and carries the rules that come up constantly — snake_case, the prefix list, unique repo-wide, UpperCamelCase classes, file-matches-class. Only the "not `v`" half of the old rule survives                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| **Style guide revised by the user** (`df5d90bb14`) | **Two new exceptions that supersede parts of the tasks filed minutes earlier.** (1) *"Match Class names exactly for file names that define a class … in UpperCamelCase"* — a file primarily defining one class is now an **exception to snake_case**, which reopens Task 54's file names (`SettingsCommon.h` vs `settings_common.h`; note the repo has no UpperCamelCase `.cpp`/`.h` today outside vendored xerces-c). (2) *"Crate files in SeamlyLayout require multiple `lib.rs` files distinguishable by their paths"* — **voids Task 57's premise**. Both are recorded in the tasks and mirrored into `CLAUDE.md`; both still need a user decision (see next steps)                                                                                                                                                                                                                                                                                                                                 |

### Commits after PR #19 (all on origin)

| Commit                         | Author | Files                                                                                                           | Change                                                                                                                                          |
| ------------------------------ | ------ | --------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| `da1ac2d1be`                 | Claude | 3 ×`TODO_*.md`, `SESSION_HANDOVER.md`                                                                      | Tasks 54-57 written up from`future-todos.md`, each with its scope measured in the tree                                                        |
| `09da7801e0`                 | Claude | same +`.github/README-DEVELOPER.md`                                                                           | Task 54 extended to the class renames; Qt-module fix applied to the doc;**Task 56 deleted**; Task 57 rewritten around the uniqueness rule |
| `ac65b9ee0b`                 | Claude | `CLAUDE.md`                                                                                                   | Coding Rules point at`.github/README-CODE-STYLES.md`; `s`-prefix rule dropped; class-naming and file-matches-class rules added              |
| `df5d90bb14`                 | user   | `.github/README-CODE-STYLES.md`, `future-todos.md`                                                          | The two naming**exceptions** (class-match file names; multiple `lib.rs` allowed)                                                        |
| `5abe181d7a`, `f6dcaded15` | Claude | `SESSION_HANDOVER.md`, `CLAUDE.md`, `project-docs/TODO_MIGRATE.md`, `project-docs/TODO_SEAMLYLAYOUT.md` | Handover brought current; Tasks 54/57 reconciled with those exceptions                                                                          |
| `b3634b4002`                 | user   | `.github/README-CODE-STYLES.md`                                                                               | Further style-guide edits                                                                                                                       |
| `1d74f7e18a`                 | user   | merge of`develop` (17 files)                                                                                  | Upstream code + doc rewrites;**dropped the Qt-module fix**; added the stray `README-DEVELOPER-NEW.md`                                   |

### Files changed (Task 53, commit `a89801e4f4`)

| File                                                                                  | Change                                                                                                                                                                                                                                                                                                  |
| ------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/libs/vmisc/vcommonsettings.cpp` / `.h`                                       | `getDefaultDataRoot()` → `~/seamlyData`; `mergeStrayCommonSettings()` verifies every key reached the destination and the destination reports `NoError`, then calls the new private `removeStrayCommonSettings()`; new public static `pruneEmptyLegacyDataRoot(legacyRoot, configuredRoot)` |
| `src/app/seamly2d/core/application_2d.cpp`, `src/app/seamlyme/application_me.cpp` | One`pruneEmptyLegacyDataRoot()` call each, after `initializeDataRoot()` — the **only** places real home paths reach it                                                                                                                                                                       |
| `src/test/Seamly2DTest/qttestmainlambda.cpp`                                        | The Task 34`initializeDataRoot()` mirroring was **removed**, with a comment saying not to restore it                                                                                                                                                                                            |
| `src/test/Seamly2DTest/tst_dataroot.cpp` / `.h`                                   | Six new cases: prune removes an empty tree / keeps one holding files / never removes the configured root / keeps a root containing the configured root / ignores a missing root; stray merged-then-deleted                                                                                              |
| `.github/README-BUILDS.md`                                                          | Data-root section rewritten — why`seamlyData` and not `seamly`, the legacy-skeleton cleanup rules, the stray-deletion rules, and the real deletion incident in the testing note                                                                                                                    |
| `project-docs/TODO_MIGRATE.md`, `project-docs/TODO_COMPLETED.md`                  | Task 53 entry added to`project-docs/TODO_COMPLETED.md` (Task 34 marked partly superseded); Task 14 and Task 52 extended; every `~/seamly` → `~/seamlyData` in Tasks 14/35/36/37/38                                                                                                               |

### Two rules established here — do not undo them

1. **Any function that deletes is called with real home paths only from `Application2D::openSettings()` / `ApplicationME::openSettings()`** — never from a shared init function the test harness also calls. `TestApplication2D`'s constructor runs *before any* `initTestCase()`, so anything it reaches executes against the developer's real settings and real home directory no matter how carefully individual tests redirect. A test run must not mutate the machine it runs on.
2. **`pruneEmptyLegacyDataRoot()` is parameterized on purpose** so tests can point it at a `QTemporaryDir`. `QDir::homePath()` cannot be redirected on Windows. Same reason Task 34 split `chooseFirstRunDataRoot(defaultRoot, legacyRoot)` out of `initializeDataRoot()`.
3. **`.github/README-CODE-STYLES.md` is the naming authority — do not reinstate the `s` prefix.** New files are snake_case with a meaningful prefix from that guide's list (`settings_*`, `dialog_<toolgroup>_<toolname>`, `tool_*`, `model_*`, `options_*`, `test_*`, `application_<appname>`, `<platform>_*`) and **unique repo-wide**, with two exceptions the user added in `df5d90bb14`: a file that primarily defines one class takes the class's **UpperCamelCase** name instead of snake_case, and seamlyLayout's multiple `lib.rs` crate roots are allowed. The old "begin new files with `s`" line in `CLAUDE.md` was removed deliberately on 2026-07-26; the "must not begin with `v`" half was kept.

### Why `seamlyData` and not `seamly`

The user first asked for `dataRoot=G:/My Drive/seamly`. That folder already existed and held **73 GB of unrelated business data** (Finances, Team, Security). A bare `seamly` collides far too easily with a folder a user already has; `seamlyData` says what it is. The rename was applied to the default, the tests, the docs and every open task that mentioned it.

### Developer-machine state (not in git — re-derive from here, do not assume the old paths)

| Item                                                        | Now                                                                                                                                                                                                                                                                                  |
| ----------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Data tree                                                   | **`G:\My Drive\seamlyData`** (was `G:\My Drive\seamly2d`). Moved by **folder rename**, not copy — one Google Drive metadata operation, no 17.6 GB re-upload, reversible. Verified identical before/after: **8,713 files / 1,050 dirs / 17,629,852,473 bytes** |
| `%APPDATA%\Seamly\qt6_common.ini`                         | `dataRoot=G:/My Drive/seamlyData`, all seven path overrides repointed                                                                                                                                                                                                              |
| Also repointed                                              | `%APPDATA%\Seamly\common.ini`; `%APPDATA%\Unknown Organization.ini`; `%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini` (10 paths). Zero stale `seamly2d` paths remain                                                                                                        |
| `C:\Users\susan\seamly2d`                                 | **Deleted** after confirming 0 files / 8 empty dirs                                                                                                                                                                                                                            |
| `%APPDATA%\Unknown Organization\` (the **folder**)  | **Gone** — merged into `%APPDATA%\Seamly\qt6_common.ini`, all four values confirmed present first                                                                                                                                                                           |
| `%APPDATA%\Unknown Organization.ini` (the **file**) | **Still live**, still holds `paths/pattern` and `paths/layout`. That is Task 52, untouched                                                                                                                                                                                 |
| Backups                                                     | Every settings file touched was backed up to the session scratchpad`…\scratchpad\settings-backup\` — session-scoped, treat as gone                                                                                                                                               |

### What is verified

- **Local build** — `scripts\sd.ps1` clean.
- **Local tests** — `scripts\st.ps1`: **32119 passed, 0 failed across 25 suites**, exit 0, with `TST_DataRoot … 22 passed, 0 failed`. `ParserTest` exit 0, `TranslationsTest` exit 0.
- **The suite no longer mutates the machine** — `C:\Users\susan\seamly2d`, `C:\Users\susan\seamlyData` and `%APPDATA%\Unknown Organization` were byte-identical before and after a full run.
- **CI** — all 11 checks on PR #19.

### Crossed `labels` / `images` — found and fixed

`%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini` had **`labels` and `images` crossed**: `labels=…/seamlyData/images`, `images=…/seamlyData/label templates`. This was *pre-existing stored data*, not a code bug — `preferencespathpage.cpp` maps rows 0–9 consistently between `Apply()` and `initializeTable()`, and `vcommonsettings.cpp` confirms `paths/labels` is the label-template path (`settingPathsLabelTemplate`) while `paths/images` is `settingImagesPath`. **Un-crossed at the user's instruction** and verified against the filesystem: `label templates` (34 files) and `images` (3 files) both resolve. Backup at `qt6_seamly2d.ini.bak-uncross`. Nothing in the repo changed — this was stored user state only.

### `/compact` hooks fixed, and the handover rule is now in `CLAUDE.md`

The `PreCompact` / `PostCompact` hooks in `.claude/settings.json` were emitting `hookSpecificOutput.additionalContext`, which the hook schema accepts **only** for `UserPromptSubmit`, `PostToolUse`, `PostToolBatch` and `Stop`. Both failed validation on every compaction, so the SESSION_HANDOVER.md instruction never reached the model. Both now emit top-level **`systemMessage`** — the only text-carrying key the compact events accept.

**Know the limitation:** `systemMessage` is *displayed*, not injected as instruction context. The hooks are a visible nudge, not a guarantee, and `PreCompact` can no longer shape the summary at all. That is why the requirement was also added to `CLAUDE.md` under Task Tracking, which *is* loaded every session — that line is the actual mechanism; the hooks are the reminder.

## Concrete next steps (resume here)

1. **Task 49** — make SeamlyLayout consume its positional argument; `qt_frontend/main.cpp` never reads its command line, so Layout Mode opens an empty canvas. Still the highest-value open item: the handoff is the whole point of the daughter app.
2. **Task 51** — the Windows MSI install-time experience (shortcuts, registry, ARP, associations, desktop/taskbar options, UAC, the "your data is safe" upgrade warning) plus the clean-machine install/upgrade/uninstall cycle. Its upgrade-warning wording must now say **`seamlyData`**.
3. **Task 14** — the check-and-move flow for an existing data tree (nine new subtasks). Shared cross-platform code, also needed by Tasks 35/36/37 and satisfying Task 38.
4. **Task 52** — the `vsettings.cpp` "Unknown Organization" stray, **starting with** the `CollectionTest` isolation subtask.
5. **Task 50** — remove the hard-coded developer path in `application_2d.cpp:507-512` before the upstream PR.
6. **Tasks 46 / 45** — port `sb.ps1`'s `.seamly-qmake-kit` marker to `sd.ps1`; clean the stale `C:\Qt\6.10.1` paths from the Claude settings allowlists.

Newly filed, none started, no priority assigned by the user yet:

- **Task 54** — rename the three `vmisc` settings files **and** their classes. Mechanical but wide: ~620 class occurrences over 25 files, 101 files including the headers, and the 22 `.ts` `tr()` contexts must move in the **same commit** or ~220 translated strings go obsolete. Do files and classes together — a split leaves the new file declaring the old class. **Blocked on one decision:** the file-name form, see below.
- **Task 55** — the `.github/README-DEVELOPER.md` refresh. **Re-read the file before starting:** the `develop` merge rewrote it, so the task's line references are stale, its Qt-module subtask is un-checked again, and the target file itself is in question (decision 3 below). Defects that survive the rewrite: the self-contradicting IDE/compiler lines (VS Code vs Visual Studio 2022 vs `CLAUDE.md`'s VS 18 Community), the Qt 5 Windows link, the `` ```bash `` fence over Windows `nmake` commands with no `vcvars64.bat` requirement, the duplicated pdftops paragraph, and **no mention of Rust/cargo, the build scripts, or the WebEngine modules**. The Qt Design Studio "don't select" warning (Task 47's hazard) was also dropped in the rewrite.
- **Task 57** — **premise superseded**, see below; decide whether to delete it (as Task 56 was) or keep only the `error.rs` ×2 collision. The user had already answered "no plan yet" before the exception was written.

## Four decisions the user still owes

1. **Task 54's file-name form** — `SettingsCommon.h` (the class-match exception) or `settings_common.h` (snake_case + the `settings_*` prefix, as originally requested). Six file names and every future class-file rename hang on it; the task's first subtask is exactly this. --> user says to use SettingsCommon.h so that the file name matches the class name.
2. **`.github/README-DEVELOPER-NEW.md`** — delete it, or fold it into `README-DEVELOPER.md` and delete the original. Two near-identical developer READMEs is worse than either alone, and a `-NEW` suffix ages badly. Whichever survives is Task 55's target. --> user says to rename `.github/README-DEVELOPER-NEW.md`to `.github/README-DEVELOPER-SEAMLY-FAMILY.md` which will be folded into `.github/README-DEVELOPER.md`when the migration is completed.
3. **Re-apply the Qt WebChannel / Qt Positioning documentation?** The merge left the developer README with no WebEngine-family modules at all. Until it is restored, a new contributor cannot build seamlyLayout and the error they get names the wrong module. `CLAUDE.md` still carries the rule, so nothing is lost — but the doc a newcomer reads does not. -->user says to maintain `.github/README-DEVELOPER-SEAMLY-FAMILY.md` until migration is complete.

## Uncommitted work in the tree

**None — the tree is clean** and local `run-seamlyLayout` equals `origin/run-seamlyLayout`. The user has been editing docs directly and committing them, so re-check `git status` before assuming a dirty file is yours.

## Gotchas

- **A `develop` merge can silently drop doc edits made on this branch.** `.github/README-DEVELOPER.md` was edited on `run-seamlyLayout` and, in the same window, four times on `develop`; the merge resolution took develop's side and the branch edit vanished with no conflict marker left behind. After merging `develop`, `git log -S "<a phrase you added>" -- <file>` is the cheapest way to confirm your change survived. The same merge also deposited a stray `README-DEVELOPER-NEW.md` that exists in no parent commit.
- **`QSettings(fileName, format, parent)` records neither an organization nor an application name** — both come back empty, and QSettings substitutes the literal `"Unknown Organization"`. Root cause of the stray files in Tasks 34 and 52. `QSettings::setPath(format, scope, dir)` *does* redirect settings files, but has **no getter** — recover the base from a probe instance.
- **`QDir::fromNativeSeparators()` rewrites backslashes only on Windows** (a backslash is a legal POSIX filename character), and Windows path comparison must be `Qt::CaseInsensitive` — both matter when comparing a configured root against a legacy one.
- **`QDir::rmdir()` over `removeRecursively()`, deliberately.** `rmdir()` cannot delete a file and refuses a non-empty directory, so it cannot run away. `removeRecursively()` also bypasses the Recycle Bin — that is how `C:\Users\susan\seamly2d` was permanently destroyed in the previous session.
- **`scripts\st.ps1` runs only `Seamly2DTests.exe`.** CI's `make check` runs four binaries — `Seamly2DTest`, `CollectionTest`, `ParserTest`, `TranslationsTest`. Run the other three by hand before pushing.
- **`gh` is not on this agent shell's `PATH`** — invoke it as `& "C:\Program Files\GitHub CLI\gh.exe"`.
- **The sandbox blocks a command that contains both a `Remove-Item` and a `G:` path**, even when they are unrelated. Split into separate calls.
- **clangd diagnostics in this repo are noise, and here is why** — the tree has **no** `compile_commands.json`, `.clangd`, `.vscode/c_cpp_properties.json` or `compile_flags.txt`, so the editor parses each file with zero include paths. The `#include "../vmisc/vabstractapplication.h"` form is valid only because every `.pro` adds `INCLUDEPATH += $$PWD/../../libs/<lib>`, which clangd never sees; one unresolved include then cascades into dozens of `Unknown type name 'QString'` / `undeclared identifier 'QStringLiteral'` entries. `src/app/seamly2d/core/BUILD_PROBLEMS.txt` is a tracked 45-entry dump of exactly this (two `pp_file_not_found` roots + 43 cascade). **The qmake build is the authority** — those same files compile clean. Filing this as a task was considered and declined; if it is ever fixed, the dump should go with it (it carries absolute `/c:/Users/susan/…` paths into source headed for the upstream PR).

### Carried forward (still true)

- **Qt Design Studio poisons `PATH`.** Bare `qmake`, `windeployqt` and `windeployqt6` resolve to a Qt **6.8.7** kit with no `mkspecs`. Never call these bare; use `qtPrepareTool` or `$$[QT_INSTALL_BINS]/…`. Root cause of Tasks 47 and 48. --> The user removed Qt Design Studio
- **PowerShell 5.1 wraps a native exe's stderr in `NativeCommandError`** and sets `$?` to `$false` even on exit 0. Do not redirect native stderr inside PowerShell — run the script as a child process with `Start-Process … -RedirectStandardOutput/-RedirectStandardError -Wait -PassThru -NoNewWindow`.
- **PowerShell splatting: `@array` is positional, `@hashtable` is by name.**
- **Qt frontend test exes are GUI-subsystem binaries** — they print nothing to captured stdout. Run with `-o <file>,txt` and `QT_QPA_PLATFORM=offscreen`.
- **`$proFile` collides with the automatic `$PROFILE`** (case-insensitive); `sd.ps1` still has it.
- **Historical 6.10 references in `project-docs/TODO_COMPLETED.md` and `project-docs/PROJECT_PLAN.md` are deliberate** — they record what was true at the time.
