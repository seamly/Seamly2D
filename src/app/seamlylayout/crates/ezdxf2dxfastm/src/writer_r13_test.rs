// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief DXF R13 (AC1012) structure tests for the DXF-ASTM writer.

#[cfg(test)]
mod tests {
    use crate::r13::upgrade_to_r13;
    use crate::writer::{export_dxf_astm, DxfAstmExportOptions};
    use seamly_svg2ezdxf::{build_contour, svg_to_ezdxf, Annotation, Block, Drawing, DxfVersion, Notch, NotchKind, Point, SvgToEzdxfOptions};
    use std::collections::HashSet;

    // @brief One DXF record: its group 0 value and the pairs after it.
    struct Rec {
        kind: String,
        groups: Vec<(i32, String)>,
    }

    impl Rec {
        // @brief First value of a group code.
        fn get(&self, code: i32) -> Option<&str> {
            self.groups.iter().find(|(c, _)| *c == code).map(|(_, v)| v.as_str())
        }
        // @brief Every group 100 value, in order.
        fn markers(&self) -> Vec<&str> {
            self.groups.iter().filter(|(c, _)| *c == 100).map(|(_, v)| v.as_str()).collect()
        }
    }

    // @brief DXF text as records, each tagged with its section name.
    fn parse(text: &str) -> Vec<(String, Rec)> {
        let lines: Vec<&str> = text.lines().collect();
        assert_eq!(lines.len() % 2, 0, "DXF text is group pairs");
        let mut out: Vec<(String, Rec)> = Vec::new();
        let mut section = String::new();
        for c in lines.chunks(2) {
            let code: i32 = c[0].trim().parse().expect("group code");
            let value = c[1].to_string();
            if code == 0 {
                // ENDSEC and EOF belong to no section.
                if value == "ENDSEC" || value == "EOF" {
                    section.clear();
                }
                out.push((section.clone(), Rec { kind: value, groups: Vec::new() }));
            } else if let Some((_, r)) = out.last_mut() {
                if r.kind == "SECTION" && code == 2 {
                    section = value.clone();
                }
                r.groups.push((code, value));
            }
        }
        out
    }

    // @brief One piece with every ASTM feature.
    fn sample_drawing(version: DxfVersion) -> Drawing {
        let square = |s: f64| vec![Point::new(0.0, 0.0), Point::new(s, 0.0), Point::new(s, s), Point::new(0.0, s)];
        let mut block = Block::new("piece_Front_M".to_string());
        block.piece_name = "Front".to_string();
        block.quantity = Some("1,1".to_string());
        block.boundary = build_contour(&square(100.0), true);
        block.sew_lines.push(build_contour(&[Point::new(10.0, 10.0), Point::new(90.0, 10.0), Point::new(90.0, 90.0), Point::new(10.0, 90.0)], true).unwrap());
        block.internal_lines.push(build_contour(&[Point::new(20.0, 50.0), Point::new(80.0, 50.0)], false).unwrap());
        block.grainline = Some((Point::new(50.0, 20.0), Point::new(50.0, 80.0)));
        block.notches.push(Notch { base: Point::new(30.0, 0.0), angle_deg: 90.0, depth: 5.0, width: 3.0, kind: NotchKind::T });
        block.annotations.push(Annotation { position: Point::new(30.0, 30.0), height: 5.0, rotation: 0.0, text: "Front".to_string() });
        let mut drawing = Drawing::new(version);
        drawing.style_name = Some("Test Shirt".to_string());
        drawing.add_block(block);
        drawing
    }

    // @brief Export to a temp file and return the DXF text.
    fn export(drawing: &Drawing, tag: &str, options: &DxfAstmExportOptions) -> String {
        let path = std::env::temp_dir().join(format!("seamly_r13_{}_{}.dxf", tag, std::process::id()));
        export_dxf_astm(drawing, &path, options).expect("export succeeds");
        let text = std::fs::read_to_string(&path).expect("read back");
        let _ = std::fs::remove_file(&path);
        text
    }

    // @brief Fixed date and time, so R12 and R13 exports compare equal.
    fn fixed_options() -> DxfAstmExportOptions {
        DxfAstmExportOptions {
            creation_date: Some("03-10-2026".to_string()),
            creation_time: Some("12-00".to_string()),
            ..DxfAstmExportOptions::default()
        }
    }

    #[test]
    fn header_names_ac1012_and_handseed_exceeds_every_handle() {
        let text = export(&sample_drawing(DxfVersion::R13), "header", &fixed_options());
        let recs = parse(&text);
        let header = &recs[0].1;
        assert_eq!(recs[0].0, "", "HEADER section record comes first");
        let pairs = &header.groups;
        let after = |var: &str| pairs.iter().position(|(c, v)| *c == 9 && v == var).map(|i| pairs[i + 1].1.clone());
        assert_eq!(after("$ACADVER").as_deref(), Some("AC1012"));
        let seed = u32::from_str_radix(&after("$HANDSEED").expect("$HANDSEED"), 16).expect("hex seed");
        // The HEADER pairs belong to its SECTION record, so skip SECTION records.
        let max = recs
            .iter()
            .filter(|(_, r)| r.kind != "SECTION")
            .flat_map(|(_, r)| r.groups.iter())
            .filter(|(c, _)| *c == 5 || *c == 105)
            .filter_map(|(_, v)| u32::from_str_radix(v, 16).ok())
            .max()
            .expect("handles present");
        assert!(seed > max, "$HANDSEED {seed:X} must exceed the largest handle {max:X}");
    }

    #[test]
    fn every_entity_table_entry_and_object_has_a_unique_handle() {
        let text = export(&sample_drawing(DxfVersion::R13), "handles", &fixed_options());
        let recs = parse(&text);
        let mut seen = HashSet::new();
        for (section, r) in &recs {
            if matches!(r.kind.as_str(), "SECTION" | "ENDSEC" | "ENDTAB" | "EOF") {
                continue;
            }
            let code = if r.kind == "DIMSTYLE" { 105 } else { 5 };
            let handle = r.get(code).unwrap_or_else(|| panic!("{} in {section} has no handle", r.kind));
            assert!(u32::from_str_radix(handle, 16).is_ok_and(|h| h > 0), "handle '{handle}' is non-zero hex");
            assert!(seen.insert(handle.to_string()), "handle {handle} is used twice");
        }
    }

    #[test]
    fn required_sections_and_tables_are_present_in_order() {
        let text = export(&sample_drawing(DxfVersion::R13), "sections", &fixed_options());
        let recs = parse(&text);
        let sections: Vec<&str> = recs.iter().filter(|(_, r)| r.kind == "SECTION").filter_map(|(_, r)| r.get(2)).collect();
        assert_eq!(sections, vec!["HEADER", "CLASSES", "TABLES", "BLOCKS", "ENTITIES", "OBJECTS"]);
        assert_eq!(recs.last().map(|(_, r)| r.kind.as_str()), Some("EOF"));

        let tables: Vec<&str> = recs.iter().filter(|(_, r)| r.kind == "TABLE").filter_map(|(_, r)| r.get(2)).collect();
        assert_eq!(tables, vec!["VPORT", "LTYPE", "LAYER", "STYLE", "VIEW", "UCS", "APPID", "DIMSTYLE", "BLOCK_RECORD"]);

        // Every block has a block record; the layout blocks come first.
        let names = |kind: &str| -> Vec<String> {
            recs.iter().filter(|(_, r)| r.kind == kind).filter_map(|(_, r)| r.get(2).map(str::to_string)).collect()
        };
        assert_eq!(names("BLOCK_RECORD"), vec!["*MODEL_SPACE", "*PAPER_SPACE", "piece_Front_M"]);
        assert_eq!(names("BLOCK"), names("BLOCK_RECORD"));

        // Every layer an entity uses is in the LAYER table.
        let table: HashSet<String> = names("LAYER").into_iter().collect();
        for (section, r) in &recs {
            if section == "BLOCKS" || section == "ENTITIES" {
                let layer = r.get(8).unwrap_or_else(|| panic!("{} has no layer", r.kind));
                assert!(table.contains(layer), "layer {layer} is missing from the LAYER table");
            }
        }

        // OBJECTS: the named object dictionary points at ACAD_GROUP.
        let dicts: Vec<&Rec> = recs.iter().filter(|(_, r)| r.kind == "DICTIONARY").map(|(_, r)| r).collect();
        assert_eq!(dicts.len(), 2);
        assert_eq!(dicts[0].get(3), Some("ACAD_GROUP"));
        assert_eq!(dicts[0].get(350), dicts[1].get(5));
        assert_eq!(dicts[1].get(330), dicts[0].get(5));
    }

    #[test]
    fn entities_carry_subclass_markers() {
        let text = export(&sample_drawing(DxfVersion::R13), "markers", &fixed_options());
        let recs = parse(&text);
        let expected = [
            ("LINE", vec!["AcDbEntity", "AcDbLine"]),
            ("POINT", vec!["AcDbEntity", "AcDbPoint"]),
            ("TEXT", vec!["AcDbEntity", "AcDbText", "AcDbText"]),
            ("POLYLINE", vec!["AcDbEntity", "AcDb2dPolyline"]),
            ("VERTEX", vec!["AcDbEntity", "AcDbVertex", "AcDb2dVertex"]),
            ("SEQEND", vec!["AcDbEntity"]),
            ("INSERT", vec!["AcDbEntity", "AcDbBlockReference"]),
            ("BLOCK", vec!["AcDbEntity", "AcDbBlockBegin"]),
            ("ENDBLK", vec!["AcDbEntity", "AcDbBlockEnd"]),
        ];
        for (kind, markers) in expected {
            let found: Vec<&Rec> = recs.iter().filter(|(_, r)| r.kind == kind).map(|(_, r)| r).collect();
            assert!(!found.is_empty(), "sample has a {kind}");
            for r in found {
                assert_eq!(r.markers(), markers, "{kind} subclass markers");
            }
        }
        // R13 POLYLINE: dummy point, no obsolete group 66.
        let poly = recs.iter().find(|(_, r)| r.kind == "POLYLINE").map(|(_, r)| r).unwrap();
        assert_eq!((poly.get(10), poly.get(20), poly.get(30), poly.get(66)), (Some("0.0"), Some("0.0"), Some("0.0"), None));
    }

    // @brief R13 holds the same D6673 entities, layers and values as R12.
    #[test]
    fn r13_keeps_the_d6673_content_of_r12() {
        // Pairs that are D6673 content: handles, markers, dummy points and R13-only records removed.
        let content = |text: &str| -> Vec<(String, Vec<(i32, String)>)> {
            parse(text)
                .into_iter()
                .filter(|(s, r)| (s == "BLOCKS" || s == "ENTITIES") && r.kind != "SECTION")
                .filter(|(_, r)| !r.get(2).is_some_and(|n| n.starts_with('*')))
                .filter(|(_, r)| !(r.kind == "ENDBLK"))
                .map(|(_, r)| {
                    let is_poly = r.kind == "POLYLINE";
                    let groups = r
                        .groups
                        .into_iter()
                        .filter(|(c, _)| !matches!(c, 5 | 100 | 66))
                        .filter(|(c, _)| !(is_poly && matches!(c, 10 | 20 | 30)))
                        .filter(|(c, v)| !(*c == 1 && v.is_empty()))
                        .collect();
                    (r.kind, groups)
                })
                .collect()
        };
        let r12 = export(&sample_drawing(DxfVersion::R12), "same12", &fixed_options());
        let r13 = export(&sample_drawing(DxfVersion::R13), "same13", &fixed_options());
        assert!(r12.contains("AC1009"));
        assert_eq!(content(&r12), content(&r13));
    }

    #[test]
    fn clo3d_group_250_survives_in_r13() {
        let options = DxfAstmExportOptions { clo3d_group_250: true, ..fixed_options() };
        let recs = parse(&export(&sample_drawing(DxfVersion::R13), "clo3d", &options));
        let marked: Vec<(&str, &str)> = recs
            .iter()
            .filter(|(_, r)| r.kind == "POLYLINE")
            .filter_map(|(_, r)| r.get(250).map(|v| (r.get(8).unwrap(), v)))
            .collect();
        assert_eq!(marked, vec![("1", "0"), ("14", "2")]);
    }

    #[test]
    fn male_shirt_fixture_exports_as_r13() {
        let svg = std::fs::read_to_string(concat!(env!("CARGO_MANIFEST_DIR"), "/test_data/male_shirt_pieces.svg"))
            .expect("fixture present");
        let doc = svg_dom::Document::parse(&svg).expect("fixture parses");
        let svg_opts = SvgToEzdxfOptions { dxf_version: DxfVersion::R13, ..SvgToEzdxfOptions::default() };
        let drawing = svg_to_ezdxf(&doc, &svg_opts).expect("converts");
        let recs = parse(&export(&drawing, "shirt", &fixed_options()));
        let records = recs.iter().filter(|(_, r)| r.kind == "BLOCK_RECORD").count();
        assert_eq!(records, 2 + 17, "layout blocks plus one block per piece");
    }

    #[test]
    fn unknown_entity_is_refused() {
        let r12 = "0\nSECTION\n2\nENTITIES\n0\nSPLINE\n8\n1\n0\nENDSEC\n0\nEOF\n";
        let err = upgrade_to_r13(r12).expect_err("SPLINE has no R13 form here");
        assert!(err.to_string().contains("SPLINE"));
    }

    #[test]
    fn teaching_version_explains_r13_codes() {
        let path = std::env::temp_dir().join(format!("seamly_r13_teach_{}.dxf", std::process::id()));
        let options = DxfAstmExportOptions { create_teaching_version: true, ..fixed_options() };
        export_dxf_astm(&sample_drawing(DxfVersion::R13), &path, &options).expect("export succeeds");
        let txt = path.with_extension("txt");
        let teaching = std::fs::read_to_string(&txt).expect("teaching file");
        let _ = std::fs::remove_file(&path);
        let _ = std::fs::remove_file(&txt);
        assert!(teaching.contains("Subclass marker: AcDbEntity"));
        assert!(teaching.contains("Section name: OBJECTS"));
        // A value of "0" (layer 0) is a value, not an entity marker for the next line.
        assert!(teaching.contains("Layer name: 0 "));
        assert!(!teaching.contains("Entity type: 5"));
    }
}
