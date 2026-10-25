---
paths:
  - "src/app/seamlylayout/qt_frontend/src/StartupOptions.*"
  - "src/app/seamlylayout/qt_frontend/seamlyLayout_main.mm"
  - "src/app/seamlylayout/qt_frontend/qml/Main.qml"
  - "src/app/seamlylayout/crates/cxxqt_bridge/src/lib.rs"
  - "src/app/seamlylayout/crates/cxxqt_bridge/src/piece_extractor.rs"
  - "src/app/seamlylayout/crates/cxxqt_bridge/src/material_split.rs"
  - "src/test/SeamlyLayoutTest/StartupOptionsTests.cpp"
  - "src/libs/vmisc/seamly_suite_paths.*"
  - "src/test/Seamly2DTest/tst_seamlysuitepaths.*"
  - "src/app/seamly2d/core/windows_foreground_window.*"
  - "project-docs/docs-data/SVG-DATA-ATTRIBUTES.md"
---

# Seamly2D → SeamlyLayout Handoff Contract

Seamly2D's Layout Mode builds the tagged pieces SVG in memory, launches SeamlyLayout with `--svg-stdin`, then writes the document to the new process's standard input and closes the channel. **No file is written** — the earlier `<pattern>.pieces.svg` handoff is gone. The contract is implemented in `qt_frontend/src/StartupOptions.{h,cpp}` and pinned by `src/test/SeamlyLayoutTest/StartupOptionsTests.cpp`; the producing half lives in `src/libs/vmisc/seamly_suite_paths.cpp` and is pinned by `TST_SeamlySuitePaths`. **Change one side and you must change the other** — see `project-docs/docs-data/SVG-DATA-ATTRIBUTES.md` for the full statement.

| Invocation | Behaviour |
| ---------- | --------- |
| `SeamlyLayout` | Empty canvas — the double-clicked-icon case |
| `SeamlyLayout --svg-stdin [--document-name <name>]` | The Seamly2D handoff: reads the SVG document from standard input (`Main.qml`'s `openSvgDocument()` → `AppController::importSvgDocument`) |
| `SeamlyLayout <file.svg>` | Opens that file through the same path as the **Import SVG** button (`Main.qml`'s `openSvgFile()` → `AppController::importSvg`) |
| `SeamlyLayout -h` / `--help`, `-v` / `--version` | Text in a dialog (no console on Windows: this is a WIN32-subsystem binary), exit 0 |
| `--svg-stdin` plus a file, empty or non-SVG standard input, two or more files, unknown option, missing / unreadable / non-`.svg` file | Error dialog naming the problem, then an empty canvas — never a silent no-op |

- **Standard input is read only for `--svg-stdin`.** `seamlyLayout_main.mm` opens stdin on every launch, so reading it unasked would hang a shell or icon launch waiting for input that never comes.
- **Both transports end in the same import.** `import_svg` (a path) and `import_svg_document` (a string) share `reset_import_state()` and `finish_import()` in `crates/cxxqt_bridge/src/lib.rs`, so the canvas cannot behave differently depending on where the SVG came from.
- **Absolute paths only** — a relative positional argument is resolved with `QFileInfo::absoluteFilePath()` at parse time, because a launched process inherits SeamlyLayout's own working directory, not the user's.
- **No single-instance handling** — each launch is its own process and window. One document per process; there are no tabs, which is also why a second positional argument is rejected rather than queued. Seamly2D itself starts at most one SeamlyLayout per Seamly2D window (`MainWindow::isSeamlyLayoutRunning`).
- **Window in front (Windows)** — Seamly2D calls `AllowSetForegroundWindow` for the new process (`src/app/seamly2d/core/windows_foreground_window.cpp`), and `seamlyLayout_main.mm` raises its window before the import. Without both, SeamlyLayout opens behind Seamly2D.
- **Untagged SVGs are opened, not refused.** Every top-level `<g>` with geometry is treated as a piece, so an ordinary drawing still lays out. When the document carries no `data-type="piece"` group, `finish_import` emits `import_warning` and QML shows a non-blocking popup (`piece_extractor::count_tagged_pieces` does the counting).
- **A tagged handoff is read from its `data-type="piece"` groups, and only those.** The handoff nests the pieces inside one `<g data-type="pattern">` per material (`data-material`: Fabric, Lining, ...). `material_split::split_by_material` gives each material its own document at import, so each document holds one pattern group. Every stage of the layout pipeline — `svg_dom::verticalize_dom`, `svg_dom::translate_dom`, `piece_extractor`, `layout_assembler`, `oversized`, `remaining`, `sheets` — assumes a piece is a **direct `<g>` child of the SVG root**. `piece_extractor::hoist_tagged_pieces` re-parents the tagged pieces to the root once, composing any wrapper `transform` onto each, and the rest of the pipeline is unchanged. **Call it from any new pre-processing entry point** (today: `layout_utils::do_process_layout` and `sheets::build_sheet_export_inputs`) — without it the packer receives the whole pattern as one sheet-sized object.
- **`id` is identity, `data-name` is what a user reads.** `PieceRect::label()` resolves `data-name` → `data-letter` → `id`; use it for warnings, error text and the Adjust overlay, never for element lookup.
- Dispatch happens from `seamlyLayout_main.mm` on a `QTimer::singleShot(0, …)`, **after** the event loop starts — the QML window and its WebEngine canvases must exist before an SVG can be pushed into them.
