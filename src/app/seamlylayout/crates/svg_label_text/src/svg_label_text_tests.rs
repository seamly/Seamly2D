// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file svg_label_text_tests.rs
// @brief Unit tests for the three label text modes and label text detection.

use super::*;

/// Bundled single-line font, loaded into test font databases as a known face.
const RELIEF: &[u8] = include_bytes!("../assets/fonts/ReliefSingleLineOutline-Regular.otf");

/// Layout SVG with Seamly2D label markup (Qt `QSvgGenerator` output).
const TROUSERS: &str = include_str!("../test_data/trousers_labels.svg");

/// @brief Small document: one piece with a piece label, a pattern label and an unlabelled text.
fn sample(family: &str) -> svg_dom::Document {
    svg_dom::Document::parse(&format!(
        r##"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
  <g id="piece_A" data-type="piece" data-name="A">
    <g id="piece_label_1_A" data-type="piece_label" data-parent="A" data-type-number="1">
      <g font-family="{family}" font-size="9">
        <text fill="#112233" fill-opacity="0.5" transform="matrix(1 0 0 1 10 20)" x="0" y="9">Front</text>
      </g>
    </g>
    <g id="pattern_label_1_A" data-type="pattern_label" data-parent="A" data-type-number="1">
      <text font-family="{family}" font-size="9" x="0" y="30" text-anchor="middle">Size 12</text>
    </g>
    <text id="note" x="5" y="5">not a label</text>
  </g>
</svg>"##
    ))
    .expect("sample parses")
} // fn sample

/// @brief Font database holding only the bundled single-line font.
fn relief_db() -> fontdb::Database {
    let mut db = fontdb::Database::new();
    db.load_font_data(RELIEF.to_vec());
    db
} // fn relief_db

/// @brief Family name of the bundled font as fontdb lists it.
fn relief_family() -> String {
    relief_db().faces().next().expect("bundled font loads").families[0].0.clone()
} // fn relief_family

/// @brief All elements named `name` anywhere under `element`.
fn find_all<'a>(element: &'a Element, name: &str, out: &mut Vec<&'a Element>) {
    for child in &element.children {
        if let XMLNode::Element(e) = child {
            if e.name == name {
                out.push(e);
            } // if match
            find_all(e, name, out);
        } // if element
    } // for child
} // fn find_all

/// @brief Every label group's attributes, in document order.
fn label_group_attributes(doc: &svg_dom::Document) -> Vec<Vec<(String, String)>> {
    let mut groups = Vec::new();
    find_all(&doc.root, "g", &mut groups);
    groups
        .into_iter()
        .filter(|g| is_label_group(g))
        .map(|g| g.attributes.iter().map(|(k, v)| (k.clone(), v.clone())).collect())
        .collect()
} // fn label_group_attributes

/// @brief `<text>` elements that sit inside label groups.
fn label_texts(doc: &mut svg_dom::Document) -> usize {
    let mut n = 0;
    for_each_label_text(&mut doc.root, &mut |_, _| n += 1);
    n
} // fn label_texts

#[test]
fn hershey_data_parses_to_one_record_per_ascii_glyph() {
    // 96 records: ASCII 32 (space) to 127. A wrapped record would break this.
    assert_eq!(hershey_strokes::glyph_records().len(), 96);
    assert_eq!(hershey_strokes::text_path_data(" ", 0.0, 0.0, 1.0).0, "");
    let (d, missing) = hershey_strokes::text_path_data("A", 0.0, 0.0, 1.0);
    assert!(d.starts_with('M') && d.contains('L'), "A draws polylines: {d}");
    assert_eq!(missing, 0);
} // hershey_data_parses_to_one_record_per_ascii_glyph

#[test]
fn state_reports_text_paths_only_and_no_labels() {
    assert_eq!(label_text_state(&sample("Arial")), LabelTextState::Text);

    let paths_only = svg_dom::Document::parse(
        r#"<svg xmlns="http://www.w3.org/2000/svg"><g data-type="piece_label"><path d="M0 0L1 1"/></g></svg>"#,
    )
    .unwrap();
    assert_eq!(label_text_state(&paths_only), LabelTextState::PathsOnly);

    let no_labels = svg_dom::Document::parse(
        r#"<svg xmlns="http://www.w3.org/2000/svg"><g data-type="piece"><text>x</text></g></svg>"#,
    )
    .unwrap();
    assert_eq!(label_text_state(&no_labels), LabelTextState::NoLabels);
} // state_reports_text_paths_only_and_no_labels

#[test]
fn mode_names_round_trip() {
    for mode in [SvgTextMode::DesignerFont, SvgTextMode::SingleLineFont, SvgTextMode::HersheyStrokes] {
        assert_eq!(SvgTextMode::from_name(mode.name()), Some(mode));
    } // for mode
    assert_eq!(SvgTextMode::from_name("asSupplied"), None);
} // mode_names_round_trip

#[test]
fn hershey_mode_replaces_label_text_with_stroked_paths() {
    let mut doc = sample("Arial");
    let report = apply_text_mode_with_fonts(&mut doc, SvgTextMode::HersheyStrokes, &fontdb::Database::new());
    assert_eq!(report.text_elements, 2);
    assert!(report.warnings.is_empty(), "{:?}", report.warnings);
    assert_eq!(label_texts(&mut doc), 0, "no <text> left inside labels");

    let mut groups = Vec::new();
    find_all(&doc.root, "g", &mut groups);
    let strokes: Vec<_> = groups
        .into_iter()
        .filter(|g| g.attributes.get("data-type").map(String::as_str) == Some("label_text"))
        .collect();
    assert_eq!(strokes.len(), 2);

    let front = strokes[0];
    assert_eq!(front.attributes.get("data-text").unwrap(), "Front");
    assert_eq!(front.attributes.get("fill").unwrap(), "none", "strokes, not filled contours");
    assert_eq!(front.attributes.get("stroke").unwrap(), "#112233", "draws in the text color");
    assert_eq!(front.attributes.get("stroke-opacity").unwrap(), "0.5");
    assert_eq!(front.attributes.get("transform").unwrap(), "matrix(1 0 0 1 10 20)");
    let mut descs = Vec::new();
    find_all(front, "desc", &mut descs);
    assert_eq!(text_content(descs[0]), "Front");
    let mut paths = Vec::new();
    find_all(front, "path", &mut paths);
    assert_eq!(paths.len(), 1);
    assert!(!paths[0].attributes.get("d").unwrap().contains(['C', 'Q', 'Z']), "open polylines only");

    // A <text> outside every label group is left alone.
    let mut texts = Vec::new();
    find_all(&doc.root, "text", &mut texts);
    assert_eq!(texts.len(), 1);
    assert_eq!(texts[0].attributes.get("id").unwrap(), "note");
} // hershey_mode_replaces_label_text_with_stroked_paths

#[test]
fn hershey_mode_centres_middle_anchored_text() {
    let mut doc = sample("Arial");
    apply_text_mode_with_fonts(&mut doc, SvgTextMode::HersheyStrokes, &fontdb::Database::new());
    let mut paths = Vec::new();
    find_all(&doc.root, "path", &mut paths);
    // "Size 12" is anchored at x = 0: its first point must lie left of 0.
    let d = paths[1].attributes.get("d").unwrap();
    let first_x: f64 = d[1..].split(' ').next().unwrap().parse().unwrap();
    assert!(first_x < 0.0, "middle anchor shifts the start left: {d}");
} // hershey_mode_centres_middle_anchored_text

#[test]
fn hershey_mode_reports_characters_without_a_glyph() {
    let mut doc = svg_dom::Document::parse(
        r#"<svg xmlns="http://www.w3.org/2000/svg"><g data-type="piece_label"><text x="0" y="0">caf&#233;</text></g></svg>"#,
    )
    .unwrap();
    let report = apply_text_mode_with_fonts(&mut doc, SvgTextMode::HersheyStrokes, &fontdb::Database::new());
    assert_eq!(report.warnings.len(), 1);
    assert!(report.warnings[0].starts_with("1 label character"), "{:?}", report.warnings);
} // hershey_mode_reports_characters_without_a_glyph

#[test]
fn bundled_font_covers_printable_ascii() {
    let face = ttf_parser::Face::parse(RELIEF, 0).expect("bundled font parses");
    let missing: String = (' '..='~').filter(|c| face.glyph_index(*c).is_none()).collect();
    assert!(missing.is_empty(), "missing glyphs: {missing:?}");
} // bundled_font_covers_printable_ascii

#[test]
fn designer_font_mode_embeds_an_installed_font() {
    let family = relief_family();
    let mut doc = sample(&family);
    let report = apply_text_mode_with_fonts(&mut doc, SvgTextMode::DesignerFont, &relief_db());
    assert_eq!(report.text_elements, 2);
    assert!(report.warnings.is_empty(), "{:?}", report.warnings);
    assert_eq!(label_texts(&mut doc), 2, "text stays text");

    let mut styles = Vec::new();
    find_all(&doc.root, "style", &mut styles);
    assert_eq!(styles.len(), 1);
    let css = text_content(styles[0]);
    assert!(css.contains(&format!("font-family: \"{family}\"")), "{css}");
    assert!(css.contains("data:font/otf;base64,"), "CFF outlines are OpenType: {css}");

    // The subset must be smaller than the whole font.
    let b64 = css.split("base64,").nth(1).unwrap().split(')').next().unwrap();
    assert!(b64.len() * 3 / 4 < RELIEF.len());
} // designer_font_mode_embeds_an_installed_font

#[test]
fn designer_font_mode_warns_for_a_missing_font() {
    let mut doc = sample("No Such Font");
    let report = apply_text_mode_with_fonts(&mut doc, SvgTextMode::DesignerFont, &relief_db());
    assert_eq!(report.warnings.len(), 1);
    assert!(report.warnings[0].contains("'No Such Font' is not installed"));
    let mut styles = Vec::new();
    find_all(&doc.root, "style", &mut styles);
    assert!(styles.is_empty(), "nothing to embed");
} // designer_font_mode_warns_for_a_missing_font

/// @brief Copy of the bundled font with OS/2 `fsType` set to "restricted license".
fn restricted_font() -> Vec<u8> {
    let mut data = RELIEF.to_vec();
    let tables = u16::from_be_bytes([data[4], data[5]]) as usize;
    for i in 0..tables {
        let record = 12 + 16 * i;
        if &data[record..record + 4] == b"OS/2" {
            let offset = u32::from_be_bytes(data[record + 8..record + 12].try_into().unwrap()) as usize;
            // fsType is at byte 8 of the OS/2 table; 0x0002 = restricted.
            data[offset + 8..offset + 10].copy_from_slice(&2u16.to_be_bytes());
        } // if OS/2
    } // for table
    data
} // fn restricted_font

#[test]
fn designer_font_mode_never_embeds_a_restricted_font() {
    let mut db = fontdb::Database::new();
    db.load_font_data(restricted_font());
    let mut doc = sample(&relief_family());
    let report = apply_text_mode_with_fonts(&mut doc, SvgTextMode::DesignerFont, &db);
    assert_eq!(report.warnings.len(), 1);
    assert!(report.warnings[0].contains("does not allow embedding"), "{:?}", report.warnings);
    let mut styles = Vec::new();
    find_all(&doc.root, "style", &mut styles);
    assert!(styles.is_empty());
} // designer_font_mode_never_embeds_a_restricted_font

#[test]
fn single_line_mode_sets_and_embeds_the_bundled_font() {
    let mut doc = sample("Arial");
    let report = apply_text_mode_with_fonts(&mut doc, SvgTextMode::SingleLineFont, &fontdb::Database::new());
    assert_eq!(report.text_elements, 2);
    assert!(report.warnings.is_empty(), "{:?}", report.warnings);

    let mut texts = Vec::new();
    find_all(&doc.root, "text", &mut texts);
    let front = texts.iter().find(|t| text_content(t) == "Front").unwrap();
    assert_eq!(front.attributes.get("font-family").unwrap(), SINGLE_LINE_FONT_FAMILY);
    assert_eq!(front.attributes.get("fill").unwrap(), "#112233", "Outline glyphs are filled");
    assert!(front.attributes.get("stroke").is_none());
    let note = texts.iter().find(|t| t.attributes.get("id").map(String::as_str) == Some("note")).unwrap();
    assert!(note.attributes.get("font-family").is_none(), "non-label text untouched");

    let mut styles = Vec::new();
    find_all(&doc.root, "style", &mut styles);
    assert!(text_content(styles[0]).contains(SINGLE_LINE_FONT_FAMILY));
} // single_line_mode_sets_and_embeds_the_bundled_font

#[test]
fn every_mode_keeps_label_group_ids_and_data_attributes() {
    let family = relief_family();
    let before = label_group_attributes(&sample(&family));
    for mode in [SvgTextMode::DesignerFont, SvgTextMode::SingleLineFont, SvgTextMode::HersheyStrokes] {
        let mut doc = sample(&family);
        apply_text_mode_with_fonts(&mut doc, mode, &relief_db());
        assert_eq!(label_group_attributes(&doc), before, "{mode:?}");
    } // for mode
} // every_mode_keeps_label_group_ids_and_data_attributes

#[test]
fn trousers_fixture_converts_every_label_line() {
    let mut doc = svg_dom::Document::parse(TROUSERS).expect("fixture parses");
    assert_eq!(label_text_state(&doc), LabelTextState::Text);
    let lines = label_texts(&mut doc);
    assert!(lines > 0);

    let report = apply_text_mode_with_fonts(&mut doc, SvgTextMode::HersheyStrokes, &fontdb::Database::new());
    assert_eq!(report.text_elements, lines);
    assert_eq!(label_texts(&mut doc), 0);

    // The result still serializes to a parseable SVG.
    let reparsed = svg_dom::Document::parse(&doc.to_string()).expect("output parses");
    assert_eq!(label_text_state(&reparsed), LabelTextState::PathsOnly);
} // trousers_fixture_converts_every_label_line

