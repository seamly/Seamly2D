// project: Seamly2D
// author: slspencer, copyright 2026 Seamly2D Project
// GPL-3.0-or-later

//! @brief The `seamly2d --layout-info` report and the choices it allows.
//!
//! SeamlyLayout computes the report from the real piece geometry and the layout
//! settings (`export_rules.rs`). This module only reads it: no layout rule is
//! repeated here, so the choices follow the CLI.

use std::path::Path;
use std::process::Command;

use serde::Deserialize;

/// @brief One layout (view) of a material and the formats it allows.
#[derive(Clone, Debug, Deserialize, PartialEq)]
pub struct LayoutChoice {
    /// Tab label, such as "Marker" or "Size 40"; empty for an individual layout.
    pub view: String,
    /// Export format names, such as "svg" or "pdf-tiled".
    pub formats: Vec<String>,
}

/// @brief One piece wider than the media.
#[derive(Clone, Debug, Deserialize, PartialEq)]
#[serde(rename_all = "camelCase")]
pub struct OversizedPiece {
    pub name: String,
    pub width_px: f64,
    pub media_width_px: f64,
}

/// @brief One material of the pattern.
#[derive(Clone, Debug, Deserialize, PartialEq)]
#[serde(rename_all = "camelCase")]
pub struct MaterialReport {
    /// Material name; empty when the handoff names none.
    pub name: String,
    pub oversized_pieces: Vec<OversizedPiece>,
    /// Layouts of a run that keeps pieces whole.
    pub whole: Vec<LayoutChoice>,
    /// Layouts of a run that splits oversized pieces; empty on tiled paper.
    pub split: Vec<LayoutChoice>,
}

/// @brief The whole `--layout-info` report.
#[derive(Clone, Debug, Deserialize, PartialEq)]
#[serde(rename_all = "camelCase")]
pub struct LayoutReport {
    pub multisize: bool,
    pub sizes: Vec<String>,
    pub tiled_paper: bool,
    pub label_text: String,
    pub svg_text_modes: Vec<String>,
    pub materials: Vec<MaterialReport>,
}

/// @brief A choice the report does not allow, with the reason shown to the user.
#[derive(Clone, Debug, PartialEq)]
pub struct Unavailable {
    pub name: String,
    pub reason: String,
}

/// @brief Every format name of headless export, in Export menu order.
pub const ALL_FORMATS: &[&str] =
    &["svg", "pdf", "pdf-tiled", "png", "jpg", "jpeg", "dxf", "hpgl", "ps", "eps", "gcode", "3mf"];

impl LayoutReport {
    /// @brief Parse the JSON text that `--layout-info` prints.
    pub fn parse(json: &str) -> Result<Self, String> {
        serde_json::from_str(json.trim()).map_err(|error| format!("The layout report is not valid: {error}"))
    } // fn parse

    /// @brief True when a piece of any material is wider than the media: the export must split it.
    pub fn needs_split(&self) -> bool {
        self.materials.iter().any(|material| !material.oversized_pieces.is_empty())
    } // fn needs_split

    /// @brief Layouts of the run, of the first material (every material has the same views).
    pub fn layouts(&self, split: bool) -> &[LayoutChoice] {
        match self.materials.first() {
            Some(material) if split => &material.split,
            Some(material) => &material.whole,
            None => &[],
        } // match first material
    } // fn layouts

    /// @brief View names to offer in the Layout dropdown.
    pub fn view_choices(&self, split: bool) -> Vec<String> {
        self.layouts(split).iter().map(|layout| layout.view.clone()).collect()
    } // fn view_choices

    /// @brief Views of the whole run that the split run does not have, with the reason.
    pub fn unavailable_views(&self, split: bool) -> Vec<Unavailable> {
        if !split {
            return Vec::new();
        } // if pieces stay whole
        let offered = self.view_choices(true);
        self.view_choices(false)
            .into_iter()
            .filter(|view| !offered.contains(view))
            .map(|view| Unavailable { name: view, reason: "unavailable when pieces are split".to_string() })
            .collect()
    } // fn unavailable_views

    /// @brief Formats that every material allows for `view`.
    pub fn format_choices(&self, view: &str, split: bool) -> Vec<String> {
        ALL_FORMATS
            .iter()
            .filter(|format| {
                !self.materials.is_empty()
                    && self.materials.iter().all(|material| {
                        let layouts = if split { &material.split } else { &material.whole };
                        layouts.iter().any(|layout| layout.view == view && layout.formats.iter().any(|f| f == *format))
                    })
            })
            .map(|format| format.to_string())
            .collect()
    } // fn format_choices

    /// @brief Formats that `view` does not allow, each with a short reason built from the report facts.
    pub fn unavailable_formats(&self, view: &str, split: bool) -> Vec<Unavailable> {
        let allowed = self.format_choices(view, split);
        ALL_FORMATS
            .iter()
            .filter(|format| !allowed.iter().any(|a| a == *format))
            .map(|format| Unavailable { name: format.to_string(), reason: self.format_reason(format, view, split) })
            .collect()
    } // fn unavailable_formats

    /// @brief Reason text for a format the report leaves out.
    fn format_reason(&self, format: &str, view: &str, split: bool) -> String {
        let reason = match format {
            "pdf" if self.tiled_paper => "tiled paper exports as PDF (tiled)",
            "pdf-tiled" if !self.tiled_paper => "needs Paper Type: Tiled",
            "pdf-tiled" | "dxf" | "gcode" | "3mf" if split => "unavailable for split layouts",
            "pdf-tiled" if view == "Marker" => "unavailable for the Marker layout",
            "dxf" if view == "Marker" || view == "Nested" => "exports one size: choose Base Size or a size",
            "gcode" | "3mf" => "module not available in this build",
            _ => "not allowed for this layout",
        }; // match format
        reason.to_string()
    } // fn format_reason
} // impl LayoutReport

/// @brief Run `seamly2d --layout-info` and parse its report.
/// @param seamly2d Seamly2D executable.
/// @param pattern Pattern file.
/// @param measurements Optional measurement file (`--mfile`).
/// @param settings Optional layout settings JSON (`--layout-settings`).
/// @return The report, or the error text of the run.
pub fn query_report(seamly2d: &Path, pattern: &Path, measurements: Option<&Path>, settings: Option<&Path>)
    -> Result<LayoutReport, String> {
    let mut arguments: Vec<String> = vec!["--layout-info".to_string()];
    if let Some(measurements) = measurements {
        arguments.extend(["--mfile".to_string(), measurements.display().to_string()]);
    } // if measurements
    if let Some(settings) = settings {
        arguments.extend(["--layout-settings".to_string(), settings.display().to_string()]);
    } // if settings
    arguments.push(pattern.display().to_string());
    let output = Command::new(seamly2d)
        .args(&arguments)
        .output()
        .map_err(|error| format!("{} could not be started: {error}", seamly2d.display()))?;
    if !output.status.success() {
        return Err(format!(
            "seamly2d --layout-info failed (exit {}):\n{}",
            output.status.code().unwrap_or(-1),
            String::from_utf8_lossy(&output.stderr).trim()
        ));
    } // if the query failed
    LayoutReport::parse(&String::from_utf8_lossy(&output.stdout))
} // fn query_report

#[cfg(test)]
mod tests {
    use super::*;

    // A report as SeamlyLayout prints it for a multisize pattern on roll paper with one oversized piece.
    const MULTISIZE: &str = r#"{"labelText":"text","materials":[
        {"name":"Fabric","oversizedPieces":[{"name":"Back","widthPx":1200,"mediaWidthPx":1100}],
         "whole":[{"formats":["svg","pdf","png"],"view":"Marker"},{"formats":["svg","pdf","dxf"],"view":"Size 40"}],
         "split":[{"formats":["svg","pdf"],"view":"Size 40"}]},
        {"name":"Lining","oversizedPieces":[],
         "whole":[{"formats":["svg","png"],"view":"Marker"},{"formats":["svg","pdf","dxf"],"view":"Size 40"}],
         "split":[{"formats":["svg","pdf"],"view":"Size 40"}]}],
        "multisize":true,"sizes":["40"],"svgTextModes":["designerFont","asSupplied"],"tiledPaper":false}"#;

    // An oversized piece in any material asks for the split run, which has no Marker.
    #[test]
    fn oversized_piece_selects_split_and_hides_marker() {
        let report = LayoutReport::parse(MULTISIZE).unwrap();
        assert!(report.needs_split());
        assert_eq!(report.view_choices(true), vec!["Size 40"]);
        assert_eq!(report.unavailable_views(true), vec![Unavailable {
            name: "Marker".to_string(),
            reason: "unavailable when pieces are split".to_string()
        }]);
        assert!(report.unavailable_views(false).is_empty());
    } // oversized_piece_selects_split_and_hides_marker

    // A format is offered only when every material allows it for the view.
    #[test]
    fn formats_need_every_material() {
        let report = LayoutReport::parse(MULTISIZE).unwrap();
        assert_eq!(report.format_choices("Marker", false), vec!["svg", "png"]);
        assert_eq!(report.format_choices("Size 40", false), vec!["svg", "pdf", "dxf"]);
        assert_eq!(report.format_choices("Size 40", true), vec!["svg", "pdf"]);
        let unavailable = report.unavailable_formats("Size 40", true);
        let dxf = unavailable.iter().find(|item| item.name == "dxf").unwrap();
        assert_eq!(dxf.reason, "unavailable for split layouts");
        let tiled = unavailable.iter().find(|item| item.name == "pdf-tiled").unwrap();
        assert_eq!(tiled.reason, "needs Paper Type: Tiled");
    } // formats_need_every_material

    // Text that is not the report gives an error, not a panic.
    #[test]
    fn invalid_report_is_an_error() {
        assert!(LayoutReport::parse("INFO: not json").is_err());
    } // invalid_report_is_an_error
} // mod tests
