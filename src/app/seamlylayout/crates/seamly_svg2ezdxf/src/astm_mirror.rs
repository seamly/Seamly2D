// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Mirror line (D6673 §4.3.6): layer 6, on the first two boundary points.
//!
//! A piece cut on the fold carries its fold line as the mirror line. The
//! standard asks for a two-point line whose points are boundary points, and
//! "either the first two or the last two points of a piece boundary must match
//! the mirror line's points".
//!
//! The cut line has no seam allowance along the fold, so one cut line edge lies
//! on the fold line. Its end points become the mirror line, and the boundary
//! starts with them. A sew line boundary (no seam allowance) holds the fold
//! line itself.

use crate::astm_contour::{distance, AstmContour};
use crate::drawing::{Block, Drawing};
use crate::entities::Point;

/// Largest distance, in mm, of a boundary key point from the fold line for it to lie on the fold.
pub const MIRROR_LINE_TOLERANCE_MM: f64 = 0.1;

/// @brief Put the mirror line of every block on its boundary, and start the boundary with it.
/// @details Call after grading: grading rebuilds the boundary. A block whose
///          boundary has no edge on its fold line loses its mirror line.
/// @param drawing Drawing whose blocks are changed in place.
/// @return One warning per block whose fold line is not on its boundary.
pub fn align_mirror_lines(drawing: &mut Drawing) -> Vec<String> {
    let mut warnings = Vec::new();
    for block in &mut drawing.blocks {
        if block.mirror_line.is_none() || block.boundary.is_none() {
            continue; // not on the fold, or no boundary to start
        } // if nothing to align
        if !align_block(block) {
            block.mirror_line = None;
            warnings.push(format!(
                "{}: the fold line is not on the cut line; no mirror line written.",
                block.display_name
            ));
        } // if not aligned
    } // for each block
    warnings
} // fn align_mirror_lines

// @brief Align one block; false when no boundary edge lies on its fold line.
fn align_block(block: &mut Block) -> bool {
    let (Some((a, b)), Some(boundary)) = (block.mirror_line, block.boundary.as_mut()) else {
        return false;
    }; // fold line and boundary
    let Some((first, last)) = fold_run(&boundary.reduced, a, b) else {
        return false; // the fold line misses the boundary
    }; // run

    // Key points strictly inside the run lie on the straight fold: drop them, so
    // the run's two ends are neighbours.
    let n = boundary.reduced.len();
    let inside: Vec<usize> = (1..(last + n - first) % n).map(|k| (first + k) % n).collect();
    let dropped: Vec<Point> = inside.iter().map(|&i| boundary.reduced[i]).collect();
    let keep = |i: usize| !inside.contains(&i);
    let order: Vec<usize> = (0..n).map(|k| (first + k) % n).filter(|&i| keep(i)).collect();
    boundary.reduced = order.iter().map(|&i| boundary.reduced[i]).collect();
    boundary.turn = order.iter().map(|&i| boundary.turn[i]).collect();
    start_dense_at(boundary);

    // A dropped key point has no boundary vertex left for its rule text.
    if let Some(grading) = block.grading.as_mut() {
        grading.rule_texts.retain(|t| t.layer != "1" || !dropped.iter().any(|&p| distance(p, t.position) < 1e-6));
    } // if graded

    block.mirror_line = Some((boundary.reduced[0], boundary.reduced[1]));
    true
} // fn align_block

// @brief The run of neighbouring key points on the fold line that covers most of the fold.
// @details The key points form a closed loop. A run has at least two points.
// @return (index of the first point, index of the last point) in loop direction.
fn fold_run(reduced: &[Point], a: Point, b: Point) -> Option<(usize, usize)> {
    let n = reduced.len();
    let length = distance(a, b);
    if n < 3 || length < 1e-9 {
        return None; // no loop, or no fold direction
    } // if degenerate
    let (ux, uy) = ((b.x - a.x) / length, (b.y - a.y) / length);
    let along = |p: Point| (p.x - a.x) * ux + (p.y - a.y) * uy;
    let on_line: Vec<bool> = reduced
        .iter()
        .map(|&p| ((p.x - a.x) * uy - (p.y - a.y) * ux).abs() <= MIRROR_LINE_TOLERANCE_MM)
        .collect();
    if on_line.iter().all(|&on| on) {
        return None; // a flat loop has no side to mirror
    } // if every point on the line

    let mut best: Option<(f64, usize, usize)> = None;
    for start in (0..n).filter(|&i| on_line[i] && !on_line[(i + n - 1) % n]) {
        // Walk the run forward from its first point.
        let mut end = start;
        while on_line[(end + 1) % n] {
            end = (end + 1) % n;
        } // while on the line
        if end == start {
            continue; // one point touches the line: no edge on the fold
        } // if single point
        // Overlap of the run with the fold segment, along the fold.
        let (s, e) = (along(reduced[start]), along(reduced[end]));
        let overlap = s.max(e).min(length) - s.min(e).max(0.0);
        if overlap > 0.0 && best.map_or(true, |(o, _, _)| overlap > o) {
            best = Some((overlap, start, end));
        } // if better
    } // for each run start
    best.map(|(_, start, end)| (start, end))
} // fn fold_run

// @brief Start the validation points at the first key point, keeping their direction.
fn start_dense_at(boundary: &mut AstmContour) {
    let first = boundary.reduced[0];
    let Some(at) = (0..boundary.dense.len())
        .min_by(|&i, &j| distance(boundary.dense[i], first).total_cmp(&distance(boundary.dense[j], first)))
    else {
        return; // no validation points
    }; // at
    boundary.dense.rotate_left(at);
} // fn start_dense_at

#[cfg(test)]
mod tests {
    use super::*;
    use crate::astm_contour::build_contour_tagged;

    // @brief A 100 × 200 mm half piece; the fold runs up its left edge, x = 0.
    fn half_piece(fold: (Point, Point)) -> Drawing {
        let outline = [
            Point::new(100.0, 0.0), Point::new(100.0, 200.0), Point::new(0.0, 200.0), Point::new(0.0, 100.0), Point::new(0.0, 0.0),
        ];
        let mut block = Block::new("piece_Back_M".to_string());
        block.display_name = "Back".to_string();
        // Every vertex a turn point, so (0, 100) stays a key point inside the fold edge.
        block.boundary = build_contour_tagged(&outline, true, Some(&[true; 5]));
        block.mirror_line = Some(fold);
        let mut drawing = Drawing::new(crate::drawing::DxfVersion::R11);
        drawing.add_block(block);
        drawing
    } // fn half_piece

    // @brief The boundary starts with the two ends of its edge on the fold; the mirror line is that edge.
    #[test]
    fn boundary_starts_on_the_mirror_line() {
        let mut drawing = half_piece((Point::new(0.0, 0.0), Point::new(0.0, 200.0)));
        assert!(align_mirror_lines(&mut drawing).is_empty());
        let block = &drawing.blocks[0];
        let boundary = block.boundary.as_ref().expect("boundary");
        let (m1, m2) = block.mirror_line.expect("mirror line");
        assert_eq!((boundary.reduced[0], boundary.reduced[1]), (m1, m2));
        assert_eq!((m1.x, m2.x), (0.0, 0.0), "both on the fold");
        assert_eq!((m1.y - m2.y).abs(), 200.0, "the whole fold edge");
        assert_eq!(boundary.reduced.len(), 4, "the point inside the fold edge is dropped");
        assert_eq!(boundary.dense[0], boundary.reduced[0], "validation points start there too");
    } // boundary_starts_on_the_mirror_line

    // @brief A cut line edge that runs past the fold line's ends still holds the mirror line.
    #[test]
    fn cut_line_edge_longer_than_the_fold_line_is_the_mirror_line() {
        let mut drawing = half_piece((Point::new(0.0, 10.0), Point::new(0.0, 190.0)));
        assert!(align_mirror_lines(&mut drawing).is_empty());
        let (m1, m2) = drawing.blocks[0].mirror_line.expect("mirror line");
        assert_eq!((m1.y.min(m2.y), m1.y.max(m2.y)), (0.0, 200.0), "boundary points, not the fold line ends");
    } // cut_line_edge_longer_than_the_fold_line_is_the_mirror_line

    // @brief A fold line off the boundary writes no mirror line and names the piece.
    #[test]
    fn fold_line_off_the_boundary_is_reported() {
        let mut drawing = half_piece((Point::new(50.0, 0.0), Point::new(50.0, 200.0)));
        let before = drawing.blocks[0].boundary.clone();
        let warnings = align_mirror_lines(&mut drawing);
        assert_eq!(warnings, vec!["Back: the fold line is not on the cut line; no mirror line written.".to_string()]);
        assert!(drawing.blocks[0].mirror_line.is_none());
        assert_eq!(drawing.blocks[0].boundary, before, "boundary unchanged");
    } // fold_line_off_the_boundary_is_reported
} // mod tests
