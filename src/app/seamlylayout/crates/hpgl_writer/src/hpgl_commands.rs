// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT
//
// @file hpgl_commands.rs
// @brief Orders polylines and writes them as HP-GL/1 instructions.

use crate::{LineClass, PenMap, PlotPolyline};

/// Most coordinate pairs in one `PD` instruction; small plotter buffers reject long ones.
const POINTS_PER_PEN_DOWN: usize = 32;

/// Plot order: labels and marks first, cut lines last, so a cutter frees a piece only at the end.
const CLASS_ORDER: [LineClass; 3] = [LineClass::Label, LineClass::Mark, LineClass::Cut];

/// @brief Write `polylines` as one HP-GL/1 program.
///
/// Classes are plotted in `CLASS_ORDER`. Within a class the next polyline is
/// the one whose start is nearest the pen, which cuts pen-up travel. A pen
/// select (`SP`) is written only when the pen changes, so classes that share
/// a pen do not swap pens.
pub(crate) fn write_program(polylines: &[PlotPolyline], pens: &PenMap) -> String {
    // IN resets the plotter; PA selects absolute coordinates.
    let mut program = String::from("IN;\nPA;\n");
    let mut pen_position = (0i32, 0i32);
    let mut current_pen: Option<u8> = None;

    for class in CLASS_ORDER {
        let mut remaining: Vec<&PlotPolyline> = polylines.iter().filter(|p| p.class == class).collect();
        if remaining.is_empty() {
            continue; // nothing in this class
        } // if empty class

        let pen = pens.pen_for(class);
        if current_pen != Some(pen) {
            // Change pen only when this class uses a different one.
            program.push_str(&format!("SP{pen};\n"));
            current_pen = Some(pen);
        } // if pen changes

        while !remaining.is_empty() {
            // Take the polyline that starts nearest the pen.
            let next_index = nearest_start(&remaining, pen_position);
            let polyline = remaining.swap_remove(next_index);
            pen_position = write_polyline(&mut program, polyline);
        } // while remaining
    } // for class

    // Lift the pen and put it away.
    program.push_str("PU;\nSP0;\n");
    program
} // fn write_program

/// @brief Index of the polyline whose first point is nearest `from`.
fn nearest_start(polylines: &[&PlotPolyline], from: (i32, i32)) -> usize {
    let distance = |p: &&PlotPolyline| {
        let (x, y) = p.points[0];
        let dx = i64::from(x - from.0);
        let dy = i64::from(y - from.1);
        dx * dx + dy * dy // squared distance; the order is the same
    };
    polylines
        .iter()
        .enumerate()
        .min_by_key(|(_, p)| distance(p))
        .map(|(index, _)| index)
        .unwrap_or(0)
} // fn nearest_start

/// @brief Append one polyline as `PU` to its start then `PD` through its points.
/// @return The pen position after the polyline.
fn write_polyline(program: &mut String, polyline: &PlotPolyline) -> (i32, i32) {
    let start = polyline.points[0];
    program.push_str(&format!("PU{},{};\n", start.0, start.1));

    // A closed polyline returns to its start.
    let mut path: Vec<(i32, i32)> = polyline.points[1..].to_vec();
    if polyline.closed && path.last() != Some(&start) {
        path.push(start);
    } // if closed and open-ended

    for chunk in path.chunks(POINTS_PER_PEN_DOWN) {
        // Split long polylines so each PD fits a small plotter buffer.
        let coordinates: Vec<String> = chunk.iter().map(|(x, y)| format!("{x},{y}")).collect();
        program.push_str(&format!("PD{};\n", coordinates.join(",")));
    } // for chunk

    path.last().copied().unwrap_or(start)
} // fn write_polyline
