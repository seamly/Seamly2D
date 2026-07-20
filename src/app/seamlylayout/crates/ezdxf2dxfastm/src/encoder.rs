// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief DXF entity encoding using group codes.

use seamly_svg2ezdxf::{Circle, DxfPoint, Entity, Line, Point, Polyline, Text};
use std::any::Any;
use std::io::Write;

// @brief Write a group code and value to the DXF file.
// @param writer The writer to write to.
// @param code The group code (integer).
// @param value The value (as string).
// @return Result indicating success or error.
fn write_group_code(writer: &mut dyn Write, code: i32, value: &str) -> std::io::Result<()> {
    writeln!(writer, "{}", code)?;
    writeln!(writer, "{}", value)?;
    Ok(())
}

// @brief Write a group code with integer value.
// @param writer The writer to write to.
// @param code The group code.
// @param value The integer value.
// @return Result indicating success or error.
fn write_group_code_int(writer: &mut dyn Write, code: i32, value: i32) -> std::io::Result<()> {
    writeln!(writer, "{}", code)?;
    writeln!(writer, "{}", value)?;
    Ok(())
}

// @brief Write a group code with float value.
// @param writer The writer to write to.
// @param code The group code.
// @param value The float value.
// @return Result indicating success or error.
fn write_group_code_float(writer: &mut dyn Write, code: i32, value: f64) -> std::io::Result<()> {
    writeln!(writer, "{}", code)?;
    writeln!(writer, "{:.6}", value)?;
    Ok(())
}

// @brief Encode a LINE entity to DXF format.
// @param writer The writer to write to.
// @param line The LINE entity to encode.
// @return Result indicating success or error.
pub fn encode_line(writer: &mut dyn Write, line: &Line) -> std::io::Result<()> {
    // Entity type.
    write_group_code(writer, 0, "LINE")?;

    // Layer name (group code 8).
    write_group_code(writer, 8, &line.layer)?;

    // Start point (group codes 10, 20 for X, Y).
    write_group_code_float(writer, 10, line.start.x)?;
    write_group_code_float(writer, 20, line.start.y)?;

    // End point (group codes 11, 21 for X, Y).
    write_group_code_float(writer, 11, line.end.x)?;
    write_group_code_float(writer, 21, line.end.y)?;

    Ok(())
}

// @brief Encode a CIRCLE entity to DXF format.
// @param writer The writer to write to.
// @param circle The CIRCLE entity to encode.
// @return Result indicating success or error.
pub fn encode_circle(writer: &mut dyn Write, circle: &Circle) -> std::io::Result<()> {
    // Entity type.
    write_group_code(writer, 0, "CIRCLE")?;

    // Layer name (group code 8).
    write_group_code(writer, 8, &circle.layer)?;

    // Center point (group codes 10, 20 for X, Y).
    write_group_code_float(writer, 10, circle.center.x)?;
    write_group_code_float(writer, 20, circle.center.y)?;

    // Radius (group code 40).
    write_group_code_float(writer, 40, circle.radius)?;

    Ok(())
}

// @brief Encode a POLYLINE entity to DXF format.
// @param writer The writer to write to.
// @param polyline The POLYLINE entity to encode.
// @return Result indicating success or error.
pub fn encode_polyline(writer: &mut dyn Write, polyline: &Polyline) -> std::io::Result<()> {
    // Entity type.
    write_group_code(writer, 0, "POLYLINE")?;

    // Layer name (group code 8).
    write_group_code(writer, 8, &polyline.layer)?;

    // Closed flag (group code 70: 1 = closed, 0 = open).
    let flags = if polyline.closed { 1 } else { 0 };
    write_group_code_int(writer, 70, flags)?;

    // Write each vertex as a VERTEX entity.
    for vertex in &polyline.vertices {
        // VERTEX entity type.
        write_group_code(writer, 0, "VERTEX")?;

        // Layer name (group code 8).
        write_group_code(writer, 8, &polyline.layer)?;

        // Vertex coordinates (group codes 10, 20 for X, Y).
        write_group_code_float(writer, 10, vertex.x)?;
        write_group_code_float(writer, 20, vertex.y)?;

        // Vertex flag (group code 70: 32 = polyline vertex, 128 = curve-fit vertex).
        write_group_code_int(writer, 70, 32)?;
    }

    // End of polyline sequence (SEQEND).
    write_group_code(writer, 0, "SEQEND")?;
    write_group_code(writer, 8, &polyline.layer)?;

    Ok(())
}

// @brief Encode a TEXT entity to DXF format.
// @param writer The writer to write to.
// @param text The TEXT entity to encode.
// @return Result indicating success or error.
pub fn encode_text(writer: &mut dyn Write, text: &Text) -> std::io::Result<()> {
    // Entity type.
    write_group_code(writer, 0, "TEXT")?;

    // Layer name (group code 8).
    write_group_code(writer, 8, &text.layer)?;

    // Insertion point (group codes 10, 20 for X, Y).
    write_group_code_float(writer, 10, text.insertion_point.x)?;
    write_group_code_float(writer, 20, text.insertion_point.y)?;

    // Text height (group code 40).
    write_group_code_float(writer, 40, text.height)?;

    // Text content (group code 1).
    write_group_code(writer, 1, &text.content)?;

    // Rotation angle in degrees (group code 50).
    write_group_code_float(writer, 50, text.rotation)?;

    Ok(())
}

// @brief Encode a DXF POINT entity (layer-2 turn points and layer-3 curve points).
// @param writer The writer to write to.
// @param point The DxfPoint entity to encode.
// @return Result indicating success or error.
pub fn encode_dxf_point(writer: &mut dyn Write, point: &DxfPoint) -> std::io::Result<()> {
    // Entity type.
    write_group_code(writer, 0, "POINT")?;

    // Layer name (group code 8): "2" = turn point, "3" = curve point.
    write_group_code(writer, 8, &point.layer)?;

    // Position (group codes 10, 20 for X, Y).
    write_group_code_float(writer, 10, point.position.x)?;
    write_group_code_float(writer, 20, point.position.y)?;

    Ok(())
} // fn encode_dxf_point

// @brief Encode a CLO3D-style POLYLINE with the vertices-follow flag and group-250 marker.
//
// Produces the exact POLYLINE + VERTEX + SEQEND sequence used by seamly2clo.py:
//   POLYLINE / layer / 66=1 / 70=1 / 250=group_250
//   VERTEX   / layer / 10=x / 20=y   (no 70=32 vertex flag)
//   ...
//   SEQEND
//
// @param writer     The writer to write to.
// @param vertices   Ordered list of vertex coordinates.
// @param layer      DXF layer string for the POLYLINE and its VERTEXes.
// @param group_250  Value for group code 250 (2 = sewing line, 0 = boundary).
// @return Result indicating success or error.
pub fn encode_clo_polyline(
    writer: &mut dyn Write,
    vertices: &[Point],
    layer: &str,
    group_250: i32,
) -> std::io::Result<()> {
    // Entity type.
    write_group_code(writer, 0, "POLYLINE")?;

    // Layer (group code 8).
    write_group_code(writer, 8, layer)?;

    // Vertices-follow flag (group code 66: 1 = vertices follow).
    write_group_code_int(writer, 66, 1)?;

    // Closed polyline flag (group code 70: 1 = closed).
    write_group_code_int(writer, 70, 1)?;

    // Group code 250: 2 = sewing line, 0 = boundary.
    write_group_code_int(writer, 250, group_250)?;

    // Write each vertex as a VERTEX entity (no 70=32 vertex flag, matching seamly2clo.py).
    for v in vertices {
        write_group_code(writer, 0, "VERTEX")?;
        write_group_code(writer, 8, layer)?;
        write_group_code_float(writer, 10, v.x)?;
        write_group_code_float(writer, 20, v.y)?;
    } // for each vertex

    // End of vertex sequence.
    write_group_code(writer, 0, "SEQEND")?;

    Ok(())
} // fn encode_clo_polyline

// @brief Encode a generic entity to DXF format.
// @param writer The writer to write to.
// @param entity The entity to encode (as Box<dyn Entity>).
// @return Result indicating success or error.
// @note This function uses downcasting via Any trait to determine the concrete entity type.
pub fn encode_entity(writer: &mut dyn Write, entity: &Box<dyn Entity>) -> std::io::Result<()> {
    // Get the entity as Any for downcasting.
    let entity_any = entity.as_ref() as &dyn Any;

    // Try to downcast to each entity type based on entity_type().
    match entity.entity_type() {
        "LINE" => {
            if let Some(line) = entity_any.downcast_ref::<Line>() {
                encode_line(writer, line)
            } else {
                Err(std::io::Error::new(
                    std::io::ErrorKind::InvalidData,
                    "Failed to downcast entity to LINE",
                ))
            }
        }
        "CIRCLE" => {
            if let Some(circle) = entity_any.downcast_ref::<Circle>() {
                encode_circle(writer, circle)
            } else {
                Err(std::io::Error::new(
                    std::io::ErrorKind::InvalidData,
                    "Failed to downcast entity to CIRCLE",
                ))
            }
        }
        "POLYLINE" => {
            if let Some(polyline) = entity_any.downcast_ref::<Polyline>() {
                encode_polyline(writer, polyline)
            } else {
                Err(std::io::Error::new(
                    std::io::ErrorKind::InvalidData,
                    "Failed to downcast entity to POLYLINE",
                ))
            }
        }
        "TEXT" => {
            if let Some(text) = entity_any.downcast_ref::<Text>() {
                encode_text(writer, text)
            } else {
                Err(std::io::Error::new(
                    std::io::ErrorKind::InvalidData,
                    "Failed to downcast entity to TEXT",
                ))
            }
        }
        "POINT" => {
            if let Some(point) = entity_any.downcast_ref::<DxfPoint>() {
                encode_dxf_point(writer, point)
            } else {
                Err(std::io::Error::new(
                    std::io::ErrorKind::InvalidData,
                    "Failed to downcast entity to POINT",
                ))
            }
        }
        _ => Err(std::io::Error::new(
            std::io::ErrorKind::Unsupported,
            format!("Unsupported entity type: {}", entity.entity_type()),
        )),
    }
}
