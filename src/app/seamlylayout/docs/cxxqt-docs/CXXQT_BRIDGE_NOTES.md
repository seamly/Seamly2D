# CXX-QT with Qt 6 and CMAKE

Rust code is "bridged" to Qt's QML code using CXX-QT. CXX-QT is relatively new but under active development.

## Build Problem:

Tthe CXX-Qt generated sources weren't included in `qt_add_qml_module`, the `qmltyperegistrar` never saw the `QML_ELEMENT` declarations, so `AppController` never got registered in the QML module's type list. When QML tries to instantiate it at runtime, the type lookup fails because it's not in the registry.

## Investigation:

- `cxx_qt_import_crate `- generates C++ files with QML_ELEMENT declarations. These C++ files should be passed to `qt_add_qml_module `for `qmltyperegistrar` to process by:

  - explicitly adding the generated sources to the target,OR
  - using a specific CXX-Qt CMake helper function to wire everything together. We will provide a function `cxx_qt_import_qml_module` that handles this automatically.
- The `#[qml_element]` macro generates both a C++ class and registration code that needs to execute via static constructors in the auto-generated files.
- `QML_ELEMENT` works as follows:

  - `qmltyperegistrar` tool scans all C++ classes with `QML_ELEMENT` at compile time
  - it generates registration code that's included in the QML module
  - this happens via `qt_add_qml_module` which runs `qmltyperegistrar`
- `cxx_qt_import_crate` works as follows:

  - it exports the generated C++ files to `CXX_QT_EXPORT_DIR`
  - the `cxxqt_import_crate` cmake function adds these to the `SOURCES` of the QML module target
  - `qmltyperegistrar` finds the `QML_ELEMENT` declaration.
  - does `cxx_qt_import_crate` take a QML_MODULE parameter?
- After calling `cxx_qt_import_crate`:

  - call `cxx_qt_import_qml_module`, (handles QML module registration), OR
  - pass the generated sources to `qt_add_qml_module`
- Environment variables:

  - `CXX_QT_EXPORT_CRATE_cxxqt_bridge=1 `
  - `CXX_QT_EXPORT_DIR=C:/src/seamlyLayout/qt_frontend/build/Debug/cxxqt`
- The CMake configuration sets up the QML module and links the CXX-Qt generated bridge. However, `qt_add_qml_module` doesn't automatically know about the C++ headers that CXX-Qt generates—it only sees the QML files explicitly listed. The generated `AppController` types need to be registered with `qmltyperegistrar`, which means those headers have to be included in the module's sources somehow, either by adding them to the CMake target or by ensuring the CXX-Qt integration exposes them properly.
- For CXX-Qt's registration to work with `qt_add_qml_module`, the generated C++ code needs to be compiled as part of the same module, which typically happens through cxx_qt_import_crate.
- For QML type registration to work, the generated C++ must be compiled into the QML module plugin, not just linked.
- Must know the exact variable name that `cxx_qt_import_crate` exports for the generated C++ files, since the build output shows they're being placed in `CXX_QT_EXPORT_DIR`. The difficulty is how to reference them in the CMake configuration. `CXX_QT_EXPORT_CRATE_cxxqt_bridge=1 ` and `CXX_QT_EXPORT_DIR=C:/src/seamlyLayout/qt_frontend/build/Debug/cxxqt` environment variables are set when cargo builds the crate. This means CXX-Qt knows to export files to that directory.
- CXX-Qt's `build.rs` exports files: it writes `.cpp `and `.h `files to `CXX_QT_EXPORT_DIR` when that env var is set. The files for our bridge would be something like:

  * `C:/src/seamlyLayout/qt_frontend/build/Debug/cxxqt/cxxqt_bridge/src/lib.rs.cpp`
  * `C:/src/seamlyLayout/qt_frontend/build/Debug/cxxqt/cxxqt_bridge/src/lib.rs.h`
- `qmltyperegistrar` scans the SOURCES passed directly to `qt_add_qml_module`, not linked libraries, `cxx_qt_import_crate` builds the Rust crate and creates a cmake target with the generated C++ --> these sources need to be explicitly added to the QML module's SOURCES for type registration to work.
- So ---> How to get the generated file paths into the cmake configuration?

  * The `qt_add_qml_module` call has no SOURCES that include the CXX-Qt generated C++ files.
  * The `qmltyperegistrar` tool only scans files listed in SOURCES of `qt_add_qml_module`, so it never sees the `QML_ELEMENT` macro declaration in the CXX-Qt generated headers.
  * The fix requires adding the CXX-Qt generated sources to `qt_add_qml_module`.

  ### Discussion

  I need to find the right CMake variable that exposes those generated files—likely something like `CXX_QT_EXPORT_DIR` or a target property that lists the generated sources—so I can pass them to `qt_add_qml_module`'s `SOURCES` argument.

  The issue is clear. The `qt_add_qml_module` call has no `SOURCES` that include the CXX-Qt generated C++ files. The `qmltyperegistrar` tool only scans files listed in `SOURCES` of `qt_add_qml_module`, so it never sees the `QML_ELEMENT` macro declaration in the CXX-Qt generated headers.

  The fix requires adding the CXX-Qt generated sources to `qt_add_qml_module`. Looking at `cxx_qt_import_crate`, it provides a target `cxxqt_bridge` which has the generated C++ sources attached.

  So the pattern is: call `cxx_qt_import_crate`, then use `get_target_property` to extract the generated sources, and pass them to `qt_add_qml_module`. This ensures the C++ bridge code gets compiled and linked properly with the QML module. I see that `cxx_qt_import_crate` generates C++ files to a specific directory and likely exposes them through a target property or CMake variable, but the real challenge is getting those generated sources into the QML type registrar so it can process them correctly. The standard approach uses target objects, though that doesn't directly solve the registration problem.

  Looking at this more carefully, I need to find where CXX-Qt exports its generated C++ files and then pass those paths to `qt_add_qml_module` so the type registrar can scan them for QML metadata. There's likely a CMake variable or property from the `cxx-qt-cmake` integration that points to the generated source directory, which I can then include in the `SOURCES` list. I'm trying to figure out the exact CMake variable that CXX-Qt uses to expose generated sources—it's either a crate-specific variable with an uppercase naming convention or the files are placed in a predictable binary directory path. Let me trace through what actually gets generated after calling `cxx_qt_import_crate` to find the reliable way to reference those sources.

  Setting up CMake configuration... I need to verify the exact directory structure and variable naming that `cxx_qt_import_crate` produces by checking the CXX-Qt documentation and examples, since the standard pattern shows the macro sets up generated sources that get passed directly to `qt_add_qml_module`.

  Looking at how `cxx_qt_import_crate` actually works under the hood, it wraps `corrosion_import_crate` and then exports the generated sources using a specific variable naming convention—the crate name converted to uppercase with dashes replaced by underscores, so `cxxqt_bridge` would become `CXXQT_BRIDGE_GENERATED_SOURCES`. After `cxx_qt_import_crate` runs, it sets a variable with the generated C++ source files that I can pass directly to `qt_add_qml_module`, though I need to mark them as generated since they won't exist until the cargo build completes. I need to verify the actual variable name that `cxx_qt_import_crate` sets, so I'm going to check the FetchContent download directory to see what's available in the cmake repository.

### Details

From reading `CxxQt.cmake`:

1. `cxx_qt_import_crate` exports files to `${CMAKE_CURRENT_BINARY_DIR}/cxxqt/` by default.
2. The structure is `${export_dir}/crates/${CRATE}/include/` for headers
3. For static library crates, it creates `initializers.o` in `${export_dir}/crates/${CRATE}/`
4. For QML modules specifically (`#[qml_element]` in Rust), there's a `cxx_qt_import_qml_module` function that handles the QML plugin registration separately. This is for when the Rust crate itself exports a complete QML module (as a plugin).

Now I'm trying to figure out whether #[qml_element] on AppController in a staticlib crate will work with cxx_qt_import_crate, or if I need to use cxx_qt_import_qml_module instead. Looking at how cxx_qt_import_qml_module handles static libraries, it seems to generate a plugin_init.o file that...

Setting up the CMake call...The key insight is that cxx_qt_import_qml_module only creates its target if the corresponding static library exists — in our case, cxxqt_bridge-static should be available after importing the crate, so the function should successfully generate the QML module target that we can then link into the executable.

Now I'm thinking through how #[qml_element] actually works in CXX-Qt — when you mark a type with this attribute, the build system registers it as a QML element, but for it to be accessible in QML, the containing module needs proper registration with Qt's type system, which is what the QML module setup handles. I need to trace through what cxx_qt_import_qml_module actually does for static libraries — it's looking for plugin_init.o at a specific path under the export directory based on the module name, and I'm realizing I should verify what the actual build output shows rather than guessing at the behavior.

Now I understand the issue. Looking at CxxQt.cmake, I can see there's a cxx_qt_import_qml_module function specifically for this. Let me check the build.rs and what actually gets exported.

So the CXX-Qt generated C++ files are:

* Headers: `cxxqt/crates/cxxqt_bridge/include/cxxqt_bridge/src/lib.cxx.h` and `lib.cxxqt.h`
* Object: `cxxqt/crates/cxxqt_bridge/initializers.o`

But there are no generated .cpp files listed. Let me check what's in the cxxqt directory.

The headers exist but no source files. Let me check what the qml_modules directory looks like.

### The Fix for problems:

**1. Build error:** "AppController is not a type"

**Root cause:**`qt_add_qml_module` runs `qmltyperegistrar` only on its listed SOURCES. The CXX-Qt generated `AppController` class was linked but never registered with Qt's QML type system.

**Why the obvious fix (adding the generated header to SOURCES) doesn't work:**

* CXX-Qt's `build.rs` compiles the moc output for `AppController` into `cxxqt_bridge.lib`
* Adding `lib.cxxqt.h` to SOURCES causes AUTOMOC to generate a second moc output for the same class
* MSVC linker raises LNK2005: duplicate `AppController::staticMetaObject`

**Why the QML_FOREIGN shim doesn't work either:**

* `AppControllerForeign.h` with `QML_FOREIGN(AppController)` was added to SOURCES
* `qmltyperegistrar` cannot follow CXX-Qt's complex template include chain (`rust/cxx.h`)
* Build succeeds but produces warnings: "AppController is declared as foreign type, but cannot be found"
* The type is never actually registered — same "AppController is not a type" error at runtime

**The final fix — runtime `qmlRegisterType`:**

* **`crates/cxxqt_bridge/src/lib.rs`** — removed `#[qml_element]` from the bridge (not needed)
* **`qt_frontend/CMakeLists.txt`** — `qt_add_qml_module` has NO SOURCES (no generated headers, no shim)
* **`qt_frontend/main.cpp`** — register at runtime before creating the engine:
  ```cpp
  #include <cxxqt_bridge/src/lib.cxxqt.h>  // include only; do NOT add to cmake SOURCES
  // ...
  qmlRegisterType<AppController>("SeamlyLayout", 1, 0, "AppController");
  QQmlApplicationEngine engine;
  engine.loadFromModule("SeamlyLayout", "Main");
  ```

  This bypasses `qmltyperegistrar` entirely. Qt resolves `AppController` in QML via the "SeamlyLayout" URI registered at runtime.

**2. Build error:** "Cannot assign to non-existent property 'onErrorOccurred'"

**Root cause:** CXX-Qt generates C++ names in snake_case by default (`error_occurred`, `imported_svg_path`, `import_svg`). QML signal handlers and property bindings are case-sensitive and must match the C++ name exactly. `Main.qml` was written expecting camelCase names (`onErrorOccurred`, `importedSvgPath`, `importSvg`), so every binding and handler was mismatched.

**Fix:** Added `#[auto_cxx_name]` to the `extern "RustQt"` block in `crates/cxxqt_bridge/src/lib.rs`.

This single attribute instructs CXX-Qt to automatically convert all exported C++ names from Rust snake_case to camelCase:

| Rust name (unchanged) | C++ name before       | C++ name after      |
| --------------------- | --------------------- | ------------------- |
| `error_occurred`    | `error_occurred`    | `errorOccurred`   |
| `imported_svg_path` | `imported_svg_path` | `importedSvgPath` |
| `import_svg`        | `import_svg`        | `importSvg`       |
| `is_layout_ready`   | `is_layout_ready`   | `isLayoutReady`   |

The Rust-side API (setters, getters, method names in `impl qobject::AppController`) is unaffected — `#[auto_cxx_name]` only converts the exported C++ symbols, not the Rust-facing interface.

---

## Final Resolved State (Phase 2f Complete)

**Build:** Succeeds without errors or warnings on Windows (MSVC, x64 Native Tools prompt).

**Key rules going forward:**

1. **Never use `#[qml_element]`** for staticlib CXX-Qt crates — use `qmlRegisterType` at runtime.
2. **Never add `lib.cxxqt.h` to cmake SOURCES** — include it only from `main.cpp` (or other non-SOURCE C++ files) to avoid AUTOMOC duplicate moc.
3. **Always add `#[auto_cxx_name]`** to `extern "RustQt"` blocks so QML sees camelCase names.
4. **Keep `edition = "2021"`** in `cxxqt_bridge/Cargo.toml` — `cxx-qt-build 0.7.3` is not compatible with Rust 2024 edition.
5. **Use `//` comments** inside `#[cxx_qt::bridge]` — `///` doc comments desugar to `#[doc]` attributes which are rejected by the bridge macro.
