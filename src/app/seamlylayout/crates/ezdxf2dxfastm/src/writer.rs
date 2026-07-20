// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief DXF-ASTM file writer.

use crate::encoder::{encode_clo_polyline, encode_dxf_point, encode_entity};
use crate::error::{DxfAstmExportError, Result};
use crate::validator::validate_astm_compliance;
use seamly_svg2ezdxf::{DxfPoint, Point};
use std::fs::{File, read_to_string};
use std::io::Write;
use std::path::Path;
use std::sync::Arc;

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
}

impl Default for DxfAstmExportOptions {
    fn default() -> Self {
        Self {
            include_header: false,
            validate_entities: true,
            sanitize_text: true,
            create_teaching_version: false,
            progress_callback: None,
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
            .finish()
    }
}

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

// @brief Write DXF HEADER section.
// @param writer The writer to write to.
// @param include_header Whether to include header (if false, writes minimal header).
// @return Result indicating success or error.
fn write_header_section(writer: &mut dyn Write, include_header: bool) -> std::io::Result<()> {
    // Section start.
    write_group_code(writer, 0, "SECTION")?;
    write_group_code(writer, 2, "HEADER")?;

    // Always write $ACADVER = AC1009 (DXF R12), matching seamly2clo.py HEADER section.
    write_group_code(writer, 9, "$ACADVER")?;
    write_group_code(writer, 1, "AC1009")?; // DXF R12

    // Include additional header variables only when explicitly requested.
    if include_header {
        // (reserved for future HEADER variables)
    } // if include_header

    // Section end.
    write_group_code(writer, 0, "ENDSEC")?;
    Ok(())
}

// @brief Write a set of POINT entities for each boundary vertex (layers 2 and 3).
//
// Layer "2" = turn point (corner), layer "3" = curve point.
// Called twice per block — once before the boundary POLYLINE, once after —
// matching the structure produced by seamly2clo.py.
//
// @param writer       The writer to write to.
// @param vertices     Boundary polyline vertices.
// @param corner_flags Per-vertex classification: true = corner (layer 2).
// @return Result indicating success or error.
fn write_point_annotations(
    writer: &mut dyn Write,
    vertices: &[Point],
    corner_flags: &[bool],
) -> std::io::Result<()> {
    for (i, v) in vertices.iter().enumerate() {
        // Default to curve point (layer 3) when corner_flags is shorter than expected.
        let is_corner = corner_flags.get(i).copied().unwrap_or(false);
        let layer = if is_corner { "2" } else { "3" };
        let pt = DxfPoint::new(layer, *v);
        encode_dxf_point(writer, &pt)?;
    } // for each vertex
    Ok(())
} // fn write_point_annotations

// @brief Write DXF BLOCKS section in CLO3D-compatible format.
//
// Each block is written with the exact structure produced by seamly2clo.py:
//   BLOCK header  (layer 1, flags 70=64)
//   POLYLINE layer 14 (sewing line, 250=2) + VERTEXes + SEQEND
//   POINT layer 2/3  (first set — turn/curve annotations)
//   LINE layer 7     (grainline, if present)
//   LINE layer 4     (notches, zero or more)
//   POLYLINE layer 1 (boundary, 250=0) + VERTEXes + SEQEND
//   POINT layer 2/3  (second set — identical to first)
//   ENDBLK
//
// @param writer The writer to write to.
// @param drawing The drawing containing blocks to write.
// @return Result indicating success or error.
fn write_blocks_section(
    writer: &mut dyn Write,
    drawing: &seamly_svg2ezdxf::Drawing,
) -> std::io::Result<()> {
    // Section start.
    write_group_code(writer, 0, "SECTION")?;
    write_group_code(writer, 2, "BLOCKS")?;

    // Write each block.
    for block in &drawing.blocks {
        // --- BLOCK header ---
        // Order: entity type / layer / name / flags / base-x / base-y
        write_group_code(writer, 0, "BLOCK")?;
        write_group_code(writer, 8, "1")?;           // layer 1 (boundary layer)
        write_group_code(writer, 2, &block.name)?;   // block name (e.g. "front_M")
        write_group_code_int(writer, 70, 64)?;        // flags: 64 = anonymous block
        write_group_code_float(writer, 10, 0.0)?;     // base point X
        write_group_code_float(writer, 20, 0.0)?;     // base point Y

        if !block.boundary_vertices.is_empty() {
            // --- Sewing-line POLYLINE (layer 14, group 250=2) ---
            encode_clo_polyline(writer, &block.boundary_vertices, "14", 2)?;

            // --- POINT annotations — first set (layers 2 and 3) ---
            write_point_annotations(writer, &block.boundary_vertices, &block.corner_flags)?;

            // --- Grainline LINE (layer 7) ---
            if let Some((p1, p2)) = &block.grainline {
                write_group_code(writer, 0, "LINE")?;
                write_group_code(writer, 8, "7")?;
                write_group_code_float(writer, 10, p1.x)?;
                write_group_code_float(writer, 20, p1.y)?;
                write_group_code_float(writer, 11, p2.x)?;
                write_group_code_float(writer, 21, p2.y)?;
            } // if grainline

            // --- Notch LINEs (layer 4) ---
            for (p1, p2) in &block.notches {
                write_group_code(writer, 0, "LINE")?;
                write_group_code(writer, 8, "4")?;
                write_group_code_float(writer, 10, p1.x)?;
                write_group_code_float(writer, 20, p1.y)?;
                write_group_code_float(writer, 11, p2.x)?;
                write_group_code_float(writer, 21, p2.y)?;
            } // for each notch

            // --- Boundary POLYLINE (layer 1, group 250=0) ---
            encode_clo_polyline(writer, &block.boundary_vertices, "1", 0)?;

            // --- POINT annotations — second set (same as first) ---
            write_point_annotations(writer, &block.boundary_vertices, &block.corner_flags)?;
        } else {
            // Fallback: no boundary polygon extracted — write raw entities.
            for entity in &block.entities {
                encode_entity(writer, entity)?;
            } // for each entity
        } // if boundary vertices

        // --- ENDBLK ---
        write_group_code(writer, 0, "ENDBLK")?;
    } // for each block

    // Section end.
    write_group_code(writer, 0, "ENDSEC")?;
    Ok(())
} // fn write_blocks_section

// @brief Write an INSERT entity to insert a block into modelspace.
//
// Matches the minimal INSERT format from seamly2clo.py:
//   INSERT / layer 1 / block name / x=0.0 / y=0.0
// (No scale or rotation group codes — these are omitted for CLO3D compatibility.)
//
// @param writer The writer to write to.
// @param block_name The name of the block to insert.
// @param x Insertion point X coordinate.
// @param y Insertion point Y coordinate.
// @return Result indicating success or error.
fn write_insert_entity(
    writer: &mut dyn Write,
    block_name: &str,
    x: f64,
    y: f64,
) -> std::io::Result<()> {
    // Entity type.
    write_group_code(writer, 0, "INSERT")?;

    // Layer (group code 8) — always layer 1 for pattern piece inserts.
    write_group_code(writer, 8, "1")?;

    // Block name (group code 2).
    write_group_code(writer, 2, block_name)?;

    // Insertion point (group codes 10, 20 for X, Y).
    write_group_code_float(writer, 10, x)?;
    write_group_code_float(writer, 20, y)?;

    Ok(())
} // fn write_insert_entity

// @brief Write DXF ENTITIES section.
// @param writer The writer to write to.
// @param drawing The drawing containing entities to write.
// @return Result indicating success or error.
fn write_entities_section(
    writer: &mut dyn Write,
    drawing: &seamly_svg2ezdxf::Drawing,
) -> std::io::Result<()> {
    // Section start.
    write_group_code(writer, 0, "SECTION")?;
    write_group_code(writer, 2, "ENTITIES")?;

    // Write modelspace entities (entities not in blocks).
    for entity in &drawing.modelspace_entities {
        encode_entity(writer, entity)?;
    }

    // Insert all blocks at origin (0, 0) — matches seamly2clo.py ENTITIES section.
    for block in &drawing.blocks {
        write_insert_entity(writer, &block.name, 0.0, 0.0)?;
    } // for each block

    // Section end.
    write_group_code(writer, 0, "ENDSEC")?;
    Ok(())
}

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
        format!("Layer name: {} (layer for this entity)", line_trimmed)
    }
    // Group code 10 - X coordinate.
    else if prev_line.trim() == "10" {
        format!(
            "Value: X = {} (X coordinate in 1/1000th inch units, divide by 1000 for inches)",
            line_trimmed
        )
    }
    // Group code 20 - Y coordinate.
    else if prev_line.trim() == "20" {
        format!(
            "Value: Y = {} (Y coordinate in 1/1000th inch units, divide by 1000 for inches)",
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
    write_blocks_section(&mut file, drawing).map_err(|e| DxfAstmExportError::Io(e))?;

    // 3. ENTITIES section (modelspace entities).
    write_entities_section(&mut file, drawing).map_err(|e| DxfAstmExportError::Io(e))?;

    // 4. EOF marker.
    write_eof(&mut file).map_err(|e| DxfAstmExportError::Io(e))?;

    // 5. Create teaching version with inline comments (if requested).
    if options.create_teaching_version {
        create_teaching_version(output_path.as_ref(), options.progress_callback.as_ref())
            .map_err(|e| DxfAstmExportError::Io(e))?;
    }

    Ok(())
}
