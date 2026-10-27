# TODO — Remove unused (dead) layout code

Tasks for removing unsued code due to implementing SeamlyLayout, and orphaning the old layout code.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Check off all completed tasks & subtasks and move completed tasks to TODO_COMPLETED.md

All TODO_MIGRATE.md tasks begin with 'Dead.' and all tasks are numbered.

## Task Dead.2 - Test the build pipeline -- ci.yml

## Task Dead.3 - Test the functionality (requires install & user testing)

## Task Dead.4 - Replace `src/libs/vlayout`

SeamlyLayout does the layout, print and export work that Seamly2D did. Seamly2D keeps one job: write the Layout Mode handoff SVG.

Decisions (user, 2026-10-24):

- Delete the nesting engine, print tiling, the layout UI, Export Pieces, File > Export As, the CLI export mode, `vdxf` and `vobj`.
- Keep Export Draft Blocks. It uses no vlayout code. SeamlyLayout has no draft-block export.
- SeamlyLayout has no Print command. Users print its PDF or PS export.
- Rewrite the handoff writer in a new library, `src/libs/layout_handoff/`, namespace `layout_handoff`.
  - No file, class or function name starts with `v`.
  - New code follows `.github/README-CODE-STYLES.md`.
  - It keeps every rule in `project-docs/docs-data/SVG-DATA-ATTRIBUTES.md`, the open `seamline` at a fold line included.
- Move `VAbstractPiece` and `VTextManager` to `vpatterndb`. Rename them `AbstractPiece` and `LabelTextManager`, and restyle them. Piece mode draws seam allowances and labels with them.

Steps, in order:

- [ ] Dead.4.1 Delete the nesting engine and print tiling
  - [x] Dead.4.1.1 CLI export mode: `--basename` and the export and layout options. Keep `--test`, `--mfile`, `--gsize`, `--gheight`, `--no-scaling`
    - The export switch was `-b`/`--basename`. `-e` is `--gheight` and stays
    - Also removed: `MainWindow::DoExport`, `VDomDocument::UnitsHelpString`, the export dialog setters only the console used; `CLI_PARAMETERS.md` rewritten
  - [x] Dead.4.1.2 Print, Print Tiled, Print Preview, Print Preview Tiled; tiled PDF export and its settings; `VPoster`; `TST_VPoster`
    - Also removed: `PreparePrinter`, `PageOrientation`, the scissors icons. `LayoutExportFormat::PDFTiled` stays, unused, so later format numbers stay the same
    - Test: `TST_ExportFormatCombobox::offersNoTiledPdf`
  - [x] Dead.4.1.3 New Print Layout, Layout Pages dock, Layout toolbar, Layout toolbox, File > Export As; `LayoutSettingsDialog`, `DialogLayoutProgress`; layout settings and the Preferences "Layout toolbar" check box
    - Also removed: `AbstractLayoutDialog`, `PageFormatCombobox`, `PaperSizeFormat`, `GetPrinterFields`, `GetMinPrinterFields`, the layout icons. `ExportLayoutDialog` is a plain `QDialog`
    - Fix: leaving Layout mode returns to the draft toolbox page the user left (it read the removed layout toolbox)
  - [x] Dead.4.1.4 Delete `VLayoutGenerator`, `VLayoutPaper`, `VBank`, `VContour`, `VBestSquare`, `VPosition`; the `LayoutErrors` and `BestFrom` enums
  - [ ] Dead.4.1.5 MSI build and user check, `AGENT_CHECKLIST.md` §28
- [ ] Dead.4.2 Delete Export Pieces, `src/libs/vdxf`, `src/libs/vobj`
  - [x] Dead.4.2.1 Export Pieces: toolbox button, Details menu and toolbar action, `MainWindow::exportPiecesAs`, the piece export code of `MainWindowsNoGUI`, `ExportProgressDialog`
  - [x] Dead.4.2.2 Delete `src/libs/vdxf` (with `libdxfrw`) and `src/libs/vobj`; drop them from `libs.pro`, `seamly2d.pro`, `translations.pro`, `Doxyfile`
  - [x] Dead.4.2.3 Trim the export dialog to Export Draft Blocks: SVG, PDF, PNG, JPG, BMP, TIF, PPM; no binary DXF, text as paths or size list. Test: `TST_ExportFormatCombobox::offersNoDxfOrObj`
  - [ ] Dead.4.2.4 MSI build and user check, `AGENT_CHECKLIST.md` §28 (with Dead.4.1.5)
- [ ] Dead.4.3 Move `VAbstractPiece` → `AbstractPiece` and `VTextManager` → `LabelTextManager` into `vpatterndb`; restyle; port their tests
  - Decisions (user): keep the method names; `VSAPoint` → `SeamAllowancePoint`; one exit point per function; no namespace
  - [x] Dead.4.3.1 Move: `vpatterndb/AbstractPiece.{h,cpp}`, `AbstractPieceData.h`, `SeamAllowancePoint.h`, `LabelTextManager.{h,cpp}`. vpatterndb uses no vlayout header; it adds QtXml
  - [x] Dead.4.3.2 Restyle: briefs, snake_case, one exit point. `LabelTextManager::min_font_size` replaces `MIN_FONT_SIZE`; `TextLine` members are `text`, `font_size_increment`, `bold`, `italic`, `alignment`
  - [x] Dead.4.3.3 Link vlayout before vpatterndb in `seamly2d.pro`, `seamlyme.pro`, `Seamly2DTest.pro`, `CollectionTest.pro`
  - [x] Dead.4.3.4 Tests: `TST_AbstractPiece` (new case `SeamAllowancePointWidths`), `TST_LabelTextManager`
  - [ ] Dead.4.3.5 MSI build, Qt suites and user check, `AGENT_CHECKLIST.md` §30
- [ ] Dead.4.4 Write `layout_handoff`: `HandoffDocument`, `HandoffPiece`, `HandoffLabel`, `handoff_turn_points`, `handoff_ids`
  - [x] Dead.4.4.1 New static lib `src/libs/layout_handoff/` (namespace `layout_handoff`; linked after `vlayout`, before `vpatterndb`). `handoff_ids` (`HandoffIds`, `sanitize_for_id`, `size_id_suffix`) and `handoff_turn_points` (`unify_turn_points`, `SizeLine`) moved out of `SvgGenerator` and `VLayoutPiece`; the old writer now calls them. Tests: `TST_HandoffIds`, `TST_HandoffTurnPoints`
  - [x] Dead.4.4.2 `HandoffLabel`, `HandoffPiece` (replaces `VLayoutPiece::Create`), `HandoffDocument` (replaces `SvgGenerator` and the Qt scene rendering)
    - [x] `HandoffPiece::create` geometry: seam line, cut line, built-in seam line, notches, internal and cutout paths (plain structs `HandoffLine`, `HandoffPath`, `HandoffNotches`). `AbstractPiece::RemoveDublicates` is now public. Test `TST_HandoffPiece` compares with `VLayoutPiece::Create`; internal and cutout paths are not yet in the test
    - [x] `HandoffPiece` grainline (path, grain axis, visible flag; a hidden grainline keeps its data), `is_on_fold`, fold line (plain, sent out to the cut line, `FoldLineGraphic`). Style (colors, `showGrainlines`, `showSeamAllowances`) stays with `HandoffDocument`. Tests added to `TST_HandoffPiece` (13 pass in the scratch harness): internal and cutout paths, grainline (3 arrow types, one outside the piece), hidden grainline, fold line, no fold line
    - [x] `HandoffLabel` (`create_piece_label`, `create_pattern_label`): shape (rotated, shifted into `HandoffPiece::bounds()`), fitted font, text lines, visible flag; a hidden label keeps its data, and `HandoffDocument` applies `showLabels`. `keep_inside` and `outline_bounds` moved to `handoff_geometry`. New test `TST_HandoffLabel` (11 pass in the scratch harness; compares text and first corner with `VLayoutPiece::Create`). Line placement (elide, alignment, mirror) is not ported yet: it goes to `HandoffDocument`
    - [x] `HandoffDocument` (`handoff_document.h/.cpp`): builds the tagged SVG as a DOM from `HandoffStyle` (from settings), `HandoffPieceInfo` and `HandoffPlacedPiece` (piece, labels, offset, mirror). Writes the fold line parts, label line placement (elide, alignment, mirror), one group per material, and the multisize groups. Hidden labels and grainlines, and items turned off by settings, are not drawn (as in the old writer); their data stays. New test `TST_HandoffDocument` (9 pass in the scratch harness): same tagged tree, data-* attributes, styles and points (0.02 px) as `SvgGenerator` for plain, fold line, built-in and no-allowance pieces, and a multisize set. `MainWindow` is not switched (Dead.4.4.3)
  - [ ] Dead.4.4.3 Compare, switch `MainWindow`, jacket2 capture (the items below)
    - [x] Gap 1: `HandoffPiece::seam_allowance_corners()`, `line_node_counts()`, static `unify_piece_turn_points()` (lines and paths). Tests in `TST_HandoffPiece` (16 pass in the scratch harness): corners and node counts equal `VLayoutPiece`; path turn points equal `VLayoutPiece::unifyTurnPoints` over three sizes and over sizes with different node counts; seam and cut lines equal `unify_turn_points`. The old class gives no getter for line turn points, so those two lines are compared with the pure function. Sizes here share turn points already, so the tangent-differs case is covered by `TST_HandoffTurnPoints` only.
    - [x] Gap 3: new `handoff_sheet.h/.cpp` (`piece_extent`, `arrange_flat`, `FlatSheet`) and `stroked_bounds` in `handoff_geometry`. Test `TST_HandoffDocument::sheetMatchesOldScene` (4 rows: cut line, fold, built in, no allowance; two pieces at fractional Mx/My) equals the `itemsBoundingRect()` sheet and the offsets. Label text is not in the extent (labels lie inside the outline); a label outside the outline would differ from the old sheet.
    - [x] Gap 2: `MainWindowsNoGUI` fills `handoff_pieces` (`HandoffPlacedPiece`) next to the `VLayoutPiece` lists; corner cuts and turn points read the new lists. `generateHandoffSvgDocument()` builds the new SVG. Compare: set `SEAMLY_HANDOFF_COMPARE_DIR=<absolute directory>`, enter Layout Mode, diff `<pattern>_old.svg` and `<pattern>_new.svg`. Task Layout.106 in `TODO_SEAMLYLAYOUT.md`; 106.9 (MSI, user test) is open.
    - Open: the `MainWindow` switch.
    - Finding: the sample-pattern compare needs a loaded `VPattern` with tools. No scratch harness loads one; it needs a built Seamly2D (MSI) or a harness that links the app objects. Not run.
  - Port `TST_SvgComponentTags`, `BuiltInPieceWithFoldLineSendsOpenSeamline` and `FoldLineGroupHoldsArrowAndLabel` included
  - Port the `fold_line` group: fold line path out to the cut line, then the `fold_line_arrow` and `fold_line_label` parts (`VLayoutPiece::createFoldLineItem()`, nested parts in `SvgGenerator::addComponentGroups()`)
  - Compare with the old writer on the 8 sample patterns and the 13 CollectionTest patterns: same tagged tree, attributes, points (0.01 px after transforms) and label text
  - Switch `MainWindow` to the new writer only after a clean compare
  - New jacket2 handoff capture: `cargo test --workspace`, DXF-ASTM compare
- [ ] Dead.4.5 Delete the old writer (`VLayoutPiece`, `VLayoutPiecePath`, `SvgGenerator`, `SvgTextItem`, `VGraphicsFillItem`, `vlayoutdef.h`) and `src/libs/vlayout`; update `SVG-DATA-ATTRIBUTES.md`. Remove the compare switch too: Task Layout.107 in `TODO_SEAMLYLAYOUT.md`.
