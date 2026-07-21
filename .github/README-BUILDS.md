# Seamly Builds — Knowledge Base

Pertinent knowledge about the Seamly family builds: why decisions were made, what is built, where things install and store data, and when/where each build runs. Update this file whenever build knowledge changes — it is the durable record behind the task entries in `TODO.md` / `COMPLETED.md`.

Apps covered:

- **seamly2d** — pattern drafting (parent app), `src/app/seamly2d`, Qt 6 / C++ / qmake
- **seamlyme** — measurements, `src/app/seamlyme`, Qt 6 / C++ / qmake
- **seamlyLayout** — daughter layout app, `src/app/seamlylayout/`, Rust + Qt 6.10/QML, own build (`src/app/seamlylayout/qd.ps1`), deliberately outside the Seamly2D qmake build

## The apps are a family, not standalone programs

This constrains every packaging decision below:

- seamly2d launches seamlyme and seamlyLayout as detached processes (`QProcess::startDetached` in `src/app/seamly2d/mainwindow.cpp`; seamlyLayout via `exportPiecesToSeamlyLayout()`).
- seamly2d hands a tagged `.pieces.svg` file to seamlyLayout (Layout Mode handoff; attribute spec in `status-docs/new-attributes.csv`).
- The apps share files and variables: measurement files, settings values (e.g. the `paths/seamlyLayoutApp` executable path stored via `VSettings`, `src/libs/vmisc/vsettings.cpp`).

Therefore all packaging must keep the apps installed together (or mutually locatable) and able to see the same user data. On sandboxed platforms (Flatpak) they must share one sandbox.

## Toolchains (Windows development)

Two toolchains are in use — the difference is intentional, not an error:

| | Where | Qt | Compiler | Notes |
|---|---|---|---|---|
| **CI** | GitHub hosted runner | 6.8.3 | MSVC 2022 | Used by release/CI workflows; limited to what GitHub runners provide |
| **Local** | Developer PC | 6.10.1 `msvc2022_64` | VS 18 Community (`vcvars64.bat`) | qmake + jom; release shadow-build in `build/` (gitignored) |

- Local debug build: `scripts/sd.ps1` — auto-detects the newest Qt 6.10.x msvc2022_64 kit under `C:\Qt` and the VS 18 MSVC environment, shadow-builds `CONFIG+=debug` into `seamly2d-build-debug/` (gitignored); debug exe at `seamly2d-build-debug/src/app/seamly2d/bin/seamly2d.exe`, Qt debug DLLs deployed by windeployqt; `-Run` launches after build.
- seamlyLayout builds separately with `src/app/seamlylayout/qd.ps1` and must stay out of the qmake build.

## Settings / preferences storage

### Windows (as of Task 15, 2026-07)

`VER_COMPANYNAME_STR` (`src/libs/vmisc/projectversion.h`) and seamlyLayout's `app.setOrganizationName(...)` (`src/app/seamlylayout/qt_frontend/main.cpp`) are both `"Seamly"`, so every app gets its own directory nested under one shared organization folder:

| App | Windows location | How it gets there |
|---|---|---|
| seamly2d | `C:\Users\<user>\AppData\Local\Seamly\Seamly2D\qt6_seamly2d.ini` | `Application2D::openSettings()` resolves `QStandardPaths::AppConfigLocation` explicitly (previously it used Qt's native `QSettings(IniFormat, UserScope, org, app)` resolution, which put a flat `Seamly2D.ini` in `%APPDATA%\Roaming\<org>\` alongside SeamlyMe's) |
| seamlyme | `C:\Users\<user>\AppData\Local\Seamly\SeamlyMe\qt6_seamlyme.ini` | same mechanism, `ApplicationME::openSettings()` |
| seamly2d + seamlyme shared "common" settings (`VCommonSettings`, e.g. individual/multisize table paths) | `%APPDATA%\Roaming\Seamly\qt6_common.ini` | unchanged mechanism — Qt's native per-organization `QSettings(IniFormat, UserScope, org, "qt6_common")` resolution, just under the renamed org folder |
| seamlyLayout | `C:\Users\<user>\AppData\Local\Seamly\SeamlyLayout\` | `QStandardPaths::AppConfigLocation` (already used before Task 15 — only the org name changed) |
| seamlyLayout (packaged defaults) | `<exeDir>\settings\` (relative to `seamlyLayout.exe`) | Inno Setup installs `default_settings.json` and paper/roll presets there; read-only legacy-migration source only, never written to at runtime |

**First-run migration (non-destructive, copy-if-missing, left in place):** each app bridges its own settings forward from its pre-Task-15 location the first time the new location is resolved:
- seamly2d/seamlyme: `VAbstractApplication::MigrateSeamlySettingsLocation()` (`src/libs/vmisc/vabstractapplication.h/.cpp`) copies from the old shared `"Seamly2DTeam"` organization folder; a one-time `NotifySeamlySettingsMigrated()` dialog tells the user, shown only in confirmed GUI mode (never during a headless CLI export or an automated test) after command-line parsing has run.
- seamlyLayout: `appConfigRootPath()` (`PreferencesModel.cpp`) and `defaultSettingsFilePath()` (`SettingsModel.cpp`) each recursively copy the whole legacy `"Seamly Systems"` AppConfigLocation tree forward; the Inno Setup installer's upgrade-guard dialog (`SeamlyLayout.iss`) also mentions the org-folder rename.

### macOS (as of Task 16, 2026-07)

All three apps use the same `QStandardPaths::AppConfigLocation` / `QSettings::IniFormat` code paths as Windows (Task 15) — no OS-specific branching was needed there, since `QStandardPaths` resolves the platform location generically from `organizationName`/`applicationName`. `QSettings::NativeFormat`/CFPreferences plists are **not** used by any Seamly app, so `CFBundleIdentifier` does not factor into settings resolution.

| App | macOS location |
|---|---|
| seamly2d | `~/Library/Application Support/Seamly/Seamly2D/qt6_seamly2d.ini` |
| seamlyme | `~/Library/Application Support/Seamly/SeamlyMe/qt6_seamlyme.ini` |
| seamly2d + seamlyme shared "common" settings | `~/Library/Preferences/Seamly/qt6_common.ini` (Qt's native per-organization `IniFormat`/`UserScope` resolution on macOS) |
| seamlyLayout | `~/Library/Application Support/Seamly/SeamlyLayout/{settings,preferences,input,output}/` |

**First-run migration** uses the same generic `MigrateSeamlySettingsLocation()` / `migrateLegacyOrganizationTree()` logic as Windows (see above) — both reconstruct the legacy path by temporarily swapping `organizationName` and re-querying `AppConfigLocation`, so no macOS-specific path literals were needed.

**Bundle-relative writable paths removed (Task 16):** seamlyLayout's default input/output folders and its debug log directory previously fell back to `<exeDir>/input`, `<exeDir>/output` when unconfigured — inside a signed, notarized `.app` bundle `Contents/MacOS/` is read-only, so those `mkpath()` calls would silently fail. `PreferencesModel::defaultInputFolderUrl()`/`resolvedInputDirectory()`/`resolvedLayoutDirectory()` and `Logger::init()` now branch on `Q_OS_MACOS` to use the writable `AppConfigLocation` root instead; Windows/Linux behavior is unchanged. Packaged defaults in `Contents/Resources/settings/` remain read-only, copied in only as a legacy-migration source (never written to at runtime).

**Bundle identifier:** seamlyLayout's CMake build previously set no `MACOSX_BUNDLE_GUI_IDENTIFIER` at all (an auto-generated placeholder); Task 16 added `io.seamly.SeamlyLayout` (`src/app/seamlylayout/qt_frontend/CMakeLists.txt`) so the bundle is well-formed for signing/notarization. This is unrelated to settings storage (see above) — seamly2d/seamlyme's existing `org.seamly2dproject.@EXECUTABLE@` identifiers (`dist/macx/*/Info.plist`) were left as-is for the same reason.

**Not yet verified:** Task 16's code changes were made and build-verified on Windows (seamlyLayout is cross-platform Qt/CMake — the `Q_OS_MACOS` branches compile out on other platforms) but have not been exercised on real macOS hardware or the `macos-15` CI runner (no Mac available in this environment). Fresh-install and upgrade-with-legacy-data verification remains an open item — see `TODO.md` Task 16.

### Linux — AppImage (as of Task 17, 2026-07)

All three apps use the same generic `QStandardPaths::AppConfigLocation` / `QSettings::IniFormat` resolution as Windows (Task 15) and macOS (Task 16) — Linux resolves `AppConfigLocation` as `$XDG_CONFIG_HOME/<organizationName>/<applicationName>` (typically `~/.config/<org>/<app>`), so no Linux-specific branching was needed for the base directory move either.

| App | Linux (XDG) location |
|---|---|
| seamly2d | `~/.config/Seamly/Seamly2D/qt6_seamly2d.ini` |
| seamlyme | `~/.config/Seamly/SeamlyMe/qt6_seamlyme.ini` |
| seamly2d + seamlyme shared "common" settings | `~/.config/Seamly/qt6_common.ini` (Qt's native per-organization `IniFormat`/`UserScope` resolution on Linux) |
| seamlyLayout | `~/.config/Seamly/SeamlyLayout/{settings,preferences,input,output}/` |

**First-run migration** reuses the same generic `MigrateSeamlySettingsLocation()` / `migrateLegacyOrganizationTree()` logic as Windows/macOS (see above): the legacy path is reconstructed by temporarily swapping `organizationName` to the pre-Task-15 value (`"Seamly2DTeam"` for seamly2d/seamlyme, `"Seamly Systems"` for seamlyLayout) and re-querying `AppConfigLocation`, so it resolves the real legacy XDG folder (`~/.config/Seamly2DTeam`, `~/.config/Seamly Systems`) with no Linux-specific path literals.

**AppImage-specific overrides checked and found not to matter:** the AppImage runtime does not override `$HOME`, `$XDG_CONFIG_HOME`, or `$XDG_DATA_HOME` in the processes it execs, so `QStandardPaths` resolves the same real user-profile location whether the app is installed natively or run from an AppImage. No code in the tree reads `$APPDIR` or implements a "portable mode" that would need reconciling with the above.

**Bundle-relative writable paths (Task 17):** an AppImage mounts its payload read-only (a FUSE-mounted squashfs), the same problem Task 16 found for a signed macOS `.app` bundle. seamlyLayout's default input/output folders and its debug log directory previously fell back to `<exeDir>/input`, `<exeDir>/output` when unconfigured — unlike the macOS case this can't be told apart at compile time, so `Platform::isAppImage()` (`src/app/seamlylayout/qt_frontend/src/Platform.h`) checks for the `APPIMAGE` environment variable the AppImage runtime sets in every process it execs. `PreferencesModel::defaultInputFolderUrl()`/`resolvedInputDirectory()`/`resolvedLayoutDirectory()` and `Logger::init()` now branch on it at runtime to use the writable `AppConfigLocation` root instead; a normal (non-AppImage) Linux install, and Windows, are unaffected. Packaged defaults would ship inside the AppImage's read-only squashfs mount, so — as already noted below — that mount enforces read-only bundled defaults on its own, with no code change needed.

**Not yet verified — seamlyLayout is not currently packaged into the Linux AppImage at all:** `ci.yml`'s `linux` job builds only seamly2d's AppImage (`dist/seamly2d.desktop`); Task 20 (still open in `TODO.md`) adds a separate CI job that *builds and tests* seamlyLayout on Linux/Qt 6.10 but does not add it to the AppImage. The `Platform::isAppImage()` code path above is therefore verified by unit test (`PreferencesModelTests`, which sets the `APPIMAGE` environment variable directly since the check itself is a plain env-var read) and by Windows build/test, but not yet exercised by a real packaged Linux AppImage — mirroring how Task 16's macOS code changes were verified without real macOS hardware.

**Not yet unified (Task 18):**

| Platform | Unified location | Task |
|---|---|---|
| Linux Flatpak | `~/.var/app/<app-id>/config/Seamly` inside the **single shared** sandbox | Task 18 |

## User data files (patterns, measurements)

- Default user data tree on Windows: `C:\Users\<user>\seamly2d`.
- Users legitimately relocate it — e.g. to a cloud-synced drive (`G:\My Drive\seamly2d`) for access while travelling. Installers and apps must treat the location as configurable, not fixed (see the Task 14 installer prompts).

## Per-platform build & packaging

### Windows

- **Current:** CI builds via GitHub workflows (Qt 6.8.3 + MSVC 2022). seamlyLayout has its own Inno Setup installer (`src/app/seamlylayout/packaging/windows/SeamlyLayout.iss`, `build_installer.ps1`) with legacy-settings migration logic.
- **Planned (Task 13):** a Windows **.msi** installer covering all three apps, x64 **and** arm64. Prerequisite: seamlyLayout launchable from seamly2d and passing unit/functional tests. Tooling decision pending (e.g. WiX).
- **Planned (Task 14):** the installer prompts for two paths instead of hard-coding them:
  1. **Executable install path** — default `C:\Program Files (x86)\Seamly2D`, any drive allowed (use case: `D:\Program Files (x86)\Seamly2D`); the chosen directory is added to the system `PATH` automatically and removed on uninstall.
  2. **User data path** — default `C:\Users\<user>\seamly2d`, any drive allowed including cloud-synced (use case: `G:\My Drive\seamly2d`); registered automatically so the apps use it without re-prompting. Open design point: whether a data directory belongs on `PATH` or is better served by an env var / registry / `QSettings` value — decide and document during Task 14.
  - Both paths must survive upgrade-in-place (MSI upgrade codes).
- Code signing: see `.github/workflows/CODE_SIGNING.md` and `.github/workflows/signing/`.

#### Running the unit tests locally (Windows)

- Build the debug tree first (`scripts\sd.ps1`), then run the suite with **`scripts\st.ps1`** ("seamly2d tests"; add `-Release` for the release `build\` tree, and any extra arguments are forwarded to the test exe as QTest options). The script prints a per-suite pass/fail table plus full `FAIL!` details, and exits with the suite's exit code.
- Why a runner script is needed (Task 23 findings, 2026-07):
  - `Seamly2DTests.exe` needs the (debug) Qt DLLs, `xerces-c_3_3.dll`, **and the Qt platform plugin** (`platforms\qwindows[d].dll`). Qt looks for the platform plugin **relative to the executable only** — if it is missing, `QGuiApplication` startup hits a `qFatal` that in a debug-CRT build pops a *hidden modal dialog*, so the suite looks like it hangs at startup with no output. `Seamly2DTest.pro` now post-links `windeployqt` (plus the xerces copy) so everything is deployed beside the test exe; `st.ps1` also sets `QT_PLUGIN_PATH`/`PATH` as a fallback for older build trees.
  - QTest **stdout is lost** when the suite's console output is redirected on Windows, and a single `-o file,txt` logger is overwritten by every suite in turn. `st.ps1` therefore sets `SEAMLY_TEST_LOG_DIR`, which `qttestmainlambda.cpp` honors by writing one text log per suite to `<build>\test-logs\<Suite>.txt`; the script aggregates those.
  - Unit tests must not depend on the **system default printer**: `QPrinter` defaults to the machine's default printer and page size (a 5×7 in photo printer broke `TST_VPoster` locally while CI, with no printers, fell back to PDF/A4). Tests that touch `QPrinter` should force `QPrinter::PdfFormat` and an explicit page size.
- Stale-tree trap: qmake subdir Makefiles in an old `build\` tree do not always regenerate when a `.pro` gains new source files, which surfaces as `LNK2019` unresolved externals for the new classes. Delete `build\src\**\Makefile*` and rebuild so qmake regenerates them.
- CI is unaffected by any of this: the `linux-test` job runs the suite under xvfb on Ubuntu.

### macOS

- Settings unification is Task 16: land in `~/Library/Application Support/Seamly`, migrate legacy `Seamly2D` / `Seamly Systems` Application Support dirs and preferences plists on first run, keep packaged defaults read-only inside the app bundle resources.
- Existing user-facing install doc: `.github/Seamly-MacOS-Installation-v2.pdf`.

### Linux — AppImage

- Built in GitHub CI (`ci.yml`'s `linux` job, `linuxdeploy` + `linuxdeploy-plugin-qt`) — **seamly2d only** today; seamlyLayout is not yet part of that AppImage (Task 20 adds a separate CI job that builds/tests seamlyLayout on Linux/Qt 6.10, but does not package it into the AppImage).
- Settings unification is Task 16 for seamly2d/seamlyme (built here) and, forward-looking for seamlyLayout, Task 17: XDG paths (`~/.config/Seamly/...`), generic first-run migration from the pre-Task-15 org folders — see the settings-storage section above for the full breakdown.
- AppImage mounts are read-only, which naturally enforces "bundled defaults are read-only"; all writes go to the XDG `Seamly` paths. seamlyLayout's exe-relative input/output/log fallbacks additionally detect the read-only mount at runtime via `Platform::isAppImage()` (Task 17) rather than relying on the mount alone, since those particular fallbacks create new directories under `<exeDir>` rather than just reading packaged files.

### Linux — Flatpak (built at Flathub, **not** on GitHub)

- **Where/when:** the Flatpak is built from the Flathub manifest repo, not this repo's CI. Releases reach Flathub via a version bump in that manifest — coordinate timing separately from GitHub releases.
- **Decision (2026-07): do NOT change the Flatpak way of building.** Keep the existing Flathub package structure and single app id.
- **Why one sandbox:** the apps share files and variables and launch each other via `QProcess::startDetached`; cross-sandbox process launches and file handoffs do not work in Flatpak. So all apps ship inside the one existing Flatpak app id, and the unified `Seamly` folder (`~/.var/app/<app-id>/config/Seamly/`) is **one shared physical directory** inside that sandbox — not per-app copies.
- Consequences (Task 18): seamlyLayout must be added to the existing Flathub package if not yet included; in-sandbox launches must resolve to `/app/bin` executables (not host paths), including the `paths/seamlyLayoutApp` setting default; legacy-settings migration must be in-app (no installer exists); packaged defaults are read from the read-only `/app/...` prefix.

## Related records

- `TODO.md` — Tasks 13–18 hold the current actionable subtasks for everything marked "planned" above; completed tasks move to `COMPLETED.md`.
- `PROJECT_PLAN.md` — the approved implementation plan.
- `.github/workflows/README_WORKFLOWS.md` — CI workflow details.
- `src/app/seamlylayout/CHANGELOG.md` — history of seamlyLayout's settings-directory moves (e.g. `<exeDir>/settings/` → AppConfigLocation).
