# Code Styles

Optimize for readability and maintainability
Utilize **JSF-AV C++ code style** rules, with a few exceptions noted below.
For more information, read full [JSF-AV standard](http://www2.research.att.com/~bs/JSF-AV-rules.pdf).

## Applies to new and edited code going forward (as of 2026/06/09):

- Documentation:
  -- Doxygen-style briefs for every file, variable, class, function, pointer, namespace, etc.
  -- Inline comments that highlight the application flow, suitable for an intermediate-level developer to understand.
- Variables: snake_case (all lowercase with underscores)
- Classes: UpperCamelCase (not JSV-AV compliant)
- Functions:
  -- Member functions `const` by default
  -- No more than 7 arguments in a function
  -- No more than 1 function exit point
- Pointers: `int* correct_pointer` (bind the asterisk to the type, not to the name)
- Namespaces: Everything in namespaces, but not more than two levels deep
- File Names:
  - Snake_case for new files: All lowercase with underscores (`application_my_new_app.cpp`)
    - Exception: Match Class names exactly for file names that define a class.
      - If a file primarily defines one class, give it the same name as the class, in UpperCamelCase.
  - No Abbreviations: Never use confusing or ambiguous shortcuts. ()`matrix_reader.cpp` not `mtx_rdr.cpp`)
  - Avoid Generic Names: Don't use names like `util.h` or `helpers.cpp`.
  - Separate Platform Code: Append platform-specific tags to files handling low-level code ()`linux_file_system.cpp`)
  - Name files for what they do. And don't start filenames with `s`!
  - Unique names: A seach for the \<filename.extension> should return only one file,
    - Exception: Crate files in SeamlyLayout require multiple `lib.rs` files distinguishable by their paths.
  - Use prefixes where applicable:
    - application_\<appname>
    - dialog_\<toolgroup>\_\<toolname>
    - tool_\<toolgroup>\_\<toolname>
    - docs_*
    - settings_*
    - options_*
    - exception_*
    - event_*
    - model_*
    - search_*
    - test_*
    - \<platform>_* (`winarm64_installation_instructions.pdf`)
- Indents: spaces, no tabs, for uniformity in rendering
- Braces: always on a new line, can't be omitted even from if/while/else/etc statements
- Line limit: 120 chars max
- Comments:
  -- Single line: `//` (not `/*...*/` to prevent accidental nesting)
  -- Multi line: `/*...*/` (not JSV-AV compliant)
- Use `#include <filename.h>` not `<filename.h>` (not JSV-AV compliant)
- No `signal.h`
- No `time.h`
- No `abort`, `exit`
- No macros (for functions, or constants)
- No unions
- No comma operator
- No use of `new` after program initialization
- No exceptions (bad "tool support")
  - explicitly bans `try`, `catch`, and `throw` approaches
  - use traditional error codes or status indicators --> enum or integer status codes
  - functions returning error information must have that information tested by the calling code
    - `if (status != SUCCESS)` immediately after every function call that returns an error code
  - use reference or out parameters:
    - Pass a pointer or non-const reference to a status or result object where the function writes its error state
    - Note: Keeping functions side-effect-free via direct return values is preferred
  - use pre-checks: Validate inputs and system states before executing operations
- No pointer arithmetic
- Recursion is allowed (not JSV-AV compliant)
  - Recursion is utilized in SeamlyLayout
