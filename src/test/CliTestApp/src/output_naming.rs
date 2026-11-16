// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief Output and archive file names of one export session.
//!
//! Names: `<basename>_<material>_<YYMMDDHHMM>.<ext>` for each material, and
//! `<basename>_<YYMMDDHHMM>.zip` for the archive. One stamp serves a whole session.

use std::path::{Path, PathBuf};

/// @brief Material segment for a pattern whose handoff names no material.
pub const UNNAMED_MATERIAL: &str = "pattern";

/// @brief Make a file name component safe on Windows, Linux and macOS.
///
/// Keeps letters, digits, `-` and `.`; every other run of characters becomes one `_`.
/// Leading and trailing `_` and `.` are removed. An empty result becomes `unnamed`.
/// @param text Raw component, such as a material name.
/// @return Sanitized component.
pub fn sanitize_component(text: &str) -> String {
    let mut sanitized = String::new();
    for character in text.chars() {
        let keep = character.is_alphanumeric() || character == '-' || character == '.';
        if keep {
            sanitized.push(character);
        } else if !sanitized.ends_with('_') {
            sanitized.push('_');
        } // if kept or replaced
    } // for each character
    let trimmed = sanitized.trim_matches(|c| c == '_' || c == '.');
    if trimmed.is_empty() { "unnamed".to_string() } else { trimmed.to_string() }
} // fn sanitize_component

/// @brief Session stamp in the form YYMMDDHHMM, local time.
pub fn session_stamp(now: &chrono::DateTime<chrono::Local>) -> String {
    now.format("%y%m%d%H%M").to_string()
} // fn session_stamp

/// @brief File extension of an export format: `pdf-tiled` writes a `.pdf`.
pub fn format_extension(format: &str) -> &str {
    match format {
        "pdf-tiled" => "pdf",
        other => other,
    } // match format
} // fn format_extension

/// @brief `<basename>_<material>_<stamp>.<ext>` in `folder`.
pub fn material_output_path(folder: &Path, basename: &str, material: &str, stamp: &str, format: &str) -> PathBuf {
    let material = if material.trim().is_empty() { UNNAMED_MATERIAL } else { material };
    folder.join(format!(
        "{}_{}_{}.{}",
        sanitize_component(basename),
        sanitize_component(material),
        stamp,
        format_extension(format)
    ))
} // fn material_output_path

/// @brief `<basename>_<stamp>.zip` in `folder`.
pub fn archive_path(folder: &Path, basename: &str, stamp: &str) -> PathBuf {
    folder.join(format!("{}_{}.zip", sanitize_component(basename), stamp))
} // fn archive_path

/// @brief `path`, or `<stem>_2.<ext>`, `<stem>_3.<ext>`, ... when that name exists or is already taken.
///
/// Existing files are never overwritten; `taken` holds the names this session gave out.
pub fn unique_path(path: &Path, taken: &[PathBuf]) -> PathBuf {
    let is_free = |candidate: &Path| !candidate.exists() && !taken.iter().any(|used| used == candidate);
    if is_free(path) {
        return path.to_path_buf();
    } // if the name is free
    let stem = path.file_stem().map(|s| s.to_string_lossy().to_string()).unwrap_or_default();
    let extension = path.extension().map(|e| format!(".{}", e.to_string_lossy())).unwrap_or_default();
    let folder = path.parent().map(Path::to_path_buf).unwrap_or_default();
    (2..)
        .map(|number| folder.join(format!("{stem}_{number}{extension}")))
        .find(|candidate| is_free(candidate))
        .expect("an unused number exists")
} // fn unique_path

#[cfg(test)]
mod tests {
    use super::*;

    // Spaces, slashes and reserved characters become one underscore.
    #[test]
    fn sanitize_replaces_unsafe_characters() {
        assert_eq!(sanitize_component("Fabric"), "Fabric");
        assert_eq!(sanitize_component("Main / Lining: 2"), "Main_Lining_2");
        assert_eq!(sanitize_component("a<>:\"|?*b"), "a_b");
        assert_eq!(sanitize_component("..."), "unnamed");
        assert_eq!(sanitize_component("jacket2_40-146"), "jacket2_40-146");
    } // sanitize_replaces_unsafe_characters

    // The output name follows <basename>_<material>_<YYMMDDHHMM>.<ext>.
    #[test]
    fn output_names_follow_the_pattern() {
        let folder = Path::new("out");
        assert_eq!(
            material_output_path(folder, "jacket 2", "Lining", "2611151830", "pdf-tiled"),
            folder.join("jacket_2_Lining_2611151830.pdf")
        );
        assert_eq!(material_output_path(folder, "shirt", "", "2611151830", "svg"), folder.join("shirt_pattern_2611151830.svg"));
        assert_eq!(archive_path(folder, "shirt", "2611151830"), folder.join("shirt_2611151830.zip"));
    } // output_names_follow_the_pattern

    // A stamp has ten digits: year, month, day, hour, minute.
    #[test]
    fn stamp_has_ten_digits() {
        use chrono::TimeZone;
        let time = chrono::Local.with_ymd_and_hms(2026, 11, 5, 9, 7, 0).unwrap();
        assert_eq!(session_stamp(&time), "2611050907");
    } // stamp_has_ten_digits

    // An existing file or a name given out earlier gets a number.
    #[test]
    fn unique_path_never_reuses_a_name() {
        let folder = tempfile::tempdir().unwrap();
        let first = folder.path().join("shirt_Fabric_2611151830.svg");
        assert_eq!(unique_path(&first, &[]), first);
        std::fs::write(&first, "x").unwrap();
        let second = unique_path(&first, &[]);
        assert_eq!(second, folder.path().join("shirt_Fabric_2611151830_2.svg"));
        assert_eq!(unique_path(&first, &[second.clone()]), folder.path().join("shirt_Fabric_2611151830_3.svg"));
    } // unique_path_never_reuses_a_name
} // mod tests
