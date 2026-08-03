// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! Greedy NFP placer with Upper-Left-Fill placement and IFP container.
//!
//! ## Algorithm (Stage 1)
//!
//! 1. Sort pieces by descending area (largest first), matching the existing
//!    MaxRects ordering — pattern pieces vary widely in size and the largest
//!    pieces dominate layout shape, so placing them first is the standard
//!    heuristic.
//! 2. For each piece B in order, for each orientation θ in the trial set:
//!    a. Compute IFP(container, B_θ) — locus of B's reference point that
//!       keeps B inside the strip width and below the bin top.
//!    b. Compute NFP(A_i, B_θ) for every already-placed piece A_i.
//!    c. Feasible region for B's anchor = IFP \ ⋃ interior(NFP_i).
//!    d. On the feasible region's boundary (anchors lie on NFP edges, never
//!       inside — that's the definition of "touching but not overlapping"),
//!       pick the Upper-Left-Fill point: minimum y, ties broken by minimum x.
//! 3. Keep the lowest-y / lowest-x placement across orientations; record it.
//!
//! ## Why anchors live on the NFP boundary
//!
//! Inside an NFP means overlap with the corresponding placed piece, so the
//! interior is forbidden.  Outside means a non-touching gap, which wastes
//! material.  The boundary itself is exactly the touching-but-not-overlapping
//! locus — and the strict subset of that boundary that's also inside IFP is
//! the feasible region the UL-Fill rule walks.
//!
//! ## Container model
//!
//! Strip packing: width fixed (the roll), height grows downward as needed.
//! The current API still threads `bin_h` through from MaxRects-era callers;
//! the placer treats it as an upper bound and reports `NoSpace` on overflow,
//! mirroring `pack_maxrects` behavior.

use i_overlay::core::fill_rule::FillRule;
use i_overlay::core::overlay::{Overlay, ShapeType};
use i_overlay::core::overlay_rule::OverlayRule;
use i_overlay::i_float::int::point::IntPoint;
use pack_types::{FreeRect, PackError, PackResult, Placed, Rect};
use std::collections::{HashMap, HashSet};

use crate::geom::{aabb, from_int, obb_for_piece, obb_overlap, rotate, to_int, IntPolygon, Orientation};
use crate::nfp::NfpCache;

// Hard caps for exact refinement complexity.
//
// The exact path computes one NFP per (already_placed_piece, current_piece,
// orientation) pair and later runs a boolean overlay.  For high-vertex pieces,
// this can explode into pathological runtimes.  The caps below force a
// graceful degrade: skip exact refinement for that orientation and continue
// searching fast OBB-feasible candidates.
// NOTE: 1,000 / 4,000 proved too strict for real-world garments (e.g. front
// pocket pieces against 100+ vertex bodies), producing false NoSpace outcomes
// even on very tall 36in × 500in media.  These relaxed caps still bound worst
// cases while allowing moderate-complexity exact checks to run.
const MAX_EXACT_PAIR_VERT_PRODUCT: usize = 10_000;
const MAX_EXACT_TOTAL_VERT_PRODUCT: usize = 120_000;
const MAX_PLACER_WALL_MS: u128 = 4_000;
const SPATIAL_BIN_PX: i32 = 512;
const MAX_IMPROVEMENT_MS: u128 = 300;

// @brief Internal placement record before conversion back to `Placed`.
//
// Kept in scaled integer space so subsequent NFP-translation arithmetic stays
// on the integer grid.  Fields are also re-used to translate cached NFPs
// (which the cache stores in shape-only form) into the bin's coordinate
// frame before each new feasibility test.
struct AnchorPlacement {
    piece_id: usize,
    orient: Orientation,
    anchor_x: i32,
    anchor_y: i32,
} // struct AnchorPlacement

// @brief Best-so-far candidate for the current piece while iterating its trial set.
//
// Tracks both the anchor (needed to seed the next piece's NFP-translation step
// after we commit) and the placed AABB top-left (the cross-orientation
// comparator key) so each per-orientation trial can compute its rank without
// re-deriving these from the polygon.
struct BestCandidate {
    orient: Orientation,
    anchor_x: i32,
    anchor_y: i32,
    placed_x: i32,
    placed_y: i32,
    placed_w: i32,
    placed_h: i32,
} // struct BestCandidate

// @brief Greedy Upper-Left-Fill placer over an NFP cache.
//
// Stage 1 entry point.  Today the body is unimplemented; the skeleton fixes
// the signature and the imports so `lib.rs::pack` can be redirected here in
// one diff once the geom/nfp modules are real.
//
// @param bin_w             Container width in pixels (strip width).
// @param bin_h             Container height in pixels (upper bound; NoSpace on overflow).
// @param gap_px            Minimum clearance between pieces; baked into each piece's
//                          outward offset during preprocessing.
// @param polygons          Per-piece outline polygons in scaled integer space,
//                          already gap-offset.  Index = piece id, matched against
//                          `rects` by position.
// @param rects             AABBs of the offset polygons; passed for the bin-fit
//                          early reject and for `Placed.w/h` reporting (the AABB
//                          is what the assembler renders into its piece-fill rect).
// @param trial_angles_deg  Per-piece rotation trial set; for each piece the placer
//                          tries every orientation and keeps the best UL-Fill point.
// @param cache             NFP cache; outlives a single `pack` call so multi-start
//                          (Stage 3) reuses entries across orderings.
//
// @return                  (placements, free-rect history) — same shape as
//                          `pack_maxrects` so the existing renderer / debug overlay
//                          consume polygon-tight output unchanged.  The free-rect
//                          history will be derived from a coarse AABB-of-placed-piece
//                          decomposition until the polygon-tight overlay lands.
#[allow(dead_code)]
pub(crate) fn place_upper_left_fill(
    bin_w: u32,
    bin_h: u32,
    _gap_px: u32,
    polygons: &[IntPolygon],
    rects: &[Rect],
    trial_angles_deg: &[u16],
    cache: &mut NfpCache,
    mut on_piece_begin: Option<&mut dyn FnMut(usize, usize)>,
) -> PackResult<(Vec<Placed>, Vec<FreeRect>)> {
    let t_start = std::time::Instant::now();
    debug_assert_eq!(
        polygons.len(), rects.len(),
        "place_upper_left_fill: polygons and rects must be index-aligned",
    );

    // Default trial set to [0°] when caller passes none — keeps the per-piece
    // inner loop non-empty without every caller having to construct a vec.
    // No upfront AABB-vs-bin early reject: with non-axis-aligned trial angles
    // a thin piece's rotated AABB can shrink below the un-rotated dims (a
    // 45° rotation puts both AABB sides at (w+h)/√2), so the per-orientation
    // `inner_fit` is the only sound feasibility test.  TooLarge is emitted
    // below when *no* orientation in the trial set produces a non-empty IFP.
    let trial: &[u16] = if trial_angles_deg.is_empty() { &[0] } else { trial_angles_deg };

    // Largest-first ordering.  Pattern pieces vary widely in size and the
    // largest pieces dominate the resulting layout shape; placing them first
    // is the standard heuristic and matches MaxRects' input ordering, so the
    // two packers' results stay comparable across the dispatcher boundary.
    let mut order: Vec<usize> = (0..polygons.len()).collect();
    order.sort_by(|&a, &b| {
        let area = |i: usize| (rects[i].w as u64) * (rects[i].h as u64);
        area(b).cmp(&area(a))
    });

    log::info!(
        "[placer] place_upper_left_fill start: pieces={}, trial_orientations={}, bin={}x{}",
        polygons.len(), trial.len(), bin_w, bin_h,
    );

    let mut placed: Vec<AnchorPlacement> = Vec::with_capacity(order.len());
    let mut placements: Vec<Placed> = Vec::with_capacity(order.len());

    // Aggregate timers — accumulated across all (piece, orientation) inner
    // iterations so the post-loop summary shows where the wall-clock went.
    // These are the four phases of the inner body: rotate, IFP, NFP collect
    // (cache lookup + translate), and the boolean Difference overlay that
    // produces the anchor candidates.
    let mut total_rotate_ms: u128 = 0;
    let mut total_ifp_ms:    u128 = 0;
    let mut total_nfp_ms:    u128 = 0;
    let mut total_overlay_ms: u128 = 0;
    let mut total_orientations_evaluated: u64 = 0;
    let mut total_anchor_vertices: u64 = 0;

    for (placement_idx, &id) in order.iter().enumerate() {
        if let Some(cb) = on_piece_begin.as_mut() {
            cb(placement_idx + 1, order.len());
        }

        if t_start.elapsed().as_millis() > MAX_PLACER_WALL_MS {
            log::warn!(
                "[placer] runtime budget exceeded before piece_id={} after {} ms (cap={} ms) — SearchLimit",
                id,
                t_start.elapsed().as_millis(),
                MAX_PLACER_WALL_MS,
            );
            return Err(PackError::SearchLimit { id });
        }

        let t_piece = std::time::Instant::now();
        log::debug!(
            "[placer] piece {}/{} begin: id={} ({}x{}px), placed_so_far={}, trial_set={:?}",
            placement_idx + 1,
            order.len(),
            id,
            rects[id].w,
            rects[id].h,
            placed.len(),
            trial,
        );
        let mut best: Option<BestCandidate> = None;
        // Broad-phase cache for already-placed pieces.  These OBBs are fixed
        // while evaluating all orientations of the current piece and are used
        // for quick SAT-based acceptance before invoking expensive NFP/overlay.
        let mut placed_obbs: Vec<_> = Vec::with_capacity(placed.len());
        let mut placed_aabbs_px: Vec<(i32, i32, i32, i32)> = Vec::with_capacity(placed.len());
        for p in &placed {
            placed_obbs.push(obb_for_piece(&polygons[p.piece_id], p.orient, p.anchor_x, p.anchor_y));
            let oriented_p = rotate(&polygons[p.piece_id], p.orient);
            let (min_x, min_y, max_x, max_y) = aabb(&oriented_p);
            let px_min = from_int(p.anchor_x + min_x).round() as i32;
            let py_min = from_int(p.anchor_y + min_y).round() as i32;
            let px_max = from_int(p.anchor_x + max_x).round() as i32;
            let py_max = from_int(p.anchor_y + max_y).round() as i32;
            placed_aabbs_px.push((px_min, py_min, px_max, py_max));
        }

        let mut spatial_bins: HashMap<(i32, i32), Vec<usize>> = HashMap::new();
        for (idx, (min_x, min_y, max_x, max_y)) in placed_aabbs_px.iter().copied().enumerate() {
            let bx0 = min_x.div_euclid(SPATIAL_BIN_PX);
            let by0 = min_y.div_euclid(SPATIAL_BIN_PX);
            let bx1 = max_x.div_euclid(SPATIAL_BIN_PX);
            let by1 = max_y.div_euclid(SPATIAL_BIN_PX);
            for bx in bx0..=bx1 {
                for by in by0..=by1 {
                    spatial_bins.entry((bx, by)).or_default().push(idx);
                }
            }
        }
        // Tracks whether the piece could fit *at all* in the empty bin at any
        // orientation — distinguishes the "physically too large" failure mode
        // (TooLarge) from the "no room left given prior placements" mode
        // (NoSpace).  Set the first time `inner_fit` returns a non-empty IFP.
        let mut any_orient_fits = false;
        // Tracks whether this piece's search was guardrail-limited (runtime or
        // exact-path complexity caps). If true and no feasible placement is
        // found, return SearchLimit instead of NoSpace so the UI can report a
        // distinct actionable message.
        let mut guardrail_limited = false;

        for &deg in trial {
            if t_start.elapsed().as_millis() > MAX_PLACER_WALL_MS {
                log::warn!(
                    "[placer] runtime budget exceeded mid-piece_id={} after {} ms (cap={} ms) — SearchLimit",
                    id,
                    t_start.elapsed().as_millis(),
                    MAX_PLACER_WALL_MS,
                );
                return Err(PackError::SearchLimit { id });
            }

            let orient = Orientation(deg);
            total_orientations_evaluated += 1;
            log::debug!(
                "[placer] piece_id={} orient={}° begin: placed_count={}",
                id,
                deg,
                placed.len(),
            );

            let t_rot = std::time::Instant::now();
            let oriented = rotate(&polygons[id], orient);
            total_rotate_ms += t_rot.elapsed().as_millis();

            // IFP: anchor positions that keep the rotated piece's AABB in [0, bin].
            let t_ifp = std::time::Instant::now();
            let ifp = inner_fit(bin_w, bin_h, &oriented);
            total_ifp_ms += t_ifp.elapsed().as_millis();
            if ifp.is_empty() {
                log::debug!(
                    "[placer] piece_id={} orient={}° skipped: inner_fit empty (bin={}x{})",
                    id,
                    deg,
                    bin_w,
                    bin_h,
                );
                continue; // doesn't fit at this orientation
            } // if no IFP
            any_orient_fits = true;

            // AABB of the rotated piece — shared by the fast path, the
            // guardrail fallback, and the exact path's anchor scoring below.
            // The placed-AABB frame (anchor + AABB min corner) is the
            // cross-orientation comparator: anchors at different orientations
            // live in different polygon frames, so scoring must happen on the
            // user-perceived placed top-left, not the raw anchor.
            let (min_x, min_y, max_x, max_y) = aabb(&oriented);

            // @brief Score `anchor` against the best-so-far candidate and
            // record it when it wins the UL-Fill comparator (lowest placed y,
            // ties broken by lowest placed x).  Used by every acceptance site
            // in this orientation loop so the comparator stays in one place.
            let consider_anchor = |best: &mut Option<BestCandidate>, anchor: IntPoint| {
                let placed_x = anchor.x + min_x;
                let placed_y = anchor.y + min_y;
                let beats = match best {
                    None => true,
                    Some(b) => placed_y < b.placed_y
                        || (placed_y == b.placed_y && placed_x < b.placed_x),
                };
                if beats {
                    *best = Some(BestCandidate {
                        orient,
                        anchor_x: anchor.x,
                        anchor_y: anchor.y,
                        placed_x,
                        placed_y,
                        placed_w: max_x - min_x,
                        placed_h: max_y - min_y,
                    });
                } // if beats
            }; // consider_anchor

            // @brief OBB broad-phase feasibility: true when the piece anchored
            // at `anchor` has no oriented-bounding-box overlap with any placed
            // piece.  Sound for acceptance (OBB separation implies polygon
            // separation) but conservative — it can reject anchors the exact
            // NFP path would accept, so a `false` here never rules a spot out.
            let sat_clear = |anchor: IntPoint| -> bool {
                // Pixel-space AABB of the candidate placement, for the
                // spatial-bin neighborhood lookup (avoids testing every
                // placed piece when many are far away).
                let cand_min_x_px = from_int(anchor.x + min_x).round() as i32;
                let cand_min_y_px = from_int(anchor.y + min_y).round() as i32;
                let cand_max_x_px = from_int(anchor.x + max_x).round() as i32;
                let cand_max_y_px = from_int(anchor.y + max_y).round() as i32;

                let bx0 = cand_min_x_px.div_euclid(SPATIAL_BIN_PX);
                let by0 = cand_min_y_px.div_euclid(SPATIAL_BIN_PX);
                let bx1 = cand_max_x_px.div_euclid(SPATIAL_BIN_PX);
                let by1 = cand_max_y_px.div_euclid(SPATIAL_BIN_PX);
                let mut neighbor_ids: HashSet<usize> = HashSet::new();
                for bx in bx0..=bx1 {
                    for by in by0..=by1 {
                        if let Some(ids) = spatial_bins.get(&(bx, by)) {
                            for &nid in ids {
                                neighbor_ids.insert(nid);
                            } // for nid
                        } // if bin occupied
                    } // for by
                } // for bx

                let cand = obb_for_piece(&polygons[id], orient, anchor.x, anchor.y);
                !neighbor_ids.iter().any(|nid| obb_overlap(&cand, &placed_obbs[*nid]))
            }; // sat_clear

            // Candidate corner anchors: the IFP's vertices in UL-lexicographic
            // order.  Shared by the optimal-head fast path just below and by
            // the guardrail fallback further down.
            let mut quick_vertices = ifp.clone();
            quick_vertices.sort_by(|a, b| {
                if a.y == b.y { a.x.cmp(&b.x) } else { a.y.cmp(&b.y) }
            });
            quick_vertices.dedup_by(|a, b| a.x == b.x && a.y == b.y);

            // Hybrid fast path (Phase 1+2): OBB broad-phase acceptance of the
            // provably-optimal anchor only.  `quick_vertices[0]` is the IFP's
            // lex-min corner — a lower bound on the UL-Fill comparator for
            // this orientation: every feasible anchor has y ≥ its y, and at
            // equal y, x ≥ its x.  If that corner is SAT-clear it IS this
            // orientation's optimum, so we accept it without paying the
            // NFP/overlay cost.  Any OTHER corner being clear proves nothing —
            // the exact NFP search can still find a better flush placement
            // along a placed piece's boundary — so we fall through to the
            // exact path instead of accepting it.  (Accepting the first
            // SAT-clear corner regardless of rank was a regression that
            // slammed every piece after the first against the bin's top-RIGHT
            // corner, since the top-left corner is usually occupied.)
            if !placed_obbs.is_empty() {
                if let Some(&head) = quick_vertices.first() {
                    if sat_clear(head) {
                        consider_anchor(&mut best, head);
                        log::debug!(
                            "[placer] piece_id={} orient={}° fast-path accepted lex-min IFP corner=({}, {}) via OBB SAT",
                            id,
                            deg,
                            head.x,
                            head.y,
                        );
                        continue;
                    } // if head clear
                } // if head exists
                log::debug!(
                    "[placer] piece_id={} orient={}° fast-path lex-min IFP corner blocked; falling back to exact NFP",
                    id,
                    deg,
                );
            }

            // Exact refinement guardrail: bypass pathological NFP workloads.
            // If the vertex-product estimate is above threshold, we skip this
            // orientation's exact path and let other orientations (or the next
            // piece-level NoSpace) decide.
            if !placed.is_empty() {
                let curr_verts = polygons[id].len();
                let mut total_vert_product: usize = 0;
                let mut worst_pair: Option<(usize, usize, usize)> = None;
                for p in &placed {
                    let placed_verts = polygons[p.piece_id].len();
                    let pair_prod = placed_verts.saturating_mul(curr_verts);
                    total_vert_product = total_vert_product.saturating_add(pair_prod);
                    if pair_prod > MAX_EXACT_PAIR_VERT_PRODUCT {
                        worst_pair = Some((p.piece_id, placed_verts, pair_prod));
                        break;
                    }
                }

                if let Some((pid, pverts, pair_prod)) = worst_pair {
                    guardrail_limited = true;
                    log::warn!(
                        "[placer] piece_id={} orient={}° exact NFP bypassed: pair complexity cap exceeded (placed_id={}, verts={}×{}={}, cap={})",
                        id,
                        deg,
                        pid,
                        pverts,
                        curr_verts,
                        pair_prod,
                        MAX_EXACT_PAIR_VERT_PRODUCT,
                    );
                    // Degraded fallback: the exact path is bypassed for this
                    // orientation, so accept the best (UL-lex-first) SAT-clear
                    // IFP corner if one exists rather than dropping the
                    // orientation entirely — a coarse corner placement beats a
                    // false NoSpace/SearchLimit on complex garments.
                    if let Some(&fb) = quick_vertices.iter().find(|&&a| sat_clear(a)) {
                        consider_anchor(&mut best, fb);
                        log::debug!(
                            "[placer] piece_id={} orient={}° guardrail fallback accepted IFP corner=({}, {})",
                            id, deg, fb.x, fb.y,
                        );
                    } // if fallback corner found
                    continue;
                }

                if total_vert_product > MAX_EXACT_TOTAL_VERT_PRODUCT {
                    guardrail_limited = true;
                    log::warn!(
                        "[placer] piece_id={} orient={}° exact NFP bypassed: total complexity cap exceeded (total_vert_product={}, cap={})",
                        id,
                        deg,
                        total_vert_product,
                        MAX_EXACT_TOTAL_VERT_PRODUCT,
                    );
                    // Same degraded fallback as the pair-cap branch above.
                    if let Some(&fb) = quick_vertices.iter().find(|&&a| sat_clear(a)) {
                        consider_anchor(&mut best, fb);
                        log::debug!(
                            "[placer] piece_id={} orient={}° guardrail fallback accepted IFP corner=({}, {})",
                            id, deg, fb.x, fb.y,
                        );
                    } // if fallback corner found
                    continue;
                }
            }

            // Forbidden region: union of NFPs against each placed piece,
            // each translated into the bin's frame by that placed piece's
            // anchor.  The cache stores shape-only NFPs (translation-
            // invariant); the per-placement translation happens here.
            let t_nfp = std::time::Instant::now();
            log::debug!(
                "[placer] piece_id={} orient={}° nfp_collect begin: against {} placed pieces",
                id,
                deg,
                placed.len(),
            );
            let mut nfps: Vec<IntPolygon> = Vec::with_capacity(placed.len());
            for (idx, p) in placed.iter().enumerate() {
                let t_pair = std::time::Instant::now();
                let pair_prod = polygons[p.piece_id].len().saturating_mul(polygons[id].len());
                log::debug!(
                    "[placer] piece_id={} orient={}° nfp_pair {}/{} begin: placed_id={} placed_orient={}° verts={}x{} (prod={})",
                    id,
                    deg,
                    idx + 1,
                    placed.len(),
                    p.piece_id,
                    p.orient.0,
                    polygons[p.piece_id].len(),
                    polygons[id].len(),
                    pair_prod,
                );
                let shape = cache.get(
                    polygons,
                    p.piece_id, p.orient,
                    id, orient,
                );
                let translated = translate_polygon(shape, p.anchor_x, p.anchor_y);
                log::debug!(
                    "[placer] piece_id={} orient={}° nfp_pair {}/{} done: nfp_verts={} translated_verts={} took {} ms",
                    id,
                    deg,
                    idx + 1,
                    placed.len(),
                    shape.len(),
                    translated.len(),
                    t_pair.elapsed().as_millis(),
                );
                nfps.push(translated);
            }
            let nfp_ms = t_nfp.elapsed().as_millis();
            total_nfp_ms += nfp_ms;
            log::debug!(
                "[placer] piece_id={} orient={}° nfp_collect done: nfps={}, total_nfp_verts={}, took {} ms",
                id,
                deg,
                nfps.len(),
                nfps.iter().map(|n| n.len()).sum::<usize>(),
                nfp_ms,
            );

            // Feasible region D = IFP \ union(NFPs).  Anchors live on D's
            // boundary — both the IFP edges (no NFP reaches there) and the
            // NFP edges where they cut into the IFP (touching-but-not-
            // overlapping placements).
            let t_overlay = std::time::Instant::now();
            log::debug!(
                "[placer] piece_id={} orient={}° anchor_candidates begin: ifp_verts={}, nfps={}",
                id,
                deg,
                ifp.len(),
                nfps.len(),
            );
            let candidates = anchor_candidates(&ifp, &nfps);
            let overlay_ms = t_overlay.elapsed().as_millis();
            total_overlay_ms += overlay_ms;
            let cand_verts: usize = candidates.iter().map(|c| c.len()).sum();
            total_anchor_vertices += cand_verts as u64;
            log::debug!(
                "[placer] piece_id={} orient={}° anchor_candidates done: contours={}, cand_verts={}, took {} ms",
                id,
                deg,
                candidates.len(),
                cand_verts,
                overlay_ms,
            );
            // Surface individually slow overlays — these dominate runtime
            // when polygons get complex.  Threshold matches the NFP-compute
            // threshold for log consistency.
            if overlay_ms >= 50 {
                log::debug!(
                    "[placer] slow overlay: piece_id={} orient={}° ifp_verts={} nfps={} (sum_verts={}) cand_verts={} took {} ms",
                    id, deg, ifp.len(), nfps.len(),
                    nfps.iter().map(|n| n.len()).sum::<usize>(),
                    cand_verts, overlay_ms,
                );
            } // if slow

            let Some(anchor) = lex_min_vertex(&candidates) else {
                log::debug!(
                    "[placer] piece_id={} orient={}° rejected: no lex-min anchor (empty feasible boundary)",
                    id,
                    deg,
                );
                continue; // no feasible anchor at this orientation
            }; // let-else lex_min

            // Score on placed-AABB top-left, not raw anchor (see the
            // `consider_anchor` helper above): the user-perceived
            // "upper-left" is the placed AABB corner, which translates by
            // the rotated polygon's `min` AABB corner.
            consider_anchor(&mut best, anchor);
        } // for orient

        let Some(b) = best else {
            // Distinguish the two failure modes: piece couldn't fit in the
            // empty bin at any orientation → TooLarge; piece could have fit
            // but every orientation's feasible region was empty under the
            // current set of placed pieces → NoSpace.
            if !any_orient_fits {
                log::warn!(
                    "[placer] TooLarge: piece_id={} ({}x{}px) does not fit in {}x{}px bin at any orientation",
                    id, rects[id].w, rects[id].h, bin_w, bin_h,
                );
                return Err(PackError::TooLarge {
                    id, w: rects[id].w, h: rects[id].h, bin_w, bin_h,
                });
            } // if too large
            if guardrail_limited {
                log::warn!(
                    "[placer] SearchLimit: piece_id={} ({}x{}px) — no feasible placement found before guardrails terminated exact search",
                    id, rects[id].w, rects[id].h,
                );
                return Err(PackError::SearchLimit { id });
            } // if guardrail limited
            log::warn!(
                "[placer] NoSpace: piece_id={} ({}x{}px) — every orientation's feasible region empty after {} placed",
                id, rects[id].w, rects[id].h, placed.len(),
            );
            return Err(PackError::NoSpace { id });
        }; // let-else best

        log::debug!(
            "[placer] piece {}/{} placed: id={} ({}x{}px) -> orient={}° at ({},{}) px in {} ms ({} placed so far)",
            placement_idx + 1, order.len(), id,
            rects[id].w, rects[id].h, b.orient.0,
            from_int(b.placed_x).round() as u32, from_int(b.placed_y).round() as u32,
            t_piece.elapsed().as_millis(),
            placed.len() + 1,
        );

        placements.push(Placed {
            id,
            x: from_int(b.placed_x).round() as u32,
            y: from_int(b.placed_y).round() as u32,
            w: from_int(b.placed_w).round() as u32,
            h: from_int(b.placed_h).round() as u32,
            rotation_deg: b.orient.0,
        });
        placed.push(AnchorPlacement {
            piece_id: id,
            orient: b.orient,
            anchor_x: b.anchor_x,
            anchor_y: b.anchor_y,
        });
    } // for piece

    // Fixed-time local quality improvement pass (rotate/reinsert-lite):
    // keep anchors fixed, try alternate orientations that improve UL score
    // while staying inside bin and non-overlapping by OBB SAT.
    let t_improve = std::time::Instant::now();
    let mut improve_passes = 0_u32;
    let mut improve_applied = 0_u32;
    while t_improve.elapsed().as_millis() < MAX_IMPROVEMENT_MS {
        improve_passes += 1;
        let mut changed_this_pass = false;
        for idx in 0..placed.len() {
            if t_improve.elapsed().as_millis() >= MAX_IMPROVEMENT_MS {
                break;
            }

            let cur = &placed[idx];
            let cur_poly = rotate(&polygons[cur.piece_id], cur.orient);
            let (cmin_x, cmin_y, _cmax_x, _cmax_y) = aabb(&cur_poly);
            let cur_px = cur.anchor_x + cmin_x;
            let cur_py = cur.anchor_y + cmin_y;

            for &deg in trial {
                let cand_orient = Orientation(deg);
                if cand_orient == cur.orient {
                    continue;
                }

                let cand_poly = rotate(&polygons[cur.piece_id], cand_orient);
                let ifp = inner_fit(bin_w, bin_h, &cand_poly);
                if ifp.is_empty() {
                    continue;
                }
                let (lo_x, lo_y, hi_x, hi_y) = {
                    let xs = [ifp[0].x, ifp[1].x, ifp[2].x, ifp[3].x];
                    let ys = [ifp[0].y, ifp[1].y, ifp[2].y, ifp[3].y];
                    (
                        *xs.iter().min().unwrap_or(&ifp[0].x),
                        *ys.iter().min().unwrap_or(&ifp[0].y),
                        *xs.iter().max().unwrap_or(&ifp[0].x),
                        *ys.iter().max().unwrap_or(&ifp[0].y),
                    )
                };
                if cur.anchor_x < lo_x || cur.anchor_x > hi_x || cur.anchor_y < lo_y || cur.anchor_y > hi_y {
                    continue;
                }

                let cand_obb = obb_for_piece(&polygons[cur.piece_id], cand_orient, cur.anchor_x, cur.anchor_y);
                let mut overlap = false;
                for (j, p) in placed.iter().enumerate() {
                    if j == idx {
                        continue;
                    }
                    let p_obb = obb_for_piece(&polygons[p.piece_id], p.orient, p.anchor_x, p.anchor_y);
                    if obb_overlap(&cand_obb, &p_obb) {
                        overlap = true;
                        break;
                    }
                }
                if overlap {
                    continue;
                }

                let (nmin_x, nmin_y, nmax_x, nmax_y) = aabb(&cand_poly);
                let new_px = cur.anchor_x + nmin_x;
                let new_py = cur.anchor_y + nmin_y;
                let better = new_py < cur_py || (new_py == cur_py && new_px < cur_px);
                if !better {
                    continue;
                }

                placed[idx].orient = cand_orient;
                placements[idx].x = from_int(new_px).round() as u32;
                placements[idx].y = from_int(new_py).round() as u32;
                placements[idx].w = from_int(nmax_x - nmin_x).round() as u32;
                placements[idx].h = from_int(nmax_y - nmin_y).round() as u32;
                placements[idx].rotation_deg = cand_orient.0;
                improve_applied += 1;
                changed_this_pass = true;
                break;
            }
        }
        if !changed_this_pass {
            break;
        }
    }

    log::info!(
        "[placer] quality loop: passes={}, applied={}, elapsed={} ms (budget={} ms)",
        improve_passes,
        improve_applied,
        t_improve.elapsed().as_millis(),
        MAX_IMPROVEMENT_MS,
    );

    log::info!(
        "[placer] phase totals: rotate={}ms, ifp={}ms, nfp_collect={}ms, overlay={}ms, orient_evals={}, anchor_verts_total={}",
        total_rotate_ms, total_ifp_ms, total_nfp_ms, total_overlay_ms,
        total_orientations_evaluated, total_anchor_vertices,
    );

    // Restore input-index order so the result is independent of the
    // area-descending sort applied internally.
    placements.sort_by_key(|p| p.id);

    // Stage 1: free-rect history is empty.  The renderer's debug overlay
    // simply shows no free rects for polygon-tight layouts; a coarse
    // AABB-of-placed-piece decomposition can populate this in a later phase.
    Ok((placements, Vec::new()))
} // fn place_upper_left_fill

// @brief Vertex-wise translation of a polygon by `(dx, dy)`.
//
// Used to lift a cached (shape-only) NFP into the bin's coordinate frame —
// the cached shape is centered on the placed piece's reference origin, so
// translating by that piece's anchor positions the forbidden region where
// it actually sits in the bin.
fn translate_polygon(poly: &IntPolygon, dx: i32, dy: i32) -> IntPolygon {
    poly.iter()
        .map(|p| IntPoint { x: p.x + dx, y: p.y + dy })
        .collect()
} // fn translate_polygon

// @brief Compute the boundary contours of the feasible-anchor region.
//
// Feasible region D = IFP \ union(NFPs).  The candidate anchors live on D's
// boundary; the lex-min comparator (next helper) consumes the contours
// returned here.  Each contour's vertex set already includes both the
// IFP-edge candidates (where no NFP cuts in) and the NFP-edge candidates
// (where an NFP boundary slices through the IFP), so the caller can iterate
// vertices uniformly.
//
// Implementation: i_overlay's `Difference` overlay with `FillRule::Positive`.
// Multiple NFPs added under `ShapeType::Clip` form a unioned clip region
// (overlapping CCW contours all contribute positive winding, so any region
// covered by ≥1 contour is "filled" under the positive rule and therefore
// removed from the subject).
fn anchor_candidates(ifp: &IntPolygon, nfps: &[IntPolygon]) -> Vec<IntPolygon> {
    log::debug!(
        "[placer::anchor_candidates] entry: ifp_verts={}, nfps={}, total_nfp_verts={}",
        ifp.len(),
        nfps.len(),
        nfps.iter().map(|n| n.len()).sum::<usize>(),
    );
    if ifp.is_empty() {
        log::debug!("[placer::anchor_candidates] exit: empty IFP -> 0 contours");
        return Vec::new();
    } // if empty IFP

    if nfps.iter().all(|n| n.is_empty()) {
        // No placed pieces (or all NFPs degenerate) — the feasible region
        // is the whole IFP.  Skip the overlay round-trip; return the IFP
        // outline as the single candidate contour.
        log::debug!("[placer::anchor_candidates] exit: no clip contours -> 1 contour (IFP)");
        return vec![ifp.clone()];
    } // if no real clips

    let total_pts = ifp.len() + nfps.iter().map(|n| n.len()).sum::<usize>();
    let mut overlay = Overlay::new(total_pts);
    overlay.add_contour(ifp, ShapeType::Subject);
    for nfp in nfps {
        if !nfp.is_empty() {
            overlay.add_contour(nfp, ShapeType::Clip);
        } // if non-empty
    } // for clip

    log::debug!(
        "[placer::anchor_candidates] overlay begin: total_pts={}, clip_contours={}",
        total_pts,
        nfps.iter().filter(|n| !n.is_empty()).count(),
    );
    let shapes = overlay.overlay(OverlayRule::Difference, FillRule::Positive);
    let shape_count = shapes.len();
    let contour_count: usize = shapes.iter().map(|s| s.len()).sum();

    // Flatten: contour 0 of each shape is the outer boundary, the rest are
    // hole boundaries.  Anchors on hole boundaries are valid feasible
    // points (the hole interior is forbidden, the boundary is touching),
    // so iterate them all.
    let out: Vec<IntPolygon> = shapes
        .into_iter()
        .flat_map(|s| s.into_iter())
        .filter(|c| !c.is_empty())
        .collect();
    log::debug!(
        "[placer::anchor_candidates] overlay done: shapes={}, contours={}, out_contours={}, out_verts={}",
        shape_count,
        contour_count,
        out.len(),
        out.iter().map(|c| c.len()).sum::<usize>(),
    );
    out
} // fn anchor_candidates

// @brief Lexicographic minimum (lowest y, ties broken by lowest x) over the
// vertex sets of the given contours.
//
// `None` ⇒ the input was empty (no contours, or every contour was empty).
// The lex-min over an integer-coord polygon's interior is always attained at
// a vertex (linear interpolation along an edge can't beat its endpoints on
// an axis-aligned comparator), so iterating only vertices is sound.
fn lex_min_vertex(contours: &[IntPolygon]) -> Option<IntPoint> {
    let mut best: Option<IntPoint> = None;
    for c in contours {
        for &p in c {
            let beats = match best {
                None => true,
                Some(b) => p.y < b.y || (p.y == b.y && p.x < b.x),
            };
            if beats {
                best = Some(p);
            } // if beats
        } // for vertex
    } // for contour
    best
} // fn lex_min_vertex

// @brief Inner-Fit Polygon: locus of B's reference point that keeps B fully
// inside the container.
//
// For an axis-aligned rectangular container of size `bin_w × bin_h` and a
// polygon B with AABB `(min_x, min_y, max_x, max_y)`, the IFP is the
// rectangle `[−min_x, bin_w − max_x] × [−min_y, bin_h − max_y]` — i.e. the
// container shrunk by B's AABB on each side.  This is the only IFP shape we
// need today; complex container shapes are out of scope.
//
// Returned in scaled integer space, same units as the NFP polygons.  An empty
// `Vec` signals "container can't hold the piece at this orientation" — the
// placer's per-orientation loop treats that as infeasible and moves on.
#[allow(dead_code)]
pub(crate) fn inner_fit(
    bin_w: u32,
    bin_h: u32,
    piece: &IntPolygon,
) -> IntPolygon {
    if piece.is_empty() {
        return Vec::new();
    } // if empty piece

    let (min_x, min_y, max_x, max_y) = aabb(piece);

    // bin_w / bin_h arrive in user-space pixels; lift into the same scaled
    // integer space the piece polygon already lives in so the subtractions
    // below stay on the integer grid.
    let bin_w_int = to_int(bin_w as f64);
    let bin_h_int = to_int(bin_h as f64);

    // Anchor must keep the piece's translated AABB inside [0..bin] on each
    // axis: ax + min_x ≥ 0  and  ax + max_x ≤ bin_w (and likewise for y).
    let lo_x = -min_x;
    let hi_x = bin_w_int - max_x;
    let lo_y = -min_y;
    let hi_y = bin_h_int - max_y;

    // Piece's AABB is wider or taller than the container — IFP is empty.
    if hi_x < lo_x || hi_y < lo_y {
        return Vec::new();
    } // if doesn't fit

    // Math-CCW corner order (positive shoelace), matching the orientation
    // convention the triangulator and Minkowski-sum pipeline assume.
    vec![
        IntPoint { x: lo_x, y: lo_y },
        IntPoint { x: hi_x, y: lo_y },
        IntPoint { x: hi_x, y: hi_y },
        IntPoint { x: lo_x, y: hi_y },
    ]
} // fn inner_fit

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // Construct an IntPoint at scaled-int coords; matches the existing nfp
    // tests' helper so test bodies stay readable.
    fn ip(x: i32, y: i32) -> IntPoint {
        IntPoint { x, y }
    }

    // @brief IFP for a piece comfortably smaller than the container has the
    // expected four corners and math-CCW orientation.
    #[test]
    fn inner_fit_basic_rectangle() {
        // Piece: 10×20 box at scaled-int origin.  bin: 100×80 user pixels →
        // 1_000_000 × 800_000 in scaled-int.
        let piece: IntPolygon = vec![ip(0, 0), ip(10, 0), ip(10, 20), ip(0, 20)];
        let ifp = inner_fit(100, 80, &piece);

        // Anchor range: x ∈ [0, 1_000_000 − 10], y ∈ [0, 800_000 − 20].
        let bin_w_int = to_int(100.0);
        let bin_h_int = to_int(80.0);
        assert_eq!(ifp.len(), 4);
        assert_eq!(ifp[0], ip(0, 0));
        assert_eq!(ifp[1], ip(bin_w_int - 10, 0));
        assert_eq!(ifp[2], ip(bin_w_int - 10, bin_h_int - 20));
        assert_eq!(ifp[3], ip(0, bin_h_int - 20));
    } // inner_fit_basic_rectangle

    // @brief Piece with vertices straddling the origin gets an IFP whose lower
    // corner is the negation of the piece's min-corner — i.e. the anchor must
    // shift the piece into the container.
    #[test]
    fn inner_fit_piece_offset_from_origin() {
        // Piece AABB (-3, -7, 12, 5).
        let piece: IntPolygon = vec![ip(-3, -7), ip(12, -7), ip(12, 5), ip(-3, 5)];
        let ifp = inner_fit(50, 40, &piece);

        let bin_w_int = to_int(50.0);
        let bin_h_int = to_int(40.0);
        assert_eq!(ifp.len(), 4);
        assert_eq!(ifp[0], ip(3, 7));
        assert_eq!(ifp[1], ip(bin_w_int - 12, 7));
        assert_eq!(ifp[2], ip(bin_w_int - 12, bin_h_int - 5));
        assert_eq!(ifp[3], ip(3, bin_h_int - 5));
    } // inner_fit_piece_offset_from_origin

    // @brief Piece wider than the container (in scaled-int space) produces an
    // empty IFP — the placer treats that as "no feasible placement" for this
    // orientation.
    #[test]
    fn inner_fit_piece_too_wide_returns_empty() {
        // bin_w = 1 user pixel = 10_000 scaled units.  Piece is 20_000 wide.
        let piece: IntPolygon = vec![ip(0, 0), ip(20_000, 0), ip(20_000, 5), ip(0, 5)];
        let ifp = inner_fit(1, 100, &piece);
        assert!(ifp.is_empty());
    } // inner_fit_piece_too_wide_returns_empty

    // @brief Piece exactly the container size produces a single-point IFP
    // (degenerate but non-empty rectangle with zero area).
    #[test]
    fn inner_fit_piece_exactly_fits() {
        let bin_w_int = to_int(10.0);
        let bin_h_int = to_int(5.0);
        let piece: IntPolygon = vec![
            ip(0, 0), ip(bin_w_int, 0), ip(bin_w_int, bin_h_int), ip(0, bin_h_int),
        ];
        let ifp = inner_fit(10, 5, &piece);
        // hi_x == lo_x == 0 and hi_y == lo_y == 0 — all four corners coincide.
        assert_eq!(ifp.len(), 4);
        for v in &ifp {
            assert_eq!(*v, ip(0, 0));
        }
    } // inner_fit_piece_exactly_fits

    // @brief Empty input polygon returns an empty IFP without panicking — the
    // placer can short-circuit on degenerate pieces without a special case.
    #[test]
    fn inner_fit_empty_piece_returns_empty() {
        let piece: IntPolygon = Vec::new();
        let ifp = inner_fit(10, 10, &piece);
        assert!(ifp.is_empty());
    } // inner_fit_empty_piece_returns_empty

    // @brief Construct an `IntPoint` from user-space pixel coords.  Polygons
    // in tests are written in user-space units (matches the bin dimensions);
    // this helper handles the SCALE conversion uniformly.
    fn upx(x: f64, y: f64) -> IntPoint {
        IntPoint { x: to_int(x), y: to_int(y) }
    }

    // @brief Single piece in an empty bin lands at the upper-left corner with
    // its declared rotation = 0° and AABB dims matching the input rect.
    #[test]
    fn place_single_piece_lands_top_left() {
        let sq: IntPolygon = vec![upx(0.0, 0.0), upx(10.0, 0.0), upx(10.0, 10.0), upx(0.0, 10.0)];
        let polygons = vec![sq];
        let rects = vec![Rect::new(10, 10)];
        let mut cache = NfpCache::new();

        let (placed, free) = place_upper_left_fill(
            100, 100, 0, &polygons, &rects, &[0], &mut cache, None,
        ).expect("placement ok");

        assert_eq!(placed.len(), 1);
        assert_eq!(placed[0].id, 0);
        assert_eq!(placed[0].x, 0);
        assert_eq!(placed[0].y, 0);
        assert_eq!(placed[0].w, 10);
        assert_eq!(placed[0].h, 10);
        assert_eq!(placed[0].rotation_deg, 0);
        // Stage 1 returns no free-rect history.
        assert!(free.is_empty());
    } // place_single_piece_lands_top_left

    // @brief Two unit squares pack flush against each other: the first lands
    // at the bin's upper-left corner (0, 0); the second's UL-Fill anchor
    // sits on the NFP boundary, putting its AABB at (10, 0) — touching the
    // first piece's right edge.
    #[test]
    fn place_two_squares_pack_flush() {
        let sq: IntPolygon = vec![upx(0.0, 0.0), upx(10.0, 0.0), upx(10.0, 10.0), upx(0.0, 10.0)];
        let polygons = vec![sq.clone(), sq];
        let rects = vec![Rect::new(10, 10), Rect::new(10, 10)];
        let mut cache = NfpCache::new();

        let (placed, _) = place_upper_left_fill(
            100, 100, 0, &polygons, &rects, &[0], &mut cache, None,
        ).expect("placement ok");

        // Returned in input-id order regardless of internal area-sort.
        assert_eq!(placed[0].id, 0);
        assert_eq!(placed[0].x, 0);
        assert_eq!(placed[0].y, 0);

        assert_eq!(placed[1].id, 1);
        assert_eq!(placed[1].x, 10);
        assert_eq!(placed[1].y, 0);
    } // place_two_squares_pack_flush

    // @brief Pieces are placed largest-first regardless of input order.  Smaller
    // square listed first in `polygons` still ends up flush against the right
    // edge of the larger square (which was placed first by the area-sort).
    #[test]
    fn place_orders_by_area_descending() {
        let small: IntPolygon = vec![upx(0.0, 0.0), upx(5.0, 0.0), upx(5.0, 5.0), upx(0.0, 5.0)];
        let large: IntPolygon = vec![upx(0.0, 0.0), upx(10.0, 0.0), upx(10.0, 10.0), upx(0.0, 10.0)];
        let polygons = vec![small, large];
        let rects = vec![Rect::new(5, 5), Rect::new(10, 10)];
        let mut cache = NfpCache::new();

        let (placed, _) = place_upper_left_fill(
            100, 100, 0, &polygons, &rects, &[0], &mut cache, None,
        ).expect("placement ok");

        // Large (id=1) placed first at (0, 0); small (id=0) flush against it.
        let p_large = &placed[1];
        let p_small = &placed[0];
        assert_eq!(p_large.id, 1);
        assert_eq!(p_large.x, 0);
        assert_eq!(p_large.y, 0);
        assert_eq!(p_small.id, 0);
        assert_eq!(p_small.x, 10);
        assert_eq!(p_small.y, 0);
    } // place_orders_by_area_descending

    // @brief A piece bigger than the bin in every trial-set orientation
    // returns TooLarge, with the rect's reported dims surfaced in the error.
    #[test]
    fn place_too_large_when_no_orient_fits() {
        let big: IntPolygon = vec![
            upx(0.0, 0.0), upx(200.0, 0.0), upx(200.0, 200.0), upx(0.0, 200.0),
        ];
        let polygons = vec![big];
        let rects = vec![Rect::new(200, 200)];
        let mut cache = NfpCache::new();

        let err = place_upper_left_fill(
            100, 100, 0, &polygons, &rects, &[0, 90], &mut cache, None,
        ).expect_err("should reject");

        match err {
            PackError::TooLarge { id, w, h, bin_w, bin_h } => {
                assert_eq!(id, 0);
                assert_eq!(w, 200);
                assert_eq!(h, 200);
                assert_eq!(bin_w, 100);
                assert_eq!(bin_h, 100);
            }
            other => panic!("expected TooLarge, got {other:?}"),
        }
    } // place_too_large_when_no_orient_fits

    // @brief Two pieces that fit individually but not together → NoSpace on
    // the second piece.  Distinguishes "no room left" from "physically too
    // big" (which would be TooLarge).
    #[test]
    fn place_no_space_when_room_runs_out() {
        // Bin 12×10, two 10×10 squares.  First fits at (0, 0); second has
        // anchor x ∈ [0, 2] and y forced to 0, but the entire forbidden
        // region (NFP) covers x ∈ [-10, 10] at y = 0 — no anchor available.
        let sq: IntPolygon = vec![upx(0.0, 0.0), upx(10.0, 0.0), upx(10.0, 10.0), upx(0.0, 10.0)];
        let polygons = vec![sq.clone(), sq];
        let rects = vec![Rect::new(10, 10), Rect::new(10, 10)];
        let mut cache = NfpCache::new();

        let err = place_upper_left_fill(
            12, 10, 0, &polygons, &rects, &[0], &mut cache, None,
        ).expect_err("should run out of space");

        assert!(matches!(err, PackError::NoSpace { id: 1 }), "got {err:?}");
    } // place_no_space_when_room_runs_out

    // @brief Rotation is consulted when the un-rotated piece doesn't fit:
    // a 5×60 thin rectangle fails 0° in a 70×30 bin (h=60 > 30) but fits
    // at 90° (AABB becomes 60×5).  The placer must pick the 90° trial and
    // record the rotation in the result.
    #[test]
    fn place_rotation_used_when_only_orient_that_fits() {
        let thin: IntPolygon = vec![
            upx(0.0, 0.0), upx(5.0, 0.0), upx(5.0, 60.0), upx(0.0, 60.0),
        ];
        let polygons = vec![thin];
        let rects = vec![Rect::new(5, 60)];
        let mut cache = NfpCache::new();

        let (placed, _) = place_upper_left_fill(
            70, 30, 0, &polygons, &rects, &[0, 90], &mut cache, None,
        ).expect("90° placement ok");

        assert_eq!(placed.len(), 1);
        assert_eq!(placed[0].rotation_deg, 90);
        assert_eq!(placed[0].x, 0);
        assert_eq!(placed[0].y, 0);
        // After 90° SVG-CCW rotation, the AABB is 60 wide × 5 tall.
        assert_eq!(placed[0].w, 60);
        assert_eq!(placed[0].h, 5);
    } // place_rotation_used_when_only_orient_that_fits

    // @brief Empty input is a valid no-op: zero placements, empty free-rect
    // history, no error.
    #[test]
    fn place_empty_input_succeeds() {
        let polygons: Vec<IntPolygon> = Vec::new();
        let rects: Vec<Rect> = Vec::new();
        let mut cache = NfpCache::new();

        let (placed, free) = place_upper_left_fill(
            100, 100, 0, &polygons, &rects, &[0], &mut cache, None,
        ).expect("empty ok");

        assert!(placed.is_empty());
        assert!(free.is_empty());
    } // place_empty_input_succeeds
} // mod tests
