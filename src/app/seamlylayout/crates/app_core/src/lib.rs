// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

use std::fs;
use std::io::Write;
use std::path::Path;

use image::codecs::jpeg::{JpegEncoder, PixelDensity};
use image::ExtendedColorType;
use resvg::tiny_skia::{Color, Pixmap, Transform};
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
    // JPEG encoding failed during render output.
    #[error("jpeg encode error: {0}")]
    Jpeg(#[from] image::ImageError),
    // Rendering failed to produce an image.
    #[error("render failed")]
    RenderFailed,
    // Pixmap could not be allocated for the requested size.
    #[error("invalid output size")]
    InvalidSize,
    // The image is larger than JPEG allows on one side.
    #[error("image is {0} x {1} px; JPEG allows at most {JPEG_MAX_SIDE_PX} px per side")]
    JpegTooLarge(u32, u32),
}

// @brief Largest width or height, in pixels, that a JPEG file can store.
pub const JPEG_MAX_SIDE_PX: u32 = 65_535;

// @brief Default JPEG quality (1-100) for layout exports.
pub const JPEG_DEFAULT_QUALITY: u8 = 90;

// @brief SVG user units per inch (CSS px): the resolution of a raster at scale 1.0.
pub const SVG_PX_PER_INCH: f32 = 96.0;

// @brief Millimetres per inch.
const MM_PER_INCH: f32 = 25.4;

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

// @brief Pixel dimensions of a tree rendered at the given scale.
// @param tree Parsed usvg tree.
// @param scale Scale factor applied to the original SVG size (1.0 = natural size).
// @return (width, height) in pixels, each at least 1.
fn raster_size(tree: &usvg::Tree, scale: f32) -> (u32, u32) {
    let size = tree.size().to_int_size();
    let w = ((size.width() as f32) * scale).max(1.0).round() as u32;
    let h = ((size.height() as f32) * scale).max(1.0).round() as u32;
    (w, h)
}

// @brief Render a usvg tree into an opaque pixmap on a white background.
// @param tree Parsed usvg tree to render.
// @param w Target width in pixels.
// @param h Target height in pixels.
fn render_pixmap(tree: &usvg::Tree, w: u32, h: u32) -> CoreResult<Pixmap> {
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
    Ok(pixmap)
}

// @brief PNG `pHYs` resolution, in pixels per metre, of a raster rendered at `scale`.
fn png_pixels_per_metre(scale: f32) -> u32 {
    (SVG_PX_PER_INCH * scale * 1000.0 / MM_PER_INCH).round().max(1.0) as u32
}

// @brief JFIF density, in pixels per inch, of a raster rendered at `scale`.
fn jpeg_pixels_per_inch(scale: f32) -> u16 {
    (SVG_PX_PER_INCH * scale).round().clamp(1.0, u16::MAX as f32) as u16
}

// @brief Render a usvg tree to a PNG file using resvg + tiny-skia.
// @param tree Parsed usvg tree to render.
// @param out_path Destination PNG path.
// @param scale Scale factor applied to the original SVG size (1.0 = natural size).
pub fn render_png(tree: &usvg::Tree, out_path: impl AsRef<Path>, scale: f32) -> CoreResult<()> {
    let (w, h) = raster_size(tree, scale);
    let pixmap = render_pixmap(tree, w, h)?;

    // Pixmap::save_png writes no pHYs chunk. A viewer or print dialog then guesses
    // the resolution, and "actual size" prints at the wrong scale.
    let ppm = png_pixels_per_metre(scale);
    let mut bytes = Vec::new();
    let mut encoder = png::Encoder::new(&mut bytes, w, h);
    encoder.set_color(png::ColorType::Rgba);
    encoder.set_depth(png::BitDepth::Eight);
    encoder.set_pixel_dims(Some(png::PixelDimensions { xppu: ppm, yppu: ppm, unit: png::Unit::Meter }));
    let mut writer = encoder.write_header()?;

    // The white fill makes every pixel opaque, so premultiplied RGBA equals straight RGBA.
    writer.write_image_data(pixmap.data())?;
    writer.finish()?;

    fs::write(out_path, bytes)?;
    Ok(())
}

// @brief Render a usvg tree to a JPEG file using resvg + tiny-skia.
// @param tree Parsed usvg tree to render.
// @param out_path Destination JPEG path.
// @param scale Scale factor applied to the original SVG size (1.0 = natural size).
// @param quality JPEG quality, 1-100; out-of-range values are clamped.
// @return Err(JpegTooLarge) before any rendering when either side exceeds JPEG_MAX_SIDE_PX.
pub fn render_jpeg(tree: &usvg::Tree, out_path: impl AsRef<Path>, scale: f32, quality: u8) -> CoreResult<()> {
    // Check the JPEG size limit first, so an oversized layout never allocates its pixmap.
    let (w, h) = raster_size(tree, scale);
    if w > JPEG_MAX_SIDE_PX || h > JPEG_MAX_SIDE_PX {
        return Err(CoreError::JpegTooLarge(w, h));
    } // if too large

    let pixmap = render_pixmap(tree, w, h)?;

    // The white fill makes every pixel opaque, so premultiplied RGBA equals
    // straight RGBA and the alpha byte can be dropped.
    let rgb: Vec<u8> = pixmap
        .data()
        .chunks_exact(4)
        .flat_map(|px| [px[0], px[1], px[2]])
        .collect();

    // Encode through a buffered writer; the file is created only after the size check.
    let file = fs::File::create(out_path)?;
    let mut writer = std::io::BufWriter::new(file);
    let mut encoder = JpegEncoder::new_with_quality(&mut writer, quality.clamp(1, 100));
    // Without a density unit, JFIF stores only the aspect ratio and a viewer guesses the print size.
    encoder.set_pixel_density(PixelDensity::dpi(jpeg_pixels_per_inch(scale)));
    encoder.encode(&rgb, w, h, ExtendedColorType::Rgb8)?;
    writer.flush()?;
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

    // @brief Render a simple SVG to JPEG; the file starts with the JPEG SOI marker.
    #[test]
    fn render_to_jpeg() {
        let tmp_dir = env::temp_dir();
        let input_path = tmp_dir.join("app_core_render_jpeg.svg");
        fs::write(&input_path, sample_svg()).unwrap();

        let (_doc, tree) = load_svg(&input_path).unwrap();
        let out_path = tmp_dir.join("app_core_render.jpg");
        render_jpeg(&tree, &out_path, 1.0, JPEG_DEFAULT_QUALITY).unwrap();

        let jpeg = fs::read(&out_path).unwrap();
        assert_eq!(&jpeg[..3], &[0xFF, 0xD8, 0xFF], "JPEG must start with the SOI marker");
    }

    // @brief The PNG carries a pHYs chunk: 3780 px/m (96 dpi) at scale 1.0, 7559 px/m at 2.0.
    #[test]
    fn render_png_writes_resolution() {
        let tree = usvg::Tree::from_str(sample_svg(), &usvg::Options::default()).unwrap();
        for (scale, expected_ppm) in [(1.0, 3780), (2.0, 7559)] {
            let out_path = env::temp_dir().join(format!("app_core_dpi_{}_{scale}.png", std::process::id()));
            render_png(&tree, &out_path, scale).unwrap();

            let reader = png::Decoder::new(fs::File::open(&out_path).unwrap()).read_info().unwrap();
            let dims = reader.info().pixel_dims.expect("PNG must have a pHYs chunk");
            assert_eq!((dims.xppu, dims.yppu), (expected_ppm, expected_ppm), "scale {scale}");
            assert_eq!(dims.unit, png::Unit::Meter, "scale {scale}");
            assert_eq!(reader.info().width, (10.0 * scale) as u32, "scale {scale}");
        } // for scale
    }

    // @brief The JPEG JFIF header stores 96 x 96 dots per inch at scale 1.0.
    #[test]
    fn render_jpeg_writes_resolution() {
        let tree = usvg::Tree::from_str(sample_svg(), &usvg::Options::default()).unwrap();
        let out_path = env::temp_dir().join(format!("app_core_dpi_{}.jpg", std::process::id()));
        render_jpeg(&tree, &out_path, 1.0, JPEG_DEFAULT_QUALITY).unwrap();

        // JFIF APP0 after SOI: marker FFE0, length (2), "JFIF\0", version (2),
        // units (1 = dots per inch), X density (2), Y density (2), big-endian.
        let jpeg = fs::read(&out_path).unwrap();
        assert_eq!(&jpeg[2..4], &[0xFF, 0xE0], "APP0 must follow SOI");
        assert_eq!(&jpeg[6..11], b"JFIF\0");
        assert_eq!(jpeg[13], 1, "density unit must be dots per inch");
        assert_eq!(u16::from_be_bytes([jpeg[14], jpeg[15]]), 96, "X density");
        assert_eq!(u16::from_be_bytes([jpeg[16], jpeg[17]]), 96, "Y density");
    }

    // @brief A layout wider than JPEG allows returns JpegTooLarge and writes no file.
    #[test]
    fn render_jpeg_rejects_oversized_image() {
        let wide = r#"<svg width="70000" height="10" xmlns="http://www.w3.org/2000/svg"><rect width="5" height="5"/></svg>"#;
        let tree = usvg::Tree::from_str(wide, &usvg::Options::default()).unwrap();
        let out_path = env::temp_dir().join(format!("app_core_too_wide_{}.jpg", std::process::id()));
        let _ = fs::remove_file(&out_path);

        let result = render_jpeg(&tree, &out_path, 1.0, JPEG_DEFAULT_QUALITY);

        assert!(matches!(result, Err(CoreError::JpegTooLarge(70000, 10))), "got {result:?}");
        assert!(!out_path.exists(), "no file may be written for an oversized image");
    }
}
