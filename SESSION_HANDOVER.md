# Session handover

## Current status

### 2026-11-01 — Seam line stops at the fold line (Seamly2D.71.13); merged into local `run-seamlyLayout`, not pushed

- Rule (user): neither the seam line nor the cut line has an edge on the fold line. Each starts at the fold line end and ends at the fold line start.
- Seamly2D: `make_line_open_at_fold` (`handoff_piece.cpp`) builds `seamline` and `cutline`. Piece mode draws the shown seam line the same way (`open_at_fold_line_path`). A hidden main path stays closed for the item shape.
- SeamlyLayout: DXF-ASTM `converter` adds the fold line edge to an outer `seamline` (no `cutline`). A `seamline` beside a `cutline` stays as sent. Docs and test names cover both lines.
- Decision kept: `VLayoutPiece` still sends both lines closed until the Dead.4.4.3 switch.
- Feedback (user): do not write "open" or "opens" alone. Say where the line starts and ends.
- Verified: harness `TST_VPiece` 134/134, `TST_HandoffPiece` 23/23, `TST_HandoffDocument` 17/17; `cargo test --workspace` all pass; `ctest --preset debug` 7/7. Not run: MSI.
- Seamly2D.71.13 done. The Seamly2D.71.12 user check now covers the seam line.
- Merge subject has no skip-ci token: the pending push holds `.pri` and `packaging/` changes. Task branch deleted.
- Not committed: `project-docs/AGENT_CHECKLIST.md` (Sue edits it). Added §34 (Seamly2D.71.12) and OPEN rows; reworded §27 Layout steps.
- Next: MSI build (ask): Seamly2D.71.9, 71.12, 11.8, 11.9, Layout.97.7.
- `run-seamlyLayout` is 44 commits ahead of `origin`.

### 2026-11-01 — Open cut line at the fold line (Seamly2D.71.11, Layout.112); merged into local `run-seamlyLayout`, not pushed

- Rule (user): with a fold line the cut line is open; the fold line runs from the cut line end to its begin. Built-in seam allowance too.
- Seamly2D: `VPiece::openAtFoldLine` (public static); `HandoffPiece` open `cutline` and `built_in_cut_line()`; Piece mode draws the cut line open (`pattern_piece_tool.cpp`).
- Decision: the live Layout Mode writer (`VLayoutPiece`) keeps the closed cut line until the Dead.4.4.3 switch. Noted there as an expected compare difference.
- SeamlyLayout: new `geometry::fold_line_join`; DXF-ASTM `converter` and `piece_divider` close an open cut line along the fold line (0.1 mm). Packing, display, HPGL: no change (tests added).
- Verified: scratch harness `TST_VPiece` 134/134, `TST_HandoffPiece` 20/20, `TST_HandoffDocument` 16/16; `cargo test --workspace` all pass; `ctest --preset debug` 7/7. Not run: MSI.
- Layout.112 moved to `TODO_COMPLETED.md`. Seamly2D.71.11 done; new Seamly2D.71.12 (user check, Piece mode).
- Merge subject has the skip-ci token (`.cpp`, `.h`, `.rs`, `.md` only). Task branch deleted.
- Not committed: `project-docs/AGENT_CHECKLIST.md` (Sue edits it). No checklist row added for Seamly2D.71.12.
- Next: MSI build (ask): Seamly2D.71.9, 71.12, 11.8, 11.9, Layout.97.7.
- `run-seamlyLayout` is 39 commits ahead of `origin`. The head commit (docs) has no skip-ci token, so the push starts CI; the pending push has `.pri` and `packaging/` changes.

### 2026-11-01 — male_shirt DXF review: Flip Both, seam line corner, On fold box, Fold needs a fold line; merged into local `run-seamlyLayout`, not pushed

- Checked DXF exports 1931 and 2029 (aw.fyi reports). `AGENT_CHECKLIST.md`: Layout.80.6.6 PASS; §27 FAIL → Seamly2D.11.10 (user).
- User decision: Flip Both writes `Flip:Y` with a `Rotation:` step that reaches 180. Merge `35b6ce1b7b` (`31752f8e20`). cargo 639 pass, ctest 7/7.
- Seamly2D.11.10 done. Merge `572ed0f74b` (`0a8954905a`): `removeConsumedEdges()` in `vpiece.cpp` collapses a reversed offset edge before `Equidistant()`. Harness: new test FAIL before, `TST_VPiece` 129/129 after.
- User decision: a mirrored piece is not on the fold (`Fold:N`). male_shirt "CollarTopInterface Mirrored" has `onFold="true"`; the mirror tool writes `false`.
- Layout.97.6 done. Merge `28741addb9` (`dc26eeeded`): removing the fold line no longer leaves "On fold" checked. `cl` compile only.
- Layout.111 closed (user decision: the warning was correct); moved to `TODO_COMPLETED.md`.
- Harness: `%TEMP%\seam_corner_harness\` (`build.bat [fix]`, `run.bat`; xerces DLL directory on PATH).
- Layout.97.8 done (user decision: no fold line → `Fold:N`, also with `onFold="true"`; FrontPanel case). Merge `2171bf01dc` (`a8e5d348a6`): `converter::piece_is_on_fold`; the `Quantity:` split follows. cargo 639 pass.
- Layout.112.5 done (user wording): DXF warning "the fold line doesn't connect with the cutline". Merge `a25e25cbb9`. cargo 639 pass.
- User rule: with a fold line, the cut line is open; the fold line runs from the cut line end to its begin. Filed Seamly2D.71.11 and Layout.112 (approach proposed, not approved).
- User action (agreed): uncheck "On fold" on "CollarTopInterface Mirrored" in both male_shirt copies.
- Next: MSI build, not yet approved (ask): Seamly2D.11.8, Seamly2D.11.9, Layout.97.7. Then the user checks `AGENT_CHECKLIST.md`, then pushes (user decision; the push will start CI).
- `run-seamlyLayout` is 33 commits ahead of `origin`.

### 2026-11-01 — `develop` merge (14 upstream commits); merged into local `run-seamlyLayout`, not built, not pushed

- Added remote `upstream` (fetch only, push URL `DISABLED`). Local `develop` already equals `upstream/develop` and `origin/develop`.
- Merge commit `06287a0dc7` on `task-merge-develop`, then `--no-ff` into `run-seamlyLayout`; task branch deleted.
- Conflicts: `core.pri` both blocks; `pattern_piece_tool.cpp` ours plus `adaptColor`; `vtextmanager.cpp` stays deleted.
- Upstream #1793 (untranslated combo values): orientation only. New `VPieceLabelData::orientationDisplayName`.
- "console mode (export / test)" comments → "(test)" in 6 files.
- Finding: upstream `85c36c0bcc` `fixGroupsOrder` adds `<images>` to pre-0.7.1 files. lxml: `jacket1_52-176.sm2d` (v0.6.8) then fails `v0.6.8.xsd`. Not changed in the merge.
- User decision: `<images>` in every draft block comes with a new schema version and converter. Filed Task Seamly2D.72 (`TODO_SEAMLY2D.md`, Required for SeamlyLayout release).
- User decision: merge without the MSI build. Merge subject has no skip-ci token (`.pri`, `packaging/`, `Q_OS_MAC`).
- Next: MSI build (ask); Task Seamly2D.72; after the push, `gh workflow run ci.yml --ref run-seamlyLayout`.

### 2026-10-28 — Fabric media size fix; merged into local `run-seamlyLayout`, not pushed

- Cause: Sue's trousers on fabric (folded, 36 in, selvedge 0.5 in) left 4 pieces out. The fabric canvas used the B0 paper sheet size that the settings keep from paper media; fabric width, fold and open length were ignored.
- Fix: `LayoutSettings::media_size_in()` gives the canvas (`page_w_px`, `page_h_px`) and the bin (`effective_bin_px`). Fabric: `fabricWidth` (0 = `pageWidth`; folded: half), `fabricHeight` (0 = 500 in, trimmed). Paper ignores the fabric fields.
- Commit `01562ef927`. Changed: `layout_settings.rs`, `layout_utils.rs`, `material_split.rs`, `size_labels.rs` (fabric fixtures use `fabricWidth`/`fabricHeight`), the layout and settings workflow docs.
- Verified: `cargo test --workspace` 636 passed; `ctest --preset debug` 7 of 7. Not run: MSI, GUI.
- Sue's popup had the old title, so her MSI predates `3b1310115e`.
- CLAUDE.md vcvars path: VS 2022 → VS 18 (user decision: local builds use VS 18; `local_build_msi.ps1` already does).
- Next: MSI build (ask). User test: trousers on 36 in folded fabric place all 12 pieces inside the fold half.
- `run-seamlyLayout` is 8 commits ahead of `origin`.

Older entries: `project-docs/SESSION_HANDOVER_ARCHIVE.md`. Do not Read it at session start.

## TODO by priority and owner

### Next items — Developer

- Layout.25 (media grows past the 500 in roll, user decision), then Layout.27.3 (repeat failures). Then Layout.70, 69, 27.11, 67.9.
- `TEST_CHECKLIST.md` line 104 names a `Front` piece; jacket2 has none. The file has user edits; ask before a change.

### Priority 1 — Developer: approved plan

- New task branch from `run-seamlyLayout`; implement the 2026-10-12 plan table in `project-docs/SESSION_HANDOVER_ARCHIVE.md`.

### Priority 2 — User: after the next MSI

- Capture the jacket2 handoff with `SEAMLYLAYOUT_SAVE_HANDOFF` (fixture for Layout.27.7).
- Capture the HJA handoff (`test-seamly-layout/patterns/HJA.sm2d`) for Layout.38.2.2.
- Run the new `TEST_CHECKLIST.md`. CLO3D checks last.

### Priority 2 — Developer: after the user captures

- Layout.27.7 and Layout.38.2.2 fixtures in `crates/cxxqt_bridge/test_data/`, end-to-end graded export tests.
- Plan **Layout.15** separately.

### Priority 3 — Developer: reconcile older unresolved items

These items have no recorded resolution. Check their TODO documents and repository state before resuming them.

- Check merge/build state for `task-piece-auto-name`, `task-grainline-name-ids`, `task-remove-export-layout-gui`, and `task-no-grainline-rotation`.
- Review outstanding **Installer.7** license notices. The pdftops item is already closed.
- Review **InstLinuxAppimage.2**: first-run data directory chooser.
- File a task for the SeamlyLayout return path: Save returns the layout SVG; other closes restore the previous canvas data.
- Correct `TEST_MSI_WIN_X64_Test_Case_1b-i.md`: split post-install and post-first-run checks in B.0a.
- Remove duplicate path leaves; replace `%DATAROOTROOT%` with `%DATAROOT%`.
- In B.0b-iii, replace the empty-file reference `qt6_seamly2d.ini` with `qt6_seamlyme.ini`.
- `CLAUDE.md` Key References names `project-docs/NEW-ATTRIBUTES.csv`; the file is `project-docs/docs-data/NEW-ATTRIBUTES.csv`.

### Priority 3 — User: finish installer checks

- Finish **Installer.6.8** on the test laptop: repair and uninstall remain open.
- Check fresh-install Change → Browse → OK and the first wizard page.
- Review section A test cases 2–4, which the source says have no walkthrough.

### Last — User: after every other open task

- Report DXF.22 to rick@apparelwerks.com: same FrontPanel corner, L1 "should be curve", L14 "agree"; earlier export the reverse.
  - Add jacket2 Detail (4) size 240, L1 pt 14: a 6.5° bend over 10 mm read as a missing corner (DXF.23).

## Blocked items

- **Layout.51, Layout.63, Layout.64:** remain blocked. The latest entry gives no release condition.
- Layout.63 drill holes and Layout.64 stripe/plaid lines have no Seamly2D source.
- **Layout.27.7, Layout.38.2.2 fixtures:** wait for the user's captures.
- **Manual checks:** require the user's installed app, CLO3D, aw.fyi reports, or test laptop.

## Decisions to retain

- Keep `data-notch-type`. Only DXF export converts every notch to a layer 4 slit.
- Notch attributes: one value per notch (D1, 2026-10-12). Replaces "one value per segment" (2026-10-11).
- Built-in seam line corners are plain offset corners; node corner types shape only the cut line (user, 2026-10-12).
- The user pushes `run-seamlyLayout` to `origin`. Delete a task branch once it is merged (user, 2026-10-12).
- Slit ends are never turn points. Slit depth uses half the local seam allowance width.
- Multisize tabs: Marker, Nested, Base Size N, then Size N. Every tab comes from the input SVG; no tab is built from grade rules. No "Multisize Layout" setting (user, 2026-10-13).
- DXF-ASTM carries one size: Base Size tab = base size + `# n` + `.rul`; size tab = one ungraded size; Marker and Nested have no DXF-ASTM export. No graded nests (user, 2026-10-13; replaces the Layout.61 graded nest).
- CLO3D export uses R13 (`AC1012`). Piece text uses `Size Name:` and omits `Category:`.
- Build only with `packaging/windows/local_build_msi.ps1`.
- Default data root is `~/seamly2d`. Fresh installs allow a change; updates and repairs keep the recorded root read-only.
- Keep CLI export until SeamlyLayout CLI parity. Keep `exportLayoutAs()` through File → Export As.
- On upstream merges, follow `CLAUDE.md`. Remove restored retired PS/EPS functions; no early upstream pdftops PR is approved.

## Verification

1. Run `cargo test --workspace` from the SeamlyLayout Rust workspace.
2. Use VS 18 `vcvars64`, then configure PATH and QMAKE.
3. From `qt_frontend/`, run `cmake --build --preset debug`, then `ctest --preset debug`.
4. Run `packaging/windows/local_build_msi.ps1` with tests and validation enabled before each merge.
5. Run the manual checklist against the resulting MSI.

- Put MSVC `link.exe` before Git's GNU `link` on PATH.
- Use a `.cmd` file; prepend `%VCToolsInstallDir%bin\Hostx64\x64` after `vcvars64` runs.
- For qmake suite logs, set `SEAMLY_TEST_LOG_DIR` and use each suite's `target_wrapper.bat`.
- For a CMake Qt suite, use `-o <file>,txt`. Console silence does not prove a pass.
- Release SeamlyLayout builds write no Rust log lines (`log_to_file` is a no-op).
- For installer checks, capture `local_install_msi.ps1 -Phase Baseline` before installation.
- Run the MSI wizard from an elevated shell. Do not use `/quiet` for manual checks.

## Protect uncommitted files

- Inspect `git status` and `git stash list` before changing branches or committing.
- Do not commit or discard user edits without authorization.
- Protect both macOS `Info.plist` files and `src/libs/vmisc/projectversion.*`; builds rewrite them (stamped 26.10.1115).
- Protect user edits in `CLAUDE.md`, `TODO_DXF.md`, `scripts/prompt_testing.txt`, and `test-seamly-layout/`.
- Never commit `test-seamly-layout/D6673-10-expired.docx`.
- Inspect older stashes by content. `stash@{0}` and `stash@{1}` hold a layout workflow document edit.

## Authoritative documents

### SeamlyLayout

- [SeamlyLayout TODO](project-docs/docs-seamlylayout/TODO_SEAMLYLAYOUT.md)
- [DXF TODO](project-docs/docs-seamlylayout/TODO_DXF.md)
- [Test checklist](project-docs/TEST_CHECKLIST.md)
- [DXF compliance](project-docs/docs-seamlylayout/dxf-docs/seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md)
- [SVG attribute contract](project-docs/docs-data/SVG-DATA-ATTRIBUTES.md)
- [Retired layout code TODO](project-docs/docs-seamlylayout/TODO_REMOVE_DEAD_LAYOUT_CODE.md)

### SeamlyMe

- [SeamlyMe TODO](project-docs/docs-seamlyme/TODO_SEAMLYME.md)

### Installer

- [Installer TODO](project-docs/docs-installer/TODO_INSTALLER.md)
- [MSI test case TODO](project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md)
- [MSI test case](project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md)
- [Windows packaging instructions](packaging/windows/README.md)
- [macOS installer](project-docs/docs-installer/TODO_INSTALLER_MACOSX.md)
- [Linux installer](project-docs/docs-installer/TODO_INSTALLER_LINUX_APPIMAGE.md)

### CLI

- [CLI TODO](project-docs/docs-cli/TODO_CLI.md)

### CICD

- [Code Signing](project-docs/docs-cicd/TODO_CODE_SIGNING.md)

### Completed Tasks

- [Completed work archive](project-docs/TODO_COMPLETED.md)
