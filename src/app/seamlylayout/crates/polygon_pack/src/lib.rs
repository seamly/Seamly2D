// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! polygon_pack — polygon-tight irregular bin packing for pattern pieces.
//!
//! Hosts the future No-Fit Polygon (NFP) based irregular strip nesting
//! algorithm and its supporting geometry: outward polygon offset, integer-
//! coordinate arithmetic, NFP / IFP construction, and the greedy NFP placer.
//!
//! At crate creation only the public entry point [`pack`] exists; its body
//! silently falls back to [`layout_engine::pack_maxrects`] with the trial
//! set `[0, 180]` so user-visible behavior is unchanged while the real
//! implementation is built.  The dispatcher in `packing::pack_pieces` routes
//! non-orthogonal trial sets here; once the NFP implementation lands, the
//! `layout_engine` dependency below can be dropped.
//!
//! Planned phases (see `docs/layout-docs/LAYOUT_ROTATION_PLAN.md`):
//!
//! 1. Polygon construction from cutline group (drop notch siblings) +
//!    outward `gap_px` offset + scale to integer coordinates.
//! 2. Greedy NFP placer with Upper-Left-Fill (lowest-y, leftmost feasible).
//! 3. Deterministic multi-start over piece orderings.
//! 4. Simulated annealing over piece order + orientation.
//!
//! Geometry primitives are kept in this crate rather than the workspace
//! `geometry` crate to keep the integer-arithmetic convention contained
//! (`geometry` operates in `f64`).
//!
//! ## Skeleton modules (unwired during the silent-fallback period)
//!
//! * [`geom`] — integer-coord types, `SCALE = 10_000`, gap offset, rotation, AABB.
//! * [`nfp`] — `NfpKey`, `NfpCache`, `compute_pair`; honors the `NFP(A,B) = -NFP(B,A)`
//!   identity to halve the cache.
//! * [`placer`] — Upper-Left-Fill placer + Inner-Fit Polygon container constraint.
//!
//! These are `pub(crate)` until Stage 1 lands; the public surface remains
//! [`pack`] alone so callers stay decoupled from the in-progress contract.

mod geom;
mod nfp;
mod placer;
pub mod svg_extract;

use pack_types::{FreeRect, PackResult, Placed, Rect};

// @brief Closed pattern-piece outline in user-space coordinates.
//
// Vertices are stored as `(x, y)` pairs in the same units the caller uses
// for `Rect` dimensions and `bin_w` / `bin_h` (typically pixels at the
// layout PPI).  By convention the polygon is closed implicitly: the last
// vertex is **not** repeated; the closing edge is from `vertices.last()`
// back to `vertices.first()`.
//
// Construction invariants (per `LAYOUT_ROTATION_PLAN.md`, enforced upstream
// by `cxxqt_bridge::piece_extractor` once the polygon variant lands):
//   * Already a closed straight-line polyline (no curves to interpolate).
//   * No holes, no self-intersections, no mirrored copies.
//   * No transforms — coordinates are already in the layout frame.
//   * Notch sibling groups dropped — silhouette is the cutline only.
//
// The internal scaled-integer representation lives in `geom::IntPolygon`;
// callers never see it.  This boundary keeps the public API independent of
// the underlying clipper library (`i_overlay` today).
#[derive(Debug, Clone, Default)]
pub struct Polygon {
    /// Closed-loop vertices in user-space units.  Last vertex != first.
    pub vertices: Vec<(f64, f64)>,
} // struct Polygon

impl Polygon {
    /// @brief Construct a polygon from a slice of `(x, y)` user-space pairs.
    pub fn new(vertices: Vec<(f64, f64)>) -> Self {
        Self { vertices }
    } // fn new
} // impl Polygon

// @brief Polygon-tight (NFP) bin packing — STUB.
//
// Future home of a No-Fit-Polygon based irregular strip nester that consults
// the piece outline rather than its axis-aligned bounding box, so non-
// orthogonal rotations (45°, 22.5°, 10°, ...) can produce tight packings
// for concave pattern pieces.
//
// **Today this is not implemented.**  The body silently falls back to
// `layout_engine::pack_maxrects` with the trial set `[0, 180]`, so callers
// still get a usable layout while the real implementation is under
// construction.  The `_trial_angles_deg` argument is accepted for forward
// compatibility with the eventual contract; once NFP lands it will drive
// the per-placement orientation trial.
//
// @param bin_w              Content rectangle width in pixels.
// @param bin_h              Content rectangle height in pixels.
// @param gap_px             Minimum clearance in pixels between adjacent pieces.
// @param rects              Input rectangles; original index is preserved on placements.
// @param _trial_angles_deg  Future trial set; unused while the body is the
//                           silent-fallback stub.
// @return                   (placements, free-rect creation history) — same
//                           contract as `layout_engine::pack_maxrects`.
pub fn pack(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    rects: &[Rect],
    _trial_angles_deg: &[u16],
) -> PackResult<(Vec<Placed>, Vec<FreeRect>)> {
    // TODO: implement NFP-based irregular strip packing (see crate-level docs
    // for the planned phases).  Until then, silently fall back to MaxRects
    // with the [0, 180] trial set so callers still get a usable layout.
    layout_engine::pack_maxrects(bin_w, bin_h, gap_px, rects, &[0, 180])
} // fn pack

// @brief Polygon-tight (NFP) bin packing.
//
// Forward-facing polygon entry point.  Routes through preprocessing
// (user-space → scaled-int + outward gap offset) and into the Stage 1
// Upper-Left-Fill placer in [`placer`].
//
// Polygon ↔ rect correspondence: `polygons[i]` is the outline of the piece
// whose AABB is `rects[i]`.  The two slices must have the same length;
// `placements[k].id` indexes into both.
//
// ## `gap_px` semantics
//
// Matches the rect packer: `gap_px` is the **total** clearance between two
// adjacent placed pieces, not a per-piece offset.  Each polygon is therefore
// inflated by `gap_px / 2` outward; two such inflated outlines touching =
// `gap_px` between the original cutlines.  Sub-pixel halves (e.g. `gap_px=5`
// → 2.5 px per side) are handled losslessly because the offset operates in
// scaled-int space (`SCALE = 10_000`).
//
// @param bin_w             Content rectangle width in pixels.
// @param bin_h             Content rectangle height in pixels.
// @param gap_px            Minimum clearance in pixels between adjacent pieces.
// @param polygons          Per-piece outline polygons in user-space (pixels).
//                          Index-aligned with `rects`.
// @param rects             AABBs of the un-offset polygons; surfaced in
//                          `PackError::TooLarge` and used by the area-sort
//                          ordering inside the placer.
// @param trial_angles_deg  Per-piece rotation trial set in degrees.
// @return                  (placements, free-rect history) — same contract as
//                          [`pack`] / `layout_engine::pack_maxrects`.  The
//                          free-rect history is empty for Stage 1; a coarse
//                          AABB decomposition can populate it later.
pub fn pack_polygons(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    polygons: &[Polygon],
    rects: &[Rect],
    trial_angles_deg: &[u16],
) -> PackResult<(Vec<Placed>, Vec<FreeRect>)> {
    pack_polygons_with_progress(
        bin_w,
        bin_h,
        gap_px,
        polygons,
        rects,
        trial_angles_deg,
        None,
    )
} // fn pack_polygons

// @brief Progress-enabled polygon-tight packing entry point.
//
// Behaves like [`pack_polygons`] and additionally invokes `on_piece_begin`
// once at the beginning of each piece-placement attempt with
// `(current_piece_1_based, total_pieces)`.
pub fn pack_polygons_with_progress(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    polygons: &[Polygon],
    rects: &[Rect],
    trial_angles_deg: &[u16],
    mut on_piece_begin: Option<&mut dyn FnMut(usize, usize)>,
) -> PackResult<(Vec<Placed>, Vec<FreeRect>)> {
    let t_total = std::time::Instant::now();

    // Diagnostic entry log: piece count, total input vertex count, gap, bin,
    // and trial set.  Heavy polygons (many vertices) drive NFP and overlay
    // costs roughly quadratically, so total_verts is the single best
    // predictor of run time on this path.
    let total_verts_in: usize = polygons.iter().map(|p| p.vertices.len()).sum();
    log::info!(
        "[polygon_pack] pack_polygons entry: pieces={}, total_verts={}, gap_px={}, bin={}x{}, trial_set={:?}",
        polygons.len(), total_verts_in, gap_px, bin_w, bin_h, trial_angles_deg,
    );

    // Convert public f64-vertex polygons to the scaled-integer form the
    // NFP / IFP / placer pipeline consumes, then inflate by half the gap
    // budget so two touching offset polygons leave `gap_px` between
    // cutlines (see "gap_px semantics" in the docstring).
    let t_pre = std::time::Instant::now();
    let half_gap_int = geom::to_int(gap_px as f64 / 2.0);
    let int_polys: Vec<geom::IntPolygon> = polygons
        .iter()
        .map(|p| {
            let raw = geom::polygon_to_int(p);
            geom::offset_outward(&raw, half_gap_int)
        })
        .collect();
    let total_verts_offset: usize = int_polys.iter().map(|p| p.len()).sum();
    log::info!(
        "[polygon_pack] preprocess done in {} ms; offset polys total_verts={} (delta={})",
        t_pre.elapsed().as_millis(),
        total_verts_offset,
        total_verts_offset as i64 - total_verts_in as i64,
    );

    // The cache lives only for this call: Stage 1 is single-call greedy.
    // Stages 3 / 4 will hoist it into a multi-start outer loop so cached
    // NFP pairs amortise across orderings.
    let t_placer = std::time::Instant::now();
    let mut cache = nfp::NfpCache::new();
    let placer_result = placer::place_upper_left_fill(
        bin_w, bin_h, gap_px,
        &int_polys, rects, trial_angles_deg,
        &mut cache,
        on_piece_begin.take(),
    );
    log::info!(
        "[polygon_pack] placer done in {} ms; nfp_cache hits={}, misses={}, compute_ms_total={}",
        t_placer.elapsed().as_millis(),
        cache.hits(), cache.misses(), cache.compute_ms(),
    );
    let (mut placements, free) = placer_result?;

    // Strip the per-side gap offset from each reported AABB so callers see
    // cutline placements (matches `pack_maxrects`' contract and what the
    // SVG assembler expects).  The placer reports the offset polygon's
    // AABB; the cutline sits inset by `gap_px / 2` on each side, so the
    // top-left shifts in by that amount and w/h shrink by twice it.
    let half_gap_px = gap_px as f64 / 2.0;
    for p in &mut placements {
        p.x = ((p.x as f64) + half_gap_px).round() as u32;
        p.y = ((p.y as f64) + half_gap_px).round() as u32;
        p.w = ((p.w as f64) - 2.0 * half_gap_px).max(0.0).round() as u32;
        p.h = ((p.h as f64) - 2.0 * half_gap_px).max(0.0).round() as u32;
    } // for placement

    log::info!(
        "[polygon_pack] pack_polygons total elapsed: {} ms ({} placements)",
        t_total.elapsed().as_millis(),
        placements.len(),
    );

    Ok((placements, free))
} // fn pack_polygons_with_progress

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief The stub returns the same placement contract as MaxRects with
    // [0, 180]: every piece carries rotation_deg = 0 (first trial-set value).
    #[test]
    fn stub_falls_back_to_maxrects() {
        let rects = [Rect::new(8, 4), Rect::new(4, 4)];
        let (placed, _) = pack(16, 16, 0, &rects, &[0, 22, 45, 67]).expect("pack ok");
        assert_eq!(placed.len(), rects.len());
        assert!(placed.iter().all(|p| p.rotation_deg == 0));
    } // stub_falls_back_to_maxrects

    // @brief End-to-end pack_polygons run with two axis-aligned pieces:
    // the polygon-tight placer is invoked, and for axis-aligned inputs the
    // 0° trial wins on the (placed_y, placed_x) comparator (its rotated
    // AABB is no larger than any other trial-set angle's), so each
    // placement carries rotation_deg = 0.  A non-trivial test of rotation
    // selection lives in `placer::tests::place_rotation_used_when_only_…`.
    #[test]
    fn pack_polygons_axis_aligned_pieces_pick_zero_rotation() {
        let rects = [Rect::new(8, 4), Rect::new(4, 4)];
        let polys = [
            Polygon::new(vec![(0.0, 0.0), (8.0, 0.0), (8.0, 4.0), (0.0, 4.0)]),
            Polygon::new(vec![(0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)]),
        ];
        let (placed, _) =
            pack_polygons(16, 16, 0, &polys, &rects, &[0, 45]).expect("pack ok");
        assert_eq!(placed.len(), rects.len());
        assert!(placed.iter().all(|p| p.rotation_deg == 0));
    } // pack_polygons_axis_aligned_pieces_pick_zero_rotation

    // @brief pack_polygons honors gap_px as total clearance between
    // adjacent cutlines.  Two 10×10 squares with gap_px = 4: the first's
    // cutline lands at (2, 2) (gap/2 inset from the bin edge from the
    // outward offset); the second's cutline at (16, 2), leaving exactly
    // 4 px between cutline edges (12 → 16).  Both pieces report w = h = 10
    // — the offset is internal preprocessing, not user-visible.
    #[test]
    fn pack_polygons_respects_gap_px() {
        let sq = Polygon::new(vec![(0.0, 0.0), (10.0, 0.0), (10.0, 10.0), (0.0, 10.0)]);
        let polys = [sq.clone(), sq];
        let rects = [Rect::new(10, 10), Rect::new(10, 10)];

        let (placed, _) =
            pack_polygons(100, 100, 4, &polys, &rects, &[0]).expect("pack ok");

        let p0 = &placed[0];
        let p1 = &placed[1];
        assert_eq!(p0.id, 0);
        assert_eq!(p0.x, 2);
        assert_eq!(p0.y, 2);
        assert_eq!(p0.w, 10);
        assert_eq!(p0.h, 10);

        assert_eq!(p1.id, 1);
        assert_eq!(p1.x, 16);
        assert_eq!(p1.y, 2);
        assert_eq!(p1.w, 10);
        assert_eq!(p1.h, 10);

        // Cutline-to-cutline gap = 16 − 12 = 4, exactly gap_px.
        let between = p1.x - (p0.x + p0.w);
        assert_eq!(between, 4);
    } // pack_polygons_respects_gap_px

    // @brief The stub honors the rectangle packer's bounds — pieces larger
    // than the bin still produce TooLarge.
    #[test]
    fn stub_propagates_too_large() {
        let rects = [Rect::new(20, 5)];
        let err = pack(10, 10, 0, &rects, &[10]).unwrap_err();
        assert!(matches!(err, pack_types::PackError::TooLarge { .. }));
    } // stub_propagates_too_large
} // mod tests
