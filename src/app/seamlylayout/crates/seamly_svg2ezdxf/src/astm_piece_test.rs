// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Tests for the ASTM D6673 piece model: contours, notches, and tagged-SVG extraction.

#[cfg(test)]
mod tests {
    use crate::astm_contour::{build_contour, distance_to_segment, CURVE_TOLERANCE_MM};
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

    // @brief Largest distance from any dense vertex to the reduced polyline.
    fn max_deviation(dense: &[Point], reduced: &[Point]) -> f64 {
        let n = reduced.len();
        dense
            .iter()
            .map(|&p| (0..n).map(|i| distance_to_segment(p, reduced[i], reduced[(i + 1) % n])).fold(f64::INFINITY, f64::min))
            .fold(0.0, f64::max)
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
        assert!(max_deviation(&c.dense, &c.reduced) <= CURVE_TOLERANCE_MM + 1e-9);
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
}
