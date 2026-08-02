# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`scripts/packaging/windows/README.md` and `INSTALL_DECISION_FLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## Current state (2026-07-31): Task 51 — the install layout is being reworked; 2 of 3 requested pieces are done

**Branch `task-51-msi-install-experience`, not pushed, no PR yet.** Task 51 stays
open in `project-docs/TODO_MIGRATE.md`.

### The install run that started all of this

The user ran the test kit on the Windows 11 laptop through the upgrade step.
**52 of 57 automated checks passed**, including the ones that matter most: all
three apps start and stay running from the install (so the deployed
Qt/WebEngine runtime is complete), all three file associations resolve and a
real `.sm2d` opens through ShellExecute, desktop shortcuts and their registry
breadcrumbs are correct, and the ARP entry carries the right name, publisher,
version, comments, links, size and uninstall string. Exactly one UAC prompt.
`SeamlyPreviousInstallDlg` displayed correctly and in the right position.

It found three real defects and one bug in the checker itself — none of which
static package inspection could have reached. **This is the argument for doing
real installs.**

### Fixed

1. **The user-data tree was never seeded** (`cfcba75238`). `ensureDataRootTree()`
   creates the nine subfolders, but its only production caller was
   `setDataRoot()`, which runs only when the user *changes* the root in
   Preferences → Paths. First run goes through `initializeDataRoot()`, which
   resolves and records the path directly — so a fresh machine recorded
   `~/seamlyData` and never created it, and an adopted legacy tree never gained
   the subfolders it lacked. `pruneEmptyLegacyDataRoot()`'s own doc comment had
   been asserting the opposite all along. Both apps now call
   `ensureDataRootTree(dataRoot())` from `openSettings()` — **in the
   applications, never inside `initializeDataRoot()`**, because that is the only
   place the real home directory reaches it and the unit tests do call
   `initializeDataRoot()`. New `TST_DataRoot::StartupResolvesThenSeedsTheConfiguredRoot`
   pins both halves, including that resolution stays free of disk side effects.

   **Do not mistake root *adoption* for this bug.** On the laptop `~/seamlyData`
   was correctly absent and the live data root is `C:\Users\susan\seamly2d`: the
   old NSIS build left that directory, and `chooseFirstRunDataRoot()` adopts an
   existing legacy tree **in place** whenever `~/seamlyData` does not exist and
   `~/seamly2d` is a directory (`vcommonsettings.cpp:735`) — deliberate Task 34
   behaviour so an upgrading user's gigabytes are never moved.
   **`C:\Users\susan\seamly2d` on that laptop is live user data; never delete
   it.** A `Remove-Item -Recurse` on it was suggested in this session before the
   adoption was understood, and withdrawn — it bypasses the Recycle Bin, and is
   how the dev PC's copy was destroyed in an earlier session.

2. **The checker mis-handled advertised shortcuts** (`cfcba75238`). All three
   Start Menu shortcuts "failed" with a target of
   `C:\Windows\Installer\{ProductCode}\*.ico`. They are advertised — nested
   inside `<File KeyPath="yes">` with no `Target`, WiX's standard pattern — and
   **`WScript.Shell` does not report an advertised shortcut's target; it returns
   the extracted icon path.** The script assumed such a shortcut came back
   *empty*, so that branch was never reached and three correct shortcuts failed
   every run. It now resolves the Darwin descriptor via `MsiGetShortcutTarget` +
   `MsiGetComponentPath`, asserting something stronger: that the shortcut
   resolves to an installed file inside the install directory.

3. **The checker watched the wrong data directory** (`90a91afa3f`).
   `Get-UserDataInventory` followed only the *configured* root, which (a) misses
   the adopted legacy tree holding the user's patterns on any upgraded machine
   and (b) **changes** the moment the apps first write `paths/dataRoot`, so
   Baseline and a later phase inventoried different directories and
   `Assert-UserDataIntact` — which matches on `Path` — reported a meaningless
   failure. It now takes a fixed, de-duplicated set: the configured root, both
   candidate roots, and both settings folders.

### The user's rework, "one thing at a time"

1. **DONE — install to `SeamlyApps`** (`b773f40b3f`). `INSTALLFOLDER` is now
   `ProgramFiles64Folder\SeamlyApps` (was `\Seamly2D`). The user first asked for
   `C:\Program Files (x86)\SeamlyApps` and **accepted the 64-bit tree instead** —
   every binary here is x64/arm64, and only the *old NSIS* product belongs under
   `(x86)` because its installer was 32-bit. **User data was deliberately left to
   the app**; a per-machine MSI's server side runs as LocalSystem, so
   `C:\Users\<name>\...` cannot be created meaningfully at install time and would
   only ever cover the installing user. That acceptance was conditional —
   *"leave it to the app if we can migrate the existing subfolders to the new
   subfolders"* — and **that migration is Task 14, NOT done.** It needs a
   decision, because today's rule deliberately adopts in place rather than
   moving. Ask which is meant: `~/seamly2d` → `~/SeamlyData`, or old subfolder
   names → the nine standard ones.

2. **DONE — remove the old NSIS installation** (`59e3494690`). A deliberate
   reversal of the earlier "detect it, never remove it" decision, on the user's
   reasoning that the MSI is a strict superset: NSIS installs seamly2d and
   seamlyme, this package installs both plus SeamlyLayout, so leaving it behind
   means two copies of each parent app and Start Menu shortcuts that launch the
   old binaries. **Its `uninstall.exe` is still never run** — that is what made
   the original decision right, and every hazard in it came from invoking that
   EXE. We delete what it created instead, which `RemoveFiles` rolls back.

3. **NOT STARTED — 2b.** **The user's message was cut off mid-sentence at
   "2b. it checks for user", so the requirement is unknown.** Get it before
   implementing anything further.

### Still open on Task 51

- **`SeamlyShortcutsDlg` never displays.** The `ControlEvent` row is in the
  shipped MSI and correct in every column (condition `1`, ordering 2, ahead of
  the built-in `NewDialog` at 4), and the `Dialog` row has `Attributes = 7`. The
  `/l*v` log shows **no attempt to create it**. Root cause is the WiX version:
  the design notes assumed WiX v3/v4's `InstallDirDlg`, but this is **WiX
  6.0.2**, whose `InstallDirDlg` Next publishes `CheckTargetPath` — a v6
  built-in from the UI extension's `uica.dll` — and the `SpawnDialog` is skipped
  in that chain. `SEAMLYDESKTOPSHORTCUTS` defaults to 1, so shortcuts are
  created and every automated check passes: the default works, the *choice* is
  never offered. **Do not chase this by rebuilding the 165 MB package per
  attempt** — build a small UI-only MSI with the same `ui:WixUI` reference and
  the same dialogs; it compiles in seconds and can be clicked through and
  cancelled at the Ready page without installing anything.
- **Dialog geometry.** `SeamlyPreviousInstallDlg`'s `BannerLine`/`BottomLine` are
  `Width="373"` on a 370-wide dialog → error 2826 twice. Stock WixUI dialogs log
  the same code at `DEBUG:` only; ours is *also* logged as a user-facing
  "unexpected error". Three characters to fix.
- **ARP `DisplayIcon` came back empty**, although `ARPPRODUCTICON = seamly2d.ico`
  *is* in the built MSI's Property table and the Icon table has the matching row
  (both verified by querying the package). Authoring is correct; cause unknown.
  Asked for `MsiGetProductInfo`'s `ProductIcon` and whether Apps & features
  paints the right icon — **not yet answered**.
- **The NSIS removal has never run against a real NSIS install.** It is verified
  at package level only (63 authoring assertions). **The user is re-running the
  `.nsi` on the laptop as of 2026-07-31 specifically so this can be tested.**
- `-Phase Upgraded` / `-Phase Removed` on the laptop.

### Verification of everything committed today

Debug build clean · `scripts/st.ps1` **32134 passed, 0 failed across 25 suites**
(`TST_DataRoot` 22 → 23) · `ParserTest` exit 0 · `TranslationsTest` exit 0 ·
`smsi.ps1` exit 0 with `wix build` clean, `wix msi validate` clean apart from the
expected ICE61, **authoring 63/63**, `Seamly2D-x64.msi` 165.4 MB.

**`CollectionTest` was deliberately not run locally** — it has a documented
pre-existing failure *and* it launches the real seamly2d, which now seeds
folders into the live `G:\My Drive\seamlyData`. CI runs it on a clean machine.

### MACHINE STATE: the Visual Studio installation is broken

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

**Workaround used for this session's build — local only, nothing on the machine
and nothing in `sd.ps1` was changed:** set `PATH`/`INCLUDE`/`LIB` by hand at
`VC\Tools\MSVC\14.51.36231` + SDK `10.0.26100.0`, then run
`C:\Qt\Tools\QtCreator\bin\jom\jom.exe -f Makefile` in `scripts/seamly2d-debug`.
**The user should repair VS 18 Community from the Visual Studio Installer** —
until then `sd.ps1` fails for them too.

### Other machine state changed this session

`WixToolset.Util.wixext` 6.0.2 was installed globally
(`wix extension add --global`), because `util:RemoveFolderEx` needs it. Reversible
with `wix extension remove`. `smsi.ps1` now requires it, and
`.github/workflows/windows-msi.yml` installs it alongside the UI extension.

### Next steps

1. Get **2b** from the user; it is the only thing blocking step 2's completion.
2. Fix `SeamlyShortcutsDlg` (UI-only test MSI) and the 373→370 geometry; assert
   both in `test_msi_authoring.ps1` **and** confirm with a real wizard run,
   since authoring passed while the page never appeared.
3. Build a fresh package and run the full cycle on the laptop — now including
   the NSIS-removal path, which the user is re-creating.
4. Repair Visual Studio, then re-run `scripts/sd.ps1` to confirm the normal
   build path works.
5. Push the branch and open the PR to `run-seamlyLayout` once Task 51's
   remaining subtasks land.

## Decisions the user has ANSWERED (act on these, do not re-ask)

1. **Task 54's file-name form** → **`SettingsCommon.h`**, i.e. the file name
   matches the class name (the style guide's class-match exception wins over the
   `settings_*` snake_case prefix for class-defining files).
2. **`.github/README-DEVELOPER-NEW.md`** → **rename it to
   `.github/README-DEVELOPER-SEAMLY-FAMILY.md`**, to be folded into
   `.github/README-DEVELOPER.md` when the migration is complete. **The rename has
   not been done yet.**
3. **Qt WebChannel / Qt Positioning documentation** → maintain it in
   `.github/README-DEVELOPER-SEAMLY-FAMILY.md` until the migration completes.
4. **`src/app/seamly2d/core/BUILD_PROBLEMS.txt`** → delete it if it is not
   useful. **Not done yet.**
5. **Testing happens on the test Windows 11 laptop, not in a VM** (re-confirmed
   2026-07-30). This PC is Windows 11 **Home**, which ships neither Hyper-V nor
   Windows Sandbox, so a VM here means a third-party hypervisor; the user
   considered VirtualBox and VMware Workstation Pro and declined both. A VM could
   not close two checklist items anyway — the *verified-publisher* UAC prompt
   needs Task 33's signing, and the arm64 repeat needs arm64 hardware.

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
  `removeRecursively()` also bypasses the Recycle Bin — that is how
  `C:\Users\susan\seamly2d` was permanently destroyed.
- **`QDir::homePath()` cannot be faked on Windows**, so no test may touch a path
  under it. First-run resolution is tested through
  `VCommonSettings::chooseFirstRunDataRoot()`, which takes both candidate roots as
  arguments.
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
  `.github/README-BUILDS.md`.
- **PowerShell here-strings passed to `git commit -m` get mangled** when the
  message contains quotes; write the message to a file and use `git commit -F`.
- **clangd diagnostics in this repo are noise.** The tree has no
  `compile_commands.json`, so the editor parses each file with zero include
  paths; one unresolved include cascades into dozens of `Unknown type name
  'QString'` entries. **The qmake build is the authority.**
  `src/app/seamly2d/core/BUILD_PROBLEMS.txt` is a tracked 45-entry dump of exactly
  this, carrying absolute `/c:/Users/susan/…` paths into source headed for the
  upstream PR — see decision 4 above.
- **PowerShell 5.1 wraps a native exe's stderr in `NativeCommandError`** and sets
  `$?` to `$false` even on exit 0. Do not redirect native stderr inside
  PowerShell — run the script as a child process with `Start-Process …
  -RedirectStandardOutput/-RedirectStandardError -Wait -PassThru -NoNewWindow`.
- **PowerShell splatting: `@array` is positional, `@hashtable` is by name.**
- **Qt frontend test exes are GUI-subsystem binaries** — they print nothing to
  captured stdout. Run with `-o <file>,txt` and `QT_QPA_PLATFORM=offscreen`.
- **`$proFile` collides with the automatic `$PROFILE`** (case-insensitive);
  `sd.ps1` still has it.
- **Historical 6.10 references in `project-docs/TODO_COMPLETED.md` and
  `project-docs/PROJECT_PLAN.md` are deliberate** — they record what was true at
  the time.
