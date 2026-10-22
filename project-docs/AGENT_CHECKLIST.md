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
| Layout.61.7.2 | 5 | DXF - R13 aw.fyi report | Fabric PASS (report: 7 of 7 graded, 0 departures). Lining not checked.
| Layout.61.7.2 | 5 | DXF - R12 aw.fyi report | >
| Layout.69.3 | 9 | PNG and JPG resolution | PASS
| Layout.71.6 | 11 | Media grows in width | ?
| Layout.72.8 | 12 | Oversized pieces: split or enlarge |
| Layout.89.3 | 12 | Oversized pieces dialog: light text |
| Layout.88.2 | 3 | Export menu: DXF-ASTM note reads in full |
| Layout.70.5 | 12 | Left-out pieces warning | ?
| Layout.28.7 | 13 | Built-in seam line | ?
| Layout.73.5 | 1, 5 | Unique piece names | PASS
| Seamly2D.12.8 | 14 | SeamlyLayout opens in front | PASS
| Layout.83.4 | 15 | Nested legend squares |
| Layout.82.3 | 16 | Nested size colors: dark and pastel |
| Layout.87.3 | 4, 17 | Marker tab: PDF (Tiled) disabled |
| Layout.86.3 | 3, 18 | Export > DXF submenu |
| Layout.84.4 | 19 | Nested exports: one grainline and label set per piece |
| Layout.80.7 | 20 | Seamly2D: mark a fold line |
| Layout.80.3.1 | 20 | Seamly2D: Piece Mirror |
| Layout.80.6.6 | 21 | SeamlyLayout: fold lines on the fold, DXF-ASTM mirror line |
| Layout.80.6.3 | 21 | Adjust Mode: the fold line moves with its piece |
| Layout.85.5 | 3, 18, 22 | DXF-AAMA (R10) export |

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
  - [x] All 8 pieces graded. ? --> Fabric: 7 of 7 graded (7 layer 5 lines). Detail 4 is Lining, so it is not in this file.
  - [x] No merged "Detail" pieces. ? --> 7 blocks, 7 names.
  - [x] No layer 5 endpoint mismatch. ? --> none reported.
  - [x] No duplicate vertices. ? --> none reported.
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
- [ ] Base Size 500 tab, Export > DXF: "DXF-ASTM (R12)", "DXF-ASTM (R13)", "DXF-ASTM (CLO3D)", "DXF-AAMA (R10)", all enabled. No note.
- [ ] Export > DXF > DXF-ASTM (R13): the export starts (Missing Piece Data or Standard/Annotated dialog). Cancel.
- [ ] Marker tab, Export > DXF: "DXF: select Base Size or a size tab" is the first item and reads in full. The four items are disabled.
- [ ] Nested tab: same.
- [ ] View menu: one "DXF-ASTM" item, no DXF submenu.

## 19. jacket2: Nested exports show one grainline and label set

Needs the next MSI. Settings from section 1: Paper, Roll, 48 in.

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

## 21. jacket2: fold lines on the fold

Needs the next MSI. jacket2 Back has a fold line.

- [ ] Settings: Media Fabric, Fabric Width 60 in. "Place fold lines on the left edge (the fold)" is disabled.
- [ ] Check Fabric Folded: the option is enabled. Check it. Layout Mode Along Grainline. Submit. Create Layout.
- [ ] Size 220 tab: Back lies with its dashed fold line on the left media edge. No margin left of it.
- [ ] Marker tab: every size of Back touches the left edge.
- [ ] Nested tab: Back is not moved to the edge (expected).
- [ ] Adjust Mode: drag Back. The fold line moves with it. Apply.
- [ ] Layout Mode With Nap, Pieces point Up. Create Layout. Size 220 tab: Back on the left edge. Its labels read forward, not mirrored.
- [ ] Uncheck the option. Create Layout: Back leaves the edge; the left margin is back.
- [ ] Option on, Along Grainline. Base Size 500 tab, Export > DXF > DXF-ASTM (R13). Open the `.dxf`:
  - [ ] The Back block has one `LINE` on layer `6`.
  - [ ] Its two points are the first two `VERTEX` points of the layer `1` `POLYLINE`.
  - [ ] No other block has layer `6`.
  - [ ] No export warning "the fold line is not on the cut line".
- [ ] Upload the `.dxf` to https://aw.fyi/dxf: no departure for the Back mirror line. Back is graded.

## 22. jacket2: DXF-AAMA (R10)

Needs the next MSI. Settings from section 1. Export all tabs unchecked.

- [ ] Base Size 500 tab, Export > DXF: "DXF-AAMA (R10)" is the last item, enabled.
- [ ] Click it. The Missing Piece Data dialog title reads "DXF-AAMA: Missing Piece Data". Export anyway, then Standard.
- [ ] Save dialog title reads "Save DXF-AAMA (R10) File". It offers `jacket2_40-146_AAMA_Fabric_BaseSize500_<stamp>.dxf`.
- [ ] The folder holds a `.dxf` and a `.rul` with the same name. "Export Complete" names both.
- [ ] Open the `.rul`: the first line is `ANSI/AAMA VERSION:1.0.0`. Then `AUTHOR:Seamly2D`, `SAMPLE SIZE:500`, `RULE: DELTA` lines.
- [ ] Open the `.dxf` in VS Code:
  - [ ] The line after `$ACADVER` and `1` is `AC1006`.
  - [ ] Search `Size:500`: found once per block. Search `Size Name:`: not found.
  - [ ] Search `Curve Tolerance` and `ASTM/D13`: not found.
  - [ ] Regex search `^8\n(15|8[0-7])$`: no match. No layer 15 or 80–87.
  - [ ] Search `# 1`: found.
- [ ] Import the `.dxf` and `.rul` in an AAMA reader (CLO3D: File > Import > DXF (AAMA/ASTM)).
  - [ ] Each piece appears once, with notches and grainline.
  - [ ] The pieces grade to the other sizes.
  - [ ] Write the reader name and version beside this step.
- [ ] Size 220 tab, Export > DXF > DXF-AAMA (R10) > Annotated Version. The `.txt` first line reads `// DXF-AAMA Annotated Version with Inline Comments`. No `.rul`.
- [ ] Marker tab, Export > DXF: "DXF: select Base Size or a size tab" is the first item. "DXF-AAMA (R10)" is disabled.
- [ ] Base Size 500 tab, Export > DXF > DXF-ASTM (R13): the save dialog still reads "Save DXF-ASTM (R13) File" and offers `…_R13_…`.
