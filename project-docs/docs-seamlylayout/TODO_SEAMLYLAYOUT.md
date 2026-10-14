# TODO — SeamlyLayout app features

Tasks that add features to the SeamlyLayout layout app.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

Tasks in this file are numbered and are prefixed with `Layout.`

## [ ] Task Layout.3 — Multisize handoff and layout: nested (piece groups) or marker (single pieces)

A pattern with a `.smms` multisize measurement file hands SeamlyLayout every size in one stringified SVG. Each piece is a group that holds that piece in every size. `process_layout()` packs either the groups or the single pieces.

Decisions (user, 2026-09-28):

- Sizes: every size in the `.smms` file, at the base height only. No size × height combinations.
- Stack order in a group: each size centered on the largest size's center, after the grainline is vertical.
- Individual `.smis` patterns keep today's handoff, unchanged.

Handoff shape: see `project-docs/docs-data/SVG-DATA-ATTRIBUTES.md`, "Multisize pattern".

- [ ] Layout.38 Tests
  - [ ] Layout.38.6 Check in the running apps: Seamly2D Layout Mode with a multisize pattern; SeamlyLayout Nested and Marker
    - Needs the next MSI. Add to the next `TEST_CHECKLIST.md`.

## [ ] Task Layout.6 - DXF-ASTM: D6673 features without a Seamly2D source

See `dxf-docs/seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`, "Not exported".

- [ ] Layout.61 Grading: grade rule identifiers (`# n` text), graded nests (one block per size, `Size Name:`), grade reference line (layer 5), grade rule table file
  - [ ] Layout.61.7 User: export a multisize pattern from "Nested" as DXF-ASTM; check it in aw.fyi (DXF.8.4)
    - [ ] Layout.61.7.2 User: install MSI 26.10.1220 or later. Export jacket2 "Nested" as DXF-ASTM (R13); run aw.fyi
      - Expect: no "Detail" merge departures, no layer 5 mismatch, no duplicate vertices. Detail (3), (6), (7), (8) stay sample size only until Layout.27.3
    - [ ] Layout.61.7.3 User: Marker "Nested" after Layout.25 and Layout.27.10

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

## [ ] Task Layout.16 — Fold lines in layout and export

Depends on Seamly2D.8.

- [ ] Layout.16.1 Import: read `data-on-fold` and the `fold_line` component
- [ ] Layout.16.2 Layout setting: pack the half piece, or unfold it (mirror across the fold line) before packing
- [ ] Layout.16.3 Adjust Mode: the fold line moves with its piece
- [ ] Layout.16.4 DXF-ASTM: layer 6 mirror line; boundary starts or ends on the mirror line points (D6673 §4.3.6); unfolded pieces write no layer 6
- [ ] Layout.16.5 Tests: Rust import, unfold geometry, layer 6 and boundary order

## [ ] Task Layout.67 — Multisize tabs: Marker, Nested, Base Size, then each size

Decision (user, 2026-10-13): simplify the multisize tabs and exports. Every tab takes its pieces from the imported SVG; no tab is built from grade rules. DXF-ASTM carries one size per file. Replaces Layout.22 and Layout.23. Layout.27.10 and Layout.61.7.3 no longer apply: the Marker tab has no DXF-ASTM export.

| Tab | Packs | DXF-ASTM | Other exports |
|---|---|---|---|
| Marker | every size of every piece alone | none | yes |
| Nested | each piece once, its sizes stacked | none | yes |
| Base Size N | the base size | `# n` texts, layer 5, `.rul`, `Sample Size:N` | yes; Export all tabs skips it |
| Size N | one size | no `.rul`, blank `Sample Size:` | yes |

- [ ] Layout.67.8 User: on the next MSI, run `TEST_CHECKLIST.md` §7 "Multisize tabs"
- [ ] Layout.67.9 "Export Complete" names only the `.dxf`. A Base Size export also writes the `.rul`; name both files (`jacket2_basesize_screenshot_1.png`, MSI 26.10.1311)

## [ ] Task Layout.24 — Multisize export file name: tab name before the date stamp

Default name of a multisize export: `<base>[_<segment>]_<tab>_<stamp>[_tiled].<ext>`. Example: `mypatternname_R13_Size220_202610112043.dxf`. An individual pattern keeps `<base>[_<segment>]_<stamp>[_tiled].<ext>`.

- [ ] Layout.24.1 `exports::default_export_file_name`: optional tab segment between `<segment>` and `<stamp>`
- [ ] Layout.24.2 Tab segment from the tab label: remove spaces; any other character that is not a letter, digit or `-` becomes `-`. "Size 220" → `Size220`
- [ ] Layout.24.3 Single export from a multisize layout: the default name uses the selected tab
- [ ] Layout.24.4 Export all tabs: each file name uses the same rule. Replaces `layout_views::tab_file_path`, which puts `_size-220` after the stamp. A name the user changed in the file dialog gets `_<tab>` before the extension
- [ ] Layout.24.5 Tests: Rust names with and without a tab, tiled, a label with `/`; Export all tabs names
- [ ] Layout.24.6 Docs: export file name rule in the SeamlyLayout docs, `TEST_CHECKLIST.md`

## [ ] Task Layout.25 — Multisize Marker layout: extend the media height until every piece fits

Applies to every multisize tab: "All sizes" (Nested or Marker), and each size tab (user decision: D3). It does not wait for Layout.23; after Layout.23 the rule covers both all-sizes tabs. Today a piece that does not fit is left out, and a warning names it.

Evidence (jacket2, Marker, 48 in × 500 in paper roll): the pack placed 96 of 208 pieces. `test-seamly-layout/layouts/jacket2/jacket2_marker_screenshot_1.png`.

Repeat with MSI 26.10.1311, Marker tab: 96 of 208 placed again. The roll is full: pieces use 74 % of its area. The 208 pieces need about 740 in. `jacket2_screenshot_1.png`, `jacket2_40-146_202610131245.svg`.

Decision (user, 2026-10-13): a layout cuts fabric, so the media grows past the 500 in roll length when the pieces need it.

- [ ] Layout.25.7 User: on the next MSI, run `TEST_CHECKLIST.md` §7 "Media grows to fit"

## [ ] Task Layout.71 — Multisize: widen the media for a piece wider than it

Decision (user, 2026-10-13): on a multisize tab, a piece wider than the content rect widens it to the right; a piece longer than it lengthens it downwards (Layout.25). Splitting pieces is for individual patterns only (Layout.72).

- [ ] Layout.71.6 User: on the next MSI, jacket2, Paper Roll 10 in wide, Create Layout: no warning; every tab is wider than 10 in; Adjust Mode and Export SVG show the same width

## [ ] Task Layout.72 — Individual pattern: a piece wider or longer than the media

Decision (user, 2026-10-13): when a piece of an individual pattern is wider or longer than the media, a dialog asks the user to split the pieces or make the media bigger.

- Dialog: names each piece and the side that does not fit. Radio buttons: "Split the pieces" (SVG, PDF, HPGL, EPS, PS, PNG, JPG: printing, cutting, pattern projector) or "Make the media bigger" (DXF). OK continues the layout. Cancel returns to the app with only 'Layout Settings' active.
- Split rule: wider than the content width, and no allowed turn fits → vertical cut lines. Longer than the content height, media not roll-form (paper roll, fabric), and no allowed turn fits → horizontal cut lines. Tiled paper: no dialog.
- Make the media bigger: same as a multisize tab (Layout.71, Layout.25).
- Accepted recommendations (user, 2026-10-13): no seam allowance on the cut edge; n equal parts, n rounded up; label plus 2 match notches per cut edge; fabric is roll-form; the paper-sheet PDF export uses the same split; a piece that fits but runs out of room is still left out with a warning.

- [ ] Layout.72.8 User: on the next MSI, trousers sample, Paper Sheet 36 × 36 in, Create Layout:
  - [ ] Dialog "Pieces do not fit the media" lists the 4 legs (Back, Front_Pleats, Front_no_pocket, Front_with_pocket) as longer than the media
  - [ ] Cancel: only Layout Settings, Preferences and Exit are active. Submit settings: Create Layout is active again
  - [ ] Split the pieces, OK: each leg is 2 parts with a blue label "<name> part i of 2, joins j" at the cut edge and 2 green match notches. Pieces that run out of room are named in a warning
  - [ ] Make the media bigger, OK: no piece split, the sheet is longer than 36 in, no warning
  - [ ] Split, Export PDF: the pages hold the parts. Export DXF-ASTM: `Piece Name:Back (1/2)` and `Back (2/2)`

## [ ] Task Layout.26 — DXF-ASTM Missing Piece Data dialog: one readable line per piece

Found with the installed sample `jacket2_40-146.sm2d` (`gost_man_ru.smms`), "Nested" tab, Marker. Evidence: `test-seamly-layout/layouts/jacket2/jacket2_marker_screenshot_2.png`, `jacket2_nested_screenshot_1.png`. The installed sample has Cyrillic names because the local MSI ships stale copies (Layout.40). The pattern hides each piece label and grainline (`visible="false"`), so those warnings are correct.

- [ ] Layout.26.7 User: on the next MSI, check the dialog scrolls and keeps its buttons (jacket2, All sizes, Marker)

## [ ] Task Layout.27 — DXF-ASTM grading fails for jacket2_40-146 from the "Nested" tab

Evidence: `test-seamly-layout/layouts/jacket2/` — `jacket2_nested_screenshot_2.png`, `jacket2_marker_screenshot_3.png`, the `nested_allsizes` and `marker_allsizes` DXF files and aw.fyi reports. 8 pieces in sizes 220–720.

- Nested: 4 pieces graded; 4 "Detail" pieces fell back to size 500.
- Marker: the layout placed 96 of 208 pieces (Layout.25). 4 pieces lost size 500; the other 4 failed the turn point check, so grading removed 71 placed blocks. The file has 25 blocks and no `.rul`.
- MSI 26.10.1311, Base Size 500 tab (`jacket2_40-146_basesize_R13_202610131248.dxf`): 4 pieces graded. Their rules match all 26 sizes within 0.011 mm (cut and sew lines, checked against the Nested SVG). Detail (3), (6), (7), (8) not graded again (Layout.27.3).

- [ ] Layout.27.1 "size 500 is not in the layout" (3 pieces): the Marker layout left the sample size block out. Layout.25 fixes this; export again and check
- [ ] Layout.27.2 When the layout left the sample size block out, the warning says so: "size 500 does not fit the media"
- [ ] Layout.27.3 "the sizes have different turn points" (4 "Detail" pieces). Cause (checked in the per-size Export-all-tabs DXF files, layer 2 counts): Seamly2D's `VAbstractPiece::turnPointIndices` flips at near-limit corners as the size changes. Piece 3: 11 → 10 from size 360; piece 1: 12 → 14/16 at 660–700; piece 6: 8 → 10 from 660; piece 7: 8 ↔ 10 from 520. Fix in the producer: after `prepareMultisizePieceLists`, a piece node that is a turn point in any size is a turn point in every size (seam line, cut line, internal paths). Needs the node vertex indices of each path in `VLayoutPiece` / `VLayoutPiecePath`. Skip a path whose node count differs between sizes
  - [ ] Layout.27.3.2 Build: `local_build_msi.ps1` runs the Qt suites
  - [ ] Layout.27.3.3 User: on the next MSI, jacket2 Base Size 500 tab, Export DXF-ASTM. Expect: Detail (3) and (6) graded. Detail (7) and (8) stay ungraded until Layout.27.12
  - Finding (fixture `jacket2_40-146_202610121819.svg`): Seamly2D sends different `data-turn-points` counts for Detail (3) and (6) only. Detail (7) and (8) get 4 per line in every size. The cause text's "piece 6" and "piece 7" are Detail (7) and (8): see Layout.27.12
- [ ] Layout.27.4 `grade_anchors` also returns None when a key point is not on the dense line. That case gets its own message
- [ ] Layout.27.5 The turn point warning names the first size that differs from the sample size
- [ ] Layout.27.7 Fixture: the jacket2 handoff in `crates/cxxqt_bridge/test_data/`; end-to-end test: every piece graded, `.rul` file written
- [ ] Layout.27.8 No `.rul` file: `astm_grading::grade_drawing` sets a rule table only when one piece or more grades. When no piece grades, "Export Complete" says "No piece graded: no .rul file written"
- [ ] Layout.27.10 Marker layout, piece that cannot grade: write every placed size as its own ungraded piece, size in the piece name: "Detail (3) 240" (user decision: D4). Today only the sample size is written, so placed cut pieces are lost. Nested keeps the sample size only: its sizes overlap
- [ ] Layout.27.11 Near-duplicate vertices remain. Base Size file (MSI 26.10.1311), Подборт, layer 87: `(333.01, 626.21)` then `(333.01, 626.22)`. `SNAP_VERTEX_MM` (0.01) equals the written precision (2 decimals), so a key point 0.01–0.02 mm from a vertex stays a separate vertex. Snap at 0.02 mm, or compare the written coordinates. Test: no two consecutive written vertices 0.01 mm apart or closer
- [ ] Layout.27.12 Line-end turn points differ between sizes: Detail (7) from size 660; Detail (8) at 520–640, 680 and 700. `astm_contour::straight_run_ends` adds line ends to tagged lines too. Its limits flip between sizes: Detail (7) has a 9.97 mm straight run at 640 and 10.2 mm at 660 (line limit 10 mm); Detail (8) flips on the 2× break test. Decision needed:
  - Option 1: Seamly2D applies the line-end test at nodes, where it knows the line sides. `unifyTurnPoints` shares the result across sizes. SeamlyLayout stops adding line ends to tagged lines. Old handoff fixtures need new copies
  - Option 2: SeamlyLayout grading shares line ends across sizes. Needs node vertex indices from Seamly2D (new attribute) to match nodes between sizes

## [ ] Task Layout.28 — Built-in seam allowance: show the seam line

Before this task: a piece with a built-in seam allowance had no seam line. Its main path is the cut line, and the handoff sent it as `seamline`. SeamlyLayout drew only that line. Seamly2D drew no seam line for it either.

Seamly2D computes the seam line: only Seamly2D knows the seam allowance width at each node.

The task stays in this file: it changes what SeamlyLayout lays out and exports (user decision).

Status (2026-10-12):

- Code, tests and docs: done. Merged into local `run-seamlyLayout`, not pushed
- `cargo test --workspace`: 472 passed, 0 failed. `local_build_msi.ps1`: Qt suites passed, MSI 26.10.1220 built
- Open: the user check in the installed apps (Layout.28.7)

- [ ] Layout.28.7 User: on the next MSI, run `TEST_CHECKLIST.md` §5 "Piece with a built-in seam allowance"

## [ ] Task Layout.68 — PDF export: page and pieces 133 % too large

Found with jacket2, MSI 26.10.1311. `svg2pdf` `PageOptions::default()` uses 72 dpi, so 1 SVG px (1/96 in) becomes 1 pt (1/72 in).

- Marker `jacket2_40-146_202610131244.pdf` (48 in × 500 in roll): MediaBox `4608 × 48000` pt = 64 in × 666.7 in. Expected `3456 × 36000` pt.
- Nested `jacket2_40-nested_146_202610131245.pdf`: `4608 × 5003` pt. Expected `3456 × 3752` pt.
- EPS and PS of the same tabs are correct: `0.75` scale, BoundingBox `3456 36000`.

- [ ] Layout.68.5 User: on the next MSI, print one tiled page at 100 %; measure a known length. Open the Marker PDF in Acrobat: Document Properties shows 48 × 500 in

## [ ] Task Layout.69 — PNG and JPG exports: no resolution metadata

Found with jacket2, MSI 26.10.1311. The pixel size is correct: `jacket2_40-146_202610131243.png` is 4608 × 48000 px, 96 px/in. The PNG has no `pHYs` chunk. The JPG JFIF density unit is 0 (aspect ratio only). A viewer or print dialog then uses its own default resolution, so "actual size" can print at the wrong scale.

- [ ] Layout.69.1 PNG: write `pHYs` 3780 px/m (96 dpi). JPG: JFIF density 96 × 96 dpi
- [ ] Layout.69.2 Tests: Rust reads the PNG `pHYs` and the JPG JFIF density

## [ ] Task Layout.70 — "Some pieces could not be placed" dialog: piece names and length

Found with jacket2, MSI 26.10.1311, Marker tab: 112 entries such as "Detail (size 240)" (`jacket2_screenshot_1.png`). The 6 "Detail" pieces cannot be told apart. The dialog has no scroll area; it grows past the screen when more pieces are left out.

- [ ] Layout.70.1 Use the numbered display name: "Detail (3)" (same rule as Layout.26.5)
- [ ] Layout.70.2 One line per piece, then its sizes: "Detail (3): 220, 240, 260". Group by `data-parent` (as Layout.26.1)
- [ ] Layout.70.3 `Main.qml` `warningDialog`: scroll area; the OK button stays visible (as Layout.26.2)
- [ ] Layout.70.4 Tests: Rust warning text for a jacket2 Marker layout that leaves pieces out
