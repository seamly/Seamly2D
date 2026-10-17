// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file size_labels.rs
// @brief Adds a "Size <size>" label to every piece of a multisize handoff at import.
//
// The label is one line of text in a `<g data-type="size_label">` group, the
// last child of the piece. Its bottom is 5 px past the front end of the
// grainline, and the text reads across the grainline, so it is level when the
// layout turns the grainline vertical. A piece without a grainline gets the
// label, level, at the centre of its outline.
//
// The grainline runs from the first point of its path to the last, as
// `svg_dom::verticalize_dom` reads it; the last point is the front end.
// A piece that has a size label already, from an earlier SeamlyLayout export,
// keeps it.

use geometry::{Matrix2D, Path, PathSegment, Point};
use xmltree::{Element, XMLNode};

// @brief `data-type` of a size label group.
pub(crate) const SIZE_LABEL_TYPE: &str = "size_label";

// Label text size, and the space between the grainline end and the text, in pixels.
const SIZE_LABEL_FONT_PX: f64 = 24.0;
const SIZE_LABEL_GAP_PX: f64 = 5.0;

// @brief Add a size label to every `data-type="piece"` group with a `data-size`.
// @param doc Imported document; changed in place.
// @return Number of labels added.
pub(crate) fn add_size_labels(doc: &mut svg_dom::Document) -> usize {
    let mut added = 0;
    add_in_subtree(&mut doc.root, &mut added);
    added
} // fn add_size_labels

// @brief Recursive step of `add_size_labels`; a piece is not searched for nested pieces.
fn add_in_subtree(elem: &mut Element, added: &mut usize) {
    for child in elem.children.iter_mut().filter_map(|n| n.as_mut_element()) {
        if is_type(child, "piece") && child.attributes.contains_key("data-size") {
            if add_label(child) {
                *added += 1;
            } // if added
        } else {
            add_in_subtree(child, added);
        } // if sized piece
    } // for child
} // fn add_in_subtree

// @brief True when `elem` is a `<g>` with this `data-type`.
fn is_type(elem: &Element, data_type: &str) -> bool {
    elem.name == "g" && elem.attributes.get("data-type").is_some_and(|t| t == data_type)
} // fn is_type

// @brief Append the size label to one piece.
// @return False when the piece has a size label already or has no geometry.
fn add_label(piece: &mut Element) -> bool {
    let has_label = piece.children.iter().filter_map(|n| n.as_element()).any(|c| is_type(c, SIZE_LABEL_TYPE));
    if has_label {
        return false; // kept from an earlier import
    } // if has_label

    // Baseline centre and text angle: past the grainline, else the outline centre.
    let Some((anchor, angle_deg)) = grainline_placement(piece).or_else(|| centre_placement(piece)) else {
        return false; // no geometry
    }; // placement

    let size = piece.attributes.get("data-size").cloned().unwrap_or_default();
    let piece_id = piece.attributes.get("id").cloned().unwrap_or_default();
    let parent = piece.attributes.get("data-name").cloned().unwrap_or_else(|| piece_id.clone());

    let mut transform = format!("translate({} {})", fmt(anchor.0), fmt(anchor.1));
    if angle_deg.abs() > 0.01 {
        transform.push_str(&format!(" rotate({})", fmt(angle_deg)));
    } // if turned
    let mut text = Element::new("text");
    for (key, value) in [
        ("x", "0".to_string()),
        ("y", "0".to_string()),
        ("text-anchor", "middle".to_string()),
        ("font-family", "Arial".to_string()),
        ("font-size", fmt(SIZE_LABEL_FONT_PX)),
        ("fill", "#000000".to_string()),
        ("stroke", "none".to_string()),
        ("transform", transform),
    ] {
        text.attributes.insert(key.to_string(), value);
    } // for attribute
    text.children.push(XMLNode::Text(format!("Size {size}")));

    let mut group = Element::new("g");
    group.attributes.insert("id".to_string(), format!("size_label_{piece_id}"));
    group.attributes.insert("data-type".to_string(), SIZE_LABEL_TYPE.to_string());
    group.attributes.insert("data-type-number".to_string(), "1".to_string());
    group.attributes.insert("data-parent".to_string(), parent);
    group.children.push(XMLNode::Element(text));
    piece.children.push(XMLNode::Element(group));
    true
} // fn add_label

// @brief Baseline centre 5 px past the front end of the grainline, and the text
// angle in degrees that makes the text read across the grainline.
fn grainline_placement(piece: &Element) -> Option<((f64, f64), f64)> {
    let grainline = piece.children.iter().filter_map(|n| n.as_element()).find(|c| is_type(c, "grainline"))?;
    let (rear, front) = first_path_ends(grainline, Matrix2D::IDENTITY)?;
    let (dx, dy) = ((front.x - rear.x) as f64, (front.y - rear.y) as f64);
    let length = dx.hypot(dy);
    if length < 1e-3 {
        return None; // a point, not a line
    } // if no length
    let (ux, uy) = (dx / length, dy / length);
    let anchor = (front.x as f64 + ux * SIZE_LABEL_GAP_PX, front.y as f64 + uy * SIZE_LABEL_GAP_PX);
    // Text "up" is the grainline direction; the baseline runs at a right angle to it.
    Some((anchor, ux.atan2(-uy).to_degrees()))
} // fn grainline_placement

// @brief First and last point of the first `<path>` in `elem`, in the piece frame.
fn first_path_ends(elem: &Element, parent: Matrix2D) -> Option<(Point, Point)> {
    let matrix = parent.mul(&own_transform(elem));
    if elem.name == "path" {
        let path = Path::parse_path_attribute(elem.attributes.get("d")?).ok()?;
        let points: Vec<Point> = path.segments.iter().filter_map(segment_end).collect();
        let (first, last) = (*points.first()?, *points.last()?);
        return Some((matrix.apply_to_point(first), matrix.apply_to_point(last)));
    } // if path
    elem.children.iter().filter_map(|n| n.as_element()).find_map(|c| first_path_ends(c, matrix))
} // fn first_path_ends

// @brief Level text at the centre of the piece outline, from every path of the piece.
fn centre_placement(piece: &Element) -> Option<((f64, f64), f64)> {
    let mut points = Vec::new();
    for child in piece.children.iter().filter_map(|n| n.as_element()) {
        collect_points(child, Matrix2D::IDENTITY, &mut points);
    } // for child
    let (min_x, max_x, min_y, max_y) = points.iter().fold(
        (f32::MAX, f32::MIN, f32::MAX, f32::MIN),
        |(x0, x1, y0, y1), p| (x0.min(p.x), x1.max(p.x), y0.min(p.y), y1.max(p.y)),
    );
    if points.is_empty() {
        return None;
    } // if no points
    // A third of the font size below the centre puts the middle of the digits there.
    let centre = (((min_x + max_x) / 2.0) as f64, ((min_y + max_y) / 2.0) as f64 + SIZE_LABEL_FONT_PX / 3.0);
    Some((centre, 0.0))
} // fn centre_placement

// @brief Every path point in `elem`'s subtree, in the piece frame.
fn collect_points(elem: &Element, parent: Matrix2D, points: &mut Vec<Point>) {
    let matrix = parent.mul(&own_transform(elem));
    if elem.name == "path" {
        if let Some(path) = elem.attributes.get("d").and_then(|d| Path::parse_path_attribute(d).ok()) {
            points.extend(path.segments.iter().filter_map(segment_end).map(|p| matrix.apply_to_point(p)));
        } // if path data
    } // if path
    for child in elem.children.iter().filter_map(|n| n.as_element()) {
        collect_points(child, matrix, points);
    } // for child
} // fn collect_points

// @brief The element's own `transform`, or identity.
fn own_transform(elem: &Element) -> Matrix2D {
    elem.attributes.get("transform").map(|t| svg_dom::parse_svg_transform(t)).unwrap_or(Matrix2D::IDENTITY)
} // fn own_transform

// @brief End point of a path segment; None for close.
fn segment_end(segment: &PathSegment) -> Option<Point> {
    match segment {
        PathSegment::MoveTo(p) | PathSegment::LineTo(p) => Some(*p),
        PathSegment::QuadTo { to, .. } | PathSegment::CubicTo { to, .. } | PathSegment::ArcTo { to, .. } => Some(*to),
        PathSegment::Close => None,
    } // match segment
} // fn segment_end

// @brief A number with at most two decimals.
fn fmt(value: f64) -> String {
    let text = format!("{value:.2}");
    let text = text.trim_end_matches('0').trim_end_matches('.');
    if text == "-0" { "0".to_string() } else { text.to_string() }
} // fn fmt

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Piece A has a vertical grainline drawn bottom to top, inside a moved group;
    // piece B has none; piece C is an individual piece.
    const HANDOFF: &str = r#"<svg xmlns="http://www.w3.org/2000/svg">
  <g id="pattern-1" data-type="pattern">
    <g id="piece-set-1" data-type="piece-set">
      <g id="piece-1_s34" data-type="piece" data-size="34" data-name="Front">
        <g data-type="cutline"><path d="M0,0 L100,0 L100,200 L0,200 Z"/></g>
        <g data-type="grainline" id="piece-1-grainline-1_s34"><g transform="matrix(1,0,0,1,10,20)"><path d="M40,150 L35,140 L45,140 L40,150 L40,50 L45,60 L35,60 L40,50 "/></g></g>
      </g>
      <g id="piece-2_s34" data-type="piece" data-size="34">
        <g data-type="cutline"><path d="M0,0 L60,0 L60,30 L0,30 Z"/></g>
      </g>
    </g>
  </g>
  <g id="piece-3" data-type="piece"><g data-type="cutline"><path d="M0,0 L1,1"/></g></g>
</svg>"#;

    // @brief The size label of the piece with this id.
    fn label<'a>(elem: &'a Element, piece_id: &str) -> Option<&'a Element> {
        if elem.attributes.get("id").is_some_and(|id| id == piece_id) {
            return elem.children.iter().filter_map(|n| n.as_element()).find(|c| is_type(c, SIZE_LABEL_TYPE));
        } // if piece
        elem.children.iter().filter_map(|n| n.as_element()).find_map(|c| label(c, piece_id))
    } // fn label

    // @brief The single `<text>` of a label: its transform and its string.
    fn text_of(group: &Element) -> (String, String) {
        let text = group.children.iter().filter_map(|n| n.as_element()).find(|e| e.name == "text").expect("text");
        (text.attributes["transform"].clone(), text.get_text().unwrap_or_default().to_string())
    } // fn text_of

    // @brief The label sits 5 px past the grainline's front end, level for an upward grainline.
    #[test]
    fn label_sits_above_the_grainline() {
        let mut doc = svg_dom::Document::parse(HANDOFF).expect("valid SVG");
        assert_eq!(add_size_labels(&mut doc), 2, "two sized pieces; the individual piece gets none");
        let group = label(&doc.root, "piece-1_s34").expect("label on piece 1");
        assert_eq!(group.attributes["data-parent"], "Front");
        assert_eq!(group.attributes["id"], "size_label_piece-1_s34");
        // Front end (40, 50) moved by (10, 20) is (50, 70); 5 px further up is (50, 65).
        assert_eq!(text_of(group), ("translate(50 65)".to_string(), "Size 34".to_string()));
        assert!(label(&doc.root, "piece-3").is_none());
    } // label_sits_above_the_grainline

    // @brief Without a grainline the label is level at the centre of the outline.
    #[test]
    fn label_without_grainline_is_centred() {
        let mut doc = svg_dom::Document::parse(HANDOFF).expect("valid SVG");
        add_size_labels(&mut doc);
        let group = label(&doc.root, "piece-2_s34").expect("label on piece 2");
        assert_eq!(text_of(group).0, "translate(30 23)");
    } // label_without_grainline_is_centred

    // @brief A grainline that points right turns the text a quarter turn clockwise.
    #[test]
    fn label_reads_across_a_sideways_grainline() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="p" data-type="piece" data-size="40">
            <g data-type="grainline"><path d="M10,10 L90,10"/></g></g></svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("valid SVG");
        add_size_labels(&mut doc);
        assert_eq!(text_of(label(&doc.root, "p").expect("label")).0, "translate(95 10) rotate(90)");
    } // label_reads_across_a_sideways_grainline

    // @brief A second import keeps the label it has; verticalize still finds the grainline.
    #[test]
    fn second_import_adds_no_label_and_grain_direction_stays() {
        let mut doc = svg_dom::Document::parse(HANDOFF).expect("valid SVG");
        add_size_labels(&mut doc);
        let again = svg_dom::Document::parse(&doc.to_string()).expect("valid SVG");
        let mut again = again;
        assert_eq!(add_size_labels(&mut again), 0);
        let piece = again.root.children.iter().filter_map(|n| n.as_element())
            .flat_map(|p| p.children.iter().filter_map(|n| n.as_element()))
            .flat_map(|s| s.children.iter().filter_map(|n| n.as_element()))
            .find(|e| e.attributes.get("id").is_some_and(|id| id == "piece-1_s34"))
            .expect("piece 1");
        assert!(svg_dom::has_grain_direction(piece));
    } // second_import_adds_no_label_and_grain_direction_stays

    // @brief After a layout turns a sideways grainline up, every size label is level,
    // centred on the grainline and 5 px above its top.
    #[test]
    fn label_is_level_above_the_grainline_after_layout() {
        let piece = |size: &str, w: u32| format!(
            r#"<g id="piece-1_s{size}" data-type="piece" data-size="{size}" data-name="Yoke" data-grainline-angle="0">
                 <g id="piece-1-cutline-1_s{size}" data-type="cutline"><path d="M0,0 L{w},0 L{w},100 L0,100 Z"/></g>
                 <g id="piece-1-grainline-1_s{size}" data-type="grainline"><path d="M50,50 L150,50"/></g>
               </g>"#);
        let svg = format!(
            r#"<svg xmlns="http://www.w3.org/2000/svg" width="1000" height="1000">
                 <g id="pattern-1" data-type="pattern" data-measurements="multisize" data-sizes="34,36" data-base-size="36">
                   <g id="piece-set-1" data-type="piece-set">{}{}</g>
                 </g></svg>"#,
            piece("34", 200), piece("36", 220));
        let mut input = svg_dom::Document::parse(&svg).expect("valid SVG");
        assert_eq!(add_size_labels(&mut input), 2);

        let settings = serde_json::json!({
            "unit": "in", "mediaType": "fabric", "paperType": "roll",
            "pageWidth": 20.0, "pageHeight": 40.0,
            "marginLeft": 0.5, "marginRight": 0.5, "marginTop": 0.5, "marginBottom": 0.5,
            "pieceGap": 0.125, "layoutMode": "alongGrainline",
            "tileSize": "Letter", "tileOrientation": "Portrait"
        }).to_string();
        let init = crate::layout_utils::do_initialize_layout(&settings, Some(&input)).expect("initialize");
        let mut progress = |_pct: i32, _status: Option<&str>| {};
        let marker = crate::layout_utils::do_process_layout(
            crate::layout_utils::ProcessLayoutArgs {
                settings_json: &settings,
                input_dom: &input,
                initial_layout_dom: &init.initial_dom,
                layout_h_px: init.h_px,
                nested_multisize: false,
                extend_media: true,
                split_oversized: false,
            },
            &mut progress,
        ).expect("Marker layout");

        let mut checked = 0;
        for piece in marker.output_doc.root.children.iter().filter_map(|n| n.as_element()).filter(|e| is_type(e, "piece")) {
            let size = &piece.attributes["data-size"];
            let grainline = piece.children.iter().filter_map(|n| n.as_element()).find(|c| is_type(c, "grainline")).expect("grainline");
            let (rear, front) = first_path_ends(grainline, Matrix2D::IDENTITY).expect("grainline path");
            assert!((rear.x - front.x).abs() < 0.01 && front.y < rear.y, "size {size}: grainline points up");

            let group = piece.children.iter().filter_map(|n| n.as_element()).find(|c| is_type(c, SIZE_LABEL_TYPE)).expect("size label");
            let (transform, text) = text_of(group);
            assert_eq!(text, format!("Size {size}"));
            let m = svg_dom::parse_svg_transform(&transform);
            assert!(m.b.abs() < 1e-3 && m.a > 0.999, "size {size}: label not level: {transform}");
            assert!((m.e - front.x).abs() < 0.5, "size {size}: label x {} vs grainline {}", m.e, front.x);
            assert!((m.f - (front.y - SIZE_LABEL_GAP_PX as f32)).abs() < 0.5, "size {size}: label y {} vs grainline top {}", m.f, front.y);
            checked += 1;
        } // for piece
        assert_eq!(checked, 2, "both sizes placed");
    } // label_is_level_above_the_grainline_after_layout
} // mod tests
