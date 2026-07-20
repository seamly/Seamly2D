# SeamlyLayout Codebase Index

This document provides a high-level index of the main components, crates, and modules in the SeamlyLayout codebase as of May 2026.

---

## Top-Level Structure

- **crates/** — Rust core logic, organized as independent crates
- **qt_frontend/** — Qt/QML frontend, C++ glue, and assets
- **docs/** — Documentation (architecture, workflow, design, etc.)

---

## Rust Crates (`crates/`)

- **app_core/** — Core application logic
  - `src/lib.rs` — Main library file
- **cli/** — Command-line interface
  - `src/main.rs` — CLI entry point
- **cxxqt_bridge/** — Rust↔Qt bridge (CXX-Qt)
- **ezdxf2dxfastm/** — DXF conversion utilities
- **geometry/** — Geometric primitives and algorithms
- **layout_engine/** — Layout and packing engine
  - `src/lib.rs` — Main engine logic
- **layout_tiling/** — Tiling and reduction algorithms
- **pack_types/** — Data types for packing/layout
- **packing/** — Packing algorithms
- **polygon_pack/** — Polygon packing utilities
- **seamly_svg2ezdxf/** — SVG to DXF conversion
- **svg_dom/** — SVG DOM manipulation

---

## Qt Frontend (`qt_frontend/`)

- **src/** — C++ source files (platform, preferences, logging, etc.)
- **qml/** — QML UI components
  - `Main.qml` — Main application window
  - `SvgCanvas.qml` — SVG rendering canvas
  - `ExportMenu.qml`, `TopMenuBar.qml` — Menus
  - `PreferencesPanel.qml`, `SettingsDialog.qml` — Settings UI
  - `Theme.qml`, `SeamlyButton.qml` — Theming and controls

---

## Documentation (`docs/`)

- **adjust-docs/**, **branding-docs/**, **dxf-docs/**, etc. — Specialized documentation
- **general-docs/** — General workflow and architecture
- **qt-docs/**, **rust-docs/** — Language/framework-specific docs

---

## Build & Scripts

- `build.ps1`, `qd.ps1`, `qr.ps1`, `run_app.bat`, etc. — Build and run scripts

---

## Notes

- All Rust code is MIT licensed; Qt/C++/QML code is LGPL-3.0.
- See `.claude/rules/CLAUDE.md` for coding and workflow rules.

---

_Last updated: 2026-05-12_
