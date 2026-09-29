// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file layout_views.rs
// @brief The layouts a multisize import shows as tabs: "All sizes", then one per size.
//
// The controller keeps the active layout in its own fields, because the canvas,
// Adjust Mode and every export read those fields.  `LayoutViews` holds the
// inactive layouts.  Selecting a tab stores the controller's layout into the
// old view and hands back the new view's layout, so the rest of the app always
// works on the selected tab without knowing that tabs exist.

use svg_dom::Document;

use crate::layout_utils::ProcessLayoutResult;

// @brief Label of the first tab: the layout of the full handoff.
pub const ALL_SIZES_LABEL: &str = "All sizes";

// @brief Label of the tab for one size.
pub fn size_label(size: &str) -> String {
    format!("Size {size}")
} // fn size_label

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
    // `data-size` this view lays out; `None` for "All sizes".
    size: Option<String>,
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

    // @brief Start a multisize layout: one active "All sizes" view, whose layout the controller holds.
    pub fn start_all_sizes(&mut self) {
        self.clear();
        self.views.push(LayoutView {
            label: ALL_SIZES_LABEL.to_string(),
            size: None,
            state: LayoutState::default(),
        });
    } // fn start_all_sizes

    // @brief Add the finished layout of one size as an inactive view.
    pub fn push_size(&mut self, size: &str, state: LayoutState) {
        self.views.push(LayoutView {
            label: size_label(size),
            size: Some(size.to_string()),
            state,
        });
    } // fn push_size

    // @brief Tab labels in display order.
    pub fn labels(&self) -> Vec<String> {
        self.views.iter().map(|v| v.label.clone()).collect()
    } // fn labels

    // @brief Index of the active view.
    pub fn active(&self) -> usize {
        self.active
    } // fn active

    // @brief `data-size` of the active view; `None` for "All sizes" or no views.
    pub fn active_size(&self) -> Option<&str> {
        self.views.get(self.active).and_then(|v| v.size.as_deref())
    } // fn active_size

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

// @brief Sizes still to lay out after the "All sizes" layout, one per UI tick.
//
// Each size packs in its own call so the event loop can repaint the progress
// popup, which names the size, between sizes.
pub struct SizeLayoutQueue {
    // Settings of the "All sizes" run; every size uses the same settings.
    pub settings_json: String,
    // Canvas height before the "All sizes" run trimmed it.
    pub layout_h_px: u32,
    // Sizes not yet laid out, in handoff order.
    pending: std::collections::VecDeque<String>,
    // Number of sizes at the start.
    total: usize,
    // One line per layout with unplaced pieces or a failure: "Size 36: Front, Back".
    pub problems: Vec<String>,
}

impl SizeLayoutQueue {
    // @brief Queue every size of a multisize import.
    pub fn new(settings_json: String, layout_h_px: u32, sizes: Vec<String>) -> Self {
        let total = sizes.len();
        Self { settings_json, layout_h_px, pending: sizes.into(), total, problems: Vec::new() }
    } // fn new

    // @brief Take the next size to lay out.
    pub fn pop(&mut self) -> Option<String> {
        self.pending.pop_front()
    } // fn pop

    // @brief Popup text for the next size: "Laying out size 36 (2 of 5)…"; `None` when empty.
    pub fn next_message(&self) -> Option<String> {
        let size = self.pending.front()?;
        let number = self.total - self.pending.len() + 1;
        Some(format!("Laying out size {size} ({number} of {})…", self.total))
    } // fn next_message
} // impl SizeLayoutQueue

#[cfg(test)]
mod tests {
    use super::*;

    // @brief The popup names each size in order and counts up to the total.
    #[test]
    fn size_queue_messages_count_up() {
        let mut queue = SizeLayoutQueue::new(String::new(), 0, vec!["34".into(), "36".into()]);
        assert_eq!(queue.next_message().as_deref(), Some("Laying out size 34 (1 of 2)…"));
        assert_eq!(queue.pop().as_deref(), Some("34"));
        assert_eq!(queue.next_message().as_deref(), Some("Laying out size 36 (2 of 2)…"));
        assert_eq!(queue.pop().as_deref(), Some("36"));
        assert_eq!(queue.pop(), None);
        assert_eq!(queue.next_message(), None);
    } // size_queue_messages_count_up

    // @brief A state whose bbox JSON names it, so a test can tell states apart.
    fn state(name: &str) -> LayoutState {
        LayoutState { piece_bboxes_json: name.to_string(), ..LayoutState::default() }
    } // fn state

    // @brief Views keep their order and labels; "All sizes" starts active.
    #[test]
    fn labels_and_active_size() {
        let mut views = LayoutViews::default();
        assert!(views.labels().is_empty());
        views.start_all_sizes();
        views.push_size("34", state("s34"));
        views.push_size("36", state("s36"));
        assert_eq!(views.labels(), vec!["All sizes", "Size 34", "Size 36"]);
        assert_eq!(views.active(), 0);
        assert_eq!(views.active_size(), None);
    } // labels_and_active_size

    // @brief A change made on a tab survives a switch away and back.
    #[test]
    fn select_round_trip_keeps_each_layout() {
        let mut views = LayoutViews::default();
        views.start_all_sizes();
        views.push_size("34", state("s34"));

        // The controller holds "all"; switching gives it size 34.
        let held = views.select(1, state("all")).unwrap_or_else(|_| panic!("select 1"));
        assert_eq!(held.piece_bboxes_json, "s34");
        assert_eq!(views.active_size(), Some("34"));

        // An Adjust edit on size 34, then back to "All sizes".
        let held = views.select(0, state("s34 adjusted")).unwrap_or_else(|_| panic!("select 0"));
        assert_eq!(held.piece_bboxes_json, "all");

        let held = views.select(1, held).unwrap_or_else(|_| panic!("select 1 again"));
        assert_eq!(held.piece_bboxes_json, "s34 adjusted");
    } // select_round_trip_keeps_each_layout

    // @brief An out-of-range index hands the controller's layout back and changes nothing.
    #[test]
    fn select_out_of_range_fails() {
        let mut views = LayoutViews::default();
        views.start_all_sizes();
        let back = views.select(3, state("all")).err().expect("index 3 must fail");
        assert_eq!(back.piece_bboxes_json, "all");
        assert_eq!(views.active(), 0);

        // Selecting the active view is a no-op that returns the same layout.
        let same = views.select(0, state("all")).unwrap_or_else(|_| panic!("select 0"));
        assert_eq!(same.piece_bboxes_json, "all");
    } // select_out_of_range_fails
} // mod tests
