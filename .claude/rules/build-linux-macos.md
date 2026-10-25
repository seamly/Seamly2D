---
paths:
  - "packaging/linux/**"
  - "packaging/macos/**"
---

# Local Linux and macOS Builds

## Linux

`packaging/linux/local_build_appimage.sh` is the build. It builds Seamly2D,
SeamlyMe and SeamlyLayout release binaries, runs the Qt and Rust unit tests,
then packages the Linux x86_64 AppImage via `linuxdeploy` (auto-downloaded
into `packaging/linux/tools/` on first run).

Requires `qmake`, `cmake`, `ninja`, `cargo`, `ctest`, `xvfb-run` and
`libxerces-c-dev` already installed — the script fails with an install hint
rather than running `sudo apt install` itself. A Qt 6.11.1+ `gcc_64` kit under
`~/Qt` is required, with the same modules as the Windows kit.

Switch: `--skip-tests` — skip `make check`, `ctest`, and `cargo test`. Only
for a packaging-only change; these suites have no other local runner on
Linux besides `ci.yml`'s `linux-test` job.

## macOS

`packaging/macos/local_build_dmg.sh` is the build. It builds Seamly2D,
SeamlyMe and SeamlyLayout release binaries, runs the Qt and Rust unit tests,
then packages the macOS DMG via `packaging/macos/macos.pro`'s
`seamlysuitedmg` target (the same `hdiutil` step `ci.yml`'s `macos` job
uses). The output is unsigned and not notarized — signing needs CI secrets.

`ci.yml`'s `macos` job does not run its own tests; it gates on the
`linux-test` job's coverage. This script runs the Qt and Rust suites anyway
for a safer local loop — pass `--skip-tests` to match CI's behavior exactly.

Requires `qmake`, `cmake`, `ninja`, `cargo`, `ctest`, `hdiutil`, `xcrun`, and
Xcode command-line tools (`xcode-select --install`) already installed, plus
`xerces-c` via Homebrew. A Qt 6.11.1+ `macos` kit under `~/Qt` is required,
with the same modules as the Windows kit (except `qtserialport`).
