// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file line_classes.rs
// @brief Sorts layout geometry into cut lines, marks and labels, and records the result so usvg keeps it.
//
// usvg drops `data-*` attributes but keeps the `id` of every `<g>`. The
// classifier therefore reads `data-type` (or, for untagged files, the id)
// on the svg_dom side and wraps the group's children in a marker group
// `<g id="hpgl-class-<class>-<n>">`. The original group and its id stay as
// they are, so no `href="#id"` reference breaks.

use xmltree::{Element, XMLNode};

/// Prefix of the marker group ids that `tag_line_classes` inserts.
const MARKER_PREFIX: &str = "hpgl-class-";

/// @brief What a plotted line is for; each class has its own pen.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LineClass {
    /// Lines the cutter cuts along: `cutline`, `cut_path`.
    Cut,
    /// Lines drawn on the piece: seam lines, internal paths, grainlines, notches.
    Mark,
    /// Label text, written as single strokes.
    Label,
} // enum LineClass

impl LineClass {
    /// @brief Name used in marker ids and option JSON.
    pub fn name(self) -> &'static str {
        match self {
            LineClass::Cut => "cut",     // cut lines
            LineClass::Mark => "mark",   // marks
            LineClass::Label => "label", // labels
        } // match self
    } // fn name

    /// @brief Parse a class name written by `name`.
    fn from_name(name: &str) -> Option<Self> {
        match name {
            "cut" => Some(LineClass::Cut),     // cut lines
            "mark" => Some(LineClass::Mark),   // marks
            "label" => Some(LineClass::Label), // labels
            _ => None,                         // not a class name
        } // match name
    } // fn from_name
} // impl LineClass

/// @brief Class for a Seamly2D `data-type` value; `None` for types that only group others.
fn class_from_data_type(data_type: &str) -> Option<LineClass> {
    match data_type {
        "cutline" | "cut_path" => Some(LineClass::Cut), // cutter lines
        "seamline" | "internal_path" | "grainline" | "notch" | "tuck" | "drill" | "hole" => {
            Some(LineClass::Mark) // drawn on the piece
        } // marks
        "piece_label" | "pattern_label" | "label_text" => Some(LineClass::Label), // text
        _ => None, // pattern, piece, and unknown types inherit
    } // match data_type
} // fn class_from_data_type

/// @brief Class from a group id, for files without `data-type`.
///
/// Matches the name-based ids (`cutline_Front`) and the older counter-based
/// ids (`Front-cutline-1`).
fn class_from_id(id: &str) -> Option<LineClass> {
    let id = id.to_lowercase();
    // Name-based ids start with the type; counter-based ids carry "-<type>".
    let has = |kind: &str| id.starts_with(kind) || id.contains(&format!("-{kind}"));

    if has("cutline") || has("cut_path") {
        Some(LineClass::Cut) // cut lines
    } else if has("piece_label") || has("pattern_label") || has("label_text") {
        Some(LineClass::Label) // labels; checked before marks so "label" wins
    } else if ["seamline", "internal_path", "ip_", "grainline", "grain_", "notch", "tuck", "drill", "hole"]
        .iter()
        .any(|kind| has(kind))
    {
        Some(LineClass::Mark) // marks
    } else {
        None // no hint in the id
    } // if id hint
} // fn class_from_id

/// @brief Class of a `<g>`: `data-type` first, then the id.
fn class_of_group(group: &Element) -> Option<LineClass> {
    if let Some(data_type) = group.attributes.get("data-type") {
        return class_from_data_type(data_type); // tagged file: data-type decides
    } // if data-type
    group.attributes.get("id").and_then(|id| class_from_id(id))
} // fn class_of_group

/// @brief Wrap the children of every classified `<g>` in a marker group that usvg keeps.
///
/// Only `<svg>` and `<g>` are walked; a group inside `<defs>` or `<clipPath>`
/// is never drawn directly, so it is left alone.
pub(crate) fn tag_line_classes(root: &mut Element) {
    let mut counter = 0usize;
    tag_children(root, &mut counter);
} // fn tag_line_classes

/// @brief Recursive step of `tag_line_classes`.
fn tag_children(parent: &mut Element, counter: &mut usize) {
    for child in parent.children.iter_mut() {
        let XMLNode::Element(group) = child else { continue }; // text and comments
        if group.name != "g" {
            continue; // only groups carry a class
        } // if not a group

        // Nested typed groups (label text inside a piece label) are tagged first.
        tag_children(group, counter);

        if let Some(class) = class_of_group(group) {
            // Move the group's children into a marker group; the nearest marker wins.
            *counter += 1;
            let mut marker = Element::new("g");
            marker
                .attributes
                .insert("id".to_string(), format!("{MARKER_PREFIX}{}-{counter}", class.name()));
            marker.children = std::mem::take(&mut group.children);
            group.children.push(XMLNode::Element(marker));
        } // if classified
    } // for child
} // fn tag_children

/// @brief Class recorded in a marker group id; `None` for any other id.
pub(crate) fn class_from_marker_id(id: &str) -> Option<LineClass> {
    let rest = id.strip_prefix(MARKER_PREFIX)?;
    let (name, _) = rest.split_once('-')?;
    LineClass::from_name(name)
} // fn class_from_marker_id
