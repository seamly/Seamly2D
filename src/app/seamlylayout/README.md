<!-- Qt frontend: LGPL-3.0 (https://www.gnu.org/licenses/lgpl-3.0.html) | Rust core: MIT (https://opensource.org/licenses/MIT) -->
# SeamlyLayout

A pattern layout application — daughter app to Seamly2D. SeamlyLayout is a **Qt 6 / C++ / Rust** hybrid: a Qt 6.11 QML + QtWidgets frontend bound to a Rust workspace core through the CXX-Qt bridge.

- **Author:** slspencer
- **Copyright:** 2026

## Architecture

- **Frontend:** Qt 6.11 + QML + QtWidgets (C++), licensed LGPL-3.0. Lives in [qt_frontend/](qt_frontend/).
- **Core:** Rust crates under [crates/](crates/), licensed MIT.
- **Bridge:** [CXX-Qt](https://github.com/KDAB/cxx-qt) 0.7 generates C++ glue from Rust in [crates/cxxqt_bridge/](crates/cxxqt_bridge/).
- **Dual license:** the Qt frontend (LGPL-3.0) and Rust core (MIT) are de-linkable.

## Workspace layout

- [crates/app_core](crates/app_core/) — shared application logic and pipeline orchestration.
- [crates/svg_dom](crates/svg_dom/) — XML/SVG DOM helpers built atop `xmltree`.
- [crates/geometry](crates/geometry/) — geometry primitives (points, affine matrices, paths, bounding boxes).
- [crates/layout_engine](crates/layout_engine/) — placement and bin-packing heuristics.
- [crates/cxxqt_bridge](crates/cxxqt_bridge/) — Rust ↔ Qt bridge types exposed to QML.
- [crates/seamly_svg2ezdxf](crates/seamly_svg2ezdxf/) and [crates/ezdxf2dxfastm](crates/ezdxf2dxfastm/) — DXF export pipeline.
- [crates/cli](crates/cli/) — command-line entry point mirroring desktop features.
- [qt_frontend/](qt_frontend/) — Qt application, QML UI, CMake build, assets.
- [assets/](assets/) — shared fonts/icons; [docs/](docs/) — design notes, guidelines, and migration plans.

> **Note:** The active desktop UI is the Qt/QML frontend in [qt_frontend/qml/](qt_frontend/qml/) with Rust bridge code in [crates/cxxqt_bridge/](crates/cxxqt_bridge/).

## Prerequisites

- **Rust** (stable, 2024 edition ready): `rustup install stable && rustup default stable`.
- **Qt 6.11** with QML, QtWidgets, and Qt Quick modules.
- **CMake** ≥ 3.21 and a C++17 toolchain (MSVC on Windows, Clang/GCC on Linux/macOS).
- See [.claude/rules/dependencies.mdc](.claude/rules/dependencies.mdc) for the exact Qt module list and crate versions.

## Building

The Qt frontend drives the build and pulls the Rust core in through CXX-Qt.

Windows:

```bat
./run_app.bat            REM release build + launch
./run_app_debug.bat      REM debug build + launch
```

From [qt_frontend/](qt_frontend/) (any platform):

```powershell
./qd.ps1                 # shorthand Qt debug build + launch
./build.ps1              # configure + build via CMake
./run_release.ps1        # run the release binary
./run_debug.ps1          # run the debug binary
```

Rust-only checks:

- `cargo build` — build all workspace crates.
- `cargo test` — run Rust tests (most coverage is in `geometry`).
- `cargo fmt && cargo clippy -- -D warnings` — format and lint.

Most crates need no Qt, but the workspace also contains `cxxqt_bridge`, whose
build script (`cxx-qt-build`) locates Qt through the **`QMAKE`** environment
variable, falling back to whatever bare `qmake` is first on `PATH`. Point it at
the same kit `build.ps1` uses before running a workspace-wide `cargo` command:

```powershell
$env:QMAKE = "C:\Qt\6.11.1\msvc2022_64\bin\qmake.exe"
cargo test --workspace
```

```bash
export QMAKE=/path/to/Qt/6.11.1/gcc_64/bin/qmake
cargo test --workspace
```

This matters on machines with **Qt Design Studio** installed: its bundled
`C:\Qt\Tools\QtDesignStudio\qt6_design_studio_reduced_version\bin\qmake.exe`
often comes first on `PATH` and is a stripped Qt with **no `mkspecs`
directory**, so the build fails naming that path instead of the real kit.
`build.ps1` exports `QMAKE` and prepends its selected kit's `bin\` to `PATH`
(and refuses a Qt with no `mkspecs`), so builds driven through it are immune —
only bare `cargo` invocations in a plain shell need the export above.

## CLI

- `./run_cli.sh -- --help` — release-builds and forwards args to the CLI.
- `./run_cli_render.sh input.svg output.png` — render helper at fixed scale 1.0.

## Platform support

Windows 11, Linux (all flavors), macOS (latest 3 versions).

## Licensing

- Qt frontend (`.qml`, `.cpp`, `.h`): LGPL-3.0.
- Rust crates (`.rs`): MIT.
- See [.claude/rules/licensing.mdc](.claude/rules/licensing.mdc) for details.

## Contributing

Pull requests and issues are welcome. Keep changes small and tested (`cargo test`, plus a Qt build when touching the frontend). Follow the style rules in [.claude/rules/](.claude/rules/).

## Git workflow

This repository now uses a **main-first** workflow:

- `main` is the integration branch.
- Create short-lived feature/fix branches from `main`.
- Open pull requests targeting `main`.
- Merge via PR (no direct pushes to `main`).
- Delete merged feature branches after verification.

Long-lived branches currently retained by design:

- `main`
- `3D-mode`
- `knitting-mode`

Removed legacy integration branches (`develop`, `qt`) should not be recreated.
