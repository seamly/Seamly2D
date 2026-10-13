// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Tests for grading: shared key points across sizes, rules, and the reference line.

#[cfg(test)]
mod tests {
    use crate::astm_contour::{build_contour, build_contour_tagged, distance, grade_contours, spline_deviation, AstmContour, CURVE_TOLERANCE_MM};
    use crate::converter::{svg_to_ezdxf, SvgToEzdxfOptions};
    use crate::drawing::{Block, Drawing};
    use crate::entities::Point;
    use crate::grade_drawing;
    use svg_dom::Document;

    // @brief Closed outline: straight top and sides, a curved hem, scaled by `k` about the origin.
    // @return (points, turn tags): the four corners are turn points.
    fn hem_outline(k: f64) -> (Vec<Point>, Vec<bool>) {
        let mut points = vec![Point::new(0.0, 0.0), Point::new(200.0 * k, 0.0), Point::new(200.0 * k, 300.0 * k)];
        for i in 1..40 {
            // Circular hem from (200k, 300k) to (0, 300k), bulging 60k.
            let t = i as f64 / 40.0;
            let x = 200.0 * k * (1.0 - t);
            points.push(Point::new(x, 300.0 * k + 60.0 * k * (std::f64::consts::PI * t).sin()));
        } // for each hem vertex
        points.push(Point::new(0.0, 300.0 * k));
        let mut tags = vec![false; points.len()];
        for i in [0, 1, 2, points.len() - 1] {
            tags[i] = true;
        } // for each corner
        (points, tags)
    }

    // @brief Contour of hem_outline(k).
    fn hem_contour(k: f64) -> AstmContour {
        let (points, tags) = hem_outline(k);
        build_contour_tagged(&points, true, Some(&tags)).expect("contour")
    }

    // @brief True when `reduced` is an ordered subset of `dense` that starts at dense[0].
    fn ordered_subset(c: &AstmContour) -> bool {
        let mut next = 0;
        for p in &c.reduced {
            match c.dense[next..].iter().position(|q| q == p) {
                Some(i) => next += i + 1,
                None => return false,
            }
        }
        c.reduced[0] == c.dense[0]
    }

    #[test]
    fn graded_sizes_share_key_points() {
        let sizes = [hem_contour(0.9), hem_contour(1.0), hem_contour(1.2)];
        let graded = grade_contours(&sizes, 1).expect("same turn points");
        assert_eq!(graded.len(), 3);
        let k = graded[1].reduced.len();
        for (g, scale) in graded.iter().zip([0.9, 1.0, 1.2]) {
            assert_eq!(g.reduced.len(), k, "same key point count in every size");
            assert_eq!(g.turn, graded[1].turn, "same turn flags in every size");
            assert_eq!(g.turn.iter().filter(|&&t| t).count(), 4, "four corners");
            assert!(ordered_subset(g), "reduced is an ordered subset of dense");
            assert!(spline_deviation(g) <= CURVE_TOLERANCE_MM + 1e-9, "spline within tolerance");
            // A scaled outline keeps arc-length fractions: key point i is the scaled sample key point i.
            for (p, q) in g.reduced.iter().zip(&graded[1].reduced) {
                assert!(distance(*p, Point::new(q.x * scale, q.y * scale)) < 1e-6, "{p:?} vs {q:?} x {scale}");
            }
        }
    }

    // @brief hem_contour(k) with each hem vertex moved `shift` mm along the hem.
    // @details The sample's key point fractions then land `shift` mm from this size's hem vertices.
    fn shifted_hem_contour(k: f64, shift: f64) -> AstmContour {
        let (mut points, tags) = hem_outline(k);
        for i in 1..40 {
            // Hem speed at t: d/dt of (200k(1 - t), 300k + 60k sin(pi t)).
            let t = i as f64 / 40.0;
            let speed = ((200.0 * k).powi(2) + (60.0 * k * std::f64::consts::PI * (std::f64::consts::PI * t).cos()).powi(2)).sqrt();
            let t = t + shift / speed;
            points[2 + i] = Point::new(200.0 * k * (1.0 - t), 300.0 * k + 60.0 * k * (std::f64::consts::PI * t).sin());
        } // for each hem vertex
        build_contour_tagged(&points, true, Some(&tags)).expect("contour")
    }

    // @brief Smallest distance between consecutive vertices of a closed polyline.
    fn closest_neighbours(points: &[Point]) -> f64 {
        (0..points.len()).map(|i| distance(points[i], points[(i + 1) % points.len()])).fold(f64::INFINITY, f64::min)
    }

    #[test]
    fn graded_key_points_snap_to_nearby_vertices() {
        // Key points of the sample land 0.005 mm from the hem vertices of the other sizes.
        let sizes = [shifted_hem_contour(0.9, 0.005), hem_contour(1.0), shifted_hem_contour(1.2, -0.005)];
        let graded = grade_contours(&sizes, 1).expect("same turn points");
        for g in &graded {
            assert_eq!(g.reduced.len(), graded[1].reduced.len(), "same key point count in every size");
            assert!(ordered_subset(g), "reduced is an ordered subset of dense");
            assert!(closest_neighbours(&g.dense) >= 0.01, "dense vertices {} mm apart", closest_neighbours(&g.dense));
            assert!(closest_neighbours(&g.reduced) >= 0.01, "key points {} mm apart", closest_neighbours(&g.reduced));
            assert!(spline_deviation(g) <= CURVE_TOLERANCE_MM + 1e-9, "spline within tolerance");
        }
    }

    #[test]
    fn different_turn_points_do_not_grade() {
        let square = build_contour(
            &[Point::new(0.0, 0.0), Point::new(100.0, 0.0), Point::new(100.0, 100.0), Point::new(0.0, 100.0)],
            true,
        )
        .expect("square");
        let triangle = build_contour(&[Point::new(0.0, 0.0), Point::new(100.0, 0.0), Point::new(50.0, 80.0)], true).expect("triangle");
        assert!(grade_contours(&[square, triangle], 0).is_none(), "4 vs 3 turn points");
    }

    #[test]
    fn graded_open_line_keeps_curve_point_ends() {
        let arc = |k: f64| -> AstmContour {
            let points: Vec<Point> = (0..=30)
                .map(|i| {
                    let a = std::f64::consts::PI * i as f64 / 30.0;
                    Point::new(80.0 * k * a.cos(), 80.0 * k * a.sin())
                })
                .collect();
            build_contour(&points, false).expect("arc")
        };
        let graded = grade_contours(&[arc(1.0), arc(1.1)], 0).expect("arcs grade");
        assert_eq!(graded[0].reduced.len(), graded[1].reduced.len());
        for g in &graded {
            assert!(!g.turn[0] && !g.turn[g.turn.len() - 1], "open ends are curve points");
            assert!(spline_deviation(g) <= CURVE_TOLERANCE_MM + 1e-9);
        }
    }

    // @brief Fixture document from ezdxf2dxfastm/test_data.
    fn fixture(name: &str) -> Document {
        let path = format!("{}/../ezdxf2dxfastm/test_data/{name}", env!("CARGO_MANIFEST_DIR"));
        Document::parse(&std::fs::read_to_string(path).expect("fixture present")).expect("fixture parses")
    }

    // @brief Sizes of the grading fixtures, in grading order.
    fn fixture_sizes() -> Vec<String> {
        ["34", "36", "38"].iter().map(|s| s.to_string()).collect()
    }

    // @brief Layout drawing of the grading fixture, only the pieces of `size`, and the input drawing.
    fn fixture_drawings(size: &str) -> (Drawing, Drawing) {
        let options = SvgToEzdxfOptions::default();
        let mut layout = svg_to_ezdxf(&fixture("multisize_grading_layout.svg"), &options).expect("layout converts");
        layout.blocks.retain(|b| b.size.as_deref() == Some(size));
        let input = svg_to_ezdxf(&fixture("multisize_grading_input.svg"), &options).expect("input converts");
        (layout, input)
    }

    // @brief The base size layout of the grading fixtures, graded; also the input drawing.
    fn graded_fixture() -> (Drawing, Drawing, Vec<String>) {
        let (mut layout, input) = fixture_drawings("36");
        let warnings = grade_drawing(&mut layout, &input, &fixture_sizes(), "36");
        (layout, input, warnings)
    }

    // @brief The block named `name`.
    fn block<'a>(drawing: &'a Drawing, name: &str) -> &'a Block {
        drawing.blocks.iter().find(|b| b.name == name).unwrap_or_else(|| panic!("{name} in the drawing"))
    }

    // @brief Points of a block in its grade reference line frame: origin at the line start, X along it.
    fn in_reference_frame(block: &Block, points: &[Point]) -> Vec<Point> {
        let (a, b) = block.grading.as_ref().expect("graded").reference_line;
        let length = distance(a, b);
        let (cos, sin) = ((b.x - a.x) / length, (b.y - a.y) / length);
        points
            .iter()
            .map(|p| {
                let (dx, dy) = (p.x - a.x, p.y - a.y);
                Point::new(cos * dx + sin * dy, -sin * dx + cos * dy)
            })
            .collect()
    }

    // @brief Distance from `p` to a closed polyline.
    fn distance_to_loop(p: Point, polyline: &[Point]) -> f64 {
        (0..polyline.len())
            .map(|i| {
                let (a, b) = (polyline[i], polyline[(i + 1) % polyline.len()]);
                let (ux, uy) = (b.x - a.x, b.y - a.y);
                let length2 = ux * ux + uy * uy;
                let t = if length2 == 0.0 { 0.0 } else { (((p.x - a.x) * ux + (p.y - a.y) * uy) / length2).clamp(0.0, 1.0) };
                distance(p, Point::new(a.x + t * ux, a.y + t * uy))
            })
            .fold(f64::INFINITY, f64::min)
    }

    #[test]
    fn grade_drawing_grades_the_sample_size_pieces() {
        let (layout, _, warnings) = graded_fixture();
        assert_eq!(layout.blocks.len(), 2, "one block per piece, no other sizes");
        // Back size 38 has one more notch: it stays ungraded, with a warning.
        assert_eq!(warnings.len(), 1, "{warnings:?}");
        assert!(warnings[0].starts_with("Back:") && warnings[0].contains("notch"), "{}", warnings[0]);
        assert!(!warnings[0].contains("Only size"), "the sample size is the only size written: {}", warnings[0]);
        assert!(block(&layout, "piece_Back_s36_M").grading.is_none(), "ungradable piece is ungraded");

        let front = block(&layout, "piece_Front_s36_M");
        let grading = front.grading.as_ref().expect("Front graded");
        assert!(distance(grading.reference_line.0, grading.reference_line.1) >= 10.0);
        assert_eq!(front.sew_lines.len(), 1);
        let rule_texts = &grading.rule_texts;
        assert_eq!(rule_texts.iter().filter(|t| t.layer == "1").count(), front.boundary.as_ref().unwrap().reduced.len(), "one # n per boundary key point");
        assert_eq!(rule_texts.iter().filter(|t| t.layer == "14").count(), front.sew_lines[0].reduced.len(), "one # n per sew line key point");
        assert_eq!(rule_texts.iter().filter(|t| t.layer == "4").count(), front.notches.len(), "one # n per notch");

        let table = layout.grade_rule_table.as_ref().expect("rule table");
        assert_eq!(table.sample_size, "36");
        assert_eq!(table.sizes, vec!["34", "36", "38"]);
    }

    #[test]
    fn rules_move_the_sample_onto_every_size() {
        let (layout, input, _) = graded_fixture();
        let table = layout.grade_rule_table.as_ref().expect("rule table");
        let sample = block(&layout, "piece_Front_s36_M");
        let boundary_rules: Vec<i32> = sample.grading.as_ref().unwrap().rule_texts.iter().filter(|t| t.layer == "1").map(|t| t.rule).collect();
        let local_sample = in_reference_frame(sample, &sample.boundary.as_ref().unwrap().reduced);

        // The reference line starts at the bottom left of the drafted sample boundary.
        let drafted = &block(&input, "piece_Front_s36_M").boundary.as_ref().unwrap().dense;
        let origin = Point::new(
            drafted.iter().map(|p| p.x).fold(f64::INFINITY, f64::min),
            drafted.iter().map(|p| p.y).fold(f64::INFINITY, f64::min),
        );
        for (s, size) in table.sizes.iter().enumerate() {
            let outline: Vec<Point> = block(&input, &format!("piece_Front_s{size}_M")).boundary.as_ref().unwrap().dense
                .iter()
                .map(|p| Point::new(p.x - origin.x, p.y - origin.y))
                .collect();
            for (i, rule_id) in boundary_rules.iter().enumerate() {
                let rule = table.rules.iter().find(|r| r.id == *rule_id).expect("rule in table");
                let (dx, dy) = rule.deltas[s];
                let moved = Point::new(local_sample[i].x + dx, local_sample[i].y + dy);
                let off = distance_to_loop(moved, &outline);
                assert!(off < 0.02, "size {size} key point {i}: {off} mm off the drafted outline");
            }
        }
        // The sample size never grows; the drafted corner at the piece offset grows by nothing in any size.
        assert!(table.rules.iter().all(|r| r.deltas[1] == (0.0, 0.0)));
        assert!(table.rules.iter().any(|r| r.deltas.iter().all(|d| d.0.abs() < 1e-6 && d.1.abs() < 1e-6)), "zero growth rule");
        // The top right corner moves along the top edge: 5% of 200 px either way.
        let step = 0.05 * 200.0 * crate::utils::MM_PER_PX;
        assert!(
            table.rules.iter().any(|r| (r.deltas[0].0 + step).abs() < 0.01 && r.deltas[0].1.abs() < 0.01 && (r.deltas[2].0 - step).abs() < 0.01),
            "top right corner rule"
        );
        // Identifiers count up from 1; equal rules share one.
        for (i, a) in table.rules.iter().enumerate() {
            assert_eq!(a.id, i as i32 + 1);
        }
        let texts = &sample.grading.as_ref().unwrap().rule_texts;
        assert!(table.rules.len() < texts.len(), "{} rules for {} points", table.rules.len(), texts.len());
    }

    #[test]
    fn other_sizes_stay_ungraded() {
        let options = SvgToEzdxfOptions::default();
        let mut layout = svg_to_ezdxf(&fixture("multisize_grading_layout.svg"), &options).expect("layout converts");
        let input = svg_to_ezdxf(&fixture("multisize_grading_input.svg"), &options).expect("input converts");
        let names: Vec<String> = layout.blocks.iter().map(|b| b.name.clone()).collect();
        grade_drawing(&mut layout, &input, &fixture_sizes(), "36");
        let after: Vec<String> = layout.blocks.iter().map(|b| b.name.clone()).collect();
        assert_eq!(after, names, "no block added, dropped or moved");
        for b in &layout.blocks {
            assert_eq!(b.grading.is_some(), b.name == "piece_Front_s36_M", "{}", b.name);
        }
    }

    #[test]
    fn layout_without_the_sample_size_grades_nothing() {
        let (mut layout, input) = fixture_drawings("34");
        let warnings = grade_drawing(&mut layout, &input, &fixture_sizes(), "36");
        assert!(warnings.is_empty(), "{warnings:?}");
        assert!(layout.blocks.iter().all(|b| b.grading.is_none()));
        assert!(layout.grade_rule_table.is_none(), "no piece graded, no table");
    }
}
