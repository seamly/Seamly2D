# Application updates — user validation checklist

Use the latest Windows MSI built for these changes. Older installed applications do not contain this batch.

Mark each item PASS, FAIL, or BLOCKED. For failures, record the application, pattern, piece, settings, and export filename.
Save screenshots and export files beside the test results. Do not count a skipped test as passed.

Checked boxes preserve the user's recorded results; this review does not certify new tests.
An unchecked box means pending unless a FAIL or BLOCKED note states otherwise.
For repeated checks, record which patterns and materials were tested before checking the box.

--> 'Detail 4' has been renamed, there is no 'Detail 4' piece anymore.

## Review status and next checks

The Windows x64 MSI `26.11.718` built successfully. `nmake check`, `ctest --preset release` (8/8),
`cargo test --workspace`, and MSI validation passed. Rebuilt executables passed the section 14
pattern-input checks (25/25), measurement CLI checks (16/16), and headless export checks.
Install this MSI before completing the remaining GUI, CAD-reader, and user-pattern checks.

| Priority | Remaining validation | Sections |
|---|---|---|
| 1 | Install MSI `26.11.718`; complete the remaining GUI and user-pattern checks. | 2–14 |
| 2 | Complete oversized-piece splitting, cancellation, and enlargement checks. | 11 |
| 3 | Complete Stripe/Plaid input errors, missing anchors, and graded drill diameter checks. | 7 |
| 4 | Complete remaining DXF unit variants, source comparisons, grading, and external CAD import. | 8, 13 |
| 5 | Verify printed scale, page assembly, and image resolution. | 12 |
| 6 | Complete relocated-data-root, template provenance, derived-piece Flip, and adjustment checks. | 2, 4, 5, 9 |

### Missing result details

Some recorded checks still have blank evidence fields. Keep their checked boxes, but complete these details:

- [x] Record tested About versions: Seamly2D `26.11.723_____`; SeamlyMe `26.11.723_____`; SeamlyLayout `26.11.723_____`.
- [x] Add an 'About' section to SeamlyLayout (button on the right side of top menu, next to the preferences icon).
- [x] Record drill diameter and pattern units from section 7: `2 cm` in `male_shirt` (`cm` units).
- [x] Record jacket2's displayed Base Size from section 9: `500`.
- [x] Record the turn-point warning, affected pieces, size comparison, and export file from section 13.

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
- [x] Record the starting piece names, layout inclusion, quantities, materials, and hidden labels/grainlines.
- [x] Use separate export folders for jacket2, male_shirt, and trousers.
- [x] Keep unmodified copies for baseline comparisons and separate copies for marking, copy, and flip tests.

## 2. SeamlyMe — measurements and file pickers

- [x] Open `male_chest_102cm.smis`. Measurements and units appear correctly.
- [x] Open `trousers.smis`. Measurements and units appear correctly.
- [x] Open `gost_man_ru.smms`. Its size and height ranges appear correctly.
- [x] Save each measurement file under a test name. Reopen it without errors or lost measurements.
- [x] Open Individual starts in the configured individual-measurement folder.
- [x] Open Multisize starts in the configured multisize-measurement folder.
- [x] Open Template starts in the configured templates folder.
- [x] Repeat after visiting another folder. Each picker still starts in its own configured folder.
- [x] With a separate test data root, missing picker folders are created and remain after cancelling the picker.
   - [x] Select a different disposable data root; accept the update prompt once and decline it on a second run.
   - Regression coverage now checks all nine default folders and confirms customized folders stay unchanged. Keep the GUI result pending until the rebuilt installer is tested.
- [x] A folder-creation failure shows an error instead of silently opening another folder.
  - Use a disposable data root where a required child folder cannot be created. Confirm an explicit error and that the prior picker path remains selected.
- [x] With a configured or renamed test data root, all three default folders follow that root.
  - Verify Individual, Multisize, and Templates point to the three default folders under the new root.
--> Seamly2D > File > Open opened to the prior data root's pattern directory.
-->SeamlyMe > File > Open Individual opened previous directory
-->SeamlyMe > File > Open Multisize opened previous directory
-->SeamlyMe > File > Open Template opened to the previous directory
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
- [x] Custom stored lines without known file provenance display `none`, not an unrelated filename.
  - [x] Edit stored label lines without selecting a template file, then save and reopen the pattern. Confirm the filename displays `none`
  - [x] Adding a new line to a label displays "\<empty>" on the new line. Then selecting a label variable from the 'Insert' dropdown list displays the variable and removes "\<empty>".
- [x] Template edits still change the rendered label correctly.
- [x] `%pFileName%` and `%mFileName%` include extensions in rendered labels and editor previews; merged from the feature worktree. Targeted Qt tests pass.
- [x] Verify filename extensions in installed-app pattern labels, piece labels, and editor previews. Remove appended `%pExt%` or `%mExt%` from existing templates.

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
- [x] Repeat Horizontal, Vertical, and Both on copied, united, and Mirror-on-Fold pieces.
  - [x]  Piece Mode now applies the display transform to path-node markers and internal paths. Verify all derived-piece cases with the rebuilt installer.
- [x] Change a previously flipped piece to each other choice; the result does not accumulate extra flips.
- [x] Rotation choices save and reopen correctly.
- [x] Create a plan to enable Piece Properties Rotation in Layout Mode "Any".
  - Implementation is tracked in [TODO_ROTATION.md](TODO_ROTATION.md); installed-app verification of Layout Mode "Any" free rotation remains pending.
- [x] Tilt None disables degrees; enabled degrees remain within 0–90.
  - [x] Tilt with degress > 0 should visually rotate the pattern piece but not the grainline.
  - [x] Tilt rotates the piece and rotates all of the points. Piece Mode now applies the same display transform to path-node markers.
  - [x] Tilt should rotate internal paths. Fix this on a worktree as a feature branch then merge into run-seamlyLayout.

The flip changes the displayed choice. It must not flatten or rewrite the stored draft geometry.
Section 6 records copied-source checks as passed. Section 5's unchecked row covers Horizontal, Vertical, and Both on copied, united, and Mirror-on-Fold pieces.

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

Anchor points are required to define drill holes and reference lines, so correction operations for anchor point add/edit/delete actions are essential; issues related to anchor points are included in this section.

- [x] Piece Properties has a dedicated Drill holes page, separate from notch settings.
- [x] Add a Drill hole at an anchor point with a known positive diameter.
- [x] Record diameter and pattern units: `2 cm` in `male_shirt` (`cm` units).
- [x] Zero, negative, nonnumeric, and missing diameters show an error and cannot be added
- [x] Adding an Anchor point adds an anchor point to a piece and does not generate an error:
- [x] Adding an Anchor point causes an 'Undo anchor point' icon to appear in the menu bar.
- [x] Selecting 'Undo anchor point' icon reverses the anchor point action.
- [ ] Installed-app GUI: after Undo anchor point, Redo anchor point is visible and enabled.
  - Anchor creation now groups both model changes into one undo operation. The MainWindow regression covers three undo/redo cycles. The reported missing-action symptom also appeared on baseline, so installed-MSI confirmation remains open.
- [x] A Drill hole with a missing or removed center anchor displays an error.
  - [x] Piece Mode: Trying to delete an anchor point that is in use will remove the anchor from the list but 'OK' and 'Apply' buttons do nothing so the deletion can't be saved. Add a display a message that the anchor point is in use and list the items where it is used, the user can click 'Close' button on this popup message to close it. Fix this in a worktree as a feature branch then merge into run-seamlyLayout.
  - [x] Draft Mode: Adding an anchor point to a piece when that anchor point has already been added generates a 'Bad Id' message. It should generate a popup message that the anchor point has already been added, the user can click 'Close' to close the popup message.
- [ ] Installed-app GUI: add Stripe and Plaid references using path-node endpoints, not anchors.
  - The path-node behavior already existed. Regression coverage verifies node-only choices, dialog refresh, and serialization.
- [x] Missing endpoints or coincident endpoints show an error.
  - Try to use the same path node for both endpoints. For a missing endpoint, use a disposable pattern whose reference points to a deleted node.
- [x] Remove selected markings; only the selected marking disappears.
- [x] Save and reopen preserve all valid markings and their types.
- [x] Piece Mode displays the markings at their anchor positions.
- [x] Moving a draft anchor moves its hole center or reference endpoint.
- [x] Flip, Copy Piece, Union, and Mirror-on-Fold retain and transform the markings correctly.
- [x] SeamlyLayout Nested: Jacket2 size changes move hole centers with the graded geometry; diameter stays constant.
  - [x] Drill holes for multisize pieces should be processed like grainlines so that each exists only once, and is grouped with grainlin and labels. Fix this in a worktree as a feature branch and merge into run-seamlyLayout.
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
- [x] Check varying-width notch depths and built-in seam allowance notches against the configured allowance.


## 9. Jacket2 — multisize handoff and layouts

Start from the baseline jacket2 copy. Use Paper > Roll, width 48 in, for the first layout.

- [x] Layout Mode prepares the size range and opens SeamlyLayout in front.
- [x] Repeating Layout Mode brings the existing window forward rather than creating a duplicate.
- [x] Fabric and Lining material tabs appear over both canvases.
- [x] Each material holds only its assigned pieces.
- [x] Marker, Nested, Base Size, and individual Size tabs match the pattern's actual size range.
- [x] Record the displayed base size: `500`. Use that value in subsequent grading checks.
- [x] Marker contains every included piece at every requested size.
- [x] Nested stacks the sizes of each piece rather than scattering them.
- [x] Grainlines and label anchors stack consistently and remain inside each size's cut line.
- [x] Jacket2 Back's piece label fits inside Size 220 without an import warning.
- [x] Lining Detail 4's annotations also stack correctly.
- [x] Nested colors differ visibly; legend squares match their sizes.
- [x] Exported Nested SVG/PDF retains the colors, legend, and intended single annotation set.
- [x] Switching materials preserves each material's selected size tab and layout.
- [x] Adjusting a piece moves its lines, labels, and markings together; saved adjustments persist.
- [x] Material controls are locked while layout generation or adjustment requires it.
  - Start layout generation and confirm material controls stay disabled until it finishes. In Adjust Mode, move a piece, save, switch materials, and reopen the layout to confirm the adjustment and its lines, labels, and markings persist.
- [x] A Marker roll may grow beyond 500 in; canvas, adjustment frame, and SVG height agree.
- [x] No included size silently disappears. Any unplaced piece is named explicitly.

## 10. Male_shirt — individual handoff and materials

- [x] All included pieces arrive in SeamlyLayout; intentionally excluded pieces do not.
- [x] Names, letters, quantities, materials, hidden objects, and placement properties match Seamly2D.
- [x] With sufficient media, every included piece is placed.
- [x] Assign at least one test piece to each material: Fabric, Lining, Interfacing, and Interlining.
- [x] Confirm each material layout contains only its assigned pieces.
- [x] Save/reopen and repeated Layout Mode retain the material assignments.
- [x] Grainline/label placement, fold lines, built-in allowance, copied pieces, and markings match the edited test pattern.
- [x] Export filenames identify the pattern, material, tab, and format where applicable.

## 11. Trousers — oversized pieces and fabric

Start with Paper > Sheet. Use a custom 36 × 36 in sheet when available. Earlier testing used Arch E (36 × 48 in), the closest available option. Keep a baseline copy without added markings.
Regression results: SettingsModel/QML 40, `layout_tiling` 40, paper-layout 6, `cxxqt_bridge` 278, and ExportMenu 20. Installed-MSI GUI validation remains pending.
- [x] Oversized-pieces:
  - [x]  dialog names the affected legs, including Back and the applicable Front variants.
  - [x] Heading, descriptions, list, and action labels are readable.
  - [x] 'Cancel' leaves the application responsive and allows Layout Settings to be changed.
  - [x] In the "Pieces do not fit the media" dialog, choose Cancel. Confirm the dialog closes, the application remains responsive, and Layout Settings can be opened.
  - [x] 'Split the pieces' creates labelled parts with matching join identifiers and match notches.
- [ ] Installed-app GUI: Tile Size lists all predefined Sheet sizes and displays dimensions.
  - SettingsModel/QML tests: 40 passed. `layout_tiling` tests: 40 passed. Rebuilt-MSI verification remains open.
- [ ] Installed-app GUI: paper sheets reject overflow and direct the user to PDF (tiled), without creating multiple sheets.
  - Paper-layout regression tests: 6 passed. Full `cxxqt_bridge` tests: 278 passed. Rebuilt-MSI verification remains open.
- [x] Paper > Sheets > Custom implemented
- [x] Paper > Sheets > 'Split the pieces'  - works in that it splits the pieces.
- [x] Paper > Sheets > 'Split the pieces' - Disable the main DXF menu option
- [ ] Installed-app GUI: splitting paper pieces preserves the selected media dimensions.
  - Paper-layout regression tests cover split dimensions, margins, and join metadata. Rebuilt-MSI verification remains open.
- [ ] Installed-app GUI: split paper layouts enable PDF, PNG, JPG, EPS, PostScript (PS), SVG, and HPGL.
  - `ExportMenuTests`: 20 cases passed. Rebuilt-MSI verification remains open.
- [x] Paper > Sheets > 'Make the media bigger' - works in that it fits all pieces onto the expanded media.
- [x] Paper > Sheets > 'Make the media bigger' - Enable all export options
--> Fix this in a worktree as a feature branch then merge into run-seamlyLayout.

- [x] Fabric (folded) > 'Split the pieces' - works in that it splits the pieces.
- [x] Fabric (folded) > 'Split the pieces' - Disable DXF export options
- [x] Fabric (folded) > 'Split the pieces' - Enable PDF, PNG, JPG, EPS, PostScript(PS), SVG, and HPGL export menu options
- [x] Fabric (folded) > 'Make the media bigger' - works in that it lengthens the fabric but does not widen the fabric.
- [x] Fabric (folded) > 'Make the media bigger' - Enable all export options

- [x] Every part is accounted for; any unplaced part is named in the warning.
- [x] The fabric warning reports the offending piece dimension and effective folded-fabric dimension.
- [x] SVG/PDF/DXF exports show the same split parts and joins.
- [x] Make the media bigger retains whole pieces and enlarges the frame sufficiently on 32 in folded fabric.
- [x] A successful enlarged media layout has no missing pieces or unexplained warning.
  - [x] display a popup message wth the new media dimensions
  - [x]  if media=fabric then do not enlarge the width, only enlarge the height
- [x] Paper sheets show the oversized-piece warning when included pieces do not fit. Reported warning gap; verify after the fix.
- [x] Repeat on a narrow paper roll; canvas and exported dimensions reflect any enlargement.
- [x] Set Fabric width 36 in, Folded, giving 18 in working width.
- [x] Fabric starts with a 500 in length and trims to the completed layout.
- [x] Folded fabric has zero left margin.
- [x] Every included trousers piece fits within the usable width when the chosen settings permit placement.
- [x] On fabric rolls, the warning names unplaced pieces and suggests fabric width/length, not a larger paper sheet.
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
- [x] Each exported file opens in its appropriate viewer and contains the expected pieces.
  - [x] View > PDF opens the selected PDF. Reported failure: no PDF opened.
  -->View > PDF needs a default value that will open the file in the system default browser
  - [x] The PNG viewer opens exported PNG files. Reported failure: the PNG viewer does not work.
    - [x] View > PNG needs a default value that will open the file in the system default browser
- [x] Export all tabs uses distinct material/tab filenames without overwriting unrelated outputs.
- [x] Nested non-DXF exports show the intended smallest-size grainline/label set per piece.
- [x] PNG/JPG print resolution and dimensions match the selected settings. Check the export files' details.
--> there are no selected settings. All exports are 96 dpi with 32 bit color depth.

## 13. DXF validation

Repeat on jacket2 Fabric and Lining, then male_shirt and trousers. Use an external CAD reader only after local checks.
For ASTM, verify cut/seam/fold layers 1/14/6. Do not apply ASTM annotation-layer expectations to AAMA.
Pattern cm units must convert to physical mm for METRIC or inches for ENGLISH; a display-unit change alone proves neither.
--> Use the following files to check the items below:
  - AAMA:
    - "<layout-output>\jacket2_40-146_AAMA_Fabric_BaseSize500_202611081029.dxf"
    - "<layout-output>\jacket2_40-146_AAMA_Fabric_BaseSize500_202611081029.rul"
    - "<layout-output>\jacket2_40-146_AAMA_Fabric_BaseSize500_202611081029-dxf-report.pdf"
  - ASTM:
    - "<layout-output>\jacket2_40-146_R13_Fabric_BaseSize500_202611081029.dxf"
    - "<layout-output>\jacket2_40-146_R13_Fabric_BaseSize500_202611081029.rul"
    - "<layout-output>\jacket2_40-146_R13_Fabric_BaseSize500_202611081029-dxf-report.pdf"
  - CLO3D
    - "<layout-output>\jacket2_40-146_CLO3D_Fabric_BaseSize500_202611081029.dxf"
    - "<layout-output>\jacket2_40-146_CLO3D_Fabric_BaseSize500_202611081029.rul"
    - "<layout-output>\jacket2_40-146_CLO3D_Fabric_BaseSize500_202611081029-dxf-report.pdf"
- [x] Export DXF-ASTM R13, CLO3D preset, and DXF-AAMA R11 from supported tabs.
- [x] The three supplied DXF reports declare METRIC; the largest piece is 312.3 x 638.6 mm and is reported consistent with millimeters.
- [x] Piece names, letters, quantities, materials, fold eligibility, and neutral transforms match the available source pattern.
- [x] All three reports identify layer 13 as Drill holes; each DXF has one layer-13 POINT with group 30/Z = 4.0.
- [x] The source Back drill hole is 4 mm; all three DXFs encode group 30/Z = 4.0.
- [ ] Confirm Stripe/Plaid references use layers 9/10.
- [x] Jacket2 Base Size export creates its `.rul` when pieces grade.
- [x] Individual Size exports have no `.rul`.
- [x] Every expected jacket2 piece grades
- [x] Record any turn-point warning and the affected pieces: none appear in the supplied DXF reports.
  - [x] In SeamlyLayout, when exporting DXF, no turn point warnings occurred
  - [x] In the supplied DXF reports, no turn-point warnings exist.
- [x] AAMA report identifies layer 1 as Boundary / cut line, layer 2 as Turn points, and layer 3 as Curve points.
- [x] Grading uses Seamly2D's actual sizes; no extra size is invented during import.
- [x] Base Size `.dxf` and `.rul` import together correctly in the chosen reader.
- [x] In AAMA and ASTM reports:
  - [x] Reports identify distinct formats: ANSI/AAMA-292 and ASTM D6673-10; both include six piece-name labels.
  - [x] Both reports identify cut/boundary layer 1, sew layer 14, and mirror/fold layer 6.
- [x] CLO3D report identifies cut/boundary layer 1 and sew layer 14.
- [ ] Import CLO3D dxf file into CLO3D,
  - [ ] with Swap Cutting Line and Sewing Line OFF - confirm this preserves cut/sew roles.
  - [ ] with "Swap Cutting Line and Sewing Line" ON -
    - [ ] confirm that only cut/sew roles reverse.
    - [ ] confirm cut moves to layer 1
    - [ ] confirm sew moves to layer 1
    - [ ] confirm geometry and other layer assignments stay unchanged.
- [x] Confirm that the supplied DXF reports contain no turn-point warnings.

Current supplied-file review (reports dated 08 November 2026; producer Seamly2D 26.11.723):
- AAMA report detects ANSI/AAMA-292 with high confidence and reports 0 departures, 1 extension, and 0 advisories. The extension is the optional `Material` text identifier.
- R13 and CLO3D reports detect ASTM D6673-10 and each report 0 departures, 0 extensions, and 3 advisories. The advisories concern validation curves and point densification, not turn-point warnings.
- All three reports flag an L5 endpoint mismatch up to 227.990; the sizes are not a pure translation. Compare this with the checked piece-grading results; the reports do not name a piece as ungraded.
- All reports list six piece blocks and the same six `Piece Name` values. ASTM and CLO3D additionally encode `Size Name`, `Quantity`, `Fold`, and `Material`.
- Reports list layers 1, 6, 7, 13, and 14 for all three formats. ASTM/CLO3D also list layers 15, 84, and 87. No report lists stripe/plaid layers 9/10.
- The available validation source pattern matches the six exported Fabric piece names and letters: Back/A, Upper Front/B, Middle Front/C, Front Sleeve/F, Back Sleeve/E, and Collar/H. Each has quantity 1 and material Fabric. Source `onFold` is true only for Back and Upper Front; ASTM/CLO3D encode `Fold:Y` for those two and `Fold:N` for the other four. All six source pieces have neutral rotation, flip, and tilt; each DXF block insertion has rotation 0 and unit scale.
- Source piece paths include curves and point/arc nodes; each DXF contains boundary/sew polylines, turn/curve points, and one grainline per piece. ASTM/CLO3D encode `Fold:Y` for Back and Upper Front; AAMA places layer-6 fold lines on those same pieces. This confirms feature presence, not exact curve, corner, notch, or marking-coordinate equivalence.
- The AAMA, R13, and CLO3D `.rul` tables each list the same 26 sizes (220–720) and 167 rules, with identical growth pairs for every size. Each rule has 26 coordinate pairs; all pairs are zero at sample size 500, and 166 rules change at each of the other 25 sizes.
- The R13 Base Size DXF labels every rule ID from 1 through 167 on its boundary and sew-line key points. It has 92 labels on each layer, including repeated IDs at shared points. This verifies grade-rule coverage for the exported boundary/sew key points, not the rendered contours at each size.
- The supplied source and Base Size DXFs contain no notch entities on layer 4 or ASTM notch layers 80–83. Notch geometry therefore could not be compared across sizes. The `.rul` tables do not provide per-size DXF renderings for grainlines, fold lines, drill holes, or other markings.
- The source Back hole uses a 4 mm diameter. Its center is anchor 335 on midpoint construction point 334 between points 38 and 36. Each DXF places its layer-13 point at (130.510, 536.090) with Z/group 30 = 4.0; the source point's evaluated size-500 coordinates were not available for a direct position comparison.
- The source Stripe and Plaid references belong to Lower Front, whose material is Lining. The supplied exports are Fabric exports and do not contain layers 9 or 10; verify those references with corresponding Lining exports.
- The available `test-seamly-layout\patterns\validation\jacket2_40-146.sm2d` was used for source metadata. The referenced `.sm2d.autosave` file is absent, so it could not be used for exact geometry or warning-reproduction checks.
- None of the three supplied PDF reports contains a turn-point warning. Their “Turn points” text labels describe DXF layer 2. The L5 endpoint-mismatch notice is separate and is not reported as a turn-point warning.
- Raw DXF headers leave `$INSUNITS` and `$MEASUREMENT` unset; reports still identify METRIC and find the geometry consistent with millimeters.
- Raw AAMA DXF header is AC1009. Raw R13 and CLO3D headers are AC1015, but their reports state AC1012; resolve this version discrepancy.
- The `.rul` files for all three exports were compared across all 26 sizes and their 167 grade rules; visual source-pattern geometry remains unverified beyond Base Size.
- CLO3D import behavior, cm/inch exports, exact source-geometry matching, evaluated hole position, and Stripe/Plaid layer assignments remain unverified.

Earlier report results (generated by Seamly2D 26.11.614; compliance counts are superseded by the current review above):
- Earlier reports identified the intended formats and declared METRIC units, with geometry consistent with millimeters.
- The L5 endpoint mismatch up to 227.990 also appears in the current reports.
- The CLO3D and R13 reports each show zero departures, zero extensions, and four advisories for validation curves and excess curve points.
- The AAMA report showed one departure for declared points off the boundary, one extension for its allowed `Material` text, and one advisory.
- The exporter does not invent grading correspondences when sizes have different turn points. For an endpoint mismatch, verify each expected piece grades or document its disposition; the current reports do not name a piece as ungraded.

## 14. CLI regression checks

Run these against the newly installed or built executables, not an older executable on PATH.
Use separate output folders and sufficient media for success cases.
Pass each application's quoted executable path explicitly; the executable does not need to be on `PATH`.

The rebuilt CLI runners passed bounded relative-input checks. The remaining rows require checks beyond those runners.
Run from the repository root; repeat from a different directory using matching relative paths.

Use the built executables and explicit test inputs. These commands exercise the bounded suites; add the same input paths and CLI options to any direct run.

```powershell
python src\test\CollectionTest\test_pattern_input_status.py "<Seamly2D.exe>" build\pattern-input-validation
python src\test\Seamly2DTest\test_measurement_cli.py "<SeamlyMe.exe>" build\measurement-cli --individual test-seamly-layout\measurements\individual\male_chest_102cm.smis --multisize test-seamly-layout\measurements\multisize\gost_man_ru.smms --pattern test-seamly-layout\patterns\jacket2_40-146.sm2d
python src\test\SeamlyLayoutTest\test_headless_export.py "<SeamlyLayout.exe>" build\cli-validation --seamly2d "<Seamly2D.exe>" --pattern test-seamly-layout\patterns\jacket2_40-146.sm2d --measurements test-seamly-layout\measurements\multisize\gost_man_ru.smms --size 40 --height 146
```

--> Use the above cli commands to perform the checks below:

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
- [x] Leave original patterns and measurements unchanged.
- [x] Keep edited validation copies and exports for repeat tests.
- [ ] Do not mark the entire backlog complete while grading issues, the remaining derived-piece Flip matrix, or CAD checks remain unresolved.

Reference: [existing user checklist](AGENT_CHECKLIST.md). This checklist uses current expectations: format 0.7.9, fitted annotation handoff, and unchanged label/grainline/ positions during Flip.
