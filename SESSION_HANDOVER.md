# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`packaging/windows/README.md` and `README_MSI_WORKFLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## Current steps

1. build .msi with `packaging\windows\test_build_msi_local.ps1`
2. clear environment with `packaging\windows\test_reset_environment.ps1` — needs
   an elevated shell
3. install MSI with `packaging\windows\seamly-msi\x64\seamly-x64.msi` — elevated
   shell, run the wizard, do **not** pass `/quiet`
4. test installation against `project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`
5. add tasks for additional errors to `project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md`
6. implement a task from `project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md`, then loop to step 1; repeat until that file is empty

**Where this loop stands, 2026-09-02.** One full turn ran on build
**26.9.2.1059** (MSI ProductVersion 26.9.2499): built, machine reset, installed
through the wizard from an elevated shell, and walked end to end against the
test case. Steps 1-4 pass with no failing check. Steps 5 and 6 have nothing to
do — `project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md` holds no open task, and
this pass found no new defect. **The loop is finished unless a new defect turns
up.**

`project-docs/TODO_SETTINGS_FILES.md` is deleted; the old steps that named it
are gone. Build only with `test_build_msi_local.ps1` — do **not** use
`src\app\seamlylayout\build.ps1` or `qd.ps1` any more. Both `CLAUDE.md` files and
`src/app/seamlylayout/.claude/rules/testing.mdc` were corrected to say so.

## Commit state

Committed on `run-seamlyLayout`, **not pushed**. Two commits ahead of
`origin/run-seamlyLayout`:

- `71505f97ec` — the task work, made on `task-log-path-and-shortcut-keys`;
- `86881a6173` — the `--no-ff` merge. The branch is deleted.

**No skip-ci token on either.** `packaging/**` and `CMakeLists.txt` changed
functionally, so the next push must run the full `ci.yml` suite.

**The `MSI1b.1` work is not committed.** Changed, on `run-seamlyLayout`:

- `packaging/windows/smsi_fix_dialog_lines.ps1` — new;
- `packaging/windows/smsi_ui.wxs`, `smsi.ps1`, `smsi_check_authoring.ps1`;
- `project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md`, `SESSION_HANDOVER.md`.

`scripts/prompt_testing.txt` was already modified before this session, and the
build rewrites `projectversion.cpp/.h` and `Info.plist` every time. `packaging/**`
changed functionally again, so the skip-ci token still must not be used.

## Machine state

- Installed: Seamly **26.9.2.1059** in `C:\Program Files\SeamlyApps`
  (seamly2d.exe, seamlyme.exe, SeamlyLayout.exe). MSI ProductVersion `26.9.2499`.
- Installed 2026-09-02 through the **wizard** from an elevated shell, onto a
  machine reset by `test_reset_environment.ps1`. Install log:
  `%TEMP%\seamly_install.log`.
- `%DATAROOT%` = `C:\Users\susan\Documents\SeamlyData`. Right after the install
  it is an **empty** directory: the MSI creates it, and the first app run seeds
  it. Same for SeamlyLayout's `default_preferences.json` /
  `default_settings.json`.
- All three apps have been run once, so every first-run artifact exists.
  `%DATAROOT%` now holds 8 subdirectories, 8 patterns, 3 individual and 1
  multisize measurement file.
- `packaging/windows/test_msi_install.ps1` cannot check this pass. It needs
  `-Phase Baseline` captured BEFORE the install, and the install is already
  done. Capture the baseline first next time.

**Elevation.** The VS Code integrated terminal is not elevated, but
`Start-Process -Verb RunAs` DOES work when someone is present to accept the UAC
prompt. It fails at once with "The operation was canceled by the user" when the
prompt goes unanswered — that message means unanswered, not refused by policy.
An unelevated `msiexec /i` fails at `InstallFinalize` with `Error 1925` and exit
1603, after every earlier action returned 1, so the log looks healthy until the
last page.

## Done this session

### Task MSI1b.1 — Error 2826 dialog overflow: fixed, verified, closed

**The cause, measured from the MSI `Dialog` and `Control` tables of build
26.9.2.996.** 37 `Line` controls carried `Width="373"` inside a 370-unit dialog.
WixUI authors its own `BannerLine`/`BottomLine` rows exactly the way our five
custom dialogs did, so one cause covers every dialog. The overflow is 3
**installer units**, not pixels: Windows Installer converts to display pixels
before it writes the message, and this 144 DPI screen prints 7. The log carries
15 lines and the table 37 rows because only the dialogs a wizard install builds
are ever created.

The fix has two halves, because the WixUI rows cannot be edited from the `.wxs`
and WiX cannot move past 6 (v7 is behind the OSMF EULA):

- `smsi_ui.wxs` — our ten `Line` controls are now `Width="370"`.
- New `packaging/windows/smsi_fix_dialog_lines.ps1` — trims any `Line` control
  that ends past its dialog edge in the built MSI. It **fails the build** when a
  non-`Line` control overflows, because shortening one of those would clip text
  or a button. `smsi.ps1` runs it after `wix build` and before
  `wix msi validate`, so the ICE pass and the authoring check both see the
  package that ships.
- `smsi_check_authoring.ps1` — new section 10 asserts every control fits its
  dialog, bottom edge as well as right (`MSI1b.1.3`).

On build 26.9.2.1059 the trim reported **27** controls, not 37: the ten that are
ours are now correct at source, which is the proof that both halves work. 0
overflowing controls remain, `wix msi validate` is clean apart from the expected
ICE61, and all authoring checks pass.

**Verified at run time and closed.** A fresh wizard install of 26.9.2.1059 from
an elevated shell finished with `Installation success or error status: 0`, and
its `/l*v` log carries no `Error 2826` line and no `extends beyond` line across
the 9 dialogs the install created. The task moved to
`project-docs/TODO_COMPLETED.md`, which leaves
`project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md` with no open task.

**Test Case 1b-i step 1b was rewritten at the same time.** The file on disk still
said `msiexec /i seamly-x64.msi /quiet /norestart`, even though an earlier
session recorded the wizard rewrite as done. A silent install builds no dialogs,
so that step could never have found this defect. It now runs the wizard with
`/l*v` and carries 1b-i, 1b-ii and 1b-iii. Treat the older handover claim as
wrong, not the file.

### Test Case 1b-i walked end to end on 26.9.2.1059 — PASS, no failures

Second consecutive pass with no failing check.

| Step | Result |
| --- | --- |
| A.1a reset | pass, exit 0; uninstalled 26.9.2436, removed `%DATAROOT%` and `%LOCALAPPDATA%\Seamly` |
| A.1b wizard install | pass, exit 0, **no `Error 2826`** — `MSI1b.1` closed |
| B.0a directories and files | pass |
| B.0b ini contents | pass, all four files |
| B.0c program directory contents | pass, all six named files |
| B.1b / 1c / 1d registry | pass; each app's key carries its own `DesktopShortcut*` flag |
| B.2 apps a-d, human at the keyboard | pass |
| B.3a / 3b SeamlyLayout log directory | pass |
| B.4 desktop shortcuts | pass, all three, correct targets |
| B.5 log errors | pass, none |

Evidence from the app runs:

- `qt6_common.ini` records `firstRunDataNotice=shown` (B.2a-ii).
- Session logs: `Seamly2D\logs\seamly2d-pid29216.log`,
  `SeamlyMe\logs\seamlyme-pid8280.log`,
  `SeamlyLayout\logs\log_260902181015.txt`. No error, critical or fatal line in
  any of them (B.5).
- `%LOCALAPPDATA%\Seamly\SeamlyLayout\cache` exists, `%LOCALAPPDATA%\SeamlyLayout`
  does not (B.2c-iii, B.3a).
- **B.2c-ii has independent proof again.** SeamlyLayout's log records
  `main(): opening startup document 'male_shirt' of 50366 characters`, and no
  `.pieces.svg` exists under `%DATAROOT%` or `%TEMP%`.
- `%DATAROOT%` is seeded after the app runs: 8 subdirectories, 8 patterns, 3
  individual and 1 multisize measurement file, plus two backups the run made.

`%DATAROOT%` is empty right after the install, and SeamlyLayout's two JSON
defaults are absent until the first app run. That is correct. B.0a still lists
them as post-install checks, so it reads as a failure on every pass — see
"Open — next steps".

### Tasks Layout.10, Layout.7 and SeamlyMe.3 implemented, verified and closed

All three moved to `project-docs/TODO_COMPLETED.md`, with the full write-up
there. Verified on a fresh wizard install of 26.9.2.996, 2026-09-02.

**Layout.10 — SeamlyLayout logs move to `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs`.**
Two separate causes, both fixed:

- `main()` called `Logger::init()` BEFORE it set the organization and
  application names. `AppConfigLocation` is built from those two names, so the
  root resolved to `%LOCALAPPDATA%\SeamlyLayout\`. The metadata block now sits
  ahead of `Logger::init()` (`main.cpp`), with a comment saying why the order
  matters.
- `Logger::init()` appended `/output`. It now appends `/logs`, on every
  platform branch. `clearOutputDirectory()` is renamed `clearLogDirectory()`.

`test_reset_environment.ps1` gained a removal for the stray
`%LOCALAPPDATA%\SeamlyLayout` tree that older builds left behind; section 4
does not reach it, because it sits outside the `Seamly` folder.

New Qt suite `src/test/SeamlyLayoutTest/LoggerTests.cpp` plus its CMake target
locks the path. It runs under `QStandardPaths::setTestModeEnabled(true)`, so it
never touches the real user configuration. 7 checks, all passing.

**Layout.7 + SeamlyMe.3 — each desktop-shortcut flag goes under its own key.**
`smsi_shortcuts.wxs` wrote all three `DesktopShortcut*` values into
`HKLM\SOFTWARE\Seamly\Seamly2D`. `DesktopShortcutSeamlyMe` now goes to
`...\SeamlyMe` and `DesktopShortcutSeamlyLayout` to `...\SeamlyLayout`. Both
keys already existed, authored by `smsi_registry.wxs`.

- `smsi_check_authoring.ps1` gained three assertions, one per app. It had none
  before, which is why the defect survived every earlier pass.
- `test_msi_install.ps1` read all three breadcrumbs out of the Seamly2D key and
  never checked SeamlyLayout at all. It now reads each from its own key and
  covers all three.

**Test Case 1b-i updated to match.** B.1c no longer says Seamly2D carries the
three flags; new B.1d checks the per-app keys. B.3 changed from "file a task if
the stray directory exists" to a two-part check: `%LOCALAPPDATA%\SeamlyLayout`
absent, session log under `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs`.

Verification on 26.9.2.996: each key carries exactly its own flag, and
SeamlyLayout's session log is
`%LOCALAPPDATA%\Seamly\SeamlyLayout\logs\log_260902170116.txt` with no
`%LOCALAPPDATA%\SeamlyLayout` anywhere.

### Test Case 1b-i walked end to end on 26.9.2.996 — PASS, no failures

First pass with **no failing check**. Section A and section B both complete.

| Group | Result |
| --- | --- |
| A.1a / 1a-i reset | pass; the run deleted a real `%LOCALAPPDATA%\SeamlyLayout` leftover |
| A.1b wizard install | pass, exit 0 |
| B.0 directories, files, ini contents | pass |
| B.1 registry, including new B.1d per-app flags | pass |
| B.2 apps a-d, human at the keyboard | pass |
| B.3 SeamlyLayout log directory | pass — `Layout.10` closed |
| B.4 desktop shortcuts | pass, all three, correct targets |
| B.5 log errors | pass, none |

**B.2a-i did not show the "Seamly data moved" notice.** There was no legacy data
to migrate, so there was nothing to announce. `qt6_common.ini` still records
`firstRunDataNotice=shown`, so B.2a-ii passes.

**B.2c-ii has independent proof.** SeamlyLayout's own session log records
`main(): opening startup document 'male_shirt' of 50370 characters`. The SVG
arrived as a string. No `.pieces.svg` exists under `%DATAROOT%` or `%TEMP%`.

**MSI1b.1 reproduced from the wizard** — exactly 15 `Error 2826` lines, all 7 px.
No other error in the install log.

**Two B.0a items are seeded on first app run, not by the MSI** — the
`%DATAROOT%` tree (task `Seamly2D.2`) and SeamlyLayout's
`default_preferences.json` / `default_settings.json` (task `SettingsFiles.6`).
They are absent right after install and present after B.2. That is correct
behaviour. B.0a still lists them as post-install checks, so it reads as a
failure on every pass. See "Open — next steps".

### Task SeamlyMe.5 closed

`SeamlyMe.5.2` verified: a SeamlyMe run created
`%LOCALAPPDATA%\Seamly\SeamlyMe\logs\seamlyme-pid30712.log`. The task moved from
`TODO_SEAMLYME.md` to `TODO_COMPLETED.md`.

### Test Case 1b-i steps 0 and 1b rewritten

- Step 0 now carries an elevation check and names the `Error 1925` failure.
- Step 1b now runs the **wizard**, not `/quiet`, and gains 1b-i, 1b-ii and
  1b-iii. A silent install builds no dialogs, so the `/quiet` command the step
  used to specify could never verify `MSI1b.1`. `scripts/prompt_testing.txt`
  step 5 says the same.

`test_reset_environment.ps1` keeps `/quiet` on its `msiexec /x`. Uninstall UI is
not under test.

### Task Seamly2D.5 / Layout.9 — stringified-SVG handoff (closed)

Piece mode now reaches SeamlyLayout as a stringified SVG on the child process's
standard input. No `.pieces.svg` file is written. Full write-up, including the
`Layout.9.1` design decision and the file-by-file change list, is in
`project-docs/TODO_COMPLETED.md`.

### `test_build_msi_local.ps1` now builds and runs the Qt unit tests

It previously passed `CONFIG+=noTests`, so `src\test` never compiled locally and
`TST_SeamlySuitePaths` had no local runner at all. Now:

- `qmake Seamly.pro -r -config release` — no `noTests`, so `src.pro` adds the
  `test` subdirectory. `-r` is required: a generated subdirs Makefile recreates
  its children only `if not exist Makefile`, so a tree left from an earlier
  `noTests` run would silently keep skipping the tests. CI never hits this, as
  every job starts from a fresh checkout.
- New `nmake check` step before packaging, so a failing test stops the build
  instead of reaching an MSI. Runs with `QT_QPA_PLATFORM=offscreen`, matching
  `ci.yml`'s `windows-test` job.
- The step clears `NoDefaultCurrentDirectoryInExePath`. It is set to `1` in this
  machine's shell; qmake's `check` target runs a **bare** `ParserTest.exe` from
  inside `bin`, and cmd then refuses to resolve it — every suite failed with
  `'ParserTest.exe' is not recognized` while the binary sat right there.
- New `-SkipTests` switch restores the old faster behaviour.
- `.gitignore` gained `target_wrapper.bat`. It already ignored
  `target_wrapper.sh`; the `.bat` only appears once the Qt suites are built on
  Windows, which had never happened locally before.

### Build instructions corrected in the rule files

`CLAUDE.md`, `src/app/seamlylayout/CLAUDE.md` and
`src/app/seamlylayout/.claude/rules/testing.mdc` all told a future session to
build with `build.ps1` / `qd.ps1`. They now name
`packaging\windows\test_build_msi_local.ps1` and retire the other two. The
project `CLAUDE.md` also documents the four build stages, the `-SkipTests` and
`-SkipValidation` switches, that arm64 is not covered, and how to read a single
Qt suite's output.

Both `build.ps1` and `qd.ps1` are still on disk — marked unsupported, not
deleted. Deleting them was not asked for.

## Verification status

| Suite | How | Result |
| --- | --- | --- |
| Seamly2DTest, CollectionTest, ParserTest, TranslationsTest | `nmake check` inside `test_build_msi_local.ps1` | pass |
| SeamlyLayout Qt tests | `ctest --preset debug` | 6/6, including the new `LoggerTests` |
| `LoggerTests` alone | per-suite log via `-o <file>,txt` | 7 passed, 0 failed |
| SeamlyLayout Rust | `cargo test --workspace` | pass |
| MSI 26.9.2.996 | `test_build_msi_local.ps1` | MSI OK, 164.6 MB; authoring check and 17 installer self-tests pass |
| Test Case 1b-i, fresh wizard install of 26.9.2.996 | manual walkthrough, 2026-09-02 | **pass, no failures** |

To read a single Qt suite's output, set `SEAMLY_TEST_LOG_DIR` and run the
binary through its own `target_wrapper.bat` — three of the four qmake suites are
GUI-subsystem binaries that print nothing to a console, and a shared `-o` target
is overwritten by each `qExec()` call. The CMake SeamlyLayout suites are
GUI-subsystem too; run one with `-o <file>,txt` to read its result.

**`cargo test` needs MSVC's `link.exe` first on PATH.** `C:\Program Files\Git\usr\bin`
holds a GNU `link` that shadows it, and the failure names the Visual Studio
installer, not the shadowing. `vcvars64.bat` alone does not fix it, and a
`vcvars && set PATH=...%PATH%...` one-liner cannot: cmd expands the whole line
before `vcvars` runs. Put the commands in a `.cmd` file and prepend
`%VCToolsInstallDir%bin\Hostx64\x64`.

## Open — next steps

1. **Nothing is open in the MSI test loop.** Test Case 1b-i passed end to end on
   26.9.2.1059 and its TODO file is empty. The next MSI work is either a new
   defect from a later pass, or test cases 2, 3 and 4 of section A, which have
   never been walked.
2. **Test-document defects in `TEST_MSI_WIN_X64_Test_Case_1b-i.md`, agreed but
   not yet fixed.** Each makes correct behaviour read as a failure. This list
   was re-checked against the file on 2026-09-02, and it is shorter than the
   older handover said — B.2b-v and B.2c-iii already name the right paths, and
   B.0a does list `label templates`. What is left:
   - B.0a is ordered wrong. Split it into a post-install part and a
     post-first-run part. Confirmed again on this pass: right after the install
     `%DATAROOT%` is an empty directory and SeamlyLayout's
     `default_preferences.json` / `default_settings.json` are absent; all three
     appear after B.2.
   - Doubled leaf in a placeholder: `%DATAROOT%\SeamlyData` (line 56) and
     `%PROGRAMDIR%\SeamlyApps` (lines 40, 85). Both placeholders already end in
     that leaf.
   - `%DATAROOTROOT%` (lines 89, 90) should be `%DATAROOT%`.
   - B.0b-iii says `qt6_seamly2d.ini` should be empty; it means
     `qt6_seamlyme.ini`, which is the file that is empty.
3. **Commit the working tree** — nothing is committed yet.

## Still to do — the SeamlyLayout return path

Not implemented, and **no task file entry exists for it yet**. It was outside
`Seamly2D.5`/`Layout.9`, whose subtasks covered the outbound handoff only.

- Closing SeamlyLayout with 'Save': convert its layout to a stringified SVG,
  pass it back to Seamly2D, show it in Seamly2D's right canvas, and return focus
  there.
- Closing SeamlyLayout any other way: refresh the right canvas with the previous
  Seamly2D data and return focus there.

Focus already returns to the mode active before the handoff (`Seamly2D.3`). What
is missing is carrying SeamlyLayout's layout back into the right canvas.
