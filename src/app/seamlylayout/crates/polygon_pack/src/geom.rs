// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! Integer-coordinate geometry primitives for polygon-tight packing.
//!
//! All NFP / IFP / placement arithmetic happens in scaled integer space to
//! avoid the robustness pitfalls of floating-point clipping (cracks between
//! coincident edges, near-zero degenerate triangles, epsilon-tuning).
//!
//! ## Scale convention
//!
//! `SCALE = 10_000` (4 decimal places of precision) matches the input
//! precision of SVG cutline coordinates emitted by Seamly2D.  At this scale
//! `i32` headroom is `i32::MAX / SCALE ≈ 214_748` user units — far beyond
//! any realistic garment piece or layout roll length, so `i_overlay`'s
//! native `i32` coordinate type is used directly.
//!
//! Cross-products and other 2-multiplication intermediates fit comfortably
//! in `i64` (`i32::MAX² < 2⁶²`); only path-length-quadratic accumulators
//! warrant `i128`.
//!
//! ## NFP-relevant invariants of input polygons (per LAYOUT_ROTATION_PLAN)
//!
//! * Constructed from the `path_cutline_<piece>` group (or `path_seamline_*`
//!   fallback); notch sibling groups are dropped.
//! * Already a closed straight-line polyline — no curves, no holes,
//!   no mirrored copies, no self-intersections.
//! * No transforms anywhere except text elements (which are not consumed).
//!
//! These invariants let the NFP layer skip the heavy edge-cases of general
//! polygon clipping and keep the sliding orbit simple.

use i_overlay::i_float::int::point::IntPoint;
use i_overlay::mesh::outline::offset::OutlineOffset;
use i_overlay::mesh::style::{LineJoin, OutlineStyle};

use crate::Polygon;

// @brief Fixed integer scale factor: 4 decimal places.
//
// `f_user × SCALE → i32` is lossless for cutline coordinates, which Seamly2D
// emits at 4-decimal precision.  Bump to 10⁷ only if a future input source
// raises precision; doing so requires migrating to an i64-coordinate clipper.
#[allow(dead_code)] // Wired in once the NFP placer goes live.
pub(crate) const SCALE: f64 = 10_000.0;

// @brief Closed integer polygon (last vertex != first; the close is implicit).
//
// Aligned with `i_overlay`'s native `IntPath` so polygons can flow into clip
// operations without a copy.  Construction from SVG cutlines lives in
// `cxxqt_bridge::piece_extractor` (or a future polygon variant of it) and
// produces values of this type.
pub(crate) type IntPolygon = Vec<IntPoint>;

// @brief Oriented bounding box (OBB) for broad-phase collision checks.
//
// The OBB is represented by center point `(cx, cy)`, two orthonormal axes
// `(ux, uy)` and `(vx, vy)`, and half-extents `(ex, ey)` along those axes.
//
// All fields live in scaled-int user space expressed as `f64` for SAT dot
// products; this keeps broad-phase robust and cheap while exact overlap logic
// remains in the polygon/NFP path.
#[derive(Debug, Clone, Copy)]
pub(crate) struct Obb {
    pub cx: f64,
    pub cy: f64,
    pub ux: f64,
    pub uy: f64,
    pub vx: f64,
    pub vy: f64,
    pub ex: f64,
    pub ey: f64,
} // struct Obb

// @brief Trial-set orientation in whole degrees.
//
// Newtype around `u16` so the (piece, orientation) cache key is type-safe
// against accidental mixing with piece ids or pixel measurements.  Values
// match `LayoutSettings::rotation_trial_set_deg` outputs: 0, 90, 180, 270,
// or any user-specified step (10°, 22°, 45° …).
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub(crate) struct Orientation(pub u16);

// @brief Convert a user-space coordinate (mm or px) to scaled integer space.
//
// Round-half-to-even via `f64::round` is sufficient: input precision is
// 4 decimals and SCALE is 10_000, so every legitimate input is exactly
// representable and rounding only catches floating-point dust.
#[allow(dead_code)] // Wired in once the NFP placer goes live.
#[inline]
pub(crate) fn to_int(f: f64) -> i32 {
    (f * SCALE).round() as i32
} // fn to_int

// @brief Convert a scaled integer coordinate back to user space.
#[allow(dead_code)] // Wired in once the NFP placer goes live.
#[inline]
pub(crate) fn from_int(i: i32) -> f64 {
    (i as f64) / SCALE
} // fn from_int

// @brief Convert a public-API `Polygon` (f64 user-space vertices) to the
// internal scaled-integer representation used by NFP / IFP / clipping ops.
//
// Allocation: one `Vec<IntPoint>` per call.  Cheap relative to the per-pair
// NFP cost; called once per piece per orientation during cache population.
#[allow(dead_code)] // Wired in once the NFP placer goes live.
pub(crate) fn polygon_to_int(poly: &Polygon) -> IntPolygon {
    poly.vertices
        .iter()
        .map(|(x, y)| IntPoint { x: to_int(*x), y: to_int(*y) })
        .collect()
} // fn polygon_to_int

// @brief Apply the gap-clearance outward offset to a polygon.
//
// Called once per piece during preprocessing — NFP itself is then computed
// against the offset polygon, so the resulting placements automatically
// honor `gap_px` clearance without any post-pass.
//
// Implementation: delegates to `i_overlay`'s `OutlineOffset::outline_fixed_scale`
// with `scale = 1.0`.  Because our `IntPolygon` is already on the SCALE=10⁴
// integer grid, casting each `IntPoint` to `[f64; 2]` is lossless (`i32` fits
// exactly in `f64`'s 52-bit mantissa), and `scale = 1.0` tells the offset
// algorithm to use the input coordinates directly as integers — no further
// re-scaling.  Result vertices come back as `f64` integer values which we
// round to `i32`; the rounding is defensive against any sub-unit noise from
// internal arithmetic.
//
// Join style: `LineJoin::Bevel` (i_overlay's default).  For garment pieces,
// bevel is conservative on sharp corners — it cuts the corner inward of the
// miter line, leaving a slightly smaller silhouette than a true Minkowski
// disc-sum.  Acceptable for clearance because it never *under*-grows the
// piece; on the contrary, it adds a tiny amount of unused space at acute
// corners, which is the safe direction for cut clearance.
//
// Sign convention:
//   * `gap_int > 0`: grow outward (the common case — gap_px clearance).
//   * `gap_int == 0`: identity short-circuit.
//   * `gap_int < 0`: shrink inward.  Permitted by the API for symmetry but
//     not used by the placer.  May yield an empty polygon if the inset
//     consumes the entire piece; in that case we fall back to the original
//     polygon to honor the "never returns degenerate" contract.
//
// @param poly     Closed input polygon in scaled integer space.
// @param gap_int  Gap distance in scaled integer units (`to_int(gap_px)`).
// @return         Offset polygon, same winding as input.  Falls back to
//                 the input on degenerate offsets.
#[allow(dead_code)] // Wired in once the NFP placer goes live.
pub(crate) fn offset_outward(poly: &IntPolygon, gap_int: i32) -> IntPolygon {
    if gap_int == 0 {
        return poly.clone();
    } // if zero offset

    // Cast i32 → f64 (lossless: i32::MAX < 2⁵² so the mantissa holds it exactly).
    // i_overlay's OutlineOffset is implemented for any `S: ShapeResource<P>`,
    // and `Vec<[f64; 2]>` qualifies — that's the simplest float adapter.
    let path: Vec<[f64; 2]> = poly
        .iter()
        .map(|p| [p.x as f64, p.y as f64])
        .collect();

    // Bevel join (default).  See header comment for why this is the right
    // choice for cut-clearance on garment pieces.
    let style = OutlineStyle::new(gap_int as f64).line_join(LineJoin::Bevel);

    // scale = 1.0: input coords already are the integer grid; the offset
    // algorithm uses them directly without an internal re-scale.
    let shapes = match path.outline_fixed_scale(&style, 1.0) {
        Ok(s) => s,
        // scale=1.0 is always valid; this branch is unreachable under the
        // public contract but keeps the function infallible at the call site.
        Err(_) => return poly.clone(),
    };

    // For our invariants (single closed polygon, no holes, no self-intersections)
    // a positive offset returns one shape with one outer contour.  Negative
    // offsets large enough to consume the polygon return an empty `Shapes`;
    // fall back to the original to maintain the "always non-degenerate" contract.
    let outer = match shapes.into_iter().next().and_then(|s| s.into_iter().next()) {
        Some(c) if !c.is_empty() => c,
        _ => return poly.clone(),
    };

    // Round f64-but-integer-valued coords back to i32.
    outer
        .into_iter()
        .map(|pt| IntPoint {
            x: pt[0].round() as i32,
            y: pt[1].round() as i32,
        })
        .collect()
} // fn offset_outward

// @brief Rotate a polygon about the origin by a whole-degree orientation.
//
// Used during NFP cache population: a piece at orientation θ has a polygon
// rotated by θ before any NFP pair is computed.  Done once per (piece,
// orientation) and the result is then frozen into the integer-coord cache,
// so the f64 round-trip cost is amortised across every cache hit.
//
// Rotation matrix (CCW with x-right, y-down — i.e. SVG/screen convention):
//   x' =  x·cos θ + y·sin θ
//   y' = -x·sin θ + y·cos θ
//
// Note on convention: SVG y grows downward, which inverts the sign of the
// rotation angle relative to the textbook (math) form.  The form above
// gives the visually-CCW rotation that matches the renderer's
// `rotate(deg cx cy)` SVG transform applied to placed pieces.
#[allow(dead_code)] // Wired in once the NFP placer goes live.
pub(crate) fn rotate(poly: &IntPolygon, orient: Orientation) -> IntPolygon {
    // 0° is identity — clone short-circuit avoids the full f64 round-trip and
    // any rounding drift on the cheapest, most common orientation.
    if orient.0 % 360 == 0 {
        return poly.clone();
    } // if zero rotation

    let theta = (orient.0 as f64).to_radians();
    let (sin_t, cos_t) = theta.sin_cos();

    poly.iter()
        .map(|p| {
            let x = p.x as f64;
            let y = p.y as f64;
            // SVG-CCW rotation (see comment above).  Round-half-to-even via
            // f64::round catches representation noise without biasing.
            let xr = (cos_t * x + sin_t * y).round() as i32;
            let yr = (-sin_t * x + cos_t * y).round() as i32;
            IntPoint { x: xr, y: yr }
        })
        .collect()
} // fn rotate

// @brief Axis-aligned bounding box of an integer polygon.
//
// Returns `(min_x, min_y, max_x, max_y)` in scaled integer space.  Used by:
//   * The early-reject path in `pack_polygons` (any AABB > bin → TooLarge),
//   * IFP construction (`placer::inner_fit` shrinks the container by the
//     piece's AABB),
//   * Producing the `Placed.w/h` reported back to the renderer, which uses
//     the AABB to size the piece-fill rect.
//
// Panics on an empty polygon — that's a contract violation (every piece has
// at least three vertices).  The closed-polyline invariant is documented at
// the module level.
#[allow(dead_code)] // Wired in once the NFP placer goes live.
pub(crate) fn aabb(poly: &IntPolygon) -> (i32, i32, i32, i32) {
    let first = poly
        .first()
        .expect("aabb called on empty polygon — input contract violated");
    let mut min_x = first.x;
    let mut min_y = first.y;
    let mut max_x = first.x;
    let mut max_y = first.y;

    for p in poly.iter().skip(1) {
        if p.x < min_x { min_x = p.x; }
        if p.x > max_x { max_x = p.x; }
        if p.y < min_y { min_y = p.y; }
        if p.y > max_y { max_y = p.y; }
    } // for p

    (min_x, min_y, max_x, max_y)
} // fn aabb

// @brief Build an OBB for a piece polygon at a given orientation/anchor.
//
// The OBB is built from the un-rotated polygon's AABB, then rotated by
// `orient` around the origin and translated by `(anchor_x, anchor_y)`.
// This OBB safely encloses the rotated polygon and is suitable for
// broad-phase non-overlap acceptance.
#[allow(dead_code)]
pub(crate) fn obb_for_piece(
    poly: &IntPolygon,
    orient: Orientation,
    anchor_x: i32,
    anchor_y: i32,
) -> Obb {
    let (min_x, min_y, max_x, max_y) = aabb(poly);

    let local_cx = (min_x as f64 + max_x as f64) * 0.5;
    let local_cy = (min_y as f64 + max_y as f64) * 0.5;
    let ex = (max_x as f64 - min_x as f64) * 0.5;
    let ey = (max_y as f64 - min_y as f64) * 0.5;

    let theta = (orient.0 as f64).to_radians();
    let (sin_t, cos_t) = theta.sin_cos();

    // Same SVG-CCW convention as `rotate()`.
    let ux = cos_t;
    let uy = -sin_t;
    let vx = sin_t;
    let vy = cos_t;

    // Rotate center in local frame, then translate by anchor.
    let rcx = cos_t * local_cx + sin_t * local_cy;
    let rcy = -sin_t * local_cx + cos_t * local_cy;

    Obb {
        cx: rcx + anchor_x as f64,
        cy: rcy + anchor_y as f64,
        ux,
        uy,
        vx,
        vy,
        ex,
        ey,
    }
} // fn obb_for_piece

// @brief SAT overlap test for two OBBs.
//
// Returns `true` when the boxes overlap (or touch), `false` when a separating
// axis exists. Axes tested: `a.u`, `a.v`, `b.u`, `b.v`.
#[allow(dead_code)]
pub(crate) fn obb_overlap(a: &Obb, b: &Obb) -> bool {
    let eps = 1e-9;

    // Rotation matrix from B into A's basis: R[i][j] = dot(A_i, B_j).
    let r00 = a.ux * b.ux + a.uy * b.uy;
    let r01 = a.ux * b.vx + a.uy * b.vy;
    let r10 = a.vx * b.ux + a.vy * b.uy;
    let r11 = a.vx * b.vx + a.vy * b.vy;

    let ar00 = r00.abs() + eps;
    let ar01 = r01.abs() + eps;
    let ar10 = r10.abs() + eps;
    let ar11 = r11.abs() + eps;

    // Translation from A to B expressed in A basis.
    let txw = b.cx - a.cx;
    let tyw = b.cy - a.cy;
    let t0 = txw * a.ux + tyw * a.uy;
    let t1 = txw * a.vx + tyw * a.vy;

    // Test axis A.u
    let ra = a.ex;
    let rb = b.ex * ar00 + b.ey * ar01;
    if t0.abs() > ra + rb {
        return false;
    }

    // Test axis A.v
    let ra = a.ey;
    let rb = b.ex * ar10 + b.ey * ar11;
    if t1.abs() > ra + rb {
        return false;
    }

    // Translation expressed in B basis.
    let tb0 = txw * b.ux + tyw * b.uy;
    let tb1 = txw * b.vx + tyw * b.vy;

    // Test axis B.u
    let ra = a.ex * ar00 + a.ey * ar10;
    let rb = b.ex;
    if tb0.abs() > ra + rb {
        return false;
    }

    // Test axis B.v
    let ra = a.ex * ar01 + a.ey * ar11;
    let rb = b.ey;
    if tb1.abs() > ra + rb {
        return false;
    }

    true
} // fn obb_overlap

// ---------------------------------------------------------------------------
// Tests — geometry primitives.  Higher-layer NFP / placer tests live in
// their respective modules.
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // Construct an IntPoint from user-space f64 coords for test readability.
    fn ip(x: f64, y: f64) -> IntPoint {
        IntPoint { x: to_int(x), y: to_int(y) }
    }

    // @brief Round-trip: f64 → int → f64 is lossless at 4-decimal input.
    #[test]
    fn to_int_from_int_round_trips() {
        for v in [0.0_f64, 1.2345, -7.8901, 12345.6789] {
            assert_eq!(from_int(to_int(v)), v, "round-trip failed for {v}");
        }
    } // to_int_from_int_round_trips

    // @brief AABB of an axis-aligned rectangle is just its corners.
    #[test]
    fn aabb_of_axis_aligned_rect() {
        let rect: IntPolygon = vec![
            ip(0.0, 0.0),
            ip(10.0, 0.0),
            ip(10.0, 5.0),
            ip(0.0, 5.0),
        ];
        let (min_x, min_y, max_x, max_y) = aabb(&rect);
        assert_eq!(min_x, to_int(0.0));
        assert_eq!(min_y, to_int(0.0));
        assert_eq!(max_x, to_int(10.0));
        assert_eq!(max_y, to_int(5.0));
    } // aabb_of_axis_aligned_rect

    // @brief AABB of a CCW triangle picks each extremum from the right vertex.
    #[test]
    fn aabb_of_triangle() {
        let tri: IntPolygon = vec![
            ip(2.0, 1.0),
            ip(7.0, 3.0),
            ip(4.0, 8.0),
        ];
        let (min_x, min_y, max_x, max_y) = aabb(&tri);
        assert_eq!(min_x, to_int(2.0));
        assert_eq!(min_y, to_int(1.0));
        assert_eq!(max_x, to_int(7.0));
        assert_eq!(max_y, to_int(8.0));
    } // aabb_of_triangle

    // @brief 0° rotation is identity.
    #[test]
    fn rotate_zero_is_identity() {
        let poly: IntPolygon = vec![ip(1.0, 2.0), ip(3.0, 4.0), ip(5.0, 6.0)];
        let rotated = rotate(&poly, Orientation(0));
        assert_eq!(rotated, poly);
    } // rotate_zero_is_identity

    // @brief 360° rotation is identity (modular wrap).
    #[test]
    fn rotate_360_is_identity() {
        let poly: IntPolygon = vec![ip(1.0, 2.0), ip(3.0, 4.0)];
        let rotated = rotate(&poly, Orientation(360));
        assert_eq!(rotated, poly);
    } // rotate_360_is_identity

    // @brief 180° rotation negates every vertex.
    //
    // Allows ±1 unit of rounding noise — at SCALE=10_000 this is 0.0001 of a
    // user unit, well below the 4-decimal input precision.
    #[test]
    fn rotate_180_negates_vertices() {
        let poly: IntPolygon = vec![ip(3.0, 4.0), ip(-2.5, 1.25)];
        let rotated = rotate(&poly, Orientation(180));
        for (orig, rot) in poly.iter().zip(rotated.iter()) {
            assert!((rot.x - (-orig.x)).abs() <= 1, "x: {} vs -{}", rot.x, orig.x);
            assert!((rot.y - (-orig.y)).abs() <= 1, "y: {} vs -{}", rot.y, orig.y);
        }
    } // rotate_180_negates_vertices

    // @brief 90° SVG-CCW: (x, y) → (y, -x).
    //
    // SVG's y-down convention means visually-CCW rotation corresponds to a
    // mathematically-CW rotation in standard (y-up) coordinates.  Verifying
    // this here pins the convention so a future edit doesn't silently flip it.
    #[test]
    fn rotate_90_svg_ccw_orientation() {
        // (1, 0) in SVG-CCW 90° → (0, -1)
        let poly: IntPolygon = vec![ip(1.0, 0.0)];
        let rotated = rotate(&poly, Orientation(90));
        assert!((rotated[0].x - to_int(0.0)).abs() <= 1);
        assert!((rotated[0].y - to_int(-1.0)).abs() <= 1);
    } // rotate_90_svg_ccw_orientation

    // @brief Rotation by 90° four times returns to the original (within ±1 unit).
    #[test]
    fn rotate_90_four_times_is_identity() {
        let poly: IntPolygon = vec![ip(3.0, 0.0), ip(0.0, 4.0)];
        let mut rotated = poly.clone();
        for _ in 0..4 {
            rotated = rotate(&rotated, Orientation(90));
        }
        for (orig, rot) in poly.iter().zip(rotated.iter()) {
            assert!((rot.x - orig.x).abs() <= 1);
            assert!((rot.y - orig.y).abs() <= 1);
        }
    } // rotate_90_four_times_is_identity

    // @brief Zero-gap offset is the identity (short-circuit path).
    #[test]
    fn offset_zero_is_identity() {
        let square: IntPolygon = vec![
            ip(0.0, 0.0), ip(10.0, 0.0), ip(10.0, 10.0), ip(0.0, 10.0),
        ];
        let offset = offset_outward(&square, 0);
        assert_eq!(offset, square);
    } // offset_zero_is_identity

    // @brief Outward-offset square: the bevel join produces 8 vertices
    // (one extra per corner), and the AABB grows by `gap_int` on each side.
    //
    // Tolerance ±2 units: the float→int round-trip in offset_outward can
    // introduce ±1 of rounding noise per coordinate, and the bevel itself
    // can land 1 unit short of the analytic miter on perfect right angles.
    #[test]
    fn offset_grows_square_aabb_by_gap_on_each_side() {
        let square: IntPolygon = vec![
            ip(0.0, 0.0), ip(10.0, 0.0), ip(10.0, 10.0), ip(0.0, 10.0),
        ];
        // 1 user unit gap = SCALE = 10_000 scaled units.
        let gap_int: i32 = to_int(1.0);
        let offset = offset_outward(&square, gap_int);

        // 4 → 8 vertices on a beveled square (one bevel per corner).
        assert_eq!(offset.len(), 8, "expected 8 vertices, got {}", offset.len());

        // AABB should have grown outward by `gap_int` on each side.
        let (min_x, min_y, max_x, max_y) = aabb(&offset);
        assert!((min_x - (-gap_int)).abs()       <= 2, "min_x: {}, want {}", min_x, -gap_int);
        assert!((min_y - (-gap_int)).abs()       <= 2, "min_y: {}, want {}", min_y, -gap_int);
        assert!((max_x - (to_int(10.0) + gap_int)).abs() <= 2, "max_x: {}", max_x);
        assert!((max_y - (to_int(10.0) + gap_int)).abs() <= 2, "max_y: {}", max_y);
    } // offset_grows_square_aabb_by_gap_on_each_side

    // @brief Outward offset on a triangle grows the AABB on every side and
    // never shrinks it.  Lower-bound only — the exact growth depends on each
    // corner's angle: at acute apex angles the bevel join reaches some
    // fraction of the miter distance (`bevel ≈ gap × sin(half_angle)`), so a
    // 27° apex on this 10×10 triangle ends up ~89% of `gap_int` from the
    // original.  That undershoot is the safe direction for cut clearance —
    // the offset still surrounds the original — so we assert "grew, didn't
    // shrink" rather than a tight numeric bound.
    #[test]
    fn offset_grows_triangle_aabb_outward() {
        let tri: IntPolygon = vec![
            ip(0.0, 0.0),
            ip(10.0, 0.0),
            ip(5.0, 10.0),
        ];
        let gap_int: i32 = to_int(1.0);
        let (orig_min_x, orig_min_y, orig_max_x, orig_max_y) = aabb(&tri);
        let offset = offset_outward(&tri, gap_int);
        let (min_x, min_y, max_x, max_y) = aabb(&offset);

        // Geometric invariant: the offset AABB strictly contains the
        // original on every side.  Bevel join distance at a corner with
        // interior angle `α` is `gap × sin(α/2)`, so the apex (α≈53°) only
        // gets ~0.45 × gap_int and the base corners (α≈63°) get ~0.53 ×
        // gap_int.  A tight numeric bound is therefore polygon-shape
        // dependent; the *contract* is monotonic outward growth, which this
        // strict-containment check captures cleanly.
        assert!(min_x < orig_min_x, "min_x: {} → {}", orig_min_x, min_x);
        assert!(min_y < orig_min_y, "min_y: {} → {}", orig_min_y, min_y);
        assert!(max_x > orig_max_x, "max_x: {} → {}", orig_max_x, max_x);
        assert!(max_y > orig_max_y, "max_y: {} → {}", orig_max_y, max_y);
    } // offset_grows_triangle_aabb_outward

    // @brief Negative offset that consumes the entire polygon falls back to
    // the input rather than returning an empty/degenerate result.  This is
    // the "always non-degenerate" contract advertised in the header comment.
    #[test]
    fn offset_inward_collapse_falls_back_to_input() {
        let square: IntPolygon = vec![
            ip(0.0, 0.0), ip(10.0, 0.0), ip(10.0, 10.0), ip(0.0, 10.0),
        ];
        // Inset by half the diagonal — definitely consumes the whole square.
        let offset = offset_outward(&square, -to_int(20.0));
        assert_eq!(offset, square, "expected fallback to input on inward collapse");
    } // offset_inward_collapse_falls_back_to_input

    // @brief Polygon → IntPolygon conversion preserves vertex count and
    // round-trips coordinates losslessly at 4-decimal precision.
    #[test]
    fn polygon_to_int_preserves_vertices() {
        let poly = Polygon {
            vertices: vec![(0.0, 0.0), (10.5, 0.0), (10.5, 5.25), (0.0, 5.25)],
        };
        let ipoly = polygon_to_int(&poly);
        assert_eq!(ipoly.len(), poly.vertices.len());
        for ((x, y), p) in poly.vertices.iter().zip(ipoly.iter()) {
            assert_eq!(from_int(p.x), *x);
            assert_eq!(from_int(p.y), *y);
        }
    } // polygon_to_int_preserves_vertices

    // @brief OBB overlap: separated axis-aligned boxes do not overlap.
    #[test]
    fn obb_overlap_separated_boxes_false() {
        let sq: IntPolygon = vec![ip(0.0, 0.0), ip(10.0, 0.0), ip(10.0, 10.0), ip(0.0, 10.0)];
        let a = obb_for_piece(&sq, Orientation(0), to_int(0.0), to_int(0.0));
        let b = obb_for_piece(&sq, Orientation(0), to_int(25.0), to_int(0.0));
        assert!(!obb_overlap(&a, &b));
    } // obb_overlap_separated_boxes_false

    // @brief OBB overlap: rotated box intersecting the origin box reports overlap.
    #[test]
    fn obb_overlap_rotated_boxes_true() {
        let rect: IntPolygon = vec![ip(0.0, 0.0), ip(20.0, 0.0), ip(20.0, 8.0), ip(0.0, 8.0)];
        let a = obb_for_piece(&rect, Orientation(0), to_int(0.0), to_int(0.0));
        let b = obb_for_piece(&rect, Orientation(45), to_int(8.0), to_int(2.0));
        assert!(obb_overlap(&a, &b));
    } // obb_overlap_rotated_boxes_true
} // mod tests
