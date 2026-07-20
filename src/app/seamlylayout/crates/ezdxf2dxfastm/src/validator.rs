// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief ASTM-D6673-10 compliance validation.

use seamly_svg2ezdxf::DxfVersion;

// @brief Validation error.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ValidationError {
    // Error message.
    pub message: String,
}

// @brief Validate Drawing for ASTM-D6673-10 compliance.
// @param drawing The Drawing to validate.
// @return Result with validation errors if any.
pub fn validate_astm_compliance(
    drawing: &seamly_svg2ezdxf::Drawing,
) -> Result<(), Vec<ValidationError>> {
    let mut errors = Vec::new();

    // Validate DXF version (must be R12).
    if drawing.version != DxfVersion::R12 {
        errors.push(ValidationError {
            message: format!(
                "DXF version must be R12 for ASTM-D6673-10, got: {:?}",
                drawing.version
            ),
        });
    }

    // Validate entity types (ASTM-D6673-10 allowed types).
    let allowed_entity_types = ["LINE", "CIRCLE", "POLYLINE", "TEXT", "ARC", "POINT"];

    // Validate blocks.
    for (block_idx, block) in drawing.blocks.iter().enumerate() {
        // Validate block name (must be ASCII-only).
        if !block.name.is_ascii() {
            errors.push(ValidationError {
                message: format!("Block {} has non-ASCII name: '{}'", block_idx, block.name),
            });
        }

        // Validate entities in block.
        for (entity_idx, entity) in block.entities.iter().enumerate() {
            let entity_type = entity.entity_type();
            if !allowed_entity_types.contains(&entity_type) {
                errors.push(ValidationError {
                    message: format!(
                        "Block {} entity {} has unsupported type: '{}'",
                        block_idx, entity_idx, entity_type
                    ),
                });
            }

            // Validate layer name (must be ASCII-only).
            let layer = entity.layer();
            if !layer.is_ascii() {
                errors.push(ValidationError {
                    message: format!(
                        "Block {} entity {} has non-ASCII layer: '{}'",
                        block_idx, entity_idx, layer
                    ),
                });
            }

            // Validate text content (if TEXT entity).
            if entity_type == "TEXT" {
                // Check if we can downcast to Text to validate content.
                if let Some(text) =
                    (entity.as_ref() as &dyn std::any::Any).downcast_ref::<seamly_svg2ezdxf::Text>()
                {
                    if !text.content.is_ascii() {
                        errors.push(ValidationError {
                            message: format!(
                                "Block {} entity {} (TEXT) has non-ASCII content",
                                block_idx, entity_idx
                            ),
                        });
                    }
                }
            }
        }
    }

    // Validate modelspace entities.
    for (entity_idx, entity) in drawing.modelspace_entities.iter().enumerate() {
        let entity_type = entity.entity_type();
        if !allowed_entity_types.contains(&entity_type) {
            errors.push(ValidationError {
                message: format!(
                    "Modelspace entity {} has unsupported type: '{}'",
                    entity_idx, entity_type
                ),
            });
        }

        // Validate layer name (must be ASCII-only).
        let layer = entity.layer();
        if !layer.is_ascii() {
            errors.push(ValidationError {
                message: format!(
                    "Modelspace entity {} has non-ASCII layer: '{}'",
                    entity_idx, layer
                ),
            });
        }

        // Validate text content (if TEXT entity).
        if entity_type == "TEXT" {
            if let Some(text) =
                (entity.as_ref() as &dyn std::any::Any).downcast_ref::<seamly_svg2ezdxf::Text>()
            {
                if !text.content.is_ascii() {
                    errors.push(ValidationError {
                        message: format!(
                            "Modelspace entity {} (TEXT) has non-ASCII content",
                            entity_idx
                        ),
                    });
                }
            }
        }
    }

    if errors.is_empty() {
        Ok(())
    } else {
        Err(errors)
    }
}
