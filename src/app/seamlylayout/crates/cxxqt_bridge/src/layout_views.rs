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
//
// An import with several materials lays out every material.  The controller
// holds the shown material's layouts; `MaterialLayout` holds each other one.

use std::collections::VecDeque;

use svg_dom::Document;

use crate::layout_utils::ProcessLayoutResult;
use crate::piece_divider::DivideSpec;

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

// @brief Advice that ends every "pieces left out" warning.
const UNPLACED_ADVICE: &str =
    "The remaining pieces were laid out. Try a larger sheet size, reduce the piece gap or margins, or remove pieces.";

// @brief Warning for a single layout that left pieces out.
// @param lines One line per piece, from `ProcessLayoutResult::unplaced_labels`.
pub fn unplaced_warning(lines: &[String]) -> String {
    format!(
        "{} piece(s) could not be placed and were left out of the layout:\n\n{}\n\n{UNPLACED_ADVICE}",
        lines.len(),
        bullet_lines(lines)
    )
} // fn unplaced_warning

// @brief Warning for pieces whose fold line stays off the left edge: one bullet per piece.
pub fn fold_line_warning(lines: &[String]) -> String {
    format!(
        "{} piece(s) with a fold line are not on the left edge (the fold):\n\n{}",
        lines.len(),
        bullet_lines(lines)
    )
} // fn fold_line_warning

// @brief One tab's entry in the warning after the last tab: the tab label, then one bullet per piece.
pub fn tab_unplaced_problem(label: &str, lines: &[String]) -> String {
    format!("{label}:\n{}", bullet_lines(lines))
} // fn tab_unplaced_problem

// @brief Warning after the last tab: every tab that left pieces out or failed.
pub fn tab_problems_warning(problems: &[String]) -> String {
    format!("Some layouts left pieces out:\n\n{}\n\n{UNPLACED_ADVICE}", problems.join("\n"))
} // fn tab_problems_warning

// @brief One "• line" per line.
fn bullet_lines(lines: &[String]) -> String {
    lines.iter().map(|line| format!("\u{2022} {line}")).collect::<Vec<_>>().join("\n")
} // fn bullet_lines

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

    // @brief True when PDF (Tiled) can export the tab.
    //
    // Marker holds every size of every piece; tiling it locks SeamlyLayout up.
    pub fn exports_pdf_tiled(&self) -> bool {
        *self != ViewKind::Marker
    } // fn exports_pdf_tiled

    // @brief True when Export all tabs writes this tab in `format`.
    //
    // @param format Export format name from QML: "dxf", "pdf-tiled", "svg", ...
    // DXF-ASTM writes every tab it can export.  Other formats skip the Base Size
    // tab: its layout repeats the size tab of the same size.  PDF (Tiled) also
    // skips Marker.
    pub fn in_all_tabs_export(&self, format: &str) -> bool {
        let repeats_size_tab = matches!(self, ViewKind::BaseSize(_));
        match format {
            "dxf"       => self.exports_dxf(),
            "pdf-tiled" => self.exports_pdf_tiled() && !repeats_size_tab,
            _           => !repeats_size_tab,
        } // match format
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
    // @brief The state of a blank canvas, before a layout: the canvas itself and its height.
    pub fn blank(initial_dom: &Document) -> Self {
        Self {
            layout_dom: Some(initial_dom.clone()),
            layout_h_px: canvas_height_px(initial_dom),
            ..Self::default()
        }
    } // fn blank

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

    // @brief Label of the active view; `None` when there are no views (individual import).
    pub fn active_label(&self) -> Option<&str> {
        self.label(self.active)
    } // fn active_label

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

// @brief Height of a canvas from its root `height`, in px; 0 when it has none.
pub fn canvas_height_px(doc: &Document) -> u32 {
    doc.root.attributes.get("height").and_then(|h| h.parse::<u32>().ok()).unwrap_or(0)
} // fn canvas_height_px

// @brief The layouts of a material that is not shown.
//
// Selecting a material parks the controller's layouts here and takes the new
// material's layouts out, as `LayoutViews::select` does for tabs.
#[derive(Default)]
pub struct MaterialLayout {
    // Blank canvas from the submitted settings; `None` before settings are submitted.
    pub initial_layout_dom: Option<Document>,
    // Layout of the material's active tab, or its blank canvas before a layout.
    pub state: LayoutState,
    // Tabs of a multisize layout.
    pub views: LayoutViews,
    // The media the layout split its pieces for; `None` when it did not split.
    pub divide_spec: Option<DivideSpec>,
    // True when the material's first layout finished, so exports and Adjust Mode can use it.
    pub is_laid_out: bool,
}

impl MaterialLayout {
    // @brief Layouts of a material whose settings were just submitted: only its blank canvas.
    pub fn blank(initial_dom: Document) -> Self {
        Self { state: LayoutState::blank(&initial_dom), initial_layout_dom: Some(initial_dom), ..Self::default() }
    } // fn blank

    // @brief Drop the layouts, keep the blank canvas: a new layout run starts.
    pub fn reset_to_blank(&mut self) {
        self.state = self.initial_layout_dom.as_ref().map(LayoutState::blank).unwrap_or_default();
        self.views.clear();
        self.divide_spec = None;
        self.is_laid_out = false;
    } // fn reset_to_blank
} // impl MaterialLayout

// @brief One layout of a layout run: a material's first layout, or one of its tabs.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LayoutJob {
    // Index of the material in the import; 0 for an import with one material.
    pub material: usize,
    // Tab to lay out; `None` for the material's first layout: the only layout of an
    // individual pattern, or the Marker tab of a multisize one.
    pub kind: Option<ViewKind>,
}

// @brief Jobs that follow the first layout of the shown material.
//
// @param active    Material whose first layout the run makes before the queue starts.
// @param tab_kinds Tabs after the first layout, one list per material; empty lists for an individual pattern.
// @return The shown material's tabs, then the first layout and tabs of each other material, in material order.
pub fn layout_jobs(active: usize, tab_kinds: &[Vec<ViewKind>]) -> Vec<LayoutJob> {
    let tabs = |material: usize| -> Vec<LayoutJob> {
        tab_kinds[material].iter().map(|kind| LayoutJob { material, kind: Some(kind.clone()) }).collect()
    };
    let mut jobs = if active < tab_kinds.len() { tabs(active) } else { Vec::new() };
    for material in (0..tab_kinds.len()).filter(|&m| m != active) {
        jobs.push(LayoutJob { material, kind: None });
        jobs.extend(tabs(material));
    } // for each other material
    jobs
} // fn layout_jobs

// @brief Layouts still to make after the first one, one per UI tick.
//
// Each layout packs in its own call so the event loop can repaint the progress
// popup, which names the layout, between layouts.
pub struct TabLayoutQueue {
    // Settings of the first run; every layout uses the same settings.
    pub settings_json: String,
    // Canvas height of the shown material before its first run trimmed it.
    pub layout_h_px: u32,
    // First layouts of the other materials extend the media, as the shown material's did.
    pub extend_media: bool,
    // First layouts of the other materials split oversized pieces, as the shown material's did.
    pub split_oversized: bool,
    // Layouts not yet made, in order.
    pending: VecDeque<LayoutJob>,
    // Number of layouts in the run, the first one included.
    total: usize,
    // Material names that start each layout label; empty when the import has one material.
    material_names: Vec<String>,
    // Label of a first layout: "Marker" for a multisize import, empty for an individual one.
    first_label: String,
    // One line per layout with unplaced pieces or a failure: "Size 36: Front, Back".
    pub problems: Vec<String>,
    // One line per piece whose fold line stays off the left edge, in individual layouts.
    pub fold_line_lines: Vec<String>,
    // Base Size layout, its material and its size: the size tab of that size shows the same layout.
    base_layout: Option<(usize, String, ProcessLayoutResult)>,
}

impl TabLayoutQueue {
    // @brief Queue the layouts that follow the first one.
    //
    // @param material_names One name per material; pass an empty list for an import with one material.
    // @param multisize      True when each first layout is a Marker tab.
    pub fn new(settings_json: String, layout_h_px: u32, jobs: Vec<LayoutJob>, material_names: Vec<String>, multisize: bool) -> Self {
        let total = jobs.len() + 1;
        Self {
            settings_json,
            layout_h_px,
            extend_media: multisize,
            split_oversized: false,
            pending: jobs.into(),
            total,
            material_names,
            first_label: if multisize { MARKER_LABEL.to_string() } else { String::new() },
            problems: Vec::new(),
            fold_line_lines: Vec::new(),
            base_layout: None,
        }
    } // fn new

    // @brief Take the next layout to make.
    pub fn pop(&mut self) -> Option<LayoutJob> {
        self.pending.pop_front()
    } // fn pop

    // @brief Name of a layout for the popup and the warnings: "Lining: Size 36", "Size 36" or "Lining".
    pub fn job_label(&self, job: &LayoutJob) -> String {
        let tab = match &job.kind {
            Some(kind) => kind.label(),
            None => self.first_label.clone(),
        }; // tab
        let material = self.material_names.get(job.material).map(String::as_str).unwrap_or("");
        match (material.is_empty(), tab.is_empty()) {
            (true, true)   => "Layout".to_string(),
            (true, false)  => tab,
            (false, true)  => material.to_string(),
            (false, false) => format!("{material}: {tab}"),
        } // match parts
    } // fn job_label

    // @brief "Lining: " before a line about material `material`; empty for an import with one material.
    pub fn material_prefix(&self, material: usize) -> String {
        match self.material_names.get(material) {
            Some(name) if !name.is_empty() => format!("{name}: "),
            _ => String::new(),
        } // match name
    } // fn material_prefix

    // @brief Popup text for the next layout: "Laying out Size 36 (4 of 6)…"; `None` when empty.
    pub fn next_message(&self) -> Option<String> {
        let job = self.pending.front()?;
        let number = self.total - self.pending.len() + 1;
        Some(format!("Laying out {} ({number} of {})…", self.job_label(job), self.total))
    } // fn next_message

    // @brief Keep a finished Base Size layout for the size tab of the same size and material.
    pub fn keep_base_layout(&mut self, job: &LayoutJob, result: &ProcessLayoutResult) {
        if let Some(ViewKind::BaseSize(size)) = &job.kind {
            self.base_layout = Some((job.material, size.clone(), result.clone()));
        } // if Base Size
    } // fn keep_base_layout

    // @brief The Base Size layout when `job` is the size tab of the base size, in the same material.
    pub fn reusable_layout(&self, job: &LayoutJob) -> Option<ProcessLayoutResult> {
        match (&job.kind, &self.base_layout) {
            (Some(ViewKind::Size(size)), Some((material, base, result))) if size == base && *material == job.material => {
                Some(result.clone())
            } // same size, same material
            _ => None,
        } // match job
    } // fn reusable_layout

    // @brief Warning after the last layout: layouts that left pieces out or failed, then fold lines off the edge.
    //
    // @return `None` when every layout placed every piece.
    pub fn final_warning(&self) -> Option<String> {
        let mut parts = Vec::new();
        if !self.problems.is_empty() {
            parts.push(tab_problems_warning(&self.problems));
        } // if problems
        if !self.fold_line_lines.is_empty() {
            parts.push(fold_line_warning(&self.fold_line_lines));
        } // if fold lines off the edge
        if parts.is_empty() { None } else { Some(parts.join("\n\n")) }
    } // fn final_warning
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
    //        PDF (Tiled) skips Marker.
    #[test]
    fn view_kinds_choose_their_exports() {
        let (marker, nested) = (ViewKind::Marker, ViewKind::Nested);
        let (base, size) = (ViewKind::BaseSize("36".into()), ViewKind::Size("34".into()));
        assert!(!marker.exports_dxf() && !nested.exports_dxf());
        assert!(base.exports_dxf() && size.exports_dxf());
        assert!(!marker.exports_pdf_tiled());
        assert!(nested.exports_pdf_tiled() && base.exports_pdf_tiled() && size.exports_pdf_tiled());

        let in_dxf: Vec<bool> = [&marker, &nested, &base, &size].iter().map(|k| k.in_all_tabs_export("dxf")).collect();
        assert_eq!(in_dxf, vec![false, false, true, true]);
        let in_pdf: Vec<bool> = [&marker, &nested, &base, &size].iter().map(|k| k.in_all_tabs_export("pdf")).collect();
        assert_eq!(in_pdf, vec![true, true, false, true]);
        let in_tiled: Vec<bool> = [&marker, &nested, &base, &size].iter().map(|k| k.in_all_tabs_export("pdf-tiled")).collect();
        assert_eq!(in_tiled, vec![false, true, false, true]);

        assert!(nested.nested() && !marker.nested() && !base.nested());
        assert_eq!((marker.size(), base.size(), size.size()), (None, Some("36"), Some("34")));
    } // view_kinds_choose_their_exports

    // @brief A tab job of material `material`.
    fn tab(material: usize, kind: ViewKind) -> LayoutJob {
        LayoutJob { material, kind: Some(kind) }
    } // fn tab

    // @brief The first-layout job of material `material`.
    fn first(material: usize) -> LayoutJob {
        LayoutJob { material, kind: None }
    } // fn first

    // @brief The popup names each tab in order and counts the first tab as done.
    #[test]
    fn tab_queue_messages_count_up() {
        let jobs = layout_jobs(0, &[vec![ViewKind::Nested, ViewKind::Size("34".into())]]);
        let mut queue = TabLayoutQueue::new(String::new(), 0, jobs, Vec::new(), true);
        assert_eq!(queue.next_message().as_deref(), Some("Laying out Nested (2 of 3)…"));
        assert_eq!(queue.pop(), Some(tab(0, ViewKind::Nested)));
        assert_eq!(queue.next_message().as_deref(), Some("Laying out Size 34 (3 of 3)…"));
        assert_eq!(queue.pop(), Some(tab(0, ViewKind::Size("34".into()))));
        assert_eq!(queue.pop(), None);
        assert_eq!(queue.next_message(), None);
    } // tab_queue_messages_count_up

    // @brief The shown material's tabs come first; each other material follows with its first layout, then its tabs.
    #[test]
    fn layout_jobs_cover_every_material() {
        let kinds = vec![ViewKind::Nested, ViewKind::Size("34".into())];
        let jobs = layout_jobs(1, &[kinds.clone(), kinds.clone(), kinds]);
        assert_eq!(jobs, vec![
            tab(1, ViewKind::Nested), tab(1, ViewKind::Size("34".into())),
            first(0), tab(0, ViewKind::Nested), tab(0, ViewKind::Size("34".into())),
            first(2), tab(2, ViewKind::Nested), tab(2, ViewKind::Size("34".into())),
        ]);

        // Individual patterns have no tabs: only the other materials' layouts remain.
        assert_eq!(layout_jobs(0, &[Vec::new(), Vec::new()]), vec![first(1)]);
        assert!(layout_jobs(0, &[Vec::new()]).is_empty(), "one individual material needs no queue");
    } // layout_jobs_cover_every_material

    // @brief With several materials each label starts with the material; a first layout is Marker or the material.
    #[test]
    fn job_labels_name_the_material() {
        let names = vec!["Fabric".to_string(), "Lining".to_string()];
        let multisize = TabLayoutQueue::new(String::new(), 0, vec![first(1)], names.clone(), true);
        assert_eq!(multisize.job_label(&first(1)), "Lining: Marker");
        assert_eq!(multisize.job_label(&tab(0, ViewKind::Size("34".into()))), "Fabric: Size 34");
        assert_eq!(multisize.next_message().as_deref(), Some("Laying out Lining: Marker (2 of 2)…"));

        let individual = TabLayoutQueue::new(String::new(), 0, vec![first(1)], names, false);
        assert_eq!(individual.job_label(&first(1)), "Lining");

        let one = TabLayoutQueue::new(String::new(), 0, Vec::new(), Vec::new(), true);
        assert_eq!(one.job_label(&first(0)), "Marker");
        assert_eq!(one.job_label(&tab(0, ViewKind::Nested)), "Nested");
    } // job_labels_name_the_material

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
            fold_line_warnings: Vec::new(),
            flat_dom: doc.clone(),
            vertical_dom: doc.clone(),
            translate_dom: doc,
            divide_spec: None,
        }
    } // fn result

    // @brief The size tab of the base size reuses the Base Size layout of its own material; other tabs do not.
    #[test]
    fn base_size_layout_is_reused_once_kept() {
        let mut queue = TabLayoutQueue::new(String::new(), 0, Vec::new(), Vec::new(), true);
        let base = tab(0, ViewKind::BaseSize("36".into()));
        assert!(queue.reusable_layout(&tab(0, ViewKind::Size("36".into()))).is_none(), "nothing kept yet");

        queue.keep_base_layout(&tab(0, ViewKind::Size("34".into())), &result("s34"));
        assert!(queue.reusable_layout(&tab(0, ViewKind::Size("34".into()))).is_none(), "only a Base Size layout is kept");

        queue.keep_base_layout(&base, &result("base"));
        let reused = queue.reusable_layout(&tab(0, ViewKind::Size("36".into()))).expect("size 36 reuses Base Size 36");
        assert_eq!((reused.bbox_json.as_str(), reused.unplaced_labels.len()), ("base", 1));
        assert!(queue.reusable_layout(&tab(0, ViewKind::Size("34".into()))).is_none());
        assert!(queue.reusable_layout(&tab(0, ViewKind::Nested)).is_none());
        assert!(queue.reusable_layout(&tab(1, ViewKind::Size("36".into()))).is_none(), "another material lays out its own");
    } // base_size_layout_is_reused_once_kept

    // @brief The final warning lists left-out pieces, then fold lines off the edge; nothing to report gives none.
    #[test]
    fn final_warning_joins_problems_and_fold_lines() {
        let mut queue = TabLayoutQueue::new(String::new(), 0, Vec::new(), Vec::new(), false);
        assert_eq!(queue.final_warning(), None);
        queue.problems.push(tab_unplaced_problem("Lining", &sizes(&["Back"])));
        queue.fold_line_lines.push("Lining: Back: the fold line crosses the grain".to_string());
        let text = queue.final_warning().expect("two problems");
        assert!(text.starts_with("Some layouts left pieces out:\n\nLining:\n\u{2022} Back"), "{text}");
        assert!(text.ends_with("(the fold):\n\n\u{2022} Lining: Back: the fold line crosses the grain"), "{text}");
    } // final_warning_joins_problems_and_fold_lines

    // @brief A material's layouts start as its blank canvas, and a new run drops them back to it.
    #[test]
    fn material_layout_resets_to_its_blank_canvas() {
        let canvas = Document::parse("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"240\"/>")
            .expect("canvas parses");
        let mut layout = MaterialLayout::blank(canvas);
        assert_eq!(layout.state.layout_h_px, 240);
        assert!(layout.state.layout_dom.is_some() && !layout.is_laid_out);

        layout.state = state("laid out");
        layout.views.start(ViewKind::Marker);
        layout.is_laid_out = true;
        layout.reset_to_blank();
        assert_eq!((layout.state.layout_h_px, layout.state.piece_bboxes_json.as_str()), (240, ""));
        assert!(layout.views.labels().is_empty() && !layout.is_laid_out);

        let mut never = MaterialLayout::default();
        never.reset_to_blank();
        assert!(never.state.layout_dom.is_none(), "no settings: no canvas");
    } // material_layout_resets_to_its_blank_canvas

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
        assert_eq!(views.active_label(), None);
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
        assert_eq!(views.active_label(), Some("Marker"));
        views.select(3, state("marker")).unwrap_or_else(|_| panic!("select 3"));
        assert_eq!(views.active_label(), Some("Size 34"));
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

    // @brief Left-out pieces read one bullet per piece, under their tab after a tab run.
    #[test]
    fn fold_line_warning_lists_one_piece_per_line() {
        let text = fold_line_warning(&sizes(&["Back: the outline lies on both sides of the fold line"]));
        assert_eq!(
            text,
            "1 piece(s) with a fold line are not on the left edge (the fold):\n\n\u{2022} Back: the outline lies on both sides of the fold line"
        );
    } // fold_line_warning_lists_one_piece_per_line

    #[test]
    fn unplaced_warnings_list_one_piece_per_line() {
        let lines = sizes(&["Detail (3): 220, 240", "Front: 220"]);
        let single = unplaced_warning(&lines);
        assert!(single.starts_with("2 piece(s) could not be placed"), "{single}");
        assert!(single.contains("\n\n\u{2022} Detail (3): 220, 240\n\u{2022} Front: 220\n\n"), "{single}");

        let problems = vec![tab_unplaced_problem(MARKER_LABEL, &lines), "Size 220: not laid out (no room)".to_string()];
        let tabs = tab_problems_warning(&problems);
        assert!(
            tabs.contains("Marker:\n\u{2022} Detail (3): 220, 240\n\u{2022} Front: 220\nSize 220: not laid out (no room)"),
            "{tabs}"
        );
    } // unplaced_warnings_list_one_piece_per_line
} // mod tests
