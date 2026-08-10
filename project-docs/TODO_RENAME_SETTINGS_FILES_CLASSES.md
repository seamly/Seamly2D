# TODO — Create the combined MSI installer for Seamly2D, SeamlyMe, and SeamlyLayout

Tasks for creating an .msi file for installation on a user's amd64 computer with Windows 10 or Windows 11.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file begin with `Rename.`

## Task Rename.1 — `VSettings`' own path settings also land in an "Unknown Organization" stray file (found doing Task 34, 2026-07-26)

Task 34 fixed this defect for the **shared** common settings file (`VCommonSettings::commonSettingsOrganization()` + `mergeStrayCommonSettings()`), but the same root cause is still live in `src/libs/vmisc/vsettings.cpp`. Eight accessors build a throwaway `QSettings` from the *instance's* organization and application names:

```cpp
QSettings settings(this->format(), this->scope(), this->organizationName(), this->applicationName());
```

Since Task 15 the apps construct their settings object from an explicit settings **file path** (`VSettings(qt6Settings, QSettings::IniFormat, this)` in `Application2D::openSettings()`), and `QSettings` records neither an organization nor an application name for that constructor — both come back empty. QSettings then substitutes the literal `"Unknown Organization"` and, with an empty application name, writes an organization-level file. Confirmed on the developer machine:

```text
%APPDATA%\Unknown Organization.ini
  [paths]
  layout=G:/My Drive/seamly2d/layouts
  pattern=G:/My Drive/seamly2d
  [pattern]
  graphicalOutput=true
```

Affected keys: `paths/pattern`, `paths/layout`, `paths/seamlyLayoutApp`, `pattern/graphicalOutput` (`getPatternPath`/`SetPathPattern`, `getLayoutPath`/`SetPathLayout`, `getSeamlyLayoutAppPath`/`setSeamlyLayoutAppPath`, `GetGraphicalOutput`/`SetGraphicalOutput`). Nothing is broken for the user *today* — the same wrong file is both written and read, so the values round-trip — but they sit outside the unified `Seamly` folder Task 15 established, are shared between apps rather than per-app, and are missed by the settings migration and by the uninstall/packaging documentation. Deliberately left out of Task 34 to keep that change scoped: unlike the common file (which had to be correct for the data root and the Task 14 installer), these keys are self-consistent where they are.

- [ ] Rename.1.1 Point the eight `vsettings.cpp` accessors at the app's own settings file — they intend "this application's settings", which post-Task-15 is `this`, so plain `value()`/`setValue()` as `VCommonSettings::getLabelTemplatePath()` already does; check `VSeamlyMeSettings` for the same pattern
- [ ] Rename.1.2 Bring existing values forward from `%APPDATA%\Unknown Organization.ini` (and the platform equivalents) on first run, non-destructively — copy-if-missing, never delete the stray file — mirroring `VCommonSettings::mergeStrayCommonSettings()`
- [ ] Rename.1.3 Decide whether `paths/pattern` and `paths/layout` belong in the app file or in the shared common file alongside the other seven `paths/*` keys, and record why
- [ ] Rename.1.4 **Stop `CollectionTest` writing into the real user settings first.** `%APPDATA%\Unknown Organization.ini` on the developer machine holds `layout=…\CollectionTest\bin\tst_seamly2d_tmp` — the suite launches the real `seamly2d.exe`, which persists a layout path through these very accessors. Today the defect is what contains the damage (the write lands in the stray file); the moment the accessors point at `this`, the same test run scribbles on `%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini`. Give the test-launched apps their own settings location (distinct organization/application name, or `QSettings::setPath()`) **before** repointing the accessors
- [ ] Rename.1.5 Add a regression test that no Seamly settings resolve to an `"Unknown Organization"` path, so a future accessor cannot reintroduce this
- [ ] Rename.1.6 Update the settings-storage tables in `.github/README-BUILDS.md` once the location changes

## Task Rename.2 — Rename the three `vmisc` settings files **and their classes** to SettingsCommon, SettingsSeamly2d, SettingsSeamlyMe

Rename the settings sources in `src/libs/vmisc/` so each name says which app it configures, and rename the classes with them so the pair complies with `.github/README-CODE-STYLES.md`: **class names** UpperCamelCase (the project's deliberate deviation from JSF-AV, which would demand `Settingscommon`), file names unique repo-wide, and no `v` prefix.

| Current file                       | class-match (style-guide exception) | Current class         | New class            |
| ---------------------------------- | ----------------------------------- | --------------------- | -------------------- |
| `vcommonsettings.cpp` / `.h`   | `SettingsCommon.cpp` / `.h`     | `VCommonSettings`   | `SettingsCommon`   |
| `vseamlymesettings.cpp` / `.h` | `SettingsSeamlyMe.cpp` / `.h`   | `VSeamlyMeSettings` | `SettingsSeamlyMe` |
| `vsettings.cpp` / `.h`         | `SettingsSeamly2D.cpp` / `.h`   | `VSettings`         | `SettingsSeamly2D` |

**Class-rename scope measured 2026-07-26:** `VCommonSettings` **447 occurrences in 17 files**, `VSettings` **147 in 18 files**, `VSeamlyMeSettings` **25 in 9 files** (`src/`, all extensions). Plus the translations: `tr()` contexts are keyed on the class name, so all **22 `share/translations/seamly2d_*.ts`** files carry a `<name>VCommonSettings</name>` context (8 messages) and a `<name>VSettings</name>` context (2) — **~220 already-translated strings** that go obsolete unless the contexts are renamed with the classes. `VSeamlyMeSettings` has no translation context.

**Scope measured 2026-07-26:** **101 files under `src/`** `#include` one of the three headers, in two forms — the in-directory `#include "vcommonsettings.h"` and the sibling-library `#include "../vmisc/vsettings.h"` form, which resolves only because every `.pro` adds `INCLUDEPATH += $$PWD/../../libs/vmisc`. `src/libs/vmisc/vmisc.pri` is the **only** build file naming them (SOURCES lines 5/8/9, HEADERS lines 24/27/28) — no other `.pro`/`.pri`/workflow lists these sources, so the build wiring is a six-line change.

**Do files and classes in one commit, not two.** Splitting them means a middle state where `settings_common.h` declares `VCommonSettings` — exactly the file/class mismatch the style rule exists to prevent — and it doubles the churn through the same ~600 call sites.

- [ ] Rename.2.1 **Settle the file-name form first** (A class-match `SettingsCommon.h` vs B snake_case `settings_common.h`), plus the two smaller calls above (brand casing; `VSettings` → `SettingsSeamly2D`); record the decision in `.github/README-CODE-STYLES.md` if it needs sharpening, since every future rename follows it
- [ ] Rename.2.2 Rename all six files with `git mv` (not delete + add) so history and `git blame` follow the rename
- [ ] Rename.2.3 Update the six entries in `src/libs/vmisc/vmisc.pri` (SOURCES 5/8/9, HEADERS 24/27/28)
- [ ] Rename.2.4 Update every `#include` across the 101 files — both the in-directory and the `../vmisc/…` form — then confirm with a repo-wide grep that no `vcommonsettings.h` / `vsettings.h` / `vseamlymesettings.h` include remains anywhere under `src/`
- [ ] Rename.2.5 Include the test suite in that sweep: `src/test/Seamly2DTest/tst_dataroot.{h,cpp}` is the only test that includes these headers (and uses `VCommonSettings` heavily), so a missed include there fails only the test build, not the app build
- [ ] Rename.2.6 Rename the include guards to match the new file names — `VCOMMONSETTINGS_H` → `SETTINGS_COMMON_H`, `VSETTINGS_H` → `SETTINGS_SEAMLY2D_H`, `VSEAMLYMESETTINGS_H` → `SETTINGS_SEAMLYME_H` (each at lines 53-54 of its header)
- [ ] Rename.2.7 Rename the three classes at every occurrence (~620 across 25 distinct files): the `class X : public Y` declarations, constructors/destructors, every forward declaration (`class VSettings;`), member and pointer types (`VSettings *Seamly2DSettings()`, `VCommonSettings *settings`), and every static/qualified call (`VCommonSettings::…`). `VSettings` is a whole-word match — nothing else contains it — so use word-boundary, case-**sensitive** replacement and never touch the lowercase `settings` identifiers that surround them
- [ ] Rename.2.8 Rename the `tr()` contexts in all 22 `share/translations/seamly2d_*.ts` files (`<name>VCommonSettings</name>` → `SettingsCommon`, `<name>VSettings</name>` → `SettingsSeamly2D`) in the same commit, or the ~220 existing translated strings in those contexts go obsolete. Verify afterwards by running `lupdate` and confirming it reports no newly-obsolete messages in these contexts
- [ ] Rename.2.9 Update the `@file` line in each of the six license-header blocks (e.g. `//  @file   vcommonsettings.h`), and the `@brief`/`@class` text of anything that names the old class, leaving the existing `@author`/`@date`/copyright lines as they are
- [ ] Rename.2.10 Update the docs that name these paths **or classes** — `.github/README-BUILDS.md:17` (`VSettings`, `src/libs/vmisc/vsettings.cpp`) and `:77` (`VCommonSettings::dataRoot()` and the rest of that API row) at minimum — and decide whether historical entries (`project-docs/TODO_COMPLETED.md`, `SESSION_HANDOVER.md`, `project-docs/TODO_SEAMLY2D.md` Task 42, `project-docs/TODO_SEAMLYME.md` Task 43) get rewritten or left as the record of what things were called at the time; record the decision either way
- [ ] Rename.2.12 Amend Task 52 above — it points at "the eight `vsettings.cpp` accessors" and at `VCommonSettings::mergeStrayCommonSettings()` / `getLabelTemplatePath()` — so whoever picks it up looks for `settings_seamly2d.cpp` and `SettingsCommon`
- [ ] Rename.2.13 Build and test locally: `scripts/sd.ps1` plus the test binaries (`scripts/st.ps1` runs only one of the four that CI runs via `make check` — run the others too). Wipe the shadow-build tree first; a stale `Makefile`/object tree can link an old object and mask a missed include (Task 46)
- [ ] Rename.2.14 Confirm CI stays green on all three workflows that compile these sources (`ci.yml`, `windows-msi.yml`, and `seamlylayout-ci.yml` only if it pulls the parent libs)
