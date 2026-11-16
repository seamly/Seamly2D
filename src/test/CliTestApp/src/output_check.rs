// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief Structure check of one export file: it exists, is not empty, and starts and ends like its format.

use std::path::Path;

/// @brief Check `path` as a file of `format`.
/// @return Ok with the file size, or Err with the first problem found.
pub fn check_export_file(path: &Path, format: &str) -> Result<u64, String> {
    let bytes = std::fs::read(path).map_err(|error| format!("{} cannot be read: {error}", path.display()))?;
    if bytes.is_empty() {
        return Err(format!("{} is empty", path.display()));
    } // if empty
    let starts = |prefix: &[u8]| bytes.starts_with(prefix);
    let tail = String::from_utf8_lossy(&bytes[bytes.len().saturating_sub(2048)..]).to_string();
    let problem = match format {
        "svg" => {
            let text = String::from_utf8_lossy(&bytes);
            (!text.contains("<svg") || !text.trim_end().ends_with("</svg>")).then_some("not a complete SVG document")
        } // svg
        "pdf" | "pdf-tiled" => (!starts(b"%PDF-") || !tail.contains("%%EOF")).then_some("not a complete PDF"),
        "png" => (!starts(&[0x89, b'P', b'N', b'G', 0x0D, 0x0A, 0x1A, 0x0A])).then_some("missing PNG signature"),
        "jpg" | "jpeg" => (!starts(&[0xFF, 0xD8]) || !bytes.ends_with(&[0xFF, 0xD9])).then_some("not a complete JPEG"),
        "dxf" => (!tail.trim_end().ends_with("EOF")).then_some("DXF does not end with EOF"),
        "ps" | "eps" => (!starts(b"%!PS-Adobe-3.0") || !tail.contains("%%EOF")).then_some("not a complete PostScript file"),
        "hpgl" => (!starts(b"IN")).then_some("HP-GL does not start with IN"),
        _ => None, // no structure rule for this format; size checked above
    }; // match format
    match problem {
        Some(problem) => Err(format!("{}: {problem}", path.display())),
        None => Ok(bytes.len() as u64),
    } // match problem
} // fn check_export_file

#[cfg(test)]
mod tests {
    use super::*;

    // A complete file passes; an empty or cut file fails.
    #[test]
    fn complete_files_pass_and_cut_files_fail() {
        let folder = tempfile::tempdir().unwrap();
        let svg = folder.path().join("a.svg");
        std::fs::write(&svg, "<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>\n").unwrap();
        assert!(check_export_file(&svg, "svg").is_ok());
        std::fs::write(&svg, "<svg xmlns=\"http://www.w3.org/2000/svg\"><g>").unwrap();
        assert!(check_export_file(&svg, "svg").is_err());
        let pdf = folder.path().join("a.pdf");
        std::fs::write(&pdf, "%PDF-1.7\n...\n%%EOF\n").unwrap();
        assert!(check_export_file(&pdf, "pdf-tiled").is_ok());
        std::fs::write(&pdf, "").unwrap();
        assert!(check_export_file(&pdf, "pdf").is_err());
        assert!(check_export_file(&folder.path().join("missing.dxf"), "dxf").is_err());
    } // complete_files_pass_and_cut_files_fail
} // mod tests
