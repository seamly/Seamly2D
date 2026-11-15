# Session handover

## Current status

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

### 2026-11-14 — Marker and Nested unavailable after a split (Layout.Y2)

- Multisize "Split the pieces": the run lays out only Base Size and each size (`layout_views::size_kinds`, was `tiled_kinds`), every material.
- `LayoutViews::start(kind, split)`: a split run puts Marker and Nested views first, with no layout; Base Size shows first.
- New QML property `layoutViewUnavailableText` (`SPLIT_UNAVAILABLE_TEXT` on those tabs): canvas shows it; no media size; Adjust, Export, View and fit-to-view off.
- Export all tabs skips them: `LayoutViews::in_all_tabs_export`.
- Removed the Nested piece-set split (`divide_unit`, `divide_piece_set`, `outline_bounds`, `PiecePart`, its test): no split run lays out Nested now.
- Tests: `split_run_makes_marker_and_nested_unavailable`, `unsplit_run_keeps_marker_and_nested`; jacket2 split test lays out size tabs only.
- Checked: `cargo test --workspace` 709 pass; `ctest` debug 8/8. Built in worktree `split-tabs`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES, PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` §47, new §49 and OPEN row.
- Open: Layout.Y2.1 installed-app check with the next MSI (§49). X7.4 (Nested set too wide) no longer applies.

### 2026-11-14 — Material tab order (Layout.Y1)

- `material_split.rs`: `pattern_materials` sorts by `MATERIAL_ORDER`: Fabric, Lining, Interfacing, Interlining. Other names go after, in handoff order. Match ignores ASCII case.
- Cause: Seamly2D writes the pattern groups in the order of the first piece of each material, so Lining could come first.
- Both tab bars (`materialLabels`) and every material index (`material_docs`, `material_layouts`) follow the new order.
- Test: `materials_follow_the_tab_order`.
- Checked: `cargo test --workspace` 708 pass; `ctest` debug 8/8. Built in worktree `material-tab-order`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES Materials.
- Open: Layout.Y1.1 installed-app check with the next MSI.

### 2026-11-14 — Split pieces keep their markings (Layout.803)

- `piece_divider.rs`: `stripe` and `plaid` reference lines are open lines that end at the cut lines, like `internal_path`.
- Drill holes: `all_points` adds circle centres. A `drill` group goes whole to the part nearest its centre (before: always part 1). `cut_positions` keeps cut lines off drill centres.
- Internal paths: no code change; already split. A closed one becomes one open line per part.
- Tests: `markings_split_with_the_piece`, `cut_line_misses_drill_centres`.
- Built and tested in worktree `split-markings`; merged `--no-ff`; branch and worktree deleted.
- Docs: SVG-DATA-ATTRIBUTES split parts, PROCESS LAYOUT WORKFLOW, SIMPLE_WORKFLOW, `AGENT_CHECKLIST.md` §48 and OPEN row.
- Open: Layout.803.1 installed-app check with the next MSI (§48).

## Decisions to retain

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
