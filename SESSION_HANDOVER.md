# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`packaging/windows/README.md` and `README_MSI_WORKFLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## 2026-09-24 — Task Layout.14: third-party license notices in every installer

Branch `task-license-notices`, merged without skip-ci (packaging, CMake, .pro changed).

- `src/app/seamlylayout/packaging/licenses/`: `generate_license_notices.py`, `about.toml`, `rust_crate_notices.hbs`; committed `rust_crate_notices.txt` (174 crates) and `qt_notices.txt` (Qt 6.11.1, LGPL-3.0, from Qt SBOMs).
- Shipped: MSI `licenses\`, AppImage `usr/share/licenses/seamly/`, DMG app bundles `Contents/Resources/licenses/`.
- `license_notices_tests` (cxxqt_bridge) fails when the files are stale.
- Machine state: `cargo-about` 0.9.2 installed in `~/.cargo/bin`.
- Open: notices for xerces-c, pdftops (Poppler), MSVC runtime. Linux/macOS packages not verified locally (CI only).

## 2026-09-24 — Export > SVG in-app check; menu fixes

Merged `task-svg-menu-check` into `run-seamlyLayout`.

- Checked in the running app: three modes, "As supplied" gating, tooltips, check mark, missing-font warning, View menu.
- Fixed `ExportMenu.qml`: hidden Projector items left a blank row in the Export menu; tooltips now wrap; "As supplied" label shortened (was cut off).
- Rust crate licenses for `svg_label_text` checked: all permissive (MIT, Apache-2.0, BSD-3-Clause, Unlicense). See Decision-003.
- Open: no installer ships Rust crate license notices (BSD-3-Clause and Apache-2.0 require them). Pre-existing gap across all crates.

## 2026-09-24 — SVG designer-font export: true font subsets

Merged `task-font-subset-allsorts` without skip-ci (`Cargo.toml` changed).

- `font_embedding.rs` subsets with `allsorts` (only used glyphs, Unicode `cmap`) instead of `subsetter`.
- Trousers with Yu Gothic UI Light: export 615 KB → 65 KB. Both font modes checked in Edge.
- Test `embedded_font_is_a_true_subset_with_a_unicode_cmap` pins glyph count, `cmap` and size.

## 2026-09-24 — SeamlyLayout Task Layout.2: three SVG label text modes

Branch `task-svg-text-modes`, merged into `run-seamlyLayout` without skip-ci (`Cargo.toml` changed).
Plan: `project-docs/docs-seamlylayout/export-docs/PLAN_LAYOUT_2_SVG_TEXT_MODES.md`. Font decision: Decision-003.

- New crate `crates/svg_label_text`: designer font (subset `@font-face`), single-line font (Relief SingleLine), Hershey strokes.
- Bridge: `exportSvg(path, mode)`, `labelTextState` property, `exportWarning` signal.
- QML: Export > SVG submenu with tooltips and last-mode check mark; View menu keeps one SVG item.
- `PreferencesModel.svgTextMode`, saved alone by `saveSvgTextMode` (INI key `svg_text_mode`).
- License notices: `src/app/seamlylayout/packaging/licenses/` (Windows MSI only; `*.txt` is gitignored, files force-added).
- In-app check done 2026-09-24 (UI Automation, trousers SVG); task moved to `TODO_COMPLETED.md`. Linux/macOS packages do not ship the notices yet.

## 2026-09-23 — MSI finds running Seamly apps at wizard start

Merged `task-msi-running-apps` into `run-seamlyLayout` without skip-ci (packaging change). See `TODO_COMPLETED.md`.

- Verified: `smsi.ps1` build, ICE, authoring check, `installer_running_apps_test.ps1` find checks.
- Close check skipped locally: a real `seamly2d.exe` ran. Close logic verified separately with a stand-in window.
- Verified on screen 2026-09-23 (user): `SeamlyAppsRunningDlg` appears at once; Retry, Close Apps, Cancel work.
- DLL build files stay in `seamly-msi\<arch>\custom-actions\` (user decision; a temporary-folder variant was reverted).

## 2026-09-23 — Version scheme YY.M.DDHH + MSI newer-version page

Branch `task-msi-version`, merged into `run-seamlyLayout` without skip-ci (full CI runs).

- Cause: a hand-typed `smsi.ps1 -Version 26.9.23.1200` (= 20:00) made the installed MSI outrank later builds; Windows showed "newer version installed".
- Version is now 3-part `YY.M.DDHH` (DDHH = day*100 + hour): `version.sh`, `ci.yml`, the 3 local build scripts, `projectversion.{h,cpp}` (`SUPER_MINOR__VERSION` and unused `APP_VERSION` removed).
- `smsi.ps1` passes `-Version` unchanged as MSI `ProductVersion`; the `YY.M.((D-1)*1440+MMMM)` mapping is gone.
- `smsi.wxs`: `MajorUpgrade AllowDowngrades`, detect-only `SEAMLYNEWERINSTALLED` row, `SeamlyBlockDowngrade` stops unconfirmed downgrades (silent: pass `SEAMLYDOWNGRADECONFIRMED=1`).
- `smsi_ui.wxs`: `SeamlyNewerVersionDlg` — radio "Uninstall Seamly, then continue…" / "Cancel this installation", OK/Cancel. User saw it on screen with 4-part test MSIs; not yet re-seen with 3-part.
- `fvupdater.cpp` `releaseIsNewer` now uses `QVersionNumber` (old loop indexed past a shorter version and ignored field order).
- Open: Trousers `TestOpenCollection` timed out once under load (passes alone, 20–44 s). If it recurs, raise the timeout at `tst_seamly2dcommandline.cpp:306` to 300 s.

## 2026-09-23 — SeamlyLayout Adjust: Lower to Bottom fix

Merged `task-adjust-z-order` into `run-seamlyLayout`. See `TODO_COMPLETED.md`.
`ctest --preset debug` 6/6, `cargo test --workspace` green. CI skipped.

## 2026-09-21 — Seamly2D: unnamed pieces now get a unique "Piece N" fallback name

Branch `task-piece-auto-name`, built and `nmake check`-verified (MSI OK),
ready to merge into `run-seamlyLayout`.

User report: a piece with no explicit `SetName()` call isn't blank — it
defaults to the literal, non-unique `"Piece"` (`VAbstractPieceData` default,
`src/libs/vlayout/vabstractpiece_p.h:70`). Two such pieces are
indistinguishable in the pieces list, layout export, and labels.

Traced every way a `VPiece` enters a document: `VContainer::AddPiece()`
(`src/libs/vpatterndb/vcontainer.cpp`) is the only chokepoint hit for a
genuinely new piece — file load and undo/redo replay always go through
`Source::FromFile` / `UpdatePiece()`, never `AddPiece()`. Fix: `AddPiece()`
now detects an empty or still-default name and assigns `"Piece N"`, where N
is one more than the highest `"Piece <n>"` number currently in the
container (scanned via `NextPieceName()`, not a persisted counter — a full
re-parse rebuilds `VContainer` from scratch, so a counter field would reset
mid-session). Numbering is derived from names currently present, so a
number freed by deleting a piece is available again; it does not collide
with a number still in use.

Four new `TST_VPiece` cases in `src/test/Seamly2DTest/tst_vpiece.cpp` cover:
first/second auto-named piece, an explicit name staying untouched, avoiding
a number still in use, and reusing a number freed by deletion.

Not mine, found mid-session and stashed rather than touched: uncommitted
work-in-progress on `task-grainline-name-ids` (the branch this session
started on) — `SESSION_HANDOVER.md`, `Info.plist` (macOS x2),
`TODO_COMPLETED.md`, `NEW-ATTRIBUTES.csv`, `SVG-DATA-ATTRIBUTES.md`,
`svg_extract.rs`, `male_shirt.sm2d`, `svg_generator.cpp/.h`,
`projectversion.cpp/.h`, `tst_svgcomponenttags.cpp/.h`, plus an untracked
`src/test/Seamly2DTest/testlogs_check/` directory. Stashed as `stash@{0}`
("WIP task-grainline-name-ids before starting task-piece-auto-name") on top
of the pre-existing, also-unresolved `stash@{1}` from
`task-seamlylayout-tight-packing`. Neither stash was touched or evaluated
further — restore with `git stash pop` on `task-grainline-name-ids` to
resume that work.

Next steps: `git merge --no-ff task-piece-auto-name` into `run-seamlyLayout`,
push with `[skip ci]` (only `src/libs` and `src/test` C++ touched — not on
the run-full-CI trigger list), delete the task branch.

## 2026-09-21 — Seamly2D: tagged component groups renamed to name-based ids (e.g. `grainline_Sleeve`)

Branch `task-grainline-name-ids`, off `run-seamlyLayout`. Full write-up moved
to `project-docs/TODO_COMPLETED.md`. Summary: `SvgGenerator::addComponentGroups()`
(`src/libs/vformat/svg_generator.cpp`) now builds `<type>_<pieceName>` /
`<type>_<n>_<pieceName>` component ids instead of `<pieceId>-<type>-<n>`,
piece names sanitized for XML validity, with a no-name fallback and a
cross-piece collision suffix. Piece/pattern ids (`piece-<n>`, `pattern-1`)
are deliberately unchanged. `project-docs/data-docs/SVG-DATA-ATTRIBUTES.md`
and `NEW-ATTRIBUTES.csv` updated; `TST_SvgComponentTags` extended.

While starting this task, found (and applied) a leftover stash on
`task-seamlylayout-local-build-scripts` from the *previous* task below: the
`piece_extractor.rs`/`svg_extract.rs` `data-type`-based fix was already
merged, but one trivial comment-wording hunk in `svg_extract.rs` was still
sitting uncommitted in a stash. Folded in; stash dropped.

Next steps: verify (`packaging\windows\local_build_msi.ps1`), commit, merge
`--no-ff` into `run-seamlyLayout`, push, delete the task branch.

Not mine, found mid-session and stashed rather than touched: an uncommitted
edit to `project-docs/seamlylayout-docs/layout-docs/seamlylayout_PROCESS LAYOUT WORKFLOW.md`
existed on `task-seamlylayout-local-build-scripts` before I started. It's in
`git stash list` on that branch (`stash@{0}` now, after the drop above),
unresolved — the user knows and referenced its content mid-session, so it
may already be intentional; not evaluated further here.

## 2026-09-21 — SeamlyLayout: pattern piece boxes were oversized on real exports; fixed and merged

Branch `task-seamlylayout-piece-bbox-filter`, merged into `run-seamlyLayout`
(`627d53d921`); task branch deleted. Full write-up in
`project-docs/TODO_COMPLETED.md`. Summary: `is_non_outline_group()` in both
`piece_extractor.rs` and `polygon_pack::svg_extract.rs` matched decoration
groups (notch/tuck/grainline/internal_path/drill/hole) by `id` prefix only,
which never matches real Seamly2D ids (`piece-3-notch-1`) — only hand-built
test fixtures. Both now check `data-type` first. Confirmed on
`richmond-shirt-handoff_pieces.svg`: packed boxes had been up to ~10x the
true cutline size on 7 of 12 real pieces.

## 2026-09-20 — Layout Mode canvas flash fixed; GUI "Export Layout" removed (`TODO_REMOVE_DEAD_LAYOUT_CODE.md` Dead.1, build verifying)

Branch `task-remove-export-layout-gui`, not yet merged into `run-seamlyLayout`.
Two commits so far:

1. `showLayoutMode()` (mainwindow.cpp) no longer switches the view to the old
   built-in layout scene (`tempSceneLayout`) before launching SeamlyLayout.exe
   — that was the flash of the superseded canvas users saw every time Layout
   Mode opened. The view now keeps showing Draft/Piece until the handoff
   succeeds; SeamlyLayout is the only canvas shown afterward.
2. Removed the GUI "Export Layout" feature entirely (Tools -> Layout ->
   Export Layout menu, `layout_ToolBar`'s entry, the `layout_Page` toolbox
   button, and the "Layout" toolbar dropdown's option) — `exportLayoutAs()`
   itself stays, now reachable only via File -> Export As... (kept per user
   decision, to repoint or remove later). Investigation (see
   `project-docs/TODO_REMOVE_DEAD_LAYOUT_CODE.md` Dead.1.1 for the full
   file:line map) found the CLI `-e`/`--export` path (`vcmdexport.cpp`,
   `MainWindow::DoExport()`) and "New Print Layout" share the same dialogs/
   helpers (`ExportLayoutDialog`, `LayoutSettingsDialog`,
   `AbstractLayoutDialog`, `DialogLayoutProgress`, `LayoutSettings()`,
   `ExportData()`, `preparePiecesForLayout()`) and all of `src/libs/vlayout`
   — none of that is removable yet. CLI export is kept deliberately until
   SeamlyLayout's `crates/cli` reaches parity.

**Verification: `packaging\windows\local_build_msi.ps1` (full run, no
switches) was still running in the background when this was written** — not
yet confirmed pass/fail. Do not merge or push until it passes.

Next steps: check the build result; if it passes, merge `--no-ff` into
`run-seamlyLayout` and push with the skip-ci token (only `src/app/seamly2d/**`
changed — not in the "run full CI" trigger list); if it fails, fix and
re-verify before merging.

## 2026-09-18 — fresh-install data location: now shown and editable (needs laptop click-through)

User feedback contradicts `Installer.6.5` (2026-09-15, "no picker"): on a
fresh install they got no notice of the default data location and no way to
change it. Re-added a Change button + browse dialog, but only for the
fresh-install case — an update/repair still shows its recorded root
read-only, never editable (`Installer.6.2` unchanged). See the dated note
under `Installer.6` in `TODO_INSTALLER.md` for the full picture.

Changed, uncommitted on `run-seamlyLayout`:

- `packaging/windows/smsi_ui.wxs` — `SeamlyDataLocationDlg` now has two
  ShowCondition/HideCondition variants on `SEAMLYDATAROOTRECORDED`: the
  existing read-only text, or a `PathEdit` bound to `SEAMLYDATAROOT` plus a
  `ChangeFolder` button that spawns the stock `BrowseDlg`. `BrowseDlg` is
  DialogRef'd back in. Its Change/Previous-install/License routing collapsed
  from three-way splits on `SEAMLYDATAROOTRECORDED` to one, since the page
  now always appears and picks its own variant.
  Also fixed a separate bug found along the way: `WixToolset.UI.wixext`
  6.0.2's stock `BrowseDlg` ships with Cancel wired but OK wired to nothing —
  added `<Publish Dialog="BrowseDlg" Control="OK" Event="EndDialog" .../>` or
  its Change button would never actually confirm a folder.
- `packaging/windows/smsi_check_authoring.ps1` — assertions rewritten for the
  two-variant page and the new BrowseDlg wiring (was asserting the *absence*
  of a picker before).
- `project-docs/TODO_INSTALLER.md` — dated note under `Installer.6`.

Verified: `wix build` (direct, dummy staging — no full app rebuild) compiles
clean; `smsi_check_authoring.ps1` passes end to end; `smsi_fix_dialog_lines.ps1`
finds no non-Line control overflow. **Not verified**: `local_build_msi.ps1`
end to end, or an actual click of Change → Browse → OK on Windows — GUI
dialog behavior only shows up on a real run.

## 2026-09-18 — installer "flash" on the first page: fixed, needs re-test on the laptop

`Installer.6.8`'s manual laptop pass caught the wizard's first page
(`SeamlyPrepareDlg`) flashing by with no visible Cancel/Continue before
`WelcomeDlg` appeared — the same defect commit `c791039af3` (2026-09-12) was
supposed to have fixed.

Inspected the actual built MSI's `Dialog`/`InstallUISequence` tables via the
Windows Installer COM API: the September fix was already correct
(`SeamlyPrepareDlg` Attributes=7, Modal bit set, sequenced before `AppSearch`,
Continue/Cancel both wired). The real defect: its title text was "Welcome to
the [ProductName] Setup Wizard", identical to `WelcomeDlg`'s own title one
click later — two separate working pages read as one broken flash.

Committed `3169a9b045` (pushed to `origin/run-seamlyLayout`, full CI running —
no skip-ci token, this touches `packaging/**`):
- `smsi_ui.wxs` — reworded `SeamlyPrepareDlg`'s title to "Preparing to install
  [ProductName]".
- `smsi_check_authoring.ps1` — new assertions that `SeamlyPrepareDlg` stays
  Modal, stays sequenced before `AppSearch`, stock `PrepareDlg` stays absent,
  and its title never re-duplicates `WelcomeDlg`'s.

Verified locally: packaging-only rebuild (`local_build_msi.ps1 -SkipTests`),
`smsi_check_authoring.ps1` passed including the six new checks. Fresh MSI at
`packaging\windows\seamly-msi\x64\seamly-x64.msi`.

**Progress on `Installer.6.8`'s manual install-matrix pass:** fresh install and
update-over-existing-install (SeamlyLayout already present) both verified
working. Still open: repair and uninstall cases, per the matrix in
`TEST_WIN_MSI_Test_Case_template.md`. No TODO_INSTALLER.md line item exists
for the flash defect itself (it was tracked only via the 2026-09-12 commit and
this handover); it does not need one unless it recurs again.

## 2026-09-15 — Installer.6: revert data directory to fixed `~/seamly2d` (done, merged, pushed)

User feedback: stop moving the Windows data directory to
`Documents\SeamlyData` — keep it at `C:\Users\<user>\seamly2d`, fixed, no
picker, no copy-my-data step. Task `Installer.6` in `project-docs/TODO_INSTALLER.md`
(6.1-6.7 checked off; 6.8, the manual install-matrix pass, is the only item
left open); supersedes `SettingsFiles.7`/`Layout.11` in `TODO_COMPLETED.md`
(left as history there, marked superseded).

`task-fixed-data-dir` merged into `run-seamlyLayout` with `--no-ff`
(`8d47c97fe8`) and pushed to `origin/run-seamlyLayout` — no skip-ci token,
since this touches `packaging/**` functionally, so the full `ci.yml` suite
is running on it; check that run before relying on this as release-verified.
Local task branch deleted. Implemented:

- **C++**: `VCommonSettings::getDefaultDataRoot()` → `~/seamly2d` on every
  platform. Deleted the now-dead legacy-migration subsystem:
  `legacy_data_migration.{h,cpp}`, `legacy_data_archive.{h,cpp}`,
  `migrateAdoptedLegacyTree()`, `pruneEmptyLegacyDataRoot()`,
  `chooseFirstRunDataRoot()`, `getLegacyDataRoot()`, the
  `firstRunNoticePending`/`markFirstRunNoticeShown` notice, and
  `VAbstractApplication::NotifySeamlyDataLocation()` (plus call sites in
  both apps' `main.cpp`/`openSettings()`). `tst_dataroot.{h,cpp}` trimmed to
  match (~25 surviving tests; migration/prune/archive/adoption groups
  removed). `vmisc.pro`/`vmisc.pri`/`Seamly2DTest.pro` no longer need
  `core-private` (was only for the deleted zip-archive code).
  SeamlyLayout's `PreferencesModel.cpp` no-installer fallback matches.
- **MSI**: `smsi.wxs`/`smsi_files.wxs`/`smsi_registry.wxs`/`smsi_ui.wxs`
  rewritten — one `SEAMLYDATAROOT` (plus `SEAMLYDATAROOTRECORDED` as the
  "earlier install exists" signal), no `SEAMLYDATAPARENT`/`SEAMLYDATACHOSEN`/
  `SEAMLYCOPYUSERDATA`, no `DataParent` registry value. `SeamlyDataDirDlg`
  (picker) and `SeamlyDataMigrateDlg` (copy prompt) replaced by one read-only
  `SeamlyDataLocationDlg`, shown only when an earlier install recorded a
  root. `smsi_migrate_user_data.ps1` + `smsi_seed_user_settings.ps1` replaced
  by one `smsi_ensure_user_data.ps1` (standard subdirs + ini seeding,
  add-only, runs on fresh/update/repair alike) with a consolidated
  `smsi_ensure_user_data_test.ps1`.
- **Docs**: `.github/README-BUILDS.md`, `packaging/windows/README_WINDOWS_BUILD.md`,
  `README_WINDOWS_INSTALLER.md`, `project-docs/FILE_PATHS_PLAN.md`,
  `TEST_WIN_MSI_Test_Case_template.md` updated.

**Verified this session** (`packaging\windows\local_build_msi.ps1`, full run):
all four Qt test suites passed via `nmake check` (a failing suite stops the
build before it reaches an MSI, so this is a hard pass/fail gate, not just
"nothing printed"); SeamlyLayout's Rust crates and `seamlylayout.exe` built
clean; `smsi_check_authoring.ps1` — every assertion including the rewritten
data-root/dialog ones — passed; the new `smsi_ensure_user_data_test.ps1`
passed 28/28; final MSI packaged at 165.1 MB. Version-stamp files
(`projectversion.{h,cpp}`, the two `Info.plist`) were left clean afterward,
as expected.

**Still open:**

- Manual install-matrix pass (fresh / update-with-custom-root / repair /
  uninstall) — `Installer.6.8`, still unchecked. Needs the test laptop.
- The `ci.yml` run triggered by this push (full suite, no skip-ci) — watch
  it before treating this as verified beyond the local build.

**Stale note below:** the "Machine state" section further down records
`%DATAROOT%` = `C:\Users\susan\Documents\SeamlyData` from the 2026-09-02
pass — that was the *previous* (now-reverted) design. Do not treat it as
current; a fresh install after this task lands creates
`C:\Users\<user>\seamly2d` instead.

## 2026-09-14 — CI: Linux AppImage now bundles SeamlyLayout (done)

Task `InstLinuxAppimage.1` closed — see `project-docs/TODO_COMPLETED.md` for
the full writeup (three CI iterations: the AppImage change itself, two
pre-existing SeamlyLayout Qt suite test failures unrelated to it, and a
missing `qtserialport` Qt module). Full `ci.yml` `workflow_dispatch` run on
`run-seamlyLayout` passed clean 2026-09-14: `linux-test`, `linux` (AppImage),
`macos`, both `windows-msi` arches, `Publish Pre-releases`.

Remaining work in this area is `InstLinuxAppimage.2` (first-run user-data
directory chooser for the AppImage) — not started.

## Current steps

1. build .msi with `packaging\windows\local_build_msi.ps1`
2. clear environment with `packaging\windows\local_reset_environment.ps1` — needs
   an elevated shell
3. install MSI with `packaging\windows\seamly-msi\x64\seamly-x64.msi` — elevated
   shell, run the wizard, do **not** pass `/quiet`
4. test installation against `project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md`
5. add tasks for additional errors to `project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md`
6. implement a task from `project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md`, then loop to step 1; repeat until that file is empty

**Where this loop stands, 2026-09-02.** One full turn ran on build
**26.9.2.1059** (MSI ProductVersion 26.9.2499): built, machine reset, installed
through the wizard from an elevated shell, and walked end to end against the
test case. Steps 1-4 pass with no failing check. Steps 5 and 6 have nothing to
do — `project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md` holds no open task, and
this pass found no new defect. **The loop is finished unless a new defect turns
up.**

`project-docs/TODO_SETTINGS_FILES.md` is deleted; the old steps that named it
are gone. Build only with `local_build_msi.ps1` — do **not** use
`src\app\seamlylayout\build.ps1` or `qd.ps1` any more. Both `CLAUDE.md` files and
`src/app/seamlylayout/.claude/rules/testing.mdc` were corrected to say so.

## Commit state

`run-seamlyLayout` is **pushed and level with `origin/run-seamlyLayout`**
(`ff7d8c441b`). Nothing is waiting to commit except three files that are not
task work:

- `scripts/prompt_testing.txt` — the user's own edit, made before this session;
- `src/libs/vmisc/projectversion.cpp` / `.h` and the two `packaging/macos` `Info.plist`
  files — the version stamp, which every local build rewrites.

The push carried `ff7d8c441b` (the `--no-ff` merge) and `3efd1017ed` (the
`MSI1b.1` task work), and **no skip-ci token**: `packaging/**` changed
functionally, so the full `ci.yml` suite runs on it. That run is the only
verification Seamly2D and SeamlyMe get — check it.

## Machine state

- Installed: Seamly **26.9.2.1059** in `%ProgramFiles%\SeamlyApps`
  (seamly2d.exe, seamlyme.exe, SeamlyLayout.exe). MSI ProductVersion `26.9.2499`.
- Installed 2026-09-02 through the **wizard** from an elevated shell, onto a
  machine reset by `local_reset_environment.ps1`. Install log:
  `%TEMP%\seamly_install.log`.
- `%DATAROOT%` = `C:\Users\susan\Documents\SeamlyData`. Right after the install
  it is an **empty** directory: the MSI creates it, and the first app run seeds
  it. Same for SeamlyLayout's `default_preferences.json` /
  `default_settings.json`.
- All three apps have been run once, so every first-run artifact exists.
  `%DATAROOT%` now holds 8 subdirectories, 8 patterns, 3 individual and 1
  multisize measurement file.
- `packaging/windows/local_install_msi.ps1` cannot check this pass. It needs
  `-Phase Baseline` captured BEFORE the install, and the install is already
  done. Capture the baseline first next time.

**Elevation.** The VS Code integrated terminal is not elevated, but
`Start-Process -Verb RunAs` DOES work when someone is present to accept the UAC
prompt. It fails at once with "The operation was canceled by the user" when the
prompt goes unanswered — that message means unanswered, not refused by policy.
An unelevated `msiexec /i` fails at `InstallFinalize` with `Error 1925` and exit
1603, after every earlier action returned 1, so the log looks healthy until the
last page.

## Done this session

### Build instructions corrected in the rule files

`CLAUDE.md`, `src/app/seamlylayout/CLAUDE.md` and
`src/app/seamlylayout/.claude/rules/testing.mdc` all told a future session to
build with `build.ps1` / `qd.ps1`. They now name
`packaging\windows\local_build_msi.ps1` and retire the other two. The
project `CLAUDE.md` also documents the four build stages, the `-SkipTests` and
`-SkipValidation` switches, that arm64 is not covered, and how to read a single
Qt suite's output.

Both `build.ps1` and `qd.ps1` are still on disk — marked unsupported, not
deleted. Deleting them was not asked for.

## Verification status

| Suite | How | Result |
| --- | --- | --- |
| Seamly2DTest, CollectionTest, ParserTest, TranslationsTest | `nmake check` inside `local_build_msi.ps1` | pass |
| SeamlyLayout Qt tests | `ctest --preset debug` | 6/6, including the new `LoggerTests` |
| `LoggerTests` alone | per-suite log via `-o <file>,txt` | 7 passed, 0 failed |
| SeamlyLayout Rust | `cargo test --workspace` | pass |
| MSI 26.9.2.996 | `local_build_msi.ps1` | MSI OK, 164.6 MB; authoring check and 17 installer self-tests pass |
| Test Case 1b-i, fresh wizard install of 26.9.2.996 | manual walkthrough, 2026-09-02 | **pass, no failures** |

To read a single Qt suite's output, set `SEAMLY_TEST_LOG_DIR` and run the
binary through its own `target_wrapper.bat` — three of the four qmake suites are
GUI-subsystem binaries that print nothing to a console, and a shared `-o` target
is overwritten by each `qExec()` call. The CMake SeamlyLayout suites are
GUI-subsystem too; run one with `-o <file>,txt` to read its result.

**`cargo test` needs MSVC's `link.exe` first on PATH.** `%ProgramFiles%Git\usr\bin`
holds a GNU `link` that shadows it, and the failure names the Visual Studio
installer, not the shadowing. `vcvars64.bat` alone does not fix it, and a
`vcvars && set PATH=...%PATH%...` one-liner cannot: cmd expands the whole line
before `vcvars` runs. Put the commands in a `.cmd` file and prepend
`%VCToolsInstallDir%bin\Hostx64\x64`.

## Open — next steps

1. **Nothing is open in the MSI test loop.** Test Case 1b-i passed end to end on
   26.9.2.1059 and its TODO file is empty. The next MSI work is either a new
   defect from a later pass, or test cases 2, 3 and 4 of section A, which have
   never been walked.
2. **Test-document defects in `TEST_MSI_WIN_X64_Test_Case_1b-i.md`, agreed but
   not yet fixed.** Each makes correct behaviour read as a failure. This list
   was re-checked against the file on 2026-09-02, and it is shorter than the
   older handover said — B.2b-v and B.2c-iii already name the right paths, and
   B.0a does list `label templates`. What is left:
   - B.0a is ordered wrong. Split it into a post-install part and a
     post-first-run part. Confirmed again on this pass: right after the install
     `%DATAROOT%` is an empty directory and SeamlyLayout's
     `default_preferences.json` / `default_settings.json` are absent; all three
     appear after B.2.
   - Doubled leaf in a placeholder: `%DATAROOT%\SeamlyData` (line 56) and
     `%PROGRAMDIR%\SeamlyApps` (lines 40, 85). Both placeholders already end in
     that leaf.
   - `%DATAROOTROOT%` (lines 89, 90) should be `%DATAROOT%`.
   - B.0b-iii says `qt6_seamly2d.ini` should be empty; it means
     `qt6_seamlyme.ini`, which is the file that is empty.
3. **Watch the `ci.yml` run on `ff7d8c441b`.** It is the first full CI run since
   the last three pushes, and the only verification the Seamly2D and SeamlyMe
   Qt code gets.

## Still to do — the SeamlyLayout return path

Not implemented, and **no task file entry exists for it yet**. It was outside
`Seamly2D.5`/`Layout.9`, whose subtasks covered the outbound handoff only.

- Closing SeamlyLayout with 'Save': convert its layout to a stringified SVG,
  pass it back to Seamly2D, show it in Seamly2D's right canvas, and return focus
  there.
- Closing SeamlyLayout any other way: refresh the right canvas with the previous
  Seamly2D data and return focus there.

Focus already returns to the mode active before the handoff (`Seamly2D.3`). What
is missing is carrying SeamlyLayout's layout back into the right canvas.
