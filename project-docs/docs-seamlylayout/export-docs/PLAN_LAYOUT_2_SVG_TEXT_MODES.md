# Plan — Task Layout.2: three SVG text modes

Source: `project-docs/docs-seamlylayout/TODO_SEAMLYLAYOUT.md`, Task Layout.2.

## Current state

- `ExportMenu.qml`: one `SVG` item → `exportSvgRequested()`.
- `TopMenuBar.qml` reuses `ExportMenu` for the View menu (`viewSvgRequested`).
- `Main.qml`: `onExportSvgRequested` → save dialog → `exportStartTimer` → `appController.exportSvg(path)`.
- `lib.rs` `export_svg` → `clone_stripped_layout_doc()` → `exports::do_export_svg` (writes DOM verbatim).
- Seamly2D label input (`vlayoutpiece.cpp` `createLabelItem`):
  - default: one `<text>` per line, in `<g data-type="piece_label|pattern_label">`, font from label template;
  - `--text2paths`: filled glyph outlines as `<path>`, no `<text>`.
- Available in `Cargo.lock` already: `fontdb 0.18`, `ttf-parser 0.21`, `subsetter 0.1.1`, `base64 0.22`.
- Hershey crates: `hershey 0.1.2` (parser, Apache-2.0 OR LGPL-3.0); `vector-text-hershey` (no license → reject).

## Design

### New Rust module

- New crate `crates/svg_label_text/` (MIT), workspace member.
- Input: `&mut svg_dom::Document` + `SvgTextMode`. Output: `Result<LabelTextReport, String>`.
- `LabelTextReport`: label count, `<text>` count, missing fonts, non-embeddable fonts.
- Scope: only `<text>` inside `data-type="piece_label"` / `"pattern_label"` groups.
- `id`, `data-*`, and group structure stay unchanged in all modes (Layout.29).

| Mode | Enum | `<text>` handling | `<style>` added |
| ---- | ---- | ---------------- | --------------- |
| 1 | `DesignerFont` | kept as-is | `@font-face` per used family/weight/style, subset, TTF data URI |
| 2 | `SingleLineFont` | `font-family` → bundled single-line font; weight/style reset | `@font-face` for bundled font, subset |
| 3 | `HersheyStrokes` | replaced by `<g>` of stroked `<path>`; `fill="none"` | none |

### Mode 1 — designer font

- Find font file with `fontdb` (system fonts, same load as PNG/PDF export).
- Check OS/2 `fsType` with `ttf-parser`. Restricted (bit 1) → no embedding.
- Subset to used code points with `subsetter`. Base64 with `base64`.
- One `<style>` element in root `<defs>`.
- Missing or restricted font → export `<text>` without `@font-face`; report the font name.

### Mode 2 — single-line font

- Bundle one OFL-1.1 hairline font through `include_bytes!` → no installer change.
- Candidate: Relief SingleLine (isdat-type, OFL-1.1). Verify license file before bundling.
- Ship the font's `OFL.txt` beside it; add to licensing docs.
- Record choice and rationale in `status-docs/seamlylayout_SEAMLYLAYOUT_DECISIONS.md`.

### Mode 3 — Hershey strokes

- Glyph data: Hershey Roman Simplex (`futural`), Usenet distribution, unrestricted-use notice kept.
- Parser: `hershey` crate (Apache-2.0). Hand-roll a small `.jhf` parser only if the crate fails to build.
- Per `<text>`: read `x`, `y`, `font-size`, `fill`, `text-anchor`, string.
- Scale glyph units to `font-size`; baseline at `y`; advance by glyph left/right bounds.
- Emit one `<path d="M… L…">` per glyph, `stroke`=text fill, `fill="none"`, hairline width.
- Replace `<text>` with `<g data-type="label_text" data-text="<string>">` + `<desc>` holding the string.
- Characters without a glyph → `?` glyph; report count.

### Path-only input (Layout.210)

- On import, `finish_import` counts `<text>` in label groups.
- New `AppController` property `labelTextState`: `"text"`, `"pathsOnly"`, `"noLabels"`.
- `"pathsOnly"` → three modes disabled, tooltip explains why.
- `"pathsOnly"` → extra item `SVG (labels as supplied)` exports the DOM verbatim (today's behavior).
- `"noLabels"` → three modes enabled; output identical.

### Frontend (Layout.24)

- `ExportMenu.qml`: `SVG` item → `Menu { title: "SVG" }` with three `MenuItem`s + tooltips.
- New property `svgModeSubmenu` (default `false`). Export menu sets `true`. View menu keeps one `SVG` item.
- Signal `exportSvgRequested(string mode)`; mode strings: `designerFont`, `singleLineFont`, `hersheyStrokes`, `asSupplied`.
- `TopMenuBar.qml` and `Main.qml` pass the mode through. `Main.qml` stages it as `pendingExportSettings`.
- Bridge: `export_svg(path, mode)` → `svg_label_text::apply(doc, mode)` → `do_export_svg`.
- Report non-empty `missing/non-embeddable fonts` through existing `import_warning`-style popup (new `export_warning` signal).

### Preferences (Layout.210.1)

- `PreferencesModel`: new `QString svgTextMode`, default `designerFont`, saved with other preferences.
- Submenu shows a check mark on the last used mode.

## Steps

1. Branch `task-svg-text-modes` from synced `run-seamlyLayout`.
2. Crate skeleton, enum, label/`<text>` discovery, `LabelTextReport`. Unit tests.
3. Mode 3 + Hershey data + license notice. Tests: output is `<path>` with `fill="none"`, no `<text>`, `data-text` kept.
4. Mode 1 + font lookup, `fsType`, subset, data URI. Tests: `<style>` present; restricted/missing font reported.
5. Mode 2 + bundled font + `OFL.txt` + decision record. Tests: `font-family` rewritten, `@font-face` present.
6. Test for Layout.29: `id`/`data-*` identical across modes.
7. Import detection + `labelTextState` property. Rust tests for three states.
8. Bridge `export_svg(path, mode)` + `export_warning` signal.
9. QML submenu, tooltips, gating, View-menu split, preference.
10. `PreferencesModelTests.cpp`: `svgTextMode` round trip + default.
11. Docs: `seamlylayout_svg-data-attributes.md`, `project-docs/SVG-DATA-ATTRIBUTES.md`, `seamlylayout_EXPORT_WORKFLOW.md` (font-license caveat), `TODO_SEAMLYLAYOUT.md`, `SESSION_HANDOVER.md`.
12. Verify: `cargo test --workspace`, `ctest --preset debug`, `local_build_msi.ps1`.
13. End-to-end: export richmond test pattern in all modes; open each in Inkscape/browser.
14. Commit, merge `--no-ff`, push. Full CI (Cargo.toml changes → no skip token).

## Test gaps

- No QML test harness exists. Menu gating logic lives in `labelTextState` (Rust-tested); QML binding checked by hand.
- Test inputs with labels: build fixtures in the Rust tests; the SVGs in `test-seamly-layout-input/` have no label groups.

## Decisions needed

| # | Question | Recommendation |
| - | -------- | -------------- |
| 1 | Mode 2 font | Relief SingleLine, OFL-1.1, bundled with `include_bytes!` |
| 2 | Hershey parser | `hershey` crate; hand-roll only on build failure |
| 3 | Path-only input | Disable three modes; add `SVG (labels as supplied)` item |
| 4 | Font not embeddable/missing (mode 1) | Export `<text>` without font; show warning |
| 5 | Menu test | No new QML harness; Rust tests + manual check |
| 6 | New crate vs module in `cxxqt_bridge` | New crate `svg_label_text` |

## Implementation notes

Approved plan implemented. Differences from the plan:

| Plan | Built | Reason |
|---|---|---|
| Bundle Relief SingleLine CAD TTF | Bundle Relief SingleLine Outline OTF; text names `Relief SingleLine CAD` | CAD TTF open contours render wrongly in browsers ("C" → "O"); see Decision-003 |
| Mode 2 stroked text | Mode 2 filled text | Outline glyphs are filled shapes |
| One `<path>` per glyph | One `<path>` per label line | Same polylines, smaller file |
| Separate `export_warning` popup | Warning text inside the Export Complete dialog | One dialog per export |
| — | `PreferencesModel::saveSvgTextMode` writes one INI key | A full `save()` would store unapplied Preferences edits |
