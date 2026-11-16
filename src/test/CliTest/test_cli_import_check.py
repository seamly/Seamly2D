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
# Usage: python test_cli_import_check.py <cli_test_report.json>
# Exit:  0 every file opened clean; 1 a file failed; 2 a library or the report is missing.

import json
import sys
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


def main() -> int:
    if len(sys.argv) != 2 or not Path(sys.argv[1]).is_file():
        print("usage: test_cli_import_check.py <cli_test_report.json>")
        return 2
    try:
        import ezdxf  # noqa: F401
        import pypdf  # noqa: F401
    except ImportError as error:
        print(f"Missing library: {error.name}. Install with: python -m pip install ezdxf pypdf")
        return 2

    report = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8-sig"))
    checkers = {".dxf": check_dxf, ".pdf": check_pdf}
    results = []
    for case in report["cases"]:
        output = case.get("output") or {}
        path = Path(output.get("path") or "")
        if case.get("status") != "pass" or not output.get("exists") or path.suffix.lower() not in checkers:
            continue
        print(f"{case['name']}: {path.name}")
        problems = checkers[path.suffix.lower()](path)
        status = "pass" if not problems else "fail"
        for problem in problems:
            print(f"      FAIL {problem}")
        results.append({"name": case["name"], "path": str(path), "status": status, "problems": problems})

    failed = sum(result["status"] == "fail" for result in results)
    summary = Path(sys.argv[1]).with_name("import_check_report.json")
    summary.write_text(json.dumps({"checked": len(results), "failed": failed, "files": results}, indent=2),
                       encoding="utf-8")
    print(f"Files: {len(results)}  failed: {failed}  report: {summary}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
