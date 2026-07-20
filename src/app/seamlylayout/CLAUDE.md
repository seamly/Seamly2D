# SeamlyLayout

A pattern layout application — daughter app to Seamly2D. Lives at `src/app/seamlylayout/` in the Seamly2D repository (tracked directly, like seamlyme) with its own Rust + Qt build, outside the Seamly2D qmake build.

- **Author:** slspencer
- **Copyright:** 2026
- **Prompts are requirements, not suggestions.**

## Architecture

- **Frontend:** Qt 6.10 + QML and QtWidgets (LGPL-3.0)
- **Core:** Rust crates under `crates/` (MIT)
- **Bridge:** CXX-Qt 0.7.3 — generates C++ glue from Rust
- **Dual license:** Qt frontend LGPL-3.0, Rust core MIT (de-linkable)

## Key Conventions

- Always use absolute file paths, never relative paths
- Use "flatten" only for baking-in transforms; use "interpolation" for converting curves to polylines
- SVG processing must use xmltree/svg_dom — never regex or string for SVG manipulation (the exception is in cxxbridge_qt files --> they cannot pass dom trees to the gui so they must convert dom to string)
- No new regex or string manipulation — use proper SVG and XML parsing libraries that update the dom XML tree
- Read docs in `docs/` regularly and update them to reflect code changes
- **UI work must consult `crates/cxxqt_bridge/` and `qt_frontend/`.** The active UI is QML in `qt_frontend/qml/` with the Rust↔Qt bridge in `crates/cxxqt_bridge/`.

## Platform Support

Windows 11, Linux (all flavors), macOS (latest 3 versions)

## Build Commands

- Qt debug build + launch from the app root (`src/app/seamlylayout/` in the Seamly2D repo): `qd.ps1`

## Search & Update Policy

- When making changes, always search and update all code files ending in `.rs`, `.cpp`, `.cxx.cpp`, `.h`, and `.qml`.
- Always search and update all text files ending in `.md` and `.txt`.

## Rules

Detailed guidelines are in `.claude/rules/`:

- [branding.mdc](./.claude/rules/branding.mdc) — Color palette and UI styling
- [dependencies.mdc](./.claude/rules/dependencies.mdc) — Crate versions, workspace structure, Qt modules
- [ffi-bridge.mdc](./.claude/rules/ffi-bridge.mdc) — extern "C" conventions, memory ownership, error codes
- [licensing.mdc](./.claude/rules/licensing.mdc) — License requirements: Qt LGPL-3.0, Rust MIT
- [qt-style.mdc](./.claude/rules/qt-style.mdc) — C++/QML coding conventions and file headers
- [rust-style.mdc](./.claude/rules/rust-style.mdc) — Rust coding conventions and file headers
- [svg-processing.mdc](./.claude/rules/svg-processing.mdc) — SVG/DOM manipulation guidelines
- [testing.mdc](./.claude/rules/testing.mdc) — Testing frameworks and commands

## Workflow Guidelines

- [Guidelines_Export_DXF.mdc](./.claude/rules/Guidelines_Export_DXF.mdc) — DXF export pipeline
- [Guidelines_Layout.mdc](./.claude/rules/Guidelines_Layout.mdc) — Layout processing pipeline
- [Guidelines_Settings.mdc](./.claude/rules/Guidelines_Settings.mdc) — Settings workflow and defaults
- [Guidelines_Tiling.mdc](./.claude/rules/Guidelines_Tiling.mdc) — Tiling calculation and reduction

## Seamly2D Family

| Application            | Role                       | UI Framework       |
| ---------------------- | -------------------------- | ------------------ |
| Seamly2D               | Parent — pattern drafting | Qt/QtWidgets       |
| SeamlyMe               | Daughter — measurements   | Qt/QtWidgets       |
| **SeamlyLayout** | Daughter — pattern layout | Qt/QML + QtWidgets |
