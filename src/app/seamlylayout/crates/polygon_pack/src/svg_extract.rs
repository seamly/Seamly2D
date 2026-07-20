// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! Extract piece-cutline polygons from a Seamly2D-style SVG.
//!
//! Each top-level `<g>` child of the SVG root is one pattern piece.  Inside
//! that group, the polygon-tight packer wants the outer cut silhouette only —
//! the cutline polyline, with notch tick-marks and other decoration dropped.
//!
//! ## Cutline resolution (primary: id-based; fallbacks: structural)
//!
//!   1. **Primary — id contains `"cutline"`.**  Any child whose `id`
//!      attribute contains that substring is the cutline.  Matches both
//!      `cutline_<piece>` (Seamly2D's canonical naming) and
//!      `path_cutline_<piece>` variants.  This is the load-bearing signal.
//!   2. **Fallback — structural position 1** (the second path-bearing
//!      `<g>` child).  Used when no id matches; covers older / draft
//!      exports that emit anonymous outline groups distinguished only by
//!      stroke style.  Across observed fixtures, Seamly2D consistently
//!      orders children `[seamline, cutline, …decoration]`.
//!   3. **Fallback — structural position 0** when the second child has
//!      fewer than 3 vertices.  Covers calibration / single-outline
//!      pieces ("Two Inch Gauge" and similar) where the structure is
//!      `[cutline, empty-placeholder, gauge-tick, …]`.
//!
//! Sibling groups always dropped regardless of position:
//!   * `<g id="notch_*">` — notch tick-marks.  The cutline encodes the
//!     silhouette without notch geometry; V-notches are a registration
//!     convention, not part of the cut.
//!   * `grainline_*`, `ip_*`, `<text>`, `<defs>`, … — never selected as
//!     the outline because either the id-based match or the structural
//!     position rule keeps them out of contention.
//!
//! ## Transforms
//!
//! Production input arrives after the bridge's flatten pipeline
//! (`flatten → verticalize → flatten → translate → flatten`), so transforms
//! are baked into vertex coordinates upstream.  This extractor therefore
//! reads the path `d` attribute as-is and does **not** apply any group-level
//! `transform=` matrix.  Test fixtures that haven't been pre-flattened will
//! produce vertices in the path's local coordinate frame, which is fine for
//! shape verification but won't reflect the piece's intended layout origin.
//!
//! ## Sibling identifiers worth knowing (defensive context)
//!
//! Useful when debugging a fixture that doesn't produce the expected
//! polygon shape:
//!   * **Grainline path**: 8 coordinates exactly (an I-beam / double-arrow
//!     glyph stretched along the piece's grain direction).  If a piece's
//!     extracted polygon shows up as exactly 8 vertices, it's overwhelmingly
//!     likely the grainline was mis-selected — check the piece's `<g>`
//!     ordering.
//!   * **Notch path**: 2–8 short collinear/near-collinear segments forming
//!     a tick mark.  Always emitted with `stroke-width ≈ 1.32` (vs. the
//!     cutline's `≈ 3.78`) and frequently with `fill-rule="nonzero"`.
//!   * **Gauge-line path**: 2 collinear vertices (a single straight
//!     segment).  Appears in calibration pieces like "Two Inch Gauge".

use geometry::{Path, PathSegment};
use xmltree::{Element, XMLNode};

use crate::Polygon;

// @brief One extracted piece outline ready for polygon-tight packing.
//
// Pairs the piece's user-visible id (taken from the top-level `<g id="...">`
// attribute) with its closed cutline polygon.  The id is what the placer
// reports in error messages and what the assembler matches on when emitting
// the placed `<g>` back into the layout DOM.
#[derive(Debug, Clone)]
pub struct PiecePolygon {
    /// Value of the `id` attribute on the piece's top-level `<g>`.  Empty
    /// when no id is present (the extractor still emits the polygon so a
    /// caller using positional indexing isn't blocked).
    pub id: String,
    /// Closed-loop outline in user-space units, vertices ordered as parsed
    /// from the path `d`.  Last vertex != first; close is implicit per the
    /// `Polygon` invariant in `lib.rs`.
    pub polygon: Polygon,
} // struct PiecePolygon

// @brief Extract every cutline polygon from a parsed SVG root.
//
// Walks each direct child `<g>` of the SVG root, treating each as one piece,
// and delegates per-piece extraction to `extract_piece_outline`.  Pieces
// whose outline cannot be extracted (no cutline / seamline group, unparseable
// path, or fewer than three distinct vertices) are silently skipped.
//
// @param svg_root  Root `<svg>` element parsed by xmltree.
// @return          One `PiecePolygon` per non-empty piece, in document order.
pub fn extract_cutline_polygons(svg_root: &Element) -> Vec<PiecePolygon> {
    let mut out = Vec::new();

    // Iterate direct `<g>` children of the SVG root — each is one piece.
    for child in &svg_root.children {
        let XMLNode::Element(piece_g) = child else { continue };
        if piece_g.name != "g" { continue; }

        let piece_id = piece_g.attributes.get("id").cloned().unwrap_or_default();

        let Some(polygon) = extract_piece_outline(piece_g) else { continue };

        out.push(PiecePolygon { id: piece_id, polygon });
    } // for child

    out
} // fn extract_cutline_polygons

// @brief Extract one piece's cutline polygon from its top-level `<g>`.
//
// Looks first for a `cutline_*` child group (preferred), falling back to
// `seamline_*` if no cutline is present.  Within that group, the first
// `<path d="...">` descendant is parsed for vertex data; only
// MoveTo / LineTo segments contribute (per the cutline straight-line
// invariant; bezier endpoints tolerated by treating `to` as line-to so
// fixtures that haven't been re-exported through Seamly2D's flatten pass
// still extract).
//
// Returns None when the piece has no usable outline group, the path doesn't
// parse, or the result has fewer than three distinct vertices.
//
// @param piece_g  Top-level `<g>` of one pattern piece.
// @return         Polygon in user-space units (vertices ordered as parsed,
//                 closing duplicate trimmed; `last != first` invariant).
pub fn extract_piece_outline(piece_g: &Element) -> Option<Polygon> {
    // Locate cutline (preferred) or seamline (fallback).  The naming
    // convention is "cutline_<piece>" / "seamline_<piece>", but a few
    // older Seamly2D exports use "path_cutline_<piece>" on either the
    // group or the path — accept either by checking for the substring.
    let outline_g = find_outline_group(piece_g)?;

    // First `<path>` descendant carries the vertex data.  Searching the
    // descendant tree (rather than only direct children) tolerates the
    // common Inkscape-style nesting where a `<g>` wraps a single
    // `<path>` for stroke-style grouping.
    let path_d = first_path_d(outline_g)?;

    // Parse the d attribute into PathSegments (absolute coords).
    let parsed = Path::parse_path_attribute(path_d).ok()?;

    let mut verts: Vec<(f64, f64)> = Vec::with_capacity(parsed.segments.len());
    for seg in &parsed.segments {
        let p = match seg {
            PathSegment::MoveTo(p)             => *p,
            PathSegment::LineTo(p)             => *p,
            PathSegment::QuadTo { to, .. }     => *to,
            PathSegment::CubicTo { to, .. }    => *to,
            PathSegment::ArcTo { to, .. }      => *to,
            PathSegment::Close                 => continue,
        };
        verts.push((p.x as f64, p.y as f64));
    }

    // Trim a duplicate trailing vertex if the path closes with a literal
    // repeat of the first point (common in Seamly2D output: the d ends
    // with `... L start.x,start.y`).  The Polygon invariant is
    // last != first; the closing edge is implicit.
    if let (Some(first), Some(last)) = (verts.first().copied(), verts.last().copied()) {
        if verts.len() > 1 && points_equal(first, last) {
            verts.pop();
        }
    }

    // Reject degenerate pieces (less than a triangle's worth of vertices).
    if verts.len() < 3 { return None; }

    Some(Polygon { vertices: verts })
} // fn extract_piece_outline

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// @brief Returns true when the group's id marks it as a known non-outline type.
//
// Non-outline groups must not be selected as the cutline / seamline candidate
// by the structural fallback in `find_outline_group`.  The recognised prefixes
// are (compared in lower-case):
//   • notch    — V-notch / tick-mark registration points
//   • tuck     — dart or tuck construction lines (e.g. `tuck_1_a_Back`)
//   • grainline / grain_ — grain direction arrow
//   • ip_      — internal path (pocket placement lines, etc.)
//   • drill / hole — drill-hole markers
//
// `starts_with` is intentional: a piece named "notch" would produce an id
// `cutline_notch` which still passes the primary `contains("cutline")` check
// and is NOT excluded here.
fn is_non_outline_group(e: &Element) -> bool {
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

// @brief Find the cutline group inside a piece (per slspencer's authoritative
// resolution order).
//
// **Resolution order:**
//   1. **Primary — id match.** Any child whose `id` attribute contains the
//      substring `"cutline"`.  Covers `cutline_<piece>` (Seamly2D's
//      canonical naming) as well as `path_cutline_*` variants without
//      pinning the exact form.  Wins over any structural fallback.
//   2. **Fallback — second `<g>` child.**  When no id matches, the cutline
//      sits at structural position 1 (zero-indexed) of the piece's
//      path-bearing `<g>` children, after the seamline at position 0.
//   3. **Fallback — first `<g>` child** when the second child has fewer
//      than 3 vertices.  This catches calibration / single-outline pieces
//      ("Two Inch Gauge") where structure is
//      `[cutline-square, empty-placeholder, gauge-tick, …]` — after the
//      empty-path filter, position 1 holds a 2-vertex tick instead of an
//      outline, so the rule retries position 0.
//
// "Path-bearing" filter: `<g>` children must (a) be `<g>` elements,
// (b) contain a `<path>` descendant, and (c) that path's `d` attribute
// must be non-empty.  Excludes label/text groups and seamline placeholders
// emitted with `<path d="">` when seam allowance is disabled.
fn find_outline_group(piece_g: &Element) -> Option<&Element> {
    // Direct `<g>` children that actually carry path geometry AND are
    // candidates for the piece outline (cutline or seamline).  Groups with
    // well-known non-outline id prefixes (notch, tuck, grainline, ip, …) are
    // excluded so they cannot be mis-selected as the cutline by the structural
    // fallback when no explicit cutline group is present.
    //
    // Example: a piece with `tuck_1_a_Back` and `tuck_1_b_Back` children but
    // no `cutline_Back` must not have the fallback land on a tuck group.
    let path_groups: Vec<&Element> = piece_g
        .children
        .iter()
        .filter_map(|c| if let XMLNode::Element(e) = c { Some(e) } else { None })
        .filter(|e| {
            e.name == "g"
                && first_path_d(e).map_or(false, |d| !d.trim().is_empty())
                && !is_non_outline_group(e)
        })
        .collect();

    // 1. Primary: id contains "cutline".  Iterate in document order so the
    //    first match wins; matching across the piece's full child list
    //    (not only positions 0 / 1) tolerates exports where the cutline
    //    isn't structurally adjacent to the seamline.
    for &g in &path_groups {
        if g.attributes.get("id").map_or(false, |id| id.contains("cutline")) {
            return Some(g);
        }
    }

    // 2. Fallback: structural position 1 (the second path-bearing `<g>`).
    if let Some(second) = path_groups.get(1).copied() {
        if yields_polygon(second) {
            return Some(second);
        }
    }

    // 3. Fallback: structural position 0 when the second child is too short
    //    to be a polygon (calibration-piece edge case described above).
    path_groups.first().copied()
} // fn find_outline_group

// @brief True when this group's first `<path d="…">` parses to at least
// three distinct vertices — the minimum for a closeable polygon.
//
// Cheap because it parses straight to `geometry::Path` and counts segments
// without copying or allocating polygon storage.  Used by `find_outline_group`
// to validate that the structural "second child" candidate is actually an
// outline rather than a 2-vertex tick or notch.
fn yields_polygon(g: &Element) -> bool {
    let Some(d) = first_path_d(g) else { return false; };
    let Ok(parsed) = Path::parse_path_attribute(d) else { return false; };
    let vert_count = parsed
        .segments
        .iter()
        .filter(|s| !matches!(s, PathSegment::Close))
        .count();
    vert_count >= 3
} // fn yields_polygon

// @brief Find the first `<path>` element anywhere in the subtree rooted at `el`,
// and return its `d` attribute.
fn first_path_d(el: &Element) -> Option<&str> {
    if el.name == "path" {
        if let Some(d) = el.attributes.get("d") {
            return Some(d.as_str());
        }
    }
    for child in &el.children {
        if let XMLNode::Element(child_el) = child {
            if let Some(d) = first_path_d(child_el) {
                return Some(d);
            }
        }
    }
    None
} // fn first_path_d

// @brief Compare two `(f64, f64)` vertices for exact equality, allowing a
// 1e-6 tolerance to absorb the trailing-decimal noise sometimes left by
// Seamly2D's path emitter.
fn points_equal(a: (f64, f64), b: (f64, f64)) -> bool {
    (a.0 - b.0).abs() < 1e-6 && (a.1 - b.1).abs() < 1e-6
} // fn points_equal

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    fn parse_svg(xml: &str) -> Element {
        Element::parse(xml.as_bytes()).expect("svg parses")
    }

    // @brief Synthetic SVG with one piece whose cutline is a 4-vertex
    // closed square.  Verifies the basic shape of the output: one piece,
    // 4 vertices, last-vertex-duplicate trimmed, AABB matches.
    #[test]
    fn extracts_simple_cutline_square() {
        let svg = r#"<?xml version="1.0"?>
            <svg xmlns="http://www.w3.org/2000/svg">
                <g id="Square">
                    <g id="cutline_Square">
                        <path id="path_cutline_Square" d="M 0,0 L 10,0 L 10,5 L 0,5 L 0,0"/>
                    </g>
                </g>
            </svg>"#;
        let root = parse_svg(svg);
        let pieces = extract_cutline_polygons(&root);

        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "Square");
        // Closing duplicate trimmed → 4 vertices.
        assert_eq!(pieces[0].polygon.vertices.len(), 4);
        assert_eq!(pieces[0].polygon.vertices[0], (0.0, 0.0));
        assert_eq!(pieces[0].polygon.vertices[2], (10.0, 5.0));
    } // extracts_simple_cutline_square

    // @brief When no cutline group is present, fall back to the seamline.
    #[test]
    fn falls_back_to_seamline_when_no_cutline() {
        let svg = r#"<?xml version="1.0"?>
            <svg xmlns="http://www.w3.org/2000/svg">
                <g id="OldPiece">
                    <g id="seamline_OldPiece">
                        <path d="M 0,0 L 1,0 L 1,1 L 0,0"/>
                    </g>
                </g>
            </svg>"#;
        let root = parse_svg(svg);
        let pieces = extract_cutline_polygons(&root);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].polygon.vertices.len(), 3);
    } // falls_back_to_seamline_when_no_cutline

    // @brief Notch sibling groups must be ignored even when they appear
    // before the cutline group in document order.  Verifies that the notch
    // group's path doesn't accidentally get picked up.
    #[test]
    fn drops_notch_sibling_groups() {
        let svg = r#"<?xml version="1.0"?>
            <svg xmlns="http://www.w3.org/2000/svg">
                <g id="Piece">
                    <g id="notch_1_Piece">
                        <path d="M 100,100 L 101,101"/>
                    </g>
                    <g id="cutline_Piece">
                        <path d="M 0,0 L 10,0 L 10,10 L 0,10 L 0,0"/>
                    </g>
                </g>
            </svg>"#;
        let root = parse_svg(svg);
        let pieces = extract_cutline_polygons(&root);
        assert_eq!(pieces.len(), 1);
        // Should be the cutline's 4 vertices, not the notch's 2.
        assert_eq!(pieces[0].polygon.vertices.len(), 4);
        assert_eq!(pieces[0].polygon.vertices[0], (0.0, 0.0));
    } // drops_notch_sibling_groups

    // @brief `extract_piece_outline` returns Some for a piece with a cutline
    // group and None for a piece carrying only a grainline (non-outline).
    // This is the per-piece helper that the bridge's
    // `extract_piece_rects_and_polygons` calls inside its single-walk loop.
    #[test]
    fn per_piece_helper_returns_some_for_cutline_and_none_for_grainline_only() {
        let svg = r#"<?xml version="1.0"?>
            <svg xmlns="http://www.w3.org/2000/svg">
                <g id="WithCutline">
                    <g id="cutline_WithCutline">
                        <path d="M 0,0 L 8,0 L 8,4 L 0,4 L 0,0"/>
                    </g>
                </g>
                <g id="GrainlineOnly">
                    <g id="grainline_GrainlineOnly">
                        <path d="M 1,0 L 1,5"/>
                    </g>
                </g>
            </svg>"#;
        let root = parse_svg(svg);

        // Pull the two top-level <g>s by walking the children directly so we
        // can call extract_piece_outline against each in isolation.
        let pieces: Vec<&Element> = root.children.iter()
            .filter_map(|c| if let XMLNode::Element(e) = c { Some(e) } else { None })
            .filter(|e| e.name == "g")
            .collect();
        assert_eq!(pieces.len(), 2);

        // Piece 0 has a cutline group → polygon should come back populated.
        let with_cut = extract_piece_outline(pieces[0]);
        assert!(with_cut.is_some(), "cutline piece should yield a polygon");
        assert_eq!(with_cut.unwrap().vertices.len(), 4);

        // Piece 1 has only a grainline (2-vertex tick) → no outline group
        // matches the cutline-id rule, and the structural fallback rejects
        // < 3 vertices.  Helper returns None.
        let grainline_only = extract_piece_outline(pieces[1]);
        assert!(grainline_only.is_none(), "grainline-only piece should yield None");
    } // per_piece_helper_returns_some_for_cutline_and_none_for_grainline_only

    // @brief A piece with neither cutline nor seamline is skipped.
    #[test]
    fn skips_piece_without_outline_group() {
        let svg = r#"<?xml version="1.0"?>
            <svg xmlns="http://www.w3.org/2000/svg">
                <g id="LabelOnly"><text>hi</text></g>
                <g id="Real">
                    <g id="cutline_Real">
                        <path d="M 0,0 L 1,0 L 0,1 L 0,0"/>
                    </g>
                </g>
            </svg>"#;
        let root = parse_svg(svg);
        let pieces = extract_cutline_polygons(&root);
        assert_eq!(pieces.len(), 1);
        assert_eq!(pieces[0].id, "Real");
    } // skips_piece_without_outline_group

    // @brief Tuck sibling groups must never be selected as the cutline fallback.
    //
    // A piece with tuck_1_a_<piece> and tuck_1_b_<piece> siblings but no
    // cutline group should fall back to the seamline (structural position 0
    // after tuck and grainline groups are excluded from path_groups).
    #[test]
    fn find_outline_group_skips_tuck_groups() {
        let svg = r#"<?xml version="1.0"?>
            <svg xmlns="http://www.w3.org/2000/svg">
                <g id="Back">
                    <g id="seamline_Back">
                        <path d="M 0,0 L 10,0 L 10,10 L 0,10 L 0,0"/>
                    </g>
                    <g id="tuck_1_a_Back">
                        <path d="M 3,2 L 5,2 L 4,8"/>
                    </g>
                    <g id="tuck_1_b_Back">
                        <path d="M 5,2 L 7,2 L 6,8"/>
                    </g>
                    <g id="grainline_Back">
                        <path d="M 5,0 L 5,10"/>
                    </g>
                </g>
            </svg>"#;
        let root = parse_svg(svg);
        let pieces = extract_cutline_polygons(&root);

        // The seamline fallback must fire; tuck and grainline groups are excluded.
        assert_eq!(pieces.len(), 1, "expected 1 piece");
        assert_eq!(pieces[0].id, "Back");
        // seamline_Back forms a 10×10 square; after the closing duplicate is
        // trimmed it has 4 vertices.
        assert_eq!(pieces[0].polygon.vertices.len(), 4,
            "expected 4-vertex seamline square, got {} vertices", pieces[0].polygon.vertices.len());
        // Confirm the AABB spans 10 × 10 (the seamline, not a tuck triangle).
        let (dx, dy) = span(&pieces[0].polygon);
        assert!((dx - 10.0).abs() < 1e-6 && (dy - 10.0).abs() < 1e-6,
            "AABB should be 10×10 (seamline); got {dx:.2}×{dy:.2} — tuck group may have been selected");
    } // find_outline_group_skips_tuck_groups

    // @brief Notch groups at structural position 1 must not be selected as fallback.
    //
    // With seamline at structural position 0 and notch_1_Piece at position 1 in the
    // raw child list, the fallback must ignore the notch and use the seamline because
    // notch groups are excluded from path_groups by is_non_outline_group.
    #[test]
    fn find_outline_group_skips_notch_at_structural_position_1() {
        let svg = r#"<?xml version="1.0"?>
            <svg xmlns="http://www.w3.org/2000/svg">
                <g id="Piece">
                    <g id="seamline_Piece">
                        <path d="M 0,0 L 8,0 L 8,6 L 0,6 L 0,0"/>
                    </g>
                    <g id="notch_1_Piece">
                        <path d="M 4,0 L 4,1 L 5,0"/>
                    </g>
                </g>
            </svg>"#;
        let root = parse_svg(svg);
        let pieces = extract_cutline_polygons(&root);

        assert_eq!(pieces.len(), 1);
        // seamline is 4-vertex square (closing dup trimmed).
        assert_eq!(pieces[0].polygon.vertices.len(), 4);
        // Verify AABB matches the 8×6 seamline, not the tiny 3-vertex notch.
        let (dx, dy) = span(&pieces[0].polygon);
        assert!(dx > 7.0 && dy > 5.0,
            "AABB too small ({dx:.2}×{dy:.2}) — notch may have been selected instead of seamline");
    } // find_outline_group_skips_notch_at_structural_position_1

    // @brief Resolve a workspace-relative fixture path from the crate root.
    fn fixture_path(rel: &str) -> std::path::PathBuf {
        std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("..").join("..")
            .join(rel)
    } // fn fixture_path

    // @brief Read a fixture from disk; return `None` (with a `skip:` log line)
    // when the file is absent so a clean clone without input files passes.
    fn read_fixture(rel: &str) -> Option<String> {
        let path = fixture_path(rel);
        match std::fs::read_to_string(&path) {
            Ok(s) => Some(s),
            Err(_) => {
                eprintln!("skip: fixture not present at {:?}", path);
                None
            }
        }
    } // fn read_fixture

    // @brief AABB span `(dx, dy)` of a polygon — tiny helper for the
    // fixture-shape sanity checks below.
    fn span(p: &Polygon) -> (f64, f64) {
        let xs: Vec<f64> = p.vertices.iter().map(|v| v.0).collect();
        let ys: Vec<f64> = p.vertices.iter().map(|v| v.1).collect();
        let dx = xs.iter().cloned().fold(f64::NEG_INFINITY, f64::max)
               - xs.iter().cloned().fold(f64::INFINITY,    f64::min);
        let dy = ys.iter().cloned().fold(f64::NEG_INFINITY, f64::max)
               - ys.iter().cloned().fold(f64::INFINITY,    f64::min);
        (dx, dy)
    } // fn span

    // @brief End-to-end against the real Seamly2D export
    // `qt_frontend/input/MyMullerShirt_pieces_sleeve.svg`.  This exercises
    // the relative-`l` path command (the file's cutline is mostly lowercase),
    // verifies a sleeve-shaped polygon comes out (110+ vertices, plausible
    // AABB), and confirms notch siblings are ignored end-to-end.
    //
    // Path resolution: `CARGO_MANIFEST_DIR` points at this crate's root, so
    // we walk up two levels to reach the workspace root and then into
    // `qt_frontend/input/`.  Skips with a message if the fixture isn't
    // present (e.g. on a clean clone before the user adds inputs).
    #[test]
    fn extracts_real_sleeve_svg() {
        let Some(xml) = read_fixture("qt_frontend/input/MyMullerShirt_pieces_sleeve.svg") else {
            return;
        };
        let root = Element::parse(xml.as_bytes()).expect("svg parses");
        let pieces = extract_cutline_polygons(&root);

        // The sleeve is the only top-level piece group in this file.
        assert_eq!(pieces.len(), 1, "expected 1 piece, got {}", pieces.len());
        let p = &pieces[0];
        assert_eq!(p.id, "Sleeve");

        // Cutline d-attribute has on the order of 110 line segments.
        // Demand "many" without pinning the exact count to keep the test
        // resilient to minor source edits.
        assert!(p.polygon.vertices.len() >= 50,
            "sleeve cutline too short: {} vertices", p.polygon.vertices.len());

        // Sleeve silhouette: roughly as tall as wide.
        let (dx, dy) = span(&p.polygon);
        assert!(dx > 100.0 && dy > 100.0, "AABB suspiciously small: {dx} × {dy}");
        assert!(dy > dx * 0.8, "sleeve should be roughly as tall as wide; got {dx} × {dy}");
    } // extracts_real_sleeve_svg

    // @brief End-to-end against the multi-piece export
    // `qt_frontend/input/richmond-shirt_pieces.svg` — 12 pattern pieces
    // (Sleeve, Front, Back, Yoke, Collar, Collar Stand, Cuff, Sleeve Placket,
    // Front Placket, Pocket, Pocket Flap, Two Inch Gauge).  Verifies the
    // extractor handles a real production-shape pattern with mixed piece
    // sizes, including the trivially-small "Two Inch Gauge" calibration
    // square.  Uses ASCII apostrophes / spaces in piece ids — the extractor
    // must not mangle them.
    #[test]
    fn extracts_richmond_shirt_pieces_svg() {
        let Some(xml) = read_fixture("qt_frontend/input/richmond-shirt_pieces.svg") else {
            return;
        };
        let root = Element::parse(xml.as_bytes()).expect("svg parses");
        let pieces = extract_cutline_polygons(&root);

        // 12 pieces in this file — pin the count so a regression in either
        // the extractor or the fixture is loud.
        assert_eq!(pieces.len(), 12, "got {} pieces", pieces.len());

        // Spot-check that the expected piece names are present.  Pieces with
        // spaces in their id ("Front Placket", "Two Inch Gauge", …) round-
        // trip through xmltree intact.
        let ids: Vec<&str> = pieces.iter().map(|p| p.id.as_str()).collect();
        for expected in [
            "Sleeve", "Front", "Back", "Yoke", "Collar", "Collar Stand",
            "Cuff", "Sleeve Placket", "Front Placket", "Pocket", "Pocket Flap",
            "Two Inch Gauge",
        ] {
            assert!(ids.contains(&expected), "missing piece id {:?}", expected);
        }

        // Every piece must produce a non-degenerate polygon.  Three vertices
        // is the minimum for any plane figure; demanding ≥ 4 catches the
        // case where the extractor accidentally pulled in a notch tick
        // (which is typically a 2-segment hash mark with 3 collinear-ish
        // points).
        for p in &pieces {
            assert!(p.polygon.vertices.len() >= 4,
                "piece {:?} has only {} vertices", p.id, p.polygon.vertices.len());
            let (dx, dy) = span(&p.polygon);
            assert!(dx > 0.0 && dy > 0.0,
                "piece {:?} has zero-area AABB ({dx} × {dy})", p.id);
        }
    } // extracts_richmond_shirt_pieces_svg

    // @brief End-to-end against `qt_frontend/input/richmond-shirt_this_one.svg`,
    // a pattern with 11 pieces and several piece-level transforms
    // (rotations, translations).  The extractor reads vertices straight from
    // the path `d` attribute and intentionally ignores group transforms —
    // see the module-level docs.  This test verifies that:
    //   * The full piece set is recovered despite the transforms,
    //   * Vertex extraction works on transformed groups (they parse the
    //     same way; only the coordinate frame differs from the visual
    //     position in the file).
    //
    // Note: ids in this file lack spaces ("FrontPlacket", "SleevePlacket",
    // "PocketFlap", "CollarStand") — different convention from the
    // `richmond-shirt_pieces.svg` fixture above.
    #[test]
    fn extracts_richmond_shirt_this_one_svg() {
        let Some(xml) = read_fixture("qt_frontend/input/richmond-shirt_this_one.svg") else {
            return;
        };
        let root = Element::parse(xml.as_bytes()).expect("svg parses");
        let pieces = extract_cutline_polygons(&root);

        assert_eq!(pieces.len(), 11, "got {} pieces", pieces.len());

        let ids: Vec<&str> = pieces.iter().map(|p| p.id.as_str()).collect();
        for expected in [
            "Sleeve", "Front", "Back", "Yoke", "Collar", "CollarStand",
            "Cuff", "SleevePlacket", "FrontPlacket", "Pocket", "PocketFlap",
        ] {
            assert!(ids.contains(&expected), "missing piece id {:?}", expected);
        }

        for p in &pieces {
            assert!(p.polygon.vertices.len() >= 4,
                "piece {:?} has only {} vertices", p.id, p.polygon.vertices.len());
            let (dx, dy) = span(&p.polygon);
            assert!(dx > 0.0 && dy > 0.0,
                "piece {:?} has zero-area AABB ({dx} × {dy})", p.id);
        }
    } // extracts_richmond_shirt_this_one_svg
} // mod tests
