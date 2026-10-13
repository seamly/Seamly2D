// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file layout_views.rs
// @brief The layouts a multisize import shows as tabs: Marker, Nested, Base Size, then one per size.
//
// The controller keeps the active layout in its own fields, because the canvas,
// Adjust Mode and every export read those fields.  `LayoutViews` holds the
// inactive layouts.  Selecting a tab stores the controller's layout into the
// old view and hands back the new view's layout, so the rest of the app always
// works on the selected tab without knowing that tabs exist.
//
// Every tab takes its pieces from the imported SVG: each size of each piece is
// already drafted there, so no tab builds a size from grade rules.

use std::collections::VecDeque;

use svg_dom::Document;

use crate::layout_utils::ProcessLayoutResult;

// @brief Label of the tab where every size of every piece packs as its own piece.
pub const MARKER_LABEL: &str = "Marker";

// @brief Label of the tab where each piece packs once, as a group of its sizes.
pub const NESTED_LABEL: &str = "Nested";

// @brief Label of the tab for one size.
pub fn size_label(size: &str) -> String {
    format!("Size {size}")
} // fn size_label

// @brief Label of the base size tab.
pub fn base_size_label(size: &str) -> String {
    format!("Base Size {size}")
} // fn base_size_label

// @brief What one tab lays out.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ViewKind {
    // Every size of every piece packs as its own piece.
    Marker,
    // Each piece packs once, as a group of all its sizes.
    Nested,
    // The base size.  Its DXF-ASTM export carries the grade rules.
    BaseSize(String),
    // One size.
    Size(String),
}

impl ViewKind {
    // @brief Text on the tab.
    pub fn label(&self) -> String {
        match self {
            ViewKind::Marker => MARKER_LABEL.to_string(),
            ViewKind::Nested => NESTED_LABEL.to_string(),
            ViewKind::BaseSize(size) => base_size_label(size),
            ViewKind::Size(size) => size_label(size),
        } // match kind
    } // fn label

    // @brief `data-size` the tab lays out; `None` for Marker and Nested, which hold every size.
    pub fn size(&self) -> Option<&str> {
        match self {
            ViewKind::BaseSize(size) | ViewKind::Size(size) => Some(size),
            ViewKind::Marker | ViewKind::Nested => None,
        } // match kind
    } // fn size

    // @brief True when the tab packs each piece-set (one piece in every size) as one unit.
    pub fn nested(&self) -> bool {
        *self == ViewKind::Nested
    } // fn nested

    // @brief True when DXF-ASTM can export the tab.
    //
    // A DXF-ASTM file carries one size; D6673 §5.1 excludes marker-laying data.
    pub fn exports_dxf(&self) -> bool {
        self.size().is_some()
    } // fn exports_dxf

    // @brief True when Export all tabs writes this tab in `format`.
    //
    // @param format Export format name from QML: "dxf", "pdf", "svg", ...
    // DXF-ASTM writes every tab it can export.  Other formats skip the Base Size
    // tab: its layout repeats the size tab of the same size.
    pub fn in_all_tabs_export(&self, format: &str) -> bool {
        if format == "dxf" {
            self.exports_dxf()
        } else {
            !matches!(self, ViewKind::BaseSize(_))
        } // if DXF
    } // fn in_all_tabs_export
} // impl ViewKind

// @brief Tabs that follow the first (Marker) tab of a multisize layout, in tab order.
//
// @param sizes     Every size of the import, in handoff order.
// @param base_size `data-base-size`; the Base Size tab is left out when it is not one of `sizes`.
pub fn queued_kinds(sizes: &[String], base_size: &str) -> Vec<ViewKind> {
    let mut kinds = vec![ViewKind::Nested];
    if sizes.iter().any(|s| s == base_size) {
        kinds.push(ViewKind::BaseSize(base_size.to_string()));
    } // if base size known
    kinds.extend(sizes.iter().map(|s| ViewKind::Size(s.clone())));
    kinds
} // fn queued_kinds

// @brief Export path for one tab: the tab label, made file-safe, before the extension.
//
// "C:/out/shirt.pdf" + "Size 40" → "C:/out/shirt_size-40.pdf".  Every character that
// is not a letter or digit becomes '-', so a size such as "40/42" cannot add a
// directory.  A path without an extension gets the suffix at the end.
pub fn tab_file_path(path: &str, label: &str) -> String {
    let suffix: String = label
        .chars()
        .map(|c| if c.is_alphanumeric() { c.to_ascii_lowercase() } else { '-' })
        .collect();

    // Split at the last '.' of the file name only; a '.' in a directory is not an extension.
    let name_start = path.rfind(['/', '\\']).map_or(0, |i| i + 1);
    match path[name_start..].rfind('.') {
        Some(dot) if dot > 0 => {
            let dot = name_start + dot;
            format!("{}_{suffix}{}", &path[..dot], &path[dot..])
        } // Some extension
        _ => format!("{path}_{suffix}"),
    } // match extension
} // fn tab_file_path

// @brief The per-layout controller fields that change when the tab changes.
//
// Fields the layouts share (settings, canvas width, roll form, the imported
// document) stay on the controller.
#[derive(Default, Clone)]
pub struct LayoutState {
    pub layout_dom: Option<Document>,
    pub layout_h_px: u32,
    pub layout_ml_px: u32,
    pub layout_mt_px: u32,
    pub piece_bboxes_json: String,
    pub flat_dom: Option<Document>,
    pub vertical_dom: Option<Document>,
    pub translate_dom: Option<Document>,
}

impl LayoutState {
    // @brief Build the state of a finished layout run.
    pub fn from_result(result: ProcessLayoutResult) -> Self {
        Self {
            layout_dom: Some(result.output_doc),
            layout_h_px: result.layout_h_px,
            layout_ml_px: result.ml_px,
            layout_mt_px: result.mt_px,
            piece_bboxes_json: result.bbox_json,
            flat_dom: Some(result.flat_dom),
            vertical_dom: Some(result.vertical_dom),
            translate_dom: Some(result.translate_dom),
        }
    } // fn from_result
} // impl LayoutState

// @brief One tab.
struct LayoutView {
    // Text on the tab.
    label: String,
    // What this view lays out.
    kind: ViewKind,
    // Empty while this view is active: the controller holds its layout then.
    state: LayoutState,
}

// @brief Every tab of the current layout and which one is active.
//
// Empty for an individual import or before a layout exists; the app then shows no tabs.
#[derive(Default)]
pub struct LayoutViews {
    views: Vec<LayoutView>,
    active: usize,
}

impl LayoutViews {
    // @brief Remove every view.
    pub fn clear(&mut self) {
        self.views.clear();
        self.active = 0;
    } // fn clear

    // @brief Start a multisize layout: one active view, whose layout the controller holds.
    pub fn start(&mut self, kind: ViewKind) {
        self.clear();
        self.views.push(LayoutView { label: kind.label(), kind, state: LayoutState::default() });
    } // fn start

    // @brief Add a finished layout as an inactive view.
    pub fn push(&mut self, kind: ViewKind, state: LayoutState) {
        self.views.push(LayoutView { label: kind.label(), kind, state });
    } // fn push

    // @brief Tab labels in display order.
    pub fn labels(&self) -> Vec<String> {
        self.views.iter().map(|v| v.label.clone()).collect()
    } // fn labels

    // @brief Label of view `index`; `None` when out of range.
    pub fn label(&self, index: usize) -> Option<&str> {
        self.views.get(index).map(|v| v.label.as_str())
    } // fn label

    // @brief Kind of view `index`; `None` when out of range.
    pub fn kind(&self, index: usize) -> Option<&ViewKind> {
        self.views.get(index).map(|v| &v.kind)
    } // fn kind

    // @brief Index of the active view.
    pub fn active(&self) -> usize {
        self.active
    } // fn active

    // @brief Kind of the active view; `None` when there are no views (individual import).
    pub fn active_kind(&self) -> Option<&ViewKind> {
        self.kind(self.active)
    } // fn active_kind

    // @brief Make another view active.
    //
    // @param index   View to activate.
    // @param current The controller's layout, which belongs to the active view.
    // @return The new active view's layout for the controller to hold, or
    //         `Err(current)` unchanged when `index` is out of range.
    pub fn select(&mut self, index: usize, current: LayoutState) -> Result<LayoutState, LayoutState> {
        if index >= self.views.len() {
            return Err(current);
        } // if out of range
        if index == self.active {
            return Ok(current);
        } // if already active

        // Park the controller's layout in the old view, then move the new one out.
        self.views[self.active].state = current;
        self.active = index;
        Ok(std::mem::take(&mut self.views[index].state))
    } // fn select
} // impl LayoutViews

// @brief Tabs still to lay out after the first one, one per UI tick.
//
// Each tab packs in its own call so the event loop can repaint the progress
// popup, which names the tab, between tabs.
pub struct TabLayoutQueue {
    // Settings of the first run; every tab uses the same settings.
    pub settings_json: String,
    // Canvas height before the first run trimmed it.
    pub layout_h_px: u32,
    // Tabs not yet laid out, in tab order.
    pending: VecDeque<ViewKind>,
    // Number of tabs in the run, the first one included.
    total: usize,
    // One line per layout with unplaced pieces or a failure: "Size 36: Front, Back".
    pub problems: Vec<String>,
    // Base Size layout and its size: the size tab of that size shows the same layout.
    base_layout: Option<(String, ProcessLayoutResult)>,
}

impl TabLayoutQueue {
    // @brief Queue the tabs that follow the first one.
    pub fn new(settings_json: String, layout_h_px: u32, kinds: Vec<ViewKind>) -> Self {
        let total = kinds.len() + 1;
        Self { settings_json, layout_h_px, pending: kinds.into(), total, problems: Vec::new(), base_layout: None }
    } // fn new

    // @brief Take the next tab to lay out.
    pub fn pop(&mut self) -> Option<ViewKind> {
        self.pending.pop_front()
    } // fn pop

    // @brief Popup text for the next tab: "Laying out Size 36 (4 of 6)…"; `None` when empty.
    pub fn next_message(&self) -> Option<String> {
        let kind = self.pending.front()?;
        let number = self.total - self.pending.len() + 1;
        Some(format!("Laying out {} ({number} of {})…", kind.label(), self.total))
    } // fn next_message

    // @brief Keep a finished Base Size layout for the size tab of the same size.
    pub fn keep_base_layout(&mut self, kind: &ViewKind, result: &ProcessLayoutResult) {
        if let ViewKind::BaseSize(size) = kind {
            self.base_layout = Some((size.clone(), result.clone()));
        } // if Base Size
    } // fn keep_base_layout

    // @brief The Base Size layout when `kind` is the size tab of the base size.
    pub fn reusable_layout(&self, kind: &ViewKind) -> Option<ProcessLayoutResult> {
        match (kind, &self.base_layout) {
            (ViewKind::Size(size), Some((base, result))) if size == base => Some(result.clone()),
            _ => None,
        } // match kind
    } // fn reusable_layout
} // impl TabLayoutQueue

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Sizes from string literals.
    fn sizes(list: &[&str]) -> Vec<String> {
        list.iter().map(|s| s.to_string()).collect()
    } // fn sizes

    // @brief Tab order: Nested, Base Size, then every size; no Base Size tab for an unknown base size.
    #[test]
    fn queued_kinds_follow_tab_order() {
        let kinds = queued_kinds(&sizes(&["34", "36"]), "36");
        assert_eq!(
            kinds,
            vec![ViewKind::Nested, ViewKind::BaseSize("36".into()), ViewKind::Size("34".into()), ViewKind::Size("36".into())]
        );
        let labels: Vec<String> = kinds.iter().map(ViewKind::label).collect();
        assert_eq!(labels, vec!["Nested", "Base Size 36", "Size 34", "Size 36"]);

        for base in ["", "99"] {
            let kinds = queued_kinds(&sizes(&["34", "36"]), base);
            assert!(!kinds.iter().any(|k| matches!(k, ViewKind::BaseSize(_))), "base size {base:?}");
        } // for each unknown base size
    } // queued_kinds_follow_tab_order

    // @brief DXF-ASTM exports one size; Export all tabs skips the repeated Base Size layout in other formats.
    #[test]
    fn view_kinds_choose_their_exports() {
        let (marker, nested) = (ViewKind::Marker, ViewKind::Nested);
        let (base, size) = (ViewKind::BaseSize("36".into()), ViewKind::Size("34".into()));
        assert!(!marker.exports_dxf() && !nested.exports_dxf());
        assert!(base.exports_dxf() && size.exports_dxf());

        let in_dxf: Vec<bool> = [&marker, &nested, &base, &size].iter().map(|k| k.in_all_tabs_export("dxf")).collect();
        assert_eq!(in_dxf, vec![false, false, true, true]);
        let in_pdf: Vec<bool> = [&marker, &nested, &base, &size].iter().map(|k| k.in_all_tabs_export("pdf")).collect();
        assert_eq!(in_pdf, vec![true, true, false, true]);

        assert!(nested.nested() && !marker.nested() && !base.nested());
        assert_eq!((marker.size(), base.size(), size.size()), (None, Some("36"), Some("34")));
    } // view_kinds_choose_their_exports

    // @brief The popup names each tab in order and counts the first tab as done.
    #[test]
    fn tab_queue_messages_count_up() {
        let mut queue = TabLayoutQueue::new(String::new(), 0, vec![ViewKind::Nested, ViewKind::Size("34".into())]);
        assert_eq!(queue.next_message().as_deref(), Some("Laying out Nested (2 of 3)…"));
        assert_eq!(queue.pop(), Some(ViewKind::Nested));
        assert_eq!(queue.next_message().as_deref(), Some("Laying out Size 34 (3 of 3)…"));
        assert_eq!(queue.pop(), Some(ViewKind::Size("34".into())));
        assert_eq!(queue.pop(), None);
        assert_eq!(queue.next_message(), None);
    } // tab_queue_messages_count_up

    // @brief A finished layout whose bbox JSON names it.
    fn result(name: &str) -> ProcessLayoutResult {
        let doc = Document::parse("<svg xmlns=\"http://www.w3.org/2000/svg\"/>").expect("empty SVG parses");
        ProcessLayoutResult {
            output_doc: doc.clone(),
            bbox_json: name.to_string(),
            layout_h_px: 0,
            ml_px: 0,
            mt_px: 0,
            unplaced_labels: vec![format!("{name} piece")],
            flat_dom: doc.clone(),
            vertical_dom: doc.clone(),
            translate_dom: doc,
            divide_spec: None,
        }
    } // fn result

    // @brief The size tab of the base size reuses the Base Size layout; other tabs do not.
    #[test]
    fn base_size_layout_is_reused_once_kept() {
        let mut queue = TabLayoutQueue::new(String::new(), 0, Vec::new());
        let base = ViewKind::BaseSize("36".into());
        assert!(queue.reusable_layout(&ViewKind::Size("36".into())).is_none(), "nothing kept yet");

        queue.keep_base_layout(&ViewKind::Size("34".into()), &result("s34"));
        assert!(queue.reusable_layout(&ViewKind::Size("34".into())).is_none(), "only a Base Size layout is kept");

        queue.keep_base_layout(&base, &result("base"));
        let reused = queue.reusable_layout(&ViewKind::Size("36".into())).expect("size 36 reuses Base Size 36");
        assert_eq!((reused.bbox_json.as_str(), reused.unplaced_labels.len()), ("base", 1));
        assert!(queue.reusable_layout(&ViewKind::Size("34".into())).is_none());
        assert!(queue.reusable_layout(&ViewKind::Nested).is_none());
    } // base_size_layout_is_reused_once_kept

    // @brief Each tab writes its own file: the label goes before the extension, file-safe.
    #[test]
    fn tab_file_path_adds_label_before_extension() {
        assert_eq!(tab_file_path("C:/out/shirt.pdf", "Size 40"), "C:/out/shirt_size-40.pdf");
        assert_eq!(tab_file_path(r"C:\out\shirt_tiled.pdf", MARKER_LABEL), r"C:\out\shirt_tiled_marker.pdf");
        assert_eq!(tab_file_path("/out/shirt.dxf", &base_size_label("40")), "/out/shirt_base-size-40.dxf");
        // A '/' in a size name must not create a directory.
        assert_eq!(tab_file_path("/out/shirt.svg", "Size 40/42"), "/out/shirt_size-40-42.svg");
        // A '.' in a directory is not an extension.
        assert_eq!(tab_file_path("/out.v2/shirt", "Size 40"), "/out.v2/shirt_size-40");
        // A hidden-file style name keeps its leading dot.
        assert_eq!(tab_file_path("/out/.pdf", "Size 40"), "/out/.pdf_size-40");
        // A mode segment stays in place; the label still goes before the extension.
        assert_eq!(tab_file_path("/out/shirt_cutlines_202610011721.plt", "Size 40"), "/out/shirt_cutlines_202610011721_size-40.plt");
        assert_eq!(tab_file_path("/out/shirt_hersheyfont_202610011721.svg", "Size 40"), "/out/shirt_hersheyfont_202610011721_size-40.svg");
    } // tab_file_path_adds_label_before_extension

    // @brief A state whose bbox JSON names it, so a test can tell states apart.
    fn state(name: &str) -> LayoutState {
        LayoutState { piece_bboxes_json: name.to_string(), ..LayoutState::default() }
    } // fn state

    // @brief Views keep their order and labels; the first view starts active.
    #[test]
    fn labels_and_active_kind() {
        let mut views = LayoutViews::default();
        assert!(views.labels().is_empty());
        assert_eq!(views.active_kind(), None);
        views.start(ViewKind::Marker);
        views.push(ViewKind::Nested, state("nested"));
        views.push(ViewKind::BaseSize("36".into()), state("base"));
        views.push(ViewKind::Size("34".into()), state("s34"));
        assert_eq!(views.labels(), vec!["Marker", "Nested", "Base Size 36", "Size 34"]);
        assert_eq!(views.label(3), Some("Size 34"));
        assert_eq!(views.label(4), None);
        assert_eq!(views.kind(2), Some(&ViewKind::BaseSize("36".into())));
        assert_eq!(views.active(), 0);
        assert_eq!(views.active_kind(), Some(&ViewKind::Marker));
    } // labels_and_active_kind

    // @brief A change made on a tab survives a switch away and back.
    #[test]
    fn select_round_trip_keeps_each_layout() {
        let mut views = LayoutViews::default();
        views.start(ViewKind::Marker);
        views.push(ViewKind::Size("34".into()), state("s34"));

        // The controller holds the Marker layout; switching gives it size 34.
        let held = views.select(1, state("marker")).unwrap_or_else(|_| panic!("select 1"));
        assert_eq!(held.piece_bboxes_json, "s34");
        assert_eq!(views.active_kind(), Some(&ViewKind::Size("34".into())));

        // An Adjust edit on size 34, then back to Marker.
        let held = views.select(0, state("s34 adjusted")).unwrap_or_else(|_| panic!("select 0"));
        assert_eq!(held.piece_bboxes_json, "marker");

        let held = views.select(1, held).unwrap_or_else(|_| panic!("select 1 again"));
        assert_eq!(held.piece_bboxes_json, "s34 adjusted");
    } // select_round_trip_keeps_each_layout

    // @brief An out-of-range index hands the controller's layout back and changes nothing.
    #[test]
    fn select_out_of_range_fails() {
        let mut views = LayoutViews::default();
        views.start(ViewKind::Marker);
        let back = views.select(3, state("marker")).err().expect("index 3 must fail");
        assert_eq!(back.piece_bboxes_json, "marker");
        assert_eq!(views.active(), 0);

        // Selecting the active view is a no-op that returns the same layout.
        let same = views.select(0, state("marker")).unwrap_or_else(|_| panic!("select 0"));
        assert_eq!(same.piece_bboxes_json, "marker");
    } // select_out_of_range_fails
} // mod tests
