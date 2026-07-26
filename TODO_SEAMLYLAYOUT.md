# TODO — SeamlyLayout app features

Tasks that add features to the SeamlyLayout layout app.

See `PROJECT_PLAN.md` for full details. Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `COMPLETED.md`.

## Task 21 — SeamlyLayout: three text modes for SVG export in the Exports menu

Replace the single "SVG" item in the SeamlyLayout Exports menu (`src/app/seamlylayout/qt_frontend/qml/ExportMenu.qml`, `exportSvgRequested()` wired through `Main.qml` to the Rust backend in `src/app/seamlylayout/crates/cxxqt_bridge/src/exports.rs`) with three SVG export modes differing in how label text is written.

1. **Text as `<text>` in the designer's selected (outline) font** — searchable, editable, re-stylable, human- and machine-readable; embeds the font via `@font-face` so it renders correctly on machines without it; supports tech-pack generation. Smallest file size.
2. **Text as `<text>` in a Hershey/single-line font** — same searchable/editable/machine-readable intent as mode 1, but using a bundled single-line font so the result is also friendly to CAD/CAM tools that resolve text. **Implementation note:** true Hershey fonts are stroke data, not installable outline fonts — for `<text>` + `font-family` to work this mode needs a "hairline" single-line TTF/OTF (an engineered font whose outline doubles back on itself to look like one stroke) embedded via `@font-face`. Known candidates: CamBam Stick Fonts (free, 9 variants, designed for CNC/plotting), MecSoft/Rhino single-stroke fonts, commercial single-line TTF bundles — verify redistribution/embedding license compatibility with the MIT Rust core before bundling. Caveats: hairline TTFs are still doubled-back outlines (stroke width not controllable via the font), and consumers that ignore embedded fonts will substitute — so mode 3 remains the guaranteed-fidelity choice for cutters. Record the font choice and rationale in `DECISIONS.md`.
3. **Text converted to paths (single-stroke)** — each label rendered as single-stroke `<path>` polylines from Hershey glyph data (**decision:** the path conversion uses the Hershey font, not the designer's outline font — a plotter then draws each character in one pen pass instead of tracing hollow glyph contours). Compatible with CAD/CAM/cutters/plotters/engravers with no font dependency in the consumer. Keep the original string machine-readable via `data-*`/`<desc>` on the label group (text is no longer searchable/editable as SVG text).

**Dependency:** all three modes need real `<text>` elements in the incoming `.pieces.svg` (Task 10) — even mode 3 needs the label *strings* to re-render them in stroke glyphs. Already-outlined input (Seamly2D `--text2paths`) can only be passed through as-is; the UI must handle path-only input (disable the text modes with an explanatory tooltip, or export the existing paths with a warning). Optional Hershey display/export on the Seamly2D side is Task 22.

- [ ] Replace the single "SVG" `MenuItem` with a three-entry submenu (or dialog choice) in `ExportMenu.qml`; add per-mode tooltips summarizing the compatibility/editability trade-off; wire new signals through `TopMenuBar.qml`/`Main.qml` to the bridge
- [ ] Mode 1: pass `<text>` through in the designer's font and embed the font as a subsetted `@font-face` data-URI (WOFF/TTF); document the font-licensing caveat (embedding rights vary by font license) in the export docs
- [ ] Mode 2: emit `<text>` styled with the bundled single-line font, embedded via `@font-face`, per the `DECISIONS.md` decision
- [ ] Mode 3: implement single-stroke text rendering in the Rust core — shape each label string into Hershey glyph strokes and emit stroked (fill-less) `<path>` polylines via `svg_dom`; keep the original label string on the group via `data-*`/`<desc>`
- [ ] Bundle Hershey/single-line glyph data under a permissive license compatible with the MIT Rust core (evaluate existing crates, e.g. a Hershey-font crate, before hand-rolling)
- [ ] Preserve the `data-*` tagging contract (`piece_label`/`pattern_label` groups, ids, `data-parent`) identically in all three modes
- [ ] Detect path-only input (no `<text>` in labels) and gate all three text modes accordingly
- [ ] Persist the last chosen SVG text mode in preferences (`PreferencesModel`)
- [ ] Tests: Rust unit tests for each conversion mode (mode 3 output is stroked polylines, not filled contours), plus the path-only-input case; frontend test for menu gating; end-to-end check with the richmond test pattern
- [ ] Update `src/app/seamlylayout/docs/status-docs/svg-data-attributes.md`, the root `status-docs/svg-data-attributes.md` mirror, and `src/app/seamlylayout/docs` export docs
- [ ] Doxygen briefs + inline comments on all touched functions

## Task 26 — Export multisize patterns (nested / marker / sized-layout-set)

Add layout export for multisize patterns — `.sm2d` patterns opened with a `.smms` multisize measurement file (multiple sizes; the CLI already exposes per-size gradation via `--gradationsize`/`--gradationheight`). The user chooses one of three multisize layout products in the settings dialog; all products orient every piece with its grainline pointing up.

- [ ] Settings dialog: user chooses "nested layout", "marker layout", or "set of sized layouts" for multisize export
- [ ] Generate a "size layout" for each size in the `.smms` file, all grainlines pointing up (per-size piece generation via the existing gradation machinery)
- [ ] Nested layout:
  - [ ] For each piece in the largest size, create a layout with all grainlines pointing up
  - [ ] For the remaining sizes in descending order: place each piece on top of its matching largest-size piece, grainline up, centering its center point on the largest piece's center point — each large piece becomes the base of a "pyramid" of matching pieces with the smallest on top
  - [ ] Apply transforms so all pieces are placed in global space
  - [ ] Group all pieces of each size together, so upstream tools (Pattern Projector, Inkscape, Illustrator, ...) can toggle each size's visibility
- [ ] Marker layout: copy all pieces from the size layouts and arrange them into a single marker layout, all grainlines pointing up
- [ ] Set of sized layouts:
  - [ ] Let the user view each size's layout in the canvas — UI design open: per-size tabs across the top of the canvas is the working idea, to be settled during implementation
  - [ ] Export the set to a single multi-page PDF, or to individual files of any export type
- [ ] Tests with a multisize test pattern (need a `.sm2d` + `.smms` fixture); verify grouping/grainline orientation in the exported SVG/PDF
- [ ] Doxygen briefs + inline comments on all touched functions; document the three products in the repo docs

## Task 57 — Give every Rust crate root a unique file name (11 crates all named `lib.rs`)

> **Superseded 2026-07-26 — this task's premise no longer holds.** The style guide revision `df5d90bb14` added an explicit carve-out to the uniqueness rule: *"Exception: Crate files in SeamlyLayout require multiple `lib.rs` files distinguishable by their paths."* Multiple `lib.rs` files are now **allowed**, so the rename below is no longer required by the rule that motivated it. Decide whether to delete this task outright, or keep only the parts the exception does not cover — the duplicated **`error.rs` ×2** (`crates/ezdxf2dxfastm/src/error.rs`, `crates/seamly_svg2ezdxf/src/error.rs`), which are ordinary modules, not crate roots, and are still a plain violation. Everything below is kept as the record of the analysis.

**Original goal (from `.github/README-CODE-STYLES.md`, File Names):** "*Unique names: a search for the \<filename.extension> should return only one file.*" Under the seamlylayout workspace, **11 crate roots are all named `lib.rs`** — `app_core`, `cxxqt_bridge`, `ezdxf2dxfastm`, `geometry`, `layout_engine`, `layout_tiling`, `pack_types`, `packing`, `polygon_pack`, `seamly_svg2ezdxf`, `svg_dom` — plus `error.rs` twice (`ezdxf2dxfastm`, `seamly_svg2ezdxf`). Opening "lib.rs" in any editor picker is a guess between 11 files.

**Rename, don't split.** Splitting `app_core`'s root into named modules does *not* meet the goal: Cargo still requires a crate root, so `lib.rs` survives as the file holding the `mod` declarations, and there are still 11 of them. Splitting also *adds* names that must then be kept unique — `error.rs` ×2 is that drift already happening. The root is ~215 lines (8 public functions over one cohesive load → convert → render pipeline, plus a 4-case test module), which does not need breaking up; revisit that on its own merits if it grows.

**Name each root after its crate, not after the app.** `lib_seamlylayout.rs` names the *application*, so applying it across the workspace would produce 12 files wanting the same name — the collision the rule forbids. `crates/<crate>/src/<crate>.rs` (`app_core.rs`, `svg_dom.rs`, `geometry.rs`, …) is unique **by construction**, because Cargo already forbids duplicate crate names in a workspace, and it matches Rust's own `mod foo` ↔ `foo.rs` convention.

**What each rename requires:** Cargo defaults a library target's root to `src/lib.rs`, so the crate stops building (`can't find library …`) until its `Cargo.toml` says otherwise:

```toml
[lib]
name = "app_core"
path = "src/app_core.rs"
```

Crate names and every `use app_core::…` are unaffected — a crate's identity is `[package] name`, not the file name. Verified 2026-07-26: nothing outside the crates names any root file — not `build.ps1`, `qd.ps1`, `qt_frontend/CMakeLists.txt`/Corrosion, the cxx-qt build, or `seamlylayout-ci.yml` (they all reference crate *directories*). Only two docs do.

- [ ] Confirm the naming scheme (`<crate>.rs`) and that it applies to all 11 library crates, not just `app_core`
- [ ] `git mv` each `crates/<crate>/src/lib.rs` → `crates/<crate>/src/<crate>.rs` so history follows the files
- [ ] Add `[lib] name` + `path` to each of the 11 `Cargo.toml`s in the same commit — a rename without it breaks the build immediately
- [ ] Decide `cli`'s binary root: `src/main.rs` is currently unique repo-wide (only `main.cpp` collides, ×5, and that is a different extension), so it can stay — or rename to `src/cli.rs` via `[[bin]] name`/`path` for consistency. Record which and why
- [ ] Resolve the `error.rs` collision at the same time (`crates/ezdxf2dxfastm/src/error.rs`, `crates/seamly_svg2ezdxf/src/error.rs`) — these are plain modules, so renaming them means updating their `mod`/`use` lines, nothing more
- [ ] Update the docs that name a root path: `docs/dxf-docs/DXF_EXPORT_ARCHITECTURE.md:420` and `docs/dxf-docs/DXF_EXPORT_PLAN.md:298` (both `// In app_core/src/lib.rs`); check `docs/CODEBASE_INDEX.md`'s crate list while there
- [ ] Record the convention in `src/app/seamlylayout/.claude/rules/rust-style.mdc` (it states no file-naming rule today) so new crates do not drift back to `lib.rs`
- [ ] Rebuild and test: `src/app/seamlylayout/build.ps1` plus `cargo test --workspace` with `$env:QMAKE` pinned to the 6.11.1 kit (Task 47); confirm each moved `#[cfg(test)] mod tests` still runs (they travel with their file — `app_core`'s 4 cases at `lib.rs:177` among them)
- [ ] Confirm `seamlylayout-ci.yml` stays green
- [ ] Note for the wider rule (out of scope here, worth its own task): the C++ tree violates the same uniqueness rule far more heavily — `stable.h`, `stable.cpp` and `warnings.pri` ×21 each, `main.cpp` ×5, `qttestmainlambda.cpp` ×3, `calculator.{h,cpp}` / `version.h` / `xml.pri` / `dialogs.pri` / `tools.pri` ×2 — while `Cargo.toml` ×13 is mandated by Cargo and the vendored `src/libs/xerces-c/{macx,mingw,msvc,msvc-arm64}` copies account for 443 duplicate basenames on their own. Any repo-wide version of this rule needs explicit carve-outs for tool-mandated names and vendored third-party code
