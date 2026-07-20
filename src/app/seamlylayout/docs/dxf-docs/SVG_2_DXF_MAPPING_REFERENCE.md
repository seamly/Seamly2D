<!-- MIT License: https://opensource.org/licenses/MIT -->
# DXF ↔ SVG Entity Mapping Reference

This document provides a reference mapping between DXF entities and SVG elements, based on learnings from `dxf2svg` and `ezdxf` libraries. This helps inform our SVG → DXF-ASTM export implementation.

## Reference Libraries

- **dxf2svg** (Python): Simple converter showing basic DXF → SVG mappings
- **ezdxf** (Python): Full-featured DXF library with drawing add-on for SVG export
- **ezdxf.addons.drawing**: Advanced rendering with style preservation

## Entity Mapping Table

### Basic Geometric Entities

| DXF Entity | SVG Element | Notes |
|------------|-------------|-------|
| **LINE** | `<line x1="..." y1="..." x2="..." y2="..."/>` | Direct 1:1 mapping |
| **CIRCLE** | `<circle cx="..." cy="..." r="..."/>` | Direct mapping, or `<path>` for arcs |
| **ARC** | `<path d="M ... A rx ry x-axis-rotation large-arc sweep x y"/>` | SVG arc command |
| **POLYLINE** | `<polyline points="..."/>` or `<polygon points="..."/>` | Closed polylines → polygon |
| **LWPOLYLINE** | Same as POLYLINE | Lightweight polyline variant |

### Text Entities

| DXF Entity | SVG Element | Notes |
|------------|-------------|-------|
| **TEXT** | `<text x="..." y="..." ...>content</text>` | Position, font, size, alignment |
| **MTEXT** | `<text>` or multiple `<text>` elements | Multi-line text may need splitting |

### Complex Entities

| DXF Entity | SVG Element | Notes |
|------------|-------------|-------|
| **SPLINE** | `<path d="M ... C ..."/>` | Convert to cubic Bezier curves |
| **ELLIPSE** | `<ellipse cx="..." cy="..." rx="..." ry="..."/>` | Or `<path>` with arc commands |
| **HATCH** | `<path>` with fill | Convert boundary to path |
| **SOLID** | `<polygon>` | 3-4 vertex filled polygon |

### Structural Elements

| DXF Element | SVG Element | Notes |
|-------------|-------------|-------|
| **BLOCK** | `<g id="...">...</g>` | Group with ID |
| **INSERT** | `<use xlink:href="#block-id" transform="..."/>` | Reference with transform |
| **LAYER** | `<g class="layer-name">...</g>` | Group with class or style |

## Coordinate System Differences

### DXF Coordinate System
- Origin: Bottom-left (typically)
- Y-axis: Increases upward
- Units: Drawing units (mm, inches, etc.)

### SVG Coordinate System
- Origin: Top-left
- Y-axis: Increases downward
- Units: User units (pixels, mm, etc.)

### Transformation Required

When converting SVG → DXF:
```
dxf_x = svg_x
dxf_y = svg_height - svg_y
```

When converting DXF → SVG (for reference):
```
svg_x = dxf_x
svg_y = svg_height - dxf_y
```

## Entity-Specific Conversion Notes

### LINE

**DXF → SVG** (for reference):
```xml
<line x1="x1" y1="y1" x2="x2" y2="y2" stroke="black" stroke-width="1"/>
```

**SVG → DXF** (our implementation):
- DXF group code: `0` (LINE), `10`/`20` (start), `11`/`21` (end)
- Layer: Map from SVG element's layer/group

### CIRCLE

**DXF → SVG** (for reference):
```xml
<circle cx="cx" cy="cy" r="radius" fill="none" stroke="black"/>
```

**SVG → DXF** (our implementation):
- DXF group code: `0` (CIRCLE), `10`/`20` (center), `40` (radius)
- Note: SVG circles are always complete; DXF circles are complete by default

### ARC

**DXF → SVG** (for reference):
```xml
<path d="M start_x start_y A rx ry x-axis-rotation large-arc sweep end_x end_y"/>
```

**DXF ARC parameters**:
- Center point (cx, cy)
- Radius
- Start angle (degrees)
- End angle (degrees)

**SVG → DXF** (our implementation):
- Parse SVG arc command (`A` in path data)
- Convert to DXF ARC: center, radius, start/end angles
- Or flatten to POLYLINE if conversion is complex

### POLYLINE

**DXF → SVG** (for reference):
```xml
<polyline points="x1,y1 x2,y2 x3,y3 ..." fill="none" stroke="black"/>
```

**SVG → DXF** (our implementation):
- DXF group code: `0` (POLYLINE), `70` (flags), `10`/`20` (vertices)
- Closed flag: `70` bit 1 = closed polyline
- Convert SVG `<polyline>` or flattened `<path>` to DXF POLYLINE

### TEXT

**DXF → SVG** (for reference):
```xml
<text x="x" y="y" font-family="Arial" font-size="12">Text content</text>
```

**DXF TEXT parameters**:
- Insertion point (x, y)
- Height (text size)
- Rotation angle
- Text value (7-bit ASCII for ASTM-D6673-10)

**SVG → DXF** (our implementation):
- Extract text content (must be ASCII-only)
- Position: `<text>` x/y attributes → DXF insertion point
- Height: `font-size` → DXF height
- Rotation: `transform="rotate(...)"` → DXF rotation angle

### PATH (SVG) → DXF Entities

SVG paths are complex and may map to multiple DXF entities:

| SVG Path Command | DXF Entity | Conversion Method |
|------------------|------------|-------------------|
| `M` (MoveTo) | Start of POLYLINE | New polyline segment |
| `L` (LineTo) | LINE or POLYLINE vertex | Add vertex to polyline |
| `H` (Horizontal) | LINE or POLYLINE | Convert to LineTo |
| `V` (Vertical) | LINE or POLYLINE | Convert to LineTo |
| `C` (Cubic Bezier) | SPLINE or POLYLINE | Flatten to polyline vertices |
| `Q` (Quadratic Bezier) | SPLINE or POLYLINE | Flatten to polyline vertices |
| `A` (Arc) | ARC or POLYLINE | Convert to ARC if possible, else flatten |
| `Z` (Close) | Closed POLYLINE | Set closed flag |

**Recommended approach for ASTM-D6673-10**:
- Flatten all curves to POLYLINE (simpler, more compatible)
- Use tolerance-based flattening for accuracy
- Preserve closed paths as closed polylines

## Layer Mapping

### DXF Layers → SVG Groups

**DXF → SVG** (for reference):
```xml
<g class="layer-name" stroke="layer-color">
  <!-- entities on this layer -->
</g>
```

### SVG Groups → DXF Layers (ASTM-D6673-10)

**SVG → DXF** (our implementation):
- Map SVG `<g>` elements with IDs to DXF layers
- Use predefined ASTM layer names:
  - `*cutline*` or `*boundary*` → "Piece boundary"
  - `*notch*` → "Notches"
  - `*grainline*` or `*grain*` → "Grain line"
  - `*seamline*` or `*seam*` → "Sew lines"
  - `*text*` → "Text/Annotations"
  - `*drill*` or `*hole*` → "Drill holes"
  - Default → "Internal lines"

## Block Structure

### DXF BLOCK → SVG Group

**DXF → SVG** (for reference):
```xml
<g id="block-name">
  <!-- block entities -->
</g>
```

### SVG Group → DXF BLOCK (Pattern Pieces)

**SVG → DXF** (our implementation):
- Each top-level `<g>` with `id` attribute → DXF BLOCK
- Block name = sanitized SVG `id` (ASCII-only)
- Block contains all entities from that group
- Entities retain their layer assignments

## Style Mapping

### DXF → SVG (for reference)

| DXF Property | SVG Attribute | Notes |
|--------------|---------------|-------|
| Color (by layer) | `stroke="color"` | RGB or named color |
| Line weight | `stroke-width="..."` | Convert to SVG units |
| Linetype | `stroke-dasharray="..."` | Convert dash pattern |
| Fill | `fill="color"` or `fill="none"` | Solid fill or none |

### SVG → DXF (ASTM-D6673-10 constraints)

**Important**: ASTM-D6673-10 has strict limitations:
- **No custom linetypes** (TABLES section omitted)
- **No text styles** (TABLES section omitted)
- **7-bit ASCII only** for all text
- **Basic entities only**: LINE, ARC, CIRCLE, POLYLINE, TEXT

**Our approach**:
- Ignore SVG styling (stroke-dasharray, custom fonts, etc.)
- Use default DXF styling (continuous line, default text)
- Focus on geometry and layer assignment
- Ensure all text is ASCII-only

## Example Conversions

### Example 1: Simple Line

**SVG**:
```xml
<line x1="10" y1="20" x2="50" y2="60" stroke="black"/>
```

**DXF** (R12):
```
0
LINE
8
Piece boundary
10
10.0
20
20.0
11
50.0
21
60.0
```

### Example 2: Circle

**SVG**:
```xml
<circle cx="100" cy="100" r="25" fill="none" stroke="black"/>
```

**DXF** (R12):
```
0
CIRCLE
8
Internal lines
10
100.0
20
100.0
40
25.0
```

### Example 3: Path (Flattened)

**SVG**:
```xml
<path d="M 0,0 L 10,0 L 10,10 L 0,10 Z" stroke="black"/>
```

**DXF** (R12, as POLYLINE):
```
0
POLYLINE
8
Piece boundary
70
1
0
VERTEX
8
Piece boundary
10
0.0
20
0.0
0
VERTEX
8
Piece boundary
10
10.0
20
0.0
0
VERTEX
8
Piece boundary
10
10.0
20
10.0
0
VERTEX
8
Piece boundary
10
0.0
20
10.0
0
SEQEND
```

### Example 4: Text

**SVG**:
```xml
<text x="50" y="50" font-size="12">Pattern Piece A</text>
```

**DXF** (R12):
```
0
TEXT
8
Text/Annotations
10
50.0
20
50.0
40
12.0
1
Pattern Piece A
```

## Validation Strategy

### Round-Trip Testing

1. **SVG → DXF**: Export our SVG pattern to DXF-ASTM
2. **DXF → SVG**: Use dxf2svg or ezdxf to convert back
3. **Compare**: Visual and geometric comparison
4. **Iterate**: Refine mapping based on differences

### Tools for Validation

- **dxf2svg**: Quick visual check of basic geometry
- **ezdxf.addons.drawing**: More accurate rendering with styles
- **CAD Software**: LibreCAD, QCAD, AutoCAD (for DXF validation)
- **Target Software**: Gerber, CLO3D (for ASTM-D6673-10 compatibility)

## References

1. **dxf2svg**: https://github.com/fritz-heinrichmeyer/dxf2svg
2. **ezdxf**: https://github.com/mozman/ezdxf
3. **ezdxf.addons.drawing**: https://ezdxf.mozman.at/docs/addons/drawing.html
4. **DXF R12 Reference**: Autodesk DXF Reference (AC1009)
5. **SVG Specification**: https://www.w3.org/TR/SVG11/
