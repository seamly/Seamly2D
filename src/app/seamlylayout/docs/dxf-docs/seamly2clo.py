#!/usr/bin/env python3
"""
Seamly2D to CLO3D DXF Converter

Converts Seamly2D AAMA DXF exports to ASTM DXF format for CLO3D compatibility.

AAMA DXF (input):
  - Layer 1: Boundary polyline
  - Layer 4: Notches
  - Layer 7: Grainline

ASTM DXF (output):
  - Layer 1: Boundary polyline
  - Layer 2: Turn points (corner vertices) - ASTM standard
  - Layer 3: Curve points (interpolation vertices) - ASTM standard
  - Layer 4: Notches
  - Layer 7: Grainline
  - Layer 14: Sewing line (duplicate polyline with group 250=2)

The ASTM DXF standard never supported bezier or spline curves - it always
represented curves as polylines with many small segments. The layer 2/3
POINT entities provide metadata indicating which vertices are "turn points"
(corners/control points) vs "curve points" (interpolation detail).

Software like CLO3D uses this metadata to determine which vertices are
selectable line segment endpoints vs which vertices form smooth curves
between those endpoints. This allows curves to be edited and sewn as
single units rather than many individual line segments.
"""

import argparse
import math
import re
import sys
from dataclasses import dataclass
from typing import Optional


@dataclass
class Point:
    x: float
    y: float


@dataclass
class Block:
    name: str
    vertices: list[Point]
    grainline: Optional[tuple[Point, Point]] = None
    notches: list[tuple[Point, Point]] = None  # List of (start, end) line segments

    def __post_init__(self):
        if self.notches is None:
            self.notches = []


def parse_dxf_pairs(lines: list[str]) -> list[tuple[int, str]]:
    """Parse DXF file into list of (group_code, value) pairs."""
    pairs = []
    i = 0
    while i + 1 < len(lines):
        try:
            code = int(lines[i].strip())
            value = lines[i + 1].strip()
            pairs.append((code, value))
            i += 2
        except ValueError:
            # Skip malformed pairs
            i += 1
    return pairs


def parse_seamly_dxf(filepath: str) -> list[Block]:
    """Parse Seamly2D DXF file and extract blocks with their vertices."""
    with open(filepath, 'r') as f:
        lines = f.readlines()

    pairs = parse_dxf_pairs(lines)
    blocks = []
    i = 0

    while i < len(pairs):
        code, value = pairs[i]

        # Look for BLOCK entity
        if code == 0 and value == 'BLOCK':
            block_name = None
            vertices = []
            grainline = None
            notches = []
            j = i + 1

            # Parse until ENDBLK
            while j < len(pairs):
                c, v = pairs[j]

                if c == 0 and v == 'ENDBLK':
                    break

                # Block name (group code 2, first occurrence)
                if c == 2 and block_name is None:
                    block_name = v

                # LWPOLYLINE entity
                if c == 0 and v == 'LWPOLYLINE':
                    layer = None
                    poly_vertices = []
                    k = j + 1

                    while k < len(pairs):
                        pc, pv = pairs[k]

                        # New entity starts
                        if pc == 0:
                            break

                        # Layer
                        if pc == 8:
                            layer = pv

                        # X coordinate
                        if pc == 10:
                            x = float(pv)
                            # Look ahead for Y
                            if k + 1 < len(pairs) and pairs[k + 1][0] == 20:
                                y = float(pairs[k + 1][1])
                                poly_vertices.append(Point(x, y))

                        k += 1

                    # Only use layer 1 (boundary) polylines
                    if layer == '1' and poly_vertices:
                        vertices = poly_vertices

                    j = k
                    continue

                # LINE entity (for grainline on layer 7, notches on layer 4)
                if c == 0 and v == 'LINE':
                    layer = None
                    x1, y1, x2, y2 = None, None, None, None
                    k = j + 1

                    while k < len(pairs):
                        lc, lv = pairs[k]

                        if lc == 0:
                            break

                        if lc == 8:
                            layer = lv
                        if lc == 10:
                            x1 = float(lv)
                        if lc == 20:
                            y1 = float(lv)
                        if lc == 11:
                            x2 = float(lv)
                        if lc == 21:
                            y2 = float(lv)

                        k += 1

                    if all(coord is not None for coord in [x1, y1, x2, y2]):
                        if layer == '7':
                            grainline = (Point(x1, y1), Point(x2, y2))
                        elif layer == '4':
                            notches.append((Point(x1, y1), Point(x2, y2)))

                    j = k
                    continue

                j += 1

            # Skip standard blocks
            if block_name and block_name not in ('*Model_Space', '*Paper_Space'):
                if vertices:
                    blocks.append(Block(name=block_name, vertices=vertices,
                                       grainline=grainline, notches=notches))

            i = j
        else:
            i += 1

    return blocks


def calculate_angle(p1: Point, p2: Point, p3: Point) -> float:
    """
    Calculate the angle at p2 formed by p1-p2-p3.
    Returns angle in degrees (0-180).
    """
    # Vectors from p2 to p1 and p2 to p3
    v1 = (p1.x - p2.x, p1.y - p2.y)
    v2 = (p3.x - p2.x, p3.y - p2.y)

    # Magnitudes
    mag1 = math.sqrt(v1[0]**2 + v1[1]**2)
    mag2 = math.sqrt(v2[0]**2 + v2[1]**2)

    if mag1 == 0 or mag2 == 0:
        return 180.0  # Degenerate case, treat as straight

    # Dot product
    dot = v1[0]*v2[0] + v1[1]*v2[1]

    # Clamp to avoid numerical issues with acos
    cos_angle = max(-1.0, min(1.0, dot / (mag1 * mag2)))

    return math.degrees(math.acos(cos_angle))


def find_closest_vertex(point: Point, vertices: list[Point]) -> int:
    """Find the index of the vertex closest to the given point."""
    min_dist = float('inf')
    min_idx = 0
    for i, v in enumerate(vertices):
        dist = math.sqrt((point.x - v.x)**2 + (point.y - v.y)**2)
        if dist < min_dist:
            min_dist = dist
            min_idx = i
    return min_idx


def detect_corners(vertices: list[Point], angle_threshold: float = 60.0,
                   notches: list[tuple[Point, Point]] = None,
                   notch_corners: bool = False) -> list[bool]:
    """
    Detect which vertices are corners based on angle threshold.
    Returns a list of booleans - True for corner, False for curve point.

    A corner is where the angle is sharper than the threshold.
    angle_threshold: angles less than this are considered corners (default 60 degrees)
    notches: list of notch line segments
    notch_corners: if True, vertices at notch positions are also marked as corners
    """
    n = len(vertices)
    is_corner = [False] * n

    for i in range(n):
        p1 = vertices[(i - 1) % n]
        p2 = vertices[i]
        p3 = vertices[(i + 1) % n]

        angle = calculate_angle(p1, p2, p3)

        # Sharp angle = corner
        if angle < angle_threshold:
            is_corner[i] = True

    # Mark notch positions as corners if requested
    if notch_corners and notches:
        for p1, p2 in notches:
            # Find closest vertex to either notch endpoint
            idx1 = find_closest_vertex(p1, vertices)
            idx2 = find_closest_vertex(p2, vertices)

            # Calculate distances
            d1 = math.sqrt((p1.x - vertices[idx1].x)**2 + (p1.y - vertices[idx1].y)**2)
            d2 = math.sqrt((p2.x - vertices[idx2].x)**2 + (p2.y - vertices[idx2].y)**2)

            # Only mark the single closest vertex
            if d1 <= d2:
                is_corner[idx1] = True
            else:
                is_corner[idx2] = True

    return is_corner


def write_clo_dxf(blocks: list[Block], output_path: str, angle_threshold: float = 60.0,
                  notch_corners: bool = False):
    """Write blocks in CLO3D-compatible DXF format."""

    lines = []

    # Header
    lines.extend([
        "  0", "SECTION",
        "  2", "HEADER",
        "  9", "$ACADVER",
        "  1", "AC1009",
        "  0", "ENDSEC",
    ])

    # Blocks section
    lines.extend(["  0", "SECTION", "  2", "BLOCKS"])

    for block in blocks:
        is_corner = detect_corners(block.vertices, angle_threshold,
                                   block.notches, notch_corners)

        # Block header
        lines.extend([
            "  0", "BLOCK",
            "  8", "1",
            "  2", f"{block.name}_M",
            "  70", "64",
            "  10", "0",
            "  20", "0",
        ])

        # First POLYLINE - layer 14 with 250=2 (sewing line)
        lines.extend([
            "  0", "POLYLINE",
            "  8", "14",
            "  66", "1",
            "  70", "1",
            "  250", "2",
        ])

        for v in block.vertices:
            lines.extend([
                "  0", "VERTEX",
                "  8", "14",
                "  10", f"{v.x:.6f}",
                "  20", f"{v.y:.6f}",
            ])

        lines.extend(["  0", "SEQEND"])

        # POINT entities for corners (layer 2) and curves (layer 3)
        for i, v in enumerate(block.vertices):
            layer = "2" if is_corner[i] else "3"
            lines.extend([
                "  0", "POINT",
                "  8", layer,
                "  10", f"{v.x:.6f}",
                "  20", f"{v.y:.6f}",
            ])

        # Grainline (layer 7)
        if block.grainline:
            p1, p2 = block.grainline
            lines.extend([
                "  0", "LINE",
                "  8", "7",
                "  10", f"{p1.x:.6f}",
                "  20", f"{p1.y:.6f}",
                "  11", f"{p2.x:.6f}",
                "  21", f"{p2.y:.6f}",
            ])

        # Notches (layer 4)
        for p1, p2 in block.notches:
            lines.extend([
                "  0", "LINE",
                "  8", "4",
                "  10", f"{p1.x:.6f}",
                "  20", f"{p1.y:.6f}",
                "  11", f"{p2.x:.6f}",
                "  21", f"{p2.y:.6f}",
            ])

        # Second POLYLINE - layer 1 with 250=0 (boundary)
        lines.extend([
            "  0", "POLYLINE",
            "  8", "1",
            "  66", "1",
            "  70", "1",
            "  250", "0",
        ])

        for v in block.vertices:
            lines.extend([
                "  0", "VERTEX",
                "  8", "1",
                "  10", f"{v.x:.6f}",
                "  20", f"{v.y:.6f}",
            ])

        lines.extend(["  0", "SEQEND"])

        # Repeat POINT entities after second polyline (as CLO3D does)
        for i, v in enumerate(block.vertices):
            layer = "2" if is_corner[i] else "3"
            lines.extend([
                "  0", "POINT",
                "  8", layer,
                "  10", f"{v.x:.6f}",
                "  20", f"{v.y:.6f}",
            ])

        lines.extend(["  0", "ENDBLK"])

    lines.extend(["  0", "ENDSEC"])

    # Entities section
    lines.extend(["  0", "SECTION", "  2", "ENTITIES"])

    for block in blocks:
        lines.extend([
            "  0", "INSERT",
            "  8", "1",
            "  2", f"{block.name}_M",
            "  10", "0.0",
            "  20", "0.0",
        ])

    lines.extend(["  0", "ENDSEC", "  0", "EOF"])

    with open(output_path, 'w') as f:
        f.write('\n'.join(lines) + '\n')


def print_analysis(blocks: list[Block], angle_threshold: float, notch_corners: bool = False):
    """Print analysis of detected corners vs curve points."""
    for block in blocks:
        is_corner = detect_corners(block.vertices, angle_threshold,
                                   block.notches, notch_corners)
        corners = sum(is_corner)
        curves = len(is_corner) - corners

        print(f"\nBlock: {block.name}")
        print(f"  Total vertices: {len(block.vertices)}")
        print(f"  Corners (layer 2): {corners}")
        print(f"  Curve points (layer 3): {curves}")
        print(f"  Has grainline: {block.grainline is not None}")
        print(f"  Notches: {len(block.notches)}")

        # Show corner indices
        corner_indices = [i for i, c in enumerate(is_corner) if c]
        print(f"  Corner indices: {corner_indices}")


def main():
    parser = argparse.ArgumentParser(
        description='Convert Seamly2D DXF to CLO3D-compatible format'
    )
    parser.add_argument('input', help='Input Seamly2D DXF file')
    parser.add_argument('-o', '--output', help='Output CLO3D DXF file')
    parser.add_argument(
        '-a', '--angle',
        type=float,
        default=120.0,
        help='Angle threshold for corner detection in degrees (default: 120). '
             'Angles sharper than this are corners.'
    )
    parser.add_argument(
        '--analyze',
        action='store_true',
        help='Print analysis of detected corners without converting'
    )
    parser.add_argument(
        '-n', '--notch-corners',
        action='store_true',
        help='Mark vertices at notch positions as corners, splitting curves at notches'
    )

    args = parser.parse_args()

    # Parse input
    blocks = parse_seamly_dxf(args.input)

    if not blocks:
        print(f"Error: No pattern blocks found in {args.input}", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(blocks)} block(s): {[b.name for b in blocks]}")

    if args.analyze:
        print_analysis(blocks, args.angle, args.notch_corners)
        return

    # Determine output path
    output_path = args.output
    if not output_path:
        output_path = args.input.rsplit('.', 1)[0] + '-clo.dxf'

    # Convert
    write_clo_dxf(blocks, output_path, args.angle, args.notch_corners)
    print(f"Converted to: {output_path}")

    # Print summary
    print_analysis(blocks, args.angle, args.notch_corners)


if __name__ == '__main__':
    main()
