// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief DXF-ASTM file writer (ASTM D6673-10, METRIC units, DXF R12 syntax).
//!
//! File layout:
//! - HEADER: `$ACADVER` only (D6673 §4.2 recommends a minimal header).
//! - BLOCKS: one block per piece; see `write_astm_block`.
//! - ENTITIES: style system text on layer 1, then one INSERT per block.

use crate::encoder::{encode_astm_polyline, encode_astm_text, encode_dxf_point, encode_entity, encode_notch};
use crate::error::{DxfAstmExportError, Result};
use crate::validator::validate_astm_compliance;
use seamly_svg2ezdxf::{AstmContour, Block, DxfPoint, Point, CURVE_TOLERANCE_MM};
use std::fs::{File, read_to_string};
use std::io::Write;
use std::path::Path;
use std::sync::Arc;

/// @brief `ASTM/D13Proposal 1 Version:` value written as style system text.
pub const ASTM_VERSION: &str = "D6673-10";

// Height of system text, in millimetres.
const SYSTEM_TEXT_HEIGHT_MM: f64 = 5.0;

// @brief Progress callback for teaching version generation (0.0 - 1.0).
pub type ProgressCallback = Arc<dyn Fn(f32) + Send + Sync>;

// @brief Export options for DXF-ASTM.
#[derive(Clone)]
pub struct DxfAstmExportOptions {
    // Whether to include HEADER section (empty if true).
    pub include_header: bool,
    // Whether to validate entities before export.
    pub validate_entities: bool,
    // Whether to sanitize text to ASCII-only.
    pub sanitize_text: bool,
    // Whether to create a teaching version with inline comments.
    pub create_teaching_version: bool,
    // Optional progress callback for teaching version generation.
    pub progress_callback: Option<ProgressCallback>,
    // `Style Name:` when the drawing has no pattern name (e.g. the file stem).
    pub style_name: Option<String>,
    // Release number in `Author:` (SeamlyLayout version).
    pub author_release: String,
    // `Creation Date:` as dd-mm-yyyy; None uses today's UTC date.
    pub creation_date: Option<String>,
    // `Creation Time:` as hh-mm; None uses the current UTC time.
    pub creation_time: Option<String>,
    // CLO3D variant: group 250 on boundary (0) and sew line (2) polylines.
    pub clo3d_group_250: bool,
}

impl Default for DxfAstmExportOptions {
    fn default() -> Self {
        Self {
            include_header: false,
            validate_entities: true,
            sanitize_text: true,
            create_teaching_version: false,
            progress_callback: None,
            style_name: None,
            author_release: env!("CARGO_PKG_VERSION").to_string(),
            creation_date: None,
            creation_time: None,
            clo3d_group_250: false,
        }
    }
}

impl std::fmt::Debug for DxfAstmExportOptions {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("DxfAstmExportOptions")
            .field("include_header", &self.include_header)
            .field("validate_entities", &self.validate_entities)
            .field("sanitize_text", &self.sanitize_text)
            .field("create_teaching_version", &self.create_teaching_version)
            .field("progress_callback", &self.progress_callback.is_some())
            .field("style_name", &self.style_name)
            .field("author_release", &self.author_release)
            .field("creation_date", &self.creation_date)
            .field("creation_time", &self.creation_time)
            .field("clo3d_group_250", &self.clo3d_group_250)
            .finish()
    }
}

// @brief Write a group code and value to the DXF file.
fn write_group_code(writer: &mut dyn Write, code: i32, value: &str) -> std::io::Result<()> {
    writeln!(writer, "{}", code)?;
    writeln!(writer, "{}", value)?;
    Ok(())
}

// @brief Write a group code with integer value.
fn write_group_code_int(writer: &mut dyn Write, code: i32, value: i32) -> std::io::Result<()> {
    writeln!(writer, "{}", code)?;
    writeln!(writer, "{}", value)?;
    Ok(())
}

// @brief Write a group code with float value, in millimetres to two places.
fn write_group_code_float(writer: &mut dyn Write, code: i32, value: f64) -> std::io::Result<()> {
    writeln!(writer, "{}", code)?;
    writeln!(writer, "{:.2}", value)?;
    Ok(())
}

// @brief Write DXF HEADER section.
// @param include_header Reserved for more header variables; `$ACADVER` is always written.
fn write_header_section(writer: &mut dyn Write, include_header: bool) -> std::io::Result<()> {
    write_group_code(writer, 0, "SECTION")?;
    write_group_code(writer, 2, "HEADER")?;
    write_group_code(writer, 9, "$ACADVER")?;
    write_group_code(writer, 1, "AC1009")?; // DXF R12 syntax, the widest importer support
    if include_header {
        // (reserved for future HEADER variables)
    } // if include_header
    write_group_code(writer, 0, "ENDSEC")?;
    Ok(())
}

// @brief Today's UTC date and time as (dd-mm-yyyy, hh-mm).
fn utc_date_time() -> (String, String) {
    let secs = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .map(|d| d.as_secs() as i64)
        .unwrap_or(0);
    let (days, rem) = (secs.div_euclid(86_400), secs.rem_euclid(86_400));
    // Civil-from-days (Howard Hinnant), proleptic Gregorian calendar.
    let z = days + 719_468;
    let era = z.div_euclid(146_097);
    let doe = z - era * 146_097;
    let yoe = (doe - doe / 1460 + doe / 36_524 - doe / 146_096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let day = doy - (153 * mp + 2) / 5 + 1;
    let month = if mp < 10 { mp + 3 } else { mp - 9 };
    let year = yoe + era * 400 + if month <= 2 { 1 } else { 0 };
    (
        format!("{:02}-{:02}-{:04}", day, month, year),
        format!("{:02}-{:02}", rem / 3600, (rem % 3600) / 60),
    )
} // fn utc_date_time

/// @brief Style system text lines (D6673 §4.3.1.1), in writing order.
/// @details Sample Size and Grade Rule Table stay blank: SeamlyLayout exports
///          one ungraded size, and the standard requires the identifiers anyway.
pub fn style_system_text(drawing: &seamly_svg2ezdxf::Drawing, options: &DxfAstmExportOptions) -> Vec<String> {
    let style_name = drawing
        .style_name
        .clone()
        .or_else(|| options.style_name.clone())
        .map(|s| seamly_svg2ezdxf::sanitize_ascii(&s).trim().to_string())
        .filter(|s| !s.is_empty())
        .unwrap_or_else(|| "Untitled".to_string());
    let (utc_date, utc_time) = utc_date_time();
    vec![
        format!("Style Name:{}", style_name),
        format!("Creation Date:{}", options.creation_date.clone().unwrap_or(utc_date)),
        format!("Creation Time:{}", options.creation_time.clone().unwrap_or(utc_time)),
        format!("Author:Seamly2D Project;SeamlyLayout;{}", options.author_release),
        "Sample Size:".to_string(),
        "Grade Rule Table:".to_string(),
        "Units:METRIC".to_string(),
        format!("Curve Tolerance:{:.2}", CURVE_TOLERANCE_MM),
        format!("ASTM/D13Proposal 1 Version:{}", ASTM_VERSION),
    ]
} // fn style_system_text

// @brief Write one contour: key points on `layer`, dense polyline on `validation_layer`.
// @param group_250 CLO3D line type for the key-point polyline only; None omits it.
fn write_contour(
    writer: &mut dyn Write,
    contour: &AstmContour,
    layer: &str,
    validation_layer: &str,
    group_250: Option<i32>,
) -> std::io::Result<()> {
    encode_astm_polyline(writer, &contour.reduced, layer, contour.closed, group_250)?;
    encode_astm_polyline(writer, &contour.dense, validation_layer, contour.closed, None)
} // fn write_contour

// @brief Write one ASTM piece block body (between BLOCK and ENDBLK).
//
// Order: piece system text, boundary (1/84), sew lines (14/87), internal
// lines (8/85), cutouts (11/86), turn/curve points (2/3), grainline (7),
// notches (4/80/81/83), annotation text (15).
// @param clo3d When true, the boundary and sew line polylines carry CLO3D group 250.
fn write_astm_block(writer: &mut dyn Write, block: &Block, boundary: &AstmContour, clo3d: bool) -> std::io::Result<()> {
    // Piece system text sits at the first boundary vertex.
    let anchor = boundary.reduced[0];
    let mut system_lines = vec![format!("Piece Name:{}", block.piece_name)];
    if let Some(q) = &block.quantity {
        system_lines.push(format!("Quantity:{}", q));
    } // if quantity
    for (i, line) in system_lines.iter().enumerate() {
        let position = Point::new(anchor.x, anchor.y - (i as f64 + 1.0) * SYSTEM_TEXT_HEIGHT_MM * 1.5);
        encode_astm_text(writer, "1", position, SYSTEM_TEXT_HEIGHT_MM, 0.0, line)?;
    } // for each system line

    // CLO3D line types: 0 = boundary, 2 = sewing line (seamly2clo.py convention).
    let (boundary_250, sew_250) = if clo3d { (Some(0), Some(2)) } else { (None, None) };
    write_contour(writer, boundary, "1", "84", boundary_250)?;
    for c in &block.sew_lines {
        write_contour(writer, c, "14", "87", sew_250)?;
    } // for each sew line
    for c in &block.internal_lines {
        write_contour(writer, c, "8", "85", None)?;
    } // for each internal line
    for c in &block.cutouts {
        write_contour(writer, c, "11", "86", None)?;
    } // for each cutout

    // Layers 2 and 3 hold the turn and curve points of layers 1, 8, 11 and 14.
    let contours = std::iter::once(boundary)
        .chain(&block.sew_lines)
        .chain(&block.internal_lines)
        .chain(&block.cutouts);
    for contour in contours {
        for (p, &turn) in contour.reduced.iter().zip(&contour.turn) {
            encode_dxf_point(writer, &DxfPoint::new(if turn { "2" } else { "3" }, *p))?;
        } // for each key point
    } // for each contour

    if let Some((p1, p2)) = &block.grainline {
        write_group_code(writer, 0, "LINE")?;
        write_group_code(writer, 8, "7")?;
        write_group_code_float(writer, 10, p1.x)?;
        write_group_code_float(writer, 20, p1.y)?;
        write_group_code_float(writer, 11, p2.x)?;
        write_group_code_float(writer, 21, p2.y)?;
    } // if grainline

    for notch in &block.notches {
        encode_notch(writer, notch)?;
    } // for each notch

    for a in &block.annotations {
        encode_astm_text(writer, "15", a.position, a.height, a.rotation, &a.text)?;
    } // for each annotation
    Ok(())
} // fn write_astm_block

// @brief Write DXF BLOCKS section, one block per pattern piece.
// @details A block without a recognised boundary (an untagged SVG group with
//          no closed outline) is written from its generic entities.
fn write_blocks_section(
    writer: &mut dyn Write,
    drawing: &seamly_svg2ezdxf::Drawing,
    options: &DxfAstmExportOptions,
) -> std::io::Result<()> {
    write_group_code(writer, 0, "SECTION")?;
    write_group_code(writer, 2, "BLOCKS")?;

    for block in &drawing.blocks {
        write_group_code(writer, 0, "BLOCK")?;
        write_group_code(writer, 8, "1")?;
        write_group_code(writer, 2, &block.name)?;
        write_group_code_int(writer, 70, 0)?; // no block flags
        write_group_code_float(writer, 10, 0.0)?; // base point
        write_group_code_float(writer, 20, 0.0)?;
        write_group_code(writer, 3, &block.name)?;

        match &block.boundary {
            Some(boundary) => write_astm_block(writer, block, boundary, options.clo3d_group_250)?,
            None => {
                encode_astm_text(writer, "1", Point::new(0.0, 0.0), SYSTEM_TEXT_HEIGHT_MM, 0.0, &format!("Piece Name:{}", block.piece_name))?;
                for entity in &block.entities {
                    encode_entity(writer, entity)?;
                } // for each entity
            } // None
        } // match boundary

        write_group_code(writer, 0, "ENDBLK")?;
    } // for each block

    write_group_code(writer, 0, "ENDSEC")?;
    Ok(())
} // fn write_blocks_section

// @brief Write an INSERT entity placing a block in modelspace at (x, y).
fn write_insert_entity(
    writer: &mut dyn Write,
    block_name: &str,
    x: f64,
    y: f64,
) -> std::io::Result<()> {
    write_group_code(writer, 0, "INSERT")?;
    write_group_code(writer, 8, "1")?;
    write_group_code(writer, 2, block_name)?;
    write_group_code_float(writer, 10, x)?;
    write_group_code_float(writer, 20, y)?;
    Ok(())
} // fn write_insert_entity

// @brief Write DXF ENTITIES section: style system text, modelspace entities, block INSERTs.
fn write_entities_section(
    writer: &mut dyn Write,
    drawing: &seamly_svg2ezdxf::Drawing,
    options: &DxfAstmExportOptions,
) -> std::io::Result<()> {
    write_group_code(writer, 0, "SECTION")?;
    write_group_code(writer, 2, "ENTITIES")?;

    // Style system text occurs once, on layer 1, stacked below the origin.
    for (i, line) in style_system_text(drawing, options).iter().enumerate() {
        let position = Point::new(0.0, -(i as f64 + 1.0) * SYSTEM_TEXT_HEIGHT_MM * 1.5);
        encode_astm_text(writer, "1", position, SYSTEM_TEXT_HEIGHT_MM, 0.0, line)?;
    } // for each style line

    for entity in &drawing.modelspace_entities {
        encode_entity(writer, entity)?;
    } // for each modelspace entity

    // Block coordinates are already absolute, so every INSERT is at the origin.
    for block in &drawing.blocks {
        write_insert_entity(writer, &block.name, 0.0, 0.0)?;
    } // for each block

    write_group_code(writer, 0, "ENDSEC")?;
    Ok(())
} // fn write_entities_section

// @brief Write DXF EOF marker.
// @param writer The writer to write to.
// @return Result indicating success or error.
fn write_eof(writer: &mut dyn Write) -> std::io::Result<()> {
    write_group_code(writer, 0, "EOF")?;
    Ok(())
}

// @brief Get comment for a DXF line based on context.
// @param line The current line.
// @param prev_line The previous line (for context).
// @param next_line The next line (for context, if available).
// @return Comment string explaining the line.
fn get_line_comment(line: &str, prev_line: &str, next_line: Option<&str>) -> String {
    let line_trimmed = line.trim();

    // Group code 0 - entity or section marker.
    if prev_line.trim() == "0" {
        match line_trimmed {
            "SECTION" => "Entity type: SECTION (marks the beginning of a section)".to_string(),
            "ENDSEC" => "Entity type: ENDSEC (marks the end of a section)".to_string(),
            "TABLE" => "Entity type: TABLE (marks the beginning of a table)".to_string(),
            "ENDTAB" => "Entity type: ENDTAB (marks the end of a table)".to_string(),
            "BLOCK" => "Entity type: BLOCK (defines a reusable block/pattern piece)".to_string(),
            "ENDBLK" => "Entity type: ENDBLK (marks the end of the BLOCK definition)".to_string(),
            "LINE" => "Entity type: LINE (line entity)".to_string(),
            "CIRCLE" => "Entity type: CIRCLE (circle entity)".to_string(),
            "ARC" => "Entity type: ARC (arc entity)".to_string(),
            "POLYLINE" => "Entity type: POLYLINE (polyline entity)".to_string(),
            "VERTEX" => "Entity type: VERTEX (vertex point of the polyline)".to_string(),
            "SEQEND" => "Entity type: SEQEND (marks the end of the vertex sequence)".to_string(),
            "TEXT" => "Entity type: TEXT (text annotation entity)".to_string(),
            "INSERT" => "Entity type: INSERT (inserts a block into modelspace)".to_string(),
            "EOF" => "End of File marker (marks the end of the DXF file)".to_string(),
            "0" => "Group code 0: End marker (end of entity or section)".to_string(),
            _ => format!("Entity type: {}", line_trimmed),
        }
    }
    // Group code 2 - section name, table name, block name, etc.
    else if prev_line.trim() == "2" {
        match line_trimmed {
            "HEADER" => "Section name: HEADER (contains drawing variables)".to_string(),
            "TABLES" => "Section name: TABLES (contains layer, linetype, style tables)".to_string(),
            "BLOCKS" => {
                "Section name: BLOCKS (contains block definitions for pattern pieces)".to_string()
            }
            "ENTITIES" => "Section name: ENTITIES (contains modelspace entities)".to_string(),
            "LAYER" => "Table name: LAYER (this is the layer table)".to_string(),
            _ => format!("Name: {} (section/table/block name)", line_trimmed),
        }
    }
    // Group code 8 - layer name.
    else if prev_line.trim() == "8" {
        format!("Layer name: {} ({})", line_trimmed, astm_layer_meaning(line_trimmed))
    }
    // Group code 10 - X coordinate.
    else if prev_line.trim() == "10" {
        format!(
            "Value: X = {} (X coordinate in millimetres)",
            line_trimmed
        )
    }
    // Group code 20 - Y coordinate.
    else if prev_line.trim() == "20" {
        format!(
            "Value: Y = {} (Y coordinate in millimetres)",
            line_trimmed
        )
    }
    // Group code 11 - End point X coordinate.
    else if prev_line.trim() == "11" {
        format!("Value: X = {} (end point X coordinate)", line_trimmed)
    }
    // Group code 21 - End point Y coordinate.
    else if prev_line.trim() == "21" {
        format!("Value: Y = {} (end point Y coordinate)", line_trimmed)
    }
    // Group code 30 - Z value; D6673 uses it for notch depth.
    else if prev_line.trim() == "30" {
        format!("Value: {} (notch depth in millimetres)", line_trimmed)
    }
    // Group code 39 - thickness; D6673 uses it for notch width.
    else if prev_line.trim() == "39" {
        format!("Value: {} (notch width in millimetres)", line_trimmed)
    }
    // Group code 250 - CLO3D line type (not DXF R12 or D6673).
    else if prev_line.trim() == "250" {
        format!("Value: {} (CLO3D line type: 0 = boundary, 2 = sewing line)", line_trimmed)
    }
    // Group code 66 - vertices-follow flag.
    else if prev_line.trim() == "66" {
        format!("Value: {} (1 = VERTEX entities follow)", line_trimmed)
    }
    // Group code 3 - block name (repeat).
    else if prev_line.trim() == "3" {
        format!("Block name: {} (repeat of group 2)", line_trimmed)
    }
    // Group code 40 - Text height, circle radius, etc.
    else if prev_line.trim() == "40" {
        format!(
            "Value: {} (text height, radius, or other size value)",
            line_trimmed
        )
    }
    // Group code 41 - X scale factor.
    else if prev_line.trim() == "41" {
        format!("Value: {} (X scale factor for INSERT entity)", line_trimmed)
    }
    // Group code 42 - Y scale factor.
    else if prev_line.trim() == "42" {
        format!("Value: {} (Y scale factor for INSERT entity)", line_trimmed)
    }
    // Group code 50 - Rotation angle.
    else if prev_line.trim() == "50" {
        format!("Value: {} (rotation angle in degrees)", line_trimmed)
    }
    // Group code 62 - Color number.
    else if prev_line.trim() == "62" {
        let color_name = match line_trimmed {
            "1" => "Red",
            "2" => "Yellow",
            "3" => "Green",
            "4" => "Cyan",
            "5" => "Blue",
            "6" => "Magenta",
            "7" => "White/Black",
            _ => "Unknown",
        };
        format!("Value: {} = {} (color number)", line_trimmed, color_name)
    }
    // Group code 70 - Flags, counts, etc.
    else if prev_line.trim() == "70" {
        if let Some(next) = next_line {
            if next.trim() == "LAYER" || next.trim().starts_with("LAYER") {
                format!("Value: {} (number of layers in table)", line_trimmed)
            } else {
                format!(
                    "Value: {} (flags or count - 0 = normal, 1 = closed/other flags)",
                    line_trimmed
                )
            }
        } else {
            format!("Value: {} (flags or count)", line_trimmed)
        }
    }
    // Group code 1 - Text content.
    else if prev_line.trim() == "1" {
        format!("Text content: {}", line_trimmed)
    }
    // Group code 6 - Linetype name.
    else if prev_line.trim() == "6" {
        format!("Linetype: {} (line style)", line_trimmed)
    }
    // Group code 9 - Variable name (in HEADER).
    else if prev_line.trim() == "9" {
        format!("Variable name: {} (header variable)", line_trimmed)
    }
    // Numeric group codes (0, 2, 8, 9, 10, 11, 20, 21, 40, 41, 42, 50, 62, 70, etc.).
    else if line_trimmed.parse::<i32>().is_ok() {
        match line_trimmed {
            "0" => "Group code 0: Start of entity or section marker".to_string(),
            "2" => "Group code 2: Section/table/block name, or entity name follows".to_string(),
            "8" => "Group code 8: Layer name follows".to_string(),
            "9" => "Group code 9: Variable name follows (header variable)".to_string(),
            "10" => "Group code 10: X coordinate, insertion point X, or start point X follows"
                .to_string(),
            "11" => "Group code 11: End point X coordinate follows".to_string(),
            "20" => "Group code 20: Y coordinate, insertion point Y, or start point Y follows"
                .to_string(),
            "21" => "Group code 21: End point Y coordinate follows".to_string(),
            "40" => "Group code 40: Text height, radius, or other size value follows".to_string(),
            "3" => "Group code 3: Block name follows (repeat)".to_string(),
            "30" => "Group code 30: Notch depth follows".to_string(),
            "39" => "Group code 39: Notch width follows".to_string(),
            "66" => "Group code 66: Vertices-follow flag follows".to_string(),
            "250" => "Group code 250: CLO3D line type follows".to_string(),
            "41" => "Group code 41: X scale factor follows".to_string(),
            "42" => "Group code 42: Y scale factor follows".to_string(),
            "50" => "Group code 50: Rotation angle in degrees follows".to_string(),
            "62" => "Group code 62: Color number follows".to_string(),
            "70" => "Group code 70: Flags, counts, or integer value follows".to_string(),
            "1" => "Group code 1: Text content or string value follows".to_string(),
            "6" => "Group code 6: Linetype name follows".to_string(),
            _ => format!("Group code {}: (numeric group code)", line_trimmed),
        }
    }
    // Empty line or other content.
    else if line_trimmed.is_empty() {
        "".to_string()
    }
    // Default: try to provide context.
    else {
        format!("Value: {} (data value)", line_trimmed)
    }
}

// @brief Meaning of an ASTM D6673 layer number, for teaching comments.
fn astm_layer_meaning(layer: &str) -> &'static str {
    match layer {
        "1" => "ASTM piece boundary and system text",
        "2" => "ASTM turn point",
        "3" => "ASTM curve point",
        "4" => "ASTM slit or V notch",
        "7" => "ASTM grainline",
        "8" => "ASTM internal line",
        "11" => "ASTM internal cutout",
        "14" => "ASTM sew line",
        "15" => "ASTM annotation text",
        "80" => "ASTM T-notch",
        "81" => "ASTM castle notch",
        "83" => "ASTM U-notch",
        "84" => "ASTM boundary quality validation curve",
        "85" => "ASTM internal line quality validation curve",
        "86" => "ASTM internal cutout quality validation curve",
        "87" => "ASTM sew line quality validation curve",
        _ => "layer for this entity",
    } // match layer
} // fn astm_layer_meaning

// @brief Create a teaching version of a DXF file with inline comments.
// @param dxf_path Path to the DXF file.
// @return Result indicating success or error.
fn create_teaching_version(
    dxf_path: &Path,
    progress_callback: Option<&ProgressCallback>,
) -> std::io::Result<()> {
    // Read the DXF file.
    let content = read_to_string(dxf_path)?;
    let lines: Vec<&str> = content.lines().collect();
    let total = lines.len().max(1);

    // Create teaching version path (same directory, .txt extension).
    let mut teaching_path = dxf_path.to_path_buf();
    teaching_path.set_extension("txt");

    // Create teaching version file.
    let mut teaching_file = File::create(&teaching_path)?;

    // Write header comment.
    writeln!(
        teaching_file,
        "// DXF-ASTM Teaching Version with Inline Comments"
    )?;
    writeln!(
        teaching_file,
        "// Generated automatically from: {}",
        dxf_path
            .file_name()
            .and_then(|s| s.to_str())
            .unwrap_or("unknown.dxf")
    )?;
    writeln!(
        teaching_file,
        "// This file contains the DXF content with explanatory comments for each line."
    )?;
    writeln!(
        teaching_file,
        "// Comments are positioned two tabs to the right of the DXF data."
    )?;
    writeln!(teaching_file, "//")?;
    writeln!(teaching_file)?;

    // Process each line and add comments.
    for (i, line) in lines.iter().enumerate() {
        let prev_line = if i > 0 { lines[i - 1] } else { "" };
        let next_line = if i + 1 < lines.len() {
            Some(lines[i + 1])
        } else {
            None
        };

        let comment = get_line_comment(line, prev_line, next_line);

        if comment.is_empty() {
            // Empty line - just write it.
            writeln!(teaching_file, "{}", line)?;
        } else {
            // Write line with comment (two tabs distance).
            writeln!(teaching_file, "{}\t\t// {}", line, comment)?;
        }
        if let Some(callback) = progress_callback {
            let progress = (i + 1) as f32 / total as f32;
            callback(progress);
        }
    }

    Ok(())
}

// @brief Export Drawing to DXF-ASTM format.
// @param drawing The ezdxf Drawing object to export.
// @param output_path Path to write the DXF file.
// @param options Export options.
// @return Result indicating success or error.
pub fn export_dxf_astm(
    drawing: &seamly_svg2ezdxf::Drawing,
    output_path: impl AsRef<std::path::Path>,
    options: &DxfAstmExportOptions,
) -> Result<()> {
    // Validate DXF version (must be R12 for ASTM).
    if drawing.version != seamly_svg2ezdxf::DxfVersion::R12 {
        return Err(DxfAstmExportError::InvalidVersion(format!(
            "DXF version must be R12 for ASTM-D6673-10, got: {:?}",
            drawing.version
        )));
    }

    // Validate entities if requested.
    if options.validate_entities {
        if let Err(errors) = validate_astm_compliance(drawing) {
            let error_messages: Vec<String> = errors.iter().map(|e| e.message.clone()).collect();
            return Err(DxfAstmExportError::Validation(format!(
                "ASTM validation failed: {}",
                error_messages.join("; ")
            )));
        }
    }

    // Create output file.
    let mut file = File::create(output_path.as_ref()).map_err(|e| DxfAstmExportError::Io(e))?;

    // Write DXF file structure.
    // 1. HEADER section (minimal or empty).
    write_header_section(&mut file, options.include_header)
        .map_err(|e| DxfAstmExportError::Io(e))?;

    // 2. BLOCKS section (pattern pieces).
    write_blocks_section(&mut file, drawing, options).map_err(|e| DxfAstmExportError::Io(e))?;

    // 3. ENTITIES section (modelspace entities).
    write_entities_section(&mut file, drawing, options).map_err(|e| DxfAstmExportError::Io(e))?;

    // 4. EOF marker.
    write_eof(&mut file).map_err(|e| DxfAstmExportError::Io(e))?;

    // 5. Create teaching version with inline comments (if requested).
    if options.create_teaching_version {
        create_teaching_version(output_path.as_ref(), options.progress_callback.as_ref())
            .map_err(|e| DxfAstmExportError::Io(e))?;
    }

    Ok(())
}
