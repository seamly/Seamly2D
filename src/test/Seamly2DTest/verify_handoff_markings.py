# @file verify_handoff_markings.py
# @author slspencer
# @brief Checks live marked-pattern captures before downstream export tests.
# Copyright (C) 2026 Seamly2D Project
# SPDX-License-Identifier: GPL-3.0-or-later
# License: https://www.gnu.org/licenses/gpl-3.0.html

"""Verify the five real marked fixtures captured by the test application.

The original, horizontal, vertical, and both fixtures use the same live Yoke
anchors. The multisize fixture grades the Back anchors without scaling its
2 mm hole. Each fixture has one drill, one stripe, and a reversed plaid.
"""

import argparse
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as xml

from compare_handoff_writers import (
    IDENTITY, compose, entries, path_points, same_attribute, transform,
)


FIXTURES = ("marked-original", "marked-horizontal", "marked-vertical",
            "marked-both", "marked-multisize")
MARKS = {"drill", "stripe", "plaid"}
DIAMETER = 2 * 96 / 25.4


def unmarked(values):
    """Keep legacy geometry for comparison without expecting legacy marks."""
    skip = False
    result = []
    for entry in values:
        if entry[0] == "group":
            skip = entry[1].get("data-type") in MARKS
        if not skip:
            result.append(entry)
    return result


def verify_legacy_geometry(old, current):
    left, right = entries(old), unmarked(entries(current))
    assert len(left) == len(right), "legacy geometry or annotation count changed"
    for index, (a, b) in enumerate(zip(left, right)):
        assert a[0] == b[0] and a[1].keys() == b[1].keys(), f"legacy entry {index} changed"
        assert all(same_attribute(key, a[1][key], b[1][key]) for key in a[1]), (
            f"legacy entry {index} attributes changed")
        assert len(a[2]) == len(b[2]), f"legacy entry {index} topology changed"
        for p, q in zip(a[2], b[2]):
            assert p[0] == q[0], f"legacy entry {index} commands changed"
            assert p[1] is None or math.dist(p[1], q[1]) <= 0.01, (
                f"legacy entry {index} coordinates changed")


def inspect(filename):
    root = xml.parse(filename).getroot()
    units = {element.get("data-units") for element in root.iter() if element.get("data-type") == "pattern"}
    assert len(units) == 1, "inconsistent material pattern units"
    unit = units.pop()
    result = []
    for piece in root.iter():
        if piece.get("data-type") != "piece":
            continue
        components = {kind: [group for group in piece.iter() if group.get("data-type") == kind]
                      for kind in MARKS}
        if not any(components.values()):
            continue
        assert all(len(groups) == 1 for groups in components.values()), "missing or duplicated marking"
        drill = components["drill"][0]
        diameter = float(drill.get("data-diameter", "nan"))
        circles = [child for child in drill.iter() if child.tag.rsplit("}", 1)[-1] == "circle"]
        assert len(circles) == 1, "drill must contain one circle"
        circle = circles[0]
        center = tuple(float(circle.get(key, "nan")) for key in ("cx", "cy"))
        radius = float(circle.get("r", "nan"))
        assert all(math.isfinite(value) for value in (*center, diameter, radius)), "nonfinite drill"
        assert math.isclose(diameter, DIAMETER, abs_tol=1e-10), "graded drill diameter changed"
        assert math.isclose(radius * 2, diameter, abs_tol=1e-10), "circle radius disagrees with diameter"
        lines = {}
        matrices = {}
        for kind, groups in components.items():
            geometry = []

            def visit(element, parent):
                matrix = compose(parent, transform(element))
                name = element.tag.rsplit("}", 1)[-1]
                if name in ("circle", "path"):
                    geometry.append((element, matrix))
                for child in element:
                    visit(child, matrix)

            visit(groups[0], IDENTITY)
            assert len(geometry) == 1, f"{kind} must contain one primitive"
            element, matrices[kind] = geometry[0]
            if kind != "drill":
                points = path_points(element.get("d", ""), IDENTITY)
                assert [command for command, _ in points] == ["M", "L"], "reference topology changed"
                lines[kind] = [position for _, position in points]
        assert matrices["drill"] == matrices["stripe"] == matrices["plaid"], "mark transforms diverged"
        assert lines["stripe"] == list(reversed(lines["plaid"])), "plaid endpoints are not reversed"
        assert math.dist(center, lines["stripe"][0]) < 1e-10, "drill detached from live reference anchor"
        assert math.dist(*lines["stripe"]) > 0, "reference endpoints collapsed"
        result.append({"piece": piece.get("data-name"), "size": piece.get("data-size"), "units": unit,
                       "flip": piece.get("data-flip"), "center": center, "diameter": diameter,
                       "stripe": lines["stripe"], "matrix": matrices["drill"]})
    assert result, "no marked pieces captured"
    return result


def capture(executable, fixtures, output):
    root = Path(__file__).resolve().parents[3]
    measurements = root / "src/app/share/samples/measurements"
    environment = os.environ.copy()
    environment["QT_QPA_PLATFORM"] = "offscreen"
    environment["SEAMLY_DATA_DIR"] = str(output / "settings-data")
    for name in FIXTURES:
        target = output / name
        target.mkdir(parents=True, exist_ok=True)
        environment["HANDOFF_CAPTURE_DIRECTORY"] = str(target)
        measurement = (measurements / "multisize/gost_man_ru.smms" if name == "marked-multisize"
                       else measurements / "individual/male_chest_102cm.smis")
        for suffix in ("_old.svg", "_new.svg", "_aligned.svg", "_warnings.txt"):
            (target / (name + suffix)).unlink(missing_ok=True)
        command = [str(executable.resolve()), "--test", str((fixtures / (name + ".sm2d")).resolve()),
                   "--mfile", str(measurement)]
        with (target / "capture.log").open("w") as stream:
            result = subprocess.run(command, env=environment, stdout=stream, stderr=subprocess.STDOUT, timeout=180)
        if result.returncode:
            raise ValueError(f"{name}: producer exited {result.returncode}; see {target / 'capture.log'}")
        if not (target / (name + "_warnings.txt")).is_file():
            raise ValueError(f"{name}: producer did not complete capture")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--executable", type=Path)
    parser.add_argument("--fixtures", type=Path)
    args = parser.parse_args()
    if bool(args.executable) != bool(args.fixtures):
        parser.error("--executable and --fixtures must be supplied together")
    args.directory = args.directory.resolve()
    if args.executable:
        try:
            capture(args.executable, args.fixtures, args.directory)
        except (ValueError, OSError, subprocess.TimeoutExpired) as error:
            print(f"FAIL capture: {error}")
            return 1
    report = {}
    for name in FIXTURES:
        directory = args.directory / name
        old = directory / (name + "_old.svg")
        current = directory / (name + "_new.svg")
        aligned = directory / (name + "_aligned.svg")
        try:
            verify_legacy_geometry(old, current)
            marks = inspect(current)
            assert inspect(aligned) == marks, "annotation alignment moved production markings"
            if name == "marked-multisize":
                assert len({mark["size"] for mark in marks}) >= 2, "fewer than two graded sizes"
                assert len({tuple(mark["center"]) for mark in marks}) >= 2, "live centers did not grade"
            else:
                assert len(marks) == 1 and marks[0]["piece"] == "Yoke", "wrong marked piece"
                assert math.dist(marks[0]["center"], (0.4918828346456693, 135.5954645669291)) < 0.01, (
                    "live Yoke center changed")
                expected_flip = None if name == "marked-original" else name.removeprefix("marked-").title()
                assert marks[0]["flip"] == expected_flip, "piece flip metadata changed"
            warnings = (directory / (name + "_warnings.txt")).read_text().strip()
            report[name] = marks
            print(f"PASS {name}: {len(marks)} marked piece sizes; units {marks[0]['units']}; "
                  f"legacy geometry unchanged; "
                  f"warnings: {warnings or 'none'}")
        except (AssertionError, KeyError, ValueError, OSError, xml.ParseError) as error:
            print(f"FAIL {name}: {error}")
            return 1
    (args.directory / "marking-results.json").write_text(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
