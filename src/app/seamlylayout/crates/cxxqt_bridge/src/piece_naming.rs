// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file piece_naming.rs
// @brief Gives every tagged pattern piece a unique `data-name` before a layout.
//
// A pattern can give several pieces one name ("Detail"), or no name. The import
// renames each `data-type="piece"` group from its number, name and letter:
//
//   * Individual pattern: `Piece_<n>[_<name>][_<letter>]`.
//   * Multisize pattern:  `<size>_Piece_<n>[_<name>][_<letter>]`.
//
// `<n>` is the 1-based position of the piece among the pieces of its size, in
// document order, so one piece has the same number in every size. The number
// ends at the first `_` after `Piece_`, so no two pieces of one size get the same
// name. A multisize piece-set gets the name of its pieces without the size.
//
// Each renamed group carries `data-name-assigned="true"`. A second import of a
// SeamlyLayout export keeps those names and does not number them again.

use xmltree::{Element, XMLNode};

// @brief Attribute that marks a `data-name` this module assigned.
pub(crate) const NAME_ASSIGNED_ATTR: &str = "data-name-assigned";

// @brief Result of `assign_unique_piece_names`.
#[derive(Debug, Default, PartialEq, Eq)]
pub(crate) struct PieceNaming {
    // Piece groups that got a new name; piece-sets are not counted.
    pub renamed: usize,
    // Warning for the user when the sizes hold different piece counts; None otherwise.
    pub warning: Option<String>,
}

// @brief True when the element's `data-name` came from `assign_unique_piece_names`.
pub(crate) fn is_name_assigned(elem: &Element) -> bool {
    elem.attributes.get(NAME_ASSIGNED_ATTR).is_some_and(|v| v == "true")
} // fn is_name_assigned

// @brief Give every tagged piece, and every multisize piece-set, a unique `data-name`.
// @param doc Imported document; changed in place. An untagged document stays unchanged.
// @return Number of renamed pieces, and a warning when the sizes differ in piece count.
pub(crate) fn assign_unique_piece_names(doc: &mut svg_dom::Document) -> PieceNaming {
    // The pattern group lists the sizes of the measurement file; empty for an individual pattern.
    let sizes = crate::piece_extractor::read_measurements_info(doc).sizes;
    let mut state = NamingState::default();
    rename_children(&mut doc.root, &mut state);
    PieceNaming { renamed: state.renamed, warning: piece_count_warning(&sizes, &state.counts) }
} // fn assign_unique_piece_names

// @brief Running totals of one naming pass.
#[derive(Default)]
struct NamingState {
    // Pieces seen per size, in first-seen order; `None` is the size of an individual piece.
    counts: Vec<(Option<String>, usize)>,
    // Pieces renamed so far.
    renamed: usize,
    // Name without the size of the first renamed piece in the current piece-set.
    set_name: Option<String>,
}

impl NamingState {
    // @brief Count one more piece of `size`.
    // @return The piece's 1-based number among the pieces of its size.
    fn next_number(&mut self, size: &Option<String>) -> usize {
        match self.counts.iter_mut().find(|(s, _)| s == size) {
            Some((_, count)) => {
                *count += 1;
                *count
            } // Some: size seen before
            None => {
                self.counts.push((size.clone(), 1));
                1
            } // None: first piece of this size
        } // match size
    } // fn next_number
} // impl NamingState

// @brief Rename the pieces below `parent`, in document order.
// @details A piece group is renamed and not searched further: its children are its
//          components. A piece-set is searched, then named after its first piece.
fn rename_children(parent: &mut Element, state: &mut NamingState) {
    for node in parent.children.iter_mut() {
        let XMLNode::Element(child) = node else {
            continue;
        }; // if not an element
        if crate::piece_extractor::is_tagged_piece(child) {
            rename_piece(child, state);
        } else if svg_dom::is_piece_set(child) {
            // The sizes inside the set give it its name.
            state.set_name = None;
            rename_children(child, state);
            if let Some(name) = state.set_name.take().filter(|_| !is_name_assigned(child)) {
                child.attributes.insert("data-name".to_string(), name);
                child.attributes.insert(NAME_ASSIGNED_ATTR.to_string(), "true".to_string());
            } // if a piece in the set was renamed
        } else {
            // Pattern group or another wrapper: the pieces are further down.
            rename_children(child, state);
        } // if piece, piece-set or wrapper
    } // for each child
} // fn rename_children

// @brief Give one piece group its unique name.
// @details The piece is counted also when it keeps an assigned name, so the pieces
//          after it keep their document positions as numbers.
fn rename_piece(piece: &mut Element, state: &mut NamingState) {
    let size = attr(piece, "data-size");
    let number = state.next_number(&size);
    if is_name_assigned(piece) {
        return;
    } // if named by an earlier import

    let base = piece_base_name(number, attr(piece, "data-name").as_deref(), attr(piece, "data-letter").as_deref());
    let name = match &size {
        Some(size) => format!("{size}_{base}"),
        None => base.clone(),
    }; // match size

    // Components name their piece in `data-parent`; keep them pointing at it.
    for node in piece.children.iter_mut() {
        if let XMLNode::Element(component) = node {
            if let Some(parent) = component.attributes.get_mut("data-parent") {
                *parent = name.clone();
            } // if the component names its piece
        } // if element
    } // for each component

    piece.attributes.insert("data-name".to_string(), name);
    piece.attributes.insert(NAME_ASSIGNED_ATTR.to_string(), "true".to_string());
    state.renamed += 1;
    state.set_name.get_or_insert(base);
} // fn rename_piece

// @brief `Piece_<number>`, then `_<name>` and `_<letter>` for each one the piece has.
fn piece_base_name(number: usize, name: Option<&str>, letter: Option<&str>) -> String {
    match (name, letter) {
        (None, None) => format!("Piece_{number}"),
        (None, Some(letter)) => format!("Piece_{number}_{letter}"),
        (Some(name), None) => format!("Piece_{number}_{name}"),
        (Some(name), Some(letter)) => format!("Piece_{number}_{name}_{letter}"),
    } // match name and letter
} // fn piece_base_name

// @brief Trimmed attribute value; None when absent or blank.
fn attr(elem: &Element, name: &str) -> Option<String> {
    elem.attributes.get(name).map(|v| v.trim().to_string()).filter(|v| !v.is_empty())
} // fn attr

// @brief Warning text when the sizes of a multisize pattern hold different piece counts.
// @param sizes  Sizes from the pattern group's `data-sizes`.
// @param counts Pieces counted per size by the naming pass.
// @return None when every size has the same count, or the pattern is individual.
fn piece_count_warning(sizes: &[String], counts: &[(Option<String>, usize)]) -> Option<String> {
    let count_of = |size: &str| counts.iter().find(|(s, _)| s.as_deref() == Some(size)).map_or(0, |(_, n)| *n);
    // Every listed size, then each size a piece carries that the list does not name.
    let mut per_size: Vec<(&str, usize)> = sizes.iter().map(|s| (s.as_str(), count_of(s))).collect();
    for (size, count) in counts {
        if let Some(size) = size.as_deref().filter(|s| !sizes.iter().any(|z| z == s)) {
            per_size.push((size, *count));
        } // if a size the list does not name
    } // for each counted size

    // The count most sizes have; a tie goes to the count seen first.
    let mut usual = per_size.first()?.1;
    let mut usual_sizes = 0;
    for (_, count) in &per_size {
        let sizes_with_count = per_size.iter().filter(|(_, n)| n == count).count();
        if sizes_with_count > usual_sizes {
            usual = *count;
            usual_sizes = sizes_with_count;
        } // if more sizes have this count
    } // for each size
    let other: Vec<String> = per_size
        .iter()
        .filter(|(_, n)| *n != usual)
        .map(|(size, n)| format!("\u{2022} Size {size}: {n} pieces"))
        .collect();
    if other.is_empty() {
        return None;
    } // if every size has the usual count

    let mut lines = vec![format!("Most sizes have {usual} pieces. These sizes do not:"), String::new()];
    lines.extend(other);
    lines.push(String::new());
    lines.push("A piece number in these sizes can belong to another piece.".to_string());
    Some(lines.join("\n"))
} // fn piece_count_warning

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Every `data-type="piece"` and piece-set `data-name`, in document order.
    fn names(doc: &svg_dom::Document) -> Vec<String> {
        fn walk(elem: &Element, out: &mut Vec<String>) {
            for node in &elem.children {
                if let XMLNode::Element(child) = node {
                    let tagged = crate::piece_extractor::is_tagged_piece(child) || svg_dom::is_piece_set(child);
                    if tagged {
                        out.push(child.attributes.get("data-name").cloned().unwrap_or_default());
                    } // if piece or piece-set
                    walk(child, out);
                } // if element
            } // for each child
        } // fn walk
        let mut out = Vec::new();
        walk(&doc.root, &mut out);
        out
    } // fn names

    const INDIVIDUAL_SVG: &str = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="pattern-1" data-type="pattern" data-measurements="individual">
    <g id="piece-1" data-type="piece" data-parent="pattern-1">
      <g id="piece-1-cutline-1" data-type="cutline" data-parent="piece-1"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
    </g>
    <g id="piece-2" data-type="piece" data-parent="pattern-1" data-letter="B"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
    <g id="piece_Front" data-type="piece" data-parent="pattern-1" data-name="Front">
      <g id="cutline_Front" data-type="cutline" data-parent="Front"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
    </g>
    <g id="piece_Back" data-type="piece" data-parent="pattern-1" data-name=" Back " data-letter="D"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
  </g>
</svg>"#;

    // @brief Individual pattern: each combination of name and letter gives its own form.
    #[test]
    fn individual_pieces_get_numbered_names() {
        let mut doc = svg_dom::Document::parse(INDIVIDUAL_SVG).expect("parse ok");
        let naming = assign_unique_piece_names(&mut doc);
        assert_eq!(naming, PieceNaming { renamed: 4, warning: None });
        assert_eq!(names(&doc), vec!["Piece_1", "Piece_2_B", "Piece_3_Front", "Piece_4_Back_D"]);
    } // individual_pieces_get_numbered_names

    // @brief A component's `data-parent` follows its piece to the new name.
    #[test]
    fn components_name_the_renamed_piece() {
        let mut doc = svg_dom::Document::parse(INDIVIDUAL_SVG).expect("parse ok");
        assign_unique_piece_names(&mut doc);
        assert_eq!(doc.get_attr_by_id("piece-1-cutline-1", "data-parent"), Some("Piece_1"));
        assert_eq!(doc.get_attr_by_id("cutline_Front", "data-parent"), Some("Piece_3_Front"));
        // The piece itself still names the pattern group.
        assert_eq!(doc.get_attr_by_id("piece_Front", "data-parent"), Some("pattern-1"));
    } // components_name_the_renamed_piece

    const MULTISIZE_SVG: &str = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="pattern-1" data-type="pattern" data-measurements="multisize" data-sizes="34,36" data-base-size="36">
    <g id="piece-set_Front" data-type="piece-set" data-type-number="1" data-name="Front" data-letter="A">
      <g id="piece_Front_s34" data-type="piece" data-parent="piece-set_Front" data-name="Front" data-letter="A" data-size="34"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
      <g id="piece_Front_s36" data-type="piece" data-parent="piece-set_Front" data-name="Front" data-letter="A" data-size="36"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
    </g>
    <g id="piece-set_Detail" data-type="piece-set" data-type-number="2" data-name="Detail">
      <g id="piece_Detail_s34" data-type="piece" data-parent="piece-set_Detail" data-name="Detail" data-size="34"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
      <g id="piece_Detail_s36" data-type="piece" data-parent="piece-set_Detail" data-name="Detail" data-size="36"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
    </g>
    <g id="piece-set-3" data-type="piece-set" data-type-number="3" data-name="Detail">
      <g id="piece_Detail_s34-5" data-type="piece" data-parent="piece-set-3" data-name="Detail" data-size="34"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
      <g id="piece_Detail_s36-6" data-type="piece" data-parent="piece-set-3" data-name="Detail" data-size="36"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
    </g>
  </g>
</svg>"#;

    // @brief Multisize pattern: the size leads each name, the number counts within the size,
    // and two pieces named "Detail" get different names. A piece-set drops the size.
    #[test]
    fn multisize_pieces_get_size_and_number() {
        let mut doc = svg_dom::Document::parse(MULTISIZE_SVG).expect("parse ok");
        let naming = assign_unique_piece_names(&mut doc);
        assert_eq!(naming, PieceNaming { renamed: 6, warning: None });
        assert_eq!(
            names(&doc),
            vec![
                "Piece_1_Front_A", "34_Piece_1_Front_A", "36_Piece_1_Front_A",
                "Piece_2_Detail", "34_Piece_2_Detail", "36_Piece_2_Detail",
                "Piece_3_Detail", "34_Piece_3_Detail", "36_Piece_3_Detail",
            ]
        );
        // The piece-set id still groups the sizes of one piece.
        assert_eq!(doc.get_attr_by_id("piece_Detail_s36-6", "data-parent"), Some("piece-set-3"));
    } // multisize_pieces_get_size_and_number

    // @brief A second pass keeps the assigned names: a re-imported export is not numbered again.
    #[test]
    fn assigned_names_are_kept() {
        let mut doc = svg_dom::Document::parse(MULTISIZE_SVG).expect("parse ok");
        assign_unique_piece_names(&mut doc);
        let first = names(&doc);
        let again = assign_unique_piece_names(&mut doc);
        assert_eq!(again, PieceNaming { renamed: 0, warning: None });
        assert_eq!(names(&doc), first);
    } // assigned_names_are_kept

    // @brief A size with a missing piece is named in a warning; the names are still assigned.
    #[test]
    fn different_piece_counts_give_a_warning() {
        let svg = MULTISIZE_SVG.replace(
            r#"<g id="piece_Detail_s36-6" data-type="piece" data-parent="piece-set-3" data-name="Detail" data-size="36"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>"#,
            "",
        );
        let mut doc = svg_dom::Document::parse(&svg).expect("parse ok");
        let naming = assign_unique_piece_names(&mut doc);
        assert_eq!(naming.renamed, 5);
        let warning = naming.warning.expect("size 36 has one piece less");
        assert!(warning.contains("Most sizes have 3 pieces"), "{warning}");
        assert!(warning.contains("\u{2022} Size 36: 2 pieces"), "{warning}");
        assert!(!warning.contains("Size 34"), "{warning}");
    } // different_piece_counts_give_a_warning

    // @brief An untagged drawing keeps its groups as they are.
    #[test]
    fn untagged_svg_is_unchanged() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="drawing"><path d="M 0 0 L 9 0 L 9 9 Z"/></g>
</svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("parse ok");
        assert_eq!(assign_unique_piece_names(&mut doc), PieceNaming::default());
        assert_eq!(doc.get_attr_by_id("drawing", "data-name"), None);
    } // untagged_svg_is_unchanged
} // mod tests
