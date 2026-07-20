// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! packing — bin-packing dispatcher.
//!
//! Single entry point for consumer code (the Qt bridge, `layout_tiling`, the
//! CLI).  `pack_pieces` looks at the rotation trial set and routes to the
//! right packer:
//!
//! * Trial set ⊆ {0, 180}                    → `layout_engine::pack_maxrects`
//! * Trial set contains any non-orthogonal θ → `polygon_pack::pack`
//!
//! Until `polygon_pack::pack` lands its real NFP-based implementation, that
//! crate's stub silently falls back to MaxRects with `[0, 180]`, so every
//! routing decision still produces a usable layout.
//!
//! Re-exports the shared `pack_types` primitives and the rectangle-packer
//! entry points so consumers depend on just this one crate.

pub use layout_engine::{pack_maxrects, pack_maxrects_multi_angle, pack_maxrects_multi_angle_lenient, pack_shelves, validate_placements};
pub use pack_types::{FreeRect, PackError, PackResult, Placed, Rect};
pub use polygon_pack::Polygon;

// @brief Dispatch a packing job to the right packer based on the rotation trial set.
//
// Routing:
//   - Trial set ⊆ {0, 180} → `layout_engine::pack_maxrects` (rectangle packer; AABB unchanged).
//   - Trial set contains any non-orthogonal angle → `polygon_pack::pack` (NFP placer).
//
// `layout_tiling::layout_settings::rotation_trial_set_deg(&LayoutSettings)`
// produces the trial set from the user's layoutMode + rotationStep choice.
// This dispatcher is the single entry point that callers (the bridge driver,
// tiled-candidate evaluation) should use; it lets the packer choice change
// without touching call sites.
//
// @param bin_w             Content rectangle width in pixels.
// @param bin_h             Content rectangle height in pixels.
// @param gap_px            Minimum clearance in pixels between adjacent pieces.
// @param rects             Input rectangles; original index is preserved on placements.
// @param trial_angles_deg  Per-piece rotation trial set in degrees.
// @return                  (placements, free-rect creation history) on success.
pub fn pack_pieces(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    rects: &[Rect],
    trial_angles_deg: &[u16],
) -> PackResult<(Vec<Placed>, Vec<FreeRect>)> {
    // True when every angle is 0 or 180 (or the set is empty — treated as [0]).
    let all_orthogonal_flips = trial_angles_deg
        .iter()
        .all(|&a| a == 0 || a == 180);

    if all_orthogonal_flips {
        // AABB is identical at 0 and 180, so the rectangle packer can honor
        // these trial sets exactly.
        layout_engine::pack_maxrects(bin_w, bin_h, gap_px, rects, trial_angles_deg)
    } else {
        // Practical Rotate backend: multi-angle MaxRects over rotated AABBs.
        layout_engine::pack_maxrects_multi_angle(bin_w, bin_h, gap_px, rects, trial_angles_deg, None)
    } // if all_orthogonal_flips
} // fn pack_pieces

// @brief Dispatch a packing job, accepting piece outlines alongside their AABBs.
//
// Same routing rule as [`pack_pieces`]: orthogonal-only trial sets go to the
// rectangle packer (polygons ignored); anything non-orthogonal goes to
// `polygon_pack::pack_polygons`.  This is the forward-facing entry point —
// callers that have polygon outlines available (the bridge's polygon
// extractor, once it lands) should call this instead of `pack_pieces` so
// they're already on the path that will benefit from the NFP placer when
// Stage 1 ships.
//
// `polygons[i]` corresponds to `rects[i]`; the two slices must be the same
// length.  When the orthogonal route is taken, the polygons are ignored
// entirely (the AABB packer doesn't need them).
//
// @param bin_w             Content rectangle width in pixels.
// @param bin_h             Content rectangle height in pixels.
// @param gap_px            Minimum clearance in pixels between adjacent pieces.
// @param polygons          Per-piece outline polygons in user-space (pixels).
// @param rects             AABBs of the polygons; index-aligned with `polygons`.
// @param trial_angles_deg  Per-piece rotation trial set in degrees.
// @return                  (placements, free-rect creation history) on success.
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

// @brief Dispatch pack_polygons with optional per-piece progress callback.
//
// Callback is invoked only when the route is polygon-tight (non-orthogonal
// trial set); the MaxRects route ignores it.
pub fn pack_polygons_with_progress(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    _polygons: &[Polygon],
    rects: &[Rect],
    trial_angles_deg: &[u16],
    on_piece_begin: Option<&mut dyn FnMut(usize, usize)>,
) -> PackResult<(Vec<Placed>, Vec<FreeRect>)> {
    let all_orthogonal_flips = trial_angles_deg
        .iter()
        .all(|&a| a == 0 || a == 180);

    if all_orthogonal_flips {
        // Orthogonal route: the rectangle packer ignores polygons by design.
        log::info!(
            "[packing::pack_polygons] dispatch=MaxRects (orthogonal trial set {:?}); pieces={}, bin={}x{}",
            trial_angles_deg, rects.len(), bin_w, bin_h,
        );
        layout_engine::pack_maxrects(bin_w, bin_h, gap_px, rects, trial_angles_deg)
    } else {
        // Practical Rotate route: ignore polygon detail for now and use the
        // multi-angle MaxRects backend over rotated AABBs.
        log::info!(
            "[packing::pack_polygons] dispatch=multi-angle MaxRects (non-orthogonal trial set {:?}); pieces={}, bin={}x{}",
            trial_angles_deg, rects.len(), bin_w, bin_h,
        );
        layout_engine::pack_maxrects_multi_angle(
            bin_w,
            bin_h,
            gap_px,
            rects,
            trial_angles_deg,
            on_piece_begin,
        )
    } // if all_orthogonal_flips
} // fn pack_polygons_with_progress

// @brief Lenient dispatch: pack what fits, skip what doesn't, report unplaced ids.
//
// The "warn, don't fail" counterpart to [`pack_polygons_with_progress`].  Same
// routing rule, but instead of returning a `PackError` on the first piece that
// can't be placed it skips that piece and continues, returning the original
// indices of every piece that could not be placed.  Used by the non-tiled
// layout path so the layout still renders every piece that fit while the caller
// surfaces the unplaced piece ids to the user as a warning.
//
// Both routes currently resolve to the multi-angle MaxRects backend (the
// rectangle packer handles orthogonal trial sets exactly via identical rotated
// AABBs); the polygon outlines are accepted for signature parity and ignored.
//
// @param bin_w             Content rectangle width in pixels.
// @param bin_h             Content rectangle height in pixels.
// @param gap_px            Minimum clearance in pixels between adjacent pieces.
// @param polygons          Per-piece outline polygons (accepted, currently ignored).
// @param rects             AABBs of the polygons; index-aligned with `polygons`.
// @param trial_angles_deg  Per-piece rotation trial set in degrees.
// @param on_piece_begin    Optional per-piece progress callback (1-based, total).
// @return                  (placements, free-rect history, unplaced original ids).
pub fn pack_polygons_lenient(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    _polygons: &[Polygon],
    rects: &[Rect],
    trial_angles_deg: &[u16],
    on_piece_begin: Option<&mut dyn FnMut(usize, usize)>,
) -> (Vec<Placed>, Vec<FreeRect>, Vec<usize>) {
    let all_orthogonal_flips = trial_angles_deg
        .iter()
        .all(|&a| a == 0 || a == 180);

    log::info!(
        "[packing::pack_polygons_lenient] dispatch={} (trial set {:?}); pieces={}, bin={}x{}",
        if all_orthogonal_flips { "MaxRects" } else { "multi-angle MaxRects" },
        trial_angles_deg, rects.len(), bin_w, bin_h,
    );

    layout_engine::pack_maxrects_multi_angle_lenient(
        bin_w,
        bin_h,
        gap_px,
        rects,
        trial_angles_deg,
        on_piece_begin,
    )
} // fn pack_polygons_lenient

// ---------------------------------------------------------------------------
// Tests — dispatcher routing only.  Per-packer behavior is tested in each
// packer's own crate.
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Dispatcher: orthogonal-only trial set routes to MaxRects.
    #[test]
    fn dispatcher_routes_orthogonal_to_maxrects() {
        let rects = [Rect::new(8, 4), Rect::new(4, 4)];
        // [0, 180] is orthogonal-only; routed to MaxRects which records first angle.
        let (placed, _) = pack_pieces(16, 16, 0, &rects, &[0, 180]).expect("pack ok");
        assert_eq!(placed.len(), rects.len());
        assert!(placed.iter().all(|p| p.rotation_deg == 0));
    } // dispatcher_routes_orthogonal_to_maxrects

    // @brief Dispatcher: any non-orthogonal angle routes to practical
    // multi-angle MaxRects.
    #[test]
    fn dispatcher_uses_multi_angle_for_non_orthogonal_trial_set() {
        let rects = [Rect::new(8, 4), Rect::new(4, 4)];
        // Trial set contains non-orthogonal angles; placement rotation is now
        // selected from the provided trial set by the multi-angle backend.
        let (placed, _) = pack_pieces(16, 16, 0, &rects, &[0, 22, 45, 67]).expect("pack ok");
        assert_eq!(placed.len(), rects.len());
        assert!(placed.iter().all(|p| [0, 22, 45, 67].contains(&p.rotation_deg)));
    } // dispatcher_uses_multi_angle_for_non_orthogonal_trial_set

    // @brief Polygon-aware dispatcher: orthogonal-only trial set still routes
    // to MaxRects; polygons are accepted and ignored on this path.
    #[test]
    fn pack_polygons_orthogonal_routes_to_maxrects() {
        let rects = [Rect::new(8, 4), Rect::new(4, 4)];
        let polys = [
            Polygon::new(vec![(0.0, 0.0), (8.0, 0.0), (8.0, 4.0), (0.0, 4.0)]),
            Polygon::new(vec![(0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)]),
        ];
        let (placed, _) = pack_polygons(16, 16, 0, &polys, &rects, &[0, 180]).expect("pack ok");
        assert_eq!(placed.len(), rects.len());
        assert!(placed.iter().all(|p| p.rotation_deg == 0));
    } // pack_polygons_orthogonal_routes_to_maxrects

    // @brief Polygon-aware dispatcher: non-orthogonal trial set routes to
    // practical multi-angle MaxRects.  Verifies the route is wired and
    // returns placements.
    #[test]
    fn pack_polygons_non_orthogonal_routes_to_multi_angle_maxrects() {
        let rects = [Rect::new(8, 4), Rect::new(4, 4)];
        let polys = [
            Polygon::new(vec![(0.0, 0.0), (8.0, 0.0), (8.0, 4.0), (0.0, 4.0)]),
            Polygon::new(vec![(0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)]),
        ];
        let (placed, _) =
            pack_polygons(16, 16, 0, &polys, &rects, &[0, 45]).expect("pack ok");
        assert_eq!(placed.len(), rects.len());
    } // pack_polygons_non_orthogonal_routes_to_multi_angle_maxrects

    // @brief Lenient dispatch skips an oversize piece and reports its id rather
    // than erroring, placing the pieces that fit.
    #[test]
    fn pack_polygons_lenient_skips_oversize_and_reports() {
        // id 0 is 40x40 (too large for the 16x16 bin); id 1 fits.
        let rects = [Rect::new(40, 40), Rect::new(4, 4)];
        let polys = [
            Polygon::new(vec![(0.0, 0.0), (40.0, 0.0), (40.0, 40.0), (0.0, 40.0)]),
            Polygon::new(vec![(0.0, 0.0), (4.0, 0.0), (4.0, 4.0), (0.0, 4.0)]),
        ];
        let (placed, _free, unplaced) =
            pack_polygons_lenient(16, 16, 0, &polys, &rects, &[0], None);
        assert_eq!(unplaced, vec![0]);
        assert_eq!(placed.len(), 1);
        assert_eq!(placed[0].id, 1);
    } // pack_polygons_lenient_skips_oversize_and_reports
}
