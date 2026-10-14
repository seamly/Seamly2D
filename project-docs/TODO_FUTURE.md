# Future TODO's

========================================================
Add tasks in project-docs/TODO_MIGRATE.md -->

1. Merge data from `src\app\seamlylayout\.claude` into the project's top-level `.claude` data files, then remove the `src\app\seamlylayout\.claude` directory.
2. Create a plan to merge data from `src\app\seamlylayout\.github` into the project's top-level `.github` directories and files; review the plan with the user; on user 'ok' add plan tasks to `project-docs/TODO_MIGRATE.md`.
3. Create a plan to move all build scripts from various directories (dist, packaging, etc.) to the .github directory; review plan with user; on user 'ok' add plan tasks to `project-docs/TODO_MIGRATE.md`
4. Create a plan to update `src/test`. Review it with the user and, after approval, save it as tasks in `project-docs/TODO_MIGRATE.md`. Test only current Seamly application versions, covering:

* Seamly2D, SeamlyMe, and SeamlyLayout through both GUI and CLI workflows
* End-to-end workflows among all three applications
* Parsing
* Translations
* Migration of `.sm2d`, `.smis`, and `.smms` files from up to six previous schema versions
* Backup and recovery of those file types
* Every CLI parameter, delegating to application-specific CLI tests where appropriate

  Use this structure:

```text
└── test/
    ├── test_backup/
    ├── test_cli/
    ├── test_inputs/
    │   ├── backup/
    │   ├── cli/
    │   ├── parser/
    │   ├── schema/
    │   ├── seamly_all/
    │   ├── seamly2d/
    │   ├── seamlylayout/
    │   ├── seamlyme/
    │   └── translations/
    ├── test_parser/
    ├── test_schema/
    ├── test_seamly_all/
    │   ├── cli/
    │   └── gui/
    ├── test_seamly2d/
    │   ├── cli/
    │   └── gui/
    ├── test_seamlylayout/
    │   ├── cli/
    │   └── gui/
    ├── test_seamlyme/
    │   ├── cli/
    │   └── gui/
    └── test_translations/
```

========================================================

Add tasks in project-docs/TODO_SEAMLY2D.md -->

1. Add or update the `CLI parameters`popup window:

* Add **Tools > CLI Parameters** to open a resizable popup.
* Persist the popup’s size and position.
* Support vertical and horizontal scrolling.
* Provide a search bar for the **Parameter** and **Description** columns.
* Include a checkbox for each parameter.
* Generate a command from the selected parameters when the user clicks  **Create CLI Command** .
* Display the generated command with a copy button.
* Include a **Close** button.

1. Add a `Renumber IDs`tool to the `Piece Tool Group` in `Draft mode`. Process pattern XML with an SVG/XML library, not regex. Use when object creation and deletion leave IDs unnecessarily high and/or numbered out of sequence.

* Prompt the user to select a piece then display the tool context menu.
* Cancel without changes if declined.
* Collect every unique integer ID, including:

  * values in {ID attributes list}
  * content, where the ID is element content rather than an attribute
* Sort IDs in descending order.
* Reassign them sequentially from the total ID count down to 1, preserving each ID’s tag and attribute location.
* Report an error if an expected element cannot be found.
* Reparse and display the updated pattern.

1. Add a `Rename Automatic Point Names` tool to the `Piece Tool Group` in `Draft mode`. Process pattern XML with an SVG/XML library, not regex. Use when automatically generated point names have integer strings that are too high, out of sequence, or require a new piece letter. Automatic point names consist of the piece letter followed by an integer and contain no underscore.

* Prompt the user to select a piece then display the tool context menu
* After the user selects a piece, open a dialog showing its name and letter.
* Let the user choose between two radio options:
  * Use current piece letter
  * Enter new piece letter, which enables the letter input field
* Verify that a new letter is not used by another piece; show an error and close the dialog if it is; confirm before continuing.
* Collect, in pattern order:
  * eligible point names from {point element list} and {name attributes list}
  * all draft blocks following the selected piece
* Sort eligible point names by their numeric suffix in descending order.
* Rename them sequentially from the point count down to 1, using the selected or existing piece letter.
* For each renamed point, replace references in:
  * subsequent elements of the selected piece
  * all following draft blocks
* Match references containing _<OLD_NAME> or <OLD_NAME>_.
* Report an error if an expected point or attribute cannot be found.
* Reparse and display the updated pattern.

## Layout Miscellaneous

- [ ] Layout.51 DXF-AAMA — biggest install base in apparel PLM, reference implementation already in the repo
  - **On hold** — no AAMA spec on hand. Get the spec, or study sample AAMA `.dxf` files from other CAD systems, before implementing.
- [ ] Layout.63 Drill holes (layer 13), with diameter (group 30) - not implemented in Seamly2D yet so move this to TODO_SEAMLY2D.md
- [ ] Layout.64 Stripe and plaid reference lines (layers 9, 10) - not implemented in Seamly2D yet so move this to TODO_SEAMLY2D.md

## [ ] Task Layout.15 — Layout by material

Depends on Seamly2D.7. Until then every piece is on one fabric.

- [ ] Layout.15.1 Import: read `data-material` and per-material quantity
- [ ] Layout.15.2 One layout tab per material; each packs only that material's pieces
- [ ] Layout.15.3 Quantity: place each piece its cut count per material
- [ ] Layout.15.4 Exports: material in the default file name; DXF-ASTM `Material:` piece text from `data-material` (today `Block::material` is `DEFAULT_MATERIAL` = `Fabric`)
- [ ] Layout.15.5 Tests: Rust import, packing per material, export text

