# Layout Orientation Modes (Current)

Author: slspencer — revised 2026-05-21

## Current status

Layout mode in Settings supports three options:

- `alongGrainline` (default)
- `withNap`
- `any`

The previous free-angle mode has been removed from the Settings dialog and the
mode parser. Legacy values are coerced safely to `alongGrainline`.

## Purpose

Document the current orientation behavior used by packing and layout assembly,
and the Settings semantics users can rely on.

## Mode behavior

All pieces are preprocessed so the grainline points upward.

- `alongGrainline`
  - Trial set: `{0°, 180°}`
  - Meaning: piece can flip head-up/head-down while staying on-grain.

- `withNap`
  - Trial set: singleton `{rotationStep°}` where `rotationStep ∈ {0, 180}`
  - Meaning: all pieces point the same direction.
  - `0` = Pieces point Up, `180` = Pieces point Down.

- `any`
  - Default trial set: `{0°, 90°, 180°, 270°}` for legacy handoffs without a Piece Properties > Layout rotation rule.
  - When present, each piece's `data-rotation` rule restricts its own angles:
    `0`/`360` → `{0°}`, `180` → `{0°, 180°}`, `90` → quarter turns, and `1` → 10° steps from 0° through 350°.
  - `data-rotation="0"` means no rotation; omission preserves the legacy quarter-turn fallback.
  - A piece wider than the media at its rule's angles gets quarter turns if a 90° turn fits (`piece_extractor::turn_wide_pieces_to_fit`). Tiled paper: no change.
  - The grainline is ignored after the existing grain-up preprocessing. Any samples 10° steps (`pack_types::ANY_ROTATION_STEP_DEG`), not continuous optimization.
  - Routed to `layout_engine::pack_maxrects_multi_angle`; the selected angle is applied to the full piece group.

## Settings model

### UI

- **Layout Mode**: `alongGrainline` | `withNap` | `any`
- **Nap Direction** (visible only when mode is `withNap`):
  - Pieces point Up (`rotationStep = 0`)
  - Pieces point Down (`rotationStep = 180`)

### JSON fields

- `layout_mode: String` (default `"alongGrainline"`)
- `rotation_step: f64` (default `0.0`)

### Legacy migration

On load:

- `withGrain` + `rotationEnabled=false` → `withNap` + `rotationStep=0`
- `withGrain` + `rotationEnabled=true`  → `alongGrainline`
- `withoutGrain` + (any)                → `alongGrainline`
- unknown mode values                    → `alongGrainline`

## Trial-set mapping

`LayoutSettings::rotation_trial_set_deg()` resolves to:

- `alongGrainline` → `[0, 180]`
- `withNap` + step≈0   → `[0]`
- `withNap` + step≈180 → `[180]`
- `any`                → `[0, 90, 180, 270]`
- unknown/legacy       → `[0, 180]`

## Packing and assembly notes

- Packing consumes the trial set produced above.
- Explicit Piece Properties rules apply only in `any`; `rotationStep` remains the Nap Direction setting.
- `Placed.rotation_deg` remains part of the placement record.
- Layout assembly applies translation and orientation transform to each piece
  group so fills and child elements remain aligned with the piece.

## Notes for future updates

- If a new orientation mode is reintroduced later, update:
  - `qt_frontend/qml/SettingsDialog.qml`
  - `qt_frontend/src/SettingsModel.{h,cpp}`
  - `crates/layout_tiling/src/layout_settings.rs`
  - this document and `docs/settings-docs/SETTINGS_WORKFLOW.md`
