// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file annotation_alignment.rs
// @brief Gives every size of a multisize piece the base size's grainline and label positions.
//
// A piece-set holds one piece per size. At import the grainline, piece label and
// pattern label of each size move to the place the base size has them, measured
// from the stack point: the start of the fold line when every size has one, else
// the centre of the piece outline's bounding box.  The Nested tab stacks the sizes
// of a piece-set on the same point (`svg_dom::center_piece_sets`), so their
// grainlines and labels stack exactly; every other tab shows them in the same place
// in each size.
//
// Seamly2D sends no group for a hidden grainline or label, so nothing moves for it.
// A moved grainline or label that leaves the cut line of a smaller size is reported.

use geometry::{Matrix2D, Path, PathSegment, Point};
use xmltree::Element;

// @brief Annotation groups that move to the base size's place, with the name a report uses.
const ANNOTATION_TYPES: [(&str, &str); 3] =
    [("grainline", "grainline"), ("piece_label", "piece label"), ("pattern_label", "pattern label")];

// @brief Offset below which an annotation counts as in place, in px.
const TOLERANCE_PX: f32 = 0.01;

// @brief What `align_size_annotations` did.
#[derive(Debug, Default, PartialEq)]
pub(crate) struct AlignmentReport {
    // Number of grainline and label groups moved.
    pub moved: usize,
    // One line per piece and annotation that lies past the cut line in some sizes:
    // "Back: pattern label past the cut line in sizes 220, 240".
    pub outside: Vec<String>,
}

// @brief Import warning for annotations that lie past the cut line: one bullet per piece and annotation.
pub(crate) fn outside_warning(lines: &[String]) -> String {
    let bullets: Vec<String> = lines.iter().map(|line| format!("\u{2022} {line}")).collect();
    format!(
        "Every size shows its grainline and labels where the base size has them. \
         In some sizes they now lie past the cut line:\n\n{}\n\n\
         Move them toward the middle of the piece in the base size in Seamly2D.",
        bullets.join("\n")
    )
} // fn outside_warning

// @brief Move the grainline and labels of every size to the base size's place.
//
// @param doc Imported document; changed in place.
// @return The groups moved, and the annotations that leave a cut line.
pub(crate) fn align_size_annotations(doc: &mut svg_dom::Document) -> AlignmentReport {
    let mut report = AlignmentReport::default();
    for pattern in doc.root.children.iter_mut().filter_map(|n| n.as_mut_element()).filter(|e| is_type(e, "pattern")) {
        let Some(base_size) = pattern.attributes.get("data-base-size").cloned() else {
            continue; // an individual pattern has no base size
        }; // base size
        for set in pattern.children.iter_mut().filter_map(|n| n.as_mut_element()).filter(|e| is_type(e, "piece-set")) {
            align_piece_set(set, &base_size, &mut report);
        } // for each piece-set
    } // for each pattern group
    report
} // fn align_size_annotations

// @brief Align the sizes of one piece-set on its base size piece.
fn align_piece_set(set: &mut Element, base_size: &str, report: &mut AlignmentReport) {
    // Stack point of a size: its fold line start when every size has a fold line, else its outline centre.
    let on_fold = set.children.iter().filter_map(|n| n.as_element())
        .filter(|e| is_type(e, "piece"))
        .all(|piece| fold_line_start(piece).is_some());
    let stack_point = |piece: &Element| if on_fold { fold_line_start(piece) } else { outline_centre(piece) };

    // Offset of each annotation of the base size from its stack point.
    let base_offsets: Vec<(&str, &str, Point)> = {
        let Some(base) = set.children.iter().filter_map(|n| n.as_element())
            .find(|e| is_type(e, "piece") && e.attributes.get("data-size").is_some_and(|s| s == base_size))
        else {
            return; // the base size is not handed off
        }; // base piece
        let Some(centre) = stack_point(base) else {
            return; // no outline to measure from
        }; // centre
        ANNOTATION_TYPES.iter()
            .filter_map(|&(data_type, name)| {
                let reference = child_of_type(base, data_type).and_then(reference_point)?;
                Some((data_type, name, Point::new(reference.x - centre.x, reference.y - centre.y)))
            }) // filter_map
            .collect()
    }; // base_offsets
    if base_offsets.is_empty() {
        return; // the base size shows no grainline or label
    } // if nothing to align

    let piece_name = set.attributes.get("data-name").cloned()
        .or_else(|| set.attributes.get("id").cloned())
        .unwrap_or_default();
    // Sizes whose moved annotation leaves the cut line, per annotation name.
    let mut outside: Vec<(&str, Vec<String>)> = Vec::new();
    for piece in set.children.iter_mut().filter_map(|n| n.as_mut_element()).filter(|e| is_type(e, "piece")) {
        let Some(centre) = stack_point(piece) else {
            continue; // no outline to measure from
        }; // centre
        let outline = cut_outline(piece);
        let size = piece.attributes.get("data-size").cloned().unwrap_or_default();
        for (data_type, name, offset) in &base_offsets {
            let Some(group) = child_of_type_mut(piece, data_type) else {
                continue; // this size does not show it
            }; // group
            let Some(reference) = reference_point(group) else {
                continue; // nothing to measure
            }; // reference
            // Move the group so its reference point sits at the base size's offset.
            let (dx, dy) = (centre.x + offset.x - reference.x, centre.y + offset.y - reference.y);
            if dx.abs() > TOLERANCE_PX || dy.abs() > TOLERANCE_PX {
                prepend_translate(group, dx, dy);
                report.moved += 1;
            } // if not in place
            if !outline.is_empty() && !annotation_points(group).iter().all(|p| inside(&outline, *p)) {
                match outside.iter_mut().find(|(n, _)| *n == *name) {
                    Some((_, sizes)) => sizes.push(size.clone()),
                    None => outside.push((name, vec![size.clone()])),
                } // match entry
            } // if past the cut line
        } // for each annotation
    } // for each size
    for (name, sizes) in outside {
        report.outside.push(format!("{piece_name}: {name} past the cut line in sizes {}", sizes.join(", ")));
    } // for each annotation past the cut line
} // fn align_piece_set

// @brief True when `elem` is a `<g>` with this `data-type`.
fn is_type(elem: &Element, data_type: &str) -> bool {
    elem.name == "g" && elem.attributes.get("data-type").is_some_and(|t| t == data_type)
} // fn is_type

// @brief First direct child group of `piece` with this `data-type`.
pub(crate) fn child_of_type<'a>(piece: &'a Element, data_type: &str) -> Option<&'a Element> {
    piece.children.iter().filter_map(|n| n.as_element()).find(|c| is_type(c, data_type))
} // fn child_of_type

// @brief First direct child group of `piece` with this `data-type`, for change.
fn child_of_type_mut<'a>(piece: &'a mut Element, data_type: &str) -> Option<&'a mut Element> {
    piece.children.iter_mut().filter_map(|n| n.as_mut_element()).find(|c| is_type(c, data_type))
} // fn child_of_type_mut

// @brief Groups that draw the piece outline: the cut lines, else the seam lines.
fn outline_groups(piece: &Element) -> Vec<&Element> {
    let groups = |data_type: &str| -> Vec<&Element> {
        piece.children.iter().filter_map(|n| n.as_element()).filter(|c| is_type(c, data_type)).collect()
    }; // groups
    let cut = groups("cutline");
    if cut.is_empty() { groups("seamline") } else { cut }
} // fn outline_groups

// @brief Centre of the bounding box of the outline, control points included, in the piece frame.
//
// `svg_dom::center_piece_sets` measures the same points after the transforms are baked in.
fn outline_centre(piece: &Element) -> Option<Point> {
    let mut points = Vec::new();
    for group in outline_groups(piece) {
        collect_points(group, Matrix2D::IDENTITY, true, &mut points);
    } // for each outline group
    bbox_centre(&points)
} // fn outline_centre

// @brief Start of the fold line, in the piece frame: the first point of the first path of the `fold_line`
//        group, outside its tagged arrow line and label groups. `svg_dom::fold_line_d` reads the same path.
// @return None when the piece has no fold line.
fn fold_line_start(piece: &Element) -> Option<Point> {
    own_path_start(child_of_type(piece, "fold_line")?, Matrix2D::IDENTITY)
} // fn fold_line_start

// @brief First point of the first `<path>` in `elem`'s subtree, outside child groups with their own
//        `data-type`, with the transforms down to it applied.
fn own_path_start(elem: &Element, parent: Matrix2D) -> Option<Point> {
    let matrix = parent.mul(&own_transform(elem));
    if elem.name == "path" {
        let path = Path::parse_path_attribute(elem.attributes.get("d")?).ok()?;
        return match path.segments.first()? {
            PathSegment::MoveTo(p) => Some(matrix.apply_to_point(*p)), // the fold line start node
            _ => None,                                                 // no start point
        }; // match first segment
    } // if path
    elem.children.iter().filter_map(|n| n.as_element())
        .filter(|c| !c.attributes.contains_key("data-type")) // a tagged part is not the fold line
        .find_map(|c| own_path_start(c, matrix))
} // fn own_path_start

// @brief The cut line (else seam line) as a polygon of segment end points, in the piece frame.
fn cut_outline(piece: &Element) -> Vec<Point> {
    let mut points = Vec::new();
    if let Some(group) = outline_groups(piece).first() {
        collect_points(group, Matrix2D::IDENTITY, false, &mut points);
    } // if outline
    points
} // fn cut_outline

// @brief Point that marks where an annotation group sits, in the piece frame.
//
// A grainline: the middle of its line. A label: the anchor of its first text line;
// a label drawn as paths: the centre of its points.
pub(crate) fn reference_point(group: &Element) -> Option<Point> {
    if is_type(group, "grainline") {
        let (rear, front) = crate::size_labels::first_path_ends(group, Matrix2D::IDENTITY)?;
        return Some(Point::new((rear.x + front.x) / 2.0, (rear.y + front.y) / 2.0));
    } // if grainline
    first_text_anchor(group, Matrix2D::IDENTITY).or_else(|| {
        let mut points = Vec::new();
        collect_points(group, Matrix2D::IDENTITY, false, &mut points);
        bbox_centre(&points)
    }) // or the centre of its points
} // fn reference_point

// @brief Points that must lie inside the cut line: the grainline ends, or every text line anchor of a label.
fn annotation_points(group: &Element) -> Vec<Point> {
    if is_type(group, "grainline") {
        return crate::size_labels::first_path_ends(group, Matrix2D::IDENTITY)
            .map(|(rear, front)| vec![rear, front])
            .unwrap_or_default();
    } // if grainline
    let mut anchors = Vec::new();
    collect_text_anchors(group, Matrix2D::IDENTITY, &mut anchors);
    anchors
} // fn annotation_points

// @brief Anchor (x, y) of the first `<text>` in `elem`'s subtree, in the piece frame.
fn first_text_anchor(elem: &Element, parent: Matrix2D) -> Option<Point> {
    let mut anchors = Vec::new();
    collect_text_anchors(elem, parent, &mut anchors);
    anchors.first().copied()
} // fn first_text_anchor

// @brief Anchor (x, y) of every `<text>` in `elem`'s subtree, in document order, in the piece frame.
fn collect_text_anchors(elem: &Element, parent: Matrix2D, anchors: &mut Vec<Point>) {
    let matrix = parent.mul(&own_transform(elem));
    if elem.name == "text" {
        let coordinate = |name: &str| elem.attributes.get(name).and_then(|v| v.trim().parse::<f32>().ok()).unwrap_or(0.0);
        anchors.push(matrix.apply_to_point(Point::new(coordinate("x"), coordinate("y"))));
    } // if text
    for child in elem.children.iter().filter_map(|n| n.as_element()) {
        collect_text_anchors(child, matrix, anchors);
    } // for child
} // fn collect_text_anchors

// @brief Path points in `elem`'s subtree, in the piece frame.
// @param with_controls True adds curve control points, as `svg_dom` bounding boxes do.
fn collect_points(elem: &Element, parent: Matrix2D, with_controls: bool, points: &mut Vec<Point>) {
    let matrix = parent.mul(&own_transform(elem));
    if elem.name == "path" {
        if let Some(path) = elem.attributes.get("d").and_then(|d| Path::parse_path_attribute(d).ok()) {
            for segment in &path.segments {
                let segment_points: Vec<Point> = match segment {
                    PathSegment::MoveTo(p) | PathSegment::LineTo(p) => vec![*p],
                    PathSegment::QuadTo { ctrl, to } if with_controls => vec![*ctrl, *to],
                    PathSegment::CubicTo { ctrl1, ctrl2, to } if with_controls => vec![*ctrl1, *ctrl2, *to],
                    PathSegment::QuadTo { to, .. } | PathSegment::CubicTo { to, .. } | PathSegment::ArcTo { to, .. } => vec![*to],
                    PathSegment::Close => Vec::new(),
                }; // segment_points
                points.extend(segment_points.into_iter().map(|p| matrix.apply_to_point(p)));
            } // for each segment
        } // if path data
    } // if path
    for child in elem.children.iter().filter_map(|n| n.as_element()) {
        collect_points(child, matrix, with_controls, points);
    } // for child
} // fn collect_points

// @brief Centre of the bounding box of `points`; None when empty.
fn bbox_centre(points: &[Point]) -> Option<Point> {
    if points.is_empty() {
        return None;
    } // if no points
    let (min_x, max_x, min_y, max_y) = points.iter().fold(
        (f32::INFINITY, f32::NEG_INFINITY, f32::INFINITY, f32::NEG_INFINITY),
        |(x0, x1, y0, y1), p| (x0.min(p.x), x1.max(p.x), y0.min(p.y), y1.max(p.y)),
    );
    Some(Point::new((min_x + max_x) / 2.0, (min_y + max_y) / 2.0))
} // fn bbox_centre

// @brief True when `point` lies inside `polygon` (even-odd rule).
fn inside(polygon: &[Point], point: Point) -> bool {
    let mut is_inside = false;
    let mut previous = polygon[polygon.len() - 1];
    for &current in polygon {
        let crosses = (current.y > point.y) != (previous.y > point.y);
        if crosses && point.x < (previous.x - current.x) * (point.y - current.y) / (previous.y - current.y) + current.x {
            is_inside = !is_inside;
        } // if the edge crosses the ray
        previous = current;
    } // for each edge
    is_inside
} // fn inside

// @brief The element's own `transform`, or identity.
pub(crate) fn own_transform(elem: &Element) -> Matrix2D {
    elem.attributes.get("transform").map(|t| svg_dom::parse_svg_transform(t)).unwrap_or(Matrix2D::IDENTITY)
} // fn own_transform

// @brief Prepend `translate(dx dy)`, so the move applies after the group's own transform.
fn prepend_translate(elem: &mut Element, dx: f32, dy: f32) {
    let translate = format!("translate({} {})", fmt(dx), fmt(dy));
    let transform = match elem.attributes.get("transform") {
        Some(existing) if !existing.trim().is_empty() => format!("{translate} {existing}"),
        _ => translate,
    }; // transform
    elem.attributes.insert("transform".to_string(), transform);
} // fn prepend_translate

// @brief A number with at most three decimals.
fn fmt(value: f32) -> String {
    let text = format!("{value:.3}");
    let text = text.trim_end_matches('0').trim_end_matches('.');
    if text == "-0" { "0".to_string() } else { text.to_string() }
} // fn fmt

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Two sizes of Front: the base size 36 and a larger size 38 drafted 50 px right and down.
    // Size 38 has its grainline and label elsewhere; size 36 has them near the outline centre.
    const TWO_SIZES: &str = r#"<svg xmlns="http://www.w3.org/2000/svg">
  <g id="pattern-1" data-type="pattern" data-measurements="multisize" data-sizes="36,38" data-base-size="36">
    <g id="piece-set-1" data-type="piece-set" data-name="Front">
      <g id="piece-1_s36" data-type="piece" data-size="36" data-name="Front">
        <g data-type="cutline"><path d="M0,0 L100,0 L100,200 L0,200 Z"/></g>
        <g data-type="grainline"><g transform="matrix(1,0,0,1,10,20)"><path d="M40,130 L40,30"/></g></g>
        <g data-type="piece_label"><g transform="matrix(1,0,0,1,30,40)"><text x="0" y="14">Front</text></g></g>
      </g>
      <g id="piece-1_s38" data-type="piece" data-size="38" data-name="Front">
        <g data-type="cutline"><path d="M50,50 L170,50 L170,290 L50,290 Z"/></g>
        <g data-type="grainline"><path d="M60,250 L60,100"/></g>
        <g data-type="piece_label"><g transform="matrix(1,0,0,1,140,260)"><text x="0" y="14">Front</text></g></g>
      </g>
    </g>
  </g>
</svg>"#;

    // @brief The piece group of size `size`.
    fn piece<'a>(doc: &'a svg_dom::Document, size: &str) -> &'a Element {
        let set = doc.root.children.iter().filter_map(|n| n.as_element()).next().expect("pattern")
            .children.iter().filter_map(|n| n.as_element()).next().expect("piece-set");
        set.children.iter().filter_map(|n| n.as_element())
            .find(|e| e.attributes.get("data-size").is_some_and(|s| s == size))
            .expect("piece of that size")
    } // fn piece

    // @brief Offset of an annotation's reference point from the outline centre.
    fn offset(doc: &svg_dom::Document, size: &str, data_type: &str) -> Point {
        let piece = piece(doc, size);
        let centre = outline_centre(piece).expect("outline");
        let reference = child_of_type(piece, data_type).and_then(reference_point).expect("annotation");
        Point::new(reference.x - centre.x, reference.y - centre.y)
    } // fn offset

    fn approx(a: Point, b: Point) {
        assert!((a.x - b.x).abs() < 0.01 && (a.y - b.y).abs() < 0.01, "{a:?} != {b:?}");
    } // fn approx

    // @brief Every size takes the base size's grainline and label offset from its outline centre.
    #[test]
    fn sizes_take_the_base_size_offsets() {
        let mut doc = svg_dom::Document::parse(TWO_SIZES).expect("fixture parses");
        let base_grain = offset(&doc, "36", "grainline");
        let base_label = offset(&doc, "36", "piece_label");
        let report = align_size_annotations(&mut doc);
        assert_eq!(report.moved, 2, "size 38 grainline and label move; size 36 stays");
        approx(offset(&doc, "38", "grainline"), base_grain);
        approx(offset(&doc, "38", "piece_label"), base_label);
        approx(offset(&doc, "36", "grainline"), base_grain);
        assert!(report.outside.is_empty(), "{:?}", report.outside);

        // A second import changes nothing.
        assert_eq!(align_size_annotations(&mut doc).moved, 0);
    } // sizes_take_the_base_size_offsets

    // @brief A label the base size puts near its edge leaves a smaller size: the report names the sizes.
    #[test]
    fn annotation_past_a_smaller_cut_line_is_reported() {
        let svg = TWO_SIZES
            .replace(r#"data-sizes="36,38" data-base-size="36""#, r#"data-sizes="36,38" data-base-size="38""#)
            .replace(r#"<path d="M60,250 L60,100"/>"#, r#"<path d="M100,250 L100,100"/>"#)
            .replace("matrix(1,0,0,1,140,260)", "matrix(1,0,0,1,55,256)");
        let mut doc = svg_dom::Document::parse(&svg).expect("fixture parses");
        let report = align_size_annotations(&mut doc);
        assert_eq!(report.outside, vec!["Front: piece label past the cut line in sizes 36".to_string()]);
    } // annotation_past_a_smaller_cut_line_is_reported

    // @brief The warning lists one bullet per line and says what to do.
    #[test]
    fn outside_warning_lists_each_line() {
        let text = outside_warning(&["Back: pattern label past the cut line in sizes 220, 240".to_string()]);
        assert!(text.contains("\n\n\u{2022} Back: pattern label past the cut line in sizes 220, 240\n\n"), "{text}");
        assert!(text.ends_with("in the base size in Seamly2D."), "{text}");
    } // outside_warning_lists_each_line

    // @brief No base size piece, or an individual pattern: nothing moves.
    #[test]
    fn without_a_base_size_nothing_moves() {
        let unknown = TWO_SIZES.replace(r#"data-base-size="36""#, r#"data-base-size="40""#);
        let mut doc = svg_dom::Document::parse(&unknown).expect("fixture parses");
        assert_eq!(align_size_annotations(&mut doc), AlignmentReport::default());

        let individual = TWO_SIZES.replace(r#" data-base-size="36""#, "");
        let mut doc = svg_dom::Document::parse(&individual).expect("fixture parses");
        assert_eq!(align_size_annotations(&mut doc), AlignmentReport::default());
    } // without_a_base_size_nothing_moves

    // @brief A hidden annotation of the base size (no group) moves nothing in the other sizes.
    #[test]
    fn hidden_base_annotation_moves_nothing() {
        let svg = TWO_SIZES.replacen(
            r#"<g data-type="grainline"><g transform="matrix(1,0,0,1,10,20)"><path d="M40,130 L40,30"/></g></g>"#, "", 1);
        let mut doc = svg_dom::Document::parse(&svg).expect("fixture parses");
        let before = child_of_type(piece(&doc, "38"), "grainline").and_then(reference_point).expect("grainline");
        let report = align_size_annotations(&mut doc);
        assert_eq!(report.moved, 1, "only the label moves");
        approx(child_of_type(piece(&doc, "38"), "grainline").and_then(reference_point).expect("grainline"), before);
    } // hidden_base_annotation_moves_nothing
} // mod tests
