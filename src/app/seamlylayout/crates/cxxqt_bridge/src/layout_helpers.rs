// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file layout_helpers.rs
// @brief Layout helper functions for SVG DOM manipulation.

use xmltree::{Element as XmlElement, XMLNode};

/// Remove a group element by id from the XML element tree.
/// Recursively searches for a <g> with matching id and removes it.
/// Returns true if a group was removed, false otherwise.
pub fn remove_group_by_id(element: &mut XmlElement, id: &str) -> bool {
    let mut removed = false;

    element.children.retain(|child| {
        if let XMLNode::Element(elem) = child {
            if elem.name == "g" {
                if let Some(elem_id) = elem.attributes.get("id") {
                    if elem_id == id {
                        removed = true;
                        return false;
                    }
                }
            }
        }
        true
    });

    for child in element.children.iter_mut() {
        if let XMLNode::Element(child_elem) = child {
            if remove_group_by_id(child_elem, id) {
                removed = true;
            }
        }
    }

    removed
}
