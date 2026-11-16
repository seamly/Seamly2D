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
| `TST_VCommandLine` | part of the above | 29 pass, 0 fail |
| CollectionTest (`TST_Seamly2DCommandLine`, `TST_SeamlyMeCommandLine`) | `bin\CollectionTest.exe`, started in `bin` (it copies its data there) | 69 pass, 0 fail, 4 skip |
| ParserTest | `bin\ParserTest.exe` | pass (exit 0) |
| TranslationsTest | `bin\TranslationsTest.exe` | 0 fail (QT_FORCE_STDERR_LOGGING=1 for counts) |
| SeamlyLayout Qt tests | `ctest --preset release` | 9/9 pass |
| SeamlyLayout Rust | `cargo test --workspace` | 734 pass, 0 fail |
| CLI tester Rust | `cargo test` in `src\test\CliTestApp` | 15 pass, 0 fail |
| CLI regression suite | `src\test\CliTest\test_cli_layout_export.ps1` | 130 cases: 130 pass, 0 fail, 0 skip (full run with `-RichmondRoot`) |
| Independent import check | `test_cli_import_check.py cli_test_report.json --inkscape <inkscape.com>` (ezdxf 1.4.4, pypdf 6.19.0, Inkscape 1.4.4) | 59 DXF, PDF, SVG and HP-GL files of the full run: 0 fail. ezdxf auditor clean; pypdf reads every page; Inkscape imports every file with paths (HP-GL: 1359 paths) |
| Richmond pattern end to end | suite `-RichmondRoot` with the pattern and measurements from git history | 6 `richmond-pattern-*` cases pass (12 pieces, 36 in, AC1012, tiled Letter) |
| Python headless runner | `test_headless_export.py ... --seamly2d ... --pattern male_shirt.sm2d` | all checks pass (47) |
| Python pattern input runner | `test_pattern_input_status.py` | all checks pass |
| Python measurement runner | `test_measurement_cli.py` (SeamlyMe) | all checks pass |
| CLI tester batch | `cli_test_app --batch` on male_shirt, jacket2 (3 materials), trousers | 3 sessions pass, ZIPs read back; Marker + DXF refused (exit 2) |
| CLI tester window | window opened; screenshots of screens 1–3 | pass (see below) |
| `local_build_msi.ps1` | full run, with tests | pass: `nmake check` (4 suites), `ctest` 9/9, `cargo test` 730/0; `seamly-x64.msi` 165.8 MB (not installed) |

## CLI regression suite

Run: `powershell -File src\test\CliTest\test_cli_layout_export.ps1 -FixtureRoot <test-seamly-layout> -QtBin C:\Qt\6.11.3\msvc2022_64\bin`.
Report: `cli_test_report.json` (command, arguments, working folder, stdout, stderr, exit code, output path and size, failures per case).

| Group | Cases | What the checks prove |
|---|---|---|
| general | help, version, console help, console error, GUI start ×2, test mode, conflicts, install folder | help/version on stdout; error then help on an attached console without redirection; GUI start keeps its event loop; SeamlyLayout found in the `SeamlyLayout` subfolder when `paths/seamlyLayoutApp` names a missing file |
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
| flags | roll width, A4 landscape mm, custom tile, folded fabric with nap, flags over the file, ignore margins, mode/gap/no-grainline, conflicts, bad values, info | page sizes and widths follow the flags; changed output for margins and mode |
| no-packing | SVG, PDF, DXF, jacket2 Lining one size, with a view | pieces exported as placed; a view is refused |
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

## Acceptance criteria (CLI_1_TASK.md §13, §9)

| Criterion | Evidence | State |
|---|---|---|
| Every advertised option has documented, verified behavior | `CLI_1_OPTION_MATRIX.md` (status and test per option); `CLI_PARAMETERS.md` | met |
| Every supported option has automated coverage | matrix "Test coverage" column; suite + `TST_VCommandLine` + `StartupOptionsTests` | met |
| Valid combinations produce verified outputs | formats, paper, fabric, margins, multisize cases: page sizes, widths, DXF versions, piece counts | met |
| Invalid combinations fail with useful diagnostics | 40+ failure cases check exit code and message text | met |
| Console help and error messages work | `console-help-without-redirection`, `console-error-without-redirection` (screen buffer read) | met |
| Headless SeamlyLayout export succeeds | every export case; `richmond-handoff-*` | met |
| Iced app generates and executes commands | `tester-*` cases; screenshots of screens 1–3 | met |
| Multi-material exports archived correctly | `tester-jacket-auto-split`: 3 files, 1 ZIP, names checked | met |
| Normal GUI behavior unchanged | `gui-start-*`; Seamly2D Qt suites; `ExportMenuTests`; GUI Layout mode keeps its progress dialog (code path unchanged in GUI mode) | met |
| Documentation matches tested functionality | `CLI_PARAMETERS.md` rows link to cases in the matrix | met |
| Available unit and integration tests pass | table above | met |
| Regressions introduced by this work resolved | stale license fingerprint (fixed); Python runner rules updated for the new format rules | met |
| Environmental limitations documented | Gaps below | met |
| §9.1–15 regression list | groups in the suite table cover each item; 14 and 15 by the `tester-*` cases | met |
| `--text2paths` (§7 coverage) | `*-text2paths` cases; section below | met |

## `--text2paths`

- Before: removed (exit 64). §7 lists it as required coverage, and `TODO_CLI.md` CLI.3.5 documents the requirement, so it is back under its old name.
- SVG: labels as single-stroke paths (`hersheyStrokes`). `shirt-svg-text2paths`: no `<text>`.
- PDF and tiled PDF: glyph outlines, no embedded font (svg2pdf `embed_text = false`). `shirt-pdf-text2paths`, `shirt-pdf-tiled-text2paths`: no `/FontFile`; without the flag, `shirt-pdf-roll-36in` has one.
- PS and EPS: no font operators; labels are paths already.
- DXF keeps TEXT entities (DXF-ASTM); raster formats have no text: exit 64.

## CLI.3 (direct layout flags)

- Done on the same branch: `--layout-<name>` flags, `--layout-no-packing`, `--text2paths`. Decisions in `TODO_COMPLETED.md` (CLI.3.1, CLI.3.7).
- The MSI build ran before the flags; the flags changed no `.pro`, `.pri`, CMake or packaging file.

## Scope of the open items

- §13 lists no CAD import and no Linux or macOS run. §9 asks to "identify any platform-specific verification gaps": they are listed below.
- CAD and cutter import is CLI.2.2, a step of task CLI.2, not of CLI.1.
- Not done on purpose: installing WSL, Docker or CAD programs, or running downloaded CAD binaries. Each changes the machine outside the worktree and needs Sue's approval.

## Gaps

- Linux and macOS: no host. Tried: `wsl.exe -l` (WSL not installed), `docker` (not installed), no macOS machine. Installing WSL needs an elevated prompt and a restart; this session is not elevated (`net session` fails), and a system install needs Sue. Console attach is Windows-only code.
- CI (`ci.yml`): needs a push; the task forbids push. `act` (local Actions runner) is not installed and needs Docker.
- MSI package: built and its tests passed; not installed on this machine. The install-folder lookup ran from a copy of the install layout.
- Richmond: the `.sm2d` and its measurements are not in the fixture folder; git history has them (`d3197ee973^:test-seamly-layout-input/`). Extracted to a scratch folder and tested end to end; not added back to the repository (personal measurements).
- CAD and cutter import (CLI.2.2): ezdxf, pypdf and Inkscape 1.4.4 open every DXF, PDF, SVG and HP-GL file. No CAD or cutter program is installed (checked Program Files for AutoCAD, LibreCAD, QCAD, FreeCAD, CLO, Optitex, Gerber, Lectra). PS and EPS import into Inkscape needs Ghostscript, which is not installed.
- Inkscape 1.4.4 for Windows ships its HP-GL reader without pyparsing; the check adds a copy of pyparsing for that reader only.
- `foldLinesOnLeftEdge` and `selvedgeWidth`: accepted and validated; their placement effects are covered by SeamlyLayout Rust tests, not by file checks here.
- Tester window clicks were not automated; Run calls the same function as `--batch`.
