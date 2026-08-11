# TODO — Update build for Windows ARM64

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file begin with `InstWinArm64.`

## Status (2026-08-11)

`seamly-arm64.msi` is built and released by `ci.yml`'s `windows-msi` job (Task
Installer.1.2). **NSIS is retired** — no workflow builds `Seamly2D-win-arm64.zip`
or runs `makensis` any more, and `dist/seamly2d-installer.nsi` is kept unbuilt
only because `seamly-family.wxs` cites it as the record of a pre-MSI
installation's on-disk footprint.

The arm64 package ships **two of the three apps** — `seamly2d` + `seamlyme`
(`smsi.ps1 -NoSeamlyLayout`). That is exactly what the retired arm64 NSIS
package carried, so nothing was lost in the switch, but it is not yet the whole
family. Closing that gap is InstWinArm64.1, and the route to it is
InstWinArm64.3 — a one-off from-source build of Qt WebEngine for win-arm64. Everything else about the package — install layout, UpgradeCode,
associations, shortcuts, legacy-install removal, version mapping, signing — is
shared with x64 and documented in `.github/README-BUILDS.md` and
`scripts/packaging/windows/README.md`.

## InstWinArm64.1 — Ship SeamlyLayout in the arm64 MSI

The one real gap. Blocked on a toolchain, not on packaging.

- [ ] InstWinArm64.1.1 Get SeamlyLayout's Rust + cxx-qt bridge cross-compiling to
  `aarch64-pc-windows-msvc` (Corrosion/cxx-qt cross story is unresolved; see
  `.github/README-BUILDS.md`)
- [ ] InstWinArm64.1.2 Resolve Qt WebEngine on arm64 — Qt ships no cross-compiled
  arm64 WebEngine, and `SvgCanvas.qml`'s `WebEngineView` requires it. **Decided
  2026-08-11: build it from source once — see InstWinArm64.3**, which is the
  route this subtask depends on
- [ ] InstWinArm64.1.3 In `ci.yml` and `windows-msi.yml`: add
  `qtwebengine qtwebchannel qtpositioning` to the arm64 leg's `qt-modules`,
  delete the `matrix.arch == 'x64'` guards on the Rust/Ninja/CMake steps, and
  drop `-NoSeamlyLayout` from the arm64 `smsi.ps1` call. Keep the two workflows
  in step
- [ ] InstWinArm64.1.4 Confirm `smsi.ps1` needs `-WinDeployQt6` on arm64 once
  SeamlyLayout is included (it is only resolved and used under `$includeLayout`,
  so today the arm64 leg correctly omits it) and that the arm64 kit's
  `windeployqt6.exe` can be run on the x64 build runner
- [ ] InstWinArm64.1.5 Re-run `test_msi_authoring.ps1` expectations for a
  three-app arm64 package

## InstWinArm64.3 — Build Qt WebEngine for win-arm64 from source, once

**The chosen route to InstWinArm64.1.2 (decided 2026-08-11).**

Build Qt WebEngine for win-arm64 from source, once. Supported since Qt 6.9 — Qt
just never shipped the binaries. Build it out of band (not per-CI-run), publish
it as a cached artifact, and point `install-qt-action`/CMake at it. Everything is
then native arm64: all three apps, one Qt runtime, no emulation, no second Qt
copy, and Task 30's flat layout survives. High up-front cost — it's a Chromium
build — but it's a one-time cost that buys the end state you actually want, and
it's the only route where arm64 and x64 are genuinely the same product.

**Routes rejected, and why** (do not re-litigate without new information):

- **Ship SeamlyLayout as an emulated x64 app with its own Qt runtime.** Puts the
  Rust nesting core — the number-crunching — on the slow side of Prism to solve a
  problem that only exists on the display side, and re-introduces the two-Qt-copy
  layout Task 30 removed (the ~187 MB package). Chromium/V8 is also among the
  harder things to emulate, not the easier, so the one component that forced the
  route is the one least likely to be happy in it.
- **Replace `WebEngineView` with a Qt Quick SVG canvas.** Rejected 2026-08-11:
  WebEngineView is required. Qt's SVG engine is SVG 1.2 Tiny, not Chromium's.

- [ ] InstWinArm64.3.1 Scope the build: Qt 6.11.1 `qtwebengine` sources, the
  host x64 Qt of the same release for the host tools, MSVC `amd64_arm64`, and
  Chromium's own prerequisites (depot_tools/gn/ninja, Python, Node). Record the
  exact configure line, because it has to be reproducible for every Qt bump
- [ ] InstWinArm64.3.2 Decide where the build runs — a GitHub runner is likely
  to exceed the 6 h job limit for Chromium, so plan for a self-hosted or
  local one-off build. Record the machine and the wall-clock cost
- [ ] InstWinArm64.3.3 Decide where the artifact lives and how it is versioned:
  one archive per Qt release (e.g. `qtwebengine-6.11.1-win-arm64`), on a GitHub
  release or a cache the workflows can reach without credentials. It must be
  reproducible from InstWinArm64.3.1, not a mystery binary
- [ ] InstWinArm64.3.4 Wire CI to consume it: the arm64 leg of `ci.yml`'s and
  `windows-msi.yml`'s MSI matrix downloads the archive and merges it into the
  `install-qt-action` kit (or sets `CMAKE_PREFIX_PATH` so
  `find_package(Qt6 ... WebEngineQuick)` resolves). Keep both workflows in step
- [ ] InstWinArm64.3.5 Verify `windeployqt6` deploys the arm64 WebEngine payload
  — the QML module tree, the Qt Quick/WebEngine DLLs and `QtWebEngineProcess.exe`
  — into the flat install layout, and that `smsi.ps1` stages it (this is what
  drops `-NoSeamlyLayout` and `-WinDeployQt6` back into the arm64 call)
- [ ] InstWinArm64.3.6 Confirm the licensing/attribution obligations of shipping
  a self-built Chromium are met in `licenses\` alongside the existing entries
- [ ] InstWinArm64.3.7 Document the whole thing in `.github/README-BUILDS.md` —
  especially the **Qt-bump cost**: every future Qt release needs this build
  repeated before arm64 can follow x64
- [ ] InstWinArm64.3.8 Only after the above: run InstWinArm64.1.3 and .1.5, then
  the hardware verification in InstWinArm64.2

## InstWinArm64.2 — Verify on real arm64 hardware

Nothing here has ever run on an arm64 machine; the CI job only cross-compiles
and inspects the package. Overlaps Installer.2.2.

- [ ] InstWinArm64.2.1 Install `seamly-arm64.msi` on a Windows 11 arm64 machine
  and run `scripts/packaging/windows/test_msi_install.ps1` through all four
  phases (`Baseline`/`Installed`/`Upgraded`/`Removed`)
- [ ] InstWinArm64.2.2 Confirm each app starts and stays running natively (not
  under x64 emulation) and that the file associations resolve
- [ ] InstWinArm64.2.3 Exercise the pre-MSI-installation removal path against a
  real legacy installation on arm64
- [ ] InstWinArm64.2.4 Confirm the signed package shows "Verified publisher:
  Seamly Systems, Inc." (depends on `TODO_CODE_SIGNING.md` CodeSign.1.6)
