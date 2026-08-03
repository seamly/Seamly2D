// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! # layout_tiling
//!
//! Tiled-paper layout algorithms for SeamlyLayout.
//!
//! This crate is **Qt-free** and **MIT-licensed**, so it can be linked by any
//! consumer — the Qt frontend (via `cxxqt_bridge`), the `cli` batch tool, and
//! any future non-Qt target (web, scripting hook, third-party integration).
//!
//! ## Public API
//!
//! Settings:
//! - [`LayoutSettings`] — deserialized JSON settings (camelCase)
//! - [`LAYOUT_PPI`]     — the canvas PPI constant (96.0)
//!
//! Tile geometry:
//! - [`TileDimensions`]              — computed tile-grid dimensions
//! - [`compute_tile_dims`]           — derive a tile grid from input SVG size + settings
//! - [`create_initial_tiled_layout_dom`] — build the blank tiled layout SVG
//! - [`measurement_to_px`]           — parse an SVG length string into pixels
//!
//! Candidate-width search (for the "horizontal row" pathological case):
//! - [`TiledCandidate`]              — one scored tiled-layout candidate
//! - [`widest_piece_tile_cols`]      — floor of the candidate-width search
//! - [`pick_best_tiled_candidate`]   — evaluate candidate widths and return the winner
//!
//! Debug logging uses the standard [`log`](https://docs.rs/log) crate facade —
//! consumers install whatever log sink they prefer.

pub mod layout_settings;
pub mod tiling;

// Re-export the most-used items at the crate root so callers can write
// `use layout_tiling::LayoutSettings;` rather than
// `use layout_tiling::layout_settings::LayoutSettings;`.
pub use layout_settings::{LayoutSettings, LAYOUT_PPI};
pub use tiling::{
    compute_tile_dims, create_initial_tiled_layout_dom, measurement_to_px,
    pick_best_tiled_candidate, widest_piece_tile_cols, TileDimensions, TiledCandidate,
};
