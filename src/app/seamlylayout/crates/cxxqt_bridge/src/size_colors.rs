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
// Colors follow the size order, smallest size first. The candidate colors run
// from dark to pastel in every hue. The first size gets blue. Each next size gets
// the candidate farthest from white and from every color given so far, so no two
// sizes look alike and every color shows on white. Distances are in OKLab, where
// equal distances look about equally different.

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
    let colors = palette_colors(sorted.len());
    sorted.into_iter().zip(colors)
        .map(|(size, color)| SizeColor { size: size.clone(), color })
        .collect()
} // fn size_palette

// Candidate colors, in OKLCH: OKLab lightness, chroma and hue.
// Lightness runs from dark to pastel; a lighter color is hard to see on white media.
const LIGHTNESS_LEVELS: [f64; 5] = [0.40, 0.50, 0.60, 0.70, 0.80];
const HUE_STEP_DEG: usize = 10;
// More chroma leaves sRGB in most hues and adds little difference.
const MAX_CHROMA: f64 = 0.20;
// A color with less chroma looks gray.
const MIN_CHROMA: f64 = 0.07;
// The smallest size is blue.
const FIRST_LIGHTNESS: f64 = 0.50;
const FIRST_HUE_DEG: usize = 260;
// The media color; no size color comes close to it.
const WHITE: Oklab = [1.0, 0.0, 0.0];

// @brief An OKLab color: lightness, then the green-red and blue-yellow axes.
type Oklab = [f64; 3];

// @brief `count` colors, `#rrggbb`, one per size in size order.
//
// The first size gets blue. Each next size gets the candidate farthest from
// white and from every color given so far. More sizes than candidates repeat
// the colors from the start.
fn palette_colors(count: usize) -> Vec<String> {
    let candidates = candidate_colors();
    let first = oklch(FIRST_LIGHTNESS, FIRST_HUE_DEG).expect("blue holds chroma");
    // Distance from each candidate to white and to the nearest color given so far.
    let mut nearest: Vec<f64> = candidates.iter().map(|c| distance(c, &WHITE)).collect();
    let mut picks: Vec<Oklab> = Vec::new();
    while picks.len() < count.min(candidates.len()) {
        let pick = if picks.is_empty() {
            first
        } else {
            // The farthest candidate; on a tie the first, so the order never changes.
            let farthest = (0..candidates.len()).fold(0, |best, i| if nearest[i] > nearest[best] { i } else { best });
            candidates[farthest]
        }; // if first size
        for (d, candidate) in nearest.iter_mut().zip(&candidates) {
            *d = d.min(distance(candidate, &pick));
        } // for candidate
        picks.push(pick);
    } // while colors needed
    (0..count).map(|i| hex(&picks[i % picks.len()])).collect()
} // fn palette_colors

// @brief Each lightness level in each hue step, at the most chroma sRGB holds; gray ones left out.
fn candidate_colors() -> Vec<Oklab> {
    LIGHTNESS_LEVELS.iter()
        .flat_map(|&lightness| (0..360).step_by(HUE_STEP_DEG).filter_map(move |hue| oklch(lightness, hue)))
        .collect()
} // fn candidate_colors

// @brief The color of this lightness and hue with the most chroma, up to `MAX_CHROMA`, that sRGB holds.
// @return None when that chroma is below `MIN_CHROMA`.
fn oklch(lightness: f64, hue_deg: usize) -> Option<Oklab> {
    let (sin, cos) = (hue_deg as f64).to_radians().sin_cos();
    let at = |chroma: f64| [lightness, chroma * cos, chroma * sin];
    let mut chroma = MAX_CHROMA;
    if !in_srgb(&at(chroma)) {
        // Bisect between gray, which sRGB holds, and MAX_CHROMA, which it does not.
        let (mut inside, mut outside) = (0.0, MAX_CHROMA);
        for _ in 0..24 {
            let middle = (inside + outside) / 2.0;
            if in_srgb(&at(middle)) { inside = middle } else { outside = middle }
        } // for step
        chroma = inside;
    } // if out of sRGB
    (chroma >= MIN_CHROMA).then(|| at(chroma))
} // fn oklch

// @brief True when every linear sRGB channel of `color` is in 0–1.
fn in_srgb(color: &Oklab) -> bool {
    linear_srgb(color).iter().all(|c| (0.0..=1.0).contains(c))
} // fn in_srgb

// @brief Linear sRGB channels of an OKLab color (Björn Ottosson's matrices).
fn linear_srgb(&[lightness, a, b]: &Oklab) -> [f64; 3] {
    let l = (lightness + 0.396_337_777_4 * a + 0.215_803_757_3 * b).powi(3);
    let m = (lightness - 0.105_561_345_8 * a - 0.063_854_172_8 * b).powi(3);
    let s = (lightness - 0.089_484_177_5 * a - 1.291_485_548_0 * b).powi(3);
    [
        4.076_741_662_1 * l - 3.307_711_591_3 * m + 0.230_969_929_2 * s,
        -1.268_438_004_6 * l + 2.609_757_401_1 * m - 0.341_319_396_5 * s,
        -0.004_196_086_3 * l - 0.703_418_614_7 * m + 1.707_614_701_0 * s,
    ]
} // fn linear_srgb

// @brief `#rrggbb` of an OKLab color inside sRGB.
fn hex(color: &Oklab) -> String {
    let byte = |linear: f64| {
        let linear = linear.max(0.0);
        let encoded = if linear <= 0.003_130_8 { 12.92 * linear } else { 1.055 * linear.powf(1.0 / 2.4) - 0.055 };
        (encoded * 255.0).round().clamp(0.0, 255.0) as u8
    };
    let [r, g, b] = linear_srgb(color).map(byte);
    format!("#{r:02x}{g:02x}{b:02x}")
} // fn hex

// @brief Euclidean OKLab distance.
fn distance(p: &Oklab, q: &Oklab) -> f64 {
    p.iter().zip(q).map(|(a, b)| (a - b) * (a - b)).sum::<f64>().sqrt()
} // fn distance

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

    // Smallest OKLab distance of a size color to another size color, and to white.
    // The golden-angle palette had two of 26 sizes 0.012 apart.
    const MIN_SIZE_DISTANCE: f64 = 0.1;
    const MIN_WHITE_DISTANCE: f64 = 0.2;

    // @brief OKLab of a `#rrggbb` color, so a test reads what the SVG gets.
    fn oklab_of(color: &str) -> Oklab {
        let linear = |i: usize| {
            let c = u8::from_str_radix(&color[i..i + 2], 16).unwrap() as f64 / 255.0;
            if c <= 0.040_45 { c / 12.92 } else { ((c + 0.055) / 1.055).powf(2.4) }
        };
        let (r, g, b) = (linear(1), linear(3), linear(5));
        let l = (0.412_221_470_8 * r + 0.536_332_536_3 * g + 0.051_445_992_9 * b).cbrt();
        let m = (0.211_903_498_2 * r + 0.680_699_545_1 * g + 0.107_396_956_6 * b).cbrt();
        let s = (0.088_302_461_9 * r + 0.281_718_837_6 * g + 0.629_978_700_5 * b).cbrt();
        [
            0.210_454_255_3 * l + 0.793_617_785_0 * m - 0.004_072_046_8 * s,
            1.977_998_495_1 * l - 2.428_592_205_0 * m + 0.450_593_709_9 * s,
            0.025_904_037_1 * l + 0.782_771_766_2 * m - 0.808_675_766_0 * s,
        ]
    } // fn oklab_of

    // @brief 26 sizes: each color is `#rrggbb`, far from every other size and from white; the first is blue.
    #[test]
    fn palette_colors_differ_and_show_on_white() {
        let sizes: Vec<String> = (0..26).map(|i| (220 + 20 * i).to_string()).collect();
        let palette = size_palette(&sizes);
        assert_eq!(palette.len(), 26);
        assert_eq!(palette[0].size, "220");
        for entry in &palette {
            assert!(entry.color.len() == 7 && entry.color.starts_with('#'), "{}", entry.color);
        } // for entry
        let colors: Vec<Oklab> = palette.iter().map(|p| oklab_of(&p.color)).collect();
        for (i, color) in colors.iter().enumerate() {
            let white = distance(color, &WHITE);
            assert!(white >= MIN_WHITE_DISTANCE, "size {} {} near white: {white:.3}", palette[i].size, palette[i].color);
            for (j, other) in colors.iter().enumerate().skip(i + 1) {
                let apart = distance(color, other);
                assert!(apart >= MIN_SIZE_DISTANCE, "sizes {} {} and {} {} look alike: {apart:.3}",
                    palette[i].size, palette[i].color, palette[j].size, palette[j].color);
            } // for other
        } // for color
        let channel = |i: usize| u8::from_str_radix(&palette[0].color[i..i + 2], 16).unwrap();
        assert!(channel(5) > channel(1) && channel(5) > channel(3), "first size not blue: {}", palette[0].color);
    } // palette_colors_differ_and_show_on_white

    // @brief 26 sizes get both dark and pastel colors.
    #[test]
    fn palette_spans_dark_to_pastel() {
        let colors: Vec<Oklab> = palette_colors(26).iter().map(|c| oklab_of(c)).collect();
        let dark = colors.iter().filter(|c| c[0] <= 0.45).count();
        let pastel = colors.iter().filter(|c| c[0] >= 0.75).count();
        assert!(dark >= 4 && pastel >= 4, "dark {dark}, pastel {pastel}");
    } // palette_spans_dark_to_pastel

    // @brief No color repeats until every candidate is used; then the colors repeat in order.
    #[test]
    fn palette_repeats_only_past_the_candidates() {
        let count = candidate_colors().len();
        let colors = palette_colors(count + 2);
        let mut distinct = colors[..count].to_vec();
        distinct.sort();
        distinct.dedup();
        assert_eq!(distinct.len(), count, "colors repeat");
        assert_eq!(colors[count..], colors[..2]);
        assert!(palette_colors(0).is_empty());
    } // palette_repeats_only_past_the_candidates

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
