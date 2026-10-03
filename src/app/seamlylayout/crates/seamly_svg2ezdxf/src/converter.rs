// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief SVG to ezdxf conversion logic.

use crate::drawing::{Block, Drawing, DxfVersion};
use crate::entities::{Circle, Entity, Line, Point, Polyline, Text};
use crate::error::Result;
use crate::layers::map_svg_to_astm_layer;
use crate::astm_contour::{build_contour_tagged, AstmContour};
use crate::astm_notch::build_notches;
use crate::drawing::Annotation;
use crate::layers::{piece_component, PieceComponent};
use crate::utils::{invert_y_axis, parse_float_attr, parse_length_attr, sanitize_ascii, sanitize_block_name, MM_PER_PX};
use geometry::Path;
use xmltree::{Element, XMLNode};

// @brief Conversion options for SVG to ezdxf.
#[derive(Debug, Clone)]
pub struct SvgToEzdxfOptions {
    // Target DXF version (default: R12 for ASTM).
    pub dxf_version: DxfVersion,
    // Whether to create blocks for pattern pieces.
    pub create_blocks: bool,
    // Coordinate system transformation (Y-axis inversion).
    pub invert_y: bool,
    // SVG height for Y-axis inversion (required if invert_y is true).
    pub svg_height: Option<f64>,
    // Interpolation tolerance for curves (in SVG units).
    pub flatten_tolerance: f64,
    // DXF units per SVG user unit (default: millimetres per px at 96 dpi).
    pub unit_scale: f64,
}

impl Default for SvgToEzdxfOptions {
    fn default() -> Self {
        Self {
            dxf_version: DxfVersion::R12,
            create_blocks: true,
            invert_y: true,
            svg_height: None,
            flatten_tolerance: 0.1,
            unit_scale: MM_PER_PX,
        }
    }
}

// @brief Convert SVG Document to ezdxf Drawing.
// @param doc The SVG DOM document to convert; it is not modified.
// @param options Conversion options.
// @return Drawing object ready for DXF export, in DXF units (`options.unit_scale`).
pub fn svg_to_ezdxf(doc: &svg_dom::Document, options: &SvgToEzdxfOptions) -> Result<Drawing> {
    // Bake every `transform` into geometry so element coordinates are absolute.
    // Text keeps one combined transform, which convert_text applies.
    let mut flat = doc.clone();
    svg_dom::flatten_dom(&mut flat);
    let root = &flat.root;

    // Y inversion needs the document height in user units: the viewBox height
    // when present, else the `height` attribute converted from its unit.
    let svg_height = options.svg_height.unwrap_or_else(|| {
        root.attributes
            .get("viewBox")
            .and_then(|vb| vb.split(|c: char| c == ',' || c.is_whitespace()).filter(|t| !t.is_empty()).nth(3).map(str::to_string))
            .and_then(|h| h.parse::<f64>().ok())
            .unwrap_or_else(|| parse_length_attr(root.attributes.get("height"), 100.0))
    }); // svg_height

    let mut drawing = Drawing::new(options.dxf_version);
    drawing.style_name = find_pattern_name(root);

    if options.create_blocks {
        extract_pattern_pieces(root, &mut drawing, options, svg_height)?;
    } else {
        convert_elements_to_modelspace(root, &mut drawing, options, svg_height)?;
    } // if create_blocks

    Ok(drawing)
} // fn svg_to_ezdxf

// @brief Name of the first `data-type="pattern"` group, if any.
fn find_pattern_name(element: &Element) -> Option<String> {
    if element.attributes.get("data-type").map(String::as_str) == Some("pattern") {
        if let Some(name) = element.attributes.get("data-name") {
            let name = sanitize_ascii(name).trim().to_string();
            if !name.is_empty() {
                return Some(name);
            } // if non-empty
        } // if data-name
    } // if pattern group
    element.children.iter().find_map(|c| match c {
        XMLNode::Element(e) => find_pattern_name(e),
        _ => None,
    }) // find in children
} // fn find_pattern_name

// @brief Collect every `data-type="piece"` group in document order.
fn collect_tagged_pieces<'a>(element: &'a Element, out: &mut Vec<&'a Element>) {
    for child in &element.children {
        if let XMLNode::Element(e) = child {
            if e.name == "g" && e.attributes.get("data-type").map(String::as_str) == Some("piece") {
                out.push(e);
            } else {
                collect_tagged_pieces(e, out);
            } // if piece
        } // if element
    } // for each child
} // fn collect_tagged_pieces

// @brief Create one block per pattern piece.
// @details A tagged SVG's pieces are its `data-type="piece"` groups. An
//          untagged SVG's pieces are the top-level `<g>` elements with an id;
//          other top-level elements go to modelspace.
// @param root SVG root element (transforms already baked).
// @param drawing Drawing to add blocks to.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
fn extract_pattern_pieces(
    root: &Element,
    drawing: &mut Drawing,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<()> {
    let mut tagged = Vec::new();
    collect_tagged_pieces(root, &mut tagged);
    if !tagged.is_empty() {
        for piece in tagged {
            add_piece_block(piece, drawing, options, svg_height)?;
        } // for each tagged piece
        return Ok(());
    } // if tagged

    for child in &root.children {
        let XMLNode::Element(element) = child else {
            continue;
        }; // if not element
        if element.name == "g" && element.attributes.contains_key("id") {
            add_piece_block(element, drawing, options, svg_height)?;
        } else {
            convert_element_tree(element, drawing, options, svg_height, None)?;
        } // if piece group
    } // for each child
    Ok(())
} // fn extract_pattern_pieces

// @brief Convert one piece group into a block with generic entities and ASTM fields.
fn add_piece_block(
    piece: &Element,
    drawing: &mut Drawing,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<()> {
    let id = piece.attributes.get("id").cloned().unwrap_or_else(|| format!("piece-{}", drawing.blocks.len() + 1));
    // "_M" suffix matches CLO3D / seamly2clo.py block naming.
    let mut block = Block::new(format!("{}_M", sanitize_block_name(&id)));
    if let Some(name) = piece.attributes.get("data-name").or_else(|| piece.attributes.get("data-letter")) {
        let name = sanitize_ascii(name).trim().to_string();
        if !name.is_empty() {
            block.piece_name = name;
        } // if non-empty
    } // if name attribute

    // Generic conversion of the piece's children (the piece group itself
    // carries no layer, so its id never leaks into a layer choice).
    for child in &piece.children {
        if let XMLNode::Element(e) = child {
            convert_element_tree(e, &mut block, options, svg_height, None)?;
        } // if element
    } // for each child

    extract_astm_piece(piece, &mut block, options, svg_height);
    drawing.add_block(block);
    Ok(())
} // fn add_piece_block

// @brief Convert all elements directly to modelspace (no blocks).
// @param root SVG root element.
// @param drawing Drawing to add entities to.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
fn convert_elements_to_modelspace(
    root: &Element,
    drawing: &mut Drawing,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<()> {
    println!("[CONVERTER] convert_elements_to_modelspace: Starting conversion");
    println!("  └─ Root element: '{}'", root.name);
    println!("  └─ Children count: {}", root.children.len());
    println!(
        "  └─ Modelspace entities before: {}",
        drawing.modelspace_entities.len()
    );

    // Recursively convert all elements.
    convert_element_tree(root, drawing, options, svg_height, None)?;

    println!(
        "  └─ Modelspace entities after: {}",
        drawing.modelspace_entities.len()
    );
    println!("[CONVERTER] convert_elements_to_modelspace: Complete");

    Ok(())
}

// @brief Recursively convert SVG elements to DXF entities.
// @param element SVG element to convert.
// @param target Target to add entities to (Block or Drawing).
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @param parent_layer Layer inherited from the nearest component group, if any.
fn convert_element_tree(
    element: &Element,
    target: &mut dyn EntityTarget,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    parent_layer: Option<&str>,
) -> Result<()> {
    // Geometry converts to one entity; a component group's layer overrides the element's own.
    let entity: Option<Box<dyn Entity>> = match element.name.as_str() {
        "line" => convert_line(element, options, svg_height)?.map(|mut e| {
            if let Some(l) = parent_layer { e.layer = l.to_string(); }
            Box::new(e) as Box<dyn Entity>
        }),
        "circle" => convert_circle(element, options, svg_height)?.map(|mut e| {
            if let Some(l) = parent_layer { e.layer = l.to_string(); }
            Box::new(e) as Box<dyn Entity>
        }),
        // Text is always annotation text (layer 15), whatever group holds it.
        "text" => convert_text(element, options, svg_height)?.map(|mut e| {
            e.layer = "15".to_string();
            Box::new(e) as Box<dyn Entity>
        }),
        "path" => convert_path(element, options, svg_height)?.map(|mut e| {
            if let Some(l) = parent_layer { e.layer = l.to_string(); }
            Box::new(e) as Box<dyn Entity>
        }),
        "polyline" => convert_polyline(element, options, svg_height, parent_layer)?.map(|e| Box::new(e) as Box<dyn Entity>),
        "polygon" => convert_polygon(element, options, svg_height, parent_layer)?.map(|e| Box::new(e) as Box<dyn Entity>),
        "rect" => convert_rect(element, options, svg_height, parent_layer)?.map(|e| Box::new(e) as Box<dyn Entity>),
        "ellipse" => convert_ellipse(element, options, svg_height, parent_layer)?,
        _ => None,
    }; // entity
    if let Some(e) = entity {
        target.add_entity(e);
        return Ok(());
    } // if converted

    // Containers: a component group sets the layer for its subtree.
    let child_layer = if element.name == "g" {
        piece_component(element).map(PieceComponent::layer).or(parent_layer)
    } else {
        parent_layer
    }; // child_layer
    for child in &element.children {
        if let XMLNode::Element(child_element) = child {
            convert_element_tree(child_element, target, options, svg_height, child_layer)?;
        } // if element
    } // for each child
    Ok(())
} // fn convert_element_tree

// @brief Trait for targets that can receive entities (Block or Drawing).
trait EntityTarget {
    // Add an entity to this target.
    fn add_entity(&mut self, entity: Box<dyn Entity>);
}

impl EntityTarget for Block {
    fn add_entity(&mut self, entity: Box<dyn Entity>) {
        Block::add_entity(self, entity);
    }
}

impl EntityTarget for Drawing {
    fn add_entity(&mut self, entity: Box<dyn Entity>) {
        Drawing::add_modelspace_entity(self, entity);
    }
}

// @brief Map an SVG user-space point to DXF: optional Y inversion, then unit scaling.
fn to_dxf(p: Point, options: &SvgToEzdxfOptions, svg_height: f64) -> Point {
    let p = if options.invert_y { invert_y_axis(p, svg_height) } else { p };
    Point::new(p.x * options.unit_scale, p.y * options.unit_scale)
} // fn to_dxf

// @brief Collect the component groups of a piece in document order; components do not nest.
fn collect_components<'a>(element: &'a Element, out: &mut Vec<(PieceComponent, &'a Element)>) {
    for child in &element.children {
        if let XMLNode::Element(e) = child {
            match piece_component(e) {
                Some(component) => out.push((component, e)),
                None => collect_components(e, out),
            } // match component
        } // if element
    } // for each child
} // fn collect_components

// @brief One polyline of a piece component.
struct TaggedPolyline {
    points: Vec<Point>,
    closed: bool,
    // Parallel to `points`: `true` = turn point. `None` = no `data-turn-points` tags.
    turns: Option<Vec<bool>>,
} // struct TaggedPolyline

// @brief Every polyline drawn under `element`, one per path subpath, in DXF units.
// @return (vertices, closed) pairs.
fn element_polylines(element: &Element, options: &SvgToEzdxfOptions, svg_height: f64) -> Vec<(Vec<Point>, bool)> {
    element_tagged_polylines(element, options, svg_height).into_iter().map(|l| (l.points, l.closed)).collect()
} // fn element_polylines

// @brief Every polyline drawn under a component group, with the group's `data-turn-points` tags.
// @details `data-turn-points` lists, space separated, the indices of the turn
//          point vertices in the component path's `d`, counting each MoveTo and
//          LineTo end point from 0. An empty value means no turn points. Tags apply
//          only to a path of straight segments, where vertices map 1:1.
fn element_tagged_polylines(element: &Element, options: &SvgToEzdxfOptions, svg_height: f64) -> Vec<TaggedPolyline> {
    // An unparseable value is treated as absent: the geometry fallback is safer than wrong tags.
    let turn_points: Option<Vec<usize>> = element
        .attributes
        .get("data-turn-points")
        .and_then(|v| v.split_whitespace().map(|t| t.parse::<usize>().ok()).collect());
    let mut out = Vec::new();
    collect_polylines(element, options, svg_height, turn_points.as_deref(), &mut out);
    out
} // fn element_tagged_polylines

// @brief Recursive worker for element_tagged_polylines.
fn collect_polylines(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    turn_points: Option<&[usize]>,
    out: &mut Vec<TaggedPolyline>,
) {
    let map = |x: f64, y: f64| to_dxf(Point::new(x, y), options, svg_height);
    match element.name.as_str() {
        "path" => {
            let Some(d) = element.attributes.get("d") else { return; };
            let Ok(path) = Path::parse_path_attribute(d) else { return; };
            // Tags index the path's vertices, so they need one vertex per segment.
            let straight = path.segments.iter().all(|s| {
                matches!(
                    s,
                    geometry::PathSegment::MoveTo(_) | geometry::PathSegment::LineTo(_) | geometry::PathSegment::Close
                )
            }); // straight
            let turn_points = turn_points.filter(|_| straight);
            let mut vertex_index = 0usize;
            // Split at each MoveTo: Seamly2D draws all notches of a piece as one path.
            let mut subpaths: Vec<Vec<geometry::PathSegment>> = Vec::new();
            for seg in &path.segments {
                if matches!(seg, geometry::PathSegment::MoveTo(_)) || subpaths.is_empty() {
                    subpaths.push(Vec::new());
                } // if new subpath
                subpaths.last_mut().expect("subpath exists").push(seg.clone());
            } // for each segment
            for segments in subpaths {
                let closed = segments.iter().any(|s| matches!(s, geometry::PathSegment::Close));
                if let Some(indices) = turn_points {
                    // Straight segments: the vertices are the segment end points.
                    let mut points = Vec::new();
                    let mut turns = Vec::new();
                    for seg in &segments {
                        if let geometry::PathSegment::MoveTo(p) | geometry::PathSegment::LineTo(p) = seg {
                            points.push(map(p.x as f64, p.y as f64));
                            turns.push(indices.contains(&vertex_index));
                            vertex_index += 1;
                        } // if vertex
                    } // for each segment
                    if points.len() >= 2 {
                        out.push(TaggedPolyline { points, closed, turns: Some(turns) });
                    } // if drawable
                    continue;
                } // if tagged
                let mut sub = Path::new();
                sub.segments = segments;
                let points: Vec<Point> = sub
                    .flatten(options.flatten_tolerance as f32)
                    .iter()
                    .map(|p| map(p.x as f64, p.y as f64))
                    .collect();
                if points.len() >= 2 {
                    out.push(TaggedPolyline { points, closed, turns: None });
                } // if drawable
            } // for each subpath
        } // path
        "line" => {
            let a = |k: &str| parse_float_attr(element.attributes.get(k), 0.0);
            let points = vec![map(a("x1"), a("y1")), map(a("x2"), a("y2"))];
            out.push(TaggedPolyline { points, closed: false, turns: None });
        } // line
        "polyline" | "polygon" => {
            if let Some(pts) = element.attributes.get("points") {
                let points: Vec<Point> = parse_points_attribute(pts).into_iter().map(|p| map(p.x, p.y)).collect();
                if points.len() >= 2 {
                    out.push(TaggedPolyline { points, closed: element.name == "polygon", turns: None });
                } // if drawable
            } // if points
        } // polyline | polygon
        _ => {
            for child in &element.children {
                if let XMLNode::Element(e) = child {
                    collect_polylines(e, options, svg_height, turn_points, out);
                } // if element
            } // for each child
        } // container
    } // match element
} // fn collect_polylines

// @brief Annotation lines of a label group: `<text>` elements and single-stroke
//        `label_text` groups (read from `data-text`).
fn collect_annotations(element: &Element, options: &SvgToEzdxfOptions, svg_height: f64, out: &mut Vec<Annotation>) {
    for child in &element.children {
        let XMLNode::Element(e) = child else { continue; };
        if e.name == "text" {
            if let Ok(Some(t)) = convert_text(e, options, svg_height) {
                out.push(Annotation { position: t.insertion_point, height: t.height, rotation: t.rotation, text: t.content });
            } // if text converted
        } else if let Some(text) = e.attributes.get("data-text") {
            // Stroked text: place the line at the bottom-left of its strokes.
            let points: Vec<Point> = element_polylines(e, options, svg_height).into_iter().flat_map(|(p, _)| p).collect();
            if points.is_empty() {
                continue;
            } // if no strokes
            let min_x = points.iter().map(|p| p.x).fold(f64::INFINITY, f64::min);
            let min_y = points.iter().map(|p| p.y).fold(f64::INFINITY, f64::min);
            let max_y = points.iter().map(|p| p.y).fold(f64::NEG_INFINITY, f64::max);
            out.push(Annotation {
                position: Point::new(min_x, min_y),
                height: (max_y - min_y).max(0.1),
                rotation: 0.0,
                text: sanitize_ascii(text).trim().to_string(),
            });
        } else {
            collect_annotations(e, options, svg_height, out);
        } // if text kind
    } // for each child
} // fn collect_annotations

// @brief `Quantity:` value from a "Cut N" label line.
// @details D6673 quantity is "R,L". Seamly2D states only a total, so an even
//          total is split into mirrored pairs and an odd total counts as right pieces.
fn quantity_from_label(annotations: &[Annotation]) -> Option<String> {
    annotations.iter().find_map(|a| {
        let text = a.text.trim().to_ascii_lowercase();
        let rest = text.strip_prefix("cut")?.trim_start();
        let digits: String = rest.chars().take_while(|c| c.is_ascii_digit()).collect();
        let total: u32 = digits.parse().ok().filter(|&n| n > 0)?;
        Some(if total % 2 == 0 { format!("{},{}", total / 2, total / 2) } else { format!("{},0", total) })
    }) // find_map
} // fn quantity_from_label

// @brief True when two contours trace the same line.
fn same_contour(a: &AstmContour, b: &AstmContour) -> bool {
    a.dense.len() == b.dense.len()
        && b.dense.iter().all(|&p| a.dense.iter().any(|&q| crate::astm_contour::distance(p, q) < 0.01))
} // fn same_contour

// @brief Fill a block's ASTM fields from the piece's component groups.
// @details The boundary is the longest closed cut line; a piece without one
//          (seam allowance hidden or built in) uses its longest closed seam line.
fn extract_astm_piece(piece: &Element, block: &mut Block, options: &SvgToEzdxfOptions, svg_height: f64) {
    let mut components = Vec::new();
    collect_components(piece, &mut components);

    let mut cut_lines: Vec<TaggedPolyline> = Vec::new();
    let mut seam_lines: Vec<TaggedPolyline> = Vec::new();
    let mut notch_lines: Vec<Vec<Point>> = Vec::new();
    let contour = |l: &TaggedPolyline| build_contour_tagged(&l.points, l.closed, l.turns.as_deref());
    for (component, element) in components {
        let lines = || element_polylines(element, options, svg_height);
        let tagged = || element_tagged_polylines(element, options, svg_height);
        match component {
            PieceComponent::Cutline => cut_lines.extend(tagged()),
            PieceComponent::Seamline => seam_lines.extend(tagged()),
            PieceComponent::Notch => notch_lines.extend(lines().into_iter().map(|(p, _)| p)),
            PieceComponent::InternalPath => block.internal_lines.extend(tagged().iter().filter_map(contour)),
            PieceComponent::CutPath => block.cutouts.extend(tagged().iter().filter_map(contour)),
            PieceComponent::Grainline => {
                if block.grainline.is_none() {
                    // The grainline path runs tip to tip, with arrowheads drawn in between.
                    block.grainline = lines().into_iter().next().map(|(p, _)| (p[0], p[p.len() - 1]));
                } // if first grainline
            } // Grainline
            PieceComponent::Label => collect_annotations(element, options, svg_height, &mut block.annotations),
            PieceComponent::DrillHole => {} // Seamly2D exports no drill holes
        } // match component
    } // for each component

    // Seamly2D prints "<empty>" for unset label fields; it carries no information.
    block.annotations.retain(|a| !a.text.is_empty() && a.text != "<empty>");
    block.quantity = quantity_from_label(&block.annotations);

    // Boundary: the longest closed cut line, else the longest closed seam line.
    let longest_closed = |lines: &[TaggedPolyline]| -> Option<AstmContour> {
        let line = lines.iter().filter(|l| l.closed && l.points.len() >= 3).max_by_key(|l| l.points.len())?;
        build_contour_tagged(&line.points, true, line.turns.as_deref())
    }; // longest_closed
    block.boundary = longest_closed(&cut_lines).or_else(|| longest_closed(&seam_lines));

    // Sew lines: every seam line that is not the boundary itself.
    for line in &seam_lines {
        if let Some(contour) = contour(line) {
            if block.boundary.as_ref().map_or(true, |b| !same_contour(b, &contour)) {
                block.sew_lines.push(contour);
            } // if distinct from boundary
        } // if contour
    } // for each seam line

    // Notches: each drawn subpath contributes its segments.
    let segments: Vec<(Point, Point)> =
        notch_lines.iter().flat_map(|p| p.windows(2).map(|w| (w[0], w[1])).collect::<Vec<_>>()).collect();
    let boundary_dense = block.boundary.as_ref().map(|b| b.dense.clone()).unwrap_or_default();
    block.notches = build_notches(&segments, &boundary_dense);
} // fn extract_astm_piece

// @brief Convert SVG <line> element to DXF LINE entity.
// @param element SVG line element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @return LINE entity or None if conversion fails.
fn convert_line(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<Option<Line>> {
    // Parse line attributes.
    let x1 = parse_float_attr(element.attributes.get("x1"), 0.0);
    let y1 = parse_float_attr(element.attributes.get("y1"), 0.0);
    let x2 = parse_float_attr(element.attributes.get("x2"), 0.0);
    let y2 = parse_float_attr(element.attributes.get("y2"), 0.0);

    // Transform coordinates if needed.
    let start = Point::new(x1, y1);
    let end = Point::new(x2, y2);
    let (start, end) = (to_dxf(start, options, svg_height), to_dxf(end, options, svg_height));

    // Get layer name.
    let layer = map_svg_to_astm_layer(element).to_string();

    Ok(Some(Line { layer, start, end }))
}

// @brief Convert SVG <circle> element to DXF CIRCLE entity.
// @param element SVG circle element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @return CIRCLE entity or None if conversion fails.
fn convert_circle(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<Option<Circle>> {
    // Parse circle attributes.
    let cx = parse_float_attr(element.attributes.get("cx"), 0.0);
    let cy = parse_float_attr(element.attributes.get("cy"), 0.0);
    let r = parse_float_attr(element.attributes.get("r"), 0.0);

    if r <= 0.0 {
        return Ok(None); // Invalid circle.
    }

    // Transform coordinates if needed.
    let center = Point::new(cx, cy);
    let center = to_dxf(center, options, svg_height);

    // Get layer name.
    let layer = map_svg_to_astm_layer(element).to_string();

    Ok(Some(Circle {
        layer,
        center,
        radius: r * options.unit_scale,
    }))
}

// @brief Convert SVG <text> element to DXF TEXT entity.
// @details The element's `transform` (left by flatten_dom) positions, rotates
//          and scales the text.
// @param element SVG text element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @return TEXT entity, or None when the text is empty.
fn convert_text(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<Option<Text>> {
    let x = parse_float_attr(element.attributes.get("x"), 0.0);
    let y = parse_float_attr(element.attributes.get("y"), 0.0);
    let font_size = parse_length_attr(element.attributes.get("font-size"), 12.0);

    // Text content, including <tspan> children.
    let mut content = String::new();
    gather_text(element, &mut content);
    let content = sanitize_ascii(&content).trim().to_string();
    if content.is_empty() {
        return Ok(None);
    } // if empty

    // Apply the text's own transform in SVG space.
    let m = element
        .attributes
        .get("transform")
        .map(|t| svg_dom::parse_svg_transform(t))
        .unwrap_or(geometry::Matrix2D::IDENTITY);
    let placed = m.apply_to_point(geometry::Point::new(x as f32, y as f32));
    let matrix_scale = ((m.a * m.d - m.b * m.c) as f64).abs().sqrt();
    // SVG angles run clockwise on screen (Y down); DXF angles run counter-clockwise.
    let svg_angle = (m.b as f64).atan2(m.a as f64).to_degrees();
    let rotation = if options.invert_y { -svg_angle } else { svg_angle }.rem_euclid(360.0);

    Ok(Some(Text {
        layer: "15".to_string(),
        insertion_point: to_dxf(Point::new(placed.x as f64, placed.y as f64), options, svg_height),
        height: font_size * matrix_scale * options.unit_scale,
        rotation,
        content,
    }))
} // fn convert_text

// @brief Append the text content of `element` and its descendants to `out`.
fn gather_text(element: &Element, out: &mut String) {
    for child in &element.children {
        match child {
            XMLNode::Text(t) => out.push_str(t),
            XMLNode::Element(c) => gather_text(c, out),
            _ => {}
        } // match child
    } // for each child
} // fn gather_text

// @brief Convert SVG <path> element to DXF POLYLINE entity.
// @param element SVG path element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @return POLYLINE entity or None if conversion fails.
fn convert_path(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<Option<Polyline>> {
    eprintln!("      [convert_path] Starting PATH conversion");

    eprintln!("      [convert_path] Step 1: Extracting path data");
    // Get path data from 'd' attribute.
    let path_data = element.attributes.get("d").ok_or_else(|| {
        eprintln!("      [convert_path] ❌ FAILED: No 'd' attribute found");
        crate::error::SvgToEzdxfError::Svg("Path element missing 'd' attribute".to_string())
    })?;
    eprintln!("        • Path data: '{}'", path_data);

    if path_data.trim().is_empty() {
        eprintln!("      [convert_path] ❌ FAILED: Empty path data");
        return Ok(None);
    }

    eprintln!("      [convert_path] Step 2: Parsing SVG path");
    // Parse SVG path using geometry crate.
    let svg_path = Path::parse_path_attribute(path_data).map_err(|e| {
        eprintln!(
            "      [convert_path] ❌ FAILED: Path parsing error: {:?}",
            e
        );
        crate::error::SvgToEzdxfError::Geometry(format!("Failed to parse path data: {:?}", e))
    })?;
    eprintln!(
        "        • Path parsed successfully, {} segments",
        svg_path.segments.len()
    );

    eprintln!("      [convert_path] Step 3: Flattening path to polyline");
    // Flatten path to points using tolerance.
    let tolerance = options.flatten_tolerance as f32;
    eprintln!("        • Flatten tolerance: {}", tolerance);
    let flattened_points = svg_path.flatten(tolerance);
    eprintln!("        • Flattened to {} points", flattened_points.len());

    if flattened_points.is_empty() {
        eprintln!("      [convert_path] ❌ FAILED: Path flattened to empty point list");
        return Ok(None);
    }

    // Check if path is closed (last point equals first point, or path ends with Close command).
    let is_closed = svg_path
        .segments
        .iter()
        .any(|seg| matches!(seg, geometry::PathSegment::Close))
        || (flattened_points.len() > 1
            && flattened_points.first().map(|p| (p.x, p.y))
                == flattened_points.last().map(|p| (p.x, p.y)));
    eprintln!("        • Path closed: {}", is_closed);

    eprintln!(
        "      [convert_path] Step 4: Converting points and applying coordinate transformation"
    );
    // Convert geometry::Point (f32) to entities::Point (f64) and apply Y-axis inversion.
    let mut vertices: Vec<Point> = flattened_points
        .iter()
        .map(|p| {
            // Convert f32 to f64.
            to_dxf(Point::new(p.x as f64, p.y as f64), options, svg_height)
        })
        .collect();
    eprintln!("        • Converted {} vertices", vertices.len());

    // Remove duplicate consecutive points (can occur from flattening).
    vertices.dedup();
    if vertices.len() < 2 {
        eprintln!(
            "      [convert_path] ❌ FAILED: Path has less than 2 unique vertices after deduplication"
        );
        return Ok(None);
    }
    eprintln!("        • After deduplication: {} vertices", vertices.len());

    eprintln!("      [convert_path] Step 5: Determining layer");
    // Get layer name.
    let layer = map_svg_to_astm_layer(element).to_string();
    eprintln!("        • Layer: '{}'", layer);

    eprintln!("      [convert_path] Step 6: Creating POLYLINE entity");
    let polyline_entity = Polyline {
        layer: layer.clone(),
        vertices,
        closed: is_closed,
    };
    eprintln!("        • POLYLINE entity created:");
    eprintln!("          - Layer: '{}'", polyline_entity.layer);
    eprintln!("          - Vertices: {}", polyline_entity.vertices.len());
    eprintln!("          - Closed: {}", polyline_entity.closed);
    eprintln!("      [convert_path] ✅ PATH conversion successful");

    Ok(Some(polyline_entity))
}

// @brief Parse SVG points attribute into a vector of Points.
// @param points_str Points string (e.g., "10,20 30,40 50,60" or "10 20 30 40 50 60").
// @return Vector of Points.
fn parse_points_attribute(points_str: &str) -> Vec<Point> {
    let mut points = Vec::new();
    let coords: Vec<&str> = points_str.trim().split_whitespace().collect();

    // Handle both formats: "x,y x,y" and "x y x y"
    let mut i = 0;
    while i < coords.len() {
        let coord_str = coords[i];
        if let Some(comma_pos) = coord_str.find(',') {
            // Format: "x,y"
            let x_str = &coord_str[..comma_pos];
            let y_str = &coord_str[comma_pos + 1..];
            if let (Ok(x), Ok(y)) = (x_str.parse::<f64>(), y_str.parse::<f64>()) {
                points.push(Point::new(x, y));
            }
            i += 1;
        } else {
            // Format: "x y" (two separate values)
            if i + 1 < coords.len() {
                if let (Ok(x), Ok(y)) = (coords[i].parse::<f64>(), coords[i + 1].parse::<f64>()) {
                    points.push(Point::new(x, y));
                }
                i += 2;
            } else {
                i += 1;
            }
        }
    }

    points
}

// @brief Convert SVG <polyline> element to DXF POLYLINE entity.
// @param element SVG polyline element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @param parent_layer Optional parent layer to override element layer.
// @return POLYLINE entity or None if conversion fails.
fn convert_polyline(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    parent_layer: Option<&str>,
) -> Result<Option<Polyline>> {
    eprintln!("      [convert_polyline] Starting POLYLINE conversion");

    // Get points attribute.
    let points_str = element.attributes.get("points").ok_or_else(|| {
        eprintln!("      [convert_polyline] ❌ FAILED: No 'points' attribute found");
        crate::error::SvgToEzdxfError::Svg(
            "Polyline element missing 'points' attribute".to_string(),
        )
    })?;

    if points_str.trim().is_empty() {
        eprintln!("      [convert_polyline] ❌ FAILED: Empty points attribute");
        return Ok(None);
    }

    eprintln!("      [convert_polyline] Step 1: Parsing points attribute");
    let mut vertices = parse_points_attribute(points_str);
    eprintln!("        • Parsed {} points", vertices.len());

    if vertices.len() < 2 {
        eprintln!("      [convert_polyline] ❌ FAILED: Polyline needs at least 2 points");
        return Ok(None);
    }

    eprintln!("      [convert_polyline] Step 2: Applying coordinate transformation");
    // Apply Y-axis inversion if needed.
    vertices = vertices.iter().map(|p| to_dxf(*p, options, svg_height)).collect();

    eprintln!("      [convert_polyline] Step 3: Determining layer");
    let layer = parent_layer
        .map(|s| s.to_string())
        .unwrap_or_else(|| map_svg_to_astm_layer(element).to_string());
    eprintln!("        • Layer: '{}'", layer);

    eprintln!("      [convert_polyline] Step 4: Creating POLYLINE entity");
    // Polyline is never closed (use <polygon> for closed shapes).
    let polyline_entity = Polyline {
        layer,
        vertices,
        closed: false,
    };
    eprintln!(
        "        • POLYLINE entity created: {} vertices, closed: false",
        polyline_entity.vertices.len()
    );
    eprintln!("      [convert_polyline] ✅ POLYLINE conversion successful");

    Ok(Some(polyline_entity))
}

// @brief Convert SVG <polygon> element to DXF POLYLINE entity (closed).
// @param element SVG polygon element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @param parent_layer Optional parent layer to override element layer.
// @return POLYLINE entity or None if conversion fails.
fn convert_polygon(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    parent_layer: Option<&str>,
) -> Result<Option<Polyline>> {
    eprintln!("      [convert_polygon] Starting POLYGON conversion");

    // Get points attribute.
    let points_str = element.attributes.get("points").ok_or_else(|| {
        eprintln!("      [convert_polygon] ❌ FAILED: No 'points' attribute found");
        crate::error::SvgToEzdxfError::Svg("Polygon element missing 'points' attribute".to_string())
    })?;

    if points_str.trim().is_empty() {
        eprintln!("      [convert_polygon] ❌ FAILED: Empty points attribute");
        return Ok(None);
    }

    eprintln!("      [convert_polygon] Step 1: Parsing points attribute");
    let mut vertices = parse_points_attribute(points_str);
    eprintln!("        • Parsed {} points", vertices.len());

    if vertices.len() < 3 {
        eprintln!("      [convert_polygon] ❌ FAILED: Polygon needs at least 3 points");
        return Ok(None);
    }

    eprintln!("      [convert_polygon] Step 2: Applying coordinate transformation");
    // Apply Y-axis inversion if needed.
    vertices = vertices.iter().map(|p| to_dxf(*p, options, svg_height)).collect();

    // Ensure polygon is closed (first point equals last point).
    if vertices.first() != vertices.last() {
        vertices.push(vertices[0]);
    }

    eprintln!("      [convert_polygon] Step 3: Determining layer");
    let layer = parent_layer
        .map(|s| s.to_string())
        .unwrap_or_else(|| map_svg_to_astm_layer(element).to_string());
    eprintln!("        • Layer: '{}'", layer);

    eprintln!("      [convert_polygon] Step 4: Creating POLYLINE entity (closed)");
    let polyline_entity = Polyline {
        layer,
        vertices,
        closed: true,
    };
    eprintln!(
        "        • POLYLINE entity created: {} vertices, closed: true",
        polyline_entity.vertices.len()
    );
    eprintln!("      [convert_polygon] ✅ POLYGON conversion successful");

    Ok(Some(polyline_entity))
}

// @brief Convert SVG <rect> element to DXF POLYLINE entity (4 vertices, closed).
// @param element SVG rect element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @param parent_layer Optional parent layer to override element layer.
// @return POLYLINE entity or None if conversion fails.
fn convert_rect(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    parent_layer: Option<&str>,
) -> Result<Option<Polyline>> {
    eprintln!("      [convert_rect] Starting RECT conversion");

    // Parse rect attributes.
    let x = parse_float_attr(element.attributes.get("x"), 0.0);
    let y = parse_float_attr(element.attributes.get("y"), 0.0);
    let width = parse_float_attr(element.attributes.get("width"), 0.0);
    let height = parse_float_attr(element.attributes.get("height"), 0.0);

    eprintln!(
        "        • Rect attributes: x={}, y={}, width={}, height={}",
        x, y, width, height
    );

    if width <= 0.0 || height <= 0.0 {
        eprintln!("      [convert_rect] ❌ FAILED: Invalid width or height");
        return Ok(None);
    }

    eprintln!("      [convert_rect] Step 1: Creating 4 vertices");
    // Create 4 vertices: top-left, top-right, bottom-right, bottom-left.
    let mut vertices = vec![
        Point::new(x, y),                  // top-left
        Point::new(x + width, y),          // top-right
        Point::new(x + width, y + height), // bottom-right
        Point::new(x, y + height),         // bottom-left
    ];

    eprintln!("      [convert_rect] Step 2: Applying coordinate transformation");
    // Apply Y-axis inversion if needed.
    vertices = vertices.iter().map(|p| to_dxf(*p, options, svg_height)).collect();

    // Close the rectangle (add first point at end).
    vertices.push(vertices[0]);

    eprintln!("      [convert_rect] Step 3: Determining layer");
    let layer = parent_layer
        .map(|s| s.to_string())
        .unwrap_or_else(|| map_svg_to_astm_layer(element).to_string());
    eprintln!("        • Layer: '{}'", layer);

    eprintln!("      [convert_rect] Step 4: Creating POLYLINE entity (closed)");
    let polyline_entity = Polyline {
        layer,
        vertices,
        closed: true,
    };
    eprintln!(
        "        • POLYLINE entity created: {} vertices, closed: true",
        polyline_entity.vertices.len()
    );
    eprintln!("      [convert_rect] ✅ RECT conversion successful");

    Ok(Some(polyline_entity))
}

// @brief Convert SVG <ellipse> element to DXF CIRCLE or POLYLINE entity.
// @param element SVG ellipse element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @param parent_layer Optional parent layer to override element layer.
// @return CIRCLE or POLYLINE entity or None if conversion fails.
// @details If rx == ry, converts to CIRCLE. Otherwise, approximates as POLYLINE.
fn convert_ellipse(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    parent_layer: Option<&str>,
) -> Result<Option<Box<dyn Entity>>> {
    eprintln!("      [convert_ellipse] Starting ELLIPSE conversion");

    // Parse ellipse attributes.
    let cx = parse_float_attr(element.attributes.get("cx"), 0.0);
    let cy = parse_float_attr(element.attributes.get("cy"), 0.0);
    let rx = parse_float_attr(element.attributes.get("rx"), 0.0);
    let ry = parse_float_attr(element.attributes.get("ry"), 0.0);

    eprintln!(
        "        • Ellipse attributes: cx={}, cy={}, rx={}, ry={}",
        cx, cy, rx, ry
    );

    if rx <= 0.0 || ry <= 0.0 {
        eprintln!("      [convert_ellipse] ❌ FAILED: Invalid rx or ry");
        return Ok(None);
    }

    // Get layer name (use parent layer if provided).
    let layer = parent_layer
        .map(|s| s.to_string())
        .unwrap_or_else(|| map_svg_to_astm_layer(element).to_string());

    // If rx == ry, convert to CIRCLE.
    if (rx - ry).abs() < 1e-6 {
        eprintln!("      [convert_ellipse] Step 1: Converting to CIRCLE (rx == ry)");
        let center = to_dxf(Point::new(cx, cy), options, svg_height);

        let circle_entity = Circle {
            layer,
            center,
            radius: rx * options.unit_scale,
        };
        eprintln!(
            "        • CIRCLE entity created: center=({}, {}), radius={}",
            center.x, center.y, rx
        );
        eprintln!("      [convert_ellipse] ✅ ELLIPSE conversion successful (as CIRCLE)");

        Ok(Some(Box::new(circle_entity)))
    } else {
        // Otherwise, approximate as POLYLINE with multiple points.
        eprintln!("      [convert_ellipse] Step 1: Converting to POLYLINE (rx != ry)");

        // Generate points around ellipse (approximate with 32 points for smooth curve).
        const NUM_POINTS: usize = 32;
        let mut vertices = Vec::with_capacity(NUM_POINTS + 1);

        for i in 0..NUM_POINTS {
            let angle = 2.0 * std::f64::consts::PI * (i as f64) / (NUM_POINTS as f64);
            let x = cx + rx * angle.cos();
            let y = cy + ry * angle.sin();
            vertices.push(Point::new(x, y));
        }

        eprintln!(
            "        • Generated {} points for ellipse approximation",
            vertices.len()
        );

        eprintln!("      [convert_ellipse] Step 2: Applying coordinate transformation");
        // Apply Y-axis inversion if needed.
        vertices = vertices.iter().map(|p| to_dxf(*p, options, svg_height)).collect();

        // Close the ellipse.
        vertices.push(vertices[0]);

        eprintln!("      [convert_ellipse] Step 3: Creating POLYLINE entity (closed)");
        let polyline_entity = Polyline {
            layer,
            vertices,
            closed: true,
        };
        eprintln!(
            "        • POLYLINE entity created: {} vertices, closed: true",
            polyline_entity.vertices.len()
        );
        eprintln!("      [convert_ellipse] ✅ ELLIPSE conversion successful (as POLYLINE)");

        Ok(Some(Box::new(polyline_entity)))
    }
}
