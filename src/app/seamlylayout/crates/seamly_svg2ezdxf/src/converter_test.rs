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

    // @brief Test layer mapping.
    #[test]
    fn test_layer_mapping() {
        eprintln!("\n");
        eprintln!("═══════════════════════════════════════════════════════════════");
        eprintln!("  test_layer_mapping: STARTING TEST");
        eprintln!("═══════════════════════════════════════════════════════════════");
        eprintln!();

        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <g id="cutline_piece1">
                    <line x1="0" y1="0" x2="10" y2="10"/>
                </g>
                <g id="notch_mark1">
                    <circle cx="20" cy="20" r="2"/>
                </g>
                <text x="30" y="30">Label</text>
            </svg>
        "#;

        eprintln!("[STEP 1] Input SVG Analysis");
        eprintln!("  └─ SVG content length: {} bytes", svg.len());
        eprintln!("  └─ SVG contains:");
        eprintln!("      • Group element with id='cutline_piece1' containing a <line>");
        eprintln!("        → Expected: Should create block 'cutline_piece1'");
        eprintln!("        → Expected: Line entity should be on layer '1' (Piece boundary)");
        eprintln!("      • Group element with id='notch_mark1' containing a <circle>");
        eprintln!("        → Expected: Should create block 'notch_mark1'");
        eprintln!("        → Expected: Circle entity should be on layer '4' (Notches)");
        eprintln!("      • Text element with content 'Label'");
        eprintln!("        → Expected: Should be in modelspace (not in a block)");
        eprintln!("        → Expected: Should be on layer '9' (Text/Annotations)");
        eprintln!();

        eprintln!("[STEP 2] Parsing SVG Document");
        let doc = Document::parse(svg).expect("Failed to parse SVG");
        eprintln!("  └─ SVG parsing: SUCCESS");
        eprintln!("  └─ Root element name: '{}'", doc.root.name);
        eprintln!("  └─ Root attributes count: {}", doc.root.attributes.len());
        eprintln!("  └─ Root children count: {}", doc.root.children.len());

        // Analyze root children
        let mut group_count = 0;
        let mut text_count = 0;
        let mut other_count = 0;
        for child in &doc.root.children {
            if let xmltree::XMLNode::Element(elem) = child {
                match elem.name.as_str() {
                    "g" => {
                        group_count += 1;
                        let id = elem
                            .attributes
                            .get("id")
                            .map(|s| s.as_str())
                            .unwrap_or("<no id>");
                        eprintln!("    • Found <g> element with id='{}'", id);
                    }
                    "text" => {
                        text_count += 1;
                        eprintln!("    • Found <text> element");
                    }
                    _ => {
                        other_count += 1;
                        eprintln!("    • Found <{}> element", elem.name);
                    }
                }
            }
        }
        eprintln!(
            "  └─ Summary: {} groups, {} text elements, {} other elements",
            group_count, text_count, other_count
        );
        eprintln!();

        eprintln!("[STEP 3] Setting Conversion Options");
        let options = SvgToEzdxfOptions {
            create_blocks: true,
            invert_y: false, // Don't invert for simpler test.
            ..Default::default()
        };
        eprintln!(
            "  └─ create_blocks: {} (will extract pattern pieces as blocks)",
            options.create_blocks
        );
        eprintln!(
            "  └─ invert_y: {} (Y-axis will NOT be inverted)",
            options.invert_y
        );
        eprintln!("  └─ dxf_version: {:?}", options.dxf_version);
        eprintln!("  └─ flatten_tolerance: {}", options.flatten_tolerance);
        eprintln!("  └─ svg_height: {:?}", options.svg_height);
        eprintln!();

        eprintln!("[STEP 4] Converting SVG to ezdxf Drawing");
        eprintln!("  └─ Calling svg_to_ezdxf()...");
        let drawing = svg_to_ezdxf(&doc, &options).expect("Failed to convert");
        eprintln!("  └─ Conversion: SUCCESS");
        eprintln!();

        eprintln!("[STEP 5] Analyzing Converted Drawing Structure");
        eprintln!("  └─ DXF Version: {:?}", drawing.version);
        eprintln!("  └─ Total Blocks Created: {}", drawing.blocks.len());
        eprintln!(
            "  └─ Total Modelspace Entities: {}",
            drawing.modelspace_entities.len()
        );
        eprintln!();

        if drawing.blocks.is_empty() {
            eprintln!("  ⚠️  WARNING: No blocks found!");
            eprintln!("     Expected: 2 blocks (from groups 'cutline_piece1' and 'notch_mark1')");
            eprintln!("     Found: 0 blocks");
            eprintln!("     Possible causes:");
            eprintln!("       • Pattern piece extraction logic is not finding <g> elements");
            eprintln!("       • <g> elements don't have 'id' attributes");
            eprintln!("       • create_blocks option is not being respected");
        } else {
            eprintln!("  └─ Blocks Detail:");
            for (i, block) in drawing.blocks.iter().enumerate() {
                eprintln!("      Block {}:", i);
                eprintln!("        • Name: '{}'", block.name);
                eprintln!("        • Entity count: {}", block.entities.len());

                if block.entities.is_empty() {
                    eprintln!("        ⚠️  WARNING: Block has NO entities!");
                    eprintln!("           Expected: At least 1 entity (line or circle)");
                    eprintln!("           Possible causes:");
                    eprintln!("             • Entities are not being added to blocks");
                    eprintln!("             • Entity conversion is failing silently");
                    eprintln!("             • Entities are going to modelspace instead");
                } else {
                    eprintln!("        • Entities:");
                    for (j, entity) in block.entities.iter().enumerate() {
                        eprintln!("          Entity {}:", j);
                        eprintln!("            - Type: '{}'", entity.entity_type());
                        eprintln!("            - Layer: '{}'", entity.layer());

                        // Additional entity-specific debug info
                        match entity.entity_type() {
                            "LINE" => {
                                eprintln!("            - Entity type: LINE");
                            }
                            "CIRCLE" => {
                                eprintln!("            - Entity type: CIRCLE");
                            }
                            "TEXT" => {
                                eprintln!("            - Entity type: TEXT");
                            }
                            _ => {
                                eprintln!(
                                    "            - Entity type: {} (unexpected)",
                                    entity.entity_type()
                                );
                            }
                        }
                    }
                }
                eprintln!();
            }
        }

        if drawing.modelspace_entities.is_empty() {
            eprintln!("  ⚠️  WARNING: No modelspace entities found!");
            eprintln!("     Expected: 1 text entity (from <text> element)");
            eprintln!("     Found: 0 modelspace entities");
            eprintln!("     Possible causes:");
            eprintln!("       • Text element is being added to a block instead");
            eprintln!("       • Text conversion is failing (empty content?)");
            eprintln!("       • Text element is not being processed");
        } else {
            eprintln!("  └─ Modelspace Entities Detail:");
            for (i, entity) in drawing.modelspace_entities.iter().enumerate() {
                eprintln!("      Modelspace Entity {}:", i);
                eprintln!("        • Type: '{}'", entity.entity_type());
                eprintln!("        • Layer: '{}'", entity.layer());

                match entity.entity_type() {
                    "TEXT" => {
                        eprintln!("        • Entity type: TEXT");
                        eprintln!("        • Expected layer: 'Text/Annotations'");
                    }
                    "LINE" => {
                        eprintln!("        • Entity type: LINE");
                        eprintln!("        • Note: LINE in modelspace (not in a block)");
                    }
                    "CIRCLE" => {
                        eprintln!("        • Entity type: CIRCLE");
                        eprintln!("        • Note: CIRCLE in modelspace (not in a block)");
                    }
                    _ => {
                        eprintln!(
                            "        • Entity type: {} (unexpected in modelspace)",
                            entity.entity_type()
                        );
                    }
                }
            }
            eprintln!();
        }

        eprintln!("[STEP 6] Running Assertions");
        eprintln!();

        // Check layer assignments.
        eprintln!("  [ASSERTION 1] Block Count Check");
        eprintln!("    Expected: 2 blocks");
        eprintln!("    Actual: {} blocks", drawing.blocks.len());
        if drawing.blocks.len() != 2 {
            eprintln!("    ❌ FAILED");
            eprintln!(
                "    Block names found: {:?}",
                drawing.blocks.iter().map(|b| &b.name).collect::<Vec<_>>()
            );
            eprintln!("    Analysis:");
            eprintln!("      • This suggests pattern piece extraction is not working correctly");
            eprintln!("      • Groups with IDs should become blocks when create_blocks=true");
        } else {
            eprintln!("    ✅ PASSED");
        }
        assert_eq!(
            drawing.blocks.len(),
            2,
            "\n═══════════════════════════════════════════════════════════════\n\
             ❌ ASSERTION FAILED: Block Count\n\
             ═══════════════════════════════════════════════════════════════\n\
             Expected: 2 blocks (from groups 'cutline_piece1' and 'notch_mark1')\n\
             Found: {} blocks\n\
             Block names found: {:?}\n\
             \n\
             Possible causes:\n\
             • Pattern piece extraction logic is not finding <g> elements\n\
             • <g> elements don't have 'id' attributes\n\
             • create_blocks option is not being respected\n\
             ════════════════════════════════════════════════════════════════",
            drawing.blocks.len(),
            drawing.blocks.iter().map(|b| &b.name).collect::<Vec<_>>()
        );
        eprintln!();

        eprintln!("  [ASSERTION 2] Modelspace Entity Count Check");
        eprintln!("    Expected: 1 modelspace entity (text element 'Label')");
        eprintln!(
            "    Actual: {} modelspace entities",
            drawing.modelspace_entities.len()
        );
        if drawing.modelspace_entities.len() != 1 {
            eprintln!("    ❌ FAILED");
            eprintln!("    Analysis:");
            eprintln!("      • Blocks: {}", drawing.blocks.len());
            eprintln!(
                "      • Modelspace entities: {}",
                drawing.modelspace_entities.len()
            );
            eprintln!(
                "      • This suggests the text element is not being converted or is going to a block instead"
            );
        } else {
            eprintln!("    ✅ PASSED");
        }
        assert_eq!(
            drawing.modelspace_entities.len(),
            1,
            "\n═══════════════════════════════════════════════════════════════\n\
             ❌ ASSERTION FAILED: Modelspace Entity Count\n\
             ═══════════════════════════════════════════════════════════════\n\
             Expected: 1 modelspace entity (text element 'Label')\n\
             Found: {} modelspace entities\n\
             Blocks: {}, Modelspace entities: {}\n\
             \n\
             Possible causes:\n\
             • Text element is being added to a block instead of modelspace\n\
             • Text conversion is failing (empty content?)\n\
             • Text element is not being processed\n\
             ════════════════════════════════════════════════════════════════",
            drawing.modelspace_entities.len(),
            drawing.blocks.len(),
            drawing.modelspace_entities.len()
        );
        eprintln!();

        // Verify blocks have entities
        eprintln!("  [ASSERTION 3] Block 0 Entity Count Check");
        if drawing.blocks.len() > 0 {
            eprintln!("    Block 0 name: '{}'", drawing.blocks[0].name);
            eprintln!("    Expected: At least 1 entity (line from cutline_piece1)");
            eprintln!("    Actual: {} entities", drawing.blocks[0].entities.len());
            if drawing.blocks[0].entities.len() == 0 {
                eprintln!("    ❌ FAILED");
                eprintln!("    Analysis:");
                eprintln!("      • Block exists but has no entities");
                eprintln!("      • This suggests entities are not being added to blocks correctly");
            } else {
                eprintln!("    ✅ PASSED");
            }
        } else {
            eprintln!("    ⚠️  SKIPPED (no blocks exist)");
        }
        assert!(
            drawing.blocks.len() > 0 && drawing.blocks[0].entities.len() > 0,
            "\n═══════════════════════════════════════════════════════════════\n\
             ❌ ASSERTION FAILED: Block 0 Entity Count\n\
             ═══════════════════════════════════════════════════════════════\n\
             Block 0 '{}' should have at least 1 entity (line from cutline_piece1)\n\
             Found: {} entities\n\
             \n\
             Possible causes:\n\
             • Entities are not being added to blocks correctly\n\
             • Entity conversion is failing silently\n\
             • Entities are going to modelspace instead\n\
             ════════════════════════════════════════════════════════════════",
            if drawing.blocks.len() > 0 {
                &drawing.blocks[0].name
            } else {
                "<no block>"
            },
            if drawing.blocks.len() > 0 {
                drawing.blocks[0].entities.len()
            } else {
                0
            }
        );
        eprintln!();

        eprintln!("  [ASSERTION 4] Block 1 Entity Count Check");
        if drawing.blocks.len() > 1 {
            eprintln!("    Block 1 name: '{}'", drawing.blocks[1].name);
            eprintln!("    Expected: At least 1 entity (circle from notch_mark1)");
            eprintln!("    Actual: {} entities", drawing.blocks[1].entities.len());
            if drawing.blocks[1].entities.len() == 0 {
                eprintln!("    ❌ FAILED");
                eprintln!("    Analysis:");
                eprintln!("      • Block exists but has no entities");
                eprintln!("      • This suggests entities are not being added to blocks correctly");
            } else {
                eprintln!("    ✅ PASSED");
            }
        } else {
            eprintln!("    ⚠️  SKIPPED (block 1 does not exist)");
        }
        assert!(
            drawing.blocks.len() > 1 && drawing.blocks[1].entities.len() > 0,
            "\n═══════════════════════════════════════════════════════════════\n\
             ❌ ASSERTION FAILED: Block 1 Entity Count\n\
             ═══════════════════════════════════════════════════════════════\n\
             Block 1 '{}' should have at least 1 entity (circle from notch_mark1)\n\
             Found: {} entities\n\
             \n\
             Possible causes:\n\
             • Entities are not being added to blocks correctly\n\
             • Entity conversion is failing silently\n\
             • Entities are going to modelspace instead\n\
             ════════════════════════════════════════════════════════════════",
            if drawing.blocks.len() > 1 {
                &drawing.blocks[1].name
            } else {
                "<no block>"
            },
            if drawing.blocks.len() > 1 {
                drawing.blocks[1].entities.len()
            } else {
                0
            }
        );
        eprintln!();

        // Verify layer assignments from parent group IDs.
        eprintln!("  [ASSERTION 5] Block 0 Layer Assignment Check");
        if drawing.blocks.len() > 0 && drawing.blocks[0].entities.len() > 0 {
            let block0_layer = drawing.blocks[0].entities[0].layer();
            eprintln!("    Block 0 name: '{}'", drawing.blocks[0].name);
            eprintln!("    Block 0 source: Group 'cutline_piece1'");
            eprintln!("    Expected layer: '1' (Piece boundary)");
            eprintln!("    Actual layer: '{}'", block0_layer);
            eprintln!(
                "    Entity type: '{}'",
                drawing.blocks[0].entities[0].entity_type()
            );

            if block0_layer != "1" {
                eprintln!("    ❌ FAILED");
                eprintln!("    Analysis:");
                eprintln!("      • Group ID 'cutline_piece1' contains 'cutline'");
                eprintln!("      • Should map to layer '1' (Piece boundary)");
                eprintln!(
                    "      • This suggests layer mapping from parent group ID is not working"
                );
                eprintln!("      • Check: convert_element_tree() parent_layer parameter");
                eprintln!("      • Check: Layer inheritance logic in group processing");
            } else {
                eprintln!("    ✅ PASSED");
            }

            assert_eq!(
                block0_layer,
                "1",
                "\n═══════════════════════════════════════════════════════════════\n\
                 ❌ ASSERTION FAILED: Block 0 Layer Assignment\n\
                 ═══════════════════════════════════════════════════════════════\n\
                 Block 0 (from group 'cutline_piece1') entity should be on layer '1' (Piece boundary)\n\
                 Found layer: '{}'\n\
                 Block name: '{}'\n\
                 Entity type: '{}'\n\
                 \n\
                 Analysis:\n\
                 • Group ID 'cutline_piece1' contains 'cutline'\n\
                 • Should map to ASTM layer '1' (Piece boundary)\n\
                 • This suggests layer mapping from parent group ID is not working\n\
                 • Check: convert_element_tree() parent_layer parameter\n\
                 • Check: Layer inheritance logic in group processing\n\
                 ════════════════════════════════════════════════════════════════",
                block0_layer,
                drawing.blocks[0].name,
                drawing.blocks[0].entities[0].entity_type()
            );
        } else {
            eprintln!("    ⚠️  SKIPPED (block 0 or its entities don't exist)");
        }
        eprintln!();

        eprintln!("  [ASSERTION 6] Block 1 Layer Assignment Check");
        if drawing.blocks.len() > 1 && drawing.blocks[1].entities.len() > 0 {
            let block1_layer = drawing.blocks[1].entities[0].layer();
            eprintln!("    Block 1 name: '{}'", drawing.blocks[1].name);
            eprintln!("    Block 1 source: Group 'notch_mark1'");
            eprintln!("    Expected layer: '4' (Notches)");
            eprintln!("    Actual layer: '{}'", block1_layer);
            eprintln!(
                "    Entity type: '{}'",
                drawing.blocks[1].entities[0].entity_type()
            );

            if block1_layer != "4" {
                eprintln!("    ❌ FAILED");
                eprintln!("    Analysis:");
                eprintln!("      • Group ID 'notch_mark1' contains 'notch'");
                eprintln!("      • Should map to ASTM layer '4' (Notches)");
                eprintln!(
                    "      • This suggests layer mapping from parent group ID is not working"
                );
                eprintln!("      • Check: convert_element_tree() parent_layer parameter");
                eprintln!("      • Check: Layer inheritance logic in group processing");
            } else {
                eprintln!("    ✅ PASSED");
            }

            assert_eq!(
                block1_layer,
                "4",
                "\n═══════════════════════════════════════════════════════════════\n\
                 ❌ ASSERTION FAILED: Block 1 Layer Assignment\n\
                 ═══════════════════════════════════════════════════════════════\n\
                 Block 1 (from group 'notch_mark1') entity should be on layer '4' (Notches)\n\
                 Found layer: '{}'\n\
                 Block name: '{}'\n\
                 Entity type: '{}'\n\
                 \n\
                 Analysis:\n\
                 • Group ID 'notch_mark1' contains 'notch'\n\
                 • Should map to ASTM layer '4' (Notches)\n\
                 • This suggests layer mapping from parent group ID is not working\n\
                 • Check: convert_element_tree() parent_layer parameter\n\
                 • Check: Layer inheritance logic in group processing\n\
                 ════════════════════════════════════════════════════════════════",
                block1_layer,
                drawing.blocks[1].name,
                drawing.blocks[1].entities[0].entity_type()
            );
        } else {
            eprintln!("    ⚠️  SKIPPED (block 1 or its entities don't exist)");
        }
        eprintln!();

        // Verify text entity layer
        eprintln!("  [ASSERTION 7] Text Entity Layer Assignment Check");
        if drawing.modelspace_entities.len() > 0 {
            let text_layer = drawing.modelspace_entities[0].layer();
            let text_type = drawing.modelspace_entities[0].entity_type();
            eprintln!("    Entity type: '{}'", text_type);
            eprintln!("    Expected layer: '9' (Text/Annotations)");
            eprintln!("    Actual layer: '{}'", text_layer);

            if text_type != "TEXT" {
                eprintln!("    ⚠️  WARNING: First modelspace entity is not TEXT type!");
                eprintln!("       This might indicate the text element wasn't converted correctly");
            }

            if text_layer != "9" {
                eprintln!("    ❌ FAILED");
                eprintln!("    Analysis:");
                eprintln!("      • Text elements should map to ASTM layer '9' (Text/Annotations)");
                eprintln!("      • This suggests text element layer mapping is not working");
                eprintln!("      • Check: convert_text() function layer assignment");
                eprintln!("      • Check: map_svg_to_astm_layer() for text elements");
            } else {
                eprintln!("    ✅ PASSED");
            }

            assert_eq!(
                text_layer, "9",
                "\n═══════════════════════════════════════════════════════════════\n\
                 ❌ ASSERTION FAILED: Text Entity Layer Assignment\n\
                 ═══════════════════════════════════════════════════════════════\n\
                 Text entity should be on ASTM layer '9' (Text/Annotations)\n\
                 Found layer: '{}'\n\
                 Entity type: '{}'\n\
                 \n\
                 Analysis:\n\
                 • Text elements should map to ASTM layer '9'\n\
                 • This suggests text element layer mapping is not working\n\
                 • Check: convert_text() function layer assignment\n\
                 • Check: map_svg_to_astm_layer() for text elements\n\
                 • Check: convert_element_tree() text handling\n\
                 ════════════════════════════════════════════════════════════════",
                text_layer, text_type
            );
        } else {
            eprintln!("    ⚠️  SKIPPED (no modelspace entities exist)");
        }
        eprintln!();

        eprintln!("═══════════════════════════════════════════════════════════════");
        eprintln!("  test_layer_mapping: ALL ASSERTIONS PASSED ✅");
        eprintln!("═══════════════════════════════════════════════════════════════");
        eprintln!();
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
            "9",
            "Expected ASTM layer '9' (Text/Annotations), found '{}'",
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
