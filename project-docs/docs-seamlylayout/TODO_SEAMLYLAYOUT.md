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
  - [ ] Layout.38.2 Real multisize handoff fixture: export from a `.sm2d` + `.smms` pattern in the running app, save to `crates/cxxqt_bridge/test_data/`, add an end-to-end test
    - [x] Layout.38.2.1 `SEAMLYLAYOUT_SAVE_HANDOFF=<absolute path>`: `import_svg_document` writes the received SVG there (`handoff_capture.rs`)
    - [ ] Layout.38.2.2 User: capture the handoff of `HJA.sm2d` + its `.smms`; then add the fixture and an end-to-end test
  - [ ] Layout.38.3 Check in the running apps: Seamly2D Layout Mode with a multisize pattern; SeamlyLayout Nested and Marker
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
  - [ ] Layout.61.7 User: export a multisize pattern from "All sizes" as DXF-ASTM; check it in aw.fyi (DXF.8.4) and one CAD reader
- Layout.62 Moved to Layout.16: mirror line (layer 6) needs Seamly2D fold lines (Seamly2D.8)
- [ ] Layout.63 Drill holes (layer 13), with diameter (group 30) - not implemented in Seamly2D yet so this must wait
- [ ] Layout.64 Stripe and plaid reference lines (layers 9, 10) - not implemented in Seamly2D yet so this must wait
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

- [ ] Layout.25.1 When the pack leaves pieces out, increase the media height and pack again until every piece fits
- [ ] Layout.25.2 Media types: paper sheet, paper roll (500 in default length), fabric
- [ ] Layout.25.3 A piece wider than the media width cannot fit at any height: leave it out and keep the warning
- [ ] Layout.25.4 Canvas, Adjust Mode and exports use the extended height
- [ ] Layout.25.5 Tests: Rust pack with more pieces than one sheet holds → every piece placed, height grows; a piece that is too wide stays unplaced
- [ ] Layout.25.6 Docs: layout workflow, `TEST_CHECKLIST.md`

## [ ] Task Layout.26 — DXF-ASTM Missing Piece Data dialog: one readable line per piece

Found with the installed sample `jacket2_40-146.sm2d` (`gost_man_ru.smms`), "All sizes" tab, Marker. Evidence: `test-seamly-layout/layouts/jacket2/jacket2_marker_screenshot_2.png`, `jacket2_nested_screenshot_1.png`. The installed sample has Cyrillic names because the local MSI ships stale copies (Layout.40). The pattern hides each piece label and grainline (`visible="false"`), so those warnings are correct.

- [ ] Layout.26.1 One line per piece, not one per placed size: group the blocks by `data-parent` (piece-set id)
- [ ] Layout.26.2 The dialog clips its text: the last line is not visible. Add a scroll area; keep the buttons visible
- [ ] Layout.26.3 Non-ASCII names: `sanitize_ascii` empties "Подборт" and "Комір", so the dialog shows the block id `piece-201_s720_M`. Show the `data-name` text in dialogs and export warnings
- [ ] Layout.26.4 DXF `Piece Name:` for a non-ASCII name is the block id today, and it differs per size, so aw.fyi cannot group the sizes. Write each non-ASCII character as an AutoCAD `\U+XXXX` escape: "Комір" → `\U+041A\U+043E\U+043C\U+0456\U+0440` (user decision: D2). Every size of a piece gets the same name
- [ ] Layout.26.5 Pieces with the same name (6 × "Detail") cannot be told apart. Add the piece letter, else the piece number: "Detail (3)". Applies also to the DXF `Piece Name:`: aw.fyi merges the 6 "Detail" pieces into one. That merge causes the aw.fyi departures "Sample size points incomplete", "Excluded layers present in graded sizes" and "L5 endpoint mismatch 103.74" (checked: per piece, L5 is a pure translation)
- [ ] Layout.26.6 Tests: Rust dialog text — one line per piece set, Cyrillic name kept, same names numbered

## [ ] Task Layout.27 — DXF-ASTM grading fails for jacket2_40-146 from the "All sizes" tab

Evidence: `test-seamly-layout/layouts/jacket2/` — `jacket2_nested_screenshot_2.png`, `jacket2_marker_screenshot_3.png`, the `nested_allsizes` and `marker_allsizes` DXF files and aw.fyi reports. 8 pieces in sizes 220–720.

- Nested: 4 pieces graded; 4 "Detail" pieces fell back to size 500.
- Marker: the layout placed 96 of 208 pieces (Layout.25). 4 pieces lost size 500; the other 4 failed the turn point check, so grading removed 71 placed blocks. The file has 25 blocks and no `.rul`.

- [ ] Layout.27.1 "size 500 is not in the layout" (3 pieces): the Marker layout left the sample size block out. Layout.25 fixes this; export again and check
- [ ] Layout.27.2 When the layout left the sample size block out, the warning says so: "size 500 does not fit the media"
- [ ] Layout.27.3 "the sizes have different turn points" (4 "Detail" pieces). Cause (checked in the per-size Export-all-tabs DXF files, layer 2 counts): Seamly2D's `VAbstractPiece::turnPointIndices` flips at near-limit corners as the size changes. Piece 3: 11 → 10 from size 360; piece 1: 12 → 14/16 at 660–700; piece 6: 8 → 10 from 660; piece 7: 8 ↔ 10 from 520. Fix in the producer: after `prepareMultisizePieceLists`, a piece node that is a turn point in any size is a turn point in every size (seam line, cut line, internal paths). Needs the node vertex indices of each path in `VLayoutPiece` / `VLayoutPiecePath`. Skip a path whose node count differs between sizes
- [ ] Layout.27.4 `grade_anchors` also returns None when a key point is not on the dense line. That case gets its own message
- [ ] Layout.27.5 The turn point warning names the first size that differs from the sample size
- [ ] Layout.27.6 Warnings use the piece names from Layout.26.3 and Layout.26.5
- [ ] Layout.27.7 Fixture: the jacket2 handoff in `crates/cxxqt_bridge/test_data/`; end-to-end test: every piece graded, `.rul` file written
- [ ] Layout.27.8 No `.rul` file: `astm_grading::grade_drawing` sets a rule table only when one piece or more grades. When no piece grades, "Export Complete" says "No piece graded: no .rul file written"
- [ ] Layout.27.9 Duplicate vertices: aw.fyi reports layers 84 (54) and 87 (450) in the graded nested file only. `GradeSize::vertex_at` inserts a key point 0.000001–0.01 mm from a dense vertex (`SAME_VERTEX_MM` = 1e-6). Snap to the vertex within 0.01 mm; drop a curve key point that lands on the vertex of the key point before or after it. Test: no consecutive dense or reduced vertices closer than 0.01 mm
- [ ] Layout.27.10 Marker layout, piece that cannot grade: write every placed size as its own ungraded piece, size in the piece name: "Detail (3) 240" (user decision: D4). Today only the sample size is written, so placed cut pieces are lost. Nested keeps the sample size only: its sizes overlap

## [ ] Task Layout.29 — Notch attributes: one value per notch, not per line segment

Found in `TEST_CHECKLIST.md` §2: BackPanel `data-notch-type` holds 15 words for 2 notches (`slit` + 14 × `uNotch`). One word per notch (user decision: D1).

- [ ] Layout.29.1 Seamly2D: `VPiece::createNotchLines` returns one type and one width per notch node, and the line count of each notch
- [ ] Layout.29.2 `VLayoutPiece`: store per-notch types, widths and line counts. `createNotchesItem` writes `data-notch-type` and `data-seam-allowances` (one value per notch) and `data-notch-segments` (line count per notch, `d` order). Sum of `data-notch-segments` = `M` count in `d`
- [ ] Layout.29.3 SeamlyLayout `converter::notch_polylines`: spread each per-notch width over its segments with `data-notch-segments`. A list of the wrong length is ignored
- [ ] Layout.29.4 Tests: `TST_SvgComponentTags::NotchGroupCarriesNotchTypes`, `TST_VPiece::NotchTypesFollowNodes`, Rust `notch_polylines` with a 14-segment U-notch
- [ ] Layout.29.5 Docs: `SVG-DATA-ATTRIBUTES.md`, `NEW-ATTRIBUTES.csv`, compliance doc "Notch rules", `TEST_CHECKLIST.md`

## [ ] Task Layout.40 — Local MSI ships stale sample patterns

The installed `jacket2_40-146.sm2d` has Cyrillic piece names; the repo copy (`src/app/share/samples/patterns/`) has English names. `copyToDestdir` in `common.pri` skips a file that exists, so `src/app/seamly2d/bin/samples` and `src/app/seamlyme/bin/samples` keep copies from 2026-08-10. `smsi.ps1` stages those `bin` folders. CI builds from a clean checkout and is not affected.

- [x] Layout.40.1 `common.pri` `copyToDestdir`: on Windows copy every file on each link, not only a missing one
- [ ] Layout.40.2 Check: after `local_build_msi.ps1`, `packaging/windows/seamly-msi/x64/parent/samples/patterns/jacket2_40-146.sm2d` matches the repo copy
- [x] Layout.40.3 `common.pri` is a `*.pri` change: the merge commit runs full CI (no skip-ci token)

## [ ] Task Layout.28 — Built-in seam allowance: show the seam line

A piece with a built-in seam allowance has no seam line. Its main path is the cut line, and the handoff sends it as `seamline`. SeamlyLayout draws only that line. Seamly2D draws no seam line for it either.

Seamly2D computes the seam line: only Seamly2D knows the seam allowance width at each node.

The task stays in this file: it changes what SeamlyLayout lays out and exports (user decision).

- [ ] Layout.28.1 Seamly2D: seam line = main path offset inward by each node's width (wider of before and after; piece width when the node sets none)
- [ ] Layout.28.2 Handoff of a built-in piece: main path as `cutline`, computed line as `seamline`. Same rule for a hidden seam line as a piece without a built-in seam allowance
- [ ] Layout.28.3 SeamlyLayout canvas, Adjust Mode and every export draw the seam line. Packing still uses the cut line
- [ ] Layout.28.4 DXF-ASTM: cut line is the boundary (layer 1); seam line is a sew line (layer 14)
- [ ] Layout.28.5 Tests: `TST_SvgComponentTags` — a built-in piece has `cutline` and `seamline`; Rust — DXF layers 1 and 14 for a built-in piece
- [ ] Layout.28.6 Docs: `SVG-DATA-ATTRIBUTES.md` (`cutline`, `seamline`, `data-seam-allowances`), `NEW-ATTRIBUTES.csv`, DXF-ASTM compliance doc, `TEST_CHECKLIST.md`

## [x] Task Layout.8 — SeamlyLayout default paths don't resolve under %DATAROOT%

## [x] Task Layout.9 — Piece-mode handoff passes a file, not a stringified SVG document
