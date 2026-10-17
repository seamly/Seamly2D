// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file scale_grid.rs
// @brief A scale check grid for a printed layout.
//
// The user measures the grid on a print to check that it printed at 100 %.
// Metric settings (mm, cm) get 3 x 3 squares of 1 cm; inch settings get
// 2 x 2 squares of 1 in. The grid goes below the lowest piece, after every
// piece is placed. Marker layouts get none. DXF exports and HPGL cut files
// leave it out.

use xmltree::{Element, XMLNode};

// @brief `data-type` and `id` of the grid group.
pub(crate) const SCALE_GRID_TYPE: &str = "scale_grid";

// Layout pixels per inch and per centimetre.
const PX_PER_IN: f64 = 96.0;
const PX_PER_CM: f64 = PX_PER_IN / 2.54;
// Label text size and the space between the grid and its label, in pixels.
const LABEL_FONT_PX: f64 = 14.0;
const LABEL_GAP_PX: f64 = 4.0;

// @brief True when the settings JSON asks for the grid (`"scaleGrid": true`).
pub(crate) fn scale_grid_requested(settings_json: &str) -> bool {
    serde_json::from_str::<serde_json::Value>(settings_json)
        .ok()
        .and_then(|v| v.get("scaleGrid").and_then(serde_json::Value::as_bool))
        .unwrap_or(false)
} // fn scale_grid_requested

// @brief Square grid of `cells` x `cells` squares, each `cell_px` wide, with a size label.
#[derive(Debug, Clone, PartialEq)]
pub(crate) struct ScaleGrid {
    cells: u32,
    cell_px: f64,
    label: String,
}

impl ScaleGrid {
    // @brief The grid for the settings unit: "in" gives 2 x 2 in; "mm" and "cm" give 3 x 3 cm.
    pub(crate) fn for_unit(unit: &str) -> Self {
        if unit == "in" {
            Self { cells: 2, cell_px: PX_PER_IN, label: "Scale check: 2 in × 2 in".to_string() }
        } else {
            Self { cells: 3, cell_px: PX_PER_CM, label: "Scale check: 3 cm × 3 cm".to_string() }
        } // if inches
    } // fn for_unit

    // @brief Side of the grid in pixels.
    fn side_px(&self) -> f64 {
        self.cells as f64 * self.cell_px
    } // fn side_px

    // @brief Height of the grid and its label, in whole pixels.
    pub(crate) fn height_px(&self) -> u32 {
        (self.side_px() + LABEL_GAP_PX + LABEL_FONT_PX).ceil() as u32
    } // fn height_px

    // @brief The grid group, its top-left corner at (`x`, `y`) in layout pixels.
    pub(crate) fn element(&self, x: f64, y: f64) -> Element {
        let side = self.side_px();
        // Outer square, then every inner line across and down.
        let mut d = format!("M{},{} L{},{} L{},{} L{},{} Z", fmt(x), fmt(y), fmt(x + side), fmt(y),
                            fmt(x + side), fmt(y + side), fmt(x), fmt(y + side));
        for i in 1..self.cells {
            let offset = i as f64 * self.cell_px;
            d.push_str(&format!(" M{},{} L{},{}", fmt(x + offset), fmt(y), fmt(x + offset), fmt(y + side)));
            d.push_str(&format!(" M{},{} L{},{}", fmt(x), fmt(y + offset), fmt(x + side), fmt(y + offset)));
        } // for inner line

        let mut path = Element::new("path");
        for (key, value) in [("d", d), ("fill", "none".into()), ("stroke", "#000000".into()), ("stroke-width", "1".into())] {
            path.attributes.insert(key.to_string(), value);
        } // for attribute

        let mut text = Element::new("text");
        for (key, value) in [
            ("x", fmt(x)),
            ("y", fmt(y + side + LABEL_GAP_PX + LABEL_FONT_PX * 0.8)),
            ("font-family", "Arial".to_string()),
            ("font-size", fmt(LABEL_FONT_PX)),
            ("fill", "#000000".to_string()),
            ("stroke", "none".to_string()),
        ] {
            text.attributes.insert(key.to_string(), value);
        } // for attribute
        text.children.push(XMLNode::Text(self.label.clone()));

        let mut group = Element::new("g");
        group.attributes.insert("id".to_string(), SCALE_GRID_TYPE.to_string());
        group.attributes.insert("data-type".to_string(), SCALE_GRID_TYPE.to_string());
        group.children.push(XMLNode::Element(path));
        group.children.push(XMLNode::Element(text));
        group
    } // fn element
} // impl ScaleGrid

// @brief Remove the grid from an export copy (DXF leaves it out).
// @return True when a grid was removed.
pub(crate) fn remove_scale_grid(doc: &mut svg_dom::Document) -> bool {
    let before = doc.root.children.len();
    doc.root.children.retain(|node| {
        !matches!(node, XMLNode::Element(e) if e.attributes.get("data-type").is_some_and(|t| t == SCALE_GRID_TYPE))
    }); // retain
    doc.root.children.len() != before
} // fn remove_scale_grid

// @brief A coordinate with at most three decimals.
fn fmt(value: f64) -> String {
    let text = format!("{value:.3}");
    text.trim_end_matches('0').trim_end_matches('.').to_string()
} // fn fmt

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Inches give 2 x 2 squares of 96 px; metric units give 3 x 3 squares of 1 cm.
    #[test]
    fn grid_size_follows_the_unit() {
        let inches = ScaleGrid::for_unit("in");
        assert_eq!((inches.cells, inches.side_px()), (2, 192.0));
        for unit in ["cm", "mm"] {
            let metric = ScaleGrid::for_unit(unit);
            assert_eq!(metric.cells, 3);
            assert!((metric.side_px() - 3.0 * 96.0 / 2.54).abs() < 1e-9, "{unit}");
        } // for unit
    } // grid_size_follows_the_unit

    // @brief The grid path holds the outer square and the inner lines; the label names the size.
    #[test]
    fn grid_element_draws_every_square() {
        let group = ScaleGrid::for_unit("in").element(10.0, 20.0);
        assert_eq!(group.attributes.get("data-type").map(String::as_str), Some(SCALE_GRID_TYPE));
        let path = group.children.iter().filter_map(|n| n.as_element()).find(|e| e.name == "path").expect("path");
        let d = &path.attributes["d"];
        assert!(d.starts_with("M10,20 L202,20 L202,212 L10,212 Z"), "{d}");
        assert!(d.contains("M106,20 L106,212") && d.contains("M10,116 L202,116"), "{d}");
        let text = group.children.iter().filter_map(|n| n.as_element()).find(|e| e.name == "text").expect("text");
        assert_eq!(text.get_text().as_deref(), Some("Scale check: 2 in × 2 in"));
        assert_eq!(ScaleGrid::for_unit("in").height_px(), (192.0 + LABEL_GAP_PX + LABEL_FONT_PX).ceil() as u32);
    } // grid_element_draws_every_square

    // @brief Only `"scaleGrid": true` asks for the grid.
    #[test]
    fn settings_ask_for_the_grid() {
        assert!(scale_grid_requested(r#"{"unit":"in","scaleGrid":true}"#));
        assert!(!scale_grid_requested(r#"{"unit":"in","scaleGrid":false}"#));
        assert!(!scale_grid_requested(r#"{"unit":"in"}"#));
        assert!(!scale_grid_requested("not json"));
    } // settings_ask_for_the_grid

    // @brief The DXF export copy loses the grid and keeps everything else.
    #[test]
    fn remove_scale_grid_keeps_the_pieces() {
        let mut doc = svg_dom::Document::parse(r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="p" data-type="piece"/></svg>"#).expect("valid SVG");
        doc.root.children.push(XMLNode::Element(ScaleGrid::for_unit("cm").element(0.0, 0.0)));
        assert!(remove_scale_grid(&mut doc));
        assert!(!remove_scale_grid(&mut doc));
        assert_eq!(doc.root.children.iter().filter_map(|n| n.as_element()).count(), 1);
    } // remove_scale_grid_keeps_the_pieces
} // mod tests
