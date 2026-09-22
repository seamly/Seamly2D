<!-- MIT License: https://opensource.org/licenses/MIT -->
# seamly_svg2ezdxf Conversion Workflow

This document describes the detailed workflow of the `seamly_svg2ezdxf` crate, which converts SVG DOM documents to an ezdxf-like intermediate representation.

## Overview

The `seamly_svg2ezdxf` crate processes SVG documents in a structured, recursive manner, converting SVG elements to DXF entities while maintaining proper layer assignments and coordinate transformations.

## High-Level Flow

```
SVG Document (svg_dom::Document)
    ↓
svg_to_ezdxf() - Main entry point
    ↓
[if create_blocks: true]
    extract_pattern_pieces() - Extract <g> elements as blocks
    ↓
[else]
    convert_elements_to_modelspace() - Direct conversion
    ↓
convert_element_tree() - Recursive element processing
    ↓
Entity conversion functions (convert_line, convert_circle, convert_text)
    ↓
Drawing object with Blocks and Modelspace Entities
```

## Detailed Workflow

### Phase 1: Initialization (`svg_to_ezdxf`)

**Input**: `svg_dom::Document`, `SvgToEzdxfOptions`

**Steps**:

1. **Extract Root Element**
   - Get `<svg>` root element
   - Log root element name and children count

2. **Determine SVG Height**
   - If `invert_y: true`:
     - Check `options.svg_height`
     - If not provided, try to parse from SVG root `height` attribute
     - Default to 100.0 if not found
   - If `invert_y: false`:
     - Height not needed (set to 0.0)

3. **Create Drawing Object**
   - Initialize with `options.dxf_version` (default: R12)
   - Start with empty blocks and modelspace entities

4. **Choose Processing Mode**
   - **Mode A**: `create_blocks: true` → Pattern piece extraction
   - **Mode B**: `create_blocks: false` → Direct modelspace conversion

### Phase 2A: Pattern Piece Extraction (`extract_pattern_pieces`)

**When**: `create_blocks: true`

**Process**:

For each direct child of the `<svg>` root:

1. **Group Elements with IDs** (`<g id="...">`):
   - Sanitize ID to create valid DXF block name
   - Create new `Block` with sanitized name
   - Call `convert_element_tree()` on the group with `parent_layer: None`
   - Add all converted entities to the block
   - Add block to drawing

2. **Group Elements without IDs** (`<g>`):
   - Convert group children directly to modelspace
   - No block created

3. **Non-Group Elements** (`<text>`, `<line>`, `<circle>`, etc.):
   - Convert directly to modelspace entities
   - No block created

**Result**: 
- Pattern pieces (groups with IDs) → `Block` definitions
- Other elements → `modelspace_entities`

### Phase 2B: Direct Modelspace Conversion (`convert_elements_to_modelspace`)

**When**: `create_blocks: false`

**Process**:

1. Call `convert_element_tree()` on root element
2. All entities go directly to `modelspace_entities`
3. No blocks are created

**Result**: All entities in `modelspace_entities`

### Phase 3: Element Tree Conversion (`convert_element_tree`)

**Recursive Function**: Processes elements and their children

**Parameters**:
- `element`: Current SVG element to process
- `target`: Where to add entities (Block or Drawing)
- `options`: Conversion options
- `svg_height`: SVG height for coordinate transformation
- `parent_layer`: Optional layer from parent group

**Process for Each Element**:

#### Step 1: Determine Layer

1. **Check Parent Layer**:
   - If `parent_layer` is provided (from meaningful group), use it
   - Otherwise, check element's own ID for layer hints

2. **Layer Hint Detection** (from element ID):
   - "cutline" or "boundary" → "Piece boundary"
   - "notch" → "Notches"
   - "grainline" or "grain" → "Grain line"
   - "seamline" or "seam" → "Sew lines"
   - "drill" or "hole" → "Drill holes"
   - Otherwise → Use `map_svg_to_astm_layer()` for default

3. **Default Layer Mapping** (`map_svg_to_astm_layer`):
   - `<text>` element → "Text/Annotations"
   - All other elements → "Internal lines"

#### Step 2: Convert Element Based on Type

**`<line>` Element**:
1. Parse attributes: `x1`, `y1`, `x2`, `y2`
2. Create `Point` objects for start and end
3. Apply Y-axis inversion if `invert_y: true`
4. Get layer from `map_svg_to_astm_layer()` (default: "Internal lines")
5. Override layer if `parent_layer` provided
6. Create `Line` entity
7. Add to target

**`<circle>` Element**:
1. Parse attributes: `cx`, `cy`, `r`
2. Validate: radius must be > 0 (return `None` if invalid)
3. Create `Point` for center
4. Apply Y-axis inversion if `invert_y: true`
5. Get layer from `map_svg_to_astm_layer()` (default: "Internal lines")
6. Override layer if `parent_layer` provided
7. Create `Circle` entity
8. Add to target

**`<text>` Element**:
1. Parse attributes: `x`, `y`, `font-size` (default: 12.0)
2. Extract text content from text node children
3. Sanitize to ASCII-only (remove non-ASCII characters)
4. Validate: return `None` if content is empty after sanitization
5. Create `Point` for insertion point
6. Apply Y-axis inversion if `invert_y: true`
7. **Always use "Text/Annotations" layer** (ignores parent layer)
8. Create `Text` entity
9. Add to target

**`<g>` (Group) Element**:
1. Check if group ID contains layer hints
2. If yes, determine group layer (e.g., "cutline_piece1" → "Piece boundary")
3. Use group layer or inherit from `parent_layer`
4. Recursively process all children with inherited layer
5. Return early (don't process children again)

**Other Elements**:
- Process children recursively
- No direct entity conversion

#### Step 3: Process Children

**Layer Inheritance Rules**:

1. **Root `<svg>` Element**:
   - **Never passes layer to children** (`child_parent_layer = None`)
   - Children determine their own layers

2. **Groups with Layer Hints**:
   - If group ID contains layer hints → Pass layer to children
   - Otherwise → Don't pass layer to children

3. **Groups without Layer Hints**:
   - Don't pass layer to children

4. **Other Elements**:
   - Don't pass layer to children

**Recursive Processing**:
- For each child element, call `convert_element_tree()` with appropriate `parent_layer`
- Skip non-element children (text nodes, comments, etc.)

## Layer Mapping Details

### Layer Inheritance Hierarchy

```
Root <svg>
  └─ No layer passed to children
     ├─ <g id="cutline_piece1">
     │  └─ Layer: "Piece boundary" (from ID)
     │     └─ Passed to children
     │        └─ <line> → Inherits "Piece boundary"
     ├─ <g id="notch_mark1">
     │  └─ Layer: "Notches" (from ID)
     │     └─ Passed to children
     │        └─ <circle> → Inherits "Notches"
     └─ <text>
        └─ Layer: "Text/Annotations" (always, never inherits)
```

### Layer Mapping Rules

| Element Type | ID Contains | Layer Name |
|--------------|-------------|------------|
| Any | "cutline" or "boundary" | "Piece boundary" |
| Any | "notch" | "Notches" |
| Any | "grainline" or "grain" | "Grain line" |
| Any | "seamline" or "seam" | "Sew lines" |
| Any | "drill" or "hole" | "Drill holes" |
| `<text>` | (any or none) | "Text/Annotations" |
| Other | (none) | "Internal lines" |

## Coordinate Transformation

### Y-Axis Inversion

**When**: `invert_y: true` and `svg_height` is provided

**Formula**: `dxf_y = svg_height - svg_y`

**Applied To**:
- Line: `start.y` and `end.y`
- Circle: `center.y`
- Text: `insertion_point.y`

**X-Axis**: Unchanged

**Example**:
- SVG: `(10, 20)` in SVG with height 100
- DXF: `(10, 80)` after inversion

## Entity Conversion Functions

### `convert_line(element, options, svg_height)`

**Input**: SVG `<line>` element

**Process**:
1. Parse `x1`, `y1`, `x2`, `y2` attributes (default: 0.0)
2. Create start and end points
3. Apply Y-axis inversion if enabled
4. Get layer from `map_svg_to_astm_layer()` (default: "Internal lines")
5. Create `Line` entity

**Output**: `Option<Line>` (always `Some` for valid line)

### `convert_circle(element, options, svg_height)`

**Input**: SVG `<circle>` element

**Process**:
1. Parse `cx`, `cy`, `r` attributes (default: 0.0)
2. Validate: `r > 0.0` (return `None` if invalid)
3. Create center point
4. Apply Y-axis inversion if enabled
5. Get layer from `map_svg_to_astm_layer()` (default: "Internal lines")
6. Create `Circle` entity

**Output**: `Option<Circle>` (`None` if radius <= 0)

### `convert_text(element, options, svg_height)`

**Input**: SVG `<text>` element

**Process**:
1. Parse `x`, `y` attributes (default: 0.0)
2. Parse `font-size` attribute (default: 12.0)
3. Extract text content from text node children
4. Sanitize to ASCII-only (remove non-ASCII characters)
5. Validate: return `None` if content is empty
6. Create insertion point
7. Apply Y-axis inversion if enabled
8. Always use "Text/Annotations" layer
9. Create `Text` entity

**Output**: `Option<Text>` (`None` if content is empty)

## Error Handling

### Conversion Errors

- **Missing SVG Height**: When `invert_y: true` but height not provided
- **Invalid Attributes**: Float parsing failures (use defaults)
- **Empty Content**: Text elements with no content (return `None`)

### Validation

- **Circle Radius**: Must be > 0.0
- **Text Content**: Must not be empty after sanitization
- **Block Names**: Sanitized to ASCII-only, valid DXF names

## Debug Output

The conversion process includes comprehensive debug logging:

- **Step-by-step progress**: Each phase logs its progress
- **Element processing**: Each element logs its type and processing
- **Layer determination**: Shows how layers are determined
- **Entity creation**: Logs entity creation and layer assignment
- **Child processing**: Shows recursive processing of children

Enable debug output by running tests with `--nocapture` flag.

## Example Conversion

### Input SVG

```xml
<svg width="100" height="100" xmlns="http://www.w3.org/2000/svg">
    <g id="cutline_piece1">
        <line x1="0" y1="0" x2="10" y2="10"/>
    </g>
    <g id="notch_mark1">
        <circle cx="20" cy="20" r="2"/>
    </g>
    <text x="30" y="30">Label</text>
</svg>
```

### Conversion Process

1. **Pattern Piece Extraction** (`create_blocks: true`):
   - Group "cutline_piece1" → Block "cutline_piece1"
     - Line inherits layer "Piece boundary" from group ID
   - Group "notch_mark1" → Block "notch_mark1"
     - Circle inherits layer "Notches" from group ID
   - Text element → Modelspace entity
     - Uses layer "Text/Annotations" (always)

2. **Result Drawing**:
   - Blocks: 2
     - Block "cutline_piece1": 1 LINE entity on "Piece boundary"
     - Block "notch_mark1": 1 CIRCLE entity on "Notches"
   - Modelspace: 1 TEXT entity on "Text/Annotations"

## Current Implementation Status

### ✅ Implemented

- Basic entity conversions (Line, Circle, Text)
- Path conversion (`<path>` → Polyline) with curve flattening
- Polyline, Polygon, Rect, Ellipse conversions
- Pattern piece extraction to blocks
- Layer mapping with inheritance rules
- Coordinate transformation (Y-axis inversion)
- Text ASCII sanitization
- Block name sanitization
- Comprehensive debug logging
- Drawing inspection output (write_to_file method)

### ⏳ Pending

- Arc conversion (`<arc>` element - not yet encountered in test files)
- Transform attribute parsing (rotation, scale, translate)
- Style attribute parsing (stroke, fill)
- Group transform inheritance

## Testing

See `docs/TESTING_SEAMLY_SVG2EZDXF.md` for testing strategies and examples.

## References

- **DXF Export Architecture**: `docs/DXF_EXPORT_ARCHITECTURE.md`
- **DXF-SVG Mapping**: `docs/DXF_SVG_MAPPING_REFERENCE.md`
- **Unit Test Commands**: `docs/UNIT_TEST_COMMANDS.md`
