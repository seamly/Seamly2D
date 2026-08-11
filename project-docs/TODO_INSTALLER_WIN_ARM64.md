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
family. Everything else about the package — install layout, UpgradeCode,
associations, shortcuts, legacy-install removal, version mapping, signing — is
shared with x64 and documented in `.github/README-BUILDS.md` and
`scripts/packaging/windows/README.md`.

## InstWinArm64.1 — Ship SeamlyLayout in the arm64 MSI

The one real gap. Blocked on a toolchain, not on packaging.

- [ ] InstWinArm64.1.1 Get SeamlyLayout's Rust + cxx-qt bridge cross-compiling to
  `aarch64-pc-windows-msvc` (Corrosion/cxx-qt cross story is unresolved; see
  `.github/README-BUILDS.md`)
- [ ] InstWinArm64.1.2 Resolve Qt WebEngine on arm64 — Qt ships no cross-compiled
  arm64 WebEngine, and `SvgCanvas.qml`'s `WebEngineView` requires it. Either find
  a supported arm64 WebEngine, build one, or decide on a non-WebEngine SVG canvas
  for arm64
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
