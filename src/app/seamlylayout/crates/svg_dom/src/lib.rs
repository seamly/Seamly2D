// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

mod transforms;
pub use transforms::{flatten_dom, parse_svg_transform, serialize_path, translate_dom, verticalize_dom};

use std::io::Cursor;
use xmltree::{Element, XMLNode};

// Thin, editable SVG DOM wrapper geared toward Qt-to-Rust parity tests.
#[derive(Debug, Clone)]
pub struct Document {
    pub root: Element,
}

impl Document {
    pub fn parse(input: &str) -> Result<Self, xmltree::ParseError> {
        let root = Element::parse(Cursor::new(input))?;
        Ok(Self { root })
    }

    pub fn to_string(&self) -> String {
        let mut buf = Vec::new();
        let mut root = self.root.clone();
        strip_nested_namespace_decls(&mut root, true);
        sort_attributes_for_serialization(&mut root);
        root
            .write_with_config(&mut buf, xmltree::EmitterConfig::new().perform_indent(true))
            .expect("serialize svg");
        String::from_utf8(buf).unwrap_or_default()
    }

    pub fn sanitize_for_rendering(&mut self) -> Vec<String> {
        let mut warnings = Vec::new();
        sanitize_element_children(&mut self.root, &mut warnings);
        warnings
    }

    pub fn find_invalid_render_nesting(&self) -> Vec<String> {
        let mut warnings = Vec::new();
        collect_invalid_render_nesting(&self.root, &mut warnings);
        warnings
    }

    pub fn get_attr_ns<'a>(
        &'a self,
        path: &[&str],
        ns_prefix: &str,
        name: &str,
    ) -> Option<&'a str> {
        let key = format!("{ns_prefix}:{name}");
        if let Some(v) = self.get_attr(path, &key) {
            return Some(v);
        }
        // xmltree may store namespaced attributes with full URI or different prefix;
        // fall back to suffix match on local name.
        let mut node = &self.root;
        for tag in path {
            match node.get_child(*tag) {
                Some(child) => node = child,
                None => return None,
            }
        }
        node.attributes
            .iter()
            .find(|(k, _)| k.ends_with(name))
            .map(|(_, v)| v.as_str())
    }

    // Returns the first element along the tag path and its attribute (if set).
    pub fn get_attr<'a>(&'a self, path: &[&str], name: &str) -> Option<&'a str> {
        let mut node = &self.root;
        for tag in path {
            match node.get_child(*tag) {
                Some(child) => node = child,
                None => return None,
            }
        }
        node.attributes.get(name).map(|s| s.as_str())
    }

    pub fn set_attr_ns(
        &mut self,
        path: &[&str],
        ns_prefix: &str,
        name: &str,
        value: impl Into<String>,
    ) -> bool {
        let key = format!("{ns_prefix}:{name}");
        self.set_attr(path, &key, value)
    }

    // Updates or inserts an attribute on the first element along the tag path.
    // Returns false if the element path is missing.
    pub fn set_attr(&mut self, path: &[&str], name: &str, value: impl Into<String>) -> bool {
        let Some(node) = find_child_mut(&mut self.root, path) else {
            return false;
        };
        if node.attributes.is_empty() {
            node.attributes = Default::default();
        }
        node.attributes.insert(name.to_string(), value.into());
        true
    }

    // Collects child elements of the given element name from the first path match.
    pub fn children_by_name<'a>(&'a self, path: &[&str], name: &str) -> Option<Vec<&'a Element>> {
        let mut node = &self.root;
        for tag in path {
            match node.get_child(*tag) {
                Some(child) => node = child,
                None => return None,
            }
        }
        let children = node
            .children
            .iter()
            .filter_map(|child| child.as_element())
            .filter(|elem| elem.name == name)
            .collect::<Vec<_>>();
        Some(children)
    }

    // Collect all id attributes depth-first.
    pub fn collect_ids(&self) -> Vec<String> {
        let mut ids = Vec::new();
        collect_ids_rec(&self.root, &mut ids);
        ids
    }

    pub fn get_attr_by_id<'a>(&'a self, id: &str, name: &str) -> Option<&'a str> {
        let node = find_by_id(&self.root, id)?;
        node.attributes.get(name).map(|s| s.as_str())
    }

    pub fn set_attr_by_id(&mut self, id: &str, name: &str, value: impl Into<String>) -> bool {
        if let Some(node) = find_by_id_mut(&mut self.root, id) {
            node.attributes.insert(name.to_string(), value.into());
            true
        } else {
            false
        }
    }

    // @brief Returns a mutable reference to the first element whose id attribute matches.
    // Needed by callers that must mutate an element's children (e.g. pop empty tile rows).
    pub fn get_element_by_id_mut(&mut self, id: &str) -> Option<&mut xmltree::Element> {
        find_by_id_mut(&mut self.root, id)
    }

    // Prefix every id with the given prefix (if not already prefixed).
    pub fn ensure_id_prefixed(&mut self, prefix: &str) {
        ensure_id_prefixed_rec(&mut self.root, prefix);
    }

    // @brief Adds a white background rectangle as the first child of the SVG root element.
    // @details The rectangle uses the SVG's width and height attributes, or defaults to "100" if missing.
    //          The rectangle has fill="white" and stroke="none" to serve as a background.
    // @return true if the background rectangle was added, false if width/height could not be determined.
    pub fn add_background_rect(&mut self) -> bool {
        // Get width and height from the SVG root element.
        let width = self.get_attr(&[], "width").unwrap_or("100").to_string();
        let height = self.get_attr(&[], "height").unwrap_or("100").to_string();

        // Create a rectangle element directly (not by parsing XML).
        let mut rect = Element {
            name: "rect".to_string(),
            attributes: Default::default(),
            children: Vec::new(),
            namespace: None,
            prefix: None,
            namespaces: None,
        };

        // Set rectangle attributes to match SVG dimensions and fill with white.
        rect.attributes.insert("x".to_string(), "0".to_string());
        rect.attributes.insert("y".to_string(), "0".to_string());
        rect.attributes.insert("width".to_string(), width);
        rect.attributes.insert("height".to_string(), height);
        rect.attributes
            .insert("fill".to_string(), "white".to_string());
        rect.attributes
            .insert("stroke".to_string(), "none".to_string());

        // Insert the rectangle as the first child of the SVG root.
        self.root.children.insert(0, XMLNode::Element(rect));
        true
    }
}

fn find_child_mut<'a>(element: &'a mut Element, path: &[&str]) -> Option<&'a mut Element> {
    if path.is_empty() {
        return Some(element);
    }
    let (head, tail) = path.split_first().unwrap();
    for child in element.children.iter_mut() {
        if let XMLNode::Element(elem) = child {
            if elem.name == *head {
                return find_child_mut(elem, tail);
            }
        }
    }
    None
}

fn sanitize_element_children(element: &mut Element, warnings: &mut Vec<String>) -> Vec<XMLNode> {
    let mut normalized_children = Vec::new();

    for child in std::mem::take(&mut element.children) {
        match child {
            XMLNode::Element(mut child_elem) => {
                let mut hoisted = sanitize_element_children(&mut child_elem, warnings);
                normalized_children.push(XMLNode::Element(child_elem));
                normalized_children.append(&mut hoisted);
            }
            other => normalized_children.push(other),
        }
    }

    if svg_leaf_element(&element.name) {
        let mut retained_children = Vec::new();
        let mut hoisted_children = Vec::new();

        for child in normalized_children {
            match child {
                XMLNode::Element(ref child_elem) => {
                    warnings.push(format!(
                        "Invalid SVG nesting repaired: <{} id='{}'> cannot contain <{} id='{}'>",
                        element.name,
                        element.attributes.get("id").map(String::as_str).unwrap_or(""),
                        child_elem.name,
                        child_elem.attributes.get("id").map(String::as_str).unwrap_or(""),
                    ));
                    hoisted_children.push(child);
                }
                other => retained_children.push(other),
            }
        }

        element.children = retained_children;
        hoisted_children
    } else {
        element.children = normalized_children;
        Vec::new()
    }
}

fn strip_nested_namespace_decls(element: &mut Element, is_root: bool) {
    if !is_root {
        element.attributes.retain(|key, _| key != "xmlns" && !key.starts_with("xmlns:"));
        element.namespaces = None;
    }

    for child in &mut element.children {
        if let XMLNode::Element(child_elem) = child {
            strip_nested_namespace_decls(child_elem, false);
        }
    }
}

fn sort_attributes_for_serialization(element: &mut Element) {
    let old_attributes = std::mem::take(&mut element.attributes);
    let mut sorted_attributes: Vec<_> = old_attributes.into_iter().collect();
    sorted_attributes.sort_by(|(left_key, _), (right_key, _)| match (left_key.as_str(), right_key.as_str()) {
        ("id", "id") => std::cmp::Ordering::Equal,
        ("id", _) => std::cmp::Ordering::Less,
        (_, "id") => std::cmp::Ordering::Greater,
        _ => left_key.cmp(right_key),
    });
    for (key, value) in sorted_attributes {
        element.attributes.insert(key, value);
    }

    for child in &mut element.children {
        if let XMLNode::Element(child_elem) = child {
            sort_attributes_for_serialization(child_elem);
        }
    }
}

fn svg_leaf_element(name: &str) -> bool {
    matches!(
        name,
        "path" | "rect" | "line" | "circle" | "ellipse" | "polyline" | "polygon" | "image"
    )
}

fn collect_invalid_render_nesting(element: &Element, warnings: &mut Vec<String>) {
    if svg_leaf_element(&element.name) {
        for child in &element.children {
            if let XMLNode::Element(child_elem) = child {
                warnings.push(format!(
                    "Invalid SVG nesting detected: <{} id='{}'> contains <{} id='{}'>",
                    element.name,
                    element.attributes.get("id").map(String::as_str).unwrap_or(""),
                    child_elem.name,
                    child_elem.attributes.get("id").map(String::as_str).unwrap_or(""),
                ));
            }
        }
    }

    for child in &element.children {
        if let XMLNode::Element(child_elem) = child {
            collect_invalid_render_nesting(child_elem, warnings);
        }
    }
}

fn collect_ids_rec(element: &Element, out: &mut Vec<String>) {
    if let Some(id) = element.attributes.get("id") {
        out.push(id.clone());
    }
    for child in &element.children {
        if let XMLNode::Element(elem) = child {
            collect_ids_rec(elem, out);
        }
    }
}

fn ensure_id_prefixed_rec(element: &mut Element, prefix: &str) {
    if let Some(id) = element.attributes.get_mut("id") {
        if !id.starts_with(prefix) {
            *id = format!("{prefix}{id}");
        }
    }
    for child in element.children.iter_mut() {
        if let XMLNode::Element(elem) = child {
            ensure_id_prefixed_rec(elem, prefix);
        }
    }
}

fn find_by_id<'a>(element: &'a Element, id: &str) -> Option<&'a Element> {
    if element.attributes.get("id").map(|v| v.as_str()) == Some(id) {
        return Some(element);
    }
    for child in &element.children {
        if let XMLNode::Element(elem) = child {
            if let Some(found) = find_by_id(elem, id) {
                return Some(found);
            }
        }
    }
    None
}

fn find_by_id_mut<'a>(element: &'a mut Element, id: &str) -> Option<&'a mut Element> {
    if element.attributes.get("id").map(|v| v.as_str()) == Some(id) {
        return Some(element);
    }
    for child in element.children.iter_mut() {
        if let XMLNode::Element(elem) = child {
            if let Some(found) = find_by_id_mut(elem, id) {
                return Some(found);
            }
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;

    const FIXTURE: &str = r#"
<svg width="100" height="100">
  <g id="root" transform="translate(5 5)">
    <rect id="r1" width="10" height="20"/>
    <rect id="r2" width="8" height="12"/>
  </g>
</svg>
"#;

    #[test]
    fn parse_and_get_attr() {
        let doc = Document::parse(FIXTURE).unwrap();
        let t = doc.get_attr(&["g"], "transform");
        assert_eq!(t, Some("translate(5 5)"));
    }

    #[test]
    fn set_attr_updates_dom() {
        let mut doc = Document::parse(FIXTURE).unwrap();
        let ok = doc.set_attr(&["g", "rect"], "fill", "red");
        assert!(ok);
        let fill = doc.get_attr(&["g", "rect"], "fill");
        assert_eq!(fill, Some("red"));
    }

    #[test]
    fn children_filtering() {
        let doc = Document::parse(FIXTURE).unwrap();
        let rects = doc.children_by_name(&["g"], "rect").unwrap();
        assert_eq!(rects.len(), 2);
        assert_eq!(rects[0].attributes.get("id").unwrap(), "r1");
    }

    #[test]
    fn namespaces_and_ids_from_fixture() {
        let path = format!(
            "{}/../../qt_frontend/assets/images/icon-gear.svg",
            env!("CARGO_MANIFEST_DIR")
        );
        let xml = fs::read_to_string(path).expect("read icon fixture");
        let mut doc = Document::parse(&xml).unwrap();

        // Namespaced attr
        let docname = doc.get_attr_ns(&[], "sodipodi", "docname");
        assert_eq!(docname, Some("icon-gear.svg"));

        // ID collection
        let ids = doc.collect_ids();
        assert!(ids.iter().any(|id| id == "gear"));

        // Prefix ids — every id should now start with "qt_".
        doc.ensure_id_prefixed("qt_");
        let ids2 = doc.collect_ids();
        assert!(ids2.iter().all(|id| id.starts_with("qt_")));
    }

    #[test]
    fn deep_id_lookup_on_logo() {
        let path = format!(
            "{}/../../qt_frontend/assets/images/seamly-layout.svg",
            env!("CARGO_MANIFEST_DIR")
        );
        let xml = fs::read_to_string(path).expect("read logo fixture");
        let mut doc = Document::parse(&xml).unwrap();

        let cx = doc.get_attr_by_id("Circle", "cx");
        assert_eq!(cx, Some("575.22125"));

        let set_ok = doc.set_attr_by_id("Circle", "data-test", "ok");
        assert!(set_ok);
        let cx_again = doc.get_attr_by_id("Circle", "data-test");
        assert_eq!(cx_again, Some("ok"));
    }

    #[test]
    fn to_string_keeps_namespace_decls_only_on_svg_root() {
        let xml = r#"
<svg xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd" width="100" height="100">
  <g xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd" id="PocketFlap">
    <path d="M 0 0 L 10 0" />
  </g>
</svg>
"#;

        let doc = Document::parse(xml).unwrap();
        let serialized = doc.to_string();

        assert!(serialized.contains("<svg"));
        assert!(serialized.contains("xmlns=\"http://www.w3.org/2000/svg\""));
        assert!(serialized.contains("xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\""));
        assert!(serialized.contains("xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\""));
        assert!(serialized.contains("<g id=\"PocketFlap\">"));
        assert!(!serialized.contains("<g xmlns="));
        assert!(!serialized.contains("<g xmlns:inkscape="));
        assert!(!serialized.contains("<g xmlns:sodipodi="));
    }

    #[test]
    fn to_string_serializes_id_first_then_remaining_attributes_alphabetically() {
        let xml = r#"
<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
  <g stroke="black" transform="translate(1 2)" id="PocketFlap" fill="none" stroke-width="1"/>
</svg>
"#;

        let doc = Document::parse(xml).unwrap();
        let serialized = doc.to_string();

        assert!(serialized.contains(
            r#"<g id="PocketFlap" fill="none" stroke="black" stroke-width="1" transform="translate(1 2)" />"#
        ));
    }
}
