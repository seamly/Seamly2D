# Future TODO's

Add task in TODO_MIGRATE.md -->

1. in directory src/libs/vmisc/, rename these files & update the code to reference the new file names:
   a. vcommonsettings.cpp & vcommonsettings.h to settings_common.cpp & settings_common.h
   b. vseamlymesettings.cpp & vseamlymesettings.h to settings_seamlyme.cpp & settings_seamlyme.h
   c. vsettings.cpp & vsettings.h to settings_seamly2d.cpp & settings_seamly2d.h
2. Update the 'Recommended installation for development on all platforms (Linux, MacOSX, Windows)' section and 'Building Seamly' section in .github/README_DEVELOPER.md to reflect the current method to build Seamly (Seamly2D, SeamlyM)using Windows, Linux, and MacOS on a local PC.

Add task in TODO_SEAMLY2D.md -->

1. fix the errors listed in BUILD_PROBLEMS.md

Add task in TODO_SEAMLYME.md -->

Add task in TODO_SEAMLYLAYOUT.md -->

1. rename `src\app\seamlylayout\crates\app_core\src\lib.rs` to `src\app\seamlylayout\crates\app_core\src\lib_seamlylayout.rs` and update the code to refence the new file name

Add rule to CLAUDE.md -->

Ask Claude -->

1. We need to merge seamlylayout's repo into seamly2d's repo. Should we:
   a. Incorporate the information from `src\app\seamlylayout\.github\copilot-instructions.md` into the project `src\app\seamlylayout\.github` directory, then delete `src\app\seamlylayout\.github\copilot-instructions.md` file
   b. incorporate the information from `src\app\seamlylayout\.claude` directory into the project `src\app\seamlylayout\.claude` directory, then delete the `src\app\seamlylayout\.claude`folder
