// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file piece_divider.rs
// @brief Split a piece wider than the media into parts that fit.
//
// Runs on the pre-processed DOM: pieces are direct `<g>` children of the root,
// grain up, with transforms baked (text keeps its own transform), before
// `translate_dom` moves each piece to the origin.
//
// A piece that fits at one of its trial angles is not split.  In Layout Mode
// Any, a piece that fits turned 90° is not split either.  Otherwise its
// outline bounding box splits into equal columns (vertical cut lines).  Roll and
// fabric media have no length limit, so only the width limits a piece.  Each
// region of the outline inside one column is a part.
//
// Per part:
//   - outline (`cutline`, else `seamline`) and `cut_path`: closed, with new
//     edges on the cut lines. With a fold line, Seamly2D sends the outline
//     without its edge on the fold line. The divider adds that edge back, so the
//     outline makes parts; each part's outline again has no edge on the fold line;
//   - `seamline` beside a cutline, `internal_path`, `grainline`, `fold_line`: open
//     lines that end at the cut lines; a part the fold line misses has none. The
//     fold line arrow line and label (`fold_line_arrow`, `fold_line_label`) go
//     whole into the fold line group of the nearest part;
//   - notches, labels and other groups: whole, on the nearest part. A pattern, piece
//     or fold line label not wholly inside the part outline moves inside it
//     (`label_inside_part.rs`);
//   - a part without grainline gets a new vertical grainline when the piece had one;
//   - a new `piece_label`: "<name> part i of n, joins j", along the longest cut
//     edge. A label wider than that edge lies level across the part, clear of the
//     other paths and text; with no clear place, it shrinks to the edge
//     (`part_label_placement.rs`);
//   - two match notches on each cut edge it shares with another part;
//   - `data-split-of`, `data-split-part`, `data-split-count`; each id gets `_part<i>`.
// The cut edge has no seam allowance: the parts join edge to edge.
// `data-turn-points` stay valid: kept vertices keep their flag, new vertices on a
// cut line are turn points.

use std::collections::{HashMap, HashSet};

use geometry::fold_line_join::{distance_to_segment, line_closes_on_fold};
use geometry::half_plane_clip::{cut_closed, cut_open, CutLine, CutVertex};
use geometry::{Path, PathSegment};
use packing::Rect;
use svg_dom::Document;
use xmltree::{Element, XMLNode};

use crate::label_inside_part::move_labels_inside_part;
use crate::part_label_placement::{along_edge_bounds, place_part_label, CutEdge};
use crate::piece_extractor::{
    extract_piece_rects, set_free_rotation_without_grainline, set_piece_rotation_for_layout_mode,
    svg_uu_per_px, turn_wide_pieces_to_fit,
};

// Match notch length; DXF-ASTM slit depth is half the seam allowance written for it.
const MATCH_NOTCH_IN: f64 = 0.25;
// Font size of the part label.
const PART_LABEL_FONT_IN: f64 = 0.25;
// Outline regions smaller than this (square user units) are slivers, not parts.
const MIN_PART_AREA_UU: f64 = 1.0;
// Upper limit of parts across a piece; tiny media must not explode a piece.
const MAX_PARTS_PER_SIDE: usize = 50;
// Largest distance, in inches, of a cut line point from the fold line for it to lie on the fold (0.1 mm).
const FOLD_LINE_TOLERANCE_IN: f64 = 0.1 / 25.4;

// @brief Media a piece must fit, in layout pixels.
#[derive(Debug, Clone, PartialEq)]
pub struct DivideSpec {
    // Content rectangle width; the media has no length limit.
    pub bin_w: u32,
    // The layout mode's trial angles.
    pub trial_angles_deg: Vec<u16>,
    // Apply each piece's source rule only in Any layout mode.
    pub apply_piece_rotation: bool,
    // The user's "Rotate freely" choice for pieces without a grainline.
    pub free_rotation_without_grainline: bool,
    // Clearance from the fold edge for pieces without a fold line.
    pub fold_edge_gap_px: u32,
    // Pin fold-line pieces to the fold edge while measuring and splitting.
    // False in Layout Mode Any (`LayoutSettings::places_fold_lines_on_left_edge`).
    pub fold_lines_on_left_edge: bool,
}

// @brief A piece wider than the media at each of its trial angles.
#[derive(Debug, Clone, PartialEq)]
pub struct OversizedPiece {
    // Name the user reads (`PieceRect::label`).
    pub label: String,
    // Piece bounding box width, grain up, in layout pixels.
    pub width_px: u32,
    // Media content width the piece does not fit, less the fold edge gap, in layout pixels.
    pub media_width_px: u32,
}

// @brief A piece that was split, and into how many parts.
#[derive(Debug, Clone, PartialEq)]
pub struct DividedPiece {
    pub label: String,
    pub parts: usize,
}

// @brief Pieces wider than the media at each of their trial angles.
//
// @param doc Pre-processed DOM: pieces grain up, transforms baked.
pub fn find_oversized_pieces(doc: &Document, spec: &DivideSpec) -> Vec<OversizedPiece> {
    let pieces = prepared_pieces(doc, spec);
    pieces
        .iter()
        .filter(|piece| columns_for(piece.rect, spec) > 1)
        .map(|piece| OversizedPiece {
            label: piece.label().to_string(),
            width_px: piece.rect.w,
            media_width_px: spec.bin_w.saturating_sub(piece.rect.min_left_gap_px),
        })
        .collect()
} // fn find_oversized_pieces

// @brief One line per piece for the oversized-pieces dialog: "• Back: width 43.10 in, media 35.50 in".
//
// @param unit Settings unit, "in", "cm" or "mm"; any other value gives inches.
pub fn oversized_pieces_message(pieces: &[OversizedPiece], unit: &str) -> String {
    pieces
        .iter()
        .map(|piece| {
            format!(
                "\u{2022} {}: width {}, media {}",
                piece.label,
                unit_length(piece.width_px, unit),
                unit_length(piece.media_width_px, unit)
            )
        })
        .collect::<Vec<_>>()
        .join("\n")
} // fn oversized_pieces_message

// @brief A length in layout pixels as text in the settings unit: "43.10 in", "109.5 cm", "1095 mm".
fn unit_length(px: u32, unit: &str) -> String {
    let inches = f64::from(px) / layout_tiling::LAYOUT_PPI;
    match unit {
        "cm" => format!("{:.1} cm", inches * 2.54),
        "mm" => format!("{:.0} mm", inches * 25.4),
        _ => format!("{inches:.2} in"),
    } // match unit
} // fn unit_length

// @brief Replace each piece that does not fit the media with parts that fit.
//
// A piece without a `cutline` or `seamline` group cannot be split; it stays whole.
//
// @param doc Pre-processed DOM, changed in place: each split piece's `<g>` is
//            replaced, at the same position, by one `<g>` per part. With fold
//            lines on the left edge, pieces also turn or mirror as
//            `fold_line_edge::plan_fold_line_edge` plans.
// @return The split pieces, in document order.
pub fn divide_oversized_pieces(doc: &mut Document, spec: &DivideSpec) -> Vec<DividedPiece> {
    let uu_per_px = svg_uu_per_px(&doc.root);
    if spec.fold_lines_on_left_edge {
        // Turn and mirror in place, so each cut follows the piece as the packer places it.
        crate::fold_line_edge::plan_fold_line_edge(doc, &spec.trial_angles_deg);
    } // if fold lines go on the left edge
    let pieces = prepared_pieces(doc, spec);

    // Column count of each piece that does not fit, keyed by its index among the root's <g> children.
    let column_counts: HashMap<usize, usize> = pieces
        .iter()
        .map(|piece| (piece.group_index, columns_for(piece.rect, spec)))
        .filter(|&(_, columns)| columns > 1)
        .collect();
    if column_counts.is_empty() {
        return Vec::new();
    } // if every piece fits

    let mut divided = Vec::new();
    let mut group_index = 0usize;
    for node in std::mem::take(&mut doc.root.children) {
        // Only <g> children count toward the extractor's group index.
        let planned = match &node {
            XMLNode::Element(el) if el.name == "g" => {
                group_index += 1;
                column_counts.get(&(group_index - 1)).copied()
            } // <g>
            _ => None,
        }; // planned
        if let (Some(columns), XMLNode::Element(piece)) = (planned, &node) {
            if let Some(parts) = divide_piece(piece, columns, uu_per_px) {
                divided.push(DividedPiece { label: display_name(piece), parts: parts.len() });
                doc.root.children.extend(parts.into_iter().map(XMLNode::Element));
                continue;
            } // if split
        } // if planned
        doc.root.children.push(node);
    } // for each root child
    divided
} // fn divide_oversized_pieces

// @brief Columns a piece splits into; 1 when it fits at one of its trial angles.
fn columns_for(rect: Rect, spec: &DivideSpec) -> usize {
    // The media has no length limit.
    if packing::fits_bin(rect, spec.bin_w, u32::MAX, &spec.trial_angles_deg) {
        return 1;
    } // if the piece fits
    let available_width = spec.bin_w.saturating_sub(rect.min_left_gap_px);
    parts_for(rect.w, available_width)
} // fn columns_for

// @brief Return piece bounds with fold-edge placement rules applied.
fn prepared_pieces(doc: &Document, spec: &DivideSpec) -> Vec<crate::piece_extractor::PieceRect> {
    let mut measured_doc = doc.clone();
    let fold_plan = if spec.fold_lines_on_left_edge {
        crate::fold_line_edge::plan_fold_line_edge(&mut measured_doc, &spec.trial_angles_deg)
    } else {
        crate::fold_line_edge::FoldEdgePlan::default()
    };
    let mut pieces = extract_piece_rects(&measured_doc);
    set_piece_rotation_for_layout_mode(&mut pieces, spec.apply_piece_rotation);
    set_free_rotation_without_grainline(&mut pieces, spec.free_rotation_without_grainline);
    crate::fold_line_edge::pin_fold_line_pieces(&mut pieces, &fold_plan);
    for piece in &mut pieces {
        if piece.rect.left_edge_angle.is_none() && spec.fold_edge_gap_px > 0 {
            piece.rect = piece.rect.with_min_left_gap(spec.fold_edge_gap_px);
        }
    }
    // Layout Mode Any: a piece that fits turned 90° turns; it is not split.
    turn_wide_pieces_to_fit(&mut pieces, spec.bin_w, &spec.trial_angles_deg);
    pieces
} // fn prepared_pieces

// @brief Smallest count of equal parts of `length` px that each fit `limit` px.
// One pixel of slack covers rounding and the cut line nudge.
fn parts_for(length: u32, limit: u32) -> usize {
    if length <= limit || limit < 2 {
        return 1;
    }
    let mut n = 2usize;
    while n < MAX_PARTS_PER_SIDE && (f64::from(length) / n as f64).ceil() + 1.0 > f64::from(limit) {
        n += 1;
    }
    n
} // fn parts_for

// ---------------------------------------------------------------------------
// Path geometry
// ---------------------------------------------------------------------------

// @brief One subpath of a straight-line path.
#[derive(Debug, Clone)]
struct Subpath {
    points: Vec<(f64, f64)>,
    // Index in the path's `d` of each point: MoveTo and LineTo end points, counted from 0.
    indices: Vec<usize>,
    closed: bool,
    // `d` index of a dropped last point that repeated the first one.
    closing_index: Option<usize>,
}

// @brief Subpaths of a path of straight lines; None when `d` has curves or does not parse.
fn straight_subpaths(d: &str) -> Option<Vec<Subpath>> {
    let path = Path::parse_path_attribute(d).ok()?;
    let mut subpaths: Vec<Subpath> = Vec::new();
    let mut index = 0usize;
    for segment in &path.segments {
        match segment {
            PathSegment::MoveTo(p) | PathSegment::LineTo(p) => {
                if matches!(segment, PathSegment::MoveTo(_)) || subpaths.is_empty() {
                    subpaths.push(Subpath { points: Vec::new(), indices: Vec::new(), closed: false, closing_index: None });
                } // if new subpath
                let subpath = subpaths.last_mut().expect("subpath exists");
                subpath.points.push((f64::from(p.x), f64::from(p.y)));
                subpath.indices.push(index);
                index += 1;
            } // vertex
            PathSegment::Close => {
                if let Some(subpath) = subpaths.last_mut() {
                    subpath.closed = true;
                } // if subpath
            } // Close
            _ => return None, // curves: no 1:1 vertices
        } // match segment
    } // for each segment
    // Seamly2D repeats the first point before Z; the clipper wants it once.
    for subpath in &mut subpaths {
        let n = subpath.points.len();
        if subpath.closed && n > 1 && same_point(subpath.points[0], subpath.points[n - 1]) {
            subpath.points.pop();
            subpath.closing_index = subpath.indices.pop();
        } // if repeated closing point
    } // for each subpath
    Some(subpaths)
} // fn straight_subpaths

// @brief True when two points are the same within rounding.
fn same_point(a: (f64, f64), b: (f64, f64)) -> bool {
    (a.0 - b.0).abs() < 1e-4 && (a.1 - b.1).abs() < 1e-4
} // fn same_point

// @brief Absolute area of a polygon.
fn polygon_area(points: &[CutVertex]) -> f64 {
    let n = points.len();
    (0..n)
        .map(|i| {
            let (a, b) = (points[i].point, points[(i + 1) % n].point);
            a.0 * b.1 - b.0 * a.1
        })
        .sum::<f64>()
        .abs()
        / 2.0
} // fn polygon_area

// @brief Distance from `p` to a polygon: 0 inside, else to the nearest edge.
fn distance_to_polygon(p: (f64, f64), polygon: &[CutVertex]) -> f64 {
    let n = polygon.len();
    if n == 0 {
        return f64::INFINITY;
    }
    let mut inside = false;
    let mut nearest = f64::INFINITY;
    for i in 0..n {
        let (a, b) = (polygon[i].point, polygon[(i + 1) % n].point);
        // Ray to +x: count the edges it crosses.
        if (a.1 > p.1) != (b.1 > p.1) && p.0 < a.0 + (p.1 - a.1) / (b.1 - a.1) * (b.0 - a.0) {
            inside = !inside;
        }
        nearest = nearest.min(segment_distance(p, a, b));
    } // for each edge
    if inside { 0.0 } else { nearest }
} // fn distance_to_polygon

// @brief Distance from `p` to segment `a`-`b`.
fn segment_distance(p: (f64, f64), a: (f64, f64), b: (f64, f64)) -> f64 {
    let (dx, dy) = (b.0 - a.0, b.1 - a.1);
    let len2 = dx * dx + dy * dy;
    let t = if len2 > 0.0 { (((p.0 - a.0) * dx + (p.1 - a.1) * dy) / len2).clamp(0.0, 1.0) } else { 0.0 };
    let (qx, qy) = (a.0 + t * dx, a.1 + t * dy);
    ((p.0 - qx).powi(2) + (p.1 - qy).powi(2)).sqrt()
} // fn segment_distance

// @brief A point that tells which part a polyline belongs to: the middle of its middle segment.
// The ends of a clipped polyline lie on a cut line, between two parts.
fn polyline_anchor(vertices: &[CutVertex]) -> (f64, f64) {
    match vertices.len() {
        0 => (0.0, 0.0),
        1 => vertices[0].point,
        n => {
            let (a, b) = (vertices[n / 2 - 1].point, vertices[n / 2].point);
            ((a.0 + b.0) / 2.0, (a.1 + b.1) / 2.0)
        } // n
    } // match len
} // fn polyline_anchor

// @brief Format a coordinate for `d`: 4 decimals, no trailing zeros.
fn fmt_coord(v: f64) -> String {
    let s = format!("{v:.4}");
    let s = s.trim_end_matches('0').trim_end_matches('.');
    if s == "-0" || s.is_empty() { "0".to_string() } else { s.to_string() }
} // fn fmt_coord

// ---------------------------------------------------------------------------
// Cells and parts
// ---------------------------------------------------------------------------

// @brief One column between two vertical cut lines; None = unbounded on that side.
#[derive(Debug, Clone, Copy)]
struct Cell {
    x_lo: Option<f64>,
    x_hi: Option<f64>,
}

impl Cell {
    // @brief Half-planes that bound the cell: the line, and true to keep its high side.
    fn bounds(&self) -> Vec<(CutLine, bool)> {
        let mut out = Vec::new();
        if let Some(at) = self.x_lo { out.push((CutLine { vertical: true, at }, true)); }
        if let Some(at) = self.x_hi { out.push((CutLine { vertical: true, at }, false)); }
        out
    } // fn bounds
} // impl Cell

// @brief Clip a polyline to a cell.  Each vertex's source indexes `points`.
fn clip_to_cell(points: &[(f64, f64)], closed: bool, cell: &Cell) -> Vec<Vec<CutVertex>> {
    let mut current: Vec<Vec<CutVertex>> =
        vec![points.iter().enumerate().map(|(i, &p)| CutVertex { point: p, source: Some(i) }).collect()];
    for (line, keep_high) in cell.bounds() {
        let mut next = Vec::new();
        for polyline in &current {
            let pts: Vec<(f64, f64)> = polyline.iter().map(|v| v.point).collect();
            let sides = if closed { cut_closed(&pts, line) } else { cut_open(&pts, line) };
            for kept in if keep_high { sides.high } else { sides.low } {
                // Map each source back through the earlier cuts.
                next.push(
                    kept.into_iter()
                        .map(|v| CutVertex { point: v.point, source: v.source.and_then(|j| polyline[j].source) })
                        .collect(),
                );
            } // for each kept piece
        } // for each polyline
        current = next;
    } // for each bound
    current
} // fn clip_to_cell

// @brief One part: an outline region inside one cell.
#[derive(Debug, Clone)]
struct Part {
    cell: usize,
    polygon: Vec<CutVertex>,
    // Index of the outline path and subpath the region came from.
    path: usize,
    subpath: usize,
} // struct Part

impl Part {
    // @brief Bounding box: (min x, min y, max x, max y).
    fn bbox(&self) -> (f64, f64, f64, f64) {
        self.polygon.iter().fold(
            (f64::INFINITY, f64::INFINITY, f64::NEG_INFINITY, f64::NEG_INFINITY),
            |(x0, y0, x1, y1), v| (x0.min(v.point.0), y0.min(v.point.1), x1.max(v.point.0), y1.max(v.point.1)),
        )
    } // fn bbox

    // @brief Centre of the bounding box.
    fn centre(&self) -> (f64, f64) {
        let (x0, y0, x1, y1) = self.bbox();
        ((x0 + x1) / 2.0, (y0 + y1) / 2.0)
    } // fn centre

    // @brief Edges on a cut line: (line, low end, high end) along the line.
    fn cut_edges(&self) -> Vec<(CutLine, f64, f64)> {
        let n = self.polygon.len();
        (0..n).filter_map(|i| cut_edge(self.polygon[i], self.polygon[(i + 1) % n])).collect()
    } // fn cut_edges
} // impl Part

// @brief The vertical cut line an edge lies on, with its ends along the line; None for any other edge.
//
// Both ends are new vertices on the same cut line.  Two new vertices on different
// lines are joined by a piece of an original edge, not by a cut edge.
fn cut_edge(a: CutVertex, b: CutVertex) -> Option<(CutLine, f64, f64)> {
    if a.source.is_some() || b.source.is_some() {
        return None; // a kept vertex: not a cut edge
    } // if a vertex is kept
    let (pa, pb) = (a.point, b.point);
    if (pa.0 - pb.0).abs() < 1e-9 && (pa.1 - pb.1).abs() > 1e-9 {
        // Same x, different y: a stretch of a vertical cut line.
        Some((CutLine { vertical: true, at: pa.0 }, pa.1.min(pb.1), pa.1.max(pb.1)))
    } else {
        // An original edge between two cut lines, or a point.
        None
    } // if the edge is vertical
} // fn cut_edge

// @brief Break a clipped loop into open runs at its edges on cut lines.
// @return The loop itself, closed, when no edge lies on a cut line.
fn open_runs(polygon: Vec<CutVertex>) -> Vec<(Vec<CutVertex>, bool)> {
    break_loop(polygon, |a, b| cut_edge(a, b).is_some())
} // fn open_runs

// @brief Break a loop into open runs at its break edges.
// @param is_break True for the edge from its first vertex to its second where the loop breaks.
// @return The loop itself, closed, when it has no break edge.
fn break_loop(polygon: Vec<CutVertex>, is_break: impl Fn(CutVertex, CutVertex) -> bool) -> Vec<(Vec<CutVertex>, bool)> {
    let n = polygon.len();
    let breaks_at = |i: usize| is_break(polygon[i], polygon[(i + 1) % n]);
    let Some(first_break) = (0..n).find(|&i| breaks_at(i)) else {
        return vec![(polygon, true)];
    };
    // Start after a break edge; each break edge ends one run.
    let mut runs = Vec::new();
    let mut run: Vec<CutVertex> = Vec::new();
    for step in 1..=n {
        let i = (first_break + step) % n;
        run.push(polygon[i]);
        if breaks_at(i) {
            let done = std::mem::take(&mut run);
            if done.len() >= 2 {
                runs.push((done, false));
            }
        } // if break edge
    } // for each vertex
    runs
} // fn break_loop

// @brief Start and end of a piece's fold line, from the first subpath of its `fold_line` path.
// @return None when the piece has no straight fold line.
fn fold_line_ends(piece: &Element) -> Option<((f64, f64), (f64, f64))> {
    let subpaths = straight_subpaths(svg_dom::fold_line_d(piece)?)?;
    let points = &subpaths.first()?.points;
    (points.len() >= 2).then(|| (points[0], points[points.len() - 1]))
} // fn fold_line_ends

// @brief Index of the part nearest to `p`, among the parts of `cell` when it has any.
fn nearest_part(parts: &[Part], cell: Option<usize>, p: (f64, f64)) -> usize {
    let in_cell = |part: &&Part| cell.map_or(true, |c| part.cell == c);
    let candidates: Vec<usize> = if parts.iter().any(|part| in_cell(&part)) {
        (0..parts.len()).filter(|&i| in_cell(&&parts[i])).collect()
    } else {
        (0..parts.len()).collect()
    }; // candidates
    candidates
        .into_iter()
        .min_by(|&a, &b| distance_to_polygon(p, &parts[a].polygon).total_cmp(&distance_to_polygon(p, &parts[b].polygon)))
        .unwrap_or(0)
} // fn nearest_part

// ---------------------------------------------------------------------------
// Element helpers
// ---------------------------------------------------------------------------

// @brief Text of an attribute, or "".
fn attr<'a>(el: &'a Element, name: &str) -> &'a str {
    el.attributes.get(name).map(String::as_str).unwrap_or("")
} // fn attr

// @brief The name to show for a piece: `data-name`, else `data-letter`, else `id`.
fn display_name(piece: &Element) -> String {
    ["data-name", "data-letter", "id"]
        .iter()
        .map(|name| attr(piece, name))
        .find(|value| !value.is_empty())
        .unwrap_or("")
        .to_string()
} // fn display_name

// @brief `d` of every `<path>` below `el`, in document order.
fn path_ds(el: &Element, out: &mut Vec<String>) {
    for child in &el.children {
        if let XMLNode::Element(e) = child {
            if e.name == "path" {
                out.push(attr(e, "d").to_string());
            } else {
                path_ds(e, out);
            } // if path
        } // if element
    } // for child
} // fn path_ds

// @brief Change or remove each `<path>` below `el`: `f(k)` gives the new `d` of the k-th path, None removes it.
fn rewrite_paths(el: &mut Element, counter: &mut usize, f: &mut dyn FnMut(usize) -> Option<String>) {
    el.children.retain_mut(|node| match node {
        XMLNode::Element(child) if child.name == "path" => {
            let k = *counter;
            *counter += 1;
            match f(k) {
                Some(d) => {
                    child.attributes.insert("d".to_string(), d);
                    true
                } // Some
                None => false,
            } // match f
        } // path
        XMLNode::Element(child) => {
            rewrite_paths(child, counter, f);
            true
        } // container
        _ => true,
    });
} // fn rewrite_paths

// @brief Every vertex of every straight path below `el`.
fn all_points(el: &Element) -> Vec<(f64, f64)> {
    let mut ds = Vec::new();
    path_ds(el, &mut ds);
    ds.iter()
        .filter_map(|d| straight_subpaths(d))
        .flatten()
        .flat_map(|s| s.points)
        .collect()
} // fn all_points

// @brief A point that tells which part an element belongs to: the centre of its
// path points, else the anchor of its first `<text>`.
fn element_anchor(el: &Element) -> Option<(f64, f64)> {
    let points = all_points(el);
    if !points.is_empty() {
        let (x0, y0, x1, y1) = points.iter().fold(
            (f64::INFINITY, f64::INFINITY, f64::NEG_INFINITY, f64::NEG_INFINITY),
            |(x0, y0, x1, y1), p| (x0.min(p.0), y0.min(p.1), x1.max(p.0), y1.max(p.1)),
        );
        return Some(((x0 + x1) / 2.0, (y0 + y1) / 2.0));
    } // if paths
    text_anchor(el)
} // fn element_anchor

// @brief Position of the first `<text>` below (or at) `el`: its x, y through its transform.
fn text_anchor(el: &Element) -> Option<(f64, f64)> {
    if el.name == "text" {
        let number = |name: &str| attr(el, name).split_whitespace().next().and_then(|v| v.parse::<f32>().ok()).unwrap_or(0.0);
        let m = svg_dom::parse_svg_transform(attr(el, "transform"));
        let p = m.apply_to_point(geometry::Point::new(number("x"), number("y")));
        return Some((f64::from(p.x), f64::from(p.y)));
    } // if text
    el.children.iter().filter_map(|n| n.as_element()).find_map(text_anchor)
} // fn text_anchor

// @brief Append `suffix` to every `id` below `el`.
fn suffix_ids(el: &mut Element, suffix: &str) {
    for child in el.children.iter_mut() {
        if let XMLNode::Element(e) = child {
            if let Some(id) = e.attributes.get_mut("id") {
                id.push_str(suffix);
            } // if id
            suffix_ids(e, suffix);
        } // if element
    } // for child
} // fn suffix_ids

// @brief A new element with the given attributes and no children.
fn new_element(name: &str, attributes: &[(&str, String)]) -> Element {
    let mut el = Element::new(name);
    for (key, value) in attributes {
        el.attributes.insert((*key).to_string(), value.clone());
    }
    el
} // fn new_element

// ---------------------------------------------------------------------------
// Splitting one piece
// ---------------------------------------------------------------------------

// @brief One clipped polyline of a component path, owned by one part.
#[derive(Debug, Clone)]
struct Clipped {
    part: usize,
    subpath: usize,
    vertices: Vec<CutVertex>,
    closed: bool,
}

// @brief One `<path>` of a component, split over the parts.
struct SplitPath {
    subpaths: Vec<Subpath>,
    pieces: Vec<Clipped>,
}

// @brief Clip every path of a component to the cells and give each piece to a part.
// @return None when a path has curves: the component then goes whole to one part.
fn split_component(group: &Element, closed: bool, cells: &[Cell], parts: &[Part]) -> Option<Vec<SplitPath>> {
    let mut ds = Vec::new();
    path_ds(group, &mut ds);
    ds.iter()
        .map(|d| {
            let subpaths = straight_subpaths(d)?;
            let mut pieces = Vec::new();
            for (s, subpath) in subpaths.iter().enumerate() {
                let loop_path = subpath.closed && subpath.points.len() >= 3;
                for (c, cell) in cells.iter().enumerate() {
                    // A closed loop clips as a polygon, so its closing edge is kept;
                    // an open component then breaks at the edges on the cut lines.
                    let clipped: Vec<(Vec<CutVertex>, bool)> = if loop_path {
                        clip_to_cell(&subpath.points, true, cell)
                            .into_iter()
                            .filter(|polygon| polygon_area(polygon) >= 1e-6) // degenerate sliver
                            .flat_map(|polygon| if closed { vec![(polygon, true)] } else { open_runs(polygon) })
                            .collect()
                    } else {
                        clip_to_cell(&subpath.points, false, cell).into_iter().map(|run| (run, false)).collect()
                    }; // clipped
                    for (vertices, is_closed) in clipped {
                        let part = nearest_part(parts, Some(c), polyline_anchor(&vertices));
                        pieces.push(Clipped { part, subpath: s, vertices, closed: is_closed });
                    } // for each clipped piece
                } // for each cell
            } // for each subpath
            Some(SplitPath { subpaths, pieces })
        })
        .collect()
} // fn split_component

// @brief `d` and turn point indices for some clipped pieces of one path.
//
// A closed piece repeats its first vertex before Z, as Seamly2D writes it.
fn serialize_pieces(pieces: &[&Clipped], subpaths: &[Subpath], turns: &HashSet<usize>) -> (String, Vec<usize>) {
    let mut d = String::new();
    let mut new_turns = Vec::new();
    let mut index = 0usize;
    for piece in pieces {
        let subpath = &subpaths[piece.subpath];
        let is_turn = |v: &CutVertex| match v.source {
            None => true, // new vertex on a cut line: a corner, or an open end
            Some(j) => {
                turns.contains(&subpath.indices[j])
                    || (j == 0 && subpath.closing_index.is_some_and(|c| turns.contains(&c)))
            } // kept vertex
        }; // is_turn
        let mut write = |v: &CutVertex, command: &str, d: &mut String| {
            if !d.is_empty() {
                d.push(' ');
            }
            d.push_str(&format!("{command}{},{}", fmt_coord(v.point.0), fmt_coord(v.point.1)));
            if is_turn(v) {
                new_turns.push(index);
            }
            index += 1;
        }; // write
        for (k, v) in piece.vertices.iter().enumerate() {
            write(v, if k == 0 { "M" } else { "L" }, &mut d);
        } // for each vertex
        if piece.closed {
            write(&piece.vertices[0], "L", &mut d);
            d.push_str(" Z");
        } // if closed
    } // for each piece
    (d, new_turns)
} // fn serialize_pieces

// @brief A component group as part `part` holds it; None when the part gets none of it.
fn component_for_part(group: &Element, split: &[SplitPath], part: usize) -> Option<Element> {
    let turns: Option<HashSet<usize>> = group
        .attributes
        .get("data-turn-points")
        .and_then(|v| v.split_whitespace().map(|t| t.parse::<usize>().ok()).collect());
    let mut new_turns: Vec<Vec<usize>> = Vec::new();
    let mut el = group.clone();
    let mut counter = 0usize;
    rewrite_paths(&mut el, &mut counter, &mut |k| {
        let path = split.get(k)?;
        let mine: Vec<&Clipped> = path.pieces.iter().filter(|c| c.part == part).collect();
        if mine.is_empty() {
            return None;
        }
        let (d, turn_list) = serialize_pieces(&mine, &path.subpaths, turns.as_ref().unwrap_or(&HashSet::new()));
        new_turns.push(turn_list);
        Some(d)
    });
    if new_turns.is_empty() {
        return None; // no geometry for this part
    }
    // Turn indices count the vertices of one path: kept only for a one-path component.
    if turns.is_some() {
        match (split.len(), new_turns.as_slice()) {
            (1, [only]) => {
                let value = only.iter().map(usize::to_string).collect::<Vec<_>>().join(" ");
                el.attributes.insert("data-turn-points".to_string(), value);
            } // one path
            _ => {
                el.attributes.remove("data-turn-points");
            } // several paths
        } // match path count
    } // if turn points
    Some(el)
} // fn component_for_part

// @brief The fold line group of each part: its stretch of the fold line, clipped like
// an open line, and the tagged arrow line and label groups whole, in the nearest part.
// @return None when the fold line path cannot be clipped.
fn fold_line_for_parts(group: &Element, cells: &[Cell], parts: &[Part]) -> Option<Vec<Option<Element>>> {
    // Take the tagged parts out, so only the fold line path is clipped.
    let mut fold_only = group.clone();
    let mut tagged: Vec<Element> = Vec::new();
    fold_only.children.retain(|node| match node {
        XMLNode::Element(e) if e.attributes.contains_key("data-type") => {
            tagged.push(e.clone());
            false // a tagged part
        } // tagged
        _ => true, // the fold line and untagged content
    }); // retain
    let split = split_component(&fold_only, false, cells, parts)?;
    let mut groups: Vec<Option<Element>> =
        (0..parts.len()).map(|i| component_for_part(&fold_only, &split, i)).collect();

    // Each tagged part goes back, after the fold line, into the group of its nearest part.
    for part_group in tagged {
        let nearest = element_anchor(&part_group).map_or(0, |p| nearest_part(parts, None, p));
        if let Some(fold) = groups[nearest].as_mut() {
            fold.children.push(XMLNode::Element(part_group));
        } // if the nearest part holds fold line
    } // for each tagged part
    Some(groups)
} // fn fold_line_for_parts

// @brief The notch groups of each part; notches go whole to the nearest part.
//
// The per-notch attributes are filtered with their notches when they hold one
// value per notch, else dropped.
fn notches_for_parts(group: &Element, parts: &[Part]) -> Vec<Option<Element>> {
    const PER_NOTCH: [&str; 3] = ["data-notch-segments", "data-seam-allowances", "data-notch-type"];
    let mut out: Vec<Option<Element>> = vec![None; parts.len()];
    let mut ds = Vec::new();
    path_ds(group, &mut ds);
    let subpaths = match (ds.as_slice(), ds.first().and_then(|d| straight_subpaths(d))) {
        ([_], Some(subpaths)) => subpaths,
        _ => {
            // Several paths or curves: the whole group to the nearest part.
            let part = element_anchor(group).map_or(0, |p| nearest_part(parts, None, p));
            out[part] = Some(group.clone());
            return out;
        } // fallback
    }; // subpaths

    // Subpaths of each notch: `data-notch-segments` when it adds up, else one subpath per notch.
    let counts: Option<Vec<usize>> = group
        .attributes
        .get("data-notch-segments")
        .and_then(|v| v.split_whitespace().map(|t| t.parse::<usize>().ok()).collect())
        .filter(|c: &Vec<usize>| c.iter().sum::<usize>() == subpaths.len());
    let counts = counts.unwrap_or_else(|| vec![1; subpaths.len()]);
    let lists: Vec<Option<Vec<String>>> = PER_NOTCH
        .iter()
        .map(|name| {
            group.attributes.get(*name)
                .map(|v| v.split_whitespace().map(str::to_string).collect::<Vec<_>>())
                .filter(|values| values.len() == counts.len())
        })
        .collect();

    // Give each notch to the part nearest the middle of its first segment.
    let mut notches_of: Vec<Vec<usize>> = vec![Vec::new(); parts.len()];
    let mut first = 0usize;
    for (notch, &count) in counts.iter().enumerate() {
        let start = first;
        first += count;
        let anchor = match subpaths.get(start).map(|s| s.points.as_slice()) {
            Some([a, b, ..]) => ((a.0 + b.0) / 2.0, (a.1 + b.1) / 2.0),
            Some([a]) => *a,
            _ => continue, // empty notch path
        }; // anchor
        notches_of[nearest_part(parts, None, anchor)].push(notch);
    } // for each notch

    for (part, notches) in notches_of.iter().enumerate() {
        if notches.is_empty() {
            continue;
        }
        let mut el = group.clone();
        let mut d = String::new();
        for &notch in notches {
            let start: usize = counts[..notch].iter().sum();
            for subpath in &subpaths[start..start + counts[notch]] {
                d.push_str(&subpath_d(subpath));
            } // for each subpath
        } // for each notch
        rewrite_paths(&mut el, &mut 0, &mut |_| Some(d.trim().to_string()));
        for (name, list) in PER_NOTCH.iter().zip(&lists) {
            match list {
                Some(values) => {
                    let kept: Vec<&str> = notches.iter().map(|&n| values[n].as_str()).collect();
                    el.attributes.insert((*name).to_string(), kept.join(" "));
                } // one value per notch
                None => {
                    el.attributes.remove(*name);
                } // not per notch
            } // match list
        } // for each per-notch attribute
        out[part] = Some(el);
    } // for each part
    out
} // fn notches_for_parts

// @brief `d` text of one unclipped subpath, with a trailing space.
fn subpath_d(subpath: &Subpath) -> String {
    let mut d = String::new();
    for (k, p) in subpath.points.iter().enumerate() {
        d.push_str(&format!("{}{},{} ", if k == 0 { "M" } else { "L" }, fmt_coord(p.0), fmt_coord(p.1)));
    }
    if subpath.closed {
        if let Some(p) = subpath.closing_index.and(subpath.points.first()) {
            d.push_str(&format!("L{},{} ", fmt_coord(p.0), fmt_coord(p.1)));
        }
        d.push_str("Z ");
    } // if closed
    d
} // fn subpath_d

// @brief Shared stretches of cut line between two parts: (line, low end, high end).
fn shared_edges(a: &Part, b: &Part) -> Vec<(CutLine, f64, f64)> {
    let mut out = Vec::new();
    for (la, lo_a, hi_a) in a.cut_edges() {
        for (lb, lo_b, hi_b) in b.cut_edges() {
            if (la.at - lb.at).abs() > 1e-6 {
                continue;
            }
            let (lo, hi) = (lo_a.max(lo_b), hi_a.min(hi_b));
            if hi - lo > 1e-3 {
                out.push((la, lo, hi));
            }
        } // for each edge of b
    } // for each edge of a
    out
} // fn shared_edges

// @brief Direction from a vertical cut line into a part: +1 to the right, -1 to the left.
fn inward(part: &Part, line: CutLine) -> f64 {
    if part.centre().0 >= line.at { 1.0 } else { -1.0 }
} // fn inward

// @brief A point `offset` across the vertical cut line, at `along` (y) on the line.
fn off_line(line: CutLine, along: f64, offset: f64) -> (f64, f64) {
    (line.at + offset, along)
} // fn off_line

// @brief A vertical grainline with two arrow heads, centred in the part.
fn grainline_d(part: &Part, uu_per_in: f64) -> Option<String> {
    let (x0, _, x1, _) = part.bbox();
    let x = (x0 + x1) / 2.0;
    // Where x crosses the outline, sorted: pairs bound the stretches inside.
    let n = part.polygon.len();
    let mut ys: Vec<f64> = (0..n)
        .filter_map(|i| {
            let (a, b) = (part.polygon[i].point, part.polygon[(i + 1) % n].point);
            ((a.0 <= x) != (b.0 <= x)).then(|| a.1 + (x - a.0) / (b.0 - a.0) * (b.1 - a.1))
        })
        .collect();
    ys.sort_by(f64::total_cmp);
    let (top, bottom) = ys.chunks(2).filter(|c| c.len() == 2).map(|c| (c[0], c[1])).max_by(|a, b| (a.1 - a.0).total_cmp(&(b.1 - b.0)))?;
    let length = (bottom - top) * 0.6;
    if length <= 0.0 {
        return None;
    }
    let (y0, y1) = ((top + bottom - length) / 2.0, (top + bottom + length) / 2.0);
    let head = (0.25 * uu_per_in).min(length / 4.0);
    let f = fmt_coord;
    Some(format!(
        "M{x},{y0} L{x},{y1} M{xl},{h0} L{x},{y0} L{xr},{h0} M{xl},{h1} L{x},{y1} L{xr},{h1}",
        x = f(x), y0 = f(y0), y1 = f(y1), xl = f(x - head / 2.0), xr = f(x + head / 2.0),
        h0 = f(y0 + head), h1 = f(y1 - head),
    ))
} // fn grainline_d

// @brief Split one piece group into part groups; None when it cannot be split.
fn divide_piece(piece: &Element, columns: usize, uu_per_px: f64) -> Option<Vec<Element>> {
    let uu_per_in = uu_per_px * layout_tiling::LAYOUT_PPI;
    let children: Vec<&Element> = piece.children.iter().filter_map(|n| n.as_element()).collect();
    let of_type = |t: &str| children.iter().position(|c| attr(c, "data-type") == t);
    let outline_index = of_type("cutline").or_else(|| of_type("seamline"))?;
    let outline = children[outline_index];

    // Outline paths: the bounding box sets the cut lines.
    let mut outline_ds = Vec::new();
    path_ds(outline, &mut outline_ds);
    let mut outline_paths: Vec<Vec<Subpath>> = outline_ds.iter().map(|d| straight_subpaths(d)).collect::<Option<_>>()?;

    // With a fold line, Seamly2D sends the outline without its edge on the fold line: the outline
    // and the fold line make the outer edge. Such a subpath closes along the fold line, so it makes parts.
    let fold = fold_line_ends(piece);
    let fold_tolerance = FOLD_LINE_TOLERANCE_IN * uu_per_in;
    let mut joined: HashSet<(usize, usize)> = HashSet::new();
    if let Some(fold) = fold {
        for (p, subpaths) in outline_paths.iter_mut().enumerate() {
            for (s, subpath) in subpaths.iter_mut().enumerate() {
                if !subpath.closed && line_closes_on_fold(&subpath.points, fold, fold_tolerance) {
                    subpath.closed = true;
                    joined.insert((p, s));
                } // if the open subpath ends on the fold line
            } // for each subpath
        } // for each outline path
    } // if fold line
    let on_fold = |v: CutVertex| fold.is_some_and(|(a, b)| distance_to_segment(v.point, a, b) <= fold_tolerance);
    let outline_points: Vec<(f64, f64)> = outline_paths.iter().flatten().flat_map(|s| s.points.iter().copied()).collect();
    let (x0, y0, x1, y1) = outline_points.iter().fold(
        (f64::INFINITY, f64::INFINITY, f64::NEG_INFINITY, f64::NEG_INFINITY),
        |(x0, y0, x1, y1), p| (x0.min(p.0), y0.min(p.1), x1.max(p.0), y1.max(p.1)),
    );
    if !(x1 > x0 && y1 > y0) {
        return None;
    }

    // Equal vertical cut positions, moved off any vertex of the piece.
    let piece_points = all_points(piece);
    let xs: Vec<f64> = (1..columns)
        .map(|k| {
            let mut at = x0 + (x1 - x0) * k as f64 / columns as f64;
            while piece_points.iter().any(|p| (p.0 - at).abs() < 1e-3) {
                at += 0.01;
            } // while a vertex lies on the cut line
            at
        })
        .collect();
    let cells: Vec<Cell> = (0..columns)
        .map(|c| Cell {
            x_lo: (c > 0).then(|| xs[c - 1]),
            x_hi: (c + 1 < columns).then(|| xs[c]),
        })
        .collect();

    // Parts: each outline region of each cell, cells left to right, then top-left first.
    let mut parts: Vec<Part> = Vec::new();
    for (c, cell) in cells.iter().enumerate() {
        let mut in_cell: Vec<Part> = Vec::new();
        for (p, subpaths) in outline_paths.iter().enumerate() {
            for (s, subpath) in subpaths.iter().enumerate() {
                if !subpath.closed || subpath.points.len() < 3 {
                    continue;
                }
                for polygon in clip_to_cell(&subpath.points, true, cell) {
                    if polygon_area(&polygon) >= MIN_PART_AREA_UU {
                        in_cell.push(Part { cell: c, polygon, path: p, subpath: s });
                    }
                } // for each region
            } // for each subpath
        } // for each outline path
        in_cell.sort_by(|a, b| {
            let (ax, ay, _, _) = a.bbox();
            let (bx, by, _, _) = b.bbox();
            ay.total_cmp(&by).then(ax.total_cmp(&bx))
        });
        parts.extend(in_cell);
    } // for each cell
    if parts.len() < 2 {
        return None; // the cut lines miss the outline
    }
    let count = parts.len();

    // Outline: each part is exactly its own region. A joined outline opens again at the fold line.
    let outline_split: Vec<SplitPath> = outline_paths
        .iter()
        .enumerate()
        .map(|(p, subpaths)| SplitPath {
            subpaths: subpaths.clone(),
            pieces: parts
                .iter()
                .enumerate()
                .filter(|(_, part)| part.path == p)
                .flat_map(|(i, part)| {
                    let runs = if joined.contains(&(part.path, part.subpath)) {
                        break_loop(part.polygon.clone(), |a, b| on_fold(a) && on_fold(b)) // open at the fold line
                    } else {
                        vec![(part.polygon.clone(), true)] // closed region
                    }; // if joined
                    runs.into_iter().map(move |(vertices, closed)| Clipped { part: i, subpath: part.subpath, vertices, closed })
                })
                .collect(),
        })
        .collect();

    // Children of each part, in the piece's child order.
    let mut part_children: Vec<Vec<Element>> = vec![Vec::new(); count];
    let mut has_grain_geometry = vec![false; count];
    for (index, child) in children.iter().enumerate() {
        let data_type = attr(child, "data-type");
        let closed = match data_type {
            _ if index == outline_index => None, // handled above
            "cutline" | "cut_path" => Some(true),
            "seamline" | "internal_path" | "grainline" | "fold_line" => Some(false),
            _ => None,
        }; // closed
        if index == outline_index {
            for (i, list) in part_children.iter_mut().enumerate() {
                if let Some(el) = component_for_part(child, &outline_split, i) {
                    list.push(el);
                }
            }
            continue;
        } // if outline
        if data_type == "notch" {
            for (i, el) in notches_for_parts(child, &parts).into_iter().enumerate() {
                if let Some(el) = el {
                    part_children[i].push(el);
                }
            }
            continue;
        } // if notch
        if data_type == "fold_line" {
            if let Some(groups) = fold_line_for_parts(child, &cells, &parts) {
                for (i, group) in groups.into_iter().enumerate() {
                    if let Some(group) = group {
                        part_children[i].push(group);
                    } // if the part holds fold line
                } // for each part
                continue;
            } // if clipped
        } // if fold line
        if let Some(split) = closed.and_then(|closed| split_component(child, closed, &cells, &parts)) {
            for (i, list) in part_children.iter_mut().enumerate() {
                if let Some(el) = component_for_part(child, &split, i) {
                    list.push(el);
                    if data_type == "grainline" {
                        has_grain_geometry[i] = true;
                    }
                }
            }
            continue;
        } // if clipped
        // Labels and anything else: whole, to the nearest part.
        let part = element_anchor(child).map_or(0, |p| nearest_part(&parts, None, p));
        part_children[part].push((*child).clone());
        if data_type == "grainline" {
            has_grain_geometry[part] = true;
        }
    } // for each child

    // A part without grainline gets a new one, styled like the piece's.
    if let Some(grainline) = of_type("grainline").map(|i| children[i]) {
        for (i, part) in parts.iter().enumerate() {
            if has_grain_geometry[i] {
                continue;
            }
            let Some(d) = grainline_d(part, uu_per_in) else { continue };
            let mut el = grainline.clone();
            el.attributes.remove("data-turn-points");
            rewrite_paths(&mut el, &mut 0, &mut |k| (k == 0).then(|| d.clone()));
            part_children[i].push(el);
        } // for each part
    } // if the piece has a grainline

    // Which parts join, and the shared stretches of cut line.
    let mut joins: Vec<Vec<usize>> = vec![Vec::new(); count];
    let mut match_notches: Vec<Vec<((f64, f64), (f64, f64))>> = vec![Vec::new(); count];
    let notch_length = MATCH_NOTCH_IN * uu_per_in;
    for a in 0..count {
        for b in a + 1..count {
            let shared = shared_edges(&parts[a], &parts[b]);
            if shared.is_empty() {
                continue;
            }
            joins[a].push(b);
            joins[b].push(a);
            for (line, lo, hi) in shared {
                if hi - lo < 4.0 * notch_length {
                    continue; // too short for two notches
                }
                for along in [lo + (hi - lo) / 3.0, lo + 2.0 * (hi - lo) / 3.0] {
                    for &p in &[a, b] {
                        let start = off_line(line, along, 0.0);
                        let end = off_line(line, along, inward(&parts[p], line) * notch_length);
                        match_notches[p].push((start, end));
                    }
                } // for each notch position
            } // for each shared stretch
        } // for each b
    } // for each a

    let piece_id = attr(piece, "id").to_string();
    let name = attr(piece, "data-name").to_string();
    let shown = display_name(piece);
    // Components name their piece in data-parent: data-name, else the piece id.
    let component_parent = if name.is_empty() { piece_id.clone() } else { name.clone() };
    let next_label_number = children
        .iter()
        .filter(|c| attr(c, "data-type") == "piece_label")
        .filter_map(|c| attr(c, "data-type-number").parse::<u32>().ok())
        .max()
        .unwrap_or(0)
        + 1;

    let mut out = Vec::with_capacity(count);
    for (i, part) in parts.iter().enumerate() {
        let number = i + 1;
        let mut group = Element::new("g");
        group.attributes = piece.attributes.clone();
        group.attributes.insert("id".to_string(), format!("{piece_id}_part{number}"));
        group.attributes.insert("data-split-of".to_string(), piece_id.clone());
        group.attributes.insert("data-split-part".to_string(), number.to_string());
        group.attributes.insert("data-split-count".to_string(), count.to_string());
        if !name.is_empty() {
            group.attributes.insert("data-name".to_string(), format!("{name} ({number}/{count})"));
        }
        let mut children: Vec<Element> = std::mem::take(&mut part_children[i]);

        // Match notches: into the part's notch group, else a new one.
        if !match_notches[i].is_empty() {
            let mut d = String::new();
            for (a, b) in &match_notches[i] {
                d.push_str(&format!("M{},{} L{},{} ", fmt_coord(a.0), fmt_coord(a.1), fmt_coord(b.0), fmt_coord(b.1)));
            }
            let count_added = match_notches[i].len();
            let sa_mm = format!("{:.2}", 2.0 * MATCH_NOTCH_IN * 25.4);
            let per_notch = |value: &str| vec![value; count_added].join(" ");
            match children.iter().position(|c| attr(c, "data-type") == "notch") {
                Some(k) => {
                    // The first path keeps its notches and gains the match notches.
                    let existing = &mut children[k];
                    let mut ds = Vec::new();
                    path_ds(existing, &mut ds);
                    let joined = format!("{} {}", ds.first().map(String::as_str).unwrap_or(""), d.trim()).trim().to_string();
                    rewrite_paths(existing, &mut 0, &mut |k| (k == 0).then(|| joined.clone()));
                    let all_present = ["data-notch-segments", "data-seam-allowances", "data-notch-type"]
                        .iter()
                        .all(|n| existing.attributes.contains_key(*n));
                    if all_present {
                        for (attr_name, value) in [
                            ("data-notch-segments", per_notch("1")),
                            ("data-seam-allowances", per_notch(&sa_mm)),
                            ("data-notch-type", per_notch("slit")),
                        ] {
                            let entry = existing.attributes.get_mut(attr_name).expect("present");
                            entry.push(' ');
                            entry.push_str(&value);
                        }
                    } // if per-notch attributes
                } // existing group
                None => {
                    let mut notches = new_element("g", &[
                        ("id", format!("notch_split_{piece_id}")),
                        ("data-type", "notch".to_string()),
                        ("data-type-number", "1".to_string()),
                        ("data-parent", component_parent.clone()),
                        ("fill", "none".to_string()),
                        ("stroke", "#006400".to_string()),
                        ("stroke-width", "1".to_string()),
                        ("data-notch-segments", per_notch("1")),
                        ("data-seam-allowances", per_notch(&sa_mm)),
                        ("data-notch-type", per_notch("slit")),
                    ]);
                    let path = new_element("path", &[("d", d.trim().to_string())]);
                    notches.children.push(XMLNode::Element(path));
                    children.push(notches);
                } // new group
            } // match notch group
        } // if match notches

        let mut others: Vec<usize> = joins[i].iter().map(|j| j + 1).collect();
        others.sort_unstable();
        let joins_text = others.iter().map(usize::to_string).collect::<Vec<_>>().join(", ");
        let text = if joins_text.is_empty() {
            format!("{shown} part {number} of {count}")
        } else {
            format!("{shown} part {number} of {count}, joins {joins_text}")
        }; // text
        let longest_edge = part
            .cut_edges()
            .into_iter()
            .max_by(|a, b| (a.2 - a.1).total_cmp(&(b.2 - b.1)))
            .map(|(line, lo, hi)| CutEdge { x: line.at, lo, hi, inward: inward(part, line) });
        let outline: Vec<(f64, f64)> = part.polygon.iter().map(|v| v.point).collect();
        let font = PART_LABEL_FONT_IN * uu_per_in;

        // The piece's labels go inside the part first, clear of the part label along the cut edge.
        let reserved = longest_edge.and_then(|edge| along_edge_bounds(&text, font, edge));
        move_labels_inside_part(&outline, &mut children, reserved.as_slice(), uu_per_in);

        // Part label beside the longest cut edge, or level across the part when the edge is too short.
        let place = place_part_label(&text, font, &outline, longest_edge, &children);
        let (x, y) = (fmt_coord(place.centre.0), fmt_coord(place.centre.1));
        let transform = if place.along_edge {
            format!("translate({x} {y}) rotate(-90)") // reads up along the cut edge
        } else {
            format!("translate({x} {y})") // level
        }; // if along the edge
        let mut label = new_element("g", &[
            ("id", format!("piece_label_split_{piece_id}")),
            ("data-type", "piece_label".to_string()),
            ("data-type-number", next_label_number.to_string()),
            ("data-parent", component_parent.clone()),
            ("fill", "none".to_string()),
        ]);
        let mut text_el = new_element("text", &[
            ("x", "0".to_string()),
            ("y", "0".to_string()),
            ("transform", transform),
            ("font-family", "Arial".to_string()),
            ("font-size", fmt_coord(place.font)),
            ("fill", "#0000ff".to_string()),
            ("stroke", "none".to_string()),
            ("text-anchor", "middle".to_string()),
            ("dominant-baseline", "middle".to_string()),
        ]);
        if let Some(length) = place.text_length {
            // Renderers that read textLength draw the label exactly as long as the edge.
            text_el.attributes.insert("textLength".to_string(), fmt_coord(length));
            text_el.attributes.insert("lengthAdjust".to_string(), "spacingAndGlyphs".to_string());
        } // if the label shrank to the edge
        text_el.children.push(XMLNode::Text(text));
        label.children.push(XMLNode::Element(text_el));
        children.push(label);

        group.children = children.into_iter().map(XMLNode::Element).collect();
        suffix_ids(&mut group, &format!("_part{number}"));
        out.push(group);
    } // for each part
    Some(out)
} // fn divide_piece

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief One tagged piece, 400 × 100, with every component type.
    const WIDE_PIECE_SVG: &str = r#"<svg xmlns="http://www.w3.org/2000/svg" width="2000" height="2000">
  <g id="piece_Front" data-type="piece" data-type-number="1" data-name="Front" data-quantity="2">
    <g id="cutline_Front" data-type="cutline" data-type-number="1" data-parent="Front" data-turn-points="0 1 2 3 4"><path d="M0,0 L400,0 L400,100 L0,100 L0,0 Z"/></g>
    <g id="seamline_Front" data-type="seamline" data-type-number="1" data-parent="Front" data-turn-points="0 1 2 3 4"><path d="M10,10 L390,10 L390,90 L10,90 L10,10 Z"/></g>
    <g id="notch_Front" data-type="notch" data-type-number="1" data-parent="Front" data-notch-segments="1 1" data-seam-allowances="10.00 12.00" data-notch-type="slit tNotch"><path d="M50,0 L50,10 M350,0 L350,10"/></g>
    <g id="internal_path_1_Front" data-type="internal_path" data-type-number="1" data-parent="Front" data-turn-points="0 1"><path d="M20,50 L380,50"/></g>
    <g id="grainline_Front" data-type="grainline" data-type-number="1" data-parent="Front"><path d="M100,20 L100,80"/></g>
    <g id="piece_label_1_Front" data-type="piece_label" data-type-number="1" data-parent="Front"><text x="300" y="60">Front</text></g>
  </g>
</svg>"#;

    // @brief Media spec in layout pixels.
    fn spec(bin_w: u32) -> DivideSpec {
        DivideSpec {
            bin_w,
            trial_angles_deg: vec![0, 180],
            apply_piece_rotation: false,
            free_rotation_without_grainline: false,
            fold_edge_gap_px: 0,
            fold_lines_on_left_edge: false,
        }
    } // fn spec

    // @brief The part groups of a divided document.
    fn part_groups(doc: &Document) -> Vec<&Element> {
        doc.root.children.iter().filter_map(|n| n.as_element()).filter(|e| attr(e, "data-type") == "piece").collect()
    } // fn part_groups

    // @brief The first child group of `data_type` in a part.
    fn component<'a>(part: &'a Element, data_type: &str) -> Option<&'a Element> {
        part.children.iter().filter_map(|n| n.as_element()).find(|e| attr(e, "data-type") == data_type)
    } // fn component

    // @brief Every `d` below an element, joined.
    fn ds(el: &Element) -> String {
        let mut out = Vec::new();
        path_ds(el, &mut out);
        out.join(" ")
    } // fn ds

    // @brief Every id in a tree.
    fn collect_ids(el: &Element, out: &mut Vec<String>) {
        if let Some(id) = el.attributes.get("id") {
            out.push(id.clone());
        }
        for child in el.children.iter().filter_map(|n| n.as_element()) {
            collect_ids(child, out);
        }
    } // fn collect_ids

    // @brief A wide piece is reported and splits in two columns; every component lands on a part.
    #[test]
    fn wide_piece_splits_into_two_parts() {
        let mut doc = Document::parse(WIDE_PIECE_SVG).expect("fixture parses");
        let oversized = find_oversized_pieces(&doc, &spec(250));
        assert_eq!(
            oversized,
            vec![OversizedPiece { label: "Front".to_string(), width_px: 400, media_width_px: 250 }]
        );

        let divided = divide_oversized_pieces(&mut doc, &spec(250));
        assert_eq!(divided, vec![DividedPiece { label: "Front".to_string(), parts: 2 }]);
        let parts = part_groups(&doc);
        assert_eq!(parts.len(), 2);
        for (i, part) in parts.iter().enumerate() {
            let n = i + 1;
            assert_eq!(attr(part, "id"), format!("piece_Front_part{n}"));
            assert_eq!(attr(part, "data-split-of"), "piece_Front");
            assert_eq!(attr(part, "data-split-part"), n.to_string());
            assert_eq!(attr(part, "data-split-count"), "2");
            assert_eq!(attr(part, "data-name"), format!("Front ({n}/2)"));
            assert_eq!(attr(part, "data-quantity"), "2", "piece data kept");

            // Cut line: closed rectangle half, every vertex a turn point.
            let cutline = component(part, "cutline").expect("cutline");
            let d = ds(cutline);
            assert!(d.ends_with('Z') && d.contains("200,"), "{d}");
            assert_eq!(attr(cutline, "data-turn-points"), "0 1 2 3 4");
            // Seam line: one open run of 4 vertices that ends on the cut line both ways.
            let seamline = component(part, "seamline").expect("seamline");
            let d = ds(seamline);
            assert!(!d.contains('Z'), "{d}");
            assert_eq!(d.matches('M').count() + d.matches('L').count(), 4, "{d}");
            assert_eq!(attr(seamline, "data-turn-points"), "0 1 2 3");
            // Internal path: one run, both ends turn points.
            assert_eq!(attr(component(part, "internal_path").expect("internal path"), "data-turn-points"), "0 1");
            // Grainline: kept on part 1, new on part 2.
            assert!(component(part, "grainline").is_some(), "part {n} has a grainline");
            // Notches: the piece's notch plus 2 match notches, attributes per notch.
            let notch = component(part, "notch").expect("notch");
            assert_eq!(attr(notch, "data-notch-segments"), "1 1 1");
            let (kind, sa) = if n == 1 { ("slit slit slit", "10.00 12.70 12.70") } else { ("tNotch slit slit", "12.00 12.70 12.70") };
            assert_eq!((attr(notch, "data-notch-type"), attr(notch, "data-seam-allowances")), (kind, sa));
        } // for each part

        // Part labels name the part and the part it joins.
        let svg = doc.to_string();
        assert!(svg.contains("Front part 1 of 2, joins 2"), "{svg}");
        assert!(svg.contains("Front part 2 of 2, joins 1"), "{svg}");
        // The original label stays whole, on the part that holds it.
        assert!(component(parts[1], "piece_label").is_some_and(|l| attr(l, "id") == "piece_label_1_Front_part2"));
        // Ids stay unique.
        let mut ids = Vec::new();
        collect_ids(&doc.root, &mut ids);
        let count = ids.len();
        ids.sort();
        ids.dedup();
        assert_eq!(ids.len(), count, "duplicate ids");
    } // wide_piece_splits_into_two_parts

    // @brief The `<text>` of each part's new label, in part order.
    fn part_label_texts(doc: &Document) -> Vec<&Element> {
        part_groups(doc)
            .into_iter()
            .filter_map(|part| part.children.iter().filter_map(|n| n.as_element()).find(|e| attr(e, "id").starts_with("piece_label_split_")))
            .filter_map(|label| label.get_child("text"))
            .collect()
    } // fn part_label_texts

    // @brief Centre of a level part label, from its `translate(x y)` transform.
    fn translate_xy(text: &Element) -> (f64, f64) {
        let transform = attr(text, "transform").trim_start_matches("translate(").trim_end_matches(')');
        let numbers: Vec<f64> = transform.split_whitespace().map(|v| v.parse().expect("number")).collect();
        (numbers[0], numbers[1])
    } // fn translate_xy

    // @brief A part label keeps the 0.25 in (24 uu) font: along a cut edge it fits, else level
    // across the part. With no clear level place, it shrinks to the edge: smaller font and
    // `textLength` equal to the edge length.
    #[test]
    fn part_label_fits_its_part() {
        // 400 × 2000 piece: the label fits the 2000 uu cut edge.
        let tall = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece_Back" data-type="piece" data-name="Back">
            <g data-type="cutline"><path d="M0,0 L400,0 L400,2000 L0,2000 Z"/></g></g></svg>"#;
        let mut doc = Document::parse(tall).expect("parses");
        divide_oversized_pieces(&mut doc, &spec(250));
        let texts = part_label_texts(&doc);
        assert_eq!(texts.len(), 2);
        for text in texts {
            assert_eq!(attr(text, "font-size"), "24");
            assert!(attr(text, "transform").ends_with("rotate(-90)"), "{:?}", text.attributes);
            assert!(!text.attributes.contains_key("textLength") && !text.attributes.contains_key("lengthAdjust"));
        } // for each part label

        // trousers mirrored waistband, 2079 × 227 uu, on a 16 in roll (1488 px inside the margins).
        // The label is wider than the 227 uu cut edge. It lies level, inside the seam line and
        // clear of the pattern label beside the cut line at x 1039.5.
        let waistband = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece-4" data-type="piece" data-name="Piece_4_Union_piece">
            <g data-type="cutline"><path d="M0,0 L2079,0 L2079,227 L0,227 Z"/></g>
            <g data-type="seamline"><path d="M38,38 L2041,38 L2041,189 L38,189 Z"/></g>
            <g id="pattern_label_1" data-type="pattern_label" font-size="18">
              <text x="0" y="19" transform="matrix(1 0 0 1 700 38)">trousers.sm2d, trousers.smis</text>
              <text x="0" y="19" transform="matrix(1 0 0 1 830 62)">11-12-2026</text>
              <text x="0" y="19" transform="matrix(1 0 0 1 900 110)">Cut 1</text>
            </g></g></svg>"#;
        // Left end, baseline and text of each pattern label line.
        let pattern_lines =
            [(700.0, 57.0, "trousers.sm2d, trousers.smis"), (830.0, 81.0, "11-12-2026"), (900.0, 129.0, "Cut 1")];
        let mut doc = Document::parse(waistband).expect("parses");
        divide_oversized_pieces(&mut doc, &spec(1488));
        let texts = part_label_texts(&doc);
        assert_eq!(texts.len(), 2);
        for text in texts {
            assert_eq!(attr(text, "font-size"), "24");
            assert!(!text.attributes.contains_key("textLength"), "{:?}", text.attributes);
            let (x, y) = translate_xy(text);
            let half_width = svg_label_text::stroke_text_width(&text.get_text().expect("label text"), 24.0) / 2.0;
            let (left, top, right, bottom) = (x - half_width, y - 12.0, x + half_width, y + 12.0);
            assert!(left > 38.0 && top > 38.0 && right < 2041.0 && bottom < 189.0, "label at ({x}, {y})");
            assert!(right <= 1039.5 || left >= 1039.5, "label at ({x}, {y}) crosses the cut line");
            for (line_left, baseline, line) in pattern_lines {
                let line_right = line_left + svg_label_text::stroke_text_width(line, 18.0);
                let beside = right <= line_left || left >= line_right;
                let clear = beside || bottom <= baseline - 18.0 || top >= baseline + 4.5;
                assert!(clear, "label at ({x}, {y}) covers \"{line}\"");
            } // for each pattern label line
        } // for each part label

        // 400 × 100 piece: no clear level place in a 200 uu part; the label shrinks to the 100 uu edge.
        let mut doc = Document::parse(WIDE_PIECE_SVG).expect("fixture parses");
        divide_oversized_pieces(&mut doc, &spec(250));
        let texts = part_label_texts(&doc);
        assert_eq!(texts.len(), 2);
        for text in texts {
            let font: f64 = attr(text, "font-size").parse().expect("font size");
            let label = text.get_text().expect("label text");
            assert!(svg_label_text::stroke_text_width(&label, 24.0) > 100.0, "{label} fits at 24 uu");
            assert!((svg_label_text::stroke_text_width(&label, font) - 100.0).abs() < 0.01, "{label} at {font} uu");
            assert_eq!((attr(text, "textLength"), attr(text, "lengthAdjust")), ("100", "spacingAndGlyphs"));
        } // for each part label
    } // part_label_fits_its_part

    // @brief A label across the cut line, or past the outline, moves wholly inside its part,
    // at full size and clear of the part label along the cut edge.
    #[test]
    fn labels_outside_a_part_move_inside_it() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece_Back" data-type="piece" data-name="Back">
            <g data-type="cutline"><path d="M0,0 L400,0 L400,600 L0,600 Z"/></g>
            <g id="pattern_label_Back" data-type="pattern_label" font-size="18">
              <text x="0" y="19" transform="matrix(1 0 0 1 170 280)">Back pattern</text>
              <text x="0" y="19" transform="matrix(1 0 0 1 170 304)">Cut 2</text>
            </g>
            <g id="piece_label_Back" data-type="piece_label" font-size="18">
              <text x="0" y="19" transform="matrix(1 0 0 1 380 100)">Back</text>
            </g></g></svg>"#;
        let mut doc = Document::parse(svg).expect("parses");
        let label_boxes = |doc: &Document| -> Vec<(usize, &'static str, crate::part_label_placement::Bounds)> {
            let mut out = Vec::new();
            for (i, part) in part_groups(doc).into_iter().enumerate() {
                for data_type in ["pattern_label", "piece_label"] {
                    let labels = part.children.iter().filter_map(|n| n.as_element());
                    for label in labels.filter(|e| attr(e, "data-type") == data_type) {
                        let kind = if attr(label, "id").starts_with("piece_label_split_") { "part_label" } else { data_type };
                        out.push((i, kind, crate::part_label_placement::drawn_bounds(label).expect("label draws")));
                    } // for each label
                } // for each label type
            } // for each part
            out
        };
        let piece = doc.root.get_child("g").expect("piece");
        let before = component(piece, "pattern_label").and_then(crate::part_label_placement::drawn_bounds).expect("pattern label");
        assert!(before.left < 200.0 && before.right > 200.0, "the pattern label crosses the cut line: {before:?}");

        divide_oversized_pieces(&mut doc, &spec(250));
        let boxes = label_boxes(&doc);
        let part_label = |part: usize| boxes.iter().find(|(i, kind, _)| *i == part && *kind == "part_label").expect("part label").2;
        let mut moved = 0;
        for &(part, kind, b) in boxes.iter().filter(|(_, kind, _)| *kind != "part_label") {
            // Part 1 spans x 0..200, part 2 x 200..400; both y 0..600.
            let (left, right) = if part == 0 { (0.0, 200.0) } else { (200.0, 400.0) };
            assert!(b.left > left && b.right < right && b.top > 0.0 && b.bottom < 600.0, "{kind} in part {part}: {b:?}");
            assert!(!b.overlaps(&part_label(part)), "{kind} covers the part label: {b:?}");
            if kind == "pattern_label" {
                assert!((b.right - b.left - (before.right - before.left)).abs() < 1e-3, "full size: {b:?}");
            } // if pattern label
            moved += 1;
        } // for each label of the piece
        assert_eq!(moved, 2, "{boxes:?}");
    } // labels_outside_a_part_move_inside_it

    // @brief The media has no length limit: a long piece is not split.
    // A wide piece that fits turned is not split when it may turn.
    #[test]
    fn long_piece_and_turned_piece() {
        let long = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece-1" data-type="piece" data-name="Leg">
            <g data-type="cutline"><path d="M0,0 L100,0 L100,4000 L0,4000 Z"/></g></g></svg>"#;
        let mut doc = Document::parse(long).expect("parses");
        assert!(find_oversized_pieces(&doc, &spec(250)).is_empty(), "no length limit");
        assert!(divide_oversized_pieces(&mut doc, &spec(250)).is_empty());

        let wide = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece-1" data-type="piece" data-name="Yoke">
            <g data-type="cutline"><path d="M0,0 L400,0 L400,100 L0,100 Z"/></g></g></svg>"#;
        let doc = Document::parse(wide).expect("parses");
        assert_eq!(find_oversized_pieces(&doc, &spec(250)).len(), 1, "0 and 180 degrees only");
        // No grainline, free rotation allowed: the piece turns instead.
        let free = DivideSpec { free_rotation_without_grainline: true, ..spec(250) };
        assert!(find_oversized_pieces(&doc, &free).is_empty());
    } // long_piece_and_turned_piece

    // @brief Layout Mode Any: a 350 x 10 piece on 300 px media turns 90° whatever its rule, so it
    // is not split.  A 350 x 320 piece fits at no angle: it splits, and its parts keep the rule.
    #[test]
    fn piece_rotation_controls_split_and_stays_on_parts() {
        let any_svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece-Bias" data-type="piece" data-name="Bias" data-rotation="1">
            <g data-type="cutline"><path d="M0,0 L350,0 L350,10 L0,10 Z"/></g></g></svg>"#;
        let any_spec = DivideSpec {
            apply_piece_rotation: true,
            ..spec(300)
        };
        // 1 = any angle, 90 = 4-way, 180 = 2-way, 0 = none, 360 = 1-way, 1.5 = unsupported.
        for rule in ["1", "90", "180", "0", "360", "1.5"] {
            let turning_svg = any_svg.replace("data-rotation=\"1\"", &format!("data-rotation=\"{rule}\""));
            let mut turning_doc = Document::parse(&turning_svg).expect("fixture parses");
            assert!(find_oversized_pieces(&turning_doc, &any_spec).is_empty(), "rule {rule}");
            assert!(divide_oversized_pieces(&mut turning_doc, &any_spec).is_empty(), "rule {rule}");
        } // for each rule

        // Other layout modes do not turn a piece held to 0 and 180 degrees.
        let fixed_doc = Document::parse(&any_svg.replace("data-rotation=\"1\"", "data-rotation=\"0\"")).expect("fixture parses");
        assert_eq!(find_oversized_pieces(&fixed_doc, &spec(300)).len(), 1, "Along Grainline");

        let too_wide_svg = any_svg
            .replace("data-rotation=\"1\"", "data-rotation=\"180\"")
            .replace("L350,10 L0,10", "L350,320 L0,320");
        let mut too_wide_doc = Document::parse(&too_wide_svg).expect("fixture parses");
        assert_eq!(find_oversized_pieces(&too_wide_doc, &any_spec).len(), 1);
        assert_eq!(divide_oversized_pieces(&mut too_wide_doc, &any_spec)[0].parts, 2);
        assert!(part_groups(&too_wide_doc).iter().all(|part| attr(part, "data-rotation") == "180"));
    } // piece_rotation_controls_split_and_stays_on_parts

    // @brief Fold-line pieces use the fold edge while other pieces keep its configured clearance.
    #[test]
    fn folded_fabric_oversize_check_uses_fold_edge_clearance() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg">
          <g id="piece_NoFold" data-type="piece" data-name="NoFold">
            <g data-type="cutline"><path d="M0,0 L96,0 L96,40 L0,40 Z"/></g>
          </g>
          <g id="piece_OnFold" data-type="piece" data-name="OnFold">
            <g data-type="cutline"><path d="M0,0 L96,0 L96,40 L0,40 Z"/></g>
            <g data-type="fold_line"><path d="M0,0 L0,40"/></g>
          </g>
        </svg>"#;
        let mut doc = Document::parse(svg).expect("fixture parses");
        let folded = DivideSpec {
            fold_edge_gap_px: 5,
            fold_lines_on_left_edge: true,
            ..spec(100)
        };

        assert_eq!(
            find_oversized_pieces(&doc, &folded),
            vec![OversizedPiece { label: "NoFold".to_string(), width_px: 96, media_width_px: 95 }]
        );
        assert_eq!(
            divide_oversized_pieces(&mut doc, &folded),
            vec![DividedPiece { label: "NoFold".to_string(), parts: 2 }]
        );
        assert_eq!(part_groups(&doc).len(), 3);
    } // folded_fabric_oversize_check_uses_fold_edge_clearance

    // @brief A fold line is clipped like the seam line: each part holds the stretch
    // of fold line inside it, and a part the fold line misses holds none.
    #[test]
    fn fold_line_is_clipped_per_part() {
        // Fold line on the top edge, 400..0; the piece splits in two columns.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece-1" data-type="piece" data-name="Back" data-on-fold="true">
            <g data-type="cutline"><path d="M0,0 L400,0 L400,400 L0,400 Z"/></g>
            <g id="fold_line_Back" data-type="fold_line"><path d="M400,0 L0,0"/></g></g></svg>"#;
        let mut doc = Document::parse(svg).expect("parses");
        let divided = divide_oversized_pieces(&mut doc, &spec(250));
        assert_eq!(divided, vec![DividedPiece { label: "Back".to_string(), parts: 2 }]);

        let mut stretches = Vec::new();
        for part in part_groups(&doc) {
            let d = ds(component(part, "fold_line").expect("each part is on the fold line"));
            assert_eq!(d.matches('M').count(), 1, "one open run: {d}");
            assert!(!d.contains('Z'), "{d}");
            stretches.push(d);
        } // for each part
        assert_eq!(stretches, vec!["M200,0 L0,0".to_string(), "M400,0 L200,0".to_string()]);

        // A fold line on the left edge is not cut: the right part has none.
        let left = svg.replace("M400,0 L0,0", "M0,400 L0,0");
        let mut doc = Document::parse(&left).expect("parses");
        assert_eq!(divide_oversized_pieces(&mut doc, &spec(250))[0].parts, 2);
        let holders: Vec<bool> = part_groups(&doc).iter().map(|part| component(part, "fold_line").is_some()).collect();
        assert_eq!(holders, vec![true, false]);
    } // fold_line_is_clipped_per_part

    // @brief An outline without its fold line edge makes parts like a closed one. A part on the fold line
    // has no outline edge on the fold line; a part off the fold line has a closed outline. The outline is
    // the cut line, or the seam line of a piece without a seam allowance.
    #[test]
    fn open_outline_on_the_fold_line_splits_and_stays_open() {
        for outline_type in ["cutline", "seamline"] {
            open_outline_on_the_fold_line_splits_and_stays_open_with(outline_type);
        } // for each outline type
    } // open_outline_on_the_fold_line_splits_and_stays_open

    // @brief One case of `open_outline_on_the_fold_line_splits_and_stays_open`.
    fn open_outline_on_the_fold_line_splits_and_stays_open_with(outline_type: &str) {
        // Fold line on the left edge, 400..0; the outline runs from the fold line end to its start.
        let svg = format!(r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece-1" data-type="piece" data-name="Back" data-on-fold="true">
            <g data-type="{outline_type}" data-turn-points="0 1 2 3"><path d="M0,0 L400,0 L400,400 L0,400"/></g>
            <g id="fold_line_Back" data-type="fold_line"><path d="M0,400 L0,0"/></g></g></svg>"#);
        let mut doc = Document::parse(&svg).expect("parses");
        let divided = divide_oversized_pieces(&mut doc, &spec(250));
        assert_eq!(divided, vec![DividedPiece { label: "Back".to_string(), parts: 2 }], "{outline_type}");

        let mut open = 0;
        for part in part_groups(&doc) {
            let outline = component(part, outline_type).expect("outline");
            let d = ds(outline);
            let (x0, _, _, _) = part_bbox(part);
            if x0 < 1.0 {
                // On the fold line: one open run, ends on the fold line, both ends turn points.
                assert!(!d.contains('Z') && d.matches('M').count() == 1, "{outline_type}: {d}");
                assert!(d.starts_with("M0,") && d.contains(" L0,"), "{outline_type}: {d}");
                assert_eq!(attr(outline, "data-turn-points"), "0 1 2 3", "{outline_type}: {d}");
                open += 1;
            } else {
                assert!(d.ends_with('Z'), "{outline_type}: {d}");
            } // if on the fold line
        } // for each part
        assert_eq!(open, 1, "{outline_type}");
    } // fn open_outline_on_the_fold_line_splits_and_stays_open_with

    // @brief An open cut line without a fold line is not an outline region: the piece is not divided.
    #[test]
    fn open_cut_line_without_fold_line_is_not_divided() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece-1" data-type="piece" data-name="Back">
            <g data-type="cutline"><path d="M0,0 L400,0 L400,400 L0,400"/></g></g></svg>"#;
        let mut doc = Document::parse(svg).expect("parses");
        assert!(divide_oversized_pieces(&mut doc, &spec(250)).is_empty());
    } // open_cut_line_without_fold_line_is_not_divided

    // @brief The fold line arrow line and label are not clipped: each goes whole into the fold line group
    // of the part nearest to it, after that part's stretch of fold line.
    #[test]
    fn fold_line_arrow_and_label_go_whole_to_the_nearest_part() {
        // Fold line on the top edge; arrow line and label near its left end, in the left part.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece-1" data-type="piece" data-name="Back" data-on-fold="true">
            <g data-type="cutline"><path d="M0,0 L400,0 L400,400 L0,400 Z"/></g>
            <g id="fold_line_Back" data-type="fold_line"><g><path d="M400,0 L0,0"/></g>
              <g id="fold_line_arrow_Back" data-type="fold_line_arrow"><path d="M20,5 L20,20 L180,20 L180,5"/></g>
              <g id="fold_line_label_Back" data-type="fold_line_label"><text x="100" y="10">Foldline</text></g></g></g></svg>"#;
        let mut doc = Document::parse(svg).expect("parses");
        assert_eq!(divide_oversized_pieces(&mut doc, &spec(250))[0].parts, 2);

        let mut holders = Vec::new();
        for part in part_groups(&doc) {
            let Some(fold) = component(part, "fold_line") else { continue };
            let kinds: Vec<&str> = fold.children.iter().filter_map(|n| n.as_element()).map(|e| attr(e, "data-type")).collect();
            holders.push((attr(part, "id").to_string(), kinds));
        } // for each part
        assert_eq!(holders.len(), 2, "{holders:?}");
        assert!(holders.iter().any(|(_, kinds)| kinds == &vec!["", "fold_line_arrow", "fold_line_label"]), "{holders:?}");
        assert!(holders.iter().any(|(_, kinds)| kinds == &vec![""]), "{holders:?}");
    } // fold_line_arrow_and_label_go_whole_to_the_nearest_part

    // @brief Bounding box (min x, min y, max x, max y) of every path point of a part.
    fn part_bbox(part: &Element) -> (f64, f64, f64, f64) {
        let mut out = Vec::new();
        path_ds(part, &mut out);
        let numbers: Vec<f64> = out.join(" ")
            .split(|c: char| !(c.is_ascii_digit() || c == '.' || c == '-'))
            .filter_map(|t| t.parse().ok())
            .collect();
        numbers.chunks(2).fold(
            (f64::INFINITY, f64::INFINITY, f64::NEG_INFINITY, f64::NEG_INFINITY),
            |(x0, y0, x1, y1), p| (x0.min(p[0]), y0.min(p[1]), x1.max(p[0]), y1.max(p[1])),
        )
    } // fn part_bbox

    // @brief The dialog line names the piece width and the media width in the settings unit.
    #[test]
    fn oversized_message_gives_the_offending_dimensions() {
        let wide = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece-1" data-type="piece" data-name="Leg">
            <g data-type="cutline"><path d="M0,0 L480,0 L480,96 L0,96 Z"/></g></g></svg>"#;
        let doc = Document::parse(wide).expect("parses");

        // 480 × 96 px = 5 × 1 in, on media 3 in wide.
        let wider = find_oversized_pieces(&doc, &spec(288));
        assert_eq!(oversized_pieces_message(&wider, "in"), "\u{2022} Leg: width 5.00 in, media 3.00 in");
        assert_eq!(oversized_pieces_message(&wider, "cm"), "\u{2022} Leg: width 12.7 cm, media 7.6 cm");
        assert_eq!(oversized_pieces_message(&wider, "mm"), "\u{2022} Leg: width 127 mm, media 76 mm");
        assert_eq!(oversized_pieces_message(&wider, ""), "\u{2022} Leg: width 5.00 in, media 3.00 in", "unknown unit: inches");

        // One line per piece, in document order.
        let two = format!(
            "{}<g id=\"piece-2\" data-type=\"piece\" data-name=\"Cuff\"><g data-type=\"cutline\"><path d=\"M0,200 L480,200 L480,296 L0,296 Z\"/></g></g></svg>",
            wide.trim_end_matches("</svg>")
        );
        let doc = Document::parse(&two).expect("parses");
        let message = oversized_pieces_message(&find_oversized_pieces(&doc, &spec(288)), "in");
        assert_eq!(message.lines().collect::<Vec<_>>(), [
            "\u{2022} Leg: width 5.00 in, media 3.00 in",
            "\u{2022} Cuff: width 5.00 in, media 3.00 in",
        ]);
    } // oversized_message_gives_the_offending_dimensions

    // @brief A U open at the right, cut across its arms: the bridge and one part per arm.
    #[test]
    fn concave_piece_gives_one_part_per_region() {
        let u = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="piece_U" data-type="piece" data-name="U">
            <g data-type="cutline"><path d="M0,0 L300,0 L300,100 L100,100 L100,200 L300,200 L300,300 L0,300 L0,0 Z"/></g></g></svg>"#;
        let mut doc = Document::parse(u).expect("parses");
        let divided = divide_oversized_pieces(&mut doc, &spec(160));
        assert_eq!(divided[0].parts, 3);
        let svg = doc.to_string();
        assert!(svg.contains("U part 1 of 3, joins 2, 3"), "{svg}");
        assert!(svg.contains("U part 2 of 3, joins 1"), "{svg}");
        assert!(svg.contains("U part 3 of 3, joins 1"), "{svg}");
    } // concave_piece_gives_one_part_per_region

    // @brief Equal parts with one pixel of slack.
    #[test]
    fn parts_for_counts_equal_parts() {
        assert_eq!(parts_for(100, 250), 1);
        assert_eq!(parts_for(400, 250), 2);
        assert_eq!(parts_for(500, 250), 3, "2 parts of 250 leave no slack");
        assert_eq!(parts_for(400, 1), 1, "media too small to split for");
    } // parts_for_counts_equal_parts
} // mod tests
