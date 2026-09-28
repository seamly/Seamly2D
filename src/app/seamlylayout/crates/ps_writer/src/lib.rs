// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file lib.rs
// @brief Writes a layout SVG as a PostScript (.ps) or Encapsulated PostScript (.eps) file.
//
// Pipeline:
//   1. usvg resolves the viewBox, shapes, arcs and text into absolute-positioned paths.
//   2. `Writer` walks the usvg tree and writes one gsave/grestore block per path.
//   3. `svg_to_postscript` wraps the body in the DSC header of the chosen flavor.
//
// One writer serves both flavors. They differ only in the header and the page setup:
// EPS must not set the page device, because the importing program owns the page.
//
// Units: 96 px per inch (the layout resolution), 72 PostScript points per inch.
// PostScript puts the origin at the bottom left with Y up, so the page matrix flips Y.
// Each path keeps its own transform (`concat`), so stroke widths scale exactly as in SVG.

#[cfg(test)]
mod ps_writer_tests;

use std::collections::BTreeSet;
use std::fmt::Write as _;

use usvg::tiny_skia_path::{PathSegment, Point};

/// PostScript points per layout pixel (72 pt / 96 px).
pub const POINTS_PER_PX: f64 = 72.0 / 96.0;

/// Name of the private dictionary that holds the path operator abbreviations.
/// A private dictionary keeps an EPS from changing the importing program's `userdict`.
const PROLOG_DICT: &str = "SeamlyLayoutDict";

/// @brief Which PostScript file an export writes.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PsFlavor {
    /// PostScript document for a printer, with its own page size.
    Ps,
    /// Encapsulated PostScript: one page to place inside another document.
    Eps,
} // enum PsFlavor

impl PsFlavor {
    /// @brief Parse "ps" or "eps".
    pub fn from_name(name: &str) -> Option<Self> {
        match name {
            "ps" => Some(PsFlavor::Ps),   // printer document
            "eps" => Some(PsFlavor::Eps), // placeable graphic
            _ => None,                    // unknown flavor
        } // match name
    } // fn from_name

    /// @brief Short name used in messages: "PS" or "EPS".
    pub fn label(self) -> &'static str {
        match self {
            PsFlavor::Ps => "PS",
            PsFlavor::Eps => "EPS",
        } // match self
    } // fn label
} // impl PsFlavor

/// @brief Result of one conversion.
#[derive(Debug, Clone, PartialEq)]
pub struct PostScriptOutput {
    /// The complete PostScript program.
    pub program: String,
    /// Parts of the SVG that PostScript cannot show as drawn; empty when the output is exact.
    pub warnings: Vec<String>,
} // struct PostScriptOutput

/// @brief SVG features that PostScript writes differently or not at all.
///
/// Ordered, so the warnings come out in a stable order.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
enum Caveat {
    Transparency,
    Gradient,
    Pattern,
    Image,
    ClipMaskFilter,
} // enum Caveat

impl Caveat {
    /// @brief User-facing text for the export success dialog.
    fn message(self) -> &'static str {
        match self {
            Caveat::Transparency => "PostScript has no transparency; semi-transparent parts are written opaque.",
            Caveat::Gradient => "Gradients are written as one solid color (the first gradient stop).",
            Caveat::Pattern => "Pattern fills are written as solid black.",
            Caveat::Image => "Raster images are not written.",
            Caveat::ClipMaskFilter => "Clip paths, masks and filters are ignored.",
        } // match self
    } // fn message
} // impl Caveat

/// @brief Convert a layout SVG into a PostScript or EPS program.
///
/// Text is written as glyph outlines, so the file needs no fonts.
/// @return The program and its caveats, or Err when the SVG does not parse
///         or the layout draws nothing.
pub fn svg_to_postscript(doc: &svg_dom::Document, flavor: PsFlavor) -> Result<PostScriptOutput, String> {
    let tree = parse_tree(doc, flavor)?;

    // Page size in points; the layout page becomes the PostScript page.
    let size = tree.size();
    let width_pt = f64::from(size.width()) * POINTS_PER_PX;
    let height_pt = f64::from(size.height()) * POINTS_PER_PX;

    // Write the drawing first: it decides whether the export can succeed.
    let mut writer = Writer::default();
    writer.write_group(tree.root());
    if writer.drawn_paths == 0 {
        return Err(format!("{} export: the layout has nothing to draw.", flavor.label()));
    } // if nothing drawn

    let mut program = String::new();
    write_header(&mut program, flavor, width_pt, height_pt);

    // Page body: private dictionary on, then the px → pt matrix with Y flipped.
    program.push_str("%%Page: 1 1\n");
    let _ = writeln!(program, "{PROLOG_DICT} begin");
    program.push_str("gsave\n");
    let _ = writeln!(program, "0 {} translate", num(height_pt));
    let _ = writeln!(program, "{} {} scale", num(POINTS_PER_PX), num(-POINTS_PER_PX));
    program.push_str(&writer.body);
    program.push_str("grestore\n");
    program.push_str("end\n");
    // EPS may end with showpage; importing programs disable it.
    program.push_str("showpage\n");
    program.push_str("%%Trailer\n%%EOF\n");

    let warnings = writer.caveats.iter().map(|c| c.message().to_string()).collect();
    Ok(PostScriptOutput { program, warnings })
} // fn svg_to_postscript

/// @brief Write the DSC comments, the prolog and (PS only) the page size.
fn write_header(program: &mut String, flavor: PsFlavor, width_pt: f64, height_pt: f64) {
    match flavor {
        PsFlavor::Ps => program.push_str("%!PS-Adobe-3.0\n"),
        PsFlavor::Eps => program.push_str("%!PS-Adobe-3.0 EPSF-3.0\n"),
    } // match flavor
    program.push_str("%%Creator: SeamlyLayout\n");
    // %%BoundingBox takes whole points; round up so no drawing falls outside it.
    let _ = writeln!(program, "%%BoundingBox: 0 0 {} {}", width_pt.ceil() as i64, height_pt.ceil() as i64);
    let _ = writeln!(program, "%%HiResBoundingBox: 0 0 {} {}", num(width_pt), num(height_pt));
    // Level 2 for setpagedevice and the << >> dictionary syntax.
    program.push_str("%%LanguageLevel: 2\n");
    program.push_str("%%DocumentData: Clean7Bit\n");
    program.push_str("%%Pages: 1\n");
    program.push_str("%%EndComments\n");

    // Short path operators keep large layouts small.
    program.push_str("%%BeginProlog\n");
    let _ = writeln!(program, "/{PROLOG_DICT} 4 dict def");
    let _ = writeln!(program, "{PROLOG_DICT} begin");
    program.push_str("/m {moveto} bind def\n");
    program.push_str("/l {lineto} bind def\n");
    program.push_str("/c {curveto} bind def\n");
    program.push_str("/h {closepath} bind def\n");
    program.push_str("end\n");
    program.push_str("%%EndProlog\n");

    if flavor == PsFlavor::Ps {
        // Only a printer document sets its page size.
        program.push_str("%%BeginSetup\n");
        let _ = writeln!(program, "<< /PageSize [{} {}] >> setpagedevice", num(width_pt), num(height_pt));
        program.push_str("%%EndSetup\n");
    } // if Ps
} // fn write_header

/// @brief Parse the SVG with usvg; system fonts load only when `<text>` is present.
fn parse_tree(doc: &svg_dom::Document, flavor: PsFlavor) -> Result<usvg::Tree, String> {
    let svg = doc.to_string();
    let mut options = usvg::Options::default();
    if svg.contains("<text") {
        // Glyph outlines need fonts; loading them is slow, so skip it when possible.
        options.fontdb_mut().load_system_fonts();
    } // if text present
    usvg::Tree::from_data(svg.as_bytes(), &options)
        .map_err(|e| format!("{} export: SVG parse failed: {e}", flavor.label()))
} // fn parse_tree

/// @brief Accumulates the page body and the caveats met on the way.
#[derive(Default)]
struct Writer {
    /// PostScript for every drawn path, in paint order.
    body: String,
    /// SVG features that the body cannot show as drawn.
    caveats: BTreeSet<Caveat>,
    /// Paths written; zero means the export has nothing to show.
    drawn_paths: usize,
} // struct Writer

impl Writer {
    /// @brief Write every node of `group` in document (paint) order.
    fn write_group(&mut self, group: &usvg::Group) {
        // Group effects have no PostScript equivalent; the content is still drawn.
        if group.opacity().get() < 1.0 {
            self.caveats.insert(Caveat::Transparency);
        } // if translucent group
        if group.clip_path().is_some() || group.mask().is_some() || !group.filters().is_empty() {
            self.caveats.insert(Caveat::ClipMaskFilter);
        } // if clipped, masked or filtered

        for node in group.children() {
            match node {
                usvg::Node::Group(child) => self.write_group(child),       // recurse
                usvg::Node::Path(path) => self.write_path(path),           // geometry
                usvg::Node::Text(text) => self.write_group(text.flattened()), // glyph outlines
                usvg::Node::Image(_) => {
                    self.caveats.insert(Caveat::Image); // not written
                } // Image
            } // match node
        } // for node
    } // fn write_group

    /// @brief Write one path as `gsave <matrix> concat <path> <paint> grestore`.
    fn write_path(&mut self, path: &usvg::Path) {
        if !path.is_visible() || (path.fill().is_none() && path.stroke().is_none()) {
            return; // nothing is drawn on screen, so nothing is written
        } // if invisible

        let t = path.abs_transform();
        if t.invert().is_none() {
            return; // a collapsed transform draws nothing, and concat would fail
        } // if singular

        self.body.push_str("gsave\n");
        // tiny-skia (sx, ky, kx, sy, tx, ty) is the PostScript matrix [a b c d tx ty].
        let _ = writeln!(
            self.body,
            "[{} {} {} {} {} {}] concat",
            num(f64::from(t.sx)),
            num(f64::from(t.ky)),
            num(f64::from(t.kx)),
            num(f64::from(t.sy)),
            num(f64::from(t.tx)),
            num(f64::from(t.ty)),
        );
        self.write_segments(path.data());

        // Fill and stroke both consume the path; gsave/grestore keeps it for the second one.
        let fill = path.fill();
        let stroke = path.stroke();
        match path.paint_order() {
            usvg::PaintOrder::FillAndStroke => {
                if let Some(f) = fill {
                    self.write_fill(f, stroke.is_some());
                } // if fill
                if let Some(s) = stroke {
                    self.write_stroke(s, false);
                } // if stroke
            } // FillAndStroke
            usvg::PaintOrder::StrokeAndFill => {
                if let Some(s) = stroke {
                    self.write_stroke(s, fill.is_some());
                } // if stroke
                if let Some(f) = fill {
                    self.write_fill(f, false);
                } // if fill
            } // StrokeAndFill
        } // match paint_order

        self.body.push_str("grestore\n");
        self.drawn_paths += 1;
    } // fn write_path

    /// @brief Write the path segments in the path's local coordinates.
    ///
    /// PostScript has no quadratic curve, so each quadratic becomes the equal cubic.
    fn write_segments(&mut self, data: &usvg::tiny_skia_path::Path) {
        // Current point and subpath start, needed to raise a quadratic to a cubic.
        let mut current = Point::zero();
        let mut subpath_start = Point::zero();
        for segment in data.segments() {
            match segment {
                PathSegment::MoveTo(p) => {
                    let _ = writeln!(self.body, "{} {} m", num(f64::from(p.x)), num(f64::from(p.y)));
                    current = p;
                    subpath_start = p;
                } // MoveTo
                PathSegment::LineTo(p) => {
                    let _ = writeln!(self.body, "{} {} l", num(f64::from(p.x)), num(f64::from(p.y)));
                    current = p;
                } // LineTo
                PathSegment::QuadTo(q, p) => {
                    // Cubic controls lie 2/3 of the way from each end point to the quadratic control.
                    let c1 = Point::from_xy(current.x + 2.0 / 3.0 * (q.x - current.x), current.y + 2.0 / 3.0 * (q.y - current.y));
                    let c2 = Point::from_xy(p.x + 2.0 / 3.0 * (q.x - p.x), p.y + 2.0 / 3.0 * (q.y - p.y));
                    self.write_curve(c1, c2, p);
                    current = p;
                } // QuadTo
                PathSegment::CubicTo(c1, c2, p) => {
                    self.write_curve(c1, c2, p);
                    current = p;
                } // CubicTo
                PathSegment::Close => {
                    self.body.push_str("h\n");
                    current = subpath_start; // closepath returns to the subpath start
                } // Close
            } // match segment
        } // for segment
    } // fn write_segments

    /// @brief Write one cubic Bézier segment.
    fn write_curve(&mut self, c1: Point, c2: Point, p: Point) {
        let _ = writeln!(
            self.body,
            "{} {} {} {} {} {} c",
            num(f64::from(c1.x)),
            num(f64::from(c1.y)),
            num(f64::from(c2.x)),
            num(f64::from(c2.y)),
            num(f64::from(p.x)),
            num(f64::from(p.y)),
        );
    } // fn write_curve

    /// @brief Fill the current path; `keep_path` wraps it in gsave/grestore for a later stroke.
    fn write_fill(&mut self, fill: &usvg::Fill, keep_path: bool) {
        if fill.opacity().get() < 1.0 {
            self.caveats.insert(Caveat::Transparency);
        } // if translucent
        if keep_path {
            self.body.push_str("gsave\n");
        } // if keep_path
        self.write_color(fill.paint());
        match fill.rule() {
            usvg::FillRule::NonZero => self.body.push_str("fill\n"),
            usvg::FillRule::EvenOdd => self.body.push_str("eofill\n"),
        } // match rule
        if keep_path {
            self.body.push_str("grestore\n");
        } // if keep_path
    } // fn write_fill

    /// @brief Stroke the current path; `keep_path` wraps it in gsave/grestore for a later fill.
    ///
    /// Only settings that differ from the PostScript defaults are written.
    fn write_stroke(&mut self, stroke: &usvg::Stroke, keep_path: bool) {
        if stroke.opacity().get() < 1.0 {
            self.caveats.insert(Caveat::Transparency);
        } // if translucent
        if keep_path {
            self.body.push_str("gsave\n");
        } // if keep_path
        self.write_color(stroke.paint());
        let _ = writeln!(self.body, "{} setlinewidth", num(f64::from(stroke.width().get())));

        // PostScript defaults: butt cap (0), miter join (0), miter limit 10.
        match stroke.linecap() {
            usvg::LineCap::Butt => {}
            usvg::LineCap::Round => self.body.push_str("1 setlinecap\n"),
            usvg::LineCap::Square => self.body.push_str("2 setlinecap\n"),
        } // match linecap
        match stroke.linejoin() {
            usvg::LineJoin::Miter | usvg::LineJoin::MiterClip => {
                // SVG's default limit is 4, PostScript's is 10.
                let limit = f64::from(stroke.miterlimit().get());
                if limit != 10.0 {
                    let _ = writeln!(self.body, "{} setmiterlimit", num(limit));
                } // if not the PostScript default
            } // Miter
            usvg::LineJoin::Round => self.body.push_str("1 setlinejoin\n"),
            usvg::LineJoin::Bevel => self.body.push_str("2 setlinejoin\n"),
        } // match linejoin
        if let Some(dashes) = stroke.dasharray() {
            let pattern: Vec<String> = dashes.iter().map(|d| num(f64::from(*d))).collect();
            let _ = writeln!(self.body, "[{}] {} setdash", pattern.join(" "), num(f64::from(stroke.dashoffset())));
        } // if dashed

        self.body.push_str("stroke\n");
        if keep_path {
            self.body.push_str("grestore\n");
        } // if keep_path
    } // fn write_stroke

    /// @brief Set the color for `paint`; non-solid paints fall back to one solid color.
    fn write_color(&mut self, paint: &usvg::Paint) {
        let color = match paint {
            usvg::Paint::Color(c) => *c,
            usvg::Paint::LinearGradient(g) => {
                self.caveats.insert(Caveat::Gradient);
                first_stop_color(g.stops())
            } // LinearGradient
            usvg::Paint::RadialGradient(g) => {
                self.caveats.insert(Caveat::Gradient);
                first_stop_color(g.stops())
            } // RadialGradient
            usvg::Paint::Pattern(_) => {
                self.caveats.insert(Caveat::Pattern);
                usvg::Color::black()
            } // Pattern
        }; // color
        let _ = writeln!(
            self.body,
            "{} {} {} setrgbcolor",
            num(f64::from(color.red) / 255.0),
            num(f64::from(color.green) / 255.0),
            num(f64::from(color.blue) / 255.0),
        );
    } // fn write_color
} // impl Writer

/// @brief Color of the first gradient stop, or black when the gradient has none.
fn first_stop_color(stops: &[usvg::Stop]) -> usvg::Color {
    stops.first().map(|s| s.color()).unwrap_or_else(usvg::Color::black)
} // fn first_stop_color

/// @brief Format a number with at most 4 decimals and no trailing zeros.
///
/// PostScript reads plain decimals only: no exponent, and "-0" becomes "0".
pub(crate) fn num(value: f64) -> String {
    let mut text = format!("{value:.4}");
    if text.contains('.') {
        // Drop trailing zeros, then a bare decimal point.
        let trimmed = text.trim_end_matches('0').trim_end_matches('.').len();
        text.truncate(trimmed);
    } // if decimal
    if text == "-0" {
        text = "0".to_string();
    } // if negative zero
    text
} // fn num
