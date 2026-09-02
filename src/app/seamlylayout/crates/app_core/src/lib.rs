// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

use std::fs;
use std::path::Path;

use resvg::tiny_skia::{Color, Pixmap, Transform};
use svg2pdf::{ConversionOptions, PageOptions};
use svg_dom::Document;
use thiserror::Error;

// @brief Application-level errors for SVG load/render steps.
#[derive(Debug, Error)]
pub enum CoreError {
    // Filesystem I/O failed.
    #[error("io error: {0}")]
    Io(#[from] std::io::Error),
    // XML parsing failed.
    #[error("xml parse error: {0}")]
    Xml(#[from] xmltree::ParseError),
    // usvg parse failed.
    #[error("usvg parse error: {0}")]
    Usvg(#[from] usvg::Error),
    // PNG encoding failed during render output.
    #[error("png encode error: {0}")]
    Png(#[from] png::EncodingError),
    // Rendering failed to produce an image.
    #[error("render failed")]
    RenderFailed,
    // Pixmap could not be allocated for the requested size.
    #[error("invalid output size")]
    InvalidSize,
}

// @brief Result alias for app_core operations.
pub type CoreResult<T> = Result<T, CoreError>;

// @brief Load an SVG file into both the editable DOM wrapper and a usvg tree.
// @param path Path to an SVG file.
// @return Tuple of (Document, usvg::Tree) ready for mutation and rendering.
pub fn load_svg(path: impl AsRef<Path>) -> CoreResult<(Document, usvg::Tree)> {
    // Read file as UTF-8 text.
    let data = fs::read_to_string(&path)?;

    // One parse path for both entry points, so a file import and the Seamly2D
    // in-memory handoff cannot diverge.
    parse_svg(&data, path.as_ref().parent())
}

// @brief Parse SVG text into both the editable DOM wrapper and a usvg tree.
// @param data SVG document text.
// @param resources_dir Directory used to resolve external references (images,
//        fonts). `None` when the SVG did not come from disk — the Seamly2D
//        piece-mode handoff, which arrives as a string with no home directory.
// @return Tuple of (Document, usvg::Tree) ready for mutation and rendering.
pub fn parse_svg(data: &str, resources_dir: Option<&Path>) -> CoreResult<(Document, usvg::Tree)> {
    // Parse editable DOM (xmltree-based) for attribute edits.
    let doc = Document::parse(data)?;

    // Parse into usvg tree for rendering/geometry tasks.
    let mut opt = usvg::Options::default();
    opt.fontdb_mut().load_system_fonts();
    opt.resources_dir = resources_dir.map(Path::to_path_buf);
    let tree = usvg::Tree::from_data(data.as_bytes(), &opt)?;

    Ok((doc, tree))
}

// @brief Save an editable Document back to disk as SVG.
// @param doc DOM to serialize.
// @param path Destination path.
pub fn save_svg(doc: &Document, path: impl AsRef<Path>) -> CoreResult<()> {
    // Serialize with indentation for readability.
    let serialized = doc.to_string();
    fs::write(path, serialized)?;
    Ok(())
}

// @brief Convert a Document to a usvg::Tree for rendering.
// @param doc The SVG DOM document to convert.
// @param resources_dir Optional directory path for resolving external resources (e.g., images, fonts).
// @return A `usvg::Tree` ready for rendering.
pub fn document_to_tree(doc: &Document, resources_dir: Option<&Path>) -> CoreResult<usvg::Tree> {
    // Serialize the document to SVG string.
    let svg_string = doc.to_string();

    // Parse the SVG string into a usvg tree for rendering.
    let mut opt = usvg::Options::default();
    opt.fontdb_mut().load_system_fonts();
    opt.resources_dir = resources_dir.map(Path::to_path_buf);
    let tree = usvg::Tree::from_data(svg_string.as_bytes(), &opt)?;

    Ok(tree)
}

// @brief Load an SVG file, add a white background rectangle, and return a usvg tree for rendering.
// @param path Path to an SVG file.
// @return A `usvg::Tree` ready for rendering with a white background rectangle added.
pub fn load_svg_with_background(path: impl AsRef<Path>) -> CoreResult<usvg::Tree> {
    // Load the SVG into a DOM document.
    let (mut doc, _) = load_svg(path.as_ref())?;

    // Add a white background rectangle as the first child of the SVG root.
    doc.add_background_rect();

    // Serialize the modified DOM back to SVG string.
    let modified_doc_str = doc.to_string();

    // Parse the modified SVG into a usvg tree for rendering.
    let mut opt = usvg::Options::default();
    opt.fontdb_mut().load_system_fonts();
    opt.resources_dir = path.as_ref().parent().map(Path::to_path_buf);
    let tree = usvg::Tree::from_data(modified_doc_str.as_bytes(), &opt)?;

    Ok(tree)
}

// @brief Calculate an appropriate preview scale to limit image size.
// @param tree The usvg tree to calculate scale for.
// @param max_width Maximum width in pixels for the preview.
// @return Scale factor to use for rendering.
pub fn calculate_preview_scale(tree: &usvg::Tree, max_width: f32) -> f32 {
    let svg_size = tree.size();
    if svg_size.width() > max_width {
        max_width / svg_size.width()
    } else {
        1.0
    }
}

// @brief Get the dimensions of a usvg tree (for debugging).
// @param tree The usvg tree to get dimensions from.
// @return Tuple of (width, height) in pixels.
pub fn get_tree_size(tree: &usvg::Tree) -> (f32, f32) {
    let size = tree.size();
    (size.width(), size.height())
}

// @brief Render a usvg tree to a PDF file using svg2pdf.
// @param tree Parsed usvg tree to render.
// @param out_path Destination PDF path.
// Vector content is preserved at full quality; only filtered objects are rasterised.
pub fn render_pdf(tree: &usvg::Tree, out_path: impl AsRef<Path>) -> CoreResult<()> {
    // Convert the usvg tree to PDF bytes with default options.
    let pdf_bytes = svg2pdf::to_pdf(
        tree,
        ConversionOptions::default(),
        PageOptions::default(),
    );

    // Write the PDF bytes to disk.
    fs::write(out_path, pdf_bytes)?;
    Ok(())
}

// @brief Render a usvg tree to a PNG file using resvg + tiny-skia.
// @param tree Parsed usvg tree to render.
// @param out_path Destination PNG path.
// @param scale Scale factor applied to the original SVG size (1.0 = natural size).
pub fn render_png(tree: &usvg::Tree, out_path: impl AsRef<Path>, scale: f32) -> CoreResult<()> {
    // Determine target pixel dimensions from the SVG size.
    let size = tree.size().to_int_size();
    let w = ((size.width() as f32) * scale).max(1.0).round() as u32;
    let h = ((size.height() as f32) * scale).max(1.0).round() as u32;

    // Allocate a pixel buffer.
    let mut pixmap = Pixmap::new(w, h).ok_or(CoreError::InvalidSize)?;

    // Fill the pixmap with a white background to ensure SVG content is visible.
    pixmap.fill(Color::WHITE);

    // Create a transform that scales from SVG coordinates to pixmap coordinates.
    // The tree.size() gives us the SVG's natural size, and we need to scale it to fit the pixmap.
    let svg_size = tree.size();
    let scale_x = w as f32 / svg_size.width();
    let scale_y = h as f32 / svg_size.height();
    let transform = Transform::from_scale(scale_x, scale_y);

    // Render into the pixmap.
    let mut pixmap_mut = pixmap.as_mut();
    resvg::render(tree, transform, &mut pixmap_mut);

    // Persist to PNG.
    pixmap.save_png(out_path)?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::env;
    use std::fs;

    // @brief Create a tiny SVG for round-trip tests.
    fn sample_svg() -> &'static str {
        r#"<svg width="10" height="10" xmlns="http://www.w3.org/2000/svg"><rect id="r1" width="5" height="5"/></svg>"#
    }

    // @brief Ensure load/save round-trips without mutation.
    #[test]
    fn load_and_save_round_trip() {
        let tmp_dir = env::temp_dir();
        let input_path = tmp_dir.join("app_core_roundtrip.svg");
        fs::write(&input_path, sample_svg()).unwrap();

        let (doc, _tree) = load_svg(&input_path).unwrap();
        let out_path = tmp_dir.join("app_core_roundtrip_out.svg");
        save_svg(&doc, &out_path).unwrap();

        let reread = fs::read_to_string(out_path).unwrap();
        assert!(reread.contains("rect"));
    }

    // @brief Render a simple SVG to PNG to validate resvg plumbing.
    #[test]
    fn render_to_png() {
        let tmp_dir = env::temp_dir();
        let input_path = tmp_dir.join("app_core_render.svg");
        fs::write(&input_path, sample_svg()).unwrap();

        let (_doc, tree) = load_svg(&input_path).unwrap();
        let out_path = tmp_dir.join("app_core_render.png");
        render_png(&tree, &out_path, 1.0).unwrap();

        let png = fs::read(&out_path).unwrap();
        assert!(!png.is_empty());
    }
}
