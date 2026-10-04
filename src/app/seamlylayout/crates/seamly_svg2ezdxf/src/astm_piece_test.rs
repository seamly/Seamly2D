// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Tests for the ASTM D6673 piece model: contours, notches, and tagged-SVG extraction.

#[cfg(test)]
mod tests {
    use crate::astm_contour::{
        build_contour, build_contour_tagged, spline_deviation, spline_points, AstmContour, CURVE_TOLERANCE_MM,
    };
    use crate::astm_notch::{build_notches, NotchKind};
    use crate::converter::{svg_to_ezdxf, SvgToEzdxfOptions};
    use crate::entities::Point;
    use crate::utils::{parse_length_attr, MM_PER_PX};
    use svg_dom::Document;

    // @brief Closed square, 100 mm side, with a collinear midpoint on every side.
    fn square_with_midpoints() -> Vec<Point> {
        vec![
            Point::new(0.0, 0.0), Point::new(50.0, 0.0), Point::new(100.0, 0.0), Point::new(100.0, 50.0),
            Point::new(100.0, 100.0), Point::new(50.0, 100.0), Point::new(0.0, 100.0), Point::new(0.0, 50.0),
            Point::new(0.0, 0.0),
        ]
    }

    #[test]
    fn square_keeps_four_turn_points_and_drops_collinear_points() {
        let c = build_contour(&square_with_midpoints(), true).expect("contour");
        assert_eq!(c.dense.len(), 8, "closing repeat removed, midpoints kept for validation");
        assert_eq!(c.reduced.len(), 4, "collinear midpoints are not key points");
        assert!(c.turn.iter().all(|&t| t), "every corner is a turn point");
        assert_eq!(c.dense[0], c.reduced[0], "both views start at the same vertex");
    }

    #[test]
    fn smooth_circle_reduces_within_tolerance_to_curve_points() {
        let circle: Vec<Point> = (0..180)
            .map(|i| {
                let a = i as f64 * std::f64::consts::TAU / 180.0;
                Point::new(50.0 * a.cos(), 50.0 * a.sin())
            })
            .collect();
        let c = build_contour(&circle, true).expect("contour");
        assert_eq!(c.dense.len(), 180);
        assert!(c.reduced.len() < 60, "reduction removes most vertices, got {}", c.reduced.len());
        assert!(c.turn.iter().all(|&t| !t), "a smooth circle has only curve points");
        assert!(spline_deviation(&c) <= CURVE_TOLERANCE_MM + 1e-9);
    }

    // @brief True when `reduced` is an ordered subset of `dense` that starts at dense[0].
    fn reduced_is_ordered_subset(c: &AstmContour) -> bool {
        let mut next = 0;
        for p in &c.reduced {
            match c.dense[next..].iter().position(|q| q == p) {
                Some(i) => next += i + 1,
                None => return false,
            }
        }
        c.reduced[0] == c.dense[0]
    }

    // @brief Positions in `reduced` of curve points the spline can lose and stay within
    //        CURVE_TOLERANCE_MM. The first key point is not counted: it must stay.
    fn removable_curve_points(c: &AstmContour) -> Vec<usize> {
        (1..c.reduced.len())
            .filter(|&r| !c.turn[r])
            .filter(|&r| {
                let mut trial = c.clone();
                trial.reduced.remove(r);
                trial.turn.remove(r);
                spline_deviation(&trial) <= CURVE_TOLERANCE_MM
            })
            .collect()
    }

    // @brief Open S curve: two half sine waves, 200 mm long and 30 mm high.
    fn s_curve() -> Vec<Point> {
        (0..=400).map(|i| {
            let x = i as f64 * 0.5;
            Point::new(x, 30.0 * (x * std::f64::consts::TAU / 200.0).sin())
        }).collect()
    }

    #[test]
    fn spline_through_key_points_stays_within_tolerance() {
        let circle: Vec<Point> = (0..180)
            .map(|i| {
                let a = i as f64 * std::f64::consts::TAU / 180.0;
                Point::new(50.0 * a.cos(), 50.0 * a.sin())
            })
            .collect();
        let (hem, hem_tags) = box_with_coarse_hem();
        let contours = [
            build_contour(&circle, true).expect("circle"),
            build_contour(&s_curve(), false).expect("s curve"),
            build_contour_tagged(&hem, true, Some(&hem_tags)).expect("hem"),
        ];
        for c in &contours {
            let d = spline_deviation(c);
            assert!(d <= CURVE_TOLERANCE_MM + 1e-9, "spline deviation {d}");
            assert!(reduced_is_ordered_subset(c), "reduced is an ordered subset of dense");
            assert_eq!(c.reduced.len(), c.turn.len());
        }
    }

    #[test]
    fn unneeded_curve_points_are_dropped() {
        let circle: Vec<Point> = (0..180)
            .map(|i| {
                let a = i as f64 * std::f64::consts::TAU / 180.0;
                Point::new(50.0 * a.cos(), 50.0 * a.sin())
            })
            .collect();
        let c = build_contour(&circle, true).expect("circle");
        assert!(c.reduced.len() <= 30, "circle keeps {} key points", c.reduced.len());
        for c in [c, build_contour(&s_curve(), false).expect("s curve")] {
            assert_eq!(removable_curve_points(&c), Vec::<usize>::new(), "every curve point is needed");
        }
    }

    #[test]
    fn spline_is_straight_between_two_turn_points() {
        let c = build_contour(&square_with_midpoints(), true).expect("contour");
        assert_eq!(c.reduced.len(), 4, "a straight side needs no curve point");
        let spline = spline_points(&c);
        assert_eq!(spline.first(), spline.last(), "closed spline ends at its start");
        assert!(spline.iter().all(|p| p.x.abs() < 1e-9 || p.y.abs() < 1e-9 || (p.x - 100.0).abs() < 1e-9 || (p.y - 100.0).abs() < 1e-9));
    }

    #[test]
    fn straight_to_curve_junctions_are_turn_points() {
        // D shape: a 100 mm straight side closed by a tangent-free semicircle.
        let mut d = vec![Point::new(0.0, 0.0)];
        for i in 0..=90 {
            let a = -std::f64::consts::FRAC_PI_2 + i as f64 * std::f64::consts::PI / 90.0;
            d.push(Point::new(50.0 * a.cos(), 50.0 + 50.0 * a.sin()));
        }
        let c = build_contour(&d, true).expect("contour");
        let turns: Vec<Point> = c.reduced.iter().zip(&c.turn).filter(|(_, &t)| t).map(|(p, _)| *p).collect();
        assert!(turns.iter().any(|p| p.x.abs() < 1e-6 && p.y.abs() < 1e-6), "bottom junction is a turn point");
        assert!(turns.iter().any(|p| p.x.abs() < 1e-6 && (p.y - 100.0).abs() < 1e-6), "top junction is a turn point");
        assert!(c.turn.iter().any(|&t| !t), "the arc keeps curve points");
    }

    #[test]
    fn open_polyline_ends_are_turn_points() {
        let c = build_contour(&[Point::new(0.0, 0.0), Point::new(5.0, 0.0), Point::new(10.0, 0.0)], false).expect("contour");
        assert_eq!(c.reduced, vec![Point::new(0.0, 0.0), Point::new(10.0, 0.0)]);
        assert_eq!(c.turn, vec![true, true]);
    }

    #[test]
    fn degenerate_contours_are_rejected() {
        assert!(build_contour(&[Point::new(1.0, 1.0), Point::new(1.0, 1.0)], false).is_none());
        assert!(build_contour(&[Point::new(0.0, 0.0), Point::new(1.0, 0.0), Point::new(0.0, 0.0)], true).is_none());
    }

    // @brief Notch segments drawn on the bottom edge (y = 0) of the square, pointing up.
    #[test]
    fn notch_shapes_are_classified_with_depth_width_and_angle() {
        let boundary: Vec<Point> = build_contour(&square_with_midpoints(), true).expect("contour").dense;
        let p = Point::new;
        let segments = vec![
            // Slit at x=10, depth 5.
            (p(10.0, 0.0), p(10.0, 5.0)),
            // V at x=30: width 4, depth 5.
            (p(28.0, 0.0), p(30.0, 5.0)),
            (p(30.0, 5.0), p(32.0, 0.0)),
            // T at x=50: stem depth 5, bar width 6.
            (p(50.0, 0.0), p(50.0, 5.0)),
            (p(47.0, 5.0), p(53.0, 5.0)),
            // Castle at x=70: width 4, depth 5.
            (p(68.0, 0.0), p(68.0, 5.0)),
            (p(68.0, 5.0), p(72.0, 5.0)),
            (p(72.0, 5.0), p(72.0, 0.0)),
            // U at x=90: four segments, width 4, depth 5.
            (p(88.0, 0.0), p(88.0, 4.0)),
            (p(88.0, 4.0), p(90.0, 5.0)),
            (p(90.0, 5.0), p(92.0, 4.0)),
            (p(92.0, 4.0), p(92.0, 0.0)),
        ];
        let notches = build_notches(&segments, &boundary);
        let kinds: Vec<NotchKind> = notches.iter().map(|n| n.kind).collect();
        assert_eq!(kinds, vec![NotchKind::Slit, NotchKind::V, NotchKind::T, NotchKind::Castle, NotchKind::U]);
        let expected_width = [0.0, 4.0, 6.0, 4.0, 4.0];
        let expected_base_x = [10.0, 30.0, 50.0, 70.0, 90.0];
        for (i, n) in notches.iter().enumerate() {
            assert!((n.depth - 5.0).abs() < 1e-6, "notch {i} depth {}", n.depth);
            assert!((n.width - expected_width[i]).abs() < 1e-6, "notch {i} width {}", n.width);
            assert!((n.base.x - expected_base_x[i]).abs() < 1e-6 && n.base.y.abs() < 1e-6, "notch {i} base {:?}", n.base);
            assert!((n.angle_deg - 90.0).abs() < 1e-6, "notch {i} angle {}", n.angle_deg);
        }
        assert_eq!(NotchKind::T.layer(), "80");
        assert_eq!(NotchKind::Castle.layer(), "81");
        assert_eq!(NotchKind::U.layer(), "83");
    }

    #[test]
    fn length_attributes_convert_to_user_units() {
        let s = |v: &str| Some(v.to_string());
        assert_eq!(parse_length_attr(s("6773").as_ref(), 0.0), 6773.0);
        assert_eq!(parse_length_attr(s("10px").as_ref(), 0.0), 10.0);
        assert!((parse_length_attr(s("25.4mm").as_ref(), 0.0) - 96.0).abs() < 1e-9);
        assert_eq!(parse_length_attr(s("1in").as_ref(), 0.0), 96.0);
        assert_eq!(parse_length_attr(s("junk").as_ref(), 7.0), 7.0);
        assert_eq!(parse_length_attr(None, 7.0), 7.0);
    }

    // Tagged two-piece fixture: 96 px = 25.4 mm. Piece A has cut and seam lines,
    // notches, a label and a grainline; piece B has only a seam line.
    const TAGGED: &str = r#"
        <svg viewBox="0 0 960 960" width="254mm" height="254mm" xmlns="http://www.w3.org/2000/svg">
          <g id="pattern-1" data-type="pattern" data-name="Test Shirt">
            <g id="piece_Front" data-type="piece" data-name="Front">
              <g data-type="cutline"><path d="M 0,0 L 960,0 L 960,960 L 0,960 Z"/></g>
              <g data-type="seamline"><path d="M 96,96 L 864,96 L 864,864 L 96,864 Z"/></g>
              <g data-type="notch"><path d="M 480,960 L 480,941 M 200,0 L 200,19"/></g>
              <g data-type="internal_path"><path d="M 200,480 L 700,480"/></g>
              <g data-type="grainline"><path d="M 480,800 L 470,780 L 490,780 L 480,800 L 480,200"/></g>
              <g data-type="piece_label">
                <g transform="matrix(1,0,0,1,300,400)"><text font-size="24" x="0" y="0">Front</text></g>
                <g transform="matrix(1,0,0,1,300,440)"><text font-size="24" x="0" y="0">&lt;empty&gt;</text></g>
                <g transform="matrix(1,0,0,1,300,480)"><text font-size="24" x="0" y="0">Cut 2</text></g>
              </g>
            </g>
            <g id="piece_Facing" data-type="piece" data-name="Facing">
              <g data-type="seamline"><path d="M 0,0 L 96,0 L 96,96 L 0,96 Z"/></g>
            </g>
          </g>
        </svg>"#;

    #[test]
    fn tagged_svg_builds_astm_pieces_in_millimetres() {
        let doc = Document::parse(TAGGED).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions::default()).expect("converts");
        assert_eq!(drawing.style_name.as_deref(), Some("Test Shirt"));
        assert_eq!(drawing.blocks.len(), 2);

        let front = &drawing.blocks[0];
        assert_eq!(front.name, "piece_Front_M");
        assert_eq!(front.piece_name, "Front");
        assert_eq!(front.quantity.as_deref(), Some("1,1"), "Cut 2 → one right, one left");

        // Boundary: 960 px square = 254 mm, Y inverted inside the 960 px viewBox.
        let boundary = front.boundary.as_ref().expect("boundary");
        assert_eq!(boundary.reduced.len(), 4);
        let max_x = boundary.dense.iter().map(|p| p.x).fold(f64::MIN, f64::max);
        assert!((max_x - 960.0 * MM_PER_PX).abs() < 1e-3, "max x {max_x}");

        assert_eq!(front.sew_lines.len(), 1, "seam line kept as sew line");
        assert_eq!(front.internal_lines.len(), 1);
        assert_eq!(front.notches.len(), 2, "one path, two subpaths, two notches");
        assert!(front.notches.iter().all(|n| (n.depth - 19.0 * MM_PER_PX).abs() < 1e-3));

        // Grainline: tip to tip, 600 px long.
        let (g1, g2) = front.grainline.expect("grainline");
        assert!(((g1.y - g2.y).abs() - 600.0 * MM_PER_PX).abs() < 1e-3);

        // Label: "<empty>" dropped; transform positions the text; font size scales to mm.
        let texts: Vec<&str> = front.annotations.iter().map(|a| a.text.as_str()).collect();
        assert_eq!(texts, vec!["Front", "Cut 2"]);
        let first = &front.annotations[0];
        assert!((first.position.x - 300.0 * MM_PER_PX).abs() < 1e-3);
        assert!((first.position.y - (960.0 - 400.0) * MM_PER_PX).abs() < 1e-3);
        assert!((first.height - 24.0 * MM_PER_PX).abs() < 1e-3);

        // A piece without a cut line uses its seam line as the boundary, not also as a sew line.
        let facing = &drawing.blocks[1];
        assert!(facing.boundary.is_some());
        assert!(facing.sew_lines.is_empty());
    }

    #[test]
    fn missing_piece_data_names_incomplete_pieces() {
        let doc = Document::parse(TAGGED).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions::default()).expect("converts");
        let missing = drawing.missing_piece_data();
        // Front has label, "Cut 2" and grainline; Facing has only a seam line.
        assert_eq!(missing.len(), 1);
        assert_eq!(missing[0].piece_name, "Facing");
        assert!(missing[0].quantity && missing[0].label && missing[0].grainline);
    }

    #[test]
    fn label_without_cut_line_misses_quantity_only() {
        let svg = TAGGED.replace("Cut 2", "Self");
        let doc = Document::parse(&svg).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions::default()).expect("converts");
        let front = drawing.missing_piece_data().into_iter().find(|m| m.piece_name == "Front").expect("front listed");
        assert!(front.quantity);
        assert!(!front.label && !front.grainline);
    }

    // @brief Box with a coarse curved hem: three corners, then 10 segments of
    //        20 mm that bend 1.5° each, like Seamly2D's interpolation of a flat curve.
    // @return (points, tags): tags mark the four corners only.
    fn box_with_coarse_hem() -> (Vec<Point>, Vec<bool>) {
        let mut points = vec![Point::new(0.0, 0.0), Point::new(200.0, 0.0), Point::new(200.0, 100.0)];
        for k in 1..10 {
            let k = k as f64;
            points.push(Point::new(200.0 - 20.0 * k, 100.0 + 0.26 * k * (10.0 - k)));
        } // for each hem vertex
        points.push(Point::new(0.0, 100.0));
        let mut tags = vec![false; points.len()];
        for i in [0, 1, 2, points.len() - 1] {
            tags[i] = true;
        } // for each corner
        (points, tags)
    }

    #[test]
    fn coarse_curve_segments_are_not_turn_points_without_tags() {
        let (points, _) = box_with_coarse_hem();
        let c = build_contour(&points, true).expect("contour");
        assert_eq!(c.turn.iter().filter(|&&t| t).count(), 4, "only the four corners: {:?}", c.turn);
    }

    #[test]
    fn tags_decide_turn_points() {
        let (points, tags) = box_with_coarse_hem();
        let c = build_contour_tagged(&points, true, Some(&tags)).expect("contour");
        assert_eq!(c.turn.iter().filter(|&&t| t).count(), 4, "tagged corners only: {:?}", c.turn);

        // A tag on a hem vertex makes it a turn point, although the geometry is smooth.
        let mut tags = tags;
        tags[6] = true;
        let c = build_contour_tagged(&points, true, Some(&tags)).expect("contour");
        let hem_turn = c.reduced.iter().zip(&c.turn).any(|(p, &t)| t && *p == points[6]);
        assert!(hem_turn, "tagged hem vertex is a turn point");
    }

    #[test]
    fn mismatched_tags_fall_back_to_geometry() {
        let (points, _) = box_with_coarse_hem();
        let c = build_contour_tagged(&points, true, Some(&[true, false])).expect("contour");
        assert_eq!(c.turn.iter().filter(|&&t| t).count(), 4);
    }

    #[test]
    fn duplicate_vertex_keeps_its_tag() {
        let p = Point::new;
        let points = [p(0.0, 0.0), p(50.0, 0.0), p(100.0, 0.0), p(100.0, 0.0), p(100.0, 100.0), p(0.0, 100.0)];
        let tags = [true, false, false, true, true, true];
        let c = build_contour_tagged(&points, true, Some(&tags)).expect("contour");
        assert_eq!(c.dense.len(), 5);
        assert_eq!(c.reduced.len(), 4);
        assert!(c.turn.iter().all(|&t| t), "repeat's tag moves to the kept vertex");
    }

    // Square cut line with a midpoint on the top side. `data-turn-points` marks
    // the midpoint as a turn point, which the geometry alone never would.
    const TURN_TAGGED: &str = r#"
        <svg viewBox="0 0 960 960" width="254mm" height="254mm" xmlns="http://www.w3.org/2000/svg">
          <g id="piece_Cuff" data-type="piece" data-name="Cuff">
            <g data-type="cutline" data-turn-points="0 1 2 3 4">
              <g><path d="M 0,0 L 480,0 L 960,0 L 960,960 L 0,960 L 0,0 Z"/></g>
            </g>
            <g data-type="seamline" data-turn-points="0 2 3 4">
              <path d="M 96,96 L 480,96 L 864,96 L 864,864 L 96,864 L 96,96 Z"/>
            </g>
          </g>
        </svg>"#;

    #[test]
    fn svg_turn_point_tags_reach_the_contours() {
        let doc = Document::parse(TURN_TAGGED).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions::default()).expect("converts");
        let cuff = &drawing.blocks[0];

        let boundary = cuff.boundary.as_ref().expect("boundary");
        assert_eq!(boundary.reduced.len(), 5, "tagged midpoint is kept as a key point");
        assert!(boundary.turn.iter().all(|&t| t));

        let sew = &cuff.sew_lines[0];
        assert_eq!(sew.reduced.len(), 4, "untagged collinear midpoint is dropped");
        assert!(sew.turn.iter().all(|&t| t));
    }
}
