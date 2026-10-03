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
| File is AC1009, not R13 (AC1012) | departure | DXF.3 |
| More curve points than the curves need (Yoke_M L14, PocketFlapRound_M L1) | advisory | DXF.4 |
| Pieces with no `Quantity:`, label, or grainline (CollarTopInterface, CollarBaseInterface, CuffInterface) | report table | DXF.5 |
| `Style Name:` is the output file stem with timestamp | report header | DXF.6 |
| Category, Material, Size columns blank | report table | DXF.7 |
| `Author` not `vendor;application;release #` (got `Seamly2D 26.10.314`; report on `male_shirt_202610031615.dxf`) | departure | DXF.9 (done, see `TODO_COMPLETED.md`) |

Report deviations carry an inch sign but match millimetres. Not verified.

Report on `male_shirt_202610031615.dxf` (after DXF.1): miscategorized points now advisory (3 pieces). DXF.2 still open: FullSleeve_M off layer 84 by 0.5319, layer 87 by 0.9309. New advisory: 5 pieces have no densified points (PlacketUnder_M and 4 others).

## [ ] Task DXF.3 — Write DXF R13 (AC1012)

D6673 §1.2 / 4.1 names AutoCAD R13. Today `$ACADVER` is `AC1009` (R12 syntax). R13 needs entity handles, a full TABLES section, CLASSES, and OBJECTS.

- [ ] DXF.3.1 Study a known-good R13 file: required header variables, tables (LTYPE, LAYER, STYLE, BLOCK_RECORD), handles, OBJECTS dictionary
- [ ] DXF.3.2 `ezdxf2dxfastm/src/writer.rs`: write `$ACADVER` `AC1012`, `$HANDSEED`, handles on every entity, subclass markers (group 100)
- [ ] DXF.3.3 Write CLASSES, TABLES, BLOCK_RECORD entries, and OBJECTS
- [ ] DXF.3.4 Keep CLO3D group 250 output working
- [ ] DXF.3.5 Check import in CLO3D and one other reader (e.g. ezdxf `audit`, LibreCAD)
- [ ] DXF.3.6 Rust tests: header version, unique handles, required sections present
- [ ] DXF.3.7 Update the compliance doc "Decisions" row for DXF version

## [ ] Task DXF.4 — Fewer curve points where a spline needs fewer

Advisory only. Do after DXF.1 and DXF.2: DXF.2 adds points, so retest first.

- [ ] DXF.4.1 Re-validate; if the advisory remains, drop key points the spline does not need while DXF.2.4 still passes
- [ ] DXF.4.2 Rust test on the Yoke and PocketFlapRound fixtures

## [ ] Task DXF.5 — Warn about pieces with no Quantity, label, or grainline

A piece with no `Cut N` label gets no `Quantity:`, and a piece with no grainline gets no layer 7. Both make the DXF less usable in downstream CAD.

- [ ] DXF.5.1 Rust: before DXF-ASTM export, list pieces missing `Quantity:` source, piece label, or grainline
- [ ] DXF.5.2 QML: when the list is not empty, show a dialog naming each piece and what it lacks. State that this affects the usability of the exported DXF file. Buttons: Export anyway, Cancel
- [ ] DXF.5.3 Tests: Rust detection; QML dialog shown for the three male_shirt interface pieces
- [ ] DXF.5.4 Update `dxf-docs/seamlylayout_DXF_EXPORT_WORKFLOW.md`

## [ ] Task DXF.6 — Style Name is the input file base name

Today `cxxqt_bridge/src/exports.rs` sets `style_name` from the output path stem (`male_shirt_202610031234`). User decision: use the base name of the input file (`male_shirt`).

- [ ] DXF.6.1 File import: the imported SVG's base name
- [ ] DXF.6.2 `--svg-stdin` handoff: the `--document-name` value
- [ ] DXF.6.3 No input name: keep the output file stem
- [ ] DXF.6.4 Rust test for each case
- [ ] DXF.6.5 Update the compliance doc "Style Name" row

## [ ] Task DXF.7 — Category, Material, Size piece text

Needs a separate discussion before implementation. Material is tracked as Layout.15.4 in `TODO_SEAMLYLAYOUT.md`.

- [ ] DXF.7.1 Discuss source and format for `Category:`, `Material:`, `Size:`
- [ ] DXF.7.2 Add subtasks from the decision

## [ ] Task DXF.8 — Re-validate

- [ ] DXF.8.1 Export male_shirt as DXF-ASTM and DXF-ASTM (CLO3D) after each task
- [ ] DXF.8.2 User: run the files through https://aw.fyi/dxf; save the report in `test-seamly-layout-input/`
- [ ] DXF.8.3 Record the result in the compliance doc
