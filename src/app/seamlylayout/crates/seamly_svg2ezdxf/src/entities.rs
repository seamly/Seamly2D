// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief DXF entity types for intermediate representation.

// @brief Point in 2D space.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Point {
    // X coordinate.
    pub x: f64,
    // Y coordinate.
    pub y: f64,
}

impl Point {
    // @brief Create a new point.
    // @param x X coordinate.
    // @param y Y coordinate.
    // @return New point.
    pub fn new(x: f64, y: f64) -> Self {
        Self { x, y }
    }
}

// @brief Base trait for DXF entities.
pub trait Entity: std::any::Any {
    // Get the layer name for this entity.
    fn layer(&self) -> &str;
    // Get the entity type name.
    fn entity_type(&self) -> &str;
}

// @brief LINE entity.
#[derive(Debug, Clone)]
pub struct Line {
    // Layer name.
    pub layer: String,
    // Start point.
    pub start: Point,
    // End point.
    pub end: Point,
}

// @brief ARC entity.
#[derive(Debug, Clone)]
pub struct Arc {
    // Layer name.
    pub layer: String,
    // Center point.
    pub center: Point,
    // Radius.
    pub radius: f64,
    // Start angle in degrees.
    pub start_angle: f64,
    // End angle in degrees.
    pub end_angle: f64,
}

// @brief CIRCLE entity.
#[derive(Debug, Clone)]
pub struct Circle {
    // Layer name.
    pub layer: String,
    // Center point.
    pub center: Point,
    // Radius.
    pub radius: f64,
}

// @brief POLYLINE entity.
#[derive(Debug, Clone)]
pub struct Polyline {
    // Layer name.
    pub layer: String,
    // Vertices.
    pub vertices: Vec<Point>,
    // Whether the polyline is closed.
    pub closed: bool,
}

// @brief DXF POINT entity — layer-2 turn points and layer-3 curve points.
//
// The ASTM standard uses POINT entities to annotate each boundary vertex:
// - Layer "2" = turn point (corner, selectable endpoint)
// - Layer "3" = curve point (smooth interpolation detail)
// CLO3D reads these to distinguish corners from curve segments.
#[derive(Debug, Clone)]
pub struct DxfPoint {
    // Layer name ("2" = turn/corner, "3" = curve point).
    pub layer: String,
    // Position.
    pub position: Point,
}

impl DxfPoint {
    // @brief Create a new DXF POINT entity.
    // @param layer Layer string ("2" or "3").
    // @param position Point coordinates.
    // @return New DxfPoint.
    pub fn new(layer: impl Into<String>, position: Point) -> Self {
        Self {
            layer: layer.into(),
            position,
        }
    } // fn new
} // impl DxfPoint

impl Entity for DxfPoint {
    fn layer(&self) -> &str {
        &self.layer
    } // fn layer

    fn entity_type(&self) -> &str {
        "POINT"
    } // fn entity_type
} // impl Entity for DxfPoint

// @brief TEXT entity.
#[derive(Debug, Clone)]
pub struct Text {
    // Layer name.
    pub layer: String,
    // Insertion point.
    pub insertion_point: Point,
    // Text height.
    pub height: f64,
    // Rotation angle in degrees.
    pub rotation: f64,
    // Text content (ASCII-only).
    pub content: String,
}

// Implement Entity trait for all entity types.

impl Entity for Line {
    fn layer(&self) -> &str {
        &self.layer
    }

    fn entity_type(&self) -> &str {
        "LINE"
    }
}

impl Entity for Arc {
    fn layer(&self) -> &str {
        &self.layer
    }

    fn entity_type(&self) -> &str {
        "ARC"
    }
}

impl Entity for Circle {
    fn layer(&self) -> &str {
        &self.layer
    }

    fn entity_type(&self) -> &str {
        "CIRCLE"
    }
}

impl Entity for Polyline {
    fn layer(&self) -> &str {
        &self.layer
    }

    fn entity_type(&self) -> &str {
        "POLYLINE"
    }
}

impl Entity for Text {
    fn layer(&self) -> &str {
        &self.layer
    }

    fn entity_type(&self) -> &str {
        "TEXT"
    }
}
