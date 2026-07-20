// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file tiling.rs
// @brief Tiling layout calculation and DOM construction.
//
// Implements the workflow defined in docs/tiling-docs/TILING_CALCULATION_WORKFLOW.md.
// Used by `initialize_layout` in lib.rs when `paper_type == "tiled"`.
//
// Public API:
//   `measurement_to_px(s)`          — convert an SVG length string to pixels
//   `TileDimensions`                — computed tile grid dimensions
//   `compute_tile_dims(w, h, s)`    — calculate tile grid from input DOM size + settings
//   `create_initial_tiled_layout_dom(td)` — build the initial tiled layout DOM
//   `widest_piece_tile_cols(...)`   — minimum tile columns needed for the widest piece
//   `pick_best_tiled_candidate(...)` — evaluate candidate widths and return the best
//   `TiledCandidate`                — one scored tiled-layout candidate

use xmltree::{Element, XMLNode};

use crate::layout_settings::{LayoutSettings, LAYOUT_PPI};

// ---------------------------------------------------------------------------
// Tile size lookup table
// ---------------------------------------------------------------------------

// @brief Built-in tile page sizes matching SettingsModel TILE_SIZES[] in C++.
// @details Columns: (name, imperial_w_in, imperial_h_in, metric_w_mm, metric_h_mm)
const TILE_SIZES_DATA: &[(&str, f64, f64, f64, f64)] = &[
    // (name,    imp_w,  imp_h,  met_w,  met_h)
    ("None",     0.0,    0.0,    0.0,    0.0),
    ("Letter",   8.5,   11.0,  216.0,  279.0),
    ("Legal",    8.5,   14.0,  216.0,  356.0),
    ("Ledger",  11.0,   17.0,  279.0,  432.0),
    ("A3",      11.69,  16.54, 297.0,  420.0),
    ("A4",       8.27,  11.69, 210.0,  297.0),
    ("A5",       5.83,   8.27, 148.0,  210.0),
]; // TILE_SIZES_DATA

// ---------------------------------------------------------------------------
// measurement_to_px
// ---------------------------------------------------------------------------

// @brief Convert an SVG length attribute string to pixels at 96 px/in.
//
// Handles unit suffixes on the SVG <svg> width/height attributes:
//   "mm" suffix: value / 25.4 * 96
//   "cm" suffix: value / 2.54  * 96
//   "in" suffix: value * 96
//   "px" suffix or no unit: value as-is
//
// Returns 0.0 if the numeric portion cannot be parsed.
//
// @param s  SVG attribute string, e.g. "210mm", "8.27in", "816", "816px".
// @return   Pixel equivalent as f64.
pub fn measurement_to_px(s: &str) -> u32 {
    let s = s.trim();

    // Identify unit suffix and strip it to get the numeric string.
    let (numeric_str, unit) = if s.ends_with("mm") {
        (&s[..s.len() - 2], "mm")
    } else if s.ends_with("cm") {
        (&s[..s.len() - 2], "cm")
    } else if s.ends_with("in") {
        (&s[..s.len() - 2], "in")
    } else if s.ends_with("px") {
        (&s[..s.len() - 2], "px")
    } else {
        (s, "") // no unit — already in pixels
    }; // strip unit suffix

    // convert str to f64, return 0 if parse fails
    let value: f64 = match numeric_str.trim().parse() {
        Ok(v) => v, // get f64 value from string for unit conversion below
        Err(_) => return 0, // return u32 0 if parse fails
    };

    // convert values to pixels as f64 for accurate unit conversion, will round to u32 at the end
    let value_f = value as f64;
    let px: f64 = match unit {
        "mm" => value_f / 25.4 * LAYOUT_PPI,  // mm → in → px as f64
        "cm" => value_f / 2.54  * LAYOUT_PPI, // cm → in → px as f64
        "in" => value_f * LAYOUT_PPI,          // in → px
        _    => value_f,                        // "px" or unknown — already pixels
    };

    // return pixels rounded up to nearest whole number as integer u32
    px.round() as u32
} // fn measurement_to_px

// ---------------------------------------------------------------------------
// tile_size_px
// ---------------------------------------------------------------------------

// @brief Look up a tile size by name and return (widthPx, heightPx).
//
// Selects the imperial columns when unit=="in", metric columns when unit=="mm",
// and metric÷10 when unit=="cm", then multiplies by LAYOUT_PPI.
// Returns None for "None" or any unknown name.
//
// @param name  Tile size name, e.g. "Letter", "A4".
// @param unit  Active user unit: "in" | "mm" | "cm".
// @return      (width_px, height_px) or None.
fn tile_size_px(name: &str, unit: &str) -> Option<(f64, f64)> {
    for &(n, imp_w, imp_h, met_w, met_h) in TILE_SIZES_DATA {
        if n != name {
            continue; // skip non-matching entry
        } // if name mismatch
        if n == "None" || (imp_w == 0.0 && met_w == 0.0) {
            return None; // "None" entry has no dimensions
        } // if None entry
        let (w_px, h_px) = match unit {
            "mm" => (met_w / 25.4 * LAYOUT_PPI, met_h / 25.4 * LAYOUT_PPI), // mm → px
            "cm" => (met_w / 10.0 / 2.54 * LAYOUT_PPI, met_h / 10.0 / 2.54 * LAYOUT_PPI), // mm→cm→px
            _    => (imp_w * LAYOUT_PPI, imp_h * LAYOUT_PPI),                // "in" or unknown → px
        }; // match unit
        return Some((w_px, h_px));
    } // for entry

    // return None if name not found or if "None" entry
    None
} // fn tile_size_px

// ---------------------------------------------------------------------------
// TileDimensions
// ---------------------------------------------------------------------------

// @brief Computed tile grid dimensions passed from `compute_tile_dims` to
//        `create_initial_tiled_layout_dom`.
//
// All pixel values are f64 at 96 px/in.
#[derive(Debug, Clone, serde::Deserialize)]
pub struct TileDimensions {
    // Number of tile columns and rows needed to cover the input DOM area.
    pub tile_cols: u32,
    pub tile_rows: u32,

    // Usable area of one tile page (paper size minus margins).
    pub trim_tile_w_px: u32,
    pub trim_tile_h_px: u32,

    // Input DOM pixel dimensions — used for SVG root, backgroundRect, contentRect.
    pub input_dom_w_px: u32,
    pub input_dom_h_px: u32,

    // Full tile grid pixel dimensions — carried for future PDF tiling use.
    // layoutWidthPx  = tile_cols * trim_tile_w_px + ml + mr
    // layoutHeightPx = tile_rows * trim_tile_h_px + mt + mb
    #[allow(dead_code)]
    pub layout_w_px: u32,
    #[allow(dead_code)]
    pub layout_h_px: u32,

    // Margins in pixels.
    pub margin_left_px:   u32,
    pub margin_right_px:  u32,
    pub margin_top_px:    u32,
    pub margin_bottom_px: u32,
} // struct TileDimensions

// ---------------------------------------------------------------------------
// compute_tile_dims
// ---------------------------------------------------------------------------

// @brief Calculate the tile grid dimensions from input DOM size and settings.
//
// Implements the Calculations section of TILING_CALCULATION_WORKFLOW.md.
//
// Steps:
//   1. Resolve tile page size via tile_size_px(); error if "None" or unknown.
//   2. Convert margins from userUnit to pixels via to_inches() * LAYOUT_PPI.
//   3. trimTileW = paperSizeWPx - ml - mr;  trimTileH = paperSizeHPx - mt - mb.
//   4. tileCols = ceil((inputW - ml - mr) / trimTileW).
//   5. tileRows = ceil((inputH - mt - mb) / trimTileH).
//   6. layoutW  = tileCols * trimTileW + ml + mr.
//   7. layoutH  = tileRows * trimTileH + mt + mb.
//
// @param input_w_px  Input DOM width in pixels (from measurement_to_px).
// @param input_h_px  Input DOM height in pixels (from measurement_to_px).
// @param settings    Parsed LayoutSettings.
// @return            TileDimensions on success; Err string on invalid tile size.
pub fn compute_tile_dims(
    input_w_px: u32,
    input_h_px: u32,
    settings: &LayoutSettings,
) -> Result<TileDimensions, String> {
    // Keep calculations in f64 until final then round and convert to u32 integer pixels

    // --- 0 convert input dimensions to f64 for accurate calculations, results will round to u32 at the end ---
    let input_w_px_f = input_w_px as f64;
    let input_h_px_f = input_h_px as f64;

    // --- 1 get tile page size f64 ---
    let (paper_size_w_px_f, paper_size_h_px_f) =
        tile_size_px(&settings.tile_size, &settings.unit)
            .ok_or_else(|| format!(
                "Tile size '{}' is not valid or has no dimensions. \
                 Select a tile size other than 'None'.",
                settings.tile_size
            ))?; // if tile_size invalid

    // --- 2 get margins f64 ---
    let (ml_f, mr_f, mt_f, mb_f) = {
        let (ml, mr, mt, mb) = settings.margin_px();
        (ml as f64, mr as f64, mt as f64, mb as f64)
    }; // margins

    // --- 3 trim tile dimensions f64 ---
    // Landscape keeps the legacy rotated-tile behavior; portrait preserves the natural size.
    let is_portrait = settings.tile_orientation == "portrait";
    let (tile_w_px_f, tile_h_px_f) = if is_portrait {
        (paper_size_w_px_f, paper_size_h_px_f)
    } else {
        (paper_size_h_px_f, paper_size_w_px_f)
    };
    let trim_tile_w_px_f: f64 = tile_w_px_f - ml_f - mr_f;
    let trim_tile_h_px_f: f64 = tile_h_px_f - mt_f - mb_f;

    if trim_tile_w_px_f <= 0.0 || trim_tile_h_px_f <= 0.0 {
        return Err(format!(
            "Tile margins exceed tile page size \
             (trimTileW={trim_tile_w_px_f:.1}px, trimTileH={trim_tile_h_px_f:.1}px). \
             Reduce margins or choose a larger tile size."
        )); // if margins exceed tile
    } // if trim tile invalid

    // --- 4 tile columns (ceiling division) f64 ---
    let width1: f64 = (input_w_px_f - ml_f - mr_f) / trim_tile_w_px_f;
    let mut tile_cols_f: f64 = width1.floor();
    tile_cols_f = if (width1 - tile_cols_f) > 0.0 {
        tile_cols_f + 1.0
    } else {
        tile_cols_f
    }; // ceiling

    // --- 5 tile rows (ceiling division) ---
    let height1: f64 = (input_h_px_f - mt_f - mb_f) / trim_tile_h_px_f;
    let mut tile_rows_f = height1.floor();
    tile_rows_f = if (height1 - tile_rows_f) > 0.0 {
        tile_rows_f + 1.0
    } else {
        tile_rows_f
    }; // ceiling

    // --- 6 convert f64 to u32
    // 6a tile_cols and tile_rows to u32, ensure at least 1 col and 1 row ---
    let mut tile_cols: u32 = tile_cols_f.round() as u32;
    let mut tile_rows: u32 = tile_rows_f.round() as u32;
    // Ensure at least one row and one column.
    tile_cols = tile_cols.max(1);
    tile_rows = tile_rows.max(1);
    // 6b trim tile dimensions to u32 pixels
    let trim_tile_w_px: u32 = trim_tile_w_px_f.round() as u32;
    let trim_tile_h_px: u32 = trim_tile_h_px_f.round() as u32;
    // 6c margins to u32 pixels
    let ml: u32 = ml_f.round() as u32;
    let mr: u32 = mr_f.round() as u32;
    let mt: u32 = mt_f.round() as u32;
    let mb: u32 = mb_f.round() as u32;
    // 6d input_dom dimensions to u32 pixels
    let input_w_px: u32 = input_w_px_f.round() as u32;
    let input_h_px: u32 = input_h_px_f.round() as u32;

    // --- 7 full grid layout dimensions u32 ---
    // layout_width = columns*tilewidth + marginLeft + marginRight
    let layout_w_px = tile_cols * trim_tile_w_px + ml + mr;
    // layout_height = rows*tileheight + marginTop + marginBottom
    let layout_h_px = tile_rows * trim_tile_h_px + mt + mb;

    // return updated TileDimensions struct with u32 px integer values
    Ok(TileDimensions {
        tile_cols,
        tile_rows,
        trim_tile_w_px,
        trim_tile_h_px,
        input_dom_w_px: input_w_px,
        input_dom_h_px: input_h_px,
        layout_w_px,
        layout_h_px,
        margin_left_px:   ml,
        margin_right_px:  mr,
        margin_top_px:    mt,
        margin_bottom_px: mb,
    }) // Ok
} // fn compute_tile_dims

// ---------------------------------------------------------------------------
// create_initial_tiled_layout_dom
// ---------------------------------------------------------------------------

// @brief Build the initial tiled layout DOM from computed tile dimensions.
//
// Implements DOM creation steps 1–11 from TILING_CALCULATION_WORKFLOW.md.
//
// SVG structure produced:
//   <svg id="layout" width=inputDomWidthPx height=inputDomHeightPx>
//     <defs>
//       <marker id="tile" viewBox="0 0 tw th" refX=0 refY=0
//               markerWidth=tw markerHeight=th">
//         <path id="tileRect" d="M 0,0 h 768.0 v 1008 h -768.0 Z"
//               fill="none" stroke="black" stroke-width="z
//       </marker>
//     </defs>
//     <g id="Rectangles">
//       <rect id="backgroundRect" .../>
//       <rect id="contentRect" .../>
//       <g id="tiledRects">
//         <path id="row_1" marker-start="url(#tile)" marker-mid="url(#tile)"
//               marker-end="url(#tile)" d="M x0,y0 M x1,y0 ..."/>
//         ...one path per row...
//       </g>
//     </g>
//   </svg>
//
// @param td  Computed TileDimensions from compute_tile_dims.
// @return    New svg_dom::Document ready to store as layout_dom.
pub fn create_initial_tiled_layout_dom(td: &TileDimensions) -> svg_dom::Document {

    // debug print that we've started create_initial_tiled_layout_dom()
    log::debug!("create_initial_tiled_layout_dom: 1 TileDimensions: {td:?}");

    // get dimensions
    let w   = td.input_dom_w_px;
    let h   = td.input_dom_h_px;
    let ml  = td.margin_left_px;
    let mr  = td.margin_right_px;
    let mt  = td.margin_top_px;
    let mb  = td.margin_bottom_px;
    let tw  = td.trim_tile_w_px;
    let th  = td.trim_tile_h_px;

    // --- 1 create new SVG root ---
    let mut svg_root = Element {
        name:       "svg".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  Some("http://www.w3.org/2000/svg".to_string()),
        prefix:     None,
        namespaces: None,
    };
    svg_root.attributes.insert("xmlns".to_string(),  "http://www.w3.org/2000/svg".to_string());
    svg_root.attributes.insert("id".to_string(),     "layout".to_string());
    svg_root.attributes.insert("width".to_string(),  format!("{}", w));
    svg_root.attributes.insert("height".to_string(), format!("{}", h));

    // --- 2 create one tile rectangle as <path id="tileMarkerPath">, will be added to <marker id="tileMarker"> in <defs> ---
    let mut tile_rect = Element {
        name:       "path".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    tile_rect.attributes.insert("id".to_string(),           "tileMarkerPath".to_string());
    tile_rect.attributes.insert(
        "d".to_string(),
        format!("M 0,0 h {} v {} h -{} Z", tw, th, tw)
    );
    tile_rect.attributes.insert("fill".to_string(),         "none".to_string());
    tile_rect.attributes.insert("stroke".to_string(),       "black".to_string());
    tile_rect.attributes.insert("stroke-width".to_string(), "1".to_string());

    // --- 3 create <marker>, add <path> to <marker>, will be added to <defs>
    let mut marker = Element {
        name:       "marker".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    marker.attributes.insert("id".to_string(),           "tileMarker".to_string());
    marker.attributes.insert("viewBox".to_string(),      "0 0 {tw} {th}".to_string());
    marker.attributes.insert("refX".to_string(),         "0".to_string());
    marker.attributes.insert("refY".to_string(),         "0".to_string());
    marker.attributes.insert("markerWidth".to_string(),  tw.to_string());
    marker.attributes.insert("markerHeight".to_string(), th.to_string());
    marker.attributes.insert("orient".to_string(),       "0".to_string()); // angle relative to path where marker appears
    // add tile_rect <path> to <marker>
    marker.children.push(XMLNode::Element(tile_rect));

    // --- 4 create <defs> group, add <marker> to <defs>, add <defs> to <svg> root
    let mut defs = Element {
        name:       "defs".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    // add <marker> to <defs>
    defs.children.push(XMLNode::Element(marker));
    // add <defs> to <svg> root
    svg_root.children.push(XMLNode::Element(defs));

    // --- 5 create <g id="Rectangles">, will be added to <svg> ---
    //       This group contains the backgroundRect, contentRect, and tiledRects groups

    let mut rects_group = Element {
        name:       "g".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    rects_group.attributes.insert("id".to_string(), "Rectangles".to_string());

    // --- 6 create <g id="tiledRects">, will contain one <path> per tile row, will be added to <Rectangles> group ---
    //
    // Each row path encodes one x,y vertex per column, using M (moveTo) for each coordinate, so no line is created between vertices.
    // The tile <marker> fires at each vertex, drawing one tile rectangle per column in the row
    // Requires marker to be drawn at the path start, all midpoints on the path, and at the end of the path
    // At angle '0' relative to the path
    // Positions the tile's (0,0) coord at the vertex, so the tile extends to the right and down from the vertex.
    // Result - Draws one tile rectangle per column in the row, at the angle of the path.
    let mut tiled_rects_group = Element {
        name:       "g".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    tiled_rects_group.attributes.insert("id".to_string(), "tiledRects".to_string());

    // --- 7 create one <path> per row, add to <g id="tiledRects"> ---
    let min_x = ml;
    let min_y = mt;

    for row_num in 0..td.tile_rows {
        // rowNum is 1-based in the spec; tileY uses the 0-based index for the calculation.
        let tile_y = min_y + row_num * th;
        let row_id = format!("row_{}", row_num + 1); // 1-based id

        // Build the path d string: "M x0,y  L x1,y  L x2,y ..." for this row
        // First vertex uses M; subsequent vertices use L.
        let mut dstr = String::new();
        for col_num in 0..td.tile_cols {
            let tile_x = min_x + col_num * tw; // colNum resets to 0 each row
            dstr.push_str(&format!("M {tile_x},{tile_y} ")); // move to next tile
        } // for col_num

        // create the <path> element
        let mut path_elem = Element {
            name:       "path".to_string(),
            attributes: Default::default(),
            children:   Vec::new(),
            namespace:  None,
            prefix:     None,
            namespaces: None,
        };
        // add attributes to <path> element
        path_elem.attributes.insert("id".to_string(),           row_id.to_string());
        path_elem.attributes.insert("fill".to_string(),         "none".to_string());
        path_elem.attributes.insert("stroke".to_string(),       "black".to_string());
        path_elem.attributes.insert("stroke-width".to_string(), "1".to_string());
        path_elem.attributes.insert("marker-start".to_string(), "url(#tileMarker)".to_string());
        path_elem.attributes.insert("marker-mid".to_string(),   "url(#tileMarker)".to_string());
        path_elem.attributes.insert("marker-end".to_string(),   "url(#tileMarker)".to_string());
        path_elem.attributes.insert("d".to_string(),            dstr.to_string());

        // add this row's <path> to <g id='tiledRects>, add tiledRects to Rectangles group after backgroundRect and contentRect are added
        tiled_rects_group.children.push(XMLNode::Element(path_elem));
    } // for row_num


    // --- 7 Update <svg> width and height to calculated layout width and height ---
    // layoutWidthPx  = tileCols * trimTileW + ml + mr
    // layoutHeightPx = tileRows * trimTileH + mt + mb
    // These dimensions are needed for future PDF tiling, and also ensures the SVG root fully contains the tiled layout.
    let layout_w_px = td.trim_tile_w_px * td.tile_cols + ml + mr;
    let layout_h_px = td.trim_tile_h_px * td.tile_rows + mt + mb;
    svg_root.attributes.insert("width".to_string(),  layout_w_px.to_string());
    svg_root.attributes.insert("height".to_string(), layout_h_px.to_string());

    // --- 8 create <rect id="backgroundRect">, add to <g id="Rectangles"> ---
    let mut bg_rect = Element {
        name:       "rect".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    bg_rect.attributes.insert("id".to_string(),           "backgroundRect".to_string());
    bg_rect.attributes.insert("x".to_string(),            "0".to_string());
    bg_rect.attributes.insert("y".to_string(),            "0".to_string());
    bg_rect.attributes.insert("width".to_string(),        layout_w_px.to_string());
    bg_rect.attributes.insert("height".to_string(),       layout_h_px.to_string());
    bg_rect.attributes.insert("fill".to_string(),         "white".to_string());
    bg_rect.attributes.insert("stroke".to_string(),       "black".to_string());
    bg_rect.attributes.insert("stroke-width".to_string(), "1".to_string());
    // add backgroundRect to Rectangles group
    rects_group.children.push(XMLNode::Element(bg_rect));

    // --- 9 create <rect id="contentRect">, add to <g id='Rectangles'> ---
    let content_w = layout_w_px - ml - mr;
    let content_h = layout_h_px - mt - mb;
    let mut content_rect = Element {
        name:       "rect".to_string(),
        attributes: Default::default(),
        children:   Vec::new(),
        namespace:  None,
        prefix:     None,
        namespaces: None,
    };
    content_rect.attributes.insert("id".to_string(),           "contentRect".to_string());
    content_rect.attributes.insert("x".to_string(),            ml.to_string());
    content_rect.attributes.insert("y".to_string(),            mt.to_string());
    content_rect.attributes.insert("width".to_string(),        content_w.to_string());
    content_rect.attributes.insert("height".to_string(),       content_h.to_string());
    content_rect.attributes.insert("fill".to_string(),         "none".to_string());
    content_rect.attributes.insert("stroke".to_string(),       "black".to_string());
    content_rect.attributes.insert("stroke-width".to_string(), "1".to_string());
    // add contentRect to Rectangles group
    rects_group.children.push(XMLNode::Element(content_rect));

    // --- 10 add tiledRects to Rectangles group, add Rectangles group to svg root ---
    rects_group.children.push(XMLNode::Element(tiled_rects_group));
    svg_root.children.push(XMLNode::Element(rects_group));

    // debug message to end
    log::debug!("create_initial_tiled_layout_dom: 2 SVG root created with width={}px height={}px", layout_w_px, layout_h_px);

    // --- 11 return tiled svg document as the initial layout_dom ---
    svg_dom::Document { root: svg_root }

} // fn create_initial_tiled_layout_dom

// ---------------------------------------------------------------------------
// Tiled candidate selection
// ---------------------------------------------------------------------------
//
// When the user picks paper_type="tiled", packing the pieces into the initial
// content rect (whose width is derived from the input SVG) can leave the
// layout wide and short — pieces along the top, wasted tiles along the bottom.
// `pick_best_tiled_candidate` tries several bin widths in multiples of the
// trimmed tile width and returns the packing with the fewest non-empty tiles
// (with smallest unused area as tiebreaker).
//
// Rotation support (not yet enabled — see docs/layout-docs/LAYOUT_ROTATION_PLAN.md)
// plugs in at two seams:
//   1. `widest_piece_tile_cols` — the floor depends on the narrowest rotated
//      orientation of each piece.
//   2. Inside `packing::pack_maxrects` — rotation is a per-placement
//      trial, not an outer loop, so this function's structure does not change.

// @brief One tiled-layout candidate evaluated at a specific bin width.
//
// Scored by (non_empty_tiles, unused_area_px2) in that priority order.
#[derive(Debug, Clone)]
pub struct TiledCandidate {
    // Number of trimmed-tile columns in the bin.
    pub tile_cols: u32,
    // Number of trimmed-tile rows needed to contain all placed pieces.
    pub tile_rows: u32,
    // Placements returned by pack_maxrects, relative to the bin origin (0, 0).
    pub placements: Vec<packing::Placed>,
    // Count of trimmed-tile cells touched by at least one piece.  A piece that
    // straddles a tile boundary counts toward every tile it overlaps, since all
    // those tiles must be printed to reproduce the piece.
    pub non_empty_tiles: u32,
    // Unused area (in px²) summed over non-empty tiles only.
    pub unused_area_px2: u64,
} // struct TiledCandidate

// @brief Minimum tile-column count that can hold the widest piece.
//
// Returned value is the floor of the candidate-width search: any smaller
// bin width would make pack_maxrects fail with TooLarge for the widest piece.
//
// Rotation seam: this function currently uses the verticalized bounding-box
// width.  When rotation lands (see LAYOUT_ROTATION_PLAN.md):
//   - 180° flip only      → no change (bbox width unchanged).
//   - 90° included        → use `min(piece.w, piece.h)` per piece.
//   - 45° included        → use the rotated bbox width at each allowed angle.
//
// @param pieces        Extracted piece rectangles in layout pixels.
// @param trim_tile_w_px Trimmed tile width in pixels.
// @param gap_px        Inter-piece clearance used by pack_maxrects.
// @return              At-least-1 tile-column count.
pub fn widest_piece_tile_cols(
    pieces: &[packing::Rect],
    trim_tile_w_px: u32,
    gap_px: u32,
) -> u32 {
    // No pieces → no packing, but keep the floor at one column to avoid a zero bin.
    if pieces.is_empty() || trim_tile_w_px == 0 {
        return 1; // degenerate — caller will handle empty-pieces case separately
    } // if pieces empty

    // Find the widest piece, then pad by gap so a placement at x=0 still leaves clearance.
    let widest = pieces.iter().map(|r| r.w).max().unwrap_or(0);
    let needed = widest.saturating_add(gap_px);

    // Ceiling division: smallest whole-tile count that covers `needed` pixels.
    let cols = (needed + trim_tile_w_px - 1) / trim_tile_w_px;
    cols.max(1) // clamp to >= 1 so an empty-ish piece set never produces a zero bin
} // fn widest_piece_tile_cols

// @brief Count how many trimmed-tile cells are touched by any placed piece.
//
// A piece straddling a boundary increments every tile it overlaps: the user
// must print each of those tiles to reproduce the piece, so all of them are
// "non-empty" from the tiling-output perspective.
//
// @param placements      Placements in bin-local pixel coordinates.
// @param tile_cols       Tile-column count of the bin.
// @param tile_rows       Tile-row count of the bin.
// @param trim_tile_w_px  Trimmed tile width.
// @param trim_tile_h_px  Trimmed tile height.
// @return                Number of non-empty tile cells.
fn count_non_empty_tiles(
    placements: &[packing::Placed],
    tile_cols: u32,
    tile_rows: u32,
    trim_tile_w_px: u32,
    trim_tile_h_px: u32,
) -> u32 {
    // Guard against degenerate inputs — occupied grid would be zero-sized.
    if tile_cols == 0 || tile_rows == 0 || trim_tile_w_px == 0 || trim_tile_h_px == 0 {
        return 0; // nothing to count
    } // if degenerate

    // Bitset of (row, col) cells; index = row * cols + col.
    let mut occupied = vec![false; (tile_cols as usize) * (tile_rows as usize)];

    for p in placements {
        if p.w == 0 || p.h == 0 { continue; } // zero-size piece → touches nothing

        // Inclusive tile ranges covered by this piece (top-left to bottom-right).
        // Subtract 1 from (x+w) and (y+h) so a piece that ends exactly on a
        // boundary does not erroneously claim the tile past the edge.
        let x0 = p.x / trim_tile_w_px;
        let y0 = p.y / trim_tile_h_px;
        let x1 = (p.x + p.w - 1) / trim_tile_w_px;
        let y1 = (p.y + p.h - 1) / trim_tile_h_px;

        // Clamp to grid bounds in case a placement spills slightly past the last tile.
        let x1 = x1.min(tile_cols - 1);
        let y1 = y1.min(tile_rows - 1);

        for ty in y0..=y1 {
            for tx in x0..=x1 {
                occupied[(ty * tile_cols + tx) as usize] = true;
            } // for tx
        } // for ty
    } // for p in placements

    occupied.iter().filter(|&&b| b).count() as u32
} // fn count_non_empty_tiles

// @brief Evaluate candidate bin widths and return the best tiled packing.
//
// For each candidate `cols` in `[floor_cols, ceil_cols]`:
//   1. Pack into a (cols * trim_tile_w_px) × (generous sentinel height) bin.
//   2. Derive the smallest `tile_rows` that contains all placements.
//   3. Score: primary = non-empty tiles, secondary = unused area in those tiles.
// The scan stops early if two consecutive widths fail to reduce the non-empty
// tile count — further widths rarely improve things and each pack run is the
// dominant cost.
//
// A TooLarge error at a given width is treated as "skip and continue": a wider
// candidate may still fit the oversize piece.  Only if every candidate fails
// does the function return an error.
//
// Rotation seam: each candidate width is evaluated through
// `packing::pack_pieces`, which dispatches between the rectangle packer
// and the polygon-tight stub based on `trial_angles_deg`.
//
// @param pieces            Piece rectangles (pixels).
// @param trim_tile_w_px    Trimmed tile width (pixels).
// @param trim_tile_h_px    Trimmed tile height (pixels).
// @param floor_cols        Minimum candidate width in tile columns (inclusive).
// @param ceil_cols         Maximum candidate width in tile columns (inclusive).
// @param gap_px            Inter-piece clearance forwarded to the packer.
// @param trial_angles_deg  Per-piece rotation trial set in degrees (see
//                          `LayoutSettings::rotation_trial_set_deg`).
// @return                  Best candidate; Err if no candidate could pack all pieces.
pub fn pick_best_tiled_candidate(
    pieces: &[packing::Rect],
    trim_tile_w_px: u32,
    trim_tile_h_px: u32,
    floor_cols: u32,
    ceil_cols: u32,
    gap_px: u32,
    trial_angles_deg: &[u16],
) -> Result<TiledCandidate, packing::PackError> {
    // Generous height sentinel — 500 inches at 96 dpi, rounded up to a tile multiple
    // so later row-trim math stays aligned with the tile grid.
    let max_h_px = {
        let raw = (500.0 * LAYOUT_PPI) as u32;
        if trim_tile_h_px == 0 {
            raw // should not happen — caller validates tile dims
        } else {
            ((raw + trim_tile_h_px - 1) / trim_tile_h_px) * trim_tile_h_px
        }
    };

    // Clamp the search window to sane values: floor >= 1, ceil >= floor.
    let floor = floor_cols.max(1);
    let ceil  = ceil_cols.max(floor);

    // Track best-so-far and early-exit streak of non-improving tile counts.
    let mut best: Option<TiledCandidate> = None;
    let mut last_count: Option<u32> = None;
    let mut no_improve_streak: u32 = 0;

    // Remember the last error so we can return something meaningful if every candidate fails.
    let mut last_err: Option<packing::PackError> = None;

    for cols in floor..=ceil {
        let bin_w = cols.saturating_mul(trim_tile_w_px);

        // Run the dispatching packer into a tall sentinel bin.  NoSpace should
        // not trip with a 500-inch height; TooLarge at narrow widths can —
        // skip to a wider candidate rather than bail.
        let (placements, _debug_rects) =
            match packing::pack_pieces(bin_w, max_h_px, gap_px, pieces, trial_angles_deg) {
                Ok(r)  => r,
                Err(e) => {
                    last_err = Some(e);
                    continue; // try the next (wider) candidate
                } // Err
            }; // match pack_pieces

        // Derive the minimum row count needed for this packing.  Ceiling-divide
        // the deepest piece bottom by trim tile height.  At least one row.
        let max_bottom = placements.iter().map(|p| p.y + p.h).max().unwrap_or(0);
        let tile_rows = if trim_tile_h_px == 0 {
            1
        } else {
            ((max_bottom + trim_tile_h_px - 1) / trim_tile_h_px).max(1)
        };

        // Score: fewest non-empty tiles first; smallest unused area within them as tiebreaker.
        let non_empty = count_non_empty_tiles(&placements, cols, tile_rows, trim_tile_w_px, trim_tile_h_px);
        let tile_area_px2 = trim_tile_w_px as u64 * trim_tile_h_px as u64;
        let total_tile_area = non_empty as u64 * tile_area_px2;
        let piece_area: u64 = placements.iter().map(|p| p.w as u64 * p.h as u64).sum();
        let unused = total_tile_area.saturating_sub(piece_area);

        let cand = TiledCandidate {
            tile_cols: cols,
            tile_rows,
            placements,
            non_empty_tiles: non_empty,
            unused_area_px2: unused,
        };

        // Update best if this candidate improves the (tile_count, unused) tuple.
        let is_better = match &best {
            None    => true,
            Some(b) => (cand.non_empty_tiles, cand.unused_area_px2)
                     < (b.non_empty_tiles,    b.unused_area_px2),
        };
        if is_better {
            best = Some(cand.clone()); // keep a full clone so we can keep scanning for ties
        } // if is_better

        // Early-exit streak: if non-empty tile count fails to improve twice in a row,
        // wider candidates are very unlikely to help.  Stops wasteful extra packs.
        match last_count {
            Some(prev) if cand.non_empty_tiles >= prev => no_improve_streak += 1,
            _                                          => no_improve_streak = 0,
        } // match last_count
        last_count = Some(cand.non_empty_tiles);
        if no_improve_streak >= 2 { break; } // done scanning
    } // for cols

    // Prefer a real result; only surface an error if every candidate failed.
    best.ok_or_else(|| last_err.unwrap_or(packing::PackError::NoSpace { id: 0 }))
} // fn pick_best_tiled_candidate

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief measurement_to_px: bare number is treated as pixels.
    #[test]
    fn parse_bare_number_is_px() {
        let px = measurement_to_px("816");
        assert_eq!(px, 816, "bare number: {px}");
    } // parse_bare_number_is_px

    // @brief measurement_to_px: "px" suffix strips to bare value.
    #[test]
    fn parse_px_suffix() {
        let px = measurement_to_px("816px");
        assert_eq!(px, 816, "px suffix: {px}");
    } // parse_px_suffix

    // @brief measurement_to_px: "mm" suffix converts correctly.
    #[test]
    fn parse_mm_suffix() {
        // 210mm = 210/25.4*96 ≈ 793.70 px
        let px = measurement_to_px("210mm");
        assert!(((px as f64) - 793.70).abs() < 0.5, "mm suffix: {px}");
    } // parse_mm_suffix

    // @brief measurement_to_px: "in" suffix converts correctly.
    #[test]
    fn parse_in_suffix() {
        // 8.5in * 96 = 816 px
        let px: u32 = measurement_to_px("8.5in");
        assert!((px as f64 - 816.0).abs() < 0.01, "in suffix: {px}");
    } // parse_in_suffix

    // @brief tile_size_px: "None" returns None.
    #[test]
    fn tile_size_none_returns_none() {
        assert!(tile_size_px("None", "in").is_none());
    } // tile_size_none_returns_none

    // @brief tile_size_px: Letter in inches = 816×1056 px.
    #[test]
    fn tile_size_letter_in() {
        let (w, h) = tile_size_px("Letter", "in").expect("Letter ok");
        assert!((w - 816.0).abs() < 0.01, "Letter w: {w}");
        assert!((h - 1056.0).abs() < 0.01, "Letter h: {h}");
    } // tile_size_letter_in

    // @brief compute_tile_dims: "None" tile size returns Err.
    #[test]
    fn compute_none_tile_size_errors() {
        let mut settings = crate::layout_settings::LayoutSettings::default_for_test();
        settings.tile_size = "None".to_string();
        let result = compute_tile_dims(2000, 3000, &settings);
        assert!(result.is_err(), "expected Err for None tile size");
    } // compute_none_tile_size_errors

    // @brief compute_tile_dims: Letter tile, 0.25in margins, A4-sized input.
    #[test]
    fn compute_tile_dims_letter_a4_tiled_input() {
        // Input DOM: A4 = 210×297mm → ~793.7×1122.5 px
        let input_w_px = measurement_to_px("24.0in"); // 24in = 24*96 = 2304 px, larger than A4 to force tiling
        let input_h_px: u32 = measurement_to_px("36.0in");   // 36in = 36*96 = 3456 px, larger than A4 to force tiling
        // get default settings
        let mut settings = crate::layout_settings::LayoutSettings::default_for_test();
        settings.tile_size = "A4".to_string();
        settings.tile_orientation = "landscape".to_string();
        // get tile dims; default_for_test uses unit="in", margins=0.25in each
        let td = compute_tile_dims(input_w_px, input_h_px, &settings).expect("compute ok");
        // reverse letter_a4 dims for landscape tile arrangement, but margins stay the same
        // convert to pixels: 24in=2304px; 36in=3456px; 210mm=793.7px; 297mm=1122.5px
        // trimTileW = 1122.5 - 24 - 24 = 1074.5 -- lancscape A4 width
        // trimTileH =  793.7 - 24 - 24 = 745.7  -- landscape A4 height
        // expected tileCols = ceil( 2304 / 1074.5 ) = ceil(2.14) = 3
        // expected tileRows = ceil( 3456 / 745.7 )  = ceil(4.63) = 5
        // do tile_cols & tile_rows match expected values?
        assert_eq!(td.tile_cols, 3, "tile_cols");
        assert_eq!(td.tile_rows, 5, "tile_rows");
    } // compute_tile_dims_letter_a4_tiled_input

    #[test]
    fn compute_tile_dims_portrait_preserves_tile_dimensions() {
        let input_w_px = measurement_to_px("24.0in");
        let input_h_px: u32 = measurement_to_px("36.0in");
        let mut settings = crate::layout_settings::LayoutSettings::default_for_test();
        settings.tile_size = "A4".to_string();
        settings.tile_orientation = "portrait".to_string();

        let td = compute_tile_dims(input_w_px, input_h_px, &settings).expect("compute ok");

        // Portrait keeps A4 width/height ordering: 210mm x 297mm, minus 0.25in margins on each side.
        assert_eq!(td.trim_tile_w_px, 746, "portrait trim_tile_w_px");
        assert_eq!(td.trim_tile_h_px, 1074, "portrait trim_tile_h_px");
        assert_eq!(td.tile_cols, 4, "portrait tile_cols");
        assert_eq!(td.tile_rows, 4, "portrait tile_rows");
    } // compute_tile_dims_portrait_preserves_tile_dimensions

    // @brief create_initial_tiled_layout_dom: SVG root has correct width/height.
    #[test]
    fn tiled_svg_root_dimensions() {
        let td = TileDimensions {
            tile_cols: 2, tile_rows: 3,
            trim_tile_w_px: 768, trim_tile_h_px: 1008, // Letter size minus margins
            input_dom_w_px: 1000, input_dom_h_px: 1500, // input is just slightly larger than one tile to force tiling
            layout_w_px: 1584, layout_h_px: 3072, // expected layout is multiple of letter width and letter height, plus margins
            margin_left_px: 24, margin_right_px: 24,
            margin_top_px: 24,  margin_bottom_px: 24,
        };
        // create_initial_tiled_layout_dom() will recalculate layout_w_px & layout_h_px and update <svg> root
        let doc = create_initial_tiled_layout_dom(&td);
        let w = doc.root.attributes.get("width").map(String::as_str).unwrap_or("");
        let h = doc.root.attributes.get("height").map(String::as_str).unwrap_or("");
        // does <svg> width & height match the expected layout dimensions?
        assert!(w.starts_with("1584"), "root width: {w}");
        assert!(h.starts_with("3072"), "root height: {h}");
    } // tiled_svg_root_dimensions

    // @brief create_initial_tiled_layout_dom: one <path> per row (not <use> per tile).
    // @returns
    #[test]
    fn tiled_svg_path_count() {
        let td = TileDimensions {
            tile_cols: 2, tile_rows: 3,
            trim_tile_w_px: 768, trim_tile_h_px: 1008,
            input_dom_w_px: 1000, input_dom_h_px: 1500,
            layout_w_px: 1584, layout_h_px: 3072,
            margin_left_px: 24, margin_right_px: 24,
            margin_top_px: 24,  margin_bottom_px: 24,
        };
        let doc = create_initial_tiled_layout_dom(&td);
        let svg_str = doc.to_string();
        // 3 rows → 3 <path> elements; no <use> elements
        let path_count = svg_str.matches("<path").count();
        let use_count  = svg_str.matches("<use").count();
        assert_eq!(path_count, 3, "path element count: {path_count}");
        assert_eq!(use_count,  0, "unexpected use elements: {use_count}");
    } // tiled_svg_path_count

    // @brief create_initial_tiled_layout_dom: row path IDs are 1-based ("row_1", "row_2", ...).
    #[test]
    fn tiled_svg_row_ids() {
        let td = TileDimensions {
            tile_cols: 2, tile_rows: 2,
            trim_tile_w_px: 768, trim_tile_h_px: 1008,
            input_dom_w_px: 1000, input_dom_h_px: 1500,
            layout_w_px: 1584, layout_h_px: 2064,
            margin_left_px: 24, margin_right_px: 24,
            margin_top_px: 24,  margin_bottom_px: 24,
        };
        let doc = create_initial_tiled_layout_dom(&td);
        let svg_str = doc.to_string();
        assert!(svg_str.contains("row_1"), "missing row_1");
        assert!(svg_str.contains("row_2"), "missing row_2");
        assert!(!svg_str.contains("row_3"), "unexpected row_3");
    } // tiled_svg_row_ids

    // @brief widest_piece_tile_cols: empty pieces → 1 (avoids zero-size bin).
    #[test]
    fn widest_piece_tile_cols_empty() {
        let cols = widest_piece_tile_cols(&[], 768, 5);
        assert_eq!(cols, 1, "empty pieces floor");
    } // widest_piece_tile_cols_empty

    // @brief widest_piece_tile_cols: widest 500 px, tile 768 px, gap 5 → 1 col.
    #[test]
    fn widest_piece_tile_cols_single_tile() {
        let pieces = [
            packing::Rect::new(500, 400),
            packing::Rect::new(300, 300),
        ];
        let cols = widest_piece_tile_cols(&pieces, 768, 5);
        assert_eq!(cols, 1, "single-tile floor");
    } // widest_piece_tile_cols_single_tile

    // @brief widest_piece_tile_cols: widest 800 px, tile 768 px → 2 cols needed.
    #[test]
    fn widest_piece_tile_cols_two_tiles() {
        let pieces = [packing::Rect::new(800, 400)];
        let cols = widest_piece_tile_cols(&pieces, 768, 5);
        assert_eq!(cols, 2, "two-tile floor");
    } // widest_piece_tile_cols_two_tiles

    // @brief count_non_empty_tiles: piece fully inside one tile → 1.
    #[test]
    fn count_non_empty_tiles_single() {
        let placements = [packing::Placed { id: 0, x: 10, y: 10, w: 100, h: 100, rotation_deg: 0 }];
        let n = count_non_empty_tiles(&placements, 2, 2, 768, 1008);
        assert_eq!(n, 1, "single-tile piece");
    } // count_non_empty_tiles_single

    // @brief count_non_empty_tiles: piece straddles a column boundary → 2.
    #[test]
    fn count_non_empty_tiles_straddles_boundary() {
        // tile_w=100, piece spans x=80..180 which crosses x=100 boundary.
        let placements = [packing::Placed { id: 0, x: 80, y: 0, w: 100, h: 50, rotation_deg: 0 }];
        let n = count_non_empty_tiles(&placements, 3, 1, 100, 100);
        assert_eq!(n, 2, "straddle counts both tiles");
    } // count_non_empty_tiles_straddles_boundary

    // @brief pick_best_tiled_candidate: three pieces in a row pack into fewer tiles when stacked.
    #[test]
    fn pick_best_tiled_candidate_prefers_compact_layout() {
        // Three 300×300 pieces + gap 5.  Trim tile = 400×400.
        // At floor_cols=1, packer stacks into 3 rows (1 col × 3 rows = 3 tiles).
        // At ceil_cols=3, row-packed into 3 cols × 1 row = 3 tiles (tied).
        // Unused area ties broken by piece placement density.
        let pieces = [
            packing::Rect::new(300, 300),
            packing::Rect::new(300, 300),
            packing::Rect::new(300, 300),
        ];
        let best = pick_best_tiled_candidate(&pieces, 400, 400, 1, 3, 5, &[0])
            .expect("pick ok");
        // Either 1×3 or 3×1 is 3 tiles — both are valid minimums for three 300px pieces
        // in 400px tiles.  Assert we got a 3-tile answer.
        assert_eq!(best.non_empty_tiles, 3, "best tile count");
    } // pick_best_tiled_candidate_prefers_compact_layout

    // @brief pick_best_tiled_candidate: the horizontal-row pathological case.
    // Three pieces laid out horizontally in the input would naturally produce
    // three tiles across and one row, wasting the rest of the height.  Giving
    // the picker a wider ceiling than strictly needed should still pick a
    // compact 1-row layout.
    #[test]
    fn pick_best_tiled_candidate_horizontal_row_input() {
        // Three 300×300 pieces; trim tile 400×400.  Ceiling = 3 cols (what the
        // initial layout_dom would use).  Floor = 1.  Best is still 3 tiles.
        let pieces = [
            packing::Rect::new(300, 300),
            packing::Rect::new(300, 300),
            packing::Rect::new(300, 300),
        ];
        let best = pick_best_tiled_candidate(&pieces, 400, 400, 1, 3, 5, &[0])
            .expect("pick ok");
        assert!(best.non_empty_tiles <= 3, "should not exceed input tile count");
        assert!(best.tile_cols >= 1 && best.tile_cols <= 3, "cols in range");
    } // pick_best_tiled_candidate_horizontal_row_input

    // @brief pick_best_tiled_candidate: floor honored — widest piece fits at the floor.
    #[test]
    fn pick_best_tiled_candidate_respects_floor() {
        // Widest piece is 600×400; trim tile 400.  Floor should be 2 cols.
        let pieces = [
            packing::Rect::new(600, 400),
            packing::Rect::new(300, 300),
        ];
        let floor = widest_piece_tile_cols(&pieces, 400, 5);
        assert_eq!(floor, 2, "computed floor");
        let best = pick_best_tiled_candidate(&pieces, 400, 400, floor, 3, 5, &[0])
            .expect("pick ok");
        assert!(best.tile_cols >= 2, "respect floor");
    } // pick_best_tiled_candidate_respects_floor

    // @brief create_initial_tiled_layout_dom: row 2 path d starts at minX (colNum reset test).
    // Row 2's path d attribute must start with "M {minX}," not "M {minX+tileW},".
    #[test]
    fn tiled_svg_col_resets_per_row() {
        let td = TileDimensions {
            tile_cols: 2, tile_rows: 2,
            trim_tile_w_px: 500, trim_tile_h_px: 500,
            input_dom_w_px: 1100, input_dom_h_px: 1100,
            layout_w_px: 1048, layout_h_px: 1048,
            margin_left_px: 24, margin_right_px: 24,
            margin_top_px: 24,  margin_bottom_px: 24,
        };
        let doc = create_initial_tiled_layout_dom(&td);
        // row_2 d attribute must start with "M 24.0000," (minX=24), not "M 524.0000,".
        let d_val = doc.get_attr_by_id("row_2", "d")
            .expect("row_2 path not found in DOM");
        assert!(
            d_val.starts_with("M 24.0000,"),
            "row_2 d should start at minX=24, got: {d_val}"
        );
    } // tiled_svg_col_resets_per_row

} // mod tests
