// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief DXF R13 (AC1012) file structure around the D6673 group stream.
//!
//! The R12 writer stays the one source of D6673 content. This module reads its
//! group pairs and writes them again with what R13 adds:
//! - a handle (group 5; group 105 for DIMSTYLE) on every entity, table, table entry and object;
//! - subclass markers (group 100);
//! - CLASSES, TABLES (with BLOCK_RECORD) and OBJECTS sections;
//! - the *MODEL_SPACE and *PAPER_SPACE blocks.
//!
//! Owner handles (group 330) on entities and table entries start with R14, so
//! only the two dictionaries carry one.
//!
//! Section order: HEADER, CLASSES, TABLES, BLOCKS, ENTITIES, OBJECTS, EOF.

use std::io::{Error, ErrorKind, Result};

// One group code and its value.
type Pair = (i32, String);

// @brief One record of the R12 stream: the group 0 value and the pairs after it.
struct Record {
    kind: String,
    groups: Vec<Pair>,
}

impl Record {
    // @brief First value of a group code.
    fn get(&self, code: i32) -> Option<&str> {
        self.groups.iter().find(|(c, _)| *c == code).map(|(_, v)| v.as_str())
    }
} // impl Record

// @brief Sequential handle source; handle 0 is reserved, so the first handle is 1.
struct Handles {
    next: u32,
}

impl Handles {
    // @brief Next unused handle, upper-case hexadecimal.
    fn take(&mut self) -> String {
        let handle = format!("{:X}", self.next);
        self.next += 1;
        handle
    }
} // impl Handles

// @brief Output pair list with short push helpers.
#[derive(Default)]
struct Out {
    pairs: Vec<Pair>,
}

impl Out {
    // @brief Append one group code and value.
    fn push(&mut self, code: i32, value: impl Into<String>) {
        self.pairs.push((code, value.into()));
    }
} // impl Out

// @brief Read the DXF text as (group code, value) pairs.
fn parse_pairs(text: &str) -> Result<Vec<Pair>> {
    let lines: Vec<&str> = text.lines().collect();
    if lines.len() % 2 != 0 {
        return Err(Error::new(ErrorKind::InvalidData, "DXF text has an odd number of lines"));
    } // if odd line count
    lines
        .chunks(2)
        .map(|c| {
            let code = c[0]
                .trim()
                .parse::<i32>()
                .map_err(|_| Error::new(ErrorKind::InvalidData, format!("bad DXF group code '{}'", c[0])))?;
            Ok((code, c[1].to_string()))
        })
        .collect()
} // fn parse_pairs

// @brief Split pairs into records, each starting at a group 0 pair.
fn split_records(pairs: Vec<Pair>) -> Vec<Record> {
    let mut records: Vec<Record> = Vec::new();
    for (code, value) in pairs {
        if code == 0 {
            records.push(Record { kind: value, groups: Vec::new() });
        } else if let Some(r) = records.last_mut() {
            r.groups.push((code, value));
        } // if group 0
    } // for each pair
    records
} // fn split_records

// @brief Subclass markers of an entity: those after the layer, and one closing marker.
// @return None for an entity type the R12 writer never emits.
fn subclass_markers(kind: &str) -> Option<(&'static [&'static str], Option<&'static str>)> {
    Some(match kind {
        "LINE" => (&["AcDbLine"], None),
        "POINT" => (&["AcDbPoint"], None),
        "CIRCLE" => (&["AcDbCircle"], None),
        // TEXT repeats AcDbText before its vertical alignment (73), which is never written.
        "TEXT" => (&["AcDbText"], Some("AcDbText")),
        "POLYLINE" => (&["AcDb2dPolyline"], None),
        "VERTEX" => (&["AcDbVertex", "AcDb2dVertex"], None),
        "SEQEND" => (&[], None),
        "INSERT" => (&["AcDbBlockReference"], None),
        "BLOCK" => (&["AcDbBlockBegin"], None),
        "ENDBLK" => (&["AcDbBlockEnd"], None),
        _ => return None,
    })
} // fn subclass_markers

// @brief Write one entity record in R13 form.
// @param default_layer Layer for a record without group 8 (the R12 ENDBLK).
fn write_entity(out: &mut Out, handles: &mut Handles, record: &Record, default_layer: &str) -> Result<()> {
    let (markers, closing) = subclass_markers(&record.kind)
        .ok_or_else(|| Error::new(ErrorKind::InvalidData, format!("no R13 form for DXF entity '{}'", record.kind)))?;

    // Common entity data: type, handle, AcDbEntity, layer.
    out.push(0, record.kind.as_str());
    out.push(5, handles.take());
    out.push(100, "AcDbEntity");
    out.push(8, record.get(8).unwrap_or(default_layer));
    for m in markers {
        out.push(100, *m);
    } // for each marker

    // R13 POLYLINE: a dummy point that is always 0; group 66 is obsolete.
    if record.kind == "POLYLINE" {
        out.push(10, "0.00");
        out.push(20, "0.00");
        out.push(30, "0.00");
    } // if POLYLINE

    // Entity data as the R12 writer wrote it, CLO3D group 250 included.
    for (code, value) in &record.groups {
        let skip = *code == 8 || (record.kind == "POLYLINE" && *code == 66);
        if !skip {
            out.push(*code, value.as_str());
        } // if kept
    } // for each group

    if record.kind == "BLOCK" {
        out.push(1, ""); // xref path name: none
    } // if BLOCK
    if let Some(m) = closing {
        out.push(100, m);
    } // if closing marker
    Ok(())
} // fn write_entity

// @brief Write one symbol table with its records.
// @param records Each record's own pairs, starting with its name (group 2).
fn write_table(out: &mut Out, handles: &mut Handles, name: &str, record_subclass: &str, records: &[Vec<Pair>]) {
    out.push(0, "TABLE");
    out.push(2, name);
    out.push(5, handles.take());
    out.push(100, "AcDbSymbolTable");
    out.push(70, records.len().to_string());
    // DIMSTYLE records keep group 5 for a dimension setting, so their handle is group 105.
    let handle_code = if name == "DIMSTYLE" { 105 } else { 5 };
    for record in records {
        out.push(0, name);
        out.push(handle_code, handles.take());
        out.push(100, "AcDbSymbolTableRecord");
        out.push(100, record_subclass);
        out.pairs.extend(record.iter().cloned());
    } // for each record
    out.push(0, "ENDTAB");
} // fn write_table

// @brief Build a table record from (code, value) literals.
fn rec(pairs: &[(i32, &str)]) -> Vec<Pair> {
    pairs.iter().map(|(c, v)| (*c, v.to_string())).collect()
} // fn rec

// @brief Bounding box (min x, min y, max x, max y) of every point group (10/20, 11/21).
// @return None when the records hold no coordinates.
fn extents(records: &[&Record]) -> Option<(f64, f64, f64, f64)> {
    let mut b: Option<(f64, f64, f64, f64)> = None;
    for r in records {
        for (x_code, y_code) in [(10, 20), (11, 21)] {
            let (Some(x), Some(y)) = (r.get(x_code), r.get(y_code)) else { continue };
            let (Ok(x), Ok(y)) = (x.trim().parse::<f64>(), y.trim().parse::<f64>()) else { continue };
            b = Some(match b {
                None => (x, y, x, y),
                Some((x0, y0, x1, y1)) => (x0.min(x), y0.min(y), x1.max(x), y1.max(y)),
            });
        } // for each point group
    } // for each record
    b
} // fn extents

// @brief Write the TABLES section.
// @param layers Layer names in first-use order, without "0".
// @param blocks Piece block names.
// @param view   Initial view: centre x, centre y, height.
fn write_tables(out: &mut Out, handles: &mut Handles, layers: &[String], blocks: &[String], view: (f64, f64, f64)) {
    out.push(0, "SECTION");
    out.push(2, "TABLES");

    // The active viewport opens on the whole drawing.
    let (cx, cy, height) = (format!("{:.2}", view.0), format!("{:.2}", view.1), format!("{:.2}", view.2));
    let vport = rec(&[
        (2, "*ACTIVE"), (70, "0"),
        (10, "0.00"), (20, "0.00"), (11, "1.00"), (21, "1.00"),
        (12, cx.as_str()), (22, cy.as_str()),
        (13, "0.00"), (23, "0.00"), (14, "10.00"), (24, "10.00"), (15, "10.00"), (25, "10.00"),
        (16, "0.00"), (26, "0.00"), (36, "1.00"), (17, "0.00"), (27, "0.00"), (37, "0.00"),
        (40, height.as_str()), (41, "1.50"), (42, "50.00"), (43, "0.00"), (44, "0.00"), (50, "0.00"), (51, "0.00"),
        (71, "0"), (72, "100"), (73, "1"), (74, "3"), (75, "0"), (76, "0"), (77, "0"), (78, "0"),
    ]);
    write_table(out, handles, "VPORT", "AcDbViewportTableRecord", &[vport]);

    let ltype = |name: &str, desc: &str| rec(&[(2, name), (70, "0"), (3, desc), (72, "65"), (73, "0"), (40, "0.00")]);
    write_table(
        out,
        handles,
        "LTYPE",
        "AcDbLinetypeTableRecord",
        &[ltype("BYBLOCK", ""), ltype("BYLAYER", ""), ltype("CONTINUOUS", "Solid line")],
    );

    // Layer 0 always exists; the D6673 layers follow in first-use order.
    let layer_records: Vec<Vec<Pair>> = std::iter::once("0")
        .chain(layers.iter().map(String::as_str))
        .map(|name| rec(&[(2, name), (70, "0"), (62, "7"), (6, "CONTINUOUS")]))
        .collect();
    write_table(out, handles, "LAYER", "AcDbLayerTableRecord", &layer_records);

    let standard_style = rec(&[
        (2, "STANDARD"), (70, "0"), (40, "0.00"), (41, "1.00"), (50, "0.00"), (71, "0"), (42, "2.50"), (3, "txt"), (4, ""),
    ]);
    write_table(out, handles, "STYLE", "AcDbTextStyleTableRecord", &[standard_style]);
    write_table(out, handles, "VIEW", "AcDbViewTableRecord", &[]);
    write_table(out, handles, "UCS", "AcDbUCSTableRecord", &[]);
    write_table(out, handles, "APPID", "AcDbRegAppTableRecord", &[rec(&[(2, "ACAD"), (70, "0")])]);
    write_table(out, handles, "DIMSTYLE", "AcDbDimStyleTableRecord", &[rec(&[(2, "STANDARD"), (70, "0")])]);

    // One block record per block, the two layout blocks first.
    let block_records: Vec<Vec<Pair>> = ["*MODEL_SPACE", "*PAPER_SPACE"]
        .into_iter()
        .chain(blocks.iter().map(String::as_str))
        .map(|name| rec(&[(2, name)]))
        .collect();
    write_table(out, handles, "BLOCK_RECORD", "AcDbBlockTableRecord", &block_records);

    out.push(0, "ENDSEC");
} // fn write_tables

// @brief Rewrite R12 DXF text from the D6673 writer as DXF R13 (AC1012).
// @param r12 Complete R12 file text: HEADER, BLOCKS, ENTITIES, EOF.
// @return R13 file text, or InvalidData for text the R12 writer cannot produce.
pub fn upgrade_to_r13(r12: &str) -> Result<String> {
    let records = split_records(parse_pairs(r12)?);

    // Sort the records of the BLOCKS and ENTITIES sections; the R12 HEADER is replaced.
    let (mut block_records, mut entity_records): (Vec<&Record>, Vec<&Record>) = (Vec::new(), Vec::new());
    let mut section = "";
    for r in &records {
        match r.kind.as_str() {
            "SECTION" => section = r.get(2).unwrap_or(""),
            "ENDSEC" | "EOF" => section = "",
            _ if section == "BLOCKS" => block_records.push(r),
            _ if section == "ENTITIES" => entity_records.push(r),
            _ => {}
        } // match kind
    } // for each record

    // Layers in first-use order and piece block names, for the tables.
    let mut layers: Vec<String> = Vec::new();
    for r in block_records.iter().chain(&entity_records) {
        if let Some(layer) = r.get(8) {
            if layer != "0" && !layers.iter().any(|l| l == layer) {
                layers.push(layer.to_string());
            } // if new layer
        } // if layer
    } // for each record
    let blocks: Vec<String> = block_records
        .iter()
        .filter(|r| r.kind == "BLOCK")
        .filter_map(|r| r.get(2).map(str::to_string))
        .collect();

    // Initial view: drawing extents with a 5 % margin.
    let all: Vec<&Record> = block_records.iter().chain(&entity_records).copied().collect();
    let view = match extents(&all) {
        Some((x0, y0, x1, y1)) => ((x0 + x1) / 2.0, (y0 + y1) / 2.0, ((y1 - y0).max((x1 - x0) / 1.5) * 1.05).max(1.0)),
        None => (0.0, 0.0, 1.0),
    };

    let mut handles = Handles { next: 1 };
    let mut body = Out::default();

    // CLASSES: no custom classes.
    body.push(0, "SECTION");
    body.push(2, "CLASSES");
    body.push(0, "ENDSEC");

    write_tables(&mut body, &mut handles, &layers, &blocks, view);

    // BLOCKS: the two layout blocks, then the piece blocks.
    body.push(0, "SECTION");
    body.push(2, "BLOCKS");
    for name in ["*MODEL_SPACE", "*PAPER_SPACE"] {
        let begin = Record {
            kind: "BLOCK".to_string(),
            groups: rec(&[(8, "0"), (2, name), (70, "0"), (10, "0.00"), (20, "0.00"), (30, "0.00"), (3, name)]),
        };
        write_entity(&mut body, &mut handles, &begin, "0")?;
        write_entity(&mut body, &mut handles, &Record { kind: "ENDBLK".to_string(), groups: Vec::new() }, "0")?;
    } // for each layout block
    // ENDBLK takes the layer of its BLOCK.
    let mut block_layer = "0".to_string();
    for r in &block_records {
        if r.kind == "BLOCK" {
            block_layer = r.get(8).unwrap_or("0").to_string();
        } // if BLOCK
        write_entity(&mut body, &mut handles, r, &block_layer)?;
    } // for each block record
    body.push(0, "ENDSEC");

    body.push(0, "SECTION");
    body.push(2, "ENTITIES");
    for r in &entity_records {
        write_entity(&mut body, &mut handles, r, "0")?;
    } // for each entity
    body.push(0, "ENDSEC");

    // OBJECTS: the named object dictionary with its required ACAD_GROUP entry.
    let (root, group) = (handles.take(), handles.take());
    body.push(0, "SECTION");
    body.push(2, "OBJECTS");
    body.push(0, "DICTIONARY");
    body.push(5, root.as_str());
    body.push(330, "0"); // the root has no owner
    body.push(100, "AcDbDictionary");
    body.push(3, "ACAD_GROUP");
    body.push(350, group.as_str());
    body.push(0, "DICTIONARY");
    body.push(5, group.as_str());
    body.push(330, root.as_str());
    body.push(100, "AcDbDictionary");
    body.push(0, "ENDSEC");
    body.push(0, "EOF");

    // HEADER last: $HANDSEED must exceed every handle written above.
    let mut out = Out::default();
    out.push(0, "SECTION");
    out.push(2, "HEADER");
    out.push(9, "$ACADVER");
    out.push(1, "AC1012");
    out.push(9, "$DWGCODEPAGE");
    out.push(3, "ANSI_1252");
    out.push(9, "$HANDSEED");
    out.push(5, handles.take());
    out.push(0, "ENDSEC");
    out.pairs.extend(body.pairs);

    let mut text = String::new();
    for (code, value) in &out.pairs {
        text.push_str(&format!("{}\n{}\n", code, value));
    } // for each pair
    Ok(text)
} // fn upgrade_to_r13
