# Layout Processing Guidelines

## Reference

See full documentation in [`docs/layout-docs/PROCESS LAYOUT WORKFLOW.md`](../../docs/layout-docs/PROCESS%20LAYOUT%20WORKFLOW.md)

## Pre-Processing Pipeline

| Step | Input | Function | Output | Debug File |
|------|-------|----------|--------|------------|
| 0 | `self.input_dom` | _(save raw)_ | — | `output/input_dom.svg` |
| 1 | `self.input_dom` | `flatten_dom()` | `self.flat_dom` | `output/flat1_dom.svg` |
| 2 | `self.flat_dom` | `verticalize_dom()` | `self.vertical_dom` | `output/vertical_dom.svg` |
| 3 | `self.vertical_dom` | `flatten_dom()` | `self.flat_dom` | `output/flat2_dom.svg` |
| 4 | `self.flat_dom` | `translate_dom()` | `self.translate_dom` | `output/translate_dom.svg` |
| 5 | `self.translate_dom` | `flatten_dom()` | `self.flat_dom` | `output/flat3_dom.svg` |

## Layout Engine Pipeline

| Step | Function | Purpose |
|------|----------|---------|
| 1 | `LayoutSettings::from_json()` | Parse settings JSON from SettingsModel |
| 2 | `LayoutSettings::effective_bin_px()` | Compute bin dimensions in pixels (margins, fold, unit conversion; fabric selvedge is already baked into the margins by `SettingsModel::syncFabricMarginsFromSelvedge()`) |
| 3 | `extract_piece_rects(&flat_dom_3)` | Extract bounding boxes from fully pre-processed DOM; sort by area descending |
| 4 | `pack_maxrects(bin_w, bin_h, GAP_PX, &rects)` | MaxRects bin packing — top-left fit selection, 4-way split, containment pruning; returns `(Vec<Placed>, Vec<FreeRect>)` |
| 5 | `assemble_layout_svg(...)` | Build output SVG; includes debug overlay groups (see below) |
| 6 | _(roll only)_ `trim_roll_height()` | Trim SVG height to `max_piece_bottom_y + margin_bottom_px` |
| 7 | `write_debug(&output_doc, "layout_dom")` | Save `<exe_dir>/output/layout_dom.svg` |
| 8 | Store in `self.layout_dom`; emit `layout_finished()` | Right canvas reloads from `getLayoutSvgString()` |

### Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `LAYOUT_PPI` | 96.0 px/in | Canvas resolution; never passed as a parameter |
| `GAP_PX` | 5 px | Clearance between adjacent placed pieces |

## MaxRects Algorithm

- Pieces sorted by area descending (largest first)
- **Placement selection:** top-left fit — lowest `minY`, break ties by lowest `minX`
- **Split on placement:** 2-way split of the chosen free rect (right strip + bottom strip);
  4-way split (left/right/top/bottom) of all other free rects that overlap the placed piece
- **Containment pruning:** after each placement, any free rect fully contained within another is removed
- Returns all free rects in creation order (`Vec<FreeRect>`) for the debug overlay

## Debug Overlay Groups

Both groups are **permanently included** in `self.layout_dom` and displayed in the right canvas.
They are **stripped from the export clone** before any export operation.

| Group id | Contents | Display | Export |
|----------|----------|---------|--------|
| `debug-bboxes` | Semi-transparent colored `<rect>` per placed piece slot (cycles `DEBUG_COLORS` palette) | ✅ shown | ❌ stripped |
| `debug-freerects` | Dashed border `<rect>` + bold creation-number `<text>` per free rect in creation order | ✅ shown | ❌ stripped |

`strip_debug_groups(doc)` removes both groups from an export clone; `self.layout_dom` is never modified.

## Roll Media Height Trim

Roll and fabric media: the packing bin height is the media length (paper roll: 500 in sentinel).
After packing, `trim_roll_height()` reduces the SVG to actual content height:

```
trimmed_h = max(placed.y + placed.h for all placements) + margin_bottom_px
```

Updates `<svg height>` and background `<rect height>`. Applied only when `trimmed_h < bin_h`.

## Extended Media (multisize tabs)

Every multisize tab sets `ProcessLayoutArgs::extend_media`. An individual pattern does not.

- Width first: `packing::required_bin_width` is the widest piece, each piece in its narrowest trial angle. A wider value widens the bin.
- `packing::pack_polygons_extending` packs at the media length first.
- While a piece stays unplaced, the bin grows by the stacked height of the unplaced pieces. The pack runs again.
- The loop ends: a bin as tall as every piece stacked holds them all. No piece is left out.
- `set_content_width` widens the canvas to the right. contentRect keeps its x; both side margins are kept.
- `trim_bottom` sets the canvas height. Roll and fabric end at the lowest piece. A sheet ends at the lowest piece, but not above its own length.
- Canvas, Adjust Mode and exports read the size from `layout_dom`. Exception: the paper-sheet PDF export packs the pieces again on sheets of the set size.
- Tiled paper does not extend: its candidate-width search sets the width.
- Example: jacket2 Marker, 208 pieces, 48 in paper roll: the roll grows from 500 in to about 798 in. On a 10 in roll it also widens.

## Export Preparation

Before any export (DXF, PDF, SVG, PNG):
1. Clone `self.layout_dom` (releases self borrow before signal calls)
2. Call `strip_debug_groups(&mut export_clone)` to remove display-only overlays
3. Pass the clean clone to the exporter

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
- **Debug groups are display features** — strip from exports, not from `self.layout_dom`
