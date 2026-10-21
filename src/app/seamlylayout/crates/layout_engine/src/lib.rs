// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! layout_engine — rectangle bin-packing implementations.
//!
//! Hosts the rectangle-only packers: greedy shelf (`pack_shelves`) and
//! MaxRects (`pack_maxrects`).  The trial-set dispatcher that chooses
//! between this crate and `polygon_pack` lives one level up in the
//! `packing` crate, so consumer code (the bridge, `layout_tiling`) calls
//! `packing::pack_pieces` rather than reaching into a specific packer.
//!
//! Shared types (`Rect`, `Placed`, `PackError`, `PackResult`, `FreeRect`)
//! live in the leaf `pack_types` crate and are re-exported here for
//! convenience.
//!
//! `maxrects_order_search` holds the MaxRects placement and the piece-order
//! search; `up_left_compaction` moves placed pieces up and left to the gap.

mod maxrects_order_search;
mod up_left_compaction;

pub use pack_types::{FreeRect, PackError, PackResult, Placed, Rect};

use maxrects_order_search::{search_layout, PackJob, SearchDepth};
use tracing::debug;

// @brief Greedy shelf (first-fit, height-decreasing) bin packing for rectangles.
//
// Rectangles are sorted by descending height to improve packing density, then placed
// left-to-right on the current shelf. When a rectangle does not fit on the current shelf,
// a new shelf is opened below. Fails if any rectangle exceeds bin dimensions or
// vertical space is exhausted.
//
// @param bin_w Bin width.
// @param bin_h Bin height.
// @param rects Input rectangles; order is preserved via their original index.
// @return List of placed rectangles with positions.
pub fn pack_shelves(bin_w: u32, bin_h: u32, rects: &[Rect]) -> PackResult<Vec<Placed>> {
    debug!("Called pack_shelves (Along Grainline layout) with bin_w={}, bin_h={}, rects={:?}", bin_w, bin_h, rects);
    // Work on indices to preserve original order metadata.
    let mut order: Vec<usize> = (0..rects.len()).collect();
    // Sort by height (desc), then width (desc) for deterministic layout.
    order.sort_by_key(|&i| (std::cmp::Reverse(rects[i].h), std::cmp::Reverse(rects[i].w)));

    let mut placements = Vec::with_capacity(rects.len());
    let mut shelf_y: u32 = 0;
    let mut shelf_height: u32 = 0;
    let mut shelf_x: u32 = 0;

    for id in order {
        let r = rects[id];

        // Reject rectangles larger than the bin.
        if r.w > bin_w || r.h > bin_h {
            return Err(PackError::TooLarge {
                id,
                w: r.w,
                h: r.h,
                bin_w,
                bin_h,
            });
        }

        // Start a new shelf if it doesn't fit horizontally.
        if shelf_x + r.w > bin_w {
            shelf_y = shelf_y.saturating_add(shelf_height);
            shelf_x = 0;
            shelf_height = 0;
        }

        // Update shelf height and check vertical fit.
        shelf_height = shelf_height.max(r.h);
        if shelf_y + shelf_height > bin_h {
            return Err(PackError::NoSpace { id });
        }

        // Place the rectangle.  pack_shelves does not consider rotation;
        // pieces are always emitted upright (rotation_deg = 0).
        placements.push(Placed {
            id,
            x: shelf_x,
            y: shelf_y,
            w: r.w,
            h: r.h,
            rotation_deg: 0,
        });
        shelf_x += r.w;
    }

    // Return placements in original id order for convenience.
    placements.sort_by_key(|p| p.id);
    Ok(placements)
}

// @brief MaxRects bin packing — significantly more efficient than shelf packing.
//
// Box-based: keeps a list of maximal free boxes (free boxes may overlap).
// Each piece goes to the top-left corner of the free box with the best score,
// and the piece is cut out of every free box it overlaps (left, right, top
// and bottom strips).  Free boxes inside other free boxes are pruned.
//
// The gap is added to the right and bottom of every piece and of the bin, so
// every pair of pieces keeps the full `gap_px` on all four sides.  After the
// pass, every piece moves up, then left, until the gap or the bin edge stops it.
//
// A greedy pass leaves the last pieces only the boxes the earlier pieces left.
// So `maxrects_order_search` packs five sort orders with two box scores, then
// searches over the piece order.  It keeps the layout with the least unplaced
// area, then the least height, then the least whitespace between side-by-side
// pieces.  The search is deterministic.
//
// Rotation handling: this packer only supports trial sets that are subsets of
// `{0, 180}` — at those angles the rotated AABB is identical to the upright
// AABB, so the packing geometry is unaffected by the choice of angle.  The
// recorded `Placed.rotation_deg` is `trial_angles_deg.first().copied().unwrap_or(0)`,
// which the dispatcher (`packing::pack_pieces`) sets up to honor the user's
// layoutMode:
//   alongGrainline → trial set [0, 180] → rotation_deg = 0
//   withNap, step=0   → [0]   → 0
//   withNap, step=180 → [180] → 180
//   any               → [0, 90, 180, 270] → routed to `pack_maxrects_multi_angle`
// Non-orthogonal trial sets MUST be routed to `polygon_pack::pack`, not here.
//
// @param bin_w             Content rectangle width in pixels.
// @param bin_h             Content rectangle height in pixels.
// @param gap_px            Minimum clearance in pixels between adjacent pieces.
// @param rects             Input rectangles; order is preserved via their original index.
// @param trial_angles_deg  Angles to record on placements; must be ⊆ {0, 180}.
// @return                  (placements sorted by original id,
//                           all free rects in creation order for the debug overlay).
pub fn pack_maxrects(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    rects: &[Rect],
    trial_angles_deg: &[u16],
) -> PackResult<(Vec<Placed>, Vec<FreeRect>)> {
    pack_maxrects_multi_angle(bin_w, bin_h, gap_px, rects, trial_angles_deg, None)
} // fn pack_maxrects

// @brief Multi-angle MaxRects backend used for practical Rotate layouts.
//
// For each piece and each candidate free box, this function evaluates all
// trial angles by computing the rotated AABB dimensions and selecting the
// box with the best score.  This preserves MaxRects speed while supporting
// 90°/45° trial sets without the expensive polygon NFP path.
//
// Packs the start orders only (see `SearchDepth::StartOrders`).
//
// @param on_progress  Optional callback (steps done, total steps) for the order search.
pub fn pack_maxrects_multi_angle(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    rects: &[Rect],
    trial_angles_deg: &[u16],
    on_progress: Option<&mut dyn FnMut(usize, usize)>,
) -> PackResult<(Vec<Placed>, Vec<FreeRect>)> {
    // Strict contract: a piece that can't be placed aborts the whole pack with
    // the matching PackError.  Implemented on top of the shared collect worker
    // so the strict and lenient entry points stay in sync.  `unplaced` lists
    // the too-large pieces first, so a too-large piece is reported first.
    // Sheet assignment and the tiled width scan call this many times, so it
    // packs the start orders only.
    let (placements, free_rects, unplaced) = pack_maxrects_collect(
        bin_w,
        bin_h,
        gap_px,
        rects,
        trial_angles_deg,
        SearchDepth::StartOrders,
        on_progress,
    );
    if let Some(u) = unplaced.into_iter().next() {
        return Err(match u.reason {
            UnplacedReason::TooLarge { w, h, bin_w, bin_h } =>
                PackError::TooLarge { id: u.id, w, h, bin_w, bin_h },
            UnplacedReason::NoSpace =>
                PackError::NoSpace { id: u.id },
        });
    }
    Ok((placements, free_rects))
} // fn pack_maxrects_multi_angle

// @brief Lenient MaxRects: place what fits, skip what doesn't, report the rest.
//
// Same packing geometry as [`pack_maxrects_multi_angle`], but instead of
// aborting on the first unplaceable piece it skips that piece — leaving the
// free space untouched for later, smaller pieces — and continues.  Pieces that
// are physically larger than the bin at every trial angle, and pieces for which
// no free space remains, are both reported as "unplaced" rather than as a hard
// error.  This backs the non-tiled layout path's "warn, don't fail" behavior:
// the layout still renders every piece that fit, and the caller surfaces the
// unplaced piece ids to the user as a warning.
//
// Runs the full order search: one layout is one call.
//
// @param on_progress  Optional callback (steps done, total steps) for the order search.
// @return (placements sorted by original id, free-rect creation history,
//          unplaced original ids sorted ascending).
pub fn pack_maxrects_multi_angle_lenient(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    rects: &[Rect],
    trial_angles_deg: &[u16],
    on_progress: Option<&mut dyn FnMut(usize, usize)>,
) -> (Vec<Placed>, Vec<FreeRect>, Vec<usize>) {
    let (placements, free_rects, unplaced) =
        pack_maxrects_collect(bin_w, bin_h, gap_px, rects, trial_angles_deg, SearchDepth::Full, on_progress);
    let mut unplaced_ids: Vec<usize> = unplaced.into_iter().map(|u| u.id).collect();
    unplaced_ids.sort_unstable();
    (placements, free_rects, unplaced_ids)
} // fn pack_maxrects_multi_angle_lenient

// @brief Lenient MaxRects on media that grows in height until every piece fits.
//
// Packs once into a bin as tall as every piece that fits the width, stacked:
// that bin holds them all.  The search ranks layouts by height, so the layout
// is as short as the search finds.  A piece wider than the bin at every trial
// angle stays unplaced.
//
// @return (placements sorted by original id, free-rect creation history,
//          unplaced original ids sorted ascending,
//          bin height used: `bin_h`, or the content height when it is larger).
pub fn pack_maxrects_multi_angle_extending(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    rects: &[Rect],
    trial_angles_deg: &[u16],
    on_progress: Option<&mut dyn FnMut(usize, usize)>,
) -> (Vec<Placed>, Vec<FreeRect>, Vec<usize>, u32) {
    // Stacked height of every piece that fits the width; too-wide pieces add nothing.
    let all_stacked_h = rects
        .iter()
        .filter_map(|&r| stacked_height(r, bin_w, gap_px, trial_angles_deg))
        .fold(0u32, |sum, h| sum.saturating_add(h));
    let pack_h = bin_h.max(all_stacked_h);

    let (placements, free_rects, unplaced) =
        pack_maxrects_multi_angle_lenient(bin_w, pack_h, gap_px, rects, trial_angles_deg, on_progress);

    // The media keeps its own length unless the pieces need more.
    let content_h = placements.iter().map(|p| p.y + p.h).max().unwrap_or(0);
    let used_h = bin_h.max(content_h);
    if used_h > bin_h {
        debug!(
            "pack_maxrects_multi_angle_extending: bin_h={} grows to {} for {} piece(s)",
            bin_h, used_h, placements.len()
        );
    } // if the media grows
    (placements, free_rects, unplaced, used_h)
} // fn pack_maxrects_multi_angle_extending

// @brief Smallest bin width that holds every piece, each in its narrowest
// trial angle.  Extended media widens to it, so no piece is too wide.
//
// @return 0 when `rects` is empty.
pub fn required_bin_width(rects: &[Rect], trial_angles_deg: &[u16]) -> u32 {
    rects
        .iter()
        .filter_map(|r| {
            piece_trial_set(trial_angles_deg, r.free_rotation)
                .into_iter()
                .map(|deg| rotated_rect_dims(r.w, r.h, deg).0)
                .min()
        })
        .max()
        .unwrap_or(0)
} // fn required_bin_width

// @brief True when the piece fits an empty `bin_w` × `bin_h` bin at one of its trial angles.
pub fn fits_bin(r: Rect, bin_w: u32, bin_h: u32, trial_angles_deg: &[u16]) -> bool {
    piece_trial_set(trial_angles_deg, r.free_rotation)
        .into_iter()
        .map(|deg| rotated_rect_dims(r.w, r.h, deg))
        .any(|(rw, rh)| rw <= bin_w && rh <= bin_h)
} // fn fits_bin

// @brief Trial angles for one piece: the layout mode's set, then, for a
// free-rotation piece, the quarter turns the set lacks.  Mode angles come
// first, so ties keep the mode's preferred orientation.
fn piece_trial_set(trial_angles_deg: &[u16], free_rotation: bool) -> Vec<u16> {
    let mut angles: Vec<u16> = if trial_angles_deg.is_empty() { vec![0] } else { trial_angles_deg.to_vec() };
    if free_rotation {
        for deg in [0u16, 90, 180, 270] {
            if !angles.contains(&deg) {
                angles.push(deg);
            }
        }
    }
    angles
} // fn piece_trial_set

// @brief Height a piece adds to a stack in a bin `bin_w` wide.
//
// @return The tallest rotated height over the piece's trial angles that fit
//         the width, plus the gap; None when no trial angle fits the width,
//         so no bin height can hold the piece.
fn stacked_height(r: Rect, bin_w: u32, gap_px: u32, trial_angles_deg: &[u16]) -> Option<u32> {
    piece_trial_set(trial_angles_deg, r.free_rotation)
        .into_iter()
        .map(|deg| rotated_rect_dims(r.w, r.h, deg))
        .filter(|&(rw, _)| rw <= bin_w)
        .map(|(_, rh)| rh.saturating_add(gap_px))
        .max()
} // fn stacked_height

// @brief Why a piece could not be placed.  Recorded by the collect worker so
// the strict wrapper can reconstruct the exact PackError it historically returned.
enum UnplacedReason {
    // Piece does not fit the empty bin at any trial orientation.
    TooLarge { w: u32, h: u32, bin_w: u32, bin_h: u32 },
    // Piece fits an empty bin but no free space remained when its turn came.
    NoSpace,
} // enum UnplacedReason

// @brief One piece the packer could not place, with the reason.
struct UnplacedPiece {
    id: usize,
    reason: UnplacedReason,
} // struct UnplacedPiece

// @brief Shared MaxRects worker: places every piece it can and collects the
// rest instead of erroring.  Both the strict (`pack_maxrects_multi_angle`) and
// lenient (`pack_maxrects_multi_angle_lenient`) public entry points are thin
// adapters over this.
//
// @param depth  How much of the piece-order search to run.
// @return (placements sorted by id, free-box history of the chosen layout,
//          unplaced pieces: too-large pieces by area descending, then pieces
//          without room in placement order).
fn pack_maxrects_collect(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    rects: &[Rect],
    trial_angles_deg: &[u16],
    depth: SearchDepth,
    on_progress: Option<&mut dyn FnMut(usize, usize)>,
) -> (Vec<Placed>, Vec<FreeRect>, Vec<UnplacedPiece>) {
    debug!(
        "[debug] layout_engine\\src\\pack_maxrects_collect(): 1 Called with bin_w={}, bin_h={}, gap_px={}, rect_count={}, trial_angles_deg={:?}",
        bin_w,
        bin_h,
        gap_px,
        rects.len(),
        trial_angles_deg
    );

    // Orientations per piece that fit the empty bin.
    let job = PackJob::new(
        bin_w,
        bin_h,
        gap_px,
        rects,
        |r| piece_trial_set(trial_angles_deg, r.free_rotation),
        rotated_rect_dims,
    );

    // A piece larger than the bin at every trial angle is left out before the search.
    let mut too_large: Vec<usize> = (0..rects.len()).filter(|&id| !job.fits_empty_bin(id)).collect();
    too_large.sort_by_key(|&id| std::cmp::Reverse(rects[id].w as u64 * rects[id].h as u64));
    let mut unplaced: Vec<UnplacedPiece> = too_large
        .into_iter()
        .map(|id| UnplacedPiece {
            id,
            reason: UnplacedReason::TooLarge { w: rects[id].w, h: rects[id].h, bin_w, bin_h },
        })
        .collect();

    // Search the piece order; pieces without room in the best layout are left out.
    let layout = search_layout(&job, rects, depth, on_progress);
    unplaced.extend(layout.no_space.iter().map(|&id| UnplacedPiece { id, reason: UnplacedReason::NoSpace }));

    let mut placements = layout.placements;
    placements.sort_by_key(|p| p.id);
    (placements, layout.history, unplaced)
} // fn pack_maxrects_collect

// @brief Compute the axis-aligned bounding box of a w×h rectangle rotated by `deg`.
fn rotated_rect_dims(w: u32, h: u32, deg: u16) -> (u32, u32) {
    let d = deg % 360;
    if d == 0 || d == 180 {
        return (w, h);
    }
    if d == 90 || d == 270 {
        return (h, w);
    }

    let theta = (d as f64).to_radians();
    let c = theta.cos().abs();
    let s = theta.sin().abs();
    let rw = (w as f64) * c + (h as f64) * s;
    let rh = (w as f64) * s + (h as f64) * c;
    (rw.ceil() as u32, rh.ceil() as u32)
} // fn rotated_rect_dims

// @brief Validate that rectangles are within the bin and do not overlap.
// @param bin_w Bin width.
// @param bin_h Bin height.
// @param placed Rectangles to validate.
// @return True if all placements are valid.
pub fn validate_placements(bin_w: u32, bin_h: u32, placed: &[Placed]) -> bool {
    // Check bounds and pairwise overlaps.
    for p in placed {
        if p.x + p.w > bin_w || p.y + p.h > bin_h {
            return false;
        }
    }
    for i in 0..placed.len() {
        for j in (i + 1)..placed.len() {
            if overlaps(&placed[i], &placed[j]) {
                return false;
            }
        }
    }
    true
}

// @brief Axis-aligned rectangle overlap test.
fn overlaps(a: &Placed, b: &Placed) -> bool {
    let sep_x = a.x + a.w <= b.x || b.x + b.w <= a.x;
    let sep_y = a.y + a.h <= b.y || b.y + b.h <= a.y;
    !(sep_x || sep_y)
}

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Ensure rectangles pack without overlap and within bounds.
    #[test]
    fn packs_simple_set() {
        let bin_w = 16;
        let bin_h = 16;
        let rects = [
            Rect::new(8, 4),
            Rect::new(4, 4),
            Rect::new(6, 6),
            Rect::new(3, 2),
        ];
        let placed = pack_shelves(bin_w, bin_h, &rects).expect("pack ok");
        assert_eq!(placed.len(), rects.len());
        assert!(validate_placements(bin_w, bin_h, &placed));
    }

    // @brief Reject rectangles that exceed bin dimensions.
    #[test]
    fn rejects_too_large() {
        let rects = [Rect::new(20, 5)];
        let err = pack_shelves(10, 10, &rects).unwrap_err();
        assert!(matches!(err, PackError::TooLarge { .. }));
    }

    // @brief Reject when shelves exceed available height.
    #[test]
    fn runs_out_of_space() {
        let rects = [Rect::new(8, 6), Rect::new(8, 6), Rect::new(8, 6)];
        let err = pack_shelves(8, 12, &rects).unwrap_err();
        assert!(matches!(err, PackError::NoSpace { .. }));
    }

    // @brief Packing is deterministic (same result ordering).
    #[test]
    fn deterministic_packing() {
        let rects = [Rect::new(3, 4), Rect::new(3, 4), Rect::new(2, 5)];
        let first = pack_shelves(10, 10, &rects).unwrap();
        let second = pack_shelves(10, 10, &rects).unwrap();
        assert_eq!(first, second);
    }

    // @brief MaxRects records rotation_deg = 0 when given trial set [0].
    #[test]
    fn maxrects_records_zero_for_upright_only() {
        let rects = [Rect::new(8, 4), Rect::new(4, 4)];
        let (placed, _) = pack_maxrects(16, 16, 0, &rects, &[0]).expect("pack ok");
        assert!(placed.iter().all(|p| p.rotation_deg == 0));
    } // maxrects_records_zero_for_upright_only

    // @brief MaxRects records the first trial-set angle on every placement;
    // for [180] this means every piece carries rotation_deg = 180.
    #[test]
    fn maxrects_records_180_when_trial_set_is_flipped_only() {
        let rects = [Rect::new(8, 4), Rect::new(4, 4)];
        let (placed, _) = pack_maxrects(16, 16, 0, &rects, &[180]).expect("pack ok");
        assert!(placed.iter().all(|p| p.rotation_deg == 180));
    } // maxrects_records_180_when_trial_set_is_flipped_only

    // @brief alongGrainline trial set [0, 180]: AABB unchanged, rotation_deg = 0
    // (per the locked spec — flip is reserved for the future polygon packer).
    #[test]
    fn maxrects_along_grainline_records_zero() {
        let rects = [Rect::new(8, 4), Rect::new(4, 4)];
        let (placed, _) = pack_maxrects(16, 16, 0, &rects, &[0, 180]).expect("pack ok");
        assert!(placed.iter().all(|p| p.rotation_deg == 0));
    } // maxrects_along_grainline_records_zero

    // @brief A free-rotation piece too wide for the bin is turned 90°; a
    // piece without the flag stays in the mode's trial set and is left out.
    #[test]
    fn free_rotation_piece_turns_to_fit() {
        let rects = [Rect::new(20, 8).with_free_rotation(true)];
        let (placed, _free, unplaced) =
            pack_maxrects_multi_angle_lenient(10, 30, 0, &rects, &[0, 180], None);
        assert!(unplaced.is_empty());
        assert!(placed[0].rotation_deg == 90 || placed[0].rotation_deg == 270);
        assert_eq!((placed[0].w, placed[0].h), (8, 20));

        let fixed = [Rect::new(20, 8)];
        let (_placed, _free, unplaced) =
            pack_maxrects_multi_angle_lenient(10, 30, 0, &fixed, &[0, 180], None);
        assert_eq!(unplaced, vec![0]);
    } // free_rotation_piece_turns_to_fit

    // @brief A free-rotation piece that fits upright keeps the mode's angle.
    #[test]
    fn free_rotation_piece_prefers_mode_angle_on_tie() {
        let rects = [Rect::new(4, 4).with_free_rotation(true)];
        let (placed, _) = pack_maxrects(16, 16, 0, &rects, &[0, 180]).expect("pack ok");
        assert_eq!(placed[0].rotation_deg, 0);
    } // free_rotation_piece_prefers_mode_angle_on_tie

    // @brief Lenient pack with everything fitting: all pieces placed, no
    // unplaced ids — identical placements to the strict packer.
    #[test]
    fn lenient_places_all_when_everything_fits() {
        let rects = [Rect::new(8, 4), Rect::new(4, 4)];
        let (placed, _free, unplaced) =
            pack_maxrects_multi_angle_lenient(16, 16, 0, &rects, &[0], None);
        assert_eq!(placed.len(), 2);
        assert!(unplaced.is_empty());
        assert!(validate_placements(16, 16, &placed));
    } // lenient_places_all_when_everything_fits

    // @brief Lenient pack skips a piece larger than the bin and still places
    // the pieces that fit, reporting the oversize piece's id as unplaced —
    // where the strict packer would have returned TooLarge and placed nothing.
    #[test]
    fn lenient_skips_too_large_keeps_rest() {
        // id 0 is 40x40 — too large for the 16x16 bin; ids 1 and 2 fit.
        let rects = [Rect::new(40, 40), Rect::new(8, 4), Rect::new(4, 4)];

        // Strict packer aborts on the oversize piece.
        let strict = pack_maxrects(16, 16, 0, &rects, &[0]);
        assert!(matches!(strict, Err(PackError::TooLarge { id: 0, .. })));

        // Lenient packer places the two that fit and reports id 0 unplaced.
        let (placed, _free, unplaced) =
            pack_maxrects_multi_angle_lenient(16, 16, 0, &rects, &[0], None);
        assert_eq!(unplaced, vec![0]);
        assert_eq!(placed.len(), 2);
        assert!(placed.iter().all(|p| p.id != 0));
        assert!(validate_placements(16, 16, &placed));
    } // lenient_skips_too_large_keeps_rest

    // @brief Lenient pack reports pieces that fit individually but run out of
    // room, while keeping the pieces it did manage to place.
    #[test]
    fn lenient_skips_when_out_of_space() {
        // Three 8x6 rects, bin only tall enough for two rows of 6 (h=12).
        let rects = [Rect::new(8, 6), Rect::new(8, 6), Rect::new(8, 6)];

        // Strict: NoSpace on the third piece, nothing returned.
        let strict = pack_maxrects(8, 12, 0, &rects, &[0]);
        assert!(matches!(strict, Err(PackError::NoSpace { .. })));

        // Lenient: two placed, exactly one unplaced.
        let (placed, _free, unplaced) =
            pack_maxrects_multi_angle_lenient(8, 12, 0, &rects, &[0], None);
        assert_eq!(placed.len(), 2);
        assert_eq!(unplaced.len(), 1);
        assert!(validate_placements(8, 12, &placed));
    } // lenient_skips_when_out_of_space

    // @brief When nothing fits, lenient returns zero placements and every id as
    // unplaced (sorted) — still no error.
    #[test]
    fn lenient_all_unplaced_when_none_fit() {
        let rects = [Rect::new(40, 40), Rect::new(50, 50)];
        let (placed, _free, unplaced) =
            pack_maxrects_multi_angle_lenient(16, 16, 0, &rects, &[0], None);
        assert!(placed.is_empty());
        assert_eq!(unplaced, vec![0, 1]);
    } // lenient_all_unplaced_when_none_fit

    // @brief More pieces than the bin holds: the bin grows and every piece is placed.
    // When every piece fits, the bin keeps its height.
    #[test]
    fn extending_places_every_piece_and_grows_height() {
        // A 8x12 bin with a 1 px gap holds one 8x6 piece; six need 41 px.
        let rects = [Rect::new(8, 6); 6];
        let (placed, _free, unplaced, h) =
            pack_maxrects_multi_angle_extending(8, 12, 1, &rects, &[0], None);
        assert!(unplaced.is_empty(), "unplaced: {unplaced:?}");
        assert_eq!(placed.len(), 6);
        assert!(h >= 41, "bin height grows to hold six pieces, got {h}");
        assert!(validate_placements(8, h, &placed));

        let fits = [Rect::new(8, 4), Rect::new(4, 4)];
        let (placed, _free, unplaced, h) =
            pack_maxrects_multi_angle_extending(16, 16, 0, &fits, &[0], None);
        assert_eq!((placed.len(), unplaced.len(), h), (2, 0, 16));
    } // extending_places_every_piece_and_grows_height

    // @brief A piece wider than the bin at every trial angle stays unplaced;
    // the other pieces are placed in the grown bin.
    #[test]
    fn extending_leaves_too_wide_piece_out() {
        let rects = [Rect::new(20, 4), Rect::new(8, 6), Rect::new(8, 6), Rect::new(8, 6)];
        let (placed, _free, unplaced, h) =
            pack_maxrects_multi_angle_extending(10, 6, 0, &rects, &[0, 180], None);
        assert_eq!(unplaced, vec![0]);
        assert_eq!(placed.len(), 3);
        assert!(validate_placements(10, h, &placed));
    } // extending_leaves_too_wide_piece_out

    // @brief A free-rotation piece too wide upright and too tall when turned is
    // placed turned once the bin grows.
    #[test]
    fn extending_turns_free_rotation_piece() {
        let rects = [Rect::new(20, 8).with_free_rotation(true)];
        let (placed, _free, unplaced, h) =
            pack_maxrects_multi_angle_extending(10, 12, 0, &rects, &[0, 180], None);
        assert!(unplaced.is_empty());
        assert!(placed[0].rotation_deg == 90 || placed[0].rotation_deg == 270);
        assert!(h >= 20, "bin height grows to the turned piece, got {h}");
    } // extending_turns_free_rotation_piece

    // @brief The required width is the widest piece in its narrowest trial angle.
    #[test]
    fn required_bin_width_uses_narrowest_angle() {
        let rects = [Rect::new(8, 4), Rect::new(20, 6)];
        assert_eq!(required_bin_width(&rects, &[0, 180]), 20);
        // A free-rotation piece may turn: 20x6 needs only 6.
        let free = [Rect::new(8, 4), Rect::new(20, 6).with_free_rotation(true)];
        assert_eq!(required_bin_width(&free, &[0, 180]), 8);
        assert_eq!(required_bin_width(&[], &[0]), 0);

        // With the bin widened to it, every piece is placed.
        let w = required_bin_width(&rects, &[0]);
        let (placed, _free, unplaced, h) =
            pack_maxrects_multi_angle_extending(w, 5, 0, &rects, &[0], None);
        assert!(unplaced.is_empty());
        assert!(validate_placements(w, h, &placed));
    } // required_bin_width_uses_narrowest_angle

    // @brief A piece fits when one trial angle fits both sides of the bin.
    #[test]
    fn fits_bin_checks_each_trial_angle() {
        let tall = Rect::new(4, 20);
        assert!(!fits_bin(tall, 30, 10, &[0, 180]));
        assert!(fits_bin(tall.with_free_rotation(true), 30, 10, &[0, 180]));
        assert!(fits_bin(tall, 30, 10, &[0, 90, 180, 270]));
        assert!(fits_bin(tall, 4, 20, &[0]));
    } // fits_bin_checks_each_trial_angle
    // @brief Smallest clearance between any two placements; u32::MAX for fewer than two.
    fn min_clearance(placed: &[Placed]) -> u32 {
        let mut least = u32::MAX;
        for (i, a) in placed.iter().enumerate() {
            for b in &placed[i + 1..] {
                // Clearance is the larger of the x and y distances between the boxes.
                let dx = b.x.saturating_sub(a.x + a.w).max(a.x.saturating_sub(b.x + b.w));
                let dy = b.y.saturating_sub(a.y + a.h).max(a.y.saturating_sub(b.y + b.h));
                least = least.min(dx.max(dy));
            } // for b
        } // for a
        least
    } // fn min_clearance

    // @brief The 26 Front pieces of the jacket2 Lining marker, sizes 220 to 720.
    //
    // Their heights differ by a few px.  The topmost-free-box rule put a piece
    // 2 px higher but 1,732 px further right, packed 8 bands in 16,944 px and
    // left holes up to 1,014 px wide between side-by-side pieces.  The pieces
    // fit in 7 bands, with only the piece gap between side-by-side pieces.
    #[test]
    fn extending_marker_keeps_side_by_side_pieces_at_the_gap() {
        let sizes: [(u32, u32); 26] = [
            (718, 2096), (754, 2100), (790, 2101), (825, 2102), (861, 2103), (897, 2105),
            (933, 2106), (969, 2107), (1004, 2108), (1040, 2109), (1076, 2110), (1112, 2111),
            (1148, 2112), (1184, 2114), (1220, 2115), (1255, 2134), (1291, 2116), (1327, 2117),
            (1363, 2118), (1399, 2119), (1435, 2120), (1471, 2121), (1507, 2122), (1542, 2123),
            (1578, 2124), (1614, 2125),
        ];
        let rects: Vec<Rect> = sizes.iter().map(|&(w, h)| Rect::new(w, h)).collect();
        let (placed, _free, unplaced, _h) =
            pack_maxrects_multi_angle_extending(4560, 4560, 5, &rects, &[0, 180], None);
        assert!(unplaced.is_empty(), "unplaced: {unplaced:?}");
        assert_eq!(placed.len(), 26);

        let content_h = placed.iter().map(|p| p.y + p.h).max().unwrap_or(0);
        assert!(content_h <= 7 * 2134 + 6 * 5, "7 bands of pieces, got {content_h} px");
        assert_eq!(maxrects_order_search::side_gap_excess(&placed, 5), 0, "{placed:?}");
        assert!(min_clearance(&placed) >= 5, "{placed:?}");
        assert!(placed.iter().all(|p| p.x + p.w <= 4560), "{placed:?}");
    } // extending_marker_keeps_side_by_side_pieces_at_the_gap

    // @brief The strict packer runs the start orders only, so callers can call it many times.
    #[test]
    fn strict_packer_skips_the_order_search() {
        let rects: Vec<Rect> = (1..=12).map(|i| Rect::new(10 + i, 20 - i)).collect();
        let mut total_steps = 0usize;
        let mut on_progress = |_done: usize, total: usize| total_steps = total;
        pack_maxrects_multi_angle(200, 200, 2, &rects, &[0, 180], Some(&mut on_progress)).expect("pack ok");
        assert!((1..=10).contains(&total_steps), "start orders and rules only, got {total_steps}");
    } // strict_packer_skips_the_order_search
}
