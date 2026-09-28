// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file ps_writer_tests.rs
// @brief Unit tests for the PostScript and EPS writer.

use crate::{num, svg_to_postscript, PostScriptOutput, PsFlavor};

/// @brief A 96 x 192 px page (1 x 2 inch = 72 x 144 pt) holding `body`.
fn page(body: &str) -> svg_dom::Document {
    svg_dom::Document::parse(&format!(
        r#"<svg xmlns="http://www.w3.org/2000/svg" width="96" height="192" viewBox="0 0 96 192">{body}</svg>"#
    ))
    .expect("test SVG should parse")
} // fn page

/// @brief Convert `body` on the test page; panics on failure.
fn convert(body: &str, flavor: PsFlavor) -> PostScriptOutput {
    svg_to_postscript(&page(body), flavor).expect("conversion should succeed")
} // fn convert

const LINE: &str = r#"<path d="M0 0 L96 0" stroke="black" fill="none"/>"#;

#[test]
fn ps_header_sets_the_page_size_in_points() {
    let ps = convert(LINE, PsFlavor::Ps).program;
    assert!(ps.starts_with("%!PS-Adobe-3.0\n"), "{ps}");
    assert!(ps.contains("%%BoundingBox: 0 0 72 144\n"), "{ps}");
    assert!(ps.contains("<< /PageSize [72 144] >> setpagedevice\n"), "{ps}");
    assert!(ps.contains("showpage\n"), "{ps}");
    assert!(ps.ends_with("%%EOF\n"), "{ps}");
} // ps_header_sets_the_page_size_in_points

#[test]
fn eps_header_has_bounding_box_and_no_page_device() {
    let eps = convert(LINE, PsFlavor::Eps).program;
    assert!(eps.starts_with("%!PS-Adobe-3.0 EPSF-3.0\n"), "{eps}");
    assert!(eps.contains("%%BoundingBox: 0 0 72 144\n"), "{eps}");
    assert!(eps.contains("%%HiResBoundingBox: 0 0 72 144\n"), "{eps}");
    assert!(!eps.contains("setpagedevice"), "EPS must not set the page device:\n{eps}");
} // eps_header_has_bounding_box_and_no_page_device

#[test]
fn page_matrix_flips_y_and_scales_px_to_points() {
    let ps = convert(LINE, PsFlavor::Ps).program;
    assert!(ps.contains("0 144 translate\n0.75 -0.75 scale\n"), "{ps}");
} // page_matrix_flips_y_and_scales_px_to_points

#[test]
fn bounding_box_rounds_up_a_fractional_page() {
    let doc = svg_dom::Document::parse(
        r#"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="50"><path d="M0 0 L10 10" stroke="black"/></svg>"#,
    )
    .unwrap();
    let eps = svg_to_postscript(&doc, PsFlavor::Eps).unwrap().program;
    // 100 px = 75 pt, 50 px = 37.5 pt.
    assert!(eps.contains("%%BoundingBox: 0 0 75 38\n"), "{eps}");
    assert!(eps.contains("%%HiResBoundingBox: 0 0 75 37.5\n"), "{eps}");
} // bounding_box_rounds_up_a_fractional_page

#[test]
fn path_keeps_its_transform_and_local_coordinates() {
    let ps = convert(r#"<g transform="translate(10 20)"><path d="M0 0 L5 0" stroke="black"/></g>"#, PsFlavor::Ps).program;
    assert!(ps.contains("[1 0 0 1 10 20] concat\n0 0 m\n5 0 l\n"), "{ps}");
} // path_keeps_its_transform_and_local_coordinates

#[test]
fn cubic_curve_is_written_as_curveto() {
    let ps = convert(r#"<path d="M0 0 C10 0 20 10 30 30" stroke="black" fill="none"/>"#, PsFlavor::Ps).program;
    assert!(ps.contains("10 0 20 10 30 30 c\n"), "{ps}");
} // cubic_curve_is_written_as_curveto

#[test]
fn quadratic_curve_becomes_the_equal_cubic() {
    // Q control (30,0) from (0,0) to (30,30): cubic controls (20,0) and (30,10).
    let ps = convert(r#"<path d="M0 0 Q30 0 30 30" stroke="black" fill="none"/>"#, PsFlavor::Ps).program;
    assert!(ps.contains("20 0 30 10 30 30 c\n"), "{ps}");
} // quadratic_curve_becomes_the_equal_cubic

#[test]
fn closed_path_uses_closepath() {
    let ps = convert(r#"<path d="M0 0 L10 0 L10 10 Z" stroke="black" fill="none"/>"#, PsFlavor::Ps).program;
    assert!(ps.contains("10 10 l\nh\n"), "{ps}");
} // closed_path_uses_closepath

#[test]
fn fill_rule_selects_fill_or_eofill() {
    let nonzero = convert(r#"<rect width="10" height="10" fill="red"/>"#, PsFlavor::Ps).program;
    assert!(nonzero.contains("1 0 0 setrgbcolor\nfill\n"), "{nonzero}");
    let evenodd = convert(r##"<rect width="10" height="10" fill="#00ff00" fill-rule="evenodd"/>"##, PsFlavor::Ps).program;
    assert!(evenodd.contains("0 1 0 setrgbcolor\neofill\n"), "{evenodd}");
} // fill_rule_selects_fill_or_eofill

#[test]
fn fill_then_stroke_keeps_the_path_for_the_stroke() {
    let ps = convert(r#"<rect width="10" height="10" fill="red" stroke="blue" stroke-width="2"/>"#, PsFlavor::Ps).program;
    assert!(
        ps.contains("gsave\n1 0 0 setrgbcolor\nfill\ngrestore\n0 0 1 setrgbcolor\n2 setlinewidth\n"),
        "{ps}"
    );
} // fill_then_stroke_keeps_the_path_for_the_stroke

#[test]
fn stroke_style_is_written_when_not_default() {
    let ps = convert(
        r#"<path d="M0 0 L10 0" stroke="black" stroke-linecap="round" stroke-linejoin="bevel" stroke-dasharray="4 2" stroke-dashoffset="1"/>"#,
        PsFlavor::Ps,
    )
    .program;
    assert!(ps.contains("1 setlinecap\n"), "{ps}");
    assert!(ps.contains("2 setlinejoin\n"), "{ps}");
    assert!(ps.contains("[4 2] 1 setdash\n"), "{ps}");
} // stroke_style_is_written_when_not_default

#[test]
fn svg_default_miter_limit_is_written() {
    // SVG's default limit is 4; PostScript's is 10.
    let ps = convert(LINE, PsFlavor::Ps).program;
    assert!(ps.contains("4 setmiterlimit\n"), "{ps}");
} // svg_default_miter_limit_is_written

#[test]
fn exact_output_has_no_warnings() {
    assert!(convert(LINE, PsFlavor::Eps).warnings.is_empty());
} // exact_output_has_no_warnings

#[test]
fn transparency_and_gradients_are_reported() {
    let body = r##"<defs><linearGradient id="g"><stop offset="0" stop-color="#0000ff"/><stop offset="1" stop-color="red"/></linearGradient></defs>
                  <rect width="10" height="10" fill="url(#g)" fill-opacity="0.5"/>"##;
    let out = convert(body, PsFlavor::Ps);
    assert!(out.program.contains("0 0 1 setrgbcolor\nfill\n"), "first stop color:\n{}", out.program);
    assert_eq!(out.warnings.len(), 2, "{:?}", out.warnings);
    assert!(out.warnings[0].contains("transparency"), "{:?}", out.warnings);
    assert!(out.warnings[1].contains("Gradients"), "{:?}", out.warnings);
} // transparency_and_gradients_are_reported

#[test]
fn prolog_definitions_stay_in_a_private_dictionary() {
    let eps = convert(LINE, PsFlavor::Eps).program;
    assert!(eps.contains("/SeamlyLayoutDict 4 dict def\nSeamlyLayoutDict begin\n/m {moveto} bind def"), "{eps}");
    assert!(eps.contains("%%Page: 1 1\nSeamlyLayoutDict begin\n"), "{eps}");
} // prolog_definitions_stay_in_a_private_dictionary

#[test]
fn empty_layout_is_an_error() {
    let err = svg_to_postscript(&page(""), PsFlavor::Eps).unwrap_err();
    assert!(err.starts_with("EPS export:"), "{err}");
} // empty_layout_is_an_error

#[test]
fn invisible_paths_are_not_written() {
    let err = svg_to_postscript(&page(r#"<path d="M0 0 L10 0" stroke="none" fill="none"/>"#), PsFlavor::Ps).unwrap_err();
    assert!(err.contains("nothing to draw"), "{err}");
} // invisible_paths_are_not_written

#[test]
fn output_is_seven_bit_ascii() {
    let ps = convert(LINE, PsFlavor::Ps).program;
    assert!(ps.is_ascii());
} // output_is_seven_bit_ascii

#[test]
fn flavor_names_parse() {
    assert_eq!(PsFlavor::from_name("ps"), Some(PsFlavor::Ps));
    assert_eq!(PsFlavor::from_name("eps"), Some(PsFlavor::Eps));
    assert_eq!(PsFlavor::from_name("pdf"), None);
} // flavor_names_parse

#[test]
fn numbers_are_plain_decimals() {
    assert_eq!(num(1.0), "1");
    assert_eq!(num(0.75), "0.75");
    assert_eq!(num(-0.00001), "0");
    assert_eq!(num(1.0 / 3.0), "0.3333");
    assert_eq!(num(1e7), "10000000");
} // numbers_are_plain_decimals
