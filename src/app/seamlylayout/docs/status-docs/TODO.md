# TODO

Author: slspencer
Copyright: 2026

Last triaged: 2026-06-30 (Adjust rotation-pivot task added and immediately completed: piece rotation center point moved from the bounding-box upper-left corner to the bounding-box center)

This file is intentionally **active tasks only**.

- Migration status board: `docs/status-docs/MIGRATION_STATUS.md`
- Architecture decisions/spec rationale: `docs/status-docs/DECISIONS.md`
- Completed task log: `docs/status-docs/TODO_COMPLETED.md`

Completion workflow: when a task is done, remove it from this file and move it to `TODO_COMPLETED.md`.

## Tag Schema (strict)

- `Status`: `Active` | `Blocked` | `Done` | `Superseded` | `NeedsDecision` | `NeedsClarification`
- `Priority`: `P0` | `P1` | `P2` | `P3` | `P4`
- `BlockedBy`: `none` or a concrete dependency (task/PR/issue)

Task format:

- `[ ] Task text` — `Status:<...> | Priority:<...> | BlockedBy:<...>`

## Current Sprint

When these tasks are completed add to TODO_COMPLETED.md

**General:**

**Preferences:**

**Settings:**

**Layout:**

**Import:**

**Adjust:**

- [ ] In `qt_frontend/CMakeLists.txt` (~line 298), add `set_tests_properties(AdjustSceneTests AdjustControllerTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")` after `add_test(NAME AdjustControllerTests COMMAND AdjustControllerTests)` so CTest forces the offscreen platform and these QtWidgets tests don't fail in headless CI environments. — `Status:Active | Priority:P1 | BlockedBy:none`
- [ ] In `qt_frontend/tests/adjust/AdjustControllerTests.cpp` (after `controller.closeAdjustWindow()` in the first-launch test), add `QTest::qWait(0);` to drain the event loop after closing the window. `AdjustWindow` is `WA_DeleteOnClose`, so destruction is deferred until the event loop runs; without this call, the window can outlive the test scope and cause flaky behavior or stray top-level windows in headless CI runs. — `Status:Active | Priority:P1 | BlockedBy:none`
- [ ] In `qt_frontend/tests/adjust/AdjustControllerTests.cpp` (after `controller.closeAdjustWindow()` in the second-launch/reload test, ~line 200), add `QTest::qWait(0);` to process pending events so the `WA_DeleteOnClose` window is actually deleted before the test returns — same `DeleteOnClose` deferral issue as the first-launch path; without it the window outlives the test scope causing flakiness and resource leakage between tests. — `Status:Active | Priority:P1 | BlockedBy:none`

**Export:**

When each task is completed, add to TODO_COMPLETED.md:

- [ ] E.6 Implement 3D export (dialog + bridge + renderer) with debug messages — implemented separately with a separate license (this feature is a 'paid' feature) — implemented after all other features of SeamlyLayout have been implemented, including the build pipeline with installation executables (e.g. *.msi file) in GitHub Actions, and connecting this application to Seamly2D through a new Seamly2D 'Advanced Layout' mode. — `Status:Active | Priority:P2 | BlockedBy:none`

**View:**

## Active Backlog

### Core functional

When these tasks are completed, add to TODO_COMPLETED.md:

### Layout/packing

When these tasks are completed, add to TODO_COMPLETED.md:

### Export

When these tasks are completed, add to TODO_COMPLETED.md:

### Validation & testing

When these tasks are completed, add to TODO_COMPLETED.md:

## Decision Queue

When these tasks are completed, add to TODO_COMPLETED.md:

Decision metadata is tracked in `docs/status-docs/DECISIONS.md`.
