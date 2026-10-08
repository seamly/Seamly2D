# TODO — DXF-ASTM compliance

Fixes for the ApparelWerks D6673 report on `test-seamly-layout-input/male_shirt_202610031234.dxf`
(`male_shirt_202610031234-dxf-report.pdf`). Applies to DXF-ASTM and DXF-ASTM (CLO3D); the two files differ only in group 250.

Check off subtasks as they are done. When every subtask of a task is done, move the task to `project-docs/TODO_COMPLETED.md`.

Tasks in this file are prefixed with `DXF.`

Reference: `dxf-docs/seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`.

## Report findings

| Finding | Severity | Task |
|---|---|---|
| Declared points miscategorized: curve points on layer 2 (7 pieces) | departure | DXF.1 (done, see `TODO_COMPLETED.md`) |
| Reconstruction from layers 2/3 off layer 84 by 0.532 (ShortSleeve_M), layer 87 by 0.942 (FullSleeve_M); limit 0.5 | extension | DXF.2 (done, see `TODO_COMPLETED.md`) |
| File is AC1009, not R13 (AC1012) | departure | DXF.3 (code done; CLO3D import check open) |
| More curve points than the curves need (Yoke_M L14, PocketFlapRound_M L1) | advisory | DXF.4 (done, see `TODO_COMPLETED.md`) |
| Pieces with no `Quantity:`, label, or grainline (CollarTopInterface, CollarBaseInterface, CuffInterface) | report table | DXF.5 (done, see `TODO_COMPLETED.md`) |
| `Style Name:` is the output file stem with timestamp | report header | DXF.6 (done, see `TODO_COMPLETED.md`) |
| Category, Material, Size columns blank | report table | DXF.7 (done, see `TODO_COMPLETED.md`) |
| `Author` not `vendor;application;release #` (got `Seamly2D 26.10.314`; report on `male_shirt_202610031615.dxf`) | departure | DXF.9 (done, see `TODO_COMPLETED.md`) |
| Reconstruction from layers 2/3 off layer 84 by 0.8469 (CollarTopInterface_M), layer 87 by 0.8423 (CollarTop_M); report on `male_shirt_202610032115.dxf` | extension | DXF.11 (done, see `TODO_COMPLETED.md`) |
| FrontPanel_M: layer 2 misses 1 of 14 corners (top edge, where a line meets a curve at ~3°); report on `male_shirt_R13_202610041815.dxf` | departure | DXF.15 (done, see `TODO_COMPLETED.md`) |
| FullSleeve_M: 10 of 80 points miscategorized, on a straight segment (hem straight run between curves); FrontPanel_M 1 point | departure | DXF.16 (done, see `TODO_COMPLETED.md`) |
| ShortSleeve_M: 1 point may be miscategorized | advisory | DXF.16; re-check in DXF.8 |
| FrontPanel_M L1 + L14: line→curve junction not a turn point (DXF.15 repeat); report on `male_shirt_R13_202610071908.dxf` | departure | DXF.19 |
| FullSleeve_M: 10 L2 points at pleat (L8) and slit (L11) ends "not needed"; 2 hem run ends "should be curve" (DXF.16 repeat) | departure | DXF.19 |
| CollarBaseInterface_M, CollarBase_M: turn point on a 1.7° curve | advisory | DXF.19 |
| Yoke_M L14: turn point mid straight edge | advisory | DXF.20.1 |
| ShortSleeve_M pt 31: sew-line turn point at hem corner on a straight edge | advisory | DXF.20.2 |
| FullSleeve_M: slit end 0.21 mm from cut-line turn point | advisory | DXF.20.3 |
| Notch depth 2.5 and 1.25 mm; 3 notches on layer 83 (U); rule: every notch a layer 4 slit, half the seam allowance | gap | DXF.17 (done, see `TODO_COMPLETED.md`) |

Report deviations carry an inch sign but match millimetres. Not verified.

Report on `male_shirt_202610031615.dxf` (after DXF.1): miscategorized points now advisory (3 pieces). DXF.2 still open: FullSleeve_M off layer 84 by 0.5319, layer 87 by 0.9309. New advisory: 5 pieces have no densified points (PlacketUnder_M and 4 others).

## [ ] Task DXF.19 — Turn points only where the tangent breaks

aw.fyi report on `male_shirt_R13_202610071908.dxf` (folder `test-seamly-layout-input/male_shirt_R12_202610071908/`): DXF.15 and DXF.16 repeat. DXF.16 made every straight run end a turn point; a single straight segment did not count as a run.

- [x] DXF.19.1 `astm_contour.rs` `straight_run_ends`: a line end is a turn point when its direction change > 2 × the next curve vertex's, and > 1°
- [x] DXF.19.2 A single segment ≥ 20 mm is a line; a run with inner vertices needs ≥ 10 mm
- [x] DXF.19.3 Closed loop split from its first known turn point
- [x] DXF.19.4 Open contour ends are curve points (user decision); reverses `open_polyline_ends_are_turn_points`
- [x] DXF.19.5 Tests: `open_polyline_ends_are_curve_points`, `line_ends_are_turn_points_where_the_tangent_breaks`, `single_segment_line_end_is_a_turn_point_only_where_the_tangent_breaks`; `writer_astm_test.rs` FrontPanel 16 turn points
- [x] DXF.19.6 Compliance doc "Contour rules"
- [ ] DXF.19.7 User: re-export male_shirt DXF-ASTM (R13); run aw.fyi (DXF.8)
- Checked on `male_shirt_202610071910.svg`: FrontPanel junction (71.10 / 71.35) now a turn point; FullSleeve hem end 345.88 / 345.77 now a curve point (252.5 stays a turn point, 1.9° vs 0.3°); L8/L11 ends curve points; one CollarBase and one CollarBaseInterface curve turn point removed; Yoke L14 mid-edge turn point removed. No other change.

## [ ] Task DXF.20 — Low-priority report advisories

From the `male_shirt_R13_202610071908.dxf` report.

- [ ] DXF.20.1 Yoke_M L14 pt 14: turn point mid straight edge. DXF.19 removed the L14 turn point at (588.36, 1680.91); confirm in the next report
- [ ] DXF.20.2 ShortSleeve_M pt 31: sew-line turn point at the hem corner on a straight edge. Comes from the Seamly2D `data-turn-points` tag; find why Seamly2D tags it
- [ ] DXF.20.3 FullSleeve_M: slit end (345.67) lies 0.21 mm from the cut-line turn point (345.88); aw.fyi calls them coincident. Seamly2D geometry; decide whether to snap or leave

## [ ] Task DXF.3 — Write DXF R13 (AC1012)

D6673 §1.2 / 4.1 names AutoCAD R13. Today `$ACADVER` is `AC1009` (R12 syntax). R13 needs entity handles, a full TABLES section, CLASSES, and OBJECTS.

Menu (user decision): **DXF-ASTM (R12)**, **DXF-ASTM (R13)**, **DXF-ASTM (CLO3D)**. CLO3D writes R13: it must be D6673-10 compliant (user decision, reverses "CLO3D stays R12"). The View menu keeps one **DXF-ASTM** item.

- [x] DXF.3.1 Study a known-good R13 file: required header variables, tables (LTYPE, LAYER, STYLE, BLOCK_RECORD), handles, OBJECTS dictionary. The repo's `seamlylayout_Skirt_ASTMD6673.dxf` says AC1012 but has no handles; libdxfrw's writer (`src/libs/vdxf/libdxfrw/libdxfrw.cpp`) was the reference
- [x] DXF.3.2 `ezdxf2dxfastm/src/r13.rs`: write `$ACADVER` `AC1012`, `$HANDSEED`, handles on every entity, subclass markers (group 100). `Drawing::version` selects the version
- [x] DXF.3.3 Write CLASSES, TABLES, BLOCK_RECORD entries, and OBJECTS
- [x] DXF.3.4 Keep CLO3D group 250 output working
- [ ] DXF.3.5 Check import in CLO3D and one other reader (e.g. ezdxf `audit`, LibreCAD). Done: ezdxf 1.4.4 `audit` 0 errors / 0 fixes; libdxfrw reads the same entity counts as R12. Open: user checks a DXF-ASTM (R13) export in CLO3D
- [x] DXF.3.6 Rust tests: header version, unique handles, required sections present (`writer_r13_test.rs`, `exports.rs` `do_export_dxf_writes_r13_when_asked`)
- [x] DXF.3.7 Update the compliance doc "Decisions" row for DXF version

## [ ] Task DXF.18 — Notches on a piece with a built-in seam allowance

A built-in seam allowance: Seamly2D's main path is the cut line, sent as the `seamline` group. Seamly2D keeps the seam allowance widths; the sew line inside is not drawn. SeamlyLayout had no width, so the slit kept its drawn depth.

- [x] DXF.18.1 `converter::notch_polylines`: read `data-seam-allowances`, one width per notch subpath; a list of the wrong length is ignored
- [x] DXF.18.2 `astm_notch::build_notches`: Seamly2D's width first, then the measured width, then the drawn depth. Slit depth = half the width
- [x] DXF.18.3 Tests: `seamly2d_width_sets_slit_depth_before_the_measured_width`, `built_in_seam_allowance_notches_use_seamly2d_widths`
- [x] DXF.18.4 Compliance doc "Notch rules"
- [ ] DXF.18.5 User check: pass results to Seamly2D.10.6


## [ ] Task DXF.8 — Re-validate

- [x] DXF.8.1 Export male_shirt as DXF-ASTM and DXF-ASTM (CLO3D) after each task
- [x] DXF.8.2 User: run the files through https://aw.fyi/dxf; save the report in `test-seamly-layout-input/`
- [ ] DXF.8.3 Record the result in the compliance doc
