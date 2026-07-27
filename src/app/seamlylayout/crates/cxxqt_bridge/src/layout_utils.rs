// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file layout_utils.rs
// @brief Pure layout logic for initialize_layout and process_layout.
//
// Each function receives plain Rust types and returns Results — no CXX-Qt,
// no Pin<&mut Self>, no signal machinery. The thin wrappers in lib.rs handle
// all Qt integration (property updates, signal emission, state storage).
//
// Exports:
//   do_initialize_layout(settings_json, input_dom) -> Result<InitLayoutResult, String>
//   do_process_layout(args)                        -> Result<ProcessLayoutResult, String>

use svg_dom::Document;

use layout_tiling::{
    compute_tile_dims, create_initial_tiled_layout_dom, measurement_to_px,
    pick_best_tiled_candidate, widest_piece_tile_cols, LayoutSettings, TileDimensions,
};

use crate::piece_extractor::{extract_piece_rects_and_polygons, hoist_tagged_pieces};
use crate::layout_assembler::{create_layout, create_initial_layout_dom, trim_bottom};
use crate::save_debug_dom;
use crate::log_to_file;

/// @brief Result returned by do_initialize_layout on success.
pub struct InitLayoutResult {
    /// The initial layout DOM (blank canvas with backgroundRect + contentRect).
    pub initial_dom: Document,
    /// Layout width in pixels, parsed from the DOM's root width attribute.
    pub w_px: u32,
    /// Layout height in pixels, parsed from the DOM's root height attribute.
    pub h_px: u32,
}

/// @brief Result returned by do_process_layout on success.
pub struct ProcessLayoutResult {
    /// The assembled layout DOM with all pieces placed.
    pub output_doc: Document,
    /// JSON string with piece bounding boxes and margin metadata.
    pub bbox_json: String,
    /// Updated layout height after trimming (may differ from input).
    pub layout_h_px: u32,
    /// Left margin in pixels (from contentRect x attribute).
    pub ml_px: u32,
    /// Top margin in pixels (from contentRect y attribute).
    pub mt_px: u32,
    /// User-facing ids of pieces that could not be placed in the layout.
    /// Empty on a fully successful pack.  Non-empty when the (non-tiled) packer
    /// ran out of room or a piece was larger than the sheet: those pieces are
    /// omitted from the rendered layout and reported to the user as a warning,
    /// rather than aborting the whole layout with a hard error.
    pub unplaced_labels: Vec<String>,
    /// Pre-processing stage snapshots, persisted on the controller so later
    /// steps (e.g. AdjustMode) can read them from memory instead of re-parsing
    /// the debug SVGs from disk.  See the matching fields on AppControllerRust.
    ///
    /// `flat_dom` is the fully pre-processed DOM (after the final flatten) — the
    /// source used for piece extraction and placement.  `vertical_dom` is the
    /// verticalized snapshot; `translate_dom` is the translated snapshot.
    pub flat_dom: Document,
    pub vertical_dom: Document,
    pub translate_dom: Document,
}

/// @brief Progress callback type for do_process_layout.
/// Called with a percentage (0–100) at each major pipeline stage.
pub type ProgressFn<'a> = &'a mut dyn FnMut(i32, Option<&str>);

/// @brief Build the initial layout DOM from settings JSON.
///
/// Parses layout settings, creates either a tiled or single-sheet canvas,
/// and returns the DOM with its pixel dimensions.
///
/// When paper_type is "tiled", uses input_dom dimensions to compute tile
/// layout; input_dom must be Some in that case.
///
/// # Errors
/// Returns a descriptive error string if settings parsing fails or tile
/// dimension computation fails.
pub fn do_initialize_layout(
    settings_json: &str,
    input_dom: Option<&Document>,
) -> Result<InitLayoutResult, String> {

    log_to_file(&format!("==========INITIALIZE LAYOUT=========="));
    log_to_file(&format!("[debug] layout_utils::do_initialize_layout: 1 Starting initialize_layout with settings: {}", settings_json));

    // --- 1 parse settings ---
    let settings = LayoutSettings::from_json(settings_json)
        .map_err(|e| format!("[debug] layout_utils::do_initialize_layout: 2 Failed to parse layout settings: {e}"))?;

    // --- 2 create initial_dom ---
    let initial_dom: Document = if settings.paper_type == "tiled" {
        log_to_file("[debug] layout_utils::do_initialize_layout:: 3 Creating tiled layout with input SVG dimensions and tile settings.");
        // 2a get input_dom dimensions
        let doc = input_dom.ok_or("[debug] layout_utils::do_initialize_layout: 4 input_dom is required for tiled layout")?;
        let w = doc.root.attributes.get("width").map(String::as_str).unwrap_or("0");
        let h = doc.root.attributes.get("height").map(String::as_str).unwrap_or("0");
        let (input_w_px, input_h_px) = (measurement_to_px(w), measurement_to_px(h));

        // 2b get trimmed tile dimensions
        let tile_dims = compute_tile_dims(input_w_px, input_h_px, &settings)
            .map_err(|e| format!("[debug] layout_utils::do_initialize_layout: 5 Failed to compute tile dimensions for tiled layout: {e}"))?;

        log_to_file(&format!(
            "[debug] layout_utils::do_initialize_layout: 6 Computed tile dimensions: tile_w_px={}, tile_h_px={}. Layout dimensions: layout_w_px={}, layout_h_px={}.",
            tile_dims.trim_tile_w_px, tile_dims.trim_tile_h_px, tile_dims.layout_w_px, tile_dims.layout_h_px
        ));

        // 2c return initial_dom with tiles and layout dimensions
        create_initial_tiled_layout_dom(&tile_dims)
    } else {
        create_initial_layout_dom(&settings)
    }; // if settings.paper_type=="tiled"

    // --- 3 extract dimensions ---
    let w = initial_dom.root.attributes.get("width")
        .and_then(|s| s.parse::<u32>().ok()).unwrap_or(0);
    let h = initial_dom.root.attributes.get("height")
        .and_then(|s| s.parse::<u32>().ok()).unwrap_or(0);

    // --- 4 save debug file ---
    save_debug_dom(&initial_dom, "initial_layout.svg");

    Ok(InitLayoutResult { initial_dom, w_px: w, h_px: h })
} // fn do_initialize_layout

/// @brief Arguments for do_process_layout.
pub struct ProcessLayoutArgs<'a> {
    /// Layout settings JSON string from QML.
    pub settings_json: &'a str,
    /// The imported SVG DOM (input_dom).
    pub input_dom: &'a Document,
    /// The blank canvas DOM snapshot (initial_layout_dom).
    pub initial_layout_dom: &'a Document,
    /// Current layout height in pixels.
    pub layout_h_px: u32,
}

/// @brief Run the full layout pipeline: preprocess, pack, assemble, trim.
///
/// Pipeline stages:
///   1. Parse settings, extract bin dimensions from initial_layout_dom
///   2. Preprocess pieces: flatten → verticalize → flatten → translate → flatten
///   3. Extract piece rectangles
///   4. Pack pieces into the bin with MaxRects
///   5. Assemble output DOM
///   6. Trim unused space (roll/fabric) or empty tile rows (tiled)
///   7. Flatten output, build piece bbox JSON
///
/// Calls progress_fn at each major stage with a percentage (20, 40, 60, 100).
///
/// # Errors
/// Returns a descriptive error string on any failure (settings parse, no input,
/// no pieces found, piece too large, no space, etc.).
pub fn do_process_layout(
    args: ProcessLayoutArgs<'_>,
    progress_fn: ProgressFn<'_>,
) -> Result<ProcessLayoutResult, String> {

    log_to_file(&format!("==========PROCESS LAYOUT=========="));

    // --- 1 parse settings and compute bin dimensions ---

    let settings = LayoutSettings::from_json(args.settings_json)
        .map_err(|e| format!("[ERROR] layout_utils::do_process_layout(): 1 Failed to parse layout settings: {e}"))?;

    // Build the per-piece rotation trial set from layoutMode + rotationStep.
    // Passed to every packing call below; the dispatcher in `packing` routes
    // to the rectangle packer (when ⊆ {0, 180}) or the polygon-tight stub.
    let trial_angles_deg: Vec<u16> = settings.rotation_trial_set_deg();

    // get "packing bin area" dimensions (w, h) and origin (x, y) from layout_dom's contentRect
    let layout_h_px = args.layout_h_px;

    let bin_w: u32 = args.initial_layout_dom
        .get_attr_by_id("contentRect", "width")
        .and_then(|s| s.parse::<u32>().ok())
        .unwrap_or(0);
    let bin_h: u32 = args.initial_layout_dom
        .get_attr_by_id("contentRect", "height")
        .and_then(|s| s.parse::<u32>().ok())
        .unwrap_or(0);
    // get margin left and top values
    let ml_px: u32 = args.initial_layout_dom
        .get_attr_by_id("contentRect", "x")
        .and_then(|s| s.parse::<u32>().ok())
        .unwrap_or(0);
    let mt_px: u32 = args.initial_layout_dom
        .get_attr_by_id("contentRect", "y")
        .and_then(|s| s.parse::<u32>().ok())
        .unwrap_or(0);
    let mb_px: u32 = (layout_h_px - mt_px) - bin_h;
    // margin right (mr_px) isn't used in these calculations

    // --- 2 clone input_dom for piece pre-processing ---
    let mut input_dom_clone = args.input_dom.clone();

    // 2a: normalise the Seamly2D handoff shape.
    //
    // Layout Mode nests every piece inside one `<g data-type="pattern">`; every
    // stage below treats a direct `<g>` child of the root as one piece, so
    // without this the packer receives the whole pattern as a single sheet-sized
    // object and places nothing (Task 59).  Untagged drawings are left alone.
    // Done on the clone so the imported document the user sees is untouched.
    let hoisted = hoist_tagged_pieces(&mut input_dom_clone);
    if hoisted > 0 {
        log_to_file(&format!(
            "[debug] layout_utils::do_process_layout(): 1a hoisted {} tagged piece(s) out of their pattern wrapper",
            hoisted
        ));
    } // if hoisted

    // --- 3 pre-process pieces in input_dom_clone ---
    // Pipeline: flatten → verticalize → flatten → translate → flatten
    // Each conceptual stage produces a snapshot DOM.  The verticalized and
    // translated snapshots (and the final flattened DOM) are returned so the
    // controller can persist them in its flat_dom / vertical_dom / translate_dom
    // fields — later steps (AdjustMode) read those from memory instead of
    // re-parsing the debug SVGs from disk.  Each stage still writes an
    // intermediate SVG to output/ for debugging.

    // 3a: save the raw imported DOM before any pre-processing.
    log_to_file(&format!("[debug] layout_utils::do_process_layout(): 2 Create input dom - input_dom.svg"));
    save_debug_dom(&input_dom_clone, "input_dom.svg");

    // 3b: flatten — bake all transforms in input_dom_clone (except text or tspan elements).
    log_to_file(&format!("[debug] layout_utils::do_process_layout(): 3 Flatten input dom to flat1_dom.svg"));
    let mut flat_dom = input_dom_clone.clone();
    svg_dom::flatten_dom(&mut flat_dom);
    save_debug_dom(&flat_dom, "flat1_dom.svg");

    // 3c: verticalize — rotate each piece so its grainline is vertical, pieces become axis-aligned.
    // Move the flatten-1 DOM into vertical_dom (no clone) and verticalize it in place;
    // vertical_dom is retained as the stage snapshot.
    log_to_file(&format!("[debug] layout_utils::do_process_layout(): 4 Verticalize flat1 dom to vertical_dom.svg"));
    let mut vertical_dom = flat_dom;
    svg_dom::verticalize_dom(&mut vertical_dom);
    save_debug_dom(&vertical_dom, "vertical_dom.svg");

    // 3d: flatten again — bake in rotation transforms from verticalize (not text or tspan).
    // Clone vertical_dom (which we keep) into the working flatten DOM.
    log_to_file(&format!("[debug] layout_utils::do_process_layout(): 5 Flatten vertical dom to flat2_dom.svg"));
    let mut flat_dom = vertical_dom.clone();
    svg_dom::flatten_dom(&mut flat_dom);
    save_debug_dom(&flat_dom, "flat2_dom.svg");

    // 3e: translate — move each piece's axis-aligned bbox (AABB) min corner up to origin (0,0).
    // Move the flatten-2 DOM into translate_dom (no clone) and translate it in place;
    // translate_dom is retained as the stage snapshot.
    log_to_file(&format!("[debug] layout_utils::do_process_layout(): Translate flat2 dom to translate_dom.svg"));
    let mut translate_dom = flat_dom;
    svg_dom::translate_dom(&mut translate_dom);
    save_debug_dom(&translate_dom, "translate_dom.svg");

    // 3f: flatten again — bake in translation transforms from translated_dom (not text or tspan).
    // Clone translate_dom (which we keep) into the final pre-processed DOM (flat_dom).
    log_to_file(&format!("layout_utils::do_process_layout(): 7 Flatten translate dom to flat3_dom.svg"));
    let mut flat_dom = translate_dom.clone();
    svg_dom::flatten_dom(&mut flat_dom);
    save_debug_dom(&flat_dom, "flat3_dom.svg");

    // 3g: emit progress — pre-processing is first major step so progress set to 20% here.
    progress_fn(20, None); // pre-processing complete

    // --- 4 extract pre-processed pattern pieces from input_clone_dom ---

    // Extract per-piece AABB rects AND cutline polygons in a single walk so
    // the two slices stay index-aligned (a `packing::pack_polygons` precondition).
    // Polygons are used by the non-orthogonal trial-set branch inside
    // `pack_polygons`; for orthogonal trial sets the polygons are ignored
    // and the call routes to MaxRects on `rects` alone.
    let (pieces, polygons) = extract_piece_rects_and_polygons(&flat_dom);
    if pieces.is_empty() {
        return Err(
            "[ERROR] layout_utils::do_process_layout(): 8 No pattern pieces found in the imported SVG. \
             A Seamly2D handoff is read from its <g data-type=\"piece\"> groups; any other SVG has \
             each top-level <g> element treated as one piece.".to_string()
        );
    } // if pieces.is_empty

    progress_fn(40, None); // extraction stage complete (40% done, no status text)

    // --- 5 place pre-processed pieces in temporary output_doc ---

    // Gap between placed pieces, sourced from `LayoutSettings::piece_gap_px()`
    // (was `const GAP_PX: u32 = 5` before the Settings dialog field landed).
    // The user enters `pieceGap` in their active unit (in / mm / cm) via
    // SettingsDialog; the C++ model stores user-units and the Rust side
    // converts to pixels at LAYOUT_PPI here.
    let gap_px: u32 = settings.piece_gap_px();

    // Extract the rectangles for packing: piece width and height are derived from the
    // pre-processed DOM, so they reflect all transforms (rotate/verticalize, translate, etc)
    // and are in layout pixels matching the layout DOM's coordinate system.
    let rects: Vec<packing::Rect> = pieces.iter().map(|p| p.rect).collect();

    // Tiled paper uses a candidate-width search: several bin widths (in multiples
    // of the trimmed tile width) are packed and the best result by tile-count
    // (then unused area) wins.  Non-tiled paths use a single pack_maxrects call
    // against the fixed bin from the initial layout DOM.
    let is_tiled = settings.media_type == "paper" && settings.paper_type == "tiled";

    // Working values produced by the chosen packing branch:
    //   placements       — piece placements in bin-local pixel coordinates
    //   output_doc       — layout_dom we hand to create_layout (tiled may rebuild it)
    //   work_bin_h       — bin height used for trim math below
    //   work_ml_px       — left margin in pixels (unchanged from initial; tiled rebuilds reuse it)
    //   work_mt_px       — top margin in pixels  (same)
    //   work_mb_px       — bottom margin in pixels (same)
    //   work_layout_h_px — SVG root height for this run
    let (
        placements,
        mut output_doc,
        work_bin_h,
        work_ml_px,
        work_mt_px,
        work_mb_px,
        mut work_layout_h_px,
        unplaced_labels,
    ) = if is_tiled {
        // --- 5a tiled branch: run candidate-width search ---
        //
        // Rotation seam: when rotation is enabled the rotation trial happens
        // inside pack_maxrects (called from pick_best_tiled_candidate), so the
        // code here does not need to change.  See docs/layout-docs/LAYOUT_ROTATION_PLAN.md.

        // Recover trim tile dimensions and margins from the settings (the initial
        // layout_dom was built from these; recompute rather than re-parse the DOM).
        let initial_td = compute_tile_dims(
            args.initial_layout_dom.root.attributes.get("width")
                .map(String::as_str).map(measurement_to_px).unwrap_or(0),
            args.initial_layout_dom.root.attributes.get("height")
                .map(String::as_str).map(measurement_to_px).unwrap_or(0),
            &settings,
        ).map_err(|e| format!("Failed to recompute tile dimensions for pack: {e}"))?;

        let trim_w = initial_td.trim_tile_w_px;
        let trim_h = initial_td.trim_tile_h_px;
        if trim_w == 0 || trim_h == 0 {
            return Err(format!(
                "Invalid trimmed tile dimensions (trim_w={trim_w}, trim_h={trim_h}). \
                 Check tile size and margins."
            ));
        } // if invalid trim dims

        // Search window.
        //   floor = smallest column count that can hold the widest piece.
        //   ceil  = the initial layout's tile_cols — never produce a WORSE layout
        //           than the user already sees.
        let floor_cols = widest_piece_tile_cols(&rects, trim_w, gap_px);
        let ceil_cols  = initial_td.tile_cols.max(floor_cols);

        // Log the search range for diagnostics — narrow inputs or bad settings
        // can collapse floor==ceil, making the picker a no-op.
        log_to_file(&format!(
            "[process_layout] tiled candidate search: trim_w={}, trim_h={}, floor={}, ceil={}",
            trim_w, trim_h, floor_cols, ceil_cols
        ));

        // Run the picker.  Any pack error propagates with a readable message.
        let choice = pick_best_tiled_candidate(&rects, trim_w, trim_h, floor_cols, ceil_cols, gap_px, &trial_angles_deg)
            .map_err(|e| match e {
                packing::PackError::TooLarge { id, w, h, bin_w, bin_h } => {
                    let label = pieces.get(id).map(|p| p.label()).unwrap_or("?");
                    format!(
                        "Piece \"{label}\" ({w}\u{d7}{h} px) is larger than the widest \
                         tiled-bin candidate ({bin_w}\u{d7}{bin_h} px). \
                         Try a larger tile size or reduce margins."
                    )
                }, // TooLarge
                packing::PackError::NoSpace { id } => {
                    let label = pieces.get(id).map(|p| p.label()).unwrap_or("?");
                    format!(
                        "Not enough tiled-bin space to place piece \"{label}\". \
                         Try a larger tile size, reduce margins, or remove pieces."
                    )
                }, // NoSpace
                packing::PackError::SearchLimit { id } => {
                    let label = pieces.get(id).map(|p| p.label()).unwrap_or("?");
                    format!(
                        "Search limit reached while placing piece \"{label}\". \
                         The rotate solver hit runtime/complexity guardrails before finishing this layout. \
                         Try reducing piece gap, simplifying input geometry, or using a less expensive rotation mode."
                    )
                } // SearchLimit
            })?; // map_err

        log_to_file(&format!(
            "[process_layout] tiled winner: tile_cols={}, tile_rows={}, non_empty_tiles={}, unused_px2={}",
            choice.tile_cols, choice.tile_rows, choice.non_empty_tiles, choice.unused_area_px2
        ));

        // Rebuild the tiled canvas at the winner's dimensions.  `input_dom_w/h_px`
        // in TileDimensions is the <svg> width/height; for the winner it equals
        // the full tile grid plus margins.
        let new_layout_w_px = choice.tile_cols * trim_w
            + initial_td.margin_left_px + initial_td.margin_right_px;
        let new_layout_h_px = choice.tile_rows * trim_h
            + initial_td.margin_top_px + initial_td.margin_bottom_px;

        let winning_td = TileDimensions {
            tile_cols:        choice.tile_cols,
            tile_rows:        choice.tile_rows,
            trim_tile_w_px:   trim_w,
            trim_tile_h_px:   trim_h,
            input_dom_w_px:   new_layout_w_px,
            input_dom_h_px:   new_layout_h_px,
            layout_w_px:      new_layout_w_px,
            layout_h_px:      new_layout_h_px,
            margin_left_px:   initial_td.margin_left_px,
            margin_right_px:  initial_td.margin_right_px,
            margin_top_px:    initial_td.margin_top_px,
            margin_bottom_px: initial_td.margin_bottom_px,
        }; // TileDimensions

        let out_doc = create_initial_tiled_layout_dom(&winning_td);

        let new_bin_h = choice.tile_rows * trim_h;
        let new_ml    = initial_td.margin_left_px;
        let new_mt    = initial_td.margin_top_px;
        let new_mb    = initial_td.margin_bottom_px;

        progress_fn(60, None); // packing complete

        // Tiled path keeps strict semantics: the candidate-width search grows
        // the bin until the widest piece fits, so a genuinely unplaceable piece
        // still surfaces as a hard error above.  No unplaced pieces to report here.
        (choice.placements, out_doc, new_bin_h, new_ml, new_mt, new_mb, new_layout_h_px, Vec::new())
    } else {
        // --- 5b non-tiled branch: single pack against the fixed bin ---
        //
        // `pack_polygons` is the dispatcher entry point that auto-routes:
        //   * orthogonal trial set ({0, 180}) → MaxRects (polygons ignored)
        //   * non-orthogonal trial set       → polygon-tight NFP packer
        // Index-aligned `polygons[i]` / `rects[i]` is the precondition; both
        // come from `extract_piece_rects_and_polygons` above.
        let total_polygon_verts: usize = polygons.iter().map(|p| p.vertices.len()).sum();
        log_to_file(&format!(
            "[process_layout] non-tiled pack_polygons call: pieces={}, polygon_verts={}, gap_px={}, bin={}x{}, trial_set={:?}",
            pieces.len(), total_polygon_verts, gap_px, bin_w, bin_h, trial_angles_deg,
        ));
        let t_pack = std::time::Instant::now();
        let mut piece_status = |current_piece_1_based: usize, total_pieces: usize| {
            let status = format!(
                "Processing piece {} of {}...",
                current_piece_1_based,
                total_pieces
            );
            progress_fn(60, Some(status.as_str()));
        };
        // Lenient pack: place every piece that fits, skip the rest, and collect
        // the unplaced ids.  This replaces the previous hard-error behavior — a
        // piece too large for the sheet, or one that runs out of room, no longer
        // aborts the whole layout.  The layout renders the pieces that fit and
        // the unplaced piece ids are surfaced to the user as a warning popup.
        let (placements, _created_rects, unplaced_ids) = packing::pack_polygons_lenient(
            bin_w,
            bin_h,
            gap_px,
            &polygons,
            &rects,
            &trial_angles_deg,
            Some(&mut piece_status),
        );

        // Map the unplaced original indices to their user-facing piece labels
        // (`data-name` where the handoff supplied one, so the warning reads
        // "Front Bodice" rather than "piece-7").
        let unplaced_labels: Vec<String> = unplaced_ids
            .iter()
            .map(|&i| pieces.get(i).map(|p| p.label().to_string()).unwrap_or_else(|| format!("#{i}")))
            .collect();

        log_to_file(&format!(
            "[process_layout] non-tiled pack_polygons_lenient returned in {} ms with {} placements, {} unplaced: {:?}",
            t_pack.elapsed().as_millis(), placements.len(), unplaced_labels.len(), unplaced_labels,
        ));

        progress_fn(60, None); // packing complete

        // Non-tiled: keep the initial canvas and margins unchanged.
        (placements, args.initial_layout_dom.clone(), bin_h, ml_px, mt_px, mb_px, layout_h_px, unplaced_labels)
    }; // let (placements, ...) = if is_tiled

    // --- 6 create layout ---
    //
    // Seamly2D SVG coordinates are in CSS pixels (1 user-unit = 1 px at 96 dpi).
    // The pre-processing pipeline preserves pixel units, so flat_dom path data
    // is already in layout pixels. No scale factor or scale_dom call is needed.
    create_layout(
        &mut output_doc,
        &flat_dom,
        &pieces,
        &placements,
        work_ml_px,
        work_mt_px,
    );

    // --- 7 cleanup layout ---
    //
    // Tiled canvases are sized to the winner's tile_cols × tile_rows already,
    // so row-trim here is a no-op in the common case.  The existing logic is
    // retained to handle edge cases (stale rows from a re-pack) and the
    // non-tiled roll/fabric trim path.

    // TODO: expose the minimum threshold as a LayoutSettings field
    const MIN_THRESHOLD_PX: u32 = 48; // 48px=0.5inch at 96 dpi

    // get the bottom edge of the lowest piece in the bin
    let max_bin_bottom = placements.iter().map(|p| p.y + p.h).max().unwrap_or(0);
    // compute the blank space below the lowest piece
    let blank_bottom = work_bin_h.saturating_sub(max_bin_bottom);

    if blank_bottom > MIN_THRESHOLD_PX {
        // trim roll and fabric layouts
        let is_roll = settings.media_type == "fabric" || settings.paper_type == "roll";
        if is_roll {
            // 7a. trim blank space below last piece for fabric or roll media
            trim_bottom(&mut output_doc, max_bin_bottom, work_mt_px, work_mb_px);
            work_layout_h_px = max_bin_bottom.saturating_add(work_mb_px);
        } // end if is_roll

        // 7b trim tiled layout
        // Only runs when media is paper and paper_type is tiled — matches the
        // <g id="tiledRects"> structure produced by create_initial_tiled_layout_dom.
        // Reuses the is_tiled flag from the packing branch above.
        if is_tiled {
            // Derive tile dimensions before taking the mutable borrow on tiledRects.
            let out_w = output_doc.root.attributes.get("width").map(String::as_str).unwrap_or("0");
            let out_h = output_doc.root.attributes.get("height").map(String::as_str).unwrap_or("0");
            let tile_dims = compute_tile_dims(measurement_to_px(out_w), measurement_to_px(out_h), &settings)
                .map_err(|e| format!("Failed to compute tile dims for trimming: {e}"))?;
            let trim_tile_h_px = tile_dims.trim_tile_h_px;

            // Phase 1: pop empty row <path> children from <g id="tiledRects">.
            // Each child <path> is one full tile row (one `M x,y` per column).
            // A row is empty when its top y is at or below the lowest piece bottom.
            let mut rows_removed: u32 = 0;
            if let Some(tiled_rects_group) = output_doc.get_element_by_id_mut("tiledRects") {
                loop {
                    if tiled_rects_group.children.is_empty() { break; }
                    // parse y from d="M x,y M x,y ..." → nth(1)="x,y" → split(',') → nth(1)="y"
                    let last_row_y = tiled_rects_group.children.last()
                        .and_then(|node| node.as_element())
                        .and_then(|el| el.attributes.get("d"))
                        .and_then(|d| d.split_whitespace().nth(1)
                            .and_then(|xy| xy.split(',').nth(1))
                            .and_then(|y| y.parse::<u32>().ok()))
                        .unwrap_or(0);
                    if max_bin_bottom > last_row_y { break; } // row is not empty
                    tiled_rects_group.children.pop();
                    rows_removed += 1;
                }
            } // mutable borrow on output_doc released here

            // Phase 2: apply height reductions once, now that the mutable borrow is released.
            if rows_removed > 0 {
                let trim_total = (trim_tile_h_px as u32).saturating_mul(rows_removed);
                work_layout_h_px = work_layout_h_px.saturating_sub(trim_total);
                if let Some(val) = output_doc.get_attr_by_id("backgroundRect", "height") {
                    if let Ok(orig) = val.parse::<i32>() {
                        output_doc.set_attr_by_id("backgroundRect", "height",
                            &(orig - trim_total as i32).to_string());
                    }
                }
                if let Some(val) = output_doc.get_attr_by_id("contentRect", "height") {
                    if let Ok(orig) = val.parse::<i32>() {
                        output_doc.set_attr_by_id("contentRect", "height",
                            &(orig - trim_total as i32).to_string());
                    }
                }
                if let Some(val) = output_doc.root.attributes.get("height") {
                    if let Ok(orig) = val.parse::<i32>() {
                        let new_h = orig - trim_total as i32;
                        output_doc.root.attributes.insert("height".to_string(), new_h.to_string());
                    }
                }
            }
        } // end if is_tiled
    } // end if blank_bottom > MIN_THRESHOLD_PX

    // 7c. Flatten output_doc ---
    // create_layout() sets transform="translate(tx ty)" on each piece group,
    // flattening is required for DXF export which needs absolute coordinates.
    svg_dom::flatten_dom(&mut output_doc);
    save_debug_dom(&output_doc, "layout_dom_flat.svg");

    // --- 8 re-build piece bbox JSON to prep for potential manual 'Adjust Layout' ---

    // All coordinates are in layout pixels.
    // (piece.x, piece.y) = piece bbox upper left corner in layout_dom's coordinate space.
    let bbox_json = {
        let piece_arr: Vec<serde_json::Value> = placements.iter().map(|p| {
            let piece  = pieces.get(p.id);
            let id     = piece.map(|pc| pc.id.as_str()).unwrap_or("");
            let name   = piece.map(|pc| pc.name.as_str()).unwrap_or("");
            let letter = piece.map(|pc| pc.letter.as_str()).unwrap_or("");
            let label  = piece.map(|pc| pc.label()).unwrap_or("");
            let ox     = piece.map(|pc| pc.origin_x).unwrap_or(0.0);
            let oy     = piece.map(|pc| pc.origin_y).unwrap_or(0.0);
            serde_json::json!({
                "id":           id,                // machine identity — never shown to the user
                "name":         name,              // data-name from the Seamly2D handoff ("" when untagged)
                "letter":       letter,            // data-letter from the handoff ("" when unset)
                "label":        label,             // what the Adjust overlay displays: name → letter → id
                "x":            work_ml_px + p.x, // piece absolute canvasspace-x position
                "y":            work_mt_px + p.y, // piece absolute canvasspace-y position
                "w":            p.w,              // piece width
                "h":            p.h,              // piece height
                "origin_x_px":  ox,               // piece localspace-x position = 0
                "origin_y_px":  oy,               // piece localspace-y position = 0
            })
        }).collect();
        let meta = serde_json::json!({
            "ml_px":  work_ml_px,
            "mt_px":  work_mt_px,
            "pieces": piece_arr,
        });
        serde_json::to_string(&meta)
            .unwrap_or_else(|_| r#"{"ml_px":0,"mt_px":0,"pieces":[]}"#.to_string())
    }; // bbox_json

    // --- 9 save debug layout ---
    save_debug_dom(&output_doc, "layout_dom.svg");

    progress_fn(100, None); // assembly complete

    Ok(ProcessLayoutResult {
        output_doc,
        bbox_json,
        layout_h_px: work_layout_h_px,
        ml_px:       work_ml_px,
        mt_px:       work_mt_px,
        unplaced_labels,
        flat_dom,
        vertical_dom,
        translate_dom,
    })
} // fn do_process_layout

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief The real Seamly2D handoff for the Richmond shirt: 12 pieces nested
    // inside one `<g id="pattern-1" data-type="pattern">`.
    //
    // Produced with the headless export the Task 49 / 59 checks use:
    //   seamly2d.exe input/richmond-shirt_v1_v061-test.sm2d \
    //       -b handoff -d <dir> -f 0 --exportOnlyDetails
    //
    // It lives in this crate's `test_data/` and NOT in the app's `input/`
    // directory: `src/app/seamlylayout/.gitignore` ignores `/input`, so a fixture
    // placed there would be missing from a fresh clone and this test would fail
    // to compile on CI.  Embedded at compile time so the test needs no runtime
    // path resolution and behaves identically on every runner.
    const HANDOFF_SVG: &str = include_str!("../test_data/richmond-shirt-handoff_pieces.svg");

    // @brief Layout settings for the end-to-end pack: a wide fabric roll, which
    // is the media the handoff is meant for and gives the packer room for all 12.
    fn fabric_roll_settings_json() -> &'static str {
        r#"{
            "unit": "in",
            "mediaType": "fabric",
            "paperType": "roll",
            "pageWidth": 60.0,
            "pageHeight": 300.0,
            "marginLeft": 0.5,
            "marginRight": 0.5,
            "marginTop": 0.5,
            "marginBottom": 0.5,
            "pieceGap": 0.125,
            "layoutMode": "alongGrainline",
            "rotationStep": 180,
            "tileSize": "Letter",
            "tileOrientation": "Portrait"
        }"#
    } // fn fabric_roll_settings_json

    // @brief Task 59, end to end: the handoff must pack as 12 individual pieces.
    //
    // Before the fix `extract_piece_rects` saw the pattern wrapper as the only
    // top-level `<g>`, so the packer was handed one sheet-sized object and logged
    // `0 placements, 1 unplaced: ["pattern-1"]`.  This drives the whole public
    // pipeline — `do_initialize_layout` then `do_process_layout` — against the
    // genuine exporter output, which is the only way to catch a regression in the
    // hoist, in discovery, or in any stage that assumes the flat shape.
    #[test]
    fn richmond_shirt_handoff_packs_twelve_individual_pieces() {
        let input_dom = Document::parse(HANDOFF_SVG).expect("handoff fixture should parse");

        // Sanity-check the fixture itself, so a bad copy fails loudly here rather
        // than looking like a packing regression.
        assert_eq!(
            crate::piece_extractor::count_tagged_pieces(&input_dom), 12,
            "fixture should carry 12 data-type=\"piece\" groups"
        );

        let settings_json = fabric_roll_settings_json();
        let init = do_initialize_layout(settings_json, Some(&input_dom))
            .expect("initialize_layout should succeed");

        let mut progress_calls = 0;
        let mut progress = |_pct: i32, _status: Option<&str>| { progress_calls += 1; };

        let result = do_process_layout(
            ProcessLayoutArgs {
                settings_json,
                input_dom: &input_dom,
                initial_layout_dom: &init.initial_dom,
                layout_h_px: init.h_px,
            },
            &mut progress,
        ).expect("process_layout should succeed on the tagged handoff");

        // Every piece placed, none reported unplaced.
        assert!(
            result.unplaced_labels.is_empty(),
            "no piece should be left unplaced, got {:?}", result.unplaced_labels
        );

        // The bbox JSON is the layout's per-piece record; 12 entries means 12
        // separate placements, not one pattern-sized blob.
        let bbox: serde_json::Value =
            serde_json::from_str(&result.bbox_json).expect("bbox_json should be valid JSON");
        let placed = bbox["pieces"].as_array().expect("pieces array");
        assert_eq!(placed.len(), 12, "expected 12 placed pieces");

        // Identity reached the layout: names, not ids (Task 59's last subtask).
        let names: Vec<&str> = placed.iter()
            .filter_map(|p| p["name"].as_str())
            .collect();
        assert!(names.contains(&"Front"), "piece names should reach the layout, got {names:?}");
        assert!(names.contains(&"Back"),  "piece names should reach the layout, got {names:?}");
        assert!(
            !placed.iter().any(|p| p["label"].as_str() == Some("pattern-1")),
            "the pattern wrapper must never appear as a placed piece"
        );

        // No two pieces may share a slot — a single stacked position would mean
        // the placements are degenerate even though the count looks right.
        let mut positions: Vec<(i64, i64)> = placed.iter()
            .map(|p| (p["x"].as_i64().unwrap_or(0), p["y"].as_i64().unwrap_or(0)))
            .collect();
        positions.sort_unstable();
        positions.dedup();
        assert_eq!(positions.len(), 12, "each piece should occupy its own slot");
    } // richmond_shirt_handoff_packs_twelve_individual_pieces
} // mod tests
