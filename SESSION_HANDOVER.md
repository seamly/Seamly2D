# Session handover

## Current state: Tasks 34 and 53 are DONE and moved to `TODO_COMPLETED.md`

**Date:** 2026-07-26. **Branch:** `run-seamlyLayout` at **`1d74f7e18a`** ("merge develop"), **pushed — local and `origin/run-seamlyLayout` are identical, working tree clean.** Task 53 landed via **PR [#19](https://github.com/seamly/Seamly2D/pull/19)** (`task-53-seamlydata-root` → `run-seamlyLayout`) — **all 11 CI checks green** (Windows x64 27m4s, Windows arm64 cross-compile 26m28s, macOS 16m51s, Linux AppImage 9m3s, Linux unit tests 9m55s, CodeQL, CodeSee, Analyze actions/python/rust, version) — **merged**, task branch deleted locally and on origin. Task 34 landed earlier the same day.

**`develop` was merged into `run-seamlyLayout`** (`1d74f7e18a`, the sanctioned direction — never the reverse). Local `develop` = `origin/develop` = `057e95bfca`. It brought in real upstream **code**, so "nothing has changed since PR #19" is no longer true: formula-reference guards (`vabstractpattern`, `vdrawtool`, `vtoolline`, `savetooloptions` — issues #1364/#1521), the pointname combobox colour fix (#1636), Finnish translations (#1637), and a CI consolidation that **deleted `.github/workflows/feat-ci.yml`** and folded feature-branch builds into `ci.yml`. **These changes have not been built or tested locally in this session.**

Refer to `TODO_MIGRATE.md`, `TODO_SEAMLY2D.md` and `TODO_SEAMLYLAYOUT.md` for the tasks still open.

**Theme of this session:** the user-data root, then naming. Task 34 renamed and generalized the data root; Task 53 renamed it again (to `seamlyData`), made the settings migration self-cleaning, and fixed up the developer's own machine. The tail of the session moved four items out of `future-todos.md` into the task lists and settled which document owns the naming rules.

### What was done this session

| Step | Outcome |
| --- | --- |
| **Task 34** | **Done.** `~/seamly2d` → `~/seamly`, made relocatable via `paths/dataRoot`; `chooseFirstRunDataRoot()`, `ensureDataRootTree()`, `rebaseOntoDataRoot()`, `mergeStrayCommonSettings()`; `TST_DataRoot` added (16 cases) |
| **Task 53** | **Done.** Default root renamed `~/seamly` → **`~/seamlyData`**; `mergeStrayCommonSettings()` now verifies then **deletes** the stray; new `pruneEmptyLegacyDataRoot()` removes the empty `~/seamly2d` skeleton. `TST_DataRoot` 16 → **22 cases** |
| **Task 14** | **Extended, not implemented.** The user asked whether the installer could check-then-move an existing data tree before anything is removed; it cannot today, so nine subtasks were folded into Task 14 under an "Update (2026-07-26) — moving an existing data tree" note |
| **Task 52** | **Extended.** Gained a new *first* subtask: stop `CollectionTest` writing into the real user settings **before** repointing the `vsettings.cpp` accessors |
| **Developer machine** | **Migrated and verified** — see the table below. This is state *outside* the repo; nothing in git records it |
| **Tasks 54, 55, 57** | **Filed, not started**, moved out of `future-todos.md` (docs-only commits). **54** (`TODO_MIGRATE.md`) renames the three `vmisc` settings **files *and* classes** — `settings_common`/`SettingsCommon`, `settings_seamlyme`/`SettingsSeamlyMe`, `settings_seamly2d`/`SettingsSeamly2D` — per `.github/README-CODE-STYLES.md`; ~620 class occurrences plus 22 `.ts` `tr()` contexts (~220 translated strings) that go obsolete if not renamed with the classes. **55** (`TODO_MIGRATE.md`) refreshes `.github/README-DEVELOPER.md`; its first subtask is **already done** (see below). **57** (`TODO_SEAMLYLAYOUT.md`) was rewritten from "rename `app_core`'s `lib.rs`" to "give every crate root a unique name" — 11 crates all use `lib.rs`; splitting the root does **not** meet the uniqueness rule because Cargo still requires a root file, so the answer is `[lib] path = "src/<crate>.rs"` across all 11 |
| **Task 56** | **Filed, then removed at the user's request** — the `BUILD_PROBLEMS.txt` clangd noise is not being tracked as a task. (The finding stands if it comes back: 45 entries, all cascading from two `pp_file_not_found` roots because the repo has no compile database; qmake+MSVC builds those files clean.) |
| **`README-DEVELOPER.md` Qt modules** | **Fixed in `09da7801e0`, then LOST in the `develop` merge.** The fix added Qt WebChannel + Qt Positioning with a note that ticking Qt WebEngine does not install them. `develop` carried four independent rewrites of the same file (`ad38f96e4e`, `d2aba7efb9`, `fbff2de94c`, `8d83757332`) and the merge resolution took their version of that section. **The current file lists neither — nor Qt WebEngine itself**, so the seamlyLayout prerequisites are now entirely undocumented and the Task 44 setup failure is fully reintroduced. Its Task 55 subtask has been un-checked |
| **`develop` merge** | Brought 11 upstream commits (code + docs, above), and left a stray **`.github/README-DEVELOPER-NEW.md`** — a 149-line near-duplicate of `README-DEVELOPER.md` (6 insertions / 5 deletions apart) that exists on **neither** `develop` nor any earlier commit. It was introduced by the merge commit itself, so it is almost certainly a conflict-resolution artefact rather than an intended file |
| **Naming rules** | **`CLAUDE.md` corrected.** It required new files to begin with `s`; `.github/README-CODE-STYLES.md` says *"don't start filenames with 's'!"* and gives meaningful prefixes instead. The user chose the **style guide as authoritative**, so `CLAUDE.md` now names it and carries the rules that come up constantly — snake_case, the prefix list, unique repo-wide, UpperCamelCase classes, file-matches-class. Only the "not `v`" half of the old rule survives |
| **Style guide revised by the user** (`df5d90bb14`) | **Two new exceptions that supersede parts of the tasks filed minutes earlier.** (1) *"Match Class names exactly for file names that define a class … in UpperCamelCase"* — a file primarily defining one class is now an **exception to snake_case**, which reopens Task 54's file names (`SettingsCommon.h` vs `settings_common.h`; note the repo has no UpperCamelCase `.cpp`/`.h` today outside vendored xerces-c). (2) *"Crate files in SeamlyLayout require multiple `lib.rs` files distinguishable by their paths"* — **voids Task 57's premise**. Both are recorded in the tasks and mirrored into `CLAUDE.md`; both still need a user decision (see next steps) |

### Commits after PR #19 (all on origin)

| Commit | Author | Files | Change |
| --- | --- | --- | --- |
| `da1ac2d1be` | Claude | 3 × `TODO_*.md`, `SESSION_HANDOVER.md` | Tasks 54-57 written up from `future-todos.md`, each with its scope measured in the tree |
| `09da7801e0` | Claude | same + `.github/README-DEVELOPER.md` | Task 54 extended to the class renames; Qt-module fix applied to the doc; **Task 56 deleted**; Task 57 rewritten around the uniqueness rule |
| `ac65b9ee0b` | Claude | `CLAUDE.md` | Coding Rules point at `.github/README-CODE-STYLES.md`; `s`-prefix rule dropped; class-naming and file-matches-class rules added |
| `df5d90bb14` | user | `.github/README-CODE-STYLES.md`, `future-todos.md` | The two naming **exceptions** (class-match file names; multiple `lib.rs` allowed) |
| `5abe181d7a`, `f6dcaded15` | Claude | `SESSION_HANDOVER.md`, `CLAUDE.md`, `TODO_MIGRATE.md`, `TODO_SEAMLYLAYOUT.md` | Handover brought current; Tasks 54/57 reconciled with those exceptions |
| `b3634b4002` | user | `.github/README-CODE-STYLES.md` | Further style-guide edits |
| `1d74f7e18a` | user | merge of `develop` (17 files) | Upstream code + doc rewrites; **dropped the Qt-module fix**; added the stray `README-DEVELOPER-NEW.md` |

### Files changed (Task 53, commit `a89801e4f4`)

| File | Change |
| --- | --- |
| `src/libs/vmisc/vcommonsettings.cpp` / `.h` | `getDefaultDataRoot()` → `~/seamlyData`; `mergeStrayCommonSettings()` verifies every key reached the destination and the destination reports `NoError`, then calls the new private `removeStrayCommonSettings()`; new public static `pruneEmptyLegacyDataRoot(legacyRoot, configuredRoot)` |
| `src/app/seamly2d/core/application_2d.cpp`, `src/app/seamlyme/application_me.cpp` | One `pruneEmptyLegacyDataRoot()` call each, after `initializeDataRoot()` — the **only** places real home paths reach it |
| `src/test/Seamly2DTest/qttestmainlambda.cpp` | The Task 34 `initializeDataRoot()` mirroring was **removed**, with a comment saying not to restore it |
| `src/test/Seamly2DTest/tst_dataroot.cpp` / `.h` | Six new cases: prune removes an empty tree / keeps one holding files / never removes the configured root / keeps a root containing the configured root / ignores a missing root; stray merged-then-deleted |
| `.github/README-BUILDS.md` | Data-root section rewritten — why `seamlyData` and not `seamly`, the legacy-skeleton cleanup rules, the stray-deletion rules, and the real deletion incident in the testing note |
| `TODO_MIGRATE.md`, `TODO_COMPLETED.md` | Task 53 entry added to `TODO_COMPLETED.md` (Task 34 marked partly superseded); Task 14 and Task 52 extended; every `~/seamly` → `~/seamlyData` in Tasks 14/35/36/37/38 |

### Two rules established here — do not undo them

1. **Any function that deletes is called with real home paths only from `Application2D::openSettings()` / `ApplicationME::openSettings()`** — never from a shared init function the test harness also calls. `TestApplication2D`'s constructor runs *before any* `initTestCase()`, so anything it reaches executes against the developer's real settings and real home directory no matter how carefully individual tests redirect. A test run must not mutate the machine it runs on.
2. **`pruneEmptyLegacyDataRoot()` is parameterized on purpose** so tests can point it at a `QTemporaryDir`. `QDir::homePath()` cannot be redirected on Windows. Same reason Task 34 split `chooseFirstRunDataRoot(defaultRoot, legacyRoot)` out of `initializeDataRoot()`.
3. **`.github/README-CODE-STYLES.md` is the naming authority — do not reinstate the `s` prefix.** New files are snake_case with a meaningful prefix from that guide's list (`settings_*`, `dialog_<toolgroup>_<toolname>`, `tool_*`, `model_*`, `options_*`, `test_*`, `application_<appname>`, `<platform>_*`) and **unique repo-wide**, with two exceptions the user added in `df5d90bb14`: a file that primarily defines one class takes the class's **UpperCamelCase** name instead of snake_case, and seamlyLayout's multiple `lib.rs` crate roots are allowed. The old "begin new files with `s`" line in `CLAUDE.md` was removed deliberately on 2026-07-26; the "must not begin with `v`" half was kept.

### Why `seamlyData` and not `seamly`

The user first asked for `dataRoot=G:/My Drive/seamly`. That folder already existed and held **73 GB of unrelated business data** (Finances, Team, Security). A bare `seamly` collides far too easily with a folder a user already has; `seamlyData` says what it is. The rename was applied to the default, the tests, the docs and every open task that mentioned it.

### Developer-machine state (not in git — re-derive from here, do not assume the old paths)

| Item | Now |
| --- | --- |
| Data tree | **`G:\My Drive\seamlyData`** (was `G:\My Drive\seamly2d`). Moved by **folder rename**, not copy — one Google Drive metadata operation, no 17.6 GB re-upload, reversible. Verified identical before/after: **8,713 files / 1,050 dirs / 17,629,852,473 bytes** |
| `%APPDATA%\Seamly\qt6_common.ini` | `dataRoot=G:/My Drive/seamlyData`, all seven path overrides repointed |
| Also repointed | `%APPDATA%\Seamly\common.ini`; `%APPDATA%\Unknown Organization.ini`; `%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini` (10 paths). Zero stale `seamly2d` paths remain |
| `C:\Users\susan\seamly2d` | **Deleted** after confirming 0 files / 8 empty dirs |
| `%APPDATA%\Unknown Organization\` (the **folder**) | **Gone** — merged into `%APPDATA%\Seamly\qt6_common.ini`, all four values confirmed present first |
| `%APPDATA%\Unknown Organization.ini` (the **file**) | **Still live**, still holds `paths/pattern` and `paths/layout`. That is Task 52, untouched |
| Backups | Every settings file touched was backed up to the session scratchpad `…\scratchpad\settings-backup\` — session-scoped, treat as gone |

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
- **Task 55** — the `.github/README-DEVELOPER.md` refresh. **Re-read the file before starting:** the `develop` merge rewrote it, so the task's line references are stale, its Qt-module subtask is un-checked again, and the target file itself is in question (decision 3 below). Defects that survive the rewrite: the self-contradicting IDE/compiler lines (VS Code vs Visual Studio 2022 vs `CLAUDE.md`'s VS 18 Community), the Qt 5 Windows link, the ```` ```bash ```` fence over Windows `nmake` commands with no `vcvars64.bat` requirement, the duplicated pdftops paragraph, and **no mention of Rust/cargo, the build scripts, or the WebEngine modules**. The Qt Design Studio "don't select" warning (Task 47's hazard) was also dropped in the rewrite.
- **Task 57** — **premise superseded**, see below; decide whether to delete it (as Task 56 was) or keep only the `error.rs` ×2 collision. The user had already answered "no plan yet" before the exception was written.

## Four decisions the user still owes

1. **Task 54's file-name form** — `SettingsCommon.h` (the class-match exception) or `settings_common.h` (snake_case + the `settings_*` prefix, as originally requested). Six file names and every future class-file rename hang on it; the task's first subtask is exactly this.
2. **Task 57's fate** — delete it like Task 56, or keep only the `error.rs` ×2 collision, now that multiple `lib.rs` files are explicitly allowed.
3. **`.github/README-DEVELOPER-NEW.md`** — delete it, or fold it into `README-DEVELOPER.md` and delete the original. Two near-identical developer READMEs is worse than either alone, and a `-NEW` suffix ages badly. Whichever survives is Task 55's target.
4. **Re-apply the Qt WebChannel / Qt Positioning documentation?** The merge left the developer README with no WebEngine-family modules at all. Until it is restored, a new contributor cannot build seamlyLayout and the error they get names the wrong module. `CLAUDE.md` still carries the rule, so nothing is lost — but the doc a newcomer reads does not.

## Uncommitted work in the tree

**None — the tree is clean** and local `run-seamlyLayout` equals `origin/run-seamlyLayout`. The user has been editing docs directly and committing them (`df5d90bb14`, `b3634b4002`, and the merge), so re-check `git status` before assuming a dirty file is yours.

## Gotchas

### New this session

- **A `develop` merge can silently drop doc edits made on this branch.** `.github/README-DEVELOPER.md` was edited on `run-seamlyLayout` and, in the same window, four times on `develop`; the merge resolution took develop's side and the branch edit vanished with no conflict marker left behind. After merging `develop`, `git log -S "<a phrase you added>" -- <file>` is the cheapest way to confirm your change survived. The same merge also deposited a stray `README-DEVELOPER-NEW.md` that exists in no parent commit.
- **`QSettings(fileName, format, parent)` records neither an organization nor an application name** — both come back empty, and QSettings substitutes the literal `"Unknown Organization"`. Root cause of the stray files in Tasks 34 and 52. `QSettings::setPath(format, scope, dir)` *does* redirect settings files, but has **no getter** — recover the base from a probe instance.
- **`QDir::fromNativeSeparators()` rewrites backslashes only on Windows** (a backslash is a legal POSIX filename character), and Windows path comparison must be `Qt::CaseInsensitive` — both matter when comparing a configured root against a legacy one.
- **`QDir::rmdir()` over `removeRecursively()`, deliberately.** `rmdir()` cannot delete a file and refuses a non-empty directory, so it cannot run away. `removeRecursively()` also bypasses the Recycle Bin — that is how `C:\Users\susan\seamly2d` was permanently destroyed in the previous session.
- **`scripts\st.ps1` runs only `Seamly2DTests.exe`.** CI's `make check` runs four binaries — `Seamly2DTest`, `CollectionTest`, `ParserTest`, `TranslationsTest`. Run the other three by hand before pushing.
- **`gh` is not on this agent shell's `PATH`** — invoke it as `& "C:\Program Files\GitHub CLI\gh.exe"`.
- **The sandbox blocks a command that contains both a `Remove-Item` and a `G:` path**, even when they are unrelated. Split into separate calls.
- **clangd diagnostics in this repo are noise, and here is why** — the tree has **no** `compile_commands.json`, `.clangd`, `.vscode/c_cpp_properties.json` or `compile_flags.txt`, so the editor parses each file with zero include paths. The `#include "../vmisc/vabstractapplication.h"` form is valid only because every `.pro` adds `INCLUDEPATH += $$PWD/../../libs/<lib>`, which clangd never sees; one unresolved include then cascades into dozens of `Unknown type name 'QString'` / `undeclared identifier 'QStringLiteral'` entries. `src/app/seamly2d/core/BUILD_PROBLEMS.txt` is a tracked 45-entry dump of exactly this (two `pp_file_not_found` roots + 43 cascade). **The qmake build is the authority** — those same files compile clean. Filing this as a task was considered and declined; if it is ever fixed, the dump should go with it (it carries absolute `/c:/Users/susan/…` paths into source headed for the upstream PR).

### Carried forward (still true)

- **Qt Design Studio poisons `PATH`.** Bare `qmake`, `windeployqt` and `windeployqt6` resolve to a Qt **6.8.7** kit with no `mkspecs`. Never call these bare; use `qtPrepareTool` or `$$[QT_INSTALL_BINS]/…`. Root cause of Tasks 47 and 48.
- **PowerShell 5.1 wraps a native exe's stderr in `NativeCommandError`** and sets `$?` to `$false` even on exit 0. Do not redirect native stderr inside PowerShell — run the script as a child process with `Start-Process … -RedirectStandardOutput/-RedirectStandardError -Wait -PassThru -NoNewWindow`.
- **PowerShell splatting: `@array` is positional, `@hashtable` is by name.**
- **Qt frontend test exes are GUI-subsystem binaries** — they print nothing to captured stdout. Run with `-o <file>,txt` and `QT_QPA_PLATFORM=offscreen`.
- **`$proFile` collides with the automatic `$PROFILE`** (case-insensitive); `sd.ps1` still has it.
- **Historical 6.10 references in `TODO_COMPLETED.md` and `PROJECT_PLAN.md` are deliberate** — they record what was true at the time.
