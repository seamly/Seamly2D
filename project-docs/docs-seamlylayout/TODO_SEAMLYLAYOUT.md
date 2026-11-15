# TODO — SeamlyLayout app features

Open SeamlyLayout tasks. Check off finished items; move fully completed tasks to `project-docs/TODO_COMPLETED.md`. Task IDs start with `Layout.`

## Little things to take care of:

- [ ] Layout.X5 - Display the media dimensions (WxH) in each layout tab, of the layout displayed in that tab.
  - [x] Layout.X5.1 - Each tab shows `<W> x <H> <unit>` below its label (`layoutViewMediaSizes`, `LayoutViews::media_sizes`). Width: paper canvas width, or the fabric width (folded too). Height: the tab's layout length. Done Adjusting updates the shown tab. Tests: `media_size_text_uses_the_settings_unit`, `media_sizes_follow_each_tab_layout`. Docs: SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` §42.
  - [ ] Layout.X5.2 - Installed-app check with the next MSI: `AGENT_CHECKLIST.md` §42.
  - [x] Layout.X5.3 - Individual pattern: each material tab over the layout shows `<W> x <H> <unit>` below its name; one material: the header over the layout shows it below "Material: <name>" (`materialMediaSizes`, `layout_views::material_media_size`). A blank canvas or a material with layout tabs shows none. Test: `material_media_size_shows_one_finished_layout`. Docs: SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` §42.
  - [ ] Display wxh below tabs of individual pattern layouts


## Implement

### [ ] Layout.803 - Implement - Fix piece objects that should be split when the piece is split.

- [x] When a piece is split then split the:
  - [x] reference lines -- `piece_divider.rs`: `stripe` and `plaid` are open lines that end at the cut lines, like `internal_path`. Before, each went whole to the nearest part.
  - [x] internal paths -- no code change: already open lines that end at the cut lines; a closed internal path becomes one open line per part. New test pins the closed case.
  - [x] drill holes -- a drill hole is a circle: it stays whole, on the part nearest its centre. Before, every drill hole went to part 1 (no anchor point). Cut lines move off drill centres.
  - [x] grainlines -- validated (trousers.sm2d, Union piece is split at the same place as the grainline and the halves of the grainline is shown on each split piece)
  - Tests: `markings_split_with_the_piece`, `cut_line_misses_drill_centres`. Docs: SVG-DATA-ATTRIBUTES split parts, PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` §48.
- [ ] Layout.803.1 - Installed-app check with the next MSI: `AGENT_CHECKLIST.md` §48.

## VERIFY

### [ ] Task Layout.93 - Verify - DXF - for each piece, use its 'piece tool' properties and attributes as source of truth instead of labels for names, letter, quantity, material, foldline, etc.; track this data with the layout pieces.

This information should be tracked in the layout data even if it's not shown in a label. The DXF export can then access piece properties instead of label data to generate the exported file.

- [x] Layout.93.1 DXF `Material:` from `data-material`. Today every block writes `Material:Fabric` (`drawing.rs` `DEFAULT_MATERIAL`), also for Lining. Keep the default as 'Fabric'.
  - Done: `converter::material_from_piece`; an empty or absent `data-material` gives `Fabric`. Test: `piece_material_comes_from_data_material`.
- [x] Layout.93.2 DXF layer 7 grainline from `data-grainline-angle` when the grainline is hidden. Today jacket2 Подборт has no layer 7 line. --> don't add a grainline in dXF if the piece grainline is hidden. "A hidden grainline indicates that there is no grainline"
- [x] Layout.93.3 Missing Piece Data: a hidden label is not missing data. Report only data that Piece Properties lack. "Hidden label(s) don't indicate anything. Use piece properties, not piece labels, to get data for DXF export"
  - Layer 15 holds only the labels the piece shows; a hidden label writes no layer 15 text (user decision). The piece system text carries the data.
  - Done: `MissingPieceData` has no label flag. `Quantity:` reads `data-quantity` only, never a "Cut N" label line. Tests: `hidden_label_is_not_missing_data`, `quantity_comes_from_data_quantity_only`.
  - Layout.93.2 needs no code: a hidden grainline writes no layer 7 line, and Missing Piece Data still reports "no grainline".
- [x] Layout.93.4 Seamly2D handoff: add `data-*` attributes for Rotation, Tilt, Fold position, Orientation, Annotation and Forbid flipping. Update `SVG-DATA-ATTRIBUTES.md` and `NEW-ATTRIBUTES.csv`.
  - Done: `data-rotation` (step: 360, 180, 90, 1), `data-tilt` (degrees, negative clockwise), `data-flip` (Horizontal, Vertical, Both), `data-orientation` (Left, Right), `data-annotation`. A value that allows nothing writes no attribute. `VLayoutPiece::setPieceProperties` keeps the Piece Properties, also with a hidden label; `VPieceLabelData::rotationStepDegrees`.
  - Fold position is gone (Layout.97): `data-on-fold` carries the fold. Forbid flipping is now Flip (Layout.98): `data-flip`.
- [x] Layout.93.5 DXF piece system text from them: `Rotation:`, `Tilt:`, `Fold:`, `Flip:` (D6673 §4.3.1.2). Values: Layout.95 (Rotation), Layout.96 (Tilt), Layout.97 (Fold), Layout.98 (Flip).
  - Done: after `Quantity:`, in D6673 order: `Rotation:180`, `Flip:Y`, `Tilt:-15`, `Fold:N`, then `Material:`. Rotation, Flip or Tilt None writes no line. `converter::rotation_from_piece`, `flip_from_piece`, `tilt_from_piece`, `piece_is_on_fold`; `write_astm_block`.
  - Decision taken: Flip Horizontal (left to right) is `Y`, Vertical is `X` (D6673: X flips about the horizontal axis). Both is `Flip:Y` with a `Rotation:` step that reaches 180 (user decision; replaces `XY`, which D6673 does not define).
  - Decision taken: Tilt has its sign: `+15` counterclockwise, `-15` clockwise. A direction with 0 degrees writes no line.
  - Decision taken: every tagged piece writes `Fold:Y` or `Fold:N`. `Fold:Y` needs a fold line (Layout.97.8).
  - Decision taken: DXF-AAMA writes none of the four: AAMA piece text has no such identifiers.
- [x] Layout.93.6 Tests: `TST_SvgComponentTags`; Rust converter and writer tests with hidden labels and grainlines.
  - `TST_SvgComponentTags::PieceCarriesPieceProperties` (hidden label); `TST_VPiece::LayoutPieceKeepsPieceProperties`, `RotationStepDegrees` (temporary test program: passed); `astm_piece_test::layout_rules_come_from_piece_properties`; `writer_astm_test::layout_rule_text_follows_quantity_in_d6673_order`; `graded_sample_blocks_follow_d6673` expects `Fold:N`. Hidden labels and grainlines: Layout.93.1–93.3 tests.
  - Checked: `cargo test --workspace` 605 passed; `cl /Zs` on every changed `.cpp`. Not run: Qt suites (next MSI).
  - Docs: `SVG-DATA-ATTRIBUTES.md` (piece attributes), `NEW-ATTRIBUTES.csv`, `seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`.
- [ ] Layout.93.7 In the next MSI build: export jacket2 as DXF-ASTM (R13) and check the file in aw.fyi. The piece text (Material, Rotation, Flip, Tilt, Fold) comes from Piece Properties, also when a label is hidden. Steps: `AGENT_CHECKLIST.md` section 23.

### [ ] Task Layout.103 — Verify - DXF units: `METRIC` (in mm) or `ENGLISH` (in inches), these are the options for DXF units.

User approved (2026-10-25). The measurement file's unit decides the file units. If a measurement file (.smms or .smis) isn't referenced in the .sm2d file then use the pattern units; if this unit is 'cm' then convert to 'mm; if this unit is different than the pattern unit, convert pattern units to measurement units.

| Pattern unit | `Units:` | Size values | Lengths |
|---|---|---|---|
| mm | `METRIC` | Unchanged | mm, 2 decimals (as now) |
| cm | `METRIC` | Multiplied by 10 (50 → 500) | mm, 2 decimals (as now) |
| inch | `ENGLISH` | Unchanged | inches, 4 decimals |

- `.smms` files cannot be in inches (SeamlyMe offers inches for `.smis` only), so `ENGLISH` is only an option for individual .smis measurement files.
- Size values: `Size:` / `Size Name:`, `Sample Size:`, `.rul` size list and sample size, the "Size N" label text in the DXF.
- The screen, tabs, file names and legend keep pattern units.
- A handoff without `data-units` writes `METRIC` with no conversion.
- [x] Layout.103.1 Seamly2D: `SvgGenerator` writes `data-units` (`mm`, `cm`, `inch`, the pattern unit) on the pattern group; caller `MainWindowsNoGUI`. Qt test in `tst_svgcomponenttags.cpp`.
- [x] Layout.103.2 `SVG-DATA-ATTRIBUTES.md`: add `data-units`; size rows name the DXF conversion.
- [x] Layout.103.3 `seamly_svg2ezdxf`: read `data-units` into `Drawing`; convert cm size values to mm.
- [x] Layout.103.4 `ezdxf2dxfastm`: `ENGLISH` writes every length in inches with 4 decimals: coordinates, text heights, notch depth and width, `Curve Tolerance:`, `.rul` grade steps. `Units:ENGLISH`.
- [x] Layout.103.5 Rust tests: mm, cm and inch handoffs; `.rul`; R11 and R13.
- [x] Layout.103.6 Docs: `seamlylayout_DXF_AAMA.md` (Decisions, McNeel table `Size:` row), `seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`.
- [x] Layout.103.8 Seamly2D picks the file unit: the referenced measurement file unit, else the pattern unit (`MainWindowsNoGUI::exportUnit`, `MainWindow::measurementFileUnit`). `data-units` carries it; size values (`data-size`, `data-sizes`, `data-base-size`, `data-height`, `data-sample-size`) convert from pattern units to it. Decision: conversion on the Seamly2D side; the `--svg-stdin` contract and the Rust DXF writers stay unchanged (lengths are px). Still to check in the app: mm/cm pattern with an inch `.smis` writes `Units:ENGLISH`; inch pattern with a `.smms` in cm writes `Units:METRIC` with sizes in mm.
- [ ] Layout.103.7 `cargo test --workspace` and `ctest --preset debug` pass. Not run yet: the Qt test `PatternCarriesUnits` (`tst_svgcomponenttags.cpp`) and the Qt build. Both run in the next MSI build. Check in the app: in an inch pattern (`.smis` in inches), export DXF-AAMA and DXF-ASTM. The DXF has `Units:ENGLISH` and inch coordinates with 4 decimals. In a cm pattern with a `.smms` file, size 50 writes `Size:500` and `Units:METRIC`.

### [ ] Task Layout.104 — Verify - DXF-AAMA piece text from the McNeel guide

User approved (2026-10-25). DXF-AAMA only; DXF-ASTM and CLO3D keep their output. Guide: `dxf-docs/seamlylayout_DXF_AAMA.md`, "McNeel guide".

- [x] Layout.104.1 `Annotation:` piece text from the piece `data-annotation` (omitted when absent). Converter field on `Block`; writer line after `Size:`.
- [x] Layout.104.2 Block name `<piece name>_<size>` (e.g. `Front_36`); individual pattern `<piece name>`. ASCII letters, digits, `_` and `-` only; use the numbered name when pieces share one; fall back to the svg id. ASTM and CLO3D keep `<svg id>_M`.
- [x] Layout.104.3 `Creation Time:` as `HH:MM:SS` in DXF-AAMA; DXF-ASTM keeps `hh-mm`. Check the `.rul` header time too.
- [x] Layout.104.4 Tests in `writer_astm_test.rs` (`aama_*`).
- [x] Layout.104.5 Docs: `seamlylayout_DXF_AAMA.md` Decisions and McNeel table.
- [ ] Layout.104.6 `cargo test --workspace` and `ctest --preset debug` pass. Not run yet: the Qt build; it runs in the next MSI build. Check in the app: export a multisize pattern to DXF-AAMA. Blocks are `<piece name>_<size>` (e.g. `Front_36`), `Annotation:` follows `Size:` when the piece has one, `Creation Time:` is `HH:MM:SS`, and the `.rul` `CREATION TIME:` too. DXF-ASTM keeps `<svg id>_M` and `hh-mm`.
- [ ] Layout.104.7 `Piece Name:` stays full length; `Material:` stays the full name; no `Category:`; layer 5 keeps the grade reference line.

### [x] Task Layout.101 - Verify - `SEAMLY_DATA_DIR` for SeamlyLayout

- Done: `DataDirectory` (`qt_frontend/src/DataDirectory.{h,cpp}`) reads the variable first in `main()`. Settings go to `<dir>/settings/`, logs to `<dir>/logs/seamlylayout/`. A relative path exits 78; an unwritable one exits 73. Seamly2D's `--svg-stdin` launch passes the variable on.
- [ ] Layout.101.4 In the next MSI build, check in the app: in PowerShell, run `$env:SEAMLY_DATA_DIR="D:/sl_job"; seamlylayout.exe`. Expect `settings\qt6_seamlylayout.ini` and `logs\seamlylayout\log_*.txt` under it, nothing new in `%LOCALAPPDATA%\Seamly\SeamlyLayout`. Run with `SEAMLY_DATA_DIR=rel`, a relative path (exit 78). Run with a path whose parent is a file, so the folder cannot be made (exit 73). Run Seamly2D with the variable set, open Layout Mode: SeamlyLayout uses the same folder. The Qt build also runs in the next MSI build.

### [ ] Task Layout.102 — Verify - DXF-ASTM (CLO3D): the DXF-ASTM export with CLO3D settings, and a setting that can swap the cut line and sew line layers

- Design rule: an export for one vendor's program may change the output so that program can read it. Such a change never changes the standard DXF-ASTM export.
- DXF-ASTM (CLO3D) uses the DXF-ASTM export code with different settings. It is not a second copy of the export code.
- User decision (Sue, 2026-11-01): DXF-ASTM keeps the ASTM D6673 layers. The CLO3D export makes only the changes that CLO3D needs to import the file. It keeps the standard mapping (layer 1 = cut line, layer 14 = sew line) for now.
- CLO3D import option (import side): "Swap Cutting Line and Sewing Line". CLO documentation confirms DXF-ASTM support and that setting. It does not show that SeamlyLayout must swap layers.
- Swap layers 1 and 14 in the CLO3D export only if the CLO3D import test (102.5) shows it is needed.
- Guide: `dxf-docs/seamlylayout_DXF_AAMA.md`, "McNeel guide" (layers 0 to 19, piece text).
- [x] Layout.102.1 Reuse the "DXF-ASTM (CLO3D)" menu item and `clo3d_group_250`. Add `CutSewLayers` (`Standard`, `Swapped`) to `DxfAstmExportOptions` (`writer.rs`). The preset sets `CLO3D_CUT_SEW_LAYERS = Standard` in `do_export_dxf()` (`exports.rs`).
- [x] Layout.102.2 Document the CLO3D import settings in `dxf-docs/seamlylayout_DXF_EXPORT_WORKFLOW.md` and `seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`.
- [x] Layout.102.3 Tests (`writer_astm_test.rs`): the standard export and the preset write layers 1/84/14/87 byte for byte alike; a `Swapped` mapping writes cut on 14/87 and sew on 1/84, group 250 follows the role. `cargo test --workspace` passed.
- [x] Layout.102.4 Integrated MSI build, Qt suites, and Rust workspace tests passed.
- [ ] Layout.102.5 Check in the app (steps: `AGENT_CHECKLIST.md` section 8): export DXF-ASTM (CLO3D), then import the file in CLO3D with "Swap Cutting Line and Sewing Line" off. If the cut line and sew line come in reversed, set `CLO3D_CUT_SEW_LAYERS = CutSewLayers::Swapped` (one line in `writer.rs`) and run the test again.

