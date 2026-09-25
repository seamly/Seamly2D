# SeamlyLayout Qt Migration Decisions

Author: slspencer
Copyright: 2026

Last updated: 2026-06-29

This file contains architecture/design decisions and open decision records.
Active implementation tasks are tracked in `docs/status-docs/TODO_SEAMLYLAYOUT_2.md`.
Status roll-up is tracked in `docs/status-docs/SEAMLYLAYOUT_MIGRATION_STATUS.md`.

## Decision Status Legend

- `Proposed` — options are documented; no final call yet.
- `Accepted` — option selected and approved.
- `Rejected` — considered and not chosen.
- `Superseded` — replaced by a newer accepted decision.

## Decision Records

### Decision-001 — Settings Submit creates `layout_dom`

- **Status:** Accepted
- **Priority:** P0
- **Owner:** Product
- **Decision deadline:** Resolved
- **Related TODO item:** Removed from `TODO.md` (non-decision)

**Problem**

Settings Submit behavior is now explicitly defined: settings application creates/displays `layout_dom` immediately so users get right-canvas visual confirmation.

**Decision outcome**

Accepted behavior is equivalent to prior Option A: Submit applies settings and immediately creates/displays `layout_dom` (settings-applied flow).

**Rationale**

- Matches intended UX: immediate right-canvas confirmation after Submit.
- Removes ambiguity in workflow semantics.
- Aligns implementation intent with operator expectations.

---

### Decision-002 — All observability file I/O gated app-wide (compile-time)

- **Status:** Accepted
- **Priority:** P1
- **Owner:** slspencer
- **Decision deadline:** 2026-06-29 (DG.5 verification gate closed)
- **Related TODO item:** `DG.5 Verify debug gate correctness` (completed 2026-06-29)

**Problem**

All observability file I/O in the application (debug log writes, SVG DOM snapshots, overlay data dumps, artifact cleanup) must be eliminated from release builds without losing debuggability in development builds.  The original scope was limited to AdjustMode; DG.5 confirmed the gate applies app-wide across all three Rust source files and the Qt C++ layer.

**Scope (app-wide)**

All four observability gates are enforced across the full application:

- `log_to_file()` in `lib.rs`, `exports.rs`, `layout_utils.rs` — gated by `#[cfg(debug_assertions)]` / no-op stub (DG.1)
- `save_debug_dom()` and `get_out_dir()` in `lib.rs`, `layout_utils.rs` — gated by `#[cfg(debug_assertions)]` / no-op stubs; `output/` dir never created in release (DG.2)
- `cleanup_adjust_output_artifacts()` in `lib.rs` — gated by `#[cfg(debug_assertions)]` / no-op returning 0 (DG.3)
- `dumpOverlayData()` in `AdjustScene.h/.cpp` — gated by `#ifdef QT_DEBUG` / inline empty-body stub (DG.4)

**Decision options**

1. **Option A:** Enforce strict no-disk-I/O for runtime path; debug writes optional and opt-in only.
2. **Option B:** Keep limited debug writes in runtime path for observability.
3. **Option C (selected):** Compile-time gate — `debug` builds allow writes, `release` builds disable them.

**Decision criteria**

- Runtime performance and UX responsiveness
- Debuggability and supportability
- Release cleanliness and deterministic behavior

**Decision outcome**

Option C selected, applied app-wide. All observability file writes are gated by `#[cfg(debug_assertions)]` (Rust) or `#ifdef QT_DEBUG` (C++). Release builds enforce strict no-disk-I/O across the entire application; debug builds retain disk writes for observability without shipping them.

**Verification (DG.5)**

- `cargo build --release -p cxxqt_bridge` confirmed: no `output/` directory created.
- `cargo test -p cxxqt_bridge`: 81 passed, 0 failed (debug config).
- `cargo test --release -p cxxqt_bridge`: 79 passed, 0 failed (release config).
- `dg5_verification_tests` module in `lib.rs` pins the acceptance contract for both build modes.

**Rationale**

- Preserves debuggability during development without leaking I/O into release builds.
- Compile-time enforcement is deterministic and requires no runtime flag management.
- Aligns with Rust idiom (`#[cfg(debug_assertions)]`) and Qt idiom (`#ifdef QT_DEBUG`).
- App-wide scope ensures no observability path is accidentally left ungated.

### Decision-003 — SVG label text modes: bundled fonts

- **Status:** Accepted
- **Owner:** Product
- **Related TODO item:** Task Layout.2 (`TODO_SEAMLYLAYOUT.md`)

**Mode 2 font: Relief SingleLine (SIL OFL 1.1)**

- Source: `github.com/isdat-type/Relief-SingleLine`. No Reserved Font Name.
- OFL allows bundling with MIT software and embedding in documents.
- The CAD TTF stores strokes as open contours. TrueType closes every contour, so browsers draw "C" as "O" and "2" as "8". Rejected for display.
- Bundled: `ReliefSingleLineOutline-Regular.otf`, the same strokes as thin filled shapes. Every SVG viewer renders it correctly.
- Label text names `Relief SingleLine CAD`; `@font-face` declares that name with the Outline data.
- Result: a CAD/CAM tool that resolves text by installed font draws true single strokes; a viewer that honours `@font-face` draws the Outline shapes.
- Mode 3 stays the guaranteed single-stroke choice for cutters.

**Mode 3 glyphs: Hershey Roman Simplex (`futural.jhf`)**

- Source: `github.com/kamalmostafa/hershey-fonts`. The Hershey notice permits any use, with its acknowledgements shipped.
- Parser: `hershey` crate 0.1.2 (Apache-2.0 OR LGPL-3.0; used under Apache-2.0).
- Scale: 32 Hershey units per em; baseline at Hershey y = 9.

**Font subsetting: `allsorts` 0.17 (Apache-2.0)**

- `subsetter` 0.1 kept every glyph slot and the full `cmap`: a CJK UI font added about 560 KB per export.
- `allsorts` keeps only the used glyphs and writes a new Unicode `cmap`, which browsers need.
- Profile `Minimal`: hinting and GSUB/GPOS are dropped. Built with `flate2_rust`, so no C zlib is needed.

**License notices**

- `crates/svg_label_text/assets/fonts/OFL.txt`, `crates/svg_label_text/assets/hershey/HERSHEY_LICENSE.txt`.
- Copies in `src/app/seamlylayout/packaging/licenses/`; `smsi.ps1` installs them to `licenses\` on Windows.
- Linux AppImage and macOS DMG do not ship these notices yet.

## Log files never go in the install directory (2026-08-15, revised 2026-09-14)

`Logger::init()` writes to the `AppConfigLocation` root on every platform, not
beside the executable — Windows, macOS, the Linux AppImage and Flatpak alike.

**Why:** the MSI installs into `%ProgramFiles%\SeamlyApps`, which a standard
user cannot write. An installed build with `--debug` therefore either failed to
log or was silently redirected to `VirtualStore`. Running as an administrator
hid the fault and left an `output\` directory inside Program Files that no
uninstall removes — the installer does not own it, so no component rule
applies. One was found on a test machine on 2026-08-15, left by an earlier
install at `%ProgramFiles%\Seamly2D\output\`.

An ordinary (non-packaged) Linux install used to log next to the executable
instead. `LoggerTests` locked one `AppConfigLocation`-rooted path on every
platform; the Linux exception never actually ran under CI until SeamlyLayout's
Qt suites were wired into `ci.yml`'s `linux-test` job (2026-09-13), and failed
against that lock — removed rather than special-cased, since a stable,
predictable log path beats matching the install directory either way.

This is separate from the DG.1–DG.5 gate above: that decides *whether* debug
files are written, this decides *where*.

## Accepted Architectural Decisions (snapshot)

- CXX-Qt bridge is the Rust↔Qt integration boundary.
- SVG canvas display uses in-memory DOM string flow rather than temp-file display paths.
- `Superseded` is the canonical spelling for retired workflow notes.

## Change Log

- 2026-05-22: Created formal decision records with explicit `Owner`, `Decision deadline`, and `Decision options` fields.
- 2026-05-23: Marked Decision-001 as `Accepted` and aligned wording with finalized Submit behavior (immediate `layout_dom` display).
- 2026-06-29: Marked Decision-002 as `Accepted` — Option C selected (compile-time gate: debug writes allowed, release builds enforce no disk I/O).
- 2026-06-29: Expanded Decision-002 scope from AdjustMode to all observability file I/O app-wide (DG.5 verification gate closed); added Scope, Verification, and updated Rationale sections.
- 2026-09-24: Added Decision-003 (SVG label text modes: bundled fonts).
- 2026-09-24: Decision-003 — font subsetting moved from `subsetter` to `allsorts`.
