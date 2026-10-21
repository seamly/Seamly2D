// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @file size_colors.rs
// @brief Gives each size of a Nested multisize layout its own color, and draws a
// size-to-color legend.
//
// The Nested tab stacks every size of a piece in one place. Each size gets one
// color; every line, notch and label of a piece of that size takes it. The
// legend lists "Size <size>" beside a square filled with its color.
//
// Colors follow the size order, smallest size first. Each next color turns the
// hue by the golden angle, so neighbor sizes, which lie closest in the stack,
// get very different hues. Lightness alternates between two values, and a color
// brighter than `MAX_LUMINANCE` is darkened, so every color shows on white.

use xmltree::{Element, XMLNode};

use crate::size_groups::compare_sizes;

// @brief `data-type` and `id` of the legend group.
pub(crate) const LEGEND_TYPE: &str = "size_legend";

// Legend geometry, in layout pixels (96 per inch).
const LEGEND_FONT_PX: f64 = 16.0;
const LEGEND_PAD_PX: f64 = 8.0;
// Side of the square swatch.
const LEGEND_SWATCH_PX: f64 = 32.0;
// A row holds one swatch, with one pad between rows.
const LEGEND_ROW_PX: f64 = LEGEND_SWATCH_PX + LEGEND_PAD_PX;
const LEGEND_SWATCH_GAP_PX: f64 = 6.0;
const LEGEND_COLUMN_GAP_PX: f64 = 16.0;
// Text width per character, as a part of the font size; wide enough for digits in a sans font.
const LEGEND_CHAR_WIDTH: f64 = 0.62;

// @brief One size and its color, `#rrggbb`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub(crate) struct SizeColor {
    pub size: String,
    pub color: String,
}

// @brief The color of each size, in size order, smallest first.
// @param sizes Distinct `data-size` values, in any order.
pub(crate) fn size_palette(sizes: &[String]) -> Vec<SizeColor> {
    let mut sorted: Vec<&String> = sizes.iter().collect();
    sorted.sort_by(|a, b| compare_sizes(a, b));
    sorted.dedup();
    sorted.into_iter().enumerate()
        .map(|(i, size)| SizeColor { size: size.clone(), color: palette_color(i) })
        .collect()
} // fn size_palette

// Highest relative luminance of a size color (0 black, 1 white).
const MAX_LUMINANCE: f64 = 0.3;

// @brief Color number `index`: golden-angle hue steps, two lightness values, never too bright.
fn palette_color(index: usize) -> String {
    const GOLDEN_ANGLE_DEG: f64 = 137.507_764;
    const START_HUE_DEG: f64 = 220.0; // blue for the smallest size
    let hue = (START_HUE_DEG + index as f64 * GOLDEN_ANGLE_DEG) % 360.0;
    let mut lightness = if index % 2 == 0 { 0.42 } else { 0.32 };
    // Yellow and green look much brighter than blue at one lightness.
    while lightness > 0.1 && luminance(hsl_to_rgb(hue, 0.85, lightness)) > MAX_LUMINANCE {
        lightness -= 0.02;
    } // while too bright
    let (r, g, b) = hsl_to_rgb(hue, 0.85, lightness);
    format!("#{r:02x}{g:02x}{b:02x}")
} // fn palette_color

// @brief Relative luminance of an sRGB color, without gamma correction.
fn luminance((r, g, b): (u8, u8, u8)) -> f64 {
    (0.2126 * r as f64 + 0.7152 * g as f64 + 0.0722 * b as f64) / 255.0
} // fn luminance

// @brief Convert HSL (hue in degrees, saturation and lightness 0–1) to RGB bytes.
fn hsl_to_rgb(hue: f64, saturation: f64, lightness: f64) -> (u8, u8, u8) {
    let chroma = (1.0 - (2.0 * lightness - 1.0).abs()) * saturation;
    let sector = hue / 60.0;
    let x = chroma * (1.0 - (sector % 2.0 - 1.0).abs());
    let (r, g, b) = match sector as u32 {
        0 => (chroma, x, 0.0),
        1 => (x, chroma, 0.0),
        2 => (0.0, chroma, x),
        3 => (0.0, x, chroma),
        4 => (x, 0.0, chroma),
        _ => (chroma, 0.0, x),
    }; // match sector
    let m = lightness - chroma / 2.0;
    let byte = |v: f64| ((v + m) * 255.0).round().clamp(0.0, 255.0) as u8;
    (byte(r), byte(g), byte(b))
} // fn hsl_to_rgb

// @brief Color every sized piece inside a top-level piece-set by its size.
//
// A stroke or fill that is not `none` takes the size color, on the piece group
// and every element in it, including `style` declarations. Text without a fill
// gets one, so a label does not fall back to black. The `piece-fill` overlay
// keeps its color.
//
// @param doc Layout document: piece-sets are direct children of the root (Nested).
// @return The color of each size, smallest size first; empty when `doc` has no sized piece-set member.
pub(crate) fn color_pieces_by_size(doc: &mut svg_dom::Document) -> Vec<SizeColor> {
    // Every size that a piece-set member carries.
    let mut sizes: Vec<String> = Vec::new();
    for set in piece_sets(&doc.root) {
        for piece in set.children.iter().filter_map(|n| n.as_element()) {
            if let Some(size) = piece.attributes.get("data-size") {
                if !sizes.contains(size) {
                    sizes.push(size.clone());
                } // if new size
            } // if sized
        } // for piece
    } // for set
    let palette = size_palette(&sizes);

    for node in doc.root.children.iter_mut() {
        let XMLNode::Element(set) = node else { continue };
        if !svg_dom::is_piece_set(set) {
            continue; // not a piece-set
        } // if not piece-set
        for piece in set.children.iter_mut().filter_map(|n| n.as_mut_element()) {
            let color = piece.attributes.get("data-size")
                .and_then(|size| palette.iter().find(|entry| entry.size == *size))
                .map(|entry| entry.color.clone());
            if let Some(color) = color {
                recolor(piece, &color);
            } // if sized
        } // for piece
    } // for node
    palette
} // fn color_pieces_by_size

// @brief The top-level piece-set groups of `root`.
fn piece_sets(root: &Element) -> impl Iterator<Item = &Element> {
    root.children.iter()
        .filter_map(|n| n.as_element())
        .filter(|e| svg_dom::is_piece_set(e))
} // fn piece_sets

// @brief Set every drawn stroke and fill in `elem` and its subtree to `color`.
fn recolor(elem: &mut Element, color: &str) {
    if elem.attributes.get("class").is_some_and(|c| c.split_whitespace().any(|n| n == "piece-fill")) {
        return; // display-only overlay
    } // if piece-fill
    for name in ["stroke", "fill"] {
        if let Some(value) = elem.attributes.get_mut(name) {
            if is_paint(value) {
                *value = color.to_string();
            } // if painted
        } // if attribute
    } // for name
    if (elem.name == "text" || elem.name == "tspan") && !elem.attributes.contains_key("fill") {
        elem.attributes.insert("fill".to_string(), color.to_string()); // default fill is black
    } // if text without fill
    if let Some(style) = elem.attributes.get("style") {
        let restyled = recolor_style(style, color);
        elem.attributes.insert("style".to_string(), restyled);
    } // if style
    for child in elem.children.iter_mut().filter_map(|n| n.as_mut_element()) {
        recolor(child, color);
    } // for child
} // fn recolor

// @brief True for a paint value that draws: not `none`, `transparent` or a paint server.
fn is_paint(value: &str) -> bool {
    let v = value.trim();
    !(v.is_empty() || v == "none" || v == "transparent" || v.starts_with("url("))
} // fn is_paint

// @brief `style` with each drawn `stroke:` and `fill:` declaration set to `color`.
fn recolor_style(style: &str, color: &str) -> String {
    style.split(';')
        .map(|declaration| {
            let Some((name, value)) = declaration.split_once(':') else {
                return declaration.to_string(); // empty or malformed: keep
            }; // if no colon
            if matches!(name.trim(), "stroke" | "fill") && is_paint(value) {
                format!("{}:{color}", name.trim())
            } else {
                declaration.to_string()
            } // if paint declaration
        })
        .collect::<Vec<_>>()
        .join(";")
} // fn recolor_style

// @brief Size-to-color legend: entries in rows, left to right, in a framed box.
#[derive(Debug, Clone)]
pub(crate) struct SizeLegend {
    entries: Vec<SizeColor>,
    columns: usize,
    entry_w: f64,
}

impl SizeLegend {
    // @brief Lay out the legend no wider than `max_width_px`; None when there are no entries.
    pub(crate) fn new(entries: Vec<SizeColor>, max_width_px: u32) -> Option<Self> {
        if entries.is_empty() {
            return None;
        } // if no entries
        let longest = entries.iter().map(|e| entry_text(&e.size).chars().count()).max().unwrap_or(0);
        let entry_w = LEGEND_SWATCH_PX + LEGEND_SWATCH_GAP_PX
            + longest as f64 * LEGEND_FONT_PX * LEGEND_CHAR_WIDTH + LEGEND_COLUMN_GAP_PX;
        let room = (max_width_px as f64 - 2.0 * LEGEND_PAD_PX).max(entry_w);
        let columns = ((room / entry_w).floor() as usize).clamp(1, entries.len());
        Some(Self { entries, columns, entry_w })
    } // fn new

    // @brief Rows of entries.
    fn rows(&self) -> usize {
        self.entries.len().div_ceil(self.columns)
    } // fn rows

    // @brief Legend width in pixels.
    pub(crate) fn width_px(&self) -> u32 {
        (2.0 * LEGEND_PAD_PX + self.columns as f64 * self.entry_w).ceil() as u32
    } // fn width_px

    // @brief Legend height in pixels.
    pub(crate) fn height_px(&self) -> u32 {
        (2.0 * LEGEND_PAD_PX + self.rows() as f64 * LEGEND_ROW_PX).ceil() as u32
    } // fn height_px

    // @brief The legend group, its top-left corner at (`x`, `y`) in layout pixels.
    pub(crate) fn element(&self, x: f64, y: f64) -> Element {
        let mut group = Element::new("g");
        group.attributes.insert("id".to_string(), LEGEND_TYPE.to_string());
        group.attributes.insert("data-type".to_string(), LEGEND_TYPE.to_string());

        // Frame around the entries.
        group.children.push(XMLNode::Element(element("rect", &[
            ("x", fmt(x)), ("y", fmt(y)),
            ("width", self.width_px().to_string()), ("height", self.height_px().to_string()),
            ("fill", "none".into()), ("stroke", "#000000".into()), ("stroke-width", "1".into()),
        ])));

        for (i, entry) in self.entries.iter().enumerate() {
            let left = x + LEGEND_PAD_PX + (i % self.columns) as f64 * self.entry_w;
            let middle = y + LEGEND_PAD_PX + (i / self.columns) as f64 * LEGEND_ROW_PX + LEGEND_ROW_PX / 2.0;
            // Swatch: a square filled with the size color, centered on the row.
            group.children.push(XMLNode::Element(element("rect", &[
                ("x", fmt(left)), ("y", fmt(middle - LEGEND_SWATCH_PX / 2.0)),
                ("width", fmt(LEGEND_SWATCH_PX)), ("height", fmt(LEGEND_SWATCH_PX)),
                ("fill", entry.color.clone()), ("stroke", "none".into()),
            ])));
            // Text: baseline a third of the font size below the middle of the row.
            let mut text = element("text", &[
                ("x", fmt(left + LEGEND_SWATCH_PX + LEGEND_SWATCH_GAP_PX)),
                ("y", fmt(middle + LEGEND_FONT_PX / 3.0)),
                ("font-family", "Arial".into()), ("font-size", fmt(LEGEND_FONT_PX)),
                ("fill", "#000000".into()), ("stroke", "none".into()),
            ]);
            text.children.push(XMLNode::Text(entry_text(&entry.size)));
            group.children.push(XMLNode::Element(text));
        } // for entry
        group
    } // fn element
} // impl SizeLegend

// @brief Legend text of one size.
fn entry_text(size: &str) -> String {
    format!("Size {size}")
} // fn entry_text

// @brief An element with the given attributes.
fn element(name: &str, attributes: &[(&str, String)]) -> Element {
    let mut elem = Element::new(name);
    for (key, value) in attributes {
        elem.attributes.insert((*key).to_string(), value.clone());
    } // for attribute
    elem
} // fn element

// @brief A coordinate with at most two decimals.
fn fmt(value: f64) -> String {
    let text = format!("{value:.2}");
    text.trim_end_matches('0').trim_end_matches('.').to_string()
} // fn fmt

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    const NESTED: &str = r##"<svg xmlns="http://www.w3.org/2000/svg">
  <g id="piece-set_A" data-type="piece-set">
    <rect class="piece-fill" fill="#ff0000" stroke="#ff0000"/>
    <g id="piece_A_s36" data-type="piece" data-size="36">
      <g data-type="cutline" stroke="black" fill="none"><g stroke="#000000"><path d="M0,0 L1,1"/></g></g>
      <g data-type="piece_label" fill="none"><text fill="#0000ff" stroke="none">A</text><text>B</text></g>
      <g data-type="notch"><path d="M0,0 L1,0" style="stroke:#000000;fill:none;stroke-width:1"/></g>
    </g>
    <g id="piece_A_s34" data-type="piece" data-size="34"><g stroke="black"><path d="M0,0 L1,1"/></g></g>
  </g>
</svg>"##;

    // @brief Every color differs, is `#rrggbb`, and is dark enough to show on white.
    #[test]
    fn palette_colors_are_distinct_and_dark() {
        let sizes: Vec<String> = (0..26).map(|i| (220 + 20 * i).to_string()).collect();
        let palette = size_palette(&sizes);
        assert_eq!(palette.len(), 26);
        assert_eq!(palette[0].size, "220");
        let mut colors: Vec<&str> = palette.iter().map(|p| p.color.as_str()).collect();
        for color in &colors {
            assert!(color.len() == 7 && color.starts_with('#'), "{color}");
            let channel = |i: usize| u8::from_str_radix(&color[i..i + 2], 16).unwrap();
            let lum = luminance((channel(1), channel(3), channel(5)));
            assert!(lum <= MAX_LUMINANCE, "{color} too light: {lum}");
        } // for color
        colors.sort();
        colors.dedup();
        assert_eq!(colors.len(), 26, "colors repeat");
    } // palette_colors_are_distinct_and_dark

    // @brief Sizes sort by number, smallest first, whatever order they arrive in.
    #[test]
    fn palette_follows_size_order() {
        let palette = size_palette(&["100".into(), "92".into(), "96".into(), "92".into()]);
        let sizes: Vec<&str> = palette.iter().map(|p| p.size.as_str()).collect();
        assert_eq!(sizes, ["92", "96", "100"]);
    } // palette_follows_size_order

    // @brief Every stroke and fill of a piece takes its size color; none and the overlay stay.
    #[test]
    fn pieces_take_their_size_color() {
        let mut doc = svg_dom::Document::parse(NESTED).expect("valid SVG");
        let palette = color_pieces_by_size(&mut doc);
        let sizes: Vec<&str> = palette.iter().map(|p| p.size.as_str()).collect();
        assert_eq!(sizes, ["34", "36"]);
        let (c34, c36) = (palette[0].color.clone(), palette[1].color.clone());
        let svg = doc.to_string();
        assert!(!svg.contains("black") && !svg.contains("#000000") && !svg.contains("#0000ff"), "{svg}");
        assert_eq!(svg.matches(r##""#ff0000""##).count(), 2, "overlay recolored: {svg}");
        assert!(svg.contains(&format!("stroke:{c36};fill:none")), "style: {svg}");
        assert_eq!(svg.matches(&format!(r#"stroke="{c34}""#)).count(), 1, "{svg}");
        assert!(svg.contains(r#"fill="none""#), "none kept: {svg}");
        // Text "B" had no fill and gets the size color.
        assert_eq!(svg.matches(&format!(r#"fill="{c36}""#)).count(), 2, "{svg}");
    } // pieces_take_their_size_color

    // @brief A layout without piece-sets gets no colors.
    #[test]
    fn layout_without_piece_sets_gets_no_colors() {
        let svg = r#"<svg xmlns="http://www.w3.org/2000/svg"><g id="p" data-type="piece" data-size="34" stroke="black"/></svg>"#;
        let mut doc = svg_dom::Document::parse(svg).expect("valid SVG");
        assert!(color_pieces_by_size(&mut doc).is_empty());
        assert!(doc.to_string().contains(r#"stroke="black""#));
    } // layout_without_piece_sets_gets_no_colors

    // @brief The legend wraps to the width it gets and lists every size in its color.
    #[test]
    fn legend_wraps_and_lists_every_size() {
        let sizes: Vec<String> = (0..26).map(|i| (220 + 20 * i).to_string()).collect();
        let palette = size_palette(&sizes);
        let wide = SizeLegend::new(palette.clone(), 10_000).expect("legend");
        assert_eq!(wide.rows(), 1);
        let narrow = SizeLegend::new(palette.clone(), 600).expect("legend");
        assert!(narrow.width_px() <= 600, "width {}", narrow.width_px());
        assert!(narrow.rows() > 1);
        assert_eq!(narrow.height_px(), (2.0 * LEGEND_PAD_PX + narrow.rows() as f64 * LEGEND_ROW_PX) as u32);

        let group = narrow.element(10.0, 20.0);
        assert_eq!(group.attributes.get("data-type").map(String::as_str), Some(LEGEND_TYPE));
        let texts: Vec<String> = group.children.iter().filter_map(|n| n.as_element())
            .filter(|e| e.name == "text").map(|e| e.get_text().unwrap_or_default().to_string()).collect();
        assert_eq!(texts.len(), 26);
        assert_eq!((texts[0].as_str(), texts[25].as_str()), ("Size 220", "Size 720"));
        // The first rect is the frame; each next rect is one swatch.
        let swatches: Vec<&Element> = group.children.iter().filter_map(|n| n.as_element())
            .filter(|e| e.name == "rect").skip(1).collect();
        assert_eq!(swatches.len(), 26);
        assert!(SizeLegend::new(Vec::new(), 600).is_none());
    } // legend_wraps_and_lists_every_size

    // @brief Each swatch is a square in its size color, no taller than its row.
    #[test]
    fn legend_swatches_are_filled_squares() {
        let palette = size_palette(&["34".into(), "36".into(), "38".into()]);
        let legend = SizeLegend::new(palette.clone(), 200).expect("legend");
        assert!(legend.rows() > 1, "test needs more than one row");
        let group = legend.element(0.0, 0.0);
        let swatches: Vec<&Element> = group.children.iter().filter_map(|n| n.as_element())
            .filter(|e| e.name == "rect").skip(1).collect();
        assert_eq!(swatches.len(), palette.len());
        let number = |e: &Element, name: &str| -> f64 { e.attributes[name].parse().unwrap() };
        for (swatch, entry) in swatches.iter().zip(&palette) {
            assert_eq!(number(swatch, "width"), LEGEND_SWATCH_PX);
            assert_eq!(number(swatch, "height"), LEGEND_SWATCH_PX);
            assert_eq!(swatch.attributes["fill"], entry.color);
            assert_eq!(swatch.attributes["stroke"], "none");
        } // for swatch
        // Rows do not overlap: the second row starts below the first swatch.
        let first_bottom = number(swatches[0], "y") + LEGEND_SWATCH_PX;
        let next_row_top = swatches.iter().map(|s| number(s, "y")).fold(f64::MIN, f64::max);
        assert!(next_row_top > first_bottom, "row top {next_row_top} above swatch bottom {first_bottom}");
        // The frame holds every swatch.
        let last_bottom = next_row_top + LEGEND_SWATCH_PX;
        assert!(last_bottom <= legend.height_px() as f64, "swatch bottom {last_bottom} past frame");
    } // legend_swatches_are_filled_squares
} // mod tests
