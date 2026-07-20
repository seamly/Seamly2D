// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! No-Fit Polygon (NFP) computation and (piece, orientation) cache.
//!
//! ## What an NFP is, and why it's cached
//!
//! NFP(A, B) is the locus of B's reference point as B slides around A while
//! staying in continuous edge contact and never overlapping.  Once that
//! polygon is known, a feasible touching-but-non-overlapping placement of B
//! against A reduces to "is B's reference point on or outside NFP(A, B)?" —
//! the hard geometric question becomes a point-in-polygon test.
//!
//! For the greedy strip-packer, every candidate placement of a new piece B
//! against the union of all already-placed pieces requires NFP(A_i, B) for
//! each placed A_i.  Pieces and orientations repeat across runs and across
//! placement attempts, so memoizing each pair pays for itself quickly.
//!
//! ## Cache key and the −NFP identity
//!
//! Key = `(piece_a_id, orient_a, piece_b_id, orient_b)`.  The classical
//! identity `NFP(A, B) = −NFP(B, A)` (point-reflected through the origin)
//! lets a hit on the swapped key serve both directions, halving cache size
//! and computation.  The cache enforces a canonical key order
//! (piece_a_id ≤ piece_b_id, ties broken by orient) on insert so lookups
//! and reflections stay symmetric.
//!
//! ## Threading
//!
//! Stage 1 fills lazily on the placer's hot path — single-threaded, no lock
//! contention.  Stage 3 (multi-start) and Stage 4 (SA) batch-fill upfront;
//! `populate_parallel` is the rayon-driven entry point for that case.

use std::collections::HashMap;

use i_overlay::core::fill_rule::FillRule;
use i_overlay::core::overlay::{Overlay, ShapeType};
use i_overlay::core::overlay_rule::OverlayRule;
use i_overlay::i_float::int::point::IntPoint;

use crate::geom::{IntPolygon, Orientation};

// @brief Cache key for an NFP pair.
//
// Stored canonically: `piece_a` ≤ `piece_b`, with orientation ordering as
// tiebreaker when the same piece appears at two orientations.  Callers go
// through `NfpCache::get`, which canonicalizes and applies the −NFP
// reflection identity transparently.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub(crate) struct NfpKey {
    pub piece_a: usize,
    pub orient_a: Orientation,
    pub piece_b: usize,
    pub orient_b: Orientation,
} // struct NfpKey

impl NfpKey {
    // @brief Construct a key in canonical order.
    //
    // The canonical form lets the cache store one entry per unordered pair;
    // the consumer flips the resulting NFP through the origin when the
    // requested order is the reverse.
    #[allow(dead_code)]
    pub(crate) fn canonical(
        piece_a: usize, orient_a: Orientation,
        piece_b: usize, orient_b: Orientation,
    ) -> (Self, bool /* swapped */) {
        let lhs = (piece_a, orient_a);
        let rhs = (piece_b, orient_b);
        if lhs <= rhs {
            (Self { piece_a, orient_a, piece_b, orient_b }, false)
        } else {
            (Self { piece_a: piece_b, orient_a: orient_b,
                    piece_b: piece_a, orient_b: orient_a }, true)
        } // if lhs <= rhs
    } // fn canonical
} // impl NfpKey

// @brief Lazy NFP cache, keyed by canonical (piece, orient) pair.
//
// `get` canonicalizes the key, computes the entry on miss, and reflects the
// result through the origin if the caller requested the reversed order.
// Hashes are over plain `(usize, u16, usize, u16)` tuples so the default
// `RandomState` hasher is fine — these maps are small (O(pieces²) entries)
// and never on a hot inner loop after warmup.
#[derive(Default)]
#[allow(dead_code)] // Wired into placer.rs once Stage 1 lands.
pub(crate) struct NfpCache {
    entries: HashMap<NfpKey, IntPolygon>,
    // Diagnostic counters — incremented inside `get` so the per-call summary
    // log in `pack_polygons` can report the cache's effective reuse rate.
    // Hits = the requested key was already present; misses = a `compute_pair`
    // (or a swap-derived reflection) had to be performed.
    hits:   u64,
    misses: u64,
    // Wall-clock total spent inside `compute_pair` across the cache's
    // lifetime (excludes the swap-derived reflection cost, which is O(n)
    // and not the bottleneck).  Reported once per `pack_polygons` call.
    compute_ms: u64,
} // struct NfpCache

impl NfpCache {
    // @brief Empty cache.
    #[allow(dead_code)]
    pub(crate) fn new() -> Self {
        Self::default()
    } // fn new

    // @brief Diagnostic accessor: number of `get` calls that found the entry
    // already present.  Used by `pack_polygons`' summary log.
    pub(crate) fn hits(&self) -> u64 {
        self.hits
    } // fn hits

    // @brief Diagnostic accessor: number of `get` calls that had to compute
    // (or swap-derive) the entry.
    pub(crate) fn misses(&self) -> u64 {
        self.misses
    } // fn misses

    // @brief Diagnostic accessor: total wall-clock ms spent in `compute_pair`
    // across the cache's lifetime.
    pub(crate) fn compute_ms(&self) -> u64 {
        self.compute_ms
    } // fn compute_ms

    // @brief Look up NFP(A, B); compute on miss using `pieces[k]` for vertex data.
    //
    // Returns the NFP polygon in B's reference frame.  Storage is keyed on
    // the canonical (piece_a ≤ piece_b) form so each unordered pair runs
    // `compute_pair` exactly once.  When a request matches the swapped form,
    // the canonical entry is reflected through the origin and stashed under
    // the swapped key — keeping the `&IntPolygon` return alive after the
    // borrow on `self.entries` is released.
    //
    // The canonical entry costs one `compute_pair` (polygon-tight Minkowski
    // sum); the derived swapped entry costs one vertex-wise negation.  Both
    // are paid once per unique key over the cache's lifetime.
    //
    // `pieces[k]` is the **un-rotated** polygon for piece id `k`; rotation by
    // `orient_*` is applied here on miss.  Rotation is cheap (linear in
    // vertex count) and the frequency is bounded by trial-set size, so a
    // dedicated rotation cache would be premature optimization.
    pub(crate) fn get(
        &mut self,
        pieces: &[IntPolygon],
        piece_a: usize, orient_a: Orientation,
        piece_b: usize, orient_b: Orientation,
    ) -> &IntPolygon {
        let req_key = NfpKey { piece_a, orient_a, piece_b, orient_b };

        if self.entries.contains_key(&req_key) {
            self.hits += 1;
            log::debug!(
                "[nfp::cache] hit: req=(p{},{}°)-(p{},{}°), verts={}",
                req_key.piece_a,
                req_key.orient_a.0,
                req_key.piece_b,
                req_key.orient_b.0,
                self.entries.get(&req_key).map(|p| p.len()).unwrap_or(0),
            );
        } else {
            self.misses += 1;

            let (canon_key, swapped) = NfpKey::canonical(
                piece_a, orient_a, piece_b, orient_b,
            );
            log::debug!(
                "[nfp::cache] miss: req=(p{},{}°)-(p{},{}°), canon=(p{},{}°)-(p{},{}°), swapped={}",
                req_key.piece_a,
                req_key.orient_a.0,
                req_key.piece_b,
                req_key.orient_b.0,
                canon_key.piece_a,
                canon_key.orient_a.0,
                canon_key.piece_b,
                canon_key.orient_b.0,
                swapped,
            );

            // Ensure the canonical entry exists; compute via Minkowski sum
            // on miss.  When `!swapped` this is exactly `req_key`, so the
            // single insert below covers both lookup paths.
            if !self.entries.contains_key(&canon_key) {
                let oriented_a = crate::geom::rotate(
                    &pieces[canon_key.piece_a], canon_key.orient_a,
                );
                let oriented_b = crate::geom::rotate(
                    &pieces[canon_key.piece_b], canon_key.orient_b,
                );
                log::debug!(
                    "[nfp::cache] compute begin: canon=(p{},{}°)-(p{},{}°), verts={}+{}",
                    canon_key.piece_a,
                    canon_key.orient_a.0,
                    canon_key.piece_b,
                    canon_key.orient_b.0,
                    oriented_a.len(),
                    oriented_b.len(),
                );
                let t_compute = std::time::Instant::now();
                let canon_poly = compute_pair(&oriented_a, &oriented_b);
                let elapsed_ms = t_compute.elapsed().as_millis() as u64;
                self.compute_ms = self.compute_ms.saturating_add(elapsed_ms);
                log::debug!(
                    "[nfp::cache] compute done: canon=(p{},{}°)-(p{},{}°), out_verts={}, took {} ms",
                    canon_key.piece_a,
                    canon_key.orient_a.0,
                    canon_key.piece_b,
                    canon_key.orient_b.0,
                    canon_poly.len(),
                    elapsed_ms,
                );
                // Surface individually slow pair computations — these are
                // the prime suspects when the placer's wall-clock blows up.
                if elapsed_ms >= 50 {
                    log::debug!(
                        "[nfp] slow compute_pair: a=(p{},{}°) b=(p{},{}°) verts={}+{} took {} ms",
                        canon_key.piece_a, canon_key.orient_a.0,
                        canon_key.piece_b, canon_key.orient_b.0,
                        oriented_a.len(), oriented_b.len(),
                        elapsed_ms,
                    );
                } // if slow
                self.entries.insert(canon_key, canon_poly);
            } // if canonical missing

            if swapped {
                // NFP(B, A) = -NFP(A, B): vertex-wise reflection through the
                // origin.  In 2D, reflection through the origin is rotation
                // by 180° (det = +1), so polygon orientation (CCW) is
                // preserved without reversing vertex order.
                let canon_poly = self.entries.get(&canon_key).expect("just inserted");
                let reflected = negate_polygon(canon_poly);
                self.entries.insert(req_key, reflected);
                log::debug!(
                    "[nfp::cache] derived swapped entry: req=(p{},{}°)-(p{},{}°), out_verts={}",
                    req_key.piece_a,
                    req_key.orient_a.0,
                    req_key.piece_b,
                    req_key.orient_b.0,
                    self.entries.get(&req_key).map(|p| p.len()).unwrap_or(0),
                );
            } // if swapped
        } // if request missing

        self.entries.get(&req_key).expect("entry inserted above")
    } // fn get

    // @brief Pre-fill the cache for every (piece, orient) × (piece, orient)
    // pair in `pairs`, using rayon to parallelize compute_pair calls.
    //
    // Used by Stage 3 (deterministic multi-start) and Stage 4 (SA), where
    // the placer evaluates many orderings against the same pair pool and
    // amortizing computation across a thread pool wins clearly.  Stage 1's
    // greedy placer fills lazily through `get` and ignores this entry.
    #[allow(dead_code)]
    pub(crate) fn populate_parallel(
        &mut self,
        _pieces: &[IntPolygon],
        _pairs: &[NfpKey],
    ) {
        // TODO(stage-3): compute each pair via rayon::prelude::*::par_iter,
        // collect into a Vec<(NfpKey, IntPolygon)>, then drain into self.entries.
        unimplemented!("NFP parallel populate — Stage 3 of polygon_pack")
    } // fn populate_parallel
} // impl NfpCache

// @brief Compute NFP(A, B) for two oriented polygons.
//
// ## Algorithm: Minkowski sum via convex decomposition + boolean union
//
// The classical identity for No-Fit Polygons of two simple polygons A and B is:
//
//     NFP(A, B) = A ⊕ (-B)
//
// where ⊕ is the Minkowski sum and -B is B reflected through the origin
// (i.e. each vertex `v` of B becomes `-v`).  For convex polygons this can be
// computed in O(|A| + |B|) by edge merging; for non-convex polygons the
// straightforward generalization is:
//
//   1. **Decompose** each polygon into convex pieces.  Ear-clipping
//      triangulation suffices and keeps the implementation small — every
//      convex piece is a triangle, and triangulation is robust against the
//      garment-piece input invariants we already enforce upstream (closed
//      simple polyline, no holes, no self-intersections).
//   2. **Sum every triangle pair.**  For triangles `t_a ∈ Δ(A)` and
//      `t_b ∈ Δ(-B)`, the convex Minkowski sum `t_a ⊕ t_b` is a polygon of
//      at most six vertices, computed by sorting the two triangles' edge
//      vectors by polar angle and accumulating from the lowest vertex of
//      each.
//   3. **Union the pairs** via `i_overlay`'s boolean overlay with
//      `OverlayRule::Subject` + `FillRule::Positive`.  All triangle sums
//      are CCW (positive winding); the union of overlapping CCW regions is
//      the filled-everywhere region of the Minkowski sum.
//
// ## Output
//
// Returns the outer boundary of NFP(A, B) as a closed polygon (last vertex
// != first; closing edge implicit).  Math-CCW orientation regardless of
// input orientation: the triangulator normalizes to positive shoelace area
// before summing, so the final union is consistent.
//
// Result is in B's reference-point coordinates: any placement where B's
// anchor lies on the NFP boundary places B in continuous edge contact with
// A; any placement strictly outside the NFP boundary leaves a gap; any
// placement strictly inside is forbidden (would cause overlap).
//
// ## Edge cases
//
// * Degenerate input (fewer than 3 vertices in either polygon) returns
//   `Vec::new()`, signaling "no valid NFP".
// * Collinear / extremely-acute triangles that ear-clipping skips produce
//   a smaller convex decomposition; the union absorbs the loss.
// * If `i_overlay`'s union returns multiple disjoint shapes (which
//   shouldn't happen for simple A and B but is theoretically possible
//   under degenerate input), the largest by AABB area wins; the rest are
//   ignored.
#[allow(dead_code)] // Wired in once the placer goes live.
pub(crate) fn compute_pair(poly_a: &IntPolygon, poly_b: &IntPolygon) -> IntPolygon {
    let t_total = std::time::Instant::now();
    log::debug!(
        "[nfp::compute_pair] entry: a_verts={}, b_verts={}",
        poly_a.len(),
        poly_b.len(),
    );
    if poly_a.len() < 3 || poly_b.len() < 3 {
        log::debug!("[nfp::compute_pair] early exit: degenerate input (<3 verts)");
        return Vec::new();
    }

    // NFP(A, B) = A ⊕ (-B): reflect B through the origin.
    let neg_b: IntPolygon = poly_b
        .iter()
        .map(|p| IntPoint { x: -p.x, y: -p.y })
        .collect();

    // Decompose each polygon into CCW triangles via ear clipping.
    let tris_a = triangulate(poly_a);
    let tris_b = triangulate(&neg_b);
    log::debug!(
        "[nfp::compute_pair] triangulate: tris_a={}, tris_b={}",
        tris_a.len(),
        tris_b.len(),
    );
    if tris_a.is_empty() || tris_b.is_empty() {
        log::debug!("[nfp::compute_pair] early exit: triangulation returned empty");
        return Vec::new(); // ear-clipper bailed on degenerate input
    }

    // Per-triangle Minkowski sum.  At most |Δ(A)| · |Δ(-B)| polygons of ≤ 6
    // vertices each.  The pre-sized vector keeps allocator chatter low for
    // hot-path NFP cache fills (Stage 3+ rayon).
    let mut sums: Vec<IntPolygon> = Vec::with_capacity(tris_a.len() * tris_b.len());
    for ta in &tris_a {
        for tb in &tris_b {
            let sum = minkowski_sum_convex(ta, tb);
            if sum.len() >= 3 {
                sums.push(sum);
            }
        }
    }
    log::debug!(
        "[nfp::compute_pair] minkowski pairwise done: sum_polys={}, total_sum_verts={}",
        sums.len(),
        sums.iter().map(|s| s.len()).sum::<usize>(),
    );
    if sums.is_empty() {
        log::debug!("[nfp::compute_pair] early exit: no valid pairwise sums");
        return Vec::new();
    }

    // Boolean union of every triangle-sum via i_overlay.  `OverlayRule::Subject`
    // with all inputs added as Subject extracts the union (areas covered by
    // at least one subject).  `FillRule::Positive` matches our CCW-only
    // convention — every triangle sum has positive winding, so a region with
    // ≥ 1 winding is filled.
    let total_pts: usize = sums.iter().map(|s| s.len()).sum();
    let mut overlay = Overlay::new(total_pts);
    for sum in &sums {
        overlay.add_contour(sum, ShapeType::Subject);
    }
    log::debug!(
        "[nfp::compute_pair] union overlay begin: contours={}, total_pts={}",
        sums.len(),
        total_pts,
    );
    let shapes = overlay.overlay(OverlayRule::Subject, FillRule::Positive);
    log::debug!(
        "[nfp::compute_pair] union overlay done: shapes={}, contours={}",
        shapes.len(),
        shapes.iter().map(|s| s.len()).sum::<usize>(),
    );

    // Pick the outer contour of the largest shape (largest AABB).  For
    // simple A and B the union is connected and `shapes.len() == 1`; the
    // explicit largest-pick is defensive against degenerate input.
    let out = pick_largest_outer(&shapes).unwrap_or_default();
    log::debug!(
        "[nfp::compute_pair] exit: out_verts={}, elapsed={} ms",
        out.len(),
        t_total.elapsed().as_millis(),
    );
    out
} // fn compute_pair

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// @brief Twice the signed area of a closed polygon (shoelace formula).
//
// Returned doubled to stay in `i64` without ever dividing — the sign is what
// matters for orientation, and `i32` coordinates with up to 5×10⁵ vertices
// stay well inside `i64` range (`i32::MAX² · 5×10⁵ ≪ i64::MAX`).
//
// Sign convention: positive ⇒ math-CCW (counter-clockwise in y-up frame),
// negative ⇒ math-CW.  The triangulator normalizes to positive before
// emitting triangles so all downstream code can assume CCW input.
fn signed_area_doubled(poly: &[IntPoint]) -> i64 {
    let n = poly.len();
    let mut s: i64 = 0;
    for i in 0..n {
        let p = poly[i];
        let q = poly[(i + 1) % n];
        s += (p.x as i64) * (q.y as i64) - (q.x as i64) * (p.y as i64);
    }
    s
} // fn signed_area_doubled

// @brief Cross product of `(b - a) × (c - a)`, doubled (no division).
//
// Sign-only test: positive ⇒ a→b→c is a left turn (CCW), negative ⇒ right
// turn (CW), zero ⇒ collinear.  Used by the ear-clipper to detect convex
// vertices and by `point_in_triangle` for barycentric sign tests.
fn cross_at(a: IntPoint, b: IntPoint, c: IntPoint) -> i64 {
    let ux = (b.x - a.x) as i64;
    let uy = (b.y - a.y) as i64;
    let vx = (c.x - a.x) as i64;
    let vy = (c.y - a.y) as i64;
    ux * vy - uy * vx
} // fn cross_at

// @brief Closed point-in-triangle test for a triangle `(a, b, c)`.
//
// Boundary points return `true` — required for the ear-clipping safety
// check: if a non-ear polygon vertex lies on the diagonal `a–c` of a
// proposed ear, the diagonal effectively splits the polygon at that
// vertex.  Strict inclusion would let such a degenerate diagonal through,
// causing later iterations to clip ears whose triangles arc outside the
// polygon (and end up CW, breaking downstream Minkowski-sum orientation).
//
// The non-degenerate case isn't affected: vertices strictly inside still
// satisfy the all-positive (or all-negative) sign pattern; vertices
// strictly outside still produce mixed signs; vertices coincident with a
// triangle vertex produce all-zero signs and are treated as "inside",
// rejecting that degenerate ear.
fn point_in_triangle(p: IntPoint, a: IntPoint, b: IntPoint, c: IntPoint) -> bool {
    let s1 = cross_at(a, b, p);
    let s2 = cross_at(b, c, p);
    let s3 = cross_at(c, a, p);
    (s1 >= 0 && s2 >= 0 && s3 >= 0) || (s1 <= 0 && s2 <= 0 && s3 <= 0)
} // fn point_in_triangle

// @brief Decompose a simple polygon into CCW triangles via ear clipping.
//
// Output triangles are math-CCW (positive shoelace area) regardless of the
// input's orientation, so all downstream geometry can assume one convention.
//
// O(n²) worst case — acceptable for the per-piece vertex counts we see
// (50–200 vertices per garment-piece cutline).  An optimal O(n²) ear clipper
// would maintain an "ear flag" per vertex and a doubly-linked list; this
// straightforward version restarts the ear search after each clip but is
// far smaller and just as correct.
//
// Returns an empty vector on degenerate input (fewer than 3 vertices, or
// the polygon has no convex vertices — which can't happen for a simple
// polygon and signals corruption).
fn triangulate(poly: &[IntPoint]) -> Vec<[IntPoint; 3]> {
    log::debug!("[nfp::triangulate] entry: verts={}", poly.len());
    if poly.len() < 3 {
        log::debug!("[nfp::triangulate] early exit: <3 verts");
        return Vec::new();
    }

    // Normalize to math-CCW (positive shoelace).  Reverse the vertex order
    // when the input is CW so the convex-vertex test below is straightforward.
    let mut indices: Vec<usize> = if signed_area_doubled(poly) >= 0 {
        (0..poly.len()).collect()
    } else {
        (0..poly.len()).rev().collect()
    };

    let mut triangles: Vec<[IntPoint; 3]> = Vec::with_capacity(poly.len().saturating_sub(2));

    // Bound the outer loop generously so a degenerate / corrupted input can't
    // hang the pipeline.  Each iteration either clips an ear (decreasing the
    // vertex count) or fails to find one (early exit), so the actual work is
    // O(n²); the bound is just a safety net.
    let max_iterations = poly.len() * poly.len();
    let mut iterations = 0usize;

    while indices.len() > 3 && iterations < max_iterations {
        iterations += 1;
        let n = indices.len();
        let mut clipped = false;

        for i in 0..n {
            let i_prev = indices[(i + n - 1) % n];
            let i_curr = indices[i];
            let i_next = indices[(i + 1) % n];

            let a = poly[i_prev];
            let b = poly[i_curr];
            let c = poly[i_next];

            // Convex vertex test on a CCW polygon: cross > 0 ⇒ left turn.
            if cross_at(a, b, c) <= 0 {
                continue;
            }

            // Ear test: no other polygon vertex inside (a, b, c).
            let mut clear = true;
            for &k in &indices {
                if k == i_prev || k == i_curr || k == i_next {
                    continue;
                }
                if point_in_triangle(poly[k], a, b, c) {
                    clear = false;
                    break;
                }
            }
            if !clear {
                continue;
            }

            triangles.push([a, b, c]);
            indices.remove(i);
            clipped = true;
            break;
        }

        if !clipped {
            // No ear found — input is degenerate (self-intersecting or
            // collinear).  Bail rather than spin.
            log::warn!(
                "[nfp::triangulate] failed: no ear found (verts_remaining={}, iterations={})",
                indices.len(),
                iterations,
            );
            return Vec::new();
        }
    }

    if indices.len() == 3 {
        triangles.push([poly[indices[0]], poly[indices[1]], poly[indices[2]]]);
    }

    log::debug!(
        "[nfp::triangulate] exit: triangles={}, iterations={}",
        triangles.len(),
        iterations,
    );
    triangles
} // fn triangulate

// @brief Index of the lex-smallest point (lowest y, ties broken by lowest x).
//
// Used as the starting vertex of the Minkowski-sum edge-merge walk: the
// algorithm needs both inputs to begin at a corner whose outgoing edge has
// the smallest polar angle (rotating CCW from the positive x-axis), and the
// bottom-most vertex of a CCW polygon satisfies that.
fn lowest_index(pts: &[IntPoint]) -> usize {
    let mut best = 0;
    for i in 1..pts.len() {
        let p = pts[i];
        let q = pts[best];
        if p.y < q.y || (p.y == q.y && p.x < q.x) {
            best = i;
        }
    }
    best
} // fn lowest_index

// @brief Convex Minkowski sum of two CCW convex polygons.
//
// The classical edge-merge algorithm: starting from the bottom-most vertex
// of each polygon, walk both boundaries in CCW order taking edges in
// monotonically-increasing polar-angle order, summing vertex by vertex.
// Output orientation is math-CCW; output vertex count is at most |a| + |b|
// (fewer when collinear edge pairs collapse into one combined edge).
//
// Polar-angle comparison uses cross product on the edge vectors:
//   `cross(e_a, e_b) > 0`  ⇒  θ(e_a) < θ(e_b)  ⇒  take edge from A first.
//   `cross(e_a, e_b) < 0`  ⇒  θ(e_b) < θ(e_a)  ⇒  take edge from B first.
//   `cross(e_a, e_b) == 0` ⇒  collinear           ⇒  take both (combine).
//
// The function assumes both inputs are convex and CCW; this is guaranteed
// because callers always pass triangles produced by [`triangulate`].
fn minkowski_sum_convex(a: &[IntPoint], b: &[IntPoint]) -> IntPolygon {
    if a.len() < 3 || b.len() < 3 {
        return Vec::new();
    }

    let n = a.len();
    let m = b.len();
    let ai = lowest_index(a);
    let bi = lowest_index(b);

    let av = |k: usize| a[(ai + k) % n];
    let bv = |k: usize| b[(bi + k) % m];

    // Edge vector helpers — i64 so the integer multiplication in the
    // polar-angle compare can't overflow.
    let edge_a = |k: usize| {
        let p0 = av(k);
        let p1 = av(k + 1);
        ((p1.x - p0.x) as i64, (p1.y - p0.y) as i64)
    };
    let edge_b = |k: usize| {
        let p0 = bv(k);
        let p1 = bv(k + 1);
        ((p1.x - p0.x) as i64, (p1.y - p0.y) as i64)
    };

    let start = IntPoint {
        x: av(0).x + bv(0).x,
        y: av(0).y + bv(0).y,
    };
    let mut result: IntPolygon = Vec::with_capacity(n + m);
    result.push(start);
    let mut cur = start;

    let mut i = 0usize;
    let mut j = 0usize;

    while i < n || j < m {
        // Pick which edge to advance based on polar-angle comparison.  Once
        // one polygon is exhausted, drain the other.
        let take_a;
        let take_b;
        if i >= n {
            take_a = false;
            take_b = true;
        } else if j >= m {
            take_a = true;
            take_b = false;
        } else {
            let (eax, eay) = edge_a(i);
            let (ebx, eby) = edge_b(j);
            let cr = eax * eby - eay * ebx;
            if cr > 0 {
                take_a = true;
                take_b = false;
            } else if cr < 0 {
                take_a = false;
                take_b = true;
            } else {
                take_a = true;
                take_b = true;
            }
        }

        let mut dx = 0i64;
        let mut dy = 0i64;
        if take_a {
            let (eax, eay) = edge_a(i);
            dx += eax;
            dy += eay;
            i += 1;
        }
        if take_b {
            let (ebx, eby) = edge_b(j);
            dx += ebx;
            dy += eby;
            j += 1;
        }
        cur = IntPoint {
            x: cur.x + dx as i32,
            y: cur.y + dy as i32,
        };

        // Skip the final closing vertex (the walk ends back at `start`).
        // Comparing against `start` rather than checking i == n && j == m
        // also handles the collinear-edge case where both indices advance
        // simultaneously and may bypass the strict "both exhausted" point.
        if cur == start {
            break;
        }
        result.push(cur);
    }

    result
} // fn minkowski_sum_convex

// @brief Vertex-wise reflection of a polygon through the origin.
//
// Implements the `NFP(B, A) = -NFP(A, B)` identity used by the cache to
// derive swapped-key entries from the canonical-key entry.  Reflection
// through the origin in 2D is `(x, y) → (-x, -y)`, equivalent to a 180°
// rotation; orientation (CCW) is preserved so downstream consumers don't
// need to reverse vertex order.
fn negate_polygon(poly: &IntPolygon) -> IntPolygon {
    poly.iter()
        .map(|p| IntPoint { x: -p.x, y: -p.y })
        .collect()
} // fn negate_polygon

// @brief Pick the outer contour of the largest shape from an i_overlay union.
//
// Selection by AABB area only — for our use case (Minkowski sum of two
// simple connected polygons, always producing a single connected output),
// the union returns one shape and this function just unwraps `shapes[0][0]`.
// The largest-pick is defensive: if degenerate input ever produces multiple
// disjoint pieces, the dominant one wins.
fn pick_largest_outer(shapes: &[Vec<Vec<IntPoint>>]) -> Option<IntPolygon> {
    let mut best: Option<(usize, i64)> = None;
    for (idx, shape) in shapes.iter().enumerate() {
        let outer = match shape.first() {
            Some(c) if !c.is_empty() => c,
            _ => continue,
        };
        // AABB area as a quick stand-in for true area (cheaper, and the
        // ranking it produces matches polygon area for any convex-ish shape).
        let mut min_x = i32::MAX;
        let mut max_x = i32::MIN;
        let mut min_y = i32::MAX;
        let mut max_y = i32::MIN;
        for p in outer {
            if p.x < min_x { min_x = p.x; }
            if p.x > max_x { max_x = p.x; }
            if p.y < min_y { min_y = p.y; }
            if p.y > max_y { max_y = p.y; }
        }
        let dx = (max_x as i64 - min_x as i64).max(0);
        let dy = (max_y as i64 - min_y as i64).max(0);
        let area = dx.saturating_mul(dy);
        if best.map_or(true, |(_, a)| area > a) {
            best = Some((idx, area));
        }
    }
    best.map(|(i, _)| shapes[i][0].clone())
} // fn pick_largest_outer

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    fn ip(x: i32, y: i32) -> IntPoint {
        IntPoint { x, y }
    }

    // @brief AABB extent of a polygon.  Tiny helper used by the assertion
    // `assert_aabb_close` below.
    fn aabb(poly: &[IntPoint]) -> (i32, i32, i32, i32) {
        let mut min_x = i32::MAX;
        let mut max_x = i32::MIN;
        let mut min_y = i32::MAX;
        let mut max_y = i32::MIN;
        for p in poly {
            if p.x < min_x { min_x = p.x; }
            if p.x > max_x { max_x = p.x; }
            if p.y < min_y { min_y = p.y; }
            if p.y > max_y { max_y = p.y; }
        }
        (min_x, min_y, max_x, max_y)
    }

    fn assert_aabb_close(actual: &[IntPoint], expected: (i32, i32, i32, i32), tol: i32) {
        let got = aabb(actual);
        let (e0, e1, e2, e3) = expected;
        assert!((got.0 - e0).abs() <= tol, "min_x: {} vs {}", got.0, e0);
        assert!((got.1 - e1).abs() <= tol, "min_y: {} vs {}", got.1, e1);
        assert!((got.2 - e2).abs() <= tol, "max_x: {} vs {}", got.2, e2);
        assert!((got.3 - e3).abs() <= tol, "max_y: {} vs {}", got.3, e3);
    }

    // @brief Triangulator: a CCW square decomposes into two triangles.
    #[test]
    fn triangulate_square_yields_two_triangles() {
        let square: IntPolygon = vec![
            ip(0, 0), ip(10, 0), ip(10, 10), ip(0, 10),
        ];
        let tris = triangulate(&square);
        assert_eq!(tris.len(), 2);
        // Each triangle must have positive area (math-CCW).
        for tri in &tris {
            let area = cross_at(tri[0], tri[1], tri[2]);
            assert!(area > 0, "triangle should be CCW: {tri:?}");
        }
    } // triangulate_square_yields_two_triangles

    // @brief Triangulator: a CW input is reversed before clipping; output
    // triangles are still CCW (positive cross product).
    #[test]
    fn triangulate_normalizes_cw_input() {
        let cw_square: IntPolygon = vec![
            ip(0, 0), ip(0, 10), ip(10, 10), ip(10, 0),
        ];
        let tris = triangulate(&cw_square);
        assert_eq!(tris.len(), 2);
        for tri in &tris {
            assert!(cross_at(tri[0], tri[1], tri[2]) > 0);
        }
    } // triangulate_normalizes_cw_input

    // @brief Triangulator: an L-shape (concave) produces 4 triangles that
    // together cover the L's interior.  Verifies ear-clipping handles
    // reflex vertices correctly.
    //
    // L-shape (CCW math):
    //
    //   (0,30)  (10,30)
    //     +------+
    //     |      |
    //     |      +------+ (30,20)
    //     |             |
    //     |             |
    //     +-------------+
    //   (0,0)         (30,0)
    #[test]
    fn triangulate_handles_concave_l_shape() {
        let l: IntPolygon = vec![
            ip(0, 0), ip(30, 0), ip(30, 20), ip(10, 20), ip(10, 30), ip(0, 30),
        ];
        let tris = triangulate(&l);
        // L-shape has 6 vertices → 6 - 2 = 4 triangles.
        assert_eq!(tris.len(), 4, "got {} triangles", tris.len());
        for tri in &tris {
            assert!(cross_at(tri[0], tri[1], tri[2]) > 0);
        }
    } // triangulate_handles_concave_l_shape

    // @brief Minkowski sum of two unit squares yields a 2× square (vertex
    // count drops to 4 because every edge pair is collinear and merges).
    #[test]
    fn minkowski_sum_two_unit_squares() {
        let sq: IntPolygon = vec![
            ip(0, 0), ip(10, 0), ip(10, 10), ip(0, 10),
        ];
        let sum = minkowski_sum_convex(&sq, &sq);
        assert_eq!(sum.len(), 4, "expected 4-vertex result, got {sum:?}");
        assert_aabb_close(&sum, (0, 0, 20, 20), 0);
    } // minkowski_sum_two_unit_squares

    // @brief Minkowski sum of two right triangles with axis-aligned legs:
    // four vertices (some edges merge), AABB = sum of the two triangles'
    // AABBs.
    #[test]
    fn minkowski_sum_two_right_triangles() {
        let ta: [IntPoint; 3] = [ip(0, 0), ip(10, 0), ip(0, 10)];
        let tb: [IntPoint; 3] = [ip(0, 0), ip(20, 0), ip(0, 30)];
        let sum = minkowski_sum_convex(&ta, &tb);
        // Resulting polygon: (0,0) → (30,0) → (10,30) → (0,40) → close.
        assert_eq!(sum.len(), 4);
        assert_aabb_close(&sum, (0, 0, 30, 40), 0);
    } // minkowski_sum_two_right_triangles

    // @brief NFP of two identical 10×10 squares is a 20×20 square (the
    // Minkowski sum A ⊕ -B for B = -A is A ⊕ A, which scales 2× when both
    // are unit squares).  Centered around the origin because -B's lex-low
    // vertex is at (-10, -10).
    #[test]
    fn compute_pair_two_squares_is_doubled_square() {
        let sq: IntPolygon = vec![
            ip(0, 0), ip(10, 0), ip(10, 10), ip(0, 10),
        ];
        let nfp = compute_pair(&sq, &sq);
        // NFP boundary should be a 4-vertex square (each side 20 long).
        assert!(!nfp.is_empty());
        assert_aabb_close(&nfp, (-10, -10, 10, 10), 1);
    } // compute_pair_two_squares_is_doubled_square

    // @brief NFP of a 20×20 square against a 5×5 square is a 25×25 square
    // (sum of side lengths on each axis).
    #[test]
    fn compute_pair_different_size_squares() {
        let big: IntPolygon = vec![
            ip(0, 0), ip(20, 0), ip(20, 20), ip(0, 20),
        ];
        let small: IntPolygon = vec![
            ip(0, 0), ip(5, 0), ip(5, 5), ip(0, 5),
        ];
        let nfp = compute_pair(&big, &small);
        assert!(!nfp.is_empty());
        // -small spans (-5,-5) to (0,0); summed with big spans
        // (-5,-5) to (20, 20).
        assert_aabb_close(&nfp, (-5, -5, 20, 20), 1);
    } // compute_pair_different_size_squares

    // @brief NFP of a triangle against itself: the Minkowski sum of A with
    // -A is centrally symmetric.  Verify the result is non-empty and has
    // an AABB twice the original triangle's AABB.
    #[test]
    fn compute_pair_triangle_self() {
        let tri: IntPolygon = vec![
            ip(0, 0), ip(10, 0), ip(0, 10),
        ];
        let nfp = compute_pair(&tri, &tri);
        assert!(!nfp.is_empty());
        // Triangle AABB: (0,0)..(10,10).  -triangle AABB: (-10,-10)..(0,0).
        // Sum AABB: (-10,-10)..(10,10).
        assert_aabb_close(&nfp, (-10, -10, 10, 10), 1);
    } // compute_pair_triangle_self

    // @brief Degenerate inputs (fewer than 3 vertices) return an empty
    // polygon — the placer must check for this rather than panic.
    #[test]
    fn compute_pair_empty_input_returns_empty() {
        let empty: IntPolygon = Vec::new();
        let sq: IntPolygon = vec![ip(0, 0), ip(1, 0), ip(1, 1), ip(0, 1)];
        assert!(compute_pair(&empty, &sq).is_empty());
        assert!(compute_pair(&sq, &empty).is_empty());

        let two_pt: IntPolygon = vec![ip(0, 0), ip(1, 0)];
        assert!(compute_pair(&two_pt, &sq).is_empty());
    } // compute_pair_empty_input_returns_empty

    // @brief NfpCache.get on a fresh cache computes via compute_pair and
    // returns a polygon byte-identical to the direct compute_pair call.
    #[test]
    fn cache_get_miss_matches_compute_pair() {
        let sq: IntPolygon = vec![ip(0, 0), ip(10, 0), ip(10, 10), ip(0, 10)];
        let pieces = vec![sq.clone(), sq.clone()];
        let mut cache = NfpCache::new();

        let cached = cache.get(&pieces, 0, Orientation(0), 1, Orientation(0)).clone();
        let direct = compute_pair(&sq, &sq);
        assert_eq!(cached, direct);
    } // cache_get_miss_matches_compute_pair

    // @brief Repeated lookups of the same key are cache hits — no new entry.
    #[test]
    fn cache_get_hit_does_not_recompute() {
        let sq: IntPolygon = vec![ip(0, 0), ip(10, 0), ip(10, 10), ip(0, 10)];
        let pieces = vec![sq.clone(), sq];
        let mut cache = NfpCache::new();

        cache.get(&pieces, 0, Orientation(0), 1, Orientation(0));
        cache.get(&pieces, 0, Orientation(0), 1, Orientation(0));
        cache.get(&pieces, 0, Orientation(0), 1, Orientation(0));
        assert_eq!(cache.entries.len(), 1, "canonical key cached once");
    } // cache_get_hit_does_not_recompute

    // @brief Swapped-key lookup (b, a) returns -NFP(a, b) — vertex-wise
    // reflection through the origin — and stashes the derived entry under
    // the swapped key without re-running compute_pair.
    #[test]
    fn cache_get_swapped_returns_origin_reflection() {
        let sq: IntPolygon = vec![ip(0, 0), ip(10, 0), ip(10, 10), ip(0, 10)];
        let tri: IntPolygon = vec![ip(0, 0), ip(20, 0), ip(0, 30)];
        let pieces = vec![sq, tri];
        let mut cache = NfpCache::new();

        let forward = cache.get(&pieces, 0, Orientation(0), 1, Orientation(0)).clone();
        let backward = cache.get(&pieces, 1, Orientation(0), 0, Orientation(0)).clone();

        assert!(!forward.is_empty());
        assert_eq!(forward.len(), backward.len());
        for (f, b) in forward.iter().zip(backward.iter()) {
            assert_eq!(f.x, -b.x, "x reflected");
            assert_eq!(f.y, -b.y, "y reflected");
        }
        // Two entries: canonical (0,1) + derived swapped (1,0).
        assert_eq!(cache.entries.len(), 2);
    } // cache_get_swapped_returns_origin_reflection

    // @brief Self-NFP (same piece, same orientation on both sides) is a
    // canonical-key request — `req_key == canon_key`, no swap, one entry.
    #[test]
    fn cache_get_self_nfp_single_entry() {
        let sq: IntPolygon = vec![ip(0, 0), ip(10, 0), ip(10, 10), ip(0, 10)];
        let pieces = vec![sq];
        let mut cache = NfpCache::new();

        let nfp = cache.get(&pieces, 0, Orientation(0), 0, Orientation(0)).clone();
        assert!(!nfp.is_empty());
        assert_eq!(cache.entries.len(), 1);
    } // cache_get_self_nfp_single_entry

    // @brief Different orientations produce different NFPs — the orient_*
    // parameters reach the rotation step, not just the cache key.
    //
    // 10×20 rect against itself: NFP at (0°, 0°) has AABB (-10,-20,10,20).
    // NFP at (0°, 90°): the second rect rotates 10×20 → AABB (0,-10,20,0),
    // i.e. it becomes 20 wide × 10 tall, so the resulting Minkowski-sum AABB
    // shifts and resizes accordingly.  The two NFPs must not be identical.
    #[test]
    fn cache_get_orientation_changes_result() {
        let rect: IntPolygon = vec![ip(0, 0), ip(10, 0), ip(10, 20), ip(0, 20)];
        let pieces = vec![rect.clone(), rect];
        let mut cache = NfpCache::new();

        let unrot = cache.get(&pieces, 0, Orientation(0), 1, Orientation(0)).clone();
        let rot90 = cache.get(&pieces, 0, Orientation(0), 1, Orientation(90)).clone();

        assert!(!unrot.is_empty());
        assert!(!rot90.is_empty());

        // Different orientations → different cache keys → different polygons.
        // AABB inequality is a tight, easy-to-read way to express that.
        assert_ne!(aabb(&unrot), aabb(&rot90));
        assert_eq!(cache.entries.len(), 2);
    } // cache_get_orientation_changes_result

    // @brief negate_polygon: vertex-wise reflection through the origin.
    #[test]
    fn negate_polygon_reflects_vertices() {
        let poly: IntPolygon = vec![ip(1, 2), ip(3, -4), ip(-5, 6)];
        let neg = negate_polygon(&poly);
        assert_eq!(neg, vec![ip(-1, -2), ip(-3, 4), ip(5, -6)]);
    } // negate_polygon_reflects_vertices

    // @brief NFP with a non-convex L-shape against a square.  Verifies the
    // triangulation-based pipeline works end-to-end on a concave input,
    // not just convex pieces.  The result should be a connected non-empty
    // polygon — exact shape is hard to assert tightly without computing
    // the analytic NFP, so we check non-emptiness and a generous AABB.
    #[test]
    fn compute_pair_l_shape_against_square() {
        let l: IntPolygon = vec![
            ip(0, 0), ip(30, 0), ip(30, 20), ip(10, 20), ip(10, 30), ip(0, 30),
        ];
        let sq: IntPolygon = vec![
            ip(0, 0), ip(10, 0), ip(10, 10), ip(0, 10),
        ];
        let nfp = compute_pair(&l, &sq);
        assert!(!nfp.is_empty(), "NFP should not be empty");
        // L spans (0,0)..(30,30); -square spans (-10,-10)..(0,0).
        // Sum AABB envelope: (-10,-10)..(30,30).  Real NFP is a subset of
        // that AABB, but should reach close to those extents.
        let (min_x, min_y, max_x, max_y) = aabb(&nfp);
        assert!(min_x <= 0 && max_x >= 25, "x-range: [{min_x}, {max_x}]");
        assert!(min_y <= 0 && max_y >= 25, "y-range: [{min_y}, {max_y}]");
    } // compute_pair_l_shape_against_square
} // mod tests
