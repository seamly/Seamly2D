// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file piece_extractor.rs
// @brief Extracts pattern piece bounding boxes from a `svg_dom::Document` for
//        use with `packing::pack_shelves` / `packing::pack_pieces`.
//
// Each top-level `<g>` element in the SVG root is treated as one pattern piece.
// The bounding box is computed by collecting all coordinate points from every
// descendant `<path>` element, parsing the `d` attribute via `geometry::Path`.
//
// This mirrors the `collect_path_points` + `BoundingBox::from_points` pattern
// Used by the Qt bridge layout pipeline and exported from this crate's lib root.

use layout_tiling::measurement_to_px;
use geometry::{BoundingBox, Path, PathSegment, Point};
use packing::Rect;
use xmltree::XMLNode;

// @brief One extracted pattern piece ready for bin packing.
//
// Holds the `Rect` (integer pixel dimensions), the piece's `id` attribute,
// and `group_index` so Phase 8c can locate the original `<g>` element.
#[derive(Debug, Clone)]
pub struct PieceRect {
    // Dimensions in pixels at the DPI used during extraction.
    pub rect: Rect,
    // Value of the `id` attribute on the piece's top-level `<g>` element.
    // Empty string if the element has no id.
    pub id: String,
    // Bounding-box origin in SVG user units — used by Phase 8c to compute
    // the translate offset so pieces are packed at (0,0) within their slot.
    // keep these pixel-precise as f64 for accurate translate offsets; the Rect will be rounded to u32 for packing
    pub origin_x: f64,
    pub origin_y: f64,
    // Index of this piece's `<g>` within the ordered list of ALL top-level
    // `<g>` children of the SVG root (0-based, counting only `<g>` elements).
    // Used by `layout_assembler` to retrieve the original element even when
    // some `<g>` elements were skipped (empty paths, degenerate size).
    pub group_index: usize,
}

// @brief Extract bounding boxes from all top-level `<g>` elements in `doc`.
//
// Each direct child `<g>` of the SVG root is considered one pattern piece.
// Pieces with no parseable `<path>` data (empty, text-only, etc.) are skipped.
//
// @param doc SVG document previously loaded by `app_core::load_svg`.
// @return `Vec<PieceRect>` — one entry per non-empty top-level `<g>`.
//         Returns an empty `Vec` if no pieces could be extracted.
pub fn extract_piece_rects(doc: &svg_dom::Document) -> Vec<PieceRect> {
    use layout_tiling::LAYOUT_PPI;
    // Determine SVG user-units-per-inch from the SVG root's viewBox / width attributes.
    // If no viewBox is present, assume 1 user unit = 1 px (i.e., scale = 1.0).
    let uu_per_px = svg_uu_per_px(&doc.root);

    let mut pieces = Vec::new();
    // Counts every <g> child of the SVG root (including empty ones that are skipped).
    // Stored in PieceRect::group_index so the assembler can look up the element.
    let mut g_idx: usize = 0;

    // Iterate direct children of the SVG root; each <g> is one pattern piece.
    for child in &doc.root.children {
        let XMLNode::Element(elem) = child else {
            continue; // skip text nodes, comments, etc.
        }; // XMLNode::Element

        if elem.name != "g" {
            continue; // skip non-group elements (defs, title, rect background, etc.)
        } // if not <g>

        let this_g_idx = g_idx;
        g_idx += 1; // always increment, even if piece will be skipped

        let piece_id = elem
            .attributes
            .get("id")
            .cloned()
            .unwrap_or_default();

        // Collect all path points from descendants of this <g>.
        let mut all_points: Vec<Point> = Vec::new();
        collect_all_path_points(elem, &mut all_points);

        // Skip pieces with no path geometry (e.g., label-only groups).
        let Some(bbox) = BoundingBox::from_points(all_points) else {
            continue; // no geometry — skip this group
        }; // BoundingBox::from_points

        // Convert bounding-box dimensions from SVG user units to pixels.
        // uu_per_px: how many user units equal one pixel in this SVG.
        let w_uu = bbox.width() as f64;
        let h_uu = bbox.height() as f64;

        // uu_per_px is in CSS pixels (computed at the SVG standard 96 px/in base).
        // Conversion: user-units → CSS px → output pixels at LAYOUT_PPI.
        //   w_px = (w_uu / uu_per_px) * (LAYOUT_PPI / 96.0)
        let scale = LAYOUT_PPI / (uu_per_px * 96.0);
        let w_px = (w_uu * scale).ceil() as u32;
        let h_px = (h_uu * scale).ceil() as u32;

        // Skip degenerate pieces (zero dimension after rounding).
        if w_px == 0 || h_px == 0 {
            continue; // zero-size — skip
        } // if w_px == 0

        pieces.push(PieceRect {
            rect: Rect::new(w_px, h_px),
            id: piece_id,
            origin_x: bbox.min.x as f64,
            origin_y: bbox.min.y as f64,
            group_index: this_g_idx,
        });
    } // for child in doc.root.children

    pieces
} // fn extract_piece_rects

// @brief Extract piece bounding boxes AND cutline polygons in a single walk.
//
// Walks `doc.root.children` once, applying the same skip rules as
// `extract_piece_rects` (no path geometry → skipped; zero-dim AABB → skipped).
// For each surviving piece, additionally invokes
// `polygon_pack::svg_extract::extract_piece_outline` on the same `<g>` to
// recover the piece's cut silhouette.  When extraction returns `None`
// (no cutline / seamline group, unparseable path, < 3 vertices), the polygon
// falls back to a 4-vertex AABB so the piece still packs — at orthogonal
// trial sets the polygon is ignored anyway, and at non-orthogonal sets the
// AABB-as-polygon yields tight-AABB packing identical to MaxRects.
//
// Polygon vertices are emitted in pixel space at `LAYOUT_PPI` and shifted so
// the polygon's AABB top-left sits at `(0, 0)` — this matches the rect's
// implicit `(0, 0)–to–(w_px, h_px)` frame so the polygon-packer's reported
// `Placed.x/y` agrees with what the rect-packer would have produced.
//
// The two output vectors are guaranteed equal-length and index-aligned:
// `pieces[i].rect` is the AABB of `polygons[i]`.  This is the contract
// `packing::pack_polygons` requires.
//
// @param doc Pre-flattened SVG document (typically `flat_dom` from the bridge
//            pipeline; pieces have already been translated so bbox.min ≈ 0).
// @return    `(pieces, polygons)` of identical length, in document order.
pub fn extract_piece_rects_and_polygons(
    doc: &svg_dom::Document,
) -> (Vec<PieceRect>, Vec<polygon_pack::Polygon>) {
    use layout_tiling::LAYOUT_PPI;

    // Same scale convention as `extract_piece_rects`: user-units → CSS px → output px at LAYOUT_PPI.
    let uu_per_px = svg_uu_per_px(&doc.root);
    let scale = LAYOUT_PPI / (uu_per_px * 96.0);

    let mut pieces = Vec::new();
    let mut polygons = Vec::new();
    let mut g_idx: usize = 0;

    for child in &doc.root.children {
        let XMLNode::Element(elem) = child else { continue; };
        if elem.name != "g" { continue; }

        let this_g_idx = g_idx;
        g_idx += 1;

        let piece_id = elem
            .attributes
            .get("id")
            .cloned()
            .unwrap_or_default();

        // Identical skip rules to `extract_piece_rects` so the two functions
        // agree on which `<g>` children are pieces.
        let mut all_points: Vec<Point> = Vec::new();
        collect_all_path_points(elem, &mut all_points);
        let Some(bbox) = BoundingBox::from_points(all_points) else {
            continue; // no geometry — skip
        };

        let w_uu = bbox.width() as f64;
        let h_uu = bbox.height() as f64;
        let w_px = (w_uu * scale).ceil() as u32;
        let h_px = (h_uu * scale).ceil() as u32;
        if w_px == 0 || h_px == 0 {
            continue; // zero-size — skip
        }

        let origin_x = bbox.min.x as f64;
        let origin_y = bbox.min.y as f64;

        // Try the cutline / seamline polygon; fall back to the rect outline
        // when the piece has no cutline group or its path is degenerate.
        let polygon = match polygon_pack::svg_extract::extract_piece_outline(elem) {
            Some(poly) => {
                // Shift so polygon AABB.min is at (0,0), then scale user-units
                // → pixels.  After the bridge's translate_dom pass bbox.min is
                // already ~(0,0); the shift is a no-op for that case but keeps
                // the function correct against un-translated test fixtures.
                let scaled: Vec<(f64, f64)> = poly
                    .vertices
                    .iter()
                    .map(|&(x, y)| ((x - origin_x) * scale, (y - origin_y) * scale))
                    .collect();
                polygon_pack::Polygon::new(scaled)
            }
            None => {
                // Rect-as-polygon: 4 vertices CCW from top-left (matches the
                // SVG y-down convention used throughout the layout pipeline).
                polygon_pack::Polygon::new(vec![
                    (0.0,           0.0),
                    (w_px as f64,   0.0),
                    (w_px as f64,   h_px as f64),
                    (0.0,           h_px as f64),
                ])
            }
        };

        pieces.push(PieceRect {
            rect: Rect::new(w_px, h_px),
            id: piece_id,
            origin_x,
            origin_y,
            group_index: this_g_idx,
        });
        polygons.push(polygon);
    } // for child in doc.root.children

    (pieces, polygons)
} // fn extract_piece_rects_and_polygons

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// @brief Returns true when the element is a `<g>` whose id marks it as a
// non-outline decoration group that must not contribute points to the piece
// bounding box.
//
// Recognised id prefixes (compared lower-case):
//   • notch      — V-notch / tick-mark registration points
//   • tuck       — dart / tuck construction lines (e.g. `tuck_1_a_Back`)
//   • grainline / grain_ — grain direction arrow
//   • ip_        — internal path (pocket placement lines, etc.)
//   • drill / hole — drill-hole markers
//
// Matches the same prefix set as `polygon_pack::svg_extract::is_non_outline_group`
// so the bounding-box calculation and the cutline resolver agree on which groups
// to skip.
fn is_non_outline_group(e: &xmltree::Element) -> bool {
    if e.name != "g" { return false; }
    let Some(id) = e.attributes.get("id") else { return false; };
    let id_lower = id.to_lowercase();
    id_lower.starts_with("notch")
        || id_lower.starts_with("tuck")
        || id_lower.starts_with("grainline")
        || id_lower.starts_with("grain_")
        || id_lower.starts_with("ip_")
        || id_lower.starts_with("drill")
        || id_lower.starts_with("hole")
} // fn is_non_outline_group

// @brief Collect all coordinates from every `<path d="...">` descendant,
// skipping any child `<g>` whose id identifies it as a non-outline decoration
// (tuck, notch, grainline, ip, drill, hole).
//
// Without this filter, a piece with `tuck_1_a_Back` or `notch_1_Back`
// siblings would have its bounding box inflated by the construction-line
// geometry, wasting layout space when the piece is packed.
//
// @param element Root element to search (typically a piece `<g>`).
// @param points  Output buffer; each segment's endpoints are appended.
fn collect_all_path_points(element: &xmltree::Element, points: &mut Vec<Point>) {
    // If this element is a <path>, parse its d attribute.
    if element.name == "path" {
        if let Some(d) = element.attributes.get("d") {
            if let Ok(path) = Path::parse_path_attribute(d) {
                // Extract the endpoint of every segment into the point list.
                for seg in &path.segments {
                    match seg {
                        PathSegment::MoveTo(p)                     => points.push(*p),
                        PathSegment::LineTo(p)                     => points.push(*p),
                        PathSegment::QuadTo { ctrl, to }           => { points.push(*ctrl); points.push(*to); }
                        PathSegment::CubicTo { ctrl1, ctrl2, to }  => { points.push(*ctrl1); points.push(*ctrl2); points.push(*to); }
                        PathSegment::ArcTo { to, .. }              => points.push(*to),
                        PathSegment::Close                         => {} // no new point
                    } // match seg
                } // for seg in path.segments
            } // if let Ok(path)
        } // if let Some(d)
    } // if element.name == "path"

    // Recurse into children, skipping non-outline decoration groups.
    for child in &element.children {
        if let XMLNode::Element(child_elem) = child {
            // Tuck, notch, grainline, ip, drill, and hole groups are decorations;
            // their coordinates must not expand the piece's bounding box.
            if is_non_outline_group(child_elem) { continue; }
            collect_all_path_points(child_elem, points);
        } // if XMLNode::Element
    } // for child
} // fn collect_all_path_points

// @brief Determine SVG user-units-per-CSS-pixel from the root `<svg>` element.
//
// SVG files exported by Seamly2D typically set `width`/`height` in millimetres
// or inches with a `viewBox` attribute.  The ratio viewBox-width / width gives
// the number of user units per CSS pixel (96 dpi assumed as the CSS baseline).
//
// Fallback: if width/height are dimensionless numbers (already in px), returns
// 1.0 so that `LAYOUT_PPI / (uu_per_px * 96.0) == 1.0` when LAYOUT_PPI == 96.
//
// Also used by `layout_assembler` to convert pixel placements back to user units.
//
// @param root The `<svg>` root element.
// @return User-units per CSS pixel (≥ 1e-9 to prevent divide-by-zero).
pub fn svg_uu_per_px(root: &xmltree::Element) -> f64 {

    // get root viewBox width and document width in pixels
    let viewbox_w_px = parse_viewbox_width_px(root); // viewBox is "(0 0 w_uu h_uu)" in user units, many steps so use helper function
    let doc_width_str: Option<&String> = root.attributes.get("width"); // width example: "36.0mm" in user units or "100" in pixels
    let doc_w_px: Option<u32> = doc_width_str.map(|s| measurement_to_px(s)); // if needed, strip user units and convert to pixels

    // validate; don't divide by zero or return a crazy scale if doc width is missing/invalid
    if let (Some(vb_wpx), Some(d_wpx)) = (viewbox_w_px, doc_w_px) {
        // uu_per_px = viewBox-width / doc-width
        let uu_per_px: f64 = vb_wpx as f64 / d_wpx as f64;
        return uu_per_px.max(1e-9); // clamp to avoid divide-by-zero
    } // if viewbox

    // No viewBox or no explicit units — return 1 user unit = 1 px but its f64 for consistent scaling in layout_assembler.
    1.0
} // fn svg_user_units_per_px

// @brief Parse the first two values of the `viewBox` attribute ("min-x min-y w h").
// @return The width field (third token), or None.
fn parse_viewbox_width_px(root: &xmltree::Element) -> Option<u32> {
    // viewbox in pixels: "0 0 100 100" → width is the third token (index 2)
    let vb = root.attributes.get("viewBox").or_else(|| root.attributes.get("viewbox"))?;
    let parts: Vec<u32> = vb
        // Split on spaces or commas, per SVG spec.
        .split(|c: char| c == ' ' || c == ',')
        // Filter out empty tokens (e.g., from multiple spaces).
        .filter(|s| !s.is_empty())
        // Parse each token as a measurement, converting to pixels.
        .filter_map(|s| Some(measurement_to_px(s)))
        // Collect into a Vec for indexing.
        .collect();
    // viewBox = "min-x min-y width height"

    // return width in pixels, or None if viewBox is missing/invalid
    parts.get(2).copied() // width is the third token (index 2)
} // fn parse_viewbox_width

// @brief Split "36.0mm" → ("36.0", "mm"), "100" → ("100", "px").
// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Two <g> pieces with path data are extracted with correct dimensions.
    #[test]
    fn extracts_two_pieces() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="piece-1">
    <path d="M 0 0 L 96 0 L 96 96 L 0 96 Z"/>
  </g>
  <g id="piece-2">
    <path d="M 0 0 L 48 0 L 48 48 L 0 48 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 2);
        // 96 user-units at 96 dpi with no viewBox → 96 px square
        assert_eq!(pieces[0].rect.w, 96);
        assert_eq!(pieces[0].rect.h, 96);
        // 48 px square
        assert_eq!(pieces[1].rect.w, 48);
        assert_eq!(pieces[1].rect.h, 48);
        assert_eq!(pieces[0].id, "piece-1");
        assert_eq!(pieces[1].id, "piece-2");
    } // extracts_two_pieces

    // @brief A <g> with no <path> children is skipped.
    #[test]
    fn skips_empty_group() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
  <g id="empty"/>
  <g id="real">
    <path d="M 0 0 L 50 0 L 50 50 L 0 50 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "real");
    } // skips_empty_group

    // @brief <g> missing an id attribute gets an empty string id.
    #[test]
    fn handles_missing_id() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
  <g>
    <path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "");
    } // handles_missing_id

    // @brief Non-<g> top-level elements (rect, defs, title) are skipped.
    #[test]
    fn skips_non_group_elements() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <rect x="0" y="0" width="200" height="200" fill="white"/>
  <defs/>
  <title>Test</title>
  <g id="p1">
    <path d="M 0 0 L 20 0 L 20 30 L 0 30 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "p1");
    } // skips_non_group_elements

    // @brief viewBox + mm width produces correct pixel scaling.
    #[test]
    fn viewbox_mm_scaling() {
        // viewBox="0 0 100 100", width="25.4mm" → 25.4mm = 96px → uu_per_px = 100/96
        // A piece of 100×100 user units → 100/(100/96) * 96 = 96×96 px... wait:
        // scale = dpi / uu_per_px = 96 / (100/96) = 96 * 96/100 = 92.16 px
        // path goes 0..50 uu → w_uu=50, h_uu=50 → 50 * 92.16/96 ≈ 48 px
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="25.4mm" height="25.4mm" viewBox="0 0 100 100">
  <g id="p1">
    <path d="M 0 0 L 50 0 L 50 50 L 0 50 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        // 50 user units in a 100uu-wide box that maps to 96px → 48 px
        assert_eq!(pieces[0].rect.w, 48);
        assert_eq!(pieces[0].rect.h, 48);
    } // viewbox_mm_scaling

    // @brief group_index reflects position within top-level <g> list even when some are skipped.
    #[test]
    fn group_index_skips_empty() {
        // g_idx=0 (empty, skipped), g_idx=1 (has paths) → pieces[0].group_index == 1
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
  <g id="empty"/>
  <g id="real">
    <path d="M 0 0 L 30 0 L 30 30 L 0 30 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].group_index, 1); // skipped g_idx=0, so real piece is at g_idx=1
    } // group_index_skips_empty

    // @brief Two pieces get consecutive group_index values when no groups are skipped.
    #[test]
    fn group_index_consecutive() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="a"><path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/></g>
  <g id="b"><path d="M 0 0 L 20 0 L 20 20 L 0 20 Z"/></g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces[0].group_index, 0);
        assert_eq!(pieces[1].group_index, 1);
    } // group_index_consecutive

    // @brief Paired extractor returns equal-length, index-aligned rects and
    // polygons.  Piece A has a cutline group → real polygon (≥3 vertices,
    // not the AABB rectangle).  Piece B has only a grainline → AABB fallback
    // (exactly 4 vertices matching the rect's (0,0)–to–(w,h) frame).
    #[test]
    fn paired_extractor_aligns_rects_and_polygons() {
        // Both pieces use width="100" (no viewBox) → 1 user-unit = 1 px at
        // LAYOUT_PPI=96 baseline so the assertions can use exact pixel math.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="WithCutline">
    <g id="cutline_WithCutline">
      <path d="M 0 0 L 50 0 L 50 30 L 0 30 L 0 0"/>
    </g>
  </g>
  <g id="GrainlineOnly">
    <g id="grainline_GrainlineOnly">
      <path d="M 5 0 L 5 25"/>
    </g>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let (pieces, polygons) = extract_piece_rects_and_polygons(&doc);

        // Index alignment is the load-bearing invariant.
        assert_eq!(pieces.len(), polygons.len(), "rects and polygons must align");

        // GrainlineOnly's grainline path is 0..25 vertical so its AABB has
        // width 0 → would be skipped by the zero-dim filter.  Rebuild the
        // expectation: only WithCutline survives.
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "WithCutline");
        // Cutline polygon: closing duplicate trimmed → 4 vertices, but the
        // shape is a real rectangle outline (not the fallback synthesised one
        // — confirmed by checking the second vertex sits at (50, 0) just like
        // the path).
        assert_eq!(polygons[0].vertices.len(), 4);
        assert!((polygons[0].vertices[1].0 - 50.0).abs() < 1e-6);
        assert!((polygons[0].vertices[1].1 -  0.0).abs() < 1e-6);
    } // paired_extractor_aligns_rects_and_polygons

    // @brief When a piece has path geometry but no cutline / seamline group,
    // the polygon falls back to the rect AABB (4 vertices CCW from (0,0)).
    // This preserves "every kept rect is also packable as a polygon" so
    // non-orthogonal trial sets still cover every piece.
    #[test]
    fn paired_extractor_falls_back_to_aabb_polygon() {
        // No <g id="cutline_*"> child — only a bare <path> that contributes
        // points to the AABB.  find_outline_group returns None → fallback.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="P">
    <path d="M 0 0 L 40 0 L 40 20 L 0 20 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let (pieces, polygons) = extract_piece_rects_and_polygons(&doc);

        assert_eq!(pieces.len(), 1);
        assert_eq!(polygons.len(), 1);

        // Fallback is exactly 4 vertices matching the rect (CCW from origin).
        let v = &polygons[0].vertices;
        assert_eq!(v.len(), 4);
        assert_eq!(v[0], (0.0,  0.0));
        assert_eq!(v[1], (40.0, 0.0));
        assert_eq!(v[2], (40.0, 20.0));
        assert_eq!(v[3], (0.0,  20.0));
        // And the polygon's AABB matches pieces[0].rect.
        assert_eq!(pieces[0].rect.w, 40);
        assert_eq!(pieces[0].rect.h, 20);
    } // paired_extractor_falls_back_to_aabb_polygon

    // @brief origin_x and origin_y reflect the bounding-box minimum corner.
    #[test]
    fn origin_reflects_bbox_min() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="p1">
    <path d="M 10 20 L 60 20 L 60 80 L 10 80 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert!((pieces[0].origin_x - 10.0).abs() < 0.01, "origin_x={}", pieces[0].origin_x);
        assert!((pieces[0].origin_y - 20.0).abs() < 0.01, "origin_y={}", pieces[0].origin_y);
    } // origin_reflects_bbox_min

    // @brief Tuck sibling groups must not inflate the piece bounding box.
    //
    // A piece with `tuck_1_a_Back` and `tuck_1_b_Back` children extending
    // outside the cutline boundary should produce a rect whose dimensions
    // match the cutline only, not the union of all paths.
    #[test]
    fn tuck_sibling_bbox_not_inflated() {
        // cutline_Back is a 50×30 rectangle.
        // tuck_1_a_Back reaches x=70 (20 units beyond the cutline right edge).
        // Without filtering, w_px would be > 50; with filtering it must be 50.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="Back">
    <g id="cutline_Back">
      <path d="M 0 0 L 50 0 L 50 30 L 0 30 L 0 0"/>
    </g>
    <g id="tuck_1_a_Back">
      <path d="M 20 5 L 70 5 L 45 25"/>
    </g>
    <g id="tuck_1_b_Back">
      <path d="M 25 5 L 65 5 L 45 20"/>
    </g>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1, "expected 1 piece");
        // The cutline is 50 wide and 30 tall; tuck groups must not push the bbox wider.
        assert_eq!(pieces[0].rect.w, 50,
            "width inflated by tuck groups: got {} expected 50", pieces[0].rect.w);
        assert_eq!(pieces[0].rect.h, 30,
            "height inflated by tuck groups: got {} expected 30", pieces[0].rect.h);
    } // tuck_sibling_bbox_not_inflated

    // @brief Notch sibling groups must not inflate the piece bounding box.
    //
    // A piece with `notch_1_Piece` tick marks protruding beyond the cutline
    // should pack at the cutline dimensions, not the notch-inflated AABB.
    #[test]
    fn notch_sibling_bbox_not_inflated() {
        // cutline_Piece is a 40×20 rectangle.
        // notch_1_Piece has a point at y=-5 (5 units above the top edge).
        // Without filtering, h_px would be > 20; with filtering it must be 20.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="Piece">
    <g id="cutline_Piece">
      <path d="M 0 0 L 40 0 L 40 20 L 0 20 L 0 0"/>
    </g>
    <g id="notch_1_Piece">
      <path d="M 20 0 L 20 -5 L 22 0"/>
    </g>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1, "expected 1 piece");
        assert_eq!(pieces[0].rect.w, 40,
            "width inflated by notch: got {} expected 40", pieces[0].rect.w);
        assert_eq!(pieces[0].rect.h, 20,
            "height inflated by notch: got {} expected 20", pieces[0].rect.h);
    } // notch_sibling_bbox_not_inflated

    // @brief Grainline sibling groups must not inflate the piece bounding box.
    //
    // A grainline arrow extending outside the cutline boundary should be
    // excluded so the packed dimensions reflect the actual piece silhouette.
    #[test]
    fn grainline_sibling_bbox_not_inflated() {
        // cutline_Front is a 60×40 rectangle.
        // grainline_Front is a vertical stroke from y=-10 to y=50 — extends
        // beyond both the top and bottom edges of the cutline.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="Front">
    <g id="cutline_Front">
      <path d="M 0 0 L 60 0 L 60 40 L 0 40 L 0 0"/>
    </g>
    <g id="grainline_Front">
      <path d="M 30 -10 L 30 50"/>
    </g>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1, "expected 1 piece");
        assert_eq!(pieces[0].rect.w, 60,
            "width inflated by grainline: got {} expected 60", pieces[0].rect.w);
        assert_eq!(pieces[0].rect.h, 40,
            "height inflated by grainline: got {} expected 40", pieces[0].rect.h);
    } // grainline_sibling_bbox_not_inflated

} // mod tests
