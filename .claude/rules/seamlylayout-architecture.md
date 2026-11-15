---
paths:
  - "src/app/seamlylayout/**"
---

# Architecture Rules

Author: slspencer
Copyright: 2026

## Scope

Applies to architecture-level decisions and cross-module design.

## Rules

- Keep frontend concerns in `qt_frontend/` and core logic in `crates/`.
- Prefer Rust for business logic and data transformations.
- Keep bridge boundaries explicit and minimal (`cxxqt_bridge` only).
- Preserve clear ownership of data across Rust ↔ Qt boundaries.
- Favor composable modules over monolithic utility files.
- Canvases get SVG text through `CanvasSvgSchemeHandler` (`seamly-canvas://` URLs, from memory).
  - Never put SVG in a `data:` URL. Chromium refuses a URL over 2 MB, and large patterns go blank.
  - Never write a temp file to display an SVG.
  - Workflow: `project-docs/docs-seamlylayout/README._SEAMLYLAYOUT.md`, "Canvas display".

## Repository-Specific Notes

- Frontend: Qt 6.11 + QML/QtWidgets (LGPL-3.0).
- Core: Rust crates (MIT).
- Bridge: CXX-Qt.
