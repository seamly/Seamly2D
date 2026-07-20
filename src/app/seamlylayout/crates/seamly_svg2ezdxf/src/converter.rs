// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief SVG to ezdxf conversion logic.

use crate::drawing::{Block, Drawing, DxfVersion};
use crate::entities::{Circle, Entity, Line, Point, Polyline, Text};
use crate::error::Result;
use crate::layers::map_svg_to_astm_layer;
use crate::utils::{detect_corners, invert_y_axis, parse_float_attr, sanitize_ascii, sanitize_block_name};
use std::any::Any;
use geometry::Path;
use xmltree::{Element, XMLNode};

// @brief Conversion options for SVG to ezdxf.
#[derive(Debug, Clone)]
pub struct SvgToEzdxfOptions {
    // Target DXF version (default: R12 for ASTM).
    pub dxf_version: DxfVersion,
    // Whether to create blocks for pattern pieces.
    pub create_blocks: bool,
    // Coordinate system transformation (Y-axis inversion).
    pub invert_y: bool,
    // SVG height for Y-axis inversion (required if invert_y is true).
    pub svg_height: Option<f64>,
    // Flattening tolerance for curves (in SVG units).
    pub flatten_tolerance: f64,
}

impl Default for SvgToEzdxfOptions {
    fn default() -> Self {
        Self {
            dxf_version: DxfVersion::R12,
            create_blocks: true,
            invert_y: true,
            svg_height: None,
            flatten_tolerance: 0.1,
        }
    }
}

// @brief Convert SVG Document to ezdxf Drawing.
// @param doc The SVG DOM document to convert.
// @param options Conversion options.
// @return Drawing object ready for DXF export.
pub fn svg_to_ezdxf(doc: &svg_dom::Document, options: &SvgToEzdxfOptions) -> Result<Drawing> {
    println!("[CONVERTER] svg_to_ezdxf: Starting conversion");
    println!("  └─ Step 1: Getting root element");
    let root = &doc.root;
    println!("    • Root element name: '{}'", root.name);
    println!("    • Root children count: {}", root.children.len());

    println!("  └─ Step 2: Determining SVG height for coordinate transformation");
    // Get SVG dimensions for coordinate transformation.
    let svg_height = if options.invert_y {
        let height = options.svg_height.unwrap_or_else(|| {
            println!("    • SVG height not in options, trying to get from root element");
            // Try to get height from SVG root element.
            let h = parse_float_attr(
                doc.root.attributes.get("height"),
                100.0, // Default height if not specified.
            );
            println!("    • SVG height from root element: {}", h);
            h
        });
        println!("    • SVG height: {} (Y-axis inversion enabled)", height);
        height
    } else {
        println!("    • SVG height: N/A (Y-axis inversion disabled)");
        0.0 // Not used if not inverting.
    };

    println!("  └─ Step 3: Creating new Drawing object");
    // Create new drawing.
    let mut drawing = Drawing::new(options.dxf_version);
    println!(
        "    • Drawing created with DXF version: {:?}",
        drawing.version
    );
    println!("    • Initial blocks count: {}", drawing.blocks.len());
    println!(
        "    • Initial modelspace entities count: {}",
        drawing.modelspace_entities.len()
    );

    println!("  └─ Step 4: Processing elements");
    // If creating blocks, extract pattern pieces (top-level <g> elements with IDs).
    if options.create_blocks {
        println!("    • Mode: Pattern piece extraction (create_blocks=true)");
        println!("    • Will create blocks for <g> elements with IDs");
        println!("    • Non-group elements will go to modelspace");
        extract_pattern_pieces(root, &mut drawing, options, svg_height)?;
    } else {
        println!("    • Mode: Direct modelspace conversion (create_blocks=false)");
        println!("    • All elements will go to modelspace");
        // Convert all elements directly to modelspace entities.
        convert_elements_to_modelspace(root, &mut drawing, options, svg_height)?;
    }

    println!("  └─ Step 5: Conversion complete");
    println!("    • Final blocks count: {}", drawing.blocks.len());
    println!(
        "    • Final modelspace entities count: {}",
        drawing.modelspace_entities.len()
    );
    println!("[CONVERTER] svg_to_ezdxf: Conversion finished successfully");

    Ok(drawing)
}

// @brief Extract pattern pieces (top-level <g> elements) and create blocks.
// @param root SVG root element.
// @param drawing Drawing to add blocks to.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
fn extract_pattern_pieces(
    root: &Element,
    drawing: &mut Drawing,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<()> {
    println!("[CONVERTER] extract_pattern_pieces: Starting pattern piece extraction");
    println!("  └─ Root children count: {}", root.children.len());

    let mut group_count = 0;
    let mut block_count = 0;
    let mut non_group_count = 0;

    // Iterate through direct children of root.
    for (i, child) in root.children.iter().enumerate() {
        println!(
            "  └─ Processing child {} of {}...",
            i + 1,
            root.children.len()
        );

        if let XMLNode::Element(element) = child {
            println!("    • Child is an element: '{}'", element.name);

            // Look for <g> elements with id attributes (pattern pieces).
            if element.name == "g" {
                group_count += 1;
                println!("    • Element is a GROUP (<g>)");

                if let Some(piece_id) = element.attributes.get("id") {
                    println!("    • Group has ID: '{}'", piece_id);
                    block_count += 1;

                    // Create a block for this pattern piece.
                    // Append "_M" suffix to match CLO3D / seamly2clo.py block naming.
                    let block_name = format!("{}_M", sanitize_block_name(piece_id));
                    println!(
                        "    • Creating block '{}' (sanitized from '{}')",
                        block_name, piece_id
                    );
                    let mut block = Block::new(block_name.clone());
                    println!(
                        "    • Block created, initial entity count: {}",
                        block.entities.len()
                    );

                    // Convert all elements within this pattern piece to entities.
                    println!("    • Converting elements within block '{}'...", block_name);
                    convert_element_tree(&element, &mut block, options, svg_height, None)?;
                    println!(
                        "    • Block '{}' conversion complete, entity count: {}",
                        block_name,
                        block.entities.len()
                    );

                    if block.entities.is_empty() {
                        println!("    ⚠️  WARNING: Block '{}' has no entities!", block_name);
                    } else {
                        println!(
                            "    ✅ Block '{}' has {} entities",
                            block_name,
                            block.entities.len()
                        );
                    }

                    // Extract CLO3D semantic fields from entities.
                    // Scan entities for boundary (layer "1"), grainline (layer "7"),
                    // and notches (layer "4") to populate the structured block fields
                    // used by write_blocks_section to produce the CLO3D DXF format.
                    extract_semantic_fields(&mut block);

                    // Add block to drawing.
                    println!("    • Adding block '{}' to drawing...", block_name);
                    drawing.add_block(block);
                    println!(
                        "    • Block '{}' added. Total blocks: {}",
                        block_name,
                        drawing.blocks.len()
                    );
                } else {
                    println!("    • Group has no ID attribute (skipping block creation)");
                    println!("    • Converting group children to modelspace instead...");
                    convert_element_tree(&element, drawing, options, svg_height, None)?;
                }
            } else {
                non_group_count += 1;
                println!("    • Element is NOT a group: '{}'", element.name);
                println!("    • Converting to modelspace entity...");
                convert_element_tree(&element, drawing, options, svg_height, None)?;
                println!("    • Element '{}' processed", element.name);
            }
        } else {
            println!("    • Child is not an element (skipping)");
        }
    }

    println!("[CONVERTER] extract_pattern_pieces: Complete");
    println!(
        "  └─ Summary: {} groups found, {} blocks created, {} non-group elements processed",
        group_count, block_count, non_group_count
    );
    println!("  └─ Final blocks count: {}", drawing.blocks.len());
    println!(
        "  └─ Final modelspace entities count: {}",
        drawing.modelspace_entities.len()
    );

    Ok(())
}

// @brief Convert all elements directly to modelspace (no blocks).
// @param root SVG root element.
// @param drawing Drawing to add entities to.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
fn convert_elements_to_modelspace(
    root: &Element,
    drawing: &mut Drawing,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<()> {
    println!("[CONVERTER] convert_elements_to_modelspace: Starting conversion");
    println!("  └─ Root element: '{}'", root.name);
    println!("  └─ Children count: {}", root.children.len());
    println!(
        "  └─ Modelspace entities before: {}",
        drawing.modelspace_entities.len()
    );

    // Recursively convert all elements.
    convert_element_tree(root, drawing, options, svg_height, None)?;

    println!(
        "  └─ Modelspace entities after: {}",
        drawing.modelspace_entities.len()
    );
    println!("[CONVERTER] convert_elements_to_modelspace: Complete");

    Ok(())
}

// @brief Recursively convert SVG elements to DXF entities.
// @param element SVG element to convert.
// @param target Target to add entities to (Block or Drawing).
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @param parent_layer Optional layer name from parent group (for layer inheritance).
fn convert_element_tree(
    element: &Element,
    target: &mut dyn EntityTarget,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    parent_layer: Option<&str>,
) -> Result<()> {
    println!(
        "[CONVERTER] convert_element_tree: Processing element '{}'",
        element.name
    );
    if let Some(id) = element.attributes.get("id") {
        println!("  └─ Element ID: '{}'", id);
    } else {
        println!("  └─ Element ID: <none>");
    }
    println!("  └─ Parent layer: {:?}", parent_layer);
    println!("  └─ Children count: {}", element.children.len());
    // Determine layer for this element (check parent first, then element itself).
    // Note: Root <svg> element should not pass a layer to its children.
    println!("  └─ Step: Determining layer for element");
    let current_layer = parent_layer
        .or_else(|| {
            // Check if this element has an ID that suggests a layer.
            element.attributes.get("id").map(|id| {
                println!("    • Checking element ID '{}' for layer hints", id);
                let id_lower = id.to_lowercase();
                let layer = if id_lower.contains("cutline") || id_lower.contains("boundary") {
                    println!("    • ID contains 'cutline' or 'boundary' → layer: '1' (Piece boundary)");
                    "1"
                } else if id_lower.contains("notch") {
                    println!("    • ID contains 'notch' → layer: '4' (Notches)");
                    "4"
                } else if id_lower.contains("grainline") || id_lower.contains("grain") {
                    println!("    • ID contains 'grainline' or 'grain' → layer: '7' (Grain line)");
                    "7"
                } else if id_lower.contains("seamline") || id_lower.contains("seam") {
                    println!("    • ID contains 'seamline' or 'seam' → layer: '14' (Sew lines)");
                    "14"
                } else if id_lower.contains("drill") || id_lower.contains("hole") {
                    println!("    • ID contains 'drill' or 'hole' → layer: '13' (Drill holes)");
                    "13"
                } else if id_lower.contains("tuck") {
                    println!("    • ID contains 'tuck' → layer: '8' (Internal lines / tuck construction marks)");
                    "8"
                } else {
                    let default_layer = map_svg_to_astm_layer(element);
                    println!(
                        "    • No layer hints in ID → using default layer: '{}'",
                        default_layer
                    );
                    default_layer
                };
                layer
            })
        })
        .unwrap_or_else(|| {
            let default_layer = map_svg_to_astm_layer(element);
            println!(
                "    • No parent layer, no element ID → using default layer: '{}'",
                default_layer
            );
            default_layer
        });

    if let Some(parent) = parent_layer {
        println!(
            "  └─ Using parent layer: '{}' (overriding element layer)",
            parent
        );
    } else {
        println!("  └─ Using element layer: '{}'", current_layer);
    }

    // Convert this element based on its type.
    println!(
        "  └─ Step: Converting element based on type '{}'",
        element.name
    );
    match element.name.as_str() {
        "line" => {
            eprintln!("    • Element type: LINE");
            eprintln!("    • Calling convert_line()...");
            if let Some(mut line_entity) = convert_line(element, options, svg_height)? {
                eprintln!("    • LINE conversion: SUCCESS");
                eprintln!("    • LINE layer before override: '{}'", line_entity.layer);
                // Override layer if parent provided one.
                if let Some(parent) = parent_layer {
                    eprintln!("    • Overriding layer with parent layer: '{}'", parent);
                    line_entity.layer = parent.to_string();
                }
                eprintln!(
                    "    • Adding LINE entity to target with layer: '{}'",
                    line_entity.layer
                );
                target.add_entity(Box::new(line_entity));
                eprintln!("    • LINE entity added successfully");
            } else {
                eprintln!("    • LINE conversion: FAILED (returned None)");
            }
        }
        "circle" => {
            eprintln!("    • Element type: CIRCLE");
            eprintln!("    • Calling convert_circle()...");
            if let Some(mut circle_entity) = convert_circle(element, options, svg_height)? {
                eprintln!("    • CIRCLE conversion: SUCCESS");
                eprintln!(
                    "    • CIRCLE layer before override: '{}'",
                    circle_entity.layer
                );
                // Override layer if parent provided one.
                if let Some(parent) = parent_layer {
                    eprintln!("    • Overriding layer with parent layer: '{}'", parent);
                    circle_entity.layer = parent.to_string();
                }
                eprintln!(
                    "    • Adding CIRCLE entity to target with layer: '{}'",
                    circle_entity.layer
                );
                target.add_entity(Box::new(circle_entity));
                eprintln!("    • CIRCLE entity added successfully");
            } else {
                eprintln!("    • CIRCLE conversion: FAILED (returned None)");
            }
        }
        "text" => {
            eprintln!("    • Element type: TEXT");
            eprintln!("    • Calling convert_text()...");
            if let Some(mut text_entity) = convert_text(element, options, svg_height)? {
                eprintln!("    • TEXT conversion: SUCCESS");
                eprintln!(
                    "    • TEXT layer from convert_text(): '{}'",
                    text_entity.layer
                );
                // Text elements always use ASTM layer "9" (Text/Annotations),
                // regardless of parent group layer.
                eprintln!(
                    "    • Text elements always use ASTM layer '9' (ignoring parent layer)"
                );
                text_entity.layer = "9".to_string();
                eprintln!(
                    "    • Adding TEXT entity to target with layer: '{}'",
                    text_entity.layer
                );
                target.add_entity(Box::new(text_entity));
                eprintln!("    • TEXT entity added successfully");
            } else {
                eprintln!("    • TEXT conversion: FAILED (returned None)");
                eprintln!("    • Possible reasons: empty text content, parsing error");
            }
        }
        "path" => {
            eprintln!("    • Element type: PATH");
            eprintln!("    • Calling convert_path()...");
            if let Some(mut polyline_entity) = convert_path(element, options, svg_height)? {
                eprintln!("    • PATH conversion: SUCCESS");
                eprintln!(
                    "    • POLYLINE layer before override: '{}'",
                    polyline_entity.layer
                );
                // Override layer if parent provided one.
                if let Some(parent) = parent_layer {
                    eprintln!("    • Overriding layer with parent layer: '{}'", parent);
                    polyline_entity.layer = parent.to_string();
                }
                eprintln!(
                    "    • Adding POLYLINE entity to target with layer: '{}'",
                    polyline_entity.layer
                );
                eprintln!(
                    "    • POLYLINE has {} vertices, closed: {}",
                    polyline_entity.vertices.len(),
                    polyline_entity.closed
                );
                target.add_entity(Box::new(polyline_entity));
                eprintln!("    • POLYLINE entity added successfully");
            } else {
                eprintln!("    • PATH conversion: FAILED (returned None)");
                eprintln!("    • Possible reasons: empty path data, parsing error");
            }
        }
        "polyline" => {
            eprintln!("    • Element type: POLYLINE");
            eprintln!("    • Calling convert_polyline()...");
            if let Some(mut polyline_entity) =
                convert_polyline(element, options, svg_height, parent_layer)?
            {
                eprintln!("    • POLYLINE conversion: SUCCESS");
                eprintln!(
                    "    • POLYLINE layer before override: '{}'",
                    polyline_entity.layer
                );
                // Override layer if parent provided one.
                if let Some(parent) = parent_layer {
                    eprintln!("    • Overriding layer with parent layer: '{}'", parent);
                    polyline_entity.layer = parent.to_string();
                }
                eprintln!(
                    "    • Adding POLYLINE entity to target with layer: '{}'",
                    polyline_entity.layer
                );
                eprintln!(
                    "    • POLYLINE has {} vertices, closed: {}",
                    polyline_entity.vertices.len(),
                    polyline_entity.closed
                );
                target.add_entity(Box::new(polyline_entity));
                eprintln!("    • POLYLINE entity added successfully");
            } else {
                eprintln!("    • POLYLINE conversion: FAILED (returned None)");
                eprintln!("    • Possible reasons: empty points attribute, invalid points");
            }
        }
        "polygon" => {
            eprintln!("    • Element type: POLYGON");
            eprintln!("    • Calling convert_polygon()...");
            if let Some(mut polyline_entity) =
                convert_polygon(element, options, svg_height, parent_layer)?
            {
                eprintln!("    • POLYGON conversion: SUCCESS");
                eprintln!(
                    "    • POLYLINE layer before override: '{}'",
                    polyline_entity.layer
                );
                // Override layer if parent provided one.
                if let Some(parent) = parent_layer {
                    eprintln!("    • Overriding layer with parent layer: '{}'", parent);
                    polyline_entity.layer = parent.to_string();
                }
                eprintln!(
                    "    • Adding POLYLINE entity to target with layer: '{}'",
                    polyline_entity.layer
                );
                eprintln!(
                    "    • POLYLINE has {} vertices, closed: {}",
                    polyline_entity.vertices.len(),
                    polyline_entity.closed
                );
                target.add_entity(Box::new(polyline_entity));
                eprintln!("    • POLYLINE entity added successfully");
            } else {
                eprintln!("    • POLYGON conversion: FAILED (returned None)");
                eprintln!("    • Possible reasons: empty points attribute, less than 3 points");
            }
        }
        "rect" => {
            eprintln!("    • Element type: RECT");
            eprintln!("    • Calling convert_rect()...");
            if let Some(mut polyline_entity) =
                convert_rect(element, options, svg_height, parent_layer)?
            {
                eprintln!("    • RECT conversion: SUCCESS");
                eprintln!(
                    "    • POLYLINE layer before override: '{}'",
                    polyline_entity.layer
                );
                // Override layer if parent provided one.
                if let Some(parent) = parent_layer {
                    eprintln!("    • Overriding layer with parent layer: '{}'", parent);
                    polyline_entity.layer = parent.to_string();
                }
                eprintln!(
                    "    • Adding POLYLINE entity to target with layer: '{}'",
                    polyline_entity.layer
                );
                eprintln!(
                    "    • POLYLINE has {} vertices, closed: {}",
                    polyline_entity.vertices.len(),
                    polyline_entity.closed
                );
                target.add_entity(Box::new(polyline_entity));
                eprintln!("    • POLYLINE entity added successfully");
            } else {
                eprintln!("    • RECT conversion: FAILED (returned None)");
                eprintln!("    • Possible reasons: invalid width or height");
            }
        }
        "ellipse" => {
            eprintln!("    • Element type: ELLIPSE");
            eprintln!("    • Calling convert_ellipse()...");
            if let Some(entity) = convert_ellipse(element, options, svg_height, parent_layer)? {
                eprintln!("    • ELLIPSE conversion: SUCCESS");
                eprintln!(
                    "    • Entity type: {}, layer: '{}'",
                    entity.entity_type(),
                    entity.layer()
                );
                eprintln!(
                    "    • Adding entity to target with layer: '{}'",
                    entity.layer()
                );
                target.add_entity(entity);
                eprintln!("    • Entity added successfully");
            } else {
                eprintln!("    • ELLIPSE conversion: FAILED (returned None)");
                eprintln!("    • Possible reasons: invalid rx or ry");
            }
        }
        "g" => {
            eprintln!("    • Element type: GROUP (<g>)");
            eprintln!("    • Checking if group defines a layer...");
            // For group elements, check if they define a layer.
            let group_layer = element.attributes.get("id").and_then(|id| {
                eprintln!("      • Group has ID: '{}'", id);
                let id_lower = id.to_lowercase();
                let layer = if id_lower.contains("cutline") || id_lower.contains("boundary") {
                    eprintln!("      • Group ID contains 'cutline' or 'boundary' → layer: '1' (Piece boundary)");
                    Some("1")
                } else if id_lower.contains("notch") {
                    eprintln!("      • Group ID contains 'notch' → layer: '4' (Notches)");
                    Some("4")
                } else if id_lower.contains("grainline") || id_lower.contains("grain") {
                    eprintln!("      • Group ID contains 'grainline' or 'grain' → layer: '7' (Grain line)");
                    Some("7")
                } else if id_lower.contains("seamline") || id_lower.contains("seam") {
                    eprintln!("      • Group ID contains 'seamline' or 'seam' → layer: '14' (Sew lines)");
                    Some("14")
                } else if id_lower.contains("drill") || id_lower.contains("hole") {
                    eprintln!("      • Group ID contains 'drill' or 'hole' → layer: '13' (Drill holes)");
                    Some("13")
                } else if id_lower.contains("tuck") {
                    eprintln!("      • Group ID contains 'tuck' → layer: '8' (Internal lines / tuck construction marks)");
                    Some("8")
                } else {
                    eprintln!("      • Group ID does not contain layer hints");
                    None
                };
                layer
            });

            if group_layer.is_none() {
                eprintln!("      • Group has no ID or ID has no layer hints");
            }

            // Use group layer if available, otherwise inherit from parent.
            let inherited_layer = group_layer.or(parent_layer);
            eprintln!(
                "    • Inherited layer for group children: {:?}",
                inherited_layer
            );
            eprintln!(
                "    • Processing {} children of group...",
                element.children.len()
            );

            // Recursively process children with inherited layer.
            for (i, child) in element.children.iter().enumerate() {
                if let XMLNode::Element(child_element) = child {
                    eprintln!(
                        "    • Processing child {} of {}: '{}'",
                        i + 1,
                        element.children.len(),
                        child_element.name
                    );
                    convert_element_tree(
                        child_element,
                        target,
                        options,
                        svg_height,
                        inherited_layer,
                    )?;
                    eprintln!("    • Child {} processed successfully", i + 1);
                } else {
                    eprintln!("    • Child {} is not an element (skipping)", i + 1);
                }
            }
            eprintln!("    • Group processing complete");
            return Ok(());
        }
        _ => {
            eprintln!(
                "    • Element type: '{}' (not directly convertible)",
                element.name
            );
            eprintln!("    • Will process children recursively");
            // For other elements, recursively process children.
        }
    }

    // Recursively process children.
    // Important: Only pass parent layer to children if:
    // 1. We have a meaningful parent layer (from a group with layer hints), OR
    // 2. The current element is a group that defines a layer
    // Do NOT pass layer for root <svg> element or other container elements without layer meaning.
    let child_parent_layer = if element.name == "svg" {
        // Root <svg> element should not pass a layer to its children.
        eprintln!(
            "  └─ Step: Processing {} children (root <svg>, no parent layer)",
            element.children.len()
        );
        None
    } else if parent_layer.is_some() {
        // We have a parent layer from a meaningful group, pass it down.
        eprintln!(
            "  └─ Step: Processing {} children with inherited layer '{}'",
            element.children.len(),
            current_layer
        );
        Some(current_layer)
    } else if element.name == "g" {
        // This is a group that might define a layer, check if it does.
        let group_has_layer = element
            .attributes
            .get("id")
            .map(|id| {
                let id_lower = id.to_lowercase();
                id_lower.contains("cutline")
                    || id_lower.contains("boundary")
                    || id_lower.contains("notch")
                    || id_lower.contains("grainline")
                    || id_lower.contains("grain")
                    || id_lower.contains("seamline")
                    || id_lower.contains("seam")
                    || id_lower.contains("drill")
                    || id_lower.contains("hole")
                    || id_lower.contains("tuck")
            })
            .unwrap_or(false);

        if group_has_layer {
            eprintln!(
                "  └─ Step: Processing {} children with group layer '{}'",
                element.children.len(),
                current_layer
            );
            Some(current_layer)
        } else {
            eprintln!(
                "  └─ Step: Processing {} children (group without layer hints, no parent layer)",
                element.children.len()
            );
            None
        }
    } else {
        // Other elements don't pass layers to children.
        eprintln!(
            "  └─ Step: Processing {} children (no parent layer)",
            element.children.len()
        );
        None
    };

    for (i, child) in element.children.iter().enumerate() {
        if let XMLNode::Element(child_element) = child {
            eprintln!(
                "    • Processing child {} of {}: '{}'",
                i + 1,
                element.children.len(),
                child_element.name
            );
            convert_element_tree(
                child_element,
                target,
                options,
                svg_height,
                child_parent_layer,
            )?;
            eprintln!("    • Child {} processed successfully", i + 1);
        } else {
            eprintln!("    • Child {} is not an element (skipping)", i + 1);
        }
    }

    eprintln!(
        "[CONVERTER] convert_element_tree: Finished processing element '{}'",
        element.name
    );
    Ok(())
}

// @brief Trait for targets that can receive entities (Block or Drawing).
trait EntityTarget {
    // Add an entity to this target.
    fn add_entity(&mut self, entity: Box<dyn Entity>);
}

impl EntityTarget for Block {
    fn add_entity(&mut self, entity: Box<dyn Entity>) {
        Block::add_entity(self, entity);
    }
}

impl EntityTarget for Drawing {
    fn add_entity(&mut self, entity: Box<dyn Entity>) {
        Drawing::add_modelspace_entity(self, entity);
    }
}

// @brief Populate CLO3D semantic fields on a Block from its entity list.
//
// After `convert_element_tree` fills `block.entities`, this function scans those
// entities (using downcasting via `Any`) and extracts:
//   - `boundary_vertices` — vertices of the first Polyline on layer "1"
//   - `corner_flags`      — per-vertex corner classification (120° threshold)
//   - `grainline`         — first Line or two-point Polyline on layer "7"
//   - `notches`           — all Lines and two-point Polylines on layer "4"
//
// All fields on `block` are written; pre-existing values are overwritten.
fn extract_semantic_fields(block: &mut Block) {
    for entity in &block.entities {
        let entity_any = entity.as_ref() as &dyn Any;

        match entity.layer() {
            // Boundary polyline (layer 1) — take the first one found.
            "1" if block.boundary_vertices.is_empty() => {
                if let Some(poly) = entity_any.downcast_ref::<Polyline>() {
                    block.boundary_vertices = poly.vertices.clone();
                } // if Polyline
            } // "1"

            // Grainline (layer 7) — Line or first/last points of Polyline.
            "7" if block.grainline.is_none() => {
                if let Some(line) = entity_any.downcast_ref::<Line>() {
                    block.grainline = Some((line.start, line.end));
                } else if let Some(poly) = entity_any.downcast_ref::<Polyline>() {
                    if poly.vertices.len() >= 2 {
                        let start = *poly.vertices.first().unwrap();
                        let end = *poly.vertices.last().unwrap();
                        block.grainline = Some((start, end));
                    } // if at least 2 vertices
                } // else if Polyline
            } // "7"

            // Notches (layer 4) — Line or first/last of Polyline.
            "4" => {
                if let Some(line) = entity_any.downcast_ref::<Line>() {
                    block.notches.push((line.start, line.end));
                } else if let Some(poly) = entity_any.downcast_ref::<Polyline>() {
                    if poly.vertices.len() >= 2 {
                        let start = *poly.vertices.first().unwrap();
                        let end = *poly.vertices.last().unwrap();
                        block.notches.push((start, end));
                    } // if at least 2 vertices
                } // else if Polyline
            } // "4"

            _ => {} // other layers — not used in CLO3D semantic fields
        } // match layer
    } // for each entity

    // Run corner detection on boundary vertices (120° threshold matches seamly2clo.py).
    if !block.boundary_vertices.is_empty() {
        block.corner_flags = detect_corners(&block.boundary_vertices, 120.0);
    } // if boundary vertices present
} // fn extract_semantic_fields

// @brief Convert SVG <line> element to DXF LINE entity.
// @param element SVG line element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @return LINE entity or None if conversion fails.
fn convert_line(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<Option<Line>> {
    // Parse line attributes.
    let x1 = parse_float_attr(element.attributes.get("x1"), 0.0);
    let y1 = parse_float_attr(element.attributes.get("y1"), 0.0);
    let x2 = parse_float_attr(element.attributes.get("x2"), 0.0);
    let y2 = parse_float_attr(element.attributes.get("y2"), 0.0);

    // Transform coordinates if needed.
    let start = Point::new(x1, y1);
    let end = Point::new(x2, y2);
    let (start, end) = if options.invert_y {
        (
            invert_y_axis(start, svg_height),
            invert_y_axis(end, svg_height),
        )
    } else {
        (start, end)
    };

    // Get layer name.
    let layer = map_svg_to_astm_layer(element).to_string();

    Ok(Some(Line { layer, start, end }))
}

// @brief Convert SVG <circle> element to DXF CIRCLE entity.
// @param element SVG circle element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @return CIRCLE entity or None if conversion fails.
fn convert_circle(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<Option<Circle>> {
    // Parse circle attributes.
    let cx = parse_float_attr(element.attributes.get("cx"), 0.0);
    let cy = parse_float_attr(element.attributes.get("cy"), 0.0);
    let r = parse_float_attr(element.attributes.get("r"), 0.0);

    if r <= 0.0 {
        return Ok(None); // Invalid circle.
    }

    // Transform coordinates if needed.
    let center = Point::new(cx, cy);
    let center = if options.invert_y {
        invert_y_axis(center, svg_height)
    } else {
        center
    };

    // Get layer name.
    let layer = map_svg_to_astm_layer(element).to_string();

    Ok(Some(Circle {
        layer,
        center,
        radius: r,
    }))
}

// @brief Convert SVG <text> element to DXF TEXT entity.
// @param element SVG text element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @return TEXT entity or None if conversion fails.
fn convert_text(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<Option<Text>> {
    // Parse text position.
    let x = parse_float_attr(element.attributes.get("x"), 0.0);
    let y = parse_float_attr(element.attributes.get("y"), 0.0);

    // Parse font size (default to 12 if not specified).
    let height = parse_float_attr(element.attributes.get("font-size"), 12.0);

    // Parse rotation (default to 0).
    let rotation = parse_float_attr(element.attributes.get("transform"), 0.0);
    // TODO: Parse rotation from transform attribute properly.

    // Extract text content from element children.
    let mut content = String::new();
    for child in &element.children {
        if let XMLNode::Text(text) = child {
            content.push_str(text);
        }
    }

    // Sanitize text to ASCII-only.
    let content = sanitize_ascii(&content);

    if content.is_empty() {
        return Ok(None); // No text content.
    }

    // Transform coordinates if needed.
    let insertion_point = Point::new(x, y);
    let insertion_point = if options.invert_y {
        invert_y_axis(insertion_point, svg_height)
    } else {
        insertion_point
    };

    // Get layer name.
    let layer = map_svg_to_astm_layer(element).to_string();

    Ok(Some(Text {
        layer,
        insertion_point,
        height,
        rotation,
        content,
    }))
}

// @brief Convert SVG <path> element to DXF POLYLINE entity.
// @param element SVG path element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @return POLYLINE entity or None if conversion fails.
fn convert_path(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
) -> Result<Option<Polyline>> {
    eprintln!("      [convert_path] Starting PATH conversion");

    eprintln!("      [convert_path] Step 1: Extracting path data");
    // Get path data from 'd' attribute.
    let path_data = element.attributes.get("d").ok_or_else(|| {
        eprintln!("      [convert_path] ❌ FAILED: No 'd' attribute found");
        crate::error::SvgToEzdxfError::Svg("Path element missing 'd' attribute".to_string())
    })?;
    eprintln!("        • Path data: '{}'", path_data);

    if path_data.trim().is_empty() {
        eprintln!("      [convert_path] ❌ FAILED: Empty path data");
        return Ok(None);
    }

    eprintln!("      [convert_path] Step 2: Parsing SVG path");
    // Parse SVG path using geometry crate.
    let svg_path = Path::parse_path_attribute(path_data).map_err(|e| {
        eprintln!(
            "      [convert_path] ❌ FAILED: Path parsing error: {:?}",
            e
        );
        crate::error::SvgToEzdxfError::Geometry(format!("Failed to parse path data: {:?}", e))
    })?;
    eprintln!(
        "        • Path parsed successfully, {} segments",
        svg_path.segments.len()
    );

    eprintln!("      [convert_path] Step 3: Flattening path to polyline");
    // Flatten path to points using tolerance.
    let tolerance = options.flatten_tolerance as f32;
    eprintln!("        • Flatten tolerance: {}", tolerance);
    let flattened_points = svg_path.flatten(tolerance);
    eprintln!("        • Flattened to {} points", flattened_points.len());

    if flattened_points.is_empty() {
        eprintln!("      [convert_path] ❌ FAILED: Path flattened to empty point list");
        return Ok(None);
    }

    // Check if path is closed (last point equals first point, or path ends with Close command).
    let is_closed = svg_path
        .segments
        .iter()
        .any(|seg| matches!(seg, geometry::PathSegment::Close))
        || (flattened_points.len() > 1
            && flattened_points.first().map(|p| (p.x, p.y))
                == flattened_points.last().map(|p| (p.x, p.y)));
    eprintln!("        • Path closed: {}", is_closed);

    eprintln!(
        "      [convert_path] Step 4: Converting points and applying coordinate transformation"
    );
    // Convert geometry::Point (f32) to entities::Point (f64) and apply Y-axis inversion.
    let mut vertices: Vec<Point> = flattened_points
        .iter()
        .map(|p| {
            // Convert f32 to f64.
            let point = Point::new(p.x as f64, p.y as f64);
            // Apply Y-axis inversion if enabled.
            if options.invert_y {
                invert_y_axis(point, svg_height)
            } else {
                point
            }
        })
        .collect();
    eprintln!("        • Converted {} vertices", vertices.len());

    // Remove duplicate consecutive points (can occur from flattening).
    vertices.dedup();
    if vertices.len() < 2 {
        eprintln!(
            "      [convert_path] ❌ FAILED: Path has less than 2 unique vertices after deduplication"
        );
        return Ok(None);
    }
    eprintln!("        • After deduplication: {} vertices", vertices.len());

    eprintln!("      [convert_path] Step 5: Determining layer");
    // Get layer name.
    let layer = map_svg_to_astm_layer(element).to_string();
    eprintln!("        • Layer: '{}'", layer);

    eprintln!("      [convert_path] Step 6: Creating POLYLINE entity");
    let polyline_entity = Polyline {
        layer: layer.clone(),
        vertices,
        closed: is_closed,
    };
    eprintln!("        • POLYLINE entity created:");
    eprintln!("          - Layer: '{}'", polyline_entity.layer);
    eprintln!("          - Vertices: {}", polyline_entity.vertices.len());
    eprintln!("          - Closed: {}", polyline_entity.closed);
    eprintln!("      [convert_path] ✅ PATH conversion successful");

    Ok(Some(polyline_entity))
}

// @brief Parse SVG points attribute into a vector of Points.
// @param points_str Points string (e.g., "10,20 30,40 50,60" or "10 20 30 40 50 60").
// @return Vector of Points.
fn parse_points_attribute(points_str: &str) -> Vec<Point> {
    let mut points = Vec::new();
    let coords: Vec<&str> = points_str.trim().split_whitespace().collect();

    // Handle both formats: "x,y x,y" and "x y x y"
    let mut i = 0;
    while i < coords.len() {
        let coord_str = coords[i];
        if let Some(comma_pos) = coord_str.find(',') {
            // Format: "x,y"
            let x_str = &coord_str[..comma_pos];
            let y_str = &coord_str[comma_pos + 1..];
            if let (Ok(x), Ok(y)) = (x_str.parse::<f64>(), y_str.parse::<f64>()) {
                points.push(Point::new(x, y));
            }
            i += 1;
        } else {
            // Format: "x y" (two separate values)
            if i + 1 < coords.len() {
                if let (Ok(x), Ok(y)) = (coords[i].parse::<f64>(), coords[i + 1].parse::<f64>()) {
                    points.push(Point::new(x, y));
                }
                i += 2;
            } else {
                i += 1;
            }
        }
    }

    points
}

// @brief Convert SVG <polyline> element to DXF POLYLINE entity.
// @param element SVG polyline element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @param parent_layer Optional parent layer to override element layer.
// @return POLYLINE entity or None if conversion fails.
fn convert_polyline(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    parent_layer: Option<&str>,
) -> Result<Option<Polyline>> {
    eprintln!("      [convert_polyline] Starting POLYLINE conversion");

    // Get points attribute.
    let points_str = element.attributes.get("points").ok_or_else(|| {
        eprintln!("      [convert_polyline] ❌ FAILED: No 'points' attribute found");
        crate::error::SvgToEzdxfError::Svg(
            "Polyline element missing 'points' attribute".to_string(),
        )
    })?;

    if points_str.trim().is_empty() {
        eprintln!("      [convert_polyline] ❌ FAILED: Empty points attribute");
        return Ok(None);
    }

    eprintln!("      [convert_polyline] Step 1: Parsing points attribute");
    let mut vertices = parse_points_attribute(points_str);
    eprintln!("        • Parsed {} points", vertices.len());

    if vertices.len() < 2 {
        eprintln!("      [convert_polyline] ❌ FAILED: Polyline needs at least 2 points");
        return Ok(None);
    }

    eprintln!("      [convert_polyline] Step 2: Applying coordinate transformation");
    // Apply Y-axis inversion if needed.
    if options.invert_y {
        vertices = vertices
            .iter()
            .map(|p| invert_y_axis(*p, svg_height))
            .collect();
    }

    eprintln!("      [convert_polyline] Step 3: Determining layer");
    let layer = parent_layer
        .map(|s| s.to_string())
        .unwrap_or_else(|| map_svg_to_astm_layer(element).to_string());
    eprintln!("        • Layer: '{}'", layer);

    eprintln!("      [convert_polyline] Step 4: Creating POLYLINE entity");
    // Polyline is never closed (use <polygon> for closed shapes).
    let polyline_entity = Polyline {
        layer,
        vertices,
        closed: false,
    };
    eprintln!(
        "        • POLYLINE entity created: {} vertices, closed: false",
        polyline_entity.vertices.len()
    );
    eprintln!("      [convert_polyline] ✅ POLYLINE conversion successful");

    Ok(Some(polyline_entity))
}

// @brief Convert SVG <polygon> element to DXF POLYLINE entity (closed).
// @param element SVG polygon element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @param parent_layer Optional parent layer to override element layer.
// @return POLYLINE entity or None if conversion fails.
fn convert_polygon(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    parent_layer: Option<&str>,
) -> Result<Option<Polyline>> {
    eprintln!("      [convert_polygon] Starting POLYGON conversion");

    // Get points attribute.
    let points_str = element.attributes.get("points").ok_or_else(|| {
        eprintln!("      [convert_polygon] ❌ FAILED: No 'points' attribute found");
        crate::error::SvgToEzdxfError::Svg("Polygon element missing 'points' attribute".to_string())
    })?;

    if points_str.trim().is_empty() {
        eprintln!("      [convert_polygon] ❌ FAILED: Empty points attribute");
        return Ok(None);
    }

    eprintln!("      [convert_polygon] Step 1: Parsing points attribute");
    let mut vertices = parse_points_attribute(points_str);
    eprintln!("        • Parsed {} points", vertices.len());

    if vertices.len() < 3 {
        eprintln!("      [convert_polygon] ❌ FAILED: Polygon needs at least 3 points");
        return Ok(None);
    }

    eprintln!("      [convert_polygon] Step 2: Applying coordinate transformation");
    // Apply Y-axis inversion if needed.
    if options.invert_y {
        vertices = vertices
            .iter()
            .map(|p| invert_y_axis(*p, svg_height))
            .collect();
    }

    // Ensure polygon is closed (first point equals last point).
    if vertices.first() != vertices.last() {
        vertices.push(vertices[0]);
    }

    eprintln!("      [convert_polygon] Step 3: Determining layer");
    let layer = parent_layer
        .map(|s| s.to_string())
        .unwrap_or_else(|| map_svg_to_astm_layer(element).to_string());
    eprintln!("        • Layer: '{}'", layer);

    eprintln!("      [convert_polygon] Step 4: Creating POLYLINE entity (closed)");
    let polyline_entity = Polyline {
        layer,
        vertices,
        closed: true,
    };
    eprintln!(
        "        • POLYLINE entity created: {} vertices, closed: true",
        polyline_entity.vertices.len()
    );
    eprintln!("      [convert_polygon] ✅ POLYGON conversion successful");

    Ok(Some(polyline_entity))
}

// @brief Convert SVG <rect> element to DXF POLYLINE entity (4 vertices, closed).
// @param element SVG rect element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @param parent_layer Optional parent layer to override element layer.
// @return POLYLINE entity or None if conversion fails.
fn convert_rect(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    parent_layer: Option<&str>,
) -> Result<Option<Polyline>> {
    eprintln!("      [convert_rect] Starting RECT conversion");

    // Parse rect attributes.
    let x = parse_float_attr(element.attributes.get("x"), 0.0);
    let y = parse_float_attr(element.attributes.get("y"), 0.0);
    let width = parse_float_attr(element.attributes.get("width"), 0.0);
    let height = parse_float_attr(element.attributes.get("height"), 0.0);

    eprintln!(
        "        • Rect attributes: x={}, y={}, width={}, height={}",
        x, y, width, height
    );

    if width <= 0.0 || height <= 0.0 {
        eprintln!("      [convert_rect] ❌ FAILED: Invalid width or height");
        return Ok(None);
    }

    eprintln!("      [convert_rect] Step 1: Creating 4 vertices");
    // Create 4 vertices: top-left, top-right, bottom-right, bottom-left.
    let mut vertices = vec![
        Point::new(x, y),                  // top-left
        Point::new(x + width, y),          // top-right
        Point::new(x + width, y + height), // bottom-right
        Point::new(x, y + height),         // bottom-left
    ];

    eprintln!("      [convert_rect] Step 2: Applying coordinate transformation");
    // Apply Y-axis inversion if needed.
    if options.invert_y {
        vertices = vertices
            .iter()
            .map(|p| invert_y_axis(*p, svg_height))
            .collect();
    }

    // Close the rectangle (add first point at end).
    vertices.push(vertices[0]);

    eprintln!("      [convert_rect] Step 3: Determining layer");
    let layer = parent_layer
        .map(|s| s.to_string())
        .unwrap_or_else(|| map_svg_to_astm_layer(element).to_string());
    eprintln!("        • Layer: '{}'", layer);

    eprintln!("      [convert_rect] Step 4: Creating POLYLINE entity (closed)");
    let polyline_entity = Polyline {
        layer,
        vertices,
        closed: true,
    };
    eprintln!(
        "        • POLYLINE entity created: {} vertices, closed: true",
        polyline_entity.vertices.len()
    );
    eprintln!("      [convert_rect] ✅ RECT conversion successful");

    Ok(Some(polyline_entity))
}

// @brief Convert SVG <ellipse> element to DXF CIRCLE or POLYLINE entity.
// @param element SVG ellipse element.
// @param options Conversion options.
// @param svg_height SVG document height for coordinate transformation.
// @param parent_layer Optional parent layer to override element layer.
// @return CIRCLE or POLYLINE entity or None if conversion fails.
// @details If rx == ry, converts to CIRCLE. Otherwise, approximates as POLYLINE.
fn convert_ellipse(
    element: &Element,
    options: &SvgToEzdxfOptions,
    svg_height: f64,
    parent_layer: Option<&str>,
) -> Result<Option<Box<dyn Entity>>> {
    eprintln!("      [convert_ellipse] Starting ELLIPSE conversion");

    // Parse ellipse attributes.
    let cx = parse_float_attr(element.attributes.get("cx"), 0.0);
    let cy = parse_float_attr(element.attributes.get("cy"), 0.0);
    let rx = parse_float_attr(element.attributes.get("rx"), 0.0);
    let ry = parse_float_attr(element.attributes.get("ry"), 0.0);

    eprintln!(
        "        • Ellipse attributes: cx={}, cy={}, rx={}, ry={}",
        cx, cy, rx, ry
    );

    if rx <= 0.0 || ry <= 0.0 {
        eprintln!("      [convert_ellipse] ❌ FAILED: Invalid rx or ry");
        return Ok(None);
    }

    // Get layer name (use parent layer if provided).
    let layer = parent_layer
        .map(|s| s.to_string())
        .unwrap_or_else(|| map_svg_to_astm_layer(element).to_string());

    // If rx == ry, convert to CIRCLE.
    if (rx - ry).abs() < 1e-6 {
        eprintln!("      [convert_ellipse] Step 1: Converting to CIRCLE (rx == ry)");
        let center = Point::new(cx, cy);
        let center = if options.invert_y {
            invert_y_axis(center, svg_height)
        } else {
            center
        };

        let circle_entity = Circle {
            layer,
            center,
            radius: rx,
        };
        eprintln!(
            "        • CIRCLE entity created: center=({}, {}), radius={}",
            center.x, center.y, rx
        );
        eprintln!("      [convert_ellipse] ✅ ELLIPSE conversion successful (as CIRCLE)");

        Ok(Some(Box::new(circle_entity)))
    } else {
        // Otherwise, approximate as POLYLINE with multiple points.
        eprintln!("      [convert_ellipse] Step 1: Converting to POLYLINE (rx != ry)");

        // Generate points around ellipse (approximate with 32 points for smooth curve).
        const NUM_POINTS: usize = 32;
        let mut vertices = Vec::with_capacity(NUM_POINTS + 1);

        for i in 0..NUM_POINTS {
            let angle = 2.0 * std::f64::consts::PI * (i as f64) / (NUM_POINTS as f64);
            let x = cx + rx * angle.cos();
            let y = cy + ry * angle.sin();
            vertices.push(Point::new(x, y));
        }

        eprintln!(
            "        • Generated {} points for ellipse approximation",
            vertices.len()
        );

        eprintln!("      [convert_ellipse] Step 2: Applying coordinate transformation");
        // Apply Y-axis inversion if needed.
        if options.invert_y {
            vertices = vertices
                .iter()
                .map(|p| invert_y_axis(*p, svg_height))
                .collect();
        }

        // Close the ellipse.
        vertices.push(vertices[0]);

        eprintln!("      [convert_ellipse] Step 3: Creating POLYLINE entity (closed)");
        let polyline_entity = Polyline {
            layer,
            vertices,
            closed: true,
        };
        eprintln!(
            "        • POLYLINE entity created: {} vertices, closed: true",
            polyline_entity.vertices.len()
        );
        eprintln!("      [convert_ellipse] ✅ ELLIPSE conversion successful (as POLYLINE)");

        Ok(Some(Box::new(polyline_entity)))
    }
}
