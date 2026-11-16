# CLI.1 Test Results

- Date: 2026-11-16. Branch `feature/cli-1`.
- Machine: Windows 11 x64, VS 18 Community (MSVC 14.51), Qt 6.11.3 `msvc2022_64`, Rust stable, Windows PowerShell 5.1, Python 3.14.
- Build: in-source `qmake Seamly.pro -r -config release CONFIG+=deferDeploy` + `jom`; SeamlyLayout `cmake --preset release`. Qt DLLs from `PATH` (no windeployqt).
- Fixtures: read-only from `C:\Users\susan\Projects\Seamly2D-private\test-seamly-layout` (`patterns\jacket2_40-146.sm2d`, `male_shirt.sm2d`, `trousers.sm2d`, `measurements\individual\*.smis`, `measurements\multisize\gost_man_ru.smms`).
- Generated files: session scratchpad and `build\cli-tests\` (git-ignored). None in the fixture folder.

## Summary

| Suite | Command | Result |
|---|---|---|
| Seamly2D Qt tests (40 suites) | `src\test\Seamly2DTest\target_wrapper.bat bin\Seamly2DTests.exe` (`QT_QPA_PLATFORM=offscreen`) | 32 621 pass, 0 fail |
| `TST_VCommandLine` | part of the above | 24 pass, 0 fail |
| SeamlyLayout Qt tests | `ctest --preset release` | 9/9 pass |
| SeamlyLayout Rust | `cargo test --workspace --no-fail-fast` | 729 pass, 0 fail (28 test binaries) |
| CLI tester Rust | `cargo test` in `src\test\CliTestApp` | 15 pass, 0 fail |
| CLI regression suite | `src\test\CliTest\test_cli_layout_export.ps1` | 99 cases: 99 pass, 0 fail, 0 skip (full run, about 20 min) |
| Python headless runner | `test_headless_export.py ... --seamly2d ... --pattern male_shirt.sm2d` | all checks pass (47) |
| Python pattern input runner | `test_pattern_input_status.py` | all checks pass |
| Python measurement runner | `test_measurement_cli.py` (SeamlyMe) | all checks pass |
| CLI tester batch | `cli_test_app --batch` on male_shirt, jacket2 (3 materials), trousers | 3 sessions pass, ZIPs read back; Marker + DXF refused (exit 2) |
| CLI tester window | window opened; screenshots of screens 1–3 | pass (see below) |
| `local_build_msi.ps1` | not run | needs Sue (CLAUDE.md); MSI install layout not tested |

## CLI regression suite

Run: `powershell -File src\test\CliTest\test_cli_layout_export.ps1 -FixtureRoot <test-seamly-layout> -QtBin C:\Qt\6.11.3\msvc2022_64\bin`.
Report: `cli_test_report.json` (command, arguments, working folder, stdout, stderr, exit code, output path and size, failures per case).

| Group | Cases | What the checks prove |
|---|---|---|
| general | help, version, console help, console error, GUI start ×2, test mode, conflicts | help/version on stdout; error then help on an attached console without redirection; GUI start keeps its event loop |
| legacy | `--text2paths`, `--basename`, `--exportOnlyDetails`, `-f`, `--layout-scale` | exit 64, "Unknown option '<name>'" |
| export, paths | companions, unknown format, text mode with PDF, missing output folder, relative pattern, `--mfile`, settings and output | exit 64/66 with the message; relative paths from the launch folder |
| settings | missing file, invalid JSON, invalid values, scale grid | exit 64 naming `marginTOP` and `tileOrientation`; `scale_grid` element present only when set |
| formats | svg, pdf, pdf-tiled, png, jpg, jpeg, dxf (R13, AAMA R11, CLO3D), hpgl, ps, eps, gcode, 3mf | file structure; DXF `AC1012`/`AC1009`; PNG 3456 px for 36 in; gcode/3mf exit 69 |
| text | designerFont, singleLineFont, hersheyStrokes, asSupplied | `<text>` present, or none for hersheyStrokes |
| paper-tiled | Letter portrait, A4 landscape, Custom 10×12 in, trousers, jacket2 Interfacing | page size from the PDF MediaBox (8.5×11, 11.69×8.27, 10×12 in) |
| paper-roll | 36 in roll, `pdf` on tiled paper, `pdf-tiled` on a roll | page width 36 in; rule violations exit 64 |
| fabric, margins | folded with nap (150 cm → 75 cm), open fabric 60 in with margins | SVG/PDF width |
| material | jacket2 without material, unknown material, `fabric` (any case) | exit 64 lists the materials |
| multisize | Marker PDF, Nested SVG, Base Size DXF + `.rul`, Size 400 DXF, Marker DXF refused, unknown view, `--gsize`/`--gheight` one size | views follow the tabs; DXF one size only |
| oversize | no choice (exit 70, old file kept), split PDF/SVG, split DXF refused, extend too wide (exit 70), jacket2 split Marker refused, split default view | page width 12 in after a split |
| transport | jacket2 handoff copy | ≥ 1 MB, ends `</svg>`, valid XML: no truncation |
| info | shirt roll, shirt oversized, jacket2 roll, info with format | stdout is one JSON document; views, split views, oversized pieces |
| tester | shirt PDF, jacket2 auto split (3 materials), Marker DXF refused | one ZIP with one file per material, names `<basename>_<material>_<YYMMDDHHMM>.<ext>` |
| seamlylayout | Richmond handoff SVG/PDF/DXF/info | 12+ pieces, 36 in width, AC1012 |

## Defects found and fixed

See `CLI_1_PLAN.md` D1–D16. Each has a case above or a unit test.

## Rust/Iced tester screens

- Screen 1 (inputs): fields, Browse, Next. Window title "Seamly2D CLI Tester".
- Screen 2 (jacket2, 12 in roll): "Split the pieces: on", Layout = Base Size 500; Marker and Nested shown as unavailable.
- Screen 3 (male_shirt, tiled Letter): format svg, SVG text designerFont; pdf, gcode, 3mf with reasons; the command with Copy.
- Interaction (clicks) was not automated; the Run path is the same function as `--batch` (`export_run::run_session`).

## Gaps

- Linux and macOS: not run. Console attach is Windows-only code; other platforms print to their terminal.
- MSI: not built; the installed-folder lookup of SeamlyLayout (`SeamlyLayout\` beside Seamly2D) is untested here.
- Richmond `.sm2d` and its measurements are not in the fixtures; only the handoff SVG was tested.
- CI (`ci.yml`) not run: no push.
- `foldLinesOnLeftEdge` and `selvedgeWidth`: accepted and validated; their placement effects are covered by SeamlyLayout Rust tests, not by file checks here.
