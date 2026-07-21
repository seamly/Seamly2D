# TODO — Tagged SVG Handoff to SeamlyLayout

See `PROJECT_PLAN.md` for full details. Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `COMPLETED.md`.

## Task 13 — Windows .msi installer for seamly2d, seamlyme, and seamlylayout (x64 + arm64)

Build a Windows MSI installer that installs all three apps — `seamly2d`, `seamlyme`, and `seamlylayout`— with separate packages (or a multi-arch pipeline) for x64 and arm64. Reference the new `AppData\Local\Seamly\<app>` locations.

**Prerequisite:** do not start until seamlyLayout can be launched from seamly2d and passes unit and functional testing.

- [ ] Choose and set up the MSI tooling (e.g. WiX Toolset) and decide packaging layout: one bundled installer vs. per-app MSIs, per-arch builds for x64 and arm64
- [ ] Author the installer definition: install `seamly2d.exe`, `seamlyme.exe`, and the seamlyLayout app plus their Qt runtime DLLs (windeployqt output) and any Rust-side runtime files for seamlyLayout
- [ ] Handle standard installer concerns: Start Menu shortcuts, file associations (`.sm2d`, measurement files), upgrade codes so newer versions upgrade in place, and clean uninstall
- [ ] Produce the arm64 variant: arm64 builds of the apps/Qt runtime (or document a cross-compile/CI story) and an arm64-flagged MSI
- [ ] Wire the MSI build into CI (GitHub runner, Qt 6.8.3 + MSVC 2022) and/or a local script following the `scripts/sd.ps1` pattern
- [ ] Verify: install and uninstall on a clean Windows x64 machine (and arm64 if hardware/VM available); all three apps launch, shortcuts and associations work, upgrade-over-install works
- [ ] Document the build/signing steps (code signing can be a follow-up if no certificate yet) in the repo docs

## Task 14 — Windows installer: prompt for executable and user-data install paths

In the updated Windows installation process (Task 13 installer), prompt the user for two locations instead of hard-coding the defaults:

1. **Executable install path** — where the Seamly executables go. Default `C:\Program Files (x86)\Seamly2D`; must support any drive/filepath (use case: install to `D:\Program Files (x86)\Seamly2D`) and add the chosen executable directory to the system `PATH` automatically.
2. **User data path** — root of the Seamly user data file tree. Default `C:\Users\<user>\seamly2d`; must support any drive/filepath, including cloud-synced drives (use case: `G:\My Drive\seamly2d` so patterns/measurements are accessible while travelling), and register the chosen user data path automatically (add to the system `PATH` per the request; evaluate whether an env var / registry setting / app config is the more appropriate mechanism for a data directory and document the decision).

- [ ] Add an installer UI page (or command-line properties for silent installs) prompting for the executable install directory, prefilled with the `C:\Program Files (x86)\Seamly2D` default, accepting any drive and filepath
- [ ] Add a second installer prompt for the user data root, prefilled with `C:\Users\<user>\seamly2d`, accepting any drive and filepath (including cloud-synced locations like `G:\My Drive\seamly2d`)
- [ ] On install, append the chosen executable directory to the system `PATH` (machine-wide, with proper broadcast so new shells pick it up); remove it on uninstall
- [ ] On install, register the chosen user data path automatically — add to the system `PATH` as requested, and/or persist it where the apps read it (registry/`QSettings`) so seamly2d/seamlyme/seamlyLayout use it as their default data location; remove/clean up on uninstall
- [ ] Make the apps honor the configured user data path on first run (no re-prompting, no fallback to the hard-coded `C:\Users\<user>\seamly2d`)
- [ ] Handle upgrades: preserve both previously chosen paths when a newer MSI upgrades in place
- [ ] Verify use case #1: fresh install to `D:\Program Files (x86)\Seamly2D`, exes run from there, directory present on system `PATH`, uninstall cleans it up
- [ ] Verify use case #2: user data root set to `G:\My Drive\seamly2d`, apps read/write pattern and measurement data there, path registered automatically, uninstall/upgrade behave correctly
- [ ] Document both prompts and the silent-install property equivalents in the installer docs89

## Task 16 — Unify settings directories: macOS build

Apply the Task 15 consolidation to the macOS build, where the organization name maps to `~/Library/Application Support/<org>` and `~/Library/Preferences` plist domains instead of `AppData\Local`.

**Note (2026-07-20):** all three apps use `QSettings::IniFormat` with explicit `QStandardPaths::AppConfigLocation`-derived file paths — none use `QSettings::NativeFormat`/CFPreferences plists — so there is no plist-domain migration to do; "Preferences plists" in the task title above doesn't apply in practice. `QStandardPaths` resolves the org-name change generically per platform, so no macOS-specific code was needed for the directory move itself.

- [x] Confirm the org-name change from Task 15 lands the apps in `~/Library/Application Support/Seamly/<app>`; adjust any macOS-specific bundle identifiers / `Info.plist` values that feed the settings domain — confirmed generic (`QStandardPaths::AppConfigLocation` keys off `organizationName`/`applicationName`, not `CFBundleIdentifier`); seamly2d/seamlyme's existing `org.seamly2dproject.@EXECUTABLE@` identifiers are unrelated and left as-is; seamlyLayout had **no** bundle identifier at all (CMake default placeholder) — added `MACOSX_BUNDLE_GUI_IDENTIFIER "io.seamly.SeamlyLayout"` plus bundle version properties in `qt_frontend/CMakeLists.txt` for a well-formed, signable bundle
- [x] Migrate existing user data from the legacy `Seamly2D` and `Seamly Systems` locations on first run — already implemented generically in Task 15 (`VAbstractApplication::MigrateSeamlySettingsLocation()`, seamlyLayout's `appConfigRootPath()`/`migrateLegacyOrganizationTree()`); all three resolve the legacy path by temporarily swapping `organizationName` and re-querying `QStandardPaths`, with no Windows-specific literals, so it applies to macOS unchanged (code-reviewed, not yet exercised on real macOS — see the unchecked verify item below)
- [x] Remove any exe-relative (app-bundle-relative) writable settings usage in seamlyLayout on macOS; keep packaged defaults read-only inside the bundle resources — `PreferencesModel::defaultInputFolderUrl()`/`resolvedInputDirectory()`/`resolvedLayoutDirectory()` and `Logger::init()` branch on `Q_OS_MACOS` to use the writable `AppConfigLocation` root instead of `<exeDir>/input`, `<exeDir>/output` (a signed/notarized bundle's `Contents/MacOS/` is read-only); Windows/Linux behavior unchanged; `Contents/Resources/settings/` packaged defaults remain read-only, used only as a legacy-migration source
- [x] Update the macOS packaging/CI (dmg/bundle steps) for any path references — `packaging/macos/build_dmg.sh` header comment corrected (was describing a stale `~/seamlyLayout/settings/` scheme; now documents the actual `~/Library/Application Support/Seamly/SeamlyLayout/{settings,preferences,input,output}/` layout and both migration paths); `.github/README-BUILDS.md` gained a full macOS settings-storage section
- [ ] Verify: fresh install and upgrade-with-legacy-data on macOS; both apps retain preferences after migration — **not verified**, no macOS hardware available in this environment; code changes are cross-platform Qt/CMake, build-verified on Windows (seamlyLayout debug build + all 4 Qt frontend ctest suites + full `cargo test --workspace`, all passing, 2026-07-20), and the `Q_OS_MACOS` branches compile out on other platforms, but real macOS runtime behavior (including the `macos-15` CI runner, which currently only builds seamly2d/seamlyme, not seamlyLayout) remains unexercised
- [ ] Verify the macOS build succeeds in GitHub Actions on origin, branch `run-seamlyLayout` (`.github/workflows/ci.yml`'s `macos` job) — note: that job currently builds only seamly2d/seamlyme via qmake (`Seamly2D.pro`), not seamlyLayout, so a green run confirms the org-name change doesn't break the parent-app macOS build but does not exercise seamlyLayout's own `Q_OS_MACOS` settings-path code (see Task 20 for adding seamlyLayout to CI)

## Task 17 — Unify settings directories: Linux AppImage build

Apply the Task 15 consolidation to the Linux AppImage build, where settings follow the XDG paths (`~/.config/<org>`, `~/.local/share/<org>`).

**Note (2026-07-21):** as with Task 16, `QStandardPaths::AppConfigLocation` resolves the shared `"Seamly"` org folder generically per platform (`$XDG_CONFIG_HOME/<org>/<app>` on Linux), so the base directory move and the generic `MigrateSeamlySettingsLocation()`/`migrateLegacyOrganizationTree()` migration needed no Linux-specific code — same finding as macOS. The real Linux-specific work: a mounted AppImage is read-only, the same problem Task 16 found for a signed macOS `.app` bundle, but on Linux this can't be told apart from a normal install at compile time — added `Platform::isAppImage()` (checks the `APPIMAGE` env var the AppImage runtime sets) and used it at runtime in the same three `PreferencesModel` fallback functions and `Logger::init()` that Task 16 branched on `Q_OS_MACOS`.

- [x] Confirm the org-name change lands the apps in `~/.config/Seamly/` and `~/.local/share/Seamly/`; check any AppImage-specific overrides (`APPDIR`-relative paths, `portable`-mode config) — confirmed generic (same `QStandardPaths::AppConfigLocation` mechanism as Windows/macOS); confirmed the AppImage runtime does not override `$HOME`/`$XDG_CONFIG_HOME`/`$XDG_DATA_HOME`, and no code in the tree reads `$APPDIR` or implements a "portable mode", so nothing interferes with the unified resolution
- [x] Migrate existing user data from `~/.config/Seamly2D`, `~/.local/share/Seamly2D`, and the `Seamly Systems` equivalents on first run — already implemented generically in Task 15 (`VAbstractApplication::MigrateSeamlySettingsLocation()` for seamly2d/seamlyme, bridging from the real legacy org folder name `"Seamly2DTeam"`; seamlyLayout's `appConfigRootPath()`/`migrateLegacyOrganizationTree()` bridging from `"Seamly Systems"`); both reconstruct the legacy path via `QStandardPaths` with no platform-specific literals, so this applies to Linux XDG paths unchanged
- [x] Ensure seamlyLayout inside the AppImage treats its bundled `settings/` defaults as read-only (AppImage mounts are read-only anyway) and writes only to the XDG `Seamly` paths — packaged defaults are compiled-in Qt resources (`:/defaults/default_preferences.json`, inherently read-only on every platform) or would ship inside the read-only squashfs mount if/when seamlyLayout is added to the AppImage; the writable-path gap was `PreferencesModel::defaultInputFolderUrl()`/`resolvedInputDirectory()`/`resolvedLayoutDirectory()` and `Logger::init()` falling back to `<exeDir>/input`, `<exeDir>/output`, which can't be created inside a read-only AppImage mount — added `Platform::isAppImage()` (`src/app/seamlylayout/qt_frontend/src/Platform.h`) and branch on it at runtime in all four functions to use the writable `AppConfigLocation` root instead, matching Task 16's macOS fix; a normal (non-AppImage) Linux install and Windows are unaffected
- [x] Update the AppImage build/CI scripts for any path references — `ci.yml`'s `linux` job builds only seamly2d's AppImage today (seamlyLayout is not yet part of it; Task 20 adds Linux CI build/test coverage for seamlyLayout but not AppImage packaging), and that job has no settings-path references to update; `.github/README-BUILDS.md` gained a full Linux/AppImage settings-storage section plus updates to the existing "Linux — AppImage" packaging section
- [ ] Verify: run the AppImage fresh and over legacy data; preferences persist across runs and migrate correctly — **not verified end-to-end**: seamlyLayout is not currently packaged into any Linux AppImage (see above), and this Windows dev environment cannot build/run a Linux AppImage directly. The `Platform::isAppImage()` code path is unit-tested (`PreferencesModelTests`, which sets the `APPIMAGE` environment variable directly, since the check is a plain env-var read with no OS-specific API); real end-to-end AppImage verification remains open, same caveat as Task 16's macOS verification
- [ ] Verify the Linux AppImage build succeeds in GitHub Actions on origin, branch `run-seamlyLayout` (`.github/workflows/ci.yml`'s `linux` job) — note: that job currently builds only seamly2d's AppImage (`linuxdeploy` + `dist/seamly2d.desktop`), not seamlyLayout, so a green run confirms the org-name change doesn't break the parent-app Linux AppImage build but does not exercise seamlyLayout's own `Platform::isAppImage()` code path (see Task 20 for adding seamlyLayout to Linux CI)

## Task 18 — Unify settings directories: Linux Flatpak build (built at Flathub, not on GitHub)

Apply the Task 15 consolidation to the Flatpak build. Flatpak sandboxes per-app data under `~/.var/app/<flatpak-app-id>/`, and the build is produced from the Flathub manifest repo rather than this repo's CI.

**Decision:** do NOT change the Flatpak way of building — keep the existing Flathub package structure and app id. The apps share files and variables and launch each other via `QProcess::startDetached` (seamly2d → seamlyme in `src/app/seamly2d/mainwindow.cpp`, seamly2d → seamlyLayout in `exportPiecesToSeamlyLayout()`, plus the `.pieces.svg` handoff and shared measurement files), so they must live in the **same sandbox**: all apps ship inside the one existing Flatpak app id, and the unified `Seamly` folder (`~/.var/app/<app-id>/config/Seamly/`) is one shared physical directory inside that sandbox. The folder-name change itself flows entirely from the Task 15 org-name change in the app source.

**Note (2026-07-21):** as with Task 16 (macOS) and Task 17 (AppImage), `QStandardPaths::AppConfigLocation` resolves the shared `"Seamly"` org folder generically — a Flatpak exports `XDG_CONFIG_HOME=~/.var/app/<app-id>/config` (and `XDG_DATA_HOME=.../data`) into the app process, so the base directory move and the generic `MigrateSeamlySettingsLocation()`/`migrateLegacyOrganizationTree()` migration land under the sandbox automatically with no Flatpak-specific code. The real Flatpak-specific work mirrored Task 17: the app payload is mounted at the `/app` prefix **read-only** (the exe is at `/app/bin/...`), the same problem Task 16 found for a signed macOS `.app` bundle and Task 17 for an AppImage mount, but — like the AppImage case — it can't be told apart from a native install at compile time. Added `Platform::isFlatpak()` (`src/app/seamlylayout/qt_frontend/src/Platform.h`; checks the `FLATPAK_ID` env var and the bind-mounted `/.flatpak-info` file) and branched on it at runtime — alongside the existing `Platform::isAppImage()` check — in the same three `PreferencesModel` fallback functions and `Logger::init()` that Tasks 16/17 touched, so seamlyLayout's default input/output/log directories fall back to the writable `AppConfigLocation` root (which Flatpak maps into `~/.var/app/<app-id>/config/Seamly/SeamlyLayout`) instead of the read-only `/app/bin/input`, `/app/bin/output`.

- [x] Confirm the org-name change lands all apps' settings under `~/.var/app/<app-id>/config/Seamly/` (and `data/Seamly/`) inside the single shared sandbox, and that cross-app sharing (settings variables, `.pieces.svg` handoff paths, measurement files) works there — confirmed generic: the sandbox's `XDG_CONFIG_HOME`/`XDG_DATA_HOME` make `AppConfigLocation` resolve under `~/.var/app/<app-id>/config/Seamly/`; because all three apps live in one app id, that `Seamly` folder is one physical shared directory, so shared settings, the `.pieces.svg` handoff, and measurement files all resolve to the same in-sandbox paths (full breakdown in `.github/README-BUILDS.md` "Linux — Flatpak" settings-storage section)
- [x] Confirm the in-sandbox app launches keep working: seamly2d → seamlyme and seamly2d → seamlyLayout via `QProcess::startDetached` resolve to executables inside the same Flatpak prefix (`/app/bin`), not host paths; the `paths/seamlyLayoutApp` setting default must work inside the sandbox — confirmed: `Application2D::seamlyMeFilePath()`/`seamlyLayoutFilePath()` resolve via `QCoreApplication::applicationDirPath()` (= `/app/bin` in-sandbox), yielding `/app/bin/seamlyme` and `/app/bin/SeamlyLayout`; the empty `paths/seamlyLayoutApp` default falls through to exactly that `/app/bin/SeamlyLayout` lookup, so no host path or per-sandbox config is involved
- [x] Migrate legacy `Seamly2D` / `Seamly Systems` config dirs inside the sandbox on first run (same in-app migration as Task 15 — it must not rely on installer logic, since Flatpak has no installer) — confirmed: the generic `MigrateSeamlySettingsLocation()` (seamly2d/seamlyme) and `appConfigRootPath()`/`migrateLegacyOrganizationTree()` (seamlyLayout) reconstruct the legacy org path via `QStandardPaths`, which resolves the legacy folder **inside the sandbox** (`~/.var/app/<app-id>/config/Seamly2DTeam`, `.../Seamly Systems`); it runs in-app on first launch, so it needs no installer step
- [x] Ensure seamlyLayout's packaged defaults are read from the Flatpak app prefix (`/app/...`) read-only, with all writable settings in the sandbox `Seamly` paths — done: added `Platform::isFlatpak()` and branched `PreferencesModel::defaultInputFolderUrl()`/`resolvedInputDirectory()`/`resolvedLayoutDirectory()` and `Logger::init()` on it so the read-only-`/app` exe-relative fallbacks use the writable `AppConfigLocation` root instead; packaged defaults are compiled-in Qt resources (`:/defaults/default_preferences.json`) or read from the read-only `/app` prefix. Unit-tested (5 new `PreferencesModelTests` cases setting `FLATPAK_ID`), local Windows build/test pass (43 → 48 PreferencesModelTests cases, all passing)
- [ ] Flathub manifest: no build restructuring — add seamlyLayout to the existing package if not yet included (it must ship in the same sandbox for the handoff to work), fix any stale references to the old dir names, and otherwise a routine version bump to the new source release — **out of this repo's scope**: the Flathub manifest lives in a separate manifest repo, not here (no Flatpak manifest exists in this tree). The required manifest changes are documented in `.github/README-BUILDS.md` ("Linux — Flatpak" packaging section) so they can be applied there at release time
- [ ] Verify: install the Flatpak fresh and over an existing sandbox with legacy data; preferences migrate and persist; the seamly2d → seamlyLayout handoff works end-to-end inside the sandbox — **not verified end-to-end**: seamlyLayout is not yet part of the Flathub package, and this Windows dev environment cannot build/run a Flatpak. The `Platform::isFlatpak()` code path is unit-tested (the check is a plain env-var/file read) and Windows build/test-verified; real end-to-end Flatpak verification remains open, the same caveat as Task 16 (macOS) and Task 17 (AppImage)

## Task 20 — GitHub workflow to build seamlylayout on push (Qt 6.10)

Add a push-triggered CI workflow that builds seamlylayout, following the pattern used for seamly2d/seamlyme in `.github/workflows/ci.yml`. The existing CI pins Qt 6.8.3 (`QT_VERSION` in `ci.yml`), but seamlylayout requires Qt 6.10, so for now it needs its own job/workflow on a separate `ubuntu-latest` runner installing Qt 6.10 via `jurplel/install-qt-action`. In the future seamly2d and seamlyme will move to Qt 6.10 as well, at which point the workflows can share one toolchain.

- [ ] Create `.github/workflows/` workflow (or a job in `ci.yml`) triggered on push touching the seamlylayout tree, on `ubuntu-latest`
- [ ] Install the toolchain: Qt 6.10.x via `jurplel/install-qt-action` (with the QML/Quick modules seamlylayout needs) plus a stable Rust toolchain (`rustup`/`dtolnay/rust-toolchain`), with cargo and Qt caching
- [ ] Build the Rust backend and the Qt 6.10 QML frontend from the seamlylayout directory (mirror what `qd.ps1` does, adapted to Linux/CI)
- [ ] Run the seamlylayout unit tests (Rust tests and the Qt frontend tests, e.g. `PreferencesModelTests`) in the workflow
- [ ] Keep the job independent of the Qt 6.8.3 seamly2d/seamlyme jobs so a seamlylayout failure doesn't block them (and vice versa); use path filters so it only runs when seamlylayout files change
- [ ] Note the future consolidation in the workflow comments: when seamly2d/seamlyme move to Qt 6.10, merge this into the shared CI toolchain
- [ ] Document the new workflow in `.github/README-BUILDS.md` and `.github/workflows/README_WORKFLOWS.md`

## Task 21 — SeamlyLayout: three text modes for SVG export in the Exports menu

Replace the single "SVG" item in the SeamlyLayout Exports menu (`src/app/seamlylayout/qt_frontend/qml/ExportMenu.qml`, `exportSvgRequested()` wired through `Main.qml` to the Rust backend in `src/app/seamlylayout/crates/cxxqt_bridge/src/exports.rs`) with three SVG export modes differing in how label text is written.

1. **Text as `<text>` in the designer's selected (outline) font** — searchable, editable, re-stylable, human- and machine-readable; embeds the font via `@font-face` so it renders correctly on machines without it; supports tech-pack generation. Smallest file size.
2. **Text as `<text>` in a Hershey/single-line font** — same searchable/editable/machine-readable intent as mode 1, but using a bundled single-line font so the result is also friendly to CAD/CAM tools that resolve text. **Implementation note:** true Hershey fonts are stroke data, not installable outline fonts — for `<text>` + `font-family` to work this mode needs a "hairline" single-line TTF/OTF (an engineered font whose outline doubles back on itself to look like one stroke) embedded via `@font-face`. Known candidates: CamBam Stick Fonts (free, 9 variants, designed for CNC/plotting), MecSoft/Rhino single-stroke fonts, commercial single-line TTF bundles — verify redistribution/embedding license compatibility with the MIT Rust core before bundling. Caveats: hairline TTFs are still doubled-back outlines (stroke width not controllable via the font), and consumers that ignore embedded fonts will substitute — so mode 3 remains the guaranteed-fidelity choice for cutters. Record the font choice and rationale in `DECISIONS.md`.
3. **Text converted to paths (single-stroke)** — each label rendered as single-stroke `<path>` polylines from Hershey glyph data (**decision:** the path conversion uses the Hershey font, not the designer's outline font — a plotter then draws each character in one pen pass instead of tracing hollow glyph contours). Compatible with CAD/CAM/cutters/plotters/engravers with no font dependency in the consumer. Keep the original string machine-readable via `data-*`/`<desc>` on the label group (text is no longer searchable/editable as SVG text).

**Dependency:** all three modes need real `<text>` elements in the incoming `.pieces.svg` (Task 10) — even mode 3 needs the label *strings* to re-render them in stroke glyphs. Already-outlined input (Seamly2D `--text2paths`) can only be passed through as-is; the UI must handle path-only input (disable the text modes with an explanatory tooltip, or export the existing paths with a warning). Optional Hershey display/export on the Seamly2D side is Task 22.

- [ ] Replace the single "SVG" `MenuItem` with a three-entry submenu (or dialog choice) in `ExportMenu.qml`; add per-mode tooltips summarizing the compatibility/editability trade-off; wire new signals through `TopMenuBar.qml`/`Main.qml` to the bridge
- [ ] Mode 1: pass `<text>` through in the designer's font and embed the font as a subsetted `@font-face` data-URI (WOFF/TTF); document the font-licensing caveat (embedding rights vary by font license) in the export docs
- [ ] Mode 2: emit `<text>` styled with the bundled single-line font, embedded via `@font-face`, per the `DECISIONS.md` decision
- [ ] Mode 3: implement single-stroke text rendering in the Rust core — shape each label string into Hershey glyph strokes and emit stroked (fill-less) `<path>` polylines via `svg_dom`; keep the original label string on the group via `data-*`/`<desc>`
- [ ] Bundle Hershey/single-line glyph data under a permissive license compatible with the MIT Rust core (evaluate existing crates, e.g. a Hershey-font crate, before hand-rolling)
- [ ] Preserve the `data-*` tagging contract (`piece_label`/`pattern_label` groups, ids, `data-parent`) identically in all three modes
- [ ] Detect path-only input (no `<text>` in labels) and gate all three text modes accordingly
- [ ] Persist the last chosen SVG text mode in preferences (`PreferencesModel`)
- [ ] Tests: Rust unit tests for each conversion mode (mode 3 output is stroked polylines, not filled contours), plus the path-only-input case; frontend test for menu gating; end-to-end check with the richmond test pattern
- [ ] Update `src/app/seamlylayout/docs/status-docs/svg-data-attributes.md`, the root `status-docs/svg-data-attributes.md` mirror, and `src/app/seamlylayout/docs` export docs
- [ ] Doxygen briefs + inline comments on all touched functions

## Task 22 — Seamly2D: optional single-stroke (Hershey) label display and export

**Scope decision (2026-07-18):** outline fonts remain fully acceptable in Seamly2D — the existing outline-font canvas display and exports stay the default and are NOT removed or locked out. This task adds single-stroke (Hershey-style) label support as an *option*, so a designer targeting a cutter/plotter can preview labels on the canvas the way the machine will draw them and export matching single-stroke output.

Current label text handling (all outline-font based, and staying available): the canvas paints labels with the user-selected `QFont` via `painter->drawText()` (`VTextGraphicsItem::paint()`, `src/libs/vwidgets/vtextgraphicsitem.cpp`); the `textAsPaths == true` branch of `VLayoutPiece::createLabelItem()` (`src/libs/vlayout/vlayoutpiece.cpp`) outlines glyphs via `QPainterPath::addText()`; DXF text goes through `VDxfEngine::drawTextItem()` (`src/libs/vdxf/vdxfengine.cpp`).

**Constraint:** neither Windows nor Qt has native stroke-font support — the OS/Qt typography stack (`QFont`) only loads outline TTF/OTF, and a true single-stroke font cannot be installed as a system font (Windows errors on it or auto-closes the open contours into filled shapes). Two known workarounds exist in the wild:

1. **App-internal stroke-glyph data** (the true single-stroke route) — bypass the system font stack entirely and render strokes from Hershey glyph data inside the app, as Inkscape's *Hershey Text* extension does (Hershey Sans/Serif/Script 1-stroke, Gothic, and Duplex/Triplex multi-stroke variants). **This is the approach for this task** — the custom renderer below; prior art also includes Valentina's single-line/SVG-font label support.
2. **"Hairline" TTFs** — engineered fonts that trace each line forward and back over itself so the closed outline *looks* like a single stroke (e.g. CamBam Stick Fonts — free, 9 variants for CNC/plotting; MecSoft/Rhino single-stroke; commercial single-line TTF bundles). These install as normal Windows fonts and work in ordinary apps, but they are still outlines (doubled-back), stroke width isn't controllable via the font, and licensing must be checked before bundling. Relevant mainly as the embeddable-`<text>` option in Task 21 mode 2, not for this task's renderer.

- [ ] Choose and bundle the stroke-glyph source (Hershey glyph data and/or single-line SVG fonts) under a GPL-compatible license; record the decision in the repo docs
- [ ] Implement a single-stroke text renderer: shape a label line into stroked (fill-less) `QPainterPath` polylines with proper advance/kerning, honoring size, bold/italic variants (stroke width/slant), alignment, eliding, mirroring, and rotation
- [ ] Add the option to the label settings UI: extend the label font selection (preferences Graphics View page / piece label settings) with the bundled single-stroke fonts alongside the existing system outline fonts; outline stays the default
- [ ] Canvas: when a single-stroke font is selected, render labels with the stroke renderer in `VTextGraphicsItem` so the preview matches what a plotter will draw; outline-font labels keep the existing `painter->drawText()` path
- [ ] Export: when a single-stroke font is selected, the `textAsPaths == true` branch of `createLabelItem()` emits single-stroke paths for those labels (outline-font labels keep the existing `QPainterPath::addText()` conversion); DXF export of single-stroke labels emits polylines
- [ ] Coordinate with Task 10/Task 21: labels in a single-stroke font exporting as `<text>` should reference the bundled single-line font name so SeamlyLayout can match it
- [ ] Verify: canvas single-stroke labels legible at typical zooms with correct placement, mirroring, and rotation; outline-font behavior unchanged everywhere; tagged pieces SVG / `.pieces.svg` / `--text2paths` / DXF / PDF / PNG correct in both font modes
- [ ] Doxygen briefs + inline comments on all touched functions; document the font architecture in the repo docs

## Task 24 — CLI: run seamly2d from the command line through to a finished seamlyLayout layout

Extend the existing console export mode (`--basename` in `src/app/seamly2d/core/vcmdexport.cpp`) so a single seamly2d command line produces the final layout: seamly2d generates the tagged `.pieces.svg` (the Layout Mode handoff, `exportPiecesToSeamlyLayout()` in `src/app/seamly2d/mainwindow.cpp`) and then runs seamlyLayout on it to produce the layout output, using the new seamlyLayout export options (the Task 21 SVG text modes and the other export formats).

**Dependency:** Task 21 (seamlyLayout export modes) for the mode pass-through; seamlyLayout also needs a headless/CLI export mode of its own, since today it is only driven interactively through its QML UI.

- [ ] Add a seamly2d CLI option (export mode) that triggers the Layout Mode handoff from the console: generate `<basename>.pieces.svg` and invoke seamlyLayout on it, resolving the app path the same way as the GUI (`paths/seamlyLayoutApp` setting)
- [ ] Add a headless CLI export mode to seamlyLayout: input `.pieces.svg`, run the layout/nesting, export to a chosen format and output path without showing the QML UI, exit with a meaningful status code
- [ ] Pass the seamlyLayout export options through the seamly2d command line (export format incl. the Task 21 SVG text modes, output destination), and document the option mapping
- [ ] Make the seamly2d invocation wait for seamlyLayout (unlike the GUI's `QProcess::startDetached`), propagate its exit status and stderr so scripted callers see failures
- [ ] Tests: seamly2d CLI option parsing (extend `tst_vcommandline`), seamlyLayout headless-export tests (Rust/Qt side), and an end-to-end check with the richmond test pattern
- [ ] Document the workflow (command-line examples) in the repo docs / `--help` output

## Task 25 — Audit and fix the Seamly2D CLI so all options work

Go through every command-line option seamly2d advertises (defined in `src/libs/vmisc/commandoptions.cpp`, wired in `src/app/seamly2d/core/vcmdexport.cpp`) and make each one actually work in console export mode. Known friction from Task 11 verification: option names are case-sensitive and inconsistently cased (e.g. `--exportOnlyDetails`), and errors only surface in a redirected stderr, not on the console.

- [ ] Inventory all options and build a test matrix: expected behavior, required companions (e.g. `--basename` enabling export mode), valid values
- [ ] Exercise each option against the richmond test pattern (all export formats, gradation size/height, page options, `--text2paths`, measurement overrides, etc.) and record which are broken, ignored, or misdocumented
- [ ] Fix the broken/ignored options; make error messages reach the console reliably (the GUI-subsystem exe detaches from the console — evaluate `AttachConsole`/subsystem handling on Windows so `--help` and errors print without redirection)
- [ ] Consider case-insensitive or consistently lowercase option aliases (keeping the existing names working for compatibility)
- [ ] Unit tests: extend `tst_vcommandline` to cover every option and the failure modes found
- [ ] Update `--help` text and repo docs with the verified behavior

## Task 26 — Export multisize patterns (nested / marker / sized-layout-set)

Add layout export for multisize patterns — `.sm2d` patterns opened with a `.smms` multisize measurement file (multiple sizes; the CLI already exposes per-size gradation via `--gradationsize`/`--gradationheight`). The user chooses one of three multisize layout products in the settings dialog; all products orient every piece with its grainline pointing up.

- [ ] Settings dialog: user chooses "nested layout", "marker layout", or "set of sized layouts" for multisize export
- [ ] Generate a "size layout" for each size in the `.smms` file, all grainlines pointing up (per-size piece generation via the existing gradation machinery)
- [ ] Nested layout:
  - [ ] For each piece in the largest size, create a layout with all grainlines pointing up
  - [ ] For the remaining sizes in descending order: place each piece on top of its matching largest-size piece, grainline up, centering its center point on the largest piece's center point — each large piece becomes the base of a "pyramid" of matching pieces with the smallest on top
  - [ ] Apply transforms so all pieces are placed in global space
  - [ ] Group all pieces of each size together, so upstream tools (Pattern Projector, Inkscape, Illustrator, ...) can toggle each size's visibility
- [ ] Marker layout: copy all pieces from the size layouts and arrange them into a single marker layout, all grainlines pointing up
- [ ] Set of sized layouts:
  - [ ] Let the user view each size's layout in the canvas — UI design open: per-size tabs across the top of the canvas is the working idea, to be settled during implementation
  - [ ] Export the set to a single multi-page PDF, or to individual files of any export type
- [ ] Tests with a multisize test pattern (need a `.sm2d` + `.smms` fixture); verify grouping/grainline orientation in the exported SVG/PDF
- [ ] Doxygen briefs + inline comments on all touched functions; document the three products in the repo docs

## Task 30 — Evolve tool-created object serialization: nest created objects as `<item>` children (first tool: trueDarts)

Seamly2D tools that create more than one object today **flatten** every created object into suffix-numbered attributes on the single tool element. The `trueDarts` point is the first tool to convert to a structure where each created object is its own nested `<item>` child element, so tool-created objects become first-class and can be processed uniformly. This is intended as the template for the other multi-object tools later (the general "evolve processing of objects created by tools" goal); do trueDarts first, then generalize.

**Current shape (verified):** the `<point type="trueDarts">` element is the tool; it creates two child `VPointF` geometric objects that live in the `VContainer` but have **no** `<point>` element of their own — they exist only as flattened attributes on the tool element:

```xml
<point id="115" baseLineP1="113" baseLineP2="114" dartP1="111" dartP2="109" dartP3="112"
       point1="116" name1="B9"  mx1="0.132292"  my1="0.264583"  showPointName1="true"
       point2="117" name2="B10" mx2="-1.65728"  my2="0.237873"  showPointName2="true"
       type="trueDarts"/>
```

**Target shape (well-formed):**

```xml
<point id="115" baseLineP1="113" baseLineP2="114" dartP1="111" dartP2="109" dartP3="112" type="trueDarts">
    <item idobj="116" objtag="point" name="B9"  mx="0.132292" my="0.264583" showPointName="true"/>
    <item idobj="117" objtag="point" name="B10" mx="-1.65728" my="0.237873" showPointName="true"/>
</point>
```

**Evaluation of the change request (2026-07-21) — implementable, with corrections to the draft:**

- **Feasible, but it is a file-format-breaking schema change.** It touches the XSD, the tool's write path, the pattern parser, and — because old files use the flat form — it requires a `VPatternConverter` upgrade step and a schema-version bump (`src/libs/ifc/xml/vpatternconverter.cpp`: `PatternMaxVerStr`/`CurrentSchema`, currently `0.7.4` → new `0.7.5`, plus a new `src/libs/ifc/schema/pattern/v0.7.5.xsd`). Not a one-liner; scope it as a full task.
- **Fix the malformed XML in the draft:** the draft wrote `<item>idobj="116" ...</item>` (attributes in element *text*). Attributes must live in the tag: `<item idobj="116" .../>` (self-closing), as shown above.
- **Drop the per-object numeric suffixes inside `<item>`:** since each object is now its own element, use un-suffixed `name`, `mx`, `my`, `showPointName` on each `<item>` (the draft kept `mx1/my1` vs `mx2/my2`, and put `showPointName1` on **both** items — a copy/paste bug).
- **Keep child ids in the global id-space (most important correctness constraint):** the parser must still register `116`/`117` in the `VContainer` under those ids, because other tools, groups, and history reference the created points (B9/B10) as inputs. `idobj` just relabels the child-of-tool id attribute; it must not move the points out of the shared id space or they stop being referenceable.
- **`objtag` is a good forward-looking discriminator** for the created object's kind (point/arc/curve/…), which is what lets this generalize to other tools; keep it distinct from the tool's own `type` attribute (hence `objtag`, not `type`).
- **Delete behavior is already correct** and must be preserved: `VToolDoublePoint` renders each child as a `VSimplePoint` whose right-click → Delete deletes the **whole** tool element with both children; individual children cannot be deleted alone today. The request's "delete 115 via right-clicking 116 or 117" already works — verify it still does after the restructure.

**Key code map (verified):**

- Write: `VToolDoublePoint::SaveOptions` (`src/libs/vtools/tools/drawTools/toolpoint/tooldoublepoint/vtooldoublepoint.cpp:382`) writes `point1/point2` + per-point `name/mx/my/showPointName`; `VToolTrueDarts::SaveOptions` (`.../vtooltruedarts.cpp:416`) adds `baseLineP1/P2`, `dartP1/P2/P3`, `type`.
- Read: `VPattern::ParseToolTrueDarts` (`src/app/seamly2d/xml/vpattern.cpp:2340`).
- Per-child save paths that also write these attributes: label move (`MoveDoubleLabel`) and show-name (`ShowDoublePointName`) undo commands invoke `SaveOptions` with the child obj (the `p1id`/`p2id` branches) — they must target the matching `<item>` element after the change.
- Attribute-name constants: `src/libs/ifc/ifcdef.*` (`AttrName1`, `AttrMx1`, `AttrPoint1`, …) — add `AttrIdObj`, `AttrObjTag`.
- Other consumers of the flat attrs to review: history dialog (`src/app/seamly2d/dialogs/history_dialog.cpp`), groups widget (`groups_widget.cpp`), tool-options property browser (`core/vtooloptionspropertybrowser.cpp`), and `src/test/Seamly2DTest/tst_findpoint.*`.

- [ ] Decide/record the schema: nested `<item idobj objtag name mx my showPointName/>` under the tool element; well-formed attributes; un-suffixed per-item names; children stay in the global id-space and remain referenceable
- [ ] Add `AttrIdObj` / `AttrObjTag` attribute constants in `src/libs/ifc/ifcdef.*`
- [ ] Update the write path (`VToolDoublePoint::SaveOptions` + `VToolTrueDarts::SaveOptions`) to emit one `<item>` per created object; update the per-child save paths (`MoveDoubleLabel`, `ShowDoublePointName`) to write into the correct `<item>`
- [ ] Update the read path (`VPattern::ParseToolTrueDarts`) to read the `<item>` children, registering each under its `idobj` in the `VContainer`
- [ ] Add `v0.7.5.xsd` allowing `<item>` children under the trueDarts point; add a `VPatternConverter` step that rewrites old flat trueDarts elements into the nested form; bump `PatternMaxVerStr`/`CurrentSchema` (0.7.4 → 0.7.5)
- [ ] Preserve delete behavior: right-click either created object deletes the whole tool element with its children; children not individually deletable (confirm `VToolDoublePoint` path unchanged)
- [ ] Update other consumers of the flat attributes: history dialog, groups widget, tool-options property browser, `tst_findpoint`
- [ ] Tests: round-trip save/load of a trueDarts pattern in the new form; a converter test upgrading a real ≤0.7.4 fixture (e.g. `src/test/CollectionTest/share/seamtest/seamtest1.sm2d`) to 0.7.5; verify the created points remain referenceable by other tools after load
- [ ] Doxygen briefs + inline comments on all touched functions; document the new tool-object `<item>` convention in the repo docs and note it is the template for the other multi-object tools
