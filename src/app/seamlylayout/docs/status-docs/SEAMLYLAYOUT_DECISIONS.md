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

## Accepted Architectural Decisions (snapshot)

- CXX-Qt bridge is the Rust↔Qt integration boundary.
- SVG canvas display uses in-memory DOM string flow rather than temp-file display paths.
- `Superseded` is the canonical spelling for retired workflow notes.

## Change Log

- 2026-05-22: Created formal decision records with explicit `Owner`, `Decision deadline`, and `Decision options` fields.
- 2026-05-23: Marked Decision-001 as `Accepted` and aligned wording with finalized Submit behavior (immediate `layout_dom` display).
- 2026-06-29: Marked Decision-002 as `Accepted` — Option C selected (compile-time gate: debug writes allowed, release builds enforce no disk I/O).
- 2026-06-29: Expanded Decision-002 scope from AdjustMode to all observability file I/O app-wide (DG.5 verification gate closed); added Scope, Verification, and updated Rationale sections.
