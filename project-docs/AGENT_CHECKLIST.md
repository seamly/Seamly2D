# Agent checklist — user tests

User tests from `AGENT_STATE.json` `needs_user_test`.
Mark each box. Write FAIL and a short note beside a failed step.

| Task | Sections | Check |
|---|---|---|
| Layout.38.6 | 1 | Multisize pattern: Nested and Marker tabs | PASS
| Layout.67.8 | 1, 3, 5–8 | Multisize tabs and exports | PASS
| Layout.25.7 | 2, 10 | Media grows to fit | PASS
| Layout.68.5 | 4 | PDF true size | PASS
| Layout.24.7 | 4–7, 12 | Export file names | Material - PASS (Fabric - PASS, Lining - PASS), Nested - PASS, Marker - PASS
| Layout.26.7 | 5 | Missing Piece Data dialog scrolls |PASS 
| Layout.27.3.3 | 5 | Base Size 500: all 8 pieces graded | PASS
| Layout.69.3 | 9 | PNG and JPG resolution | PASS
| Layout.73.5 | 1, 5 | Unique piece names | PASS
| Seamly2D.12.8 | 14 | SeamlyLayout windows opens in front of Seamly2D | PASS

JACKET_2
| Layout.61.7.2 | 5 | SL > jacket_2 > DXF-ASTM (R13) - multisize (base size only) - aw.fyi report - Fabric | See jacket2_40-146_R13_Fabric_BaseSize500_202610231417-dxf-report.pdf | PASS. Layer 5 caption accepted as a viewer note
| Layout.84.4 | 19 | SL > jacket_2 > Nested exports: one grainline and label set per piece | See jacket2_40-146_R13_Fabric_BaseSize500_202610231417-dxf-report.pdf | Not checked: the Nested tab has no DXF export. Run §19
| Layout.93.7 | 23 | SL > jacket_2 > DXF piece properties: Material, Missing Piece Data, Rotation, Flip, Tilt, Fold | ?
| Layout.92.4 | 25 | SL > jacket_2 > Material tabs over the layout; every material laid out | ?
| Layout.94.5 | 26 | SL > jacket_2 > Grainlines and labels in the base size's place; Nested stacks | ?

MALE_SHIRT
| Layout.28.7 | 13 | S2D > male_shirt > Built-in seam line | ?
| Layout.80.7 | 20 | S2D > male_shirt > mark a fold line | ?
| Layout.80.3.1 | 20 | S2D: male_shirt > Piece Mirror tool | ?
| Layout.95.6 | 24 | S2D > male_shirt > Piece Properties: Rotation radio buttons | ?
| Layout.96.6 | 24 | S2D > male_shirt > Piece Properties: Tilt radio buttons and degrees | ?
| Layout.97.5 | 24 | S2D > male_shirt, jacket2 > Piece Properties: On fold check box, no fold position | ?
| Layout.98.6 | 24 | S2D > male_shirt > Piece Properties: Flip radio buttons, Flip submenu | ?
| Seamly2D.11.9 | 27 | S2D > male_shirt > Built-in seam line in Piece mode, at a fold line | ?

| Layout.83.4 | 15 | SL > male_shirt > Nested legend squares | ?
| Layout.82.3 | 16 | SL > male_shirt > Nested size colors: dark and pastel | ?
| Layout.87.3 | 4, 17 | SL > male_shirt > Marker tab: PDF (Tiled) disabled | ?
| Layout.86.3 | 3, 18 | SL > male_shirt > Export > DXF submenu | ?
| Layout.80.6.6 | 21 | S > male_shirt: fold lines placed along left edge of content rect | ?
| Layout.80.6.3 | 21 | SL > male_shirt > Adjust Mode: fold line moves with its piece | ?
| Layout.88.2 | 3 | SL > male_shirt > Export menu: DXF-ASTM note reads in full | ?
| Layout.80.6.6 | 21 | SL > Export > DXF-ASTM (R13): mirror line | ?
| Layout.61.7.2 | 5 | SL > male_shirt > DXF-ASTM (R13): aw.fyi report - Lining | ?
| Layout.85.5 | 3, 18, 22 | SL > male_shirt > DXF-AAMA (R11): aw.fyi report - Fabric | ?
| Layout.85.5 | 3, 18, 22 | SL > male_shirt > DXF-AAMA (R11): aw.fyi report - Lining | ?

TROUSERS
| Layout.71.6 | 11 | SL > trousers > Media grows in width | ?
| Layout.72.8 | 12 | SL > trousers > Settings > B0 sheet: Oversized pieces - split or enlarge |
| Layout.89.3 | 12 | SL > trousers > Settings > B0 sheet: Oversized pieces dialog: light text |
| Layout.70.5 | 12 | SL > trousers > Left-out pieces warning | ?

## [x] 0. Install

Only Layout.27.3.3 needs the next MSI. All other sections run on the installed 26.10.1421.

- [x] Close Seamly2D, SeamlyMe and SeamlyLayout.
- [x] Run the new MSI.
- [x] Seamly2D Help > About shows a version newer than 26.10.1421.

## [x] 1. jacket2: multisize tabs

Pattern: `C:\Users\susan\seamly2d\patterns\jacket2_40-146.sm2d`.
Save exports in `test-seamly-layout\layouts\jacket2\`.

- [x] Run `C:\Users\susan\seamly2d\capture_handoff_jacket2.bat`. Press Layout.
- [x] `C:\Users\susan\seamly2d\layouts\handoff_jacket2.svg` exists. The agent needs it for Layout.27.13.4. --> is this still needed?
  --> Yes. It replaces the Rust test fixture (Layout.27.13.4, Layout.27.7.2). File found, saved 13:47.
- [x] Layout Settings has no "Multisize Layout" choice.
- [x] Settings: Paper, Roll, width 48 in. Submit. Create Layout.
- [x] Tabs: Marker, Nested, Base Size 500, Size 220 … Size 720.
- [x] Nested tab: each piece once, its sizes stacked. Grainlines vertical. Sizes centered on the largest size.
- [x] Marker tab: every size of every piece placed alone. The size is inserted before the date, e.g. `jacket2_40-146_Fabric_Size340_202610211332.'

## 2. Media grows in length

Settings from section 1: Paper, Roll, 48 in.

- [x] No warning names the Marker tab. Before Layout.25 it named 112 pieces.
- [x] Marker tab: roll longer than 500 in (about 800 in). No piece outside the frame.
- [x] Marker tab, Adjust Layout: the frame shows the full length.
- [ ] Marker tab, Export > SVG: page height matches the canvas.

## 3. Export menu on Marker and Nested

- [x] Marker tab, Export menu: DXF-ASTM items disabled.
- [ ] A note reads "DXF: select Base Size or a size tab". Layout.85 changed the text from "DXF-ASTM: …".
  --> the note is there but it is mostly unreadable as it extends past the right side of the menu
  --> Filed: Layout.88.
- [ ] Next MSI: the note is at the top of Export > DXF. The submenu is wider. The whole note reads, with no "..." at the end.
- [ ] Nested tab: same.
- [ ] Export > SVG submenu: each item reads in full, with no "..." at the end.

## 4. PDF true size

Export all tabs unchecked.

- [x] Marker tab, Export > PDF. Acrobat/Inkscape, File > Properties: page size is 48 in × the Marker roll length from section 2.
  - A page side over 200 in uses `/UserUnit`. Other readers can show the page smaller. Use Acrobat.
  --> Inkscape: width=47.510", height=601.217", the content rect and background rectangle are not exported so this is the correct width and height.
- [ ] Marker tab, Export > PDF (Tiled).
  - [ ] Save dialog offers `jacket2_40-146_Marker_<stamp>_tiled.pdf`.
  - [ ] Print one page at 100% (Actual size). A known piece length measures true.
  --> See Layout.87 - disable PDF (tiled) for markers due to lockups.
  - Layout.87 disables this item on the Marker tab. Run the two checks above on the Nested tab instead; §17 checks the Marker tab.

## 5. Base Size 500: DXF-ASTM

Export all tabs unchecked.

- [x] Base Size 500 tab, Export > DXF-ASTM (R13).
- [x] Save dialog offers `jacket2_40-146_R13_Fabric_BaseSize500_<timestamp>.dxf`.
- [x] "DXF-ASTM: Missing Piece Data" dialog appears. Expected: jacket2 hides labels and grainlines. "no piece label" means the hidden label block, not the name.
  - Next MSI: the dialog no longer names a hidden label. See §23.
  - [ ] One line per piece, not one per size.
  - [ ] Each piece named `Size_500_Piece_<n>_<name>[_<letter>]`, n = 1 to 8. No spaces. All names differ. No "Detail (3)".
  - [ ] Cyrillic names show: Подборт, Комір.
  - [x] The size tabs scrolls to the right. All size tabs are accessible.
- [x] No warning "Size_500_Piece_<n>_<name>: not graded, size N has other turn points than size 500".
- [x] The folder holds a `.dxf` and a `.rul` with the same name.
- [x] "Export Complete" names the `.dxf` and the `.rul`.
- [ ] Open the `.dxf` in VS Code:
  - [x] Search `# 1`: found.
  - [x] `Sample Size:500` and `Grade Rule Table:<file name>` found on (many) lines below ENTITIES.
  - [x] `Size Name:` is 500 in every block. One block per piece.
  - [x] Search `Piece Name:piece-`: not found.
- [x] Open the `.rul`: `SAMPLE SIZE:500`, `NUMBER OF SIZES:26`, `RULE: DELTA` lines.
- [x] Upload the `.dxf` (and the `.rul` if asked) to https://aw.fyi/dxf. Save the report beside the files.
  - Report `jacket2_40-146_R13_Fabric_BaseSize500_202610211408_apparelwerks_compliance_report.pdf`: 0 departures, 5 advisories (no action).
  - Report `20261023/jacket2_40-146_R13_Fabric_BaseSize500_202610231417-dxf-report.pdf` (MSI 26.10.2312): 0 departures, the same 5 advisories.
  - [x] All 8 pieces graded. ? --> Fabric: 7 of 7 graded (7 layer 5 lines). Detail 4 is Lining, so it is not in this file.
    --> Report 1417: 7 of 7 graded, sizes 220–720.
  - [x] No merged "Detail" pieces. ? --> 7 blocks, 7 names.
    --> Report 1417: same. Names `Size_500_Piece_1_…` to `Size_500_Piece_7_…`, no spaces.
    --> Cyrillic names are `\U+XXXX` escapes. They decode to Подборт and Комір. aw.fyi shows the escapes (advisory, allowed).
  - [x] No layer 5 endpoint mismatch. ? --> none reported.
    --> Report 1417, page 1 caption: "L5 endpoint mismatch up to 227.990 — sizes not a pure translation". Not a departure.
    --> Not new: the `.rul` rules match the 1408 export. The 1408 report is a browser print with no overview chart.
    --> Likely source: Back. Its layer 5 line ends on rule 41 (no growth) and rule 63 (x −132.99 to +104.50 mm).
    --> Layer 5 sets the X axis of the rules (D6673 §4.3.5). Only a graded nest needs the same line in every size (§4.3.1.5). This file is not a graded nest.
    --> Decision: accept the caption as a viewer note. Revisit if §8 (CLO3D) grades Back wrong.
  - [x] No duplicate vertices. ? --> none reported. Report 1417: none.
  - [ ] Lining tab: Base Size 500, Export > DXF > DXF-ASTM (R13), aw.fyi. Detail 4 graded.
  - Layout.61.7.2 named the Nested tab. Nested has no DXF-ASTM export since Layout.67, so Base Size 500 replaces it.

## 6. Size 220: DXF-ASTM

- [ ] Size 220 tab, Export > DXF > DXF-ASTM (R13).
- [ ] Save dialog offers `jacket2_40-146_R13_Size220_<stamp>.dxf`.
- [ ] No `.rul` file.
- [ ] Search `# 1`: not found.
- [ ] `Sample Size:` is blank.

## 7. Export all tabs

- [ ] Check Export all tabs. Marker tab shown. Export > DXF > DXF-ASTM (R13):
  - [ ] Files for Base Size 500 and every size. No Marker or Nested file.
  - [ ] Only the Base Size file has a `.rul`.
  - [ ] "Export Complete" lists the `.rul` after the Base Size `.dxf`.
- [ ] Export > PDF, keep the offered name:
  - [ ] Save dialog offers `jacket2_40-146_<stamp>.pdf`, with no tab.
  - [ ] Files: `jacket2_40-146_Marker_<stamp>.pdf`, `…_Nested_<stamp>.pdf`, `…_Size220_<stamp>.pdf`, and so on.
  - [ ] No Base Size file.
- [ ] Export > PDF, rename the file to `test.pdf`:
  - [ ] Files: `test_Marker.pdf`, `test_Nested.pdf`, `test_Size220.pdf`, and so on.
- [ ] Uncheck Export all tabs.

## 8. CLO3D

- [ ] Import the Base Size `.dxf` from section 5 with its `.rul`. Each piece appears once, graded.

## 9. PNG and JPG resolution

- [ ] Size 220 tab, Export > PNG, then Export > JPG.
- [ ] GIMP, Image > Print Size: 96 pixels/in, not 72. Check both files.
- [ ] Print the PNG at "Actual size". A known piece length measures true.

## 10. Media grows on a sheet

- [ ] Settings: Paper, Sheet 36 × 48 in. Submit. Create Layout.
- [ ] Marker sheet longer than 48 in. No piece left out. No warning.
  - `TEST_CHECKLIST.md` §7 expects a warning for pieces wider than 35.5 in. Layout.71 removed it: the media now widens.

## 11. Media grows in width

- [ ] Settings: Paper, Roll, width 10 in. Submit. Create Layout.
- [ ] Every tab is wider than 10 in. No warning.
- [ ] Adjust Layout on each tab: the frame matches the canvas.
- [ ] Export > SVG: page size matches the canvas.
- [ ] Close SeamlyLayout and Seamly2D. Do not save the pattern.

## 12. trousers: oversized pieces

Pattern: `C:\Users\susan\seamly2d\patterns\trousers.sm2d` (individual).

- [ ] Start Seamly2D from the Start menu. Open the pattern. Press Layout.
- [ ] Settings: Paper, Sheet 36 × 36 in. Submit. Create Layout.
- [ ] Dialog "Pieces do not fit the media" lists 4 legs: Back, Front_Pleats, Front_no_pocket, Front_with_pocket.
- [ ] Dialog text is light on the violet background: heading, piece list, "Split the pieces", "Make the media bigger", both descriptions. No black text.
- [ ] Cancel: only Layout Settings, Preferences and Exit are enabled.
- [ ] Submit the settings: Create Layout is enabled.
- [ ] Create Layout. Choose "Split the pieces". OK.
  - [ ] Each leg is 2 parts.
  - [ ] Each part has a blue label `<name> part i of 2, joins j`.
  - [ ] Each cut edge has 2 green match notches.
  - [ ] Left-out pieces warning: one bullet per piece. A long list scrolls. OK stays visible.
  - [ ] Export > PDF: save dialog offers `trousers_<stamp>.pdf`, with no tab. The split parts appear.
  - [ ] Export > DXF > DXF-ASTM (R13): blocks named `Back (1/2)` and `Back (2/2)`.
- [ ] Create Layout. Choose "Make the media bigger". OK.
  - [ ] Sheet longer than 36 in. No split pieces. No warning.
- [ ] Close both apps.

## 13. male_shirt: built-in seam line

- [ ] Run `C:\Users\susan\seamly2d\capture_handoff_male_shirt.bat`.
- [ ] Piece mode. Pick one piece. Piece Properties > Seam allowance: Built in, width W. Note W.
- [ ] Press Layout. In `C:\Users\susan\seamly2d\layouts\handoff_male_shirt.svg`, the piece has:
  - [ ] a `cutline` group (the main path);
  - [ ] a `seamline` group W inside it.
- [ ] Canvas and Adjust Mode draw both lines. The packing gap is measured from the cut line.
- [ ] Export SVG and PDF: both lines appear.
- [ ] Export > DXF > DXF-ASTM (R13) > Annotated. Layer 1 is the cut line. Layer 14 is the seam line.
- [ ] In the `.txt`, each layer 4 notch has group 30 = W / 2.
- [ ] Piece Properties > Hide seam line. Press Layout: the piece has a `cutline` group and no `seamline` group.
- [ ] Do not save the pattern changes.
- Seamly2D piece mode draws no seam line for this piece. Not in Layout.28 scope.

## 14. jacket2: SeamlyLayout opens in front

Needs the next MSI.

- [ ] Start Seamly2D from the Start menu. Open `C:\Users\susan\seamly2d\patterns\jacket2_40-146.sm2d`.
- [ ] Piece mode: the Detail 1 and Detail 4 labels look as before.
- [ ] Press Layout.
  - [ ] A progress dialog shows "Preparing size 220 (1 of 26) for SeamlyLayout", then the other sizes.
  - [ ] The Seamly2D title never shows "Not Responding".
  - [ ] SeamlyLayout opens in front of Seamly2D, with the pieces. Expected: well under 26 s (the time before).
- [ ] Press Layout again: SeamlyLayout comes to the front.
- [ ] Press Draft mode, then Layout:
  - [ ] Message "SeamlyLayout is already open".
  - [ ] After OK, SeamlyLayout comes to the front. Only one SeamlyLayout window.
- [ ] Close SeamlyLayout. Press Layout, then Cancel in the progress dialog:
  - [ ] Seamly2D returns to Draft mode. No SeamlyLayout opens.
  - [ ] Press Layout again: SeamlyLayout opens.

## 15. jacket2: Nested legend squares

Needs the next MSI. Settings from section 1: Paper, Roll, 48 in.

- [ ] Nested tab: below the pieces, each legend entry shows a filled square in its size color, then "Size <size>".
- [ ] Each square is as wide as the old color line and as high as it is wide.
- [ ] Rows do not overlap. The frame holds every entry.
- [ ] Export > SVG and Export > PDF: the legend shows the same squares.

## 16. jacket2: Nested size colors

Needs the next MSI. Settings from section 1: Paper, Roll, 48 in.

- [ ] Nested tab: dark and pastel colors both appear.
- [ ] No two sizes look alike. Neighbor sizes in a stack differ clearly.
- [ ] Size 220 is blue.
- [ ] Pastel lines show on the white canvas.
- [ ] Each legend color matches its size on the pieces.
- [ ] Export > SVG, PDF and PNG: the files keep the colors.

## 17. jacket2: PDF (Tiled) on the Marker tab

Needs the next MSI.

- [ ] Settings: Paper, Paper Type Tiled. Submit. Create Layout.
- [ ] Marker tab, Export menu: "PDF (Tiled): select a tab other than Marker" is disabled.
- [ ] Nested tab and Size 220 tab, Export menu: "PDF (Tiled)" is enabled.
- [ ] Size 220 tab, Export > PDF (Tiled): the file is written.
- [ ] Check Export all tabs. Marker tab shown. Export > PDF (Tiled):
  - [ ] The item is enabled.
  - [ ] Files for Nested and every size. No Marker or Base Size file.
  - [ ] SeamlyLayout does not lock up.
- [ ] Uncheck Export all tabs.
- [ ] View menu, any tab: "PDF (Tiled)" is enabled.

## 18. jacket2: DXF submenu

Needs the next MSI. Settings from section 1.

- [ ] Export menu: one "DXF" item below "Export all tabs". No DXF-ASTM item in the Export menu itself.
- [ ] Base Size 500 tab, Export > DXF: "DXF-ASTM (R12)", "DXF-ASTM (R13)", "DXF-ASTM (CLO3D)", "DXF-AAMA (R11)", all enabled. No note.
- [ ] Export > DXF > DXF-ASTM (R13): the export starts (Missing Piece Data or Standard/Annotated dialog). Cancel.
- [ ] Marker tab, Export > DXF: "DXF: select Base Size or a size tab" is the first item and reads in full. The four items are disabled.
- [ ] Nested tab: same.
- [ ] View menu: one "DXF-ASTM" item, no DXF submenu.

## 19. jacket2: Nested exports show one grainline and label set

Needs the next MSI. Settings from section 1: Paper, Roll, 48 in.

- The DXF report 1417 does not cover this section: the Nested tab has no DXF export.
- Report 1417 shows no change on Base Size 500: at most one grainline (layer 7) and one label set (layer 15) per piece, as in report 1408. Подборт has no grainline: the pattern hides it.

- [ ] Nested tab canvas: every size still shows its grainline and labels.
- [ ] Export > SVG, any text mode. Open the file in Inkscape:
  - [ ] Each piece shows one grainline and one set of labels. The size label reads "Size 220".
  - [ ] Objects panel: `labels_and_grainlines` is the last item, directly below `size_720`.
  - [ ] Hide `labels_and_grainlines`: no grainline or label is left.
  - [ ] Expand `size_720`: its pieces hold no grainline or label.
- [ ] Export > PDF, PNG, JPG, PostScript (PS), EPS and HPGL > Plot: each piece shows one grainline and one set of labels.
- [ ] Settings: Paper, Paper Type Sheet. Submit. Create Layout. Nested tab, Export > PDF: same.
- [ ] Marker tab and Size 220 tab, Export > SVG: every piece keeps its grainline and labels.

## 20. Seamly2D: fold line and Piece Mirror

Needs the next MSI. Any pattern with a half piece, for example a back cut on the fold.

- [ ] Piece mode, Piece Properties > Path: mark two neighbouring nodes Fold Line.
  - [ ] The fold line is dashed. No seam allowance along it.
  - [ ] "On fold" is checked and locked.
  - [ ] Save, reopen: the fold line is still there.
  - [ ] Press Layout: the handoff has a `fold_line` group (`capture_handoff_*.bat`).
- [ ] Mark a third node Fold Line: OK is disabled.
- [ ] Piece mode > Piece Mirror, click the half piece, OK:
  - [ ] The full piece is right of the half. Its outline is symmetric. Seam allowance all round. No fold line.
  - [ ] The half piece is out of the layout.
  - [ ] Draft mode shows the Mirror by Line operation. Move a draft point: both halves follow.
  - [ ] Undo removes the full piece and the operation. Redo. Save, reopen.
- [ ] Piece Mirror, click a piece without a fold line: status bar message, no dialog.
- [ ] A pattern with several draft blocks: mirror a piece of the first block. Undo, redo, save, reopen.

## 21. jacket2: fold lines on the left edge

Needs the next MSI. jacket2 Back has a fold line.

- [ ] Settings: "Place fold lines on the left edge (fabric: no left margin)" is under "Add scale check grid", for paper and fabric.
- [ ] Media Fabric, Fabric Width 60 in, Fabric Folded. Check the option. Layout Mode Along Grainline. Submit. Create Layout.
- [ ] Size 220 tab: Back lies with its dashed fold line on the left media edge. No margin left of it.
- [ ] Marker tab: every size of Back touches the left edge.
- [ ] Nested tab: Back is not moved to the edge (expected).
- [ ] Adjust Mode: drag Back. The fold line moves with it. Apply.
- [ ] Layout Mode With Nap, Pieces point Up. Create Layout. Size 220 tab: Back on the left edge. Its labels read forward, not mirrored.
- [ ] Uncheck the option. Create Layout: Back leaves the edge; the left margin is back.
- [ ] Option on, Media Paper, Paper Type Roll. Create Layout. Size 220 tab: Back's fold line on the left edge of the content, inside the left margin.
- [ ] Paper Type Sheet, 36 × 48 in. Create Layout. Size 220 tab, Export > PDF: Back's fold line on the left content edge of its page.
- [ ] Option on, Along Grainline. Base Size 500 tab, Export > DXF > DXF-ASTM (R13). Open the `.dxf`:
  - [ ] The Back block has one `LINE` on layer `6`.
  - [ ] Its two points are the first two `VERTEX` points of the layer `1` `POLYLINE`.
  - [ ] No other block has layer `6`.
  - [ ] No export warning "the fold line is not on the cut line".
- [ ] Upload the `.dxf` to https://aw.fyi/dxf: no departure for the Back mirror line. Back is graded.

## 22. jacket2: DXF-AAMA (R11)

Needs the next MSI. Settings from section 1. Export all tabs unchecked.

- [ ] Base Size 500 tab, Export > DXF: "DXF-AAMA (R11)" is the last item, enabled.
- [ ] Click it. The Missing Piece Data dialog title reads "DXF-AAMA: Missing Piece Data". Export anyway, then Standard.
- [ ] Save dialog title reads "Save DXF-AAMA (R11) File". It offers `jacket2_40-146_AAMA_Fabric_BaseSize500_<stamp>.dxf`.
- [ ] The folder holds a `.dxf` and a `.rul` with the same name. "Export Complete" names both.
- [ ] Open the `.rul`: the first line is `ANSI/AAMA VERSION:1.0.0`. Then `AUTHOR:Seamly2D`, `SAMPLE SIZE:500`, `RULE: DELTA` lines.
- [ ] Open the `.dxf` in VS Code:
  - [ ] The line after `$ACADVER` and `1` is `AC1009`.
  - [ ] Search `Size:500`: found once per block. Search `Size Name:`: not found.
  - [ ] Search `Curve Tolerance` and `ASTM/D13`: not found.
  - [ ] Regex search `^8\n(15|8[0-7])$`: no match. No layer 15 or 80–87.
  - [ ] Search `# 1`: found.
- [ ] Import the `.dxf` and `.rul` in an AAMA reader (CLO3D: File > Import > DXF (AAMA/ASTM)).
  - [ ] Each piece appears once, with notches and grainline.
  - [ ] The pieces grade to the other sizes.
  - [ ] Write the reader name and version beside this step.
- [ ] With a fold line piece (§21 settings): the Back block has one `LINE` on layer `6`. Its two points are the first two `VERTEX` points of the layer `1` `POLYLINE`.
- [ ] Size 220 tab, Export > DXF > DXF-AAMA (R11) > Annotated Version. The `.txt` first line reads `// DXF-AAMA Annotated Version with Inline Comments`. No `.rul`.
- [ ] Marker tab, Export > DXF: "DXF: select Base Size or a size tab" is the first item. "DXF-AAMA (R11)" is disabled.
- [ ] Base Size 500 tab, Export > DXF > DXF-ASTM (R13): the save dialog still reads "Save DXF-ASTM (R13) File" and offers `…_R13_…`.

## 23. jacket2: DXF piece properties

Needs the next MSI. Settings from section 1. Export all tabs unchecked.

- [ ] Fabric tab, Base Size 500, Export > DXF > DXF-ASTM (R13).
  - [ ] Missing Piece Data lists Подборт only: "no grainline". No line says "piece label".
  - [ ] Open the `.dxf`: every block has `Material:Fabric`.
- [ ] Lining tab, Base Size 500, Export > DXF > DXF-ASTM (R13).
  - [ ] No Missing Piece Data dialog: Detail 4 shows its grainline.
  - [ ] Open the `.dxf`: the Detail 4 block has `Material:Lining` and `Quantity:1,1`.
- [ ] Seamly2D: set Back to Rotation 2-Way, Tilt CW 5°, Flip Horizontal (its label may stay hidden). Press Layout again.
  - [ ] Base Size 500, Export > DXF > DXF-ASTM (R13). The Back block piece text reads, in order: `Quantity:`, `Rotation:180`, `Flip:Y`, `Tilt:-5`, `Fold:Y`, `Material:Fabric`.
  - [ ] Every other block has `Fold:N` and no `Rotation:`, `Flip:` or `Tilt:` line.
  - [ ] DXF-AAMA (R11) of the same tab: no `Rotation:`, `Flip:`, `Tilt:` or `Fold:` line.
  - [ ] Upload the R13 file to https://aw.fyi/dxf: note any finding about `Flip:` or `Tilt:` beside this step.

## 24. Seamly2D: piece properties

Needs the next MSI. Pattern: `male_shirt.sm2d`, an older format. Save it under a new name first.

- [ ] Open the copy. Piece mode, open a piece's Piece Properties > Properties.
- [ ] Rotation shows five radio buttons: None, 1-Way, 2-Way (first row); 4-Way, Any (second row). None is selected.
- [ ] Select 2-Way. OK. Save. Close and reopen the pattern: Rotation is still 2-Way.
- [ ] Open the saved `.sm2d` in a text editor: `<version>0.7.7</version>`; the piece `data` element has `rotationWay="2-Way"`.
- [ ] Add a new piece: Rotation is None.
- [ ] Tilt shows three radio buttons: None, CCW, CW. None is selected. Degrees reads 0° and is disabled.
- [ ] Select CCW: Degrees is enabled. Type 100: the field stops at 90. Set 15. OK. Save, reopen: CCW, 15°.
- [ ] The saved `.sm2d` piece `data` element has `tilt="CCW"` and `tiltDegrees="15"`.
- [ ] Select None. OK. Save: `tilt="None"`, `tiltDegrees="0"`.
- [ ] No "Fold position" row. The row "Fold:" has an "On fold" check box, unchecked on a new piece.
- [ ] Check On fold. OK. Save: the piece `data` element has `onFold="true"` and no `foldPosition`. Uncheck, save: `onFold="false"`.
- [ ] Open an older pattern with a fold position (jacket2 Back: `foldPosition="Up/Down"`). Save it under a new name: no `foldPosition`; Back keeps `onFold="true"`.
- [ ] Attributes: "Flip:" shows None, Horizontal, Vertical, Both. None is selected. No "Forbid" check box.
- [ ] Select Vertical. OK. Save: the `piece` element has `flip="Vertical"` and no `forbidFlipping`. Reopen: Vertical.
- [ ] Right-click the piece: "Flip" submenu, Vertical checked. Choose Both. Undo: back to Vertical.
- [ ] Preferences > Pattern: no "Forbid flipping" check box. Help > Shortcuts: no "Forbid Flipping F" row.

## 25. jacket2: material tabs over the layout

Needs the next MSI. jacket2 handoff: Fabric and Lining. Settings from section 1.

- [ ] Before Settings: tabs Fabric and Lining over both canvases. Fabric is shown on both.
- [ ] Settings > Submit. Click Lining over the right canvas: both tab bars show Lining; the right canvas shows a blank canvas.
- [ ] Back to Fabric. Create Layout. The popup names each layout: "Laying out Nested (2 of …)", later "Laying out Lining: Marker", "Lining: Size 220".
- [ ] When the popup closes, Fabric shows its Marker tab. Click Lining over the right canvas: the left canvas shows Detail 4; the right canvas shows the Lining Marker tab with its own tabs.
- [ ] Lining: select Size 300. Back to Fabric, then Lining again: Size 300 is still shown.
- [ ] Lining, Adjust Mode: move Detail 4, Save. Switch to Fabric and back: the move is still there.
- [ ] While Create Layout runs and in Adjust Mode, both material tab bars are locked.
- [ ] Lining, Size 300 tab, Export > SVG: the default name has `_Lining_Size300_`. The file holds Detail 4 only.
- [ ] Individual pattern with two materials (male_shirt: set one piece's Material to Lining): Create Layout lays out both; each tab shows its pieces.

## 26. jacket2: grainlines and labels in the base size's place

Needs the next MSI. jacket2 handoff (grainlines and labels shown).

- [ ] Import: one warning names "Back: pattern label past the cut line in sizes 220, 240, 260, 280".
- [ ] Nested tab, Fabric: each piece shows one grainline stack and one label stack; the sizes' copies lie exactly on top of each other.
- [ ] Size 220 and Size 720 tabs: each piece's grainline and labels sit in the same place inside the piece as on the Base Size 500 tab.
- [ ] Lining, Nested tab: Detail 4's grainline, piece label and pattern label each stack as one.
- [ ] Seamly2D: move Back's pattern label toward the middle of the piece. Press Layout: no warning.

## 27. male_shirt: built-in seam line in Piece mode

Needs the next MSI. Do not save the pattern changes.

- [ ] Piece mode. Pick one piece. Piece Properties > Seam allowance: check Seam allowance and Built in. Note the width W.
  - [ ] Automatic tab: Width, Before and After stay editable. Angle is disabled. The Custom tab is disabled.
  - [ ] Uncheck Seam allowance, then check it again: the Automatic tab is enabled again.
- [ ] OK. The canvas draws the main path in the cut line style and a seam line W inside it in the seam line style. A hatch fills the space between.
- [ ] Right-click > Hide Seam Line: the seam line goes; the cut line stays. Show it again.
- [ ] View > Seam Allowance off: both lines stay; the hatch goes. Turn it on again.
- [ ] Piece Properties > Path: mark two neighbouring point nodes as Fold Line. OK.
  - [ ] Both ends of the seam line lie on the fold line. No seam line runs along the fold line or crosses it.
- [ ] Piece tool: draw a new piece with Built in checked. The preview shows the seam line inside the path; changing the width moves it.
- [ ] Press Layout. SeamlyLayout shows the cut line, the open seam line and the fold line.
- [ ] SeamlyLayout Export > DXF > DXF-ASTM (R13). Layer 1 is the cut line. Layer 14 is the seam line, open; its ends lie on the layer 6 mirror line.
- [ ] Seamly2D Piece mode > Export Pieces As, a DXF AAMA format. Layer 8 is the seam line, not a copy of the layer 1 outline.
