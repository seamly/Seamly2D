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

Each file loads when you touch a path in its `paths` list. Read one by hand for work outside those paths.

- [seamlylayout-architecture.md](seamlylayout-architecture.md) — frontend/core/bridge boundaries
- [seamlylayout_dependencies.md](seamlylayout_dependencies.md) — crate versions, workspace structure, Qt modules
- [seamlylayout_ffi-bridge.md](seamlylayout_ffi-bridge.md) — `extern "C"` conventions, memory ownership, error codes
- [seamlylayout_licensing.md](seamlylayout_licensing.md) — Qt LGPL-3.0, Rust MIT
- [seamlylayout_qt-style.md](seamlylayout_qt-style.md) — C++/QML conventions and file headers
- [seamlylayout_rust-style.md](seamlylayout_rust-style.md) — Rust conventions and file headers
- [seamlylayout_svg-processing.md](seamlylayout_svg-processing.md) — SVG/DOM manipulation
- [seamlylayout_testing.md](seamlylayout_testing.md) — test frameworks and commands
- [seamlylayout_guidelines_export_dxf.md](seamlylayout_guidelines_export_dxf.md) — DXF export pipeline
- [seamlylayout_guidelines_layout.md](seamlylayout_guidelines_layout.md) — layout pipeline
- [seamlylayout_guidelines_settings.md](seamlylayout_guidelines_settings.md) — settings workflow and defaults
- [seamlylayout_guidelines_tiling.md](seamlylayout_guidelines_tiling.md) — tiling calculation and reduction

No branding rule file exists.
