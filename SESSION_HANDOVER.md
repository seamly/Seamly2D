# Session handover

## Current status

### 2026-11-17 — Plan: private Rust source

- Plan: `project-docs/docs-cicd/TODO_PRIVATE_RUST_SOURCE.md` (prefix `PrivateRust.`). Order 0 → 1 → 8 → 2 → 3 → 4 → 5 → 7 → 6.
- Decisions D1–D9 recorded. D7: Rust core in shared library `seamlylayout_core`; `seamlylayout.exe` holds open-source code only.
- D8: crate fonts (Relief, Hershey) move to `src/fonts/` and stay public.
- Done: 1.1; 1.3 (copyright assigned to Seamly Systems, Inc.).
- Open 1.2: counsel re-confirms. Proprietary code still runs on the PC: core library, `seamlycli.exe`.
- D9: fonts compiled into `seamlylayout.exe` as Qt resources; frontend passes the bytes to the core at startup.
- Stop-gap 0.1: keep new Rust commits off `run-seamlyLayout` until PrivateRust.5.
- No code changes. Plan and handover commits only.
- 2026-11-18 user decision: PrivateRust.8 Rust commits wait on local branch `task-move-label-fonts` until the split. Do not merge or delete it.
- Next: PrivateRust.8.

### 2026-11-17 — CI: fold label test and push builds

- CI failure 11-02 to 11-17: `TST_HandoffDocument::pieceMatchesOldWriter` fold rows; label 0.19 px off, Linux only.
- Cause: old writer sized the label with `QGraphicsSimpleTextItem::boundingRect()`; on Linux its height differs from `QFontMetricsF::height()`.
- Fix 66a7d9c969: `VLayoutPiece::createFoldLineLabelItem` uses `QFontMetricsF` in both branches. MSVC syntax check passed; Windows cannot reproduce.
- Fix ab86c39682: `ci-layout.yml` linux, macos, windows-msi jobs run on push to `run-seamlyLayout`. Publish jobs unchanged.
- 3bd7434c3f: CI builds only the Windows x64 MSI. Platform order is in `TODO_INSTALLER.md` Installer.8 (user decision).
- 74a618eaf7: our workflow is `ci-layout.yml` (`CI Layout`), push trigger `run-seamlyLayout` only. `license_notices` tests pass.
- a91d3e7e96: upstream `ci.yml` restored beside it, identical to `develop`'s copy (user decision). Rule in CLAUDE.md CI section.
- Upstream `ci.yml` fails on our tree (no `Seamly2D.pro`, `scripts/version.sh`, `seamly2d-installer.nsi`).
- Workflow "CI" (`ci.yml`) disabled in seamly/Seamly2D: `gh workflow disable ci.yml` (user decision). Upstream still runs it. Re-enable: `gh workflow enable ci.yml --repo seamly/Seamly2D`.
- Sue pushed (origin at d1fea41371). CI Layout run 44165339525 failed:
  - `version` job wrote `2026.11.17.2004`; `version.sh` needs `YY.M.DDHH`. The 10-10 `develop` merge brought upstream's formula back. Fixed in `ci-layout.yml`; merge-upstream skill notes it.
  - `linux-test`: parallel qmake runs (make -j) wrote and read the missing version header at once. New `project_version.pri`, included first by `Seamly.pro`. Scratch qmake check passed.
- Not run since: Linux ctest and cargo (11-02), platform builds (10-19). More failures possible.
- Extra origin branches: `gh-pages` (`document` job, `builddocs.yml`), `dependabot/cargo/src/app/seamlycli/cargo-4cb1234933` (PR #32).
- Open: Export Draft Blocks SVG writer to replace `SvgGenerator` (a: vformat draft-only writer, recommended; b: `QSvgGenerator`).
- Open: skip-ci default in CLAUDE.md stops push builds; keep or drop.
- Next: Sue pushes the fix; watch `linux-test` and the x64 MSI job; then Installer.8.2 (arm64).

### 2026-11-16 — Utilities menu launches SeamlyCLI

- Added Utilities > SeamlyCLI in Seamly2D.
- The action finds SeamlyCLI beside Seamly2D, in the development release directory, or through PATH.
- The action starts a detached process and reports launch failures.
- The Qt 6.11.3 Windows build passed with the existing MSVC Makefile.
- Menu launch verification remains pending. The installed Seamly2D still runs and remains unchanged.
- Files: `src/app/seamly2d/mainwindow.cpp` and `mainwindow.ui`.
- No commit or push.

### 2026-11-16 — SeamlyCLI results buttons

- Added Exit to close SeamlyCLI from the results page.
- Placed Start Over and Exit in a fixed footer at the bottom right. Back remains at the left.
- All 25 SeamlyCLI release unit tests passed. Visual verification remains pending.
- File: `src/app/seamlycli/src/app_view.rs`.
- No commit or push.

### 2026-11-16 — SeamlyCLI export buttons and command text

- Renamed Run to Create Layout.
- Placed Cancel and Create Layout in a fixed footer at the bottom right of the export screen.
- Cancel closes SeamlyCLI before export. The buttons remain disabled during export.
- Increased command text to 18 pixels on the export and results screens.
- All 25 SeamlyCLI release unit tests passed. Visual verification remains pending.
- File: `src/app/seamlycli/src/app_view.rs`.
- No commit or push.

## Decisions to retain

- Upstream `ci.yml` may fail on the final upstream PR. Handle it then (user, 2026-11-17).
- Only `develop` and `run-seamlyLayout` belong on seamly/Seamly2D (user, 2026-11-17).
- A push to `run-seamlyLayout` builds all platforms; it does not publish (user, 2026-11-17).
- CLI Linux and macOS checks run in CI (`gh workflow run ci-layout.yml`), not in a local WSL install (user, 2026-11-16).
- SeamlyLayout is complete after the `develop` merge. Next focus: CLI layouts from pattern and measurement files plus parameters (user, 2026-11-15).
- Adopt develop's `command_line.*` file names and lowerCamel `VCommandLine` method names: they match the code style (user, 2026-11-15).
- Own CLI design (`--layout-export` to SeamlyLayout) replaces upstream console export. Lost legacy options go to `TODO_CLI.md` CLI.3 (user, 2026-11-15).
- Upstream per-app DMG files stay in `packaging/macos/dmg/` for a later 3-app DMG Finder layout (user, 2026-11-15).
- Keep our Copy Piece (`PatternPieceCopy`); remove develop's Duplicate. Details > Copy Piece calls our code with Sue's `copy_piece` icon (user, 2026-11-15).
- Keep develop's #1827 Move tool fix over our #853 rule: it fixed a problem (user, 2026-11-15).
- Canvas SVG goes to WebEngine through `CanvasSvgSchemeHandler` (`seamly-canvas://`, from memory). No `data:` URL: the 2 MB limit is not acceptable (user, 2026-11-15).
- Pieces with a fold line go first, down the left edge from the top, on every layout and tab, on paper and fabric (user, 2026-11-15).
- Piece Layout rotation Any samples 10° steps, not 1°; 1° steps are not efficient (user, 2026-11-15).
- "Place fold lines on the left edge" applies to paper and fabric; default on; it sits below Media Type. Paper and open fabric keep the left margin; only folded fabric has none (user, 2026-11-15). Replaces folded fabric only.
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
