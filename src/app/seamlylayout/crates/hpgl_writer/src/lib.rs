// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file lib.rs
// @brief Writes a layout SVG as an HP-GL/1 program for pen plotters and cutters.
//
// Pipeline:
//   1. `line_classes::tag_line_classes` records each group's line class in an id usvg keeps.
//   2. usvg resolves transforms, the viewBox, shapes and arcs into absolute paths.
//   3. `collect_polylines` interpolates curves and converts px to plotter units.
//   4. `hpgl_commands::write_program` orders the polylines and writes HP-GL/1.
//
// Units: 96 px per inch (the layout resolution), 40 plotter units per mm.
// HP-GL puts the origin at the bottom left with Y up, so Y is flipped.

mod hpgl_commands;
mod line_classes;

#[cfg(test)]
mod hpgl_writer_tests;

pub use line_classes::LineClass;

use usvg::tiny_skia_path::{PathSegment, Point};

/// HP-GL plotter units per millimetre (one unit is 0.025 mm).
pub const PLOTTER_UNITS_PER_MM: f64 = 40.0;

/// Layout pixels per millimetre at the 96 ppi layout resolution.
const PX_PER_MM: f64 = 96.0 / 25.4;

/// Highest pen number the pen submenus offer.
pub const MAX_PEN: u8 = 8;

/// Largest distance, in plotter units, between a curve and its interpolated polyline.
const CURVE_TOLERANCE_UNITS: f64 = 2.0;

/// @brief Which lines an HPGL export writes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HpglMode {
    /// All lines, one pen per line class.
    Plot,
    /// Cut lines only, so a cutter does not cut seam lines or labels.
    Cut,
} // enum HpglMode

impl HpglMode {
    /// @brief Parse "plot" or "cut".
    pub fn from_name(name: &str) -> Option<Self> {
        match name {
            "plot" => Some(HpglMode::Plot), // all lines
            "cut" => Some(HpglMode::Cut),   // cut lines only
            _ => None,                      // unknown mode
        } // match name
    } // fn from_name

    /// @brief True when this mode writes lines of `class`.
    fn includes(self, class: LineClass) -> bool {
        match self {
            HpglMode::Plot => true,                     // every class
            HpglMode::Cut => class == LineClass::Cut,   // cut lines only
        } // match self
    } // fn includes
} // impl HpglMode

/// @brief Pen number for each line class.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PenMap {
    /// Pen for cut lines.
    pub cut: u8,
    /// Pen for marks.
    pub mark: u8,
    /// Pen for labels.
    pub label: u8,
} // struct PenMap

impl Default for PenMap {
    fn default() -> Self {
        Self { cut: 1, mark: 2, label: 3 }
    } // fn default
} // impl Default for PenMap

impl PenMap {
    /// @brief Pen that draws `class`.
    pub fn pen_for(&self, class: LineClass) -> u8 {
        match class {
            LineClass::Cut => self.cut,     // cut pen
            LineClass::Mark => self.mark,   // mark pen
            LineClass::Label => self.label, // label pen
        } // match class
    } // fn pen_for

    /// @brief Err when a pen is outside 1..=MAX_PEN; pen 0 means "no pen" in HP-GL.
    pub fn validate(&self) -> Result<(), String> {
        for class in [LineClass::Cut, LineClass::Mark, LineClass::Label] {
            let pen = self.pen_for(class);
            if !(1..=MAX_PEN).contains(&pen) {
                return Err(format!(
                    "HPGL export: the {} pen is {pen}; use a pen from 1 to {MAX_PEN}.",
                    class.name()
                )); // if out of range
            } // if out of range
        } // for class
        Ok(())
    } // fn validate
} // impl PenMap

/// @brief Options for one HPGL export.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct HpglOptions {
    /// Which lines to write.
    pub mode: HpglMode,
    /// Pen for each line class.
    pub pens: PenMap,
} // struct HpglOptions

impl Default for HpglMode {
    fn default() -> Self {
        HpglMode::Plot
    } // fn default
} // impl Default for HpglMode

/// @brief One pen-down stroke in plotter units, Y up.
#[derive(Debug, Clone, PartialEq)]
pub struct PlotPolyline {
    /// Line class; selects the pen.
    pub class: LineClass,
    /// At least two points.
    pub points: Vec<(i32, i32)>,
    /// True when the stroke returns to its first point.
    pub closed: bool,
} // struct PlotPolyline

/// @brief Convert a layout SVG into an HP-GL/1 program.
///
/// The caller converts label text to single strokes first; any `<text>` that
/// remains is plotted as glyph outlines.
/// @return The program text, or Err when the options are invalid, the SVG
///         does not parse, or no line of the chosen mode exists.
pub fn svg_to_hpgl(doc: &svg_dom::Document, options: &HpglOptions) -> Result<String, String> {
    options.pens.validate()?;

    // Record line classes on a copy; the caller's document is not changed.
    let mut tagged = doc.clone();
    line_classes::tag_line_classes(&mut tagged.root);

    let tree = parse_tree(&tagged)?;
    let polylines: Vec<PlotPolyline> = collect_polylines(&tree)
        .into_iter()
        .filter(|p| options.mode.includes(p.class))
        .collect();

    if polylines.is_empty() {
        let what = match options.mode {
            HpglMode::Plot => "no lines to plot",   // empty layout
            HpglMode::Cut => "no cut lines",        // nothing tagged as a cut line
        }; // match mode
        return Err(format!("HPGL export: the layout has {what}."));
    } // if nothing to write

    Ok(hpgl_commands::write_program(&polylines, &options.pens))
} // fn svg_to_hpgl

/// @brief Parse the SVG with usvg; system fonts load only when `<text>` remains.
fn parse_tree(doc: &svg_dom::Document) -> Result<usvg::Tree, String> {
    let svg = doc.to_string();
    let mut options = usvg::Options::default();
    if svg.contains("<text") {
        // Glyph outlines need fonts; loading them is slow, so skip it when possible.
        options.fontdb_mut().load_system_fonts();
    } // if text remains
    usvg::Tree::from_data(svg.as_bytes(), &options).map_err(|e| format!("HPGL export: SVG parse failed: {e}"))
} // fn parse_tree

/// @brief Every visible path of `tree` as polylines in plotter units.
pub fn collect_polylines(tree: &usvg::Tree) -> Vec<PlotPolyline> {
    let mut polylines = Vec::new();
    let page_height_px = f64::from(tree.size().height());
    collect_group(tree.root(), None, page_height_px, &mut polylines);
    polylines
} // fn collect_polylines

/// @brief Walk one group; the nearest marker group sets the class.
fn collect_group(
    group: &usvg::Group,
    inherited: Option<LineClass>,
    page_height_px: f64,
    polylines: &mut Vec<PlotPolyline>,
) {
    let class = line_classes::class_from_marker_id(group.id()).or(inherited);
    for node in group.children() {
        match node {
            usvg::Node::Group(child) => collect_group(child, class, page_height_px, polylines), // recurse
            usvg::Node::Path(path) => collect_path(path, class, page_height_px, polylines),     // geometry
            usvg::Node::Text(text) => collect_group(text.flattened(), class, page_height_px, polylines), // glyph outlines
            usvg::Node::Image(_) => {} // a raster image has no lines
        } // match node
    } // for node
} // fn collect_group

/// @brief Interpolate one usvg path into polylines, one per subpath.
fn collect_path(
    path: &usvg::Path,
    class: Option<LineClass>,
    page_height_px: f64,
    polylines: &mut Vec<PlotPolyline>,
) {
    if !path.is_visible() || (path.fill().is_none() && path.stroke().is_none()) {
        return; // nothing is drawn on screen, so nothing is plotted
    } // if invisible

    let transform = path.abs_transform();
    // Absolute px → plotter units, Y flipped so the origin is bottom left.
    let to_units = |p: Point| -> (f64, f64) {
        let x = f64::from(transform.sx * p.x + transform.kx * p.y + transform.tx);
        let y = f64::from(transform.ky * p.x + transform.sy * p.y + transform.ty);
        let scale = PLOTTER_UNITS_PER_MM / PX_PER_MM;
        (x * scale, (page_height_px - y) * scale)
    };

    let mut current: Vec<(f64, f64)> = Vec::new();
    for segment in path.data().segments() {
        match segment {
            PathSegment::MoveTo(p) => {
                // A new subpath ends the open one.
                push_polyline(&mut current, false, class, polylines);
                current.push(to_units(p));
            } // MoveTo
            PathSegment::LineTo(p) => current.push(to_units(p)), // straight segment
            PathSegment::QuadTo(c, p) => {
                // Quadratic curve: interpolate in plotter units.
                let start = last_or(&current, to_units(c));
                interpolate(&mut current, &[start, to_units(c), to_units(p)]);
            } // QuadTo
            PathSegment::CubicTo(c1, c2, p) => {
                // Cubic curve: interpolate in plotter units.
                let start = last_or(&current, to_units(c1));
                interpolate(&mut current, &[start, to_units(c1), to_units(c2), to_units(p)]);
            } // CubicTo
            PathSegment::Close => push_polyline(&mut current, true, class, polylines), // closed subpath
        } // match segment
    } // for segment
    push_polyline(&mut current, false, class, polylines); // last open subpath
} // fn collect_path

/// @brief Last point of `points`, or `fallback` when empty.
fn last_or(points: &[(f64, f64)], fallback: (f64, f64)) -> (f64, f64) {
    points.last().copied().unwrap_or(fallback)
} // fn last_or

/// @brief Append the points of a Bézier curve after its start point.
///
/// Wang's formula gives the segment count that keeps the polyline within
/// `CURVE_TOLERANCE_UNITS` of the curve.
/// @param control Start point, control points, end point (3 for quadratic, 4 for cubic).
fn interpolate(points: &mut Vec<(f64, f64)>, control: &[(f64, f64)]) {
    let degree = control.len() - 1;
    // Largest second difference of the control polygon.
    let second_difference = control
        .windows(3)
        .map(|w| {
            let dx = w[0].0 - 2.0 * w[1].0 + w[2].0;
            let dy = w[0].1 - 2.0 * w[1].1 + w[2].1;
            (dx * dx + dy * dy).sqrt()
        })
        .fold(0.0, f64::max);
    let factor = (degree * (degree - 1)) as f64 / 8.0;
    let segments = ((factor * second_difference / CURVE_TOLERANCE_UNITS).sqrt().ceil() as usize).max(1);

    for step in 1..=segments {
        let t = step as f64 / segments as f64;
        points.push(bezier_point(control, t));
    } // for step
} // fn interpolate

/// @brief Point at `t` on a Bézier curve (de Casteljau).
fn bezier_point(control: &[(f64, f64)], t: f64) -> (f64, f64) {
    let mut level: Vec<(f64, f64)> = control.to_vec();
    while level.len() > 1 {
        // Each pass blends neighbours until one point is left.
        level = level
            .windows(2)
            .map(|w| (w[0].0 + (w[1].0 - w[0].0) * t, w[0].1 + (w[1].1 - w[0].1) * t))
            .collect();
    } // while more than one point
    level[0]
} // fn bezier_point

/// @brief Round `points` to plotter units and store them as one polyline; clears `points`.
///
/// An untagged closed outline is treated as a cut line and an untagged open
/// line as a mark.
fn push_polyline(
    points: &mut Vec<(f64, f64)>,
    closed: bool,
    class: Option<LineClass>,
    polylines: &mut Vec<PlotPolyline>,
) {
    let mut rounded: Vec<(i32, i32)> = points.iter().map(|(x, y)| (x.round() as i32, y.round() as i32)).collect();
    rounded.dedup(); // interpolation can repeat a point after rounding
    points.clear();

    if rounded.len() < 2 {
        return; // a single point draws nothing
    } // if too short

    let class = class.unwrap_or(if closed { LineClass::Cut } else { LineClass::Mark });
    polylines.push(PlotPolyline { class, points: rounded, closed });
} // fn push_polyline
