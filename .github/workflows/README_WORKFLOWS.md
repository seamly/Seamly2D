# Seamly2D GitHub Workflows Overview

## Automated Workflows

### [CI](ci.yml) - Main Continuous Integration Workflow
**Triggers**: Pull requests, pushes to develop, scheduled releases (Mondays 01:30 UTC), manual dispatch

**Features**:
- **Tests**: Builds all platforms on pull requests with downloadable artifacts and Linux unit tests
- **Pre-Releases**: Automatic prereleases when PRs are merged to develop branch
- **Releases**: Scheduled weekly releases with date-based versioning (vYYYY.MM.DD.HHMM)
- **Code Signing**: Integrated Windows and Mac code signing for develop branch
  - Signs both 64-bit and 32-bit Windows executables
  - Signs and notarizes Mac builds
  - Uses Google Cloud KMS with CloudHSM for secure signing
  - Uses Mac Developer ID certificate and private key and notarize API key in secrets

**Builds**: Linux AppImage, Windows 64-bit/32-bit installers (.exe/.zip), macOS (.dmg/.zip)

### [SeamlyLayout CI](seamlylayout-ci.yml) - SeamlyLayout build + test (Qt 6.10)

**Triggers**: pushes to `develop` / `run-seamlyLayout` that touch `src/app/seamlylayout/**` (or the workflow file), pull requests touching the same paths, and manual dispatch. Path filters keep it from running on unrelated changes.

**Purpose**: builds and unit-tests the SeamlyLayout daughter app (Rust core + Qt 6.10 QML frontend) on `ubuntu-latest`, mirroring what `src/app/seamlylayout/qd.ps1` / `build.ps1` do locally.

**Why it is separate from [CI](ci.yml)**: the main CI pins **Qt 6.11.1** (matching the seamly2d/seamlyme qmake release toolchain), but SeamlyLayout requires **Qt 6.10.1** plus a Rust + CMake/Ninja toolchain. Keeping it in its own workflow makes the two fully independent — a SeamlyLayout failure never blocks the seamly2d/seamlyme jobs, and vice versa.

**What it does**:
1. Installs Rust (stable) and Qt 6.10 (`jurplel/install-qt-action`, with the `qtwebengine` module the frontend needs), with cargo and Qt caching.
2. Configures + builds `qt_frontend` via its CMake `debug` preset; the same build drives Corrosion / cxx-qt-cmake, which compiles the `cxxqt_bridge` Rust crate — so one step builds both the Rust bridge and the C++/QML app.
3. Runs the Qt frontend unit tests (`ctest`, under `xvfb`) and the Rust workspace tests (`cargo test --workspace`).

**Future consolidation**: when seamly2d/seamlyme move to Qt 6.10, merge this job back into `ci.yml` so the whole family shares one Qt toolchain (noted in the workflow's header comment).

### [Windows MSI](windows-msi.yml) - Bundled WiX `.msi` installer (Task 13)

**Triggers**: pushes to `develop` / `run-seamlyLayout` that touch `packaging/windows/**`, `scripts/smsi.ps1`, or the workflow file; pull requests touching the same paths; and manual dispatch. Path filters keep it from running on unrelated changes.

**Purpose**: builds the Windows **MSI installer** that ships the whole Seamly app family — `seamly2d`, `seamlyme`, and `SeamlyLayout` — in one bundled package **per architecture** (x64 and arm64), using the WiX toolset from [`seamly-family.wxs`](../../packaging/windows/seamly-family.wxs) via [`scripts/smsi.ps1`](../../scripts/smsi.ps1). The hands-on build/test reference is [`packaging/windows/README.md`](../../packaging/windows/README.md).

**Why it is separate from [CI](ci.yml)**: same reasoning as SeamlyLayout CI, but inverted — the MSI needs **both** toolchains in one job (Qt 6.11 for the qmake-built parent apps *and* Qt 6.10.1 + Rust + CMake/Ninja for SeamlyLayout) to build all three apps and bundle them. Folding that into `ci.yml` would slow and destabilize every push, so it lives on its own and only runs when the packaging inputs change. `ci.yml`'s NSIS installer remains the released Windows installer until the MSI replaces it.

**What it does** (matrix: `x64`, `arm64`):
1. Installs both Qt kits (6.10.1 for SeamlyLayout first, then 6.11.1 for the parents so the bare `qmake`/`nmake` build resolves to the parent Qt) plus MSVC (`ilammy/msvc-dev-cmd`), and for x64 also Rust + Ninja.
2. Builds `seamly2d.exe` + `seamlyme.exe` (qmake/nmake, cross-compiled for arm64) and, on x64 only, `SeamlyLayout.exe` (CMake release preset).
3. Runs `scripts/smsi.ps1` to stage the runtimes and build the MSI with WiX **v6** (v7 is gated behind an OSMF EULA, error `WIX7015`) — **x64 = all three apps, arm64 = the two parents only** (`-NoSeamlyLayout`, since SeamlyLayout has no arm64 build yet).
4. Signs the `.msi` with `jsign` (Google Cloud KMS, same as the NSIS exe), guarded on the `SEAMLY_SIGNING_PROJECT_ID` secret so untrusted PR runs skip it, and uploads the MSI as a build artifact.

**Future consolidation**: when the MSI replaces the NSIS installer, fold these steps into `ci.yml`'s windows job and wire the `.msi` into the release/publish job (noted in the workflow's header comment).

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
- [Nullsoft Scriptable Install System](https://nsis.sourceforge.io/Main_Page) Not an action, but NSIS for short, builds the Windows installer using the [seamly2d-installer.nsi](/dist/seamly2d-installer.nsi) script file. As of this moment, the script includes steps for setting up a start menu group and configuration necessary to provide an uninstaller.
- [WiX Toolset](https://wixtoolset.org/) Not an action either — installed in [windows-msi.yml](windows-msi.yml) as the `wix` .NET global tool (pinned to `6.*`; v7 is gated behind an Open Source Maintenance Fee EULA, error `WIX7015`). It builds the bundled Seamly family MSI from [`packaging/windows/seamly-family.wxs`](../../packaging/windows/seamly-family.wxs); the `WixToolset.UI.wixext` extension (version-matched to the core tool) supplies the directory-chooser installer UI.
