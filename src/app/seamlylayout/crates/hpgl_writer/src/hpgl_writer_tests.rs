// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file hpgl_writer_tests.rs
// @brief Unit tests for the HP-GL/1 writer.

use crate::line_classes::{class_from_marker_id, tag_line_classes};
use crate::{svg_to_hpgl, HpglMode, HpglOptions, LineClass, PenMap};

/// @brief Parse an SVG string into a document.
fn doc(svg: &str) -> svg_dom::Document {
    svg_dom::Document::parse(svg).expect("test SVG should parse")
} // fn doc

/// @brief A 96 x 96 px page (1 inch = 1016 plotter units) holding `body`.
fn page(body: &str) -> svg_dom::Document {
    doc(&format!(
        r#"<svg xmlns="http://www.w3.org/2000/svg" width="96" height="96" viewBox="0 0 96 96">{body}</svg>"#
    ))
} // fn page

/// @brief One stroked path inside a group with `data-type`.
fn typed_path(data_type: &str, d: &str) -> String {
    format!(r#"<g data-type="{data_type}"><path d="{d}" stroke="black" fill="none"/></g>"#)
} // fn typed_path

/// @brief Plot `doc` with default options.
fn plot(doc: &svg_dom::Document) -> String {
    svg_to_hpgl(doc, &HpglOptions::default()).expect("plot should succeed")
} // fn plot

/// @brief Index of `needle` in `haystack`; panics when absent.
fn position(haystack: &str, needle: &str) -> usize {
    haystack.find(needle).unwrap_or_else(|| panic!("'{needle}' missing from:\n{haystack}"))
} // fn position

#[test]
fn one_inch_line_is_1016_units_and_y_is_flipped() {
    // y = 0 at the top of a 1-inch page is y = 1016 in HP-GL.
    let program = plot(&page(&typed_path("cutline", "M0 0 L96 0")));
    assert!(program.contains("PU0,1016;"), "{program}");
    assert!(program.contains("PD1016,1016;"), "{program}");
} // one_inch_line_is_1016_units_and_y_is_flipped

#[test]
fn bottom_of_page_is_y_zero() {
    let program = plot(&page(&typed_path("cutline", "M0 96 L96 96")));
    assert!(program.contains("PU0,0;"), "{program}");
    assert!(program.contains("PD1016,0;"), "{program}");
} // bottom_of_page_is_y_zero

#[test]
fn millimetre_page_with_view_box_scales_to_the_physical_size() {
    // 25.4 mm across, whatever the viewBox units are.
    let svg = doc(&format!(
        r#"<svg xmlns="http://www.w3.org/2000/svg" width="25.4mm" height="25.4mm" viewBox="0 0 10 10">{}</svg>"#,
        typed_path("cutline", "M0 10 L10 10")
    ));
    let program = plot(&svg);
    assert!(program.contains("PU0,0;"), "{program}");
    assert!(program.contains("PD1016,0;"), "{program}");
} // millimetre_page_with_view_box_scales_to_the_physical_size

#[test]
fn group_transform_is_applied() {
    let svg = page(r#"<g data-type="cutline" transform="translate(48 0)"><path d="M0 96 L48 96" stroke="black" fill="none"/></g>"#);
    let program = plot(&svg);
    assert!(program.contains("PU508,0;"), "{program}");
    assert!(program.contains("PD1016,0;"), "{program}");
} // group_transform_is_applied

#[test]
fn file_starts_with_reset_and_ends_with_pen_put_away() {
    let program = plot(&page(&typed_path("cutline", "M0 0 L96 0")));
    assert!(program.starts_with("IN;\nPA;\n"), "{program}");
    assert!(program.ends_with("PU;\nSP0;\n"), "{program}");
} // file_starts_with_reset_and_ends_with_pen_put_away

#[test]
fn labels_then_marks_then_cut_lines() {
    let body = [
        typed_path("cutline", "M0 0 L10 0"),
        typed_path("seamline", "M0 10 L10 10"),
        typed_path("piece_label", "M0 20 L10 20"),
    ]
    .concat();
    let program = plot(&page(&body));
    let label = position(&program, "SP3;");
    let mark = position(&program, "SP2;");
    let cut = position(&program, "SP1;");
    assert!(label < mark && mark < cut, "{program}");
} // labels_then_marks_then_cut_lines

#[test]
fn every_mark_type_uses_the_mark_pen() {
    for data_type in ["seamline", "internal_path", "grainline", "notch"] {
        let program = plot(&page(&typed_path(data_type, "M0 0 L10 0")));
        assert!(program.contains("SP2;"), "{data_type}: {program}");
    } // for data_type
} // every_mark_type_uses_the_mark_pen

#[test]
fn cut_path_uses_the_cut_pen() {
    let program = plot(&page(&typed_path("cut_path", "M0 0 L10 0")));
    assert!(program.contains("SP1;"), "{program}");
} // cut_path_uses_the_cut_pen

#[test]
fn chosen_pens_are_written() {
    let body = [typed_path("cutline", "M0 0 L10 0"), typed_path("notch", "M0 10 L10 10")].concat();
    let options = HpglOptions { mode: HpglMode::Plot, pens: PenMap { cut: 5, mark: 7, label: 3 } };
    let program = svg_to_hpgl(&page(&body), &options).unwrap();
    assert!(program.contains("SP5;") && program.contains("SP7;"), "{program}");
    assert!(!program.contains("SP1;") && !program.contains("SP2;"), "{program}");
} // chosen_pens_are_written

#[test]
fn shared_pen_is_selected_once() {
    let body = [typed_path("cutline", "M0 0 L10 0"), typed_path("seamline", "M0 10 L10 10")].concat();
    let options = HpglOptions { mode: HpglMode::Plot, pens: PenMap { cut: 1, mark: 1, label: 1 } };
    let program = svg_to_hpgl(&page(&body), &options).unwrap();
    assert_eq!(program.matches("SP1;").count(), 1, "{program}");
} // shared_pen_is_selected_once

#[test]
fn pen_outside_one_to_eight_is_rejected() {
    let body = typed_path("cutline", "M0 0 L10 0");
    for bad in [0u8, 9u8] {
        let options = HpglOptions { mode: HpglMode::Plot, pens: PenMap { cut: bad, mark: 2, label: 3 } };
        assert!(svg_to_hpgl(&page(&body), &options).is_err(), "pen {bad} accepted");
    } // for bad
} // pen_outside_one_to_eight_is_rejected

#[test]
fn closed_path_returns_to_its_start() {
    let program = plot(&page(&typed_path("cutline", "M0 96 L96 96 L96 0 Z")));
    assert!(program.contains("PU0,0;"), "{program}");
    assert!(program.contains("PD1016,0,1016,1016,0,0;"), "{program}");
} // closed_path_returns_to_its_start

#[test]
fn curve_stays_within_tolerance() {
    // Circle of radius 48 px (508 units) centred on the page.
    let svg = page(r#"<g data-type="cutline"><circle cx="48" cy="48" r="48" stroke="black" fill="none"/></g>"#);
    let program = plot(&svg);
    let coordinates: Vec<i32> = program
        .lines()
        .filter(|line| line.starts_with("PU") || line.starts_with("PD"))
        .flat_map(|line| line[2..line.len() - 1].split(',').filter_map(|n| n.parse().ok()).collect::<Vec<i32>>())
        .collect();
    assert!(coordinates.len() > 16, "circle was not interpolated: {program}");
    for pair in coordinates.chunks(2) {
        let dx = f64::from(pair[0] - 508);
        let dy = f64::from(pair[1] - 508);
        let error = ((dx * dx + dy * dy).sqrt() - 508.0).abs();
        assert!(error <= 3.0, "point {pair:?} is {error} units off the circle");
    } // for pair
} // curve_stays_within_tolerance

#[test]
fn long_polyline_is_split_into_short_pen_down_instructions() {
    let d: String = (0..80).map(|i| format!("{}{} 0 ", if i == 0 { "M" } else { "L" }, i)).collect();
    let program = plot(&page(&typed_path("seamline", d.trim())));
    for line in program.lines().filter(|line| line.starts_with("PD")) {
        assert!(line.matches(',').count() < 64, "PD too long: {line}");
    } // for PD line
} // long_polyline_is_split_into_short_pen_down_instructions

#[test]
fn cut_mode_writes_only_cut_lines() {
    let body = [
        typed_path("cutline", "M0 0 L10 0"),
        typed_path("seamline", "M0 10 L10 10"),
        typed_path("pattern_label", "M0 20 L10 20"),
    ]
    .concat();
    let options = HpglOptions { mode: HpglMode::Cut, pens: PenMap::default() };
    let program = svg_to_hpgl(&page(&body), &options).unwrap();
    assert!(program.contains("SP1;"), "{program}");
    assert!(!program.contains("SP2;") && !program.contains("SP3;"), "{program}");
} // cut_mode_writes_only_cut_lines

#[test]
fn cut_mode_without_cut_lines_is_an_error() {
    let options = HpglOptions { mode: HpglMode::Cut, pens: PenMap::default() };
    let error = svg_to_hpgl(&page(&typed_path("seamline", "M0 0 L10 0")), &options).unwrap_err();
    assert!(error.contains("no cut lines"), "{error}");
} // cut_mode_without_cut_lines_is_an_error

#[test]
fn empty_layout_is_an_error() {
    assert!(svg_to_hpgl(&page(""), &HpglOptions::default()).is_err());
} // empty_layout_is_an_error

#[test]
fn invisible_path_is_not_plotted() {
    let svg = page(r#"<g data-type="cutline"><path d="M0 0 L10 0" stroke="none" fill="none"/></g>"#);
    assert!(svg_to_hpgl(&svg, &HpglOptions::default()).is_err());
} // invisible_path_is_not_plotted

#[test]
fn nearest_polyline_is_plotted_next() {
    // Far line listed first; the pen starts at 0,0 and takes the near one first.
    let body = [typed_path("seamline", "M48 96 L96 96"), typed_path("seamline", "M0 96 L5 96")].concat();
    let program = plot(&page(&body));
    assert!(position(&program, "PU0,0;") < position(&program, "PU508,0;"), "{program}");
} // nearest_polyline_is_plotted_next

#[test]
fn untagged_file_classifies_by_id_then_by_shape() {
    let body = r#"
        <g id="notch_Front"><path d="M0 0 L10 0" stroke="black" fill="none"/></g>
        <g id="Front"><path d="M0 50 L10 50 L10 60 Z" stroke="black" fill="none"/>
                      <path d="M20 50 L30 50" stroke="black" fill="none"/></g>"#;
    let options = HpglOptions { mode: HpglMode::Plot, pens: PenMap { cut: 4, mark: 5, label: 6 } };
    let program = svg_to_hpgl(&page(body), &options).unwrap();
    // The notch and the open line are marks; the closed outline is a cut line.
    assert!(program.contains("SP5;") && program.contains("SP4;"), "{program}");
    let marks = &program[position(&program, "SP5;")..position(&program, "SP4;")];
    assert_eq!(marks.matches("PU").count(), 2, "{program}");
} // untagged_file_classifies_by_id_then_by_shape

#[test]
fn tagging_keeps_original_ids_and_nearest_class_wins() {
    let mut svg = page(
        r#"<g id="piece_A" data-type="piece">
             <g id="piece_label_1_A" data-type="piece_label"><g id="t" data-type="label_text"/></g>
           </g>"#,
    );
    tag_line_classes(&mut svg.root);
    let text = svg.to_string();
    assert!(text.contains(r#"id="piece_A""#) && text.contains(r#"id="piece_label_1_A""#), "{text}");
    assert_eq!(text.matches("hpgl-class-label-").count(), 2, "{text}");
    assert!(!text.contains("hpgl-class-cut-") && !text.contains("hpgl-class-mark-"), "{text}");
} // tagging_keeps_original_ids_and_nearest_class_wins

#[test]
fn marker_ids_round_trip() {
    assert_eq!(class_from_marker_id("hpgl-class-cut-3"), Some(LineClass::Cut));
    assert_eq!(class_from_marker_id("hpgl-class-mark-12"), Some(LineClass::Mark));
    assert_eq!(class_from_marker_id("hpgl-class-label-1"), Some(LineClass::Label));
    assert_eq!(class_from_marker_id("cutline_Front"), None);
    assert_eq!(class_from_marker_id("hpgl-class-bogus-1"), None);
} // marker_ids_round_trip

#[test]
fn mode_names_parse() {
    assert_eq!(HpglMode::from_name("plot"), Some(HpglMode::Plot));
    assert_eq!(HpglMode::from_name("cut"), Some(HpglMode::Cut));
    assert_eq!(HpglMode::from_name("Plot"), None);
} // mode_names_parse
