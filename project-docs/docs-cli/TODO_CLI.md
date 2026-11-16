# TODO — CLI

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file start with `CLI.`

## Task CLI.1 - Audit & fix the CLI parameters so that each option works

**Dependency:** seamlyLayout export modes is required for the mode pass-through; seamlyLayout also needs a headless/CLI export mode of its own, since today it is only driven interactively through its QML UI.

Seamly2D has no console export mode: Dead.4.1.1 removed `--basename` and the old layout options. Add a new export option in `src/app/seamly2d/core/command_line.cpp` so a single seamly2d command line produces the final layout: seamly2d generates the tagged `.pieces.svg` (the Layout Mode handoff, `exportPiecesToSeamlyLayout()` in `src/app/seamly2d/mainwindow.cpp`) and then runs seamlyLayout on it to produce the layout output, using the new seamlyLayout export options.

Go through every command-line option seamly2d advertises (defined in `src/libs/vmisc/commandoptions.cpp`, wired in `src/app/seamly2d/core/command_line.cpp`) and make each one actually work in console export mode. Known friction from Task 11 verification: option names are case-sensitive and inconsistently cased (e.g. `--exportOnlyDetails`), and errors only surface in a redirected stderr, not on the console.

These tasks are not numbered in the order they should be implemented -- analyze and update these tasks tersely to form a step-by-step implementation plan.

- [x] CLI.1.1 Add `--layout-export`, resolve `paths/seamlyLayoutApp`, and send the generated handoff through stdin without an intermediate file.
- [x] CLI.1.2 Headless SeamlyLayout reads a file or stdin, lays out pieces, exports the selected format, and reports meaningful status without QML UI.
- [x] CLI.1.3 Forward format, SVG text modes, output, and settings; document the option mapping.
- [x] CLI.1.4 Wait for SeamlyLayout and propagate stderr/status. Incomplete layouts fail before writing partial output.
- [ ] CLI.1.5 Tests: seamly2d CLI option parsing (extend `tst_vcommandline`), seamlyLayout headless-export tests (Rust/Qt side), and an end-to-end check with the richmond test pattern
- [x] CLI.1.6 Document workflow examples, options, statuses, and `--help` behavior.
- [ ] CLI.1.7 Inventory all options and build a test matrix: expected behavior, required companions (e.g. `--basename` enabling export mode), valid values. The matrix should organize the options from the Layout Settings dialog in order, starting with the top option 'Layout Mode' and ending with the last option 'Margins'.
 series of .ps1 files to exercise these options, using test-seamly-layout\patterns\jacket2_40-146.sm2d to exercise multisize options and test-seamly-layout\patterns\male_shirt.sm2d and test-seamly-layout\patterns\trousers.sm2d to exercise individual size options. Create PDF, PDF (tiled), DXF (R13), and SVG exports. Include settings for Paper > Tiled, Paper > Roll, Fabric (folded) with Grainline 'With Nap'. 
- [ ] CLI.1.9 Create an interactive menu-driven app in Rust/Ice (Qt not required) that will allow us to test the CLI thoroughly. The app generates a command line string then executes it to create the output file(s), based on user input.
  - [ ] The first dialog allows selection of pattern file, measurement file, layout settings, and output file basename (using a file picker with the pattern's basename and application preferences' layout directory as default). If there are pieces wider than the media then generate the layout with 'Split the pieces' selected by default without asking the user.
  - [ ] For multisize pattern files a second dialog appears to allow selection of Layout using a 'Layout' dropddown that lists all Layouts possible based on layout settings (e.g. the list includes Marker, Nested, Baseline, and all sizes. But if pieces are split then 'Marker' and 'Nested' are disabled). 
  - [ ] A final dialog appears to allow export format selection using a 'Export format' dropdown that lists all formats possible based on the settings and layout selected (e.g. if paper type='PDF (tiled)' then export 'PDF (Tiled)' is enabled, if pieces are split then DXF options are disabled, etc.)
  - [ ] For the layout selected, build a command line string for each material using the input data, where the material's layout file is '<basename>_<material>_<YYMMDDHHMM>.<ext>' 
  - [ ] Execute the command line string(s) to generate the layout file(s) and archive these files to a .zip file '<basename>_<YYMMDDHHMM>.zip'.
- [ ] CLI.1.8 Exercise each option against the jacket 2 test pattern (all export formats, gradation size/height, page options, `--text2paths`, measurement overrides, etc.) and record which are broken, ignored, or misdocumented
- [ ] CLI.1.10 Fix the broken/ignored options; make error messages reach the console reliably (the GUI-subsystem exe detaches from the console — evaluate `AttachConsole`/subsystem handling on Windows so `--help` and errors print without redirection)
  - Seamly2D and SeamlyMe log cleanup now uses absolute log paths without changing the launch directory.
  - Added relative pattern, measurement, override, and missing-file regression checks. Build and runtime verification remain pending at the user's request.
  - `VCommandLine::showHelp()` prints help to the console when stdout or stderr has a handle. With no console it shows a help dialog (from upstream). Not built yet.
- [ ] CLI.1.11 Consider case-insensitive or consistently lowercase option aliases (keeping the existing names working for compatibility)
- [ ] CLI.1.12 Unit tests: extend `tst_vcommandline` to cover every option and the failure modes found
- [ ] CLI.1.13 Update `--help` text and repo docs with the verified behavior

## Task CLI.2 - run seamly2d/seamlyme/seamlylayout from the command line with an input .sm2d file and an input measurement file to create exports for each of seamlyLayout's export options; validate outputs to validate the CLI parameters

Implemented validation covers ten public formats, paid-format rejection, measurement/gradation overrides, omitted-piece failures, and physical PDF/DXF/HPGL calibration. SeamlyMe validates measurement inputs; it does not read `.sm2d` patterns or export layouts. Exact Richmond pattern-and-measurement verification remains blocked: only the cached handoff SVG is present. The exhaustive older-option audit and user/CAD import checks remain open. Integrated individual and multisize-input export checks, 19 pattern-status checks, and 13 SeamlyMe checks pass with matching Qt plugins.

## Task CLI.3 - Legacy layout options as direct `--layout-*` flags

Upstream `develop` keeps the old console export mode (`--basename`, `--format`, the `vlayout` packer and `vdxf`). This branch replaced it with `--layout-export`, which sends the handoff SVG to SeamlyLayout. The legacy options below have no direct flag. Most map to a field in the `--layout-settings` JSON (`crates/layout_tiling/src/layout_settings.rs`).

Design:

- Seamly2D parses each flag in `src/app/seamly2d/core/command_line.cpp` and forwards it to SeamlyLayout headless export.
- SeamlyLayout parses it in `StartupOptions.cpp`. Rust applies it to `LayoutSettings`.
- Precedence: defaults, then the `--layout-settings` JSON, then the direct flag. Needs Sue's confirmation.
- Lowercase kebab-case names only (see CLI.1.10).

| Legacy option | Legacy behavior | SeamlyLayout today | Action |
|---|---|---|---|
| `-b`/`--basename`, `-d`/`--destination` | output file name, folder | `--layout-output` | none |
| `-f`/`--format` | format number | `--layout-format` name | none |
| `-p`/`--pageformat` | paper template | `paper_type` (JSON) | add flag |
| `-W`/`--pagew`, `-H`/`--pageh` | page width, height | `page_width`, `page_height` (JSON) | add flags |
| `-U`/`--pageunits`, `-l`/`--layounits` | page and layout units | `unit` (JSON) | add one flag |
| `-L`, `-R`, `-T`, `-B` | left, right, top, bottom margin | `margin_left`, `margin_right`, `margin_top`, `margin_bottom` (JSON) | add flags |
| `-i`/`--ignoremargins` | all margins 0 | set each `margin_*` to 0 | add flag |
| `-G`/`--gapwidth` | piece gap | `piece_gap` (JSON) | add flag |
| `-r`/`--rotate` | rotation divisor of 360° | `layout_mode`, `rotation_step`, `no_grainline_rotation` (JSON) | add flags |
| `-c`/`--crop` | trim unused sheet length | roll reduction | verify in headless export |
| `-u`/`--unite` | one image for all sheets | roll media gives one sheet | verify; no flag |
| `-S`/`--savelen` | fill sheet width to save length | packer | evaluate |
| `-g`/`--groups` | piece order by area groups | none | evaluate |
| `-s`/`--shiftlen` | step length of the legacy packer | none | not ported: the SeamlyLayout packer has no step length |
| `--bdxf` | binary DXF | ASCII DXF only | evaluate |
| `--text2paths` | text as paths in all vector formats | `--svg-text-mode` (SVG only) | extend to PDF, PS, EPS |
| `--exportOnlyDetails` | export pieces at their Piece mode positions, no layout | none | add flag: skip packing |
| formats BMP, TIFF, PPM, OBJ | raster and 3D output | PNG, JPEG, 3MF | evaluate |
| DXF flat, AAMA, ASTM in versions AC1006–AC1027 | DXF flavor and AutoCAD version | `dxf` plus `--layout-export-options` | check that export options select flavor and version |

- [ ] CLI.3.1 Confirm flag names and precedence
- [ ] CLI.3.2 Add the paper, page size, unit, margin, no-margin and gap flags
- [ ] CLI.3.3 Add the layout mode, nap direction and no-grainline rotation flags
- [ ] CLI.3.4 Verify headless roll export trims unused length and writes one sheet (`--crop`, `--unite`)
- [ ] CLI.3.5 Add text as paths for PDF, PS and EPS, if the exports keep text as text today
- [ ] CLI.3.6 Add a flag that exports pieces at their handoff positions without packing
- [ ] CLI.3.7 Evaluate `--savelen`, `--groups`, `--bdxf`, BMP, TIFF, PPM, OBJ and the DXF flavor and version; record each decision here
- [ ] CLI.3.8 Tests: `tst_vcommandline` for each Seamly2D flag; SeamlyLayout startup option tests for each forwarded flag
- [ ] CLI.3.9 Update `project-docs/docs-cli/CLI_PARAMETERS.md` and the `--help` text
