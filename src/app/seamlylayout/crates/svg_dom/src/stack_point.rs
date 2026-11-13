// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file stack_point.rs
// @brief Stack point of each size of a multisize piece: the point where the Nested tab stacks the sizes.
//
// Three places use the stack point and must agree, so that the annotations of every size stack exactly:
// `center_piece_sets` (Nested tab), `cxxqt_bridge`'s `annotation_alignment` (import), and Seamly2D's
// `src/libs/layout_handoff/handoff_stack_point.cpp` (handoff), which has the same rules in C++.
// Change the rules in both languages together.
//
// The rules measure each size in its grain frame: x' runs across the grain, y' runs against the grain
// direction. When the grainline points up, x' is the SVG x and y' is the SVG y, so a larger y' is lower.
// 1. Fold line: when every size has a fold line, the lower end of the largest size's fold line, or its
//    left end when the fold line is level. Every size uses the same end, start or end.
// 2. Corner: when every size has turn points, the corner (turn point) of the largest size with the longest
//    horizontal edge; with no horizontal edge, the corner with the longest edge. On a tie: the longer other
//    edge, then the lower corner, then the left one. Every other size uses the turn point nearest the same
//    place in its bounding box.
// 3. Centre: else the centre of each size's outline bounding box.
// The largest size is the first one with the largest bounding-box area.

use geometry::{Matrix2D, Path, PathSegment, Point};
use xmltree::Element;

use crate::transforms::{fold_line_group, parse_svg_transform};

// @brief Grain direction of a piece whose grainline points up, as a unit vector in SVG coordinates.
pub const GRAIN_UP: (f64, f64) = (0.0, -1.0);

// @brief Distance at or below which two points are one point, and two lengths or places tie, in px.
const SAME_PX: f64 = 0.01;

// @brief Distance from an edge's line at or below which a vertex continues the edge, in px.
const ON_EDGE_PX: f64 = 0.5;

// @brief Sine of 1°: an edge whose direction rises or falls by this much or less is horizontal.
const HORIZONTAL_SINE: f64 = 0.017_452_406_437_283_51;

// @brief A point in a grain frame: (x', y').
type FramePoint = (f64, f64);

// @brief One size's outline and fold line, measured in its grain frame.
struct SizeOutline {
    // Outline vertices, without the closing vertex that repeats the first one.
    vertices: Vec<FramePoint>,
    // Turn point indices into `vertices`, sorted and unique; None when the outline has no `data-turn-points`.
    turn_points: Option<Vec<usize>>,
    // Fold line start and end.
    fold_line: Option<(FramePoint, FramePoint)>,
    // Grain direction unit vector, in the piece frame.
    grain: (f64, f64),
}

// @brief One corner of an outline, with the edge lengths that rank it.
struct Corner {
    // Vertex index.
    index: usize,
    // True when one of its two edges is horizontal.
    horizontal: bool,
    // The horizontal edge's length when only one edge is horizontal, else the longer edge's length.
    primary: f64,
    // The other edge's length.
    secondary: f64,
}

// @brief Stack point of each size of a piece-set, in each piece's own frame.
// @param pieces  Each size's piece `<g>` with its grain direction unit vector in the piece frame, in
//                document order. The piece's own `transform` is not applied.
// @return One point per piece; None for a piece whose outline has fewer than three vertices.
pub fn stack_points(pieces: &[(&Element, (f64, f64))]) -> Vec<Option<Point>> {
    let sizes: Vec<Option<SizeOutline>> = pieces.iter().map(|&(piece, grain)| size_outline(piece, grain)).collect();
    frame_stack_points(&sizes)
        .into_iter()
        .zip(&sizes)
        .map(|(point, size)| Some(size.as_ref()?.to_piece(point?)))
        .collect()
} // fn stack_points

// @brief Apply the three rules to every size.
// @return One grain frame point per size; None for a size without an outline.
fn frame_stack_points(sizes: &[Option<SizeOutline>]) -> Vec<Option<FramePoint>> {
    let measured: Vec<&SizeOutline> = sizes.iter().flatten().collect();
    // The largest size: the first with the largest bounding-box area.
    let mut largest: Option<&SizeOutline> = None;
    for &size in &measured {
        if largest.is_none_or(|l| size.area() > l.area()) {
            largest = Some(size); // first or larger
        } // if larger
    } // for each measured size
    let Some(largest) = largest else {
        return vec![None; sizes.len()]; // no size has an outline
    }; // largest

    // Rule 1: the fold line end of the largest size, the same end in every size.
    if measured.iter().all(|s| s.fold_line.is_some()) {
        if let Some((start, end)) = largest.fold_line {
            let use_end = lower_or_left(end, start);
            return sizes
                .iter()
                .map(|s| s.as_ref()?.fold_line.map(|(a, b)| if use_end { b } else { a }))
                .collect();
        } // if fold line
    } // if every size has a fold line

    // Rule 2: the best corner of the largest size, then the nearest turn point of every size.
    if measured.iter().all(|s| s.turn_points.as_ref().is_some_and(|t| !t.is_empty())) {
        if let Some(corner) = best_corner(largest) {
            let place = largest.place(largest.vertices[corner]);
            return sizes.iter().map(|s| s.as_ref().map(|s| s.vertices[s.nearest_turn_point(place)])).collect();
        } // if corner
    } // if every size has turn points

    // Rule 3: the bounding-box centre.
    sizes.iter().map(|s| s.as_ref().map(SizeOutline::centre)).collect()
} // fn frame_stack_points

// @brief True when `a` is lower than `b`, or level with `b` and left of it.
fn lower_or_left(a: FramePoint, b: FramePoint) -> bool {
    if (a.1 - b.1).abs() > SAME_PX {
        return a.1 > b.1; // not level: the larger y' is lower
    } // if not level
    a.0 < b.0 - SAME_PX
} // fn lower_or_left

// @brief Corner of the outline with the longest horizontal edge, else with the longest edge.
// @return Vertex index; None when the outline has no turn points.
fn best_corner(size: &SizeOutline) -> Option<usize> {
    let corners: Vec<Corner> = size.turn_points.as_deref()?.iter().map(|&i| size.corner(i)).collect();
    let any_horizontal = corners.iter().any(|c| c.horizontal);
    let mut best: Option<&Corner> = None;
    // Ascending index order: on a full tie the first corner stays.
    for corner in corners.iter().filter(|c| c.horizontal || !any_horizontal) {
        if best.is_none_or(|b| size.ranks_above(corner, b)) {
            best = Some(corner); // first or better
        } // if better
    } // for each candidate corner
    best.map(|c| c.index)
} // fn best_corner

// @brief Distance between two points.
fn distance(a: FramePoint, b: FramePoint) -> f64 {
    (a.0 - b.0).hypot(a.1 - b.1)
} // fn distance

impl SizeOutline {
    // @brief Bounding box: (min x', min y', max x', max y').
    fn bounds(&self) -> (f64, f64, f64, f64) {
        self.vertices.iter().fold(
            (f64::INFINITY, f64::INFINITY, f64::NEG_INFINITY, f64::NEG_INFINITY),
            |(x0, y0, x1, y1), p| (x0.min(p.0), y0.min(p.1), x1.max(p.0), y1.max(p.1)),
        )
    } // fn bounds

    // @brief Bounding-box area.
    fn area(&self) -> f64 {
        let (x0, y0, x1, y1) = self.bounds();
        (x1 - x0) * (y1 - y0)
    } // fn area

    // @brief Bounding-box centre.
    fn centre(&self) -> FramePoint {
        let (x0, y0, x1, y1) = self.bounds();
        ((x0 + x1) / 2.0, (y0 + y1) / 2.0)
    } // fn centre

    // @brief Place of a point in the bounding box: 0 at the left or top, 1 at the right or bottom.
    // @details A side of zero length puts every point at 0.5 along it.
    fn place(&self, p: FramePoint) -> FramePoint {
        let (x0, y0, x1, y1) = self.bounds();
        let fraction = |value: f64, low: f64, high: f64| if high - low > SAME_PX { (value - low) / (high - low) } else { 0.5 };
        (fraction(p.0, x0, x1), fraction(p.1, y0, y1))
    } // fn place

    // @brief Turn point whose place in the bounding box is nearest `place`; the first one on a tie.
    // @return Vertex index. The caller checks that turn points exist.
    fn nearest_turn_point(&self, place: FramePoint) -> usize {
        let mut nearest = (usize::MAX, f64::INFINITY);
        for &index in self.turn_points.as_deref().unwrap_or_default() {
            let gap = distance(self.place(self.vertices[index]), place);
            if gap < nearest.1 {
                nearest = (index, gap); // nearer
            } // if nearer
        } // for each turn point
        nearest.0
    } // fn nearest_turn_point

    // @brief True when `index` is a turn point.
    fn is_turn_point(&self, index: usize) -> bool {
        self.turn_points.as_ref().is_some_and(|t| t.binary_search(&index).is_ok())
    } // fn is_turn_point

    // @brief The corner at vertex `index`, with its two edges measured.
    fn corner(&self, index: usize) -> Corner {
        let (before, before_horizontal) = self.edge(index, false);
        let (after, after_horizontal) = self.edge(index, true);
        let (primary, secondary) = match (before_horizontal, after_horizontal) {
            (true, false) => (before, after),                                // only the edge before is horizontal
            (false, true) => (after, before),                                // only the edge after is horizontal
            _ => (before.max(after), before.min(after)),                     // both or neither
        }; // match horizontal edges
        Corner { index, horizontal: before_horizontal || after_horizontal, primary, secondary }
    } // fn corner

    // @brief Length of the straight edge that leaves vertex `corner`, and whether it is horizontal.
    // @details The first vertex away from the corner sets the edge direction. The edge goes on through the
    //          vertices within `ON_EDGE_PX` of that line that move away from the corner, and stops at a
    //          turn point.
    // @param forward  True follows the vertex order; false goes against it.
    fn edge(&self, corner: usize, forward: bool) -> (f64, bool) {
        let count = self.vertices.len();
        let step = |k: usize| if forward { (k + 1) % count } else { (k + count - 1) % count };
        let origin = self.vertices[corner];

        // Skip vertices on the corner itself.
        let mut k = step(corner);
        while k != corner && distance(self.vertices[k], origin) <= SAME_PX {
            k = step(k);
        } // while on the corner
        if k == corner {
            return (0.0, false); // every vertex is on the corner
        } // if no other vertex

        let mut length = distance(self.vertices[k], origin);
        let unit = ((self.vertices[k].0 - origin.0) / length, (self.vertices[k].1 - origin.1) / length);
        while !self.is_turn_point(k) {
            let next = step(k);
            if next == corner {
                break; // back at the start
            } // if back at the corner
            let offset = (self.vertices[next].0 - origin.0, self.vertices[next].1 - origin.1);
            let off_line = (offset.0 * unit.1 - offset.1 * unit.0).abs();
            let along = offset.0 * unit.0 + offset.1 * unit.1;
            if off_line > ON_EDGE_PX || along < length {
                break; // the outline leaves the edge's line or turns back
            } // if off the edge
            length = along;
            k = next;
        } // while not at a turn point
        (length, unit.1.abs() <= HORIZONTAL_SINE)
    } // fn edge

    // @brief True when corner `a` ranks above corner `b`: longer primary edge, longer secondary edge, lower,
    //        then left, each by more than `SAME_PX`.
    fn ranks_above(&self, a: &Corner, b: &Corner) -> bool {
        if (a.primary - b.primary).abs() > SAME_PX {
            return a.primary > b.primary; // longer primary edge
        } // if primary differs
        if (a.secondary - b.secondary).abs() > SAME_PX {
            return a.secondary > b.secondary; // longer other edge
        } // if secondary differs
        lower_or_left(self.vertices[a.index], self.vertices[b.index])
    } // fn ranks_above

    // @brief Map a grain frame point back to the piece frame: p = x'·r − y'·g, with r = (−g.y, g.x).
    fn to_piece(&self, p: FramePoint) -> Point {
        let (gx, gy) = self.grain;
        Point::new((p.0 * -gy - p.1 * gx) as f32, (p.0 * gx - p.1 * gy) as f32)
    } // fn to_piece
} // impl SizeOutline

// @brief Read one size's outline, turn points and fold line, in its grain frame.
// @details The outline is the first `cutline` group, else the first `seamline` group, else every path of the
//          piece. Its vertices are the path segment end points, with the transforms below the piece applied.
//          A turn point that names the closing vertex names the first vertex; one past the vertices is dropped.
// @param piece  Piece `<g>`.
// @param grain  Grain direction unit vector, in the piece frame.
// @return None when the outline has fewer than three vertices.
fn size_outline(piece: &Element, grain: (f64, f64)) -> Option<SizeOutline> {
    let (gx, gy) = grain;
    // x' = p·r and y' = −(p·g), with r = (−g.y, g.x).
    let to_frame = |p: Point| {
        let (x, y) = (f64::from(p.x), f64::from(p.y));
        (-x * gy + y * gx, -(x * gx + y * gy))
    }; // to_frame

    let outline = child_group(piece, "cutline").or_else(|| child_group(piece, "seamline"));
    let mut points = Vec::new();
    match outline {
        Some(group) => collect_end_points(group, Matrix2D::IDENTITY, &mut points), // the tagged outline
        None => {
            // An untagged piece: every path, without the piece's own transform.
            for child in piece.children.iter().filter_map(|n| n.as_element()) {
                collect_end_points(child, Matrix2D::IDENTITY, &mut points);
            } // for each child
        } // None
    } // match outline

    let mut vertices: Vec<FramePoint> = points.into_iter().map(to_frame).collect();
    let count = vertices.len();
    let closed = count >= 2 && distance(vertices[0], vertices[count - 1]) <= SAME_PX;
    if closed {
        vertices.pop(); // the closing vertex repeats the first one
    } // if closed
    if vertices.len() < 3 {
        return None; // no area to stack on
    } // if too few vertices

    let turn_points = outline.and_then(|g| g.attributes.get("data-turn-points")).map(|text| {
        let mut indices: Vec<usize> = text
            .split_whitespace()
            .filter_map(|t| t.parse::<usize>().ok())
            .map(|i| if closed && i == count - 1 { 0 } else { i })
            .filter(|&i| i < vertices.len())
            .collect();
        indices.sort_unstable();
        indices.dedup();
        indices
    }); // turn_points

    let fold_line = fold_line_group(piece)
        .and_then(|group| own_path_ends(group, Matrix2D::IDENTITY))
        .map(|(start, end)| (to_frame(start), to_frame(end)));

    Some(SizeOutline { vertices, turn_points, fold_line, grain })
} // fn size_outline

// @brief First direct child `<g>` of `piece` with this `data-type`.
fn child_group<'a>(piece: &'a Element, data_type: &str) -> Option<&'a Element> {
    piece
        .children
        .iter()
        .filter_map(|n| n.as_element())
        .find(|e| e.name == "g" && e.attributes.get("data-type").is_some_and(|t| t == data_type))
} // fn child_group

// @brief The element's own `transform`, or identity.
fn own_transform(elem: &Element) -> Matrix2D {
    elem.attributes.get("transform").map(|t| parse_svg_transform(t)).unwrap_or(Matrix2D::IDENTITY)
} // fn own_transform

// @brief End point of every path segment in `elem`'s subtree, in document order, with transforms applied.
// @param parent  Transform of `elem`'s parent.
fn collect_end_points(elem: &Element, parent: Matrix2D, points: &mut Vec<Point>) {
    let matrix = parent.mul(&own_transform(elem));
    if elem.name == "path" {
        points.extend(path_end_points(elem).into_iter().map(|p| matrix.apply_to_point(p)));
    } // if path
    for child in elem.children.iter().filter_map(|n| n.as_element()) {
        collect_end_points(child, matrix, points);
    } // for each child
} // fn collect_end_points

// @brief End point of every segment of a `<path>`, untransformed. A close adds no point.
fn path_end_points(path: &Element) -> Vec<Point> {
    let Some(parsed) = path.attributes.get("d").and_then(|d| Path::parse_path_attribute(d).ok()) else {
        return Vec::new(); // no path data
    }; // parsed
    parsed
        .segments
        .iter()
        .filter_map(|segment| match segment {
            PathSegment::MoveTo(p) | PathSegment::LineTo(p) => Some(*p),
            PathSegment::QuadTo { to, .. } | PathSegment::CubicTo { to, .. } | PathSegment::ArcTo { to, .. } => Some(*to),
            PathSegment::Close => None, // no new point
        }) // filter_map
        .collect()
} // fn path_end_points

// @brief First and last point of the first `<path>` in `elem`'s subtree, outside child groups with their own
//        `data-type`, with transforms applied. `fold_line_d` reads the same path.
// @param parent  Transform of `elem`'s parent.
// @return None when no such path has two or more points.
fn own_path_ends(elem: &Element, parent: Matrix2D) -> Option<(Point, Point)> {
    let matrix = parent.mul(&own_transform(elem));
    if elem.name == "path" {
        let points = path_end_points(elem);
        if points.len() < 2 {
            return None; // no line
        } // if too few points
        return Some((matrix.apply_to_point(points[0]), matrix.apply_to_point(points[points.len() - 1])));
    } // if path
    elem.children
        .iter()
        .filter_map(|n| n.as_element())
        .filter(|c| !c.attributes.contains_key("data-type")) // a tagged part is not the fold line
        .find_map(|c| own_path_ends(c, matrix))
} // fn own_path_ends

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use crate::Document;

    // @brief Size outline in a grain-up frame from SVG points, with turn points and no fold line.
    fn outline(points: &[(f64, f64)], turn_points: &[usize]) -> SizeOutline {
        SizeOutline {
            vertices: points.to_vec(),
            turn_points: Some(turn_points.to_vec()),
            fold_line: None,
            grain: GRAIN_UP,
        }
    } // fn outline

    // @brief Piece `<g>` elements of the first piece-set of `svg`.
    fn pieces(doc: &Document) -> Vec<&Element> {
        let set = doc.root.children.iter().find_map(|n| n.as_element()).expect("piece-set");
        set.children.iter().filter_map(|n| n.as_element()).collect()
    } // fn pieces

    // @brief Stack points of the first piece-set of `svg`, every piece with its grainline up.
    fn points_of(svg: &str) -> Vec<Option<(f32, f32)>> {
        let doc = Document::parse(svg).expect("fixture parses");
        let input: Vec<(&Element, (f64, f64))> = pieces(&doc).into_iter().map(|p| (p, GRAIN_UP)).collect();
        stack_points(&input).into_iter().map(|p| p.map(|p| (p.x, p.y))).collect()
    } // fn points_of

    // @brief Two trapezoid sizes, wide edge at the top: the stack point is the top left corner.
    const TRAPEZOIDS: &str = r#"<svg>
  <g data-type="piece-set">
    <g data-type="piece" data-size="34">
      <g data-type="cutline" data-turn-points="0 1 2 3 4"><path d="M 0,0 L 40,0 L 30,30 L 10,30 Z"/></g>
    </g>
    <g data-type="piece" data-size="36">
      <g data-type="cutline" data-turn-points="0 1 2 3 4"><path d="M 100,100 L 180,100 L 160,160 L 120,160 Z"/></g>
    </g>
  </g>
</svg>"#;

    // @brief The longest horizontal edge wins; the left end of a level edge wins the tie.
    #[test]
    fn longest_horizontal_edge_wins() {
        assert_eq!(points_of(TRAPEZOIDS), vec![Some((0.0, 0.0)), Some((100.0, 100.0))]);
    } // longest_horizontal_edge_wins

    // @brief The closing vertex index names the first vertex; an index past the vertices is dropped.
    #[test]
    fn closing_index_names_the_first_vertex() {
        let svg = TRAPEZOIDS
            .replace("M 100,100 L 180,100 L 160,160 L 120,160 Z", "M 100,100 L 180,100 L 160,160 L 120,160 L 100,100 Z")
            .replace(r#"data-turn-points="0 1 2 3 4""#, r#"data-turn-points="2 3 4 9""#);
        let doc = Document::parse(&svg).expect("fixture parses");
        let size = size_outline(pieces(&doc)[1], GRAIN_UP).expect("outline");
        assert_eq!(size.vertices.len(), 4, "the closing vertex is dropped");
        assert_eq!(size.turn_points, Some(vec![0, 2, 3]));
    } // closing_index_names_the_first_vertex

    // @brief A size without turn points puts every size on its bounding-box centre.
    #[test]
    fn size_without_turn_points_centres_every_size() {
        let svg = TRAPEZOIDS.replacen(r#" data-turn-points="0 1 2 3 4""#, "", 1);
        assert_eq!(points_of(&svg), vec![Some((20.0, 15.0)), Some((140.0, 130.0))]);
    } // size_without_turn_points_centres_every_size

    // @brief The edges run on through vertices on their line, up to the next turn point.
    #[test]
    fn edge_runs_through_vertices_on_its_line() {
        // Bottom edge 0→1→2 (100 long) has a vertex at 50 that is not a turn point. The top edge is 60 long.
        let size = outline(&[(0.0, 100.0), (50.0, 100.2), (100.0, 100.0), (80.0, 0.0), (20.0, 0.0)], &[0, 2, 3, 4]);
        let corner = size.corner(0);
        assert!(corner.horizontal);
        assert!((corner.primary - 100.0).abs() < 0.01, "{}", corner.primary);
        // Bottom corners 0 and 2 tie on both edges and on height, so the left one wins.
        assert_eq!(best_corner(&size), Some(0));

        // A turn point at the middle vertex stops the edge there.
        let split = outline(&size.vertices, &[0, 1, 2, 3, 4]);
        assert!((split.corner(0).primary - 50.0).abs() < 0.01, "{}", split.corner(0).primary);
        assert_eq!(best_corner(&split), Some(4), "the top edge (60) is now the longest horizontal edge");
    } // edge_runs_through_vertices_on_its_line

    // @brief With equal horizontal edges, the longer other edge wins, then the lower corner.
    #[test]
    fn ties_take_the_longer_other_edge_then_the_lower_corner() {
        // Top corners 0 and 1 share the horizontal edge (60). Corner 1's other edge (100) is longer than
        // corner 0's (40), so corner 1 wins, although corner 0 is left of it.
        let size = outline(&[(0.0, 0.0), (60.0, 0.0), (60.0, 100.0), (0.0, 40.0)], &[0, 1, 2, 3]);
        assert_eq!(best_corner(&size), Some(1));

        // A rectangle: every corner ties on both edges, so a lower corner wins, then the left one.
        let rectangle = outline(&[(0.0, 0.0), (40.0, 0.0), (40.0, 80.0), (0.0, 80.0)], &[0, 1, 2, 3]);
        assert_eq!(best_corner(&rectangle), Some(3));
    } // ties_take_the_longer_other_edge_then_the_lower_corner

    // @brief With no horizontal edge, the corner with the longest edge wins.
    #[test]
    fn without_a_horizontal_edge_the_longest_edge_wins() {
        // The edge from 3 to 0 (~121.7) is the longest. Corner 3's other edge (~92.2) is longer than
        // corner 0's (~31.6), so corner 3 wins.
        let size = outline(&[(0.0, 0.0), (30.0, 10.0), (40.0, 50.0), (-20.0, 120.0)], &[0, 1, 2, 3]);
        assert_eq!(best_corner(&size), Some(3));
    } // without_a_horizontal_edge_the_longest_edge_wins

    // @brief The grain frame decides what is horizontal and lower. A 40 x 80 rectangle with its grain up
    //        stacks on its lower left corner. With its grain pointing right, its 80 px SVG-vertical edges are
    //        horizontal, and the lower left corner of the turned piece is SVG (0, 0).
    #[test]
    fn grain_frame_turns_the_rules() {
        let svg = r#"<svg><g data-type="piece-set"><g data-type="piece">
  <g data-type="cutline" data-turn-points="0 1 2 3"><path d="M 0,0 L 40,0 L 40,80 L 0,80 Z"/></g>
</g></g></svg>"#;
        let doc = Document::parse(svg).expect("fixture parses");
        let rectangle = pieces(&doc)[0];
        assert_eq!(stack_points(&[(rectangle, GRAIN_UP)]), vec![Some(Point::new(0.0, 80.0))]);
        assert_eq!(stack_points(&[(rectangle, (1.0, 0.0))]), vec![Some(Point::new(0.0, 0.0))]);
    } // grain_frame_turns_the_rules

    // @brief Every size with a fold line stacks on the lower end, the same end in every size.
    #[test]
    fn fold_line_lower_end_wins() {
        let svg = r#"<svg>
  <g data-type="piece-set">
    <g data-type="piece" data-size="34">
      <g data-type="cutline" data-turn-points="0 1 2 3 4"><path d="M 0,0 L 30,0 L 30,60 L 0,60 Z"/></g>
      <g data-type="fold_line"><g data-type="fold_line_arrow"><path d="M 2,40 L 5,40"/></g><path d="M 0,0 L 0,60"/></g>
    </g>
    <g data-type="piece" data-size="36">
      <g data-type="cutline" data-turn-points="0 1 2 3 4"><path d="M 100,100 L 140,100 L 140,180 L 100,180 Z"/></g>
      <g data-type="fold_line" transform="translate(1,0)"><path d="M 99,100 L 99,180"/></g>
    </g>
  </g>
</svg>"#;
        assert_eq!(points_of(svg), vec![Some((0.0, 60.0)), Some((100.0, 180.0))]);
    } // fold_line_lower_end_wins

    // @brief A level fold line stacks on its left end.
    #[test]
    fn level_fold_line_left_end_wins() {
        assert!(lower_or_left((0.0, 5.0), (10.0, 5.0)));
        assert!(!lower_or_left((10.0, 5.0), (0.0, 5.0)));
        assert!(lower_or_left((10.0, 6.0), (0.0, 5.0)));
    } // level_fold_line_left_end_wins

    // @brief The other sizes take the turn point nearest the largest size's corner place.
    #[test]
    fn other_sizes_take_the_nearest_place() {
        // The small size has an extra turn point near the top left, at 10% of its width.
        let svg = TRAPEZOIDS.replace(
            r#"<g data-type="cutline" data-turn-points="0 1 2 3 4"><path d="M 0,0 L 40,0 L 30,30 L 10,30 Z"/>"#,
            r#"<g data-type="cutline" data-turn-points="0 1 2 3 4 5"><path d="M 4,0 L 40,0 L 30,30 L 10,30 L 0,0 Z"/>"#,
        );
        // Largest size: top left (0, 0) of its bounding box. Small size: vertex 4 at (0,0), not vertex 0.
        assert_eq!(points_of(&svg), vec![Some((0.0, 0.0)), Some((100.0, 100.0))]);
    } // other_sizes_take_the_nearest_place

    // @brief An outline with fewer than three vertices has no stack point.
    #[test]
    fn short_outline_has_no_stack_point() {
        let svg = TRAPEZOIDS.replacen("M 0,0 L 40,0 L 30,30 L 10,30 Z", "M 0,0 L 40,0", 1);
        assert_eq!(points_of(&svg), vec![None, Some((100.0, 100.0))]);
    } // short_outline_has_no_stack_point
} // mod tests
