# TODO — SeamlyLayout app features

Open SeamlyLayout tasks. Check off finished items; move fully completed tasks to `project-docs/TODO_COMPLETED.md`. Task IDs start with `Layout.`

## [ ] Task Layout.3 — Test multisize layouts

A `.smms` pattern sends all sizes in one SVG. Nested packs size groups; Marker packs individual pieces.

Rules (2026-09-28):

- Include all `.smms` sizes at base height only.
- Align grainlines vertically; center all sizes on the largest size.
- Keep `.smis` handoff unchanged.

Handoff shape: see `project-docs/docs-data/SVG-DATA-ATTRIBUTES.md`, "Multisize pattern".

- [ ] Layout.38.6 Next MSI: test a multisize pattern from Seamly2D Layout Mode in SeamlyLayout Nested and Marker. Add to `TEST_CHECKLIST.md`.

## [ ] Task Layout.6 — Verify DXF-ASTM grading

See `dxf-docs/seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`, "Not exported".

- [ ] Layout.61 Verify grade IDs (`# n`), size blocks (`Size Name:`), layer 5, and `.rul` output.
  - [ ] Layout.61.7 Validate the multisize DXF-ASTM export with aw.fyi (DXF.8.4).
    - [ ] Layout.61.7.2 On MSI 26.10.1220+, export jacket2 Nested as DXF-ASTM R13; check in aw.fyi. Expect no merged Detail pieces, layer 5 mismatches, or duplicate vertices. Detail (3), (6), (7), (8) remain sample-size only until Layout.27.3.

    | aw.fyi finding | Report | Cause | Task |
    |---|---|---|---|
    | Departure "Sample size points incomplete" | both | 6 pieces share `Piece Name:Detail`; aw.fyi reads their blocks as one graded piece | Layout.26.5, done after these files |
    | Departure "Excluded layers present in graded sizes" | both | Same merge | Layout.26.5 |
    | "L5 endpoint mismatch up to 103.74; 4 blocks without L5" | Nested | Same merge. The 4 blocks are the ungraded Detail sample sizes | Layout.26.5 |
    | Duplicate vertices, layers 84 (54) and 87 (450) | Nested | Graded key points 0.000001–0.01 mm from a dense vertex | Layout.27.9, done after these files |
    | Departure "Corners missing from layer 2": Detail (4) size 240, L1 pt 14 | Marker | 6.5° bend over 10 mm; no tangent break | Reader advisory (DXF.23) |
    | Advisory: no densified points, 13 pieces | Marker | Rectangles (Комір) and triangles (Detail (5)): straight sides | No action |
    | Advisory: more curve points than the curves need (piece-113 L14, Detail (4) size 500 L1) | Nested | Long gentle curves | No action (advisory) |
    | Advisory: validation spans, curve tolerance | both | Pass | — |
    | No finding: `Piece Name:piece-201_s720_M` | both | Cyrillic name emptied by `sanitize_ascii`; aw.fyi cannot group the sizes | Layout.26.4, done after these files |


## [ ] Task Layout.67 — Test multisize tabs and export messages

Decision (2026-10-13): all tabs use the imported SVG, not grade rules. DXF-ASTM exports one size per file. Supersedes Layout.22, Layout.23, Layout.27.10, and Layout.61.7.3.

| Tab | Packs | DXF-ASTM | Other exports |
|---|---|---|---|
| Marker | every size of every piece alone | none | yes |
| Nested | each piece once, its sizes stacked | none | yes |
| Base Size N | the base size | `# n` texts, layer 5, `.rul`, `Sample Size:N` | yes; Export all tabs skips it |
| Size N | one size | no `.rul`, blank `Sample Size:` | yes |

- [ ] Layout.67.8 Next MSI: run `TEST_CHECKLIST.md` §7, "Multisize tabs".
- [x] Layout.67.9 Base Size "Export Complete" must name both `.dxf` and `.rul` files (MSI 26.10.1311, `jacket2_basesize_screenshot_1.png`). Signal `export_companion_written`. User check: Layout.67.8.

## [ ] Task Layout.24 — Put the tab name before the export timestamp - Priority 2

Default name of a multisize export: `<base>[_<segment>]_<tab>_<stamp>[_tiled].<ext>`. Example: `mypatternname_R13_Size220_202610112043.dxf`. An individual pattern keeps `<base>[_<segment>]_<stamp>[_tiled].<ext>`.

- [x] Layout.24.1 Add optional tab segment before `<stamp>` in `exports::default_export_file_name`.
- [x] Layout.24.2 Remove spaces from tab labels; replace non-alphanumeric characters (except `-`) with `-`. "Size 220" → `Size220`.
- [x] Layout.24.3 Include the selected tab in single-export default names.
- [x] Layout.24.4 Apply the rule to Export All. Replace `layout_views::tab_file_path` (currently puts `_size-220` after timestamp). For renamed files, insert `_<tab>` before the extension.
- [x] Layout.24.5 Test names with/without tab, tiled names, `/` in labels, and Export All.
- [x] Layout.24.6 Docs: export file name rule in the SeamlyLayout docs, `TEST_CHECKLIST.md`
- [ ] Layout.24.7 Next MSI: run `TEST_CHECKLIST.md` §7, "Export file names (Layout.24)".

## [ ] Task Layout.25 — Verify automatic media length

All multisize tabs (Marker, Nested, individual sizes) extend the media length to fit their pieces (decision D3).

Previous failure (jacket2 Marker, 48 × 500 in roll, MSI 26.10.1311): 96/208 placed; all pieces need about 740 in. Evidence: `jacket2_marker_screenshot_1.png`, `jacket2_screenshot_1.png`, `jacket2_40-146_202610131245.svg`.

Decision (2026-10-13): extend rolls beyond 500 in when needed.

- [ ] Layout.25.7 Next MSI: run `TEST_CHECKLIST.md` §7, "Media grows to fit".

## [ ] Task Layout.71 — Verify automatic media width

Multisize layouts expand right for wide pieces and down for long pieces. Only individual patterns split pieces (Layout.72).

- [ ] Layout.71.6 Next MSI: jacket2, 10-in Paper Roll. All tabs should widen without warnings; Adjust Mode and SVG export should match.

## [ ] Task Layout.72 — Test handling oversized individual pieces

For oversized individual-pattern pieces, ask whether to split them or enlarge the media (decision 2026-10-13).

- Dialog: list pieces and offending dimensions; offer "Split the pieces" (SVG, PDF, HPGL, EPS, PS, PNG, JPG) or "Make the media bigger" (DXF). OK continues; Cancel leaves only Layout Settings active.
- Split only if no allowed rotation fits: too wide → vertical cuts; too long on non-roll media → horizontal cuts. Paper roll and fabric are roll-form. No dialog for tiled paper.
- Enlarge media using Layout.71 (width) and Layout.25 (length).
- Split into the minimum number of equal parts (rounded up), without cut-edge seam allowance. Label each part; add 2 match notches per cut edge. Apply the same split to sheet PDF. Warn if a fitting piece is left out because the media is full.

- [ ] Layout.72.8 Next MSI: trousers sample, Paper Sheet 36 × 36 in, Create Layout:
  - [ ] Dialog lists four too-long legs: Back, Front_Pleats, Front_no_pocket, Front_with_pocket.
  - [ ] Cancel: only Layout Settings, Preferences, Exit enabled; submitting settings re-enables Create Layout.
  - [ ] Split: each leg becomes 2 parts, blue "<name> part i of 2, joins j" label, and 2 green match notches. Warn about unplaced pieces.
  - [ ] Enlarge: sheet exceeds 36 in length; no splits or warnings.
  - [ ] Export split PDF parts and DXF-ASTM blocks named `Back (1/2)` and `Back (2/2)`.

## [ ] Task Layout.26 — Verify Missing Piece Data dialog

jacket2 hides piece labels and grainlines (`visible="false"`), so missing-data warnings are expected. Old MSI samples used Cyrillic names (Layout.40). Evidence: `jacket2_marker_screenshot_2.png`, `jacket2_nested_screenshot_1.png`.

- [ ] Layout.26.7 Next MSI: jacket2 Marker; check that the dialog scrolls and buttons remain visible.

## [ ] Task Layout.27 — Fix jacket2 DXF-ASTM grading

Prior results (jacket2, sizes 220–720): Nested graded 4 of 8 pieces; Marker placed 96/208 pieces and lost grading on others. Base Size 500 in MSI 26.10.1311 graded 4 pieces, with rules accurate to 0.011 mm. Detail (3), (6), (7), (8) remained ungraded. Evidence: `test-seamly-layout/layouts/jacket2/`, aw.fyi reports, `jacket2_40-146_basesize_R13_202610131248.dxf`.

- [x] Layout.27.1 Re-export after Layout.25; confirm sample size 500 is no longer omitted from Marker. Rust test `jacket2_marker_extends_paper_roll`: 208 of 208 placed on a 48 × 500 in roll. Marker has no DXF-ASTM export (Layout.67). User check: Layout.25.7.
- [x] Layout.27.2 If a sample-size piece is unplaced, warn: "size 500 does not fit the media". `grade_drawing` names it.
- [ ] Layout.27.3 Make turn points consistent across all sizes of each piece (cutline, seamline, internal paths). Seamly2D `VAbstractPiece::turnPointIndices` currently changes at near-threshold corners. After `prepareMultisizePieceLists`, share turn points across sizes using node vertex indices in `VLayoutPiece` / `VLayoutPiecePath`; skip paths whose node counts differ.
  - [x] Layout.27.3.1 Code: `unifyTurnPoints` (`63ab2447a3`).
  - [ ] Layout.27.3.2 Run the Qt suites with `local_build_msi.ps1`.
  - [ ] Layout.27.3.3 Next MSI: export jacket2 Base Size 500 to DXF-ASTM. All 8 pieces should grade, with no "other turn points" warning.
  - Note: `jacket2_40-146_202610121819.svg` has inconsistent `data-turn-points` counts for Detail (3) and (6); Detail (7) and (8) have four per line in every size. See Layout.27.12.
- [x] Layout.27.4 Give `grade_anchors` a distinct error when a key point is missing from the dense line. `GradeError::KeyPointOffLine`.
- [x] Layout.27.5 Name the first size whose turn points differ from the sample size. jacket2 fixture: Detail (3) size 660, Detail (6) size 220, both on the cut line.
- [ ] Layout.27.7 Add jacket2 handoff fixture to `crates/cxxqt_bridge/test_data/`; end-to-end test should grade every piece and write `.rul`.
- [x] Layout.27.8 If nothing grades, show "No piece graded: no .rul file written". `astm_grading::grade_drawing` writes `.rul` only when at least one piece grades.
- [x] Layout.27.11 Fix near-duplicate written vertices. Example: Подборт layer 87 has `(333.01, 626.21)` followed by `(333.01, 626.22)`. Increase `SNAP_VERTEX_MM` from 0.01 to 0.02, or compare rounded coordinates. Test: no consecutive written vertices 0.01 mm apart or closer. Done: `DUPLICATE_TOLERANCE_MM` and `SNAP_VERTEX_MM` = 0.025; jacket2 test found 6 before the fix.
- [ ] Layout.27.12 Make line-end turn points consistent across sizes. Detail (7) differs from size 660; Detail (8) differs at 520–640, 680 and 700. `astm_contour::straight_run_ends` flips at its 10-mm / 2× break thresholds (e.g., 9.97 mm at size 640 vs 10.2 mm at 660). Decision (2026-10-13): Option 1.
  - Option 1 (chosen): Seamly2D checks node line ends, then `unifyTurnPoints` shares flags across sizes. SeamlyLayout stops adding line ends to tagged paths. Refresh old fixtures.
  - Option 2: SeamlyLayout shares line-end flags across sizes, using new node-index attributes from Seamly2D.
  - [x] Layout.27.12.1 Seamly2D: `turnPointIndices` marks line ends; `unifyTurnPoints` shares them (`5bcb1f3cf1`).
  - [x] Layout.27.12.2 SeamlyLayout: no line ends added to tagged input (`5bcb1f3cf1`).
  - [ ] Layout.27.12.3 Refresh fixtures from the next MSI handoff: `male_shirt_pieces.svg` (expect 16 FrontPanel turn points, not 14); jacket2 (expect all 8 pieces graded).
  - [ ] Layout.27.12.4 Build and user check: Layout.27.3.2, Layout.27.3.3.

## [ ] Task Layout.28 — Verify built-in seam line

Seamly2D now sends both cut and seam lines for built-in seam allowances. SeamlyLayout draws and exports both. Only app verification remains.

Status (2026-10-12): code, docs, Rust (472 pass) and Qt tests pass; MSI 26.10.1220 built. Changes are local to `run-seamlyLayout` (not pushed).

- [ ] Layout.28.7 Next MSI: run `TEST_CHECKLIST.md` §5, "Piece with a built-in seam allowance".

## [ ] Task Layout.68 — Verify PDF export scale

Cause (MSI 26.10.1311): PDF used 72 dpi instead of SVG’s 96 dpi, scaling pages and pieces by 4/3.

- Marker `jacket2_40-146_202610131244.pdf` (48 in × 500 in roll): MediaBox `4608 × 48000` pt = 64 in × 666.7 in. Expected `3456 × 36000` pt.
- Nested `jacket2_40-nested_146_202610131245.pdf`: `4608 × 5003` pt. Expected `3456 × 3752` pt.
- EPS and PS of the same tabs are correct: `0.75` scale, BoundingBox `3456 36000`.

- [ ] Layout.68.5 Next MSI: print a tiled page at 100% and measure it. Acrobat should report the Marker PDF as 48 × 500 in.

## [ ] Task Layout.69 — Add PNG/JPG resolution metadata - Priority 1

MSI 26.10.1311 exports correct pixels at 96 dpi (jacket2 PNG: 4608 × 48000) but omits physical resolution metadata. Printing at actual size may be wrong.

- [x] Layout.69.1 Write PNG `pHYs` = 3780 px/m and JPG JFIF density = 96 × 96 dpi.
- [x] Layout.69.2 Rust tests verify PNG `pHYs` and JPG JFIF density.
- [ ] Layout.69.3 Next MSI: run `TEST_CHECKLIST.md` §7, "PNG and JPG resolution".

## [ ] Task Layout.70 — "Some pieces could not be placed" dialog: piece names and length

jacket2 Marker, MSI 26.10.1311: 112 entries such as "Detail (size 240)" (`jacket2_screenshot_1.png`). The 6 "Detail" pieces cannot be told apart. The dialog has no scroll area and grows past the screen.

- [x] Layout.70.1 Use the numbered display name: "Detail (3)" (same rule as Layout.26.5).
- [x] Layout.70.2 One line per piece, then its sizes: "Detail (3): 220, 240, 260". Group by `data-parent` (as Layout.26.1).
- [x] Layout.70.3 `Main.qml` `warningDialog`: scroll area; the OK button stays visible (as Layout.26.2).
- [x] Layout.70.4 Rust test: warning text for a jacket2 Marker layout that leaves pieces out.
- [ ] Layout.70.5 Next MSI: run `TEST_CHECKLIST.md` §7, "Left-out pieces warning".

