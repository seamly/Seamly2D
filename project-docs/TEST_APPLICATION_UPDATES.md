# Application updates — user validation checklist

Use the latest Windows MSI built for these changes. Older installed applications do not contain this batch.

Mark each item PASS, FAIL, or BLOCKED. For failures, record the application, pattern, piece, settings, and export filename.
Save screenshots and export files beside the test results. Do not count a skipped test as passed.

## 1. Prepare the test files

Run from the repository root. Save edited patterns under new names; do not overwrite these originals.

| Pattern | Measurement file | Pattern units |
|---|---|---|
| `test-seamly-layout\patterns\jacket2_40-146.sm2d` | `test-seamly-layout\measurements\multisize\gost_man_ru.smms` | mm |
| `test-seamly-layout\patterns\male_shirt.sm2d` | `test-seamly-layout\measurements\individual\male_chest_102cm.smis` | cm |
| `test-seamly-layout\patterns\trousers.sm2d` | `test-seamly-layout\measurements\individual\trousers.smis` | cm |

- [ ] Close all three applications before installing the latest MSI.
- [ ] Start Seamly2D, SeamlyMe, and SeamlyLayout. Record each About version.
- [ ] Save test copies of all three patterns in a separate validation folder.
- [ ] Relink each copy to the measurement file listed above when prompted.
- [ ] Confirm jacket2 uses the listed file, not its old link outside this repository.
- [ ] Record the starting piece names, layout inclusion, quantities, materials, and hidden labels/grainlines.
- [ ] Use separate export folders for jacket2, male_shirt, and trousers.
- [ ] Keep unmodified copies for baseline comparisons and separate copies for marking, copy, and flip tests.

## 2. SeamlyMe — measurements and file pickers

- [ ] Open `male_chest_102cm.smis`. Measurements and units appear correctly.
- [ ] Open `trousers.smis`. Measurements and units appear correctly.
- [ ] Open `gost_man_ru.smms`. Its size and height ranges appear correctly.
- [ ] Save each measurement file under a test name. Reopen it without errors or lost measurements.
- [ ] Open Individual starts in the configured individual-measurement folder.
- [ ] Open Multisize starts in the configured multisize-measurement folder.
- [ ] Open Template starts in the configured templates folder.
- [ ] Repeat after visiting another folder. Each picker still starts in its own configured folder.
- [ ] With a separate test data root, missing picker folders are created and remain after cancelling the picker.
- [ ] A folder-creation failure shows an error instead of silently opening another folder.
- [ ] With a configured or renamed test data root, all three default folders follow that root.
- [ ] Templates remain under `<dataRoot>\templates`; no migration to `measurements\template` is expected.

Use a disposable data root for path tests. Do not rename or make the working measurement folders unwritable.

## 3. Seamly2D — conversion, save, and reopen

Repeat for jacket2, male_shirt, and trousers.

- [ ] The older pattern opens without a schema error.
- [ ] Draft and Piece Mode show the original geometry, names, materials, and layout inclusion.
- [ ] Save the test copy, close it, and reopen it without errors.
- [ ] The saved pattern uses format `0.7.9`.
- [ ] Every draft block has `groups` followed by `images`, including blocks without images.
- [ ] Measurement links, custom formulas, piece labels, seam allowances, and fold lines survive reopening.
- [ ] Existing hidden labels/grainlines remain hidden; do not treat intentionally hidden objects as missing.

## 4. Seamly2D — label template filenames

Use male_shirt. Repeat the pattern-label checks on jacket2.

- [ ] Piece Properties > Labels > Piece Label displays the template filename beside Edit template.
- [ ] Piece Properties > Labels > Pattern Label displays the corresponding filename.
- [ ] Pattern Preferences > Label data displays the pattern-label template filename.
- [ ] Hovering over a filename shows its full path.
- [ ] Selecting another template updates the displayed filename when the editor closes.
- [ ] Cancelling template selection preserves the current label and filename.
- [ ] Matching default template lines display the default filename.
- [ ] Custom stored lines without known file provenance display `none`, not an unrelated filename.
- [ ] Template edits still change the rendered label correctly.

## 5. Seamly2D — Flip and placement properties

Use an asymmetric male_shirt piece, such as FrontPanel.

- [ ] Flip choices are None, Horizontal, Vertical, and Both in Piece Properties and the context menu.
- [ ] The context menu marks the current choice.
- [ ] Horizontal mirrors the displayed outline left to right.
- [ ] Vertical mirrors the displayed outline top to bottom.
- [ ] Both applies both directions. None restores the unflipped display.
- [ ] Seam line, cut line, notches, fold line, and added markings follow the displayed flip.
- [ ] Labels and grainline retain their position and readability.
- [ ] Undo and redo restore the displayed flip and its selected menu choice.
- [ ] Save and reopen preserve the choice and display.
- [ ] Repeat Horizontal, Vertical, and Both on copied, united, and Mirror-on-Fold pieces.
- [ ] Change a previously flipped piece to each other choice; the result does not accumulate extra flips.
- [ ] Rotation choices and Tilt direction/degrees still save and reopen correctly.
- [ ] Tilt None disables degrees; enabled degrees remain within 0–90.

The flip changes the displayed choice. It must not flatten or rewrite the stored draft geometry.

## 6. Seamly2D — independent Copy Piece

Use male_shirt, then a jacket2 piece.

- [ ] Right-click > Copy Piece creates a separate piece beside the source.
- [ ] The copy has a distinct suffixed name and letter when a letter exists.
- [ ] Copying the copy creates another distinct name.
- [ ] The copy retains paths, seam allowance, fold line, labels, grainline, markings, and placement properties.
- [ ] The copy opens in Pattern Piece Tool and remains editable.
- [ ] Editing the copy does not change the source piece.
- [ ] Editing source-only piece settings does not change the copy.
- [ ] Moving a shared draft point updates both pieces correctly.
- [ ] Undo removes the complete copy in one step; redo restores it.
- [ ] Save and reopen retain both independent pieces.
- [ ] Repeat with a united piece, a horizontally flipped piece, and a vertically flipped piece.
- [ ] Repeat with a Mirror-on-Fold piece and a copy of a copy.
- [ ] In a disposable copy, remove the source piece. The copied piece still recalculates and exports.

## 7. Seamly2D — drill holes and stripe/plaid references

Use a male_shirt test copy and jacket2 Back. Add suitable live anchors if the piece has none.

- [ ] Piece Properties has a dedicated Drill holes page, separate from notch settings.
- [ ] Add a hole at a live center anchor with a known positive diameter.
- [ ] Record diameter and pattern units: `_____` in `_____`.
- [ ] Zero, negative, nonnumeric, and missing diameters show an error and cannot be added.
- [ ] A missing or removed center anchor shows an error.
- [ ] Add one Stripe reference and one Plaid reference between distinct live anchors.
- [ ] Missing endpoints or coincident endpoints show an error.
- [ ] Remove selected markings; only the selected marking disappears.
- [ ] Save and reopen preserve all valid markings and their types.
- [ ] Piece Mode displays the markings at their anchor positions.
- [ ] Moving a draft anchor moves its hole center or reference endpoint.
- [ ] Flip, Copy Piece, Union, and Mirror-on-Fold retain and transform the markings correctly.
- [ ] Jacket2 size changes move hole centers with the graded geometry; diameter stays constant.
- [ ] SeamlyLayout imports the same marking counts, positions, and physical diameters.
- [ ] Layout adjustment moves the markings with their piece.
- [ ] References do not enlarge packing bounds or prevent otherwise valid placement.

## 8. Fold lines and built-in seam allowance

Check jacket2 Back, male_shirt BackPanel, and CollarTopInterface-Built-in-SA.

- [ ] Piece Mode shows both seam line and cut line where enabled.
- [ ] Neither line includes an edge along the fold line.
- [ ] Each line starts at the fold line end and ends at the fold line start.
- [ ] The fold line, arrow line, and label remain visible where enabled.
- [ ] Built-in seam line lies inside the cut line by the configured allowance.
- [ ] CollarTopInterface-Built-in-SA has no seam-line spike at D11 or D12.
- [ ] Hide Seam Line removes only that line; the cut line remains.
- [ ] Marking two neighbouring fold-line nodes checks and locks On fold.
- [ ] Removing the fold line releases and unchecks On fold when there was no independent user choice.
- [ ] A third fold-line node is rejected.
- [ ] Mirror-on-Fold creates a full symmetric piece without a fold line.
- [ ] The mirrored source is excluded from the layout as intended.
- [ ] SeamlyLayout, SVG, PDF, and DXF preserve the seam/cut/fold relationships.
- [ ] A piece without a valid fold line exports `Fold:N`, including FrontPanel and mirrored full pieces.
- [ ] A correctly defined on-fold piece exports `Fold:Y`.
- [ ] DXF fold-line export does not warn that the fold line fails to connect with the cut line.
- [ ] DXF cut/seam layers are 1/14; the mirror line is layer 6.
- [ ] Check varying-width notch depths and built-in seam allowance notches against the configured allowance.

## 9. Jacket2 — multisize handoff and layouts

Start from the baseline jacket2 copy. Use Paper > Roll, width 48 in, for the first layout.

- [ ] Layout Mode prepares the size range and opens SeamlyLayout in front.
- [ ] Repeating Layout Mode brings the existing window forward rather than creating a duplicate.
- [ ] Fabric and Lining material tabs appear over both canvases.
- [ ] Each material holds only its assigned pieces.
- [ ] Marker, Nested, Base Size, and individual Size tabs match the pattern's actual size range.
- [ ] Record the displayed base size: `_____`. Use that value in subsequent grading checks.
- [ ] Marker contains every included piece at every requested size.
- [ ] Nested stacks the sizes of each piece rather than scattering them.
- [ ] Grainlines and label anchors stack consistently and remain inside each size's cut line.
- [ ] Jacket2 Back's piece label fits inside Size 220 without an import warning.
- [ ] Lining Detail 4's annotations also stack correctly.
- [ ] Nested colors differ visibly; legend squares match their sizes.
- [ ] Exported Nested SVG/PDF retains the colors, legend, and intended single annotation set.
- [ ] Switching materials preserves each material's selected size tab and layout.
- [ ] Adjusting a piece moves its lines, labels, and markings together; saved adjustments persist.
- [ ] Material controls are locked while layout generation or adjustment requires it.
- [ ] A Marker roll may grow beyond 500 in; canvas, adjustment frame, and SVG height agree.
- [ ] No included size silently disappears. Any unplaced piece is named explicitly.

## 10. Male_shirt — individual handoff and materials

- [ ] All included pieces arrive in SeamlyLayout; intentionally excluded pieces do not.
- [ ] Names, letters, quantities, materials, hidden objects, and placement properties match Seamly2D.
- [ ] With sufficient media, every included piece is placed.
- [ ] Assign one test piece to Lining. Fabric and Lining layouts contain the correct separate pieces.
- [ ] Save/reopen and repeated Layout Mode retain the material assignments.
- [ ] Grainline/label placement, fold lines, built-in allowance, copied pieces, and markings match the edited test pattern.
- [ ] Export filenames identify the pattern, material, tab, and format where applicable.

## 11. Trousers — oversized pieces and fabric

Start with Paper > Sheet, 36 × 36 in. Keep a baseline copy without added markings.

- [ ] Oversized-piece dialog names the affected legs, including Back and the applicable Front variants.
- [ ] Heading, descriptions, list, and action labels are readable.
- [ ] Cancelling leaves the application responsive and allows Layout Settings to be changed.
- [ ] Split the pieces creates labelled parts with matching join identifiers and match notches.
- [ ] Every part is accounted for; any unplaced part is named in the warning.
- [ ] SVG/PDF/DXF exports show the same split parts and joins.
- [ ] Make the media bigger retains whole pieces and enlarges the frame sufficiently.
- [ ] A successful enlarged layout has no missing pieces or unexplained warning.
- [ ] Repeat on a narrow paper roll; canvas and exported dimensions reflect any enlargement.
- [ ] Set Fabric width 36 in, Folded, giving 18 in working width.
- [ ] Fabric starts with a 500 in length and trims to the completed layout.
- [ ] Folded fabric has zero left margin.
- [ ] Every included trousers piece fits within the usable width when the chosen settings permit placement.
- [ ] If pieces cannot fit, the warning names them and suggests fabric width/length, not a larger paper sheet.
- [ ] Repeat unfolded fabric and paper settings; neither keeps incorrect folded-fabric margins.
- [ ] Turn Place fold lines on the left edge on and off; fold placement follows the selection.

## 12. Export menu and physical scale

Use jacket2 for tab restrictions and male_shirt/trousers for individual layouts.

- [ ] Export has one DXF submenu with readable, untruncated text.
- [ ] Marker/Nested show the note directing DXF export to Base Size or a size tab.
- [ ] DXF items are disabled on unsupported tabs.
- [ ] No obsolete DXF-ASTM R12 export remains.
- [ ] Marker PDF (Tiled) is disabled for single-tab export.
- [ ] Export all tabs omits unsupported Marker/Base Size tiled-PDF outputs without freezing.
- [ ] Export SVG using each offered text mode; labels, geometry, and grouping are correct.
- [ ] Export PDF, tiled PDF, PNG, JPG, PS, EPS, and HPGL where available.
- [ ] Each file opens in its appropriate viewer and contains the expected pieces.
- [ ] Export all tabs uses distinct material/tab filenames without overwriting unrelated outputs.
- [ ] Nested non-DXF exports show the intended smallest-size grainline/label set per piece.
- [ ] PNG/JPG print resolution and dimensions match the selected settings.
- [ ] Letter tiled PDFs measure 8.5 × 11 in portrait and 11 × 8.5 in landscape.
- [ ] A4 tiled PDFs measure 210 × 297 mm portrait and 297 × 210 mm landscape.
- [ ] Print a calibration page at Actual size/100%, without Fit or Shrink.
- [ ] A known 2 in line measures 50.8 mm, or a known pattern dimension measures correctly.
- [ ] Scale-check grids appear only where requested and remain absent from DXF/HPGL cutter exports.
- [ ] PDF page boundaries, overlap, grid, and joins are readable and align when assembled.

## 13. DXF validation

Repeat on jacket2 Fabric and Lining, then male_shirt and trousers. Use an external CAD reader only after local checks.

- [ ] Export DXF-ASTM R13, CLO3D preset, and DXF-AAMA R11 from supported tabs.
- [ ] Declared units and measured geometry agree; compare mm, cm, and inch display/export choices.
- [ ] Curves, corners, notches, grainlines, labels, and fold lines match the source pattern.
- [ ] Piece text matches source names, letters, quantities, materials, rotation, flip, tilt, and fold eligibility.
- [ ] Drill holes use ASTM layer 13 and group 30 diameter.
- [ ] Stripe/Plaid references use layers 9/10; hole positions and diameters remain correct in metric and English exports.
- [ ] Jacket2 Base Size export creates its `.rul` when pieces grade.
- [ ] Individual Size exports have no `.rul`.
- [ ] Every expected jacket2 piece grades; explicitly check Lining Detail 4.
- [ ] Record any turn-point warning and the first failing size. Detail 4 grading remains an unresolved check, not a presumed pass.
- [ ] Grading uses Seamly2D's actual sizes; no extra size is invented during import.
- [ ] Base Size `.dxf` and `.rul` import together correctly in the chosen reader.
- [ ] Record reader name, version, and unit selection.
- [ ] AAMA and ASTM retain their distinct format headers and piece text.
- [ ] CLO3D with Swap Cutting Line and Sewing Line OFF preserves cut/sew roles.
- [ ] If testing the swap option ON, confirm that only those roles reverse.
- [ ] Save local reports before uploading anything. Upload pattern/export files only when you choose to share them.

## 14. CLI regression checks

Run these against the newly installed or built executables, not an older executable on PATH.
Use separate output folders and sufficient media for success cases.

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
- [ ] SeamlyLayout reads both saved SVG input and SVG stdin.
- [ ] PNG scale and SVG text-mode options change only the intended output properties.

Repeatable checks and exact commands are documented in [CLI_PARAMETERS.md](docs-cli/CLI_PARAMETERS.md).
Use `test_headless_export.py` for both individual and multisize-input patterns, with explicit layout settings.
Use `test_measurement_cli.py` and `test_pattern_input_status.py` for bounded failure checks.
When running source-tree Windows executables, use matching Qt DLLs and `QT_PLUGIN_PATH`.

## 15. Record the remaining checks

- [ ] Summarize PASS/FAIL/BLOCKED by application and pattern.
- [ ] Attach failing exports, screenshots, piece names, and exact layout settings.
- [ ] Leave original patterns and measurements unchanged.
- [ ] Keep edited validation copies and exports for repeat tests.
- [ ] Do not mark the entire backlog complete while Detail 4 grading, flipped-copy checks, or CAD checks remain unresolved.
- [ ] Exact Richmond source-pattern validation remains separate; these three patterns do not replace that missing fixture.

Reference: [existing user checklist](AGENT_CHECKLIST.md). This checklist uses current expectations: format 0.7.9, fitted annotation handoff, and unchanged label/grainline positions during Flip.
