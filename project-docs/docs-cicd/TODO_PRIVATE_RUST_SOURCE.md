<!-- Author: slspencer | Copyright 2026 Seamly2D Project | License: MIT -->
# TODO — Private Rust Source

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file are numbered with prefix `PrivateRust.`

## Goal

- Public repository: no Rust source.
- `seamlylayout.exe` holds only open-source code: Qt frontend (C++, QML).
- Proprietary Rust ships as separate files: the `seamlylayout_core` shared library and `seamlycli.exe`.
- Public repository keeps: Qt frontend, C header `seamlylayout_core.h`, pinned download of the prebuilt core, fonts in `src/fonts/`.
- Private repository: all Rust source, Rust tests, Rust build and release.

## Facts (checked 2026-11-17)

| Rust code | `.rs` files | License now | Public on `origin` since |
| --- | --- | --- | --- |
| `src/app/seamlylayout/crates/` (15 crates), `Cargo.toml`, `Cargo.lock` | 85 | MIT | 2026-07-19 |
| `src/app/seamlycli/` (Rust + Iced) | 10 | GPL-3.0-or-later | 2026-11-16 |

- Total: 50,677 lines of Rust.
- Sole author: slspencer. One Dependabot version bump. Copyright assigned to Seamly Systems, Inc.
- Sue maintains `FashionFreedom/Seamly2D`.
- Upstream releases: weekly tag `v<date>.<n>`, marked Latest. Upstream workflows have no tag or release triggers.
- Relicensing does not recall published copies. A holder of a published version keeps its MIT or GPL grant.
- `seamly/Seamly2D` is a public fork of `FashionFreedom/Seamly2D`. The fork network shares objects: any pushed commit is readable by SHA through every repository in it.
- Exposure: 1 fork (`elros88`, 2024, no Rust), 8 stars, 116 clones / 29 unique in 14 days (CI checkouts count).
- `upstream/develop` holds no Rust.
- Today the Rust links statically into `seamlylayout.exe`: crate `cxxqt_bridge` is a `staticlib` (release `.lib`: 85 MB). The staged exe holds source paths of 12 crates.
- Bridge: one QObject, `AppController`. All 45 methods are `Q_INVOKABLE`. It has 11 signals.
- Frontend use of `AppController`:
  - `qt_frontend/seamlyLayout_main.mm`: `qmlRegisterType<AppController>`.
  - `qt_frontend/src/HeadlessExport.cpp`: includes `cxxqt_bridge/src/lib.cxxqt.h`; calls 33 methods; connects 3 signals.
  - QML: uses only properties, invokables, and signals.
- So the frontend can reach `AppController` through `QMetaObject` alone. No generated header is needed outside the core library.
- Generated QML: none. Runtime `qmlRegisterType` replaces `qmldir`/`qmltypes`.
- LGPLv3: the core library and the exe link Qt dynamically. No LGPL duty requires Rust source or generated code.
- Fonts inside the crates, embedded with `include_bytes!` / `include_str!`:
  - `crates/svg_label_text/assets/fonts/ReliefSingleLineOutline-Regular.otf`, `OFL.txt` (used in `font_embedding.rs:31`, `svg_label_text_tests.rs:11`).
  - `crates/svg_label_text/assets/hershey/futural.jhf`, `HERSHEY_LICENSE.txt` (used in `hershey_strokes.rs:22`).
- Test SVGs read with `include_str!` sit in `#[cfg(test)]` modules. They are not in the release exe.
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
| CXX-Qt generated code, export file of the core library | Private repo |
| `packaging/licenses/about.toml`, `rust_crate_notices.hbs` | Private repo |
| Qt frontend C++ and QML | Public repo (MIT) |
| `seamlylayout_core.h` (C API) | Public: `src/app/seamlylayout/core_release/` |
| Core version, per-platform URL, SHA-256 | Public: `src/app/seamlylayout/core_release/core_release.cmake` |
| Relief and Hershey fonts with licenses | Public: `src/fonts/`; compiled into `seamlylayout.exe` as Qt resources |
| Prebuilt package (core library, CMake config, notices, `seamlycli.exe`) | Private CI builds it; `FashionFreedom/Seamly2D` releases serve it |

## Core library

- Private CMake target `seamlylayout_core`, `SHARED`: the `cxxqt_bridge` staticlib plus one C++ export file.
- Files: `seamlylayout_core.dll` (Windows), `libseamlylayout_core.so` (Linux), `libseamlylayout_core.dylib` (macOS).
- It links Qt Core and Qt Qml dynamically.
- Exported symbols, and no others:

| Function | Purpose |
| --- | --- |
| `int seamlylayout_core_api_version()` | Frontend refuses a library with another API version |
| `void seamlylayout_core_register_qml_types()` | Calls `qmlRegisterType<AppController>("SeamlyLayout", 1, 0, "AppController")` |
| `QObject* seamlylayout_core_create_controller(QObject* parent)` | Gives `HeadlessExport` an `AppController` |
| `int seamlylayout_core_set_label_fonts(const uint8_t* relief_otf, size_t relief_size, const uint8_t* hershey_jhf, size_t hershey_size)` | Gives the core the label fonts from the exe's Qt resources. The core copies the bytes. Returns 0, or an error code for invalid font data |

- Symbol control: Windows `__declspec(dllexport)`; Linux `-fvisibility=hidden` and a version script; macOS `-exported_symbols_list`.
- Windows: library and exe both use the `/MD` runtime, so a `QObject` made in one can be deleted in the other.

## Public build modes

- Package mode (default): CMake `FetchContent` downloads the pinned zip with `URL_HASH SHA256`, then `find_package(SeamlyLayoutCore CONFIG REQUIRED)`.
- Source mode: `-DSEAMLYLAYOUT_CORE_SOURCE_DIR=<private checkout>` builds the core library from source. Sue edits Rust and frontend together in this mode.
- Both modes give imported target `SeamlyLayoutCore::seamlylayout_core`.

## Decisions

User decisions, 2026-11-17:

| # | Decision | Choice |
| --- | --- | --- |
| D1 | Scope | SeamlyLayout crates and SeamlyCLI |
| D2 | Public history | Rewrite `origin` and ask GitHub Support to purge. Last step, after the new build passes |
| D3 | Private repo | New repo under `seamly`. Archive the stale `seamly/seamlyLayout` |
| D4 | Package host | `FashionFreedom/Seamly2D` releases |
| D5 | Licensor | Seamly Systems, Inc. Proprietary, free redistribution |
| D6 | Upstream | `FashionFreedom/Seamly2D` is the public repo. It gets the frontend, `seamlylayout_core.h`, fonts, and download hook |
| D7 | Separation | Rust core in its own shared library, not linked into `seamlylayout.exe` |
| D8 | Fonts | Move the crate fonts to `src/fonts/` |
| D9 | Font delivery | Qt resources in `seamlylayout.exe`. The frontend passes the font bytes to the core once at startup |

- D2 reason: current algorithms are the main asset, and exposure is low. Cost: rewrite breaks every clone and `.worktrees/` checkout.
- D4 constraints: tag `seamlylayout-core-v<version>`; create with `--latest=false` so weekly app releases stay Latest; never delete a core release, because old builds pin it.
- D5 reason: "Seamly2D Project" is not a legal entity. A licensor must be one.

- D9 reason: the proprietary library holds no third-party fonts. No font files to install, so no font path per platform. Cost: one more exported function.

## Task PrivateRust.0 — Stop-gap

- [ ] PrivateRust.0.1 Keep new Rust commits off `run-seamlyLayout` until PrivateRust.5 lands. Each push of `run-seamlyLayout` publishes them.
  - User decision 2026-11-18: Rust commits wait on local branches until the split. Do not merge or delete those branches.
  - A local branch has no backup until the private repo exists (PrivateRust.2.1).

## Task PrivateRust.1 — License groundwork

- [x] PrivateRust.1.1 Record D1–D6 in this file.
- [ ] PrivateRust.1.2 Counsel check: Seamly2D (GPL) and SeamlyLayout (proprietary core) ship in one installer and talk only through `--svg-stdin` and command-line arguments. No linking.
  - 2026-11-17: counsel answered yes. Stated basis: all code on the user's PC is open source (Qt, C++).
  - Open: with D7, `seamlylayout.exe` is open source only. Proprietary code still runs on the user's PC: `seamlylayout_core` (loaded into the SeamlyLayout process) and `seamlycli.exe`. Counsel re-confirms with this fact.
- [x] PrivateRust.1.3 Sue assigns, in writing, her copyright in the Rust code to Seamly Systems, Inc. Done 2026-11-17.
- [ ] PrivateRust.1.4 Write the binary license, licensor Seamly Systems, Inc.: redistribution, use by the MIT frontend, inclusion in Seamly2D installers.
- [ ] PrivateRust.1.5 Private repo: proprietary `LICENSE` (Copyright 2026 Seamly Systems, Inc.), Rust file headers, `publish = false`. Every `Cargo.toml`, SeamlyCLI's `GPL-3.0-or-later` too, uses `license-file`.
- [ ] PrivateRust.1.6 `about.toml`: elect MIT or Apache-2.0 for `r-efi`, `hershey`, `self_cell`. Add the MPL-2.0 source pointer for `resvg`, `usvg`.

## Task PrivateRust.8 — Move crate fonts to `src/fonts/`

Work on local branch `task-move-label-fonts` (PrivateRust.0.1). At the split:

- Rust changes go to the private repo after PrivateRust.2.2.
- The public part (`src/fonts/`, Qt resource, `seamlylayout_core.h`, `seamlyLayout_main.mm`, docs) merges into `run-seamlyLayout` with PrivateRust.4.
- It lands before PrivateRust.5.1 deletes `crates/`, so the fonts stay public.

- [x] PrivateRust.8.1 Decide how the Rust gets the fonts: D9, Qt resources in `seamlylayout.exe` (user decision 2026-11-17).
- [ ] PrivateRust.8.2 `git mv` the fonts and their licenses:
  - `crates/svg_label_text/assets/fonts/ReliefSingleLineOutline-Regular.otf`, `OFL.txt` → `src/fonts/relief_single_line/`.
  - `crates/svg_label_text/assets/hershey/futural.jhf`, `HERSHEY_LICENSE.txt` → `src/fonts/hershey/`.
- [ ] PrivateRust.8.3 `svg_label_text`: replace the `include_bytes!` / `include_str!` constants (`font_embedding.rs:31`, `hershey_strokes.rs:22`) with fonts set once at run time. Label text before the fonts are set gives an error.
- [ ] PrivateRust.8.4 `cxxqt_bridge`: add `extern "C"` `seamlylayout_core_set_label_fonts` (see Core library). It copies the bytes and checks that both fonts parse. Declare it in a new public `src/app/seamlylayout/core_release/seamlylayout_core.h`.
- [ ] PrivateRust.8.5 Frontend: add the two font files to a Qt resource under `/fonts`. `seamlyLayout_main.mm` reads them and calls `seamlylayout_core_set_label_fonts` before the QML engine loads and before a headless export. A failure gives an error and a nonzero exit.
- [ ] PrivateRust.8.6 Unit tests: label text before the fonts are set gives an error; invalid font bytes give an error.
- [ ] PrivateRust.8.7 Rust tests read the fonts from `src/fonts/` through a path from `CARGO_MANIFEST_DIR`. After the split, the private repo keeps a test copy.
- [ ] PrivateRust.8.8 Check that the MSI, AppImage, and DMG still ship `relief_singleline_OFL.txt` and `hershey_fonts_notice.txt` from `packaging/licenses/`.
- [ ] PrivateRust.8.9 Update the font paths in `.claude/rules/seamlylayout_licensing.md` (asset table: add `src/fonts/`) and `project-docs/docs-seamlylayout/status-docs/seamlylayout_SEAMLYLAYOUT_DECISIONS.md`.

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

- [ ] PrivateRust.3.1 Add the `seamlylayout_core` shared library target and its export file (see Core library).
- [ ] PrivateRust.3.2 Build the core library and SeamlyCLI per target, Qt 6.11.3, `QMAKE` set:

| Target | Runner | Constraint |
| --- | --- | --- |
| windows-x64 | Windows, MSVC | `/MD` runtime |
| linux-x64 | `ubuntu-22.04` | Same glibc and GCC as the AppImage build |
| macos-universal | `macos-26` | `lipo` x86_64 + arm64; `MACOSX_DEPLOYMENT_TARGET` = DMG minimum; `@rpath` install name |
| windows-arm64 | later | No arm64 MSI yet |

- [ ] PrivateRust.3.3 Release profile: `lto = "fat"`, `codegen-units = 1`, `debug = false`, `--remap-path-prefix`. Strip the shared library.
- [ ] PrivateRust.3.4 Symbol audit: only the four exported functions are visible (`dumpbin /exports`, `nm -D --defined-only`, `nm -gU`).
- [ ] PrivateRust.3.5 Package contents:
  - core library; Windows import `.lib`;
  - `seamlylayout_core.h`;
  - `SeamlyLayoutCoreConfig.cmake`: imported target, API version, Qt version used;
  - `rust_crate_notices.txt`;
  - `seamlycli.exe` (windows-x64);
  - `LICENSE`, `SHA256SUMS`.
- [ ] PrivateRust.3.6 `cargo test --workspace`, SeamlyCLI tests, and a load test of the core library gate the release.
- [ ] PrivateRust.3.7 Tag `core-v<version>` in the private repo starts the release. CI runs `gh release create seamlylayout-core-v<version> --repo FashionFreedom/Seamly2D --latest=false` and uploads the zips.
- [ ] PrivateRust.3.8 Token: fine-grained PAT or GitHub App, `FashionFreedom/Seamly2D` only, `contents: write`. Store it as a private repo secret.
- [ ] PrivateRust.3.9 Windows: private CI signs `seamlylayout_core.dll` and `seamlycli.exe` before packaging. Same certificate and timestamp as `TODO_CODE_SIGNING.md` CodeSign.1.

## Task PrivateRust.4 — Public build switch

- [ ] PrivateRust.4.1 Add `src/app/seamlylayout/core_release/`: `seamlylayout_core.h`, `core_release.cmake`.
- [ ] PrivateRust.4.2 `seamlyLayout_main.mm`: replace `qmlRegisterType<AppController>` with `seamlylayout_core_register_qml_types()`. Check `seamlylayout_core_api_version()` first.
- [ ] PrivateRust.4.3 `HeadlessExport.cpp`: remove the `lib.cxxqt.h` include. Get the controller from `seamlylayout_core_create_controller()`. Call the 33 methods with `QMetaObject::invokeMethod`. Connect the 3 signals by name.
- [ ] PrivateRust.4.4 Startup check: every method and signal name the frontend uses exists in the controller's `QMetaObject`. A missing name gives an error that names it.
- [ ] PrivateRust.4.5 `qt_frontend/CMakeLists.txt`: package mode and source mode. Remove `FetchContent(CxxQt)` and `cxx_qt_import_crate` from package mode. Link `SeamlyLayoutCore::seamlylayout_core`.
- [ ] PrivateRust.4.6 Configure checks: Qt version ≥ package Qt version; package `seamlylayout_core.h` hash = committed header hash.
- [ ] PrivateRust.4.7 Add `packaging/update_layout_core.ps1` and `.sh`: download a version, verify SHA-256, copy `seamlylayout_core.h`, write `core_release.cmake`. Sue runs it and commits.
- [ ] PrivateRust.4.8 Stage the core library beside `seamlylayout.exe`: MSI component (`smsi.ps1`, WiX), AppImage `usr/lib`, DMG `Contents/Frameworks`.
- [ ] PrivateRust.4.9 `local_build_msi.ps1`, `local_build_appimage.sh`, `local_build_dmg.sh`: remove `cargo` steps. Take the core library, `seamlycli.exe`, and Rust notices from the package. Source mode also runs the private `cargo test`.
- [ ] PrivateRust.4.10 `ci-layout.yml`: remove `dtolnay/rust-toolchain`, `Swatinem/rust-cache`, `cargo` steps, SeamlyCLI build. Pass the package directory to `smsi.ps1 -SeamlyCLIBin`.
- [ ] PrivateRust.4.11 `generate_license_notices.py`: remove the cargo-about step. Copy `rust_crate_notices.txt` from the package.
- [ ] PrivateRust.4.12 ctest smoke tests: `SeamlyLayout --info <fixture.svg>` exits 0; one headless export per format exits 0. They prove the library loads and every name-based call resolves.

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

- [ ] PrivateRust.7.1 Windows: fresh public clone, no Rust on PATH. `local_build_msi.ps1` passes. Install the MSI. SeamlyLayout imports a handoff, creates a layout, exports DXF and PDF with label text. SeamlyCLI starts.
- [ ] PrivateRust.7.2 `gh workflow run ci-layout.yml`: Linux and macOS jobs pass without Rust.
- [ ] PrivateRust.7.3 Source mode: a Rust edit in the private checkout shows in the next build.
- [ ] PrivateRust.7.4 `seamlylayout.exe` holds no Rust: no `crates\` source paths, no Rust symbols.
- [ ] PrivateRust.7.5 `git ls-files '*.rs'` in the public repo is empty.

## Order

PrivateRust.0 → 1 → 8 (local branch) → 2 → 3 → 4 (with the public part of 8) → 5 → 7 → 6.

## Costs and risks

- The prebuilt library pins public builders: Qt ≥ 6.11.3, glibc and GCC ≥ `ubuntu-22.04`. A Qt upgrade needs a new core release.
- Name-based calls lose compile-time checks. PrivateRust.4.4 and 4.12 catch a renamed or removed method.
- A bridge API change takes two steps: core release, then public pin update.
- Public builds cannot debug Rust. Use source mode.
- A binary slows reverse engineering. It does not stop it.
- Outside contributors cannot change Rust behavior.
- Linux distributions can refuse a proprietary binary.
