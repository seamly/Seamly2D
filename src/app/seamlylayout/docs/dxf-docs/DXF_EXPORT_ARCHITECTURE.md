<!-- MIT License: https://opensource.org/licenses/MIT -->
# DXF Export Architecture: Two-Stage Conversion

## Overview

This document outlines the two-stage conversion architecture for exporting SVG layout data to DXF-ASTM format. The architecture separates concerns into two reusable libraries:

1. **`seamly_svg2ezdxf`**: Converts SVG DOM to an ezdxf-like intermediate representation
2. **`ezdxf2dxfastm`**: Converts the intermediate representation to DXF-ASTM format

## Architecture Benefits

### Separation of Concerns
- **SVG Understanding**: `seamly_svg2ezdxf` focuses on parsing SVG and understanding geometry
- **DXF Formatting**: `ezdxf2dxfastm` focuses on DXF file structure and ASTM constraints

### Reusability
- `seamly_svg2ezdxf` can be used for other DXF export formats (not just ASTM)
- `ezdxf2dxfastm` can be used with other sources (not just SVG)
- Both libraries can be tested independently

### Extensibility
- Easy to add new DXF export formats (e.g., DXF R2000, DXF R2018)
- Easy to add new input formats (e.g., import from DXF, import from other CAD formats)
- Can add validation/transformation stages between the two

### Testability
- Test SVG → ezdxf conversion independently
- Test ezdxf → DXF-ASTM conversion independently
- Round-trip testing: SVG → ezdxf → DXF → ezdxf → SVG

## Workflow

```
SVG layout_dom (svg_dom::Document)
    ↓
[seamly_svg2ezdxf crate]
    ↓
ezdxf::Drawing (intermediate representation)
    ↓
[ezdxf2dxfastm crate]
    ↓
DXF-ASTM file (R12 format, constrained structure)
```

## Detailed Conversion Workflow: `seamly_svg2ezdxf`

### Step-by-Step Process

#### 1. Initialization (`svg_to_ezdxf`)

1. **Get Root Element**: Extract the `<svg>` root element from the document
2. **Determine SVG Height**:
   - If `invert_y: true`, get height from options or SVG root attributes (default: 100.0)
   - If `invert_y: false`, height is not needed
3. **Create Drawing Object**: Initialize with specified DXF version (default: R12)
4. **Choose Processing Mode**:
   - If `create_blocks: true` → Extract pattern pieces (blocks mode)
   - If `create_blocks: false` → Direct modelspace conversion

#### 2. Pattern Piece Extraction (`extract_pattern_pieces`) - When `create_blocks: true`

For each direct child of the `<svg>` root:

1. **Check Element Type**:
   - If `<g>` element with `id` attribute:
     - Sanitize ID to create block name (ASCII-only, valid DXF name)
     - Create new `Block` with sanitized name
     - Convert all elements within the group to entities (recursive)
     - Add block to drawing
   - If `<g>` element without `id`:
     - Convert group children directly to modelspace (no block created)
   - If other element (e.g., `<text>`, `<line>`, `<circle>`):
     - Convert directly to modelspace entity

2. **Result**:
   - Pattern pieces (groups with IDs) become `Block` definitions
   - Other elements go to `modelspace_entities`

#### 3. Direct Modelspace Conversion (`convert_elements_to_modelspace`) - When `create_blocks: false`

1. **Recursively Process Root**: Convert all elements directly to modelspace entities
2. **Result**: All entities go to `modelspace_entities` (no blocks created)

#### 4. Element Tree Conversion (`convert_element_tree`) - Core Recursive Function

For each element:

1. **Determine Layer**:
   - Check if parent layer is provided (from meaningful group)
   - If no parent layer, check element's own ID for layer hints:
     - ID contains "cutline" or "boundary" → "Piece boundary"
     - ID contains "notch" → "Notches"
     - ID contains "grainline" or "grain" → "Grain line"
     - ID contains "seamline" or "seam" → "Sew lines"
     - ID contains "drill" or "hole" → "Drill holes"
     - Otherwise → Use `map_svg_to_astm_layer()` for default mapping

2. **Convert Element Based on Type**:
   - **`<line>`**: Convert to `Line` entity
     - Parse `x1`, `y1`, `x2`, `y2` attributes
     - Apply Y-axis inversion if enabled
     - Override layer if parent layer provided
   - **`<circle>`**: Convert to `Circle` entity
     - Parse `cx`, `cy`, `r` attributes
     - Apply Y-axis inversion if enabled
     - Override layer if parent layer provided
   - **`<text>`**: Convert to `Text` entity
     - Parse `x`, `y`, `font-size` attributes
     - Extract text content from children
     - Sanitize to ASCII-only
     - **Always use "Text/Annotations" layer** (ignores parent layer)
     - Apply Y-axis inversion if enabled
   - **`<g>` (group)**: Process recursively
     - Check if group ID contains layer hints
     - If yes, pass layer to children as parent layer
     - If no, process children without parent layer
     - Recursively convert all children

3. **Process Children**:
   - **Root `<svg>` element**: Never passes layer to children (always `None`)
   - **Groups with layer hints**: Pass layer to children as parent layer
   - **Groups without layer hints**: Don't pass layer to children
   - **Other elements**: Don't pass layer to children

#### 5. Layer Inheritance Rules

- **Root `<svg>` element**: Does NOT pass layer to children
- **Groups with meaningful IDs**: Pass layer hints to children (e.g., "cutline_piece1" → "Piece boundary")
- **Groups without layer hints**: Don't pass layer to children
- **Text elements**: Always use "Text/Annotations" layer (never inherit)
- **Other elements**: Inherit from parent if parent provides meaningful layer

#### 6. Coordinate Transformation

When `invert_y: true`:
- Formula: `dxf_y = svg_height - svg_y`
- Applied to: Line start/end points, Circle center, Text insertion point
- X-axis: Unchanged

#### 7. Entity Conversion Details

**Line Conversion**:
- Attributes: `x1`, `y1`, `x2`, `y2`
- Default layer: "Internal lines" (if no parent layer)
- Layer override: Uses parent layer if provided

**Circle Conversion**:
- Attributes: `cx`, `cy`, `r`
- Validation: Radius must be > 0
- Default layer: "Internal lines" (if no parent layer)
- Layer override: Uses parent layer if provided

**Text Conversion**:
- Attributes: `x`, `y`, `font-size`
- Content: Extracted from text node children
- Sanitization: Non-ASCII characters removed
- Layer: Always "Text/Annotations" (never inherits)
- Validation: Empty content returns `None`

### Current Implementation Status

✅ **Implemented**:
- Basic entity conversions (Line, Circle, Text)
- Path conversion (`<path>` → Polyline) with curve flattening
- Polyline, Polygon, Rect, Ellipse conversions
- Pattern piece extraction to blocks
- Layer mapping with inheritance rules
- Coordinate transformation (Y-axis inversion)
- Text ASCII sanitization
- Block name sanitization
- Block insertion into ENTITIES section (INSERT entities)
- Teaching version generation with inline comments

⏳ **Pending**:
- Arc conversion (`<arc>` element)
- Transform attribute parsing (rotation, scale, translate)
- Style attribute parsing (stroke, fill)
- Group transform inheritance

## Crate 1: `seamly_svg2ezdxf`

### Purpose
Convert SVG DOM structure to an ezdxf-like intermediate representation (Rust equivalent of Python's `ezdxf.Drawing`).

### Responsibilities
1. Parse SVG DOM structure (`svg_dom::Document`)
2. Extract pattern pieces (top-level `<g>` elements with IDs)
3. Convert SVG elements to ezdxf entities:
   - `<path>` → `Line`, `Arc`, `Polyline` entities
   - `<line>` → `Line` entity
   - `<circle>` → `Circle` entity
   - `<text>` → `Text` entity
4. Map SVG groups/layers to DXF layers (ASTM layer names)
5. Create ezdxf `Block` definitions for pattern pieces
6. Handle coordinate transformations (Y-axis inversion)
7. Flatten curves to polylines (with tolerance)

### Data Structures

```rust
// Intermediate representation (ezdxf-like)

// @brief DXF version enumeration.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DxfVersion {
    R12,  // AC1009
    R13,  // AC1012
    // ... other versions if needed
}

// @brief DXF layer name.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Layer(String);

// @brief Point in 2D space.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Point {
    pub x: f64,
    pub y: f64,
}

// @brief Base trait for DXF entities.
pub trait Entity {
    fn layer(&self) -> &Layer;
    fn entity_type(&self) -> &str;
}

// @brief LINE entity.
#[derive(Debug, Clone)]
pub struct Line {
    pub layer: Layer,
    pub start: Point,
    pub end: Point,
}

// @brief ARC entity.
#[derive(Debug, Clone)]
pub struct Arc {
    pub layer: Layer,
    pub center: Point,
    pub radius: f64,
    pub start_angle: f64,  // degrees
    pub end_angle: f64,     // degrees
}

// @brief CIRCLE entity.
#[derive(Debug, Clone)]
pub struct Circle {
    pub layer: Layer,
    pub center: Point,
    pub radius: f64,
}

// @brief POLYLINE entity.
#[derive(Debug, Clone)]
pub struct Polyline {
    pub layer: Layer,
    pub vertices: Vec<Point>,
    pub closed: bool,
}

// @brief TEXT entity.
#[derive(Debug, Clone)]
pub struct Text {
    pub layer: Layer,
    pub insertion_point: Point,
    pub height: f64,
    pub rotation: f64,  // degrees
    pub content: String,  // ASCII-only
}

// @brief BLOCK definition.
#[derive(Debug, Clone)]
pub struct Block {
    pub name: String,  // ASCII-only, sanitized
    pub entities: Vec<Box<dyn Entity>>,
}

// @brief Drawing (ezdxf-like intermediate representation).
#[derive(Debug, Clone)]
pub struct Drawing {
    pub version: DxfVersion,
    pub blocks: Vec<Block>,
    pub modelspace_entities: Vec<Box<dyn Entity>>,  // Entities not in blocks
}
```

### API

```rust
// In seamly_svg2ezdxf crate

// @brief Convert SVG Document to ezdxf Drawing.
// @param doc The SVG DOM document to convert.
// @param options Conversion options.
// @return Drawing object ready for DXF export.
pub fn svg_to_ezdxf(
    doc: &svg_dom::Document,
    options: &SvgToEzdxfOptions,
) -> Result<Drawing, SvgToEzdxfError>;

// @brief Conversion options.
#[derive(Debug, Clone)]
pub struct SvgToEzdxfOptions {
    // Target DXF version (default: R12 for ASTM).
    pub dxf_version: DxfVersion,
    // Whether to create blocks for pattern pieces.
    pub create_blocks: bool,
    // Coordinate system transformation (Y-axis inversion).
    pub invert_y: bool,
    // SVG height for Y-axis inversion (required if invert_y is true).
    pub svg_height: Option<f64>,
    // Flattening tolerance for curves (in SVG units).
    pub flatten_tolerance: f64,
    // Custom layer mapping function.
    pub layer_mapper: Option<Box<dyn Fn(&xmltree::Element) -> String>>,
}

// @brief Default options for ASTM-D6673-10 export.
impl Default for SvgToEzdxfOptions {
    fn default() -> Self {
        Self {
            dxf_version: DxfVersion::R12,
            create_blocks: true,
            invert_y: true,
            svg_height: None,
            flatten_tolerance: 0.1,
            layer_mapper: None,
        }
    }
}
```

### Dependencies
- `svg_dom` (workspace crate)
- `geometry` (workspace crate) - for path parsing and flattening
- `xmltree` - for DOM traversal

## Crate 2: `ezdxf2dxfastm`

### Purpose
Convert the ezdxf intermediate representation to DXF-ASTM format (R12, constrained structure).

### Responsibilities
1. Validate Drawing object (must be R12, entities must conform to ASTM)
2. Write DXF R12 file structure:
   - Minimal/empty HEADER section
   - BLOCKS section (pattern piece blocks)
   - ENTITIES section (INSERT entities for all blocks)
   - EOF marker
3. Enforce ASTM-D6673-10 constraints:
   - No TABLES section
   - No default layout blocks
   - 7-bit ASCII only for text
   - Only basic entity types
4. Encode DXF entities using group codes
5. Insert blocks into ENTITIES section using INSERT entities (so blocks are visible)
6. Generate teaching version with inline comments (optional)
7. Handle coordinate system (already transformed in seamly_svg2ezdxf)

### Data Structures

```rust
// In ezdxf2dxfastm crate

// @brief DXF writer for ASTM-D6673-10 format.
pub struct DxfAstmWriter {
    writer: std::io::BufWriter<std::fs::File>,
    version: DxfVersion,
}

// @brief Export options.
#[derive(Debug, Clone)]
pub struct DxfAstmExportOptions {
    // Whether to include HEADER section (empty if true).
    pub include_header: bool,
    // Whether to validate entities before export.
    pub validate_entities: bool,
    // Whether to sanitize text to ASCII-only.
    pub sanitize_text: bool,
    // Whether to create a teaching version with inline comments.
    pub create_teaching_version: bool,
}
```

### API

```rust
// In ezdxf2dxfastm crate

// @brief Export Drawing to DXF-ASTM format.
// @param drawing The ezdxf Drawing object to export.
// @param output_path Path to write the DXF file.
// @param options Export options.
// @return Result indicating success or error.
pub fn export_dxf_astm(
    drawing: &seamly_svg2ezdxf::Drawing,
    output_path: impl AsRef<std::path::Path>,
    options: &DxfAstmExportOptions,
) -> Result<(), DxfAstmExportError>;

// @brief Validate Drawing for ASTM-D6673-10 compliance.
// @param drawing The Drawing to validate.
// @return Result with validation errors if any.
pub fn validate_astm_compliance(
    drawing: &seamly_svg2ezdxf::Drawing,
) -> Result<(), Vec<ValidationError>>;
```

### Dependencies
- `seamly_svg2ezdxf` (workspace crate) - for Drawing type
- Standard library only (file I/O, string handling)

## Integration in `app_core`

### High-Level Export Function

```rust
// In app_core/src/lib.rs

use seamly_svg2ezdxf::{svg_to_ezdxf, SvgToEzdxfOptions};
use ezdxf2dxfastm::{export_dxf_astm, DxfAstmExportOptions};

// @brief Export SVG layout DOM to DXF-ASTM format.
// @param doc The SVG DOM document (typically layout_dom).
// @param output_path Path to write the DXF file.
// @return Result indicating success or error.
pub fn export_layout_to_dxf_astm(
    doc: &svg_dom::Document,
    output_path: impl AsRef<Path>,
) -> CoreResult<()> {
    // Step 1: Convert SVG to ezdxf intermediate representation
    let options = SvgToEzdxfOptions {
        dxf_version: seamly_svg2ezdxf::DxfVersion::R12,
        create_blocks: true,
        invert_y: true,
        svg_height: get_svg_height(doc)?,
        flatten_tolerance: 0.1,
        layer_mapper: None,  // Use default ASTM layer mapping
    };

    let drawing = svg_to_ezdxf(doc, &options)
        .map_err(|e| CoreError::SvgToEzdxf(e))?;

    // Step 2: Export to DXF-ASTM format
    let export_options = DxfAstmExportOptions {
        include_header: false,
        validate_entities: true,
        sanitize_text: true,
        create_teaching_version: false, // Set based on user preference
    };

    export_dxf_astm(&drawing, output_path, &export_options)
        .map_err(|e| CoreError::DxfAstmExport(e))?;

    Ok(())
}
```

## File Structure

```
crates/
  seamly_svg2ezdxf/
    Cargo.toml
    src/
      lib.rs              # Public API
      drawing.rs          # Drawing, Block, Entity types
      entities.rs         # Entity implementations (Line, Arc, etc.)
      converter.rs        # SVG to ezdxf conversion logic
      layers.rs           # Layer mapping
      error.rs            # Error types
      tests/
        test_*.rs

  ezdxf2dxfastm/
    Cargo.toml
    src/
      lib.rs              # Public API
      writer.rs           # DXF file writer
      encoder.rs           # Entity encoding (group codes)
      validator.rs        # ASTM compliance validation
      error.rs            # Error types
      tests/
        test_*.rs
```

## Testing Strategy

### Unit Tests

**seamly_svg2ezdxf**:
- Test SVG path → Line conversion
- Test SVG path → Polyline conversion (flattened curves)
- Test SVG circle → Circle entity
- Test SVG text → Text entity
- Test pattern piece → Block conversion
- Test layer mapping

**ezdxf2dxfastm**:
- Test Line → DXF encoding
- Test Arc → DXF encoding
- Test Polyline → DXF encoding
- Test Block → DXF encoding
- Test file structure (HEADER, BLOCKS, ENTITIES, EOF)

### Integration Tests

1. **Full Pipeline Test**:
   - Load SVG file
   - Convert SVG → ezdxf
   - Convert ezdxf → DXF-ASTM
   - Validate DXF file structure

2. **Round-Trip Test**:
   - SVG → ezdxf → DXF-ASTM → (use dxf2svg) → SVG
   - Compare geometry

3. **Validation Test**:
   - Test with non-compliant entities (should fail validation)
   - Test with ASCII-only text (should pass)
   - Test with Unicode text (should sanitize or fail)

## Future Extensions

### Additional DXF Export Formats

```rust
// Future: ezdxf2dxfr2000 crate
pub fn export_dxf_r2000(drawing: &seamly_svg2ezdxf::Drawing, ...) -> Result<...>;

// Future: ezdxf2dxfr2018 crate
pub fn export_dxf_r2018(drawing: &seamly_svg2ezdxf::Drawing, ...) -> Result<...>;
```

### DXF Import (Future)

```rust
// Future: dxf2ezdxf crate
pub fn dxf_to_ezdxf(input_path: &Path) -> Result<seamly_svg2ezdxf::Drawing, ...>;

// Then: ezdxf2svg crate
pub fn ezdxf_to_svg(drawing: &seamly_svg2ezdxf::Drawing, ...) -> Result<svg_dom::Document, ...>;
```

### Other Input Formats

```rust
// Future: other2ezdxf crates
// - step2ezdxf (STEP/IGES import)
// - dwg2ezdxf (AutoCAD DWG import)
```

## Implementation Order

1. **Phase 1**: Create `seamly_svg2ezdxf` crate
   - Define Drawing, Block, Entity types
   - Implement basic entity conversions (Line, Circle, Text)
   - Implement path flattening and conversion
   - Implement pattern piece extraction

2. **Phase 2**: Create `ezdxf2dxfastm` crate
   - Implement DXF R12 file writer
   - Implement entity encoding (group codes)
   - Implement ASTM constraint validation
   - Implement file structure (HEADER, BLOCKS, ENTITIES)

3. **Phase 3**: Integration
   - Add export function to `app_core`
   - Test with real SVG files
   - Validate with DXF parsers

4. **Phase 4**: Polish
   - Error handling improvements
   - Performance optimization
   - Documentation
   - Additional entity types if needed

## UI Integration

The DXF-ASTM export is fully integrated into the desktop UI (`ui_desktop` crate):

- **Export Menu**: Available in the "Export ▼" dropdown
- **File Dialog**: Standard file save dialog for selecting output path
- **Teaching Version Dialog**: User prompt to create teaching version with comments
- **Status Updates**: Real-time status messages in the UI

See `docs/DXF_EXPORT_UI_WORKFLOW.md` for detailed UI workflow documentation.

## References

- **ezdxf Python Library**: https://github.com/mozman/ezdxf (MIT License)
- **DXF R12 Specification**: Autodesk DXF Reference (AC1009)
- **ASTM-D6673-10 Standard**: https://store.astm.org/d6673-10.html
- **DXF ↔ SVG Mapping Reference**: See `docs/DXF_SVG_MAPPING_REFERENCE.md`
- **UI Workflow**: See `docs/DXF_EXPORT_UI_WORKFLOW.md`