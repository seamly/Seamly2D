// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file nested_annotations.rs
// @brief Keeps one grainline and one set of labels per piece in a Nested multisize export.
//
// A Nested piece-set stacks every size of one piece, and each size has its own
// grainline and labels. Every export but DXF-ASTM keeps only those of the
// smallest size in each piece-set. The SVG export also moves them into one
// `<g id="labels_and_grainlines">`, so an editor can show or hide them apart
// from the size groups. Outlines, notches, fold lines and other components of
// every size stay.

use xmltree::{Element, XMLNode};

use crate::size_groups::{compare_sizes, is_sized_piece, SIZE_GROUP_TYPE};

// @brief `id` of the SVG export group that holds the kept labels and grainlines.
pub(crate) const LABELS_AND_GRAINLINES_ID: &str = "labels_and_grainlines";

// @brief `data-type` values of the groups this module keeps for the smallest size only.
const ANNOTATION_TYPES: [&str; 4] = ["grainline", "piece_label", "pattern_label", "size_label"];

// @brief Remove the grainline and label groups of every size but the smallest from each piece-set.
//
// Finds piece-sets at any depth, so it works on a layout and on the sheet
// export DOM. A document without piece-sets (Marker, a size tab, an individual
// pattern) is unchanged.
//
// @param doc Export copy of a layout; changed in place.
// @return Number of groups removed.
pub(crate) fn keep_smallest_size_annotations(doc: &mut svg_dom::Document) -> usize {
    let mut removed = 0;
    strip_in_subtree(&mut doc.root, &mut removed);
    removed
} // fn keep_smallest_size_annotations

// @brief Recursive step of `keep_smallest_size_annotations`; a piece is not searched.
fn strip_in_subtree(elem: &mut Element, removed: &mut usize) {
    for child in elem.children.iter_mut().filter_map(|n| n.as_mut_element()) {
        if svg_dom::is_piece_set(child) {
            *removed += strip_piece_set(child);
        } else if !is_sized_piece(child) {
            strip_in_subtree(child, removed); // a pattern or other wrapper
        } // if piece-set
    } // for child
} // fn strip_in_subtree

// @brief Strip the annotations of every sized piece in `set` but the smallest size.
// @return Number of groups removed.
fn strip_piece_set(set: &mut Element) -> usize {
    // Smallest `data-size` among the members, compared as numbers when it can.
    let Some(smallest) = set.children.iter()
        .filter_map(|n| n.as_element())
        .filter(|e| is_sized_piece(e))
        .filter_map(|e| e.attributes.get("data-size"))
        .min_by(|a, b| compare_sizes(a, b))
        .cloned()
    else {
        return 0; // no sized piece
    }; // smallest

    let mut removed = 0;
    for piece in set.children.iter_mut().filter_map(|n| n.as_mut_element()).filter(|e| is_sized_piece(e)) {
        if piece.attributes.get("data-size") != Some(&smallest) {
            removed += remove_annotations(piece);
        } // if not the smallest size
    } // for piece
    removed
} // fn strip_piece_set

// @brief Remove every grainline and label group in `elem`'s subtree.
// @return Number of groups removed.
fn remove_annotations(elem: &mut Element) -> usize {
    let before = elem.children.len();
    elem.children.retain(|n| !matches!(n, XMLNode::Element(e) if is_annotation(e)));
    let mut removed = before - elem.children.len();
    for child in elem.children.iter_mut().filter_map(|n| n.as_mut_element()) {
        removed += remove_annotations(child);
    } // for child
    removed
} // fn remove_annotations

// @brief True for a `<g>` whose `data-type` is a grainline or a label.
fn is_annotation(elem: &Element) -> bool {
    elem.name == "g" && elem.attributes.get("data-type").is_some_and(|t| ANNOTATION_TYPES.contains(&t.as_str()))
} // fn is_annotation

// @brief Move every grainline and label group out of the size groups into one
// `<g id="labels_and_grainlines">`, placed just before the first size group.
//
// Call after `size_groups::group_pieces_by_size`: the document is flat then, so
// a group keeps its position when it leaves its piece. The moved groups keep
// their document order. Size groups are the only top-level groups searched.
//
// @param doc SVG export copy of a Nested layout; changed in place.
// @return Number of groups moved; 0, and `doc` unchanged, when none is found.
pub(crate) fn gather_labels_and_grainlines(doc: &mut svg_dom::Document) -> usize {
    let Some(at) = doc.root.children.iter().position(|n| matches!(n, XMLNode::Element(e) if is_size_group(e))) else {
        return 0; // not a Nested export
    }; // at

    // Take the groups out of each size group, largest size first.
    let mut moved = Vec::new();
    for group in doc.root.children.iter_mut().filter_map(|n| n.as_mut_element()).filter(|e| is_size_group(e)) {
        take_annotations(group, &mut moved);
    } // for size group
    if moved.is_empty() {
        return 0; // no grainline or label to move
    } // if none

    let count = moved.len();
    let mut group = Element::new("g");
    group.attributes.insert("id".to_string(), LABELS_AND_GRAINLINES_ID.to_string());
    group.children = moved;
    doc.root.children.insert(at, XMLNode::Element(group));
    count
} // fn gather_labels_and_grainlines

// @brief True for a top-level `<g data-type="size">` of a Nested SVG export.
fn is_size_group(elem: &Element) -> bool {
    elem.attributes.get("data-type").is_some_and(|t| t == SIZE_GROUP_TYPE)
} // fn is_size_group

// @brief Move every grainline and label group in `elem`'s subtree to `moved`, in document order.
fn take_annotations(elem: &mut Element, moved: &mut Vec<XMLNode>) {
    let mut kept = Vec::with_capacity(elem.children.len());
    for node in std::mem::take(&mut elem.children) {
        match node {
            XMLNode::Element(e) if is_annotation(&e) => moved.push(XMLNode::Element(e)),
            XMLNode::Element(mut e) => {
                take_annotations(&mut e, moved); // a piece or a component group
                kept.push(XMLNode::Element(e));
            } // other element
            other => kept.push(other), // text and comments stay
        } // match node
    } // for node
    elem.children = kept;
} // fn take_annotations

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief One piece-set in sizes 92 and 100, moved by its transform. Each size has a
    // cutline, a notch, a grainline and three labels. A piece-set inside a pattern
    // group, as in the sheet export input, and a plain individual piece follow.
    const NESTED: &str = r#"<svg xmlns="http://www.w3.org/2000/svg" width="400" height="200">
  <defs/>
  <g id="piece-set_A" data-type="piece-set" transform="translate(100 50)">
    <g id="piece_A_s100" data-type="piece" data-size="100">
      <g id="cutline_A_s100" data-type="cutline"><path d="M0,0 L40,0 L40,40 Z"/></g>
      <g id="notch_A_s100" data-type="notch"><path d="M0,0 L1,1"/></g>
      <g id="grainline_A_s100" data-type="grainline"><path d="M20,30 L20,5"/></g>
      <g id="piece_label_A_s100" data-type="piece_label"><text x="5" y="5">A</text></g>
      <g id="pattern_label_A_s100" data-type="pattern_label"><text x="5" y="9">Shirt</text></g>
      <g id="size_label_A_s100" data-type="size_label"><text transform="translate(20 1)">Size 100</text></g>
    </g>
    <g id="piece_A_s92" data-type="piece" data-size="92">
      <g id="cutline_A_s92" data-type="cutline"><path d="M0,0 L30,0 L30,30 Z"/></g>
      <g id="notch_A_s92" data-type="notch"><path d="M0,0 L1,1"/></g>
      <g id="grainline_A_s92" data-type="grainline"><path d="M15,25 L15,5"/></g>
      <g id="piece_label_A_s92" data-type="piece_label"><text x="4" y="4">A</text></g>
      <g id="pattern_label_A_s92" data-type="pattern_label"><text x="4" y="8">Shirt</text></g>
      <g id="size_label_A_s92" data-type="size_label"><text transform="translate(15 1)">Size 92</text></g>
    </g>
  </g>
  <g id="size_legend" data-type="size_legend"><text x="5" y="190">Size 92</text></g>
</svg>"#;

    // @brief The element with `id` in the subtree of `elem`.
    fn find<'a>(elem: &'a Element, id: &str) -> Option<&'a Element> {
        if elem.attributes.get("id").is_some_and(|v| v == id) {
            return Some(elem);
        } // if match
        elem.children.iter().filter_map(|n| n.as_element()).find_map(|c| find(c, id))
    } // fn find

    // @brief The ids of the element children of the element with `id`.
    fn child_ids(doc: &svg_dom::Document, id: &str) -> Vec<String> {
        find(&doc.root, id).expect("element").children.iter()
            .filter_map(|n| n.as_element())
            .map(|e| e.attributes.get("id").cloned().unwrap_or_default())
            .collect()
    } // fn child_ids

    // @brief The ids of the direct child elements of the root.
    fn top_ids(doc: &svg_dom::Document) -> Vec<String> {
        doc.root.children.iter()
            .filter_map(|n| n.as_element())
            .map(|e| e.attributes.get("id").cloned().unwrap_or_default())
            .collect()
    } // fn top_ids

    // @brief The smallest size, by number, keeps its grainline and labels; the larger
    // size keeps only its outline and notch.
    #[test]
    fn only_the_smallest_size_keeps_grainline_and_labels() {
        let mut doc = svg_dom::Document::parse(NESTED).expect("valid SVG");
        assert_eq!(keep_smallest_size_annotations(&mut doc), 4);
        assert_eq!(child_ids(&doc, "piece_A_s100"), vec!["cutline_A_s100", "notch_A_s100"]);
        assert_eq!(child_ids(&doc, "piece_A_s92"), vec![
            "cutline_A_s92", "notch_A_s92", "grainline_A_s92", "piece_label_A_s92", "pattern_label_A_s92", "size_label_A_s92",
        ]);
        assert!(find(&doc.root, "size_legend").is_some(), "the legend is not a piece label");
    } // only_the_smallest_size_keeps_grainline_and_labels

    // @brief A piece-set inside a pattern group, as in the sheet export input, is found.
    #[test]
    fn piece_set_inside_a_pattern_group_is_found() {
        let svg = NESTED.replace(r#"<defs/>"#, r#"<defs/><g id="pattern" data-type="pattern">"#)
            .replace(r#"<g id="size_legend""#, r#"</g><g id="size_legend""#);
        let mut doc = svg_dom::Document::parse(&svg).expect("valid SVG");
        assert_eq!(keep_smallest_size_annotations(&mut doc), 4);
        assert!(find(&doc.root, "grainline_A_s100").is_none());
        assert!(find(&doc.root, "grainline_A_s92").is_some());
    } // piece_set_inside_a_pattern_group_is_found

    // @brief A layout without piece-sets (Marker, a size tab, an individual pattern) is unchanged.
    #[test]
    fn layout_without_piece_sets_is_unchanged() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg">
  <g id="piece_A_s34" data-type="piece" data-size="34"><g data-type="grainline"><path d="M0,0 L0,9"/></g></g>
  <g id="piece_A_s36" data-type="piece" data-size="36"><g data-type="grainline"><path d="M0,0 L0,9"/></g></g>
</svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("valid SVG");
        let before = doc.to_string();
        assert_eq!(keep_smallest_size_annotations(&mut doc), 0);
        assert_eq!(doc.to_string(), before);
    } // layout_without_piece_sets_is_unchanged

    // @brief SVG export: the kept groups move into `labels_and_grainlines`, just before
    // the first size group, and keep their position.
    #[test]
    fn svg_export_gathers_labels_and_grainlines_before_the_sizes() {
        let mut doc = svg_dom::Document::parse(NESTED).expect("valid SVG");
        keep_smallest_size_annotations(&mut doc);
        assert_eq!(crate::size_groups::group_pieces_by_size(&mut doc), 2);
        assert_eq!(gather_labels_and_grainlines(&mut doc), 4);

        assert_eq!(top_ids(&doc), vec!["", LABELS_AND_GRAINLINES_ID, "size_100", "size_92", "size_legend"]);
        assert_eq!(child_ids(&doc, LABELS_AND_GRAINLINES_ID), vec![
            "grainline_A_s92", "piece_label_A_s92", "pattern_label_A_s92", "size_label_A_s92",
        ]);
        assert_eq!(child_ids(&doc, "piece_A_s92"), vec!["cutline_A_s92", "notch_A_s92"]);

        // The piece-set offset (100, 50) is baked into the moved grainline.
        let grainline = find(&doc.root, "grainline_A_s92").expect("grainline");
        let d = grainline.children.iter().filter_map(|n| n.as_element())
            .find(|e| e.name == "path").and_then(|p| p.attributes.get("d")).expect("path d");
        let numbers: Vec<f64> = d.split(|c: char| !(c.is_ascii_digit() || c == '.' || c == '-'))
            .filter_map(|t| t.parse().ok()).collect();
        assert_eq!(&numbers[..2], &[115.0, 75.0], "grainline moved: {d}");
    } // svg_export_gathers_labels_and_grainlines_before_the_sizes

    // @brief A document without size groups is unchanged.
    #[test]
    fn gather_without_size_groups_is_unchanged() {
        let mut doc = svg_dom::Document::parse(NESTED).expect("valid SVG");
        let before = doc.to_string();
        assert_eq!(gather_labels_and_grainlines(&mut doc), 0);
        assert_eq!(doc.to_string(), before);
    } // gather_without_size_groups_is_unchanged
} // mod tests
