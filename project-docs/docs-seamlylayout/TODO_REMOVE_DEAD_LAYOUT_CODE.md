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
- [ ] Dead.4.4 Write `layout_handoff`: `HandoffDocument`, `HandoffPiece`, `HandoffLabel`, `handoff_turn_points`, `handoff_ids`
  - Port `TST_SvgComponentTags`, `BuiltInPieceWithFoldLineSendsOpenSeamline` included
  - Compare with the old writer on the 8 sample patterns and the 13 CollectionTest patterns: same tagged tree, attributes, points (0.01 px after transforms) and label text
  - Switch `MainWindow` to the new writer only after a clean compare
  - New jacket2 handoff capture: `cargo test --workspace`, DXF-ASTM compare
- [ ] Dead.4.5 Delete the old writer (`VLayoutPiece`, `VLayoutPiecePath`, `SvgGenerator`, `SvgTextItem`, `VGraphicsFillItem`, `vlayoutdef.h`) and `src/libs/vlayout`; update `SVG-DATA-ATTRIBUTES.md`
