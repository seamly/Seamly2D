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
//   do_process_size_layout(args, size)             -> Result<ProcessLayoutResult, String>
//   do_find_oversized_pieces(settings_json, input_dom, initial_layout_dom)
//                                                  -> Result<Vec<OversizedPiece>, String>

use svg_dom::Document;

use layout_tiling::{
    compute_tile_dims, create_initial_tiled_layout_dom, measurement_to_px,
    pick_best_tiled_candidate, widest_piece_tile_cols, LayoutSettings, TileDimensions,
};

use crate::piece_extractor::{
    extract_piece_rects_and_polygons, hoist_layout_units, keep_size, set_free_rotation_without_grainline,
};
use crate::layout_assembler::{create_layout, create_initial_layout_dom, set_content_width, trim_bottom};
use crate::piece_divider::{self, DivideSpec, OversizedPiece};
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
#[derive(Clone)]
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
    /// Warning lines for pieces that could not be placed in the layout: one per
    /// piece, then its sizes (`piece_extractor::unplaced_piece_lines`).
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
    /// The media the pieces were split for; None when no split was asked for.
    /// The paper-sheet PDF export splits the same way.
    pub divide_spec: Option<DivideSpec>,
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
    /// Multisize only: `true` packs each piece-set (one piece in every size) as one
    /// unit (Nested tab); `false` packs every size of every piece alone (Marker tab).
    pub nested_multisize: bool,
    /// `true` extends a sheet, roll or fabric until every piece is placed: wider
    /// for the widest piece, then longer (multisize tabs).  `false` leaves out a
    /// piece that does not fit.  Tiled paper ignores it.
    pub extend_media: bool,
    /// `true` splits each piece wider or longer than the media into parts that
    /// fit (individual pattern, the user's choice).  Tiled paper ignores it.
    pub split_oversized: bool,
}

/// @brief Run the full layout pipeline: preprocess, pack, assemble, trim.
///
/// Pipeline stages:
///   1. Parse settings, extract bin dimensions from initial_layout_dom
///   2. Preprocess pieces: flatten → verticalize → flatten → translate → flatten
///   3. Extract piece rectangles
///   4. Pack pieces into the bin with MaxRects; extended media grows the bin
///   5. Assemble output DOM
///   6. Fit the canvas to extended media, trim unused space (roll/fabric) or
///      empty tile rows (tiled)
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
    // margin right: kept when extended media widens the canvas
    let mr_px: u32 = args.initial_layout_dom.root.attributes
        .get("width")
        .and_then(|s| s.parse::<u32>().ok())
        .unwrap_or(0)
        .saturating_sub(ml_px + bin_w);

    // --- 2 clone input_dom for piece pre-processing ---
    let mut input_dom_clone = args.input_dom.clone();

    // 2a: normalise the Seamly2D handoff shape.
    //
    // Layout Mode nests every piece inside one `<g data-type="pattern">`; every
    // stage below treats a direct `<g>` child of the root as one piece, so
    // without this the packer receives the whole pattern as a single sheet-sized
    // object and places nothing (Task 59).  Untagged drawings are left alone.
    // Done on the clone so the imported document the user sees is untouched.
    // Multisize: Nested keeps each piece-set (one piece in every size) as one
    // unit; Marker hoists every size of every piece as its own unit.
    let hoisted = hoist_layout_units(&mut input_dom_clone, args.nested_multisize);
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

    // 3d': Nested multisize — stack the sizes of each piece-set on the largest
    // size's centre, then bake the offsets.  Needs vertical, flattened sizes,
    // and must run before translate_dom measures each set's bounding box.
    let piece_sets = svg_dom::center_piece_sets(&mut flat_dom);
    if piece_sets > 0 {
        log_to_file(&format!(
            "[debug] layout_utils::do_process_layout(): 5a centred the sizes of {} piece-set(s)",
            piece_sets
        ));
        svg_dom::flatten_dom(&mut flat_dom);
        save_debug_dom(&flat_dom, "centred_dom.svg");
    } // if piece_sets

    // 3d'': the user chose to split each piece wider or longer than the media.
    // Needs grain-up pieces; must run before translate_dom moves each part to the origin.
    // Tiled paper never splits: its pages already cover a large piece.
    let is_tiled = settings.media_type == "paper" && settings.paper_type == "tiled";
    let divide_spec = (args.split_oversized && !is_tiled).then(|| layout_divide_spec(&settings, bin_w, bin_h));
    if let Some(spec) = &divide_spec {
        let divided = piece_divider::divide_oversized_pieces(&mut flat_dom, spec);
        if !divided.is_empty() {
            log_to_file(&format!("[debug] layout_utils::do_process_layout(): 5b split pieces: {divided:?}"));
            save_debug_dom(&flat_dom, "divided_dom.svg");
        } // if divided
    } // if split

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
    let (mut pieces, polygons) = extract_piece_rects_and_polygons(&flat_dom);
    set_free_rotation_without_grainline(&mut pieces, settings.free_rotation_without_grainline());
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
    // against the fixed bin from the initial layout DOM.  `is_tiled` is set in step 3.

    // Extended media widens to the widest piece, so no piece is too wide.
    // Tiled paper never extends: its candidate-width search sets the width.
    let pack_bin_w = if args.extend_media && !is_tiled {
        bin_w.max(packing::required_bin_width(&rects, &trial_angles_deg))
    } else {
        bin_w
    }; // pack_bin_w

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
        // Extended media is already wide enough for every piece and grows past
        // its length instead, so it leaves no piece out; `packed_bin_h` is the
        // height it grew to.
        let (placements, _created_rects, unplaced_ids, packed_bin_h) = if args.extend_media {
            packing::pack_polygons_extending(
                pack_bin_w,
                bin_h,
                gap_px,
                &polygons,
                &rects,
                &trial_angles_deg,
                Some(&mut piece_status),
            )
        } else {
            let (placements, created_rects, unplaced_ids) = packing::pack_polygons_lenient(
                bin_w,
                bin_h,
                gap_px,
                &polygons,
                &rects,
                &trial_angles_deg,
                Some(&mut piece_status),
            );
            (placements, created_rects, unplaced_ids, bin_h)
        }; // if extend_media

        // Map the unplaced original indices to warning lines: one per piece, named as
        // DXF-ASTM names it ("Detail (3)"), then its sizes.
        let unplaced_labels = crate::piece_extractor::unplaced_piece_lines(&flat_dom, &pieces, &unplaced_ids);

        log_to_file(&format!(
            "[process_layout] non-tiled pack returned in {} ms with {} placements, {} unplaced: {:?}; bin {}x{} -> {}x{}",
            t_pack.elapsed().as_millis(), placements.len(), unplaced_labels.len(), unplaced_labels,
            bin_w, bin_h, pack_bin_w, packed_bin_h,
        ));

        progress_fn(60, None); // packing complete

        // Non-tiled: keep the initial canvas and margins.  Step 7 sets the
        // canvas size when the media was extended.
        (placements, args.initial_layout_dom.clone(), packed_bin_h, ml_px, mt_px, mb_px, layout_h_px, unplaced_labels)
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
    // Extended media: wider than the media for the widest piece, or longer
    // because the packer grew the bin.  Tiled paper never extends.
    let widened = pack_bin_w > bin_w;
    let extended = !is_tiled && (widened || work_bin_h > bin_h);

    // 7a. Widen the canvas to the right; both side margins are kept.
    if widened {
        set_content_width(&mut output_doc, pack_bin_w, work_ml_px, mr_px);
        log_to_file(&format!(
            "[process_layout] media widened: content width {bin_w} px -> {pack_bin_w} px"
        ));
    } // if widened

    // 7b. Content height the canvas ends at.  Roll and fabric end at the lowest
    // piece when the blank space below it is large.  Extended media always ends
    // at the lowest piece, but an extended sheet keeps at least its own length.
    let fitted_content_h = if extended && !settings.is_roll_form() {
        Some(max_bin_bottom.max(bin_h))
    } else if extended || (settings.is_roll_form() && blank_bottom > MIN_THRESHOLD_PX) {
        Some(max_bin_bottom)
    } else {
        None
    }; // fitted_content_h
    if let Some(content_h) = fitted_content_h {
        trim_bottom(&mut output_doc, content_h, work_mt_px, work_mb_px);
        work_layout_h_px = work_mt_px + content_h + work_mb_px;
        if extended {
            log_to_file(&format!(
                "[process_layout] media extended: content height {bin_h} px -> {content_h} px"
            ));
        } // if extended
    } // if fitted_content_h

    if blank_bottom > MIN_THRESHOLD_PX {
        // 7c trim tiled layout
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

    // 7d. Flatten output_doc ---
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
        divide_spec,
    })
} // fn do_process_layout

/// @brief The media a piece must fit to stay whole: the content rect, limited in
/// length only for a sheet.
pub fn layout_divide_spec(settings: &LayoutSettings, bin_w: u32, bin_h: u32) -> DivideSpec {
    DivideSpec {
        bin_w,
        bin_h,
        limit_length: !settings.is_roll_form(),
        trial_angles_deg: settings.rotation_trial_set_deg(),
        free_rotation_without_grainline: settings.free_rotation_without_grainline(),
    }
} // fn layout_divide_spec

/// @brief Pieces of an individual pattern that do not fit the media at any of their trial angles.
///
/// Runs the pre-processing up to grain-up pieces, as `do_process_layout` does.
/// Tiled paper reports none: its pages already cover a large piece.
///
/// # Errors
/// Returns an error when the settings do not parse.
pub fn do_find_oversized_pieces(
    settings_json: &str,
    input_dom: &Document,
    initial_layout_dom: &Document,
) -> Result<Vec<OversizedPiece>, String> {
    let settings = LayoutSettings::from_json(settings_json)
        .map_err(|e| format!("Failed to parse layout settings: {e}"))?;
    if settings.media_type == "paper" && settings.paper_type == "tiled" {
        return Ok(Vec::new());
    } // if tiled
    let size = |name: &str| -> u32 {
        initial_layout_dom.get_attr_by_id("contentRect", name).and_then(|s| s.parse().ok()).unwrap_or(0)
    };
    let mut dom = input_dom.clone();
    hoist_layout_units(&mut dom, false);
    svg_dom::flatten_dom(&mut dom);
    svg_dom::verticalize_dom(&mut dom);
    svg_dom::flatten_dom(&mut dom);
    Ok(piece_divider::find_oversized_pieces(&dom, &layout_divide_spec(&settings, size("width"), size("height"))))
} // fn do_find_oversized_pieces

/// @brief Lay out one size of a multisize handoff as its own layout.
///
/// Only that size's pieces are packed, one unit per piece, with the same
/// settings as the main layout.  A single size has nothing to nest, so
/// `args.nested_multisize` is ignored.
///
/// @param args Same arguments as `do_process_layout`; `input_dom` is the full handoff.
/// @param size The `data-size` value to lay out.
///
/// # Errors
/// Returns an error when the size has no pieces, or `do_process_layout` fails.
pub fn do_process_size_layout(
    args: ProcessLayoutArgs<'_>,
    size: &str,
    progress_fn: ProgressFn<'_>,
) -> Result<ProcessLayoutResult, String> {
    log_to_file(&format!("==========PROCESS SIZE LAYOUT {size}=========="));

    // Keep only this size's pieces.
    let mut size_dom = args.input_dom.clone();
    if keep_size(&mut size_dom, size) == 0 {
        return Err(format!("Size {size}: no pattern pieces of this size in the imported SVG."));
    } // if no pieces

    // Pack every piece alone: the piece-sets of the other sizes are gone.
    do_process_layout(
        ProcessLayoutArgs { input_dom: &size_dom, nested_multisize: false, ..args },
        progress_fn,
    )
} // fn do_process_size_layout

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief The real Seamly2D handoff for the Richmond shirt: 12 pieces nested
    // inside one `<g id="pattern-1" data-type="pattern">`.
    //
    // Produced with the headless export:
    //   seamly2d.exe test-seamly-layout-input/richmond-shirt_v1_v061-test.sm2d \
    //       -b handoff -d <dir> -f 0 --exportOnlyDetails
    //
    // It lives in this crate's `test_data/` rather than the top-level
    // `test-seamly-layout-input/` directory on purpose: `include_str!` makes this
    // a *compile-time* dependency of the test, so the file has to be tracked
    // unconditionally. `test-seamly-layout-input/` holds hand-run sample patterns
    // and is tracked only "until development is complete" (root `.gitignore`);
    // a fixture there would stop compiling the day that policy is reverted.
    // Embedded at compile time so the test needs no runtime path resolution and
    // behaves identically on every runner.
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
                nested_multisize: false,
                extend_media: false,
                split_oversized: false,
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

    // @brief Seamly2D handoff for the trousers sample: 12 pieces, of which the
    // four trouser legs carry no grainline.  Produced like HANDOFF_SVG.
    const TROUSERS_HANDOFF_SVG: &str = include_str!("../test_data/trousers-handoff_pieces.svg");

    // @brief Pieces without a grainline keep their drafted orientation.
    //
    // Each trouser leg is about 36 cm wide and 110 cm long.  A leg turned on its
    // side is wider than a 36 in roll, so it would be left out of the layout.
    #[test]
    fn trousers_handoff_fits_36_inch_roll() {
        let result = process_trousers(36.0, 500.0, "upright");
        assert!(
            result.unplaced_labels.is_empty(),
            "no piece should be left unplaced, got {:?}", result.unplaced_labels
        );
    } // trousers_handoff_fits_36_inch_roll

    // @brief On fabric 42 in long a 43 in trouser leg fits only when turned.
    // "upright" leaves the four legs (no grainline) out; "free" turns them to fit.
    // Pieces with a grainline never turn, so some of them stay unplaced in both runs.
    #[test]
    fn trousers_legs_turn_only_with_free_rotation() {
        const LEGS: [&str; 4] = ["Back", "Front_Pleats", "Front_no_pocket", "Front_with_pocket"];

        let upright = process_trousers(100.0, 42.0, "upright");
        for leg in LEGS {
            assert!(
                upright.unplaced_labels.iter().any(|l| l == leg),
                "{leg} should not fit upright, unplaced: {:?}", upright.unplaced_labels
            );
        }

        let free = process_trousers(100.0, 42.0, "free");
        for leg in LEGS {
            assert!(
                !free.unplaced_labels.iter().any(|l| l == leg),
                "{leg} should fit when turned, unplaced: {:?}", free.unplaced_labels
            );
        }
        assert!(
            free.unplaced_labels.iter().any(|l| l == "WaistBand1"),
            "a piece with a grainline must not turn, unplaced: {:?}", free.unplaced_labels
        );
    } // trousers_legs_turn_only_with_free_rotation

    // @brief Lay out the trousers handoff on fabric of the given size (inches).
    fn process_trousers(width_in: f64, length_in: f64, no_grainline_rotation: &str) -> ProcessLayoutResult {
        let input_dom = Document::parse(TROUSERS_HANDOFF_SVG).expect("trousers fixture should parse");
        let settings_json = serde_json::json!({
            "unit": "in",
            "mediaType": "fabric",
            "paperType": "roll",
            "pageWidth": width_in,
            "pageHeight": length_in,
            "marginLeft": 0.5,
            "marginRight": 0.5,
            "marginTop": 0.5,
            "marginBottom": 0.5,
            "pieceGap": 0.125,
            "layoutMode": "alongGrainline",
            "rotationStep": 180,
            "noGrainlineRotation": no_grainline_rotation,
            "tileSize": "Letter",
            "tileOrientation": "Portrait"
        }).to_string();
        let init = do_initialize_layout(&settings_json, Some(&input_dom))
            .expect("initialize_layout should succeed");

        let mut progress = |_pct: i32, _status: Option<&str>| {};
        do_process_layout(
            ProcessLayoutArgs {
                settings_json: &settings_json,
                input_dom: &input_dom,
                initial_layout_dom: &init.initial_dom,
                layout_h_px: init.h_px,
                nested_multisize: false,
                extend_media: false,
                split_oversized: false,
            },
            &mut progress,
        ).expect("process_layout should succeed on the trousers handoff")
    } // fn process_trousers
    // @brief Paper sheet settings, in inches, for the oversized-piece tests.
    fn paper_sheet_settings_json(width_in: f64, height_in: f64) -> String {
        serde_json::json!({
            "unit": "in",
            "mediaType": "paper",
            "paperType": "sheet",
            "pageWidth": width_in,
            "pageHeight": height_in,
            "marginLeft": 0.25,
            "marginRight": 0.25,
            "marginTop": 0.25,
            "marginBottom": 0.25,
            "pieceGap": 0.125,
            "layoutMode": "alongGrainline"
        }).to_string()
    } // fn paper_sheet_settings_json

    // @brief Lay out the trousers on a paper sheet, splitting or extending as asked.
    fn process_trousers_sheet(settings_json: &str, split: bool, extend: bool) -> ProcessLayoutResult {
        let input_dom = Document::parse(TROUSERS_HANDOFF_SVG).expect("trousers fixture should parse");
        let init = do_initialize_layout(settings_json, Some(&input_dom)).expect("initialize_layout should succeed");
        let mut progress = |_pct: i32, _status: Option<&str>| {};
        do_process_layout(
            ProcessLayoutArgs {
                settings_json,
                input_dom: &input_dom,
                initial_layout_dom: &init.initial_dom,
                layout_h_px: init.h_px,
                nested_multisize: false,
                extend_media: extend,
                split_oversized: split,
            },
            &mut progress,
        ).expect("process_layout should succeed on the trousers handoff")
    } // fn process_trousers_sheet

    // @brief The bbox JSON labels of a layout.
    fn placed_labels(result: &ProcessLayoutResult) -> Vec<String> {
        let bbox: serde_json::Value = serde_json::from_str(&result.bbox_json).expect("valid JSON");
        bbox["pieces"].as_array().expect("pieces array").iter()
            .filter_map(|p| p["label"].as_str().map(str::to_string))
            .collect()
    } // fn placed_labels

    // @brief On a sheet 36 in long the 43 in trouser legs are longer than the media.
    // Split: each long piece becomes parts that fit; the sheet is wide enough for all.
    // The DXF-ASTM export writes each part as its own piece.
    #[test]
    fn trousers_long_pieces_split_on_a_sheet() {
        let settings_json = paper_sheet_settings_json(200.0, 36.0);
        let input_dom = Document::parse(TROUSERS_HANDOFF_SVG).expect("trousers fixture should parse");
        let init = do_initialize_layout(&settings_json, Some(&input_dom)).expect("initialize_layout should succeed");
        let oversized = do_find_oversized_pieces(&settings_json, &input_dom, &init.initial_dom).expect("check runs");
        let long: Vec<&str> = oversized.iter().filter(|p| p.longer && !p.wider).map(|p| p.label.as_str()).collect();
        for leg in ["Back", "Front_Pleats", "Front_no_pocket", "Front_with_pocket"] {
            assert!(long.contains(&leg), "{leg} is longer than the sheet: {oversized:?}");
        }

        let result = process_trousers_sheet(&settings_json, true, false);
        assert!(result.unplaced_labels.is_empty(), "unplaced: {:?}", result.unplaced_labels);
        assert!(result.divide_spec.is_some(), "the split media is kept for the PDF export");
        let labels = placed_labels(&result);
        for leg in ["Back", "Front_Pleats", "Front_no_pocket", "Front_with_pocket"] {
            for part in ["(1/2)", "(2/2)"] {
                assert!(labels.contains(&format!("{leg} {part}")), "{leg} {part} placed: {labels:?}");
            }
        } // for each leg

        // The paper-sheet PDF export splits the same way.
        let (_, sheet_pieces) = crate::sheets::build_sheet_export_inputs(&input_dom, false, false, result.divide_spec.as_ref())
            .expect("sheet inputs build");
        assert_eq!(sheet_pieces.len(), labels.len(), "the PDF pages hold the same parts");

        // DXF-ASTM: one block per part, named "<piece> (i/n)".
        let (_, dxf, _, _) = export_dxf_for_test(&result, "split_trousers", None);
        let names: Vec<&str> = dxf.lines().map(str::trim).filter_map(|l| l.strip_prefix("Piece Name:")).collect();
        assert!(names.contains(&"Back (1/2)") && names.contains(&"Back (2/2)"), "{names:?}");
        assert_eq!(names.len(), labels.len(), "one block per placed part");
    } // trousers_long_pieces_split_on_a_sheet

    // @brief "Make the media bigger": the sheet grows until every piece is placed whole.
    #[test]
    fn trousers_long_pieces_extend_a_sheet() {
        let settings_json = paper_sheet_settings_json(36.0, 36.0);
        let result = process_trousers_sheet(&settings_json, false, true);
        assert!(result.unplaced_labels.is_empty(), "unplaced: {:?}", result.unplaced_labels);
        assert!(result.divide_spec.is_none());
        assert_eq!(placed_count(&result), 12, "no piece split");
        assert!(result.layout_h_px > 36 * 96, "the sheet grows past 36 in: {} px", result.layout_h_px);
    } // trousers_long_pieces_extend_a_sheet

    // @brief Fabric roll settings for the multisize fixture.
    fn multisize_settings_json() -> String {
        serde_json::json!({
            "unit": "in",
            "mediaType": "fabric",
            "paperType": "roll",
            "pageWidth": 36.0,
            "pageHeight": 100.0,
            "marginLeft": 0.5,
            "marginRight": 0.5,
            "marginTop": 0.5,
            "marginBottom": 0.5,
            "pieceGap": 0.125,
            "layoutMode": "alongGrainline",
            "tileSize": "Letter",
            "tileOrientation": "Portrait"
        }).to_string()
    } // fn multisize_settings_json

    // @brief Lay out the multisize fixture as the Nested tab (`nested`) or the Marker tab.
    fn process_multisize(nested: bool) -> ProcessLayoutResult {
        let input_dom = Document::parse(crate::piece_extractor::tests::MULTISIZE_HANDOFF_SVG)
            .expect("multisize fixture should parse");
        let settings_json = multisize_settings_json();
        let init = do_initialize_layout(&settings_json, Some(&input_dom))
            .expect("initialize_layout should succeed");

        let mut progress = |_pct: i32, _status: Option<&str>| {};
        do_process_layout(
            ProcessLayoutArgs {
                settings_json: &settings_json,
                input_dom: &input_dom,
                initial_layout_dom: &init.initial_dom,
                layout_h_px: init.h_px,
                nested_multisize: nested,
                extend_media: false,
                split_oversized: false,
            },
            &mut progress,
        ).expect("process_layout should succeed on the multisize handoff")
    } // fn process_multisize

    // @brief Lay out one size of the multisize fixture, as a size tab or the Base Size tab.
    fn process_multisize_size(size: &str) -> ProcessLayoutResult {
        let input_dom = Document::parse(crate::piece_extractor::tests::MULTISIZE_HANDOFF_SVG)
            .expect("multisize fixture should parse");
        let settings_json = multisize_settings_json();
        let init = do_initialize_layout(&settings_json, Some(&input_dom))
            .expect("initialize_layout should succeed");

        let mut progress = |_pct: i32, _status: Option<&str>| {};
        do_process_size_layout(
            ProcessLayoutArgs {
                settings_json: &settings_json,
                input_dom: &input_dom,
                initial_layout_dom: &init.initial_dom,
                layout_h_px: init.h_px,
                nested_multisize: false,
                extend_media: false,
                split_oversized: false,
            },
            size,
            &mut progress,
        ).expect("size layout should succeed on the multisize handoff")
    } // fn process_multisize_size

    // @brief Number of placed pieces in a layout's bbox JSON.
    fn placed_count(result: &ProcessLayoutResult) -> usize {
        let bbox: serde_json::Value =
            serde_json::from_str(&result.bbox_json).expect("bbox_json should be valid JSON");
        bbox["pieces"].as_array().expect("pieces array").len()
    } // fn placed_count

    // @brief Nested packs one unit per piece; Marker packs one unit per piece per size.
    #[test]
    fn multisize_nested_packs_sets_and_marker_packs_sizes() {
        let nested = process_multisize(true);
        assert!(nested.unplaced_labels.is_empty(), "unplaced: {:?}", nested.unplaced_labels);
        assert_eq!(placed_count(&nested), 2, "Nested: one unit per piece");
        // Adjust Mode moves each bbox entry by id, so a set id moves every size together.
        let bbox: serde_json::Value = serde_json::from_str(&nested.bbox_json).expect("valid JSON");
        let mut ids: Vec<&str> = bbox["pieces"].as_array().unwrap().iter()
            .filter_map(|p| p["id"].as_str())
            .collect();
        ids.sort_unstable();
        assert_eq!(ids, vec!["piece-set_Back", "piece-set_Front"]);

        let marker = process_multisize(false);
        assert!(marker.unplaced_labels.is_empty(), "unplaced: {:?}", marker.unplaced_labels);
        assert_eq!(placed_count(&marker), 4, "Marker: pieces x sizes");
    } // multisize_nested_packs_sets_and_marker_packs_sizes

    // @brief Nested: the smaller size sits centred on the larger one, largest first,
    // and each placed piece keeps its data-size.
    #[test]
    fn multisize_nested_centres_sizes_on_largest() {
        let nested = process_multisize(true);
        let svg = nested.output_doc.to_string();
        let big = svg.find(r#"id="piece_Front_s36""#).expect("size 36 in output");
        let small = svg.find(r#"id="piece_Front_s34""#).expect("size 34 in output");
        assert!(big < small, "the largest size must come first (drawn at the bottom)");
        assert!(svg.contains(r#"data-size="34""#), "placed pieces keep data-size");

        let set = nested.flat_dom.root.children.iter()
            .filter_map(|n| n.as_element())
            .find(|e| e.attributes.get("id").map(String::as_str) == Some("piece-set_Front"))
            .expect("Front piece-set in the pre-processed DOM");
        let centres: Vec<(f32, f32)> = set.children.iter()
            .filter_map(|n| n.as_element())
            .map(|member| {
                let mut points = Vec::new();
                collect_points_for_test(member, &mut points);
                let min_x = points.iter().map(|p| p.0).fold(f32::INFINITY, f32::min);
                let max_x = points.iter().map(|p| p.0).fold(f32::NEG_INFINITY, f32::max);
                let min_y = points.iter().map(|p| p.1).fold(f32::INFINITY, f32::min);
                let max_y = points.iter().map(|p| p.1).fold(f32::NEG_INFINITY, f32::max);
                ((min_x + max_x) / 2.0, (min_y + max_y) / 2.0)
            })
            .collect();
        assert_eq!(centres.len(), 2);
        assert!(
            (centres[0].0 - centres[1].0).abs() < 0.01 && (centres[0].1 - centres[1].1).abs() < 0.01,
            "sizes should share one centre, got {centres:?}"
        );
    } // multisize_nested_centres_sizes_on_largest

    // @brief A size layout packs only that size's pieces, one unit per piece.
    #[test]
    fn size_layout_packs_only_that_size() {
        let input_dom = Document::parse(crate::piece_extractor::tests::MULTISIZE_HANDOFF_SVG)
            .expect("multisize fixture should parse");
        let settings_json = multisize_settings_json();
        let init = do_initialize_layout(&settings_json, Some(&input_dom))
            .expect("initialize_layout should succeed");

        for size in ["34", "36"] {
            let mut progress = |_pct: i32, _status: Option<&str>| {};
            let result = do_process_size_layout(
                ProcessLayoutArgs {
                    settings_json: &settings_json,
                    input_dom: &input_dom,
                    initial_layout_dom: &init.initial_dom,
                    layout_h_px: init.h_px,
                    // Nested must not matter for a single size.
                    nested_multisize: true,
                    extend_media: false,
                    split_oversized: false,
                },
                size,
                &mut progress,
            ).expect("size layout should succeed");
            assert!(result.unplaced_labels.is_empty(), "unplaced: {:?}", result.unplaced_labels);
            assert_eq!(placed_count(&result), 2, "size {size}: one unit per piece");

            let svg = result.output_doc.to_string();
            assert!(svg.contains(&format!(r#"data-size="{size}""#)), "size {size} pieces placed");
            let other = if size == "34" { "36" } else { "34" };
            assert!(!svg.contains(&format!(r#"data-size="{other}""#)), "size {other} must be absent");
        } // for size

        // A size that is not in the handoff is an error, not an empty layout.
        let mut progress = |_pct: i32, _status: Option<&str>| {};
        let missing = do_process_size_layout(
            ProcessLayoutArgs {
                settings_json: &settings_json,
                input_dom: &input_dom,
                initial_layout_dom: &init.initial_dom,
                layout_h_px: init.h_px,
                nested_multisize: false,
                extend_media: false,
                split_oversized: false,
            },
            "99",
            &mut progress,
        );
        assert!(missing.is_err());
    } // size_layout_packs_only_that_size

    // @brief Collect path end points of an element tree, for centre checks.
    fn collect_points_for_test(element: &xmltree::Element, points: &mut Vec<(f32, f32)>) {
        if element.name == "path" {
            if let Some(d) = element.attributes.get("d") {
                if let Ok(path) = geometry::Path::parse_path_attribute(d) {
                    for seg in &path.segments {
                        match seg {
                            geometry::PathSegment::MoveTo(p) | geometry::PathSegment::LineTo(p) => points.push((p.x, p.y)),
                            _ => {} // the fixture uses straight lines only
                        } // match seg
                    } // for seg
                } // if parsed
            } // if d
        } // if path
        for child in &element.children {
            if let xmltree::XMLNode::Element(e) = child {
                collect_points_for_test(e, points);
            } // if element
        } // for child
    } // fn collect_points_for_test

    // @brief Write a DXF-ASTM export of `layout` to a temp file named `tag`.
    // @return (warnings, DXF text, `.rul` text when the export wrote one, file stem).
    fn export_dxf_for_test(
        layout: &ProcessLayoutResult,
        tag: &str,
        grading: Option<crate::exports::DxfGradingSource>,
    ) -> (Vec<String>, String, Option<String>, String) {
        let style = crate::exports::DxfStyleInfo { grading, ..crate::exports::DxfStyleInfo::default() };
        let stem = format!("{tag}_{}", std::process::id());
        let path = std::env::temp_dir().join(format!("{stem}.dxf"));
        let _ = std::fs::remove_file(path.with_extension("rul"));
        let report = crate::exports::do_export_dxf(&layout.output_doc, &path.to_string_lossy(), false, &style, &mut |_| {})
            .expect("DXF export succeeds");
        let dxf = std::fs::read_to_string(&path).expect("DXF written");
        let rul = std::fs::read_to_string(path.with_extension("rul")).ok();
        // The report names the `.rul` file exactly when the export wrote one: "Export Complete" lists it.
        assert_eq!(report.rule_file.is_some(), rul.is_some(), "report rule_file {:?}", report.rule_file);
        if let Some(rule_file) = &report.rule_file {
            assert_eq!(std::path::Path::new(rule_file), path.with_extension("rul"), "rule file beside the DXF");
        } // if rule file
        let _ = std::fs::remove_file(&path);
        let _ = std::fs::remove_file(path.with_extension("rul"));
        (report.warnings, dxf, rul, stem)
    } // fn export_dxf_for_test

    // @brief The Base Size tab exports the base size with `# n` texts and the `.rul` file;
    // a size tab exports one ungraded size.
    #[test]
    fn base_size_tab_exports_grade_rules() {
        let input = Document::parse(crate::piece_extractor::tests::MULTISIZE_HANDOFF_SVG).expect("fixture parses");
        let grading = crate::exports::DxfGradingSource {
            input,
            sizes: vec!["34".to_string(), "36".to_string()],
            base_size: "36".to_string(),
        };
        let (warnings, dxf, rul, stem) = export_dxf_for_test(&process_multisize_size("36"), "base_size", Some(grading));
        assert!(warnings.is_empty(), "{warnings:?}");
        let lines: Vec<&str> = dxf.lines().map(str::trim).collect();
        assert!(lines.contains(&"Sample Size:36"), "sample size");
        assert!(lines.contains(&format!("Grade Rule Table:{stem}").as_str()), "table name");
        // One block per piece, all of the base size; one grade reference line per piece.
        let sizes: Vec<&str> = lines.iter().filter_map(|l| l.strip_prefix("Size Name:")).collect();
        assert_eq!(sizes, vec!["36", "36"]);
        let reference_lines = lines.windows(4).filter(|w| w[0] == "0" && w[1] == "LINE" && w[2] == "8" && w[3] == "5").count();
        assert_eq!(reference_lines, 2, "one layer 5 line per piece");
        assert!(lines.contains(&"# 1"), "grade rule identifiers");
        let rul = rul.expect("rule file beside the DXF");
        assert!(rul.contains(&format!("GRADE RULE TABLE:{stem}")) && rul.contains("SIZE LIST:34 36"), "{rul}");
        assert!(rul.lines().any(|l| l.starts_with("RULE: DELTA 1 ")), "{rul}");

        let (warnings, dxf, rul, _) = export_dxf_for_test(&process_multisize_size("34"), "size_tab", None);
        assert!(warnings.is_empty(), "{warnings:?}");
        let lines: Vec<&str> = dxf.lines().map(str::trim).collect();
        assert!(lines.contains(&"Sample Size:") && lines.contains(&"Grade Rule Table:"), "blank grading identifiers");
        assert!(!lines.iter().any(|l| l.starts_with("# ")), "no grade rule identifiers");
        assert!(rul.is_none(), "no rule file for a size tab");
    } // base_size_tab_exports_grade_rules

    // @brief A Base Size export in which no piece grades writes no `.rul` file and says so.
    #[test]
    fn base_size_export_without_graded_piece_says_no_rule_file() {
        // The handoff holds no piece, so each piece fails with "size 34 is not in the handoff".
        let input = Document::parse(r#"<svg xmlns="http://www.w3.org/2000/svg"></svg>"#).expect("empty handoff parses");
        let grading = crate::exports::DxfGradingSource {
            input,
            sizes: vec!["34".to_string(), "36".to_string()],
            base_size: "36".to_string(),
        };
        let (warnings, _, rul, _) = export_dxf_for_test(&process_multisize_size("36"), "no_graded_piece", Some(grading));
        assert!(rul.is_none(), "no rule file");
        assert!(warnings.len() > 1, "each piece is named first: {warnings:?}");
        assert_eq!(warnings.last().map(String::as_str), Some(crate::exports::NO_PIECE_GRADED_WARNING), "{warnings:?}");
    } // base_size_export_without_graded_piece_says_no_rule_file

    // @brief Real multisize handoff: jacket2_40-146.sm2d with gost_man_ru.smms, 8 pieces
    // in 26 sizes (220-720), base size 500, captured with SEAMLYLAYOUT_SAVE_HANDOFF
    // from MSI 26.10.1416. The pieces carry the names of the installed sample: two
    // Cyrillic names and six pieces named "Detail".
    const JACKET2_HANDOFF_SVG: &str = include_str!("../test_data/jacket2_40-146_handoff.svg");
    const JACKET2_BASE_SIZE: &str = "500";

    // @brief The jacket2 handoff and its sizes, from the pattern group's `data-sizes`.
    fn jacket2_handoff() -> (Document, Vec<String>) {
        let doc = Document::parse(JACKET2_HANDOFF_SVG).expect("jacket2 handoff should parse");
        let sizes = crate::piece_extractor::read_measurements_info(&doc).sizes;
        (doc, sizes)
    } // fn jacket2_handoff

    // @brief Fabric roll settings for jacket2, in inches.
    fn jacket2_settings_json(width_in: f64, length_in: f64) -> String {
        serde_json::json!({
            "unit": "in",
            "mediaType": "fabric",
            "paperType": "roll",
            "pageWidth": width_in,
            "pageHeight": length_in,
            "marginLeft": 0.5,
            "marginRight": 0.5,
            "marginTop": 0.5,
            "marginBottom": 0.5,
            "pieceGap": 0.125,
            "layoutMode": "alongGrainline",
            "tileSize": "Letter",
            "tileOrientation": "Portrait"
        }).to_string()
    } // fn jacket2_settings_json

    // @brief Lay out the jacket2 handoff as the Nested tab (`nested`) or the Marker tab.
    fn process_jacket2(nested: bool, width_in: f64, length_in: f64) -> ProcessLayoutResult {
        let (input_dom, _) = jacket2_handoff();
        let settings_json = jacket2_settings_json(width_in, length_in);
        let init = do_initialize_layout(&settings_json, Some(&input_dom))
            .expect("initialize_layout should succeed");

        let mut progress = |_pct: i32, _status: Option<&str>| {};
        do_process_layout(
            ProcessLayoutArgs {
                settings_json: &settings_json,
                input_dom: &input_dom,
                initial_layout_dom: &init.initial_dom,
                layout_h_px: init.h_px,
                nested_multisize: nested,
                extend_media: false,
                split_oversized: false,
            },
            &mut progress,
        ).expect("process_layout should succeed on the jacket2 handoff")
    } // fn process_jacket2

    // @brief Lay out one size of the jacket2 handoff, as a size tab or the Base Size tab.
    fn process_jacket2_size(size: &str) -> ProcessLayoutResult {
        let (input_dom, _) = jacket2_handoff();
        process_jacket2_input_size(&input_dom, size)
    } // fn process_jacket2_size

    // @brief Lay out one size of a jacket2 input document, as a size tab or the Base Size tab.
    fn process_jacket2_input_size(input_dom: &Document, size: &str) -> ProcessLayoutResult {
        let settings_json = jacket2_settings_json(60.0, 300.0);
        let init = do_initialize_layout(&settings_json, Some(input_dom))
            .expect("initialize_layout should succeed");

        let mut progress = |_pct: i32, _status: Option<&str>| {};
        do_process_size_layout(
            ProcessLayoutArgs {
                settings_json: &settings_json,
                input_dom,
                initial_layout_dom: &init.initial_dom,
                layout_h_px: init.h_px,
                nested_multisize: false,
                extend_media: false,
                split_oversized: false,
            },
            size,
            &mut progress,
        ).expect("size layout should succeed on the jacket2 handoff")
    } // fn process_jacket2_input_size

    // @brief The jacket2 handoff reads as one multisize pattern: 8 piece-sets, 208 pieces, 26 sizes.
    #[test]
    fn jacket2_handoff_is_multisize() {
        let (doc, sizes) = jacket2_handoff();
        assert_eq!(crate::piece_extractor::count_tagged_pieces(&doc), 208, "8 pieces x 26 sizes");
        let info = crate::piece_extractor::read_measurements_info(&doc);
        assert!(info.multisize);
        assert_eq!(info.sizes, sizes);
        assert_eq!((info.sizes.len(), info.sizes[0].as_str(), info.sizes[25].as_str()), (26, "220", "720"));
        assert_eq!(info.base_size, JACKET2_BASE_SIZE);
    } // jacket2_handoff_is_multisize

    // @brief Nested packs the 8 piece-sets, every size stacked inside its set.
    #[test]
    fn jacket2_nested_packs_eight_piece_sets() {
        let nested = process_jacket2(true, 60.0, 300.0);
        assert!(nested.unplaced_labels.is_empty(), "unplaced: {:?}", nested.unplaced_labels);
        assert_eq!(placed_count(&nested), 8, "Nested: one unit per piece");
        let svg = nested.output_doc.to_string();
        assert_eq!(svg.matches(r#"data-type="piece""#).count(), 208, "every size of every piece placed");
    } // jacket2_nested_packs_eight_piece_sets

    // @brief Bounding box of every `data-type="piece"` group, by id.
    fn piece_boxes(doc: &Document) -> std::collections::BTreeMap<String, (f32, f32, f32, f32)> {
        fn walk(elem: &xmltree::Element, boxes: &mut std::collections::BTreeMap<String, (f32, f32, f32, f32)>) {
            if elem.attributes.get("data-type").is_some_and(|t| t == "piece") {
                let id = elem.attributes.get("id").cloned().unwrap_or_default();
                let b = crate::bbox_from_group_geometry(elem).expect("piece geometry");
                boxes.insert(id, (b.min.x, b.min.y, b.max.x, b.max.y));
            } // if piece
            for child in elem.children.iter().filter_map(|n| n.as_element()) {
                walk(child, boxes);
            } // for child
        } // fn walk
        let mut boxes = std::collections::BTreeMap::new();
        walk(&doc.root, &mut boxes);
        boxes
    } // fn piece_boxes

    // @brief The Nested SVG export holds one group per size, largest size first,
    // 8 pieces each, every piece where the layout put it.
    #[test]
    fn jacket2_nested_svg_export_groups_pieces_by_size() {
        let nested = process_jacket2(true, 60.0, 300.0);
        let mut doc = nested.output_doc.clone();
        crate::layout_assembler::remove_color_blocks(&mut doc);
        let before = piece_boxes(&doc);

        assert_eq!(crate::size_groups::group_pieces_by_size(&mut doc), 26);
        let groups: Vec<&xmltree::Element> = doc.root.children.iter()
            .filter_map(|n| n.as_element())
            .filter(|e| e.attributes.get("data-type").is_some_and(|t| t == "size"))
            .collect();
        let sizes: Vec<&str> = groups.iter().map(|g| g.attributes["data-size"].as_str()).collect();
        let mut expected = jacket2_handoff().1;
        expected.reverse();
        assert_eq!(sizes, expected, "largest size first");
        for group in &groups {
            let size = &group.attributes["data-size"];
            let pieces: Vec<&xmltree::Element> = group.children.iter().filter_map(|n| n.as_element()).collect();
            assert_eq!(pieces.len(), 8, "size {size}");
            assert!(pieces.iter().all(|p| p.attributes.get("data-size") == Some(size)), "size {size}");
        } // for group
        assert!(!doc.to_string().contains(r#"data-type="piece-set""#), "no piece-set left");

        let after = piece_boxes(&doc);
        assert_eq!(after.len(), 208);
        for (id, b) in &before {
            let a = after[id];
            let moved = (a.0 - b.0).abs().max((a.1 - b.1).abs()).max((a.2 - b.2).abs()).max((a.3 - b.3).abs());
            assert!(moved < 0.01, "{id} moved by {moved}");
        } // for piece
    } // jacket2_nested_svg_export_groups_pieces_by_size

    // @brief Marker packs every size of every piece as its own unit, numbered by its piece-set.
    #[test]
    fn jacket2_marker_places_every_size() {
        let marker = process_jacket2(false, 60.0, 1500.0);
        assert!(marker.unplaced_labels.is_empty(), "unplaced: {:?}", marker.unplaced_labels);
        assert_eq!(placed_count(&marker), 208, "Marker: pieces x sizes");
        let svg = marker.output_doc.to_string();
        for set in 1..=8 {
            assert_eq!(svg.matches(&format!(r#"data-set-number="{set}""#)).count(), 26, "piece-set {set}: 26 sizes");
        } // for each piece-set
    } // jacket2_marker_places_every_size

    // @brief A Marker roll too short for jacket2 names each left-out piece once, numbered, with its sizes.
    #[test]
    fn jacket2_marker_warning_names_pieces_once() {
        let marker = process_jacket2(false, 48.0, 300.0);
        let lines = &marker.unplaced_labels;
        assert!(!lines.is_empty(), "a 300 in roll cannot hold every size");
        assert!(lines.len() <= 8, "one line per piece: {lines:?}");
        let names: Vec<&str> = lines.iter().map(|l| l.split(": ").next().unwrap_or_default()).collect();
        for (i, name) in names.iter().enumerate() {
            assert!(!names[i + 1..].contains(name), "{name} listed twice: {lines:?}");
            assert!(*name != "Detail" && !name.contains("(size "), "numbered piece name: {lines:?}");
        } // for each line
        // Each line ends with the sizes of that piece left out, all from the size list.
        let (_, sizes) = jacket2_handoff();
        for line in lines {
            let (_, list) = line.split_once(": ").expect("multisize line names its sizes");
            assert!(list.split(", ").all(|s| sizes.iter().any(|z| z == s)), "{line}");
        } // for each line
        let warning = crate::layout_views::tab_unplaced_problem(crate::layout_views::MARKER_LABEL, lines);
        assert_eq!(warning.lines().count(), lines.len() + 1, "{warning}");
    } // jacket2_marker_warning_names_pieces_once

    // @brief Lay out the jacket2 Marker tab on extended media.
    fn process_jacket2_extended(settings_json: &str) -> ProcessLayoutResult {
        let (input_dom, _) = jacket2_handoff();
        let init = do_initialize_layout(settings_json, Some(&input_dom))
            .expect("initialize_layout should succeed");

        let mut progress = |_pct: i32, _status: Option<&str>| {};
        do_process_layout(
            ProcessLayoutArgs {
                settings_json,
                input_dom: &input_dom,
                initial_layout_dom: &init.initial_dom,
                layout_h_px: init.h_px,
                nested_multisize: false,
                extend_media: true,
                split_oversized: false,
            },
            &mut progress,
        ).expect("process_layout should succeed on the jacket2 handoff")
    } // fn process_jacket2_extended

    // @brief Canvas height of an extended layout: the SVG, backgroundRect and
    // `layout_h_px` agree, and every placed piece is inside contentRect.
    // @return Content rectangle height in pixels.
    fn assert_canvas_holds_pieces(result: &ProcessLayoutResult) -> u32 {
        let doc = &result.output_doc;
        let attr = |id: &str, name: &str| -> u32 {
            doc.get_attr_by_id(id, name).and_then(|v| v.parse().ok()).expect("frame attribute")
        };
        let svg_h: u32 = doc.root.attributes.get("height").and_then(|v| v.parse().ok()).expect("svg height");
        assert_eq!(svg_h, result.layout_h_px, "svg height");
        assert_eq!(attr("backgroundRect", "height"), svg_h, "backgroundRect height");

        let svg_w: u32 = doc.root.attributes.get("width").and_then(|v| v.parse().ok()).expect("svg width");
        assert_eq!(attr("backgroundRect", "width"), svg_w, "backgroundRect width");

        let (content_x, content_w) = (attr("contentRect", "x"), attr("contentRect", "width"));
        let (content_y, content_h) = (attr("contentRect", "y"), attr("contentRect", "height"));
        let bbox: serde_json::Value = serde_json::from_str(&result.bbox_json).expect("valid JSON");
        for piece in bbox["pieces"].as_array().expect("pieces array") {
            let right = piece["x"].as_u64().unwrap() + piece["w"].as_u64().unwrap();
            assert!(right <= u64::from(content_x + content_w), "{} ends right of contentRect", piece["label"]);
            let bottom = piece["y"].as_u64().unwrap() + piece["h"].as_u64().unwrap();
            assert!(bottom <= u64::from(content_y + content_h), "{} ends below contentRect", piece["label"]);
        } // for each placed piece
        content_h
    } // fn assert_canvas_holds_pieces

    // @brief Marker on a 48 in × 500 in paper roll: 208 pieces need about 740 in.
    // The roll grows until every piece is placed, then ends at the lowest piece.
    #[test]
    fn jacket2_marker_extends_paper_roll() {
        let settings_json = serde_json::json!({
            "unit": "in",
            "mediaType": "paper",
            "paperType": "roll",
            "rollWidth": 48.0,
            "marginLeft": 0.5,
            "marginRight": 0.5,
            "marginTop": 0.5,
            "marginBottom": 0.5,
            "pieceGap": 0.125,
            "layoutMode": "alongGrainline"
        }).to_string();
        let marker = process_jacket2_extended(&settings_json);
        assert!(marker.unplaced_labels.is_empty(), "unplaced: {:?}", marker.unplaced_labels);
        assert_eq!(placed_count(&marker), 208, "Marker: pieces x sizes");

        let content_h = assert_canvas_holds_pieces(&marker);
        assert!(content_h > 500 * 96, "the roll grows past 500 in, content height {content_h} px");
    } // jacket2_marker_extends_paper_roll

    // @brief Marker on a 48 in × 48 in paper sheet: the sheet grows until every piece is placed.
    #[test]
    fn jacket2_marker_extends_paper_sheet() {
        let settings_json = serde_json::json!({
            "unit": "in",
            "mediaType": "paper",
            "paperType": "sheet",
            "pageWidth": 48.0,
            "pageHeight": 48.0,
            "marginLeft": 0.5,
            "marginRight": 0.5,
            "marginTop": 0.5,
            "marginBottom": 0.5,
            "pieceGap": 0.125,
            "layoutMode": "alongGrainline"
        }).to_string();
        let marker = process_jacket2_extended(&settings_json);
        assert!(marker.unplaced_labels.is_empty(), "unplaced: {:?}", marker.unplaced_labels);
        assert_eq!(placed_count(&marker), 208, "Marker: pieces x sizes");

        let content_h = assert_canvas_holds_pieces(&marker);
        assert!(content_h > 47 * 96, "the sheet grows past its length, content height {content_h} px");
    } // jacket2_marker_extends_paper_sheet

    // @brief Marker on a 10 in paper roll: the pieces are wider than the roll.
    // The roll widens to the right for the widest piece, keeps both side margins,
    // then grows in length until every piece is placed.
    #[test]
    fn jacket2_marker_widens_narrow_roll() {
        let settings_json = serde_json::json!({
            "unit": "in",
            "mediaType": "paper",
            "paperType": "roll",
            "rollWidth": 10.0,
            "marginLeft": 0.5,
            "marginRight": 0.25,
            "marginTop": 0.5,
            "marginBottom": 0.5,
            "pieceGap": 0.125,
            "layoutMode": "alongGrainline"
        }).to_string();
        let marker = process_jacket2_extended(&settings_json);
        assert!(marker.unplaced_labels.is_empty(), "unplaced: {:?}", marker.unplaced_labels);
        assert_eq!(placed_count(&marker), 208, "Marker: pieces x sizes");
        assert_canvas_holds_pieces(&marker);

        let doc = &marker.output_doc;
        let attr = |id: &str, name: &str| -> u32 {
            doc.get_attr_by_id(id, name).and_then(|v| v.parse().ok()).expect("frame attribute")
        };
        let (content_x, content_w) = (attr("contentRect", "x"), attr("contentRect", "width"));
        assert_eq!(content_x, 48, "contentRect keeps its x: the roll grows to the right");
        assert!(content_w > 888, "the roll widens past its 9.25 in (888 px) content width, got {content_w} px");
        assert_eq!(attr("backgroundRect", "width"), 48 + content_w + 24, "both side margins kept");
    } // jacket2_marker_widens_narrow_roll

    // @brief The DXF-ASTM warning dialog lists each piece once, by the name the user wrote.
    // The installed jacket2 hides every piece label and grainline, so all 8 pieces are listed.
    #[test]
    fn jacket2_missing_piece_data_lists_each_piece_once() {
        let layouts = [("nested", process_jacket2(true, 60.0, 300.0)), ("base size", process_jacket2_size(JACKET2_BASE_SIZE))];
        for (tab, layout) in layouts {
            let text = crate::exports::dxf_missing_piece_data_message(&layout.output_doc).expect("converts");
            let mut pieces: Vec<&str> =
                text.lines().filter_map(|l| l.strip_prefix("\u{2022} ")).map(|l| l.split(':').next().unwrap()).collect();
            pieces.sort_unstable();
            assert_eq!(
                pieces,
                vec!["Detail (3)", "Detail (4)", "Detail (5)", "Detail (6)", "Detail (7)", "Detail (8)", "Комір", "Подборт"],
                "{tab}: {text}"
            );
        } // for each tab
    } // jacket2_missing_piece_data_lists_each_piece_once

    // @brief The Base Size DXF-ASTM export: one block per piece, `Piece Name:` non-ASCII as
    // `\U+XXXX`, repeated names numbered, and `# n` texts on every piece that grades.
    #[test]
    fn jacket2_base_size_export_names_and_grades_each_piece() {
        let layout = process_jacket2_size(JACKET2_BASE_SIZE);
        let (input, sizes) = jacket2_handoff();
        let grading = crate::exports::DxfGradingSource { input, sizes, base_size: JACKET2_BASE_SIZE.to_string() };
        let (warnings, dxf, rul, _) = export_dxf_for_test(&layout, "jacket2_base_size", Some(grading));

        // Piece Name: system text, one per written block.
        let lines: Vec<&str> = dxf.lines().map(str::trim).collect();
        let mut names: Vec<&str> = lines.iter().filter_map(|l| l.strip_prefix("Piece Name:")).collect();
        assert!(names.iter().all(|n| n.is_ascii()), "Piece Name: is 7-bit ASCII");
        names.sort_unstable();
        let komir = r"\U+041A\U+043E\U+043C\U+0456\U+0440";
        let podbort = r"\U+041F\U+043E\U+0434\U+0431\U+043E\U+0440\U+0442";
        assert_eq!(
            names,
            vec!["Detail (3)", "Detail (4)", "Detail (5)", "Detail (6)", "Detail (7)", "Detail (8)", komir, podbort],
            "one block per piece"
        );
        let sizes: Vec<&str> = lines.iter().filter_map(|l| l.strip_prefix("Size Name:")).collect();
        assert!(sizes.len() == 8 && sizes.iter().all(|s| *s == JACKET2_BASE_SIZE), "{sizes:?}");
        assert!(lines.contains(&"Sample Size:500"), "sample size");

        // Detail (3) and (6) stay ungraded: their lines differ in shape between sizes.
        // Detail (3), sizes 660-700: the seam line reverses 0.2 mm past a corner, so the
        // corner has two turn points. Detail (6), sizes 220-340: the cut line's last seam
        // allowance corner is two vertices; from 360 it is one. Warnings name the piece as
        // the user wrote it, and the first size whose turn points differ from the sample size.
        let expected: Vec<String> = [(3, 660), (6, 220)]
            .iter()
            .map(|(n, size)| format!("Detail ({n}): not graded, size {size} has other turn points than size 500 on the cut line."))
            .collect();
        assert_eq!(warnings, expected);
        let reference_lines = lines.windows(4).filter(|w| w[0] == "0" && w[1] == "LINE" && w[2] == "8" && w[3] == "5").count();
        assert_eq!(reference_lines, 6, "one layer 5 line per graded piece");
        assert!(lines.contains(&"# 1"), "grade rule identifiers");
        let rul = rul.expect("six graded pieces write the rule table");
        assert!(rul.contains("SAMPLE SIZE:500") && rul.contains("NUMBER OF SIZES:26"), "{rul}");
    } // jacket2_base_size_export_names_and_grades_each_piece

    // @brief The import gives the jacket2 pieces unique names; the Base Size export writes
    // them as `Piece Name:`, names them in its dialogs, and still grades by piece-set.
    #[test]
    fn jacket2_assigned_names_reach_the_base_size_export() {
        let (mut input, sizes) = jacket2_handoff();
        let naming = crate::piece_naming::assign_unique_piece_names(&mut input);
        assert_eq!(naming, crate::piece_naming::PieceNaming { renamed: 208, warning: None }, "8 pieces in each of 26 sizes");
        let layout = process_jacket2_input_size(&input, JACKET2_BASE_SIZE);

        // The Missing Piece Data dialog names each piece by its assigned name.
        let text = crate::exports::dxf_missing_piece_data_message(&layout.output_doc).expect("converts");
        let mut listed: Vec<&str> =
            text.lines().filter_map(|l| l.strip_prefix("\u{2022} ")).map(|l| l.split(':').next().unwrap()).collect();
        listed.sort_unstable();
        let expected: Vec<String> = ["Подборт", "Комір", "Detail", "Detail", "Detail", "Detail", "Detail", "Detail"]
            .iter()
            .enumerate()
            .map(|(i, name)| format!("Size_500_Piece_{}_{name}", i + 1))
            .collect();
        let mut sorted = expected.clone();
        sorted.sort_unstable();
        assert_eq!(listed, sorted, "{text}");

        // `Piece Name:` escapes the non-ASCII characters; the numbers keep the Detail pieces apart.
        let grading = crate::exports::DxfGradingSource { input, sizes, base_size: JACKET2_BASE_SIZE.to_string() };
        let (warnings, dxf, rul, _) = export_dxf_for_test(&layout, "jacket2_assigned_names", Some(grading));
        let lines: Vec<&str> = dxf.lines().map(str::trim).collect();
        let mut names: Vec<&str> = lines.iter().filter_map(|l| l.strip_prefix("Piece Name:")).collect();
        names.sort_unstable();
        let mut written: Vec<String> = expected.iter().map(|n| seamly_svg2ezdxf::dxf_unicode_escape(n)).collect();
        written.sort_unstable();
        assert_eq!(names, written, "one block per piece");

        // The same two pieces stay ungraded, named as the dialog names them.
        let not_graded: Vec<String> = [(3, 660), (6, 220)]
            .iter()
            .map(|(n, size)| format!("Size_500_Piece_{n}_Detail: not graded, size {size} has other turn points than size 500 on the cut line."))
            .collect();
        assert_eq!(warnings, not_graded);
        assert!(rul.is_some(), "six graded pieces write the rule table");
    } // jacket2_assigned_names_reach_the_base_size_export

    // @brief Consecutive POLYLINE vertices of a DXF, as written, 0.01 mm apart or closer.
    // @return One "layer L: (x, y) then (x, y)" entry per pair; a closed polyline also pairs its last and first vertex.
    fn near_duplicate_vertices(dxf: &str) -> Vec<String> {
        let lines: Vec<&str> = dxf.lines().map(str::trim).collect();
        // (layer, closed, vertices) per polyline.
        let mut polylines: Vec<(String, bool, Vec<(f64, f64)>)> = Vec::new();
        let mut in_polyline = false;
        for pair in lines.chunks(2) {
            let [code, value] = pair else { break; };
            match (*code, *value) {
                ("0", "POLYLINE") => {
                    polylines.push((String::new(), false, Vec::new()));
                    in_polyline = true;
                } // POLYLINE
                ("0", "VERTEX") if in_polyline => polylines.last_mut().expect("open polyline").2.push((f64::NAN, f64::NAN)),
                ("0", "SEQEND") => in_polyline = false,
                ("8", layer) if in_polyline => polylines.last_mut().expect("open polyline").0 = layer.to_string(),
                ("70", flags) if in_polyline => {
                    polylines.last_mut().expect("open polyline").1 = flags.parse::<i32>().unwrap_or(0) & 1 == 1;
                } // flags
                ("10", x) | ("20", x) if in_polyline => {
                    let v = x.parse::<f64>().expect("coordinate");
                    if let Some(last) = polylines.last_mut().expect("open polyline").2.last_mut() {
                        if *code == "10" { last.0 = v } else { last.1 = v }
                    } // if vertex
                } // coordinate
                _ => {}
            } // match pair
        } // for each group code pair

        let mut found = Vec::new();
        for (layer, closed, vertices) in &polylines {
            let n = vertices.len();
            let pairs = if *closed && n > 2 { n } else { n.saturating_sub(1) };
            for i in 0..pairs {
                let (a, b) = (vertices[i], vertices[(i + 1) % n]);
                if (a.0 - b.0).hypot(a.1 - b.1) <= 0.01 + 1e-9 {
                    found.push(format!("layer {layer}: ({}, {}) then ({}, {})", a.0, a.1, b.0, b.1));
                } // if near duplicate
            } // for each pair
        } // for each polyline
        found
    } // fn near_duplicate_vertices

    // @brief No two consecutive written vertices of a jacket2 DXF-ASTM export are 0.01 mm apart or closer.
    // A reader reports them as duplicate vertices, e.g. Подборт layer 87 `(333.01, 626.21)` then `(333.01, 626.22)`.
    #[test]
    fn jacket2_dxf_has_no_near_duplicate_vertices() {
        let layout = process_jacket2_size(JACKET2_BASE_SIZE);
        let (input, sizes) = jacket2_handoff();
        let grading = crate::exports::DxfGradingSource { input, sizes, base_size: JACKET2_BASE_SIZE.to_string() };
        let (_, dxf, _, _) = export_dxf_for_test(&layout, "jacket2_vertices", Some(grading));
        let found = near_duplicate_vertices(&dxf);
        assert!(found.is_empty(), "{} near-duplicate vertices: {:?}", found.len(), &found[..found.len().min(10)]);

        let nested = process_jacket2(true, 60.0, 300.0);
        let (_, dxf, _, _) = export_dxf_for_test(&nested, "jacket2_nested_vertices", None);
        let found = near_duplicate_vertices(&dxf);
        assert!(found.is_empty(), "Nested: {} near-duplicate vertices: {:?}", found.len(), &found[..found.len().min(10)]);
    } // jacket2_dxf_has_no_near_duplicate_vertices
} // mod tests
