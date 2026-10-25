<!-- MIT License: https://opensource.org/licenses/MIT -->
# DXF-AAMA export — ANSI/AAMA-292

Export > DXF > **DXF-AAMA (R11)** writes ANSI/AAMA-292 pattern data in a DXF R11 (AC1009) file, the DXF version the standard builds on.
It uses the DXF-ASTM pipeline. `DxfAstmExportOptions::aama` selects the AAMA content.

## Standard

- ANSI/AAMA-292, "Pattern Data Interchange — Data Format", American Apparel Manufacturers Association, 1993. Revised as 292A.
- Built on AutoCAD Version 11 DXF. Defines 14 layers (NIST IR 5969, 1997).
- The DXF carries no grade growth. A `# n` text names a grade rule; the rule table is a separate `.rul` file.
- ASTM D6673 replaced it and cites ANSI/AAMA-292A. D6673 kept layers 1–14 and added layers 15 and 80–87.
- The normative 292 text is not public. The rules below come from D6673, NIST IR 5969, vendor files and other AAMA writers.

## AAMA and ASTM differences in the writer

| Item | DXF-ASTM (D6673-10) | DXF-AAMA |
|---|---|---|
| `$ACADVER` | AC1009 (R12) or AC1012 (R13) | AC1009 (R11; R11 and R12 share it) |
| Contours on layers 1, 8, 11, 14 | key points; dense polyline on 84–87 | dense polyline only |
| Layers 2 and 3 | key points | key points (each is a vertex of the dense polyline) |
| Layer 6 mirror line | first two key points | first two vertices of the dense boundary; fold vertices between them dropped |
| Notches | layer 4 slit and V; 80 T, 81 castle, 83 U | layer 4 only: T, castle, U become V with their width |
| Label text | layer 15 | layer 1, without lines that read as piece text |
| Size piece text | `Size Name:` | `Size:` |
| Style text | ends with `Curve Tolerance:` and `ASTM/D13Proposal 1 Version:` | ends with `Units:` |
| `.rul` first line | `ASTM/D13 Proposal 1 VERSION:D6673-10` | `ANSI/AAMA VERSION:1.0.0` |

Same in both: layer 1 boundary and system text, `Piece Name:`, `Quantity:`, `Material:`, layer 5 grade reference line, layer 7 grainline, `# n` grade rule texts, the `.rul` body, METRIC units.

## Decisions

- **R11 (AC1009).** User decision: ANSI/AAMA-292 compliance, so the file uses the DXF version the standard builds on. R11 and R12 share AC1009, so the file structure is the DXF-ASTM (R12) structure.
- **Labels on layer 1.** AAMA has no annotation layer. The Seamly2D (`VDxfEngine::ExportAAMAText`) and Valentina AAMA writers also put label text on layer 1.
- **Labels never repeat piece text.** A label line `<identifier>:...` (`Piece Name`, `Size`, `Size Name`, `Quantity`, `Rotation`, `Flip`, `Tilt`, `Fold`, `Material`, `Category`, `Annotation`, any case) or `#...` is not written. A label cannot override the piece text or add a grade rule identifier. Seamly2D issue FashionFreedom/Seamly2D#1780 asks for the same rule.
- **Piece text comes from the piece data.** `Piece Name:`, `Size:`, `Quantity:` and `Material:` come from the piece `data-*` attributes. A hidden label changes none of them. Test: `aama_piece_text_comes_from_piece_data_not_label`.
- **`Quantity:` is `R,L`.** User decision: the DXF-ASTM form, not a total. Even total N → `N/2,N/2`; odd N, or on the fold → `N,0`. A reader that wants a total adds R and L.
- **`Size:` is in pattern units.** User decision: the `data-size` value as sent. It matches the `.rul` size list and the size labels. A millimetre multisize pattern writes `Size:500`, not `Size:50`.
- **`Units:METRIC`.** D6673 allows `METRIC` or `ENGLISH` only. `Units:MM` is not a defined value.
- **No `Category:`.** Category is AAMA piece text, but the handoff has no value for it. AccuMark uses the piece name for a blank category.
- **`.rul` version line.** `ANSI/AAMA VERSION: 1.0.0` is the header keyword of AAMA rule files in the field (FILExt RUL survey).

## Limits

- An untagged SVG (no `data-type`) keeps its generic layers. Its `<text>` stays on layer 15.
- Unicode piece names keep the `\U+XXXX` escape. A reader that does not decode it shows the escape as text.

## McNeel guide

Two McNeel forum posts give AAMA practice (implementation evidence, not the normative 292 text). This table compares them with SeamlyLayout DXF-AAMA output.

### Style and piece text

| Item | Guide | SeamlyLayout |
|---|---|---|
| `Style Name:` | Project name; best the file name | Pattern `data-name`, else the file stem |
| `Creation Date:` | `DD-MM-YYYY` | `dd-mm-yyyy`, UTC |
| `Creation Time:` | `HH:MM:SS` | `hh-mm`, UTC (the D6673 form) |
| `Author:` | System, version | `Seamly;Seamly2D;<release>` |
| `Sample Size:` | Base size of the garment | Base size of a graded export, else blank |
| `Units:` | `METRIC` (mm) or `ENGLISH` (inch) | `METRIC`; geometry in mm |
| Block name | `<piece name>_<size>` | `<svg id>_M` (CLO3D naming) |
| `Piece Name:` | Piece name, under 8 characters | Full piece name |
| `Size:` | Garment size | `data-size`, in pattern units |
| `Annotation:` | Piece description | Not written; Seamly2D sends `data-annotation` |
| `Category:` | Short classification: material, piece type | Not written (user decision) |
| `Quantity:` | `1,0` for one piece; `1,1` for a pair | `R,L` from `data-quantity` |
| `Material:` (or `Fabric:`) | Short fabric code, 2 characters | Full `data-material`, else `Fabric` |

### Layers

| Layer | Guide | SeamlyLayout |
|---|---|---|
| 0 | Empty | Empty |
| 1 | Every entity; cut outline and piece text | Same |
| 2 | Corner points; grade and turn points | Turn points; `# n` rule text |
| 3 | Curve points | Curve points; `# n` rule text |
| 4 | Notch points; notch lines go on 11 | One notch POINT each |
| 5 | Empty, optional | Grade reference line of a graded export |
| 6 | Mirror (symmetry) line | Same |
| 7 | Grainline | Same |
| 8 | Internal lines | Same |
| 9, 10, 12 | Empty, optional | Empty |
| 11 | Internal cut | Internal cutouts |
| 13 | Drill and punch holes | None: Seamly2D sends no drill holes |
| 14 | Net line (no seam allowance) | Sew line |
| 15 | First post: ASTM text notes. Second post: quality lines | Not used: AAMA defines layers 1 to 14 |
| 16 to 18 | Never | Not used |
| 19 | Modaris text notes; other readers take them on layer 1 | Not used: label text on layer 1 |

The two posts disagree on layer 15. D6673 puts text notes on 15 and quality lines on 84 to 87.

## Code

| Item | Place |
|---|---|
| AAMA content | `ezdxf2dxfastm/src/writer.rs`: `write_astm_block`, `write_contour`, `start_on_mirror_line`, `aama_notch`, `repeats_piece_text`, `style_system_text` |
| `.rul` version line | `ezdxf2dxfastm/src/grade_rule_file.rs`: `AAMA_RULE_FILE_VERSION` |
| Version | `seamly_svg2ezdxf/src/drawing.rs`: `DxfVersion::R11` |
| Bridge | `cxxqt_bridge/src/exports.rs`: `DxfStyleInfo::aama`; `lib.rs` `export_dxf`: `"aama"`, `"dxfVersion":"R11"` |
| Menu | `ExportMenu.qml` `exportDxfAamaRequested`; `Main.qml` `requestDxfExport(false, "R11", true)` |
| Tests | `writer_astm_test.rs` `aama_*`; `exports.rs` `do_export_dxf_writes_aama_r10_when_asked` |

## Sources

- NIST IR 5969 (1997), Pattern Information Model: <https://nvlpubs.nist.gov/nistpubs/Legacy/IR/nistir5969.pdf>
- ASTM D6673-10 text: `dxf-docs/D6673-10-expired.docx`
- AAMA piece text in practice (`Piece Name`, `Size`, `Annotation`, `Category`, `Quantity`, `Material`): <https://discourse.mcneel.com/t/how-to-export-dxf-aama-format/52629>
- AAMA vs ASTM (`.rul` with base size, ASTM adds notch types and text): <https://help.grafis.de/en-gb/Content/ExportIImport/Exportformate_und_deren_Besonderheiten.htm>
- `.rul` header keywords, `ANSI/AAMA VERSION: 1.0.0`: <https://filext.com/file-extension/RUL>
- Valentina AAMA writer (layers 1–4, 7, 8, 11, 13, 14 and 19; piece label text on layer 1): <https://gitlab.com/smart-pattern/valentina/-/blob/develop/src/libs/vdxf/vdxfengine.cpp>
- Seamly2D label-independent AAMA piece text: <https://github.com/FashionFreedom/Seamly2D/issues/1780>
- Category history: `dxf-docs/seamlylayout_DXF_ASTM_D6673_from_AAMA.md`
