<!-- MIT License: https://opensource.org/licenses/MIT -->
# Testing seamly_svg2ezdxf Functionality

This document describes how to test if `seamly_svg2ezdxf` outputs a proper ezdxf-like Drawing object.

## Overview

The `seamly_svg2ezdxf` crate converts SVG DOM documents into an intermediate representation (Drawing object) that mimics Python's ezdxf Drawing structure. To verify it works correctly, we need to test:

1. **Entity Conversion**: SVG elements → DXF entities
2. **Pattern Piece Extraction**: SVG groups → DXF blocks
3. **Coordinate Transformation**: Y-axis inversion (SVG → DXF)
4. **Layer Mapping**: SVG groups → ASTM layer names
5. **Drawing Structure**: Proper block and entity organization

## Testing Approaches

### 1. Unit Tests (Recommended First Step)

Create unit tests in `crates/seamly_svg2ezdxf/src/converter_test.rs` or add `#[cfg(test)]` modules to existing files.

**Example Test Structure:**

```rust
#[cfg(test)]
mod tests {
    use crate::converter::{svg_to_ezdxf, SvgToEzdxfOptions};
    use crate::drawing::{Drawing, DxfVersion};
    use crate::entities::{Entity, Line, Circle, Text};
    use svg_dom::Document;

    #[test]
    fn test_convert_simple_line() {
        // Create simple SVG with a line
        let svg = r#"
            <svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
                <line x1="10" y1="20" x2="50" y2="60"/>
            </svg>
        "#;
        
        let doc = Document::parse(svg).expect("Parse SVG");
        let options = SvgToEzdxfOptions {
            create_blocks: false,
            invert_y: true,
            svg_height: Some(100.0),
            ..Default::default()
        };
        
        let drawing = svg_to_ezdxf(&doc, &options).expect("Convert to Drawing");
        
        // Verify Drawing structure
        assert_eq!(drawing.version, DxfVersion::R12);
        assert_eq!(drawing.modelspace_entities.len(), 1);
        
        // Verify entity type and properties
        let entity = &drawing.modelspace_entities[0];
        assert_eq!(entity.entity_type(), "LINE");
        assert_eq!(entity.layer(), "Internal lines");
    }
}
```

**Run tests:**
```bash
cargo test -p seamly_svg2ezdxf
```

### 2. Integration Tests with Real SVG Files

Test with actual SVG files from your `input/` directory:

```rust
#[test]
fn test_convert_real_svg_file() {
    use std::fs;
    use std::path::Path;
    
    let svg_path = Path::new("input/richmond-shirt_v1_v061-02.svg");
    let svg_content = fs::read_to_string(svg_path).expect("Read SVG file");
    let doc = Document::parse(&svg_content).expect("Parse SVG");
    
    let options = SvgToEzdxfOptions::default();
    let drawing = svg_to_ezdxf(&doc, &options).expect("Convert to Drawing");
    
    // Verify conversion succeeded
    assert!(drawing.blocks.len() > 0, "Should have pattern pieces");
    
    // Verify each block has entities
    for block in &drawing.blocks {
        assert!(block.entities.len() > 0, 
                "Block '{}' should have entities", block.name);
    }
}
```

### 3. Manual Inspection Tests

Create a simple test program that prints the Drawing structure:

```rust
// In a test or example program
fn inspect_drawing(drawing: &Drawing) {
    println!("DXF Version: {:?}", drawing.version);
    println!("Blocks: {}", drawing.blocks.len());
    println!("Modelspace Entities: {}", drawing.modelspace_entities.len());
    
    for (i, block) in drawing.blocks.iter().enumerate() {
        println!("  Block {}: '{}' ({} entities)", 
                 i, block.name, block.entities.len());
        for (j, entity) in block.entities.iter().enumerate() {
            println!("    Entity {}: {} on layer '{}'", 
                     j, entity.entity_type(), entity.layer());
        }
    }
    
    for (i, entity) in drawing.modelspace_entities.iter().enumerate() {
        println!("  Modelspace Entity {}: {} on layer '{}'", 
                 i, entity.entity_type(), entity.layer());
    }
}
```

### 4. Round-Trip Validation (Future)

Once `ezdxf2dxfastm` is implemented, test the full pipeline:

```rust
#[test]
fn test_full_pipeline() {
    // SVG → ezdxf Drawing
    let drawing = svg_to_ezdxf(&doc, &options)?;
    
    // ezdxf Drawing → DXF-ASTM file
    export_dxf_astm(&drawing, "test_output.dxf", &export_options)?;
    
    // Verify DXF file was created and is valid
    assert!(Path::new("test_output.dxf").exists());
    
    // Optionally: Use dxf2svg or ezdxf to read back and verify
}
```

## What to Verify

### Drawing Structure
- ✅ `drawing.version` is correct (R12 for ASTM)
- ✅ `drawing.blocks` contains pattern pieces (if `create_blocks: true`)
- ✅ `drawing.modelspace_entities` contains entities (if `create_blocks: false`)

### Entity Properties
- ✅ Entity type is correct (LINE, CIRCLE, TEXT, etc.)
- ✅ Layer name matches ASTM layer mapping
- ✅ Coordinates are correct (especially Y-axis inversion)

### Pattern Pieces
- ✅ Each top-level `<g>` with `id` becomes a Block
- ✅ Block name is sanitized (ASCII-only, valid DXF name)
- ✅ Entities within pattern piece are in the block

### Coordinate Transformation
- ✅ Y-axis is inverted when `invert_y: true`
- ✅ Formula: `dxf_y = svg_height - svg_y`
- ✅ X-axis remains unchanged

### Layer Mapping
- ✅ Elements with `id` containing "cutline" → "Piece boundary"
- ✅ Elements with `id` containing "notch" → "Notches"
- ✅ Elements with `id` containing "grainline" → "Grain line"
- ✅ `<text>` elements → "Text/Annotations"
- ✅ Default → "Internal lines"

## Test Cases to Implement

1. **Basic Entity Conversion**
   - [ ] SVG `<line>` → DXF LINE
   - [ ] SVG `<circle>` → DXF CIRCLE
   - [ ] SVG `<text>` → DXF TEXT

2. **Pattern Piece Extraction**
   - [ ] Single pattern piece → single block
   - [ ] Multiple pattern pieces → multiple blocks
   - [ ] Pattern piece without ID → not converted to block

3. **Coordinate Transformation**
   - [ ] Y-axis inversion with known SVG height
   - [ ] Y-axis inversion with SVG height from attributes
   - [ ] No inversion when `invert_y: false`

4. **Layer Mapping**
   - [ ] Cutline elements → "Piece boundary"
   - [ ] Notch elements → "Notches"
   - [ ] Text elements → "Text/Annotations"
   - [ ] Default elements → "Internal lines"

5. **Edge Cases**
   - [ ] Empty SVG → empty Drawing
   - [ ] SVG with no pattern pieces → modelspace entities only
   - [ ] Invalid coordinates → handled gracefully
   - [ ] Non-ASCII text → sanitized to ASCII

## Running Tests

```bash
# Run all tests
cargo test -p seamly_svg2ezdxf

# Run specific test
cargo test -p seamly_svg2ezdxf test_convert_simple_line

# Run with output
cargo test -p seamly_svg2ezdxf -- --nocapture

# Run tests and show documentation
cargo test -p seamly_svg2ezdxf --doc
```

## Debugging Tips

1. **Print Drawing Structure**: Use `inspect_drawing()` function to see what was converted
2. **Check Entity Counts**: Verify expected number of entities per block
3. **Verify Coordinates**: Print entity coordinates to check Y-axis inversion
4. **Check Layer Names**: Verify layer mapping is working correctly
5. **Test Incrementally**: Start with simple SVGs, then test complex ones

## Next Steps

Once basic entity conversion tests pass:
1. Implement path conversion (SVG `<path>` → DXF POLYLINE)
2. Test with real pattern files from `input/` directory
3. Integrate with `ezdxf2dxfastm` for full pipeline testing
4. Validate output with DXF parsers (LibreCAD, QCAD)
