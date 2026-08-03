// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file layout_settings.rs
// @brief Deserializes SettingsModel JSON into a Rust struct and computes
//        the effective bin dimensions for layout_engine::pack_shelves.
//
// SettingsModel::save() writes camelCase JSON; `#[serde(rename_all = "camelCase")]`
// handles the key mapping automatically.
//
// All dimension fields are stored in the active unit system (see `unit`).
// `effective_bin_px()` converts everything to pixels before returning.

use serde::Deserialize;

// Pixels per inch — the canvas resolution for all layout and geometry calculations.
// 96 px/in is the standard CSS/SVG baseline; used to convert physical measurements to pixels.
pub const LAYOUT_PPI: f64 = 96.0;

// Full roll length sentinel in inches: 500 in = one standard fabric bolt.
// Used for any roll-form media (media_type == "roll", or paper_type == "roll")
// because roll length is unbounded at layout time.  Trimmed after packing.
const ROLL_DEFAULT_LENGTH_IN: f64 = 500.0;

// Minimum bin dimension in pixels; prevents zero-size bins on bad settings.
const MIN_BIN_PX: u32 = 1;

// @brief Layout settings deserialized from the JSON emitted by `SettingsModel::save()`.
//
// All dimension fields are in the active unit system stored in `unit`.
// Fields marked "deferred" are parsed but not yet used by the layout engine.
#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LayoutSettings {

    // Unit system for all dimension fields: "in" | "mm" | "cm".
    #[serde(default = "default_unit")]
    pub unit: String,

    // Media type: "paper" | "roll".
    #[serde(default = "default_media_type")]
    pub media_type: String,

    // Paper sub-type: "sheet" | "tiled".
    // "tiled" is deferred — not yet supported by the layout engine.
    #[serde(default = "default_paper_type")]
    pub paper_type: String,

    // Sheet (paper) width in active units; used when media_type == "paper".
    #[serde(default = "default_page_width")]
    pub page_width: f64,

    // Sheet (paper) height in active units; used when media_type == "paper".
    #[serde(default = "default_page_height")]
    pub page_height: f64,

    // Roll width in active units; used when media_type == "roll" or paper_type == "roll".
    // Roll height sentinel is ROLL_DEFAULT_LENGTH_IN (500 in); trimmed after packing.
    #[serde(default = "default_roll_width")]
    pub roll_width: f64,

    // Margins in active units; subtracted from the base bin dimensions.
    #[serde(default = "default_margin")]
    pub margin_top: f64,
    #[serde(default = "default_margin")]
    pub margin_bottom: f64,
    #[serde(default = "default_margin")]
    pub margin_left: f64,
    #[serde(default = "default_margin")]
    pub margin_right: f64,

    // Selvedge width in active units.  Selvedge is a fabric concept (the
    // woven edge of the cloth) and does not apply to paper/roll media.
    // For media_type == "fabric" the C++ side already folds it into the four
    // margins (`SettingsModel::syncFabricMarginsFromSelvedge()`), so this
    // field is informational here — `effective_bin_px()` must NOT deduct it
    // again or the selvedge would be subtracted twice.
    #[serde(default)]
    pub selvedge_width: f64,

    // Minimum clearance between adjacent placed pieces, in active units.
    // Total gap (not per-side); the polygon packer halves it internally before
    // applying as an outward polygon offset, the rect packer applies it
    // unchanged between AABBs.  Default ≈ 5 px @ 96 dpi to preserve the
    // historic `GAP_PX` const's behavior.
    #[serde(default = "default_piece_gap")]
    pub piece_gap: f64,

    // True when fabric is folded lengthwise (selvedge-to-selvedge).
    // Halves the effective bin width: pieces are placed on the folded half.
    // The fold edge has no selvedge; the open edge has one selvedge.
    // Deferred: grain-direction logic is not yet in the layout engine.
    #[serde(default)]
    pub fabric_folded: bool,

    // Fabric width override in active units (0 = use page/roll width).
    #[serde(default)]
    pub fabric_width: f64,

    // Fabric height override in active units (0 = use page/roll height).
    #[serde(default)]
    pub fabric_height: f64,

    // Piece-arrangement mode: "alongGrainline" | "withNap".
    //   alongGrainline → grain-up baseline, trial set {0°, 180°}
    //   withNap        → grain-up baseline, trial set {0°} only
    // Legacy/unknown values are coerced to alongGrainline behavior in
    // `rotation_trial_set_deg`.
    #[serde(default = "default_layout_mode")]
    pub layout_mode: String,

    // Rotation step in degrees. Consulted by withNap as a fixed direction:
    //   0.0   = pieces point up
    //   180.0 = pieces point down
    #[serde(default = "default_rotation_step")]
    pub rotation_step: f64,

    #[serde(default)]
    pub sheet_name: String,
    #[serde(default)]
    pub roll_size: String,
    #[serde(default)]
    pub tile_size: String,

    #[serde(default = "default_tile_orientation")]
    pub tile_orientation: String,

    // export format: "svg" | "pdf" | "pdf-tiled" | "png" | "dxf-astm" | "ASCII DXF" | "DXF R12" " "GCode" | "HPGL" | "Gerber" |
    #[serde(default)]
    pub output_format: String,

} // struct LayoutSettings

// ---------------------------------------------------------------------------
// serde defaults — match SettingsModel C++ defaults exactly
// ---------------------------------------------------------------------------

fn default_unit()          -> String { "in".to_string()         } // default unit
fn default_media_type()    -> String { "paper".to_string()      } // default media
fn default_paper_type()    -> String { "sheet".to_string()      } // default paper sub-type
fn default_page_width()    -> f64    { 36.0                     } // ARCH E width in inches
fn default_page_height()   -> f64    { 48.0                     } // ARCH E height in inches
fn default_roll_width()    -> f64    { 0.0                      } // 0" roll width
fn default_margin()        -> f64    { 0.25                     } // 0.25 in default margin
fn default_layout_mode()   -> String { "alongGrainline".to_string() } // default piece-arrangement mode
fn default_rotation_step() -> f64    { 0.0                      } // default withNap direction: head-up
fn default_tile_orientation() -> String { "landscape".to_string() } // default tiled-paper orientation
fn default_piece_gap()     -> f64    { 0.05                     } // 0.05 in ≈ 5 px @ 96 dpi (historic GAP_PX)

// ---------------------------------------------------------------------------
// Implementation
// ---------------------------------------------------------------------------

impl LayoutSettings {

    // @brief Deserialize from a JSON string emitted by `SettingsModel::save()`.
    //
    // Missing keys fall back to their `#[serde(default)]` values, which match
    // the C++ `SettingsModel` field defaults.
    //
    // @param json JSON string (UTF-8).
    // @return Parsed settings or a serde_json error.
    pub fn from_json(json: &str) -> Result<Self, serde_json::Error> {
        serde_json::from_str(json)
    } // fn from_json

    // @brief Convert a value from the active unit system to inches.
    //
    // @param value Value in the active unit system.
    // @param unit  Unit string: "in" | "mm" | "cm".
    // @return Equivalent value in inches.
    fn to_inches(value: f64, unit: &str) -> f64 {
        match unit {
            "mm" => value / 25.4,  // millimetres → inches
            "cm" => value / 2.54,  // centimetres → inches
            _    => value,         // "in" or unknown — treat as inches
        } // match unit
    } // fn to_inches

    // @brief Return all four margins converted to pixels at `LAYOUT_PPI`.
    //
    // Used by `create_initial_layout_dom` to position the content rectangle
    // and size the full canvas, and by `process_layout` for roll-trim math.
    //
    // @return `(margin_left_px, margin_right_px, margin_top_px, margin_bottom_px)`
    pub fn margin_px(&self) -> (u32, u32, u32, u32) {
        let ml = (Self::to_inches(self.margin_left,   &self.unit) * LAYOUT_PPI).round() as u32;
        let mr = (Self::to_inches(self.margin_right,  &self.unit) * LAYOUT_PPI).round() as u32;
        let mt = (Self::to_inches(self.margin_top,    &self.unit) * LAYOUT_PPI).round() as u32;
        let mb = (Self::to_inches(self.margin_bottom, &self.unit) * LAYOUT_PPI).round() as u32;
        (ml, mr, mt, mb)
    } // fn margin_px

    // @brief Piece-gap clearance in pixels at `LAYOUT_PPI`.
    //
    // Mirrors `SettingsModel::pieceGapPx()` on the C++ side.  Used by the
    // bridge to feed `gap_px` into `packing::pack_pieces` /
    // `packing::pack_polygons` — replaces the historic `GAP_PX` const.
    pub fn piece_gap_px(&self) -> u32 {
        (Self::to_inches(self.piece_gap, &self.unit) * LAYOUT_PPI).round() as u32
    } // fn piece_gap_px

    // @brief Return the page/canvas width in pixels at `LAYOUT_PPI`.
    //
    // For roll media uses `roll_width`; for sheet/tiled uses `page_width`.
    pub fn page_w_px(&self) -> u32 {
        let is_roll = self.media_type == "roll"
            || (self.media_type == "paper" && self.paper_type == "roll");
        let w_in = if is_roll {
            Self::to_inches(self.roll_width, &self.unit)
        } else {
            Self::to_inches(self.page_width, &self.unit)
        };
        (w_in * LAYOUT_PPI).round() as u32
    } // fn page_w_px

    // @brief Return the page/canvas height in pixels at `LAYOUT_PPI`.
    //
    // For roll media uses the `ROLL_DEFAULT_LENGTH_IN` sentinel; for sheet/tiled uses `page_height`.
    pub fn page_h_px(&self) -> u32 {
        let is_roll = self.media_type == "roll"
            || (self.media_type == "paper" && self.paper_type == "roll");
        let h_in = if is_roll {
            ROLL_DEFAULT_LENGTH_IN
        } else {
            Self::to_inches(self.page_height, &self.unit)
        };
        (h_in * LAYOUT_PPI).round() as u32
    } // fn page_h_px

    // @brief Return the bottom margin converted to pixels at `LAYOUT_PPI`.
    //
    // Used by `process_layout` to compute the trimmed roll height after packing:
    //   `trimmed_h = max_piece_bottom_y + margin_bottom_px()`
    pub fn margin_bottom_px(&self) -> u32 {
        let (_, _, _, mb) = self.margin_px();
        mb
    } // fn margin_bottom_px

    // @brief Compute effective bin dimensions in pixels for `pack_maxrects`.
    //
    // Applies the following transformations in order:
    //   1. Select base dimensions:
    //      - media_type=="roll" OR paper_type=="roll" → rollW × ROLL_DEFAULT_LENGTH_IN (500 in sentinel)
    //      - media_type=="paper" (sheet or tiled)     → pageW × pageH
    //   2. Override base with `fabric_width` / `fabric_height` when > 0.
    //   3. Halve width if `fabric_folded` (pieces are placed on the folded half).
    //   4. Subtract all four margins.  For media_type == "fabric" the margins
    //      already carry the selvedge (SettingsModel::syncFabricMarginsFromSelvedge()
    //      sets each margin to selvedgeWidth), so no separate selvedge step
    //      exists here — see the `selvedge_width` field comment.
    //   5. Clamp each dimension to `MIN_BIN_PX` to prevent zero-size bins.
    //   6. Convert from inches to pixels at `LAYOUT_PPI` (96 px/in).
    //
    // NOTE: `layout_mode` and `rotation_step` are NOT applied here — piece
    // arrangement (alongGrainline / withNap) and rotation step are
    // deferred pending layout_engine support.
    // NOTE: `paper_type == "tiled"` is NOT handled — tiling is deferred.
    //
    // @return `(bin_w_px, bin_h_px)` as `u32` values ready for `pack_maxrects`.
    pub fn effective_bin_px(&self) -> (u32, u32) {
        // TODO: convert to user selected units whether this is inches, cm, or mm — currently hardcoded to inches.

        let u = self.unit.as_str();

        // --- step 1: base dimensions from media_type / paper_type ---
        let is_roll = self.media_type == "roll"
            || (self.media_type == "paper" && self.paper_type == "roll");
        let (base_w_in, base_h_in) = if is_roll {
            // Roll form: width is finite; height sentinel = full bolt (500 in); trimmed after packing.
            let rw = Self::to_inches(self.roll_width, u);
            (rw, ROLL_DEFAULT_LENGTH_IN)
        } else {
            // "paper" (sheet or tiled) — use physical sheet dimensions
            (
                Self::to_inches(self.page_width,  u),
                Self::to_inches(self.page_height, u),
            )
        }; // if is_roll

        // --- step 2: fabric_width / fabric_height overrides ---
        let base_w_in = if self.fabric_width > 0.0 {
            Self::to_inches(self.fabric_width, u)
        } else {
            base_w_in
        }; // if fabric_width override
        let base_h_in = if self.fabric_height > 0.0 {
            Self::to_inches(self.fabric_height, u)
        } else {
            base_h_in
        }; // if fabric_height override

        // --- step 3: fabric_folded halves the usable width ---
        //
        // When folded, pieces are cut on the fold: the cutting area is the
        // half-width from the fold line to the open selvedge edge.
        // layout_mode (piece arrangement) is deferred — see NOTE above.
        let base_w_in = if self.fabric_folded {
            base_w_in / 2.0
        } else {
            base_w_in
        }; // if fabric_folded

        // --- step 4: subtract margins ---
        let ml = Self::to_inches(self.margin_left,   u);
        let mr = Self::to_inches(self.margin_right,  u);
        let mt = Self::to_inches(self.margin_top,    u);
        let mb = Self::to_inches(self.margin_bottom, u);

        let w_in = base_w_in - (ml + mr);
        let h_in = base_h_in - (mt + mb);

        // NOTE: no separate selvedge step.  Selvedge is a fabric-only concept;
        // for media_type == "fabric" the C++ SettingsModel maps selvedgeWidth
        // into all four margins before the JSON reaches this crate
        // (syncFabricMarginsFromSelvedge()), so step 4 above already accounts
        // for it.  Deducting `selvedge_width` here as well would subtract the
        // selvedge twice; for paper/roll media it never applies at all.

        // --- step 5: clamp to minimum ---
        // Prevent zero-size bins on bad settings (e.g., margins larger than page).
        let w_in = w_in.max(MIN_BIN_PX as f64 / LAYOUT_PPI);
        let h_in = h_in.max(MIN_BIN_PX as f64 / LAYOUT_PPI);

        // --- step 6: convert to pixels at 96 px/in (u32, no decimals for pixels) ---
        let w_px = (w_in * LAYOUT_PPI).round() as u32;
        let h_px = (h_in * LAYOUT_PPI).round() as u32;

        // --- final: return effective bin dimensions in pixels, clamped to minimum ---
        (w_px.max(MIN_BIN_PX), h_px.max(MIN_BIN_PX))
    } // fn effective_bin_px

    // @brief Build the per-piece rotation trial set (degrees) for the packer.
    //
    // Maps the user's `layout_mode` + `rotation_step` choice to the list of
    // angles the packer should consider for each placement.  Returned values
    // are `u16` degrees in the range `[0, 360)` so `packing::pack_pieces`
    // can route based on whether they are all in `{0, 180}`.
    //
    // | layout_mode      | rotation_step       | trial set                |
    // | ---------------- | ------------------- | ------------------------ |
    // | "alongGrainline" | unused              | [0, 180]                 |
    // | "withNap"        | 0                   | [0]                      |
    // | "withNap"        | 180                 | [180]                    |
    // | unknown/legacy    | —                   | [0, 180]                 |
    //
    // Note: this returns only orthogonal trial sets ({0}, {180}, {0,180}).
    pub fn rotation_trial_set_deg(&self) -> Vec<u16> {
        match self.layout_mode.as_str() {
            "alongGrainline" => vec![0, 180],
            "withNap" => {
                // rotation_step is the fixed offset (head-up vs head-down).
                // Snap to {0, 180}; any other value defaults to 0 (head-up).
                if (self.rotation_step - 180.0).abs() < 0.5 {
                    vec![180]
                } else {
                    vec![0]
                } // if 180
            } // "withNap"
            _ => vec![0, 180], // unknown/legacy (including "rotate") → alongGrainline behavior
        } // match layout_mode
    } // fn rotation_trial_set_deg

} // impl LayoutSettings

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

impl LayoutSettings {
    // @brief Construct a LayoutSettings with sensible defaults for unit tests.

    #[cfg(test)]
    pub fn default_for_test() -> Self {
        // defaults for tiled PDF tests — not all fields are relevant to every test, but this is a convenient starting point
        Self {
            unit:           "in".to_string(),
            media_type:     "paper".to_string(),
            paper_type:     "tiled".to_string(),
            page_width:     8.5, // tile paper width in inches (Letter)
            page_height:    11.0, // tile paper height in inches (Letter)
            roll_width:     0.0,
            margin_top:     0.25,
            margin_bottom:  0.25,
            margin_left:    0.25,
            margin_right:   0.25,
            selvedge_width: 0.0,
            piece_gap:      0.05,
            fabric_folded:  false,
            fabric_width:   0.0,
            fabric_height:  0.0,
            layout_mode:    "alongGrainline".to_string(),
            rotation_step:  0.0,
            sheet_name:     "".to_string(),
            roll_size:      "".to_string(),
            tile_size:      "Letter".to_string(),
            tile_orientation: "landscape".to_string(),
            output_format:  "pdf".to_string(),
        } // Self
    } // fn default_for_test
} // impl LayoutSettings (test helpers)

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    // @brief ARCH E sheet at 96 DPI with 0.25 in margins produces correct bin.
    #[test]
    fn arch_e_paper_bin() {
        let s = LayoutSettings {
            unit:         "in".to_string(),
            media_type:   "paper".to_string(),
            paper_type:   "sheet".to_string(),
            page_width:   36.0,
            page_height:  48.0,
            roll_width:   36.0,
            margin_top:   0.25,
            margin_bottom:0.25,
            margin_left:  0.25,
            margin_right: 0.25,
            selvedge_width: 0.0,
            piece_gap:      0.05,
            fabric_folded:  false,
            fabric_width:   0.0,
            fabric_height:  0.0,
            layout_mode:    "alongGrainline".to_string(),
            rotation_step:  0.0,
            sheet_name:     "ARCH E".to_string(),
            roll_size:      "36 in".to_string(),
            tile_size:      "Letter".to_string(),
            tile_orientation: "landscape".to_string(),
            output_format:  "svg".to_string(),
        };
        let (w, h) = s.effective_bin_px();
        // (36 - 0.5) * 96 = 3408, (48 - 0.5) * 96 = 4560
        assert_eq!(w, 3408);
        assert_eq!(h, 4560);
    } // arch_e_paper_bin

    // @brief Roll media uses 500-inch length sentinel regardless of roll width.
    #[test]
    fn roll_media_bin() {
        // default paper roll settings
        let s = LayoutSettings {
            unit:         "in".to_string(),
            media_type:   "paper".to_string(),
            paper_type:   "roll".to_string(),
            page_width:   0.0,
            page_height:  0.0,
            roll_width:   36.0,
            margin_top:   0.0,
            margin_bottom:0.0,
            margin_left:  0.0,
            margin_right: 0.0,
            selvedge_width: 0.0,
            piece_gap:      0.05,
            fabric_folded:  false,
            fabric_width:   0.0,
            fabric_height:  0.0,
            layout_mode:    "alongGrainline".to_string(),
            rotation_step:  0.0,
            sheet_name:     "none".to_string(),
            roll_size:      "36 in".to_string(),
            tile_size:      "none".to_string(),
            tile_orientation: "landscape".to_string(),
            output_format:  "svg".to_string(),
        };
        let (w, h) = s.effective_bin_px();
        // 36 * 96 = 3456; 500 * 96 = 48000
        assert_eq!(w, 3456);
        assert_eq!(h, 48000);
    } // roll_media_bin

    // @brief Fabric selvedge arrives baked into the margins (SettingsModel::
    // syncFabricMarginsFromSelvedge() sets every margin to selvedgeWidth), so
    // effective_bin_px() must apply the margins once and NOT deduct
    // selvedge_width a second time.
    #[test]
    fn selvedge_baked_into_margins_for_fabric() {
        let s = LayoutSettings {
            unit:          "in".to_string(),
            media_type:    "fabric".to_string(),
            paper_type:    "sheet".to_string(),
            page_width:    36.0,
            page_height:   48.0,
            roll_width:    36.0,
            // Margins mirror what syncFabricMarginsFromSelvedge() produces
            // for selvedgeWidth = 0.5: every margin equals the selvedge.
            margin_top:    0.5,
            margin_bottom: 0.5,
            margin_left:   0.5,
            margin_right:  0.5,
            selvedge_width: 0.5,
            piece_gap:      0.05,
            fabric_folded:  false,
            fabric_width:   0.0,
            fabric_height:  0.0,
            layout_mode:    "alongGrainline".to_string(),
            rotation_step:  0.0,
            sheet_name:     "ARCH E".to_string(),
            roll_size:      "36 in".to_string(),
            tile_size:      "Letter".to_string(),
            tile_orientation: "landscape".to_string(),
            output_format:  "svg".to_string(),
        };
        let (w, h) = s.effective_bin_px();
        // Width: (36 - 0.5 - 0.5) * 96 = 3360 — the selvedge is deducted once,
        // via the margins.  A double deduction would give (36 - 2.0) * 96 = 3264.
        assert_eq!(w, 3360);
        // Height: (48 - 0.5 - 0.5) * 96 = 4512.
        assert_eq!(h, 4512);
    } // selvedge_baked_into_margins_for_fabric

    // @brief Selvedge is a fabric concept and never applies to paper media:
    // a stray selvedge_width value must not shrink a paper bin.
    #[test]
    fn selvedge_ignored_for_paper() {
        let s = LayoutSettings {
            unit:          "in".to_string(),
            media_type:    "paper".to_string(),
            paper_type:    "sheet".to_string(),
            page_width:    36.0,
            page_height:   48.0,
            roll_width:    36.0,
            margin_top:    0.0,
            margin_bottom: 0.0,
            margin_left:   0.0,
            margin_right:  0.0,
            selvedge_width: 0.5,
            piece_gap:      0.05,
            fabric_folded:  false,
            fabric_width:   0.0,
            fabric_height:  0.0,
            layout_mode:    "alongGrainline".to_string(),
            rotation_step:  0.0,
            sheet_name:     "ARCH E".to_string(),
            roll_size:      "36 in".to_string(),
            tile_size:      "Letter".to_string(),
            tile_orientation: "landscape".to_string(),
            output_format:  "svg".to_string(),
        };
        let (w, _) = s.effective_bin_px();
        // Full 36 in width: 36 * 96 = 3456 — selvedge_width is ignored for paper.
        assert_eq!(w, 3456);
    } // selvedge_ignored_for_paper

    // @brief Fabric folded halves the effective width.
    #[test]
    fn fabric_folded_halves_width() {
        let s = LayoutSettings {
            unit:          "in".to_string(),
            media_type:    "paper".to_string(),
            paper_type:    "sheet".to_string(),
            page_width:    36.0,
            page_height:   48.0,
            roll_width:    36.0,
            margin_top:    0.0,
            margin_bottom: 0.0,
            margin_left:   0.0,
            margin_right:  0.0,
            selvedge_width: 0.0,
            piece_gap:      0.05,
            fabric_folded:  true,
            fabric_width:   0.0,
            fabric_height:  0.0,
            layout_mode:    "alongGrainline".to_string(),
            rotation_step:  0.0,
            sheet_name:     "ARCH E".to_string(),
            roll_size:      "36 in".to_string(),
            tile_size:      "Letter".to_string(),
            tile_orientation: "landscape".to_string(),
            output_format:  "svg".to_string(),
        };
        let (w, _) = s.effective_bin_px();
        // (36 / 2) * 96 = 1728
        assert_eq!(w, 1728);
    } // fabric_folded_halves_width

    // @brief Millimetre units are converted correctly.
    #[test]
    fn mm_unit_conversion() {
        let s = LayoutSettings {
            unit:          "mm".to_string(),
            media_type:    "paper".to_string(),
            paper_type:    "sheet".to_string(),
            page_width:    914.0,  // ARCH E in mm
            page_height:   1219.0, // ARCH E in mm
            roll_width:    914.0,
            margin_top:    0.0,
            margin_bottom: 0.0,
            margin_left:   0.0,
            margin_right:  0.0,
            selvedge_width: 0.0,
            piece_gap:      0.05,
            fabric_folded:  false,
            fabric_width:   0.0,
            fabric_height:  0.0,
            layout_mode:    "alongGrainline".to_string(),
            rotation_step:  0.0,
            sheet_name:     "ARCH E".to_string(),
            roll_size:      "36 in".to_string(),
            tile_size:      "Letter".to_string(),
            tile_orientation: "landscape".to_string(),
            output_format:  "svg".to_string(),
        };
        let (w, h) = s.effective_bin_px();
        // 914 / 25.4 * 96 ≈ 3455, 1219 / 25.4 * 96 ≈ 4608
        assert!((w as i64 - 3455).abs() <= 2, "w={w}");
        assert!((h as i64 - 4608).abs() <= 2, "h={h}");
    } // mm_unit_conversion

    // @brief from_json round-trips with camelCase keys.
    #[test]
    fn from_json_parses_camel_case() {
        let json = r#"{
            "unit": "in",
            "mediaType": "paper",
            "paperType": "sheet",
            "pageWidth": 36.0,
            "pageHeight": 48.0,
            "rollWidth": 36.0,
            "marginTop": 0.25,
            "marginBottom": 0.25,
            "marginLeft": 0.25,
            "marginRight": 0.25,
            "selvedgeWidth": 0.0,
            "fabricFolded": false,
            "fabricWidth": 0.0,
            "fabricHeight": 0.0,
            "layoutMode": "alongGrainline",
            "rotationStep": 0.0,
            "sheetName": "ARCH E",
            "rollSize": "36 in",
            "tileSize": "Letter",
            "tileOrientation": "landscape",
            "outputFormat": "svg"
        }"#;
        let s = LayoutSettings::from_json(json).expect("parse ok");
        assert_eq!(s.unit, "in");
        assert_eq!(s.media_type, "paper");
        assert!((s.page_width - 36.0).abs() < 1e-9);
        assert!((s.margin_top - 0.25).abs() < 1e-9);
        assert_eq!(s.tile_orientation, "landscape");
        let (w, h) = s.effective_bin_px();
        assert_eq!(w, 3408);
        assert_eq!(h, 4560);
    } // from_json_parses_camel_case

    // @brief alongGrainline → [0, 180] regardless of rotation_step.
    #[test]
    fn trial_set_along_grainline() {
        let mut s = LayoutSettings::default_for_test();
        s.layout_mode = "alongGrainline".to_string();
        s.rotation_step = 90.0;
        assert_eq!(s.rotation_trial_set_deg(), vec![0, 180]);
    } // trial_set_along_grainline

    // @brief withNap → singleton {0} for head-up.
    #[test]
    fn trial_set_with_nap_up() {
        let mut s = LayoutSettings::default_for_test();
        s.layout_mode = "withNap".to_string();
        s.rotation_step = 0.0;
        assert_eq!(s.rotation_trial_set_deg(), vec![0]);
    } // trial_set_with_nap_up

    // @brief withNap → singleton {180} for head-down.
    #[test]
    fn trial_set_with_nap_down() {
        let mut s = LayoutSettings::default_for_test();
        s.layout_mode = "withNap".to_string();
        s.rotation_step = 180.0;
        assert_eq!(s.rotation_trial_set_deg(), vec![180]);
    } // trial_set_with_nap_down

    // @brief Unknown layout_mode falls back to alongGrainline trial set.
    #[test]
    fn trial_set_unknown_mode() {
        let mut s = LayoutSettings::default_for_test();
        s.layout_mode = "nonsense".to_string();
        assert_eq!(s.rotation_trial_set_deg(), vec![0, 180]);
    } // trial_set_unknown_mode

    // @brief Legacy rotate mode is coerced to alongGrainline trial set.
    #[test]
    fn trial_set_legacy_rotate_mode() {
        let mut s = LayoutSettings::default_for_test();
        s.layout_mode = "rotate".to_string();
        s.rotation_step = 45.0;
        assert_eq!(s.rotation_trial_set_deg(), vec![0, 180]);
    } // trial_set_legacy_rotate_mode

} // mod tests
