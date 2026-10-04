# D6673 "Category" inherited from ANSI/AAMA-292

I found enough evidence to materially change the interpretation of **“Category.”** It appears that **Category was inherited from the older AAMA/Gerber pattern-data model and was not originally synonymous with Material.**

The strongest evidence points to **Category being a general piece-classification/identifier field**, while **Material/Fabric was a separate concept**.

ANSI/AAMA-292 was published in 1993 as the apparel industry's DXF-based pattern interchange standard. A 1997 NIST report describes it as an AAMA standard built on AutoCAD Version 11 DXF, with fourteen defined layers for apparel-pattern information. NIST was actually building translators between its STEP Pattern Information Model and ANSI/AAMA-292 at the time. [NIST Publications](https://nvlpubs.nist.gov/nistpubs/Legacy/IR/nistir5969.pdf) The University of Illinois Grainger Engineering Library catalog confirms that it holds **ANSI/AAMA-292, Standard for Pattern Data Interchange, 1993**. [ENX Standards](https://enxstandards.web.illinois.edu/standard/aama-292/?utm_source=chatgpt.com)

What I have **not** yet found publicly accessible is a scan of the complete original ANSI/AAMA-292/292A normative text. That matters because I don't want to claim wording from the original standard that I have not actually seen.

However, the surviving implementation documentation is quite revealing.

### 1. Gerber AccuMark has a real, independent `Category` property

Current Gerber/Lectra AccuMark documentation says:

> “You may enter a category for each piece or allow the piece name to be assigned as the category automatically.”

It even provides an option called **“Same Category as Piece Name.”** [Gerber Help](https://gerber-help.lectra.com/AccuMark/AMX/Function_Dig_Process_Opt.htm?utm_source=chatgpt.com)

That is very important.

If Category meant **fabric/material**, automatically setting:

`Category = Piece Name`

would make little sense.

For example:

`Piece Name = FRONT`  
`Category = FRONT`

makes sense if Category is a **piece classification/indexing field**.

It would make no sense if Category meant:

`Material = FRONT`.

So this is strong evidence that **Category was not originally intended as textile type or material**.

### 2. Older AccuMark behavior treats Category almost like a secondary piece identifier

I found an AccuMark release note describing an AAMA/ASTM import bug. When an incoming DXF had a blank category, AccuMark assigned the original piece name as the category. The bug concerned whether changes to the piece name during import also altered the resulting piece category. [Scribd](https://www.scribd.com/document/613006720/relnot-PE?utm_source=chatgpt.com)

Again, this tells us something about its semantics:

**Category is associated with identification/classification of the pattern piece itself.**

It is not behaving like a material assignment.

### 3. Gerber's own pattern-making UI distinguishes Category from Description

An AccuMark training document shows the Piece Save dialog with separate fields for:

`File name`  
`Category`  
`Description`

and examples in which Category is used as a type/classification for the piece. [www.slideshare.net](https://www.slideshare.net/slideshow/gerber-accumark-pds-tra-cu-lnh-gerber-accumark/229559419?utm_source=chatgpt.com)

Another portion of the same material shows operations such as splitting a piece with choices to:

- copy the original Category, or
- use the Piece Name as the new Category. [www.slideshare.net](https://www.slideshare.net/TILIUMAYVMINPH/gerber-accumark-pds-tra-cu-lnh-gerber-accumark?utm_source=chatgpt.com)

That's particularly persuasive. If a pattern piece is split into two pieces, the software asks how to assign the **piece category**. This is clearly piece metadata, not fabric metadata.

### 4. Surviving AAMA practice shows `Category` and `Material` as distinct fields

A practitioner who worked extensively with DXF-AAMA/ASTM describes AAMA-style piece metadata as including separate entries for:

`Piece Name`  
`Size`  
`Annotation`  
`Category`  
`Quantity`  
`Material`

and describes Category as a short term used to categorize the piece, while Material is a separate short fabric/material code. [McNeel Forum](https://discourse.mcneel.com/t/how-to-export-dxf-aama-format/52629?utm_source=chatgpt.com)

I would treat that source as **implementation evidence rather than normative evidence**, but it fits the Gerber evidence extremely well.

So we now have multiple independent clues supporting the same distinction:

**Category ≠ Material.**

---

## 5. This explains the strange wording in ASTM D6673

Now look at the lineage.

ANSI/AAMA-292 existed by 1993. NIST confirms that it was already a vendor-oriented apparel pattern interchange mechanism. [NIST Publications](https://nvlpubs.nist.gov/nistpubs/Legacy/IR/nistir5969.pdf)

ASTM D6673-01 explicitly cites **ANSI/AAMA-292A** as a referenced document. [iTeh Standards](https://www.standards.iteh.ai/catalog/standards/astm/25fa73b6-a4cd-464a-a9c0-78432ec64cf3/astm-d6673-01?utm_source=chatgpt.com)

And ASTM's Summary of Practice says that the transfer can contain:

> “piece name, quantity, category, rotation, flip, tilt, fold.” [Adobe](https://community.adobe.com/havfw69955/attachments/havfw69955/illustrator/387224/1/DXF-Ansi-AAMA.pdf?utm_source=chatgpt.com)

So I now think the most likely history is:

**AAMA model**

`Piece Name`  
`Category`  
`Quantity`  
other piece properties

↓

**ASTM D6673 development**

ASTM carried the **conceptual list** from the AAMA-era model into §4.1.

↓

But ASTM then created a different, more tightly specified **Piece System Text** mechanism.

And somewhere in that transition, **Category never received a defined ASTM system-text identifier.**

That's why D6673 can say:

> piece information includes “category”

while failing to define:

`Category:<string>`

anywhere in its encoding rules.

This looks much less like a typo now and more like a **migration defect from AAMA to ASTM**.

---

## 6. Material appears to be a different concept added later

By D6673-10, ASTM defines:

`Material:<string>`

and describes it as the material in which the piece is cut. The same edition nevertheless leaves the old `category` word in the Summary. [Scribd](https://www.scribd.com/document/772301782/DXF-ASTM-SpecD6673-33508?utm_source=chatgpt.com)

That strongly argues **against** my earlier possibility that Category simply became Material.

They appear to have been **two distinct concepts**:

| Concept | Likely meaning |
|---|---|
| **Piece Name** | Unique/name identifier for the pattern piece |
| **Category** | Piece classification/grouping used by CAD systems |
| **Material** | Material/fabric in which the piece is cut |

And some legacy systems apparently also had:

| Concept | Likely meaning |
|---|---|
| **Annotation/Description** | Human-readable description of the piece |
| **Fabric/Material code** | Short code associating the piece with a material group |

That model is consistent with the Gerber evidence.

---

# So what did `Category` actually mean?

I would currently define the historical concept approximately as:

> **Category — a user- or CAD-system-defined classification associated with a pattern piece, used to group or identify pieces independently of the piece name.**

But there is an important caveat:

### There does not appear to have been a standardized vocabulary.

Nothing I've found establishes values such as:

`FRONT`  
`BACK`  
`SLEEVE`

as normative AAMA categories.

Nor have I found evidence that it necessarily meant:

`SHELL`  
`LINING`  
`INTERFACING`

Those might have been *used* as categories by individual systems, but the field seems fundamentally to have been **free-form/vendor-defined classification metadata**.

That explains why one practitioner says it could categorize by material **or** piece type—the field had weak semantics. [McNeel Forum](https://discourse.mcneel.com/t/how-to-export-dxf-aama-format/52629?utm_source=chatgpt.com)

---

# The NIST work is interesting here too

The 1997 NIST Pattern Information Model is actually much more semantically disciplined than AAMA.

It defines entities such as:

- `PATTERN_PIECE`
- `ANNOTATION_FEATURE`
- `MARK_FEATURE`
- `ORIENTATION_CONSTRAINT`
- `COMPOSITE_CURVE_FEATURE`

and enumerated feature types for things like cut boundaries, internal cuts, fold lines, sew lines, drills, grain lines, etc. [NIST Publications](https://nvlpubs.nist.gov/nistpubs/Legacy/IR/nistir5969.pdf)

Notice what it **doesn't** appear to need:

a generic `"Category"` attribute.

Instead, the *semantics themselves* are modeled.

That is exactly the direction you are taking with AIM: rather than throwing meaning into an arbitrary string, explicitly model what the thing **is** and what role it performs.

---

# I would revise the D6673 Issues Register entry

Based on this additional evidence, I'd make the issue stronger and more historically precise:

> **Undefined legacy Piece Category**
>
> D6673 lists `category` among the supported pattern-piece information in §4.1 but provides no Category system-text identifier, syntax, definition, value domain, or interchange requirements.
>
> Evidence from ANSI/AAMA-292-era implementations indicates that **Category was a legacy piece-classification property distinct from Piece Name and Material**. Gerber AccuMark, for example, maintains a Category attribute independently from Piece Name and can default Category to Piece Name. Legacy AAMA implementations also distinguish Category from Material/Fabric.
>
> D6673 appears to have inherited the Category concept from ANSI/AAMA-292/292A in its descriptive text without defining its encoding in the ASTM Piece System Text specification. D6673-10 subsequently introduced `Material:<string>` while retaining the legacy reference to Category, leaving the relationship between these concepts undefined.
>
> **Interoperability impact:** CAD systems may preserve, discard, reinterpret, or synthesize piece Category values because the standard advertises the information but provides no interoperable representation or semantics.

I'd rate that **high severity for semantic interoperability**, even if it has relatively low impact on pure geometry exchange.

---

## And this gives us an important warning for the revised standard

I would **not reuse `Category` for textile type**.

That's tempting because words such as *fabric category* or *textile category* sound natural, but we've now found historical baggage attached to the word.

For the revised format, I'd keep the concepts explicitly separate:

**Piece classification**
→ perhaps `piece_type`, `piece_role`, or a proper ontology relationship

**Textile role**
→ `textile_type` / `material_role`

**Material identity**
→ `material_id` or `material_specification`

For example:

```text
Piece Name:FRONT
Piece Role:front_bodice
Textile Type:fabric
Material ID:SHELL-01
```

That is much less ambiguous than resurrecting:

```text
Category:SHELL
```

---

There is one especially useful loose end: **the University of Illinois Grainger Engineering Library says it physically holds the original 1993 ANSI/AAMA-292**. [ENX Standards](https://enxstandards.web.illinois.edu/standard/aama-292/?utm_source=chatgpt.com) If we can obtain that actual document—or an authorized scan—we could determine conclusively whether the normative AAMA specification itself defined `Category`, what syntax it used, and whether **292A** changed it. That would let us document the provenance of the defect rather than merely infer it from implementations.
