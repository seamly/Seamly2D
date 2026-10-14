// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Grade rules (D6673 §4.3.1.4) and the grade rule table (§4.4) of a base size export.
//!
//! The layout holds the sample size of every piece, each at its layout
//! position. The rules need every size at its drafted position, where a size
//! differs from the sample size only by growth. The handoff (input) document has
//! them there, so:
//! 1. Each line is graded in input space: `astm_contour::grade_contours` gives
//!    every size the same key points.
//! 2. Each rule is the growth of one key point or notch from the sample size,
//!    in input space. Equal rules share one identifier.
//! 3. A rigid fit of the layout block to its input block maps the graded
//!    sample lines, the `# n` texts and the grade reference line into the layout.
//!
//! The grade reference line is one horizontal draft-space line per piece. It
//! gives the reader the X axis of the rules when the layout turned the piece.

use crate::astm_contour::{distance, grade_contours, AstmContour, GradeError};
use crate::drawing::{Block, BlockGrading, Drawing, GradeRule, GradeRuleTable, RuleText};
use crate::entities::Point;

// A layout block fits its input block when every vertex lands this close.
const FIT_TOLERANCE_MM: f64 = 0.1;
// Two rules whose growths all differ by less than this are one rule.
const RULE_TOLERANCE_MM: f64 = 0.01;
// Shortest grade reference line.
const MIN_REFERENCE_LINE_MM: f64 = 10.0;

// @brief Rotation and translation that maps input space to layout space.
#[derive(Debug, Clone, Copy)]
struct Rigid {
    cos: f64,
    sin: f64,
    tx: f64,
    ty: f64,
}

impl Rigid {
    // @brief Map one point.
    fn apply(&self, p: Point) -> Point {
        Point::new(self.cos * p.x - self.sin * p.y + self.tx, self.sin * p.x + self.cos * p.y + self.ty)
    } // fn apply

    // @brief Map every point of a contour; key points and turn flags keep their order.
    fn contour(&self, c: &AstmContour) -> AstmContour {
        AstmContour {
            dense: c.dense.iter().map(|&p| self.apply(p)).collect(),
            reduced: c.reduced.iter().map(|&p| self.apply(p)).collect(),
            turn: c.turn.clone(),
            closed: c.closed,
        }
    } // fn contour
} // impl Rigid

// @brief The lines of one piece size that grading gives matching key points.
struct PieceLines<'a> {
    boundary: &'a AstmContour,
    sew_lines: &'a [AstmContour],
    internal_lines: &'a [AstmContour],
    cutouts: &'a [AstmContour],
}

impl<'a> PieceLines<'a> {
    // @brief The lines of a block; None without a boundary.
    fn of(block: &'a Block) -> Option<Self> {
        Some(Self {
            boundary: block.boundary.as_ref()?,
            sew_lines: &block.sew_lines,
            internal_lines: &block.internal_lines,
            cutouts: &block.cutouts,
        })
    } // fn of

    // @brief Every line with its D6673 layer, in writing order.
    fn all(&self) -> Vec<(&'static str, &'a AstmContour)> {
        let mut lines = vec![("1", self.boundary)];
        lines.extend(self.sew_lines.iter().map(|c| ("14", c)));
        lines.extend(self.internal_lines.iter().map(|c| ("8", c)));
        lines.extend(self.cutouts.iter().map(|c| ("11", c)));
        lines
    } // fn all
} // impl PieceLines

/// @brief Give each sample size piece of a layout drawing its grade rules.
/// @details A piece is graded when every size of the input has the same lines and
///          notches, and its layout block fits its input block. A graded block gets
///          the graded key points, a `# n` text on each key point and notch, and a
///          grade reference line; `layout.grade_rule_table` gets the rules. A piece
///          that cannot grade keeps its ungraded block, and a warning names it. A
///          warning also names each sample size piece the layout left out.
///          Blocks of other sizes are left as they are.
/// @param layout Drawing of the layout: the sample size pieces at their layout positions.
/// @param input Drawing of the handoff document: every size of every piece, at drafted positions.
/// @param sizes Every size, in grading order (`data-sizes`).
/// @param sample_size Sample size (`data-base-size`).
/// @return Warnings for the user, one per piece that is not graded.
pub fn grade_drawing(layout: &mut Drawing, input: &Drawing, sizes: &[String], sample_size: &str) -> Vec<String> {
    let mut warnings = Vec::new();
    let Some(sample) = sizes.iter().position(|s| s == sample_size) else {
        warnings.push(format!("Not graded: the sample size {sample_size} is not in the size list."));
        return warnings;
    }; // if no sample size
    if sizes.len() < 2 {
        return warnings;
    } // if one size

    let mut table = GradeRuleTable { sample_size: sample_size.to_string(), sizes: sizes.to_vec(), ..Default::default() };
    for b in 0..layout.blocks.len() {
        if layout.blocks[b].size.as_deref() != Some(sample_size) {
            continue;
        } // if not the sample size
        if let Err(reason) = grade_block(layout, b, input, sample, sizes, &mut table) {
            // Warnings name the piece as the user wrote it, not its escaped DXF name.
            warnings.push(format!("{}: not graded, {reason}.", layout.blocks[b].display_name));
        } // if not graded
    } // for each block

    // A sample size piece the packer left out has no block to grade; name it, so its loss is not silent.
    for missing in input.blocks.iter().filter(|b| b.size.as_deref() == Some(sample_size)) {
        let key = piece_key(missing);
        let placed = layout.blocks.iter().any(|b| b.size.as_deref() == Some(sample_size) && piece_key(b) == key);
        if !placed {
            warnings.push(format!("{}: not graded, size {sample_size} does not fit the media.", missing.display_name));
        } // if left out
    } // for each input piece of the sample size

    if !table.rules.is_empty() {
        layout.grade_rule_table = Some(table);
    } // if any piece graded
    warnings
} // fn grade_drawing

// @brief Piece key of a block: `parent` (the piece-set id), else the piece name.
fn piece_key(block: &Block) -> String {
    block.parent.clone().unwrap_or_else(|| block.piece_name.clone())
} // fn piece_key

// @brief Grade one sample size block: graded lines, `# n` texts and the reference line.
// @param block Index of the sample size block in `layout`.
// @param sample Index of the sample size in `sizes`.
// @return Err(reason) when the piece cannot be graded; the layout is then unchanged.
fn grade_block(
    layout: &mut Drawing,
    block: usize,
    input: &Drawing,
    sample: usize,
    sizes: &[String],
    table: &mut GradeRuleTable,
) -> Result<(), String> {
    let key = piece_key(&layout.blocks[block]);

    // Step 1: the input block of every size, from the same piece.
    let input_blocks: Vec<&Block> = sizes
        .iter()
        .map(|size| {
            input
                .blocks
                .iter()
                .find(|b| piece_key(b) == key && b.size.as_deref() == Some(size.as_str()))
                .ok_or_else(|| format!("size {size} is not in the handoff"))
        })
        .collect::<Result<_, _>>()?;
    let input_lines: Vec<PieceLines> = input_blocks
        .iter()
        .map(|b| PieceLines::of(b).ok_or_else(|| "a size has no boundary".to_string()))
        .collect::<Result<_, _>>()?;

    // Step 2: every size needs the same lines and notches.
    let sample_layers: Vec<&str> = input_lines[sample].all().iter().map(|(l, _)| *l).collect();
    let notch_count = input_blocks[sample].notches.len();
    for (lines, input_block) in input_lines.iter().zip(&input_blocks) {
        let layers: Vec<&str> = lines.all().iter().map(|(l, _)| *l).collect();
        if layers != sample_layers {
            return Err("the sizes have different internal lines or sew lines".to_string());
        } // if lines differ
        if input_block.notches.len() != notch_count {
            return Err("the sizes have different notch counts".to_string());
        } // if notches differ
    } // for each size

    // Step 3: grade each line across the sizes, in input space.
    let mut graded: Vec<Vec<AstmContour>> = vec![Vec::with_capacity(sample_layers.len()); sizes.len()];
    for line in 0..sample_layers.len() {
        let per_size: Vec<AstmContour> = input_lines.iter().map(|l| l.all()[line].1.clone()).collect();
        let result = grade_contours(&per_size, sample).map_err(|e| grade_error_text(e, sizes, sample, sample_layers[line]))?;
        for (size, contour) in result.into_iter().enumerate() {
            graded[size].push(contour);
        } // for each size
    } // for each line

    // Step 4: map the layout block from its input block.
    let layout_boundary = layout.blocks[block].boundary.as_ref().ok_or("the piece has no boundary in the layout")?;
    let fit = fit_rigid(&input_lines[sample].boundary.dense, &layout_boundary.dense)
        .or_else(|| fit_rigid(&input_lines[sample].boundary.reduced, &layout_boundary.reduced))
        .ok_or("the layout piece does not match its handoff shape")?;
    if layout.blocks[block].notches.len() != notch_count {
        return Err("the layout notches do not match the handoff".to_string());
    } // if notches differ

    // Step 5: rules. One per key point and per notch: growth from the sample size.
    let mut rule_texts = Vec::new();
    for (line, layer) in sample_layers.iter().enumerate() {
        for (i, &p) in graded[sample][line].reduced.iter().enumerate() {
            let deltas: Vec<(f64, f64)> = graded.iter().map(|g| growth(g[line].reduced[i], p)).collect();
            let rule = intern_rule(table, deltas);
            rule_texts.push(RuleText { position: fit.apply(p), layer, rule });
        } // for each key point
    } // for each line
    for (j, notch) in layout.blocks[block].notches.iter().enumerate() {
        let base = input_blocks[sample].notches[j].base;
        let deltas: Vec<(f64, f64)> = input_blocks.iter().map(|b| growth(b.notches[j].base, base)).collect();
        let rule = intern_rule(table, deltas);
        // The text sits on the written notch base: a reader pairs them by XY.
        rule_texts.push(RuleText { position: notch.base, layer: "4", rule });
    } // for each notch

    // Step 6: grade reference line, one horizontal draft-space line per piece.
    let reference = reference_line(&input_lines[sample].boundary.dense);

    // Step 7: write the graded sample lines and the grade data into the layout block.
    let sew_count = input_lines[sample].sew_lines.len();
    let internal_count = input_lines[sample].internal_lines.len();
    let target = &mut layout.blocks[block];
    let mut lines = graded[sample].iter().map(|c| fit.contour(c));
    target.boundary = lines.next();
    target.sew_lines = lines.by_ref().take(sew_count).collect();
    target.internal_lines = lines.by_ref().take(internal_count).collect();
    target.cutouts = lines.collect();
    target.grading = Some(BlockGrading { reference_line: (fit.apply(reference.0), fit.apply(reference.1)), rule_texts });
    Ok(())
} // fn grade_block

// @brief Warning text for a line that `grade_contours` cannot grade.
// @param layer D6673 layer of the line: 1, 14, 8 or 11.
// @return "size 640 has other turn points than size 500 on the cut line", for example.
fn grade_error_text(error: GradeError, sizes: &[String], sample: usize, layer: &str) -> String {
    let line = match layer {
        "1" => "the cut line",
        "14" => "the seam line",
        "8" => "an internal line",
        _ => "a cutout",
    }; // line
    let size = |index: usize| sizes.get(index).map_or("?", String::as_str);
    match error {
        GradeError::NoSample => format!("size {} is not in the size list", size(sample)),
        GradeError::ClosureDiffers(i) => format!("{line} of size {} is not closed as in size {}", size(i), size(sample)),
        GradeError::KeyPointOffLine(i) => format!("a key point of size {} is not on {line}", size(i)),
        GradeError::TurnPointsDiffer(i) => format!("size {} has other turn points than size {} on {line}", size(i), size(sample)),
    } // match error
} // fn grade_error_text

// @brief Growth of a point from its sample size position.
fn growth(p: Point, sample: Point) -> (f64, f64) {
    (p.x - sample.x, p.y - sample.y)
} // fn growth

// @brief Identifier of the rule with these growths, adding the rule when it is new.
fn intern_rule(table: &mut GradeRuleTable, deltas: Vec<(f64, f64)>) -> i32 {
    let same = |rule: &GradeRule| {
        rule.deltas
            .iter()
            .zip(&deltas)
            .all(|(a, b)| (a.0 - b.0).abs() < RULE_TOLERANCE_MM && (a.1 - b.1).abs() < RULE_TOLERANCE_MM)
    }; // same
    if let Some(rule) = table.rules.iter().find(|r| same(r)) {
        return rule.id;
    } // if known
    let id = table.rules.len() as i32 + 1;
    table.rules.push(GradeRule { id, deltas });
    id
} // fn intern_rule

// @brief Horizontal line along the bottom of the sample boundary, at least MIN_REFERENCE_LINE_MM long.
fn reference_line(boundary: &[Point]) -> (Point, Point) {
    let min_x = boundary.iter().map(|p| p.x).fold(f64::INFINITY, f64::min);
    let max_x = boundary.iter().map(|p| p.x).fold(f64::NEG_INFINITY, f64::max);
    let min_y = boundary.iter().map(|p| p.y).fold(f64::INFINITY, f64::min);
    (Point::new(min_x, min_y), Point::new(max_x.max(min_x + MIN_REFERENCE_LINE_MM), min_y))
} // fn reference_line

// @brief Rotation and translation that maps `from` onto `to`, vertex for vertex.
// @details A loop may start at another vertex in `to`, so every start is tried when
//          the first fails. A mirror image does not fit: stacking on the grade
//          reference line cannot undo it.
// @return None when the counts differ or no fit puts every vertex within FIT_TOLERANCE_MM.
fn fit_rigid(from: &[Point], to: &[Point]) -> Option<Rigid> {
    let n = from.len();
    if n < 2 || to.len() != n {
        return None;
    } // if not comparable
    (0..n).find_map(|shift| fit_rigid_shifted(from, to, shift))
} // fn fit_rigid

// @brief Least-squares rigid fit of from[i] onto to[(i + shift) % n] (2D Kabsch).
fn fit_rigid_shifted(from: &[Point], to: &[Point], shift: usize) -> Option<Rigid> {
    let n = from.len();
    let target = |i: usize| to[(i + shift) % n];
    let centroid = |points: &mut dyn Iterator<Item = Point>| {
        let (sx, sy) = points.fold((0.0, 0.0), |(x, y), p| (x + p.x, y + p.y));
        Point::new(sx / n as f64, sy / n as f64)
    }; // centroid
    let ca = centroid(&mut from.iter().copied());
    let cb = centroid(&mut (0..n).map(target));
    // Rotation angle from the cross and dot sums of the centred pairs.
    let (mut cross, mut dot) = (0.0, 0.0);
    for (i, a) in from.iter().enumerate() {
        let b = target(i);
        let (ax, ay, bx, by) = (a.x - ca.x, a.y - ca.y, b.x - cb.x, b.y - cb.y);
        cross += ax * by - ay * bx;
        dot += ax * bx + ay * by;
    } // for each pair
    let angle = cross.atan2(dot);
    let (sin, cos) = angle.sin_cos();
    let fit = Rigid { cos, sin, tx: cb.x - (cos * ca.x - sin * ca.y), ty: cb.y - (sin * ca.x + cos * ca.y) };
    from.iter().enumerate().all(|(i, &a)| distance(fit.apply(a), target(i)) <= FIT_TOLERANCE_MM).then_some(fit)
} // fn fit_rigid_shifted
