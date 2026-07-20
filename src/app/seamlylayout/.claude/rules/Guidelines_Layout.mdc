# Layout Processing Guidelines

## Reference

See full documentation in [`docs/layout-docs/PROCESS LAYOUT WORKFLOW.md`](../../docs/layout-docs/PROCESS%20LAYOUT%20WORKFLOW.md)

## Pre-Processing Pipeline

| Step | Input | Function | Output | Debug File |
|------|-------|----------|--------|------------|
| 1 | inputDom | `flatten_dom()` | flatDom_1 | `output/flatDom_1.svg` |
| 2 | flatDom_1 | `verticalize_dom()` | verticalDom | `output/verticalDom.svg` |
| 3 | verticalDom | `flatten_dom()` | flatDom_2 | `output/flatDom_2.svg` |
| 4 | flatDom_2 | `translate_dom()` | translatedDom | `output/translatedDom.svg` |
| 5 | translatedDom | `flatten_dom()` | flatDom_3 | `output/flatDom_3.svg` |

## Layout Engine Pipeline

| Step | Function | Purpose |
|------|----------|---------|
| 1 | `read_layout_settings()` | Load from `layoutSettings.json` |
| 2 | `gather_pieces()` | Extract pieces from flatDom_3 |
| 3 | `place_pieces()` | Arrange on media → layoutDom |
| 4 | `save_dom()` | Save to `output/layoutDom.svg` |

## 3D Export Pipeline

1. Extract per-piece geometry and layout IDs from layoutDom
2. Tessellate curves → polylines (small chord tolerance)
3. Triangulate pieces using hole-aware earcut
4. Write single 3MF mesh with metadata mapping
5. Open in viewer (online default or OS fallback)

## Key Constraints

- **Always save intermediate DOMs** to `output/` for debugging
- **Flatten after each transform** to bake in changes
- **Layout IDs** must map vertex/triangle ranges in 3MF