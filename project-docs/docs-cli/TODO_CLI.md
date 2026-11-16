# TODO — CLI

Check off subtasks as they are accomplished; when every subtask of a task is complete, move the task to `project-docs/TODO_COMPLETED.md`.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Tasks in this file start with `CLI.`

## Task CLI.2 - run seamly2d/seamlyme/seamlylayout from the command line with an input .sm2d file and an input measurement file to create exports for each of seamlyLayout's export options; validate outputs to validate the CLI parameters

- `src/test/CliTest/test_cli_layout_export.ps1` runs Seamly2D and SeamlyLayout for every option group and export format with male_shirt, trousers and jacket2. Results: `CLI_1_TEST_RESULTS.md`.
- SeamlyMe validates measurement inputs only; it does not read `.sm2d` patterns or export layouts.

- [ ] CLI.2.1 Richmond pattern and measurement files are not in the fixtures. Only `cxxqt_bridge/test_data/richmond-shirt-handoff_pieces.svg` exists; the suite exports it through SeamlyLayout.
- [ ] CLI.2.2 Import the exports into CAD and cutter software (user check).
