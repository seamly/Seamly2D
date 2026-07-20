// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file layout_assembler.rs
// @brief Creates and assembles the output layout SVG DOM.
//
// Two-phase API:
//   1. `create_initial_layout_dom` — builds the blank canvas from layout settings:
//        <svg width=canvasW height=canvasH>
//          <g id="Rectangles">
//            <rect id="backgroundRect" …/>
//            <rect id="contentRect" …/>
//          </g>
//        </svg>
//      Stored as `initial_layout_dom` and displayed as `layout_dom` after Settings Submit.
//
//   2. `create_layout` — places pattern pieces into an existing `layout_dom`
//      (a clone of `initial_layout_dom`).  Does NOT change the SVG dimensions,
//      backgroundRect, contentRect, or tileRects paths.  Appends:
//        - A scale+translate group wrapping all placed piece `<g>` elements.
//          Each piece `<g>` gets a `<rect class="piece-fill">` prepended as its
//          first child — a semi-transparent colored background that is a permanent
//          visual element of the layout; it moves and rotates with the piece.
//
// Post-assembly trimming:
//   - Case 1 — Roll or fabric (media_type=="fabric" OR paper_type=="roll"):
//     `trim_bottom` shrinks svg height, backgroundRect height, and contentRect height
//     to remove blank space below the last placed piece (> 48 px threshold).
//   - Case 2 — Tiled: deferred; see docs/tiling-docs/TILING_REDUCTION_WORKFLOW.md.

use std::u32;
use packing::Placed;
use xmltree::{Element, XMLNode};

use layout_tiling::LayoutSettings;
use crate::piece_extractor::PieceRect;

// @brief Palette of distinct piece-fill colors.
// Each placed piece gets one color from this palette, cycling if there are more
// pieces than colors.  Excludes gray, black, and white for good contrast.
const PIECE_FILL_COLORS: &[&str] = &[
    "#FF4444", // red
    "#4488FF", // blue
    "#44BB44", // green
    "#FF8800", // orange
    "#AA00CC", // purple
    "#00AAAA", // teal
    "#FF66BB", // pink
    "#CC8800", // amber
    "#00CC88", // mint
    "#FF00FF", // magenta
    "#88CC00", // lime
    "#FF6644", // coral
];

// @brief Create the initial layout SVG shown immediately after Settings are submitted.
//
// Produces a blank canvas sized to the full physical media (before margin subtraction)
// with a content rectangle marking the usable packing area.
//
// SVG structure produced:
//   <svg width=layout_w_px height=layout_h_px xmlns="http://www.w3.org/2000/svg">
//     <g id="Rectangles">
//       <rect id="backgroundRect" x=0 y=0 width=layout_w_px height=layout_h_px fill=white stroke=none/>
//       <rect id="contentRect"    x=margin_left_px y=margin_top_px  width=layout_w_px-margin_left_px-margin_right_px height=layout_h_px-margin_top_px-margin_bottom_px fill=none stroke=black stroke-width=1/>
//     </g>
//   </svg>
//
// The `contentRect` id is used by `process_layout` to position pieces
//
// The `backgroundRect` and `contentRect` ids are used by `trim_bottom` to trim the final layout height for
// paper_type='roll' or media_type='fabric' if there is excessive blank space below the last piece.
//
// @param settings Parsed `LayoutSettings`; margin values used to size the canvas.
// @return New `svg_dom::Document` ready to serialize and display.
// Called by `submit_settings()` when media_type='fabric' or paper_type in ('sheet', 'roll')
pub fn create_initial_layout_dom(settings: &LayoutSettings) -> svg_dom::Document {
    let (ml, mr, mt, mb): (u32, u32, u32, u32) = settings.margin_px();
    let canvas_w: u32    = settings.page_w_px();
    let canvas_h: u32    = settings.page_h_px();

    // Build the <svg> root sized to the media.
    let mut svg_root = Element {
        name:       "svg".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  Some("http://www.w3.org/2000/svg".to_string()),
        prefix:     None,
        namespaces: None,
    };
    svg_root.attributes.insert("xmlns".to_string(),  "http://www.w3.org/2000/svg".to_string());
    svg_root.attributes.insert("width".to_string(),  canvas_w.to_string());
    svg_root.attributes.insert("height".to_string(), canvas_h.to_string());

    let mut output_doc = svg_dom::Document { root: svg_root };

    // Background rectangle — white fill covering the full page.
    let mut bg_rect = Element {
        name:       "rect".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    bg_rect.attributes.insert("id".to_string(),     "backgroundRect".to_string());
    bg_rect.attributes.insert("x".to_string(),      "0".to_string());
    bg_rect.attributes.insert("y".to_string(),      "0".to_string());
    bg_rect.attributes.insert("width".to_string(),  canvas_w.to_string());
    bg_rect.attributes.insert("height".to_string(), canvas_h.to_string());
    bg_rect.attributes.insert("fill".to_string(),   "white".to_string());
    bg_rect.attributes.insert("stroke".to_string(), "none".to_string());

    // Content rectangle — marks the usable packing area (bin dimensions, margin-offset).
    let mut content_rect = Element {
        name:       "rect".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };

    // calculate content rectangle dimensions by subtracting margins from page dimensions;
    let bin_w = canvas_w - ml - mr;
    let bin_h = canvas_h - mt - mb;

    content_rect.attributes.insert("id".to_string(),           "contentRect".to_string());
    content_rect.attributes.insert("x".to_string(),            ml.to_string());
    content_rect.attributes.insert("y".to_string(),            mt.to_string());
    content_rect.attributes.insert("width".to_string(),        bin_w.to_string());
    content_rect.attributes.insert("height".to_string(),       bin_h.to_string());
    content_rect.attributes.insert("fill".to_string(),         "none".to_string());
    content_rect.attributes.insert("stroke".to_string(),       "black".to_string());
    content_rect.attributes.insert("stroke-width".to_string(), "1".to_string());

    // Rectangles group — holds backgroundRect and contentRect so trim_bottom
    // can locate them by id rather than by position.
    let mut rects_group = Element {
        name:       "g".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    rects_group.attributes.insert("id".to_string(), "Rectangles".to_string());

    // add backgroundRect and contentRect to the Rectangles group, then add the group to the SVG root
    rects_group.children.push(XMLNode::Element(bg_rect));
    rects_group.children.push(XMLNode::Element(content_rect));
    output_doc.root.children.push(XMLNode::Element(rects_group));

    // return output_doc with the initial layout SVG structure
    output_doc
} // fn create_initial_layout_dom


// @brief Place pattern pieces into an existing layout DOM.
//
// The `layout_doc` must have been produced by `create_initial_layout_dom` —
// it already contains the SVG dimensions, backgroundRect, and contentRect
// (inside a `<g id="Rectangles">` group).  This function appends one `<g>`
// element per placed piece, directly as children of the SVG root — **no
// shared scale wrapper, no scale factor**.
//
// Seamly2D SVG coordinates are in CSS pixels (1 user-unit = 1 px at 96 dpi),
// so piece path data is already in layout pixels after the pre-processing
// pipeline (flatten → verticalize → flatten → translate → flatten).
// Each piece group therefore only needs a `translate(tx_px ty_px)` transform
// to position it on the canvas.
//
// Transform math (pixel-only):
//   tx_px = margin_left_px + placed.x - piece.origin_x
//   ty_px = margin_top_px  + placed.y - piece.origin_y
//   (origin ≈ 0 after translate_dom + flatten_dom; subtracted for correctness)
//
// A semi-transparent `<rect class="piece-fill">` is prepended as the first child
// of each piece group so it moves and rotates with the piece.  This is a
// permanent visual element of layout_dom; remove_color_blocks() removes it from
// export copies (DXF, PDF, etc.) where fill colors are not appropriate.  The
// fill rect dimensions are in pixels (placed.w × placed.h).
//
// @param layout_doc     Existing layout DOM to modify in place (clone of initial_layout_dom).
// @param input_doc      Pre-processed flat_dom; all coordinates in layout pixels.
// @param pieces         Extracted `PieceRect` list from `extract_piece_rects`.
// @param placements     Packed placements; `placed.id` indexes into `pieces`.
// @param margin_left_px Left margin in pixels; shifts pieces into content rect.
// @param margin_top_px  Top  margin in pixels; shifts pieces into content rect.
pub fn create_layout(
    layout_doc: &mut svg_dom::Document,
    input_doc: &svg_dom::Document,
    pieces: &[PieceRect],
    placements: &[Placed],
    margin_left_px: u32,
    margin_top_px: u32,
) {
    let ml = margin_left_px;
    let mt = margin_top_px;

    // Collect all top-level <g> elements in order (matching extract_piece_rects order).
    let group_elements: Vec<&Element> = input_doc
        .root
        .children
        .iter()
        .filter_map(|node| match node {
            XMLNode::Element(e) if e.name == "g" => Some(e),
            _ => None,
        })
        .collect(); // group_elements

    // --- Place each piece directly in the SVG root (no shared scale wrapper) ---
    //
    // Each piece <g> gets transform="translate(tx_px ty_px)".
    // The piece's internal path coordinates are already in layout pixels because
    // the caller applied svg_dom::scale_dom before calling this function.

    for (slot_idx, placed) in placements.iter().enumerate() {
        let piece_idx = placed.id; // index into `pieces`
        let piece = match pieces.get(piece_idx) {
            Some(p) => p,
            None    => continue, // should not happen; skip corrupt placement
        }; // match pieces.get

        let orig_group = match group_elements.get(piece.group_index) {
            Some(g) => *g,
            None    => continue, // should not happen; skip corrupt index
        }; // match group_elements.get

        // Pixel-space placement target: bin-local slot origin + content-rect margins.
        // This is where the ROTATED piece bbox min corner must land in canvas space.
        let target_x = (ml + placed.x) as f64;
        let target_y = (mt + placed.y) as f64;

        // Piece-local upright bbox in SVG user units (pixel-pure at this stage).
        let orig_x = piece.origin_x;
        let orig_y = piece.origin_y;
        let upright_w = piece.rect.w as f64;
        let upright_h = piece.rect.h as f64;

        let mut group_clone = orig_group.clone();

        // Compose the SVG transform.
        //
        // IMPORTANT: for 90°/270° (and any non-zero angle), translate must be
        // computed from the rotated AABB minimum corner, not from the upright
        // origin. Otherwise the piece can be shifted outside contentRect even if
        // the packer returned an in-bounds slot.
        //
        // SVG applies `translate(...) rotate(...)` right-to-left, so rotation
        // happens first in piece-local coordinates; we therefore compute the
        // rotated AABB min in local space and then translate that min to the
        // target slot origin.
        let transform = match placed.rotation_deg {
            0 => {
                let tx = target_x - orig_x;
                let ty = target_y - orig_y;
                format!("translate({tx:.4} {ty:.4})")
            }
            deg => {
                let cx = orig_x + upright_w / 2.0;
                let cy = orig_y + upright_h / 2.0;
                let theta = (deg as f64).to_radians();
                let (ct, st) = (theta.cos(), theta.sin());

                // Rotate upright bbox corners about (cx, cy), then measure the
                // rotated AABB minimum in local coordinates.
                let corners = [
                    (orig_x,             orig_y),
                    (orig_x + upright_w, orig_y),
                    (orig_x + upright_w, orig_y + upright_h),
                    (orig_x,             orig_y + upright_h),
                ];

                let mut min_rx = f64::INFINITY;
                let mut min_ry = f64::INFINITY;
                for (x, y) in corners {
                    let dx = x - cx;
                    let dy = y - cy;
                    let rx = cx + (dx * ct) - (dy * st);
                    let ry = cy + (dx * st) + (dy * ct);
                    min_rx = min_rx.min(rx);
                    min_ry = min_ry.min(ry);
                }

                let tx = target_x - min_rx;
                let ty = target_y - min_ry;
                format!("translate({tx:.4} {ty:.4}) rotate({deg} {cx:.4} {cy:.4})")
            } // deg
        }; // match rotation_deg
        group_clone.attributes.insert("transform".to_string(), transform);

        // Prepend a colored piece-fill rect inside the piece group so it moves/rotates with it.
        // The fill rect uses the piece's UPRIGHT bbox (not placed.w/h) — it lives
        // inside the rotating group, so SVG applies the same rotate() transform
        // to it as to the path geometry.  Sourcing the dims from `pieces[id].rect`
        // keeps the fill aligned with the piece even when the packer records 180°.
        // All coordinates are in layout pixels (1 UU = 1 px for Seamly2D SVGs).
        let color  = PIECE_FILL_COLORS[slot_idx % PIECE_FILL_COLORS.len()];
        let w_px   = upright_w as u32;
        let h_px   = upright_h as u32;
        let mut fill_rect = Element {
            name:       "rect".to_string(),
            attributes: Default::default(),
            children:   Vec::new(),
            namespace:  None,
            prefix:     None,
            namespaces: None,
        };

        // update fill_rect attributes
        fill_rect.attributes.insert("class".to_string(),          "piece-fill".to_string());
        // x/y = piece bbox origin in local pixel coords (≈ 0 after translate_dom).
        fill_rect.attributes.insert("x".to_string(),              format!("{orig_x:.4}"));
        fill_rect.attributes.insert("y".to_string(),              format!("{orig_y:.4}"));
        fill_rect.attributes.insert("width".to_string(),          w_px.to_string());
        fill_rect.attributes.insert("height".to_string(),         h_px.to_string());
        fill_rect.attributes.insert("fill".to_string(),           color.to_string());
        fill_rect.attributes.insert("fill-opacity".to_string(),   "0.3".to_string());
        fill_rect.attributes.insert("stroke".to_string(),         color.to_string());
        fill_rect.attributes.insert("stroke-width".to_string(),   "2".to_string());
        fill_rect.attributes.insert("stroke-opacity".to_string(), "0.8".to_string());
        // Insert fill_rect to piece group at position 0 (first child) so the fill renders behind the piece's path geometry.
        group_clone.children.insert(0, XMLNode::Element(fill_rect));

        // Append piece group as a direct child of SVG root & return the modified layout_doc.
        layout_doc.root.children.push(XMLNode::Element(group_clone));
    } // for slot_idx, placed in placements
} // fn create_layout

// @brief Trim unused whitespace below the last placed piece (Case 1: roll or fabric).
//
// After `create_layout`, the content rectangle may have a large blank region
// below the last placed piece (roll bin uses a 500-inch sentinel height).  If the
// unused bottom space exceeds 48 pixels, this function reduces:
//   - The `<svg height>` attribute.
//   - The `<rect id="backgroundRect">` height attribute.
//   - The `<rect id="contentRect">` height attribute.
//
// New heights:
//   svg / backgroundRect height = margin_top_px + max_bin_bottom + margin_bottom_px
//   contentRect height          = max_bin_bottom
//
// The caller is responsible for the 48-pixel threshold check before calling.
//
// @param doc              Layout SVG document after `create_layout`.
// @param max_bin_bottom   max(placed.y + placed.h) across all placements, in bin pixels.
//                         (Bin coordinates: (0, 0) = content rect top-left corner.)
// @param margin_top_px    Top margin in pixels.
// @param margin_bottom_px Bottom margin in pixels.
pub fn trim_bottom(
    doc: &mut svg_dom::Document,
    max_bin_bottom: u32,
    margin_top_px: u32,
    margin_bottom_px: u32,
) {
    let new_doc_height = margin_top_px + max_bin_bottom + margin_bottom_px;

    // Update SVG root height attribute.
    doc.root
        .attributes
        .insert("height".to_string(), new_doc_height.to_string());

    // Update backgroundRect height (located by id).
    doc.set_attr_by_id("backgroundRect", "height", new_doc_height.to_string());

    // Update contentRect height (located by id); x, y, and width are preserved.
    doc.set_attr_by_id("contentRect", "height", max_bin_bottom.to_string());
} // fn trim_bottom

// @brief Remove piece-fill color rects from an export copy of the layout SVG.
//
// `layout_dom` retains `<rect class="piece-fill">` rects inside each piece group
// as a permanent visual element for canvas display.  This function strips them
// from a **clone** of the document before it is passed to any exporter (DXF, PDF,
// SVG, PNG) so that exported files contain only the pattern piece geometry.
//
// @param doc  Mutable reference to the export clone; modified in place.
pub fn remove_color_blocks(doc: &mut svg_dom::Document) {
    remove_color_blocks_rec(&mut doc.root);
} // fn remove_color_blocks

// @brief Recursively remove all overlay elements marked with class `piece-fill`.
//
// @param element  SVG element whose subtree is pruned in place.
fn remove_color_blocks_rec(element: &mut Element) {
    element.children.retain(|node| {
        if let XMLNode::Element(e) = node {
            let cls = e.attributes.get("class").map(String::as_str).unwrap_or("");
            if cls.split_whitespace().any(|name| name == "piece-fill") {
                return false; // remove any piece-fill overlay element regardless of tag name
            } // if piece-fill class token
        } // if Element
        true // keep everything else
    }); // retain

    // Recurse into child elements.
    for node in &mut element.children {
        if let XMLNode::Element(child) = node {
            remove_color_blocks_rec(child);
        } // if Element
    } // for node
} // fn remove_color_blocks_rec

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use crate::piece_extractor::extract_piece_rects;

    // @brief Create a minimal base layout doc for testing (0 margins, canvas = bin).
    // Mirrors the structure produced by `create_initial_layout_dom` with all margins = 0.
    fn make_base_doc(bin_w: u32, bin_h: u32) -> svg_dom::Document {
        let xml = format!(
            r#"<svg xmlns="http://www.w3.org/2000/svg" width="{bin_w}" height="{bin_h}"><g id="Rectangles"><rect id="backgroundRect" x="0" y="0" width="{bin_w}" height="{bin_h}" fill="white" stroke="none"/><rect id="contentRect" x="0" y="0" width="{bin_w}" height="{bin_h}" fill="none" stroke="black" stroke-width="1"/></g></svg>"#
        );
        svg_dom::Document::parse(&xml).expect("parse base doc ok")
    } // fn make_base_doc

    // @brief Assemble two pieces and verify the output SVG has correct groups.
    #[test]
    fn assembles_two_pieces() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="500" height="500">
  <g id="p1"><path d="M 0 0 L 96 0 L 96 96 L 0 96 Z"/></g>
  <g id="p2"><path d="M 0 0 L 48 0 L 48 48 L 0 48 Z"/></g>
</svg>"#;
        let input_doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&input_doc);
        assert_eq!(pieces.len(), 2);

        let rects: Vec<packing::Rect> = pieces.iter().map(|p| p.rect).collect();
        let bin_w = 300u32;
        let bin_h = 300u32;
        let placements = packing::pack_shelves(bin_w, bin_h, &rects)
            .expect("pack ok");

        let mut base_doc = make_base_doc(bin_w, bin_h);
        create_layout(&mut base_doc, &input_doc, &pieces, &placements, 0, 0);
        let svg_str = base_doc.to_string();

        // Canvas dimensions unchanged.
        assert!(svg_str.contains("width=\"300\""),  "width: {svg_str}");
        assert!(svg_str.contains("height=\"300\""), "height: {svg_str}");

        // Both piece groups should be present.
        assert!(svg_str.contains("id=\"p1\""), "missing p1");
        assert!(svg_str.contains("id=\"p2\""), "missing p2");

        // Every placed group should have a translate transform.
        assert!(svg_str.contains("translate("), "missing translate");
    } // assembles_two_pieces

    // @brief Assembler emits per-piece translate; no scale group anywhere in the output.
    #[test]
    fn assembler_sets_pixel_translate() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="300" height="300">
  <g id="p1"><path d="M 0 0 L 50 0 L 50 50 L 0 50 Z"/></g>
</svg>"#;
        let input_doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&input_doc);
        let rects: Vec<packing::Rect> = pieces.iter().map(|p| p.rect).collect();
        let placements = packing::pack_shelves(200, 200, &rects).expect("pack ok");

        let mut base_doc = make_base_doc(200, 200);
        create_layout(&mut base_doc, &input_doc, &pieces, &placements, 0, 0);
        let svg_str = base_doc.to_string();

        // Per-piece translate is present.
        assert!(svg_str.contains("translate("),  "missing translate");
        // No scale() anywhere in the output — layout_dom is pixel-pure.
        assert!(!svg_str.contains("scale("), "unexpected scale: {svg_str}");
    } // assembler_sets_pixel_translate

    // @brief remove_color_blocks removes piece-fill rects from export copies.
    #[test]
    fn remove_color_blocks_removes_fills() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="p1"><path d="M 0 0 L 30 0 L 30 30 L 0 30 Z"/></g>
</svg>"#;
        let input_doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&input_doc);
        let rects: Vec<packing::Rect> = pieces.iter().map(|p| p.rect).collect();
        let placements = packing::pack_shelves(200, 200, &rects).expect("pack ok");

        let mut base_doc = make_base_doc(200, 200);
        create_layout(&mut base_doc, &input_doc, &pieces, &placements, 0, 0);
        let before = base_doc.to_string();
        assert!(before.contains("piece-fill"), "piece-fill missing before strip");

        remove_color_blocks(&mut base_doc);
        let after = base_doc.to_string();
        assert!(!after.contains("piece-fill"), "piece-fill still present after strip");
        // Pattern piece and Rectangles group should still be present.
        assert!(after.contains("id=\"p1\""),   "piece p1 removed by strip");
        assert!(after.contains("Rectangles"),  "Rectangles group removed by strip");
    } // remove_color_blocks_removes_fills

        // @brief remove_color_blocks removes piece-fill rects even when class has multiple tokens.
        #[test]
        fn remove_color_blocks_removes_piece_fill_multiclass() {
                let xml = r#"
<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
    <g id="p1">
        <rect class="piece-fill overlay" x="0" y="0" width="10" height="10"/>
        <path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/>
    </g>
</svg>
"#;
                let mut doc = svg_dom::Document::parse(xml).expect("parse multiclass piece fill doc");
                remove_color_blocks(&mut doc);
                let after = doc.to_string();
                assert!(!after.contains("piece-fill"), "piece-fill class still present after strip");
                assert!(after.contains("id=\"p1\""), "piece group removed unexpectedly");
                assert!(after.contains("<path"), "piece path removed unexpectedly");
        } // remove_color_blocks_removes_piece_fill_multiclass

    // @brief remove_color_blocks removes non-rect overlays when class includes piece-fill.
    #[test]
    fn remove_color_blocks_removes_piece_fill_non_rect() {
        let xml = r#"
<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
    <g id="p1">
        <path class="piece-fill" d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/>
        <path d="M 20 20 L 30 20 L 30 30 L 20 30 Z"/>
    </g>
</svg>
"#;
        let mut doc = svg_dom::Document::parse(xml).expect("parse non-rect piece fill doc");
        remove_color_blocks(&mut doc);
        let after = doc.to_string();
        assert!(!after.contains("class=\"piece-fill\""), "piece-fill class still present after strip");
        assert!(after.contains("id=\"p1\""), "piece group removed unexpectedly");
        // Keep the non-overlay path.
        assert!(after.contains("M 20 20"), "non-overlay path removed unexpectedly");
    } // remove_color_blocks_removes_piece_fill_non_rect

    // @brief trim_bottom updates svg height, backgroundRect height, and contentRect height.
    #[test]
    fn trim_bottom_updates_dimensions() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="p1"><path d="M 0 0 L 30 0 L 30 30 L 0 30 Z"/></g>
</svg>"#;
        let input_doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&input_doc);
        let rects: Vec<packing::Rect> = pieces.iter().map(|p| p.rect).collect();
        let bin_w = 200u32;
        let bin_h = 3456u32; // simulated sentinel height (roll)
        let placements = packing::pack_shelves(bin_w, bin_h, &rects).expect("pack ok");

        let mut base_doc = make_base_doc(bin_w, bin_h);
        create_layout(&mut base_doc, &input_doc, &pieces, &placements, 0, 0);

        // Verify sentinel height before trim.
        assert_eq!(
            base_doc.root.attributes.get("height").map(String::as_str),
            Some("3456"),
            "sentinel height before trim"
        );

        // Trim: max_bin_bottom=60, margins=0 → new svg/bg height=60, contentRect height=60.
        trim_bottom(&mut base_doc, 60, 0, 0);
        let svg_str = base_doc.to_string();

        // SVG root height updated.
        assert_eq!(
            base_doc.root.attributes.get("height").map(String::as_str),
            Some("60"),
            "svg root height not updated"
        );
        // Check serialized output contains new height.
        assert!(svg_str.contains("height=\"60\""), "height=60 not in svg: {svg_str}");

        // backgroundRect height updated by id.
        let bg_h = base_doc.get_attr_by_id("backgroundRect", "height");
        assert_eq!(bg_h, Some("60"), "backgroundRect height not updated: {bg_h:?}");

        // contentRect height updated by id.
        let cr_h = base_doc.get_attr_by_id("contentRect", "height");
        assert_eq!(cr_h, Some("60"), "contentRect height not updated: {cr_h:?}");
    } // trim_bottom_updates_dimensions

    // @brief trim_bottom with margins: new svg height = mt + max_bin_bottom + mb.
    #[test]
    fn trim_bottom_with_margins() {
        let mut base_doc = make_base_doc(200, 5000); // tall sentinel
        // max_bin_bottom=100, mt=24, mb=24 → new svg height = 24+100+24 = 148
        trim_bottom(&mut base_doc, 100, 24, 24);

        assert_eq!(
            base_doc.root.attributes.get("height").map(String::as_str),
            Some("148"),
            "svg height with margins"
        );
        assert_eq!(
            base_doc.get_attr_by_id("backgroundRect", "height"),
            Some("148"),
            "backgroundRect height with margins"
        );
        // contentRect height = max_bin_bottom only (margin not included).
        assert_eq!(
            base_doc.get_attr_by_id("contentRect", "height"),
            Some("100"),
            "contentRect height with margins"
        );
    } // trim_bottom_with_margins

    // @brief Rotated placement translates by rotated-AABB min, not upright origin.
    //
    // Regression for out-of-bounds rotated pieces (e.g., Collar / CollarStand):
    // with a non-zero piece-local origin and 90° rotation, the assembler must
    // compute tx/ty from the rotated bbox min corner so the packed slot stays
    // inside contentRect.
    #[test]
    fn rotated_translate_uses_rotated_bbox_min() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="1000">
  <g id="p1"><path d="M 100 200 L 300 200 L 300 300 L 100 300 Z"/></g>
</svg>"#;

        let input_doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&input_doc);
        assert_eq!(pieces.len(), 1);

        // Packed slot at the content origin with 90° rotation.
        let placements = vec![packing::Placed {
            id: 0,
            x: 0,
            y: 0,
            w: 100,
            h: 200,
            rotation_deg: 90,
        }];

        let mut base_doc = make_base_doc(500, 500);
        create_layout(&mut base_doc, &input_doc, &pieces, &placements, 24, 24);

        // Expected transform for this geometry:
        // upright bbox: min=(100,200), size=(200,100), center=(200,250)
        // rotated 90° bbox min=(150,150)
        // target slot min=(24,24) => translate=(-126,-126)
        let mut found = None::<String>;
        for node in &base_doc.root.children {
            if let XMLNode::Element(e) = node {
                if e.name == "g" && e.attributes.get("id").map(String::as_str) == Some("p1") {
                    found = e.attributes.get("transform").cloned();
                    break;
                }
            }
        }

        assert_eq!(
            found.as_deref(),
            Some("translate(-126.0000 -126.0000) rotate(90 200.0000 250.0000)"),
            "unexpected transform for rotated placement"
        );
    } // rotated_translate_uses_rotated_bbox_min

} // mod tests
