# Seamly2D GitHub Workflows Overview

## Automated Workflows

### [CI](ci.yml) - Main Continuous Integration Workflow
**Triggers**: Pull requests, pushes to `develop` / `run-seamlyLayout` / `feat-*`, scheduled releases (Mondays 01:30 UTC), manual dispatch

**Features**:
- **Tests**: Builds all platforms on pull requests with downloadable artifacts and Linux unit tests
- **Pre-Releases**: `schedule` and `workflow_dispatch` runs on **`run-seamlyLayout`** publish a GitHub **pre-release** (`prerelease: true`) with date-based versioning (vYYYY.MM.DD.HHMM). The ref is deliberately *not* `develop`: `origin/develop` is kept as a pristine mirror of upstream `FashionFreedom/Seamly2D` until the SeamlyLayout migration is finished, so nothing is released from it. `run-seamlyLayout` is also this repository's default branch, which is the ref the `schedule` trigger runs on.
- **Code Signing**: Integrated Windows and Mac code signing
  - Signs both Windows `.msi` packages (x64 and arm64)
  - Signs and notarizes Mac builds
  - Uses Google Cloud KMS with CloudHSM for secure signing
  - Uses Mac Developer ID certificate and private key and notarize API key in secrets

**Builds**: Linux AppImage, macOS (.dmg/.zip), and the two Windows `.msi` packages — **`seamly-x64.msi`** and **`seamly-arm64.msi`** (see the `windows-msi` job below).

#### The `windows-msi` job (Tasks Installer.1.1 and Installer.1.2)

**Windows ships MSIs and nothing else.** NSIS was retired on 2026-08-11: the old `windows` job, which built `Seamly2D-windows.zip` (x64) and `Seamly2D-win-arm64.zip` (arm64), is gone and no workflow runs `makensis` any more.

The job is a two-leg matrix over `arch`:

| Leg | Runner | Apps in the package | Qt host / arch | Qt modules |
|---|---|---|---|---|
| `x64` | `windows-latest` | seamly2d + seamlyme (qmake) **and** SeamlyLayout (CMake/Ninja + Cargo) | `windows` / `win64_msvc2022_64` | `qtmultimedia qtwebengine qtwebchannel qtpositioning` |
| `arm64` | `windows-11-arm` | same three apps | `windows_arm64` / `win64_msvc2022_arm64` | same four modules |

Each leg installs one Qt 6.11.1 kit, builds the apps, then runs [`smsi.ps1`](../../scripts/packaging/windows/smsi.ps1) to stage the shared runtime and build, validate and authoring-test `scripts/seamly-msi/<arch>/seamly-<arch>.msi`. `jsign` signs each package and `publish` attaches both to the pre-release. `fail-fast: false` so one arch's failure does not hide the other's result.

**Both legs are native — nothing is cross-compiled.** Each arch builds on its own runner with its own host kit: no `amd64_arm64` MSVC, no `..._cross_compiled` Qt, no `host-qmake`, and no explicit cargo target (each runner's default host toolchain already matches the package it is building). That is also why `windeployqt` needs no `--qtpaths` wrapper on either leg — see `common.pri`'s `deployQtRuntime()`.

**arm64 ships all three apps as of 2026-08-11** (commit `fba962c4d8`). It previously shipped the two parents only (`-NoSeamlyLayout`, `qtmultimedia` alone), on the belief that Qt publishes no arm64 Windows WebEngine — true of Qt 6.8, **false for 6.11.1**. The `qt-arm64-module-probe` workflow verifies this at every Qt bump; do not re-assert the claim without re-running it.

**Why the steps are inline here rather than reused from `windows-msi.yml`:** the `publish` job releases these `.msi` files, so they have to be built from the same commit, in the same run, as every other release artifact. `windows-msi.yml` still rebuilds the packages on its own path-filtered triggers when only the packaging inputs change, without dragging the whole of CI along. **The two copies of the build steps must be kept in step** — the job here is `windows-msi.yml`'s `msi` job verbatim, minus its own version step.

### [SeamlyLayout CI](seamlylayout-ci.yml) - SeamlyLayout build + test (Qt 6.11)

**Triggers**: pushes to `develop` / `run-seamlyLayout` that touch `src/app/seamlylayout/**` (or the workflow file), pull requests touching the same paths, and manual dispatch. Path filters keep it from running on unrelated changes.

**Purpose**: builds and unit-tests the SeamlyLayout daughter app (Rust core + Qt 6.11 QML frontend) on `ubuntu-latest`, mirroring what `src/app/seamlylayout/qd.ps1` / `build.ps1` do locally.

**Why it is separate from [CI](ci.yml)**: **not** the Qt version — since Task 30 this workflow and `ci.yml` both pin **Qt 6.11.1**, and the two `QT_VERSION` values must be kept in step. What keeps them apart is the build system: SeamlyLayout is CMake/Ninja + Cargo/Corrosion with its own toolchain steps (Rust, cargo cache, the WebEngine module set), while `ci.yml`'s parent-app jobs are qmake. Separate workflows also keep the triggers path-filtered and the failures independent — a SeamlyLayout failure never blocks the seamly2d/seamlyme jobs, and vice versa.

**What it does**:
1. Installs Rust (stable) and Qt 6.11 (`jurplel/install-qt-action`, with the `qtwebengine` module the frontend needs plus its own `qtwebchannel`/`qtpositioning` dependencies, which `aqtinstall` does not auto-resolve), with cargo and Qt caching.
2. Configures + builds `qt_frontend` via its CMake `debug` preset; the same build drives Corrosion / cxx-qt-cmake, which compiles the `cxxqt_bridge` Rust crate — so one step builds both the Rust bridge and the C++/QML app.
3. Runs the Qt frontend unit tests (`ctest`, under `xvfb`) and the Rust workspace tests (`cargo test --workspace`).

**Consolidation, re-evaluated (Task 30)**: the differing Qt pins used to be the blocker for merging this job into `ci.yml`. That is resolved, but the merge was **deliberately not made** — different build systems and different path filters mean folding them together would rebuild the parent apps on every layout-only change for no benefit. Revisit only if the parent apps also move to CMake.

### [Windows MSI](windows-msi.yml) - Bundled WiX `.msi` installer (Task 13)

**Triggers**: pushes to `develop` / `run-seamlyLayout` that touch `scripts/packaging/windows/**` (the WiX source, `license.rtf`, and the `smsi.ps1` driver all live here) or the workflow file; pull requests touching the same paths; and manual dispatch. Path filters keep it from running on unrelated changes.

**Purpose**: builds the Windows **MSI installer** that ships the whole Seamly app family — `seamly2d`, `seamlyme`, and `SeamlyLayout` — in one bundled package **per architecture** (x64 and arm64), using the WiX toolset from [`seamly-family.wxs`](../../scripts/packaging/windows/seamly-family.wxs) via [`scripts/packaging/windows/smsi.ps1`](../../scripts/packaging/windows/smsi.ps1). The hands-on build/test reference is [`scripts/packaging/windows/README.md`](../../scripts/packaging/windows/README.md).

**Why it is separate from [CI](ci.yml)**: it only runs when the packaging inputs change, so a `seamly-family.wxs` or `smsi.ps1` edit gets a full x64 **and** arm64 package check without waiting on the rest of CI. `ci.yml` has its own `windows-msi` job carrying **the same build steps for both arches**, because the artifacts it publishes must come from the same commit and run as every other release artifact — **keep the two in step when either changes**. Both `.msi` files are the released Windows installers; NSIS is retired entirely.

**What it does** (matrix: `x64`, `arm64`):

1. Installs **one** Qt 6.11.1 kit for all three apps (Task 30 — it used to install two kits in a carefully ordered dance so the parent `qmake` would not bind to SeamlyLayout's Qt) plus MSVC (`ilammy/msvc-dev-cmd`), Rust and Ninja. Both legs take the identical module set: `qtmultimedia qtwebengine qtwebchannel qtpositioning`.
2. Builds `seamly2d.exe` + `seamlyme.exe` (qmake/nmake) and `SeamlyLayout.exe` (CMake release preset). **Every leg is native** — x64 on `windows-latest`, arm64 on `windows-11-arm` with the `windows_arm64` host kit; nothing is cross-compiled.
3. Runs `scripts/packaging/windows/smsi.ps1` to stage the **single shared Qt runtime** and build the MSI with WiX **v6** (v7 is gated behind an OSMF EULA, error `WIX7015`). **Both arches ship all three apps** — the same `smsi.ps1` invocation, no `-NoSeamlyLayout`.
4. Signs `scripts/seamly-msi/<arch>/seamly-<arch>.msi` with `jsign` (Google Cloud KMS, same as the NSIS exe), guarded on the `SEAMLY_SIGNING_PROJECT_ID` secret so untrusted PR runs skip it, and uploads the MSI as a build artifact.

**Consolidation (Tasks Installer.1.1 and Installer.1.2)**: `ci.yml` now builds and releases **both** `.msi` files itself. This workflow is kept only for packaging-only checks and is marked deprecated in `ci.yml` — remove it once that arrangement has been exercised.

## Code Signing Workflow

### Integrated Signing Process
The main CI workflow includes integrated code signing for Windows and Mac executables.

### Signing Requirements
- **Branch**: Only runs on `develop` branch
- **Secrets**: Requires Google Cloud KMS secrets and Mac Developer ID certificate and notarize API key configured

## Emergency Procedures

### Skip Code Signing (Emergency Override)
When signing infrastructure fails (certificate expiration, KMS issues, etc.):

1. Go to repository **Settings** → **Secrets and variables** → **Actions**
2. Remove the **Secret** `SEAMLY_SIGNING_PROJECT_ID` for windows and `APPLE_SIGN_IDENTITY` for mac
3. Push to `develop` branch to trigger workflow
4. Workflow will:
   - ✅ Build Windows 64-bit and 32-bit executables
   - 📦 Release unsigned executables with warnings

**⚠️ Warning**: Unsigned executables will trigger security warnings and should only be used for testing or emergency releases.

### Re-enable Code Signing
To restore normal signing after emergency:

1. Add the **Secrets** back in
2. Push to `develop` branch
3. Normal signing workflow will resume with approval required

## External Github Actions
- [Install Qt](https://github.com/marketplace/actions/install-qt). Referenced as `jurplel/install-qt-action`, installs the Qt platform across all the three different runners (ubuntu-18.04, macos-latest, windows-2022) consistently. Internally it uses the [aqtinstall](https://github.com/miurahr/aqtinstall/) installer written in Python. Worth knowing if those errors propagate up through the GitHub action.
- [Enable Developer Command Prompt](https://github.com/marketplace/actions/enable-developer-command-prompt) Referenced as `ilammy/msvc-dev-cmd`, sets up the command line environment on the windows-2022 runner (`PATH` and such) to expose Microsoft Visual C++.
- [softprops/action-gh-release](https://github.com/marketplace/actions/gh-release). Referenced as `softprops/action-gh-release`, creates a release and uploads all artifacts to that release.
- [WiX Toolset](https://wixtoolset.org/) Not an action — installed in [ci.yml](ci.yml) and [windows-msi.yml](windows-msi.yml) as the `wix` .NET global tool (pinned to `6.*`; v7 is gated behind an Open Source Maintenance Fee EULA, error `WIX7015`). It builds the bundled Seamly family MSI from [`scripts/packaging/windows/seamly-family.wxs`](../../scripts/packaging/windows/seamly-family.wxs); the `WixToolset.UI.wixext` extension (version-matched to the core tool) supplies the directory-chooser installer UI, and `WixToolset.Util.wixext` supplies the `RemoveFolderEx` used to clear a pre-MSI installation.
- **NSIS is retired** (Task Installer.1.2, 2026-08-11). No workflow runs `makensis` any more; Windows ships `seamly-x64.msi` and `seamly-arm64.msi` only. [`dist/seamly2d-installer.nsi`](/dist/seamly2d-installer.nsi) is kept unbuilt, as the record of what a pre-MSI installation left on disk for the MSI's removal authoring to clean up.
