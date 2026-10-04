# DXF-ASTM export — ASTM D6673-10 compliance

SeamlyLayout's **DXF-ASTM** export follows ASTM D6673-10 (Sewn Products Pattern Data Interchange).
Standard text: `test-seamly-layout-input/D6673-10-expired.docx`.

## Pipeline

| Stage | Crate / file | Output |
|---|---|---|
| SVG DOM → drawing | `seamly_svg2ezdxf::svg_to_ezdxf` (`converter.rs`) | `Drawing` in millimetres |
| Contours | `seamly_svg2ezdxf/src/astm_contour.rs` | key points + validation polyline |
| Notches | `seamly_svg2ezdxf/src/astm_notch.rs` | D6673 notch POINTs |
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
| Curve tolerance | 0.25 mm | Douglas–Peucker tolerance for the first key points, and limit for the spline through the final key points. Chords between final key points can be farther off |
| Quantity | From the "Cut N" label line: even N → `N/2,N/2`, odd N → `N,0` | Seamly2D exports only a total, not right/left counts |
| Style Name | Input base name: the imported SVG's base name, or the `--document-name` value. Else the pattern `data-name`, else the output file stem | User decision. The output stem carries a timestamp |
| Size | `Size Name:` piece text: piece `data-size` (multisize). Individual: pattern `data-sample-size` = `bust_circ`, else `waist_circ`. Omitted when neither exists | User decision. A multisize layout tab holds one size. Identifier is `Size Name:` (D6673-01 name), not AAMA-292 `Size:` |
| Material | `Material:Fabric` on every piece | User decision. Layout.15.4 replaces it with `data-material` |
| Category | Not written | User decision. `Category:` is ANSI/AAMA-292 piece text, not valid in any DXF-ASTM edition. A piece classification, not material. Seamly2D stores no category; a pattern library could supply one. D6673-10 §4.1 names "category", but §4.3.1.2 defines no `Category:` identifier. See `seamlylayout_DXF_ASTM_D6673_INCONSISTENCIES.md` and `seamlylayout_DXF_ASTM_D6673_from_AAMA.md` |
| Sample Size, Grade Rule Table | Written blank | Required identifiers; export is one ungraded size |

## Layers written

| Layer | Content | Entity |
|---|---|---|
| 1 | Boundary key points; style text (ENTITIES); `Piece Name:`, `Size Name:`, `Quantity:`, `Material:` (BLOCK) | POLYLINE, TEXT |
| 2 / 3 | Turn / curve points of layers 1, 8, 11, 14 | POINT |
| 4 | Slit notch (no 39), V-notch | POINT + 30 depth, 39 width, 50 angle |
| 7 | Grainline | LINE |
| 8 | Internal lines (`internal_path`) | POLYLINE |
| 11 | Internal cutouts (`cut_path`) | POLYLINE |
| 14 | Sew lines (`seamline`) | POLYLINE |
| 15 | Piece and pattern label text | TEXT |
| 80 / 81 / 83 | T / castle / U notch | POINT |
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
- No tag (untagged SVG, or tags that do not match the path): turn point = direction change > 25°, an open end, or an end of a straight chord ≥ 10 mm with at least one dense vertex on it. A single long segment is not evidence of a straight line: coarse curve interpolation makes them too.
- All other key points are curve points.
- Closing repeat vertex removed; group 70 = 1 closes the polyline.

## Boundary and sew lines

- Boundary = longest closed `cutline` path.
- No cut line (seam allowance hidden or built in): the longest closed `seamline` is the boundary.
- A seam line equal to the boundary is not repeated on layer 14.

## Notch rules (`astm_notch.rs`)

Touching segments form one notch. Contacts = vertices within 0.5 mm of the boundary.

| Segments | Contacts | Kind | Layer |
|---|---|---|---|
| 1 | any | Slit | 4 |
| 2 | 2 | V | 4 |
| 2 | 1 | T | 80 |
| 3 | 2 | Castle | 81 |
| ≥ 4 | any | U | 83 |

- Base = centre of the contacts. Angle = toward the inner vertices, counter-clockwise from +X.
- Depth = farthest reach along the angle. Width = spread across it.

## Not exported

Seamly2D sends no source data for these. Tracked in `TODO_SEAMLYLAYOUT.md`.

- Grading, grade rule identifiers (`# n`), graded nests, grade rule table file.
- Grade reference line (layer 5), mirror line (layer 6).
- Stripe and plaid reference lines (layers 9, 10), drill holes (layer 13).
- Check notch (layer 82). Notch dependency (ATTDEF).

## Known producer gaps

- Seamly2D writes empty notch paths (`M x,y Z`) in current handoff SVGs, so no notches reach the DXF.
- No `data-quantity`, `data-on-fold`, `data-material` or `data-notch-type` attributes; quantity and notch kind are inferred; material is always `Fabric`.

## Tests

- `seamly_svg2ezdxf/src/astm_piece_test.rs` — contours, notches, tagged-SVG extraction.
- `ezdxf2dxfastm/src/writer_astm_test.rs` — style text, layers, 2-decimal format, and the
  `test_data/male_shirt_pieces.svg` fixture end to end.
