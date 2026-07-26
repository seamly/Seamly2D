# Session handover

## Current state: Tasks 30, 31, 44 and 48 are DONE and moved to `COMPLETED.md`

**Date:** 2026-07-25. **Branch:** `run-seamlyLayout`. The work landed via **PR [#17](https://github.com/seamly/Seamly2D/pull/17)** (`task-48-windeployqt-kit` → `run-seamlyLayout`) — **all 12 CI checks green** (both MSI legs, both Windows builds, Linux unit tests, AppImage, macOS, SeamlyLayout Qt 6.11, CodeQL) — **merged**, task branch deleted locally and on origin.

Refer to `TODO_MIGRATE.md` for the tasks still open.

**Status:** every item in the previous handover's *Concrete next steps* list was carried out. The Qt-module blocker (Task 44) is cleared, the `windeployqt` bug (Task 48) is fixed and verified, SeamlyLayout builds and runs on Qt 6.11.1, and the single-shared-runtime MSI was rebuilt and exercised. One new defect was found while verifying and logged rather than fixed (**Task 49**), plus one incidental finding (**Task 50**).

### What was done this session

| Step | Outcome |
| --- | --- |
| **Task 44** — install the missing Qt modules | **Done.** `MaintenanceTool.exe install qt.qt6.6111.addons.qtwebchannel qt.qt6.6111.addons.qtpositioning --accept-licenses --accept-obligations --confirm-command --default-answer`. All eight CMake config packages now present in the 6.11.1 kit |
| **gh CLI** | **Already installed** — `gh` 2.96.0 at `C:\Program Files\GitHub CLI\gh.exe`, authenticated as `slspencer`, default repo pinned to `seamly/Seamly2D`. It is on the *machine* `PATH` but not in this agent shell's inherited environment; a new terminal picks it up |
| **Task 48** — bare `windeployqt` | **Fixed and verified.** All three `win32-msvc` post-link branches now use `qtPrepareTool(WINDEPLOYQT, windeployqt)` |
| **Task 30** — final verify subtask | **Closed**, with one carve-out (Task 49) |
| **Task 31** — rebuild + MSI size subtasks | **Closed**, size measured |
| **Task 30 → `COMPLETED.md`** | **Moved**, along with 31, 44 and 48. `COMPLETED.md`'s newest entries are now these four |

### Files changed

| File | Change |
| --- | --- |
| `src/app/seamly2d/seamly2d.pro`, `src/app/seamlyme/seamlyme.pro`, `src/test/Seamly2DTest/Seamly2DTest.pro` | `win32-msvc` post-link: bare `windeployqt` → `qtPrepareTool(WINDEPLOYQT, windeployqt)` + `$$WINDEPLOYQT`, each with a comment on why the bare name is unsafe |
| `scripts/sb.ps1` | New `Assert-DeployedQtVersion` guard over both parent `bin` dirs; **bug fix** — the SeamlyLayout step splatted an *array* into `build.ps1`, which binds positionally, so it always died on `ValidateSet`; now a hashtable splat |
| `scripts/sd.ps1` | Same guard for the debug tree's `Qt6Cored.dll` |
| `src/app/seamlylayout/build.ps1` | Fail-fast check for `Qt6WebEngine` / `Qt6WebChannel` / `Qt6Positioning` in the selected kit, with both install routes in the error text (Task 44's last subtask) |
| `.github/README-BUILDS.md` | New toolchain bullet: never invoke a Qt tool by bare name on a developer PC; lists every call site and how each is pinned |
| `scripts/packaging/windows/README_WINDOWS_BUILD.md` | New **§3.4** documenting the bug, the fix, and what the 2026-07-23 MSI's Qt version can and cannot be claimed to have been |
| `TODO_MIGRATE.md`, `COMPLETED.md` | Subtasks checked off; Tasks 30/31/44/48 moved; Tasks 49 and 50 added |

### What is verified

- **Parents** — clean `scripts\sb.ps1 -Clean` rebuild. `build\src\app\{seamly2d,seamlyme}\bin\Qt6Core.dll` now report **`6.11.1.0`** (they reported `6.8.7.0` before the Task 48 fix), and both exes launch from their `bin` directories and stay up.
- **Unit tests** — clean debug rebuild via `scripts\sd.ps1` (so the changed `Seamly2DTest.pro` post-link actually re-ran), then `scripts\st.ps1`: **32097 passed, 0 failed across 24 suites**, exit 0.
- **SeamlyLayout** — `scripts\sb.ps1 -SkipParents` configures and builds clean on Qt 6.11.1: `find_package(Qt6 6.11.1 … WebEngineQuick)` succeeds, Corrosion rebuilds `cxxqt_bridge`, `[86/86] Linking CXX executable SeamlyLayout.exe`. The running process loads **`Qt6Core.dll`, `Qt6WebEngineQuick.dll` and `Qt6WebEngineCore.dll`, all `6.11.1.0`** — the bumped `import QtWebEngine 6.11` and the QML/WebEngine load path are genuinely exercised, and the UI renders.
- **MSI** — `smsi.ps1` with its plain default invocation (no `-WinDeployQt6`): **165.3 MB, down from 186.8 MB** (−21.5 MB / −11.5 %). `wix msi validate` clean apart from the expected ICE61. ProductVersion `26.7.34941`, UpgradeCode unchanged. Staging has **no `layout\` tree** and exactly **one** `Qt6Core.dll`. Expanded with `msiexec /a` (1623 files): flat `Seamly2D\` directory, no `SeamlyLayout\` subdirectory, and **all three exes launch from it, each loading the same single `Qt6Core.dll 6.11.1.0`**.
  - The saving is smaller than "two runtimes → one" suggests because the surviving runtime still carries Qt WebEngine, whose `.pak` locales and `QtWebEngineProcess.exe` dominate the payload. What the collapse removes is the duplicated Qt core/GUI/QML DLL set.

### What is NOT verified

- **A real elevated `msiexec /i` system install was not performed** — the verification above used an administrative extraction (`msiexec /a`) plus launching all three exes from the expanded tree. That covers the file layout, the shared runtime and app startup, but not shortcuts, registry entries, file associations or the ARP entry. A real install needs a UAC prompt. Note there is already an **NSIS**-installed Seamly2D on this machine (`C:\Program Files (x86)\Seamly2D`, `uninstall.exe`); the MSI is a separate product code and would install alongside it, not over it. **All of this is now tracked as Task 51**, together with the install-time options and warnings the installer should be offering.
- **`msiexec /a` needs a short target path.** Extracting under a long path fails at `InstallFinalize` with 1603 (MAX_PATH). Not a package defect.

## New tasks logged in `TODO_MIGRATE.md`

| Task | Status | Summary |
| --- | --- | --- |
| **49** | open, **important** | **SeamlyLayout ignores the SVG path seamly2d hands it.** `MainWindow::exportPiecesToSeamlyLayout()` writes `<pattern>.pieces.svg` and calls `QProcess::startDetached(exe, {svgPath}, wd)`, but `qt_frontend/main.cpp` never reads its command line — no `QCoreApplication::arguments()`, no `QCommandLineParser`, nothing. Verified on the fresh 6.11.1 build: the window comes up with both panes empty. **Pre-existing, not a Qt-bump regression** — `git log -S "arguments()"` finds no commit that ever added argument handling |
| **50** | open | A developer's absolute home path is hard-coded in `src/app/seamly2d/core/application_2d.cpp:507-512` (`C:/Users/susan/Projects/Seamly2D-private/…/build/Debug/SeamlyLayout.exe`) as the Layout Mode dev fallback. Harmless elsewhere, but it is a personal path in a GPL source file headed for the upstream PR, and on that one machine it silently prefers a possibly stale Debug build |
| **51** | open | **Windows MSI install-time experience.** Everything Windows Installer does *around* the files, which the extraction-based verification deliberately did not cover: Start Menu shortcuts, the `HKLM\SOFTWARE\Seamly\Seamly2D` registry rows, the Add/Remove Programs entry, `.sm2d`/`.smis`/`.smms` associations end to end, **optional desktop / taskbar shortcuts** offered at install time, a proper **UAC** elevation prompt, and a dialog warning that an existing installation will be replaced **while the user's data stays intact** in the `seamly` user directory. Also folds in the clean-machine install/upgrade/uninstall cycle that Task 13's last subtask still wants |

Still open from before, untouched this session: **45** (stale `C:\Qt\6.10.1` paths in the `.claude` settings allowlists), **46** (`sd.ps1` reuses stale qmake sub-Makefiles after a Qt change — `sb.ps1` already has the `.seamly-qmake-kit` marker fix to port), **47** subtask 4 (optional developer `PATH` cleanup so the real kit precedes Qt Design Studio).

## Concrete next steps (resume here)

1. **Task 49** — make SeamlyLayout consume its positional argument, so Layout Mode actually opens the pattern instead of an empty canvas. Highest-value open item: the handoff is the whole point of the daughter app.
2. **Task 51** — the MSI install-time experience (shortcuts, registry, ARP, associations, desktop/taskbar options, UAC, the "your data is safe" upgrade warning) plus the clean-machine install/upgrade/uninstall cycle. The elevated `msiexec /i` run lives here now; it was deliberately not performed this session.
3. **Task 50** — remove the hard-coded developer path before the upstream PR.
4. **Task 46** — port `sb.ps1`'s `.seamly-qmake-kit` marker to `sd.ps1` so a Qt change wipes the debug tree automatically.
5. **Task 45** — the two-line settings-allowlist cleanup.

## Gotchas seen this session

- **Qt Design Studio poisons `PATH`.** Bare `qmake`, `windeployqt` and `windeployqt6` all resolve to `C:\Qt\Tools\QtDesignStudio\qt6_design_studio_reduced_version\bin\` — a Qt **6.8.7** kit with **no `mkspecs`**. Root cause of Tasks 47 and 48. Never call these bare; use `qtPrepareTool`, `$$[QT_INSTALL_BINS]/…`, or pin `QMAKE`/`PATH`. Every repo call site is now pinned and both build scripts verify the deployed DLL version afterwards.
- **Qt `MaintenanceTool` CLI: name the parent package, not the arch child.** `qt.qt6.6111.addons.qtwebchannel.win64_msvc2022_64` is rejected with *"Component is virtual"*; `qt.qt6.6111.addons.qtwebchannel` installs the right child for the installed kit. The tool logs in with the stored Qt Account and runs unattended with `--accept-licenses --accept-obligations --confirm-command --default-answer`.
- **PowerShell splatting: `@array` is positional, `@hashtable` is by name.** `sb.ps1`'s `@('-Preset','release','-NoRun')` passed the literal string `-Preset` as the *value* of `-Preset`. The old comment in that file explicitly (and wrongly) claimed the array form worked.
- **PowerShell 5.1 + `$ErrorActionPreference='Stop'`:** piping a native command through `2>&1 | …` turns its stderr into a terminating `NativeCommandError` even on exit 0. Redirect at the `cmd` level instead. Long builds in this session were run as detached `cmd` scripts writing a log plus an exit-code sentinel file, which sidesteps this entirely and is not bounded by any tool timeout.
- **`$proFile` collides with PowerShell's automatic `$PROFILE`** (case-insensitive). `sb.ps1` uses `$proPath`; **`sd.ps1` still has the collision** — harmless, worth cleaning up.
- **Qt frontend test exes are GUI-subsystem binaries**, so they print nothing to a captured stdout and `ctest` can appear to hang. Run with `-o <file>,txt` and `QT_QPA_PLATFORM=offscreen`.
- **Historical 6.10 references in `COMPLETED.md` and `PROJECT_PLAN.md` were left alone** deliberately — they record what was true at the time.
