# @file compare_handoff_writers.py
# @author slspencer
# @brief Compares paired writer captures after composing their SVG transforms.
# Copyright (C) 2026 Seamly2D Project
# SPDX-License-Identifier: GPL-3.0-or-later
# License: https://www.gnu.org/licenses/gpl-3.0.html

"""Compare <pattern>_old.svg and <pattern>_new.svg in one capture directory.

Ignores untagged wrapper groups and serialization differences. Compares tagged
tree order, ids, data attributes, label text, effective styles, and transformed
polyline vertices and text anchors. Coordinates may differ by at most 0.01 px.
Supports the matrix transforms and absolute polylines emitted by both writers.
"""

import argparse
import math
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as xml


NUMBER = r"[-+]?(?:\d*\.\d+|\d+\.?\d*)(?:[eE][-+]?\d+)?"
IDENTITY = (1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
STYLES = ("stroke", "fill", "stroke-linecap", "stroke-linejoin",
          "stroke-width", "stroke-dasharray", "stroke-dashoffset")


def compose(parent, child):
    """Compose child then parent in SVG coordinates."""
    a, b, c, d, e, f = parent
    g, h, i, j, k, l = child
    return (a*g+c*h, b*g+d*h, a*i+c*j, b*i+d*j, a*k+c*l+e, b*k+d*l+f)


def transform(element):
    value = element.get("transform", "").strip()
    if not value:
        return IDENTITY
    match = re.fullmatch(r"matrix\(([^)]*)\)", value)
    if not match:
        raise ValueError(f"unsupported transform: {value}")
    values = tuple(float(x) for x in re.findall(NUMBER, match[1]))
    if len(values) != 6:
        raise ValueError(f"invalid matrix: {value}")
    return values


def point(matrix, x, y):
    a, b, c, d, e, f = matrix
    return (a*x+c*y+e, b*x+d*y+f)


def path_points(value, matrix):
    """Retain subpath commands so separate lines cannot compare as one line."""
    tokens = re.findall(r"[A-Za-z]|" + NUMBER, value)
    result = []
    index = 0
    while index < len(tokens):
        command = tokens[index]
        index += 1
        if command == "Z":
            result.append(("Z", None))
            continue
        if command not in ("M", "L"):
            raise ValueError(f"unsupported path command: {command}")
        first = True
        while index < len(tokens) and not re.fullmatch(r"[A-Za-z]", tokens[index]):
            x, y = float(tokens[index]), float(tokens[index+1])
            index += 2
            result.append((command if first else "L", point(matrix, x, y)))
            first = False
    return result


def entries(filename):
    """Flatten only semantic groups and effective geometry."""
    result = []

    def visit(element, parent, inherited):
        name = element.tag.rsplit("}", 1)[-1]
        matrix = compose(parent, transform(element))
        styles = inherited | {key: element.get(key) for key in STYLES if key in element.attrib}
        if name == "g" and element.get("data-type"):
            metadata = {key: value for key, value in element.attrib.items()
                        if key.startswith("data-") or key == "id"}
            result.append(("group", metadata, []))
        elif name == "path":
            effective = dict(styles)
            if effective.get("stroke") == "none":
                for key in ("stroke-linecap", "stroke-linejoin", "stroke-width",
                            "stroke-dasharray", "stroke-dashoffset"):
                    effective.pop(key, None)
            result.append(("path", effective, path_points(element.get("d", ""), matrix)))
        elif name == "text":
            metadata = {key: element.get(key, "") for key in
                        ("font-size", "font-weight", "font-style", "fill")}
            metadata["text"] = "".join(element.itertext())
            result.append(("text", metadata, [("anchor", point(matrix, float(element.get("x", 0)),
                                                             float(element.get("y", 0))))]))
        for child in element:
            visit(child, matrix, styles)

    visit(xml.parse(filename).getroot(), IDENTITY, {})
    return result


def same_attribute(key, left, right):
    if key in ("stroke-width", "stroke-dasharray", "stroke-dashoffset", "data-grainline-angle"):
        old = [float(x) for x in re.findall(NUMBER, left)]
        new = [float(x) for x in re.findall(NUMBER, right)]
        return len(old) == len(new) and all(abs(a-b) <= 0.01 for a, b in zip(old, new))
    return left == right


def compare(old_file, new_file):
    """Return the first semantic difference, or None."""
    if not old_file.read_bytes().strip() and not new_file.read_bytes().strip():
        return None  # Valid patterns without layout pieces produce no SVG.
    old, new = entries(old_file), entries(new_file)
    for index, (left, right) in enumerate(zip(old, new)):
        if left[0] != right[0]:
            return f"entry {index}: {left[0]} != {right[0]}"
        if left[1].keys() != right[1].keys():
            return f"entry {index}: attribute names {left[1].keys()} != {right[1].keys()}"
        for key in left[1]:
            if not same_attribute(key, left[1][key], right[1][key]):
                return f"entry {index}: {key}: {left[1][key]!r} != {right[1][key]!r}"
        if len(left[2]) != len(right[2]):
            return f"entry {index}: point counts {len(left[2])} != {len(right[2])}"
        for vertex, (a, b) in enumerate(zip(left[2], right[2])):
            if a[0] != b[0]:
                return f"entry {index} point {vertex}: commands {a[0]} != {b[0]}"
            if a[1] is not None and math.dist(a[1], b[1]) > 0.01:
                return f"entry {index} point {vertex}: {a[1]} != {b[1]}"
    if len(old) != len(new):
        return f"entry counts {len(old)} != {len(new)}"
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--expected-count", type=int, default=21)
    args = parser.parse_args()
    files = sorted(args.directory.rglob("*_old.svg"))
    failed = len(files) != args.expected_count
    if failed:
        print(f"FAIL capture count: {len(files)} != {args.expected_count}")
    for old in files:
        new = old.with_name(old.name.removesuffix("_old.svg") + "_new.svg")
        try:
            difference = compare(old, new)
        except (ValueError, IndexError, OSError, xml.ParseError) as error:
            difference = str(error)
        print(f"{'FAIL' if difference else 'PASS'} {old.relative_to(args.directory)}"
              + (f": {difference}" if difference else ""))
        failed |= difference is not None
    return int(failed)


if __name__ == "__main__":
    sys.exit(main())
