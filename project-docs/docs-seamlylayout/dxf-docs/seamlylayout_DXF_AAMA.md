<!-- MIT License: https://opensource.org/licenses/MIT -->
# DXF-AAMA export — ANSI/AAMA-292

Export > DXF > **DXF-AAMA (R10)** writes ANSI/AAMA-292 pattern data in a DXF R10 (AC1006) file.
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
| `$ACADVER` | AC1009 (R12) or AC1012 (R13) | AC1006 (R10) |
| BLOCK group 3 | block name again | none: Release 10 does not define it |
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

- **R10.** The menu item writes R10, as asked. AAMA-292 itself names R11 DXF (AC1009). A reader that refuses AC1006 needs an R12 variant.
- **Labels on layer 1.** AAMA has no annotation layer. The Seamly2D (`VDxfEngine::ExportAAMAText`) and Valentina AAMA writers also put label text on layer 1.
- **Labels never repeat piece text.** A label line `<identifier>:...` (`Piece Name`, `Size`, `Size Name`, `Quantity`, `Rotation`, `Flip`, `Tilt`, `Fold`, `Material`, `Category`, `Annotation`, any case) or `#...` is not written. A label cannot override the piece text or add a grade rule identifier. Seamly2D issue FashionFreedom/Seamly2D#1780 asks for the same rule.
- **No `Category:`.** Category is AAMA piece text, but the handoff has no value for it. AccuMark uses the piece name for a blank category.
- **`.rul` version line.** `ANSI/AAMA VERSION: 1.0.0` is the header keyword of AAMA rule files in the field (FILExt RUL survey).

## Limits

- An untagged SVG (no `data-type`) keeps its generic layers. Its `<text>` stays on layer 15.
- Unicode piece names keep the `\U+XXXX` escape. An R10 reader shows the escape as text.

## Code

| Item | Place |
|---|---|
| AAMA content | `ezdxf2dxfastm/src/writer.rs`: `write_astm_block`, `write_contour`, `start_on_mirror_line`, `aama_notch`, `repeats_piece_text`, `style_system_text` |
| R10 header and BLOCK | `writer.rs`: `write_header_section`, `write_blocks_section` |
| `.rul` version line | `ezdxf2dxfastm/src/grade_rule_file.rs`: `AAMA_RULE_FILE_VERSION` |
| Version | `seamly_svg2ezdxf/src/drawing.rs`: `DxfVersion::R10` |
| Bridge | `cxxqt_bridge/src/exports.rs`: `DxfStyleInfo::aama`; `lib.rs` `export_dxf`: `"aama"`, `"dxfVersion":"R10"` |
| Menu | `ExportMenu.qml` `exportDxfAamaRequested`; `Main.qml` `requestDxfExport(false, "R10", true)` |
| Tests | `writer_astm_test.rs` `aama_*`; `exports.rs` `do_export_dxf_writes_aama_r10_when_asked` |

## Sources

- NIST IR 5969 (1997), Pattern Information Model: <https://nvlpubs.nist.gov/nistpubs/Legacy/IR/nistir5969.pdf>
- ASTM D6673-10 text: `dxf-docs/D6673-10-expired.docx`
- DXF Release 10 reference (BLOCK has groups 2, 70, 10–30 only): <https://paulbourke.net/dataformats/dxf/dxf10.html>
- Release 10 is AC1006: PRONOM fmt/74, <https://pronom.nationalarchives.gov.uk/fmt/74.xml>
- AAMA piece text in practice (`Piece Name`, `Size`, `Annotation`, `Category`, `Quantity`, `Material`): <https://discourse.mcneel.com/t/how-to-export-dxf-aama-format/52629>
- AAMA vs ASTM (`.rul` with base size, ASTM adds notch types and text): <https://help.grafis.de/en-gb/Content/ExportIImport/Exportformate_und_deren_Besonderheiten.htm>
- `.rul` header keywords, `ANSI/AAMA VERSION: 1.0.0`: <https://filext.com/file-extension/RUL>
- Valentina AAMA writer (layers 1–4, 7, 8, 11, 13, 14 and 19; piece label text on layer 1): <https://gitlab.com/smart-pattern/valentina/-/blob/develop/src/libs/vdxf/vdxfengine.cpp>
- Seamly2D label-independent AAMA piece text: <https://github.com/FashionFreedom/Seamly2D/issues/1780>
- Category history: `dxf-docs/seamlylayout_DXF_ASTM_D6673_from_AAMA.md`
