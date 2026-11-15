# Session handover

## Current status

### 2026-11-15 — Fold edge option in the Fabric section (Layout.Y7)

- `SettingsDialog.qml`: "Place fold lines on the left edge" moved to the Fabric section, below "Fabric Folded". Enabled only when Fabric Folded is checked.
- Default on: `LayoutSettings` serde default, `SettingsModel` member and `resetToDefaults`, `default_settings.json`.
- `LayoutSettings::is_folded_fabric`: `places_fold_lines_on_left_edge` and the zero left margin need folded fabric. Paper and open fabric ignore a saved value.
- `layout_utils.rs` uses `is_folded_fabric` at both fold checks.
- Tests: `fold_lines_on_left_edge_apply_to_folded_fabric_only`, `fold_lines_on_left_edge_default_on`; paper and across-grain cases in `layout_utils`; Qt `foldLinesOnLeftEdge_defaultsOnAndRoundtrips`, `foldLinesOnLeftEdgeCheck_enabledOnlyForFoldedFabric`.
- Checked: `cargo test --workspace` 714 pass; `ctest` debug 8/8. Built in worktree `fold-edge`; merged `--no-ff`; branch and worktree deleted.
- Docs: SETTINGS_WORKFLOW, CREATE_LAYOUT_WORKFLOW, SVG-DATA-ATTRIBUTES, `AGENT_CHECKLIST.md` §21, §39, new §54 and OPEN row.
- Open: Layout.Y7.3 installed-app check with the next MSI (§54).

### 2026-11-14 — Paper roll and fabric keep their width (Layout.Y6)

- `layout_utils::do_process_layout`: every non-tiled pack uses `bin_w`, the contentRect width. Removed `pack_bin_w`, `mr_px`, `set_content_width` (`layout_assembler.rs`).
- A piece wider than a paper roll or fabric stays unplaced; the warning names it. Only the length grows.
- `lib.rs` `process_layout`: the oversized-piece check runs for every layout. Multisize paper rolls now get the dialog too.
- `Main.qml`: dialog choice "Make the media longer"; text says the media keeps its width.
- `enlarged_media_message` compares the length only.
- Tests: `placement_enlargement_dimensions_and_notifications`, `jacket2_marker_keeps_narrow_roll_width` (replaces `jacket2_marker_widens_narrow_roll`).
- Checked: `cargo test --workspace` 713 pass; `ctest` debug 8/8. Built in worktree `roll-width`; merged `--no-ff`; branch and worktree deleted.
- Docs: PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` new §53 and OPEN row; §11 and its OPEN row removed.
- Open: Layout.Y6.1 installed-app check with the next MSI (§53).

### 2026-11-14 — Tilted piece: grainline tilts (Layout.Y5)

- Seamly2D Piece Mode: `PatternPieceTool::updateGrainline` turns the grainline with the piece tilt, about the same center. Labels stay level.
- `applyTiltToGeometry` keeps the tilt in `m_tilt_degrees` and `m_tilt_center`.
- Grainline move, resize and rotate remove the tilt before they store the grainline; the pattern keeps a level grainline.
- Flip is unchanged: the grainline still does not mirror (applyFlipToGeometry rule).
- Test: `TST_VPiece::PieceTiltTransformTurnsGrainline`, passed in a scratch QtTest build. `pattern_piece_tool.cpp` compiles (cl). Full Qt suites run in the next MSI.
- Built in worktree `tilt-grain`; merged `--no-ff`; branch and worktree deleted.
- Docs: `AGENT_CHECKLIST.md` new §52 and OPEN row; `TEST_APPLICATION_UPDATES.md` tilt line.
- Open: Layout.Y5.1 installed-app check with the next MSI (§52).

### 2026-11-14 — Split drill holes (Layout.Y4)

- `piece_divider.rs` `drill_for_parts`: a cut line that crosses a drill circle splits it. Each part's `<circle>` becomes a `<path>` of its arcs (cubic curves, 90° or less each) that end at the cut line.
- The part nearest the centre keeps `<circle r="0" stroke="none" fill="none">`: one DXF-ASTM layer 13 drill point. Decision: DXF has no half drill hole; the point stays where the centre is.
- Cubic curves, not SVG `A` arcs: `svg_dom` bakes an arc wrongly under a flip.
- `seamly_svg2ezdxf` `extract_drill_hole`: a split part (`data-split-of`) with no circle writes no point; a zero-radius circle writes the point. Unsplit pieces keep the old checks.
- Tests: `drill_hole_splits_at_the_cut_line`, `narrow_cell_holds_two_arcs`, `split_drill_hole_keeps_one_drill_point`; two reject cases.
- Checked: `cargo test --workspace` 712 pass; `ctest` debug 8/8. Built in worktree `split-drill`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES, PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, DXF-ASTM compliance, `AGENT_CHECKLIST.md` §48, new §51 and OPEN row.
- Open: Layout.Y4.1 installed-app check with the next MSI (§51).

### 2026-11-14 — Nested: lowest corner without a right angle (Layout.Y3)

- Stack point Rule 2a: a right angle at one or more turn points keeps the old rule.
- Rule 2b, no right angle: the lowest turn point (largest y' in the grain frame) wins. A level tie uses the 2a edge ranks, then left.
- Right angle: the two edges meet within 1° of 90°. The first point away from the turn point sets each edge's direction.
- Same rule in `svg_dom/src/stack_point.rs` (`ranks_lower`, `RIGHT_ANGLE_COSINE`) and `layout_handoff/handoff_stack_point.cpp`.
- jacket2: only Detail 3 changes. Detail 2 has no right angle but has a fold line (Rule 1).
- Fixture `jacket2_40-146_handoff.svg` realigned: 25 Detail 3 grainline transforms.
- Tests: `right_angle_is_within_one_degree`, `without_a_right_angle_the_lowest_corner_wins`, Qt `stackPointTakesLowestCornerWithoutRightAngle`. Trapezoid fixtures are right trapezoids.
- Checked: `cargo test --workspace` 711 pass; `ctest` debug 8/8; `TestHandoffAnnotationAlignment` 19/19 in a scratch QtTest build. Full Qt suites run in the next MSI.
- Built in worktree `nested-corner`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES "Multisize stack point"; `AGENT_CHECKLIST.md` new §50 and OPEN row.
- Open: Layout.Y3.1 installed-app check with the next MSI (§50).

## Decisions to retain

- "Place fold lines on the left edge" applies to folded fabric only; default on. The dialog enables it only when Fabric Folded is checked. A hidden setting must not change the layout (2026-11-15).
- No layout changes the width of a paper roll or fabric (folded or not), or the contentRect width. Only the length grows; a piece wider than the media is left out or split (user, 2026-11-14). Replaces Layout.71 roll widening.
- Multisize stack point with no right-angle turn point: the lowest turn point wins; the sizes align along its longest edge. An edge is the straight part only (user, 2026-11-14).
- Layout Mode Any: a piece too wide at its Rotation rule's angles turns 90° if that fits. This replaces the rule for that piece only; the packer cannot place it otherwise (Layout.802 task text, 2026-11-12).
- Custom page size lives in Tile Size > Custom (`tileWidth`, `tileHeight`). Paper Type has no custom size (user, 2026-11-12).
- Layout Mode Any ignores "Place fold lines on the left edge": no pin, no turn, no mirror, no conflict error (user, 2026-11-12).
- A paper sheet never grows, multisize tabs included. Overflow directs the user to PDF (tiled) (user, 2026-11-12).
- Keep `data-notch-type`. Only DXF export converts every notch to a layer 4 slit.
- Notch attributes: one value per notch (D1, 2026-10-12). Replaces "one value per segment" (2026-10-11).
- Built-in seam line corners are plain offset corners; node corner types shape only the cut line (user, 2026-10-12).
- Slit ends are never turn points. Slit depth uses half the local seam allowance width.
- Multisize tabs: Marker, Nested, Base Size N, then Size N. Every tab comes from the input SVG; no tab is built from grade rules. No "Multisize Layout" setting (user, 2026-10-13).
- DXF-ASTM carries one size: Base Size tab = base size + `# n` + `.rul`; size tab = one ungraded size; Marker and Nested have no DXF-ASTM export. No graded nests (user, 2026-10-13).
- CLO3D export uses R13 (`AC1012`). Piece text uses `Size Name:` and omits `Category:`.
- Default data root is `~/seamly2d`. Fresh installs allow a change; updates and repairs keep the recorded root read-only.
- Keep CLI export until SeamlyLayout CLI parity. Keep `exportLayoutAs()` through File → Export As.

## Authoritative documents

- [Application update tests](project-docs/TEST_APPLICATION_UPDATES.md)
- [SeamlyLayout TODO](project-docs/docs-seamlylayout/TODO_SEAMLYLAYOUT.md)
- [DXF TODO](project-docs/docs-seamlylayout/TODO_DXF.md)
- [SVG attribute contract](project-docs/docs-data/SVG-DATA-ATTRIBUTES.md)
- [Installer TODO](project-docs/docs-installer/TODO_INSTALLER.md)
- [Completed work archive](project-docs/TODO_COMPLETED.md)
