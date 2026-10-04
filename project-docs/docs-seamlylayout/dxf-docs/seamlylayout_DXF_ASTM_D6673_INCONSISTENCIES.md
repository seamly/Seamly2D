# D6673 inconsistencies: piece "Category" across editions

I traced it through the three published editions, and the history is clearer than I expected: **“Category” was present from the original 2001 edition, but it was never actually defined as a Piece System Text field.** Then, in 2010, ASTM added a new `Material:` field without removing “category” from the Summary of Practice.

Here is the progression.

| Edition | Summary of Practice says piece information includes | Actual Piece System Text fields | What happened to “Category”? |
|---|---|---|---|
| **D6673-01** | piece name, quantity, **category**, rotation, flip, tilt, fold | Piece Name, Quantity, Rotation, Flip, Tilt, Fold | Mentioned in summary, **but no `Category:` field exists** |
| **D6673-04** | piece name, quantity, **category**, rotation, flip, tilt, fold | Piece Name, Quantity, Rotation, Flip, Tilt, Fold | Same inconsistency remains |
| **D6673-10** | piece name, quantity, **category**, rotation, flip, tilt, fold | Piece Name, Quantity, Rotation, Flip, Tilt, Fold, **Material** | `Category` still not implemented; `Material:` added instead |

### D6673-01 — the inconsistency begins

The original 2001 standard explicitly says in §4.1 that it can incorporate:

> “piece information: piece name, quantity, category, rotation, flip, tilt, fold.”

So **Category was already in the original high-level description**. [iTeh Standards](https://www.standards.iteh.ai/catalog/standards/astm/25fa73b6-a4cd-464a-a9c0-78432ec64cf3/astm-d6673-01?utm_source=chatgpt.com)

But when the standard gets to §4.3.1.2, **Piece System Text**, the fields are:

`Piece Name`  
`Quantity`  
`Rotation`  
`Flip`  
`Tilt`  
`Fold`

There is **no `Category:` system-text identifier**. [iTeh Standards](https://www.standards.iteh.ai/catalog/standards/astm/25fa73b6-a4cd-464a-a9c0-78432ec64cf3/astm-d6673-01?utm_source=chatgpt.com)

That means the contradiction was not introduced later. It appears to have existed in the **first published edition**.

### D6673-04 — still no Category field

The 2004 revision retained exactly the same phrase in §4.1:

> “piece information: piece name, quantity, category, rotation, flip, tilt, fold.” [Adobe](https://community.adobe.com/havfw69955/attachments/havfw69955/illustrator/387224/1/DXF-Ansi-AAMA.pdf?utm_source=chatgpt.com)

But §4.3.1.2 again defines only:

`Piece Name:<string>`  
`Quantity:<R,L>`  
`Rotation:<0..360>`  
`Flip:<X|Y>`  
`Tilt:<+/-0..90>`  
`Fold:<Y|N>`

There is still **no Category identifier, no Category syntax, and no definition of allowed Category values.** [Adobe](https://community.adobe.com/havfw69955/attachments/havfw69955/illustrator/387224/1/DXF-Ansi-AAMA.pdf?utm_source=chatgpt.com)

This is important because it rules out the theory that `Category` was an old implemented field that got removed between 2001 and 2004. At least in the published standards, **it appears never to have been implemented at all.**

### D6673-10 — Material suddenly appears

The 2010 revision still carries the old §4.1 wording unchanged:

> “piece information: piece name, quantity, category, rotation, flip, tilt, fold.” [Scribd](https://www.scribd.com/document/772301782/DXF-ASTM-SpecD6673-33508)

But §4.3.1.2 now adds a seventh Piece System Text field:

`Material:<string>`

along with the previous six. ASTM then explicitly explains:

> “Material is a string giving the name of the material in which the piece is cut.” [Scribd](https://www.scribd.com/document/772301782/DXF-ASTM-SpecD6673-33508)

So this appears to be a **new 2010 addition**. It is not present in the D6673-04 Piece System Text table. Compare the 2004 table here [Adobe](https://community.adobe.com/havfw69955/attachments/havfw69955/illustrator/387224/1/DXF-Ansi-AAMA.pdf?utm_source=chatgpt.com) with the 2010 table here. [Scribd](https://www.scribd.com/document/772301782/DXF-ASTM-SpecD6673-33508)

### That gives us a likely editorial history

I would reconstruct the history this way:

**2001**

Conceptual requirement:

`Category`

Actual encoded metadata:

`Piece Name / Quantity / Rotation / Flip / Tilt / Fold`

So Category was probably **intended functionality that never made it into the actual syntax**.

**2004**

No correction was made. The summary continued to promise `Category`, while the syntax still did not support it.

**2010**

Someone evidently recognized the need for material-related piece metadata and added:

`Material:<string>`

But the editors **did not update §4.1**, so `Category` remained there as legacy text.

The result was:

**Summary:** Category  
**Actual file syntax:** Material

This discrepancy persisted until D6673-10 was withdrawn in 2019. ASTM confirms the edition history as D6673-01 → D6673-04 → D6673-10. [ASTM International](https://store.astm.org/d6673-04.html?utm_source=chatgpt.com)

## The significant finding for your D6673 issues register

I would not write that **Category was renamed to Material**, because the evidence does **not** establish that.

A more defensible finding is:

> **The term “category” has appeared in the Summary of Practice since D6673-01 as part of the list of supported piece information, but no `Category:` system-text identifier or definition is provided in D6673-01, D6673-04, or D6673-10. D6673-10 separately introduced the Piece System Text identifier `Material:<string>`, defined as the name of the material in which the piece is cut, while retaining the older reference to “category” in §4.1. Consequently, the relationship, if any, between “category” and “Material” is undefined.**

That is a fairly strong interoperability defect because an implementer reading §4.1 could reasonably conclude that **Category must be exchanged**, but the implementation section gives them **no way to encode it**.

### I would classify this as two separate defects

**1. Undefined advertised field — `Category`**

The standard claims Category is supported piece information but specifies:

- no identifier,
- no data type,
- no syntax,
- no semantics,
- no controlled vocabulary,
- no example.

**2. Ambiguous material semantics — `Material`**

D6673-10 defines `Material` only as a free-text “name of the material.” It does not distinguish:

`material role` — shell, lining, interfacing, trim, etc.

from

`material identity/specification` — 14 oz denim, fusible tricot, polyester lining, etc.

That distinction is probably worth making explicit in the replacement standard.

There is also a **second, unrelated use of “category”** in D6673-10 for Layer 13 drill holes—the integer associated with a drill is described as specifying its drill category/type. [Scribd](https://www.scribd.com/document/772301782/DXF-ASTM-SpecD6673-33508) That makes leaving the piece-level term “category” undefined even more problematic, because the word has another technical meaning elsewhere in the same document.

For the D13.66 work, I think the strongest next step is to look at **ANSI/AAMA-292A and pre-ASTM DXF/AAMA documentation**. D6673 did not originate this interchange format from scratch, and there's a good chance the mysterious piece `Category` came from the earlier AAMA specification. That may tell us what the original authors intended “Category” to mean before it was lost in the ASTM implementation.
