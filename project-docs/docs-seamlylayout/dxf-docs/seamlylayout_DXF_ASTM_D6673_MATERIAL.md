# DXF-ASTM `Material:` piece text

## D6673-10 definition

- Identifier: `Material:<string>` in the piece system text (§4.3.1.2). Optional.
- Meaning: "a string giving the name of the material in which the piece is cut".
- New in D6673-10. D6673-01 and D6673-04 do not define it.
- No vocabulary. No rule separates material role from material identity.

## Material is not Category

- `Category:` is a piece classification, inherited from AAMA-era CAD (e.g. AccuMark).
- D6673 §4.1 names "category"; §4.3.1.2 defines no `Category:` identifier.
- SeamlyLayout writes `Category:` empty. Seamly2D stores no category.
- Evidence: `seamlylayout_DXF_ASTM_D6673_INCONSISTENCIES.md`, `seamlylayout_DXF_ASTM_D6673_from_AAMA.md`.

## Two concepts in one string

| Concept | Examples | Seamly2D source |
|---|---|---|
| Material role (textile type) | fabric, lining, interfacing, interlining | Seamly2D.7 material type |
| Material identity | 14 oz denim, fusible tricot | None |

- D6673-10 `Material:` can hold either. SeamlyLayout writes the material role.

## Current output

- Every piece: `Material:Fabric`.
- Source: `seamly_svg2ezdxf::DEFAULT_MATERIAL`, stored in `Block::material`.
- Piece text order: `Piece Name:`, `Size:`, `Category:`, `Quantity:`, `Material:`.

## Planned output

- Seamly2D.7.5: the handoff SVG carries `data-material` and a per-material quantity on each piece `<g>`.
- Layout.15: one layout tab per material.
- Layout.15.4: `Material:` takes the piece `data-material`. `Fabric` stays the default when the attribute is absent.
- A piece cut from two materials is two layout entries, one per material.
