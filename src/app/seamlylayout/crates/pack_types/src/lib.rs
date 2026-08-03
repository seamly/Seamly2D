// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! Shared bin-packing data types.
//!
//! Leaf crate: every packer implementation (`layout_engine::pack_maxrects`,
//! `polygon_pack::pack`, …) and the dispatcher (`packing::pack_pieces`) agree
//! on these primitives.  Keeping them in their own crate breaks the would-be
//! cycle between the dispatcher and the polygon-tight implementation while
//! the latter still needs to fall back to the rectangle packer.

// @brief Rectangle to pack, defined by integer dimensions.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Rect {
    // Width in units (typically pixels or mm).
    pub w: u32,
    // Height in units.
    pub h: u32,
}

impl Rect {
    // @brief Construct a rectangle.
    // @param w Width.
    // @param h Height.
    // @return New rectangle.
    pub const fn new(w: u32, h: u32) -> Self {
        Self { w, h }
    }
}

// @brief A rectangle with a position inside a bin.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Placed {
    // Rectangle identifier (matches index in the original list).
    pub id: usize,
    // X coordinate of the top-left corner.
    pub x: u32,
    // Y coordinate of the top-left corner.
    pub y: u32,
    // Width in units.
    pub w: u32,
    // Height in units.
    pub h: u32,
    // Rotation in degrees applied to the piece for this placement.
    // For the rectangle packer this is always 0 or 180 (AABB unchanged).
    // The renderer uses this to compose a `rotate(deg cx cy)` SVG transform.
    pub rotation_deg: u16,
}

// @brief Errors returned by the packing routine.
#[derive(Debug, thiserror::Error, PartialEq, Eq)]
pub enum PackError {
    // A rectangle is larger than the bin and cannot be placed.
    #[error("rect {id} ({w}x{h}) exceeds bin ({bin_w}x{bin_h})")]
    TooLarge {
        // Rectangle identifier.
        id: usize,
        // Rectangle width.
        w: u32,
        // Rectangle height.
        h: u32,
        // Bin width.
        bin_w: u32,
        // Bin height.
        bin_h: u32,
    },
    // The shelf algorithm ran out of vertical space.
    #[error("no space left to place rect {id}")]
    NoSpace {
        // Rectangle identifier.
        id: usize,
    },
    // The packer stopped early due to a runtime/complexity guardrail.
    #[error("search limit reached while placing rect {id}")]
    SearchLimit {
        // Rectangle identifier.
        id: usize,
    },
}

// @brief Result alias for packing.
pub type PackResult<T> = Result<T, PackError>;

// @brief A free (unoccupied) rectangle available for placing a piece.
//
// Used internally by `pack_maxrects` to track available space.
// Free rectangles may overlap each other — that is intentional in the
// MaxRects algorithm, as each rect represents a maximal free region.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FreeRect {
    // X coordinate of the top-left corner.
    pub x: u32,
    // Y coordinate of the top-left corner.
    pub y: u32,
    // Width in units.
    pub w: u32,
    // Height in units.
    pub h: u32,
}

impl FreeRect {
    // @brief True when this free rect overlaps the placed piece area (px..px+pw, py..py+ph).
    //
    // Overlap is strict — touching edges do not count.
    pub fn overlaps_piece(self, px: u32, py: u32, pw: u32, ph: u32) -> bool {
        let sep_x = self.x + self.w <= px || px + pw <= self.x;
        let sep_y = self.y + self.h <= py || py + ph <= self.y;
        !(sep_x || sep_y)
    } // fn overlaps_piece

    // @brief True when this free rect is fully contained within `other`.
    //
    // Used during the pruning step to discard redundant smaller free rects.
    pub fn contained_in(self, other: FreeRect) -> bool {
        other.x <= self.x
            && self.x + self.w <= other.x + other.w
            && other.y <= self.y
            && self.y + self.h <= other.y + other.h
    } // fn contained_in
} // impl FreeRect
