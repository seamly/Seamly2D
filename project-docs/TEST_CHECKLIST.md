# Test checklist — MSI 26.10.1115

Tests the changes merged in `46eed35264`: piece quantity, handoff capture, graded DXF-ASTM export.
Mark each box. Write FAIL and a short note beside a failed step.

## 0. Install

- [x] Close Seamly2D, SeamlyMe and SeamlyLayout.
- [x] Run `packaging\windows\seamly-msi\x64\seamly-x64.msi`.
- [x] Seamly2D Help > About shows 26.10.1115.

## 1. Start Seamly2D with handoff capture

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

## 2. Piece quantity

`Quantity:R,L` in a DXF-ASTM file: R = pieces cut as drawn, L = pieces cut mirrored.

- [x] Start Seamly2D as in step 1 (absolute path). Open `male_shirt.sm2d`. Piece mode.
- [x] Piece A (e.g. Yoke): edit piece > Properties: Quantity 3, "on fold" off.
- [x] Piece B (e.g. CollarTop): Quantity 2, "on fold" on.
- [x] Piece C (e.g. Sleeve): Quantity 2, "on fold" off.
- [ ] Press Layout. Open the captured SVG in VS Code. Search:
  - [x] `data-quantity` on every `data-type="piece"` group.
  - [x] `data-on-fold="true"` on piece B only.
  - [ ] `data-notch-type` on each notch group, one word per notch line segment. Word count = `M` count in `d` = `data-seam-allowances` value count.
- [x] SeamlyLayout: Settings > Submit, then Create Layout. Export > DXF-ASTM (R13) > Standard. Open the `.dxf` in VS Code. Search `Quantity:`:

| Piece | Expected |
|---|---|
| A: 3, not on fold | `Quantity:3,0` |
| B: 2, on fold | `Quantity:2,0` |
| C: 2, not on fold | `Quantity:1,1` |

- [x] The "DXF-ASTM: Missing Piece Data" dialog before the save no longer says "no Quantity".
- [x] Do not save the pattern changes.

## 3. Multisize layout

Pattern: HJA (your multisize pattern), or `C:\Users\susan\seamly2d\patterns\jacket2_40-146.sm2d`.

- [x] Start Seamly2D.
- [x] Open the pattern `C:\Users\susan\seamly2d\patterns\jacket2_40-146.sm2d`. Press Layout.
- [x] Settings shows "Multisize Layout:" with Nested and Marker.
- [x] Nested, Submit, Create Layout: tabs "All sizes" plus one per size. Each piece's sizes sit stacked.
- [x] Marker, Submit, Create Layout: every size placed alone; piece labels say "(size N)".
- [x] Export 'jacket2_40-146_size_220_202610112039.svg' to `test-seamly-layout-input/`.

## 4. Graded DXF-ASTM export

Use the multisize layout from step 3. Do it once with Nested and once with Marker.

- [ ] Select the "All sizes" tab. Export > DXF-ASTM (R13) > Standard.
  - [ ] The export finishes. Copy any "not graded" lines from the success dialog and send them to me.
  - [ ] The folder holds a `.dxf` and a `.rul` with the same name. 
    --> NO .rul FILE!!!
- [ ] Open the `.rul` in VS Code:
  - [ ] Lines `UNITS:METRIC`, `SAMPLE SIZE:<base size>`, `NUMBER OF SIZES:<n>`, `SIZE LIST:` with every size.
  - [ ] `RULE: DELTA 1 ...` lines follow. In each, the sample size pair is `0.00,0.00`.
- [ ] Open the `.dxf` in VS Code:
  - [ ] `Sample Size:<base size>` and `Grade Rule Table:<file name>` near the top of ENTITIES.
  - [ ] Search `Piece Name:Front` (or any piece): one block per size, in size order.
  - [ ] Search `# 1`: found only in the sample size blocks.
- [ ] Select one size tab. Export > DXF-ASTM (R13). No `.rul` file; `Sample Size:` is blank.
- [ ] Export menu > check "Export all tabs". Export DXF-ASTM (R13): one `.dxf` per tab; only the "All sizes" file has a `.rul`.
- [ ] Upload the graded `.dxf` to https://aw.fyi/dxf (add the `.rul` if it asks). Save the report in `test-seamly-layout-input/`.
- [ ] If you have CLO3D: import the graded `.dxf`. The sizes appear as one graded piece per piece name.

## 5. Older checks still open

- [ ] `male_shirt` DXF-ASTM (R13) to aw.fyi. FullSleeve_M: no "not needed" points at the pleat and slit ends. Save the report in `test-seamly-layout-input/`. 
  --> test-seamly-layout-input\male_shirt_R13_202610112117-dxf-report.pdf
- [ ] CLO3D: import the DXF-ASTM (R13) and DXF-ASTM (CLO3D) files of `male_shirt`. Pieces, sew lines and notches appear. 
  --> Do this after all other tests pass
- [ ] Piece with a built-in seam allowance of width W: Layout, Export DXF-ASTM > Annotated. In the `.txt`, each layer 4 notch has group 30 = W / 2.

## 6. Decisions

- [ ] On-fold quantity: `2,0` for Quantity 2 on fold. Keep, or change?
- [ ] FrontPanel contradiction: report it to ApparelWerks, or accept it?
