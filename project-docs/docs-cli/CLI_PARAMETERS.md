# Calling Seamly2D from the command line

Usage: `seamly2d [options] filename`

| Argument | Definition |
|---|---|
| `filename` | Pattern file (`.sm2d`). |

## Options

| Option | Definition |
|---|---|
| `-h`, `--help` | Show the help text, exit 0. |
| `-v`, `--version` | Show the version, exit 0. |
| `-t`, `--test` | Test mode: load one pattern file, then quit without showing the main window. |
| `-m`, `--mfile <file>` | Measurement file (`.smis`, `.smms`) for test mode, layout export and layout info. Replaces the file the pattern names. |
| `-x`, `--gsize <cm>` | Size of a multisize pattern. Exports that one size. |
| `-e`, `--gheight <cm>` | Height of a multisize pattern. Exports that one height. |
| `--layout-export` | Lay out the pattern in SeamlyLayout and export it. No window opens. |
| `--layout-info` | Print a JSON report of materials, layouts, oversized pieces and formats. Exports nothing. |
| `--layout-format <format>` | `svg` (default), `pdf`, `pdf-tiled`, `png`, `jpg`, `jpeg`, `dxf`, `hpgl`, `ps`, `eps`, `gcode`, `3mf`. |
| `--layout-output <path>` | Export file. Required by `--layout-export`. |
| `--layout-settings <path>` | Layout settings JSON (the Layout Settings dialog). Default: the settings SeamlyLayout saved last. |
| `--layout-export-options <path>` | Format options JSON. DXF: `dxfVersion` (`R13`, `R11`), `aama`, `clo3dGroup250`, `createAnnotatedVersion`. HP-GL: `mode` (`plot`, `cut`), `cutPen`, `markPen`, `labelPen`. |
| `--svg-text-mode <mode>` | SVG labels: `designerFont` (default), `singleLineFont`, `hersheyStrokes` (stroke paths), `asSupplied`. SVG only. |
| `--layout-material <name>` | Material to export, such as `Fabric`. Required when the pattern has several materials. Not case-sensitive. |
| `--layout-view <name>` | Multisize layout: `marker`, `nested`, `base`, a size as its tab shows it (`500` for "Size 500"), or the full tab label. Default: the first layout. |
| `--layout-oversize <choice>` | Pieces wider than a roll or fabric: `split` (split the pieces) or `extend` (make the media longer). Default: stop with exit 70. |
| `--no-scaling` | Turn off high DPI scaling. Same as `QT_AUTO_SCREEN_SCALE_FACTOR=0`. |

Option names are case-sensitive. Each long option also accepts `--name=value`.

## Workflow

`--layout-export` and `--layout-info` load one pattern, apply `--mfile`, `--gsize` and `--gheight`,
and build the tagged pieces SVG in memory, as Layout mode does. Seamly2D starts SeamlyLayout
(`paths/seamlyLayoutApp`, else the install folder, else the build tree) with `--svg-stdin` and
writes the SVG to its standard input. No intermediate file is written. Seamly2D waits for
SeamlyLayout and returns its exit code. SeamlyLayout messages go to standard error.

- A multisize pattern without `--gsize` and `--gheight` hands off every size at the base height.
  SeamlyLayout makes the Marker, Nested, Base Size and Size layouts; `--layout-view` selects one.
- With `--gsize` or `--gheight` the handoff holds one size and height, with no views.
- SeamlyLayout lays out every material and view before it exports, as Create Layout does.
  An oversized piece in any material therefore needs `--layout-oversize`, also when another material is exported.
- An unplaced piece in any layout stops the export with exit 70 and lists the pieces.

```text
seamly2d --layout-export --layout-format pdf --layout-output C:\exports\jacket_Fabric.pdf ^
  --layout-settings C:\settings\roll_36in.json --layout-material Fabric --layout-view marker ^
  C:\patterns\jacket.sm2d

seamly2d --layout-export --layout-format pdf-tiled --layout-output shirt.pdf ^
  --layout-settings letter.json --mfile measurements\shirt.smis patterns\shirt.sm2d

seamly2d --layout-export --layout-format pdf --layout-oversize split ^
  --layout-settings roll_12in.json --layout-output shirt.pdf shirt.sm2d

seamly2d --layout-info --layout-settings roll_36in.json jacket.sm2d
```

## Paths

- Relative paths resolve against the launch folder: pattern, `--mfile`, `--layout-output`,
  `--layout-settings`, `--layout-export-options`.
- The output folder must exist.

## Valid and invalid combinations

| Combination | Result |
|---|---|
| `--test` with `--layout-export` or `--layout-info` | exit 64 |
| `--layout-export` with `--layout-info` | exit 64 |
| A `--layout-*` option or `--svg-text-mode` without `--layout-export` | exit 64 (`--layout-settings` also serves `--layout-info`) |
| `--layout-info` with an export choice (format, output, material, view, oversize, text mode, export options) | exit 64 |
| `--layout-export` without `--layout-output`, or with no or two patterns | exit 64 |
| `--svg-text-mode` with a format other than `svg` | exit 64 |
| `--layout-view` with `--gsize` or `--gheight` | exit 64 |
| `--layout-view` on an individual pattern, or a view the layout does not have | exit 64; the message lists the views |
| No `--layout-material` on a pattern with several materials, or an unknown material | exit 64; the message lists the materials |
| `--gsize` or `--gheight` on an individual pattern | exit 65 |
| `pdf` on tiled paper | exit 64: use `pdf-tiled` |
| `pdf-tiled` on a roll or fabric | exit 64 |
| `pdf-tiled` on the Marker layout | exit 64 |
| `dxf` on the Marker or Nested layout | exit 64: DXF exports one size |
| `dxf`, `pdf-tiled`, `gcode`, `3mf` with `--layout-oversize split` | exit 64 |
| `--layout-view marker` or `nested` with `--layout-oversize split` | exit 64 |
| `gcode`, `3mf` | exit 69 while their modules are not in the build |
| `--layout-oversize extend` with a piece wider than the media | exit 70: the piece cannot be placed |

`--layout-info` reports the allowed formats of each layout, so a caller can choose without a failed run.

## Layout settings JSON

Keys and values of `--layout-settings`. A missing key keeps its default.
An unknown key or an invalid value stops the export with exit 64 and names the key.

| Key | Values | Default |
|---|---|---|
| `layoutMode` | `alongGrainline`, `withNap`, `any` | `alongGrainline` |
| `rotationStep` | `withNap`: `0` (pieces point up), `180` (down) | `0` |
| `noGrainlineRotation` | `upright`, `free` | `upright` |
| `pieceGap` | number ≥ 0, in `unit` | `0.05` |
| `scaleGrid` | `true`, `false` | `false` |
| `unit` | `in`, `cm`, `mm` | `in` |
| `mediaType` | `paper`, `fabric` | `paper` |
| `foldLinesOnLeftEdge` | `true`, `false` | `true` |
| `paperType` | `tiled`, `roll` | `tiled` |
| `tileSize` | `Letter`, `Legal`, `Ledger`, `ANSI C`–`ANSI E`, `ARCH A`–`ARCH E3`, `A0`–`A5`, `B0`–`B6`, `Custom` | `Letter` |
| `tileWidth`, `tileHeight` | number > 0, for `Custom` | `8.5`, `11` |
| `tileOrientation` | `landscape`, `portrait` | `landscape` |
| `rollSize` | `custom`, or width and unit (`36 in`, `900 mm`) | `36 in` |
| `rollWidth` | number ≥ 0, for `custom` | `36` |
| `fabricFolded` | `true`, `false` | `false` |
| `fabricWidth`, `fabricHeight` | number ≥ 0; width `0` uses `pageWidth`, height `0` has no end | `0` |
| `pageWidth`, `pageHeight` | number > 0 | `36`, `48` |
| `selvedgeWidth` | number ≥ 0 | `0` |
| `marginTop`, `marginLeft`, `marginRight`, `marginBottom` | number ≥ 0 | `0.25` |

`CLI_1_OPTION_MATRIX.md` in the repository root maps each Layout Settings control to its key.

## Exit codes

| Code | Meaning |
|---|---|
| 0 | Success. The output file exists and passed its format's write. |
| 64 | Usage: unknown option, missing companion, invalid value, combination the Export menu disables. |
| 65 | Data: the pattern cannot be calculated; `--gsize` on an individual pattern. |
| 66 | Input: pattern or measurement file missing or unreadable. |
| 69 | Unavailable: SeamlyLayout cannot start; paid format not in the build. |
| 70 | Layout: oversized pieces without `--layout-oversize`; unplaced pieces; SeamlyLayout crashed. |
| 73 | Output: the file cannot be written. |
| 78 | Configuration: `SEAMLY_DATA_DIR` is relative. |

- A run that stops before the export keeps an existing file at `--layout-output`.
- SeamlyLayout removes the old file just before it writes, and removes a partial file after a failed write.
- Seamly2D removes the output file after a SeamlyLayout crash.

## Windows console

Seamly2D is a GUI-subsystem program. With a console option it attaches to the parent console,
so help and errors print without redirection. Command shells do not wait for a GUI program:

- `cmd`: `start /wait seamly2d ...`, then `echo %ERRORLEVEL%`.
- PowerShell: `$p = Start-Process seamly2d -ArgumentList ... -Wait -PassThru; $p.ExitCode`,
  or pipe the output (`seamly2d ... | Out-Host`), which waits.
- With redirection (`> out.txt 2> err.txt`) the streams go to the files.

A normal start (no console option) does not attach to a console.

## Removed legacy options

These options of the old Seamly2D writer stay removed and fail with exit 64 and "Unknown option":
`-b`/`--basename`, `-d`/`--destination`, `-f`/`--format`, `--bdxf`, `--text2paths`,
`--exportOnlyDetails`, `-p`/`--pageformat`, `-W`/`--pagew`, `-H`/`--pageh`, `-U`/`--pageunits`,
`-i`/`--ignoremargins`, `-L`/`--lmargin`, `-R`/`--rmargin`, `-T`/`--tmargin`, `-B`/`--bmargin`,
`-r`/`--rotate`, `-c`/`--crop`, `-u`/`--unite`, `-S`/`--savelen`, `-l`/`--layounits`,
`-s`/`--shiftlen`, `-G`/`--gapwidth`, `-g`/`--groups`.

- `--text2paths`: `--svg-text-mode hersheyStrokes` writes labels as stroke paths in SVG.
- `--layout-scale` is removed: PNG export renders at 96 px per inch.
- Page, margin and gap options: use the `--layout-settings` keys above.

## SeamlyLayout headless options

Seamly2D maps its options to these. They also work on a saved pieces SVG.

| Seamly2D | SeamlyLayout |
|---|---|
| `--layout-format` | `--export` (alias `--format`) |
| `--layout-output` | `--output` (alias `--destination`) |
| `--layout-settings` | `--settings` (alias `--layout-settings`) |
| `--layout-export-options` | `--export-options` |
| `--svg-text-mode` | `--svg-text-mode` (alias `--text-mode`) |
| `--layout-material` | `--material` |
| `--layout-view` | `--view` |
| `--layout-oversize` | `--oversize` |
| `--layout-info` | `--info` |

```text
seamlylayout --export dxf --output out.dxf --settings roll.json pieces.svg
seamlylayout --info --settings roll.json pieces.svg
```

## Automated tests

- Unit: `TST_VCommandLine` (Seamly2D options), `StartupOptionsTests` (SeamlyLayout options),
  `cargo test` (`settings_validation`, `export_rules`).
- Command line: `src\test\CliTest\test_cli_layout_export.ps1 -FixtureRoot <test-seamly-layout> -QtBin <Qt bin>`.
  It runs Seamly2D and SeamlyLayout for every option group and checks exit codes, messages and
  file structure (SVG size and pieces, PDF page size and count, DXF version, PNG pixels).
  The JSON report goes to `build\cli-tests\<stamp>\cli_test_report.json`.
- Interactive: `src\test\CliTestApp` (Rust, Iced). See its `README.md`.
- Older runners: `src\test\SeamlyLayoutTest\test_headless_export.py`,
  `src\test\CollectionTest\test_pattern_input_status.py`, `src\test\Seamly2DTest\test_measurement_cli.py`.
