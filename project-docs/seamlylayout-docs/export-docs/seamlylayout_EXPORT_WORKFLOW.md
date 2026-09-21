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
