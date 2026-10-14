# Test checklist — MSI 26.10.1115

Tests the changes merged in `46eed35264`: piece quantity, handoff capture, graded DXF-ASTM export.
Mark each box. Write FAIL and a short note beside a failed step.

## 0. Install

- [x] Close Seamly2D, SeamlyMe and SeamlyLayout.
- [x] Run `packaging\windows\seamly-msi\x64\seamly-x64.msi`.
- [x] Seamly2D Help > About shows 26.10.1115.

## [x] 1. Start Seamly2D with handoff capture

The setting reaches SeamlyLayout only when Seamly2D starts from this PowerShell window.

- [x] Open PowerShell. Run:

  ```powershell
  $env:SEAMLYLAYOUT_SAVE_HANDOFF = 'C:\Users\susan\seamly2d\layouts\handoff_male_shirt.svg';
  & 'C:\Program Files\SeamlyApps\seamly2d.exe'
  ```

- [x] Open `C:\Users\susan\seamly2d\patterns\male_shirt.sm2d`. Press Layout.
  - [x] SeamlyLayout opens with the pieces.
  - [x] `handoff_male_shirt.svg` exists in `C:\Users\susan\seamly2d\layouts\`.
- [x] Close both apps. In the same PowerShell window, run:

  ```powershell
  $env:SEAMLYLAYOUT_SAVE_HANDOFF = 'handoff.svg';
  & 'C:\Program Files\SeamlyApps\seamly2d.exe'
  ```

  - [x] Press Layout: a warning names `SEAMLYLAYOUT_SAVE_HANDOFF` and "absolute". The pieces still load.
- [x] Start Seamly2D from the Start menu. Press Layout: no warning, no file.

## [x] 2. Piece quantity

`Quantity:R,L` in a DXF-ASTM file: R = pieces cut as drawn, L = pieces cut mirrored.

- [x] Start Seamly2D as in step 1 (absolute path). Open `male_shirt.sm2d`. Piece mode.
- [x] Piece A (e.g. Yoke): edit piece > Properties: Quantity 3, "on fold" off.
- [x] Piece B (e.g. CollarTop): Quantity 2, "on fold" on.
- [x] Piece C (e.g. Sleeve): Quantity 2, "on fold" off.
- [ ] Press Layout and export SVG
  --> 'test-seamly-layout\layouts\male_shirt\male_shirt_202610121755.svg'
  - [ ] Open the captured SVG 'male_shirt_202610121755.svg' in VS Code. Search:
    - [x] `data-quantity` on every `data-type="piece"` group.
    - [x] `data-on-fold="true"` on piece B only.
    - [ ] `data-notch-type` on each notch group, one word per notch. `data-seam-allowances` and `data-notch-segments` have the same value count. Sum of `data-notch-segments` = `M` count in `d`.
      --> too many words per notch line. Added a task to fix this to TODO_SEAMLYLAYOUT.md (Layout.29)
- [x] SeamlyLayout: Settings > Submit, then Create Layout. Export > DXF-ASTM (R13) > Standard. Open the `.dxf` in VS Code. Search `Quantity:`:

| Piece | Expected |
|---|---|
| A: 3, not on fold | `Quantity:3,0` |
| B: 2, on fold | `Quantity:2,0` |
| C: 2, not on fold | `Quantity:1,1` |

- [x] The "DXF-ASTM: Missing Piece Data" dialog before the save no longer says "no Quantity".
- [x] Do not save the pattern changes.

## [x] 3. Multisize layout

Pattern: HJA (your multisize pattern), or `C:\Users\susan\seamly2d\patterns\jacket2_40-146.sm2d`.

- [x] Start Seamly2D.
- [x] Open the pattern `seamly2d`. Press Layout.
- [x] Settings shows "Multisize Layout:" with Nested and Marker.

### Create Nested layouts

- [x] Nested, Submit, Create Layout: tabs "All sizes" plus one per size. Each piece's sizes sit stacked.
  - [x] Export to DXF R13, all sizes
    - [x] All sizes DXF file: `test-seamly-layout\layouts\jacket2\jacket2_40-146_nested_allsizes_R13_202610121136.dxf'
    - [x] All sizes RUL file: 'test-seamly-layout\layouts\jacket2\jacket2_40-146_nested_allsizes_R13_202610121136.rul'
    - [x] All sizes Screenshots: 'test-seamly-layout\layouts\jacket2\jacket2_nested_screenshot_1.png', 'test-seamly-layout\layouts\jacket2\jacket2_nested_screenshot_2.png'
  - [x] Export to DXF R13, single size
    - [x] Single size DXF file: 'test-seamly-layout\layouts\jacket2\jacket2_40-146_nested_size240_R13_202610121142.dxf'
      --> to save time I saved only one size (size 240)
    - [x] Single size Screenshots: 'test-seamly-layout\layouts\jacket2\jacket2_nested_size240_screenshot_1.png'

### Create Marker layouts

- [x] Marker, Submit, Create Layout: every size placed alone; piece labels say "(size N)".
  - [x] Export to DXF R13, All sizes
    - [x] All sizes DXF file: 'test-seamly-layout\layouts\jacket2\jacket2_40-146_marker_allsizes_R13_202610121149.dxf'
    - [x] All sizes Screenshots: test-seamly-layout\layouts\jacket2\jacket2_marker_screenshot_1.png, test-seamly-layout\layouts\jacket2\jacket2_marker_screenshot_2.png, test-seamly-layout\layouts\jacket2\jacket2_marker_screenshot_3.png
  - [x] Export to DXF R13, Single size
    - [x] Single Size DXF file: test-seamly-layout\layouts\jacket2\jacket2_40-146_marker_size240_R13_202610121152.dxf
    - [x] Single Size Screenshots: test-seamly-layout\layouts\jacket2\jacket2_marker_size240_screenshot_1.png
  - [x] Export to DXF R13, All Tabs
    - [x] All Tabs DXF files, basename pattern 'jacket2_40-146_marker_alltabs_R13_*.dxf' in 'test-seamly-layout\layouts\jacket2\'
    - [x] All Tabs Screenshots: 'test-seamly-layout\layouts\jacket2\jacket2_marker_alltabs_screenshot_1.png'

## 4. Analyze Layouts

### Nested - All Sizes

- [ ] In VSCode, Open the Nested 'All Sizes' DXF file: 'test-seamly-layout\layouts\jacket2\jacket2_40-146_nested_allsizes_R13_202610121136.dxf'
  - [x] Search `Sample Size:<base size>` near the top of ENTITIES
    --> line 234822: 'Sample Size:500'
  - [x] Search `Grade Rule Table:<file name>` near the top of ENTITIES.
    --> line 234842: 'Grade Rule Table:jacket2_40-146_nested_allsizes_R13_202610121136'
  - [ ] Search `Piece Name:Front` (or any piece): one block per size, in size order.
    --> line 1666: 'Piece Name:piece-1_s220_M'
        line 5400: 'Piece Name:piece-9_s240_M'
        line 9166: 'Piece Name:piece-17_s260_M'
        line 12932: 'Piece Name:piece-25_s280_M'
        line 16698: 'Piece Name:piece-33_s300_M'
        line 20496: 'Piece Name:piece-41_s320_M'
        and more...
    --> No piece name 'Front' was found

- [x] In VSCode, Open the `.rul` file: 'test-seamly-layout\layouts\jacket2\jacket2_40-146_nested_allsizes_R13_202610121136.rul'
  - [x] Lines `UNITS:METRIC` appears only once in the file
    --> line 5: 'UNITS:METRIC'
  - [x] `SAMPLE SIZE:<base size>`
    --> line 7: 'SAMPLE SIZE:500'
  - [x] `NUMBER OF SIZES:<n>`
    --> line 8: 'NUMBER OF SIZES:26'
  - [x] `SIZE LIST:` with every size.
    --> line 9: 'SIZE LIST:220 240 260 280 300 320 340 360 380 400 420 440 460 480 500 520 540 560 580 600 620 640 660 680 700 720'
  - [x] `RULE: DELTA 1 ...` lines follow. In each, the sample size pair is `0.00,0.00`.
    --> line 10: 'RULE: DELTA 1 -241.51,-7.44 -224.26,-6.97 -207.01,-6.48 -189.76,-5.98 -172.51,-5.47 -155.25,-4.96 -138.00,-4.43 -120.75,-3.89 -103.50,-3.35 -86.25,-2.80 -69.00,-2.25 -51.75,-1.69 -34.50,-1.13 -17.25,-0.57 0.00,0.00 17.25,0.57 34.50,1.14 51.75,1.72 69.00,2.30 86.25,2.88 103.50,3.46 120.75,4.04 138.00,4.62 155.25,5.21 172.50,5.80 189.75,6.39', followed by additional RULE: DELTA N lines

- [x] Get the Apparel Werks DXF report
  - [x] Upload the Nested All Sizes DXF file to https://aw.fyi/dxf
  - [x] upload the RUL file if it asks
    --> it did not ask for the RUL file
  - [x] Save the report in `test-seamly-layout/'
    --> layouts\jacket2\jacket2_40-146_nested_allsizes_R13_202610121136-dxf-report.pdf'

### Nested - Single Size

- [ ] In VSCode, Open the Nested 'Single Size' DXF file:  
  --> 'test-seamly-layout\layouts\jacket2\jacket2_40-146_nested_size240_R13_202610121142.dxf'
  - [ ] Search `# 1` (found only in the single size blocks)
    --> not found
  - [x] `Sample Size:` is blank
    --> line 17626: 'Sample Size:'

- [x] Get the Apparel Werks DXF report
  - [x] Upload the Nested 'Single Size' DXF file to https://aw.fyi/dxf
  - [x] Save the report in `test-sesamly-layout/`
    --> 'test-seamly-layout\layouts\jacket2\jacket2_40-146_nested_size240_R13_202610121142-dxf-report.pdf'

### Marker - All Sizes

- [ ] Open the `.rul` in VS Code:
  - [ ] Lines `UNITS:METRIC`, `SAMPLE SIZE:<base size>`, `NUMBER OF SIZES:<n>`, `SIZE LIST:` with every size.
  - [ ] `RULE: DELTA 1 ...` lines follow. In each, the sample size pair is `0.00,0.00`.
  --> No RUL file was created with the Marker 'All Sizes' DXF R13 export

- [ ] Open the Marker 'All Sizes' DXF file in VS Code: 'test-seamly-layout\layouts\jacket2\jacket2_40-146_marker_allsizes_R13_202610121149.dxf'
  - [x] Search `Sample Size:<base size>` near the top of ENTITIES
    --> line 41934: 'Sample Size:'
    Sample size is blank
  - [x] Search `Grade Rule Table:<file name>` near the top of ENTITIES.
    --> line 41954: 'Grade Rule Table:'
    Grade rule table is blank
  - [ ] Search `Piece Name:Front` (or any piece): one block per size, in size order.
    --> 'Piece Name:' appears 25 times
    line 820: Piece Name:piece-201_s720_M
    line 4226: Piece Name:piece-146_s580_M
    line 4810: Piece Name:piece-154_s600_M
    line 5394: Piece Name:Detail
    all remaining Piece Names are 'Detail'
  - [ ] Search `# 1`: found only in the sample size blocks.
    --> '# 1' not found

- [x] Get the Apparel Werks DXF report
  - [x] Upload the Nested 'Single Size' DXF file to https://aw.fyi/dxf
  - [x] Save the report in 'test-seamly-layout\layouts\jacket2\jacket2_40-146_marker_allsizes_R13_202610121149-dxf-report.pdf'

### Marker: Single Size

- [x] In VSCode, open the Marker 'Single Size' DXF file: 'test-seamly-layout\layouts\jacket2\jacket2_40-146_marker_size240_R13_202610121152.dxf'
  - [x] `Sample Size:` is blank
  --> line 17626: Sample Size:
  Sample size is blank

### CLO3D

Perform these tests after all other tests have passed

- [ ] import the multisize "Nested" DXF: 'test-seamly-layout\layouts\jacket2\jacket2_40-146_nested_allsizes_R13_202610121136.dxf' 
  - [ ] The sizes appear as one graded piece per piece name

- [ ] import the multisize "Single size" DXF:
  - [ ] The size appears as one graded piece per piece name

- [ ] import the DXF-ASTM (R13) and DXF-ASTM (CLO3D) files of `male_shirt`
  - [ ] Pieces, sew lines and notches appear

## 5. Older checks still open

- [x] `male_shirt` DXF-ASTM (R13)
  - [x] upload to aw.fyi. FullSleeve_M: no "not needed" points at the pleat and slit ends. Save the report in `test-sesamly-layout/`
  --> test-sesamly-layout\male_shirt_R13_202610112117-dxf-report.pdf
  --> Perform this test after all other tests pass
- [ ] Piece with a built-in seam allowance of width W (Piece Properties > Seam allowance, Built in):
  - [ ] Start Seamly2D with handoff capture (step 1). Press Layout. In the captured SVG, the piece has a `cutline` group (the main path) and a `seamline` group W inside it.
  - [ ] SeamlyLayout canvas and Adjust Mode draw both lines. The packing gap is measured from the cut line.
  - [ ] Export SVG and PDF: both lines appear.
  - [ ] Export Layout, Export DXF-ASTM > Annotated. Layer 1 is the cut line. Layer 14 is the seam line.
  - [ ] In the `.txt`, each layer 4 notch has group 30 = W / 2.
  - [ ] Piece Properties > Hide seam line. Press Layout: the piece has a `cutline` group and no `seamline` group.
  - Seamly2D piece mode still draws no seam line for this piece: not in Layout.28.
  --> Perform this test after all other tests (except CLO3D test) have passed

## 6. Decisions

- [x] On-fold quantity: `2,0` for Quantity 2 on fold. Keep, or change? 
  --> Keep
- [ ] FrontPanel contradiction: report it to ApparelWerks, or accept it?
  --> wait until after all tests have passed before reporting to ApparelWerks

## 7. Multisize tabs (next MSI)

Replaces the `# 1` checks in §4: `# 1` is now in the Base Size file only.

Pattern: `C:\Users\susan\seamly2d\patterns\jacket2_40-146.sm2d`.

- [ ] Layout Settings has no "Multisize Layout" choice.
- [ ] Create Layout: tabs Marker, Nested, Base Size 500, Size 220 … Size 720.
- [ ] Media grows to fit (Layout.25). Settings: Paper, Roll, width 48 in. Create Layout:
  - [ ] No warning names the Marker tab. Before Layout.25 it named 112 pieces.
  - [ ] Marker tab: the roll is longer than 500 in (about 800 in). No piece sits outside the frame.
  - [ ] Adjust Layout on the Marker tab: the frame shows the full length.
  - [ ] Export SVG from the Marker tab: the page height matches the canvas.
- [ ] Settings: Paper, Sheet 36 × 48 in. Create Layout: the Marker sheet is longer than 48 in. A warning names only pieces wider than 35.5 in.
- [ ] Marker or Nested tab shown, Export menu:
  - [ ] DXF-ASTM items are disabled.
  - [ ] A note reads "DXF-ASTM: select Base Size or a size tab".
- [ ] Base Size 500 tab, Export > DXF-ASTM (R13):
  - [ ] The folder holds a `.dxf` and a `.rul` with the same name.
  - [ ] In the `.dxf`, search `# 1`: found.
  - [ ] `Sample Size:500` and `Grade Rule Table:<file name>` near the top of ENTITIES.
  - [ ] `Size Name:` is 500 in every block. One block per piece.
  - [ ] In the `.rul`, `SAMPLE SIZE:500`, `NUMBER OF SIZES:26`, and `RULE: DELTA` lines.
  - [ ] Upload the `.dxf` (and the `.rul` if asked) to https://aw.fyi/dxf. Save the report beside the files.
- [ ] Size 240 tab, Export > DXF-ASTM (R13):
  - [ ] No `.rul` file.
  - [ ] Search `# 1`: not found.
  - [ ] `Sample Size:` is blank.
- [ ] Export all tabs checked, Marker tab shown, Export > DXF-ASTM (R13):
  - [ ] Files for Base Size 500 and every size. No Marker or Nested file.
  - [ ] Only the Base Size file has a `.rul`.
- [ ] Export all tabs checked, Export > PDF:
  - [ ] Files for Marker, Nested and every size. No Base Size file.
- [ ] Export file names (Layout.24). Export all tabs unchecked:
  - [ ] Size 220 tab, Export > DXF-ASTM (R13): save dialog offers `jacket2_40-146_R13_Size220_<stamp>.dxf`.
  - [ ] Base Size 500 tab, same export: `jacket2_40-146_R13_BaseSize500_<stamp>.dxf`; the `.rul` has the same name.
  - [ ] Marker tab, Export > PDF (Tiled): `jacket2_40-146_Marker_<stamp>_tiled.pdf`.
- [ ] Export all tabs checked, Export > PDF, keep the offered name:
  - [ ] The save dialog offers `jacket2_40-146_<stamp>.pdf`, with no tab.
  - [ ] Files: `jacket2_40-146_Marker_<stamp>.pdf`, `…_Nested_<stamp>.pdf`, `…_Size220_<stamp>.pdf`, and so on.
- [ ] Export all tabs checked, Export > PDF, rename the file to `test.pdf`:
  - [ ] Files: `test_Marker.pdf`, `test_Nested.pdf`, `test_Size220.pdf`, and so on.
- [ ] Individual pattern (`.smis`), Export > PDF: `<pattern>_<stamp>.pdf`, with no tab.
- [ ] CLO3D: import the Base Size `.dxf` with its `.rul`. Each piece appears once, graded.
