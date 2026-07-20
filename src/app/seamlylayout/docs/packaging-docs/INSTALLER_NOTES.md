# SeamlyLayout — Installer Packaging Notes

Author: slspencer
Copyright: 2026

Last updated: 2026-06-22

## Overview

This document covers the runtime folder layout, legacy migration behavior, and
platform-specific packaging for SeamlyLayout releases.

---

## Runtime Folder Layout

SeamlyLayout uses two runtime folder trees — one for **settings** (layout
parameters) and one for **preferences** (application preferences such as viewer
paths and default directories).

| Folder | Platform path | Purpose |
|---|---|---|
| `settings` | `~/seamlyLayout/settings/` | Layout settings JSON files (`*.json`) |
| `preferences` | `~/seamlyLayout/preferences/` | User preferences (`preferences.json`) |

The exact OS paths are:

| OS | Base (`~/seamlyLayout/`) |
|---|---|
| Windows | `%USERPROFILE%\seamlyLayout\` |
| Linux | `$HOME/seamlyLayout/` |
| macOS | `$HOME/seamlyLayout/` |

Both folders are created automatically on first run if they do not exist.

### settings folder

- Stores named layout settings files (`.json`), including `default_settings.json`.
- The user can save additional named settings files here for quick recall.
- The Settings dialog's **Load** / **Save** file pickers default to this folder.
- Configured via `settings_directory` in `preferences.json`.

### preferences folder

- Stores `preferences.json` — viewer executable paths, default directories.
- Configured via `preferences_directory` in `preferences.json`.

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

`preferences.json` is **not** bundled — it contains user-specific paths and is
seeded at first run from the embedded Qt resource `:/defaults/default_preferences.json`.

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

`PreferencesModel::load()` inspects every stored directory and file path in
`preferences.json`. Any path whose folder segment still uses a legacy name is
automatically rewritten to the canonical name:

1. The canonical target folder is created if it does not exist.
2. Any existing files in the legacy folder are **copied** (not moved) to the
   canonical folder so the upgrade is non-destructive.
3. The in-memory path value is updated to point at the new location.
4. A log line is emitted for each migration (`PreferencesModel::load(): migrated legacy ...`).

`PreferencesModel::defaultPreferencesFilePath()` performs the same seed/copy
logic for `preferences.json` itself when it does not yet exist in the canonical
location, checking three legacy source candidates in priority order:

1. `AppConfigLocation/layout-preferences/preferences.json`
2. `<exeDir>/layout-settings/preferences.json`
3. `<exeDir>/settings/preferences.json`

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
- Qt 6.10.1 msvc2022_64 at `C:\Qt\6.10.1\msvc2022_64`
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
- Qt 6.10.1 macOS at `/usr/local/Qt/6.10.1/macos`
- `create-dmg` (`brew install create-dmg`)

Output: `packaging/macos/Output/SeamlyLayout-0.1.0-macOS.dmg`

---

## LGPL-3.0 Compliance

Qt 6.10 is dynamically linked under LGPL-3.0.  Every installer must:

1. Include `packaging/licenses/LGPL-3.0.txt` alongside the application.
2. Include `packaging/licenses/qt-source-notice.txt` directing users to
   `https://download.qt.io` for Qt source code.
3. Not statically link Qt in a way that would require relicensing under GPL.

The `LGPL-3.0.txt` file is intentionally **not committed** to the repository
(it is large and unchanged from the official text).  The build scripts will
error and instruct the packager to download it if it is missing.

Download from: `https://www.gnu.org/licenses/lgpl-3.0.txt`  
Save to: `packaging/licenses/LGPL-3.0.txt`
