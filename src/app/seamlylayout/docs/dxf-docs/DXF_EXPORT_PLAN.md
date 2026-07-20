<!-- MIT License: https://opensource.org/licenses/MIT -->
# DXF-ASTM Export Implementation Plan

## Overview

This document outlines the plan for implementing DXF-ASTM (ASTM-D6673-10) export functionality in SeamlyLayout, based on learnings from ezdxf's `gerber_D6673` add-on.

**Architecture**: We use a two-stage conversion approach with two reusable crates:
1. **`seamly_svg2ezdxf`**: Converts SVG DOM to ezdxf-like intermediate representation
2. **`ezdxf2dxfastm`**: Converts intermediate representation to DXF-ASTM format

See `docs/DXF_EXPORT_ARCHITECTURE.md` for detailed architecture documentation.

## Reference: ezdxf's gerber_D6673 Add-on

The `gerber_D6673` add-on in ezdxf (MIT licensed) provides a reference implementation for exporting DXF files that conform to ASTM-D6673-10 constraints.

### Important: Exporter Workflow

**Key Insight**: `gerber_D6673` is an **exporter**, not an importer. It takes a pre-built `ezdxf.Drawing` object as input and formats it for Gerber Technology's DXF parser.

**Workflow in ezdxf (Python)**:
1. Create a DXF R12 Drawing: `doc = ezdxf.new("R12")`
2. Build geometry using ezdxf API: add LINE, ARC, POLYLINE, TEXT entities to modelspace
3. Ensure content conforms to ASTM-D6673-10 (correct layers, entity types, ASCII text)
4. Pass the Drawing to exporter: `gerber_D6673.export_file(doc, "output.dxf")`
5. Exporter automatically formats the file (removes HEADER, TABLES, default blocks)

**For Rust Implementation**:
- We cannot use ezdxf directly (it's Python)
- We must implement our own DXF R12 writer
- We can learn from ezdxf's structure and constraints
- Our workflow: Build geometry from SVG → Create DXF entities → Write constrained DXF file

### Key Constraints from gerber_D6673

1. **DXF Version**: Only DXF R12 (AC1009) is supported
   - Rejects any other version
   - R12 has simpler structure, better compatibility with older parsers

2. **Minimal File Structure** (handled by exporter):
   - **No HEADER section** (or empty/default only)
   - **No TABLES section** (no linetypes, text styles, etc.)
   - **No default layout blocks** (`$MODEL_SPACE`, `$PAPER_SPACE`)
   - **Only ENTITIES section** (and BLOCKS if needed for pattern pieces)

3. **Entity Limitations** (must be enforced when building geometry):
   - Only basic entities: LINE, ARC, CIRCLE, POLYLINE, TEXT
   - No advanced features (MTEXT, complex linetypes, etc.)
   - No custom styles or linetypes (since TABLES section is omitted)

4. **Text Encoding** (must be enforced when building geometry):
   - 7-bit ASCII only
   - No Unicode or extended ASCII characters
   - All text content must be ASCII-compatible

5. **Block Structure** (build as DXF BLOCKS):
   - Each pattern piece should be a DXF BLOCK
   - Blocks contain geometry (outlines, notches, etc.) and text annotations
   - Block names should be meaningful (e.g., pattern piece IDs)

6. **Input Requirements**:
   - Drawing object must be DXF R12
   - Entities must already conform to ASTM-D6673-10 standard
   - Content should use appropriate layers (Piece boundary, Notches, Grain line, etc.)

## ASTM-D6673-10 Standard Requirements

### Layer Structure

The standard defines approximately 23 predefined layers for pattern data:

1. **Piece boundary** - Main outline of pattern piece
2. **Notches** - Notch marks on pattern edges
3. **Sew lines** - Seam lines
4. **Grain line** - Fabric grain direction indicator
5. **Internal lines** - Internal cutting lines, darts, etc.
6. **Drill holes** - Marking points
7. **Text/Annotations** - Piece labels, instructions
8. **Cut line** - Cutting lines
9. **Fold line** - Folding lines
10. **Placement line** - Placement guides
11. ... (additional layers as specified in standard)

### Pattern Piece Structure

- Each pattern piece is represented as a DXF BLOCK
- Block contains:
  - Geometry entities (lines, arcs, polylines) on appropriate layers
  - Text annotations (piece name, size, instructions)
  - Grain line indicators
  - Notches and drill holes

### Coordinate System

- DXF uses standard Cartesian coordinates
- SVG Y-axis is inverted (SVG: top-to-bottom, DXF: bottom-to-top)
- Need to transform: `dxf_y = svg_height - svg_y`

## Implementation Plan

### Key Difference: Rust Implementation vs. ezdxf

**ezdxf approach (Python)**:
- Uses `ezdxf.new("R12")` to create a Drawing object
- Uses Drawing API to add entities (LINE, ARC, etc.)
- Calls `gerber_D6673.export_file(doc, path)` to write constrained DXF

**Our Rust approach**:
- Parse SVG DOM to extract geometry
- Convert SVG paths to DXF entities (LINE, ARC, POLYLINE)
- Build DXF file structure directly (no intermediate Drawing object)
- Write constrained DXF R12 format (no HEADER, no TABLES, minimal structure)

**Why this approach**:
- We already have SVG DOM parsing and geometry conversion
- We can map SVG elements directly to DXF entities
- We control the entire export pipeline
- No need for a full DXF library - just a minimal R12 writer

### Phase 1: Research and Setup

1. **Investigate Rust DXF Libraries**
   - Search for existing Rust crates for DXF read/write
   - Evaluate: `dxf`, `dxf-rs`, or similar
   - If none suitable, plan to implement minimal DXF R12 writer
   - **Decision**: Likely implement minimal writer since we only need R12 export

2. **Study Mapping References**
   - Review `dxf2svg` implementation to understand DXF → SVG mappings
   - Study `ezdxf.addons.drawing` for advanced entity handling
   - Review `gerber_D6673.py` implementation for export constraints
   - Create mapping reference document (see `DXF_SVG_MAPPING_REFERENCE.md`)
   - Understand DXF R12 file structure and entity encoding format
   - Note: We can reference the approach but must implement in Rust

3. **Create Mapping Reference**
   - Document DXF ↔ SVG entity mappings
   - Document coordinate system transformations
   - Document layer mapping strategies
   - See `docs/DXF_SVG_MAPPING_REFERENCE.md` for complete reference

### Phase 2: Create DXF Export Crate

**Option A: New Crate `dxf_export`**
- Pros: Separation of concerns, reusable
- Cons: Additional crate to maintain

**Option B: Add to `app_core`**
- Pros: Simpler structure, co-located with other export functions
- Cons: Mixes concerns

**Recommendation**: Create new crate `dxf_export` for modularity.

### Phase 3: Core DXF Writer Implementation

#### 3.1 DXF R12 File Structure

```
0
SECTION
2
ENTITIES
  ... entities ...
0
ENDSEC
0
SECTION
2
BLOCKS
  ... block definitions ...
0
ENDSEC
0
EOF
```

#### 3.2 Entity Encoding

DXF uses group codes:
- `0`: Entity type (LINE, ARC, POLYLINE, TEXT, etc.)
- `8`: Layer name
- `10`, `20`, `30`: X, Y, Z coordinates
- Additional codes per entity type

#### 3.3 Basic Entity Writers

Implement functions to write:
- `LINE`: Start and end points
- `ARC`: Center, radius, start/end angles
- `CIRCLE`: Center and radius
- `POLYLINE`: Sequence of vertices
- `TEXT`: Position, height, content
- `BLOCK`: Block definition with entities
- `INSERT`: Block reference

### Phase 4: SVG to DXF Mapping

**Direct Entity Creation**: Unlike ezdxf which uses a Drawing API, we'll create DXF entities directly from SVG elements and write them immediately to the DXF file structure.

#### 4.1 Path Conversion

1. **Flatten SVG Paths**: Use `geometry::Path::flatten()` to convert curves to polylines
   - Curves (Cubic, Quadratic, Arc) → POLYLINE with multiple vertices
   - Tolerance-based flattening for accuracy
2. **Coordinate Transform**: Invert Y-axis for DXF coordinate system
   - SVG: Y increases downward
   - DXF: Y increases upward
   - Transform: `dxf_y = svg_height - svg_y`
3. **Layer Mapping**: Map SVG groups/elements to ASTM layers based on:
   - Element ID patterns (e.g., `*cutline*`, `*grainline*`, `*notch*`)
   - SVG class attributes
   - Element hierarchy

#### 4.2 Pattern Piece Extraction

1. **Identify Pattern Pieces**: Find top-level `<g>` elements with IDs (pattern pieces)
2. **Extract Geometry**: Collect all paths within each piece
   - Convert each `<path>` element to DXF entities
   - Group by layer (cutline, notch, grainline, etc.)
3. **Extract Annotations**: Find text elements, grain lines, notches
   - Convert `<text>` elements to DXF TEXT entities
   - Ensure ASCII-only text (strip/replace non-ASCII characters)
4. **Create DXF Blocks**: One block per pattern piece
   - Block name = pattern piece ID (sanitized to ASCII)
   - Block contains all entities for that piece
   - Entities retain their layer assignments

#### 4.3 Layer Mapping Logic

```rust
fn map_svg_to_dxf_layer(element: &Element) -> &str {
    let id = element.attributes.get("id").unwrap_or("");
    let id_lower = id.to_lowercase();

    if id_lower.contains("cutline") || id_lower.contains("boundary") {
        "Piece boundary"
    } else if id_lower.contains("notch") {
        "Notches"
    } else if id_lower.contains("grainline") || id_lower.contains("grain") {
        "Grain line"
    } else if id_lower.contains("seamline") || id_lower.contains("seam") {
        "Sew lines"
    } else if id_lower.contains("text") || element.name == "text" {
        "Text/Annotations"
    } else if id_lower.contains("drill") || id_lower.contains("hole") {
        "Drill holes"
    } else {
        "Internal lines"  // Default
    }
}
```

### Phase 5: Export Function API

```rust
// In dxf_export crate

// @brief Export an SVG Document to DXF-ASTM format.
// @param doc The SVG DOM document containing pattern pieces.
// @param output_path Path to write the DXF file.
// @param options Export options (coordinate system, layer mapping, etc.).
// @return Result indicating success or error.
pub fn export_dxf_astm(
    doc: &svg_dom::Document,
    output_path: impl AsRef<Path>,
    options: &ExportOptions,
) -> Result<(), DxfExportError>;

// @brief Export options for DXF-ASTM export.
#[derive(Debug, Clone)]
pub struct ExportOptions {
    // DXF version (must be R12 for ASTM-D6673-10).
    pub dxf_version: DxfVersion,
    // Whether to create blocks for pattern pieces.
    pub use_blocks: bool,
    // Coordinate system transformation (Y-axis inversion).
    pub invert_y: bool,
    // Flattening tolerance for curves (in SVG units).
    pub flatten_tolerance: f32,
    // Custom layer mapping function.
    pub layer_mapper: Option<Box<dyn Fn(&Element) -> String>>,
}

// @brief DXF version enumeration.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DxfVersion {
    R12,  // AC1009 - Required for ASTM-D6673-10
    R13,  // AC1012 - Alternative if needed
}
```

### Phase 6: Integration with app_core

Add export function to `app_core`:

```rust
// In app_core/src/lib.rs

// @brief Export an SVG Document to DXF-ASTM format.
// @param doc The SVG DOM document to export.
// @param path Destination DXF file path.
pub fn export_dxf_astm(doc: &svg_dom::Document, path: impl AsRef<Path>) -> CoreResult<()> {
    dxf_export::export_dxf_astm(doc, path, &dxf_export::ExportOptions::default())
        .map_err(|e| CoreError::Dxf(e))
}
```

## Implementation Workflow

### High-Level Process

```
SVG Document (xmltree::Document)
    ↓
1. Extract Pattern Pieces (<g> elements with IDs)
    ↓
2. For each Pattern Piece:
    a. Extract paths, text, annotations
    b. Flatten curves to polylines
    c. Map to ASTM layers
    d. Convert to DXF entities (LINE, ARC, POLYLINE, TEXT)
    e. Create DXF BLOCK definition
    ↓
3. Write DXF R12 File:
    a. Write minimal HEADER (empty or default)
    b. Write BLOCKS section (all pattern piece blocks)
    c. Write ENTITIES section (empty or block references)
    d. Write EOF
    ↓
DXF-ASTM File (R12 format, constrained structure)
```

### Detailed Conversion Steps

1. **SVG Parsing** (already implemented in `svg_dom`)
   - Parse SVG file into `Document` structure
   - Access elements via `xmltree::Element`

2. **Pattern Piece Identification**
   - Find all top-level `<g>` elements with `id` attributes
   - These represent individual pattern pieces

3. **Geometry Extraction**
   - For each pattern piece:
     - Find all `<path>` elements (recursively)
     - Parse path data using `geometry::Path::from_svg_path()`
     - Flatten curves using `geometry::Path::flatten(tolerance)`
     - Identify layer from element ID/class

4. **DXF Entity Creation**
   - Convert flattened paths to DXF POLYLINE entities
   - Convert straight segments to DXF LINE entities (optional optimization)
   - Convert arcs to DXF ARC entities (if not flattened)
   - Convert text to DXF TEXT entities (ASCII-only)

5. **Block Assembly**
   - Create one DXF BLOCK per pattern piece
   - Block name = sanitized pattern piece ID
   - Block contains all entities for that piece
   - Entities assigned to appropriate ASTM layers

6. **File Writing**
   - Write DXF R12 structure:
     - Minimal/empty HEADER
     - BLOCKS section with all pattern piece blocks
     - ENTITIES section (can be empty, or contain block references)
     - EOF marker

## Implementation Steps

### Step 1: Create `seamly_svg2ezdxf` Crate

1. Add to workspace `Cargo.toml`
2. Create `crates/seamly_svg2ezdxf/` directory
3. Define Drawing, Block, Entity types (see architecture doc)
4. Set up basic structure with error types
5. Implement basic entity types (Line, Circle, Text, Polyline, Arc)

### Step 2: Create `ezdxf2dxfastm` Crate

1. Add to workspace `Cargo.toml`
2. Create `crates/ezdxf2dxfastm/` directory
3. Set up DXF writer structure
4. Implement entity encoding (group codes)
5. Implement ASTM validation

### Step 3: Implement SVG to ezdxf Converter (in `seamly_svg2ezdxf`)

1. Pattern piece extraction from SVG DOM
2. Path flattening and conversion to entities
3. Layer mapping logic (SVG groups → ASTM layers)
4. Block creation for pattern pieces
5. Coordinate transformation (Y-axis inversion)

### Step 4: Implement ezdxf to DXF-ASTM Writer (in `ezdxf2dxfastm`)

1. Create `Writer` struct for DXF file output
2. Implement section writers (HEADER, BLOCKS, ENTITIES)
3. Implement entity encoders (LINE, ARC, POLYLINE, TEXT, etc.)
4. Implement ASTM constraint validation
5. Write file structure (minimal HEADER, no TABLES, EOF)

### Step 5: Testing

1. **Unit Tests for `seamly_svg2ezdxf`**
   - Test SVG path → Line conversion
   - Test SVG path → Polyline conversion (flattened curves)
   - Test SVG circle → Circle entity
   - Test SVG text → Text entity
   - Test pattern piece → Block conversion
   - Test layer mapping logic
   - Test coordinate transformations (Y-axis inversion)

2. **Unit Tests for `ezdxf2dxfastm`**
   - Test Line → DXF encoding
   - Test Arc → DXF encoding
   - Test Polyline → DXF encoding
   - Test Block → DXF encoding
   - Test file structure (HEADER, BLOCKS, ENTITIES, EOF)
   - Test ASCII text sanitization
   - Test ASTM validation

3. **Integration Tests**
   - Test full pipeline: SVG → ezdxf → DXF-ASTM
   - Test with sample SVG files from `input/` directory
   - Test with `layout_dom` from UI

4. **Round-Trip Validation**
   - Export SVG → DXF using our implementation
   - Convert DXF → SVG using `dxf2svg` or `ezdxf.addons.drawing`
   - Compare geometry visually and programmatically
   - Identify and fix mapping issues

5. **DXF Parser Validation**
   - Test with LibreCAD (open source CAD)
   - Test with QCAD (open source CAD)
   - Validate DXF file structure is correct

6. **Target Software Testing**
   - Test with Gerber Technology software (if available)
   - Test with CLO3D (if available)
   - Validate ASTM-D6673-10 compatibility

### Step 6: Integration in `app_core`

1. Add export function to `app_core`
2. Integrate with UI (export button/menu)
3. Add error handling
4. Add user feedback

### Step 7: Documentation

1. API documentation for both crates
2. Usage examples
3. Layer mapping guide
4. Architecture documentation (see `DXF_EXPORT_ARCHITECTURE.md`)
5. Known limitations

## Dependencies

Potential Rust crates to evaluate:
- `dxf` or `dxf-rs` - If available for DXF writing
- Or implement minimal DXF R12 writer from scratch

## File Structure

```
crates/
  seamly_svg2ezdxf/
    Cargo.toml
    src/
      lib.rs              # Public API
      drawing.rs          # Drawing, Block, Entity types
      entities.rs         # Entity implementations
      converter.rs        # SVG to ezdxf conversion
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

See `docs/DXF_EXPORT_ARCHITECTURE.md` for complete architecture details.

## References

1. **ezdxf gerber_D6673**: https://ezdxf.readthedocs.io/en/stable/addons/gerber_D6673.html
2. **ezdxf GitHub**: https://github.com/mozman/ezdxf (MIT License)
3. **dxf2svg**: https://github.com/fritz-heinrichmeyer/dxf2svg (for mapping reference)
4. **ezdxf.addons.drawing**: https://ezdxf.mozman.at/docs/addons/drawing.html (for advanced rendering)
5. **ASTM-D6673-10 Standard**: https://store.astm.org/d6673-10.html (withdrawn 2019, but still referenced)
6. **DXF R12 Specification**: Autodesk DXF Reference (AC1009)
7. **DXF ↔ SVG Mapping Reference**: See `docs/DXF_SVG_MAPPING_REFERENCE.md`

## Notes

- The ASTM-D6673-10 standard was withdrawn in 2019, but many systems still support it
- ezdxf's `gerber_D6673` is MIT licensed, so we can reference and learn from its implementation
- We cannot copy the standard text verbatim without purchasing the ASTM standard document
- Focus on compatibility with common pattern software (Gerber, CLO3D, etc.)
