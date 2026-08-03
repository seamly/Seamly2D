// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file piece_extractor.rs
// @brief Extracts pattern piece bounding boxes from a `svg_dom::Document` for
//        use with `packing::pack_shelves` / `packing::pack_pieces`.
//
// Piece discovery has two modes, chosen per document:
//
//   * **Tagged** — the file came from Seamly2D's Layout Mode handoff and carries
//     `data-type="piece"` groups (see `project-docs/SVG-DATA-ATTRIBUTES.md`).
//     Only those groups are pieces; everything else at the root is ignored.
//   * **Untagged** — an ordinary drawing with no `data-*` tagging.  Every
//     top-level `<g>` with geometry is treated as one piece, which is what makes
//     a hand-drawn SVG lay out.  This is the historical rule and is kept as the
//     fallback (see the Task 49 `import_warning` contract in this app's CLAUDE.md).
//
// The handoff nests every piece inside a single `<g data-type="pattern">` wrapper,
// so `hoist_tagged_pieces` re-parents the tagged pieces up to the SVG root before
// the layout pipeline runs.  After that one normalisation the whole pipeline —
// `verticalize_dom`, `translate_dom`, extraction, `layout_assembler`, `oversized`,
// `remaining`, `sheets` — sees the flat "one piece per top-level `<g>`" shape it
// has always assumed, and `PieceRect::group_index` stays a valid index into the
// root's `<g>` children.
//
// The bounding box is computed by collecting all coordinate points from every
// descendant `<path>` element, parsing the `d` attribute via `geometry::Path`.
//
// This mirrors the `collect_path_points` + `BoundingBox::from_points` pattern
// Used by the Qt bridge layout pipeline and exported from this crate's lib root.

use layout_tiling::measurement_to_px;
use geometry::{BoundingBox, Path, PathSegment, Point};
use packing::Rect;
use xmltree::{Element, XMLNode};

// @brief One extracted pattern piece ready for bin packing.
//
// Holds the `Rect` (integer pixel dimensions), the piece's identity attributes,
// and `group_index` so Phase 8c can locate the original `<g>` element.
#[derive(Debug, Clone)]
pub struct PieceRect {
    // Dimensions in pixels at the DPI used during extraction.
    pub rect: Rect,
    // Value of the `id` attribute on the piece's top-level `<g>` element.
    // Empty string if the element has no id.  This is the piece's *identity*
    // (`piece-7`), not its display name — use `label()` for anything a user reads.
    pub id: String,
    // Value of `data-name` — the human-readable piece name ("Front Bodice").
    // Empty for untagged SVGs and for tagged pieces whose name is blank
    // (Seamly2D omits the attribute when the name is empty).
    pub name: String,
    // Value of `data-letter` — the piece letter ("A"), only set when the pattern
    // assigns one.  Empty otherwise.
    pub letter: String,
    // Bounding-box origin in SVG user units — used by Phase 8c to compute
    // the translate offset so pieces are packed at (0,0) within their slot.
    // keep these pixel-precise as f64 for accurate translate offsets; the Rect will be rounded to u32 for packing
    pub origin_x: f64,
    pub origin_y: f64,
    // Index of this piece's `<g>` within the ordered list of ALL top-level
    // `<g>` children of the SVG root (0-based, counting only `<g>` elements).
    // Used by `layout_assembler` to retrieve the original element even when
    // some `<g>` elements were skipped (empty paths, degenerate size).
    pub group_index: usize,
}

impl PieceRect {
    // @brief The name to show a user for this piece.
    //
    // Prefers `data-name` ("Front Bodice"), then `data-letter` ("A"), then the
    // raw `id` ("piece-7").  Used for the unplaced-piece warning, the packing
    // error messages and the Adjust overlay — never for identity lookups, which
    // must keep using `id`.
    //
    // @return Borrowed display label; never empty unless the piece has no id either.
    pub fn label(&self) -> &str {
        if !self.name.is_empty() {
            return self.name.as_str(); // data-name is the best label
        } // if named
        if !self.letter.is_empty() {
            return self.letter.as_str(); // fall back to the piece letter
        } // if lettered
        self.id.as_str() // last resort: the machine id
    } // fn label
} // impl PieceRect

// @brief Extract bounding boxes for every pattern piece in `doc`.
//
// Piece discovery follows the two modes described in this file's header: when the
// document contains any `data-type="piece"` element (a Seamly2D handoff) only the
// tagged groups are pieces; otherwise every direct child `<g>` is one piece.
// Pieces with no parseable `<path>` data (empty, text-only, etc.) are skipped in
// both modes.  Call `hoist_tagged_pieces` first — a handoff whose pieces are still
// nested inside their pattern wrapper yields nothing here, by design.
//
// @param doc SVG document previously loaded by `app_core::load_svg`.
// @return `Vec<PieceRect>` — one entry per non-empty piece group.
//         Returns an empty `Vec` if no pieces could be extracted.
pub fn extract_piece_rects(doc: &svg_dom::Document) -> Vec<PieceRect> {
    use layout_tiling::LAYOUT_PPI;
    // Determine SVG user-units-per-inch from the SVG root's viewBox / width attributes.
    // If no viewBox is present, assume 1 user unit = 1 px (i.e., scale = 1.0).
    let uu_per_px = svg_uu_per_px(&doc.root);

    // Tagged handoff or untagged drawing?  Decided once for the whole document so
    // a stray untagged group cannot slip into a tagged pattern's piece list.
    let tagged_mode = document_has_tagged_pieces(&doc.root);

    let mut pieces = Vec::new();
    // Counts every <g> child of the SVG root (including ones that are skipped).
    // Stored in PieceRect::group_index so the assembler can look up the element.
    let mut g_idx: usize = 0;

    // Iterate direct children of the SVG root; each <g> is a candidate piece.
    for child in &doc.root.children {
        let XMLNode::Element(elem) = child else {
            continue; // skip text nodes, comments, etc.
        }; // XMLNode::Element

        if elem.name != "g" {
            continue; // skip non-group elements (defs, title, rect background, etc.)
        } // if not <g>

        let this_g_idx = g_idx;
        g_idx += 1; // always increment, even if piece will be skipped

        // In tagged mode the pattern's own wrapper leftovers, legend groups and
        // anything else untagged are NOT pieces — only `data-type="piece"` is.
        if tagged_mode && !is_tagged_piece(elem) {
            continue; // untagged group inside a tagged document — not a piece
        } // if tagged_mode

        let (piece_id, piece_name, piece_letter) = piece_identity(elem);

        // Collect all path points from descendants of this <g>.
        let mut all_points: Vec<Point> = Vec::new();
        collect_all_path_points(elem, &mut all_points);

        // Skip pieces with no path geometry (e.g., label-only groups).
        let Some(bbox) = BoundingBox::from_points(all_points) else {
            continue; // no geometry — skip this group
        }; // BoundingBox::from_points

        // Convert bounding-box dimensions from SVG user units to pixels.
        // uu_per_px: how many user units equal one pixel in this SVG.
        let w_uu = bbox.width() as f64;
        let h_uu = bbox.height() as f64;

        // uu_per_px is in CSS pixels (computed at the SVG standard 96 px/in base).
        // Conversion: user-units → CSS px → output pixels at LAYOUT_PPI.
        //   w_px = (w_uu / uu_per_px) * (LAYOUT_PPI / 96.0)
        let scale = LAYOUT_PPI / (uu_per_px * 96.0);
        let w_px = (w_uu * scale).ceil() as u32;
        let h_px = (h_uu * scale).ceil() as u32;

        // Skip degenerate pieces (zero dimension after rounding).
        if w_px == 0 || h_px == 0 {
            continue; // zero-size — skip
        } // if w_px == 0

        pieces.push(PieceRect {
            rect: Rect::new(w_px, h_px),
            id: piece_id,
            name: piece_name,
            letter: piece_letter,
            origin_x: bbox.min.x as f64,
            origin_y: bbox.min.y as f64,
            group_index: this_g_idx,
        });
    } // for child in doc.root.children

    pieces
} // fn extract_piece_rects

// @brief Extract piece bounding boxes AND cutline polygons in a single walk.
//
// Walks `doc.root.children` once, applying the same discovery and skip rules as
// `extract_piece_rects` (tagged-vs-untagged mode; no path geometry → skipped;
// zero-dim AABB → skipped).
// For each surviving piece, additionally invokes
// `polygon_pack::svg_extract::extract_piece_outline` on the same `<g>` to
// recover the piece's cut silhouette.  When extraction returns `None`
// (no cutline / seamline group, unparseable path, < 3 vertices), the polygon
// falls back to a 4-vertex AABB so the piece still packs — at orthogonal
// trial sets the polygon is ignored anyway, and at non-orthogonal sets the
// AABB-as-polygon yields tight-AABB packing identical to MaxRects.
//
// Polygon vertices are emitted in pixel space at `LAYOUT_PPI` and shifted so
// the polygon's AABB top-left sits at `(0, 0)` — this matches the rect's
// implicit `(0, 0)–to–(w_px, h_px)` frame so the polygon-packer's reported
// `Placed.x/y` agrees with what the rect-packer would have produced.
//
// The two output vectors are guaranteed equal-length and index-aligned:
// `pieces[i].rect` is the AABB of `polygons[i]`.  This is the contract
// `packing::pack_polygons` requires.
//
// @param doc Pre-flattened SVG document (typically `flat_dom` from the bridge
//            pipeline; pieces have already been translated so bbox.min ≈ 0).
// @return    `(pieces, polygons)` of identical length, in document order.
pub fn extract_piece_rects_and_polygons(
    doc: &svg_dom::Document,
) -> (Vec<PieceRect>, Vec<polygon_pack::Polygon>) {
    use layout_tiling::LAYOUT_PPI;

    // Same scale convention as `extract_piece_rects`: user-units → CSS px → output px at LAYOUT_PPI.
    let uu_per_px = svg_uu_per_px(&doc.root);
    let scale = LAYOUT_PPI / (uu_per_px * 96.0);

    // Same discovery decision as `extract_piece_rects` — the two functions must
    // agree on which `<g>` children are pieces or `group_index` diverges.
    let tagged_mode = document_has_tagged_pieces(&doc.root);

    let mut pieces = Vec::new();
    let mut polygons = Vec::new();
    let mut g_idx: usize = 0;

    for child in &doc.root.children {
        let XMLNode::Element(elem) = child else { continue; };
        if elem.name != "g" { continue; }

        let this_g_idx = g_idx;
        g_idx += 1;

        if tagged_mode && !is_tagged_piece(elem) {
            continue; // untagged group inside a tagged document — not a piece
        } // if tagged_mode

        let (piece_id, piece_name, piece_letter) = piece_identity(elem);

        // Identical skip rules to `extract_piece_rects` so the two functions
        // agree on which `<g>` children are pieces.
        let mut all_points: Vec<Point> = Vec::new();
        collect_all_path_points(elem, &mut all_points);
        let Some(bbox) = BoundingBox::from_points(all_points) else {
            continue; // no geometry — skip
        };

        let w_uu = bbox.width() as f64;
        let h_uu = bbox.height() as f64;
        let w_px = (w_uu * scale).ceil() as u32;
        let h_px = (h_uu * scale).ceil() as u32;
        if w_px == 0 || h_px == 0 {
            continue; // zero-size — skip
        }

        let origin_x = bbox.min.x as f64;
        let origin_y = bbox.min.y as f64;

        // Try the cutline / seamline polygon; fall back to the rect outline
        // when the piece has no cutline group or its path is degenerate.
        let polygon = match polygon_pack::svg_extract::extract_piece_outline(elem) {
            Some(poly) => {
                // Shift so polygon AABB.min is at (0,0), then scale user-units
                // → pixels.  After the bridge's translate_dom pass bbox.min is
                // already ~(0,0); the shift is a no-op for that case but keeps
                // the function correct against un-translated test fixtures.
                let scaled: Vec<(f64, f64)> = poly
                    .vertices
                    .iter()
                    .map(|&(x, y)| ((x - origin_x) * scale, (y - origin_y) * scale))
                    .collect();
                polygon_pack::Polygon::new(scaled)
            }
            None => {
                // Rect-as-polygon: 4 vertices CCW from top-left (matches the
                // SVG y-down convention used throughout the layout pipeline).
                polygon_pack::Polygon::new(vec![
                    (0.0,           0.0),
                    (w_px as f64,   0.0),
                    (w_px as f64,   h_px as f64),
                    (0.0,           h_px as f64),
                ])
            }
        };

        pieces.push(PieceRect {
            rect: Rect::new(w_px, h_px),
            id: piece_id,
            name: piece_name,
            letter: piece_letter,
            origin_x,
            origin_y,
            group_index: this_g_idx,
        });
        polygons.push(polygon);
    } // for child in doc.root.children

    (pieces, polygons)
} // fn extract_piece_rects_and_polygons

// ---------------------------------------------------------------------------
// Piece discovery — the Seamly2D handoff contract
// ---------------------------------------------------------------------------

// @brief Re-parent every nested `data-type="piece"` element up to the SVG root.
//
// Seamly2D's Layout Mode wraps the whole pattern in one group:
//
// ```xml
// <svg>
//   <g id="pattern-1" data-type="pattern">
//     <g id="piece-1" data-type="piece">…</g>   <!-- ×12 -->
//   </g>
// </svg>
// ```
//
// Every stage of this app's layout pipeline — `svg_dom::verticalize_dom`,
// `svg_dom::translate_dom`, `extract_piece_rects*`, `layout_assembler`,
// `oversized`, `remaining`, `sheets` — treats a **direct** `<g>` child of the
// root as one piece.  Left alone, the wrapper is that one piece: the packer is
// handed a single sheet-sized object and places nothing (Task 59).
//
// Rather than teach eight call sites a new tree shape, the document is
// normalised once, here: each tagged piece is lifted out of its wrapper and
// appended to the root, and any wrapper left with no element children is
// dropped.  Wrappers that still hold content (a legend, a title group) are
// kept — in tagged mode `extract_piece_rects` ignores them anyway because they
// carry no `data-type="piece"`.
//
// **Transforms are composed, not discarded.** Each hoisted piece inherits the
// concatenation of its former ancestors' `transform` attributes, prepended to
// its own so the ancestor transform still applies first.  Seamly2D's exporter
// puts no transform on the pattern group today, so this is normally a no-op —
// but a piece that silently moved would be a very expensive bug to find later.
//
// Untagged SVGs and already-flat tagged SVGs are left byte-for-byte alone.
//
// @param doc SVG document to normalise in place.
// @return Number of pieces re-parented; 0 when the document needed no change.
pub fn hoist_tagged_pieces(doc: &mut svg_dom::Document) -> usize {
    // Cheap guard: only tagged documents whose pieces are actually nested need
    // rewriting.  Keeps the untagged fallback path completely untouched.
    if !has_nested_tagged_piece(&doc.root) {
        return 0; // nothing nested — leave the document as it is
    } // if not nested

    // Pieces lifted out of wrappers, in document order.  Appended to the root
    // after the surviving children so their relative order is preserved.
    let mut hoisted: Vec<Element> = Vec::new();

    // Rebuild the root's child list: take ownership so each child can be
    // mutated (pieces removed from it) before deciding whether to keep it.
    let children = std::mem::take(&mut doc.root.children);
    let mut kept: Vec<XMLNode> = Vec::new();

    for node in children {
        let XMLNode::Element(mut elem) = node else {
            kept.push(node); // text, comment, CDATA — nothing to hoist
            continue;
        }; // XMLNode::Element

        if is_tagged_piece(&elem) {
            kept.push(XMLNode::Element(elem)); // already at the root — leave in place
            continue;
        } // if already a top-level piece

        // A wrapper's own transform is the first link of the inherited chain for
        // every piece beneath it.  The `<svg>` root itself cannot carry one.
        let wrapper_transform = elem.attributes.get("transform").cloned().unwrap_or_default();

        let before = hoisted.len();
        take_tagged_pieces(&mut elem, &wrapper_transform, &mut hoisted);
        let took_pieces = hoisted.len() > before;

        // A wrapper that existed only to hold pieces is now empty — drop it so
        // it cannot be mistaken for a piece or emit a stray empty group.
        if took_pieces && !has_element_child(&elem) {
            continue; // wrapper consumed
        } // if emptied wrapper

        kept.push(XMLNode::Element(elem));
    } // for node in children

    doc.root.children = kept;

    let count = hoisted.len();
    for piece in hoisted {
        doc.root.children.push(XMLNode::Element(piece));
    } // for piece

    count
} // fn hoist_tagged_pieces

// @brief Recursive worker for `hoist_tagged_pieces`.
//
// Removes every `data-type="piece"` descendant of `parent` from the tree,
// prepending `inherited` to each one's own transform, and appends them to `out`
// in document order.  Intermediate groups that end up with no element children
// are dropped; ones that still hold content are kept.
//
// @param parent    Subtree root to strip pieces out of (mutated in place).
// @param inherited Concatenated `transform` of every ancestor between the SVG
//                  root and `parent`, inclusive; empty when there is none.
// @param out       Accumulator receiving the removed piece elements.
fn take_tagged_pieces(parent: &mut Element, inherited: &str, out: &mut Vec<Element>) {
    let children = std::mem::take(&mut parent.children);
    let mut kept: Vec<XMLNode> = Vec::new();

    for node in children {
        let XMLNode::Element(mut elem) = node else {
            kept.push(node); // non-element node — keep it where it is
            continue;
        }; // XMLNode::Element

        if is_tagged_piece(&elem) {
            // Bake the ancestor chain into the piece so it renders unchanged
            // once it hangs directly off the root.
            let own = elem.attributes.get("transform").cloned().unwrap_or_default();
            let composed = join_transforms(inherited, &own);
            if composed.is_empty() {
                elem.attributes.remove("transform"); // no transform at all — do not emit an empty one
            } else {
                elem.attributes.insert("transform".to_string(), composed);
            } // if composed.is_empty
            out.push(elem);
            continue;
        } // if tagged piece

        // Not a piece: descend, carrying this element's transform along.
        let own = elem.attributes.get("transform").cloned().unwrap_or_default();
        let chained = join_transforms(inherited, &own);

        let before = out.len();
        take_tagged_pieces(&mut elem, &chained, out);
        let took_pieces = out.len() > before;

        if took_pieces && !has_element_child(&elem) {
            continue; // intermediate group emptied by the hoist — drop it
        } // if emptied

        kept.push(XMLNode::Element(elem));
    } // for node in children

    parent.children = kept;
} // fn take_tagged_pieces

// @brief Concatenate two SVG transform lists, outer first.
//
// SVG applies a transform list left-to-right as nested coordinate systems, so
// `"<ancestor> <own>"` reproduces exactly what the nesting did.  Either side may
// be empty.
//
// @param outer Ancestor transform (applied first); may be empty.
// @param inner Element's own transform (applied second); may be empty.
// @return Combined transform string; empty when both inputs are empty.
fn join_transforms(outer: &str, inner: &str) -> String {
    match (outer.trim(), inner.trim()) {
        ("", "")         => String::new(),
        ("", i)          => i.to_string(),
        (o, "")          => o.to_string(),
        (o, i)           => format!("{o} {i}"),
    } // match
} // fn join_transforms

// @brief True when the element carries the Seamly2D piece tag.
// @param elem Element to test.
// @return `true` for `data-type="piece"`, `false` otherwise (exact match — the
//         `piecework` case in the tests must not be treated as a prefix).
fn is_tagged_piece(elem: &Element) -> bool {
    matches!(elem.attributes.get("data-type"), Some(value) if value == "piece")
} // fn is_tagged_piece

// @brief True when the document contains a tagged piece **anywhere**.
//
// This is the switch between tagged and untagged discovery.  It deliberately
// searches the whole tree rather than just the root's children: extraction only
// ever *collects* top-level groups, so a handoff that reached it without being
// hoisted would otherwise fall back to the untagged rule and pack the pattern
// wrapper as one sheet-sized piece — the exact Task 59 failure.  Searching the
// whole tree makes that case yield zero pieces instead, which surfaces as the
// loud "No pattern pieces found" error rather than a silently wrong layout.
//
// @param root The `<svg>` root element.
// @return `true` for a Seamly2D handoff, `false` for an ordinary drawing.
fn document_has_tagged_pieces(root: &Element) -> bool {
    is_tagged_piece(root) || subtree_has_tagged_piece(root)
} // fn document_has_tagged_pieces

// @brief True when a tagged piece sits below a direct child of the SVG root.
//
// Distinguishes "needs hoisting" (the handoff's `<g data-type="pattern">` shape)
// from "already flat" and from "untagged", so `hoist_tagged_pieces` can leave the
// latter two documents untouched.
//
// @param root The `<svg>` root element.
// @return `true` when at least one piece is nested two or more levels deep.
fn has_nested_tagged_piece(root: &Element) -> bool {
    root.children.iter().any(|node| match node {
        // A root child that IS a piece is already flat — look inside the others.
        XMLNode::Element(e) if !is_tagged_piece(e) => subtree_has_tagged_piece(e),
        _ => false,
    })
} // fn has_nested_tagged_piece

// @brief Recursive worker for `has_nested_tagged_piece`.
// @param elem Subtree root to search below (not counting `elem` itself).
// @return `true` when any descendant carries `data-type="piece"`.
fn subtree_has_tagged_piece(elem: &Element) -> bool {
    elem.children.iter().any(|node| match node {
        XMLNode::Element(child) => is_tagged_piece(child) || subtree_has_tagged_piece(child),
        _ => false,
    })
} // fn subtree_has_tagged_piece

// @brief True when the element has at least one child element node.
// @param elem Element to test.
// @return `false` for an element holding only text, comments, or nothing.
fn has_element_child(elem: &Element) -> bool {
    elem.children.iter().any(|node| matches!(node, XMLNode::Element(_)))
} // fn has_element_child

// @brief Read a piece group's identity attributes.
//
// `id` is the machine identity used for element lookup; `data-name` and
// `data-letter` are what a user should see (see `PieceRect::label`).  Untagged
// SVGs have neither `data-*` attribute, so both come back empty and `label()`
// falls through to the id — the historical behaviour.
//
// @param elem Piece `<g>` element.
// @return `(id, data-name, data-letter)`, each empty when the attribute is absent.
fn piece_identity(elem: &Element) -> (String, String, String) {
    let id     = elem.attributes.get("id").cloned().unwrap_or_default();
    let name   = elem.attributes.get("data-name").cloned().unwrap_or_default();
    let letter = elem.attributes.get("data-letter").cloned().unwrap_or_default();
    (id, name, letter)
} // fn piece_identity

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// @brief Returns true when the element is a `<g>` whose id marks it as a
// non-outline decoration group that must not contribute points to the piece
// bounding box.
//
// Recognised id prefixes (compared lower-case):
//   • notch      — V-notch / tick-mark registration points
//   • tuck       — dart / tuck construction lines (e.g. `tuck_1_a_Back`)
//   • grainline / grain_ — grain direction arrow
//   • ip_        — internal path (pocket placement lines, etc.)
//   • drill / hole — drill-hole markers
//
// Matches the same prefix set as `polygon_pack::svg_extract::is_non_outline_group`
// so the bounding-box calculation and the cutline resolver agree on which groups
// to skip.
fn is_non_outline_group(e: &xmltree::Element) -> bool {
    if e.name != "g" { return false; }
    let Some(id) = e.attributes.get("id") else { return false; };
    let id_lower = id.to_lowercase();
    id_lower.starts_with("notch")
        || id_lower.starts_with("tuck")
        || id_lower.starts_with("grainline")
        || id_lower.starts_with("grain_")
        || id_lower.starts_with("ip_")
        || id_lower.starts_with("drill")
        || id_lower.starts_with("hole")
} // fn is_non_outline_group

// @brief Collect all coordinates from every `<path d="...">` descendant,
// skipping any child `<g>` whose id identifies it as a non-outline decoration
// (tuck, notch, grainline, ip, drill, hole).
//
// Without this filter, a piece with `tuck_1_a_Back` or `notch_1_Back`
// siblings would have its bounding box inflated by the construction-line
// geometry, wasting layout space when the piece is packed.
//
// @param element Root element to search (typically a piece `<g>`).
// @param points  Output buffer; each segment's endpoints are appended.
fn collect_all_path_points(element: &xmltree::Element, points: &mut Vec<Point>) {
    // If this element is a <path>, parse its d attribute.
    if element.name == "path" {
        if let Some(d) = element.attributes.get("d") {
            if let Ok(path) = Path::parse_path_attribute(d) {
                // Extract the endpoint of every segment into the point list.
                for seg in &path.segments {
                    match seg {
                        PathSegment::MoveTo(p)                     => points.push(*p),
                        PathSegment::LineTo(p)                     => points.push(*p),
                        PathSegment::QuadTo { ctrl, to }           => { points.push(*ctrl); points.push(*to); }
                        PathSegment::CubicTo { ctrl1, ctrl2, to }  => { points.push(*ctrl1); points.push(*ctrl2); points.push(*to); }
                        PathSegment::ArcTo { to, .. }              => points.push(*to),
                        PathSegment::Close                         => {} // no new point
                    } // match seg
                } // for seg in path.segments
            } // if let Ok(path)
        } // if let Some(d)
    } // if element.name == "path"

    // Recurse into children, skipping non-outline decoration groups.
    for child in &element.children {
        if let XMLNode::Element(child_elem) = child {
            // Tuck, notch, grainline, ip, drill, and hole groups are decorations;
            // their coordinates must not expand the piece's bounding box.
            if is_non_outline_group(child_elem) { continue; }
            collect_all_path_points(child_elem, points);
        } // if XMLNode::Element
    } // for child
} // fn collect_all_path_points

// @brief Count the elements tagged as pattern pieces by Seamly2D.
//
// Seamly2D's Layout Mode writes one `<g data-type="piece" …>` per pattern
// piece (see `project-docs/NEW-ATTRIBUTES.csv` and
// `src/libs/vformat/svg_generator.cpp`).  SeamlyLayout does not *require* the
// tagging — `extract_piece_rects` treats every top-level `<g>` with geometry as
// a piece, so a hand-drawn SVG still lays out — but its absence means the file
// did not come from the Layout Mode handoff, which is worth telling the user
// about before they wonder why the result looks nothing like their pattern.
//
// The whole tree is walked, not just the root's children, so the count is
// unaffected by any wrapper group a future exporter might introduce.
//
// @param doc SVG document previously loaded by `app_core::load_svg`.
// @return Number of elements carrying `data-type="piece"`; 0 for an untagged SVG.
pub fn count_tagged_pieces(doc: &svg_dom::Document) -> usize {
    count_tagged_pieces_in(&doc.root)
} // fn count_tagged_pieces

// @brief Recursive worker for `count_tagged_pieces`.
// @param element Subtree root to count within (counted itself as well).
// @return Number of `data-type="piece"` elements in this subtree.
fn count_tagged_pieces_in(element: &xmltree::Element) -> usize {
    // Count this element when it carries the piece tag.
    let mut count = match element.attributes.get("data-type") {
        Some(value) if value == "piece" => 1,
        _ => 0, // untagged, or tagged as pattern/seamline/cutline/…
    }; // match data-type

    // Recurse into every child element; text nodes and comments cannot be tagged.
    for child in &element.children {
        if let XMLNode::Element(child_elem) = child {
            count += count_tagged_pieces_in(child_elem);
        } // if XMLNode::Element
    } // for child

    count
} // fn count_tagged_pieces_in

// @brief Determine SVG user-units-per-CSS-pixel from the root `<svg>` element.
//
// SVG files exported by Seamly2D typically set `width`/`height` in millimetres
// or inches with a `viewBox` attribute.  The ratio viewBox-width / width gives
// the number of user units per CSS pixel (96 dpi assumed as the CSS baseline).
//
// Fallback: if width/height are dimensionless numbers (already in px), returns
// 1.0 so that `LAYOUT_PPI / (uu_per_px * 96.0) == 1.0` when LAYOUT_PPI == 96.
//
// Also used by `layout_assembler` to convert pixel placements back to user units.
//
// @param root The `<svg>` root element.
// @return User-units per CSS pixel (≥ 1e-9 to prevent divide-by-zero).
pub fn svg_uu_per_px(root: &xmltree::Element) -> f64 {

    // get root viewBox width and document width in pixels
    let viewbox_w_px = parse_viewbox_width_px(root); // viewBox is "(0 0 w_uu h_uu)" in user units, many steps so use helper function
    let doc_width_str: Option<&String> = root.attributes.get("width"); // width example: "36.0mm" in user units or "100" in pixels
    let doc_w_px: Option<u32> = doc_width_str.map(|s| measurement_to_px(s)); // if needed, strip user units and convert to pixels

    // validate; don't divide by zero or return a crazy scale if doc width is missing/invalid
    if let (Some(vb_wpx), Some(d_wpx)) = (viewbox_w_px, doc_w_px) {
        // uu_per_px = viewBox-width / doc-width
        let uu_per_px: f64 = vb_wpx as f64 / d_wpx as f64;
        return uu_per_px.max(1e-9); // clamp to avoid divide-by-zero
    } // if viewbox

    // No viewBox or no explicit units — return 1 user unit = 1 px but its f64 for consistent scaling in layout_assembler.
    1.0
} // fn svg_user_units_per_px

// @brief Parse the first two values of the `viewBox` attribute ("min-x min-y w h").
// @return The width field (third token), or None.
fn parse_viewbox_width_px(root: &xmltree::Element) -> Option<u32> {
    // viewbox in pixels: "0 0 100 100" → width is the third token (index 2)
    let vb = root.attributes.get("viewBox").or_else(|| root.attributes.get("viewbox"))?;
    let parts: Vec<u32> = vb
        // Split on spaces or commas, per SVG spec.
        .split(|c: char| c == ' ' || c == ',')
        // Filter out empty tokens (e.g., from multiple spaces).
        .filter(|s| !s.is_empty())
        // Parse each token as a measurement, converting to pixels.
        .filter_map(|s| Some(measurement_to_px(s)))
        // Collect into a Vec for indexing.
        .collect();
    // viewBox = "min-x min-y width height"

    // return width in pixels, or None if viewBox is missing/invalid
    parts.get(2).copied() // width is the third token (index 2)
} // fn parse_viewbox_width

// @brief Split "36.0mm" → ("36.0", "mm"), "100" → ("100", "px").
// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Two <g> pieces with path data are extracted with correct dimensions.
    #[test]
    fn extracts_two_pieces() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="piece-1">
    <path d="M 0 0 L 96 0 L 96 96 L 0 96 Z"/>
  </g>
  <g id="piece-2">
    <path d="M 0 0 L 48 0 L 48 48 L 0 48 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 2);
        // 96 user-units at 96 dpi with no viewBox → 96 px square
        assert_eq!(pieces[0].rect.w, 96);
        assert_eq!(pieces[0].rect.h, 96);
        // 48 px square
        assert_eq!(pieces[1].rect.w, 48);
        assert_eq!(pieces[1].rect.h, 48);
        assert_eq!(pieces[0].id, "piece-1");
        assert_eq!(pieces[1].id, "piece-2");
    } // extracts_two_pieces

    // @brief A <g> with no <path> children is skipped.
    #[test]
    fn skips_empty_group() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
  <g id="empty"/>
  <g id="real">
    <path d="M 0 0 L 50 0 L 50 50 L 0 50 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "real");
    } // skips_empty_group

    // @brief <g> missing an id attribute gets an empty string id.
    #[test]
    fn handles_missing_id() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
  <g>
    <path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "");
    } // handles_missing_id

    // @brief A Seamly2D handoff SVG reports one tagged piece per data-type="piece" group.
    #[test]
    fn counts_tagged_pieces() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="pattern-1" data-type="pattern" data-name="Richmond Shirt">
    <g id="piece-1" data-type="piece" data-type-number="1" data-parent="pattern-1">
      <g id="seamline-1" data-type="seamline" data-parent="piece-1">
        <path d="M 0 0 L 96 0 L 96 96 L 0 96 Z"/>
      </g>
    </g>
    <g id="piece-2" data-type="piece" data-type-number="2" data-parent="pattern-1">
      <path d="M 0 0 L 48 0 L 48 48 L 0 48 Z"/>
    </g>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        // Nested two levels below the root — the walk is recursive, not top-level only.
        assert_eq!(count_tagged_pieces(&doc), 2);
    } // counts_tagged_pieces

    // @brief An SVG with no data-* tagging reports zero pieces.
    // This is what triggers the import warning: the file did not come from
    // Seamly2D's Layout Mode, even though it may still lay out fine.
    #[test]
    fn counts_zero_for_untagged_svg() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="piece-1">
    <path d="M 0 0 L 96 0 L 96 96 L 0 96 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        assert_eq!(count_tagged_pieces(&doc), 0);
    } // counts_zero_for_untagged_svg

    // @brief Other data-type values (pattern, seamline, grainline, …) are not counted.
    #[test]
    fn counts_only_the_piece_data_type() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="pattern-1" data-type="pattern">
    <g id="grainline-1" data-type="grainline"/>
    <g id="notch-1" data-type="notch"/>
    <g id="pieces" data-type="piecework"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        // "piecework" must not match: the comparison is exact, not a prefix.
        assert_eq!(count_tagged_pieces(&doc), 0);
    } // counts_only_the_piece_data_type

    // @brief Non-<g> top-level elements (rect, defs, title) are skipped.
    #[test]
    fn skips_non_group_elements() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <rect x="0" y="0" width="200" height="200" fill="white"/>
  <defs/>
  <title>Test</title>
  <g id="p1">
    <path d="M 0 0 L 20 0 L 20 30 L 0 30 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "p1");
    } // skips_non_group_elements

    // @brief viewBox + mm width produces correct pixel scaling.
    #[test]
    fn viewbox_mm_scaling() {
        // viewBox="0 0 100 100", width="25.4mm" → 25.4mm = 96px → uu_per_px = 100/96
        // A piece of 100×100 user units → 100/(100/96) * 96 = 96×96 px... wait:
        // scale = dpi / uu_per_px = 96 / (100/96) = 96 * 96/100 = 92.16 px
        // path goes 0..50 uu → w_uu=50, h_uu=50 → 50 * 92.16/96 ≈ 48 px
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="25.4mm" height="25.4mm" viewBox="0 0 100 100">
  <g id="p1">
    <path d="M 0 0 L 50 0 L 50 50 L 0 50 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        // 50 user units in a 100uu-wide box that maps to 96px → 48 px
        assert_eq!(pieces[0].rect.w, 48);
        assert_eq!(pieces[0].rect.h, 48);
    } // viewbox_mm_scaling

    // @brief group_index reflects position within top-level <g> list even when some are skipped.
    #[test]
    fn group_index_skips_empty() {
        // g_idx=0 (empty, skipped), g_idx=1 (has paths) → pieces[0].group_index == 1
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100">
  <g id="empty"/>
  <g id="real">
    <path d="M 0 0 L 30 0 L 30 30 L 0 30 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].group_index, 1); // skipped g_idx=0, so real piece is at g_idx=1
    } // group_index_skips_empty

    // @brief Two pieces get consecutive group_index values when no groups are skipped.
    #[test]
    fn group_index_consecutive() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="a"><path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/></g>
  <g id="b"><path d="M 0 0 L 20 0 L 20 20 L 0 20 Z"/></g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces[0].group_index, 0);
        assert_eq!(pieces[1].group_index, 1);
    } // group_index_consecutive

    // @brief Paired extractor returns equal-length, index-aligned rects and
    // polygons.  Piece A has a cutline group → real polygon (≥3 vertices,
    // not the AABB rectangle).  Piece B has only a grainline → AABB fallback
    // (exactly 4 vertices matching the rect's (0,0)–to–(w,h) frame).
    #[test]
    fn paired_extractor_aligns_rects_and_polygons() {
        // Both pieces use width="100" (no viewBox) → 1 user-unit = 1 px at
        // LAYOUT_PPI=96 baseline so the assertions can use exact pixel math.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="WithCutline">
    <g id="cutline_WithCutline">
      <path d="M 0 0 L 50 0 L 50 30 L 0 30 L 0 0"/>
    </g>
  </g>
  <g id="GrainlineOnly">
    <g id="grainline_GrainlineOnly">
      <path d="M 5 0 L 5 25"/>
    </g>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let (pieces, polygons) = extract_piece_rects_and_polygons(&doc);

        // Index alignment is the load-bearing invariant.
        assert_eq!(pieces.len(), polygons.len(), "rects and polygons must align");

        // GrainlineOnly's grainline path is 0..25 vertical so its AABB has
        // width 0 → would be skipped by the zero-dim filter.  Rebuild the
        // expectation: only WithCutline survives.
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "WithCutline");
        // Cutline polygon: closing duplicate trimmed → 4 vertices, but the
        // shape is a real rectangle outline (not the fallback synthesised one
        // — confirmed by checking the second vertex sits at (50, 0) just like
        // the path).
        assert_eq!(polygons[0].vertices.len(), 4);
        assert!((polygons[0].vertices[1].0 - 50.0).abs() < 1e-6);
        assert!((polygons[0].vertices[1].1 -  0.0).abs() < 1e-6);
    } // paired_extractor_aligns_rects_and_polygons

    // @brief When a piece has path geometry but no cutline / seamline group,
    // the polygon falls back to the rect AABB (4 vertices CCW from (0,0)).
    // This preserves "every kept rect is also packable as a polygon" so
    // non-orthogonal trial sets still cover every piece.
    #[test]
    fn paired_extractor_falls_back_to_aabb_polygon() {
        // No <g id="cutline_*"> child — only a bare <path> that contributes
        // points to the AABB.  find_outline_group returns None → fallback.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="P">
    <path d="M 0 0 L 40 0 L 40 20 L 0 20 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let (pieces, polygons) = extract_piece_rects_and_polygons(&doc);

        assert_eq!(pieces.len(), 1);
        assert_eq!(polygons.len(), 1);

        // Fallback is exactly 4 vertices matching the rect (CCW from origin).
        let v = &polygons[0].vertices;
        assert_eq!(v.len(), 4);
        assert_eq!(v[0], (0.0,  0.0));
        assert_eq!(v[1], (40.0, 0.0));
        assert_eq!(v[2], (40.0, 20.0));
        assert_eq!(v[3], (0.0,  20.0));
        // And the polygon's AABB matches pieces[0].rect.
        assert_eq!(pieces[0].rect.w, 40);
        assert_eq!(pieces[0].rect.h, 20);
    } // paired_extractor_falls_back_to_aabb_polygon

    // @brief origin_x and origin_y reflect the bounding-box minimum corner.
    #[test]
    fn origin_reflects_bbox_min() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="p1">
    <path d="M 10 20 L 60 20 L 60 80 L 10 80 Z"/>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert!((pieces[0].origin_x - 10.0).abs() < 0.01, "origin_x={}", pieces[0].origin_x);
        assert!((pieces[0].origin_y - 20.0).abs() < 0.01, "origin_y={}", pieces[0].origin_y);
    } // origin_reflects_bbox_min

    // @brief Tuck sibling groups must not inflate the piece bounding box.
    //
    // A piece with `tuck_1_a_Back` and `tuck_1_b_Back` children extending
    // outside the cutline boundary should produce a rect whose dimensions
    // match the cutline only, not the union of all paths.
    #[test]
    fn tuck_sibling_bbox_not_inflated() {
        // cutline_Back is a 50×30 rectangle.
        // tuck_1_a_Back reaches x=70 (20 units beyond the cutline right edge).
        // Without filtering, w_px would be > 50; with filtering it must be 50.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="Back">
    <g id="cutline_Back">
      <path d="M 0 0 L 50 0 L 50 30 L 0 30 L 0 0"/>
    </g>
    <g id="tuck_1_a_Back">
      <path d="M 20 5 L 70 5 L 45 25"/>
    </g>
    <g id="tuck_1_b_Back">
      <path d="M 25 5 L 65 5 L 45 20"/>
    </g>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1, "expected 1 piece");
        // The cutline is 50 wide and 30 tall; tuck groups must not push the bbox wider.
        assert_eq!(pieces[0].rect.w, 50,
            "width inflated by tuck groups: got {} expected 50", pieces[0].rect.w);
        assert_eq!(pieces[0].rect.h, 30,
            "height inflated by tuck groups: got {} expected 30", pieces[0].rect.h);
    } // tuck_sibling_bbox_not_inflated

    // @brief Notch sibling groups must not inflate the piece bounding box.
    //
    // A piece with `notch_1_Piece` tick marks protruding beyond the cutline
    // should pack at the cutline dimensions, not the notch-inflated AABB.
    #[test]
    fn notch_sibling_bbox_not_inflated() {
        // cutline_Piece is a 40×20 rectangle.
        // notch_1_Piece has a point at y=-5 (5 units above the top edge).
        // Without filtering, h_px would be > 20; with filtering it must be 20.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="Piece">
    <g id="cutline_Piece">
      <path d="M 0 0 L 40 0 L 40 20 L 0 20 L 0 0"/>
    </g>
    <g id="notch_1_Piece">
      <path d="M 20 0 L 20 -5 L 22 0"/>
    </g>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1, "expected 1 piece");
        assert_eq!(pieces[0].rect.w, 40,
            "width inflated by notch: got {} expected 40", pieces[0].rect.w);
        assert_eq!(pieces[0].rect.h, 20,
            "height inflated by notch: got {} expected 20", pieces[0].rect.h);
    } // notch_sibling_bbox_not_inflated

    // -----------------------------------------------------------------------
    // Task 59 — the Seamly2D handoff shape: pieces nested in a pattern wrapper
    // -----------------------------------------------------------------------

    // @brief The real handoff shape, as `SvgGenerator::mergeSvgDoms` writes it:
    // one `<g data-type="pattern">` wrapping every piece.  Two levels deep, and
    // the seamline group inside each piece adds a third.
    fn nested_handoff_svg() -> &'static str {
        r#"<svg xmlns="http://www.w3.org/2000/svg" width="400" height="400">
  <g id="pattern-1" data-type="pattern" data-type-number="1" data-name="The Richmond Shirt">
    <g id="piece-1" data-type="piece" data-type-number="1" data-parent="pattern-1"
       data-name="Front Bodice" data-letter="A">
      <g id="seamline-1" data-type="seamline" data-parent="piece-1">
        <path d="M 0 0 L 96 0 L 96 96 L 0 96 Z"/>
      </g>
    </g>
    <g id="piece-2" data-type="piece" data-type-number="2" data-parent="pattern-1"
       data-name="Back Bodice">
      <path d="M 0 0 L 48 0 L 48 48 L 0 48 Z"/>
    </g>
  </g>
</svg>"#
    } // fn nested_handoff_svg

    // @brief The pattern wrapper must never pack as a piece.
    //
    // This is the Task 59 failure pinned from the other side: before the fix the
    // untagged rule packed `<g id="pattern-1">` as one sheet-sized object and the
    // packer reported `0 placements, 1 unplaced: ["pattern-1"]`.  Because the
    // tagged/untagged decision searches the whole tree, a handoff that somehow
    // reaches extraction un-hoisted now yields *nothing* — which `do_process_layout`
    // turns into a visible "No pattern pieces found" error instead of a silently
    // wrong layout.
    #[test]
    fn pattern_wrapper_never_packs_as_a_piece() {
        let doc = svg_dom::Document::parse(nested_handoff_svg()).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 0, "the pattern wrapper must never pack as a piece");
    } // pattern_wrapper_never_packs_as_a_piece

    // @brief After hoisting, each tagged piece is a direct child of the root and
    // extraction finds all of them at their own dimensions.
    #[test]
    fn hoist_flattens_the_pattern_wrapper() {
        let mut doc = svg_dom::Document::parse(nested_handoff_svg()).expect("parse ok");
        assert_eq!(hoist_tagged_pieces(&mut doc), 2, "both pieces should be hoisted");

        // The emptied wrapper is gone; only the two pieces remain at the root.
        let root_groups: Vec<&str> = doc.root.children.iter()
            .filter_map(|n| n.as_element())
            .filter(|e| e.name == "g")
            .map(|e| e.attributes.get("id").map(String::as_str).unwrap_or(""))
            .collect();
        assert_eq!(root_groups, vec!["piece-1", "piece-2"]);

        // And they pack as two pieces of their own sizes, not one sheet-sized blob.
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 2);
        assert_eq!((pieces[0].rect.w, pieces[0].rect.h), (96, 96));
        assert_eq!((pieces[1].rect.w, pieces[1].rect.h), (48, 48));
    } // hoist_flattens_the_pattern_wrapper

    // @brief Piece identity survives the hoist: id, data-name and data-letter all
    // reach `PieceRect`, and `label()` prefers the name a user recognises.
    #[test]
    fn hoist_preserves_piece_identity() {
        let mut doc = svg_dom::Document::parse(nested_handoff_svg()).expect("parse ok");
        hoist_tagged_pieces(&mut doc);
        let pieces = extract_piece_rects(&doc);

        assert_eq!(pieces[0].id,     "piece-1");
        assert_eq!(pieces[0].name,   "Front Bodice");
        assert_eq!(pieces[0].letter, "A");
        assert_eq!(pieces[0].label(), "Front Bodice"); // name wins over letter and id

        assert_eq!(pieces[1].name,   "Back Bodice");
        assert_eq!(pieces[1].letter, "");              // this piece has no letter
        assert_eq!(pieces[1].label(), "Back Bodice");
    } // hoist_preserves_piece_identity

    // @brief `label()` falls back letter → id when data-name is absent, so an
    // untagged drawing still labels its pieces exactly as it always did.
    #[test]
    fn label_falls_back_to_letter_then_id() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="pattern-1" data-type="pattern">
    <g id="piece-1" data-type="piece" data-letter="B">
      <path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/>
    </g>
    <g id="piece-2" data-type="piece">
      <path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/>
    </g>
  </g>
</svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("parse ok");
        hoist_tagged_pieces(&mut doc);
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces[0].label(), "B");       // no name → letter
        assert_eq!(pieces[1].label(), "piece-2"); // neither → id
    } // label_falls_back_to_letter_then_id

    // @brief A transform on the pattern wrapper is composed onto each piece as it
    // is re-parented, so a hoisted piece renders exactly where it did before.
    // Seamly2D writes no wrapper transform today; a silently moved piece would be
    // a very expensive bug, so the composition is pinned.
    #[test]
    fn hoist_composes_wrapper_transform_onto_pieces() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="400" height="400">
  <g id="pattern-1" data-type="pattern" transform="translate(10,20)">
    <g id="piece-1" data-type="piece" transform="rotate(90)">
      <path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/>
    </g>
    <g id="piece-2" data-type="piece">
      <path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/>
    </g>
  </g>
</svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("parse ok");
        assert_eq!(hoist_tagged_pieces(&mut doc), 2);

        let transform_of = |id: &str| -> String {
            doc.root.children.iter()
                .filter_map(|n| n.as_element())
                .find(|e| e.attributes.get("id").map(String::as_str) == Some(id))
                .and_then(|e| e.attributes.get("transform").cloned())
                .unwrap_or_default()
        };

        // Wrapper transform first (outermost), then the piece's own — the same
        // order SVG applied them when the piece was still nested.
        assert_eq!(transform_of("piece-1"), "translate(10,20) rotate(90)");
        // A piece with no transform of its own simply inherits the wrapper's.
        assert_eq!(transform_of("piece-2"), "translate(10,20)");
    } // hoist_composes_wrapper_transform_onto_pieces

    // @brief An untagged SVG is left completely alone — the hoist reports 0 and
    // the historical "every top-level <g> is a piece" rule still applies.
    #[test]
    fn hoist_leaves_untagged_svg_untouched() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="drawing">
    <g id="inner"><path d="M 0 0 L 40 0 L 40 20 L 0 20 Z"/></g>
  </g>
</svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("parse ok");
        assert_eq!(hoist_tagged_pieces(&mut doc), 0, "no tagging — nothing to hoist");

        // Still one top-level group holding its nested child, and it packs as the
        // single piece the untagged rule says it is.
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "drawing");
        assert_eq!(pieces[0].label(), "drawing"); // no data-* → id is the label
    } // hoist_leaves_untagged_svg_untouched

    // @brief A tagged SVG whose pieces already sit at the root needs no rewrite.
    #[test]
    fn hoist_is_a_no_op_when_pieces_are_already_top_level() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="piece-1" data-type="piece"><path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/></g>
  <g id="piece-2" data-type="piece"><path d="M 0 0 L 20 0 L 20 20 L 0 20 Z"/></g>
</svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("parse ok");
        assert_eq!(hoist_tagged_pieces(&mut doc), 0);
        assert_eq!(extract_piece_rects(&doc).len(), 2);
    } // hoist_is_a_no_op_when_pieces_are_already_top_level

    // @brief A wrapper that still holds non-piece content survives the hoist, and
    // tagged mode refuses to pack it even though it carries path geometry.
    #[test]
    fn hoist_keeps_a_wrapper_that_still_has_content() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="400" height="400">
  <g id="pattern-1" data-type="pattern">
    <g id="legend"><path d="M 0 0 L 300 0 L 300 300 L 0 300 Z"/></g>
    <g id="piece-1" data-type="piece"><path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/></g>
  </g>
</svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("parse ok");
        assert_eq!(hoist_tagged_pieces(&mut doc), 1);

        // The wrapper is kept because <g id="legend"> is still inside it.
        let root_groups: Vec<&str> = doc.root.children.iter()
            .filter_map(|n| n.as_element())
            .filter(|e| e.name == "g")
            .map(|e| e.attributes.get("id").map(String::as_str).unwrap_or(""))
            .collect();
        assert_eq!(root_groups, vec!["pattern-1", "piece-1"]);

        // ...but only the tagged piece packs.  Were the untagged rule still in
        // force the 300×300 legend wrapper would swamp the 10×10 piece.
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "piece-1");
    } // hoist_keeps_a_wrapper_that_still_has_content

    // @brief `group_index` must stay a valid index into the root's `<g>` children
    // after hoisting — `layout_assembler`, `oversized` and `remaining` all look
    // the original element up that way, and an off-by-one there silently places
    // the wrong geometry.
    #[test]
    fn group_index_indexes_root_groups_after_hoist() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="400" height="400">
  <g id="legend"><path d="M 0 0 L 300 0 L 300 300 L 0 300 Z"/></g>
  <g id="pattern-1" data-type="pattern">
    <g id="piece-1" data-type="piece"><path d="M 0 0 L 10 0 L 10 10 L 0 10 Z"/></g>
    <g id="piece-2" data-type="piece"><path d="M 0 0 L 20 0 L 20 20 L 0 20 Z"/></g>
  </g>
</svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("parse ok");
        hoist_tagged_pieces(&mut doc);

        // Root order after the hoist: legend (kept), then the two hoisted pieces.
        let root_groups: Vec<&xmltree::Element> = doc.root.children.iter()
            .filter_map(|n| n.as_element())
            .filter(|e| e.name == "g")
            .collect();

        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 2);
        for piece in &pieces {
            let looked_up = root_groups[piece.group_index];
            assert_eq!(
                looked_up.attributes.get("id").map(String::as_str),
                Some(piece.id.as_str()),
                "group_index {} does not resolve back to {}", piece.group_index, piece.id
            );
        }
        assert_eq!(pieces[0].group_index, 1); // index 0 is the legend group
        assert_eq!(pieces[1].group_index, 2);
    } // group_index_indexes_root_groups_after_hoist

    // @brief The paired extractor makes the same discovery decision as
    // `extract_piece_rects`; if the two disagreed, `group_index` would diverge
    // between the packing inputs and the assembler's element lookup.
    #[test]
    fn paired_extractor_agrees_with_rect_extractor_on_tagged_pieces() {
        let mut doc = svg_dom::Document::parse(nested_handoff_svg()).expect("parse ok");
        hoist_tagged_pieces(&mut doc);

        let rects_only = extract_piece_rects(&doc);
        let (paired, polygons) = extract_piece_rects_and_polygons(&doc);

        assert_eq!(paired.len(), polygons.len());
        assert_eq!(rects_only.len(), paired.len());
        for (a, b) in rects_only.iter().zip(paired.iter()) {
            assert_eq!(a.id, b.id);
            assert_eq!(a.group_index, b.group_index);
            assert_eq!(a.rect.w, b.rect.w);
            assert_eq!(a.rect.h, b.rect.h);
        }
    } // paired_extractor_agrees_with_rect_extractor_on_tagged_pieces

    // @brief Grainline sibling groups must not inflate the piece bounding box.
    //
    // A grainline arrow extending outside the cutline boundary should be
    // excluded so the packed dimensions reflect the actual piece silhouette.
    #[test]
    fn grainline_sibling_bbox_not_inflated() {
        // cutline_Front is a 60×40 rectangle.
        // grainline_Front is a vertical stroke from y=-10 to y=50 — extends
        // beyond both the top and bottom edges of the cutline.
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
  <g id="Front">
    <g id="cutline_Front">
      <path d="M 0 0 L 60 0 L 60 40 L 0 40 L 0 0"/>
    </g>
    <g id="grainline_Front">
      <path d="M 30 -10 L 30 50"/>
    </g>
  </g>
</svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse ok");
        let pieces = extract_piece_rects(&doc);
        assert_eq!(pieces.len(), 1, "expected 1 piece");
        assert_eq!(pieces[0].rect.w, 60,
            "width inflated by grainline: got {} expected 60", pieces[0].rect.w);
        assert_eq!(pieces[0].rect.h, 40,
            "height inflated by grainline: got {} expected 40", pieces[0].rect.h);
    } // grainline_sibling_bbox_not_inflated

} // mod tests
