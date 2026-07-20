// project: SeamlyLayout
// author: slspencer, copyright 2026
// MIT License: https://opensource.org/licenses/MIT

// @brief Basic 2D point used throughout geometry helpers.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Point {
    // X coordinate.
    pub x: f32,
    // Y coordinate.
    pub y: f32,
}

impl Point {
    // @brief Construct a new point.
    // @param x Horizontal component.
    // @param y Vertical component.
    // @return A new `Point`.
    pub const fn new(x: f32, y: f32) -> Self {
        // Build the point from coordinates.
        Self { x, y }
    }
}

// @brief Affine 2D matrix matching the common SVG representation:
// | a c e |
// | b d f |
// | 0 0 1 |
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Matrix2D {
    // Horizontal scaling / rotation component.
    pub a: f32,
    // Vertical shear / rotation component.
    pub b: f32,
    // Horizontal shear / rotation component.
    pub c: f32,
    // Vertical scaling / rotation component.
    pub d: f32,
    // Horizontal translation.
    pub e: f32,
    // Vertical translation.
    pub f: f32,
}

impl Matrix2D {
    // @brief Identity matrix constant.
    pub const IDENTITY: Self = Self {
        a: 1.0,
        b: 0.0,
        c: 0.0,
        d: 1.0,
        e: 0.0,
        f: 0.0,
    };

    // @brief Create a translation matrix.
    // @param tx X offset.
    // @param ty Y offset.
    // @return Translation matrix.
    pub const fn from_translate(tx: f32, ty: f32) -> Self {
        // Apply offsets to translation slots.
        Self {
            e: tx,
            f: ty,
            ..Self::IDENTITY
        }
    }

    // @brief Create a scale matrix.
    // @param sx Scale on X axis.
    // @param sy Scale on Y axis.
    // @return Scaling matrix.
    pub const fn from_scale(sx: f32, sy: f32) -> Self {
        // Scale factors populate the diagonal.
        Self {
            a: sx,
            d: sy,
            ..Self::IDENTITY
        }
    }

    // @brief Create a rotation matrix in degrees.
    // @param deg Degrees to rotate counter-clockwise.
    // @return Rotation matrix.
    pub fn from_rotate(deg: f32) -> Self {
        // Convert degrees to radians for trig functions.
        let rad = deg.to_radians();
        let (sin, cos) = rad.sin_cos();
        Self {
            a: cos,
            b: sin,
            c: -sin,
            d: cos,
            e: 0.0,
            f: 0.0,
        }
    }

    // @brief Create an X-axis skew matrix.
    // @param deg Degrees of skew along X.
    // @return Skew matrix.
    pub fn from_skew_x(deg: f32) -> Self {
        // Convert to tangent for skew component.
        let t = deg.to_radians().tan();
        Self {
            a: 1.0,
            b: 0.0,
            c: t,
            d: 1.0,
            e: 0.0,
            f: 0.0,
        }
    }

    // @brief Create a Y-axis skew matrix.
    // @param deg Degrees of skew along Y.
    // @return Skew matrix.
    pub fn from_skew_y(deg: f32) -> Self {
        // Use tangent for the Y skew term.
        let t = deg.to_radians().tan();
        Self {
            a: 1.0,
            b: t,
            c: 0.0,
            d: 1.0,
            e: 0.0,
            f: 0.0,
        }
    }

    // @brief Reflect across the X axis.
    // @return Reflection matrix.
    pub const fn reflect_x() -> Self {
        // Flip Y while keeping X unchanged.
        Self {
            a: 1.0,
            b: 0.0,
            c: 0.0,
            d: -1.0,
            e: 0.0,
            f: 0.0,
        }
    }

    // @brief Reflect across the Y axis.
    // @return Reflection matrix.
    pub const fn reflect_y() -> Self {
        // Flip X while keeping Y unchanged.
        Self {
            a: -1.0,
            b: 0.0,
            c: 0.0,
            d: 1.0,
            e: 0.0,
            f: 0.0,
        }
    }

    // @brief Apply this matrix to a point.
    // @param p Point to transform.
    // @return Transformed point.
    pub fn apply_to_point(&self, p: Point) -> Point {
        // Multiply by matrix and add translation.
        Point {
            x: self.a * p.x + self.c * p.y + self.e,
            y: self.b * p.x + self.d * p.y + self.f,
        }
    }

    // @brief Compose this matrix with another (self * other).
    // @param other Matrix to post-multiply.
    // @return Combined matrix.
    pub fn mul(&self, other: &Self) -> Self {
        // Perform affine matrix multiplication.
        Self {
            a: self.a * other.a + self.c * other.b,
            b: self.b * other.a + self.d * other.b,
            c: self.a * other.c + self.c * other.d,
            d: self.b * other.c + self.d * other.d,
            e: self.a * other.e + self.c * other.f + self.e,
            f: self.b * other.e + self.d * other.f + self.f,
        }
    }
}

// @brief Axis-aligned bounding box helper used to validate geometry ops.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct BoundingBox {
    // Minimum corner.
    pub min: Point,
    // Maximum corner.
    pub max: Point,
}

impl BoundingBox {
    // @brief Compute box width.
    // @return Width along X.
    pub fn width(&self) -> f32 {
        // Subtract min from max for width.
        self.max.x - self.min.x
    }

    // @brief Compute box height.
    // @return Height along Y.
    pub fn height(&self) -> f32 {
        // Subtract min from max for height.
        self.max.y - self.min.y
    }

    // @brief Expand the box to include a point.
    // @param p Point to include.
    // @return Expanded box.
    pub fn expand_to_include(mut self, p: Point) -> Self {
        // Grow bounds if the point lies outside current range.
        self.min.x = self.min.x.min(p.x);
        self.min.y = self.min.y.min(p.y);
        self.max.x = self.max.x.max(p.x);
        self.max.y = self.max.y.max(p.y);
        self
    }

    // @brief Build a bounding box from an iterator of points.
    // @param points Iterable of points.
    // @return Bounding box or None if empty.
    pub fn from_points<I: IntoIterator<Item = Point>>(points: I) -> Option<Self> {
        // Seed from the first point to avoid defaults.
        let mut iter = points.into_iter();
        let first = iter.next()?;
        let mut bbox = Self {
            min: first,
            max: first,
        };
        // Expand using remaining points.
        for p in iter {
            bbox = bbox.expand_to_include(p);
        }
        Some(bbox)
    }
}

// @brief Apply a matrix to all points and return the resulting bounding box.
// @param points Collection of points to transform.
// @param matrix Matrix to apply.
// @return Bounding box of transformed points.
pub fn bbox_after_transform<I: IntoIterator<Item = Point>>(
    points: I,
    matrix: &Matrix2D,
) -> Option<BoundingBox> {
    // Transform each point then derive the bounding box.
    let transformed = points.into_iter().map(|p| matrix.apply_to_point(p));
    BoundingBox::from_points(transformed)
}

// @brief Simple SVG path representation with enough fidelity for bbox and transforms.
#[derive(Debug, Clone, PartialEq)]
pub enum PathSegment {
    // Move pen to a point.
    MoveTo(Point),
    // Draw straight line to a point.
    LineTo(Point),
    // Quadratic curve segment.
    QuadTo { ctrl: Point, to: Point },
    // Cubic curve segment.
    CubicTo {
        ctrl1: Point,
        ctrl2: Point,
        to: Point,
    },
    // Elliptical arc segment.
    ArcTo {
        // X radius.
        rx: f32,
        // Y radius.
        ry: f32,
        // Rotation in degrees of the arc's x-axis.
        x_axis_rotation: f32,
        // Large-arc flag.
        large_arc: bool,
        // Sweep flag.
        sweep: bool,
        // Destination point.
        to: Point,
    },
    // Close the current subpath.
    Close,
}

// @brief SVG path container.
#[derive(Debug, Clone, PartialEq, Default)]
pub struct Path {
    // Ordered list of path segments.
    pub segments: Vec<PathSegment>,
}

impl Path {
    // @brief Create an empty path.
    // @return New path instance.
    pub fn new() -> Self {
        // Start with no segments.
        Self {
            segments: Vec::new(),
        }
    }

    // @brief Parse a minimal subset of SVG path data.
    // @param data SVG path data string.
    // @return Parsed path or parse error.
    pub fn parse_path_attribute(data: &str) -> Result<Self, PathParseError> {
        // Tokenize the data for incremental parsing.
        let mut tokens = Tokenizer::new(data);
        let mut path = Path::new();
        let mut cursor = Point::new(0.0, 0.0);
        let mut subpath_start = Point::new(0.0, 0.0);
        let mut last_cmd: Option<char> = None;

        // Process tokens until exhausted.
        while let Some(tok) = tokens.next_token()? {
            let cmd = match tok {
                Token::Cmd(c) => {
                    last_cmd = Some(c);
                    c
                }
                Token::Num(_) => {
                    // Reuse the last command when numbers continue the previous one.
                    let c = last_cmd.ok_or(PathParseError::InvalidCommandSequence)?;
                    tokens.push_back(tok);
                    c
                }
            };

            match cmd {
                'M' | 'm' => {
                    let is_rel = cmd == 'm';
                    let (x, y) = tokens.read_pair()?;
                    let p = if is_rel {
                        Point::new(cursor.x + x, cursor.y + y)
                    } else {
                        Point::new(x, y)
                    };
                    path = path.move_to(p);
                    cursor = p;
                    subpath_start = p;
                    // Subsequent pairs are treated as LineTos.
                    while tokens.peek_is_num()? {
                        let (x, y) = tokens.read_pair()?;
                        let p = if is_rel {
                            Point::new(cursor.x + x, cursor.y + y)
                        } else {
                            Point::new(x, y)
                        };
                        path = path.line_to(p);
                        cursor = p;
                    }
                }
                'L' | 'l' => {
                    let is_rel = cmd == 'l';
                    while tokens.peek_is_num()? {
                        let (x, y) = tokens.read_pair()?;
                        let p = if is_rel {
                            Point::new(cursor.x + x, cursor.y + y)
                        } else {
                            Point::new(x, y)
                        };
                        path = path.line_to(p);
                        cursor = p;
                    }
                }
                'H' | 'h' => {
                    let is_rel = cmd == 'h';
                    while let Some(n) = tokens.next_number()? {
                        let x = if is_rel { cursor.x + n } else { n };
                        cursor = Point::new(x, cursor.y);
                        // Horizontal move becomes a line segment.
                        path = path.line_to(cursor);
                    }
                }
                'V' | 'v' => {
                    let is_rel = cmd == 'v';
                    while let Some(n) = tokens.next_number()? {
                        let y = if is_rel { cursor.y + n } else { n };
                        cursor = Point::new(cursor.x, y);
                        // Vertical move becomes a line segment.
                        path = path.line_to(cursor);
                    }
                }
                'C' | 'c' => {
                    let is_rel = cmd == 'c';
                    while tokens.peek_is_num()? {
                        let (x1, y1) = tokens.read_pair()?;
                        let (x2, y2) = tokens.read_pair()?;
                        let (x, y) = tokens.read_pair()?;
                        let ctrl1 = if is_rel {
                            Point::new(cursor.x + x1, cursor.y + y1)
                        } else {
                            Point::new(x1, y1)
                        };
                        let ctrl2 = if is_rel {
                            Point::new(cursor.x + x2, cursor.y + y2)
                        } else {
                            Point::new(x2, y2)
                        };
                        let to = if is_rel {
                            Point::new(cursor.x + x, cursor.y + y)
                        } else {
                            Point::new(x, y)
                        };
                        // Add cubic segment and advance cursor.
                        path = path.cubic_to(ctrl1, ctrl2, to);
                        cursor = to;
                    }
                }
                'Q' | 'q' => {
                    let is_rel = cmd == 'q';
                    while tokens.peek_is_num()? {
                        let (x1, y1) = tokens.read_pair()?;
                        let (x, y) = tokens.read_pair()?;
                        let ctrl = if is_rel {
                            Point::new(cursor.x + x1, cursor.y + y1)
                        } else {
                            Point::new(x1, y1)
                        };
                        let to = if is_rel {
                            Point::new(cursor.x + x, cursor.y + y)
                        } else {
                            Point::new(x, y)
                        };
                        // Add quadratic segment and advance cursor.
                        path = path.quad_to(ctrl, to);
                        cursor = to;
                    }
                }
                'A' | 'a' => {
                    let is_rel = cmd == 'a';
                    while tokens.peek_is_num()? {
                        let rx = tokens.next_number()?.ok_or(PathParseError::UnexpectedEof)?;
                        let ry = tokens.next_number()?.ok_or(PathParseError::UnexpectedEof)?;
                        let rot = tokens.next_number()?.ok_or(PathParseError::UnexpectedEof)?;
                        let laf = tokens.next_flag()?;
                        let sf = tokens.next_flag()?;
                        let (x, y) = tokens.read_pair()?;
                        let to = if is_rel {
                            Point::new(cursor.x + x, cursor.y + y)
                        } else {
                            Point::new(x, y)
                        };
                        // Add arc segment and advance cursor.
                        path = path.arc_to(rx, ry, rot, laf, sf, to);
                        cursor = to;
                    }
                }
                'Z' | 'z' => {
                    // Close current subpath and snap cursor back to start.
                    path = path.close();
                    cursor = subpath_start;
                }
                _ => return Err(PathParseError::InvalidCommand(cmd)),
            }
        }

        Ok(path)
    }

    // @brief Append a MoveTo segment.
    // @param p Destination point.
    // @return Updated path.
    pub fn move_to(mut self, p: Point) -> Self {
        // Record move and return builder.
        self.segments.push(PathSegment::MoveTo(p));
        self
    }

    // @brief Append a LineTo segment.
    // @param p Destination point.
    // @return Updated path.
    pub fn line_to(mut self, p: Point) -> Self {
        // Record line and return builder.
        self.segments.push(PathSegment::LineTo(p));
        self
    }

    // @brief Append a CubicTo segment.
    // @param ctrl1 First control point.
    // @param ctrl2 Second control point.
    // @param to Destination point.
    // @return Updated path.
    pub fn cubic_to(mut self, ctrl1: Point, ctrl2: Point, to: Point) -> Self {
        // Record cubic curve and return builder.
        self.segments
            .push(PathSegment::CubicTo { ctrl1, ctrl2, to });
        self
    }

    // @brief Append a QuadTo segment.
    // @param ctrl Control point.
    // @param to Destination point.
    // @return Updated path.
    pub fn quad_to(mut self, ctrl: Point, to: Point) -> Self {
        // Record quadratic curve and return builder.
        self.segments.push(PathSegment::QuadTo { ctrl, to });
        self
    }

    // @brief Append an ArcTo segment.
    // @param rx X radius.
    // @param ry Y radius.
    // @param x_axis_rotation Rotation in degrees.
    // @param large_arc Large-arc flag.
    // @param sweep Sweep flag.
    // @param to Destination point.
    // @return Updated path.
    pub fn arc_to(
        mut self,
        rx: f32,
        ry: f32,
        x_axis_rotation: f32,
        large_arc: bool,
        sweep: bool,
        to: Point,
    ) -> Self {
        // Record arc definition and return builder.
        self.segments.push(PathSegment::ArcTo {
            rx,
            ry,
            x_axis_rotation,
            large_arc,
            sweep,
            to,
        });
        self
    }

    // @brief Close the current subpath.
    // @return Updated path.
    pub fn close(mut self) -> Self {
        // Push a Close segment to terminate the subpath.
        self.segments.push(PathSegment::Close);
        self
    }

    // @brief Apply a transform to all segments.
    // @param m Matrix to apply.
    // @return Transformed path.
    pub fn transform(&self, m: &Matrix2D) -> Self {
        // Map each segment through the matrix.
        let segments = self
            .segments
            .iter()
            .map(|seg| match seg {
                PathSegment::MoveTo(p) => PathSegment::MoveTo(m.apply_to_point(*p)),
                PathSegment::LineTo(p) => PathSegment::LineTo(m.apply_to_point(*p)),
                PathSegment::QuadTo { ctrl, to } => PathSegment::QuadTo {
                    ctrl: m.apply_to_point(*ctrl),
                    to: m.apply_to_point(*to),
                },
                PathSegment::CubicTo { ctrl1, ctrl2, to } => PathSegment::CubicTo {
                    ctrl1: m.apply_to_point(*ctrl1),
                    ctrl2: m.apply_to_point(*ctrl2),
                    to: m.apply_to_point(*to),
                },
                PathSegment::ArcTo {
                    rx,
                    ry,
                    x_axis_rotation,
                    large_arc,
                    sweep,
                    to,
                } => PathSegment::ArcTo {
                    rx: *rx,
                    ry: *ry,
                    x_axis_rotation: *x_axis_rotation,
                    large_arc: *large_arc,
                    sweep: *sweep,
                    to: m.apply_to_point(*to),
                },
                PathSegment::Close => PathSegment::Close,
            })
            .collect();
        Self { segments }
    }

    // @brief Compute the bounding box of the path.
    // @return Bounding box or None if empty.
    pub fn bounding_box(&self) -> Option<BoundingBox> {
        // Collect representative points from each segment.
        let mut points = Vec::new();
        let mut cursor: Option<Point> = None;
        let mut start_of_subpath: Option<Point> = None;

        for seg in &self.segments {
            match *seg {
                PathSegment::MoveTo(p) => {
                    cursor = Some(p);
                    start_of_subpath = Some(p);
                    points.push(p);
                }
                PathSegment::LineTo(p) => {
                    cursor = Some(p);
                    points.push(p);
                }
                PathSegment::QuadTo { ctrl, to } => {
                    cursor = Some(to);
                    points.push(ctrl);
                    points.push(to);
                }
                PathSegment::CubicTo { ctrl1, ctrl2, to } => {
                    cursor = Some(to);
                    points.push(ctrl1);
                    points.push(ctrl2);
                    points.push(to);
                }
                PathSegment::ArcTo { to, .. } => {
                    cursor = Some(to);
                    points.push(to);
                }
                PathSegment::Close => {
                    if let (Some(start), Some(cur)) = (start_of_subpath, cursor) {
                        // Include closing segment endpoints.
                        points.push(start);
                        points.push(cur);
                        cursor = Some(start);
                    }
                }
            }
        }

        BoundingBox::from_points(points)
    }

    // @brief Flatten the path into polyline points (including the initial MoveTo).
    // @param tolerance Maximum chord error tolerated.
    // @return Sequence of approximated points.
    pub fn flatten(&self, tolerance: f32) -> Vec<Point> {
        // Collect discretized points for each segment.
        let mut out = Vec::new();
        let mut cursor = Point::new(0.0, 0.0);
        let mut start_of_subpath = Point::new(0.0, 0.0);
        let tol = tolerance.max(0.001);

        for seg in &self.segments {
            match *seg {
                PathSegment::MoveTo(p) => {
                    cursor = p;
                    start_of_subpath = p;
                    out.push(p);
                }
                PathSegment::LineTo(p) => {
                    out.push(p);
                    cursor = p;
                }
                PathSegment::QuadTo { ctrl, to } => {
                    let steps = steps_for_tolerance(&[cursor, ctrl, to], tol);
                    for i in 1..=steps {
                        let t = i as f32 / steps as f32;
                        // Subdivide quadratic curve.
                        out.push(quad_point(cursor, ctrl, to, t));
                    }
                    cursor = to;
                }
                PathSegment::CubicTo { ctrl1, ctrl2, to } => {
                    let steps = steps_for_tolerance(&[cursor, ctrl1, ctrl2, to], tol);
                    for i in 1..=steps {
                        let t = i as f32 / steps as f32;
                        // Subdivide cubic curve.
                        out.push(cubic_point(cursor, ctrl1, ctrl2, to, t));
                    }
                    cursor = to;
                }
                PathSegment::ArcTo {
                    rx,
                    ry,
                    x_axis_rotation,
                    large_arc,
                    sweep,
                    to,
                } => {
                    // Convert arc to polyline approximation.
                    let arc_points = arc_to_points(
                        cursor,
                        to,
                        rx,
                        ry,
                        x_axis_rotation.to_radians(),
                        large_arc,
                        sweep,
                        tol,
                    );
                    for p in arc_points {
                        out.push(p);
                    }
                    cursor = to;
                }
                PathSegment::Close => {
                    // Close subpath if not already at the start.
                    if cursor != start_of_subpath {
                        out.push(start_of_subpath);
                        cursor = start_of_subpath;
                    }
                }
            }
        }
        out
    }
}

// @brief Errors that can occur while parsing path data.
#[derive(Debug, thiserror::Error, PartialEq)]
pub enum PathParseError {
    // End of data reached unexpectedly.
    #[error("unexpected end of data")]
    UnexpectedEof,
    // Number token failed to parse.
    #[error("invalid number: {0}")]
    InvalidNumber(String),
    // Flag token failed to parse.
    #[error("invalid flag: {0}")]
    InvalidFlag(String),
    // Command character not recognized.
    #[error("invalid command: {0}")]
    InvalidCommand(char),
    // Number found without a preceding command.
    #[error("command sequence is invalid")]
    InvalidCommandSequence,
}

// @brief Token types used during path parsing.
#[derive(Debug, Clone)]
enum Token {
    // Command character token.
    Cmd(char),
    // Numeric literal token.
    Num(f32),
}

// @brief Streaming tokenizer for SVG path data.
struct Tokenizer<'a> {
    // Peekable character iterator.
    iter: std::iter::Peekable<std::str::Chars<'a>>,
    // Tokens to re-consume.
    pushback: Vec<Token>,
}

impl<'a> Tokenizer<'a> {
    // @brief Construct a tokenizer over the provided data.
    // @param data Raw SVG path string.
    // @return New tokenizer.
    fn new(data: &'a str) -> Self {
        // Wrap chars in a peekable iterator.
        Self {
            iter: data.chars().peekable(),
            pushback: Vec::new(),
        }
    }

    // @brief Push a token back for later consumption.
    // @param tok Token to push back.
    fn push_back(&mut self, tok: Token) {
        // Store for next access.
        self.pushback.push(tok);
    }

    // @brief Read the next token (command or number).
    // @return Optional token or parse error.
    fn next_token(&mut self) -> Result<Option<Token>, PathParseError> {
        // Return pushback first.
        if let Some(tok) = self.pushback.pop() {
            return Ok(Some(tok));
        }
        self.skip_ws();
        let ch = match self.iter.peek().copied() {
            Some(c) => c,
            None => return Ok(None),
        };
        if is_cmd_char(ch) {
            // Consume command character.
            self.iter.next();
            return Ok(Some(Token::Cmd(ch)));
        }
        // Otherwise parse number.
        let num = self.read_number()?;
        Ok(Some(Token::Num(num)))
    }

    // @brief Try reading the next number token.
    // @return Number or None if next token is not numeric.
    fn next_number(&mut self) -> Result<Option<f32>, PathParseError> {
        // Attempt to parse number, rewinding if not numeric.
        match self.next_token()? {
            Some(Token::Num(n)) => Ok(Some(n)),
            Some(tok) => {
                self.push_back(tok);
                Ok(None)
            }
            None => Ok(None),
        }
    }

    // @brief Read two consecutive numbers as a pair.
    // @return Tuple of parsed floats.
    fn read_pair(&mut self) -> Result<(f32, f32), PathParseError> {
        // Sequentially parse x then y.
        let x = self.next_number()?.ok_or(PathParseError::UnexpectedEof)?;
        let y = self.next_number()?.ok_or(PathParseError::UnexpectedEof)?;
        Ok((x, y))
    }

    // @brief Read a flag value (0 or 1).
    // @return Boolean flag.
    fn next_flag(&mut self) -> Result<bool, PathParseError> {
        // Parse numeric token and validate allowed values.
        let n = self.next_number()?.ok_or(PathParseError::UnexpectedEof)?;
        if (n - 0.0).abs() < f32::EPSILON {
            Ok(false)
        } else if (n - 1.0).abs() < f32::EPSILON {
            Ok(true)
        } else {
            Err(PathParseError::InvalidFlag(format!("{n}")))
        }
    }

    // @brief Peek to see if next token is numeric.
    // @return True if next token is a number.
    fn peek_is_num(&mut self) -> Result<bool, PathParseError> {
        // Temporarily consume then push back token.
        match self.next_token()? {
            Some(Token::Num(n)) => {
                self.push_back(Token::Num(n));
                Ok(true)
            }
            Some(tok) => {
                self.push_back(tok);
                Ok(false)
            }
            None => Ok(false),
        }
    }

    // @brief Skip whitespace and commas.
    fn skip_ws(&mut self) {
        // Advance until a non-separator character is found.
        while let Some(c) = self.iter.peek().copied() {
            if c.is_whitespace() || c == ',' {
                self.iter.next();
            } else {
                break;
            }
        }
    }

    // @brief Parse a floating-point literal.
    // @return Parsed float.
    fn read_number(&mut self) -> Result<f32, PathParseError> {
        // Build string buffer of numeric characters.
        let mut buf = String::new();
        while let Some(c) = self.iter.peek().copied() {
            if c.is_ascii_alphabetic() || c == ',' || c.is_whitespace() {
                break;
            }
            if c == '-' || c == '+' || c == '.' || c.is_ascii_digit() || c == 'e' || c == 'E' {
                buf.push(c);
                self.iter.next();
            } else {
                break;
            }
        }
        if buf.is_empty() {
            return Err(PathParseError::UnexpectedEof);
        }
        // Convert string to float or report parse error.
        buf.parse::<f32>()
            .map_err(|e| PathParseError::InvalidNumber(e.to_string()))
    }
}

// @brief Check if a character is a valid SVG command.
// @param c Character to inspect.
// @return True if it matches a known command.
fn is_cmd_char(c: char) -> bool {
    // Match against the SVG path command set.
    matches!(
        c,
        'M' | 'm'
            | 'L'
            | 'l'
            | 'H'
            | 'h'
            | 'V'
            | 'v'
            | 'C'
            | 'c'
            | 'Q'
            | 'q'
            | 'A'
            | 'a'
            | 'Z'
            | 'z'
    )
}

// @brief Heuristic step count based on tolerance and control points.
// @param points Control points influencing curvature.
// @param tolerance Maximum allowed chord error.
// @return Number of segments to approximate the curve.
fn steps_for_tolerance(points: &[Point], tolerance: f32) -> usize {
    // Estimate required steps; tighter tolerance increases samples.
    let count = points.len().saturating_sub(1) as f32;
    let steps = ((count * 4.0) / tolerance.max(0.0001)).sqrt();
    steps.clamp(4.0, 50.0).ceil() as usize
}

// @brief Evaluate a quadratic Bezier at parameter t.
// @param p0 Start point.
// @param p1 Control point.
// @param p2 End point.
// @param t Parameter from 0..1.
// @return Interpolated point.
fn quad_point(p0: Point, p1: Point, p2: Point, t: f32) -> Point {
    // Use Bernstein basis for quadratic evaluation.
    let mt = 1.0 - t;
    Point {
        x: mt * mt * p0.x + 2.0 * mt * t * p1.x + t * t * p2.x,
        y: mt * mt * p0.y + 2.0 * mt * t * p1.y + t * t * p2.y,
    }
}

// @brief Evaluate a cubic Bezier at parameter t.
// @param p0 Start point.
// @param p1 First control point.
// @param p2 Second control point.
// @param p3 End point.
// @param t Parameter from 0..1.
// @return Interpolated point.
fn cubic_point(p0: Point, p1: Point, p2: Point, p3: Point, t: f32) -> Point {
    // Use Bernstein basis for cubic evaluation.
    let mt = 1.0 - t;
    let mt2 = mt * mt;
    let t2 = t * t;
    Point {
        x: mt2 * mt * p0.x + 3.0 * mt2 * t * p1.x + 3.0 * mt * t2 * p2.x + t2 * t * p3.x,
        y: mt2 * mt * p0.y + 3.0 * mt2 * t * p1.y + 3.0 * mt * t2 * p2.y + t2 * t * p3.y,
    }
}

// @brief Convert an SVG arc command to polyline points.
// @param from Start point.
// @param to End point.
// @param rx X radius.
// @param ry Y radius.
// @param x_axis_rotation Rotation in radians.
// @param large_arc Large-arc flag.
// @param sweep Sweep flag.
// @param tolerance Allowed chord error.
// @return Polyline approximating the arc.
fn arc_to_points(
    from: Point,
    to: Point,
    mut rx: f32,
    mut ry: f32,
    x_axis_rotation: f32,
    large_arc: bool,
    sweep: bool,
    tolerance: f32,
) -> Vec<Point> {
    // Implementation adapted from SVG spec section F.6 (elliptical arc implementation notes).
    if rx.abs() < f32::EPSILON || ry.abs() < f32::EPSILON {
        // Degenerate arc reduces to the endpoint.
        return vec![to];
    }

    let phi = x_axis_rotation;
    let cos_phi = phi.cos();
    let sin_phi = phi.sin();

    // Step 1: compute (x1', y1')
    let dx2 = (from.x - to.x) / 2.0;
    let dy2 = (from.y - to.y) / 2.0;
    let x1p = cos_phi * dx2 + sin_phi * dy2;
    let y1p = -sin_phi * dx2 + cos_phi * dy2;

    // Ensure radii are large enough.
    let mut rx_sq = rx * rx;
    let mut ry_sq = ry * ry;
    let x1p_sq = x1p * x1p;
    let y1p_sq = y1p * y1p;
    let radii_check = x1p_sq / rx_sq + y1p_sq / ry_sq;
    if radii_check > 1.0 {
        // Scale radii when the endpoints are too far apart.
        let scale = radii_check.sqrt();
        rx *= scale;
        ry *= scale;
        rx_sq = rx * rx;
        ry_sq = ry * ry;
    }

    // Step 2: compute (cx', cy')
    let sign = if large_arc == sweep { -1.0 } else { 1.0 };
    let sq = ((rx_sq * ry_sq - rx_sq * y1p_sq - ry_sq * x1p_sq)
        / (rx_sq * y1p_sq + ry_sq * x1p_sq))
        .max(0.0);
    let coef = sign * sq.sqrt();
    let cxp = coef * (rx * y1p) / ry;
    let cyp = coef * -(ry * x1p) / rx;

    // Step 3: compute (cx, cy)
    let cx = cos_phi * cxp - sin_phi * cyp + (from.x + to.x) / 2.0;
    let cy = sin_phi * cxp + cos_phi * cyp + (from.y + to.y) / 2.0;

    // Step 4: compute angles
    let ux = (x1p - cxp) / rx;
    let uy = (y1p - cyp) / ry;
    let vx = (-x1p - cxp) / rx;
    let vy = (-y1p - cyp) / ry;

    let theta = angle(1.0, 0.0, ux, uy);
    let mut delta = angle(ux, uy, vx, vy);

    if !sweep && delta > 0.0 {
        // Adjust delta direction when sweep flag is false.
        delta -= 2.0 * std::f32::consts::PI;
    } else if sweep && delta < 0.0 {
        // Ensure correct sweep direction.
        delta += 2.0 * std::f32::consts::PI;
    }

    // Step 5: approximate the arc using polyline.
    let radius = rx.max(ry);
    let chord_tol = tolerance.max(0.01);
    let est = (delta.abs() * radius / chord_tol).ceil();
    let segments = est.clamp(6.0, 256.0) as usize;
    let mut points = Vec::with_capacity(segments);
    for i in 1..=segments {
        let t = i as f32 / segments as f32;
        let angle = theta + delta * t;
        let x = rx * angle.cos();
        let y = ry * angle.sin();
        // Rotate back to original coordinate system.
        let xp = cos_phi * x - sin_phi * y + cx;
        let yp = sin_phi * x + cos_phi * y + cy;
        points.push(Point::new(xp, yp));
    }
    if points.last().copied() != Some(to) {
        // Ensure the exact endpoint is included.
        points.push(to);
    }
    points
}

// @brief Compute the signed angle between vectors u and v.
// @param ux U vector x component.
// @param uy U vector y component.
// @param vx V vector x component.
// @param vy V vector y component.
// @return Signed angle in radians.
fn angle(ux: f32, uy: f32, vx: f32, vy: f32) -> f32 {
    // Use atan2 of determinant and dot product.
    let dot = ux * vx + uy * vy;
    let det = ux * vy - uy * vx;
    det.atan2(dot)
}

// @brief Geometry unit tests.
#[cfg(test)]
mod tests {
    use super::*;

    // @brief Ensure translation and scaling compose correctly on a point.
    #[test]
    fn translate_and_scale_point() {
        // Transform a point and verify expected coordinates.
        let p = Point::new(10.0, -2.0);
        let m = Matrix2D::from_translate(5.0, 3.0).mul(&Matrix2D::from_scale(2.0, 4.0));
        let result = m.apply_to_point(p);
        assert_eq!(result, Point::new(25.0, -5.0));
    }

    // @brief Rotation should preserve vector length.
    #[test]
    fn rotation_preserves_length() {
        // Rotate unit vector and confirm magnitude is unchanged.
        let p = Point::new(1.0, 0.0);
        let m = Matrix2D::from_rotate(90.0);
        let rotated = m.apply_to_point(p);
        let len_sq = rotated.x.powi(2) + rotated.y.powi(2);
        assert!((len_sq - 1.0).abs() < 1e-5);
    }

    // @brief Verify bounding box after translation.
    #[test]
    fn bbox_after_matrix_transform() {
        // Translate four corners and compute bbox.
        let pts = [
            Point::new(0.0, 0.0),
            Point::new(10.0, 0.0),
            Point::new(0.0, 5.0),
            Point::new(10.0, 5.0),
        ];
        let m = Matrix2D::from_translate(2.0, -1.0);
        let bbox = bbox_after_transform(pts, &m).unwrap();
        assert_eq!(bbox.min, Point::new(2.0, -1.0));
        assert_eq!(bbox.max, Point::new(12.0, 4.0));
        assert_eq!(bbox.width(), 10.0);
        assert_eq!(bbox.height(), 5.0);
    }

    // @brief Validate skew and reflection helper matrices.
    #[test]
    fn skew_and_reflect_matrices() {
        // Skew a point and mirror it across X.
        let p = Point::new(2.0, 3.0);
        let skewed = Matrix2D::from_skew_x(45.0).apply_to_point(p);
        assert!((skewed.x - 5.0).abs() < 1e-4);
        let reflected = Matrix2D::reflect_x().apply_to_point(skewed);
        assert_eq!(reflected.y, -skewed.y);
    }

    // @brief Bounding box should track transformations.
    #[test]
    fn path_bbox_and_transform() {
        // Build rectangle, get bbox, transform, and re-evaluate.
        let path = Path::new()
            .move_to(Point::new(0.0, 0.0))
            .line_to(Point::new(10.0, 0.0))
            .line_to(Point::new(10.0, 5.0))
            .line_to(Point::new(0.0, 5.0))
            .close();
        let bbox = path.bounding_box().unwrap();
        assert_eq!(bbox.width(), 10.0);
        assert_eq!(bbox.height(), 5.0);

        let skew = Matrix2D::from_skew_x(45.0);
        let transformed = path.transform(&skew);
        let bbox2 = transformed.bounding_box().unwrap();
        assert_eq!(bbox2.min, Point::new(0.0, 0.0));
        assert_eq!(bbox2.max, Point::new(15.0, 5.0));
    }

    // @brief Flatten cubic and arc segments and validate endpoints.
    #[test]
    fn cubic_and_arc_flattening() {
        // Build mixed path and ensure flattening preserves end point.
        let p0 = Point::new(0.0, 0.0);
        let c1 = Point::new(5.0, 10.0);
        let c2 = Point::new(10.0, 10.0);
        let p3 = Point::new(15.0, 0.0);
        let arc_end = Point::new(30.0, 0.0);

        let path = Path::new()
            .move_to(p0)
            .cubic_to(c1, c2, p3)
            .arc_to(7.5, 7.5, 0.0, false, true, arc_end);

        let pts = path.flatten(0.5);
        assert!(pts.len() > 6);
        let end = pts.last().copied().unwrap();
        assert!((end.x - arc_end.x).abs() < 1e-4);
        assert!((end.y - arc_end.y).abs() < 1e-4);
    }

    // @brief Parse and evaluate a simple path.
    #[test]
    fn parse_simple_path() {
        // Simple rectangle path parsed from SVG data.
        let p = Path::parse_path_attribute("M0 0 L10 0 l0 5 z").unwrap();
        let bbox = p.bounding_box().unwrap();
        assert_eq!(bbox.max, Point::new(10.0, 5.0));
    }

    // @brief Parse and flatten an arc command.
    #[test]
    fn parse_arc_path() {
        // Basic arc path should end at specified coordinates.
        let p = Path::parse_path_attribute("M0 0 A 5 5 0 0 1 10 0").unwrap();
        let pts = p.flatten(0.25);
        let end = pts.last().copied().unwrap();
        assert!((end.x - 10.0).abs() < 1e-4);
        assert!(end.y.abs() < 1e-3);
    }

    // @brief Parse a fixture path excerpt.
    #[test]
    fn parse_logo_path_fixture() {
        // Snippet from seamly-layout.svg path74.
        let data = "m 2896.0528,408.62033 l -14.6201,-89.54234 c 76.6263,-31.36519 43.7069,-158.3149 43.7069,-158.3149 l 67.7364,-28.22212 c 8.3795,57.368 5.5872,107.61324 79.1968,108.71552 l 0.1356,167.37243 z";
        let p = Path::parse_path_attribute(data).unwrap();
        let pts = p.flatten(1.0);
        assert!(pts.len() > 10);
    }
}
