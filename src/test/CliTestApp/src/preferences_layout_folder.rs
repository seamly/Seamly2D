// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief The Layout folder of Seamly2D Preferences > Paths, the default output folder.
//!
//! Seamly2D keeps `paths/layout` in `qt6_seamly2d.ini`:
//! - with `SEAMLY_DATA_DIR`: `<data>/settings/Seamly/Seamly2D/qt6_seamly2d.ini`;
//! - Windows: `%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini`;
//! - macOS: `~/Library/Preferences/Seamly/Seamly2D/qt6_seamly2d.ini`;
//! - Linux: `$XDG_CONFIG_HOME` or `~/.config`, then `Seamly/Seamly2D/qt6_seamly2d.ini`.
//! Without the key, Seamly2D uses `<home>/seamly2d/layouts`.

use std::path::{Path, PathBuf};

/// @brief Settings file of Seamly2D for this user; None when no home folder is known.
pub fn settings_file() -> Option<PathBuf> {
    if let Some(data) = std::env::var_os("SEAMLY_DATA_DIR").filter(|value| !value.is_empty()) {
        return Some(PathBuf::from(data).join("settings/Seamly/Seamly2D/qt6_seamly2d.ini"));
    } // if a data folder replaces the user folders
    let config_root = if cfg!(windows) {
        std::env::var_os("LOCALAPPDATA").map(PathBuf::from)
    } else if cfg!(target_os = "macos") {
        home_folder().map(|home| home.join("Library/Preferences"))
    } else {
        std::env::var_os("XDG_CONFIG_HOME").map(PathBuf::from).or_else(|| home_folder().map(|home| home.join(".config")))
    }; // config root
    config_root.map(|root| root.join("Seamly/Seamly2D/qt6_seamly2d.ini"))
} // fn settings_file

/// @brief Home folder from USERPROFILE (Windows) or HOME.
fn home_folder() -> Option<PathBuf> {
    std::env::var_os(if cfg!(windows) { "USERPROFILE" } else { "HOME" }).map(PathBuf::from)
} // fn home_folder

/// @brief Value of `key` in `[section]` of INI text; quotes around the value are removed.
pub fn ini_value(text: &str, section: &str, key: &str) -> Option<String> {
    let mut in_section = false;
    for line in text.lines().map(str::trim) {
        if line.starts_with('[') && line.ends_with(']') {
            in_section = line[1..line.len() - 1].eq_ignore_ascii_case(section);
            continue;
        } // if section header
        if !in_section {
            continue;
        } // if another section
        if let Some((name, value)) = line.split_once('=') {
            if name.trim() == key {
                return Some(value.trim().trim_matches('"').to_string());
            } // if key matches
        } // if key and value
    } // for each line
    None
} // fn ini_value

/// @brief The Layout folder of Seamly2D preferences, or `<home>/seamly2d/layouts`.
pub fn layout_folder() -> PathBuf {
    let configured = settings_file()
        .and_then(|file| std::fs::read_to_string(file).ok())
        .and_then(|text| ini_value(&text, "paths", "layout"))
        .filter(|value| !value.is_empty())
        .map(PathBuf::from);
    configured
        .or_else(|| home_folder().map(|home| home.join("seamly2d").join("layouts")))
        .unwrap_or_else(|| Path::new(".").to_path_buf())
} // fn layout_folder

#[cfg(test)]
mod tests {
    use super::*;

    // The key is read from its own section only.
    #[test]
    fn ini_value_reads_one_section() {
        let text = "[General]\nlayout=wrong\n\n[paths]\nlayout=C:/Users/x/seamly2d/layouts\nseamlyLayoutApp=\"C:/a b/SeamlyLayout.exe\"\n";
        assert_eq!(ini_value(text, "paths", "layout").as_deref(), Some("C:/Users/x/seamly2d/layouts"));
        assert_eq!(ini_value(text, "paths", "seamlyLayoutApp").as_deref(), Some("C:/a b/SeamlyLayout.exe"));
        assert_eq!(ini_value(text, "paths", "missing"), None);
    } // ini_value_reads_one_section
} // mod tests
