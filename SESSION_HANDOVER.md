# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`scripts/packaging/windows/README.md` and `INSTALL_DECISION_FLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## Current state (2026-08-02): Task 51's install cycle has been run twice; the findings are filed as Tasks 61-67

**Branch `task-51-msi-install-experience`, 12 commits, not pushed, no PR.**
Tasks 51 and 60 stay open in `project-docs/TODO_MIGRATE.md`.

The install-layout rework the user asked for "one thing at a time" is **complete**:
step 1 (install to `C:\Program Files\SeamlyApps`) and step 2a (remove any
pre-existing NSIS installation) are both implemented and both verified on the
laptop. **There is no step 2b** — the user's message was cut off mid-sentence,
and they later said not to expect more after 2a. Do not go looking for it.

### What run 2 proved (2026-08-02, packages `26.7.44158` → `26.7.44161`)

The test machine is **Windows 10 22H2 (10.0.19045), PowerShell 5.1** — not
Windows 11. That matters for one finding below.

1. **Install lands in `C:\Program Files\SeamlyApps`**, unchanged across upgrade.
2. **The NSIS installation is removed by the install** — all five checks passed
   (directory, `Install_Dir` key, ARP entry, Start Menu folder, and the MSI
   provably installed elsewhere), still with **no `CustomAction` in the package**.
   Its `uninstall.exe` is never run; we delete what it created, which
   `RemoveFiles` rolls back.
3. **Task 60's migration works on a real profile** — `~/seamly2d` was copied
   wholesale to `Documents\Seamly`: all eight existing folders including the
   user-added `bodyscans`, plus the nine standard ones so `images` finally
   exists; the legacy tree intact at 4 → 5 files (the gain is the marker); and
   `MIGRATED-TO-SEAMLY.txt` naming the new root and date.

Also passing: one ARP entry after upgrade, newer build, unmoved directory, all
three apps starting and staying running, all three associations resolving, a
real `.sm2d` opening through ShellExecute.

**The `Removed` phase was never run** — the tester issued `msiexec /x` then
`Stop-Transcript`. Nothing about uninstall is verified on a real machine.

### The four failures, neither of which is a package defect

- **Three Start Menu shortcut failures are a second checker bug**, same shape as
  run 1's. `Get-AdvertisedShortcutTarget` accepts `MsiGetComponentPath` states
  **4/5**; the constants are `INSTALLSTATE_LOCAL = 3` and
  `INSTALLSTATE_SOURCE = 4`. All three returned 3 — installed locally — reported
  as broken. **The shortcuts have been correct in both runs and the checker
  wrong in both.** Fix is Task 61.
- **ARP `DisplayIcon`** — diagnosed, no longer a mystery. `ARPPRODUCTICON` is set
  and `ProductInfo(… ProductIcon=seamly2d.ico …)` executes, so Windows Installer
  records the icon as *product metadata* and never writes the registry value.
  The tester's by-eye check corroborates it: `appwiz.cpl` shows icon and
  publisher, the Settings app shows neither, because Settings reads the registry
  directly. Fix is Task 62.

### Tasks 61-67 — run 2's findings, filed 2026-08-02

Filed as tasks rather than Task 51 subtasks because they span three areas (the
checker, the package authoring, the applications).

| Task | Problem |
| --- | --- |
| 61 | checker: `INSTALLSTATE` constants; inventory snapshotted before the migration; sample pattern needs a `.smis` the kit lacks |
| 62 | ARP `DisplayIcon` (and possibly `Publisher`) never written to the registry |
| 63 | wizard says "Seamly2D" while installing three apps |
| 64 | previous-install dialog too long, and names the dead `seamlyData` path; absorbs the 2826 geometry fix |
| 65 | destination-folder wording, and whether `INSTALLFOLDER` becomes `Seamly` |
| 66 | one ARP entry for three apps |
| 67 | first-run modal dialogs swallow a pattern opened by double-click |

**Task 61 must land before the uninstall leg**, or the same four false failures
reappear in the next transcript.

**Task 67 is the only genuine user-facing bug in the set.** Some of what the
tester saw was the checker killing processes, but "double-click a pattern on a
freshly installed machine and it does not open" is a real path, and the three
apps disagree today — Seamly2D and SeamlyMe show a first-run dialog,
SeamlyLayout shows none.

### Decisions the user made on these (act on them, do not re-ask)

- **Task 66 → keep one ARP entry and rename it "Seamly".** Display-only entries
  for SeamlyMe and SeamlyLayout were rejected as misleading (uninstalling
  "SeamlyMe" would remove all three); three products or a Burn bundle is far
  larger than the problem. Note `test_msi_install.ps1` finds the product by
  `UpgradeCode`, not DisplayName — deliberately, because the old NSIS product
  also called itself "Seamly2D" — so the rename breaks assertions, not lookup.
- **Task 63 → the "Seamly2D Project" → "Seamly Project" rename is
  package-and-About ONLY.** `Manufacturer`/`ARPPUBLISHER`, the three exes'
  version resources, and the About boxes. **Source-file copyright headers stay
  "2026 Seamly2D Project"** as `CLAUDE.md` specifies. Do not extend this while
  editing nearby code.

### Still open on Task 51 itself

- **`SeamlyShortcutsDlg` never displays.** The `ControlEvent` row is in the
  shipped MSI and correct in every column (condition `1`, ordering 2, ahead of
  the built-in `NewDialog` at 4), `Dialog` `Attributes = 7`, and the `/l*v` log
  shows **no attempt to create it**. Root cause is the WiX version: the design
  notes assumed v3/v4's `InstallDirDlg`, but this is **WiX 6.0.2**, whose
  `InstallDirDlg` Next publishes `CheckTargetPath` (a v6 built-in in the UI
  extension's `uica.dll`) and the `SpawnDialog` is skipped in that chain.
  `SEAMLYDESKTOPSHORTCUTS` defaults to 1, so shortcuts are created and every
  automated check passes: the default works, the *choice* is never offered.
  **Do not chase this by rebuilding the 165 MB package per attempt** — build a
  small UI-only MSI with the same `ui:WixUI` reference and dialogs; it compiles
  in seconds and can be clicked through and cancelled at Ready.
- The four "verify …" subtasks and Task 13's outstanding subtask all close on
  the uninstall leg.

### Open question, not yet answered

**Task 65 reverses a settled decision.** The tester's requested destination-page
wording implies `C:\Program Files\Seamly`, not `SeamlyApps` — which run 2 just
verified end to end. Renaming invalidates both staged MSIs, every path assertion
in both test scripts, `INSTALL_DECISION_FLOW.md` and the READMEs. There is also
a wrinkle: showing `C:\Program Files\` in the edit box while silently appending
`Seamly` means the control no longer displays the path it edits. **Ask before
changing `INSTALLFOLDER`.**

### Task 60 — implemented, and verified where it counts least

`getDefaultDataRoot()` → `<DocumentsLocation>/Seamly`; `migrateDataTree()` does a
wholesale recursive copy, merge-never-overwrite, size-verified per file,
refusing a destination nested inside the source and never deleting the source;
`markDataTreeMigrated()` / `dataTreeWasMigrated()` handle the marker;
`migrateAdoptedLegacyTree()` repoints `paths/dataRoot` **only after** the copy
verifies. `TST_DataRoot` is 28 cases.

**The one subtask that matters most is untested:** the laptop's legacy tree was
four files, so the copy was instant. **A multi-gigabyte copy still blocks
startup silently** — the user's own tree is ~17 GB on a cloud drive — and the UX
(progress, cancel, or defer-and-offer) is undecided.

Two Task 60 subtasks remain: three-root detection (only the `~/seamly2d`-alone
case has been exercised) and `pruneEmptyLegacyDataRoot()` against the new
three-root world.

### The test kit, and where the evidence is

`scripts/seamly-msi/task51-test-kit/` holds both MSIs, `test_msi_install.ps1`,
`sample-pattern.sm2d`, the annotated `RUN-ME-FIRST.md`, `task51-run2.txt` and
`task51-upgrade.log`. **That whole directory is gitignored** (`.gitignore:120`),
so run 2's transcript and the tester's annotations are **not saved by git**. The
eight earlier transcripts are tracked in `installation-troubleshooting/`; the
user has been offered the same for these and has not answered.

### Next steps

1. Task 61 — the checker fixes; then run the **uninstall leg** on the laptop.
2. Tasks 62, 63, 64, 66 — all touch `seamly-family.wxs` and both test scripts;
   worth doing as one package change and one laptop run.
3. Ask about Task 65 before touching `INSTALLFOLDER`.
4. Decide Task 60's large-copy UX before that migration ships to anyone.
5. Fix `SeamlyShortcutsDlg` via a UI-only test MSI, plus the 373→370 geometry.
6. Repair Visual Studio, then re-run `scripts/sd.ps1`.
7. Push the branch and open the PR to `run-seamlyLayout` once Task 51 lands.

## MACHINE STATE: the Visual Studio installation is broken

**`scripts/sd.ps1` fails with `'cl' is not recognized`.** Not the script, and not
the agent sandbox — the same failure occurs with the sandbox disabled:

- `vcvars64.bat` **and** `vcvarsall.bat x64` exit 1 with `[ERROR:VsDevCmd.bat]
  *** VsDevCmd.bat encountered errors ***`; three sub-scripts fail to init —
  `core\msbuild.bat`, `ext\cmake.bat`, `ext\ConnectionManagerExe.bat`
- The toolset is fine on disk: `cl.exe` 19.51.36252 under
  `VC\Tools\MSVC\14.51.36231`, Windows SDKs 10.0.22621.0 / 10.0.26100.0 present
- **Plain `vswhere -products *` returns nothing**; only
  `vswhere -all -prerelease -legacy` finds
  `C:\Program Files\Microsoft Visual Studio\18\Community`. This is *"Visual
  Studio 2026 Developer Command Prompt v18.8.1"*, a prerelease build, and the
  instance registration looks damaged. Instance data exists at
  `C:\ProgramData\Microsoft\VisualStudio\Packages\_Instances\a9afd7ad`

**Workaround — local only, nothing on the machine and nothing in `sd.ps1` was
changed:** set `PATH`/`INCLUDE`/`LIB` by hand at `VC\Tools\MSVC\14.51.36231` +
SDK `10.0.26100.0`, then run `C:\Qt\Tools\QtCreator\bin\jom\jom.exe -f Makefile`
in `scripts/seamly2d-debug`. **The user should repair VS 18 Community from the
Visual Studio Installer** — until then `sd.ps1` fails for them too.

## Other machine state changed outside the repo

`WixToolset.Util.wixext` 6.0.2 is installed globally
(`wix extension add --global`), because `util:RemoveFolderEx` needs it.
Reversible with `wix extension remove`. `smsi.ps1` requires it, and
`.github/workflows/windows-msi.yml` installs it alongside the UI extension.

## SAFETY — read before deleting anything

- **`C:\Users\susan\seamly2d` on the test laptop is live user data.** It holds a
  real pattern of the user's and is the source tree Task 60's migration copies
  from. A `Remove-Item -Recurse` on it was suggested in an earlier session and
  withdrawn: it bypasses the Recycle Bin, and that is how the dev PC's copy was
  permanently destroyed.
- **No test may touch a path under `QDir::homePath()`** — it cannot be faked on
  Windows. First-run resolution is tested through
  `VCommonSettings::chooseFirstRunDataRoot()`, which takes both candidate roots
  as arguments; everything else uses `QTemporaryDir`.
- **`CollectionTest` is deliberately not run locally.** It has a documented
  pre-existing failure *and* launches the real seamly2d, which seeds folders into
  the live data root. CI runs it on a clean machine.

## Decisions the user has ANSWERED (act on these, do not re-ask)

1. **Task 54's file-name form** → **`SettingsCommon.h`**, i.e. the file name
   matches the class name (the style guide's class-match exception wins over the
   `settings_*` snake_case prefix for class-defining files).
2. **`.github/README-DEVELOPER-NEW.md`** → **rename it to
   `.github/README-DEVELOPER-SEAMLY-FAMILY.md`**, to be folded into
   `.github/README-DEVELOPER.md` when the migration is complete. **Not done yet.**
3. **Qt WebChannel / Qt Positioning documentation** → maintain it in
   `.github/README-DEVELOPER-SEAMLY-FAMILY.md` until the migration completes.
4. **`src/app/seamly2d/core/BUILD_PROBLEMS.txt`** → delete it if it is not
   useful. **Not done yet.**
5. **Testing happens on the test laptop, not in a VM** (re-confirmed 2026-07-30).
   This PC is Windows 11 **Home**, which ships neither Hyper-V nor Windows
   Sandbox, so a VM here means a third-party hypervisor; the user considered
   VirtualBox and VMware Workstation Pro and declined both. A VM could not close
   two checklist items anyway — the *verified-publisher* UAC prompt needs Task
   33's signing, and the arm64 repeat needs arm64 hardware.
6. **Data migration is copy-and-verify, leaving the legacy tree intact** — never
   a bare rename, because a user may need to roll back to an earlier release.
7. **The migration lives in the applications, not the installer.** A per-machine
   MSI's server side runs as LocalSystem, so a per-user path resolves to the
   SYSTEM profile and would only cover whoever ran setup; macOS and Linux have no
   MSI at all, so the logic would be written twice.

## Gotchas

- **`CollectionTest.exe` must be run with its working directory set to its own
  `bin/`.** `initTestCase()` removes `tst_seamly2d_tmp` *relative to the CWD* but
  re-creates it under `applicationDirPath()`, so from any other CWD it aborts on
  the leftover directory from the previous run ("Fail to prepare test files for
  testing"). Use `Start-Process … -WorkingDirectory <that bin>`.
- **A SeamlyLayout build-tree exe needs Qt on `PATH` to launch.** There is no
  windeployqt output beside `qt_frontend/build/Debug/SeamlyLayout.exe`, so from a
  plain shell it starts and does nothing — no log file is even created. Prepend
  `C:\Qt\6.11.1\msvc2022_64\bin`. `ctest` handles this itself via the
  `ENVIRONMENT_MODIFICATION` added in Task 58.
- **SeamlyLayout's log file has two independent writers and they overwrite each
  other.** C++ `Logger` holds a buffered `QTextStream` on the file while Rust's
  `log_to_file()` opens/appends per call, so lines get clipped mid-string. Do not
  conclude a log line is absent because it looks truncated — grep for a
  distinctive fragment.
- **A tagged handoff SVG can be produced headlessly**, without driving the Layout
  Mode GUI: `seamly2d.exe <pattern>.sm2d -b <name> -d <dir> -f 0
  --exportOnlyDetails` writes `<name>_pieces.svg` through the same `exportSVG()`
  that `generatePiecesSvg()` uses.
- **`QCommandLineParser::parse()` ≠ `process()`.** `process()` prints to a console
  this GUI-subsystem app does not have on Windows, and calls `exit()`. `parse()`
  returns a bool and fills `errorText()`.
- **A `develop` merge can silently drop doc edits made on this branch.** After
  merging `develop`, `git log -S "<a phrase you added>" -- <file>` is the cheapest
  way to confirm your change survived.
- **`QSettings(fileName, format, parent)` records neither an organization nor an
  application name** — both come back empty, and QSettings substitutes the literal
  `"Unknown Organization"`. Root cause of the stray files in Tasks 34 and 52.
  `QSettings::setPath(format, scope, dir)` *does* redirect settings files, but has
  **no getter** — recover the base from a probe instance.
- **`QDir::fromNativeSeparators()` rewrites backslashes only on Windows** (a
  backslash is a legal POSIX filename character), and Windows path comparison must
  be `Qt::CaseInsensitive`.
- **`QDir::rmdir()` over `removeRecursively()`, deliberately.** `rmdir()` cannot
  delete a file and refuses a non-empty directory, so it cannot run away.
- **`scripts\st.ps1` runs only `Seamly2DTests.exe`.** CI's `make check` runs four
  binaries — `Seamly2DTest`, `CollectionTest`, `ParserTest`, `TranslationsTest`.
  Run the other three by hand before pushing.
- **`gh` is not on this agent shell's `PATH`** — invoke it as
  `& "C:\Program Files\GitHub CLI\gh.exe"`.
- **The sandbox blocks a command containing both a `Remove-Item` and a protected
  path string** — `G:` paths and `C:\Program Files` have both triggered it, even
  when the deletion targets something else entirely (the MSVC environment
  variables set at the top of a build command are enough). Put deletions in
  their own call.
- **Renaming a shadow-build directory invalidates the whole tree.** qmake bakes
  absolute paths into every generated Makefile, so after a rename the sub-builds
  still look for `...\<old name>\...\vtools.lib` and fail with "dependent ...
  does not exist". The tree can only be regenerated, not repaired: delete it and
  re-run qmake. Same failure shape as the toolchain-change trap in
  `.github/README-BUILDS.md`. The build and packaging directory names are now
  `-BuildDirName` / `-OutputDirName` parameters; **a new value needs a matching
  `.gitignore` entry**, and for packaging also the CI artifact path.
- **PowerShell here-strings passed to `git commit -m` get mangled** when the
  message contains quotes; write the message to a file and use `git commit -F`.
- **clangd diagnostics in this repo are noise.** The tree has no
  `compile_commands.json`, so the editor parses each file with zero include
  paths; one unresolved include cascades into dozens of `Unknown type name
  'QString'` entries. **The qmake build is the authority.**
- **`MD060`/`MD056` table warnings from the editor are noise** on the wide spec
  tables in `TODO_MIGRATE.md`, as `MD041` is repo-wide. Fix a genuinely malformed
  row; do not reflow tables to silence alignment style.
- **PowerShell 5.1 wraps a native exe's stderr in `NativeCommandError`** and sets
  `$?` to `$false` even on exit 0. Do not redirect native stderr inside
  PowerShell — run the script as a child process with `Start-Process …
  -RedirectStandardOutput/-RedirectStandardError -Wait -PassThru -NoNewWindow`.
- **PowerShell splatting: `@array` is positional, `@hashtable` is by name.**
- **Qt frontend test exes are GUI-subsystem binaries** — they print nothing to
  captured stdout. Run with `-o <file>,txt` and `QT_QPA_PLATFORM=offscreen`.
- **`$proFile` collides with the automatic `$PROFILE`** (case-insensitive);
  `sd.ps1` still has it.
- **Historical 6.10 references and old directory names in
  `project-docs/TODO_COMPLETED.md` and `project-docs/PROJECT_PLAN.md` are
  deliberate** — they record what was true at the time.
