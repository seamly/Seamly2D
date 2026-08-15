# TODO — Migrate SeamlyLayout into the Seamly2D structure

Tasks for migrating the SeamlyLayout app into the Seamly2D structure — where SeamlyMe and SeamlyLayout are callable from within Seamly2D and as standalone applications, can be called from CLI for the full combined workflow of seamly2d+seamlyme+seamlylayout and as individual appications, and all three apps are distributed together in a single Qt runtime for installation on a user's computer.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Check off all completed tasks & subtasks and move completed tasks to TODO_COMPLETED.md

All TODO_MIGRATE.md tasks begin with 'M.' and all tasks are numbered

## Task M.1 — Implement tasks in TODO_INSTALLER.md to build Seamly pre-releases using github.com/seamly/seamly2d GitHub workflow ci.yml file to build after each pull-request to the run-seamlylayout branch (not the default branch)

## Task M.2 - Test Seamly pre-releases installation for 3 use cases: a. where Seamly is not previously installed, b. where Seamly version without SeamlyLayout is installed, c. where Seamly version with SeamlyLayout is installed

- M.2.1 - Windows x64 .msi
- M.2.2 - Windows arm64 .msi
- M.2.3 - MacOS .dmg files in .zip file
- M.2.4 - Linux AppImage .appimage file
- M.2.5 - Linux FlatPak

## Task M.3 - Create step-by-step instructions as .pdf (including steps regarding data migration from previous version without SeamlyLayout) for each (Win X64, Win Arm64, MacOS, Linux AppImage -- not needed for Linux FlatPak)

- M.3.1 - Windows x64 .msi
- M.3.2 - Windows arm64 .msi
- M.3.3 - MacOS .dmg files in .zip file
- M.3.4 - Linux AppImage .appimage file
- M.3.5 - Linux FlatPak

## Task M.4 - Re-organize all files needed to build the Seamly executables with the GitHub CI/CD ci.yml file so that all files are under the .github directory; update the CI/CD files with the new locations of moved files; build & test the updated

- M.4.0 - Re-organize files
- M.4.1 - Update ci.yml and related files to reflect new location
- M.4.2 - Build pre-releases with ci.yml
- M.4.3 - Test pre-releases
- M.4.3.1 - Windows x64 .msi
- M.4.3.2 - Windows arm64 .msi
- M.4.3.3 - MacOS .dmg files in .zip file
- M.4.3.4 - Linux AppImage .appimage file
- M.4.3.5 - Linux FlatPak

## Task M.5 - Implement tasks in TODO_CODE_SIGNING.md

## Task M.6 — Implement tasks in TODO_RENAME_SETTINGS_FILES.md

Rename the settings sources in `src/libs/vmisc/` so each name says which app it configures, and rename the classes with them so the pair complies with `.github/README-CODE-STYLES.md`: **class names** UpperCamelCase (the project's deliberate deviation from JSF-AV, which would demand `Settingscommon`), file names unique repo-wide, and no `v` prefix.

| Current file                       | class-match (style-guide exception) | Current class         | New class            |
| ---------------------------------- | ----------------------------------- | --------------------- | -------------------- |
| `vcommonsettings.cpp` / `.h`   | `SettingsCommon.cpp` / `.h`     | `VCommonSettings`   | `SettingsCommon`   |
| `vseamlymesettings.cpp` / `.h` | `SettingsSeamlyMe.cpp` / `.h`   | `VSeamlyMeSettings` | `SettingsSeamlyMe` |
| `vsettings.cpp` / `.h`         | `SettingsSeamly2D.cpp` / `.h`   | `VSettings`         | `SettingsSeamly2D` |

**Class-rename scope measured 2026-07-26:** `VCommonSettings` **447 occurrences in 17 files**, `VSettings` **147 in 18 files**, `VSeamlyMeSettings` **25 in 9 files** (`src/`, all extensions). Plus the translations: `tr()` contexts are keyed on the class name, so all **22 `share/translations/seamly2d_*.ts`** files carry a `<name>VCommonSettings</name>` context (8 messages) and a `<name>VSettings</name>` context (2) — **~220 already-translated strings** that go obsolete unless the contexts are renamed with the classes. `VSeamlyMeSettings` has no translation context.

**Scope measured 2026-07-26:** **101 files under `src/`** `#include` one of the three headers, in two forms — the in-directory `#include "vcommonsettings.h"` and the sibling-library `#include "../vmisc/vsettings.h"` form, which resolves only because every `.pro` adds `INCLUDEPATH += $$PWD/../../libs/vmisc`. `src/libs/vmisc/vmisc.pri` is the **only** build file naming them (SOURCES lines 5/8/9, HEADERS lines 24/27/28) — no other `.pro`/`.pri`/workflow lists these sources, so the build wiring is a six-line change.

**Do files and classes in one commit, not two.** Splitting them means a middle state where `settings_common.h` declares `VCommonSettings` — exactly the file/class mismatch the style rule exists to prevent — and it doubles the churn through the same ~600 call sites.

- [ ] **Settle the file-name form first** (A class-match `SettingsCommon.h` vs B snake_case `settings_common.h`), plus the two smaller calls above (brand casing; `VSettings` → `SettingsSeamly2D`); record the decision in `.github/README-CODE-STYLES.md` if it needs sharpening, since every future rename follows it
- [ ] Rename all six files with `git mv` (not delete + add) so history and `git blame` follow the rename
- [ ] Update the six entries in `src/libs/vmisc/vmisc.pri` (SOURCES 5/8/9, HEADERS 24/27/28)
- [ ] Update every `#include` across the 101 files — both the in-directory and the `../vmisc/…` form — then confirm with a repo-wide grep that no `vcommonsettings.h` / `vsettings.h` / `vseamlymesettings.h` include remains anywhere under `src/`
- [ ] Include the test suite in that sweep: `src/test/Seamly2DTest/tst_dataroot.{h,cpp}` is the only test that includes these headers (and uses `VCommonSettings` heavily), so a missed include there fails only the test build, not the app build
- [ ] Rename the include guards to match the new file names — `VCOMMONSETTINGS_H` → `SETTINGS_COMMON_H`, `VSETTINGS_H` → `SETTINGS_SEAMLY2D_H`, `VSEAMLYMESETTINGS_H` → `SETTINGS_SEAMLYME_H` (each at lines 53-54 of its header)
- [ ] Rename the three classes at every occurrence (~620 across 25 distinct files): the `class X : public Y` declarations, constructors/destructors, every forward declaration (`class VSettings;`), member and pointer types (`VSettings *Seamly2DSettings()`, `VCommonSettings *settings`), and every static/qualified call (`VCommonSettings::…`). `VSettings` is a whole-word match — nothing else contains it — so use word-boundary, case-**sensitive** replacement and never touch the lowercase `settings` identifiers that surround them
- [ ] Rename the `tr()` contexts in all 22 `share/translations/seamly2d_*.ts` files (`<name>VCommonSettings</name>` → `SettingsCommon`, `<name>VSettings</name>` → `SettingsSeamly2D`) in the same commit, or the ~220 existing translated strings in those contexts go obsolete. Verify afterwards by running `lupdate` and confirming it reports no newly-obsolete messages in these contexts
- [ ] Update the `@file` line in each of the six license-header blocks (e.g. `//  @file   vcommonsettings.h`), and the `@brief`/`@class` text of anything that names the old class, leaving the existing `@author`/`@date`/copyright lines as they are
- [ ] Update the docs that name these paths **or classes** — `.github/README-BUILDS.md:17` (`VSettings`, `src/libs/vmisc/vsettings.cpp`) and `:77` (`VCommonSettings::dataRoot()` and the rest of that API row) at minimum — and decide whether historical entries (`project-docs/TODO_COMPLETED.md`, `SESSION_HANDOVER.md`, `project-docs/TODO_SEAMLY2D.md` Task 42, `project-docs/TODO_SEAMLYME.md` Task 43) get rewritten or left as the record of what things were called at the time; record the decision either way
- [ ] Amend Task 52 above — it points at "the eight `vsettings.cpp` accessors" and at `VCommonSettings::mergeStrayCommonSettings()` / `getLabelTemplatePath()` — so whoever picks it up looks for `settings_seamly2d.cpp` and `SettingsCommon`
- [ ] Verify the build and the four test binaries through `ci.yml` — the local build and test scripts were deleted in August 2026. If you do build by hand, wipe the shadow-build tree first; a stale `Makefile`/object tree can link an old object and mask a missed include (Task 46)
- [ ] Confirm CI stays green on `ci.yml`, the one workflow that compiles these sources

## Task M.7 - Implement tasks in TODO_SEAMLYTEAM.md

## Task M.8 - Implement tasks in TODO_SEAMLY2D.md

## Task M.9 - Implement tasks in TODO_SEAMLYLAYOUT.md

## Task M.10 - Implement tasks in TODO_SEAMLYME.md

## Task M.11 - Implement tasks in TODO_CLI.md

## Task M.12 - Implement tasks in TODO_REMOVE_DEAD_LAYOUT_CODE.md

## Task M.13 - Update the .github/README.md file to point at the correct downloadable release

Do this only when the migration is pushed upstream — the badges link to
`FashionFreedom/Seamly2D/releases/latest`, so repointing them before the release
that carries the new artifacts breaks the live public download links.

- [ ] Windows x64 — `Seamly2D-windows.zip` (NSIS) → **`seamly-x64.msi`**, which `ci.yml`'s `windows-msi` job now builds and the `publish` job attaches to the pre-release (Task Installer.1.1). The NSIS x64 package is no longer produced at all.
- [ ] Windows arm64 — `Seamly2D-win-arm64.zip` (NSIS) → **`seamly-arm64.msi`**, from the same `windows-msi` matrix (Task Installer.1.2). **NSIS is retired entirely**; no workflow runs `makensis` any more.
- [ ] MacOS
- [ ] Linux appimage
- [ ] Linux flatpak
