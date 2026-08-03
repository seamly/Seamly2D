// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Convert ezdxf intermediate representation to DXF-ASTM format.
//! @details This crate exports the ezdxf-like Drawing object to DXF-ASTM
//!          (ASTM-D6673-10) format, enforcing all constraints required
//!          by the standard and Gerber Technology's DXF parser.

mod encoder;
mod error;
mod validator;
mod writer;

#[cfg(test)]
mod writer_test;

pub use encoder::{encode_circle, encode_clo_polyline, encode_dxf_point, encode_entity, encode_line, encode_polyline, encode_text};
pub use error::{DxfAstmExportError, Result};
pub use validator::{ValidationError, validate_astm_compliance};
pub use writer::{DxfAstmExportOptions, ProgressCallback, export_dxf_astm};
