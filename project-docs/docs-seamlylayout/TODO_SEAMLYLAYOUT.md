# TODO — SeamlyLayout app features

Tasks that add features to the SeamlyLayout layout app.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

Tasks in this file are numbered and are prefixed with `Layout.`

## [x] Task Layout.01 - Open seamlylayout.exe

- [x] Layout.01 While in 'Piece Mode' in Seamly2D, pressing the 'Layout' button on the menu should immediately run seamlylayout.exe without displaying the old Seamly2D layout canvas or calling vlayout which has been superceded by the seamlylayout.exe application. The old vlayout code has been superceded by seamlylayout.exe

## [x] Task Layout.02 - Layout Settings

- [x] Layout.02 - In the settings dialog, section 'Layout Mode', add a checkbox for 'Any'; when 'Any' is checked the layout algorithm should ignore the grainline direction when placing the pieces efficiently.

## [x] Task Layout.03 - layout algorithm improvement

- [x] Layout.031 - The layout algorithm has suffered regression - the pieces are nested sub-optimally (there is a lot of space between the pieces). Fix the layout algorithm so that the pieces only have the gap specified between them.

- [x] Layout.032 - Implement the 'Layout Mode' == 'None' option so that the grainline direction is ignored while the pieces are efficiently arranged.

## [x] Task Layout.1 - 'Adjust Mode' improvement

- [x] Layout.11 - The 'Adjust Layout' feature has suffered regression - when 'Adjust Layout' opens there are no pattern pieces to adjust - the pieces from the main SeamlyLayout canvas should be available in 'Adjust Layout', otherwise there is nothing to adjust and save back to the main SeamlyLayout canvas.

- [x] Layout.12 - When Adjust Mode closes it should return focus to SeamlyLayout's right canvas

- [x] Layout.13 - When focus returns to the right canvas after 'adjust mode' closes, the right canvas should contain either:
  - [x] Layout.13.1 - the layout with updates from 'adjust mode' ('adjust mode' closed via 'Save')
  - [x] Layout.13.2 - the layout without updates from 'adjust mode' ('adjust mode' closed via 'Cancel')

## [ ] Task Layout.3 — Multisize handoff and layout: nested (piece groups) or marker (single pieces)

A pattern with a `.smms` multisize measurement file hands SeamlyLayout every size in one stringified SVG. Each piece is a group that holds that piece in every size. `process_layout()` packs either the groups or the single pieces.

Decisions (user, 2026-09-28):

- Sizes: every size in the `.smms` file, at the base height only. No size × height combinations.
- Stack order in a group: each size centered on the largest size's center, after the grainline is vertical.
- Individual `.smis` patterns keep today's handoff, unchanged.

Handoff shape: see `project-docs/docs-data/SVG-DATA-ATTRIBUTES.md`, "Multisize pattern".

- [x] Layout.30 Seamly2D producer
  - [x] Layout.30.1 `MainWindow::prepareMultisizePieceLists()`: every size, base height, then restore; shared helpers `recalculateAtSize()` and `currentLayoutPieces()` also serve `exportPiecesAs()`
  - [x] Layout.30.2 Piece matched across sizes by `VPiece` id; one `piece-set` group per piece, one `piece` child per size
  - [x] Layout.30.3 `data-measurements`, `data-sizes`, `data-base-size`, `data-size`; `NEW-ATTRIBUTES.csv` and `SVG-DATA-ATTRIBUTES.md` updated
  - [x] Layout.30.4 `TST_SvgComponentTags`: 4 new tests
- [x] Layout.31 SeamlyLayout import: `read_measurements_info()`; `AppController.isMultisize`, `multisizeSizes`
- [x] Layout.32 Settings dialog: "Multisize Layout" — Nested | Marker, shown only for a multisize import; JSON key `multisizeLayout`
- [x] Layout.33 Nested layout
  - [x] Layout.33.1 `hoist_piece_sets()`: piece-sets reach the root as one unit
  - [x] Layout.33.2 `verticalize_dom` turns each size on its own grainline; `center_piece_sets` stacks sizes on the largest size
  - [x] Layout.33.3 One `PieceRect` per set: bbox = union of sizes; outline polygon = largest size
  - [x] Layout.33.4 Placed pieces keep `data-size`
- [x] Layout.34 Marker layout: every size packs alone; label "Name (size N)"
- [x] Layout.35 Adjust Mode: moves each placed unit by id, so Nested moves a whole set
- [x] Layout.36 Export: user chooses the multisize export (user, 2026-09-28)
  - [x] Layout.36.1 **Nested**: one file; each piece keeps its `piece-set` group; every piece keeps `data-size`
  - [x] Layout.36.2 **Export all tabs**: checkable Export menu item; every format writes one file per tab, tab label before the extension; each file is that tab's own packed layout (user decision, 2026-09-29: explicit menu item, no chooser dialog)
- [x] Layout.37 Sized layouts. Layout.36.2 depends on this
  - [x] Layout.37.1 Pack each size again as its own layout: only that size's pieces, same settings; progress popup names each size
  - [x] Layout.37.2 Tabs across the top of the right canvas: "All sizes", then one per size
  - [x] Layout.37.3 Adjust Mode and exports use the selected tab
  - Layout.37.4 Won't do: one PDF per tab. Select a size tab, then export PDF or PDF (Tiled). One page per size conflicts with sheet and tiled pages (user decision, 2026-09-29)
- [ ] Layout.38 Tests
  - [x] Layout.38.1 Rust: set hoist, centring, set bbox, set outline, marker count = pieces × sizes (synthetic fixture `MULTISIZE_HANDOFF_SVG`)
  - [x] Layout.38.2 export SVG from a `.sm2d` + `.smms` pattern, save to `crates/cxxqt_bridge/test_data/':
    - [x] Layout.38.2.1 save files to `crates/cxxqt_bridge/test_data/':
      --> src\app\seamlylayout\crates\cxxqt_bridge\test_data\jacket2_40-146_202610121819.svg
      --> src\app\seamlylayout\crates\cxxqt_bridge\test_data\jacket2_40-146_nested_allsizes_R13_202610121813.dxf
      - The SVG is committed. The DXF is not used by a test and stays untracked.
      - The SVG is a SeamlyLayout SVG export, not the raw handoff: the piece-sets sit at the root, and the pattern group with `data-sizes` and `data-base-size` is gone. `layout_utils::tests::jacket2_handoff` puts the pattern group back (sizes from `data-size`, base size 500). A `SEAMLYLAYOUT_SAVE_HANDOFF` copy can replace the file and the rebuild.
    - [x] Layout.38.2.2 implement an end-to-end test: `layout_utils::tests::jacket2_*` (5 tests)
  - [x] Layout.38.3 `SEAMLYLAYOUT_SAVE_HANDOFF=<absolute path>`: `import_svg_document` writes the received SVG there (`handoff_capture.rs`)
  - [x] Layout.38.4 User: capture the SVG of `jacket2_40-146.sm2d` + its `.smms` :  src\app\seamlylayout\crates\cxxqt_bridge\test_data\jacket2_40-146_202610121819.svg
  - [x] Layout.38.5 add the fixture and an end-to-end test
    - `jacket2_handoff_is_multisize`: 208 pieces, 26 sizes 220-720, base size 500
    - `jacket2_nested_packs_eight_piece_sets`: 60 in x 300 in roll, 8 units, 208 pieces placed
    - `jacket2_marker_places_every_size`: 60 in x 1500 in roll, 208 units; each piece-set number on 26 pieces
    - `jacket2_missing_piece_data_lists_each_piece_once`: Nested and Marker, 8 lines, Cyrillic names, `Detail (3)` to `Detail (8)`
    - `jacket2_all_sizes_export_names_every_size_alike`: escaped `Piece Name:`; 4 pieces graded (26 sizes each); `Detail (3)`, `(6)`, `(7)`, `(8)` fall back on turn points (Layout.27.3); `.rul` written
  - [ ] Layout.38.6 Check in the running apps: Seamly2D Layout Mode with a multisize pattern; SeamlyLayout Nested and Marker
    - Needs the next MSI. Add to the next `TEST_CHECKLIST.md`.
- [x] Layout.39 Doxygen briefs + inline comments on touched functions; document both layouts in the SeamlyLayout docs

## [ ] Task Layout.5 - Implement additional export formats

- [ ] Layout.51 DXF-AAMA — biggest install base in apparel PLM, reference implementation already in the repo
  - **On hold** — no AAMA spec on hand. Get the spec, or study sample AAMA `.dxf` files from other CAD systems, before implementing.
- [x] Layout.52 HPGL — unlocks the whole plotter and cutter class
- [x] Layout.53 PS/EPS — one writer covers both and retires pdftops
- [x] Layout.54 JPG — trivial, fit it anywhere
- [x] Layout.55 DXF-ASTM - improve current export so that it meets the DXF-ASTM standard
- [x] Layout.57 DXF-ASTM (CLO3D) — Export menu item: D6673-10 file plus CLO3D group 250 (0 = boundary, 2 = sew line)

## [ ] Task Layout.6 - DXF-ASTM: D6673 features without a Seamly2D source

See `dxf-docs/seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`, "Not exported".

- [ ] Layout.61 Grading: grade rule identifiers (`# n` text), graded nests (one block per size, `Size Name:`), grade reference line (layer 5), grade rule table file
  - [x] Layout.61.1 Trigger: DXF-ASTM export from the "All sizes" tab of a multisize layout; the handoff DOM goes to `do_export_dxf` (`DxfGradingSource`)
  - [x] Layout.61.2 `astm_contour::grade_contours`: same key points in every size, each size within 0.25 mm
  - [x] Layout.61.3 `astm_grading::grade_drawing`: nests in size order, rules in draft space, rigid fit into the layout, layer 5 line, `# n` texts; ungradable piece → sample size only + export warning
  - [x] Layout.61.4 Writer: graded size blocks without layers 2/3/4; `Sample Size:` and `Grade Rule Table:` style text; `.rul` file (`grade_rule_file.rs`)
  - [x] Layout.61.5 Tests: `astm_grading_test.rs`, `writer_astm_test.rs` graded tests, `multisize_layout_exports_graded_nests`
  - [x] Layout.61.6 Compliance doc "Grading"
  - [ ] Layout.61.7 User: export a multisize pattern from "All sizes" as DXF-ASTM; check it in aw.fyi (DXF.8.4)
    - [x] Layout.61.7.1 Check the jacket2 reports in `test-seamly-layout/layouts/jacket2/`. Result: grading not verified
      - Both files: MSI 26.10.1115, before Layout.26 and Layout.27.9
      - Marker `jacket2_40-146_marker_allsizes_R13_202610121149.dxf`: nothing graded. `Sample Size:` and `Grade Rule Table:` blank, no `.rul`, layer 2 in all 25 blocks (Layout.25, 27.1, 27.3)
      - Nested `jacket2_40-146_nested_allsizes_R13_202610121136.dxf`: aw.fyi reads sample size 500 and 104 layer 5 lines. 4 pieces graded; Detail (3), (6), (7), (8) sample size only (Layout.27.3)
    - [ ] Layout.61.7.2 User: install MSI 26.10.1220 or later. Export jacket2 Nested "All sizes" as DXF-ASTM (R13); run aw.fyi
      - Expect: no "Detail" merge departures, no layer 5 mismatch, no duplicate vertices. Detail (3), (6), (7), (8) stay sample size only until Layout.27.3
    - [ ] Layout.61.7.3 User: Marker "All sizes" after Layout.25 and Layout.27.10

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

- Layout.62 Moved to Layout.16: mirror line (layer 6) needs Seamly2D fold lines (Seamly2D.8)
- [ ] Layout.63 Drill holes (layer 13), with diameter (group 30) - not implemented in Seamly2D yet so move this to TODO_SEAMLY2D.md
- [ ] Layout.64 Stripe and plaid reference lines (layers 9, 10) - not implemented in Seamly2D yet so move this to TODO_SEAMLY2D.md
- [x] Layout.65 Notches reach the DXF as layer 4 slits, half the seam allowance deep (done, see `TODO_COMPLETED.md`)
- [x] Layout.66 Seamly2D producer attributes (done, see `TODO_COMPLETED.md`)

## [ ] Task Layout.15 — Layout by material

Depends on Seamly2D.7. Until then every piece is on one fabric.

- [ ] Layout.15.1 Import: read `data-material` and per-material quantity
- [ ] Layout.15.2 One layout tab per material; each packs only that material's pieces
- [ ] Layout.15.3 Quantity: place each piece its cut count per material
- [ ] Layout.15.4 Exports: material in the default file name; DXF-ASTM `Material:` piece text from `data-material` (today `Block::material` is `DEFAULT_MATERIAL` = `Fabric`)
- [ ] Layout.15.5 Tests: Rust import, packing per material, export text

## [ ] Task Layout.16 — Fold lines in layout and export

Depends on Seamly2D.8.

- [ ] Layout.16.1 Import: read `data-on-fold` and the `fold_line` component
- [ ] Layout.16.2 Layout setting: pack the half piece, or unfold it (mirror across the fold line) before packing
- [ ] Layout.16.3 Adjust Mode: the fold line moves with its piece
- [ ] Layout.16.4 DXF-ASTM: layer 6 mirror line; boundary starts or ends on the mirror line points (D6673 §4.3.6); unfolded pieces write no layer 6
- [ ] Layout.16.5 Tests: Rust import, unfold geometry, layer 6 and boundary order

## [ ] Task Layout.22 — Multisize tab: rename "All sizes" to "All sizes - group"

The tab shows the Nested layout: one piece-set group per piece, every size stacked.

- [ ] Layout.22.1 `layout_views::ALL_SIZES_LABEL` = "All sizes - group"; update the `layout_views` label tests
- [ ] Layout.22.2 Export all tabs: check the file name with the new label on Windows, Linux, macOS
- [ ] Layout.22.3 Docs: replace the tab name in the SeamlyLayout docs, the DXF-ASTM compliance doc, `TEST_CHECKLIST.md`

## [ ] Task Layout.23 — Multisize tab: add "All sizes - marker"

Tab order: "All sizes - group", "All sizes - marker", then one tab per size. The new tab shows the Marker layout: each size of each piece packs alone.

Decision (user, 2026-10-11): remove the Settings "Multisize Layout" choice (Nested | Marker). Each all-sizes tab has a fixed mode, so the choice has no effect.

- [ ] Layout.23.1 `layout_views`: second all-sizes view, Marker mode, after "All sizes - group"
- [ ] Layout.23.2 Remove the "Multisize Layout" setting: `SettingsDialog.qml`, `SettingsModel.{h,cpp}`, `layout_settings.rs`, `layout_utils.rs`. Ignore a stored `multisizeLayout` JSON key
- [ ] Layout.23.3 Size run: lay out the marker tab, then each size; progress popup names each tab
- [ ] Layout.23.4 Adjust Mode and exports use the marker tab when it is selected
- [ ] Layout.23.5 DXF-ASTM from the marker tab: graded nests and `.rul`, same as "All sizes - group"
- [ ] Layout.23.6 Export all tabs: one file for the marker tab
- [ ] Layout.23.7 Tests: Rust view labels and order; marker tab piece count = pieces × sizes; settings JSON without `multisizeLayout`
- [ ] Layout.23.8 Docs: settings workflow, simple workflow, `TEST_CHECKLIST.md`

## [ ] Task Layout.67 — Multisize tabs: Marker, Nested, Base Size, then each size

Decision (user, 2026-10-13): simplify the multisize tabs and exports. Every tab takes its pieces from the imported SVG; no tab is built from grade rules. DXF-ASTM carries one size per file. Replaces Layout.22 and Layout.23. Layout.27.10 and Layout.61.7.3 no longer apply: the Marker tab has no DXF-ASTM export.

| Tab | Packs | DXF-ASTM | Other exports |
|---|---|---|---|
| Marker | every size of every piece alone | none | yes |
| Nested | each piece once, its sizes stacked | none | yes |
| Base Size N | the base size | `# n` texts, layer 5, `.rul`, `Sample Size:N` | yes; Export all tabs skips it |
| Size N | one size | no `.rul`, blank `Sample Size:` | yes |

- [x] Layout.67.1 `layout_views`: `ViewKind` per tab; tab run Marker → Nested → Base Size → sizes; the base size's own tab reuses the Base Size layout
- [x] Layout.67.2 Settings "Multisize Layout" removed (`SettingsDialog.qml`, `SettingsModel.{h,cpp}`, `layout_settings.rs`); a stored `multisizeLayout` key is ignored
- [x] Layout.67.3 PDF and tiled exports take the packing mode from the shown tab
- [x] Layout.67.4 DXF-ASTM grading: Base Size tab only; the base size block gets `# n` texts and the layer 5 line; no graded nest
- [x] Layout.67.5 Export menu: DXF-ASTM disabled on Marker and Nested, with a note. Export all tabs: DXF-ASTM skips Marker and Nested; other formats skip Base Size
- [x] Layout.67.6 Tests: Rust `layout_views`, grading, writer, Base Size and size tab exports (jacket2 included); Qt `SettingsModelTests::multisizeLayout_storedKeyIsDropped`
- [x] Layout.67.7 Docs: DXF-ASTM compliance doc, simple workflow, settings workflow, `SVG-DATA-ATTRIBUTES.md`
- [ ] Layout.67.8 User: on the next MSI, run `TEST_CHECKLIST.md` §7 "Multisize tabs"
- [ ] Layout.67.9 "Export Complete" names only the `.dxf`. A Base Size export also writes the `.rul`; name both files (`jacket2_basesize_screenshot_1.png`, MSI 26.10.1311)

## [ ] Task Layout.24 — Multisize export file name: tab name before the date stamp

Default name of a multisize export: `<base>[_<segment>]_<tab>_<stamp>[_tiled].<ext>`. Example: `mypatternname_R13_Size220_202610112043.dxf`. An individual pattern keeps `<base>[_<segment>]_<stamp>[_tiled].<ext>`.

- [ ] Layout.24.1 `exports::default_export_file_name`: optional tab segment between `<segment>` and `<stamp>`
- [ ] Layout.24.2 Tab segment from the tab label: remove spaces; any other character that is not a letter, digit or `-` becomes `-`. "Size 220" → `Size220`, "All sizes - group" → `Allsizes-group`
- [ ] Layout.24.3 Single export from a multisize layout: the default name uses the selected tab
- [ ] Layout.24.4 Export all tabs: each file name uses the same rule. Replaces `layout_views::tab_file_path`, which puts `_size-220` after the stamp. A name the user changed in the file dialog gets `_<tab>` before the extension
- [ ] Layout.24.5 Tests: Rust names with and without a tab, tiled, a label with `/`; Export all tabs names
- [ ] Layout.24.6 Docs: export file name rule in the SeamlyLayout docs, `TEST_CHECKLIST.md`

## [ ] Task Layout.25 — Multisize Marker layout: extend the media height until every piece fits

Applies to every multisize tab: "All sizes" (Nested or Marker), and each size tab (user decision: D3). It does not wait for Layout.23; after Layout.23 the rule covers both all-sizes tabs. Today a piece that does not fit is left out, and a warning names it.

Evidence (jacket2, Marker, 48 in × 500 in paper roll): the pack placed 96 of 208 pieces. `test-seamly-layout/layouts/jacket2/jacket2_marker_screenshot_1.png`.

Repeat with MSI 26.10.1311, Marker tab: 96 of 208 placed again. The roll is full: pieces use 74 % of its area. The 208 pieces need about 740 in. `jacket2_screenshot_1.png`, `jacket2_40-146_202610131245.svg`.

Decision (user, 2026-10-13): a layout cuts fabric, so the media grows past the 500 in roll length when the pieces need it.

- [x] Layout.25.1 When the pack leaves pieces out, increase the media height and pack again until every piece fits
  - `packing::pack_polygons_extending`: the bin grows by the stacked height of the unplaced pieces until every piece that fits the width is placed. jacket2 Marker, 48 in roll: 208 of 208 placed, about 798 in
  - Multisize tabs only, as scoped above. An individual pattern still leaves out a piece that does not fit
- [x] Layout.25.2 Media types: paper sheet, paper roll (500 in default length), fabric
  - Roll and fabric end at the lowest piece. A sheet ends at the lowest piece, but not above its own length. Tiled paper does not extend
- [x] Layout.25.3 A piece wider than the media width cannot fit at any height: leave it out and keep the warning
  - Replaced by Layout.71: the media widens, so no multisize piece is left out
- [x] Layout.25.4 Canvas, Adjust Mode and exports use the extended height
  - Exception, unchanged: the paper-sheet PDF export packs the pieces again on sheets of the set size
- [x] Layout.25.5 Tests: Rust pack with more pieces than one sheet holds → every piece placed, height grows; a piece that is too wide stays unplaced
  - `extending_*` (`layout_engine`), `pack_polygons_extending_grows_and_reports_too_wide`, `jacket2_marker_extends_paper_roll`, `jacket2_marker_extends_paper_sheet`
- [x] Layout.25.6 Docs: layout workflow, `TEST_CHECKLIST.md`
- [ ] Layout.25.7 User: on the next MSI, run `TEST_CHECKLIST.md` §7 "Media grows to fit"

## [ ] Task Layout.71 — Multisize: widen the media for a piece wider than it

Decision (user, 2026-10-13): on a multisize tab, a piece wider than the content rect widens it to the right; a piece longer than it lengthens it downwards (Layout.25). Splitting pieces is for individual patterns only (Layout.72).

- [x] Layout.71.1 `layout_engine::required_bin_width`: widest piece, each in its narrowest trial angle; `layout_utils` widens the bin to it when `extend_media` is set. Tiled paper unchanged
- [x] Layout.71.2 `layout_assembler::set_content_width`: SVG, `backgroundRect` and `contentRect` widths; contentRect keeps its x, both side margins kept
- [x] Layout.71.3 Tab warning: the Layout.25 text "grows in length, not in width" removed; non-tiled tabs leave no piece out
- [x] Layout.71.4 Tests: `required_bin_width_uses_narrowest_angle`, `set_content_width_keeps_margins`, `jacket2_marker_widens_narrow_roll` (10 in roll, 208 of 208)
- [x] Layout.71.5 Docs: layout workflows
- [ ] Layout.71.6 User: on the next MSI, jacket2, Paper Roll 10 in wide, Create Layout: no warning; every tab is wider than 10 in; Adjust Mode and Export SVG show the same width

## [ ] Task Layout.72 — Individual pattern: a piece wider or longer than the media

Decision (user, 2026-10-13): when a piece of an individual pattern is wider or longer than the media, a dialog asks the user to split the pieces or make the media bigger.

- Dialog: names each piece and the side that does not fit. Radio buttons: "Split the pieces" (SVG, PDF, HPGL, EPS, PS, PNG, JPG: printing, cutting, pattern projector) or "Make the media bigger" (DXF). OK continues the layout. Cancel returns to the app with only 'Layout Settings' active.
- Split rule: wider than the content width, and no allowed turn fits → vertical cut lines. Longer than the content height, media not roll-form (paper roll, fabric), and no allowed turn fits → horizontal cut lines. Tiled paper: no dialog.
- Make the media bigger: same as a multisize tab (Layout.71, Layout.25).
- Accepted recommendations (user, 2026-10-13): no seam allowance on the cut edge; n equal parts, n rounded up; label plus 2 match notches per cut edge; fabric is roll-form; the paper-sheet PDF export uses the same split; a piece that fits but runs out of room is still left out with a warning.

- [ ] Layout.72.1 `geometry`: cut a closed or open polyline with a line; keep each vertex's source index; one region per part
- [ ] Layout.72.2 `piece_divider.rs`: split a piece group into parts. Cut line and cut paths closed; seam line and internal paths open; notches, labels by position; grainline copied into a part without one. New `piece_label` "<name> part i of n, joins j"; 2 match notches per cut edge; `data-split-of`, `data-split-part`, `data-split-count`; ids `<id>_part<i>`
- [ ] Layout.72.3 Pipeline: find oversized pieces after the pieces turn grain-up; split before they move to the origin. Same split in `sheets::build_sheet_export_inputs`
- [ ] Layout.72.4 Bridge and QML: oversized-pieces check before the layout, dialog in `Main.qml`, Cancel state
- [ ] Layout.72.5 DXF-ASTM: a part is its own block, `Piece Name:` "Front (1/2)", not graded, with a warning
- [ ] Layout.72.6 Tests: Rust clip cases (square, U-shape, vertex on the cut line, open line), trousers leg on a narrow sheet, DXF-ASTM of a split piece
- [ ] Layout.72.7 Docs: `SVG-DATA-ATTRIBUTES.md`, layout workflows, `TEST_CHECKLIST.md`

## [ ] Task Layout.26 — DXF-ASTM Missing Piece Data dialog: one readable line per piece

Found with the installed sample `jacket2_40-146.sm2d` (`gost_man_ru.smms`), "All sizes" tab, Marker. Evidence: `test-seamly-layout/layouts/jacket2/jacket2_marker_screenshot_2.png`, `jacket2_nested_screenshot_1.png`. The installed sample has Cyrillic names because the local MSI ships stale copies (Layout.40). The pattern hides each piece label and grainline (`visible="false"`), so those warnings are correct.

- [x] Layout.26.1 One line per piece, not one per placed size: group the blocks by `data-parent` (piece-set id)
- [x] Layout.26.2 The dialog clips its text: the last line is not visible. Add a scroll area; keep the buttons visible
- [x] Layout.26.3 Non-ASCII names: `sanitize_ascii` empties "Подборт" and "Комір", so the dialog shows the block id `piece-201_s720_M`. Show the `data-name` text in dialogs and export warnings
- [x] Layout.26.4 DXF `Piece Name:` for a non-ASCII name is the block id today -- Piece name does not differ per size -- so aw.fyi cannot group the sizes. Write each non-ASCII character as an AutoCAD `\U+XXXX` escape: "Комір" → `\U+041A\U+043E\U+043C\U+0456\U+0440` (user decision: D2). Every size of a piece gets the same name
- [x] Layout.26.5 Pieces with the same name (6 × "Detail") cannot be told apart. Add the piece letter, else the piece number: "Detail (3)". Applies also to the DXF `Piece Name:`: aw.fyi merges the 6 "Detail" pieces into one. That merge causes the aw.fyi departures "Sample size points incomplete", "Excluded layers present in graded sizes" and "L5 endpoint mismatch 103.74" (checked: per piece, L5 is a pure translation)
- [x] Layout.26.6 Tests: Rust dialog text — one line per piece set, Cyrillic name kept, same names numbered
  - `piece_names_keep_unicode_and_number_shared_names`, `hoist_tagged_pieces_dissolves_piece_sets` (`data-set-number`), `jacket2_missing_piece_data_lists_each_piece_once`, `jacket2_all_sizes_export_names_every_size_alike`
- [ ] Layout.26.7 User: on the next MSI, check the dialog scrolls and keeps its buttons (jacket2, All sizes, Marker)

## [ ] Task Layout.27 — DXF-ASTM grading fails for jacket2_40-146 from the "All sizes" tab

Evidence: `test-seamly-layout/layouts/jacket2/` — `jacket2_nested_screenshot_2.png`, `jacket2_marker_screenshot_3.png`, the `nested_allsizes` and `marker_allsizes` DXF files and aw.fyi reports. 8 pieces in sizes 220–720.

- Nested: 4 pieces graded; 4 "Detail" pieces fell back to size 500.
- Marker: the layout placed 96 of 208 pieces (Layout.25). 4 pieces lost size 500; the other 4 failed the turn point check, so grading removed 71 placed blocks. The file has 25 blocks and no `.rul`.
- MSI 26.10.1311, Base Size 500 tab (`jacket2_40-146_basesize_R13_202610131248.dxf`): 4 pieces graded. Their rules match all 26 sizes within 0.011 mm (cut and sew lines, checked against the Nested SVG). Detail (3), (6), (7), (8) not graded again (Layout.27.3).

- [ ] Layout.27.1 "size 500 is not in the layout" (3 pieces): the Marker layout left the sample size block out. Layout.25 fixes this; export again and check
- [ ] Layout.27.2 When the layout left the sample size block out, the warning says so: "size 500 does not fit the media"
- [ ] Layout.27.3 "the sizes have different turn points" (4 "Detail" pieces). Cause (checked in the per-size Export-all-tabs DXF files, layer 2 counts): Seamly2D's `VAbstractPiece::turnPointIndices` flips at near-limit corners as the size changes. Piece 3: 11 → 10 from size 360; piece 1: 12 → 14/16 at 660–700; piece 6: 8 → 10 from 660; piece 7: 8 ↔ 10 from 520. Fix in the producer: after `prepareMultisizePieceLists`, a piece node that is a turn point in any size is a turn point in every size (seam line, cut line, internal paths). Needs the node vertex indices of each path in `VLayoutPiece` / `VLayoutPiecePath`. Skip a path whose node count differs between sizes
- [ ] Layout.27.4 `grade_anchors` also returns None when a key point is not on the dense line. That case gets its own message
- [ ] Layout.27.5 The turn point warning names the first size that differs from the sample size
- [x] Layout.27.6 Warnings use the piece names from Layout.26.3 and Layout.26.5
  - MSI 26.10.1311: "Export Complete" names `Detail (3)`, `Detail (6)`, … (`jacket2_basesize_screenshot_1.png`)
- [ ] Layout.27.7 Fixture: the jacket2 handoff in `crates/cxxqt_bridge/test_data/`; end-to-end test: every piece graded, `.rul` file written
- [ ] Layout.27.8 No `.rul` file: `astm_grading::grade_drawing` sets a rule table only when one piece or more grades. When no piece grades, "Export Complete" says "No piece graded: no .rul file written"
- [x] Layout.27.9 Duplicate vertices: aw.fyi reports layers 84 (54) and 87 (450) in the graded nested file only. `GradeSize::vertex_at` inserts a key point 0.000001–0.01 mm from a dense vertex (`SAME_VERTEX_MM` = 1e-6). Snap to the vertex within 0.01 mm; drop a curve key point that lands on the vertex of the key point before or after it. Test: no consecutive dense or reduced vertices closer than 0.01 mm
- [ ] Layout.27.10 Marker layout, piece that cannot grade: write every placed size as its own ungraded piece, size in the piece name: "Detail (3) 240" (user decision: D4). Today only the sample size is written, so placed cut pieces are lost. Nested keeps the sample size only: its sizes overlap
- [ ] Layout.27.11 Near-duplicate vertices remain. Base Size file (MSI 26.10.1311), Подборт, layer 87: `(333.01, 626.21)` then `(333.01, 626.22)`. `SNAP_VERTEX_MM` (0.01) equals the written precision (2 decimals), so a key point 0.01–0.02 mm from a vertex stays a separate vertex. Snap at 0.02 mm, or compare the written coordinates. Test: no two consecutive written vertices 0.01 mm apart or closer

## [ ] Task Layout.40 — Local MSI ships stale sample patterns

The installed `jacket2_40-146.sm2d` has Cyrillic piece names; the repo copy (`src/app/share/samples/patterns/`) has English names. `copyToDestdir` in `common.pri` skips a file that exists, so `src/app/seamly2d/bin/samples` and `src/app/seamlyme/bin/samples` keep copies from 2026-08-10. `smsi.ps1` stages those `bin` folders. CI builds from a clean checkout and is not affected.

- [x] Layout.40.1 `common.pri` `copyToDestdir`: on Windows copy every file on each link, not only a missing one
- [x] Layout.40.2 Check: after `local_build_msi.ps1`, `packaging/windows/seamly-msi/x64/parent/samples/patterns/jacket2_40-146.sm2d` matches the repo copy
  - MSI 26.10.1311: the staged copy and the installed copy (`C:\Program Files\SeamlyApps\samples\patterns\`) match the repo copy
- [x] Layout.40.3 `common.pri` is a `*.pri` change: the merge commit runs full CI (no skip-ci token)

## [ ] Task Layout.28 — Built-in seam allowance: show the seam line

Before this task: a piece with a built-in seam allowance had no seam line. Its main path is the cut line, and the handoff sent it as `seamline`. SeamlyLayout drew only that line. Seamly2D drew no seam line for it either.

Seamly2D computes the seam line: only Seamly2D knows the seam allowance width at each node.

The task stays in this file: it changes what SeamlyLayout lays out and exports (user decision).

Status (2026-10-12):

- Code, tests and docs: done. Merged into local `run-seamlyLayout`, not pushed
- `cargo test --workspace`: 472 passed, 0 failed. `local_build_msi.ps1`: Qt suites passed, MSI 26.10.1220 built
- Open: the user check in the installed apps (Layout.28.7)

- [x] Layout.28.1 Seamly2D: seam line = main path offset inward by each node's width (wider of before and after; piece width when the node sets none)
  - `VPiece::builtInSeamLinePoints()`. A curve spreads the widths of its ends along its length. Corner types shape only the cut line: seam line corners are plain offset corners (user confirmed)
- [x] Layout.28.2 Handoff of a built-in piece: main path as `cutline`, computed line as `seamline`. Same rule for a hidden seam line as a piece without a built-in seam allowance
  - `VLayoutPiece::createSeamlineItem()`, `createAllowanceItem()`. The built-in `cutline` is sent also when seam allowances are not shown: it is the piece outline
- [x] Layout.28.3 SeamlyLayout canvas, Adjust Mode and every export draw the seam line. Packing still uses the cut line
  - No SeamlyLayout code change: every view and export already draws `seamline` and `cutline` groups, and packing prefers `cutline`
- [x] Layout.28.4 DXF-ASTM: cut line is the boundary (layer 1); seam line is a sew line (layer 14)
  - No code change: `converter::extract_astm_piece` already does this for a piece with both lines
- [x] Layout.28.5 Tests: `TST_SvgComponentTags` — a built-in piece has `cutline` and `seamline`; Rust — DXF layers 1 and 14 for a built-in piece
  - `TST_SvgComponentTags::BuiltInPieceSendsCutlineAndSeamline`, `BuiltInPieceHiddenSeamLineSendsCutlineOnly`; `TST_VPiece::BuiltInSeamLineUsesWiderNodeWidth`, `BuiltInSeamLineFollowsCurve`
  - Rust: `built_in_seam_allowance_writes_boundary_and_sew_line`, `built_in_seam_allowance_has_boundary_and_sew_line`, `built_in_seam_allowance_packs_by_its_cut_line`
- [x] Layout.28.6 Docs: `SVG-DATA-ATTRIBUTES.md` (`cutline`, `seamline`, `data-seam-allowances`), `NEW-ATTRIBUTES.csv`, DXF-ASTM compliance doc, `TEST_CHECKLIST.md`
- [ ] Layout.28.7 User: on the next MSI, run `TEST_CHECKLIST.md` §5 "Piece with a built-in seam allowance"
- [x] Layout.28.8 Verify: Qt suites (`nmake check`) in `local_build_msi.ps1`
  - The first run found a bug: a main path that starts at a corner next to a curve lost most of its seam line. The inward offset loops at that corner, and `CheckLoops()` cut the wrong side of a loop at the path start. Fix: the offset starts in the middle of the longest edge
- Not in this task: Seamly2D piece mode draws no seam line for a built-in piece. Filed as Seamly2D.11 in `TODO_SEAMLY2D.md` (user decision)

## [ ] Task Layout.68 — PDF export: page and pieces 133 % too large

Found with jacket2, MSI 26.10.1311. `svg2pdf` `PageOptions::default()` uses 72 dpi, so 1 SVG px (1/96 in) becomes 1 pt (1/72 in).

- Marker `jacket2_40-146_202610131244.pdf` (48 in × 500 in roll): MediaBox `4608 × 48000` pt = 64 in × 666.7 in. Expected `3456 × 36000` pt.
- Nested `jacket2_40-nested_146_202610131245.pdf`: `4608 × 5003` pt. Expected `3456 × 3752` pt.
- EPS and PS of the same tabs are correct: `0.75` scale, BoundingBox `3456 36000`.

- [x] Layout.68.1 One renderer for every PDF page: `exports::render_tree_to_pdf_bytes` passes `PageOptions { dpi: 96 }`. `app_core::render_pdf` removed
- [x] Layout.68.2 PDF (Tiled) and sheet pages use the same renderer. A Letter tile page is `612 × 792` pt
- [x] Layout.68.3 Acrobat limits a page to 200 in (14 400 pt). A 500-in roll page is 36 000 pt. Decision (user): a fabric layout can be longer than 500 in, so a page side longer than 200 in gets `/UserUnit` n (smallest whole number). A reader that ignores `/UserUnit` shows the page at 1/n size
- [x] Layout.68.4 Tests: Rust MediaBox of a 48 in × 500 in layout = `3456 × 36000`; tiled Letter page = `612 × 792`
  - `pdf_page_is_true_size`, `pdf_page_longer_than_200_in_sets_user_unit`, `tiled_pdf_pages_are_paper_size`
- [x] Layout.68.6 Docs: `export-docs/seamlylayout_EXPORT_WORKFLOW.md` "PDF export — true size", simple workflow, codebase index
- [ ] Layout.68.5 User: on the next MSI, print one tiled page at 100 %; measure a known length. Open the Marker PDF in Acrobat: Document Properties shows 48 × 500 in

## [ ] Task Layout.69 — PNG and JPG exports: no resolution metadata

Found with jacket2, MSI 26.10.1311. The pixel size is correct: `jacket2_40-146_202610131243.png` is 4608 × 48000 px, 96 px/in. The PNG has no `pHYs` chunk. The JPG JFIF density unit is 0 (aspect ratio only). A viewer or print dialog then uses its own default resolution, so "actual size" can print at the wrong scale.

- [ ] Layout.69.1 PNG: write `pHYs` 3780 px/m (96 dpi). JPG: JFIF density 96 × 96 dpi
- [ ] Layout.69.2 Tests: Rust reads the PNG `pHYs` and the JPG JFIF density

## [ ] Task Layout.70 — "Some pieces could not be placed" dialog: piece names and length

Found with jacket2, MSI 26.10.1311, Marker tab: 112 entries such as "Detail (size 240)" (`jacket2_screenshot_1.png`). The 6 "Detail" pieces cannot be told apart. The dialog has no scroll area; it grows past the screen when more pieces are left out.

- [ ] Layout.70.1 Use the numbered display name: "Detail (3)" (same rule as Layout.26.5)
- [ ] Layout.70.2 One line per piece, then its sizes: "Detail (3): 220, 240, 260". Group by `data-parent` (as Layout.26.1)
- [ ] Layout.70.3 `Main.qml` `warningDialog`: scroll area; the OK button stays visible (as Layout.26.2)
- [ ] Layout.70.4 Tests: Rust warning text for a jacket2 Marker layout that leaves pieces out

## [x] Task Layout.8 — SeamlyLayout default paths don't resolve under %DATAROOT%

## [x] Task Layout.9 — Piece-mode handoff passes a file, not a stringified SVG document
