# CLI.1 Implementation Plan

Branch `feature/cli-1`. Dependency order. A box is checked only after a test run proves it.

## Audit findings (2026-11-15, from code)

| # | Defect | Location |
|---|---|---|
| D1 | Parse error text (`Unknown option`) prints before the console attaches; Windows terminal shows help only | `command_line.cpp` `getCommandLine()` |
| D2 | Console attach misses `--opt=value` forms and `--no-scaling` with CLI modes | `attachParentConsoleForCommandLine()` |
| D3 | Multisize `--layout-export` hands off one size; GUI hands off every size (Marker, Nested, Base, Size tabs) | `exportLayoutFromCommandLine()` |
| D4 | Headless never drains the multisize/material tab queue; other tabs and materials are not laid out or checked | `HeadlessExport.cpp` |
| D5 | Headless always exports material 0; no material selection; a multi-material pattern exports one material silently | `HeadlessExport.cpp` |
| D6 | No multisize view selection (Marker, Nested, Base Size, Size N) | CLI and headless |
| D7 | Oversized pieces always fail; no split / make-longer choice | `HeadlessExport.cpp` |
| D8 | Settings JSON: unknown keys and invalid values pass silently (coerced to defaults) | `SettingsModel::load()`, `LayoutSettings::from_json()` |
| D9 | Format rules of the Export menu not enforced headless: `pdf` on tiled paper, `pdf-tiled` off tiled paper, split + DXF/pdf-tiled/paid, Marker/Nested + DXF | `HeadlessExport.cpp` |
| D10 | Stale output file from an earlier run passes the "file exists" check after a failed export | `HeadlessExport.cpp` |
| D11 | SeamlyLayout crash leaves a partial output file; Seamly2D does not remove it | `exportLayoutFromCommandLine()` |
| D12 | No query mode: a caller cannot list materials, views or oversized pieces without a layout run | new |

## Tasks

1. [ ] Build all three apps in the worktree; run `cargo test`, `ctest`, `tst_vcommandline` as baseline.
2. [ ] D1, D2: attach console before any CLI message; match `--opt=value`.
3. [ ] D8: `LayoutSettings::validate_json()` (Rust, strict keys, enums, numbers); headless calls it before layout.
4. [ ] D4, D5, D6, D7, D9, D10: headless options `--material`, `--view`, `--oversize`; drain tab queue; format rules; stale/partial output removal.
5. [ ] D12: headless `--info` prints JSON: materials, views, split views, oversized pieces, label text state, media.
6. [ ] D3: CLI multisize handoff carries every size when `--gsize`/`--gheight` are absent; no progress dialog in CLI mode.
7. [ ] Seamly2D options `--layout-material`, `--layout-view`, `--layout-oversize`, `--layout-info`; validation; forwarding. D11.
8. [ ] Unit tests: `tst_vcommandline`, `StartupOptionsTests`, Rust tests. `cargo test --workspace`, `ctest`.
9. [ ] PowerShell CLI regression suite (`src/test/CliTest/`), JSON report; run on jacket2, male_shirt, trousers.
10. [ ] Rust/Iced test application (`src/test/CliTestApp/`).
11. [ ] Regression run of the full matrix; record in `CLI_1_TEST_RESULTS.md`.
12. [ ] Docs: `--help`, `CLI_PARAMETERS.md`, `CLI_1_OPTION_MATRIX.md`, `TODO_CLI.md`.

## Decisions

- Layout Settings dialog controls map to `--layout-settings` JSON keys. No direct flags: CLI.3 owns them (flag names and precedence need Sue).
- Removed legacy options stay removed, `--text2paths` included. They fail with exit 64 and "Unknown option". `--svg-text-mode hersheyStrokes` gives stroke paths for SVG. CLI.3.5 owns text as paths for PDF/PS/EPS.
- Option names are already lowercase kebab-case. No aliases added. Single-letter options stay case-sensitive.
- Multisize without `--gsize`/`--gheight`: every size at the base height, as Layout mode. With either: one size, one height, no tabs.
- A multi-material pattern needs `--layout-material` when the export is one file; without it the export fails (exit 64) and lists the materials.
- Windows: keep the GUI subsystem plus `AttachConsole`. A console subsystem flashes a console on GUI start. Callers that need the exit code wait for the process (PowerShell `Start-Process -Wait`, redirection, or `start /wait`).
