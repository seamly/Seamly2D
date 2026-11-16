// project: SeamlyLayout
// author: slspencer, copyright 2026 Seamly2D Project
// MIT License: https://opensource.org/licenses/MIT

// @file export_rules.rs
// @brief Which export formats a layout allows, for headless export and its `--info` report.
//
// The Export menu disables the same formats through QML bindings. Headless
// export has no menu, so it asks `format_problem` before each export, and the
// `--info` report lists the allowed formats of every layout a caller can choose.
//
// Rules (`format_problem`):
// | Format            | Refused when                                                     |
// |-------------------|------------------------------------------------------------------|
// | pdf               | tiled paper (use pdf-tiled)                                      |
// | pdf-tiled         | not tiled paper; split layout; Marker view                       |
// | dxf               | split layout; Marker or Nested view                              |
// | gcode, 3mf        | split layout; module not available in this build                 |
// | any other format  | unknown name                                                     |

use serde_json::{json, Value};

use crate::layout_views::{queued_kinds, size_kinds, ViewKind};
use crate::piece_divider::OversizedPiece;

// @brief Every format name headless export accepts, in Export menu order.
pub const EXPORT_FORMATS: &[&str] =
    &["svg", "pdf", "pdf-tiled", "png", "jpg", "jpeg", "dxf", "hpgl", "ps", "eps", "gcode", "3mf"];

// @brief The layout facts that decide which formats an export may use.
#[derive(Clone, Copy, Debug)]
pub struct ExportContext<'a> {
    // View of a multisize layout; None for an individual layout.
    pub view: Option<&'a ViewKind>,
    // Paper media printed on tiled pages.
    pub tiled_paper: bool,
    // The layout split pieces wider than the media.
    pub split: bool,
    // The paid G-Code module is usable.
    pub gcode_available: bool,
    // The paid 3MF module is usable.
    pub mesh_available: bool,
}

// @brief Why `format` cannot export a layout of `context`; None when it can.
// @param format Lower-case format name, e.g. "pdf-tiled".
// @param context Facts of the layout to export.
// @return A sentence that names the cause and the alternative.
pub fn format_problem(format: &str, context: &ExportContext) -> Option<String> {
    let view_label = context.view.map(ViewKind::label).unwrap_or_default();
    match format {
        "pdf" if context.tiled_paper => {
            Some("pdf does not export tiled paper; use pdf-tiled, or set paperType to roll.".to_string())
        } // pdf on tiled paper
        "pdf-tiled" if !context.tiled_paper => Some(
            "pdf-tiled needs tiled paper: set mediaType to paper and paperType to tiled.".to_string(),
        ), // pdf-tiled off tiled paper
        "pdf-tiled" if context.split => Some("pdf-tiled does not export split layouts; use pdf.".to_string()),
        "pdf-tiled" if context.view.is_some_and(|view| !view.exports_pdf_tiled()) => {
            Some(format!("pdf-tiled does not export the {view_label} layout; select another view."))
        } // Marker view
        "dxf" if context.split => {
            Some("dxf does not export split layouts; use the oversize choice extend.".to_string())
        } // split dxf
        "dxf" if context.view.is_some_and(|view| !view.exports_dxf()) => Some(format!(
            "dxf exports one size; the {view_label} layout holds every size. Select the Base Size or a Size view."
        )), // Marker or Nested dxf
        "gcode" | "3mf" if context.split => Some(format!("{format} does not export split layouts.")),
        "gcode" if !context.gcode_available => Some("G-Code export is not available in this build.".to_string()),
        "3mf" if !context.mesh_available => Some("3MF export is not available in this build.".to_string()),
        known if EXPORT_FORMATS.contains(&known) => None, // allowed
        unknown => Some(format!("Unknown export format '{unknown}'. Supported formats: {}.", EXPORT_FORMATS.join(", "))),
    } // match format
} // fn format_problem

// @brief Formats of `EXPORT_FORMATS` that `context` allows, in menu order.
pub fn allowed_formats(context: &ExportContext) -> Vec<&'static str> {
    EXPORT_FORMATS.iter().copied().filter(|format| format_problem(format, context).is_none()).collect()
} // fn allowed_formats

// @brief Facts of one material for the `--info` report.
pub struct MaterialFacts<'a> {
    // Material name; empty when the handoff names none.
    pub name: &'a str,
    // `data-base-size` of the material's pattern group; empty for an individual pattern.
    pub base_size: &'a str,
    // Pieces wider than the media with the given settings.
    pub oversized: &'a [OversizedPiece],
}

// @brief Build the `--info` JSON report of an import and its settings.
//
// For each material: its oversized pieces and two lists of layouts with their formats.
// "whole" is the layout run that keeps pieces whole (as is, or media made longer);
// "split" is the run that splits oversized pieces. A multisize run lists its views
// in tab order; an individual run has one layout with an empty view name.
//
// @param sizes Sizes of a multisize import in handoff order; empty for an individual import.
// @param tiled_paper The settings select paper on tiled pages.
// @param label_text `label_text_state` of the import: "text", "pathsOnly" or "noLabels".
// @param materials One entry per material, in material tab order.
// @param paid (G-Code available, 3MF available).
// @return JSON text on one line.
pub fn layout_info_json(
    sizes: &[String],
    tiled_paper: bool,
    label_text: &str,
    materials: &[MaterialFacts],
    paid: (bool, bool),
) -> String {
    let report_materials: Vec<Value> = materials
        .iter()
        .map(|material| {
            let views_of = |split: bool| -> Vec<Option<ViewKind>> {
                if sizes.is_empty() {
                    vec![None] // individual: one layout
                } else if split || tiled_paper {
                    size_kinds(sizes, material.base_size).into_iter().map(Some).collect()
                } else {
                    std::iter::once(ViewKind::Marker)
                        .chain(queued_kinds(sizes, material.base_size))
                        .map(Some)
                        .collect()
                } // if multisize
            }; // views_of
            let layouts = |split: bool| -> Vec<Value> {
                views_of(split)
                    .iter()
                    .map(|view| {
                        let context = ExportContext {
                            view: view.as_ref(),
                            tiled_paper,
                            split,
                            gcode_available: paid.0,
                            mesh_available: paid.1,
                        }; // context
                        json!({
                            "view": view.as_ref().map(ViewKind::label).unwrap_or_default(),
                            "formats": allowed_formats(&context),
                        })
                    })
                    .collect()
            }; // layouts
            let oversized: Vec<Value> = material
                .oversized
                .iter()
                .map(|piece| json!({ "name": piece.label, "widthPx": piece.width_px, "mediaWidthPx": piece.media_width_px }))
                .collect();
            // Tiled paper finds no oversized piece, so it has no split run.
            let split_layouts = if tiled_paper { Vec::new() } else { layouts(true) };
            json!({
                "name": material.name,
                "oversizedPieces": oversized,
                "whole": layouts(false),
                "split": split_layouts,
            })
        })
        .collect();

    let report = json!({
        "multisize": !sizes.is_empty(),
        "sizes": sizes,
        "tiledPaper": tiled_paper,
        "labelText": label_text,
        "svgTextModes": svg_text_modes(label_text),
        "materials": report_materials,
    });
    serde_json::to_string(&report).unwrap_or_default()
} // fn layout_info_json

// @brief SVG text modes that the import's labels allow, as in the Export menu.
// Labels that are already paths take only "asSupplied"; text labels take the three conversions.
pub fn svg_text_modes(label_text: &str) -> Vec<&'static str> {
    if label_text == "pathsOnly" {
        vec!["asSupplied"]
    } else {
        vec!["designerFont", "singleLineFont", "hersheyStrokes", "asSupplied"]
    } // if labels are paths
} // fn svg_text_modes

#[cfg(test)]
mod tests {
    use super::*;

    fn context(view: Option<&ViewKind>, tiled_paper: bool, split: bool) -> ExportContext<'_> {
        ExportContext { view, tiled_paper, split, gcode_available: false, mesh_available: false }
    } // fn context

    // An individual roll layout takes every free format; tiled paper swaps pdf for pdf-tiled.
    #[test]
    fn individual_layout_formats_follow_the_paper() {
        assert_eq!(
            allowed_formats(&context(None, false, false)),
            vec!["svg", "pdf", "png", "jpg", "jpeg", "dxf", "hpgl", "ps", "eps"]
        );
        assert_eq!(
            allowed_formats(&context(None, true, false)),
            vec!["svg", "pdf-tiled", "png", "jpg", "jpeg", "dxf", "hpgl", "ps", "eps"]
        );
    } // individual_layout_formats_follow_the_paper

    // A split layout refuses dxf; Marker refuses dxf and pdf-tiled; Nested refuses dxf.
    #[test]
    fn split_and_all_size_views_refuse_dxf() {
        assert!(format_problem("dxf", &context(None, false, true)).is_some());
        let marker = ViewKind::Marker;
        let nested = ViewKind::Nested;
        let size = ViewKind::Size("40".to_string());
        assert!(format_problem("dxf", &context(Some(&marker), false, false)).is_some());
        assert!(format_problem("pdf-tiled", &context(Some(&marker), true, false)).is_some());
        assert!(format_problem("dxf", &context(Some(&nested), false, false)).is_some());
        assert!(format_problem("pdf-tiled", &context(Some(&nested), true, false)).is_none());
        assert!(format_problem("dxf", &context(Some(&size), false, false)).is_none());
    } // split_and_all_size_views_refuse_dxf

    // Paid formats need their module; unknown names list the supported formats.
    #[test]
    fn paid_and_unknown_formats_are_refused() {
        assert!(format_problem("gcode", &context(None, false, false)).unwrap().contains("not available"));
        assert!(format_problem("3mf", &context(None, false, false)).unwrap().contains("not available"));
        assert!(format_problem("bmp", &context(None, false, false)).unwrap().contains("Supported formats"));
    } // paid_and_unknown_formats_are_refused

    // A multisize report lists Marker, Nested, Base Size and sizes whole, and sizes only when split.
    #[test]
    fn multisize_report_lists_views_per_run() {
        let sizes = vec!["40".to_string(), "42".to_string()];
        let material = MaterialFacts { name: "Fabric", base_size: "42", oversized: &[] };
        let report: Value =
            serde_json::from_str(&layout_info_json(&sizes, false, "text", &[material], (false, false))).unwrap();
        let views = |run: &str| -> Vec<String> {
            report["materials"][0][run].as_array().unwrap().iter()
                .map(|layout| layout["view"].as_str().unwrap().to_string()).collect()
        }; // views
        assert_eq!(views("whole"), vec!["Marker", "Nested", "Base Size 42", "Size 40", "Size 42"]);
        assert_eq!(views("split"), vec!["Base Size 42", "Size 40", "Size 42"]);
        assert_eq!(report["materials"][0]["split"][0]["formats"].as_array().unwrap().len(), 8);
    } // multisize_report_lists_views_per_run

    // Labels that are already paths allow only asSupplied.
    #[test]
    fn path_labels_allow_only_as_supplied() {
        assert_eq!(svg_text_modes("pathsOnly"), vec!["asSupplied"]);
        assert_eq!(svg_text_modes("text").len(), 4);
    } // path_labels_allow_only_as_supplied
} // mod tests
