# TODO — Create the combined MSI installer for Seamly2D, SeamlyMe, and SeamlyLayout

Tasks for creating an .msi file for installation on a user's amd64 computer with Windows 10 or Windows 11.

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file start with `CLI.`

## Task CLI.1 - Audit & fix the CLI parameters so that each option works

**Dependency:** seamlyLayout export modes is required for the mode pass-through; seamlyLayout also needs a headless/CLI export mode of its own, since today it is only driven interactively through its QML UI.

Extend the existing console export mode (`--basename` in `src/app/seamly2d/core/vcmdexport.cpp`) so a single seamly2d command line produces the final layout: seamly2d generates the tagged `.pieces.svg` (the Layout Mode handoff, `exportPiecesToSeamlyLayout()` in `src/app/seamly2d/mainwindow.cpp`) and then runs seamlyLayout on it to produce the layout output, using the new seamlyLayout export options.

Go through every command-line option seamly2d advertises (defined in `src/libs/vmisc/commandoptions.cpp`, wired in `src/app/seamly2d/core/vcmdexport.cpp`) and make each one actually work in console export mode. Known friction from Task 11 verification: option names are case-sensitive and inconsistently cased (e.g. `--exportOnlyDetails`), and errors only surface in a redirected stderr, not on the console.

These tasks are not numbered in the order they should be implemented -- analyze and update these tasks tersely to form a step-by-step implementation plan.

- [ ] CLI.1.1 Add a seamly2d CLI option (export mode) that triggers the Layout Mode handoff from the console: generate `<basename>.pieces.svg` and invoke seamlyLayout on it, resolving the app path the same way as the GUI (`paths/seamlyLayoutApp` setting)
- [ ] CLI.1.2 Add a headless CLI export mode to seamlyLayout: input `.pieces.svg`, run the layout/nesting, export to a chosen format and output path without showing the QML UI, exit with a meaningful status code
- [ ] CLI.1.3 Pass the seamlyLayout export options through the seamly2d command line (export format incl. the Task 21 SVG text modes, output destination), and document the option mapping
- [ ] CLI.1.4 Make the seamly2d invocation wait for seamlyLayout (unlike the GUI's `QProcess::startDetached`), propagate its exit status and stderr so scripted callers see failures
- [ ] CLI.1.5 Tests: seamly2d CLI option parsing (extend `tst_vcommandline`), seamlyLayout headless-export tests (Rust/Qt side), and an end-to-end check with the richmond test pattern
- [ ] CLI.1.6 Document the workflow (command-line examples) in the repo docs / `--help` output
- [ ] CLI.1.7 Inventory all options and build a test matrix: expected behavior, required companions (e.g. `--basename` enabling export mode), valid values
- [ ] CLI.1.8 Exercise each option against the richmond test pattern (all export formats, gradation size/height, page options, `--text2paths`, measurement overrides, etc.) and record which are broken, ignored, or misdocumented
- [ ] CLI.1.9 Fix the broken/ignored options; make error messages reach the console reliably (the GUI-subsystem exe detaches from the console — evaluate `AttachConsole`/subsystem handling on Windows so `--help` and errors print without redirection)
- [ ] CLI.1.10 Consider case-insensitive or consistently lowercase option aliases (keeping the existing names working for compatibility)
- [ ] CLI.1.11 Unit tests: extend `tst_vcommandline` to cover every option and the failure modes found
- [ ] CLI.1.12 Update `--help` text and repo docs with the verified behavior

## Task CLI.2 - run seamly2d/seamlyme/seamlylayout from the command line with an input .sm2d file and an input measurement file to create exports for each of seamlyLayout's export options; validate outputs to validate the CLI parameters