# QA — DXF-ASTM, producer attributes, grading, handoff capture

Check each box. Write FAIL and the task number beside a failed step.

## 0. Setup

- [ ] Install `packaging\windows\seamly-msi\x64\seamly-x64.msi`.
- [ ] Help > About shows the new build number.

## 1. Handoff capture (Layout.38.2.1)

Start Seamly2D from PowerShell, so SeamlyLayout inherits the variable:

```powershell
$env:SEAMLYLAYOUT_SAVE_HANDOFF = 'C:\Users\susan\seamly2d\layouts\handoff.svg'
& 'C:\Program Files\SeamlyApps\seamly2d.exe'
```

- [ ] Open `male_shirt.sm2d`. Press Layout. `handoff.svg` exists and opens in a browser.
- [ ] Set the variable to `handoff.svg` (relative). Layout Mode: warning popup names `SEAMLYLAYOUT_SAVE_HANDOFF`. Canvas still loads.
- [ ] New PowerShell window, no variable. Layout Mode: no file, no popup.

## 2. Producer attributes and Quantity (Layout.66)

Use the capture from step 1. Open `handoff.svg` in a text editor.

- [ ] Every `data-type="piece"` group has `data-quantity`.
- [ ] Set piece A: Quantity 3. Set piece B: Quantity 2, On fold checked. Recapture.
  - [ ] Piece B group has `data-on-fold="true"`. Piece A has none.
  - [ ] Each notch group has `data-notch-type` (e.g. `slit slit tNotch`).
- [ ] SeamlyLayout: export DXF-ASTM (R13), Annotated. Search the `.txt`:

| Piece | Expected `Quantity:` |
|---|---|
| A (3) | `3,0` |
| B (2, on fold) | `2,0` |
| Any piece with 2, not on fold | `1,1` |

- [ ] The "pieces lack data" dialog no longer lists "no Quantity" for the interface pieces.
- [ ] Decide: on-fold quantity `N,0`. Keep or change?

## 3. DXF-ASTM single size (DXF.21.5, DXF.8.4, DXF.22.1, DXF.3.5)

- [ ] Export `male_shirt` as DXF-ASTM (R13). Run it through https://aw.fyi/dxf. Save the report in `test-seamly-layout-input/`.
  - [ ] FullSleeve_M: no "not needed" points at pleat (L8) and slit (L11) ends.
- [ ] DXF.22.1: FrontPanel L1 junction. Report to ApparelWerks, or accept as an advisory.
- [ ] DXF.3.5: import the R13 file and the CLO3D file into CLO3D. Pieces, sew lines, notches appear.

## 4. Built-in seam allowance slit depth (DXF.18.5, Seamly2D.10.6)

- [ ] Piece with a built-in seam allowance, width W. Layout Mode. Export DXF-ASTM, Annotated.
- [ ] In the `.txt`, each layer 4 POINT has group 30 = W / 2.

## 5. Multisize layout in the running apps (Layout.38.3)

Pattern: `HJA.sm2d` with its `.smms`, or `C:\Users\susan\seamly2d\patterns\jacket2_40-146.sm2d`.

- [ ] Layout Mode opens SeamlyLayout. Settings shows "Multisize Layout".
- [ ] Nested: tabs "All sizes" plus one per size. Sizes stack per piece.
- [ ] Marker: every size placed alone; labels say "(size N)".
- [ ] Capture this handoff with step 1 as `HJA_handoff.svg`. Copy it to `test-seamly-layout-input/` (Layout.38.2.2).

## 6. Graded export (Layout.61.7)

Multisize layout from step 5.

- [ ] Select "All sizes". Export DXF-ASTM (R13).
  - [ ] Success dialog. Note any "not graded" lines; send them to me.
  - [ ] A `.rul` file sits beside the `.dxf`, same name.
- [ ] `.rul` in a text editor:
  - [ ] Header: `UNITS:METRIC`, `SAMPLE SIZE:<base size>`, `SIZE LIST:` with every size.
  - [ ] `RULE: DELTA` lines; the sample size column is `0.00,0.00`.
- [ ] `.dxf` in a text editor:
  - [ ] `Sample Size:<base size>`, `Grade Rule Table:<file name>`.
  - [ ] Each piece: one block per size, in size order. `# 1`, `# 2` … texts in the sample size block only.
- [ ] Select a size tab. Export DXF-ASTM. No `.rul`; `Sample Size:` blank.
- [ ] aw.fyi: upload the graded `.dxf` (and the `.rul` if it asks). Save the report in `test-seamly-layout-input/`.
- [ ] Open the graded `.dxf` in CLO3D or another CAD reader. Sizes stack on the grade reference line.
