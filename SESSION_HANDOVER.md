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

`project-docs/TODO_SETTINGS_FILES.md` is deleted; the old steps that named it
are gone. Build only with `test_build_msi_local.ps1` — do **not** use
`src\app\seamlylayout\build.ps1` or `qd.ps1` any more. Both `CLAUDE.md` files and
`src/app/seamlylayout/.claude/rules/testing.mdc` were corrected to say so.

## Nothing is committed

Every change below is in the working tree only. No commit, no branch, no push.
Branch is still `run-seamlyLayout`.

Some modifications predate this session and are not mine:
`project-docs/TEST_MSI_WIN_X64_Test_Case_template.md`, `scripts/prompt.txt`,
and the deletion of `project-docs/TODO_SETTINGS_FILES.md`.

Two new untracked files to add:
`project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md` and
`scripts/prompt_testing.txt`.

Changed this session:
`project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md` (steps 0 and 1b),
`project-docs/TODO_SEAMLYME.md` and `project-docs/TODO_COMPLETED.md`
(`SeamlyMe.5` moved), `scripts/prompt_testing.txt` (step 5),
and this file.

## Machine state

- Installed: Seamly **26.9.2.664** in `C:\Program Files\SeamlyApps`
  (seamly2d.exe, seamlyme.exe, SeamlyLayout.exe). MSI ProductVersion `26.9.2104`.
- Installed 2026-09-02 by `msiexec /i ... /quiet /norestart` from an elevated
  shell, onto a machine reset by `test_reset_environment.ps1`.
- `%DATAROOT%` = `C:\Users\susan\Documents\SeamlyData`, seeded and populated:
  9 subdirectories, 8 patterns, 4 measurement files.
- All three apps have been run once, so every first-run artifact exists.

**Elevation trap.** The VS Code integrated terminal is not elevated, and a
UAC prompt raised from it fails at once with "The operation was canceled by the
user". An unelevated `msiexec /i` fails at `InstallFinalize` with `Error 1925`
and exit 1603 — after every earlier action returned 1, so the log looks healthy
until the last page. Use a separate Administrator PowerShell window.

## Done this session

### Test Case 1b-i walked end to end — PASS

Fresh install of 26.9.2.664. Section A and section B both complete. Every check
passed except B.3, which is `Layout.10` and already filed.

| Group | Result |
| --- | --- |
| A.1a / 1a-i reset | pass |
| A.1b install | pass, status 0 |
| B.0 directories, files, ini contents | pass |
| B.1 registry | pass |
| B.2 apps a-d, human at the keyboard | pass |
| B.3 stray `%LOCALAPPDATA%\SeamlyLayout\output` | fail — `Layout.10` |
| B.4 desktop shortcuts | pass |
| B.5 log errors | pass, none |

**B.2c-ii has independent proof.** SeamlyLayout's own session log records
`main(): opening startup document 'male_shirt' of 50362 characters`. The SVG
arrived as a string. No `.pieces.svg` exists under `%DATAROOT%` or `%TEMP%`.

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
| `TST_SeamlySuitePaths` alone | per-suite log via `SEAMLY_TEST_LOG_DIR` | 22 passed, 0 failed |
| SeamlyLayout Qt tests | `ctest --preset debug` | 5/5; `StartupOptionsTests` 28 passed |
| SeamlyLayout Rust | `cargo test --workspace` | pass |
| MSI | `test_build_msi_local.ps1` | MSI OK, 164.6 MB; authoring check and 17 installer self-tests pass |
| Test Case 1b-i, fresh install of 26.9.2.664 | manual walkthrough, 2026-09-02 | pass; only B.3 fails, and that is `Layout.10` |

To read a single Qt suite's output, set `SEAMLY_TEST_LOG_DIR` and run the
binary through its own `target_wrapper.bat` — three of the four suites are
GUI-subsystem binaries that print nothing to a console, and a shared `-o` target
is overwritten by each `qExec()` call.

## Open — next steps

1. **`Layout.10` — implement it, do not verify it again.** Second confirmed
   failure (2026-09-02, build 26.9.2.664): SeamlyLayout wrote
   `%LOCALAPPDATA%\SeamlyLayout\output\log_260902143926.txt`. Logs belong in
   `%LOCALAPPDATA%\Seamly\SeamlyLayout\logs`.
2. **`Layout.7` — implement it, do not verify it again.** Third confirmed
   failure (2026-09-02, build 26.9.2.664): `DesktopShortcutSeamlyLayout` is
   still written to `HKLM\SOFTWARE\Seamly\Seamly2D` instead of
   `HKLM\SOFTWARE\Seamly\SeamlyLayout`. `SeamlyMe.3` is the same defect for
   `DesktopShortcutSeamlyMe`; fix both together.
3. **`MSI1b.1`** — in `project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md`.
   Error 2826: 15 controls across 10 installer dialogs overflow their dialog by
   7 px. Cosmetic; both `wix msi validate` and `smsi_check_authoring.ps1` miss
   it. Measure the cause first — our custom dialogs are only 3 px over
   (`Width="373"` on a 370 dialog) and that does not explain the stock WixUI
   dialogs, so subtracting 7 from each width would be a guess.
   **Still unverified on 26.9.2.664** — that pass installed with `/quiet`, which
   builds no dialogs. Test Case 1b-i step 1b now mandates the wizard.
4. **Four test-document defects in
   `TEST_MSI_WIN_X64_Test_Case_1b-i.md`, agreed but not yet fixed.** Each makes
   correct behaviour read as a failure:
   - B.0a is ordered wrong. Split it into a post-install part and a
     post-first-run part. It also omits `label templates`, which
     `qt6_seamly2d.ini` requires.
   - B.2c-iii expects `%DATAROOT%\SeamlyLayout\cache` and `\logs`. Neither
     exists, and neither should — `%DATAROOT%` holds user data. The real cache
     is `%LOCALAPPDATA%\Seamly\SeamlyLayout\cache`. The logs expectation also
     contradicts `Layout.10`.
   - B.2b-v names `%LOCALAPPDATA%\SeamlyMe\logs`. The correct path is
     `%LOCALAPPDATA%\Seamly\SeamlyMe\logs`.
   - Placeholder typos: `%DATAROOT%\SeamlyData`, `%PROGRAMDIR%\SeamlyApps`,
     `%DATAROOTT%`, `%DATAROOTROOT%`. B.0b-iii says `qt6_seamly2d.ini` should be
     empty; it means `qt6_seamlyme.ini`.
5. **Commit the working tree** — nothing is committed yet.

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
