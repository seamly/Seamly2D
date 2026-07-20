// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file oversized.rs
// @brief Phase A of the "sheets" paper_type — Task L.2.1.
//
// Identifies pieces too large for a single sheet's contentRect and packs them
// into a compact "oversized" SVG ready for the existing tiled-PDF pipeline.
//
// Algorithm (from Task L.2.1):
//   1. Identify all oversized pieces that are wider or taller than the sheet's
//      contentRect width or height.
//   2. Sort oversized pieces by area (largest first); determine total image
//      width and height that can hold all oversized pieces.
//   3. Create an 'oversized' SVG containing backgroundRect and contentRect.
//   4. Place oversized pieces within the contentRect using MaxRects.
//   5. Return the assembled SVG; callers feed it into the tiled-PDF pipeline.
//
// Public API:
//   partition_oversized_pieces(pieces, content_w_px, content_h_px)
//       -> (Vec<usize>, Vec<usize>)
//   build_oversized_svg(flat_dom, all_pieces, oversized_indices, gap_px,
//                       trial_angles_deg, ml_px, mt_px, mr_px, mb_px)
//       -> Result<svg_dom::Document, String>

use svg_dom::Document;
use xmltree::{Element, XMLNode};
use layout_tiling::LAYOUT_PPI;
use crate::piece_extractor::PieceRect;

// Height sentinel in pixels for the initial MaxRects bin (500 inches at 96 dpi).
// Trimmed to the actual extent of placed pieces after packing.
const OVERSIZED_HEIGHT_SENTINEL_PX: u32 = (500.0 * LAYOUT_PPI as f64) as u32; // 48_000 px

// ---------------------------------------------------------------------------
// partition_oversized_pieces
// ---------------------------------------------------------------------------

// @brief Split pieces into oversized and remaining by comparing to sheet contentRect.
//
// A piece is oversized if its width exceeds `content_w_px` OR its height exceeds
// `content_h_px`.  Both returned vectors hold indices into the `pieces` slice.
// Relative order within each partition is preserved.
//
// @param pieces         All extracted PieceRect entries after preprocessing.
// @param content_w_px   Sheet contentRect width in pixels (from settings.effective_bin_px()).
// @param content_h_px   Sheet contentRect height in pixels (from settings.effective_bin_px()).
// @return (oversized_indices, remaining_indices) — parallel index vectors into `pieces`.
pub fn partition_oversized_pieces(
    pieces: &[PieceRect],
    content_w_px: u32,
    content_h_px: u32,
) -> (Vec<usize>, Vec<usize>) {
    // Walk every piece and classify by its AABB dimensions against the sheet bin.
    let mut oversized: Vec<usize> = Vec::new();
    let mut remaining: Vec<usize> = Vec::new();
    for (i, p) in pieces.iter().enumerate() {
        // A piece is oversized when it exceeds the sheet contentRect on either axis.
        if p.rect.w > content_w_px || p.rect.h > content_h_px {
            oversized.push(i);
        } else {
            remaining.push(i);
        } // if oversized
    } // for (i, p)
    (oversized, remaining)
} // fn partition_oversized_pieces

// ---------------------------------------------------------------------------
// build_oversized_svg
// ---------------------------------------------------------------------------

// @brief Pack oversized pieces and assemble an "oversized" SVG for Phase A.
//
// Steps (matching L.2.1 task description):
//   1. Collect oversized piece rects by index.
//   2. Sort by area, largest first (MaxRects heuristic for better density).
//   3. Compute bin_w = widest oversized piece; bin_h = OVERSIZED_HEIGHT_SENTINEL_PX.
//   4. Run MaxRects (via packing::pack_pieces) to place all oversized pieces.
//   5. Trim bin_h to the actual extent of placed pieces (max placed.y + placed.h).
//   6. Build the SVG root sized to (content_w + ml + mr) × (content_h + mt + mb).
//      Add backgroundRect at (0, 0) and contentRect at (ml, mt).
//   7. Copy each oversized piece <g> from flat_dom with a translate transform.
//   8. Return the assembled SVG Document.
//
// The returned Document is ready to pass to do_export_pdf_tile_with_settings() /
// do_export_pdf_tile_inner() to produce the multipage tiled PDF.
//
// @param flat_dom           Pre-processed SVG DOM (flatten→verticalize→translate→flatten).
// @param all_pieces         All PieceRect entries; `oversized_indices` index into this.
// @param oversized_indices  Indices of oversized pieces in `all_pieces`.
// @param gap_px             Inter-piece clearance in pixels.
// @param trial_angles_deg   Rotation trial set in degrees (from LayoutSettings).
// @param ml_px              Left  margin in pixels (from sheet settings).
// @param mt_px              Top   margin in pixels.
// @param mr_px              Right margin in pixels.
// @param mb_px              Bottom margin in pixels.
// @return Oversized SVG Document on success; Err with a descriptive message otherwise.
pub fn build_oversized_svg(
    flat_dom: &Document,
    all_pieces: &[PieceRect],
    oversized_indices: &[usize],
    gap_px: u32,
    trial_angles_deg: &[u16],
    ml_px: u32,
    mt_px: u32,
    mr_px: u32,
    mb_px: u32,
) -> Result<Document, String> {

    // Guard: caller must supply at least one oversized piece.
    if oversized_indices.is_empty() {
        return Err(
            "build_oversized_svg: oversized_indices is empty — nothing to pack.".to_string()
        );
    } // if empty

    // Validate all indices before dereferencing.
    for &idx in oversized_indices {
        if idx >= all_pieces.len() {
            return Err(format!(
                "build_oversized_svg: oversized index {idx} out of range (pieces.len()={}).",
                all_pieces.len()
            ));
        } // if out of range
    } // for idx

    // --- 1 collect oversized piece references ---
    // Uses original indices so we can map back to all_pieces[i] and group_index later.
    let oversized_pieces: Vec<&PieceRect> = oversized_indices
        .iter()
        .map(|&i| &all_pieces[i])
        .collect();

    // --- 2 sort by area, largest first ---
    // local_order[j] is the position within oversized_pieces that should be packed j-th.
    // Pack-id returned by MaxRects is an index into the sorted_rects slice; we reverse
    // this mapping via local_order to get the original oversized_pieces index.
    let mut local_order: Vec<usize> = (0..oversized_pieces.len()).collect();
    local_order.sort_by(|&a, &b| {
        let area_a = oversized_pieces[a].rect.w as u64 * oversized_pieces[a].rect.h as u64;
        let area_b = oversized_pieces[b].rect.w as u64 * oversized_pieces[b].rect.h as u64;
        area_b.cmp(&area_a) // descending area — largest piece first
    }); // sort local_order

    // Build the sorted rect slice passed to the packer.
    // sorted_rects[i] corresponds to oversized_pieces[local_order[i]].
    let sorted_rects: Vec<packing::Rect> = local_order
        .iter()
        .map(|&li| oversized_pieces[li].rect)
        .collect();

    // --- 3 determine bin dimensions ---
    // bin_w = widest oversized piece (minimum width that fits the largest piece on one row).
    // bin_h = generous sentinel; trimmed to actual extent after packing.
    let bin_w: u32 = sorted_rects.iter().map(|r| r.w).max().unwrap_or(1);
    let bin_h: u32 = OVERSIZED_HEIGHT_SENTINEL_PX;

    log::debug!(
        "build_oversized_svg: {} oversized pieces; bin={}x{}; gap_px={}; trial_angles={:?}",
        oversized_pieces.len(), bin_w, bin_h, gap_px, trial_angles_deg
    );

    // --- 4 pack oversized pieces with MaxRects ---
    let (raw_placements, _free_rects) = packing::pack_pieces(
        bin_w,
        bin_h,
        gap_px,
        &sorted_rects,
        trial_angles_deg,
    ).map_err(|e| {
        match e {
            packing::PackError::TooLarge { id, w, h, bin_w, bin_h } => {
                // Map pack-id back to the original piece label for diagnostics.
                let local_idx = local_order.get(id).copied().unwrap_or(id);
                let label = oversized_pieces.get(local_idx).map(|p| p.id.as_str()).unwrap_or("?");
                format!(
                    "Oversized piece \"{label}\" ({w}\u{d7}{h} px) is larger than the \
                     oversized packing bin ({bin_w}\u{d7}{bin_h} px). \
                     This should not happen — please report this as a bug."
                )
            }, // TooLarge
            packing::PackError::NoSpace { id } => {
                let local_idx = local_order.get(id).copied().unwrap_or(id);
                let label = oversized_pieces.get(local_idx).map(|p| p.id.as_str()).unwrap_or("?");
                format!(
                    "No space for oversized piece \"{label}\" in the oversized packing bin."
                )
            }, // NoSpace
            packing::PackError::SearchLimit { id } => {
                let local_idx = local_order.get(id).copied().unwrap_or(id);
                let label = oversized_pieces.get(local_idx).map(|p| p.id.as_str()).unwrap_or("?");
                format!(
                    "Rotation search limit exceeded for oversized piece \"{label}\". \
                     Try reducing piece gap or using a simpler rotation mode."
                )
            }, // SearchLimit
        } // match e
    })?; // pack_pieces

    // --- 5 trim height to actual placed extent ---
    // content_w is fixed to bin_w (the narrowest valid bin for this set of pieces).
    // content_h is the bottom edge of the lowest placed piece.
    let content_w: u32 = bin_w;
    let content_h: u32 = raw_placements
        .iter()
        .map(|p| p.y + p.h)
        .max()
        .unwrap_or(0);

    // Total SVG dimensions including margins.
    let svg_w: u32 = content_w + ml_px + mr_px;
    let svg_h: u32 = content_h + mt_px + mb_px;

    log::debug!(
        "build_oversized_svg: content={}x{} svg={}x{}; {} placements",
        content_w, content_h, svg_w, svg_h, raw_placements.len()
    );

    // --- 6 build SVG document ---

    // SVG root element.
    let mut svg_root = Element {
        name:       "svg".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  Some("http://www.w3.org/2000/svg".to_string()),
        prefix:     None,
        namespaces: None,
    };
    svg_root.attributes.insert("xmlns".to_string(),  "http://www.w3.org/2000/svg".to_string());
    svg_root.attributes.insert("id".to_string(),     "oversized_layout".to_string());
    svg_root.attributes.insert("width".to_string(),  svg_w.to_string());
    svg_root.attributes.insert("height".to_string(), svg_h.to_string());

    let mut out_doc = Document { root: svg_root };

    // backgroundRect — white fill covering the full oversized SVG area.
    let bg = make_rect("backgroundRect", 0, 0, svg_w, svg_h, "white", "none");

    // contentRect — marks the packing area (margin-inset from the SVG boundary).
    let mut cr = make_rect("contentRect", ml_px, mt_px, content_w, content_h, "none", "black");
    cr.attributes.insert("stroke-width".to_string(), "1".to_string());

    // <g id="Rectangles"> group holds both rect elements (same convention as layout_assembler).
    let mut rects_group = Element {
        name:       "g".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    rects_group.attributes.insert("id".to_string(), "Rectangles".to_string());
    rects_group.children.push(XMLNode::Element(bg));
    rects_group.children.push(XMLNode::Element(cr));
    out_doc.root.children.push(XMLNode::Element(rects_group));

    // --- 7 copy piece <g> elements into the oversized SVG ---

    // Build an ordered list of all top-level <g> elements from flat_dom.
    // PieceRect::group_index stores the position within this list.
    let group_elements: Vec<&Element> = flat_dom
        .root
        .children
        .iter()
        .filter_map(|node| match node {
            XMLNode::Element(e) if e.name == "g" => Some(e),
            _ => None,
        })
        .collect();

    // Place each oversized piece into the oversized SVG.
    // raw_placements[i].id → sorted_rects index → local_order[id] → oversized_pieces index.
    for placed in &raw_placements {
        // Reverse the sort mapping: placement id → local_order → oversized_pieces.
        let local_idx = match local_order.get(placed.id) {
            Some(&li) => li,
            None => {
                log::debug!(
                    "build_oversized_svg: placement id {} out of range in local_order (len={}); skipping.",
                    placed.id, local_order.len()
                );
                continue; // should not happen; skip corrupt placement
            } // None
        }; // local_idx

        let piece: &PieceRect = match oversized_pieces.get(local_idx) {
            Some(p) => *p,
            None => {
                log::debug!(
                    "build_oversized_svg: local_idx {} out of range in oversized_pieces; skipping.",
                    local_idx
                );
                continue; // should not happen
            } // None
        }; // piece

        let orig_group: &Element = match group_elements.get(piece.group_index) {
            Some(g) => *g,
            None => {
                log::debug!(
                    "build_oversized_svg: group_index {} out of range in flat_dom groups (len={}); skipping.",
                    piece.group_index, group_elements.len()
                );
                continue; // should not happen
            } // None
        }; // orig_group

        // Compose the SVG transform that positions this piece in the oversized SVG.
        // Target corner: (ml_px + placed.x, mt_px + placed.y) in canvas coordinates.
        let target_x: f64 = (ml_px + placed.x) as f64;
        let target_y: f64 = (mt_px + placed.y) as f64;
        let orig_x:   f64 = piece.origin_x;
        let orig_y:   f64 = piece.origin_y;
        let upright_w: f64 = piece.rect.w as f64;
        let upright_h: f64 = piece.rect.h as f64;

        // Mirror the rotation transform math from layout_assembler::create_layout.
        // SVG applies transform="translate(...) rotate(...)" right-to-left, so rotation
        // is applied first in piece-local space, then translation moves the rotated
        // AABB minimum corner to the target slot origin.
        let transform = match placed.rotation_deg {
            0 => {
                // No rotation: simple translate from piece origin to target slot.
                let tx = target_x - orig_x;
                let ty = target_y - orig_y;
                format!("translate({tx:.4} {ty:.4})")
            } // 0° rotation
            deg => {
                // Compute the rotated AABB minimum corner in local piece coordinates,
                // then translate that minimum to the target slot origin.
                let cx = orig_x + upright_w / 2.0; // rotation center x
                let cy = orig_y + upright_h / 2.0; // rotation center y
                let theta = (deg as f64).to_radians();
                let (ct, st) = (theta.cos(), theta.sin());

                // Rotate all four corners of the upright bbox about (cx, cy).
                let corners: [(f64, f64); 4] = [
                    (orig_x,              orig_y),
                    (orig_x + upright_w,  orig_y),
                    (orig_x + upright_w,  orig_y + upright_h),
                    (orig_x,              orig_y + upright_h),
                ];
                let mut min_rx = f64::INFINITY;
                let mut min_ry = f64::INFINITY;
                for (x, y) in corners {
                    let dx = x - cx;
                    let dy = y - cy;
                    let rx = cx + dx * ct - dy * st; // rotated x
                    let ry = cy + dx * st + dy * ct; // rotated y
                    min_rx = min_rx.min(rx);
                    min_ry = min_ry.min(ry);
                } // for corner

                // Translate so the rotated AABB min lands at the target slot origin.
                let tx = target_x - min_rx;
                let ty = target_y - min_ry;
                format!("translate({tx:.4} {ty:.4}) rotate({deg} {cx:.4} {cy:.4})")
            } // non-zero rotation
        }; // match rotation_deg

        // Clone the piece <g> and attach the composed transform.
        let mut group_clone = orig_group.clone();
        group_clone.attributes.insert("transform".to_string(), transform);

        // Append to the oversized SVG root.
        out_doc.root.children.push(XMLNode::Element(group_clone));
    } // for placed

    Ok(out_doc)
} // fn build_oversized_svg

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// @brief Build a plain SVG <rect> element with the given attributes.
//
// @param id     Value for the `id` attribute.
// @param x      Left edge in pixels.
// @param y      Top edge in pixels.
// @param w      Width in pixels.
// @param h      Height in pixels.
// @param fill   Fill color string (e.g., "white", "none").
// @param stroke Stroke color string (e.g., "none", "black").
// @return New Element ready to push into a parent's children vec.
fn make_rect(
    id: &str,
    x: u32,
    y: u32,
    w: u32,
    h: u32,
    fill: &str,
    stroke: &str,
) -> Element {
    let mut rect = Element {
        name:       "rect".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    rect.attributes.insert("id".to_string(),     id.to_string());
    rect.attributes.insert("x".to_string(),      x.to_string());
    rect.attributes.insert("y".to_string(),      y.to_string());
    rect.attributes.insert("width".to_string(),  w.to_string());
    rect.attributes.insert("height".to_string(), h.to_string());
    rect.attributes.insert("fill".to_string(),   fill.to_string());
    rect.attributes.insert("stroke".to_string(), stroke.to_string());
    rect
} // fn make_rect

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // Helper: build a minimal PieceRect for testing.
    fn piece(w: u32, h: u32, id: &str, group_index: usize) -> PieceRect {
        PieceRect {
            rect: packing::Rect::new(w, h),
            id:   id.to_string(),
            origin_x:    0.0,
            origin_y:    0.0,
            group_index,
        }
    } // fn piece

    // Helper: build a minimal flat_dom with N empty <g> groups for testing.
    fn flat_dom_with_n_groups(n: usize) -> Document {
        let svg_str_parts: Vec<String> = (0..n)
            .map(|i| {
                // Each group has a single path so extract_piece_rects would not skip it,
                // but for oversized tests we only need the <g> element to be cloneable.
                format!(
                    "<g id=\"p{i}\"><path d=\"M 0 0 L {w} 0 L {w} {h} L 0 {h} Z\"/></g>",
                    w = (i + 1) * 100,
                    h = (i + 1) * 100,
                )
            })
            .collect();
        let svg_str = format!(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"2000\" height=\"2000\">{}</svg>",
            svg_str_parts.join("")
        );
        Document::parse(&svg_str).expect("parse ok")
    } // fn flat_dom_with_n_groups

    // -----------------------------------------------------------------------
    // partition_oversized_pieces
    // -----------------------------------------------------------------------

    // @brief All pieces fit on the sheet — no oversized, all remaining.
    #[test]
    fn partition_no_oversized() {
        let pieces = vec![
            piece(100, 200, "a", 0),
            piece(300, 400, "b", 1),
        ];
        let (over, rem) = partition_oversized_pieces(&pieces, 500, 600);
        assert!(over.is_empty(), "expected no oversized: {over:?}");
        assert_eq!(rem, vec![0, 1], "remaining should be all pieces");
    } // partition_no_oversized

    // @brief All pieces are oversized — no remaining.
    #[test]
    fn partition_all_oversized() {
        let pieces = vec![
            piece(800, 200, "wide", 0),  // wider than 700
            piece(100, 900, "tall", 1),  // taller than 700
        ];
        let (over, rem) = partition_oversized_pieces(&pieces, 700, 700);
        assert_eq!(over, vec![0, 1], "both pieces should be oversized");
        assert!(rem.is_empty(), "expected no remaining");
    } // partition_all_oversized

    // @brief Mixed: some pieces fit, some don't.
    #[test]
    fn partition_mixed() {
        let pieces = vec![
            piece(300, 400, "fits",  0),
            piece(800, 400, "wide",  1), // too wide
            piece(300, 900, "tall",  2), // too tall
            piece(200, 200, "small", 3),
        ];
        let (over, rem) = partition_oversized_pieces(&pieces, 500, 500);
        assert_eq!(over, vec![1, 2], "oversized indices");
        assert_eq!(rem,  vec![0, 3], "remaining indices");
    } // partition_mixed

    // @brief Piece exactly matching the content dimensions is NOT oversized (boundary).
    #[test]
    fn partition_exact_boundary_not_oversized() {
        let pieces = vec![
            piece(500, 500, "exact", 0),
        ];
        let (over, rem) = partition_oversized_pieces(&pieces, 500, 500);
        assert!(over.is_empty(), "exact-match piece should NOT be oversized");
        assert_eq!(rem, vec![0]);
    } // partition_exact_boundary_not_oversized

    // @brief Piece one pixel wider than content is oversized.
    #[test]
    fn partition_one_pixel_over_width() {
        let pieces = vec![piece(501, 100, "just_over", 0)];
        let (over, rem) = partition_oversized_pieces(&pieces, 500, 500);
        assert_eq!(over, vec![0]);
        assert!(rem.is_empty());
    } // partition_one_pixel_over_width

    // @brief Empty piece list returns two empty vecs.
    #[test]
    fn partition_empty_pieces() {
        let pieces: Vec<PieceRect> = vec![];
        let (over, rem) = partition_oversized_pieces(&pieces, 500, 500);
        assert!(over.is_empty());
        assert!(rem.is_empty());
    } // partition_empty_pieces

    // -----------------------------------------------------------------------
    // build_oversized_svg
    // -----------------------------------------------------------------------

    // @brief Empty oversized_indices → Err (nothing to pack).
    #[test]
    fn build_empty_indices_errors() {
        let dom = flat_dom_with_n_groups(2);
        let all_pieces = vec![piece(900, 900, "big", 0)];
        let result = build_oversized_svg(
            &dom, &all_pieces, &[], 5, &[0], 24, 24, 24, 24,
        );
        assert!(result.is_err(), "empty indices should error");
    } // build_empty_indices_errors

    // @brief Out-of-range index → Err.
    #[test]
    fn build_out_of_range_index_errors() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(900, 900, "big", 0)];
        let result = build_oversized_svg(
            &dom, &all_pieces, &[99], 5, &[0], 24, 24, 24, 24,
        );
        assert!(result.is_err(), "out-of-range index should error");
    } // build_out_of_range_index_errors

    // @brief Single oversized piece → SVG has correct root dimensions.
    #[test]
    fn build_single_piece_root_dimensions() {
        // One piece 900 × 600 px.  Margins = 24 px each.
        // Expected svg_w = 900 + 24 + 24 = 948
        // Expected svg_h = 600 + 24 + 24 = 648 (one piece fills content_h exactly)
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(900, 600, "big", 0)];
        let doc = build_oversized_svg(
            &dom, &all_pieces, &[0], 0, &[0], 24, 24, 24, 24,
        ).expect("build ok");

        let w: u32 = doc.root.attributes.get("width")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        let h: u32 = doc.root.attributes.get("height")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        assert_eq!(w, 948, "svg width");
        assert_eq!(h, 648, "svg height");
    } // build_single_piece_root_dimensions

    // @brief backgroundRect and contentRect are present with correct attributes.
    #[test]
    fn build_has_background_and_content_rect() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(800, 500, "big", 0)];
        let doc = build_oversized_svg(
            &dom, &all_pieces, &[0], 0, &[0], 24, 24, 24, 24,
        ).expect("build ok");

        let svg_str = doc.to_string();
        assert!(svg_str.contains("id=\"backgroundRect\""), "backgroundRect missing");
        assert!(svg_str.contains("id=\"contentRect\""),   "contentRect missing");
        assert!(svg_str.contains("id=\"Rectangles\""),    "Rectangles group missing");
    } // build_has_background_and_content_rect

    // @brief contentRect is offset by margins (x=ml, y=mt).
    #[test]
    fn build_content_rect_margin_offset() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(900, 600, "big", 0)];
        let doc = build_oversized_svg(
            &dom, &all_pieces, &[0], 0, &[0], 30, 40, 30, 40,
        ).expect("build ok");

        let cr_x: u32 = doc.get_attr_by_id("contentRect", "x")
            .and_then(|s| s.parse().ok()).unwrap_or(999);
        let cr_y: u32 = doc.get_attr_by_id("contentRect", "y")
            .and_then(|s| s.parse().ok()).unwrap_or(999);
        assert_eq!(cr_x, 30, "contentRect x should equal ml_px");
        assert_eq!(cr_y, 40, "contentRect y should equal mt_px");
    } // build_content_rect_margin_offset

    // @brief Two oversized pieces → both <g> elements appear in the output SVG.
    #[test]
    fn build_two_pieces_both_placed() {
        // Two pieces of 800×400; flat_dom has groups at index 0 and 1.
        let dom = flat_dom_with_n_groups(2);
        let all_pieces = vec![
            piece(800, 400, "p0", 0),
            piece(800, 400, "p1", 1),
        ];
        let doc = build_oversized_svg(
            &dom, &all_pieces, &[0, 1], 5, &[0], 24, 24, 24, 24,
        ).expect("build ok");

        // Both piece groups should appear in the SVG string.
        let svg_str = doc.to_string();
        assert!(svg_str.contains("id=\"p0\""), "piece p0 missing from oversized SVG");
        assert!(svg_str.contains("id=\"p1\""), "piece p1 missing from oversized SVG");
    } // build_two_pieces_both_placed

    // @brief Placed pieces have a transform attribute (translate transform applied).
    #[test]
    fn build_pieces_have_transform() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(800, 500, "big", 0)];
        let doc = build_oversized_svg(
            &dom, &all_pieces, &[0], 0, &[0], 24, 24, 24, 24,
        ).expect("build ok");

        // The piece <g> should carry a transform="translate(...)" attribute.
        let svg_str = doc.to_string();
        assert!(
            svg_str.contains("transform=\"translate("),
            "placed piece missing transform attribute; svg={svg_str}"
        );
    } // build_pieces_have_transform

    // @brief backgroundRect fills the full SVG area (x=0, y=0, w=svg_w, h=svg_h).
    #[test]
    fn build_background_rect_fills_svg() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(900, 600, "big", 0)];
        let doc = build_oversized_svg(
            &dom, &all_pieces, &[0], 0, &[0], 24, 24, 24, 24,
        ).expect("build ok");

        let svg_w: u32 = doc.root.attributes.get("width")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        let svg_h: u32 = doc.root.attributes.get("height")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        let bg_w: u32 = doc.get_attr_by_id("backgroundRect", "width")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        let bg_h: u32 = doc.get_attr_by_id("backgroundRect", "height")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        assert_eq!(bg_w, svg_w, "backgroundRect width should match SVG root width");
        assert_eq!(bg_h, svg_h, "backgroundRect height should match SVG root height");
    } // build_background_rect_fills_svg

    // @brief Sort: larger piece is placed before smaller piece in the bin.
    // Verifies that the largest piece (by area) is packed first, so it lands
    // in the upper portion of the bin (y=0), not below the smaller piece.
    #[test]
    fn build_sorts_largest_first() {
        // Two pieces: big 800×800, small 100×100.
        // After sort: big goes first.  With bin_w=800, big lands at y=0.
        let dom = flat_dom_with_n_groups(2);
        let all_pieces = vec![
            piece(100, 100, "small", 0), // index 0 — smaller
            piece(800, 800, "big",   1), // index 1 — bigger (should pack first)
        ];
        let doc = build_oversized_svg(
            &dom, &all_pieces, &[0, 1], 0, &[0], 0, 0, 0, 0,
        ).expect("build ok");

        // The oversized SVG height should be > 800 (both pieces packed)
        let h: u32 = doc.root.attributes.get("height")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        assert!(h >= 900, "svg height should fit both pieces; got h={h}");
    } // build_sorts_largest_first

    // @brief Only the specified oversized indices are placed; remaining are skipped.
    //
    // flat_dom groups have ids "p0", "p1", "p2" (from flat_dom_with_n_groups).
    // PieceRect.group_index determines which flat_dom <g> is copied.
    // The output SVG carries the flat_dom group id, NOT the PieceRect.id.
    #[test]
    fn build_only_oversized_indices_placed() {
        // flat_dom has 3 groups with ids "p0", "p1", "p2".
        // Only the piece at group_index=1 (id="p1" in flat_dom) is oversized.
        let dom = flat_dom_with_n_groups(3);
        let all_pieces = vec![
            piece(200, 200, "p0", 0), // remaining (not oversized) — group_index=0 → flat_dom "p0"
            piece(900, 600, "p1", 1), // oversized — group_index=1 → flat_dom "p1"
            piece(300, 300, "p2", 2), // remaining (not oversized) — group_index=2 → flat_dom "p2"
        ];
        let doc = build_oversized_svg(
            &dom, &all_pieces, &[1], 0, &[0], 24, 24, 24, 24,
        ).expect("build ok");

        let svg_str = doc.to_string();
        // The oversized piece's flat_dom group (id="p1") must appear in the output.
        assert!(svg_str.contains("id=\"p1\""),    "oversized piece (group p1) should be placed");
        // The non-oversized pieces' groups must NOT appear.
        assert!(!svg_str.contains("id=\"p0\""), "non-oversized piece p0 should NOT be placed");
        assert!(!svg_str.contains("id=\"p2\""), "non-oversized piece p2 should NOT be placed");
    } // build_only_oversized_indices_placed

} // mod tests
