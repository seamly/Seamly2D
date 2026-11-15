# TODO — SeamlyLayout app features

Open SeamlyLayout tasks. Check off finished items; move fully completed tasks to `project-docs/TODO_COMPLETED.md`. Task IDs start with `Layout.`

## Little things to fix

- [ ] yy Layout.Y2 - multisize - Split the pieces: The Marker tab and Nested tab should display a messaage that Marker and Nested layouts are unavailable when pieces are split, and should not display the media, as their layouts are disabled when pieces are split.
  - [x] A split run lays out only Base Size and each size (`layout_views::size_kinds`). Marker and Nested tabs stay first with no layout and no media size; the canvas shows `SPLIT_UNAVAILABLE_TEXT` (QML `layoutViewUnavailableText`). Adjust, Export, View off there; Export all tabs skips them. Nested piece-set split code removed. Tests: `split_run_makes_marker_and_nested_unavailable`, `unsplit_run_keeps_marker_and_nested`. Docs: PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, SVG-DATA-ATTRIBUTES, `AGENT_CHECKLIST.md` §49.
  - [ ] Layout.Y2.1 - Installed-app check with the next MSI: `AGENT_CHECKLIST.md` §49.

- [ ] yy Layout.Y3 - multisize - Nested: When a piece has corners but none are 90 degrees then align the sizes along the cutline's longest edge.
  - [x] Stack point Rule 2b: with no right-angle turn point, the lowest turn point wins; the sizes align along its longest edge (straight part only). Same rule in `svg_dom/src/stack_point.rs` and `layout_handoff/handoff_stack_point.cpp`. jacket2: only Detail 3 changes.
  - [x] Tests: `right_angle_is_within_one_degree`, `without_a_right_angle_the_lowest_corner_wins`, `stackPointTakesLowestCornerWithoutRightAngle`. Docs: SVG-DATA-ATTRIBUTES "Multisize stack point".
  - [ ] Layout.Y3.1 - Installed-app check with the next MSI: `AGENT_CHECKLIST.md` §50. The MSI build also runs the full Qt suites.

- [x] Layout.Y4 - Split the pieces: When a piece is split, split the drill hole when needed. Currently 'trousers.sm2d > Along grainline > Keep upright > paper > roll > custom > width=16.00in' produces a split piece "Back" where the stripe and plaid reference lines are split correctly but the drill hole is not split -- the drill hole is located on "Back part 2 of 2" where half of the drill hole lies outside the split line.
  - [x] `piece_divider.rs` `drill_for_parts`: a cut line that crosses a drill circle splits it. Each part's `<circle>` becomes a `<path>` of its arcs, cubic curves that end at the cut line.
  - [x] The part nearest the centre keeps `<circle r="0" stroke="none">` at the centre: the one DXF-ASTM layer 13 drill point (DXF has no half drill hole).
  - [x] `seamly_svg2ezdxf` `extract_drill_hole`: on a split part (`data-split-of`), a drill group without a circle writes no point; a zero-radius circle writes the point with `data-diameter`.
  - Tests: `drill_hole_splits_at_the_cut_line`, `narrow_cell_holds_two_arcs`, `split_drill_hole_keeps_one_drill_point`; two new reject cases for unsplit pieces.
  - Docs: SVG-DATA-ATTRIBUTES split parts, PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, DXF-ASTM compliance layer 13, `AGENT_CHECKLIST.md` §48, new §51 and OPEN row.
- [ ] Layout.Y4.1 - Installed-app check with the next MSI (`AGENT_CHECKLIST.md` §51).

- [ ] yy Layout.Y5 - Tilt: Currently male_shirt.sm2d's piece 'BackPanel Mirrored' is tilted but the grainline isn't tilted. Grainlines should tilt; internal paths, stripe reference lines, plaid reference lines, and drill holes tilt correctly. Fix grainlines so that they tilt, also. 

- [ ] yy Layout.Y6 - Paper > roll : PROHIBIT CHANGING THE WIDTH OF THE ROLL PAPER!!! DO NOT MAKE ME TELL YOU THIS AGAIN!!!!! THE ROLL LENGTH CAN CHANGE BUT THE WIDTH CANNOT! I have submitted multiple prompts to fix this problem, and I don't want to keep wasting tokens to fix this one problem. It's simple. When media=paper and papertype=roll then do not ever change the width of the media or content rect. Also applies to media=fabric --> do not ever change the media width or content rect for fabric (folded) or fabric (not folded)> 

## Implement

### [ ] Layout.803 - Implement - Fix piece objects that should be split when the piece is split.

- [x] When a piece is split then split the:
  - [x] reference lines -- `piece_divider.rs`: `stripe` and `plaid` are open lines that end at the cut lines, like `internal_path`. Before, each went whole to the nearest part.
  - [x] internal paths -- no code change: already open lines that end at the cut lines; a closed internal path becomes one open line per part. New test pins the closed case.
  - [x] drill holes -- a drill hole inside one column stays whole, on the part nearest its centre. Before, every drill hole went to part 1 (no anchor point). Cut lines move off drill centres. A cut line that crosses the circle splits it: see Layout.Y4.
  - [x] grainlines -- validated (trousers.sm2d, Union piece is split at the same place as the grainline and the halves of the grainline is shown on each split piece)
  - Tests: `markings_split_with_the_piece`, `cut_line_misses_drill_centres`. Docs: SVG-DATA-ATTRIBUTES split parts, PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` §48.
- [X] Layout.803.1 - Installed-app check with the next MSI: `AGENT_CHECKLIST.md` §48.

## VERIFY

- Export DXF files & create aw.fyi/dxf

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

### [ ] Task Layout.104 — Verify - DXF-AAMA piece text from the McNeel guide

User approved (2026-10-25). DXF-AAMA only; DXF-ASTM and CLO3D keep their output. Guide: `dxf-docs/seamlylayout_DXF_AAMA.md`, "McNeel guide".

- [x] Layout.104.1 `Annotation:` piece text from the piece `data-annotation` (omitted when absent). Converter field on `Block`; writer line after `Size:`.
- [x] Layout.104.2 Block name `<piece name>_<size>` (e.g. `Front_36`); individual pattern `<piece name>`. ASCII letters, digits, `_` and `-` only; use the numbered name when pieces share one; fall back to the svg id. ASTM and CLO3D keep `<svg id>_M`.
- [x] Layout.104.3 `Creation Time:` as `HH:MM:SS` in DXF-AAMA; DXF-ASTM keeps `hh-mm`. Check the `.rul` header time too.
- [x] Layout.104.4 Tests in `writer_astm_test.rs` (`aama_*`).
- [x] Layout.104.5 Docs: `seamlylayout_DXF_AAMA.md` Decisions and McNeel table.
- [ ] Layout.104.6 `cargo test --workspace` and `ctest --preset debug` pass. Not run yet: the Qt build; it runs in the next MSI build. Check in the app: export a multisize pattern to DXF-AAMA. Blocks are `<piece name>_<size>` (e.g. `Front_36`), `Annotation:` follows `Size:` when the piece has one, `Creation Time:` is `HH:MM:SS`, and the `.rul` `CREATION TIME:` too. DXF-ASTM keeps `<svg id>_M` and `hh-mm`.
- [ ] Layout.104.7 `Piece Name:` stays full length; `Material:` stays the full name; no `Category:`; layer 5 keeps the grade reference line.

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