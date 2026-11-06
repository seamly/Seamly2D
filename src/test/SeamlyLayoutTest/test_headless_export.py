# Author: slspencer
# Copyright: 2026 Seamly2D Project
# License: MIT
"""Validate the actual headless executable without creating a display."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as xml


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
    arguments = parser.parse_args()
    if bool(arguments.seamly2d) != bool(arguments.pattern):
        parser.error("--seamly2d and --pattern must be supplied together")
    executable = arguments.executable.resolve()
    directory = arguments.output_directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["QT_QPA_PLATFORM"] = "offscreen"
    environment["SEAMLY_DATA_DIR"] = str(directory / "data")
    fixture = directory / "pieces.svg"
    fixture.write_text(
        '<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">'
        '<g id="piece-1"><path d="M0 0 L96 0 L96 96 L0 96 Z" fill="none" stroke="black"/></g>'
        '<g id="piece-2"><path d="M0 0 L48 0 L48 48 L0 48 Z" fill="none" stroke="black"/></g></svg>',
        encoding="utf-8",
    )
    settings = directory / "tiled.json"
    settings.write_text(json.dumps({"paperType": "tiled"}), encoding="utf-8")
    failures = []

    def run(name, options, expected_status=0, input_bytes=None, program=None):
        """Record diagnostics and reject unexpected statuses or missing errors."""
        result = subprocess.run(
            [str(program or executable), *options],
            input=input_bytes,
            capture_output=True,
            timeout=60,
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
    for format_name in ("svg", "pdf", "pdf-tiled", "png", "jpg", "jpeg", "dxf", "hpgl", "ps", "eps"):
        output = directory / ("layout." + format_name)
        output.unlink(missing_ok=True)
        options = ["--export", format_name, "--output", str(output), str(fixture)]
        if format_name == "pdf-tiled":
            options.extend(["--settings", str(settings)])
        result = run(format_name, options)
        if result.returncode != 0 or not output.exists() or output.stat().st_size == 0:
            failures.append(format_name + "-output")
            continue
        content = output.read_bytes()
        if format_name in signatures and not content.startswith(signatures[format_name]):
            failures.append(format_name + "-signature")
        if format_name == "svg":
            root = xml.fromstring(content)
            if len(root.findall(".//{http://www.w3.org/2000/svg}path")) != 2:
                failures.append("svg-piece-count")
        if format_name == "dxf" and b"ENTITIES" not in content:
            failures.append("dxf-entities")
        if format_name == "hpgl" and b"PD" not in content:
            failures.append("hpgl-geometry")

    for format_name in ("gcode", "3mf"):
        run(format_name, ["--export", format_name, "--output", str(directory / format_name), str(fixture)], 69)
    run("scale-overflow", ["--export", "png", "--scale", "1e100", "--output",
                          str(directory / "overflow.png"), str(fixture)], 64)
    run("scale-underflow", ["--export", "png", "--scale", "1e-100", "--output",
                           str(directory / "underflow.png"), str(fixture)], 64)
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
    run("stdin", ["--export", "svg", "--svg-stdin", "--output", str(directory / "stdin.svg")],
        input_bytes=fixture.read_bytes())
    run("empty-stdin", ["--export", "svg", "--svg-stdin", "--output", str(directory / "empty.svg")],
        64, input_bytes=b"")
    run("help", ["--export", "svg", "--help"])
    if arguments.seamly2d:
        settings_directory = directory / "data" / "settings" / "Seamly" / "Seamly2D"
        settings_directory.mkdir(parents=True, exist_ok=True)
        (settings_directory / "qt6_seamly2d.ini").write_text(
            "[paths]\nseamlyLayoutApp=" + executable.as_posix() + "\n", encoding="utf-8"
        )
        environment.pop("HANDOFF_CAPTURE_DIRECTORY", None)
        companions = []
        for option, value in (("--mfile", arguments.measurements),
                              ("--gsize", arguments.size), ("--gheight", arguments.height)):
            if value is not None:
                companions.extend([option, str(value.resolve() if isinstance(value, Path) else value)])
        for format_name in ("svg", "pdf", "pdf-tiled", "png", "jpg", "jpeg",
                            "dxf", "hpgl", "ps", "eps", "gcode", "3mf"):
            output = directory / ("pass-through." + format_name)
            output.unlink(missing_ok=True)
            options = ["--layout-export", "--layout-format", format_name,
                       "--layout-output", str(output), *companions, str(arguments.pattern.resolve())]
            if format_name == "pdf-tiled":
                options.extend(["--layout-settings", str(settings)])
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
                                        str(arguments.pattern.resolve())], 64,
            program=arguments.seamly2d.resolve())
    if failures:
        print("FAILED: " + ", ".join(failures))
        return 1
    print("All headless export checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
