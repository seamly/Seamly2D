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
| FrontPanel_M L1 + L14: line→curve junction not a turn point (DXF.15 repeat); report on `male_shirt_R13_202610071908.dxf` | departure | DXF.19 (done, see `TODO_COMPLETED.md`) |
| FullSleeve_M: 10 L2 points at pleat (L8) and slit (L11) ends "not needed"; 2 hem run ends "should be curve" (DXF.16 repeat) | departure | DXF.19 (done, see `TODO_COMPLETED.md`) |
| CollarBaseInterface_M, CollarBase_M: turn point on a 1.7° curve | advisory | DXF.19 (done, see `TODO_COMPLETED.md`) |
| Yoke_M L14: turn point mid straight edge | advisory | DXF.20 (done, see `TODO_COMPLETED.md`) |
| ShortSleeve_M pt 31: sew-line turn point at hem corner on a straight edge | advisory | DXF.20 (done, see `TODO_COMPLETED.md`) |
| FullSleeve_M: slit end 0.21 mm from cut-line turn point | advisory | DXF.20 (done, see `TODO_COMPLETED.md`) |
| Notch depth 2.5 and 1.25 mm; 3 notches on layer 83 (U); rule: every notch a layer 4 slit, half the seam allowance | gap | DXF.17 (done, see `TODO_COMPLETED.md`) |
| FullSleeve_M: 10 L3 points at pleat (L8) and slit (L11) ends "not needed – definite" (DXF.19 repeat, now as curve points); report on `male_shirt_R13_202610101837.dxf` | departure | DXF.21 |
| FrontPanel_M L1 pt 3: line→curve junction turn point "should be curve"; L14 pt 39, same geometry, agrees | departure | DXF.22 |
| FullSleeve_M L1 pt 1 / L14 pt 36: hem turn point "maybe curve"; ShortSleeve_M L14 pt 31 "maybe" | advisory | No action (DXF.20.2) |

Report deviations carry an inch sign but match millimetres. Not verified.

Report on `male_shirt_202610031615.dxf` (after DXF.1): miscategorized points now advisory (3 pieces). DXF.2 still open: FullSleeve_M off layer 84 by 0.5319, layer 87 by 0.9309. New advisory: 5 pieces have no densified points (PlacketUnder_M and 4 others).

## [x] Task DXF.21 — A straight internal line declares no turn or curve points

aw.fyi report on `male_shirt_R13_202610101837.dxf` (folder `test-seamly-layout-input/male_shirt_202610101837/`): FullSleeve_M, 10 of 80 declared points miscategorized. They are both ends of the 4 pleat lines (L8) and the slit (L11), each a 2-vertex straight line. The 2026-10-07 report flagged the same ends as layer 2 points; this one flags them as layer 3 points. Both "not needed – definite".

- [x] DXF.21.1 `AstmContour::declares_points`: an open contour with 2 key points writes nothing on layers 2 and 3. Slit ends are never turn points (user decision)
- [x] DXF.21.2 `writer.rs` `write_astm_block` skips those contours on layers 2 and 3
- [x] DXF.21.3 Tests: `straight_open_line_declares_no_points`; `writer_astm_test.rs` layer 3 count and male_shirt declared-point count
- [x] DXF.21.4 Compliance doc "Contour rules"
- [ ] DXF.21.5 User: re-export male_shirt DXF-ASTM (R13); run aw.fyi (DXF.8)
- Checked on `male_shirt_202610101837.svg`: FullSleeve_M layer 3 70 → 60 points. No other block changed.

## [ ] Task DXF.22 — FrontPanel L1 junction: aw.fyi contradicts itself

Same report. FrontPanel_M top edge, where the straight line meets the curve (L1 71.10, 1068.59; L14 71.35, 1058.59).

| Report | L1 junction | aw.fyi | L14 junction | aw.fyi |
|---|---|---|---|---|
| 2026-10-07 | not a key point | turn point missing | curve point | should be turn |
| 2026-10-10 | turn point | should be curve | turn point | agree |

- Geometry is the same on both lines: tangent break 2.79° (L1) / 2.85° (L14) at the junction, 1.15° / 1.05° at the next dense vertex. Key-point angles 4.86° / 6.29° on both. Arc tangent through the next two key points: 1.57° / 1.62°.
- No code change: the turn point stays. Any rule that changes L1 also changes L14.
- [ ] DXF.22.1 User: report the contradiction to ApparelWerks (rick@apparelwerks.com). Until he replies, accept it as a reader advisory (user decision)

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
- [x] DXF.8.3 Record the result in the compliance doc ("aw.fyi results")
- [ ] DXF.8.4 User: re-run aw.fyi after DXF.21 and Layout.61
