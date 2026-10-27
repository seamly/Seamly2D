# AAMA interoperability

SeamlyLayout writes ANSI/AAMA-292 pattern data with Export > DXF > **DXF-AAMA (R11)**.
This page tells a receiving application what to expect.
The writer rules, the decisions and the code map are in `dxf-docs/seamlylayout_DXF_AAMA.md`.

## File

- DXF R11 (`$ACADVER` AC1009), one block and one INSERT for each piece.
- A grade rule file (`.rul`) goes with a graded export. The DXF has no grade growth.
- Geometry is dense: each contour is one polyline on layer 1, 8, 11 or 14.

## Units

- `Units:METRIC`: lengths in mm, 2 decimals. The pattern units `mm`, `cm` and unspecified write this.
- `Units:ENGLISH`: lengths in inches, 4 decimals. The pattern unit `inch` writes this.
- The unit comes from the measurement file. `Units:MM` is not written.

## Style text

Plain TEXT on layer 1, before the blocks:

| Field | Value |
|---|---|
| `Style Name:` | Pattern name, else the file name |
| `Creation Date:` | `DD-MM-YYYY`, UTC |
| `Creation Time:` | `HH:MM:SS`, UTC |
| `Author:` | `Seamly;Seamly2D;<release>` |
| `Sample Size:` | Base size of a graded export, else blank |
| `Units:` | `METRIC` or `ENGLISH` |

## Piece text

Plain TEXT on layer 1 inside each piece block. The values come from the piece data, not from the label.

| Field | Value |
|---|---|
| `Piece Name:` | Full piece name |
| `Size:` | Piece size. A `cm` pattern writes it in mm (50 cm is `500`) |
| `Annotation:` | Piece description. Omitted when empty |
| `Quantity:` | `R,L`: right and left copies. Add both for a total |
| `Material:` | Full material name, else `Fabric` |

- Label text never repeats these fields and never adds a `#` rule text. A hidden label changes nothing.
- `Category:` is not written.
- Block name: `<piece name>_<size>` for a multisize piece, `<piece name>` for an individual piece.
- Non-ASCII characters use the `\U+XXXX` escape.

## Layers

| Layer | Contents |
|---|---|
| 1 | Cutting outline and piece text |
| 2, 3 | Turn points and curve points; `# n` grade rule texts |
| 4 | Notch points, one POINT each. T, castle and U notches become V |
| 5 | Grade reference line of a graded export |
| 6 | Mirror line |
| 7 | Grainline |
| 8 | Internal lines |
| 11 | Internal cutouts |
| 14 | Sew line |

- Layers 9, 10 and 12 are empty. Layer 13 (drill holes) is not written. Layers 15 and 80 to 87 are not used.
- A `# n` text names a grade rule. The `.rul` file holds the rule table, with the header `ANSI/AAMA VERSION: 1.0.0`.

## Limits

- No ASTM D6673 certification claim. For ASTM, use Export > DXF > DXF-ASTM.
- No drill holes, no fabric codes, no mirrored-copy count.
- Quantity does not say how many copies are mirrored.
