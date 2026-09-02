# SeamlyLayout

A pattern layout application — daughter app to Seamly2D. Lives at `src/app/seamlylayout/` in the Seamly2D repository (tracked directly, like seamlyme) with its own Rust + Qt build, outside the Seamly2D qmake build.

- **Author:** slspencer
- **Copyright:** 2026
- **Prompts are requirements, not suggestions.**

## Architecture

- **Frontend:** Qt 6.11 + QML and QtWidgets (LGPL-3.0)
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

## Command Line — the Seamly2D handoff contract (Task 49, revised by Seamly2D.5)

Seamly2D's Layout Mode builds the tagged pieces SVG in memory, launches this app with `--svg-stdin`, then writes the document to the new process's standard input and closes the channel. **No file is written** — the earlier `<pattern>.pieces.svg` handoff is gone. The contract is implemented in `qt_frontend/src/StartupOptions.{h,cpp}` and pinned by `src/test/SeamlyLayoutTest/StartupOptionsTests.cpp`; the producing half lives in `src/libs/vmisc/seamly_suite_paths.cpp` and is pinned by `TST_SeamlySuitePaths`. **Change one side and you must change the other** — see `project-docs/SVG-DATA-ATTRIBUTES.md` for the full statement.

| Invocation | Behaviour |
| ---------- | --------- |
| `SeamlyLayout` | Empty canvas — the double-clicked-icon case |
| `SeamlyLayout --svg-stdin [--document-name <name>]` | The Seamly2D handoff: reads the SVG document from standard input (`Main.qml`'s `openSvgDocument()` → `AppController::importSvgDocument`) |
| `SeamlyLayout <file.svg>` | Opens that file through the same path as the **Import SVG** button (`Main.qml`'s `openSvgFile()` → `AppController::importSvg`) |
| `SeamlyLayout -h` / `--help`, `-v` / `--version` | Text in a dialog (no console on Windows: this is a WIN32-subsystem binary), exit 0 |
| `--svg-stdin` plus a file, empty or non-SVG standard input, two or more files, unknown option, missing / unreadable / non-`.svg` file | Error dialog naming the problem, then an empty canvas — never a silent no-op |

- **Standard input is read only for `--svg-stdin`.** `main.cpp` opens stdin on every launch, so reading it unasked would hang a shell or icon launch waiting for input that never comes.
- **Both transports end in the same import.** `import_svg` (a path) and `import_svg_document` (a string) share `reset_import_state()` and `finish_import()` in `crates/cxxqt_bridge/src/lib.rs`, so the canvas cannot behave differently depending on where the SVG came from.
- **Absolute paths only** — a relative positional argument is resolved with `QFileInfo::absoluteFilePath()` at parse time, because a launched process inherits SeamlyLayout's own working directory, not the user's.
- **No single-instance handling** — each launch is its own process and window. One document per process; there are no tabs, which is also why a second positional argument is rejected rather than queued.
- **Untagged SVGs are opened, not refused.** Every top-level `<g>` with geometry is treated as a piece, so an ordinary drawing still lays out. When the document carries no `data-type="piece"` group, `finish_import` emits `import_warning` and QML shows a non-blocking popup (`piece_extractor::count_tagged_pieces` does the counting).
- **A tagged handoff is read from its `data-type="piece"` groups, and only those** (Task 59). The handoff nests all pieces inside one `<g data-type="pattern">`, but every stage of the layout pipeline — `svg_dom::verticalize_dom`, `svg_dom::translate_dom`, `piece_extractor`, `layout_assembler`, `oversized`, `remaining`, `sheets` — assumes a piece is a **direct `<g>` child of the SVG root**. `piece_extractor::hoist_tagged_pieces` re-parents the tagged pieces to the root once, composing any wrapper `transform` onto each, and the rest of the pipeline is unchanged. **Call it from any new pre-processing entry point** (today: `layout_utils::do_process_layout` and `sheets::build_sheet_export_inputs`) — without it the packer receives the whole pattern as one sheet-sized object.
- **`id` is identity, `data-name` is what a user reads.** `PieceRect::label()` resolves `data-name` → `data-letter` → `id`; use it for warnings, error text and the Adjust overlay, never for element lookup.
- Dispatch happens from `main.cpp` on a `QTimer::singleShot(0, …)`, **after** the event loop starts — the QML window and its WebEngine canvases must exist before an SVG can be pushed into them.

## Platform Support

Windows 11, Linux (all flavors), macOS (latest 3 versions)

## Build Commands

- **Build with `packaging\windows\test_build_msi_local.ps1` from the repository
  root.** It builds all three apps, runs the Qt unit tests, and packages the
  Windows x64 MSI.
- Do **not** use `build.ps1` or `qd.ps1` any more (user decision, 2026-09-02).
  They build SeamlyLayout alone, which is not what the install-and-test loop
  needs. Both files still exist; treat them as unsupported.
- Tests stay where they are: `ctest --preset debug` for the Qt frontend suites,
  `cargo test --workspace` for the Rust crates.

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

## Seamly Application Suite

| Application            | Role                       | UI Framework       |
| ---------------------- | -------------------------- | ------------------ |
| Seamly2D               | Parent — pattern drafting | Qt/QtWidgets       |
| SeamlyMe               | Daughter — measurements   | Qt/QtWidgets       |
| **SeamlyLayout** | Daughter — pattern layout | Qt/QML + QtWidgets |
