// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file part_label_placement.rs
// @brief Size and place the new label of one part of a split piece.
//
// The label keeps its full font size when it can. Order of preference:
//   1. Along the longest cut edge, reading up, when the label is not wider than the edge.
//   2. Level, across the part, in the clear place nearest the cut edge. Among equally
//      near places, the one nearest the middle of the edge wins.
//   3. Along the cut edge, with a smaller font and `textLength` equal to the edge length.
// A clear place lies inside the outline and keeps a quarter font from every other path
// and every other text of the part.
// The single-stroke font measures all text. It needs no system font, and its lower-case
// letters and spaces are wider than Arial's, so the measure leaves room to spare.

use geometry::{Path, PathSegment};
use xmltree::{Element, XMLNode};

// Gap between a level label and any other path or text, in font sizes; also the row step.
const CLEARANCE_EM: f64 = 0.25;
// Distance from the cut line to the centre line of a label along the edge, in font sizes.
const EDGE_OFFSET_EM: f64 = 1.5;
// Distance from the cut line to the near end of a level label, in font sizes.
const LEVEL_START_EM: f64 = 0.5;

// @brief A stretch of a vertical cut line on a part's outline.
#[derive(Debug, Clone, Copy, PartialEq)]
pub(crate) struct CutEdge {
    // x of the cut line.
    pub x: f64,
    // Low and high y of the edge.
    pub lo: f64,
    pub hi: f64,
    // +1 when the part lies right of the cut line, -1 when it lies left.
    pub inward: f64,
}

impl CutEdge {
    // @brief Length of the edge.
    fn length(&self) -> f64 {
        self.hi - self.lo
    } // fn length
} // impl CutEdge

// @brief Where a part label goes and how large it is.
#[derive(Debug, Clone, PartialEq)]
pub(crate) struct PartLabelPlace {
    // Centre of the label text.
    pub centre: (f64, f64),
    // True: the label reads up along the cut edge (`rotate(-90)`). False: it is level.
    pub along_edge: bool,
    // Font size in user units.
    pub font: f64,
    // `textLength` when the label shrank to the cut edge, else None.
    pub text_length: Option<f64>,
}

// @brief Size and place the label of one part.
// @param text Label text.
// @param font Full font size, in user units.
// @param outline Part outline: a closed polygon.
// @param edge Longest cut edge of the part; None when the part has no cut edge.
// @param others The part's other components; a level label keeps clear of their paths and text.
// @return Centre, direction, font size and `textLength` of the label.
pub(crate) fn place_part_label(
    text: &str,
    font: f64,
    outline: &[(f64, f64)],
    edge: Option<CutEdge>,
    others: &[Element],
) -> PartLabelPlace {
    let width = svg_label_text::stroke_text_width(text, font);
    match edge {
        None => {
            // No cut edge: centre of the part, full size.
            let centre = Bounds::around(outline.iter().copied())
                .map_or((0.0, 0.0), |b| ((b.left + b.right) / 2.0, (b.top + b.bottom) / 2.0));
            PartLabelPlace { centre, along_edge: false, font, text_length: None }
        } // None
        Some(edge) if width <= edge.length() => {
            // The label fits the edge: along it, full size.
            along_edge(edge, font, None)
        } // fits the edge
        Some(edge) => {
            // Too wide for the edge: level across the part, else smaller along the edge.
            let obstacles = Obstacles::of(outline, others);
            match level_place(width, font, outline, edge, &obstacles) {
                Some(centre) => PartLabelPlace { centre, along_edge: false, font, text_length: None }, // clear place
                None => along_edge(edge, font * edge.length() / width, Some(edge.length())), // shrink to the edge
            } // match a clear level place
        } // too wide for the edge
    } // match edge
} // fn place_part_label

// @brief Label reading up along `edge`, its centre line `EDGE_OFFSET_EM` fonts inside the part.
fn along_edge(edge: CutEdge, font: f64, text_length: Option<f64>) -> PartLabelPlace {
    let centre = (edge.x + edge.inward * EDGE_OFFSET_EM * font, (edge.lo + edge.hi) / 2.0);
    PartLabelPlace { centre, along_edge: true, font, text_length }
} // fn along_edge

// @brief Centre of the clear level place nearest the cut edge; None when the part has none.
//
// Each row is a band as high as the label plus its clearance. Rows step a quarter font
// from the middle of the edge outward. Distances run from the cut line into the part.
fn level_place(
    width: f64,
    font: f64,
    outline: &[(f64, f64)],
    edge: CutEdge,
    obstacles: &Obstacles,
) -> Option<(f64, f64)> {
    let clearance = CLEARANCE_EM * font;
    let half_height = font / 2.0 + clearance;
    let span = width + 2.0 * clearance; // label plus clearance at both ends
    let start = LEVEL_START_EM * font - clearance; // near end of the span
    let x_at = |distance: f64| edge.x + edge.inward * (distance + span / 2.0); // span centre
    let middle = (edge.lo + edge.hi) / 2.0;
    let rows = Bounds::around(outline.iter().copied())
        .map(|b| row_centres(middle, b.top + half_height, b.bottom - half_height, clearance))
        .unwrap_or_default();
    rows.into_iter()
        .filter_map(|y| {
            // Nearest clear start in this row; the span centre must lie inside the outline.
            let blocked = obstacles.blocked_in_band(y - half_height, y + half_height, edge);
            first_clear_start(&blocked, start, span, |distance| inside(outline, (x_at(distance), y)))
                .map(|distance| (distance, y))
        })
        .min_by(|a, b| a.0.total_cmp(&b.0)) // first of equals: the row nearest the middle
        .map(|(distance, y)| (x_at(distance), y))
} // fn level_place

// @brief Row centres from `middle` outward in steps of `step`, inside `low`..=`high`.
fn row_centres(middle: f64, low: f64, high: f64, step: f64) -> Vec<f64> {
    let reach = (middle - low).max(high - middle).max(0.0);
    let steps = (reach / step).ceil() as usize;
    (0..=steps)
        .flat_map(|k| {
            let offset = k as f64 * step;
            if k == 0 {
                vec![middle] // the middle row once
            } else {
                vec![middle + offset, middle - offset] // one row below, one above
            } // if middle row
        })
        .filter(|y| (low..=high).contains(y))
        .collect()
} // fn row_centres

// @brief Smallest start at or past `start` whose span is clear and whose centre is inside.
//
// Candidates: `start`, and the far end of each blocked range past it.
// @param blocked Distance ranges from the cut line that obstacles cover in this row.
// @param span Length of the label plus its clearance.
// @param centre_inside True when the span starting at a distance has its centre inside the outline.
fn first_clear_start(
    blocked: &[(f64, f64)],
    start: f64,
    span: f64,
    centre_inside: impl Fn(f64) -> bool,
) -> Option<f64> {
    let mut starts: Vec<f64> = blocked.iter().map(|range| range.1).filter(|&end| end > start).collect();
    starts.push(start);
    starts.sort_by(f64::total_cmp);
    starts
        .into_iter()
        .find(|&s| blocked.iter().all(|&(lo, hi)| hi <= s || lo >= s + span) && centre_inside(s))
} // fn first_clear_start

// @brief True when `p` lies inside the closed polygon `outline` (even-odd rule).
fn inside(outline: &[(f64, f64)], p: (f64, f64)) -> bool {
    let n = outline.len();
    let crossings = (0..n)
        .filter(|&i| {
            // A ray from p toward +x crosses this edge.
            let (a, b) = (outline[i], outline[(i + 1) % n]);
            (a.1 > p.1) != (b.1 > p.1) && p.0 < a.0 + (p.1 - a.1) / (b.1 - a.1) * (b.0 - a.0)
        })
        .count();
    crossings % 2 == 1
} // fn inside

// @brief x range of the part of segment `a`-`b` inside the band `top`..=`bottom`; None when it misses.
fn x_range_in_band(a: (f64, f64), b: (f64, f64), top: f64, bottom: f64) -> Option<(f64, f64)> {
    let dy = b.1 - a.1;
    // Parameter range t in 0..=1 where a + t (b - a) lies in the band.
    let (t0, t1) = if dy.abs() < 1e-12 {
        if (top..=bottom).contains(&a.1) { (0.0, 1.0) } else { (1.0, 0.0) } // level: all or nothing
    } else {
        // Sloped: clip t to the band.
        let (ta, tb) = ((top - a.1) / dy, (bottom - a.1) / dy);
        (ta.min(tb).max(0.0), ta.max(tb).min(1.0))
    }; // if level
    (t0 <= t1).then(|| {
        let (xa, xb) = (a.0 + t0 * (b.0 - a.0), a.0 + t1 * (b.0 - a.0));
        (xa.min(xb), xa.max(xb))
    })
} // fn x_range_in_band

// @brief Axis-aligned box.
#[derive(Debug, Clone, Copy, PartialEq)]
struct Bounds {
    left: f64,
    top: f64,
    right: f64,
    bottom: f64,
}

impl Bounds {
    // @brief Smallest box around `points`; None when there are none.
    fn around(points: impl IntoIterator<Item = (f64, f64)>) -> Option<Bounds> {
        points.into_iter().fold(None, |bounds: Option<Bounds>, (x, y)| {
            Some(match bounds {
                Some(b) => Bounds {
                    left: b.left.min(x),
                    top: b.top.min(y),
                    right: b.right.max(x),
                    bottom: b.bottom.max(y),
                }, // widen the box
                None => Bounds { left: x, top: y, right: x, bottom: y }, // first point
            }) // match bounds so far
        })
    } // fn around
} // impl Bounds

// @brief Presentation attributes a `<text>` takes from itself and its ancestors.
#[derive(Debug, Clone, Default)]
struct TextStyle {
    font_size: Option<String>,
    anchor: Option<String>,
    baseline: Option<String>,
}

impl TextStyle {
    // @brief Style of `element`: its own attributes override the inherited ones.
    fn with(&self, element: &Element) -> TextStyle {
        let own = |name: &str, inherited: &Option<String>| {
            element.attributes.get(name).cloned().or_else(|| inherited.clone())
        };
        TextStyle {
            font_size: own("font-size", &self.font_size),
            anchor: own("text-anchor", &self.anchor),
            baseline: own("dominant-baseline", &self.baseline),
        }
    } // fn with

    // @brief Font size in user units; 12 when absent or not a plain number.
    fn font_size(&self) -> f64 {
        self.font_size
            .as_deref()
            .map(|s| s.trim().trim_end_matches("px"))
            .and_then(|s| s.parse::<f64>().ok())
            .filter(|size| *size > 0.0)
            .unwrap_or(12.0)
    } // fn font_size
} // impl TextStyle

// @brief Lines and boxes a level label must keep clear of.
#[derive(Debug, Default)]
struct Obstacles {
    // Outline edges and the lines of straight-line paths.
    segments: Vec<((f64, f64), (f64, f64))>,
    // Text, and curved paths.
    boxes: Vec<Bounds>,
}

impl Obstacles {
    // @brief The outline edges plus every path and text of `others`.
    fn of(outline: &[(f64, f64)], others: &[Element]) -> Obstacles {
        let mut obstacles = Obstacles::default();
        let n = outline.len();
        obstacles.segments.extend((0..n).map(|i| (outline[i], outline[(i + 1) % n])));
        for element in others {
            obstacles.add(element, &TextStyle::default());
        } // for each component
        obstacles
    } // fn of

    // @brief Add the paths and text of `element` and its descendants.
    // @param inherited Text style of the ancestors.
    fn add(&mut self, element: &Element, inherited: &TextStyle) {
        let style = inherited.with(element);
        match element.name.as_str() {
            "path" => self.add_path(element.attributes.get("d").map_or("", String::as_str)), // lines or a box
            "text" => self.add_text(element, &style), // one box per text
            _ => {
                // Container: look inside.
                for child in element.children.iter().filter_map(XMLNode::as_element) {
                    self.add(child, &style);
                } // for each child
            } // container
        } // match element name
    } // fn add

    // @brief Add the lines of a straight-line path, or the bounding box of a curved one.
    fn add_path(&mut self, d: &str) {
        if let Ok(path) = Path::parse_path_attribute(d) {
            let curved = path.segments.iter().any(|segment| {
                matches!(segment, PathSegment::QuadTo { .. } | PathSegment::CubicTo { .. } | PathSegment::ArcTo { .. })
            });
            if curved {
                // Control points included: the box covers the curve.
                self.boxes.extend(path.bounding_box().map(|b| Bounds {
                    left: f64::from(b.min.x),
                    top: f64::from(b.min.y),
                    right: f64::from(b.max.x),
                    bottom: f64::from(b.max.y),
                }));
            } else {
                // Straight lines: each one exactly.
                self.add_lines(&path);
            } // if curved
        } // if the path parses
    } // fn add_path

    // @brief Add each line of a straight-line path; `Z` adds the line back to the subpath start.
    fn add_lines(&mut self, path: &Path) {
        let xy = |p: geometry::Point| (f64::from(p.x), f64::from(p.y));
        let mut start: Option<(f64, f64)> = None;
        let mut pen: Option<(f64, f64)> = None;
        for segment in &path.segments {
            match *segment {
                PathSegment::MoveTo(p) => {
                    // New subpath: no line.
                    start = Some(xy(p));
                    pen = start;
                } // MoveTo
                PathSegment::LineTo(p) => {
                    // Line from the pen.
                    self.segments.extend(pen.map(|from| (from, xy(p))));
                    pen = Some(xy(p));
                } // LineTo
                PathSegment::Close => {
                    // Line back to the subpath start.
                    self.segments.extend(pen.zip(start));
                    pen = start;
                } // Close
                _ => {} // curves: not in a straight-line path
            } // match segment
        } // for each segment
    } // fn add_lines

    // @brief Add the box a `<text>` covers: its single-stroke width, cap height and descenders.
    fn add_text(&mut self, text: &Element, style: &TextStyle) {
        let size = style.font_size();
        let width = svg_label_text::stroke_text_width(&text_content(text), size);
        if width > 0.0 {
            let number = |name: &str| {
                text.attributes
                    .get(name)
                    .and_then(|v| v.split([' ', ',']).find(|s| !s.is_empty()).and_then(|s| s.parse::<f64>().ok()))
                    .unwrap_or(0.0)
            };
            let (x, y) = (number("x"), number("y"));
            let left = match style.anchor.as_deref() {
                Some("middle") => x - width / 2.0, // centred on x
                Some("end") => x - width,          // ends at x
                _ => x,                            // starts at x
            }; // match text-anchor
            let (top, bottom) = if style.baseline.as_deref() == Some("middle") {
                (y - size / 2.0, y + size / 2.0) // centred on y
            } else {
                (y - size, y + size / 4.0) // baseline at y
            }; // if centred on y
            // The box through the text's own transform; a turned text gets the box around it.
            let matrix = svg_dom::parse_svg_transform(text.attributes.get("transform").map_or("", String::as_str));
            let corners = [(left, top), (left + width, top), (left, bottom), (left + width, bottom)].map(|(cx, cy)| {
                let p = matrix.apply_to_point(geometry::Point::new(cx as f32, cy as f32));
                (f64::from(p.x), f64::from(p.y))
            });
            self.boxes.extend(Bounds::around(corners));
        } // if the text draws anything
    } // fn add_text

    // @brief Distance ranges from the cut line that obstacles cover inside the band `top`..=`bottom`.
    fn blocked_in_band(&self, top: f64, bottom: f64, edge: CutEdge) -> Vec<(f64, f64)> {
        let distance = |x: f64| edge.inward * (x - edge.x);
        let lines = self.segments.iter().filter_map(|&(a, b)| x_range_in_band(a, b, top, bottom));
        let boxes = self.boxes.iter().filter(|b| b.top <= bottom && b.bottom >= top).map(|b| (b.left, b.right));
        lines
            .chain(boxes)
            .map(|(x0, x1)| {
                let (d0, d1) = (distance(x0), distance(x1));
                (d0.min(d1), d0.max(d1))
            })
            .collect()
    } // fn blocked_in_band
} // impl Obstacles

// @brief Character content of `element` and its descendants (`<tspan>` included).
fn text_content(element: &Element) -> String {
    element
        .children
        .iter()
        .map(|child| match child {
            XMLNode::Text(t) | XMLNode::CData(t) => t.clone(), // literal text
            XMLNode::Element(e) => text_content(e),            // nested <tspan>
            _ => String::new(),                                // comments, processing instructions
        })
        .collect()
} // fn text_content

#[cfg(test)]
mod tests {
    use super::*;

    // Full font size: 0.25 in at 96 user units per inch.
    const FONT: f64 = 24.0;
    const TEXT: &str = "Piece_4_Union_piece part 1 of 2, joins 2";

    // @brief Closed rectangle outline.
    fn rectangle(left: f64, top: f64, right: f64, bottom: f64) -> Vec<(f64, f64)> {
        vec![(left, top), (right, top), (right, bottom), (left, bottom)]
    } // fn rectangle

    // @brief Assert two points are equal within rounding.
    fn assert_near(actual: (f64, f64), expected: (f64, f64)) {
        let near = (actual.0 - expected.0).abs() < 1e-9 && (actual.1 - expected.1).abs() < 1e-9;
        assert!(near, "{actual:?} is not {expected:?}");
    } // fn assert_near

    // @brief Components parsed from SVG markup.
    fn components(markup: &str) -> Vec<Element> {
        let root = Element::parse(format!("<g>{markup}</g>").as_bytes()).expect("markup parses");
        root.children.into_iter().filter_map(|n| n.as_element().cloned()).collect()
    } // fn components

    // @brief A label that fits its cut edge reads along it at full size.
    #[test]
    fn label_that_fits_reads_along_the_edge() {
        let edge = CutEdge { x: 200.0, lo: 0.0, hi: 2000.0, inward: -1.0 };
        let place = place_part_label(TEXT, FONT, &rectangle(0.0, 0.0, 200.0, 2000.0), Some(edge), &[]);
        let expected = PartLabelPlace { centre: (164.0, 1000.0), along_edge: true, font: FONT, text_length: None };
        assert_eq!(place, expected);
    } // label_that_fits_reads_along_the_edge

    // @brief A label wider than a short cut edge lies level beside it, at full size,
    // half a font from the cut line, on the middle of the edge.
    #[test]
    fn short_edge_puts_the_label_level_beside_it() {
        let width = svg_label_text::stroke_text_width(TEXT, FONT);
        assert!(width > 227.0, "the label is wider than the edge");
        let outline = rectangle(0.0, 0.0, 1000.0, 227.0);
        let right_edge = CutEdge { x: 1000.0, lo: 0.0, hi: 227.0, inward: -1.0 };
        let place = place_part_label(TEXT, FONT, &outline, Some(right_edge), &[]);
        assert!(!place.along_edge && place.font == FONT && place.text_length.is_none(), "{place:?}");
        assert_near(place.centre, (1000.0 - 12.0 - width / 2.0, 113.5));
        let left_edge = CutEdge { x: 0.0, lo: 0.0, hi: 227.0, inward: 1.0 };
        assert_near(place_part_label(TEXT, FONT, &outline, Some(left_edge), &[]).centre, (12.0 + width / 2.0, 113.5));
    } // short_edge_puts_the_label_level_beside_it

    // @brief A level label keeps clear of other text and lines: here it goes below a
    // label block that fills the middle rows beside the cut edge, inside a seam line.
    #[test]
    fn level_label_keeps_clear_of_other_text_and_lines() {
        let others = components(
            r#"<g data-type="seamline"><path d="M1000,38 L38,38 L38,189 L1000,189"/></g>
            <g data-type="pattern_label" font-size="18">
              <text x="0" y="19" transform="matrix(1 0 0 1 700 38)">trousers.sm2d, trousers.smis</text>
              <text x="0" y="19" transform="matrix(1 0 0 1 830 62)">11-12-2026</text>
              <text x="0" y="19" transform="matrix(1 0 0 1 900 110)">Cut 1</text>
            </g>"#,
        );
        let edge = CutEdge { x: 1000.0, lo: 0.0, hi: 227.0, inward: -1.0 };
        let place = place_part_label(TEXT, FONT, &rectangle(0.0, 0.0, 1000.0, 227.0), Some(edge), &others);
        let width = svg_label_text::stroke_text_width(TEXT, FONT);
        assert!(!place.along_edge && place.font == FONT && place.text_length.is_none(), "{place:?}");
        // Below the last text line (bottom 110 + 19 + 4.5) and above the seam line.
        assert!(place.centre.1 - FONT / 2.0 > 133.5 && place.centre.1 + FONT / 2.0 < 189.0, "{place:?}");
        // Still half a font from the cut line.
        assert_near((place.centre.0, 0.0), (1000.0 - 12.0 - width / 2.0, 0.0));
    } // level_label_keeps_clear_of_other_text_and_lines

    // @brief With no clear level place, the label shrinks to the cut edge.
    #[test]
    fn no_level_place_shrinks_the_label_to_the_edge() {
        let edge = CutEdge { x: 300.0, lo: 0.0, hi: 100.0, inward: -1.0 };
        let place = place_part_label(TEXT, FONT, &rectangle(0.0, 0.0, 300.0, 100.0), Some(edge), &[]);
        assert!(place.along_edge && place.font < FONT, "{place:?}");
        assert_eq!(place.text_length, Some(100.0));
        assert!((svg_label_text::stroke_text_width(TEXT, place.font) - 100.0).abs() < 1e-9);
    } // no_level_place_shrinks_the_label_to_the_edge

    // @brief A curved path blocks its bounding box; a straight one blocks its lines.
    #[test]
    fn paths_become_lines_or_boxes() {
        let others = components(r#"<path d="M0,0 C10,10 20,10 30,0"/><path d="M0,50 L40,50 L40,60 Z"/>"#);
        let obstacles = Obstacles::of(&[], &others);
        assert_eq!(obstacles.boxes, vec![Bounds { left: 0.0, top: 0.0, right: 30.0, bottom: 10.0 }]);
        let lines = vec![((0.0, 50.0), (40.0, 50.0)), ((40.0, 50.0), (40.0, 60.0)), ((40.0, 60.0), (0.0, 50.0))];
        assert_eq!(obstacles.segments, lines);
    } // paths_become_lines_or_boxes
} // mod tests
