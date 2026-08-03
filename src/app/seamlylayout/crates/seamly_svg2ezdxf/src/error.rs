// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Error types for SVG to ezdxf conversion.

use thiserror::Error;

// @brief Result type for SVG to ezdxf operations.
pub type Result<T> = std::result::Result<T, SvgToEzdxfError>;

// @brief Errors that can occur during SVG to ezdxf conversion.
#[derive(Debug, Error)]
pub enum SvgToEzdxfError {
    // SVG parsing or DOM access error.
    #[error("SVG error: {0}")]
    Svg(String),

    // Geometry conversion error.
    #[error("Geometry error: {0}")]
    Geometry(String),

    // Invalid coordinate system configuration.
    #[error("Coordinate system error: {0}")]
    CoordinateSystem(String),

    // Layer mapping error.
    #[error("Layer mapping error: {0}")]
    Layer(String),
}
