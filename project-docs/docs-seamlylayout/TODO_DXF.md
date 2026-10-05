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
| Notch depth 2.5 and 1.25 mm; 3 notches on layer 83 (U); rule: every notch a layer 4 slit, half the seam allowance | gap | DXF.17 (done, see `TODO_COMPLETED.md`) |

Report deviations carry an inch sign but match millimetres. Not verified.

Report on `male_shirt_202610031615.dxf` (after DXF.1): miscategorized points now advisory (3 pieces). DXF.2 still open: FullSleeve_M off layer 84 by 0.5319, layer 87 by 0.9309. New advisory: 5 pieces have no densified points (PlacketUnder_M and 4 others).

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

## [ ] Task DXF.8 — Re-validate

- [ ] DXF.8.1 Export male_shirt as DXF-ASTM and DXF-ASTM (CLO3D) after each task
- [ ] DXF.8.2 User: run the files through https://aw.fyi/dxf; save the report in `test-seamly-layout-input/`
- [ ] DXF.8.3 Record the result in the compliance doc
