// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! Up-left compaction of placed pieces.
//!
//! Each piece moves up, then left, until the piece gap separates it from a
//! blocking piece or it touches the bin edge.  After compaction no gap above
//! or left of a piece is wider than the piece gap, except where the bin edge
//! is closer.  A piece only moves up or left, so the layout stays inside the
//! bin and no two pieces come closer than the gap.

// @brief Tolerance for coordinate comparisons, in pixels.
const EPSILON: f64 = 1e-6;

// @brief Upper limit of passes over all pieces; a pass with no move ends earlier.
const MAX_PASSES: usize = 64;

// @brief Axis-aligned piece box; the gap is not included.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct PieceBox {
    // Left edge.
    pub x: f64,
    // Top edge.
    pub y: f64,
    // Width.
    pub w: f64,
    // Height.
    pub h: f64,
} // struct PieceBox

impl PieceBox {
    // @brief True when the x ranges are closer than the gap.
    fn x_close(&self, other: &PieceBox, gap: f64) -> bool {
        other.x < self.x + self.w + gap - EPSILON && self.x < other.x + other.w + gap - EPSILON
    } // fn x_close

    // @brief True when the y ranges are closer than the gap.
    fn y_close(&self, other: &PieceBox, gap: f64) -> bool {
        other.y < self.y + self.h + gap - EPSILON && self.y < other.y + other.h + gap - EPSILON
    } // fn y_close
} // impl PieceBox

// @brief Move every box up, then left, as far as the gap and the bin edges allow.
//
// Boxes are processed from top to bottom, then left to right; passes repeat
// until no box moves.  Precondition: no two boxes are closer than `gap` and
// no coordinate is negative.
//
// @param boxes  Boxes to move in place.
// @param gap    Clearance between boxes, in the units of the boxes.
pub fn compact_up_left(boxes: &mut [PieceBox], gap: f64) {
    let mut order: Vec<usize> = (0..boxes.len()).collect();
    for _ in 0..MAX_PASSES {
        // Process the boxes in reading order of their current positions.
        order.sort_by(|&a, &b| {
            boxes[a].y.total_cmp(&boxes[b].y).then(boxes[a].x.total_cmp(&boxes[b].x))
        });

        let mut moved = false;
        for &i in &order {
            let piece = boxes[i];

            // Up: stop under the lowest box above that is closer than the gap in x.
            let top = boxes
                .iter()
                .enumerate()
                .filter(|&(j, q)| j != i && piece.x_close(q, gap) && q.y + q.h + gap <= piece.y + EPSILON)
                .map(|(_, q)| q.y + q.h + gap)
                .fold(0.0, f64::max);
            if top < piece.y - EPSILON {
                boxes[i].y = top;
                moved = true;
            } // if the box moves up

            // Left: stop right of the rightmost box on the left that is closer than the gap in y.
            let piece = boxes[i];
            let left = boxes
                .iter()
                .enumerate()
                .filter(|&(j, q)| j != i && piece.y_close(q, gap) && q.x + q.w + gap <= piece.x + EPSILON)
                .map(|(_, q)| q.x + q.w + gap)
                .fold(0.0, f64::max);
            if left < piece.x - EPSILON {
                boxes[i].x = left;
                moved = true;
            } // if the box moves left
        } // for each box

        if !moved {
            break;
        } // if no box moved in this pass
    } // for each pass
} // fn compact_up_left

#[cfg(test)]
mod tests {
    use super::*;

    // @brief A box moves up to the bin edge, then left against its neighbor at the gap.
    #[test]
    fn box_moves_up_then_left_to_the_gap() {
        let mut boxes = [
            PieceBox { x: 0.0, y: 0.0, w: 10.0, h: 10.0 },
            PieceBox { x: 40.0, y: 7.0, w: 10.0, h: 10.0 },
        ];
        compact_up_left(&mut boxes, 2.0);
        assert_eq!((boxes[1].x, boxes[1].y), (12.0, 0.0));
    } // box_moves_up_then_left_to_the_gap

    // @brief A box under another stops one gap below it, then moves left to the bin edge.
    #[test]
    fn box_stops_one_gap_below_the_box_above() {
        let mut boxes = [
            PieceBox { x: 0.0, y: 0.0, w: 10.0, h: 10.0 },
            PieceBox { x: 5.0, y: 30.0, w: 10.0, h: 10.0 },
        ];
        compact_up_left(&mut boxes, 2.0);
        // At y 12 the y ranges are a full gap apart, so the box above does not block the left move.
        assert_eq!((boxes[1].x, boxes[1].y), (0.0, 12.0));
    } // box_stops_one_gap_below_the_box_above
} // mod tests
