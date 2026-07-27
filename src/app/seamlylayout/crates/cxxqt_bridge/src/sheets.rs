// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file sheets.rs
// @brief Orchestrator for sheet-mode multi-page PDF export — Tasks L.2.1 + L.2.2.
//
// Combines Phase A (oversized pieces → tiled multi-page PDF, L.2.1) and
// Phase B (remaining pieces → one PDF page per sheet, L.2.2) into a single
// merged PDF written to disk.
//
// Call flow:
//   1. Compute content rect from settings (effective_bin_px + margin_px).
//   2. partition_oversized_pieces → (oversized_indices, remaining_indices).
//   3. Phase A: if oversized non-empty:
//        build_oversized_svg() → collect_tiled_pdf_page_bytes() → page bytes
//   4. Phase B: if remaining non-empty:
//        build_remaining_svgs() → render each to a single-page PDF byte buffer
//   5. Merge all page byte buffers into one PDF and write to `path`.
//
// Public API (crate-internal):
//   build_sheet_export_inputs(input_dom) -> Result<(flat_dom, pieces), String>
//   do_export_sheets_pdf(flat_dom, all_pieces, path, settings) -> Result<(), String>

use layout_tiling::LayoutSettings;
use svg_dom::Document;
use crate::piece_extractor::PieceRect;
use crate::oversized::{partition_oversized_pieces, build_oversized_svg};
use crate::remaining::build_remaining_svgs;
use crate::exports::{
    collect_tiled_pdf_page_bytes,
    render_svg_doc_to_pdf_bytes,
    merge_single_page_pdfs,
};

// @brief Rebuild the translated/flattened source DOM and extract packable pieces.
//
// Sheet-mode PDF export needs the pre-layout piece geometry, not the assembled
// `layout_dom`, because the export pipeline may redistribute the same source
// pieces across multiple physical pages. This helper mirrors the preprocessing
// sequence used by `do_process_layout`: flatten → verticalize → flatten →
// translate → flatten.
//
// @param input_dom Raw imported SVG DOM.
// @return Tuple of `(flat_dom, pieces)` ready for L.2.1/L.2.2 export logic.
pub(crate) fn build_sheet_export_inputs(
    input_dom: &Document,
) -> Result<(Document, Vec<PieceRect>), String> {
    let mut flat_dom = input_dom.clone();

    // Match the layout pipeline so export sees the same normalized piece space.
    // The hoist must come first and must not be skipped: without it a Seamly2D
    // handoff exports as one sheet-sized "piece" (Task 59).
    crate::piece_extractor::hoist_tagged_pieces(&mut flat_dom);
    svg_dom::flatten_dom(&mut flat_dom);
    svg_dom::verticalize_dom(&mut flat_dom);
    svg_dom::flatten_dom(&mut flat_dom);
    svg_dom::translate_dom(&mut flat_dom);
    svg_dom::flatten_dom(&mut flat_dom);

    let pieces = crate::extract_piece_rects(&flat_dom);
    if pieces.is_empty() {
        return Err(
            "Sheets PDF: no pattern pieces found in the imported SVG after preprocessing."
                .to_string(),
        );
    }

    Ok((flat_dom, pieces))
} // fn build_sheet_export_inputs

// @brief Export all pieces as a sheet-mode PDF: Phase A (oversized/tiled) + Phase B (remaining/single-page).
//
// When there are no oversized pieces the output is pure Phase B (one page per sheet).
// When there are no remaining pieces the output is pure Phase A (tiled pages only).
// When both groups are empty, an error is returned.
//
// The tile_size in settings must be a valid named size (e.g., "Letter", "A4") because
// the Phase A tiled pipeline calls `compute_tile_dims` which looks up the tile page size.
// Phase B does NOT use `tile_size`; it renders each remaining-sheet SVG at 1:1.
//
// @param flat_dom    Pre-processed SVG DOM (flatten→verticalize→translate→flatten).
// @param all_pieces  All extracted PieceRect entries.
// @param path        Destination PDF file path.
// @param settings    Parsed LayoutSettings; paper_type must be "sheet".
// @return Ok(()) on success; Err(message) on any failure.
pub(crate) fn do_export_sheets_pdf(
    flat_dom: &Document,
    all_pieces: &[PieceRect],
    path: &str,
    settings: &LayoutSettings,
) -> Result<(), String> {

    use std::fs;

    // Derive content-rect dimensions from settings.
    // effective_bin_px() = (page_w - margins - selvedge) × (page_h - margins).
    let (content_w_px, content_h_px) = settings.effective_bin_px();
    let (ml_px, mr_px, mt_px, mb_px) = settings.margin_px();
    let gap_px = settings.piece_gap_px();
    let trial_angles_deg = settings.rotation_trial_set_deg();

    log::debug!(
        "do_export_sheets_pdf: content={}×{}px; margins=({},{},{},{}); gap={}; angles={:?}; pieces={}",
        content_w_px, content_h_px, ml_px, mt_px, mr_px, mb_px,
        gap_px, trial_angles_deg, all_pieces.len()
    );

    // Partition pieces into oversized (→ Phase A) and remaining (→ Phase B).
    let (oversized_indices, remaining_indices) =
        partition_oversized_pieces(all_pieces, content_w_px, content_h_px);

    log::debug!(
        "do_export_sheets_pdf: {} oversized pieces (Phase A), {} remaining (Phase B).",
        oversized_indices.len(), remaining_indices.len()
    );

    if oversized_indices.is_empty() && remaining_indices.is_empty() {
        return Err("Sheets PDF: no pieces to export.".to_string());
    } // if nothing to export

    let all_page_bytes = collect_sheet_pdf_page_bytes(flat_dom, all_pieces, settings)?;

    if all_page_bytes.is_empty() {
        return Err("Sheets PDF: no pages produced.".to_string());
    } // if empty

    // Merge all pages into a single multi-page PDF and write to disk.
    let merged = merge_single_page_pdfs(all_page_bytes)
        .map_err(|e| format!("Sheets PDF merge: {e}"))?;

    fs::write(path, &merged)
        .map_err(|e| format!("Sheets PDF write failed: {e}")) // if write failed

} // fn do_export_sheets_pdf

// @brief Collect all sheet-export pages as single-page PDF byte buffers.
//
// Ordering is stable:
//   1. Phase A tile pages (thumbnail + tiled pages for oversized pieces)
//   2. Phase B one-page-per-sheet PDFs for remaining pieces
//
// @param flat_dom    Pre-processed SVG DOM (flatten→verticalize→translate→flatten).
// @param all_pieces  All extracted PieceRect entries.
// @param settings    Parsed LayoutSettings for sheet export.
// @return Ordered vector of single-page PDF byte buffers.
pub(crate) fn collect_sheet_pdf_page_bytes(
    flat_dom: &Document,
    all_pieces: &[PieceRect],
    settings: &LayoutSettings,
) -> Result<Vec<Vec<u8>>, String> {
    // Accumulate all per-page PDF byte buffers in order:
    //   [Phase A tile pages…, Phase B sheet pages…]
    let mut all_page_bytes: Vec<Vec<u8>> = Vec::new();

    // ---- Phase A: oversized pieces → tiled multipage PDF pages ----
    let (content_w_px, content_h_px) = settings.effective_bin_px();
    let (ml_px, mr_px, mt_px, mb_px) = settings.margin_px();
    let gap_px = settings.piece_gap_px();
    let trial_angles_deg = settings.rotation_trial_set_deg();
    let (oversized_indices, remaining_indices) =
        partition_oversized_pieces(all_pieces, content_w_px, content_h_px);

    if !oversized_indices.is_empty() {
        log::debug!("do_export_sheets_pdf: Phase A — building oversized SVG.");

        let oversized_doc: Document = build_oversized_svg(
            flat_dom,
            all_pieces,
            &oversized_indices,
            gap_px,
            &trial_angles_deg,
            ml_px, mt_px, mr_px, mb_px,
        ).map_err(|e| format!("Sheets PDF Phase A: {e}"))?;

        // Feed the oversized SVG through the tiled-PDF pipeline to get per-tile pages.
        let phase_a_pages = collect_tiled_pdf_page_bytes(&oversized_doc, settings)
            .map_err(|e| format!("Sheets PDF Phase A tiling: {e}"))?;

        log::debug!(
            "do_export_sheets_pdf: Phase A produced {} tile page(s).",
            phase_a_pages.len()
        );

        all_page_bytes.extend(phase_a_pages);
    } // Phase A

    // ---- Phase B: remaining pieces → one PDF page per sheet ----
    if !remaining_indices.is_empty() {
        log::debug!("do_export_sheets_pdf: Phase B — building remaining sheet SVGs.");

        let sheet_docs: Vec<Document> = build_remaining_svgs(
            flat_dom,
            all_pieces,
            &remaining_indices,
            content_w_px, content_h_px,
            gap_px,
            &trial_angles_deg,
            ml_px, mt_px, mr_px, mb_px,
        ).map_err(|e| format!("Sheets PDF Phase B: {e}"))?;

        log::debug!(
            "do_export_sheets_pdf: Phase B produced {} sheet SVG(s).",
            sheet_docs.len()
        );

        for (i, sheet_doc) in sheet_docs.iter().enumerate() {
            let page_bytes = render_svg_doc_to_pdf_bytes(
                sheet_doc,
                &format!("remaining sheet {i}"),
            ).map_err(|e| format!("Sheets PDF Phase B sheet {i}: {e}"))?;
            all_page_bytes.push(page_bytes);
        } // for sheet_doc
    } // Phase B

    Ok(all_page_bytes)
} // fn collect_sheet_pdf_page_bytes

// @brief Count pages in a PDF byte buffer.
//
// @param pdf_bytes Serialized PDF bytes.
// @return Number of pages in the document.
#[cfg(test)]
fn count_pdf_pages(pdf_bytes: &[u8]) -> usize {
    lopdf::Document::load_mem(pdf_bytes)
        .expect("PDF bytes should parse")
        .get_pages()
        .len()
} // fn count_pdf_pages

// @brief Count pages in a merged PDF file on disk.
//
// @param path Path to a PDF file.
// @return Number of pages in the document.
#[cfg(test)]
fn count_pdf_file_pages(path: &std::path::Path) -> usize {
    lopdf::Document::load(path)
        .expect("PDF file should parse")
        .get_pages()
        .len()
} // fn count_pdf_file_pages

// @brief Build a temporary unique path for test PDF output.
//
// @param name Logical basename for the temp file.
// @return Absolute temp path.
#[cfg(test)]
fn unique_test_pdf_path(name: &str) -> std::path::PathBuf {
    let unique = format!(
        "seamlylayout_{name}_{}_{}.pdf",
        std::process::id(),
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .expect("clock should be after epoch")
            .as_nanos()
    );
    std::env::temp_dir().join(unique)
} // fn unique_test_pdf_path

// @brief Build a minimal sheet-export settings object for tests.
//
// @return Parsed LayoutSettings with sheet paper type and Letter tile pages.
#[cfg(test)]
fn test_sheet_settings() -> LayoutSettings {
    LayoutSettings::from_json(
        r#"{
            "unit": "in",
            "mediaType": "paper",
            "paperType": "sheet",
            "pageWidth": 20.0,
            "pageHeight": 20.0,
            "marginLeft": 0.0,
            "marginRight": 0.0,
            "marginTop": 0.0,
            "marginBottom": 0.0,
            "pieceGap": 0.0,
            "layoutMode": "alongGrainline",
            "rotationStep": 180,
            "tileSize": "Letter"
        }"#,
    )
    .expect("test settings should parse")
} // fn test_sheet_settings

// @brief Build a minimal flat DOM with rectangular test pieces.
//
// @param dims Piece sizes as `(width, height)` pairs.
// @return Preprocessed DOM and extracted pieces.
#[cfg(test)]
fn flat_dom_and_pieces_for_dims(dims: &[(u32, u32)]) -> (Document, Vec<PieceRect>) {
    let groups: Vec<String> = dims
        .iter()
        .enumerate()
        .map(|(i, (w, h))| {
            format!(
                "<g id=\"p{i}\"><path d=\"M 0 0 L {w} 0 L {w} {h} L 0 {h} Z\"/></g>"
            )
        })
        .collect();
    let svg = format!(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4000\" height=\"4000\">{}</svg>",
        groups.join("")
    );
    let input = Document::parse(&svg).expect("test SVG should parse");
    build_sheet_export_inputs(&input).expect("sheet export inputs should build")
} // fn flat_dom_and_pieces_for_dims

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Preprocessing helper extracts translated pieces for sheet export.
    #[test]
    fn build_sheet_export_inputs_extracts_pieces() {
        let (flat_dom, pieces) = flat_dom_and_pieces_for_dims(&[(200, 100), (120, 80)]);
        assert_eq!(pieces.len(), 2, "expected both test pieces to be extracted");
        assert!(
            flat_dom.to_string().contains("id=\"p0\""),
            "preprocessed DOM should preserve piece groups"
        );
    } // build_sheet_export_inputs_extracts_pieces

    // @brief Pure remaining-piece export yields one PDF page per packed sheet.
    #[test]
    fn collect_sheet_pdf_page_bytes_remaining_only() {
        let settings = test_sheet_settings();
        let (flat_dom, pieces) = flat_dom_and_pieces_for_dims(&[(1200, 1200), (1200, 1200)]);

        let pages = collect_sheet_pdf_page_bytes(&flat_dom, &pieces, &settings)
            .expect("page collection should succeed");

        assert_eq!(pages.len(), 2, "two large remaining pieces should require two pages");
        for page in &pages {
            assert_eq!(count_pdf_pages(page), 1, "each collected buffer should be one page");
        }
    } // collect_sheet_pdf_page_bytes_remaining_only

    // @brief Mixed oversized + remaining export yields Phase A and Phase B pages.
    #[test]
    fn collect_sheet_pdf_page_bytes_mixed_phase_a_and_b() {
        let settings = test_sheet_settings();
        let (flat_dom, pieces) = flat_dom_and_pieces_for_dims(&[(2500, 300), (1200, 1200)]);
        let (content_w_px, content_h_px) = settings.effective_bin_px();
        let (oversized_indices, remaining_indices) =
            partition_oversized_pieces(&pieces, content_w_px, content_h_px);
        let oversized_doc = build_oversized_svg(
            &flat_dom,
            &pieces,
            &oversized_indices,
            settings.piece_gap_px(),
            &settings.rotation_trial_set_deg(),
            settings.margin_px().0,
            settings.margin_px().2,
            settings.margin_px().1,
            settings.margin_px().3,
        )
        .expect("oversized doc should build");
        let phase_a_pages = collect_tiled_pdf_page_bytes(&oversized_doc, &settings)
            .expect("phase A pages should render");

        let pages = collect_sheet_pdf_page_bytes(&flat_dom, &pieces, &settings)
            .expect("mixed page collection should succeed");

        assert_eq!(remaining_indices.len(), 1, "fixture should leave exactly one remaining piece");
        assert_eq!(
            pages.len(),
            phase_a_pages.len() + 1,
            "total pages should equal Phase A tiled pages plus one remaining-sheet page"
        );
        for page in &pages {
            assert_eq!(count_pdf_pages(page), 1, "each collected buffer should be one page");
        }
    } // collect_sheet_pdf_page_bytes_mixed_phase_a_and_b

    // @brief Disk export merges collected pages into one multi-page PDF file.
    #[test]
    fn do_export_sheets_pdf_writes_merged_pdf() {
        let settings = test_sheet_settings();
        let (flat_dom, pieces) = flat_dom_and_pieces_for_dims(&[(2500, 300), (1200, 1200)]);
        let out_path = unique_test_pdf_path("sheet_export");
        let expected_pages = collect_sheet_pdf_page_bytes(&flat_dom, &pieces, &settings)
            .expect("page collection should succeed")
            .len();

        do_export_sheets_pdf(
            &flat_dom,
            &pieces,
            out_path.to_string_lossy().as_ref(),
            &settings,
        )
        .expect("sheet export should succeed");

        assert_eq!(
            count_pdf_file_pages(&out_path),
            expected_pages,
            "merged PDF should contain all collected pages"
        );

        let _ = std::fs::remove_file(&out_path);
    } // do_export_sheets_pdf_writes_merged_pdf
} // mod tests
