## Process Layout Async Plan

This plan describes how to convert the current synchronous `process_layout()` workflow
into a background task so the UI remains responsive and status messages render immediately.

### Goals

- Run all heavy layout processing off the UI thread.
- Preserve current workflow and output behavior.
- Keep DOM + canvas rendering (no SVG handles).
- Keep code readable and minimize bloat.

### Constraints

- The UI thread must not mutate state from inside the async task.
- The async task should operate on cloned inputs and return a single result struct.
- All UI-visible state updates must occur in `update()` when the task finishes.
- Doxygen-compatible comments are required for new functions.
- Inline comments are required where non-obvious logic exists.
- Add a comment to the closing brace for all `for`, `while`, `match`, and `if` statements.

### Plan of Action

1) **Define task input/output structs**
   - `ProcessLayoutInput`: cloned `input_dom`, `layout_dom`, `layout_settings`.
   - `ProcessLayoutResult`: `flat_dom`, `vertical_dom`, `translated_dom`, `layout_dom`,
     `layout_flat_dom`, `bounding_boxes`, and updated `layout_settings` (for tiled pruning).

2) **Create a pure compute function**
   - Add `process_layout_compute(input: ProcessLayoutInput) -> Result<ProcessLayoutResult, String>`.
   - Move the current pipeline logic from `process_layout()` into this function.
   - Replace any direct UI-side messages with debug logging only.
   - Use local variables, and return all computed DOMs and bounding boxes.

3) **Refactor helpers to be compute-friendly**
   - Add `flatten_dom_for(source_dom: &Document) -> Document`.
   - Add `verticalize_dom_for(flat_dom: &Document, bounding_boxes: &mut Vec<PieceInfo>) -> Document`.
   - Add `translate_dom_for(flat_dom: &Document) -> Document`.
   - Add `place_pieces_for(flat_dom: &Document, layout_dom: &Document, settings: &LayoutSettings,
     bounding_boxes: &mut Vec<PieceInfo>) -> Document`.
   - Add `prune_empty_tiled_rows_for(layout_flat_dom: &mut Document, settings: &mut LayoutSettings)`.
   - These are pure (no `self`) and return new values rather than mutating UI state.

4) **Wire async execution**
   - In `Message::ProcessLayout`, capture inputs and return
     `Command::perform(async move { process_layout_compute(input) }, Message::ProcessLayoutFinished)`.
   - Keep immediate status updates in `Message::ProcessLayout`.

5) **Apply results on completion**
   - In `Message::ProcessLayoutFinished`, update `self.*` fields from the result.
   - Then call `display_dom()`, `update_status()`, and `enable_export()`.
   - Set `self.last_action = "Layout processed."` and `self.settings_applied = false`.
   - On error, set `last_action/right_canvas_msg` to failure text.

6) **Retire the old synchronous path**
   - Remove or reduce `process_layout()` to a thin wrapper or delete it if unused.
   - Ensure there are no remaining direct calls to the old synchronous workflow.

7) **Compile and fix warnings**
   - Confirm no new warnings.
   - If any new warnings appear, resolve them in the same pass.

### Testing Checklist

- Click “Process Layout” and confirm:
  - UI remains responsive (no “Not Responding”).
  - `Last action` updates immediately to “Processing Layout...please wait.”
  - Final output renders and `Last action` becomes “Layout processed.”
- Verify output SVGs are still written (`flat1_dom.svg`, `vertical_dom.svg`, etc.).
- Confirm tiled media still prunes empty rows and updates page height.

