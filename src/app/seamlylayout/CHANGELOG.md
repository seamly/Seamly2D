# SeamlyLayout Changelog

Author: slspencer
Copyright: 2026

---

## [Unreleased] — 0.1.0

### Added

- **Installer packaging** — Windows Inno Setup script (`packaging/windows/SeamlyLayout.iss`)
  and build script (`packaging/windows/build_installer.ps1`); macOS DMG script
  (`packaging/macos/build_dmg.sh`); Linux desktop entry
  (`packaging/linux/seamlylayout.desktop`).
- **Packaged default settings** — `qt_frontend/settings/default_settings.json` and
  sample settings files are now installed to `<installDir>/settings/` by the CMake
  install target, providing a seed source for legacy migration on upgrade.
- **SettingsModelTests** — new Qt test suite covering default field values, legacy
  `layoutMode` schema migration (`withGrain`/`withoutGrain` → `alongGrainline`/`withNap`),
  save/load round-trips, and `resetToDefaults()`.
- **PreferencesModel migration tests** — six new tests in `PreferencesModelTests`
  covering legacy folder-name rewriting (`layout-settings` → `settings`,
  `layout-preferences` → `preferences`) performed by `PreferencesModel::load()`.

### Changed

- **Runtime folder rename** — canonical user runtime folders are now
  `~/seamlyLayout/settings/` and `~/seamlyLayout/preferences/` (previously
  `layout-settings` and `layout-preferences` under `AppConfigLocation`).
  Existing files are copied to the new locations automatically on first launch
  after upgrade; old folders are not deleted.
- **CMakeLists.txt install rules** — added `install(DIRECTORY settings/ ...)` target
  that bundles default settings JSON files next to the executable (excluding the
  user-specific `preferences.json`).

### Migration notes (upgrading from pre-0.1.0)

On first launch after upgrade, `PreferencesModel::load()` detects any paths in
`preferences.json` that still reference the legacy folder names and rewrites them
to the canonical names, copying files where they do not yet exist at the new
location.  No manual steps are required.

Legacy folder name mapping:

| Old (pre-0.1.0) | New (0.1.0+) |
|---|---|
| `layout-settings` | `settings` |
| `layout-preferences` | `preferences` |
| `<exeDir>/settings/` | `~/seamlyLayout/settings/` (AppConfigLocation) |
| `<exeDir>/settings/preferences.json` | `~/seamlyLayout/preferences/preferences.json` |
