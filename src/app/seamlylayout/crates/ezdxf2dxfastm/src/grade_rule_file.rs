// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Grade rule table file (D6673 §4.4): the `.rul` file beside a graded DXF-ASTM export.
//!
//! An ASCII file: a header, then one `RULE: DELTA` line per grade rule
//! identifier. Each rule lists the X,Y growth of every size from the sample
//! size, in size list order. `UNITS:METRIC` writes millimetres to two places;
//! `UNITS:ENGLISH` writes inches to four places.
//! A DXF-AAMA export writes the same file with an `ANSI/AAMA VERSION:` first line.

use crate::writer::{utc_date_time, DxfAstmExportOptions, ASTM_VERSION};
use seamly_svg2ezdxf::{DxfUnits, GradeRuleTable};
use std::path::Path;

/// @brief `ANSI/AAMA VERSION:` value, the first line of a DXF-AAMA rule file.
pub const AAMA_RULE_FILE_VERSION: &str = "1.0.0";

/// @brief Grade rule table name from a file stem: ASCII, separators replaced by `_`.
/// @details §4.4.1: a name holds no separator (comma, space, tab, newline).
///          The same name goes in the DXF `Grade Rule Table:` style text.
pub fn grade_rule_table_name(stem: &str) -> String {
    let name = without_separators(&seamly_svg2ezdxf::sanitize_ascii(stem));
    if name.is_empty() { "GradeRules".to_string() } else { name }
} // fn grade_rule_table_name

/// @brief Text of the grade rule table file.
/// @param units DXF units of the growth values.
/// @param options Supplies `Creation Date:` (dd-mm-yyyy) and `Creation Time:` (hh-mm);
///        None uses the current UTC date and time. The file writes the time as hh:mm.
pub fn grade_rule_file_text(table: &GradeRuleTable, units: DxfUnits, options: &DxfAstmExportOptions) -> String {
    let (utc_date, utc_time) = utc_date_time();
    let date = options.creation_date.clone().unwrap_or(utc_date);
    let time = options.creation_time.clone().unwrap_or(utc_time).replace('-', ":");
    let sizes: Vec<String> = table.sizes.iter().map(|s| without_separators(s)).collect();

    let version = if options.aama {
        format!("ANSI/AAMA VERSION:{}", AAMA_RULE_FILE_VERSION)
    } else {
        format!("ASTM/D13 Proposal 1 VERSION:{}", ASTM_VERSION)
    }; // version line
    // Header: each identifier once, before the rules (§4.4.1).
    let mut lines = vec![
        version,
        // AUTHOR allows letters, digits, dashes and underscores only.
        "AUTHOR:Seamly2D".to_string(),
        format!("CREATION DATE:{}", date),
        format!("CREATION TIME:{}", time),
        format!("UNITS:{}", units.name()),
        format!("GRADE RULE TABLE:{}", table.name),
        format!("SAMPLE SIZE:{}", without_separators(&table.sample_size)),
        format!("NUMBER OF SIZES:{}", sizes.len()),
        format!("SIZE LIST:{}", sizes.join(" ")),
    ];
    // Rules: one growth pair per size, in size list order (§4.4.2).
    for rule in &table.rules {
        let growths: Vec<String> = rule.deltas.iter().map(|&(x, y)| format!("{},{}", growth(units, x), growth(units, y))).collect();
        lines.push(format!("RULE: DELTA {} {}", rule.id, growths.join(" ")));
    } // for each rule
    let mut text = lines.join("\n");
    text.push('\n');
    text
} // fn grade_rule_file_text

// @brief Write the grade rule table file.
pub(crate) fn write_grade_rule_file(table: &GradeRuleTable, units: DxfUnits, path: &Path, options: &DxfAstmExportOptions) -> std::io::Result<()> {
    std::fs::write(path, grade_rule_file_text(table, units, options))
} // fn write_grade_rule_file

// @brief A growth value (millimetres) in the DXF units; a value that rounds to zero is written without a sign.
fn growth(units: DxfUnits, value: f64) -> String {
    let text = units.length(value);
    match text.strip_prefix('-') {
        Some(rest) if rest.chars().all(|c| c == '0' || c == '.') => rest.to_string(),
        _ => text,
    }
} // fn growth

// @brief `s` trimmed, with every rule file separator replaced by `_`.
fn without_separators(s: &str) -> String {
    s.trim().chars().map(|c| if c == ',' || c.is_whitespace() { '_' } else { c }).collect()
} // fn without_separators
