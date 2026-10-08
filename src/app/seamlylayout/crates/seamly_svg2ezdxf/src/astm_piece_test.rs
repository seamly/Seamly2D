// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Tests for the ASTM D6673 piece model: contours, notches, and tagged-SVG extraction.

#[cfg(test)]
mod tests {
    use crate::astm_contour::{
        build_contour, build_contour_tagged, spline_deviation, spline_points, AstmContour, CURVE_TOLERANCE_MM,
    };
    use crate::astm_notch::{build_notches, seam_allowance_at, NotchKind};
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
    fn open_polyline_ends_are_curve_points() {
        let line = [Point::new(0.0, 0.0), Point::new(5.0, 0.0), Point::new(10.0, 0.0)];
        let c = build_contour(&line, false).expect("contour");
        assert_eq!(c.reduced, vec![Point::new(0.0, 0.0), Point::new(10.0, 0.0)]);
        assert_eq!(c.turn, vec![false, false]);

        // Seamly2D tags on the ends do not make them turn points.
        let c = build_contour_tagged(&line, false, Some(&[true, false, true])).expect("contour");
        assert_eq!(c.turn, vec![false, false]);
    }

    #[test]
    fn degenerate_contours_are_rejected() {
        assert!(build_contour(&[Point::new(1.0, 1.0), Point::new(1.0, 1.0)], false).is_none());
        assert!(build_contour(&[Point::new(0.0, 0.0), Point::new(1.0, 0.0), Point::new(0.0, 0.0)], true).is_none());
    }

    // @brief Notch segments drawn on the bottom edge (y = 0) of the square, pointing up, depth 5.
    // Order: slit, V, T, castle, U.
    fn notch_shape_segments() -> Vec<(Point, Point)> {
        let p = Point::new;
        vec![
            // Slit at x=10.
            (p(10.0, 0.0), p(10.0, 5.0)),
            // V at x=30, width 4.
            (p(28.0, 0.0), p(30.0, 5.0)),
            (p(30.0, 5.0), p(32.0, 0.0)),
            // T at x=50: stem, bar width 6.
            (p(50.0, 0.0), p(50.0, 5.0)),
            (p(47.0, 5.0), p(53.0, 5.0)),
            // Castle at x=70, width 4.
            (p(68.0, 0.0), p(68.0, 5.0)),
            (p(68.0, 5.0), p(72.0, 5.0)),
            (p(72.0, 5.0), p(72.0, 0.0)),
            // U at x=90: four segments, width 4.
            (p(88.0, 0.0), p(88.0, 4.0)),
            (p(88.0, 4.0), p(90.0, 5.0)),
            (p(90.0, 5.0), p(92.0, 4.0)),
            (p(92.0, 4.0), p(92.0, 0.0)),
        ]
    }

    // @brief Closed sew line inside the 100 mm square: bottom edge `bottom` mm in, other edges 10 mm in.
    fn sew_line(bottom: f64) -> AstmContour {
        let p = Point::new;
        build_contour(&[p(10.0, bottom), p(90.0, bottom), p(90.0, 90.0), p(10.0, 90.0), p(10.0, bottom)], true).expect("sew line")
    }

    #[test]
    fn every_notch_shape_is_a_layer_4_slit_half_the_seam_allowance_deep() {
        let boundary: Vec<Point> = build_contour(&square_with_midpoints(), true).expect("contour").dense;
        let notches = build_notches(&notch_shape_segments(), &[], &boundary, &[sew_line(10.0)]);
        assert_eq!(notches.len(), 5);
        let expected_base_x = [10.0, 30.0, 50.0, 70.0, 90.0];
        for (i, n) in notches.iter().enumerate() {
            assert_eq!(n.kind, NotchKind::Slit, "notch {i}");
            assert_eq!(n.kind.layer(), "4", "notch {i}");
            assert!((n.depth - 5.0).abs() < 1e-6, "notch {i} depth {}", n.depth);
            assert_eq!(n.width, 0.0, "notch {i}");
            assert!((n.base.x - expected_base_x[i]).abs() < 1e-6 && n.base.y.abs() < 1e-6, "notch {i} base {:?}", n.base);
            assert!((n.angle_deg - 90.0).abs() < 1e-6, "notch {i} angle {}", n.angle_deg);
        }
    }

    #[test]
    fn slit_depth_follows_the_seam_allowance_of_its_edge() {
        let boundary: Vec<Point> = build_contour(&square_with_midpoints(), true).expect("contour").dense;
        let p = Point::new;
        // Bottom edge 6 mm, top edge 10 mm: one slit on each.
        let segments = [(p(30.0, 0.0), p(30.0, 5.0)), (p(50.0, 100.0), p(50.0, 95.0))];
        let notches = build_notches(&segments, &[], &boundary, &[sew_line(6.0)]);
        let depths: Vec<f64> = notches.iter().map(|n| n.depth).collect();
        assert!((depths[0] - 3.0).abs() < 1e-6 && (depths[1] - 5.0).abs() < 1e-6, "depths {depths:?}");
    }

    #[test]
    fn slit_keeps_drawn_depth_without_sew_line() {
        let boundary: Vec<Point> = build_contour(&square_with_midpoints(), true).expect("contour").dense;
        let notches = build_notches(&notch_shape_segments(), &[], &boundary, &[]);
        assert!(notches.iter().all(|n| n.kind == NotchKind::Slit && (n.depth - 5.0).abs() < 1e-6));
    }

    #[test]
    fn seamly2d_width_sets_slit_depth_before_the_measured_width() {
        let boundary: Vec<Point> = build_contour(&square_with_midpoints(), true).expect("contour").dense;
        let segments = notch_shape_segments();
        let given = vec![Some(8.0); segments.len()];
        // Given width wins over the 10 mm sew line.
        let notches = build_notches(&segments, &given, &boundary, &[sew_line(10.0)]);
        assert!(notches.iter().all(|n| (n.depth - 4.0).abs() < 1e-6), "{notches:?}");
        // Built-in seam allowance: no sew line, the given width alone sets the depth.
        let notches = build_notches(&segments, &given, &boundary, &[]);
        assert!(notches.iter().all(|n| n.kind == NotchKind::Slit && (n.depth - 4.0).abs() < 1e-6));
        // No seam allowance (0) or a list of the wrong length: measured width, else drawn depth.
        let notches = build_notches(&segments, &vec![Some(0.0); segments.len()], &boundary, &[sew_line(10.0)]);
        assert!(notches.iter().all(|n| (n.depth - 5.0).abs() < 1e-6));
        let notches = build_notches(&segments, &[Some(8.0)], &boundary, &[]);
        assert!(notches.iter().all(|n| (n.depth - 5.0).abs() < 1e-6), "drawn depth is 5");
    }

    // Built-in seam allowance: the main path is the cut line, sent as `seamline`; no
    // cut line, no sew line. 96 px = 25.4 mm. Notch widths 10 and 6 mm.
    const BUILT_IN: &str = r#"
        <svg viewBox="0 0 960 960" width="254mm" height="254mm" xmlns="http://www.w3.org/2000/svg">
          <g id="pattern-1" data-type="pattern" data-name="Built In">
            <g id="piece_Yoke" data-type="piece" data-name="Yoke">
              <g data-type="seamline"><path d="M 0,0 L 960,0 L 960,960 L 0,960 Z"/></g>
              <g data-type="notch" data-seam-allowances="10.00 6.00"><path d="M 480,960 L 480,941 M 200,0 L 200,19"/></g>
            </g>
          </g>
        </svg>"#;

    #[test]
    fn built_in_seam_allowance_notches_use_seamly2d_widths() {
        let doc = Document::parse(BUILT_IN).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions::default()).expect("converts");
        let yoke = &drawing.blocks[0];
        assert!(yoke.boundary.is_some() && yoke.sew_lines.is_empty());
        let depths: Vec<f64> = yoke.notches.iter().map(|n| n.depth).collect();
        assert_eq!(depths.len(), 2);
        assert!((depths[0] - 5.0).abs() < 1e-6 && (depths[1] - 3.0).abs() < 1e-6, "depths {depths:?}");
        assert!(yoke.notches.iter().all(|n| n.kind == NotchKind::Slit));
    }

    #[test]
    fn seam_allowance_at_is_cut_line_to_nearest_sew_line() {
        let p = Point::new;
        assert!((seam_allowance_at(p(50.0, 0.0), &[sew_line(6.0)]).expect("width") - 6.0).abs() < 1e-6);
        // Closing edge of a closed sew line counts: left edge, x = 10.
        assert!((seam_allowance_at(p(0.0, 50.0), &[sew_line(10.0)]).expect("width") - 10.0).abs() < 1e-6);
        // Seam allowance built in, or no sew line: none.
        let built_in = build_contour(&square_with_midpoints(), true).expect("contour");
        assert_eq!(seam_allowance_at(p(50.0, 0.0), &[built_in]), None);
        assert_eq!(seam_allowance_at(p(50.0, 0.0), &[]), None);
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
        // Seam allowance 96 px; slit depth is half of it, not the drawn 19 px.
        assert!(front.notches.iter().all(|n| n.kind == NotchKind::Slit && (n.depth - 48.0 * MM_PER_PX).abs() < 1e-3));

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
    fn piece_size_comes_from_data_size() {
        let svg = TAGGED.replace(r#"data-name="Front">"#, r#"data-name="Front" data-size=" 36 ">"#);
        let doc = Document::parse(&svg).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions::default()).expect("converts");
        assert_eq!(drawing.blocks[0].size.as_deref(), Some("36"));
        assert_eq!(drawing.blocks[1].size, None, "no data-size, no size");
        assert!(drawing.blocks.iter().all(|b| b.material == "Fabric"));
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

    // @brief A long straight line that runs smoothly into a tight curve.
    // @details One 100 mm dense edge, then a quarter circle of radius 10 mm in
    //          1 mm steps. Only the two open ends are turn points.
    fn line_into_tight_curve() -> (Vec<Point>, Vec<bool>) {
        let mut points = vec![Point::new(0.0, 0.0)];
        for i in 0..=15 {
            let a = std::f64::consts::FRAC_PI_2 * i as f64 / 15.0;
            points.push(Point::new(100.0 + 10.0 * a.sin(), 10.0 - 10.0 * a.cos()));
        }
        let mut turns = vec![false; points.len()];
        turns[0] = true;
        *turns.last_mut().unwrap() = true;
        (points, turns)
    }

    #[test]
    fn long_edge_next_to_tight_curve_is_split_for_chord_length_reader() {
        let (points, turns) = line_into_tight_curve();
        let c = build_contour_tagged(&points, false, Some(&turns)).expect("contour");
        // The chord-length spline bulges off the 100 mm edge, so the edge gains a vertex.
        assert!(c.dense.len() > points.len(), "straight edge split");
        for p in c.dense.iter().filter(|p| p.x > 0.0 && p.x < 100.0) {
            assert!(p.y.abs() < 1e-9, "split vertex lies on the straight edge");
        }
        assert!(spline_deviation(&c) <= CURVE_TOLERANCE_MM + 1e-9);
        // `reduced` stays an ordered subset of `dense`.
        let mut from = 0;
        for k in &c.reduced {
            from += c.dense[from..].iter().position(|d| d == k).expect("key point in dense");
        }
    }

    // @brief Closed piece tagged at its four corners only.
    // @details Bottom edge: curve, straight run at y = -2 with pleat vertices, curve. Both
    //          curves meet the run tangentially. Top edge, right to left: curve, then a
    //          straight line from x = 100 to the corner, with one vertex between. The curve
    //          leaves the line at 1.5°, more than twice its next vertex angle of 0.7°.
    // @return Vertices and their corner tags.
    fn lines_meeting_curves() -> (Vec<Point>, Vec<bool>) {
        let mut points = vec![Point::new(0.0, 0.0)];
        for x in [10.0, 20.0, 30.0] {
            let t: f64 = x / 40.0;
            points.push(Point::new(x, -2.0 * t * (2.0 - t)));
        } // for each left hem curve vertex
        for x in [40.0, 70.0, 100.0, 130.0, 160.0] {
            points.push(Point::new(x, -2.0));
        } // for each straight hem vertex
        for x in [170.0, 180.0, 190.0] {
            let t: f64 = (x - 160.0) / 40.0;
            points.push(Point::new(x, -2.0 * (1.0 - t * t)));
        } // for each right hem curve vertex
        points.push(Point::new(200.0, 0.0));
        for x in [200.0, 190.0, 180.0, 170.0, 160.0, 150.0, 140.0, 130.0, 120.0, 110.0] {
            let u: f64 = x - 100.0;
            points.push(Point::new(x, 100.0 + 0.0006 * u * u + 0.02 * u));
        } // for each top curve vertex, the first one the top-right corner
        points.extend([Point::new(100.0, 100.0), Point::new(50.0, 100.0), Point::new(0.0, 100.0)]);
        let corners = [Point::new(0.0, 0.0), Point::new(200.0, 0.0), Point::new(200.0, 108.0), Point::new(0.0, 100.0)];
        let tags = points.iter().map(|p| corners.iter().any(|c| (c.x - p.x).abs() < 1e-9 && (c.y - p.y).abs() < 1e-9)).collect();
        (points, tags)
    }

    #[test]
    fn line_ends_are_turn_points_where_the_tangent_breaks() {
        let (points, tags) = lines_meeting_curves();
        let c = build_contour_tagged(&points, true, Some(&tags)).expect("contour");
        let turns: Vec<Point> = c.reduced.iter().zip(&c.turn).filter(|(_, &t)| t).map(|(p, _)| *p).collect();
        let has = |x: f64, y: f64| turns.iter().any(|p| (p.x - x).abs() < 1e-9 && (p.y - y).abs() < 1e-9);
        assert!(!has(40.0, -2.0) && !has(160.0, -2.0), "tangent hem run ends are curve points: {turns:?}");
        assert!(has(100.0, 100.0), "where the top line meets the curve: {turns:?}");
        assert_eq!(turns.len(), 5, "four corners and one line end: {turns:?}");
        assert!(c.turn.iter().any(|&t| !t), "the curves keep curve points");
        assert!(spline_deviation(&c) <= CURVE_TOLERANCE_MM + 1e-9);
    }

    #[test]
    fn vertex_inserted_on_a_curve_segment_is_not_a_straight_run() {
        // Circle, radius 500 mm, a vertex every 6 degrees (52 mm chords), with a
        // notch vertex inserted at the midpoint of one chord.
        let mut circle: Vec<Point> = (0..60)
            .map(|i| {
                let a = (i as f64 * 6.0).to_radians();
                Point::new(500.0 * a.cos(), 500.0 * a.sin())
            })
            .collect();
        let mid = Point::new((circle[0].x + circle[1].x) / 2.0, (circle[0].y + circle[1].y) / 2.0);
        circle.insert(1, mid);
        let c = build_contour_tagged(&circle, true, Some(&vec![false; circle.len()])).expect("contour");
        assert!(c.turn.iter().all(|&t| !t), "a smooth circle has only curve points: {:?}", c.turn);
    }

    // @brief Open polyline: one straight segment of `line_mm` along x, then ten 15 mm
    //        curve segments. The curve leaves the line at `break_deg` and turns `step_deg`
    //        at each later vertex.
    fn single_segment_into_curve(line_mm: f64, break_deg: f64, step_deg: f64) -> Vec<Point> {
        let mut points = vec![Point::new(0.0, 0.0), Point::new(line_mm, 0.0)];
        let mut heading = break_deg;
        for _ in 0..10 {
            let last = *points.last().unwrap();
            let h = heading.to_radians();
            points.push(Point::new(last.x + 15.0 * h.cos(), last.y + 15.0 * h.sin()));
            heading += step_deg;
        } // for each curve segment
        points
    }

    #[test]
    fn single_segment_line_end_is_a_turn_point_only_where_the_tangent_breaks() {
        let junction_turn = |line_mm: f64, break_deg: f64, step_deg: f64| {
            let points = single_segment_into_curve(line_mm, break_deg, step_deg);
            let c = build_contour_tagged(&points, false, Some(&vec![false; points.len()])).expect("contour");
            c.reduced.iter().zip(&c.turn).any(|(p, &t)| t && *p == points[1])
        };
        assert!(junction_turn(65.0, 2.9, 1.0), "curve leaves the line at a break");
        assert!(!junction_turn(65.0, 1.2, 2.0), "curve leaves the line tangentially");
        assert!(!junction_turn(65.0, 0.8, 0.2), "flat curve: angles below the break floor");
        assert!(!junction_turn(12.0, 7.4, 2.5), "a short single segment is a curve chord");
    }
}
