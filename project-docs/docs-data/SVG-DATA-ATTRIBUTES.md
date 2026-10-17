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
- **Already running** — SeamlyLayout has no single-instance handling: every launch is a new process with its own window, and the app holds one document with no tabs. Seamly2D starts at most one SeamlyLayout per Seamly2D window. While that SeamlyLayout runs, Layout tells the user to close it first, and on Windows brings it to the front.
- **Window in front** — on Windows only the foreground process can bring a window to the front. Seamly2D calls `AllowSetForegroundWindow` for the new process (`WindowsForegroundWindow`, `src/app/seamly2d/core/windows_foreground_window.cpp`); `seamlyLayout_main.mm` then raises and activates its window before the import. Without both, SeamlyLayout opens behind Seamly2D.
- **Untagged input** — the `data-*` tagging is *not* required to open a file. SeamlyLayout treats every top-level `<g>` with geometry as a piece, so an ordinary SVG still lays out. When an imported file contains no `data-type="piece"` group at all, SeamlyLayout shows a non-blocking warning saying so (`AppController::finish_import` → the `import_warning` signal, reached from both `import_svg` and `import_svg_document`), because a file that did not come from Layout Mode will usually not lay out the way the user expects.
- **Piece discovery** — for a *tagged* file the pieces are the `data-type="piece"` groups and nothing else; the untagged "every top-level `<g>`" rule applies only to files with no tagging anywhere. Because this document nests all pieces inside `<g data-type="pattern">` and SeamlyLayout's layout pipeline is built around pieces being direct children of the SVG root, `piece_extractor::hoist_tagged_pieces` re-parents the tagged pieces up to the root (composing any wrapper `transform` onto each one) before the pipeline runs. **A producer-side change that adds another wrapper level, or that stops tagging pieces, silently changes what SeamlyLayout packs** — before this normalisation existed, the whole pattern packed as one sheet-sized "piece".
- **Grain direction** — `svg_dom::verticalize_dom` turns each piece so its grain runs vertically and points up. Source order: a drawn `grainline` group, then `data-grainline-angle`. A piece with neither keeps its drafted orientation; no direction is guessed from the outline. The **No Grainline** layout setting (`noGrainlineRotation`) decides whether the packer may turn such a piece 90° or 270° to fit.
- **Piece identity in the layout** — `id`, `data-name` and `data-letter` are carried through packing into the layout SVG, the piece bbox JSON and the Adjust overlay. Anything a user reads is labelled `data-name` → `data-letter` → `id` (`PieceRect::label()`), so a warning names "Front Bodice" rather than `piece-7`. `id` remains the identity key for element lookup and must stay unique.

## Document shape

```xml
<svg width="..." height="..." viewBox="..." xmlns="http://www.w3.org/2000/svg" ...>
  <g id="pattern-1" data-type="pattern" data-type-number="1" data-name="Pattern Name">
    <g id="piece_Front_Bodice" data-type="piece" data-type-number="1" data-parent="pattern-1"
       data-name="Front Bodice" data-letter="A" data-grainline-angle="90.0000">
      <rect class="piece-fill" id="rect_Front_Bodice" .../>
      <g id="seamline_Front_Bodice" data-type="seamline" data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="cutline_Front_Bodice"  data-type="cutline"  data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="notch_Front_Bodice"    data-type="notch"    data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="internal_path_1_Front_Bodice" data-type="internal_path" data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="cut_path_1_Front_Bodice"      data-type="cut_path"      data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="grainline_Front_Bodice"       data-type="grainline"     data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="piece_label_1_Front_Bodice"   data-type="piece_label"   data-type-number="1" data-parent="Front Bodice">…</g>
      <g id="pattern_label_1_Front_Bodice" data-type="pattern_label" data-type-number="1" data-parent="Front Bodice">…</g>
    </g>
    <g id="piece_Back_Bodice" data-type="piece" data-type-number="2" data-parent="pattern-1" data-name="Back Bodice">…</g>
  </g>
</svg>
```

### Capture a handoff

Set `SEAMLYLAYOUT_SAVE_HANDOFF` to an absolute file path before Seamly2D starts. SeamlyLayout inherits Seamly2D's environment and writes the received document there, unchanged, on each `--svg-stdin` import. A relative path or a failed write shows a warning; the import continues. Code: `cxxqt_bridge/src/handoff_capture.rs`. Use: test fixtures from real patterns.

### Multisize pattern

A pattern with a `.smms` measurement file hands off every size in the size list, at the base height.
Each piece is a `piece-set` group holding that piece once per size.

```xml
<g id="pattern-1" data-type="pattern" data-type-number="1" data-name="Pattern Name"
   data-measurements="multisize" data-sizes="34,36,38" data-base-size="36">
  <g id="piece-set_Front" data-type="piece-set" data-type-number="1" data-parent="pattern-1"
     data-name="Front" data-letter="A">
    <g id="piece_Front_s34" data-type="piece" data-size="34" data-parent="piece-set_Front" ...>
      <g id="seamline_Front_s34" data-type="seamline" data-parent="Front">…</g>
    </g>
    <g id="piece_Front_s36" data-type="piece" data-size="36" data-parent="piece-set_Front" ...>…</g>
  </g>
</g>
```

- Producer: `MainWindow::prepareMultisizePieceLists()` → `MainWindowsNoGUI::generateMultisizePiecesSvgDocument()` → `SvgGenerator::setMultisize()` and `addSvgFromScene(..., size, pieceSetKey)`.
- The same piece is matched across sizes by its `VPiece` id.
- Each size keeps its drafted position. SeamlyLayout stacks the sizes (`svg_dom::center_piece_sets`).
- Tests: `TST_SvgComponentTags::Multisize*` (producer), `piece_extractor` and `layout_utils` `multisize` tests (consumer).

### Size labels

At import, SeamlyLayout adds a label to every multisize piece (`data-size` set): `<g id="size_label_<piece id>" data-type="size_label" data-type-number="1" data-parent="<piece name>">`, the last child of the piece, with one `<text>` "Size <size>" (Arial, 24 px, `text-anchor="middle"`).

- The bottom of the text is 5 px past the front end of the grainline: the last point of the grainline path, as `verticalize_dom` reads the direction. The text reads across the grainline, so it is level when the layout turns the grainline up.
- A piece without a grainline gets the label level at the centre of its outline.
- A piece that has a `size_label` already, from an earlier SeamlyLayout export, keeps it.
- Every consumer treats `size_label` as a label: text modes, HPGL label pen, DXF layer 15 annotation text; it adds nothing to the piece outline.
- Code: `cxxqt_bridge/src/size_labels.rs`, called by `finish_import` after the piece names.

### Nested size colors

On the Nested tab each size has its own color. Every stroke and fill of a piece that is not `none` takes the color of its size; text without a fill gets one. The Marker, Base Size and size tabs keep the drafted colors.

- Colors follow the size order, smallest first: golden-angle hue steps from blue, alternating lightness, relative luminance at most 0.3.
- The legend is `<g id="size_legend" data-type="size_legend">`: a frame, then per size a line in its color and the text "Size <size>". It sits one piece gap below the lowest piece, at the left edge of the content; the media grows to hold it (tiled paper gets more tile rows).
- Every export of the tab keeps the colors and the legend. The paper-sheet PDF repacks the pieces: it keeps the colors, not the legend.
- Text modes and HPGL treat `size_legend` as a label: HPGL plot mode draws it with the label pen; cut mode leaves it out.
- Code: `cxxqt_bridge/src/size_colors.rs`, called by `layout_utils::do_process_layout` and `sheets::build_sheet_export_inputs`.

### Nested SVG export

SeamlyLayout's SVG export of the Nested tab holds one `<g id="size_<size>" data-type="size" data-size="<size>">` per size, holding that size of every piece. The largest size comes first, so it is drawn underneath; the smallest size is on top. Every piece keeps its layout position. The file has no `transform` except on `<text>`. Other exports and the other tabs keep the piece-set or piece groups. Code: `cxxqt_bridge/src/size_groups.rs`.

The `piece-fill` `<rect>` is not part of the C++ handoff — SeamlyLayout inserts it as the
first child of each piece group when it places the piece into a layout sheet
(`layout_assembler.rs::create_layout()`). Its `id` mirrors the piece's own id
(`rect_<name-or-n>`), derived from the piece group's `id` at insertion time.

## Attributes

| Attribute | Applies to | Value |
|---|---|---|
| `data-type` | every tagged `<g>` | One of `pattern`, `piece-set`, `piece`, `seamline`, `cutline`, `internal_path`, `cut_path`, `grainline`, `notch`, `piece_label`, `pattern_label`. SeamlyLayout adds `size_label` (import), `size` (Nested SVG export) and `size_legend` (Nested layout). More types may be added later; consumers must ignore unknown types gracefully. |
| `data-type-number` | every tagged `<g>` | Per-scope 1-based counter for that `data-type`. The pattern is always `1`; pieces count up across the file; component counters reset per piece and per type. |
| `data-parent` | `piece-set`, `piece` and component groups | For a piece: the pattern group's `id` (`pattern-1`), or its piece-set's `id` in a multisize handoff. For a component: the owning piece's `data-name` (e.g. `Front Bodice`), or the piece's numeric fallback `id` (e.g. `piece-3`) when the piece has no name. The pattern group has no `data-parent` (it is the root). True parent/child identity is the DOM nesting. The DXF-ASTM Base Size export reads a piece's value: pieces with the same piece-set id are the sizes of one piece, for its grade rules. |
| `data-name` | `pattern`, `piece` | Pattern name, or piece name. Omitted when empty. SeamlyLayout replaces each piece name at import: see **SeamlyLayout piece names**. |
| `data-letter` | `piece`, `piece-set` | The piece letter, only when one is set on the piece. |
| `data-quantity` | `piece` | Cut count from the piece label data (Piece Properties > Quantity). Written for every piece. DXF-ASTM `Quantity:` piece text. |
| `data-on-fold` | `piece` | `true` when the piece label data says the piece is cut on the fold. Omitted otherwise. |
| `data-measurements` | `pattern` | `individual` or `multisize`. Absent in older handoffs; SeamlyLayout reads absent as `individual`. |
| `data-sizes` | `pattern` (multisize) | Every handed-off size, comma-separated, in pattern units (e.g. `34,36,38`). |
| `data-base-size` | `pattern` (multisize) | Base size of the `.smms` file, in pattern units. |
| `data-size` | `piece` (multisize) | Size of this copy of the piece. Kept on placed pieces in the layout and its exports. DXF-ASTM `Size Name:` piece text. |
| `data-set-number` | `piece` (multisize, SeamlyLayout only) | `data-type-number` of the piece-set the piece left. SeamlyLayout writes it when the Marker tab or a size tab dissolves the piece-sets, so the piece number survives in the layout and its exports. Not in the Seamly2D handoff. DXF-ASTM uses it to number pieces that share a name: `Detail (3)`. |
| `data-name-assigned` | `piece`, `piece-set` (SeamlyLayout only) | `true` when SeamlyLayout assigned the `data-name`. A later import keeps that name. Not in the Seamly2D handoff. |
| `data-split-of` | `piece` (SeamlyLayout only) | `id` of the piece this part was split from. SeamlyLayout writes it when the user chose "Split the pieces" for an individual pattern with a piece wider or longer than the media. Not in the Seamly2D handoff. |
| `data-split-part` | `piece` (SeamlyLayout only) | 1-based number of this part, in reading order (top row first, left to right). |
| `data-split-count` | `piece` (SeamlyLayout only) | Number of parts the piece was split into. |
| `data-sample-size` | `pattern` (individual) | Measurement `bust_circ`, else `waist_circ`, in pattern units. Omitted when the pattern has neither. DXF-ASTM `Size Name:` piece text for every piece. |
| `data-grainline-angle` | `piece` | Grain direction in degrees, counter-clockwise as seen on screen (Qt `QLineF::angle()`); `90` = grain points up. Written with 4 decimals. Present whenever the piece has a grain rotation, **also when the grainline is hidden**, so SeamlyLayout can still orient the piece. Before that transform, a grainline without top and bottom anchor points always points up: an angle in (180°, 360°) is written as the angle 180° less. Includes any Seamly2D layout transform (rotation, mirror). Omitted when the piece has no grain rotation. |
| `data-turn-points` | `seamline`, `cutline`, `internal_path`, `cut_path` | Turn points of the component's path: space-separated, 0-based vertex indices into its `d`, counting each `M` and `L` end point. A turn point is a vertex where the tangent breaks: a piece node, curve end or spline end node whose tangents differ by more than 0.5° where two straight lines meet, or more than 5° where a curve meets a line or curve; a line end where the curve leaves the line at more than 1° and more than twice the bend at the curve's next vertex; a direction change above 25°; or an open-path end. All other vertices are curve points, spline interior nodes included. Empty value = no turn points (a smooth closed loop). Omitted when Seamly2D does not know the node vertices; SeamlyLayout then detects turn points from the geometry. Read by the DXF-ASTM export (D6673 layers 2 and 3). |
| `data-seam-allowances` | `notch` | Seam allowance width in mm at each notch, space-separated, one value per notch in `d` order, 2 decimals. The width is the wider of the notch node's before and after widths, the piece width when the node sets none. Present also for a built-in seam allowance. `0.00` = the piece has no seam allowance. Omitted when unknown. DXF-ASTM slit depth = half the width. |
| `data-notch-type` | `notch` | Seamly2D notch type at each notch, space-separated, one value per notch in `d` order: `slit`, `tNotch`, `uNotch`, `vInternal`, `vExternal`, `castle`, `diamond`. A slit and a 14-segment U-notch give `slit uNotch`. Omitted when unknown. The drawn shape stays in `d`; DXF-ASTM writes every notch as a slit. |
| `data-notch-segments` | `notch` | Line segment count of each notch, space-separated, one value per notch in `d` order. Each segment is one `M … L …` subpath. A slit and a 14-segment U-notch give `1 14`. Sum = `M` count in `d`. Pairs each `data-seam-allowances` and `data-notch-type` value with its subpaths. Omitted when unknown; then the other two notch attributes are omitted too. |

## `id` scheme

- Pattern: `pattern-1` (one pattern per file).
- Piece *n*: `piece_<pieceName>` (e.g. `piece_Front_Bodice`) when the piece has a usable name. Falls back to the legacy numeric `piece-<n>` (n = `data-type-number` of the piece) when the name is empty or entirely non-sanitizable. Piece names are not guaranteed unique by the SVG format itself (`data-letter` is a separate disambiguator), so a name collision between two pieces gets the later piece's `data-type-number` appended (e.g. `piece_Facing-2`) to stay unique — see **Name collision** below.
  - **This id was name-based once before and was reverted** for exactly the uniqueness reason above. It is safe to reintroduce now because Seamly2D's Piece Properties dialog (`PatternPieceDialog::pieceNameChanged()`) blocks entering a piece name already used by another piece in the same pattern — the collision suffix here is defense in depth, not the primary guard.
- Multisize piece-set: `piece-set_<pieceName>` (fallback `piece-set-<n>`). Multisize piece and component ids end in `_s<size>` (e.g. `piece_Front_s34`, `seamline_Front_s34`).
- `piece-fill` `<rect>` (SeamlyLayout-side, not part of the C++ handoff): `rect_<pieceName>` or `rect-<n>`, mirroring whichever form the piece's own `id` took.
- Component, one per piece today (`seamline`, `cutline`, `grainline`, `notch`): `<type>_<pieceName>` (e.g. `grainline_Front_Bodice`).
- Component, can repeat per piece (`internal_path`, `cut_path`, `piece_label`, `pattern_label`): `<type>_<m>_<pieceName>` (e.g. `internal_path_3_Front_Bodice`), where *m* is that type's counter within the piece.
- `pieceName` is the piece's `data-name`, sanitized to a valid XML id: every run of characters outside `[A-Za-z0-9_.-]` becomes `_`, and a leading digit gets a `_` prefix (e.g. piece name `2" Front/Bodice` → id fragment `_2_Front_Bodice`).
- **No usable piece name** (empty or entirely non-sanitizable) — the component falls back to the legacy `<pieceId>-<type>-<m>` id (e.g. `piece-3-seamline-1`), which is unique by construction.
- **Name collision** — two pieces sharing a (sanitized) name would otherwise produce colliding ids; the later one gets its piece's `data-type-number` appended (e.g. `grainline_Facing-2`, `piece_Facing-2`) to stay unique.

All ids are unique and XML-valid by construction. **Breaking change vs. pre-contract exports:** the piece `id` was previously the raw piece name; the name now lives in `data-name`. Component ids changed again from the counter-based `<pieceId>-<type>-<m>` scheme to the name-based scheme above; SeamlyLayout's decoration-exclusion matchers (`is_non_outline_group()` in `piece_extractor.rs` and `svg_extract.rs`) already recognize both forms via a `data-type` exact-match check, so this is not a breaking change for that consumer.

## Guarantees

- Every `<g>` under `pattern-1` carries `data-type`, `data-type-number`, and `data-parent`.
- No empty `<g>` elements and no spurious `M0,0` / empty-`d` paths (Qt generator artifacts are stripped).
- Components that paint nothing (e.g. a piece without notches or internal paths) are simply absent — consumers must not assume every type exists in every piece.
- Component geometry is emitted in the merged document's single coordinate space (the flat-arranged paper; `viewBox` in scene units at the generator resolution). No transforms are introduced beyond what Qt's SVG generator emits inside the groups.
- Label groups contain real `<text>` elements when "text as paths" is off (label lines are rendered by `SvgTextItem`, `src/libs/vlayout/svg_text_item.cpp`, which paints through `QPainter::drawText()` so Qt's SVG engine emits `<text>` with the label's `font-family`, `font-size`, `font-weight`/`font-style` and fill color), and `<path>` glyph outlines when on (`--text2paths` / "text as paths"); the Layout Mode handoff always keeps real text.

## Semantics / notes

- **`seamline`** — the sew line of the piece. Omitted when the seam line is hidden (Piece Properties > Hide seam line).
- **`cutline`** — the seam-allowance outline (cut line). Pieces drawn without a seam allowance may have no `cutline` group. SeamlyLayout packs a piece by its `cutline`, else by its `seamline`.
- **Built-in seam allowance** — the main path is the cut line, sent as `cutline` also when seam allowances are not shown. Seamly2D computes the `seamline`: the main path offset inward by each node's width, the wider of its before and after widths, the piece width when the node sets none. A curve spreads the widths of its ends along its length. Corner types shape only the cut line: seam line corners are plain offset corners. A hidden seam line is omitted, as for any piece. Producer: `VPiece::builtInSeamLinePoints()`, `VLayoutPiece::createSeamlineItem()` and `createAllowanceItem()`. Before this rule the main path was sent as `seamline` and no sew line was sent.
- **`notch`** — all notches of a piece in one group for now; per-notch splitting is a possible follow-on if the nesting algorithm needs individual notches.
- **`internal_path`** — one group per plain (non-cutout) internal path of the piece.
- **`cut_path`** — one group per internal *cutout* path: a closed path that is cut out of the piece (a hole) and may carry its own seam allowance. Distinguished in the pattern data by `VLayoutPiecePath::isCutPath()`; the nesting algorithm may treat cutout interiors as usable area, unlike `internal_path` markings.
- **`grainline`** — grainline arrow geometry.
- **`data-turn-points`** — Seamly2D knows which vertices are nodes; the interpolated polyline alone does not. A coarse curve interpolation has long single segments, so geometry cannot tell them from straight lines. Producer: `VAbstractPiece::turnPointIndices()` (`src/libs/vlayout/vabstractpiece.cpp`), with node vertices from `VPiecePath::pathNodeVertices()` and `VAbstractPiece::Equidistant()`. Consumer: `astm_contour::build_contour_tagged` (`seamly_svg2ezdxf`). The indices stay valid through SeamlyLayout's transform baking, which maps path segments 1:1. Multisize: a node that is a turn point in one size of a piece is a turn point in every size of that piece, line ends included, so DXF-ASTM grading finds the same turn points in each size. SeamlyLayout adds no turn point to a tagged path. Nodes match by their order along the line; a closed loop that starts at another node in another size is matched by node position. A line whose node count differs between sizes keeps the turn points of each size. A spline interior node is not a node, so a size that moves one onto a curve segment keeps the node count. Seamly2D writes each piece whose node count still differs between sizes to its log (category `v.mainnoguiwindow`). Producer: `VLayoutPiece::unifyTurnPoints()`, from `MainWindow::prepareMultisizePieceLists()`.
- **`data-seam-allowances`** — SeamlyLayout uses these widths before it measures the cut line to sew line distance. They are the only width source for a piece without a `cutline`. Producer: `VPiece::createNotchLines()` (width per notch, `VAbstractPiece::MaxLocalSA`), `VLayoutPiece::createNotchesItem()`. Consumer: `converter::notch_polylines`, `astm_notch::build_notches` (`seamly_svg2ezdxf`).
- **`data-notch-segments`** — `converter::notch_polylines` gives each subpath the width of its notch. The widths are ignored when the two lists differ in length or the counts do not sum to the subpath count. Without `data-notch-segments` (an older SVG), each width covers one subpath. Producer: `VPiece::createNotchLines()` (line count per notch), `VLayoutPiece::setNotches()` (drops counts that do not sum to the line count), `VLayoutPiece::createNotchesItem()`. A notch node that draws no line has no entry in any notch attribute.
- **`data-quantity` / `data-on-fold`** — piece data, so the DXF-ASTM export no longer parses a "Cut N" label line. Producer: `VPieceLabelData::GetQuantity()` / `IsOnFold()` via `VLayoutPiece::Create()`. Consumer: `converter::quantity_from_piece` (`seamly_svg2ezdxf`): even N → `N/2,N/2`, odd N → `N,0`, on fold → `N,0`. The label line is the fallback for an SVG without `data-quantity`.
- **`data-notch-type`** — the notch type Seamly2D drew. Producer: `VPiece::createNotchLines()` (type per notch), `VLayoutPiece::createNotchesItem()`. SeamlyLayout keeps it on the notch group through layout and export. DXF-ASTM ignores it: every notch is a layer 4 slit.
- **`piece_label` / `pattern_label`** — the on-piece label text blocks. One `<text>` element per label line (or one `<path>` per line in text-as-paths mode); per-line bold/italic, alignment, middle-eliding to the label width, mirroring and rotation are preserved in either mode.
- Counters are per SvgGenerator instance: one instance = one file = one pattern.

## SeamlyLayout piece names

SeamlyLayout gives every tagged piece a unique `data-name` at import, before any layout. Code: `crates/cxxqt_bridge/src/piece_naming.rs`.

| Pattern | Piece `data-name` |
|---|---|
| Individual | `Piece_<n>[_<name>][_<letter>]` |
| Multisize | `Size_<size>_Piece_<n>[_<name>][_<letter>]` |

- `<n>`: 1-based position of the piece among the pieces of its size, in document order. One piece has the same number in every size.
- `<name>`, `<letter>`: the Seamly2D `data-name` and `data-letter`, trimmed. Each is left out when absent.
- Each run of whitespace in the name becomes one `_`: `Detail 1` and `Detail  1` give `Detail_1`.
- Examples: `Piece_1`, `Piece_2_B`, `Piece_3_Front`, `Piece_4_Back_D`, `Size_220_Piece_3_Detail_1_C`.
- A multisize piece-set gets the name of its pieces without the size: `Piece_3_Detail_1_C`.
- Component `data-parent` takes the new name.
- Each renamed group gets `data-name-assigned="true"`. A group that has it keeps its name, so a re-imported export is not numbered again.
- Sizes with different piece counts: the import still names the pieces and shows a warning that lists those sizes.
- The layout, the Adjust overlay, the dialogs and DXF-ASTM `Piece Name:` use the new name. A multisize label adds no "(size N)".
- The label text drawn on a piece does not change.

## SeamlyLayout split parts (individual pattern)

When a piece of an individual pattern is wider or longer than the media, the user can split it. Code: `crates/cxxqt_bridge/src/piece_divider.rs`.

- Each part is a `data-type="piece"` group with the piece's attributes, plus `data-split-of`, `data-split-part`, `data-split-count`.
- Part `id`: `<piece id>_part<i>`. Every id inside the part gets `_part<i>`.
- Part `data-name`: `<name> (<i>/<n>)`, e.g. `Front (1/2)`. DXF-ASTM `Piece Name:` uses it.
- `cutline` and `cut_path`: closed, with new edges on the cut lines. No seam allowance on a cut edge.
- `seamline` (beside a cutline), `internal_path`, `grainline`: open lines that end at the cut lines.
- `data-turn-points`: kept vertices keep their flag; new vertices on a cut line are turn points.
- Notches, `piece_label`, `pattern_label`: whole, on the part that holds them.
- A part without a grainline gets a new vertical one when the piece had one.
- New `piece_label` per part: `<name> part <i> of <n>, joins <j>`.
- 2 match notches per shared cut edge, in the part's `notch` group: `data-notch-type` `slit`, `data-seam-allowances` `12.70` (6.35 mm deep).

## SeamlyLayout SVG export — label text modes

Export > SVG offers three label text modes. They change only the `<text>` inside
`piece_label` and `pattern_label` groups. Group `id`, `data-*` attributes and
group structure are the same in every mode. Code: `crates/svg_label_text/`.

| Mode (QML name) | Label line in the exported file |
|---|---|
| Text: designer font (`designerFont`) | `<text>` unchanged. Root `<defs><style>` holds a subset `@font-face` per used face. |
| Text: single-line font (`singleLineFont`) | `<text>` with `font-family="Relief SingleLine CAD"`, weight 400, normal style. `@font-face` embeds the bundled font. |
| Paths: single-stroke (`hersheyStrokes`) | `<g data-type="label_text" data-text="…">` with a `<desc>` and one stroked, unfilled `<path>` of Hershey polylines. |

- `label_text` is new and export-only. Seamly2D never writes it. It sits inside a label group and keeps the text's `id` and `transform`.
- `data-text` holds the label string, so machines can still read it.
- Labels that are already paths (`--text2paths`) disable all three modes. The menu then offers "As supplied", which exports the layout unchanged.
