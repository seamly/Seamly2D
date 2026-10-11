// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Tests for graded nests: shared key points across sizes, rules, and the reference line.

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

    // @brief Layout and input drawings of the grading fixtures, graded.
    fn graded_fixture() -> (Drawing, Vec<String>) {
        let options = SvgToEzdxfOptions::default();
        let mut layout = svg_to_ezdxf(&fixture("multisize_grading_layout.svg"), &options).expect("layout converts");
        let input = svg_to_ezdxf(&fixture("multisize_grading_input.svg"), &options).expect("input converts");
        let sizes: Vec<String> = ["34", "36", "38"].iter().map(|s| s.to_string()).collect();
        let warnings = grade_drawing(&mut layout, &input, &sizes, "36");
        (layout, warnings)
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

    #[test]
    fn grade_drawing_builds_a_nest_in_size_order() {
        let (layout, warnings) = graded_fixture();
        let names: Vec<&str> = layout.blocks.iter().map(|b| b.name.as_str()).collect();
        assert_eq!(names, vec!["piece_Front_s34_M", "piece_Front_s36_M", "piece_Front_s38_M", "piece_Back_s36_M"]);
        // Back size 38 has one more notch: only its sample size stays, with a warning.
        assert_eq!(warnings.len(), 1, "{warnings:?}");
        assert!(warnings[0].starts_with("Back:") && warnings[0].contains("notch"), "{}", warnings[0]);
        assert!(layout.blocks[3].grading.is_none(), "fallen-back piece is ungraded");

        let front = &layout.blocks[..3];
        assert!(front.iter().all(|b| b.piece_name == "Front"), "one piece name per nest");
        let samples: Vec<bool> = front.iter().map(|b| b.grading.as_ref().expect("graded").sample).collect();
        assert_eq!(samples, vec![false, true, false]);
        let key_count = front[1].boundary.as_ref().unwrap().reduced.len();
        for block in front {
            assert_eq!(block.boundary.as_ref().unwrap().reduced.len(), key_count, "{}", block.name);
            assert_eq!(block.sew_lines.len(), 1);
            let grading = block.grading.as_ref().unwrap();
            assert!(distance(grading.reference_line.0, grading.reference_line.1) >= 10.0);
            assert_eq!(grading.rule_texts.is_empty(), !grading.sample, "only the sample block carries # n");
        }
        let table = layout.grade_rule_table.as_ref().expect("rule table");
        assert_eq!(table.sample_size, "36");
        assert_eq!(table.sizes, vec!["34", "36", "38"]);
    }

    #[test]
    fn stacking_on_the_reference_line_matches_the_rules() {
        let (layout, _) = graded_fixture();
        let table = layout.grade_rule_table.as_ref().expect("rule table");
        let front = &layout.blocks[..3];
        let sample = &front[1];
        let rule_texts = &sample.grading.as_ref().unwrap().rule_texts;
        let boundary_rules: Vec<i32> = rule_texts.iter().filter(|t| t.layer == "1").map(|t| t.rule).collect();
        assert_eq!(boundary_rules.len(), sample.boundary.as_ref().unwrap().reduced.len(), "one # n per boundary key point");
        assert_eq!(rule_texts.iter().filter(|t| t.layer == "4").count(), sample.notches.len(), "one # n per notch");
        assert_eq!(rule_texts.iter().filter(|t| t.layer == "14").count(), sample.sew_lines[0].reduced.len());

        let local_sample = in_reference_frame(sample, &sample.boundary.as_ref().unwrap().reduced);
        for (s, block) in front.iter().enumerate() {
            let local = in_reference_frame(block, &block.boundary.as_ref().unwrap().reduced);
            for (i, rule_id) in boundary_rules.iter().enumerate() {
                let rule = table.rules.iter().find(|r| r.id == *rule_id).expect("rule in table");
                let (dx, dy) = rule.deltas[s];
                let (gx, gy) = (local[i].x - local_sample[i].x, local[i].y - local_sample[i].y);
                assert!((gx - dx).abs() < 0.02 && (gy - dy).abs() < 0.02, "size {s} point {i}: nest ({gx}, {gy}) vs rule ({dx}, {dy})");
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
        // Equal rules share an identifier.
        for (i, a) in table.rules.iter().enumerate() {
            assert_eq!(a.id, i as i32 + 1);
        }
    }

    #[test]
    fn missing_sample_size_leaves_the_piece_ungraded() {
        let options = SvgToEzdxfOptions::default();
        let mut layout = svg_to_ezdxf(&fixture("multisize_grading_layout.svg"), &options).expect("layout converts");
        layout.blocks.retain(|b| b.name != "piece_Front_s36_M");
        let input = svg_to_ezdxf(&fixture("multisize_grading_input.svg"), &options).expect("input converts");
        let sizes: Vec<String> = ["34", "36", "38"].iter().map(|s| s.to_string()).collect();
        let warnings = grade_drawing(&mut layout, &input, &sizes, "36");
        assert!(warnings.iter().any(|w| w.starts_with("Front:") && w.contains("size 36 is not in the layout")), "{warnings:?}");
        assert!(layout.blocks.iter().filter(|b| b.piece_name == "Front").all(|b| b.grading.is_none()));
        assert!(layout.grade_rule_table.is_none(), "no piece graded, no table");
    }
}
