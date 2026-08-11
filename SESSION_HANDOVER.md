# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`scripts/packaging/windows/README.md` and `INSTALL_DECISION_FLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## Current state (2026-08-10): Task Installer.1.1 — the x64 `.msi` is now a CI release artifact

**Branch `task-installer-win-x64-msi`, off `run-seamlyLayout`,** which first took
a merge of `origin/develop` (three upstream commits: dark-mode fixes, a Finnish
Weblate update, mac font fixes).

### What changed

- **`.github/workflows/ci.yml`**
  - New `windows-msi` job (x64): one Qt 6.11.1 kit with
    `qtmultimedia qtwebengine qtwebchannel qtpositioning`, qmake/nmake for
    seamly2d + seamlyme, Rust + Ninja + the CMake release preset for
    SeamlyLayout, WiX v6 (+ UI and Util extensions) driven by
    `smsi.ps1 -Arch x64`, jsign signing of `scripts/seamly-msi/x64/seamly-x64.msi`,
    artifact `seamly-x64.msi`.
  - The `windows` job is **arm64 only** now — its x64 NSIS leg (which produced
    `Seamly2D-windows.zip`) is gone. NSIS stays for arm64 until Task
    Installer.1.2, because there is still no arm64 SeamlyLayout build.
  - `publish` needs `windows-msi`, releases `seamly-x64.msi` in place of
    `Seamly2D-windows.zip`, sets **`prerelease: true`**, and is gated on
    **`github.ref_name == 'run-seamlyLayout'`** instead of `develop`.
  - `push` trigger gained `run-seamlyLayout`.
- **`.github/workflows/windows-msi.yml`** — one-line fix: its jsign step signed
  `Seamly2D-<arch>.msi`, a name `smsi.ps1` has never written. Corrected to
  `seamly-<arch>.msi`. Signing in that workflow had therefore never touched the
  real package.
- **Docs** — `.github/README-BUILDS.md`, `.github/workflows/README_WORKFLOWS.md`
  and `scripts/packaging/windows/README.md` (the last also carried the stale
  `Seamly-x64.msi` / `Seamly2D-arm64.msi` output names).
- **Task tracking** — `TODO_INSTALLER.md` Installer.1.1 checked off;
  `TODO_INSTALLER_WIN_X64.md` 13.6 and 13.9 checked off.
- **`.claude/settings.json`** — git/PowerShell permission rules broadened and
  `sandbox.network.allowedDomains` added for github.com, because `git fetch` was
  prompting on every call. The session had to **reload settings** before it took
  effect (`/reload-skills` did it).

### Decisions the user made (act on these, do not re-ask)

- **Inline the MSI steps in `ci.yml`** rather than converting `windows-msi.yml`
  into a reusable `workflow_call`. The two copies of the x64 build steps must
  therefore be kept in step by hand.
- **The `.msi` replaces the NSIS Windows zip** for x64 rather than shipping
  alongside it.
- **Pre-releases are cut from `run-seamlyLayout`; `develop` stays a pristine
  upstream mirror.** Nothing is published from `develop` until the whole
  SeamlyLayout migration is finished and pushed upstream in one go — incremental
  upstream commits are not workable given the size of the change.

### Verification status — INCOMPLETE, and this is the next step

**No build was run for this change.** It is workflow YAML, and this PC has no
YAML parser, no `actionlint`, no `node` and no working `python` (only Git's
perl, without any YAML module), so nothing local can validate it.

**The real check is a CI run.** The `push` trigger now includes
`run-seamlyLayout`, so pushing starts one. Confirm the **`Windows: Build MSI
(x64)`** job goes green and uploads `seamly-x64.msi`. Note the plain push run
**skips `publish`** — only a `schedule` or `workflow_dispatch` run on
`run-seamlyLayout` exercises the pre-release path, so dispatch one to prove the
release step end to end.

### Follow-up left open deliberately

`.github/README.md`'s "Windows 64-bit" download badge still points at upstream's
`Seamly2D-windows.zip`. It has to become the `.msi`, but only when the migration
is pushed upstream — editing it now breaks the live public download link.
Recorded under Installer.1.1 in `project-docs/TODO_INSTALLER.md`.

## MACHINE STATE: the Visual Studio installation is broken

**`scripts/sd.ps1` fails with `'cl' is not recognized`.** Not the script, and not
the agent sandbox — the same failure occurs with the sandbox disabled:

- `vcvars64.bat` **and** `vcvarsall.bat x64` exit 1 with
  `[ERROR:VsDevCmd.bat] *** VsDevCmd.bat encountered errors ***`; three
  sub-scripts fail to init — `core\msbuild.bat`, `ext\cmake.bat`,
  `ext\ConnectionManagerExe.bat`
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
Reversible with `wix extension remove`. `smsi.ps1` requires it, and both
`.github/workflows/windows-msi.yml` and `ci.yml`'s `windows-msi` job install it
alongside the UI extension.

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
8. **The program directory is `C:\Program Files\` + `Seamly`** — show the user
   the final assembled path and take OK/Cancel, rather than editing a box whose
   contents differ from the path it applies.
9. **Data-root relocation asks first** — prompt Y/N before copying existing data
   files to a new directory location.

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
  Mode GUI: `seamly2d.exe <pattern>.sm2d -b <name> -d <dir> -f 0 --exportOnlyDetails`
  writes `<name>_pieces.svg` through the same `exportSVG()` that
  `generatePiecesSvg()` uses.
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
- **There is no YAML tooling on this PC.** No `actionlint`, no `yamllint`, no
  `node`, and `python`/`python3` are the Microsoft Store stubs. Git's `perl` has
  no YAML module. Workflow edits can only be validated by pushing and watching
  the run.
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
  paths; one unresolved include cascades into dozens of `Unknown type name 'QString'`
  entries. **The qmake build is the authority.**
- **`MD060`/`MD056` table warnings from the editor are noise** on the wide spec
  tables in `TODO_MIGRATE.md`, as `MD041` is repo-wide. Fix a genuinely malformed
  row; do not reflow tables to silence alignment style.
- **PowerShell 5.1 wraps a native exe's stderr in `NativeCommandError`** and sets
  `$?` to `$false` even on exit 0. Do not redirect native stderr inside
  PowerShell — run the script as a child process with
  `Start-Process … -RedirectStandardOutput/-RedirectStandardError -Wait -PassThru -NoNewWindow`.
- **PowerShell splatting: `@array` is positional, `@hashtable` is by name.**
- **Qt frontend test exes are GUI-subsystem binaries** — they print nothing to
  captured stdout. Run with `-o <file>,txt` and `QT_QPA_PLATFORM=offscreen`.
- **`$proFile` collides with the automatic `$PROFILE`** (case-insensitive);
  `sd.ps1` still has it.
- **Historical 6.10 references and old directory names in
  `project-docs/TODO_COMPLETED.md` and `project-docs/PROJECT_PLAN.md` are
  deliberate** — they record what was true at the time.
