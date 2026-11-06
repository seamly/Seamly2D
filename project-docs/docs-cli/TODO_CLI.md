# TODO — CLI

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file start with `CLI.`

## Task CLI.1 - Audit & fix the CLI parameters so that each option works

**Dependency:** seamlyLayout export modes is required for the mode pass-through; seamlyLayout also needs a headless/CLI export mode of its own, since today it is only driven interactively through its QML UI.

Seamly2D has no console export mode: Dead.4.1.1 removed `--basename` and the old layout options. Add a new export option in `src/app/seamly2d/core/vcmdexport.cpp` so a single seamly2d command line produces the final layout: seamly2d generates the tagged `.pieces.svg` (the Layout Mode handoff, `exportPiecesToSeamlyLayout()` in `src/app/seamly2d/mainwindow.cpp`) and then runs seamlyLayout on it to produce the layout output, using the new seamlyLayout export options.

Go through every command-line option seamly2d advertises (defined in `src/libs/vmisc/commandoptions.cpp`, wired in `src/app/seamly2d/core/vcmdexport.cpp`) and make each one actually work in console export mode. Known friction from Task 11 verification: option names are case-sensitive and inconsistently cased (e.g. `--exportOnlyDetails`), and errors only surface in a redirected stderr, not on the console.

These tasks are not numbered in the order they should be implemented -- analyze and update these tasks tersely to form a step-by-step implementation plan.

- [x] CLI.1.1 Add `--layout-export`, resolve `paths/seamlyLayoutApp`, and send the generated handoff through stdin without an intermediate file.
- [x] CLI.1.2 Headless SeamlyLayout reads a file or stdin, lays out pieces, exports the selected format, and reports meaningful status without QML UI.
- [x] CLI.1.3 Forward format, SVG text modes, output, and settings; document the option mapping.
- [x] CLI.1.4 Wait for SeamlyLayout and propagate stderr/status. Incomplete layouts fail before writing partial output.
- [ ] CLI.1.5 Tests: seamly2d CLI option parsing (extend `tst_vcommandline`), seamlyLayout headless-export tests (Rust/Qt side), and an end-to-end check with the richmond test pattern
- [x] CLI.1.6 Document workflow examples, options, statuses, and `--help` behavior.
- [ ] CLI.1.7 Inventory all options and build a test matrix: expected behavior, required companions (e.g. `--basename` enabling export mode), valid values
- [ ] CLI.1.8 Exercise each option against the richmond test pattern (all export formats, gradation size/height, page options, `--text2paths`, measurement overrides, etc.) and record which are broken, ignored, or misdocumented
- [ ] CLI.1.9 Fix the broken/ignored options; make error messages reach the console reliably (the GUI-subsystem exe detaches from the console — evaluate `AttachConsole`/subsystem handling on Windows so `--help` and errors print without redirection)
- [ ] CLI.1.10 Consider case-insensitive or consistently lowercase option aliases (keeping the existing names working for compatibility)
- [ ] CLI.1.11 Unit tests: extend `tst_vcommandline` to cover every option and the failure modes found
- [ ] CLI.1.12 Update `--help` text and repo docs with the verified behavior

## Task CLI.2 - run seamly2d/seamlyme/seamlylayout from the command line with an input .sm2d file and an input measurement file to create exports for each of seamlyLayout's export options; validate outputs to validate the CLI parameters

Implemented validation covers ten public formats, paid-format rejection, measurement/gradation overrides, omitted-piece failures, and physical PDF/DXF/HPGL calibration. SeamlyMe validates measurement inputs; it does not read `.sm2d` patterns or export layouts. Exact Richmond pattern-and-measurement verification remains blocked: only the cached handoff SVG is present. The exhaustive older-option audit and user/CAD import checks remain open. Integrated individual and multisize-input export checks, 19 pattern-status checks, and 13 SeamlyMe checks pass with matching Qt plugins.