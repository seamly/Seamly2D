// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file fold_line_join.rs
// @brief Join an outer line and the fold line into one closed outer edge.
//
// With a fold line, Seamly2D sends the seam line and the cut line without their
// edge on the fold line. The fold line begins where such a line ends and ends
// where it begins, so the outer line and the fold line make the outer edge. The
// outer line is the cut line, or the seam line of a piece without a seam
// allowance. Packing, the DXF-ASTM boundary and the piece divider need that edge
// closed; the display and the cutter use the line as drawn. An older handoff
// sends the line closed, with an edge along the fold line; it needs no join.

// @brief Shortest distance from `p` to the segment `a`-`b`.
pub fn distance_to_segment(p: (f64, f64), a: (f64, f64), b: (f64, f64)) -> f64 {
    let (dx, dy) = (b.0 - a.0, b.1 - a.1);
    let length_squared = dx * dx + dy * dy;
    // Project `p` on the segment, clamped to its ends; a zero-length segment is its start point.
    let t = if length_squared == 0.0 {
        0.0 // degenerate segment
    } else {
        (((p.0 - a.0) * dx + (p.1 - a.1) * dy) / length_squared).clamp(0.0, 1.0) // along the segment
    }; // if degenerate segment
    let (x, y) = (a.0 + t * dx, a.1 + t * dy);
    ((p.0 - x).powi(2) + (p.1 - y).powi(2)).sqrt()
} // fn distance_to_segment

// @brief True when an open line closes along the fold line: both of its ends lie on the fold line.
// @details The closing edge from the last point to the first point then runs along the fold line.
// @param points    Vertices of the open line; fewer than 3 never close.
// @param fold      Fold line start and end.
// @param tolerance Largest distance of an end from the fold line, in the units of the points.
pub fn line_closes_on_fold(points: &[(f64, f64)], fold: ((f64, f64), (f64, f64)), tolerance: f64) -> bool {
    let on_fold = |p: (f64, f64)| distance_to_segment(p, fold.0, fold.1) <= tolerance;
    match (points.first(), points.last()) {
        (Some(&first), Some(&last)) if points.len() >= 3 => on_fold(first) && on_fold(last), // ends on the fold line
        _ => false, // too short to enclose an area
    } // match ends
} // fn line_closes_on_fold

// @brief Fold line join unit tests.
#[cfg(test)]
mod tests {
    use super::*;

    const FOLD: ((f64, f64), (f64, f64)) = ((0.0, 100.0), (0.0, 0.0));

    // @brief A point beside, on, and past the end of a segment.
    #[test]
    fn distance_to_segment_clamps_to_the_ends() {
        assert!((distance_to_segment((3.0, 50.0), FOLD.0, FOLD.1) - 3.0).abs() < 1e-12);
        assert!(distance_to_segment((0.0, 50.0), FOLD.0, FOLD.1) < 1e-12);
        assert!((distance_to_segment((0.0, 104.0), FOLD.0, FOLD.1) - 4.0).abs() < 1e-12);
        assert!((distance_to_segment((3.0, 4.0), (0.0, 0.0), (0.0, 0.0)) - 5.0).abs() < 1e-12);
    }

    // @brief The open cut line begins at the fold line end and ends at its start.
    #[test]
    fn open_cut_line_with_both_ends_on_the_fold_line_closes() {
        let cut = [(0.0, 0.0), (50.0, 0.0), (50.0, 100.0), (0.0, 100.0)];
        assert!(line_closes_on_fold(&cut, FOLD, 0.1));
        // Within the tolerance, and the other direction.
        let near = [(0.05, 100.0), (50.0, 100.0), (50.0, 0.0), (-0.05, 0.0)];
        assert!(line_closes_on_fold(&near, FOLD, 0.1));
    }

    // @brief An end off the fold line, a short line, and no line do not close.
    #[test]
    fn line_with_an_end_off_the_fold_line_does_not_close() {
        assert!(!line_closes_on_fold(&[(0.0, 0.0), (50.0, 0.0), (50.0, 100.0), (1.0, 100.0)], FOLD, 0.1));
        assert!(!line_closes_on_fold(&[(0.0, 0.0), (50.0, 0.0), (50.0, 100.0), (0.0, 120.0)], FOLD, 0.1));
        assert!(!line_closes_on_fold(&[(0.0, 0.0), (0.0, 100.0)], FOLD, 0.1));
        assert!(!line_closes_on_fold(&[], FOLD, 0.1));
    }
}
