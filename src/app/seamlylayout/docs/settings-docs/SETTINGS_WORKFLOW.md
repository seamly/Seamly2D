# SETTINGS WORKFLOW

- Prepopulate the settings fields with values from the default settings file specified in preferences.json; if no default settings file is specified then prepopulate the settings field values with the defaults specified below.

- Unit -- 'cm', 'mm', or 'in' radio buttons; Default is 'in'.
# Media Settings
- media -- 'paper' or 'fabric' radio buttons; default is 'paper'.
- if media = 'paper':
    > paperType - 'sheet', 'roll', 'tiled' radio buttons; default is 'sheet'.
    > if paperType='sheet':
      >> paperSize -- dropdown list of entries from /assets/paperSizes/paper_sizes, displaying the name, imperialDimensions, and metricDimensions values in 'Name', 'Inches', 'Millimeters' columns; default is 'Arch E'.
      >> paperWidth=paperSize's width converted to user's unit; default is 'Arch E' width 36.
      >> paperHeight=paperSize's height converted to user's unit; default is 'Arch E' height 48.
      >> Margins - marginLeft, marginRight, marginTop, marginBottom
        >>> if unit='cm' then default is 1 for all margins.
        >>> if unit='mm' then default is 10 for all margins.
        >>> if unit='in' then default is .25 for all margins.
    > if paperType='roll':
      >> rollSize --  dropdown list of roll paper widths (24 in, 32 in, 36 in, 40 in, 42 in, 297 mm, 420 mm, 432 mm, 594 mm, 841 mm);  default is '36 in'.
      >> paperWidth = rollSize's width converted from inches or millimeters into user unit; default is 36.
      >> paperHeight = 500 inches converted to user unit.
      >> Margins - marginLeft, marginRight, marginTop, marginBottom
        >>> if unit='cm' then default is 1 for all margins.
        >>> if unit='mm' then default is 10 for all margins.
        >>> if unit='in' then default is .25 for all margins.
    > if paperType='tiled':
      >> tileSize = dropdown list of paperSize restricted to Letter, Legal, Tabloid, A3, A4, A5; default is Letter.
      >> tileWidth = tileSize width converted to user units; default is 8.5.
      >> tileHeight = tileSize height converted to user units; default is 11.
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
    > fabricWidth -- width of fabric inluding selvedges; default is 0.
    > fabricHeight -- height of fabric including margins; default is 0.
    > selvedgeWidth -- default is 0.
    > fabricFold -- checkbox.
    > Margins - marginLeft, marginRight, marginTop, marginBottom
      >> if fabricFold is True then marginLeft=0 else marginLeft=SelvedgeWidth.
      >> marginRight=selvedgeWidth.
      >> marginTop=selvedgeWidth.
      >> marginBottom=selvedgeWidth.
# Layout Settings
- layoutMode -- 'alongGrainline' or 'withNap' radio buttons; default is 'alongGrainline'.
  All modes orient each pattern piece so the grainline points "up" (preprocessing).
  - alongGrainline: trial set {0°, 180°} — head-up or head-down.
  - withNap:        trial set is a singleton — every piece points the same direction.
- if layoutMode='withNap':
  >> Nap Direction -- 'Pieces point Up' (rotationStep=0) | 'Pieces point Down' (rotationStep=180);
     default is 'Pieces point Up'.
# Save/Load Settings
- settingSavepath -- opens a file-save dialog in the /settings directorywhen the user clicks the 'Save Settings' button, saves parameters to a file, can be a relative filepath, extension must be '.settings'.
- settingOpenpath -- opens a file-open dialog in the /settings directory when the user clicks the 'Load Settigs' button, loads parameters from a file, can be a relative filepath, extension must be '.settings'.

# Viewer Preferences (current)
- DXF viewer path -- optional. If empty, fall back to OS default.
- PDF viewer path -- optional. If empty, fall back to OS default.
- 3D viewer -- optional online viewer URL. Default is https://3mfviewer.com/.
