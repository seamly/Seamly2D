// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Drawing and block structures for ezdxf-like intermediate representation.

use crate::astm_contour::AstmContour;
use crate::astm_notch::Notch;
use crate::entities::{Entity, Point};

// @brief DXF version enumeration.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum DxfVersion {
    // DXF R10 (AC1006): the DXF-AAMA export.
    R10,
    // DXF R12 (AC1009): the widest importer support.
    #[default]
    R12,
    // DXF R13 (AC1012): the version ASTM D6673 §1.2 names.
    R13,
}

// @brief Annotation text of a piece (DXF layer 15), in DXF units.
#[derive(Debug, Clone, PartialEq)]
pub struct Annotation {
    // Insertion point.
    pub position: Point,
    // Text height.
    pub height: f64,
    // Rotation in degrees counter-clockwise.
    pub rotation: f64,
    // Text content (ASCII-only).
    pub text: String,
}

// @brief `Material:` value until the handoff SVG carries a per-piece material.
pub const DEFAULT_MATERIAL: &str = "Fabric";

// @brief A `# n` grade rule identifier text (D6673 §4.3.1.4), written in the sample size block.
#[derive(Debug, Clone, PartialEq)]
pub struct RuleText {
    // XY of the graded point or notch base the text belongs to.
    pub position: Point,
    // Layer of that point: 1, 4, 8, 11 or 14.
    pub layer: &'static str,
    // Grade rule identifier, a key of `GradeRuleTable::rules`.
    pub rule: i32,
}

// @brief Grade data of one sample size block (D6673 §4.3.1.4).
#[derive(Debug, Clone, PartialEq)]
pub struct BlockGrading {
    // Grade reference line (layer 5): start and end. Gives the X axis of the rules.
    pub reference_line: (Point, Point),
    // `# n` texts, one per graded key point and notch.
    pub rule_texts: Vec<RuleText>,
}

// @brief One DELTA grade rule (D6673 §4.4.2).
#[derive(Debug, Clone, PartialEq)]
pub struct GradeRule {
    // Grade rule identifier, written as `# n` text.
    pub id: i32,
    // X and Y growth from the sample size, one pair per size in `GradeRuleTable::sizes`
    // order, in DXF units. The sample size pair is (0, 0).
    pub deltas: Vec<(f64, f64)>,
}

// @brief Grade rule table of a graded export, written as the `.rul` file beside the DXF.
#[derive(Debug, Clone, PartialEq, Default)]
pub struct GradeRuleTable {
    // Table name: `Grade Rule Table:` style text and the file's `GRADE RULE TABLE:`.
    pub name: String,
    // Sample size name: `Sample Size:` style text.
    pub sample_size: String,
    // Every size, in grading order.
    pub sizes: Vec<String>,
    // Rules, in identifier order.
    pub rules: Vec<GradeRule>,
}

// @brief Block definition: one pattern piece.
//
// `entities` holds the generic conversion of every SVG element. The ASTM fields
// hold the same piece classified by `data-type`, in DXF units; the DXF-ASTM
// writer uses them whenever `boundary` is present.
pub struct Block {
    // Block name (ASCII-only, sanitized, with "_M" suffix).
    pub name: String,
    // All converted entities (generic, used for validation and inspection).
    pub entities: Vec<Box<dyn Entity>>,
    // `Piece Name:` system text: `display_name` with each non-ASCII character as an AutoCAD `\U+XXXX` escape.
    pub piece_name: String,
    // Piece name for dialogs and warnings: piece `data-name` as written, else the block name.
    // A name that several pieces share ends in the piece letter or number: "Detail (3)".
    pub display_name: String,
    // Piece `data-parent`: the piece-set id in a multisize handoff. Groups the sizes of one piece.
    pub parent: Option<String>,
    // `Quantity:` system text in "R,L" form, from `data-quantity` or a "Cut N" label line.
    pub quantity: Option<String>,
    // `Size Name:` system text: the piece `data-size`, else the pattern sample size.
    pub size: Option<String>,
    // `Material:` system text: the material the piece is cut from.
    pub material: String,
    // Piece boundary (layer 1 / 84).
    pub boundary: Option<AstmContour>,
    // Sew lines (layer 14 / 87).
    pub sew_lines: Vec<AstmContour>,
    // Internal lines (layer 8 / 85).
    pub internal_lines: Vec<AstmContour>,
    // Internal cutouts (layer 11 / 86).
    pub cutouts: Vec<AstmContour>,
    // Grainline start and end points (layer 7), if present.
    pub grainline: Option<(Point, Point)>,
    // Mirror line (layer 6): the fold line of a piece cut on the fold. After
    // `align_mirror_lines` its points are the first two boundary key points.
    pub mirror_line: Option<(Point, Point)>,
    // Notches (layers 4, 80, 81, 83).
    pub notches: Vec<Notch>,
    // Annotation text (layer 15).
    pub annotations: Vec<Annotation>,
    // Grade data; None for an ungraded piece.
    pub grading: Option<BlockGrading>,
}

impl Block {
    // @brief Create a new empty block.
    // @param name Block name (should already include "_M" suffix).
    // @return New block whose piece name is the block name.
    pub fn new(name: String) -> Self {
        Self {
            piece_name: name.clone(),
            display_name: name.clone(),
            parent: None,
            name,
            entities: Vec::new(),
            quantity: None,
            size: None,
            material: DEFAULT_MATERIAL.to_string(),
            boundary: None,
            sew_lines: Vec::new(),
            internal_lines: Vec::new(),
            cutouts: Vec::new(),
            grainline: None,
            mirror_line: None,
            notches: Vec::new(),
            annotations: Vec::new(),
            grading: None,
        }
    } // fn new

    // @brief Add an entity to this block.
    // @param entity Entity to add.
    pub fn add_entity(&mut self, entity: Box<dyn Entity>) {
        self.entities.push(entity);
    } // fn add_entity
} // impl Block

// @brief Downstream CAD data a piece lacks in a DXF-ASTM export.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MissingPieceData {
    // Display name of the piece (`Block::display_name`).
    pub piece_name: String,
    // No `data-quantity` and no "Cut N" label line, so no `Quantity:` system text.
    pub quantity: bool,
    // No label text (layer 15).
    pub label: bool,
    // No grainline (layer 7).
    pub grainline: bool,
}

// @brief Drawing (ezdxf-like intermediate representation).
pub struct Drawing {
    // DXF version.
    pub version: DxfVersion,
    // Block definitions (pattern pieces).
    pub blocks: Vec<Block>,
    // Entities in modelspace (not in blocks).
    pub modelspace_entities: Vec<Box<dyn Entity>>,
    // Pattern name from the `data-type="pattern"` group, if the SVG has one.
    pub style_name: Option<String>,
    // Grade rule table of a graded export; None for an ungraded export.
    pub grade_rule_table: Option<GradeRuleTable>,
}

impl Drawing {
    // @brief Create a new drawing.
    // @param version DXF version.
    // @return New drawing.
    pub fn new(version: DxfVersion) -> Self {
        Self {
            version,
            blocks: Vec::new(),
            modelspace_entities: Vec::new(),
            style_name: None,
            grade_rule_table: None,
        }
    }

    // @brief Add a block to the drawing.
    // @param block Block to add.
    pub fn add_block(&mut self, block: Block) {
        self.blocks.push(block);
    }

    // @brief Add an entity to modelspace.
    // @param entity Entity to add.
    pub fn add_modelspace_entity(&mut self, entity: Box<dyn Entity>) {
        self.modelspace_entities.push(entity);
    }

    // @brief Pieces that lack a `Quantity:` source, label text, or a grainline.
    // @details Only pieces with a boundary count: the DXF-ASTM writer writes
    //          system text, labels and grainlines for those pieces only.
    // @return One entry per incomplete piece, in first-block order. The sizes of a
    //         multisize piece share one display name and give one entry: a size
    //         that lacks an item marks it missing for the piece.
    pub fn missing_piece_data(&self) -> Vec<MissingPieceData> {
        let mut pieces: Vec<MissingPieceData> = Vec::new();
        for b in self.blocks.iter().filter(|b| b.boundary.is_some()) {
            let entry = MissingPieceData {
                piece_name: b.display_name.clone(),
                quantity: b.quantity.is_none(),
                label: b.annotations.is_empty(),
                grainline: b.grainline.is_none(),
            }; // entry
            // Merge every size of a piece into its first entry.
            match pieces.iter_mut().find(|m| m.piece_name == entry.piece_name) {
                Some(m) => {
                    m.quantity |= entry.quantity;
                    m.label |= entry.label;
                    m.grainline |= entry.grainline;
                } // Some: another size
                None => pieces.push(entry),
            } // match piece
        } // for each piece block
        pieces.retain(|m| m.quantity || m.label || m.grainline);
        pieces
    } // fn missing_piece_data

    // @brief Write drawing to a human-readable text file for inspection.
    // @param file_path Path to write the file.
    // @return Result indicating success or error.
    pub fn write_to_file(&self, file_path: impl AsRef<std::path::Path>) -> std::io::Result<()> {
        use std::io::Write;

        let mut file = std::fs::File::create(file_path)?;

        writeln!(file, "// SeamlyLayout ezdxf Drawing Inspection File")?;
        writeln!(file, "// Generated by seamly_svg2ezdxf")?;
        writeln!(file, "")?;

        writeln!(file, "DXF Version: {:?}", self.version)?;
        writeln!(file, "Blocks: {}", self.blocks.len())?;
        writeln!(
            file,
            "Modelspace Entities: {}",
            self.modelspace_entities.len()
        )?;
        writeln!(file, "")?;

        if !self.blocks.is_empty() {
            writeln!(file, "=== BLOCKS ===")?;
            for (i, block) in self.blocks.iter().enumerate() {
                writeln!(file, "Block {}: '{}'", i, block.name)?;
                writeln!(file, "  Entity count: {}", block.entities.len())?;
                for (j, entity) in block.entities.iter().enumerate() {
                    writeln!(
                        file,
                        "  Entity {}: {} on layer '{}'",
                        j,
                        entity.entity_type(),
                        entity.layer()
                    )?;
                }
                writeln!(file, "")?;
            }
        }

        if !self.modelspace_entities.is_empty() {
            writeln!(file, "=== MODELSPACE ENTITIES ===")?;
            for (i, entity) in self.modelspace_entities.iter().enumerate() {
                writeln!(
                    file,
                    "Entity {}: {} on layer '{}'",
                    i,
                    entity.entity_type(),
                    entity.layer()
                )?;
            }
        }

        Ok(())
    }
}
