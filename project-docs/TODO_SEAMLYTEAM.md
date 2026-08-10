# Task Team.1 — Retire the legacy organization names (`Seamly2DTeam`, `Seamly Systems`)

Goal: get `Seamly2DTeam` and `Seamly Systems` out of the codebase without breaking settings migration.

**Why they can't just be renamed.** They are folder names already on users' disks, not labels the project chooses. Git history: `ValentinaTeam` (f60d3e3017) → `Seamly2DTeam` (fdd0fbc113) → `Seamly` (Task 15, 4bab1efb85); seamlyLayout used `Seamly Systems` over the same span. `SeamlyTeam` and `Seamly Project` never shipped as org names. Renaming the lookup keys points migration at paths that have never existed, so it silently no-ops and pre-Task-15 upgraders get default settings while their real config is stranded. The fix is to delete the migration code and the keys together, once nobody is upgrading across Task 15.

## Now

- [ ] Team.1 Confirm the live labels are consistent — current org is `"Seamly"` (`VER_COMPANYNAME_STR`, seamlyLayout `main.cpp`), MSI `Manufacturer="Seamly Project"`, Doxygen `DOCSET_PUBLISHER_NAME="Seamly Project"` (done 2026-08-03)
- [ ] Team.2 Grep for any remaining *live* (non-legacy, non-comment) use of either string; there should be none
- [ ] Team.3 Delete the dead commented-out `VSettings(... "Seamly2DTeam", "Seamly2D" ...)` block at `src/app/seamlyme/tmainwindow.cpp:596`
- [ ] Team.4 Leave the five `kLegacyOrganizationName` constants and the Inno Setup legacy probe untouched, and add a one-line "do not rename — on-disk folder name" comment to each so this does not resurface

## At sunset

- [ ] Team.5 Pick the sunset release — the first that no longer migrates pre-Task-15 settings; record the version and the reasoning here
- [ ] Team.6 Release-note it beforehand: upgrading from a pre-Task-15 build requires an intermediate release first, or settings start fresh
- [ ] Team.7 Delete `VAbstractApplication::MigrateSeamlySettingsLocation()` / `NotifySeamlySettingsMigrated()` and their call sites in `Application2D::openSettings()` / `ApplicationME::openSettings()`
- [ ] Team.8 Delete seamlyLayout's `migrateLegacyOrganizationTree()` and the legacy branches in `PreferencesModel::appConfigRootPath()` / `SettingsModel::defaultSettingsFilePath()`
- [ ] Team.9 Delete the legacy probe and upgrade-guard text in `src/app/seamlylayout/packaging/windows/SeamlyLayout.iss`
- [ ] Team.10 Delete the migration tests and the `kLegacyOrganizationName` fixture in `src/test/Seamly2DTest/qttestmainlambda.cpp`
- [ ] Team.11 Strip the now-obsolete legacy-org paragraphs from `.github/README-BUILDS.md` (Windows/macOS/AppImage/Flatpak settings sections) and `project-docs/TODO_MIGRATE.md` Tasks 17/18
- [ ] Team.12 Verify: `scripts\st.ps1` green, `ctest --preset debug` green, and a clean-profile first run still lands in `%LOCALAPPDATA%\Seamly\<app>\` with no legacy lookup attempted

## Verification note

Any change here touches first-run behaviour on real profiles. Test against throwaway directories only — `QDir::homePath()` cannot be redirected on Windows (see the testing note in `.github/README-BUILDS.md`).
