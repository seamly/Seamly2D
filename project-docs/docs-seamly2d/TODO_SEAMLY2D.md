# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

Tasks in this file begin with `Seamly2D.`

## Required for SeamlyLayout release

### [ ] Task Seamly2D.11 — Implement - Piece mode: Fix Built-in seam allowance calculation, display, and export of built-in seam allowance

A piece with a built-in seam allowance erroneously:

1) displays only the main path (cutline) and hides the seamline,
2) creates the seamline on top of the cutline,
3) sends the seamline but not the cutline or foldline to SeamlyLayout and to piece exports (`VPiece::builtInSeamLinePoints()`).

Display, calculation, fold line, handoff, tests and schema are done: `TODO_COMPLETED.md`, "Completed subtasks from Task Seamly2D.11". Open:

- [ ] Seamly2D.11.8 Next MSI: Qt suites (`nmake check` in `local_build_msi.ps1`). A scratch harness passed every Seamly2D suite with the changed sources.
- [ ] Seamly2D.11.9 Verify (User): `AGENT_CHECKLIST.md` §27.

### [ ] Task Seamly2D.71 - Implement - Piece Mode - Foldline processing, graphic and text

Currently, if foldline is enabled for a piece, the main path should end with the foldline and a graphic line and text should be created.

Fix this by reorganizing the main path nodes and creating the foldline's arrowedline and text label. There are four items related to foldline: foldline group, foldline path, foldline arrowline, and foldline text label -- the foldline group contains the foldline path, the foldline arrowline, and the foldine text label.

- [ ] Rearrange the main path nodes - When the user selects two adjacent nodes in the main path, then clicks 'Apply' or 'OK', rotate any main path nodes from below the second foldline node to the top of the main path, maintaining node order, so that the main path ends with the two foldline nodes.
- [ ] Generate a foldline group with id="<piecename>_foldline" in the piece group that defines the foldline graphic line (simliar to grainline):
  - length is 50% the distance between the foldline nodes
  - parallel to the foldline nodes
  - centered vertically between the foldline nodes
  - the top and bottom 10% of the line bends 90 degrees to point towards the foldline
  - uses the Application Preferences > Foldline style
  - an example of a foldline graphic arrowline is here (not to scale, in red, etc.): test-seamly-layout\foldline_arrowline.png, test-seamly-layout\foldline_arrowline.svg
  - if a foldline is defined then the foldline graphic is visible; there is no checkbox in Piece Properties for foldline graphic visibility
- [ ] Generate a foldline label group, similar to pattern label and piece label, with id="<piecename_foldline_label>:
  - text is 'Foldline'
  - text is rotated 90 degrees
  - text is centered horizontally between the foldline and the <piecename>_foldline's arrowed line
  - text is centered vertically between the lowest y-value and highet y-value of the <piecename>_foldline's path
  - if a foldline is defined then the foldline label is visible; there is no checkbox in Piece Properties for foldline label visibility
- [ ] Do not generate a seam allowance between the foldline nodes, so that the cutline and seamline end at the first foldline node
- [ ] when a cutline is generated:
  - [ ] the foldline segment (not the foldline's arrowline) should be extended with two new nodes, one at each end of the foldline, to intersect the folline with the cutline
  - [ ] Export the two extended nodes as the foldline nodes piece export files.
  - [ ] Update SeamlyLayout layout processing to utilize the extended foldline to place patterns along the media left content rect edge when 'on fold' is checked.
- [ ] in SeamlyLayout, multisize, nested layouts --> nest pieces that have a foldline so that their first foldline nodes are aligned on top of each other; do not nest them using their boundary box center point.
- [ ] Create test
- [ ] Update Schema with conversion from previous version
- [ ] Verify (User)

## Not required for SeamlyLayout release

### [ ] Task Seamly2D.9 — Pattern preferences should show the label template file name

`Pattern preferences > Label data > Label template:` has only the `Edit template` button (`pushButtonEditPatternLabel`, `src/app/seamly2d/dialogs/dialogpatternproperties.ui`). The user cannot see which template file the pattern label uses without opening the editor.

- [ ] In Pattern Piece Tool > Properties > Labels > Piece Label tab, add text to the right of `Edit template` button that displays the label file name <labelbasename.ext>
  - [ ] Get the label file name from `NewPieceDefaults::pieceLabelTemplateFile(lines)`, the same call `DialogPatternProperties` passes to `EditLabelTemplateDialog::setTemplateFile`
  - [ ]If no file name then display the Preferences default; else "none" (the pattern stores lines, not a file), to match the Edit Label Template dialog
  - [ ] Repeat for Pattern Piece Tool > Properties > Labels > Pattern Label tab by getting the pattern label file name from NewPieceDefaults::patternLabelTemplateFile()
- [ ] Refresh the label after `Edit template` closes with new lines (`templateDataChanged`)
- [ ] Tooltip shows the full absolute path
- [ ] Unit test: file name shown for selected, default, and no-file cases
- [ ] Update Schema with conversion from previous version
- [ ] Verify (User)

### [ ] Task Seamly2D.50 - Implement - Piece Mode - Drill Holes

- [ ] Implement Drill holes -- not as a notch type, as a separate tab in Piece Properties
  - [ ] Implement Drill Holes in Piece Mode
  - [ ] SeamlyLayout - Implement Drill Holes in DXF R13 (layer 13), with diameter (group 30)
  - [ ] Create test
- [ ] Update Schema with conversion from previous version
- [ ] Verify (User)

### Task Seamly2D.60 - Implement - Strip & plaid reference lines

- [ ] Seamly2D.60 - Implement Stripe and plaid reference lines
  - [ ] Seamly2D.60.1 - Implement Stripe and plaid reference lines in piece mode
  - [ ] Seamly2D.60.2 - SeamlyLayout - Implement Stripe and plaid reference lines in DXF R13 export (layers 9, 10)
- [ ] Create test
- [ ] Update Schema with conversion from previous version
- [ ] Verify (User)

### [ ] Task Seamly2D.62 - Implement - Piece Mode - 'Flip Piece' tool

- [ ] User selects radio button for flipping horizontally or vertically
- [ ] Flip the entire piece including labels, grainlines, etc.
- [ ] After flipping the entire piece then flip the labels, in place, in the same direction (vertical or horizontal) so that text isn't backwards or upside down.
- [ ] The new piece should be flattened (except for text) to remove transform strings.
- [ ] 'Flip Piece' tool can be applied to any piece in Piece Mode, including pieces created by 'Union', 'Flip Piece', 'Copy Piece', and 'Mirror on Foldline' tools.
- [ ] Create test
- [ ] Update Schema with conversion from previous version
- [ ] Test/Verify - check flipping:
  - [ ] Single piece
  - [ ] Copied piece
  - [ ] Flipped piece vertical
  - [ ] Flipped piece horizontal

### [ ] Task Seamly2D.61 - Piece Mode - 'Copy Piece' tool

- [ ] Copy an entire piece - resulting piece is a separate piece linked to draft objects (not linked to original piece).
- [ ] Update piece name with a suffix, e.g. Collar-1. If a copy of 'Collar-1' is made, then the updated piece name is 'Collar-1-1'
- [ ] Update piece letter with a suffix, e.g. A-2.
- [ ] The new piece should be flattened (except for text) to remove transform strings.
- [ ] The new piece can be edited with Pattern Piece Tool
- [ ] Create test
- [ ] Update Schema with conversion from previous version
- [ ] Verify - 'Copy' tool can be applied to any piece in Piece Mode, including pieces created by:
  - [ ] Union
  - [ ] Flip Piece - Vertical
  - [ ] Flip Piece - Horizontal
  - [ ] Copy Piece
  - [ ] Mirror on Foldline

## Verify

### [ ] Task Seamly2D.10 — Verify - Handoff: seam allowance width per notch

SeamlyLayout measured the seaml allowance width between the cut and sew lines. Where the seam allowance width changes at a notch node, it could get the narrower one; Seamly2D uses the wider one. 

- [ ] Seamly2D.10.1 `VPiece::createNotchLines`: width per notch line, `MaxLocalSA` at the notch node; if seam allowance width = 0 then use notch default length; built in or not
- [x] Seamly2D.10.2 `VLayoutPiece` keeps the widths; `createNotchesItem` sets `PieceItemData::SeamAllowances`; `SvgGenerator` writes `data-seam-allowances` (mm, 2 decimals, one per notch subpath)
- [x] Seamly2D.10.3 `SVG-DATA-ATTRIBUTES.md`, `NEW-ATTRIBUTES.csv`
- [x] Seamly2D.10.4 Tests: `TST_VPiece::NotchSeamAllowanceWidths`, `TST_SvgComponentTags::NotchGroupCarriesSeamAllowances`
- [x] Seamly2D.10.5 `local_build_msi.ps1` passes (`nmake check`)
- [ ] Seamly2D.10.6 Test/Verify - User:
  - [x] install the MSI
  - [x] Create a Layout that contains a piece with a built-in seam allowance
  - [x] export DXF-ASTM
  - [ ] check slit depths with files generated and checked in SeamlyLayout: DXF.18

### [x] Task Seamly2D.4 — Verify - Preferences > Paths has no row for bodyscans
