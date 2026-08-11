# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`scripts/packaging/windows/README.md` and `INSTALL_DECISION_FLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## Current state (2026-08-11): CI version numbers could be octal C++ literals — fixed

**Branch `task-ci-version-octal`, off `run-seamlyLayout`.**

Run [`31447296387`](https://github.com/seamly/Seamly2D/actions/runs/31447296387)
failed **all four build jobs** (macOS, Linux AppImage, Windows NSIS arm64,
Windows MSI x64) on the same error:

```text
projectversion.cpp:67:43: error: invalid digit '8' in octal constant
   67 | extern const int SUPER_MINOR__VERSION = 048;
```

Not a code regression — a **time-of-day bug**. `ci.yml:35` and
`windows-msi.yml:99` build the version with `date +%Y.%-m.%-d.%-H%M`; the run
started at 00:48 UTC, so the fourth component was `048`, and `scripts/version.sh`
substituted it verbatim into `projectversion.cpp` as a C++ integer literal. A
leading zero makes that literal **octal**: it fails to compile when the minutes
contain an 8 or a 9, and — more quietly — compiles to the *wrong number*
otherwise (`047` is 39). Any build started in the 00:00 UTC hour was affected.

**Fix is in `scripts/version.sh`, not the workflows**, so it covers both YAML
call sites at once: every component is validated as numeric and normalized with
`$((10#…))`, and `VERSIONSTR` is rebuilt from the normalized parts so
`VER_FILEVERSION_STR` and the two `Info.plist` files agree with the integers. A
non-4-part or non-numeric argument is now rejected instead of corrupting the
source files.

`scripts/test_version_script.sh` is new: it drives `version.sh` over the release
case, the `048` regression, the silently-wrong `047` case and an all-zero
component, asserts no `= 0[0-9]` literal is ever written, checks both rejection
paths, and backs up/restores all four files it rewrites. **26/26 assertions
pass.** No local qmake build was run — no C++ changed, and the emitted
`projectversion.cpp` is restored byte-for-byte by the test.

## Verification happens on GitHub CI, not on this PC

User's instruction, 2026-08-10: concentrate on the CI pre-releases and ignore
local MSVC/qmake builds. That also settles what to do about the broken Visual
Studio install below — nothing, for now.

A plain push run **skips `publish`** — only a `schedule` or `workflow_dispatch`
run on `run-seamlyLayout` exercises the pre-release path, so dispatch one to
prove the release step end to end.

**`gh` is installed and authenticated** (2026-08-11), at
`C:\Program Files\GitHub CLI\gh.exe`. Read run state with it directly —
`gh run list --repo seamly/Seamly2D --branch run-seamlyLayout --workflow ci.yml`,
then `gh run view <id>` and `gh run view --job <id> --log-failed`.

## Follow-up left open deliberately

`.github/README.md`'s "Windows 64-bit" download badge still points at upstream's
`Seamly2D-windows.zip`. It has to become `seamly-x64.msi`, but only when the
migration is pushed upstream — the badges link to
`FashionFreedom/Seamly2D/releases/latest`, so editing them now breaks the live
public download link. Tracked as **Task M.12** in `TODO_MIGRATE.md`.

## MACHINE STATE: the Visual Studio installation is broken — and that is now accepted

**The user's decision (2026-08-10): build and verify on GitHub CI; do not try to
build locally with MSVC.** So this section is background, not a blocker. Do not
spend a session repairing Visual Studio unless the user asks.

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
10. **The MSI steps are inlined in `ci.yml`**, not factored out of
    `windows-msi.yml` as a reusable `workflow_call`. The two copies of the x64
    build steps must therefore be kept in step by hand.
11. **The x64 `.msi` replaces the NSIS Windows zip** rather than shipping
    alongside it. NSIS stays for arm64 until Task Installer.1.2, because there is
    still no arm64 SeamlyLayout build.
12. **Pre-releases are cut from `run-seamlyLayout`; `develop` stays a pristine
    upstream mirror.** Nothing is published from `develop` until the whole
    SeamlyLayout migration is finished and pushed upstream in one go —
    incremental upstream commits are not workable given the size of the change.

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
- **`gh` is on `PATH` and authenticated** as of 2026-08-11; plain `gh …` works.
  If a shell ever comes up without it, invoke
  `& "C:\Program Files\GitHub CLI\gh.exe"`.
- **A CI version component with a leading zero is a C++ octal literal.**
  `scripts/version.sh` now normalizes them; do not "simplify" that `$((10#…))`
  away. Covered by `scripts/test_version_script.sh`.
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
