# Claude Rules for SeamlyLayout

See also: [TODO.md](../../docs/status-docs/TODO.md) | [Simple Workflow](../../docs/general-docs/SIMPLE_WORKFLOW_DESCRIPTION.md)

## General Rules

- **Prompts are requirements, not suggestions.**
- Author: slspencer, Copyright: 2026
- Read the `.md`, `.txt`, and `.rtf` files in the `docs/` folder regularly
- Update docs to reflect code changes
- Use "flatten" only for baking-in transforms. Use "interpolation" for converting curves to polylines.
- **Always use absolute file paths, never relative paths.** Resolve via `QFileInfo::absoluteFilePath()` (C++) or `std::path::Path::canonicalize()` (Rust).
- When making code changes, search and update files with .rs, .cpp, .h, and .qml extensions
- When making text updates, search and update files with .md and .txt extensions
- Ignore markdown linting errors (MD001, MD004, MD013, etc.) — they are VS Code diagnostic noise, not blocking issues
- Move as much functionality of the application into .rs files
- License Seamly2D .qml, .cpp, and .h files with GPLv3
- License SeamlyLayout .qml, .cpp, .h, and .rs files with MIT
- Add license and author headers to all new files
- Add @doxygen briefs to all functions, methods, wrappers, controllers
- Add inline coments to all new code to describe the workflow, control flow, and data flow so that an intermediate-level programmer can understand

## Shell Commands Policy

The following Bash and PowerShell commands are pre-allowed in `.claude/settings.json` and may be run without permission prompts:

**Bash (Git Bash / POSIX):**

- File discovery: `ls *`, `ls`, `find *`
- Navigation: `cd *`, `pwd`, `chdir *`
- File ops: `cp *`, `mv *`, `rm *`, `cat *`, `mkdir *`
- Inspection: `grep *`, `which *`, `where *`, `type *`, `env`, `env *`, `date`, `set`, `set *`, `whoami`, `hostname`
- Scripting: `echo *`, `PATH=*`
- Version control: `git *`, `gh *`

**PowerShell:**

- File discovery: `ls *`, `find *`, `dir *`
- Navigation: `cd *`, `pwd`
- File ops: `cp *`, `copy *`, `mv *`, `rm *`, `cat *`, `mkdir *`
- Inspection: `where *`, `date`, `whoami`, `hostname`
- Scripting: `echo *`, `$env:*`, `$*`
- Version control: `git *`, `gh *`

## Regex Policy

- **Do NOT introduce new regex** — use proper parsing libraries instead
- Existing regex in Rust code is acceptable
- **SVG processing must use xmltree/svg_dom** — never regex for SVG manipulation

## Style Rules

- [Branding](branding.mdc) — Color palette and UI styling
- [Dependencies](dependencies.mdc) — Crate versions, workspace structure, Qt 6.10 modules for SeamlyLayout, Qt 6.5.3 for Seamly2D
- [Licensing](licensing.mdc) — License requirements: SeamlyLayout - Qt, Cpp, & Rust = MIT; Seamly2D - Qt, Cpp = GPL-3.0
- [Rust Style](rust-style.mdc) — Rust coding conventions and file headers
- [Qt Style](qt-style.mdc) — C++/QML coding conventions and file headers
- [FFI Bridge](ffi-bridge.mdc) — SeamlyLayout: extern "C" conventions, memory ownership, error codes, and file headers
