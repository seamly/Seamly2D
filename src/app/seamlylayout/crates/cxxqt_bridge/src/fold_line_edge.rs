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
// A fold line across or aslant the grain outranks the layout mode: the piece
// turns in the DOM until its fold line is vertical on the side the packer angle
// turns to the left. In Layout Mode Any, an explicit Piece Properties rotation
// must allow the whole turn.
// The packer then puts the piece against the left bin edge (`Rect::with_left_edge`).
//
// The fold line is the first path of the piece's `fold_line` group
// (`svg_dom::fold_line_d`): the fold line out to the cut line, not the arrow
// line or the label inside the group. A Nested piece-set uses the fold line its
// sizes share: `svg_dom::center_piece_sets` stacks them on their fold line start.
//
// Not pinned: a piece-set whose sizes do not share one fold line, a fold line
// without length, and an outline on both sides of its fold line. Each gets a
// warning line. A second plan of the planned DOM changes nothing.

use std::collections::{HashMap, HashSet};

use geometry::{Path, PathSegment, Point};
use packing::PieceRotationRule;
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
    // Piece indices that cannot satisfy both the fold edge and explicit rotation rule.
    pub conflicts: HashSet<usize>,
} // struct FoldEdgePlan

// @brief Turn or mirror each piece with a fold line so its fold line can go on the left edge.
// @param doc                         Pre-processed DOM; a turned or mirrored piece changes in place.
// @param trial_angles_deg            The layout mode's trial angles.
// @param apply_piece_rotation_rules  True only for Layout Mode Any.
// @return The packer angle per piece, and the pieces that stay off the edge.
pub(crate) fn plan_fold_line_edge(
    doc: &mut svg_dom::Document,
    trial_angles_deg: &[u16],
    apply_piece_rotation_rules: bool,
) -> FoldEdgePlan {
    let mut plan = FoldEdgePlan::default();
    let mut changed = false;
    let groups = doc.root.children.iter_mut().filter_map(|n| n.as_mut_element()).filter(|e| e.name == "g");
    for (index, piece) in groups.enumerate() {
        let name = piece_name(piece);
        let ends = if svg_dom::is_piece_set(piece) {
            piece_set_fold_line_ends(piece) // Nested: the fold line the sizes share
        } else {
            Ok(fold_line_ends(piece)) // one piece
        }; // if piece-set
        let (a, b) = match ends {
            Ok(Some(ends)) => ends, // cut on the fold
            Ok(None) => continue,   // not cut on the fold
            Err(reason) => {
                plan.warnings.push(format!("{name}: {reason}"));
                continue; // sizes apart
            } // Err
        }; // match ends
        let (dx, dy) = ((b.x - a.x).abs(), (b.y - a.y).abs());
        if dx.hypot(dy) < ON_EDGE_TOLERANCE_UU {
            plan.warnings.push(format!("{name}: the fold line has no length"));
            continue;
        } // if the fold line is a point
        let points = crate::piece_extractor::piece_outline_points(piece);
        if points.is_empty() {
            continue; // no outline
        } // if no outline

        // Side of the fold line the piece lies on.
        let Some(toward) = toward_outline(&points, a, b) else {
            plan.warnings.push(format!("{name}: the outline lies on both sides of the fold line"));
            continue;
        }; // toward the outline
        let rule = apply_piece_rotation_rules.then(|| crate::piece_extractor::piece_rotation_rule(piece, &name));
        let allowed_angles = match rule {
            Some(rule) => rule.angles_for_any_mode(trial_angles_deg),
            None => trial_angles_deg.to_vec(),
        }; // allowed_angles

        if dx > dy * VERTICAL_TOLERANCE {
            // Across or aslant the grain: turn the piece so a later plan sees a vertical fold line.
            let turn = edge_turn_deg(toward);
            let explicit_rule = rule.filter(|rule| *rule != PieceRotationRule::Unspecified);
            if explicit_rule.is_some_and(|rule| !rule.angles_for_any_mode(trial_angles_deg).contains(&(turn.round() as u16 % 360))) {
                plan.warnings.push(format!("{name}: Piece Properties rotation conflicts with the fold edge"));
                plan.conflicts.insert(index);
                continue;
            } // if the piece rule forbids the turn
            // The angle a vertical fold line takes without a mirror, so a second plan keeps it.
            let angle = if allowed_angles.contains(&0) || !allowed_angles.contains(&180) { 0 } else { 180 };
            let centre = Point::new((a.x + b.x) / 2.0, (a.y + b.y) / 2.0);
            prepend_transform(piece, &format!("rotate({} {} {})", turn - f32::from(angle), centre.x, centre.y));
            changed = true;
            plan.angles.insert(index, angle);
            continue;
        } // if not vertical

        // 0 keeps a fold line on the left; 180 turns a fold line on the right to the left.
        let fold_x = (a.x + b.x) / 2.0;
        let (wanted, other) = if toward.0 > 0.0 { (0, 180) } else { (180, 0) };
        let angle = if allowed_angles.contains(&wanted) {
            wanted
        } else if allowed_angles.contains(&other) && !apply_piece_rotation_rules {
            mirror_across(piece, fold_x);
            changed = true;
            other
        } else {
            let reason = if apply_piece_rotation_rules {
                "Piece Properties rotation conflicts with the fold edge"
            } else {
                "the layout mode turns the piece off the fold"
            };
            plan.warnings.push(format!("{name}: {reason}"));
            if apply_piece_rotation_rules {
                plan.conflicts.insert(index);
            }
            continue;
        }; // selected angle
        plan.angles.insert(index, angle);
    } // for each piece

    if changed {
        svg_dom::flatten_dom(doc); // bake the turn and mirror transforms
        for piece in doc.root.children.iter_mut().filter_map(|n| n.as_mut_element()) {
            unmirror_text(piece);
        } // for each piece
    } // if any piece was turned or mirrored
    plan
} // fn plan_fold_line_edge

// @brief Unit normal from the fold line a–b toward the outline.
// @return None when outline points lie more than `ON_EDGE_TOLERANCE_UU` on both sides of the line.
fn toward_outline(points: &[Point], a: Point, b: Point) -> Option<(f32, f32)> {
    let length = (b.x - a.x).hypot(b.y - a.y);
    let normal = ((a.y - b.y) / length, (b.x - a.x) / length);
    let (low, high) = points
        .iter()
        .map(|p| (p.x - a.x) * normal.0 + (p.y - a.y) * normal.1)
        .fold((f32::MAX, f32::MIN), |(low, high), d| (low.min(d), high.max(d)));
    if low >= -ON_EDGE_TOLERANCE_UU {
        Some(normal)
    } else if high <= ON_EDGE_TOLERANCE_UU {
        Some((-normal.0, -normal.1))
    } else {
        None
    } // if one side
} // fn toward_outline

// @brief SVG `rotate` angle, 0 to 360, that turns `toward` to +x: the fold line then lies left of the outline.
fn edge_turn_deg(toward: (f32, f32)) -> f32 {
    (-toward.1.atan2(toward.0)).to_degrees().rem_euclid(360.0)
} // fn edge_turn_deg

// @brief Set the left-edge angle on every planned piece.
pub(crate) fn pin_fold_line_pieces(pieces: &mut [PieceRect], plan: &FoldEdgePlan) {
    for piece in pieces.iter_mut() {
        if plan.conflicts.contains(&piece.group_index) {
            piece.rect = piece.rect.with_fold_edge_conflict();
        } else if let Some(&angle) = plan.angles.get(&piece.group_index) {
            piece.rect = piece.rect.with_left_edge(angle);
        } // if planned
    } // for each piece
} // fn pin_fold_line_pieces

// @brief First and last point of the piece's fold line (`svg_dom::fold_line_d`).
fn fold_line_ends(piece: &Element) -> Option<(Point, Point)> {
    let path = Path::parse_path_attribute(svg_dom::fold_line_d(piece)?).ok()?;
    let points: Vec<Point> = path.segments.iter().filter_map(segment_end).collect();
    Some((*points.first()?, *points.last()?))
} // fn fold_line_ends

// @brief Fold line of a Nested piece-set: the largest size's, when every size has one on the same line.
// @details `svg_dom::center_piece_sets` puts the largest size first and stacks the sizes on their fold
//          line start, so the fold lines of the sizes lie on one line.
// @return Ok(None) when a size has no fold line; Err with the reason when the sizes' fold lines lie apart.
fn piece_set_fold_line_ends(set: &Element) -> Result<Option<(Point, Point)>, &'static str> {
    let mut first: Option<(Point, Point)> = None;
    let mut apart = false;
    for size in set.children.iter().filter_map(|n| n.as_element()).filter(|e| e.name == "g") {
        let Some((a, b)) = fold_line_ends(size) else {
            return Ok(None); // a size without a fold line: the set is not on the fold
        }; // fold line
        match first {
            Some((fa, fb)) => {
                // Distance of this fold line's middle from the largest size's fold line.
                let middle = Point::new((a.x + b.x) / 2.0, (a.y + b.y) / 2.0);
                apart |= line_distance(middle, fa, fb) > ON_EDGE_TOLERANCE_UU;
            } // Some: a later size
            None => first = Some((a, b)), // the largest size
        } // match first
    } // for each size
    if apart {
        Err("the sizes do not share one fold line")
    } else {
        Ok(first)
    } // if apart
} // fn piece_set_fold_line_ends

// @brief Distance from `p` to the line through `a` and `b`; to `a` when the two ends meet.
fn line_distance(p: Point, a: Point, b: Point) -> f32 {
    let length = (b.x - a.x).hypot(b.y - a.y);
    if length < f32::EPSILON {
        return (p.x - a.x).hypot(p.y - a.y);
    } // if a point
    ((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x)).abs() / length
} // fn line_distance

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
    prepend_transform(piece, &format!("matrix(-1 0 0 1 {} 0)", 2.0 * fold_x));
    piece.attributes.insert("data-fold-mirrored".to_string(), "true".to_string());
} // fn mirror_across

// @brief Apply `transform` after the piece's own transform.
fn prepend_transform(piece: &mut Element, transform: &str) {
    let combined = match piece.attributes.get("transform") {
        Some(own) => format!("{transform} {own}"),
        None => transform.to_string(),
    }; // combined
    piece.attributes.insert("transform".to_string(), combined);
} // fn prepend_transform

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
            let plan = plan_fold_line_edge(&mut d, &[0, 180], false);
            assert_eq!(plan.angles, HashMap::from([(0, angle)]), "fold at x {fold_x}");
            assert!(plan.warnings.is_empty());
            assert_eq!(d.to_string(), before, "fold at x {fold_x}: nothing mirrored");
        } // for each side
    } // turn_brings_the_fold_line_left

    // @brief Any mode respects a fixed piece rule and leaves an impossible fold rule unplaced.
    #[test]
    fn any_mode_fold_edges_intersect_piece_rotation() {
        let fixed = svg("M0,190 L0,10").replace(
            "data-on-fold=\"true\"",
            "data-on-fold=\"true\" data-rotation=\"0\"",
        );
        let mut left = svg_dom::Document::parse(&fixed).expect("valid SVG");
        let plan = plan_fold_line_edge(&mut left, &[0, 90, 180, 270], true);
        assert_eq!(plan.angles, HashMap::from([(0, 0)]));
        assert!(plan.conflicts.is_empty());

        let mut right = svg_dom::Document::parse(&fixed.replace("M0,190 L0,10", "M100,190 L100,10"))
            .expect("valid SVG");
        let plan = plan_fold_line_edge(&mut right, &[0, 90, 180, 270], true);
        assert!(plan.angles.is_empty());
        assert_eq!(plan.conflicts, HashSet::from([0]));

        let mut pieces = crate::piece_extractor::extract_piece_rects(&right);
        crate::piece_extractor::set_piece_rotation_for_layout_mode(&mut pieces, true);
        pin_fold_line_pieces(&mut pieces, &plan);
        assert!(pieces[0].rect.fold_edge_conflict);
        assert!(!packing::fits_bin(
            pieces[0].rect,
            200,
            200,
            &[0, 90, 180, 270],
        ));
    } // any_mode_fold_edges_intersect_piece_rotation

    // @brief With nap up (0 only), a fold line on the right is mirrored to the left;
    // the text reads forward and covers the mirrored area.
    #[test]
    fn mirror_brings_the_fold_line_left_when_the_mode_forbids_the_turn() {
        let mut d = doc(100);
        let plan = plan_fold_line_edge(&mut d, &[0], false);
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
        assert_eq!(plan_fold_line_edge(&mut down, &[180], false).angles, HashMap::from([(0, 180)]));
        let bbox = bbox_of(&down, "piece_Back");
        assert_eq!((bbox.min.x, bbox.max.x), (-100.0, 0.0), "fold line on the right edge");
    } // mirror_brings_the_fold_line_left_when_the_mode_forbids_the_turn

    // @brief First piece's fold line ends after a plan.
    fn back_fold_line(doc: &svg_dom::Document) -> (Point, Point) {
        let back = doc.root.children.iter().filter_map(|n| n.as_element()).next().expect("Back");
        fold_line_ends(back).expect("fold line")
    } // fn back_fold_line

    // @brief True when `a` and `b` differ by less than 0.01.
    fn near(a: f32, b: f32) -> bool {
        (a - b).abs() < 0.01
    } // fn near

    // @brief A fold line across the grain turns the piece a quarter turn, whatever the layout mode allows.
    //        The fold line ends vertical on the side the packer angle brings to the left; a second plan
    //        changes nothing.
    #[test]
    fn fold_line_across_the_grain_turns_the_piece() {
        // Fold line along the bottom edge: 90 brings it left, about its middle (50, 200).
        for (trial, angle, fold_on_left) in [(vec![0, 180], 0, true), (vec![0], 0, true), (vec![180], 180, false)] {
            let mut d = svg_dom::Document::parse(&svg("M0,200 L100,200")).expect("valid SVG");
            let plan = plan_fold_line_edge(&mut d, &trial, false);
            assert_eq!(plan.angles, HashMap::from([(0, angle)]), "trial {trial:?}");
            assert!(plan.warnings.is_empty(), "trial {trial:?}: {:?}", plan.warnings);

            let (a, b) = back_fold_line(&d);
            assert!(near(a.x, 50.0) && near(b.x, 50.0), "trial {trial:?}: fold line {a:?} {b:?}");
            let bbox = bbox_of(&d, "piece_Back");
            let fold_edge = if fold_on_left { bbox.min.x } else { bbox.max.x };
            assert!(near(fold_edge, 50.0), "trial {trial:?}: outline {bbox:?}");
            assert!(near(bbox.max.x - bbox.min.x, 200.0), "trial {trial:?}: quarter turn");

            let planned = d.to_string();
            assert_eq!(plan_fold_line_edge(&mut d, &trial, false), plan, "trial {trial:?}: second plan");
            assert_eq!(d.to_string(), planned, "trial {trial:?}: second plan changes nothing");
        } // for each trial set
    } // fold_line_across_the_grain_turns_the_piece

    // @brief A slanted fold line turns the piece until the fold line is vertical on its left.
    #[test]
    fn slanted_fold_line_turns_the_piece() {
        let triangle = r#"<svg xmlns="http://www.w3.org/2000/svg">
  <g id="piece_Gusset" data-type="piece" data-name="Gusset" data-on-fold="true">
    <g data-type="cutline"><path d="M0,0 L100,0 L0,100 Z"/></g>
    <g id="fold_line_Gusset" data-type="fold_line"><path d="M100,0 L0,100"/></g>
  </g>
</svg>"#;
        let mut d = svg_dom::Document::parse(triangle).expect("valid SVG");
        let plan = plan_fold_line_edge(&mut d, &[0, 180], false);
        assert_eq!(plan.angles, HashMap::from([(0, 0)]));
        let (a, b) = back_fold_line(&d);
        assert!(near(a.x, b.x), "vertical fold line: {a:?} {b:?}");
        assert!(near(bbox_of(&d, "piece_Gusset").min.x, a.x), "fold line on the left");
    } // slanted_fold_line_turns_the_piece

    // @brief Layout Mode Any: an explicit Piece Properties rotation must allow the quarter turn.
    #[test]
    fn any_mode_turn_needs_the_piece_rotation() {
        for (rotation, turned) in [("0", false), ("180", false), ("90", true), ("1", true)] {
            let source = svg("M0,200 L100,200").replace(
                "data-on-fold=\"true\"",
                &format!("data-on-fold=\"true\" data-rotation=\"{rotation}\""),
            );
            let mut d = svg_dom::Document::parse(&source).expect("valid SVG");
            let plan = plan_fold_line_edge(&mut d, &[0, 90, 180, 270], true);
            if turned {
                assert_eq!(plan.angles, HashMap::from([(0, 0)]), "rotation {rotation}");
                assert!(plan.conflicts.is_empty(), "rotation {rotation}");
            } else {
                assert!(plan.angles.is_empty(), "rotation {rotation}");
                assert_eq!(plan.conflicts, HashSet::from([0]), "rotation {rotation}");
                assert_eq!(bbox_of(&d, "piece_Back").max.x, 100.0, "rotation {rotation}: not turned");
            } // if turned
        } // for each rotation
    } // any_mode_turn_needs_the_piece_rotation

    // @brief A fold line without length, or inside the outline, stays off the edge with a warning.
    #[test]
    fn fold_line_off_the_edge_is_reported() {
        let mut point = svg_dom::Document::parse(&svg("M10,0 L10,0.5")).expect("valid SVG");
        let plan = plan_fold_line_edge(&mut point, &[0, 180], false);
        assert!(plan.angles.is_empty());
        assert_eq!(plan.warnings, vec!["Back: the fold line has no length".to_string()]);

        let mut inside = doc(50);
        let plan = plan_fold_line_edge(&mut inside, &[0, 180], false);
        assert!(plan.angles.is_empty());
        assert_eq!(plan.warnings, vec!["Back: the outline lies on both sides of the fold line".to_string()]);
    } // fold_line_off_the_edge_is_reported

    // @brief Back as a Nested piece-set of two sizes, stacked on their fold line start: each size's fold line
    //        at x `fold_x`, from y 200 up to y `top`.
    fn nested_svg(fold_x: [u32; 2]) -> String {
        let size = |n: usize, top: u32| {
            format!(r#"<g id="piece_Back_s{n}" data-type="piece" data-name="Back" data-on-fold="true">
      <g data-type="cutline"><path d="M{x},{top} L{r},{top} L{r},200 L{x},200 Z"/></g>
      <g id="fold_line_Back_s{n}" data-type="fold_line"><path d="M{x},200 L{x},{top}"/></g>
    </g>"#, x = fold_x[n], r = fold_x[n] + 100)
        }; // size
        format!(r#"<svg xmlns="http://www.w3.org/2000/svg">
  <g id="piece-set_Back" data-type="piece-set" data-name="Back">
    {}
    {}
  </g>
</svg>"#, size(0, 0), size(1, 50))
    } // fn nested_svg

    // @brief A Nested piece-set whose sizes share one fold line is planned as one piece; sizes apart are
    //        reported and stay off the edge.
    #[test]
    fn piece_set_on_a_shared_fold_line_is_pinned() {
        let mut shared = svg_dom::Document::parse(&nested_svg([0, 0])).expect("valid SVG");
        let plan = plan_fold_line_edge(&mut shared, &[0, 180], false);
        assert_eq!(plan.angles, HashMap::from([(0, 0)]));
        assert!(plan.warnings.is_empty());

        let mut apart = svg_dom::Document::parse(&nested_svg([0, 30])).expect("valid SVG");
        let plan = plan_fold_line_edge(&mut apart, &[0, 180], false);
        assert!(plan.angles.is_empty());
        assert_eq!(plan.warnings, vec!["Back: the sizes do not share one fold line".to_string()]);
    } // piece_set_on_a_shared_fold_line_is_pinned

    // @brief Sizes stacked on the start of a fold line across the grain share that fold line; the set turns once.
    #[test]
    fn piece_set_on_a_shared_fold_line_across_the_grain_is_pinned() {
        let size = |n: usize, right: u32| {
            format!(r#"<g id="piece_Back_s{n}" data-type="piece" data-name="Back" data-on-fold="true">
      <g data-type="cutline"><path d="M0,0 L{right},0 L{right},200 L0,200 Z"/></g>
      <g id="fold_line_Back_s{n}" data-type="fold_line"><path d="M0,200 L{right},200"/></g>
    </g>"#)
        }; // size
        let source = format!(r#"<svg xmlns="http://www.w3.org/2000/svg">
  <g id="piece-set_Back" data-type="piece-set" data-name="Back">
    {}
    {}
  </g>
</svg>"#, size(0, 100), size(1, 60));
        let mut d = svg_dom::Document::parse(&source).expect("valid SVG");
        let plan = plan_fold_line_edge(&mut d, &[0, 180], false);
        assert_eq!(plan.angles, HashMap::from([(0, 0)]));
        assert!(plan.warnings.is_empty(), "{:?}", plan.warnings);

        let planned = d.to_string();
        assert_eq!(plan_fold_line_edge(&mut d, &[0, 180], false), plan);
        assert_eq!(d.to_string(), planned, "the turned set has a vertical fold line");
    } // piece_set_on_a_shared_fold_line_across_the_grain_is_pinned

    // @brief The fold line group of a Seamly2D handoff holds the fold line, then the tagged arrow line and
    //        label groups. The fold line, not the arrow line, decides the side.
    #[test]
    fn fold_line_group_with_arrow_and_label_reads_the_fold_line() {
        let handoff = svg("M0,210 L0,-10").replace(
            r#"<path d="M0,210 L0,-10"/></g>"#,
            r#"<g><path d="M0,210 L0,-10"/></g>
      <g id="fold_line_arrow_Back" data-type="fold_line_arrow"><path d="M2.5,140 L10,140 L10,60 L2.5,60"/></g>
      <g id="fold_line_label_Back" data-type="fold_line_label"><text x="0" y="5" transform="matrix(0 1 -1 0 5 80)">Foldline</text></g></g>"#,
        );
        let mut d = svg_dom::Document::parse(&handoff).expect("valid SVG");
        assert_eq!(fold_line_ends(d.root.children.iter().filter_map(|n| n.as_element()).next().expect("Back")),
                   Some((Point::new(0.0, 210.0), Point::new(0.0, -10.0))));
        let plan = plan_fold_line_edge(&mut d, &[0, 180], false);
        assert_eq!(plan.angles, HashMap::from([(0, 0)]));
        assert!(plan.warnings.is_empty());
    } // fold_line_group_with_arrow_and_label_reads_the_fold_line

    // @brief Pinning sets the left-edge angle on the planned pieces only.
    #[test]
    fn pin_marks_the_planned_pieces() {
        let mut d = doc(100);
        let plan = plan_fold_line_edge(&mut d, &[0, 180], false);
        let mut pieces = crate::piece_extractor::extract_piece_rects(&d);
        pin_fold_line_pieces(&mut pieces, &plan);
        let angles: Vec<Option<u16>> = pieces.iter().map(|p| p.rect.left_edge_angle).collect();
        assert_eq!(angles, vec![Some(180), None]);
    } // pin_marks_the_planned_pieces
} // mod tests
