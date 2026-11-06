# Calling Seamly2D from the command line

Usage: `seamly2d [options] filename`

| Argument | Definition |
|---|---|
| `filename` | Pattern file (`.sm2d`) |

| Option | Definition |
|---|---|
| `-h`, `--help` | Show the help text. |
| `-v`, `--version` | Show the version. |
| `-t`, `--test` | Test mode: load one pattern file, then quit without showing the main window. |
| `-m`, `--mfile <measure file>` | Measurement file for test mode or CLI export. |
| `-x`, `--gsize <size value>` | Size of a multisize pattern in test mode or CLI export. |
| `-e`, `--gheight <height value>` | Height of a multisize pattern in test mode or CLI export. |
| `--layout-export` | Generate a layout through SeamlyLayout and exit without showing either UI. |
| `--layout-format <format>` | SeamlyLayout format: `svg`, `pdf`, `pdf-tiled`, `png`, `jpg`, `jpeg`, `dxf`, `hpgl`, `ps`, `eps`, `gcode`, or `3mf`. Defaults to `svg`. |
| `--layout-output <path>` | Required destination file for the export. |
| `--layout-settings <path>` | Optional SeamlyLayout settings JSON file. |
| `--layout-export-options <path>` | Optional format-specific options JSON file. |
| `--svg-text-mode <mode>` | SVG labels: `designerFont`, `singleLineFont`, `hersheyStrokes`, or `asSupplied`. |
| `--layout-scale <factor>` | PNG scale factor. Valid only with `--layout-format png`. |
| `--no-scaling` | Turn off high DPI scaling. Same as the `QT_AUTO_SCREEN_SCALE_FACTOR=0` environment variable. |

## Seamly2D to SeamlyLayout export

`--layout-export` loads one `.sm2d` pattern, applies optional measurement and
gradation overrides, creates the existing tagged pieces SVG in memory, and
sends it to SeamlyLayout over standard input. It does not create a
`<basename>.pieces.svg` file. Seamly2D waits for SeamlyLayout and returns its
exit code; SeamlyLayout diagnostics are forwarded to standard error.
On Windows, CLI options attach standard output and error to the parent console
when one exists.

```text
seamly2d --layout-export --layout-format svg --svg-text-mode hersheyStrokes \
  --layout-output C:\exports\shirt.svg --mfile C:\measurements\shirt.smms \
  --gsize 40 --gheight 165 C:\patterns\shirt.sm2d
```

`--mfile`, `--gsize`, and `--gheight` also work with `--layout-export`.
Gradation options select the size and height used to generate the pieces SVG.
`--test` and `--layout-export` cannot be combined. `--layout-export` requires
one pattern file and `--layout-output`.

The export options map to SeamlyLayout as follows:

| Seamly2D option | SeamlyLayout option |
|---|---|
| `--layout-format <format>` | `--export <format>` |
| `--layout-output <path>` | `--output <path>` |
| `--layout-settings <path>` | `--settings <path>` |
| `--layout-export-options <path>` | `--export-options <path>` |
| `--svg-text-mode <mode>` | `--svg-text-mode <mode>` |
| `--layout-scale <factor>` | `--scale <factor>` |

SeamlyLayout also accepts an SVG file as its positional input. Its supported
headless formats are SVG, PDF, tiled PDF, PNG, JPEG, DXF, HP-GL, PostScript,
EPS, G-Code, and 3MF. G-Code and 3MF return an unavailable status when their
export modules are not present. Format-specific JSON options use the same
camelCase fields as the corresponding export dialog.

## Removed legacy options

The old Seamly2D writer options remain removed: `-b`/`--basename`,
`-d`/`--destination`, `-f`/`--format`, `--bdxf`, `--text2paths`,
`--exportOnlyDetails`, `-p`/`--pageformat`, `-W`/`--pagew`, `-H`/`--pageh`,
`-U`/`--pageunits`, `-i`/`--ignoremargins`, `-L`/`--lmargin`,
`-R`/`--rmargin`, `-T`/`--tmargin`, `-B`/`--bmargin`, `-r`/`--rotate`,
`-c`/`--crop`, `-u`/`--unite`, `-S`/`--savelen`, `-l`/`--layounits`,
`-s`/`--shiftlen`, `-G`/`--gapwidth`, and `-g`/`--groups`. The
`--layout-*` options above map only to SeamlyLayout exports.

## Automated headless validation

Run from the repository root after building SeamlyLayout:

```text
python src\test\SeamlyLayoutTest\test_headless_export.py <seamlylayout-executable> build\cli-validation
```

The runner uses an isolated data directory and Qt's offscreen platform.
It checks public formats, file signatures, SVG piece count, stdin transport,
diagnostics, and exit statuses. G-Code and 3MF must report unavailable in the
standard build. Tiled PDF uses an explicit `paperType: tiled` settings file.
Each process has a timeout. Any failed check returns a nonzero runner status.

Add `--seamly2d <executable> --pattern <pattern.sm2d>` to validate waiting
pass-through for every format and child-error propagation.
Add `--measurements <file.smis-or-smms>`, `--size <cm>`, and `--height <cm>`
to exercise measurement and gradation overrides.
The runner sets the SeamlyLayout executable path only in its isolated settings.

The synthetic two-piece smoke test does not replace geometry-fidelity checks.
Real-pattern coverage requires the optional Seamly2D arguments.
Use a short shadow-build path on Windows; generated CXX-Qt wrapper paths can
otherwise exceed the compiler's path-length limit.

Headless export rejects a layout with unplaced pieces before writing output.
It returns status 70 and lists the omitted pieces on standard error.
The interactive GUI still permits viewing a partial layout.
Use `--unplaced-settings <settings.json>` with the Seamly2D runner arguments
to assert that insufficient media returns 70 through both applications.

Use `--svg-input <pieces.svg> --layout-settings <settings.json>` to validate a
saved handoff with explicit media dimensions.
The committed Richmond handoff has 12 pieces and a two-inch calibration square.
The runner checks its piece/path counts and physical scale in SVG, DXF, and HP-GL.
It also checks PDF/PostScript page sizes and PNG/JPEG image dimensions.
The Richmond source pattern and measurement files remain necessary for a complete
Seamly2D end-to-end check; a cached handoff is not a substitute.

SeamlyMe accepts measurement files, not `.sm2d` patterns or layout-export options.
Use `seamlyme --test <measurements.smis-or-smms>` for a noninteractive file-load check.
Its `--height`, `--size`, and `--unit` options select measurement display inputs.
Seamly2D consumes those measurement files when it generates a layout handoff.
In `--test` mode, invalid arguments return status 64 without an error dialog.
An input `.sm2d` pattern returns status 66 because it is not a measurement file.

```text
python src\test\Seamly2DTest\test_measurement_cli.py <seamlyme-executable> build\measurement-cli --individual <measurements.smis> --multisize <measurements.smms> --pattern <pattern.sm2d>
```
