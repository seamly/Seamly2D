// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file fold_line_edge.rs
// @brief Puts the fold line of each piece cut on the fold on the left content edge.
//
// Fabric: the left edge is the fold, with no margin, so one cut gives the whole
// piece. Paper: the left content edge, inside the margin, where the user lays
// the fold of the fabric. Runs on the pre-processed DOM, before `translate_dom`:
// pieces are direct `<g>` children of the root, grain up, with transforms baked.
//
// Per piece with a vertical fold line, the packer angle that brings the fold
// line to the left of the piece:
//   - 0 when the piece lies right of its fold line, 180 when it lies left of it;
//   - when the layout mode does not allow that angle, the piece is mirrored
//     across its fold line and takes the other angle. The cut piece is the
//     same: the two halves of a piece cut on the fold are mirror images. Text
//     keeps reading forward; only its position is mirrored.
// The packer then puts the piece against the left bin edge (`Rect::with_left_edge`).
//
// Not pinned: a piece-set (Nested: its sizes are centred, so their fold lines
// differ), a fold line that is not vertical, and an outline on both sides of
// its fold line. The last two get a warning line.

use std::collections::HashMap;

use geometry::{Path, PathSegment, Point};
use xmltree::Element;

use crate::piece_extractor::PieceRect;

// Largest distance, in user units, between the fold line and the outline edge
// on the fold side: the fold line lies on the outline.
const ON_EDGE_TOLERANCE_UU: f32 = 1.0;

// Largest tilt of a vertical fold line: tan(1°).
const VERTICAL_TOLERANCE: f32 = 0.017_455;

// @brief Which pieces go on the left edge, at which packer angle.
#[derive(Debug, Default, Clone, PartialEq)]
pub(crate) struct FoldEdgePlan {
    // Packer angle per piece, by the piece's index among the root's `<g>` children
    // (`PieceRect::group_index`).
    pub angles: HashMap<usize, u16>,
    // One line per piece with a fold line that stays off the edge: its name and why.
    pub warnings: Vec<String>,
} // struct FoldEdgePlan

// @brief Turn or mirror each piece with a fold line so its fold line can go on the left edge.
// @param doc               Pre-processed DOM; a mirrored piece changes in place.
// @param trial_angles_deg  The layout mode's trial angles.
// @return The packer angle per piece, and the pieces that stay off the edge.
pub(crate) fn plan_fold_line_edge(doc: &mut svg_dom::Document, trial_angles_deg: &[u16]) -> FoldEdgePlan {
    let mut plan = FoldEdgePlan::default();
    let mut mirrored = false;
    let groups = doc.root.children.iter_mut().filter_map(|n| n.as_mut_element()).filter(|e| e.name == "g");
    for (index, piece) in groups.enumerate() {
        if svg_dom::is_piece_set(piece) {
            continue; // Nested: the sizes do not share one fold line
        } // if piece-set
        let Some((a, b)) = fold_line_ends(piece) else {
            continue; // not cut on the fold
        }; // fold line
        let name = piece_name(piece);
        let (dx, dy) = ((b.x - a.x).abs(), (b.y - a.y).abs());
        if dy < ON_EDGE_TOLERANCE_UU || dx > dy * VERTICAL_TOLERANCE {
            plan.warnings.push(format!("{name}: the fold line does not run along the grainline"));
            continue;
        } // if not vertical
        let Some(bbox) = crate::piece_extractor::piece_bbox(piece) else {
            continue; // no outline
        }; // bbox
        let fold_x = (a.x + b.x) / 2.0;

        // Side of the fold line the piece lies on.
        let fold_on_left = fold_x - bbox.min.x <= ON_EDGE_TOLERANCE_UU;
        let fold_on_right = bbox.max.x - fold_x <= ON_EDGE_TOLERANCE_UU;
        if !fold_on_left && !fold_on_right {
            plan.warnings.push(format!("{name}: the outline lies on both sides of the fold line"));
            continue;
        } // if the fold line is inside the outline

        // 0 keeps a fold line on the left; 180 turns a fold line on the right to the left.
        let (wanted, other) = if fold_on_left { (0, 180) } else { (180, 0) };
        let angle = if trial_angles_deg.contains(&wanted) {
            wanted
        } else if trial_angles_deg.contains(&other) {
            mirror_across(piece, fold_x);
            mirrored = true;
            other
        } else {
            plan.warnings.push(format!("{name}: the layout mode turns the piece off the fold"));
            continue;
        }; // angle
        plan.angles.insert(index, angle);
    } // for each piece

    if mirrored {
        svg_dom::flatten_dom(doc); // bake the mirror transforms
        for piece in doc.root.children.iter_mut().filter_map(|n| n.as_mut_element()) {
            unmirror_text(piece);
        } // for each piece
    } // if any piece was mirrored
    plan
} // fn plan_fold_line_edge

// @brief Set the left-edge angle on every planned piece.
pub(crate) fn pin_fold_line_pieces(pieces: &mut [PieceRect], plan: &FoldEdgePlan) {
    for piece in pieces.iter_mut() {
        if let Some(&angle) = plan.angles.get(&piece.group_index) {
            piece.rect = piece.rect.with_left_edge(angle);
        } // if planned
    } // for each piece
} // fn pin_fold_line_pieces

// @brief True for a `fold_line` component group, by `data-type` or by id.
fn is_fold_line(elem: &Element) -> bool {
    elem.name == "g"
        && (elem.attributes.get("data-type").is_some_and(|t| t == "fold_line")
            || elem.attributes.get("id").is_some_and(|id| id.to_lowercase().starts_with("fold_line")))
} // fn is_fold_line

// @brief First and last point of the first path of the piece's fold line.
fn fold_line_ends(piece: &Element) -> Option<(Point, Point)> {
    let fold = piece.children.iter().filter_map(|n| n.as_element()).find(|e| is_fold_line(e))?;
    let d = first_path_d(fold)?;
    let path = Path::parse_path_attribute(d).ok()?;
    let points: Vec<Point> = path.segments.iter().filter_map(segment_end).collect();
    Some((*points.first()?, *points.last()?))
} // fn fold_line_ends

// @brief `d` of the first `<path>` in `elem`'s subtree.
fn first_path_d(elem: &Element) -> Option<&str> {
    if elem.name == "path" {
        return elem.attributes.get("d").map(String::as_str);
    } // if path
    elem.children.iter().filter_map(|n| n.as_element()).find_map(first_path_d)
} // fn first_path_d

// @brief End point of a path segment; None for close.
fn segment_end(segment: &PathSegment) -> Option<Point> {
    match segment {
        PathSegment::MoveTo(p) | PathSegment::LineTo(p) => Some(*p),
        PathSegment::QuadTo { to, .. } | PathSegment::CubicTo { to, .. } | PathSegment::ArcTo { to, .. } => Some(*to),
        PathSegment::Close => None,
    } // match segment
} // fn segment_end

// @brief Name of a piece for a warning: `data-name`, else `data-letter`, else `id`.
fn piece_name(piece: &Element) -> String {
    ["data-name", "data-letter", "id"]
        .iter()
        .filter_map(|key| piece.attributes.get(*key))
        .find(|value| !value.is_empty())
        .cloned()
        .unwrap_or_default()
} // fn piece_name

// @brief Mirror the piece across the vertical line x = `fold_x`; `flatten_dom` bakes it.
fn mirror_across(piece: &mut Element, fold_x: f32) {
    let mirror = format!("matrix(-1 0 0 1 {} 0)", 2.0 * fold_x);
    let transform = match piece.attributes.get("transform") {
        Some(own) => format!("{mirror} {own}"),
        None => mirror,
    }; // transform
    piece.attributes.insert("transform".to_string(), transform);
    piece.attributes.insert("data-fold-mirrored".to_string(), "true".to_string());
} // fn mirror_across

// @brief In a mirrored piece, make each mirrored `<text>` read forward again at its mirrored place.
// @details The text matrix T becomes T · scale(-1, 1); `x` changes sign, so
//          the anchor stays where the mirror put it; `start` and `end` anchors
//          swap, so the text covers the mirrored area.
fn unmirror_text(piece: &mut Element) {
    if piece.attributes.remove("data-fold-mirrored").is_none() {
        return; // not mirrored
    } // if not mirrored
    fix_texts(piece);
} // fn unmirror_text

// @brief Recursive step of `unmirror_text`.
fn fix_texts(elem: &mut Element) {
    for child in elem.children.iter_mut().filter_map(|n| n.as_mut_element()) {
        if child.name != "text" {
            fix_texts(child);
            continue;
        } // if not text
        let Some(m) = child.attributes.get("transform").map(|t| svg_dom::parse_svg_transform(t)) else {
            continue; // no transform: the mirror left no matrix to fix
        }; // matrix
        if m.a * m.d - m.b * m.c >= 0.0 {
            continue; // reads forward already
        } // if not mirrored
        child.attributes.insert(
            "transform".to_string(),
            format!("matrix({} {} {} {} {} {})", -m.a, -m.b, m.c, m.d, m.e, m.f),
        );
        negate_x(child);
        for span in child.children.iter_mut().filter_map(|n| n.as_mut_element()).filter(|e| e.name == "tspan") {
            negate_x(span);
        } // for each tspan
        let anchor = match child.attributes.get("text-anchor").map(String::as_str) {
            Some("end") => "start",
            Some("middle") => "middle",
            _ => "end", // start is the default
        }; // anchor
        child.attributes.insert("text-anchor".to_string(), anchor.to_string());
    } // for each child
} // fn fix_texts

// @brief Change the sign of a single-number `x` attribute.
fn negate_x(elem: &mut Element) {
    if let Some(x) = elem.attributes.get("x").and_then(|v| v.trim().parse::<f32>().ok()) {
        let value = if x == 0.0 { 0.0 } else { -x };
        elem.attributes.insert("x".to_string(), value.to_string());
    } // if numeric x
} // fn negate_x

#[cfg(test)]
mod tests {
    use super::*;
    use geometry::BoundingBox;

    // @brief The piece bounding box `plan_fold_line_edge` reads.
    fn bbox_of(doc: &svg_dom::Document, id: &str) -> BoundingBox {
        let piece = doc.root.children.iter().filter_map(|n| n.as_element())
            .find(|e| e.attributes.get("id").is_some_and(|v| v == id)).expect("piece");
        crate::piece_extractor::piece_bbox(piece).expect("bbox")
    } // fn bbox_of

    // @brief One piece 100 × 200 with a fold line path `fold_d`, a label line, and a
    // plain piece without a fold line.
    fn svg(fold_d: &str) -> String {
        format!(r#"<svg xmlns="http://www.w3.org/2000/svg">
  <g id="piece_Back" data-type="piece" data-name="Back" data-on-fold="true">
    <g data-type="cutline"><path d="M0,0 L100,0 L100,200 L0,200 Z"/></g>
    <g id="fold_line_Back" data-type="fold_line"><path d="{fold_d}"/></g>
    <g data-type="piece_label"><text x="0" y="12" transform="matrix(1 0 0 1 20 50)">Back</text></g>
  </g>
  <g id="piece_Yoke" data-type="piece" data-name="Yoke">
    <g data-type="cutline"><path d="M0,0 L50,0 L50,50 L0,50 Z"/></g>
  </g>
</svg>"#)
    } // fn svg

    // @brief The fixture with its fold line on the left (x 0) or the right (x 100) edge.
    fn doc(fold_x: u32) -> svg_dom::Document {
        svg_dom::Document::parse(&svg(&format!("M{fold_x},190 L{fold_x},10"))).expect("valid SVG")
    } // fn doc

    // @brief The text of the label.
    fn label_text(doc: &svg_dom::Document) -> Element {
        fn find(elem: &Element) -> Option<&Element> {
            if elem.name == "text" {
                return Some(elem);
            } // if text
            elem.children.iter().filter_map(|n| n.as_element()).find_map(find)
        } // fn find
        find(&doc.root).expect("text").clone()
    } // fn label_text

    // @brief A fold line on the left keeps angle 0; on the right it takes 180. No piece moves.
    #[test]
    fn turn_brings_the_fold_line_left() {
        for (fold_x, angle) in [(0, 0), (100, 180)] {
            let mut d = doc(fold_x);
            let before = d.to_string();
            let plan = plan_fold_line_edge(&mut d, &[0, 180]);
            assert_eq!(plan.angles, HashMap::from([(0, angle)]), "fold at x {fold_x}");
            assert!(plan.warnings.is_empty());
            assert_eq!(d.to_string(), before, "fold at x {fold_x}: nothing mirrored");
        } // for each side
    } // turn_brings_the_fold_line_left

    // @brief With nap up (0 only), a fold line on the right is mirrored to the left;
    // the text reads forward and covers the mirrored area.
    #[test]
    fn mirror_brings_the_fold_line_left_when_the_mode_forbids_the_turn() {
        let mut d = doc(100);
        let plan = plan_fold_line_edge(&mut d, &[0]);
        assert_eq!(plan.angles, HashMap::from([(0, 0)]));
        // Mirrored across x 100: the outline now spans x 100..200, the fold line on its left.
        let bbox = bbox_of(&d, "piece_Back");
        assert_eq!((bbox.min.x, bbox.max.x), (100.0, 200.0));
        assert!(!d.to_string().contains("data-fold-mirrored"), "marker removed");

        // The text started at x 20 and ran right; mirrored, it ends at x 180.
        let text = label_text(&d);
        let m = svg_dom::parse_svg_transform(&text.attributes["transform"]);
        assert!(m.a * m.d - m.b * m.c > 0.0, "text reads forward: {:?}", text.attributes["transform"]);
        assert_eq!((m.e, m.f), (180.0, 50.0));
        assert_eq!(text.attributes.get("text-anchor").map(String::as_str), Some("end"));

        // With nap down (180 only), a fold line on the left is mirrored to the right, then turned.
        let mut down = doc(0);
        assert_eq!(plan_fold_line_edge(&mut down, &[180]).angles, HashMap::from([(0, 180)]));
        let bbox = bbox_of(&down, "piece_Back");
        assert_eq!((bbox.min.x, bbox.max.x), (-100.0, 0.0), "fold line on the right edge");
    } // mirror_brings_the_fold_line_left_when_the_mode_forbids_the_turn

    // @brief A fold line across the grain, or inside the outline, stays off the edge with a warning.
    #[test]
    fn fold_line_off_the_edge_is_reported() {
        let mut across = svg_dom::Document::parse(&svg("M10,0 L90,0")).expect("valid SVG");
        let plan = plan_fold_line_edge(&mut across, &[0, 180]);
        assert!(plan.angles.is_empty());
        assert_eq!(plan.warnings, vec!["Back: the fold line does not run along the grainline".to_string()]);

        let mut inside = doc(50);
        let plan = plan_fold_line_edge(&mut inside, &[0, 180]);
        assert!(plan.angles.is_empty());
        assert_eq!(plan.warnings, vec!["Back: the outline lies on both sides of the fold line".to_string()]);
    } // fold_line_off_the_edge_is_reported

    // @brief A Nested piece-set is not pinned.
    #[test]
    fn piece_set_is_not_pinned() {
        let nested = svg("M0,190 L0,10")
            .replace(r#"<g id="piece_Back""#, r#"<g id="piece-set_Back" data-type="piece-set"><g id="piece_Back""#)
            .replace(r#"<g id="piece_Yoke""#, r#"</g><g id="piece_Yoke""#);
        let mut d = svg_dom::Document::parse(&nested).expect("valid SVG");
        let plan = plan_fold_line_edge(&mut d, &[0, 180]);
        assert_eq!(plan, FoldEdgePlan::default());
    } // piece_set_is_not_pinned

    // @brief Pinning sets the left-edge angle on the planned pieces only.
    #[test]
    fn pin_marks_the_planned_pieces() {
        let mut d = doc(100);
        let plan = plan_fold_line_edge(&mut d, &[0, 180]);
        let mut pieces = crate::piece_extractor::extract_piece_rects(&d);
        pin_fold_line_pieces(&mut pieces, &plan);
        let angles: Vec<Option<u16>> = pieces.iter().map(|p| p.rect.left_edge_angle).collect();
        assert_eq!(angles, vec![Some(180), None]);
    } // pin_marks_the_planned_pieces
} // mod tests
