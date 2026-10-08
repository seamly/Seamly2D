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
//! A reader rebuilds each curve as a spline through the key points. The
//! reader's spline method is not known. Two Catmull-Rom splines, split at turn
//! points, stand in for it: centripetal and chord-length (`READER_ALPHAS`).
//! They differ most where neighboring chords differ in length. `reduced` keeps
//! enough curve points that both splines stay within `CURVE_TOLERANCE_MM` of
//! `dense`, and no more: a curve point neither spline needs is dropped.
//!
//! Turn points come from Seamly2D's `data-turn-points` tags when present
//! (`build_contour_tagged`); otherwise they are detected from the geometry.

use crate::entities::Point;

/// @brief Curve tolerance in millimetres, written as `Curve Tolerance:` style text.
/// @details Douglas-Peucker tolerance for the first key points, and the maximum
///          distance between `dense` and the spline through the final key points.
///          The final key-point chords can be farther from `dense`.
pub const CURVE_TOLERANCE_MM: f64 = 0.25;

// Vertices closer than this are the same vertex.
const DUPLICATE_TOLERANCE_MM: f64 = 0.005;
// A chord whose dense points all lie within this distance is a straight line.
const STRAIGHT_TOLERANCE_MM: f64 = 0.05;
// A straight run's vertices lie within this distance of its chord. Seamly2D writes
// lines exactly; a curve sampled this flat would need a radius of kilometres.
const STRAIGHT_RUN_TOLERANCE_MM: f64 = 0.01;
// A straight chord or straight run at least this long is a line.
const STRAIGHT_MIN_LENGTH_MM: f64 = 10.0;
// A line end is a turn point when its direction change exceeds this many times the
// change at the next curve vertex. A tangent junction bends about half as much.
const TANGENT_BREAK_RATIO: f64 = 2.0;
// A line end is a turn point only above this direction change. On flat curves the
// vertex angles fall below it, and their ratio is noise.
const MIN_BREAK_DEG: f64 = 1.0;
// A single segment shorter than this is a curve chord, not a line. Seamly2D
// interpolates the start of a tight curve with chords of about 11 mm.
const SINGLE_SEGMENT_LINE_MM: f64 = 20.0;
// A direction change above this angle makes a vertex a turn point.
const TURN_ANGLE_DEG: f64 = 25.0;
// A dense edge shorter than this is never split to pull a spline back.
const MIN_SPLIT_EDGE_MM: f64 = 1.0;
// Minimum samples per spline segment between two key points.
const SPLINE_SAMPLES: usize = 32;
// Catmull-Rom knot exponents of the stand-in reader splines: 0.5 = centripetal,
// 1.0 = chord-length. The first one is the spline `spline_points` returns.
const READER_ALPHAS: [f64; 2] = [0.5, 1.0];

/// @brief One contour in the ASTM two-view representation.
#[derive(Debug, Clone, PartialEq)]
pub struct AstmContour {
    /// Full interpolated polyline, written on the validation layer.
    pub dense: Vec<Point>,
    /// Key points, written on the primary layer.
    pub reduced: Vec<Point>,
    /// Parallel to `reduced`: `true` = turn point (layer 2), `false` = curve point (layer 3).
    /// The ends of an open contour are curve points.
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
    // untagged input uses sharp direction changes. Both add each line end where
    // the tangent breaks, even slightly. The ends of an open polyline anchor it here,
    // but Step 8 writes them as curve points.
    let turn_flags = |dense: &[Point], tagged: &Option<Vec<bool>>| {
        let mut flags = match tagged {
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
        }; // match tagged
        let run_ends = straight_run_ends(dense, closed, &flags);
        for (flag, run_end) in flags.iter_mut().zip(run_ends) {
            *flag |= run_end;
        } // for each vertex
        flags
    }; // turn_flags
    let mut sharp = turn_flags(&dense, &tagged);
    let mut anchors: Vec<usize> = (0..dense.len()).filter(|&i| sharp[i]).collect();
    if anchors.is_empty() {
        anchors.push(0); // smooth closed loop: start anywhere
    } // if no anchors

    // Step 3: a closed loop starts at its first anchor, so `dense` and `reduced`
    // begin with the same vertex.
    if closed && anchors[0] != 0 {
        let shift = anchors[0];
        // The flags rotate with the vertices: a straight-run scan from the new
        // vertex 0 could split a run differently.
        dense.rotate_left(shift);
        sharp.rotate_left(shift);
        if let Some(t) = tagged.as_mut() {
            t.rotate_left(shift);
        } // if tagged
        anchors.iter_mut().for_each(|a| *a -= shift);
    } // if rotate

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

    // Step 6: add curve points until the spline through the key points follows `dense`.
    let (kept, turn) = refine_to_spline(&mut dense, kept, turn, closed);

    // Step 7: drop curve points the spline does not need.
    let (kept, mut turn) = prune_to_spline(&dense, kept, turn, closed);

    // Step 8: an open polyline's ends are curve points. A reader flags a layer 2
    // point at the end of an internal line as not needed; its spline ends there anyway.
    if !closed {
        let last = turn.len() - 1;
        turn[0] = false;
        turn[last] = false;
    } // if open
    let reduced = kept.iter().map(|&i| dense[i]).collect();

    Some(AstmContour { dense, reduced, turn, closed })
} // fn build_contour

/// @brief Sample the centripetal spline that a reader rebuilds from the key points of `contour`.
/// @return Polyline through the spline; a closed contour ends at its first key point.
pub fn spline_points(contour: &AstmContour) -> Vec<Point> {
    spline_points_alpha(contour, READER_ALPHAS[0])
} // fn spline_points

// @brief Sample the Catmull-Rom spline with knot exponent `alpha` through the key points.
fn spline_points_alpha(contour: &AstmContour, alpha: f64) -> Vec<Point> {
    let k = contour.reduced.len();
    if k < 2 {
        return contour.reduced.clone();
    } // if no segment
    let segment_count = if contour.closed { k } else { k - 1 };
    let mut out = vec![contour.reduced[0]];
    for s in 0..segment_count {
        // Each segment starts where the previous one ends; skip the repeat.
        let samples = sample_segment(&contour.reduced, &contour.turn, contour.closed, s, SPLINE_SAMPLES, alpha);
        out.extend_from_slice(&samples[1..]);
    } // for each segment
    out
} // fn spline_points_alpha

/// @brief Largest distance between the stand-in reader splines and `dense`, in both directions.
/// @details Measures each dense vertex to each spline, and each spline sample to the
///          dense polyline. This is the check a reader makes against layers 84–87.
pub fn spline_deviation(contour: &AstmContour) -> f64 {
    let mut dense = contour.dense.clone();
    if contour.closed {
        dense.push(dense[0]); // closing edge
    } // if closed
    READER_ALPHAS
        .iter()
        .map(|&alpha| {
            let spline = spline_points_alpha(contour, alpha);
            let to_spline = contour.dense.iter().map(|&p| distance_to_polyline(p, &spline)).fold(0.0, f64::max);
            let to_dense = spline.iter().map(|&p| distance_to_polyline(p, &dense)).fold(0.0, f64::max);
            to_spline.max(to_dense)
        })
        .fold(0.0, f64::max)
} // fn spline_deviation

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

// @brief Flag the ends of straight lines where the tangent breaks.
// @details The contour splits into straight pieces. A piece grows while every vertex
//          inside stays within STRAIGHT_RUN_TOLERANCE_MM of its chord. A piece at least
//          STRAIGHT_MIN_LENGTH_MM long is a line; a single segment needs SINGLE_SEGMENT_LINE_MM.
//          A line end is a turn point when the direction change there exceeds
//          TANGENT_BREAK_RATIO times the change at the far end of the next piece, and
//          MIN_BREAK_DEG. A curve that leaves
//          a line tangentially bends less at the junction than at its next vertex; a corner
//          bends more. On a coarse curve the two changes are equal, so no curve segment
//          becomes a line. A next piece with two or more vertices inside is a line, and its
//          far end says nothing about a curve, so the junction compares against 0.
//          A closed loop is split from its first known turn point, so no piece starts mid-edge.
// @param turns Turn points already known, parallel to `points`.
fn straight_run_ends(points: &[Point], closed: bool, turns: &[bool]) -> Vec<bool> {
    let n = points.len();
    let mut ends = vec![false; n];
    // Indices below are relative to `first`; a closed loop splits back round to it (index n).
    let first = if closed { turns.iter().position(|&t| t).unwrap_or(0) } else { 0 };
    let at = |i: usize| points[(first + i) % n];
    let last = if closed { n } else { n - 1 };

    // Split the contour into straight pieces, each from the end of the previous one.
    let mut pieces: Vec<(usize, usize)> = Vec::new();
    let mut start = 0;
    while start < last {
        let mut end = start + 1;
        while end < last && is_on_chord(points, first + start, first + end + 1) {
            end += 1;
        } // while the piece grows
        pieces.push((start, end));
        start = end;
    } // while vertices remain

    // The piece `step` places after piece j, or None past the end of an open contour.
    let m = pieces.len() as isize;
    let piece_at = |j: usize, step: isize| -> Option<(usize, usize)> {
        let k = j as isize + step;
        if closed {
            Some(pieces[k.rem_euclid(m) as usize])
        } else {
            (0..m).contains(&k).then(|| pieces[k as usize])
        } // if closed: wrap
    };

    for (j, &(a, b)) in pieces.iter().enumerate() {
        let min_length = if b == a + 1 { SINGLE_SEGMENT_LINE_MM } else { STRAIGHT_MIN_LENGTH_MM };
        if distance(at(a), at(b)) < min_length {
            continue;
        } // if not a line
        // Each end: (junction, the line's other end, step towards the neighbour piece).
        for (junction, other, step) in [(a, b, -1), (b, a, 1)] {
            let Some(next) = piece_at(j, step) else {
                continue;
            }; // if open end
            // The neighbour's far end, and the piece after it.
            let far = if step > 0 { next.1 } else { next.0 };
            let break_deg = direction_change_deg(at(other), at(junction), at(far));
            let next_inner = next.1 - next.0 - 1;
            let next_deg = match piece_at(j, 2 * step) {
                Some(after) if next_inner < 2 => {
                    let beyond = if step > 0 { after.1 } else { after.0 };
                    direction_change_deg(at(junction), at(far), at(beyond))
                } // curve neighbour
                _ => 0.0, // straight neighbour or open end
            }; // match piece after
            if break_deg > MIN_BREAK_DEG && break_deg > TANGENT_BREAK_RATIO * next_deg {
                ends[(first + junction) % n] = true;
            } // if the tangent breaks
        } // for each end
    } // for each piece
    ends
} // fn straight_run_ends

// @brief True when every vertex strictly between `a` and `b` lies within STRAIGHT_RUN_TOLERANCE_MM
//        of the chord points[a]→points[b]. Indices wrap round a closed loop.
fn is_on_chord(points: &[Point], a: usize, b: usize) -> bool {
    let n = points.len();
    let (pa, pb) = (points[a % n], points[b % n]);
    (a + 1..b).all(|i| distance_to_segment(points[i % n], pa, pb) < STRAIGHT_RUN_TOLERANCE_MM)
} // fn is_on_chord

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

// @brief Add dense vertices as curve points until every segment of every reader
//        spline is within CURVE_TOLERANCE_MM of `dense`.
// @details Each pass adds at most one vertex per failing segment, then rebuilds
//          the spline, because a new key point also bends its neighbor segments.
//          Added vertices come from `dense`, so `kept` stays an ordered subset of it.
//          A segment between adjacent dense vertices has no vertex to add: when a
//          spline bulges there, the dense edge is split at the point nearest the
//          bulge. The new vertex lies on the edge, so the dense polyline keeps its shape.
// @param dense May gain vertices on its edges.
// @param kept  Ascending indices into `dense`; a closed loop starts at index 0.
// @param turn  Parallel to `kept`; added vertices are curve points.
fn refine_to_spline(dense: &mut Vec<Point>, mut kept: Vec<usize>, mut turn: Vec<bool>, closed: bool) -> (Vec<usize>, Vec<bool>) {
    loop {
        let n = dense.len();
        let k = kept.len();
        let key: Vec<Point> = kept.iter().map(|&i| dense[i]).collect();
        let segment_count = if closed { k } else { k - 1 };
        let mut added: Vec<usize> = Vec::new();
        let mut split: Option<(usize, Point)> = None;
        for s in 0..segment_count {
            let a = kept[s];
            // The last segment of a loop wraps back to vertex 0 (index n).
            let b = if s + 1 < k { kept[s + 1] } else { n };
            let samples_for = |alpha: f64| sample_segment(&key, &turn, closed, s, SPLINE_SAMPLES.max(4 * (b - a)), alpha);
            if b == a + 1 {
                // Adjacent key points: only an edge split can pull the spline back.
                if split.is_none() {
                    split = READER_ALPHAS.iter().find_map(|&alpha| edge_split_point(dense[a], dense[b % n], &samples_for(alpha))).map(|p| (a, p));
                } // if no split chosen yet
                continue;
            } // if adjacent key points
            // The first reader spline that strays picks the vertex to add.
            if let Some(i) = READER_ALPHAS.iter().find_map(|&alpha| worst_vertex(dense, a, b, &samples_for(alpha))) {
                added.push(i);
            } // if outside tolerance
        } // for each segment
        if added.is_empty() && split.is_none() {
            return (kept, turn);
        } // if every segment within tolerance
        for i in added {
            let pos = kept.partition_point(|&j| j < i);
            kept.insert(pos, i);
            turn.insert(pos, false);
        } // for each added vertex
        // One split per pass: it shifts every later dense index by one.
        if let Some((a, p)) = split {
            dense.insert(a + 1, p);
            kept.iter_mut().filter(|j| **j > a).for_each(|j| *j += 1);
            let pos = kept.partition_point(|&j| j <= a);
            kept.insert(pos, a + 1);
            turn.insert(pos, false);
        } // if split
    } // loop until within tolerance
} // fn refine_to_spline

// @brief The point at which to split the dense edge `pa`–`pb` when the spline
//        samples bulge more than CURVE_TOLERANCE_MM away from it.
// @return The point on the edge nearest the worst bulge, or None when the spline
//         is within tolerance or the edge is too short to split.
fn edge_split_point(pa: Point, pb: Point, samples: &[Point]) -> Option<Point> {
    if distance(pa, pb) < MIN_SPLIT_EDGE_MM {
        return None;
    } // if edge too short
    let (bulge, bulge_dist) = samples
        .iter()
        .map(|&p| (p, distance_to_segment(p, pa, pb)))
        .fold((pa, 0.0), |best, cur| if cur.1 > best.1 { cur } else { best });
    if bulge_dist <= CURVE_TOLERANCE_MM {
        return None;
    } // if within tolerance
    // Project the bulge onto the edge, away from its ends.
    let (dx, dy) = (pb.x - pa.x, pb.y - pa.y);
    let t = (((bulge.x - pa.x) * dx + (bulge.y - pa.y) * dy) / (dx * dx + dy * dy)).clamp(0.25, 0.75);
    Some(Point::new(pa.x + t * dx, pa.y + t * dy))
} // fn edge_split_point

// @brief Remove curve points while every spline segment stays within
//        CURVE_TOLERANCE_MM of `dense`.
// @details Each pass removes the curve point whose removal leaves the smallest
//          deviation, so the points that shape the curve go last. Turn points
//          and the first key point stay: `dense` and `reduced` start together.
// @param kept Ascending indices into `dense`; a closed loop starts at index 0.
// @param turn Parallel to `kept`.
fn prune_to_spline(dense: &[Point], mut kept: Vec<usize>, mut turn: Vec<bool>, closed: bool) -> (Vec<usize>, Vec<bool>) {
    let min_len = if closed { 3 } else { 2 };
    loop {
        let k = kept.len();
        if k <= min_len {
            return (kept, turn);
        } // if no key point to spare
        let mut best: Option<(usize, f64)> = None;
        for r in (1..k).filter(|&r| !turn[r]) {
            // Trial contour without key point r.
            let mut trial_kept = kept.clone();
            let mut trial_turn = turn.clone();
            trial_kept.remove(r);
            trial_turn.remove(r);
            let deviation = segments_around(r, k - 1, closed)
                .map(|s| segment_deviation(dense, &trial_kept, &trial_turn, closed, s))
                .fold(0.0, f64::max);
            if deviation <= CURVE_TOLERANCE_MM && best.map_or(true, |(_, d)| deviation < d) {
                best = Some((r, deviation));
            } // if removable and best so far
        } // for each curve point
        let Some((r, _)) = best else {
            return (kept, turn);
        }; // if every curve point is needed
        kept.remove(r);
        turn.remove(r);
    } // loop until no curve point can go
} // fn prune_to_spline

// @brief Spline segments whose shape depends on a key point removed at position `r`.
// @details A Catmull-Rom segment uses two key points on each side of it, so the
//          merged segment r - 1 and its two neighbours change.
// @param k Key point count after the removal.
fn segments_around(r: usize, k: usize, closed: bool) -> impl Iterator<Item = usize> {
    let segment_count = if closed { k } else { k - 1 };
    let mut segments: Vec<usize> = (0..3)
        .filter_map(|d| {
            let s = r as isize - 2 + d;
            if closed {
                Some(s.rem_euclid(k as isize) as usize)
            } else {
                (0..segment_count as isize).contains(&s).then_some(s as usize)
            } // if closed: wrap
        })
        .collect();
    segments.sort_unstable();
    segments.dedup();
    segments.into_iter()
} // fn segments_around

// @brief Largest distance between segment `s` of any reader spline and the dense
//        vertices it spans, in both directions.
// @param kept Ascending indices into `dense`; a closed loop starts at index 0.
fn segment_deviation(dense: &[Point], kept: &[usize], turn: &[bool], closed: bool, s: usize) -> f64 {
    let (n, k) = (dense.len(), kept.len());
    let a = kept[s];
    // The last segment of a loop wraps back to vertex 0 (index n).
    let b = if s + 1 < k { kept[s + 1] } else { n };
    let key: Vec<Point> = kept.iter().map(|&i| dense[i]).collect();
    let chain: Vec<Point> = (a..=b).map(|i| dense[i % n]).collect();
    READER_ALPHAS
        .iter()
        .map(|&alpha| {
            let samples = sample_segment(&key, turn, closed, s, SPLINE_SAMPLES.max(4 * (b - a)), alpha);
            let to_spline = chain.iter().map(|&p| distance_to_polyline(p, &samples)).fold(0.0, f64::max);
            let to_dense = samples.iter().map(|&p| distance_to_polyline(p, &chain)).fold(0.0, f64::max);
            to_spline.max(to_dense)
        })
        .fold(0.0, f64::max)
} // fn segment_deviation

// @brief The dense vertex to add when a spline segment strays more than
//        CURVE_TOLERANCE_MM from the dense vertices a..=b.
// @param b May equal dense.len(), meaning the wrap-around to vertex 0. Must exceed a + 1.
// @return The vertex farthest from the spline. When only the spline bulges away
//         between dense vertices, the vertex nearest the bulge. None when within tolerance.
fn worst_vertex(dense: &[Point], a: usize, b: usize, samples: &[Point]) -> Option<usize> {
    let n = dense.len();
    // Direction 1: dense vertex to spline.
    let (mut far, mut far_dist) = (a + 1, 0.0);
    for i in a + 1..b {
        let d = distance_to_polyline(dense[i], samples);
        if d > far_dist {
            far = i;
            far_dist = d;
        } // if farther
    } // for interior vertex
    if far_dist > CURVE_TOLERANCE_MM {
        return Some(far);
    } // if a vertex is off the spline

    // Direction 2: spline sample to dense polyline.
    let chain: Vec<Point> = (a..=b).map(|i| dense[i % n]).collect();
    let (mut bulge, mut bulge_dist) = (samples[0], 0.0);
    for &p in samples {
        let d = distance_to_polyline(p, &chain);
        if d > bulge_dist {
            bulge = p;
            bulge_dist = d;
        } // if farther
    } // for each sample
    if bulge_dist <= CURVE_TOLERANCE_MM {
        return None;
    } // if within tolerance
    (a + 1..b).min_by(|&i, &j| distance(dense[i], bulge).total_cmp(&distance(dense[j], bulge)))
} // fn worst_vertex

// @brief Sample the spline segment from key point `s` to the next key point.
// @details A turn point ends a spline, so the tangent there uses a phantom
//          neighbor reflected through the turn point. A curve point uses its
//          real neighbor. The ends of an open contour are always turn points.
// @param alpha Catmull-Rom knot exponent; see READER_ALPHAS.
// @return count + 1 points; the first and last are the two key points.
fn sample_segment(key: &[Point], turn: &[bool], closed: bool, s: usize, count: usize, alpha: f64) -> Vec<Point> {
    let k = key.len();
    let j = (s + 1) % k;
    let (p1, p2) = (key[s], key[j]);
    let reflect = |p: Point, q: Point| Point::new(2.0 * p.x - q.x, 2.0 * p.y - q.y);
    let p0 = if turn[s] || (!closed && s == 0) { reflect(p1, p2) } else { key[(s + k - 1) % k] };
    let p3 = if turn[j] || (!closed && j == k - 1) { reflect(p2, p1) } else { key[(j + 1) % k] };
    catmull_rom([p0, p1, p2, p3], count, alpha)
} // fn sample_segment

// @brief Sample the Catmull-Rom segment from p[1] to p[2].
// @details Barry-Goldman evaluation with knot spacing (chord length)^alpha.
// @return count + 1 points from p[1] to p[2].
fn catmull_rom(p: [Point; 4], count: usize, alpha: f64) -> Vec<Point> {
    // Coincident points would give a zero knot interval.
    let knot = |a: Point, b: Point| distance(a, b).powf(alpha).max(1e-9);
    let t0 = 0.0;
    let t1 = t0 + knot(p[0], p[1]);
    let t2 = t1 + knot(p[1], p[2]);
    let t3 = t2 + knot(p[2], p[3]);
    let lerp = |a: Point, b: Point, ta: f64, tb: f64, t: f64| {
        let w = (t - ta) / (tb - ta);
        Point::new(a.x + w * (b.x - a.x), a.y + w * (b.y - a.y))
    };
    (0..=count)
        .map(|i| {
            let t = t1 + (t2 - t1) * i as f64 / count as f64;
            let a1 = lerp(p[0], p[1], t0, t1, t);
            let a2 = lerp(p[1], p[2], t1, t2, t);
            let a3 = lerp(p[2], p[3], t2, t3, t);
            let b1 = lerp(a1, a2, t0, t2, t);
            let b2 = lerp(a2, a3, t1, t3, t);
            lerp(b1, b2, t1, t2, t)
        })
        .collect()
} // fn catmull_rom

// @brief Shortest distance from `p` to the polyline through `points`.
fn distance_to_polyline(p: Point, points: &[Point]) -> f64 {
    if points.len() == 1 {
        return distance(p, points[0]);
    } // if single point
    points.windows(2).map(|w| distance_to_segment(p, w[0], w[1])).fold(f64::INFINITY, f64::min)
} // fn distance_to_polyline
