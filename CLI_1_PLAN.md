# CLI.1 Implementation Plan

Branch `feature/cli-1`. Dependency order. A box is checked only after a test run proved it.
Results: `CLI_1_TEST_RESULTS.md`. Options: `CLI_1_OPTION_MATRIX.md`.

## Audit findings (2026-11-15, from code and first runs)

| # | Defect | Location | State |
|---|---|---|---|
| D1 | Parse error text printed before the console attached; the Windows console showed help only | `command_line.cpp` | fixed |
| D2 | Console attach missed `--opt=value` forms | `command_line.cpp` | fixed |
| D3 | Multisize `--layout-export` handed off one size; Layout mode hands off every size | `exportLayoutFromCommandLine()` | fixed |
| D4 | Headless never ran the queued views and materials; their unplaced pieces went unchecked | `HeadlessExport.cpp` | fixed |
| D5 | Headless always exported material 0, silently | `HeadlessExport.cpp` | fixed: `--material`, exit 64 when several and none named |
| D6 | No multisize view selection | CLI, headless | fixed: `--layout-view` |
| D7 | Oversized pieces always failed; no split or longer-media choice | `HeadlessExport.cpp` | fixed: `--layout-oversize` |
| D8 | Settings JSON: unknown keys and invalid values were coerced silently | `SettingsModel::load()` | fixed: `validate_settings_json` |
| D9 | Export menu format rules not enforced headless | `HeadlessExport.cpp` | fixed: `export_rules.rs` |
| D10 | An old output file passed the "file exists" check after a failed export | `HeadlessExport.cpp` | fixed |
| D11 | A SeamlyLayout crash left a partial output file | `exportLayoutFromCommandLine()` | fixed |
| D12 | No query of materials, views, oversized pieces, formats | new | fixed: `--layout-info` |
| D13 | Relative `--mfile` resolved against the pattern folder | `measurementPathOption()` | fixed |
| D14 | `--layout-scale` accepted but ignored: PNG export is fixed at 96 px/in | CLI, headless, bridge | removed |
| D15 | INFO log lines, DXF converter debug lines and doubled CR on stdout/stderr | log handler, `converter.rs`, forwarding | fixed |
| D16 | Python headless runner: stale expectations (unplaced on tiled paper, tile marker path) | `test_headless_export.py` | updated |
| D17 | No text-as-paths export: PDF embeds fonts; `--text2paths` was removed | CLI, headless, `exports.rs` | fixed: `--text2paths` |

## Tasks

1. [x] Build the three apps in the worktree; run `cargo test`, `ctest`, `Seamly2DTests`.
2. [x] D1, D2: console attach before any CLI message.
3. [x] D8: `validate_settings_json` (Rust); headless checks an explicit settings file first.
4. [x] D4, D5, D6, D7, D9, D10: headless `--material`, `--view`, `--oversize`; queue drain; format rules; output handling.
5. [x] D12: headless `--info` JSON.
6. [x] D3: every size in the CLI handoff unless `--gsize`/`--gheight`; no progress dialog in CLI modes.
7. [x] Seamly2D `--layout-material`, `--layout-view`, `--layout-oversize`, `--layout-info`; D11, D13, D14, D15.
8. [x] Unit tests: `TST_VCommandLine` (24), `StartupOptionsTests`, `rust:settings_validation`, `rust:export_rules`.
9. [x] PowerShell suite `src/test/CliTest/` with JSON report.
10. [x] Rust/Iced tester `src/test/CliTestApp/` (GUI and `--batch`).
11. [x] Full regression run; results in `CLI_1_TEST_RESULTS.md`.
12. [x] Docs: `--help`, `project-docs/docs-cli/CLI_PARAMETERS.md`, option matrix, `TODO_CLI.md`.

## Decisions

- Layout Settings controls map to `--layout-settings` JSON keys. Direct flags stay in CLI.3 (flag names and precedence need Sue).
- Removed legacy options stay removed: exit 64 "Unknown option". Exception: `--text2paths` (CLI_1_TASK §7, CLI.3.5): SVG single-stroke paths, PDF glyph outlines; PS and EPS draw paths already.
- Option names are lowercase kebab-case already; no aliases added.
- Multisize without `--gsize`/`--gheight`: every size at the base height, as Layout mode. With either: one size and height, no views.
- Several materials need `--layout-material`; the tester runs one command per material.
- A run that fails before the export keeps an existing output file (the Python runner's earlier contract); SeamlyLayout removes it only just before it writes.
- Windows: GUI subsystem plus `AttachConsole`. A console subsystem would flash a console on GUI start. Callers that need the exit code wait (`start /wait`, `Start-Process -Wait`, a pipe).
- `--layout-scale` removed rather than implemented: the PNG export is fixed at 96 px/in by design.
