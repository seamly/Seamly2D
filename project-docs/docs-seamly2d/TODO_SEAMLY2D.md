# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

Tasks in this file begin with `Seamly2D.`

## [ ] Task Seamly2D.1 — Seamly2D: optional single-stroke (Hershey) label display and export

**Scope decision (2026-07-18):** outline fonts remain fully acceptable in Seamly2D — the existing outline-font canvas display and exports stay the default and are NOT removed or locked out. This task adds single-stroke (Hershey-style) label support as an *option*, so a designer targeting a cutter/plotter can preview labels on the canvas the way the machine will draw them and export matching single-stroke output.

Current label text handling (all outline-font based, and staying available): the canvas paints labels with the user-selected `QFont` via `painter->drawText()` (`VTextGraphicsItem::paint()`, `src/libs/vwidgets/vtextgraphicsitem.cpp`); the `textAsPaths == true` branch of `VLayoutPiece::createLabelItem()` (`src/libs/vlayout/vlayoutpiece.cpp`) outlines glyphs via `QPainterPath::addText()`; DXF text goes through `VDxfEngine::drawTextItem()` (`src/libs/vdxf/vdxfengine.cpp`).

**Constraint:** neither Windows nor Qt has native stroke-font support — the OS/Qt typography stack (`QFont`) only loads outline TTF/OTF, and a true single-stroke font cannot be installed as a system font (Windows errors on it or auto-closes the open contours into filled shapes). Two known workarounds exist in the wild:

1. **App-internal stroke-glyph data** (the true single-stroke route) — bypass the system font stack entirely and render strokes from Hershey glyph data inside the app, as Inkscape's *Hershey Text* extension does (Hershey Sans/Serif/Script 1-stroke, Gothic, and Duplex/Triplex multi-stroke variants). **This is the approach for this task** — the custom renderer below; prior art also includes Valentina's single-line/SVG-font label support.
2. **"Hairline" TTFs** — engineered fonts that trace each line forward and back over itself so the closed outline *looks* like a single stroke (e.g. CamBam Stick Fonts — free, 9 variants for CNC/plotting; MecSoft/Rhino single-stroke; commercial single-line TTF bundles). These install as normal Windows fonts and work in ordinary apps, but they are still outlines (doubled-back), stroke width isn't controllable via the font, and licensing must be checked before bundling. Relevant mainly as the embeddable-`<text>` option in Task 21 mode 2, not for this task's renderer.

- [ ] Seamly2D.1.1 Choose and bundle the stroke-glyph source (Hershey glyph data and/or single-line SVG fonts) under a GPL-compatible license; record the decision in the repo docs
- [ ] Seamly2D.1.2 Implement a single-stroke text renderer: shape a label line into stroked (fill-less) `QPainterPath` polylines with proper advance/kerning, honoring size, bold/italic variants (stroke width/slant), alignment, eliding, mirroring, and rotation
- [ ] Seamly2D.1.3 Add the option to the label settings UI: extend the label font selection (preferences Graphics View page / piece label settings) with the bundled single-stroke fonts alongside the existing system outline fonts; outline stays the default
- [ ] Seamly2D.1.4 Canvas: when a single-stroke font is selected, render labels with the stroke renderer in `VTextGraphicsItem` so the preview matches what a plotter will draw; outline-font labels keep the existing `painter->drawText()` path
- [ ] Seamly2D.1.5 Export: when a single-stroke font is selected, the `textAsPaths == true` branch of `createLabelItem()` emits single-stroke paths for those labels (outline-font labels keep the existing `QPainterPath::addText()` conversion); DXF export of single-stroke labels emits polylines
- [ ] Seamly2D.1.6 Coordinate with Task 10/Task 21: labels in a single-stroke font exporting as `<text>` should reference the bundled single-line font name so SeamlyLayout can match it
- [ ] Seamly2D.1.7 Verify: canvas single-stroke labels legible at typical zooms with correct placement, mirroring, and rotation; outline-font behavior unchanged everywhere; tagged pieces SVG / `.pieces.svg` / `--text2paths` / DXF / PDF / PNG correct in both font modes
- [ ] Seamly2D.1.8 Doxygen briefs + inline comments on all touched functions; document the font architecture in the repo docs

## [ ] Task Seamly2D.7 — Per-piece material

Today every piece is laid out on one fabric. Label placeholders `mFabric`, `mLining`, `mInterfacing`, `mInterlining` are fixed words, not piece data.

- [ ] Seamly2D.7.1 Piece data: material type (fabric, lining, interfacing, interlining, user-defined) and cut count per material
- [ ] Seamly2D.7.2 `.sm2d` schema: new attributes, schema version bump, converter for older files
- [ ] Seamly2D.7.3 Piece Properties dialog: material list editor
- [ ] Seamly2D.7.4 Label placeholders read the piece material data
- [ ] Seamly2D.7.5 Handoff SVG: `data-material` and per-material quantity on the piece `<g>`; update `SVG-DATA-ATTRIBUTES.md` and `NEW-ATTRIBUTES.csv`
- [ ] Seamly2D.7.6 Unit tests: schema round-trip, handoff attributes
- [ ] Seamly2D.7.7 SeamlyLayout side: Layout.15

## [ ] Task Seamly2D.8 — Fold lines (mirror lines)

A piece has an "on fold" flag (`VPieceLabelData::IsOnFold()`) but no fold line. `FoldPosition` is free text.

- [ ] Seamly2D.8.1 In Piece mode: implement a fold line
  - [ ] Seamly2D.8.1.1 Application Preferences - implement default Fold line properties in 
  - [ ] Seamly2D.8.1.2 UI - Implement Fold Line in the node context menu in the Piece Properties > Path. User selects two adjacent nodes along the path.
  - [ ] Seamly2D.8.1.3 Canvas - draw the fold line with its line style. No seam allowance (no cutline or seamline, it's a solid line) can be added to this line. The Line is displayed using the default Foldline properties in Application Preferences; "on fold" flag follows the fold line
- [ ] Seamly2D.8.2 Update Schema
  - [ ] Seamly2D.8.2.1 - Create a new `.sm2d` schema: fold line node ids, schema version bump
  - [ ] Seamly2D.8.2.2 - Create a converter for older files
- [ ] Seamly2D.8.3 In Piece mode implement a new 'Piece Mirror' tool - mirror the half piece across the fold line
- [ ] Seamly2D.8.4 Handoff SVG: `data-on-fold` on the piece `<g>`, a `fold_line` component group with the two points; update `SVG-DATA-ATTRIBUTES.md` and `NEW-ATTRIBUTES.csv`
- [ ] Seamly2D.8.5 Unit tests: schema round-trip, handoff attributes
- [ ] Seamly2D.8.6 SeamlyLayout side: Layout.16

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
