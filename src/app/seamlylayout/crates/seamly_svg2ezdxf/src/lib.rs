// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Convert SVG DOM to ezdxf-like intermediate representation.
//! @details This crate provides functionality to convert SVG documents
//!          into an intermediate representation similar to Python's ezdxf
//!          Drawing object. This intermediate format can then be exported
//!          to various DXF formats (e.g., DXF-ASTM via ezdxf2dxfastm).

mod converter;
mod drawing;
mod entities;
mod error;
mod layers;
mod utils;

#[cfg(test)]
mod converter_test;

pub use converter::{SvgToEzdxfOptions, svg_to_ezdxf};
pub use drawing::{Block, Drawing, DxfVersion};
pub use entities::{Arc, Circle, DxfPoint, Entity, Line, Point, Polyline, Text};
pub use error::{Result, SvgToEzdxfError};
pub use utils::{detect_corners, invert_y_axis, parse_float_attr, sanitize_ascii, sanitize_block_name};

// @brief Write Drawing to output directory for inspection.
// @param drawing The Drawing object to write.
// @param filename Optional filename (default: "ezdxf_drawing.txt").
// @return Result indicating success or error.
pub fn write_drawing_to_output(
    drawing: &Drawing,
    filename: Option<&str>,
) -> std::io::Result<std::path::PathBuf> {
    use std::path::PathBuf;

    // Use output directory.
    let output_dir = PathBuf::from("output");

    // Create output directory if it doesn't exist.
    std::fs::create_dir_all(&output_dir)?;

    // Determine filename.
    let filename = filename.unwrap_or("ezdxf_drawing.txt");
    let file_path = output_dir.join(filename);

    // Write drawing to file.
    drawing.write_to_file(&file_path)?;

    Ok(file_path)
}
