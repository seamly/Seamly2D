# SVG `data-*` Attribute Contract — Seamly2D → SeamlyLayout

**Status:** implemented (Seamly2D branch `run-seamlyLayout`)
**Producer:** Seamly2D — `SvgGenerator` (`src/libs/vformat/svg_generator.cpp`) fed by the piece item tree built in `VLayoutPiece::GetItem()` (`src/libs/vlayout/vlayoutpiece.cpp`)
**Consumer:** SeamlyLayout (SVG parsed via its `svg_dom` crate)
**Source spec:** `project-docs/NEW-ATTRIBUTES.csv` — this document is the authoritative, expanded contract; keep both apps developing against it.

## When tagged SVGs are produced

1. **Layout Mode handoff** — clicking Layout Mode in Seamly2D writes `<pattern-basename>.pieces.svg` next to the saved pattern file, then launches SeamlyLayout with that path as its single command-line argument.
2. **Manual piece exports** — Piece mode → Export Pieces → SVG carries the same attributes (with or without "text as paths").

Whole-scene exports (draft blocks) keep the legacy untagged single-group structure; only piece-based exports are tagged.

## Launch contract (Task 49)

The handoff in (1) is a process launch, and both halves of it are pinned by tests so they cannot drift apart.

| | Producer — Seamly2D | Consumer — SeamlyLayout |
|---|---|---|
| Code | `MainWindow::exportPiecesToSeamlyLayout()` via `SeamlyFamilyPaths::piecesSvgFilePath()` and `SeamlyFamilyPaths::seamlyLayoutLaunchArguments()` (`src/libs/vmisc/seamly_family_paths.cpp`) | `StartupOptions::parse()` (`src/app/seamlylayout/qt_frontend/src/StartupOptions.cpp`), dispatched from `main.cpp` into `Main.qml`'s `openSvgFile()` |
| Tests | `TST_SeamlyFamilyPaths` (`src/test/Seamly2DTest`) | `StartupOptionsTests` (`src/test/SeamlyLayoutTest`) |

**The contract:**

- **File name** — `<pattern complete base name>.pieces.svg`, in the pattern file's own directory. `richmond-shirt.sm2d` → `richmond-shirt.pieces.svg`; `shirt.v2.sm2d` → `shirt.v2.pieces.svg` (only the last extension is replaced).
- **Command line** — `SeamlyLayout <absolute path to the .pieces.svg>`, launched detached with the SeamlyLayout executable's own directory as the working directory. **Exactly one positional argument**; the path is passed as a single argument-vector element, so spaces in it need no quoting by the caller.
- **Also accepted** — `-h` / `--help` and `-v` / `--version` (shown in a dialog, exit 0), and no argument at all (empty canvas). Anything else — a second positional argument, an unknown option, a missing/unreadable/non-`.svg` file — is reported to the user in SeamlyLayout's error dialog, and the application then continues with an empty canvas rather than exiting.
- **Exit codes** — `0` for `--help` / `--version` and for a normal session; `-1` when the QML root object fails to load. A rejected argument is *not* an exit code: the window is already the place the message has to appear, because the launch is detached and has no console.
- **Already running** — no single-instance handling: every launch is a new process with its own window, so a second Layout Mode handoff opens a second SeamlyLayout. This is deliberate — the app holds one document with no tabs, and comparing two layouts side by side is useful. Seamly2D does not track or reuse a previously launched instance.
- **Untagged input** — the `data-*` tagging is *not* required to open a file. SeamlyLayout treats every top-level `<g>` with geometry as a piece, so an ordinary SVG still lays out. When an imported file contains no `data-type="piece"` group at all, SeamlyLayout shows a non-blocking warning saying so (`AppController::import_svg` → the `import_warning` signal), because a file that did not come from Layout Mode will usually not lay out the way the user expects.
- **Piece discovery (Task 59)** — for a *tagged* file the pieces are the `data-type="piece"` groups and nothing else; the untagged "every top-level `<g>`" rule applies only to files with no tagging anywhere. Because this document nests all pieces inside `<g data-type="pattern">` and SeamlyLayout's layout pipeline is built around pieces being direct children of the SVG root, `piece_extractor::hoist_tagged_pieces` re-parents the tagged pieces up to the root (composing any wrapper `transform` onto each one) before the pipeline runs. **A producer-side change that adds another wrapper level, or that stops tagging pieces, silently changes what SeamlyLayout packs** — before this normalisation existed, the whole pattern packed as one sheet-sized "piece".
- **Piece identity in the layout** — `id`, `data-name` and `data-letter` are carried through packing into the layout SVG, the piece bbox JSON and the Adjust overlay. Anything a user reads is labelled `data-name` → `data-letter` → `id` (`PieceRect::label()`), so a warning names "Front Bodice" rather than `piece-7`. `id` remains the identity key for element lookup and must stay unique.

## Document shape

```xml
<svg width="..." height="..." viewBox="..." xmlns="http://www.w3.org/2000/svg" ...>
  <g id="pattern-1" data-type="pattern" data-type-number="1" data-name="Pattern Name">
    <g id="piece-1" data-type="piece" data-type-number="1" data-parent="pattern-1"
       data-name="Front Bodice" data-letter="A">
      <g id="piece-1-seamline-1" data-type="seamline" data-type-number="1" data-parent="piece-1">…</g>
      <g id="piece-1-cutline-1"  data-type="cutline"  data-type-number="1" data-parent="piece-1">…</g>
      <g id="piece-1-notch-1"    data-type="notch"    data-type-number="1" data-parent="piece-1">…</g>
      <g id="piece-1-internal_path-1" data-type="internal_path" data-type-number="1" data-parent="piece-1">…</g>
      <g id="piece-1-cut_path-1"      data-type="cut_path"      data-type-number="1" data-parent="piece-1">…</g>
      <g id="piece-1-grainline-1"     data-type="grainline"     data-type-number="1" data-parent="piece-1">…</g>
      <g id="piece-1-piece_label-1"   data-type="piece_label"   data-type-number="1" data-parent="piece-1">…</g>
      <g id="piece-1-pattern_label-1" data-type="pattern_label" data-type-number="1" data-parent="piece-1">…</g>
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
| `data-parent` | `piece` and component groups | For a piece: the pattern group's `id` (`pattern-1`). For a component: the owning piece group's `id` (e.g. `piece-3`). The pattern group has no `data-parent` (it is the root). |
| `data-name` | `pattern`, `piece` | Pattern name, or piece name. Omitted when empty. |
| `data-letter` | `piece` | The piece letter, only when one is set on the piece. |

## `id` scheme

- Pattern: `pattern-1` (one pattern per file).
- Piece *n*: `piece-<n>` (n = `data-type-number` of the piece).
- Component: `<pieceId>-<type>-<m>` (e.g. `piece-2-internal_path-3`), where *m* is that type's counter within the piece.

All ids are unique and XML-valid by construction. **Breaking change vs. pre-contract exports:** the piece `id` was previously the raw piece name; the name now lives in `data-name`.

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
