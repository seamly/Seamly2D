# TODO — Remove unused (dead) layout code

Tasks for removing unsued code due to implementing SeamlyLayout, and orphaning the old layout code.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Check off all completed tasks & subtasks and move completed tasks to TODO_COMPLETED.md

All TODO_MIGRATE.md tasks begin with 'Dead.' and all tasks are numbered.

## TASK Dead.1 - Remove the orphaned layout code; orphaned when calling SeamlyLayout instead of the previous layout code and workflow

- [x] Dead.1.1 - locate the seamly2d code (vlayout*.cpp, etc.) that was orphaned by calling the new seamlylayout.exe
  - Layout Mode's built-in canvas (tempSceneLayout, scenes, showLayoutPages(),
    layout_ToolBox page, layoutPages_DockWidget) — no longer reached from
    showLayoutMode(). Kept in place: still shared with "New Print Layout"
    (toolLayoutSettings()) and CLI export (MainWindow::DoExport()).
  - GUI "Export Layout" (Tools -> Layout -> Export Layout menu, its toolbar
    entry, its toolbox button, and the "Layout" toolbar dropdown's Export
    Layout option) — fully orphaned; File -> Export As... called the same
    exportLayoutAs() and is the one surviving entry point (kept per decision
    below).
  - CLI headless `-e`/`--export` (vcmdexport.cpp/.h) and the vlayout nesting
    engine it depends on (VLayoutGenerator, vlayoutpaper, vbank, vcontour,
    vposition, vbestsquare) — NOT orphaned: DoExport() still runs them.
  - VLayoutPiece/VLayoutPiecePath — NOT orphaned: used by
    preparePiecesForLayout()/generatePiecesSvgDocument() to build the SVG
    sent to SeamlyLayout.
  - Decisions recorded: remove the GUI Export Layout menu/toolbar/toolbox
    entries; keep File -> Export As... for now; keep CLI export until
    SeamlyLayout's crates/cli reaches parity.
- [x] Dead.1.2 - remove the located orphaned code (GUI Export Layout entry points only)
  - Removed: exportLayout_ToolButton, exportLayout_Action (menu + toolbar),
    the handleLayoutMenu() popup's Export Layout option, and their wiring in
    mainwindow.cpp/mainwindow.ui.
  - Not removed (still required by CLI export / New Print Layout / Export
    Pieces / Export Draft Blocks): ExportLayoutDialog, AbstractLayoutDialog,
    LayoutSettingsDialog, DialogLayoutProgress, ExportProgressDialog,
    exportLayoutAs()/LayoutSettings()/ExportData()/ContinueIfLayoutStale()/
    preparePiecesForLayout(), and all of src/libs/vlayout.
  - Remaining orphaned-code work (not done here): migrate CLI `--export` to
    SeamlyLayout's crates/cli, then remove vcmdexport.cpp/.h and the vlayout
    nesting engine (vlayoutgenerator/vlayoutpaper/vbank/vcontour/vposition/
    vbestsquare); decide File -> Export As...'s fate once that lands.
- [x] Dead.1.3 - remove the tests for the located orphaned code
  - No test code (Seamly2DTest, CollectionTest, ParserTest) references
    export_layout_dialog, layoutsettings_dialog, abstractlayout_dialog,
    ExportLayoutDialog, LayoutSettingsDialog, or exportLayoutAs. Nothing to
    remove for this pass.
- [ ] Dead.1.4 - check that tests are run for SeamlyLayout during the linux-test job in ci.yml

## Task Dead.2 - Test the build pipeline -- ci.yml

## Task Dead.3 - Test the functionality (requires install & user testing)
