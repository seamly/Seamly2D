# Author: slspencer
# Copyright: 2026 Seamly2D Project
# SPDX-License-Identifier: GPL-3.0-or-later
# https://www.gnu.org/licenses/gpl-3.0.html
"""Validate bounded SeamlyMe measurement-load and argument-failure behavior."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess


def main():
    """Keep test-mode CLI validation isolated from user settings and dialogs."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output_directory", type=Path)
    parser.add_argument("--individual", type=Path, required=True)
    parser.add_argument("--multisize", type=Path, required=True)
    parser.add_argument("--pattern", type=Path, required=True)
    arguments = parser.parse_args()
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
    environment["SEAMLY_DATA_DIR"] = str(directory / "data")
    environment["QT_QPA_PLATFORM"] = "offscreen"
    inputs = directory / "inputs"
    inputs.mkdir(parents=True, exist_ok=True)
    for source in (arguments.individual, arguments.multisize, arguments.pattern):
        shutil.copy2(source, inputs / source.name)
    individual = str(inputs / arguments.individual.name)
    multisize = str(inputs / arguments.multisize.name)
    cases = (
        ("individual", ["--test", individual], 0, None),
        ("multisize", ["--test", "--height", "146", "--size", "40", "--unit", "mm", multisize], 0, None),
        ("relative-individual", ["--test", arguments.individual.name], 0, None),
        ("relative-multisize", ["--test", "--height", "146", "--size", "40", "--unit", "mm",
                               arguments.multisize.name], 0, None),
        ("relative-missing-input", ["--test", "missing.smis"], 66, b"doesn't exist"),
        ("pattern-rejected", ["--test", str(inputs / arguments.pattern.name)], 66, b"unknown format"),
        ("invalid-height", ["--test", "--height", "1", multisize], 64, b"height"),
        ("invalid-size", ["--test", "--size", "1", multisize], 64, b"size"),
        ("invalid-unit", ["--test", "--unit", "invalid", multisize], 64, b"unit"),
        ("missing-value", ["--test", "--height"], 64, b"height"),
        ("unknown-option", ["--test", "--unknown"], 64, b"unknown"),
        ("unknown-before-test", ["--unknown", "--test"], 64, b"unknown"),
        ("missing-input", ["--test"], 64, b"input"),
        ("multiple-inputs", ["--test", individual, multisize], 64, b"several"),
        ("help", ["--test", "--help"], 0, None),
        ("version", ["--test", "--version"], 0, None),
    )
    failures = []
    for name, options, expected, diagnostic in cases:
        process = subprocess.Popen(
            [str(arguments.executable.resolve()), *options],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=environment, cwd=inputs,
        )
        try:
            stdout, stderr = process.communicate(timeout=20)
        except subprocess.TimeoutExpired:
            process.kill()
            stdout, stderr = process.communicate()
            failures.append(name + "-timeout")
        (directory / (name + ".stdout.txt")).write_bytes(stdout)
        (directory / (name + ".stderr.txt")).write_bytes(stderr)
        valid = process.returncode == expected and (diagnostic is None or diagnostic in stderr.lower())
        if not valid:
            failures.append(name)
        print(f"{name}: status={process.returncode}, expected={expected}, {'PASS' if valid else 'FAIL'}")
    if failures:
        print("FAILED: " + ", ".join(failures))
        return 1
    print("All measurement CLI checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
