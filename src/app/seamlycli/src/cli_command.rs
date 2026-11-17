// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief The Seamly2D command line of one material export.
//!
//! The argument list goes to `std::process::Command` item by item; no shell
//! parses it. `display_command` only shows it in a form a user can paste.

use std::path::{Path, PathBuf};

/// @brief DXF flavor; it selects the `--layout-export-options` JSON.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DxfVariant {
    /// DXF-ASTM, AutoCAD R13.
    AstmR13,
    /// DXF-ASTM R13 with CLO3D group 250 data.
    Clo3d,
    /// DXF-AAMA, AutoCAD R11.
    AamaR11,
}

impl DxfVariant {
    /// @brief Every variant, in menu order.
    pub const ALL: [DxfVariant; 3] = [DxfVariant::AstmR13, DxfVariant::Clo3d, DxfVariant::AamaR11];

    /// @brief Export options JSON of the variant.
    pub fn options_json(self) -> &'static str {
        match self {
            DxfVariant::AstmR13 => r#"{"dxfVersion":"R13"}"#,
            DxfVariant::Clo3d => r#"{"dxfVersion":"R13","clo3dGroup250":true}"#,
            DxfVariant::AamaR11 => r#"{"dxfVersion":"R11","aama":true}"#,
        } // match variant
    } // fn options_json
} // impl DxfVariant

impl std::fmt::Display for DxfVariant {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        formatter.write_str(match self {
            DxfVariant::AstmR13 => "DXF-ASTM (R13)",
            DxfVariant::Clo3d => "DXF-ASTM (CLO3D)",
            DxfVariant::AamaR11 => "DXF-AAMA (R11)",
        })
    } // fn fmt
} // impl Display for DxfVariant

/// @brief Every choice of one export session, shared by all materials.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct ExportRequest {
    pub seamly2d: PathBuf,
    pub pattern: PathBuf,
    pub measurements: Option<PathBuf>,
    pub settings: Option<PathBuf>,
    /// Multisize view label; None for an individual pattern.
    pub view: Option<String>,
    /// Split pieces wider than the media.
    pub split: bool,
    pub format: String,
    /// SVG text mode; None keeps the default for the labels.
    pub svg_text_mode: Option<String>,
    /// File with the DXF export options; None for formats other than DXF.
    pub export_options: Option<PathBuf>,
    /// True when the pattern has several materials: each run names its material.
    pub several_materials: bool,
}

impl ExportRequest {
    /// @brief Argument list of the export of `material` to `output`.
    /// @param material Material name from the report; empty when the handoff names none.
    /// @param output Output file path.
    pub fn arguments(&self, material: &str, output: &Path) -> Vec<String> {
        let mut arguments = vec![
            "--layout-export".to_string(),
            "--layout-format".to_string(),
            self.format.clone(),
            "--layout-output".to_string(),
            output.display().to_string(),
        ];
        if self.view.is_some() {
            arguments.push("--basesize".to_string());
        } // if multisize measurements
        let mut add = |option: &str, value: String| {
            arguments.push(option.to_string());
            arguments.push(value);
        }; // add
        if let Some(measurements) = &self.measurements {
            add("--mfile", measurements.display().to_string());
        } // if measurements
        if let Some(settings) = &self.settings {
            add("--layout-settings", settings.display().to_string());
        } // if settings
        if self.several_materials && !material.is_empty() {
            add("--layout-material", material.to_string());
        } // if a material must be named
        if let Some(view) = &self.view {
            add("--layout-view", view.clone());
        } // if multisize view
        if self.split {
            add("--layout-oversize", "split".to_string());
        } // if pieces are split
        if let (Some(mode), "svg") = (&self.svg_text_mode, self.format.as_str()) {
            add("--svg-text-mode", mode.clone());
        } // if SVG text mode
        if let Some(options) = &self.export_options {
            add("--layout-export-options", options.display().to_string());
        } // if export options
        arguments.push(self.pattern.display().to_string());
        arguments
    } // fn arguments
} // impl ExportRequest

/// @brief Quote one argument for display as Windows shells read it (CommandLineToArgvW rules).
pub fn quote_argument(argument: &str) -> String {
    if !argument.is_empty() && !argument.chars().any(|c| c.is_whitespace() || c == '"') {
        return argument.to_string();
    } // if no quoting needed
    let mut quoted = String::from('"');
    let mut backslashes = 0;
    for character in argument.chars() {
        match character {
            '\\' => backslashes += 1,
            '"' => {
                quoted.push_str(&"\\".repeat(backslashes * 2 + 1));
                quoted.push('"');
                backslashes = 0;
            } // quote
            other => {
                quoted.push_str(&"\\".repeat(backslashes));
                quoted.push(other);
                backslashes = 0;
            } // other
        } // match character
    } // for each character
    quoted.push_str(&"\\".repeat(backslashes * 2));
    quoted.push('"');
    quoted
} // fn quote_argument

/// @brief The command as one line to read and copy.
pub fn display_command(program: &Path, arguments: &[String]) -> String {
    std::iter::once(quote_argument(&program.display().to_string()))
        .chain(arguments.iter().map(|argument| quote_argument(argument)))
        .collect::<Vec<_>>()
        .join(" ")
} // fn display_command

#[cfg(test)]
mod tests {
    use super::*;

    fn request() -> ExportRequest {
        ExportRequest {
            seamly2d: PathBuf::from("seamly2d.exe"),
            pattern: PathBuf::from("C:/patterns/jacket 2.sm2d"),
            measurements: Some(PathBuf::from("m.smms")),
            settings: Some(PathBuf::from("roll.json")),
            view: Some("Size 40".to_string()),
            split: true,
            format: "svg".to_string(),
            svg_text_mode: Some("hersheyStrokes".to_string()),
            export_options: None,
            several_materials: true,
        }
    } // fn request

    // Every choice becomes its option; the pattern is last.
    #[test]
    fn arguments_carry_every_choice() {
        let arguments = request().arguments("Lining", Path::new("out/j_Lining.svg"));
        assert_eq!(arguments, vec![
            "--layout-export", "--layout-format", "svg", "--layout-output", "out/j_Lining.svg", "--basesize",
            "--mfile", "m.smms", "--layout-settings", "roll.json", "--layout-material", "Lining",
            "--layout-view", "Size 40", "--layout-oversize", "split", "--svg-text-mode", "hersheyStrokes",
            "C:/patterns/jacket 2.sm2d",
        ]);
    } // arguments_carry_every_choice

    // One material needs no material option; the text mode applies to SVG only.
    #[test]
    fn single_material_pdf_omits_material_and_text_mode() {
        let mut request = request();
        request.several_materials = false;
        request.format = "pdf".to_string();
        let arguments = request.arguments("Fabric", Path::new("o.pdf"));
        assert!(!arguments.contains(&"--layout-material".to_string()));
        assert!(!arguments.contains(&"--svg-text-mode".to_string()));
    } // single_material_pdf_omits_material_and_text_mode

    #[test]
    fn individual_pattern_omits_base_size() {
        let mut request = request();
        request.view = None;
        assert!(!request.arguments("Fabric", Path::new("o.svg")).contains(&"--basesize".to_string()));
    }

    // Spaces and quotes are quoted the way the program reads them back.
    #[test]
    fn display_quotes_spaces_and_quotes() {
        assert_eq!(quote_argument("plain"), "plain");
        assert_eq!(quote_argument("two words"), "\"two words\"");
        assert_eq!(quote_argument(r#"say "hi""#), r#""say \"hi\"""#);
        assert_eq!(quote_argument(r"C:\dir with space\"), r#""C:\dir with space\\""#);
        assert_eq!(display_command(Path::new("s.exe"), &["a b".to_string()]), "s.exe \"a b\"");
    } // display_quotes_spaces_and_quotes
} // mod tests
