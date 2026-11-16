# Author: slspencer
# Copyright: 2026 Seamly2D Project
# License: MIT
"""Validate the actual headless executable without creating a display."""

import argparse
import json
import math
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import xml.etree.ElementTree as xml
import zlib


def jpeg_size(content):
    """Read JPEG frame dimensions without adding an imaging dependency."""
    offset = 2
    while offset < len(content):
        if content[offset] != 255:
            return None
        marker = content[offset + 1]
        offset += 2
        length = int.from_bytes(content[offset:offset + 2], "big")
        if marker in (192, 193, 194):
            height, width = struct.unpack(">HH", content[offset + 3:offset + 7])
            return width, height
        if length < 2:
            return None
        offset += length
    return None


def contains_square(polylines, side, tolerance):
    """Find a physical calibration square irrespective of translation or quarter-turn rotation."""
    for points in polylines:
        if len(points) < 4:
            continue
        width = max(x for x, _ in points) - min(x for x, _ in points)
        height = max(y for _, y in points) - min(y for _, y in points)
        if abs(width - side) <= tolerance and abs(height - side) <= tolerance:
            return True
    return False


def dxf_polylines(content):
    """Read classic DXF polyline vertices, whose coordinates are millimeters."""
    lines = content.decode("utf-8", errors="replace").splitlines()
    current = {}
    points = []
    polylines = []
    for code, value in zip(lines[::2], lines[1::2]):
        code = code.strip()
        if code != "0":
            current[code] = value
            continue
        if current.get("type") == "VERTEX" and "10" in current and "20" in current:
            points.append((float(current["10"]), float(current["20"])))
        if value == "POLYLINE":
            points = []
        if value == "SEQEND":
            polylines.append(points)
            points = []
        current = {"type": value}
    return polylines


def hpgl_polylines(content):
    """Read absolute pen strokes in HP-GL units, forty units per millimeter."""
    polylines = []
    points = []
    for command, arguments in re.findall(r"([A-Z]{2})([^;]*);", content.decode("ascii")):
        if command not in ("PU", "PD"):
            continue
        numbers = [int(value) for value in re.findall(r"-?\d+", arguments)]
        coordinates = list(zip(numbers[::2], numbers[1::2]))
        if command == "PU":
            polylines.append(points)
            points = coordinates
        else:
            points.extend(coordinates)
    polylines.append(points)
    return polylines


def pdf_calibration_edges(content):
    """Measure 192-pixel horizontal edges after the PDF graphics-state transforms."""
    measurements = []
    for stream in re.findall(rb"stream\r?\n(.*?)\r?\nendstream", content, re.DOTALL):
        try:
            drawing = zlib.decompress(stream).decode("latin-1")
        except zlib.error:
            continue
        matrix = (1, 0, 0, 1, 0, 0)
        stack = []
        previous = None
        for line in drawing.splitlines():
            fields = line.split()
            if fields == ["q"]:
                stack.append(matrix)
            elif fields == ["Q"] and stack:
                matrix = stack.pop()
            elif len(fields) == 7 and fields[-1] == "cm":
                a, b, c, d, e, f = matrix
                g, h, i, j, k, l = map(float, fields[:6])
                matrix = (a*g+c*h, b*g+d*h, a*i+c*j, b*i+d*j, a*k+c*l+e, b*k+d*l+f)
            elif len(fields) == 3 and fields[-1] in ("m", "l"):
                point = tuple(map(float, fields[:2]))
                if fields[-1] == "l" and previous:
                    dx, dy = point[0] - previous[0], point[1] - previous[1]
                    if abs(abs(dx) - 192) < 0.02 and abs(dy) < 0.02:
                        a, b, c, d, _, _ = matrix
                        measurements.append(math.hypot(a*dx+c*dy, b*dx+d*dy))
                previous = point
    return measurements


def main():
    """Run format, transport, and failure checks with bounded process waits."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output_directory", type=Path)
    parser.add_argument("--seamly2d", type=Path)
    parser.add_argument("--pattern", type=Path)
    parser.add_argument("--measurements", type=Path)
    parser.add_argument("--size")
    parser.add_argument("--height")
    parser.add_argument("--svg-input", type=Path)
    parser.add_argument("--layout-settings", type=Path)
    parser.add_argument("--timeout", type=float, default=180)
    parser.add_argument("--unplaced-settings", type=Path)
    arguments = parser.parse_args()
    if bool(arguments.seamly2d) != bool(arguments.pattern):
        parser.error("--seamly2d and --pattern must be supplied together")
    executable = arguments.executable.resolve()
    directory = arguments.output_directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    if not environment.get("QT_PLUGIN_PATH"):
        qmake = shutil.which("qmake")
        if not qmake:
            parser.error("Set QT_PLUGIN_PATH or put the matching Qt qmake on PATH.")
        query = subprocess.run([qmake, "-query", "QT_INSTALL_PLUGINS"],
                               capture_output=True, text=True, timeout=10, check=True)
        environment["QT_PLUGIN_PATH"] = query.stdout.strip()
    print("QT_PLUGIN_PATH=" + environment["QT_PLUGIN_PATH"], flush=True)
    environment["QT_QPA_PLATFORM"] = "offscreen"
    environment["SEAMLY_DATA_DIR"] = str(directory / "data")
    fixture = directory / "pieces.svg"
    fixture.write_text(
        '<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">'
        '<g id="piece-1"><path d="M0 0 L96 0 L96 96 L0 96 Z" fill="none" stroke="black"/></g>'
        '<g id="piece-2"><path d="M0 0 L48 0 L48 48 L0 48 Z" fill="none" stroke="black"/></g></svg>',
        encoding="utf-8",
    )
    if arguments.svg_input:
        fixture.write_bytes(arguments.svg_input.read_bytes())
    source_root = xml.parse(fixture).getroot()
    expected_paths = len(source_root.findall(".//{http://www.w3.org/2000/svg}path"))
    expected_pieces = sum(element.get("data-type") == "piece" for element in source_root.iter())
    richmond_fixture = arguments.svg_input and "richmond" in arguments.svg_input.name.lower()
    format_settings = directory / "layout.json"
    settings_object = {}
    if arguments.layout_settings:
        settings_object = json.loads(arguments.layout_settings.read_text(encoding="utf-8"))
    format_settings.write_text(json.dumps(settings_object), encoding="utf-8")
    settings = directory / "tiled.json"
    settings.write_text(json.dumps({**settings_object, "paperType": "tiled"}), encoding="utf-8")
    # Plain PDF exports a roll or fabric; tiled paper exports as pdf-tiled (Export menu rule).
    roll_settings = directory / "roll.json"
    roll_settings.write_text(json.dumps({**settings_object, "mediaType": "paper", "paperType": "roll"}),
                             encoding="utf-8")

    def settings_for(format_name):
        """Settings file for a format: tiled paper for pdf-tiled, a roll for pdf, else as given."""
        return {"pdf-tiled": settings, "pdf": roll_settings}.get(format_name, format_settings)
    failures = []

    def run(name, options, expected_status=0, input_bytes=None, program=None):
        """Record diagnostics and reject unexpected statuses or missing errors."""
        result = subprocess.run(
            [str(program or executable), *options],
            input=input_bytes,
            capture_output=True,
            timeout=arguments.timeout,
            env=environment,
        )
        (directory / (name + ".stdout.txt")).write_bytes(result.stdout)
        (directory / (name + ".stderr.txt")).write_bytes(result.stderr)
        valid = result.returncode == expected_status
        if expected_status:
            valid = valid and bool(result.stderr.strip())
        if not valid:
            failures.append(name)
        print(f"{name}: status={result.returncode}, expected={expected_status}, {'PASS' if valid else 'FAIL'}")
        return result

    signatures = {
        "pdf": b"%PDF", "pdf-tiled": b"%PDF", "png": b"\x89PNG\r\n\x1a\n",
        "jpg": b"\xff\xd8", "jpeg": b"\xff\xd8", "ps": b"%!PS", "eps": b"%!PS",
    }
    if richmond_fixture:
        incomplete_output = directory / "richmond-incomplete.svg"
        incomplete_output.unlink(missing_ok=True)
        result = run("richmond-incomplete", [
            "--export", "svg", "--output", str(incomplete_output), str(fixture),
        ], 70)
        if incomplete_output.exists() or b"Front" not in result.stderr or result.stdout.strip():
            failures.append("richmond-incomplete-status")
    width = height = 0
    for format_name in ("svg", "pdf", "pdf-tiled", "png", "jpg", "jpeg", "dxf", "hpgl", "ps", "eps"):
        output = directory / ("layout." + format_name)
        output.unlink(missing_ok=True)
        options = ["--export", format_name, "--output", str(output), str(fixture),
                   "--settings", str(settings_for(format_name))]
        result = run(format_name, options)
        if result.returncode != 0 or not output.exists() or output.stat().st_size == 0:
            failures.append(format_name + "-output")
            continue
        content = output.read_bytes()
        if format_name in signatures and not content.startswith(signatures[format_name]):
            failures.append(format_name + "-signature")
        if format_name == "svg":
            root = xml.fromstring(content)
            # Tiled paper adds one tile marker path; every input path must survive.
            piece_paths = [path for path in root.findall(".//{http://www.w3.org/2000/svg}path")
                           if path.get("id") != "tileMarkerPath"]
            if len(piece_paths) != expected_paths:
                failures.append("svg-path-count")
            piece_count = sum(element.get("data-type") == "piece" for element in root.iter())
            if expected_pieces and piece_count != expected_pieces:
                failures.append("svg-piece-count")
            width, height = float(root.get("width")), float(root.get("height"))
            if width <= 0 or height <= 0:
                failures.append("svg-media-size")
            if "pageWidth" in settings_object and settings_object.get("paperType") == "sheet":
                factor = {"mm": 96 / 25.4, "cm": 96 / 2.54, "in": 96}[settings_object.get("unit", "in")]
                if abs(width - settings_object["pageWidth"] * factor) > 1:
                    failures.append("svg-width-units")
                if abs(height - settings_object["pageHeight"] * factor) > 1:
                    failures.append("svg-height-units")
            if richmond_fixture:
                gauge = next((element for element in root.iter() if element.get("id") == "piece-2"), None)
                path = gauge.find(".//{http://www.w3.org/2000/svg}path") if gauge is not None else None
                coordinates = path.get("d", "") if path is not None else ""
                numbers = [float(value) for value in re.findall(r"-?\d+(?:\.\d+)?", coordinates)]
                if (not numbers or abs(max(numbers[::2]) - min(numbers[::2]) - 192) > 0.02
                        or abs(max(numbers[1::2]) - min(numbers[1::2]) - 192) > 0.02):
                    failures.append("richmond-two-inch-gauge")
        if format_name == "png" and struct.unpack(">II", content[16:24]) != (round(width), round(height)):
            failures.append("png-media-size")
        if format_name in ("jpg", "jpeg") and jpeg_size(content) != (round(width), round(height)):
            failures.append(format_name + "-media-size")
        if format_name in ("pdf", "pdf-tiled"):
            boxes = re.findall(rb"/MediaBox\s*\[0 0 ([\d.]+) ([\d.]+)\]", content)
            if not boxes or any(float(x) <= 0 or float(y) <= 0 for x, y in boxes):
                failures.append(format_name + "-page-size")
            if format_name == "pdf" and boxes:
                # Plain PDF uses the roll settings: compare it with an SVG of the same layout.
                reference = directory / "roll-reference.svg"
                run("roll-reference-svg", ["--export", "svg", "--output", str(reference), str(fixture),
                                           "--settings", str(roll_settings)])
                reference_root = xml.parse(reference).getroot()
                roll_width = float(re.sub(r"[a-z]+$", "", reference_root.get("width", "0")))
                roll_height = float(re.sub(r"[a-z]+$", "", reference_root.get("height", "0")))
                user_unit = re.search(rb"/UserUnit\s+([\d.]+)", content)
                scale = float(user_unit.group(1)) if user_unit else 1.0
                if (abs(float(boxes[0][0]) * scale - roll_width * 0.75) > 1
                        or abs(float(boxes[0][1]) * scale - roll_height * 0.75) > 1):
                    failures.append("pdf-physical-media-size")
            drawing_streams = []
            for stream in re.findall(rb"stream\r?\n(.*?)\r?\nendstream", content, re.DOTALL):
                try:
                    drawing_streams.append(zlib.decompress(stream))
                except zlib.error:
                    continue
            if not any(re.search(rb"[-+\d.]+\s+[-+\d.]+\s+m\b", stream) for stream in drawing_streams):
                failures.append(format_name + "-vector-geometry")
        if format_name in ("ps", "eps"):
            box = re.search(rb"%%BoundingBox: 0 0 (\d+) (\d+)", content)
            if not box or abs(int(box[1]) - width * 0.75) > 1 or abs(int(box[2]) - height * 0.75) > 1:
                failures.append(format_name + "-physical-media-size")
            if not re.search(rb"[-+\d.]+\s+[-+\d.]+\s+m\b", content):
                failures.append(format_name + "-vector-geometry")
        if format_name == "dxf" and b"ENTITIES" not in content:
            failures.append("dxf-entities")
        if format_name == "hpgl" and b"PD" not in content:
            failures.append("hpgl-geometry")
        if richmond_fixture and format_name == "dxf" and not contains_square(dxf_polylines(content), 50.8, 0.03):
            failures.append("dxf-two-inch-gauge")
        if richmond_fixture and format_name == "hpgl" and not contains_square(hpgl_polylines(content), 2032, 2):
            failures.append("hpgl-two-inch-gauge")

    for format_name in ("gcode", "3mf"):
        run(format_name, ["--export", format_name, "--settings", str(format_settings),
                         "--output", str(directory / format_name), str(fixture)], 69)
    if richmond_fixture:
        for paper, orientation, expected in (
            ("Letter", "portrait", (612, 792)),
            ("Letter", "landscape", (792, 612)),
            ("A4", "portrait", (210*72/25.4, 297*72/25.4)),
            ("A4", "landscape", (297*72/25.4, 210*72/25.4)),
        ):
            paper_name = paper + "-" + orientation
            paper_settings = directory / (paper_name + "-exact.json")
            paper_settings.write_text(json.dumps({
                **settings_object, "paperType": "tiled", "tileSize": paper, "tileOrientation": orientation,
            }), encoding="utf-8")
            paper_output = directory / (paper_name + "-exact.pdf")
            result = run("physical-" + paper_name, [
                "--export", "pdf-tiled", "--settings", str(paper_settings),
                "--output", str(paper_output), str(fixture),
            ])
            if result.returncode != 0:
                continue
            content = paper_output.read_bytes()
            boxes = re.findall(rb"/MediaBox\s*\[0 0 ([\d.]+) ([\d.]+)\]", content)
            # PDF decimal serialization rounds A4 dimensions to five decimal places.
            if not boxes or any(abs(float(x)-expected[0]) > 0.0001
                                or abs(float(y)-expected[1]) > 0.0001 for x, y in boxes):
                failures.append(paper_name + "-exact-media-size")
            edges = pdf_calibration_edges(content)
            full_scale_edges = [edge for edge in edges if abs(edge-144) < 0.01]
            if not full_scale_edges:
                failures.append(paper_name + "-two-inch-pdf-gauge")
            print(f"{paper_name}: {len(boxes)} pages, MediaBox={sorted(set(boxes))}, "
                  f"144pt gauge edges={len(full_scale_edges)}")
    unplaced_fixture = directory / "unplaced.svg"
    unplaced_fixture.write_text(
        '<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">'
        '<g id="piece-one"><path d="M0 0 L96 0 L96 96 L0 96 Z"/></g>'
        '<g id="piece-two"><path d="M0 0 L96 0 L96 96 L0 96 Z"/></g></svg>',
        encoding="utf-8",
    )
    insufficient_settings = directory / "insufficient.json"
    # A 20 mm roll is narrower than the 25.4 mm pieces: headless export stops with 70.
    # (A tiled sheet grows its tile grid, so it cannot leave a piece out.)
    insufficient_settings.write_text(json.dumps({
        "unit": "mm", "mediaType": "paper", "paperType": "roll", "rollSize": "custom",
        "rollWidth": 20, "pieceGap": 1,
        "marginLeft": 0, "marginRight": 0, "marginTop": 0, "marginBottom": 0,
    }), encoding="utf-8")
    for format_name in ("svg", "pdf", "png", "jpg", "jpeg", "dxf", "hpgl", "ps", "eps"):
        partial_output = directory / ("partial." + format_name)
        partial_output.unlink(missing_ok=True)
        result = run("unplaced-" + format_name, [
            "--export", format_name, "--settings", str(insufficient_settings),
            "--output", str(partial_output), str(unplaced_fixture),
        ], 70)
        if partial_output.exists() or b"exported " in result.stdout or b"piece-two" not in result.stderr:
            failures.append("unplaced-" + format_name + "-partial-output")
    existing_output = directory / "existing.svg"
    existing_output.write_bytes(b"existing output must remain intact")
    run("unplaced-preserves-existing", [
        "--export", "svg", "--settings", str(insufficient_settings),
        "--output", str(existing_output), str(unplaced_fixture),
    ], 70)
    if existing_output.read_bytes() != b"existing output must remain intact":
        failures.append("unplaced-overwrites-existing")
    run("scale-option-removed", ["--export", "png", "--scale", "2", "--output",
                                 str(directory / "scaled.png"), str(fixture)], 64)
    run("missing-output-directory", ["--export", "svg", "--output",
                                     str(directory / "absent" / "out.svg"), str(fixture)], 64)
    run("unsupported", ["--export", "unknown", "--output", str(directory / "unknown"), str(fixture)], 64)
    run("missing-input", ["--export", "svg", "--output", str(directory / "missing.svg"),
                          str(directory / "absent.svg")], 64)
    invalid = directory / "invalid.json"
    invalid.write_text("not JSON", encoding="utf-8")
    run("invalid-options", ["--export", "svg", "--export-options", str(invalid),
                            "--output", str(directory / "invalid.svg"), str(fixture)], 64)
    for name, document, status in (
        ("invalid-svg", "not SVG", 66),
        ("no-pieces", '<svg xmlns="http://www.w3.org/2000/svg"/>', 70),
    ):
        invalid_document = directory / (name + ".svg")
        invalid_document.write_text(document, encoding="utf-8")
        run(name, ["--export", "svg", "--output", str(directory / (name + "-output.svg")),
                   str(invalid_document)], status)
    run("stdin", ["--export", "svg", "--svg-stdin", "--settings", str(format_settings),
                  "--output", str(directory / "stdin.svg")],
        input_bytes=fixture.read_bytes())
    run("empty-stdin", ["--export", "svg", "--svg-stdin", "--output", str(directory / "empty.svg")],
        64, input_bytes=b"")
    run("help", ["--export", "svg", "--help"])
    if arguments.seamly2d:
        input_directory = directory / "pattern-input"
        input_directory.mkdir(parents=True, exist_ok=True)
        isolated_pattern = input_directory / arguments.pattern.name
        shutil.copy2(arguments.pattern, isolated_pattern)
        isolated_measurements = None
        measurement_source = arguments.measurements
        if measurement_source is None:
            measurement_path = xml.parse(arguments.pattern).getroot().findtext("measurements")
            if measurement_path:
                measurement_source = arguments.pattern.parent / measurement_path
        if measurement_source is not None:
            isolated_measurements = input_directory / measurement_source.name
            shutil.copy2(measurement_source, isolated_measurements)
        settings_directory = directory / "data" / "settings" / "Seamly" / "Seamly2D"
        settings_directory.mkdir(parents=True, exist_ok=True)
        (settings_directory / "qt6_seamly2d.ini").write_text(
            "[paths]\nseamlyLayoutApp=" + executable.as_posix() + "\n", encoding="utf-8"
        )
        environment.pop("HANDOFF_CAPTURE_DIRECTORY", None)
        companions = []
        for option, value in (("--mfile", isolated_measurements),
                              ("--gsize", arguments.size), ("--gheight", arguments.height)):
            if value is not None:
                companions.extend([option, str(value.resolve() if isinstance(value, Path) else value)])
        for format_name in ("svg", "pdf", "pdf-tiled", "png", "jpg", "jpeg",
                            "dxf", "hpgl", "ps", "eps", "gcode", "3mf"):
            output = directory / ("pass-through." + format_name)
            output.unlink(missing_ok=True)
            options = ["--layout-export", "--layout-format", format_name,
                       "--layout-output", str(output), *companions, str(isolated_pattern)]
            options.extend(["--layout-settings", str(settings_for(format_name))])
            expected_status = 69 if format_name in ("gcode", "3mf") else 0
            result = run("pass-through-" + format_name, options, expected_status,
                         program=arguments.seamly2d.resolve())
            if expected_status == 0:
                if result.returncode != 0 or not output.exists() or output.stat().st_size == 0:
                    failures.append("pass-through-" + format_name + "-output")
                elif format_name in signatures and not output.read_bytes().startswith(signatures[format_name]):
                    failures.append("pass-through-" + format_name + "-signature")
        run("pass-through-child-error", ["--layout-export", "--layout-output",
                                        str(directory / "absent" / "child.svg"), *companions,
                                        str(isolated_pattern)], 64,
            program=arguments.seamly2d.resolve())
        if arguments.unplaced_settings:
            output = directory / "pass-through-incomplete.svg"
            output.unlink(missing_ok=True)
            result = run("pass-through-unplaced", [
                "--layout-export", "--layout-settings", str(arguments.unplaced_settings.resolve()),
                "--layout-output", str(output), *companions, str(isolated_pattern),
            ], 70, program=arguments.seamly2d.resolve())
            if output.exists() or b"exported " in result.stdout or b"piece" not in result.stderr.lower():
                failures.append("pass-through-unplaced-partial-output")
    if failures:
        print("FAILED: " + ", ".join(failures))
        return 1
    print("All headless export checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
