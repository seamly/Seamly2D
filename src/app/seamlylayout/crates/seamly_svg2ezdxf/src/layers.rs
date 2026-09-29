// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Layer mapping from SVG elements to ASTM D6673-10 numeric layers.
//!
//! | Layer | Purpose | SVG `data-type` |
//! |-------|---------|-----------------|
//! | 1     | Piece boundary | `cutline` |
//! | 2, 3  | Turn / curve points | generated from contours |
//! | 4, 80, 81, 83 | Notches | `notch` |
//! | 7     | Grainline | `grainline` |
//! | 8     | Internal lines (also the default) | `internal_path` |
//! | 11    | Internal cutouts | `cut_path` |
//! | 13    | Drill holes | id hint only |
//! | 14    | Sew lines | `seamline` |
//! | 15    | Annotation text | `piece_label`, `pattern_label`, `<text>` |
//! | 84–87 | Quality validation curves | generated from contours |

/// @brief Component kinds of a pattern piece, as tagged by Seamly2D.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PieceComponent {
    Cutline,
    Seamline,
    Notch,
    Grainline,
    InternalPath,
    CutPath,
    Label,
    DrillHole,
}

impl PieceComponent {
    /// @brief ASTM layer for this component.
    pub fn layer(self) -> &'static str {
        match self {
            PieceComponent::Cutline => "1",
            PieceComponent::Notch => "4",
            PieceComponent::Grainline => "7",
            PieceComponent::InternalPath => "8",
            PieceComponent::CutPath => "11",
            PieceComponent::DrillHole => "13",
            PieceComponent::Seamline => "14",
            PieceComponent::Label => "15",
        } // match component
    } // fn layer
} // impl PieceComponent

/// @brief Identify the piece component an element stands for.
/// @details `data-type` decides when present, so a piece named "Buttonhole"
///          does not become a drill hole. Untagged SVGs fall back to id hints.
/// @param element The SVG element.
/// @return The component, or None for pieces, patterns and plain containers.
pub fn piece_component(element: &xmltree::Element) -> Option<PieceComponent> {
    if let Some(data_type) = element.attributes.get("data-type") {
        return match data_type.as_str() {
            "cutline" => Some(PieceComponent::Cutline),
            "seamline" => Some(PieceComponent::Seamline),
            "notch" => Some(PieceComponent::Notch),
            "grainline" => Some(PieceComponent::Grainline),
            "internal_path" => Some(PieceComponent::InternalPath),
            "cut_path" => Some(PieceComponent::CutPath),
            "piece_label" | "pattern_label" => Some(PieceComponent::Label),
            _ => None,
        }; // match data-type
    } // if data-type

    let id = element.attributes.get("id")?.to_lowercase();
    if id.contains("cut_path") {
        Some(PieceComponent::CutPath)
    } else if id.contains("cutline") || id.contains("boundary") {
        Some(PieceComponent::Cutline)
    } else if id.contains("notch") {
        Some(PieceComponent::Notch)
    } else if id.contains("grain") {
        Some(PieceComponent::Grainline)
    } else if id.contains("seam") {
        Some(PieceComponent::Seamline)
    } else if id.contains("drill") || id.contains("hole") {
        Some(PieceComponent::DrillHole)
    } else if id.contains("tuck") || id.contains("internal_path") {
        Some(PieceComponent::InternalPath)
    } else if id.contains("label") {
        Some(PieceComponent::Label)
    } else {
        None
    } // if id hint
} // fn piece_component

/// @brief Map an SVG element to an ASTM D6673-10 numeric layer string.
/// @param element The SVG element to map.
/// @return The component layer, "15" for `<text>`, else "8" (internal lines).
pub fn map_svg_to_astm_layer(element: &xmltree::Element) -> &'static str {
    if let Some(component) = piece_component(element) {
        return component.layer();
    } // if component
    match element.name.as_str() {
        "text" => "15",
        _ => "8",
    } // match element name
} // fn map_svg_to_astm_layer
