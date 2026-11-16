# CLI.1 session handover

Branch `feature/cli-1`, worktree `C:\Users\susan\Projects\seamly2d-cli-1`. Not merged, not pushed.
The general handover is `SESSION_HANDOVER.md`; this file holds the CLI state only.

## State

- CLI.1 and CLI.3: done, in `project-docs/TODO_COMPLETED.md`.
- Plan and defects D1–D17: `CLI_1_PLAN.md`. Options: `CLI_1_OPTION_MATRIX.md`. Results, acceptance, gaps: `CLI_1_TEST_RESULTS.md`.
- Reference: `project-docs/docs-cli/CLI_PARAMETERS.md`.

## Tests (last run)

| Suite | Result |
|---|---|
| `src\test\CliTest\test_cli_layout_export.ps1 -RichmondRoot <folder>` | 130/130 |
| `Seamly2DTests`, CollectionTest (start in `bin`), ParserTest, TranslationsTest | 0 fail |
| `ctest --preset release` | 9/9 |
| `cargo test --workspace` | 734/0 |
| `src\test\CliTestApp`: `cargo test` | 15/0 |
| `local_build_msi.ps1` | pass; MSI not installed |

## Commands

```powershell
$env:PATH = "C:\Qt\6.11.3\msvc2022_64\bin;$env:USERPROFILE\.cargo\bin;$env:PATH"
powershell -File src\test\CliTest\test_cli_layout_export.ps1 -FixtureRoot <test-seamly-layout> -QtBin C:\Qt\6.11.3\msvc2022_64\bin -RichmondRoot <folder>
```

Richmond files: `git show d3197ee973^:test-seamly-layout-input/richmond-shirt_v1_v061-test.sm2d` and `.../2025-06-08-Sue.smis`.

## Open, outside this machine

- Linux and macOS runs: in CI, not a local WSL install (Sue, 2026-11-16).
- CI: needs a push (not allowed for this task).

## Next

- Sue reviews and merges `feature/cli-1` into `run-seamlyLayout`. No skip-ci token: `packaging/**` changed.
- Before a release: `gh workflow run ci.yml --ref run-seamlyLayout` covers Linux and macOS.
- `CLI_1_TASK.md` stays untracked: it mentions agents, which committed `.md` files may not.

## 2026-11-16 — Command line help in the Help menu

- Added **Help > Command line options** on `feature/cli-1`.
- The window uses `VCommandLine::helpText()` for registered descriptions, limits, removed options, and exit codes.
- `--basename` remains removed; the window explains `--layout-export` and `--layout-output` as its replacements.
- Shared the dialog with the existing console-free `--help` fallback. Closing it from the menu does not exit Seamly2D.
- Verified the Windows Qt 6.11.3 build and all 40 Qt suites: 32628 passed, zero failures.
- The CLI suite includes 31 passing checks, including help content, scrolling, and closing without exiting.
- Linux and macOS verification remains deferred to CI. No merge or push.
