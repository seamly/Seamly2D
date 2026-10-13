// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

//! @brief Graded nests (D6673 §4.3.1.5) and the grade rule table (§4.4) of a multisize export.
//!
//! A multisize layout holds every size of every piece, each at its layout
//! position. Grading needs the sizes at their drafted positions, where a size
//! differs from the sample size only by growth. The handoff (input) document has
//! them there, so:
//! 1. Each line is graded in input space: `astm_contour::grade_contours` gives
//!    every size the same key points.
//! 2. Each rule is the growth of one key point or notch from the sample size,
//!    in input space. Equal rules share one identifier.
//! 3. A rigid fit of each layout block to its input block maps the graded
//!    lines and the grade reference line into the layout.
//!
//! The grade reference line is one horizontal draft-space line per piece, the
//! same for every size, so stacking the blocks on it gives the same growth as
//! the rules.

use crate::astm_contour::{distance, grade_contours, AstmContour};
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

// @brief Graded lines of one size, in `PieceLines::all` order.
type GradedLines = Vec<AstmContour>;

/// @brief Turn the pieces of a multisize layout drawing into graded nests.
/// @details Blocks of one piece share a `parent` (the piece-set id), else a piece
///          name. A piece is graded when the layout holds its sample size and every
///          size of the input has the same lines and notches. Otherwise only its
///          sample size block stays, and a warning names it. Graded nest blocks are
///          written in size order; `layout.grade_rule_table` gets the rules.
/// @param layout Drawing of the layout: one block per piece and size, at its layout position.
/// @param input Drawing of the handoff document: the same block names, at drafted positions.
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
    // Block indices to drop (graded sizes of a piece that fell back to its sample size).
    let mut dropped = vec![false; layout.blocks.len()];
    // Per block: (group, size index) for the nest order.
    let mut order: Vec<Option<(usize, usize)>> = vec![None; layout.blocks.len()];

    for (group, members) in nest_groups(layout).into_iter().enumerate() {
        // Size index of each member; a block of an unknown size stays as it is.
        let sized: Vec<(usize, usize)> = members
            .iter()
            .filter_map(|&b| {
                let size = layout.blocks[b].size.as_deref()?;
                Some((b, sizes.iter().position(|s| s == size)?))
            })
            .collect();
        // Warnings name the piece as the user wrote it, not its escaped DXF name.
        let piece_name = layout.blocks[members[0]].display_name.clone();
        let Some(&(sample_block, _)) = sized.iter().find(|(_, s)| *s == sample) else {
            warnings.push(format!("{piece_name}: not graded, size {sample_size} is not in the layout."));
            continue;
        }; // if no sample block

        match grade_group(layout, input, &sized, sample, sizes, &mut table) {
            Ok(()) => {
                // Nest order: this piece's blocks together, in size order.
                for &(b, s) in &sized {
                    order[b] = Some((group, s));
                } // for each member
            } // Ok
            Err(reason) => {
                // Sample size only: a nest with unmatched points would mislead the reader.
                warnings.push(format!("{piece_name}: not graded, {reason}. Only size {sample_size} is written."));
                for &(b, _) in sized.iter().filter(|(b, _)| *b != sample_block) {
                    dropped[b] = true;
                } // for each graded size
            } // Err
        } // match grade_group
    } // for each piece

    // Rebuild the block list: drop fallen-back sizes, and write each nest in size order
    // where its first block stood.
    let mut slots: Vec<(usize, usize, usize)> = Vec::new(); // (position, size index, block)
    let mut first_position: Vec<Option<usize>> = Vec::new();
    for (b, slot) in order.iter().enumerate() {
        let position = match slot {
            Some((group, _)) => {
                if first_position.len() <= *group {
                    first_position.resize(*group + 1, None);
                } // if new group
                *first_position[*group].get_or_insert(b)
            } // nest member
            None => b, // ungraded block keeps its place
        }; // position
        slots.push((position, slot.map_or(0, |(_, s)| s), b));
    } // for each block
    slots.sort_by_key(|&(position, size, block)| (position, size, block));
    let mut blocks: Vec<Option<Block>> = std::mem::take(&mut layout.blocks).into_iter().map(Some).collect();
    layout.blocks = slots
        .into_iter()
        .filter(|&(_, _, b)| !dropped[b])
        .filter_map(|(_, _, b)| blocks[b].take())
        .collect();

    if !table.rules.is_empty() {
        layout.grade_rule_table = Some(table);
    } // if any piece graded
    warnings
} // fn grade_drawing

// @brief Layout blocks grouped by piece, in first-appearance order.
// @details Key: `parent` (the piece-set id), else the piece name. Blocks without a size are left out.
fn nest_groups(layout: &Drawing) -> Vec<Vec<usize>> {
    let mut keys: Vec<String> = Vec::new();
    let mut groups: Vec<Vec<usize>> = Vec::new();
    for (b, block) in layout.blocks.iter().enumerate().filter(|(_, b)| b.size.is_some()) {
        let key = nest_key(block);
        match keys.iter().position(|k| *k == key) {
            Some(g) => groups[g].push(b), // known piece
            None => {
                keys.push(key);
                groups.push(vec![b]);
            } // new piece
        } // match key
    } // for each sized block
    groups
} // fn nest_groups

// @brief Piece key of a block: `parent`, else the piece name.
fn nest_key(block: &Block) -> String {
    block.parent.clone().unwrap_or_else(|| block.piece_name.clone())
} // fn nest_key

// @brief Grade one piece: graded lines and reference line on every member, `# n` texts on the sample.
// @param sized (layout block index, size index) of every member.
// @return Err(reason) when the piece cannot be graded; the layout is then unchanged.
fn grade_group(
    layout: &mut Drawing,
    input: &Drawing,
    sized: &[(usize, usize)],
    sample: usize,
    sizes: &[String],
    table: &mut GradeRuleTable,
) -> Result<(), String> {
    let key = nest_key(&layout.blocks[sized[0].0]);

    // Step 1: the input block of every size, from the same piece.
    let input_blocks: Vec<&Block> = sizes
        .iter()
        .map(|size| {
            input
                .blocks
                .iter()
                .find(|b| nest_key(b) == key && b.size.as_deref() == Some(size.as_str()))
                .ok_or_else(|| format!("size {size} is not in the handoff"))
        })
        .collect::<Result<_, _>>()?;
    let input_lines: Vec<PieceLines> = input_blocks
        .iter()
        .map(|b| PieceLines::of(b).ok_or_else(|| "a size has no boundary".to_string()))
        .collect::<Result<_, _>>()?;

    // Step 2: every size needs the same lines and notches.
    let line_count = input_lines[sample].all().len();
    let notch_count = input_blocks[sample].notches.len();
    for (lines, block) in input_lines.iter().zip(&input_blocks) {
        let layers: Vec<&str> = lines.all().iter().map(|(l, _)| *l).collect();
        let sample_layers: Vec<&str> = input_lines[sample].all().iter().map(|(l, _)| *l).collect();
        if layers != sample_layers {
            return Err("the sizes have different internal lines or sew lines".to_string());
        } // if lines differ
        if block.notches.len() != notch_count {
            return Err("the sizes have different notch counts".to_string());
        } // if notches differ
    } // for each size

    // Step 3: grade each line across the sizes, in input space.
    let mut graded: Vec<GradedLines> = vec![Vec::with_capacity(line_count); sizes.len()];
    for line in 0..line_count {
        let per_size: Vec<AstmContour> = input_lines.iter().map(|l| l.all()[line].1.clone()).collect();
        let result = grade_contours(&per_size, sample).ok_or_else(|| "the sizes have different turn points".to_string())?;
        for (size, contour) in result.into_iter().enumerate() {
            graded[size].push(contour);
        } // for each size
    } // for each line

    // Step 4: map each layout block from its input block.
    let mut fits: Vec<(usize, usize, Rigid)> = Vec::with_capacity(sized.len());
    for &(b, size) in sized {
        let layout_boundary = layout.blocks[b].boundary.as_ref().ok_or("a size has no boundary in the layout")?;
        let fit = fit_rigid(&input_lines[size].boundary.dense, &layout_boundary.dense)
            .or_else(|| fit_rigid(&input_lines[size].boundary.reduced, &layout_boundary.reduced))
            .ok_or_else(|| format!("size {} does not match its handoff shape", sizes[size]))?;
        fits.push((b, size, fit));
    } // for each member
    let sample_fit = fits.iter().find(|(_, s, _)| *s == sample).map(|f| f.2).ok_or("no sample size block")?;
    let sample_block = fits.iter().find(|(_, s, _)| *s == sample).map(|f| f.0).ok_or("no sample size block")?;
    if layout.blocks[sample_block].notches.len() != notch_count {
        return Err("the layout notches do not match the handoff".to_string());
    } // if notches differ

    // Step 5: rules. One per key point and per notch: growth from the sample size.
    let layers: Vec<&'static str> = input_lines[sample].all().iter().map(|(l, _)| *l).collect();
    let mut rule_texts = Vec::new();
    for (line, layer) in layers.iter().enumerate() {
        for (i, &p) in graded[sample][line].reduced.iter().enumerate() {
            let deltas: Vec<(f64, f64)> = graded.iter().map(|g| growth(g[line].reduced[i], p)).collect();
            let rule = intern_rule(table, deltas);
            rule_texts.push(RuleText { position: sample_fit.apply(p), layer, rule });
        } // for each key point
    } // for each line
    for (j, notch) in layout.blocks[sample_block].notches.iter().enumerate() {
        let base = input_blocks[sample].notches[j].base;
        let deltas: Vec<(f64, f64)> = input_blocks.iter().map(|b| growth(b.notches[j].base, base)).collect();
        let rule = intern_rule(table, deltas);
        // The text sits on the written notch base: a reader pairs them by XY.
        rule_texts.push(RuleText { position: notch.base, layer: "4", rule });
    } // for each notch

    // Step 6: grade reference line, one horizontal draft-space line per piece.
    let reference = reference_line(&input_lines[sample].boundary.dense);

    // Step 7: write the graded lines and grading into each layout block.
    for (b, size, fit) in fits {
        let block = &mut layout.blocks[b];
        let mut lines = graded[size].iter().map(|c| fit.contour(c));
        block.boundary = lines.next();
        block.sew_lines = lines.by_ref().take(input_lines[size].sew_lines.len()).collect();
        block.internal_lines = lines.by_ref().take(input_lines[size].internal_lines.len()).collect();
        block.cutouts = lines.collect();
        block.grading = Some(BlockGrading {
            reference_line: (fit.apply(reference.0), fit.apply(reference.1)),
            sample: size == sample,
            rule_texts: if size == sample { std::mem::take(&mut rule_texts) } else { Vec::new() },
        });
    } // for each member
    Ok(())
} // fn grade_group

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
