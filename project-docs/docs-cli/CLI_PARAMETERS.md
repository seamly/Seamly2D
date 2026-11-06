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
