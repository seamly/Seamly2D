# TODO — Update build for Windows ARM64

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file begin with `InstWinArm64.`

## Status (2026-08-11, revised 2026-08-10 after the native-runner switch)

`seamly-arm64.msi` is built and released by `ci.yml`'s `windows-msi` job (Task
Installer.1.2). **NSIS is retired** — no workflow builds `Seamly2D-win-arm64.zip`
or runs `makensis` any more, and `dist/seamly2d-installer.nsi` is kept unbuilt
only because `seamly-family.wxs` cites it as the record of a pre-MSI
installation's on-disk footprint.

**The arm64 package ships all three apps, same as x64, and builds NATIVELY.**
Commit `fba962c4d8` moved the arm64 leg onto the `windows-11-arm` runner with the
`windows_arm64` host and the `win64_msvc2022_arm64` kit. Nothing is
cross-compiled: no `amd64_arm64` MSVC, no `..._cross_compiled` Qt, no
`host-qmake`, no explicit cargo target. Both matrix legs install the identical Qt
module set (`qtmultimedia qtwebengine qtwebchannel qtpositioning`) and run the
identical `smsi.ps1` invocation — `-NoSeamlyLayout` is no longer passed by either
workflow. Everything else about the package — install layout, UpgradeCode,
associations, shortcuts, legacy-install removal, version mapping, signing — is
shared with x64 and documented in `.github/README-BUILDS.md` and
`scripts/packaging/windows/README.md`.

**What remains open is verification on real hardware (InstWinArm64.2), not
toolchain work.**

## InstWinArm64.1 — Ship SeamlyLayout in the arm64 MSI — DONE (2026-08-11)

Closed by the native-runner switch (`fba962c4d8`), which dissolved the problem
rather than solving it: building on an arm64 runner means there is no cross
story to resolve for the Rust + cxx-qt bridge, and Qt publishes `qtwebengine`
for the native `windows_arm64` host kit.

- [X] InstWinArm64.1.1 SeamlyLayout's Rust + cxx-qt bridge — no cross-compilation
  needed; cargo's default host toolchain on `windows-11-arm` already targets
  `aarch64-pc-windows-msvc`
- [X] InstWinArm64.1.2 Qt WebEngine on arm64 — **the "Qt ships no arm64 WebEngine"
  premise was false for 6.11.1** (true only of the Qt 6.8 era it came from).
  `qtwebengine` is published for the native `windows_arm64` host kit; verified by
  the `qt-arm64-module-probe` workflow, since deleted. No from-source build was
  needed — see InstWinArm64.3, dropped
- [X] InstWinArm64.1.3 `ci.yml` and `windows-msi.yml` both carry
  `qtmultimedia qtwebengine qtwebchannel qtpositioning` on the arm64 leg, the
  `matrix.arch == 'x64'` step guards are gone, and neither passes
  `-NoSeamlyLayout`. The two workflows are in step
- [X] InstWinArm64.1.4 `smsi.ps1` gets `-WinDeployQt6 "$env:QT_ROOT_DIR\bin\windeployqt6.exe"`
  on both legs. Because each runner is native, that executable is always one the
  runner can run *and* belongs to the kit being deployed — no `QT_HOST_PATH`
  split and no `--qtpaths` wrapper. (The same reasoning drives `common.pri`'s
  `deployQtRuntime()`; passing `--qtpaths host-qtpaths.bat` is what broke the
  arm64 MSI build on 2026-08-10)
- [ ] InstWinArm64.1.5 Re-run `test_msi_authoring.ps1` expectations for a
  three-app arm64 package — the only subtask here still outstanding

## InstWinArm64.3 — Build Qt WebEngine for win-arm64 from source, once — DROPPED (2026-08-11)

**Not needed, and never started.** This task existed only to work around the
belief that Qt ships no arm64 Windows WebEngine. That belief was Qt 6.8-era and
is false for 6.11.1: `aqt list-qt windows desktop --modules 6.11.1` lists
`qtwebengine` for the native `windows_arm64` host kit (and for the cross-compiled
one). The `qt-arm64-module-probe` workflow confirmed this before it was deleted
on 2026-08-11; re-run the `aqt list-qt` command above at any Qt bump. Installing the published
module reaches the same end state — all three apps native arm64, one Qt runtime,
Task 30's flat layout intact — at zero build cost, so a one-off Chromium build
and its per-Qt-bump repeat cost buy nothing.

**Kept for the record — do not re-litigate without new information:**

- **Ship SeamlyLayout as an emulated x64 app with its own Qt runtime.** Rejected:
  puts the Rust nesting core — the number-crunching — on the slow side of Prism
  to solve a problem that only exists on the display side, and re-introduces the
  two-Qt-copy layout Task 30 removed (the ~187 MB package). Chromium/V8 is also
  among the harder things to emulate.
- **Replace `WebEngineView` with a Qt Quick SVG canvas.** Rejected 2026-08-11:
  WebEngineView is required. Qt's SVG engine is SVG 1.2 Tiny, not Chromium's.

## InstWinArm64.2 — Verify on real arm64 hardware

**The remaining work in this file.** CI now builds and packages arm64 natively,
but nothing here has ever been installed or run on an arm64 machine — the job
only inspects the package it built. Overlaps Installer.2.2.

- [ ] InstWinArm64.2.1 Install `seamly-arm64.msi` on a Windows 11 arm64 machine
  and run `scripts/packaging/windows/test_msi_install.ps1` through all four
  phases (`Baseline`/`Installed`/`Upgraded`/`Removed`)
- [ ] InstWinArm64.2.2 Confirm each app starts and stays running natively (not
  under x64 emulation) and that the file associations resolve
- [ ] InstWinArm64.2.3 Exercise the pre-MSI-installation removal path against a
  real legacy installation on arm64
- [ ] InstWinArm64.2.4 Confirm the signed package shows "Verified publisher:
  Seamly Systems, Inc." (depends on `TODO_CODE_SIGNING.md` CodeSign.1.6)
