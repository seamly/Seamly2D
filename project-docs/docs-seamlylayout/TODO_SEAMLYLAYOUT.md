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
- [ ] Layout.36 Export: user chooses the multisize export (user, 2026-09-28)
  - [x] Layout.36.1 **Nested**: one file; each piece keeps its `piece-set` group; every piece keeps `data-size`
  - [ ] Layout.36.2 **By size**: one file per size, file name suffixed with the size; each file is that size's own packed layout from Layout.37, not a filter of the Nested layout
- [ ] Layout.37 Sized layouts: pack each size again as its own layout (only that size's pieces, same settings); per-size canvas tabs; multi-page PDF option. Layout.36.2 depends on this
- [ ] Layout.38 Tests
  - [x] Layout.38.1 Rust: set hoist, centring, set bbox, set outline, marker count = pieces × sizes (synthetic fixture `MULTISIZE_HANDOFF_SVG`)
  - [ ] Layout.38.2 Real multisize handoff fixture: export from a `.sm2d` + `.smms` pattern in the running app, save to `crates/cxxqt_bridge/test_data/`, add an end-to-end test
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
- [ ] Layout.62 Mirror line (layer 6) for pieces cut on the fold
- [ ] Layout.63 Drill holes (layer 13), with diameter (group 30)
- [ ] Layout.64 Stripe and plaid reference lines (layers 9, 10)
- [ ] Layout.65 Seamly2D writes empty notch paths (`M x,y Z`) to the handoff SVG, so no notches reach the DXF — fix the producer
- [ ] Layout.66 Seamly2D producer attributes: `data-quantity` (right/left), `data-on-fold`, `data-material`, `data-notch-type` — replaces label parsing and notch-shape inference

## [x] Task Layout.8 — SeamlyLayout default paths don't resolve under %DATAROOT%

## [x] Task Layout.9 — Piece-mode handoff passes a file, not a stringified SVG document
