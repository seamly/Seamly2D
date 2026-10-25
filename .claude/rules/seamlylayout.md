---
paths:
  - "src/app/seamlylayout/**"
  - "src/test/SeamlyLayoutTest/**"
  - "project-docs/docs-seamlylayout/**"
---

# SeamlyLayout Rules

## General Rules

- Before you change an area, grep `project-docs/docs-seamlylayout/` for its doc. Read only the matching section.
- Update docs to reflect code changes.
- Use "flatten" only for baking in transforms. Use "interpolation" for converting curves to polylines.
- Always use absolute file paths, never relative paths. Resolve via `QFileInfo::absoluteFilePath()` (C++) or `std::path::Path::canonicalize()` (Rust).
- UI work must consult `crates/cxxqt_bridge/` and `qt_frontend/`. The active UI is QML in `qt_frontend/qml/`, with the Rust↔Qt bridge in `crates/cxxqt_bridge/`.
- Code changes: search and update `.rs`, `.cpp`, `.cxx.cpp`, `.h`, and `.qml` files.
- Text changes: search and update `.md` and `.txt` files.
- Ignore markdown linting errors (MD001, MD004, MD013, etc.) in SeamlyLayout docs.
- Move as much application functionality as possible into `.rs` files.
- Add Doxygen briefs to all functions, methods, wrappers, controllers.
- Add inline comments to all new code to describe workflow, control flow, and data flow, so an intermediate-level programmer can follow it.

## Regex Policy

- Do not introduce new regex in SeamlyLayout code. Use proper parsing libraries instead.
- Existing regex in Rust code is acceptable.
- SVG processing must use `xmltree`/`svg_dom`, never regex, for SVG manipulation.

## Detailed Rule Files

Read the file for the area you change. They do not load automatically (`.mdc`).

- [seamlylayout-architecture.mdc](seamlylayout-architecture.mdc) — frontend/core/bridge boundaries
- [seamlylayout_dependencies.mdc](seamlylayout_dependencies.mdc) — crate versions, workspace structure, Qt modules
- [seamlylayout_ffi-bridge.mdc](seamlylayout_ffi-bridge.mdc) — `extern "C"` conventions, memory ownership, error codes
- [seamlylayout_licensing.mdc](seamlylayout_licensing.mdc) — Qt LGPL-3.0, Rust MIT
- [seamlylayout_qt-style.mdc](seamlylayout_qt-style.mdc) — C++/QML conventions and file headers
- [seamlylayout_rust-style.mdc](seamlylayout_rust-style.mdc) — Rust conventions and file headers
- [seamlylayout_svg-processing.mdc](seamlylayout_svg-processing.mdc) — SVG/DOM manipulation
- [seamlylayout_testing.mdc](seamlylayout_testing.mdc) — test frameworks and commands
- [seamlylayout_guidelines_export_dxf.mdc](seamlylayout_guidelines_export_dxf.mdc) — DXF export pipeline
- [seamlylayout_guidelines_layout.mdc](seamlylayout_guidelines_layout.mdc) — layout pipeline
- [seamlylayout_guidelines_settings.mdc](seamlylayout_guidelines_settings.mdc) — settings workflow and defaults
- [seamlylayout_guidelines_tiling.mdc](seamlylayout_guidelines_tiling.mdc) — tiling calculation and reduction

No branding rule file exists.
