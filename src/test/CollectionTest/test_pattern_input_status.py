# Author: slspencer
# Copyright: 2026 Seamly2D Project
# SPDX-License-Identifier: GPL-3.0-or-later
# https://www.gnu.org/licenses/gpl-3.0.html
"""Verify that CLI orchestration preserves the pattern loader's exit statuses."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess


def main():
    """Run the CollectionTest invalid-input matrix against an actual application."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output_directory", type=Path)
    arguments = parser.parse_args()
    directory = arguments.output_directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    fixtures = directory / "fixtures"
    shutil.copytree(Path(__file__).parent / "tst_seamly2d", fixtures, dirs_exist_ok=True)
    for issue in ("issue_256", "issue_1275"):
        shutil.copytree(Path(__file__).parent / "share" / issue, fixtures, dirs_exist_ok=True)
    (fixtures / "txt.sm2d").write_text("", encoding="utf-8")
    environment = os.environ.copy()
    environment["SEAMLY_DATA_DIR"] = str(directory / "data")
    environment["QT_QPA_PLATFORM"] = "offscreen"
    environment.pop("HANDOFF_CAPTURE_DIRECTORY", None)
    cases = (
        ("wrong-path", "wrongPath.sm2d", [], 66),
        ("invalid-object-type", "wrong_obj_type.sm2d", [], 66),
        ("empty-text", "txt.sm2d", [], 66),
        ("wrong-measurement-path", "issue_256_wrong_path.smis", [], 66),
        ("wrong-individual", "issue_256.sm2d", ["-m", str(fixtures / "issue_256_wrong.smis")], 66),
        ("wrong-multisize", "issue_256.sm2d", ["-m", str(fixtures / "issue_256_wrong.smms")], 66),
        ("missing-measurement", "issue_1275.sm2d", ["-m", str(fixtures / "issue_1275.smis")], 66),
        ("valid-empty-pattern", "empty.sm2d", [], 0),
        ("valid-measurements", "issue_256.sm2d", [], 0),
        ("valid-individual", "issue_256.sm2d", ["-m", str(fixtures / "issue_256_correct.smis")], 0),
        ("valid-multisize", "issue_256.sm2d", ["-m", str(fixtures / "issue_256_correct.smms")], 0),
        ("wrong-formula", "wrong_formula.sm2d", [], 65),
    )
    failures = []
    for mode in ("test", "layout"):
        for name, filename, companions, expected in cases:
            if mode == "layout" and expected in (0, 65):
                continue
            options = ["--test"] if mode == "test" else [
                "--layout-export", "--layout-output", str(directory / (name + ".svg")),
            ]
            result = subprocess.run(
                [str(arguments.executable.resolve()), *options, *companions, str(fixtures / filename)],
                capture_output=True, timeout=30, env=environment,
            )
            (directory / (mode + "-" + name + ".stderr.txt")).write_bytes(result.stderr)
            valid = result.returncode == expected and (expected == 0 or bool(result.stderr.strip()))
            if not valid:
                failures.append(mode + "-" + name)
            print(f"{mode}-{name}: status={result.returncode}, expected={expected}, "
                  f"{'PASS' if valid else 'FAIL'}")
    if failures:
        print("FAILED: " + ", ".join(failures))
        return 1
    print("All pattern input status checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
