SIMPLE_WORKFLOW_DESCRIPTION.md
Do not delete this file
## Application Assumptions
* 'Import SVG File': The input SVG file (from Seamly2D) <svg> element's width and height is expressed in 'mm', 'cm', or 'in', e.g. width=500mm. We strip off the suffix from width and height and convert them to pixels (e.g. widthPX and heightPX). All other data in the input svg file is expressed in pixels.
* 'Settings': The user inputs their preference of units so they can meaningfully input their media and layout data. When 'Submit' is clicked, the Settings numerical data is converted to pixels.
* The application always performs functions using pixel units, and does not perform functions using inch, cm, or mm units.
* All coordinates in layout_dom should be in pixels. The layout dom's <svg> elements width and height values should be expressed in pixels.
* No scaling is allowed in the application except for display purposes. No scaling is allowed in any dom (input_dom, flat_dom, vertical_dom, translate_dom, layout_dom)

## Application Workflow

The application workflow loop consists of:

0. Load data and set workflow state:

* find and load preferences data for use by the 'Import SVG file', 'Settings', and 'Export' features.
* If the preferences variables have empty values, use these values:
  * Import SVG Directory: './input/'
  * Layout Output Directory: './output/'
  * Default Settings file: './settings/default_settings.json
  * DXF Viewer: prompt the user to open Preferences and set the DXF viewer application or URL
  * PDF Viewer: prompt the user to open Preferences and set the PDF Viewer application or URL
  * Enable 'Import SVG File' button
  * Disable 'Settings' button
  * Disable 'Create Layout' button
  * Disable 'Adjust Layout' button
  * Disable 'Export' button

1. Import --> User clicks 'Import SVG File' button:

   * File-picker opens to allow user to select an input svg file, or cancel:
     * User clicks 'Cancel':
       * file picker closes
       * application state does not change
     * User selects file and clicks 'Import':
       * File picker closes
       * Creates self.input_dom from the selected input svg file
       * On Success:
         * Display self.input_dom in the left canvas
         * Clear the the right canvas
         * Reset input svg data, settings data, layout data, and export data
         * Enable Settings Button
         * Disable 'Create Layout' button (ensure it is disabled)
         * Disable 'Adjust Layout' button (ensure it is disabled)
         * Disable 'Export' button (ensure it is disabled)
2. Settings --> User clicks 'Settings' button:

   * NOTE 1: Settings dialog gathers parameters for layout_dom properties and layout process
   * NOTE 2: Maintain paired variables: one in user units (in/cm/mm) and one in pixels, for every dimension field in SettingsModel.
   * Read the preferences' settings filepath and load the  settings file. If this file does not exist, or this variable is empty, then load the application's default settings file in './settings/default_settings.json'.
   * Dialog allows users to:
     * Update fields
     * Click on 'Load' button: Select and read a settings file into current data using a file-picker dialog
     * Click on 'Save' button: Save current data to a settings file using a file-saver dialog
     * Click on 'Submit' button:
       * Convert numeric data to pixels (widthPx, heightPx, marginLeftPx, marginRightPx, marginTopPx, marginBottomPx)
       * Save data to temp_dir/layoutSettings.json
       * On Success:
         * For paperType!=tiled:
           * Create self.layout_dom where svg element has width=widthPx, height=heightPx
           * Create background rectangle with id=backgroundRect, x=0, y=0, width=widthPx, height=heightPx, fill=white, stroke=none
           * Create content rectangle with id=contentRect, x=marginLeftPx, y=marginTopPx, width=widthPx - marginLeftPx - marginRightPx, height = heightPx - marginTopPx - marginBottomPx, stroke=black, stroke-width=1, fill=none
           * Create group element with id=Rectangles
           * Add background rectangle to Rectangles group
           * Add content rectangle to Rectangles group
           * Add Rectangles group to self.layout_dom
         * For paperType=tiled:
           * Calculate input_dom information:
             * inputDomWidthPx  = input_dom's svg element's width converted from mm to px
             * inputDomHeightPx = input_dom's svg element's height converted from mm to px
           * Calculate Tile information:
             * tileWidthPx  = widthPx - marginLeftPx - marginRightPx
             * tileHeightPx = heightPx - marginTopPx - marginBottomPx
             * tileCols = (input_dom.width/tileWidthPx) then if tileCols not an integer round tileCols up to the next integer
             * tileRows = (input_dom.height/tileHeightPx) then if tileRows not an integer round tileRows up to the next integer
             * contentRectWidthPx  = (tileCols * tileWidthPx)
             * contentRectHeightPx = (tileRows * tileHeightPx)
             * layoutWidthPx  = contentRectWidthPx + marginRightPx + marginLeftPx
             * layoutHeightPx = contentRectHeightPx + marginTopPx + marginBottomPx
           * Create self.layout_dom with svg element width=layoutWidthPx and height=layoutHeightPx
           * Create rectangle group element with id=Rectangles
           * Create background rectangle element with id=backgroundRect, x=0, y=0, width=layoutWidthPx, height=layoutHeightPx, fill=white, stroke=none
           * Add background rectangle to Rectangles group
           * Create content rectangle with id=contentRect, x=marginLeftPx, y=marginTopPx, width=(layoutWidthPx - marginLeftPx - marginRightPx), height=(layoutHeightPx - marginTopPx - marginBottomPx), stroke=black, stroke-width=1, fill=none
           * Add content rectangle to Rectangles group
           * Create tile marker element with id=tileMarker, x=0, y=0, width=tileWidthPx, height=tileHeightPx, fill=none, stroke=gray, stroke-width=1
           * Add tile marker element to `<defs>` group
           * Create tile group element with id=tileRects
           * Create the tile rectangles:
             rowY = marginTopPx
             colX = marginLeftPx
             for row in tileRows:
             * Create path string d_string="M"
             * rowY = marginTopPx + tileHeightPx * row
               for col in tileCols:
             * colX = marginLeftPx + tileWidthPx * col
             * tileCoord = " colX, rowY"
             * Append tileCoord to d_string
             * Create row path element with id=tile `<row><col>`, d=d_string, fill=none, stroke=none, marker=#tileMarker
             * Add row path element to tileRects group
           * Add tile group element to Rectangles group
           * Add Rectangles group to self.layout_dom
         * Display self.layout_dom in the right canvas
         * Enable 'Create Layout' button
         * Disable 'Adjust Layout' button (ensure it is disabled)
         * Disable 'Export' button (ensure it is disabled)
3. Layout Preprocessing --> User clicks 'Create Layout' button:

   * Flatten #1:
   * Copy self.input_dom to self.flat_dom
   * Flatten (bake-in transforms) self.flat_dom to remove transforms (except for text and tspan elements)
   * Save self.flat_dom to flat1_dom.svg
   * Verticalization:
   * Copy self.flat_dom to self.vertical_dom
   * Verticalize self.vertical_dom so pieces are vertical based on to each piece's grainline angle
   * Save self.vertical_dom to vertical.svg
   * Flatten #2:
   * Copy self.vertical_dom to self.flat_dom
   * Flatten (bake-in transforms) self.flat_dom to remove transforms (except for text and tspan elements)
   * Save self.flat_dom to flat2_dom.svg
   * Translation:
   * Copy self.flat_dom to self.translate_dom
   * Translate pieces to origin (0,0)
   * Save self.translate_dom to translate.svg
   * Flatten #3:
   * Copies self.translate_dom to self.flat_dom
   * Flattens (bakes-in) self.flat_dom to remove transforms (except for text and tspan elements)
   * Save self.flat_dom to flat3_dom.svg
   * On Success:
     * Call to layout_processing()
4. Layout Processing:

   * NOTE: LAYOUT_PPI = 96 px/in is a global constant; no dpi parameter is passed between functions
   * NOTE: GAP_PX = 5 px clearance between adjacent placed pieces
   * Call `extract_piece_rects()` to extract bounding boxes from self.flat_dom; sort pieces by area descending
   * Use a MaxRects algorithm to arrange pieces in self.layout_dom's content rectangle:
     * NOTE: rect.widthPx = rect.maxX (= rect.minX + rect.width); rect.heightPx = rect.maxY (= rect.minY + rect.height)
     * Create rect1 equal to the content rectangle and add to free rects list; record in creation history
     * for each piece in sorted pieces:
       * If piece is larger than contRect: mark piece as unplaced, continue to next piece
       * Select free rect using top-left fit: among all free rects that contain the piece,
         choose the one with the lowest minY; break ties by lowest minX
       * If no fitting free rect exists: mark piece as unplaced, continue to next piece
       * If fitting rect found:
         * place pattern piece at (rect.minX, rect.minY) in self.layout_dom
         * remove rect from free rects list
         * create 2 new rectangles from the used rect; record each in creation history; add to free rects list:
           rect `<next>`.minX   = rect.minX + piece.widthPx + GAP_PX
           rect `<next>`.minY   = rect.minY
           rect `<next>`.width  = rect.widthPx - rect `<next>`.minX
           rect `<next>`.height = rect.heightPx

           rect<next+1>.minX   = rect.minX
           rect<next+1>.minY   = rect.minY + piece.heightPx + GAP_PX
           rect<next+1>.width  = rect.widthPx
           rect<next+1>.height = rect.heightPx - rect<next+1>.minY
         * split all OTHER existing free rects that overlap the placed piece:

           * for each other free rect F that overlaps (piece.minX..piece.maxX, piece.minY..piece.maxY):
             * remove F from free rects list
             * generate up to 4 sub-rects; record each in creation history; add those with positive dimensions:
               left   = (F.minX,              F.minY, piece.minX - F.minX,              F.heightPx)
               right  = (piece.maxX + GAP_PX, F.minY, F.widthPx - (piece.maxX + GAP_PX), F.heightPx)
               top    = (F.minX,              F.minY, F.widthPx,  piece.minY - F.minY)
               bottom = (F.minX, piece.maxY + GAP_PX, F.widthPx, F.heightPx - (piece.maxY + GAP_PX))
           * NOTE: splitting other overlapping rects prevents later pieces from being placed
             in a free rect that covers space already occupied by a placed piece
         * prune: remove any free rect fully contained within another free rect
     * continue loop pieces
   * Assemble layout SVG:
     * Write placed pattern pieces into self.layout_dom at their packed positions
     * Add debug-bboxes group: one semi-transparent filled colored rect per placed piece slot
     * Add debug-freerects group: one dashed colored border + creation number per rect in creation history
   * Save self.layout_dom to <exe_dir>/output/layout_dom.svg (debug)
   * Trim excess whitespace from layout_dom when media_type=roll:
     * highestY    = max(piece.y + piece.h) for all placed pieces (pixels)
     * trimmedHeight = highestY + marginBottomPx
     * If trimmedHeight < bin_h: set `<svg height=trimmedHeight>` and `<backgroundRect height=trimmedHeight>`
      NOTE: tiled trim deferred — see docs/layout-docs/tiling-docs/TILING_REDUCTION_WORKFLOW.md
   * On Success:
     * Emit layout_finished signal → right canvas reloads from self.layout_dom
     * Disable 'Create Layout' button
     * Enable 'Adjust Layout' button
     * Enable 'Export' button
5. Adjust Layout --> User clicks 'Adjust Layout' button:

   * Activates 'AdjustMode' that:
     * Disables 'Import SVG File', 'Settings', 'Create Layout', and 'Export' buttons
     * Displays a persistent 'Adjust Mode' banner or colored canvas border so the user always knows they are in an interactive state
     * Displays a status bar hint showing available shortcuts: Ctrl+Z (undo), Ctrl+RightShift+Z (redo), click to select, Ctrl+click to multi-select
     * Displays 'Accept Adjustments' and 'Discard Adjustments' buttons at the bottom of the interactive canvas
     * Displays an asterisk or visual 'dirty' indicator on the 'Accept Adjustments' button whenever the canvas has unsaved changes
     * Wraps the displayed layout in an interactive wrapper that:
       * Allows user to move and rotate piece groups in the layout displayed in the right canvas as an interactive canvas. Move and rotate of items within piece groups is not allowed.
       * Allows rubber-band selection or Ctrl+click to select multiple piece groups for simultaneous move/rotate.
       * When rotating a piece group, displays the current rotation angle numerically (e.g. '45°') adjacent to the piece. Holding Shift constrains rotation to 45° increments.
       * Optionally snaps pieces to a configurable grid interval (e.g. 5mm) or snaps piece edges to adjacent pieces to prevent accidental overlap.
       * Optionally locks piece rotation to multiples of the piece's grainline angle (stored in input_dom).
       * Highlights pieces in red when they overlap another piece or extend outside the content rectangle boundary.
       * Allows mouse-based pan and zoom functionality.
       * Allows `<Ctrl-Z>` and `<Ctrl-RightShift-Z>` keys for undo and redo functionality.
     * On user click 'Accept Adjustments':
       * If any pieces overlap or are outside the content rectangle, display a warning dialog listing the conflicts and ask the user to confirm before proceeding.
       * Applies piece transforms from canvas to self.layout_dom (changes nothing else in self.layout_dom)
       * Displays updated self.layout_dom in non-interactive canvas (e.g. exits 'AdjustMode')
       * Enables 'Import SVG File', 'Settings', 'Create Layout', and 'Export' buttons
     * On user click 'Discard Adjustments':
       * If the canvas has unsaved changes (dirty indicator is set), display a 'Discard changes?' confirmation dialog before proceeding.
       * Displays self.layout_dom in non-interactive canvas (e.g. exits 'AdjustMode')
       * Enables 'Import SVG File', 'Settings', 'Create Layout', and 'Export' buttons
     * NOTE: If the user returns to Settings and re-submits while in AdjustMode or while adjusted layout_dom exists, display a warning: 'This will replace your adjusted layout. Continue?' before rebuilding layout_dom.
6. Export --> User selects export option from dropdown list:

   * NOTE: Saves exported files to /output directory
   * DXF-ASTM:
   * PDF:
   * PDF Tiled:

## Operations outside of workflow loop:

1. Preferences --> User clicks 'Preferences' gear icon:
   * Read and display data from preferences file
   * Allows user to:
     * Update data
     * Browse and select directories using file-picker dialog
     * Save data to the preferences file

## Repository collaboration workflow

Use this Git workflow for all implementation tasks:

1. Start from `main` and create a short-lived branch for one feature/fix.
2. Commit focused changes and push the branch.
3. Open a pull request targeting `main`.
4. Review, test, and merge through the pull request flow.
5. Delete the merged branch (local + remote).

Branch policy:

- `main` is the default integration branch.
- Keep only designated long-lived branches (`3D-mode`, `knitting-mode`) in addition to `main`.
- Do not use or recreate `develop` for integration.
- Do not use or recreate `qt` as an integration branch.

---

SUMMARY:

**SeamlyLayout Application Workflow**

| Step                           | Trigger              | Key Actions                                                                                                                                                              |
| ------------------------------ | -------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **0. Init**              | App start            | Load preferences; enable Import only; all other buttons disabled                                                                                                         |
| **1. Import**            | Import SVG button    | File picker →`input_dom`; display in left canvas; enable Settings                                                                                                     |
| **2. Settings**          | Settings button      | Load settings file; user edits; Submit → builds `layout_dom` (with background/content rects, and for tiled: tile grid); display in right canvas; enable Create Layout |
| **3. Layout Preprocess** | Create Layout button | Flatten→Verticalize→Flatten→Translate→Flatten pipeline on `input_dom`; produces `flat_dom`                                                                       |
| **4. Layout Processing** | Auto after step 3    | MaxRects bin-packing of pieces into content rect; assemble layout SVG with debug overlays; trim roll media; emit signal → right canvas reloads; enable Adjust/Export    |
| **5. Adjust Layout**     | Adjust Layout button | TODO                                                                                                                                                                     |
| **6. Export**            | Export dropdown      | DXF-ASTM,PDF, PDF Tiled, PNG →`/output/`                                                                                                                             |
| **Preferences**          | Gear icon            | Read/edit/save preferences file (dirs, viewer apps)                                                                                                                      |

**Key constants:** `LAYOUT_PPI = 96 px/in`, `GAP_PX = 5 px` between placed pieces.
