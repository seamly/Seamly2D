// project: SeamlyLayout
// author: slspencer, copyright 2026 Seamly2D Project
// MIT License: https://opensource.org/licenses/MIT

// @file settings_validation.rs
// @brief Strict check of a layout settings JSON file for headless export.
//
// `LayoutSettings::from_json` and `SettingsModel::load` accept any value and
// fall back to defaults. That suits the Settings dialog, which shows the result.
// A command line shows nothing, so a typing error ("Portrait", "marginTOP")
// must stop the run with a message that names the key.

use serde_json::Value;

use crate::tiling::is_tile_size_name;

// @brief Keys that older settings files carry; they are read or ignored, never refused.
const LEGACY_KEYS: &[&str] = &["outputFormat", "sheetName", "rotationEnabled"];

// @brief Check every key and value of a settings JSON document.
//
// Rules:
// - The document is one JSON object; every key is a known setting.
// - Text settings hold one of their listed values (exact case).
// - Sizes are finite numbers; page and tile sizes are above zero, the others zero or more.
// - A "withNap" layout has a rotation step of 0 (pieces up) or 180 (pieces down).
//
// @param json Settings JSON text, as written by `SettingsModel::save()`.
// @return Ok, or Err with one sentence per problem, in key order.
pub fn validate_settings_json(json: &str) -> Result<(), Vec<String>> {
    let document: Value = serde_json::from_str(json)
        .map_err(|error| vec![format!("The layout settings are not valid JSON: {error}.")])?;
    let Some(object) = document.as_object() else {
        return Err(vec!["The layout settings must be a JSON object.".to_string()]);
    }; // object

    let mut problems = Vec::new();
    for (key, value) in object {
        let problem = match key.as_str() {
            "layoutMode" => check_text(key, value, &["alongGrainline", "withNap", "any", "withGrain", "withoutGrain"]),
            "noGrainlineRotation" => check_text(key, value, &["upright", "free"]),
            "unit" => check_text(key, value, &["in", "mm", "cm"]),
            "mediaType" => check_text(key, value, &["paper", "fabric"]),
            "paperType" => check_text(key, value, &["tiled", "roll", "sheet"]),
            "tileOrientation" => check_text(key, value, &["landscape", "portrait"]),
            "tileSize" => check_tile_size(value),
            "rollSize" => check_roll_size(value),
            "rotationStep" => check_rotation_step(value, object.get("layoutMode")),
            "pageWidth" | "pageHeight" | "tileWidth" | "tileHeight" => check_number(key, value, true),
            "rollWidth" | "marginTop" | "marginBottom" | "marginLeft" | "marginRight" | "fabricWidth"
            | "fabricHeight" | "selvedgeWidth" | "pieceGap" => check_number(key, value, false),
            "fabricFolded" | "scaleGrid" | "foldLinesOnLeftEdge" => check_bool(key, value),
            legacy if LEGACY_KEYS.contains(&legacy) => None, // read by SettingsModel, or retired
            unknown => Some(format!("Unknown layout setting '{unknown}'.")),
        }; // match key
        problems.extend(problem);
    } // for each key

    if problems.is_empty() { Ok(()) } else { Err(problems) }
} // fn validate_settings_json

// @brief A text value that must be one of `allowed`.
fn check_text(key: &str, value: &Value, allowed: &[&str]) -> Option<String> {
    match value.as_str() {
        Some(text) if allowed.contains(&text) => None, // listed value
        _ => Some(format!("'{key}' is {value}; use one of: {}.", allowed.join(", "))),
    } // match value
} // fn check_text

// @brief A tile size name of the catalog, "Custom", or empty (no tile size chosen).
fn check_tile_size(value: &Value) -> Option<String> {
    match value.as_str() {
        Some(name) if name.is_empty() || is_tile_size_name(name) => None, // known size
        _ => Some(format!("'tileSize' is {value}; use a paper name such as Letter, A4 or ARCH D, or Custom.")),
    } // match value
} // fn check_tile_size

// @brief A roll size: "custom", empty, or "<width> <unit>" such as "36 in" or "900 mm".
fn check_roll_size(value: &Value) -> Option<String> {
    let valid = value.as_str().is_some_and(|text| {
        let parts: Vec<&str> = text.split_whitespace().collect();
        match parts.as_slice() {
            [] | ["custom"] => true, // no size, or the rollWidth field
            [width, unit] => {
                width.parse::<f64>().is_ok_and(|width| width.is_finite() && width > 0.0)
                    && ["in", "mm", "cm"].contains(unit)
            } // width and unit
            _ => false, // any other text
        } // match parts
    }); // valid
    if valid {
        None
    } else {
        Some(format!("'rollSize' is {value}; use \"custom\" or a width and unit such as \"36 in\"."))
    } // if valid
} // fn check_roll_size

// @brief A finite number; `positive` also refuses zero, otherwise only negatives are refused.
fn check_number(key: &str, value: &Value, positive: bool) -> Option<String> {
    let valid = value.as_f64().is_some_and(|number| {
        number.is_finite() && if positive { number > 0.0 } else { number >= 0.0 }
    }); // valid
    match (valid, positive) {
        (true, _) => None, // number in range
        (false, true) => Some(format!("'{key}' is {value}; use a number above 0.")),
        (false, false) => Some(format!("'{key}' is {value}; use a number of 0 or more.")),
    } // match valid
} // fn check_number

// @brief A JSON boolean.
fn check_bool(key: &str, value: &Value) -> Option<String> {
    if value.is_boolean() {
        None
    } else {
        Some(format!("'{key}' is {value}; use true or false."))
    } // if boolean
} // fn check_bool

// @brief A finite rotation step; "withNap" allows only 0 (pieces up) and 180 (pieces down).
fn check_rotation_step(value: &Value, layout_mode: Option<&Value>) -> Option<String> {
    let Some(step) = value.as_f64().filter(|step| step.is_finite()) else {
        return Some(format!("'rotationStep' is {value}; use a number."));
    }; // step
    let with_nap = layout_mode.and_then(Value::as_str) == Some("withNap");
    if with_nap && step != 0.0 && step != 180.0 {
        return Some(format!("'rotationStep' is {value}; a withNap layout uses 0 (pieces up) or 180 (pieces down)."));
    } // if withNap step is not a nap direction
    None
} // fn check_rotation_step

#[cfg(test)]
mod tests {
    use super::validate_settings_json;

    // A file written by the Settings dialog passes unchanged.
    #[test]
    fn dialog_settings_pass() {
        let json = r#"{"layoutMode":"withNap","rotationStep":180,"noGrainlineRotation":"free",
            "fabricFolded":true,"scaleGrid":false,"foldLinesOnLeftEdge":true,"unit":"cm",
            "mediaType":"fabric","paperType":"roll","pageWidth":91.44,"pageHeight":121.92,
            "rollSize":"900 mm","rollWidth":90,"tileSize":"A4","tileOrientation":"portrait",
            "tileWidth":21,"tileHeight":29.7,"marginTop":1,"marginBottom":1,"marginLeft":0,
            "marginRight":1,"fabricWidth":150,"fabricHeight":0,"selvedgeWidth":1,"pieceGap":0.2,
            "outputFormat":"svg"}"#;
        assert_eq!(validate_settings_json(json), Ok(()));
    } // dialog_settings_pass

    // An empty object keeps every default.
    #[test]
    fn empty_object_passes() {
        assert_eq!(validate_settings_json("{}"), Ok(()));
    } // empty_object_passes

    // Wrong case, unknown keys and negative sizes each give one message naming the key.
    #[test]
    fn typing_errors_name_each_key() {
        let json = r#"{"tileOrientation":"Portrait","marginTOP":1,"marginLeft":-1,"pageWidth":0,
            "mediaType":"cloth","fabricFolded":"yes","tileSize":"Letterr","rollSize":"wide"}"#;
        let problems = validate_settings_json(json).unwrap_err();
        assert_eq!(problems.len(), 8, "{problems:?}");
        for key in ["tileOrientation", "marginTOP", "marginLeft", "pageWidth", "mediaType", "fabricFolded",
                    "tileSize", "rollSize"] {
            assert!(problems.iter().any(|problem| problem.contains(key)), "{key}: {problems:?}");
        } // for each key
    } // typing_errors_name_each_key

    // withNap takes a nap direction; other modes ignore the step.
    #[test]
    fn with_nap_needs_a_nap_direction() {
        assert!(validate_settings_json(r#"{"layoutMode":"withNap","rotationStep":90}"#).is_err());
        assert_eq!(validate_settings_json(r#"{"layoutMode":"any","rotationStep":90}"#), Ok(()));
    } // with_nap_needs_a_nap_direction

    // Text that is not JSON, or JSON that is not an object, is refused.
    #[test]
    fn non_object_documents_fail() {
        assert!(validate_settings_json("not json").is_err());
        assert!(validate_settings_json("[1, 2]").is_err());
    } // non_object_documents_fail
} // mod tests
