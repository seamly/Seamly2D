# @file capture_handoff_writers.py
# @author slspencer
# @brief Captures both writers from real sample and CollectionTest patterns.
# Copyright (C) 2026 Seamly2D Project
# SPDX-License-Identifier: GPL-3.0-or-later
# License: https://www.gnu.org/licenses/gpl-3.0.html

"""Run a capture-enabled Seamly2D test application against all valid patterns.

The test application reads HANDOFF_CAPTURE_DIRECTORY after LoadPattern, builds
the same size lists as Layout Mode, and writes _old.svg, _new.svg, _aligned.svg,
and _warnings.txt. This hook belongs in the test build, not the shipped app.
The shell must provide the current Qt DLLs and plugin path.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as xml


COLLECTION = (
    "bra/bra", "suit/pants1_52-176", "suit/pants2_40-146", "suit/pants7",
    "tshirt_test", "test_dart", "basic_block_women-2016", "gent_jacket_with_tummy",
    "steampunk_trousers/steampunk_trousers", "patron_blusa/patron_blusa",
    "susan/pajama_top_wrap2", "keiko_skirt/keiko_skirt", "pantaloon/pantalon_base_Eli",
    "modell/modell_2", "imk_zhaketa/imk_zhaketa_poluprilegayuschego_silueta",
    "moulage/moulage_05_armhole_neckline", "moulage/07_armhole_adjustment_010",
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("--output", type=Path, default=Path("build") / "writer-captures")
    parser.add_argument("--timeout", type=int, default=180)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    executable = args.executable.resolve()
    output = args.output.resolve()
    patterns = [("samples", path) for path in sorted(
        (root / "src/app/share/samples/patterns").glob("*.sm2d"))]
    patterns += [("collection", root / "src/test/CollectionTest/share" / (name + ".sm2d"))
                 for name in COLLECTION]
    environment = os.environ.copy()
    environment["SEAMLY_DATA_DIR"] = str(output / "settings-data")
    environment["QT_QPA_PLATFORM"] = "offscreen"
    results = []
    for category, pattern in patterns:
        target = output / category / pattern.stem
        target.mkdir(parents=True, exist_ok=True)
        environment["HANDOFF_CAPTURE_DIRECTORY"] = str(target)
        command = [str(executable), "--test", str(pattern)]
        measurements = xml.parse(pattern).getroot().findtext("measurements")
        if measurements and not (pattern.parent / measurements).is_file():
            basename = Path(measurements.replace("\\", "/")).name
            candidates = list((root / "src/app/share/samples/measurements").rglob(basename))
            if len(candidates) != 1:
                print(f"FAIL {pattern.name}: measurement override is not unique: {basename}")
                return 1
            command += ["--mfile", str(candidates[0])]
        for suffix in ("_old.svg", "_new.svg", "_aligned.svg", "_warnings.txt"):
            (target / (pattern.stem + suffix)).unlink(missing_ok=True)
        try:
            with (target / "capture.log").open("w") as stream:
                status = subprocess.run(command, env=environment, stdout=stream,
                                        stderr=subprocess.STDOUT, timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:
            status = "TIMEOUT"
        captured = all((target / (pattern.stem + suffix)).is_file()
                       for suffix in ("_old.svg", "_new.svg", "_aligned.svg", "_warnings.txt"))
        print(f"{'PASS' if status == 0 and captured else 'FAIL'} {category}/{pattern.name}: "
              f"exit {status}, capture complete {captured}", flush=True)
        results.append({"pattern": str(pattern.relative_to(root)), "exit": status, "captured": captured})
    (output / "capture-results.json").write_text(json.dumps(results, indent=2))
    return int(any(result["exit"] != 0 or not result["captured"] for result in results))


if __name__ == "__main__":
    sys.exit(main())
