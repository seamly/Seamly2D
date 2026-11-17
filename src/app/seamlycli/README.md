# SeamlyCLI

Interactive and batch tester of the Seamly2D command-line layout export. Rust and Iced; no Qt.
It builds `seamly2d --layout-export` commands from your choices, runs them, checks the files,
and archives them. The choices come from `seamly2d --layout-info`, so the tester repeats no layout rule.

## Build

```powershell
cd src\app\seamlycli
cargo build --release
cargo test
```

## Run

Seamly2D needs its Qt runtime on `PATH` and must find SeamlyLayout (`paths/seamlyLayoutApp`,
the install folder, or the build tree).

```powershell
target\release\seamlycli.exe
target\release\seamlycli.exe --pattern C:\patterns\jacket.sm2d --settings C:\settings\roll.json --next
```

SeamlyCLI finds Seamly2D through PATH. The MSI adds its application folder to the system PATH.
Batch tests can use --seamly2d to name a specific build.

## Screens

1. Inputs: pattern, measurements (optional), editable layout settings or a settings JSON file,
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
seamlycli.exe --batch --pattern <file.sm2d> [--mfile <file>] [--settings <file.json>]
    [--basename <name>] [--output-dir <folder>] [--view <label>] [--format <format>]
    [--svg-text-mode <mode>] [--dxf r13|clo3d|aama] [--seamly2d <seamly2d.exe>]
```

Without `--view` and `--format` it takes the first allowed choice.

## Multisize patterns

SeamlyCLI adds --basesize when Seamly2D requires a multisize selection.
Select Base Size on the Layout screen to export the measurement base size.
Select another layout to export that view.
Individual patterns use no size flag.

## Windows installer

The Windows x64 MSI builds and installs SeamlyCLI beside Seamly2D, SeamlyMe, and SeamlyLayout.
Run packaging/windows/local_build_msi.ps1 from the repository root to build the installer.
The MSI includes a Start Menu shortcut and an optional desktop shortcut.
SeamlyCLI uses Rust and Iced. It needs no Qt to build or display its interface.
The Seamly2D and SeamlyLayout processes use the shared Qt runtime in the MSI.

## Input folders and layout settings

The pattern picker opens in Preferences > Paths > Patterns.
The measurement picker opens in the individual or multisize measurement folder from Preferences.
SeamlyCLI uses the selected pattern or measurement file to choose that folder.

Enter units, packing, paper, tiles, roll width, fabric, and margins in the layout settings controls.
Select a settings JSON file to use its values and disable the controls.
Select Use entered settings to clear the file selection and enable the controls.
SeamlyCLI writes entered settings to a temporary JSON file for report and export commands.
