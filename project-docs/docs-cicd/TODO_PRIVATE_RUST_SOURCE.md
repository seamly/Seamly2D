<!-- Author: slspencer | Copyright 2026 Seamly2D Project | License: MIT -->
# TODO — Private Rust Source

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file are numbered with prefix `PrivateRust.`

## Goal

- Public repository: no Rust source.
- Public repository keeps: SeamlyLayout Qt frontend (C++, QML), CXX-Qt generated headers, pinned download of the prebuilt Rust library.
- Private repository: all Rust source, Rust tests, Rust build and release.

## Facts (checked 2026-11-17)

| Rust code | `.rs` files | License now | Public on `origin` since |
| --- | --- | --- | --- |
| `src/app/seamlylayout/crates/` (15 crates), `Cargo.toml`, `Cargo.lock` | 85 | MIT | 2026-07-19 |
| `src/app/seamlycli/` (Rust + Iced) | 10 | GPL-3.0-or-later | 2026-11-16 |

- Total: 50,677 lines of Rust.
- Sole author: slspencer. One Dependabot version bump. Sue can relicense.
- Sue maintains `FashionFreedom/Seamly2D`.
- Upstream releases: weekly tag `v<date>.<n>`, marked Latest. Upstream workflows have no tag or release triggers.
- Relicensing does not recall published copies. A holder of a published version keeps its MIT or GPL grant.
- `seamly/Seamly2D` is a public fork of `FashionFreedom/Seamly2D`. The fork network shares objects: any pushed commit is readable by SHA through every repository in it.
- Exposure: 1 fork (`elros88`, 2024, no Rust), 8 stars, 116 clones / 29 unique in 14 days (CI checkouts count).
- `upstream/develop` holds no Rust.
- Bridge: crate `cxxqt_bridge` (`staticlib`) exposes one QObject, `AppController`. Release `.lib`: 85 MB.
- Frontend use: `qt_frontend/src/HeadlessExport.cpp` includes `cxxqt_bridge/src/lib.cxxqt.h`. `qt_frontend/seamlyLayout_main.mm` calls `qmlRegisterType<AppController>`.
- Generated headers: 221 KB in `<build>/cxxqt/crates/cxxqt_bridge/include/` (`cxxqt_bridge/`, `cxx-qt/`, `cxx-qt-lib/`, `rust/`).
- Generated QML: none. Runtime `qmlRegisterType` replaces `qmldir`/`qmltypes`. If the bridge later uses `QmlModule`, ship those files too.
- Generated C++ and moc output compile into the staticlib. The public build needs only the headers.
- LGPLv3: Qt links dynamically, so users can swap Qt. No LGPL duty requires Rust source or generated code.
- Dependency licenses: no blocker.
  - `resvg`, `usvg`: MPL-2.0. Ship the notice and a source pointer (crates.io). Do not modify them.
  - `r-efi`, `hershey`, `self_cell`: dual license with an LGPL or GPL option. Elect MIT or Apache-2.0.
- No sign of GPL-derived code in the crates (grep: Valentina, vlayout, ported, GPL).
- `gh-pages` Doxygen site: no SeamlyLayout or Rust content.
- Stale remote branches with Rust: `origin/dependabot/cargo/src/app/seamlylayout/cargo-8b29273532`, `origin/copilot/fix-linux-run-unit-tests`.

## Target layout

| Item | Location |
| --- | --- |
| Rust crates, `Cargo.toml`, `Cargo.lock`, Rust tests | Private repo |
| SeamlyCLI source | Private repo |
| `packaging/licenses/about.toml`, `rust_crate_notices.hbs` | Private repo |
| Qt frontend C++ and QML | Public repo (MIT) |
| Generated headers | Public: `src/app/seamlylayout/bridge_generated/include/` |
| Core version, per-platform URL, SHA-256 | Public: `src/app/seamlylayout/bridge_generated/core_release.cmake` |
| Prebuilt package (lib, CMake config, notices, `seamlycli.exe`) | Private CI builds it; `FashionFreedom/Seamly2D` releases serve it |

## Public build modes

- Package mode (default): CMake `FetchContent` downloads the pinned zip with `URL_HASH SHA256`, then `find_package(SeamlyLayoutCore CONFIG REQUIRED)`.
- Source mode: `-DSEAMLYLAYOUT_CORE_SOURCE_DIR=<private checkout>` runs the current `cxx_qt_import_crate` path. Sue edits Rust and frontend together in this mode.
- Both modes define target `cxxqt_bridge`. `target_link_libraries(SeamlyLayout ...)` stays unchanged.

## Decisions

User decisions, 2026-11-17:

| # | Decision | Choice |
| --- | --- | --- |
| D1 | Scope | SeamlyLayout crates and SeamlyCLI |
| D2 | Public history | Rewrite `origin` and ask GitHub Support to purge. Last step, after the new build passes |
| D3 | Private repo | New repo under `seamly`. Archive the stale `seamly/seamlyLayout` |
| D4 | Package host | `FashionFreedom/Seamly2D` releases |
| D5 | Licensor | Seamly Systems, Inc. Proprietary, free redistribution |
| D6 | Upstream | `FashionFreedom/Seamly2D` is the public repo. It gets the frontend, generated headers, and download hook |

- D2 reason: current algorithms are the main asset, and exposure is low. Cost: rewrite breaks every clone and `.worktrees/` checkout.
- D4 constraints: tag `seamlylayout-core-v<version>`; create with `--latest=false` so weekly app releases stay Latest; never delete a core release, because old builds pin it.
- D5 reason: "Seamly2D Project" is not a legal entity. A licensor must be one.

## Task PrivateRust.0 — Stop-gap

- [ ] PrivateRust.0.1 Keep new Rust commits off `run-seamlyLayout` until PrivateRust.5 lands. Each push of `run-seamlyLayout` publishes them.

## Task PrivateRust.1 — License groundwork

- [x] PrivateRust.1.1 Record D1–D6 in this file.
- [ ] PrivateRust.1.2 Counsel check: Seamly2D (GPL) and SeamlyLayout (proprietary core) ship in one installer and talk only through `--svg-stdin` and command-line arguments. No linking.
- [ ] PrivateRust.1.3 Sue assigns, in writing, her copyright in the Rust code to Seamly Systems, Inc. Counsel confirms the form.
- [ ] PrivateRust.1.4 Write the binary license, licensor Seamly Systems, Inc.: redistribution, linking with the MIT frontend, inclusion in Seamly2D installers.
- [ ] PrivateRust.1.5 Private repo: proprietary `LICENSE` (Copyright 2026 Seamly Systems, Inc.), Rust file headers, `publish = false`. Every `Cargo.toml`, SeamlyCLI's `GPL-3.0-or-later` too, uses `license-file`.
- [ ] PrivateRust.1.6 `about.toml`: elect MIT or Apache-2.0 for `r-efi`, `hershey`, `self_cell`. Add the MPL-2.0 source pointer for `resvg`, `usvg`.

## Task PrivateRust.2 — Private repository

- [ ] PrivateRust.2.1 Create the private repo under `seamly` (D3). Archive `seamly/seamlyLayout`.
- [ ] PrivateRust.2.2 In a fresh clone, not the shared working tree, keep the Rust history: `git filter-repo --path src/app/seamlylayout/crates/ --path src/app/seamlylayout/Cargo.toml --path src/app/seamlylayout/Cargo.lock --path src/app/seamlycli/ --path packaging/licenses/about.toml --path packaging/licenses/rust_crate_notices.hbs`. Push it to the private repo.
- [ ] PrivateRust.2.3 Root `Cargo.toml` workspace. `cargo test --workspace` and SeamlyCLI `cargo test --release --locked` pass.
- [ ] PrivateRust.2.4 Move Rust docs and agent rules. List first, then move:
  - `.claude/rules/seamlylayout_rust-style.md`, `seamlylayout_ffi-bridge.md`, `seamlylayout-architecture.md`, `seamlylayout_dependencies.md`, `seamlylayout_guidelines_*.md`, `seamlylayout_svg-processing.md`, Rust parts of `seamlylayout_testing.md`.
  - `project-docs/docs-seamlylayout/dxf-docs/` files with Rust snippets (5 files).
  - Rust design notes in other `project-docs/docs-seamlylayout/` files.
- [ ] PrivateRust.2.5 Add `CLAUDE.md`, rules, and `.claude/agents/seamlylayout-developer.md` to the private repo. Its file header rule: Author `slspencer`, Copyright `2026 Seamly Systems, Inc.`, license `Proprietary`.

## Task PrivateRust.3 — Private CI and release package

- [ ] PrivateRust.3.1 Build the `cxxqt_bridge` release staticlib and SeamlyCLI per target, Qt 6.11.3, `QMAKE` set:

| Target | Runner | Constraint |
| --- | --- | --- |
| windows-x64 | Windows, MSVC | Toolset not newer than public builders; `/MD` runtime |
| linux-x64 | `ubuntu-22.04` | Same glibc and GCC as the AppImage build |
| macos-universal | `macos-26` | `lipo` x86_64 + arm64; `MACOSX_DEPLOYMENT_TARGET` = DMG minimum |
| windows-arm64 | later | No arm64 MSI yet |

- [ ] PrivateRust.3.2 Package profile: `lto = "fat"`, `codegen-units = 1`, `debug = false`, `--remap-path-prefix`. Strip local symbols. Check with `nm` / `dumpbin /symbols`: no crate-internal names.
- [ ] PrivateRust.3.3 Package contents:
  - lib;
  - generated headers and their hash;
  - `SeamlyLayoutCoreConfig.cmake`: imported target, link libraries from `cargo rustc -- --print native-static-libs`, Qt version used;
  - `rust_crate_notices.txt`;
  - `seamlycli.exe` (windows-x64);
  - `LICENSE`, `SHA256SUMS`.
- [ ] PrivateRust.3.4 `cargo test --workspace` and SeamlyCLI tests gate the release.
- [ ] PrivateRust.3.5 Tag `core-v<version>` in the private repo starts the release. CI runs `gh release create seamlylayout-core-v<version> --repo FashionFreedom/Seamly2D --latest=false` and uploads the zips.
- [ ] PrivateRust.3.6 Token: fine-grained PAT or GitHub App, `FashionFreedom/Seamly2D` only, `contents: write`. Store it as a private repo secret.

## Task PrivateRust.4 — Public build switch

- [ ] PrivateRust.4.1 Add `src/app/seamlylayout/bridge_generated/`: headers, `core_release.cmake`, `README.md` (headers MIT; `cxx` and `cxx-qt` headers keep their MIT/Apache notices).
- [ ] PrivateRust.4.2 `qt_frontend/CMakeLists.txt`: add package mode and source mode. Package mode does not fetch `cxx-qt-cmake`.
- [ ] PrivateRust.4.3 Configure checks:
  - Qt version ≥ package Qt version.
  - Package headers hash = committed headers hash.
  - Source mode: generated headers = committed headers.
- [ ] PrivateRust.4.4 Add `packaging/update_layout_core.ps1` and `.sh`: download a version, verify SHA-256, copy headers, write `core_release.cmake`. Sue runs it and commits.
- [ ] PrivateRust.4.5 `local_build_msi.ps1`, `local_build_appimage.sh`, `local_build_dmg.sh`: remove `cargo` steps. Take `seamlycli.exe` and Rust notices from the package. Source mode also runs the private `cargo test`.
- [ ] PrivateRust.4.6 `ci-layout.yml`: remove `dtolnay/rust-toolchain`, `Swatinem/rust-cache`, `cargo` steps, SeamlyCLI build. Pass the package directory to `smsi.ps1 -SeamlyCLIBin`.
- [ ] PrivateRust.4.7 `generate_license_notices.py`: remove the cargo-about step. Copy `rust_crate_notices.txt` from the package.
- [ ] PrivateRust.4.8 ctest smoke test: `SeamlyLayout --info <fixture.svg>` exits 0. It proves the prebuilt lib links and runs.

## Task PrivateRust.5 — Remove Rust from the public tree

- [ ] PrivateRust.5.1 Delete `src/app/seamlylayout/crates/`, `src/app/seamlylayout/Cargo.toml`, `src/app/seamlylayout/Cargo.lock`, `src/app/seamlycli/`, `packaging/licenses/about.toml`, `packaging/licenses/rust_crate_notices.hbs`.
- [ ] PrivateRust.5.2 `.gitignore`: `*.rs`, `Cargo.toml`, `Cargo.lock`, `target/` under `src/app/seamlylayout/` and `src/app/seamlycli/`.
- [ ] PrivateRust.5.3 Guard: a `ci-layout.yml` step and a local pre-commit hook fail when `git ls-files '*.rs' '*Cargo.toml' '*Cargo.lock'` is not empty.
- [ ] PrivateRust.5.4 Update:
  - `CLAUDE.md`: apps table license, Build Rules, Task Workflow verify step.
  - `AGENTS.md`, `.claude/rules/seamlylayout.md`, `.claude/rules/seamlylayout_licensing.md`.
  - `.github/README-BUILDS.md`, `.github/workflows/README_WORKFLOWS.md`, `packaging/windows/README_WINDOWS_BUILD.md`.
  - `project-docs/docs-seamlylayout/testing-docs/seamlylayout_UNIT_TEST_COMMANDS.md`, `project-docs/docs-seamlylayout/README._SEAMLYLAYOUT.md`.
- [ ] PrivateRust.5.5 Sue deletes the two stale remote branches.
- [ ] PrivateRust.5.6 Delete the `Swatinem/rust-cache` entries on `seamly/Seamly2D` (`gh cache delete`).

## Task PrivateRust.6 — Public history rewrite

- [ ] PrivateRust.6.1 Start only after PrivateRust.7.1 and 7.2 pass.
- [ ] PrivateRust.6.2 Fresh `--mirror` clone. `git filter-repo --invert-paths` with the PrivateRust.2.2 paths. Optional: `--replace-text` for Rust snippets in docs.
- [ ] PrivateRust.6.3 Check: `git log --all --oneline -- '*.rs' '*Cargo.toml' '*Cargo.lock'` is empty.
- [ ] PrivateRust.6.4 Sue force-pushes all branches and tags. Every clone and `.worktrees/` checkout re-clones.
- [ ] PrivateRust.6.5 Sue, as `FashionFreedom/Seamly2D` maintainer, asks GitHub Support to purge cached views and unreachable objects in the fork network. Until then old SHAs stay readable, also through `FashionFreedom/Seamly2D`.
- [ ] PrivateRust.6.6 Before the endgame push to `FashionFreedom:run-seamlyLayout`, repeat the 6.3 check on the pushed branch.

## Task PrivateRust.7 — Verify

- [ ] PrivateRust.7.1 Windows: fresh public clone, no Rust on PATH. `local_build_msi.ps1` passes. Install the MSI. SeamlyLayout imports a handoff, creates a layout, exports DXF and PDF. SeamlyCLI starts.
- [ ] PrivateRust.7.2 `gh workflow run ci-layout.yml`: Linux and macOS jobs pass without Rust.
- [ ] PrivateRust.7.3 Source mode: a Rust edit in the private checkout shows in the next build.
- [ ] PrivateRust.7.4 Symbol audit of the shipped libs passes (PrivateRust.3.2).
- [ ] PrivateRust.7.5 `git ls-files '*.rs'` in the public repo is empty.

## Order

PrivateRust.0 → 1 → 2 → 3 → 4 → 5 → 7 → 6.

## Costs and risks

- The prebuilt lib pins public builders: Qt ≥ 6.11.3, MSVC toolset ≥ private build, glibc and GCC ≥ `ubuntu-22.04`. A Qt upgrade needs a new core release.
- A bridge API change takes two steps: core release, then public pin update.
- Public builds cannot debug Rust. Use source mode.
- A binary slows reverse engineering. It does not stop it.
- Outside contributors cannot change Rust behavior.
- Linux distributions can refuse a proprietary binary.
