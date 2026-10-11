// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Saves a copy of the Seamly2D handoff document when asked, to build test fixtures.
//!
//! The handoff reaches SeamlyLayout on standard input and never touches disk.
//! A test fixture needs the exact document, so `SEAMLYLAYOUT_SAVE_HANDOFF`
//! names an absolute file path that receives a copy. Seamly2D starts
//! SeamlyLayout with its own environment, so the variable set for Seamly2D
//! also reaches SeamlyLayout.

use std::path::{Path, PathBuf};

// Environment variable that holds the absolute path of the copy.
pub(crate) const SAVE_HANDOFF_VAR: &str = "SEAMLYLAYOUT_SAVE_HANDOFF";

// @brief Writes `svg` to the absolute file path in `target`.
// @param target Value of `SEAMLYLAYOUT_SAVE_HANDOFF`; None or blank asks for no copy.
// @param svg Handoff document exactly as received.
// @return Ok(None) when no copy was asked for; Ok(Some(path)) when the copy was
//         written; Err(message) for a relative path or a failed write. The
//         import continues in every case.
pub(crate) fn save_handoff_copy(target: Option<&str>, svg: &str) -> Result<Option<PathBuf>, String> {
    // No variable, or an empty one: the normal case.
    let Some(target) = target.map(str::trim).filter(|t| !t.is_empty()) else {
        return Ok(None);
    }; // if no target

    // A relative path would resolve against SeamlyLayout's working directory,
    // which is the install directory, not a place the user chose.
    let path = Path::new(target);
    if !path.is_absolute() {
        return Err(format!("{SAVE_HANDOFF_VAR} must be an absolute file path, not \"{target}\"."));
    } // if relative

    // Bytes as received: the fixture must match the handoff exactly.
    std::fs::write(path, svg.as_bytes())
        .map_err(|e| format!("Could not save the handoff copy to \"{}\": {e}", path.display()))?;
    Ok(Some(path.to_path_buf()))
} // fn save_handoff_copy

#[cfg(test)]
mod tests {
    use super::*;

    // @brief A fresh, empty directory under the system temp directory.
    fn temp_dir(name: &str) -> PathBuf {
        let dir = std::env::temp_dir().join(format!("seamlylayout_handoff_capture_{name}_{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(&dir).expect("temp dir created");
        dir
    }

    #[test]
    fn no_target_writes_nothing() {
        assert_eq!(save_handoff_copy(None, "<svg/>"), Ok(None));
        assert_eq!(save_handoff_copy(Some("  "), "<svg/>"), Ok(None));
    }

    #[test]
    fn absolute_target_receives_the_document_unchanged() {
        let path = temp_dir("absolute").join("handoff.svg");
        let svg = "<svg xmlns=\"http://www.w3.org/2000/svg\">\r\n  <g data-type=\"pattern\"/>\r\n</svg>";
        let written = save_handoff_copy(Some(path.to_str().expect("utf-8 path")), svg).expect("written");
        assert_eq!(written.as_deref(), Some(path.as_path()));
        assert_eq!(std::fs::read_to_string(&path).expect("copy readable"), svg);
    }

    #[test]
    fn relative_target_is_refused() {
        let err = save_handoff_copy(Some("handoff.svg"), "<svg/>").expect_err("relative path refused");
        assert!(err.contains(SAVE_HANDOFF_VAR) && err.contains("absolute"), "{err}");
        assert!(!Path::new("handoff.svg").exists(), "nothing written to the working directory");
    }

    #[test]
    fn unwritable_target_reports_the_path() {
        let path = temp_dir("missing").join("no_such_dir").join("handoff.svg");
        let err = save_handoff_copy(Some(path.to_str().expect("utf-8 path")), "<svg/>").expect_err("write fails");
        assert!(err.contains("no_such_dir"), "{err}");
    }
}
