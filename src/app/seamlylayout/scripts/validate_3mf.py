#!/usr/bin/env python3
import argparse
import sys
from pathlib import Path


def load_model(path, strict_mode):
    try:
        import lib3mf
    except ImportError as exc:
        raise RuntimeError(
            "lib3mf is not installed. Install with: pip install lib3mf"
        ) from exc

    wrapper = lib3mf.get_wrapper()
    model = wrapper.CreateModel()
    reader = model.QueryReader("3mf")
    reader.SetStrictModeActive(strict_mode)
    reader.ReadFromFile(str(path))

    warnings = []
    for i in range(reader.GetWarningCount()):
        code, message = reader.GetWarning(i)
        warnings.append((code, message))

    return model, warnings


def validate_model(model):
    errors = []
    warnings = []

    build_items = model.GetBuildItems()
    build_count = build_items.Count()
    if build_count == 0:
        errors.append("No build items found (missing <build>/<item> entries).")

    objects = model.GetObjects()
    object_count = objects.Count()
    if object_count == 0:
        errors.append("No objects found in model resources.")

    while objects.MoveNext():
        obj = objects.GetCurrentObject()
        if not obj.IsValid():
            name = obj.GetName() or "(unnamed)"
            part = obj.GetPartNumber() or "(no part number)"
            errors.append(f"Invalid object: name={name}, part={part}.")

    meshes = model.GetMeshObjects()
    while meshes.MoveNext():
        mesh = meshes.GetCurrentMeshObject()
        if mesh.GetTriangleCount() == 0:
            warnings.append((None, "Mesh has zero triangles."))
        if not mesh.IsManifoldAndOriented():
            warnings.append((None, "Mesh is not manifold and oriented."))

    return errors, warnings


def main():
    parser = argparse.ArgumentParser(
        description="Validate a 3MF file using lib3mf."
    )
    parser.add_argument("path", type=Path, help="Path to .3mf file")
    parser.add_argument(
        "--no-strict",
        dest="strict",
        action="store_false",
        help="Disable lib3mf strict mode when reading",
    )
    parser.add_argument(
        "--warnings-as-errors",
        action="store_true",
        help="Exit non-zero if warnings are present",
    )
    args = parser.parse_args()

    if not args.path.exists():
        print(f"File not found: {args.path}", file=sys.stderr)
        return 2

    try:
        model, read_warnings = load_model(args.path, args.strict)
    except Exception as exc:
        print(f"Failed to read 3MF: {exc}", file=sys.stderr)
        return 2

    errors, warnings = validate_model(model)
    warnings = read_warnings + warnings

    if warnings:
        print("Warnings:")
        for code, message in warnings:
            if code is None:
                print(f"  - {message}")
            else:
                print(f"  - [{code}] {message}")

    if errors:
        print("Errors:")
        for message in errors:
            print(f"  - {message}")
        return 2

    if warnings and args.warnings_as_errors:
        return 1

    print("Validation OK.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
