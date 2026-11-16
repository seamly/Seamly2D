# TODO — CLI

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file start with `CLI.`

## Task CLI.2 - run seamly2d/seamlyme/seamlylayout from the command line with an input .sm2d file and an input measurement file to create exports for each of seamlyLayout's export options; validate outputs to validate the CLI parameters

- `src/test/CliTest/test_cli_layout_export.ps1` runs Seamly2D and SeamlyLayout for every option group and export format with male_shirt, trousers and jacket2. Results: `CLI_1_TEST_RESULTS.md`.
- SeamlyMe validates measurement inputs only; it does not read `.sm2d` patterns or export layouts.

- [ ] CLI.2.1 Richmond pattern and measurement files are not in the fixtures. Only `cxxqt_bridge/test_data/richmond-shirt-handoff_pieces.svg` exists; the suite exports it through SeamlyLayout.
- [ ] CLI.2.2 Import the exports into CAD and cutter software (user check).

## Task CLI.3 - Legacy layout options as direct `--layout-*` flags

Upstream `develop` keeps the old console export mode (`--basename`, `--format`, the `vlayout` packer and `vdxf`). This branch replaced it with `--layout-export`, which sends the handoff SVG to SeamlyLayout. The legacy options below have no direct flag. Most map to a field in the `--layout-settings` JSON (`crates/layout_tiling/src/layout_settings.rs`).

Material, multisize view, oversized pieces and the JSON report have flags already:
`--layout-material`, `--layout-view`, `--layout-oversize`, `--layout-info` (`CLI_PARAMETERS.md`).

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
| `--layout-scale` (this branch) | PNG scale | removed: PNG export is fixed at 96 px/in | none |
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
