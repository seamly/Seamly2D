ExportMenu.MenuItem("PNG").onTriggered
  → ExportMenu.exportPngRequested()                          signal
    → TopMenuBar.onExportPngRequested                        signal relay
      → TopMenuBar.exportPngRequested()                      signal
        → Main.qml onExportPngRequested                      handler
          → preferencesModel.layoutDirectory                 getter → "C:/src/seamlyLayout/qt_frontend/output"
          → root.makeExportFileName("png")                   QML function → "<baseName>_YYYYMMDDHHMM.png"
            → appController.defaultExportFileName(base, segment, stamp, ext, tiled)   Rust exports::default_export_file_name
          → preferencesModel.getSaveFilePath(title, dir, name, filter)   C++ static
            → QDir(dir).absolutePath()                       resolve dir to absolute
            → QFileDialog.setDirectory(absDir)
            → QFileDialog.selectFile(defaultName)
            → QFileDialog.exec()                             user picks file, clicks Save
            → dlg.directory().absolutePath()                 get chosen directory
            → QFileInfo(selectedFiles().first()).fileName()  get just the filename
            → returns chosenDir + "/" + fileName             ← absolute path
          → appController.exportPng(path, 3.125)             CXX-Qt bridge call
            → path.to_string()                               QString → Rust String
            → self.rust().layout_dom.clone()                 clone the layout DOM
            → remove_piece_color_blocks(&mut layout_doc)             remove fill rects
              → remove_piece_color_blocks_rect(root_element)          recursive child walk
            → app_core::document_to_tree(&layout_doc, None)  DOM → usvg::Tree
            → app_core::render_png(&tree, Path::new(&path_str), scale)
              → Pixmap::new(w, h)                            allocate pixel buffer
              → pixmap.fill(Color::WHITE)                    white background
              → resvg::render(tree, transform, &mut pixmap)  rasterize SVG
              → pixmap.save_png(out_path)                    write PNG file to disk
            → self.export_finished(path_str)                 signal → QML
              → Main.qml onExportFinished(path)              handler
                → root.lastExportedPngPath = path            store for View menu
                → preferencesModel.openInViewer(pngViewerPath, path)  C++ static
                  → QProcess::startDetached(viewerPath, [filePath])
                  → (fallback) QDesktopServices::openUrl(filePath)

## SVG export — label text modes

```
ExportMenu SvgModeItem.onTriggered → chosen(mode) → exportSvgModeRequested(mode)
  → TopMenuBar.exportSvgRequested(mode)
    → Main.qml onExportSvgRequested(mode)
      → preferencesModel.saveSvgTextMode(iniPath, mode)   last mode, marked next time
      → pendingExportSettings = mode; exportStartTimer
        → appController.exportSvg(path, mode)
          → clone_stripped_layout_doc()
          → svg_label_text::apply_text_mode(doc, mode)    skipped for "asSupplied"
          → do_export_svg(doc, path)
          → export_warning(message)                       only when caveats exist
          → export_finished(path)                         success dialog shows caveats
```

| Mode | Use | Font dependency |
|---|---|---|
| Text: designer font | Tech packs, editing in Inkscape/Illustrator | Embedded subset when the font license allows |
| Text: single-line font | CAD/CAM tools that resolve text by font name | Embedded Relief SingleLine (OFL-1.1) |
| Paths: single-stroke (Hershey) | Plotters, cutters, engravers | None |

- `labelTextState` (`AppController`) gates the modes: `pathsOnly` disables them and shows "As supplied".
- The View menu keeps one SVG item.

### Font licensing caveat

- Mode 1 embeds the designer's font. Font licenses set embedding rights.
- Mode 1 never embeds a font whose OS/2 `fsType` is "restricted" or forbids subsetting. The text then names the font but does not carry it; the success dialog says so.
- A font that is not installed is not embedded either; the dialog names it.
- The subset keeps only the used glyphs, with a new Unicode `cmap` (`allsorts`). Trousers with Yu Gothic UI Light: 55 KB input, 65 KB export.
- Check the font license before you share a mode 1 file.

## HPGL export — plotters and cutters

```
ExportMenu HPGL submenu → exportHpglRequested("plot" | "cut")
  → TopMenuBar.exportHpglRequested(mode)
    → Main.qml onExportHpglRequested(mode)
      → save dialog, extension = preferencesModel.hpglExtension
      → pendingExportSettings = {"mode", "cutPen", "markPen", "labelPen"}; exportStartTimer
        → appController.exportHpgl(path, optionsJson)
          → exports::parse_hpgl_options(json)
          → clone_stripped_layout_doc()
          → exports::do_export_hpgl(doc, path, options)
            → svg_label_text::apply_text_mode(HersheyStrokes)   plot mode only
            → hpgl_writer::svg_to_hpgl(doc, options)
          → export_warning(message)                            missing Hershey glyphs only
          → export_finished(path)
```

- Dialect: HP-GL/1. Program: `IN;PA;`, then `SP n;` / `PU x,y;` / `PD x,y,...;`, then `PU;SP0;`.
- Units: 40 plotter units per mm; 96 px per inch. Y is flipped: HP-GL origin is bottom left.
- usvg resolves transforms, the viewBox, shapes and arcs. Curves are interpolated within 2 units (0.05 mm).
- `PD` holds at most 32 points; small plotter buffers reject longer instructions.

| Submenu item | Writes |
|---|---|
| Plot (all lines) | Labels, marks, then cut lines, each with its pen |
| Cut (cut lines only) | Cut lines only |
| Cut line pen / Mark pen / Label pen | Pen 1-8 per line class. Defaults 1 / 2 / 3 |
| File extension | `.plt` (default) or `.hpgl` |

- Pens and extension persist in the preferences INI: `hpgl_cut_pen`, `hpgl_mark_pen`, `hpgl_label_pen`, `hpgl_extension`.
- Cut lines go last, so a cutter frees a piece only at the end.
- A pen select is written only when the pen changes. Classes that share a pen do not swap pens.
- Within a class, the next path is the one whose start is nearest the pen.

| Line class | `data-type` |
|---|---|
| Cut | `cutline`, `cut_path` |
| Mark | `seamline`, `internal_path`, `grainline`, `notch`, `tuck`, `drill`, `hole` |
| Label | `piece_label`, `pattern_label`, `label_text` |

- A file without `data-type`: the group id decides (`cutline_*`, `*-notch-*`, ...).
- No hint at all: a closed path is a cut line, an open path is a mark.
- usvg drops `data-*`. `line_classes::tag_line_classes` wraps each classified group's children in `<g id="hpgl-class-<class>-<n>">`, which usvg keeps. Original ids do not change.
- View > HPGL: file picker (`*.plt *.hpgl`), then `openInViewer(hpglViewerPath, file)`.
- Preferences > HPGL Viewer: default `https://tiny-online.tools/embroidery-cnc-tools/hpgl-plt-viewer`. A web viewer opens its page only; the user loads the file there.
- A missing `hpgl_viewer_path` key (INI or defaults JSON) falls back to that URL. The installer also seeds the key.
- Not covered: HP-GL/2, a rotate option for the roll axis, sheet-mode pages.

## PostScript export — PS and EPS

```
ExportMenu EPS / PostScript (PS) → exportPostscriptRequested("eps" | "ps")
  → TopMenuBar.exportPostscriptRequested(flavor)
    → Main.qml onExportPostscriptRequested(flavor)
      → save dialog (*.eps or *.ps)
      → pendingExportFormat = flavor; exportStartTimer
        → appController.exportPostscript(path, flavor)
          → clone_stripped_layout_doc()
          → exports::do_export_postscript(doc, path, PsFlavor)
            → ps_writer::svg_to_postscript(doc, flavor)
          → export_warning(message)                    caveats only
          → export_finished(path)
```

- One writer serves both flavors. Only the header and the page setup differ.
- Units: 96 px per inch → 72 pt per inch (× 0.75). The page matrix flips Y.
- Each path keeps its own transform (`concat`), so stroke widths scale as in SVG.
- Curves stay curves: cubic → `curveto`, quadratic → the equal cubic.
- Text is written as glyph outlines. The file needs no fonts.
- Level 2. 7-bit ASCII. Path operators `m l c h` live in `SeamlyLayoutDict`, not `userdict`.

| Item | PS | EPS |
|---|---|---|
| First line | `%!PS-Adobe-3.0` | `%!PS-Adobe-3.0 EPSF-3.0` |
| `%%BoundingBox`, `%%HiResBoundingBox` | layout page | layout page |
| `setpagedevice` page size | yes | no (the importing program owns the page) |

| SVG feature | Written as | Warning |
|---|---|---|
| Opacity < 1 | opaque | yes |
| Gradient | first stop color | yes |
| Pattern fill | black | yes |
| Raster image | not written | yes |
| Clip path, mask, filter | ignored | yes |

- An empty layout is an error. No file is written.
- The View menu has no PS or EPS item. The system viewer opens the file.
- Not covered: tiled multi-page PS.
- Seamly2D exports no PS or EPS, and ships no `pdftops`. CLI formats 8 and 9 are rejected; other format numbers do not change.

## JPG export

```
ExportMenu JPG → exportJpgRequested()
  → TopMenuBar.exportJpgRequested()
    → Main.qml onExportJpgRequested
      → save dialog (*.jpg *.jpeg), default <baseName>_YYYYMMDDHHMM.jpg
      → pendingExportFormat = "jpg"; exportStartTimer
        → appController.exportJpeg(path)
          → clone_stripped_layout_doc()
          → exports::do_export_jpeg(doc, path, progress)      progress 10, 90
            → app_core::document_to_tree(doc, None)
            → app_core::render_jpeg(tree, path, 1.0, JPEG_DEFAULT_QUALITY)
          → export_finished(path)                             success dialog
```

- Same render as PNG: 100% scale (96 px per inch), white background.
- Encoder: `image` crate, `jpeg` feature only. Quality 90, fixed. No UI.
- JPEG has no alpha. The white fill makes every pixel opaque, so the alpha byte is dropped.
- Limit: 65535 px per side (about 17.3 m at 96 px/in). A larger layout is an error before render. No file is written.
- A finished JPG export shows the success dialog. It does not open a viewer.
- View > PNG/JPG: file picker (`*.png *.jpg *.jpeg`), then `openInViewer(pngViewerPath, file)`.
- Preferences label: "Image Viewer (PNG, JPG)". The INI key stays `png_viewer_path`.

## Paid formats — G-Code and 3D mesh (3MF)

Stubs only. No module ships yet.

- `exports::paid_export_available(format)` is the gate. It returns `false` for `"gcode"` and `"3mf"`.
- The Export menu shows "G-Code" and "3D Mesh (3MF)" only when the gate returns `true`.
- Chain: `ExportMenu` → `TopMenuBar` → `Main.qml` save dialog → `AppController.exportGcode` / `exportMesh` → `do_export_gcode` / `do_export_mesh`.
- Both `do_export_*` stubs return `Err` and write no file.
- To add a module: change the gate, then the stub body. QML needs no change.

## Default file names

`makeExportFileName(ext, tiled, segment)` → `<baseName>[_<segment>]_YYYYMMDDHHMM[_tiled].<ext>`.

| Export | Default name |
| ------ | ------------ |
| DXF-ASTM (R12) | `male_shirt_R12_202610031234.dxf` |
| DXF-ASTM (R13) | `male_shirt_R13_202610031234.dxf` |
| DXF-ASTM (CLO3D), R13 | `male_shirt_CLO3D_202610031234.dxf` |
| PDF tiled | `male_shirt_202610031234_tiled.pdf` |
| HPGL Cut (cut lines only) | `male_shirt_cutlines_202610031234.<plt\|hpgl>` |
| HPGL Plot | `male_shirt_202610031234.<plt\|hpgl>` |
| SVG single-line font | `male_shirt_singlelinefont_202610031234.svg` |
| SVG Hershey strokes | `male_shirt_hersheyfont_202610031234.svg` |
| SVG designer font, as supplied | `male_shirt_202610031234.svg` |
| Other formats | `male_shirt_202610031234.<ext>` |

- HPGL and SVG segments come from `appController.exportNameSegment(format, mode)` → Rust `exports::export_name_segment`.
- Export all tabs: tab label goes before the extension. Example: `male_shirt_cutlines_202610031234_size-40.plt`.
