// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief Pattern, measurement, and output folders from Seamly2D Preferences.
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

/// @brief Shared preferences beside the per-application settings directory.
fn common_settings_file() -> Option<PathBuf> {
    settings_file().and_then(|file| file.parent()?.parent().map(|folder| folder.join("qt6_common.ini")))
}

/// @brief Read one configured path without changing the preferences.
fn configured_folder(file: Option<PathBuf>, key: &str, fallback: PathBuf) -> PathBuf {
    file.and_then(|file| std::fs::read_to_string(file).ok())
        .and_then(|text| ini_value(&text, "paths", key))
        .filter(|value| !value.trim().is_empty())
        .map(PathBuf::from)
        .unwrap_or(fallback)
}

/// @brief The shared data root from the environment, preferences, or the default user directory.
fn data_root() -> PathBuf {
    if let Some(data) = std::env::var_os("SEAMLY_DATA_DIR").filter(|value| !value.is_empty()) {
        return PathBuf::from(data);
    }
    let fallback = home_folder().map(|home| home.join("seamly2d")).unwrap_or_else(|| PathBuf::from("."));
    configured_folder(common_settings_file(), "dataRoot", fallback)
}

/// @brief The output folder from Preferences > Paths > Layout.
pub fn layout_folder() -> PathBuf {
    configured_folder(settings_file(), "layout", data_root().join("layouts"))
}

/// @brief The picker folder from Preferences > Paths > Patterns.
pub fn pattern_folder() -> PathBuf {
    configured_folder(settings_file(), "pattern", data_root().join("patterns"))
}

/// @brief Identify a multisize reference in the selected pattern.
fn pattern_is_multisize(pattern: &Path) -> bool {
    std::fs::read_to_string(pattern).ok().and_then(|text| {
        roxmltree::Document::parse(&text).ok().and_then(|document| {
            document.descendants().find(|node| node.has_tag_name("measurements"))
                .and_then(|node| node.text()).map(|path| {
                    let name = path.trim().to_ascii_lowercase();
                    name.ends_with(".smms") || name.ends_with(".vst")
                })
        })
    }).unwrap_or(false)
}

/// @brief The measurement picker folder for the selected pattern and measurement type.
pub fn measurement_folder(pattern: &Path, measurements: &Path) -> PathBuf {
    let multisize = match measurements.extension().and_then(|extension| extension.to_str()) {
        Some(extension) if extension.eq_ignore_ascii_case("smms") || extension.eq_ignore_ascii_case("vst") => true,
        Some(extension) if extension.eq_ignore_ascii_case("smis") || extension.eq_ignore_ascii_case("vit") => false,
        _ => pattern_is_multisize(pattern),
    };
    let (key, subdirectory) = if multisize {
        ("multi_size_measurements", "multisize")
    } else {
        ("individual_size_measurements", "individual")
    };
    configured_folder(common_settings_file(), key, data_root().join("measurements").join(subdirectory))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn configured_paths_use_their_preference_keys() {
        let directory = tempfile::tempdir().unwrap();
        let file = directory.path().join("settings.ini");
        std::fs::write(&file, "[paths]\npattern=C:/custom patterns\nindividual_size_measurements=C:/individual\nmulti_size_measurements=C:/multisize\n").unwrap();
        assert_eq!(configured_folder(Some(file.clone()), "pattern", PathBuf::from("fallback")), PathBuf::from("C:/custom patterns"));
        assert_eq!(configured_folder(Some(file.clone()), "individual_size_measurements", PathBuf::new()), PathBuf::from("C:/individual"));
        assert_eq!(configured_folder(Some(file.clone()), "multi_size_measurements", PathBuf::new()), PathBuf::from("C:/multisize"));
        assert_eq!(configured_folder(Some(file), "missing", PathBuf::from("fallback")), PathBuf::from("fallback"));
    }

    #[test]
    fn pattern_measurement_reference_selects_the_directory_type() {
        let directory = tempfile::tempdir().unwrap();
        let file = directory.path().join("pattern.sm2d");
        std::fs::write(&file, "<pattern><measurements>../measurements/test.SMMS</measurements></pattern>").unwrap();
        assert!(pattern_is_multisize(&file));
        std::fs::write(&file, "<pattern><measurements>test.smis</measurements></pattern>").unwrap();
        assert!(!pattern_is_multisize(&file));
    }

    // The key is read from its own section only.
    #[test]
    fn ini_value_reads_one_section() {
        let text = "[General]\nlayout=wrong\n\n[paths]\nlayout=C:/Users/x/seamly2d/layouts\nseamlyLayoutApp=\"C:/a b/SeamlyLayout.exe\"\n";
        assert_eq!(ini_value(text, "paths", "layout").as_deref(), Some("C:/Users/x/seamly2d/layouts"));
        assert_eq!(ini_value(text, "paths", "seamlyLayoutApp").as_deref(), Some("C:/a b/SeamlyLayout.exe"));
        assert_eq!(ini_value(text, "paths", "missing"), None);
    } // ini_value_reads_one_section
} // mod tests
