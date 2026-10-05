// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief ASTM D6673 notch model: rebuild notches from drawn line segments.
//!
//! Seamly2D draws each notch as one or more straight segments and does not tag
//! the notch type. This module groups touching segments into one notch and
//! writes every notch as a slit (layer 4), whatever shape Seamly2D drew.
//! Slit depth is half the seam allowance width at the notch (user decision):
//! the distance from the notch base on the cut line to the sew line. Each
//! notch keeps the width Seamly2D gave its edge. Without a seam allowance,
//! the depth is the drawn depth.

use crate::astm_contour::{distance, distance_to_segment, AstmContour};
use crate::entities::Point;

// Segments closer than this touch and belong to the same notch.
const TOUCH_TOLERANCE_MM: f64 = 0.05;
// A notch vertex closer than this to the boundary lies on the boundary.
const BOUNDARY_TOLERANCE_MM: f64 = 0.5;
// Segments shorter than this are ignored.
const MIN_SEGMENT_MM: f64 = 0.01;
// A narrower seam allowance counts as none.
const MIN_SEAM_ALLOWANCE_MM: f64 = 0.1;

/// @brief Notch shape, which selects the DXF layer. `build_notches` writes only `Slit`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NotchKind {
    Slit,
    V,
    T,
    Castle,
    U,
}

impl NotchKind {
    /// @brief D6673 layer for this notch shape.
    pub fn layer(self) -> &'static str {
        match self {
            NotchKind::Slit | NotchKind::V => "4",
            NotchKind::T => "80",
            NotchKind::Castle => "81",
            NotchKind::U => "83",
        } // match kind
    } // fn layer
} // impl NotchKind

/// @brief One notch in D6673 §4.3.4 form.
#[derive(Debug, Clone, PartialEq)]
pub struct Notch {
    /// Base point on the boundary (POINT group 10/20).
    pub base: Point,
    /// Direction from the base into the notch, degrees counter-clockwise from +X (group 50).
    pub angle_deg: f64,
    /// Depth along `angle_deg` (group 30).
    pub depth: f64,
    /// Width (group 39); 0 for a slit.
    pub width: f64,
    /// Shape.
    pub kind: NotchKind,
}

/// @brief Rebuild notches from drawn segments, each as a slit.
/// @param segments Notch line segments in DXF units.
/// @param boundary Closed piece boundary the notches sit on; may be empty.
/// @param sew_lines Sew lines inside a cut line boundary. Empty when the boundary is a seam line;
///        every slit then keeps its drawn depth.
/// @return One slit notch per group of touching segments.
pub fn build_notches(segments: &[(Point, Point)], boundary: &[Point], sew_lines: &[AstmContour]) -> Vec<Notch> {
    let segments: Vec<(Point, Point)> = segments
        .iter()
        .copied()
        .filter(|(a, b)| distance(*a, *b) >= MIN_SEGMENT_MM)
        .collect();
    group_touching(&segments)
        .into_iter()
        .map(|group| {
            let group_segments: Vec<(Point, Point)> = group.iter().map(|&i| segments[i]).collect();
            let mut notch = slit_from_drawing(&group_segments, boundary);
            if let Some(width) = seam_allowance_at(notch.base, sew_lines) {
                notch.depth = width / 2.0;
            } // if seam allowance known
            notch
        })
        .collect()
} // fn build_notches

/// @brief Seam allowance width at a point on the cut line: distance to the nearest sew line.
/// @param base Point on the cut line.
/// @param sew_lines Sew lines of the piece.
/// @return `None` without a sew line, or when the width is under 0.1 mm (seam allowance built in).
pub fn seam_allowance_at(base: Point, sew_lines: &[AstmContour]) -> Option<f64> {
    let width = sew_lines
        .iter()
        .map(|line| {
            let n = line.dense.len();
            // A closed sew line also has the edge from its last vertex back to its first.
            let edges = if line.closed { n } else { n.saturating_sub(1) };
            (0..edges)
                .map(|i| distance_to_segment(base, line.dense[i], line.dense[(i + 1) % n]))
                .fold(f64::INFINITY, f64::min)
        })
        .fold(f64::INFINITY, f64::min);
    (width.is_finite() && width >= MIN_SEAM_ALLOWANCE_MM).then_some(width)
} // fn seam_allowance_at

// @brief Group segment indices whose segments touch, transitively (union-find).
fn group_touching(segments: &[(Point, Point)]) -> Vec<Vec<usize>> {
    let n = segments.len();
    let mut parent: Vec<usize> = (0..n).collect();
    fn root(parent: &mut [usize], mut i: usize) -> usize {
        while parent[i] != i {
            parent[i] = parent[parent[i]];
            i = parent[i];
        } // while not root
        i
    } // fn root
    for i in 0..n {
        for j in i + 1..n {
            if segment_distance(segments[i], segments[j]) < TOUCH_TOLERANCE_MM {
                let (ri, rj) = (root(&mut parent, i), root(&mut parent, j));
                parent[ri] = rj;
            } // if touching
        } // for j
    } // for i
    // Keep groups in first-segment order so the output is deterministic.
    let mut groups: Vec<Vec<usize>> = Vec::new();
    let mut group_of_root: Vec<Option<usize>> = vec![None; n];
    for i in 0..n {
        let r = root(&mut parent, i);
        match group_of_root[r] {
            Some(g) => groups[g].push(i),
            None => {
                group_of_root[r] = Some(groups.len());
                groups.push(vec![i]);
            } // None
        } // match group
    } // for each segment
    groups
} // fn group_touching

// @brief Shortest distance between two segments (they do not cross in notch geometry).
fn segment_distance(s: (Point, Point), t: (Point, Point)) -> f64 {
    distance_to_segment(s.0, t.0, t.1)
        .min(distance_to_segment(s.1, t.0, t.1))
        .min(distance_to_segment(t.0, s.0, s.1))
        .min(distance_to_segment(t.1, s.0, s.1))
} // fn segment_distance

// @brief Shortest distance from `p` to a closed polyline; infinite when it is empty.
fn distance_to_boundary(p: Point, boundary: &[Point]) -> f64 {
    let n = boundary.len();
    (0..n)
        .map(|i| distance_to_segment(p, boundary[i], boundary[(i + 1) % n]))
        .fold(f64::INFINITY, f64::min)
} // fn distance_to_boundary

// @brief Slit for one drawn notch: base on the boundary, angle into the piece, drawn depth.
fn slit_from_drawing(segments: &[(Point, Point)], boundary: &[Point]) -> Notch {
    // Distinct vertices of the notch drawing.
    let mut vertices: Vec<Point> = Vec::new();
    for &(a, b) in segments {
        for p in [a, b] {
            if vertices.iter().all(|&q| distance(p, q) >= TOUCH_TOLERANCE_MM) {
                vertices.push(p);
            } // if new vertex
        } // for each end
    } // for each segment

    // Vertices on the boundary. Without a boundary contact, the nearest vertex
    // stands in for it, so a notch is never dropped.
    let dists: Vec<f64> = vertices.iter().map(|&p| distance_to_boundary(p, boundary)).collect();
    let mut contacts: Vec<Point> = vertices
        .iter()
        .zip(&dists)
        .filter(|(_, &d)| d < BOUNDARY_TOLERANCE_MM)
        .map(|(&p, _)| p)
        .collect();
    if contacts.is_empty() {
        let nearest = dists
            .iter()
            .enumerate()
            .min_by(|a, b| a.1.total_cmp(b.1))
            .map_or(0, |(i, _)| i);
        contacts.push(vertices[nearest]);
    } // if no contact

    // Base point: centre of the boundary contacts.
    let count = contacts.len() as f64;
    let base = Point::new(
        contacts.iter().map(|p| p.x).sum::<f64>() / count,
        contacts.iter().map(|p| p.y).sum::<f64>() / count,
    );

    // Direction: from the base toward the centre of the vertices off the boundary.
    let inner: Vec<Point> = vertices
        .iter()
        .copied()
        .filter(|&p| contacts.iter().all(|&c| distance(p, c) >= TOUCH_TOLERANCE_MM))
        .collect();
    let target = if inner.is_empty() {
        // Every vertex touches the boundary: point at the farthest one.
        vertices
            .iter()
            .copied()
            .max_by(|a, b| distance(*a, base).total_cmp(&distance(*b, base)))
            .unwrap_or(base)
    } else {
        let m = inner.len() as f64;
        Point::new(inner.iter().map(|p| p.x).sum::<f64>() / m, inner.iter().map(|p| p.y).sum::<f64>() / m)
    }; // target
    let (dx, dy) = (target.x - base.x, target.y - base.y);
    let len = (dx * dx + dy * dy).sqrt();
    let (ux, uy) = if len > 0.0 { (dx / len, dy / len) } else { (1.0, 0.0) };

    // Depth: farthest reach along the direction.
    let depth = vertices.iter().map(|&p| (p.x - base.x) * ux + (p.y - base.y) * uy).fold(0.0, f64::max);
    let angle_deg = uy.atan2(ux).to_degrees().rem_euclid(360.0);

    Notch { base, angle_deg, depth, width: 0.0, kind: NotchKind::Slit }
} // fn slit_from_drawing
