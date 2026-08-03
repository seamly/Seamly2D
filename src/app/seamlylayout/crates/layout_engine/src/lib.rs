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

pub use pack_types::{FreeRect, PackError, PackResult, Placed, Rect};

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
// Maintains a list of maximal free rectangles (free rects may overlap).
// Pieces are sorted by area descending (largest first) and each is placed at
// the top-left corner of the best-fit free rect (smallest area that fits).
//
// After each placement, every free rect that overlaps the placed piece is
// removed and replaced with up to four non-overlapping sub-rects (left strip,
// right strip, top strip, bottom strip).  The `gap_px` parameter inserts that
// many pixels of clearance around each placed piece when computing splits.
// Finally, any free rect fully contained within another is pruned.
//
// Split geometry (piece at px, py with size pw × ph, gap g):
//   left   = (F.x,          F.y,          px         - F.x,              F.h)
//   right  = (px + pw + g,  F.y,          F.x + F.w  - (px + pw + g),    F.h)
//   top    = (F.x,          F.y,          F.w,        py - F.y               )
//   bottom = (F.x,          py + ph + g,  F.w,        F.y + F.h - (py + ph + g))
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
// For each piece and each candidate free rect, this function evaluates all
// trial angles by computing the rotated AABB dimensions and selecting the best
// top-left candidate (lowest y, then lowest x, then lowest waste area).  This
// preserves MaxRects speed while supporting 90°/45° trial sets without the
// expensive polygon NFP path.
pub fn pack_maxrects_multi_angle(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    rects: &[Rect],
    trial_angles_deg: &[u16],
    on_piece_begin: Option<&mut dyn FnMut(usize, usize)>,
) -> PackResult<(Vec<Placed>, Vec<FreeRect>)> {
    // Strict contract (unchanged): the first piece that can't be placed aborts
    // the whole pack with the matching PackError.  Implemented on top of the
    // shared collect worker so the strict and lenient entry points stay in sync.
    // `unplaced` is in area-descending iteration order, so `unplaced[0]` is the
    // exact piece the historical early-return version reported.
    let (placements, free_rects, unplaced) =
        pack_maxrects_collect(bin_w, bin_h, gap_px, rects, trial_angles_deg, on_piece_begin);
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
// @return (placements sorted by original id, free-rect creation history,
//          unplaced original ids sorted ascending).
pub fn pack_maxrects_multi_angle_lenient(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    rects: &[Rect],
    trial_angles_deg: &[u16],
    on_piece_begin: Option<&mut dyn FnMut(usize, usize)>,
) -> (Vec<Placed>, Vec<FreeRect>, Vec<usize>) {
    let (placements, free_rects, unplaced) =
        pack_maxrects_collect(bin_w, bin_h, gap_px, rects, trial_angles_deg, on_piece_begin);
    let mut unplaced_ids: Vec<usize> = unplaced.into_iter().map(|u| u.id).collect();
    unplaced_ids.sort_unstable();
    (placements, free_rects, unplaced_ids)
} // fn pack_maxrects_multi_angle_lenient

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
// adapters over this.  Skipping an unplaceable piece leaves the free-rect set
// untouched, so smaller pieces later in area order can still fill the space.
fn pack_maxrects_collect(
    bin_w: u32,
    bin_h: u32,
    gap_px: u32,
    rects: &[Rect],
    trial_angles_deg: &[u16],
    mut on_piece_begin: Option<&mut dyn FnMut(usize, usize)>,
) -> (Vec<Placed>, Vec<FreeRect>, Vec<UnplacedPiece>) {
    debug!(
        "[debug] layout_engine\\src\\pack_maxrects_collect(): 1 Called with bin_w={}, bin_h={}, gap_px={}, rect_count={}, trial_angles_deg={:?}",
        bin_w,
        bin_h,
        gap_px,
        rects.len(),
        trial_angles_deg
    );

    let trial: &[u16] = if trial_angles_deg.is_empty() { &[0] } else { trial_angles_deg };

    // Sort by area descending, preserving original indices.
    let mut order: Vec<usize> = (0..rects.len()).collect();
    order.sort_by_key(|&i| std::cmp::Reverse(rects[i].w as u64 * rects[i].h as u64));

    let rect1 = FreeRect { x: 0, y: 0, w: bin_w, h: bin_h };
    let mut free_rects: Vec<FreeRect> = vec![rect1];
    let mut all_created: Vec<FreeRect> = vec![rect1];
    let mut placements: Vec<Placed> = Vec::with_capacity(rects.len());
    let mut unplaced: Vec<UnplacedPiece> = Vec::new();

    for (idx, &id) in order.iter().enumerate() {
        if let Some(cb) = on_piece_begin.as_mut() {
            cb(idx + 1, order.len());
        }

        let r = rects[id];

        // Any orientation physically fits in empty bin?
        let mut any_fit_in_empty = false;
        for &deg in trial {
            let (rw, rh) = rotated_rect_dims(r.w, r.h, deg);
            if rw <= bin_w && rh <= bin_h {
                any_fit_in_empty = true;
                break;
            }
        }
        if !any_fit_in_empty {
            // Physically larger than the bin at every orientation — skip and record.
            unplaced.push(UnplacedPiece {
                id,
                reason: UnplacedReason::TooLarge { w: r.w, h: r.h, bin_w, bin_h },
            });
            continue;
        }

        // Best candidate across all free rects and trial angles.
        // Key: topmost (y), then leftmost (x), then least waste in chosen free rect.
        let mut best: Option<(usize, u16, u32, u32, u64)> = None;
        for (fi, f) in free_rects.iter().enumerate() {
            for &deg in trial {
                let (rw, rh) = rotated_rect_dims(r.w, r.h, deg);
                if f.w < rw || f.h < rh {
                    continue;
                }
                let waste = (f.w as u64) * (f.h as u64) - (rw as u64) * (rh as u64);
                let cand_key = (f.y, f.x, waste);
                let is_better = match best {
                    None => true,
                    Some((bfi, _, brw, brh, bwaste)) => {
                        let bf = free_rects[bfi];
                        let best_key = (bf.y, bf.x, bwaste);
                        cand_key < best_key || (cand_key == best_key && ((rw as u64) * (rh as u64) > (brw as u64) * (brh as u64)))
                    }
                };
                if is_better {
                    best = Some((fi, deg, rw, rh, waste));
                }
            }
        }

        let (best_idx, best_deg, place_w, place_h, _) = match best {
            Some(v) => v,
            None => {
                // Fits an empty bin but no room remains now — skip and record.
                unplaced.push(UnplacedPiece { id, reason: UnplacedReason::NoSpace });
                continue;
            }
        };

        let chosen = free_rects[best_idx];
        let px = chosen.x;
        let py = chosen.y;
        placements.push(Placed {
            id,
            x: px,
            y: py,
            w: place_w,
            h: place_h,
            rotation_deg: best_deg,
        });

        let mut new_free: Vec<FreeRect> = Vec::new();
        let mut keep: Vec<bool> = vec![true; free_rects.len()];

        for (fi, &f) in free_rects.iter().enumerate() {
            if !f.overlaps_piece(px, py, place_w, place_h) {
                continue;
            }
            keep[fi] = false;

            if px > f.x {
                let sub = FreeRect { x: f.x, y: f.y, w: px - f.x, h: f.h };
                new_free.push(sub);
                all_created.push(sub);
            }

            let right_x = px.saturating_add(place_w).saturating_add(gap_px);
            if right_x < f.x + f.w {
                let sub = FreeRect { x: right_x, y: f.y, w: f.x + f.w - right_x, h: f.h };
                new_free.push(sub);
                all_created.push(sub);
            }

            if py > f.y {
                let sub = FreeRect { x: f.x, y: f.y, w: f.w, h: py - f.y };
                new_free.push(sub);
                all_created.push(sub);
            }

            let bottom_y = py.saturating_add(place_h).saturating_add(gap_px);
            if bottom_y < f.y + f.h {
                let sub = FreeRect { x: f.x, y: bottom_y, w: f.w, h: f.y + f.h - bottom_y };
                new_free.push(sub);
                all_created.push(sub);
            }
        }

        let mut retained: Vec<FreeRect> = free_rects
            .into_iter()
            .zip(keep)
            .filter_map(|(f, k)| if k { Some(f) } else { None })
            .collect();
        retained.extend(new_free);
        free_rects = retained;

        let n = free_rects.len();
        let mut dominated = vec![false; n];
        for i in 0..n {
            for j in 0..n {
                if i != j && !dominated[i] && free_rects[i].contained_in(free_rects[j]) {
                    dominated[i] = true;
                }
            }
        }
        free_rects = free_rects
            .into_iter()
            .zip(dominated)
            .filter_map(|(f, d)| if !d { Some(f) } else { None })
            .collect();
    }

    placements.sort_by_key(|p| p.id);
    (placements, all_created, unplaced)
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
}
