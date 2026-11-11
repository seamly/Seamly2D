# Session handover

## Current status

### 2026-11-11 — Fold lines across the grain

- Cause: trousers `Test-halfleg` fold line runs across the grain; the fold plan pinned vertical fold lines only.
- `fold_line_edge::plan_fold_line_edge` turns such a piece until its fold line is vertical. Fold edge outranks layout mode; Any mode checks an explicit piece rotation.
- `piece_divider::divide_oversized_pieces` turns in place before it measures.
- Risk: on fabric, a turned piece's grain runs across the fabric.
- `cargo test --workspace`: pass. Installed-app check with `trousers.sm2d` runs with the next MSI.

### 2026-11-10 — Reference lines icon

- Pattern Piece dialog: Reference lines page shows stripe and plaid icon (`reference_lines.svg`).
- Temporary render check: 8/8 pixels pass. `TST_PieceMarkings` runs in the next MSI build.

## Decisions to retain

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
