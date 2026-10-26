# Separate settings and logs for automated runs

Set `SEAMLY_DATA_DIR` to an absolute, writable directory before starting
Seamly2D or SeamlyMe. Both programs validate the directory before creating the
application and use its `settings/` folder for INI settings, including shared
settings. Seamly2D writes its log files under `logs/seamly2d/`, and SeamlyMe
under `logs/seamlyme/`. This keeps batch jobs independent of an operator's
interactive settings and log files.

For example, in PowerShell:

```powershell
$env:SEAMLY_DATA_DIR = 'E:/seamly-jobs/job-001/state'
seamly2d --test --gsize 50 --gheight 176 skirt.sm2d
```

The Qt 6 application settings files are:

- `<dir>/settings/Seamly/Seamly2D/qt6_seamly2d.ini`
- `<dir>/settings/Seamly/SeamlyMe/qt6_seamlyme.ini`
- `<dir>/settings/Seamly/qt6_common.ini` for shared settings

The settings migration copies older files from `<dir>/settings/Seamly2DTeam/`:
`qt6_seamly2d.ini` or `Seamly2D.ini`, `qt6_seamlyme.ini` or `SeamlyMe.ini`, and
`qt6_common.ini` or `common.ini`. No settings are migrated from the operator's
normal profile when this variable is set.

An empty data directory starts with default settings. Output may differ from
the operator's GUI session. To reproduce a job, seed the directory with
known settings before starting either application. Existing path settings can
point outside the data directory; this option redirects the default folders,
not paths explicitly chosen in saved settings.

A relative path stops the program with exit status 78 (configuration error).
An uncreatable or unwritable settings/log directory stops it with status 73.
Write access is checked with temporary files, including when directories already
exist. The cleaned absolute path is then reused for the life of the process.
The existing locations remain the default when the variable is empty or unset.

On headless Linux, use Qt's offscreen platform plugin:

```sh
QT_QPA_PLATFORM=offscreen SEAMLY_DATA_DIR=/srv/seamly-jobs/job-001/state \
    seamly2d --test /srv/seamly-jobs/job-001/skirt.sm2d
```

The default measurement, template, pattern, layout, label, image, backup and
body-scan paths use `<dir>` instead of `~/seamly2d/`. The option does not redirect
input pattern files or the operating system's
temporary directory. Set `TEMP`/`TMP` (or `TMPDIR` on Unix) separately when needed.
