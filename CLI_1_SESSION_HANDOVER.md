# CLI.1 session handover

Branch `feature/cli-1`, worktree `C:\Users\susan\Projects\seamly2d-cli-1`. Not merged, not pushed.
The general handover is `SESSION_HANDOVER.md`; this file holds the CLI state only.

## State

- CLI.1 and CLI.3: done, in `project-docs/TODO_COMPLETED.md`. `TODO_CLI.md` keeps CLI.2.2 (CAD and cutter import).
- Plan and defects D1–D17: `CLI_1_PLAN.md`. Options: `CLI_1_OPTION_MATRIX.md`. Results, acceptance, gaps: `CLI_1_TEST_RESULTS.md`.
- Reference: `project-docs/docs-cli/CLI_PARAMETERS.md`.

## Tests (last run)

| Suite | Result |
|---|---|
| `src\test\CliTest\test_cli_layout_export.ps1 -RichmondRoot <folder>` | 130/130 |
| `src\test\CliTest\test_cli_import_check.py <report> --inkscape <inkscape.com>` | 59 files, 0 fail |
| `Seamly2DTests`, CollectionTest (start in `bin`), ParserTest, TranslationsTest | 0 fail |
| `ctest --preset release` | 9/9 |
| `cargo test --workspace` | 734/0 |
| `src\test\CliTestApp`: `cargo test` | 15/0 |
| `local_build_msi.ps1` | pass; MSI not installed |

## Commands

```powershell
$env:PATH = "C:\Qt\6.11.3\msvc2022_64\bin;$env:USERPROFILE\.cargo\bin;$env:PATH"
powershell -File src\test\CliTest\test_cli_layout_export.ps1 -FixtureRoot <test-seamly-layout> -QtBin C:\Qt\6.11.3\msvc2022_64\bin -RichmondRoot <folder>
python src\test\CliTest\test_cli_import_check.py <cli_test_report.json> --inkscape "C:\Program Files\Inkscape\bin\inkscape.com"
```

Richmond files: `git show d3197ee973^:test-seamly-layout-input/richmond-shirt_v1_v061-test.sm2d` and `.../2025-06-08-Sue.smis`.

## Open, outside this machine

- Linux and macOS runs: in CI, not a local WSL install (Sue, 2026-11-16).
- CI: needs a push (not allowed for this task).
- CAD program and cutter import (CLI.2.2): none installed.

## Next

- Sue reviews and merges `feature/cli-1` into `run-seamlyLayout`. No skip-ci token: `packaging/**` changed.
- Before a release: `gh workflow run ci.yml --ref run-seamlyLayout` covers Linux and macOS.
- `CLI_1_TASK.md` stays untracked: it mentions agents, which committed `.md` files may not.
