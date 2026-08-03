# project: SeamlyLayout
# author: slspencer, copyright 2026
# LGPL-3.0 License: https://www.gnu.org/licenses/lgpl-3.0.html
#
# CMakeLists.txt — Qt 6.11 + QML frontend for SeamlyLayout.
# Links against libseamly_core (Rust FFI bridge) once Phase 2g is complete.
# See docs/status-docs/TODO_SEAMLYLAYOUT_2.md for the full migration plan.

cmake_minimum_required(VERSION 3.28)

project(SeamlyLayout VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SEAMLY_ICON_ICO  "${CMAKE_CURRENT_SOURCE_DIR}/assets/images/seamly-layout.ico")
set(SEAMLY_ICON_ICNS "${CMAKE_CURRENT_SOURCE_DIR}/assets/images/seamly-layout.icns")
set(SEAMLY_ICON_PNG  "${CMAKE_CURRENT_SOURCE_DIR}/assets/images/seamly-layout_512x512.png")
set(SEAMLY_ICON_SVG  "${CMAKE_CURRENT_SOURCE_DIR}/assets/images/seamly-layout.svg")

# ---------------------------------------------------------------------------
# Windows/MSVC: Rust always links against the non-debug DLL runtime
# (MultiThreadedDLL). Set this before any targets are defined so that all
# C++ targets link against the same runtime as the Rust static library.
# Without this, the linker produces LNK2038 "mismatch detected for
# RuntimeLibrary" on debug builds.
# See: https://github.com/corrosion-rs/corrosion/blob/master/doc/src/common_issues.md
# ---------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL") # match Rust's non-debug DLL runtime
endif() # MSVC

# ---------------------------------------------------------------------------
# Qt 6.11 — require minimum 6.11.1; all modules must be LGPLv3 (no GPL modules)
#
# Task 30: bumped from 6.10.1 so SeamlyLayout shares the same Qt release as the
# seamly2d/seamlyme parent apps (Qt 6.11.1 msvc2022_64, matching ci.yml). One Qt
# version across the family lets the Windows MSI ship a single shared Qt runtime
# instead of two side-by-side copies with colliding DLL names.
# ---------------------------------------------------------------------------
find_package(Qt6 6.11.1 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    QuickDialogs2
    Svg
    SvgWidgets
    Test
    Widgets
    PrintSupport
    WebEngineQuick
    Xml
)

# Enables AUTOMOC, AUTORCC, AUTOUIC and sets standard Qt project defaults
qt_standard_project_setup(REQUIRES 6.11.1)

# ---------------------------------------------------------------------------
# Windows/MSVC: map Qt imported targets' Debug config to Release.
#
# Rust always links against the non-debug (MultiThreaded DLL) runtime. Qt's
# debug import libraries link against the MSVC debug runtime. Setting
# MAP_IMPORTED_CONFIG_DEBUG on each Qt target tells cmake to use the Release
# import library location when building in Debug mode, avoiding LNK2038.
#
# Important: set this on specific Qt targets only, not via the global
# CMAKE_MAP_IMPORTED_CONFIG_DEBUG variable. The global form remaps ALL
# imported targets (including Corrosion's cxxqt_bridge-static), causing
# cmake to look for the Release Rust output which doesn't exist yet, and
# producing a 'cxxqt_bridge-static-NOTFOUND' linker error.
#
# See: https://github.com/KDAB/cxx-qt/pull/683
# ---------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    foreach(_qt_target IN ITEMS
            Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2
            Qt6::QuickDialogs2 Qt6::Svg Qt6::SvgWidgets Qt6::Widgets
            Qt6::PrintSupport Qt6::WebEngineQuick Qt6::Test
            Qt6::Qml Qt6::QmlModels Qt6::Network Qt6::Xml)
        if(TARGET ${_qt_target}) # guard: target may not exist if module not found
            set_property(TARGET ${_qt_target}
                PROPERTY MAP_IMPORTED_CONFIG_DEBUG "RELEASE") # use Release libs in Debug build
        endif() # TARGET ${_qt_target}
    endforeach() # _qt_target
endif() # MSVC

# ---------------------------------------------------------------------------
# Phase 2f: CXX-Qt CMake integration
#
# The cmake integration for CXX-Qt lives in a separate cmake-only repository:
#   https://github.com/kdab/cxx-qt-cmake.git
# This repo provides cxx_qt_import_crate (a wrapper over corrosion_import_crate
# from the Corrosion cmake-Rust bridge). It is distinct from the main CXX-Qt
# source repo (https://github.com/KDAB/cxx-qt.git) which contains only the
# Rust crates brought in via Cargo.toml.
#
# Workflow:
#   1. FetchContent downloads cxx-qt-cmake and makes it available as a
#      cmake subdirectory; this defines cxx_qt_import_crate in this scope.
#   2. cxx_qt_import_crate invokes Corrosion to run cargo build on the
#      cxxqt_bridge crate and produces a linkable CMake target.
#   3. The cxxqt_bridge target is linked into SeamlyLayout below.
# ---------------------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)  # don't re-clone on every configure
FetchContent_Declare(
    CxxQt
    GIT_REPOSITORY https://github.com/kdab/cxx-qt-cmake.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(CxxQt)

# ---------------------------------------------------------------------------
# Application executable
# ---------------------------------------------------------------------------
qt_add_executable(SeamlyLayout
    WIN32
    main.cpp
    src/Logger.cpp
    src/Platform.cpp
    src/SettingsModel.cpp
    src/StartupOptions.cpp
    src/PreferencesModel.cpp
    src/PreferencesController.cpp
    src/PreferencesWindow.cpp
    src/adjust/AdjustController.cpp
    src/adjust/PieceOverlayItem.cpp
    src/adjust/AdjustScene.cpp
    src/adjust/AdjustWindow.cpp
)

if(WIN32)
    target_sources(SeamlyLayout PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/app_icon.rc"
    )
elseif(APPLE)
    target_sources(SeamlyLayout PRIVATE
        "${SEAMLY_ICON_ICNS}"
    )
    set_source_files_properties("${SEAMLY_ICON_ICNS}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    # Task 16: give the bundle a stable identifier (previously unset, so CMake fell back
    # to an auto-generated placeholder). This does not affect settings storage — Qt's
    # QStandardPaths::AppConfigLocation resolution is driven by organizationName/
    # applicationName (see main.cpp), not CFBundleIdentifier — but a real identifier is
    # still required for a well-formed, signable/notarizable bundle.
    set_target_properties(SeamlyLayout PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE "seamly-layout.icns"
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.seamly.SeamlyLayout"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

# ---------------------------------------------------------------------------
# Icon resources — embedded application icons (self-contained in qt_frontend)
# ---------------------------------------------------------------------------
set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/images")
qt_add_resources(SeamlyLayout "icons"
    PREFIX "/icons"
    BASE "${ASSETS_DIR}"
    FILES
        "${SEAMLY_ICON_SVG}"
        "${SEAMLY_ICON_PNG}"
)

# ---------------------------------------------------------------------------
# Default configuration resources — first-run seeded preference defaults
# ---------------------------------------------------------------------------
qt_add_resources(SeamlyLayout "defaults"
    PREFIX "/defaults"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/preferences"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/preferences/default_preferences.json"
)

# ---------------------------------------------------------------------------
# QML module — URI "SeamlyLayout" exposes C++ types and QML files
#
# Phase 2f: AppController.h/.cpp removed from SOURCES — CXX-Qt now provides
# AppController as a QML_ELEMENT via the bridge crate. Both the C++ stub and
# the CXX-Qt type declare QML_ELEMENT on the same name, which would cause a
# duplicate registration conflict if both were compiled into the module.
# The C++ files remain on disk and are deleted in Phase 3.
# ---------------------------------------------------------------------------

# Mark Theme.qml as a singleton (required for pragma Singleton to work)
set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

# OUTPUT_DIRECTORY: the QML module URI ("SeamlyLayout") equals the executable
# target name. By default Qt materialises the module (qmldir, .qmltypes, cached
# qml) in a directory named after the URI directly under the binary dir —
# i.e. <build>/SeamlyLayout/. On Windows the executable is SeamlyLayout.exe, so
# that directory and the binary never clash; but on Linux/macOS the executable
# is just "SeamlyLayout", which collides with the module directory and makes the
# final link fail with "cannot open output file SeamlyLayout: Is a directory".
# Redirecting the module under its own qml_modules/ parent keeps the leaf name
# "SeamlyLayout" (so `import SeamlyLayout` still resolves via the added import
# path) while moving it clear of the executable. Runtime QML still loads from the
# compiled-in resources, so this only affects on-disk build layout.
qt_add_qml_module(SeamlyLayout
    URI SeamlyLayout
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_modules/SeamlyLayout"
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/SeamlyButton.qml
        qml/TopMenuBar.qml
        qml/ExportMenu.qml
        qml/SvgCanvas.qml
        qml/SettingsDialog.qml
        qml/PreferencesPanel.qml
        qml/DxfTeachingDialog.qml
        qml/ViewDxfTeachingDialog.qml
)
# AppController is registered at runtime via qmlRegisterType in main.cpp.
# QML_FOREIGN (AppControllerForeign.h) was tried first but qmltyperegistrar
# cannot resolve AppController through CXX-Qt's complex template include chain,
# producing "cannot be found" warnings and no actual registration.

# ---------------------------------------------------------------------------
# Phase 2f: import and link the CXX-Qt bridge crate
#
# MANIFEST_PATH points to the workspace Cargo.toml so that Corrosion can
# resolve workspace-level dependencies for cxxqt_bridge.
# CRATES names the specific crate to build from that workspace.
# QT_MODULES lists the Qt targets that the generated C++ glue depends on;
# cxx-qt-cmake passes them to Corrosion so the Rust build finds Qt headers.
# ---------------------------------------------------------------------------
cxx_qt_import_crate(
    MANIFEST_PATH "${CMAKE_SOURCE_DIR}/../Cargo.toml"
    CRATES        cxxqt_bridge
    QT_MODULES    Qt::Core Qt::Qml
)

# ---------------------------------------------------------------------------
# Include paths
# ---------------------------------------------------------------------------
target_include_directories(SeamlyLayout PRIVATE
    src/
    # Phase 2g: uncomment when libseamly_core is built
    # ${CMAKE_SOURCE_DIR}/../crates/ffi_bridge/include
)

# ---------------------------------------------------------------------------
# Link Qt libraries — all LGPLv3; do NOT add GPL-only modules (Qt Charts, etc.)
# ---------------------------------------------------------------------------
target_link_libraries(SeamlyLayout PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickDialogs2
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::WebEngineQuick
    Qt6::Xml
    cxxqt_bridge  # Phase 2f: CXX-Qt generated AppController QObject
)

# ---------------------------------------------------------------------------
# Qt frontend tests
#
# Task 58 moved the four Qt/C++ test suites out of this app tree and into
# `src/test/SeamlyLayoutTest/`, alongside the rest of the family's tests
# (`Seamly2DTest`, `ParserTest`, `TranslationsTest`, `CollectionTest`). They
# stay CMake-built and ctest-run from here — `src/test/test.pro` deliberately
# does NOT list `SeamlyLayoutTest`, because seamlyLayout is kept out of the
# Seamly2D qmake build.
#
# The Rust tests did not move: `#[cfg(test)]` modules compile as part of their
# crate and reach its private items, and Cargo requires integration tests to
# sit beside the crate's `Cargo.toml`. They remain under `crates/*/src/`, run
# by `cargo test --workspace`.
#
# SEAMLYLAYOUT_TEST_DIR is resolved once, as an absolute path, so the four
# targets below never repeat a fragile `../../../` literal.
#   qt_frontend -> seamlylayout -> app -> src, then down into test/
# ---------------------------------------------------------------------------
enable_testing()

get_filename_component(SEAMLYLAYOUT_TEST_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../test/SeamlyLayoutTest"
    ABSOLUTE
)

# Fail loudly at configure time rather than with a confusing "no such source
# file" error if the tests are ever moved again without updating this path.
if(NOT EXISTS "${SEAMLYLAYOUT_TEST_DIR}")
    message(FATAL_ERROR
        "SeamlyLayout test directory not found: ${SEAMLYLAYOUT_TEST_DIR}"
    )
endif()

add_executable(AdjustSceneTests
    ${SEAMLYLAYOUT_TEST_DIR}/AdjustSceneTests.cpp
    src/adjust/PieceOverlayItem.cpp
    src/adjust/AdjustScene.cpp
)

target_include_directories(AdjustSceneTests PRIVATE
    src/
)

target_link_libraries(AdjustSceneTests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::Test
    Qt6::Widgets
    Qt6::Xml
)

add_test(NAME AdjustSceneTests COMMAND AdjustSceneTests)

add_executable(AdjustControllerTests
    ${SEAMLYLAYOUT_TEST_DIR}/AdjustControllerTests.cpp
    src/adjust/AdjustController.cpp
    src/adjust/AdjustWindow.cpp
    src/adjust/AdjustScene.cpp
    src/adjust/PieceOverlayItem.cpp
    src/Logger.cpp
)

target_include_directories(AdjustControllerTests PRIVATE
    src/
)

target_link_libraries(AdjustControllerTests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::Test
    Qt6::Widgets
    Qt6::Xml
)

if(WIN32)
    target_link_libraries(AdjustControllerTests PRIVATE dwmapi)
endif()

add_test(NAME AdjustControllerTests COMMAND AdjustControllerTests)

set_tests_properties(AdjustSceneTests AdjustControllerTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(PreferencesModelTests
    ${SEAMLYLAYOUT_TEST_DIR}/PreferencesModelTests.cpp
    src/PreferencesModel.cpp
    src/Logger.cpp
    src/Platform.cpp
)

target_include_directories(PreferencesModelTests PRIVATE
    src/
)

target_link_libraries(PreferencesModelTests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    Qt6::Widgets
)

if(WIN32)
    target_link_libraries(PreferencesModelTests PRIVATE dwmapi)
endif()

add_test(NAME PreferencesModelTests COMMAND PreferencesModelTests)

add_executable(SettingsModelTests
    ${SEAMLYLAYOUT_TEST_DIR}/SettingsModelTests.cpp
    src/SettingsModel.cpp
    src/Logger.cpp
)

target_include_directories(SettingsModelTests PRIVATE
    src/
)

target_link_libraries(SettingsModelTests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    Qt6::Widgets
)

add_test(NAME SettingsModelTests COMMAND SettingsModelTests)

# Task 49 — the command-line contract with Seamly2D's Layout Mode handoff.
# StartupOptions is a plain value type with no QObject and no GUI, so this
# suite needs neither the offscreen platform nor a window.
add_executable(StartupOptionsTests
    ${SEAMLYLAYOUT_TEST_DIR}/StartupOptionsTests.cpp
    src/StartupOptions.cpp
)

target_include_directories(StartupOptionsTests PRIVATE
    src/
)

target_link_libraries(StartupOptionsTests PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME StartupOptionsTests COMMAND StartupOptionsTests)

# On Windows the test executables are launched by ctest straight out of the
# build directory, with no windeployqt output beside them. Qt's DLLs and the
# platform plugin are then only reachable through PATH, so a bare
# `ctest --preset debug` in a shell that has not sourced the kit fails at
# startup - either "the code execution cannot proceed because Qt6Cored.dll was
# not found", or, once the DLLs resolve, "could not find or load the Qt
# platform plugin". Prepend the kit's own bin directory for the duration of the
# test run so ctest works without any external environment setup.
#
# `$<TARGET_FILE_DIR:Qt6::Core>` is the directory CMake actually found Qt in,
# so this follows the kit rather than pinning a version. Linux/macOS need no
# equivalent: CI installs Qt onto the loader path, and RPATH covers local runs.
if(WIN32)
    set_tests_properties(
        AdjustSceneTests
        AdjustControllerTests
        PreferencesModelTests
        SettingsModelTests
        StartupOptionsTests
        PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>"
    )
endif()

# ---------------------------------------------------------------------------
# Platform-specific libraries
# ---------------------------------------------------------------------------
if(WIN32)
    # dwmapi for Windows title bar color customization
    target_link_libraries(SeamlyLayout PRIVATE dwmapi)
elseif(APPLE)
    # Cocoa framework for macOS title bar color customization
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    target_link_libraries(SeamlyLayout PRIVATE ${COCOA_FRAMEWORK})
endif() # WIN32 / APPLE

# ---------------------------------------------------------------------------
# Phase 2g: link Rust core library once ffi_bridge crate is built
# Uncomment when libseamly_core is available:
#
# set(SEAMLY_CORE_LIB "${CMAKE_SOURCE_DIR}/../target/release/seamly_core")
# if(WIN32)
#     target_link_libraries(SeamlyLayout PRIVATE "${SEAMLY_CORE_LIB}.dll.lib")
# elseif(APPLE)
#     target_link_libraries(SeamlyLayout PRIVATE "${SEAMLY_CORE_LIB}.dylib")
# else()
#     target_link_libraries(SeamlyLayout PRIVATE "${SEAMLY_CORE_LIB}.so")
# endif()
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Install rules (for windeployqt / macdeployqt packaging — Phase 16)
# ---------------------------------------------------------------------------
install(TARGETS SeamlyLayout
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---------------------------------------------------------------------------
# Packaged runtime defaults — installed next to the executable so the
# first-run migration code can seed user AppConfig from them.
#
#   <installBin>/settings/  — default_settings.json + sample settings files
#
# preferences.json is a per-user file and must NOT be bundled; user
# preferences are seeded at first run from the embedded Qt resource
# :/defaults/default_preferences.json instead.
# ---------------------------------------------------------------------------
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/settings/"
    DESTINATION ${CMAKE_INSTALL_BINDIR}/settings
    FILES_MATCHING
        PATTERN "*.json"
        PATTERN "preferences.json" EXCLUDE
)

if(UNIX AND NOT APPLE)
    install(FILES "${SEAMLY_ICON_PNG}"
        DESTINATION share/icons/hicolor/512x512/apps
        RENAME SeamlyLayout.png
    )
    install(FILES "${SEAMLY_ICON_SVG}"
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME SeamlyLayout.svg
    )
    install(FILES "${SEAMLY_ICON_PNG}"
        DESTINATION share/pixmaps
        RENAME SeamlyLayout.png
    )
endif()

qt_generate_deploy_qml_app_script(
    TARGET SeamlyLayout
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
