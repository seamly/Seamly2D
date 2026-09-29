// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief ASTM D6673-10 structure tests for the DXF-ASTM writer.

#[cfg(test)]
mod tests {
    use crate::writer::{export_dxf_astm, DxfAstmExportOptions};
    use seamly_svg2ezdxf::{
        build_contour, svg_to_ezdxf, Annotation, Block, Drawing, DxfVersion, Notch, NotchKind, Point, SvgToEzdxfOptions,
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
        let mut drawing = Drawing::new(DxfVersion::R12);
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
                "Author:Seamly2D Project;SeamlyLayout;1.2.3",
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
    fn piece_block_uses_astm_layers_and_entities() {
        let dxf = parse_dxf(&export_to_string(&sample_drawing(), "block", &DxfAstmExportOptions::default()));
        assert_eq!(dxf.blocks.len(), 1);
        let (_, body) = &dxf.blocks[0];

        let texts: Vec<(&str, &str)> = body.iter().filter(|e| e.kind == "TEXT").map(|e| (e.layer(), e.get(1).unwrap())).collect();
        assert_eq!(texts, vec![("1", "Piece Name:Front"), ("1", "Quantity:1,1"), ("15", "Front")]);

        // Every primary layer has a matching validation layer with the same polyline count.
        for (primary, validation) in [("1", "84"), ("14", "87"), ("8", "85"), ("11", "86")] {
            let (p, v) = (polyline_sizes(body, primary), polyline_sizes(body, validation));
            assert_eq!(p.len(), 1, "one polyline on layer {primary}");
            assert_eq!(p.len(), v.len(), "layer {validation} mirrors layer {primary}");
            assert!(p.iter().zip(&v).all(|(a, b)| a <= b));
        }

        // Turn and curve points: 4 + 4 + 2 + 3 key points, all corners or ends here.
        let points_on = |l: &str| body.iter().filter(|e| e.kind == "POINT" && e.layer() == l).count();
        assert_eq!(points_on("2"), 13);
        assert_eq!(points_on("3"), 0);

        // Notches: POINT with depth (30) and angle (50); width (39) only when it has one.
        let slit = body.iter().find(|e| e.kind == "POINT" && e.layer() == "4").expect("slit notch");
        assert_eq!((slit.get(30), slit.get(39), slit.get(50)), (Some("5.00"), None, Some("90.00")));
        let t = body.iter().find(|e| e.kind == "POINT" && e.layer() == "80").expect("T notch");
        assert_eq!((t.get(30), t.get(39)), (Some("5.00"), Some("3.00")));

        let grain = body.iter().find(|e| e.kind == "LINE").expect("grainline");
        assert_eq!(grain.layer(), "7");
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

    // @brief The male shirt handoff SVG exports with every D6673 requirement met.
    #[test]
    fn male_shirt_fixture_exports_complete_astm_file() {
        let svg = std::fs::read_to_string(concat!(env!("CARGO_MANIFEST_DIR"), "/test_data/male_shirt_pieces.svg"))
            .expect("fixture present");
        let doc = svg_dom::Document::parse(&svg).expect("fixture parses");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions::default()).expect("converts");
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
            // Turn and curve points equal the key points of every primary polyline.
            let key_points: usize = ["1", "8", "11", "14"].iter().map(|l| polyline_sizes(body, l).iter().sum::<usize>()).sum();
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
    }
}
