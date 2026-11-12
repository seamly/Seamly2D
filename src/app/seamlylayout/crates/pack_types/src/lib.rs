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
    // True when the packer may also try quarter turns (90°, 270°) for this
    // rectangle, in addition to the layout mode's trial set.  Set for pieces
    // without a grain direction when the user allows free rotation.
    pub free_rotation: bool,
    // Some(angle): the rectangle touches the left bin edge, at this rotation
    // only.  Set for a piece whose fold line goes on the fold of the media.
    pub left_edge_angle: Option<u16>,
    // Minimum x coordinate required for this rectangle inside the bin.
    pub min_left_gap_px: u32,
    // Piece Properties rule carried from the SVG handoff.
    pub piece_rotation: PieceRotationRule,
    // True only when the active layout mode applies per-piece rotation rules.
    pub apply_piece_rotation: bool,
}

impl Rect {
    // @brief Construct a rectangle limited to the layout mode's trial set.
    // @param w Width.
    // @param h Height.
    // @return New rectangle.
    pub const fn new(w: u32, h: u32) -> Self {
        Self {
            w,
            h,
            free_rotation: false,
            left_edge_angle: None,
            min_left_gap_px: 0,
            piece_rotation: PieceRotationRule::Unspecified,
            apply_piece_rotation: false,
        }
    }

    // @brief Return a copy with the given free-rotation flag.
    pub const fn with_free_rotation(self, free_rotation: bool) -> Self {
        Self { free_rotation, ..self }
    }

    // @brief Return a copy that touches the left bin edge at `angle` only.
    pub const fn with_left_edge(self, angle: u16) -> Self {
        Self { left_edge_angle: Some(angle), ..self }
    }

    // @brief Return a copy with the minimum clearance from the left bin edge.
    pub const fn with_min_left_gap(self, gap_px: u32) -> Self {
        Self { min_left_gap_px: gap_px, ..self }
    }

    // @brief Return a copy with the piece's source rotation rule.
    pub const fn with_piece_rotation(self, piece_rotation: PieceRotationRule) -> Self {
        Self { piece_rotation, ..self }
    }

    // @brief Enable or disable applying the source rule in Any layout mode.
    pub const fn with_piece_rotation_enabled(self, apply_piece_rotation: bool) -> Self {
        Self { apply_piece_rotation, ..self }
    }
}

// @brief Source rotation choices carried by a piece, independent of layout mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PieceRotationRule {
    // The handoff did not provide a rule; keep the layout mode's current set.
    Unspecified,
    // None or 1-Way; both permit only the normalized baseline angle.
    Fixed,
    // Permit 0° and 180°.
    TwoWay,
    // Permit quarter turns.
    FourWay,
    // Permit every integer degree in Any layout mode.
    Any,
    // An explicit unsupported value; layout mode Any restricts it to baseline.
    Invalid,
}

impl PieceRotationRule {
    // @brief Return the effective angle set for Any mode, using the legacy set when unspecified.
    pub fn angles_for_any_mode(self, fallback: &[u16]) -> Vec<u16> {
        match self {
            Self::Unspecified => {
                if fallback.is_empty() { vec![0] } else { fallback.to_vec() }
            }
            Self::Fixed | Self::Invalid => vec![0],
            Self::TwoWay => vec![0, 180],
            Self::FourWay => vec![0, 90, 180, 270],
            Self::Any => (0..360).collect(),
        } // match rule
    } // fn angles_for_any_mode

    // @brief Intersect two rules using Any-mode compatibility defaults.
    pub fn intersect_for_any_mode(self, other: Self) -> Self {
        if self == Self::Invalid || other == Self::Invalid {
            return Self::Invalid;
        } // if either source rule is invalid
        let rank = |rule| match rule {
            Self::Unspecified | Self::FourWay => 2,
            Self::Fixed | Self::Invalid => 0,
            Self::TwoWay => 1,
            Self::Any => 3,
        }; // rank
        match rank(self).min(rank(other)) {
            0 => Self::Fixed,
            1 => Self::TwoWay,
            2 => Self::FourWay,
            _ => Self::Any,
        } // intersection
    } // fn intersect_for_any_mode
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
