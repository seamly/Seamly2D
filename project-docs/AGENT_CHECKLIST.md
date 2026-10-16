# Agent checklist — user tests

User tests from `AGENT_STATE.json` `needs_user_test`.
Mark each box. Write FAIL and a short note beside a failed step.

| Task | Sections | Check |
|---|---|---|
| Layout.38.6 | 1 | Multisize pattern: Nested and Marker tabs |
| Layout.67.8 | 1, 3, 5–8 | Multisize tabs and exports |
| Layout.25.7 | 2, 10 | Media grows to fit |
| Layout.68.5 | 4 | PDF true size |
| Layout.24.7 | 4–7, 12 | Export file names |
| Layout.26.7 | 5 | Missing Piece Data dialog scrolls |
| Layout.27.3.3 | 5 | Base Size 500: all 8 pieces graded |
| Layout.61.7.2 | 5 | aw.fyi report |
| Layout.69.3 | 9 | PNG and JPG resolution |
| Layout.71.6 | 11 | Media grows in width |
| Layout.72.8 | 12 | Oversized pieces: split or enlarge |
| Layout.70.5 | 12 | Left-out pieces warning |
| Layout.28.7 | 13 | Built-in seam line |
| Layout.73.5 | 1, 5 | Unique piece names |

## 0. Install

Only Layout.27.3.3 needs the next MSI. All other sections run on the installed 26.10.1421.

- [ ] Close Seamly2D, SeamlyMe and SeamlyLayout.
- [ ] Run the new MSI.
- [ ] Seamly2D Help > About shows a version newer than 26.10.1421.

## 1. jacket2: multisize tabs

Pattern: `C:\Users\susan\seamly2d\patterns\jacket2_40-146.sm2d`.
Save exports in `test-seamly-layout\layouts\jacket2\`.

- [ ] Run `C:\Users\susan\seamly2d\capture_handoff_jacket2.bat`. Press Layout.
  - [ ] `C:\Users\susan\seamly2d\layouts\handoff_jacket2.svg` exists. The agent needs it for Layout.27.13.4.
- [ ] Layout Settings has no "Multisize Layout" choice.
- [ ] Settings: Paper, Roll, width 48 in. Submit. Create Layout.
- [ ] Tabs: Marker, Nested, Base Size 500, Size 220 … Size 720.
- [ ] Nested tab: each piece once, its sizes stacked. Grainlines vertical. Sizes centered on the largest size.
- [ ] Marker tab: every size of every piece placed alone. Labels start with the size, e.g. `Size_220_Piece_3_Detail_1_A`. No "(size N)".

## 2. Media grows in length

Settings from section 1: Paper, Roll, 48 in.

- [ ] No warning names the Marker tab. Before Layout.25 it named 112 pieces.
- [ ] Marker tab: roll longer than 500 in (about 800 in). No piece outside the frame.
- [ ] Marker tab, Adjust Layout: the frame shows the full length.
- [ ] Marker tab, Export > SVG: page height matches the canvas.

## 3. Export menu on Marker and Nested

- [ ] Marker tab, Export menu: DXF-ASTM items disabled.
- [ ] A note reads "DXF-ASTM: select Base Size or a size tab".
- [ ] Nested tab: same.

## 4. PDF true size

Export all tabs unchecked.

- [ ] Marker tab, Export > PDF. Acrobat, File > Properties: page size is 48 in × the Marker roll length from section 2.
  - A page side over 200 in uses `/UserUnit`. Other readers can show the page smaller. Use Acrobat.
- [ ] Marker tab, Export > PDF (Tiled).
  - [ ] Save dialog offers `jacket2_40-146_Marker_<stamp>_tiled.pdf`.
  - [ ] Print one page at 100% (Actual size). A known piece length measures true.

## 5. Base Size 500: DXF-ASTM

Export all tabs unchecked.

- [ ] Base Size 500 tab, Export > DXF-ASTM (R13).
- [ ] Save dialog offers `jacket2_40-146_R13_BaseSize500_<stamp>.dxf`.
- [ ] "DXF-ASTM: Missing Piece Data" dialog appears. Expected: jacket2 hides labels and grainlines. "no piece label" means the hidden label block, not the name.
  - [ ] One line per piece, not one per size.
  - [ ] Each piece named `Size_500_Piece_<n>_<name>[_<letter>]`, n = 1 to 8. No spaces. All names differ. No "Detail (3)".
  - [ ] Cyrillic names show: Подборт, Комір.
  - [ ] The list scrolls. Buttons stay visible.
- [ ] No warning "Size_500_Piece_<n>_<name>: not graded, size N has other turn points than size 500".
- [ ] The folder holds a `.dxf` and a `.rul` with the same name.
- [ ] "Export Complete" names the `.dxf` and the `.rul`.
- [ ] Open the `.dxf` in VS Code:
  - [ ] Search `# 1`: found.
  - [ ] `Sample Size:500` and `Grade Rule Table:<file name>` near the top of ENTITIES.
  - [ ] `Size Name:` is 500 in every block. One block per piece.
  - [ ] Search `Piece Name:piece-`: not found.
- [ ] Open the `.rul`: `SAMPLE SIZE:500`, `NUMBER OF SIZES:26`, `RULE: DELTA` lines.
- [ ] Upload the `.dxf` (and the `.rul` if asked) to https://aw.fyi/dxf. Save the report beside the files.
  - [ ] All 8 pieces graded.
  - [ ] No merged "Detail" pieces.
  - [ ] No layer 5 endpoint mismatch.
  - [ ] No duplicate vertices.
  - Layout.61.7.2 named the Nested tab. Nested has no DXF-ASTM export since Layout.67, so Base Size 500 replaces it.

## 6. Size 220: DXF-ASTM

- [ ] Size 220 tab, Export > DXF-ASTM (R13).
- [ ] Save dialog offers `jacket2_40-146_R13_Size220_<stamp>.dxf`.
- [ ] No `.rul` file.
- [ ] Search `# 1`: not found.
- [ ] `Sample Size:` is blank.

## 7. Export all tabs

- [ ] Check Export all tabs. Marker tab shown. Export > DXF-ASTM (R13):
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
- [ ] Cancel: only Layout Settings, Preferences and Exit are enabled.
- [ ] Submit the settings: Create Layout is enabled.
- [ ] Create Layout. Choose "Split the pieces". OK.
  - [ ] Each leg is 2 parts.
  - [ ] Each part has a blue label `<name> part i of 2, joins j`.
  - [ ] Each cut edge has 2 green match notches.
  - [ ] Left-out pieces warning: one bullet per piece. A long list scrolls. OK stays visible.
  - [ ] Export > PDF: save dialog offers `trousers_<stamp>.pdf`, with no tab. The split parts appear.
  - [ ] Export > DXF-ASTM (R13): blocks named `Back (1/2)` and `Back (2/2)`.
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
- [ ] Export > DXF-ASTM (R13) > Annotated. Layer 1 is the cut line. Layer 14 is the seam line.
- [ ] In the `.txt`, each layer 4 notch has group 30 = W / 2.
- [ ] Piece Properties > Hide seam line. Press Layout: the piece has a `cutline` group and no `seamline` group.
- [ ] Do not save the pattern changes.
- Seamly2D piece mode draws no seam line for this piece. Not in Layout.28 scope.
