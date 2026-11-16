# Calling Seamly2D from the command line

Usage: `seamly2d [options] filename`

| Argument | Definition |
|---|---|
| `filename` | Pattern file (`.sm2d`). |

## Help menu

Select **Help > Command line options** to open the read-only option tables.
The window uses the application font and colors. Tabs separate options, limits, exit codes, and removed options.
Drag column boundaries to adjust their widths. Use the filter to find matching options and descriptions.
Select **Close** to return to the pattern without changing the command line mode.
The same information appears with `--help`.

## Options

| Option | Definition |
|---|---|
| `-h`, `--help` | Show the help text, exit 0. |
| `-v`, `--version` | Show the version, exit 0. |
| `-t`, `--test` | Test mode: load one pattern file, then quit without showing the main window. |
| `-m`, `--mfile <file>` | Measurement file (`.smis`, `.smms`) for test mode, layout export and layout info. Replaces the file the pattern names. |
| `--basesize` | Select the base size from the multisize measurement file. Takes no argument. Cannot accompany `--gsize`. |
| `-x`, `--gsize <cm>` | Select a numeric multisize size. Cannot accompany `--basesize`. |
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
| `--text2paths` | Label text as paths: SVG labels as single-stroke paths (as `--svg-text-mode hersheyStrokes`), PDF and tiled PDF text as glyph outlines (no embedded font). PS and EPS draw paths already. Formats `svg`, `pdf`, `pdf-tiled`, `ps`, `eps`. |
| `--layout-no-packing` | Export the pieces where Piece mode places them, without a layout run. Not with `--layout-view`, `--layout-oversize` or `pdf-tiled`. |
| `--layout-<name> <value>` | Direct layout flags; see "Direct layout flags". They win over `--layout-settings`. |
| `--no-scaling` | Turn off high DPI scaling. Same as `QT_AUTO_SCREEN_SCALE_FACTOR=0`. |

Option names are case-sensitive. Each long option also accepts `--name=value`.

## Workflow

`--layout-export` and `--layout-info` load one pattern, apply `--mfile`, `--gsize` or `--basesize`, and `--gheight`,
and build the tagged pieces SVG in memory, as Layout mode does. Seamly2D starts SeamlyLayout
(`paths/seamlyLayoutApp`, else the install folder, else the build tree) with `--svg-stdin` and
writes the SVG to its standard input. No intermediate file is written. Seamly2D waits for
SeamlyLayout and returns its exit code. SeamlyLayout messages go to standard error.

- Multisize console runs require --gsize <cm> or --basesize. Both options together cause exit 64.
- --basesize reads the size from the active measurement file, including a --mfile override.
  Seamly2D converts the measurement unit before it selects the size.
- --basesize exports only the base size unless --layout-view selects a multisize view.
  --layout-info still reports every available view.
- --gsize selects one numeric size. --gheight can accompany either size option.
- --basesize on an individual pattern causes exit 65.
- GUI file opening does not require a size option.
- SeamlyLayout lays out every material and view before it exports, as Create Layout does.
  An oversized piece in any material therefore needs `--layout-oversize`, also when another material is exported.
- An unplaced piece in any layout stops the export with exit 70 and lists the pieces.

```text
seamly2d --layout-export --layout-format pdf --layout-output C:\exports\jacket_Fabric.pdf ^
  --layout-settings C:\settings\roll_36in.json --layout-material Fabric --basesize --layout-view marker ^
  C:\patterns\jacket.sm2d

seamly2d --layout-export --layout-format pdf-tiled --layout-output shirt.pdf ^
  --layout-settings letter.json --mfile measurements\shirt.smis patterns\shirt.sm2d

seamly2d --layout-export --layout-format pdf --layout-oversize split ^
  --layout-settings roll_12in.json --layout-output shirt.pdf shirt.sm2d

seamly2d --layout-info --basesize --layout-settings roll_36in.json jacket.sm2d
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
| `--text2paths` with `--svg-text-mode`, or with `png`, `jpg`, `jpeg`, `dxf`, `hpgl`, `gcode`, `3mf` | exit 64 |
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

## Direct layout flags

Each flag sets keys of the settings above. Precedence: saved settings, `--layout-settings`, then the flags.
They also work with `--layout-info`.

| Flag | Value | Keys | Implies |
|---|---|---|---|
| `--layout-unit` | `in`, `cm`, `mm` | `unit` | |
| `--layout-media` | `paper`, `fabric` | `mediaType` | |
| `--layout-paper` | `tiled`, `roll` | `paperType` | paper |
| `--layout-tile-size` | catalog name or `Custom` | `tileSize` | tiled paper |
| `--layout-tile-orientation` | `portrait`, `landscape` | `tileOrientation` | tiled paper |
| `--layout-tile-width`, `--layout-tile-height` | number | `tileWidth`, `tileHeight`; `tileSize` Custom | tiled paper |
| `--layout-roll-width` | number | `rollSize` custom, `rollWidth` | paper roll |
| `--layout-fabric-width` | number | `fabricWidth` | fabric |
| `--layout-fabric-folded` | `yes`, `no` | `fabricFolded` | fabric |
| `--layout-selvedge-width` | number | `selvedgeWidth` | fabric |
| `--layout-margins` | number | all four margins | |
| `--layout-margin-top`, `-left`, `-right`, `-bottom` | number | one margin; wins over `--layout-margins` | |
| `--layout-ignore-margins` | none | every margin 0 | |
| `--layout-gap` | number | `pieceGap` | |
| `--layout-mode` | `alongGrainline`, `withNap`, `any` | `layoutMode` | |
| `--layout-nap` | `up`, `down` | `rotationStep` 0 or 180 | `layoutMode` withNap |
| `--layout-no-grainline` | `upright`, `free` | `noGrainlineRotation` | |

Exit 64 for: paper and fabric flags together; tile flags with a roll; a tile width with a named tile size;
`--layout-ignore-margins` with a margin flag; `--layout-nap` with a mode other than withNap; a flag twice;
a value that is not a number or not a listed value.

```text
seamly2d --layout-export --layout-format pdf --layout-roll-width 24 --layout-margins 0.5 --layout-output shirt.pdf shirt.sm2d
seamly2d --layout-export --layout-unit cm --layout-fabric-width 150 --layout-fabric-folded yes --layout-nap down ^
  --layout-output shirt.svg shirt.sm2d
```

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
`-b`/`--basename`, `-d`/`--destination`, `-f`/`--format`, `--bdxf`,
`--exportOnlyDetails`, `-p`/`--pageformat`, `-W`/`--pagew`, `-H`/`--pageh`, `-U`/`--pageunits`,
`-i`/`--ignoremargins`, `-L`/`--lmargin`, `-R`/`--rmargin`, `-T`/`--tmargin`, `-B`/`--bmargin`,
`-r`/`--rotate`, `-c`/`--crop`, `-u`/`--unite`, `-S`/`--savelen`, `-l`/`--layounits`,
`-s`/`--shiftlen`, `-G`/`--gapwidth`, `-g`/`--groups`.

- `--text2paths` is back, for the vector formats above. DXF keeps TEXT entities: DXF-ASTM requires them.
- `--layout-scale` is removed: PNG export renders at 96 px per inch.
- Page, margin, gap and rotation options: the direct layout flags above.
- `--exportOnlyDetails`: `--layout-no-packing`.

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
| `--text2paths` | `--text2paths` |
| `--layout-no-packing` | `--no-packing` |
| `--layout-<name> <value>` | `--<name> <value>` |

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
- Independent import: `python src\test\CliTest\test_cli_import_check.py <cli_test_report.json> [--inkscape <inkscape.com>]` opens each DXF with ezdxf and each PDF with pypdf (`pip install ezdxf pypdf pyparsing`), and with `--inkscape` imports each DXF, PDF, SVG and HP-GL file into Inkscape.
- Richmond pattern: add `-RichmondRoot <folder>` with `richmond-shirt_v1_v061-test.sm2d` and `2025-06-08-Sue.smis` (from git history).
- Interactive: `src\test\CliTestApp` (Rust, Iced). See its `README.md`.
- Older runners: `src\test\SeamlyLayoutTest\test_headless_export.py`,
  `src\test\CollectionTest\test_pattern_input_status.py`, `src\test\Seamly2DTest\test_measurement_cli.py`.

## Examples tab

Open **Help > Command line options > Examples**.

The tab builds commands from the executable path and the pattern and layout folders in Preferences.
Each command quotes file paths. Select a row and click **Copy command**.
Windows examples use Command Prompt and wait for the export.

| Example | Pattern | Settings |
|---|---|---|
| SVG layout | male_shirt.sm2d | Paper > Tiled > Letter |
| PDF layout (tiled) | male_shirt.sm2d | Paper > Tiled > Letter; pdf-tiled |
| DXF export (R13) | jacket2_40-146.sm2d | Paper > Roll > Custom; width 42.00 inches; Fabric; --basesize |

DXF uses R13 by default. SVG contains the complete layout; PDF provides separate tiles.
The commands use the remaining saved SeamlyLayout settings.
Keep the sample measurement files in their original folders.
The commands replace existing output files.
