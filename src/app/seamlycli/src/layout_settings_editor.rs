// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief Edit SeamlyLayout settings or display the selected settings file.

use std::path::{Path, PathBuf};
use iced::widget::{column, pick_list, row, text, text_input};
use iced::{Element, Length};
use serde_json::{Map, Value};

#[derive(Clone, Debug)]
pub enum Message {
    Edited(usize, String),
}

#[derive(Clone, Debug)]
/// @brief One layout setting and its editable value.
struct Field {
    group: &'static str,
    key: &'static str,
    label: &'static str,
    value: String,
    choices: &'static [(&'static str, &'static str)],
}

/// @brief Define a length in the selected unit.
fn number(group: &'static str, key: &'static str, label: &'static str, value: &str) -> Field {
    Field { group, key, label, value: value.to_string(), choices: &[] }
}

/// @brief Define a labelled selection with its JSON value.
fn choice(group: &'static str, key: &'static str, label: &'static str,
          choices: &'static [(&'static str, &'static str)], value: &str) -> Field {
    Field { group, key, label, value: value.to_string(), choices }
}

const YES_NO: &[(&str, &str)] = &[("Yes", "true"), ("No", "false")];

#[derive(Clone, Debug)]
/// @brief Layout settings shared by report queries and export commands.
pub struct LayoutSettingsEditor {
    fields: Vec<Field>,
}

impl Default for LayoutSettingsEditor {
    fn default() -> Self {
        Self { fields: vec![
            choice("Units and packing", "unit", "Unit",
                   &[("Inches", "in"), ("Centimetres", "cm"), ("Millimetres", "mm")], "in"),
            choice("Units and packing", "layoutMode", "Layout mode",
                   &[("Along grainline", "alongGrainline"), ("With nap", "withNap"), ("Any rotation", "any")],
                   "alongGrainline"),
            choice("Units and packing", "rotationStep", "Nap direction",
                   &[("Up", "0"), ("Down", "180")], "0"),
            choice("Units and packing", "noGrainlineRotation", "Without grainline",
                   &[("Upright", "upright"), ("Free rotation", "free")], "upright"),
            number("Units and packing", "pieceGap", "Piece gap", "0.05"),
            choice("Units and packing", "scaleGrid", "Scale grid", YES_NO, "false"),
            choice("Units and packing", "foldLinesOnLeftEdge", "Fold lines on left edge", YES_NO, "true"),
            choice("Media", "mediaType", "Media",
                   &[("Paper", "paper"), ("Fabric", "fabric")], "paper"),
            choice("Media", "paperType", "Paper",
                   &[("Tiled", "tiled"), ("Roll", "roll")], "tiled"),
            choice("Media", "tileSize", "Tile size",
                   &[("Letter", "Letter"), ("Legal", "Legal"), ("Ledger", "Ledger"),
                     ("ANSI C", "ANSI C"), ("ANSI D", "ANSI D"), ("ANSI E", "ANSI E"),
                     ("ARCH A", "ARCH A"), ("ARCH B", "ARCH B"), ("ARCH C", "ARCH C"),
                     ("ARCH D", "ARCH D"), ("ARCH E", "ARCH E"), ("ARCH E1", "ARCH E1"),
                     ("ARCH E2", "ARCH E2"), ("ARCH E3", "ARCH E3"),
                     ("A0", "A0"), ("A1", "A1"), ("A2", "A2"), ("A3", "A3"),
                     ("A4", "A4"), ("A5", "A5"), ("B0", "B0"), ("B1", "B1"),
                     ("B2", "B2"), ("B3", "B3"), ("B4", "B4"), ("B5", "B5"),
                     ("B6", "B6"), ("Custom", "Custom")], "Letter"),
            choice("Media", "tileOrientation", "Tile orientation",
                   &[("Landscape", "landscape"), ("Portrait", "portrait")], "landscape"),
            number("Media", "tileWidth", "Custom tile width", "8.5"),
            number("Media", "tileHeight", "Custom tile height", "11"),
            choice("Media", "rollSize", "Roll width",
                   &[("24 inches", "24 in"), ("30 inches", "30 in"), ("36 inches", "36 in"),
                     ("42 inches", "42 in"), ("48 inches", "48 in"), ("54 inches", "54 in"),
                     ("60 inches", "60 in"), ("900 millimetres", "900 mm"), ("Custom", "custom")], "36 in"),
            number("Media", "rollWidth", "Custom roll width", "36"),
            number("Media", "fabricWidth", "Fabric width", "0"),
            number("Media", "fabricHeight", "Fabric length (0: no end)", "0"),
            choice("Media", "fabricFolded", "Folded fabric", YES_NO, "false"),
            number("Media", "selvedgeWidth", "Selvedge width", "0"),
            number("Margins", "marginTop", "Top", "0.25"),
            number("Margins", "marginBottom", "Bottom", "0.25"),
            number("Margins", "marginLeft", "Left", "0.25"),
            number("Margins", "marginRight", "Right", "0.25"),
        ] }
    }
}

impl LayoutSettingsEditor {
    fn value(&self, key: &str) -> &str {
        self.fields.iter().find(|field| field.key == key).map(|field| field.value.as_str()).unwrap_or("")
    }

    fn visible(&self, key: &str) -> bool {
        let paper = self.value("mediaType") == "paper";
        let tiled = paper && self.value("paperType") == "tiled";
        let roll = paper && !tiled;
        match key {
            "rotationStep" => self.value("layoutMode") == "withNap",
            "paperType" => paper,
            "tileSize" | "tileOrientation" => tiled,
            "tileWidth" | "tileHeight" => tiled && self.value("tileSize") == "Custom",
            "rollSize" => roll,
            "rollWidth" => roll && self.value("rollSize") == "custom",
            "fabricWidth" | "fabricHeight" | "fabricFolded" | "selvedgeWidth" => !paper,
            "marginTop" | "marginBottom" | "marginLeft" | "marginRight" => paper,
            _ => true,
        }
    }

    /// @brief Ignore edits while a settings file supplies the layout settings.
    pub fn update(&mut self, message: Message, enabled: bool) {
        if enabled {
            let Message::Edited(index, value) = message;
            if self.fields.get(index).map(|field| field.key) == Some("unit") {
                let millimetres = |unit: &str| match unit { "in" => 25.4, "cm" => 10.0, _ => 1.0 };
                let factor = millimetres(self.value("unit")) / millimetres(&value);
                for field in &mut self.fields {
                    if field.choices.is_empty() {
                        if let Ok(length) = field.value.parse::<f64>() {
                            field.value = format!("{:.6}", length * factor).trim_end_matches('0').trim_end_matches('.').to_string();
                        }
                    }
                }
            }
            if let Some(field) = self.fields.get_mut(index) {
                field.value = value;
            }
        }
    }

    /// @brief Build JSON with the types and keys SeamlyLayout accepts.
    pub fn json(&self) -> Result<Value, String> {
        let mut values = Map::new();
        for field in &self.fields {
            let value = if field.choices.is_empty() || field.key == "rotationStep" {
                let number = field.value.trim().parse::<f64>()
                    .map_err(|_| format!("Enter a number for {}.", field.label))?;
                let positive = matches!(field.key, "tileWidth" | "tileHeight");
                if !number.is_finite() || number < 0.0 || (positive && number == 0.0) {
                    return Err(format!("Enter a valid positive value for {}.", field.label));
                }
                serde_json::json!(number)
            } else if matches!(field.key, "scaleGrid" | "foldLinesOnLeftEdge" | "fabricFolded") {
                Value::Bool(field.value == "true")
            } else {
                Value::String(field.value.clone())
            };
            values.insert(field.key.to_string(), value);
        }
        Ok(Value::Object(values))
    }

    /// @brief Save entered settings for the report and subsequent export commands.
    pub fn write(&self) -> Result<PathBuf, String> {
        let path = std::env::temp_dir().join(format!("seamlycli-layout-settings-{}.json", std::process::id()));
        let text = serde_json::to_string_pretty(&self.json()?).map_err(|error| error.to_string())?;
        std::fs::write(&path, text).map_err(|error| format!("Cannot write layout settings: {error}"))?;
        Ok(path)
    }

    /// @brief Display settings from a selected file without changing that file.
    pub fn read(path: &Path) -> Result<Self, String> {
        let text = std::fs::read_to_string(path).map_err(|error| error.to_string())?;
        let values: Value = serde_json::from_str(&text).map_err(|error| error.to_string())?;
        let object = values.as_object().ok_or("The layout settings must be a JSON object.")?;
        let mut editor = Self::default();
        for field in &mut editor.fields {
            if let Some(value) = object.get(field.key) {
                field.value = value.as_str().map(str::to_string).unwrap_or_else(|| value.to_string());
            }
        }
        Ok(editor)
    }

    /// @brief Show media controls that match the selected layout settings.
    pub fn view(&self, enabled: bool) -> Element<'_, Message> {
        let mut fields = column![text("Lengths use the selected unit.").size(13)].spacing(8);
        let mut group = "";
        for (index, field) in self.fields.iter().enumerate().filter(|(_, field)| self.visible(field.key)) {
            if group != field.group {
                group = field.group;
                fields = fields.push(text(group).size(18));
            }
            let input: Element<'_, Message> = if field.choices.is_empty() {
                text_input("", &field.value)
                    .on_input_maybe(enabled.then_some(move |value| Message::Edited(index, value)))
                    .width(Length::Fill).into()
            } else {
                let labels: Vec<&str> = field.choices.iter().map(|(label, _)| *label).collect();
                let selected = field.choices.iter().find(|(_, value)| *value == field.value)
                    .map(|(label, _)| *label).unwrap_or(&field.value);
                if enabled {
                    pick_list(labels, Some(selected), move |label: &str| {
                        let value = field.choices.iter().find(|(name, _)| *name == label).unwrap().1;
                        Message::Edited(index, value.to_string())
                    }).width(Length::Fill).into()
                } else {
                    text_input("", selected).width(Length::Fill).into()
                }
            };
            fields = fields.push(row![text(field.label).width(Length::Fixed(210.0)), input].spacing(8));
        }
        fields.into()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn defaults_use_letter_and_typed_settings() {
        let json = LayoutSettingsEditor::default().json().unwrap();
        assert_eq!(json["tileSize"], "Letter");
        assert_eq!(json["unit"], "in");
        assert!(json.get("pageWidth").is_none());
        assert!(json.get("pageHeight").is_none());
        assert_eq!(json["fabricFolded"], false);
    }

    #[test]
    fn selected_file_disables_edits_and_clear_restores_editing() {
        let mut editor = LayoutSettingsEditor::default();
        editor.update(Message::Edited(0, "cm".to_string()), false);
        assert_eq!(editor.value("unit"), "in");
        editor.update(Message::Edited(0, "cm".to_string()), true);
        assert_eq!(editor.value("unit"), "cm");
    }

    #[test]
    fn unit_changes_preserve_physical_lengths() {
        let mut editor = LayoutSettingsEditor::default();
        editor.update(Message::Edited(0, "mm".to_string()), true);
        let json = editor.json().unwrap();
        assert_eq!(json["rollWidth"], 914.4);
        assert_eq!(json["marginLeft"], 6.35);
        assert_eq!(json["rotationStep"], 0.0);
    }

    #[test]
    fn invalid_lengths_stop_the_run() {
        let mut editor = LayoutSettingsEditor::default();
        editor.fields.iter_mut().find(|field| field.key == "pieceGap").unwrap().value = "-1".to_string();
        assert!(editor.json().is_err());
    }

    #[test]
    fn custom_roll_and_file_settings_keep_their_values() {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("roll.json");
        std::fs::write(&path, r#"{"unit":"in","mediaType":"paper","paperType":"roll","rollSize":"custom","rollWidth":42.0}"#).unwrap();
        let editor = LayoutSettingsEditor::read(&path).unwrap();
        assert!(editor.visible("rollWidth"));
        assert!(!editor.visible("tileSize"));
        assert_eq!(editor.json().unwrap()["rollWidth"], 42.0);
    }
}
