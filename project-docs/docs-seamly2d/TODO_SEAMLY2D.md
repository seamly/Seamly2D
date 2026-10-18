# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

Tasks in this file begin with `Seamly2D.`

## [ ] Task Seamly2D.10 — Handoff: seam allowance width per notch

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

## [ ] Task Seamly2D.11 — Piece mode: draw the seam line of a built-in seam allowance

A piece with a built-in seam allowance shows only its main path, the cut line. Layout.28 sends the seam line inside it to SeamlyLayout and to piece exports (`VPiece::builtInSeamLinePoints()`). Piece mode does not draw it (user decision: add this task).

- [ ] Seamly2D.11.1 `PatternPieceTool::RefreshGeometry()`: draw `VPiece::builtInSeamLinePoints()` in the seam line style; the main path keeps the cut line style
- [ ] Seamly2D.11.2 A hidden seam line draws nothing, as for a piece without a built-in seam allowance
- [ ] Seamly2D.11.3 Piece dialog preview (`PatternPieceVisual`): the same rule
- [ ] Seamly2D.11.4 Seam line corners stay plain offset corners: node corner types shape only the cut line (user decision, Layout.28)
- [ ] Seamly2D.11.5 Tests and `local_build_msi.ps1` (`nmake check`)
- [ ] Seamly2D.11.6 User: piece mode shows the seam line W inside the main path; Hide seam line removes it

## [ ] Task Seamly2D.12 — Layout: SeamlyLayout opens behind Seamly2D

jacket2 (26 sizes, labels visible): Seamly2D did not respond for about 26 s, then SeamlyLayout opened behind Seamly2D. The user saw no window and pressed Layout again: two SeamlyLayout windows.

Found: piece labels cost 4 s → 20 s (installed 26.10.1615, pattern copies with and without labels). Stack samples: `VTextManager::FitFontSize` loads a new font engine for every pixel size it tries.

- [x] Seamly2D.12.1 Window in front: `WindowsForegroundWindow::allowForeground` after the start (`AllowSetForegroundWindow`); `seamlyLayout_main.mm` raises and activates its window before the import
- [x] Seamly2D.12.2 Progress: modal `QProgressDialog` per size, with Cancel; Cancel restores the size and the prior mode
- [x] Seamly2D.12.3 One SeamlyLayout per Seamly2D window: a second Layout says "already open" and brings SeamlyLayout to the front (Windows); Layout in Layout mode brings it to the front
- [x] Seamly2D.12.4 `waitForStarted()` failure shows a message, not nothing
- [x] Seamly2D.12.5 Label fit: `VTextManager::largestFittingSize` starts at the estimated size (jacket2 labels: 13 and 28 measurements → 2 and 3); `FitFontSize` keeps each result per thread
- [x] Seamly2D.12.6 Test `TST_VTextManager::LargestFittingSize`; `cl /Zs` on the changed sources
- [ ] Seamly2D.12.7 `local_build_msi.ps1` passes (`nmake check`)
- [ ] Seamly2D.12.8 User: jacket2 Layout shows the progress dialog and SeamlyLayout opens in front, well under 26 s; Cancel; second Layout; see `AGENT_CHECKLIST.md` section 14

## [ ] Task Seamly2D.9 — Pattern preferences shows the label template file name

`Pattern preferences > Label data > Label template:` has only the `Edit template` button (`pushButtonEditPatternLabel`, `src/app/seamly2d/dialogs/dialogpatternproperties.ui`). The user cannot see which template file the pattern label uses without opening the editor.

- [ ] Seamly2D.9.1 Add a label to the right of `Edit template` that shows `labelbasename.ext` (file name only, no directory)
- [ ] Seamly2D.9.2 Get the file from `NewPieceDefaults::patternLabelTemplateFile(lines)`, the same call `DialogPatternProperties` passes to `EditLabelTemplateDialog::setTemplateFile`
- [ ] Seamly2D.9.3 Show the selected file; else the Preferences default; else "none" (the pattern stores lines, not a file), to match the Edit Label Template dialog
- [ ] Seamly2D.9.4 Refresh the label after `Edit template` closes with new lines (`templateDataChanged`)
- [ ] Seamly2D.9.5 Tooltip shows the full absolute path
- [ ] Seamly2D.9.6 Unit test: file name shown for selected, default, and no-file cases

## [x] Task Seamly2D.4 — Preferences > Paths has no row for bodyscans

## [ ] Task Seamly2D.50 - Drill Holes

- [ ] Seamly2D.50 Implement Drill holes
  - [ ] Seamly2D.50.1 - Implement Drill Holes in Piece Mode, similar to Notches 
  - [ ] Seamly2D.50.2 - Implement Drill Holes in SeamlyLayout in exported DXF R12 and DXF R13 (layer 13), with diameter (group 30).

  ## Task Seamly2D.60 - 

  - [ ] Seamly2D.60 - Implement Stripe and plaid reference lines 
    - [ ] Seamly2D.60.1 - Implement Stripe and plaid reference lines in piece mode
    - [ ] ISeamly2D.60.2 - Implement Stripe and plaid reference lines in DXF R12 and DXF R13 exports(layers 9, 10) - not implemented in Seamly2D yet so move this to TODO_SEAMLY2D.md
