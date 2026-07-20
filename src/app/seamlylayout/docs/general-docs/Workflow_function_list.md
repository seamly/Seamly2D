cxxqt_bridge\src\lib.rs 
rust state:
layout_dom
adjust_dom
piece_bboxes_json: json array created during process_layout(), updated 
piece_bboxes_json_snapshot: snapshot of piece_bboxes_json created in enter_adjust_mode(), used by discard_adjustments() and exit_adjust_mode()

// functions not converted to appController bridge
Xfn process_layout(): creates layout_dom, piece_bboxes_json

//these functions are converted to appController bridge in camelcase//
Xfn import_svg(): clears all doms, layout meta data, piece_bboxes_json
Xfn get_piece_bboxes():  stringifies layout_dom's piece_bboxes_json{"ml_px":0,"mt_px":0,"pieces":[{"id":"piece1","x":0,"y":0,"w":100,"h":100,"origin_x_px":50,"origin_y_px":50},...]}
Xfn save_adjust_dom(): save adjust_dom to output/adjust_canvas.svg

A
Xfn enter_adjust_mode(): creates adjust_dom, adjust_canvas.svg, piece_bboxes_json_snapshot
-> adjust_dom=layout_dom.clone()
-> save adjust_dom to output/adjust_dom.svg
->rust.adjust_dom = adjust_dom
->rust.piece_bboxes_json_snapshot = rust.piece_bboxes_json.clone()

fn accept_adjustments(self, transformsJson): updates piece_bboxes_json
->doc from rust.adjust_dom
->doc."<piece>".transform=transformsJson  (updates adjust_dom)
->rust.set_is_adjust_dirty=false
->rust.adjust_applied() -> 
->return true to onReplyRequested()


______________________________________________________
Legacy QML adjust canvas

Xfn resetInteractiveState(): clears root.dragOffsets, .selectedIds, .conflictIds, .undoStack, .redoStack, ._lastDdx, ._lastDdy, .zoomScale 

Xfn activateForAdjust():
->XappController.saveAdjustDom()     -- from appController.save_adjust_dom() -- save adjust_dom to adjust_canvas.svg
->XappController.getPieceBboxes(), -- from appController.get_layout_bboxes() -- get stringified piece_bboxes_json
->XpieceOverlayBboxModel.clear() 
->Xappend overlay bboxes to pieceOverlayBboxModel
->XresetInteractiveState()

Xfn buildTransform():
->for each moved overlay in pieceOverlayBboxModel
--> update moved overlay with current root.dragOffset.dx, .dy, .angle
-->retain ox, oy
--> calculate tx_px, tx_py as p.x + dx - ox, p.y + dy - oy
--> if angle>.001 calculate cx, cy piece center for angle>.001 and create transform "translate(tx_px, ty_py)  rotate(angle, cx, cy)"
--> else create transform  "translate(tx_px, ty_py)"
--> arr.append(id, transform)
->root.dragOffsets={}
->return one moved overlay's new transform string

fn collectBboxesJson():
-> for each overlay in pieceOverlayBboxModel
-->arr.push(id,x,y,w,h,ox, oy)
---->>>>Should we update the pieceOverlayBboxModel transform_str with transformsJson here?
-> return all overlays base data

___________________
Main.qml

*ApplicationWindow

fn makeExportFilename(ext,tiled):
->return ""
->return "<name.ext>" or "<name_tiled.ext>"

onCompleted():
->preferencesModel.load("settings/preferences.json"
->settingsModel.load("settings/default_settings.json"

**AppController

onErrorOccured(message):
->erroDialog.errorText=message
->errorDialog.open()

onImportFinished():
->leftCanvas.reloadSvg(appController.getImportDomString())
->rightCanvas.reloadSvg("")

onLayoutFinished(path):
->root.lastExportedDxfPath=path
->root.lastExportedPdfPath=path
->root.lastExportedPngPath=path
->preferencesModel.openInViewer(preferencesModel.pngViewerPath,path)
->preferencesModel.openInViewer(preferencesModel.pdfViewerPath,path)
->exportSuccessDialog.exportPath = path
->exportSuccessDialog.open();

1 START HERE:
signal onAdjustLayoutClicked:  
->appController.enterAdjustMode() -- from appController.enter_adjust_mode() in lib.rs -- -> create adjust_dom.svg, rust.adjust_dom, rust.piece_bboxes_json_snapshot
->adjustController.launchAdjustWindow()
->appController.saveAdjustDom()
->XappController.getPieceBboxes()

signal Connections:
fn onAdjustApplied():
->adjustController.launchAdjustWindow()
->appController.saveAdjustDom()
->XappController.getPieceBboxes()

fn onSvgLoaded():
->adjustController.closedAdjustWindow()

fn saveAdjustDom() -- converted from lib.rs.save_adjust_dom()
Xfn getPieceBboxes() -- converted from lib.rs.get_piece_bboxes() 
fn checkOverlaps()

**AdjustController

onApplyRequested(transformsJson):
->appController.checkOverlaps()
->appController.getPieceBboxes()
->emit acceptAdjustments(transformsJsson) -- from appController.accept_adjustments() in lib.rs

onSaveRequested():
->root.pendingAdjustClose true
->appController.exitAdjustMode() -- from appController.exit_adjust_mode()

onCancelRequested():
->adjustDiscardDialog.open()
->appController.discardAdjustments()


**TopMenuBar

onImportClicked()
->importDialog.currentFolder = preferencesModel.inputDirectory !== ""
->preferencesModel.localFileToUrl(preferencesModel.inputDirectory)
->preferencesModel.defaultInputFolderUrl()
->importDialog.open()


onAdjustLayoutClicked:
->appController.enterAdjustMode()
->adjustController.launchAdjustWindow()
->appController.saveAdjustDom()
->adjustController.getPieceBboxes()







______________________________________________________
AdjustScene.cpp

void loadLayout(): 
->clear()
->m_pieces.clear()
->m_background.clear()
->m_background.bg: new svg background layer 0
->setSceneRect(bg->boundingRect()) - fits scene to svg background
->bboxJson into jsonBytes into doc into root into pieces array
->new  PieceOverlayItem from each piece array
->m_pieces.append(item)

const Qstring getMovedTransform()
->buildTransform() for moved piece {id, transform}

______________________________________________________
AdjustController.cpp

void launchAdjustWindow(path, bboxJson):
->m_window=new AdjustWindow(path,bboxJson)
-> if user accepted (Apply or Enter) -> signal applyRequested
->if user Save -> signal saveRequested
-> if user Cancel -> signal cancelRequested
->m_window.show()
->m_window.raise()
->m_window.ActivateWindow()


______________________________________________________
AdjustWindow.cpp

void onApplyClicked():
->m_scene=getMovedTransform()
->m_scene->clearPieces()
->emit accepted(transforms)-->triggers onApplyRequested(transforms)


______________________________________________________
PieceOverlayItem.cpp

QString buildTransform():
tx=pos().x - m_initialPos.x();
ty=pox().y - m_initialPos.y();
angle = rotation()
return (translate(tx, ty) rotate(angle)) or (translate(tx,ty) or (rotate(angle)) as QString






buildTransform()->buildMovedTransform()->onApplyClicked
