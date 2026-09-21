# Layout Orientation Modes (Current)

Author: slspencer — revised 2026-05-21

## Current status

Layout mode in Settings now supports two options only:

- `alongGrainline` (default)
- `withNap`

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

## Settings model

### UI

- **Layout Mode**: `alongGrainline` | `withNap`
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
- unknown/legacy       → `[0, 180]`

## Packing and assembly notes

- Packing consumes the trial set produced above.
- `Placed.rotation_deg` remains part of the placement record.
- Layout assembly applies translation and orientation transform to each piece
  group so fills and child elements remain aligned with the piece.

## Notes for future updates

- If a new orientation mode is reintroduced later, update:
  - `qt_frontend/qml/SettingsDialog.qml`
  - `qt_frontend/src/SettingsModel.{h,cpp}`
  - `crates/layout_tiling/src/layout_settings.rs`
  - this document and `docs/settings-docs/SETTINGS_WORKFLOW.md`
