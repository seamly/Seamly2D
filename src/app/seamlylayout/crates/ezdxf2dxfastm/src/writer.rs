// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief DXF-ASTM file writer (ASTM D6673-10, METRIC units, DXF R11 or R13).
//!
//! AC1009 file layout:
//! - HEADER: `$ACADVER` only (D6673 §4.2 recommends a minimal header).
//! - BLOCKS: one block per piece; see `write_astm_block`.
//! - ENTITIES: style system text on layer 1, then one INSERT per block.
//!
//! `Drawing::version` selects the file version. An R13 file is the AC1009 file
//! rewritten by `r13::upgrade_to_r13`, so both versions carry the same D6673 content.
//!
//! `DxfAstmExportOptions::aama` writes DXF-AAMA (ANSI/AAMA-292) content instead,
//! in the same file layout. AAMA defines layers 1 to 14 only:
//! - each contour is one dense polyline on its own layer, with no validation curve (84 to 87);
//! - a boundary with a mirror line (layer 6) starts with the mirror line's two points;
//! - every notch is on layer 4 (no 80 to 83);
//! - label text is on layer 1 (no 15);
//! - no `Curve Tolerance:` or ASTM version style text; the size is `Size:`, not `Size Name:`.
//!
//! An R11 file is the AC1009 file.

use crate::encoder::{encode_astm_polyline, encode_astm_text, encode_dxf_point, encode_entity, encode_notch};
use crate::error::{DxfAstmExportError, Result};
use crate::grade_rule_file::write_grade_rule_file;
use crate::r13::upgrade_to_r13;
use crate::validator::validate_astm_compliance;
use seamly_svg2ezdxf::{AstmContour, Block, DxfPoint, DxfVersion, Notch, NotchKind, Point, CURVE_TOLERANCE_MM};
use std::fs::{File, read_to_string};
use std::io::Write;
use std::path::Path;
use std::sync::Arc;

/// @brief `ASTM/D13Proposal 1 Version:` value written as style system text.
pub const ASTM_VERSION: &str = "D6673-10";

// Height of system text, in millimetres.
const SYSTEM_TEXT_HEIGHT_MM: f64 = 5.0;
// Height of `# n` grade rule identifier text, in millimetres.
const RULE_TEXT_HEIGHT_MM: f64 = 1.0;

// A dense vertex this close to a key point, in millimetres, is that key point:
// the file writes two decimals.
const MIRROR_VERTEX_TOLERANCE_MM: f64 = 0.01;

// Piece text identifiers of D6673 and ANSI/AAMA-292. A DXF-AAMA label line that
// starts with one of them is not written, so a label cannot override the piece text.
const PIECE_TEXT_IDENTIFIERS: [&str; 11] =
    ["Piece Name", "Size", "Size Name", "Quantity", "Rotation", "Flip", "Tilt", "Fold", "Material", "Category", "Annotation"];

// @brief Progress callback for annotated version generation (0.0 - 1.0).
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
    // Whether to create an annotated version with inline comments.
    pub create_annotated_version: bool,
    // Optional progress callback for annotated version generation.
    pub progress_callback: Option<ProgressCallback>,
    // `Style Name:` when the drawing has no pattern name (e.g. the file stem).
    pub style_name: Option<String>,
    // Seamly2D suite version: the release field of `Author:Seamly;Seamly2D;<release>`.
    pub author_release: String,
    // `Creation Date:` as dd-mm-yyyy; None uses today's UTC date.
    pub creation_date: Option<String>,
    // `Creation Time:` as hh-mm; None uses the current UTC time.
    pub creation_time: Option<String>,
    // CLO3D variant: group 250 on boundary (0) and sew line (2) polylines.
    pub clo3d_group_250: bool,
    // DXF-AAMA (ANSI/AAMA-292) content: layers 1 to 14 only; see the module notes.
    pub aama: bool,
}

impl Default for DxfAstmExportOptions {
    fn default() -> Self {
        Self {
            include_header: false,
            validate_entities: true,
            sanitize_text: true,
            create_annotated_version: false,
            progress_callback: None,
            style_name: None,
            author_release: env!("CARGO_PKG_VERSION").to_string(),
            creation_date: None,
            creation_time: None,
            clo3d_group_250: false,
            aama: false,
        }
    }
}

impl std::fmt::Debug for DxfAstmExportOptions {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("DxfAstmExportOptions")
            .field("include_header", &self.include_header)
            .field("validate_entities", &self.validate_entities)
            .field("sanitize_text", &self.sanitize_text)
            .field("create_annotated_version", &self.create_annotated_version)
            .field("progress_callback", &self.progress_callback.is_some())
            .field("style_name", &self.style_name)
            .field("author_release", &self.author_release)
            .field("creation_date", &self.creation_date)
            .field("creation_time", &self.creation_time)
            .field("clo3d_group_250", &self.clo3d_group_250)
            .field("aama", &self.aama)
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
    write_group_code(writer, 1, "AC1009")?; // DXF R11 syntax, the widest importer support
    if include_header {
        // (reserved for future HEADER variables)
    } // if include_header
    write_group_code(writer, 0, "ENDSEC")?;
    Ok(())
}

// @brief Today's UTC date and time as (dd-mm-yyyy, hh-mm).
pub(crate) fn utc_date_time() -> (String, String) {
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
/// @details Sample Size and Grade Rule Table come from the drawing's grade rule
///          table. An ungraded export leaves them blank: the standard requires
///          the identifiers anyway. DXF-AAMA has no validation curves and no
///          version identifier, so it ends at `Units:`.
pub fn style_system_text(drawing: &seamly_svg2ezdxf::Drawing, options: &DxfAstmExportOptions) -> Vec<String> {
    let style_name = drawing
        .style_name
        .clone()
        .or_else(|| options.style_name.clone())
        .map(|s| seamly_svg2ezdxf::sanitize_ascii(&s).trim().to_string())
        .filter(|s| !s.is_empty())
        .unwrap_or_else(|| "Untitled".to_string());
    let (utc_date, utc_time) = utc_date_time();
    let (sample_size, table_name) = drawing
        .grade_rule_table
        .as_ref()
        .map_or((String::new(), String::new()), |t| (t.sample_size.clone(), t.name.clone()));
    let mut lines = vec![
        format!("Style Name:{}", style_name),
        format!("Creation Date:{}", options.creation_date.clone().unwrap_or(utc_date)),
        format!("Creation Time:{}", options.creation_time.clone().unwrap_or(utc_time)),
        format!("Author:Seamly;Seamly2D;{}", options.author_release),
        format!("Sample Size:{}", sample_size),
        format!("Grade Rule Table:{}", table_name),
        "Units:METRIC".to_string(),
    ];
    if !options.aama {
        lines.push(format!("Curve Tolerance:{:.2}", CURVE_TOLERANCE_MM));
        lines.push(format!("ASTM/D13Proposal 1 Version:{}", ASTM_VERSION));
    } // if ASTM
    lines
} // fn style_system_text

// @brief Write one contour: key points on `layer`, dense polyline on `validation_layer`.
// @param validation_layer None (DXF-AAMA) writes only the dense polyline, on `layer`.
//        Its vertices include every key point, so layers 2 and 3 still sit on the line.
// @param group_250 CLO3D line type for the polyline on `layer` only; None omits it.
fn write_contour(
    writer: &mut dyn Write,
    contour: &AstmContour,
    layer: &str,
    validation_layer: Option<&str>,
    group_250: Option<i32>,
) -> std::io::Result<()> {
    let Some(validation_layer) = validation_layer else {
        return encode_astm_polyline(writer, &contour.dense, layer, contour.closed, group_250);
    }; // if no validation layer
    encode_astm_polyline(writer, &contour.reduced, layer, contour.closed, group_250)?;
    encode_astm_polyline(writer, &contour.dense, validation_layer, contour.closed, None)
} // fn write_contour

// @brief The notch as DXF-AAMA writes it: on layer 4, which holds every AAMA notch.
// @details A T, castle or U notch (D6673 layers 80, 81, 83) becomes a V notch with its width.
fn aama_notch(notch: &Notch) -> Notch {
    let kind = if notch.kind == NotchKind::Slit { NotchKind::Slit } else { NotchKind::V };
    Notch { kind, ..notch.clone() }
} // fn aama_notch

// @brief `boundary` with no dense vertex between its first two key points.
// @details DXF-AAMA writes the dense boundary on layer 1, and the mirror line must
//          be its first two points (D6673 §4.3.6). After `align_mirror_lines` the
//          first two key points are the mirror line ends; the dense vertices between
//          them lie on the straight fold, so dropping them keeps the outline.
fn start_on_mirror_line(boundary: &AstmContour) -> AstmContour {
    let mut trimmed = boundary.clone();
    let second = boundary.reduced[1];
    let at_second = |p: &Point| (p.x - second.x).hypot(p.y - second.y) < MIRROR_VERTEX_TOLERANCE_MM;
    if let Some(i) = boundary.dense.iter().skip(1).position(at_second) {
        trimmed.dense.drain(1..i + 1);
    } // if the second key point is a dense vertex
    trimmed
} // fn start_on_mirror_line

// @brief True when a label line reads as piece text or a grade rule identifier.
// @details DXF-AAMA writes label text on layer 1, beside the piece text: `Size: 40`
//          would be a second `Size:`, and `# 3` a grade rule identifier.
fn repeats_piece_text(text: &str) -> bool {
    let text = text.trim_start();
    if text.starts_with('#') {
        return true;
    } // if rule identifier
    let Some((identifier, _)) = text.split_once(':') else {
        return false;
    }; // if no colon
    PIECE_TEXT_IDENTIFIERS.iter().any(|id| id.eq_ignore_ascii_case(identifier.trim()))
} // fn repeats_piece_text

// @brief Write one ASTM piece block body (between BLOCK and ENDBLK).
//
// Order: piece system text, boundary (1/84), sew lines (14/87), internal
// lines (8/85), cutouts (11/86), turn/curve points (2/3), mirror line (6), grainline (7),
// notches (4/80/81/83), annotation text (15), grade reference line (5),
// grade rule identifiers.
// @param options `clo3d_group_250` marks the boundary and sew line polylines with CLO3D group 250.
//        `aama` writes the DXF-AAMA layers: no 15, 80 to 87; label text on layer 1.
fn write_astm_block(writer: &mut dyn Write, block: &Block, boundary: &AstmContour, options: &DxfAstmExportOptions) -> std::io::Result<()> {
    let aama = options.aama;
    // Piece system text sits at the first boundary vertex.
    let anchor = boundary.reduced[0];
    let mut system_lines = vec![format!("Piece Name:{}", block.piece_name)];
    if let Some(size) = &block.size {
        // D6673 §4.3.1.5.1 names the size `Size Name:`; AAMA readers read `Size:`.
        system_lines.push(format!("{}:{}", if aama { "Size" } else { "Size Name" }, size));
    } // if size
    // No `Category:`: no D6673 edition defines it, and the handoff has no value for it.
    if let Some(q) = &block.quantity {
        system_lines.push(format!("Quantity:{}", q));
    } // if quantity
    if !aama {
        // D6673 §4.3.1.2 layout rules, in its order; a piece that allows nothing writes no line.
        // DXF-AAMA piece text has no Rotation, Flip, Tilt or Fold.
        let rules = [("Rotation", &block.rotation), ("Flip", &block.flip), ("Tilt", &block.tilt), ("Fold", &block.fold)];
        for (identifier, value) in rules {
            if let Some(value) = value {
                system_lines.push(format!("{identifier}:{value}"));
            } // if value
        } // for each rule
    } // if DXF-ASTM
    system_lines.push(format!("Material:{}", block.material));
    for (i, line) in system_lines.iter().enumerate() {
        let position = Point::new(anchor.x, anchor.y - (i as f64 + 1.0) * SYSTEM_TEXT_HEIGHT_MM * 1.5);
        encode_astm_text(writer, "1", position, SYSTEM_TEXT_HEIGHT_MM, 0.0, line)?;
    } // for each system line

    // CLO3D line types: 0 = boundary, 2 = sewing line (seamly2clo.py convention).
    let (boundary_250, sew_250) = if options.clo3d_group_250 { (Some(0), Some(2)) } else { (None, None) };
    // DXF-AAMA has no quality validation layers.
    let validation = |layer: &'static str| (!aama).then_some(layer);
    if aama && block.mirror_line.is_some() {
        write_contour(writer, &start_on_mirror_line(boundary), "1", None, boundary_250)?;
    } else {
        write_contour(writer, boundary, "1", validation("84"), boundary_250)?;
    } // if AAMA with a mirror line
    for c in &block.sew_lines {
        write_contour(writer, c, "14", validation("87"), sew_250)?;
    } // for each sew line
    for c in &block.internal_lines {
        write_contour(writer, c, "8", validation("85"), None)?;
    } // for each internal line
    for c in &block.cutouts {
        write_contour(writer, c, "11", validation("86"), None)?;
    } // for each cutout

    // Layers 2 and 3 hold the turn and curve points of layers 1, 8, 11 and 14.
    let contours = std::iter::once(boundary)
        .chain(&block.sew_lines)
        .chain(&block.internal_lines)
        .chain(&block.cutouts)
        .filter(|c| c.declares_points());
    for contour in contours {
        for (p, &turn) in contour.reduced.iter().zip(&contour.turn) {
            encode_dxf_point(writer, &DxfPoint::new(if turn { "2" } else { "3" }, *p))?;
        } // for each key point
    } // for each contour

    // Mirror line: a two-point line on the first two boundary points (D6673 §4.3.6).
    if let Some((p1, p2)) = &block.mirror_line {
        write_group_code(writer, 0, "LINE")?;
        write_group_code(writer, 8, "6")?;
        write_group_code_float(writer, 10, p1.x)?;
        write_group_code_float(writer, 20, p1.y)?;
        write_group_code_float(writer, 11, p2.x)?;
        write_group_code_float(writer, 21, p2.y)?;
    } // if mirror line

    if let Some((p1, p2)) = &block.grainline {
        write_group_code(writer, 0, "LINE")?;
        write_group_code(writer, 8, "7")?;
        write_group_code_float(writer, 10, p1.x)?;
        write_group_code_float(writer, 20, p1.y)?;
        write_group_code_float(writer, 11, p2.x)?;
        write_group_code_float(writer, 21, p2.y)?;
    } // if grainline

    for notch in &block.notches {
        if aama {
            encode_notch(writer, &aama_notch(notch))?;
        } else {
            encode_notch(writer, notch)?;
        } // if AAMA
    } // for each notch

    // DXF-AAMA has no annotation layer: label text goes on layer 1, except lines that read as piece text.
    let annotation_layer = if aama { "1" } else { "15" };
    for a in block.annotations.iter().filter(|a| !aama || !repeats_piece_text(&a.text)) {
        encode_astm_text(writer, annotation_layer, a.position, a.height, a.rotation, &a.text)?;
    } // for each annotation

    if let Some(grading) = &block.grading {
        // The grade reference line gives the X axis of the rules (D6673 §4.3.5).
        let (p1, p2) = grading.reference_line;
        write_group_code(writer, 0, "LINE")?;
        write_group_code(writer, 8, "5")?;
        write_group_code_float(writer, 10, p1.x)?;
        write_group_code_float(writer, 20, p1.y)?;
        write_group_code_float(writer, 11, p2.x)?;
        write_group_code_float(writer, 21, p2.y)?;
        // `# n` on the layer of the point it grades, at the same XY (D6673 §4.3.1.4).
        for t in &grading.rule_texts {
            encode_astm_text(writer, t.layer, t.position, RULE_TEXT_HEIGHT_MM, 0.0, &format!("# {}", t.rule))?;
        } // for each rule text
    } // if graded
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
            Some(boundary) => write_astm_block(writer, block, boundary, options)?,
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
// @param is_code True for a group code line, false for a value line.
//                A value can look like a group code ("0", "70"), so position decides.
// @return Comment string explaining the line.
fn get_line_comment(line: &str, prev_line: &str, next_line: Option<&str>, is_code: bool) -> String {
    let line_trimmed = line.trim();

    // Group code line: explain what the next line holds.
    if is_code {
        return group_code_comment(line_trimmed);
    } // if group code line

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
            "POINT" => "Entity type: POINT (turn point, curve point or notch)".to_string(),
            "BLOCK_RECORD" => "Table entry: BLOCK_RECORD (one per block, DXF R13)".to_string(),
            "DICTIONARY" => "Object type: DICTIONARY (named object list, DXF R13)".to_string(),
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
            "CLASSES" => "Section name: CLASSES (custom object classes, DXF R13; empty here)".to_string(),
            "OBJECTS" => "Section name: OBJECTS (non-graphical objects, DXF R13)".to_string(),
            "LAYER" => "Table name: LAYER (this is the layer table)".to_string(),
            _ => format!("Name: {} (section/table/block name)", line_trimmed),
        }
    }
    // Group code 5 - handle; DIMSTYLE entries use group 105.
    else if prev_line.trim() == "5" || prev_line.trim() == "105" {
        format!("Handle: {} (unique hexadecimal object id, DXF R13)", line_trimmed)
    }
    // Group code 100 - subclass marker.
    else if prev_line.trim() == "100" {
        format!("Subclass marker: {} (data of this class follows, DXF R13)", line_trimmed)
    }
    // Group code 330 - owner handle.
    else if prev_line.trim() == "330" {
        format!("Owner handle: {} (object that owns this one)", line_trimmed)
    }
    // Group code 350 - handle of a dictionary entry.
    else if prev_line.trim() == "350" {
        format!("Handle: {} (object named by the group 3 before it)", line_trimmed)
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
        format!("Value: {} (notch depth in millimetres, or Z coordinate)", line_trimmed)
    }
    // Group code 39 - thickness; D6673 uses it for notch width.
    else if prev_line.trim() == "39" {
        format!("Value: {} (notch width in millimetres)", line_trimmed)
    }
    // Group code 250 - CLO3D line type (not DXF R11 or D6673).
    else if prev_line.trim() == "250" {
        format!("Value: {} (CLO3D line type: 0 = boundary, 2 = sewing line)", line_trimmed)
    }
    // Group code 66 - vertices-follow flag.
    else if prev_line.trim() == "66" {
        format!("Value: {} (1 = VERTEX entities follow)", line_trimmed)
    }
    // Group code 3 - block name (repeat), code page, or dictionary entry name.
    else if prev_line.trim() == "3" {
        format!("Name: {} (block name repeat, code page, or dictionary entry name)", line_trimmed)
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
    // Empty value.
    else if line_trimmed.is_empty() {
        "".to_string()
    }
    // Default: try to provide context.
    else {
        format!("Value: {} (data value)", line_trimmed)
    }
}

// @brief Comment for a group code line: what its value line holds.
fn group_code_comment(code: &str) -> String {
    match code {
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
        "3" => "Group code 3: Block name repeat, code page, or dictionary entry name follows".to_string(),
        "30" => "Group code 30: Notch depth (POINT) or Z coordinate follows".to_string(),
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
        "5" => "Group code 5: Handle follows".to_string(),
        "105" => "Group code 105: DIMSTYLE handle follows".to_string(),
        "100" => "Group code 100: Subclass marker follows".to_string(),
        "330" => "Group code 330: Owner handle follows".to_string(),
        "350" => "Group code 350: Dictionary entry handle follows".to_string(),
        _ => format!("Group code {}: (numeric group code)", code),
    } // match code
} // fn group_code_comment

// @brief Meaning of an ASTM D6673 layer number, for annotated comments.
fn astm_layer_meaning(layer: &str) -> &'static str {
    match layer {
        "1" => "ASTM piece boundary and system text",
        "2" => "ASTM turn point",
        "3" => "ASTM curve point",
        "4" => "ASTM slit or V notch",
        "5" => "ASTM grade reference line",
        "6" => "ASTM mirror line",
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

// @brief Create an annotated version of a DXF file with inline comments.
// @param dxf_path Path to the DXF file.
// @param standard "DXF-ASTM" or "DXF-AAMA", for the title line.
// @return Result indicating success or error.
fn create_annotated_version(
    dxf_path: &Path,
    progress_callback: Option<&ProgressCallback>,
    standard: &str,
) -> std::io::Result<()> {
    // Read the DXF file.
    let content = read_to_string(dxf_path)?;
    let lines: Vec<&str> = content.lines().collect();
    let total = lines.len().max(1);

    // Create annotated version path (same directory, .txt extension).
    let mut annotated_path = dxf_path.to_path_buf();
    annotated_path.set_extension("txt");

    // Create annotated version file.
    let mut annotated_file = File::create(&annotated_path)?;

    // Write header comment.
    writeln!(
        annotated_file,
        "// {} Annotated Version with Inline Comments",
        standard
    )?;
    writeln!(
        annotated_file,
        "// Generated automatically from: {}",
        dxf_path
            .file_name()
            .and_then(|s| s.to_str())
            .unwrap_or("unknown.dxf")
    )?;
    writeln!(
        annotated_file,
        "// This file contains the DXF content with explanatory comments for each line."
    )?;
    writeln!(
        annotated_file,
        "// Comments are positioned two tabs to the right of the DXF data."
    )?;
    writeln!(annotated_file, "//")?;
    writeln!(annotated_file)?;

    // Process each line and add comments.
    for (i, line) in lines.iter().enumerate() {
        let prev_line = if i > 0 { lines[i - 1] } else { "" };
        let next_line = if i + 1 < lines.len() {
            Some(lines[i + 1])
        } else {
            None
        };

        let comment = get_line_comment(line, prev_line, next_line, i % 2 == 0);

        if comment.is_empty() {
            // Empty line - just write it.
            writeln!(annotated_file, "{}", line)?;
        } else {
            // Write line with comment (two tabs distance).
            writeln!(annotated_file, "{}\t\t// {}", line, comment)?;
        }
        if let Some(callback) = progress_callback {
            let progress = (i + 1) as f32 / total as f32;
            callback(progress);
        }
    }

    Ok(())
}

// @brief Export Drawing to DXF-ASTM format, or to DXF-AAMA when `options.aama` is set.
// @param drawing The ezdxf Drawing object to export; its `version` selects DXF R11 or R13.
// @param output_path Path to write the DXF file.
// @param options Export options.
// @return Result indicating success or error.
pub fn export_dxf_astm(
    drawing: &seamly_svg2ezdxf::Drawing,
    output_path: impl AsRef<std::path::Path>,
    options: &DxfAstmExportOptions,
) -> Result<()> {
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

    // Write the AC1009 file structure to memory; an R13 file is rewritten from it.
    // An R11 file is the same file: R11 is AC1009.
    let mut buffer: Vec<u8> = Vec::new();
    // 1. HEADER section (minimal or empty).
    write_header_section(&mut buffer, options.include_header)?;
    // 2. BLOCKS section (pattern pieces).
    write_blocks_section(&mut buffer, drawing, options)?;
    // 3. ENTITIES section (modelspace entities).
    write_entities_section(&mut buffer, drawing, options)?;
    // 4. EOF marker.
    write_eof(&mut buffer)?;

    let r12 = String::from_utf8(buffer)
        .map_err(|e| std::io::Error::new(std::io::ErrorKind::InvalidData, e))?;
    let text = match drawing.version {
        DxfVersion::R11 => r12,
        DxfVersion::R13 => upgrade_to_r13(&r12)?,
    }; // match version
    std::fs::write(output_path.as_ref(), text)?;

    // A graded export writes its grade rule table beside the DXF, same stem, `.rul`.
    if let Some(table) = &drawing.grade_rule_table {
        write_grade_rule_file(table, &output_path.as_ref().with_extension("rul"), options)?;
    } // if graded

    // 5. Create annotated version with inline comments (if requested).
    if options.create_annotated_version {
        let standard = if options.aama { "DXF-AAMA" } else { "DXF-ASTM" };
        create_annotated_version(output_path.as_ref(), options.progress_callback.as_ref(), standard)
            .map_err(|e| DxfAstmExportError::Io(e))?;
    }

    Ok(())
}
