// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file material_split.rs
// @brief Splits a Seamly2D handoff into one document per material.
//
// Seamly2D hands off one `<g data-type="pattern" data-material="...">` group per
// material the pieces are cut from (Fabric, Lining, Interfacing, Interlining).
// Each material gets its own document: the root and every element that is not a
// pattern group, plus the pattern groups of that material. Each document then
// has the shape of a one-material handoff, so the import and layout pipeline
// treat it as before.

use xmltree::XMLNode;

// @brief One material and the document that holds its pieces.
#[derive(Debug, Clone)]
pub(crate) struct MaterialDoc {
    // `data-material` of the pattern groups; empty when the handoff names no material.
    pub name: String,
    pub doc: svg_dom::Document,
}

// @brief Split `doc` into one document per material, in the order the materials first appear.
// @param doc Imported document.
// @return One entry per material; a single entry, holding `doc` unchanged, when the
//         document has fewer than two materials.
pub(crate) fn split_by_material(doc: svg_dom::Document) -> Vec<MaterialDoc> {
    let materials = pattern_materials(&doc);
    if materials.len() < 2 {
        let name = materials.into_iter().next().unwrap_or_default();
        return vec![MaterialDoc { name, doc }];
    } // if one material

    materials.into_iter()
        .map(|name| {
            let mut part = doc.clone();
            // Keep every node except the pattern groups of other materials.
            part.root.children.retain(|node| match node {
                XMLNode::Element(e) if is_pattern(e) => material_of(e) == name,
                _ => true,
            }); // retain
            MaterialDoc { name, doc: part }
        })
        .collect()
} // fn split_by_material

// @brief Distinct `data-material` values of the top-level pattern groups, in document order.
fn pattern_materials(doc: &svg_dom::Document) -> Vec<String> {
    let mut materials: Vec<String> = Vec::new();
    for group in doc.root.children.iter().filter_map(|n| n.as_element()).filter(|e| is_pattern(e)) {
        let name = material_of(group);
        if !materials.contains(&name) {
            materials.push(name);
        } // if new material
    } // for group
    materials
} // fn pattern_materials

// @brief True for a `<g data-type="pattern">`.
fn is_pattern(elem: &xmltree::Element) -> bool {
    elem.name == "g" && elem.attributes.get("data-type").is_some_and(|t| t == "pattern")
} // fn is_pattern

// @brief The group's `data-material`, or empty.
fn material_of(elem: &xmltree::Element) -> String {
    elem.attributes.get("data-material").cloned().unwrap_or_default()
} // fn material_of

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    const TWO_MATERIALS: &str = r#"<svg xmlns="http://www.w3.org/2000/svg" width="400" height="400">
  <defs/>
  <g id="pattern-1" data-type="pattern" data-type-number="1" data-material="Fabric" data-measurements="individual">
    <g id="piece_Front" data-type="piece" data-material="Fabric" data-parent="pattern-1"><path d="M0,0 L10,0 L10,10 Z"/></g>
    <g id="piece_Back" data-type="piece" data-material="Fabric" data-parent="pattern-1"><path d="M0,0 L10,0 L10,10 Z"/></g>
  </g>
  <g id="pattern-2" data-type="pattern" data-type-number="2" data-material="Lining" data-measurements="individual">
    <g id="piece_Front_Lining" data-type="piece" data-material="Lining" data-parent="pattern-2"><path d="M0,0 L9,0 L9,9 Z"/></g>
  </g>
</svg>"#;

    // @brief The ids of the pieces in a document.
    fn piece_ids(doc: &svg_dom::Document) -> Vec<String> {
        let mut ids = Vec::new();
        for group in doc.root.children.iter().filter_map(|n| n.as_element()) {
            for piece in group.children.iter().filter_map(|n| n.as_element()) {
                ids.push(piece.attributes.get("id").cloned().unwrap_or_default());
            } // for piece
        } // for group
        ids
    } // fn piece_ids

    // @brief Each material gets a document with its own pieces, in handoff order.
    #[test]
    fn each_material_gets_its_own_document() {
        let parts = split_by_material(svg_dom::Document::parse(TWO_MATERIALS).expect("valid SVG"));
        let names: Vec<&str> = parts.iter().map(|p| p.name.as_str()).collect();
        assert_eq!(names, ["Fabric", "Lining"]);
        assert_eq!(piece_ids(&parts[0].doc), ["piece_Front", "piece_Back"]);
        assert_eq!(piece_ids(&parts[1].doc), ["piece_Front_Lining"]);
        // The root, its size and <defs> stay in every document.
        for part in &parts {
            assert_eq!(part.doc.root.attributes.get("width").map(String::as_str), Some("400"));
            assert!(part.doc.root.children.iter().filter_map(|n| n.as_element()).any(|e| e.name == "defs"));
            assert_eq!(crate::piece_extractor::count_tagged_pieces(&part.doc), piece_ids(&part.doc).len());
        } // for part
    } // each_material_gets_its_own_document

    // @brief One material, or an older handoff without materials, stays one document.
    #[test]
    fn one_material_stays_one_document() {
        let one = TWO_MATERIALS.replace(r#"data-material="Lining" data-measurements"#, r#"data-material="Fabric" data-measurements"#);
        let parts = split_by_material(svg_dom::Document::parse(&one).expect("valid SVG"));
        assert_eq!(parts.len(), 1);
        assert_eq!(parts[0].name, "Fabric");
        assert_eq!(piece_ids(&parts[0].doc).len(), 3);

        let old = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="pattern-1" data-type="pattern"><g id="p" data-type="piece"/></g></svg>"#;
        let parts = split_by_material(svg_dom::Document::parse(old).expect("valid SVG"));
        assert_eq!((parts.len(), parts[0].name.as_str()), (1, ""));

        let plain = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="drawing"><path d="M0,0 L1,1"/></g></svg>"#;
        assert_eq!(split_by_material(svg_dom::Document::parse(plain).expect("valid SVG")).len(), 1);
    } // one_material_stays_one_document

    // @brief A material's layout holds that material's pieces only.
    #[test]
    fn material_layout_holds_its_own_pieces() {
        let parts = split_by_material(svg_dom::Document::parse(TWO_MATERIALS).expect("valid SVG"));
        let settings = serde_json::json!({
            "unit": "in", "mediaType": "fabric", "paperType": "roll",
            "pageWidth": 20.0, "pageHeight": 40.0,
            "marginLeft": 0.5, "marginRight": 0.5, "marginTop": 0.5, "marginBottom": 0.5,
            "pieceGap": 0.125, "layoutMode": "alongGrainline",
            "tileSize": "Letter", "tileOrientation": "Portrait"
        }).to_string();
        for (part, expected) in parts.iter().zip([vec!["piece_Front", "piece_Back"], vec!["piece_Front_Lining"]]) {
            let init = crate::layout_utils::do_initialize_layout(&settings, Some(&part.doc)).expect("initialize");
            let mut progress = |_pct: i32, _status: Option<&str>| {};
            let layout = crate::layout_utils::do_process_layout(
                crate::layout_utils::ProcessLayoutArgs {
                    settings_json: &settings,
                    input_dom: &part.doc,
                    initial_layout_dom: &init.initial_dom,
                    layout_h_px: init.h_px,
                    nested_multisize: false,
                    extend_media: false,
                    split_oversized: false,
                },
                &mut progress,
            ).expect("layout");
            let bbox: serde_json::Value = serde_json::from_str(&layout.bbox_json).expect("bbox JSON");
            let mut placed: Vec<&str> = bbox["pieces"].as_array().expect("pieces").iter()
                .map(|p| p["id"].as_str().unwrap_or_default()).collect();
            placed.sort();
            let mut expected = expected;
            expected.sort();
            assert_eq!(placed, expected, "material {}", part.name);
        } // for part
    } // material_layout_holds_its_own_pieces
} // mod tests
