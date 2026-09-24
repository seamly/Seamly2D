// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file lib.rs
// @brief Rewrites label text in an exported layout SVG in one of three text modes.
//
// Seamly2D writes each label line as a `<text>` inside a
// `<g data-type="piece_label">` or `<g data-type="pattern_label">` group.
// This crate changes only those `<text>` elements. Group ids, `data-*`
// attributes and the group structure stay the same in every mode.
//
// | Mode             | `<text>`                         | Added to the document            |
// | ---------------- | -------------------------------- | -------------------------------- |
// | `DesignerFont`   | kept                             | `@font-face` per used font       |
// | `SingleLineFont` | font set to the bundled font     | `@font-face` for the bundled font|
// | `HersheyStrokes` | replaced by stroked `<path>`     | nothing                          |
//
// Presentation attributes are read; CSS `style="…"` declarations are not.

mod font_embedding;
mod hershey_strokes;

use xmltree::{Element, XMLNode};

pub use font_embedding::SINGLE_LINE_FONT_FAMILY;

/// `data-type` values of the groups whose text this crate rewrites.
const LABEL_TYPES: [&str; 2] = ["piece_label", "pattern_label"];

/// @brief How label text is written in an exported SVG.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SvgTextMode {
    /// `<text>` in the designer's font, embedded as a subset `@font-face`.
    DesignerFont,
    /// `<text>` in the bundled single-line font, embedded as a subset `@font-face`.
    SingleLineFont,
    /// Single-stroke Hershey `<path>` polylines; the string stays in `data-text` and `<desc>`.
    HersheyStrokes,
} // enum SvgTextMode

impl SvgTextMode {
    /// @brief Parse the mode name QML sends.
    /// @return None for an unknown name.
    pub fn from_name(name: &str) -> Option<Self> {
        match name {
            "designerFont" => Some(Self::DesignerFont),     // mode 1
            "singleLineFont" => Some(Self::SingleLineFont), // mode 2
            "hersheyStrokes" => Some(Self::HersheyStrokes), // mode 3
            _ => None,                                      // unknown name
        } // match name
    } // fn from_name

    /// @brief Mode name as QML and the preferences file write it.
    pub fn name(self) -> &'static str {
        match self {
            Self::DesignerFont => "designerFont",
            Self::SingleLineFont => "singleLineFont",
            Self::HersheyStrokes => "hersheyStrokes",
        } // match self
    } // fn name
} // impl SvgTextMode

/// @brief What kind of label text a document carries.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LabelTextState {
    /// At least one label group holds a `<text>`; all three modes apply.
    Text,
    /// Label groups exist but none holds a `<text>` (Seamly2D `--text2paths`).
    PathsOnly,
    /// No label groups; all three modes give the same output.
    NoLabels,
} // enum LabelTextState

impl LabelTextState {
    /// @brief State name for the QML `labelTextState` property.
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Text => "text",
            Self::PathsOnly => "pathsOnly",
            Self::NoLabels => "noLabels",
        } // match self
    } // fn as_str
} // impl LabelTextState

/// @brief Result of one text-mode conversion.
#[derive(Debug, Clone, Default, PartialEq)]
pub struct LabelTextReport {
    /// Number of `<text>` elements inside label groups.
    pub text_elements: usize,
    /// Messages for the user: missing fonts, fonts that forbid embedding, missing glyphs.
    pub warnings: Vec<String>,
} // struct LabelTextReport

/// @brief Style values a `<text>` takes from itself and its ancestors.
#[derive(Debug, Clone, Default)]
pub(crate) struct TextStyle {
    pub font_family: Option<String>,
    pub font_size: Option<String>,
    pub font_weight: Option<String>,
    pub font_style: Option<String>,
    pub fill: Option<String>,
    pub fill_opacity: Option<String>,
    pub text_anchor: Option<String>,
} // struct TextStyle

impl TextStyle {
    /// @brief Style of `element`: its own presentation attributes override the inherited ones.
    pub(crate) fn inherit(&self, element: &Element) -> TextStyle {
        let own = |name: &str, parent: &Option<String>| {
            element.attributes.get(name).cloned().or_else(|| parent.clone())
        };
        TextStyle {
            font_family: own("font-family", &self.font_family),
            font_size: own("font-size", &self.font_size),
            font_weight: own("font-weight", &self.font_weight),
            font_style: own("font-style", &self.font_style),
            fill: own("fill", &self.fill),
            fill_opacity: own("fill-opacity", &self.fill_opacity),
            text_anchor: own("text-anchor", &self.text_anchor),
        }
    } // fn inherit

    /// @brief Font size in user units; 12 when absent or not a plain number.
    pub(crate) fn font_size_px(&self) -> f64 {
        self.font_size
            .as_deref()
            .map(|s| s.trim().trim_end_matches("px"))
            .and_then(|s| s.parse::<f64>().ok())
            .filter(|v| *v > 0.0)
            .unwrap_or(12.0)
    } // fn font_size_px
} // impl TextStyle

/// @brief True when `element` is a `<g>` tagged as a piece or pattern label.
fn is_label_group(element: &Element) -> bool {
    element.name == "g"
        && element
            .attributes
            .get("data-type")
            .is_some_and(|t| LABEL_TYPES.contains(&t.as_str()))
} // fn is_label_group

/// @brief Call `visit` for every `<text>` inside a label group, with its resolved style.
///
/// `visit` may rewrite the `<text>` element in place (mode 3 turns it into a `<g>`).
/// A `<text>` outside every label group is never visited.
pub(crate) fn for_each_label_text(root: &mut Element, visit: &mut dyn FnMut(&mut Element, &TextStyle)) {
    let style = TextStyle::default().inherit(root);
    walk_label_text(root, &style, false, visit);
} // fn for_each_label_text

/// @brief Recursive step of `for_each_label_text`.
fn walk_label_text(
    element: &mut Element,
    style: &TextStyle,
    in_label: bool,
    visit: &mut dyn FnMut(&mut Element, &TextStyle),
) {
    for child in element.children.iter_mut() {
        // Only elements can hold label text; skip text nodes and comments.
        let XMLNode::Element(child) = child else { continue };
        let child_style = style.inherit(child);
        let child_in_label = in_label || is_label_group(child);
        if child_in_label && child.name == "text" {
            // Label line found: hand it over; its own children are its content.
            visit(child, &child_style);
        } else {
            // Not a label line: descend.
            walk_label_text(child, &child_style, child_in_label, visit);
        } // if label text
    } // for child
} // fn walk_label_text

/// @brief Character content of `element` and its descendants (`<tspan>` included).
pub(crate) fn text_content(element: &Element) -> String {
    let mut out = String::new();
    for child in &element.children {
        match child {
            XMLNode::Text(t) | XMLNode::CData(t) => out.push_str(t), // literal text
            XMLNode::Element(e) => out.push_str(&text_content(e)),   // nested <tspan>
            _ => {}                                                  // comments, PIs
        } // match child
    } // for child
    out
} // fn text_content

/// @brief New element in the same namespace as `sibling_or_parent`.
pub(crate) fn new_element(name: &str, sibling_or_parent: &Element) -> Element {
    let mut element = Element::new(name);
    element.namespace = sibling_or_parent.namespace.clone();
    element
} // fn new_element

/// @brief Classify the label text of a document.
pub fn label_text_state(doc: &svg_dom::Document) -> LabelTextState {
    // Count label groups and the <text> elements inside them in one pass.
    fn count(element: &Element, in_label: bool, groups: &mut usize, texts: &mut usize) {
        for child in &element.children {
            let XMLNode::Element(child) = child else { continue };
            let is_group = is_label_group(child);
            if is_group {
                *groups += 1; // label group found
            } // if label group
            let child_in_label = in_label || is_group;
            if child_in_label && child.name == "text" {
                *texts += 1; // label line found
            } else {
                count(child, child_in_label, groups, texts); // descend
            } // if label text
        } // for child
    } // fn count

    let (mut groups, mut texts) = (0usize, 0usize);
    count(&doc.root, false, &mut groups, &mut texts);
    if groups == 0 {
        LabelTextState::NoLabels // nothing to convert
    } else if texts == 0 {
        LabelTextState::PathsOnly // labels already outlined
    } else {
        LabelTextState::Text // at least one real label line
    } // if groups/texts
} // fn label_text_state

/// @brief Rewrite the label text of `doc` in `mode`, using the system fonts for mode 1.
pub fn apply_text_mode(doc: &mut svg_dom::Document, mode: SvgTextMode) -> LabelTextReport {
    let mut fonts = fontdb::Database::new();
    if mode == SvgTextMode::DesignerFont {
        // Only mode 1 looks up the designer's fonts; loading them costs time.
        fonts.load_system_fonts();
    } // if designer font
    apply_text_mode_with_fonts(doc, mode, &fonts)
} // fn apply_text_mode

/// @brief Rewrite the label text of `doc` in `mode`, looking fonts up in `fonts`.
///
/// Tests pass their own `fonts` so the result does not depend on the machine.
pub fn apply_text_mode_with_fonts(
    doc: &mut svg_dom::Document,
    mode: SvgTextMode,
    fonts: &fontdb::Database,
) -> LabelTextReport {
    match mode {
        SvgTextMode::DesignerFont => font_embedding::embed_designer_fonts(&mut doc.root, fonts),
        SvgTextMode::SingleLineFont => font_embedding::apply_single_line_font(&mut doc.root),
        SvgTextMode::HersheyStrokes => hershey_strokes::convert_to_strokes(&mut doc.root),
    } // match mode
} // fn apply_text_mode_with_fonts

#[cfg(test)]
#[path = "svg_label_text_tests.rs"]
mod tests;
