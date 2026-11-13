# SETTINGS WORKFLOW

- Prepopulate settings from the default settings file specified in `qt6_seamlylayout.ini`.
- Use the defaults below when the INI file does not specify a default settings file.

- Unit -- 'cm', 'mm', or 'in' radio buttons; Default is 'in'.
# Media Settings
- media -- 'paper' or 'fabric' radio buttons; default is 'paper'.
- if media = 'paper':
    > paperType - 'tiled', 'roll' dropdown; default is 'tiled'. A saved 'sheet' or 'custom' loads as 'tiled'.
    > if paperType='roll':
      >> rollSize --  dropdown list of roll paper widths (36 in, 48 in, 60 in, 72 in, 900 mm, 1200 mm, 1500 mm, custom);  default is '36 in'.
      >> 'custom' shows a Roll Width field: the user enters the width in the user unit. rollSize saves as 'custom'.
      >> paperWidth = rollSize's width converted from inches or millimeters into user unit, or the custom width; default is 36.
      >> paperHeight = 500 inches converted to user unit.
      >> Margins - marginLeft, marginRight, marginTop, marginBottom
        >>> if unit='cm' then default is 1 for all margins.
        >>> if unit='mm' then default is 10 for all margins.
        >>> if unit='in' then default is .25 for all margins.
    > if paperType='tiled':
      >> tileSize -- dropdown of 27 predefined sizes with imperial and metric dimension labels, then 'Custom'; default is Letter.
      >> Sizes include Letter, Legal, Ledger, ANSI C–E, ARCH A–E/E1–E3, ISO A0–A5, and ISO B0–B6. None is not a selectable tile size.
      >> 'Custom' shows Custom Size fields: the user enters the tile width and height in the user unit.
        >>> Selecting 'Custom' copies the current predefined size into the fields: imperial for 'in', metric for 'mm' and 'cm'.
        >>> The fields reject a value that is not above zero.
      >> Settings store the size name, not the dimension label. Tile orientation applies to every size, 'Custom' included.
      >> tileWidth, tileHeight = the custom tile size in user units; default is 8.5 × 11. A unit change converts them.
        >>> Used only when tileSize = 'Custom'. A predefined tileSize takes its dimensions from the size table.
      >> Margins - marginLeft, marginRight, marginTop, marginBottom
        >>> if unit='cm' then default is 1 for all margins.
        >>> if unit='mm' then default is 10 for all margins.
        >>> if unit='in' then default is .25 for all margins.
      >> trimTileWidth = tileWidth - marginLeft - marginRight
      >> trimTileHeight = tileHeight - marginTop - marginBottom
      >> inputDomWidth = inputDom's <svg> width converted to user units.
      >> inputDomHeight = inputDom's <svg> height converted to user units.
      >> numTilesWide = modulo(inputDom.width/trimTileWidth) + roundUp(remainder(inputDomWidth/trimTileWidth))
      >> numTilesHigh = modulo(inputDom.height/trimTileHeight) + roundUp(remainder(inputDomHeight/trimTileHeight))
      >> pageWidth = numTilesWide * trimTileWidth + marginLeft + marginRight
      >> pageHeight = numTilesHigh * trimTileHeight + marginTop + marginBottom
  - if media='fabric':
    > fabricWidth -- width of fabric including selvedges; default is 0 (then pageWidth).
      >> if fabricFold is True then the canvas is fabricWidth / 2: pieces go on the folded half.
    > fabricHeight -- length of fabric including margins; default is 0: no fixed length (500 in, trimmed after packing). No field in the Settings dialog.
    > The page size (pageWidth, pageHeight) does not apply to fabric.
    > selvedgeWidth -- default is 0.
    > fabricFold -- checkbox.
    > Margins - marginLeft, marginRight, marginTop, marginBottom
      >> if fabricFold is True then marginLeft=0 else marginLeft=SelvedgeWidth.
      >> marginRight=selvedgeWidth.
      >> marginTop=selvedgeWidth.
      >> marginBottom=selvedgeWidth.
# Layout Settings
- layoutMode -- 'alongGrainline', 'withNap' or 'any' radio buttons; default is 'alongGrainline'.
  All modes orient each pattern piece so the grainline points "up" (preprocessing).
  - alongGrainline: trial set {0°, 180°} — head-up or head-down.
  - withNap:        trial set is a singleton — every piece points the same direction.
  - any:            per-piece Piece Properties > Rotation applies; missing legacy rules use {0°, 90°, 180°, 270°}.
                    `Any` samples integer angles from 0° to 359°. `rotationStep` remains unused.
- if layoutMode='withNap':
  >> Nap Direction -- 'Pieces point Up' (rotationStep=0) | 'Pieces point Down' (rotationStep=180);
     default is 'Pieces point Up'.
- A multisize pattern has no setting of its own. Non-tiled paper makes the Marker, Nested,
  Base Size and size tabs with the same settings. Tiled paper omits Marker and Nested.
  A stored `multisizeLayout` key is ignored.
- if layoutMode is not 'any':
  >> No Grainline -- 'Keep upright' (noGrainlineRotation='upright') | 'Rotate freely' ('free');
     default is 'Keep upright'.
     Applies only to pieces with no drawn grainline and no data-grainline-angle.
     'Keep upright' keeps the drafted orientation and the layoutMode trial set.
     'Rotate freely' also lets the packer try 90° and 270° for those pieces.
- foldLinesOnLeftEdge -- checkbox "Place fold lines on the left edge (fabric: no left margin)"; default off.
  Every media.
  >> Each piece with a fold line lies with its fold line on the left content edge.
  >> A fold line across or aslant the grain turns its piece; the fold edge outranks the layout mode.
  >> layoutMode='any' ignores this option: no fold line goes on the edge, and no conflict is reported.
     Fabric keeps marginLeft unless fabricFolded.
  >> media='fabric': marginLeft=0; the left edge is the fold.
  >> media='paper': marginLeft is kept; the fold line lies on the left content edge.
  >> A Nested piece-set: its sizes stack on the lower end of their fold line, so the stack goes on the edge when the sizes share one fold line; else a warning.
# Save/Load Settings
- settingSavepath -- opens a file-save dialog in the /settings directorywhen the user clicks the 'Save Settings' button, saves parameters to a file, can be a relative filepath, extension must be '.settings'.
- settingOpenpath -- opens a file-open dialog in the /settings directory when the user clicks the 'Load Settigs' button, loads parameters from a file, can be a relative filepath, extension must be '.settings'.

# Viewer Preferences (current)
- DXF viewer path -- optional. If empty, fall back to OS default.
- PDF viewer path -- optional. If empty, fall back to OS default.
- PNG/JPG viewer path -- optional. If empty, fall back to OS default.
- 3D viewer -- optional online viewer URL. Default is https://3mfviewer.com/.
