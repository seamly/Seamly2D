# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

Tasks in this file begin with `Seamly2D.`

## Required for SeamlyLayout release

### [ ] Task Seamly2D.10 — Handoff: seam allowance width per notch

SeamlyLayout measured the width from the cut and sew lines. Where the width changes at the notch node, it could get the narrower one; Seamly2D uses the wider one. A built-in seam allowance has no sew line, so nothing to measure.

- [x] Seamly2D.10.1 `VPiece::createNotchLines`: width per notch line, `MaxLocalSA` at the notch node; 0 without a seam allowance; built in or not
- [x] Seamly2D.10.2 `VLayoutPiece` keeps the widths; `createNotchesItem` sets `PieceItemData::SeamAllowances`; `SvgGenerator` writes `data-seam-allowances` (mm, 2 decimals, one per notch subpath)
- [x] Seamly2D.10.3 `SVG-DATA-ATTRIBUTES.md`, `NEW-ATTRIBUTES.csv`
- [x] Seamly2D.10.4 Tests: `TST_VPiece::NotchSeamAllowanceWidths`, `TST_SvgComponentTags::NotchGroupCarriesSeamAllowances`
- [x] Seamly2D.10.5 `local_build_msi.ps1` passes (`nmake check`)
- [ ] Seamly2D.10.6 User: 
  - [x] install the MSI; 
  - [x] Layout Mode handoff of a piece with a built-in seam allowance; 
  - [x] export DXF-ASTM; 
  - [ ] check slit depths with files generated and checked in SeamlyLayout: DXF.18

## Not required for SeamlyLayout release

### [ ] Task Seamly2D.11 — Piece mode: draw the seam line of a built-in seam allowance

A piece with a built-in seam allowance shows only its main path, the cut line. Layout.28 sends the seam line inside it to SeamlyLayout and to piece exports (`VPiece::builtInSeamLinePoints()`). Piece mode does not draw it (user decision: add this task).

- [ ] Seamly2D.11.1 `PatternPieceTool::RefreshGeometry()`: draw `VPiece::builtInSeamLinePoints()` in the seam line style; the main path keeps the cut line style
- [ ] Seamly2D.11.2 A hidden seam line draws nothing, as for a piece without a built-in seam allowance
- [ ] Seamly2D.11.3 Piece dialog preview (`PatternPieceVisual`): the same rule
- [ ] Seamly2D.11.4 Seam line corners stay plain offset corners: node corner types shape only the cut line (user decision, Layout.28)
- [ ] Seamly2D.11.5 Tests and `local_build_msi.ps1` (`nmake check`)
- [ ] Seamly2D.11.6 User: piece mode shows the seam line W inside the main path; Hide seam line removes it

### [ ] Task Seamly2D.9 — Pattern preferences should show the label template file name

`Pattern preferences > Label data > Label template:` has only the `Edit template` button (`pushButtonEditPatternLabel`, `src/app/seamly2d/dialogs/dialogpatternproperties.ui`). The user cannot see which template file the pattern label uses without opening the editor.

- [ ] Seamly2D.9.1 Add a label to the right of `Edit template` that shows `labelbasename.ext` (file name only, no directory)
- [ ] Seamly2D.9.2 Get the file from `NewPieceDefaults::patternLabelTemplateFile(lines)`, the same call `DialogPatternProperties` passes to `EditLabelTemplateDialog::setTemplateFile`
- [ ] Seamly2D.9.3 Show the selected file; else the Preferences default; else "none" (the pattern stores lines, not a file), to match the Edit Label Template dialog
- [ ] Seamly2D.9.4 Refresh the label after `Edit template` closes with new lines (`templateDataChanged`)
- [ ] Seamly2D.9.5 Tooltip shows the full absolute path
- [ ] Seamly2D.9.6 Unit test: file name shown for selected, default, and no-file cases

### [x] Task Seamly2D.4 — Preferences > Paths has no row for bodyscans

### [ ] Task Seamly2D.50 - Drill Holes

- [ ] Seamly2D.50 Implement Drill holes
  - [ ] Seamly2D.50.1 - Implement Drill Holes in Piece Mode, similar to Notches 
  - [ ] Seamly2D.50.2 - Implement Drill Holes in SeamlyLayout in exported DXF R12 and DXF R13 (layer 13), with diameter (group 30).

### Task Seamly2D.60 - Strip & plaid reference lines

  - [ ] Seamly2D.60 - Implement Stripe and plaid reference lines 
    - [ ] Seamly2D.60.1 - Implement Stripe and plaid reference lines in piece mode
    - [ ] ISeamly2D.60.2 - Implement Stripe and plaid reference lines in DXF R12 and DXF R13 exports(layers 9, 10) - not implemented in Seamly2D yet so move this to TODO_SEAMLY2D.md

### [ ] Task Seamly2D.61 - Piece Mode - 'Copy Piece' tool

- [ ] Copy an entire piece - resulting piece is a separate piece linked to draft objects (not linked to original piece).
- [ ] Update piece name with a suffix, e.g. Collar-1. If a copy of 'Collar-1' is made, then the updated piece name is 'Collar-1-1'
- [ ] Update piece letter with a suffix, e.g. A-2.
- [ ] The new piece should be flattened (except for text) to remove transform strings.
- [ ] The new piece can be edited with Pattern Piece Tool. 
- [ ] 'Copy' tool can be applied to any piece in Piece Mode, including pieces created by 'Union', 'Flip Piece - Vertical', 'Flip Piece - Horizontal', 'Copy Piece' tool, and 'Mirror on Foldline' and this tool.

### [ ] Task Seamly2D.62 - Piece Mode - 'Flip Piece' tool

- [ ] User selects radio button for flipping horizontally or vertically
- [ ] Flip the entire piece including labels, grainlines, etc.
- [ ] After flipping the entire piece then flip the labels, in place, in the same direction (vertical or horizontal) so that text isn't backwards or upside down.
- [ ] The new piece should be flattened (except for text) to remove transform strings.
- [ ] 'Flip Piece' tool can be applied to any piece in Piece Mode, including pieces created by 'Union', 'Flip Piece', 'Copy Piece', and 'Mirror on Foldline' tools.
- [ ] Test/Verify: check flipping on
  - [ ] single piece
  - [ ] Copied piece
  - [ ] Flipped piece vertical
  - [ ] Flipped piece horizontal
  - [ ] Mirrored on Foldline piece