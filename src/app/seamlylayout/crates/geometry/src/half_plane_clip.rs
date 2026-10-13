// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file half_plane_clip.rs
// @brief Cut straight-line outlines with an axis-parallel line.
//
// A cut line `x = at` (vertical) or `y = at` (horizontal) splits the plane into
// a low side (coordinate < at) and a high side (coordinate >= at).  Each output
// vertex records the input vertex it came from, so per-vertex data such as turn
// point indices survives the cut.  A vertex on the cut line has no source.
//
// Closed outlines must be simple polygons.  A concave polygon can give more than
// one polygon on a side.  The caller keeps the cut line off the input vertices:
// a vertex exactly on the line belongs to the high side and gives a zero-length edge.

// @brief An axis-parallel cut line.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct CutLine {
    // True for `x = at`, false for `y = at`.
    pub vertical: bool,
    // Position of the line.
    pub at: f64,
}

impl CutLine {
    // @brief The coordinate the line cuts: x for a vertical line, y for a horizontal one.
    fn across(&self, p: (f64, f64)) -> f64 {
        if self.vertical { p.0 } else { p.1 }
    } // fn across

    // @brief The coordinate along the line: y for a vertical line, x for a horizontal one.
    fn along(&self, p: (f64, f64)) -> f64 {
        if self.vertical { p.1 } else { p.0 }
    } // fn along

    // @brief True when `p` is on the high side.
    pub fn is_high(&self, p: (f64, f64)) -> bool {
        self.across(p) >= self.at
    } // fn is_high

    // @brief Point where segment `a`-`b` crosses the line; exactly on the line.
    fn crossing(&self, a: (f64, f64), b: (f64, f64)) -> (f64, f64) {
        let (ca, cb) = (self.across(a), self.across(b));
        let t = (self.at - ca) / (cb - ca);
        let along = self.along(a) + t * (self.along(b) - self.along(a));
        if self.vertical { (self.at, along) } else { (along, self.at) }
    } // fn crossing
} // impl CutLine

// @brief One vertex of a cut result.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct CutVertex {
    // Position.
    pub point: (f64, f64),
    // Index of the input vertex at this position; None for a new vertex on the cut line.
    pub source: Option<usize>,
}

// @brief Polygons or polylines on each side of a cut line.
#[derive(Debug, Clone, Default, PartialEq)]
pub struct CutSides {
    // Results on the low side (coordinate < at).
    pub low: Vec<Vec<CutVertex>>,
    // Results on the high side (coordinate >= at).
    pub high: Vec<Vec<CutVertex>>,
}

// @brief Cut a closed simple polygon.
//
// The boundary splits into chains at each crossing.  Sorted along the cut line,
// the crossings pair up: each pair bounds one stretch of the line inside the
// polygon.  A side's polygon follows one of its chains, then the line to the
// paired crossing, then the chain that starts there, until it closes.
//
// @param points Polygon vertices, without a repeated closing vertex.
// @return Closed polygons on each side, without a repeated closing vertex.
pub fn cut_closed(points: &[(f64, f64)], line: CutLine) -> CutSides {
    let n = points.len();
    let mut sides = CutSides::default();
    if n == 0 {
        return sides;
    }
    let high: Vec<bool> = points.iter().map(|&p| line.is_high(p)).collect();

    // Crossings in boundary order: (edge start index, point on the line).
    let crossings: Vec<(usize, (f64, f64))> = (0..n)
        .filter(|&i| high[i] != high[(i + 1) % n])
        .map(|i| (i, line.crossing(points[i], points[(i + 1) % n])))
        .collect();

    // No crossing: the whole polygon is on one side.
    if crossings.is_empty() {
        let whole: Vec<CutVertex> =
            points.iter().enumerate().map(|(i, &p)| CutVertex { point: p, source: Some(i) }).collect();
        if high[0] { sides.high.push(whole) } else { sides.low.push(whole) }
        return sides;
    }

    // Pair the crossings along the line: 0-1, 2-3, ...
    let m = crossings.len();
    let mut order: Vec<usize> = (0..m).collect();
    order.sort_by(|&a, &b| line.along(crossings[a].1).total_cmp(&line.along(crossings[b].1)));
    let mut partner: Vec<usize> = (0..m).collect();
    for pair in order.chunks(2) {
        if let [a, b] = *pair {
            partner[a] = b;
            partner[b] = a;
        }
    }

    // Chain k starts at crossing k and holds the vertices up to the start of edge k+1.
    let chain_vertices = |k: usize| -> Vec<usize> {
        let last = crossings[(k + 1) % m].0;
        let mut j = (crossings[k].0 + 1) % n;
        let mut out = vec![j];
        while j != last {
            j = (j + 1) % n;
            out.push(j);
        }
        out
    }; // chain_vertices
    let chain_is_high = |k: usize| high[(crossings[k].0 + 1) % n];

    // Walk each unvisited chain into one closed polygon of its side.
    let mut visited = vec![false; m];
    for first in 0..m {
        if visited[first] {
            continue;
        }
        let side_high = chain_is_high(first);
        let mut polygon: Vec<CutVertex> = Vec::new();
        let mut k = first;
        loop {
            visited[k] = true;
            polygon.push(CutVertex { point: crossings[k].1, source: None });
            for j in chain_vertices(k) {
                polygon.push(CutVertex { point: points[j], source: Some(j) });
            }
            let end = (k + 1) % m;
            polygon.push(CutVertex { point: crossings[end].1, source: None });
            // Along the line to the paired crossing; its chain is on the same side.
            let next = partner[end];
            if next == first || visited[next] || chain_is_high(next) != side_high {
                break;
            }
            k = next;
        } // loop over the chains of one polygon
        if side_high { sides.high.push(polygon) } else { sides.low.push(polygon) }
    } // for each chain
    sides
} // fn cut_closed

// @brief Cut an open polyline.
//
// @param points Polyline vertices in order.
// @return Open polylines on each side.  A piece ends, and the next starts, at a new vertex on the line.
pub fn cut_open(points: &[(f64, f64)], line: CutLine) -> CutSides {
    let mut sides = CutSides::default();
    let Some(&first) = points.first() else {
        return sides;
    };
    let mut run_high = line.is_high(first);
    let mut run: Vec<CutVertex> = vec![CutVertex { point: first, source: Some(0) }];
    for i in 1..points.len() {
        let p = points[i];
        if line.is_high(p) != run_high {
            // End this run on the line and start the next one there.
            let cross = CutVertex { point: line.crossing(points[i - 1], p), source: None };
            run.push(cross);
            let done = std::mem::replace(&mut run, vec![cross]);
            if run_high { sides.high.push(done) } else { sides.low.push(done) }
            run_high = !run_high;
        }
        run.push(CutVertex { point: p, source: Some(i) });
    } // for each vertex
    if run_high { sides.high.push(run) } else { sides.low.push(run) }
    sides
} // fn cut_open

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Absolute area of a polygon (shoelace).
    fn area(polygon: &[CutVertex]) -> f64 {
        let n = polygon.len();
        (0..n)
            .map(|i| {
                let (a, b) = (polygon[i].point, polygon[(i + 1) % n].point);
                a.0 * b.1 - b.0 * a.1
            })
            .sum::<f64>()
            .abs()
            / 2.0
    } // fn area

    // @brief A square cut in the middle gives one rectangle per side, with two new corners each.
    #[test]
    fn square_cut_vertically_gives_two_halves() {
        let square = [(0.0, 0.0), (10.0, 0.0), (10.0, 10.0), (0.0, 10.0)];
        let sides = cut_closed(&square, CutLine { vertical: true, at: 4.0 });
        assert_eq!((sides.low.len(), sides.high.len()), (1, 1));
        assert!((area(&sides.low[0]) - 40.0).abs() < 1e-9);
        assert!((area(&sides.high[0]) - 60.0).abs() < 1e-9);
        for polygon in [&sides.low[0], &sides.high[0]] {
            assert_eq!(polygon.len(), 4);
            assert_eq!(polygon.iter().filter(|v| v.source.is_none()).count(), 2, "{polygon:?}");
        }
        // Original vertices keep their index.
        assert!(sides.low[0].iter().any(|v| v.source == Some(0) && v.point == (0.0, 0.0)));
        assert!(sides.high[0].iter().any(|v| v.source == Some(2) && v.point == (10.0, 10.0)));
    } // square_cut_vertically_gives_two_halves

    // @brief A U open to the right, cut through both arms: the base on the low side,
    // one polygon per arm on the high side.
    #[test]
    fn u_shape_gives_two_arms() {
        let u = [(0.0, 0.0), (10.0, 0.0), (10.0, 3.0), (3.0, 3.0), (3.0, 7.0), (10.0, 7.0), (10.0, 10.0), (0.0, 10.0)];
        let sides = cut_closed(&u, CutLine { vertical: true, at: 5.0 });
        assert_eq!((sides.low.len(), sides.high.len()), (1, 2));
        let total: f64 = sides.low.iter().chain(&sides.high).map(|p| area(p)).sum();
        assert!((total - 72.0).abs() < 1e-9, "areas add up to the U: {total}");
        for arm in &sides.high {
            assert!((area(arm) - 15.0).abs() < 1e-9);
        }
    } // u_shape_gives_two_arms

    // @brief A horizontal line, and a polygon that does not reach it.
    #[test]
    fn horizontal_cut_and_no_crossing() {
        let square = [(0.0, 0.0), (10.0, 0.0), (10.0, 10.0), (0.0, 10.0)];
        let sides = cut_closed(&square, CutLine { vertical: false, at: 2.5 });
        assert!((area(&sides.low[0]) - 25.0).abs() < 1e-9);
        assert!((area(&sides.high[0]) - 75.0).abs() < 1e-9);

        let away = cut_closed(&square, CutLine { vertical: true, at: 20.0 });
        assert_eq!((away.low.len(), away.high.len()), (1, 0));
        assert!(away.low[0].iter().all(|v| v.source.is_some()));
    } // horizontal_cut_and_no_crossing

    // @brief An open polyline crossing the line twice gives three runs; runs meet on the line.
    #[test]
    fn open_polyline_cut_into_runs() {
        let zigzag = [(0.0, 0.0), (10.0, 1.0), (0.0, 2.0)];
        let sides = cut_open(&zigzag, CutLine { vertical: true, at: 5.0 });
        assert_eq!((sides.low.len(), sides.high.len()), (2, 1));
        let high = &sides.high[0];
        assert_eq!(high.len(), 3);
        assert_eq!((high[0].source, high[1].source, high[2].source), (None, Some(1), None));
        assert_eq!(high[0].point, (5.0, 0.5));
        assert_eq!(sides.low[0].last().unwrap().point, high[0].point);
    } // open_polyline_cut_into_runs
} // mod tests
