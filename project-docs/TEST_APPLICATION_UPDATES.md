# Application updates — user validation checklist

Use the latest Windows MSI built for these changes. Older installed applications do not contain this batch.

Mark each item PASS, FAIL, or BLOCKED. For failures, record the application, pattern, piece, settings, and export filename.
Save screenshots and export files beside the test results. Do not count a skipped test as passed.

Checked boxes preserve the user's recorded results; this review does not certify new tests.
An unchecked box means pending unless a FAIL or BLOCKED note states otherwise.
For repeated checks, record which patterns and materials were tested before checking the box.

## Review status and next checks

The original application batch passed automated build and export tests.
The newer relative-command-line-path fix has not been rebuilt or runtime-verified; section 14 needs rebuilt applications.

| Priority | Remaining validation | Sections |
|---|---|---|
| 1 | Rebuild before testing relative CLI paths; verify diagnostics and output paths. | 14 |
| 2 | Complete oversized-piece splitting, cancellation, and enlargement checks. | 11 |
| 3 | Complete Stripe/Plaid input errors, missing anchors, and graded drill diameter checks. | 7 |
| 4 | Verify DXF layers, units, piece text, grading, and external CAD import. | 8, 13 |
| 5 | Verify printed scale, page assembly, and image resolution. | 12 |
| 6 | Complete relocated-data-root, template provenance, derived-piece Flip, and adjustment checks. | 2, 4, 5, 9 |

### Missing result details

Some recorded checks still have blank evidence fields. Keep their checked boxes, but complete these details:

- [ ] Record tested About versions: Seamly2D `_____`; SeamlyMe `_____`; SeamlyLayout `_____`.
- [ ] Record drill diameter, units, pattern, and center anchor from section 7: `_____`.
- [ ] Record jacket2's displayed Base Size value and unit from section 9: `_____`.
- [ ] Record the turn-point warning, affected piece, first failing size, and export file from section 13, or write `none`.

## 1. Prepare the test files

Run from the repository root. Save edited patterns under new names; do not overwrite these originals.

| Pattern | Measurement file | Pattern units |
|---|---|---|
| `test-seamly-layout\patterns\jacket2_40-146.sm2d` | `test-seamly-layout\measurements\multisize\gost_man_ru.smms` | mm |
| `test-seamly-layout\patterns\male_shirt.sm2d` | `test-seamly-layout\measurements\individual\male_chest_102cm.smis` | cm |
| `test-seamly-layout\patterns\trousers.sm2d` | `test-seamly-layout\measurements\individual\trousers.smis` | cm |

- [x] Close all three applications before installing the latest MSI.
- [x] Start Seamly2D, SeamlyMe, and SeamlyLayout. Record each About version.
- [x] Save test copies of all three patterns in a separate validation folder.
- [x] Relink each copy to the measurement file listed above when prompted.
- [x] Confirm jacket2 uses the listed file, not its old link outside this repository.
- [ ] Record the starting piece names, layout inclusion, quantities, materials, and hidden labels/grainlines.
- [ ] Use separate export folders for jacket2, male_shirt, and trousers.
- [ ] Keep unmodified copies for baseline comparisons and separate copies for marking, copy, and flip tests.

## 2. SeamlyMe — measurements and file pickers

- [x] Open `male_chest_102cm.smis`. Measurements and units appear correctly.
- [x] Open `trousers.smis`. Measurements and units appear correctly.
- [x] Open `gost_man_ru.smms`. Its size and height ranges appear correctly.
- [x] Save each measurement file under a test name. Reopen it without errors or lost measurements.
- [x] Open Individual starts in the configured individual-measurement folder.
- [x] Open Multisize starts in the configured multisize-measurement folder.
- [x] Open Template starts in the configured templates folder.
- [x] Repeat after visiting another folder. Each picker still starts in its own configured folder.
- [ ] With a separate test data root, missing picker folders are created and remain after cancelling the picker.
- [ ] A folder-creation failure shows an error instead of silently opening another folder.
- [ ] With a configured or renamed test data root, all three default folders follow that root.
- [x] Templates remain under `<dataRoot>\templates`; no migration to `measurements\template` is expected.

Use a disposable data root for path tests. Do not rename or make the working measurement folders unwritable.

## 3. Seamly2D — conversion, save, and reopen

Repeat for jacket2, male_shirt, and trousers.

- [x] The older pattern opens without a schema error.
- [x] Draft and Piece Mode show the original geometry, names, materials, and layout inclusion.
- [x] Save the test copy, close it, and reopen it without errors.
- [x] The saved pattern uses format `0.7.9`.
- [x] Every draft block has `groups` followed by `images`, including blocks without images.
- [x] Measurement links, custom formulas, piece labels, seam allowances, and fold lines survive reopening.
- [x] Existing hidden labels/grainlines remain hidden; do not treat intentionally hidden objects as missing.

## 4. Seamly2D — label template filenames

Use male_shirt. Repeat the pattern-label checks on jacket2.

- [x] Piece Properties > Labels > Piece Label displays the template filename beside Edit template.
- [x] Piece Properties > Labels > Pattern Label displays the corresponding filename.
- [x] Pattern Preferences > Label data displays the pattern-label template filename.
- [x] Hovering over a filename shows its full path.
- [x] Selecting another template updates the displayed filename when the editor closes.
- [x] Cancelling template selection preserves the current label and filename.
- [x] Matching default template lines display the default filename.
- [ ] Custom stored lines without known file provenance display `none`, not an unrelated filename.
- [x] Template edits still change the rendered label correctly.

## 5. Seamly2D — Flip and placement properties

Use an asymmetric male_shirt piece, such as FrontPanel.

- [x] Flip choices are None, Horizontal, Vertical, and Both in Piece Properties and the context menu.
- [x] The context menu marks the current choice.
- [x] Horizontal mirrors the displayed outline left to right.
- [x] Vertical mirrors the displayed outline top to bottom.
- [x] Both applies both directions. None restores the unflipped display.
- [x] Seam line, cut line, notches, fold line, and added markings follow the displayed flip.
- [x] Labels and grainline retain their position and readability.
- [x] Undo and redo restore the displayed flip and its selected menu choice.
- [x] Save and reopen preserve the choice and display.
- [ ] Repeat Horizontal, Vertical, and Both on copied, united, and Mirror-on-Fold pieces.
- [x] Change a previously flipped piece to each other choice; the result does not accumulate extra flips.
- [ ] Rotation choices and Tilt direction/degrees still save and reopen correctly.
- [ ] Tilt None disables degrees; enabled degrees remain within 0–90.

The flip changes the displayed choice. It must not flatten or rewrite the stored draft geometry.
Section 6 records copied-source checks as passed; section 5's unchecked row still covers the full derived-piece Flip matrix.

## 6. Seamly2D — independent Copy Piece

Use male_shirt, then a jacket2 piece.

- [x] Right-click > Copy Piece creates a separate piece beside the source.
- [x] The copy has a distinct suffixed name and letter when a letter exists.
- [x] Copying the copy creates another distinct name.
- [x] The copy retains paths, seam allowance, fold line, labels, grainline, markings, and placement properties.
- [x] The copy opens in Pattern Piece Tool and remains editable.
- [x] Editing the copy does not change the source piece.
- [x] Editing source-only piece settings does not change the copy.
- [x] Moving a shared draft point updates both pieces correctly.
- [x] Undo removes the complete copy in one step; redo restores it.
- [x] Save and reopen retain both independent pieces.
- [x] Repeat with a united piece, a horizontally flipped piece, and a vertically flipped piece.
- [x] Repeat with a Mirror-on-Fold piece and a copy of a copy.
- [x] In a disposable copy, remove the source piece. The copied piece still recalculates and exports.

## 7. Seamly2D — drill holes and stripe/plaid references

Use a male_shirt test copy and jacket2 Back. Add suitable live anchors if the piece has none.

- [x] Piece Properties has a dedicated Drill holes page, separate from notch settings.
- [x] Add a hole at a live center anchor with a known positive diameter.
- [x] Record diameter and pattern units: `_____` in `_____`.
- [x] Zero, negative, nonnumeric, and missing diameters show an error and cannot be added.
- [ ] A missing or removed center anchor shows an error.
- [ ] Add one Stripe reference and one Plaid reference between distinct live anchors.
- [ ] Missing endpoints or coincident endpoints show an error.
- [ ] Record separate Stripe/Plaid results; previously checked general marking tests do not complete these two unchecked cases.
- [x] Remove selected markings; only the selected marking disappears.
- [x] Save and reopen preserve all valid markings and their types.
- [x] Piece Mode displays the markings at their anchor positions.
- [x] Moving a draft anchor moves its hole center or reference endpoint.
- [x] Flip, Copy Piece, Union, and Mirror-on-Fold retain and transform the markings correctly.
- [ ] Jacket2 size changes move hole centers with the graded geometry; diameter stays constant.
- [x] SeamlyLayout imports the same marking counts, positions, and physical diameters.
- [x] Layout adjustment moves the markings with their piece.
- [x] References do not enlarge packing bounds or prevent otherwise valid placement.

## 8. Fold lines and built-in seam allowance

Check jacket2 Back, male_shirt BackPanel, and CollarTopInterface-Built-in-SA.

- [x] Piece Mode shows both seam line and cut line where enabled.
- [x] Neither line includes an edge along the fold line.
- [x] Each line starts at the fold line end and ends at the fold line start.
- [x] The fold line, arrow line, and label remain visible where enabled.
- [x] Built-in seam line lies inside the cut line by the configured allowance.
- [x] CollarTopInterface-Built-in-SA has no seam-line spike at D11 or D12.
- [x] Hide Seam Line removes only that line; the cut line remains.
- [x] Marking two neighbouring fold-line nodes checks and locks On fold.
- [x] Removing the fold line releases and unchecks On fold when there was no independent user choice.
- [x] A third fold-line node is rejected.
- [x] Mirror-on-Fold creates a full symmetric piece without a fold line.
- [x] The mirrored source is excluded from the layout as intended.
- [x] SeamlyLayout, SVG, PDF, and DXF preserve the seam/cut/fold relationships.
- [x] A piece without a valid fold line exports `Fold:N`, including FrontPanel and mirrored full pieces.
- [x] A correctly defined on-fold piece exports `Fold:Y`.
- [x] DXF fold-line export does not warn that the fold line fails to connect with the cut line.
- [ ] DXF cut/seam layers are 1/14; the mirror line is layer 6.
- [x] Check varying-width notch depths and built-in seam allowance notches against the configured allowance.

## 9. Jacket2 — multisize handoff and layouts

Start from the baseline jacket2 copy. Use Paper > Roll, width 48 in, for the first layout.

- [x] Layout Mode prepares the size range and opens SeamlyLayout in front.
- [x] Repeating Layout Mode brings the existing window forward rather than creating a duplicate.
- [x] Fabric and Lining material tabs appear over both canvases.
- [x] Each material holds only its assigned pieces.
- [x] Marker, Nested, Base Size, and individual Size tabs match the pattern's actual size range.
- [x] Record the displayed base size: `_____`. Use that value in subsequent grading checks.
- [x] Marker contains every included piece at every requested size.
- [x] Nested stacks the sizes of each piece rather than scattering them.
- [x] Grainlines and label anchors stack consistently and remain inside each size's cut line.
- [x] Jacket2 Back's piece label fits inside Size 220 without an import warning.
- [x] Lining Detail 4's annotations also stack correctly.
- [x] Nested colors differ visibly; legend squares match their sizes.
- [x] Exported Nested SVG/PDF retains the colors, legend, and intended single annotation set.
- [x] Switching materials preserves each material's selected size tab and layout.
- [ ] Adjusting a piece moves its lines, labels, and markings together; saved adjustments persist.
- [ ] Material controls are locked while layout generation or adjustment requires it.
- [x] A Marker roll may grow beyond 500 in; canvas, adjustment frame, and SVG height agree.
- [x] No included size silently disappears. Any unplaced piece is named explicitly.

## 10. Male_shirt — individual handoff and materials

- [x] All included pieces arrive in SeamlyLayout; intentionally excluded pieces do not.
- [x] Names, letters, quantities, materials, hidden objects, and placement properties match Seamly2D.
- [x] With sufficient media, every included piece is placed.
- [x] Assign one test piece to Lining. Fabric and Lining layouts contain the correct separate pieces.
- [x] Save/reopen and repeated Layout Mode retain the material assignments.
- [x] Grainline/label placement, fold lines, built-in allowance, copied pieces, and markings match the edited test pattern.
- [x] Export filenames identify the pattern, material, tab, and format where applicable.

## 11. Trousers — oversized pieces and fabric

Start with Paper > Sheet, 36 × 36 in. Keep a baseline copy without added markings.

- [x] Oversized-piece dialog names the affected legs, including Back and the applicable Front variants.
- [x] Heading, descriptions, list, and action labels are readable.
- [ ] Cancelling leaves the application responsive and allows Layout Settings to be changed.
- [ ] Split the pieces creates labelled parts with matching join identifiers and match notches.
- [x] Every part is accounted for; any unplaced part is named in the warning.
- [ ] SVG/PDF/DXF exports show the same split parts and joins.
- [ ] Make the media bigger retains whole pieces and enlarges the frame sufficiently.
- [ ] A successful enlarged layout has no missing pieces or unexplained warning.
- [x] Repeat on a narrow paper roll; canvas and exported dimensions reflect any enlargement.
- [x] Set Fabric width 36 in, Folded, giving 18 in working width.
- [x] Fabric starts with a 500 in length and trims to the completed layout.
- [x] Folded fabric has zero left margin.
- [x] Every included trousers piece fits within the usable width when the chosen settings permit placement.
- [x] If pieces cannot fit, the warning names them and suggests fabric width/length, not a larger paper sheet.
- [x] Repeat unfolded fabric and paper settings; neither keeps incorrect folded-fabric margins.
- [x] Turn Place fold lines on the left edge on and off; fold placement follows the selection.

The checked part-accounting result does not complete the unchecked split-generation and split-export tests.
Compare whole-piece and split-piece runs separately; record the settings and piece counts for each.
Folded fabric keeps zero left margin even when left-edge fold placement is disabled.

## 12. Export menu and physical scale

Use jacket2 for tab restrictions and male_shirt/trousers for individual layouts.

- [x] Export has one DXF submenu with readable, untruncated text.
- [x] Marker/Nested show the note directing DXF export to Base Size or a size tab.
- [x] DXF items are disabled on unsupported tabs.
- [x] No obsolete DXF-ASTM R12 export remains.
- [x] Marker PDF (Tiled) is disabled for single-tab export.
- [x] Export all tabs omits unsupported Marker/Base Size tiled-PDF outputs without freezing.
- [x] Export SVG using each offered text mode; labels, geometry, and grouping are correct.
- [x] Export PDF, tiled PDF, PNG, JPG, PS, EPS, and HPGL where available.
- [x] Each file opens in its appropriate viewer and contains the expected pieces.
- [x] Export all tabs uses distinct material/tab filenames without overwriting unrelated outputs.
- [x] Nested non-DXF exports show the intended smallest-size grainline/label set per piece.
- [ ] PNG/JPG print resolution and dimensions match the selected settings.
- [ ] Letter tiled PDFs measure 8.5 × 11 in portrait and 11 × 8.5 in landscape.
- [ ] A4 tiled PDFs measure 210 × 297 mm portrait and 297 × 210 mm landscape.
- [ ] Print a calibration page at Actual size/100%, without Fit or Shrink.
- [ ] A known 2 in line measures 50.8 mm, or a known pattern dimension measures correctly.
- [x] Scale-check grids appear only where requested and remain absent from DXF/HPGL cutter exports.
- [ ] PDF page boundaries, overlap, grid, and joins are readable and align when assembled.

## 13. DXF validation

Repeat on jacket2 Fabric and Lining, then male_shirt and trousers. Use an external CAD reader only after local checks.
For ASTM, verify cut/seam/fold layers 1/14/6. Do not apply ASTM annotation-layer expectations to AAMA.
Pattern cm units must convert to physical mm for METRIC or inches for ENGLISH; a display-unit change alone proves neither.

- [x] Export DXF-ASTM R13, CLO3D preset, and DXF-AAMA R11 from supported tabs.
- [ ] Declared units and measured geometry agree; compare mm, cm, and inch display/export choices.
- [ ] Curves, corners, notches, grainlines, labels, and fold lines match the source pattern.
- [ ] Piece text matches source names, letters, quantities, materials, rotation, flip, tilt, and fold eligibility.
- [ ] Drill holes use ASTM layer 13 and group 30 diameter.
- [ ] Stripe/Plaid references use layers 9/10; hole positions and diameters remain correct in metric and English exports.
- [ ] Jacket2 Base Size export creates its `.rul` when pieces grade.
- [x] Individual Size exports have no `.rul`.
- [ ] Every expected jacket2 piece grades; explicitly check Lining Detail 4.
- [x] Record any turn-point warning and the first failing size. Detail 4 grading remains an unresolved check, not a presumed pass.
- [ ] Grading uses Seamly2D's actual sizes; no extra size is invented during import.
- [ ] Base Size `.dxf` and `.rul` import together correctly in the chosen reader.
- [ ] Record reader name, version, and unit selection.
- [ ] AAMA and ASTM retain their distinct format headers and piece text.
- [ ] CLO3D with Swap Cutting Line and Sewing Line OFF preserves cut/sew roles.
- [ ] If testing the swap option ON, confirm that only those roles reverse.
- [x] Save local reports before uploading anything. Upload pattern/export files only when you choose to share them.

## 14. CLI regression checks

Run these against the newly installed or built executables, not an older executable on PATH.
Use separate output folders and sufficient media for success cases.

Rebuild the applications after the launch-directory fix before testing the following relative-path cases.
Run from the repository root; repeat from a different directory using matching relative paths.

- [ ] Seamly2D `--test` resolves a relative `.sm2d` filename against the launch directory.
- [ ] Seamly2D resolves relative `--mfile` paths against the same launch directory.
- [ ] SeamlyMe `--test` resolves relative `.smis` and `.smms` filenames against the launch directory.
- [ ] Missing relative input files return 66 promptly with stderr, not a dialog.
- [ ] Relative layout settings and output paths resolve against the launch directory, not the log directory.
- [ ] Repeat with spaces in input and output paths; quoted arguments still work.

- [ ] Seamly2D `--test` loads each pattern with its listed measurement file.
- [ ] SeamlyMe `--test` loads both individual measurement files and the multisize measurement file.
- [ ] SeamlyMe rejects a `.sm2d` pattern with status 66 instead of showing a dialog.
- [ ] Invalid SeamlyMe size, height, or unit returns status 64 promptly with stderr.
- [ ] Seamly2D exports each pattern through `--layout-export` without opening either application window.
- [ ] Test all ten public formats: svg, pdf, pdf-tiled, png, jpg, jpeg, dxf, hpgl, ps, eps.
- [ ] Jacket2 accepts `--mfile`, `--gsize 40`, and `--gheight 146`; changed size/height inputs affect the exported geometry correctly.
- [ ] Seamly2D waits for the output to finish and returns the child's status.
- [ ] Missing inputs return 66; invalid formulas return 65; invalid options return 64.
- [ ] Insufficient media returns 70, names omitted pieces, and creates no partial output.
- [ ] A failed export leaves an existing destination file unchanged.
- [ ] Missing output folders and unavailable formats report explicit errors.
- [ ] G-Code/3MF return 69 when those modules are unavailable in the standard build.
- [ ] Redirected stdout/stderr remain readable.
- [ ] Check PowerShell `$LASTEXITCODE` immediately after each command; saving a file alone does not establish success.
- [ ] SeamlyLayout reads both saved SVG input and SVG stdin.
- [ ] PNG scale and SVG text-mode options change only the intended output properties.

Repeatable checks and exact commands are documented in [CLI_PARAMETERS.md](docs-cli/CLI_PARAMETERS.md).
Use `test_headless_export.py` for both individual and multisize-input patterns, with explicit layout settings.
Use `test_measurement_cli.py` and `test_pattern_input_status.py` for bounded failure checks.
When running source-tree Windows executables, use matching Qt DLLs and `QT_PLUGIN_PATH`.
Use an explicit large sheet for complete-export checks; test insufficient media separately.
Keep measurements and test patterns as copied inputs, since older-format conversion may update a test copy.

## 15. Record the remaining checks

- [ ] Summarize PASS/FAIL/BLOCKED by application and pattern.
- [ ] Attach failing exports, screenshots, piece names, and exact layout settings.
- [ ] Leave original patterns and measurements unchanged.
- [ ] Keep edited validation copies and exports for repeat tests.
- [ ] Do not mark the entire backlog complete while Detail 4 grading, the remaining derived-piece Flip matrix, or CAD checks remain unresolved.
- [ ] Exact Richmond source-pattern validation remains separate; these three patterns do not replace that missing fixture.

Reference: [existing user checklist](AGENT_CHECKLIST.md). This checklist uses current expectations: format 0.7.9, fitted annotation handoff, and unchanged label/grainline positions during Flip.
