// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file exports.rs
// @brief Pure export logic for all output formats.
//
// Each `do_export_*` function receives an already-cloned, already-stripped
// svg_dom::Document and a destination path string.  The functions contain no
// CXX-Qt or signal machinery — that lives in the thin wrappers in lib.rs.
//
// This separation makes every format independently testable without the
// CXX-Qt macro infrastructure.
//
// Exports:
//   do_export_dxf(doc, path, create_teaching_version, style, progress) -> Result<(), String>
//   dxf_missing_piece_data_message(doc)              -> Result<String, String>
//   do_export_pdf(doc, path)                          -> Result<(), String>
//   (doc, path, tile_dims)         -> Result<(), String>
//   do_export_png(doc, path, scale)                   -> Result<(), String>
//   do_export_jpeg(doc, path, progress)               -> Result<(), String>
//   do_export_svg(doc, path)                          -> Result<(), String>
//   parse_hpgl_options(options_json)                  -> Result<HpglOptions, String>
//   do_export_hpgl(doc, path, options)                -> Result<Vec<String>, String>
//   do_export_postscript(doc, path, flavor)           -> Result<Vec<String>, String>
//   do_export_gcode(doc, path)                        -> Result<(), String> (stub)
//   do_export_mesh(doc, path)                         -> Result<(), String> (stub)
//   paid_export_available(format)                     -> bool
//
// Internal helpers:
//   merge_single_page_pdfs(page_bytes) -> Result<Vec<u8>, String>

use std::path::Path;
use xmltree::{Element as XmlElement, XMLNode};

use ezdxf2dxfastm::{export_dxf_astm, DxfAstmExportOptions};
use seamly_svg2ezdxf::{svg_to_ezdxf, DxfVersion, MissingPieceData, SvgToEzdxfOptions};

use hpgl_writer::{HpglMode, HpglOptions, PenMap};
use ps_writer::PsFlavor;
use layout_tiling::{compute_tile_dims, measurement_to_px, LayoutSettings, TileDimensions};

// @brief Render an SVG DOM document to a single-page PDF byte buffer.
// @param doc      SVG DOM page content.
// @param context  Human-readable context included in error messages.
// @return PDF bytes on success; Err(message) on failure.
pub(crate) fn render_svg_doc_to_pdf_bytes(
    doc: &svg_dom::Document,
    context: &str,
) -> Result<Vec<u8>, String> {
    use svg2pdf::{ConversionOptions, PageOptions};

    // Serialize page DOM to SVG string for usvg parsing.
    let svg_str = doc.to_string();

    // Parse with system-font support so text rendering is stable across exports.
    let mut opt = usvg::Options::default();
    opt.fontdb_mut().load_system_fonts();
    let tree = usvg::Tree::from_data(svg_str.as_bytes(), &opt)
        .map_err(|e| format!("Tiled PDF: SVG parse failed for {context}: {e}"))?;

    // Render to one PDF page.
    Ok(svg2pdf::to_pdf(
        &tree,
        ConversionOptions::default(),
        PageOptions::default(),
    ))
}

// @brief Build the tiled-PDF thumbnail page document.
//
// Produces a first page where the full layout is uniformly scaled to fit
// inside the tile page trim area (paper size minus margins). The thumbnail is
// centered within the trim area, with a footer label in the bottom margin.
//
// @param doc          Full layout DOM.
// @param tile_dims    Tile/grid dimensions with paper + margin metrics.
// @param layout_w_px  Full layout width in px.
// @param layout_h_px  Full layout height in px.
// @return SVG DOM for thumbnail page on success; Err(message) otherwise.
fn build_tiled_pdf_thumbnail_doc(
    doc: &svg_dom::Document,
    tile_dims: &TileDimensions,
    layout_w_px: u32,
    layout_h_px: u32,
) -> Result<svg_dom::Document, String> {
    if layout_w_px == 0 || layout_h_px == 0 {
        return Err("Tiled PDF: cannot build thumbnail from zero-sized layout.".to_string());
    }

    // Physical page dimensions in px.
    let paper_w = tile_dims.trim_tile_w_px + tile_dims.margin_left_px + tile_dims.margin_right_px;
    let paper_h = tile_dims.trim_tile_h_px + tile_dims.margin_top_px + tile_dims.margin_bottom_px;

    // Trim area where the thumbnail must fit.
    let trim_w = tile_dims.trim_tile_w_px as f64;
    let trim_h = tile_dims.trim_tile_h_px as f64;
    let layout_w = layout_w_px as f64;
    let layout_h = layout_h_px as f64;

    // Uniform fit scale for the full layout into trim area.
    let thumb_scale = (trim_w / layout_w).min(trim_h / layout_h);
    if thumb_scale <= 0.0 || !thumb_scale.is_finite() {
        return Err("Tiled PDF: invalid thumbnail scale.".to_string());
    }

    // Center the scaled thumbnail inside the trim area.
    let drawn_w = layout_w * thumb_scale;
    let drawn_h = layout_h * thumb_scale;
    let offset_x = tile_dims.margin_left_px as f64 + (trim_w - drawn_w) / 2.0;
    let offset_y = tile_dims.margin_top_px as f64 + (trim_h - drawn_h) / 2.0;

    let mut thumb_doc = doc.clone();
    thumb_doc.root.attributes.insert(
        "viewBox".to_string(),
        format!("0 0 {paper_w:.4} {paper_h:.4}"),
    );
    thumb_doc
        .root
        .attributes
        .insert("width".to_string(), format!("{paper_w:.4}"));
    thumb_doc
        .root
        .attributes
        .insert("height".to_string(), format!("{paper_h:.4}"));

    // Wrap original root content with translate+scale transform so the full
    // layout appears as a centered thumbnail inside page margins.
    let existing_children = std::mem::take(&mut thumb_doc.root.children);
    let mut thumb_group = XmlElement {
        name: "g".to_string(),
        attributes: Default::default(),
        children: existing_children,
        namespace: None,
        prefix: None,
        namespaces: None,
    };
    thumb_group
        .attributes
        .insert("id".to_string(), "tiledPdfThumbnailGroup".to_string());
    thumb_group.attributes.insert(
        "transform".to_string(),
        format!("translate({offset_x:.4} {offset_y:.4}) scale({thumb_scale:.8})"),
    );
    thumb_doc.root.children.push(XMLNode::Element(thumb_group));

    // Draw a visible outline around the full scaled layout image on page 1.
    // This gives users a clear outer boundary for the overview thumbnail.
    let mut thumb_outline = XmlElement {
        name: "rect".to_string(),
        attributes: Default::default(),
        children: Vec::new(),
        namespace: None,
        prefix: None,
        namespaces: None,
    };
    thumb_outline
        .attributes
        .insert("id".to_string(), "tiledPdfThumbnailOutline".to_string());
    thumb_outline
        .attributes
        .insert("x".to_string(), format!("{offset_x:.4}"));
    thumb_outline
        .attributes
        .insert("y".to_string(), format!("{offset_y:.4}"));
    thumb_outline
        .attributes
        .insert("width".to_string(), format!("{drawn_w:.4}"));
    thumb_outline
        .attributes
        .insert("height".to_string(), format!("{drawn_h:.4}"));
    thumb_outline
        .attributes
        .insert("fill".to_string(), "none".to_string());
    thumb_outline
        .attributes
        .insert("stroke".to_string(), "#111111".to_string());
    thumb_outline
        .attributes
        .insert("stroke-width".to_string(), "1".to_string());
    thumb_outline.attributes.insert(
        "vector-effect".to_string(),
        "non-scaling-stroke".to_string(),
    );
    thumb_doc
        .root
        .children
        .push(XMLNode::Element(thumb_outline));

    // Add a page-1 footer in the bottom margin so users can identify this
    // first page as an overview thumbnail sheet.
    let footer_text = format!(
        "Thumbnail overview — {} columns × {} rows",
        tile_dims.tile_cols, tile_dims.tile_rows,
    );
    let footer_y = if tile_dims.margin_bottom_px >= 16 {
        paper_h as f64 - (tile_dims.margin_bottom_px as f64 / 2.0)
    } else {
        paper_h as f64 - 8.0
    };
    let footer_font_size = if tile_dims.margin_bottom_px >= 28 {
        12
    } else {
        10
    };

    let mut footer_elem = XmlElement {
        name: "text".to_string(),
        attributes: Default::default(),
        children: Vec::new(),
        namespace: None,
        prefix: None,
        namespaces: None,
    };
    footer_elem
        .attributes
        .insert("id".to_string(), "tiledPdfThumbnailFooter".to_string());
    footer_elem
        .attributes
        .insert("x".to_string(), format!("{:.4}", paper_w as f64 / 2.0));
    footer_elem
        .attributes
        .insert("y".to_string(), format!("{footer_y:.4}"));
    footer_elem
        .attributes
        .insert("text-anchor".to_string(), "middle".to_string());
    footer_elem
        .attributes
        .insert("dominant-baseline".to_string(), "middle".to_string());
    footer_elem
        .attributes
        .insert("font-family".to_string(), "sans-serif".to_string());
    footer_elem
        .attributes
        .insert("font-size".to_string(), footer_font_size.to_string());
    footer_elem
        .attributes
        .insert("fill".to_string(), "#111111".to_string());
    footer_elem.children.push(XMLNode::Text(footer_text));
    thumb_doc.root.children.push(XMLNode::Element(footer_elem));

    Ok(thumb_doc)
}

// @brief Build one tiled-PDF page document for a specific tile row and column.
//
// The returned page is paper-sized. Layout content is rendered through a
// trim-sized nested SVG viewport positioned inside the page margins, with an
// explicit clip path so off-tile geometry and labels cannot leak into the page.
//
// @param doc        Full layout DOM.
// @param tile_dims  Tile/grid dimensions with paper + margin metrics.
// @param row        Zero-based tile row.
// @param col        Zero-based tile column.
// @return SVG DOM for the requested tile page.
fn build_tiled_pdf_tile_doc(
    doc: &svg_dom::Document,
    tile_dims: &TileDimensions,
    row: u32,
    col: u32,
) -> svg_dom::Document {
    let tile_content_x = tile_dims.margin_left_px + col * tile_dims.trim_tile_w_px;
    let tile_content_y = tile_dims.margin_top_px + row * tile_dims.trim_tile_h_px;

    let paper_w = tile_dims.trim_tile_w_px + tile_dims.margin_left_px + tile_dims.margin_right_px;
    let paper_h = tile_dims.trim_tile_h_px + tile_dims.margin_top_px + tile_dims.margin_bottom_px;

    let mut tile_doc = doc.clone();
    let existing_children = std::mem::take(&mut tile_doc.root.children);

    tile_doc.root.attributes.remove("viewBox");
    tile_doc
        .root
        .attributes
        .insert("width".to_string(), format!("{paper_w:.4}"));
    tile_doc
        .root
        .attributes
        .insert("height".to_string(), format!("{paper_h:.4}"));

    let clip_id = format!("tileClipRect_r{row}_c{col}");
    let mut clip_rect = XmlElement {
        name: "rect".to_string(),
        attributes: Default::default(),
        children: Vec::new(),
        namespace: None,
        prefix: None,
        namespaces: None,
    };
    clip_rect
        .attributes
        .insert("x".to_string(), format!("{:.4}", tile_content_x));
    clip_rect
        .attributes
        .insert("y".to_string(), format!("{:.4}", tile_content_y));
    clip_rect.attributes.insert(
        "width".to_string(),
        format!("{:.4}", tile_dims.trim_tile_w_px),
    );
    clip_rect.attributes.insert(
        "height".to_string(),
        format!("{:.4}", tile_dims.trim_tile_h_px),
    );

    let mut clip_path = XmlElement {
        name: "clipPath".to_string(),
        attributes: Default::default(),
        children: vec![XMLNode::Element(clip_rect)],
        namespace: None,
        prefix: None,
        namespaces: None,
    };
    clip_path
        .attributes
        .insert("id".to_string(), clip_id.clone());
    clip_path
        .attributes
        .insert("clipPathUnits".to_string(), "userSpaceOnUse".to_string());

    let mut clip_defs = XmlElement {
        name: "defs".to_string(),
        attributes: Default::default(),
        children: vec![XMLNode::Element(clip_path)],
        namespace: None,
        prefix: None,
        namespaces: None,
    };
    clip_defs
        .attributes
        .insert("id".to_string(), format!("tileClipDefs_r{row}_c{col}"));

    let mut clipped_content = XmlElement {
        name: "g".to_string(),
        attributes: Default::default(),
        children: existing_children,
        namespace: None,
        prefix: None,
        namespaces: None,
    };
    clipped_content.attributes.insert(
        "id".to_string(),
        format!("tileContentClipGroup_r{row}_c{col}"),
    );
    clipped_content
        .attributes
        .insert("clip-path".to_string(), format!("url(#{clip_id})"));

    let mut tile_viewport = XmlElement {
        name: "svg".to_string(),
        attributes: Default::default(),
        children: vec![
            XMLNode::Element(clip_defs),
            XMLNode::Element(clipped_content),
        ],
        namespace: None,
        prefix: None,
        namespaces: None,
    };
    tile_viewport
        .attributes
        .insert("id".to_string(), format!("tileViewport_r{row}_c{col}"));
    tile_viewport
        .attributes
        .insert("x".to_string(), format!("{:.4}", tile_dims.margin_left_px));
    tile_viewport
        .attributes
        .insert("y".to_string(), format!("{:.4}", tile_dims.margin_top_px));
    tile_viewport.attributes.insert(
        "width".to_string(),
        format!("{:.4}", tile_dims.trim_tile_w_px),
    );
    tile_viewport.attributes.insert(
        "height".to_string(),
        format!("{:.4}", tile_dims.trim_tile_h_px),
    );
    tile_viewport
        .attributes
        .insert("overflow".to_string(), "hidden".to_string());
    tile_viewport.attributes.insert(
        "viewBox".to_string(),
        format!(
            "{:.4} {:.4} {:.4} {:.4}",
            tile_content_x, tile_content_y, tile_dims.trim_tile_w_px, tile_dims.trim_tile_h_px,
        ),
    );
    tile_doc.root.children.push(XMLNode::Element(tile_viewport));

    let trim_x = tile_dims.margin_left_px as f64;
    let trim_y = tile_dims.margin_top_px as f64;
    let trim_w = tile_dims.trim_tile_w_px as f64;
    let trim_h = tile_dims.trim_tile_h_px as f64;

    let mut trim_border = XmlElement {
        name: "rect".to_string(),
        attributes: Default::default(),
        children: Vec::new(),
        namespace: None,
        prefix: None,
        namespaces: None,
    };
    trim_border
        .attributes
        .insert("id".to_string(), format!("tileTrimBorder_r{row}_c{col}"));
    trim_border
        .attributes
        .insert("x".to_string(), format!("{trim_x:.4}"));
    trim_border
        .attributes
        .insert("y".to_string(), format!("{trim_y:.4}"));
    trim_border
        .attributes
        .insert("width".to_string(), format!("{trim_w:.4}"));
    trim_border
        .attributes
        .insert("height".to_string(), format!("{trim_h:.4}"));
    trim_border
        .attributes
        .insert("fill".to_string(), "none".to_string());
    trim_border
        .attributes
        .insert("stroke".to_string(), "#111111".to_string());
    trim_border
        .attributes
        .insert("stroke-width".to_string(), "1".to_string());
    trim_border.attributes.insert(
        "vector-effect".to_string(),
        "non-scaling-stroke".to_string(),
    );
    tile_doc.root.children.push(XMLNode::Element(trim_border));

    let tile_label_text = format!("row {}, col {}", row + 1, col + 1);
    let mut tile_label = XmlElement {
        name: "text".to_string(),
        attributes: Default::default(),
        children: Vec::new(),
        namespace: None,
        prefix: None,
        namespaces: None,
    };
    tile_label
        .attributes
        .insert("id".to_string(), format!("tileLabel_r{row}_c{col}"));
    tile_label
        .attributes
        .insert("x".to_string(), format!("{:.4}", trim_x + 8.0));
    tile_label
        .attributes
        .insert("y".to_string(), format!("{:.4}", trim_y + 24.0));
    tile_label
        .attributes
        .insert("font-family".to_string(), "sans-serif".to_string());
    tile_label
        .attributes
        .insert("font-size".to_string(), "20".to_string());
    tile_label
        .attributes
        .insert("fill".to_string(), "#111111".to_string());
    tile_label
        .attributes
        .insert("text-anchor".to_string(), "start".to_string());
    tile_label.children.push(XMLNode::Text(tile_label_text));
    tile_doc.root.children.push(XMLNode::Element(tile_label));

    let cut_left = col > 0;
    let cut_top = row > 0;

    if cut_left || cut_top {
        let scissor_symbol_id = format!("tileScissorSymbol_r{row}_c{col}");

        let mut scissor_group = XmlElement {
            name: "g".to_string(),
            attributes: Default::default(),
            children: Vec::new(),
            namespace: None,
            prefix: None,
            namespaces: None,
        };
        scissor_group
            .attributes
            .insert("id".to_string(), scissor_symbol_id.clone());
        scissor_group
            .attributes
            .insert("fill".to_string(), "none".to_string());
        scissor_group
            .attributes
            .insert("stroke".to_string(), "#111111".to_string());
        scissor_group
            .attributes
            .insert("stroke-width".to_string(), "1.25".to_string());
        scissor_group
            .attributes
            .insert("stroke-linecap".to_string(), "round".to_string());
        scissor_group
            .attributes
            .insert("stroke-linejoin".to_string(), "round".to_string());

        let mut blade_a = XmlElement {
            name: "line".to_string(),
            attributes: Default::default(),
            children: Vec::new(),
            namespace: None,
            prefix: None,
            namespaces: None,
        };
        blade_a.attributes.insert("x1".to_string(), "0".to_string());
        blade_a.attributes.insert("y1".to_string(), "0".to_string());
        blade_a.attributes.insert("x2".to_string(), "7".to_string());
        blade_a
            .attributes
            .insert("y2".to_string(), "-5".to_string());

        let mut blade_b = XmlElement {
            name: "line".to_string(),
            attributes: Default::default(),
            children: Vec::new(),
            namespace: None,
            prefix: None,
            namespaces: None,
        };
        blade_b.attributes.insert("x1".to_string(), "0".to_string());
        blade_b.attributes.insert("y1".to_string(), "0".to_string());
        blade_b.attributes.insert("x2".to_string(), "7".to_string());
        blade_b.attributes.insert("y2".to_string(), "5".to_string());

        let mut handle_a = XmlElement {
            name: "circle".to_string(),
            attributes: Default::default(),
            children: Vec::new(),
            namespace: None,
            prefix: None,
            namespaces: None,
        };
        handle_a
            .attributes
            .insert("cx".to_string(), "-3.5".to_string());
        handle_a
            .attributes
            .insert("cy".to_string(), "-2.5".to_string());
        handle_a.attributes.insert("r".to_string(), "2".to_string());

        let mut handle_b = XmlElement {
            name: "circle".to_string(),
            attributes: Default::default(),
            children: Vec::new(),
            namespace: None,
            prefix: None,
            namespaces: None,
        };
        handle_b
            .attributes
            .insert("cx".to_string(), "-3.5".to_string());
        handle_b
            .attributes
            .insert("cy".to_string(), "2.5".to_string());
        handle_b.attributes.insert("r".to_string(), "2".to_string());

        scissor_group.children.push(XMLNode::Element(blade_a));
        scissor_group.children.push(XMLNode::Element(blade_b));
        scissor_group.children.push(XMLNode::Element(handle_a));
        scissor_group.children.push(XMLNode::Element(handle_b));

        let mut defs = XmlElement {
            name: "defs".to_string(),
            attributes: Default::default(),
            children: vec![XMLNode::Element(scissor_group)],
            namespace: None,
            prefix: None,
            namespaces: None,
        };
        defs.attributes
            .insert("id".to_string(), format!("tileCutDefs_r{row}_c{col}"));
        tile_doc.root.children.push(XMLNode::Element(defs));

        if cut_left {
            let icon_x = trim_x;
            let icon_y = trim_y + (trim_h / 2.0);

            let mut left_scissor_use = XmlElement {
                name: "use".to_string(),
                attributes: Default::default(),
                children: Vec::new(),
                namespace: None,
                prefix: None,
                namespaces: None,
            };
            left_scissor_use
                .attributes
                .insert("href".to_string(), format!("#{scissor_symbol_id}"));
            left_scissor_use.attributes.insert(
                "transform".to_string(),
                format!("translate({icon_x:.4} {icon_y:.4}) rotate(-90) scale(2)"),
            );
            tile_doc
                .root
                .children
                .push(XMLNode::Element(left_scissor_use));
        }

        if cut_top {
            let icon_x = trim_x + (trim_w / 2.0);
            let icon_y = trim_y;

            let mut top_scissor_use = XmlElement {
                name: "use".to_string(),
                attributes: Default::default(),
                children: Vec::new(),
                namespace: None,
                prefix: None,
                namespaces: None,
            };
            top_scissor_use
                .attributes
                .insert("href".to_string(), format!("#{scissor_symbol_id}"));
            top_scissor_use.attributes.insert(
                "transform".to_string(),
                format!("translate({icon_x:.4} {icon_y:.4}) scale(2)"),
            );
            tile_doc
                .root
                .children
                .push(XMLNode::Element(top_scissor_use));
        }
    }

    tile_doc
}

// ---------------------------------------------------------------------------
// DXF-ASTM export
// ---------------------------------------------------------------------------

// @brief Frontend choices for a DXF-ASTM export: style system text, CLO3D variant and DXF version.
// @details Local date and time come from Qt, which knows the user's time zone;
//          absent values fall back to UTC in the writer.
#[derive(Debug, Clone, Default)]
pub struct DxfStyleInfo {
    // CLO3D variant: add group 250 to boundary and sew line polylines.
    pub clo3d_group_250: bool,
    // File version: R12 (AC1009) or R13 (AC1012).
    pub dxf_version: DxfVersion,
    // Seamly2D suite version for `Author:`.
    pub app_version: Option<String>,
    // `Creation Date:` as dd-mm-yyyy.
    pub creation_date: Option<String>,
    // `Creation Time:` as hh-mm.
    pub creation_time: Option<String>,
    // Input base name for `Style Name:`: the imported SVG file's base name, or
    // the `--document-name` value. None or blank when the input has no name.
    pub input_name: Option<String>,
}

// @brief Export a stripped layout document to a DXF-ASTM file.
//
// Pipeline:
//   1. SVG DOM → ezdxf Drawing via svg_to_ezdxf.      (progress → 10% then 50%)
//   2. Drawing → DXF-ASTM file via export_dxf_astm.   (progress → 90%)
//   3. Optionally generate teaching version (.txt).
//
// @param doc                    Cloned, piece-fill-stripped layout DOM.
// @param path                   Destination file path.
// @param create_teaching_version When true, emits teaching-version DXF annotations.
// @param style                  Style system text supplied by the frontend.
// @param progress               Callback invoked with integer percent (0–100) at each stage.
//                               The caller owns 0% (before call) and 100% (after Ok return).
// @return Ok(()) on success; Err(message) on any failure.
pub fn do_export_dxf(
    doc: &svg_dom::Document,
    path: &str,
    create_teaching_version: bool,
    style: &DxfStyleInfo,
    progress: &mut impl FnMut(i32),
) -> Result<(), String> {
    crate::log_to_file(&format!("[exports.rs] do_export_dxf(): 1 converting SVG DOM to ezdxf Drawing for '{path}' teaching_version={create_teaching_version} version={:?}", style.dxf_version));

    // Stage 1 start: SVG DOM → ezdxf Drawing (~10% of total work).
    progress(10);

    // The drawing's version selects the DXF file version the writer produces.
    let svg_opts = SvgToEzdxfOptions { dxf_version: style.dxf_version, ..SvgToEzdxfOptions::default() };
    let mut drawing = svg_to_ezdxf(doc, &svg_opts)
        .map_err(|e| {
            crate::log_to_file(&format!("[exports.rs] do_export_dxf(): 2 SVG→ezdxf conversion failed: {e}"));
            format!("DXF conversion failed: {e}")
        })?; // if svg_to_ezdxf failed

    crate::log_to_file(&format!(
        "[exports.rs] do_export_dxf(): 2 ezdxf Drawing ready ({} blocks, {} modelspace entities); writing DXF-ASTM to '{path}'",
        drawing.blocks.len(),
        drawing.modelspace_entities.len(),
    ));

    // Stage 1 complete / Stage 2 start: write DXF-ASTM file (~50% of total work).
    progress(50);

    // Step 2: Drawing → DXF-ASTM file.
    // Style Name precedence: input base name, then the pattern `data-name`,
    // then the output file stem. The writer prefers the drawing's name.
    if let Some(name) = style.input_name.as_deref().map(str::trim).filter(|n| !n.is_empty()) {
        drawing.style_name = Some(name.to_string());
    } // if input name
    let defaults = DxfAstmExportOptions::default();
    let export_opts = DxfAstmExportOptions {
        create_teaching_version,
        style_name: Path::new(path).file_stem().map(|s| s.to_string_lossy().into_owned()),
        author_release: style.app_version.clone().filter(|v| !v.is_empty()).unwrap_or(defaults.author_release.clone()),
        creation_date: style.creation_date.clone(),
        creation_time: style.creation_time.clone(),
        clo3d_group_250: style.clo3d_group_250,
        ..defaults
    }; // export_opts
    let result = export_dxf_astm(&drawing, Path::new(path), &export_opts)
        .map_err(|e| {
            crate::log_to_file(&format!("[exports.rs] do_export_dxf(): 3 DXF write failed: {e}"));
            format!("DXF export failed: {e}")
        }); // if export_dxf_astm failed

    if result.is_ok() {
        // Stage 2 complete: DXF file written (and teaching version, if requested).
        progress(90);
        if create_teaching_version {
            crate::log_to_file(&format!("[exports.rs] do_export_dxf(): 3 wrote DXF '{path}' and teaching version (.txt)"));
        } else {
            crate::log_to_file(&format!("[exports.rs] do_export_dxf(): 3 wrote DXF '{path}'"));
        } // if teaching version
    } // if ok

    result
} // fn do_export_dxf

// @brief Warning text for pieces that lack DXF-ASTM data, shown before a DXF-ASTM export.
// @param doc Cloned, piece-fill-stripped layout DOM.
// @return Ok("") when every piece is complete; Ok(text) naming each piece and
//         what it lacks; Err(message) when the DOM does not convert.
pub fn dxf_missing_piece_data_message(doc: &svg_dom::Document) -> Result<String, String> {
    let drawing = svg_to_ezdxf(doc, &SvgToEzdxfOptions::default()).map_err(|e| format!("DXF conversion failed: {e}"))?;
    let missing = drawing.missing_piece_data();
    crate::log_to_file(&format!("[exports.rs] dxf_missing_piece_data_message(): {} incomplete pieces", missing.len()));
    Ok(missing_piece_data_text(&missing))
} // fn dxf_missing_piece_data_message

// @brief One line per piece, then the effect on the exported file.
// @return Empty string when `missing` is empty.
fn missing_piece_data_text(missing: &[MissingPieceData]) -> String {
    if missing.is_empty() {
        return String::new();
    } // if complete

    let mut lines = vec!["These pieces lack data that DXF-ASTM uses:".to_string(), String::new()];
    for m in missing {
        // Each flag names one absent item; Quantity comes from a "Cut N" label line.
        let mut lacks = Vec::new();
        if m.quantity {
            lacks.push("Quantity (no \"Cut N\" label line)");
        } // if no quantity
        if m.label {
            lacks.push("piece label");
        } // if no label
        if m.grainline {
            lacks.push("grainline");
        } // if no grainline
        lines.push(format!("\u{2022} {}: no {}", m.piece_name, lacks.join(", no ")));
    } // for each piece
    lines.push(String::new());
    lines.push("This affects the usability of the exported DXF file in downstream CAD systems.".to_string());
    lines.join("\n")
} // fn missing_piece_data_text

// ---------------------------------------------------------------------------
// PDF export (single page)
// ---------------------------------------------------------------------------

// @brief Export a stripped layout document to a single-page PDF file.
//
// Pipeline:
//   1. DOM → usvg::Tree via app_core::document_to_tree.   (progress → 10%)
//   2. usvg::Tree → PDF bytes via svg2pdf (vector quality preserved).
//   3. Write bytes to disk.                                (progress → 90%)
//
// @param doc       Cloned, piece-fill-stripped layout DOM.
// @param path      Destination file path.
// @param progress  Callback invoked with integer percent (0–100) at each stage.
//                  The caller owns 0% (before call) and 100% (after Ok return).
// @return Ok(()) on success; Err(message) on any failure.
pub fn do_export_pdf(
    doc: &svg_dom::Document,
    path: &str,
    progress: &mut impl FnMut(i32),
) -> Result<(), String> {
    crate::log_to_file(&format!("[exports.rs] do_export_pdf(): 1 parsing SVG DOM to usvg tree for '{path}'"));

    // Stage 1 start: DOM → usvg tree (~10% of total work).
    progress(10);

    // Step 1: DOM → usvg tree.
    let tree = app_core::document_to_tree(doc, None)
        .map_err(|e| {
            crate::log_to_file(&format!("[exports.rs] do_export_pdf(): 2 SVG parse failed: {e}"));
            format!("PDF export failed — SVG parse error: {e}")
        })?; // if parse failed

    crate::log_to_file(&format!("[exports.rs] do_export_pdf(): 2 SVG parsed; rendering PDF to '{path}'"));

    // Step 2–3: usvg tree → PDF bytes → file.
    let result = app_core::render_pdf(&tree, Path::new(path))
        .map_err(|e| {
            crate::log_to_file(&format!("[exports.rs] do_export_pdf(): 3 render failed: {e}"));
            format!("PDF export failed — render error: {e}")
        }); // if render failed

    if result.is_ok() {
        // Stage 3 complete: PDF file written.
        progress(90);
        crate::log_to_file(&format!("[exports.rs] do_export_pdf(): 3 wrote PDF '{path}'"));
    } // if ok

    result
} // fn do_export_pdf

// ---------------------------------------------------------------------------
// PDF Tiled export (multi-page) — inner implementation
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// PDF Tiled export — collect pages as bytes (no disk I/O)
// ---------------------------------------------------------------------------

// @brief Render a layout DOM into tiled PDF pages and return them as byte buffers.
//
// Extracted from `do_export_pdf_tile_inner` so the sheets orchestrator
// (`sheets::do_export_sheets_pdf`) can collect Phase A tile pages and Phase B
// sheet pages together before a single merged write.
//
// The returned Vec is ordered: [thumbnail page, tile pages row-major].
//
// @param doc       Layout DOM to tile (already stripped / sized).
// @param settings  Parsed LayoutSettings; must have a valid `tile_size`.
// @return Vec of single-page PDF byte buffers; Err on any failure.
pub(crate) fn collect_tiled_pdf_page_bytes(
    doc: &svg_dom::Document,
    settings: &LayoutSettings,
) -> Result<Vec<Vec<u8>>, String> {
    // Collect per-page single-page PDF bytes.
    // Order: [thumbnail page, tile pages row-major].
    let mut tile_pages: Vec<Vec<u8>> = Vec::new();

    // Use current layout DOM dimensions as the input area for tile-grid reconstruction.
    let layout_w_px = doc
        .root
        .attributes
        .get("width")
        .map(|s| measurement_to_px(s))
        .unwrap_or(0);
    let layout_h_px = doc
        .root
        .attributes
        .get("height")
        .map(|s| measurement_to_px(s))
        .unwrap_or(0);

    // Get tile dimensions from parsed settings and current layout size.
    let tile_dims: TileDimensions = compute_tile_dims(layout_w_px, layout_h_px, settings)
        .map_err(|e| format!("Tiled PDF: invalid tile dimensions: {e}"))?;

    // Physical paper page size in pixels (trim area + margins).
    let paper_w = tile_dims.trim_tile_w_px + tile_dims.margin_left_px + tile_dims.margin_right_px;
    let paper_h = tile_dims.trim_tile_h_px + tile_dims.margin_top_px + tile_dims.margin_bottom_px;

    // First page: full-layout thumbnail scaled to fit inside page margins.
    let thumb_doc = build_tiled_pdf_thumbnail_doc(doc, &tile_dims, layout_w_px, layout_h_px)?;
    tile_pages.push(render_svg_doc_to_pdf_bytes(&thumb_doc, "thumbnail page")?);

    for row in 0..tile_dims.tile_rows {
        for col in 0..tile_dims.tile_cols {
            // Tile origin in CONTENT coordinates (inside contentRect only).
            //
            // Export each page at PAPER size and place the trimmed tile inside
            // the page margins by using a paper-sized viewport anchored so the
            // trimmed content lands at (margin_left, margin_top) on the page.
            let tile_content_x = tile_dims.margin_left_px + col * tile_dims.trim_tile_w_px;
            let tile_content_y = tile_dims.margin_top_px + row * tile_dims.trim_tile_h_px;

            // Build page as full paper-size root with a nested trim-sized SVG
            // viewport placed at fixed margins. This avoids overlapping paper-
            // width windows and removes strip/blank artifacts.
            let mut tile_doc = doc.clone();

            // Extract existing layout children (<defs>, piece groups, etc.).
            let existing_children = std::mem::take(&mut tile_doc.root.children);

            let clip_id = format!("tileClipRect_r{row}_c{col}");
            let mut clip_rect = XmlElement {
                name: "rect".to_string(),
                attributes: Default::default(),
                children: Vec::new(),
                namespace: None,
                prefix: None,
                namespaces: None,
            };
            clip_rect
                .attributes
                .insert("x".to_string(), format!("{:.4}", tile_content_x));
            clip_rect
                .attributes
                .insert("y".to_string(), format!("{:.4}", tile_content_y));
            clip_rect.attributes.insert(
                "width".to_string(),
                format!("{:.4}", tile_dims.trim_tile_w_px),
            );
            clip_rect.attributes.insert(
                "height".to_string(),
                format!("{:.4}", tile_dims.trim_tile_h_px),
            );

            let mut clip_path = XmlElement {
                name: "clipPath".to_string(),
                attributes: Default::default(),
                children: vec![XMLNode::Element(clip_rect)],
                namespace: None,
                prefix: None,
                namespaces: None,
            };
            clip_path
                .attributes
                .insert("id".to_string(), clip_id.clone());
            clip_path
                .attributes
                .insert("clipPathUnits".to_string(), "userSpaceOnUse".to_string());

            let mut clip_defs = XmlElement {
                name: "defs".to_string(),
                attributes: Default::default(),
                children: vec![XMLNode::Element(clip_path)],
                namespace: None,
                prefix: None,
                namespaces: None,
            };
            clip_defs
                .attributes
                .insert("id".to_string(), format!("tileClipDefs_r{row}_c{col}"));

            let mut clipped_content = XmlElement {
                name: "g".to_string(),
                attributes: Default::default(),
                children: existing_children,
                namespace: None,
                prefix: None,
                namespaces: None,
            };
            clipped_content.attributes.insert(
                "id".to_string(),
                format!("tileContentClipGroup_r{row}_c{col}"),
            );
            clipped_content
                .attributes
                .insert("clip-path".to_string(), format!("url(#{clip_id})"));

            // Root page geometry: full paper size, no root viewBox windowing.
            tile_doc.root.attributes.remove("viewBox");
            tile_doc
                .root
                .attributes
                .insert("width".to_string(), format!("{paper_w:.4}"));
            tile_doc
                .root
                .attributes
                .insert("height".to_string(), format!("{paper_h:.4}"));

            // Nested tile viewport: trim-sized window into the full layout,
            // positioned at page margins.
            let mut tile_viewport = XmlElement {
                name: "svg".to_string(),
                attributes: Default::default(),
                children: vec![
                    XMLNode::Element(clip_defs),
                    XMLNode::Element(clipped_content),
                ],
                namespace: None,
                prefix: None,
                namespaces: None,
            };
            tile_viewport
                .attributes
                .insert("id".to_string(), format!("tileViewport_r{row}_c{col}"));
            tile_viewport
                .attributes
                .insert("x".to_string(), format!("{:.4}", tile_dims.margin_left_px));
            tile_viewport
                .attributes
                .insert("y".to_string(), format!("{:.4}", tile_dims.margin_top_px));
            tile_viewport.attributes.insert(
                "width".to_string(),
                format!("{:.4}", tile_dims.trim_tile_w_px),
            );
            tile_viewport.attributes.insert(
                "height".to_string(),
                format!("{:.4}", tile_dims.trim_tile_h_px),
            );
            tile_viewport
                .attributes
                .insert("overflow".to_string(), "hidden".to_string());
            tile_viewport.attributes.insert(
                "viewBox".to_string(),
                format!(
                    "{:.4} {:.4} {:.4} {:.4}",
                    tile_content_x,
                    tile_content_y,
                    tile_dims.trim_tile_w_px,
                    tile_dims.trim_tile_h_px,
                ),
            );

            tile_doc.root.children.push(XMLNode::Element(tile_viewport));

            // Draw trim boundary on the paper page using configured margins and
            // trim dimensions (no hard-coded page offsets).
            let trim_x = tile_dims.margin_left_px as f64;
            let trim_y = tile_dims.margin_top_px as f64;
            let trim_w = tile_dims.trim_tile_w_px as f64;
            let trim_h = tile_dims.trim_tile_h_px as f64;

            let mut trim_border = XmlElement {
                name: "rect".to_string(),
                attributes: Default::default(),
                children: Vec::new(),
                namespace: None,
                prefix: None,
                namespaces: None,
            };
            trim_border
                .attributes
                .insert("id".to_string(), format!("tileTrimBorder_r{row}_c{col}"));
            trim_border
                .attributes
                .insert("x".to_string(), format!("{trim_x:.4}"));
            trim_border
                .attributes
                .insert("y".to_string(), format!("{trim_y:.4}"));
            trim_border
                .attributes
                .insert("width".to_string(), format!("{trim_w:.4}"));
            trim_border
                .attributes
                .insert("height".to_string(), format!("{trim_h:.4}"));
            trim_border
                .attributes
                .insert("fill".to_string(), "none".to_string());
            trim_border
                .attributes
                .insert("stroke".to_string(), "#111111".to_string());
            trim_border
                .attributes
                .insert("stroke-width".to_string(), "1".to_string());
            trim_border.attributes.insert(
                "vector-effect".to_string(),
                "non-scaling-stroke".to_string(),
            );
            tile_doc.root.children.push(XMLNode::Element(trim_border));

            // Add tile row/column label inside the upper-left corner of the
            // trim rectangle for assembly reference.
            let tile_label_text = format!("row {}, col {}", row + 1, col + 1);
            let mut tile_label = XmlElement {
                name: "text".to_string(),
                attributes: Default::default(),
                children: Vec::new(),
                namespace: None,
                prefix: None,
                namespaces: None,
            };
            tile_label
                .attributes
                .insert("id".to_string(), format!("tileLabel_r{row}_c{col}"));
            tile_label
                .attributes
                .insert("x".to_string(), format!("{:.4}", trim_x + 8.0));
            tile_label
                .attributes
                .insert("y".to_string(), format!("{:.4}", trim_y + 24.0));
            tile_label
                .attributes
                .insert("font-family".to_string(), "sans-serif".to_string());
            tile_label
                .attributes
                .insert("font-size".to_string(), "20".to_string());
            tile_label
                .attributes
                .insert("fill".to_string(), "#111111".to_string());
            tile_label
                .attributes
                .insert("text-anchor".to_string(), "start".to_string());
            tile_label.children.push(XMLNode::Text(tile_label_text));
            tile_doc.root.children.push(XMLNode::Element(tile_label));

            // Cut-edge policy for overlap assembly:
            // - cut left edge when col > 0 (overlap onto previous tile in row)
            // - cut top edge  when row > 0 (overlap onto previous row)
            let cut_left = col > 0;
            let cut_top = row > 0;

            if cut_left || cut_top {
                // Per-page vector scissor symbol in defs; instantiated with <use>.
                let scissor_symbol_id = format!("tileScissorSymbol_r{row}_c{col}");

                let mut scissor_group = XmlElement {
                    name: "g".to_string(),
                    attributes: Default::default(),
                    children: Vec::new(),
                    namespace: None,
                    prefix: None,
                    namespaces: None,
                };
                scissor_group
                    .attributes
                    .insert("id".to_string(), scissor_symbol_id.clone());
                scissor_group
                    .attributes
                    .insert("fill".to_string(), "none".to_string());
                scissor_group
                    .attributes
                    .insert("stroke".to_string(), "#111111".to_string());
                scissor_group
                    .attributes
                    .insert("stroke-width".to_string(), "1.25".to_string());
                scissor_group
                    .attributes
                    .insert("stroke-linecap".to_string(), "round".to_string());
                scissor_group
                    .attributes
                    .insert("stroke-linejoin".to_string(), "round".to_string());

                // Blade lines.
                let mut blade_a = XmlElement {
                    name: "line".to_string(),
                    attributes: Default::default(),
                    children: Vec::new(),
                    namespace: None,
                    prefix: None,
                    namespaces: None,
                };
                blade_a.attributes.insert("x1".to_string(), "0".to_string());
                blade_a.attributes.insert("y1".to_string(), "0".to_string());
                blade_a.attributes.insert("x2".to_string(), "7".to_string());
                blade_a
                    .attributes
                    .insert("y2".to_string(), "-5".to_string());

                let mut blade_b = XmlElement {
                    name: "line".to_string(),
                    attributes: Default::default(),
                    children: Vec::new(),
                    namespace: None,
                    prefix: None,
                    namespaces: None,
                };
                blade_b.attributes.insert("x1".to_string(), "0".to_string());
                blade_b.attributes.insert("y1".to_string(), "0".to_string());
                blade_b.attributes.insert("x2".to_string(), "7".to_string());
                blade_b.attributes.insert("y2".to_string(), "5".to_string());

                // Handle loops.
                let mut handle_a = XmlElement {
                    name: "circle".to_string(),
                    attributes: Default::default(),
                    children: Vec::new(),
                    namespace: None,
                    prefix: None,
                    namespaces: None,
                };
                handle_a
                    .attributes
                    .insert("cx".to_string(), "-3.5".to_string());
                handle_a
                    .attributes
                    .insert("cy".to_string(), "-2.5".to_string());
                handle_a.attributes.insert("r".to_string(), "2".to_string());

                let mut handle_b = XmlElement {
                    name: "circle".to_string(),
                    attributes: Default::default(),
                    children: Vec::new(),
                    namespace: None,
                    prefix: None,
                    namespaces: None,
                };
                handle_b
                    .attributes
                    .insert("cx".to_string(), "-3.5".to_string());
                handle_b
                    .attributes
                    .insert("cy".to_string(), "2.5".to_string());
                handle_b.attributes.insert("r".to_string(), "2".to_string());

                scissor_group.children.push(XMLNode::Element(blade_a));
                scissor_group.children.push(XMLNode::Element(blade_b));
                scissor_group.children.push(XMLNode::Element(handle_a));
                scissor_group.children.push(XMLNode::Element(handle_b));

                let mut defs = XmlElement {
                    name: "defs".to_string(),
                    attributes: Default::default(),
                    children: vec![XMLNode::Element(scissor_group)],
                    namespace: None,
                    prefix: None,
                    namespaces: None,
                };
                defs.attributes
                    .insert("id".to_string(), format!("tileCutDefs_r{row}_c{col}"));
                tile_doc.root.children.push(XMLNode::Element(defs));

                if cut_left {
                    // Place icon centered on the cut line.
                    let icon_x = trim_x;
                    let icon_y = trim_y + (trim_h / 2.0);

                    let mut left_scissor_use = XmlElement {
                        name: "use".to_string(),
                        attributes: Default::default(),
                        children: Vec::new(),
                        namespace: None,
                        prefix: None,
                        namespaces: None,
                    };
                    left_scissor_use
                        .attributes
                        .insert("href".to_string(), format!("#{scissor_symbol_id}"));
                    // Rotate -90° so blades point toward the left edge.
                    left_scissor_use.attributes.insert(
                        "transform".to_string(),
                        format!("translate({icon_x:.4} {icon_y:.4}) rotate(-90) scale(2)"),
                    );
                    tile_doc
                        .root
                        .children
                        .push(XMLNode::Element(left_scissor_use));
                } // if cut_left

                if cut_top {
                    let icon_x = trim_x + (trim_w / 2.0);
                    // Place icon centered on the cut line.
                    let icon_y = trim_y;

                    let mut top_scissor_use = XmlElement {
                        name: "use".to_string(),
                        attributes: Default::default(),
                        children: Vec::new(),
                        namespace: None,
                        prefix: None,
                        namespaces: None,
                    };
                    top_scissor_use
                        .attributes
                        .insert("href".to_string(), format!("#{scissor_symbol_id}"));
                    // Keep default symbol orientation for top-edge marker.
                    top_scissor_use.attributes.insert(
                        "transform".to_string(),
                        format!("translate({icon_x:.4} {icon_y:.4}) scale(2)"),
                    );
                    tile_doc
                        .root
                        .children
                        .push(XMLNode::Element(top_scissor_use));
                } // if cut_top
            } // if cut_left || cut_top

            // Render this tile to a single-page PDF.
            tile_pages.push(render_svg_doc_to_pdf_bytes(
                &tile_doc,
                &format!("tile ({row},{col})"),
            )?);
        } // for col
    } // for row

    if tile_pages.is_empty() {
        return Err("Tiled PDF: no tiles to export.".to_string()); // if no tiles
    } // if empty

    Ok(tile_pages)
} // fn collect_tiled_pdf_page_bytes

// ---------------------------------------------------------------------------
// PDF Tiled export — inner (collects + merges + writes)
// ---------------------------------------------------------------------------

// @brief Core multi-page tiled PDF export logic (disk-writing wrapper).
//
// Delegates page rendering to `collect_tiled_pdf_page_bytes`, merges the
// resulting single-page buffers with `merge_single_page_pdfs`, and writes
// the merged PDF to `path`.
//
// Does NOT check `settings.paper_type` — the caller is responsible for
// ensuring the settings have a valid `tile_size` before calling.
//
// @param doc       Layout DOM (piece-fill-stripped by the caller).
// @param path      Destination file path.
// @param settings  Parsed LayoutSettings; must have a valid `tile_size`.
// @return Ok(()) on success; Err(message) on any failure.
fn do_export_pdf_tile_inner(
    doc: &svg_dom::Document,
    path: &str,
    settings: &LayoutSettings,
) -> Result<(), String> {
    use std::fs;

    crate::log_to_file(&format!("[exports.rs] do_export_pdf_tile_inner(): 1 collecting tile pages for '{path}' tile_size='{}'", settings.tile_size));

    let tile_pages = collect_tiled_pdf_page_bytes(doc, settings)?;
    let page_count = tile_pages.len();

    crate::log_to_file(&format!("[exports.rs] do_export_pdf_tile_inner(): 2 merging {page_count} pages"));

    let merged = merge_single_page_pdfs(tile_pages)?;

    crate::log_to_file(&format!("[exports.rs] do_export_pdf_tile_inner(): 3 writing merged PDF ({} bytes) to '{path}'", merged.len()));

    let result = fs::write(path, &merged).map_err(|e| {
        crate::log_to_file(&format!("[exports.rs] do_export_pdf_tile_inner(): 4 write failed: {e}"));
        format!("Tiled PDF: write failed: {e}")
    }); // if write failed

    if result.is_ok() {
        crate::log_to_file(&format!("[exports.rs] do_export_pdf_tile_inner(): 4 wrote tiled PDF '{path}' ({page_count} pages)"));
    } // if ok

    result
} // fn do_export_pdf_tile_inner

// ---------------------------------------------------------------------------
// PDF Tiled export — Qt-facing public wrapper
// ---------------------------------------------------------------------------

// @brief Export a layout document as a multi-page tiled PDF (Qt-facing entry point).
//
// Parses `settings_json`, validates that `paper_type == "tiled"`, then delegates
// to `do_export_pdf_tile_inner`.  The Qt bridge calls this from `export_pdf_tiled()`.
//
// @param doc           Cloned, piece-fill-stripped layout DOM.
// @param path          Destination file path.
// @param settings_json Layout settings JSON from QML/C++.
// @return Ok(()) on success; Err(message) on any failure.
pub fn do_export_pdf_tile(
    doc: &svg_dom::Document,
    path: &str,
    settings_json: &cxx_qt_lib::QString,
) -> Result<(), String> {
    crate::log_to_file(&format!("[exports.rs] do_export_pdf_tile(): 1 parsing settings for '{path}'"));

    // Parse layout settings from the Qt JSON string.
    let settings = LayoutSettings::from_json(&settings_json.to_string())
        .map_err(|e| {
            crate::log_to_file(&format!("[exports.rs] do_export_pdf_tile(): 2 invalid settings JSON: {e}"));
            format!("Tiled PDF: invalid settings JSON: {e}")
        })?;

    // Guard: this function is only valid for the "tiled" paper_type.
    if settings.paper_type != "tiled" {
        crate::log_to_file(&format!("[exports.rs] do_export_pdf_tile(): 2 paper_type='{}' is not 'tiled'", settings.paper_type));
        return Err("Tiled PDF: export requested while paperType is not 'tiled'.".to_string());
    } // if not tiled

    crate::log_to_file(&format!("[exports.rs] do_export_pdf_tile(): 2 paper_type='tiled' tile_size='{}'; delegating to inner", settings.tile_size));

    do_export_pdf_tile_inner(doc, path, &settings)
} // fn do_export_pdf_tile

// ---------------------------------------------------------------------------
// PDF Tiled export — Phase A / oversized pieces entry point
// ---------------------------------------------------------------------------

// @brief Export an oversized-pieces SVG as a multi-page tiled PDF (Phase A).
//
// Called from `oversized.rs` during L.2.1 Phase A processing to feed the
// assembled "oversized" SVG Document into the tiled-PDF pipeline.
//
// Unlike the Qt-facing `do_export_pdf_tile`, this function:
//   - Takes a plain Rust `&LayoutSettings` instead of a Qt JSON string.
//   - Does NOT check `paper_type` — the caller (oversized.rs) is responsible
//     for providing settings with a valid `tile_size`.
//
// @param doc      Oversized SVG Document produced by `oversized::build_oversized_svg`.
// @param path     Destination PDF file path.
// @param settings Parsed settings; `tile_size` must be a valid named tile size
//                 (e.g., "Letter", "A4") so `compute_tile_dims` can succeed.
// @return Ok(()) on success; Err(message) on any failure.
#[allow(dead_code)] // TODO: remove or use once sheet-mode / Phase A callers are finalized
pub(crate) fn do_export_pdf_tile_with_settings(
    doc: &svg_dom::Document,
    path: &str,
    settings: &LayoutSettings,
) -> Result<(), String> {
    do_export_pdf_tile_inner(doc, path, settings)
} // fn do_export_pdf_tile_with_settings

// ---------------------------------------------------------------------------
// PDF multi-page merge helper (lopdf)
// ---------------------------------------------------------------------------

// @brief Merge a sequence of single-page PDF byte buffers into one multi-page PDF.
//
// Each element of `page_bytes` must be a valid single-page PDF produced by
// svg2pdf.  The first document is used as the base; subsequent documents have
// their objects renumbered to avoid ID conflicts, their single page is
// reparented into the base Pages tree, and their objects are transferred into
// the base document.
//
// @param page_bytes  Ordered vector of single-page PDF byte buffers.
// @return Ok(merged_bytes) on success; Err(message) on any failure.
pub(crate) fn merge_single_page_pdfs(page_bytes: Vec<Vec<u8>>) -> Result<Vec<u8>, String> {
    use lopdf::{Document, Object};

    if page_bytes.is_empty() {
        return Err("Tiled PDF: merge called with no pages.".into()); // if empty
    } // if empty

    if page_bytes.len() == 1 {
        // Single page — no merge required.
        return Ok(page_bytes.into_iter().next().unwrap()); // return single page bytes
    } // if single page

    // Load the first tile as the base document.
    let mut base = Document::load_mem(&page_bytes[0])
        .map_err(|e| format!("Tiled PDF: load tile 0 failed: {e}"))?; // if load failed

    // Locate the base document's Pages dictionary object ID.
    let base_pages_id = {
        let pages_ref = base
            .catalog()
            .map_err(|e| format!("Tiled PDF: base catalog error: {e}"))?
            .get(b"Pages")
            .map_err(|e| format!("Tiled PDF: base Pages entry missing: {e}"))?
            .clone(); // clone to release the borrow on base
        if let Object::Reference(id) = pages_ref {
            id // Pages dictionary object ID
        } else {
            return Err("Tiled PDF: base Pages entry is not a reference".into());
            // if not a reference
        } // if Pages ref
    }; // base_pages_id

    // Append each subsequent tile's single page into the base Pages tree.
    for (i, bytes) in page_bytes[1..].iter().enumerate() {
        let tile_num = i + 1;

        // Load and renumber the tile document to avoid object ID conflicts with base.
        let mut other = Document::load_mem(bytes)
            .map_err(|e| format!("Tiled PDF: load tile {tile_num} failed: {e}"))?; // if load failed
        other.renumber_objects_with(base.max_id + 1);

        // Find the single page object ID in this tile document.
        let other_page_id = *other
            .get_pages()
            .values()
            .next()
            .ok_or_else(|| format!("Tiled PDF: tile {tile_num} has no pages"))?; // if no pages

        // Reparent the tile's page to the base Pages dictionary.
        if let Ok(Object::Dictionary(ref mut dict)) = other.get_object_mut(other_page_id) {
            dict.set("Parent", Object::Reference(base_pages_id));
        } // if page dict found

        // Transfer all objects from the tile document into the base document.
        let other_max_id = other.max_id;
        for (id, obj) in other.objects {
            base.objects.insert(id, obj);
        } // for each object
          // Keep base.max_id current so the next renumber call starts after all transferred IDs.
        if other_max_id > base.max_id {
            base.max_id = other_max_id;
        } // if other max_id is larger

        // Append the new page reference to the base Pages/Kids array and increment Count.
        if let Ok(Object::Dictionary(ref mut pages_dict)) = base.get_object_mut(base_pages_id) {
            // Clone the current Kids array, append the new page, and write it back.
            let mut kids = pages_dict
                .get(b"Kids")
                .ok()
                .and_then(|o| {
                    if let Object::Array(arr) = o {
                        Some(arr.clone())
                    } else {
                        None
                    }
                })
                .unwrap_or_default(); // default to empty vec if Kids missing or wrong type
            kids.push(Object::Reference(other_page_id));

            let count = match pages_dict.get(b"Count").ok() {
                Some(Object::Integer(n)) => *n, // existing count
                _ => 1,                         // fallback: treat base as having 1 page
            }; // count

            pages_dict.set("Kids", Object::Array(kids));
            pages_dict.set("Count", Object::Integer(count + 1));
        } // if pages dict found
    } // for each additional tile

    // Serialize the merged multi-page document to bytes.
    let mut out: Vec<u8> = Vec::new();
    base.save_to(&mut out)
        .map_err(|e| format!("Tiled PDF: merge serialise failed: {e}"))?; // if write failed
    Ok(out)
} // fn merge_single_page_pdfs

// ---------------------------------------------------------------------------
// PNG export
// ---------------------------------------------------------------------------

// @brief Export a stripped layout document to a PNG file.
//
// Pipeline:
//   1. DOM → usvg::Tree via app_core::document_to_tree.   (progress → 10%)
//   2. usvg::Tree → PNG via resvg at fixed 100% scale.
//   3. Write PNG to disk.                                  (progress → 90%)
//
// @param doc      Cloned, piece-fill-stripped layout DOM.
// @param path     Destination file path.
// @param progress Callback invoked with integer percent (0–100) at each stage.
//                 The caller owns 0% (before call) and 100% (after Ok return).
// @return Ok(()) on success; Err(message) on any failure.
pub fn do_export_png(
    doc: &svg_dom::Document,
    path: &str,
    progress: &mut impl FnMut(i32),
) -> Result<(), String> {
    crate::log_to_file(&format!("[exports.rs] do_export_png(): 1 parsing SVG DOM to usvg tree for '{path}'"));

    // Stage 1 start: DOM → usvg tree (~10% of total work).
    progress(10);

    // Step 1: DOM → usvg tree.
    let tree = app_core::document_to_tree(doc, None)
        .map_err(|e| {
            crate::log_to_file(&format!("[exports.rs] do_export_png(): 2 SVG parse failed: {e}"));
            format!("PNG export failed — SVG parse error: {e}")
        })?; // if parse failed

    crate::log_to_file(&format!("[exports.rs] do_export_png(): 2 SVG parsed; rendering PNG to '{path}'"));

    // Step 2–3: usvg tree → PNG rasterisation → file write.
    let result = app_core::render_png(&tree, Path::new(path), 1.0)
        .map_err(|e| {
            crate::log_to_file(&format!("[exports.rs] do_export_png(): 3 render failed: {e}"));
            format!("PNG export failed — render error: {e}")
        }); // if render failed

    if result.is_ok() {
        // Stage 2 complete: PNG file written.
        progress(90);
        crate::log_to_file(&format!("[exports.rs] do_export_png(): 3 wrote PNG '{path}'"));
    } // if ok

    result
} // fn do_export_png

// ---------------------------------------------------------------------------
// JPEG export
// ---------------------------------------------------------------------------

// @brief Export a stripped layout document to a JPEG file.
//
// Same render as PNG export (100% scale, white background), then JPEG
// encoding at app_core::JPEG_DEFAULT_QUALITY.
//
// @param doc      Cloned, piece-fill-stripped layout DOM.
// @param path     Destination .jpg or .jpeg file path.
// @param progress Callback invoked with 10 (parse start) and 90 (file written).
//                 The caller owns 0% (before call) and 100% (after Ok return).
// @return Ok(()) on success; Err(message) on any failure, including a layout
//         larger than JPEG allows on one side.
pub fn do_export_jpeg(
    doc: &svg_dom::Document,
    path: &str,
    progress: &mut impl FnMut(i32),
) -> Result<(), String> {
    crate::log_to_file(&format!("[exports.rs] do_export_jpeg(): 1 parsing SVG DOM to usvg tree for '{path}'"));
    progress(10);

    // DOM → usvg tree.
    let tree = app_core::document_to_tree(doc, None).map_err(|e| {
        crate::log_to_file(&format!("[exports.rs] do_export_jpeg(): 2 SVG parse failed: {e}"));
        format!("JPG export failed — SVG parse error: {e}")
    })?; // if parse failed

    crate::log_to_file(&format!("[exports.rs] do_export_jpeg(): 2 SVG parsed; rendering JPEG to '{path}'"));

    // usvg tree → pixmap → JPEG file.
    app_core::render_jpeg(&tree, Path::new(path), 1.0, app_core::JPEG_DEFAULT_QUALITY).map_err(|e| {
        crate::log_to_file(&format!("[exports.rs] do_export_jpeg(): 3 render failed: {e}"));
        format!("JPG export failed — {e}")
    })?; // if render failed

    progress(90);
    crate::log_to_file(&format!("[exports.rs] do_export_jpeg(): 3 wrote JPEG '{path}'"));
    Ok(())
} // fn do_export_jpeg

// ---------------------------------------------------------------------------
// SVG export
// ---------------------------------------------------------------------------

// @brief Export the layout document to an SVG file, preserving it verbatim.
//
// Serializes the DOM directly to disk; no rasterisation or conversion.
// Because the document is written as-is, all styles, <g> groups, and id
// attributes on it are preserved in the output file. The caller
// (AppController::export_svg) passes a piece-fill/Rectangles-stripped clone,
// so this function's own output has neither.
//
// @param doc   Layout DOM to serialize, as-is.
// @param path  Destination file path.
// @return Ok(()) on success; Err(message) on any failure.
pub fn do_export_svg(
    doc: &svg_dom::Document,
    path: &str,
) -> Result<(), String> {

    crate::log_to_file(&format!("[exports.rs] do_export_svg(): 1 serializing layout DOM to '{path}'"));

    app_core::save_svg(doc, Path::new(path))
        .map(|_| crate::log_to_file(&format!("[exports.rs] do_export_svg(): 2 saved SVG '{path}'")))
        .map_err(|e| format!("SVG export failed: {e}")) // if save failed

} // fn do_export_svg

// ---------------------------------------------------------------------------
// HPGL export
// ---------------------------------------------------------------------------

// @brief Parse the HPGL options JSON sent by QML.
//
// Shape: {"mode": "plot" | "cut", "cutPen": 1, "markPen": 2, "labelPen": 3}.
// A missing pen takes its default; an unknown mode is an error, never a silent fallback.
// @param options_json JSON text from Main.qml.
// @return Parsed options; Err(message) for bad JSON, an unknown mode or a pen outside 1..=8.
pub fn parse_hpgl_options(options_json: &str) -> Result<HpglOptions, String> {
    #[derive(serde::Deserialize)]
    #[serde(rename_all = "camelCase")]
    struct HpglOptionsJson {
        mode: String,
        cut_pen: Option<u8>,
        mark_pen: Option<u8>,
        label_pen: Option<u8>,
    } // struct HpglOptionsJson

    let parsed: HpglOptionsJson = serde_json::from_str(options_json)
        .map_err(|e| format!("HPGL export: invalid options: {e}"))?;
    let mode = HpglMode::from_name(&parsed.mode)
        .ok_or_else(|| format!("HPGL export: unknown mode '{}'.", parsed.mode))?;

    // Missing pens keep the default pen map.
    let defaults = PenMap::default();
    let pens = PenMap {
        cut: parsed.cut_pen.unwrap_or(defaults.cut),
        mark: parsed.mark_pen.unwrap_or(defaults.mark),
        label: parsed.label_pen.unwrap_or(defaults.label),
    }; // pens
    pens.validate()?;
    Ok(HpglOptions { mode, pens })
} // fn parse_hpgl_options

// @brief Export the layout document as an HP-GL/1 plotter file.
//
// Plot mode first rewrites label text as single Hershey strokes, so a pen
// draws each letter in one pass instead of tracing glyph outlines. Cut mode
// drops labels, so it skips that step.
//
// @param doc     Cloned, piece-fill-stripped layout DOM.
// @param path    Destination .plt or .hpgl file path.
// @param options Mode and pen map.
// @return Label-text warnings (for the success dialog) on success; Err(message) on failure.
pub fn do_export_hpgl(
    doc: &svg_dom::Document,
    path: &str,
    options: &HpglOptions,
) -> Result<Vec<String>, String> {
    crate::log_to_file(&format!("[exports.rs] do_export_hpgl(): 1 mode={:?} pens={:?} path='{path}'", options.mode, options.pens));

    let mut plot_doc = doc.clone();
    let warnings = match options.mode {
        HpglMode::Plot => {
            // Labels are plotted: turn their text into single strokes.
            svg_label_text::apply_text_mode(&mut plot_doc, svg_label_text::SvgTextMode::HersheyStrokes).warnings
        } // Plot
        HpglMode::Cut => Vec::new(), // labels are not written
    }; // match mode

    let program = hpgl_writer::svg_to_hpgl(&plot_doc, options).map_err(|e| {
        crate::log_to_file(&format!("[exports.rs] do_export_hpgl(): 2 conversion failed: {e}"));
        e
    })?; // if conversion failed

    std::fs::write(path, program.as_bytes()).map_err(|e| {
        crate::log_to_file(&format!("[exports.rs] do_export_hpgl(): 3 write failed: {e}"));
        format!("HPGL export: write failed: {e}")
    })?; // if write failed

    crate::log_to_file(&format!("[exports.rs] do_export_hpgl(): 3 wrote {} bytes to '{path}'", program.len()));
    Ok(warnings)
} // fn do_export_hpgl

// ---------------------------------------------------------------------------
// PostScript / EPS export
// ---------------------------------------------------------------------------

// @brief Export the layout document as a PostScript (.ps) or EPS (.eps) file.
//
// Label text is written as glyph outlines of the fonts the SVG names, so the
// file needs no fonts where it is printed or placed.
//
// @param doc    Cloned, piece-fill-stripped layout DOM.
// @param path   Destination .ps or .eps file path.
// @param flavor PsFlavor::Ps for a printer document, PsFlavor::Eps for a placeable graphic.
// @return Caveats for the success dialog (for example, transparency written opaque); Err(message) on failure.
pub fn do_export_postscript(
    doc: &svg_dom::Document,
    path: &str,
    flavor: PsFlavor,
) -> Result<Vec<String>, String> {
    crate::log_to_file(&format!("[exports.rs] do_export_postscript(): 1 flavor={flavor:?} path='{path}'"));

    let output = ps_writer::svg_to_postscript(doc, flavor).map_err(|e| {
        crate::log_to_file(&format!("[exports.rs] do_export_postscript(): 2 conversion failed: {e}"));
        e
    })?; // if conversion failed

    std::fs::write(path, output.program.as_bytes()).map_err(|e| {
        crate::log_to_file(&format!("[exports.rs] do_export_postscript(): 3 write failed: {e}"));
        format!("{} export: write failed: {e}", flavor.label())
    })?; // if write failed

    crate::log_to_file(&format!(
        "[exports.rs] do_export_postscript(): 3 wrote {} bytes to '{path}'",
        output.program.len()
    ));
    Ok(output.warnings)
} // fn do_export_postscript

// ---------------------------------------------------------------------------
// Paid export modules (G-Code, 3D mesh)
// ---------------------------------------------------------------------------

// @brief Format key for G-Code export.
pub const PAID_EXPORT_GCODE: &str = "gcode";

// @brief Format key for 3D mesh export, written as a .3mf file.
pub const PAID_EXPORT_MESH: &str = "3mf";

// @brief Report whether a paid export module is installed and usable.
//
// The Export menu shows a paid format only when this returns true.
// No paid module exists yet, so every format returns false.
// Replace the body with the module or license check when a module ships.
//
// @param format PAID_EXPORT_GCODE or PAID_EXPORT_MESH; any other key returns false.
pub fn paid_export_available(format: &str) -> bool {
    match format {
        PAID_EXPORT_GCODE | PAID_EXPORT_MESH => false, // no module installed
        _ => false,                                    // unknown format
    } // match format
} // fn paid_export_available

// @brief Export the layout document as G-Code (stub).
//
// Writes no file. Returns Err until a G-Code module is available.
//
// @param doc  Cloned, stripped layout DOM.
// @param path Destination .gcode file path.
// @return Err(message) while no G-Code module is available.
pub fn do_export_gcode(
    _doc: &svg_dom::Document,
    path: &str,
) -> Result<(), String> {
    crate::log_to_file(&format!("[exports.rs] do_export_gcode(): 1 requested '{path}'"));

    // Guard: the menu hides this format, so a call here means a caller skipped the gate.
    if !paid_export_available(PAID_EXPORT_GCODE) {
        crate::log_to_file("[exports.rs] do_export_gcode(): 2 module not available");
        return Err("G-Code export is not available.".to_string());
    } // if not available

    Err("G-Code export is not implemented.".to_string())
} // fn do_export_gcode

// @brief Export the layout document as a 3D mesh in 3MF format (stub).
//
// Writes no file. Returns Err until a 3D mesh module is available.
//
// @param doc  Cloned, stripped layout DOM.
// @param path Destination .3mf file path.
// @return Err(message) while no 3D mesh module is available.
pub fn do_export_mesh(
    _doc: &svg_dom::Document,
    path: &str,
) -> Result<(), String> {
    crate::log_to_file(&format!("[exports.rs] do_export_mesh(): 1 requested '{path}'"));

    // Guard: the menu hides this format, so a call here means a caller skipped the gate.
    if !paid_export_available(PAID_EXPORT_MESH) {
        crate::log_to_file("[exports.rs] do_export_mesh(): 2 module not available");
        return Err("3D mesh export is not available.".to_string());
    } // if not available

    Err("3D mesh export is not implemented.".to_string())
} // fn do_export_mesh

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use svg_dom::Document;

    // @brief Build a minimal tiled-export settings object for tests.
    fn test_tiled_settings() -> LayoutSettings {
        LayoutSettings::from_json(
            r#"{
                "unit": "in",
                "mediaType": "paper",
                "paperType": "tiled",
                "pageWidth": 20.0,
                "pageHeight": 20.0,
                "marginLeft": 0.25,
                "marginRight": 0.25,
                "marginTop": 0.25,
                "marginBottom": 0.25,
                "pieceGap": 0.0,
                "layoutMode": "alongGrainline",
                "rotationStep": 180,
                "tileSize": "Letter",
                "tileOrientation": "Portrait"
            }"#,
        )
        .expect("test tiled settings should parse")
    } // fn test_tiled_settings

    // @brief Tile pages add explicit clipping so off-page labels cannot leak through.
    #[test]
    fn build_tiled_pdf_tile_doc_adds_explicit_clip() {
        let doc = Document::parse(
            r#"<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="1600">
                <g id="piece0">
                    <rect x="0" y="0" width="300" height="300"/>
                    <text id="offpageLabel" x="1000" y="1400">label</text>
                </g>
            </svg>"#,
        )
        .expect("fixture SVG should parse");
        let settings = test_tiled_settings();
        let tile_dims =
            compute_tile_dims(1200, 1600, &settings).expect("tile dimensions should compute");

        let tile_doc = build_tiled_pdf_tile_doc(&doc, &tile_dims, 0, 0);
        let tile_svg = tile_doc.to_string();

        assert!(
            tile_svg.contains("id=\"tileViewport_r0_c0\""),
            "tile viewport should be present"
        );
        assert!(
            tile_svg.contains("overflow=\"hidden\""),
            "tile viewport should explicitly hide overflow"
        );
        assert!(
            tile_svg.contains("id=\"tileClipRect_r0_c0\""),
            "tile clip path should be defined"
        );
        assert!(
            tile_svg.contains("clip-path=\"url(#tileClipRect_r0_c0)\""),
            "layout content should be clipped to the tile rect"
        );
    } // build_tiled_pdf_tile_doc_adds_explicit_clip

    // @brief do_export_svg must preserve the DOM it is given verbatim — styles,
    // <g> groups, id names, and child geometry survive the round trip. Any
    // stripping (piece-fill overlays, the Rectangles group) is the caller's
    // job, done on the clone before it reaches this function.
    #[test]
    fn svg_export_preserves_styles_groups_and_ids() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="100" height="100"><g id="piece1" class="cut"><path d="M 0 0 L 10 0 L 10 10 L 0 10 Z" style="fill:#aabbcc;stroke:#000000"/></g></svg>"#;
        let doc = svg_dom::Document::parse(svg).expect("parse input svg");

        // Unique temp path so parallel test runs don't collide.
        let mut path = std::env::temp_dir();
        path.push(format!("seamly_svg_export_test_{}.svg", std::process::id()));
        let path_str = path.to_string_lossy().to_string();

        do_export_svg(&doc, &path_str).expect("svg export ok");

        let written = std::fs::read_to_string(&path_str).expect("read exported svg back");
        let _ = std::fs::remove_file(&path_str); // best-effort cleanup

        let out = svg_dom::Document::parse(&written).expect("re-parse exported svg");

        // Group with its id and class attributes survives.
        let group = out
            .root
            .children
            .iter()
            .filter_map(|n| n.as_element())
            .find(|e| e.name == "g" && e.attributes.get("id").map(String::as_str) == Some("piece1"))
            .expect("group id='piece1' preserved");
        assert_eq!(group.attributes.get("class").map(String::as_str), Some("cut"));

        // The child <path> and its inline style survive.
        let path_el = group
            .children
            .iter()
            .filter_map(|n| n.as_element())
            .find(|e| e.name == "path")
            .expect("child path preserved");
        let style = path_el.attributes.get("style").map(String::as_str).unwrap_or("");
        assert!(style.contains("fill:#aabbcc"), "fill style preserved, got '{style}'");
        assert!(style.contains("stroke:#000000"), "stroke style preserved, got '{style}'");
        assert!(path_el.attributes.contains_key("d"), "path geometry preserved");
    } // svg_export_preserves_styles_groups_and_ids

    // @brief do_export_dxf writes a DXF file containing expected DXF markers (AC1009/SECTION/EOF).
    //
    // Creates a minimal SVG DOM (one stroked rectangle), exports it via the full
    // do_export_dxf pipeline (DOM → ezdxf Drawing → DXF-ASTM → disk), and asserts
    // that the written file contains the DXF R12 version identifier "AC1009", a
    // SECTION marker anywhere in the file, and ends with the EOF marker.
    #[test]
    fn do_export_dxf_writes_valid_dxf() {
        let svg = r##"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
            <g id="piece0">
                <rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000" stroke-width="1"/>
            </g>
        </svg>"##;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // Unique temp path so parallel test runs don't collide.
        let mut path = std::env::temp_dir();
        path.push(format!("seamly_dxf_export_test_{}.dxf", std::process::id()));
        let path_str = path.to_string_lossy().to_string();

        do_export_dxf(&doc, &path_str, false, &DxfStyleInfo::default(), &mut |_| {}).expect("DXF export should succeed");

        let content = std::fs::read_to_string(&path_str).expect("exported DXF file should be readable");
        let _ = std::fs::remove_file(&path_str); // best-effort cleanup

        // DXF R12 files contain the AC1009 version identifier in the HEADER section.
        assert!(
            content.contains("AC1009"),
            "exported DXF should contain AC1009 version identifier, got first 200 chars: {:?}",
            content.get(..200).unwrap_or(&content)
        );
        // DXF files contain SECTION markers.
        assert!(
            content.contains("SECTION"),
            "exported DXF should contain SECTION marker"
        );
        // DXF files end with the EOF marker.
        assert!(
            content.trim_end().ends_with("EOF"),
            "exported DXF should end with EOF marker"
        );
    } // do_export_dxf_writes_valid_dxf

    // @brief do_export_dxf with dxf_version R13 writes an AC1012 file with handles and OBJECTS.
    #[test]
    fn do_export_dxf_writes_r13_when_asked() {
        let svg = r##"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
            <g id="piece0">
                <rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000" stroke-width="1"/>
            </g>
        </svg>"##;
        let doc = Document::parse(svg).expect("fixture SVG should parse");
        let mut path = std::env::temp_dir();
        path.push(format!("seamly_dxf_r13_test_{}.dxf", std::process::id()));
        let path_str = path.to_string_lossy().to_string();

        let style = DxfStyleInfo { dxf_version: DxfVersion::R13, ..DxfStyleInfo::default() };
        do_export_dxf(&doc, &path_str, false, &style, &mut |_| {}).expect("R13 DXF export should succeed");

        let content = std::fs::read_to_string(&path_str).expect("exported DXF file should be readable");
        let _ = std::fs::remove_file(&path_str);
        assert!(content.contains("AC1012"), "R13 file names AC1012");
        assert!(!content.contains("AC1009"), "R13 file does not name AC1009");
        assert!(content.contains("$HANDSEED"), "R13 file has a handle seed");
        assert!(content.contains("OBJECTS"), "R13 file has an OBJECTS section");
    } // do_export_dxf_writes_r13_when_asked

    // @brief Export a one-piece SVG to a temp file named `<stem>_<pid>.dxf` and return the DXF text.
    // @param pattern_name `data-name` of a `data-type="pattern"` wrapper; None writes no wrapper.
    fn export_style_name_fixture(stem: &str, pattern_name: Option<&str>, input_name: Option<&str>) -> String {
        let rect = r##"<g id="piece0" data-type="piece"><rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000"/></g>"##;
        // Wrap the piece in a named pattern group when the case needs one.
        let body = match pattern_name {
            Some(n) => format!(r#"<g data-type="pattern" data-name="{n}">{rect}</g>"#),
            None => rect.to_string(),
        }; // body
        let svg = format!(r#"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">{body}</svg>"#);
        let doc = Document::parse(&svg).expect("fixture SVG should parse");
        let mut path = std::env::temp_dir();
        path.push(format!("{stem}_{}.dxf", std::process::id()));
        let path_str = path.to_string_lossy().to_string();

        let style = DxfStyleInfo { input_name: input_name.map(str::to_string), ..DxfStyleInfo::default() };
        do_export_dxf(&doc, &path_str, false, &style, &mut |_| {}).expect("DXF export should succeed");
        let content = std::fs::read_to_string(&path_str).expect("exported DXF file should be readable");
        let _ = std::fs::remove_file(&path_str);
        content
    } // fn export_style_name_fixture

    // @brief Value of the `Style Name:` text line in a DXF file.
    fn style_name_line(content: &str) -> String {
        content
            .lines()
            .find_map(|l| l.trim_end().strip_prefix("Style Name:"))
            .expect("DXF has a Style Name line")
            .to_string()
    } // fn style_name_line

    // @brief A file import's base name is the Style Name, ahead of the pattern name and output stem.
    #[test]
    fn dxf_style_name_is_imported_file_base_name() {
        let content = export_style_name_fixture("male_shirt_202610031234", Some("Men's Shirt"), Some("male_shirt"));
        assert_eq!(style_name_line(&content), "male_shirt");
    } // dxf_style_name_is_imported_file_base_name

    // @brief The `--document-name` value is the Style Name of a standard input handoff.
    #[test]
    fn dxf_style_name_is_document_name() {
        let content = export_style_name_fixture("seamly_dxf_style_handoff", None, Some("richmond-shirt_v1"));
        assert_eq!(style_name_line(&content), "richmond-shirt_v1");
    } // dxf_style_name_is_document_name

    // @brief No input name, or a blank one, keeps the output file stem.
    #[test]
    fn dxf_style_name_without_input_name_is_output_stem() {
        let pid = std::process::id();
        for input_name in [None, Some(""), Some("  ")] {
            let content = export_style_name_fixture("seamly_dxf_style_stem", None, input_name);
            assert_eq!(style_name_line(&content), format!("seamly_dxf_style_stem_{pid}"), "input_name={input_name:?}");
        } // for each absent input name
    } // dxf_style_name_without_input_name_is_output_stem

    // @brief Without an input name, the pattern `data-name` still wins over the output stem.
    #[test]
    fn dxf_style_name_without_input_name_keeps_pattern_name() {
        let content = export_style_name_fixture("seamly_dxf_style_pattern", Some("Men's Shirt"), None);
        assert_eq!(style_name_line(&content), "Men's Shirt");
    } // dxf_style_name_without_input_name_keeps_pattern_name

    // @brief The male shirt handoff names its three interface pieces, and only those.
    #[test]
    fn dxf_missing_piece_data_names_male_shirt_interface_pieces() {
        let svg = std::fs::read_to_string(concat!(env!("CARGO_MANIFEST_DIR"), "/../ezdxf2dxfastm/test_data/male_shirt_pieces.svg"))
            .expect("fixture present");
        let doc = Document::parse(&svg).expect("fixture parses");
        let text = dxf_missing_piece_data_message(&doc).expect("converts");

        let pieces: Vec<&str> =
            text.lines().filter_map(|l| l.strip_prefix("\u{2022} ")).map(|l| l.split(':').next().unwrap()).collect();
        assert_eq!(pieces, vec!["CollarBaseInterface", "CollarTopInterface", "CuffInterface"]);
        assert!(text.contains("CuffInterface: no Quantity (no \"Cut N\" label line), no piece label, no grainline"), "{text}");
        assert!(text.contains("usability of the exported DXF file"), "{text}");
    } // dxf_missing_piece_data_names_male_shirt_interface_pieces

    // @brief No incomplete pieces gives no warning text.
    #[test]
    fn missing_piece_data_text_is_empty_when_complete() {
        assert_eq!(missing_piece_data_text(&[]), "");
        let only_grain = MissingPieceData { piece_name: "Yoke".into(), quantity: false, label: false, grainline: true };
        assert!(missing_piece_data_text(&[only_grain]).contains("\u{2022} Yoke: no grainline\n"));
    } // missing_piece_data_text_is_empty_when_complete

    // @brief do_export_dxf returns Err when the path is not writable.
    //
    // Passes a path whose parent directory does not exist so the OS rejects the
    // write. Asserts that do_export_dxf propagates a descriptive error string.
    #[test]
    fn do_export_dxf_returns_err_on_bad_path() {
        // Use a minimal non-empty piece group so failures are attributable to
        // the unwritable path, not empty-SVG handling in the converter.
        let svg = r##"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
            <g id="piece0">
                <rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000" stroke-width="1"/>
            </g>
        </svg>"##;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // A path whose parent directory does not exist cannot be written.
        // Double-nested unique path guarantees the parent never exists, even if the
        // outer fixed directory was created by a prior run.
        let unique_dir = format!(
            "nonexistent_dir_seamly_{}_{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_nanos()
        );
        let _ = std::fs::remove_dir_all(std::env::temp_dir().join(&unique_dir));
        let bad_path = std::env::temp_dir()
            .join("nonexistent_dir_seamly")
            .join(unique_dir)
            .join("test.dxf")
            .to_string_lossy()
            .to_string();

        let result = do_export_dxf(&doc, &bad_path, false, &DxfStyleInfo::default(), &mut |_| {});
        assert!(result.is_err(), "do_export_dxf should return Err for unwritable path");
        let msg = result.unwrap_err();
        assert!(
            msg.contains("DXF export failed"),
            "error message should contain 'DXF export failed', got: {msg}"
        );
    } // do_export_dxf_returns_err_on_bad_path

    // @brief do_export_dxf with teaching version creates a .txt file alongside the .dxf.
    //
    // Exports a minimal layout DOM with create_teaching_version=true and verifies
    // that both the .dxf file and a .txt teaching version exist and contain the
    // expected DXF-ASTM teaching header comment.
    #[test]
    fn do_export_dxf_with_teaching_version_creates_txt() {
        let svg = r##"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
            <g id="frontPiece">
                <rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000" stroke-width="1"/>
            </g>
        </svg>"##;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // Unique temp path so parallel test runs don't collide.
        let mut dxf_path = std::env::temp_dir();
        dxf_path.push(format!("seamly_dxf_teaching_test_{}.dxf", std::process::id()));
        let dxf_path_str = dxf_path.to_string_lossy().to_string();

        do_export_dxf(&doc, &dxf_path_str, true, &DxfStyleInfo::default(), &mut |_| {}).expect("DXF export with teaching version should succeed");

        // DXF file must exist.
        assert!(dxf_path.exists(), "DXF file should exist at '{dxf_path_str}'");

        // Teaching version (.txt) must exist alongside the .dxf file.
        let mut txt_path = dxf_path.clone();
        txt_path.set_extension("txt");
        assert!(
            txt_path.exists(),
            "teaching version .txt should exist at '{}'",
            txt_path.display()
        );

        // Teaching version must contain the teaching header comment.
        let txt_content = std::fs::read_to_string(&txt_path)
            .expect("teaching version .txt should be readable");
        assert!(
            txt_content.contains("DXF-ASTM Teaching Version"),
            "teaching version should contain header comment, got first 200 chars: {:?}",
            txt_content.get(..200).unwrap_or(&txt_content)
        );

        // Cleanup.
        let _ = std::fs::remove_file(&dxf_path);
        let _ = std::fs::remove_file(&txt_path);
    } // do_export_dxf_with_teaching_version_creates_txt

    // @brief do_export_dxf fires the progress callback with values 10, 50, and 90.
    //
    // The caller (lib.rs export_dxf) owns the 0% and 100% ticks; this test
    // confirms do_export_dxf emits the three intermediate checkpoints in order
    // and that each value falls within the expected range (1–99).
    #[test]
    fn do_export_dxf_emits_intermediate_progress() {
        let svg = r##"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
            <g id="piece0">
                <rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000" stroke-width="1"/>
            </g>
        </svg>"##;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // Unique temp path so parallel test runs don't collide.
        let mut path = std::env::temp_dir();
        path.push(format!("seamly_dxf_progress_test_{}.dxf", std::process::id()));
        let path_str = path.to_string_lossy().to_string();

        // Collect every progress tick emitted during the export.
        let mut ticks: Vec<i32> = Vec::new();
        do_export_dxf(&doc, &path_str, false, &DxfStyleInfo::default(), &mut |pct| ticks.push(pct))
            .expect("DXF export should succeed");
        let _ = std::fs::remove_file(&path_str); // best-effort cleanup

        // Three intermediate ticks must be emitted: 10, 50, 90.
        assert_eq!(
            ticks.len(),
            3,
            "expected exactly 3 progress ticks, got {}: {:?}",
            ticks.len(),
            ticks
        );
        // Values must be in increasing order and within the caller-owned 0–100 range.
        for &pct in &ticks {
            assert!(
                (1..=99).contains(&pct),
                "intermediate progress {pct} must be between 1 and 99 (caller owns 0% and 100%)"
            );
        } // for each tick
        let sorted = {
            let mut s = ticks.clone();
            s.sort_unstable();
            s
        };
        assert_eq!(
            ticks, sorted,
            "progress ticks must be emitted in non-decreasing order: {:?}",
            ticks
        );
        // Pinned values: 10% after entry, 50% after SVG conversion, 90% after DXF write.
        assert_eq!(ticks[0], 10, "first tick must be 10% (SVG→ezdxf start)");
        assert_eq!(ticks[1], 50, "second tick must be 50% (SVG→ezdxf done / DXF write start)");
        assert_eq!(ticks[2], 90, "third tick must be 90% (DXF file written)");
    } // do_export_dxf_emits_intermediate_progress

    // @brief do_export_pdf produces a file that begins with the PDF magic bytes.
    //
    // Creates a minimal SVG DOM (one filled rectangle), exports it to a temp file
    // via the full do_export_pdf pipeline (DOM → usvg tree → svg2pdf → disk),
    // and asserts that the written file starts with the `%PDF-` signature.
    // This pins the complete single-page PDF render path end-to-end.
    #[test]
    fn do_export_pdf_writes_valid_pdf() {
        let svg = r##"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
            <rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000" stroke-width="1"/>
        </svg>"##;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // Unique temp path so parallel test runs don't collide.
        let mut path = std::env::temp_dir();
        path.push(format!("seamly_pdf_export_test_{}.pdf", std::process::id()));
        let path_str = path.to_string_lossy().to_string();

        do_export_pdf(&doc, &path_str, &mut |_| {}).expect("PDF export should succeed");

        let bytes = std::fs::read(&path_str).expect("exported PDF file should be readable");
        let _ = std::fs::remove_file(&path_str); // best-effort cleanup

        // PDF files begin with the %PDF- magic signature.
        assert!(
            bytes.starts_with(b"%PDF-"),
            "exported file should start with PDF magic bytes, got {:?}",
            &bytes[..bytes.len().min(8)]
        );
    } // do_export_pdf_writes_valid_pdf

    // @brief do_export_pdf returns Err when the path is not writable.
    //
    // Passes a directory path (not a file path) so the OS will reject the write.
    // Asserts that do_export_pdf propagates a descriptive error string.
    #[test]
    fn do_export_pdf_returns_err_on_bad_path() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10"></svg>"#;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // A path whose parent directory does not exist cannot be written.
        // Double-nested unique path guarantees the parent never exists, even if the
        // outer fixed directory was created by a prior run.
        let unique_dir = format!(
            "nonexistent_dir_seamly_{}_{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_nanos()
        );
        let _ = std::fs::remove_dir_all(std::env::temp_dir().join(&unique_dir));
        let bad_path = std::env::temp_dir()
            .join("nonexistent_dir_seamly")
            .join(unique_dir)
            .join("test.pdf")
            .to_string_lossy()
            .to_string();

        let result = do_export_pdf(&doc, &bad_path, &mut |_| {});
        assert!(result.is_err(), "do_export_pdf should return Err for unwritable path");
        let msg = result.unwrap_err();
        assert!(
            msg.contains("PDF export failed"),
            "error message should contain 'PDF export failed', got: {msg}"
        );
    } // do_export_pdf_returns_err_on_bad_path

    // @brief do_export_pdf fires the progress callback with values 10 and 90.
    //
    // The caller (lib.rs export_pdf) owns the 0% and 100% ticks; this test
    // confirms do_export_pdf emits the two intermediate checkpoints in order
    // and that each value falls within the expected range (1–99).
    #[test]
    fn do_export_pdf_emits_intermediate_progress() {
        let svg = r##"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
            <rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000" stroke-width="1"/>
        </svg>"##;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // Unique temp path so parallel test runs don't collide.
        let mut path = std::env::temp_dir();
        path.push(format!("seamly_pdf_progress_test_{}.pdf", std::process::id()));
        let path_str = path.to_string_lossy().to_string();

        // Collect every progress tick emitted during the export.
        let mut ticks: Vec<i32> = Vec::new();
        do_export_pdf(&doc, &path_str, &mut |pct| ticks.push(pct))
            .expect("PDF export should succeed");
        let _ = std::fs::remove_file(&path_str); // best-effort cleanup

        // Two intermediate ticks must be emitted: 10 (SVG parse start), 90 (PDF written).
        assert_eq!(
            ticks.len(),
            2,
            "expected exactly 2 progress ticks, got {}: {:?}",
            ticks.len(),
            ticks
        );
        // Values must be in increasing order and within the caller-owned 0–100 range.
        for &pct in &ticks {
            assert!(
                (1..=99).contains(&pct),
                "intermediate progress {pct} must be between 1 and 99 (caller owns 0% and 100%)"
            );
        } // for each tick
        let sorted = {
            let mut s = ticks.clone();
            s.sort_unstable();
            s
        };
        assert_eq!(
            ticks, sorted,
            "progress ticks must be emitted in non-decreasing order: {:?}",
            ticks
        );
        // Pinned values: 10% at SVG parse start, 90% after PDF file written.
        assert_eq!(ticks[0], 10, "first tick must be 10% (SVG parse start)");
        assert_eq!(ticks[1], 90, "second tick must be 90% (PDF file written)");
    } // do_export_pdf_emits_intermediate_progress

    // @brief do_export_png produces a file that begins with the PNG magic bytes.
    //
    // Creates a minimal SVG DOM (one stroked rectangle), exports it via the full
    // do_export_png pipeline (DOM → usvg tree → resvg → PNG → disk), and asserts
    // that the written file starts with the 8-byte PNG signature.
    // This pins the complete PNG render path end-to-end.
    #[test]
    fn do_export_png_writes_valid_png() {
        let svg = r##"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
            <rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000" stroke-width="1"/>
        </svg>"##;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // Unique temp path so parallel test runs don't collide.
        let mut path = std::env::temp_dir();
        path.push(format!("seamly_png_export_test_{}.png", std::process::id()));
        let path_str = path.to_string_lossy().to_string();

        do_export_png(&doc, &path_str, &mut |_| {}).expect("PNG export should succeed");

        let bytes = std::fs::read(&path_str).expect("exported PNG file should be readable");
        let _ = std::fs::remove_file(&path_str); // best-effort cleanup

        // PNG files begin with the 8-byte signature: 0x89 P N G \r \n 0x1A \n
        assert!(
            bytes.starts_with(&[0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]),
            "exported file should start with PNG magic bytes, got {:?}",
            &bytes[..bytes.len().min(8)]
        );
    } // do_export_png_writes_valid_png

    // @brief do_export_png returns Err when the path is not writable.
    //
    // Passes a path whose parent directory does not exist so the OS rejects the
    // write. Asserts that do_export_png propagates a descriptive error string.
    #[test]
    fn do_export_png_returns_err_on_bad_path() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10"></svg>"#;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // A path whose parent directory does not exist cannot be written.
        // Double-nested unique path guarantees the parent never exists, even if the
        // outer fixed directory was created by a prior run.
        let unique_dir = format!(
            "nonexistent_dir_seamly_{}_{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap_or_default()
                .as_nanos()
        );
        let _ = std::fs::remove_dir_all(std::env::temp_dir().join(&unique_dir));
        let bad_path = std::env::temp_dir()
            .join("nonexistent_dir_seamly")
            .join(unique_dir)
            .join("test.png")
            .to_string_lossy()
            .to_string();

        let result = do_export_png(&doc, &bad_path, &mut |_| {});
        assert!(result.is_err(), "do_export_png should return Err for unwritable path");
        let msg = result.unwrap_err();
        assert!(
            msg.contains("PNG export failed"),
            "error message should contain 'PNG export failed', got: {msg}"
        );
    } // do_export_png_returns_err_on_bad_path

    // @brief do_export_png fires the progress callback with values 10 and 90.
    //
    // The caller (lib.rs export_png) owns the 0% and 100% ticks; this test
    // confirms do_export_png emits the two intermediate checkpoints in order
    // and that each value falls within the expected range (1–99).
    #[test]
    fn do_export_png_emits_intermediate_progress() {
        let svg = r##"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
            <rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000" stroke-width="1"/>
        </svg>"##;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // Unique temp path so parallel test runs don't collide.
        let mut path = std::env::temp_dir();
        path.push(format!("seamly_png_progress_test_{}.png", std::process::id()));
        let path_str = path.to_string_lossy().to_string();

        // Collect every progress tick emitted during the export.
        let mut ticks: Vec<i32> = Vec::new();
        do_export_png(&doc, &path_str, &mut |pct| ticks.push(pct))
            .expect("PNG export should succeed");
        let _ = std::fs::remove_file(&path_str); // best-effort cleanup

        // Two intermediate ticks must be emitted: 10 (SVG parse start), 90 (PNG written).
        assert_eq!(
            ticks.len(),
            2,
            "expected exactly 2 progress ticks, got {}: {:?}",
            ticks.len(),
            ticks
        );
        // Values must be in increasing order and within the caller-owned 0–100 range.
        for &pct in &ticks {
            assert!(
                (1..=99).contains(&pct),
                "intermediate progress {pct} must be between 1 and 99 (caller owns 0% and 100%)"
            );
        } // for each tick
        let sorted = {
            let mut s = ticks.clone();
            s.sort_unstable();
            s
        };
        assert_eq!(
            ticks, sorted,
            "progress ticks must be emitted in non-decreasing order: {:?}",
            ticks
        );
        // Pinned values: 10% at SVG parse start, 90% after PNG file written.
        assert_eq!(ticks[0], 10, "first tick must be 10% (SVG parse start)");
        assert_eq!(ticks[1], 90, "second tick must be 90% (PNG file written)");
    } // do_export_png_emits_intermediate_progress

    // @brief do_export_jpeg writes a JPEG file and emits the 10 and 90 progress ticks.
    #[test]
    fn do_export_jpeg_writes_valid_jpeg() {
        let svg = r##"<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
            <rect x="10" y="10" width="180" height="180" fill="none" stroke="#000000" stroke-width="1"/>
        </svg>"##;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // Unique temp path so parallel test runs don't collide.
        let path_str = std::env::temp_dir()
            .join(format!("seamly_jpeg_export_test_{}.jpg", std::process::id()))
            .to_string_lossy()
            .to_string();

        let mut ticks: Vec<i32> = Vec::new();
        do_export_jpeg(&doc, &path_str, &mut |pct| ticks.push(pct)).expect("JPG export should succeed");

        let bytes = std::fs::read(&path_str).expect("exported JPEG file should be readable");
        let _ = std::fs::remove_file(&path_str); // best-effort cleanup

        // JPEG files begin with the SOI marker FF D8 followed by another marker FF.
        assert!(
            bytes.starts_with(&[0xFF, 0xD8, 0xFF]),
            "exported file should start with JPEG SOI marker, got {:?}",
            &bytes[..bytes.len().min(3)]
        );
        assert_eq!(ticks, vec![10, 90], "progress ticks must be 10 then 90");
    } // do_export_jpeg_writes_valid_jpeg

    // @brief do_export_jpeg returns a 'JPG export failed' error for an unwritable path.
    #[test]
    fn do_export_jpeg_returns_err_on_bad_path() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10"></svg>"#;
        let doc = Document::parse(svg).expect("fixture SVG should parse");

        // A path whose parent directory does not exist cannot be written.
        let bad_path = std::env::temp_dir()
            .join(format!("nonexistent_dir_seamly_jpeg_{}", std::process::id()))
            .join("missing")
            .join("test.jpg")
            .to_string_lossy()
            .to_string();

        let msg = do_export_jpeg(&doc, &bad_path, &mut |_| {}).expect_err("unwritable path must fail");
        assert!(msg.contains("JPG export failed"), "got: {msg}");
    } // do_export_jpeg_returns_err_on_bad_path

    // @brief do_export_jpeg reports the JPEG size limit instead of writing a file.
    #[test]
    fn do_export_jpeg_returns_err_for_oversized_layout() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg" width="70000" height="10"></svg>"#;
        let doc = Document::parse(svg).expect("fixture SVG should parse");
        let path_str = std::env::temp_dir()
            .join(format!("seamly_jpeg_too_wide_{}.jpg", std::process::id()))
            .to_string_lossy()
            .to_string();

        let msg = do_export_jpeg(&doc, &path_str, &mut |_| {}).expect_err("oversized layout must fail");
        assert!(msg.contains("65535"), "error should name the JPEG limit, got: {msg}");
        assert!(!Path::new(&path_str).exists(), "no file may be written");
    } // do_export_jpeg_returns_err_for_oversized_layout

    // @brief PS and EPS exports write their DSC headers to disk.
    #[test]
    fn do_export_postscript_writes_ps_and_eps_files() {
        for (flavor, file, header) in [
            (PsFlavor::Ps, "seamlylayout_ps_test.ps", "%!PS-Adobe-3.0\n"),
            (PsFlavor::Eps, "seamlylayout_ps_test.eps", "%!PS-Adobe-3.0 EPSF-3.0\n"),
        ] {
            let path = std::env::temp_dir().join(file);
            let _ = std::fs::remove_file(&path);
            let warnings = do_export_postscript(&hpgl_test_doc(), path.to_str().unwrap(), flavor)
                .expect("export should succeed");
            assert!(warnings.is_empty(), "{warnings:?}");
            let text = std::fs::read_to_string(&path).expect("file should exist");
            assert!(text.starts_with(header), "{text}");
            let _ = std::fs::remove_file(&path);
        } // for flavor
    } // do_export_postscript_writes_ps_and_eps_files

    // @brief An empty layout fails and writes no file.
    #[test]
    fn do_export_postscript_empty_layout_writes_nothing() {
        let path = std::env::temp_dir().join("seamlylayout_ps_empty_test.eps");
        let _ = std::fs::remove_file(&path);
        let doc = Document::parse(r#"<svg xmlns="http://www.w3.org/2000/svg" width="96" height="96"/>"#).unwrap();
        assert!(do_export_postscript(&doc, path.to_str().unwrap(), PsFlavor::Eps).is_err());
        assert!(!path.exists());
    } // do_export_postscript_empty_layout_writes_nothing

    // @brief No paid export module ships, so the gate hides both formats.
    #[test]
    fn paid_export_available_is_false_for_all_formats() {
        assert!(!paid_export_available(PAID_EXPORT_GCODE));
        assert!(!paid_export_available(PAID_EXPORT_MESH));
        assert!(!paid_export_available("unknown"));
    } // paid_export_available_is_false_for_all_formats

    // @brief A two-line layout: one cut line and one seam line on a 1-inch page.
    fn hpgl_test_doc() -> Document {
        Document::parse(
            r#"<svg xmlns="http://www.w3.org/2000/svg" width="96" height="96" viewBox="0 0 96 96">
                 <g data-type="cutline"><path d="M0 96 L96 96" stroke="black" fill="none"/></g>
                 <g data-type="seamline"><path d="M0 48 L96 48" stroke="black" fill="none"/></g>
               </svg>"#,
        )
        .expect("HPGL test SVG should parse")
    } // fn hpgl_test_doc

    // @brief Plot mode writes an HP-GL file with both pens.
    #[test]
    fn do_export_hpgl_writes_plot_file() {
        let path = std::env::temp_dir().join("seamlylayout_hpgl_plot_test.plt");
        let _ = std::fs::remove_file(&path);
        let options = parse_hpgl_options(r#"{"mode":"plot"}"#).unwrap();
        do_export_hpgl(&hpgl_test_doc(), path.to_str().unwrap(), &options).expect("export should succeed");
        let program = std::fs::read_to_string(&path).expect("file should exist");
        assert!(program.starts_with("IN;"), "{program}");
        assert!(program.contains("SP1;") && program.contains("SP2;"), "{program}");
        let _ = std::fs::remove_file(&path);
    } // do_export_hpgl_writes_plot_file

    // @brief Cut mode writes the cut pen only.
    #[test]
    fn do_export_hpgl_cut_mode_skips_marks() {
        let path = std::env::temp_dir().join("seamlylayout_hpgl_cut_test.hpgl");
        let _ = std::fs::remove_file(&path);
        let options = parse_hpgl_options(r#"{"mode":"cut","cutPen":4}"#).unwrap();
        do_export_hpgl(&hpgl_test_doc(), path.to_str().unwrap(), &options).expect("export should succeed");
        let program = std::fs::read_to_string(&path).expect("file should exist");
        assert!(program.contains("SP4;") && !program.contains("SP2;"), "{program}");
        let _ = std::fs::remove_file(&path);
    } // do_export_hpgl_cut_mode_skips_marks

    // @brief Options JSON: pens default, bad mode and bad pen are errors.
    #[test]
    fn parse_hpgl_options_defaults_and_errors() {
        let options = parse_hpgl_options(r#"{"mode":"plot","labelPen":6}"#).unwrap();
        assert_eq!(options.pens, PenMap { cut: 1, mark: 2, label: 6 });
        assert!(parse_hpgl_options(r#"{"mode":"engrave"}"#).is_err());
        assert!(parse_hpgl_options(r#"{"mode":"plot","cutPen":9}"#).is_err());
        assert!(parse_hpgl_options("not json").is_err());
    } // parse_hpgl_options_defaults_and_errors

    // @brief The G-Code stub returns Err and writes no file.
    #[test]
    fn do_export_gcode_stub_returns_err_and_writes_no_file() {
        let doc = Document::parse(r#"<svg xmlns="http://www.w3.org/2000/svg"/>"#)
            .expect("minimal SVG should parse");
        let path = std::env::temp_dir().join("seamlylayout_stub_test.gcode");
        let _ = std::fs::remove_file(&path);
        assert!(do_export_gcode(&doc, path.to_str().unwrap()).is_err());
        assert!(!path.exists(), "stub must not write a file");
    } // do_export_gcode_stub_returns_err_and_writes_no_file

    // @brief The 3D mesh stub returns Err and writes no file.
    #[test]
    fn do_export_mesh_stub_returns_err_and_writes_no_file() {
        let doc = Document::parse(r#"<svg xmlns="http://www.w3.org/2000/svg"/>"#)
            .expect("minimal SVG should parse");
        let path = std::env::temp_dir().join("seamlylayout_stub_test.3mf");
        let _ = std::fs::remove_file(&path);
        assert!(do_export_mesh(&doc, path.to_str().unwrap()).is_err());
        assert!(!path.exists(), "stub must not write a file");
    } // do_export_mesh_stub_returns_err_and_writes_no_file
} // mod tests
