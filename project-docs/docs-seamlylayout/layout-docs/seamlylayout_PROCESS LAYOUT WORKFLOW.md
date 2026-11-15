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
| 4 | `pack_maxrects(bin_w, bin_h, GAP_PX, &rects)` | MaxRects bin packing with a piece-order search (see below); returns `(Vec<Placed>, Vec<FreeRect>)` |
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

Box-based packer: `crates/layout_engine/src/maxrects_order_search.rs` and `up_left_compaction.rs`.

### One pass

- Free boxes: maximal free rectangles. Free boxes can overlap.
- Each piece goes to the top-left corner of the free box that the rule picks:
  - `TopLeft` — topmost box, then leftmost.
  - `WeightedTopLeft` — lowest `8 × y + x`. A box a few px lower but far to the left wins.
- The piece, plus the gap, is cut out of every free box it overlaps: left, right, top and bottom strips.
- A free box inside another free box is removed.
- The gap is added to the right and bottom of each piece and of the bin. Every pair of pieces keeps the full gap on all sides.
- After the pass, each piece moves up, then left, until the gap or the bin edge stops it.

### Piece-order search

- Start orders: pieces sorted by area, height, width, longer side, perimeter. Each order is packed with both rules.
- Then late-acceptance hill climbing: swap two pieces, move one piece, or change the rule.
- Fixed random seed: the same input always gives the same layout.
- Layout rank, in order: least unplaced area, least height, least side-gap excess, pieces higher.
- Side-gap excess: whitespace beyond the gap left of each piece. It ends at the nearest side-by-side piece or at the bin edge.
  - Side by side: the y ranges share at least half the height of the lower piece.

| Entry point | Search |
|---|---|
| `pack_maxrects_multi_angle_lenient`, `pack_maxrects_multi_angle_extending` | Full: start orders, then hill climbing up to `SEARCH_WORK` |
| `pack_maxrects_multi_angle`, `pack_maxrects` (strict) | Start orders only: sheet assignment and the tiled width scan call it many times |

- Returns the free boxes of the chosen layout in creation order for the debug overlay.
- Progress callback: `(steps done, total steps)` of the search.

## Debug Overlay Groups

Both groups are **permanently included** in `self.layout_dom` and displayed in the right canvas.
They are **stripped from the export clone** before any export operation.

| Group id | Contents | Display | Export |
|----------|----------|---------|--------|
| `debug-bboxes` | Semi-transparent colored `<rect>` per placed piece slot (cycles `DEBUG_COLORS` palette) | ✅ shown | ❌ stripped |
| `debug-freerects` | Dashed border `<rect>` + bold creation-number `<text>` per free rect in creation order | ✅ shown | ❌ stripped |

`strip_debug_groups(doc)` removes both groups from an export clone; `self.layout_dom` is never modified.

## Roll Media Height Trim

Roll and fabric media: the packing bin height is the media length (paper roll: 500 in sentinel; fabric: `fabricHeight`, or the 500 in sentinel when 0).

`LayoutSettings::media_size_in()` sizes the canvas (`create_initial_layout_dom`) and `effective_bin_px()`. Each media reads only its own fields:

| Media | Width | Length |
|---|---|---|
| fabric | `fabricWidth` (`pageWidth` when 0), half when `fabricFolded` | `fabricHeight`, 500 in when 0 |
| paper roll | `rollWidth` | 500 in |
| paper tiled | `pageWidth` | `pageHeight` |

The settings keep the page size while fabric is the media. Fabric never reads it.
After packing, `trim_roll_height()` reduces the SVG to actual content height:

```
trimmed_h = max(placed.y + placed.h for all placements) + margin_bottom_px
```

Updates `<svg height>` and background `<rect height>`. Applied only when `trimmed_h < bin_h`.

## Oversized Pieces (individual pattern, multisize fabric)

- `process_layout` first runs `layout_utils::do_find_oversized_pieces`: pre-processing up to grain-up pieces, then `piece_divider::find_oversized_pieces`.
- It runs for an individual pattern, and for a multisize pattern on fabric: fabric does not widen. Multisize lists each size piece alone, as Marker packs it.
- A piece is oversized when no trial angle fits the content width. Paper roll and fabric limit only the width.
- Layout Mode Any: a piece too wide at its `data-rotation` angles gets quarter turns if a 90° turn fits (`piece_extractor::turn_wide_pieces_to_fit`). It is not oversized. The packer gets the same angles. Warnings report each offending piece width and the media width inside the margins. Tiled paper is never checked.
- Oversized pieces: the `oversized_pieces_found` signal, no layout. QML asks the user, then calls `process_layout_oversized(settings, "split" | "extend")` or `cancel_layout()`.
- "split": `ProcessLayoutArgs::split_oversized`. `piece_divider::divide_oversized_pieces` runs after the piece-set centring and before `translate_dom`.
  - Equal columns for a piece too wide. Each outline region in a column is one part.
  - On folded fabric with fold lines on the left edge, true fold-line pieces remain pinned to the fold. Layout Mode Any pins none. Other pieces and split parts keep at least `pieceGap` from the fold edge. Oversize checks and split sizing use that same reduced width.
  - Cutting: `geometry::half_plane_clip` (`cut_closed`, `cut_open`). Each output vertex keeps its source vertex index, so `data-turn-points` stay valid.
  - Internal paths, grainlines and reference lines (`stripe`, `plaid`) end at the cut lines. A `drill` hole goes whole to the part nearest its circle centre; `cut_positions` keeps cut lines off drill centres.
  - Labels: `pattern_label`, `piece_label` and `fold_line_label` go whole to the nearest part.
    A label not wholly inside the part outline moves to the nearest place inside it, 1/16 in clear (`label_inside_part.rs`).
    It keeps clear of other labels and of the part label place along the cut edge when it can.
    A label too large for the part shrinks in 10% steps. A label with no place at 10% stays where it is.
  - Multisize: every queued layout gets `split_oversized` (Marker, Nested, Base Size, each size, every material).
  - Nested: `divide_piece_set` cuts every size of a piece-set on the same lines, from the outline bounds of all sizes.
    Each part of the largest size starts a piece-set `<set id>_part<k>`. A part of another size joins the set in its column with the nearest outline centre.
  - `ProcessLayoutResult::divide_spec` is set only when at least one piece was divided. `is_split_layout_active` follows it.
  - Paper rolls keep the selected roll width and margins when splitting, even if a caller requests enlargement. Only the length follows the content.
  - Roll and fabric retry the remaining pieces on successive media lengths (`packing::pack_polygons_on_sheets`). Fabric keeps its fixed width.
  - Paper and fabric split layouts enable PDF, PNG, JPG, EPS, PostScript (PS), SVG, and HPGL.
    Each format exports the assembled layout.
    DXF, PDF (Tiled), and paid formats remain disabled.
    `run_layout` drops the previous `divide_spec` at the start and refreshes `is_split_layout_active` at the end.
  - Each successful fabric retry removes at least one remaining piece. No-progress and coordinate-limit stops preserve explicit unplaced-piece warnings.
- "extend": `ProcessLayoutArgs::extend_media`, as below.
- `cancel_layout` sets `is_settings_required`: QML disables every button but 'Layout Settings', Preferences and Exit. `initialize_layout` clears it.

## Extended Media (multisize tabs)

Every multisize tab sets `ProcessLayoutArgs::extend_media`. An individual pattern sets it only when the user chose "Make the media bigger".

- Paper width: `packing::required_bin_width` finds the widest required bin. A wider value widens paper rolls.
- Fabric keeps its configured width, including the folded half-width and selvedge margins. Only its height can enlarge.
- `packing::pack_polygons_extending` packs into a bin as tall as all pieces that fit its width, stacked.
- Pieces that cannot fit the fixed fabric width remain unplaced and appear in the warning popup, unless the user chose "split".
- `set_content_width` widens the canvas to the right. contentRect keeps its x; both side margins are kept.
- `trim_bottom` sets the canvas height. Roll and fabric end at the lowest piece.
- Canvas, Adjust Mode and exports read the size from `layout_dom`.
- Tiled paper does not extend: its candidate-width search sets the width.
- User-requested enlargement shows the resulting width and height in the active unit. Folded fabric reports its full configured width.
- Multiple materials collect their enlargement dimensions and unplaced warnings in one completion popup.
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
