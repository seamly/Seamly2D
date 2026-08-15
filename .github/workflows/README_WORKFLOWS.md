# Seamly2D GitHub Workflows Overview

## Automated Workflows

### [CI](ci.yml) - Main Continuous Integration Workflow
**Triggers**: Pull requests, pushes to `run-seamlyLayout`, manual dispatch. Pushes carry a `paths-ignore` for `**.md` / `project-docs/**` / `LICENSE`, so documentation changes never start a build; `pull_request` is deliberately unfiltered so a docs-only PR still reports its checks. `concurrency: cancel-in-progress` supersedes older in-progress runs on the same ref.

> **A full run costs ~50 minutes** across Linux, macOS and Windows x64/arm64. The task workflow in [CLAUDE.md](../../CLAUDE.md) therefore puts `[skip ci]` in the step-8 merge commit by default and clears the accumulated skips with one deliberate `gh workflow run ci.yml --ref run-seamlyLayout` before a milestone. Omit the skip when the task touched workflows, packaging, build files (`*.pro`, `CMakeLists.txt`, `Cargo.toml`) or platform-specific code — the local Windows build cannot verify those.

**Features**:
- **Tests**: Builds all platforms on pull requests with downloadable artifacts and Linux unit tests
- **Pre-Releases**: `workflow_dispatch` runs on **`run-seamlyLayout`** publish a GitHub **pre-release** (`prerelease: true`) with date-based versioning (vYYYY.MM.DD.HHMM). The `publish` job also still tests for `github.event_name == 'schedule'`, but `on:` carries no `schedule` trigger any more, so dispatch is the only way a release is cut — **a plain push never publishes**, it only builds. The ref is deliberately *not* `develop`: `origin/develop` is kept as a pristine mirror of upstream `FashionFreedom/Seamly2D` until the SeamlyLayout migration is finished, so nothing is released from it. `run-seamlyLayout` is also this repository's default branch, which is the ref the `schedule` trigger runs on.
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

**arm64 ships all three apps as of 2026-08-11** (commit `fba962c4d8`). It previously shipped the two parents only (`qtmultimedia` alone, via the since-removed `-NoSeamlyLayout` switch), on the belief that Qt publishes no arm64 Windows WebEngine — true of Qt 6.8, **false for 6.11.1**. Re-check this at every Qt bump before re-asserting the claim: `aqt list-qt windows_arm64 desktop --modules <version> <arch>` (and the same for the `windows` host) lists what Qt actually publishes. A `qt-arm64-module-probe.yml` workflow ran that check here until 2026-08-11; it was deleted once the question was settled.

**These steps live here and nowhere else.** The `publish` job releases these `.msi` files, so they must be built from the same commit, in the same run, as every other release artifact. A second packaging-only workflow, `windows-msi.yml`, used to carry a duplicate of these steps on a `scripts/packaging/windows/**` path trigger; it was deleted on 2026-08-11 (Task InstWinX64.1.3.2) because it built both packages a second time on every packaging edit and its copy drifted. A `.wxs` or `smsi.ps1` change now runs the full CI suite — that is the trade, and it is the only copy to maintain.

### SeamlyLayout CI — removed 2026-08-12

`seamlylayout-ci.yml` built the SeamlyLayout daughter app (Rust core + Qt QML frontend) on `ubuntu-latest` and ran its `ctest` and `cargo test --workspace` suites. It was deleted on 2026-08-12: **`ci.yml` is the only workflow that builds the family on GitHub.** `ci.yml`'s `windows-msi` job already builds SeamlyLayout on both arches with the same CMake/Ninja + Cargo toolchain, so the second workflow duplicated the build and added a second `QT_VERSION` to keep in step.

**What CI no longer does:** SeamlyLayout's unit tests do not run on GitHub any more, and SeamlyLayout is not built on Linux there. Run both locally — `ctest --preset debug` in `src/app/seamlylayout/qt_frontend/`, and `cargo test --workspace` in `src/app/seamlylayout/`. Add the two test steps to `ci.yml` if that coverage is wanted back.

### Windows MSI — removed 2026-08-11

`windows-msi.yml` built the same two `.msi` packages on a `scripts/packaging/windows/**` path trigger. Task InstWinX64.1.3.2 deleted it: `ci.yml`'s `windows-msi` job (described above) already builds both arches and feeds `publish`, so the file only duplicated the work and gave the build steps a second copy to drift out of step. The Windows packaging description now lives in the [CI](ci.yml) section above.

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
- [WiX Toolset](https://wixtoolset.org/) Not an action — installed in [ci.yml](ci.yml) as the `wix` .NET global tool (pinned to `6.*`; v7 is gated behind an Open Source Maintenance Fee EULA, error `WIX7015`). It builds the bundled Seamly family MSI from [`scripts/packaging/windows/seamly-family.wxs`](../../scripts/packaging/windows/seamly-family.wxs); the `WixToolset.UI.wixext` extension (version-matched to the core tool) supplies the directory-chooser installer UI, and `WixToolset.Util.wixext` supplies the `RemoveFolderEx` used to clear a pre-MSI installation.
- **NSIS is retired** (Task Installer.1.2, 2026-08-11). No workflow runs `makensis` any more; Windows ships `seamly-x64.msi` and `seamly-arm64.msi` only. [`dist/seamly2d-installer.nsi`](/dist/seamly2d-installer.nsi) is kept unbuilt, as the record of what a pre-MSI installation left on disk for the MSI's removal authoring to clean up.
