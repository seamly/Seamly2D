// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief ASTM D6673 contour model: turn/curve point classification and
//!        reduction of an interpolated polyline to its key points.
//!
//! Seamly2D hands over every line as an already-interpolated polyline, so the
//! original curve nodes are lost. This module rebuilds the two ASTM views of
//! one contour:
//! - `reduced` — the key points written on the primary layer (1, 8, 11, 14),
//!   each flagged as a turn point (layer 2) or a curve point (layer 3);
//! - `dense`   — the full interpolated polyline written on the matching
//!   quality-validation layer (84, 85, 86, 87).
//!
//! `reduced` is an ordered subset of `dense` and both start at the same vertex,
//! as D6673-10 §4.3.3.1 requires.
//!
//! Turn points come from Seamly2D's `data-turn-points` tags when present
//! (`build_contour_tagged`); otherwise they are detected from the geometry.

use crate::entities::Point;

/// @brief Curve tolerance in millimetres, written as `Curve Tolerance:` style text.
/// @details Maximum distance of a dense vertex from the reduced polyline chord
///          that replaces it.
pub const CURVE_TOLERANCE_MM: f64 = 0.25;

// Vertices closer than this are the same vertex.
const DUPLICATE_TOLERANCE_MM: f64 = 0.005;
// A chord whose dense points all lie within this distance is a straight line.
const STRAIGHT_TOLERANCE_MM: f64 = 0.05;
// A straight chord at least this long makes both of its ends turn points.
const STRAIGHT_MIN_LENGTH_MM: f64 = 10.0;
// A direction change above this angle makes a vertex a turn point.
const TURN_ANGLE_DEG: f64 = 25.0;

/// @brief One contour in the ASTM two-view representation.
#[derive(Debug, Clone, PartialEq)]
pub struct AstmContour {
    /// Full interpolated polyline, written on the validation layer.
    pub dense: Vec<Point>,
    /// Key points, written on the primary layer.
    pub reduced: Vec<Point>,
    /// Parallel to `reduced`: `true` = turn point (layer 2), `false` = curve point (layer 3).
    pub turn: Vec<bool>,
    /// Whether the contour is a closed loop.
    pub closed: bool,
}

/// @brief Build the ASTM contour for one polyline, detecting turn points from the geometry.
/// @param points Polyline vertices in DXF units; a closed loop may repeat its first vertex.
/// @param closed Whether the polyline is a closed loop.
/// @return The contour, or None when fewer than 2 (open) or 3 (closed) distinct vertices remain.
pub fn build_contour(points: &[Point], closed: bool) -> Option<AstmContour> {
    build_contour_tagged(points, closed, None)
} // fn build_contour

/// @brief Build the ASTM contour for one polyline.
/// @param points Polyline vertices in DXF units; a closed loop may repeat its first vertex.
/// @param closed Whether the polyline is a closed loop.
/// @param turns  Parallel to `points`: `true` = turn point, from Seamly2D's `data-turn-points`.
///               `None` = unknown; turn points are then detected from the geometry.
/// @return The contour, or None when fewer than 2 (open) or 3 (closed) distinct vertices remain.
pub fn build_contour_tagged(points: &[Point], closed: bool, turns: Option<&[bool]>) -> Option<AstmContour> {
    // Tags that do not match the vertices are ignored, not trusted.
    let turns = turns.filter(|t| t.len() == points.len());

    // Step 1: remove repeated vertices, including the closing repeat of a loop.
    // A removed vertex passes its turn tag to the vertex it repeats.
    let (mut dense, mut tagged) = remove_duplicates(points, turns);
    if closed && dense.len() > 1 && distance(dense[0], dense[dense.len() - 1]) < DUPLICATE_TOLERANCE_MM {
        dense.pop();
        if let Some(t) = tagged.as_mut() {
            let last = t.pop().unwrap_or(false);
            t[0] |= last;
        } // if tagged
    } // if closing repeat
    let min_len = if closed { 3 } else { 2 };
    if dense.len() < min_len {
        return None;
    } // if too few vertices

    // Step 2: turn points anchor the reduction. Tagged input uses the tags;
    // untagged input uses sharp direction changes.
    let turn_flags = |dense: &[Point], tagged: &Option<Vec<bool>>| match tagged {
        Some(t) => {
            let mut t = t.clone();
            if !closed {
                t[0] = true;
                let last = t.len() - 1;
                t[last] = true;
            } // if open: ends are turn points
            t
        } // tagged
        None => sharp_vertices(dense, closed),
    }; // turn_flags
    let sharp = turn_flags(&dense, &tagged);
    let mut anchors: Vec<usize> = (0..dense.len()).filter(|&i| sharp[i]).collect();
    if anchors.is_empty() {
        anchors.push(0); // smooth closed loop: start anywhere
    } // if no anchors

    // Step 3: a closed loop starts at its first anchor, so `dense` and `reduced`
    // begin with the same vertex.
    if closed && anchors[0] != 0 {
        let shift = anchors[0];
        dense.rotate_left(shift);
        if let Some(t) = tagged.as_mut() {
            t.rotate_left(shift);
        } // if tagged
        anchors.iter_mut().for_each(|a| *a -= shift);
    } // if rotate
    let sharp = turn_flags(&dense, &tagged);

    // Step 4: reduce each span between consecutive anchors with Douglas-Peucker.
    let n = dense.len();
    let mut kept: Vec<usize> = Vec::new();
    let span_count = if closed { anchors.len() } else { anchors.len() - 1 };
    for s in 0..span_count {
        let start = anchors[s];
        // The last span of a loop wraps back to the first anchor (index n).
        let end = if s + 1 < anchors.len() { anchors[s + 1] } else { n };
        let span: Vec<Point> = (start..=end).map(|i| dense[i % n]).collect();
        kept.push(start);
        for local in douglas_peucker(&span, CURVE_TOLERANCE_MM) {
            kept.push(start + local);
        } // for kept interior vertex
    } // for each span
    if !closed {
        kept.push(anchors[anchors.len() - 1]);
    } // if open: keep last vertex

    // Step 5: classify key points. Tagged input: the tags decide. Untagged input:
    // sharp vertices, open ends, and both ends of a long straight chord are turn
    // points. Everything else is a curve point.
    let k = kept.len();
    let mut turn: Vec<bool> = kept.iter().map(|&i| sharp[i]).collect();
    let chord_count = if tagged.is_some() { 0 } else if closed { k } else { k - 1 };
    for c in 0..chord_count {
        let a = kept[c];
        let b = if c + 1 < k { kept[c + 1] } else { n };
        if is_straight_chord(&dense, a, b) {
            turn[c] = true;
            turn[(c + 1) % k] = true;
        } // if straight chord
    } // for each chord
    let reduced = kept.iter().map(|&i| dense[i]).collect();

    Some(AstmContour { dense, reduced, turn, closed })
} // fn build_contour

/// @brief Distance between two points.
pub fn distance(a: Point, b: Point) -> f64 {
    ((a.x - b.x).powi(2) + (a.y - b.y).powi(2)).sqrt()
} // fn distance

/// @brief Shortest distance from `p` to the segment `a`–`b`.
pub fn distance_to_segment(p: Point, a: Point, b: Point) -> f64 {
    let (dx, dy) = (b.x - a.x, b.y - a.y);
    let len2 = dx * dx + dy * dy;
    if len2 == 0.0 {
        return distance(p, a);
    } // if degenerate segment
    let t = (((p.x - a.x) * dx + (p.y - a.y) * dy) / len2).clamp(0.0, 1.0);
    distance(p, Point::new(a.x + t * dx, a.y + t * dy))
} // fn distance_to_segment

// @brief Drop consecutive vertices closer than DUPLICATE_TOLERANCE_MM.
// @return The kept vertices, and their turn tags when `turns` is given. A dropped
//         vertex's tag is merged into the kept vertex it repeats.
fn remove_duplicates(points: &[Point], turns: Option<&[bool]>) -> (Vec<Point>, Option<Vec<bool>>) {
    let mut out: Vec<Point> = Vec::with_capacity(points.len());
    let mut out_turns: Vec<bool> = Vec::with_capacity(points.len());
    for (i, &p) in points.iter().enumerate() {
        let tag = turns.map_or(false, |t| t[i]);
        if out.last().map_or(true, |&q| distance(p, q) >= DUPLICATE_TOLERANCE_MM) {
            out.push(p);
            out_turns.push(tag);
        } else if let Some(last) = out_turns.last_mut() {
            *last |= tag;
        } // if distinct
    } // for each point
    (out, turns.map(|_| out_turns))
} // fn remove_duplicates

// @brief Flag vertices whose direction change exceeds TURN_ANGLE_DEG.
// @details The ends of an open polyline are always flagged.
fn sharp_vertices(points: &[Point], closed: bool) -> Vec<bool> {
    let n = points.len();
    (0..n)
        .map(|i| {
            if !closed && (i == 0 || i == n - 1) {
                return true; // open end
            } // if open end
            let prev = points[(i + n - 1) % n];
            let next = points[(i + 1) % n];
            direction_change_deg(prev, points[i], next) > TURN_ANGLE_DEG
        })
        .collect()
} // fn sharp_vertices

// @brief Angle in degrees between the incoming and outgoing directions at `p`.
fn direction_change_deg(prev: Point, p: Point, next: Point) -> f64 {
    let (ax, ay) = (p.x - prev.x, p.y - prev.y);
    let (bx, by) = (next.x - p.x, next.y - p.y);
    let (la, lb) = ((ax * ax + ay * ay).sqrt(), (bx * bx + by * by).sqrt());
    if la == 0.0 || lb == 0.0 {
        return 0.0;
    } // if degenerate
    ((ax * bx + ay * by) / (la * lb)).clamp(-1.0, 1.0).acos().to_degrees()
} // fn direction_change_deg

// @brief True when the chord dense[a]→dense[b] is long and all dense points between lie on it.
// @details A chord with no dense point between its ends is not evidence of a
//          straight line: a coarse curve interpolation has long single segments too.
// @param b May equal dense.len(), meaning the wrap-around to vertex 0.
fn is_straight_chord(dense: &[Point], a: usize, b: usize) -> bool {
    let n = dense.len();
    let (pa, pb) = (dense[a], dense[b % n]);
    if b <= a + 1 || distance(pa, pb) < STRAIGHT_MIN_LENGTH_MM {
        return false;
    } // if single segment or short chord
    (a + 1..b).all(|i| distance_to_segment(dense[i % n], pa, pb) < STRAIGHT_TOLERANCE_MM)
} // fn is_straight_chord

// @brief Douglas-Peucker reduction of one span.
// @return Indices (into `span`) of the kept interior vertices, in order; the
//         two span ends are not included.
fn douglas_peucker(span: &[Point], tolerance: f64) -> Vec<usize> {
    let mut kept = Vec::new();
    dp_recurse(span, 0, span.len() - 1, tolerance, &mut kept);
    kept.sort_unstable();
    kept
} // fn douglas_peucker

// @brief Recursive worker for douglas_peucker.
fn dp_recurse(span: &[Point], first: usize, last: usize, tolerance: f64, kept: &mut Vec<usize>) {
    if last <= first + 1 {
        return;
    } // if no interior vertex
    // Find the interior vertex farthest from the chord.
    let (mut worst, mut worst_dist) = (first, 0.0);
    for i in first + 1..last {
        let d = distance_to_segment(span[i], span[first], span[last]);
        if d > worst_dist {
            worst = i;
            worst_dist = d;
        } // if farther
    } // for interior vertex
    if worst_dist > tolerance {
        kept.push(worst);
        dp_recurse(span, first, worst, tolerance, kept);
        dp_recurse(span, worst, last, tolerance, kept);
    } // if outside tolerance
} // fn dp_recurse
