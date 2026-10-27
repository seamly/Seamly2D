// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Pattern units of a handoff and the DXF units they select.

// Millimetres per inch.
const MM_PER_INCH: f64 = 25.4;

// @brief DXF `Units:` value: the unit of every length in the file.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum DxfUnits {
    // Millimetres, two decimals.
    #[default]
    Metric,
    // Inches, four decimals.
    English,
}

impl DxfUnits {
    // @brief `Units:` text value.
    pub fn name(self) -> &'static str {
        match self {
            DxfUnits::Metric => "METRIC",
            DxfUnits::English => "ENGLISH",
        }
    } // fn name

    // @brief A length in the DXF units, as text.
    // @param mm Length in millimetres; every drawing length is held in millimetres.
    pub fn length(self, mm: f64) -> String {
        match self {
            DxfUnits::Metric => format!("{:.2}", mm),
            DxfUnits::English => format!("{:.4}", mm / MM_PER_INCH),
        }
    } // fn length
} // impl DxfUnits

// @brief Pattern unit of a handoff: `data-units` of the pattern group.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum PatternUnit {
    // No `data-units`, or an unknown value: METRIC, size values unchanged.
    #[default]
    Unspecified,
    Mm,
    Cm,
    Inch,
}

impl PatternUnit {
    // @brief Parse a `data-units` value; anything else is `Unspecified`.
    pub fn from_attribute(value: &str) -> Self {
        match value.trim().to_ascii_lowercase().as_str() {
            "mm" => PatternUnit::Mm,
            "cm" => PatternUnit::Cm,
            "inch" | "in" => PatternUnit::Inch,
            _ => PatternUnit::Unspecified,
        }
    } // fn from_attribute

    // @brief DXF units of this pattern unit: inches write ENGLISH, everything else METRIC.
    pub fn dxf_units(self) -> DxfUnits {
        if self == PatternUnit::Inch { DxfUnits::English } else { DxfUnits::Metric }
    } // fn dxf_units

    // @brief A size value (`Size Name:`, sample size, size list) in DXF units.
    // @details A cm value is multiplied by 10 (50 becomes 500, millimetres). Other units and
    //          text that is not a number stay unchanged.
    pub fn size_value(self, value: &str) -> String {
        if self != PatternUnit::Cm {
            return value.to_string();
        }
        match value.trim().parse::<f64>() {
            // Round to two places to drop float noise, then print without trailing zeros.
            Ok(v) => format!("{}", (v * 10.0 * 100.0).round() / 100.0),
            Err(_) => value.to_string(),
        }
    } // fn size_value
} // impl PatternUnit

#[cfg(test)]
mod tests {
    use super::*;

    // @brief Units map to DXF units and size values convert.
    #[test]
    fn pattern_unit_selects_dxf_units_and_sizes() {
        assert_eq!(PatternUnit::from_attribute("cm"), PatternUnit::Cm);
        assert_eq!(PatternUnit::from_attribute("inch").dxf_units(), DxfUnits::English);
        assert_eq!(PatternUnit::from_attribute("").dxf_units(), DxfUnits::Metric);
        assert_eq!(PatternUnit::Cm.size_value("50"), "500");
        assert_eq!(PatternUnit::Cm.size_value("50.5"), "505");
        assert_eq!(PatternUnit::Cm.size_value("M"), "M");
        assert_eq!(PatternUnit::Mm.size_value("500"), "500");
        assert_eq!(PatternUnit::Inch.size_value("40"), "40");
    } // fn pattern_unit_selects_dxf_units_and_sizes

    // @brief Lengths print in millimetres or inches.
    #[test]
    fn dxf_units_format_lengths() {
        assert_eq!(DxfUnits::Metric.length(25.4), "25.40");
        assert_eq!(DxfUnits::English.length(25.4), "1.0000");
        assert_eq!(DxfUnits::Metric.name(), "METRIC");
        assert_eq!(DxfUnits::English.name(), "ENGLISH");
    } // fn dxf_units_format_lengths
}
