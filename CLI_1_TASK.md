# CLI.1 — Audit, Repair, and Validate Seamly2D CLI Export

## 1. Objective

Make every supported Seamly2D command-line option functional, consistent, documented, and tested.

Enable a single Seamly2D command to generate final layout exports through headless SeamlyLayout.

Build automated CLI regression tests and a standalone interactive Rust/Iced testing application to exercise all supported layout and export combinations.

**Work autonomously until all acceptance criteria are verified. Do not stop after creating a plan.**

## 2. Repository and Git configuration

- Repository: `Seamly2D-private`
- Base branch: `run-seamlyLayout`
- Working branch: `run-seamlyLayout` in the CLI worktree, as requested on 2026-11-16.
- Worktree: `C:\Users\susan\Projects\seamly2d-cli-1`

Seamly2D and SeamlyLayout are contained in this same repository. Do not create or search for a separate SeamlyLayout checkout.

All implementation, tests, and documentation changes must occur within the designated worktree.

Do not modify the original `Seamly2D-private` working directory.

Do not push, merge, publish, or release automatically.

Preserve existing functionality and compatibility.

## 3. Architecture

The export pipeline is:

`Seamly2D CLI → tagged .pieces.svg data → stdin → SeamlyLayout headless → layout output`

### Seamly2D responsibilities

- Load patterns and associated measurement data.
- Apply measurement overrides.
- Resolve multisize selections and gradation settings.
- Generate tagged SVG piece data.
- Invoke SeamlyLayout.
- Forward compatible layout and export parameters.
- Report success, failure, and diagnostic messages.

### SeamlyLayout responsibilities

- Receive SVG from stdin or an input file.
- Parse the tagged SVG.
- Generate layouts according to the supplied settings.
- Apply media and piece constraints.
- Export the requested file format.
- Return meaningful diagnostics and exit codes.

### Requirements

1. No intermediate SVG file between Seamly2D and SeamlyLayout.
2. Resolve SeamlyLayout using `paths/seamlyLayoutApp`.
3. Preserve piece IDs and semantic metadata.
4. Support large multisize SVG data without command-line length limits.
5. Propagate stdout, stderr, and subprocess exit status.
6. Reject invalid and unsupported setting combinations.
7. Prevent incomplete exports from being reported as successful.
8. Preserve existing CLI behavior wherever compatible with the new implementation.
9. Do not require the SeamlyLayout QML interface for headless export.
10. Avoid duplicating layout or export logic already implemented.

## 4. Existing implementation

The following tasks were previously marked complete and require verification before additional changes:

- CLI.1.1 — Add `--layout-export`, resolve SeamlyLayout executable, and transfer SVG through stdin.
- CLI.1.2 — Implement headless SeamlyLayout input, processing, and export.
- CLI.1.3 — Forward output format, SVG text modes, output paths, and layout settings.
- CLI.1.4 — Wait for completion, propagate diagnostics/status, reject incomplete layouts.
- CLI.1.6 — Document workflow examples, options, statuses, and help behavior.

Known source locations:

- `src/libs/vmisc/commandoptions.cpp`
- `src/app/seamly2d/core/command_line.cpp`
- `src/app/seamly2d/mainwindow.cpp`
- `exportPiecesToSeamlyLayout()`
- `tst_vcommandline`

Previous development notes:

- Dead.4.1.1 removed `--basename` and old layout options.
- Some command-line option names use inconsistent capitalization, such as `--exportOnlyDetails`.
- Some errors may be written to redirected stderr but not displayed in the Windows terminal.
- `VCommandLine::showHelp()` has been modified to use a console when available, otherwise show a dialog.
- Relative-path regression tests exist but may not have been built or executed.
- Log cleanup now uses absolute paths without changing the working directory.

Treat these as historical notes, not verified descriptions of current code.

**Inspect the implementation before changing it.**

## 5. Phase 1 — Audit and implementation plan

Inventory every advertised Seamly2D CLI option.

Inspect the definitions, parsing, validation, forwarding, execution, tests, and documentation.

Determine:

1. Exact option names and aliases.
2. Accepted values and defaults.
3. Required companion parameters.
4. Conflicting or unsupported combinations.
5. Mapping to SeamlyLayout settings.
6. Whether the option actually affects exported output.
7. Current tests and missing coverage.
8. Existing defects and implementation gaps.

Create `CLI_1_OPTION_MATRIX.md`.

The matrix must contain:

| Field | Description |
|---|---|
| UI setting | Corresponding Layout Settings control |
| CLI option | Exact supported option |
| Accepted values | Valid parameters and ranges |
| Default | Actual implementation default |
| Dependencies | Required companion options |
| Constraints | Unsupported combinations |
| SeamlyLayout mapping | Forwarded option or setting |
| Implementation status | Working, broken, ignored, or unverified |
| Test coverage | Test name and status |

**Organize layout-related options in the same order as the Layout Settings dialog, beginning with "Layout Mode" and ending with "Margins".**

Put non-layout options in a separate section.

Inspect the actual QML and application code rather than inventing options.

Create `CLI_1_PLAN.md` containing a numbered implementation plan in dependency order.

## 6. Phase 2 — Repair the CLI

Correct all confirmed defects.

Required behavior:

- All advertised, supported CLI options parse successfully.
- Invalid values produce clear errors and nonzero exit status.
- Required companion parameters are enforced.
- Unsupported combinations are rejected explicitly.
- Layout and export settings reach SeamlyLayout correctly.
- Relative paths work independently of the launch directory.
- Missing input files produce meaningful diagnostics.
- `--help` works reliably from a terminal.
- Windows console errors appear without requiring redirection.
- SeamlyLayout launch and communication failures are handled correctly.
- Large SVG input can be transferred without truncation.
- Failed exports do not leave misleading final output files.

Evaluate Windows `AttachConsole` and executable subsystem behavior before selecting an implementation.

Ensure normal Seamly2D GUI startup remains unchanged.

Consider consistent lowercase or kebab-case aliases while preserving existing accepted names.

Do not reintroduce removed legacy options without a documented requirement.

## 7. Phase 3 — Automated CLI testing

Create repeatable PowerShell `.ps1` scripts to exercise every supported option and layout/export combination.

### Test patterns

Multisize:

`test-seamly-layout/patterns/jacket2_40-146.sm2d`

Individual size:

`test-seamly-layout/patterns/male_shirt.sm2d`

`test-seamly-layout/patterns/trousers.sm2d`

Use the repository's actual pattern and measurement fixtures. Identify missing files instead of fabricating replacements.

### Export formats

Test:

- PDF.
- PDF (tiled).
- DXF (R13).
- SVG.

Include other supported formats discovered during the audit.

### Required settings coverage

Test:

- Every Layout Mode.
- Multisize size and height selections.
- Marker, Nested, Baseline, and individual sizes where supported.
- Measurement files and measurement overrides.
- Paper dimensions and orientation.
- Paper > Tiled.
- Paper > Roll.
- Fabric (folded).
- Grainline: With Nap.
- Margins.
- Piece splitting.
- `--text2paths`.
- Output basename and directory handling.
- Missing or invalid inputs.
- Invalid or incompatible setting combinations.
- Console error messages and exit codes.

### Test execution

For every test:

1. Execute the actual CLI.
2. Record command arguments.
3. Capture stdout and stderr.
4. Capture exit code.
5. Verify expected success or failure.
6. Verify output existence and size.
7. Validate file structure and content where possible.
8. Verify the output corresponds to the requested settings.
9. Record results in a machine-readable report.

Do not declare a test successful merely because an output file exists.

Keep generated files outside source fixture directories.

Extend `tst_vcommandline` and applicable Rust/Qt tests.

Add an end-to-end test using the Richmond pattern if that fixture exists.

## 8. Phase 4 — Interactive Rust/Iced CLI testing application

Create a standalone interactive application using Rust and Iced.

Qt is not required for the testing application.

The application must construct CLI arguments from user selections, display them, execute the commands, and verify the resulting files.

Use existing CLI functionality rather than duplicating layout-generation logic.

### Screen 1 — Inputs and layout settings

Allow selection of:

- Pattern file.
- Measurement file when required.
- Layout settings.
- Output basename.
- Output directory.

Use file pickers where practical.

Defaults:

- Pattern basename for output naming.
- Application preferences' layout directory for output location.

If a piece exceeds usable media width, automatically select **Split the pieces**.

Determine this from actual piece geometry and media constraints.

### Screen 2 — Layout selection

For multisize patterns, show a Layout dropdown with valid choices derived from the pattern and layout settings.

Possible choices:

- Marker.
- Nested.
- Baseline.
- Available individual sizes.

If splitting is enabled, disable Marker and Nested when unsupported.

Avoid an unnecessary second screen for individual-size patterns.

### Screen 3 — Export format

Show export formats permitted by the selected layout and settings.

Examples:

- Enable PDF (Tiled) only when its tiled-media requirements are satisfied.
- Disable DXF options when piece splitting makes them unsupported.

Make invalid options unavailable or clearly disabled with an explanation.

Base these rules on the verified option matrix, not guessed restrictions.

### Execution and output

For each material:

1. Construct the complete Seamly2D CLI invocation.
2. Display the command for inspection and copying.
3. Execute using a structured argument list, not unsafe shell-string interpolation.
4. Capture stdout, stderr, and exit code.
5. Verify all expected outputs.
6. Display per-material results.
7. Generate a ZIP archive only when all required outputs succeed.

Output naming:

`<basename>_<material>_<YYMMDDHHMM>.<ext>`

Archive naming:

`<basename>_<YYMMDDHHMM>.zip`

Sanitize filename components.

Use one consistent timestamp per export session.

Handle duplicate output names safely.

Preserve generated output files unless project requirements explicitly specify otherwise.

## 9. Phase 5 — Regression verification

Run the complete test matrix.

Verify:

1. Every supported CLI option.
2. All valid option combinations represented in the test plan.
3. Invalid option handling.
4. Multisize exports.
5. Individual-size exports.
6. Measurement overrides.
7. All supported output formats.
8. File validity and requested settings.
9. Windows console output.
10. Relative and absolute file paths.
11. Normal Seamly2D GUI startup.
12. SeamlyLayout headless operation.
13. Error propagation and exit codes.
14. Rust/Iced application behavior.
15. ZIP archive contents.

Record executed results and identify any platform-specific verification gaps.

## 10. Phase 6 — Documentation

Update:

- CLI `--help`.
- Repository CLI documentation.
- Workflow examples.
- Option mapping.
- Supported export formats.
- Valid and invalid parameter combinations.
- Exit codes and diagnostics.
- Interactive testing application instructions.

Documentation must describe verified behavior, not merely intended behavior.

## 11. Persistent state and session handover

Maintain:

- `CLI_1_PLAN.md`
- `CLI_1_OPTION_MATRIX.md`
- `SESSION_HANDOVER.md`
- `CLI_1_TEST_RESULTS.md`

Update `SESSION_HANDOVER.md` after each significant milestone.

Record:

- Current branch and commit.
- Verified completed tasks.
- Current incomplete task.
- Relevant modified files.
- Test commands and results.
- Unresolved defects.
- Blockers.
- Exact next actions.

On restart, read these files before modifying code.

Do not repeat verified work unless regression testing identifies a problem.

Commit logically complete changes to `feature/cli-1`.

Do not push or merge.

## 12. Autonomous operating rules

1. Work in dependency order.
2. Continue without routine approval requests.
3. Reuse working implementations.
4. Keep changes focused.
5. Build and test after significant changes.
6. Fix regressions before continuing.
7. Do not weaken tests to obtain passing results.
8. Do not silently ignore unsupported options.
9. Do not claim unexecuted tests passed.
10. Do not create concurrent agents that edit the same files.
11. Do not modify unrelated files or branches.
12. Do not delete untracked user files.
13. Record genuine blockers precisely.
14. Avoid unlimited retries of identical failures.
15. Continue with independent tasks when one task is blocked.
16. Keep the implementation plan and handover synchronized.
17. Update task completion checkboxes only after verification.

## 13. Definition of done

CLI.1 is complete only when:

- Every advertised CLI option has documented, verified behavior.
- Every supported option has automated test coverage.
- Valid combinations produce verified outputs.
- Invalid combinations fail with useful diagnostics.
- Console help and error messages work.
- Headless SeamlyLayout export succeeds.
- The interactive Rust/Iced application can generate and execute CLI commands.
- Multi-material exports are generated and archived correctly.
- Normal GUI behavior remains unchanged.
- Documentation matches tested functionality.
- Available unit and integration tests pass.
- All known regressions introduced by this work are resolved.
- Remaining environmental limitations are explicitly documented.

A checked task box is not proof of completion. Actual verification is required.

## 14. Final reporting

When complete, provide a concise report:

- Completed changes.
- Files modified.
- Commits created.
- Tests passed, failed, and skipped.
- Supported and unsupported options.
- Remaining defects or environmental limitations.
- Readiness for review and merging into `run-seamlyLayout`.

## 15. Initial execution

Begin immediately by:

1. Verifying the worktree and branch.
2. Reading the existing code and tests.
3. Auditing the previously completed CLI.1 tasks.
4. Creating the option matrix.
5. Creating the dependency-ordered implementation plan.
6. Implementing and testing the first incomplete task.
7. Continuing through all remaining phases.

**Do not stop after generating the plan. Continue until the implementation satisfies the verified definition of done or a genuine external blocker prevents further useful work.**