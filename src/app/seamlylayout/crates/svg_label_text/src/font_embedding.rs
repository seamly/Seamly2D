// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file font_embedding.rs
// @brief Modes 1 and 2: keep label `<text>` and embed its font as a subset `@font-face`.
//
// The subset holds only the glyphs the labels use, renumbered, with a new
// Unicode `cmap` for them; browsers need that `cmap` to map text to glyphs.
// Hinting and layout tables such as GSUB/GPOS are dropped: kerning and
// ligatures are lost, plain text is not.
//
// Font licenses decide embedding. A font whose OS/2 `fsType` is "restricted",
// or that forbids subsetting, is not embedded; its text stays and names the
// font, and the report says so.

use std::collections::{BTreeMap, BTreeSet};

use base64::Engine;
use xmltree::{Element, XMLNode};

use crate::{for_each_label_text, new_element, text_content, LabelTextReport, TextStyle};

/// Bundled font data for mode 2: Relief SingleLine Outline, SIL OFL 1.1
/// (assets/fonts/OFL.txt).
///
/// Relief's CAD TTF stores each stroke as an open contour. TrueType closes
/// every contour, so a browser draws "C" as "O" and "2" as "8". The Outline
/// variant draws the same strokes as thin filled shapes, which every SVG viewer
/// renders correctly. See `SINGLE_LINE_FONT_FAMILY` for the name the text uses.
const SINGLE_LINE_FONT: &[u8] = include_bytes!("../assets/fonts/ReliefSingleLineOutline-Regular.otf");

/// Family name mode 2 writes on label text and declares in `@font-face`.
///
/// The name is the CAD variant's, so a CAD/CAM tool that resolves text by its
/// installed fonts draws true single-line strokes. A viewer that honours the
/// embedded `@font-face` draws the Outline data under this name instead.
pub const SINGLE_LINE_FONT_FAMILY: &str = "Relief SingleLine CAD";

/// @brief One font face a label uses: family, CSS weight and italic flag.
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
struct FontKey {
    family: String,
    weight: u16,
    italic: bool,
} // struct FontKey

impl FontKey {
    /// @brief Face used by a `<text>` with `style`; None when it names no family.
    fn from_style(style: &TextStyle) -> Option<FontKey> {
        let family = first_family(style.font_family.as_deref()?)?;
        let weight = match style.font_weight.as_deref().map(str::trim) {
            Some("bold") => 700,                                   // CSS keyword
            Some(w) => w.parse::<u16>().unwrap_or(400),            // numeric weight
            None => 400,                                           // CSS initial value
        }; // match font-weight
        let italic = matches!(style.font_style.as_deref().map(str::trim), Some("italic") | Some("oblique"));
        Some(FontKey { family, weight, italic })
    } // fn from_style

    /// @brief CSS `font-style` value for this face.
    fn css_style(&self) -> &'static str {
        if self.italic { "italic" } else { "normal" }
    } // fn css_style
} // impl FontKey

/// @brief First family of a CSS `font-family` list, without quotes.
fn first_family(list: &str) -> Option<String> {
    let first = list.split(',').next()?.trim().trim_matches(|c| c == '"' || c == '\'').trim();
    (!first.is_empty()).then(|| first.to_string())
} // fn first_family

/// @brief Find the installed face that best matches `key`.
///
/// Tries the CSS-style query first. Windows lists some faces only under their
/// typographic family ("Yu Gothic UI") while Qt writes the legacy family
/// ("Yu Gothic UI Light"), so the fallback also compares the PostScript name
/// with spaces and hyphens removed.
fn find_face(fonts: &fontdb::Database, key: &FontKey) -> Option<fontdb::ID> {
    let style = if key.italic { fontdb::Style::Italic } else { fontdb::Style::Normal };
    let query = fontdb::Query {
        families: &[fontdb::Family::Name(&key.family)],
        weight: fontdb::Weight(key.weight),
        stretch: fontdb::Stretch::Normal,
        style,
    };
    if let Some(id) = fonts.query(&query) {
        return Some(id); // exact family match
    } // if query matched

    let squash = |s: &str| s.chars().filter(|c| !matches!(c, ' ' | '-')).collect::<String>().to_lowercase();
    let wanted = squash(&key.family);
    fonts
        .faces()
        .filter(|face| {
            face.families.iter().any(|(name, _)| squash(name) == wanted)
                || squash(&face.post_script_name) == wanted
        })
        // Prefer the matching style, then the nearest weight.
        .min_by_key(|face| (face.style != style, face.weight.0.abs_diff(key.weight)))
        .map(|face| face.id)
} // fn find_face

/// @brief Escape a family name for a double-quoted CSS string.
fn css_string(value: &str) -> String {
    value.replace('\\', "\\\\").replace('"', "\\\"")
} // fn css_string

/// @brief Subset face `index` of `data` to `glyphs`, with a Unicode `cmap`.
/// @param glyphs sorted, unique glyph ids; the first must be 0 (.notdef).
fn subset_font(data: &[u8], index: u32, glyphs: &[u16]) -> Result<Vec<u8>, String> {
    let font = allsorts::binary::read::ReadScope::new(data)
        .read::<allsorts::font_data::FontData<'_>>()
        .map_err(|e| e.to_string())?;
    let provider = font.table_provider(index as usize).map_err(|e| e.to_string())?;
    allsorts::subset::subset(
        &provider,
        glyphs,
        &allsorts::subset::SubsetProfile::Minimal,
        allsorts::subset::CmapTarget::Unicode,
    )
    .map_err(|e| e.to_string())
} // fn subset_font

/// @brief Build one `@font-face` rule holding a subset of the face in `data`.
/// @return The rule, or a user-facing reason why the face cannot be embedded.
fn font_face_rule(
    data: &[u8],
    index: u32,
    key: &FontKey,
    chars: &BTreeSet<char>,
) -> Result<String, String> {
    let face = ttf_parser::Face::parse(data, index)
        .map_err(|e| format!("Font '{}' could not be read ({e}); its label text is not embedded.", key.family))?;
    if face.permissions() == Some(ttf_parser::Permissions::Restricted) || !face.is_subsetting_allowed() {
        // The font's license forbids embedding: never override it.
        return Err(format!(
            "Font '{}' does not allow embedding; label text names the font but does not carry it.",
            key.family
        ));
    } // if restricted

    // Glyph 0 (.notdef) must stay; then every glyph the labels use.
    let mut glyphs: Vec<u16> = vec![0];
    glyphs.extend(chars.iter().filter_map(|c| face.glyph_index(*c)).map(|g| g.0));
    glyphs.sort_unstable();
    glyphs.dedup();
    let subset = subset_font(data, index, &glyphs)
        .map_err(|e| format!("Font '{}' could not be subset ({e}); its label text is not embedded.", key.family))?;

    // CFF outlines are OpenType; glyf outlines are TrueType.
    let is_cff = face.raw_face().table(ttf_parser::Tag::from_bytes(b"CFF ")).is_some()
        || face.raw_face().table(ttf_parser::Tag::from_bytes(b"CFF2")).is_some();
    let (mime, format) = if is_cff { ("font/otf", "opentype") } else { ("font/ttf", "truetype") };
    let encoded = base64::engine::general_purpose::STANDARD.encode(subset);
    Ok(format!(
        "@font-face {{ font-family: \"{}\"; font-weight: {}; font-style: {}; src: url(data:{mime};base64,{encoded}) format(\"{format}\"); }}\n",
        css_string(&key.family),
        key.weight,
        key.css_style(),
    ))
} // fn font_face_rule

/// @brief Append `css` to a `<style>` in the root `<defs>`, creating both when needed.
fn insert_style(root: &mut Element, css: String) {
    let defs_index = root
        .children
        .iter()
        .position(|n| matches!(n, XMLNode::Element(e) if e.name == "defs"));
    let defs_index = match defs_index {
        Some(i) => i, // reuse the existing <defs>
        None => {
            // No <defs>: add one first, so styles precede everything they style.
            let defs = new_element("defs", root);
            root.children.insert(0, XMLNode::Element(defs));
            0
        } // None
    }; // match defs
    let mut style = new_element("style", root);
    style.attributes.insert("type".into(), "text/css".into());
    style.children.push(XMLNode::Text(css));
    if let XMLNode::Element(defs) = &mut root.children[defs_index] {
        defs.children.push(XMLNode::Element(style));
    } // if defs element
} // fn insert_style

/// @brief Mode 1 entry point: embed every font the label text uses, as found in `fonts`.
pub(crate) fn embed_designer_fonts(root: &mut Element, fonts: &fontdb::Database) -> LabelTextReport {
    let mut report = LabelTextReport::default();

    // Collect the characters each face draws.
    let mut usage: BTreeMap<FontKey, BTreeSet<char>> = BTreeMap::new();
    let mut unnamed = 0usize;
    for_each_label_text(root, &mut |text, style| {
        report.text_elements += 1;
        match FontKey::from_style(style) {
            Some(key) => usage.entry(key).or_default().extend(text_content(text).chars()),
            None => unnamed += 1, // no font-family: the viewer's default font applies
        } // match key
    });
    if unnamed > 0 {
        report.warnings.push(format!("{unnamed} label line(s) name no font; they use the viewer's default font."));
    } // if unnamed

    // One @font-face per face; a face that cannot be embedded adds a warning instead.
    let mut css = String::new();
    for (key, chars) in &usage {
        let Some(id) = find_face(fonts, key) else {
            report.warnings.push(format!(
                "Font '{}' is not installed; label text names the font but does not carry it.",
                key.family
            ));
            continue;
        };
        let rule = fonts
            .with_face_data(id, |data, index| font_face_rule(data, index, key, chars))
            .unwrap_or_else(|| Err(format!("Font '{}' could not be loaded; its label text is not embedded.", key.family)));
        match rule {
            Ok(rule) => css.push_str(&rule),       // embedded
            Err(msg) => report.warnings.push(msg), // named but not carried
        } // match rule
    } // for key

    if !css.is_empty() {
        insert_style(root, css);
    } // if any rule
    report
} // fn embed_designer_fonts

/// @brief Mode 2 entry point: set label text to the bundled single-line font and embed it.
pub(crate) fn apply_single_line_font(root: &mut Element) -> LabelTextReport {
    let mut report = LabelTextReport::default();
    let mut chars: BTreeSet<char> = BTreeSet::new();

    for_each_label_text(root, &mut |text, _| {
        report.text_elements += 1;
        chars.extend(text_content(text).chars());
        // The font replaces the designer's face; weight and slant no longer apply.
        text.attributes.insert("font-family".into(), SINGLE_LINE_FONT_FAMILY.into());
        text.attributes.insert("font-weight".into(), "400".into());
        text.attributes.insert("font-style".into(), "normal".into());
    });
    if report.text_elements == 0 {
        return report; // nothing uses the font: do not embed it
    } // if no text

    // Characters the single-line font cannot draw fall back to the viewer's font.
    if let Ok(face) = ttf_parser::Face::parse(SINGLE_LINE_FONT, 0) {
        let missing = chars.iter().filter(|c| !c.is_whitespace() && face.glyph_index(**c).is_none()).count();
        if missing > 0 {
            report.warnings.push(format!(
                "{missing} distinct label character(s) are not in {SINGLE_LINE_FONT_FAMILY}; viewers draw them in another font."
            ));
        } // if missing
    } // if face parsed

    let key = FontKey { family: SINGLE_LINE_FONT_FAMILY.into(), weight: 400, italic: false };
    match font_face_rule(SINGLE_LINE_FONT, 0, &key, &chars) {
        Ok(rule) => insert_style(root, rule),    // embedded
        Err(msg) => report.warnings.push(msg),   // bundled font unreadable: should not happen
    } // match rule
    report
} // fn apply_single_line_font
