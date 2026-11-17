# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

Tasks in this file begin with `S2D.`

Piece marking behavior is described in [Piece markings](piece-markings.md).

## Required for SeamlyLayout release

### [ ] S2D.73 - Implement - Piece Mode - Export Pieces to an SVG file from Piece Mode

Layout mode sends the pieces SVG to SeamlyLayout as a string in memory (`generateHandoffSvgDocument()`). Export Pieces writes the same string to an SVG file that the user selects.
copy the .png file used for export drafts in Draft mode. 

- [x] S2D.73.1 Details menu: add `export_pieces_action` below Copy Piece (`duplicate_piece_action`).
  - Text "Export Pieces"; tooltip "Export Pieces (E, P)"; shortcut `E, P`. All three come from develop.
  - Icon `:/toolicon/32x32/export.png` (develop's icon; already in `toolicon.qrc`).
  - Enabled in Piece mode only, like Copy Piece.
  - Piece mode toolbox, Details page: `export_pieces_tool_button` below `copy_piece_tool_button` (y 154). Add 48 px to `details_Page` and `piece_ToolBox` heights. The button triggers `export_pieces_action`.

- [x] S2D.73.2 Build the string with the Layout mode steps: pieces with "in layout" set, `preparePiecesForLayout()`, `prepareMultisizePieceLists()`, `generateHandoffSvgDocument()`.
  - Move the steps into one function. Layout mode and Export Pieces call it. Do not copy the steps.
  - Same messages as Layout mode: no piece in layout, empty SVG, grainline and label warnings.
  - Cancel in the multisize dialog stops the export with no message.
  - The mode and the scene do not change.

- [x] S2D.73.3 File picker: `QFileDialog::getSaveFileName`, filter "SVG Files (*.svg)".
  - Default file name: `<patternbasename>_pieces_<YYMMDDHHMM>.svg`. `FileName()` gives the base name ("unnamed" for a new pattern). Time: `QDateTime::currentDateTime().toString("yyMMddHHmm")`.
  - Start folder: `VSettings::getLayoutPath()` (Preferences > Paths > Layout). User decision.
  - Write UTF-8. A write error shows a message box with the path.

- [x] S2D.73.4 Tests: default file name format; the file content equals the `generateHandoffSvgDocument()` string.
  - `TST_HandoffSvgFile`: 8/8 pass in a focused QtTest harness. Generated individual and multisize SVG files retain their exact UTF-8 bytes. `nmake check` runs with the next `local_build_msi.ps1`.
- [ ] S2D.73.5 Verify (User): Piece mode > Details > Export Pieces. SeamlyLayout opens the file and shows the same pieces as Layout mode. Also with a multisize pattern.

### [ ] S2D.75 Piece Mode - handoff flipped pieces to seamlylayout: when a piece is flipped in Piece Mode then the handover to seamlylayout should display the piece as flipped. Currently a flipped piece is displayed in the seamlylayout input canvas in it's original orientation -- this is incorrect, the flipped piece should retain it's flipped orientation.

- [x] Mirror the shared geometry for Export Pieces and the SVG handoff.
- [x] Keep Export Drafts as the construction scene. Apply flips only when an export contains pieces.
- [x] Verify every flip, two sizes, seam allowances, fold lines, paths, notches, markings, and grainlines.
- [ ] User: inspect the exported SVG and the SeamlyLayout input canvas.

### [ ] S2D.78 Command line - multisize files - currently there is no option to select the base size of a multisize file. the '--gsize' parameter allows selecting a numeric size. The command line parameter to select the base size could be '--basesize'. for a multisize file, either --gsize or --basesize is required -- both --gsize and --basesize cannot be used together.