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
| `-m`, `--mfile <measure file>` | Measurement file to use (test mode). |
| `-x`, `--gsize <size value>` | Size of a multisize pattern (test mode). |
| `-e`, `--gheight <height value>` | Height of a multisize pattern (test mode). |
| `--no-scaling` | Turn off high DPI scaling. Same as the `QT_AUTO_SCREEN_SCALE_FACTOR=0` environment variable. |

## Removed: console export mode

- Seamly2D has no console export mode. `-b`/`--basename` and its export and layout options are gone (Task Dead.4.1.1).
- SeamlyLayout makes all layouts and exports. A console route through SeamlyLayout is Task CLI.1 (`TODO_CLI.md`).
- Removed options: `-b`/`--basename`, `-d`/`--destination`, `-f`/`--format`, `--bdxf`, `--text2paths`, `--exportOnlyDetails`, `-p`/`--pageformat`, `-W`/`--pagew`, `-H`/`--pageh`, `-U`/`--pageunits`, `-i`/`--ignoremargins`, `-L`/`--lmargin`, `-R`/`--rmargin`, `-T`/`--tmargin`, `-B`/`--bmargin`, `-r`/`--rotate`, `-c`/`--crop`, `-u`/`--unite`, `-S`/`--savelen`, `-l`/`--layounits`, `-s`/`--shiftlen`, `-G`/`--gapwidth`, `-g`/`--groups`.
