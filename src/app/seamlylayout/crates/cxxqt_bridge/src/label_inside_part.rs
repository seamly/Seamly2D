// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file label_inside_part.rs
// @brief Move each label of a split part that lies wholly or partly outside the part outline.
//
// `piece_divider.rs` puts each label whole on the nearest part, so a label can cross a
// cut line or lie past the part outline. Labels: `pattern_label`, `piece_label`, and the
// `fold_line_label` inside the fold line group. One box holds all that a label draws
// (`part_label_placement::drawn_bounds`).
//
// A label whose box lies inside the outline stays. Any other label moves to the place
// nearest its old place where its box, plus a clearance, lies inside the outline.
// Order of preference:
//   1. full size, clear of the other labels and of the reserved boxes;
//   2. full size, anywhere inside the outline;
//   3. smaller: the label shrinks about its centre in 10% steps, then 1 and 2 again.
// A label that fits nowhere at 10% stays where it is.
// Places lie on a grid of whole steps from the old centre, so a label that only crosses
// a cut line moves straight away from it.
// The move goes into the label's paths and text transforms (`svg_dom::flatten_dom`):
// the DOM keeps no group transform.

use xmltree::{Element, XMLNode};

use crate::part_label_placement::{drawn_bounds, inside, x_range_in_band, Bounds};

// `data-type` of the label groups that must lie inside the part.
const LABEL_TYPES: [&str; 3] = ["pattern_label", "piece_label", "fold_line_label"];
// Gap between a moved label and the outline or another label; also the smallest grid step.
const LABEL_CLEARANCE_IN: f64 = 0.0625;
// Most grid steps across the part: a large part gets a coarser step.
const MAX_STEPS_ACROSS: f64 = 200.0;
// Shrink steps when the full-size label fits nowhere: 100%, 90%, ... 10%.
const SCALE_STEPS: usize = 10;

// @brief Where a moved label goes and how large it is.
#[derive(Debug, Clone, Copy, PartialEq)]
struct LabelFit {
    // New centre of the label box.
    centre: (f64, f64),
    // Size factor about the old centre; 1 keeps the size.
    scale: f64,
}

// @brief Move each label of a part that is not wholly inside `outline` to a place inside it.
// @param outline Part outline: a closed polygon.
// @param components The part's components; label groups among them, or inside them, change in place.
// @param reserved Boxes a moved label keeps clear of when it can, such as the part label's place.
// @param uu_per_in User units per inch.
pub(crate) fn move_labels_inside_part(
    outline: &[(f64, f64)],
    components: &mut [Element],
    reserved: &[Bounds],
    uu_per_in: f64,
) {
    let Some(outline_box) = Bounds::around(outline.iter().copied()) else {
        return; // no outline
    }; // outline box
    let clearance = LABEL_CLEARANCE_IN * uu_per_in;

    // Where each label is in the component tree, and the box it covers now.
    let mut paths: Vec<Vec<usize>> = Vec::new();
    for (index, component) in components.iter().enumerate() {
        label_paths(component, vec![index], &mut paths);
    } // for each component
    let mut boxes: Vec<Option<Bounds>> = paths.iter().map(|path| drawn_bounds(element_at(components, path))).collect();

    for k in 0..paths.len() {
        let Some(old) = boxes[k] else {
            continue; // the label draws nothing
        }; // old box
        if box_inside(outline, old) {
            continue; // already inside the part
        } // if inside

        // Keep clear of the other labels where they are now, and of the reserved boxes.
        let obstacles: Vec<Bounds> = boxes
            .iter()
            .enumerate()
            .filter(|&(j, _)| j != k)
            .filter_map(|(_, other)| *other)
            .chain(reserved.iter().copied())
            .collect();
        let Some(fit) = find_fit(outline, outline_box, old, &obstacles, clearance) else {
            continue; // fits nowhere: stays where it is
        }; // fit

        // Move the label and record its new box for the labels after it.
        let from = centre_of(old);
        move_label(element_at_mut(components, &paths[k]), from, fit);
        let (half_width, half_height) = half_size(old, fit.scale);
        boxes[k] = Some(Bounds {
            left: fit.centre.0 - half_width,
            top: fit.centre.1 - half_height,
            right: fit.centre.0 + half_width,
            bottom: fit.centre.1 + half_height,
        });
    } // for each label
} // fn move_labels_inside_part

// @brief Add the tree path (child indexes) of each label group at or below `element`.
// @param path Indexes that lead to `element`: component index, then child node indexes.
fn label_paths(element: &Element, path: Vec<usize>, out: &mut Vec<Vec<usize>>) {
    let data_type = element.attributes.get("data-type").map_or("", String::as_str);
    if LABEL_TYPES.contains(&data_type) {
        out.push(path);
        return; // a label holds no other label
    } // if label
    for (n, node) in element.children.iter().enumerate() {
        if let XMLNode::Element(child) = node {
            let mut child_path = path.clone();
            child_path.push(n);
            label_paths(child, child_path, out);
        } // if element
    } // for each child
} // fn label_paths

// @brief The element at `path` (see `label_paths`).
fn element_at<'a>(components: &'a [Element], path: &[usize]) -> &'a Element {
    path[1..].iter().fold(&components[path[0]], |el, &n| match &el.children[n] {
        XMLNode::Element(child) => child,
        _ => unreachable!("label_paths records element nodes only"),
    })
} // fn element_at

// @brief The element at `path` (see `label_paths`), to change.
fn element_at_mut<'a>(components: &'a mut [Element], path: &[usize]) -> &'a mut Element {
    path[1..].iter().fold(&mut components[path[0]], |el, &n| match &mut el.children[n] {
        XMLNode::Element(child) => child,
        _ => unreachable!("label_paths records element nodes only"),
    })
} // fn element_at_mut

// @brief Centre of a box.
fn centre_of(b: Bounds) -> (f64, f64) {
    ((b.left + b.right) / 2.0, (b.top + b.bottom) / 2.0)
} // fn centre_of

// @brief Half width and half height of a box at `scale`.
fn half_size(b: Bounds, scale: f64) -> (f64, f64) {
    ((b.right - b.left) * scale / 2.0, (b.bottom - b.top) * scale / 2.0)
} // fn half_size

// @brief True when `b` lies inside the closed polygon `outline`: its corners are inside
// and no outline edge crosses it.
fn box_inside(outline: &[(f64, f64)], b: Bounds) -> bool {
    let corners = [(b.left, b.top), (b.right, b.top), (b.right, b.bottom), (b.left, b.bottom)];
    if !corners.iter().all(|&corner| inside(outline, corner)) {
        return false; // a corner lies outside
    } // if a corner is outside
    // A concave outline can reach into the box between its corners.
    let n = outline.len();
    (0..n).all(|i| {
        x_range_in_band(outline[i], outline[(i + 1) % n], b.top, b.bottom)
            .map_or(true, |(x0, x1)| x1 <= b.left || x0 >= b.right)
    })
} // fn box_inside

// @brief Place and size for box `old` inside `outline`; full size wins over any smaller size.
// @param outline_box Box around the outline.
// @param obstacles Boxes the label keeps clear of when it can.
// @param clearance Gap to the outline and to the obstacles.
// @return The fit; None when the label fits nowhere, even at the smallest size.
fn find_fit(
    outline: &[(f64, f64)],
    outline_box: Bounds,
    old: Bounds,
    obstacles: &[Bounds],
    clearance: f64,
) -> Option<LabelFit> {
    let longest_side = (outline_box.right - outline_box.left).max(outline_box.bottom - outline_box.top);
    let step = clearance.max(longest_side / MAX_STEPS_ACROSS);
    let from = centre_of(old);
    (0..SCALE_STEPS).find_map(|k| {
        // 1, 0.9, ... 0.1 of the old size, about the old centre.
        let scale = 1.0 - k as f64 / SCALE_STEPS as f64;
        let half = half_size(old, scale);
        nearest_place(outline, outline_box, from, half, obstacles, clearance, step)
            .map(|centre| LabelFit { centre, scale })
    })
} // fn find_fit

// @brief Grid centre nearest `from` where a box of half size `half`, grown by `clearance`,
// lies inside `outline`. A centre clear of `obstacles` wins over a nearer one that is not.
// @param step Grid step; centres lie whole steps from `from`.
// @return The centre; None when no grid centre puts the box inside the outline.
fn nearest_place(
    outline: &[(f64, f64)],
    outline_box: Bounds,
    from: (f64, f64),
    half: (f64, f64),
    obstacles: &[Bounds],
    clearance: f64,
    step: f64,
) -> Option<(f64, f64)> {
    // Whole steps from `at` that keep a centre inside `low`..=`high`.
    let steps = |low: f64, high: f64, at: f64| ((low - at) / step).ceil() as i64..=((high - at) / step).floor() as i64;
    // The box and its clearance must lie inside the outline box.
    let x_steps = steps(outline_box.left + half.0 + clearance, outline_box.right - half.0 - clearance, from.0);
    let y_steps = steps(outline_box.top + half.1 + clearance, outline_box.bottom - half.1 - clearance, from.1);
    let mut offsets: Vec<(i64, i64)> = x_steps.flat_map(|i| y_steps.clone().map(move |j| (i, j))).collect();
    // Nearest first; ties: upper first, then left first.
    offsets.sort_by_key(|&(i, j)| (i * i + j * j, j, i));

    let mut inside_not_clear: Option<(f64, f64)> = None;
    for (i, j) in offsets {
        let centre = (from.0 + i as f64 * step, from.1 + j as f64 * step);
        let at = Bounds { left: centre.0 - half.0, top: centre.1 - half.1, right: centre.0 + half.0, bottom: centre.1 + half.1 }
            .grown(clearance);
        let clear = obstacles.iter().all(|obstacle| !at.overlaps(obstacle));
        if !clear && inside_not_clear.is_some() {
            continue; // a nearer place inside is known; only a clear one can win now
        } // if not clear
        if !inside(outline, centre) || !box_inside(outline, at) {
            continue; // crosses or lies past the outline
        } // if not inside
        if clear {
            return Some(centre); // nearest clear place inside
        } // if clear
        inside_not_clear = Some(centre); // nearest place inside
    } // for each grid centre
    inside_not_clear
} // fn nearest_place

// @brief Move `label` so the centre of its box goes from `from` to `fit.centre`, sized by `fit.scale`.
//
// The matrix goes into each path and text transform below the label, as `flatten_dom` does.
fn move_label(label: &mut Element, from: (f64, f64), fit: LabelFit) {
    let LabelFit { centre, scale } = fit;
    // translate(centre) scale(scale) translate(-from)
    let (e, f) = (centre.0 - scale * from.0, centre.1 - scale * from.1);
    let mut moved = svg_dom::Document { root: std::mem::replace(label, Element::new("g")) };
    moved.root.attributes.insert("transform".to_string(), format!("matrix({scale} 0 0 {scale} {e} {f})"));
    svg_dom::flatten_dom(&mut moved);
    *label = moved.root;
} // fn move_label

#[cfg(test)]
mod tests {
    use super::*;

    // 96 user units per inch: the clearance is 6 uu.
    const UU_PER_IN: f64 = 96.0;
    const CLEARANCE: f64 = 6.0;

    // @brief Closed rectangle outline.
    fn rectangle(left: f64, top: f64, right: f64, bottom: f64) -> Vec<(f64, f64)> {
        vec![(left, top), (right, top), (right, bottom), (left, bottom)]
    } // fn rectangle

    // @brief Components parsed from SVG markup.
    fn components(markup: &str) -> Vec<Element> {
        let root = Element::parse(format!("<g>{markup}</g>").as_bytes()).expect("markup parses");
        root.children.into_iter().filter_map(|n| n.as_element().cloned()).collect()
    } // fn components

    // @brief Box of the component with `data-type`, at any depth.
    fn label_box(components: &[Element], data_type: &str) -> Bounds {
        fn find<'a>(el: &'a Element, data_type: &str) -> Option<&'a Element> {
            if el.attributes.get("data-type").map(String::as_str) == Some(data_type) {
                return Some(el);
            } // if found
            el.children.iter().filter_map(XMLNode::as_element).find_map(|child| find(child, data_type))
        } // fn find
        let label = components.iter().find_map(|c| find(c, data_type)).expect("label");
        drawn_bounds(label).expect("label draws")
    } // fn label_box

    // @brief Assert `b` grown by the clearance lies inside `outline`.
    fn assert_inside(outline: &[(f64, f64)], b: Bounds) {
        assert!(box_inside(outline, b.grown(CLEARANCE - 1e-6)), "{b:?} is not inside {outline:?}");
    } // fn assert_inside

    const LEVEL_LABEL: &str = r#"<g data-type="pattern_label" font-size="18">
          <text x="0" y="19" transform="matrix(1 0 0 1 150 20)">Back</text>
          <text x="0" y="19" transform="matrix(1 0 0 1 150 44)">Cut 2</text>
        </g>"#;

    // @brief A label inside the outline does not change.
    #[test]
    fn label_inside_the_outline_stays() {
        let mut parts = components(LEVEL_LABEL);
        let before = parts.clone();
        move_labels_inside_part(&rectangle(0.0, 0.0, 400.0, 200.0), &mut parts, &[], UU_PER_IN);
        assert_eq!(parts, before);
    } // label_inside_the_outline_stays

    // @brief A label across the cut line moves straight left into the part, at full size.
    #[test]
    fn label_across_the_cut_line_moves_inside() {
        let outline = rectangle(0.0, 0.0, 180.0, 200.0);
        let mut parts = components(LEVEL_LABEL);
        let before = label_box(&parts, "pattern_label");
        assert!(before.right > 180.0, "the label crosses the cut line: {before:?}");
        move_labels_inside_part(&outline, &mut parts, &[], UU_PER_IN);
        let after = label_box(&parts, "pattern_label");
        assert_inside(&outline, after);
        // Same size and height; moved left by less than one grid step past the clearance.
        assert!((after.right - after.left - (before.right - before.left)).abs() < 1e-3, "{after:?}");
        assert!((after.top - before.top).abs() < 1e-3, "{after:?}");
        assert!(after.right <= 180.0 - CLEARANCE && after.right > 180.0 - 2.0 * CLEARANCE, "{after:?}");
        // The text keeps its font size; the move is in its transform.
        let text = parts[0].children.iter().filter_map(XMLNode::as_element).next().expect("text");
        assert_eq!(text.attributes.get("x").map(String::as_str), Some("0"));
        let m = svg_dom::parse_svg_transform(&text.attributes["transform"]);
        assert_eq!((m.a, m.d), (1.0, 1.0));
    } // label_across_the_cut_line_moves_inside

    // @brief A moved label keeps clear of the other labels and of the reserved boxes.
    #[test]
    fn moved_label_keeps_clear_of_other_labels_and_reserved_boxes() {
        let outline = rectangle(0.0, 0.0, 180.0, 300.0);
        let mut parts = components(&format!(
            r#"{LEVEL_LABEL}
            <g data-type="piece_label" font-size="18"><text x="0" y="19" transform="matrix(1 0 0 1 60 20)">Front</text></g>"#
        ));
        let reserved = Bounds { left: 120.0, top: 60.0, right: 180.0, bottom: 300.0 };
        move_labels_inside_part(&outline, &mut parts, &[reserved], UU_PER_IN);
        let moved = label_box(&parts, "pattern_label");
        assert_inside(&outline, moved);
        assert!(!moved.overlaps(&label_box(&parts, "piece_label")), "{moved:?}");
        assert!(!moved.overlaps(&reserved), "{moved:?}");
    } // moved_label_keeps_clear_of_other_labels_and_reserved_boxes

    // @brief A label larger than the part shrinks until it fits; its paths shrink with it.
    #[test]
    fn label_larger_than_the_part_shrinks_to_fit() {
        let outline = rectangle(0.0, 0.0, 100.0, 100.0);
        let mut parts = components(
            r#"<g data-type="piece_label" font-size="18">
              <path d="M0,0 L160,0 L160,40 L0,40 Z"/>
              <text x="0" y="19" transform="matrix(1 0 0 1 10 10)">Back</text>
            </g>"#,
        );
        move_labels_inside_part(&outline, &mut parts, &[], UU_PER_IN);
        let after = label_box(&parts, "piece_label");
        assert_inside(&outline, after);
        assert!(after.right - after.left < 100.0 - 2.0 * CLEARANCE + 1e-6, "{after:?}");
        let text = parts[0].children.iter().filter_map(XMLNode::as_element).find(|e| e.name == "text").expect("text");
        let m = svg_dom::parse_svg_transform(&text.attributes["transform"]);
        assert!(m.a < 1.0 && (m.a - m.d).abs() < 1e-6, "text shrinks evenly: {m:?}");
    } // label_larger_than_the_part_shrinks_to_fit

    // @brief The fold line label inside the fold line group moves; the fold line stays.
    #[test]
    fn fold_line_label_in_the_fold_line_group_moves() {
        let outline = rectangle(0.0, 0.0, 200.0, 200.0);
        let mut parts = components(
            r#"<g data-type="fold_line"><path d="M0,0 L0,200"/>
              <g data-type="fold_line_label" font-size="12"><text x="0" y="0" transform="matrix(1 0 0 1 -20 100)">Foldline</text></g>
            </g>"#,
        );
        let fold_line_d = |parts: &[Element]| {
            parts[0].get_child("path").and_then(|p| p.attributes.get("d")).cloned().expect("fold line")
        };
        let d_before = fold_line_d(&parts);
        move_labels_inside_part(&outline, &mut parts, &[], UU_PER_IN);
        assert_inside(&outline, label_box(&parts, "fold_line_label"));
        assert_eq!(fold_line_d(&parts), d_before);
    } // fold_line_label_in_the_fold_line_group_moves

    // @brief A label with every corner inside a concave outline, but across a slot in it, moves.
    #[test]
    fn label_across_a_slot_in_the_outline_moves() {
        // 300 × 200 with a 60 uu wide slot from the top down to y 150, at x 120..180.
        let outline = vec![
            (0.0, 0.0), (120.0, 0.0), (120.0, 150.0), (180.0, 150.0), (180.0, 0.0),
            (300.0, 0.0), (300.0, 200.0), (0.0, 200.0),
        ];
        let mut parts = components(
            r#"<g data-type="pattern_label" font-size="18"><text x="0" y="19" transform="matrix(1 0 0 1 100 20)">Pattern piece</text></g>"#,
        );
        let before = label_box(&parts, "pattern_label");
        let corners = [(before.left, before.top), (before.right, before.top), (before.right, before.bottom), (before.left, before.bottom)];
        assert!(corners.iter().all(|&c| inside(&outline, c)), "every corner is inside: {before:?}");
        assert!(!box_inside(&outline, before), "the slot crosses the label");
        move_labels_inside_part(&outline, &mut parts, &[], UU_PER_IN);
        assert_inside(&outline, label_box(&parts, "pattern_label"));
    } // label_across_a_slot_in_the_outline_moves
}
