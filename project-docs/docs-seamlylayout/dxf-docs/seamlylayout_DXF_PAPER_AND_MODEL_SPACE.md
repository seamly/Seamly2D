<!-- MIT License: https://opensource.org/licenses/MIT -->
# DXF paper space and model space

Research and plan for Layout.90 (paper space) and Layout.91 (model space). Nothing is implemented. Read, then choose one option per space.

## What ASTM D6673-10 says

- No clause names paper space or model space.
- §3.2.1: style system text goes in the ENTITIES section. In R13, ENTITIES holds the model space entities.
- §4.1: a file has an optional header, then tables, blocks and entities.
- §4.2: keep the optional header to a minimum; many CAD programs cannot read or write one.
- §4.3.1.3: one BLOCK per piece. A BLOCK holds no INSERT of another block.
- §4.3.1.5: blocks can sit on top of each other (a stack of sizes) or each in its own coordinate space.
- ANSI/AAMA-292 builds on R11 DXF. Its rules use BLOCKS and ENTITIES only.

## What the export writes now

| Item | DXF-ASTM (R12), DXF-AAMA (R11) | DXF-ASTM (R13), (CLO3D) |
|---|---|---|
| Model space | ENTITIES: style system text on layer 1, then one INSERT per piece block at (0, 0). Block coordinates are the layout positions | Same |
| Paper space | none: R11 and R12 need none | BLOCK_RECORD and empty `*MODEL_SPACE` and `*PAPER_SPACE` blocks: the structure R13 requires. No viewport |
| Header | `$ACADVER` | `$ACADVER`, `$DWGCODEPAGE`, `$HANDSEED` |

Code: `ezdxf2dxfastm/src/writer.rs` `write_entities_section`; `ezdxf2dxfastm/src/r13.rs`.

## Options

### Paper space (Layout.90)

| Option | Change | For | Against |
|---|---|---|---|
| P1 (recommended) | None | D6673 needs nothing there. CLO3D, AccuMark and aw.fyi read the blocks and the model space INSERTs. PDF covers printing | A general CAD viewer opens model space, not a print sheet |
| P2 | R13 only: one paper space VIEWPORT that shows the whole layout, and the header `$PLIMMIN` / `$PLIMMAX` at the media size | AutoCAD-type viewers open a print-ready sheet | A larger header (§4.2). A reader that counts entities can take the viewport for a piece. R11/R12 and DXF-AAMA stay as they are |

### Model space (Layout.91)

| Option | Change | For | Against |
|---|---|---|---|
| M1 (recommended) | None | The model space already holds what D6673 asks: style system text and the piece INSERTs | A viewer computes the drawing extents itself |
| M2 | Header `$EXTMIN` / `$EXTMAX` (box around the pieces) and `$LIMMIN` / `$LIMMAX` (the media) in every DXF export | A viewer opens zoomed to the pieces; the media size travels with the file | Four more header variables (§4.2). Values must stay correct for a graded export, where blocks hold one size |
| M3 | Place each block at its layout position with the INSERT point, and write block coordinates around the block's own origin | Cleaner blocks: a reader that ignores INSERT sees each piece at the origin | Every reader that now reads block coordinates as placed sees all pieces at the origin. Large change to every DXF test |

## Plan, after a choice

- P1 and M1: close Layout.90 and Layout.91 with no code change; note the decision in `seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`.
- P2: `r13.rs` writes the VIEWPORT entity in `*PAPER_SPACE` and the two header variables; tests in `writer_r13_test.rs`; check with ezdxf `audit` and aw.fyi.
- M2: `writer.rs` header for R12/R11 and `r13.rs` header for R13; extents from the block contours; tests in `writer_astm_test.rs` and `writer_r13_test.rs`; check with aw.fyi and CLO3D.
- M3: not recommended.
