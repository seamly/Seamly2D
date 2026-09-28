ExportMenu.MenuItem("PNG").onTriggered
  → ExportMenu.exportPngRequested()                          signal
    → TopMenuBar.onExportPngRequested                        signal relay
      → TopMenuBar.exportPngRequested()                      signal
        → Main.qml onExportPngRequested                      handler
          → preferencesModel.layoutDirectory                 getter → "C:/src/seamlyLayout/qt_frontend/output"
          → root.makeExportFileName("png")                   QML function → "<baseName>_YYYYMMDDHHSS.png"
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
- The View menu has no HPGL item: no HPGL viewer is configured.
- Not covered: HP-GL/2, a rotate option for the roll axis, sheet-mode pages.

## Paid formats — G-Code and 3D mesh (3MF)

Stubs only. No module ships yet.

- `exports::paid_export_available(format)` is the gate. It returns `false` for `"gcode"` and `"3mf"`.
- The Export menu shows "G-Code" and "3D Mesh (3MF)" only when the gate returns `true`.
- Chain: `ExportMenu` → `TopMenuBar` → `Main.qml` save dialog → `AppController.exportGcode` / `exportMesh` → `do_export_gcode` / `do_export_mesh`.
- Both `do_export_*` stubs return `Err` and write no file.
- To add a module: change the gate, then the stub body. QML needs no change.
