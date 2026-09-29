// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Tests for SVG to ezdxf conversion.

#[cfg(test)]
mod tests {
    use crate::converter::{SvgToEzdxfOptions, svg_to_ezdxf};
    use crate::drawing::DxfVersion;
    use svg_dom::Document;

    // @brief Test converting a simple SVG with a line to ezdxf Drawing.
    #[test]
    fn test_convert_simple_line() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <line x1="10" y1="20" x2="50" y2="60" stroke="black"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: true,
            svg_height: Some(100.0),
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        // Verify drawing structure.
        assert_eq!(drawing.version, DxfVersion::R12);
        assert_eq!(drawing.blocks.len(), 0);
        assert_eq!(drawing.modelspace_entities.len(), 1);

        // Verify line entity.
        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "LINE");
        assert_eq!(entity.layer(), "8"); // layer 8 = internal lines
    }

    // @brief Test converting a simple SVG with a circle to ezdxf Drawing.
    #[test]
    fn test_convert_simple_circle() {
        let svg = r#"
            <svg width="200" height="200" xmlns="http://www.w3.org/2000/svg">
                <circle cx="100" cy="100" r="25" fill="none" stroke="black"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: true,
            svg_height: Some(200.0),
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        assert_eq!(drawing.modelspace_entities.len(), 1);

        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "CIRCLE");
        assert_eq!(entity.layer(), "8"); // layer 8 = internal lines
    }

    // @brief Test converting pattern pieces to blocks.
    #[test]
    fn test_convert_pattern_pieces_to_blocks() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <g id="piece1">
                    <line x1="0" y1="0" x2="10" y2="10"/>
                </g>
                <g id="piece2">
                    <circle cx="50" cy="50" r="5"/>
                </g>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: true,
            invert_y: true,
            svg_height: Some(100.0),
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        // Should have 2 blocks.
        assert_eq!(drawing.blocks.len(), 2);
        assert_eq!(drawing.modelspace_entities.len(), 0);

        // Check block names (all get "_M" suffix per CLO3D convention).
        assert_eq!(drawing.blocks[0].name, "piece1_M");
        assert_eq!(drawing.blocks[1].name, "piece2_M");

        // Check entities in blocks.
        assert_eq!(drawing.blocks[0].entities.len(), 1);
        assert_eq!(drawing.blocks[1].entities.len(), 1);

        assert_eq!(drawing.blocks[0].entities[0].entity_type(), "LINE");
        assert_eq!(drawing.blocks[1].entities[0].entity_type(), "CIRCLE");
    }

    // @brief Component groups set the layer of their geometry; the piece id never does.
    #[test]
    fn test_layer_mapping() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <g id="Buttonhole_Placket">
                    <g id="cutline_piece1"><line x1="0" y1="0" x2="10" y2="10"/></g>
                    <g id="notch_mark1"><circle cx="20" cy="20" r="2"/></g>
                    <g data-type="cut_path"><line x1="1" y1="1" x2="5" y2="5"/></g>
                    <line x1="0" y1="5" x2="10" y2="5"/>
                    <text x="30" y="30">Label</text>
                </g>
            </svg>
        "#;
        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let drawing = svg_to_ezdxf(&doc, &SvgToEzdxfOptions::default()).expect("Failed to convert");

        assert_eq!(drawing.blocks.len(), 1);
        let layers: Vec<&str> = drawing.blocks[0].entities.iter().map(|e| e.layer()).collect();
        // cutline → 1, notch → 4, cut_path → 11, plain line → 8 (not 13 from "hole"), text → 15.
        assert_eq!(layers, vec!["1", "4", "11", "8", "15"]);
    }

    // @brief Test coordinate transformation (Y-axis inversion).
    #[test]
    fn test_coordinate_transformation() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <line x1="10" y1="20" x2="50" y2="80"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");

        // Test with Y inversion.
        let options_with_invert = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: true,
            svg_height: Some(100.0),
            ..Default::default()
        };

        let drawing_inverted = svg_to_ezdxf(&doc, &options_with_invert).expect("Failed to convert");
        assert_eq!(drawing_inverted.modelspace_entities.len(), 1);

        // Test without Y inversion.
        let options_no_invert = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: false,
            ..Default::default()
        };

        let drawing_not_inverted =
            svg_to_ezdxf(&doc, &options_no_invert).expect("Failed to convert");
        assert_eq!(drawing_not_inverted.modelspace_entities.len(), 1);
    }

    // @brief Test text conversion and ASCII sanitization.
    #[test]
    fn test_text_conversion() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <text x="10" y="20" font-size="14">Hello World</text>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: false,
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        // Debug: Print drawing structure if test fails.
        eprintln!("Drawing structure:");
        eprintln!("  Blocks: {}", drawing.blocks.len());
        eprintln!(
            "  Modelspace entities: {}",
            drawing.modelspace_entities.len()
        );

        assert_eq!(
            drawing.modelspace_entities.len(),
            1,
            "Expected 1 modelspace entity, found {}. Blocks: {}, Modelspace: {}",
            drawing.modelspace_entities.len(),
            drawing.blocks.len(),
            drawing.modelspace_entities.len()
        );

        let entity = &drawing.modelspace_entities[0];

        assert_eq!(
            entity.entity_type(),
            "TEXT",
            "Expected TEXT entity, found '{}'",
            entity.entity_type()
        );

        assert_eq!(
            entity.layer(),
            "15",
            "Expected ASTM layer '15' (annotation text), found '{}'",
            entity.layer()
        );
    }

    // @brief Test path conversion to polyline.
    #[test]
    fn test_convert_path() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <path d="M10 10 L50 10 L50 50 L10 50 Z"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: false,
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        assert_eq!(drawing.modelspace_entities.len(), 1);
        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "POLYLINE");
        assert_eq!(entity.layer(), "8"); // layer 8 = internal lines
    }

    // @brief Test path with curves conversion.
    #[test]
    fn test_convert_path_with_curves() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <path d="M10 10 Q20 20 30 10"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: false,
            flatten_tolerance: 0.1,
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        assert_eq!(drawing.modelspace_entities.len(), 1);
        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "POLYLINE");
    }

    // @brief Test closed path conversion.
    #[test]
    fn test_convert_closed_path() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <path d="M10 10 L20 10 L20 20 L10 20 Z"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: false,
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        assert_eq!(drawing.modelspace_entities.len(), 1);
        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "POLYLINE");
    }

    // @brief Test writing drawing to output directory.
    #[test]
    fn test_write_drawing_to_output() {
        use crate::write_drawing_to_output;

        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <g id="piece1">
                    <line x1="0" y1="0" x2="10" y2="10"/>
                </g>
                <path d="M20 20 L30 20 L30 30 Z"/>
                <text x="40" y="40">Test</text>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: true,
            invert_y: false,
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        // Write to output directory.
        let file_path = write_drawing_to_output(&drawing, Some("test_ezdxf_drawing.txt"))
            .expect("Failed to write drawing to file");

        // Verify file was created.
        assert!(
            file_path.exists(),
            "Output file should exist: {:?}",
            file_path
        );

        // Verify file contents.
        let contents = std::fs::read_to_string(&file_path).expect("Failed to read output file");
        assert!(
            contents.contains("DXF Version"),
            "File should contain DXF version"
        );
        assert!(contents.contains("Blocks: 1"), "File should show 1 block");
        assert!(
            contents.contains("piece1"),
            "File should contain block name"
        );
    }

    // @brief Test polyline conversion.
    #[test]
    fn test_convert_polyline() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <polyline points="10,10 20,20 30,10 40,20"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: false,
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        assert_eq!(drawing.modelspace_entities.len(), 1);
        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "POLYLINE");
    }

    // @brief Test polygon conversion.
    #[test]
    fn test_convert_polygon() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <polygon points="10,10 20,20 30,10"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: false,
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        assert_eq!(drawing.modelspace_entities.len(), 1);
        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "POLYLINE");
    }

    // @brief Test rect conversion.
    #[test]
    fn test_convert_rect() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <rect x="10" y="20" width="30" height="40"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: false,
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        assert_eq!(drawing.modelspace_entities.len(), 1);
        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "POLYLINE");
    }

    // @brief Test ellipse conversion to circle (rx == ry).
    #[test]
    fn test_convert_ellipse_to_circle() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <ellipse cx="50" cy="50" rx="25" ry="25"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: false,
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        assert_eq!(drawing.modelspace_entities.len(), 1);
        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "CIRCLE");
    }

    // @brief Test ellipse conversion to polyline (rx != ry).
    #[test]
    fn test_convert_ellipse_to_polyline() {
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <ellipse cx="50" cy="50" rx="30" ry="20"/>
            </svg>
        "#;

        let doc = Document::parse(svg).expect("Failed to parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: false,
            ..Default::default()
        };

        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");

        assert_eq!(drawing.modelspace_entities.len(), 1);
        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "POLYLINE");
    }
}
