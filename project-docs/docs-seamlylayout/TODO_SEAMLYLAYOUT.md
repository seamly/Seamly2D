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
- Layout.62 Moved to Layout.16: mirror line (layer 6) needs Seamly2D fold lines (Seamly2D.8)
- [ ] Layout.63 Drill holes (layer 13), with diameter (group 30)
- [ ] Layout.64 Stripe and plaid reference lines (layers 9, 10)
- [ ] Layout.65 Seamly2D writes empty notch paths (`M x,y Z`) to the handoff SVG, so no notches reach the DXF — fix the producer
- [ ] Layout.66 Seamly2D producer attributes: `data-quantity` (right/left), `data-on-fold`, `data-notch-type` — replaces label parsing and notch-shape inference. `data-material` moved to Seamly2D.7 / Layout.15

## [ ] Task Layout.15 — Layout by material

Depends on Seamly2D.7. Until then every piece is on one fabric.

- [ ] Layout.15.1 Import: read `data-material` and per-material quantity
- [ ] Layout.15.2 One layout tab per material; each packs only that material's pieces
- [ ] Layout.15.3 Quantity: place each piece its cut count per material
- [ ] Layout.15.4 Exports: material in the default file name; DXF-ASTM `Material:` piece text
- [ ] Layout.15.5 Tests: Rust import, packing per material, export text

## [ ] Task Layout.16 — Fold lines in layout and export

Depends on Seamly2D.8.

- [ ] Layout.16.1 Import: read `data-on-fold` and the `fold_line` component
- [ ] Layout.16.2 Layout setting: pack the half piece, or unfold it (mirror across the fold line) before packing
- [ ] Layout.16.3 Adjust Mode: the fold line moves with its piece
- [ ] Layout.16.4 DXF-ASTM: layer 6 mirror line; boundary starts or ends on the mirror line points (D6673 §4.3.6); unfolded pieces write no layer 6
- [ ] Layout.16.5 Tests: Rust import, unfold geometry, layer 6 and boundary order

## [ ] Task Layout.17 — HPGL cut-lines export name has `_cutlines`

Export > HPGL > Cut (cut lines only) uses the same default name as Plot. The user cannot tell the two files apart.

- [ ] Layout.17.1 `Main.qml` `onExportHpglRequested`: mode `"cut"` puts `_cutlines` before the timestamp. Example: `male_shirt_cutlines_202610011721.plt`
- [ ] Layout.17.2 `makeExportFileName`: add the segment parameter; `_tiled` keeps its place after the timestamp
- [ ] Layout.17.3 Mode `"plot"` name stays `<importedBaseName>_YYYYMMDDHHMM.<ext>`
- [ ] Layout.17.4 Export all tabs: tab label still goes before the extension
- [ ] Layout.17.5 Update `export-docs/seamlylayout_EXPORT_WORKFLOW.md` and the name comments in `Main.qml`
- [ ] Layout.17.6 Test: cut name has `_cutlines`; plot and tiled names unchanged

## [ ] Task Layout.18 — SVG export name shows the label text mode

Export > SVG uses one default name for every text mode. The user cannot tell the files apart. Uses the segment parameter from Layout.17.2.

- [ ] Layout.18.1 `Main.qml` `onExportSvgRequested`: mode `"singleLineFont"` puts `_singlelinefont` before the timestamp. Example: `male_shirt_singlelinefont_202610011721.svg`
- [ ] Layout.18.2 Mode `"hersheyStrokes"` puts `_hersheyfont` before the timestamp. Example: `male_shirt_hersheyfont_202610011721.svg`
- [ ] Layout.18.3 Modes `"designerFont"` and `"asSupplied"` name stays `<importedBaseName>_YYYYMMDDHHMM.svg`
- [ ] Layout.18.4 Export all tabs: tab label still goes before the extension
- [ ] Layout.18.5 Update `export-docs/seamlylayout_EXPORT_WORKFLOW.md` and the name comments in `Main.qml`
- [ ] Layout.18.6 Test: each mode gives its name

## [ ] Task Layout.19 — DXF-ASTM (CLO3D) export name has `_CLO3D`

Export > DXF-ASTM (CLO3D) uses the same default name as DXF-ASTM. The user cannot tell the two files apart. Uses the segment parameter from Layout.17.2.

- [ ] Layout.19.1 `Main.qml` `requestDxfExport(clo3d)`: `clo3d` true puts `_CLO3D` before the timestamp. Example: `male_shirt_CLO3D_202610011721.dxf`
- [ ] Layout.19.2 Keep the uppercase `CLO3D`; other segments are lowercase
- [ ] Layout.19.3 DXF-ASTM name stays `<importedBaseName>_YYYYMMDDHHMM.dxf`
- [ ] Layout.19.4 Export all tabs: tab label still goes before the extension
- [ ] Layout.19.5 Update `export-docs/seamlylayout_EXPORT_WORKFLOW.md` and the name comments in `Main.qml`
- [ ] Layout.19.6 Test: CLO3D name has `_CLO3D`; DXF-ASTM name unchanged

## [ ] Task Layout.20 — Annotated DXF export name has `_annotated`

The annotated DXF export uses the same default name as the standard export. The user cannot tell the files apart. Uses the segment parameter from Layout.17.2. Do Layout.21 first or together, so new code uses "annotated".

- [ ] Layout.20.1 `Main.qml` `requestDxfExport`: ask Standard or Annotated before the save dialog. Today the save dialog comes first, so the name cannot know the choice
- [ ] Layout.20.2 Annotated puts `_annotated` before the timestamp. Example: `male_shirt_annotated_202610011721.dxf`
- [ ] Layout.20.3 The companion `.txt` keeps the `.dxf` base name: `male_shirt_annotated_202610011721.txt`
- [ ] Layout.20.4 With CLO3D (Layout.19): `male_shirt_CLO3D_annotated_202610011721.dxf`
- [ ] Layout.20.5 Standard name stays `<importedBaseName>_YYYYMMDDHHMM.dxf`
- [ ] Layout.20.6 Export all tabs: tab label still goes before the extension
- [ ] Layout.20.7 Update `export-docs/seamlylayout_EXPORT_WORKFLOW.md`, `dxf-docs/seamlylayout_DXF_EXPORT_WORKFLOW.md`, and the name comments in `Main.qml`
- [ ] Layout.20.8 Test: annotated name has `_annotated`; standard name unchanged; `.txt` matches the `.dxf`

## [ ] Task Layout.21 — Rename "teaching version" to "annotated version"

"Teaching" sounds amateur. "Annotated" is the professional term (user decision, 2026-10-03).

- [ ] Layout.21.1 UI text: `DxfTeachingDialog.qml` ("Generate a teaching version?", "Teaching Version" button, help text), `ViewDxfTeachingDialog.qml`
- [ ] Layout.21.2 Rename files and types: `DxfTeachingDialog.qml` → `DxfAnnotatedDialog.qml`, `ViewDxfTeachingDialog.qml` → `ViewDxfAnnotatedDialog.qml`; update `qt_frontend/CMakeLists.txt`
- [ ] Layout.21.3 QML: `Main.qml` ids, `pendingExportTeachingVersion`, `teaching` parameters, comments, log text
- [ ] Layout.21.4 C++: `PreferencesModel::dxfTeachingFilePath` → `dxfAnnotatedFilePath`; `PreferencesModelTests.cpp`
- [ ] Layout.21.5 Rust: `create_teaching_version` → `create_annotated_version` in `cxxqt_bridge/src/exports.rs`, `lib.rs`, `ezdxf2dxfastm/src/writer.rs`, `writer_test.rs`; JSON key `createTeachingVersion` → `createAnnotatedVersion` on both sides. The key is not saved to any file, so no migration
- [ ] Layout.21.6 `.txt` header comment: "DXF-ASTM Teaching Version" → "DXF-ASTM Annotated Version"; update the test that checks it
- [ ] Layout.21.7 Docs: `dxf-docs/*.md`, `status-docs/*.md`. Leave `TODO_COMPLETED.md` and `log.txt` as history
- [ ] Layout.21.8 Check: `grep -ri teaching` over `src/app/seamlylayout` (without `build/`, `target/`) returns nothing

## [x] Task Layout.8 — SeamlyLayout default paths don't resolve under %DATAROOT%

## [x] Task Layout.9 — Piece-mode handoff passes a file, not a stringified SVG document
