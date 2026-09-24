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
- The subset keeps all glyph slots and the full `cmap`, so a large font (for example a CJK UI font) still adds hundreds of KB.
- Check the font license before you share a mode 1 file.
