FEATURE: Piece Material / Component Classification
LOCATION: Seamly2D > Piece Mode > Piece context menu / Piece properties

PURPOSE
Add structured metadata to each Seamly2D piece so the user can identify what
the piece represents in the finished sewn product.

The classification must distinguish between:
1. Textiles
2. Trim Materials
3. Trim Components / Hardware
4. Other

This feature describes WHAT the piece represents.

Do not mix manufacturing operations (cut, sew, bond, stretch during
application, etc.) into the primary classification. Manufacturing operations
should be represented separately.

============================================================
1. PRIMARY CLASSIFICATION
============================================================

Field:
    pieceType

Control:
    Radio buttons

Values:
    textile
    trim_material
    trim_component
    other

Display labels:
    Textile
    Trim Material
    Trim Component / Hardware
    Other

Default:
    textile

Only one pieceType may be selected.

Use progressive disclosure. Fields that are not relevant to the selected
pieceType should be hidden.

============================================================
2. TEXTILE
============================================================

Displayed when:
    pieceType == textile

Field:
    textileType

Control:
    Dropdown / combo box

Values:
    shell
    lining
    interfacing
    interlining
    padding_insulation
    reinforcement
    other

Display labels:
    Shell / Main Fabric
    Lining
    Interfacing
    Interlining
    Padding / Insulation
    Reinforcement
    Other

Default:
    shell

If textileType == other:

Field:
    textileTypeOther

Control:
    Text field

Display label:
    Description

============================================================
3. TRIM MATERIAL
============================================================

Displayed when:
    pieceType == trim_material

Field:
    trimMaterialType

Control:
    Dropdown / combo box

Values:
    elastic
    tape
    binding
    webbing
    cord_drawcord
    piping_welting
    boning_stay
    ribbon_braid
    lace_decorative
    reflective
    seam_sealing_bonding_tape
    other

Display labels:
    Elastic
    Tape
    Binding
    Webbing
    Cord / Drawcord
    Piping / Welting
    Boning / Stay
    Ribbon / Braid
    Lace / Decorative Trim
    Reflective Trim
    Seam-Sealing / Bonding Tape
    Other


------------------------------------------------------------
3A. ELASTIC SUBTYPE
------------------------------------------------------------

Displayed when:
    trimMaterialType == elastic

Field:
    trimMaterialSubtype

Control:
    Dropdown / combo box

Values:
    braided_elastic
    knitted_elastic
    woven_elastic
    clear_elastic
    fold_over_elastic
    elastic_cord
    buttonhole_elastic
    other

Display labels:
    Braided Elastic
    Knitted Elastic
    Woven Elastic
    Clear Elastic
    Fold-Over Elastic
    Elastic Cord
    Buttonhole Elastic
    Other


------------------------------------------------------------
3B. TAPE SUBTYPE
------------------------------------------------------------

Displayed when:
    trimMaterialType == tape

Field:
    trimMaterialSubtype

Control:
    Dropdown / combo box

Values:
    twill_tape
    hem_tape
    stay_tape
    seam_tape
    reinforcement_tape
    grosgrain_tape
    herringbone_tape
    other

Display labels:
    Twill Tape
    Hem Tape
    Stay Tape
    Seam Tape
    Reinforcement Tape
    Grosgrain Tape
    Herringbone Tape
    Other


------------------------------------------------------------
3C. OTHER TRIM MATERIAL SUBTYPES
------------------------------------------------------------

Subtype should be optional.

Do not require a subtype for every trimMaterialType.

The implementation should allow additional subtype enumerations to be added
later without changing the overall data model.

If trimMaterialType == other OR trimMaterialSubtype == other:

Field:
    trimMaterialOther

Control:
    Text field

Display label:
    Description


------------------------------------------------------------
3D. OPTIONAL HUMAN-READABLE NAME
------------------------------------------------------------

Field:
    materialName

Control:
    Text field

Display label:
    Material Name

Optional.

Examples:
    25 mm knitted waistband elastic
    10 mm cotton twill tape
    6 mm polyester piping cord

This field does NOT replace the controlled classification.

Example:

    pieceType = trim_material
    trimMaterialType = elastic
    trimMaterialSubtype = knitted_elastic
    materialName = "25 mm waistband elastic"

============================================================
4. TRIM COMPONENT / HARDWARE
============================================================

Displayed when:
    pieceType == trim_component

Field:
    trimComponentType

Control:
    Dropdown / combo box

Values:
    zipper
    button
    snap
    hook_eye
    buckle
    grommet_eyelet
    d_ring
    slider_adjuster
    cord_lock
    other

Display labels:
    Zipper
    Button
    Snap
    Hook & Eye
    Buckle
    Grommet / Eyelet
    D-Ring / Ring
    Slider / Adjuster
    Cord Lock
    Other

If trimComponentType == other:

Field:
    trimComponentOther

Control:
    Text field

Display label:
    Description


Optional field:

    componentName

Control:
    Text field

Display label:
    Component Name

Example:
    #5 separating molded-tooth zipper

============================================================
5. OTHER PIECE TYPE
============================================================

Displayed when:
    pieceType == other

Field:
    pieceTypeOther

Control:
    Text field

Display label:
    Description

============================================================
6. OPTIONAL MATERIAL PROPERTIES
============================================================

These are independent properties and therefore should use checkboxes rather
than radio buttons.

Field group:
    materialProperties

Possible boolean values:

    cutToLength
    cutToShape
    stretchElastic
    fusibleAdhesive
    decorative
    structural
    reflective

Display labels:

    Cut to Length
    Cut to Shape
    Stretch / Elastic
    Fusible / Adhesive-Backed
    Decorative
    Structural
    Reflective

Multiple properties may be true simultaneously.

IMPORTANT:
These properties describe characteristics of the material. They should NOT
be used to encode manufacturing operations.

For example:

    stretchElastic = true

means that the material has stretch/elastic characteristics.

It does NOT mean:

    "stretch the elastic during sewing"

The latter is a manufacturing operation and belongs in a separate operation
model.

============================================================
7. UI BEHAVIOR
============================================================

Use progressive disclosure.

Initially show:

    Piece Type

After selecting Piece Type, show only the fields applicable to that type.

Example 1:

    Piece Type
        (x) Textile
        ( ) Trim Material
        ( ) Trim Component / Hardware
        ( ) Other

    Textile Type
        [ Shell / Main Fabric              v ]


Example 2:

    Piece Type
        ( ) Textile
        (x) Trim Material
        ( ) Trim Component / Hardware
        ( ) Other

    Trim Type
        [ Elastic                          v ]

    Subtype
        [ Knitted Elastic                  v ]

    Material Name
        [ 25 mm waistband elastic            ]


Example 3:

    Piece Type
        ( ) Textile
        ( ) Trim Material
        (x) Trim Component / Hardware
        ( ) Other

    Component Type
        [ Zipper                           v ]

    Component Name
        [ #5 separating molded-tooth zipper  ]

============================================================
8. CONTEXT MENU DESIGN
============================================================

Do NOT place the entire form directly into the Piece Mode right-click menu.

The context menu should provide a compact entry such as:

    Piece Type / Material...

or:

    Classification...

Selecting it should open the appropriate Piece Properties section/dialog.

If Seamly2D already has an appropriate Piece Properties dialog, integrate
these controls there rather than creating a separate dialog.

The context menu may optionally display the current classification:

    Classification >
        Textile: Shell / Main Fabric

or:

    Classification >
        Trim Material: Elastic

The full editing UI should remain in the properties interface.

============================================================
9. DATA MODEL
============================================================

Store stable machine-readable identifiers, NOT translated display strings.

Example:

    pieceType = "trim_material"
    trimMaterialType = "elastic"
    trimMaterialSubtype = "knitted_elastic"
    materialName = "25 mm waistband elastic"

Display strings such as:

    "Trim Material"
    "Knitted Elastic"

must be translatable UI strings.

Do not store translated UI strings as enumeration values.

============================================================
10. BACKWARD COMPATIBILITY
============================================================

Existing Seamly2D pattern files will not contain these fields.

When classification metadata is absent, treat the piece as:

    pieceType = textile
    textileType = shell

Do not require migration of existing pattern files merely to open them.

Only serialize the new metadata according to the existing Seamly2D XML
versioning/schema conventions.

Opening an older file must not generate an error because these fields are
missing.

============================================================
11. EXTENSIBILITY
============================================================

Design the implementation so new classifications can be added later.

In particular, do not hard-code UI logic in a way that makes adding:

    textile types
    trim material types
    trim material subtypes
    component types

difficult.

The classification hierarchy should conceptually be:

Piece
 |
 +-- Textile
 |    +-- Shell
 |    +-- Lining
 |    +-- Interfacing
 |    +-- Interlining
 |    +-- Padding / Insulation
 |    +-- Reinforcement
 |
 +-- Trim Material
 |    +-- Elastic
 |    |    +-- Braided
 |    |    +-- Knitted
 |    |    +-- Woven
 |    |    +-- ...
 |    |
 |    +-- Tape
 |    |    +-- Twill
 |    |    +-- Hem
 |    |    +-- Stay
 |    |    +-- ...
 |    |
 |    +-- Binding
 |    +-- Webbing
 |    +-- Cord / Drawcord
 |    +-- Piping / Welting
 |    +-- Boning / Stay
 |    +-- Ribbon / Braid
 |    +-- Lace / Decorative Trim
 |    +-- Reflective Trim
 |    +-- Seam-Sealing / Bonding Tape
 |
 +-- Trim Component / Hardware
 |    +-- Zipper
 |    +-- Button
 |    +-- Snap
 |    +-- Hook & Eye
 |    +-- Buckle
 |    +-- Grommet / Eyelet
 |    +-- D-Ring / Ring
 |    +-- Slider / Adjuster
 |    +-- Cord Lock
 |
 +-- Other

============================================================
12. IMPORTANT ARCHITECTURAL PRINCIPLE
============================================================

Keep these concepts separate:

A. OBJECT CLASSIFICATION
   "What is this piece?"

B. MATERIAL PROPERTIES
   "What characteristics does it have?"

C. MANUFACTURING OPERATIONS
   "What should manufacturing do with it?"

Example:

OBJECT:
    Trim Material > Elastic > Knitted Elastic

PROPERTIES:
    stretchElastic = true
    cutToLength = true

OPERATIONS:
    Cut to specified length
    Position at waistband
    Stretch to specified ratio
    Stitch to shell

Only A and, optionally, B are part of this feature.

Do not implement manufacturing operations as part of this task.