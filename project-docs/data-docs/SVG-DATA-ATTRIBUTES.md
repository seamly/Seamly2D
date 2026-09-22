# SVG `data-*` Attribute Contract — Seamly2D → SeamlyLayout

**Status:** implemented (Seamly2D branch `run-seamlyLayout`)
**Producer:** Seamly2D — `SvgGenerator` (`src/libs/vformat/svg_generator.cpp`) fed by the piece item tree built in `VLayoutPiece::GetItem()` (`src/libs/vlayout/vlayoutpiece.cpp`)
**Consumer:** SeamlyLayout (SVG parsed via its `svg_dom` crate)
**Source spec:** `project-docs/NEW-ATTRIBUTES.csv` — this document is the authoritative, expanded contract; keep both apps developing against it.

## When tagged SVGs are produced

1. **Layout Mode handoff** — clicking Layout Mode in Seamly2D builds the tagged pieces SVG in memory, launches SeamlyLayout with `--svg-stdin`, and writes that document to the new process's standard input. No file is written.
2. **Manual piece exports** — Piece mode → Export Pieces → SVG carries the same attributes (with or without "text as paths").

Whole-scene exports (draft blocks) keep the legacy untagged single-group structure; only piece-based exports are tagged.

## Launch contract

The handoff in (1) is a process launch, and both halves of it are pinned by tests so they cannot drift apart.

| | Producer — Seamly2D | Consumer — SeamlyLayout |
|---|---|---|
| Code | `MainWindow::exportPiecesToSeamlyLayout()` via `MainWindowsNoGUI::generatePiecesSvgDocument()`, `SeamlySuitePaths::patternDocumentName()` and `SeamlySuitePaths::seamlyLayoutLaunchArguments()` (`src/libs/vmisc/seamly_suite_paths.cpp`) | `StartupOptions::parse()` (`src/app/seamlylayout/qt_frontend/src/StartupOptions.cpp`), dispatched from `seamlyLayout_main.mm` into `Main.qml`'s `openSvgDocument()` |
| Tests | `TST_SeamlySuitePaths` (`src/test/Seamly2DTest`) | `StartupOptionsTests` (`src/test/SeamlyLayoutTest`) |

**The contract:**

- **Transport** — the piece-mode document is a **stringified SVG on standard input**, not a file. Seamly2D builds it with `generatePiecesSvgDocument()`, starts SeamlyLayout, writes the UTF-8 bytes to the child's standard input and closes the write channel; SeamlyLayout reads to end. Nothing is written to disk, so no handoff file is left beside the pattern and an unsaved or read-only pattern no longer blocks Layout Mode. Until Seamly2D.5 the handoff wrote `<pattern-basename>.pieces.svg` and passed its path.
- **Command line** — `SeamlyLayout --svg-stdin [--document-name <pattern base name>]`, launched with the SeamlyLayout executable's own directory as the working directory. The launch is *not* detached: the standard-input channel needs a live `QProcess`, which is also what lets Seamly2D restore the prior mode when SeamlyLayout exits (Seamly2D.3).
- **Document name** — `--document-name` carries the pattern's complete base name (`shirt.v2.sm2d` → `shirt.v2`), because a document with no file has no name for SeamlyLayout's default export file names. It is omitted, never passed empty, for an unsaved pattern.
- **Still accepted** — a single positional `<file.svg>` (the CLI and any file association), `-h` / `--help` and `-v` / `--version` (shown in a dialog, exit 0), and no argument at all (empty canvas). Anything else — `--svg-stdin` with a file as well, `--svg-stdin` with empty or non-SVG input, a second positional argument, an unknown option, a missing/unreadable/non-`.svg` file — is reported to the user in SeamlyLayout's error dialog, and the application then continues with an empty canvas rather than exiting.
- **Exit codes** — `0` for `--help` / `--version` and for a normal session; `-1` when the QML root object fails to load. A rejected argument is *not* an exit code: the window is already the place the message has to appear, because this is a WIN32-subsystem binary with no console.
- **Already running** — no single-instance handling: every launch is a new process with its own window, so a second Layout Mode handoff opens a second SeamlyLayout. This is deliberate — the app holds one document with no tabs, and comparing two layouts side by side is useful. Seamly2D does not track or reuse a previously launched instance.
- **Untagged input** — the `data-*` tagging is *not* required to open a file. SeamlyLayout treats every top-level `<g>` with geometry as a piece, so an ordinary SVG still lays out. When an imported file contains no `data-type="piece"` group at all, SeamlyLayout shows a non-blocking warning saying so (`AppController::finish_import` → the `import_warning` signal, reached from both `import_svg` and `import_svg_document`), because a file that did not come from Layout Mode will usually not lay out the way the user expects.
- **Piece discovery** — for a *tagged* file the pieces are the `data-type="piece"` groups and nothing else; the untagged "every top-level `<g>`" rule applies only to files with no tagging anywhere. Because this document nests all pieces inside `<g data-type="pattern">` and SeamlyLayout's layout pipeline is built around pieces being direct children of the SVG root, `piece_extractor::hoist_tagged_pieces` re-parents the tagged pieces up to the root (composing any wrapper `transform` onto each one) before the pipeline runs. **A producer-side change that adds another wrapper level, or that stops tagging pieces, silently changes what SeamlyLayout packs** — before this normalisation existed, the whole pattern packed as one sheet-sized "piece".
- **Piece identity in the layout** — `id`, `data-name` and `data-letter` are carried through packing into the layout SVG, the piece bbox JSON and the Adjust overlay. Anything a user reads is labelled `data-name` → `data-letter` → `id` (`PieceRect::label()`), so a warning names "Front Bodice" rather than `piece-7`. `id` remains the identity key for element lookup and must stay unique.

## Document shape

```xml
<svg width="..." height="..." viewBox="..." xmlns="http://www.w3.org/2000/svg" ...>
  <g id="pattern-1" data-type="pattern" data-type-number="1" data-name="Pattern Name">
    <g id="piece-1" data-type="piece" data-type-number="1" data-parent="pattern-1"
       data-name="Front Bodice" data-letter="A">
      <g id="seamline_Front_Bodice" data-type="seamline" data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="cutline_Front_Bodice"  data-type="cutline"  data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="notch_Front_Bodice"    data-type="notch"    data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="internal_path_1_Front_Bodice" data-type="internal_path" data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="cut_path_1_Front_Bodice"      data-type="cut_path"      data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="grainline_Front_Bodice"       data-type="grainline"     data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="piece_label_1_Front_Bodice"   data-type="piece_label"   data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="pattern_label_1_Front_Bodice" data-type="pattern_label" data-type-number="1" data-parent="Front Bodice">…</g>
    </g>
    <g id="piece-2" data-type="piece" data-type-number="2" data-parent="pattern-1" data-name="Back Bodice">…</g>
  </g>
</svg>
```

## Attributes

| Attribute | Applies to | Value |
|---|---|---|
| `data-type` | every tagged `<g>` | One of `pattern`, `piece`, `seamline`, `cutline`, `internal_path`, `cut_path`, `grainline`, `notch`, `piece_label`, `pattern_label`. More types may be added later; consumers must ignore unknown types gracefully. |
| `data-type-number` | every tagged `<g>` | Per-scope 1-based counter for that `data-type`. The pattern is always `1`; pieces count up across the file; component counters reset per piece and per type. |
| `data-parent` | `piece` and component groups | For a piece: the pattern group's `id` (`pattern-1`). For a component: the owning piece's `data-name` (e.g. `Front Bodice`), or the piece's `id` (e.g. `piece-3`) when the piece has no name. The pattern group has no `data-parent` (it is the root). Not read by any SeamlyLayout code today — true parent/child identity is the DOM nesting — so this is a documentation-level cross-reference, not a lookup key. |
| `data-name` | `pattern`, `piece` | Pattern name, or piece name. Omitted when empty. |
| `data-letter` | `piece` | The piece letter, only when one is set on the piece. |

## `id` scheme

- Pattern: `pattern-1` (one pattern per file).
- Piece *n*: `piece-<n>` (n = `data-type-number` of the piece). **Deliberately not name-based** — piece names aren't guaranteed unique (`data-letter` is the disambiguator for that), and a prior version of this contract used the raw name as the piece `id` and had to revert it for exactly that reason.
- Component, one per piece today (`seamline`, `cutline`, `grainline`, `notch`): `<type>_<pieceName>` (e.g. `grainline_Front_Bodice`).
- Component, can repeat per piece (`internal_path`, `cut_path`, `piece_label`, `pattern_label`): `<type>_<m>_<pieceName>` (e.g. `internal_path_3_Front_Bodice`), where *m* is that type's counter within the piece.
- `pieceName` is the piece's `data-name`, sanitized to a valid XML id: every run of characters outside `[A-Za-z0-9_.-]` becomes `_`, and a leading digit gets a `_` prefix (e.g. piece name `2" Front/Bodice` → id fragment `_2_Front_Bodice`).
- **No usable piece name** (empty or entirely non-sanitizable) — the component falls back to the legacy `<pieceId>-<type>-<m>` id (e.g. `piece-3-seamline-1`), which is unique by construction.
- **Name collision** — two pieces sharing a (sanitized) name would otherwise produce colliding component ids; the later one gets its piece's `data-type-number` appended (e.g. `grainline_Facing-2`) to stay unique.

All ids are unique and XML-valid by construction. **Breaking change vs. pre-contract exports:** the piece `id` was previously the raw piece name; the name now lives in `data-name`. Component ids changed again from the counter-based `<pieceId>-<type>-<m>` scheme to the name-based scheme above; SeamlyLayout's decoration-exclusion matchers (`is_non_outline_group()` in `piece_extractor.rs` and `svg_extract.rs`) already recognize both forms via a `data-type` exact-match check, so this is not a breaking change for that consumer.

## Guarantees

- Every `<g>` under `pattern-1` carries `data-type`, `data-type-number`, and `data-parent`.
- No empty `<g>` elements and no spurious `M0,0` / empty-`d` paths (Qt generator artifacts are stripped).
- Components that paint nothing (e.g. a piece without notches or internal paths) are simply absent — consumers must not assume every type exists in every piece.
- Component geometry is emitted in the merged document's single coordinate space (the flat-arranged paper; `viewBox` in scene units at the generator resolution). No transforms are introduced beyond what Qt's SVG generator emits inside the groups.
- Label groups contain real `<text>` elements when "text as paths" is off (label lines are rendered by `SvgTextItem`, `src/libs/vlayout/svg_text_item.cpp`, which paints through `QPainter::drawText()` so Qt's SVG engine emits `<text>` with the label's `font-family`, `font-size`, `font-weight`/`font-style` and fill color), and `<path>` glyph outlines when on (`--text2paths` / "text as paths"); the Layout Mode handoff always keeps real text.

## Semantics / notes

- **`seamline`** — the sew line of the piece.
- **`cutline`** — the seam-allowance outline (cut line). Pieces drawn without a seam allowance may have no `cutline` group.
- **`notch`** — all notches of a piece in one group for now; per-notch splitting is a possible follow-on if the nesting algorithm needs individual notches.
- **`internal_path`** — one group per plain (non-cutout) internal path of the piece.
- **`cut_path`** — one group per internal *cutout* path: a closed path that is cut out of the piece (a hole) and may carry its own seam allowance. Distinguished in the pattern data by `VLayoutPiecePath::isCutPath()`; the nesting algorithm may treat cutout interiors as usable area, unlike `internal_path` markings.
- **`grainline`** — grainline arrow geometry.
- **`piece_label` / `pattern_label`** — the on-piece label text blocks. One `<text>` element per label line (or one `<path>` per line in text-as-paths mode); per-line bold/italic, alignment, middle-eliding to the label width, mirroring and rotation are preserved in either mode.
- Counters are per SvgGenerator instance: one instance = one file = one pattern.
