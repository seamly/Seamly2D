// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Example program to test DXF-ASTM export.

use ezdxf2dxfastm::{DxfAstmExportOptions, export_dxf_astm};
use seamly_svg2ezdxf::{Block, Circle, Drawing, DxfVersion, Line, Point};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("=== Testing DXF-ASTM Export ===");

    // Create a simple drawing.
    let mut drawing = Drawing::new(DxfVersion::R12);

    // Add a block with a line.
    let mut block = Block::new("TestPiece".to_string());
    let line = Line {
        layer: "Piece boundary".to_string(),
        start: Point::new(0.0, 0.0),
        end: Point::new(100.0, 100.0),
    };
    block.add_entity(Box::new(line));
    drawing.add_block(block);

    // Add a modelspace circle.
    let circle = Circle {
        layer: "Notches".to_string(),
        center: Point::new(50.0, 50.0),
        radius: 10.0,
    };
    drawing.add_modelspace_entity(Box::new(circle));

    // Export to DXF file.
    let output_file = "output/example_export.dxf";
    std::fs::create_dir_all("output")?;

    let options = DxfAstmExportOptions::default();
    export_dxf_astm(&drawing, output_file, &options)?;

    println!("✅ DXF file exported successfully to: {}", output_file);
    println!("\nFile contents (first 50 lines):");
    println!("---");

    let contents = std::fs::read_to_string(output_file)?;
    for (i, line) in contents.lines().take(50).enumerate() {
        println!("{:3}: {}", i + 1, line);
    }
    if contents.lines().count() > 50 {
        println!("... ({} more lines)", contents.lines().count() - 50);
    }

    Ok(())
}
