# TODO — Remove unused (dead) layout code

Tasks for removing unsued code due to implementing SeamlyLayout, and orphaning the old layout code.

If decisions are required for any portion of a task or subtask, present the user with radio buttons to select options including 'Other'.

Check off all completed tasks & subtasks and move completed tasks to TODO_COMPLETED.md

All TODO_MIGRATE.md tasks begin with 'Dead.' and all tasks are numbered.

## TASK Dead.1 - Remove the orphaned layout code; orphaned when calling SeamlyLayout instead of the previous layout code and workflow

- [ ] Dead.1.1 - locate the code that was orphaned by calling SeamlyLayout
- [ ] Dead.1.2 - remove the located orphaned code
- [ ] Dead.1.3 - remove the tests for the located orphaned code
- [ ] Dead.1.4 - check that tests are run for SeamlyLayout during the linux-test job in ci.yml

## Task Dead.2 - Test the build pipeline -- ci.yml

## Task Dead.3 - Test the functionality (requires install & user testing)