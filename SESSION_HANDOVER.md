# Session handover

Only the **current** state lives here. Completed tasks are written up in
`project-docs/TODO_COMPLETED.md`, and the reasoning behind shipped decisions
lives beside the code it governs — for Windows packaging that is
`packaging/windows/README.md` and `README_MSI_WORKFLOW.md`. Do not
re-accumulate finished-session narrative in this file.

## 2026-10-08 — Merge-break fix; aw.fyi R13 report review; DXF.19

### Build break from the `develop` merge

- Merge `9dc09c9e49` restored `MainWindowsNoGUI::exportEPS`, `exportPS`, `convertPdfToPs` bodies (retired in `14e1d8723b`). Build failed C2039.
- Fix committed: `becb0c7c18` on `run-seamlyLayout`. **Not pushed. Not built.**
- `CLAUDE.md`: new section "Merge from Upstream `develop`" (9 steps, retired-code list). Uncommitted; file also holds user edits. User commits it.
- `git config rerere.enabled true` set in this repo.
- User decision: no early upstream PR to remove pdftops. Delete the three functions on each merge.
- Other uncommitted user files: `packaging/macos/*/Info.plist`, `src/libs/vmisc/projectversion.*`, `test-seamly-layout-input/*`. Do not commit unasked.

### aw.fyi report: `male_shirt_R13_202610071908.dxf`

- Files: `test-seamly-layout-input/male_shirt_R12_202610071908/`. Report: `male_shirt_R13_202610071908-dxf-report.pdf`.
- `male_shirt_202610032219_R13.dxf` in the same folder is an old AC1009 file; ignore it.
- Result: 2 departures, 0 extensions, 5 advisories. **DXF.15 and DXF.16 repeat** → fix next, do not re-file.
- Analysis scripts (scratchpad, not kept): ezdxf 1.4.4 dump of block layers; PyMuPDF crops of report evidence images.

| Piece | Finding | Cause |
|---|---|---|
| FrontPanel_M L1 + L14 | Line→curve junction (71.1 / 71.35, y 1068.59 / 1058.59) not a turn point | `straight_run_ends` ignores a one-segment straight line. Old input had a notch vertex on the edge; now it has none |
| FullSleeve_M | 10 L2 points "not needed – definite": both ends of 4 pleat lines (L8) and slit (L11) | `build_contour_tagged` forces open-polyline ends to turn points |
| FullSleeve_M | 2 hem run ends at x≈345.8 (L1, L14) "should be curve" | Curve meets the hem line tangentially; DXF.16 makes every run end a turn point |
| FullSleeve_M | Slit end (345.67) 0.21 mm from cut-line turn point (345.88): coincident | Seamly2D geometry; minor |
| CollarBaseInterface_M pt 7, CollarBase_M pt 20 | Turn point on a 1.7° curve | DXF.16 run-end rule |
| Yoke_M pt 14 (sew line) | Turn point mid straight edge | Probably greedy run scan splits the edge |
| ShortSleeve_M pt 31 (advisory) | Sew-line turn point at hem corner on a straight edge | Seamly2D tag; low priority |

### DXF.19 — turn points only where the tangent breaks (implemented)

- Branch `task-dxf-turn-points`, merged into local `run-seamlyLayout`. See `TODO_DXF.md` DXF.19 for rule and checks.
- User decision: open contour ends (L8, L11) are curve points, tagged or not.
- Rule values found on `male_shirt_202610071910.svg`: break > 2 × next vertex angle and > 1°; single segment ≥ 20 mm. A 0.5° floor or 10 mm single segment added 9 false turn points on flat curves and cap chords.
- `cargo test --workspace`, `ctest --preset debug`: pass.
- DXF.20 filed: Yoke L14 (probably fixed), ShortSleeve pt 31 tag, FullSleeve slit coincidence.

### Tuck / fold lines (answered, no action)

- D6673 has no fold-line layer. Fold and stitch lines → layer 8; tuck legs → layer 4 notches; inner end → layer 13 drill hole; fold/press direction → layer 15 text. Layer 6 = cut-on-fold mirror line only.
- Repo sample `seamlylayout_Skirt_ASTMD6673.dxf` uses non-standard named layers `FOLD`/`STITCH`.
- Possible later work: Seamly2D tags fold vs stitch internal paths; export writes layer 15 text. Not filed.

### Next steps

1. `local_build_msi.ps1` on `run-seamlyLayout` (verifies `becb0c7c18` and DXF.19); then push.
2. User: install MSI; re-export male_shirt DXF-ASTM (R13); run aw.fyi (DXF.19.7, DXF.8).
3. Still open: Seamly2D.10.6 / DXF.18.5 (built-in SA slit depth check); DXF.3.5 (CLO3D import); DXF.20.

## 2026-10-05 — Seamly2D.10 + DXF.18: seam allowance width per notch

Merged `task-notch-seam-allowance`. `local_build_msi.ps1` passed (build, `nmake check`, MSI). New Qt tests checked in the suite logs. `cargo test --workspace`, `ctest --preset debug` passed.

- Producer: `VPiece::createNotchLines` gives a width per notch line (`MaxLocalSA`); `VLayoutPiece` keeps it; notch group gets `data-seam-allowances` (mm per subpath).
- Consumer: Seamly2D width first, then measured width (`seam_allowance_at`), then drawn depth.
- Built-in seam allowance: main path is the cut line, sent as `seamline`; no sew line. Depth comes only from the attribute.
- Open: Seamly2D.10.6 / DXF.18.5 — user installs the MSI, hands off a built-in piece, checks slit depths. Then move both tasks to `TODO_COMPLETED.md`.
- User edit to `CLAUDE.md` (build command section) left uncommitted.
- Build rule: build only with `local_build_msi.ps1` (user decision).

## 2026-10-05 — DXF.17 / Layout.65: every notch is a layer 4 slit

Merged `task-notch-slit`. `cargo test --workspace`, `ctest --preset debug` passed.

- `astm_notch.rs`: no shape classification. Every notch: `NotchKind::Slit`, layer 4, no width.
- Depth = half the seam allowance width at each notch. `seam_allowance_at`: notch base → nearest sew line. No cut line or sew line: drawn depth.
- User decision: keep each edge's seam allowance as Seamly2D defines it (piece-wide median replaced, `task-notch-local-sa`).
- Seamly2D sends no seam allowance width attribute. Where the width changes at the notch node, the measurement can give the narrower width; Seamly2D uses the wider one. A per-notch producer attribute would be exact; not filed.
- `NotchKind` T/castle/U kept: encoder still writes them; Layout.66 `data-notch-type` may use them.
- Checked on `test-seamly-layout-input/male_shirt_202610050911.svg`: 7 slits, depth 5.00 mm.
- DXF.17, Layout.65 moved to `TODO_COMPLETED.md`.
- User actions open: DXF.8 (re-export R13, re-run aw.fyi); DXF.3.5 (CLO3D import); Seamly2D.6.4 handoff check in the running app.

## 2026-10-05 — Seamly2D.6.4: handoff cutline notches clipped to the cutline

Merged `task-handoff-notch-clip`. Seamly2DTests, CollectionTest, ParserTest, TranslationsTest passed (run directly from each `bin\`).

- Cause: `VLayoutPiece::Create` called `createNotchLines(pattern)` with no cutline points. Diamond, U, V, castle ends missed the cutline at corners and curves.
- Fix: pass the seam allowance points, as the canvas does. Task Seamly2D.6 moved to `TODO_COMPLETED.md`.
- Test: `TST_VPiece::LayoutNotchesMatchCanvas`. Failed before the fix (diamond end ~3 px off).
- Not checked in the running app. User action: male_shirt Layout Mode handoff; check 7 notches sit on the cutline.
- Test file: user's uncommitted `test-seamly-layout-input/male_shirt.sm2d`. Do not commit it unasked.
- Next: DXF.17 / Layout.65. Every notch becomes a layer 4 slit, length = half the seam allowance width.
- User actions still open: DXF.8 (re-export R13, re-run aw.fyi); DXF.3.5 (CLO3D import).

## 2026-10-05 — DXF.15 + DXF.16: straight runs end in turn points

Merged `task-dxf-turn-points`. `cargo test --workspace`, `ctest --preset debug` passed.

- Source: aw.fyi report on `male_shirt_R13_202610041815.dxf` (2 departures, 5 advisories).
- `astm_contour.rs` `straight_run_ends`: both ends of a straight run are turn points, tagged or not. Covers the FrontPanel top junction and the FullSleeve hem run.
- Run rule: ≥ 10 mm, within 0.01 mm, ≥ 2 inner vertices (1 if an end is a turn point).
- Fixture `male_shirt_pieces.svg` is older than the user's current `male_shirt.sm2d`; it has neither junction. Checked against the report file's layers 84/87 instead.
- New: DXF.17 (every notch a layer 4 slit, half the seam allowance). Waits for Seamly2D.6.
- User action: export DXF-ASTM (R13); re-run aw.fyi (DXF.8).

## 2026-10-04 — Seamly2D.6: cutline notches no longer depend on the view setting

Merged `task-cutline-notch`. Seamly2DTests, CollectionTest, ParserTest, TranslationsTest passed (run directly; `nmake check` wrapper returns 9009 in this shell).

- User decisions: fix Seamly2D.6 here, not upstream first. Seamly2D.6 comes before Layout.65.
- Fix: `VPiece::createNotchLines(..., includeCutlineNotches)`. Only the canvas passes `showSeamAllowances()`. Layout and export always get cutline notches.
- Test: `TST_VPiece::NotchFlagsSelectLine`. Straight edge, slit: each flag puts its notch on the correct line.
- Reported canvas symptom not reproduced in the unit test. Sample notch nodes have both flags `false`.
- Layout.65 rule recorded: DXF writes every notch as a slit, length = half the seam allowance width.
- User action: Seamly2D.6.1.2 — reproduce on the canvas; give notch type, subtype, edge shape, seam allowance view state.
- Next: Seamly2D.6.3.2 after that report, then Seamly2D.6.4, then Layout.65.

## 2026-10-03 — Layout.20: annotated DXF export name has `_annotated`

Merged `task-dxf-annotated-name`. `cargo test --workspace`, `ctest --preset debug` passed.

- DXF flow: missing-data warning → Standard/Annotated dialog → save dialog → export.
- Annotated name: `male_shirt_R13_annotated_<stamp>.dxf` + `.txt`. Standard name unchanged.
- Rust `exports::dxf_export_name_segment`; QML `appController.dxfExportNameSegment`.
- User action: Export each DXF-ASTM variant, Standard and Annotated; check default names and Cancel in both dialogs.

## 2026-10-03 — Layout.17 + Layout.18: HPGL and SVG export names show the mode

Merged `task-export-name-segments`. `cargo test --workspace`, `ctest --preset debug` passed.

- HPGL Cut → `_cutlines`; SVG single-line font → `_singlelinefont`; Hershey → `_hersheyfont`. Other modes unchanged.
- One Rust mapping: `exports::export_name_segment`; QML `appController.exportNameSegment`.
- Next: Layout.20 (`_annotated` export name).
- User action: Export > HPGL > Cut, and each SVG text mode; check default names.

## 2026-10-03 — Layout.21: "teaching version" is now "annotated version"

Merged `task-annotated-rename`. `cargo test --workspace`, `ctest --preset debug` passed.

- Rename only; no behavior change. JSON key `createAnnotatedVersion` is not saved, so no migration.
- Full CI not skipped: `CMakeLists.txt` changed.
- Next: Layout.20 (`_annotated` export name).
- User action: export DXF-ASTM, choose Annotated Version; check dialog text.

## 2026-10-03 — DXF.14: every real value has 2 decimals

Merged `task-dxf-two-decimals`. `cargo test --workspace`, `ctest --preset debug` passed.

- R13 fixed values (`0.0`, `1.0`, …) now `0.00`, `1.00`, …. R12 was already 2 decimals.
- User action: export DXF-ASTM (R13); re-run aw.fyi (DXF.8).

## 2026-10-03 — DXF.13: no `Category:` piece text

Merged `task-dxf-drop-category`. `cargo test --workspace`, `ctest --preset debug` passed.

- `Category:` removed from R12, R13 and CLO3D (one writer). User decision: DXF-AAMA only.
- User action: export DXF-ASTM (R13); re-run aw.fyi (DXF.8).

## 2026-10-03 — DXF.12: piece text `Size Name:`

Merged `task-dxf-size-name`. `cargo test --workspace`, `ctest --preset debug` passed.

- `Size:` → `Size Name:` in R12, R13 and CLO3D (one writer).
- User action: export DXF-ASTM (R13); re-run aw.fyi (DXF.8).

## 2026-10-03 — DXF.11: key points pass a chord-length reader spline

Merged `task-dxf-reader-neutral-spline`. `cargo test --workspace`, `ctest --preset debug` passed.

- Cause: aw.fyi flagged the CollarTop seam line hook (0.85 mm, limit 0.5). Its spline is not centripetal.
- `astm_contour.rs` checks centripetal and chord-length Catmull-Rom (`READER_ALPHAS`). A bulge between adjacent dense vertices splits the dense edge.
- male_shirt curve points 278 → 311.
- User action: export DXF-ASTM (R13); re-run aw.fyi (DXF.8).
- `male_shirt.sm2d` sample has uncommitted user edits (TestPiece1, label lines). Not committed.

## 2026-10-03 — Layout.19: DXF-ASTM (CLO3D) writes R13, name `_CLO3D`

Merged `task-dxf-clo3d-r13`. `cargo test --workspace`, `ctest --preset debug` passed.

- User decision: CLO3D variant must be D6673-10 compliant, so it writes R13 (`AC1012`). Reverses "CLO3D stays R12".
- Default name: `<base>_CLO3D_YYYYMMDDHHMM.dxf`.
- Risk: DXF.3.5 still open — no one has imported an R13 file into CLO3D. User action: export DXF-ASTM (CLO3D); import in CLO3D.

## 2026-10-03 — DXF.10: DXF-ASTM export name shows the DXF version

Merged `task-dxf-version-name`. `cargo test --workspace`, `ctest --preset debug` passed.

- Default names: `<base>_R12_YYYYMMDDHHMM.dxf`, `<base>_R13_YYYYMMDDHHMM.dxf`. CLO3D unchanged (Layout.19 adds `_CLO3D`).
- Rust `exports::default_export_file_name` builds every default name; QML `makeExportFileName(ext, tiled, segment)` calls it. Layout.17.2 done.
- Not checked in the running app. User action: export R12 and R13; confirm save-dialog names.

## 2026-10-03 — DXF.7: Size, Category, Material piece text

Merged `task-dxf-piece-size-material`. Local MSI build + `nmake check`, `cargo test --workspace`, `ctest --preset debug` passed.

- Piece text order: `Piece Name:`, `Size:`, `Category:`, `Quantity:`, `Material:`.
- `Size:` = piece `data-size` (multisize), else new pattern attribute `data-sample-size` (individual: `bust_circ`, else `waist_circ`).
- `Category:` written empty (piece classification; Seamly2D stores none). `Material:Fabric` until Layout.15.4.
- Handoff contract changed: Seamly2D `SvgGenerator::setSampleSize`; docs `SVG-DATA-ATTRIBUTES.md`, `NEW-ATTRIBUTES.csv`.
- New docs: `dxf-docs/seamlylayout_DXF_ASTM_D6673_{HISTORY,INCONSISTENCIES,from_AAMA,MATERIAL}.md`.
- Not checked in the running app. User action: export male_shirt DXF-ASTM; confirm `Size:102`, `Category:`, `Material:Fabric`; re-run aw.fyi (DXF.8).

## 2026-10-03 — DXF.6: Style Name is the input base name

Merged `task-dxf-style-name`. `cargo test --workspace` and `ctest --preset debug` passed.

- `Style Name:` = imported SVG base name, or `--document-name` value. Else pattern `data-name`, else output file stem.
- QML sends `styleName` (`importedBaseName`) in the `exportDxf` options JSON.
- Not checked in the running app. User action: export male_shirt DXF-ASTM; confirm `Style Name:male_shirt`.

## 2026-10-03 — DXF.5: warn about incomplete pieces before DXF-ASTM export

Merged `task-dxf-missing-piece-data`. `cargo test --workspace` and `ctest --preset debug` passed.

- Before the DXF save dialog, `DxfMissingDataDialog.qml` names pieces with no `Cut N` line, no label, or no grainline. Buttons: Export anyway, Cancel.
- Text built in Rust (`exports::dxf_missing_piece_data_message`); male_shirt test pins the three interface pieces.
- New QML file in `qt_frontend/CMakeLists.txt`, so the merge runs full CI.
- Not checked in the running app. User action: export male_shirt DXF-ASTM; confirm the dialog lists CollarBaseInterface, CollarTopInterface, CuffInterface.

## 2026-10-03 — DXF.4: fewer curve points

Merged `task-dxf-fewer-curve-points`. `cargo test --workspace` and `ctest --preset debug` passed.

- `astm_contour::prune_to_spline` drops curve points the spline does not need. male_shirt: 480 → 278 curve points, all splines ≤ 0.25 mm.
- Key-point chords can now exceed 0.25 mm; only the spline is held to it.
- User action: re-export male_shirt DXF-ASTM, re-run aw.fyi (DXF.8). Check the advisory and the reconstruction findings.

## 2026-10-03 — DXF.3: DXF-ASTM (R13) export

Merged `task-dxf-r13`. `cargo test --workspace` and `ctest --preset debug` passed.

- Export menu: **DXF-ASTM (R12)** (was **DXF-ASTM**), new **DXF-ASTM (R13)**, **DXF-ASTM (CLO3D)** stays R12 (user decision). View menu: one **DXF-ASTM** item.
- `ezdxf2dxfastm/src/r13.rs` rewrites the R12 group stream as AC1012; D6673 content is identical in both versions.
- QML passes `dxfVersion` (`"R12"`/`"R13"`) in the `exportDxf` options JSON.
- ezdxf `audit` and libdxfrw read the male_shirt R13 file without errors.
- User action: export DXF-ASTM (R13), import in CLO3D, run aw.fyi (DXF.3.5, DXF.8).
- Not checked in the running app.

## 2026-10-03 — DXF.2: spline through key points within tolerance

Merged `task-dxf-spline-tolerance`. `cargo test --workspace` and `ctest --preset debug` passed.

- `astm_contour::refine_to_spline` adds curve points until a centripetal Catmull-Rom spline is within 0.25 mm of `dense`.
- aw.fyi's spline method is unknown; Catmull-Rom is a guess. Sleeves: 0.40 mm → 0.24 mm under the stand-in.
- Next in `TODO_DXF.md`: DXF.3 (R13), DXF.4 (fewer points; retest first).
- User action: re-export male_shirt DXF-ASTM, re-run aw.fyi (DXF.8).

## 2026-10-03 — DXF.9: `Author:Seamly;Seamly2D;<version>`

Merged `task-dxf-author-format`. `cargo test --workspace` passed.

- aw.fyi flagged `Author:Seamly2D 26.10.314` as a departure; D6673 needs `vendor;application;release #`. User chose `Seamly;Seamly2D`.
- Remaining report items tracked in `TODO_DXF.md`: DXF.2 (tolerance), DXF.3 (R13), DXF.4 (advisory).
- User action: re-export male_shirt DXF-ASTM, re-run aw.fyi.

## 2026-10-03 — Seamly2D.10: pattern label left of the grainline

Merged `task-pattern-label-left`. `local_build_msi.ps1` passed: build, `nmake check`, MSI.

- New piece and Union piece: pattern label right edge 1 cm left of grainline; piece label stays 1 cm right.
- Existing pieces keep their stored label positions; only new pieces change.
- A piece too narrow for a label left of the grainline still gets shifted inside by `UpdateLabelItem`.
- Not checked in the running app.

## 2026-10-03 — DXF.1: Seamly2D tags turn points (`data-turn-points`)

Merged `task-dxf-turn-points`. `nmake check` (4 Qt suites), `cargo test --workspace`, `ctest --preset debug` passed.

- Plan: `project-docs/docs-seamlylayout/TODO_DXF.md`, from the aw.fyi report `test-seamly-layout-input/male_shirt_202610031234-dxf-report.pdf`. Next: DXF.2 (spline tolerance check).
- Contract change on both sides: `docs-data/SVG-DATA-ATTRIBUTES.md` `data-turn-points`.
- Thresholds 0.5° / 5° / 25° are in `vabstractpiece.cpp`; 5° chosen because the FrontPanel neckline join bends about 2°.
- User action: export male_shirt DXF-ASTM from the app, run it through aw.fyi (DXF.8).
- Not checked in the running app.

## 2026-10-03 — DXF `Author:` is `Seamly2D <version>`

Merged `task-dxf-author-version`. `cargo test --workspace` and `ctest --preset debug` passed.

- DXF style system text: `Author:Seamly2D 26.9.2609`, was `Author:Seamly2D Project;SeamlyLayout;0.1.0` (user decision). One writer covers DXF-ASTM and CLO3D.
- SeamlyLayout now reports the suite version: `qt_frontend/CMakeLists.txt` reads `VER_FILEVERSION_STR` from `src/libs/vmisc/projectversion.h` into `SEAMLY_SUITE_VERSION`; `setApplicationVersion` uses it. `--version` shows it too.
- Rust fallback without the frontend (tests, CLI) stays `CARGO_PKG_VERSION`.
- Open tasks Layout.17–21: name segments `_cutlines`, `_singlelinefont`, `_hersheyfont`, `_CLO3D`, `_annotated`; rename "teaching" to "annotated".
- Not checked in the running app.

## 2026-10-03 — Export name timestamp, Tiled PDF hint

Merged `task-export-name-tiled-hint`. `cargo test --workspace` and `ctest --preset debug` passed.

- Default export name is `<baseName>_YYYYMMDDHHMM[_tiled].<ext>`, e.g. `male_shirt_202609222209.svg` (`makeExportFileName`, user decision). Seconds removed.
- Two exports in the same minute get the same default name; the save dialog asks before overwrite.
- Export > PDF (Tiled) stays disabled unless Paper Type is Tiled (user decision: page size comes from tile size). Disabled text reads "PDF (Tiled): set Paper Type to Tiled". Text, not tooltip: a disabled item gets no hover.
- Not checked in the running app.

## 2026-10-02 — New-piece grainline, label placement, template file name

Merged `task-grainline-defaults`. `local_build_msi.ps1` passed: build, `nmake check`, MSI.

- New piece and Union piece: labels sit 1 cm right of the grainline (`VPatternLabelData::defaultPieceLabelPos` takes `VGrainlineData::lineRect`). A hidden grainline counts too.
- Existing piece with a hidden grainline: Pattern Piece Tool shows the Preferences length, rotation, arrow length.
- Preferences > Pattern grainline label "Angle:" is now "Rotation:", to match the Pattern Piece Tool.
- Edit Label Template shows "Template file: …": the imported or exported file, or the Preferences default when the lines match it (`NewPieceDefaults::pieceLabelTemplateFile`). Else "none": the pattern stores lines, not a file.
- Not checked in the running app. Check: new piece → labels clear of grainline; open label editor → file name shown.
- Risk: on a narrow piece the labels can extend past the right edge.

## 2026-09-29 — Export all tabs (Layout.36.2)

Merged `task-export-all-tabs`. `cargo test --workspace`, `ctest --preset debug` passed.

- Export > "Export all tabs": checkable, shown only when the layout has tabs. While checked, every format writes one file per tab.
- File name: tab label before the extension (`layout_views::tab_file_path`): `shirt_size-36.pdf`.
- Bridge: `select_export_tab` (silent swap, no canvas reload) and `export_tab_path`. QML: `runExport()`, `exportEachTab()` in `Main.qml`.
- One "Export Complete" dialog lists every file. First failure stops the run; earlier files stay on disk.
- Not checked in the running app. Check: multisize layout → check "Export all tabs" → export PDF, Tiled PDF, DXF → one file per tab; shown tab unchanged.

## 2026-09-29 — Sized layouts and size tabs (Layout.37.1–37.3)

Merged `task-sized-layout-tabs`. `cargo test --workspace`, `ctest --preset debug` passed.

- Multisize Create Layout: "All sizes" layout first, then each size alone (`do_process_size_layout`, `piece_extractor::keep_size`), one size per QML timer tick.
- Progress popup names each size: "Laying out size 36 (2 of 5)…". One warning at the end lists layouts that left pieces out.
- Tabs above the right canvas. `layout_views.rs`: selecting a tab swaps the layout fields, so Adjust Mode and exports use the selected tab. Sheet PDF filters the import to the tab's size.
- User decisions: pack at Create Layout; first tab "All sizes"; 37.4 multi-page PDF dropped: one PDF per tab (2026-09-29).
- Open: 38.2 real fixture; 38.3 running-app check.
- Not checked in the running app. Check: multisize pattern → Create Layout → popup names sizes → tabs switch → Adjust on a size tab → export.

## 2026-09-29 — Multisize handoff: Nested and Marker layouts (Layout.30–35)

Merged `task-multisize-layout`. `cargo test --workspace`, `ctest --preset debug`, `Seamly2DTests` passed. CollectionTest, ParserTest, TranslationsTest not run: no code they cover changed.

- Seamly2D: multisize Layout Mode hands off every size at base height; each piece is a `data-type="piece-set"` group, one `piece` per size (`data-size`).
- Pattern group: `data-measurements`, `data-sizes`, `data-base-size`. Contract: `project-docs/docs-data/SVG-DATA-ATTRIBUTES.md`, "Multisize pattern".
- `exportPiecesAs()` batch export now uses `recalculateAtSize()`: size in pattern units, as `ChangedSize()` does. Before, it converted the size to cm, which is wrong for a mm pattern.
- SeamlyLayout: setting `multisizeLayout` (`nested` default | `marker`), shown only when `AppController.isMultisize`.
- Nested: `hoist_piece_sets`; each size turned on its own grainline; `svg_dom::center_piece_sets` stacks sizes on the largest, largest first. Marker: sets dissolve, label "Name (size N)".
- Open items: see the sized-layouts entry above.
- Not checked in the running apps. Check: open a pattern with a `.smms` file (e.g. `src/app/share/samples/measurements/multisize/gost_man_ru.smms`), enter Layout Mode, run Nested and Marker.

## 2026-09-29 — DXF-ASTM (CLO3D) export variant (Layout.57)

Merged `task-dxf-astm-clo3d`. `cargo test --workspace`, `ctest --preset debug` passed.

- Export > DXF-ASTM (CLO3D): same D6673 file plus group 250 (0 on layer 1, 2 on layer 14 key-point polylines).
- Writer option `clo3d_group_250`; bridge JSON `clo3dGroup250`; QML `requestDxfExport(clo3d)` shared by both menu items.
- Not checked in CLO3D, VStitcher or the running app.

## 2026-09-28 — DXF-ASTM to ASTM D6673-10 (Layout.55)

Merged `task-dxf-astm`. Full CI: `Cargo.toml` changed. `cargo test --workspace`, `ctest --preset debug` passed. `local_build_msi.ps1` not run: no Seamly2D or SeamlyMe code changed.

- Rules, layers and decisions: `project-docs/docs-seamlylayout/dxf-docs/seamlylayout_DXF_ASTM_D6673_COMPLIANCE.md`.
- User decisions: METRIC only (px → mm, 2 decimals); keep AC1009; drop CLO3D group 250 and the duplicate L2/L3 set; keep `_M` block names; L1 key points + L84 dense (curve tolerance 0.25 mm); SeamlyLayout-only scope.
- New: style and piece system text, real seam line on L14, L84–L87, L8/L11/L15, notch POINTs (4/80/81/83), turn/curve classifier.
- New modules `astm_contour.rs`, `astm_notch.rs`; tests `astm_piece_test.rs`, `writer_astm_test.rs`; fixture `ezdxf2dxfastm/test_data/male_shirt_pieces.svg`.
- `ezdxf2dxfastm` dev-dependency `svg_dom`; `rust_crate_notices.txt` fingerprint regenerated.
- QML `Main.qml` sends `appVersion`, local `creationDate`, `creationTime` in the DXF options JSON.
- Not checked in the running app, nor re-analysed at aw.fyi/dxf. Check: export male_shirt layout to DXF-ASTM; upload to aw.fyi/dxf; compare with `test-seamly-layout-input/male_shirt_202609241936-dxf-report.pdf`.
- Follow-ups filed as Task Layout.6 (grading, L6, L13, L9/L10, empty notch paths from Seamly2D, producer attributes).

## 2026-09-28 — JPG export (Layout.54)

Merged `task-jpg-export`. Full CI: `Cargo.toml` changed. `cargo test --workspace`, `ctest --preset debug` passed. `local_build_msi.ps1` not run: no Seamly2D or SeamlyMe code changed.

- `app_core::render_jpeg` (quality 90, fixed); bridge `AppController::export_jpeg`; Export > JPG.
- `image` crate, `jpeg` feature only. Already in `Cargo.lock` through `svg2pdf`; `rust_crate_notices.txt` fingerprint regenerated.
- JPEG limit 65535 px per side: error before render, no file.
- User decisions: success dialog after export (no viewer); View > PNG becomes View > PNG/JPG; Preferences label "Image Viewer (PNG, JPG)". INI key stays `png_viewer_path`.
- Preference label column widened 160 → 200 px in `PreferencesWindow.cpp` and `PreferencesPanel.qml`, so the longer label fits.
- Not checked in the running app. Check: Export > JPG; View > PNG/JPG opens a `.jpg`; Preferences label fits.

## 2026-09-28 — local_build_msi.ps1 echo fix

Merged `task-build-echo-ampersand`. Full CI: touches `packaging/**`.

- Two `echo` banners had a bare `&`; `cmd.exe` ran the text after it. Caused the `CMake Error: ... "Seamly2D-private/path" does not exist` log line. `&` → "and".
- Checked: new lines in `cmd.exe`; PowerShell parse. Full local build not rerun.

## 2026-09-27 — PS/EPS export (Layout.53); pdftops retired

Merged `task-ps-eps-writer`. Full CI: changes `ci.yml`, `*.pro`, `packaging/**`, `Cargo.toml`.

- New crate `crates/ps_writer`: one writer, PS and EPS headers. Bridge `AppController::export_postscript`; Export > EPS, Export > PostScript (PS).
- Seamly2D (user decision, option B): PS/EPS removed from every export path and the CLI. PDF and SVG stay. `LayoutExportFormat::PS`/`EPS` enum values kept so CLI numbers do not move.
- `pdftops` removed: both tracked binaries, `seamly2d.pro`, `ci.yml`, `local_build_appimage.sh`, READMEs, MSI test template. Installer.7.2 closed.
- New Qt test `TST_ExportFormatCombobox` (Seamly2DTest).
- `rust_crate_notices.txt` regenerated.
- Output checked structurally only (no Ghostscript on this machine). Check: export a layout to EPS and PS; open in Ghostscript, Illustrator or Inkscape.
- Deferred (user decision): tiled multi-page PS, now Layout.56.

## 2026-09-27 — View > HPGL and HPGL viewer preference

Merged `task-hpgl-viewer`. Full CI: touches `packaging/**`. `ctest --preset debug` and `smsi_ensure_user_data_test.ps1` passed.

- `hpgl_viewer_path` preference; default `https://tiny-online.tools/embroidery-cnc-tools/hpgl-plt-viewer`. Missing key falls back to it.
- Seeded by `smsi_ensure_user_data.ps1`, `default_preferences.json`, and `seedFromBundledDefaults()`.
- Preferences window: HPGL Viewer row above Projector. `PreferencesPanel.qml` (not used) unchanged.
- Not checked in the running app.

## 2026-09-27 — HPGL export (Layout.52)

Merged `task-hpgl-export`. Full CI: new crate changes `Cargo.toml`. `cargo test --workspace`, `ctest --preset debug` passed. `local_build_msi.ps1` not run: no Seamly2D or SeamlyMe code changed.

- New crate `crates/hpgl_writer`; bridge `AppController::export_hpgl`; Export > HPGL submenu.
- Pen per line class (1-8) and extension (`.plt` / `.hpgl`) chosen in submenus, saved in the preferences INI.
- `rust_crate_notices.txt` regenerated for the new `Cargo.lock`.
- Menu checked offscreen (`qml.exe`); not checked in the running app or on a plotter. Check: export a layout, open the `.plt` in a HPGL viewer.
- Workflow: `project-docs/docs-seamlylayout/export-docs/seamlylayout_EXPORT_WORKFLOW.md`.

## 2026-09-27 — Paid export stubs: G-Code and 3D mesh (3MF)

Merged `task-paid-export-stubs` with skip-ci. `cargo test --workspace`, `ctest --preset debug` passed.

- Gate `exports::paid_export_available()` returns `false`; the Export menu hides both items.
- Not checked in a running app. Check: Export menu shows no G-Code or 3D Mesh row and no blank row.

## 2026-09-27 — One writer for the SeamlyLayout debug log

Merged `task-single-log-writer` with skip-ci. `cargo test --workspace`, `cargo test --release -p cxxqt_bridge log_sink`, `ctest --preset debug` passed.

- C++ `Logger` is the only writer; Rust `log_to_file()` sends lines through `seamly_layout_set_log_sink()` (registered in `seamlyLayout_main.mm`).
- `SEAMLY_LOG_FILE` removed; use `Logger::filePath()`.
- Not checked in a running app. Check: one debug session log holds both C++ and Rust lines, none clipped.

## 2026-09-26 — New pieces read Preferences > Pattern defaults

Merged `task-new-piece-defaults`. Full CI: the change touches `vmisc.pro` and `tools.pri`. Local build + all Qt suites passed.

- `NewPieceDefaults` (`vtools/tools/new_piece_defaults.{h,cpp}`): label size, grainline and arrow length in pattern units; label templates.
- Missing user template → built-in resource `:/labels/*.xml`. `ensureDataRootTree` seeds both templates at startup.
- New pieces only. Existing pieces with empty label text stay as they are.
- Not tested in the GUI. Check: new piece shows piece + pattern label text; cm pattern with inch settings gets correctly sized labels.

## 2026-09-25 — Piece and pattern labels beside the grainline

Merged `task-label-placement` with skip-ci.

- New piece (dialog, Union tool): piece label centered 1 cm right of bounding box center; pattern label 1 cm right of piece label.
- Only new pieces. Existing pieces keep stored label positions.
- Not tested in the GUI.

## 2026-09-25 — Grainline default angle, centered, pointing up

Merged `task-grainline-default-angle` with skip-ci. Local build + all Qt suites passed.

- Setting `pattern/defaultGrainlineAngle` (default 90), Preferences > Pattern > Grainlines > Angle.
- `VGrainlineData::upwardAngle` / `centeredStart`: used by `PatternPieceDialog::placeGrainline`, `union_tool`, `PatternPieceTool::SaveRotateGrainline`, `VLayoutPiece::setGrainAxis`.
- Not tested in the GUI.
- `verticalize_dom` now turns grain to point up (θ = −90° in SVG), user decision. Upward grainlines get no rotation. Merged `task-verticalize-grain-up` with skip-ci.

## 2026-09-25 — Adjust Apply fits roll-form frame to pieces

Merged `task-adjust-roll-trim`, then `task-adjust-roll-fit`, with skip-ci (Rust-only changes).

- `LayoutSettings::is_roll_form()`: fabric, or paper + roll. Also used by `process_layout` trim.
- `AppControllerRust.is_roll_layout`: set by `process_layout`.
- `accept_adjustments` calls `fit_roll_frame_to_pieces_in_adjust_dom` (user decision: trim top, grow bottom).
  - Measures a flattened clone.
  - All pieces move so the top piece sits 1 px below contentRect top (user decision: 1 px gap, top and bottom). Shift is pre-multiplied into each piece `matrix(...)`.
  - contentRect ends 1 px below the lowest piece: shrinks or grows. Margins kept.
- Not tested in the GUI.

## 2026-09-24 — Pieces without a grainline: setting + handoff grain angle

Branch `task-no-grainline-rotation`. User decisions: a setting with default "Keep upright"; the handoff carries the grain angle.

- Setting `noGrainlineRotation` ("upright" | "free"): `SettingsModel`, `SettingsDialog.qml` ("No Grainline:", hidden in Any mode), `LayoutSettings`.
- `pack_types::Rect.free_rotation`: packer adds 90°/270° for that piece only. Set by `piece_extractor::set_free_rotation_without_grainline`.
- Seamly2D: `VLayoutPiece::setGrainAxis` keeps the grain direction even when the grainline is hidden; `SvgGenerator` writes `data-grainline-angle`.
- `svg_dom::verticalize_dom` reads drawn grainline, then `data-grainline-angle`.
- Resolved 2026-09-25: `verticalize_dom` now turns grain to point **up** (user decision).

## 2026-09-24 — Layout fix: pieces without a grainline keep drafted orientation

Merged `task-no-grainline-orientation` with skip-ci (Rust-only change).

- Cause: `svg_dom::verticalize_dom` fell back to the longest edge when a piece had no grainline. Trouser legs (no grainline) turned 90°, 110 cm wide on a 36" roll → unplaced.
- Fix: no labelled grainline → no rotation. Fallback removed from `svg_dom/src/transforms.rs`.
- Test: `trousers_handoff_fits_36_inch_roll` (`cxxqt_bridge/test_data/trousers-handoff_pieces.svg`).
- Units were correct: handoff is 96 px/in; not a unit bug.

## 2026-09-25 — License notices moved to `packaging/licenses/`

User decision: packaging files live at the top-level `packaging/`, matching the 2026-09-21 cleanup.
`src/app/seamlylayout/packaging/` no longer exists. Updated: `smsi.ps1`, SeamlyLayout `CMakeLists.txt`, `seamly2d.pro`, `seamlyme.pro`, `generate_license_notices.py`, `license_notices_tests.rs`, `.gitignore`, docs.

## 2026-09-24 — Task Layout.14: third-party license notices in every installer

Branch `task-license-notices`, merged without skip-ci (packaging, CMake, .pro changed).

- `packaging/licenses/` (moved 2026-09-25): `generate_license_notices.py`, `about.toml`, `rust_crate_notices.hbs`; committed `rust_crate_notices.txt` (174 crates) and `qt_notices.txt` (Qt 6.11.1, LGPL-3.0, from Qt SBOMs).
- Shipped: MSI `licenses\`, AppImage `usr/share/licenses/seamly/`, DMG app bundles `Contents/Resources/licenses/`.
- `license_notices_tests` (cxxqt_bridge) fails when the files are stale.
- Machine state: `cargo-about` 0.9.2 installed in `~/.cargo/bin`.
- Verified 2026-09-25 in CI run 36088119655: MSI, DMG (all 3 bundles) and AppImage carry the notices. Task moved to `TODO_COMPLETED.md`.
- Also fixed: Windows MSI MSVC setup step (merge residue) and AppImage `.desktop` path (`packaging/linux/`).
- Open: Installer.7 (xerces-c, pdftops, MSVC runtime notices).

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

## Still open in DXF-ASTM format

- Drill holes (Layout.63) and stripe/plaid lines (Layout.64) also have no Seamly2D source. Should they become new Seamly2D tasks, like fold lines?
- D2, Diamond notch: my recommendation is to infer it from its shape.
- D3, grade reference line: my recommendation is each size's grainline.
- D4, scope: phases 1–3, one merge per phase. Grading (phase 3) would use the node points above, which adds producer work to phase 3.