// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Example: Convert SVG to ezdxf Drawing and write to output directory for inspection.
//! @details This example demonstrates how to:
//!          1. Parse an SVG file
//!          2. Convert it to an ezdxf Drawing
//!          3. Write the Drawing to the output directory for inspection

use seamly_svg2ezdxf::{SvgToEzdxfOptions, svg_to_ezdxf, write_drawing_to_output};
use std::fs;
use std::path::Path;
use svg_dom::Document;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("=== SeamlyLayout ezdxf Drawing Inspector ===\n");

    // Example 1: Simple SVG with basic entities
    println!("Example 1: Converting simple SVG with line, circle, text, and path");
    let svg1 = r#"
        <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
            <g id="cutline_piece1">
                <line x1="0" y1="0" x2="10" y2="10"/>
            </g>
            <g id="notch_mark1">
                <circle cx="20" cy="20" r="5"/>
            </g>
            <path d="M30 30 L40 30 L40 40 L30 40 Z"/>
            <text x="50" y="50">Label</text>
        </svg>
    "#;

    let doc1 = Document::parse(svg1)?;
    let options1 = SvgToEzdxfOptions {
        create_blocks: true,
        invert_y: false,
        ..Default::default()
    };

    let drawing1 = svg_to_ezdxf(&doc1, &options1)?;
    let file_path1 = write_drawing_to_output(&drawing1, Some("example1_ezdxf.txt"))?;
    println!("  ✓ Drawing written to: {:?}\n", file_path1);

    // Example 2: Real SVG file (if available)
    let test_svg_path = Path::new("../../input/richmond-shirt_v1_v061-02.svg");
    if test_svg_path.exists() {
        println!("Example 2: Converting real SVG file");
        let svg_content = fs::read_to_string(test_svg_path)?;
        let doc2 = Document::parse(&svg_content)?;

        let options2 = SvgToEzdxfOptions {
            create_blocks: true,
            invert_y: true,
            svg_height: Some(1000.0), // Approximate height
            flatten_tolerance: 0.1,
            ..Default::default()
        };

        let drawing2 = svg_to_ezdxf(&doc2, &options2)?;
        let file_path2 = write_drawing_to_output(&drawing2, Some("richmond_shirt_ezdxf.txt"))?;
        println!("  ✓ Drawing written to: {:?}", file_path2);
        println!(
            "  ✓ Blocks: {}, Modelspace entities: {}\n",
            drawing2.blocks.len(),
            drawing2.modelspace_entities.len()
        );
    } else {
        println!(
            "Example 2: Skipped (test SVG file not found at {:?})\n",
            test_svg_path
        );
    }

    println!("=== Inspection complete ===");
    println!("Check the 'output/' directory for the generated .txt files");

    Ok(())
}
