# Session handover

## Current status

2026-10-11. Branch `task-producer-attributes`, from `run-seamlyLayout` (`035a7a7474`).

- Done in code: Layout.66, Layout.38.2.1, Layout.61.1–61.6.
- `cargo test --workspace`: pass. `ctest --preset debug`: pass (6/6).
- `local_build_msi.ps1`: pass. Version 26.10.1115; `nmake check` pass; MSI `packaging/windows/seamly-msi/x64/seamly-x64.msi`. ICE61 warning only (known).
- Committed, merged `--no-ff` into `run-seamlyLayout` with skip-ci, pushed. The WIP commit was rewritten without task names.
- Layout.66 moved to `TODO_COMPLETED.md`. Layout.61 and Layout.38.2 stay open in `TODO_SEAMLYLAYOUT.md`: user checks remain.

### Changed this session

- Layout.66 consumer: `converter::quantity_from_piece` (`data-quantity` first, else "Cut N"; on fold or odd → `N,0`). Missing-data text now says "no piece quantity or "Cut N" label line".
- Layout.38.2.1: `cxxqt_bridge/src/handoff_capture.rs`; `import_svg_document` writes the received SVG to `SEAMLYLAYOUT_SAVE_HANDOFF`. Relative path or failed write → `import_warning`; import continues.
- Layout.61: `astm_contour::grade_contours`, `seamly_svg2ezdxf/src/astm_grading.rs` (`grade_drawing`), `ezdxf2dxfastm/src/grade_rule_file.rs` (`.rul`), writer graded blocks + layer 5 + `# n`, bridge `DxfGradingSource`; `do_export_dxf` now returns warnings → `export_warning`.
- Grading trigger: `export_dxf` with `layout_views.active_size() == None` and a multisize input with ≥ 2 sizes and a base size.
- Ungradable piece (turn points, lines or notch counts differ; no rigid fit; mirrored) → sample size block only + warning. Sample size not in the layout → piece ungraded, all sizes, warning.
- Fixtures: `ezdxf2dxfastm/test_data/multisize_grading_input.svg`, `multisize_grading_layout.svg` (generator script was in the scratchpad, not kept).
- Docs: compliance doc "Grading" section; `SVG-DATA-ATTRIBUTES.md` "Capture a handoff", `data-parent` now read; QA checklist `project-docs/docs-seamlylayout/TEST_DXF_LAYOUT_QA.md`.

### Risk

- Real patterns may differ in turn points between sizes (tangent-break heuristic near 1°). Those pieces fall back to the sample size. Only synthetic fixtures tested; HJA capture will show the real rate.

## TODO by priority and owner

Priority sets the handover order. Developer and user identify the work type, not a named assignee.

### Priority 1 — User: resolve quantity and report questions

- Confirm the on-fold quantity `N,0`.
- Decide DXF.22: FrontPanel layer 1 reports “should be curve”; layer 14 agrees with the same geometry. Report to ApparelWerks or accept.

### Priority 2 — Developer: after user captures

- Layout.38.2.2: add the HJA handoff fixture to `crates/cxxqt_bridge/test_data/`, then an end-to-end layout + graded export test.
- Fix any piece the HJA graded export leaves ungraded.
- Plan **Layout.15** separately.

### Priority 2 — User: verify the built app

- Follow `project-docs/docs-seamlylayout/TEST_DXF_LAYOUT_QA.md` (sections 0–6).
- Check multisize Nested/Marker layouts, size tabs, Adjust Mode, and one export file per tab.
- Check export names, Annotated text, missing-data warnings, and cancellation in the running app.
- Check PS/EPS with Ghostscript, Illustrator, or Inkscape; check HPGL in a viewer or on a plotter.
- Check JPG export/viewing, new-piece labels and grainlines, roll-frame adjustment, and the combined C++/Rust debug log.

### Priority 3 — Developer: reconcile older unresolved items

These items have no recorded resolution. Check their TODO documents and repository state before resuming them.

- Check merge/build state for `task-piece-auto-name`, `task-grainline-name-ids`, `task-remove-export-layout-gui`, and `task-no-grainline-rotation`.
- Review outstanding **Installer.7** license notices. The pdftops item is already closed.
- Review **InstLinuxAppimage.2**: first-run data directory chooser.
- File a task for the SeamlyLayout return path: Save returns the layout SVG; other closes restore the previous canvas data.
- Correct `TEST_MSI_WIN_X64_Test_Case_1b-i.md`: split post-install and post-first-run checks in B.0a.
- Remove duplicate path leaves; replace `%DATAROOTROOT%` with `%DATAROOT%`.
- In B.0b-iii, replace the empty-file reference `qt6_seamly2d.ini` with `qt6_seamlyme.ini`.

### Priority 3 — User: finish installer checks

- Finish **Installer.6.8** on the test laptop: repair and uninstall remain open.
- Check fresh-install Change → Browse → OK and the first wizard page.
- Review section A test cases 2–4, which the source says have no walkthrough.

## Blocked items

- **Layout.16, Layout.51, Layout.63, Layout.64:** remain blocked. The latest entry gives no release condition.
- Layout.63 drill holes and Layout.64 stripe/plaid lines have no Seamly2D source.
- **Layout.38.2.2 fixture/test:** waits for the user-supplied HJA capture.
- **Manual checks:** require the user's installed app, CLO3D, aw.fyi reports, or test laptop.

## Decisions to retain

- Keep `data-notch-type`. Only DXF export converts every notch to a layer 4 slit.
- Slit ends are never turn points. Slit depth uses half the local seam allowance width.
- Use `test-seamly-layout-input/male_shirt_202610101837/male_shirt_R13_202610101837.dxf` for DXF.20.1. Leave DXF.20.3 unchanged.
- Layout.61: graded nest + `# n` identifiers + `.rul` table. Grading only from the All sizes tab. The old grainline-reference suggestion is superseded.
- CLO3D export uses R13 (`AC1012`). Piece text uses `Size Name:` and omits `Category:`.
- Build only with `packaging/windows/local_build_msi.ps1`. Do not use `build.ps1` or `qd.ps1`.
- Default data root is `~/seamly2d`. Fresh installs allow a change; updates and repairs keep the recorded root read-only.
- Keep CLI export until SeamlyLayout CLI parity. Keep `exportLayoutAs()` through File → Export As.
- On upstream merges, follow `CLAUDE.md`. Remove restored retired PS/EPS functions; no early upstream pdftops PR is approved.

## Verification

1. Run `cargo test --workspace` from the SeamlyLayout Rust workspace.
2. Use VS 18 `vcvars64`, then configure PATH and QMAKE.
3. From `qt_frontend/`, run `cmake --build --preset debug`, then `ctest --preset debug`.
4. Run `packaging/windows/local_build_msi.ps1` with tests and validation enabled before each merge.
5. Run the manual QA checklist against the resulting MSI.

Layout.66 C++ tests: `NotchGroupCarriesNotchTypes`, `PieceCarriesQuantityAndFold`, `NotchTypesFollowNodes`, and `LayoutPieceKeepsQuantityAndFold`.

- Put MSVC `link.exe` before Git's GNU `link` on PATH.
- Use a `.cmd` file; prepend `%VCToolsInstallDir%bin\Hostx64\x64` after `vcvars64` runs.
- For qmake suite logs, set `SEAMLY_TEST_LOG_DIR` and use each suite's `target_wrapper.bat`.
- For a CMake Qt suite, use `-o <file>,txt`. Console silence does not prove a pass.
- If a build timeout recurs, capture a process dump while the process hangs.
- For installer checks, capture `local_install_msi.ps1 -Phase Baseline` before installation.
- Run the MSI wizard from an elevated shell. Do not use `/quiet` for manual checks.

## Protect uncommitted files

- Inspect `git status` and `git stash list` before changing branches or committing.
- Do not commit or discard user edits without authorization.
- Protect both macOS `Info.plist` files and `src/libs/vmisc/projectversion.*`; builds rewrite them (now stamped 26.10.1115).
- `SESSION_HANDOVER.simplified.md` supersedes `SESSION_HANDOVER.md` (user decision). `SESSION_HANDOVER.md` holds uncommitted user edits; do not commit it unasked.
- Protect user edits in `CLAUDE.md`, `TODO_DXF.md`, `scripts/prompt_testing.txt`, and `test-seamly-layout-input/*`.
- Keep the copied aw.fyi reports in `male_shirt_202610101837/` untracked. Do not commit them.
- Inspect older stashes by content. Their recorded `stash@{n}` positions may have changed.
- Older stash notes name `task-grainline-name-ids`, `task-seamlylayout-tight-packing`, and a layout workflow document edit.

## Authoritative documents

Links are relative to the project repository. The supplied attachment does not include these documents.

### SeamlyLayout

- [SeamlyLayout TODO](project-docs/docs-seamlylayout/TODO_SEAMLYLAYOUT.md)
- [DXF TODO](project-docs/docs-seamlylayout/TODO_DXF.md)
- [DXF/layout QA checklist](project-docs/docs-seamlylayout/TEST_DXF_LAYOUT_QA.md)
- [DXF compliance](project-docs/docs-seamlylayout/dxf-docs/seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md)
- [SVG attribute contract](project-docs/docs-data/SVG-DATA-ATTRIBUTES.md)
- [Retired layout code TODO](project-docs/docs-seamlylayout/TODO_REMOVE_DEAD_LAYOUT_CODE.md)

### SeamlyMe

-[SeamlyMe TODO](project-docs\docs-seamlyme\TODO_SEAMLYME.md)

### Installer

- [Installer TODO](project-docs/docs-installer\TODO_INSTALLER.md)
- [MSI test case TODO](project-docs/TODO_MSI_WIN_X64_Test_Case_1b-i.md)
- [MSI test case](project-docs/TEST_MSI_WIN_X64_Test_Case_1b-i.md)
- [Windows packaging instructions](packaging/windows/README.md)
- [MacOS installer](project-docs\docs-installer\TODO_INSTALLER_MACOSX.md)
- [Linux installer](project-docs\docs-installer\TODO_INSTALLER_LINUX_APPIMAGE.md)

Consult `README_MSI_WORKFLOW.md` for shipped packaging decisions; the source does not specify its directory.

### CLI

- [CLI TODO](project-docs\docs-cli\TODO_CLI.md)

### CICD

- [Code Signing](project-docs\docs-cicd\TODO_CODE_SIGNING.md)

### Completed Tasks

- [Completed work archive](project-docs/TODO_COMPLETED.md)
