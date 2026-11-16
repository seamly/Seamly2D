# CLI tester (`cli_test_app`)

Interactive and batch tester of the Seamly2D command-line layout export. Rust and Iced; no Qt.
It builds `seamly2d --layout-export` commands from your choices, runs them, checks the files,
and archives them. The choices come from `seamly2d --layout-info`, so the tester repeats no layout rule.

## Build

```powershell
cd src\test\CliTestApp
cargo build --release
cargo test
```

## Run

Seamly2D needs its Qt runtime on `PATH` and must find SeamlyLayout (`paths/seamlyLayoutApp`,
the install folder, or the build tree).

```powershell
target\release\cli_test_app.exe
target\release\cli_test_app.exe --pattern C:\patterns\jacket.sm2d --settings C:\settings\roll.json --next
```

Seamly2D program: `--seamly2d`, else `SEAMLY2D_EXE`, else beside the tester, else the build tree, else `PATH`.

## Screens

1. Inputs: Seamly2D program, pattern, measurements (optional), layout settings JSON (optional),
   output basename (default: pattern name), output folder (default: Preferences > Paths > Layout).
   Next reads the pattern with `--layout-info`. A piece wider than the media turns on Split the pieces.
2. Layout (multisize patterns only): Marker, Nested, Base Size, sizes. A split run lists Marker
   and Nested as unavailable.
3. Export format: formats that every material allows for the layout; the others with the reason.
   SVG text mode and DXF variant when they apply. One command per material, with Copy.
4. Results: per material the exit code, the file check, and the messages. The ZIP is made only
   when every material passed.

## Names

- Output: `<basename>_<material>_<YYMMDDHHMM>.<ext>`; `pattern` when the pattern names no material.
- Archive: `<basename>_<YYMMDDHHMM>.zip`. A DXF-ASTM `.rul` file goes into the archive too.
- One stamp per session. An existing name gets `_2`, `_3`, ...; no file is overwritten or deleted.

## Batch mode

Same steps, no window. Exit 0 when every material and the archive passed, 1 on a failed run, 2 on bad input.

```powershell
cli_test_app.exe --batch --pattern <file.sm2d> [--mfile <file>] [--settings <file.json>]
    [--basename <name>] [--output-dir <folder>] [--view <label>] [--format <format>]
    [--svg-text-mode <mode>] [--dxf r13|clo3d|aama] [--seamly2d <seamly2d.exe>]
```

Without `--view` and `--format` it takes the first allowed choice.
