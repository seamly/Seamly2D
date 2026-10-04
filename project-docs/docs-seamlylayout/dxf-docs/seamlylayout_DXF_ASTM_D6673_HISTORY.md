# D6673-10: "Category" versus `Material:`

In **ASTM D6673-10, “Category” is a bit of a historical/terminology oddity**. It appears in the standard’s summary of the kinds of *piece information* that can be carried—“piece name, quantity, category, rotation, flip, tilt, fold”—but the actual **Piece System Text** definition in D6673-10 does **not define a `Category:` identifier**. Instead, the defined field is:

> `Material:<string>`

and ASTM explains that **Material is the name of the material in which the piece is cut**. [Scribd](https://www.scribd.com/document/772301782/DXF-ASTM-SpecD6673-33508)

So, for D6673-10, I would **not interpret “Category” as a formally specified apparel classification with enumerated values** such as “front,” “back,” “sleeve,” “lining,” etc. The normative piece metadata listed by the standard is:

- `Piece Name:<string>`
- `Quantity:<R,L>`
- `Rotation:<0..360>`
- `Flip:<X|Y>`
- `Tilt:<+/-0..90>`
- `Fold:<Y|N>`
- `Material:<string>`

Only **Piece Name is required**. [Scribd](https://www.scribd.com/document/772301782/DXF-ASTM-SpecD6673-33508)

There is therefore an apparent inconsistency inside D6673-10: the introductory text says that piece information includes **“category”**, while the actual system-text syntax specifies **Material** instead. [Scribd](https://www.scribd.com/document/772301782/DXF-ASTM-SpecD6673-33508)

### What I think happened

“Category” looks like a **legacy term left in the descriptive text**, probably inherited from an earlier version/proposal of the DXF-ASTM format, while the actual D6673-10 implementation settled on `Material`. It is not defined in the D6673-10 terminology section as a separate concept.

And `Material` itself is extremely loose: ASTM says only that it is **“a string giving the name of the material in which the piece is cut.”** [Scribd](https://www.scribd.com/document/772301782/DXF-ASTM-SpecD6673-33508)

That could therefore contain vendor-specific values such as:

`SHELL`  
`LINING`  
`INTERFACING`  
`FABRIC`  
`DENIM`  
`POCKETING`

…but the standard provides **no vocabulary or semantic rules for those strings**.

### This matters for your D6673 revision

I think this is exactly one of the semantic ambiguities your task group should clean up.

The existing standard effectively has:

**Piece → Material → arbitrary string**

whereas what you have been developing for Seamly is closer to:

**Piece → Textile Type → Fabric / Lining / Interfacing / Trim / etc.**

possibly with a separate **material identifier** describing the actual material.

I would therefore distinguish the concepts in a revised standard:

**Textile Category / Textile Type** = the functional role of the material in the product  
e.g. `fabric`, `lining`, `interfacing`, `trim`

**Material** = the actual material/specification assigned to the piece  
e.g. `14 oz denim`, `polyester lining`, `fusible tricot`

That would eliminate the ambiguity that D6673-10 currently leaves behind.

One additional wrinkle I noticed: D6673-10 also uses the word **“category” for drill holes**, where an integer value associated with a drill specifies the *drill category/type*. That usage is unrelated to the piece-level “category” mentioned earlier. [Scribd](https://www.scribd.com/document/772301782/DXF-ASTM-SpecD6673-33508)
