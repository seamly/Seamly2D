# SeamlyLayout Workspace Instructions for Copilot

Author: slspencer
Copyright: 2026

> Synchronization note: `AGENTS.md` mirrors this file. When updating guidance, update both files in the same change.

## Priority and Scope

- Prompts are requirements, not suggestions.
- Apply these rules across this entire workspace unless a task explicitly states otherwise.

## Architecture Context

- Frontend: Qt 6.10 + QML/QtWidgets (`qt_frontend/`) under LGPL-3.0.
- Core logic: Rust crates (`crates/`) under MIT.
- Bridge: CXX-Qt (`crates/cxxqt_bridge/`) connecting Rust and Qt.
- For UI tasks, always inspect both `qt_frontend/qml/` and `crates/cxxqt_bridge/`.

## Mandatory Engineering Rules

- Always use absolute file paths (C++: `QFileInfo::absoluteFilePath()`, Rust: `std::path::Path::canonicalize()`).
- Do not introduce new regex for parsing/manipulating SVG or XML.
- SVG processing must use DOM/parser-based workflows (`xmltree` / `svg_dom`) and update the XML tree.
- Use terminology correctly:
  - **flatten** = baking-in transforms
  - **interpolation** = converting curves to polylines

## File Update Coverage

- When making code changes, search relevant files with extensions:
  - `.rs`, `.cpp`, `.cxx.cpp`, `.h`, `.qml`
- When making text/documentation updates, search relevant files with extensions:
  - `.md`, `.txt`
- Keep docs in `docs/` synchronized with behavior changes.

## Language and Licensing Expectations

- Prefer moving business logic into Rust where practical.
- License expectations:
  - `.rs` files: MIT
  - `.qml`, `.cpp`, `.h` files: LGPL

## Code Quality Expectations

- Add `@doxygen` briefs to functions/methods/wrappers/controllers.
- Add inline comments that clarify workflow, control flow, and data flow.

## Shell Commands Policy

The following Bash and PowerShell commands are permitted and may be run without restriction:

**Bash (Git Bash / POSIX):**
- File discovery: `ls *`, `ls`, `find *`
- Navigation: `cd *`, `pwd`, `chdir *`
- File ops: `cp *`, `mv *`, `rm *`, `cat *`, `mkdir *`
- Inspection: `grep *`, `which *`, `where *`, `type *`, `env`, `date`, `set`, `whoami`, `hostname`
- Scripting: `echo *`, `PATH=*`
- Version control: `git *`, `gh *`

**PowerShell:**
- File discovery: `ls *`, `find *`, `dir *`
- Navigation: `cd *`, `pwd`
- File ops: `cp *`, `copy *`, `mv *`, `rm *`, `cat *`, `mkdir *`
- Inspection: `where *`, `date`, `whoami`, `hostname`
- Scripting: `echo *`, `$env:*`, `$*`
- Version control: `git *`, `gh *`

## Rule References

- Root guidance: `CLAUDE.md`
- Expanded rules: `.claude/rules/CLAUDE.md`
- Detailed standards in `.claude/rules/`:
  - `branding.mdc`, `dependencies.mdc`, `licensing.mdc`
  - `rust-style.mdc`, `qt-style.mdc`, `ffi-bridge.mdc`
  - `svg-processing.mdc`, `testing.mdc`
  - `Guidelines_Export_DXF.mdc`, `Guidelines_Layout.mdc`, `Guidelines_Settings.mdc`, `Guidelines_Tiling.mdc`