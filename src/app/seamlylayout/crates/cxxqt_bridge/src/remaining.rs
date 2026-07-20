// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file remaining.rs
// @brief Phase B of the "sheets" paper_type — Task L.2.2.
//
// Takes the remaining (non-oversized) pieces and packs them onto one or more
// sheet-sized SVG documents.  Pieces that do not fit together on a single sheet
// overflow into a second sheet, then a third, etc., until all pieces are placed.
// Each resulting SVG is identical in canvas size (margin + contentRect) and can
// be rendered to a single-page PDF directly (no tiling needed, since each piece
// fits within the sheet's contentRect by definition of "remaining").
//
// Algorithm (from Task L.2.2):
//   1. Sort remaining pieces by area, largest first (MaxRects heuristic).
//   2. Try to pack all remaining pieces onto a single content-rect-sized bin.
//   3. When a piece cannot be placed on the current bin, defer it to the next bin.
//   4. Repeat for the deferred pieces until all are placed or the MAX_BINS guard fires.
//   5. For each bin, assemble a sheet SVG with backgroundRect + contentRect + piece groups.
//   6. Return the Vec<Document>; callers render each to a single-page PDF.
//
// Public API:
//   build_remaining_svgs(
//       flat_dom, all_pieces, remaining_indices,
//       content_w_px, content_h_px, gap_px, trial_angles_deg,
//       ml_px, mt_px, mr_px, mb_px,
//   ) -> Result<Vec<svg_dom::Document>, String>

use svg_dom::Document;
use xmltree::{Element, XMLNode};
use crate::piece_extractor::PieceRect;

// Maximum number of sheet bins created before giving up.
// Prevents infinite loops when a piece can never be placed (e.g., gap_px exceeds bin).
const MAX_BINS: usize = 200;

// ---------------------------------------------------------------------------
// build_remaining_svgs
// ---------------------------------------------------------------------------

// @brief Pack remaining (non-oversized) pieces onto multiple sheet SVG documents.
//
// All pieces in `remaining_indices` fit within the sheet contentRect by definition
// (partition_oversized_pieces guarantees this).  The only reason multiple bins are
// needed is when pieces don't pack together efficiently enough to share a sheet.
//
// The returned Vec contains one Document per sheet.  An empty Vec is returned if
// `remaining_indices` is empty (not an error; callers that need at least one sheet
// should check the slice length before calling).
//
// Steps:
//   1. Validate indices.
//   2. Sort pieces by area (largest first) for better MaxRects efficiency.
//   3. Run the iterative bin-assignment loop (assign_to_bins).
//   4. For each bin, assemble a sheet SVG via build_sheet_doc.
//
// @param flat_dom          Pre-processed SVG DOM (flatten→verticalize→translate→flatten).
// @param all_pieces        All PieceRect entries; `remaining_indices` index into this.
// @param remaining_indices Indices of pieces that fit on a single sheet.
// @param content_w_px      Sheet contentRect width in pixels.
// @param content_h_px      Sheet contentRect height in pixels.
// @param gap_px            Inter-piece clearance in pixels.
// @param trial_angles_deg  Rotation trial set in degrees (from LayoutSettings).
// @param ml_px             Left  margin in pixels.
// @param mt_px             Top   margin in pixels.
// @param mr_px             Right margin in pixels.
// @param mb_px             Bottom margin in pixels.
// @return Vec of sheet SVG Documents on success; Err with a descriptive message on failure.
pub fn build_remaining_svgs(
    flat_dom: &Document,
    all_pieces: &[PieceRect],
    remaining_indices: &[usize],
    content_w_px: u32,
    content_h_px: u32,
    gap_px: u32,
    trial_angles_deg: &[u16],
    ml_px: u32,
    mt_px: u32,
    mr_px: u32,
    mb_px: u32,
) -> Result<Vec<Document>, String> {

    // Empty remaining set is valid — caller handles it.
    if remaining_indices.is_empty() {
        return Ok(Vec::new());
    } // if empty

    // Validate all indices before dereferencing.
    for &idx in remaining_indices {
        if idx >= all_pieces.len() {
            return Err(format!(
                "build_remaining_svgs: index {idx} out of range (all_pieces.len()={}).",
                all_pieces.len()
            ));
        } // if out of range
    } // for idx

    // Sort pieces by area, largest first.
    // sorted_pending holds (orig_piece_idx, Rect) pairs.
    let mut sorted_pending: Vec<(usize, packing::Rect)> = remaining_indices
        .iter()
        .map(|&i| (i, all_pieces[i].rect))
        .collect();
    sorted_pending.sort_by(|(a, _), (b, _)| {
        let area_a = all_pieces[*a].rect.w as u64 * all_pieces[*a].rect.h as u64;
        let area_b = all_pieces[*b].rect.w as u64 * all_pieces[*b].rect.h as u64;
        area_b.cmp(&area_a) // descending area — largest first
    }); // sort

    log::debug!(
        "build_remaining_svgs: {} remaining pieces; bin={}×{}; gap={}; angles={:?}",
        sorted_pending.len(), content_w_px, content_h_px, gap_px, trial_angles_deg
    );

    // Iteratively assign pieces to bins.
    let total_remaining = sorted_pending.len();
    let bins: Vec<Vec<(usize, packing::Placed)>> = assign_to_bins(
        sorted_pending,
        content_w_px, content_h_px,
        gap_px, trial_angles_deg,
    );
    let placed_count: usize = bins.iter().map(Vec::len).sum();
    if placed_count != total_remaining {
        return Err(format!(
            "build_remaining_svgs: only {placed_count} of {total_remaining} remaining pieces were placed."
        ));
    }


    log::debug!("build_remaining_svgs: {} sheet bins produced.", bins.len());

    // Build an SVG document for each bin.
    let mut docs: Vec<Document> = Vec::with_capacity(bins.len());
    for (i, bin_placements) in bins.iter().enumerate() {
        let doc = build_sheet_doc(
            flat_dom, all_pieces, bin_placements,
            content_w_px, content_h_px,
            ml_px, mt_px, mr_px, mb_px,
        ).map_err(|e| format!("build_remaining_svgs: bin {i}: {e}"))?;
        docs.push(doc);
    } // for bin

    Ok(docs)
} // fn build_remaining_svgs

// ---------------------------------------------------------------------------
// Private: assign_to_bins
// ---------------------------------------------------------------------------

// @brief Iteratively assign pieces to sheet-sized bins using MaxRects.
//
// Each outer iteration attempts to pack all pending pieces.  When the packer
// reports that a piece does not fit, that piece is deferred to the next bin.
// This continues until all pieces are assigned or MAX_BINS is reached.
//
// The sort order of pieces reaching each bin is preserved from the input
// (area-descending from the caller) so each bin gets the best packing chance.
//
// @param sorted_pending  (orig_piece_idx, Rect) pairs, area-desc ordered.
// @param bin_w           Content bin width in pixels.
// @param bin_h           Content bin height in pixels.
// @param gap_px          Inter-piece gap in pixels.
// @param trial_angles    Rotation trial angles in degrees.
// @return Vec of bins; each bin is a Vec<(orig_piece_idx, Placed)>.
fn assign_to_bins(
    sorted_pending: Vec<(usize, packing::Rect)>,
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    trial_angles: &[u16],
) -> Vec<Vec<(usize, packing::Placed)>> {

    let mut pending = sorted_pending;
    let mut bins: Vec<Vec<(usize, packing::Placed)>> = Vec::new();

    // Outer loop: one iteration per sheet bin.
    while !pending.is_empty() && bins.len() < MAX_BINS {

        // Move all pending pieces into the current bin attempt.
        let mut current: Vec<(usize, packing::Rect)> = std::mem::take(&mut pending);
        let mut next_pending: Vec<(usize, packing::Rect)> = Vec::new();

        // Inner loop: retry packing, deferring one failing piece per iteration,
        // until either all remaining pieces fit or the bin is fully drained.
        let bin_placements: Vec<(usize, packing::Placed)> = loop {

            if current.is_empty() {
                // All pieces for this bin were deferred — break with empty bin.
                break Vec::new();
            } // if empty

            // Build the rect slice for the packer; indices align with `current`.
            let rects: Vec<packing::Rect> = current.iter().map(|(_, r)| *r).collect();

            match packing::pack_pieces(bin_w, bin_h, gap_px, &rects, trial_angles) {
                Ok((placed, _)) => {
                    // All current pieces placed — remap placed.id to original piece index.
                    let result: Vec<(usize, packing::Placed)> = placed
                        .iter()
                        .map(|p| {
                            let orig_idx = current[p.id].0;
                            let remapped = packing::Placed { id: orig_idx, ..*p };
                            (orig_idx, remapped)
                        })
                        .collect();
                    break result;
                } // Ok

                Err(e) => {
                    // One piece couldn't be placed — defer it to the next bin.
                    let fail_id = match e {
                        packing::PackError::NoSpace { id }       => id,
                        packing::PackError::TooLarge  { id, .. } => id,
                        packing::PackError::SearchLimit { id }   => id,
                    }; // fail_id

                    // Clamp defensively in case the packer returns an out-of-range id.
                    let fail_id = fail_id.min(current.len().saturating_sub(1));

                    log::debug!(
                        "assign_to_bins: bin {}: piece at current[{}] (orig_idx={}) deferred to next bin.",
                        bins.len(), fail_id, current[fail_id].0
                    );

                    let deferred = current.remove(fail_id);
                    next_pending.push(deferred);
                } // Err
            } // match
        }; // loop

        // Restore remaining pieces for the next outer iteration.
        pending = next_pending;

        // Safety: if no progress was made (all pieces deferred, empty bin),
        // stop to avoid an infinite retry loop.
        if bin_placements.is_empty() {
            log::warn!(
                "assign_to_bins: {} pieces could not be placed on a fresh bin — stopping.",
                pending.len()
            );
            break;
        } // if empty

        bins.push(bin_placements);
    } // while

    // Log any pieces left unplaced after MAX_BINS.
    if !pending.is_empty() {
        log::warn!(
            "assign_to_bins: {} pieces remain unplaced after {} bins.",
            pending.len(), bins.len()
        );
    } // if unplaced

    bins
} // fn assign_to_bins

// ---------------------------------------------------------------------------
// Private: build_sheet_doc
// ---------------------------------------------------------------------------

// @brief Assemble a single sheet SVG document from a bin's placed pieces.
//
// The document dimensions are (content_w + ml + mr) × (content_h + mt + mb),
// matching the physical sheet with margins.  A backgroundRect and contentRect
// are added in a <g id="Rectangles"> group, followed by the piece <g> elements
// from flat_dom with position/rotation transforms applied.
//
// Transform composition mirrors `layout_assembler::create_layout` and
// `oversized::build_oversized_svg` exactly:
//   0°  → translate(target - origin)
//   θ°  → translate(target - min_rotated_corner) rotate(θ cx cy)
//
// @param flat_dom         Source DOM; piece <g> elements cloned from here.
// @param all_pieces       All PieceRect entries for dimension and group_index lookup.
// @param bin_placements   (orig_piece_idx, Placed) pairs for this bin.
// @param content_w_px     Sheet content width in pixels.
// @param content_h_px     Sheet content height in pixels.
// @param ml_px            Left  margin in pixels.
// @param mt_px            Top   margin in pixels.
// @param mr_px            Right margin in pixels.
// @param mb_px            Bottom margin in pixels.
// @return Sheet SVG Document on success; Err on failure.
fn build_sheet_doc(
    flat_dom: &Document,
    all_pieces: &[PieceRect],
    bin_placements: &[(usize, packing::Placed)],
    content_w_px: u32,
    content_h_px: u32,
    ml_px: u32,
    mt_px: u32,
    mr_px: u32,
    mb_px: u32,
) -> Result<Document, String> {

    // Full SVG canvas dimensions include margins.
    let svg_w: u32 = content_w_px + ml_px + mr_px;
    let svg_h: u32 = content_h_px + mt_px + mb_px;

    // Build SVG root element.
    let mut svg_root = Element {
        name:       "svg".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  Some("http://www.w3.org/2000/svg".to_string()),
        prefix:     None,
        namespaces: None,
    };
    svg_root.attributes.insert("xmlns".to_string(), "http://www.w3.org/2000/svg".to_string());
    svg_root.attributes.insert("id".to_string(),    "remaining_sheet".to_string());
    svg_root.attributes.insert("width".to_string(),  svg_w.to_string());
    svg_root.attributes.insert("height".to_string(), svg_h.to_string());

    let mut out_doc = Document { root: svg_root };

    // backgroundRect fills the full canvas.
    let bg = make_rect("backgroundRect", 0, 0, svg_w, svg_h, "white", "none");

    // contentRect marks the packing area, inset by margins.
    let mut cr = make_rect("contentRect", ml_px, mt_px, content_w_px, content_h_px, "none", "black");
    cr.attributes.insert("stroke-width".to_string(), "1".to_string());

    // <g id="Rectangles"> group follows the same convention as layout_assembler and oversized.rs.
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

    // Collect top-level <g> elements from flat_dom for group_index lookup.
    let group_elements: Vec<&Element> = flat_dom
        .root
        .children
        .iter()
        .filter_map(|node| match node {
            XMLNode::Element(e) if e.name == "g" => Some(e),
            _ => None,
        })
        .collect();

    // Place each piece into the sheet SVG.
    for (orig_idx, placed) in bin_placements {
        let piece: &PieceRect = match all_pieces.get(*orig_idx) {
            Some(p) => p,
            None => {
                log::debug!(
                    "build_sheet_doc: orig_idx {} out of range (all_pieces.len={}); skipping.",
                    orig_idx, all_pieces.len()
                );
                continue;
            } // None
        }; // piece

        let orig_group: &Element = match group_elements.get(piece.group_index) {
            Some(g) => *g,
            None => {
                log::debug!(
                    "build_sheet_doc: group_index {} out of range (groups.len={}); skipping.",
                    piece.group_index, group_elements.len()
                );
                continue;
            } // None
        }; // orig_group

        // Target slot origin in canvas coordinates.
        let target_x: f64 = (ml_px + placed.x) as f64;
        let target_y: f64 = (mt_px + placed.y) as f64;
        let orig_x:   f64 = piece.origin_x;
        let orig_y:   f64 = piece.origin_y;
        let upright_w: f64 = piece.rect.w as f64; // UPRIGHT bbox width
        let upright_h: f64 = piece.rect.h as f64; // UPRIGHT bbox height

        // Rotation transform mirroring layout_assembler::create_layout and oversized.rs.
        // SVG applies "translate rotate" right-to-left: rotation in local piece space first,
        // then translation moves the rotated AABB minimum corner to the target slot.
        let transform = match placed.rotation_deg {
            0 => {
                // No rotation: simple translate from piece origin to slot origin.
                let tx = target_x - orig_x;
                let ty = target_y - orig_y;
                format!("translate({tx:.4} {ty:.4})")
            } // 0°

            deg => {
                // Rotation center = UPRIGHT bbox center.
                let cx = orig_x + upright_w / 2.0;
                let cy = orig_y + upright_h / 2.0;
                let theta = (deg as f64).to_radians();
                let (ct, st) = (theta.cos(), theta.sin());

                // Rotate all four UPRIGHT bbox corners about (cx, cy).
                let corners: [(f64, f64); 4] = [
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
                    let rx = cx + dx * ct - dy * st;
                    let ry = cy + dx * st + dy * ct;
                    min_rx = min_rx.min(rx);
                    min_ry = min_ry.min(ry);
                } // for corner

                // Translate so rotated AABB min lands at the target slot origin.
                let tx = target_x - min_rx;
                let ty = target_y - min_ry;
                format!("translate({tx:.4} {ty:.4}) rotate({deg} {cx:.4} {cy:.4})")
            } // non-zero rotation
        }; // match rotation_deg

        // Clone the piece <g> from flat_dom and attach the transform.
        let mut group_clone = orig_group.clone();
        group_clone.attributes.insert("transform".to_string(), transform);

        out_doc.root.children.push(XMLNode::Element(group_clone));
    } // for (orig_idx, placed)

    Ok(out_doc)
} // fn build_sheet_doc

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// @brief Build a plain SVG <rect> element.
//
// @param id     Value for the `id` attribute.
// @param x      Left edge in pixels.
// @param y      Top edge in pixels.
// @param w      Width in pixels.
// @param h      Height in pixels.
// @param fill   Fill color string (e.g. "white", "none").
// @param stroke Stroke color string (e.g. "none", "black").
// @return New Element ready to push into a parent's children vec.
fn make_rect(
    id:     &str,
    x:      u32,
    y:      u32,
    w:      u32,
    h:      u32,
    fill:   &str,
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

    // Helper: build a PieceRect with given dimensions.
    fn piece(w: u32, h: u32, id: &str, group_index: usize) -> PieceRect {
        PieceRect {
            rect:        packing::Rect::new(w, h),
            id:          id.to_string(),
            origin_x:    0.0,
            origin_y:    0.0,
            group_index,
        }
    } // fn piece

    // Helper: build a minimal flat_dom with N empty <g> groups.
    fn flat_dom_with_n_groups(n: usize) -> Document {
        let parts: Vec<String> = (0..n)
            .map(|i| {
                format!(
                    "<g id=\"p{i}\"><path d=\"M 0 0 L {w} 0 L {w} {h} Z\"/></g>",
                    w = (i + 1) * 100,
                    h = (i + 1) * 100,
                )
            })
            .collect();
        let svg_str = format!(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"2000\" height=\"2000\">{}</svg>",
            parts.join("")
        );
        Document::parse(&svg_str).expect("parse ok")
    } // fn flat_dom_with_n_groups

    // -----------------------------------------------------------------------
    // build_remaining_svgs — structural tests
    // -----------------------------------------------------------------------

    // @brief Empty remaining_indices returns an empty Vec (not an error).
    #[test]
    fn remaining_empty_indices_returns_empty_vec() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(300, 300, "p0", 0)];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[], 600, 600, 0, &[0], 24, 24, 24, 24,
        ).expect("ok for empty input");
        assert!(docs.is_empty(), "empty indices should return empty vec");
    } // remaining_empty_indices_returns_empty_vec

    // @brief Out-of-range index returns Err.
    #[test]
    fn remaining_out_of_range_index_errors() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(300, 300, "p0", 0)];
        let result = build_remaining_svgs(
            &dom, &all_pieces, &[99], 600, 600, 0, &[0], 24, 24, 24, 24,
        );
        assert!(result.is_err(), "out-of-range index should return Err");
    } // remaining_out_of_range_index_errors

    // @brief Single piece produces exactly one SVG document.
    #[test]
    fn remaining_single_piece_one_sheet() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(300, 200, "p0", 0)];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[0], 600, 600, 0, &[0], 24, 24, 24, 24,
        ).expect("ok");
        assert_eq!(docs.len(), 1, "one piece → one sheet SVG");
    } // remaining_single_piece_one_sheet

    // @brief Two pieces that both fit produce one SVG document.
    #[test]
    fn remaining_two_fitting_pieces_one_sheet() {
        // Two 200×200 pieces should both fit in a 600×600 bin.
        let dom = flat_dom_with_n_groups(2);
        let all_pieces = vec![
            piece(200, 200, "p0", 0),
            piece(200, 200, "p1", 1),
        ];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[0, 1], 600, 600, 0, &[0], 24, 24, 24, 24,
        ).expect("ok");
        assert_eq!(docs.len(), 1, "two fitting pieces → one sheet SVG");
    } // remaining_two_fitting_pieces_one_sheet

    // @brief Pieces that don't all fit on one sheet overflow to a second.
    //
    // Two 400×400 pieces in a 500×500 bin cannot both fit:
    //   col 0: 400 fits; col 1: 400 > (500-400)=100 → deferred.
    // Result: 2 bins, each holding one piece.
    #[test]
    fn remaining_overflow_creates_second_sheet() {
        let dom = flat_dom_with_n_groups(2);
        let all_pieces = vec![
            piece(400, 400, "p0", 0),
            piece(400, 400, "p1", 1),
        ];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[0, 1], 500, 500, 0, &[0], 0, 0, 0, 0,
        ).expect("ok");
        assert_eq!(docs.len(), 2, "two overflowing pieces → two sheets");
    } // remaining_overflow_creates_second_sheet

    // @brief Each sheet has backgroundRect, contentRect, and Rectangles group.
    #[test]
    fn remaining_sheet_has_structural_elements() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(300, 200, "p0", 0)];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[0], 600, 600, 0, &[0], 24, 24, 24, 24,
        ).expect("ok");
        let svg_str = docs[0].to_string();
        assert!(svg_str.contains("id=\"backgroundRect\""), "missing backgroundRect");
        assert!(svg_str.contains("id=\"contentRect\""),   "missing contentRect");
        assert!(svg_str.contains("id=\"Rectangles\""),    "missing Rectangles group");
    } // remaining_sheet_has_structural_elements

    // @brief Sheet root dimensions = (content_w + ml + mr) × (content_h + mt + mb).
    #[test]
    fn remaining_sheet_root_dimensions() {
        // content 600×400, margins 30 left/right, 40 top/bottom
        // expected: svg_w = 600 + 30 + 30 = 660, svg_h = 400 + 40 + 40 = 480
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(300, 200, "p0", 0)];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[0], 600, 400, 0, &[0], 30, 40, 30, 40,
        ).expect("ok");

        let w: u32 = docs[0].root.attributes.get("width")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        let h: u32 = docs[0].root.attributes.get("height")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        assert_eq!(w, 660, "svg width");
        assert_eq!(h, 480, "svg height");
    } // remaining_sheet_root_dimensions

    // @brief contentRect is offset by margins (x=ml, y=mt).
    #[test]
    fn remaining_sheet_content_rect_margin_offset() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(300, 200, "p0", 0)];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[0], 600, 400, 0, &[0], 30, 40, 30, 40,
        ).expect("ok");

        let cr_x: u32 = docs[0].get_attr_by_id("contentRect", "x")
            .and_then(|s| s.parse().ok()).unwrap_or(999);
        let cr_y: u32 = docs[0].get_attr_by_id("contentRect", "y")
            .and_then(|s| s.parse().ok()).unwrap_or(999);
        assert_eq!(cr_x, 30, "contentRect x should equal ml_px");
        assert_eq!(cr_y, 40, "contentRect y should equal mt_px");
    } // remaining_sheet_content_rect_margin_offset

    // @brief backgroundRect fills the full SVG canvas area.
    #[test]
    fn remaining_background_rect_fills_svg() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(300, 200, "p0", 0)];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[0], 600, 400, 0, &[0], 30, 40, 30, 40,
        ).expect("ok");

        let svg_w: u32 = docs[0].root.attributes.get("width")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        let svg_h: u32 = docs[0].root.attributes.get("height")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        let bg_w: u32 = docs[0].get_attr_by_id("backgroundRect", "width")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        let bg_h: u32 = docs[0].get_attr_by_id("backgroundRect", "height")
            .and_then(|s| s.parse().ok()).unwrap_or(0);
        assert_eq!(bg_w, svg_w, "backgroundRect width must match SVG root width");
        assert_eq!(bg_h, svg_h, "backgroundRect height must match SVG root height");
    } // remaining_background_rect_fills_svg

    // @brief Placed pieces carry a transform attribute.
    #[test]
    fn remaining_pieces_have_transform() {
        let dom = flat_dom_with_n_groups(1);
        let all_pieces = vec![piece(300, 200, "p0", 0)];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[0], 600, 600, 0, &[0], 24, 24, 24, 24,
        ).expect("ok");

        let svg_str = docs[0].to_string();
        assert!(
            svg_str.contains("transform=\"translate("),
            "placed piece should have a translate transform; svg={svg_str}"
        );
    } // remaining_pieces_have_transform

    // @brief Correct flat_dom group element appears in the output for each piece.
    //
    // flat_dom groups have ids "p0", "p1", "p2" (from flat_dom_with_n_groups).
    // PieceRect.group_index determines which flat_dom <g> is copied.
    #[test]
    fn remaining_correct_group_ids_in_output() {
        // Only pieces at group_index=0 ("p0") and group_index=2 ("p2") are remaining.
        let dom = flat_dom_with_n_groups(3);
        let all_pieces = vec![
            piece(200, 200, "a", 0), // group_index=0 → "p0"
            piece(200, 200, "b", 1), // not in remaining_indices
            piece(200, 200, "c", 2), // group_index=2 → "p2"
        ];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[0, 2], 600, 600, 0, &[0], 24, 24, 24, 24,
        ).expect("ok");

        assert_eq!(docs.len(), 1, "both fit on one sheet");
        let svg_str = docs[0].to_string();
        assert!(svg_str.contains("id=\"p0\""),  "group p0 should appear");
        assert!(svg_str.contains("id=\"p2\""),  "group p2 should appear");
        assert!(!svg_str.contains("id=\"p1\""), "group p1 should NOT appear");
    } // remaining_correct_group_ids_in_output

    // @brief Multiple pieces overflowing to two sheets: all piece groups present across both sheets.
    #[test]
    fn remaining_overflow_all_groups_present_across_sheets() {
        // Three 400×400 pieces in a 500×500 bin — only one fits per sheet.
        let dom = flat_dom_with_n_groups(3);
        let all_pieces = vec![
            piece(400, 400, "p0", 0),
            piece(400, 400, "p1", 1),
            piece(400, 400, "p2", 2),
        ];
        let docs = build_remaining_svgs(
            &dom, &all_pieces, &[0, 1, 2], 500, 500, 0, &[0], 0, 0, 0, 0,
        ).expect("ok");

        assert_eq!(docs.len(), 3, "three overflowing pieces → three sheets");

        // Collect all group ids across all sheets.
        let all_svg: String = docs.iter().map(|d| d.to_string()).collect::<Vec<_>>().join("\n");
        assert!(all_svg.contains("id=\"p0\""), "p0 must appear on some sheet");
        assert!(all_svg.contains("id=\"p1\""), "p1 must appear on some sheet");
        assert!(all_svg.contains("id=\"p2\""), "p2 must appear on some sheet");
    } // remaining_overflow_all_groups_present_across_sheets

} // mod tests
