# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

Tasks in this file begin with `Seamly2D.`

Piece marking behavior is described in [Piece markings](piece-markings.md).

## Required for SeamlyLayout release

### [ ] Task Seamly2D.73 - Implement - Piece Mode - Export Pieces to an SVG file

Layout mode sends the pieces SVG to SeamlyLayout as a string in memory (`generateHandoffSvgDocument()`). Export Pieces writes the same string to an SVG file that the user selects.

- [x] Seamly2D.73.1 Details menu: add `export_pieces_action` below Copy Piece (`duplicate_piece_action`).
  - Text "Export Pieces"; tooltip "Export Pieces (E, P)"; shortcut `E, P`. All three come from develop.
  - Icon `:/toolicon/32x32/export.png` (develop's icon; already in `toolicon.qrc`).
  - Enabled in Piece mode only, like Copy Piece.
  - Piece mode toolbox, Details page: `export_pieces_tool_button` below `copy_piece_tool_button` (y 154). Add 48 px to `details_Page` and `piece_ToolBox` heights. The button triggers `export_pieces_action`.
- [x] Seamly2D.73.2 Build the string with the Layout mode steps: pieces with "in layout" set, `preparePiecesForLayout()`, `prepareMultisizePieceLists()`, `generateHandoffSvgDocument()`.
  - Move the steps into one function. Layout mode and Export Pieces call it. Do not copy the steps.
  - Same messages as Layout mode: no piece in layout, empty SVG, grainline and label warnings.
  - Cancel in the multisize dialog stops the export with no message.
  - The mode and the scene do not change.
- [x] Seamly2D.73.3 File picker: `QFileDialog::getSaveFileName`, filter "SVG Files (*.svg)".
  - Default file name: `<patternbasename>_pieces_<YYMMDDHHMM>.svg`. `FileName()` gives the base name ("unnamed" for a new pattern). Time: `QDateTime::currentDateTime().toString("yyMMddHHmm")`.
  - Start folder: `VSettings::getLayoutPath()` (Preferences > Paths > Layout). User decision.
  - Write UTF-8. A write error shows a message box with the path.
- [x] Seamly2D.73.4 Tests: default file name format; the file content equals the `generateHandoffSvgDocument()` string.
  - `TST_HandoffSvgFile`: 7/7 pass in a scratch QtTest harness. `nmake check` runs with the next `local_build_msi.ps1`.
- [ ] Seamly2D.73.5 Verify (User): Piece mode > Details > Export Pieces. SeamlyLayout opens the file and shows the same pieces as Layout mode. Also with a multisize pattern.

### [ ] Task Seamly2D.200 - Implement - Preferences > Paths: remove the entry for the SeamlyLayout application

The Application Preferences > Paths section displays the path to SeamlyLayout. This display adds no value - the user should not be able to change it, and it provides no value for the user to view it here.

- [x] Seamly2D.200.1 - remove the SeamlyLayout path from displaying/editing in Application Preferences > Paths.
  - Done: `preferencespathpage.cpp` has 10 rows. `paths/seamlyLayoutApp` stays: the MSI seeds it; `Application2D::seamlyLayoutFilePath()` reads it first.
- [ ] Seamly2D.200.2 `local_build_msi.ps1` passes. Verify (User): Preferences > Paths has no SeamlyLayout row; Layout mode still starts SeamlyLayout.

### [ ] S2D.75 — Retain flipped piece geometry in SVG exports and handoff

- [x] Mirror the shared handoff geometry for Export Pieces and the SVG handoff.
- [x] Keep Export Drafts as the construction scene. Apply flips only when an export contains pieces.
- [x] Cover every flip, two sizes, seam allowances, fold lines, internal paths, notches, markings, and grainlines.
- [ ] User: inspect flipped pieces in the exported SVG and the SeamlyLayout input canvas.
