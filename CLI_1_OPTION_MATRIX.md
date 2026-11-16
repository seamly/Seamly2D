# CLI.1 Option Matrix

Source: `command_line.cpp`, `commandoptions.cpp`, `StartupOptions.cpp`, `HeadlessExport.cpp`,
`SettingsDialog.qml`, `SettingsModel.cpp`, `settings_validation.rs`, `export_rules.rs`.

- Status: working, broken, ignored, removed, unverified. "Working" = a test below ran and passed.
- Tests: `TST_VCommandLine::<name>` (Qt), `StartupOptionsTests::<name>` (Qt), `<case>` = `src/test/CliTest` case, `rust:<name>` = cargo test.
- Each Layout Settings control is a `--layout-settings` JSON key and, for most, a direct flag `--layout-<name>` (wins over the file).
- JSON keys reach SeamlyLayout as `--settings <path>`; SettingsModel loads them, `validate_settings_json` checks them first.

## Layout settings (Layout Settings dialog order)

| UI setting | CLI option | Accepted values | Default | Dependencies | Constraints | SeamlyLayout mapping | Status | Test coverage |
|---|---|---|---|---|---|---|---|---|
| Layout Mode | `--layout-mode`; key `layoutMode` | `alongGrainline`, `withNap`, `any` (legacy `withGrain`, `withoutGrain` migrate) | `alongGrainline` | `--layout-export` or `--layout-info` | other text: exit 64 | `--settings` → `LayoutSettings::layout_mode` | working | `shirt-pdf-tiled-custom-margins` (any), `shirt-fabric-folded-with-nap` (withNap), `rust:with_nap_needs_a_nap_direction` |
| Nap Direction | `--layout-nap up/down`; key `rotationStep` | `0` up, `180` down (withNap) | `0` | `layoutMode` `withNap` | withNap with other step: exit 64 | `rotation_step` | working | `shirt-fabric-folded-with-nap` (180), `shirt-fabric-open-margins` (0), `rust:with_nap_needs_a_nap_direction` |
| No Grainline | `--layout-no-grainline`; key `noGrainlineRotation` | `upright`, `free` | `upright` | — | other text: exit 64; ignored in `any` | `no_grainline_rotation` | working | `shirt-fabric-open-margins` (free), `rust:dialog_settings_pass` |
| Piece Gap | `--layout-gap`; key `pieceGap` | number ≥ 0 in `unit` | `0.05` | — | negative: exit 64 | `piece_gap` | working | all settings fixtures, `rust:typing_errors_name_each_key` |
| Add scale check grid | key `scaleGrid` | `true`, `false` | `false` | — | not boolean: exit 64 | `scale_grid` (bridge) | working | `shirt-svg-scale-grid` (SVG has `id="scale_grid"`), `shirt-svg-designer-font` (none without it) |
| Unit | `--layout-unit`; key `unit` | `in`, `cm`, `mm` | `in` | — | other text: exit 64 | `unit` | working | `shirt-pdf-tiled-a4-landscape` (mm), `shirt-fabric-folded-with-nap` (cm) |
| Media Type | `--layout-media`; key `mediaType` | `paper`, `fabric` | `paper` | — | `cloth` etc.: exit 64 | `media_type` | working | `shirt-fabric-*`, `shirt-pdf-*`, `rust:typing_errors_name_each_key` |
| Place fold lines on the left edge | key `foldLinesOnLeftEdge` | `true`, `false` | `true` | — | not boolean: exit 64 | `fold_lines_on_left_edge` | working (accepted; placement tested in Rust layout tests) | `shirt-fabric-folded-with-nap` |
| Paper Type | `--layout-paper`; key `paperType` | `tiled`, `roll` (legacy `sheet` = tiled) | `tiled` | `mediaType` `paper` | `pdf` on tiled, `pdf-tiled` on roll: exit 64 | `paper_type` | working | `shirt-pdf-on-tiled-paper`, `shirt-pdf-tiled-on-roll`, `shirt-pdf-roll-36in` |
| Paper > Tiled: Tile Size | `--layout-tile-size`; key `tileSize` | catalog names (`Letter` … `B6`), `Custom` | `Letter` | `paperType` `tiled` | unknown name: exit 64 | `tile_size` | working | `shirt-pdf-tiled-letter-portrait` (8.5×11 in pages), `shirt-pdf-tiled-a4-landscape` (11.69×8.27 in) |
| Paper > Tiled: Custom Size | `--layout-tile-width`, `--layout-tile-height`; keys `tileWidth`, `tileHeight` | number > 0 | `8.5`, `11` | `tileSize` `Custom` | ≤ 0: exit 64 | `tile_width`, `tile_height` | working | `shirt-pdf-tiled-custom-margins` (10×12 in pages) |
| Paper > Tiled: Tile Orientation | `--layout-tile-orientation`; key `tileOrientation` | `landscape`, `portrait` (exact case) | `landscape` | `paperType` `tiled` | `Portrait`: exit 64 | `tile_orientation` | working | `shirt-pdf-tiled-letter-portrait`, `shirt-pdf-tiled-a4-landscape`, `settings-invalid-values` |
| Paper > Roll: Roll Width | `--layout-roll-width`; keys `rollSize`, `rollWidth` | `custom` or `<n> in/mm/cm`; width ≥ 0 | `36 in`, `36` | `paperType` `roll` | bad text: exit 64 | `roll_width` | working | `shirt-pdf-roll-36in` (36 in page), `shirt-oversize-split-pdf` (12 in page) |
| Paper > Roll: Roll Length | none (display only) | — | grows with the layout | — | — | roll trim after packing | working | `shirt-pdf-roll-36in` (one page) |
| Fabric Folded | `--layout-fabric-folded yes/no`; key `fabricFolded` | `true`, `false` | `false` | `mediaType` `fabric` | not boolean: exit 64 | `fabric_folded` (half width) | working | `shirt-fabric-folded-with-nap` (SVG width 75 cm of 150 cm) |
| Fabric Width | `--layout-fabric-width`; keys `fabricWidth`, `fabricHeight` | number ≥ 0 | `0` (uses `pageWidth`) | `mediaType` `fabric` | negative: exit 64 | `fabric_width`, `fabric_height` | working | `shirt-fabric-open-margins` (60 in page) |
| Selvedge Width | `--layout-selvedge-width`; key `selvedgeWidth` | number ≥ 0 | `0` | `mediaType` `fabric` | negative: exit 64 | margins via `syncFabricMarginsFromSelvedge` | working (accepted) | `shirt-fabric-folded-with-nap` |
| Margins | `--layout-margins`, `--layout-margin-*`, `--layout-ignore-margins`; keys `marginTop`, `marginLeft`, `marginRight`, `marginBottom` | number ≥ 0 | `0.25` | — | negative, `marginTOP`: exit 64 | `margin_*` | working | `shirt-pdf-tiled-custom-margins`, `shirt-fabric-open-margins`, `settings-invalid-values` |
| Oversized pieces dialog: Split the pieces | `--layout-oversize split` | `split` | stop, exit 70 | `--layout-export` | with `dxf`, `pdf-tiled`, `gcode`, `3mf`, Marker, Nested: exit 64 | `--oversize split` → `processLayoutOversized` | working | `shirt-oversize-split-pdf`, `shirt-oversize-split-svg`, `shirt-oversize-split-dxf`, `jacket-split-*`, `shirt-oversize-no-choice` |
| Oversized pieces dialog: Make the media longer | `--layout-oversize extend` | `extend` | stop, exit 70 | `--layout-export` | a piece wider than the media stays unplaced: exit 70 | `--oversize extend` | working | `shirt-oversize-extend-too-wide`, `oversize-unknown-choice` |

## Layout and export selection

| UI setting | CLI option | Accepted values | Default | Dependencies | Constraints | SeamlyLayout mapping | Status | Test coverage |
|---|---|---|---|---|---|---|---|---|
| Material tabs | `--layout-material` | material name, any case | the only material | `--layout-export` | several materials and none named: exit 64; unknown: exit 64 | `--material` → `selectMaterial` | working | `jacket-no-material`, `shirt-material-fabric`, `shirt-material-unknown`, `TST_VCommandLine::LayoutChoiceOptions` |
| Multisize tabs (Marker, Nested, Base Size, Size) | `--layout-view` | `marker`, `nested`, `base`, size value, full label | first tab | multisize pattern, no `--gsize`/`--gheight` | individual pattern, unknown view, with `--gsize`: exit 64 | `--view` → `selectLayoutView` | working | `jacket-fabric-marker-pdf`, `jacket-fabric-nested-svg`, `jacket-fabric-base-dxf`, `jacket-lining-size-400-dxf`, `jacket-unknown-view`, `shirt-view-on-individual`, `view-with-gsize` |
| Export menu format | `--layout-format` | `svg`, `pdf`, `pdf-tiled`, `png`, `jpg`, `jpeg`, `dxf`, `hpgl`, `ps`, `eps`, `gcode`, `3mf` | `svg` | `--layout-export` | rules of `export_rules.rs`; unknown: exit 64 | `--export` | working (gcode, 3mf: exit 69, no module) | `shirt-*` format cases, `unknown-format`, `rust:individual_layout_formats_follow_the_paper` |
| Save dialog path | `--layout-output` | file path; folder must exist | — (required) | `--layout-export` | missing: exit 64 | `--output` | working | `export-without-output`, `output-directory-missing`, `shirt-relative-output` |
| Export > SVG text submenu | `--svg-text-mode` | `designerFont`, `singleLineFont`, `hersheyStrokes`, `asSupplied` (any case) | `designerFont`; `asSupplied` for path labels | `--layout-format svg` | other format: exit 64 | `--svg-text-mode` | working | `shirt-svg-designer-font`, `shirt-svg-single-line-font`, `shirt-svg-hershey-strokes` (no `<text>`), `shirt-svg-as-supplied`, `text-mode-with-pdf` |
| Export > DXF submenu (R13, CLO3D, AAMA R11); HP-GL pens | `--layout-export-options` | JSON object: `dxfVersion`, `aama`, `clo3dGroup250`, `createAnnotatedVersion`; HP-GL `mode`, pens | DXF-ASTM R13 | `--layout-export` | not a JSON object: exit 64 | `--export-options` | working | `shirt-dxf-r13` (AC1012), `shirt-dxf-aama-r11` (AC1009), `shirt-dxf-clo3d` |
| Layout settings file | `--layout-settings` | JSON file | SeamlyLayout saved settings | `--layout-export` or `--layout-info` | missing file, invalid JSON, unknown key, invalid value: exit 64 | `--settings` | working | `settings-missing-file`, `settings-invalid-json`, `settings-invalid-values` |
| none (query) | `--layout-info` | flag | — | one pattern | export choices: exit 64 | `--info` | working | `info-shirt-roll`, `info-shirt-oversized`, `info-jacket-roll`, `info-with-format`, `richmond-handoff-info` |

## Direct layout flags and no packing

| CLI option | Status | Test coverage |
|---|---|---|
| `--layout-<name>` flags (see `CLI_PARAMETERS.md`) | working | `flags-*` (roll width page 24 in, A4 landscape mm, custom tile 10×12 in, fabric 150 cm folded with nap, flags win over the file, ignore margins and mode/gap change the output, conflicts, bad numbers, invalid values, info), `TST_VCommandLine::LayoutFlags*`, `StartupOptionsTests::layoutFlags_storedInTableOrder`, `rust:settings_flags` |
| `--layout-no-packing` | working | `no-packing-svg`, `no-packing-pdf`, `no-packing-dxf`, `no-packing-jacket-lining`, `no-packing-with-view`, `TST_VCommandLine::LayoutNoPackingCombinations`, `StartupOptionsTests::noPacking_refusesLayoutChoices` |
| Roll trim and one page (legacy `--crop`, `--unite`) | working | `shirt-pdf-roll-36in`, `flags-roll-width-pdf` |

## Non-layout options

| CLI option | Accepted values | Default | Dependencies | Constraints | SeamlyLayout mapping | Status | Test coverage |
|---|---|---|---|---|---|---|---|
| `-h`, `--help` | flag | — | — | — | — | working (console and redirected) | `help`, `console-help-without-redirection` |
| `-v`, `--version` | flag | — | — | — | — | working | `version` |
| `-t`, `--test` | flag | — | one pattern | with `--layout-export`/`--layout-info`: exit 64 | — | working | `test-mode`, `test-mode-relative-paths`, `test-mode-two-files`, `test-and-export` |
| `-m`, `--mfile` | measurement file; relative to launch folder | pattern's file | test, export or info mode | missing file: exit 66 | handoff geometry | working (relative path fixed) | `shirt-measurement-override`, `missing-measurements`, `shirt-relative-paths`, `TST_VCommandLine::MeasurementOverrideIsAbsolute` |
| `-x`, `--gsize` | size in cm from the help list | requires --gsize or --basesize | multisize pattern | invalid value: 64; individual pattern: 65; with `--layout-view`: 64 | one-size handoff | working | `jacket-gsize-gheight-one-size`, `gsize-invalid-value`, `gsize-individual-pattern` |
| `--basesize` | no argument | requires --gsize or --basesize | multisize measurements | with --gsize: 64; missing selection: 64; individual pattern: 65 | measurement base size; --layout-view selects an explicit view | working | basesize-* |
| `-e`, `--gheight` | height in cm from the help list | base height | multisize pattern | as `--gsize` | one-height handoff | working | `jacket-gsize-gheight-one-size` |
| `--layout-export` | flag | — | `--layout-output`, one pattern | see combinations in `CLI_PARAMETERS.md` | `--svg-stdin` + `--export` | working | all export cases, `export-without-pattern`, `export-two-patterns` |
| `--no-scaling` | flag | scaling on | — | — | — | working (parsed; GUI display only) | `TST_VCommandLine::LayoutChoiceOptionsAppearInHelp` |
| filename | `.sm2d` path; relative to launch folder | — | — | missing: exit 66 | — | working | `missing-pattern`, `shirt-relative-paths` |
| `--layout-scale` | — | — | — | removed: PNG export is fixed at 96 px/in, the value was ignored | — | removed | `legacy-layout-scale`, `TST_VCommandLine::LayoutScaleOptionIsRemoved`, `StartupOptionsTests::headlessExport_scaleOptionIsRemoved` |
| `--text2paths` | flag | text kept as text | `--layout-export`; format `svg`, `pdf`, `pdf-tiled`, `ps`, `eps` | with `--svg-text-mode` or another format: exit 64 | `--text2paths` → SVG `hersheyStrokes`; PDF `setPdfTextAsPaths` (svg2pdf `embed_text = false`) | working | `shirt-svg-text2paths` (no `<text>`), `shirt-pdf-text2paths` and `shirt-pdf-tiled-text2paths` (no `/FontFile`; `shirt-pdf-roll-36in` has one), `shirt-eps-text2paths`, `text2paths-with-*`, `text2paths-without-export`, `TST_VCommandLine::Text2Paths*`, `StartupOptionsTests::textAsPaths_vectorFormatsOnly`, `rust:pdf_text_as_paths_writes_no_font` |
| `--basename`, `--format`, `--exportOnlyDetails` and the other legacy writer options | — | — | — | removed: exit 64 "Unknown option" | — | removed | `legacy-*`, `TST_VCommandLine::RemovedLegacyOptionsFailToParse` |

## Transport and process behavior

| Behavior | Status | Test coverage |
|---|---|---|
| Handoff through stdin, no intermediate file; 2 MB jacket2 handoff arrives whole | working | `jacket-handoff-not-truncated` (SEAMLYLAYOUT_SAVE_HANDOFF copy ≥ 1 MB, ends `</svg>`, valid XML) |
| `paths/seamlyLayoutApp` resolves SeamlyLayout | working | every export case (isolated `SEAMLY_DATA_DIR` ini) |
| A missing `paths/seamlyLayoutApp` file falls back to the install folder (`SeamlyLayout\SeamlyLayout.exe`) | working | `layout-app-installed-folder` |
| SeamlyLayout cannot start | working, exit 69 | `layout-app-not-a-program` |
| stdout carries only the result in layout modes | working | `shirt-dxf-r13`, `info-*` (stdout parses as JSON) |
| Errors print on the parent console without redirection | working | `console-error-without-redirection` |
| GUI start unchanged (no console option) | working | `gui-start-unchanged`, `gui-start-with-pattern` |
| Failed run keeps an existing output file; success replaces it | working | `shirt-oversize-no-choice`, `shirt-old-output-replaced` |
| Unplaced pieces stop the export | working, exit 70 | `shirt-oversize-extend-too-wide` |
| Richmond pattern end to end (Seamly2D → SeamlyLayout) | working | `richmond-pattern-*` (with `-RichmondRoot`) |
| DXF, PDF, SVG and HP-GL open in independent software | working | `test_cli_import_check.py --inkscape` (ezdxf auditor, pypdf, Inkscape 1.4.4; 59 files) |
