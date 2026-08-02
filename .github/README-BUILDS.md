# Seamly Builds — Knowledge Base

Pertinent knowledge about the Seamly family builds: why decisions were made, what is built, where things install and store data, and when/where each build runs. Update this file whenever build knowledge changes — it is the durable record behind the task entries in the `TODO_*.md` files / `project-docs/TODO_COMPLETED.md`.

Apps covered:

- **seamly2d** — pattern drafting (parent app), `src/app/seamly2d`, Qt 6 / C++ / qmake
- **seamlyme** — measurements, `src/app/seamlyme`, Qt 6 / C++ / qmake
- **seamlyLayout** — daughter layout app, `src/app/seamlylayout/`, Rust + Qt 6.11/QML, own build (`src/app/seamlylayout/qd.ps1`), deliberately outside the Seamly2D qmake build

## The apps are a family, not standalone programs

This constrains every packaging decision below:

- seamly2d launches seamlyme and seamlyLayout as detached processes (`QProcess::startDetached` in `src/app/seamly2d/mainwindow.cpp`; seamlyLayout via `exportPiecesToSeamlyLayout()`).
- seamly2d hands a tagged `.pieces.svg` file to seamlyLayout (Layout Mode handoff; attribute spec in `project-docs/NEW-ATTRIBUTES.csv`).
- The apps share files and variables: measurement files, settings values (e.g. the `paths/seamlyLayoutApp` executable path stored via `VSettings`, `src/libs/vmisc/vsettings.cpp`).

Therefore all packaging must keep the apps installed together (or mutually locatable) and able to see the same user data. On sandboxed platforms (Flatpak) they must share one sandbox.

## Toolchains (Windows development)

Since **Task 30** every app in the family builds against the **same Qt release, 6.11.1** — CI and the developer PC alike:

| | Where | Qt | Compiler | Notes |
|---|---|---|---|---|
| **CI** | GitHub hosted runner | 6.11.1 | MSVC 2022 | Used by release/CI workflows; `QT_VERSION` in `ci.yml`, `seamlylayout-ci.yml` and `windows-msi.yml` must all name this release |
| **Local** | Developer PC | 6.11.1 `msvc2022_64` | VS 18 Community (`vcvars64.bat`) | qmake + jom for the parents; CMake + Ninja + Cargo for seamlyLayout; release shadow-build in `build/` (gitignored) |

- Local debug build: `scripts/sd.ps1` — auto-detects the newest Qt msvc2022_64 kit (6.11.1 or newer) under `C:\Qt` and the VS 18 MSVC environment, shadow-builds `CONFIG+=debug` into `scripts/seamly2d-debug/` (gitignored); debug exe at `scripts/seamly2d-debug/src/app/seamly2d/bin/seamly2d.exe`, Qt debug DLLs deployed by windeployqt; `-Run` launches after build.
- seamlyLayout builds separately with `src/app/seamlylayout/qd.ps1` and must stay out of the qmake build.
- **Never invoke a Qt tool by bare name on a developer PC (Tasks 47/48).** Qt Design Studio installs a stripped Qt 6.8.x at `C:\Qt\Tools\QtDesignStudio\qt6_design_studio_reduced_version\bin\` and puts it on `PATH`, so a bare `qmake`, `windeployqt` or `windeployqt6` resolves to *that* Qt, not the build kit. The consequences are silent: bare `qmake` fails with a spec error naming the reduced prefix (it ships no `mkspecs/`), and bare `windeployqt` deploys 6.8.x DLLs beside exes linked against 6.11.1 — a tree that cannot start and that `smsi.ps1` would package verbatim. Every call site is now pinned: the `.pro` post-link steps use `qtPrepareTool(WINDEPLOYQT, windeployqt)` (resolves from `$$[QT_INSTALL_BINS]`, i.e. the Qt that qmake itself belongs to), the macOS branches use `$$[QT_INSTALL_BINS]/macdeployqt`, `src/app/seamlylayout/build.ps1` exports `QMAKE` and prepends the kit's `bin\`, and `smsi.ps1` reads the kit out of SeamlyLayout's `CMakeCache.txt`. `sb.ps1` and `sd.ps1` additionally compare the deployed `Qt6Core.dll`/`Qt6Cored.dll` FileVersion against the kit that compiled the exes and fail loudly on a mismatch. CI is unaffected — the runners have no Design Studio.
- **Required Qt modules.** The parents need `qtmultimedia`. seamlyLayout additionally needs **`qtwebengine`** (its `SvgCanvas.qml` uses `WebEngineView`) *and* `qtwebengine`'s own dependencies **`qtwebchannel`** and **`qtpositioning`** — `Qt6WebEngineCoreDependencies.cmake` lists them, and neither the Qt online installer nor `aqtinstall` pulls them in automatically when Qt WebEngine is selected. Without them `find_package(Qt6 ... WebEngineQuick)` fails at configure time with *"Qt6WebEngineQuick could not be found because dependency Qt6WebEngineCore could not be found"*. The CI workflows list all three explicitly in their `modules:` input.

## Continuous integration (CI)

Three independent GitHub Actions workflows, split by build system / purpose (see the `README_WORKFLOWS.md` in `.github/workflows/` for the full descriptions). Since **Task 30** all three pin the **same** Qt:

| Workflow | App(s) | Qt | Runner(s) | What it does |
|---|---|---|---|---|
| `ci.yml` | seamly2d, seamlyme | 6.11.1 | ubuntu / macos-15 / windows-2022 | qmake build + Linux unit tests (xvfb), AppImage / .dmg / NSIS installer (`.exe`/`.zip`) packaging, signing, releases |
| `seamlylayout-ci.yml` (**Task 20**) | seamlyLayout | 6.11.1 | ubuntu-latest | Rust + Qt 6.11 CMake/Ninja build, then `ctest` (Qt frontend suites, under xvfb) + `cargo test --workspace` |
| `windows-msi.yml` (**Task 13**) | seamly2d, seamlyme, seamlyLayout | 6.11.1 (one shared kit) | windows-2022 | builds the parents (qmake) and SeamlyLayout (CMake) in one job, then the bundled WiX **.msi** per arch (x64 = all three, arm64 = parents only), validates, signs, uploads the MSI artifact |

- **Why split workflows:** the Qt pin is no longer the reason — all three now name Qt 6.11.1, and the three `QT_VERSION` values must be kept in step. What remains is the **build system**: seamly2d/seamlyme are qmake, seamlyLayout is CMake/Ninja + Cargo/Corrosion with its own extra toolchain steps (Rust, cargo cache, the `qtwebengine`/`qtwebchannel`/`qtpositioning` module set). Keeping `ci.yml` and `seamlylayout-ci.yml` separate also keeps their triggers path-filtered and their failures independent — a seamlyLayout failure never blocks the seamly2d/seamlyme jobs, and vice versa. `windows-msi.yml` is the one workflow that deliberately runs **both** build systems in a single job (it needs all three apps to bundle them), which is exactly why it is kept out of `ci.yml` — it must not slow down or destabilize every push.
- **`seamlylayout-ci.yml` specifics:** triggered by pushes to `develop`/`run-seamlyLayout` and by pull requests, both path-filtered to `src/app/seamlylayout/**` (+ the workflow file) so it only runs when seamlyLayout changes. Qt 6.11 comes from `jurplel/install-qt-action` with the `qtwebengine qtwebchannel qtpositioning` modules (see the toolchain section above for why all three are required); the CMake build drives Corrosion/cxx-qt-cmake, which compiles the `cxxqt_bridge` Rust crate, so one build step produces both the Rust bridge and the C++/QML app — the CI equivalent of `qd.ps1`/`build.ps1`.
- **`seamlylayout-ci.yml` builds and tests seamlyLayout; it does not package it.** No AppImage/MSI/dmg is produced there — packaging stays with the per-platform installers (Tasks 13–18). On Windows that packaging now exists: `windows-msi.yml` (Task 13) bundles seamlyLayout into the x64 MSI (see the Windows section below).
- **Consolidation, re-evaluated (Task 30):** merging `seamlylayout-ci.yml` into `ci.yml` was originally blocked by the differing Qt pins. That block is gone, but the merge was **deliberately not made** — the two jobs use different build systems and different triggers, so folding them together would rebuild the parent apps on every layout-only change for no benefit. Revisit only if the parent apps also move to CMake.

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

## User data tree — the relocatable data root (Task 34, 2026-07; renamed by Task 53)

Settings (above) are separate from the **user data tree**: patterns, measurements, templates, bodyscans, label templates, images, backups and layouts. Task 34 replaced the nine independent `QDir::homePath() + "/seamly2d/<subdir>"` literals with a single settings-backed **data root**:

| Aspect | Value |
|---|---|
| Setting | `paths/dataRoot`, in the shared common settings file (`%APPDATA%\Seamly\qt6_common.ini` on Windows; the platform equivalents above elsewhere) |
| Built-in default | `<home>/seamlyData` — `QDir::homePath()` resolves it natively per platform |
| Derived subfolders | `<dataRoot>/{measurements/individual, measurements/multisize, templates, bodyscans, label templates, images, backups, patterns, layouts}` — the subfolder names are translated (`tr()`) |
| API | `VCommonSettings::dataRoot()` / `dataSubdirPath()` / `getDataRoot()` / `setDataRoot()` / `ensureDataRootTree()` (`src/libs/vmisc/vcommonsettings.h`); the two layout/pattern paths live in `VSettings` but derive from the same root |
| Any drive or path | The root may be any volume the user can write to — an external disk or a cloud-synced folder such as `G:\My Drive\seamlyData` — so the whole tree relocates without moving files by hand |
| Edited after install | **Preferences → Paths**, first row ("My Seamly Data") in both seamly2d and seamlyme. Because the dialog writes every row back as an absolute override, `VCommonSettings::rebaseOntoDataRoot()` moves the rows that still live inside the old root; a folder deliberately parked outside the root is left alone |

**Why `seamlyData` and not `seamly` (Task 53).** Task 34 first renamed the default to `<home>/seamly`, matching the `Seamly` settings umbrella. That name turned out to be too generic to claim: on the developer's own machine `G:\My Drive\seamly` was already a large unrelated business folder, so pointing a data root there would have scattered the nine app subfolders through it. `seamlyData` says what the folder holds and is unlikely to collide with anything a user already has. The legacy name `<home>/seamly2d` is unchanged and still recognised by first-run resolution.

**First-run resolution** (`VCommonSettings::initializeDataRoot()`, called from each app's `openSettings()`), non-destructive and re-entrant:

1. A root already configured — by an earlier run, the user, or a Windows installer prompt (Task 14) — is honoured untouched.
2. Nothing configured, an existing `<home>/seamly2d` tree and no `<home>/seamlyData` — the legacy tree is **adopted in place** as the root. Adoption, not copying: an upgrading user's data can be many gigabytes and may sit on a cloud-synced drive, so nothing is moved, copied or deleted. The decision itself is `chooseFirstRunDataRoot()`, split out so it can be unit-tested against throwaway directories.
3. Otherwise — a fresh install — the `<home>/seamlyData` default.

Note what resolution still does **not** do: it never moves files. Repointing the root leaves the old tree where it is, which is safe but can look like data loss — the check-and-move flow that fixes it is folded into **Task 14**.

**Legacy-skeleton cleanup (Task 53).** Because `ensureDataRootTree()` stocks whatever root is configured with the nine subfolders, a user who moves off `<home>/seamly2d` is left with an empty tree that looks like data but is not. `VCommonSettings::pruneEmptyLegacyDataRoot(legacyRoot, configuredRoot)` removes it, under two conditions that both matter: the legacy root must **not** be the configured root (Task 34 *adopts* an existing `~/seamly2d`, which makes it the live data tree for an upgrading user), and the tree must hold **no file at any depth**. Only empty directories are then removed, deepest first, with `QDir::rmdir()` — never `removeRecursively()`, so the function cannot delete anything it has not counted. The call site with real home paths lives in `Application2D::openSettings()` and `ApplicationME::openSettings()`, deliberately **not** in `initializeDataRoot()`, because the test harness calls that and a test process must never be the caller that deletes a home-directory path.

**Shared-settings resolution fix (Task 34, completed by Task 53).** The apps build their `VCommonSettings` from an explicit settings *file path*, and `QSettings` records no organization for that constructor. `organizationName()` was therefore empty at the `paths/*` accessors, and QSettings substitutes the literal `"Unknown Organization"` for an empty organization — so those shared values were being read and written under `%APPDATA%\Unknown Organization\qt6_common.ini` instead of `%APPDATA%\Seamly\`. `commonSettingsOrganization()` now falls back to the application-wide organization name, and `mergeStrayCommonSettings()` copies any stranded value forward on first run (only where the correctly located file has none, so a newer setting always wins). Task 53 then **deletes** the stray file and removes its `Unknown Organization` folder — but only after re-reading the destination and confirming every stray key is present there, and only via `QDir::rmdir()`, which fails harmlessly if anything else lives in that folder. If verification fails, the stray survives for the next run to retry. The same defect still affects `VSettings`' own eight accessors (`%APPDATA%\Unknown Organization.ini`) — that is **Task 52**.

> **Testing note.** `QDir::homePath()` **cannot be redirected on Windows** — `QFileSystemEngine::homePath()` asks the OS through `GetUserProfileDirectory()` and only falls back to `USERPROFILE`/`HOME` if that fails. A test that creates or removes `~/seamlyData` or `~/seamly2d` is therefore operating on the real user's data tree, whatever the environment says. `TST_DataRoot` (`src/test/Seamly2DTest/tst_dataroot.cpp`) works exclusively inside a `QTemporaryDir`, and exercises first-run resolution through `chooseFirstRunDataRoot()` and cleanup through `pruneEmptyLegacyDataRoot()`, both of which take their candidate roots as arguments. This is not hypothetical: an earlier draft of the Task 34 tests set `HOME`/`USERPROFILE` to a temporary directory, assumed `homePath()` would follow, and permanently deleted the developer's real `C:\Users\<user>\seamly2d`.

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

**Not yet verified:** Task 16's code changes were made and build-verified on Windows (seamlyLayout is cross-platform Qt/CMake — the `Q_OS_MACOS` branches compile out on other platforms) but have not been exercised on real macOS hardware or the `macos-15` CI runner (no Mac available in this environment). Fresh-install and upgrade-with-legacy-data verification remains an open item — see `project-docs/TODO_MIGRATE.md` Task 16.

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

**Not yet verified — seamlyLayout is not currently packaged into the Linux AppImage at all:** `ci.yml`'s `linux` job builds only seamly2d's AppImage (`dist/seamly2d.desktop`); the Task 20 workflow (`seamlylayout-ci.yml`) *builds and tests* seamlyLayout on Linux/Qt 6.10 but does not add it to the AppImage. The `Platform::isAppImage()` code path above is therefore verified by unit test (`PreferencesModelTests`, which sets the `APPIMAGE` environment variable directly since the check itself is a plain env-var read) and by Windows build/test, but not yet exercised by a real packaged Linux AppImage — mirroring how Task 16's macOS code changes were verified without real macOS hardware.

### Linux — Flatpak (as of Task 18, 2026-07)

A Flatpak sandbox exports `XDG_CONFIG_HOME=~/.var/app/<app-id>/config` and `XDG_DATA_HOME=~/.var/app/<app-id>/data` into the app process, so the same generic `QStandardPaths::AppConfigLocation` resolution used everywhere else (Task 15/16/17) lands under the sandbox automatically — no Flatpak-specific code was needed for the base directory move, matching the macOS and AppImage findings.

| App | Flatpak (in-sandbox XDG) location |
|---|---|
| seamly2d | `~/.var/app/<app-id>/config/Seamly/Seamly2D/qt6_seamly2d.ini` |
| seamlyme | `~/.var/app/<app-id>/config/Seamly/SeamlyMe/qt6_seamlyme.ini` |
| seamly2d + seamlyme shared "common" settings | `~/.var/app/<app-id>/config/Seamly/qt6_common.ini` |
| seamlyLayout | `~/.var/app/<app-id>/config/Seamly/SeamlyLayout/{settings,preferences,input,output}/` |

**One shared physical directory:** because all three apps ship inside the **single existing Flatpak app id** (the apps launch each other via `QProcess::startDetached` and share files/variables, which does not work across sandboxes), the `~/.var/app/<app-id>/config/Seamly/` folder above is **one physical directory shared by all three** — not per-app copies. This is what makes the cross-app settings sharing, the `.pieces.svg` handoff, and shared measurement files work inside the sandbox.

**In-sandbox app launches:** seamly2d resolves seamlyme and seamlyLayout via `QCoreApplication::applicationDirPath()` (`Application2D::seamlyMeFilePath()` / `seamlyLayoutFilePath()`), which inside the sandbox is `/app/bin`, so both resolve to `/app/bin/seamlyme` and `/app/bin/SeamlyLayout` — executables inside the same read-only `/app` prefix, never host paths. The `paths/seamlyLayoutApp` setting default is empty, which falls through to exactly that `/app/bin/SeamlyLayout` lookup, so no host-specific configuration is required in the sandbox.

**First-run migration** reuses the same generic `MigrateSeamlySettingsLocation()` / `migrateLegacyOrganizationTree()` logic as Windows/macOS/AppImage — it reconstructs the legacy path by temporarily swapping `organizationName` to the pre-Task-15 value (`"Seamly2DTeam"`, `"Seamly Systems"`) and re-querying `AppConfigLocation`, which resolves the legacy org folder **inside the sandbox** (`~/.var/app/<app-id>/config/Seamly2DTeam`, etc.). Crucially this runs in-app, not from installer logic — Flatpak has no installer step to hook — so it works on the first launch of a newer Flatpak over an older sandbox.

**Read-only `/app` prefix (Task 18):** a Flatpak mounts the app payload at `/app` read-only, the same problem Task 16 found for a macOS `.app` bundle and Task 17 for an AppImage mount. seamlyLayout's default input/output folders and its debug log directory fall back to `<exeDir>/input`, `<exeDir>/output` when unconfigured — inside the sandbox `<exeDir>` is `/app/bin`, which is read-only, so those `mkpath()` calls would silently fail. `Platform::isFlatpak()` (`src/app/seamlylayout/qt_frontend/src/Platform.h`) detects the sandbox at runtime (the `FLATPAK_ID` environment variable and the bind-mounted `/.flatpak-info` file), and `PreferencesModel::defaultInputFolderUrl()`/`resolvedInputDirectory()`/`resolvedLayoutDirectory()` and `Logger::init()` branch on it — alongside the existing `Platform::isAppImage()` check — to use the writable `AppConfigLocation` root (which Flatpak maps into `~/.var/app/<app-id>/config/Seamly/SeamlyLayout`) instead. Packaged defaults are compiled-in Qt resources (`:/defaults/default_preferences.json`, inherently read-only) or ship inside the read-only `/app` prefix, so "packaged defaults are read-only" is enforced structurally.

**Not yet verified — seamlyLayout is not currently part of the Flathub package, and the build is not done here:** the Flatpak is produced from the Flathub manifest repo (see the packaging section below), not this repo's CI, so end-to-end fresh-install / upgrade-over-legacy-sandbox verification and the seamly2d→seamlyLayout in-sandbox handoff remain open until seamlyLayout is added to that manifest. The `Platform::isFlatpak()` code path is verified by unit test (`PreferencesModelTests` sets `FLATPAK_ID` directly, since the check is a plain env-var read) and by a local Windows build/test pass — the same verification posture Task 16 (macOS) and Task 17 (AppImage) left in place without the real target platform.

## User data files (patterns, measurements)

- Default user data tree on Windows: `C:\Users\<user>\seamlyData` (Task 34 renamed it from `seamly2d`, Task 53 settled on `seamlyData` — a bare `seamly` collides far too easily with a folder a user already has). **An existing `seamly2d` tree is adopted in place on first run**, never moved or copied, so an upgrading user's gigabytes stay where they are — see "User data tree — the relocatable data root" above. The nine standard subfolders are created under whichever root wins, by `ensureDataRootTree()`, called from each app's `openSettings()`.
- Users legitimately relocate it — e.g. to a cloud-synced drive (`G:\My Drive\seamlyData`) for access while travelling. Since Task 34 the location is one setting, `paths/dataRoot`, that every data subfolder derives from; installers and apps must treat it as configurable, not fixed (see the Task 14 installer prompts).

## Per-platform build & packaging

### Windows

- **Current:** CI builds via GitHub workflows (Qt 6.11.1 + MSVC 2022; see `ci.yml`'s `QT_VERSION`). seamlyLayout has its own Inno Setup installer (`src/app/seamlylayout/packaging/windows/SeamlyLayout.iss`, `build_installer.ps1`) with legacy-settings migration logic. The `ci.yml` NSIS installer (`dist/seamly2d-installer.nsi`) remains the **released** Windows installer.
- **MSI installer (Task 13) — shipped:** a single bundled Windows **.msi** per architecture (x64 and arm64) that installs all three apps together, built with the **WiX toolset** from `scripts/packaging/windows/seamly-family.wxs` via `scripts/packaging/windows/smsi.ps1` (local) and `.github/workflows/windows-msi.yml` (CI). The hands-on build/test reference is `scripts/packaging/windows/README.md`. Key design points:
  - **WiX v6, not v7.** WiX v7 (July 2026) refuses to run until an Open Source Maintenance Fee (OSMF) EULA is accepted (error `WIX7015`) — a policy decision the project has not made — so both the script and the workflow pin the `wix` .NET tool to `6.*`. **Two extensions are required and both must match the core tool version:** `WixToolset.UI.wixext` (the wizard) and `WixToolset.Util.wixext` (`RemoveFolderEx`, used to remove the old NSIS installation). `smsi.ps1` fails early naming whichever is missing. Note the Util extension's MSI tables are namespaced to its major version — `Wix4RemoveFolderEx` under WiX 6 — which matters when inspecting a built package.
  - **One bundled MSI per arch, not per-app MSIs** — the three apps are a family and are installed/removed together.
  - **Install layout — one flat directory, one Qt runtime (Task 30).** `\Program Files\SeamlyApps\` holds all three executables (`seamly2d.exe`, `seamlyme.exe`, `SeamlyLayout.exe`) and the **single** Qt runtime they share: the parents' windeployqt output merged with SeamlyLayout's `windeployqt6` output (QML module tree, Qt Quick/WebEngine DLLs, `QtWebEngineProcess.exe`), plus xerces-c, SeamlyLayout's packaged `settings\` and `licenses\`, and the app-local MSVC CRT. seamly2d resolves the executable at runtime via `SeamlyFamilyPaths::locateSeamlyLayout()` (`src/libs/vmisc/seamly_family_paths.cpp`), which checks this flat layout first.
    - **Why it used to be two.** Before Task 30, SeamlyLayout was built against Qt 6.10 while the parents were on 6.11; Qt's DLL file names are identical across releases, so the two runtimes could not share a flat directory and SeamlyLayout was installed into a `\Program Files\Seamly2D\SeamlyLayout\` subdirectory carrying its **own** full Qt copy — which is why the MSI weighed ~187 MB. Unifying on Qt 6.11.1 removed the split. `locateSeamlyLayout()` still falls back to the subdirectory so a seamly2d upgraded in place over such an install keeps working.
  - **MSVC CRT deployed app-locally** (`msvcp140.dll`, `vcruntime140*.dll`, ...) beside the executables — no merge modules, no `vc_redist.exe` chaining. `smsi.ps1` finds the CRT via `VCToolsRedistDir` (falling back to scanning VS installs). With one shared install directory this is now a single copy for all three apps.
  - **Standard installer concerns.** Three advertised Start Menu shortcuts; file associations `.sm2d` → seamly2d and `.smis`/`.smms` → seamlyme (SeamlyLayout has none — its input is the `.pieces.svg` handoff, and a double `.pieces.svg` extension can't be registered distinctly from plain `.svg`); `MajorUpgrade` with `AllowSameVersionUpgrades` so newer versions upgrade in place and uninstall is clean. The **UpgradeCode `cbf4b5f1-c32c-4dbb-b385-3ee4a7b30658` is fixed forever** and shared by both architectures; never change it. The per-build ProductCode is auto-generated.
  - **Version mapping.** MSI caps the ProductVersion major field at 255, so the project's `YYYY.M.D.HHMM` scheme can't be used directly. `smsi.ps1` derives a strictly-increasing numeric version `(YYYY−2000).M.((D−1)·1440 + HH·60 + MM)` (third field = minutes-of-month, max 44639 < 65535) so `MajorUpgrade` always sees newer builds as newer, and stores the real `YYYY.M.D.HHMM` string as `DisplayVersion` in `HKLM\SOFTWARE\Seamly\Seamly2D` (an install breadcrumb also read by the Task 14 prompts).
  - **arm64.** seamly2d/seamlyme cross-compile for arm64 exactly as in `ci.yml`'s windows matrix. **SeamlyLayout has no arm64 build yet** (its Rust + cxx-qt cross story is unresolved, and Qt ships no cross-compiled arm64 WebEngine), so the arm64 MSI ships the two parent apps only — `smsi.ps1 -NoSeamlyLayout`, and the arm64 matrix leg installs only the `qtmultimedia` module. When SeamlyLayout gains an arm64 build, drop that flag and add the WebEngine modules.
  - **User data is never touched, and cannot be.** The installer neither creates nor removes `%LOCALAPPDATA%\Seamly\<app>`, `%APPDATA%\Seamly\qt6_common.ini`, or the user data tree (`C:\Users\<user>\seamlyData` since Task 53, `seamly` in Task 34, `seamly2d` before that) — uninstall and upgrade leave all of them in place, so user data survives. This is not merely a policy: a per-machine MSI's server side runs as **LocalSystem**, so `C:\Users\<name>\...` resolves to the SYSTEM profile at install time and could only ever cover the one user who ran setup. The apps therefore settle the data root per user on first launch — `initializeDataRoot()` resolves it (adopting an existing legacy `~/seamly2d` tree **in place**, never moving gigabytes) and `ensureDataRootTree()` creates the nine standard subfolders under whichever root won. Both are called from each app's `openSettings()`.
  - **The old NSIS installation is removed during install (Task 51).** It is a strict subset of this package — NSIS ships seamly2d and seamlyme, the MSI ships both plus SeamlyLayout — so leaving it behind means two copies of each parent app, two "Seamly2D" entries in Apps & features, and Start Menu shortcuts that launch the old binaries. **Its own `uninstall.exe` is deliberately never run**: it is interactive, its uninstall section is `RMDir /r $INSTDIR` (so it deletes anything else in that folder), and Windows Installer cannot roll an external uninstaller back if the rest of the install then fails. Instead the four things it created are removed directly — its directory tree, its Start Menu folder, `HKLM\SOFTWARE\NSIS_Seamly2D` and `HKLM\...\Uninstall\Seamly2D` — which `RemoveFiles` rolls back on failure. Two components are needed because bitness is per-component: the NSIS keys are under `WOW6432Node` (32-bit installer, never switched view), and the 32-bit component sits under `ProgramFilesFolder` because ICE80 rejects a 32-bit component in a 64-bit directory. Its Start Menu shortcuts are **per-user** — the `.nsi` never calls `SetShellVarContext all` — so another user's stale copy on the same machine cannot be reached by a per-machine install.
  - **Install-time dialogs (Task 51).** A previous-installation warning page (shown from `InstallUISequence`, on either kind of existing install and only when not already installed) and a desktop-shortcut checkbox page. The decisions behind both, and the full per-case behaviour, are in `scripts/packaging/windows/README.md` and `scripts/packaging/windows/INSTALL_DECISION_FLOW.md` — read the latter before changing upgrade or previous-install behaviour.
  - **Signing.** `windows-msi.yml` signs the `.msi` with `jsign` (Google Cloud KMS) exactly as `ci.yml` signs the NSIS exe, guarded on the `SEAMLY_SIGNING_PROJECT_ID` secret so 3rd-party PR runs skip it. Code signing can otherwise be a follow-up.
  - **Automated checks.** Two scripts, and neither replaces the other. `test_msi_authoring.ps1` (**63 assertions**, run by `smsi.ps1` on every build, so CI runs it for both architectures) opens the built MSI with the Windows Installer COM API and asserts what the *package contains*. `test_msi_install.ps1` asserts what a real elevated install *did to a machine*, in four phases (`Baseline`/`Installed`/`Upgraded`/`Removed`) run around the `msiexec` commands; it is standalone, so it can be copied to a test machine beside the `.msi`. Authoring passes on a package whose exes cannot start, which is why the install script launches each app and checks it stays running.
  - **Verification status (2026-07-31).** Local build and validation clean (`wix build`, `wix msi validate` with only the expected **ICE61**, a benign consequence of `AllowSameVersionUpgrades`; authoring 63/63). A real install/upgrade cycle has been run on a Windows 11 laptop: **52 of 57 runtime checks passed**, including all three apps starting and staying running, all three file associations resolving through ShellExecute, the shortcuts, the HKLM rows and the ARP entry. **Not yet exercised:** the NSIS-removal path against a real NSIS installation, the uninstall phase, and any arm64 run (no arm64 hardware in this environment). **Known open defects** are tracked in `project-docs/TODO_MIGRATE.md` under Task 51 — chiefly that the desktop-shortcut page never displays under WiX 6's `CheckTargetPath` publish chain, so the default applies and the choice is never offered.
- **Planned (Task 14):** an install-path prompt, and a check-and-move flow for an existing user-data tree.
  1. **Executable install path** — default `C:\Program Files\SeamlyApps` (the 64-bit tree: every binary in the package is x64 or arm64; only the old *NSIS* product lives under `Program Files (x86)`, because its installer was 32-bit). Any drive allowed; already overridable with `msiexec … INSTALLFOLDER=D:\SeamlyApps`, so what Task 14 adds is the prompt, not the capability. **Nothing is added to the system `PATH`** — an earlier version of this document claimed the NSIS installer did that, which `dist/seamly2d-installer.nsi` does not.
  2. **User data path** — default `C:\Users\<user>\seamlyData`, any drive allowed including cloud-synced (use case: `G:\My Drive\seamlyData`). **This belongs to the apps, not the installer**, for the LocalSystem reason above; it is stored as `paths/dataRoot` in `%APPDATA%\Seamly\qt6_common.ini` and is already settable in Preferences → Paths, which relocates all nine subfolders with it. What Task 14 adds is the first-run "an existing data tree was found — keep it here or move it?" flow. **Open design point:** today's rule deliberately *adopts* an existing legacy tree in place rather than moving it, so a migration option has to be reconciled with that.
  - The install path must survive upgrade-in-place; the data root is independent of the installer entirely.
- Code signing: see `.github/workflows/CODE_SIGNING.md` and `.github/workflows/signing/`.

#### Running the unit tests locally (Windows)

- Build the debug tree first (`scripts\sd.ps1`), then run the suite with **`scripts\st.ps1`** ("seamly2d tests"; add `-Release` for the release `build\` tree, and any extra arguments are forwarded to the test exe as QTest options). The script prints a per-suite pass/fail table plus full `FAIL!` details, and exits with the suite's exit code.
- Why a runner script is needed (Task 23 findings, 2026-07):
  - `Seamly2DTests.exe` needs the (debug) Qt DLLs, `xerces-c_3_3.dll`, **and the Qt platform plugin** (`platforms\qwindows[d].dll`). Qt looks for the platform plugin **relative to the executable only** — if it is missing, `QGuiApplication` startup hits a `qFatal` that in a debug-CRT build pops a *hidden modal dialog*, so the suite looks like it hangs at startup with no output. `Seamly2DTest.pro` now post-links `windeployqt` (plus the xerces copy) so everything is deployed beside the test exe; `st.ps1` also sets `QT_PLUGIN_PATH`/`PATH` as a fallback for older build trees.
  - QTest **stdout is lost** when the suite's console output is redirected on Windows, and a single `-o file,txt` logger is overwritten by every suite in turn. `st.ps1` therefore sets `SEAMLY_TEST_LOG_DIR`, which `qttestmainlambda.cpp` honors by writing one text log per suite to `<build>\test-logs\<Suite>.txt`; the script aggregates those.
  - Unit tests must not depend on the **system default printer**: `QPrinter` defaults to the machine's default printer and page size (a 5×7 in photo printer broke `TST_VPoster` locally while CI, with no printers, fell back to PDF/A4). Tests that touch `QPrinter` should force `QPrinter::PdfFormat` and an explicit page size.
- Stale-tree trap: qmake subdir Makefiles in an old `build\` tree do not always regenerate when a `.pro` gains new source files, which surfaces as `LNK2019` unresolved externals for the new classes. Delete `build\src\**\Makefile*` and rebuild so qmake regenerates them.
- CI is unaffected by any of this: the `linux-test` job runs the suite under xvfb on Ubuntu.

#### Where each app's tests live (Task 58)

All four test directories sit under `src/test/`, but they are **not** all built the same way:

| Directory | Covers | Build system | Run with |
| --- | --- | --- | --- |
| `src/test/Seamly2DTest` | seamly2d + seamlyme | qmake (`src/test/test.pro`) | `make check` / `scripts\st.ps1` |
| `src/test/ParserTest`, `TranslationsTest`, `CollectionTest` | shared libs, translations, collection | qmake (`src/test/test.pro`) | `make check` |
| `src/test/SeamlyLayoutTest` | seamlyLayout Qt/C++ frontend (4 suites) | **CMake** (`src/app/seamlylayout/qt_frontend/CMakeLists.txt`) | `ctest --preset debug` |

Task 58 moved the seamlyLayout Qt/C++ suites out of `src/app/seamlylayout/qt_frontend/tests/` so every app's tests live in one place. **`src/test/test.pro` deliberately does not list `SeamlyLayoutTest`** — seamlyLayout is CMake + Cargo and is kept out of the Seamly2D qmake build; all `SUBDIRS` in `Seamly.pro`, `src/src.pro` and `src/test/test.pro` are explicit, with no globbing, so the directory cannot be pulled in by accident. `qt_frontend/CMakeLists.txt` reaches the sources through a single normalized `SEAMLYLAYOUT_TEST_DIR` variable that hard-fails at configure time if the directory ever moves again.

seamlyLayout's **Rust** tests did not move and should not: `#[cfg(test)]` modules compile as part of their crate and reach its private items, and Cargo requires integration tests to sit beside the crate's `Cargo.toml`. They stay in `src/app/seamlylayout/crates/*/src/`, run by `cargo test --workspace`.

CI note: `seamlylayout-ci.yml`'s `push`/`pull_request` path filters were extended with `src/test/SeamlyLayoutTest/**`; without that entry a test-only change would trigger no workflow at all. `ci.yml` has no path filters, so the parent-app jobs are unaffected either way.

### macOS

- Settings unification is Task 16: land in `~/Library/Application Support/Seamly`, migrate legacy `Seamly2D` / `Seamly Systems` Application Support dirs and preferences plists on first run, keep packaged defaults read-only inside the app bundle resources.
- Existing user-facing install doc: `.github/Seamly-MacOS-Installation-v2.pdf`.

### Linux — AppImage

- Built in GitHub CI (`ci.yml`'s `linux` job, `linuxdeploy` + `linuxdeploy-plugin-qt`) — **seamly2d only** today; seamlyLayout is not yet part of that AppImage (the Task 20 workflow `seamlylayout-ci.yml` builds/tests seamlyLayout on Linux/Qt 6.11, but does not package it into the AppImage).
- Settings unification is Task 16 for seamly2d/seamlyme (built here) and, forward-looking for seamlyLayout, Task 17: XDG paths (`~/.config/Seamly/...`), generic first-run migration from the pre-Task-15 org folders — see the settings-storage section above for the full breakdown.
- AppImage mounts are read-only, which naturally enforces "bundled defaults are read-only"; all writes go to the XDG `Seamly` paths. seamlyLayout's exe-relative input/output/log fallbacks additionally detect the read-only mount at runtime via `Platform::isAppImage()` (Task 17) rather than relying on the mount alone, since those particular fallbacks create new directories under `<exeDir>` rather than just reading packaged files.

### Linux — Flatpak (built at Flathub, **not** on GitHub)

- **Where/when:** the Flatpak is built from the Flathub manifest repo, not this repo's CI. Releases reach Flathub via a version bump in that manifest — coordinate timing separately from GitHub releases.
- **Decision (2026-07): do NOT change the Flatpak way of building.** Keep the existing Flathub package structure and single app id.
- **Why one sandbox:** the apps share files and variables and launch each other via `QProcess::startDetached`; cross-sandbox process launches and file handoffs do not work in Flatpak. So all apps ship inside the one existing Flatpak app id, and the unified `Seamly` folder (`~/.var/app/<app-id>/config/Seamly/`) is **one shared physical directory** inside that sandbox — not per-app copies.
- Consequences (Task 18) — the app-source side is now done (see the "Linux — Flatpak" settings-storage section above for the full breakdown):
  - The unified `Seamly` org folder lands under the sandbox's `~/.var/app/<app-id>/config/Seamly/` automatically via `QStandardPaths` — no Flatpak-specific code for the move.
  - In-sandbox launches resolve to `/app/bin/seamlyme` and `/app/bin/SeamlyLayout` via `applicationDirPath()`, never host paths; the empty `paths/seamlyLayoutApp` default falls through to that same `/app/bin/SeamlyLayout` lookup.
  - Legacy-settings migration runs in-app (the generic `MigrateSeamlySettingsLocation()` / `migrateLegacyOrganizationTree()` logic), so it works with no installer step.
  - seamlyLayout's read-only-`/app` writable-path fallbacks now detect the sandbox at runtime via `Platform::isFlatpak()` (alongside `Platform::isAppImage()`), writing to the sandbox `Seamly` paths instead of `/app/bin`; packaged defaults are read-only Qt resources / `/app` files.
  - **Remaining, in the Flathub manifest repo (not this repo):** add seamlyLayout to the existing single-app-id package so it ships in the same sandbox for the handoff, fix any stale references to the old dir names, and bump to the new source release. No build restructuring.

## Related records

- `project-docs/TODO_MIGRATE.md` — Tasks 13–18 hold the current actionable subtasks for everything marked "planned" above; completed tasks move to `project-docs/TODO_COMPLETED.md`.
- `project-docs/PROJECT_PLAN.md` — the approved implementation plan.
- `.github/workflows/README_WORKFLOWS.md` — CI workflow details.
- `src/app/seamlylayout/CHANGELOG.md` — history of seamlyLayout's settings-directory moves (e.g. `<exeDir>/settings/` → AppConfigLocation).
