# Session handover

## Current state (2026-07-30, latest session): Task 51 — the laptop install run happened and found three real defects; one is fixed

**Branch `task-51-msi-install-experience`.** The user ran the kit on the test Windows 11 laptop through the upgrade step. **52 of 57 automated checks passed.** Task 51 stays open in `project-docs/TODO_MIGRATE.md`, which now carries a "Progress 2026-07-30" block and **two new subtasks** for the defects still open.

### What the run proved works

All three apps start and stay running from the install (so the deployed Qt/WebEngine runtime is complete — the single highest-value check); all three file associations resolve and a real `.sm2d` opens through ShellExecute; desktop shortcuts and their registry breadcrumbs correct; ARP entry correct on name, publisher, version, comments, links, size, uninstall string; exactly one UAC prompt; `SeamlyPreviousInstallDlg` displayed correctly and in the right position (log: `Action start 18:20:16` → `Dialog created` → `Return value 1`), ahead of Welcome → EULA → Destination Folder → Ready.

### The three defects

1. **`SeamlyShortcutsDlg` never appears — STILL OPEN.** Not a packaging error. The `ControlEvent` row is in the shipped MSI and correct in every column (`InstallDirDlg`/`Next`/`SpawnDialog`/`SeamlyShortcutsDlg`, condition `1`, ordering 2, ahead of the built-in `NewDialog` at 4), and the `Dialog` row has `Attributes = 7`. The `/l*v` log shows **no attempt to create it** — and a failed creation would have logged 2803/2826 like the other dialogs. **Root cause is the WiX version:** the design notes assumed WiX v3/v4's `InstallDirDlg` (`DoAction WixUIValidatePath` + conditional `SpawnDialog InvalidDirDlg`), but this is **WiX 6.0.2**, whose `InstallDirDlg` Next publishes `CheckTargetPath` — a v6 built-in from the UI extension's `uica.dll`. Our `SpawnDialog` is skipped in that chain. Because `SEAMLYDESKTOPSHORTCUTS` defaults to 1 the shortcuts were created anyway and every automated check passed: the default works, the *choice* is never offered. **Do not chase this by rebuilding the 165 MB package per attempt** — build a small UI-only MSI with the same `ui:WixUI` reference and the same dialogs; it compiles in seconds and can be clicked through and cancelled at the Ready page without installing anything.
2. **Dialog geometry — STILL OPEN.** `SeamlyPreviousInstallDlg`'s `BannerLine`/`BottomLine` are `Width="373"` on a 370-wide dialog → error 2826 twice. Stock WixUI dialogs log the same code at `DEBUG:` only; ours is *also* logged as a user-facing "unexpected error". Three characters to fix.
3. **The user-data tree was never created on a fresh install — FIXED in this session.** `~\seamlyData` did not exist at all after installing and running the apps. `ensureDataRootTree()` creates the nine subfolders but its only production caller was `setDataRoot()`, which runs only when the user *changes* the root in Preferences → Paths; first run goes through `initializeDataRoot()`, which resolves and records the path directly. Fixed by calling `ensureDataRootTree(dataRoot())` from `Application2D::openSettings()` and `ApplicationME::openSettings()` — **in the applications, not inside `initializeDataRoot()`**, because that is the only place the real home directory reaches it and the unit tests do call `initializeDataRoot()` (the standing Task 34/53 rule).

### The checker had a bug of its own, now fixed

All three Start Menu shortcuts reported `FAILED … points into the install directory - target = 'C:\Windows\Installer\{ProductCode}\seamly2d.ico'`. They are **advertised** shortcuts (nested inside `<File KeyPath="yes">` with no `Target` — WiX's standard pattern), and **`WScript.Shell` does not report an advertised shortcut's target; it returns the extracted icon path.** The script assumed an unresolvable advertised shortcut came back *empty*, so that branch was never reached and three correct shortcuts failed every run. `test_msi_install.ps1` now resolves the Darwin descriptor through `MsiGetShortcutTarget` + `MsiGetComponentPath`, asserting something stronger: that the shortcut resolves to an installed file inside the install directory. **The kit copy was refreshed from the source copy — they are byte-identical again.**

### Files changed

| File | Change |
| ---- | ------ |
| `src/app/seamly2d/core/application_2d.cpp`, `src/app/seamlyme/application_me.cpp` | `ensureDataRootTree(dataRoot())` after `initializeDataRoot()`, with the reasoning inline |
| `src/test/Seamly2DTest/tst_dataroot.{cpp,h}` | New `StartupResolvesThenSeedsTheConfiguredRoot` — pins that resolution has no disk side effects *and* that seeding creates all nine folders |
| `scripts/packaging/windows/test_msi_install.ps1` | New `Get-AdvertisedShortcutTarget` (msi.dll P/Invoke) + rewritten Start Menu check |
| `scripts/seamly-build-msi/task51-test-kit/test_msi_install.ps1` | Refreshed copy (gitignored) |
| `project-docs/TODO_MIGRATE.md` | Task 51 "Progress 2026-07-30" + two new subtasks |

### Verification

Build exit 0 · `scripts/st.ps1` **32134 passed, 0 failed across 25 suites** (`TST_DataRoot` 22 → 23) · `ParserTest` exit 0 · `TranslationsTest` exit 0. **`CollectionTest` was deliberately not run locally** — it has a documented pre-existing failure *and* it launches the real seamly2d, which now seeds folders into the live `G:\My Drive\seamlyData`. CI runs it on a clean machine.

### MACHINE STATE: the Visual Studio installation is broken (not caused by anything here)

**`scripts/sd.ps1` fails with `'cl' is not recognized`.** It is not the script and not the agent sandbox — the same failure occurs with the sandbox disabled:

- `vcvars64.bat` **and** `vcvarsall.bat x64` exit 1 with `[ERROR:VsDevCmd.bat] *** VsDevCmd.bat encountered errors ***`; three sub-scripts fail to init — `core\msbuild.bat`, `ext\cmake.bat`, `ext\ConnectionManagerExe.bat`
- The toolset is fine on disk: `cl.exe` 19.51.36252 under `VC\Tools\MSVC\14.51.36231`, Windows SDKs 10.0.22621.0 / 10.0.26100.0 present
- **Plain `vswhere -products *` returns nothing**; only `vswhere -all -prerelease -legacy` finds `C:\Program Files\Microsoft Visual Studio\18\Community`. This VS is *"Visual Studio 2026 Developer Command Prompt v18.8.1"*, a prerelease build, and the instance registration looks damaged. Instance data exists at `C:\ProgramData\Microsoft\VisualStudio\Packages\_Instances\a9afd7ad`

**Workaround used for this session's build, local only — nothing on the machine and nothing in `sd.ps1` was changed:** set `PATH`/`INCLUDE`/`LIB` by hand at `VC\Tools\MSVC\14.51.36231` + SDK `10.0.26100.0`, then run `C:\Qt\Tools\QtCreator\bin\jom\jom.exe -f Makefile` in `scripts/seamly2d-build-debug`. **The user should repair VS 18 Community from the Visual Studio Installer** — until then `sd.ps1` fails for them too.

### Open questions from the laptop run (needed to close subtasks)

- **ARP `DisplayIcon` came back empty**, although `ARPPRODUCTICON = seamly2d.ico` *is* in the built MSI's Property table and the Icon table has the matching `seamly2d.ico` row (both verified locally by querying the package). Authoring is correct; cause unknown. Asked for `MsiGetProductInfo`'s `ProductIcon` and whether Apps & features paints the right icon — **not yet answered**
- **The old NSIS install was gone** by the `Installed` phase (both `HKLM\SOFTWARE\WOW6432Node\NSIS_Seamly2D` and `C:\Program Files (x86)\Seamly2D`). The package contains **no `CustomAction` at all**, so it cannot have removed it; the tester most likely uninstalled it as the dialog advises. **Not yet confirmed** — if it was *not* the tester, this needs a proper investigation
- Which paragraph the existing-installation page showed on the upgrade (should be the *upgrade* one, since NSIS was gone) — **not yet answered**
- `-Phase Upgraded` and the uninstall + `-Phase Removed` legs — **not yet run**. Note the "uninstall preserves user data" check is currently **vacuous on that laptop** because no user data exists; create a saved pattern first if that guarantee is to be tested for real

### Next steps

1. Fix `SeamlyShortcutsDlg` (UI-only test MSI, per above) and the 373→370 geometry; assert both in `test_msi_authoring.ps1` *and* confirm with a real wizard run, since authoring passed while the page never appeared.
2. Finish the laptop cycle: `-Phase Upgraded`, uninstall, `-Phase Removed`, with the refreshed checker copied over.
3. Repair Visual Studio, then re-run `scripts/sd.ps1` to confirm the normal build path works again.
4. Push the branch and open the PR to `run-seamlyLayout` once Task 51's remaining subtasks land.

## Earlier state (2026-07-29): Task 51 — the install cycle is now scripted and a test kit is staged; it awaits a run on the test Windows 11 laptop

**Branch `task-51-msi-install-experience`.** Task 51 stays in `project-docs/TODO_MIGRATE.md` with the same five subtasks open — nothing was installed anywhere, so nothing could be checked off. What changed is that those subtasks are now *executable* instead of a prose checklist.

### The decision that shaped this session

The only work left in Task 51 is an elevated install/upgrade/uninstall cycle. **There is no VM on this PC, and there will not be one.** Asked where to run it, **the user chose the test Windows 11 laptop** (2026-07-29), not the developer PC. So the deliverable became a portable, self-contained verification kit rather than a run.

**Do not propose a VM again (re-asked and re-declined 2026-07-30).** This PC runs **Windows 11 Home**, which ships neither Hyper-V nor Windows Sandbox — both are Pro/Enterprise-only — so any VM here means installing a third-party hypervisor. The hardware would be fine (Ryzen 9 7900X 12c/24t, 63 GB RAM, 478 GB free on C:, 863 GB on E:; VBS is running, so VirtualBox/VMware would use Hyper-V-backed mode). The user considered VirtualBox and VMware Workstation Pro and **chose to keep testing on the laptop**. Note that a VM could not close two of the checklist items anyway — the *verified-publisher* UAC prompt needs Task 33's KMS signing, and the arm64 repeat needs arm64 hardware.

### What was built

| File | Change |
| ---- | ------ |
| `scripts/packaging/windows/test_msi_install.ps1` | **New.** Verifies a *real install* in four phases — `Baseline` / `Installed` / `Upgraded` / `Removed` — run around the `msiexec` commands, sharing a JSON state file. Standalone: no repo, build tree or Qt needed on the test machine |
| `scripts/packaging/windows/README.md` | "Installing / testing" rewritten into "The scripted cycle" + "What still needs human eyes"; the file table gained the new script |
| `scripts/packaging/windows/README_WINDOWS_BUILD.md` | The user's own trim of the historical §3 problem sections, **plus** repair of the two references it broke (see below) |
| `project-docs/TODO_MIGRATE.md` | Task 51 "Progress 2026-07-29" paragraph; the four verify subtasks and the cycle subtask annotated with what the script now covers; Task 33's stale `§6` pointer fixed |
| `scripts/seamly-build-msi/task51-test-kit/` | **Gitignored, ~330 MB.** Both MSIs, the checker, `sample-pattern.sm2d`, and `RUN-ME-FIRST.md` |

### Two packages exist, and the pair matters

The upgrade leg cannot be tested with one MSI — re-running the same package is a *repair*, not an upgrade. `smsi.ps1` derives ProductVersion from the build timestamp and generates a fresh ProductCode per build, so two builds share the fixed UpgradeCode and genuinely major-upgrade each other:

| Package | project version | ProductVersion | ProductCode |
| --- | --- | --- | --- |
| `Seamly2D-x64-older.msi` | 2026.7.28.2355 | 26.7.40315 | `{DE4AB233-…}` |
| `Seamly2D-x64-newer.msi` | 2026.7.29.0041 | 26.7.40361 | `{BEB6C667-…}` |

The rebuild that produced the newer one ran clean: `wix build` OK, `test_msi_authoring.ps1` passed, 165.4 MB.

### What the checker covers, and the two judgement calls in it

It asserts the installed files and a slice of the Qt runtime; the Start Menu and desktop shortcuts and their targets; the `HKLM\SOFTWARE\Seamly\Seamly2D` rows including the desktop-shortcut breadcrumbs; the Apps & features entry (found by **UpgradeCode**, because the old NSIS product shares the DisplayName "Seamly2D"); all three associations in the registry *and* opening a real `.sm2d` via `Start-Process`, which goes through ShellExecute — the same route Explorer takes on a double-click; that an upgrade leaves exactly one ARP entry, a changed version and an unmoved install directory; that uninstall removes all of it; and that `seamlyData`, `%LOCALAPPDATA%\Seamly`, `%APPDATA%\Seamly` and any NSIS install survive.

**The highest-value check is that it starts each app and confirms it stays running.** A missing Qt DLL or QML module kills the process in about a second, and no amount of package inspection can see that — it is exactly the class of bug §2 of `README_WINDOWS_BUILD.md` records twice.

- **User data is checked as "never shrank", not "identical".** Starting the apps legitimately creates settings and seeds the data tree, so an exact-match test would fail for the right reasons. What must never happen is a file disappearing.
- **The effective file association is reported, not asserted.** A per-user `UserChoice` overrides the machine-wide registration, so HKLM being correct is all an installer can be held to.

### Verification of the checker itself (it was not written and left untested)

- `Baseline` on this PC **passes**, correctly reporting the NSIS install at `C:\Program Files (x86)\Seamly2D` and the three user-data trees (`G:\My Drive\seamlyData` 8751 files / 16.8 GB, and the two settings dirs). 2.3 s even against the cloud drive.
- A deliberate **negative** run — `-Phase Installed` on a machine with nothing installed — **fails with exit 1** and names the two failing expectations. So the checks are known not to be vacuous, which is the failure mode a checker like this actually has.
- Fixed while testing: a failed phase used to blank `InstallFolder` in the state file, which would have silently disabled the `Removed` phase's leftover-file check — turning one real failure into a false pass later. Now only non-empty values overwrite state.

### The user's concurrent edits, carried in this commit

Two edits were in the working tree and are **not mine**: the trim of `README_WINDOWS_BUILD.md`'s historical §3 sections, and **Task 54's naming decision being settled in `TODO_MIGRATE.md`** (the A/B table collapsed to the class-match form — `SettingsCommon.h` etc.). That answers Task 54's first subtask; treat it as decided.

That trim renumbered the file's sections and deleted §3.1/§3.5, leaving two **broken pointers** which are now repaired: the prerequisites table's `(see §3.1)` / `(see §3.5)` were rewritten to name §2 instead of a deleted number, and Task 33's `README_WINDOWS_BUILD.md §6` became `§5`.

### Next steps

1. **Copy `scripts/seamly-build-msi/task51-test-kit/` to the test Windows 11 laptop and follow `RUN-ME-FIRST.md`** — elevated PowerShell, `Start-Transcript`, five steps, ~20 minutes. The transcript closes Task 51's four verify subtasks and its cycle subtask, and **Task 13**'s last subtask with them. Note the laptop already had the old standalone install per Task 38, so the NSIS warning paragraph should appear on the first install.
2. **Task 14** — the check-and-move flow for an existing data tree (also needed by Tasks 35/36/37; satisfies Task 38).
3. **Task 52** — the `vsettings.cpp` "Unknown Organization" stray, starting with its `CollectionTest` isolation subtask.
4. **Task 54** — the file-name form is now settled by the user; the 22 `.ts` `tr()` contexts must still move in the **same commit** as the classes.
5. **Task 55** — the developer-README refresh; the rename to `.github/README-DEVELOPER-SEAMLY-FAMILY.md` has still not been done.
6. `src/app/seamly2d/core/BUILD_PROBLEMS.txt` — the user said to delete it if it is not useful; still not done.

## Earlier state (2026-07-28): Task 51 — the MSI install-time experience is authored and machine-checked; only the clean-VM cycle is left

**Branch `task-51-msi-install-experience`, off `run-seamlyLayout` (`39e9512637`).** `develop` was already merged into `run-seamlyLayout`, so no sync was needed. Task 51 stays in `project-docs/TODO_MIGRATE.md` — 5 of its 9 subtasks are checked off, 4 need an elevated install on a clean machine.

### What was done

| File | Change |
| ---- | ------ |
| `scripts/packaging/windows/seamly-family.wxs` | Two install-time dialogs (`SeamlyPreviousInstallDlg`, `SeamlyShortcutsDlg`), the NSIS registry searches, `SEAMLYDESKTOPSHORTCUTS`, `ARPCOMMENTS`, and two conditional desktop-shortcut components |
| `scripts/packaging/windows/test_msi_authoring.ps1` | **New.** ~50 assertions against the built MSI's database (elevation, ARP, upgrade + NSIS detection, both dialogs and the warning's wording, shortcuts, associations, registry rows) |
| `scripts/packaging/windows/smsi.ps1` | Runs that check after `wix msi validate`; suppresses ICE43/ICE57 with the reasoning inline |
| `scripts/packaging/windows/README.md` | New "Install-time experience (Task 51)" section with all seven decisions; the manual clean-machine checklist rewritten and now kept here only |
| `scripts/packaging/windows/README_WINDOWS_BUILD.md` | Wizard walkthrough, the new build step, §3.5 on the local Qt kit gap |
| `project-docs/TODO_MIGRATE.md` | Task 51 subtasks checked off / annotated with what was verified statically |

### Decisions worth not reversing

- **Neither dialog is wired by publishing a second `NewDialog` on `InstallDirDlg`'s Next button**, the obvious way to insert a WixUI page. Two unconditionally-true `NewDialog` events on one control is undefined behaviour — WixUI itself never does it (its competing publishes all carry mutually exclusive conditions) and the built-in row's condition is the literal `1`, so nothing can exclude it. The warning page is a `Show` in `InstallUISequence` (1250, before WixUI's first dialog at 1296); the shortcuts page is a `SpawnDialog` at `Ordering` 2, ahead of the built-in `NewDialog` at 4 — the mechanism WixUI uses for its own `BrowseDlg`. Both are correct whichever way that ambiguity resolves.
- **Never write that as `Before="WelcomeDlg"`.** Every WixUI dialog set defines `InstallUISequence/WelcomeDlg`, so the reference pulls `WixUI_Minimal` and `WixUI_Advanced` into the link beside `WixUI_InstallDir` and the build dies on duplicate `TextStyle`/`Property`/`WixAction` symbols. Hit and diagnosed; the sequence number is written out with a comment.
- **The NSIS search must be `Bitness="always32"`.** The old installer is 32-bit and never switches views, so its keys are under `WOW6432Node`; verified against the real NSIS install on this PC (`HKLM\SOFTWARE\WOW6432Node\NSIS_Seamly2D\Install_Dir` = `C:\Program Files (x86)\Seamly2D`).
- **The NSIS install is detected and explained, never removed.** Its uninstaller is interactive, its uninstall section is `RMDir /r $INSTDIR`, and MSI cannot roll back an external uninstaller.
- **One desktop-shortcut checkbox for seamly2d + seamlyme, none for SeamlyLayout** (a desktop launch of a document-driven app shows an empty canvas). **No taskbar-pinning checkbox at all** — Windows 10+ blocks programmatic pinning, so it would silently do nothing.
- **ICE43 and ICE57 are suppressed, and only those two.** Both assume `DesktopFolder` is in the user profile, true only of a per-user install. Obeying them would be actively wrong: the server side of a per-machine install runs as LocalSystem, so an HKCU key path lands in the SYSTEM hive and every launch triggers self-repair.
- **ARP's DisplayVersion cannot show the project version** — `RegisterProduct` overwrites it after `WriteRegistryValues`. It reaches the user via `ARPCOMMENTS` and `HKLM\SOFTWARE\Seamly\Seamly2D\DisplayVersion` instead.

### Verification

`smsi.ps1` (the default, all three apps) exit 0 — `wix build` clean, `wix msi validate` clean apart from the expected ICE61, `test_msi_authoring.ps1` **51/51**, `Seamly2D-x64.msi` **165.4 MB**. The two-app package (`-NoSeamlyLayout`, the arm64 shape) passes **48/48**. **No install was performed** — the user chose static verification only, so the four "verify on a real install" subtasks are explicitly still open.

Sequence note: the three-app build was only possible after the user reinstalled the missing Qt modules mid-session (below). Before that, the three-app *authoring* was verified by building with `-d IncludeSeamlyLayout=1` against a hand-staged exe; the run recorded above is the genuine one, with SeamlyLayout's real Qt runtime deployed.

Two bugs in the new checker were found and fixed while writing it, both worth remembering: **rows must be PSCustomObjects, not arrays** (a row array passing through a pipeline gets unrolled, so `(… | Where-Object {…}).Count -eq 1` counts the matched row's *fields* and reports 2 for a single two-column match), and **the MSI `Shortcut.Name` column stores `short|long` for names over 8.3**, so `SeamlyLayout` is `SEAMLY~1|SeamlyLayout` while `Seamly2D` and `SeamlyMe` are plain.

### Machine state (not in git) — the local Qt kit was incomplete, and the user fixed it mid-session

**`C:\Qt\6.11.1\msvc2022_64` had Qt WebEngine but no Qt WebChannel and no Qt Positioning**, so `windeployqt6` failed with *"Unable to find dependent libraries … Qt6WebChannelQuick.dll"* and the three-app MSI could not be built here at all. **The user reinstalled the Qt WebEngine and WebChannel extensions on 2026-07-28**; the kit now has `Qt6WebChannel[Quick].dll` and `Qt6Positioning[Quick].dll` in `bin\`, `qml\QtWebChannel` and `qml\QtPositioning`, and the matching CMake packages, and the default `smsi.ps1` produces the full package again.

The lesson worth keeping: **`src/app/seamlylayout/build.ps1`'s guard probes the `Qt6WebEngineQuick` CMake package, which was present the whole time**, so the guard passed while deployment was impossible. A kit can satisfy `find_package(Qt6 … WebEngineQuick)` and still be undeployable. Recorded as §3.5 of `README_WINDOWS_BUILD.md`. CI was never affected — `install-qt-action` installs the full module list.

### Next steps

1. **Task 51's remaining four subtasks** — the elevated install/upgrade/uninstall cycle on a clean Windows x64 VM, working through the checklist in `scripts/packaging/windows/README.md`. Closes **Task 13**'s last subtask too.
2. **Task 14** — the check-and-move flow for an existing data tree (also needed by Tasks 35/36/37; satisfies Task 38).
3. **Task 52** — the `vsettings.cpp` "Unknown Organization" stray, starting with its `CollectionTest` isolation subtask.
4. **Task 54** — rename the three `vmisc` settings files *and* their classes (`SettingsCommon.h`); the 22 `.ts` `tr()` contexts must move in the **same commit**.
5. **Task 55** — the developer-README refresh; the rename to `.github/README-DEVELOPER-SEAMLY-FAMILY.md` has still not been done.
6. `src/app/seamly2d/core/BUILD_PROBLEMS.txt` — the user said to delete it if it is not useful; still not done.

## Earlier state (2026-07-27): Task 59 DONE — the handoff now lays out, not just opens

**Merged.** PR [#23](https://github.com/seamly/Seamly2D/pull/23) (`task-59-nested-piece-extraction` → `run-seamlyLayout`), commit `34f66462d5`, merge commit `11c0b0f4c5`. **All 13 CI checks green** — Windows x64 27m21s, Windows arm64 cross-compile 26m6s, macOS 8m53s, Linux AppImage 8m41s, Linux unit tests 9m28s, **Linux: Build & test SeamlyLayout (Qt 6.11) 4m57s**, CodeQL, CodeSee, Analyze actions/python/rust, version. Local and remote task branches deleted; local `run-seamlyLayout` = `origin/run-seamlyLayout` = `11c0b0f4c5`.

Task 59 moved from `project-docs/TODO_MIGRATE.md` to `project-docs/TODO_COMPLETED.md` (full write-up at the top of that file).

### The bug and the shape of the fix

`SvgGenerator` wraps all 12 pieces in one `<g id="pattern-1" data-type="pattern">`, but `piece_extractor::extract_piece_rects()` treated each **direct child `<g>` of the SVG root** as a piece. The packer got one sheet-sized object: `0 placements, 1 unplaced: ["pattern-1"]`.

**The decision that matters: normalise the document once, do not teach eight call sites a new tree shape.** `extract_piece_rects` is far from the only consumer of the flat assumption — `extract_piece_rects_and_polygons`, `layout_assembler::create_layout`, `oversized::build_oversized_svg` and `remaining::build_sheet_doc` all resolve a piece to its element through `PieceRect::group_index` (an index into the root's `<g>` children), and **`svg_dom::verticalize_dom` / `svg_dom::translate_dom` iterate `doc.root.children` directly**, so a wrapper would have verticalized and translated the whole pattern as one piece. New `piece_extractor::hoist_tagged_pieces()` re-parents the tagged pieces to the root before any of that runs; every other stage is untouched and `group_index` stays valid.

### Files changed (all under `src/app/seamlylayout/` — no Seamly2D parent source was touched)

| File                                                                        | Change                                                                                                                                                                                                                                                                                                                                          |
| --------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `crates/cxxqt_bridge/src/piece_extractor.rs`                              | New`hoist_tagged_pieces()` + helpers (`take_tagged_pieces`, `join_transforms`, `is_tagged_piece`, `document_has_tagged_pieces`, `has_nested_tagged_piece`, `piece_identity`). Both extractors now select only `data-type="piece"` groups in tagged mode. `PieceRect` gained `name` / `letter` / `label()`. 10 new tests |
| `crates/cxxqt_bridge/src/layout_utils.rs`                                 | Calls the hoist on the`input_dom` clone (stage 2a); `label()` in the unplaced warning and all three `PackError` messages; `name`/`letter`/`label` added to the bbox JSON; new `mod tests` with the full-pipeline regression test                                                                                                  |
| `crates/cxxqt_bridge/src/sheets.rs`                                       | `build_sheet_export_inputs` calls the hoist first — it mirrors the same preprocessing pipeline for sheet-mode PDF export                                                                                                                                                                                                                     |
| `crates/cxxqt_bridge/src/lib.rs`                                          | Exports`hoist_tagged_pieces`; `get_adjust_piece_boxes` emits `name`/`letter`/`label`                                                                                                                                                                                                                                                  |
| `crates/cxxqt_bridge/src/{oversized,remaining}.rs`                        | Test`PieceRect` helpers updated for the two new fields                                                                                                                                                                                                                                                                                        |
| `qt_frontend/src/adjust/PieceOverlayItem.{h,cpp}`                         | New`setDisplayLabel()` / `displayLabel()`; the context menu header shows the name, not the id. **Deliberately a setter, not a constructor parameter** — the constructor signature is used by `AdjustSceneTests`                                                                                                                    |
| `qt_frontend/src/adjust/AdjustScene.cpp`                                  | Reads`label` from the bbox JSON and passes it to the overlay item                                                                                                                                                                                                                                                                             |
| `crates/cxxqt_bridge/test_data/richmond-shirt-handoff_pieces.svg`         | **New fixture, 101 KB** — genuine exporter output, `include_str!`'d by the pipeline test. **Deliberately not in `input/`:** `include_str!` makes it a compile-time dependency that must be tracked unconditionally, whereas `input/` is tracked only until development is complete                                         |
| `src/app/seamlylayout/CLAUDE.md`, `project-docs/SVG-DATA-ATTRIBUTES.md` | The discovery + identity contract on both sides                                                                                                                                                                                                                                                                                                 |

### Decisions worth not reversing

- **The tagged/untagged decision searches the whole tree**, not just the root's children. My first version checked only root children, and a test caught it: an un-hoisted handoff then fell back to the untagged rule and packed the wrapper. Now such a document yields **zero** pieces → the visible "No pattern pieces found" error, which is a much better failure than a silently wrong layout.
- **Wrapper transforms are composed onto each hoisted piece** (`"<ancestor> <own>"`). Seamly2D writes no transform on the pattern group today, so this is a no-op in practice — kept and tested because a silently moved piece would be very expensive to find.
- **A wrapper that still holds non-piece content is kept**, not deleted; it simply cannot pack, because tagged mode only accepts `data-type="piece"`.
- **`id` stays the identity key everywhere.** `label()` (`data-name` → `data-letter` → `id`) is display-only.

### Verification

`cargo test --workspace` **265 passed / 0 failed** (252 at Task 49) · SeamlyLayout `ctest --preset debug` **5/5** · `build.ps1 -Preset debug` clean, exit 0 · launching `SeamlyLayout.exe <handoff>` still logs `[import_svg] 12 tagged pattern piece(s) found`.

**The end-to-end check is now a permanent test, not a one-off run.** `layout_utils::tests::richmond_shirt_handoff_packs_twelve_individual_pieces` drives `do_initialize_layout` + `do_process_layout` — the exact `ProcessLayoutArgs` the QML `process_layout` wrapper builds — against the committed exporter output, asserting 12 placements, 0 unplaced, 12 *distinct* slot positions, real piece names, and no `pattern-1`. **Confirmed load-bearing:** disabling the hoist call makes it fail. The GUI "Create Layout" button was *not* clicked (no GUI automation here); the test covers the identical code path.

### `input/` SVGs are now tracked — standing user decision (2026-07-27)

`src/app/seamlylayout/.gitignore` ignored `/input` outright, so only the files that happened to be carried in by Task 19's directory move were tracked; `richmond-shirt-baseline_pieces.svg` and the two `MyMullerShirt-2_layout_*.svg` files were **untracked and invisible**, and earlier handover notes referred to the baseline SVG as though it were in the repo. On the user's instruction — *"track the input SVGs, we'll track these until we've completed development"* — the rule is now:

```gitignore
/input/*
!/input/*.svg
!/input/*.sm2d
```

`/input/*` rather than `/input` matters: **git does not descend into an ignored directory**, so negations under a bare `/input` are never evaluated. Tracking is now the default there, instead of depending on someone remembering `git add -f`.

`input/2025-06-08-Sue.smis` (a measurements file) is **still ignored** — the instruction named SVGs, and the pattern `.sm2d` was already tracked. Add `!/input/*.smis` if it should be tracked too.

**When development is complete this is meant to be reverted**, which is exactly why the Task 59 test fixture lives in `crates/cxxqt_bridge/test_data/` and not here: `include_str!` makes it a compile-time dependency that must be tracked unconditionally.

### Machine-state note (not in git)

A **stale `SeamlyLayout.exe` debug process from a previous session (PID 37316, 01:47) held the build outputs locked** and would not die — `taskkill` reported "no running instance" while `Get-Process` still listed it with `HasExited=True`. `build.ps1`'s own `Stop-Process` could not clear it either. Fix: rename `SeamlyLayout.exe`, `SeamlyLayout.pdb` and `SeamlyLayout.lib` aside (renaming worked where deleting/killing did not), rebuild, then delete the `*.stale` files. Worth trying first if a link fails with LNK1168 / LNK1201.

### Next steps

1. **Task 51** — the Windows MSI install-time experience; its upgrade-warning wording must say **`seamlyData`**.
2. **Task 14** — the check-and-move flow for an existing data tree (also needed by Tasks 35/36/37; satisfies Task 38).
3. **Task 52** — the `vsettings.cpp` "Unknown Organization" stray, starting with its `CollectionTest` isolation subtask.
4. **Task 54** — rename the three `vmisc` settings files *and* their classes (`SettingsCommon.h`); the 22 `.ts` `tr()` contexts must move in the **same commit**.
5. **Task 55** — the developer-README refresh; the rename to `.github/README-DEVELOPER-SEAMLY-FAMILY.md` has still not been done.
6. `src/app/seamly2d/core/BUILD_PROBLEMS.txt` — the user said to delete it if it is not useful; still not done.

Blocked, not startable here: Tasks 13/38/39/40 (clean VM, arm64, macOS, Linux hardware), Task 33/41 (KMS credentials).

## Earlier state (2026-07-27): Task 49 DONE and MERGED — the Seamly2D → SeamlyLayout handoff finally opens the pattern

**Merged.** PR [#22](https://github.com/seamly/Seamly2D/pull/22) (`task-49-seamlylayout-svg-argument` → `run-seamlyLayout`), commit `f720df1b63`, merged by the user with a fast-forward merge and pushed to `origin/run-seamlyLayout`. **All 13 CI checks green** — Windows x64 (28m19s), Windows arm64 cross-compile (26m16s), macOS (12m53s), Linux AppImage, Linux unit tests, **Linux: Build & test SeamlyLayout (Qt 6.11) 5m5s** (the leg that runs the new `StartupOptionsTests`), CodeQL, CodeSee, Analyze actions/python/rust, version. Local task branch deleted; the remote one was deleted on merge.

The user then committed `fc487aec93` "pass svg argument to seamlyLayout" on top — **no code**, only `.claude/settings.local.json` (this session's auto-approval entries) and two screenshots into `project-docs/`.

**`.claude/settings.local.json` is no longer tracked** (`84724ac5c2`). It had been tracked *deliberately*: `.gitignore:189` read `!.claude/settings.local.json`, an explicit negation commented "track Claude Code personal overrides (shared settings.json is committed)", and no other rule in the file matched that path. The harness rewrites it on every permission grant, so it kept accumulating absolute `C:\Users\susan\…` paths and session ids — the same class of leak Task 50's coding rule targets, in a repo headed for the upstream PR. Untracking alone would not have held: with only the negation removed the file shows as *untracked* rather than ignored, and the next `git add -A` re-commits it (which is exactly how it reached `fc487aec93`), so the negation was replaced by a plain ignore rule at `.gitignore:193`. The shared `.claude/settings.json` is unaffected and stays committed. **To reverse:** flip line 193 back to `!.claude/settings.local.json` and re-add the file.

Local `run-seamlyLayout` = `origin/run-seamlyLayout` = `fc487aec93`; local `develop` = `origin/develop` = `057e95bfca`.

### What Task 49 changed

SeamlyLayout read `argc`/`argv` only to hand them to `QApplication`, so the `.pieces.svg` seamly2d wrote and passed was discarded and the window came up empty.

| File                                                                        | Change                                                                                                                                                                                                                                                                                               |
| --------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/app/seamlylayout/qt_frontend/src/StartupOptions.{h,cpp}`             | **New.** Value class (no QObject, no GUI) parsing the one positional `<svg-file>` with `QCommandLineParser::parse()` — the non-exiting sibling of `process()` — and validating it. Four statuses: `NoFile`, `OpenFile`, `ShowInformation` (`--help`/`--version`), `Failed` |
| `src/app/seamlylayout/qt_frontend/main.cpp`                               | Parses after the app metadata is set (so`--version` can report it); dispatches on `QTimer::singleShot(0, …)` **after** the event loop starts, because the QML window and its WebEngine canvases must exist first                                                                          |
| `src/app/seamlylayout/qt_frontend/qml/Main.qml`                           | New`openSvgFile(localPath)` (the file dialog now calls it too — one entry point) and `reportStartupError(message)`; new `onImportWarning` handler                                                                                                                                             |
| `crates/cxxqt_bridge/src/piece_extractor.rs`                              | New`count_tagged_pieces()` (recursive `data-type="piece"` count) + 3 unit tests                                                                                                                                                                                                                  |
| `crates/cxxqt_bridge/src/lib.rs`                                          | New`import_warning` qsignal; `import_svg` emits it when the imported SVG carries no piece tagging — **a warning, never an error**, because untagged SVGs still lay out                                                                                                                    |
| `src/libs/vmisc/seamly_family_paths.{cpp,h}`                              | New`piecesSvgFilePath()` and `seamlyLayoutLaunchArguments()` — the seamly2d half of the contract, extracted out of `mainwindow.cpp` so it has one definition and one test                                                                                                                     |
| `src/app/seamly2d/mainwindow.cpp`                                         | `exportPiecesToSeamlyLayout()` now calls both; added the `seamly_family_paths.h` include                                                                                                                                                                                                         |
| `src/test/SeamlyLayoutTest/StartupOptionsTests.cpp`                       | **New**, 5th ctest target (guiless), 18 cases                                                                                                                                                                                                                                                  |
| `src/test/Seamly2DTest/tst_seamlyfamilypaths.{cpp,h}`                     | 6 new cases for the contract                                                                                                                                                                                                                                                                         |
| `src/app/seamlylayout/CLAUDE.md`, `project-docs/SVG-DATA-ATTRIBUTES.md` | The contract, written down on both sides                                                                                                                                                                                                                                                             |
| `project-docs/TODO_MIGRATE.md` / `TODO_COMPLETED.md`                    | Task 49 moved across;**new Task 59 filed** (below)                                                                                                                                                                                                                                             |
| `.claude/settings.json`                                                   | Allowlist entries for the repo's build/test scripts +`ctest` (the user asked for this mid-session after a prompt on `sd.ps1`)                                                                                                                                                                    |

### Decisions recorded in the contract (do not quietly reverse them)

- **No single-instance handling** — every launch is its own process and window; one document per process (no tabs), which is also why a *second* positional argument is rejected rather than queued.
- **A bad argument does not exit.** The message goes to the QML error dialog and the app stays open with an empty canvas — a detached launch has no console to print to.
- **`--help`/`--version` go to a `QMessageBox`**, because this is a WIN32-subsystem binary with no console on Windows.
- **Untagged SVGs are opened, not refused** — warning only.

### Verification (all local, all passing)

`scripts/sd.ps1` exit 0 · `scripts/st.ps1` **32133 passed / 0 failed, 25 suites** (`TST_SeamlyFamilyPaths` 15 → 21) · `ParserTest` 0 · `TranslationsTest` 0 · SeamlyLayout `ctest --preset debug` **5/5** (`StartupOptionsTests` 19 passed / 1 skipped — the unreadable-file case, NTFS) · `cargo test --workspace` **252 / 0 across 20 targets**.

**End-to-end, with a genuine handoff file:** `seamly2d.exe <pattern>.sm2d -b handoff -d <dir> -f 0 --exportOnlyDetails` produces a tagged SVG through the same `exportSVG()` `generatePiecesSvg()` uses (12 `data-type="piece"` groups). `SeamlyLayout.exe <that file>` logs `main(): opening startup file …` then `[import_svg] 12 tagged pattern piece(s) found`. The untagged baseline SVG logs the `no data-type="piece" groups` warning; a non-existent path logs the startup error. **All three paths confirmed.**

### THE CollectionTest LOOSE END IS CLOSED — it is pre-existing

The previous session's unfinished baseline check was completed here. With every Task 49 source change stashed and the tree rebuilt, `TST_Seamly2DCommandLine::TestOpenCollection(07_armhole_adjustment_010)` fails **identically** (42 passed / 1 failed). Unrelated to Tasks 49/50. Two facts worth keeping:

- **`CollectionTest.exe` must be run with its working directory set to its own `bin/`.** `initTestCase()` removes `tst_seamly2d_tmp` *relative to the CWD* but creates it under `applicationDirPath()`, so running it from anywhere else aborts on a leftover directory from the previous run ("Fail to prepare test files for testing").
- The failure presents as either "Program crashed" or "The finish operation timed out" against `AbstractTest::Run()`'s 120 s limit.

### NEW — Task 59, filed not fixed: the layout packs the whole pattern as one piece

Found by the end-to-end run and **the most valuable thing in this session after the fix itself.** `piece_extractor::extract_piece_rects()` treats each direct child `<g>` of the SVG root as a piece, but the tagged handoff nests all 12 pieces inside one `<g id="pattern-1" data-type="pattern">`. The packer therefore receives a single sheet-sized object: `0 placements, 1 unplaced: ["pattern-1"]`. Task 49 made the handoff *open*; Task 59 (`project-docs/TODO_MIGRATE.md`, bottom) makes it *lay out*. It should be the next task.

### Loose ends carried forward

- `src/app/seamly2d/core/BUILD_PROBLEMS.txt` — the user said to delete it if it is not useful; **not done in this session** (out of Task 49's scope).
- ~~`.claude/settings.local.json`~~ — **resolved in `84724ac5c2`**, see the current-state section above. It is untracked and genuinely ignored now.
- The user's own uncommitted edits to `SESSION_HANDOVER.md` and `project-docs/TODO_SEAMLYLAYOUT.md` (Task 57 deleted) were present at session start and went into the Task 49 commit.

### NEXT STEPS — this list supersedes the older "Concrete next steps" further down

1. **Task 59** — the layout stage packs the whole pattern as one piece (see above). Highest value: Task 49 made the handoff open, this makes it *work*. `project-docs/TODO_MIGRATE.md`, bottom of the file.
2. **Task 51** — the Windows MSI install-time experience plus the clean-machine install/upgrade/uninstall cycle. Its upgrade-warning wording must say **`seamlyData`**.
3. **Task 14** — the check-and-move flow for an existing data tree (nine subtasks). Shared cross-platform code; also needed by Tasks 35/36/37 and satisfies Task 38.
4. **Task 52** — the `vsettings.cpp` "Unknown Organization" stray, **starting with** its `CollectionTest` isolation subtask.
5. **Task 54** — rename the three `vmisc` settings files *and* their classes. The blocking decision is now answered: **`SettingsCommon.h`**, file name matching the class name. Wide but mechanical — ~620 class occurrences over 25 files, and the 22 `.ts` `tr()` contexts must move in the **same commit** or ~220 translated strings go obsolete.
6. **Task 55** — the developer-README refresh. Per the user's answer, the target is now `.github/README-DEVELOPER-SEAMLY-FAMILY.md` (renamed from `-NEW`), maintained separately until the migration completes and then folded into `README-DEVELOPER.md`. **Neither rename nor fold has been done yet.**
7. **Task 57** — premise superseded by the style-guide carve-out; decide whether to delete it (as Task 56 was) or keep only the `error.rs` ×2 collision. *(The user deleted this task from `project-docs/TODO_SEAMLYLAYOUT.md` in an uncommitted edit that the Task 49 commit carried in, so this may already be closed — check that file first.)*

Blocked, not startable here: Tasks 13/38/39/40 (clean VM, arm64, macOS, Linux hardware), Task 33/41 (KMS credentials).

## Earlier state (2026-07-27, later session): Tasks 45 and 50 done — committed and pushed to `run-seamlyLayout`

**Branch:** `run-seamlyLayout`, committed directly (no PR) — the user explicitly asked for stage + commit + push to origin `run-seamlyLayout`, skipping the `develop` pull and the task-branch/PR cycle in `CLAUDE.md`.

### What was asked

The session opened with an analysis question: *which task in `project-docs/TODO_MIGRATE.md` should be done next?* The answer given was **Task 49** (SeamlyLayout ignores the SVG path argument — verified still true: `qt_frontend/main.cpp` passes `argc`/`argv` to `QApplication` and never reads them; `AppController::import_svg()` already exists at `crates/cxxqt_bridge/src/lib.rs:863` and is what the QML Import button calls at `qml/Main.qml:850`, so the plumbing for the fix is in place). Every other open task in that file is blocked on hardware (clean VM / arm64 / macOS / Linux), on KMS credentials, or on a user decision. **The user then chose Tasks 45 and 50 instead.** Task 49 remains the recommended next item.

### Task 45 — stale Qt 6.10.1 paths in the Claude allowlists (DONE, moved to `project-docs/TODO_COMPLETED.md`)

Both entries turned out to be **redundant with broader rules already present**, so the fix is removal, not a version bump — which makes them permanently version-agnostic:

- `.claude/settings.json:154` — the compound `Test-Path "C:\Qt\6.10.1\…"; & …vswhere.exe …` was already covered by `PowerShell(Test-Path *)` on line 10. Replaced with one version-agnostic prefix entry naming no Qt version: `PowerShell(& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" *)`
- `.claude/settings.local.json:17` and `:19` — both deleted. That file opens with `PowerShell(*)` / `Bash(*)`, which already allow everything in it. ~~**This file is gitignored (`.gitignore:189`)**, so that half exists only on this machine and is *not* in the commit~~ — **wrong, corrected 2026-07-27:** `.gitignore:189` was `!.claude/settings.local.json`, a *negation* that force-tracked the file. The deletions were left out of commit `4a6054ab14` because they were never staged, not because git was ignoring them; they reached origin later inside the user's `fc487aec93`. (Verified: no `6.10.1` entry survives anywhere.) The file is untracked and ignored for real as of `84724ac5c2`

Both files re-validated as parseable JSON.

### Task 50 — hard-coded developer path in `application_2d.cpp` (DONE, moved to `project-docs/TODO_COMPLETED.md`)

- **New `SeamlyFamilyPaths::locateSeamlyLayoutDevBuild(startDirectory)`** in `src/libs/vmisc/seamly_family_paths.{cpp,h}`. Walks up from the running executable's directory, testing each ancestor as a checkout root for `<root>/src/app/seamlylayout/qt_frontend/build/<config>/SeamlyLayout(.exe)`. Both shadow-build layouts resolve without being named — release `<checkout>/build/…` (5 levels) and `sd.ps1`'s debug `<checkout>/scripts/seamly2d-build-debug/…` (6). **Bounded at 8 parents** so it cannot climb to the filesystem root. **Release preferred over Debug** (the old path pinned Debug unconditionally). Put in `vmisc` beside `locateSeamlyLayout()` so the test suite reaches it, and **parameterized on the start directory** per the Task 34/53 rule, so tests use `QTemporaryDir`
- `application_2d.cpp:515` now calls it; the dev build stays **last** in the lookup chain, after the configured setting and the installed copy, so a source tree can never shadow an installation
- **Coding-rules note** added to `.github/README-CODE-STYLES.md`: "No absolute machine-specific paths in source", with allowed alternatives, an explicit carve-out for placeholder comments and test data, and a `git grep` command

**A CI gate was deliberately not added** — measured first, and a naive grep is unusable: `tst_misc.cpp` has ~20 synthetic `/home/user/...` rows, `tst_dataroot.cpp` uses `C:/Users/tester/...`, and `vcommonsettings.cpp` / `PreferencesModel.cpp` carry `C:/Users/<user>/...` Doxygen placeholders — all legitimate. Telling a real home directory from a placeholder needs a human.

### Flagged, NOT fixed — decide before the upstream PR

**`src/app/seamly2d/core/BUILD_PROBLEMS.txt` is tracked and carries ~45 absolute `/c:/Users/susan/Projects/Seamly2D-private/…` paths** — the same leak Task 50 just closed in code, and it names the *private* repo directory. It is the clangd dump described further down this file (editor noise; the qmake build compiles those files clean). Deleting a tracked file was outside the task's scope. This is the single most likely thing to embarrass the upstream PR. --> user says to delete `src/app/seamly2d/core/BUILD_PROBLEMS.txt`if it isn't useful.

### Verification — read this before trusting the state

| Check                              | Result                                                                                                                                                                     |
| ---------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `scripts/sd.ps1` debug build     | **Clean, exit 0**                                                                                                                                                    |
| `scripts/st.ps1` (Seamly2DTests) | **32127 passed, 0 failed across 25 suites**, exit 0. `TST_SeamlyFamilyPaths` 5 → 13 cases (reported as 15 with init/cleanup); total up exactly +8                 |
| `ParserTest`                     | exit 0                                                                                                                                                                     |
| `TranslationsTest`               | exit 0                                                                                                                                                                     |
| `CollectionTest`                 | **exit 1 — 42 passed, 1 failed.** `TST_Seamly2DCommandLine::TestOpenCollection(07_armhole_adjustment_010)` "Program crashed", `tst_seamly2dcommandline.cpp:302` |

**The CollectionTest failure is pre-existing per the user, but that was NOT confirmed.** Evidence it is unrelated: the only caller of the changed `seamlyLayoutFilePath()` is `mainwindow.cpp:4136` inside `exportPiecesToSeamlyLayout()`, the GUI Layout Mode handoff, which a console `seamly2d --test <pattern>` run never enters. The definitive check — stash the change, rebuild, rerun that one case — was started and **interrupted by the user before the baseline build ran**, so it is unfinished. Note also that the previous session's handover records `ParserTest` and `TranslationsTest` as verified but **never mentions running `CollectionTest`**, so there is no known-good baseline for it either way.

> **RESOLVED in the Task 49 session — do not redo this check.** The baseline run was completed: with all changes stashed and the tree rebuilt, the same case fails identically. Pre-existing and unrelated. See "THE CollectionTest LOOSE END IS CLOSED" in the current-state section at the top.

**A stash hazard was hit and cleared:** the source changes were stashed for that baseline test and the session was interrupted while stashed. They were restored with `git stash pop` (all 9 files back, stash dropped) before committing. If a future session interrupts mid-stash, check `git stash list` first. --> user pushed to origin/run-seamlyLayout, currently nothing to commit.

### Files changed

| File                                                         | Change                                                                                                                                                                    |
| ------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/libs/vmisc/seamly_family_paths.cpp` / `.h`          | New`locateSeamlyLayoutDevBuild()`; file-local `sourceTreeBuildSubPath` and `maxUpwardLevels`                                                                        |
| `src/app/seamly2d/core/application_2d.cpp`                 | Hard-coded path replaced by the call;`seamlyLayoutFilePath()` Doxygen updated                                                                                           |
| `src/test/Seamly2DTest/tst_seamlyfamilypaths.cpp` / `.h` | 8 new cases, all`QTemporaryDir`                                                                                                                                         |
| `.github/README-CODE-STYLES.md`                            | New "No absolute machine-specific paths in source" rule                                                                                                                   |
| `.claude/settings.json`                                    | Line 154 replaced with the version-agnostic vswhere entry                                                                                                                 |
| `project-docs/TODO_MIGRATE.md`                             | Tasks 45 and 50 removed                                                                                                                                                   |
| `project-docs/TODO_COMPLETED.md`                           | Tasks 45 and 50 added at the top with full write-ups                                                                                                                      |
| `.claude/settings.local.json`                              | Two entries deleted — not in that commit because they were never**staged** (the "gitignored" reason recorded here at the time was wrong; see the correction above) |

### Next steps

1. **Finish the CollectionTest baseline check** (above) — the one loose end of this session.
2. **Task 49** — the recommended next task; see the analysis at the top of this section.
3. Decide the fate of `BUILD_PROBLEMS.txt`.
4. The four decisions the user still owes are unchanged — see "Four decisions the user still owes" below --> user added answers to the four decisions.

## Earlier state (2026-07-27): Task 58 merged; documentation reorganized

**Branch:** `run-seamlyLayout`. **Task 58 is DONE** (moved to `project-docs/TODO_COMPLETED.md` — see below) and two documentation reorganizations landed alongside it.

### What happened this session

| Item                                                                                 | Outcome                                                                                                                                                                                              |
| ------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Task 58** — migrate the SeamlyLayout tests to `src/test/SeamlyLayoutTest` | **DONE, merged** via PR [#20](https://github.com/seamly/Seamly2D/pull/20) (`8ddab2a4c3`), all 12 CI checks green. Task written up in `project-docs/TODO_MIGRATE.md` first, then implemented |
| **`status-docs/` → `project-docs/`**                                      | Merged via PR[#21](https://github.com/seamly/Seamly2D/pull/21) (`b89fbe161d`), plus a local merge by the user — see the divergence note below                                                      |
| **Tracking docs moved + SeamlyLayout status docs prefixed**                    | Committed on branch`reorganize-project-docs`, **not yet pushed**                                                                                                                             |

### Task 58 — what moved and what deliberately did not

The four Qt/C++ suites (`AdjustSceneTests`, `AdjustControllerTests`, `PreferencesModelTests`, `SettingsModelTests`) moved from `src/app/seamlylayout/qt_frontend/tests/{adjust,preferences,settings}/` to a flat `src/test/SeamlyLayoutTest/`, matching the sibling `Seamly2DTest`. Git recorded pure renames; no source edits were needed because every project include resolves through `target_include_directories(… src/)`.

**Three decisions that must not be undone:**

1. **The Rust tests stay in `crates/`.** `#[cfg(test)]` modules compile as part of their crate and reach its private items, and Cargo requires integration tests beside the crate's `Cargo.toml`. Moving them would need per-crate `[[test]] path = "../../../../test/…"` entries that break `cargo test -p <crate>`.
2. **`src/test/test.pro` must never list `SeamlyLayoutTest`.** seamlyLayout is CMake + Cargo and stays out of the Seamly2D qmake build. All `SUBDIRS` in `Seamly.pro`, `src/src.pro` and `src/test/test.pro` are explicit literals with no globbing, so the directory cannot be picked up by accident.
3. **`seamlylayout-ci.yml`'s path filters now include `src/test/SeamlyLayoutTest/**`.** Without it a test-only change triggers *no* CI at all — the filters were `src/app/seamlylayout/**` only. `ci.yml` has no path filters, so the parent jobs are unaffected either way.

### Two pre-existing defects fixed inside Task 58

- **`src/app/seamlylayout/build.ps1:117`** probed for a CMake package named `Qt6WebEngine`, which Qt has never shipped (the packages are `Qt6WebEngineCore` / `Qt6WebEngineQuick` / `Qt6WebEngineWidgets`). The guard fired on *every* correctly installed kit, aborting the build while telling the developer to install modules already present. Now probes `Qt6WebEngineQuick`.
- **`ctest --preset debug` could not run on Windows** from a shell that had not sourced the Qt kit — the test exes launch out of the build tree with no windeployqt output beside them. Added a `WIN32`-guarded `ENVIRONMENT_MODIFICATION` prepending `$<TARGET_FILE_DIR:Qt6::Core>` to `PATH` for the test run only, as a generator expression so it follows whichever kit CMake found.

Verified locally: `ctest --preset debug` with no Qt on `PATH` → 4/4 suites, **107 cases** (26 + 7 + 48 + 26), 1 skipped; `PreferencesModelTests`' 48 matches its pre-move count. `cargo test --workspace` → **251 tests across 22 targets**, 0 failures.

### Documentation reorganization

`status-docs/` → `project-docs/`, with `new-attributes.csv` → `NEW-ATTRIBUTES.csv` and `svg-data-attributes.md` → `SVG-DATA-ATTRIBUTES.md`. The empty `status-docs/baseline/` shell went with it; its SVG survives byte-identically (line endings aside) at `src/app/seamlylayout/input/richmond-shirt-baseline_pieces.svg`.

Then, on the **unpushed `reorganize-project-docs` branch**: `PROJECT_PLAN.md` and all six `TODO_*.md` files moved into `project-docs/`, and SeamlyLayout's status docs gained an app-name prefix (`SEAMLYLAYOUT_COMPLETED.md`, `SEAMLYLAYOUT_DECISIONS.md`, `SEAMLYLAYOUT_TODO_FUTURE.md`, `SEAMLYLAYOUT_MIGRATION_STATUS.md`, `TODO_SEAMLYLAYOUT_2.md`).

**`SESSION_HANDOVER.md` stays at the repository root** — a deliberate user decision, reversing an initial move. Both `.claude/settings.json` compaction hooks name it there, and that wording is correct as written. Do not move it.

**`SEAMLYLAYOUT_TODO_FUTURE.md` was `FUTURE_TODOs.txt` and untracked** — `src/app/seamlylayout/.gitignore` ignores `*.txt`. The `.md` extension brought it into version control for the first time, on the user's explicit instruction.

**`src/app/seamlylayout/docs/status-docs/` keeps its directory name** — only the repo-root `status-docs/` was renamed. Several lines name both in one sentence, so reference rewrites were anchored on a leading backtick, which the daughter-app mirrors never carry.

## Earlier state: Tasks 34 and 53 are DONE and moved to `project-docs/TODO_COMPLETED.md`

### Two rules established here — do not undo them

1. **Any function that deletes is called with real home paths only from `Application2D::openSettings()` / `ApplicationME::openSettings()`** — never from a shared init function the test harness also calls. `TestApplication2D`'s constructor runs *before any* `initTestCase()`, so anything it reaches executes against the developer's real settings and real home directory no matter how carefully individual tests redirect. A test run must not mutate the machine it runs on.
2. **`pruneEmptyLegacyDataRoot()` is parameterized on purpose** so tests can point it at a `QTemporaryDir`. `QDir::homePath()` cannot be redirected on Windows. Same reason Task 34 split `chooseFirstRunDataRoot(defaultRoot, legacyRoot)` out of `initializeDataRoot()`.
3. **`.github/README-CODE-STYLES.md` is the naming authority — do not reinstate the `s` prefix.** New files are snake_case with a meaningful prefix from that guide's list (`settings_*`, `dialog_<toolgroup>_<toolname>`, `tool_*`, `model_*`, `options_*`, `test_*`, `application_<appname>`, `<platform>_*`) and **unique repo-wide**, with two exceptions the user added in `df5d90bb14`: a file that primarily defines one class takes the class's **UpperCamelCase** name instead of snake_case, and seamlyLayout's multiple `lib.rs` crate roots are allowed. The old "begin new files with `s`" line in `CLAUDE.md` was removed deliberately on 2026-07-26; the "must not begin with `v`" half was kept.

### Why `seamlyData` and not `seamly`

The user first asked for `dataRoot=G:/My Drive/seamly`. That folder already existed and held **73 GB of unrelated business data** (Finances, Team, Security). A bare `seamly` collides far too easily with a folder a user already has; `seamlyData` says what it is. The rename was applied to the default, the tests, the docs and every open task that mentioned it.

### Developer-machine state (not in git — re-derive from here, do not assume the old paths)

| Item                                                        | Now                                                                                                                                                                                                                                                                                  |
| ----------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Data tree                                                   | **`G:\My Drive\seamlyData`** (was `G:\My Drive\seamly2d`). Moved by **folder rename**, not copy — one Google Drive metadata operation, no 17.6 GB re-upload, reversible. Verified identical before/after: **8,713 files / 1,050 dirs / 17,629,852,473 bytes** |
| `%APPDATA%\Seamly\qt6_common.ini`                         | `dataRoot=G:/My Drive/seamlyData`, all seven path overrides repointed                                                                                                                                                                                                              |
| Also repointed                                              | `%APPDATA%\Seamly\common.ini`; `%APPDATA%\Unknown Organization.ini`; `%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini` (10 paths). Zero stale `seamly2d` paths remain                                                                                                        |
| `C:\Users\susan\seamly2d`                                 | **Deleted** after confirming 0 files / 8 empty dirs                                                                                                                                                                                                                            |
| `%APPDATA%\Unknown Organization\` (the **folder**)  | **Gone** — merged into `%APPDATA%\Seamly\qt6_common.ini`, all four values confirmed present first                                                                                                                                                                           |
| `%APPDATA%\Unknown Organization.ini` (the **file**) | **Still live**, still holds `paths/pattern` and `paths/layout`. That is Task 52, untouched                                                                                                                                                                                 |
| Backups                                                     | Every settings file touched was backed up to the session scratchpad`…\scratchpad\settings-backup\` — session-scoped, treat as gone                                                                                                                                               |

### What is verified

- **Local build** — `scripts\sd.ps1` clean.
- **Local tests** — `scripts\st.ps1`: **32119 passed, 0 failed across 25 suites**, exit 0, with `TST_DataRoot … 22 passed, 0 failed`. `ParserTest` exit 0, `TranslationsTest` exit 0.
- **The suite no longer mutates the machine** — `C:\Users\susan\seamly2d`, `C:\Users\susan\seamlyData` and `%APPDATA%\Unknown Organization` were byte-identical before and after a full run.
- **CI** — all 11 checks on PR #19.

### Crossed `labels` / `images` — found and fixed

`%LOCALAPPDATA%\Seamly\Seamly2D\qt6_seamly2d.ini` had **`labels` and `images` crossed**: `labels=…/seamlyData/images`, `images=…/seamlyData/label templates`. This was *pre-existing stored data*, not a code bug — `preferencespathpage.cpp` maps rows 0–9 consistently between `Apply()` and `initializeTable()`, and `vcommonsettings.cpp` confirms `paths/labels` is the label-template path (`settingPathsLabelTemplate`) while `paths/images` is `settingImagesPath`. **Un-crossed at the user's instruction** and verified against the filesystem: `label templates` (34 files) and `images` (3 files) both resolve. Backup at `qt6_seamly2d.ini.bak-uncross`. Nothing in the repo changed — this was stored user state only.

### `/compact` hooks fixed, and the handover rule is now in `CLAUDE.md`

The `PreCompact` / `PostCompact` hooks in `.claude/settings.json` were emitting `hookSpecificOutput.additionalContext`, which the hook schema accepts **only** for `UserPromptSubmit`, `PostToolUse`, `PostToolBatch` and `Stop`. Both failed validation on every compaction, so the SESSION_HANDOVER.md instruction never reached the model. Both now emit top-level **`systemMessage`** — the only text-carrying key the compact events accept.

**Know the limitation:** `systemMessage` is *displayed*, not injected as instruction context. The hooks are a visible nudge, not a guarantee, and `PreCompact` can no longer shape the summary at all. That is why the requirement was also added to `CLAUDE.md` under Task Tracking, which *is* loaded every session — that line is the actual mechanism; the hooks are the reminder.

## Decisions the user has ANSWERED (act on these, do not re-ask)

1. **Task 54's file-name form** → **`SettingsCommon.h`**, i.e. the file name matches the class name (the style guide's class-match exception wins over the `settings_*` snake_case prefix for class-defining files).
2. **`.github/README-DEVELOPER-NEW.md`** → **rename it to `.github/README-DEVELOPER-SEAMLY-FAMILY.md`**, to be folded into `.github/README-DEVELOPER.md` when the migration is complete. **The rename has not been done yet.**
3. **Qt WebChannel / Qt Positioning documentation** → maintain it in `.github/README-DEVELOPER-SEAMLY-FAMILY.md` until the migration is complete.
4. **`src/app/seamly2d/core/BUILD_PROBLEMS.txt`** → delete it if it is not useful. **Not done yet.**

## Gotchas

### Learned in the Task 49 session (2026-07-27)

- **`CollectionTest.exe` must be run with its working directory set to its own `bin/`.** `initTestCase()` removes `tst_seamly2d_tmp` *relative to the CWD* but re-creates it under `applicationDirPath()`, so from any other CWD it aborts at `initTestCase` on the leftover directory from the previous run ("Fail to prepare test files for testing"). Use `Start-Process … -WorkingDirectory <that bin>`.
- **A SeamlyLayout build-tree exe needs Qt on `PATH` to launch.** There is no windeployqt output beside `qt_frontend/build/Debug/SeamlyLayout.exe`, so launching it from a plain shell produces a process that starts and does nothing (no log file is even created). Prepend `C:\Qt\6.11.1\msvc2022_64\bin` first. `ctest` handles this itself via the `ENVIRONMENT_MODIFICATION` added in Task 58.
- **SeamlyLayout's log file has two independent writers and they overwrite each other.** C++ `Logger` holds a buffered `QTextStream` on the file while Rust's `log_to_file()` opens/appends per call, so lines get clipped mid-string (`-shirt.pieces.svg` on its own line, a missing leading `[`). Do not conclude a log line is absent because it looks truncated — grep for a distinctive fragment instead.
- **A tagged handoff SVG can be produced headlessly**, without driving the Layout Mode GUI: `seamly2d.exe <pattern>.sm2d -b <name> -d <dir> -f 0 --exportOnlyDetails` writes `<name>_pieces.svg` through the same `exportSVG()` that `generatePiecesSvg()` uses. This is how Task 49's end-to-end check was run, and how Task 59's should be.
- **`QCommandLineParser::parse()` ≠ `process()`.** `process()` prints to a console this GUI-subsystem app does not have on Windows, and calls `exit()`. `parse()` returns a bool and fills `errorText()`, which is what makes `StartupOptions` unit-testable.

### Standing

- **A `develop` merge can silently drop doc edits made on this branch.** `.github/README-DEVELOPER.md` was edited on `run-seamlyLayout` and, in the same window, four times on `develop`; the merge resolution took develop's side and the branch edit vanished with no conflict marker left behind. After merging `develop`, `git log -S "<a phrase you added>" -- <file>` is the cheapest way to confirm your change survived. The same merge also deposited a stray `README-DEVELOPER-NEW.md` that exists in no parent commit.
- **`QSettings(fileName, format, parent)` records neither an organization nor an application name** — both come back empty, and QSettings substitutes the literal `"Unknown Organization"`. Root cause of the stray files in Tasks 34 and 52. `QSettings::setPath(format, scope, dir)` *does* redirect settings files, but has **no getter** — recover the base from a probe instance.
- **`QDir::fromNativeSeparators()` rewrites backslashes only on Windows** (a backslash is a legal POSIX filename character), and Windows path comparison must be `Qt::CaseInsensitive` — both matter when comparing a configured root against a legacy one.
- **`QDir::rmdir()` over `removeRecursively()`, deliberately.** `rmdir()` cannot delete a file and refuses a non-empty directory, so it cannot run away. `removeRecursively()` also bypasses the Recycle Bin — that is how `C:\Users\susan\seamly2d` was permanently destroyed in the previous session.
- **`scripts\st.ps1` runs only `Seamly2DTests.exe`.** CI's `make check` runs four binaries — `Seamly2DTest`, `CollectionTest`, `ParserTest`, `TranslationsTest`. Run the other three by hand before pushing.
- **`gh` is not on this agent shell's `PATH`** — invoke it as `& "C:\Program Files\GitHub CLI\gh.exe"`.
- **The sandbox blocks a command that contains both a `Remove-Item` and a `G:` path**, even when they are unrelated. Split into separate calls.
- **clangd diagnostics in this repo are noise, and here is why** — the tree has **no** `compile_commands.json`, `.clangd`, `.vscode/c_cpp_properties.json` or `compile_flags.txt`, so the editor parses each file with zero include paths. The `#include "../vmisc/vabstractapplication.h"` form is valid only because every `.pro` adds `INCLUDEPATH += $$PWD/../../libs/<lib>`, which clangd never sees; one unresolved include then cascades into dozens of `Unknown type name 'QString'` / `undeclared identifier 'QStringLiteral'` entries. `src/app/seamly2d/core/BUILD_PROBLEMS.txt` is a tracked 45-entry dump of exactly this (two `pp_file_not_found` roots + 43 cascade). **The qmake build is the authority** — those same files compile clean. Filing this as a task was considered and declined; if it is ever fixed, the dump should go with it (it carries absolute `/c:/Users/susan/…` paths into source headed for the upstream PR).

### Carried forward (still true)

- **Qt Design Studio poisons `PATH`.** Bare `qmake`, `windeployqt` and `windeployqt6` resolve to a Qt **6.8.7** kit with no `mkspecs`. Never call these bare; use `qtPrepareTool` or `$$[QT_INSTALL_BINS]/…`. Root cause of Tasks 47 and 48. --> The user removed Qt Design Studio
- **PowerShell 5.1 wraps a native exe's stderr in `NativeCommandError`** and sets `$?` to `$false` even on exit 0. Do not redirect native stderr inside PowerShell — run the script as a child process with `Start-Process … -RedirectStandardOutput/-RedirectStandardError -Wait -PassThru -NoNewWindow`.
- **PowerShell splatting: `@array` is positional, `@hashtable` is by name.**
- **Qt frontend test exes are GUI-subsystem binaries** — they print nothing to captured stdout. Run with `-o <file>,txt` and `QT_QPA_PLATFORM=offscreen`.
- **`$proFile` collides with the automatic `$PROFILE`** (case-insensitive); `sd.ps1` still has it.
- **Historical 6.10 references in `project-docs/TODO_COMPLETED.md` and `project-docs/PROJECT_PLAN.md` are deliberate** — they record what was true at the time.
