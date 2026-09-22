TILING_CALCULATION_WORKFLOW

## Workflow:

### Settings Data

#### Get userUnit, media, paperType, and paperSize from Settings:

userUnit (cm, in, or mm)
media
paperType
paperSize

#### Get paper size width and height and margin data from Settings, convert to pixels using userUnit

paperSizeWidth-->convert from userUnit to pixels as paperSizeWidthPx
paperSizeHeight-->convert from userUnits to pixels as to paperSizeHeightPx
marginLeft-->convert from userUnit to to pixels as marginLeftPx
marginRight-->convert from userUnit to pixels as to marginRightPx
marginTop-->convert from userUnit to pixels as marginTopPx
marginBottom-->convert from userUnit to pixels as marginBottomPx

When media==paper and paper_type == "tiled":

### input_dom Data

#### Get input_dom's width and height, convert to pixels using svg unit if needed:

inputDomWidthStr  = input_dom's `<svg>` width attribute
inputDomHeightStr = input_dom's `<svg>` height attribute
inputDomSvgUnit = get 'mm', 'cm', or 'in' suffix string from inputDomWidthStr
inputDomWidth   = if inputDomSvgUnit not empty then strip 'mm', 'cm', 'in' suffix from inputDomWidthStr. Convert inputDomWidthStr to float
inputDomHeight  = if inputDomSvgUnit not empty then strip 'mm', 'cm', 'in' suffix from inputDomHeightStr. Convert inputDomHeightStr to float
inputDomWidthPx   = if inputDomSvgUnit empty then inputDomWidth else if inputDomSvgUnit=='cm' then inputDomWidth * 1in/2.54cm * 96px/in else if inputDomSvgUnit=='mm' then inputDomWidth * 1in/25.4mm * 96px/in else inputDomWidth * 96px/in
inputDomHeightPx  = if inputDomSvgUnit empty then inputDomHeight else if inputDomSvgUnit=='cm' then inputDomHeight * 1in/2.54cm * 96px/in else if inputDomSvgUnit=='mm' then inputDomHeight * 1in/25.4mm * 96px/in else inputDomHeight * 96px/in

#### Calculations:

trimTileWidthPx  = paperSizeWidthPx - marginLeftPx - marginRightPx
trimTileHeightPx = paperSizeHeightPx - marginTopPx - marginBottomPx

width1    = (inputDomWidthPx-marginLeftPx-marginRightPx) / trimTileWidthPx
tileCols  = floor(width1)
remainder = width1 - tileCols
if remainder > 0 then tileCols = (tileCols + 1)

height1   = (inputDomHeightPx-marginTopPx-marginBottomPx) / trimTileHeightPx
tileRows = floor(height1)
remainder = height1 - tileRows
if remainder > 0 then tileRows = (tileRows + 1)

layoutWidthPx  = (tileCols  * trimTileWidthPx)  + marginLeftPx + marginRightPx
layoutHeightPx = (tileRows * trimTileHeightPx) + marginTopPx  + marginBottomPx

### Create and display new layout_dom

// Create `<svg>` element

1. Create `<svg>` element as root and set id='layout', width=inputDomWidthPx, and height=inputDomHeightPx
   // Create `<defs>` marker
2. Create `<defs>` element
3. Create a `<marker>` element with id="tile", viewbox="0 0 `<trimTileWidthPx> <trimTileHeightPx>`", refX=0, refY=0, markerWidth=trimTileWidthPx, markerHeight=trimTileHeightPx,orient="auto-start-reverse"
4. `Create a <rect>` element with id="tileRect", x=0, y=0, width=trimTileWidthPx, height=trimTileHeightPx, fill=none, stroke=black, stroke-width=1 `; `
5. `add <rect> element to <marker>` element; add `<marker> element to ` `<defs>`; add `<defs> element `to `<svg>` root// Create background `<rect>` and content `<rect>`
6. Create `<g>` element with id='Rectangles'
7. Create `<rect>` element with id='backgroundRect', x=0, y=0, width=(inputDomWidthPx), height=(inputDomHeightPx), fill=white, stroke=black, stroke-width=1; add to `<g>` id='Rectangles'
8. Create `<rect>` element with id='contentRect', x=marginLeftPx, y=marginTopPx, width=(inputDomWidthPx-marginLeftPx-marginRightPx), height=(inputDomHeightPx-marginTopPx-marginBottomPx), fill=none, stroke=black, stroke-width=1; add to `<g>` id='Rectangles'
   // Create tiled rectangles
9. Create `<g>` element with id='tiledRects'
10. Create tiled rectangles as follows:minX = marginLeftPx
    ```
    minY = marginTopPx
    rowNum = 0
    for row in tileRows:
    	tileY = minY + (rowNum * trimTileHeightPx)
    	dstr = "M" // create path where each coord is upper left corner of marker
    	rowNum += 1
    	colNum = 0
    	for col in tileCols:
    		tileX = minX + (colNum * trimTileWidthPx)
    		if colNum == 1 then dstr += " L"
    		dstr += " <tileX>, <tileY>"
    		colNum += 1
    	// end for col in tileCols
    	// create one path per row, each coord starts a marker (tile rectangle)
    	create <path> element with id="row_rowNum", stroke="none", fill="none", marker-start="url(#tile)", marker-mid="url(#tile)", marker-end="url(#tile)", d=dstr"
    	add <path> element to <g> id='tileRects'
    // end for row in tileRows
    ```
11. add `<g>` id='tiledRects' to `<g>` id='Rectangles; Add `<g>` 'Rectangles' to `<svg>` root
    // Create layout_dom
12. layout_dom = `<svg>` root
    // create layoutWidthPx and layoutHeightPx
13. self.layoutWidthPx = inputDomWidthPx
    self.layoutHeightPx = inputDomHeightPx
    // display layout_dom
14. display layout_dom in the right canvas (no need to update the left canvas)

### End of Tiling calculation workflow
