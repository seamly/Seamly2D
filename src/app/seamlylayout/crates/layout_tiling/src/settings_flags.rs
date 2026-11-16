// project: SeamlyLayout
// author: slspencer, copyright 2026 Seamly2D Project
// MIT License: https://opensource.org/licenses/MIT

// @file settings_flags.rs
// @brief Direct layout flags of the command line, merged over a settings JSON document.
//
// Seamly2D `--layout-<name> <value>` reaches SeamlyLayout as `--<name> <value>`.
// Precedence: saved settings, then the `--layout-settings` file, then these flags.
//
// | Flag             | Settings keys                                  | Implies               |
// |------------------|------------------------------------------------|-----------------------|
// | unit             | unit                                           |                       |
// | media            | mediaType                                      |                       |
// | paper            | paperType                                      | paper media           |
// | tile-size        | tileSize                                       | tiled paper           |
// | tile-orientation | tileOrientation                                | tiled paper           |
// | tile-width       | tileWidth (tileSize Custom)                    | tiled paper           |
// | tile-height      | tileHeight (tileSize Custom)                   | tiled paper           |
// | roll-width       | rollSize "custom", rollWidth                   | paper roll            |
// | fabric-width     | fabricWidth                                    | fabric                |
// | fabric-folded    | fabricFolded (yes, no)                         | fabric                |
// | selvedge-width   | selvedgeWidth                                  | fabric                |
// | margins          | marginTop, marginLeft, marginRight, marginBottom |                     |
// | margin-<side>    | margin<Side>; wins over margins                |                       |
// | ignore-margins   | every margin 0 (no value)                      |                       |
// | gap              | pieceGap                                       |                       |
// | mode             | layoutMode                                     |                       |
// | nap              | rotationStep 0 (up) or 180 (down)              | layoutMode withNap    |
// | no-grainline     | noGrainlineRotation                            |                       |

use serde_json::{Map, Value};

use crate::settings_validation::validate_settings_json;

// @brief Flag names in precedence order: a later flag wins over an earlier one for the same key.
pub const LAYOUT_FLAG_NAMES: &[&str] = &[
    "unit", "media", "paper", "tile-size", "tile-orientation", "tile-width", "tile-height", "roll-width",
    "fabric-width", "fabric-folded", "selvedge-width", "margins", "margin-top", "margin-left", "margin-right",
    "margin-bottom", "ignore-margins", "gap", "mode", "nap", "no-grainline",
];

// @brief Flags that take no value.
pub const LAYOUT_SWITCH_NAMES: &[&str] = &["ignore-margins"];

const TILE_FLAGS: &[&str] = &["tile-size", "tile-orientation", "tile-width", "tile-height"];
const ROLL_FLAGS: &[&str] = &["roll-width"];
const FABRIC_FLAGS: &[&str] = &["fabric-width", "fabric-folded", "selvedge-width"];
const MARGIN_FLAGS: &[&str] = &["margins", "margin-top", "margin-left", "margin-right", "margin-bottom"];

// @brief Merge direct layout flags over a settings JSON document.
//
// @param base_json Settings JSON object (saved settings or the `--layout-settings` file).
// @param flags     `name=value` items; a switch has an empty value. Names are those of `LAYOUT_FLAG_NAMES`.
// @return The merged settings JSON, checked by `validate_settings_json`; or one sentence per problem.
pub fn apply_layout_flags(base_json: &str, flags: &[String]) -> Result<String, Vec<String>> {
    let mut settings: Map<String, Value> = match serde_json::from_str::<Value>(base_json) {
        Ok(Value::Object(object)) => object,
        _ => return Err(vec!["The layout settings are not a JSON object.".to_string()]),
    }; // settings

    // Each flag once; an unknown name or a duplicate is a usage error.
    let mut problems = Vec::new();
    let mut given: Vec<(&str, String)> = Vec::new();
    for item in flags {
        let (name, value) = item.split_once('=').unwrap_or((item.as_str(), ""));
        match LAYOUT_FLAG_NAMES.iter().find(|known| **known == name) {
            None => problems.push(format!("Unknown layout flag '{name}'.")),
            Some(_) if given.iter().any(|(seen, _)| *seen == name) => problems.push(format!("Layout flag {name} is given twice.")),
            Some(known) => given.push((known, value.trim().to_string())),
        } // match name
    } // for each flag
    let value_of = |name: &str| given.iter().find(|(seen, _)| *seen == name).map(|(_, value)| value.as_str());
    let any = |names: &[&str]| names.iter().any(|name| value_of(name).is_some());

    // Media: the paper, roll and fabric flags imply their media; two media are a conflict.
    let media = value_of("media");
    let paper = value_of("paper");
    let wants_fabric = any(FABRIC_FLAGS) || media == Some("fabric");
    let wants_paper = paper.is_some() || any(TILE_FLAGS) || any(ROLL_FLAGS) || media == Some("paper");
    if wants_fabric && wants_paper {
        problems.push("Fabric flags (media fabric, fabric-width, fabric-folded, selvedge-width) conflict with paper flags (media paper, paper, tile-*, roll-width).".to_string());
    } // if two media
    if any(TILE_FLAGS) && (paper == Some("roll") || any(ROLL_FLAGS)) {
        problems.push("Tile flags select tiled paper; they conflict with paper roll and roll-width.".to_string());
    } // if tiled and roll
    if any(ROLL_FLAGS) && paper == Some("tiled") {
        problems.push("roll-width selects a paper roll; it conflicts with paper tiled.".to_string());
    } // if roll and tiled
    if let Some(size) = value_of("tile-size") {
        if size != "Custom" && (value_of("tile-width").is_some() || value_of("tile-height").is_some()) {
            problems.push(format!("tile-width and tile-height set a Custom tile; tile-size is {size}."));
        } // if a named size with a custom width
    } // if tile size
    if value_of("ignore-margins").is_some() && any(MARGIN_FLAGS) {
        problems.push("ignore-margins sets every margin to 0; it conflicts with margins and margin-*.".to_string());
    } // if two margin choices
    if let (Some(_), Some(mode)) = (value_of("nap"), value_of("mode")) {
        if mode != "withNap" {
            problems.push(format!("nap needs layout mode withNap; mode is {mode}."));
        } // if another mode
    } // if nap and mode

    let mut set = |key: &str, value: Value| {
        settings.insert(key.to_string(), value);
    }; // set
    if wants_fabric && !wants_paper {
        set("mediaType", Value::from("fabric"));
    } else if wants_paper && !wants_fabric {
        set("mediaType", Value::from("paper"));
        let paper_type = paper.or(any(TILE_FLAGS).then_some("tiled")).or(any(ROLL_FLAGS).then_some("roll"));
        if let Some(paper_type) = paper_type {
            set("paperType", Value::from(paper_type));
        } // if a paper type follows
    } // if one media

    // Values in precedence order.
    for (name, value) in given.iter().map(|(name, value)| (*name, value.as_str())) {
        let number = || -> Option<Value> {
            match value.parse::<f64>() {
                Ok(number) if number.is_finite() => Some(Value::from(number)),
                _ => None,
            } // match parse
        }; // number
        let number_or_problem = |problems: &mut Vec<String>| {
            let parsed = number();
            if parsed.is_none() {
                problems.push(format!("Layout flag {name}: '{value}' is not a number."));
            } // if not a number
            parsed
        }; // number_or_problem
        match name {
            "unit" => set("unit", Value::from(value)),
            "tile-size" => set("tileSize", Value::from(value)),
            "tile-orientation" => set("tileOrientation", Value::from(value)),
            "tile-width" | "tile-height" => {
                if let Some(number) = number_or_problem(&mut problems) {
                    set(if name == "tile-width" { "tileWidth" } else { "tileHeight" }, number);
                    set("tileSize", Value::from("Custom"));
                } // if a number
            } // custom tile
            "roll-width" => {
                if let Some(number) = number_or_problem(&mut problems) {
                    set("rollSize", Value::from("custom"));
                    set("rollWidth", number);
                } // if a number
            } // roll width
            "fabric-width" => {
                if let Some(number) = number_or_problem(&mut problems) { set("fabricWidth", number); }
            } // fabric width
            "selvedge-width" => {
                if let Some(number) = number_or_problem(&mut problems) { set("selvedgeWidth", number); }
            } // selvedge width
            "fabric-folded" => match value {
                "yes" | "true" => set("fabricFolded", Value::from(true)),
                "no" | "false" => set("fabricFolded", Value::from(false)),
                _ => problems.push(format!("Layout flag fabric-folded: '{value}'; use yes or no.")),
            }, // fabric folded
            "margins" => {
                if let Some(number) = number_or_problem(&mut problems) {
                    for key in ["marginTop", "marginLeft", "marginRight", "marginBottom"] {
                        set(key, number.clone());
                    } // for each side
                } // if a number
            } // margins
            "margin-top" | "margin-left" | "margin-right" | "margin-bottom" => {
                if let Some(number) = number_or_problem(&mut problems) {
                    let key = match name {
                        "margin-top" => "marginTop",
                        "margin-left" => "marginLeft",
                        "margin-right" => "marginRight",
                        _ => "marginBottom",
                    }; // key
                    set(key, number);
                } // if a number
            } // one margin
            "ignore-margins" => {
                if !value.is_empty() {
                    problems.push("Layout flag ignore-margins takes no value.".to_string());
                } // if a value
                for key in ["marginTop", "marginLeft", "marginRight", "marginBottom"] {
                    set(key, Value::from(0.0));
                } // for each side
            } // ignore margins
            "gap" => {
                if let Some(number) = number_or_problem(&mut problems) { set("pieceGap", number); }
            } // gap
            "mode" => set("layoutMode", Value::from(value)),
            "nap" => match value {
                "up" => { set("layoutMode", Value::from("withNap")); set("rotationStep", Value::from(0.0)); }
                "down" => { set("layoutMode", Value::from("withNap")); set("rotationStep", Value::from(180.0)); }
                _ => problems.push(format!("Layout flag nap: '{value}'; use up or down.")),
            }, // nap
            "no-grainline" => set("noGrainlineRotation", Value::from(value)),
            _ => {} // media and paper are set above
        } // match name
    } // for each flag

    if !problems.is_empty() {
        return Err(problems);
    } // if flag problems
    let merged = Value::Object(settings).to_string();
    validate_settings_json(&merged).map(|()| merged)
} // fn apply_layout_flags

#[cfg(test)]
mod tests {
    use super::*;

    fn flags(items: &[&str]) -> Vec<String> {
        items.iter().map(|item| item.to_string()).collect()
    } // fn flags

    fn merged(base: &str, items: &[&str]) -> Value {
        serde_json::from_str(&apply_layout_flags(base, &flags(items)).expect("flags apply")).expect("JSON")
    } // fn merged

    // A roll width selects a paper roll; a tile width selects a Custom tile on tiled paper.
    #[test]
    fn media_flags_imply_their_media() {
        let roll = merged(r#"{"mediaType":"fabric","paperType":"tiled"}"#, &["roll-width=24"]);
        assert_eq!((roll["mediaType"].as_str(), roll["paperType"].as_str()), (Some("paper"), Some("roll")));
        assert_eq!((roll["rollSize"].as_str(), roll["rollWidth"].as_f64()), (Some("custom"), Some(24.0)));
        let tile = merged("{}", &["tile-width=10", "tile-height=12"]);
        assert_eq!((tile["paperType"].as_str(), tile["tileSize"].as_str()), (Some("tiled"), Some("Custom")));
        let fabric = merged("{}", &["fabric-width=150", "fabric-folded=yes", "unit=cm"]);
        assert_eq!((fabric["mediaType"].as_str(), fabric["fabricFolded"].as_bool()), (Some("fabric"), Some(true)));
    } // media_flags_imply_their_media

    // A specific margin wins over margins; ignore-margins sets every side to 0.
    #[test]
    fn margins_and_nap() {
        let sides = merged("{}", &["margins=1", "margin-top=2"]);
        assert_eq!((sides["marginTop"].as_f64(), sides["marginLeft"].as_f64()), (Some(2.0), Some(1.0)));
        let none = merged(r#"{"marginTop":3}"#, &["ignore-margins="]);
        assert_eq!(none["marginTop"].as_f64(), Some(0.0));
        let nap = merged("{}", &["nap=down"]);
        assert_eq!((nap["layoutMode"].as_str(), nap["rotationStep"].as_f64()), (Some("withNap"), Some(180.0)));
    } // margins_and_nap

    // Conflicts, bad numbers, unknown flags and invalid values each give a message.
    #[test]
    fn conflicts_and_invalid_values_fail() {
        for items in [
            &["fabric-width=60", "paper=roll"][..],
            &["tile-size=A4", "roll-width=36"][..],
            &["tile-size=A4", "tile-width=10"][..],
            &["ignore-margins=", "margins=1"][..],
            &["nap=up", "mode=any"][..],
            &["gap=wide"][..],
            &["colour=red"][..],
            &["gap=1", "gap=2"][..],
            &["tile-orientation=Portrait"][..],
            &["unit=ft"][..],
        ] {
            assert!(apply_layout_flags("{}", &flags(items)).is_err(), "{items:?}");
        } // for each bad set
    } // conflicts_and_invalid_values_fail

    // Flags win over the settings file; other keys stay.
    #[test]
    fn flags_win_over_the_file() {
        let result = merged(r#"{"pieceGap":0.5,"unit":"in","scaleGrid":true}"#, &["gap=0.1"]);
        assert_eq!((result["pieceGap"].as_f64(), result["scaleGrid"].as_bool()), (Some(0.1), Some(true)));
    } // flags_win_over_the_file
} // mod tests
