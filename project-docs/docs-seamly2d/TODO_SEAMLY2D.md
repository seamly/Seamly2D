# TODO — Seamly2D app features

Tasks that add features to the Seamly2D pattern-drafting app.

Tasks in this file begin with `Seamly2D.`

Piece marking behavior is described in [Piece markings](piece-markings.md).

## Required for SeamlyLayout release

### [ ] Task Seamly2D.72 — Implement - Pattern schema: every draft block has `<images>`

Upstream `85c36c0bcc` changed `VPatternConverter::fixGroupsOrder()`. It adds missing `<groups>` and `<images>` tags before validation. Schemas before v0.7.1 have no `<images>`. A pattern before v0.7.1 that has `<groups>` then fails validation and does not open. Example: `src/app/share/samples/patterns/jacket1_52-176.sm2d` (v0.6.8).

User decision: every draft block gets `<images>` in a new schema version. Seamly2D opens a pattern in its own schema version, then saves it in the new version.

- [x] Seamly2D.72.1 Find the newest upstream pattern schema. Use the next free version number (v0.7.8 if free). Our v0.7.6 and v0.7.7 differ from upstream.
- [x] Seamly2D.72.2 Add `src/libs/ifc/schema/pattern/v0.7.8.xsd`: a copy of v0.7.7, with `images` required in `draftBlock` (`minOccurs="1"`), after `groups`. Add it to `src/libs/ifc/schema.qrc`.
- [x] Seamly2D.72.3 `VPatternConverter`: update `PatternMaxVerStr`, `CurrentSchema`, `PatternMaxVer`, `getSchema()`, `ApplyPatches()`. Add `toVersion0_7_8()`: it adds the missing `<groups>` and `<images>` in each draft block, in schema order.
- [x] Seamly2D.72.4 `fixGroupsOrder()`: only move `<groups>` in front of `<images>`. It must not add tags before validation.
- [x] Seamly2D.72.5 Writer: each new draft block gets `<images>` (`VAbstractPattern::appendDraftBlock()`, `createDraftImages()`, `createGroups()`).
- [x] Seamly2D.72.6 Unit tests: all eight bundled patterns validate before conversion and convert to v0.7.8. Reordered/missing groups and images, empty documents, and new-block ordering are covered. `PatternConverterTests`: 9 passed.
- [x] Seamly2D.72.7 Integrated Windows suite passed `nmake check`, CTest 8/8, Cargo workspace tests, and MSI validation.
- [ ] Seamly2D.72.8 Verify (User): open `jacket1_52-176.sm2d`, save, reopen. The file has the new version, and each draft block has `<images>`.

### [ ] Task Seamly2D.11 — Implement - Piece mode: Fix Built-in seam allowance calculation, display, and export of built-in seam allowance

A piece with a built-in seam allowance erroneously:

1) displays only the main path (cutline) and hides the seamline,
2) creates the seamline on top of the cutline,
3) sends the seamline but not the cutline or foldline to SeamlyLayout and to piece exports (`VPiece::builtInSeamLinePoints()`).
4) Is calculated incorrectly - it enters into the seam allowance margin

Display, calculation, fold line, handoff, tests and schema are done: `TODO_COMPLETED.md`, "Completed subtasks from Task Seamly2D.11". Open:
- [x] Seamly2D.11.10 Fix - The built-in seamline crosses the seam allowance. For example in male_shirt.sm2d, piece 'CollarTopInterface-Built-in-SA', the seamline is incorrect at D11 and D12.
  - Evidence, `male_shirt_R13_202611012029.dxf` (mm): cut edge x=199.23, seam line x=189.23.
    - D12: seam vertex (197.06, 1251.52), spike to (189.23, 1249.46). Correct corner ~(189.23, 1249.77).
    - D11: spike from (189.23, 1195.39) to seam vertex (199.11, 1192.79). Correct corner ~(189.23, 1194.93).
  - Cause: the inward offset turns a short spline chord around at the 76 degree corner. `CheckLoops()` misses the spike: it only touches the front edge seam line.
  - Fix: `removeConsumedEdges()` in `vpiece.cpp` collapses each such edge to the crossing of its neighbour edge lines, before `Equidistant()`. Seam line only; the seam allowance is unchanged.
  - Test `TST_VPiece::BuiltInSeamLineMeetsAtAcuteCorner`. Harness: FAIL before the fix (2.17 mm from the cut line), 129/129 PASS after.
- [x] Seamly2D.11.8 Integrated Windows suite passed `nmake check`.
- [ ] Seamly2D.11.9 Verify (User): `AGENT_CHECKLIST.md` §27. Last result: FAIL (Seamly2D.11.10). Check again after the next MSI: no spike at D11 and D12.

### [ ] Task Seamly2D.71 - Implement - Piece Mode - Foldline processing, graphic and text

Currently, if foldline is enabled for a piece, the main path should end with the foldline and a graphic line and text should be created.

Fix this by reorganizing the main path nodes and creating the foldline's arrowedline and text label. There are four items related to foldline: foldline group, foldline path, foldline arrowline, and foldline text label -- the foldline group contains the foldline path, the foldline arrowline, and the foldine text label.

Rotation, arrow line, label, extended fold line, SeamlyLayout fold placement and nesting, tests and conversion are done: `TODO_COMPLETED.md`, "Completed subtasks from Task Seamly2D.71". Open:

- [x] Seamly2D.71.9 Integrated Windows suite passed `nmake check`, including piece, SVG component, and handoff tests.
- [ ] Seamly2D.71.10 Verify (User): `AGENT_CHECKLIST.md` §29.
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
- [x] Seamly2D.71.14 Fix - Layout Mode (`VLayoutPiece`) sends the seam line and the cut line without an edge on the fold line, as `HandoffPiece` does.
  - Done: `make_line_open_at_fold` is public (`handoff_piece.h`). `VLayoutPiece::setMainPathPoints()` and `setSeamAllowancePoints()` take the node vertices and the fold line and call it. Also the built-in seam allowance main path.
  - `isContourClosed()` and `isSeamAllowanceClosed()` give the result. `createMainPath()` and `createAllowancePath()` add no closing edge when the line has no fold line edge.
  - `unifyTurnPoints()`: a line that is closed in one size and has no fold line edge in another keeps the turn points of each size.
  - Tests: `TST_HandoffPiece::seamAndCutLinesMatchOldWriter` (4 rows, closed flags); `TST_HandoffDocument::pieceMatchesOldWriter` and `sheetMatchesOldScene` (new row "built in on the fold"; the fold rows now compare `seamline` and `cutline`); `TST_SvgComponentTags::FoldLinePieceSendsLinesWithoutFoldEdge`. Temporary test program (`TST_HandoffPiece`, `TST_HandoffDocument`, `TST_SvgComponentTags`): 25/25, 19/19, 51/51 PASS.
- [ ] Seamly2D.71.15 Verify (User): Layout Mode, piece with a fold line, sent to SeamlyLayout: the cut line and the seam line stop at both fold line ends; no line along the fold line. Also with a built-in seam allowance. `AGENT_CHECKLIST.md` §35.
- [ ] Seamly2D.71.12 Verify (User): Piece mode, piece with a fold line: the cut line and the seam line stop at both fold line ends; no line along the fold line. Also with a built-in seam allowance, and with no seam allowance. `AGENT_CHECKLIST.md` §34.

## Not required for SeamlyLayout release

### [ ] Task Seamly2D.9 — Pattern preferences should show the label template file name

`Pattern preferences > Label data > Label template:` has only the `Edit template` button (`pushButtonEditPatternLabel`, `src/app/seamly2d/dialogs/dialogpatternproperties.ui`). The user cannot see which template file the pattern label uses without opening the editor.

- [x] Display the label template filename beside `Edit template` in Piece Properties and Pattern Preferences.
  - [x] Resolve piece defaults through `NewPieceDefaults::pieceLabelTemplateFile(lines)`.
  - [x] Show the matching default or selected filename; show "none" when stored lines have no known file.
  - [x] Resolve and display the pattern-label template filename.
- [x] Refresh the filename after template editing.
- [x] Tooltip shows the full absolute path.
- [x] Selected/default/no-file tests pass; integrated Qt suites pass.
- [x] Schema conversion is unnecessary: template provenance is dialog state; the existing format stores label lines.
- [x] Verify (User)

### [ ] Task Seamly2D.50 - Implement - Piece Mode - Drill Holes

- [x] Implement Drill holes as a dedicated Piece Properties page, separate from notches.
  - [x] Resolve live drill centers in Piece Mode; require positive constant diameters and valid anchors.
  - [x] Export DXF R13 layer 13 with group 30 diameter.
  - [x] Model, editor, mirror/union/copy, real producer, DXF, inch-unit, and packing-invariance tests pass.
- [x] Schema v0.7.9 and conversion preserve markings. Integrated Qt and Rust suites pass.
- [x] Verify (User)

### Task Seamly2D.60 - Implement - Strip & plaid reference lines

- [x] Seamly2D.60 - Implement Stripe and plaid reference lines.
  - [x] Seamly2D.60.1 Live, distinct main-path point nodes define reference-only lines in Piece Mode.
  - [x] Seamly2D.60.2 DXF R13 writes stripe/plaid lines on layers 9/10 without changing packing bounds.
- [x] Real producer, transformation, DXF, and packing-invariance tests pass.
- [x] Schema v0.7.9 and conversion preserve reference lines.
- [x] Dialog regression verifies path-node choices, saved IDs, anchor independence, and edited endpoint geometry.
- [ ] Verify (User)

### [ ] Task Seamly2D.62 - Implement - Piece Mode - 'Flip Piece' tool

The 'Flip Piece' tool can be applied to any piece in Piece Mode, including pieces created by 'Union', 'Flip Piece', 'Copy Piece', and 'Mirror on Foldline' tools.

- [x] Piece Properties and the Piece Mode context menu select None, Horizontal, Vertical, or Both.
- [x] Follow the newer Layout.110 decision: mirror displayed geometry; labels and grainline retain their place and readability.
- [x] Apply Piece Mode flip and tilt to path-node markers and internal paths without changing stored geometry.
- [x] Do not flatten or rewrite stored geometry. The flip is a display of the stored choice.
- [x] Centered flip-transform tests and integrated Qt suites pass.
- [x] Existing flip schema and conversion remain in use; no additional schema change is needed.
- [ ] Test/Verify - check flipping:
  - [ ] Single piece - vertical & horizontal & both
  - [ ] Copied piece (requires Seamly2D.61) - vertical & horizontal & both
  - [ ] Flip a piece that was previously Flipped - vertical
  - [ ] Flip a piece that was previously Flipped - horizontal
  - [ ] Flip a piece that was previously Flipped - both

### [ ] Task Seamly2D.61 - Implement - Piece Mode - 'Copy Piece' tool

- [x] Reconstruct independent private modeling nodes and paths while retaining live draft links, including Union producers.
- [x] Add piece-name suffixes, including copies of copies.
- [x] Add piece-letter suffixes.
- [x] Reconstruct native modeling records rather than storing flattened rendering transforms.
- [x] Copies remain editable with Pattern Piece Tool; placement and one-step undo/redo are implemented.
- [x] Copy/recopy, source deletion, live draft updates, and marked-piece tests pass; integrated Qt suites pass.
- [x] Existing schema records represent independent copies; no conversion is needed.
- [ ] Verify - 'Copy' tool can be applied to any piece in Piece Mode, including pieces created by:
  - [x] Union
  - [ ] Flip Piece - Vertical
  - [ ] Flip Piece - Horizontal
  - [x] Copy Piece
  - [x] Mirror on Foldline

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
