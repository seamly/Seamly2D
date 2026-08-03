// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Tests for DXF-ASTM writer.

#[cfg(test)]
mod tests {
    use crate::encoder::{encode_circle, encode_entity, encode_line, encode_polyline, encode_text};
    use crate::validator::validate_astm_compliance;
    use crate::writer::{DxfAstmExportOptions, ProgressCallback, export_dxf_astm};
    use seamly_svg2ezdxf::{Block, Circle, Drawing, DxfVersion, Line, Point, Polyline, Text};
    use std::fs;
    use std::sync::{Arc, Mutex};

    // @brief Test encoding a LINE entity.
    #[test]
    fn test_encode_line() {
        let line = Line {
            layer: "Piece boundary".to_string(),
            start: Point::new(10.0, 20.0),
            end: Point::new(50.0, 60.0),
        };

        let mut buffer = Vec::new();
        encode_line(&mut buffer, &line).expect("Failed to encode LINE");

        let output = String::from_utf8(buffer).expect("Invalid UTF-8");
        assert!(output.contains("LINE"));
        assert!(output.contains("Piece boundary"));
        assert!(output.contains("10.000000")); // X coordinate
        assert!(output.contains("20.000000")); // Y coordinate
    }

    // @brief Test encoding a CIRCLE entity.
    #[test]
    fn test_encode_circle() {
        let circle = Circle {
            layer: "Notches".to_string(),
            center: Point::new(100.0, 100.0),
            radius: 25.0,
        };

        let mut buffer = Vec::new();
        encode_circle(&mut buffer, &circle).expect("Failed to encode CIRCLE");

        let output = String::from_utf8(buffer).expect("Invalid UTF-8");
        assert!(output.contains("CIRCLE"));
        assert!(output.contains("Notches"));
        assert!(output.contains("100.000000")); // Center X
        assert!(output.contains("25.000000")); // Radius
    }

    // @brief Test encoding a POLYLINE entity.
    #[test]
    fn test_encode_polyline() {
        let polyline = Polyline {
            layer: "Sew lines".to_string(),
            vertices: vec![
                Point::new(0.0, 0.0),
                Point::new(10.0, 0.0),
                Point::new(10.0, 10.0),
                Point::new(0.0, 10.0),
            ],
            closed: true,
        };

        let mut buffer = Vec::new();
        encode_polyline(&mut buffer, &polyline).expect("Failed to encode POLYLINE");

        let output = String::from_utf8(buffer).expect("Invalid UTF-8");
        assert!(output.contains("POLYLINE"));
        assert!(output.contains("Sew lines"));
        assert!(output.contains("VERTEX"));
        assert!(output.contains("SEQEND"));
    }

    // @brief Test encoding a TEXT entity.
    #[test]
    fn test_encode_text() {
        let text = Text {
            layer: "Text/Annotations".to_string(),
            insertion_point: Point::new(50.0, 50.0),
            height: 12.0,
            rotation: 0.0,
            content: "Test Label".to_string(),
        };

        let mut buffer = Vec::new();
        encode_text(&mut buffer, &text).expect("Failed to encode TEXT");

        let output = String::from_utf8(buffer).expect("Invalid UTF-8");
        assert!(output.contains("TEXT"));
        assert!(output.contains("Text/Annotations"));
        assert!(output.contains("Test Label"));
        assert!(output.contains("12.000000")); // Height
    }

    // @brief Test encoding a generic entity via encode_entity function.
    #[test]
    fn test_encode_entity() {
        let line = Line {
            layer: "Internal lines".to_string(),
            start: Point::new(5.0, 5.0),
            end: Point::new(15.0, 15.0),
        };

        let entity: Box<dyn seamly_svg2ezdxf::Entity> = Box::new(line);
        let mut buffer = Vec::new();
        encode_entity(&mut buffer, &entity).expect("Failed to encode entity");

        let output = String::from_utf8(buffer).expect("Invalid UTF-8");
        assert!(output.contains("LINE"));
    }

    // @brief Test writing a complete DXF file.
    #[test]
    fn test_export_dxf_astm() {
        // Create a simple drawing.
        let mut drawing = Drawing::new(DxfVersion::R12);

        // Add a block.
        let mut block = Block::new("TestPiece".to_string());
        let line = Line {
            layer: "Piece boundary".to_string(),
            start: Point::new(0.0, 0.0),
            end: Point::new(10.0, 10.0),
        };
        block.add_entity(Box::new(line));
        drawing.add_block(block);

        // Add a modelspace entity.
        let circle = Circle {
            layer: "Notches".to_string(),
            center: Point::new(50.0, 50.0),
            radius: 5.0,
        };
        drawing.add_modelspace_entity(Box::new(circle));

        // Create output directory if it doesn't exist.
        std::fs::create_dir_all("output").expect("Failed to create output directory");

        // Export to temporary file.
        let test_file = "output/test_export.dxf";
        let options = DxfAstmExportOptions::default();

        export_dxf_astm(&drawing, test_file, &options).expect("Failed to export DXF");

        // Verify file was created.
        assert!(
            std::path::Path::new(test_file).exists(),
            "DXF file should exist"
        );

        // Read and verify file contents.
        let contents = fs::read_to_string(test_file).expect("Failed to read DXF file");
        assert!(contents.contains("SECTION"));
        assert!(contents.contains("HEADER"));
        assert!(contents.contains("BLOCKS"));
        assert!(contents.contains("ENTITIES"));
        assert!(contents.contains("EOF"));
        assert!(contents.contains("TestPiece"));
        assert!(contents.contains("LINE"));
        assert!(contents.contains("CIRCLE"));

        // Clean up.
        let _ = fs::remove_file(test_file);
    }

    // @brief Test teaching version progress callback.
    #[test]
    fn test_teaching_version_progress_callback() {
        // Create a simple drawing.
        let mut drawing = Drawing::new(DxfVersion::R12);

        let mut block = Block::new("TestPiece".to_string());
        let line = Line {
            layer: "Piece boundary".to_string(),
            start: Point::new(0.0, 0.0),
            end: Point::new(10.0, 10.0),
        };
        block.add_entity(Box::new(line));
        drawing.add_block(block);

        let circle = Circle {
            layer: "Notches".to_string(),
            center: Point::new(50.0, 50.0),
            radius: 5.0,
        };
        drawing.add_modelspace_entity(Box::new(circle));

        std::fs::create_dir_all("output").expect("Failed to create output directory");

        let test_file = "output/test_teaching_progress.dxf";
        let teaching_file = "output/test_teaching_progress.txt";

        let progress_values: Arc<Mutex<Vec<f32>>> = Arc::new(Mutex::new(Vec::new()));
        let progress_capture = progress_values.clone();
        let callback: ProgressCallback = Arc::new(move |p: f32| {
            if let Ok(mut values) = progress_capture.lock() {
                values.push(p);
            }
        });

        let options = DxfAstmExportOptions {
            include_header: false,
            validate_entities: true,
            sanitize_text: true,
            create_teaching_version: true,
            progress_callback: Some(callback),
        };

        export_dxf_astm(&drawing, test_file, &options)
            .expect("Failed to export DXF teaching version");

        assert!(
            std::path::Path::new(test_file).exists(),
            "DXF file should exist"
        );
        assert!(
            std::path::Path::new(teaching_file).exists(),
            "Teaching file should exist"
        );

        let values = progress_values
            .lock()
            .expect("Failed to lock progress values");
        assert!(!values.is_empty(), "Progress callback should be called");
        for window in values.windows(2) {
            assert!(window[1] >= window[0], "Progress should be monotonic");
        }
        let last = *values.last().unwrap_or(&0.0);
        assert!((last - 1.0).abs() < 0.001, "Final progress should be 1.0");

        let _ = fs::remove_file(test_file);
        let _ = fs::remove_file(teaching_file);
    }

    // @brief Test ASTM validation.
    #[test]
    fn test_validate_astm_compliance() {
        // Create a valid drawing (R12).
        let drawing = Drawing::new(DxfVersion::R12);
        let result = validate_astm_compliance(&drawing);
        assert!(result.is_ok(), "Valid R12 drawing should pass validation");

        // Create an invalid drawing (R13).
        let drawing_r13 = Drawing::new(DxfVersion::R13);
        let result_r13 = validate_astm_compliance(&drawing_r13);
        assert!(result_r13.is_err(), "R13 drawing should fail validation");
    }

    // @brief Test DXF file structure (HEADER, BLOCKS, ENTITIES, EOF).
    #[test]
    fn test_dxf_file_structure() {
        // Create output directory if it doesn't exist.
        std::fs::create_dir_all("output").expect("Failed to create output directory");

        let drawing = Drawing::new(DxfVersion::R12);
        let test_file = "output/test_structure.dxf";
        let options = DxfAstmExportOptions::default();

        export_dxf_astm(&drawing, test_file, &options).expect("Failed to export DXF");

        let contents = fs::read_to_string(test_file).expect("Failed to read DXF file");

        // Verify file structure.
        let sections: Vec<&str> = contents.lines().collect();
        let mut found_header = false;
        let mut found_blocks = false;
        let mut found_entities = false;
        let mut found_eof = false;

        for line in sections {
            if line == "HEADER" {
                found_header = true;
            } else if line == "BLOCKS" {
                found_blocks = true;
            } else if line == "ENTITIES" {
                found_entities = true;
            } else if line == "EOF" {
                found_eof = true;
            }
        }

        assert!(found_header, "File should contain HEADER section");
        assert!(found_blocks, "File should contain BLOCKS section");
        assert!(found_entities, "File should contain ENTITIES section");
        assert!(found_eof, "File should contain EOF marker");

        // Clean up.
        let _ = fs::remove_file(test_file);
    }

    // @brief Test export with multiple blocks.
    #[test]
    fn test_export_multiple_blocks() {
        let mut drawing = Drawing::new(DxfVersion::R12);

        // Add first block.
        let mut block1 = Block::new("Piece1".to_string());
        let line1 = Line {
            layer: "Piece boundary".to_string(),
            start: Point::new(0.0, 0.0),
            end: Point::new(10.0, 0.0),
        };
        block1.add_entity(Box::new(line1));
        drawing.add_block(block1);

        // Add second block.
        let mut block2 = Block::new("Piece2".to_string());
        let line2 = Line {
            layer: "Piece boundary".to_string(),
            start: Point::new(0.0, 0.0),
            end: Point::new(20.0, 0.0),
        };
        block2.add_entity(Box::new(line2));
        drawing.add_block(block2);

        // Create output directory if it doesn't exist.
        std::fs::create_dir_all("output").expect("Failed to create output directory");

        let test_file = "output/test_multiple_blocks.dxf";
        let options = DxfAstmExportOptions::default();

        export_dxf_astm(&drawing, test_file, &options).expect("Failed to export DXF");

        let contents = fs::read_to_string(test_file).expect("Failed to read DXF file");
        assert!(contents.contains("Piece1"));
        assert!(contents.contains("Piece2"));

        // Verify that INSERT entities are written to ENTITIES section.
        // Blocks must be inserted into ENTITIES section to be visible.
        let lines: Vec<&str> = contents.lines().collect();
        let mut in_entities_section = false;
        let mut found_insert_piece1 = false;
        let mut found_insert_piece2 = false;

        for (i, line) in lines.iter().enumerate() {
            if *line == "ENTITIES" && i > 0 && lines[i - 1] == "2" {
                in_entities_section = true;
            } else if in_entities_section && *line == "ENDSEC" {
                break;
            } else if in_entities_section && *line == "INSERT" {
                // INSERT entity format: 0, INSERT, 8, layer, 2, block_name, ...
                // Check if block name (after group code 2) is Piece1 or Piece2.
                if i + 4 < lines.len() && lines[i + 1] == "8" && lines[i + 3] == "2" {
                    let block_name = lines[i + 4];
                    if block_name == "Piece1" {
                        found_insert_piece1 = true;
                    } else if block_name == "Piece2" {
                        found_insert_piece2 = true;
                    }
                }
            }
        }

        assert!(
            found_insert_piece1,
            "ENTITIES section should contain INSERT for Piece1"
        );
        assert!(
            found_insert_piece2,
            "ENTITIES section should contain INSERT for Piece2"
        );

        // Clean up.
        let _ = fs::remove_file(test_file);
    }
}
