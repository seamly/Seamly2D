# Adjust Mode QGraphicsScene Workflow

**SeamlyLayout — Adjust Mode Overlay and Transform Workflow**
Author: slspencer
Copyright: 2026

---

## Overview

This document describes the workflow for managing overlays (QGraphicsRectItem, `PieceOverlayItem`) and SVG transforms in SeamlyLayout's adjust mode. It details how overlays are created, updated during user interaction, and how transforms are applied and serialized.

---

## 1. Overlay Creation and Initial Positioning

- **File:** `AdjustScene.cpp`
- **Function:** `AdjustScene::loadLayout`
  - Reads the SVG and bounding box JSON.
  - For each piece, creates a `PieceOverlayItem` at its original (x, y), with width, height, origin, and rotation.
  - Adds each `PieceOverlayItem` to the scene and to `m_pieces`.
  - **Overlay position is set to the original (x, y) from JSON.**

---

## 2. During Move (Drag Interaction)

- **File:** `PieceOverlayItem.cpp`
- **Functions:** `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent`
  - When the user drags a piece, the overlay’s position is updated in real-time using `setPos()` for visual feedback.
  - This change is **temporary** and only for user interaction.

---

## 3. After Move (Apply/Accept Action)

- **File:** Typically in `AdjustWindow.cpp` or a controller/QML handler
- **Function:** (e.g., `onApplyClicked`, `acceptAdjustments`)
  - When the user confirms the move (presses Enter or clicks Apply):
    - The move delta is converted into a new SVG `transform` string (e.g., `translate(...)` or `rotate(...)`).
    - This transform is **appended** to the SVG element’s `transform` attribute (using xmltree/svg_dom).
    - The overlay’s QGraphicsItem position **must be reset** to the original (x, y) so that only the SVG transform accumulates the move.
    - This is where `applyTransformAndReset(newTransform)` should be called for each moved piece.

---

## 4. Reloading the Scene

- **File:** `AdjustWindow.cpp`
- **Function:** `AdjustWindow::reload`
  - Calls `m_scene->loadLayout(svgPath, bboxJson);`
  - This clears and recreates all overlays at their original (x, y) positions.
  - The SVG background is reloaded, and overlays are re-instantiated.

---

## 5. Clearing Overlays

- **File:** `AdjustScene.cpp`
- **Function:** `AdjustScene::clearPieces`
  - Removes and deletes all `PieceOverlayItem` overlays from the scene.
  - Called after applying adjustments or when reloading.

---

## 6. Serializing Transforms

- **File:** `AdjustScene.cpp`
- **Function:** `AdjustScene::collectTransformsJson`
  - Iterates over all `PieceOverlayItem` overlays.
  - For each moved piece, calls `buildTransform()` to get the new transform string.
  - Serializes these transforms to JSON for saving or further processing.

---

## Summary Table

| Step                 | File/Function                          | What Happens                             |
| -------------------- | -------------------------------------- | ---------------------------------------- |
| Create overlays      | `AdjustScene::loadLayout`            | Overlays created at original (x, y)      |
| Drag overlays        | `PieceOverlayItem` mouse events      | Overlay moves visually via `setPos()`  |
| Apply move           | (Controller/UI handler)                | Transform appended to SVG, overlay reset |
| Reload scene         | `AdjustWindow::reload`               | Overlays recreated at original (x, y)    |
| Clear overlays       | `AdjustScene::clearPieces`           | Overlays removed from scene              |
| Serialize transforms | `AdjustScene::collectTransformsJson` | New transforms collected for saving      |

---

## Key Points

- **Overlay QGraphicsItem position is only for visual feedback during drag.**
- **All permanent moves/rotations are stored in the SVG `transform` attribute.**
- **After apply, overlays must be reset to original (x, y) to avoid double transforms.**
- **Scene reload always recreates overlays at original positions.**
- **The overlay class is `PieceOverlayItem`.**

---

_Last updated: 2026-04-04_
