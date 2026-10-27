// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief ASTM D6673-10 structure tests for the DXF-ASTM writer.

#[cfg(test)]
mod tests {
    use crate::writer::{export_dxf_astm, DxfAstmExportOptions};
    use seamly_svg2ezdxf::{
        build_contour, spline_deviation, svg_to_ezdxf, Annotation, AstmContour, Block, Drawing, DxfVersion, Notch, NotchKind, Point, SvgToEzdxfOptions, CURVE_TOLERANCE_MM,
    };
    use std::collections::HashMap;

    // @brief One DXF entity: its type and its (group code, value) pairs.
    struct DxfEntity {
        kind: String,
        groups: Vec<(i32, String)>,
    }

    impl DxfEntity {
        // @brief First value of a group code.
        fn get(&self, code: i32) -> Option<&str> {
            self.groups.iter().find(|(c, _)| *c == code).map(|(_, v)| v.as_str())
        }
        // @brief Layer (group 8).
        fn layer(&self) -> &str {
            self.get(8).unwrap_or("")
        }
    }

    // @brief Parsed DXF: entities of the BLOCKS section per block, and of the ENTITIES section.
    struct ParsedDxf {
        blocks: Vec<(String, Vec<DxfEntity>)>,
        entities: Vec<DxfEntity>,
        all_pairs: Vec<(i32, String)>,
    }

    // @brief Split DXF text into group pairs and group them by section and block.
    fn parse_dxf(text: &str) -> ParsedDxf {
        let lines: Vec<&str> = text.lines().collect();
        let pairs: Vec<(i32, String)> = lines
            .chunks(2)
            .map(|c| (c[0].trim().parse::<i32>().expect("group code"), c[1].to_string()))
            .collect();
        let mut raw: Vec<DxfEntity> = Vec::new();
        for (code, value) in &pairs {
            if *code == 0 {
                raw.push(DxfEntity { kind: value.clone(), groups: Vec::new() });
            } else if let Some(e) = raw.last_mut() {
                e.groups.push((*code, value.clone()));
            }
        }
        let (mut blocks, mut entities) = (Vec::new(), Vec::new());
        let mut section = String::new();
        for e in raw {
            match e.kind.as_str() {
                "SECTION" => section = e.get(2).unwrap_or("").to_string(),
                "ENDSEC" | "ENDBLK" | "EOF" => {}
                "BLOCK" => blocks.push((e.get(2).unwrap_or("").to_string(), Vec::new())),
                _ if section == "BLOCKS" => blocks.last_mut().expect("block open").1.push(e),
                _ if section == "ENTITIES" => entities.push(e),
                _ => {}
            }
        }
        ParsedDxf { blocks, entities, all_pairs: pairs }
    }

    // @brief Vertex counts of the POLYLINEs on `layer`, in order.
    fn polyline_sizes(entities: &[DxfEntity], layer: &str) -> Vec<usize> {
        let mut sizes = Vec::new();
        let mut current: Option<usize> = None;
        for e in entities {
            match e.kind.as_str() {
                "POLYLINE" => current = if e.layer() == layer { Some(0) } else { None },
                "VERTEX" => {
                    if let Some(n) = current.as_mut() {
                        *n += 1;
                    }
                }
                "SEQEND" => {
                    if let Some(n) = current.take() {
                        sizes.push(n);
                    }
                }
                _ => {}
            }
        }
        sizes
    }

    // @brief Export a drawing to a temp file and return the DXF text.
    fn export_to_string(drawing: &Drawing, tag: &str, options: &DxfAstmExportOptions) -> String {
        let path = std::env::temp_dir().join(format!("seamly_astm_{}_{}.dxf", tag, std::process::id()));
        export_dxf_astm(drawing, &path, options).expect("export succeeds");
        let text = std::fs::read_to_string(&path).expect("read back");
        let _ = std::fs::remove_file(&path);
        text
    }

    // @brief One piece with every ASTM feature, built directly.
    fn sample_drawing() -> Drawing {
        let square = |s: f64| vec![Point::new(0.0, 0.0), Point::new(s, 0.0), Point::new(s, s), Point::new(0.0, s)];
        let mut block = Block::new("piece_Front_M".to_string());
        block.piece_name = "Front".to_string();
        block.quantity = Some("1,1".to_string());
        block.boundary = build_contour(&square(100.0), true);
        block.sew_lines.push(build_contour(&[Point::new(10.0, 10.0), Point::new(90.0, 10.0), Point::new(90.0, 90.0), Point::new(10.0, 90.0)], true).unwrap());
        block.internal_lines.push(build_contour(&[Point::new(20.0, 50.0), Point::new(80.0, 50.0)], false).unwrap());
        block.cutouts.push(build_contour(&[Point::new(40.0, 40.0), Point::new(60.0, 40.0), Point::new(60.0, 60.0)], true).unwrap());
        block.grainline = Some((Point::new(50.0, 20.0), Point::new(50.0, 80.0)));
        block.notches.push(Notch { base: Point::new(50.0, 0.0), angle_deg: 90.0, depth: 5.0, width: 0.0, kind: NotchKind::Slit });
        block.notches.push(Notch { base: Point::new(30.0, 0.0), angle_deg: 90.0, depth: 5.0, width: 3.0, kind: NotchKind::T });
        block.annotations.push(Annotation { position: Point::new(30.0, 30.0), height: 5.0, rotation: 0.0, text: "Front".to_string() });
        let mut drawing = Drawing::new(DxfVersion::R11);
        drawing.style_name = Some("Test Shirt".to_string());
        drawing.add_block(block);
        drawing
    }

    #[test]
    fn style_system_text_is_written_once_in_entities() {
        let options = DxfAstmExportOptions {
            creation_date: Some("28-09-2026".to_string()),
            creation_time: Some("14-05".to_string()),
            author_release: "1.2.3".to_string(),
            ..DxfAstmExportOptions::default()
        };
        let dxf = parse_dxf(&export_to_string(&sample_drawing(), "style", &options));
        let texts: Vec<&str> = dxf.entities.iter().filter(|e| e.kind == "TEXT").map(|e| e.get(1).unwrap()).collect();
        assert_eq!(
            texts,
            vec![
                "Style Name:Test Shirt",
                "Creation Date:28-09-2026",
                "Creation Time:14-05",
                "Author:Seamly;Seamly2D;1.2.3",
                "Sample Size:",
                "Grade Rule Table:",
                "Units:METRIC",
                "Curve Tolerance:0.25",
                "ASTM/D13Proposal 1 Version:D6673-10",
            ]
        );
        assert!(dxf.entities.iter().filter(|e| e.kind == "TEXT").all(|e| e.layer() == "1"));
    }

    #[test]
    fn piece_system_text_writes_size_name_before_quantity_and_no_category() {
        let mut drawing = sample_drawing();
        drawing.blocks[0].size = Some("102".to_string());
        let dxf = parse_dxf(&export_to_string(&drawing, "size", &DxfAstmExportOptions::default()));
        let (_, body) = &dxf.blocks[0];
        let texts: Vec<&str> = body.iter().filter(|e| e.kind == "TEXT" && e.layer() == "1").map(|e| e.get(1).unwrap()).collect();
        assert_eq!(texts, vec!["Piece Name:Front", "Size Name:102", "Quantity:1,1", "Material:Fabric"]);
    }

    // @brief The D6673 layout rules follow Quantity in D6673 order; a rule without a value writes no line.
    //        DXF-AAMA piece text has none of them.
    #[test]
    fn layout_rule_text_follows_quantity_in_d6673_order() {
        let mut drawing = sample_drawing();
        drawing.blocks[0].rotation = Some("180".to_string());
        drawing.blocks[0].flip = Some("Y".to_string());
        drawing.blocks[0].tilt = Some("-15".to_string());
        drawing.blocks[0].fold = Some("N".to_string());
        let system_texts = |dxf: &ParsedDxf| -> Vec<String> {
            dxf.blocks[0].1.iter().filter(|e| e.kind == "TEXT" && e.layer() == "1").map(|e| e.get(1).unwrap().to_string()).collect()
        };

        let astm = parse_dxf(&export_to_string(&drawing, "rules", &DxfAstmExportOptions::default()));
        assert_eq!(
            system_texts(&astm),
            vec!["Piece Name:Front", "Quantity:1,1", "Rotation:180", "Flip:Y", "Tilt:-15", "Fold:N", "Material:Fabric"]
        );

        drawing.blocks[0].flip = None;
        drawing.blocks[0].tilt = None;
        let fewer = parse_dxf(&export_to_string(&drawing, "rules_fewer", &DxfAstmExportOptions::default()));
        assert_eq!(system_texts(&fewer), vec!["Piece Name:Front", "Quantity:1,1", "Rotation:180", "Fold:N", "Material:Fabric"]);

        drawing.version = DxfVersion::R11;
        let aama = parse_dxf(&export_to_string(&drawing, "rules_aama", &aama_options()));
        let texts = system_texts(&aama);
        assert!(texts.iter().all(|t| !["Rotation:", "Flip:", "Tilt:", "Fold:"].iter().any(|id| t.starts_with(id))), "{texts:?}");
    }

    #[test]
    fn piece_block_uses_astm_layers_and_entities() {
        let dxf = parse_dxf(&export_to_string(&sample_drawing(), "block", &DxfAstmExportOptions::default()));
        assert_eq!(dxf.blocks.len(), 1);
        let (_, body) = &dxf.blocks[0];

        let texts: Vec<(&str, &str)> = body.iter().filter(|e| e.kind == "TEXT").map(|e| (e.layer(), e.get(1).unwrap())).collect();
        assert_eq!(texts, vec![("1", "Piece Name:Front"), ("1", "Quantity:1,1"), ("1", "Material:Fabric"), ("15", "Front")]);

        // Every primary layer has a matching validation layer with the same polyline count.
        for (primary, validation) in [("1", "84"), ("14", "87"), ("8", "85"), ("11", "86")] {
            let (p, v) = (polyline_sizes(body, primary), polyline_sizes(body, validation));
            assert_eq!(p.len(), 1, "one polyline on layer {primary}");
            assert_eq!(p.len(), v.len(), "layer {validation} mirrors layer {primary}");
            assert!(p.iter().zip(&v).all(|(a, b)| a <= b));
        }

        // Turn points: 4 + 4 + 3 corners. The straight open internal line declares no points.
        let points_on = |l: &str| body.iter().filter(|e| e.kind == "POINT" && e.layer() == l).count();
        assert_eq!(points_on("2"), 11);
        assert_eq!(points_on("3"), 0);

        // Notches: POINT with depth (30) and angle (50); width (39) only when it has one.
        let slit = body.iter().find(|e| e.kind == "POINT" && e.layer() == "4").expect("slit notch");
        assert_eq!((slit.get(30), slit.get(39), slit.get(50)), (Some("5.00"), None, Some("90.00")));
        let t = body.iter().find(|e| e.kind == "POINT" && e.layer() == "80").expect("T notch");
        assert_eq!((t.get(30), t.get(39)), (Some("5.00"), Some("3.00")));

        let grain = body.iter().find(|e| e.kind == "LINE").expect("grainline");
        assert_eq!(grain.layer(), "7");
    }

    // @brief The mirror line is a layer 6 LINE on the first two boundary vertices.
    #[test]
    fn mirror_line_is_a_layer_6_line_on_the_boundary_start() {
        let mut drawing = sample_drawing();
        let boundary = drawing.blocks[0].boundary.clone().expect("boundary");
        drawing.blocks[0].mirror_line = Some((boundary.reduced[0], boundary.reduced[1]));
        let dxf = parse_dxf(&export_to_string(&drawing, "mirror", &DxfAstmExportOptions::default()));
        let (_, body) = &dxf.blocks[0];

        let mirror: Vec<&DxfEntity> = body.iter().filter(|e| e.kind == "LINE" && e.layer() == "6").collect();
        assert_eq!(mirror.len(), 1, "one mirror line");
        let ends = [mirror[0].get(10), mirror[0].get(20), mirror[0].get(11), mirror[0].get(21)];

        // The first two vertices of the layer 1 polyline.
        let first = body.iter().position(|e| e.kind == "POLYLINE" && e.layer() == "1").expect("boundary polyline");
        let vertices: Vec<&DxfEntity> = body[first + 1..].iter().take_while(|e| e.kind == "VERTEX").collect();
        let start = [vertices[0].get(10), vertices[0].get(20), vertices[1].get(10), vertices[1].get(20)];
        assert_eq!(ends, start);

        // Without a mirror line, no layer 6.
        let plain = parse_dxf(&export_to_string(&sample_drawing(), "no_mirror", &DxfAstmExportOptions::default()));
        assert!(plain.blocks[0].1.iter().all(|e| e.layer() != "6"));
    }

    #[test]
    fn coordinates_use_two_decimals_and_no_clo3d_group_250() {
        let text = export_to_string(&sample_drawing(), "format", &DxfAstmExportOptions::default());
        let dxf = parse_dxf(&text);
        assert!(dxf.all_pairs.iter().all(|(c, _)| *c != 250), "group 250 is not D6673");
        for (code, value) in &dxf.all_pairs {
            if matches!(code, 10 | 20 | 11 | 21 | 30 | 39 | 40 | 50) {
                let decimals = value.split('.').nth(1).map_or(0, str::len);
                assert_eq!(decimals, 2, "group {code} value '{value}' must have two decimals");
            }
        }
    }

    // @brief The CLO3D variant adds group 250 to the boundary (0) and sew line (2) only.
    #[test]
    fn clo3d_variant_marks_boundary_and_sew_line_with_group_250() {
        let options = DxfAstmExportOptions { clo3d_group_250: true, ..DxfAstmExportOptions::default() };
        let dxf = parse_dxf(&export_to_string(&sample_drawing(), "clo3d", &options));
        let (_, body) = &dxf.blocks[0];
        let marked: Vec<(&str, &str)> = body
            .iter()
            .filter(|e| e.kind == "POLYLINE")
            .filter_map(|e| e.get(250).map(|v| (e.layer(), v)))
            .collect();
        assert_eq!(marked, vec![("1", "0"), ("14", "2")]);
        // Everything else stays D6673: validation layers are still written.
        assert_eq!(polyline_sizes(body, "84").len(), 1);
        assert_eq!(polyline_sizes(body, "87").len(), 1);
    }

    // @brief Handoff of a piece with a built-in seam allowance: the main path is the
    //        `cutline`, the line Seamly2D computes 10 mm inside it is the `seamline`.
    // @details 96 px = 25.4 mm, so 10 mm = 37.795276 px.
    const BUILT_IN_HANDOFF: &str = r#"
        <svg viewBox="0 0 960 960" width="254mm" height="254mm" xmlns="http://www.w3.org/2000/svg">
          <g id="pattern-1" data-type="pattern" data-name="Built In">
            <g id="piece_Yoke" data-type="piece" data-name="Yoke" data-quantity="1">
              <g data-type="seamline" data-turn-points="0 1 2 3">
                <path d="M 37.795276,37.795276 L 922.204724,37.795276 L 922.204724,922.204724 L 37.795276,922.204724 L 37.795276,37.795276 Z"/>
              </g>
              <g data-type="cutline" data-turn-points="0 1 2 3">
                <path d="M 0,0 L 960,0 L 960,960 L 0,960 L 0,0 Z"/>
              </g>
              <g data-type="notch" data-notch-type="slit" data-seam-allowances="10.00" data-notch-segments="1">
                <path d="M 480,960 L 480,941"/>
              </g>
            </g>
          </g>
        </svg>"#;

    // @brief A built-in seam allowance writes the cut line as the boundary (layer 1)
    //        and the seam line as a sew line (layer 14), each with its validation layer.
    #[test]
    fn built_in_seam_allowance_writes_boundary_and_sew_line() {
        let doc = svg_dom::Document::parse(BUILT_IN_HANDOFF).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions { dxf_version: DxfVersion::R11, ..SvgToEzdxfOptions::default() }).expect("converts");
        let dxf = parse_dxf(&export_to_string(&drawing, "built_in", &DxfAstmExportOptions::default()));
        let (_, body) = &dxf.blocks[0];

        for (primary, validation) in [("1", "84"), ("14", "87")] {
            assert_eq!(polyline_sizes(body, primary).len(), 1, "one polyline on layer {primary}");
            assert_eq!(polyline_sizes(body, validation).len(), 1, "one polyline on layer {validation}");
        }

        // Smallest x of the vertices on a layer, in mm.
        let min_x = |layer: &str| {
            body.iter()
                .filter(|e| e.kind == "VERTEX" && e.layer() == layer)
                .map(|e| e.get(10).unwrap().parse::<f64>().unwrap())
                .fold(f64::MAX, f64::min)
        };
        assert!((min_x("14") - min_x("1") - 10.0).abs() < 0.01, "sew line 10 mm inside the boundary");

        // The slit is half the seam allowance deep.
        let slit = body.iter().find(|e| e.kind == "POINT" && e.layer() == "4").expect("slit notch");
        assert_eq!(slit.get(30), Some("5.00"));
    }

    // @brief The male shirt handoff SVG exports with every D6673 requirement met.
    #[test]
    fn male_shirt_fixture_exports_complete_astm_file() {
        let svg = std::fs::read_to_string(concat!(env!("CARGO_MANIFEST_DIR"), "/test_data/male_shirt_pieces.svg"))
            .expect("fixture present");
        let doc = svg_dom::Document::parse(&svg).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions { dxf_version: DxfVersion::R11, ..SvgToEzdxfOptions::default() }).expect("converts");
        let options = DxfAstmExportOptions { style_name: Some("male_shirt".to_string()), ..DxfAstmExportOptions::default() };
        let dxf = parse_dxf(&export_to_string(&drawing, "fixture", &options));

        assert_eq!(dxf.blocks.len(), 17, "one block per piece");
        let mut quantities: HashMap<String, String> = HashMap::new();
        for (name, body) in &dxf.blocks {
            // Value of the system text with this identifier.
            let text = |prefix: &str| {
                body.iter().filter(|e| e.kind == "TEXT").filter_map(|e| e.get(1)).find_map(|t| t.strip_prefix(prefix)).map(str::to_string)
            };
            let piece_name = text("Piece Name:").unwrap_or_else(|| panic!("{name}: Piece Name missing"));
            if let Some(q) = text("Quantity:") {
                quantities.insert(piece_name, q);
            }
            // Layer 1 is required in every block, with layer 84 mirroring it.
            let (l1, l84) = (polyline_sizes(body, "1"), polyline_sizes(body, "84"));
            assert_eq!(l1.len(), 1, "{name}: one boundary polyline");
            assert_eq!(l84.len(), 1, "{name}: one boundary validation polyline");
            assert!(l1[0] < l84[0] || l1[0] == l84[0], "{name}: layer 84 holds every layer 1 vertex");
            assert_eq!(polyline_sizes(body, "14").len(), polyline_sizes(body, "87").len(), "{name}: layer 87 mirrors layer 14");
            // Turn and curve points equal the key points of every contour that declares them.
            let block = drawing.blocks.iter().find(|b| &b.name == name).expect("block in drawing");
            let key_points: usize = block.boundary.iter()
                .chain(&block.sew_lines)
                .chain(&block.internal_lines)
                .chain(&block.cutouts)
                .filter(|c| c.declares_points())
                .map(|c| c.reduced.len())
                .sum();
            let declared = body.iter().filter(|e| e.kind == "POINT" && (e.layer() == "2" || e.layer() == "3")).count();
            assert_eq!(declared, key_points, "{name}: layers 2 and 3 declare every key point once");
        }
        assert_eq!(quantities.get("BackPanel").map(String::as_str), Some("1,0"), "label says Cut 1");

        // The fixture is px at 96 dpi; its back panel cut line is 1391.09 x 2664.57 px, 368 x 705 mm.
        let back = &dxf.blocks.iter().find(|(n, _)| n == "piece_BackPanel_M").expect("back panel").1;
        let xs: Vec<f64> = back.iter().filter(|e| e.kind == "VERTEX" && e.layer() == "84").map(|e| e.get(10).unwrap().parse().unwrap()).collect();
        let ys: Vec<f64> = back.iter().filter(|e| e.kind == "VERTEX" && e.layer() == "84").map(|e| e.get(20).unwrap().parse().unwrap()).collect();
        let span = |v: &[f64]| v.iter().cloned().fold(f64::MIN, f64::max) - v.iter().cloned().fold(f64::MAX, f64::min);
        assert!((span(&xs) - 1391.09 * 25.4 / 96.0).abs() < 1.0, "width {}", span(&xs));
        assert!((span(&ys) - 2664.57 * 25.4 / 96.0).abs() < 1.0, "length {}", span(&ys));

        // The front panel's cut line and sew line each have 8 turn points, from
        // Seamly2D's data-turn-points tags: corners, shoulder and side seam joins,
        // and the front edge line end, where the curve leaves the line at 2.9°.
        // The hem and neckline joins are smooth, so they are curve points even where
        // the interpolation has long single segments.
        let front = &dxf.blocks.iter().find(|(n, _)| n == "piece_FrontPanel_M").expect("front panel").1;
        let turn_points = front.iter().filter(|e| e.kind == "POINT" && e.layer() == "2").count();
        assert_eq!(turn_points, 16, "front panel turn points");
    }

    // @brief Both stand-in reader splines through each contour's key points stay
    //        near its validation polyline (layers 84–87), so a reader's reconstruction passes.
    // @details The limit is CURVE_TOLERANCE_MM. Without added curve points the
    //          sleeves miss it by about 0.15 mm. With the centripetal spline alone,
    //          the chord-length spline missed it on the CollarTop seam line hook
    //          and on the FrontPanel hem line.
    #[test]
    fn male_shirt_splines_follow_validation_layers() {
        let svg = std::fs::read_to_string(concat!(env!("CARGO_MANIFEST_DIR"), "/test_data/male_shirt_pieces.svg"))
            .expect("fixture present");
        let doc = svg_dom::Document::parse(&svg).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions { dxf_version: DxfVersion::R11, ..SvgToEzdxfOptions::default() }).expect("converts");
        let limit = CURVE_TOLERANCE_MM + 1e-9;
        for block in &drawing.blocks {
            let groups: Vec<(&str, Vec<&AstmContour>)> = vec![
                ("84", block.boundary.iter().collect()),
                ("85", block.internal_lines.iter().collect()),
                ("86", block.cutouts.iter().collect()),
                ("87", block.sew_lines.iter().collect()),
            ];
            for (layer, contours) in groups {
                for c in contours {
                    let d = spline_deviation(c);
                    assert!(d <= limit, "{} layer {layer}: spline off by {d:.3} mm", block.name);
                }
            }
        }
    }

    // @brief Yoke and PocketFlapRound carry no curve point the spline does not need.
    // @details The aw.fyi report flagged Yoke_M layer 14 (14 curve points) and
    //          PocketFlapRound_M layer 1. The ceilings hold the dropped points.
    #[test]
    fn male_shirt_curves_carry_only_needed_curve_points() {
        let svg = std::fs::read_to_string(concat!(env!("CARGO_MANIFEST_DIR"), "/test_data/male_shirt_pieces.svg"))
            .expect("fixture present");
        let doc = svg_dom::Document::parse(&svg).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions { dxf_version: DxfVersion::R11, ..SvgToEzdxfOptions::default() }).expect("converts");
        let ceilings = [("piece_Yoke_M", 7, 8), ("piece_PocketFlapRound_M", 4, 5)];
        for (name, boundary_max, sew_max) in ceilings {
            let block = drawing.blocks.iter().find(|b| b.name == name).expect("piece present");
            let boundary = block.boundary.as_ref().expect("boundary");
            let sew = block.sew_lines.first().expect("sew line");
            for (layer, c, max) in [("1", boundary, boundary_max), ("14", sew, sew_max)] {
                let curves = c.turn.iter().filter(|&&t| !t).count();
                assert!(curves <= max, "{name} layer {layer}: {curves} curve points, ceiling {max}");
                assert!(spline_deviation(c) <= CURVE_TOLERANCE_MM + 1e-9, "{name} layer {layer}: spline off");
                // No single curve point can go and keep the spline within tolerance.
                for r in (1..c.reduced.len()).filter(|&r| !c.turn[r]) {
                    let mut trial = c.clone();
                    trial.reduced.remove(r);
                    trial.turn.remove(r);
                    assert!(spline_deviation(&trial) > CURVE_TOLERANCE_MM, "{name} layer {layer}: curve point {r} not needed");
                }
            }
        }
    }

    // @brief The grading fixtures, graded: base size layout drawing in `version`, and the grading warnings.
    fn graded_drawing(version: DxfVersion) -> (Drawing, Vec<String>) {
        let read = |name: &str| {
            let path = format!("{}/test_data/{name}", env!("CARGO_MANIFEST_DIR"));
            svg_dom::Document::parse(&std::fs::read_to_string(path).expect("fixture present")).expect("fixture parses")
        };
        let options = SvgToEzdxfOptions { dxf_version: version, ..SvgToEzdxfOptions::default() };
        let mut layout = svg_to_ezdxf(&read("multisize_grading_layout.svg"), &options).expect("layout converts");
        // The Base Size tab lays out the base size only.
        layout.blocks.retain(|b| b.size.as_deref() == Some("36"));
        let input = svg_to_ezdxf(&read("multisize_grading_input.svg"), &options).expect("input converts");
        let sizes: Vec<String> = ["34", "36", "38"].iter().map(|s| s.to_string()).collect();
        let warnings = seamly_svg2ezdxf::grade_drawing(&mut layout, &input, &sizes, "36");
        layout.grade_rule_table.as_mut().expect("graded").name = "GradingTest".to_string();
        (layout, warnings)
    }

    // @brief Fixed date and time, so the rule file text is stable.
    fn dated_options() -> DxfAstmExportOptions {
        DxfAstmExportOptions {
            creation_date: Some("11-10-2026".to_string()),
            creation_time: Some("14-05".to_string()),
            ..DxfAstmExportOptions::default()
        }
    }

    // @brief A graded export writes the sample size only (D6673 §4.3.1.4): every layer,
    //        the full piece system text, a grade reference line and the `# n` texts.
    #[test]
    fn graded_sample_blocks_follow_d6673() {
        let (drawing, _) = graded_drawing(DxfVersion::R11);
        let parsed = parse_dxf(&export_to_string(&drawing, "graded_sample", &dated_options()));
        let names: Vec<&str> = parsed.blocks.iter().map(|(n, _)| n.as_str()).collect();
        assert_eq!(names, vec!["piece_Front_s36_M", "piece_Back_s36_M"]);

        let texts = |entities: &[DxfEntity], layer: &str| -> Vec<String> {
            entities.iter().filter(|e| e.kind == "TEXT" && e.layer() == layer).map(|e| e.get(1).unwrap_or("").to_string()).collect()
        };
        let count = |entities: &[DxfEntity], kind: &str, layer: &str| entities.iter().filter(|e| e.kind == kind && e.layer() == layer).count();
        let is_rule_text = |t: &String| t.starts_with("# ");

        let (name, front) = &parsed.blocks[0];
        assert_eq!(count(front, "LINE", "5"), 1, "{name}: layer 5 line");
        let system: Vec<String> = texts(front, "1").into_iter().filter(|t| !is_rule_text(t)).collect();
        assert_eq!(system, vec!["Piece Name:Front", "Size Name:36", "Quantity:1,1", "Fold:N", "Material:Fabric"]);
        assert!(count(front, "POINT", "2") > 0 && count(front, "POINT", "4") == 1, "{name}: turn points and notch");
        assert!(texts(front, "1").iter().any(is_rule_text), "{name}: rule text on layer 1");
        assert_eq!(texts(front, "4").len(), 1, "{name}: rule text on the notch");

        // The piece that cannot grade is an ordinary ungraded block.
        let (name, back) = &parsed.blocks[1];
        assert_eq!(count(back, "LINE", "5"), 0, "{name}: no layer 5 line");
        let rule_texts = back.iter().filter(|e| e.kind == "TEXT" && e.get(1).is_some_and(|t| t.starts_with("# "))).count();
        assert_eq!(rule_texts, 0, "{name}: no rule text");
    }

    // @brief Style text names the sample size and the grade rule table.
    #[test]
    fn graded_style_text_names_sample_size_and_table() {
        let (drawing, _) = graded_drawing(DxfVersion::R11);
        let lines = crate::writer::style_system_text(&drawing, &dated_options());
        assert!(lines.contains(&"Sample Size:36".to_string()), "{lines:?}");
        assert!(lines.contains(&"Grade Rule Table:GradingTest".to_string()), "{lines:?}");
    }

    // @brief The `.rul` file (D6673 §4.4) sits beside the DXF: header, then one rule per identifier.
    #[test]
    fn graded_export_writes_the_rule_file() {
        let (drawing, _) = graded_drawing(DxfVersion::R11);
        let path = std::env::temp_dir().join(format!("seamly_astm_rul_{}.dxf", std::process::id()));
        export_dxf_astm(&drawing, &path, &dated_options()).expect("export succeeds");
        let rul_path = path.with_extension("rul");
        let text = std::fs::read_to_string(&rul_path).expect("rule file beside the DXF");
        let _ = std::fs::remove_file(&path);
        let _ = std::fs::remove_file(&rul_path);

        let lines: Vec<&str> = text.lines().collect();
        assert_eq!(
            lines[..9],
            [
                "ASTM/D13 Proposal 1 VERSION:D6673-10",
                "AUTHOR:Seamly2D",
                "CREATION DATE:11-10-2026",
                "CREATION TIME:14:05",
                "UNITS:METRIC",
                "GRADE RULE TABLE:GradingTest",
                "SAMPLE SIZE:36",
                "NUMBER OF SIZES:3",
                "SIZE LIST:34 36 38",
            ]
        );
        let table = drawing.grade_rule_table.as_ref().expect("table");
        let rules: Vec<&str> = lines[9..].to_vec();
        assert_eq!(rules.len(), table.rules.len());
        for (i, rule) in rules.iter().enumerate() {
            let fields: Vec<&str> = rule.split_whitespace().collect();
            let id = (i + 1).to_string();
            assert_eq!(fields[..3], ["RULE:", "DELTA", id.as_str()]);
            assert_eq!(fields.len(), 3 + 3, "one growth per size: {rule}");
            assert_eq!(fields[4], "0.00,0.00", "the sample size never grows: {rule}");
            assert!(!rule.contains("-0.00"), "no signed zero: {rule}");
        }
    }

    // @brief Equal growths share one identifier: the texts use fewer rules than points.
    #[test]
    fn equal_rules_share_an_identifier() {
        let (drawing, _) = graded_drawing(DxfVersion::R11);
        let sample = drawing.blocks.iter().find(|b| b.grading.is_some()).expect("graded block");
        let texts = &sample.grading.as_ref().unwrap().rule_texts;
        let table = drawing.grade_rule_table.as_ref().unwrap();
        assert!(table.rules.len() < texts.len(), "{} rules for {} points", table.rules.len(), texts.len());
        assert!(texts.iter().all(|t| table.rules.iter().any(|r| r.id == t.rule)));
    }

    // @brief An R13 graded export lists layer 5 in the LAYER table.
    #[test]
    fn graded_r13_export_declares_layer_5() {
        let (drawing, _) = graded_drawing(DxfVersion::R13);
        let text = export_to_string(&drawing, "graded_r13", &dated_options());
        let lines: Vec<&str> = text.lines().map(str::trim).collect();
        let declared = lines.windows(4).any(|w| w[0] == "100" && w[1] == "AcDbLayerTableRecord" && w[2] == "2" && w[3] == "5");
        assert!(declared, "layer 5 in the LAYER table");
    }

    // @brief DXF-AAMA options with a fixed date and time.
    fn aama_options() -> DxfAstmExportOptions {
        DxfAstmExportOptions { aama: true, ..dated_options() }
    }

    // @brief `sample_drawing` as a DXF R11 drawing.
    fn sample_r11_drawing() -> Drawing {
        let mut drawing = sample_drawing();
        drawing.version = DxfVersion::R11;
        drawing
    }

    // @brief A DXF-AAMA file is DXF R11, the version ANSI/AAMA-292 builds on: `$ACADVER` AC1009.
    #[test]
    fn aama_r11_header_names_ac1009() {
        let text = export_to_string(&sample_r11_drawing(), "aama_header", &aama_options());
        let dxf = parse_dxf(&text);
        let version = dxf.all_pairs.windows(2).find(|w| w[0] == (9, "$ACADVER".to_string())).map(|w| w[1].1.clone());
        assert_eq!(version.as_deref(), Some("AC1009"));
        // The ASTM drawing in R11 is the same AC1009 file.
        assert_eq!(text, export_to_string(&sample_drawing(), "aama_astm_r11", &aama_options()));
    }

    // @brief Style text ends at `Units:`: AAMA has no curve tolerance and no ASTM version.
    #[test]
    fn aama_style_text_has_no_curve_tolerance_or_astm_version() {
        let dxf = parse_dxf(&export_to_string(&sample_r11_drawing(), "aama_style", &aama_options()));
        let texts: Vec<&str> = dxf.entities.iter().filter(|e| e.kind == "TEXT").map(|e| e.get(1).unwrap()).collect();
        assert_eq!(texts.last(), Some(&"Units:METRIC"));
        assert!(texts.iter().all(|t| !t.starts_with("Curve Tolerance:") && !t.starts_with("ASTM/")), "{texts:?}");
    }

    // @brief A DXF-AAMA block uses layers 1 to 14 only: dense polylines with no validation
    //        layer, every notch on layer 4, label text on layer 1 after the piece text.
    #[test]
    fn aama_block_uses_layers_1_to_14_only() {
        let mut drawing = sample_r11_drawing();
        drawing.blocks[0].size = Some("36".to_string());
        let dxf = parse_dxf(&export_to_string(&drawing, "aama_block", &aama_options()));
        let (_, body) = &dxf.blocks[0];

        let layers: Vec<u32> = body.iter().map(|e| e.layer().parse().expect("numeric layer")).collect();
        assert!(layers.iter().all(|l| (1..=14).contains(l)), "{layers:?}");

        let texts: Vec<(&str, &str)> = body.iter().filter(|e| e.kind == "TEXT").map(|e| (e.layer(), e.get(1).unwrap())).collect();
        assert_eq!(texts, vec![("1", "Piece Name:Front"), ("1", "Size:36"), ("1", "Quantity:1,1"), ("1", "Material:Fabric"), ("1", "Front")]);

        // One dense polyline per contour, on its own layer, with every key point of the contour.
        let block = &drawing.blocks[0];
        let contours = [("1", vec![block.boundary.as_ref().unwrap()]), ("14", block.sew_lines.iter().collect()), ("8", block.internal_lines.iter().collect()), ("11", block.cutouts.iter().collect())];
        for (layer, contours) in contours {
            let dense: Vec<usize> = contours.iter().map(|c| c.dense.len()).collect();
            assert_eq!(polyline_sizes(body, layer), dense, "layer {layer}");
        }

        // The T notch becomes a V notch on layer 4: depth, width and angle kept.
        let notches: Vec<&DxfEntity> = body.iter().filter(|e| e.kind == "POINT" && e.layer() == "4").collect();
        assert_eq!(notches.len(), 2);
        assert_eq!((notches[0].get(30), notches[0].get(39)), (Some("5.00"), None), "slit");
        assert_eq!((notches[1].get(30), notches[1].get(39), notches[1].get(50)), (Some("5.00"), Some("3.00"), Some("90.00")), "V");

        // Turn points stay on layer 2, the grainline on layer 7.
        assert_eq!(body.iter().filter(|e| e.kind == "POINT" && e.layer() == "2").count(), 11);
        assert_eq!(body.iter().find(|e| e.kind == "LINE").map(DxfEntity::layer), Some("7"));
    }

    // @brief A label line that reads as piece text or a rule identifier is not written in DXF-AAMA.
    #[test]
    fn aama_labels_never_repeat_piece_text() {
        let mut drawing = sample_r11_drawing();
        let label = |text: &str| Annotation { position: Point::new(30.0, 40.0), height: 5.0, rotation: 0.0, text: text.to_string() };
        drawing.blocks[0].annotations.extend(["Size: 40", "quantity:3", "# 2", "Cut 2 of Fabric", "Note: fold"].map(label));
        let dxf = parse_dxf(&export_to_string(&drawing, "aama_labels", &aama_options()));
        let texts: Vec<&str> = dxf.blocks[0].1.iter().filter(|e| e.kind == "TEXT").map(|e| e.get(1).unwrap()).collect();
        assert_eq!(texts, vec!["Piece Name:Front", "Quantity:1,1", "Material:Fabric", "Front", "Cut 2 of Fabric", "Note: fold"]);

        // DXF-ASTM keeps every label line on layer 15.
        let astm = parse_dxf(&export_to_string(&sample_drawing_with(&["Size: 40"]), "astm_labels", &dated_options()));
        assert!(astm.blocks[0].1.iter().any(|e| e.layer() == "15" && e.get(1) == Some("Size: 40")));
    }

    // @brief `sample_drawing` with extra label lines.
    fn sample_drawing_with(labels: &[&str]) -> Drawing {
        let mut drawing = sample_drawing();
        for text in labels {
            drawing.blocks[0].annotations.push(Annotation { position: Point::new(30.0, 40.0), height: 5.0, rotation: 0.0, text: text.to_string() });
        } // for each label
        drawing
    }

    // @brief Handoff of one multisize piece; `{label}` is its `piece_label` group, or nothing for a hidden label.
    const AAMA_HANDOFF: &str = r#"
        <svg viewBox="0 0 960 960" width="254mm" height="254mm" xmlns="http://www.w3.org/2000/svg">
          <g id="pattern-1" data-type="pattern" data-name="Skirt">
            <g id="piece_Front_s36" data-type="piece" data-name="Front" data-size="36" data-quantity="2" data-material="Lining">
              <g data-type="cutline"><path d="M 0,0 L 960,0 L 960,960 L 0,960 Z"/></g>
              <g data-type="grainline"><path d="M 480,800 L 470,780 L 490,780 L 480,800 L 480,200"/></g>
              {label}
            </g>
          </g>
        </svg>"#;

    // @brief Label lines that contradict the piece data, then two lines that do not.
    const AAMA_CONFLICTING_LABEL: &str = r#"
              <g data-type="piece_label">
                <g transform="matrix(1,0,0,1,300,400)"><text font-size="24" x="0" y="0">Piece Name: Back</text></g>
                <g transform="matrix(1,0,0,1,300,440)"><text font-size="24" x="0" y="0">Quantity: 9</text></g>
                <g transform="matrix(1,0,0,1,300,480)"><text font-size="24" x="0" y="0">size : 99</text></g>
                <g transform="matrix(1,0,0,1,300,520)"><text font-size="24" x="0" y="0">Material: Cotton</text></g>
                <g transform="matrix(1,0,0,1,300,560)"><text font-size="24" x="0" y="0">Cut 2 of Fabric</text></g>
                <g transform="matrix(1,0,0,1,300,600)"><text font-size="24" x="0" y="0">Note: fold</text></g>
              </g>"#;

    // @brief DXF-AAMA piece text comes from the piece data whether the label is hidden or shown:
    //        each field once, with the piece's value. Other label lines stay on layer 1.
    #[test]
    fn aama_piece_text_comes_from_piece_data_not_label() {
        let piece_text = ["Piece Name:Front", "Size:36", "Quantity:1,1", "Material:Lining"];
        let cases = [("hidden", "", vec![]), ("shown", AAMA_CONFLICTING_LABEL, vec!["Cut 2 of Fabric", "Note: fold"])];
        for (case, label, kept) in cases {
            let doc = svg_dom::Document::parse(&AAMA_HANDOFF.replace("{label}", label)).expect("fixture parses");
            let options = SvgToEzdxfOptions { dxf_version: DxfVersion::R11, ..SvgToEzdxfOptions::default() };
            let drawing = svg_to_ezdxf(&doc, &options).expect("converts");
            let dxf = parse_dxf(&export_to_string(&drawing, &format!("aama_piece_text_{case}"), &aama_options()));
            let texts: Vec<&str> =
                dxf.blocks[0].1.iter().filter(|e| e.kind == "TEXT" && e.layer() == "1").map(|e| e.get(1).unwrap()).collect();
            let expected: Vec<&str> = piece_text.into_iter().chain(kept).collect();
            assert_eq!(texts, expected, "{case} label");
        } // for each case
    }

    // @brief A graded DXF-AAMA export keeps layer 5 and the `# n` texts, and its rule file names ANSI/AAMA.
    #[test]
    fn aama_graded_export_writes_rule_ids_and_aama_rule_file() {
        let (drawing, _) = graded_drawing(DxfVersion::R11);
        let path = std::env::temp_dir().join(format!("seamly_aama_rul_{}.dxf", std::process::id()));
        export_dxf_astm(&drawing, &path, &aama_options()).expect("export succeeds");
        let dxf = parse_dxf(&std::fs::read_to_string(&path).expect("DXF written"));
        let rul_path = path.with_extension("rul");
        let rul = std::fs::read_to_string(&rul_path).expect("rule file beside the DXF");
        let _ = std::fs::remove_file(&path);
        let _ = std::fs::remove_file(&rul_path);

        let (name, front) = &dxf.blocks[0];
        assert_eq!(front.iter().filter(|e| e.kind == "LINE" && e.layer() == "5").count(), 1, "{name}: layer 5 line");
        assert!(front.iter().any(|e| e.kind == "TEXT" && e.layer() == "1" && e.get(1).is_some_and(|t| t.starts_with("# "))), "{name}: rule text");
        assert!(front.iter().any(|e| e.get(1) == Some("Size:36")), "{name}: AAMA size text");

        let lines: Vec<&str> = rul.lines().collect();
        assert_eq!(lines[0], "ANSI/AAMA VERSION:1.0.0");
        assert_eq!(lines[1..9], ["AUTHOR:Seamly2D", "CREATION DATE:11-10-2026", "CREATION TIME:14:05", "UNITS:METRIC", "GRADE RULE TABLE:GradingTest", "SAMPLE SIZE:36", "NUMBER OF SIZES:3", "SIZE LIST:34 36 38"]);
        assert!(lines[9].starts_with("RULE: DELTA 1 "), "{}", lines[9]);
    }

    // @brief The annotated version of a DXF-AAMA file names DXF-AAMA in its title line.
    #[test]
    fn aama_annotated_version_names_dxf_aama() {
        let path = std::env::temp_dir().join(format!("seamly_aama_annotated_{}.dxf", std::process::id()));
        let options = DxfAstmExportOptions { create_annotated_version: true, ..aama_options() };
        export_dxf_astm(&sample_r11_drawing(), &path, &options).expect("export succeeds");
        let txt_path = path.with_extension("txt");
        let annotated = std::fs::read_to_string(&txt_path).expect("annotated version written");
        let _ = std::fs::remove_file(&path);
        let _ = std::fs::remove_file(&txt_path);
        assert_eq!(annotated.lines().next(), Some("// DXF-AAMA Annotated Version with Inline Comments"));
    }

    // @brief DXF-AAMA writes the dense boundary, so a vertex on the fold between the
    //        mirror line ends must go: the boundary starts with the mirror line.
    #[test]
    fn aama_boundary_starts_on_the_mirror_line() {
        // A 100 x 200 mm half piece; the fold runs up its left edge with a vertex at (0, 100).
        let outline = [Point::new(100.0, 0.0), Point::new(100.0, 200.0), Point::new(0.0, 200.0), Point::new(0.0, 100.0), Point::new(0.0, 0.0)];
        let mut block = Block::new("piece_Back_M".to_string());
        block.piece_name = "Back".to_string();
        block.boundary = build_contour(&outline, true);
        block.mirror_line = Some((Point::new(0.0, 0.0), Point::new(0.0, 200.0)));
        let mut drawing = Drawing::new(DxfVersion::R11);
        drawing.add_block(block);
        assert!(seamly_svg2ezdxf::align_mirror_lines(&mut drawing).is_empty());
        assert!(drawing.blocks[0].boundary.as_ref().unwrap().dense.contains(&Point::new(0.0, 100.0)), "fold vertex in the dense line");

        let dxf = parse_dxf(&export_to_string(&drawing, "aama_mirror", &aama_options()));
        let (_, body) = &dxf.blocks[0];
        let mirror: Vec<&DxfEntity> = body.iter().filter(|e| e.kind == "LINE" && e.layer() == "6").collect();
        assert_eq!(mirror.len(), 1, "one mirror line");
        let ends = [mirror[0].get(10), mirror[0].get(20), mirror[0].get(11), mirror[0].get(21)];
        let first = body.iter().position(|e| e.kind == "POLYLINE" && e.layer() == "1").expect("boundary polyline");
        let vertices: Vec<&DxfEntity> = body[first + 1..].iter().take_while(|e| e.kind == "VERTEX").collect();
        let start = [vertices[0].get(10), vertices[0].get(20), vertices[1].get(10), vertices[1].get(20)];
        assert_eq!(ends, start);
        assert_eq!(vertices.len(), 4, "the fold vertex is not written");
    }

    // @brief `sample_drawing` for a pattern unit.
    fn sample_drawing_in(unit: seamly_svg2ezdxf::PatternUnit, version: DxfVersion) -> Drawing {
        let mut drawing = sample_drawing();
        drawing.version = version;
        drawing.pattern_unit = unit;
        drawing
    }

    // @brief An inch pattern writes `Units:ENGLISH` and every length in inches to four places;
    //        angles keep two places.
    #[test]
    fn english_units_write_inches_with_four_decimals() {
        let dxf = parse_dxf(&export_to_string(&sample_drawing_in(seamly_svg2ezdxf::PatternUnit::Inch, DxfVersion::R11), "english", &dated_options()));
        let texts: Vec<&str> = dxf.entities.iter().filter(|e| e.kind == "TEXT").map(|e| e.get(1).unwrap()).collect();
        assert!(texts.contains(&"Units:ENGLISH"), "{texts:?}");
        assert!(texts.contains(&"Curve Tolerance:0.0098"), "{texts:?}");
        let block = &dxf.blocks[0].1;
        // System text height 5 mm.
        assert!(block.iter().filter(|e| e.kind == "TEXT").all(|e| e.get(40).is_some()));
        assert_eq!(block.iter().find(|e| e.kind == "TEXT").unwrap().get(40), Some("0.1969"));
        // The 100 mm boundary corner.
        let boundary_xs: Vec<&str> = block.iter().filter(|e| e.kind == "VERTEX" && e.layer() == "1").filter_map(|e| e.get(10)).collect();
        assert!(boundary_xs.contains(&"3.9370"), "{boundary_xs:?}");
        // Notch depth 5 mm and width 3 mm; angle stays in degrees.
        let notch = block.iter().find(|e| e.kind == "POINT" && e.layer() == "80").or_else(|| block.iter().find(|e| e.kind == "POINT" && e.layer() == "4" && e.get(39).is_some())).expect("T notch");
        assert_eq!(notch.get(30), Some("0.1969"));
        assert_eq!(notch.get(39), Some("0.1181"));
        assert_eq!(notch.get(50), Some("90.00"));
    }

    // @brief A cm pattern and a pattern without units write METRIC in millimetres.
    #[test]
    fn cm_and_unspecified_units_write_metric() {
        for unit in [seamly_svg2ezdxf::PatternUnit::Cm, seamly_svg2ezdxf::PatternUnit::Mm, seamly_svg2ezdxf::PatternUnit::Unspecified] {
            let text = export_to_string(&sample_drawing_in(unit, DxfVersion::R11), "metric_units", &dated_options());
            assert_eq!(text, export_to_string(&sample_drawing(), "metric_default", &dated_options()));
            assert!(text.contains("Units:METRIC"));
        }
    }

    // @brief DXF R13 in inches: the same lengths, no millimetre value left.
    #[test]
    fn english_units_in_r13() {
        let text = export_to_string(&sample_drawing_in(seamly_svg2ezdxf::PatternUnit::Inch, DxfVersion::R13), "english_r13", &dated_options());
        assert!(text.contains("Units:ENGLISH"));
        assert!(text.contains("3.9370"));
        assert!(!text.contains("
100.00
"), "a boundary coordinate is still in millimetres");
    }

    // @brief The `.rul` file of an inch pattern: `UNITS:ENGLISH`, growth in inches to four places.
    #[test]
    fn english_units_write_inches_in_the_rule_file() {
        let (mut drawing, _) = graded_drawing(DxfVersion::R11);
        drawing.pattern_unit = seamly_svg2ezdxf::PatternUnit::Inch;
        let table = drawing.grade_rule_table.as_ref().expect("table");
        let metric = crate::grade_rule_file_text(table, seamly_svg2ezdxf::DxfUnits::Metric, &dated_options());
        let english = crate::grade_rule_file_text(table, seamly_svg2ezdxf::DxfUnits::English, &dated_options());
        assert!(metric.contains("UNITS:METRIC") && english.contains("UNITS:ENGLISH"));
        // Every growth value has four decimals; the sample size column is zero.
        let rule = english.lines().find(|l| l.starts_with("RULE:")).expect("a rule");
        for growth in rule.split_whitespace().skip(3).flat_map(|g| g.split(',')) {
            assert_eq!(growth.split('.').nth(1).map(str::len), Some(4), "{rule}");
        }
        assert!(!english.contains("-0.0000"));
        // The first non-zero metric growth, in inches.
        let first = |t: &str| t.lines().find(|l| l.starts_with("RULE:")).unwrap().split_whitespace().nth(3).unwrap().to_string();
        let x_mm: f64 = first(&metric).split(',').next().unwrap().parse().unwrap();
        let x_in: f64 = first(&english).split(',').next().unwrap().parse().unwrap();
        assert!((x_mm / 25.4 - x_in).abs() < 0.0001);
    }
}
