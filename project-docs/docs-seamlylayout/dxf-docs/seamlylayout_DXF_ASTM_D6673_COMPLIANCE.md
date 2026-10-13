# DXF-ASTM export — ASTM D6673-10 compliance

SeamlyLayout's **DXF-ASTM** export follows ASTM D6673-10 (Sewn Products Pattern Data Interchange).
Standard text: `test-seamly-layout-input/D6673-10-expired.docx`.

## Pipeline

| Stage | Crate / file | Output |
|---|---|---|
| SVG DOM → drawing | `seamly_svg2ezdxf::svg_to_ezdxf` (`converter.rs`) | `Drawing` in millimetres |
| Contours | `seamly_svg2ezdxf/src/astm_contour.rs` | key points + validation polyline |
| Notches | `seamly_svg2ezdxf/src/astm_notch.rs` | Slit notch POINTs (layer 4) |
| Grading | `seamly_svg2ezdxf/src/astm_grading.rs`, `astm_contour::grade_contours` | Graded nests, grade rules |
| Rule table | `ezdxf2dxfastm/src/grade_rule_file.rs` | `.rul` file beside the DXF |
| Drawing → file | `ezdxf2dxfastm::export_dxf_astm` (`writer.rs`) | `.dxf` (+ annotated `.txt`) |
| R12 → R13 | `ezdxf2dxfastm::upgrade_to_r13` (`r13.rs`) | R13 file from the R12 group stream |
| Bridge | `cxxqt_bridge::exports::do_export_dxf` | style text from QML |

## Decisions

| Topic | Decision | Reason |
|---|---|---|
| Units | Always `Units:METRIC`: millimetres to 2 decimals. Every real value (groups 10–59) has exactly 2 decimals, R13 table and dummy values included (`0.00`) | User decision. SVG px (96 dpi) × 25.4 / 96 |
| DXF version | User picks: **DXF-ASTM (R12)** writes `AC1009`; **DXF-ASTM (R13)** writes `AC1012`. **DXF-ASTM (CLO3D)** writes R13 (`AC1012`): the CLO3D variant must be D6673-10 compliant (user decision) | D6673 §1.2 / 4.1 names R13. R12 has the widest importer support. Gerber AccuMark writes no version at all |
| R13 structure | R12 group stream plus handles (5; 105 for DIMSTYLE), subclass markers (100), CLASSES (empty), TABLES (VPORT, LTYPE, LAYER, STYLE, VIEW, UCS, APPID, DIMSTYLE, BLOCK_RECORD), `*MODEL_SPACE` / `*PAPER_SPACE` blocks, OBJECTS (root dictionary + ACAD_GROUP) | One source of D6673 content for both versions. No owner handles (330) on entities and table entries: R14 added them. Checked: ezdxf 1.4.4 `audit` 0 errors, 0 fixes; libdxfrw reads the same entity counts as R12 |
| Header | R12: `$ACADVER` only. R13: `$ACADVER`, `$DWGCODEPAGE`, `$HANDSEED` | D6673 §4.2: keep the header minimal. R13 needs `$HANDSEED` |
| CLO3D group 250 | Off in **DXF-ASTM (R12)** and **(R13)**; on in **DXF-ASTM (CLO3D)** | Not DXF R12, not D6673. CLO3D reads it: 0 = boundary (layer 1), 2 = sew line (layer 14). Only those key-point polylines carry it. The R13 rewrite keeps it |
| Block names | `<svg id>_M` kept | Existing CLO3D naming. `Piece Name:` carries the real name |
| Piece Name | Piece `data-name`, each non-ASCII character as an AutoCAD `\U+XXXX` escape: "Комір" → `\U+041A\U+043E\U+043C\U+0456\U+0440`. A name that several pieces share ends in the piece letter, else the piece number: `Detail (3)`. Every size of a piece gets the same name. Dialogs and warnings show the name as written, with the same number | User decision. System text is 7-bit ASCII (§4.3.1.2). aw.fyi groups the sizes of a piece by name, and merges pieces that share one. Piece number: piece-set `data-type-number`, else the piece `data-set-number` (Marker layout), else the individual piece `data-type-number` |
| Missing Piece Data dialog | One line per piece, not per placed size. A size that lacks Quantity, label or grainline marks it missing for the piece | A multisize layout places every size |
| Curve tolerance | 0.25 mm | Douglas–Peucker tolerance for the first key points, and limit for the spline through the final key points. Chords between final key points can be farther off |
| Quantity | Piece `data-quantity`, else the "Cut N" label line: even N → `N/2,N/2`, odd N → `N,0`. Piece `data-on-fold="true"` → `N,0` | Seamly2D exports only a total, not right/left counts. A folded piece unfolds to one symmetric piece, never a left and right pair |
| Style Name | Input base name: the imported SVG's base name, or the `--document-name` value. Else the pattern `data-name`, else the output file stem | User decision. The output stem carries a timestamp |
| Size | `Size Name:` piece text: piece `data-size` (multisize). Individual: pattern `data-sample-size` = `bust_circ`, else `waist_circ`. Omitted when neither exists | User decision. A multisize layout tab holds one size. Identifier is `Size Name:` (D6673-01 name), not AAMA-292 `Size:` |
| Material | `Material:Fabric` on every piece | User decision. Layout.15.4 replaces it with `data-material` |
| Category | Not written | User decision. `Category:` is ANSI/AAMA-292 piece text, not valid in any DXF-ASTM edition. A piece classification, not material. Seamly2D stores no category; a pattern library could supply one. D6673-10 §4.1 names "category", but §4.3.1.2 defines no `Category:` identifier. See `seamlylayout_DXF_ASTM_D6673_INCONSISTENCIES.md` and `seamlylayout_DXF_ASTM_D6673_from_AAMA.md` |
| Sample Size, Grade Rule Table | Graded export: `data-base-size`, and the output file stem (separators → `_`). Otherwise blank | Required identifiers. An individual pattern or a size tab exports one ungraded size |
| Grading | Only a multisize layout exported from the **All sizes** tab | User decision: graded nest + `# n` identifiers + `.rul` table file. A size tab holds one size |

## Layers written

| Layer | Content | Entity |
|---|---|---|
| 1 | Boundary key points; style text (ENTITIES); `Piece Name:`, `Size Name:`, `Quantity:`, `Material:` (BLOCK) | POLYLINE, TEXT |
| 2 / 3 | Turn / curve points of layers 1, 8, 11, 14 | POINT |
| 4 | Every notch, as a slit (no 39) | POINT + 30 depth, 50 angle |
| 5 | Grade reference line, every block of a graded nest | LINE |
| 7 | Grainline | LINE |
| 8 | Internal lines (`internal_path`) | POLYLINE |
| 11 | Internal cutouts (`cut_path`) | POLYLINE |
| 14 | Sew lines (`seamline`) | POLYLINE |
| 15 | Piece and pattern label text | TEXT |
| 84 / 85 / 86 / 87 | Validation curves for 1 / 8 / 11 / 14 | POLYLINE |

## Contour rules (`astm_contour.rs`)

- Seamly2D sends interpolated polylines; it tags the turn point vertices with `data-turn-points`.
- `dense` = the full polyline → validation layer (84–87).
- `reduced` = Douglas–Peucker key points per span between turn points → primary layer (1, 8, 11, 14).
- Spline check: two Catmull-Rom splines through the key points, centripetal and chord-length, split at turn points (reflected end tangent). The reader's method is unknown; these are stand-ins. They differ most where neighboring chords differ in length.
- Where either spline is more than 0.25 mm from `dense` (either direction), the farthest dense vertex becomes a curve point. Repeat until every segment passes.
- A segment with no dense vertex between its key points has nothing to add. When a spline bulges there, the dense edge is split at the point nearest the bulge; the new vertex lies on the edge. Edges under 1 mm are not split.
- Then each curve point neither spline needs is dropped, least-needed first. Turn points and the first key point stay.
- `spline_deviation()` measures the result; `writer_astm_test.rs` checks every male_shirt contour on layers 84–87.
- `reduced` is an ordered subset of `dense`, and both start at the same vertex (§4.3.3.1).
- Turn points come from Seamly2D's `data-turn-points` tag on each path component (see `docs-data/SVG-DATA-ATTRIBUTES.md`). Seamly2D marks a node vertex whose tangent breaks by more than 0.5° (line–line) or 5° (with a curve), any vertex that turns more than 25°, and open ends.
- Tagged or not, a line end is a turn point where the tangent breaks, even under Seamly2D's 5°.
  - Line: every vertex within 0.01 mm of the chord, ≥ 10 mm long. A single segment needs ≥ 20 mm: shorter ones are curve chords (tight curve ends reach about 11 mm).
  - Break: direction change at the line end > 2 × the change at the next curve vertex, and > 1°. A tangent junction bends about half as much as the next vertex; a coarse curve bends equally at every vertex.
  - Next piece a line with ≥ 2 inner vertices: compare against 0°.
  - A closed loop is split into lines from its first known turn point, so no line starts mid-edge.
  - aw.fyi checks: FrontPanel top edge 2.9° vs 1.0° → turn. FullSleeve hem 1.2° vs 2.0° → curve. Flat curves 0.8° vs 0.2° → curve (below 1°).
- Open contour ends (internal lines, slits) are curve points, tagged or not. User decision: aw.fyi reports layer 2 points there as not needed. The reader's spline still ends there.
- A straight open line (2 key points: pleat line, slit) writes nothing on layers 2 and 3. aw.fyi flags its ends as not needed, as turn points and as curve points. Slit ends are never turn points (user decision). `AstmContour::declares_points`.
- No tag (untagged SVG, or tags that do not match the path): turn point = direction change > 25°, or an end of a straight chord ≥ 10 mm with at least one dense vertex on it. A single long segment is not evidence of a straight line: coarse curve interpolation makes them too.
- All other key points are curve points.
- Closing repeat vertex removed; group 70 = 1 closes the polyline.

## Boundary and sew lines

- Boundary = longest closed `cutline` path.
- No cut line (no seam allowance, or seam allowances not shown): the longest closed `seamline` is the boundary.
- Built-in seam allowance: the `cutline` is the main path, so it is the boundary (layer 1). The `seamline` is the line Seamly2D computes inside it, a sew line (layer 14).
- A seam line equal to the boundary is not repeated on layer 14.

## Notch rules (`astm_notch.rs`)

Every notch is a slit on layer 4, whatever shape Seamly2D drew (slit, T, U, V, castle, diamond). User decision.

- Touching segments form one notch. Contacts = vertices within 0.5 mm of the boundary.
- Base = centre of the contacts. Angle = toward the inner vertices, counter-clockwise from +X.
- Depth = half the seam allowance width at the notch. User decision: keep each edge's width as Seamly2D defines it.
- Width source, first found:
  1. Seamly2D's width for the notch node, `data-seam-allowances`: one value per notch. `data-notch-segments` gives each notch's segment count, so each segment gets the width of its notch. Without `data-notch-segments`, one value per segment. A list of the wrong length is ignored. The only source for a piece without a cut line.
  2. Distance from the base to the nearest sew line (`seam_allowance_at`), for an SVG without the attribute.
  3. None, or width under 0.1 mm: depth = drawn depth (farthest reach along the angle).
- No width (group 39). Layers 80, 81, 83 are not written.

## Grading (`astm_grading.rs`)

A multisize layout exported from the **All sizes** tab is graded (D6673 §4.3.1.4, §4.3.1.5, §4.4).

- Input: the layout DOM (every size of every piece, placed) and the handoff DOM (every size at its drafted position).
- A piece's sizes share the piece `data-parent` (piece-set id). Blocks of one piece are written together, in `data-sizes` order.
- Key points: `grade_contours` gives every size the same turn points and the same curve points. Curve point j sits at the same arc-length fraction of the same span in every size. Curve points are added until every size passes the 0.25 mm spline check, then dropped where no size needs them.
- Duplicate vertices: a key point within 0.01 mm of a dense vertex snaps to that vertex, so grading inserts no vertex within 0.01 mm of another. A curve point within 0.01 mm of a span's turn point, or on the vertex of the key point before or after it, in any size, is dropped.
- Grading runs in draft space: a size differs from the sample size only by growth there.
- Rule = growth of one key point or notch from the sample size, per size. Rules equal within 0.01 mm share one identifier. Zero growth is a rule too (§4.3.1.4).
- Sample size block: every layer, full piece system text, `# n` TEXT on layers 1, 4, 8, 11, 14 at each graded point.
- Graded size block: `Piece Name:`, `Size Name:`, `Quantity:`; layers 1, 7, 8, 11, 14, 15, 84–87, 5. No layers 2, 3, 4 (§4.3.1.5).
- Grade reference line (layer 5): one horizontal draft-space line along the bottom of the sample boundary, the same for every size. A rigid fit of each layout block to its handoff block maps it into the block. Stacking the blocks on it gives the rule growths.
- `.rul` file: same stem as the DXF. Header: version, `AUTHOR:Seamly2D`, date, time (hh:mm), `UNITS:METRIC`, table name, sample size, number of sizes, size list. Then `RULE: DELTA n dx,dy ...`, one growth per size.
- Not graded, with a warning in the export dialog; only the sample size block is written:
  - sizes differ in turn points, lines or notch count;
  - a layout block does not fit its handoff shape (also a mirrored piece).
- The sample size is not in the layout (unplaced): the piece is written ungraded, every size, with a warning.

## Not exported

Seamly2D sends no source data for these. Tracked in `TODO_SEAMLYLAYOUT.md`.

- Alternate grade reference lines (layer 5), mirror line (layer 6).
- Stripe and plaid reference lines (layers 9, 10), drill holes (layer 13).
- Check notch (layer 82). Notch dependency (ATTDEF).

## Known producer gaps

- No `data-material` attribute; material is always `Fabric` (Seamly2D.7 / Layout.15).
- `data-notch-type` is read by no exporter: DXF-ASTM writes every notch as a slit.

## aw.fyi results

Reports live beside the DXF files in `test-seamly-layout/layouts/<pattern>/`.

| Report | Departures | Open findings |
|---|---|---|
| `male_shirt_R13_202610071908.dxf` | 2 | FrontPanel junction missing turn point; FullSleeve open line ends as turn points (DXF.19) |
| `male_shirt_R13_202610101837.dxf` | 1 | FullSleeve open line ends as curve points (DXF.21); FrontPanel L1 junction "should be curve" while L14 agrees (DXF.22) |
| `male_shirt_R12_202610101837.dxf` | 2 | As R13, plus "File is not DXF R13" (expected for R12) |
| `jacket2_40-146_nested_allsizes_R13_202610121136.dxf` | 2 | Both from 6 pieces named "Detail" (Layout.26.5); layer 5 mismatch, same cause; duplicate vertices (Layout.27.9) |
| `jacket2_40-146_nested_size240_R13_202610121142.dxf` | 3 | 2 from the "Detail" names; Detail (4) and Detail (6) bends not turn points: reader advisory (DXF.23) |
| `jacket2_40-146_marker_allsizes_R13_202610121149.dxf` | 3 | 2 from the "Detail" names; Detail (4) bend: reader advisory (DXF.23). Nothing graded (Layout.25, 27.3) |

Advisories in the 2026-10-10 report: 4 pieces with no densified points (straight-sided pieces); FullSleeve hem turn point and ShortSleeve L14 pt 31 "maybe". Notch depths, curve tolerance and validation spans pass.

The jacket2 files come from MSI 26.10.1115, before Layout.26 and Layout.27.9. Advisories: straight-sided pieces with no densified points; more curve points than 3 primitives need on 2 long curves. Curve tolerance and validation spans pass.

## Tests

- `seamly_svg2ezdxf/src/astm_piece_test.rs` — contours, notches, tagged-SVG extraction.
- `ezdxf2dxfastm/src/writer_astm_test.rs` — style text, layers, 2-decimal format, and the
  `test_data/male_shirt_pieces.svg` fixture end to end. Graded nest blocks, `.rul` file, layer 5 in R13.
- `seamly_svg2ezdxf/src/astm_grading_test.rs` — shared key points, rules against the stacked nest, fallbacks.
  Fixtures: `ezdxf2dxfastm/test_data/multisize_grading_input.svg`, `multisize_grading_layout.svg`.
- `cxxqt_bridge/src/layout_utils.rs` `multisize_layout_exports_graded_nests` — Nested and Marker layouts, end to end.
