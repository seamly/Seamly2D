# D6673-10 `Material:` piece text

In **ASTM D6673-10, there are no standardized “types of material.”** `Material` is defined simply as a free-form string:

`Material:<string>`

The standard describes it as the **name of the material in which the piece is cut**. It does **not** define an enumeration such as `fabric`, `lining`, `interfacing`, `trim`, etc. [Normsplash](https://www.normsplash.com/Samples/ASTM/191361149/ASTM-D6673-10-en.pdf?utm_source=chatgpt.com)

So all of these could technically be valid values:

- `SHELL`
- `LINING`
- `INTERFACING`
- `DENIM`
- `14 OZ DENIM`
- `POCKETING`
- `FUSIBLE`
- `FABRIC 1`
- `A`
- `MAIN`

…but those values would be **application/vendor conventions**, not D6673-10-defined material types.

The significant interoperability problem is that D6673-10 doesn't distinguish between:

**material role/type**  
such as `fabric`, `lining`, `interfacing`, `trim`

and

**material identity/specification**  
such as `14 oz cotton denim`, `polyester lining`, `fusible tricot`.

The standard provides only this single free-text field:

```text
Material:<string>
```

And only `Piece Name` is mandatory; `Material` is optional. [Antpedia](https://img.antpedia.com/standard/files/pdfs_ora/20221211/astm/ASTM%20D6673-10.pdf?utm_source=chatgpt.com)

So for your D6673 modernization work, I would record this as a semantic gap: **D6673-10 supports material assignment but provides no material taxonomy, controlled vocabulary, identifier scheme, or distinction between material function and material identity.**

This also reinforces our previous finding that **`Category` should not be assumed to mean material type**. `Category` and `Material` appear to be separate historical concepts.

## SeamlyLayout output

- Every piece: `Material:Fabric`. Source: `seamly_svg2ezdxf::DEFAULT_MATERIAL`, stored in `Block::material`.
- `Category:` is not written. It is ANSI/AAMA-292 piece text, not valid in any DXF-ASTM edition.
- Piece text order: `Piece Name:`, `Size Name:`, `Quantity:`, `Material:`.
- Planned: Seamly2D.7.5 adds `data-material` to the handoff; Layout.15.4 writes it as `Material:`.
