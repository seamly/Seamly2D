# Plan: Simultaneous Layer 0 + Layer 1 Transform Application

---

## Current Flow (problem)

User drag → QML emits transforms JSON
           → Rust accept_adjustments() updates SVG DOM (layer 0)
           → adjust_applied() signal
           → QML reloads SVG canvas from output/adjust_canvas.svg
           → AdjustScene does full scene reload (destroys/recreates overlays)
Layer 1 overlays are destroyed and recreated from scratch on every apply. They never "know" about applied transforms — they always start fresh from the original bbox positions.

---

## Proposed Flow

User drag → QML emits transforms JSON
           → Rust accept_adjustments():
               1. Updates SVG DOM transform attrs (layer 0) ← existing
               2. Extracts accumulated transform data (translate, rotate, scale) for each piece from the updated DOM — does not update x or y values ← new
               3. Emits piece_transforms_updated(transforms_json) ← new signal
           → AdjustWindow receives signal:
               1. Tells QML canvas to reload SVG (layer 0 visual refresh)
               2. Calls AdjustScene::updateOverlayTransforms(transforms_json) so overlays are in sync with the SVG pieces in layer 0 ← new
           → Each PieceOverlayItem repositions to its correct position based on its original x,y postion because the QT canvas SVG display actions apply the overlay's transforms to the original x,y automatically. We don't need to recalculate overlay x or y attributes as long as we keep the transforms current.

---

## Changes Required

1. **Rust — crates/cxxqt_bridge/src/lib.rs**

   In `accept_adjustments()`, after updating the DOM transform attrs:

   - Extract the accumulated transform data (translate, rotate, scale) for each piece from the updated DOM — **not** recalculated bboxes, just the transform values themselves
   - Build a JSON payload mapping piece IDs → their current transform data (e.g. `{ "id": "piece_1", "transforms": { "translate": [dx, dy], "rotate": angle, "scale": [sx, sy] } }`)
   - Emit `piece_transforms_updated(transforms_json)` signal alongside `adjust_applied()`
   - Do **not** update x or y values in piece_bboxes_json — the original x,y positions remain the canonical base

2. **C++ — AdjustWindow.cpp/.h**

   - Connect new `piece_transforms_updated` signal from the Rust bridge
   - On receive: first tell QML canvas to reload SVG (layer 0 visual refresh), then call `m_adjustScene->updateOverlayTransforms(transforms_json)` to sync layer 1

3. **C++ — AdjustScene.cpp/.h**

   New method `updateOverlayTransforms(transforms_json)`:

   - Parse the transforms JSON
   - For each `PieceOverlayItem*` in `m_pieces`: find its entry by ID, call `item->applyTransforms(translate, rotate, scale)`
   - No scene clear, no item recreation — overlays stay alive and just update their QGraphicsItem transforms

4. **C++ — PieceOverlayItem.cpp/.h**

   New method `applyTransforms(translate, rotate, scale)`:

   - Set the item's QGraphicsItem transform to match the piece's accumulated transforms (using `setTransform()` or the individual `setRotation()`, `setScale()` calls)
   - The item's `pos()` stays at the original x,y from initial creation — Qt applies transforms relative to that base automatically
   - Reset internal delta tracking so the next drag starts from zero relative to the new transformed state

---

## Key Decisions

1. **Original x,y values are never modified.** The Proposed Flow relies on Qt's built-in behavior: `QGraphicsItem::setTransform()` applies transforms relative to the item's `pos()`. As long as `pos()` stays at the original bbox position and the transform data stays current, overlays and SVG pieces stay in sync. No bbox recalculation needed.
2. **The comment in lib.rs:1100 about not updating piece_bboxes_json is correct and compatible.** Since we only pass transform data (not updated positions), `piece_bboxes_json` retains the original base positions permanently. "Apply, move more, Apply again" works because each Apply accumulates transforms in the DOM, and the emitted `piece_transforms_updated` signal carries the full accumulated transform — each subsequent drag is a delta on top of that.
3. **Transform-only signal vs. full bbox signal.** The signal should carry transform data, not recalculated bboxes. This avoids the cost and potential inaccuracy of re-extracting bboxes from the transformed DOM, and aligns with the principle that overlays keep their original x,y and let Qt handle the rest.
