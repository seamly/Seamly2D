# TODO — SeamlyLayout app features

Open SeamlyLayout tasks. Check off finished items; move fully completed tasks to `project-docs/TODO_COMPLETED.md`. Task IDs start with `Layout.`

## Implement

## VERIFY

### [ ] Task Layout.6 — Verify - DXF-ASTM grading

See `dxf-docs/seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`, "Not exported".

- [ ] Layout.61 Verify grade IDs (`# n`), size blocks (`Size Name:`), layer 5, and `.rul` output.
  - [ ] Layout.61.7 Validate the multisize DXF-ASTM export with aw.fyi (DXF.8.4).
    - [ ] Layout.61.7.2 Export jacket2 Base Size 500 as DXF-ASTM R13; check in aw.fyi. Expect all 8 pieces graded and no merged Detail pieces, layer 5 mismatches, or duplicate vertices. Nested has no DXF-ASTM export (Layout.67). User check: `AGENT_CHECKLIST.md` §5.
      - [x] Fabric R13, MSI 26.10.2023: `test-seamly-layout/layouts/jacket2/20261021/jacket2_40-146_R13_Fabric_BaseSize500_202610211408_apparelwerks_compliance_report.pdf`. 7 of 7 pieces graded: 7 blocks, 7 layer 5 lines, `# n` texts in each block. No departures. 5 advisories: Unicode escapes, L84 densification, validation spans, curve tolerance, piece-113 layer 14 curve points.
      - [x] Fabric R13, MSI 26.10.2312: `test-seamly-layout/layouts/jacket2/20261023/jacket2_40-146_R13_Fabric_BaseSize500_202610231417-dxf-report.pdf`. 7 of 7 graded, 0 departures, the same 5 advisories. Page 1 caption "L5 endpoint mismatch up to 227.990 — sizes not a pure translation": accepted as a viewer note (user decision). Back's layer 5 line ends on two corners with different grade rules; only a graded nest needs the same line in every size (D6673 §4.3.1.5).
      - [ ] Lining R13: Detail 4, the 8th piece, is in the Lining document. Export its Base Size 500 and check it in aw.fyi.
      - [ ] R12: the same check for Fabric and Lining.

    | aw.fyi finding | Report | Cause | Task |
    |---|---|---|---|
    | Departure "Sample size points incomplete" | both | 6 pieces share `Piece Name:Detail`; aw.fyi reads their blocks as one graded piece | Layout.26.5, done after these files |
    | Departure "Excluded layers present in graded sizes" | both | Same merge | Layout.26.5 |
    | "L5 endpoint mismatch up to 103.74; 4 blocks without L5" | Nested | Same merge. The 4 blocks are the ungraded Detail sample sizes | Layout.26.5 |
    | Duplicate vertices, layers 84 (54) and 87 (450) | Nested | Graded key points 0.000001–0.01 mm from a dense vertex | Layout.27.9, done after these files |
    | Departure "Corners missing from layer 2": Detail (4) size 240, L1 pt 14 | Marker | 6.5° bend over 10 mm; no tangent break | Reader advisory (DXF.23) |
    | Advisory: no densified points, 13 pieces | Marker | Rectangles (Комір) and triangles (Detail (5)): straight sides | No action |
    | Advisory: more curve points than the curves need (piece-113 L14, Detail (4) size 500 L1) | Nested | Long gentle curves | No action (advisory) |
    | Advisory: validation spans, curve tolerance | both | Pass | — |
    | No finding: `Piece Name:piece-201_s720_M` | both | Cyrillic name emptied by `sanitize_ascii`; aw.fyi cannot group the sizes | Layout.26.4, done after these files |

### [ ] Task Layout.71 — Verify - automatic media width

Multisize layouts expand right for wide pieces and down for long pieces. Only individual patterns split pieces (Layout.72).

- [ ] Layout.71.6 Next MSI: jacket2, 10-in Paper Roll. All tabs should widen without warnings; Adjust Mode and SVG export should match.

### [ ] Task Layout.28 — Verify - built-in seam line

Seamly2D now sends both cut and seam lines for built-in seam allowances. SeamlyLayout draws and exports both. Only app verification remains.

Status (2026-10-12): code, docs, Rust (472 pass) and Qt tests pass; MSI 26.10.1220 built. Changes are local to `run-seamlyLayout` (not pushed).

- [ ] Layout.28.7 Next MSI: run `TEST_CHECKLIST.md` §5, "Piece with a built-in seam allowance".

### [ ] Task Layout.68 — Verify - add grid to prove PDF export scale

Cause (MSI 26.10.1311): PDF used 72 dpi instead of SVG’s 96 dpi, scaling pages and pieces by 4/3. Create an option to add a grid using pattern units for easy layout scale verification. Include the grid in each layout except 'Marker'. For DXF exports, exclude the grid.

- Marker `jacket2_40-146_202610131244.pdf` (48 in × 500 in roll): MediaBox `4608 × 48000` pt = 64 in × 666.7 in. Expected `3456 × 36000` pt.
- Nested `jacket2_40-nested_146_202610131245.pdf`: `4608 × 5003` pt. Expected `3456 × 3752` pt.
- EPS and PS of the same tabs are correct: `0.75` scale, BoundingBox `3456 36000`.

- [x] Layout.68.5 In Settings create an option to add a verify scale grid; if pattern unit=(mm or cm) then grid is 9 blocks arranged 3x3 --> 3cm wide x 3cm high, if pattern unit=inches then grid is 4 blocks arranged 2x2 (2 inches wide x 2 inches high)
- [x] Layout.68.6 When creating layouts, place the grid after all other pieces have been placed. Exception: Do not place a grid on a Marker layout.
- [x] Layout.68.7 When creating DXF exports, exclude the grid.
- [x] Layout.68.9 When creating HPGL cutter exports, exclude the grid.
  - Done: `SettingsModel::scaleGrid` (`scaleGrid` in the settings file and JSON), Settings checkbox under Piece Gap; `scale_grid.rs`, placed by `do_process_layout` one piece gap below the pieces and the legend, media grows (tiled: more rows); Marker (more than one size, no piece-sets) gets none. DXF: `clone_dxf_layout_doc` removes it. HPGL: mark class, so cut mode leaves it out. Not on the paper-sheet PDF, which repacks the pieces.
  - Tests: 4 `scale_grid` tests; `layout_utils` `jacket2_scale_grid_on_every_layout_but_marker`; `hpgl_writer` `cut_mode_drops_the_scale_grid`; `SettingsModelTests::scaleGrid_defaultsOffAndRoundtrips`. Doc: `SVG-DATA-ATTRIBUTES.md`, "Scale check grid".
- [x] User check, PDF true size: Marker tab PDF page matches the layout. Passed (MSI 26.10.2023). Moved to `TODO_COMPLETED.md`.
- [ ] Layout.68.10 Test/Verify - Check Settings for grid option.
- [ ] Layout.68.11 Test/Verify - Check grid in layouts; Check grid is not included in Marker.
- [ ] Layout.68.12 Test/Verify - Check grid in exports; Check grid is not in DXF or HPGL cutter files.
- [ ] Layout.58.13 Test/Verify - Check scale; Print a small layout, check scale for correct grid size (3cm x 3cm, or 2" x 2")

### [ ] Task Layout.70 — Verify - "Some pieces could not be placed" dialog: piece names and length

jacket2 Marker, MSI 26.10.1311: 112 entries such as "Detail (size 240)" (`jacket2_screenshot_1.png`). The 6 "Detail" pieces cannot be told apart. The dialog has no scroll area and grows past the screen.
It might be difficult to reproduce this error as we now have Layout.71 automatic media width and Layout.25 automatic media height

- [x] Layout.70.1 Use the numbered display name: "Detail (3)" (same rule as Layout.26.5).
- [x] Layout.70.2 One line per piece, then its sizes: "Detail (3): 220, 240, 260". Group by `data-parent` (as Layout.26.1).
- [x] Layout.70.3 `Main.qml` `warningDialog`: scroll area; the OK button stays visible (as Layout.26.2).
- [x] Layout.70.4 Rust test: warning text for a jacket2 Marker layout that leaves pieces out.
- [ ] Layout.70.5 Next MSI: run `TEST_CHECKLIST.md` §7, "Left-out pieces warning".

### [ ] Task Layout.27 — Verify - Fix jacket2 DXF-ASTM grading

Prior results (jacket2, sizes 220–720): Nested graded 4 of 8 pieces; Marker placed 96/208 pieces and lost grading on others. Base Size 500 in MSI 26.10.1311 graded 4 pieces, with rules accurate to 0.011 mm. Detail (3), (6), (7), (8) remained ungraded. Evidence: `test-seamly-layout/layouts/jacket2/`, aw.fyi reports, `jacket2_40-146_basesize_R13_202610131248.dxf`.

Note: Seamly2D performs the grading and passes these graded pieces to SeamlyLayout in the input stringified SVG. Do not perform grading in SeamlyLayout.

Note: The piece names in the jacket2 pattern have changed, so items below may refer to piece names

- [x] Layout.27.1 Re-export after Layout.25; confirm sample size 500 is no longer omitted from Marker. Rust test `jacket2_marker_extends_paper_roll`: 208 of 208 placed on a 48 × 500 in roll. Marker has no DXF-ASTM export (Layout.67). User check: Layout.25.7.
- [x] Layout.27.2 If a sample-size piece is unplaced, warn: "size 500 does not fit the media". `grade_drawing` names it.
- [x] Layout.27.3 Make turn points consistent across all sizes of each piece (cutline, seamline, internal paths). Seamly2D `VAbstractPiece::turnPointIndices` currently changes at near-threshold corners. After `prepareMultisizePieceLists`, share turn points across sizes using node vertex indices in `VLayoutPiece` / `VLayoutPiecePath`; skip paths whose node counts differ.
  - [x] Layout.27.3.1 Code: `unifyTurnPoints` (`63ab2447a3`).
  - [x] Layout.27.3.2 Run the Qt suites with `local_build_msi.ps1`. Passed in MSI 26.10.1921: 6 turn point tests in `TST_SvgComponentTags`.
  - [x] Layout.27.3.3 Next MSI: export jacket2 Base Size 500 to DXF-ASTM. With Layout.27.13: all 8 graded, no "other turn points" warning.
    - Passed (MSI 26.10.2023): no "other turn points" warning. Fabric: 7 of 7 graded (Layout.61.7.2). Lining (Detail 4) not checked.
  - Note: the MSI 26.10.1416 handoff has the same `data-turn-points` as the old fixture. Detail (7) and (8) are consistent; Detail (3) and (6) differ in shape (Layout.27.13).
- [x] Layout.27.4 Give `grade_anchors` a distinct error when a key point is missing from the dense line. `GradeError::KeyPointOffLine`.
- [x] Layout.27.5 Name the first size whose turn points differ from the sample size. jacket2 fixture: Detail (3) size 660, Detail (6) size 220, both on the cut line.
- [ ] Layout.27.7 Add jacket2 handoff fixture to `crates/cxxqt_bridge/test_data/`; end-to-end test should grade every piece and write `.rul`.
  - [x] Layout.27.7.1 `jacket2_40-146_handoff.svg` (MSI 26.10.1416) replaces `jacket2_40-146_202610121819.svg`; `jacket2_base_size_export_names_and_grades_each_piece` grades 6 and writes `.rul`.
  - [ ] Layout.27.7.2 Expect 8 graded after Layout.27.13.
    - MSI 26.10.2023 fixture: 7 of 8 graded, `.rul` written. Detail 4 (Lining) warns: "size 220 has other turn points than size 500 on the cut line". Tests `jacket2_base_size_export_names_and_grades_each_piece`, `jacket2_assigned_names_reach_the_base_size_export` pin it.
- [x] Layout.27.8 If nothing grades, show "No piece graded: no .rul file written". `astm_grading::grade_drawing` writes `.rul` only when at least one piece grades.
- [x] Layout.27.11 Fix near-duplicate written vertices. Example: Подборт layer 87 has `(333.01, 626.21)` followed by `(333.01, 626.22)`. Increase `SNAP_VERTEX_MM` from 0.01 to 0.02, or compare rounded coordinates. Test: no consecutive written vertices 0.01 mm apart or closer. Done: `DUPLICATE_TOLERANCE_MM` and `SNAP_VERTEX_MM` = 0.025; jacket2 test found 6 before the fix.
- [x] Layout.27.12 Make line-end turn points consistent across sizes. Detail (7) differs from size 660; Detail (8) differs at 520–640, 680 and 700. `astm_contour::straight_run_ends` flips at its 10-mm / 2× break thresholds (e.g., 9.97 mm at size 640 vs 10.2 mm at 660). Decision (2026-10-13): Option 1.
  - Option 1 (chosen): Seamly2D checks node line ends, then `unifyTurnPoints` shares flags across sizes. SeamlyLayout stops adding line ends to tagged paths. Refresh old fixtures.
  - Option 2: SeamlyLayout shares line-end flags across sizes, using new node-index attributes from Seamly2D.
  - [x] Layout.27.12.1 Seamly2D: `turnPointIndices` marks line ends; `unifyTurnPoints` shares them (`5bcb1f3cf1`).
  - [x] Layout.27.12.2 SeamlyLayout: no line ends added to tagged input (`5bcb1f3cf1`).
  - [x] Layout.27.12.3 Refresh fixtures from the next MSI handoff: `male_shirt_pieces.svg` (expect 16 FrontPanel turn points, not 14); jacket2 (expect all 8 pieces graded). Done with MSI 26.10.1416: FrontPanel 16. jacket2: 6 of 8 (Layout.27.13).
  - [x] Layout.27.12.4 Build and user check: Layout.27.3.2, Layout.27.3.3.
- [ ] Layout.27.13 jacket2 Detail (3) and (6) lines change shape between sizes, so they cannot grade. Found in the MSI 26.10.1416 handoff. Decision (2026-10-14): fix both in Seamly2D.
  - Detail (3), sizes 660–700: the seam line and cut line reverse 0.2 mm past a corner (178.7° turn, then a 0.2 mm segment). The corner gets two turn points; other sizes have one. Cause: `VAbstractCurve::FromBegin` keeps a curve end in the same pixel as the node point, so the curve overshoots it.
  - Detail (6), sizes 220–340: the cut line's last seam allowance corner is two vertices (67° each, 9.3 mm apart). From size 360 it is one vertex (131°). Cause: `AngleByLength` cuts a miter longer than 2.4 × the width; the miter is near that limit.
  - The extra vertices also change the node count, so `unifyTurnPoints` skipped these lines.
  - [x] Layout.27.13.1 Detail (3): `vpiecepath.cpp` `curveSegmentPoints` ends a curve at its node point when both round to the same pixel. Test `TST_VPiece::CurveEndInNodePixelEndsAtNode`.
  - [x] Layout.27.13.2 Detail (6): the multisize handoff cuts a ByLength corner in every size when one size cuts it (`MainWindowsNoGUI::unevenCornerCuts`, `VPiece::setForcedCornerCuts`). Only those sizes are built again. Test `TST_VPiece::ForcedCornerCutCutsShortMiter`.
  - [x] Layout.27.13.3 Run the Qt suites. Done: `Seamly2DTests` passed, with both new tests; `ParserTest` and `TranslationsTest` passed; `CollectionTest` exited 1 on the first run and passed 44 of 44 on a rerun. These were the binaries that the MSI 26.10.1421 build compiled.
  - [ ] Layout.27.13.4 Next MSI: run `capture_handoff_jacket2.bat` again; replace `jacket2_40-146_handoff.svg`; expect 8 graded (Layout.27.7.2).
    - MSI 26.10.2023 capture: `C:\Users\susan\seamly2d\layouts\handoff_jacket2.svg`. Two material groups: Fabric 7 pieces, Lining 1 (Detail 4).
    - Done: the capture replaces `jacket2_40-146_handoff.svg`; the jacket2 tests follow (names, grainlines, labels). Result: 7 of 8 graded; Detail 4 does not (Layout.27.13.6).
    - MSI 26.10.1421 result: still 6 of 8 graded. The fixture was not replaced.
    - Detail (3): sizes 660 and 680 are fixed. Size 700 has one extra turn point, 3.7 mm before the corner at node 247.
    - Detail (6): no change. Sizes 220–340 still cut the corner and 360+ do not, so the forced cut did not apply.
    - Suspected cause (not verified): the corner and turn point lists count spline interior nodes. Both pieces end a curve at a `cutSplinePath` point (217: `SplPath_А_А3-22`; 209: 50 mm along path 80). An interior node can be on that curve segment in some sizes only. The node count then differs between sizes, so `unevenCornerCuts` and `unifyNodeTurnPoints` skip the piece.
    - Detail (3) evidence: interior node А20 is on the curve from about size 540 up, and past О6 in smaller sizes. Size 700 flags А20 as a turn point; sizes 560–720 have the same vertex without the flag. А20 is smooth: its curve bends 1.4° there.
    - Spline node terms: end node (first or last spline point, or a spline point where the tangent breaks), interior node (spline point with the same tangent on both sides; never a turn point), control node (off-curve handle).
    - Next MSI check: the Seamly2D log (`%LOCALAPPDATA%\Seamly\Seamly2D\logs\seamly2d-pid*.log`) has the line "Multisize layout: 26 sizes; 0 of 8 pieces differ in node count between sizes." Lines starting `Piece "Detail"` name a piece that still differs.
  - [ ] Layout.27.13.6 Detail 4 cut line turn points differ (MSI 26.10.2023 capture; Seamly2D log of that run):
    - Node counts (seam line/cut line) differ: Detail 4 280 and 520: 5/5, other sizes 6/7. Back 280, 480, 700: 5/5, others 6/6. Sleeve 1 620: 3/3, others 4/4. `unifyTurnPoints` skips such a piece.
    - Likely cause, from the code (not yet checked on the capture): `RemoveDublicates` erases the later of two vertices closer than 0.1555 mm. A curve point just before a node stays and the node vertex goes; the node match tolerance is 0.01 px, so the node is lost in that size only.
    - [x] Layout.27.13.6.1 Fix: `VAbstractPiece::snapNodeVertices` moves the nearest vertex back onto such a node (seam line, cut line, built-in seam line, internal paths). Test `TST_VPiece::SnapNodeVerticesRestoresErasedNode` (harness: passed). `cl /Zs` on the changed `.cpp`. Not run: Qt suites.
    - Still open after the fix (geometry, not node count):
      - Sizes 220, 240: one more cut line turn point, a -29° bend followed by a 1.4 mm segment at about 10% across, 23% down the piece box. Other sizes have no such bend.
      - Size 520: the cut corner near the top left is one vertex (+121°); other sizes cut it into two vertices 8 mm apart (+61°, +61°), although the log says "Every size cuts them".
    - [ ] Layout.27.13.6.2 Next MSI: capture jacket2 again. The log line should read "0 of 8 pieces differ in node count". Check the Detail 4 sizes 220, 240 and 520 cut line in the capture; replace the fixture.
  - [x] Layout.27.13.5 Seamly2D: a spline interior node is a curve point, not a node vertex (`isSplineEndNode` in `vpiecepath.cpp`). A spline point is an end node when it is first or last, the two curves that meet at it have different tangents, or a control node has length 0. The handoff logs each piece whose node counts differ between sizes, and each forced corner cut. Tests `TST_VPiece::SplineNodeVertices`, `TST_VPiece::CubicBezierPathNodeVertices`, `TST_VPiece::SplineInteriorNodeKeepsNodeCount`. Done: `cl /Zs`. Qt suites passed in `local_build_msi.ps1` (MSI 26.10.1921).
    - First Qt run failed: row `SplineNodeVertices(tangent breaks)` asked for a point that cannot exist. `VSplinePoint` makes the control node angles opposite. `VCubicBezierPath::GetSpline()` turns the control node after an interior node opposite the one before. Only a control node of length 0 gives a tangent break at an interior node.
    - Fix: `isSplineEndNode` reads the two curves that meet at the point, as drawn. The row now expects a smooth point.
    - User decision: keep both corrections. A tangent break is two curves that meet at a point. Never make an end node smooth. Never drop an end node.

### [ ] Task Layout.74 - Multisize - Nested - each size has its own color, with a size-to-color legend

Currently in the nested layout, all pieces are black. To distinguish between sizes, each size should be assigned a unique color, all pieces of that size will be displayed in that color (cutlines, seamlines, notches, etc.)

- [x] Layout 74.1 - Assign each size a color. `size_colors::size_palette`: smallest size first, golden-angle hue steps, relative luminance at most 0.3. Layout.82 replaced this palette.
- [x] Layout 74.2 - Assign each piece its color based on its size. These colors are displayed in the Nested layout, and included in all exports except DXF (Nested can't be exported to DXF). `color_pieces_by_size`, called by `do_process_layout` (Nested only) and `build_sheet_export_inputs` (paper-sheet PDF).
- [x] Layout 74.3 - Create a color-to-size legend. Place this legend on the Nested Layout. `SizeLegend`: `<g id="size_legend" data-type="size_legend">`, one piece gap below the lowest piece; the media grows to hold it (tiled: more tile rows). Text modes and HPGL treat it as a label. Not on the paper-sheet PDF, which repacks the pieces.
- [ ] Layout 74.4 - Test/Verify using a multisize pattern. Next MSI: jacket2, Nested tab, then Export > SVG, PDF, PNG.
  - [ ] Layout 74.4.1 - Each piece is displayed with its size's color
  - [ ] Layout 74.4.2 - The legend appears in the Layout
  - [ ] Layout 74.4.3 - Exported files retain the colors
  - [ ] Layout 74.4.4 - Exported files retain the legend
  - [ ] Layout 74.4.5 - 5 `size_colors` tests; `layout_utils` `jacket2_nested_colors_sizes_and_adds_legend`, `jacket2_nested_tiled_legend_has_tile_rows`; `svg_label_text` `size_legend_text_is_label_text`; `hpgl_writer` `cut_mode_drops_the_size_legend`. `cargo test --workspace` 533 passed; `ctest --preset debug` 6 of 6. jacket2 Nested PNG render checked by eye. Doc: `SVG-DATA-ATTRIBUTES.md`, "Nested size colors".

### [ ] Task Layout.76 - Verify - Multisize - Filename - each piece must have the size in a label

Currently in Multisize layouts, the pattern pieces don't display the size. Add a single-line text label, or 'Size label', that displays 'Size <size>', where the bottom of the label is 5 pixels above the top of the grainline.

- [x] Task Layout.76.1 - During import from seamly2D, add a 'Size label' above the grainline in multisize pieces that displays the size label. The left canvas should display the pieces with the additional 'Size label'. SeamlyLayout should process the 'Size label' similarly to how it currently processes 'Piece label' and 'Pattern label'
  - Done: `size_labels::add_size_labels`, called by `finish_import`. `<g data-type="size_label">` with "Size <size>", Arial 24 px; text bottom 5 px past the grainline front end, reading across the grainline, so it is level after the layout turns the grainline up. Text modes, HPGL, DXF (layer 15) and the outline search treat it as a label.
  - Decision taken (no grainline): the label sits level at the centre of the piece outline. jacket2 hides its grainlines, so its labels are centred.
  - Tests: 5 `size_labels` tests (one lays out a sideways grainline and checks the label is level, 5 px above the top); `layout_utils` `jacket2_size_labels_reach_the_exports` (8 labels on Size 300, 8 DXF texts). Doc: `SVG-DATA-ATTRIBUTES.md`, "Size labels".
- [ ] Task Layout.76.2 - Test/Verify - Check that exports include the size label. Next MSI: a multisize pattern with visible grainlines; the left canvas shows "Size <size>" above each grainline; Size tab exports (SVG, PDF, DXF) show it.

### [ ] Task Layout.77 - Verify - Import - Separate pieces into fabric, a.k.a 'Material'; display on separate tabs in left import canvas

Currently in the import left canvas, all pieces are displayed together regardless of the fabric type. From Seamly2D, separate pieces into fabric type--> read each piece and create a list of fabric types; group pieces by fabric type; create separate stringified svgs for each fabric type; display each stringified svg in separate tabs in the import left canvas where the tab contains the name of the fabric type.

- [ ] Layout.77.0 - Update Seamly2D to track Material type for each piece:
  - Note: Currently in 'Seamly2D > Piece Mode > Pattern Piece Tool > Labels > Piece Label (or Pattern Label) > Edit Label > Insert'. When the user clicks the 'Insert' dropdown box they can select 'Material: Fabric', 'Material: Interfacing', 'Material: Interlining', and 'Material: Lining' which places the text 'Fabric', 'Interfacing', 'Interlining', or 'Lining' into the label.
  - [x] Layout.77.0.1 - In the .sm2d file, add a new 'material' attribute to Piece group elements. Pattern format 0.7.6 (`v0.7.6.xsd`, converter `toVersion0_7_6`); `VAbstractPiece::getMaterial`, default `Fabric`; read in `VPattern::parsePieceElement`, written by `PatternPieceTool::addAttributes`.
  - [x] Layout.77.0.2 - In 'Pattern Piece Tool > Properties > Properties', add a new 'Material' dropdown field after the 'Letter' field and before the 'Quantity' field. The dropdown selections are 'Fabric', 'Lining', 'Interfacing', 'Interlining' (we may add more choices in the future). Save the user's selection to a new attribute 'material' in the Piece group definition. `pattern_piece_dialog.ui` row `material_ComboBox`; the list shows translated names and stores the untranslated name.
  - [x] Layout.77.0.3 - When handing off to SeamlyLayout, create a separate handoff for each Material type found in the pieces. Decision taken: one stdin document with one pattern group per material (`pattern-1`, `pattern-2`, ...; `data-material`), so the stdin contract stays one SVG. `SvgGenerator::mergeSvgDoms`; tests `TST_SvgComponentTags::PiecesOfEachMaterialShareAPatternGroup`, `OneMaterialKeepsOnePatternGroup`. Checked: `cl /Zs` on the 8 changed `.cpp` files. Qt suites passed in `local_build_msi.ps1` (MSI 26.10.1921).
- [x] Layout.77.1 - Update SeamlyLayout to create layouts for each Material identified in the pieces:
  - [x] Layout.77.1.1 - in left import canvas, display each separate handoff in a tab labeled with its material type. `material_split::split_by_material`, called by `finish_import`; `Main.qml` `materialTabs`; `AppController::select_material`.
  - [x] Layout.77.1.2 - in left import canvas, when user clicks 'Create Layout', create layouts for the currently selected left import canvas Material tab. The shown material's document is `input_dom`. Decision taken: a tab switch drops the previous material's layout and rebuilds the blank canvas from the submitted settings.
  - [x] Layout.77.1.3 - in right layout canvas, display the Material type above the right layout canvas. `Main.qml` `materialHeader`: "Material: <material>", shown whenever the handoff names a material.
  - [x] Layout.77.1.4 - in exported file names, append the currently selected '_<materialtype>' before the date stamp (only if there are multiple Material types). `exports::material_name_segment`: `<base>[_<segment>]_<material>[_<tab>]_<stamp>`; Export all tabs keeps the same order.
- [ ] Layout.77.2 - Test/Verify - check for individual patterns:
  - [ ] Layout.77.2.0 - check 'Pattern Piece Tool > Properties > Properties' for new 'Material' dropdown field
  - [ ] Layout.77.2.1 - check left import canvas that there is a tab for each Material identified in the pieces.
  - [ ] Layout.77.2.2 - check left import canvas that the pieces are correct for each Material tab.
  - [ ] Layout.77.2.3 - check right layout canvas that the Material type is displayed above the canvas.
  - [ ] Layout.77.2.4 - check right layout canvas that the pieces are correct for that material.
  - [ ] Layout.77.2.5 - check that .sm2d files contain pieces with material attribute
  - [ ] Layout.77.2.6 - check that older .sm2d files update to new schema properly
  - [ ] Layout.77.2.7 - `material_split` 3 tests (one lays out each material and checks its pieces); `exports` `material_segment_goes_after_the_variant_and_before_the_tab`. `cargo test --workspace` 543 passed; `ctest --preset debug` 6 of 6 (QML compiles). Doc: `SVG-DATA-ATTRIBUTES.md`, "Materials"; `NEW-ATTRIBUTES.csv`.
- [x] Layout.77.3 - Update Schema
  - [x] Layout.77.3.1 - Create a new `.sm2d` schema: foldline node ids, schema version bump
    - Done for the material: `v0.7.6.xsd` adds the optional piece attribute `material`; `VPatternConverter` max version 0.7.6. Fold line node ids belong to Layout.80 (Layout.80.2); 0.7.6 is not released yet, so Layout.80 can add them to the same schema version.
  - [x] Layout.77.3.2 - Create a converter for older files
    - Done: `VPatternConverter::toVersion0_7_6` moves 0.7.5 files to 0.7.6. A piece without `material` reads as Fabric; the next save writes the attribute. Check: Layout.77.2.6.

### [ ] Task Layout.78 - Verify - Multisize - Nested - SVG export - place each size in its own group

Renumbered from a second Layout.75.

Currently in multisize SVG exports, the pieces are grouped together by piecename to support layout processing, adjust mode actions, etc. However, when opening and utilizing the exported multisize nested layout SVG file, the sizes must be grouped together to enable the user to view one size at a time (hiding all other size groups).

Note: This may require flatting the SVG data before grouping pieces by size to prevent changing the position of any piece.

- [x] Task Layout.78.1 - when exporting an SVG for a nested layout, group the pieces according to size, so that there is a group element for each size that contains all the pieces of that size. Order the size groups in the SVG layout from the largest (so the largest size is underneath all other sizes in the svg display canvas) to the smallest (so it is highest in the svg display canvas) to enable easy selection of each size. The exported SVG should be flat (contains no transforms except for text elements).
  - Done: `size_groups::group_pieces_by_size`, called by `export_svg`. Groups `<g id="size_<size>" data-type="size" data-size="<size>">`, largest first; piece-sets removed; `flatten_dom` first. Only a layout with top-level piece-sets (Nested) changes. Tests: 4 `size_groups` tests; `layout_utils` `jacket2_nested_svg_export_groups_pieces_by_size` (26 groups of 8, no piece moves). Doc: `SVG-DATA-ATTRIBUTES.md`, "Nested SVG export".
- [ ] Task Layout.78.2 - Test/Verify - Export a multisize file to SVG, open the SVG file using the View feature, confirm that the file is grouped by size. Next MSI: jacket2 Nested tab, Export > SVG; in Inkscape, Objects panel lists `size_720` … `size_220`; hiding one group hides one size.

### [ ] Task Layout.80 — Verify - Fold lines (mirror lines)

A piece has an "on fold" flag (`VPieceLabelData::IsOnFold()`) but no fold line. `FoldPosition` is free text.

- [x] Layout.80.1 implement foldline:
  - [x] Layout.80.1.1 - Application Preferences - implement default Foldline style properties. Preferences > Pattern > Fold Lines tab: color (black), line type (dash dot), line weight (0.35 mm). `VCommonSettings::getDefaultFoldLine*`.
  - [x] Layout.80.1.2 - Piece Mode - Implement a 'Foldline' check option in 'Piece Properties > Path > node context menu'; user may select two adjacent nodes along the path --> must be two (not one, not three). Point node menu "Fold Line"; the row shows "(fold line)". `VPiece::foldLineNodeIndexes`: two point nodes, neighbours when excluded nodes are skipped; the closing segment counts. Any other choice: "A fold line needs two neighbouring points!", OK disabled.
  - [x] Layout.80.1.3 - Canvas - draw the foldline with its line style; a foldline is not part of the cutline or the seamline; no seam allowance can be added to this line; the "on fold" flag follows the foldline. `PatternPieceTool::m_foldLine`. Seam allowance width 0 along the fold line. A valid fold line checks and locks "On fold".
    - Decision taken: the outline stays closed, so the seam line and cut line still run along the fold segment; the fold line is drawn over it. A built-in seam allowance: the seam line stops on the fold line and does not run along it (Seamly2D.11, `TODO_COMPLETED.md`).
- [x] Layout.80.2 Update Schema:
  - [x] Layout.80.2.1 - Create a new `.sm2d` schema: fold line node ids, schema version bump. Decision taken: a main path node attribute `foldLine="true"` on each end node, in `v0.7.6.xsd` (0.7.6 is not released, so no second bump).
  - [x] Layout.80.2.2 - Create a converter for older files. `toVersion0_7_6` needs no change: an older node has no fold line.
- [x] Layout.80.3 In Seamly2D > Piece mode, implement a new 'Piece Mirror' tool - mirror the half piece across the fold line.
  - Done: Piece mode > Details > Piece Mirror (M, P); also Tools > Details menu and Details toolbar. Click a piece with a valid fold line, in any draft block; the dialog confirms.
  - Fixed: the tool refused a piece outside the active draft block ("Select a piece of the active draft block") and showed no dialog. The operation and the full piece now go into the block that holds the half piece in the file; the active block is restored after.
  - Rule (user decision): the active draft block affects Draft mode only. Piece mode works on the chosen piece and its own block. Tools made in Piece mode (Piece Mirror, Union, their nodes) record that block in the history, not the active block.
  - `PieceMirrorTool`: one Mirror by Line operation at the end of the draft block copies each half piece object off the fold line. New piece `<name> Mirrored` from the half objects and their copies.
  - `PieceMirrorPath`: full path = half from the fold line end to its start, then the mirrored half back; excluded nodes left out. Mirrored nodes: curves reversed, before/after widths and first/second edge corner rules swapped.
  - Full piece: internal paths, anchors and custom seam allowance records on both halves; pins on the anchor copies; no fold line, not on fold; 2 cm right of the half piece. One undo step.
  - Decisions taken: the mirror is a draft operation, not a new piece tool element, so the full piece follows draft changes and the file format does not change. The half piece stays; dialog option "Take the half piece out of the layout", checked by default. A custom seam allowance record across the fold line is left out.
  - Also fixed: `v0.7.6.xsd` allows `foldLine` on `unionPiece` nodes; a union of a piece with a fold line wrote an invalid file.
  - Tests: `TST_VPiece` `PieceMirrorFullPathOrder` (7 rows), `PieceMirrorNodeAttributes`, `PieceMirrorFoldNodesTakeHalfWidth`, `PieceMirrorBuildsFullOutline`, `PieceMirrorRecordRanges`, `PieceMirrorDropsRecordAcrossFold`. Passed in a scratch harness against the current `vpatterndb` sources. Checked: `cl /Zs` on every changed `.cpp`. Qt suites passed in `local_build_msi.ps1` (MSI 26.10.1921).
  - [ ] Layout.80.3.1 Next MSI (`AGENT_CHECKLIST.md` §20): mark a fold line on a half piece; Piece mode > Piece Mirror, click the piece, OK. Check: full piece right of the half; symmetric outline; seam allowance all round; no fold line; half piece out of the layout. Draft mode shows the Mirror by Line operation; move a draft point: both halves follow. Undo removes the full piece and the operation. Save, reopen. Click a piece without a fold line: status bar message, no dialog. In a pattern with several draft blocks, mirror a piece of the first block; undo, redo, save, reopen.
- [x] Layout.80.4 Handoff SVG: `data-on-fold` on the piece `<g>`, a `fold_line` component group with the two points; update `SVG-DATA-ATTRIBUTES.md` and `NEW-ATTRIBUTES.csv`. `VLayoutPiece::createFoldLineItem`; id `fold_line_<name>`. HPGL draws it as a mark; DXF-ASTM writes layer 6 (Layout.80.6.4).
- [x] Layout.80.5 Unit tests: schema round-trip, handoff attributes. Written: `TST_VPiece` `FoldLineNodeIndexes` (8 rows), `FoldLineHasNoSeamAllowance`, `LayoutPieceTakesFoldLine`, `FoldLineNodeAttributeIsRead`; `TST_SvgComponentTags::FoldLineSendsFoldLineGroup`. Checked: `cl /Zs` on every changed `.cpp`. Qt suites passed in `local_build_msi.ps1` (MSI 26.10.1921).
- [ ] Layout.80.6 SeamlyLayout side (was Layout.16):
  - [x] Layout.80.6.1 Import: read `data-on-fold` and the `fold_line` component.
    - `converter::quantity_from_piece` reads `data-on-fold` for the DXF-ASTM quantity (on fold → `N,0`).
    - `piece_extractor`, `polygon_pack::svg_extract`: `fold_line` is a piece component, never the outline.
    - `piece_divider`: `fold_line` is cut like the seam line. Each part holds the stretch inside it; a part the fold line misses has none.
  - [x] Layout.80.6.2 Layout setting: Settings > "Place fold lines on the left edge (fabric: no left margin)", under the scale check grid, every media. `foldLinesOnLeftEdge`, off by default.
    - Fabric, folded or not: the left edge is the fold, left margin 0 (`LayoutSettings::margin_px`, `effective_bin_px`). Paper keeps its left margin; the fold line lies on the left content edge (user decision).
    - The paper-sheet PDF export plans and pins the same way (`sheets::build_sheet_export_inputs`).
    - `fold_line_edge::plan_fold_line_edge`, after the split and before `translate_dom`: packer angle 0 or 180 brings the fold line to the left of the piece.
    - With nap (one angle only): the piece is mirrored across its fold line instead; text keeps reading forward. Decision taken: the two halves of a piece cut on the fold are mirror images, so the cut is the same.
    - `Rect::with_left_edge`: MaxRects takes only free boxes at x 0; start orders place these pieces first.
    - Not on the edge: Nested piece-sets (sizes centred, so their fold lines differ). A fold line across the grain or inside the outline: layout warning popup on an individual pattern; log only on a multisize layout.
    - Known limit: a mirrored piece does not grade in a Base Size DXF-ASTM export ("does not fit its handoff shape").
  - [ ] Layout.80.6.3 Adjust Mode: the fold line moves with its piece.
    - Expected to work: `accept_adjustments` sets the transform on the piece `<g>`; `fold_line` is its child. User check, `AGENT_CHECKLIST.md` §21.
  - [x] Layout.80.6.4 DXF-ASTM: layer 6 mirror line; boundary starts or ends on the mirror line points (D6673 §4.3.6); a piece without a fold line writes no layer 6.
    - `layers`: `fold_line` → layer 6. `converter`: `Block::mirror_line`. Writer: two-point `LINE` before the grainline.
    - `astm_mirror::align_mirror_lines`, after grading: the cut line edge on the fold line (0.1 mm) is the mirror line, and the boundary starts with it. Key points inside that edge are dropped, with their rule texts.
    - Fold line off the cut line: no layer 6, export warning names the piece. Piece Mirror full pieces have no fold line, so no layer 6.
  - [x] Layout.80.6.5 Tests: Rust import, left-edge placement, split, layer 6 and boundary order.
    - `astm_piece_test::piece_on_fold_counts_every_cut_as_right`, `fold_line_becomes_the_mirror_line_at_the_boundary_start`, `fold_line_group_maps_to_layer_6`; `astm_mirror` (3); `writer_astm_test::mirror_line_is_a_layer_6_line_on_the_boundary_start`; `exports::do_export_dxf_writes_the_mirror_line`.
    - `piece_extractor::fold_line_is_a_component_group`; `svg_extract::find_outline_group_skips_fold_line_at_structural_position_1`; `piece_divider::fold_line_is_clipped_per_part`.
    - `fold_line_edge` (5); `layout_utils::fold_line_lies_on_the_left_edge` (fabric and paper), `sheet_export_pins_fold_line_pieces`; `layout_views::fold_line_warning_lists_one_piece_per_line`; `layout_engine` `left_edge_pieces_touch_the_left_edge`, `left_edge_piece_is_not_turned_to_fit`; `layout_settings::fold_lines_on_left_edge_drop_the_left_margin_of_fabric_only`; `SettingsModelTests::foldLinesOnLeftEdge_defaultsOffAndRoundtrips`.
    - Checked: `cargo test --workspace` 597 passed. `cl /Zs` on `SettingsModel.cpp` and `SettingsModelTests.cpp`. `qmllint SettingsDialog.qml`: no new kind of warning. Not run: ctest, MSI.
  - [ ] Layout.80.6.6 Next MSI: user check, `AGENT_CHECKLIST.md` §21.
- [ ] Layout.80.7 Next MSI: Piece Properties > Path, mark two neighbouring nodes Fold Line; check the dashed line, no seam allowance on it, "On fold" checked; save, reopen; Layout Mode handoff has `fold_line`. Mark three nodes: OK disabled. `AGENT_CHECKLIST.md` §20.

### [ ] Task Layout.84 - Verify - Multisize - Nested - non-DXF export - display only the smallest size's grainline and labels

To meet current best practices for nested multisize image and vector files, show only one set of labels and grainline per piece.

Display only the labels and grainline of the smallest size of each piece. (In SVG exports, place them in their own group id="labels_and_grainlines" as the group just above the content rect.) Do not include any other grainline or label elements in the export.

- [x] Layout.84.1 Every export but DXF-ASTM: each piece-set keeps the `grainline`, `piece_label`, `pattern_label` and `size_label` groups of its smallest size only. Outlines, notches and fold lines of every size stay.
  - `nested_annotations::keep_smallest_size_annotations`, called by `clone_export_layout_doc` (PDF, PDF (Tiled), SVG, PNG, JPG, HPGL, PS, EPS, paid formats) and `sheets::build_sheet_export_inputs` (paper-sheet PDF).
  - Sheet PDF: stripped after the piece extraction, so the pieces pack as on the Nested tab.
  - Smallest size: `data-size` compared as numbers. Its size label stays ("Size 220").
- [x] Layout.84.2 SVG: `nested_annotations::gather_labels_and_grainlines` moves the kept groups into `<g id="labels_and_grainlines">`, just before the largest size group.
  - Decision taken: the export strips the `Rectangles` group, which holds `contentRect`. The new group takes its place: the first content group, under the size groups. Inkscape lists it directly below `size_720`.
- [x] Layout.84.3 Tests: `nested_annotations` (5); `layout_utils` `jacket2_nested_export_keeps_the_smallest_size_annotations`, `jacket2_nested_sheet_export_keeps_the_smallest_size_annotations`. `cargo test --workspace` 568 passed.
  - Docs: `SVG-DATA-ATTRIBUTES.md`, "Nested SVG export"; `seamlylayout_EXPORT_WORKFLOW.md`, "Nested tab — one grainline and label set per piece".
- [ ] Layout.84.4 Next MSI: user check, `AGENT_CHECKLIST.md` §19.

### [ ] Task Layout.82 - Verify - Multisize - Nested - wider range of colors.

Currently all colors are dark and appear to be repeated across sizes. Use some pastel range colors to ensure all sizes get a uniquely identifiable color. 

- [x] Layout.82.1 New palette. `size_colors::palette_colors`: OKLCH candidates, lightness 0.40 (dark) to 0.80 (pastel), 10° hue steps. The first size is blue. Each next size takes the candidate farthest, in OKLab, from white and from every earlier size.
  - jacket2, 26 sizes: closest pair 0.11 apart in OKLab. Before: 0.012 (`#085e1d`, `#085e28`). 7 pastel, 5 dark. Distance to white at least 0.2.
- [x] Layout.82.2 Tests: `size_colors` `palette_colors_differ_and_show_on_white`, `palette_spans_dark_to_pastel`, `palette_repeats_only_past_the_candidates`. `cargo test --workspace` 561 passed. Doc: `SVG-DATA-ATTRIBUTES.md`, "Nested size colors".
- [ ] Layout.82.3 Next MSI: user check, `AGENT_CHECKLIST.md` §16.

### [ ] Task Layout.89 - Verify - Export sheet - dialog text is too dark 

Text "Split the pieces" and "Make the media bigger" need to be white, not black.
See screenshot "C:\Users\susan\Projects\Seamly2D-private\test-seamly-layout\screenshots\Screenshot 2026-10-21 192850 jacket_2.png"

- [x] Layout.89.1 Cause: the screenshot shows a scratch `qml` render of the dialog, not SeamlyLayout. The render had no Fusion style and no `Theme`, so the radio labels took the default black. In SeamlyLayout, both radio labels and both descriptions are `Theme.textOnDark` (`#f3f3f3`).
- [x] Layout.89.2 `Main.qml` `oversizedPiecesLabel`: the piece list was `Theme.violetLight` on `Theme.dialogBackground`, contrast 2.1:1. Now `Theme.textOnDark`, contrast 7.9:1. Checked: Fusion render of the dialog from `Main.qml` with `Theme.qml`; `qmllint` shows no new warnings.
- [ ] Layout.89.3 Next MSI: user check, `AGENT_CHECKLIST.md` §12.

### [ ] Task Layout.88 — Verify - Export menu: DXF-ASTM note is cut off

Marker tab, Export menu (MSI 26.10.2023): the note "DXF-ASTM: select Base Size or a size tab" runs past the right edge of the menu. Most of it cannot be read.

- [x] Layout.88.1 Show the whole note: wrap it, or widen the menu to fit it.
  - Cause: a Qt Quick `Menu` keeps its 200 px implicit width and elides a longer item. The note is 220 px.
  - Done: `ExportMenu.qml` `fittedWidth`. The Export menu and its SVG submenu take the width of their widest shown item. A hidden item does not widen a menu.
  - The SVG submenu had the same defect: "Paths: convert to single-stroke (Hershey)" 245 px, "Paths: no conversion - labels are already paths" 254 px.
  - Checked: Fusion render of `ExportMenu.qml`. Export menu 222 px, SVG submenu 247/256 px, no elided text. `qmllint` shows no warnings.
- [ ] Layout.88.2 Next MSI: Marker and Nested tabs, Export menu; the note is readable. `AGENT_CHECKLIST.md` §3.

### [ ] Task Layout.72 — Verify - Split oversized individual pieces

For oversized individual-pattern pieces, ask whether to split them or enlarge the media (decision 2026-10-13).

- Dialog: list pieces and offending dimensions; offer "Split the pieces" (SVG, PDF, HPGL, EPS, PS, PNG, JPG) or "Make the media bigger" (DXF). OK continues; Cancel leaves only Layout Settings active.
- Split only if no allowed rotation fits: too wide → vertical cuts; too long on non-roll media → horizontal cuts. Paper roll and fabric are roll-form. No dialog for tiled paper.
- Enlarge media using Layout.71 (width) and Layout.25 (length).
- Split into the minimum number of equal parts (rounded up), without cut-edge seam allowance. Label each part; add 2 match notches per cut edge. Apply the same split to sheet PDF. Warn if a fitting piece is left out because the media is full.

- [x] Layout.72.9 Dialog names each offending dimension with the piece and media sizes, in the settings unit: `• Back: length 43.10 in, media 35.50 in`. The piece list is plain text and scrolls past 40% of the window. Tests: `piece_divider` `oversized_message_gives_the_offending_dimensions`; `trousers_long_pieces_split_on_a_sheet` checks each leg line. Checked: `cargo test -p cxxqt_bridge`; `qmllint` shows no new warnings; scratch `qml` render, 30 pieces: list 280 px, dialog 523 px in a 700 px window.
- [ ] Layout.72.8 Next MSI: trousers sample, Paper Sheet 36 × 36 in, Create Layout:
  - [ ] Dialog lists four too-long legs: Back, Front_Pleats, Front_no_pocket, Front_with_pocket. Each line reads `<name>: length <n> in, media 35.50 in`.
  - [ ] Cancel: only Layout Settings, Preferences, Exit enabled; submitting settings re-enables Create Layout.
  - [ ] Split: each leg becomes 2 parts, blue "<name> part i of 2, joins j" label, and 2 green match notches. Warn about unplaced pieces.
  - [ ] Enlarge: sheet exceeds 36 in length; no splits or warnings.
  - [ ] Export split PDF parts and DXF-ASTM blocks named `Back (1/2)` and `Back (2/2)`.

### [ ] Task Layout.83 - Verify - Multisize - Nested - Legend - change legend color from line to rect element with width equal to current legend color line width and height equal to width. (makes legend easier to read)

- [x] Layout.83.1 `SizeLegend::element`: each swatch is a 32 × 32 px `rect`, filled with the size color, no stroke, centered on its row. Was a 32 px line, stroke width 4.
- [x] Layout.83.2 Row height 24 → 40 px (swatch plus one 8 px pad), so rows do not overlap. The legend is taller; the media grows to hold it as before.
- [x] Layout.83.3 Tests: `size_colors` `legend_swatches_are_filled_squares` (new); `legend_wraps_and_lists_every_size` counts swatch rects. `cargo test --workspace` 561 passed. Doc: `SVG-DATA-ATTRIBUTES.md`, "Nested size colors".
- [ ] Layout.83.4 Next MSI: user check, `AGENT_CHECKLIST.md` §15.

### [ ] Task Layout.86 - Verify - Export - Combine the DXF exports into a DXF submenu

### [ ] Task Layout.87 - Verify - Export - Marker - Disable 'PDF (Tiled)' layout for Marker layout (seamlylayout will lock up when exporting a Marker as a tiled layout)

### [ ] Task Layout.85 - Verify - Exports - add an option to export DXF 10 (DXF-AAMA)

Export > DXF > "DXF-AAMA (R11)": ANSI/AAMA-292 content in a DXF R11 (AC1009) file, the DXF version the standard builds on. Same pipeline as DXF-ASTM; `DxfAstmExportOptions::aama` selects the AAMA layers. Reference: `dxf-docs/seamlylayout_DXF_AAMA.md`.

- [x] Layout.85.1 Research ANSI/AAMA-292: AutoCAD R11 DXF base, 14 layers, grade rules in a `.rul` file. D6673 added layers 15 and 80–87.
- [x] Layout.85.2 Writer (`ezdxf2dxfastm/src/writer.rs`):
  - `$ACADVER` AC1009 (R11). R11 and R12 share it, so the file structure is the DXF-ASTM (R12) structure.
  - Dense polylines on layers 1, 8, 11, 14; no validation layers 84–87. Layers 2 and 3 keep the key points.
  - Every notch on layer 4: T, castle and U become V notches with their width.
  - Label text on layer 1. A label line that reads as piece text (`Size: 40`) or a rule identifier (`# 2`) is not written.
  - `Size:` piece text, not `Size Name:`. Style text ends at `Units:`.
  - `.rul` first line `ANSI/AAMA VERSION:1.0.0`.
  - Mirror line (layer 6, from the fold line work): the dense boundary starts with its two points. Dense vertices on the fold between them are dropped.
- [x] Layout.85.3 Menu and bridge: `ExportMenu.qml` item "DXF-AAMA (R11)"; `Main.qml` `requestDxfExport(false, "R11", true)`; options JSON `"aama"`, `"dxfVersion":"R11"`.
  - Default name `<base>_AAMA[_annotated][_<material>][_<tab>]_<stamp>.dxf`.
  - Marker and Nested: disabled, as DXF-ASTM. The submenu note is now "DXF: select Base Size or a size tab".
  - Missing Piece Data dialog, save dialog, progress text and annotated `.txt` title name DXF-AAMA.
- [x] Layout.85.4 Tests: `writer_astm_test` `aama_r11_header_names_ac1009`, `aama_style_text_has_no_curve_tolerance_or_astm_version`, `aama_block_uses_layers_1_to_14_only`, `aama_labels_never_repeat_piece_text`, `aama_graded_export_writes_rule_ids_and_aama_rule_file`, `aama_annotated_version_names_dxf_aama`, `aama_boundary_starts_on_the_mirror_line`; `exports.rs` `do_export_dxf_writes_aama_r11_when_asked` and AAMA name segments.
  - Checked: `cargo test --workspace` 596 passed, after the fold line merge. ezdxf 1.4.4 `recover`: male_shirt and graded fixtures as DXF-AAMA, 0 errors, layers 1–14 only. Scratch render of `ExportMenu.qml`: item order, signal, disabled state. `qmllint`: no new warnings.
  - Not run: ctest, MSI.
  - Decisions taken:
    - R11 DXF (AC1009), user decision: ANSI/AAMA-292 compliance. The first version wrote R10 (AC1006) from the task title.
    - Labels on layer 1, as the Seamly2D and Valentina AAMA writers do. AAMA has no annotation layer.
    - No `Category:`: the handoff has no value for it.
  - Open: an untagged SVG (no `data-type`) keeps its generic layers, so its text stays on layer 15.
- [ ] Layout.85.5 Next MSI: user check, `AGENT_CHECKLIST.md` §22. Import the file in an AAMA reader (CLO3D or another pattern CAD).


### [ ] Task Layout.95 - Verify - Seamly2D Piece Properties - Rotation" Change Rotation to be radio buttons for: None, 1-Way → 360, 2-Way → 180, 4-Way → 90, Any → 1. None or Undefined writes no line. (Default is "None"). Include this in the schema developed for SeamlyLayout; create conversion from previous schema; create test.

- [x] Layout.95.1 Piece Properties > Rotation: five radio buttons in three columns. Default None. `rotation_ButtonGroup`; button id = `PieceRotation` value.
- [x] Layout.95.2 Data: `PieceRotation` (`floatitemdef.h`); `VPieceLabelData::getRotationWay`, `rotationToString`, `stringToRotation`, `rotationDisplayName`. Label `%pRotation%` shows the translated name.
- [x] Layout.95.3 Schema: pattern format 0.7.7 (`v0.7.7.xsd`), `rotationWay` type `pieceRotation`: `None`, `1-Way`, `2-Way`, `4-Way`, `Any`.
  - Decision taken: a new format 0.7.7, not a change to 0.7.6. Builds already wrote 0.7.6 files (jacket2, trousers samples) with free-text values; a 0.7.6 file is not converted again.
  - Layout.96–98 add to the same 0.7.7 format.
- [x] Layout.95.4 Converter: `VPatternConverter::toVersion0_7_7`, `rotationWayToV0_7_7`. An empty, unknown or translated value becomes `None`.
- [x] Layout.95.5 Tests: `TST_VPiece::PieceRotationValuesRoundTrip`, `ConverterSetsRotationWay` (3 rows). Passed in a scratch harness (in-tree libs, current `vpatternconverter.cpp`, `vpiecelabeldata.cpp`, `schema.qrc`); the converted file validates against `v0.7.7.xsd`. `cl /Zs` on every changed `.cpp`. Fusion render of the dialog: the page fits. Not run: Qt suites (next MSI).
- [ ] Layout.95.6 Next MSI: user check, `AGENT_CHECKLIST.md` §24.
- DXF `Rotation:` line and the handoff attribute: Layout.93.4, 93.5.

### [ ] Task Layout.96 - Verify - Seamly2D Piece Properties - Tilt: change to radio buttons "None", "CCW", "CW". If "CCW" or "CW' chosen then enable a text field 'Degrees' that accepts integers between 0 and 90. Default radio button is "None", Default Degrees is "0". Include this in the schema developed for SeamlyLayout; create conversion from previous schema; create test.

- [x] Layout.96.1 Piece Properties > Tilt: radio buttons None, CCW, CW; new row "Degrees:" spin box 0–90 (suffix °), enabled for CCW and CW only. Default None, 0. A piece set to None saves 0 degrees.
- [x] Layout.96.2 Data: `PieceTilt`; `VPieceLabelData::GetTilt`, `getTiltDegrees` (clamped 0–90), `tiltToString`, `stringToTilt`, `tiltDisplayText`. Label `%pTilt%` shows "CCW 15"; None shows "None".
- [x] Layout.96.3 Schema 0.7.7: `tilt` type `pieceTilt` (`None`, `CCW`, `CW`); new attribute `tiltDegrees` type `tiltDegrees` (0–90). `VAbstractPattern::AttrTiltDegrees`.
- [x] Layout.96.4 Converter `tiltToV0_7_7`: old "CW X" / "CCW X" → direction, 0 degrees; "CCW 15" → CCW, 15; a number past 90 → 0; any other value → None without `tiltDegrees`.
- [x] Layout.96.5 Tests: `TST_VPiece::PieceTiltValuesRoundTrip`, `ConverterSplitsTilt` (6 rows). Passed in the scratch harness, with the rotation tests (13 passed). `cl /Zs` on every changed `.cpp`. Fusion render of the dialog. Not run: Qt suites (next MSI).
- [ ] Layout.96.6 Next MSI: user check, `AGENT_CHECKLIST.md` §24.
- DXF `Tilt:` line and the handoff attribute: Layout.93.4, 93.5.

### [ ] Layout.97 - Verify - Seamly2D Piece Properties - Fold: Seamly2D gives Up/Down or Left/Right. D6673 Fold: is Y or N, and means the piece may go on the fold of tubular fabric. Change Seamly2D Piece Properties "On Fold" to be a checkbox that saves as "True" or "False", which exports to 'Y' or 'N' in DXF formats. This removes the possibility for 'Up/Down' folds as this is not typical for soft goods development. (Default is unchecked/"False"). Include this in the schema developed for SeamlyLayout; create conversion from previous schema; create test.

- [x] Layout.97.1 Piece Properties: the Fold position list is removed. The row "Placement:" is now "Fold:" with the check box "On fold" (tooltip names DXF `Fold:Y`). Default unchecked. A valid fold line still checks and locks it.
- [x] Layout.97.2 The pattern file keeps `onFold` (`xs:boolean`: `true` / `false`). `VPieceLabelData` has no fold position; `VAbstractPattern::AttrFoldPosition` is removed.
- [x] Layout.97.3 Schema 0.7.7: no `foldPosition` attribute. Converter `removeFoldPositionV0_7_7` drops it; `onFold` is unchanged.
  - Decision taken: a fold position (Up/Down, Left/Right) does not set `onFold`; the check box was the fold flag before too.
  - Label placeholder `%pFoldPosition%`: removed from the label editor Insert menu; an old template prints nothing for it. No sample or template uses it.
- [x] Layout.97.4 Tests: `TST_VPiece::ConverterRemovesFoldPosition` (3 rows). Harness: 16 passed. `cl /Zs` on every changed `.cpp`. Fusion render: the page fits again. Not run: Qt suites (next MSI).
- [ ] Layout.97.5 Next MSI: user check, `AGENT_CHECKLIST.md` §24.
- DXF `Fold:Y` / `Fold:N`: Layout.93.5.

### [ ] Task Layout.98 - Verify - Seamly2D Piece Properties - Flip: change Flip to radio buttons 'None', 'Horizontal', 'Vertical', and 'Both' are enabled. (Default is 'None'); create test.

- [x] Layout.98.1 Piece Properties > Attributes: "Flip:" radio buttons None, Horizontal, Vertical (first row), Both. All four are enabled. Default None. Replaces the "Flipping: Forbid" check box.
- [x] Layout.98.2 Data: `PieceFlip` (`def.h`), `pieceFlipToString`, `stringToPieceFlip`; `VAbstractPiece::getFlip`, `setFlip`. `IsForbidFlipping()` is now true for None; the Seamly2D layout engine (`vlayoutpaper.cpp`, `vposition.cpp`) keeps using it. `VLayoutPiece` keeps the flip.
- [x] Layout.98.3 Schema 0.7.7: piece attribute `flip` type `pieceFlip`; `forbidFlipping` removed. Converter `removeForbidFlippingV0_7_7`.
  - Decision taken: every converted piece gets flip None, also `forbidFlipping="false"`. Most old files hold `false` only because it was the default; a DXF export then writes no `Flip:` line, as before. Change the piece to allow a flip.
- [x] Layout.98.4 Other places that set the old flag:
  - Piece context menu: "Flip" submenu with the four choices replaces "Forbid Flipping".
  - Decision taken: the F key no longer toggles flipping (four states); the Shortcuts dialog row is removed.
  - Preferences > Pattern: the "Forbid flipping" default is removed (`VCommonSettings::getForbidPieceFlipping`, key `pattern/forbidFlipping`): a new piece is always None.
- [x] Layout.98.5 Tests: `TST_VPiece::PieceFlipValuesRoundTrip`, `LayoutPieceKeepsFlip`, `ConverterRemovesForbidFlipping` (2 rows). Harness: 19 passed (`LayoutPieceKeepsFlip` needs a rebuilt `vlayout`; Qt suites). The harness also converted the 8 committed samples and 13 CollectionTest patterns to 0.7.7; all validate. Not converted: 4 inputs that fail before conversion (seamtest1–3 at 0.4.0, the empty `text.sm2d`) and 21 formats older than `readOnly`. `cl /Zs` on every changed `.cpp`. Fusion and Windows renders of the dialog. Not run: Qt suites (next MSI).
- [ ] Layout.98.6 Next MSI: user check, `AGENT_CHECKLIST.md` §24.
- DXF `Flip:` line and the handoff attribute: Layout.93.4, 93.5.

### [ ] Task Layout.92 - Verify - right layout canvas - Material tabs

Currently SeamlyLayout's left input canvas displays the Material types in tabs; the layout algorithm processes only the currently selected Material tab from the left hand canvas. Update the layout algorithm to generate the layouts for all material tabs, and display the material layouts in the right hand canvas using Material tabs (as is done in the left input canvas).

- [x] Layout.92.1 Create Layout lays out every material: the shown material first, then each other material in tab order, each with its first layout (individual layout or Marker) and all its tabs. One progress popup; labels name the material: "Laying out Lining: Size 220 (30 of 56)…".
  - `layout_views::layout_jobs`, `LayoutJob`; `TabLayoutQueue` holds jobs of every material, labels (`job_label`), the Base Size reuse per material, and the final warning (`final_warning`: left-out pieces, then fold lines off the edge).
  - Settings Submit makes a blank canvas per material (`initialize_layout`); the oversized-pieces dialog lists the pieces of every material.
- [x] Layout.92.2 Right canvas: material tabs above the layout tabs, the same tabs as over the left canvas (`Main.qml` inline component `MaterialTabBar`). One material: the "Material: <m>" header stays.
  - Decision taken: both tab bars show the same material. A click on either shows that material's pieces on the left and its layouts on the right.
  - Decision taken: a material switch keeps every layout, its shown tab and its Adjust edits (`layout_views::MaterialLayout`; `select_material` parks and restores, as tabs do). Replaces the Layout.77.1.2 rule that a switch drops the layout.
  - Exports and Export all tabs work on the shown material, as before.
- [x] Layout.92.3 Tests: `layout_views` `layout_jobs_cover_every_material`, `job_labels_name_the_material`, `final_warning_joins_problems_and_fold_lines`, `material_layout_resets_to_its_blank_canvas`; `tab_queue_messages_count_up` and `base_size_layout_is_reused_once_kept` updated. Existing `material_split::material_layout_holds_its_own_pieces` lays out each material. Doc: `SVG-DATA-ATTRIBUTES.md`, "Materials".
  - Checked: `cargo test --workspace` 603 passed; `ctest --preset debug` 6 of 6 (debug build with the new QML); `qmllint`: no new kind of warning. Not run: GUI, MSI.
- [ ] Layout.92.4 Next MSI: user check, `AGENT_CHECKLIST.md` §25.

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
  - Done: after `Quantity:`, in D6673 order: `Rotation:180`, `Flip:Y`, `Tilt:-15`, `Fold:N`, then `Material:`. Rotation, Flip or Tilt None writes no line. `converter::rotation_from_piece`, `flip_from_piece`, `tilt_from_piece`, `fold_from_piece`; `write_astm_block`.
  - Decision taken: Flip Horizontal (left to right) is `Y`, Vertical is `X` (D6673: X flips about the horizontal axis). Both is `XY`; D6673 defines only `X` or `Y`. Change if a reader refuses it.
  - Decision taken: Tilt has its sign: `+15` counterclockwise, `-15` clockwise. A direction with 0 degrees writes no line.
  - Decision taken: every tagged piece writes `Fold:Y` or `Fold:N`.
  - Decision taken: DXF-AAMA writes none of the four: AAMA piece text has no such identifiers.
- [x] Layout.93.6 Tests: `TST_SvgComponentTags`; Rust converter and writer tests with hidden labels and grainlines.
  - `TST_SvgComponentTags::PieceCarriesPieceProperties` (hidden label); `TST_VPiece::LayoutPieceKeepsPieceProperties`, `RotationStepDegrees` (harness: passed); `astm_piece_test::layout_rules_come_from_piece_properties`; `writer_astm_test::layout_rule_text_follows_quantity_in_d6673_order`; `graded_sample_blocks_follow_d6673` expects `Fold:N`. Hidden labels and grainlines: Layout.93.1–93.3 tests.
  - Checked: `cargo test --workspace` 605 passed; `cl /Zs` on every changed `.cpp`. Not run: Qt suites (next MSI).
  - Docs: `SVG-DATA-ATTRIBUTES.md` (piece attributes), `NEW-ATTRIBUTES.csv`, `seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`.
- [ ] Layout.93.7 Next MSI: user check, `AGENT_CHECKLIST.md` §23.

### [ ] Task Layout.94 - Verify - Multisize the grainline, pattern label, and piece label all have the same position relative to local space within their piece. One of the results will be that the Nested layout's grainlines, pattern label, and piece label stack exactly on top of each other to make the nested layout visually more simple and easier to read. Another results is that no grainline, piece label, pattern label is placed outside or extends beyond the cutline.

Select the position of the base size's grainline (when visible), pattern label (when visible), and piece label (when visible); replace the reamining pieces' grainline, pattern label, and piece label 's positions with the base size's positions.

- [x] Layout.94.1 At import, every size's grainline, piece label and pattern label move to the base size's place: the same offset from the centre of the outline's bounding box. Only shown (handed-off) groups move. `cxxqt_bridge/src/annotation_alignment.rs`, called by `finish_import` before the piece names; size labels follow the moved grainline.
  - Reference point: grainline middle; label first text line anchor.
- [x] Layout.94.2 Nested stacks the sizes on the outline centre (`svg_dom::center_piece_sets` now measures the cut line, else the seam line), the same centre the move uses. jacket2: 12 stacks, each within 0.5 px.
- [x] Layout.94.3 Cut line check: a grainline end or label line anchor outside a size's cut line is named in one import warning.
  - Finding: jacket2 Back's pattern label sits near the edge in size 500 and leaves sizes 220–280: "Back: pattern label past the cut line in sizes 220, 240, 260, 280". Fix in Seamly2D: move Back's pattern label toward the middle in the base size.
  - Decision taken: positions stay the base size's even then (the task's rule); the warning names the sizes. No label is moved inward on its own.
  - Open: on the Nested canvas the size labels of every size stack too ("Size 220" over "Size 240"). Nested exports keep the smallest size's only (Layout.84).
- [x] Layout.94.4 Tests: `annotation_alignment` (5); `layout_utils::jacket2_nested_stacks_each_piece_set_annotations` with the MSI 26.10.2023 capture as fixture (now `jacket2_40-146_handoff.svg`, Layout.27.13.4). `cargo test --workspace` 611 passed. Doc: `SVG-DATA-ATTRIBUTES.md`, "Multisize grainline and label positions".
- [ ] Layout.94.5 Next MSI: user check, `AGENT_CHECKLIST.md` §26.

