# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

Tasks in this file begin with `Seamly2D.`

Piece marking behavior is described in [Piece markings](piece-markings.md).

## Required for SeamlyLayout release

### [ ] Task Seamly2D.71 - Implement - Piece Mode - Foldline processing, graphic and text

Currently, if foldline is enabled for a piece, the main path should end with the foldline and a graphic line and text should be created.

Fix this by reorganizing the main path nodes and creating the foldline's arrowedline and text label. There are four items related to foldline: foldline group, foldline path, foldline arrowline, and foldline text label -- the foldline group contains the foldline path, the foldline arrowline, and the foldine text label.

Rotation, arrow line, label, extended fold line, SeamlyLayout fold placement and nesting, tests and conversion are done: `TODO_COMPLETED.md`, "Completed subtasks from Task Seamly2D.71". Open:

- [x] Seamly2D.71.9 Integrated Windows suite passed `nmake check`, including piece, SVG component, and handoff tests.
- [x] Seamly2D.71.11 Fix - A piece with a fold line: the cut line is an open path and does not run along the fold line.
  - Rule (user decision): the fold line begins at the cut line end and ends at the cut line begin. The cut line and the fold line make the outer edge.
  - A cut line that runs along the fold line means an improperly defined piece.
  - A piece without a fold line keeps a closed cut line. A mirrored piece has no fold line.
  - Done: `VPiece::openAtFoldLine()` (public static). Piece mode draws the cut line without its fold line edge (`open_at_fold_line_path`, `pattern_piece_tool.cpp`). `HandoffPiece` sends `cutline` the same way, also the built-in seam allowance main path (`built_in_cut_line()`).
  - Superseded by Seamly2D.71.14 (user decision): Layout Mode (`VLayoutPiece`) sends the cut line the same way.
  - Tests: `TST_VPiece` (open at the fold line), `TST_HandoffPiece`, `TST_HandoffDocument`. Temporary test program: 134/134, 20/20, 16/16 PASS. `pattern_piece_tool.cpp` builds.
- [x] Seamly2D.71.13 Fix - A piece with a fold line: the seam line does not run along the fold line either.
  - Rule (user decision): neither the seam line nor the cut line has an edge on the fold line. Each starts at the fold line end and ends at the fold line start.
  - Done: `make_line_open_at_fold` (`handoff_piece.cpp`) builds `seamline` and `cutline`. Piece mode draws the shown seam line the same way (`open_at_fold_line_path`). A hidden main path stays closed for the item shape.
  - SeamlyLayout: an outer `seamline` (no `cutline`) gets its fold line edge back for the DXF-ASTM boundary (`converter::extract_astm_piece`). A `seamline` beside a `cutline` stays as sent (layer 14). Packing and the piece divider already read it.
  - Superseded by Seamly2D.71.14 (user decision): `VLayoutPiece` sends the seam line the same way.
  - Tests: `TST_HandoffPiece::seamLineOpensAtFoldLine` (3 rows), `TST_HandoffDocument::foldPieceSendsOpenLines` (3 rows), `astm_piece_test` `open_seam_line_without_cut_line_closes_along_the_fold_line`, `piece_divider` `open_outline_on_the_fold_line_splits_and_stays_open` (cutline and seamline). Temporary test program: 134/134, 23/23, 17/17 PASS. `cargo test --workspace` and `ctest --preset debug` (7/7) pass.
- [x] Seamly2D.71.14 Fix - Layout Mode sends the seam line and the cut line without an edge on the fold line, as `HandoffPiece` does.
  - Done: `make_line_open_at_fold` is public (`handoff_piece.h`). `VLayoutPiece::setMainPathPoints()` and `setSeamAllowancePoints()` take the node vertices and the fold line and call it. Also the built-in seam allowance main path.
  - `isContourClosed()` and `isSeamAllowanceClosed()` give the result. `createMainPath()` and `createAllowancePath()` add no closing edge when the line has no fold line edge.
  - `unifyTurnPoints()`: a line that is closed in one size and has no fold line edge in another keeps the turn points of each size.
  - Tests: `TST_HandoffPiece::seamAndCutLinesMatchOldWriter` (4 rows, closed flags); `TST_HandoffDocument::pieceMatchesOldWriter` and `sheetMatchesOldScene` (new row "built in on the fold"; the fold rows now compare `seamline` and `cutline`); `TST_SvgComponentTags::FoldLinePieceSendsLinesWithoutFoldEdge`. Temporary test program (`TST_HandoffPiece`, `TST_HandoffDocument`, `TST_SvgComponentTags`): 25/25, 19/19, 51/51 PASS.
- [x] Seamly2D.71.15 Verify (User): Layout Mode, piece with a fold line, sent to SeamlyLayout: the cut line and the seam line stop at both fold line ends; no line along the fold line. 
  - [ ] check with a built-in seam allowance. `AGENT_CHECKLIST.md` §35.
- [x] Seamly2D.71.12 Verify (User): Piece mode, piece with a fold line: the cut line and the seam line stop at both fold line ends; no line along the fold line. Also with a built-in seam allowance, and with no seam allowance. `AGENT_CHECKLIST.md` §34.
- [ ] Seamly2D.71..16 For pieces with built-in SA's --> Verify that cutline and seamline begin and end at foldline (the don't include foldline)

## Not required for SeamlyLayout release

## Verify

### [ ] Task Seamly2D.10 — Verify - Handoff: seam allowance width per notch

SeamlyLayout measured the seaml allowance width between the cut and sew lines. Where the seam allowance width changes at a notch node, it could get the narrower one; Seamly2D uses the wider one. 

- [ ] Seamly2D.10.1 `VPiece::createNotchLines`: width per notch line, `MaxLocalSA` at the notch node; if seam allowance width = 0 then use notch default length; built in or not
- [x] Seamly2D.10.2 `VLayoutPiece` keeps the widths; `createNotchesItem` sets `PieceItemData::SeamAllowances`; `SvgGenerator` writes `data-seam-allowances` (mm, 2 decimals, one per notch subpath)
- [x] Seamly2D.10.3 `SVG-DATA-ATTRIBUTES.md`, `NEW-ATTRIBUTES.csv`
- [x] Seamly2D.10.4 Tests: `TST_VPiece::NotchSeamAllowanceWidths`, `TST_SvgComponentTags::NotchGroupCarriesSeamAllowances`
- [x] Seamly2D.10.5 `local_build_msi.ps1` passes (`nmake check`)
- [x] Seamly2D.10.6 Test/Verify - User:
  - [x] Create a Layout that contains a piece with a built-in seam allowance
    - [x] export DXF-ASTM
    - [ ] check slit depths with files generated and checked in SeamlyLayout: DXF.18
  - [ ] set default SA to 0
    - [ ] create a piece and add a notch, and display seam allowance
    - [ ] check that notch is correct
    - [ ] create a piece and add a notch, display seam allowance, and check 'built-in seam allowance'
    - [ ] check that notch is correct

### [ ] Task Seamly2D.200 - Implement - Preferences > Paths: remove the entry for the SeamlyLayout application

The Application Preferences > Paths section displays the path to SeamlyLayout. This display adds no value - the user should not be able to change it, and it provides no value for the user to view it here.

 [ ] Seamly2D.200.1 - remove the SeamlyLayout path from displaying/editing in Application Preferences > Paths.