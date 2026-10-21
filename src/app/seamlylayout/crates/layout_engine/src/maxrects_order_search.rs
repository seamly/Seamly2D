// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! MaxRects placement and piece-order search.
//!
//! MaxRects is box-based: it keeps a list of maximal free boxes, puts each
//! piece in the free box with the best score, and cuts the piece out of every
//! free box it overlaps.  One greedy pass packs the first pieces well, but the
//! last pieces get only the boxes that the earlier pieces left.  The search
//! packs several sort orders with two box scores, then changes the piece order
//! with late-acceptance hill climbing and keeps the best layout.
//!
//! The search is deterministic: the same input always gives the same layout.

use crate::up_left_compaction::{compact_up_left, PieceBox};
use pack_types::{FreeRect, Placed, Rect};
use tracing::debug;

// @brief Upper limit of search work, in evaluations × pieces².
//
// One evaluation packs every piece once.  Its cost grows with the square of
// the piece count, so this limit keeps large layouts to a few seconds.
const SEARCH_WORK: u64 = 60_000_000;

// @brief Upper limit of evaluations for small piece counts.
const MAX_SEARCH_EVALS: u64 = 4_000;

// @brief Length of the late-acceptance history list.
const LAHC_HISTORY: usize = 50;

// @brief Fixed seed of the search random numbers; keeps layouts repeatable.
const SEARCH_SEED: u64 = 0x5EA4_1E1A_7047_0001;

// @brief WeightedTopLeft score: a free box 1 px lower scores the same as a box this many px further right.
const Y_WEIGHT: u64 = 8;

// @brief One orientation a piece can take, with its rotated bounding box.
#[derive(Debug, Clone, Copy)]
pub(crate) struct Orientation {
    // Rotation in degrees from the trial set.
    pub deg: u16,
    // Rotated bounding-box width, without the gap.
    pub w: u32,
    // Rotated bounding-box height, without the gap.
    pub h: u32,
} // struct Orientation

// @brief Score that picks the free box for a piece; the piece goes to the box's top-left corner.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum PlacementRule {
    // Topmost box, then leftmost.  A box 1 px higher wins at any distance to the right.
    TopLeft,
    // Lowest `Y_WEIGHT * y + x`.  A box a few px lower but far to the left wins,
    // so pieces of almost equal height do not leave wide gaps between them.
    WeightedTopLeft,
} // enum PlacementRule

impl PlacementRule {
    // @brief The other rule; a search move.
    fn other(self) -> Self {
        match self {
            // TopLeft changes to WeightedTopLeft
            PlacementRule::TopLeft => PlacementRule::WeightedTopLeft,
            // WeightedTopLeft changes to TopLeft
            PlacementRule::WeightedTopLeft => PlacementRule::TopLeft,
        } // match self
    } // fn other
} // impl PlacementRule

// @brief How much of the order search to run.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum SearchDepth {
    // Start orders with both rules only: ten packs at most.  For callers that
    // pack many times, such as the sheet assignment that retries after each
    // piece it defers.
    StartOrders,
    // Start orders, then hill climbing over the piece order.  For one pack per layout.
    Full,
} // enum SearchDepth

// @brief Result of one pass that places the pieces in a given order.
pub(crate) struct Decoded {
    // Placements in placement order.
    pub placements: Vec<Placed>,
    // Ids that fit the empty bin but found no room, in placement order.
    pub no_space: Vec<usize>,
    // Free boxes in creation order, clipped to the bin; empty unless recorded.
    pub history: Vec<FreeRect>,
} // struct Decoded

// @brief Layout quality; a smaller value is a better layout.
//
// Fields compare in order: the area of the pieces left out, the height the
// pieces use, the side gap excess, then the area moment.  Of two layouts with
// the same height, the one with less whitespace between side-by-side pieces
// wins.  The moment is lower when the pieces sit higher.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
struct Fitness {
    unplaced_area: u64,
    height: u32,
    side_gaps: u64,
    moment: u64,
} // struct Fitness

// @brief One state of the search: a piece order and a placement rule.
#[derive(Clone)]
struct Candidate {
    order: Vec<usize>,
    rule: PlacementRule,
    fitness: Fitness,
} // struct Candidate

// @brief Fixed inputs of one pack.
//
// The gap is added to the right and bottom side of every piece and of the bin.
// Pieces then touch in the packer, and every pair keeps the full gap on the media.
pub(crate) struct PackJob {
    // Content width in pixels, without the gap.
    bin_w: u32,
    // Content height in pixels, without the gap.
    bin_h: u32,
    // Clearance between pieces in pixels.
    gap: u32,
    // Orientations per piece that fit the empty bin, in trial-set order.
    // Empty when the piece does not fit the empty bin.
    orientations: Vec<Vec<Orientation>>,
    // Unrotated area per piece.
    areas: Vec<u64>,
} // struct PackJob

impl PackJob {
    // @brief Build the job from the pieces and the trial angle of each piece.
    //
    // @param trial_for  Trial angles for one piece, in preference order.
    // @param rotate     Rotated bounding box (w, h) of a w × h piece at an angle.
    pub fn new(
        bin_w: u32,
        bin_h: u32,
        gap: u32,
        rects: &[Rect],
        trial_for: impl Fn(&Rect) -> Vec<u16>,
        rotate: impl Fn(u32, u32, u16) -> (u32, u32),
    ) -> Self {
        let orientations = rects
            .iter()
            .map(|r| {
                let mut list: Vec<Orientation> = Vec::new();
                for deg in trial_for(r) {
                    let (w, h) = rotate(r.w, r.h, deg);
                    // An orientation that does not fit the empty bin never fits later.
                    let fits_bin = w <= bin_w && h <= bin_h;
                    // An orientation with the size of an earlier one never wins a tie.
                    let duplicate = list.iter().any(|o| o.w == w && o.h == h);
                    if fits_bin && !duplicate {
                        list.push(Orientation { deg, w, h });
                    } // if fits_bin && !duplicate
                } // for deg
                list
            })
            .collect();
        let areas = rects.iter().map(|r| r.w as u64 * r.h as u64).collect();
        Self { bin_w, bin_h, gap, orientations, areas }
    } // fn new

    // @brief True when the piece fits the empty bin in one orientation.
    pub fn fits_empty_bin(&self, id: usize) -> bool {
        !self.orientations[id].is_empty()
    } // fn fits_empty_bin

    // @brief Place the pieces in `order` with MaxRects and the given rule, then compact them.
    //
    // Each piece goes to the top-left corner of the free box the rule picks.
    // The placed piece, with its gap, is cut out of every free box it
    // overlaps.  A piece without room is recorded in `no_space`.  At the end
    // every piece moves up, then left, until the gap or the bin edge stops it.
    //
    // @param record  True records the free-box history for the debug overlay.
    pub fn place_in_order(&self, order: &[usize], rule: PlacementRule, record: bool) -> Decoded {
        let first = FreeRect {
            x: 0,
            y: 0,
            w: self.bin_w.saturating_add(self.gap),
            h: self.bin_h.saturating_add(self.gap),
        };
        let mut free: Vec<FreeRect> = vec![first];
        let mut fresh: Vec<FreeRect> = Vec::new();
        let mut decoded = Decoded {
            placements: Vec::with_capacity(order.len()),
            no_space: Vec::new(),
            history: Vec::new(),
        };
        if record {
            decoded.history.push(first);
        } // if record

        for &id in order {
            // Find the corner and orientation the rule prefers.
            let Some((x, y, o)) = self.choose_position(id, &free, rule) else {
                decoded.no_space.push(id);
                continue;
            }; // let Some(position)

            decoded.placements.push(Placed { id, x, y, w: o.w, h: o.h, rotation_deg: o.deg });
            split_free_rects(&mut free, &mut fresh, x, y, o.w + self.gap, o.h + self.gap);
            if record {
                decoded.history.extend_from_slice(&fresh);
            } // if record
        } // for id

        // Close any gap above or left of a piece that is wider than the piece gap.
        // Whole-pixel boxes stay whole pixels: compaction only adds sizes and gaps.
        let mut boxes: Vec<PieceBox> = decoded
            .placements
            .iter()
            .map(|p| PieceBox { x: p.x as f64, y: p.y as f64, w: p.w as f64, h: p.h as f64 })
            .collect();
        compact_up_left(&mut boxes, self.gap as f64);
        for (p, b) in decoded.placements.iter_mut().zip(&boxes) {
            p.x = b.x as u32;
            p.y = b.y as u32;
        } // for each placement

        // History boxes include the gap strip at the right and bottom; clip them to the bin.
        if record {
            decoded.history = decoded
                .history
                .iter()
                .filter(|f| f.x < self.bin_w && f.y < self.bin_h)
                .map(|f| FreeRect {
                    x: f.x,
                    y: f.y,
                    w: f.w.min(self.bin_w - f.x),
                    h: f.h.min(self.bin_h - f.y),
                })
                .collect();
        } // if record
        decoded
    } // fn place_in_order

    // @brief Corner and orientation for one piece, or None when no free box holds it.
    //
    // Ties go to the smaller bounding box, then to the earlier trial angle,
    // so a piece keeps the layout mode's angle when a turn gains nothing.
    fn choose_position(
        &self,
        id: usize,
        free: &[FreeRect],
        rule: PlacementRule,
    ) -> Option<(u32, u32, Orientation)> {
        let gap = self.gap;
        let mut best: Option<((u64, u32, u32, u64, usize), u32, u32, Orientation)> = None;
        for f in free {
            for (k, o) in self.orientations[id].iter().enumerate() {
                if f.w < o.w.saturating_add(gap) || f.h < o.h.saturating_add(gap) {
                    continue;
                } // if the box does not hold the piece
                let score = match rule {
                    // TopLeft: topmost, then leftmost
                    PlacementRule::TopLeft => f.y as u64,
                    // WeightedTopLeft: y counts Y_WEIGHT times as much as x
                    PlacementRule::WeightedTopLeft => f.y as u64 * Y_WEIGHT + f.x as u64,
                }; // match rule
                let key = (score, f.y, f.x, o.w as u64 * o.h as u64, k);
                if best.map_or(true, |b| key < b.0) {
                    best = Some((key, f.x, f.y, *o));
                } // if better
            } // for orientation
        } // for free box
        best.map(|(_, x, y, o)| (x, y, o))
    } // fn choose_position

    // @brief Quality of a decoded layout.
    fn fitness(&self, decoded: &Decoded) -> Fitness {
        let unplaced_area = decoded.no_space.iter().map(|&id| self.areas[id]).sum();
        let height = decoded.placements.iter().map(|p| p.y + p.h).max().unwrap_or(0);
        let moment = decoded
            .placements
            .iter()
            .map(|p| p.w as u64 * p.h as u64 * (2 * p.y as u64 + p.h as u64))
            .sum();
        let side_gaps = side_gap_excess(&decoded.placements, self.gap);
        Fitness { unplaced_area, height, side_gaps, moment }
    } // fn fitness

    // @brief Pack order and rule, and return the quality.
    fn evaluate(&self, order: Vec<usize>, rule: PlacementRule) -> Candidate {
        let fitness = self.fitness(&self.place_in_order(&order, rule, false));
        Candidate { order, rule, fitness }
    } // fn evaluate
} // impl PackJob

// @brief Total horizontal whitespace, beyond the gap, left of each piece.
//
// Two pieces sit side by side when their y ranges share at least half the
// height of the lower piece.  The whitespace left of a piece ends at the
// nearest side-by-side piece on its left, or at the bin edge.  A piece that
// up-left compaction stops on a thin overlap can still have a wide hole
// on its left; this sum counts that hole.
pub(crate) fn side_gap_excess(placements: &[Placed], gap: u32) -> u64 {
    placements
        .iter()
        .map(|p| {
            // Right edge plus gap of the nearest side-by-side piece on the left; 0 is the bin edge.
            let stop = placements
                .iter()
                .filter(|q| q.x + q.w <= p.x)
                .filter(|q| {
                    let shared = (p.y + p.h).min(q.y + q.h).saturating_sub(p.y.max(q.y));
                    2 * shared >= p.h.min(q.h)
                })
                .map(|q| q.x + q.w + gap)
                .max()
                .unwrap_or(0);
            p.x.saturating_sub(stop) as u64
        })
        .sum()
} // fn side_gap_excess

// @brief Cut the placed piece out of every free box it overlaps.
//
// Up to four maximal strips (left, right, top, bottom) replace each
// overlapped box.  A new strip inside another free box is dropped.
// An old box is never inside a new strip: the strip lies inside the
// box it was cut from, and the free list holds no nested boxes.
// So only the new strips need the containment test.
//
// @param fresh  Scratch list; holds the new strips (before the containment test) on return.
fn split_free_rects(free: &mut Vec<FreeRect>, fresh: &mut Vec<FreeRect>, px: u32, py: u32, pw: u32, ph: u32) {
    fresh.clear();
    free.retain(|&f| {
        if !f.overlaps_piece(px, py, pw, ph) {
            return true;
        } // if no overlap
        if px > f.x {
            fresh.push(FreeRect { x: f.x, y: f.y, w: px - f.x, h: f.h });
        } // if left strip
        if px + pw < f.x + f.w {
            fresh.push(FreeRect { x: px + pw, y: f.y, w: f.x + f.w - (px + pw), h: f.h });
        } // if right strip
        if py > f.y {
            fresh.push(FreeRect { x: f.x, y: f.y, w: f.w, h: py - f.y });
        } // if top strip
        if py + ph < f.y + f.h {
            fresh.push(FreeRect { x: f.x, y: py + ph, w: f.w, h: f.y + f.h - (py + ph) });
        } // if bottom strip
        false
    }); // retain

    let old_count = free.len();
    for (i, &r) in fresh.iter().enumerate() {
        let inside_old = free[..old_count].iter().any(|&f| r.contained_in(f));
        // Of two equal strips, only the first stays.
        let inside_new = fresh
            .iter()
            .enumerate()
            .any(|(j, &o)| j != i && r.contained_in(o) && (r != o || j < i));
        if !inside_old && !inside_new {
            free.push(r);
        } // if maximal
    } // for new strip
} // fn split_free_rects

// @brief SplitMix64 random numbers; small, fast, and repeatable.
struct SplitMix64(u64);

impl SplitMix64 {
    // @brief Next 64-bit value.
    fn next(&mut self) -> u64 {
        self.0 = self.0.wrapping_add(0x9E37_79B9_7F4A_7C15);
        let mut z = self.0;
        z = (z ^ (z >> 30)).wrapping_mul(0xBF58_476D_1CE4_E5B9);
        z = (z ^ (z >> 27)).wrapping_mul(0x94D0_49BB_1331_11EB);
        z ^ (z >> 31)
    } // fn next

    // @brief Value in 0..n; n must be above 0.
    fn below(&mut self, n: usize) -> usize {
        (self.next() % n as u64) as usize
    } // fn below
} // impl SplitMix64

// @brief Start orders: the ids sorted in descending order of five size keys.
//
// Equal keys keep the ids in ascending order.  The first order (area) is the
// order of the single-pass packer.  Duplicate orders are dropped.
fn seed_orders(rects: &[Rect], ids: &[usize]) -> Vec<Vec<usize>> {
    let keys: [fn(&Rect) -> (u64, u64); 5] = [
        // area, then longer side
        |r| (r.w as u64 * r.h as u64, r.w.max(r.h) as u64),
        // height, then width
        |r| (r.h as u64, r.w as u64),
        // width, then height
        |r| (r.w as u64, r.h as u64),
        // longer side, then shorter side
        |r| (r.w.max(r.h) as u64, r.w.min(r.h) as u64),
        // perimeter, then area
        |r| (r.w as u64 + r.h as u64, r.w as u64 * r.h as u64),
    ];
    let mut orders: Vec<Vec<usize>> = Vec::with_capacity(keys.len());
    for key in keys {
        let mut order = ids.to_vec();
        order.sort_by(|&a, &b| key(&rects[b]).cmp(&key(&rects[a])).then(a.cmp(&b)));
        if !orders.contains(&order) {
            orders.push(order);
        } // if new order
    } // for key
    orders
} // fn seed_orders

// @brief Find a good piece order and placement rule, and return its layout.
//
// 1. Pack every start order with both rules; keep the best.
// 2. Late-acceptance hill climbing: swap two pieces, move one piece, or change
//    the rule.  Keep a change that is no worse than the current layout or the
//    layout of `LAHC_HISTORY` steps ago.
// 3. Stop at the evaluation limit, or when the height equals the tallest piece
//    and every piece is placed.
//
// @param rects     Pieces; index-aligned with the job.
// @param depth     `StartOrders` skips step 2.
// @param progress  Optional callback (steps done, total steps); called at most about 100 times.
// @return          Layout of the best order, with the free-box history recorded.
pub(crate) fn search_layout(
    job: &PackJob,
    rects: &[Rect],
    depth: SearchDepth,
    mut progress: Option<&mut dyn FnMut(usize, usize)>,
) -> Decoded {
    let ids: Vec<usize> = (0..rects.len()).filter(|&id| job.fits_empty_bin(id)).collect();
    let n = ids.len() as u64;
    let seeds = seed_orders(rects, &ids);
    let rules = [PlacementRule::TopLeft, PlacementRule::WeightedTopLeft];
    let evals = match depth {
        // Start orders only: no hill climbing
        SearchDepth::StartOrders => 0,
        // Full: the work limit sets the evaluation count
        SearchDepth::Full if n >= 2 => (SEARCH_WORK / (n * n)).min(MAX_SEARCH_EVALS) as usize,
        // Full with fewer than two pieces: no order to change
        SearchDepth::Full => 0,
    }; // match depth
    let total_steps = seeds.len() * rules.len() + evals;

    // No layout is lower than the tallest piece in its flattest orientation.
    let height_floor = ids
        .iter()
        .filter_map(|&id| job.orientations[id].iter().map(|o| o.h).min())
        .max()
        .unwrap_or(0);
    let is_floor = |c: &Candidate| c.fitness.unplaced_area == 0 && c.fitness.height <= height_floor;

    // Report progress only when the whole percentage changes.
    let mut last_percent = usize::MAX;
    let mut report = |done: usize| {
        let percent = done * 100 / total_steps.max(1);
        if percent != last_percent {
            last_percent = percent;
            if let Some(cb) = progress.as_mut() {
                cb(done, total_steps);
            } // if callback
        } // if percent changed
    }; // report

    // --- 1 start orders × rules ---
    let mut steps = 0usize;
    let mut best: Option<Candidate> = None;
    for order in &seeds {
        for &rule in &rules {
            let cand = job.evaluate(order.clone(), rule);
            if best.as_ref().map_or(true, |b| cand.fitness < b.fitness) {
                best = Some(cand);
            } // if better
            steps += 1;
            report(steps);
        } // for rule
    } // for order
    let Some(mut best) = best else {
        // No piece fits the empty bin: an empty layout.
        return job.place_in_order(&[], PlacementRule::TopLeft, true);
    }; // let Some(best)
    let seed_fitness = best.fitness;

    // --- 2 late-acceptance hill climbing over the order ---
    let mut rng = SplitMix64(SEARCH_SEED);
    let mut current = best.clone();
    let mut accepted_before = [current.fitness; LAHC_HISTORY];
    let mut evals_done = 0usize;
    for step in 0..evals {
        if is_floor(&best) {
            break;
        } // if no better layout exists

        // Change the current state with one move.
        let mut order = current.order.clone();
        let mut rule = current.rule;
        let len = order.len();
        match rng.below(20) {
            // 45 %: swap two pieces
            0..=8 => {
                let i = rng.below(len);
                let mut j = rng.below(len - 1);
                if j >= i {
                    j += 1;
                } // if j at or after i
                order.swap(i, j);
            } // swap
            // 45 %: move one piece to another place in the order
            9..=17 => {
                let piece = order.remove(rng.below(len));
                order.insert(rng.below(len), piece);
            } // move
            // 10 %: change the placement rule
            _ => rule = rule.other(),
        } // match move

        let cand = job.evaluate(order, rule);
        let slot = step % LAHC_HISTORY;
        if cand.fitness <= current.fitness || cand.fitness <= accepted_before[slot] {
            current = cand;
        } // if accepted
        accepted_before[slot] = current.fitness;
        if current.fitness < best.fitness {
            best = current.clone();
        } // if new best

        evals_done += 1;
        report(steps + evals_done);
    } // for step
    report(total_steps);

    debug!(
        "maxrects_order_search::search_layout(): pieces={}, evals={}/{}, seed={:?}, best={:?}, rule={:?}",
        n, evals_done, evals, seed_fitness, best.fitness, best.rule
    );

    // --- 3 pack the best order again and record the free-box history ---
    job.place_in_order(&best.order, best.rule, true)
} // fn search_layout

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Placement at (x, y) with size w × h.
    fn at(id: usize, x: u32, y: u32, w: u32, h: u32) -> Placed {
        Placed { id, x, y, w, h, rotation_deg: 0 }
    } // fn at

    // @brief Side-by-side pieces at the gap add nothing; a wider space adds its excess.
    #[test]
    fn side_gap_excess_counts_space_beyond_the_gap() {
        assert_eq!(side_gap_excess(&[at(0, 0, 0, 10, 10), at(1, 15, 0, 10, 10)], 5), 0);
        assert_eq!(side_gap_excess(&[at(0, 0, 0, 10, 10), at(1, 40, 2, 10, 10)], 5), 25);
    } // side_gap_excess_counts_space_beyond_the_gap

    // @brief A piece that shares only a thin y band is not side by side: the space runs to the bin edge.
    #[test]
    fn side_gap_excess_ignores_a_thin_overlap() {
        // y ranges share 2 px of a 10 px height.
        assert_eq!(side_gap_excess(&[at(0, 0, 0, 10, 10), at(1, 15, 8, 10, 10)], 5), 15);
    } // side_gap_excess_ignores_a_thin_overlap

    // @brief WeightedTopLeft takes a box a few px lower at the left; TopLeft takes the higher box.
    #[test]
    fn weighted_rule_prefers_a_slightly_lower_box_far_left() {
        let rects = [Rect::new(10, 10)];
        let job = PackJob::new(200, 100, 0, &rects, |_| vec![0], |w, h, _| (w, h));
        let free = [FreeRect { x: 0, y: 2, w: 200, h: 98 }, FreeRect { x: 150, y: 0, w: 50, h: 100 }];
        assert_eq!(job.choose_position(0, &free, PlacementRule::TopLeft).map(|(x, y, _)| (x, y)), Some((150, 0)));
        assert_eq!(job.choose_position(0, &free, PlacementRule::WeightedTopLeft).map(|(x, y, _)| (x, y)), Some((0, 2)));
    } // weighted_rule_prefers_a_slightly_lower_box_far_left

    // @brief The same input always gives the same layout.
    #[test]
    fn search_is_deterministic() {
        let rects: Vec<Rect> = (0..20).map(|i| Rect::new(30 + 7 * (i % 5), 40 + 3 * (i % 7))).collect();
        let job = PackJob::new(200, 2000, 3, &rects, |_| vec![0, 180], |w, h, _| (w, h));
        let first = search_layout(&job, &rects, SearchDepth::Full, None).placements;
        let second = search_layout(&job, &rects, SearchDepth::Full, None).placements;
        assert_eq!(first, second);
    } // search_is_deterministic
} // mod tests
