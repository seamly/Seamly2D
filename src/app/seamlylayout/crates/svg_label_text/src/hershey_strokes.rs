// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file hershey_strokes.rs
// @brief Mode 3: replaces each label `<text>` with single-stroke Hershey polylines.
//
// Glyph data: Hershey Roman Simplex (`futural.jhf`), see
// assets/hershey/HERSHEY_LICENSE.txt. Each glyph is a set of open polylines,
// so a plotter or cutter draws every character in one pen pass.
//
// Hershey glyph units: x grows right, y grows down, the baseline is y = 9 and
// the cap height is 21 units. One em is taken as 32 units.

use std::sync::OnceLock;

use xmltree::{Element, XMLNode};

use crate::{for_each_label_text, new_element, text_content, LabelTextReport, TextStyle};

/// Raw Hershey Roman Simplex data: one glyph per record, ASCII 32 upward.
const FUTURAL_JHF: &str = include_str!("../assets/hershey/futural.jhf");

/// First character in `FUTURAL_JHF`.
const FIRST_CHAR: char = ' ';

/// Hershey y of the text baseline.
const BASELINE_Y: f64 = 9.0;

/// Hershey units per em; font-size / EM_UNITS is the scale to user units.
const EM_UNITS: f64 = 32.0;

/// Stroke width as a fraction of the font size.
const STROKE_WIDTH_PER_EM: f64 = 1.0 / 20.0;

/// Stand-in for characters Roman Simplex does not have.
const MISSING_GLYPH: char = '?';

/// @brief Glyph records of `FUTURAL_JHF`, without the 8-character record header.
///
/// A record starts with a 5-character id and a 3-character vertex count, then
/// two characters per vertex; the first vertex holds the left and right bounds.
/// The bundled file has one record per line; a test pins that it parses to 96
/// records, because a wrapped record would shift every later character.
pub(crate) fn glyph_records() -> &'static [&'static str] {
    static RECORDS: OnceLock<Vec<&'static str>> = OnceLock::new();
    RECORDS.get_or_init(|| {
        FUTURAL_JHF
            .lines()
            .map(|line| line.trim_end_matches('\r'))
            .filter(|line| line.len() >= 8)
            .map(|line| {
                // Clamp to the line so a short record cannot panic.
                let count: usize = line[5..8].trim().parse().unwrap_or(0);
                &line[8..(8 + 2 * count).min(line.len())]
            })
            .collect()
    })
} // fn glyph_records

/// @brief Hershey font over the bundled Roman Simplex records.
fn font() -> hershey::Font<'static> {
    hershey::Font::new(glyph_records(), FIRST_CHAR)
} // fn font

/// @brief Left and right bounds of `ch`, or None when the font has no glyph for it.
fn glyph_bounds(ch: char) -> Option<(i32, i32)> {
    let index = (ch as usize).checked_sub(FIRST_CHAR as usize)?;
    let record = glyph_records().get(index)?;
    let mut bytes = record.bytes();
    let left = bytes.next()? as i32 - 'R' as i32;
    let right = bytes.next()? as i32 - 'R' as i32;
    Some((left, right))
} // fn glyph_bounds

/// @brief Character actually drawn for `ch`: itself, a space for whitespace, or `MISSING_GLYPH`.
fn drawable(ch: char) -> (char, bool) {
    if ch.is_whitespace() {
        return (' ', false); // tabs and newlines advance like a space
    } // if whitespace
    if glyph_bounds(ch).is_some() {
        (ch, false) // glyph present
    } else {
        (MISSING_GLYPH, true) // glyph missing
    } // if glyph present
} // fn drawable

/// @brief Advance width of `text` in Hershey units.
fn advance_units(text: &str) -> i32 {
    text.chars()
        .filter_map(|ch| glyph_bounds(drawable(ch).0))
        .map(|(left, right)| right - left)
        .sum()
} // fn advance_units

/// @brief SVG path data for `text` drawn from baseline point (`x`, `y`) at `scale`.
/// @return Path data (empty for blank text) and the number of missing glyphs.
pub(crate) fn text_path_data(text: &str, x: f64, y: f64, scale: f64) -> (String, usize) {
    let font = font();
    let mut d = String::new();
    let mut missing = 0usize;
    let mut pen_x = x;
    for ch in text.chars() {
        let (ch, was_missing) = drawable(ch);
        missing += usize::from(was_missing);
        let Some((left, right)) = glyph_bounds(ch) else { continue };
        let Ok(glyph) = font.glyph(ch) else { continue };
        // Glyph x is relative to its centre; shift so its left bound sits on the pen.
        let to_user = |gx: i32, gy: i32| {
            (pen_x + f64::from(gx - left) * scale, y + (f64::from(gy) - BASELINE_Y) * scale)
        };
        for vector in &glyph.vectors {
            match *vector {
                hershey::Vector::MoveTo { x: gx, y: gy } => {
                    // Pen up: start a new polyline.
                    let (ux, uy) = to_user(gx, gy);
                    d.push_str(&format!("M{ux:.3} {uy:.3}"));
                } // MoveTo
                hershey::Vector::LineTo { x: gx, y: gy } => {
                    // Pen down: extend the current polyline.
                    let (ux, uy) = to_user(gx, gy);
                    d.push_str(&format!("L{ux:.3} {uy:.3}"));
                } // LineTo
            } // match vector
        } // for vector
        pen_x += f64::from(right - left) * scale;
    } // for ch
    (d, missing)
} // fn text_path_data

/// @brief First number of an SVG length list such as `x="10 20"`; 0 when absent.
fn first_number(value: Option<&String>) -> f64 {
    value
        .and_then(|v| v.split([' ', ',']).find(|s| !s.is_empty()).map(str::to_string))
        .and_then(|s| s.trim_end_matches("px").parse::<f64>().ok())
        .unwrap_or(0.0)
} // fn first_number

/// @brief Rewrite one label `<text>` in place as a `<g>` of stroked Hershey polylines.
///
/// The group keeps the text's `id` and `transform`, stores the string in
/// `data-text` and in a `<desc>`, and draws in the text's fill color.
/// @return Number of characters drawn with `MISSING_GLYPH`.
fn convert_text(text: &mut Element, style: &TextStyle) -> usize {
    let string = text_content(text);
    let font_size = style.font_size_px();
    let scale = font_size / EM_UNITS;

    // Baseline start; text-anchor moves it left by all or half the width.
    let mut x = first_number(text.attributes.get("x"));
    let y = first_number(text.attributes.get("y"));
    let width = f64::from(advance_units(&string)) * scale;
    match style.text_anchor.as_deref() {
        Some("middle") => x -= width / 2.0, // centred on x
        Some("end") => x -= width,          // ends at x
        _ => {}                             // start: begins at x
    } // match text-anchor
    let (d, missing) = text_path_data(&string, x, y, scale);

    // Build the replacement group from the old element's identity.
    let mut group = new_element("g", text);
    for keep in ["id", "transform"] {
        if let Some(value) = text.attributes.get(keep) {
            group.attributes.insert(keep.to_string(), value.clone());
        } // if attribute present
    } // for keep
    group.attributes.insert("data-type".into(), "label_text".into());
    group.attributes.insert("data-text".into(), string.clone());
    group.attributes.insert("fill".into(), "none".into());
    let color = style.fill.clone().filter(|c| c != "none").unwrap_or_else(|| "#000000".into());
    group.attributes.insert("stroke".into(), color);
    if let Some(opacity) = &style.fill_opacity {
        group.attributes.insert("stroke-opacity".into(), opacity.clone());
    } // if opacity
    group.attributes.insert("stroke-width".into(), format!("{:.3}", font_size * STROKE_WIDTH_PER_EM));
    group.attributes.insert("stroke-linecap".into(), "round".into());
    group.attributes.insert("stroke-linejoin".into(), "round".into());

    // <desc> keeps the string readable for tools that ignore data-* attributes.
    let mut desc = new_element("desc", text);
    desc.children.push(XMLNode::Text(string));
    group.children.push(XMLNode::Element(desc));
    if !d.is_empty() {
        // Blank lines draw nothing; skip the empty path.
        let mut path = new_element("path", text);
        path.attributes.insert("d".into(), d);
        group.children.push(XMLNode::Element(path));
    } // if path data

    *text = group;
    missing
} // fn convert_text

/// @brief Mode 3 entry point: convert every label `<text>` under `root`.
pub(crate) fn convert_to_strokes(root: &mut Element) -> LabelTextReport {
    let mut report = LabelTextReport::default();
    let mut missing = 0usize;
    for_each_label_text(root, &mut |text, style| {
        report.text_elements += 1;
        missing += convert_text(text, style);
    });
    if missing > 0 {
        // One summary line; per-character detail would flood the dialog.
        report.warnings.push(format!(
            "{missing} label character(s) have no single-stroke glyph and were drawn as '{MISSING_GLYPH}'."
        ));
    } // if missing
    report
} // fn convert_to_strokes
