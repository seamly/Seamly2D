// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file transforms.rs
// @brief Bakes SVG `transform` attributes into element geometry.
//
// "Flatten" in this codebase means baking `transform` attribute values into
// coordinate data so that no `transform` attributes remain in the output DOM.
// This is distinct from "interpolation" (converting curves to polylines).
//
// Public entry point: [`flatten_dom`].

use geometry::{Matrix2D, Path, PathSegment, Point};
use xmltree::{Element, XMLNode};

use crate::Document;

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/// @brief Bake all `transform` attributes into element geometry for the document.
/// @details Recursively walks the element tree, accumulates composed affine
///          matrices from ancestor `transform` attributes, applies the matrix
///          to each shape element's coordinate attributes, and removes every
///          `transform` attribute.  After this call no `transform` attribute
///          remains anywhere in the DOM.
/// @param doc  SVG document to flatten in-place.
pub fn flatten_dom(doc: &mut Document) {
    // Start accumulation at the root with the identity transform.
    flatten_element(&mut doc.root, Matrix2D::IDENTITY);
} // flatten_dom


// ---------------------------------------------------------------------------
// Internal recursive worker
// ---------------------------------------------------------------------------

/// @brief Recursively flatten one element and its entire subtree.
/// @param element     Element to process (modified in-place).
/// @param accumulated Composed affine matrix inherited from all ancestor elements.
fn flatten_element(element: &mut Element, accumulated: Matrix2D) {
    // Skip <defs> and <symbol> subtrees — their children are not directly rendered
    // and altering their coordinates would break use/symbol references.
    if element.name == "defs" || element.name == "symbol" {
        return;
    } // if defs or symbol

    // Skip layout rectangle groups that define canvas/tiling geometry.
    // These elements (backgroundRects, contentRects, tiledRects) must survive
    // flattening intact — their transforms and child coordinates are consumed by
    // the tiling and DXF-export pipelines and must not be baked or removed.
    let elem_id = element.attributes.get("id").map(|s| s.as_str());
    if matches!(
        elem_id,
        Some("backgroundRects") | Some("contentRects") | Some("tiledRects")
    ) {
        return;
    } // if reserved layout rect group

    // Parse this element's own transform attribute (IDENTITY if absent).
    let own_matrix = element
        .attributes
        .get("transform")
        .map(|t| parse_svg_transform(t))
        .unwrap_or(Matrix2D::IDENTITY);

    // Combined transform = parent_accumulated * own_transform.
    let combined = accumulated.mul(&own_matrix);

    // Special handling for <text> and <tspan>: absorb the accumulated parent transform
    // into this element's own transform attribute so the text stays visually correct,
    // but do NOT bake transforms into coordinate attributes (there is no safe way to bake
    // rotation/skew into text x/y/dx/dy while preserving rendering).
    if element.name == "text" || element.name == "tspan" {
        // Replace the element's transform attribute with the combined matrix.
        element.attributes.remove("transform");
        if !matrix_is_identity(&combined) {
            element
                .attributes
                .insert("transform".to_string(), matrix_to_svg_string(&combined));
        } // if combined is non-identity

        // Recurse children with IDENTITY — accumulated transform is now in this element's
        // transform attribute; children must not re-accumulate it.
        let n = element.children.len();
        for i in 0..n {
            let XMLNode::Element(_) = &element.children[i] else {
                continue;
            }; // XMLNode::Element guard
            let mut child_node = element.children.remove(i);
            if let XMLNode::Element(ref mut child_elem) = child_node {
                flatten_element(child_elem, Matrix2D::IDENTITY);
            } // if XMLNode::Element
            element.children.insert(i, child_node);
        } // for i
        return; // text/tspan handled — no coordinate baking
    } // if text or tspan

    // Remove the transform attribute — it will be baked into geometry below.
    element.attributes.remove("transform");

    // Apply the combined transform to this element's geometry where applicable.
    match element.name.as_str() {
        // Path elements carry all their geometry in the `d` attribute.
        "path" => apply_to_path(element, &combined),
        // Line elements use x1,y1,x2,y2 attributes.
        "line" => apply_to_line(element, &combined),
        // Polyline and polygon share the `points` attribute format.
        "polyline" | "polygon" => apply_to_points_attr(element, &combined),
        // Circle: translate centre; scale radius.
        "circle" => apply_to_circle(element, &combined),
        // Ellipse: translate centre; scale each radius independently.
        "ellipse" => apply_to_ellipse(element, &combined),
        // Rect: convert to a <path> so rotation/skew are always correct.
        "rect" => apply_to_rect(element, &combined),
        // All other elements (g, svg, text, …) have no geometric coordinates
        // to update directly; the transform has already been removed above.
        _ => {}
    } // match element.name

    // Recurse into child elements carrying the combined transform.
    let n = element.children.len();
    for i in 0..n {
        // Guard: only recurse into element nodes.
        let XMLNode::Element(_) = &element.children[i] else {
            continue;
        }; // XMLNode::Element guard

        // Temporarily move the child out so we can mutably recurse.
        let mut child_node = element.children.remove(i);
        if let XMLNode::Element(ref mut child_elem) = child_node {
            flatten_element(child_elem, combined);
        } // if XMLNode::Element
        // Put the child back at the same index.
        element.children.insert(i, child_node);
    } // for i
} // flatten_element

// ---------------------------------------------------------------------------
// Shape-specific transform application
// ---------------------------------------------------------------------------

/// @brief Bake the transform matrix into a `<path d="...">` element.
/// @param element  `<path>` element to modify.
/// @param matrix   Accumulated affine matrix to apply.
fn apply_to_path(element: &mut Element, matrix: &Matrix2D) {
    let d = match element.attributes.get("d") {
        Some(d) => d.clone(),
        // No `d` attribute — nothing to transform.
        None => return,
    }; // match d

    let path = match Path::parse_path_attribute(&d) {
        Ok(p) => p,
        // Leave unparseable path data unchanged.
        Err(_) => return,
    }; // match Path::parse_path_attribute

    let transformed = path.transform(matrix);
    element
        .attributes
        .insert("d".to_string(), serialize_path(&transformed));
} // apply_to_path

/// @brief Bake the transform into a `<line x1 y1 x2 y2>` element.
/// @param element  `<line>` element to modify.
/// @param matrix   Accumulated affine matrix to apply.
fn apply_to_line(element: &mut Element, matrix: &Matrix2D) {
    let x1 = get_f32(element, "x1");
    let y1 = get_f32(element, "y1");
    let x2 = get_f32(element, "x2");
    let y2 = get_f32(element, "y2");

    let p1 = matrix.apply_to_point(Point::new(x1, y1));
    let p2 = matrix.apply_to_point(Point::new(x2, y2));

    set_f32(element, "x1", p1.x);
    set_f32(element, "y1", p1.y);
    set_f32(element, "x2", p2.x);
    set_f32(element, "y2", p2.y);
} // apply_to_line

/// @brief Bake the transform into a `<polyline>` or `<polygon>` `points` attribute.
/// @details Parses comma/space-separated coordinate pairs, applies the matrix
///          to each pair, and writes back `"x0,y0 x1,y1 ..."` format.
/// @param element  `<polyline>` or `<polygon>` element to modify.
/// @param matrix   Accumulated affine matrix to apply.
fn apply_to_points_attr(element: &mut Element, matrix: &Matrix2D) {
    let pts_str = match element.attributes.get("points") {
        Some(s) => s.clone(),
        // No `points` attribute — nothing to transform.
        None => return,
    }; // match points

    // Parse all numbers in declaration order (x, y, x, y, …).
    let nums: Vec<f32> = pts_str
        .split(|c: char| c == ',' || c.is_ascii_whitespace())
        .filter(|s| !s.is_empty())
        .filter_map(|s| s.trim().parse::<f32>().ok())
        .collect();

    let mut out = String::new();
    let mut i = 0;
    // Consume pairs; skip any trailing orphaned number.
    while i + 1 < nums.len() {
        let p = matrix.apply_to_point(Point::new(nums[i], nums[i + 1]));
        if !out.is_empty() {
            out.push(' ');
        } // if not first point
        out.push_str(&format!("{},{}", fmt_f32(p.x), fmt_f32(p.y)));
        i += 2;
    } // while i

    element.attributes.insert("points".to_string(), out);
} // apply_to_points_attr

/// @brief Bake the transform into a `<circle cx cy r>` element.
/// @details The centre is transformed exactly; the radius is scaled by the
///          RMS of the matrix's two column-vector norms — exact for uniform
///          scaling and rotation, approximate for non-uniform scaling.
/// @param element  `<circle>` element to modify.
/// @param matrix   Accumulated affine matrix to apply.
fn apply_to_circle(element: &mut Element, matrix: &Matrix2D) {
    let cx = get_f32(element, "cx");
    let cy = get_f32(element, "cy");
    let r = get_f32(element, "r");

    let centre = matrix.apply_to_point(Point::new(cx, cy));
    // RMS of the column-vector norms of the linear (non-translation) part.
    let scale = col_rms_norm(matrix);

    set_f32(element, "cx", centre.x);
    set_f32(element, "cy", centre.y);
    set_f32(element, "r", r * scale);
} // apply_to_circle

/// @brief Bake the transform into an `<ellipse cx cy rx ry>` element.
/// @details The centre is transformed exactly; each radius is scaled by its
///          corresponding column-vector norm of the linear part.
/// @param element  `<ellipse>` element to modify.
/// @param matrix   Accumulated affine matrix to apply.
fn apply_to_ellipse(element: &mut Element, matrix: &Matrix2D) {
    let cx = get_f32(element, "cx");
    let cy = get_f32(element, "cy");
    let rx = get_f32(element, "rx");
    let ry = get_f32(element, "ry");

    let centre = matrix.apply_to_point(Point::new(cx, cy));
    // X-radius scales by the norm of the first column [a, b].
    let sx = (matrix.a.powi(2) + matrix.b.powi(2)).sqrt();
    // Y-radius scales by the norm of the second column [c, d].
    let sy = (matrix.c.powi(2) + matrix.d.powi(2)).sqrt();

    set_f32(element, "cx", centre.x);
    set_f32(element, "cy", centre.y);
    set_f32(element, "rx", rx * sx);
    set_f32(element, "ry", ry * sy);
} // apply_to_ellipse

/// @brief Bake the transform into a `<rect>` element.
/// @details When the accumulated matrix is axis-aligned (no rotation or skew)
///          with positive scales, the rectangle stays a `<rect>` and its
///          `x`, `y`, `width`, `height` attributes are updated in place — this
///          preserves identity-purpose rects like `id="contentRect"` so
///          downstream consumers (e.g. AdjustScene) can read them as rectangles.
///          When the matrix contains rotation, skew, or negative scales, the
///          rectangle is converted to a closed `<path>` through its four
///          transformed corners (always-correct fallback).
/// @param element  `<rect>` element to convert and modify.
/// @param matrix   Accumulated affine matrix to apply.
fn apply_to_rect(element: &mut Element, matrix: &Matrix2D) {
    let x = get_f32(element, "x");
    let y = get_f32(element, "y");
    let w = get_f32(element, "width");
    let h = get_f32(element, "height");

    // Axis-aligned, positive-scale matrix: b=c=0 and a,d > 0 means only
    // translate+uniform-axis-scale; the rect remains axis-aligned afterwards
    // and can stay a <rect> with updated geometry attributes.
    const EPS: f32 = 1e-6;
    let axis_aligned = matrix.b.abs() < EPS
        && matrix.c.abs() < EPS
        && matrix.a > 0.0
        && matrix.d > 0.0;

    if axis_aligned {
        let new_x = matrix.a * x + matrix.e;
        let new_y = matrix.d * y + matrix.f;
        let new_w = matrix.a * w;
        let new_h = matrix.d * h;
        set_f32(element, "x", new_x);
        set_f32(element, "y", new_y);
        set_f32(element, "width", new_w);
        set_f32(element, "height", new_h);
        return;
    } // if axis_aligned

    // Fallback for rotation/skew/negative-scale matrices: transform all four
    // corners and emit a closed <path>.
    let tl = matrix.apply_to_point(Point::new(x, y));
    let tr = matrix.apply_to_point(Point::new(x + w, y));
    let br = matrix.apply_to_point(Point::new(x + w, y + h));
    let bl = matrix.apply_to_point(Point::new(x, y + h));

    let d = format!(
        "M {},{} L {},{} L {},{} L {},{} Z",
        fmt_f32(tl.x),
        fmt_f32(tl.y),
        fmt_f32(tr.x),
        fmt_f32(tr.y),
        fmt_f32(br.x),
        fmt_f32(br.y),
        fmt_f32(bl.x),
        fmt_f32(bl.y),
    );

    for attr in &["x", "y", "width", "height", "rx", "ry"] {
        element.attributes.remove(*attr);
    } // for attr

    element.name = "path".to_string();
    element.attributes.insert("d".to_string(), d);
} // apply_to_rect

// ---------------------------------------------------------------------------
// SVG transform string parser
// ---------------------------------------------------------------------------

/// @brief Parse an SVG `transform` attribute string into a single `Matrix2D`.
/// @details Handles all six SVG transform functions:
///          `translate`, `scale`, `rotate`, `skewX`, `skewY`, `matrix`.
///          Multiple transforms in one string are composed left-to-right
///          (result = T1 * T2 * … applied right-to-left to points).
/// @param s  Value of the `transform` attribute.
/// @return Composed affine matrix; `Matrix2D::IDENTITY` for empty or invalid input.
pub fn parse_svg_transform(s: &str) -> Matrix2D {
    let mut result = Matrix2D::IDENTITY;
    let mut rest = s.trim();

    // Consume each `function(params)` token from the string.
    while !rest.is_empty() {
        rest = rest.trim_start_matches(|c: char| c.is_ascii_whitespace() || c == ',');
        if rest.is_empty() {
            break;
        } // if empty after trim

        // Locate the opening parenthesis.
        let paren = match rest.find('(') {
            Some(p) => p,
            None => break,
        }; // match paren
        let func_name = rest[..paren].trim();
        rest = &rest[paren + 1..];

        // Locate the closing parenthesis.
        let close = match rest.find(')') {
            Some(p) => p,
            None => break,
        }; // match close
        let params_str = &rest[..close];
        rest = &rest[close + 1..];

        // Parse all numbers inside the parentheses.
        let params: Vec<f32> = params_str
            .split(|c: char| c == ',' || c.is_ascii_whitespace())
            .filter(|s| !s.is_empty())
            .filter_map(|s| s.trim().parse::<f32>().ok())
            .collect();

        // Build the transform matrix for this function call.
        let m = match func_name {
            "translate" => {
                let tx = params.first().copied().unwrap_or(0.0);
                let ty = params.get(1).copied().unwrap_or(0.0);
                Matrix2D::from_translate(tx, ty)
            } // translate

            "scale" => {
                let sx = params.first().copied().unwrap_or(1.0);
                let sy = params.get(1).copied().unwrap_or(sx);
                Matrix2D::from_scale(sx, sy)
            } // scale

            "rotate" => {
                let angle = params.first().copied().unwrap_or(0.0);
                let cx = params.get(1).copied().unwrap_or(0.0);
                let cy = params.get(2).copied().unwrap_or(0.0);
                if cx != 0.0 || cy != 0.0 {
                    // rotate(angle,cx,cy) ≡ translate(cx,cy) · rotate(angle) · translate(-cx,-cy)
                    let t_to = Matrix2D::from_translate(cx, cy);
                    let rot = Matrix2D::from_rotate(angle);
                    let t_from = Matrix2D::from_translate(-cx, -cy);
                    t_to.mul(&rot).mul(&t_from)
                } else {
                    Matrix2D::from_rotate(angle)
                } // if cx cy not zero
            } // rotate

            "skewX" => {
                let angle = params.first().copied().unwrap_or(0.0);
                Matrix2D::from_skew_x(angle)
            } // skewX

            "skewY" => {
                let angle = params.first().copied().unwrap_or(0.0);
                Matrix2D::from_skew_y(angle)
            } // skewY

            "matrix" => {
                // matrix(a,b,c,d,e,f)
                Matrix2D {
                    a: params.first().copied().unwrap_or(1.0),
                    b: params.get(1).copied().unwrap_or(0.0),
                    c: params.get(2).copied().unwrap_or(0.0),
                    d: params.get(3).copied().unwrap_or(1.0),
                    e: params.get(4).copied().unwrap_or(0.0),
                    f: params.get(5).copied().unwrap_or(0.0),
                }
            } // matrix

            // Unknown function — skip without modifying the accumulator.
            _ => Matrix2D::IDENTITY,
        }; // match func_name

        // Compose: result = result * m (left-to-right transform application).
        result = result.mul(&m);
    } // while rest not empty

    result
} // parse_svg_transform

// ---------------------------------------------------------------------------
// Path serialisation
// ---------------------------------------------------------------------------

/// @brief Serialise a `Path` back into an SVG path data string.
/// @details All segments are written in absolute notation.
/// @param path  Path to serialise.
/// @return SVG `d` attribute value.
pub fn serialize_path(path: &Path) -> String {
    let mut out = String::new();

    for seg in &path.segments {
        if !out.is_empty() {
            out.push(' ');
        } // if not first segment

        match seg {
            PathSegment::MoveTo(p) => {
                out.push_str(&format!("M {},{}", fmt_f32(p.x), fmt_f32(p.y)));
            } // MoveTo

            PathSegment::LineTo(p) => {
                out.push_str(&format!("L {},{}", fmt_f32(p.x), fmt_f32(p.y)));
            } // LineTo

            PathSegment::QuadTo { ctrl, to } => {
                out.push_str(&format!(
                    "Q {},{} {},{}",
                    fmt_f32(ctrl.x),
                    fmt_f32(ctrl.y),
                    fmt_f32(to.x),
                    fmt_f32(to.y)
                ));
            } // QuadTo

            PathSegment::CubicTo { ctrl1, ctrl2, to } => {
                out.push_str(&format!(
                    "C {},{} {},{} {},{}",
                    fmt_f32(ctrl1.x),
                    fmt_f32(ctrl1.y),
                    fmt_f32(ctrl2.x),
                    fmt_f32(ctrl2.y),
                    fmt_f32(to.x),
                    fmt_f32(to.y)
                ));
            } // CubicTo

            PathSegment::ArcTo {
                rx,
                ry,
                x_axis_rotation,
                large_arc,
                sweep,
                to,
            } => {
                out.push_str(&format!(
                    "A {} {} {} {} {} {},{}",
                    fmt_f32(*rx),
                    fmt_f32(*ry),
                    fmt_f32(*x_axis_rotation),
                    *large_arc as u8,
                    *sweep as u8,
                    fmt_f32(to.x),
                    fmt_f32(to.y)
                ));
            } // ArcTo

            PathSegment::Close => {
                out.push('Z');
            } // Close
        } // match seg
    } // for seg

    out
} // serialize_path

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// @brief Read a floating-point attribute from an element, stripping any unit suffix.
/// @param element  Element to read from.
/// @param name     Attribute name.
/// @return Parsed value, or `0.0` if missing or unparseable.
fn get_f32(element: &Element, name: &str) -> f32 {
    element
        .attributes
        .get(name)
        .and_then(|s| {
            // Strip unit suffix (px, mm, cm, pt, etc.) before parsing.
            let trimmed = s.trim_end_matches(|c: char| c.is_alphabetic() || c == '%');
            trimmed.trim().parse::<f32>().ok()
        })
        .unwrap_or(0.0)
} // get_f32

/// @brief Write a floating-point value as an attribute string.
/// @param element  Element to update.
/// @param name     Attribute name.
/// @param value    Value to write.
fn set_f32(element: &mut Element, name: &str, value: f32) {
    element
        .attributes
        .insert(name.to_string(), fmt_f32(value));
} // set_f32

/// @brief Format an `f32` to a compact decimal string.
/// @details Writes up to 6 decimal places and strips trailing zeros.
/// @param v  Value to format.
/// @return Compact decimal string.
fn fmt_f32(v: f32) -> String {
    // Six decimal places is sufficient precision for SVG pattern data.
    let s = format!("{:.6}", v);
    // Trim trailing zeros after the decimal point.
    if s.contains('.') {
        let trimmed = s.trim_end_matches('0');
        let trimmed = trimmed.trim_end_matches('.');
        trimmed.to_string()
    } else {
        s
    } // if contains decimal point
} // fmt_f32

/// @brief Return true if the matrix is the identity transform within epsilon.
/// @param m  Matrix to test.
fn matrix_is_identity(m: &Matrix2D) -> bool {
    (m.a - 1.0).abs() < 1e-10
        && m.b.abs() < 1e-10
        && m.c.abs() < 1e-10
        && (m.d - 1.0).abs() < 1e-10
        && m.e.abs() < 1e-10
        && m.f.abs() < 1e-10
} // matrix_is_identity

/// @brief Format a `Matrix2D` as an SVG `matrix(a b c d e f)` string.
/// @param m  Matrix to format.
/// @return SVG transform attribute value string.
fn matrix_to_svg_string(m: &Matrix2D) -> String {
    format!(
        "matrix({} {} {} {} {} {})",
        fmt_f32(m.a),
        fmt_f32(m.b),
        fmt_f32(m.c),
        fmt_f32(m.d),
        fmt_f32(m.e),
        fmt_f32(m.f),
    )
} // matrix_to_svg_string

/// @brief Compute the RMS of the two column-vector norms of the linear part.
/// @details Used as the scale factor for circle radii when the transform
///          may include rotation as well as uniform scaling.
/// @param m  Matrix whose linear part to analyse.
/// @return RMS norm, ≥ 0.
fn col_rms_norm(m: &Matrix2D) -> f32 {
    // Column 1 norm: ||(a, b)||; column 2 norm: ||(c, d)||.
    let n1_sq = m.a.powi(2) + m.b.powi(2);
    let n2_sq = m.c.powi(2) + m.d.powi(2);
    ((n1_sq + n2_sq) / 2.0).sqrt()
} // col_rms_norm

// ---------------------------------------------------------------------------
// verticalize_dom
// ---------------------------------------------------------------------------

/// @brief Rotate each top-level pattern piece `<g>` so its grainline runs vertically
///        (at 90° in SVG coordinates — parallel to the Y axis, pointing down).
/// @details For each direct `<g>` child of `doc.root` that carries an `id` attribute:
///          1. Locates the grainline element — the first descendant whose `id` contains
///             the token "grainline" (case-insensitive).  Searches that element for a
///             `<path>` or `<line>`, extracts its first→last-point direction.
///          2. Falls back to the longest chord across all segment types (LineTo, QuadTo,
///             CubicTo, ArcTo) from all `<path>` and `<line>` descendants if no labelled
///             grainline is found.
///          3. Computes θ = atan2(dy, dx) for the grainline direction vector.
///          4. Computes rotation_angle = 90° − θ, normalised to [−180, 180].
///          5. Adds `transform="rotate(rotation_angle, cx, cy)"` to the `<g>`, where
///             (cx, cy) is the piece's axis-aligned bounding-box centre.
///          Groups with no geometry, no id, or whose rotation is < 0.1° are skipped.
///          Mirrors the original `verticalize_piece` / `get_rotation_angle` logic.
/// @param doc  SVG document to verticalize in-place.
pub fn verticalize_dom(doc: &mut Document) {
    // Collect rotation parameters in a first pass (immutable borrow).
    // Applied in a second pass (mutable borrow) to satisfy the borrow checker.
    struct RotParams {
        // Index within doc.root.children where the <g> element lives.
        child_idx: usize,
        // Rotation angle in degrees: 90° - θ, normalised to [-180, 180].
        angle_deg: f64,
        // Bounding-box centre of the piece — used as the rotation pivot.
        cx: f32,
        cy: f32,
    } // struct RotParams

    let mut rotations: Vec<RotParams> = Vec::new();

    // --- First pass: read geometry, compute rotation parameters ---
    for (i, child) in doc.root.children.iter().enumerate() {
        // Only process element nodes.
        let XMLNode::Element(elem) = child else {
            continue; // skip text nodes, comments, processing instructions
        }; // XMLNode::Element

        // Only direct <g> children with an id attribute are pattern pieces.
        if elem.name != "g" {
            continue; // skip defs, rect, title, etc.
        } // if not <g>
        if !elem.attributes.contains_key("id") {
            continue; // skip anonymous groups — not pattern pieces
        } // if no id

        // Locate the grainline angle (θ in degrees from the X axis).
        let Some(theta_deg) = grainline_angle(elem) else {
            continue; // no grainline and no fallback segment — skip
        }; // grainline_angle

        // Compute rotation needed to make the grainline vertical (90° in SVG).
        // Mirrors the original get_rotation_angle() function.
        let mut angle_deg = 90.0 - theta_deg;
        // Normalise to [-180, 180] to choose the shortest rotation.
        while angle_deg > 180.0 {
            angle_deg -= 360.0;
        } // while > 180
        while angle_deg < -180.0 {
            angle_deg += 360.0;
        } // while < -180

        // Skip pieces already vertical — threshold 0.1° matches original.
        if angle_deg.abs() < 0.1 {
            continue; // already vertical — no rotation needed
        } // if negligible rotation

        // Compute the bounding-box centre as the rotation pivot.
        let Some((cx, cy)) = bbox_centre(elem) else {
            continue; // no geometry — cannot compute centre
        }; // bbox_centre

        rotations.push(RotParams { child_idx: i, angle_deg, cx, cy });
    } // for (i, child) — first pass

    // --- Second pass: write transform attributes ---
    for rp in &rotations {
        let XMLNode::Element(ref mut elem) = doc.root.children[rp.child_idx] else {
            continue; // should not happen — index was validated in first pass
        }; // XMLNode::Element

        // Build "rotate(angle,cx,cy)" transform string.
        // SVG rotate(angle, cx, cy) is equivalent to
        //   translate(cx,cy) rotate(angle) translate(-cx,-cy),
        // matching the matrix chain in the original apply_rotation_angle_to_piece().
        let transform_str = format!(
            "rotate({},{},{})",
            fmt_f64(rp.angle_deg),
            fmt_f32(rp.cx),
            fmt_f32(rp.cy),
        );

        // Prepend to any existing transform so this rotation is applied first.
        let existing = elem.attributes.get("transform").cloned().unwrap_or_default();
        let new_transform = if existing.is_empty() {
            transform_str // no previous transform — set directly
        } else {
            format!("{} {}", transform_str, existing) // prepend before existing
        }; // if existing transform

        elem.attributes.insert("transform".to_string(), new_transform);
    } // for rp — second pass
} // fn verticalize_dom

// ---------------------------------------------------------------------------
// Public API — translate_dom
// ---------------------------------------------------------------------------

/// @brief Translate each top-level pattern piece `<g>` so its AABB min corner is at (0, 0).
/// @details For each direct `<g>` child of `doc.root` that carries an `id` attribute,
/// the axis-aligned bounding box of all path/line points is computed.
/// If the minimum corner is not already at the origin (tolerance 0.001 px),
/// a translate(-min_x, -min_y) is prepended to the element's transform
/// attribute so that subsequent flatten_dom bakes the translation into coordinates.
/// This mirrors the earlier desktop implementation of `translate_piece_to_origin()`.
/// @param doc  SVG document to translate in-place.
pub fn translate_dom(doc: &mut Document) {
    // Tolerance matches original translate_piece_to_origin() constant.
    const TOLERANCE: f64 = 0.001;

    // --- Struct to carry per-piece translation parameters ---
    struct TranslateParams {
        child_idx: usize,
        dx: f64,
        dy: f64,
    }

    // --- First pass: collect translation parameters (immutable borrow) ---
    let mut translations: Vec<TranslateParams> = Vec::new();

    for (i, child) in doc.root.children.iter().enumerate() {
        let XMLNode::Element(elem) = child else {
            continue; // not an element node
        }; // XMLNode::Element

        // Only translate top-level <g> elements with an id attribute (pattern pieces).
        if elem.name != "g" || !elem.attributes.contains_key("id") {
            continue; // not a pattern piece group
        } // if not a pattern piece

        // Compute the AABB minimum corner from all path/line points in the group.
        let Some((min_x, min_y)) = bbox_min(elem) else {
            continue; // no geometry — cannot determine translation
        }; // bbox_min

        let dx = -(min_x as f64);
        let dy = -(min_y as f64);

        // Skip if already at origin within tolerance.
        if dx.abs() < TOLERANCE && dy.abs() < TOLERANCE {
            continue; // already at origin — no translation needed
        } // if negligible displacement

        translations.push(TranslateParams { child_idx: i, dx, dy });
    } // for (i, child) — first pass

    // --- Second pass: write transform attributes ---
    for tp in &translations {
        let XMLNode::Element(ref mut elem) = doc.root.children[tp.child_idx] else {
            continue; // should not happen — index was validated in first pass
        }; // XMLNode::Element

        // Build "translate(dx,dy)" string.
        let transform_str = format!("translate({},{})", fmt_f64(tp.dx), fmt_f64(tp.dy));

        // Prepend to any existing transform so the translation is outermost.
        // (After flatten_dom the existing string is empty; check is defensive.)
        let existing = elem.attributes.get("transform").cloned().unwrap_or_default();
        let new_transform = if existing.is_empty() {
            transform_str // no previous transform — set directly
        } else {
            format!("{} {}", transform_str, existing) // prepend before existing
        }; // if existing transform

        elem.attributes.insert("transform".to_string(), new_transform);
    } // for tp — second pass
} // fn translate_dom

// ---------------------------------------------------------------------------
// verticalize_dom helpers
// ---------------------------------------------------------------------------

/// @brief Return the grainline angle θ (degrees from X axis) for a pattern piece.
/// @details Searches for a labelled grainline first; falls back to the longest
///          segment chord.  Returns `None` only if no geometry is found at all.
/// @param group  `<g>` element to inspect.
/// @return Angle in degrees, or `None`.
fn grainline_angle(group: &Element) -> Option<f64> {
    // Try the labelled grainline first (id contains "grainline", case-insensitive).
    if let Some(theta) = labelled_grainline_angle(group) {
        return Some(theta); // labelled grainline found
    } // if labelled

    // Fall back to the longest segment chord in the subtree.
    longest_segment_angle(group) // None if no geometry exists
} // fn grainline_angle

/// @brief Find the grainline angle from the first descendant whose id contains "grainline".
/// @details Matches the original `find_first_descendant_with_id_token(piece, "grainline")`
///          behaviour.  After finding the element:
///          - If it is a `<line>`, uses x1,y1 → x2,y2 direction.
///          - Otherwise, searches for the first descendant `<path>` and uses its
///            first→last-point direction (matches `get_first_last_points_from_path`).
/// @param group  `<g>` element to search.
/// @return Angle in degrees, or `None` if not found.
fn labelled_grainline_angle(group: &Element) -> Option<f64> {
    // Find the grainline element (or group containing it) by id token.
    let grainline_elem = find_descendant_with_id_token(group, "grainline")?;

    // If the element itself is a <line>, extract direction from its endpoints.
    if grainline_elem.name == "line" {
        let dx = (get_f32(&grainline_elem, "x2") - get_f32(&grainline_elem, "x1")) as f64;
        let dy = (get_f32(&grainline_elem, "y2") - get_f32(&grainline_elem, "y1")) as f64;
        if dx.abs() + dy.abs() > 1e-6 {
            return Some(dy.atan2(dx).to_degrees()); // valid direction from <line>
        } // if non-degenerate
    } // if <line>

    // Otherwise find the first <path> descendant and use its first→last direction.
    let path_elem = find_first_descendant_path(&grainline_elem)?;
    first_last_angle(&path_elem) // direction from first MoveTo to last distinct point
} // fn labelled_grainline_angle

/// @brief Find the first descendant element whose `id` contains `token` (case-insensitive).
/// @details Mirrors the original `find_first_descendant_with_id_token`.
/// @param element  Element to search.
/// @param token    Substring to match in id attributes.
/// @return Cloned matching element, or `None`.
fn find_descendant_with_id_token(element: &Element, token: &str) -> Option<Element> {
    // Check this element's own id.
    if let Some(id) = element.attributes.get("id") {
        if id.to_lowercase().contains(token) {
            return Some(element.clone()); // id matches — return this element
        } // if id contains token
    } // if id present

    // Recurse into children.
    for child in &element.children {
        if let XMLNode::Element(child_elem) = child {
            if let Some(found) = find_descendant_with_id_token(child_elem, token) {
                return Some(found); // found in a descendant
            } // if found
        } // if XMLNode::Element
    } // for child

    None // not found in this subtree
} // fn find_descendant_with_id_token

/// @brief Find the first `<path>` descendant (including the element itself).
/// @details Mirrors the original `find_descendant_path`.
/// @param element  Element to search.
/// @return Cloned `<path>` element, or `None`.
fn find_first_descendant_path(element: &Element) -> Option<Element> {
    // If this element is itself a <path>, return it.
    if element.name == "path" {
        return Some(element.clone()); // element is a path
    } // if path

    // Search immediate children first for performance.
    for child in &element.children {
        if let XMLNode::Element(child_elem) = child {
            if child_elem.name == "path" {
                return Some(child_elem.clone()); // immediate child path found
            } // if path child
        } // if XMLNode::Element
    } // for child

    // Deep search for nested paths.
    for child in &element.children {
        if let XMLNode::Element(child_elem) = child {
            if let Some(found) = find_first_descendant_path(child_elem) {
                return Some(found); // nested path found
            } // if found
        } // if XMLNode::Element
    } // for child

    None // no path found
} // fn find_first_descendant_path

/// @brief Compute the angle of a `<path>` grainline from its first to its last distinct point.
/// @details Mirrors the original `get_first_last_points_from_path` + `get_grainline_angle`
///          logic.  "Last distinct point" means the last endpoint that is not equal (within
///          0.001 SVG units) to the first point — handles closed paths that return to start.
/// @param element  `<path>` element.
/// @return Angle in degrees, or `None` if fewer than two distinct points exist.
fn first_last_angle(element: &Element) -> Option<f64> {
    let d = element.attributes.get("d")?;
    let path = Path::parse_path_attribute(d).ok()?;
    if path.segments.is_empty() {
        return None; // empty path
    } // if empty

    let mut first_point: Option<Point> = None;
    let mut all_points: Vec<Point> = Vec::new();

    // Collect all segment endpoints; record the first MoveTo as first_point.
    for seg in &path.segments {
        match seg {
            PathSegment::MoveTo(p) => {
                if first_point.is_none() {
                    first_point = Some(*p); // first point of the grainline
                } // if first not yet set
                all_points.push(*p);
            } // MoveTo
            PathSegment::LineTo(p)           => all_points.push(*p),
            PathSegment::QuadTo { to, .. }   => all_points.push(*to),
            PathSegment::CubicTo { to, .. }  => all_points.push(*to),
            PathSegment::ArcTo { to, .. }    => all_points.push(*to),
            PathSegment::Close               => {} // no new distinct endpoint
        } // match seg
    } // for seg

    let first = first_point?;
    if all_points.len() < 2 {
        return None; // need at least two points for a direction
    } // if too few points

    // Find the last point that is NOT equal to first_point (handles closed paths).
    let mut last_point = *all_points.last().unwrap();
    for i in (1..all_points.len()).rev() {
        let pt = all_points[i];
        if (pt.x - first.x).abs() > 0.001 || (pt.y - first.y).abs() > 0.001 {
            last_point = pt;
            break; // last distinct point found
        } // if distinct from first
    } // for i

    let dx = (last_point.x - first.x) as f64;
    let dy = (last_point.y - first.y) as f64;

    if dx.abs() + dy.abs() < 1e-6 {
        return None; // degenerate — all points coincide
    } // if degenerate

    Some(dy.atan2(dx).to_degrees()) // angle in degrees from X axis
} // fn first_last_angle

/// @brief Find the angle of the longest chord across all segment types in the subtree.
/// @details Considers LineTo, QuadTo, CubicTo, ArcTo chords and `<line>` elements.
///          Mirrors the original `calculate_longest_edge_angle` + `collect_path_segments`
///          fallback logic.
/// @param element  Root element to search recursively.
/// @return Angle in degrees, or `None` if no geometry found.
fn longest_segment_angle(element: &Element) -> Option<f64> {
    let mut max_len_sq = 0.0_f64;
    let mut best_start: Option<Point> = None;
    let mut best_end: Option<Point> = None;
    collect_longest_chord(element, &mut max_len_sq, &mut best_start, &mut best_end);

    if let (Some(s), Some(e)) = (best_start, best_end) {
        let dx = (e.x - s.x) as f64;
        let dy = (e.y - s.y) as f64;
        Some(dy.atan2(dx).to_degrees()) // angle of the longest chord
    } else {
        None // no chord found
    } // if best found
} // fn longest_segment_angle

/// @brief Recursive worker: update (max_len_sq, best_start, best_end) with the longest
///        chord (consecutive segment start→end) found in `element` and descendants.
/// @details Considers ALL segment types: LineTo, QuadTo, CubicTo, ArcTo, and `<line>`.
///          Mirrors the original `calculate_longest_edge_angle` traversal.
/// @param element    Element to inspect.
/// @param max_len_sq Running maximum of chord length²; updated in-place.
/// @param best_start Start point of the current longest chord; updated in-place.
/// @param best_end   End point of the current longest chord; updated in-place.
fn collect_longest_chord(
    element: &Element,
    max_len_sq: &mut f64,
    best_start: &mut Option<Point>,
    best_end: &mut Option<Point>,
) {
    match element.name.as_str() {
        "line" => {
            // A <line> is always a straight chord.
            let x1 = get_f32(element, "x1") as f64;
            let y1 = get_f32(element, "y1") as f64;
            let x2 = get_f32(element, "x2") as f64;
            let y2 = get_f32(element, "y2") as f64;
            let dx = x2 - x1;
            let dy = y2 - y1;
            let len_sq = dx * dx + dy * dy;
            if len_sq > *max_len_sq {
                *max_len_sq = len_sq;
                *best_start = Some(Point::new(x1 as f32, y1 as f32));
                *best_end   = Some(Point::new(x2 as f32, y2 as f32));
            } // if longer
        } // "line"

        "path" => {
            if let Some(d) = element.attributes.get("d") {
                if let Ok(path) = Path::parse_path_attribute(d) {
                    let mut current: Option<Point> = None;
                    for seg in &path.segments {
                        let end: Option<Point> = match seg {
                            PathSegment::MoveTo(p) => {
                                current = Some(*p); // update position, no chord
                                None
                            } // MoveTo
                            PathSegment::LineTo(p)          => Some(*p),
                            PathSegment::QuadTo { to, .. }  => Some(*to),
                            PathSegment::CubicTo { to, .. } => Some(*to),
                            PathSegment::ArcTo { to, .. }   => Some(*to),
                            PathSegment::Close              => None, // no chord for close
                        }; // match seg

                        if let (Some(from), Some(to)) = (current, end) {
                            let dx = (to.x - from.x) as f64;
                            let dy = (to.y - from.y) as f64;
                            let len_sq = dx * dx + dy * dy;
                            if len_sq > *max_len_sq {
                                *max_len_sq = len_sq;
                                *best_start = Some(from);
                                *best_end   = Some(to); // new longest chord
                            } // if longer
                            current = end; // advance position
                        } else if end.is_some() {
                            current = end; // advance even if no chord yet
                        } // if from and to
                    } // for seg
                } // if let Ok(path)
            } // if let Some(d)
        } // "path"

        _ => {} // other elements carry no direct segment data
    } // match element.name

    // Recurse into children.
    for child in &element.children {
        if let XMLNode::Element(child_elem) = child {
            collect_longest_chord(child_elem, max_len_sq, best_start, best_end);
        } // if XMLNode::Element
    } // for child
} // fn collect_longest_chord

/// @brief Compute the AABB centre of a pattern piece `<g>`.
/// @details Collects all coordinate points from `<path>` and `<line>` descendants
///          and returns the midpoint of the resulting axis-aligned bounding box.
/// @param group  `<g>` element to measure.
/// @return (cx, cy) bounding-box centre, or `None` if no geometry was found.
fn bbox_centre(group: &Element) -> Option<(f32, f32)> {
    let mut points: Vec<Point> = Vec::new();
    collect_all_points(group, &mut points);

    if points.is_empty() {
        return None; // no geometry — cannot compute centre
    } // if no points

    let min_x = points.iter().map(|p| p.x).fold(f32::INFINITY, f32::min);
    let max_x = points.iter().map(|p| p.x).fold(f32::NEG_INFINITY, f32::max);
    let min_y = points.iter().map(|p| p.y).fold(f32::INFINITY, f32::min);
    let max_y = points.iter().map(|p| p.y).fold(f32::NEG_INFINITY, f32::max);

    Some(((min_x + max_x) / 2.0, (min_y + max_y) / 2.0))
} // fn bbox_centre

/// @brief Compute the AABB minimum corner of a pattern piece `<g>`.
/// @details Collects all coordinate points from `<path>` and `<line>` descendants
///          and returns the (min_x, min_y) corner of the bounding box.
/// @param group  `<g>` element to measure.
/// @return (min_x, min_y) bounding-box minimum corner, or `None` if no geometry was found.
fn bbox_min(group: &Element) -> Option<(f32, f32)> {
    let mut points: Vec<Point> = Vec::new();
    collect_all_points(group, &mut points);

    if points.is_empty() {
        return None; // no geometry — cannot compute minimum
    } // if no points

    let min_x = points.iter().map(|p| p.x).fold(f32::INFINITY, f32::min);
    let min_y = points.iter().map(|p| p.y).fold(f32::INFINITY, f32::min);

    Some((min_x, min_y))
} // fn bbox_min

/// @brief Collect all coordinate points from `<path>` and `<line>` descendants.
/// @param element  Element to search recursively.
/// @param points   Output buffer; endpoints are appended.
fn collect_all_points(element: &Element, points: &mut Vec<Point>) {
    match element.name.as_str() {
        "path" => {
            if let Some(d) = element.attributes.get("d") {
                if let Ok(path) = Path::parse_path_attribute(d) {
                    for seg in &path.segments {
                        match seg {
                            PathSegment::MoveTo(p)                    => points.push(*p),
                            PathSegment::LineTo(p)                    => points.push(*p),
                            PathSegment::QuadTo { ctrl, to }          => { points.push(*ctrl); points.push(*to); }
                            PathSegment::CubicTo { ctrl1, ctrl2, to } => { points.push(*ctrl1); points.push(*ctrl2); points.push(*to); }
                            PathSegment::ArcTo { to, .. }             => points.push(*to),
                            PathSegment::Close                        => {} // no new point
                        } // match seg
                    } // for seg
                } // if let Ok(path)
            } // if let Some(d)
        } // "path"

        "line" => {
            // Include both endpoints.
            points.push(Point::new(get_f32(element, "x1"), get_f32(element, "y1")));
            points.push(Point::new(get_f32(element, "x2"), get_f32(element, "y2")));
        } // "line"

        _ => {} // other elements carry no direct coordinate data
    } // match element.name

    // Recurse into children.
    for child in &element.children {
        if let XMLNode::Element(child_elem) = child {
            collect_all_points(child_elem, points);
        } // if XMLNode::Element
    } // for child
} // fn collect_all_points

/// @brief Format an `f64` to a compact decimal string for transform attributes.
fn fmt_f64(v: f64) -> String {
    let s = format!("{:.6}", v);
    if s.contains('.') {
        let trimmed = s.trim_end_matches('0').trim_end_matches('.');
        trimmed.to_string()
    } else {
        s
    } // if contains decimal
} // fn fmt_f64

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use crate::Document;

    // @brief A simple SVG with a group that has a translate transform and a
    //        nested path.  After flatten_dom the transform must be gone and the
    //        path points shifted accordingly.
    const TRANSLATED_PATH: &str = r#"<svg width="200" height="200">
  <g transform="translate(10,20)">
    <path id="p1" d="M 0,0 L 100,0 L 100,50 Z"/>
  </g>
</svg>"#;

    #[test]
    fn translate_baked_into_path() {
        let mut doc = Document::parse(TRANSLATED_PATH).unwrap();
        flatten_dom(&mut doc);

        // No transform attributes should remain anywhere.
        let svg = doc.to_string();
        assert!(
            !svg.contains("transform="),
            "transform attribute still present after flatten"
        );

        // The path's first MoveTo should now be at (10, 20) (was (0,0) + translate(10,20)).
        let d = doc
            .get_attr_by_id("p1", "d")
            .expect("path d attribute missing");
        // After parse→transform→serialize the first command is "M 10,20".
        assert!(d.contains("M 10,20"), "expected M 10,20 in '{d}'");
    } // translate_baked_into_path

    // @brief Nested groups: outer scale(2), inner translate(5,5).
    //        A point at (1,1) should end up at (2*(1+5), 2*(1+5)) = (12,12).
    const NESTED_GROUPS: &str = r#"<svg width="200" height="200">
  <g transform="scale(2)">
    <g transform="translate(5,5)">
      <path id="p2" d="M 1,1 Z"/>
    </g>
  </g>
</svg>"#;

    #[test]
    fn nested_transforms_composed() {
        let mut doc = Document::parse(NESTED_GROUPS).unwrap();
        flatten_dom(&mut doc);

        let svg = doc.to_string();
        assert!(!svg.contains("transform="), "transform still present");

        let d = doc
            .get_attr_by_id("p2", "d")
            .expect("path d attribute missing");
        // scale(2) * translate(5,5) applied to (1,1):
        // first translate: (6,6); then scale by 2: (12,12).
        assert!(d.contains("M 12,12"), "expected M 12,12 in '{d}'");
    } // nested_transforms_composed

    // @brief Identity matrix leaves coordinates unchanged.
    const IDENTITY_SVG: &str = r#"<svg width="100" height="100">
  <g transform="translate(0,0)">
    <path id="p3" d="M 5,5 L 50,5 Z"/>
  </g>
</svg>"#;

    #[test]
    fn identity_transform_unchanged() {
        let mut doc = Document::parse(IDENTITY_SVG).unwrap();
        flatten_dom(&mut doc);

        let svg = doc.to_string();
        assert!(!svg.contains("transform="), "transform still present");

        let d = doc
            .get_attr_by_id("p3", "d")
            .expect("path d attribute missing");
        assert!(d.contains("M 5,5"), "expected M 5,5 in '{d}'");
    } // identity_transform_unchanged

    // @brief An axis-aligned <rect> (no rotation/skew, positive scale) must stay
    //        a <rect> after flatten so downstream consumers (e.g. AdjustScene's
    //        contentRect reader) can still parse x/y/width/height.
    const RECT_NO_TRANSFORM: &str = r#"<svg width="200" height="200">
  <g id="Rectangles">
    <rect id="contentRect" x="24" y="24" width="2304" height="7056" fill="none" stroke="black"/>
  </g>
</svg>"#;

    #[test]
    fn axis_aligned_rect_stays_a_rect() {
        let mut doc = Document::parse(RECT_NO_TRANSFORM).unwrap();
        flatten_dom(&mut doc);

        // contentRect must still be a <rect> with intact geometry attributes.
        let svg = doc.to_string();
        assert!(
            svg.contains(r#"<rect"#) && svg.contains(r#"id="contentRect""#),
            "contentRect was promoted away from <rect>: {svg}"
        );
        assert_eq!(doc.get_attr_by_id("contentRect", "x").as_deref(), Some("24"));
        assert_eq!(doc.get_attr_by_id("contentRect", "y").as_deref(), Some("24"));
        assert_eq!(doc.get_attr_by_id("contentRect", "width").as_deref(), Some("2304"));
        assert_eq!(doc.get_attr_by_id("contentRect", "height").as_deref(), Some("7056"));
    } // axis_aligned_rect_stays_a_rect

    // @brief Translate-only ancestor: rect stays a rect, x/y shift by the translate,
    //        width/height unchanged.
    const RECT_TRANSLATED: &str = r#"<svg width="500" height="500">
  <g transform="translate(10,20)">
    <rect id="r" x="5" y="6" width="100" height="50"/>
  </g>
</svg>"#;

    #[test]
    fn translated_rect_stays_a_rect_with_shifted_origin() {
        let mut doc = Document::parse(RECT_TRANSLATED).unwrap();
        flatten_dom(&mut doc);

        let svg = doc.to_string();
        assert!(svg.contains("<rect"), "rect collapsed to path: {svg}");
        assert_eq!(doc.get_attr_by_id("r", "x").as_deref(), Some("15"));
        assert_eq!(doc.get_attr_by_id("r", "y").as_deref(), Some("26"));
        assert_eq!(doc.get_attr_by_id("r", "width").as_deref(), Some("100"));
        assert_eq!(doc.get_attr_by_id("r", "height").as_deref(), Some("50"));
    } // translated_rect_stays_a_rect_with_shifted_origin

    // @brief Rotation present: rect MUST still be converted to a <path> because the
    //        result is no longer axis-aligned (always-correct fallback).
    const RECT_ROTATED: &str = r#"<svg width="200" height="200">
  <g transform="rotate(30)">
    <rect id="r2" x="0" y="0" width="100" height="50"/>
  </g>
</svg>"#;

    #[test]
    fn rotated_rect_becomes_path() {
        let mut doc = Document::parse(RECT_ROTATED).unwrap();
        flatten_dom(&mut doc);

        // r2 must now be a <path> with a `d` attribute and no `x`/`width`.
        assert!(doc.get_attr_by_id("r2", "d").is_some(),
                "rotated rect should have been converted to a <path> with a `d` attribute");
        assert!(doc.get_attr_by_id("r2", "x").is_none(),
                "rotated rect should no longer expose `x` attribute");
    } // rotated_rect_becomes_path

    // @brief Rotation around origin: rotate(90) maps (1,0) to (0,1).
    const ROTATED_PATH: &str = r#"<svg width="200" height="200">
  <g transform="rotate(90)">
    <path id="p4" d="M 1,0 Z"/>
  </g>
</svg>"#;

    #[test]
    fn rotation_applied_correctly() {
        let mut doc = Document::parse(ROTATED_PATH).unwrap();
        flatten_dom(&mut doc);

        let d = doc
            .get_attr_by_id("p4", "d")
            .expect("path d attribute missing");
        // rotate(90°) maps (1,0) → (0,1) in standard SVG coordinates.
        // Allow for floating-point epsilon in the serialised output.
        let path = geometry::Path::parse_path_attribute(&d).expect("parse path");
        if let geometry::PathSegment::MoveTo(p) = path.segments[0] {
            assert!((p.x - 0.0).abs() < 1e-4, "x expected ~0, got {}", p.x);
            assert!((p.y - 1.0).abs() < 1e-4, "y expected ~1, got {}", p.y);
        } else {
            panic!("expected MoveTo segment");
        } // if MoveTo
    } // rotation_applied_correctly

    // -----------------------------------------------------------------------
    // verticalize_dom tests
    // -----------------------------------------------------------------------

    // @brief A grainline <line id="grainline"> at 45° (θ=45°) should produce
    //        rotate(+45°, cx, cy) on the <g> — 90° - 45° = 45°, makes it vertical.
    const GRAINLINE_45: &str = r#"<svg width="200" height="200">
  <g id="piece-1">
    <path d="M 0,0 L 100,0 L 100,80 L 0,80 Z"/>
    <line id="grainline" x1="10" y1="10" x2="20" y2="20"/>
  </g>
</svg>"#;

    #[test]
    fn grainline_at_45_gets_rotate_transform() {
        let mut doc = Document::parse(GRAINLINE_45).unwrap();
        verticalize_dom(&mut doc);

        // The <g> should have a transform attribute.
        let svg = doc.to_string();
        assert!(svg.contains("rotate("), "expected rotate() transform: {svg}");

        // rotation_angle = 90° - 45° = +45°.
        // The grainline (dx=10, dy=10) has θ = atan2(10,10) = 45°.
        // To make it vertical (90°): rotate piece by 90° - 45° = +45°.
        let transform = doc
            .get_attr_by_id("piece-1", "transform")
            .expect("piece-1 missing transform after verticalize_dom");
        assert!(
            transform.starts_with("rotate(45"),
            "expected rotate(45...) in '{transform}'"
        );
    } // grainline_at_45_gets_rotate_transform

    // @brief A grainline already vertical (θ=90°, pointing down) must not be rotated.
    // rotation_angle = 90° - 90° = 0° → skip.
    const GRAINLINE_VERTICAL: &str = r#"<svg width="200" height="200">
  <g id="piece-2">
    <path d="M 0,0 L 100,0 L 100,80 L 0,80 Z"/>
    <line id="grainline" x1="50" y1="0" x2="50" y2="100"/>
  </g>
</svg>"#;

    #[test]
    fn grainline_already_vertical_unchanged() {
        let mut doc = Document::parse(GRAINLINE_VERTICAL).unwrap();
        verticalize_dom(&mut doc);

        // Grainline (dx=0, dy=100) → θ = 90° → rotation = 0° → skip.
        // No transform attribute should be added.
        let transform = doc.get_attr_by_id("piece-2", "transform");
        assert!(
            transform.is_none(),
            "piece with already-vertical grainline should not receive a transform, got {:?}",
            transform
        );
    } // grainline_already_vertical_unchanged

    // @brief When no labelled grainline exists, the longest chord is used.
    // Path has a short horizontal L(10,0) and a longer 45° L(30,30).
    // Longest chord: (0,0)→(30,30), θ=45° → rotation = 90°-45° = 45°.
    const NO_LABEL_FALLBACK: &str = r#"<svg width="200" height="200">
  <g id="piece-3">
    <path d="M 0,0 L 10,0 M 0,0 L 30,30"/>
  </g>
</svg>"#;

    #[test]
    fn no_grainline_fallback_uses_longest_segment() {
        let mut doc = Document::parse(NO_LABEL_FALLBACK).unwrap();
        verticalize_dom(&mut doc);

        // Longest chord (0,0)→(30,30): θ=45° → rotation = +45°.
        let transform = doc
            .get_attr_by_id("piece-3", "transform")
            .expect("piece-3 missing transform after verticalize_dom");
        assert!(
            transform.starts_with("rotate(45"),
            "expected rotate(45...) for 45° longest chord, got '{transform}'"
        );
    } // no_grainline_fallback_uses_longest_segment

    // @brief A group with a grainline identified by id token "grainline" inside a <g>.
    // Seamly2D SVGs typically wrap grainlines in <g id="grainline-..."><path .../></g>.
    const GRAINLINE_IN_GROUP: &str = r#"<svg width="200" height="200">
  <g id="piece-4">
    <path d="M 0,0 L 80,0 L 80,60 L 0,60 Z"/>
    <g id="grainline-arrow">
      <path d="M 0,0 L 30,30"/>
    </g>
  </g>
</svg>"#;

    #[test]
    fn grainline_in_child_group_found() {
        let mut doc = Document::parse(GRAINLINE_IN_GROUP).unwrap();
        verticalize_dom(&mut doc);

        // id "grainline-arrow" contains "grainline"; path direction (30,30) → θ=45°
        // → rotation = +45°.
        let transform = doc
            .get_attr_by_id("piece-4", "transform")
            .expect("piece-4 missing transform");
        assert!(
            transform.starts_with("rotate(45"),
            "expected rotate(45...) from grainline child group, got '{transform}'"
        );
    } // grainline_in_child_group_found

    // @brief A group with no geometry at all is left unchanged.
    const EMPTY_GROUP: &str = r#"<svg width="200" height="200">
  <g id="empty"/>
</svg>"#;

    #[test]
    fn empty_group_left_unchanged() {
        let mut doc = Document::parse(EMPTY_GROUP).unwrap();
        verticalize_dom(&mut doc);

        // No geometry → no transform should be added.
        let transform = doc.get_attr_by_id("empty", "transform");
        assert!(
            transform.is_none(),
            "empty group should not receive a transform, got {:?}",
            transform
        );
    } // empty_group_left_unchanged

    // -----------------------------------------------------------------------
    // translate_dom tests
    // -----------------------------------------------------------------------

    // @brief A pattern piece whose AABB min corner is at (50, 80) must receive
    //        translate(-50,-80) so its bounding box starts at (0,0).
    const PIECE_OFFSET: &str = r#"<svg width="300" height="300">
  <g id="piece-A">
    <path id="path-A" d="M 50,80 L 150,80 L 150,180 Z"/>
  </g>
</svg>"#;

    #[test]
    fn piece_at_offset_translated_to_origin() {
        let mut doc = Document::parse(PIECE_OFFSET).unwrap();
        translate_dom(&mut doc);

        let transform = doc
            .get_attr_by_id("piece-A", "transform")
            .expect("piece-A missing transform after translate_dom");

        // Expected: translate(-50,-80) — negative of the AABB min corner.
        assert!(
            transform.starts_with("translate(-50"),
            "expected translate(-50,...) got '{transform}'"
        );
        assert!(
            transform.contains("-80"),
            "expected -80 dy in transform, got '{transform}'"
        );
    } // piece_at_offset_translated_to_origin

    // @brief A pattern piece already at (0,0) must not receive any transform.
    const PIECE_AT_ORIGIN: &str = r#"<svg width="200" height="200">
  <g id="piece-B">
    <path d="M 0,0 L 100,0 L 100,100 Z"/>
  </g>
</svg>"#;

    #[test]
    fn piece_already_at_origin_unchanged() {
        let mut doc = Document::parse(PIECE_AT_ORIGIN).unwrap();
        translate_dom(&mut doc);

        let transform = doc.get_attr_by_id("piece-B", "transform");
        assert!(
            transform.is_none(),
            "piece already at origin should not receive a transform, got {:?}",
            transform
        );
    } // piece_already_at_origin_unchanged

    // @brief translate_dom followed by flatten_dom must produce a piece with
    //        its AABB min corner at (0,0) and no remaining transform attributes.
    #[test]
    fn translate_then_flatten_clears_transforms() {
        let mut doc = Document::parse(PIECE_OFFSET).unwrap();
        translate_dom(&mut doc);
        flatten_dom(&mut doc);

        // No transform attributes should remain.
        let svg = doc.to_string();
        assert!(
            !svg.contains("transform="),
            "transform attribute still present after translate_dom + flatten_dom"
        );

        // The path's first point should now be near (0, 0).
        let d = doc
            .get_attr_by_id("path-A", "d")
            .expect("path d attribute missing after translate+flatten");
        let path = geometry::Path::parse_path_attribute(d).expect("parse path");
        if let geometry::PathSegment::MoveTo(p) = &path.segments[0] {
            assert!(
                p.x.abs() < 0.01,
                "first point x should be ~0 after translate, got {}",
                p.x
            );
            assert!(
                p.y.abs() < 0.01,
                "first point y should be ~0 after translate, got {}",
                p.y
            );
        } // if MoveTo
    } // translate_then_flatten_clears_transforms

    // -----------------------------------------------------------------------
    // Reserved-ID exclusion tests
    // -----------------------------------------------------------------------

    // @brief An SVG with backgroundRects, contentRects, and tiledRects groups
    //        that each carry a translate transform.  After flatten_dom all three
    //        groups must retain their transforms and their children must be
    //        untouched, while a regular sibling path IS flattened normally.
    const RESERVED_IDS_SVG: &str = r#"<svg width="500" height="500">
  <g id="backgroundRects" transform="translate(5,5)">
    <rect id="bgr" x="0" y="0" width="100" height="100"/>
  </g>
  <g id="contentRects" transform="translate(3,3)">
    <rect id="cr" x="10" y="10" width="80" height="80"/>
  </g>
  <g id="tiledRects" transform="translate(2,2)">
    <path id="tr" d="M 0,0 L 50,0 Z"/>
  </g>
  <g transform="translate(10,20)">
    <path id="regular" d="M 0,0 L 100,0 Z"/>
  </g>
</svg>"#;

    // @brief backgroundRects, contentRects, and tiledRects groups must not have
    //        their transforms removed or their children's coordinates modified.
    //        A regular element outside these groups must still be flattened.
    #[test]
    fn reserved_layout_ids_excluded_from_flatten() {
        let mut doc = Document::parse(RESERVED_IDS_SVG).unwrap();
        flatten_dom(&mut doc);

        // All three reserved groups must retain their transform attributes.
        let bg_t = doc.get_attr_by_id("backgroundRects", "transform");
        assert_eq!(
            bg_t.as_deref(), Some("translate(5,5)"),
            "backgroundRects transform must be preserved, got {:?}", bg_t
        );

        let cr_t = doc.get_attr_by_id("contentRects", "transform");
        assert_eq!(
            cr_t.as_deref(), Some("translate(3,3)"),
            "contentRects transform must be preserved, got {:?}", cr_t
        );

        let tr_t = doc.get_attr_by_id("tiledRects", "transform");
        assert_eq!(
            tr_t.as_deref(), Some("translate(2,2)"),
            "tiledRects transform must be preserved, got {:?}", tr_t
        );

        // Children of reserved groups must not have transforms baked into them.
        assert_eq!(doc.get_attr_by_id("bgr", "x").as_deref(), Some("0"),
            "bgr.x must stay 0 — backgroundRects children must not be flattened");
        assert_eq!(doc.get_attr_by_id("bgr", "y").as_deref(), Some("0"),
            "bgr.y must stay 0 — backgroundRects children must not be flattened");
        assert_eq!(doc.get_attr_by_id("cr", "x").as_deref(), Some("10"),
            "cr.x must stay 10 — contentRects children must not be flattened");
        assert_eq!(doc.get_attr_by_id("cr", "y").as_deref(), Some("10"),
            "cr.y must stay 10 — contentRects children must not be flattened");

        let tr_d = doc.get_attr_by_id("tr", "d").expect("tiledRects child path must still exist");
        assert!(tr_d.contains("M 0,0"), "tiledRects path d must be unchanged, got '{tr_d}'");

        // The regular element outside reserved groups must still be flattened.
        let d = doc.get_attr_by_id("regular", "d")
            .expect("regular path must still exist after flatten");
        assert!(d.contains("M 10,20"),
            "regular path must be shifted by translate(10,20), got '{d}'");
    } // reserved_layout_ids_excluded_from_flatten

    // @brief backgroundRects must not be deleted — it must remain in the DOM.
    #[test]
    fn background_rects_group_survives_flatten() {
        let mut doc = Document::parse(RESERVED_IDS_SVG).unwrap();
        flatten_dom(&mut doc);
        let svg = doc.to_string();
        assert!(svg.contains(r#"id="backgroundRects""#),
            "backgroundRects group must not be deleted by flatten_dom");
    } // background_rects_group_survives_flatten

    // @brief contentRects must not be deleted — it must remain in the DOM.
    #[test]
    fn content_rects_group_survives_flatten() {
        let mut doc = Document::parse(RESERVED_IDS_SVG).unwrap();
        flatten_dom(&mut doc);
        let svg = doc.to_string();
        assert!(svg.contains(r#"id="contentRects""#),
            "contentRects group must not be deleted by flatten_dom");
    } // content_rects_group_survives_flatten

    // @brief tiledRects must not be deleted — it must remain in the DOM.
    #[test]
    fn tiled_rects_group_survives_flatten() {
        let mut doc = Document::parse(RESERVED_IDS_SVG).unwrap();
        flatten_dom(&mut doc);
        let svg = doc.to_string();
        assert!(svg.contains(r#"id="tiledRects""#),
            "tiledRects group must not be deleted by flatten_dom");
    } // tiled_rects_group_survives_flatten

    // @brief When reserved groups are nested inside a group that carries a
    //        transform, the reserved groups must still be skipped and their own
    //        transform must be preserved (the ancestor's transform is not applied
    //        to them, and their children are untouched).
    const RESERVED_NESTED_SVG: &str = r#"<svg width="400" height="400">
  <g id="Rectangles" transform="translate(24,24)">
    <g id="backgroundRects" transform="translate(0,0)">
      <rect id="bgrn" x="0" y="0" width="200" height="300"/>
    </g>
    <g id="contentRects" transform="translate(10,10)">
      <rect id="crn" x="5" y="5" width="180" height="280"/>
    </g>
  </g>
</svg>"#;

    // @brief Reserved groups nested inside a transformed Rectangles group must
    //        not have any transform — ancestor or own — baked into their children.
    #[test]
    fn reserved_ids_nested_inside_transformed_parent_preserved() {
        let mut doc = Document::parse(RESERVED_NESTED_SVG).unwrap();
        flatten_dom(&mut doc);

        // backgroundRects must retain its own transform.
        let bg_t = doc.get_attr_by_id("backgroundRects", "transform");
        assert_eq!(bg_t.as_deref(), Some("translate(0,0)"),
            "nested backgroundRects transform must be preserved, got {:?}", bg_t);

        // contentRects must retain its own transform.
        let cr_t = doc.get_attr_by_id("contentRects", "transform");
        assert_eq!(cr_t.as_deref(), Some("translate(10,10)"),
            "nested contentRects transform must be preserved, got {:?}", cr_t);

        // Children of the reserved groups must be untouched (no ancestor or own
        // transforms baked in).
        assert_eq!(doc.get_attr_by_id("bgrn", "x").as_deref(), Some("0"),
            "bgrn.x must stay 0 — backgroundRects children must not be flattened");
        assert_eq!(doc.get_attr_by_id("crn", "x").as_deref(), Some("5"),
            "crn.x must stay 5 — contentRects children must not be flattened");
    } // reserved_ids_nested_inside_transformed_parent_preserved

    // @brief parse_svg_transform handles all six function forms.
    #[test]
    fn parse_all_transform_functions() {
        // translate
        let m = parse_svg_transform("translate(3,4)");
        let p = m.apply_to_point(Point::new(0.0, 0.0));
        assert!((p.x - 3.0).abs() < 1e-5, "translate x");
        assert!((p.y - 4.0).abs() < 1e-5, "translate y");

        // scale
        let m = parse_svg_transform("scale(2,3)");
        let p = m.apply_to_point(Point::new(1.0, 1.0));
        assert!((p.x - 2.0).abs() < 1e-5, "scale x");
        assert!((p.y - 3.0).abs() < 1e-5, "scale y");

        // matrix identity
        let m = parse_svg_transform("matrix(1,0,0,1,5,6)");
        let p = m.apply_to_point(Point::new(0.0, 0.0));
        assert!((p.x - 5.0).abs() < 1e-5, "matrix tx");
        assert!((p.y - 6.0).abs() < 1e-5, "matrix ty");

        // skewX (non-zero result on y=1 point)
        let m = parse_svg_transform("skewX(45)");
        let p = m.apply_to_point(Point::new(0.0, 1.0));
        assert!((p.x - 1.0).abs() < 1e-4, "skewX");
    } // parse_all_transform_functions
} // mod tests
