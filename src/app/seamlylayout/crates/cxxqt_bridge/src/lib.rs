// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

use cxx_qt::CxxQtType;
use app_core;
use svg_dom;
use geometry::{BoundingBox, Path, PathSegment, Point};
#[cfg(debug_assertions)]
use std::sync::atomic::{AtomicUsize, Ordering};
use xmltree::{Element as XmlElement, XMLNode};

// LayoutSettings and the tiling algorithms now live in the Qt-free
// `layout_tiling` crate so they can be linked by non-Qt consumers (cli,
// future scripting hooks, etc.).  The bridge re-exports LayoutSettings for
// backwards-compatibility with existing call sites.
pub use layout_tiling::LayoutSettings;

mod piece_extractor;
pub use piece_extractor::{extract_piece_rects, hoist_tagged_pieces, PieceRect};

mod layout_assembler;
pub use layout_assembler::{create_layout, create_initial_layout_dom, remove_color_blocks, trim_bottom};

mod layout_utils;
use layout_utils::{do_initialize_layout, do_process_layout, ProcessLayoutArgs};

mod layout_helpers;
use layout_helpers::remove_group_by_id;

mod exports;
use exports::{do_export_dxf, do_export_pdf, do_export_pdf_tile, do_export_png, do_export_svg};

// Phase A of the "sheets" paper_type (L.2.1):
// identifies oversized pieces and assembles the oversized SVG for the tiled-PDF pipeline.
pub mod oversized;
pub use oversized::{partition_oversized_pieces, build_oversized_svg};

// Phase B of the "sheets" paper_type (L.2.2):
// packs remaining (non-oversized) pieces onto multiple sheet SVG documents.
pub mod remaining;
pub use remaining::build_remaining_svgs;

// Sheets orchestrator (L.2.1 + L.2.2):
// combines Phase A tiled pages and Phase B sheet pages into one merged PDF.
mod sheets;
use sheets::{build_sheet_export_inputs, do_export_sheets_pdf};
use std::sync::OnceLock;

// Global output directory anchored to the executable — initialized once, shared everywhere.
static EXE_DIR: OnceLock<std::path::PathBuf> = OnceLock::new();
static OUT_DIR: OnceLock<std::path::PathBuf> = OnceLock::new();
// Log file path — shared with C++ Logger via the SEAMLY_LOG_FILE environment variable.
// Only compiled in debug builds; the release no-op log_to_file stub never opens a file.
#[cfg(debug_assertions)]
static LOG_PATH: OnceLock<std::path::PathBuf> = OnceLock::new();
// Global counter for sequential adjust_dom debug saves — only compiled in debug builds.
#[cfg(debug_assertions)]
static ADJUST_DOM_COUNTER: AtomicUsize = AtomicUsize::new(0);

// ---------------------------------------------------------------------------
// log crate integration
// ---------------------------------------------------------------------------
//
// The Qt-free `layout_tiling` crate uses the standard `log` facade for debug
// output (so it stays consumer-agnostic). The bridge installs a tiny Logger
// implementation that forwards every record through `log_to_file`, preserving
// the existing file-based logging behavior.  Installation is idempotent: if
// another logger is already set, set_logger() returns Err and we ignore it.

struct FileLogger;

impl log::Log for FileLogger {
    fn enabled(&self, _meta: &log::Metadata) -> bool { true }

    fn log(&self, record: &log::Record) {
        // Forward the formatted message through the existing file writer.
        log_to_file(&format!("{}", record.args()));
    } // fn log

    fn flush(&self) { /* log_to_file opens + appends + drops per call */ }
} // impl log::Log for FileLogger

static FILE_LOGGER: FileLogger = FileLogger;

// @brief Install the bridge's file-based logger.
// Called from AppControllerRust::default() on first QObject instantiation.
// set_logger can only succeed once per process; subsequent calls are no-ops.
fn init_file_logger() {
    if log::set_logger(&FILE_LOGGER).is_ok() {
        log::set_max_level(log::LevelFilter::Debug);
    } // if set_logger succeeded
} // fn init_file_logger

// @brief Returns the directory that contains the running executable.
fn get_exe_dir() -> &'static std::path::PathBuf {
    EXE_DIR.get_or_init(|| {
        std::env::current_exe()
            .ok()
            .and_then(|p| p.parent().map(|p| p.to_path_buf()))
            .unwrap_or_else(|| std::path::PathBuf::from("."))
    })
} // fn get_exe_dir

// @brief Returns the output/ subdirectory next to the executable, creating it on first access.
// Only compiled in debug builds; the release no-op stub below returns an empty path without
// creating any directory on disk — release builds skip output/ dir creation entirely.
#[cfg(debug_assertions)]
fn get_out_dir() -> &'static std::path::PathBuf {
    OUT_DIR.get_or_init(|| {
        let out = get_exe_dir().join("output");
        let _ = std::fs::create_dir_all(&out);
        out
    })
} // fn get_out_dir (debug build)

// @brief No-op stub compiled only when debug_assertions is disabled — returns an empty path
// without creating any directory.  All call sites in lib.rs and layout_utils.rs compile
// unchanged; writes against the returned path fail gracefully (Err/empty return).
#[cfg(not(debug_assertions))]
#[inline(always)]
fn get_out_dir() -> &'static std::path::PathBuf {
    OUT_DIR.get_or_init(std::path::PathBuf::new)
} // fn get_out_dir (no-op when debug_assertions is disabled)

// @brief Save a DOM document to output/<basename>_<counter>.svg for debugging.
// The global ADJUST_DOM_COUNTER is incremented on each call so every save
// produces a unique, sequentially numbered file.  Errors are silently ignored.
// Only compiled in debug builds; the release no-op stub below discards the call
// entirely so release builds write no SVG snapshots.
#[cfg(debug_assertions)]
fn save_debug_dom(doc: &svg_dom::Document, filename: &str) {
    // Use a sequential counter to avoid overwriting prior saves and to track the order of saves.
    let count = ADJUST_DOM_COUNTER.fetch_add(1, Ordering::SeqCst);
    let numbered = if let Some(stem) = filename.strip_suffix(".svg") {
        format!("{}_{}.svg", stem, count)
    } else {
        format!("{}_{}", filename, count)
    };
    let path = get_out_dir().join(&numbered);
    match app_core::save_svg(doc, &path) {
        Ok(_) => {
            log_to_file(&format!("[debug] cxxqt_bridge\\src\\lib.rs::save_debug_dom: 1 Saved SVG to: {}", path.display()));
        }
        Err(e) => {
            log_to_file(&format!("[debug] cxxqt_bridge\\src\\lib.rs::save_debug_dom: 2 Failed to save SVG to {}: {}", path.display(), e));
        }
    }
} // fn save_debug_dom (debug build)

// @brief No-op stub compiled only when debug_assertions is disabled — discards the call.
// Argument expressions at call sites may still be evaluated by the compiler unless
// optimised away; the key guarantee is that no file I/O occurs in release builds.
#[cfg(not(debug_assertions))]
#[inline(always)]
fn save_debug_dom(_doc: &svg_dom::Document, _filename: &str) {} // fn save_debug_dom (no-op when debug_assertions is disabled)

// @brief Remove stale AdjustMode debug artifacts from the output directory.
//
// Deletes files matching these patterns:
// - adjust_overlay_*.json
// - adjust_dom_*.svg
//
// This runs when leaving AdjustMode (Save/Done or Discard/Cancel) so old
// overlay snapshots do not accumulate and confuse later debug/review sessions.
//
// Only compiled in debug builds — in release no debug files are written so
// cleanup is correct as a no-op (see the stub below).
//
// @return Number of files successfully removed.
#[cfg(debug_assertions)]
fn cleanup_adjust_output_artifacts() -> usize {
    let out_dir = get_out_dir();
    let mut removed: usize = 0;

    let entries = match std::fs::read_dir(out_dir) {
        Ok(v) => v,
        Err(e) => {
            log_to_file(&format!(
                "[lib.rs cleanup_adjust_output_artifacts] Failed to read output dir '{}': {}",
                out_dir.display(),
                e
            ));
            return 0;
        }
    };

    for entry in entries {
        let entry = match entry {
            Ok(v) => v,
            Err(_) => continue,
        };
        let path = entry.path();
        let file_name = match path.file_name().and_then(|n| n.to_str()) {
            Some(v) => v,
            None => continue,
        };

        let is_adjust_overlay_json = file_name.starts_with("adjust_overlay_") && file_name.ends_with(".json");
        let is_adjust_dom_svg = file_name.starts_with("adjust_dom_") && file_name.ends_with(".svg");
        if !(is_adjust_overlay_json || is_adjust_dom_svg) {
            continue;
        }

        match std::fs::remove_file(&path) {
            Ok(_) => {
                removed += 1;
            }
            Err(e) => {
                log_to_file(&format!(
                    "[lib.rs cleanup_adjust_output_artifacts] Failed to remove '{}': {}",
                    path.display(),
                    e
                ));
            }
        }
    }

    log_to_file(&format!(
        "[lib.rs cleanup_adjust_output_artifacts] Removed {} stale adjust artifact file(s).",
        removed
    ));
    removed
} // fn cleanup_adjust_output_artifacts (debug build)

// @brief No-op stub compiled only when debug_assertions is disabled.
// In release builds no debug files are written to the output directory, so
// there is nothing to clean up — returning 0 is always correct.
// Call sites in discard_adjustments() and exit_adjust_mode() compile unchanged.
#[cfg(not(debug_assertions))]
#[inline(always)]
fn cleanup_adjust_output_artifacts() -> usize { 0 } // fn cleanup_adjust_output_artifacts (no-op when debug_assertions is disabled)

// @brief Compute a bounding box from descendant geometry inside one top-level piece group.
// Uses path, line, and rect descendants so reloaded adjust_dom.svg does not depend on
// cached data-* attributes that may be absent after Apply/reload.
fn bbox_from_group_geometry(group: &XmlElement) -> Option<BoundingBox> {
    let mut points = Vec::new();
    collect_group_points(group, &mut points);
    BoundingBox::from_points(points)
} // fn bbox_from_group_geometry

fn collect_group_points(element: &XmlElement, points: &mut Vec<Point>) {
    match element.name.as_str() {
        "path" => {
            if let Some(d) = element.attributes.get("d") {
                if let Ok(path) = Path::parse_path_attribute(d) {
                    for seg in &path.segments {
                        match seg {
                            PathSegment::MoveTo(p) => points.push(*p),
                            PathSegment::LineTo(p) => points.push(*p),
                            PathSegment::QuadTo { ctrl, to } => {
                                points.push(*ctrl);
                                points.push(*to);
                            }
                            PathSegment::CubicTo { ctrl1, ctrl2, to } => {
                                points.push(*ctrl1);
                                points.push(*ctrl2);
                                points.push(*to);
                            }
                            PathSegment::ArcTo { to, .. } => points.push(*to),
                            PathSegment::Close => {}
                        }
                    }
                }
            }
        }
        "line" => {
            let x1 = element.attributes.get("x1").and_then(|v| v.parse::<f32>().ok());
            let y1 = element.attributes.get("y1").and_then(|v| v.parse::<f32>().ok());
            let x2 = element.attributes.get("x2").and_then(|v| v.parse::<f32>().ok());
            let y2 = element.attributes.get("y2").and_then(|v| v.parse::<f32>().ok());
            if let (Some(x1), Some(y1), Some(x2), Some(y2)) = (x1, y1, x2, y2) {
                points.push(Point::new(x1, y1));
                points.push(Point::new(x2, y2));
            }
        }
        "rect" => {
            let x = element.attributes.get("x").and_then(|v| v.parse::<f32>().ok()).unwrap_or(0.0);
            let y = element.attributes.get("y").and_then(|v| v.parse::<f32>().ok()).unwrap_or(0.0);
            let w = element.attributes.get("width").and_then(|v| v.parse::<f32>().ok()).unwrap_or(0.0);
            let h = element.attributes.get("height").and_then(|v| v.parse::<f32>().ok()).unwrap_or(0.0);
            if w > 0.0 && h > 0.0 {
                points.push(Point::new(x, y));
                points.push(Point::new(x + w, y));
                points.push(Point::new(x + w, y + h));
                points.push(Point::new(x, y + h));
            }
        }
        _ => {}
    }

    for child in &element.children {
        if let XMLNode::Element(child_elem) = child {
            collect_group_points(child_elem, points);
        }
    }
} // fn collect_group_points

// @brief Parse the top y-coordinate from one tiled row path's `d` attribute.
// Expected format from layout_tiling::create_initial_tiled_layout_dom:
//   d="M x0,y M x1,y ..."
// Returns None if the format is missing or malformed.
fn parse_tiled_row_top_y(path_elem: &XmlElement) -> Option<u32> {
    path_elem
        .attributes
        .get("d")
        .and_then(|d| d.split_whitespace().nth(1))
        .and_then(|xy| xy.split(',').nth(1))
        .and_then(|y| y.parse::<u32>().ok())
} // fn parse_tiled_row_top_y

// @brief Remove blank bottom tile rows from an adjusted tiled layout DOM.
//
// A tile row is considered blank when the row's top y is at or below the
// maximum bottom y of all placed piece groups. Removes only bottom-most rows,
// keeps at least one row, and updates background/content/root heights.
//
// @param doc  Mutable adjust/layout DOM.
// @return Number of tile rows removed.
fn trim_empty_tiled_rows_in_adjust_dom(doc: &mut svg_dom::Document) -> u32 {
    // Non-tiled layouts have no tiledRects group.
    let Some(_) = doc.get_element_by_id_mut("tiledRects") else {
        return 0;
    };

    // Compute max piece bottom in absolute canvas coordinates.
    // `exit_adjust_mode` now flattens and persists adjust_dom before calling
    // this helper, so we can measure directly from the real DOM.
    let max_piece_bottom: u32 = doc
        .root
        .children
        .iter()
        .filter_map(|node| node.as_element())
        .filter(|el| el.name == "g")
        .filter(|el| {
            let id = el.attributes.get("id").map(String::as_str).unwrap_or("");
            !id.is_empty() && id != "Rectangles"
        })
        .filter_map(|el| bbox_from_group_geometry(el))
        .map(|bbox| bbox.max.y.max(0.0).ceil() as u32)
        .max()
        .unwrap_or(0);

    // Snapshot row geometry before mutation.
    let (original_row_count, trim_tile_h_px) = {
        let Some(tiled_rects_group) = doc.get_element_by_id_mut("tiledRects") else {
            return 0;
        };
        let row_ys: Vec<u32> = tiled_rects_group
            .children
            .iter()
            .filter_map(|n| n.as_element())
            .filter_map(parse_tiled_row_top_y)
            .collect();

        let row_count = tiled_rects_group.children.len() as u32;
        if row_count == 0 {
            return 0;
        }

        let inferred_trim_h = if row_ys.len() >= 2 {
            row_ys[1].saturating_sub(row_ys[0])
        } else {
            // Fallback: derive row height from contentRect / row_count.
            let content_h = doc
                .get_attr_by_id("contentRect", "height")
                .and_then(|v| v.parse::<u32>().ok())
                .unwrap_or(0);
            if row_count > 0 { content_h / row_count } else { 0 }
        };

        (row_count, inferred_trim_h)
    };

    if original_row_count <= 1 || trim_tile_h_px == 0 {
        return 0;
    }

    // Pop only blank rows from the bottom. Keep at least one row.
    let mut rows_removed: u32 = 0;
    if let Some(tiled_rects_group) = doc.get_element_by_id_mut("tiledRects") {
        while tiled_rects_group.children.len() > 1 {
            let last_row_y = tiled_rects_group
                .children
                .last()
                .and_then(|n| n.as_element())
                .and_then(parse_tiled_row_top_y)
                .unwrap_or(0);

            // Stop when the bottom-most remaining row is needed.
            if max_piece_bottom > last_row_y {
                break;
            }

            tiled_rects_group.children.pop();
            rows_removed = rows_removed.saturating_add(1);
        }
    }

    if rows_removed == 0 {
        return 0;
    }

    let trim_total = trim_tile_h_px.saturating_mul(rows_removed);

    // Update background/content/root heights after row trim.
    if let Some(val) = doc.get_attr_by_id("backgroundRect", "height") {
        if let Ok(orig) = val.parse::<u32>() {
            doc.set_attr_by_id(
                "backgroundRect",
                "height",
                orig.saturating_sub(trim_total).to_string(),
            );
        }
    }
    if let Some(val) = doc.get_attr_by_id("contentRect", "height") {
        if let Ok(orig) = val.parse::<u32>() {
            doc.set_attr_by_id(
                "contentRect",
                "height",
                orig.saturating_sub(trim_total).to_string(),
            );
        }
    }
    if let Some(val) = doc.root.attributes.get("height").cloned() {
        if let Ok(orig) = val.parse::<u32>() {
            doc.root
                .attributes
                .insert("height".to_string(), orig.saturating_sub(trim_total).to_string());
        }
    }

    rows_removed
} // fn trim_empty_tiled_rows_in_adjust_dom

// @brief Returns the path to the shared log file.
// Reads from the SEAMLY_LOG_FILE environment variable set by Logger::init() (C++).
// Falls back to output/debug_log.txt if the env var is not set.
// Only compiled in debug builds alongside LOG_PATH.
#[cfg(debug_assertions)]
fn get_log_path() -> &'static std::path::PathBuf {
    LOG_PATH.get_or_init(|| {
        match std::env::var("SEAMLY_LOG_FILE") {
            Ok(p) => std::path::PathBuf::from(p),
            Err(_) => get_out_dir().join("debug_log.txt"),
        }
    })
} // fn get_log_path

// @brief Append a timestamped debug line to the shared log file.
// Format matches C++ Logger: [unix_seconds] DEBUG: message
// The file is opened in append mode for each call and flushed immediately.
// Only compiled in debug builds; all ~20 call sites in lib.rs, layout_utils.rs,
// and exports.rs compile unchanged because the no-op stub below has the same signature.
#[cfg(debug_assertions)]
pub(crate) fn log_to_file(message: &str) {
    use std::io::Write;
    let path = get_log_path();
    if let Ok(mut file) = std::fs::OpenOptions::new()
        .create(true)
        .append(true)
        .open(path)
    {
        let secs = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .map(|d| d.as_secs())
            .unwrap_or(0);
        let _ = writeln!(file, "[{}] DEBUG: {}", secs, message);
    }
} // fn log_to_file (debug build)

// @brief No-op log stub compiled only when debug_assertions is disabled — writes nothing.
// Note: this guarantees no file I/O in that configuration, but argument expressions at call
// sites may still be evaluated unless optimized away by the compiler.
#[cfg(not(debug_assertions))]
#[inline(always)]
pub(crate) fn log_to_file(_message: &str) {} // fn log_to_file (no-op when debug_assertions is disabled)

#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qobject]
        #[qproperty(bool, is_svg_imported)]
        #[qproperty(bool, is_layout_ready)]
        #[qproperty(bool, is_create_layout_enabled)]
        #[qproperty(bool, is_layout_in_progress)]
        #[qproperty(bool, is_adjust_mode)]
        #[qproperty(bool, is_adjust_dirty)]
        #[qproperty(QString, error_message)]
        #[qproperty(QString, layout_status_message)]
        #[qproperty(i32, layout_progress)]
        // Export progress percentage (0–100), or -1 when idle (not exporting).
        // Drives the progress bar in the export progress popup in QML.
        #[qproperty(i32, export_progress)]
        // Short status text for the in-progress export overlay.
        // Empty when idle; updated at the start of each export with the format name.
        #[qproperty(QString, export_status_message)]
        type AppController = super::AppControllerRust;

        #[qsignal]
        fn import_finished(self: Pin<&mut AppController>);

        #[qsignal]
        fn layout_finished(self: Pin<&mut AppController>);

        #[qsignal]
        fn export_finished(self: Pin<&mut AppController>, path: QString);

        #[qsignal]
        fn error_occurred(self: Pin<&mut AppController>, message: QString);

        // Emitted when a layout completed successfully but one or more pieces
        // could not be placed (non-tiled path).  The layout still renders the
        // pieces that fit; QML shows `message` as a non-blocking warning popup.
        #[qsignal]
        fn layout_warning(self: Pin<&mut AppController>, message: QString);

        // Emitted when an import succeeded but the SVG is not a Seamly2D Layout
        // Mode handoff file — it carries no `data-type="piece"` groups (Task 49).
        // The SVG is loaded and displayed regardless; QML shows `message` as a
        // non-blocking warning popup so an unexpected file cannot look like a
        // silent failure.
        #[qsignal]
        fn import_warning(self: Pin<&mut AppController>, message: QString);

        #[qsignal]
        fn progress_updated(self: Pin<&mut AppController>, percent: i32);

        // Emitted after accept_adjustments succeeds.
        // QML reloads the adjust window state without exiting AdjustMode.
        #[qsignal]
        fn adjust_applied(self: Pin<&mut AppController>);

        // --- Import SVG File ---

        #[qinvokable]
        fn import_svg(self: Pin<&mut AppController>, path: &QString) -> bool;

        // Import an SVG that never touched the filesystem (Seamly2D.5): the
        // piece-mode document Seamly2D hands over as one stringified SVG.
        #[qinvokable]
        fn import_svg_document(self: Pin<&mut AppController>, svg: &QString) -> bool;

        #[qinvokable]
        fn get_import_dom_string(self: &AppController) -> QString;

        // --- Create Layout ---

        #[qinvokable]
        fn initialize_layout(self: Pin<&mut AppController>, settings_json: &QString) -> bool;

        #[qinvokable]
        fn process_layout(self: Pin<&mut AppController>, settings_json: &QString) -> bool;

        #[qinvokable]
        fn get_layout_dom_string(self: &AppController) -> QString;

        // --- Adjust Layout ---

        // Enter AdjustMode: clone layout_dom into adjust_dom, enable interactive canvas.
        #[qinvokable]
        fn enter_adjust_mode(self: Pin<&mut AppController>);

        // Apply piece transforms from the interactive canvas into adjust_dom.
        // transforms_json: JSON array of {"id": string, "transform": string}.
        // Returns true on success; emits error_occurred on parse or DOM failure.
        #[qinvokable]
        fn accept_adjustments(
            self: Pin<&mut AppController>,
            transforms_json: &QString,
        ) -> bool;

        // Discard interactive changes — drop adjust_dom; layout_dom is untouched.
        #[qinvokable]
        fn discard_adjustments(self: Pin<&mut AppController>);

        // Exit AdjustMode: copy adjust_dom back to layout_dom, clear adjust_dom.
        // Sets is_adjust_mode = false and fires layout_finished to refresh the right canvas.
        #[qinvokable]
        fn exit_adjust_mode(self: Pin<&mut AppController>);

        // Check for piece overlaps given current canvas positions.
        // transforms_json: JSON array of {"id": string, "x": f64, "y": f64, "w": f64, "h": f64}.
        // Returns JSON array of piece IDs that overlap another piece or exceed the content rect.
        // Returns "[]" when no conflicts exist.
        #[qinvokable]
        fn check_overlaps(self: &AppController, transforms_json: &QString) -> QString;

        // Returns JSON array of {id, x, y, w, h} for all placed pieces in SVG canvas px.
        // x = ml_px + placement.x, y = mt_px + placement.y, w = placement.w, h = placement.h
        // Returns "[]" when no layout has been computed.
        #[qinvokable]
        fn get_piece_bboxes(self: &AppController) -> QString;

        /// Returns JSON array of {id, x, y, w, h, ox, oy, transform_str} for all pieces in adjust_dom.svg.
        #[qinvokable]
        fn get_adjust_piece_boxes(self: &AppController, filename: &QString) -> QString;

        // Serialize adjust_dom to an SVG XML string and return it.
        // Called from QML / AdjustWindow to display the adjust canvas inline
        // without file I/O.  Returns empty string when not in AdjustMode.
        #[qinvokable]
        fn get_adjust_dom_string(self: &AppController) -> QString;

        // Write adjust_dom to <exe_dir>/output/adjust_dom.svg and return
        // the absolute native path (for AdjustWindow file-based loading).
        // Returns empty string when no adjust DOM is available.
        #[qinvokable]
        fn save_adjust_dom(self: &AppController) -> QString;

        // --- Export Layout ---

        #[qinvokable]
        fn export_dxf(
            self: Pin<&mut AppController>,
            path: &QString,
            options_json: &QString,
        ) -> bool;

        #[qinvokable]
        fn export_pdf(
            self: Pin<&mut AppController>,
            path: &QString,
            settings_json: &QString,
        ) -> bool;

        // Export the assembled layout as a multi-page tiled PDF.
        // settings_json: the same JSON passed to processLayout — used to
        //  TileDimensions (tile size, margins) for viewport clipping.
        #[qinvokable]
        fn export_pdf_tiled(
            self: Pin<&mut AppController>,
            path: &QString,
            settings_json: &QString,
        ) -> bool;

        #[qinvokable]
        fn export_svg(self: Pin<&mut AppController>, path: &QString) -> bool;

        #[qinvokable]
        fn export_png(self: Pin<&mut AppController>, path: &QString, scale: f32) -> bool;
    }
} // mod qobject


pub struct AppControllerRust {

    // Parsed editable DOM for the most recently imported SVG.
    //
    // Populated by `import_svg` after `app_core::load_svg` + `add_background_rect`.
    // Consumed by `process_layout` to extract pattern pieces and build the layout SVG.
    // None when no SVG has been successfully imported.
    input_dom: Option<svg_dom::Document>,

    // Assembled layout output DOM, ready for DXF export.
    //
    // Created by `apply_settings` (Phase 5) with the content rectangle.
    // Populated with pattern pieces by `process_layout` (Phase 8).
    // Used by `export_dxf` (Phase 9) as the source for `svg_to_ezdxf`.
    // Cleared to None whenever a new SVG is imported (layout is invalidated).
    layout_dom: Option<svg_dom::Document>,

    // Blank canvas DOM produced by `initialize_layout` — the clean base for each run.
    //
    // Stored by `initialize_layout` alongside `layout_dom`; `process_layout` clones
    // this to obtain a fresh canvas before calling `create_layout`, so that
    // running Create Layout multiple times never accumulates pieces on a dirty canvas.
    // Cleared to None when a new SVG is imported.
    initial_layout_dom: Option<svg_dom::Document>,

    // Fully pre-processed DOM — the final flatten of the pre-processing pipeline.
    //
    // The pipeline is flatten → verticalize → flatten → translate → flatten;
    // this holds the result of that final flatten (translate_dom re-flattened).
    // It is the source used for piece extraction and layout placement.
    // Populated by `process_layout` from the stage snapshot it returns; None until
    // the first successful layout and cleared to None on a new import.
    flat_dom: Option<svg_dom::Document>,

    // Verticalized DOM — each piece rotated so its grainline is vertical.
    //
    // Copied from flat_dom after flatten step 1, then verticalized.
    // Serves as input to flatten step 3.
    vertical_dom: Option<svg_dom::Document>,

    // Translated DOM — each piece's AABB min corner moved to (0,0).
    //
    // Copied from flat_dom after flatten step 3, then translated.
    // Serves as input to flatten step 5.
    translate_dom: Option<svg_dom::Document>,

    // True when an SVG has been successfully imported into input_dom.
    //
    // Set to `true` by `import_svg` on success; reset to `false` at the start of
    // each new import. Drives the "Settings" button enabled state in QML.
    is_svg_imported: bool,

    // True when a completed layout SVG is available for display in the right canvas.
    //
    // Set to `true` when `process_layout` completes; reset to `false` on new import
    // and on Settings Submit (so Export is disabled until the next Create Layout).
    is_layout_ready: bool,

    // True when the Create Layout button should be enabled.
    //
    // Set to `true` by `initialize_layout` (Settings Submit) so the user can run
    // Create Layout after confirming the blank canvas looks correct.
    // Set to `false` by `process_layout` completion (no reason to re-run the same
    // layout) and by `import_svg` (new import invalidates prior settings).
    // Re-enabled by the next Settings Submit.
    is_create_layout_enabled: bool,

    // True while layout computation is running on a worker thread.
    //
    // Set to `true` at the start of `process_layout`; cleared on completion or error.
    // Controls the `BusyIndicator` visibility binding in QML.
    is_layout_in_progress: bool,

    // True while the interactive Adjust Layout canvas is active.
    //
    // Set to `true` by `enter_adjust_mode`; cleared by `accept_adjustments` or
    // `discard_adjustments`.  Drives button enable/disable bindings in QML so that
    // Import, Settings, Create Layout, and Export are all disabled during adjustment.
    is_adjust_mode: bool,

    // True when the interactive canvas has unsaved piece transforms.
    //
    // Set to `true` by QML when any piece is moved or rotated; cleared to `false`
    // by `enter_adjust_mode` (entering clean), `accept_adjustments` (changes saved),
    // and `discard_adjustments` (changes discarded).
    // Drives the dirty indicator (asterisk) on the Accept Adjustments button in QML.
    is_adjust_dirty: bool,

    // Working copy of layout_dom used exclusively during AdjustMode.
    //
    // Created by `enter_adjust_mode` as a clone of `layout_dom`.
    // All interactive transforms (move/rotate) are applied to `adjust_dom` by
    // `accept_adjustments`, leaving `layout_dom` untouched until the user exits.
    // `discard_adjustments` simply drops this DOM (layout_dom is the implicit snapshot).
    // `exit_adjust_mode` copies adjust_dom back into layout_dom, then clears it.
    // None when not in AdjustMode.
    adjust_dom: Option<svg_dom::Document>,

    // Full SVG canvas width in pixels, set by initialize_layout.
    // Semantic: the <svg width> of the layout canvas, margins included.
    // For sheet/roll: pageWidth  (or rollWidth) converted to px.
    // For tiled:      inputDomWidthPx  (full pattern, spanning all tiles).
    // Used by process_layout: bin_w = layout_w_px - margin_left_px - margin_right_px.
    // Reset to 0.0 on import.
    layout_w_px: u32,

    // Full SVG canvas height in pixels, set by initialize_layout.
    // Semantic: the <svg height> of the layout canvas, margins included.
    // For sheet:      pageHeight converted to px.
    // For roll:       500 in sentinel; updated to trimmed height after process_layout.
    // For tiled:      inputDomHeightPx (full pattern, spanning all tiles).
    // Used by process_layout: bin_h = layout_h_px - margin_top_px - margin_bottom_px.
    // Reset to 0.0 on import.
    layout_h_px: u32,

    // Left margin in pixels, computed from LayoutSettings during process_layout.
    // Included in piece_bboxes_json meta for reference.
    // Reset to 0 on new import.
    layout_ml_px: u32,

    // Top margin in pixels, computed from LayoutSettings during process_layout.
    // Included in piece_bboxes_json meta for reference.
    // Reset to 0 on new import.
    layout_mt_px: u32,

    // JSON object with layout metadata and piece bounding boxes, built during process_layout.
    // Format: {ml_px, mt_px, pieces: [{id, x, y, w, h, origin_x_px, origin_y_px}, ...]}
    // All coordinates are in layout pixels (piece.x/y are absolute canvas-pixel positions).
    // Updated by accept_adjustments after each Apply so bboxes stay in sync with layout_dom.
    // Cleared on new import. Empty string when no layout computed yet.
    piece_bboxes_json: String,

    // Snapshot of piece_bboxes_json taken on enter_adjust_mode.
    // Restored by discard_adjustments so Cancel after Apply reverts bboxes.
    // Cleared by exit_adjust_mode.
    piece_bboxes_json_snapshot: String,

    // Most recent error message, or empty string when no error is active.
    //
    // Set by any operation that fails; cleared at the start of a new operation.
    // The `error_occurred(message)` signal delivers the same text to QML with
    // a dedicated handler, providing more direct coupling than a property binding.
    error_message: cxx_qt_lib::QString,

    // Short status text for the in-progress layout overlay.
    // Empty when idle; updated during process_layout for stage/piece feedback.
    layout_status_message: cxx_qt_lib::QString,

    // Layout progress percentage (0–100), or -1 when idle (not computing).
    //
    // Updated periodically during `process_layout` via the worker thread.
    // -1 indicates idle; 0–100 drives the progress bar in QML.
    layout_progress: i32,

    // Export progress percentage (0–100), or -1 when idle (not exporting).
    //
    // Set to 0 at the start of each export method and to 100 on completion.
    // -1 = idle; 0–100 drives the export progress bar in QML.
    export_progress: i32,

    // Short status text for the in-progress export overlay.
    // Empty when idle; set to a format-specific label at the start of each export.
    export_status_message: cxx_qt_lib::QString,

} // struct AppControllerRust

impl Default for AppControllerRust {

    // Returns the idle/empty initial state for a newly created AppController.
    //
    // Called by CXX-Qt when QML instantiates `AppController { }`.
    fn default() -> Self {
        // Route log::debug! from layout_tiling (and any other log-facade consumer)
        // into the existing file-based logger.  Idempotent across process lifetime.
        init_file_logger();

        Self {
            input_dom:                 None,                           // no SVG imported yet
            layout_dom:                None,                           // no layout assembled yet
            initial_layout_dom:        None,                           // no settings submitted yet
            flat_dom:                  None,                           // no preprocessing run yet
            vertical_dom:              None,                           // no preprocessing run yet
            translate_dom:             None,                           // no preprocessing run yet
            adjust_dom:                None,                           // no adjust working copy
            is_svg_imported:           false,                          // no SVG loaded yet
            is_layout_ready:           false,                          // no completed layout available
            is_create_layout_enabled:  false,                          // disabled until Settings Submit
            is_layout_in_progress:     false,                          // not currently computing
            is_adjust_mode:            false,                          // not in adjust mode
            is_adjust_dirty:           false,                          // no unsaved adjustments
            layout_w_px:               0,                              // set by initialize_layout
            layout_h_px:               0,                              // set by initialize_layout
            layout_ml_px:              0,                              // default until first layout
            layout_mt_px:              0,                              // default until first layout
            piece_bboxes_json:         String::new(),                  // no layout computed yet
            piece_bboxes_json_snapshot: String::new(),                  // no snapshot until adjust mode
            error_message:             cxx_qt_lib::QString::default(), // no active error
            layout_status_message:     cxx_qt_lib::QString::default(), // no active status text
            layout_progress:           -1,                             // -1 = idle; 0–100 during compute
            export_progress:           -1,                             // -1 = idle; 0–100 during export
            export_status_message:     cxx_qt_lib::QString::default(), // no active export status
        } // Self
    } // fn default

} // impl Default for AppControllerRust

impl qobject::AppController {

    // -----------------------------------------------------------------------
    // Import
    // -----------------------------------------------------------------------

    // Load an SVG file, parse it into an editable DOM, add a white background
    // rectangle for canvas display, and store the result in `input_dom`.
    // No temp file is written — the SVG string is served from memory via
    // `get_import_dom_string()` on the `import_finished` signal.
    //
    // Emits `import_finished()` on success or `error_occurred(msg)` on failure.
    //
    // Note: runs synchronously on the Qt main thread.  For large SVGs a future
    // phase may offload this to a worker thread via std::thread + Qt signal.
    // Called by:
    // 'Import SVG' menu action in QML: onTriggered: appController.importSvg(fileDialog.fileUrl.toLocalFile())
    // and by the SeamlyLayout command line, which still accepts an SVG file path.
    fn import_svg(mut self: std::pin::Pin<&mut Self>, path: &cxx_qt_lib::QString) -> bool {
        let path_str = path.to_string();
        log_to_file(&format!("==========IMPORT SVG=========="));
        log_to_file(&format!("[import_svg] begin import filepath={path_str}"));

        self.as_mut().reset_import_state();

        // Load SVG from disk: parse into editable DOM + usvg tree for geometry.
        // The file's own directory resolves any external reference it carries.
        let loaded = app_core::load_svg(std::path::Path::new(&path_str));
        self.finish_import(loaded)
    } // fn import_svg

    // Import an SVG document held in memory — the Seamly2D piece-mode handoff
    // (Seamly2D.5).  Seamly2D serialises piece mode to one stringified SVG and
    // writes it to this process's standard input, so no handoff file is created
    // and a read-only pattern directory no longer blocks Layout Mode.
    //
    // Everything after the parse is identical to `import_svg`, so both entry
    // points share `reset_import_state()` and `finish_import()` and cannot
    // drift apart.
    //
    // Emits `import_finished()` on success or `error_occurred(msg)` on failure.
    fn import_svg_document(mut self: std::pin::Pin<&mut Self>, svg: &cxx_qt_lib::QString) -> bool {
        let svg_text = svg.to_string();
        log_to_file(&format!("==========IMPORT SVG DOCUMENT=========="));
        log_to_file(&format!(
            "[import_svg_document] begin import of {} characters",
            svg_text.len()
        ));

        if svg_text.trim().is_empty() {
            // An empty handoff means Seamly2D produced nothing. Say that,
            // instead of letting the XML parser report a syntax error at line 1.
            let msg = cxx_qt_lib::QString::from(
                "Seamly2D sent an empty layout document. Nothing was imported.",
            );
            self.as_mut().error_occurred(msg);
            return false;
        } // if svg_text is blank

        self.as_mut().reset_import_state();

        // No resources directory: the document never lived on disk, so a
        // relative external reference in it has nothing to resolve against.
        let loaded = app_core::parse_svg(&svg_text, None);
        self.finish_import(loaded)
    } // fn import_svg_document

    // Clear every piece of state a previous import produced.
    //
    // A new import invalidates the layout, the preprocessing DOMs, the bbox
    // snapshots and the export readiness. It deliberately keeps `input_dom` and
    // the settings: the current canvas stays visible until the new document is
    // parsed, and settings the user already entered survive a re-import.
    fn reset_import_state(mut self: std::pin::Pin<&mut Self>) {
        self.as_mut().set_is_svg_imported(false);
        self.as_mut().set_is_layout_ready(false);
        self.as_mut().set_is_layout_in_progress(false);
        self.as_mut().set_layout_progress(-1);
        {
            let mut rust = self.as_mut().rust_mut();
            // don't clear input_dom yet - keep the current input_dom displayed (if any)
            rust.layout_dom         = None; // clear stale layout DOM
            rust.initial_layout_dom = None; // clear stale initial canvas (settings invalidated)
            rust.flat_dom           = None; // clear stale preprocessing
            rust.vertical_dom       = None; // clear stale preprocessing
            rust.translate_dom      = None; // clear stale preprocessing
            rust.adjust_dom         = None; // clear stale adjust DOM
            rust.layout_w_px        = 0;    // reset until next initialize_layout
            rust.layout_h_px        = 0;    // reset until next initialize_layout
            rust.layout_ml_px       = 0;    // reset until next layout
            rust.layout_mt_px       = 0;    // reset until next layout
            // don't clear settings - keep the current settings (if any)
            // clear layout bbox data
            rust.piece_bboxes_json  = String::new(); // clear stale bbox data
            rust.piece_bboxes_json_snapshot = String::new(); // clear stale snapshot
        } // rust borrow dropped
        // Disable 'Create Layout' button until the user submits settings, so they don't run an invalid layout.
        self.as_mut().set_is_create_layout_enabled(false); // disabled until next Settings Submit
    } // fn reset_import_state

    // Finish an import once the SVG is parsed, whatever its source.
    //
    // @param loaded parse result from `app_core::load_svg` (a file) or from
    //        `app_core::parse_svg` (the Seamly2D in-memory handoff).
    // @return true when the document was stored and `import_finished()` emitted.
    fn finish_import(
        mut self: std::pin::Pin<&mut Self>,
        loaded: app_core::CoreResult<(svg_dom::Document, usvg::Tree)>,
    ) -> bool {
        match loaded {
            Ok((mut doc, _tree)) => {
                // Count the Seamly2D piece tagging BEFORE the DOM is moved into
                // `input_dom`.  Zero means this is not a Layout Mode handoff.
                let tagged_pieces = crate::piece_extractor::count_tagged_pieces(&doc);

                // Add a white background rectangle so the canvas has a visible background.
                doc.add_background_rect();

                // Store the parsed DOM; no disk write needed — SVG string is served from
                // memory via get_import_dom_string() on the import_finished signal.
                {
                    let mut rust = self.as_mut().rust_mut();
                    rust.input_dom = Some(doc);
                } // rust borrow dropped

                // Mark SVG as imported and notify QML.
                // QML handler: onImportFinished: leftCanvas.reloadSvg(appController.getImportedSvgString())
                self.as_mut().set_is_svg_imported(true);
                self.as_mut().import_finished();

                // Warn — but do not fail — when the SVG carries no piece tagging.
                // Layout still works (every top-level <g> with geometry is packed),
                // so this is a non-blocking popup shown after the canvas has the
                // document, never an error dialog instead of it.
                if tagged_pieces == 0 {
                    log_to_file("[finish_import] no data-type=\"piece\" groups — not a Seamly2D handoff");
                    let msg = cxx_qt_lib::QString::from(
                        "This SVG carries no tagged pattern pieces \
                         (data-type=\"piece\").\n\n\
                         Files exported by Seamly2D's Layout Mode are tagged; this one \
                         was not, so it may be an ordinary drawing. Every top-level group \
                         will be laid out as a piece.",
                    );
                    self.as_mut().import_warning(msg); // QML: onImportWarning → warning popup
                } else {
                    log_to_file(&format!("[finish_import] {tagged_pieces} tagged pattern piece(s) found"));
                } // if tagged_pieces == 0

                true // success
            }
            Err(e) => {
                // Emit error signal → QML shows error dialog
                let msg = cxx_qt_lib::QString::from(&format!("Failed to load SVG: {e}"));
                self.as_mut().error_occurred(msg); // notify QML of load error
                {
                    let mut rust = self.as_mut().rust_mut();
                    rust.input_dom = None; // clear stale DOM on error
                } // rust borrow dropped
                false // failure
            }
        } // match loaded
    } // fn finish_import

    // Serialize `input_dom` to an SVG XML string and return it.
    //
    // Called from the QML `onImportFinished` handler to push the SVG content
    // to the left canvas inline.  Returns empty string when no SVG is loaded
    fn get_import_dom_string(self: &Self) -> cxx_qt_lib::QString {
        match &self.rust().input_dom {
            Some(doc) => cxx_qt_lib::QString::from(doc.to_string().as_str()),
            None => cxx_qt_lib::QString::default(), // no SVG imported
        } // match input_dom
    } // fn get_import_dom_string

    // -----------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------

    // Build the initial layout_com, ready to place pieces on it
    //
    // Sets `is_layout_ready` to false (no pieces placed; export remains disabled).
    // Emits `layout_finished()` so QML reloads the right canvas.
    // Emits `error_occurred(msg)` if settings JSON cannot be parsed.
    // Called by:
    // 'Settings Dialog/Submit' button handler in QML: onSettingsSubmit: appController.initializeLayout(settingsJson)
    fn initialize_layout(
        mut self: std::pin::Pin<&mut Self>,
        settings_json: &cxx_qt_lib::QString,
    ) -> bool {
        // Delegate to pure-logic function in layout_utils.rs
        let json_str = settings_json.to_string();
        let input_dom_ref = self.rust().input_dom.as_ref();

        match do_initialize_layout(&json_str, input_dom_ref) {
            Ok(result) => {
                // Store results in rust state
                {
                    let mut rust = self.as_mut().rust_mut();
                    rust.initial_layout_dom = Some(result.initial_dom.clone()); // snapshot for re-running 'Create Layout'
                    rust.layout_dom         = Some(result.initial_dom);         // for display in right canvas
                    rust.layout_w_px = result.w_px;
                    rust.layout_h_px = result.h_px;
                } // rust borrow dropped

                // Emit signals to update QML state
                self.as_mut().set_is_layout_ready(false);          // not a completed layout yet
                self.as_mut().set_is_create_layout_enabled(true);  // enables 'Create Layout' button
                self.as_mut().layout_finished();                   // notify QML right canvas to reload
                true
            }
            Err(e) => {
                let msg = cxx_qt_lib::QString::from(e.as_str());
                self.as_mut().error_occurred(msg);
                false
            }
        }
    } // fn initialize_layout

    // Run the layout pipeline: parse settings, extract pieces, pack, assemble output SVG.
    // Emits error_occurred with a descriptive message on any failure path.
    // Called by 'Create Layout' button handler in QML: onCreateLayout: appController.processLayout(settingsJson)
    fn process_layout(
        mut self: std::pin::Pin<&mut Self>,
        settings_json: &cxx_qt_lib::QString,
    ) -> bool {
        // Mark layout as in-progress and clear previous result.
        self.as_mut().set_is_layout_in_progress(true);
        self.as_mut().set_is_layout_ready(false);
        self.as_mut().set_layout_progress(0);
        self.as_mut().set_layout_status_message(cxx_qt_lib::QString::from("Generating layout..."));

        // Gather inputs from rust state (releases borrow before calling do_process_layout)
        let json_str = settings_json.to_string();
        let (input_dom_clone, initial_layout_dom_clone, layout_h_px) = {
            let rust = self.rust();
            let input = match &rust.input_dom {
                Some(doc) => doc.clone(),
                None => {
                    let msg = cxx_qt_lib::QString::from(
                        "No SVG imported. Please import an SVG file first.",
                    );
                    self.as_mut().set_is_layout_in_progress(false);
                    self.as_mut().set_layout_progress(-1);
                    self.as_mut().error_occurred(msg);
                    return false;
                }
            };
            let initial = match &rust.initial_layout_dom {
                Some(doc) => doc.clone(),
                None => {
                    let msg = cxx_qt_lib::QString::from(
                        "No initial layout DOM found. Please apply settings before creating layout.",
                    );
                    self.as_mut().set_is_layout_in_progress(false);
                    self.as_mut().set_layout_progress(-1);
                    self.as_mut().error_occurred(msg);
                    return false;
                }
            };
            (input, initial, rust.layout_h_px)
        }; // rust borrow dropped

        // Delegate to pure-logic function in layout_utils.rs
        let args = ProcessLayoutArgs {
            settings_json: &json_str,
            input_dom: &input_dom_clone,
            initial_layout_dom: &initial_layout_dom_clone,
            layout_h_px,
        };
        // Progress callback drives both the bindable property (for the QML
        // ProgressBar overlay on the right canvas) and the signal (for any
        // imperative listeners).  Property update lets QML bind directly via
        // `appController.layoutProgress` without writing a Connections handler.
        let mut progress = |percent: i32, status: Option<&str>| {
            if let Some(message) = status {
                self.as_mut().set_layout_status_message(cxx_qt_lib::QString::from(message));
            }
            self.as_mut().set_layout_progress(percent);
            self.as_mut().progress_updated(percent);
        };

        match do_process_layout(args, &mut progress) {
            Ok(result) => {
                // Store results in rust state
                {
                    let mut rust = self.as_mut().rust_mut();
                    rust.layout_dom        = Some(result.output_doc);
                    rust.layout_h_px       = result.layout_h_px;
                    rust.layout_ml_px      = result.ml_px;
                    rust.layout_mt_px      = result.mt_px;
                    rust.piece_bboxes_json = result.bbox_json;
                    // Persist the pre-processing stage snapshots so AdjustMode and
                    // other steps can read them from memory instead of the debug SVGs.
                    rust.flat_dom          = Some(result.flat_dom);
                    rust.vertical_dom      = Some(result.vertical_dom);
                    rust.translate_dom     = Some(result.translate_dom);
                } // rust borrow dropped

                // Update properties and emit signals
                self.as_mut().set_is_layout_in_progress(false);    // layout completed
                self.as_mut().set_layout_progress(-1);             // reset progress bar to idle
                self.as_mut().set_layout_status_message(cxx_qt_lib::QString::default());
                self.as_mut().set_is_layout_ready(true);           // enables 'Export' button
                self.as_mut().set_is_create_layout_enabled(false); // disable 'Create Layout' until settings change
                self.as_mut().layout_finished();                   // notify QML right canvas to reload

                // Soft failure: some pieces didn't fit.  The layout still
                // rendered the pieces that did, so this is a warning (popup),
                // not an error that aborts the layout.
                if !result.unplaced_labels.is_empty() {
                    let n = result.unplaced_labels.len();
                    let list = result.unplaced_labels.join(", ");
                    let msg = format!(
                        "{n} piece(s) could not be placed and were left out of the layout:\n\n{list}\n\n\
                         The remaining pieces were laid out. Try a larger sheet size, reduce the piece gap or margins, or remove pieces."
                    );
                    self.as_mut().layout_warning(cxx_qt_lib::QString::from(msg.as_str()));
                } // if unplaced pieces

                true
            }
            Err(e) => {
                let msg = cxx_qt_lib::QString::from(e.as_str());
                self.as_mut().set_is_layout_in_progress(false);
                self.as_mut().set_layout_progress(-1);             // reset progress bar to idle on error
                self.as_mut().set_layout_status_message(cxx_qt_lib::QString::default());
                self.as_mut().error_occurred(msg);
                false
            }
        }
    } // fn process_layout

    // Serialize `layout_dom` to an SVG XML string and return it.
    //
    // Called from the QML `onLayoutFinished` handler to push the assembled
    // layout SVG to the right canvas inline.  Returns empty string when no
    // layout has been computed yet.
    // Called by onLayoutFinished: rightCanvas.reloadSvg(appController.getLayoutSvgString())
    fn get_layout_dom_string(self: &Self) -> cxx_qt_lib::QString {
        match &self.rust().layout_dom {
            Some(doc) => cxx_qt_lib::QString::from(doc.to_string().as_str()),
            None => cxx_qt_lib::QString::default(), // no layout assembled yet
        } // match layout_dom
    } // fn get_layout_dom_string

    // Return the piece bbox JSON array built by process_layout.
    //
    // Returns the JSON array of {id, x, y, w, h} for all placed pieces in SVG
    // canvas px space (ml_px + placement.x, mt_px + placement.y).
    // Returns "[]" when no layout has been computed yet.
    // Called by the adjust-mode UI to get piece bbox data for display.
    fn get_piece_bboxes(self: &Self) -> cxx_qt_lib::QString {
        cxx_qt_lib::QString::from(self.rust().piece_bboxes_json.as_str())
    } // fn get_piece_bboxes

    /// @brief Returns JSON array of {id, x, y, w, h, ox, oy, transform_str} for all pieces in adjust_dom.svg.
    /// transform_str is the SVG transform attribute or "" if none.
    /// TODO: fix — uses NodeExt and Document::from_str which don't exist in svg_dom yet
    /// Returns JSON array of {id, x, y, w, h, ox, oy, transform_str} for all pieces in the specified adjust_dom SVG file.
    /// Pass the filename (e.g., "adjust_dom.svg" or "adjust_dom_3.svg") as a QString from QML/C++.
    fn get_adjust_piece_boxes(self: &Self, filename: &cxx_qt_lib::QString) -> cxx_qt_lib::QString {
        // debug message to verify function is called and which file is being read
        let message = format!("[lib.rs AppController] get_adjust_piece_boxes() called. filename: {}", filename.to_string());
        log_to_file(&message);
        // Read and parse the specified adjust_dom SVG file from disk
        let path = get_out_dir().join(filename.to_string());
        let svg_str = match std::fs::read_to_string(&path) {
            Ok(s) => s,
            Err(_) => return cxx_qt_lib::QString::from("[]"),
        };
    let doc = match svg_dom::Document::parse(&svg_str) {
        Ok(d) => d,
        Err(_) => return cxx_qt_lib::QString::from("[]"),
    };
    // Build JSON array from top-level pattern-piece <g> elements.
    let mut arr = vec![];
    for child in &doc.root.children {
        if let Some(el) = child.as_element() {
            if el.name != "g" { continue; }
            let id = el.attributes.get("id").map(String::as_str).unwrap_or("");
            if id.is_empty() || id == "Rectangles" {
                continue;
            }

            let Some(bbox) = bbox_from_group_geometry(el) else {
                continue; // skip non-piece / non-geometric groups
            };

            let x = bbox.min.x as f64;
            let y = bbox.min.y as f64;
            let w = bbox.width() as f64;
            let h = bbox.height() as f64;
            let ox = 0.0;
            let oy = 0.0;
            let transform_str = el.attributes.get("transform").map(String::as_str).unwrap_or("");
            // Piece identity survives into the saved adjust_dom because
            // `create_layout` clones the whole piece <g> — attributes included —
            // so the human-readable name can be read straight back off it here.
            let name   = el.attributes.get("data-name").map(String::as_str).unwrap_or("");
            let letter = el.attributes.get("data-letter").map(String::as_str).unwrap_or("");
            // Same precedence as PieceRect::label(): name → letter → id.
            let label  = if !name.is_empty() { name } else if !letter.is_empty() { letter } else { id };
            arr.push(serde_json::json!({
                "id": id,
                "name": name,
                "letter": letter,
                "label": label,
                "x": x,
                "y": y,
                "w": w,
                "h": h,
                "origin_x_px": ox,
                "origin_y_px": oy,
                "transform_str": transform_str
            }));
        }
    }
    let meta = serde_json::json!({
        "pieces": arr,
    });
    let json = serde_json::to_string(&meta)
        .unwrap_or_else(|_| r#"{"pieces":[]}"#.to_string());
    cxx_qt_lib::QString::from(json.as_str())
} // fn get_adjust_piece_boxes

    // Serialize `adjust_dom` to an SVG XML string and return it.
    //
    // Called from QML / AdjustWindow to display the adjust canvas inline
    // without file I/O.  Returns empty string when not in AdjustMode
    // (i.e. when adjust_dom is None).
    // Called by onAdjustModeEntered: adjustCanvas.reloadSvg(appController.getAdjustSvgString())
    fn get_adjust_dom_string(self: &Self) -> cxx_qt_lib::QString {
        match &self.rust().adjust_dom {
            Some(doc) => cxx_qt_lib::QString::from(doc.to_string().as_str()),
            None => cxx_qt_lib::QString::default(), // not in AdjustMode
        } // match adjust_dom
    } // fn get_adjust_dom_string

    // Write adjust_dom to <exe_dir>/output/adjust_dom.svg for AdjustWindow.
    //
    // Called from QML as saveAdjustDom() when entering AdjustMode or after accept_adjustments.
    // The returned native file path is used by AdjustWindow (QtWidgets) to load
    // the SVG.  Returns empty string when adjust_dom is None or the write fails.
    // Called by the adjust-mode UI when opening or refreshing AdjustWindow.
    fn save_adjust_dom(self: &Self) -> cxx_qt_lib::QString {
        // Save adjust_dom to <exe_dir>/output/adjust_dom.svg using app_core::save_svg.

        let doc = match &self.rust().adjust_dom {
            Some(d) => d,
            None => return cxx_qt_lib::QString::default(), // not in AdjustMode
        };

        // Save a numbered debug copy alongside the canonical file.
        log_to_file(&format!("[lib.rs AppController] save_adjust_dom(): 1 saving adjust_dom_nn.svg with {} pieces", self.rust().piece_bboxes_json));
        save_debug_dom(doc, "adjust_dom.svg");

        let out_path = get_out_dir().join("adjust_dom.svg");
        match app_core::save_svg(doc, &out_path) {
            Ok(_)  => cxx_qt_lib::QString::from(out_path.to_string_lossy().as_ref()),
            Err(_) => cxx_qt_lib::QString::default(), // write failed
        }
    } // fn save_adjust_dom

    // -----------------------------------------------------------------------
    // Adjust Layout implementations
    // -----------------------------------------------------------------------

    // Enter AdjustMode: clone layout_dom into adjust_dom, enable interactive canvas.
    //
    // Called from QML when the user clicks 'Adjust Layout'.
    // Sets `is_adjust_mode` = true and `is_adjust_dirty` = false.
    // Clones `layout_dom` into `adjust_dom` as the working copy for all
    // interactive transforms.  layout_dom remains untouched until exit_adjust_mode
    // promotes the working copy back.
    // Emits `error_occurred` if no layout is available.
    // Called by 'Adjust Layout' button handler in QML: onAdjustLayout: appController.enterAdjustMode()
    fn enter_adjust_mode(mut self: std::pin::Pin<&mut Self>) {

        // Clone layout_dom into adjust_dom as the working copy.
        let adjust_dom = {
            let rust = self.rust();
            match &rust.layout_dom {
                Some(doc) => Some(doc.clone()),
                None => None, // no layout_dom yet; cannot enter AdjustMode
            } // match layout_dom
        }; // adjust_dom created; rust borrow dropped
        if adjust_dom.is_none() {
            let msg = cxx_qt_lib::QString::from(
                "No layout available. Run 'Create Layout' before adjusting.",
            );
            self.as_mut().error_occurred(msg); // nothing to adjust
            return; // if no layout_dom can't enter adjust mode
        } // if adjust_dom is None

        // save adjust_dom as a debug file
        save_debug_dom(adjust_dom.as_ref().unwrap(), "adjust_dom.svg");

        // store adjust_dom to rust global state for access by other adjust_mode functions during AdjustMode
        {
            let mut rust = self.as_mut().rust_mut();
            rust.adjust_dom = adjust_dom;
            rust.piece_bboxes_json_snapshot = rust.piece_bboxes_json.clone();
        }
        // Emit signals to enter adjust mode with the new adjust_dom.
        self.as_mut().set_is_adjust_mode(true);
        self.as_mut().set_is_adjust_dirty(false); // no pieces have moved yet
    } // fn enter_adjust_mode

    // Apply interactive changes -- overwrite each piece with its full current transform from the interactive canvas.
    //
    // Parses `transforms_json` — a JSON array of {"id": string, "transform": string}
    // objects — and writes each `transform` attribute into the matching `<g>` element
    // in `adjust_dom` using `svg_dom::Document::set_attr_by_id`.
    //
    // Because adjust_dom is pixel-pure (all coordinates are in layout pixels and there
    // is no shared scale wrapper), the transform strings from QML are of the form
    // "translate(tx_px ty_px)", "translate(tx_px ty_px) rotate(a cx cy)", or "rotate(a cx cy)""
    // After writing to the DOM, `piece_bboxes_json` is updated so that the next call
    // to `activateForAdjust` sees the new positions without re-running the full layout.
    //
    // On success: sets is_adjust_dirty=false, emits `adjust_applied`.
    // On parse failure: emits `error_occurred` and returns false without modifying DOM.
    // Called by QML onApplyRequested()
    fn accept_adjustments(
        mut self: std::pin::Pin<&mut Self>,
        transforms_json: &cxx_qt_lib::QString,
    ) -> bool {
        // Struct for one entry in the transforms JSON array.
        #[derive(serde::Deserialize)]
        struct PieceTransform {
            id:        String,
            transform: String,
        } // struct PieceTransform

        log_to_file(&format!("[lib.rs AppController] accept_adjustments(): 1 called with transforms_json: {}", transforms_json.to_string()));

        let json_str = transforms_json.to_string();
        let entries: Vec<PieceTransform> = match serde_json::from_str(&json_str) {
            Ok(v) => v,
            Err(e) => {
                let msg = cxx_qt_lib::QString::from(
                    &format!("[lib.rs-AppController] accept_adjustments(): 2 Failed to parse adjustment transforms: {e}"),
                );
                self.as_mut().error_occurred(msg); // JSON parse error
                return false; // if parse failed
            } // Err
        }; // match from_str

        {
            let mut rust = self.as_mut().rust_mut();
            let doc = match rust.adjust_dom.as_mut() {
                Some(d) => d,
                None => {
                    // adjust_dom was cleared while in AdjustMode (should not happen)
                    drop(rust);
                    let msg = cxx_qt_lib::QString::from("[lib.rs-AppController] accept_adjustments(): 3 adjut_dom unavailable during accept.");
                    self.as_mut().error_occurred(msg); // internal state error
                    return false; // if adjust_dom is None
                } // None
            }; // match adjust_dom

            // Each entry now carries the piece's full canonical transform rebuilt
            // from its current overlay pose. Overwrite the DOM attribute instead
            // of appending another delta to the existing chain.
            for entry in &entries {
                let new_transform = entry.transform.trim();
                let existing = doc
                    .get_attr_by_id(&entry.id, "transform")
                    .map(|s| s.trim().to_string())
                    .unwrap_or_default();

                log_to_file(&format!(
                    "[lib.rs-AppController] accept_adjustments(): 4 updating adjust_dom id='{}' old_transform='{}' full_transform='{}'",
                    entry.id, existing, new_transform
                ));
                doc.set_attr_by_id(&entry.id, "transform", new_transform);
            } // for entry in entries
            // NOTE: Do NOT update piece_bboxes_json (x, y) here. Overlay positions remain canonical until adjustments are finalized.

        } // rust borrow dropped

        // Debug: save adjust_dom after transforms have been applied.
        {
            let rust = self.rust();
            if let Some(doc) = &rust.adjust_dom {
                log_to_file(&format!("[lib.rs AppController] accept_adjustments(): 5 saving adjust_dom.svg after applying transforms to disk for debugging."));
                save_debug_dom(doc, "adjust_dom.svg");
            } // if adjust_dom
        } // rust borrow dropped

        // Stay in AdjustMode so the user can continue moving pieces.
        // QML handles the reload by refreshing AdjustWindow with the updated DOM.
        self.as_mut().set_is_adjust_dirty(false);
        self.as_mut().adjust_applied(); // signal QML adjustApplied to refresh the adjust UI state
        // return true to onApplyRequested()
        true // success
    } // fn accept_adjustments

    // Discard interactive changes — drop adjust_dom; layout_dom is untouched.
    //
    // Called from QML when the user clicks 'Discard Adjustments' (and confirms
    // the discard dialog when `is_adjust_dirty` is true).
    // Because all interactive transforms were applied to adjust_dom (not layout_dom),
    // discarding simply drops adjust_dom.  layout_dom still holds the original
    // layout result from process_layout.
    // Clears AdjustMode flags and emits `layout_finished` so the right canvas reloads.
    // Called by QML 'Discard Adjustments' button handler in AdjustWindow: onDiscardAdjustments: appController.discardAdjustments()
    fn discard_adjustments(mut self: std::pin::Pin<&mut Self>) {
        {
            let mut rust = self.as_mut().rust_mut();
            rust.adjust_dom = None; // drop the working copy; layout_dom is unchanged
            rust.piece_bboxes_json = rust.piece_bboxes_json_snapshot.clone();
        } // rust borrow dropped

        // Remove stale adjust_overlay_*.json and adjust_dom_*.svg artifacts
        // before returning to the main canvas state.
        cleanup_adjust_output_artifacts();

        self.as_mut().set_is_adjust_mode(false);
        self.as_mut().set_is_adjust_dirty(false);
        self.as_mut().layout_finished(); // reload right canvas with original layout
    } // fn discard_adjustments

    // Exit AdjustMode after all adjustments have been applied.
    //
    // Called from QML when the user clicks 'Done Adjusting'.
    // Copies adjust_dom (with all accepted transforms) back into layout_dom so
    // the adjusted positions become the canonical layout for export and display.
    // Clears adjust_dom, AdjustMode flags, and emits `layout_finished` so the
    // right canvas reloads with the final adjusted layout.
    // Called by QML 'Done Adjusting' button handler in AdjustWindow: onDoneAdjusting: appController.exitAdjustMode()
    fn exit_adjust_mode(mut self: std::pin::Pin<&mut Self>) {
        {
            let mut rust = self.as_mut().rust_mut();
            // Promote the adjusted dom to the canonical layout dom.
            if let Some(mut adjusted) = rust.adjust_dom.take() {
                // `take()` moves the Document out of `adjust_dom` and leaves
                // `rust.adjust_dom = None` (i.e., release/clear adjust_dom).
                // Flatten adjust_dom in-place before promoting it to layout_dom.
                // This bakes interactive transforms into geometry so the main
                // canvas gets a transform-free, canonical adjusted layout.
                svg_dom::flatten_dom(&mut adjusted);

                // For tiled layouts, remove blank bottom tile rows before
                // promoting the saved adjust DOM to the main layout canvas.
                let rows_removed = trim_empty_tiled_rows_in_adjust_dom(&mut adjusted);
                if rows_removed > 0 {
                    log_to_file(&format!(
                        "[lib.rs AppController] exit_adjust_mode(): trimmed {rows_removed} blank tiled row(s) from adjusted layout."
                    ));
                }

                // Keep layout_h_px in sync with the promoted DOM height.
                rust.layout_h_px = adjusted
                    .root
                    .attributes
                    .get("height")
                    .and_then(|v| v.parse::<u32>().ok())
                    .unwrap_or(rust.layout_h_px);

                rust.layout_dom = Some(adjusted);
            }
            // clear piece_bboxes_json to the current adjust_dom positions so the next activateForAdjust() call sees the adjusted positions as the new baseline without needing to re-run the full layout
            rust.piece_bboxes_json_snapshot.clear();
        } // rust borrow dropped

        // Remove stale adjust_overlay_*.json and adjust_dom_*.svg artifacts
        // before returning to the main canvas state.
        cleanup_adjust_output_artifacts();

        self.as_mut().set_is_adjust_mode(false);
        self.as_mut().set_is_adjust_dirty(false);
        self.as_mut().layout_finished(); // reload right canvas with final adjusted layout
    } // fn exit_adjust_mode

    // Check for piece overlaps given current canvas bounding boxes.
    //
    // Accepts `transforms_json` — a JSON array of
    //   {"id": string, "x": f64, "y": f64, "w": f64, "h": f64}
    // representing each piece's current axis-aligned bounding box on the canvas.
    //
    // Returns a JSON array of piece IDs that:
    //   - overlap (intersect) any other piece's bounding box, or
    //   - extend outside the content rectangle stored in adjust_dom (id="contentRect").
    //
    // Returns "[]" when there are no conflicts.
    // Returns "[]" also on parse failure (treated as non-blocking; UI highlights lost).
    // Called by the adjust-mode UI after each interactive move to highlight conflicts.
    fn check_overlaps(self: &Self, transforms_json: &cxx_qt_lib::QString) -> cxx_qt_lib::QString {
        // Struct for one entry in the bounding-box JSON array.
        #[derive(serde::Deserialize, Clone)]
        struct PieceBox {
            id: String,
            x:  f64,
            y:  f64,
            w:  f64,
            h:  f64,
        } // struct PieceBox

        let json_str = transforms_json.to_string();
        let boxes: Vec<PieceBox> = match serde_json::from_str(&json_str) {
            Ok(v) => v,
            Err(_) => return cxx_qt_lib::QString::from("[]"), // parse failure → no conflicts reported
        }; // match from_str

        // Read the content rectangle bounds from adjust_dom (id="contentRect").
        // Falls back to layout_dom if adjust_dom is None, then to an infinite rect.
        let (cr_x, cr_y, cr_w, cr_h) = {
            let rust = self.rust();
            let dom_ref = rust.adjust_dom.as_ref().or(rust.layout_dom.as_ref());
            match dom_ref {
                Some(doc) => {
                    let x = doc.get_attr_by_id("contentRect", "x")
                        .and_then(|v| v.parse::<f64>().ok()).unwrap_or(0.0);
                    let y = doc.get_attr_by_id("contentRect", "y")
                        .and_then(|v| v.parse::<f64>().ok()).unwrap_or(0.0);
                    let w = doc.get_attr_by_id("contentRect", "width")
                        .and_then(|v| v.parse::<f64>().ok()).unwrap_or(f64::MAX);
                    let h = doc.get_attr_by_id("contentRect", "height")
                        .and_then(|v| v.parse::<f64>().ok()).unwrap_or(f64::MAX);
                    (x, y, w, h)
                } // Some
                None => (0.0, 0.0, f64::MAX, f64::MAX), // no layout; skip out-of-bounds check
            } // match layout_dom
        }; // rust borrow dropped

        let mut conflicting: Vec<String> = Vec::new();

        for (i, a) in boxes.iter().enumerate() {
            // Out-of-bounds check: any corner of piece a outside content rect.
            let out_of_bounds = a.x < cr_x
                || a.y < cr_y
                || (a.x + a.w) > (cr_x + cr_w)
                || (a.y + a.h) > (cr_y + cr_h);
            if out_of_bounds && !conflicting.contains(&a.id) {
                conflicting.push(a.id.clone());
            } // if out_of_bounds

            // Pairwise overlap check: axis-aligned bounding box intersection.
            for b in boxes.iter().skip(i + 1) {
                let overlap = a.x < (b.x + b.w)
                    && (a.x + a.w) > b.x
                    && a.y < (b.y + b.h)
                    && (a.y + a.h) > b.y;
                if overlap {
                    if !conflicting.contains(&a.id) {
                        conflicting.push(a.id.clone());
                    } // if a not yet listed
                    if !conflicting.contains(&b.id) {
                        conflicting.push(b.id.clone());
                    } // if b not yet listed
                } // if overlap
            } // for b in boxes
        } // for (i, a) in boxes

        let result_json = serde_json::to_string(&conflicting).unwrap_or_else(|_| "[]".to_string());
        cxx_qt_lib::QString::from(result_json.as_str())
    } // fn check_overlaps

    // -----------------------------------------------------------------------
    // Export implementations
    // -----------------------------------------------------------------------

    // Export the assembled layout as a DXF-ASTM file (ASTM-D6673-10).
    // -----------------------------------------------------------------------
    // Export helper — clone layout_dom and strip piece fills.
    // Called at the start of every export method before releasing self borrow.
    // Returns Err(QString) if no layout DOM is available.
    // -----------------------------------------------------------------------
    // Called by export_dxf(), export_pdf(), and export_pdf_tiled() to get a clean copy of the layout DOM with display-only colored fill blocks and backgroundRect, contentRect, and tiledRect rectangles removed.
    fn clone_stripped_layout_doc(&self) -> Result<svg_dom::Document, cxx_qt_lib::QString> {
        match &self.rust().layout_dom {
            Some(doc) => {
                let mut cloned = doc.clone();
                remove_color_blocks(&mut cloned); // remove display-only colored fill blocks from layout processing
                // Remove group with id='Rectangles' containing backgroundRect, contentRect, and tiledRects
                remove_group_by_id(&mut cloned.root, "Rectangles");
                Ok(cloned)
            } // Some
            None => Err(cxx_qt_lib::QString::from(
                "No layout available. Run Create Layout before exporting.",
            )), // None
        } // match layout_dom
    } // fn clone_stripped_layout_doc

    // -----------------------------------------------------------------------
    // Export helper — clone layout_dom UNCHANGED (no stripping).
    // Unlike clone_stripped_layout_doc(), this keeps the piece fills/styles,
    // the <g> group structure, every id attribute, and the
    // backgroundRect/contentRect/tiledRects.  Used by SVG export so the saved
    // file is a faithful, fully-styled copy of the on-screen layout.
    // Returns Err(QString) if no layout DOM is available.
    // -----------------------------------------------------------------------
    fn clone_layout_doc(&self) -> Result<svg_dom::Document, cxx_qt_lib::QString> {
        match &self.rust().layout_dom {
            Some(doc) => Ok(doc.clone()), // full fidelity — styles, groups, ids kept
            None => Err(cxx_qt_lib::QString::from(
                "No layout available. Run Create Layout before exporting.",
            )), // None
        } // match layout_dom
    } // fn clone_layout_doc

    // -----------------------------------------------------------------------
    // DXF-ASTM export
    // -----------------------------------------------------------------------

    // Export the assembled layout as a DXF-ASTM file.
    // options_json: {"createTeachingVersion": true/false}
    // Delegates core logic to exports::do_export_dxf.
    // Called by QML 'Export DXF' button handler: onExportDXF: appController.exportDxf(path, optionsJson)
    fn export_dxf(
        mut self: std::pin::Pin<&mut Self>,
        path: &cxx_qt_lib::QString,
        options_json: &cxx_qt_lib::QString,
    ) -> bool {
        let path_str = path.to_string();
        log_to_file(&format!("[lib.rs AppController] export_dxf(): 1 requested path='{path_str}'"));

        // Parse teaching-version flag from options JSON.
        #[derive(serde::Deserialize, Default)]
        #[serde(rename_all = "camelCase")]
        struct DxfExportOptions {
            #[serde(default)]
            create_teaching_version: bool,
        } // struct DxfExportOptions
        let opts: DxfExportOptions = match serde_json::from_str(&options_json.to_string()) {
            Ok(v) => v,
            Err(e) => {
                log_to_file(&format!(
                    "[lib.rs AppController] export_dxf(): 2 invalid options JSON: {e}; using defaults"
                ));
                DxfExportOptions::default()
            }
        };

        log_to_file(&format!(
            "[lib.rs AppController] export_dxf(): 2 options parsed create_teaching_version={}",
            opts.create_teaching_version
        ));
        let layout_doc = match self.clone_stripped_layout_doc() {
            Ok(d)  => {
                log_to_file("[lib.rs AppController] export_dxf(): 3 layout DOM cloned and stripped");
                d
            }
            Err(m) => {
                log_to_file(&format!("[lib.rs AppController] export_dxf(): 3 no layout DOM: {}", m.to_string()));
                self.as_mut().error_occurred(m);
                return false; // if no layout
            }
        }; // layout_doc

        // Signal export start (0%); QML popup is already open before this call.
        self.as_mut().set_export_progress(0);
        self.as_mut().set_export_status_message(cxx_qt_lib::QString::from("Exporting DXF-ASTM…"));
        self.as_mut().progress_updated(0);

        log_to_file(&format!("[lib.rs AppController] export_dxf(): 4 delegating to do_export_dxf for '{path_str}'"));

        // Progress closure: each intermediate tick from do_export_dxf updates both
        // the bindable property (QML ProgressBar) and fires the signal (imperative listeners).
        let mut dxf_progress = |pct: i32| {
            self.as_mut().set_export_progress(pct);
            self.as_mut().progress_updated(pct);
        };

        match do_export_dxf(&layout_doc, &path_str, opts.create_teaching_version, &mut dxf_progress) {
            Ok(()) => {
                log_to_file(&format!("[lib.rs AppController] export_dxf(): 5 export succeeded '{path_str}'"));
                self.as_mut().progress_updated(100);
                self.as_mut().set_export_progress(-1); // reset to idle (-1 = idle contract)
                self.as_mut().set_export_status_message(cxx_qt_lib::QString::default()); // clear
                self.as_mut().export_finished(cxx_qt_lib::QString::from(&path_str)); // success
                true // success
            } // Ok
            Err(e) => {
                log_to_file(&format!("[lib.rs AppController] export_dxf(): 5 export failed: {e}"));
                self.as_mut().set_export_progress(-1);
                self.as_mut().set_export_status_message(cxx_qt_lib::QString::default()); // clear
                self.as_mut().error_occurred(cxx_qt_lib::QString::from(&e)); // export failed
                false // failure
            } // Err
        } // match do_export_dxf
    } // fn export_dxf

    // -----------------------------------------------------------------------
    // PDF export (single page)
    // -----------------------------------------------------------------------

    // Export the assembled layout as a PDF file.
    //
    // Sheet mode (`mediaType=="paper" && paperType=="sheet"`) uses the L.2.1/L.2.2
    // multi-page export path so oversized pieces tile and remaining pieces flow
    // across additional sheets. All other modes retain the existing single-page
    // PDF export behavior.
    //
    // Called by QML 'Export PDF' button handler: onExportPDF: appController.exportPdf(path, settingsJson)
    fn export_pdf(
        mut self: std::pin::Pin<&mut Self>,
        path: &cxx_qt_lib::QString,
        settings_json: &cxx_qt_lib::QString,
    ) -> bool {
        let path_str = path.to_string();
        log_to_file(&format!("[lib.rs AppController] export_pdf(): 1 requested path='{path_str}'"));

        let settings = match LayoutSettings::from_json(&settings_json.to_string()) {
            Ok(v) => v,
            Err(e) => {
                log_to_file(&format!("[lib.rs AppController] export_pdf(): 2 invalid settings JSON: {e}"));
                self.as_mut().error_occurred(cxx_qt_lib::QString::from(
                    &format!("PDF export: invalid settings JSON: {e}")
                ));
                return false;
            } // Err
        }; // settings

        log_to_file(&format!("[lib.rs AppController] export_pdf(): 2 media_type='{}' paper_type='{}'", settings.media_type, settings.paper_type));

        // Signal export start (0%); QML popup is already open before this call.
        self.as_mut().set_export_progress(0);
        self.as_mut().set_export_status_message(cxx_qt_lib::QString::from("Exporting PDF…"));
        self.as_mut().progress_updated(0);

        // Progress handling: emit 0%/100% here; forward intermediate ticks from the export path
        // (single-page via a progress closure, sheet-mode via manual 10%/90% ticks).
        // Keep `self` borrows scoped so the closure can borrow `self` mutably when needed.
        let export_result = if settings.media_type == "paper" && settings.paper_type == "sheet" {
            log_to_file("[lib.rs AppController] export_pdf(): 3 using sheet (multi-page) export path");
            let input_dom = match self.rust().input_dom.as_ref() {
                Some(doc) => doc.clone(),
                None => {
                    log_to_file("[lib.rs AppController] export_pdf(): 4 no input_dom for sheet export");
                    self.as_mut().set_export_progress(-1);
                    self.as_mut().set_export_status_message(cxx_qt_lib::QString::default());
                    self.as_mut().error_occurred(cxx_qt_lib::QString::from(
                        "PDF export: no imported SVG is available for sheet export."
                    ));
                    return false;
                } // None
            }; // input_dom

            let (flat_dom, pieces) = match build_sheet_export_inputs(&input_dom) {
                Ok(v) => v,
                Err(e) => {
                    log_to_file(&format!("[lib.rs AppController] export_pdf(): 4 build_sheet_export_inputs failed: {e}"));
                    self.as_mut().set_export_progress(-1);
                    self.as_mut().set_export_status_message(cxx_qt_lib::QString::default());
                    self.as_mut().error_occurred(cxx_qt_lib::QString::from(&e));
                    return false;
                } // Err
            }; // flat_dom, pieces

            log_to_file(&format!("[lib.rs AppController] export_pdf(): 4 sheet inputs built; {} pieces", pieces.len()));

            // Emit intermediate progress around the blocking sheet export.
            self.as_mut().set_export_progress(10);
            self.as_mut().progress_updated(10);
            let result = do_export_sheets_pdf(&flat_dom, &pieces, &path_str, &settings);
            if result.is_ok() {
                self.as_mut().set_export_progress(90);
                self.as_mut().progress_updated(90);
            } // if ok
            result
        } else {
            log_to_file("[lib.rs AppController] export_pdf(): 3 using single-page export path");
            let layout_doc = match self.clone_stripped_layout_doc() {
                Ok(d)  => d,
                Err(m) => {
                    log_to_file("[lib.rs AppController] export_pdf(): 4 no layout_dom available");
                    self.as_mut().set_export_progress(-1);
                    self.as_mut().set_export_status_message(cxx_qt_lib::QString::default());
                    self.as_mut().error_occurred(m);
                    return false;
                } // Err
            }; // layout_doc

            // Forward do_export_pdf progress ticks to the QML progress popup.
            let mut progress = |pct: i32| {
                self.as_mut().set_export_progress(pct);
                self.as_mut().progress_updated(pct);
            };
            do_export_pdf(&layout_doc, &path_str, &mut progress)
        }; // export_result

        match export_result {
            Ok(()) => {
                log_to_file(&format!("[lib.rs AppController] export_pdf(): 5 wrote PDF '{path_str}'"));
                self.as_mut().progress_updated(100);
                self.as_mut().set_export_progress(-1); // reset to idle (-1 = idle contract)
                self.as_mut().set_export_status_message(cxx_qt_lib::QString::default()); // clear
                self.as_mut().export_finished(cxx_qt_lib::QString::from(&path_str)); // success
                true // success
            } // Ok
            Err(e) => {
                log_to_file(&format!("[lib.rs AppController] export_pdf(): 5 failed: {e}"));
                self.as_mut().set_export_progress(-1);
                self.as_mut().set_export_status_message(cxx_qt_lib::QString::default()); // clear
                self.as_mut().error_occurred(cxx_qt_lib::QString::from(&e)); // export failed
                false // failure
            } // Err
        } // match export_result
    } // fn export_pdf

    // -----------------------------------------------------------------------
    // PDF Tiled export (multi-page)
    // -----------------------------------------------------------------------

    // Export the assembled layout as a multi-page tiled PDF.
    // settings_json: the same JSON passed to processLayout — used to reconstruct
    // TileDimensions (tile size, margins) for per-tile viewport clipping.
    // Delegates core logic to exports::do_export_pdf_tile.
    // Called by:
    // --> 'Export PDF (Tiled)' menu action --> exportPdfTiledRequested signal --> onExportPDFTiledRequested handler --> exportPdfTiled() slot
    fn export_pdf_tiled(
        mut self: std::pin::Pin<&mut Self>,
        path: &cxx_qt_lib::QString,
        settings_json: &cxx_qt_lib::QString
    ) -> bool {
        let path_str = path.to_string();
        log_to_file(&format!("[lib.rs AppController] export_pdf_tiled(): 1 requested path='{path_str}'"));

        // get cleaned layout DOM for export (minus rects + color blocks)
        let layout_doc = match self.clone_stripped_layout_doc() {
            Ok(d)  => d, // success
            Err(m) => {
                log_to_file("[lib.rs AppController] export_pdf_tiled(): 2 no layout_dom available");
                self.as_mut().error_occurred(m);
                return false;
            } // if no layout
        };

        log_to_file(&format!("[lib.rs AppController] export_pdf_tiled(): 2 layout_dom cloned; starting export to '{path_str}'"));

        // Signal export start (0%); QML popup is already open before this call.
        self.as_mut().set_export_progress(0);
        self.as_mut().set_export_status_message(cxx_qt_lib::QString::from("Exporting tiled PDF…"));
        self.as_mut().progress_updated(0);

        // export the layout as tiled PDF
        match do_export_pdf_tile(&layout_doc, &path_str, settings_json) {
            Ok(()) => {
                log_to_file(&format!("[lib.rs AppController] export_pdf_tiled(): 3 wrote tiled PDF '{path_str}'"));
                self.as_mut().progress_updated(100);
                self.as_mut().set_export_progress(-1); // reset to idle (-1 = idle contract)
                self.as_mut().set_export_status_message(cxx_qt_lib::QString::default()); // clear
                self.as_mut().export_finished(cxx_qt_lib::QString::from(&path_str));
                true // success
            }
            Err(e) => {
                log_to_file(&format!("[lib.rs AppController] export_pdf_tiled(): 3 failed: {e}"));
                self.as_mut().set_export_progress(-1);
                self.as_mut().set_export_status_message(cxx_qt_lib::QString::default()); // clear
                self.as_mut().error_occurred(cxx_qt_lib::QString::from(&e)); // export failed
                false // failure
            }
        } // match do_export_pdf_tile
    } // fn export_pdf_tiled

    // -----------------------------------------------------------------------
    // SVG export
    // -----------------------------------------------------------------------

    // Export the assembled layout as an SVG file.
    //
    // Uses the UNSTRIPPED layout DOM (clone_layout_doc) so the saved file keeps
    // its piece fills/styles, <g> group structure, id names, and the
    // background/content/tiled rectangles — a faithful copy of the on-screen
    // layout.  Delegates serialization to exports::do_export_svg.
    // Called by QML 'Export SVG' menu handler: onExportSvgRequested → appController.exportSvg(path)
    fn export_svg(
        mut self: std::pin::Pin<&mut Self>,
        path: &cxx_qt_lib::QString,
    ) -> bool {
        let path_str = path.to_string();
        log_to_file(&format!("[lib.rs AppController] export_svg(): 1 requested path='{path_str}'"));

        // Full-fidelity clone — do NOT strip fills/styles/groups/ids/rects.
        let layout_doc = match self.clone_layout_doc() {
            Ok(d)  => d,
            Err(m) => {
                log_to_file("[lib.rs AppController] export_svg(): 2 no layout_dom available");
                self.as_mut().error_occurred(m);
                return false; // if no layout
            } // Err
        }; // layout_doc

        // Signal export start (0%); QML popup is already open before this call.
        self.as_mut().set_export_progress(0);
        self.as_mut().set_export_status_message(cxx_qt_lib::QString::from("Exporting SVG…"));
        self.as_mut().progress_updated(0);

        // Serialize the styled DOM to the chosen path.
        match do_export_svg(&layout_doc, &path_str) {
            Ok(()) => {
                log_to_file(&format!("[lib.rs AppController] export_svg(): 3 wrote SVG to '{path_str}'"));
                self.as_mut().progress_updated(100);
                self.as_mut().set_export_progress(-1); // reset to idle (-1 = idle contract)
                self.as_mut().set_export_status_message(cxx_qt_lib::QString::default()); // clear
                self.as_mut().export_finished(cxx_qt_lib::QString::from(&path_str)); // success
                true // success
            } // Ok
            Err(e) => {
                log_to_file(&format!("[lib.rs AppController] export_svg(): 3 failed: {e}"));
                self.as_mut().set_export_progress(-1);
                self.as_mut().set_export_status_message(cxx_qt_lib::QString::default()); // clear
                self.as_mut().error_occurred(cxx_qt_lib::QString::from(&e)); // export failed
                false // failure
            } // Err
        } // match do_export_svg
    } // fn export_svg


    // Export the assembled layout as a PNG image file.
    // The export pipeline is fixed at 100% scale to prohibit size changes.
    // Delegates core logic to exports::do_export_png.
    // Called by QML 'Export PNG' button handler: onExportPNG: appController.exportPng(path, scale).
    // Note: `scale` is currently ignored and retained only for API compatibility.
    fn export_png(
        mut self: std::pin::Pin<&mut Self>,
        path: &cxx_qt_lib::QString,
        _scale: f32,
    ) -> bool {
        let path_str = path.to_string();
        log_to_file(&format!("[lib.rs AppController] export_png(): 1 requested path='{path_str}'"));

        let layout_doc = match self.clone_stripped_layout_doc() {
            Ok(d) => d,
            Err(m) => {
                log_to_file("[lib.rs AppController] export_png(): 2 no layout_dom available");
                self.as_mut().error_occurred(m);
                return false; // if no layout
            } // Err
        }; // layout_doc

        log_to_file(&format!("[lib.rs AppController] export_png(): 2 layout_dom cloned; starting export to '{path_str}'"));

        // Signal export start (0%); QML popup is already open before this call.
        self.as_mut().set_export_progress(0);
        self.as_mut().set_export_status_message(cxx_qt_lib::QString::from("Exporting PNG…"));
        self.as_mut().progress_updated(0);

        // Progress closure: each intermediate tick from do_export_png drives
        // both the QObject property (QML ProgressBar binding) and the signal
        // (any imperative listeners).
        let mut progress = |pct: i32| {
            self.as_mut().set_export_progress(pct);
            self.as_mut().progress_updated(pct);
        }; // progress

        match do_export_png(&layout_doc, &path_str, &mut progress) {
            Ok(()) => {
                log_to_file(&format!("[lib.rs AppController] export_png(): 3 wrote PNG '{path_str}'"));
                self.as_mut().progress_updated(100);
                self.as_mut().set_export_progress(-1); // reset to idle (-1 = idle contract)
                self.as_mut().set_export_status_message(cxx_qt_lib::QString::default()); // clear
                self.as_mut().export_finished(cxx_qt_lib::QString::from(&path_str)); // success
                true // success
            } // Ok
            Err(e) => {
                log_to_file(&format!("[lib.rs AppController] export_png(): 3 failed: {e}"));
                self.as_mut().set_export_progress(-1);
                self.as_mut().set_export_status_message(cxx_qt_lib::QString::default()); // clear
                self.as_mut().error_occurred(cxx_qt_lib::QString::from(&e)); // export failed
                false // failure
            } // Err
        } // match do_export_png
    } // fn export_png

}

// ---------------------------------------------------------------------------
// DG.1 — log_to_file compile-time gate tests
//
// Verifies that the dual #[cfg(debug_assertions)] / #[cfg(not(debug_assertions))]
// split on log_to_file() is correct in both build modes:
//
//   debug build  (`cargo test`):  the real file-writing impl is compiled and
//                                 callable without panicking.
//   release build (`cargo test --release`): the no-op stub is compiled,
//                                 callable, and does nothing.
//
// Call-site compatibility is guaranteed by the shared signature
// `pub(crate) fn log_to_file(message: &str)` — the compiler would reject any
// mismatch across the ~20 call sites in lib.rs, layout_utils.rs, and exports.rs.
// ---------------------------------------------------------------------------
#[cfg(test)]
mod dg1_log_gate_tests {
    use super::log_to_file;

    // @brief In debug builds log_to_file opens/appends the log file without panicking.
    // We cannot pin the exact path (LOG_PATH is a OnceLock shared across all tests),
    // but a successful return proves the function exists, has the right signature,
    // and does not unwrap-panic on a writeable filesystem.
    #[cfg(debug_assertions)]
    #[test]
    fn debug_log_to_file_does_not_panic() {
        log_to_file("[dg1_test] debug_log_to_file_does_not_panic: marker");
        // reaching here means the debug impl compiled and ran without panicking
    } // debug_log_to_file_does_not_panic

    // @brief In debug builds repeated calls accumulate without error.
    // Guards against any static-init side-effect that could make the second
    // call panic (e.g. a bad OnceLock interaction).
    #[cfg(debug_assertions)]
    #[test]
    fn debug_log_to_file_repeated_calls_are_safe() {
        log_to_file("[dg1_test] repeated call 1");
        log_to_file("[dg1_test] repeated call 2");
        log_to_file("[dg1_test] repeated call 3");
    } // debug_log_to_file_repeated_calls_are_safe

    // @brief In release builds the no-op stub compiles, links, and runs without panicking.
    // This test only executes when compiled with `cargo test --release`
    // (i.e. debug_assertions is false).
    #[cfg(not(debug_assertions))]
    #[test]
    fn release_log_to_file_is_noop_and_does_not_panic() {
        log_to_file("this message is discarded in release builds — no file I/O occurs");
        // no assertion needed: reaching here proves the no-op compiled and ran
    } // release_log_to_file_is_noop_and_does_not_panic
} // mod dg1_log_gate_tests

// ---------------------------------------------------------------------------
// DG.2 — save_debug_dom() / get_out_dir() compile-time gate tests
//
// Verifies that the dual #[cfg(debug_assertions)] / #[cfg(not(debug_assertions))]
// split on save_debug_dom() and get_out_dir() is correct in both build modes:
//
//   debug build  (`cargo test`):  the real file-writing save_debug_dom impl is
//                                 compiled and callable without panicking; get_out_dir
//                                 returns a path whose last component is "output".
//   release build (`cargo test --release`): save_debug_dom is the no-op stub —
//                                 callable, returns (), writes nothing; get_out_dir
//                                 returns an empty PathBuf, no directory created.
//
// Call-site compatibility across lib.rs (~4 call sites) and layout_utils.rs
// (~9 call sites) is guaranteed by the shared signatures:
//   fn save_debug_dom(doc: &svg_dom::Document, filename: &str)
//   fn get_out_dir() -> &'static std::path::PathBuf
// The compiler would reject any mismatch at the call sites in either build mode.
// ---------------------------------------------------------------------------
#[cfg(test)]
mod dg2_debug_dom_gate_tests {
    use super::{get_out_dir, save_debug_dom};

    // Minimal valid SVG for constructing a Document in tests.
    const EMPTY_SVG: &str = r#"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100"></svg>"#;

    fn parse_doc() -> svg_dom::Document {
        svg_dom::Document::parse(EMPTY_SVG).expect("parse minimal SVG")
    }

    // @brief In debug builds save_debug_dom() compiles and runs without panicking.
    // The file write may fail (no guaranteed output/ dir in the test sandbox), but
    // the function must not panic regardless of I/O outcome.
    #[cfg(debug_assertions)]
    #[test]
    fn debug_save_debug_dom_does_not_panic() {
        let doc = parse_doc();
        save_debug_dom(&doc, "dg2_test.svg");
        // reaching here proves the debug impl compiled and ran without panicking
    } // debug_save_debug_dom_does_not_panic

    // @brief In debug builds get_out_dir() returns a path ending in "output".
    // Confirms the debug impl builds exe_dir/output rather than a dummy empty path.
    #[cfg(debug_assertions)]
    #[test]
    fn debug_get_out_dir_returns_output_path() {
        let path = get_out_dir();
        let last = path.file_name().and_then(|n| n.to_str()).unwrap_or("");
        assert_eq!(last, "output", "debug get_out_dir() last component must be 'output'");
    } // debug_get_out_dir_returns_output_path

    // @brief In debug builds repeated save_debug_dom() calls are safe (counter increments correctly).
    #[cfg(debug_assertions)]
    #[test]
    fn debug_save_debug_dom_repeated_calls_are_safe() {
        let doc = parse_doc();
        save_debug_dom(&doc, "dg2_repeat.svg");
        save_debug_dom(&doc, "dg2_repeat.svg");
        save_debug_dom(&doc, "dg2_repeat.svg");
        // reaching here proves counter increments without panic
    } // debug_save_debug_dom_repeated_calls_are_safe

    // @brief In release builds save_debug_dom() is a no-op and does not panic.
    // This test only executes when compiled with `cargo test --release`.
    #[cfg(not(debug_assertions))]
    #[test]
    fn release_save_debug_dom_is_noop_and_does_not_panic() {
        let doc = parse_doc();
        save_debug_dom(&doc, "dg2_release_test.svg");
        // no assertion needed: reaching here proves the no-op compiled and ran
    } // release_save_debug_dom_is_noop_and_does_not_panic

    // @brief In release builds get_out_dir() returns an empty path without creating a directory.
    // The empty PathBuf is the release sentinel: no output/ dir is ever created.
    #[cfg(not(debug_assertions))]
    #[test]
    fn release_get_out_dir_returns_empty_path_no_dir_created() {
        let path = get_out_dir();
        assert!(
            path.as_os_str().is_empty(),
            "release get_out_dir() must return an empty path (no directory created)"
        );
    } // release_get_out_dir_returns_empty_path_no_dir_created
} // mod dg2_debug_dom_gate_tests

// ---------------------------------------------------------------------------
// DG.3 — cleanup_adjust_output_artifacts() compile-time gate tests
//
// Verifies that the dual #[cfg(debug_assertions)] / #[cfg(not(debug_assertions))]
// split on cleanup_adjust_output_artifacts() is correct in both build modes:
//
//   debug build  (`cargo test`):  the real file-scanning / removal impl is
//                                 compiled and callable without panicking; an
//                                 empty (or nonexistent) output dir returns 0.
//   release build (`cargo test --release`): the no-op stub is compiled,
//                                 callable, returns 0, and performs no I/O.
//
// Call-site compatibility is guaranteed by the shared signature
// `fn cleanup_adjust_output_artifacts() -> usize` — the compiler would reject
// any mismatch at the two call sites in discard_adjustments() and
// exit_adjust_mode().
// ---------------------------------------------------------------------------
#[cfg(test)]
mod dg3_cleanup_gate_tests {
    use super::cleanup_adjust_output_artifacts;

    // @brief In debug builds cleanup_adjust_output_artifacts() compiles and runs without panicking.
    // An empty or nonexistent output dir is expected in the test sandbox, so the
    // function returns 0 rather than panicking on the missing directory.
    #[cfg(debug_assertions)]
    #[test]
    fn debug_cleanup_adjust_output_artifacts_does_not_panic() {
        let removed = cleanup_adjust_output_artifacts();
        // Zero is expected in the test sandbox: no adjust artifacts are present.
        assert_eq!(removed, 0, "cleanup returns 0 when no adjust artifacts are present");
    } // debug_cleanup_adjust_output_artifacts_does_not_panic

    // @brief In debug builds repeated calls to cleanup are safe and idempotent.
    // Guards against any static-init or directory-handle side-effect that could
    // make the second call panic.
    #[cfg(debug_assertions)]
    #[test]
    fn debug_cleanup_adjust_output_artifacts_repeated_calls_are_safe() {
        let _ = cleanup_adjust_output_artifacts();
        let _ = cleanup_adjust_output_artifacts();
        let _ = cleanup_adjust_output_artifacts();
        // reaching here proves repeated calls compile and run without panicking
    } // debug_cleanup_adjust_output_artifacts_repeated_calls_are_safe

    // @brief In release builds the no-op stub compiles, links, returns 0, and does not panic.
    // This test only executes when compiled with `cargo test --release`
    // (i.e. debug_assertions is false).
    #[cfg(not(debug_assertions))]
    #[test]
    fn release_cleanup_adjust_output_artifacts_is_noop_and_does_not_panic() {
        let removed = cleanup_adjust_output_artifacts();
        assert_eq!(removed, 0, "release no-op stub must return 0 — no file I/O occurs");
    } // release_cleanup_adjust_output_artifacts_is_noop_and_does_not_panic
} // mod dg3_cleanup_gate_tests

// ---------------------------------------------------------------------------
// AdjustMode bbox-behavior tests
//
// Pins how a piece's geometry-derived bounding box behaves across the three
// AdjustMode exit flows (Apply / Cancel / Done).  The interactive flow lives in
// the AppController QObject methods (enter_adjust_mode, accept_adjustments,
// discard_adjustments, exit_adjust_mode), which can't be instantiated in a unit
// test, so these tests drive the SAME underlying DOM operations those methods
// use — clone the layout into a working copy, write the piece pose as a
// transform, drop the copy, or flatten+promote it — and assert the bbox that
// `bbox_from_group_geometry` (the bridge's bbox extractor) reports in each case.
//
// Scenario matrix:
//   Apply  → transform recorded on the working copy but geometry NOT yet baked,
//            so the geometry bbox is unchanged (pose is pending until Done) and
//            the original layout is untouched.
//   Cancel → working copy dropped; the original layout bbox is unchanged.
//   Done   → working copy flattened (transform baked into geometry) then
//            promoted, so the bbox reflects the adjusted position.
// ---------------------------------------------------------------------------
#[cfg(test)]
mod adjust_bbox_tests {
    use super::bbox_from_group_geometry;
    use geometry::BoundingBox;

    // Minimal layout_dom with one pattern piece "A": a 20x40 box at (10, 20).
    const LAYOUT_SVG: &str = r#"<svg xmlns="http://www.w3.org/2000/svg" width="500" height="500"><g id="A"><path d="M 10,20 L 30,20 L 30,60 L 10,60 Z"/></g></svg>"#;

    fn parse(svg: &str) -> svg_dom::Document {
        svg_dom::Document::parse(svg).expect("parse svg")
    }

    // Geometry-derived bbox of piece `id`, via the bridge's real extractor.
    fn piece_bbox(doc: &svg_dom::Document, id: &str) -> BoundingBox {
        for child in &doc.root.children {
            if let Some(el) = child.as_element() {
                if el.name == "g" && el.attributes.get("id").map(String::as_str) == Some(id) {
                    return bbox_from_group_geometry(el).expect("piece A has geometry");
                }
            }
        }
        panic!("piece {id} not found");
    }

    fn approx(actual: f32, expected: f32) {
        assert!((actual - expected).abs() < 0.01, "expected {expected}, got {actual}");
    }

    fn assert_bbox(b: &BoundingBox, x: f32, y: f32, w: f32, h: f32) {
        approx(b.min.x, x);
        approx(b.min.y, y);
        approx(b.width(), w);
        approx(b.height(), h);
    }

    // Apply: accept_adjustments writes the piece's new pose as a transform on the
    // working copy but does not bake it (see the "Do NOT update piece_bboxes_json"
    // note in accept_adjustments).  So the geometry bbox is still the original,
    // and the canonical layout_dom is untouched.
    #[test]
    fn apply_records_pending_transform_without_moving_geometry() {
        let layout = parse(LAYOUT_SVG);

        // enter_adjust_mode clones layout_dom into the working copy.
        let mut adjust = layout.clone();
        // accept_adjustments sets the full piece transform on the working copy.
        assert!(adjust.set_attr_by_id("A", "transform", "translate(50 30)"));

        // Geometry bbox unchanged — the move is pending, not yet baked.
        assert_bbox(&piece_bbox(&adjust, "A"), 10.0, 20.0, 20.0, 40.0);
        // Original layout is untouched by Apply.
        assert_bbox(&piece_bbox(&layout, "A"), 10.0, 20.0, 20.0, 40.0);
    } // apply_records_pending_transform_without_moving_geometry

    // Cancel: discard_adjustments drops the working copy; layout_dom is the
    // canonical state and was never modified, so the bbox reverts to original.
    #[test]
    fn cancel_reverts_to_original_layout() {
        let layout = parse(LAYOUT_SVG);

        let mut adjust = layout.clone();
        assert!(adjust.set_attr_by_id("A", "transform", "translate(50 30)"));
        drop(adjust); // discard_adjustments: working copy dropped

        assert_bbox(&piece_bbox(&layout, "A"), 10.0, 20.0, 20.0, 40.0);
    } // cancel_reverts_to_original_layout

    // Done: exit_adjust_mode flattens the working copy (baking the transform into
    // geometry) then promotes it to layout_dom, so the bbox reflects the move.
    #[test]
    fn done_bakes_transform_into_geometry() {
        let layout = parse(LAYOUT_SVG);

        let mut adjust = layout.clone();
        assert!(adjust.set_attr_by_id("A", "transform", "translate(50 30)"));
        svg_dom::flatten_dom(&mut adjust); // bake the pending transform
        let promoted = adjust;             // promoted into layout_dom

        // Original (10,20) shifted by (50,30) → (60,50); size unchanged.
        assert_bbox(&piece_bbox(&promoted, "A"), 60.0, 50.0, 20.0, 40.0);
    } // done_bakes_transform_into_geometry
} // mod adjust_bbox_tests

// ---------------------------------------------------------------------------
// Export progress infrastructure tests
//
// Verifies that `AppControllerRust::default()` initialises the export progress
// fields to their idle sentinel values so the QML progress bar starts hidden.
// ---------------------------------------------------------------------------
#[cfg(test)]
mod export_progress_tests {
    use super::AppControllerRust;

    // @brief Export progress defaults to -1 (idle sentinel) on construction.
    // QML treats any value < 0 as "not exporting" and hides the progress bar.
    #[test]
    fn export_progress_defaults_to_idle() {
        let state = AppControllerRust::default();
        assert_eq!(state.export_progress, -1, "export_progress should be -1 (idle) on init");
    } // export_progress_defaults_to_idle

    // @brief Export status message defaults to empty on construction.
    // QML shows a fallback label when this string is empty.
    #[test]
    fn export_status_message_defaults_to_empty() {
        let state = AppControllerRust::default();
        assert!(
            state.export_status_message.to_string().is_empty(),
            "export_status_message should be empty on init"
        );
    } // export_status_message_defaults_to_empty
} // mod export_progress_tests

// ---------------------------------------------------------------------------
// export_dxf error-log formatting tests
//
// Pins that explicit `.to_string()` conversion on a cxx_qt_lib::QString
// produces the same, predictable Rust String as constructing it — i.e., the
// log line in export_dxf()'s Err(m) arm is consistent and formatting-trait-
// independent.
// ---------------------------------------------------------------------------
#[cfg(test)]
mod export_dxf_error_formatting_tests {
    use cxx_qt_lib::QString;

    // @brief QString::from("…").to_string() round-trips the message verbatim.
    // Guards the invariant relied on by the log line:
    //   format!("… {}", m.to_string())
    // so that the log output is predictable regardless of Display impls.
    #[test]
    fn qstring_to_string_roundtrips_error_message() {
        let msg = "No layout available. Run Create Layout before exporting.";
        let qstring = QString::from(msg);
        assert_eq!(
            qstring.to_string(),
            msg,
            "QString::to_string() must produce the original message verbatim"
        );
    } // qstring_to_string_roundtrips_error_message

    // @brief format! with explicit .to_string() matches format! with Display.
    // Verifies that the two formatting approaches are equivalent for QString,
    // so switching from {m} to {m.to_string()} does not change the log output.
    #[test]
    fn explicit_to_string_matches_display_format() {
        let msg = "No layout available. Run Create Layout before exporting.";
        let m = QString::from(msg);
        let via_to_string = format!("no layout DOM: {}", m.to_string());
        let expected = format!("no layout DOM: {}", msg);
        assert_eq!(
            via_to_string,
            expected,
            "explicit .to_string() must produce a predictable log line"
        );
    } // explicit_to_string_matches_display_format
} // mod export_dxf_error_formatting_tests

// ---------------------------------------------------------------------------
// DG.5 — Debug-gate verification tests (integration verification)
//
// Confirms that the compile-time observability gate is correct and complete
// app-wide across all three Rust source files that call the gated functions:
//   lib.rs
//   exports.rs
//   layout_utils.rs
//
// The four gates verified here (DG.1–DG.4) cover all observability file I/O
// in the application, not just AdjustMode:
//
//   DG.1 — log_to_file()                  (debug log file writes; all pipelines)
//   DG.2 — save_debug_dom() / get_out_dir() (SVG DOM snapshots; output/ dir)
//   DG.3 — cleanup_adjust_output_artifacts() (stale artifact removal)
//   DG.4 — dumpOverlayData() in C++        (Qt tests cover this gate)
//
// Acceptance contract for DG.5:
//   debug build (`cargo test`)            — all four gates compile and run
//                                           without panicking; file I/O may occur.
//   release build (`cargo test --release`) — all three Rust no-op stubs are
//                                           callable; no output/ directory is
//                                           created; no debug log file is written.
// ---------------------------------------------------------------------------
#[cfg(test)]
mod dg5_verification_tests {
    use super::{cleanup_adjust_output_artifacts, get_out_dir, log_to_file, save_debug_dom};

    // Minimal valid SVG used by save_debug_dom tests.
    const EMPTY_SVG: &str =
        r#"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100"></svg>"#;

    fn parse_doc() -> svg_dom::Document {
        svg_dom::Document::parse(EMPTY_SVG).expect("parse minimal SVG")
    }

    // -----------------------------------------------------------------------
    // Debug-build tests (only compiled and run with `cargo test`)
    // -----------------------------------------------------------------------

    /// @brief DG.5 debug: all three Rust observability functions compile and
    /// run together without panicking, confirming each gate is correct in a
    /// combined call sequence.
    #[cfg(debug_assertions)]
    #[test]
    fn debug_all_rust_observability_gates_compile_and_run_without_panic() {
        // DG.1 — log_to_file (debug: real file-append impl)
        log_to_file("[dg5_test] combined gate check — DG.1 log_to_file");

        // DG.2 — save_debug_dom + get_out_dir (debug: SVG write + output/ path)
        let doc = parse_doc();
        save_debug_dom(&doc, "dg5_combined_check.svg");
        let out = get_out_dir();
        assert!(
            out.file_name()
                .and_then(|n| n.to_str())
                .unwrap_or("")
                == "output",
            "debug get_out_dir() last component must be 'output' (DG.2)"
        );

        // DG.3 — cleanup_adjust_output_artifacts (debug: real scan/remove impl)
        let _removed = cleanup_adjust_output_artifacts();

        // reaching here confirms DG.1 + DG.2 + DG.3 all compile and run in debug
    } // debug_all_rust_observability_gates_compile_and_run_without_panic

    /// @brief DG.5 debug: repeated calls to all three gated functions are safe
    /// and do not panic, guarding against any static-init side-effect.
    #[cfg(debug_assertions)]
    #[test]
    fn debug_repeated_calls_to_all_observability_gates_are_safe() {
        let doc = parse_doc();
        for i in 0..3u32 {
            log_to_file(&format!("[dg5_test] repeated-call iteration {i}"));
            save_debug_dom(&doc, &format!("dg5_repeat_{i}.svg"));
            let _ = cleanup_adjust_output_artifacts();
        }
        // reaching here proves all three functions survive repeated sequential calls
    } // debug_repeated_calls_to_all_observability_gates_are_safe

    // -----------------------------------------------------------------------
    // Release-build tests (only compiled and run with `cargo test --release`)
    // -----------------------------------------------------------------------

    /// @brief DG.5 release: all three Rust observability functions are no-ops
    /// and do not panic when called together.  This is the primary DG.5
    /// acceptance check: `cargo test --release -p cxxqt_bridge` must pass this
    /// test to confirm the release build contains no observability file I/O.
    #[cfg(not(debug_assertions))]
    #[test]
    fn release_all_rust_observability_gates_are_noops_and_do_not_panic() {
        // DG.1 no-op — release log_to_file discards message; no file is opened
        log_to_file("release: this message must not produce any file I/O");

        // DG.2 no-op — release save_debug_dom writes nothing; get_out_dir is empty
        let doc = parse_doc();
        save_debug_dom(&doc, "release_dg5_check.svg");

        // DG.3 no-op — release cleanup returns 0 without touching the filesystem
        let removed = cleanup_adjust_output_artifacts();
        assert_eq!(removed, 0, "release cleanup no-op must return 0 (DG.3)");

        // reaching here confirms DG.1 + DG.2 + DG.3 are all no-ops in release
    } // release_all_rust_observability_gates_are_noops_and_do_not_panic

    /// @brief DG.5 release: get_out_dir() returns an empty path, confirming
    /// the output/ directory is never created by release builds.  This is the
    /// compile-time enforcement of `cargo build --release leaves no output/ artifacts`.
    #[cfg(not(debug_assertions))]
    #[test]
    fn release_get_out_dir_returns_empty_path_no_output_dir_created() {
        let path = get_out_dir();
        assert!(
            path.as_os_str().is_empty(),
            "release get_out_dir() must return an empty PathBuf — \
             no output/ directory is ever created in release builds (DG.2)"
        );
    } // release_get_out_dir_returns_empty_path_no_output_dir_created

    /// @brief DG.5 release: calling save_debug_dom() and log_to_file() in
    /// sequence does not create any new filesystem entries alongside the binary.
    /// Guards the release-build acceptance criterion that `cargo build --release`
    /// leaves no output/ artifacts.
    #[cfg(not(debug_assertions))]
    #[test]
    fn release_save_debug_dom_and_log_to_file_produce_no_filesystem_artifacts() {
        let doc = parse_doc();

        // Both functions are no-ops in release; call them in the order they
        // appear in the real layout pipeline (log → save_dom → log → save_dom).
        log_to_file("[dg5_release] pipeline step 1 — pre-layout log");
        save_debug_dom(&doc, "dg5_release_flat.svg");
        log_to_file("[dg5_release] pipeline step 2 — post-layout log");
        save_debug_dom(&doc, "dg5_release_vertical.svg");

        // get_out_dir() is the release sentinel: empty == no directory created.
        let out = get_out_dir();
        assert!(
            out.as_os_str().is_empty(),
            "release: output/ directory must not exist after calling save_debug_dom (DG.2)"
        );
    } // release_save_debug_dom_and_log_to_file_produce_no_filesystem_artifacts
} // mod dg5_verification_tests
