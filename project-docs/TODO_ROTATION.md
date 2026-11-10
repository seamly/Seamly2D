# TODO — Piece Properties rotation in Layout Mode Any

Source: [TEST_APPLICATION_UPDATES.md, section 5](TEST_APPLICATION_UPDATES.md#5-seamly2d--flip-and-placement-properties).

Status: implementation plan only. No rotation behavior is implemented by this document.
All implementation and verification boxes remain open.

Tasks use the `Rotation.` prefix. Check completed subtasks individually.
Move fully completed tasks to [TODO_COMPLETED.md](TODO_COMPLETED.md).

## Scope and constraints

- Apply each piece's **Piece Properties > Rotation** only when SeamlyLayout's `layoutMode` is `any`.
- Preserve `alongGrainline` and `withNap`, including nap direction and the existing No Grainline exception.
- The checklist calls the first mode “With Grainline”; the current UI calls it “Along Grainline”.
- Distinguish layout **Any** from piece Rotation **Any**. The first enables the policy; the second supplies a 1-degree step.
- Do not add a rotation-enabled checkbox. `rotationEnabled` exists only in legacy settings migration.
- Do not change Piece Mode display, draft geometry, pattern schemas, Flip, Tilt, or DXF rule serialization.
- Preserve grain-up preprocessing. Trial angles are additional SVG rotations from that baseline, not absolute draft angles.
- Do not restore the retired free-angle layout mode or introduce polygon nesting.
- Leave `TEST_APPLICATION_UPDATES.md` unchanged. Installed-app acceptance remains a separate user check.

## References and verified code

Read these references before implementation:

- [Code styles](../.github/README-CODE-STYLES.md), [build and test rules](../.github/README-BUILDS.md).
- [SVG attribute contract](docs-data/SVG-DATA-ATTRIBUTES.md) and [attribute specification](docs-data/NEW-ATTRIBUTES.csv).
- [Current rotation behavior](docs-seamlylayout/layout-docs/seamlylayout_LAYOUT_ROTATION_PLAN.md).
- [Layout processing](docs-seamlylayout/layout-docs/seamlylayout_PROCESS%20LAYOUT%20WORKFLOW.md).
- [Settings workflow](docs-seamlylayout/settings-docs/seamlylayout_SETTINGS_WORKFLOW.md).
- [Existing Rotation task, Layout.95](docs-seamlylayout/TODO_SEAMLYLAYOUT.md).
- Applicable `.claude\rules\seamlylayout*.md`: architecture, handoff, layout, settings, SVG processing, Rust/Qt styles, and testing.

Repository-relative paths below identify the implementation points:

| Area | Evidence and consequence |
| --- | --- |
| Seamly2D properties | `src\libs\vtools\dialogs\tools\piece\pattern_piece_dialog.cpp` reads/writes `PieceRotation` through `VPieceLabelData`. This is not the label angle formula. |
| Stored values | `src\libs\vpatterndb\floatItemData\vpiecelabeldata.cpp`, `rotationStepDegrees()`: None → 0, 1-Way → 360, 2-Way → 180, 4-Way → 90, Any → 1. |
| Active producer | `src\libs\layout_handoff\handoff_document.cpp`, `HandoffPieceInfo::create()` and `HandoffDocument::to_dom()`, emit `data-rotation` for positive steps, even without visible labels. None is omitted. |
| Existing coverage | `src\test\Seamly2DTest\tst_vpiece.cpp` covers stored values and degree mapping. `tst_svgcomponenttags.cpp` covers old-writer attributes; extend `tst_handoffdocument.cpp` for the active writer. |
| Global policy | `src\app\seamlylayout\crates\layout_tiling\src\layout_settings.rs`, `rotation_trial_set_deg()`, returns `[0,90,180,270]` for `any`. `rotation_step` controls nap direction only. |
| Missing consumer | `crates\cxxqt_bridge\src\piece_extractor.rs`, both extractors, create `PieceRect`/`Rect` without reading `data-rotation`. Crate paths in this table are relative to `src\app\seamlylayout`. |
| Shared packing | `crates\pack_types\src\lib.rs` stores `Rect` and `Placed`. `crates\layout_engine\src\lib.rs`, `piece_trial_set()`, currently combines global angles, `free_rotation`, and `left_edge_angle`. |
| Existing arbitrary-angle support | `rotated_rect_dims()` already calculates conservative rotated AABBs. `maxrects_order_search.rs`, `PackJob::new()`, caches fitting orientations and deduplicates equal dimensions. |
| Actual backend | `crates\packing\src\lib.rs` routes both rectangle and polygon entry points to MaxRects. Some comments still describe polygon routing; follow executable code. |
| Additional consumers | `crates\cxxqt_bridge\src\layout_utils.rs`, `sheets.rs`, `piece_divider.rs`, `oversized.rs`, `remaining.rs`, and `crates\layout_tiling` perform packing or fit decisions. |
| Rendering | `layout_assembler.rs`, `oversized.rs`, and `remaining.rs` already compose arbitrary-angle rotation and rotated-AABB-minimum translation. Verify these paths rather than replacing them unnecessarily. |

## Proposed rotation contract

This table applies only to `layoutMode="any"`, after existing grain-up normalization.
Normalize angles to integers in `[0,359]`; prefer ascending angles, beginning with 0.
“Free rotation” means all 360 integer-degree candidates, not continuous-angle optimization.

| Piece Rotation | Handoff value | Proposed trial angles |
| --- | --- | --- |
| None | Absent on a tagged piece | `[0]` |
| 1-Way | `360` | `[0]`; do not store 360 as a distinct orientation |
| 2-Way | `180` | `[0,180]` |
| 4-Way | `90` | `[0,90,180,270]` |
| Any | `1` | Every integer angle from 0 through 359 |
| Untagged SVG without a rule | Absent | Existing fallback `[0,90,180,270]` |
| Invalid explicit value | Empty, nonnumeric, nonfinite, unsupported step | `[0]` with a piece-named warning |

**Decision gate:** the attribute contract does not define the nesting meaning of omitted Rotation.
The conservative proposal treats tagged None as no additional rotation.
This also affects older tagged SVGs without `data-rotation`; omission cannot distinguish their origin.
Confirm this policy before implementation, or explicitly select the existing quarter-turn fallback for omitted values.
Do not silently treat None as piece Any.

Fold-edge placement remains a geometric constraint, not permission to bypass Rotation.
In Any, choose an allowed angle that keeps the fold line on the left edge.
If none exists, report the conflict and leave the piece unplaced.
Do not introduce a mirror to evade Rotation. Preserve existing non-Any fold behavior.

## [ ] Task Rotation.1 — Pin the policy and compatibility rules

**Why:** omission, angle origin, and fold precedence must be explicit before changing packing.
**Dependencies:** none.

- [ ] Rotation.1.1 Resolve the omission decision above; record the selected behavior in this document and policy tests.
- [ ] Rotation.1.2 Encode the table as data-driven tests for every mode, every piece value, and invalid input.
- [ ] Rotation.1.3 Assert that `alongGrainline`, nap-up, nap-down, unknown modes, and legacy settings retain their current results.
- [ ] Rotation.1.4 Keep `rotationStep` as nap direction. Test that stale No Grainline settings cannot expand an explicit Any-mode piece rule.
- [ ] Rotation.1.5 Test fold-rule conflicts and the normalized-angle convention. Keep the existing grain-up baseline, including hidden grainlines.

## [ ] Task Rotation.2 — Carry rotation rules from SVG to packing data

**Why:** the handoff already supplies Rotation, but extraction currently discards it.
**Dependencies:** Rotation.1.

- [ ] Rotation.2.1 Add a typed, copyable rotation policy to `pack_types::Rect`; preserve `Rect::new()` defaults for existing callers.
  Keep unresolved source metadata distinct from the Any-only activated packing policy.
- [ ] Rotation.2.2 Parse `data-rotation` using `xmltree` in both `piece_extractor` extraction paths.
  Use the same parser for individual, Marker, size-tab, and Nested inputs.
- [ ] Rotation.2.3 Preserve rules through material splitting, wrapper hoisting, flattening, and piece division.
  Identify pieces by `id`; show warnings with `PieceRect::label()`.
- [ ] Rotation.2.4 For Nested piece-sets, intersect the member sizes' allowed angles.
  Do not read a nonexistent parent rule or silently select only the first size's rule.
- [ ] Rotation.2.5 Add active-handoff Qt tests for all five choices, visible/hidden labels, and save/reopen.
  Verify omitted None and exact numeric attributes without changing schema or DXF export behavior.
- [ ] Rotation.2.6 Add Rust extraction tests for missing, valid, invalid, hidden-grainline, transformed, nested, and split-piece inputs.

## [ ] Task Rotation.3 — Enforce per-piece angles in the shared packer

**Why:** one global trial set cannot represent a layout containing differently restricted pieces.
**Dependencies:** Rotation.2.

- [ ] Rotation.3.1 Extend the shared effective-angle resolver used by `piece_trial_set()`.
  Activate piece rules only for Any; otherwise preserve current mode and `free_rotation` behavior.
- [ ] Rotation.3.2 Intersect Any-mode rules with fold-edge angles.
  An empty intersection must remain unplaceable; never convert it to the existing empty-global-set fallback `[0]`.
- [ ] Rotation.3.3 Use the resolver in strict, lenient, extending, `fits_bin()`, `required_bin_width()`, and stacked-height calculations.
  Keep `Placed.id` aligned with original inputs, including skipped pieces.
- [ ] Rotation.3.4 Reuse `rotated_rect_dims()` and the existing MaxRects backend.
  Record the actual chosen angle in `Placed.rotation_deg`; do not round free rotation to quarter turns.
- [ ] Rotation.3.5 Retain deterministic orientation ordering and safe equal-AABB deduplication.
  Filter prohibited angles before deduplication; retain the first permitted representative.
- [ ] Rotation.3.6 Cache orientations once per job. Include orientation count when assessing existing search-work limits.
  Keep progress responsive; never silently truncate the allowed angle set to meet a runtime budget.
- [ ] Rotation.3.7 Add mixed-policy packing tests and retain all non-Any golden results.
  Assert allowed angles, containment, clearance, determinism, and correct unplaced IDs.

## [ ] Task Rotation.4 — Apply one policy to every layout and fit decision

**Why:** fitting, splitting, tiled candidates, and export must not disagree with the displayed layout.
**Dependencies:** Rotation.3.

- [ ] Rotation.4.1 Apply the shared policy in `layout_utils::do_process_layout()` and size-layout entry points.
  Pass sufficient mode context rather than inferring Any from a numeric trial set.
- [ ] Rotation.4.2 Update `sheets::build_sheet_export_inputs()` and its controller caller in `cxxqt_bridge::lib.rs`.
  Carry the same mode/rules into sheet assignment, oversized output, and remaining-piece packing.
- [ ] Rotation.4.3 Replace `oversized::partition_oversized_pieces()` upright/quarter-turn special cases with the shared fit predicate.
  Honor gap and fold-edge constraints consistently with packing.
- [ ] Rotation.4.4 Carry mode context through `piece_divider::DivideSpec` and its `prepared_pieces()` path.
  Do not split or enlarge media for a piece that fits at a permitted angle.
  Retain rules on every generated part.
- [ ] Rotation.4.5 Audit `layout_tiling` candidate-width and page-count calculations for the same per-piece policy.
  Preserve individual, Marker, Nested, per-size, paper-sheet, tiled, roll, and fabric behavior.
- [ ] Rotation.4.6 Resolve fold-edge eligibility against each piece's rules before pinning or mirroring the DOM.
  Do not leave an unauthorized `left_edge_angle` capable of overriding the shared resolver.
- [ ] Rotation.4.7 Test preview/export agreement and oversized/split decisions with identical input/settings.
  Include Any-only, non-quarter-turn fits and restrictions that prevent an otherwise possible quarter turn.

## [ ] Task Rotation.5 — Verify assembly and end-to-end rotation

**Why:** a correct selected angle is insufficient if rendering shifts geometry outside its packed slot.
**Dependencies:** Rotation.4.

- [ ] Rotation.5.1 Test `layout_assembler`, `oversized`, and `remaining` with nonzero origins and non-quarter-turn placements.
  Compare transformed bounds with packed slots; include margins, integer rounding, and clearance.
- [ ] Rotation.5.2 Verify seamline, cutline, notches, internal paths, labels, grainlines, and fold graphics receive the same placement transform.
  Preserve existing text-readability handling and rotation metadata in saved/exported SVG.
- [ ] Rotation.5.3 Add a deterministic free-angle regression: a 100×10 rectangle fits a 78×78 bin at 45°, but not at quarter turns.
  Disable media enlargement, splitting, and gaps for this fixture.
  Piece Any must fit at an allowed non-quarter angle; 1-Way, 2-Way, and 4-Way must remain unplaced.
- [ ] Rotation.5.4 Add mixed-value, hidden/absent-grainline, nested-size intersection, material, and fold-conflict integration fixtures.
  Test repeated layout and reload, not only the parser.
- [ ] Rotation.5.5 Repeat those fixtures under Along Grainline, Nap Up, and Nap Down.
  Changing Piece Rotation must not change their existing output or No Grainline behavior.
- [ ] Rotation.5.6 Measure all-Any runtime with a representative multi-piece fixture against the quarter-turn baseline.
  Record piece count, orientation count, elapsed time, and search-limit outcomes before accepting performance.

## [ ] Task Rotation.6 — Update explanations and verify the implementation

**Why:** current documentation promises only quarter turns, and acceptance requires actual layout evidence.
**Dependencies:** Rotation.5.

- [ ] Rotation.6.1 Update the rotation document, settings workflow, and SVG contract consumer notes.
  Explain Any-only precedence, omission behavior, normalized baseline, and 1-degree sampling.
  Do not rewrite attribute numbers or reuse `rotationStep` for this policy.
- [ ] Rotation.6.2 Update relevant comments/tooltips in `SettingsDialog.qml`, `SettingsModel`, and Rust settings/packing code.
  Keep current mode labels, defaults, and legacy migration.
- [ ] Rotation.6.3 Run focused Rust tests, then required full tests, from the directories below.
  Record commands and results; do not mark blocked or unrun checks complete.
- [ ] Rotation.6.4 Obtain approval before the integrated Windows MSI build.
  Use `packaging\windows\local_build_msi.ps1`; inspect per-suite logs if a Qt suite fails.
- [ ] Rotation.6.5 In the rebuilt apps, use asymmetric FrontPanel from male_shirt.
  Save/reopen each Rotation value, select layout Any, and verify allowed rotations without expecting every packing run to rotate a piece.
  Use the forced-fit fixture to prove non-quarter rotation.
- [ ] Rotation.6.6 Verify Along Grainline and both Nap directions remain unchanged.
  Leave the source checklist untouched; record evidence here and update `SESSION_HANDOVER.md`.

### Implementation test commands

Use the documented Qt 6.11.3/MSVC environment and Qt/Rust environment variables from `.github\README-BUILDS.md`.
Build only with approved suite tooling; a stale test executable is not verification of new code.

From `src\app\seamlylayout`:

```powershell
cargo test -p pack_types -p layout_engine -p packing -p layout_tiling -p cxxqt_bridge
cargo test --workspace
```

From `src\app\seamlylayout\qt_frontend`, with a configured, rebuilt debug tree:

```powershell
ctest --preset debug
```

The approved integrated MSI build also runs the Seamly2D Qt suites, release CTest, and Rust workspace tests.
Keep GUI acceptance and performance results separate from automated test results.

## Completion criteria

- Each Any-mode placement uses an angle permitted by its piece rule and any fold constraint.
- Piece Any can select a non-quarter angle when the input requires one.
- Packing, fit warnings, splitting, tiled candidates, and sheet exports agree.
- Along Grainline, With Nap, legacy migration, Piece Mode, and DXF rotation metadata retain their existing contracts.
- Required automated checks pass; rebuilt-app and performance evidence is recorded.
