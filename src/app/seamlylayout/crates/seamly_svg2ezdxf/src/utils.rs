// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Utility functions for coordinate transformation, text sanitization,
//!        and corner detection.

use crate::entities::Point;

/// @brief Transform a point by inverting the Y-axis (SVG to DXF coordinate system).
/// @param point Point in SVG coordinates (origin top-left).
/// @param svg_height SVG document height in user units.
/// @return Point in DXF coordinates (origin bottom-left).
pub fn invert_y_axis(point: Point, svg_height: f64) -> Point {
    Point::new(point.x, svg_height - point.y)
} // fn invert_y_axis

/// @brief Parse a floating-point number from a string attribute.
/// @param value String value to parse (from xmltree attributes).
/// @param default Default value if the attribute is absent or unparseable.
/// @return Parsed float or default.
pub fn parse_float_attr(value: Option<&String>, default: f64) -> f64 {
    value.and_then(|v| v.parse::<f64>().ok()).unwrap_or(default)
} // fn parse_float_attr

/// @brief Sanitize text to ASCII-only (required for ASTM-D6673-10).
/// @param text Input text (may contain Unicode).
/// @return ASCII-only text with non-ASCII characters removed.
pub fn sanitize_ascii(text: &str) -> String {
    text.chars().filter(|c| c.is_ascii()).collect()
} // fn sanitize_ascii

/// @brief Sanitize a block name to be valid for DXF (ASCII alphanumeric, `_`, `-`).
/// @param name Original name.
/// @return Sanitized name.
pub fn sanitize_block_name(name: &str) -> String {
    name.chars()
        .filter(|c| c.is_ascii_alphanumeric() || *c == '_' || *c == '-')
        .collect()
} // fn sanitize_block_name

// ---------------------------------------------------------------------------
// Corner detection (mirrors seamly2clo.py detect_corners / calculate_angle)
// ---------------------------------------------------------------------------

/// @brief Calculate the interior angle at `p2` formed by the path p1→p2→p3.
/// @param p1 Previous vertex.
/// @param p2 Current vertex (angle is measured here).
/// @param p3 Next vertex.
/// @return Angle in degrees in the range [0.0, 180.0].
///         Returns 180.0 for degenerate (zero-length) segments.
fn calculate_angle_at(p1: Point, p2: Point, p3: Point) -> f64 {
    // Vectors from p2 toward p1 and toward p3.
    let v1 = (p1.x - p2.x, p1.y - p2.y);
    let v2 = (p3.x - p2.x, p3.y - p2.y);

    let mag1 = (v1.0 * v1.0 + v1.1 * v1.1).sqrt();
    let mag2 = (v2.0 * v2.0 + v2.1 * v2.1).sqrt();

    // Degenerate segment — treat as straight (180°).
    if mag1 == 0.0 || mag2 == 0.0 {
        return 180.0;
    } // if degenerate

    let dot = v1.0 * v2.0 + v1.1 * v2.1;
    let cos_angle = (dot / (mag1 * mag2)).clamp(-1.0, 1.0);
    cos_angle.acos().to_degrees()
} // fn calculate_angle_at

/// @brief Classify each vertex of a closed polyline as a turn point or curve point.
///
/// A vertex is a **turn point** (corner) when the interior angle is less than
/// `angle_threshold_degrees`.  The default threshold used by CLO3D / seamly2clo.py
/// is **120°** — anything sharper than 120° is a corner.
///
/// The polyline is treated as closed: the neighbourhood of vertex 0 wraps around
/// to the last vertex, and vice versa.
///
/// @param vertices  Ordered boundary vertices of the closed polygon.
/// @param angle_threshold_degrees  Angles strictly less than this value mark a corner.
/// @return `Vec<bool>` parallel to `vertices`; `true` = turn point, `false` = curve point.
pub fn detect_corners(vertices: &[Point], angle_threshold_degrees: f64) -> Vec<bool> {
    let n = vertices.len();

    // Need at least 3 vertices for angle calculation.
    if n < 3 {
        return vec![false; n];
    } // if too few vertices

    let mut is_corner = vec![false; n];

    for i in 0..n {
        // Wrap-around neighbours for closed polygon.
        let p1 = vertices[(i + n - 1) % n];
        let p2 = vertices[i];
        let p3 = vertices[(i + 1) % n];

        let angle = calculate_angle_at(p1, p2, p3);

        // Sharp angle → corner / turn point.
        if angle < angle_threshold_degrees {
            is_corner[i] = true;
        } // if corner
    } // for each vertex

    is_corner
} // fn detect_corners
