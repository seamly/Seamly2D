// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Error types for DXF-ASTM export.

use thiserror::Error;

// @brief Result type for DXF-ASTM export operations.
pub type Result<T> = std::result::Result<T, DxfAstmExportError>;

// @brief Errors that can occur during DXF-ASTM export.
#[derive(Debug, Error)]
pub enum DxfAstmExportError {
    // I/O error during file writing.
    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),

    // DXF version mismatch (must be R12 for ASTM).
    #[error("Invalid DXF version: {0}")]
    InvalidVersion(String),

    // Entity validation failed.
    #[error("Entity validation error: {0}")]
    Validation(String),

    // Text encoding error (non-ASCII characters).
    #[error("Text encoding error: {0}")]
    TextEncoding(String),
}
