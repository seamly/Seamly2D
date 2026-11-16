# @file   test_cli_import_check.py
# @author slspencer
# @brief  Reads the DXF and PDF files of a CLI test run with independent libraries.
# @copyright Copyright (C) 2026 Seamly2D Project
# @license GPL-3.0-or-later
#
# The PowerShell suite checks file structure by hand. This script opens each DXF
# with ezdxf (a CAD library; its auditor reports what a CAD program would reject)
# and each PDF with pypdf. It does not replace an import into the target CAD or
# cutter software.
#
# With --inkscape <inkscape.com>, each DXF, PDF, SVG and HP-GL file is also imported into Inkscape
# (command line, no window) and saved as SVG; the import must give path elements.
#
# Usage: python test_cli_import_check.py <cli_test_report.json> [--inkscape <inkscape.com>]
# Exit:  0 every file opened clean; 1 a file failed; 2 a library or the report is missing.

import json
import os
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as xml
from pathlib import Path


def check_dxf(path: Path) -> list[str]:
    """Open a DXF with ezdxf, run its auditor, and count the pieces' blocks and entities."""
    import ezdxf
    from ezdxf import recover

    problems = []
    try:
        document, auditor = recover.readfile(str(path))
    except (IOError, ezdxf.DXFStructureError) as error:
        return [f"ezdxf cannot read it: {error}"]
    if auditor.has_errors:
        problems.append(f"ezdxf auditor: {len(auditor.errors)} errors, first: {auditor.errors[0].message}")
    entities = len(document.modelspace())
    blocks = [block.name for block in document.blocks if not block.name.startswith("*")]
    if entities == 0 and not blocks:
        problems.append("no entities and no blocks")
    # ezdxf loads R11-R14 files as R2000 in memory; the file header keeps the written version.
    lines = path.read_text(encoding="latin-1").splitlines()
    written = next((lines[i + 2].strip() for i, line in enumerate(lines[:-2]) if line.strip() == "$ACADVER"), "?")
    print(f"      file {written}, loaded as {document.dxfversion}, {entities} model space entities, {len(blocks)} blocks")
    return problems


def check_pdf(path: Path) -> list[str]:
    """Open a PDF with pypdf and read every page box and content stream."""
    import pypdf

    try:
        reader = pypdf.PdfReader(str(path), strict=True)
        pages = list(reader.pages)
        for page in pages:
            box = page.mediabox
            if float(box.width) <= 0 or float(box.height) <= 0:
                return [f"page with an empty MediaBox: {box}"]
            page.get_contents()
    except Exception as error:  # pypdf raises several error types; any of them fails the import
        return [f"pypdf cannot read it: {error}"]
    if not pages:
        return ["no pages"]
    print(f"      {len(pages)} pages")
    return []


def run_inkscape_hpgl_reader(inkscape: Path, path: Path, target: Path) -> subprocess.CompletedProcess:
    """Run Inkscape's HP-GL reader (hpgl2_input.py) with Inkscape's Python.

    The Windows build of Inkscape 1.4 ships the reader without its pyparsing module, so
    `inkscape file.hpgl` cannot open the file. pyparsing is pure Python: a copy from this
    Python goes on PYTHONPATH, alone, so no other module of this Python mixes in.
    """
    import pyparsing

    extensions = inkscape.parent.parent / "share" / "inkscape" / "extensions"
    with tempfile.TemporaryDirectory() as folder:
        shutil.copytree(Path(pyparsing.__file__).parent, Path(folder) / "pyparsing")
        environment = dict(os.environ, PYTHONPATH=folder)
        with open(target, "wb") as output:
            return subprocess.run([str(inkscape.parent / "python.exe"), "hpgl2_input.py", str(path)],
                                  cwd=extensions, stdout=output, stderr=subprocess.PIPE, env=environment, timeout=300)


def import_into_inkscape(inkscape: Path, path: Path, folder: Path) -> list[str]:
    """Import a file into Inkscape and save it as SVG; the result must hold path elements."""
    folder.mkdir(parents=True, exist_ok=True)
    target = folder / (path.stem + "_" + path.suffix.lstrip(".") + ".svg")
    target.unlink(missing_ok=True)
    try:
        if path.suffix.lower() == ".hpgl":
            run = run_inkscape_hpgl_reader(inkscape, path, target)
        else:
            run = subprocess.run([str(inkscape), str(path), "--export-type=svg", f"--export-filename={target}"],
                                 capture_output=True, timeout=300)
    except subprocess.TimeoutExpired:
        return ["Inkscape import timed out"]
    except ImportError:
        return ["HP-GL import needs pyparsing in this Python (python -m pip install pyparsing)"]
    if run.returncode != 0 or not target.is_file():
        return [f"Inkscape import failed (exit {run.returncode}): {run.stderr.decode(errors='replace')[-300:]}"]
    try:
        root = xml.parse(target).getroot()
    except xml.ParseError as error:
        return [f"Inkscape wrote no valid SVG: {error}"]
    paths = sum(1 for element in root.iter() if element.tag.endswith("}path") or element.tag == "path")
    print(f"      Inkscape: {paths} paths")
    return [] if paths > 0 else ["Inkscape imported no paths"]


def main() -> int:
    arguments = sys.argv[1:]
    inkscape = None
    if "--inkscape" in arguments:
        index = arguments.index("--inkscape")
        inkscape = Path(arguments[index + 1]) if index + 1 < len(arguments) else None
        arguments = arguments[:index] + arguments[index + 2:]
        if inkscape is None or not inkscape.is_file():
            print("--inkscape needs the path of inkscape.com")
            return 2
    if len(arguments) != 1 or not Path(arguments[0]).is_file():
        print("usage: test_cli_import_check.py <cli_test_report.json> [--inkscape <inkscape.com>]")
        return 2
    report_path = Path(arguments[0])
    try:
        import ezdxf  # noqa: F401
        import pypdf  # noqa: F401
    except ImportError as error:
        print(f"Missing library: {error.name}. Install with: python -m pip install ezdxf pypdf")
        return 2

    report = json.loads(report_path.read_text(encoding="utf-8-sig"))
    checkers = {".dxf": check_dxf, ".pdf": check_pdf}
    inkscape_formats = {".dxf", ".pdf", ".svg", ".hpgl"} if inkscape else set()
    inkscape_folder = report_path.with_name("inkscape_import")
    results = []
    for case in report["cases"]:
        output = case.get("output") or {}
        path = Path(output.get("path") or "")
        suffix = path.suffix.lower()
        # A failed run keeps an old file at its output path; only real exports are checked.
        exported = case.get("status") == "pass" and case.get("expectedExit") == 0 and output.get("exists")
        if not exported or suffix not in set(checkers) | inkscape_formats:
            continue
        print(f"{case['name']}: {path.name}")
        problems = checkers[suffix](path) if suffix in checkers else []
        if suffix in inkscape_formats:
            problems += import_into_inkscape(inkscape, path, inkscape_folder)
        status = "pass" if not problems else "fail"
        for problem in problems:
            print(f"      FAIL {problem}")
        results.append({"name": case["name"], "path": str(path), "status": status, "problems": problems})

    failed = sum(result["status"] == "fail" for result in results)
    summary = report_path.with_name("import_check_report.json")
    summary.write_text(json.dumps({"checked": len(results), "failed": failed, "files": results}, indent=2),
                       encoding="utf-8")
    print(f"Files: {len(results)}  failed: {failed}  report: {summary}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
