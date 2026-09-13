# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`packaging/windows/README.md` and `README_MSI_WORKFLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## 2026-09-13 — CI: parallel Windows compile with jom

Job `Build SeamlyLayout Release` (run `34784858753`) failed at
`smsi.ps1 -SeamlyLayoutBin` — a stale parameter name from before commit
`794d13092e` (already fixed on `run-seamlyLayout`, so no action needed there).

Analyzing that job's log surfaced a real slowdown: `qmake`/`nmake` compiles
Seamly2D + SeamlyMe single-threaded and takes ~20 of the job's ~27 minutes.
Branch `task-jom-parallel-nmake` swaps `nmake` for `jom -j
$env:NUMBER_OF_PROCESSORS` (Qt's parallel nmake clone) in both
`windows-test`'s and `windows-msi`'s build steps in `ci.yml`. `nmake check`
(test execution) is left alone on purpose — the four Qt suites share one `-o`
log target and would clobber each other if run concurrently.

jom ships x86/x64 only; the `windows-msi` matrix's `arm64` leg
(`windows-11-arm`) runs it too, on the assumption Windows 11 ARM's x64
emulation handles it — user's explicit call, unverified until that leg's CI
run is watched.

`packaging\windows\local_build_msi.ps1`'s docstring (lines 26-29) already
claims "Uses jom instead of nmake" but its body still calls plain `nmake` /
`nmake check` (lines 222, 247) — a pre-existing mismatch, not touched by this
task. Worth a follow-up.

**Next step:** merge `task-jom-parallel-nmake` into `run-seamlyLayout`, push
without the skip-ci token (`.github/workflows/**` changed functionally), and
watch the `ci.yml` run — this is the only way to confirm both the timing win
and the arm64 leg.

## Current steps

1. build .msi with `packaging\windows\local_build_msi.ps1`
2. clear environment with `packaging\windows\local_reset_environment.ps1` — needs
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
are gone. Build only with `local_build_msi.ps1` — do **not** use
`src\app\seamlylayout\build.ps1` or `qd.ps1` any more. Both `CLAUDE.md` files and
`src/app/seamlylayout/.claude/rules/testing.mdc` were corrected to say so.

## Commit state

`run-seamlyLayout` is **pushed and level with `origin/run-seamlyLayout`**
(`ff7d8c441b`). Nothing is waiting to commit except three files that are not
task work:

- `scripts/prompt_testing.txt` — the user's own edit, made before this session;
- `src/libs/vmisc/projectversion.cpp` / `.h` and the two `packaging/macos` `Info.plist`
  files — the version stamp, which every local build rewrites.

The push carried `ff7d8c441b` (the `--no-ff` merge) and `3efd1017ed` (the
`MSI1b.1` task work), and **no skip-ci token**: `packaging/**` changed
functionally, so the full `ci.yml` suite runs on it. That run is the only
verification Seamly2D and SeamlyMe get — check it.

## Machine state

- Installed: Seamly **26.9.2.1059** in `%ProgramFiles%\SeamlyApps`
  (seamly2d.exe, seamlyme.exe, SeamlyLayout.exe). MSI ProductVersion `26.9.2499`.
- Installed 2026-09-02 through the **wizard** from an elevated shell, onto a
  machine reset by `local_reset_environment.ps1`. Install log:
  `%TEMP%\seamly_install.log`.
- `%DATAROOT%` = `C:\Users\susan\Documents\SeamlyData`. Right after the install
  it is an **empty** directory: the MSI creates it, and the first app run seeds
  it. Same for SeamlyLayout's `default_preferences.json` /
  `default_settings.json`.
- All three apps have been run once, so every first-run artifact exists.
  `%DATAROOT%` now holds 8 subdirectories, 8 patterns, 3 individual and 1
  multisize measurement file.
- `packaging/windows/local_install_msi.ps1` cannot check this pass. It needs
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

### Build instructions corrected in the rule files

`CLAUDE.md`, `src/app/seamlylayout/CLAUDE.md` and
`src/app/seamlylayout/.claude/rules/testing.mdc` all told a future session to
build with `build.ps1` / `qd.ps1`. They now name
`packaging\windows\local_build_msi.ps1` and retire the other two. The
project `CLAUDE.md` also documents the four build stages, the `-SkipTests` and
`-SkipValidation` switches, that arm64 is not covered, and how to read a single
Qt suite's output.

Both `build.ps1` and `qd.ps1` are still on disk — marked unsupported, not
deleted. Deleting them was not asked for.

## Verification status

| Suite | How | Result |
| --- | --- | --- |
| Seamly2DTest, CollectionTest, ParserTest, TranslationsTest | `nmake check` inside `local_build_msi.ps1` | pass |
| SeamlyLayout Qt tests | `ctest --preset debug` | 6/6, including the new `LoggerTests` |
| `LoggerTests` alone | per-suite log via `-o <file>,txt` | 7 passed, 0 failed |
| SeamlyLayout Rust | `cargo test --workspace` | pass |
| MSI 26.9.2.996 | `local_build_msi.ps1` | MSI OK, 164.6 MB; authoring check and 17 installer self-tests pass |
| Test Case 1b-i, fresh wizard install of 26.9.2.996 | manual walkthrough, 2026-09-02 | **pass, no failures** |

To read a single Qt suite's output, set `SEAMLY_TEST_LOG_DIR` and run the
binary through its own `target_wrapper.bat` — three of the four qmake suites are
GUI-subsystem binaries that print nothing to a console, and a shared `-o` target
is overwritten by each `qExec()` call. The CMake SeamlyLayout suites are
GUI-subsystem too; run one with `-o <file>,txt` to read its result.

**`cargo test` needs MSVC's `link.exe` first on PATH.** `%ProgramFiles%Git\usr\bin`
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
3. **Watch the `ci.yml` run on `ff7d8c441b`.** It is the first full CI run since
   the last three pushes, and the only verification the Seamly2D and SeamlyMe
   Qt code gets.

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
