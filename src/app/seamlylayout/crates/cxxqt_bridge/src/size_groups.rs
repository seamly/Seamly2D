// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file size_groups.rs
// @brief Regroups the pieces of a Nested multisize layout by size, for SVG export.
//
// The Nested layout keeps each piece-set (one piece in every size) as one group,
// because the packer and Adjust Mode move a piece-set as one unit. An SVG export
// holds one group per size instead, so a graphics editor can show one size and
// hide the others. Every piece keeps its position.
//
// The size groups run largest size first: the largest size is drawn underneath,
// the smallest on top.

use std::cmp::Ordering;

use xmltree::{Element, XMLNode};

// @brief `data-type` of a size group in an exported SVG.
pub(crate) const SIZE_GROUP_TYPE: &str = "size";

// @brief Move every sized piece out of its piece-set into one group per size.
//
// Bakes all transforms first, so the export is flat (text keeps its own
// transform) and a piece does not move when it leaves its piece-set. A piece-set
// left with no element child is removed. The size groups take the place of the
// first piece-set; other top-level elements keep their order.
//
// @param doc Export copy of a layout; changed in place.
// @return Number of size groups; 0, and `doc` unchanged, when it has no top-level piece-set.
pub(crate) fn group_pieces_by_size(doc: &mut svg_dom::Document) -> usize {
    let has_piece_set = doc.root.children.iter()
        .any(|node| matches!(node, XMLNode::Element(e) if svg_dom::is_piece_set(e)));
    if !has_piece_set {
        return 0; // Marker, a size tab, or an individual pattern
    } // if no piece-set

    svg_dom::flatten_dom(doc);

    // Pieces per size, in first-seen order; sorted after the walk.
    let mut sizes: Vec<(String, Vec<XMLNode>)> = Vec::new();
    let mut kept: Vec<XMLNode> = Vec::new();
    let mut insert_at: Option<usize> = None;
    for node in std::mem::take(&mut doc.root.children) {
        let XMLNode::Element(mut set) = node else {
            kept.push(node); // text and comments stay where they are
            continue;
        }; // if not an element
        if !svg_dom::is_piece_set(&set) {
            kept.push(XMLNode::Element(set)); // legend, frame and other groups
            continue;
        } // if not a piece-set
        insert_at.get_or_insert(kept.len());

        // Take each sized piece out; anything else stays in the piece-set.
        let mut rest = Vec::new();
        for child in std::mem::take(&mut set.children) {
            match child {
                XMLNode::Element(piece) if is_sized_piece(&piece) => {
                    let size = piece.attributes.get("data-size").cloned().unwrap_or_default();
                    match sizes.iter_mut().find(|(known, _)| *known == size) {
                        Some((_, pieces)) => pieces.push(XMLNode::Element(piece)),
                        None => sizes.push((size, vec![XMLNode::Element(piece)])),
                    } // match known size
                } // sized piece
                other => rest.push(other),
            } // match child
        } // for child
        if rest.iter().any(|n| matches!(n, XMLNode::Element(_))) {
            set.children = rest;
            kept.push(XMLNode::Element(set)); // piece-set still holds an element
        } // if rest
    } // for node

    // Largest size first, so it is drawn underneath the smaller ones.
    sizes.sort_by(|a, b| compare_sizes(&b.0, &a.0));
    let groups: Vec<XMLNode> = sizes.into_iter()
        .map(|(size, pieces)| XMLNode::Element(size_group(&size, pieces)))
        .collect();
    let count = groups.len();
    let at = insert_at.unwrap_or(kept.len());
    kept.splice(at..at, groups);
    doc.root.children = kept;
    count
} // fn group_pieces_by_size

// @brief True for a `data-type="piece"` group that carries `data-size`.
fn is_sized_piece(elem: &Element) -> bool {
    elem.attributes.get("data-type").is_some_and(|t| t == "piece") && elem.attributes.contains_key("data-size")
} // fn is_sized_piece

// @brief Order two `data-size` values: numerically when both are numbers, else as text.
pub(crate) fn compare_sizes(a: &str, b: &str) -> Ordering {
    match (a.trim().parse::<f64>(), b.trim().parse::<f64>()) {
        (Ok(x), Ok(y)) => x.partial_cmp(&y).unwrap_or(Ordering::Equal),
        _ => a.cmp(b),
    } // match parsed
} // fn compare_sizes

// @brief A `<g data-type="size">` holding the pieces of one size.
fn size_group(size: &str, pieces: Vec<XMLNode>) -> Element {
    let mut group = Element::new("g");
    group.attributes.insert("id".to_string(), format!("size_{}", id_token(size)));
    group.attributes.insert("data-type".to_string(), SIZE_GROUP_TYPE.to_string());
    group.attributes.insert("data-size".to_string(), size.to_string());
    group.children = pieces;
    group
} // fn size_group

// @brief `size` with every character other than a letter, digit, `-` or `.` replaced by `_`.
fn id_token(size: &str) -> String {
    size.chars()
        .map(|c| if c.is_ascii_alphanumeric() || c == '-' || c == '.' { c } else { '_' })
        .collect()
} // fn id_token

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Two piece-sets of sizes 34 and 36, the second moved by its group transform.
    const NESTED: &str = r#"<svg xmlns="http://www.w3.org/2000/svg" width="400" height="200">
  <g id="Rectangles"><rect id="backgroundRect" x="0" y="0" width="400" height="200"/></g>
  <g id="piece-set_A" data-type="piece-set">
    <g id="piece_A_s34" data-type="piece" data-size="34"><path d="M0,0 L10,0 L10,10 Z"/></g>
    <g id="piece_A_s36" data-type="piece" data-size="36"><path d="M0,0 L12,0 L12,12 Z"/></g>
  </g>
  <g id="piece-set_B" data-type="piece-set" transform="translate(100 50)">
    <g id="piece_B_s36" data-type="piece" data-size="36"><path d="M0,0 L20,0 L20,20 Z"/><text x="1" y="2">B</text></g>
    <g id="piece_B_s34" data-type="piece" data-size="34"><path d="M0,0 L18,0 L18,18 Z"/></g>
  </g>
  <g id="size-legend" data-type="size_legend"><text x="5" y="190">Size 34</text></g>
</svg>"#;

    // @brief The direct child groups of the root, by id.
    fn top_ids(doc: &svg_dom::Document) -> Vec<String> {
        doc.root.children.iter()
            .filter_map(|n| n.as_element())
            .map(|e| e.attributes.get("id").cloned().unwrap_or_default())
            .collect()
    } // fn top_ids

    // @brief The ids of the pieces inside one size group.
    fn piece_ids(doc: &svg_dom::Document, group_id: &str) -> Vec<String> {
        doc.root.children.iter()
            .filter_map(|n| n.as_element())
            .find(|e| e.attributes.get("id").is_some_and(|id| id == group_id))
            .map(|g| g.children.iter().filter_map(|n| n.as_element())
                .map(|e| e.attributes.get("id").cloned().unwrap_or_default()).collect())
            .unwrap_or_default()
    } // fn piece_ids

    // @brief Each size gets one group, largest first, in place of the piece-sets.
    #[test]
    fn nested_pieces_regroup_by_size_largest_first() {
        let mut doc = svg_dom::Document::parse(NESTED).expect("valid SVG");
        assert_eq!(group_pieces_by_size(&mut doc), 2);
        assert_eq!(top_ids(&doc), vec!["Rectangles", "size_36", "size_34", "size-legend"]);
        assert_eq!(piece_ids(&doc, "size_36"), vec!["piece_A_s36", "piece_B_s36"]);
        assert_eq!(piece_ids(&doc, "size_34"), vec!["piece_A_s34", "piece_B_s34"]);
        let group = doc.root.children.iter().filter_map(|n| n.as_element())
            .find(|e| e.attributes.get("id").is_some_and(|id| id == "size_36")).expect("size group");
        assert_eq!(group.attributes.get("data-type").map(String::as_str), Some("size"));
        assert_eq!(group.attributes.get("data-size").map(String::as_str), Some("36"));
    } // nested_pieces_regroup_by_size_largest_first

    // @brief A piece keeps its position: the piece-set transform is baked, and no
    // transform is left except on text.
    #[test]
    fn regrouped_pieces_keep_their_position_and_are_flat() {
        let mut doc = svg_dom::Document::parse(NESTED).expect("valid SVG");
        group_pieces_by_size(&mut doc);

        // The first point of piece B, size 36, is the piece-set offset (100, 50).
        let piece = find(&doc.root, "piece_B_s36").expect("piece B size 36");
        let d = piece.children.iter().filter_map(|n| n.as_element())
            .find(|e| e.name == "path").and_then(|p| p.attributes.get("d")).expect("path d");
        let numbers: Vec<f64> = d.split(|c: char| !(c.is_ascii_digit() || c == '.' || c == '-'))
            .filter_map(|t| t.parse().ok()).collect();
        assert_eq!(&numbers[..2], &[100.0, 50.0], "piece B moved: {d}");

        // Only text keeps a transform.
        let mut with_transform = Vec::new();
        collect_transformed(&doc.root, &mut with_transform);
        assert!(with_transform.iter().all(|name| name == "text"), "transform on {with_transform:?}");
    } // regrouped_pieces_keep_their_position_and_are_flat

    // @brief The element with `id` in the subtree of `elem`.
    fn find<'a>(elem: &'a Element, id: &str) -> Option<&'a Element> {
        if elem.attributes.get("id").is_some_and(|v| v == id) {
            return Some(elem);
        } // if match
        elem.children.iter().filter_map(|n| n.as_element()).find_map(|c| find(c, id))
    } // fn find

    // @brief Names of the elements that carry a `transform`.
    fn collect_transformed(elem: &Element, names: &mut Vec<String>) {
        if elem.attributes.contains_key("transform") {
            names.push(elem.name.clone());
        } // if transform
        for child in elem.children.iter().filter_map(|n| n.as_element()) {
            collect_transformed(child, names);
        } // for child
    } // fn collect_transformed

    // @brief Sizes order by number, not by text: 100 is larger than 92.
    #[test]
    fn sizes_order_by_number() {
        assert_eq!(compare_sizes("100", "92"), Ordering::Greater);
        assert_eq!(compare_sizes("34.5", "34"), Ordering::Greater);
        assert_eq!(compare_sizes("M", "L"), Ordering::Greater);
    } // sizes_order_by_number

    // @brief A layout without piece-sets (Marker, a size tab, an individual pattern) is unchanged.
    #[test]
    fn layout_without_piece_sets_is_unchanged() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece_A_s34" data-type="piece" data-size="34" transform="translate(5 5)"><path d="M0,0 L1,1"/></g></svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("valid SVG");
        let before = doc.to_string();
        assert_eq!(group_pieces_by_size(&mut doc), 0);
        assert_eq!(doc.to_string(), before);
    } // layout_without_piece_sets_is_unchanged
} // mod tests
