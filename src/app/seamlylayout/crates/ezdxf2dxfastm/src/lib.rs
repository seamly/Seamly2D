// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Convert ezdxf intermediate representation to DXF-ASTM format.
//! @details This crate exports the ezdxf-like Drawing object to DXF-ASTM
//!          (ASTM D6673-10) format.

mod encoder;
mod error;
mod validator;
mod writer;

#[cfg(test)]
mod writer_test;
#[cfg(test)]
mod writer_astm_test;

pub use encoder::{encode_astm_polyline, encode_astm_text, encode_circle, encode_dxf_point, encode_entity, encode_line, encode_notch, encode_polyline, encode_text};
pub use error::{DxfAstmExportError, Result};
pub use validator::{ValidationError, validate_astm_compliance};
pub use writer::{DxfAstmExportOptions, ProgressCallback, export_dxf_astm};
