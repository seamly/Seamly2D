# SeamlyLayout — Installer Packaging Notes

Author: slspencer
Copyright: 2026

Last updated: 2026-08-20

## Overview

This document covers the runtime folder layout, legacy migration behavior, and
platform-specific packaging for SeamlyLayout releases.

---

## Runtime Folder Layout

SeamlyLayout stores layout profiles separately from application preferences and from
user data (patterns, exported layouts). Both `settings` and `preferences` are app-config,
so they nest under the same `QStandardPaths::AppConfigLocation` root as
`qt6_seamlylayout.ini` — never under the user's home directory or under `%DATAROOT%`
(Layout.8, 2026-08-30: fixed a defect where a fresh MSI install seeded them under the raw
home directory instead).

| Item | Platform path | Purpose |
|---|---|---|
| `settings` | `<AppConfigLocation>/settings/` | Layout settings JSON files (`*.json`) |
| `qt6_seamlylayout.ini` | `QStandardPaths::AppConfigLocation` | Application preferences |
| `preferences` | `<AppConfigLocation>/preferences/` | Optional JSON default profiles |

The exact OS paths are:

| OS | `AppConfigLocation` |
|---|---|
| Windows | `%LOCALAPPDATA%\Seamly\SeamlyLayout\` |
| Linux | `$HOME/.config/Seamly/SeamlyLayout/` |
| macOS | `$HOME/Library/Preferences/Seamly/SeamlyLayout/` |

The application creates required folders on first run.

By contrast, `input_directory`/`layout_directory` (the SVG import default and the export
default) are user data. The bundled `default_preferences.json` seeds both to the same
shared `<DataRoot-or-home>/seamlyLayout/layouts` folder on Windows — one folder for both
import and export, not separate `/input`/`/output` trees — nested under the
installer-recorded `%DATAROOT%` when one was recorded, or under the raw home directory
otherwise. If either field is later cleared back to empty, the runtime fallback in
`resolvedInputDirectory()`/`resolvedLayoutDirectory()` takes over instead and nests them
separately as `<DataRoot>/input`/`<DataRoot>/output`, falling back further to
`<exeDir>/input`/`/output` (or the `AppConfigLocation` root inside a read-only macOS
bundle, Linux AppImage, or Flatpak sandbox) when no data root is recorded.

### settings folder

- Stores named layout settings files (`.json`), including `default_settings.json`.
- The user can save additional named settings files here for quick recall.
- The Settings dialog's **Load** / **Save** file pickers default to this folder.
- Configured through `settings_directory` in `qt6_seamlylayout.ini`.

### Application preferences

- Stores viewer paths, default directories, and the selected settings profile.
- Uses `qt6_seamlylayout.ini` directly under `QStandardPaths::AppConfigLocation`.
- Uses `%LOCALAPPDATA%\Seamly\SeamlyLayout\qt6_seamlylayout.ini` on Windows.
- Keeps `default_preferences.json` as an optional reset profile.

---

## Packaged Defaults

The installer bundles the following files next to the executable under
`<installDir>/settings/`:

| File | Purpose |
|---|---|
| `default_settings.json` | Application default layout settings |
| `B0.json` | Sample B0-paper layout settings |
| `roll_36in.json` | Sample 36-inch roll settings |
| `roll_48in.json` | Sample 48-inch roll settings |

The installer does not bundle `qt6_seamlylayout.ini` because it contains user-specific values.
The embedded `:/defaults/default_preferences.json` resource supplies first-run defaults.

---

## Legacy Migration

### Background

Before 2026-05-22, the runtime folders used different names:

| Legacy name | Current canonical name |
|---|---|
| `layout-settings` | `settings` |
| `layout-preferences` | `preferences` |

Before 2026-05-22 (earlier still), settings and preferences were written under
the install directory (`<exeDir>/settings/`) rather than the user's home tree.

### What the app does on first run after an upgrade

`PreferencesModel::load()` imports an existing `preferences.json` when the INI file is absent.
It saves the imported values to `qt6_seamlylayout.ini` and keeps the JSON file.

The model inspects every stored directory and file path. Any path whose folder segment uses a legacy name is
automatically rewritten to the canonical name:

1. The canonical target folder is created if it does not exist.
2. Any existing files in the legacy folder are **copied** (not moved) to the
   canonical folder so the upgrade is non-destructive.
3. The in-memory path value is updated to point at the new location.
4. A log line is emitted for each migration (`PreferencesModel::load(): migrated legacy ...`).

The first-run import checks these JSON source candidates in priority order:

1. `AppConfigLocation/preferences/preferences.json`
2. `AppConfigLocation/layout-preferences/preferences.json`
3. `<exeDir>/layout-settings/preferences.json`
4. `<exeDir>/settings/preferences.json`

### What the installer does

The Windows installer (`SeamlyLayout.iss` / `build_installer.ps1`) detects a
`layout-preferences` folder in the target install directory during the
"Select Destination Location" page and shows a friendly notice that migration
will happen at first launch.  No files are moved by the installer itself.

---

## Platform Packaging

### Windows — Inno Setup

**Script:** `packaging/windows/SeamlyLayout.iss`  
**Build script:** `packaging/windows/build_installer.ps1`

```powershell
# One-liner (from repo root):
.\packaging\windows\build_installer.ps1
```

Prerequisites:
- Qt 6.11.1 msvc2022_64 at `C:\Qt\6.11.1\msvc2022_64`
- Inno Setup 6 at `C:\Program Files (x86)\Inno Setup 6\iscc.exe`
- `packaging/licenses/LGPL-3.0.txt` (download from gnu.org, not committed)

Output: `packaging/windows/Output/SeamlyLayout-0.1.0-win64.exe`

### Linux — Desktop Entry + AppImage (planned)

**Desktop file:** `packaging/linux/seamlylayout.desktop`

Install the desktop entry and icons (from the CMake install target):

```bash
cmake --install qt_frontend/build/Release --prefix /usr
```

AppImage packaging is planned but not yet implemented.

### macOS — DMG

**Script:** `packaging/macos/build_dmg.sh`

```bash
bash packaging/macos/build_dmg.sh
```

Prerequisites:
- Qt 6.11.1 macOS at `/usr/local/Qt/6.11.1/macos`
- `create-dmg` (`brew install create-dmg`)

Output: `packaging/macos/Output/SeamlyLayout-0.1.0-macOS.dmg`

---

## LGPL-3.0 Compliance

Qt 6.11 is dynamically linked under LGPL-3.0.  Every installer must:

1. Include `packaging/licenses/LGPL-3.0.txt` alongside the application.
2. Include `packaging/licenses/qt-source-notice.txt` directing users to
   `https://download.qt.io` for Qt source code.
3. Not statically link Qt in a way that would require relicensing under GPL.

The `LGPL-3.0.txt` file is intentionally **not committed** to the repository
(it is large and unchanged from the official text).  The build scripts will
error and instruct the packager to download it if it is missing.

Download from: `https://www.gnu.org/licenses/lgpl-3.0.txt`  
Save to: `packaging/licenses/LGPL-3.0.txt`
